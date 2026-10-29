"""Find functions Ghidra folded into the body before them (hazards.md "A body swallows the
unreferenced function after it").

python hidden.py <lo> <hi>     scan a range (a TU's extent), read-only

Prints two kinds of candidates:
- code right after a `jr ra` / `j` / `b` (and its delay slot, nops skipped) that no branch in the range
  targets and that does not start a known function: a new entry (rx's slot-check wrappers, 2026-10-22;
  `b` only as a spin -- a backward `b` over nothing but nops, the assert handlers' endless loop, whose
  next function Ghidra folds in: debug/mimu_debug.cpp 2026-10-29; or any `b` whose next code is a frame
  setup `addiu sp, sp, -N`: i3d/i3dgeom.cpp 2026-10-29);
- `jal` targets inside the range that do not start a known function (a `j` there is usually a
  loop branch; check tail jumps by hand).
Check each with `ghidra.py asm`, then `ghidra.py mkfunc <addr>` and name it in a batch.
"""
import re
import subprocess
import sys

import rosepan


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
    out = subprocess.run([sys.executable, str(rosepan.TOOLS / 'ghidra.py'), 'asm', hex(lo), hex(hi)],
                         capture_output=True, text=True).stdout
    ins = []
    for line in out.splitlines():
        m = re.match(r'([0-9a-f]{8}): (.*)', line)
        if m:
            ins.append((int(m.group(1), 16), m.group(2)))
    targets = set()
    for _, text in ins:
        for m in re.finditer(r'0x([0-9a-f]{8})', text):
            targets.add(int(m.group(1), 16))
    starts = set()
    for line in rosepan.FUNCTIONS_TXT.read_text(encoding='utf-8').splitlines():
        m = re.match(r'/\* ([0-9a-f]{8}) ', line)
        if m:
            starts.add(int(m.group(1), 16))

    print('-- code after a return that nothing branches to')
    for i, (_, text) in enumerate(ins):
        if text.startswith('b 0x'):
            # only a spin: a backward b over nops (other b's land after jump-table cases), or a b
            # into the epilogue followed by a frame setup (i3dgeom 0x236348 / 0x236870, 2026-10-29)
            t = int(text[4:], 16)
            j = i + 2
            while j < len(ins) and ins[j][1] == 'nop':
                j += 1
            prologue = j < len(ins) and ins[j][1].startswith('addiu sp, sp, -')
            if not prologue and not (t <= ins[i][0] and all(x == 'nop' for a, x in ins if t <= a < ins[i][0])):
                continue
        if text.startswith(('jr ra', 'j 0x', 'b 0x')):
            j = i + 2
            while j < len(ins) and ins[j][1] == 'nop':
                j += 1
            if j < len(ins):
                addr = ins[j][0]
                if addr not in targets and addr not in starts:
                    print('%08x  %s' % (addr, ins[j][1]))
    print('-- jal targets inside the range that start no function')
    for addr, text in ins:
        m = re.match(r'jal 0x([0-9a-f]{8})', text)
        if m:
            target = int(m.group(1), 16)
            if lo <= target < hi and target not in starts:
                print('%08x  %s  (from %08x)' % (target, text, addr))
    return 0


if __name__ == '__main__':
    sys.exit(main())
