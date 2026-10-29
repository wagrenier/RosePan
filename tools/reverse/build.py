"""Build (and optionally smoke-run) RosePan, with the toolchain set up and errors summarised.

python build.py [--config relwithdebinfo|debug|release] [--run SECONDS [--shot-every N] [--pad SCRIPT]
                [--env NAME=VALUE ...]] [--log FILE]

- Puts the MinGW bin directory on PATH (without it cc1plus fails with no diagnostics).
- Configures the build directory on first use, reusing a sibling ReiPan / MioPan build's fetched
  dependency sources when present (no re-download).
- Runs ninja; retries once when parallel shadercross runs collide on a Windows file lock.
- Refuses raw NUL bytes in src/ (MinGW compiles a quoted NUL, MSVC fails C2137).
- Prints unique compile errors and undefined references. Exit code = build result.
- --run N starts the exe for N seconds and prints stub hits, warnings and the log tail; the full output
  goes to work/run_<config>.log (tus.py ranks reached stubs first). The platform layer saves a screenshot
  every --shot-every presented frames (default 60) to work/shots/shot_<frame>.png + last.png
  (ROSEPAN_SHOT_DIR / ROSEPAN_SHOT_EVERY); $ROSEPAN_DATA (else tus.json settings.data_dir) is passed on as ROSEPAN_DATA_DIR.
- --pad SCRIPT scripts pad 0 for the run ("<read>:<BUTTON>[*<reads>],...": hold BUTTON 3 pad reads, or
  <reads>, from the given pad read, about one per game frame; LSUP/LSDOWN/LSLEFT/LSRIGHT push the left
  stick) and masks the host's pads and keyboard (ROSEPAN_AUTOPAD, src/sdk/libpad.cpp). Without --pad,
  $ROSEPAN_AUTOPAD (if set) is passed on; `--pad none` runs with host input.
- --env NAME=VALUE passes extra variables to the game (debug hooks a session adds: a warp, a probe).
Override paths with ROSEPAN_MINGW_BIN / ROSEPAN_NINJA.
"""
import os
import re
import subprocess
import sys
from pathlib import Path

import rosepan

sys.stdout.reconfigure(encoding='utf-8', errors='replace')

MINGW = rosepan.MINGW_BIN
NINJA = rosepan.NINJA
EXE = rosepan.EXE_NAME
config = sys.argv[sys.argv.index('--config') + 1] if '--config' in sys.argv else 'relwithdebinfo'
build_type = {'relwithdebinfo': 'RelWithDebInfo', 'debug': 'Debug', 'release': 'Release'}[config]
bdir = rosepan.REPO / f'cmake-build-{config}'
log = Path(sys.argv[sys.argv.index('--log') + 1]) if '--log' in sys.argv else rosepan.WORK / f'build_{config}.log'
log.parent.mkdir(parents=True, exist_ok=True)
env = dict(os.environ, PATH=MINGW + os.pathsep + os.environ.get('PATH', ''))

if not (rosepan.REPO / 'CMakeLists.txt').exists():
    raise SystemExit('no CMakeLists.txt: the PC project is not bootstrapped yet (host task in tus.json)')

if not (bdir / 'build.ninja').exists():
    cmd = ['cmake', '-S', str(rosepan.REPO), '-B', str(bdir), '-G', 'Ninja', f'-DCMAKE_BUILD_TYPE={build_type}',
           f'-DCMAKE_CXX_COMPILER={MINGW}\\g++.exe', f'-DCMAKE_C_COMPILER={MINGW}\\gcc.exe',
           f'-DCMAKE_MAKE_PROGRAM={NINJA}']
    for sibling in (rosepan.REIPAN, rosepan.MIOPAN):
        deps = sibling / f'cmake-build-{config}' / '_deps'
        if not deps.exists():
            deps = sibling / 'cmake-build-relwithdebinfo' / '_deps'
        if deps.exists():
            for name in ('SDL3', 'SPDLOG', 'CGLM', 'IMGUI', 'IMPLOT'):
                src = deps / f'{name.lower()}-src'
                if src.exists():
                    cmd.append(f'-DFETCHCONTENT_SOURCE_DIR_{name}={src}')
            break
    print('configuring', bdir.name)
    r = subprocess.run(cmd, env=env, capture_output=True, text=True)
    if r.returncode:
        print(r.stdout[-3000:], r.stderr[-3000:])
        raise SystemExit('configure failed')
else:
    # pick up added/removed source files (the CMake globs are CONFIGURE_DEPENDS, but be explicit)
    subprocess.run(['cmake', str(bdir)], env=env, capture_output=True)


# A raw NUL byte in a source file (a shell-written backslash-zero that landed unescaped): GCC takes the
# quoted NUL as a character, MSVC stops on C2137 "empty character constant". MinGW never shows it.
nul = [f'{p.relative_to(rosepan.REPO)}:{i}' for p in sorted((rosepan.REPO / 'src').rglob('*'))
       if p.suffix in ('.c', '.cpp', '.h', '.hpp', '.inl')
       for i, line in enumerate(p.read_bytes().split(b'\n'), 1) if bytes(1) in line]
if nul:
    print('raw NUL bytes (MSVC rejects them; write the escape instead):', ', '.join(nul))
    raise SystemExit('build FAILED (NUL bytes in sources)')


def build():
    with open(log, 'w', encoding='utf-8', errors='replace') as f:
        return subprocess.run([NINJA, '-C', str(bdir), EXE], env=env, stdout=f, stderr=subprocess.STDOUT).returncode


rc = build()
text = log.read_text(encoding='utf-8', errors='replace')
if rc and 'being used by another process' in text:
    print('shadercross file-lock collision; retrying once')
    rc = build()
    text = log.read_text(encoding='utf-8', errors='replace')

errors = sorted(set(re.findall(r'error: .*', text)))
undefined = sorted(set(re.findall(r"undefined reference to [`'](.*?)'", text)))
for e in errors[:60]:
    print(e.replace(str(rosepan.REPO).replace('\\', '/') + '/', ''))
if undefined:
    print(f'{len(undefined)} undefined references (python stubgen.py --from-build {log}):')
    for u in undefined[:80]:
        print('   ', u)
print(f'build {"OK" if rc == 0 else "FAILED"} ({bdir.name}); log: {log}')
if rc:
    sys.exit(rc)

if '--run' in sys.argv:
    secs = int(sys.argv[sys.argv.index('--run') + 1])
    exe = bdir / EXE / f'{EXE}.exe'
    shots = rosepan.WORK / 'shots'
    shots.mkdir(parents=True, exist_ok=True)
    for old in shots.glob('*.png'):
        old.unlink()
    every = sys.argv[sys.argv.index('--shot-every') + 1] if '--shot-every' in sys.argv else '60'
    run_env = dict(env, ROSEPAN_SHOT_DIR=str(shots), ROSEPAN_SHOT_EVERY=every)
    if 'ROSEPAN_DATA_DIR' not in run_env and rosepan.GAME_DATA is not None:
        run_env['ROSEPAN_DATA_DIR'] = str(rosepan.GAME_DATA)
    if '--pad' in sys.argv:
        pad = sys.argv[sys.argv.index('--pad') + 1]
        if pad == 'none':
            run_env.pop('ROSEPAN_AUTOPAD', None)
        else:
            run_env['ROSEPAN_AUTOPAD'] = pad
    if run_env.get('ROSEPAN_AUTOPAD') is not None:
        print(f'scripted pad: {run_env["ROSEPAN_AUTOPAD"]!r}')
    for i, a in enumerate(sys.argv):
        if a == '--env' and i + 1 < len(sys.argv) and '=' in sys.argv[i + 1]:
            k, v = sys.argv[i + 1].split('=', 1)
            run_env[k] = v
            print(f'{k}: {v!r}')
    try:
        r = subprocess.run([str(exe)], cwd=exe.parent, env=run_env, capture_output=True, text=True,
                           timeout=secs, encoding='utf-8', errors='replace')
        out = r.stdout + r.stderr
        print(f'exited early with code {r.returncode}' + (' (access violation: python gdbrun.py)'
                                                          if r.returncode == 3221225477 else ''))
    except subprocess.TimeoutExpired as t:
        out = (t.stdout or b'').decode('utf-8', 'replace') if isinstance(t.stdout, bytes) else (t.stdout or '')
        print(f'still running after {secs}s (stopped)')
    (rosepan.WORK / f'run_{config}.log').write_text(out, encoding='utf-8')
    hits = [l for l in out.splitlines() if '[stub]' in l or '[W]' in l or '[E]' in l or 'ASSERT' in l]
    print(f'{len(hits)} stub/warning/error lines:')
    for l in hits[:60]:
        print('   ', l)
    print('--- log tail ---')
    print('\n'.join(out.splitlines()[-15:]))
    taken = sorted(p.name for p in shots.glob('shot_*.png'))
    if taken:
        print(f'--- {len(taken)} screenshots in {shots} (every {every} frames): {", ".join(taken)}')
        print(f'    look at {shots / "last.png"} (the Read tool shows images)')
    else:
        print('--- no screenshots (nothing presented, or the renderer never started)')
