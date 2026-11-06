"""Offline disassembler for the sequence script images (ELFR101) inside the script packs.

The scripts are MIPS code the game's VM interprets (architecture.md "The sequence script VM"); this
prints one with what the compiler's conventions make readable: natives by number (a jal below the code
is native (t - 0x144) / 0x40), strings at their load, writes to the script globals and bits.

    python seqdis.py W001                      the stage script (data/SEQ/W001/W000.WRD)
    python seqdis.py W001 A168                 room 168's script
    python seqdis.py G000                      the system script
    python seqdis.py W001 A168 --grep ev1new04 only the functions mentioning the text
    python seqdis.py W001 A168 --func 0x8804   one function
    python seqdis.py W001 --strings            the image's strings

The data root comes from rosepan.py (ROSEPAN_DATA).
"""
import argparse
import os
import re
import struct
import sys

import mipsdis
import rosepan


def data_root():
    if rosepan.GAME_DATA is None:
        raise SystemExit('no game data: set ROSEPAN_DATA or tus.json settings.data_dir')
    return str(rosepan.GAME_DATA)


def pack_files(b):
    """The files of an RTPK pack: [(offset, bytes)]."""
    count = struct.unpack_from('<H', b, 0xe)[0]
    flags = struct.unpack_from('<H', b, 0xa)[0]
    p = 0x20
    sizes = None
    if flags & 1:
        sizes = struct.unpack_from('<%dI' % count, b, p)
        p += count * 4
    offs = struct.unpack_from('<%dI' % count, b, p)
    out = []
    for i in range(count):
        end = offs[i] + sizes[i] if sizes else (offs[i + 1] if i + 1 < count and offs[i + 1] else len(b))
        out.append((offs[i], b[offs[i]:end]))
    return out


def find_image(pack):
    for _, f in pack_files(pack):
        if f[:4] == b'ELFR':
            return f
    raise SystemExit('no ELFR image in the pack')


class Image:
    """One script image: code at +0x30 linked at 0x8160, data at [+0x14] linked at [+0x24]."""

    def __init__(self, b):
        self.b = b
        self.data_off = struct.unpack_from('<I', b, 0x14)[0]
        self.data_link = struct.unpack_from('<I', b, 0x24)[0]
        self.code = b[0x30:self.data_off]
        self.words = struct.unpack('<%dI' % (len(self.code) // 4), self.code[:len(self.code) // 4 * 4])

    def string_at(self, addr):
        off = addr - self.data_link + self.data_off
        if off < self.data_off or off >= len(self.b):
            return None
        end = self.b.find(b'\0', off)
        if end <= off:
            return None
        s = self.b[off:end]
        if all(0x20 <= c < 0x7f or c in (9, 10) for c in s) and len(s) >= 2:
            return s.decode('ascii')
        return None

    def functions(self):
        """Entry offsets: a stack frame opening with `addiu sp,sp,-n`."""
        out = []
        for i, w in enumerate(self.words):
            if (w & 0xffff8000) == 0x27bd8000:
                # The epilogue's addiu sp is positive, so this is a prologue.
                out.append(i)
        return out

    def lines(self, lo, hi):
        hi_reg = {}
        for i in range(lo, hi):
            w = self.words[i]
            pc = 0x8160 + i * 4
            text, _ = mipsdis.decode(pc, w, {}, 0)
            note = ''
            op = w >> 26
            rs = (w >> 21) & 31
            rt = (w >> 16) & 31
            if op == 0x0f:
                hi_reg[rt] = (w & 0xffff) << 16
            elif op == 0x09 and rs == rt and rs in hi_reg:
                imm = w & 0xffff
                addr = hi_reg[rs] + (imm - 0x10000 if imm & 0x8000 else imm)
                s = self.string_at(addr)
                if s is not None:
                    note = ' ; "%s"' % s.replace('\n', '\\n')
                elif hi_reg[rs] == 0x410000 or hi_reg[rs] == 0x400000:
                    note = ' ; &0x%x' % addr
            elif op == 0x03:
                target = (w & 0x03ffffff) << 2
                t = target & 0xffff
                if t < 0x8160:
                    note = ' ; native %d' % ((t - 0x144) // 0x40)
            yield pc, text, note


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('stage')
    ap.add_argument('room', nargs='?')
    ap.add_argument('--grep')
    ap.add_argument('--func')
    ap.add_argument('--strings', action='store_true')
    ap.add_argument('--data')
    args = ap.parse_args(argv)
    root = args.data or data_root()
    if args.stage.upper() == 'G000':
        path = os.path.join(root, 'DATA', 'SEQ', 'G000.GRB')
    elif args.room:
        path = os.path.join(root, 'DATA', 'SEQ', args.stage, args.room, args.room + '.ARA')
    else:
        path = os.path.join(root, 'DATA', 'SEQ', args.stage, 'W000.WRD')
    with open(path, 'rb') as f:
        image = Image(find_image(f.read()))

    if args.strings:
        for m in re.finditer(rb'[\x20-\x7e\n]{4,}', image.b[image.data_off:]):
            print('%#x %s' % (m.start() + image.data_link, m.group().decode().replace('\n', '\\n')))
        return 0

    starts = image.functions() + [len(image.words)]
    for a, b in zip(starts, starts[1:]):
        lines = list(image.lines(a, b))
        pc0 = 0x8160 + a * 4
        if args.func and int(args.func, 16) != pc0:
            continue
        text = '\n'.join('%06x: %s%s' % l for l in lines)
        if args.grep and args.grep not in text:
            continue
        print('; ---- function 0x%x' % pc0)
        print(text)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
