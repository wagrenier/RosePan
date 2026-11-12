"""The i3r item accessors of i3d/i3ritem.cpp: classify, name and generate them.

python i3racc.py list                  every accessor in 0x22c5a0..0x22e840: address, item type, op, slot
python i3racc.py mkfunc                create the Ghidra functions the accessors lack (one save at the end)
python i3racc.py batch <out.json>      a name_ctx.py batch naming them all (tag ror-inferred)
python i3racc.py cpp                   print the C++ (header prototypes, then the definitions per type)
python i3racc.py apply                 insert them into src/i3d/i3ritem.cpp / .h (once)

Between the per-type data fixups (i3rRelocDataXX / i3rUnrelocDataXX) the ROM has 288 small,
unreferenced accessors of the item of that type (the fixup pair right before them). Each takes the
item and reads one of its fixed child slots (children[k], 16 bytes each), in ten shapes: the slot
itself, its data != 0, its child count, child n, its children's begin / end, the child lookup by name
(i3rItemFindChild), child n's name, the child by name, has a child of that name. The shapes are matched
instruction for instruction with their offsets (k * 16 + field), so a function that is anything else
fails the classification instead of being guessed. Names and code follow i3d/i3rtype.cpp, whose
accessors (0x22a6c0..0x22b1b0) are the same shapes: i3r<Op><type>[_<k>]. Offline (the ELF); mkfunc
needs Ghidra.
"""
import json
import re
import sys

import rosepan
from elf32 import Elf
from mipsdis import decode

LO, HI = 0x22c5a0, 0x22e840
FIND = 0x0022c1f8   # i3rItemFindChild
FALSE_STARTS = {0x22d2b0}   # i3rUnrelocData46's loop tail (a b back after its jr ra)
NL = chr(10)

# op -> (name stem, C return type, C parameters, C body with {k}) -- i3d/i3rtype.cpp's style
S = 'item->getChildren()[{k}]'
OPS = {
    'slot':     ('GetSlot',        'I3rItem *',    '', 'return &' + S + ';'),
    'hasdata':  ('HasData',        'int',          '', 'return ' + S + '.m_data != 0;'),
    'num':      ('GetNumChild',    'int',          '', 'return (int)' + S + '.getNumChild();'),
    'child':    ('GetChild',       'I3rItem *',    ', int n', 'return ' + S + '.getChildren() + n;'),
    'begin':    ('GetChildBegin',  'I3rItem *',    '', 'return ' + S + '.getChildren();'),
    'end':      ('GetChildEnd',    'I3rItem *',    '',
                 'return ' + S + '.getChildren() + ' + S + '.getNumChild();'),
    'find':     ('FindChild',      'int',          ', const char *name',
                 'return i3rItemFindChild(&' + S + ', name);'),
    'name':     ('GetChildName',   'const char *', ', int n', 'return ' + S + '.getChildren()[n].getName();'),
    'byname':   ('GetChildByName', 'I3rItem *',    ', const char *name',
                 'const I3rItem *list = &' + S + ';' + NL + NL
                 + '    return list->getChildren() + i3rItemFindChild(list, name);'),
    'haschild': ('HasChild',       'int',          ', const char *name',
                 'return i3rItemFindChild(&' + S + ', name) != -1;'),
}


def shapes(k):
    """The exact instruction text of every op for slot k (as mipsdis prints it, trailing nops dropped)."""
    s, c, n = k * 16, k * 16 + 8, k * 16 + 4
    add = [f'addiu a0,a0,0x{s:x}'] if k else []
    return {
        'slot': ([f'lw v0,0x8(a0)', 'jr ra', f'addiu v0,v0,0x{s:x}'] if k else ['jr ra', 'lw v0,0x8(a0)']),
        'hasdata': ['lw t7,0x8(a0)', f'lw v0,0x{s:x}(t7)', 'jr ra', 'sltu v0,zero,v0'],
        'num': ['lw t7,0x8(a0)', f'lw v0,0x{n:x}(t7)', 'sll v0,v0,8', 'jr ra', 'srl v0,v0,8'],
        'child': ['lw t7,0x8(a0)', 'sll a1,a1,4', f'lw v0,0x{c:x}(t7)', 'jr ra', 'addu v0,v0,a1'],
        'begin': ['lw t7,0x8(a0)', 'jr ra', f'lw v0,0x{c:x}(t7)'],
        'end': (['lw t7,0x8(a0)', f'addiu t7,t7,0x{s:x}', 'lw t6,0x4(t7)', 'lw v0,0x8(t7)', 'sll t6,t6,8',
                 'srl t6,t6,4', 'jr ra', 'addu v0,v0,t6'] if k else
                ['lw t6,0x8(a0)', 'lw t7,0x4(t6)', 'lw v0,0x8(t6)', 'sll t7,t7,8', 'srl t7,t7,4', 'jr ra',
                 'addu v0,v0,t7']),
        'find': ['addiu sp,sp,-0x10', 'sd ra,0x0(sp)', 'lw a0,0x8(a0)', 'ld ra,0x0(sp)', *add,
                 f'j 0x{FIND:08x}', 'addiu sp,sp,0x10'],
        'name': ['lw t6,0x8(a0)', 'sll a1,a1,4', f'lw t7,0x{c:x}(t6)', 'addu a1,a1,t7', 'jr ra',
                 'lw v0,0xc(a1)'],
        'byname': ['addiu sp,sp,-0x10', 'sd ra,0x8(sp)', 'sd s0,0x0(sp)', 'lw s0,0x8(a0)',
                   *([f'addiu s0,s0,0x{s:x}'] if k else []), f'jal 0x{FIND:08x}', 'move a0,s0',
                   'lw t7,0x8(s0)', 'sll v0,v0,4', 'ld ra,0x8(sp)', 'addu t7,t7,v0', 'ld s0,0x0(sp)',
                   'move v0,t7', 'jr ra', 'addiu sp,sp,0x10'],
        'haschild': (['addiu sp,sp,-0x10', 'sd ra,0x0(sp)', 'lw a0,0x8(a0)', f'jal 0x{FIND:08x}',
                      f'addiu a0,a0,0x{s:x}'] if k else
                     ['addiu sp,sp,-0x10', 'sd ra,0x0(sp)', f'jal 0x{FIND:08x}', 'lw a0,0x8(a0)'])
                    + ['nor v0,zero,v0', 'ld ra,0x0(sp)', 'sltu v0,zero,v0', 'jr ra', 'addiu sp,sp,0x10'],
    }


SHAPES = {}
for _k in range(32):
    for _op, _ins in shapes(_k).items():
        SHAPES[tuple(_ins)] = (_op, _k)


def scan():
    """[(addr, type, op, slot)] in ROM order; raises on code that matches no shape."""
    elf = Elf(rosepan.ELF)
    names = rosepan.cached_names()
    ins = []
    for pc in range(LO, HI, 4):
        text = decode(pc, elf.u32(pc), names, elf.gp)[0]
        ins.append((pc, text.rstrip()))
    starts = {int(a, 16) for a in names if LO <= int(a, 16) < HI}
    for i, (pc, t) in enumerate(ins):
        if t.startswith('jr ra') or t.startswith('j 0x'):
            j = i + 2
            while j < len(ins) and ins[j][1] == 'nop':
                j += 1
            if j < len(ins):
                starts.add(ins[j][0])
    starts -= FALSE_STARTS
    st = sorted(starts)
    out, typ, bad = [], None, []
    for a, b in zip(st, st[1:] + [HI]):
        name = names.get(f'{a:08x}', '')
        m = re.match(r'i3r(?:Unreloc|Reloc)Data([0-9A-F]{2})$', name)
        if m:
            typ = int(m.group(1), 16)
            continue
        body = [t for pc, t in ins if a <= pc < b]
        while body and body[-1] == 'nop':
            body.pop()
        hit = SHAPES.get(tuple(body))
        if hit is None:
            bad.append((a, body))
            continue
        out.append((a, typ, hit[0], hit[1]))
    if bad:
        raise SystemExit('unclassified:' + NL + NL.join(f'{a:08x}: {" ; ".join(b)}' for a, b in bad))
    seen = {}
    for a, t, op, k in out:
        if (t, op, k) in seen:
            raise SystemExit(f'{a:08x} repeats {seen[(t, op, k)]:08x}: type {t:02X} {op} {k}')
        seen[(t, op, k)] = a
    return out


def fname(t, op, k):
    """i3rtype.h's naming: i3r<Op><type>[_<k>], no suffix for slot 0."""
    return f'i3r{OPS[op][0]}{t:02X}' + (f'_{k}' if k else '')


def blocks(acc):
    """[(type, [(addr, op, k)])] in ROM order."""
    out = []
    for a, t, op, k in acc:
        if not out or out[-1][0] != t:
            out.append((t, []))
        out[-1][1].append((a, op, k))
    return out


def proto(a, t, op, k):
    _, ret, params, _ = OPS[op]
    lead = f'{ret[:-1].rstrip():<11}*' if ret.endswith('*') else f'{ret:<12}'   # i3rtype.h's columns
    return f'{lead}{fname(t, op, k)}(const I3rItem *item{params});'.ljust(78) + f'// ROR 0x{a:08x}'


def defn(a, t, op, k):
    _, ret, params, body = OPS[op]
    sp = '' if ret.endswith('*') else ' '
    lines = [f'/* ROR 0x{a:08x} */']
    if op == 'byname':
        lines.append('/* -1 (absent) gives the item before the list, as in the ROM. */')
    return lines + [f'{ret}{sp}{fname(t, op, k)}(const I3rItem *item{params})', '{',
                    '    ' + body.replace('{k}', str(k)), '}', '']


def type_banner(t):
    return ['// ' + chr(0x2500) * 70, f'// Type 0x{t:02x}: its accessors, after its fixup pair (above).', '']


def cmd_list():
    acc = scan()
    for a, t, op, k in acc:
        print(f'{a:08x}  type {t:02X}  {op:<8} slot {k}  {fname(t, op, k)}')
    print(f'{len(acc)} accessors over {len(blocks(acc))} types')


def cmd_mkfunc():
    if not rosepan.ghidra_up():
        raise SystemExit(f'Ghidra: {rosepan.ghidra_down_reason()}')
    made = failed = 0
    for a, t, op, k in scan():
        if rosepan.has_function(a):
            continue
        if rosepan.create_function(a, fname(t, op, k)):
            made += 1
        else:
            failed += 1
            print(f'{a:08x}: FAILED')
    print(f'created {made}, failed {failed}')
    print(rosepan.save_program()[:120])


def cmd_batch(path):
    batch = [{'addr': f'{a:08x}', 'name': fname(t, op, k), 'tag': 'ror-inferred',
              'why': f'i3racc.py: unreferenced accessor after type 0x{t:02X} fixups, shape {op} of child slot {k}'}
             for a, t, op, k in scan()]
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(batch, f, indent=1)
    print(f'{len(batch)} names -> {path}')


def cmd_cpp():
    acc = scan()
    print('/* ---- header ---- */')
    for t, items in blocks(acc):
        print(f'/* Type 0x{t:02x}. */')
        for a, op, k in items:
            print(proto(a, t, op, k))
    print('/* ---- definitions ---- */')
    for t, items in blocks(acc):
        print(NL.join(type_banner(t)))
        for a, op, k in items:
            print(NL.join(defn(a, t, op, k)))


def cmd_apply():
    """Insert the definitions into src/i3d/i3ritem.cpp after each type's fixup pair (before the next
    function's ROR marker) and the prototypes into i3ritem.h; refuses when they are already there."""
    src = rosepan.SRC / 'i3d' / 'i3ritem.cpp'
    hdr = rosepan.SRC / 'i3d' / 'i3ritem.h'
    acc = scan()
    text = src.read_text(encoding='utf-8')
    a0, t0, op0, k0 = acc[0]
    if fname(t0, op0, k0) + '(' in text:
        raise SystemExit('i3ritem.cpp already has the accessors')
    lines = text.split(NL)
    markers = {}
    for i, l in enumerate(lines):
        m = re.match(r'/\* ROR 0x([0-9a-f]{8}) \*/', l)
        if m:
            markers[int(m.group(1), 16)] = i
    tail = next(i for i, l in enumerate(lines) if l.startswith('/* data 0x2db360'))
    inserts = []
    for t, items in blocks(acc):
        after = [a for a in markers if items[-1][0] < a < HI]
        out = type_banner(t)
        for a, op, k in items:
            out += defn(a, t, op, k)
        inserts.append((markers[min(after)] if after else tail, out))
    for at, out in sorted(inserts, key=lambda x: -x[0]):
        lines[at:at] = out
    src.write_text(NL.join(lines), encoding='utf-8', newline=NL)
    decl = []
    for t, items in blocks(acc):
        decl += ['', f'/* Type 0x{t:02x}. */'] + [proto(a, t, op, k) for a, op, k in items]
    h = hdr.read_text(encoding='utf-8')
    end = NL + '#endif /* _I3D_I3RITEM_H */'
    hdr.write_text(h.replace(end, NL.join(decl) + NL + end), encoding='utf-8', newline=NL)
    print(f'{len(acc)} accessors in {len(blocks(acc))} types -> {src.name}, {hdr.name}')


if __name__ == '__main__':
    argv = sys.argv[1:]
    cmds = {'list': cmd_list, 'mkfunc': cmd_mkfunc, 'batch': cmd_batch, 'cpp': cmd_cpp, 'apply': cmd_apply}
    if not argv or argv[0] not in cmds:
        raise SystemExit(__doc__)
    cmds[argv[0]](*argv[1:])
