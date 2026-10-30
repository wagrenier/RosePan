"""RXSNDDRV (the IOP sound driver) offline: decompile every function of the rxsnddrv.elf program once.

python iopdec.py dump           every function of rxsnddrv.elf -> work/iop/dec/<addr>.c, plus
                                work/iop/dec/index.txt (addr, extent to the next function, name, the
                                FUN_ callees, the strings it prints); needs Ghidra
python iopdec.py show ADDR ...  the cached decompilation (offline)
python iopdec.py index [FILTER] the index lines (offline), FILTER a substring of the line

The TU tools only know SLUS_214.48 and key their caches by address alone, so the IOP program is
read here, into its own directory, never through fetch / features / the name ledger (hazards.md).
RXSNDDRV is tracked by hand: src/iop/rxsnddrv/ annotates each function IOP 0x<addr> (open-items.md
"Decisions", 2026-10-30).
"""
import os
import re
import sys

os.environ['ROSEPAN_PROGRAM'] = 'rxsnddrv.elf'
import rosepan  # noqa: E402  (reads ROSEPAN_PROGRAM at import)

sys.stdout.reconfigure(encoding='utf-8', errors='replace')
OUT = rosepan.WORK / 'iop' / 'dec'


def dump():
    OUT.mkdir(parents=True, exist_ok=True)
    names = rosepan.function_names()
    addrs = sorted(int(a, 16) for a in names)
    lines = []
    for i, a in enumerate(addrs):
        name = names[f'{a:08x}']
        nxt = addrs[i + 1] if i + 1 < len(addrs) else 0xd0f0
        code = rosepan.decompile(a)
        (OUT / f'{a:05x}.c').write_text(code + '\n', encoding='utf-8')
        callees = sorted(set(re.findall(r'FUN_0000([0-9a-f]{4,5})\(', code)) - {f'{a:04x}'})
        strs = re.findall(r'"((?:[^"\\]|\\.)*)"', code)
        lines.append(f'{a:05x} {nxt - a:5x} {name:28s} calls {",".join(c.lstrip("0") for c in callees) or "-"}'
                     + (f'  | {" | ".join(s[:60] for s in strs)}' if strs else ''))
        print(lines[-1])
    (OUT / 'index.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8')


def main(argv):
    cmd = argv[0] if argv else 'index'
    if cmd == 'dump':
        dump()
    elif cmd == 'show':
        for a in argv[1:]:
            path = OUT / f'{int(a, 16):05x}.c'
            print(f'===== IOP {a}')
            print(path.read_text(encoding='utf-8') if path.exists() else '(not cached: python iopdec.py dump)')
    elif cmd == 'index':
        flt = argv[1] if len(argv) > 1 else ''
        for line in (OUT / 'index.txt').read_text(encoding='utf-8').splitlines():
            if flt in line:
                print(line)
    else:
        print(__doc__)


if __name__ == '__main__':
    main(sys.argv[1:])
