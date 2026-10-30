# gdb side of gdbrun.py (gdb runs it with -x; not a CLI). Runs the game, lets first-chance
# faults inside DLLs pass once (drivers probe memory under SEH), and on a real fault -- or a
# manual break (gdbrun.py --break) -- writes the state to crash.txt and holds the stopped game,
# serving gdb commands from cmd.txt (results in out.txt) until "quit" or "continue".
import os
import time

import gdb

DIR = os.environ['ROSEPAN_GDB_DIR']
CMD = os.path.join(DIR, 'cmd.txt')
OUT = os.path.join(DIR, 'out.txt')
CRASH = os.path.join(DIR, 'crash.txt')
BREAK = os.path.join(DIR, 'break.flag')
CRASH_SIGNALS = {'SIGSEGV', 'SIGILL', 'SIGFPE', 'SIGABRT', 'SIGBUS'}

state = {'stop': None, 'exited': None}
gdb.events.stop.connect(lambda ev: state.__setitem__('stop', ev))
gdb.events.exited.connect(lambda ev: state.__setitem__('exited', getattr(ev, 'exit_code', None)))


def run(cmd):
    # Not only gdb.error: an exception escaping this script ends gdb -batch and the held game.
    try:
        return gdb.execute(cmd, to_string=True)
    except Exception as e:
        if isinstance(e, UnicodeError) or 'codec' in str(e):
            return run_logged(cmd)
        return f'<error: {e}>\n'


def run_logged(cmd):
    """cmd's output through gdb's log file, read as bytes: to_string decodes it as UTF-8, and the
    game's SJIS text (a `bt full` local) is not."""
    path = os.path.join(DIR, 'gdbout.tmp').replace('\\', '/')
    try:
        for c in (f'set logging file {path}', 'set logging overwrite on', 'set logging redirect on',
                  'set logging enabled on'):
            gdb.execute(c)
        try:
            gdb.execute(cmd)
        finally:
            gdb.execute('set logging enabled off')
        with open(path, 'rb') as f:
            return f.read().decode('utf-8', 'replace')
    except Exception as e:
        return f'<error: {e}>\n'


def resume(cmd):
    """run / continue; returns the error when gdb reports one (the inferior may still be stopped)."""
    state['stop'] = None
    try:
        gdb.execute(cmd)
        return None
    except Exception as e:
        log(f'{cmd}: {e}')
        return e


def log(msg):
    print(f'[gdbdrive] {msg}', flush=True)


def write(path, text):
    with open(path + '.tmp', 'w', encoding='utf-8') as f:
        f.write(text)
    os.replace(path + '.tmp', path)


def dump(reason):
    parts = [f'=== {reason}']
    for c in ('info registers rip rsp rbp rax rbx rcx rdx rsi rdi r8 r9 r10 r11 r12 r13 r14 r15',
              'x/12i $pc-24', 'bt 40', 'bt full 12', 'info threads', 'thread apply all bt 12'):
        parts.append(f'--- {c}\n{run(c)}')
    write(CRASH, '\n'.join(parts))
    log(f'state written to {CRASH}')


def serve():
    """Runs the commands written to cmd.txt (one per line) into out.txt; returns on quit / continue."""
    log('holding the game; commands from cmd.txt')
    while True:
        if os.path.exists(CMD):
            time.sleep(0.2)
            with open(CMD, encoding='utf-8') as f:
                cmds = [l.strip() for l in f if l.strip()]
            os.remove(CMD)
            out = []
            for c in cmds:
                if c in ('quit', 'kill'):
                    run('kill')
                    write(OUT, 'killed\n')
                    return 'quit'
                if c in ('continue', 'c'):
                    write(OUT, 'continuing\n')
                    return 'continue'
                out.append(f'(gdb) {c}\n{run(c)}')
            write(OUT, '\n'.join(out))
        time.sleep(0.3)


def main():
    last_dll_fault = None
    for c in ('set pagination off', 'set print pretty on', 'set confirm off'):
        run(c)
    log('starting game')
    err = resume('run')
    while True:
        if state['exited'] is not None or not gdb.selected_inferior().pid:
            log(f'game exited (code {state["exited"]})')
            return
        ev = state['stop']
        sig = getattr(ev, 'stop_signal', None)
        pc = int(gdb.parse_and_eval('$pc'))
        where = gdb.solib_name(pc) or 'RosePan.exe'
        reason = None
        if os.path.exists(BREAK):
            os.remove(BREAK)
            reason = f'manual break ({sig}) at {pc:#x} in {where}'
        elif isinstance(ev, gdb.BreakpointEvent):
            reason = f'breakpoint at {pc:#x} in {where}'
        elif sig in CRASH_SIGNALS:
            if where != 'RosePan.exe' and last_dll_fault != pc:
                last_dll_fault = pc
                log(f'first-chance {sig} at {pc:#x} in {where}: passed on\n{run("bt 8")}')
            else:
                reason = f'{sig} at {pc:#x} in {where}'
        elif ev is None and err is not None:
            reason = f'stopped at {pc:#x} in {where} after a gdb error: {err}'
        else:
            log(f'stop ({sig}) at {pc:#x} in {where}: continuing')
        if reason:
            dump(reason)
            if serve() == 'quit':
                return
        err = resume('continue')


main()
