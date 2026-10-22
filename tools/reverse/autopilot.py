"""Unattended porting: one fresh Claude Code session per translation unit (TU).

python autopilot.py [--count N] [--tu TU] [--model M] [--effort LEVEL] [--max-usd X]
                    [--mode acceptEdits|auto] [--hours H] [--limit-wait MIN] [--dry-run]

Run it yourself, from a terminal: it starts unattended sessions. Permissions are deny-by-default:
file edits inside the repo, this project's tools, read-only git and Ghidra reads (see ALLOWED below);
`tus.py finish` is the only way a session commits.

Each round:
  1. preflight: on a work branch (never main), clean tree, Ghidra up with RoR open, no work/STOP file
  2. `tus.py next --claim` (or --tu) picks the TU
  3. `claude -p "/rosepan-port tu <TU>"` runs a NEW headless session with the skill; its stream goes to
     work/sessions/<time>_<tu>.jsonl and a one-line-per-step trace to the console
  4. a session stopped by a usage limit waits --limit-wait minutes (default 30) and is resumed
     (`--resume <session id>`), up to 6 times
  5. verify: the session finished its TU (no claim left active), committed everything, build is green
  6. record the round in work/sessions/index.tsv
Stops after N rounds (default 1), when the queue is empty, when work/STOP exists (create it to stop
after the current session), or at the first round that does not verify -- the tree is then left as the
session left it, for a look (`claude --resume <session id>` reopens the dialogue).

The claude executable: $ROSEPAN_CLAUDE, else `claude` on PATH, else ~/.local/bin/claude.exe (the native
installer), else the newest one bundled with the Claude desktop app (%APPDATA%/Claude/claude-code/*/*/claude.exe).
"""
import glob
import json
import os
import re
import shutil
import subprocess
import sys
import time
import uuid
from pathlib import Path

import rosepan

sys.stdout.reconfigure(encoding='utf-8', errors='replace')
HERE = Path(__file__).resolve().parent
SESSIONS = rosepan.WORK / 'sessions'
STOP = rosepan.WORK / 'STOP'
LOCK = rosepan.WORK / 'autopilot.lock'

# What a TU session may do without a person: edit files in this repo (acceptEdits), run this project's
# own tools, read git state and read Ghidra. Commits happen only inside `tus.py finish` (build-gated,
# refuses main). Anything not listed is denied (--permission-prompts none), never prompted, so a session
# cannot hang and cannot do more than this list.
TOOLS = ('tus', 'stage', 'build', 'stubgen', 'callcheck', 'layoutcheck', 'refresh', 'export', 'status', 'ctxcard',
         'name_ctx', 'modview', 'dec', 'ghidra', 'rtti', 'tuid', 'sections', 'libmatch', 'gcc2', 'mipsdis', 'mipsemu', 'sinit',
         'datadump', 'tm2png', 'restore_names', 'features', 'fetch', 'gdbrun')
GHIDRA_READ = ('decompile_function', 'disassemble_function', 'get_function_callers', 'get_function_callees',
               'get_xrefs_to', 'get_xrefs_from', 'get_function_by_address', 'list_strings', 'search_strings',
               'search_functions', 'get_function_signature', 'read_memory', 'inspect_memory_content',
               'list_data_items', 'get_assembly_context', 'batch_decompile')
ALLOWED = (['Read', 'Edit', 'Write', 'Glob', 'Grep', 'Skill']
           + [f'Bash(python {p}{t}.py*)' for t in TOOLS for p in ('', 'tools/reverse/')]
           + ['Bash(cd *)', 'Bash(ls *)', 'Bash(head *)', 'Bash(tail *)', 'Bash(grep *)', 'Bash(wc *)',
              'Bash(git status*)', 'Bash(git diff*)', 'Bash(git log*)', 'Bash(git show*)',
              'Bash(git checkout -- *)']
           + [f'mcp__ghidra__{t}' for t in GHIDRA_READ])
DENIED = ['Bash(git push*)', 'Bash(git reset*)', 'Bash(git rebase*)', 'Bash(git switch*)', 'Bash(git branch*)',
          'Bash(git clean*)', 'Bash(git commit*)', 'Bash(rm *)', 'PowerShell']

RULES = """RosePan autopilot. This is an unattended, headless session started by tools/reverse/autopilot.py,
one in a chain: each translation unit gets its own fresh session, and the rosepan-port skill plus
tools/reverse/data/tus.json carry everything between sessions.

- Nobody is watching. Never wait for input or ask questions: decide, and write doubts and open
  questions into .claude/skills/rosepan-port/references/open-items.md (or, if writes under .claude/ are
  denied, into the --notes of tus.py finish after "For the references:").
- Work on exactly one TU: the one in the prompt (or the one you `tus.py claim` if boundary work shows
  the slice belongs to another file). Follow the skill's "One TU per session" protocol to the end.
- End by running `python tus.py finish <tu> --status done|verify|partial|blocked --summary ... --notes ...
  --coauthor "<the Co-Authored-By line from your session instructions>"` from tools/reverse. It
  builds, refreshes status, records the session and commits. Every commit must build.
- Leave the tree clean (finish commits everything). Do not push, switch branches, rewrite history,
  or touch files outside this repository (../ReiPan and ../MioPan are read-only references).
- Permissions are a fixed list: edit files in the repo, run the tools in tools/reverse
  (`python <tool>.py ...` from tools/reverse, or `python tools/reverse/<tool>.py ...` from the root),
  read-only git and Ghidra MCP reads. Naming goes through name_ctx.py, function creation through
  `ghidra.py mkfunc`, commits through tus.py finish. A denied tool call is final: use the listed tools
  instead of retrying variants. No backticks in --notes / --summary (the shell runs them).
- If you cannot finish the slice, keep what builds, revert what does not, and finish with
  --status partial (work remains) or blocked (cannot proceed without a decision), with notes the next
  session can act on.
"""


def opt(name, default=None):
    if name in sys.argv:
        return sys.argv[sys.argv.index(name) + 1]
    return default


def find_claude():
    exe = os.environ.get('ROSEPAN_CLAUDE') or shutil.which('claude')
    if exe:
        return exe
    native = Path.home() / '.local' / 'bin' / 'claude.exe'          # the native installer's location
    if native.exists():
        return str(native)
    found = glob.glob(os.path.join(os.environ.get('APPDATA', ''), 'Claude', 'claude-code', '*', '*', 'claude.exe'))
    if not found:
        raise SystemExit('claude executable not found; set ROSEPAN_CLAUDE')

    def ver(p):
        return tuple(int(x) if x.isdigit() else 0 for x in Path(p).parent.parent.name.split('.'))
    return max(found, key=ver)


def git(*args):
    return subprocess.run(['git', '-C', str(rosepan.REPO), *args], capture_output=True, text=True,
                          encoding='utf-8', errors='replace').stdout.strip()


def tus(*args):
    r = subprocess.run([sys.executable, 'tus.py', *args], cwd=HERE, capture_output=True, text=True,
                       encoding='utf-8', errors='replace')
    return r.returncode, r.stdout


def preflight():
    problems = []
    branch = git('rev-parse', '--abbrev-ref', 'HEAD')
    if branch in ('main', 'master', 'HEAD', ''):
        problems.append(f'on {branch or "no branch"}: check out a work branch first')
    if git('status', '--porcelain'):
        problems.append('working tree is not clean (commit or stash first)')
    if not rosepan.ghidra_up():
        problems.append(f'Ghidra (GhidraMCP on {rosepan.GHIDRA}) is not reachable or {rosepan.PROGRAM} is not open')
    if STOP.exists():
        problems.append(f'{STOP} exists (delete it to run)')
    return branch, problems


def active_claims():
    db = rosepan.load_json(rosepan.TUS_JSON, {})
    return [k for k, e in db.get('tus', {}).items() if e.get('status') == 'active']


def summarize(ev):
    """One console line for a stream-json event, or None."""
    if ev.get('type') == 'assistant':
        for c in ev.get('message', {}).get('content', []):
            if c.get('type') == 'tool_use':
                inp = c.get('input', {})
                arg = inp.get('command') or inp.get('file_path') or inp.get('pattern') or inp.get('skill') or ''
                if not arg and inp:
                    arg = json.dumps(inp)[:120]
                return f'  {c["name"]}: {str(arg).splitlines()[0][:150] if arg else ""}'
            if c.get('type') == 'text' and c.get('text', '').strip():
                return '  > ' + c['text'].strip().splitlines()[0][:150]
    return None


def limited(result):
    """True when the session stopped on a usage / rate limit (worth waiting out and resuming)."""
    if not result or not result.get('is_error'):
        return False
    text = json.dumps(result).lower()
    return bool(re.search(r'usage limit|rate limit|limit reached|overloaded|429', text))


def run_session(claude, tu, sid, log_path, args, resume=False):
    prompt = 'Continue the TU where you left off (the session stopped on a usage limit).' if resume \
        else f'/rosepan-port tu {tu}'
    cmd = [claude, '-p', prompt, *(['--resume', sid] if resume else ['--session-id', sid]),
           '--name', f'RosePan TU {tu}', '--output-format', 'stream-json', '--verbose',
           '--permission-mode', args['mode'], '--permission-prompts', 'none',
           '--append-system-prompt', RULES,
           '--allowedTools', *ALLOWED, '--disallowedTools', *DENIED]
    if args['model']:
        cmd += ['--model', args['model']]
    if args['effort']:
        cmd += ['--effort', args['effort']]
    if args['max_usd']:
        cmd += ['--max-budget-usd', args['max_usd']]
    env = dict(os.environ, ROSEPAN_SESSION_ID=sid, ROSEPAN_AUTOPILOT='1')
    result, start = None, time.time()
    with open(log_path, 'a', encoding='utf-8') as log:
        p = subprocess.Popen(cmd, cwd=rosepan.REPO, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, text=True, encoding='utf-8', errors='replace')
        try:
            for line in p.stdout:
                log.write(line)
                log.flush()
                try:
                    ev = json.loads(line)
                except ValueError:
                    print('  ' + line.rstrip()[:200])
                    continue
                if ev.get('type') == 'result':
                    result = ev
                s = summarize(ev)
                if s:
                    print(time.strftime('%H:%M:%S') + s, flush=True)
                if time.time() - start > args['hours'] * 3600:
                    print(f'session exceeded {args["hours"]} h; stopping it')
                    p.kill()
                    break
            p.wait()
        except KeyboardInterrupt:
            p.kill()
            raise
    return p.returncode, result, time.time() - start


def main():
    args = {'count': int(opt('--count', 1)), 'tu': opt('--tu'), 'model': opt('--model'),
            'effort': opt('--effort', 'xhigh'), 'max_usd': opt('--max-usd'), 'mode': opt('--mode', 'acceptEdits'),
            'hours': float(opt('--hours', 4)), 'limit_wait': float(opt('--limit-wait', 30))}
    dry = '--dry-run' in sys.argv
    claude = find_claude()
    SESSIONS.mkdir(parents=True, exist_ok=True)
    if LOCK.exists() and time.time() - LOCK.stat().st_mtime < 6 * 3600:
        raise SystemExit(f'another autopilot seems to be running ({LOCK}); delete the file if not')
    print(f'claude: {claude}')
    index = SESSIONS / 'index.tsv'
    if not index.exists():
        index.write_text('time\ttu\tsession\tresult\tturns\tcost_usd\tminutes\tcommit\n', encoding='utf-8')
    LOCK.write_text(str(os.getpid()), encoding='utf-8')
    try:
        for i in range(args['count']):
            branch, problems = preflight()
            if problems:
                print('not starting a session:\n  - ' + '\n  - '.join(problems))
                return 1
            if args['tu'] and i == 0:
                rc, out = tus('claim', args['tu'])
                tu = args['tu']
            else:
                rc, out = tus('next', '--json', *([] if dry else ['--claim']))
                info = json.loads(out.strip().splitlines()[-1]) if out.strip() else {}
                tu = info.get('tu')
                if not tu:
                    print('queue empty:', info.get('reason'))
                    return 0
                print(f'[{i + 1}/{args["count"]}] {tu}: {info.get("reason")}')
            if rc and args['tu']:
                print(out)
                return 1
            if dry:
                print(f'dry run: would start a session for {tu} on {branch}')
                return 0
            sid = str(uuid.uuid4())
            stamp = time.strftime('%Y%m%d_%H%M%S')
            log_path = SESSIONS / f'{stamp}_{tu.replace("/", "_")}.jsonl'
            head = git('rev-parse', 'HEAD')
            print(f'session {sid} -> {log_path.relative_to(rosepan.REPO).as_posix()}')
            rc, result, secs = run_session(claude, tu, sid, log_path, args)
            for _ in range(6):
                if not limited(result) or STOP.exists():
                    break
                print(f'usage limit: waiting {args["limit_wait"]:.0f} min, then resuming {sid}')
                time.sleep(args['limit_wait'] * 60)
                rc, result, more = run_session(claude, tu, sid, log_path, args, resume=True)
                secs += more
            # ---- verify ------------------------------------------------------------
            verdict = []
            if rc:
                verdict.append(f'claude exited with {rc}')
            if result and result.get('is_error'):
                verdict.append(f'session ended with an error: {result.get("subtype")}')
            left = active_claims()
            if left:
                verdict.append('TU not finished (still active: ' + ', '.join(left) + ')')
            if git('status', '--porcelain'):
                verdict.append('uncommitted changes left behind')
            if git('rev-parse', 'HEAD') == head:
                verdict.append('no commit')
            if (rosepan.REPO / 'CMakeLists.txt').exists():
                b = subprocess.run([sys.executable, 'build.py'], cwd=HERE, capture_output=True, text=True,
                                   encoding='utf-8', errors='replace')
                if b.returncode:
                    verdict.append('build is red after the session')
            cost = (result or {}).get('total_cost_usd', 0) or 0
            turns = (result or {}).get('num_turns', '')
            ok = not verdict
            with open(index, 'a', encoding='utf-8') as f:
                f.write(f'{stamp}\t{tu}\t{sid}\t{"ok" if ok else "; ".join(verdict)}\t{turns}\t{cost:.2f}\t'
                        f'{secs / 60:.0f}\t{git("rev-parse", "--short", "HEAD")}\n')
            print(f'[{i + 1}/{args["count"]}] {tu}: {"OK" if ok else "FAILED: " + "; ".join(verdict)} '
                  f'({secs / 60:.0f} min, {turns} turns, ${cost:.2f})')
            if result and result.get('result'):
                print('  final message: ' + result['result'].strip()[:1500].replace('\n', '\n  '))
            if not ok:
                print(f'stopping. Inspect with: git status / git log; reopen the dialogue: claude --resume {sid}')
                return 1
            if STOP.exists():
                print(f'{STOP} found: stopping after this session')
                return 0
        return 0
    finally:
        LOCK.unlink(missing_ok=True)


if __name__ == '__main__':
    sys.exit(main())
