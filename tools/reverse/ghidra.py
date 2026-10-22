"""Small Ghidra operations over GhidraMCP HTTP, for sessions without the MCP tools (autopilot).

python ghidra.py asm <lo> <hi>          disassemble a byte range, read-only (works where no function covers it)
python ghidra.py func <addr>            the function containing addr: name, entry, body range
python ghidra.py xrefs <addr>           references Ghidra knows to addr
python ghidra.py refs <addr>            every way code could reach addr: Ghidra xrefs, `jal`/`j` encodings,
                                        the address as a data word (vtable slot, table entry), and
                                        `addiu ..., lo16` halves of a lui/addiu pair
python ghidra.py find <hex> [mask]      byte-pattern search (little-endian bytes, e.g. e29b070c; the mask is
                                        ignored by GhidraMCP: search exact patterns)
python ghidra.py segments               memory blocks
python ghidra.py mkfunc <addr> [name]   create a function at addr (a Ghidra write) and save the program;
                                        use when a body swallowed the next function (hazards.md)

Addresses are hex (0x optional). Always the RoR program; `program` goes in the query string.
"""
import json
import struct
import sys

import rosepan

sys.stdout.reconfigure(encoding='utf-8', errors='replace')


def addr(text):
    return int(text, 16)


def get(endpoint, **query):
    return rosepan.http('GET', endpoint, None, timeout=120, **query)


def post(endpoint, body, **query):
    return rosepan.http('POST', endpoint, None, body, timeout=120, **query)


def find(pattern, mask=''):
    out = json.loads(get('search_byte_patterns', pattern=pattern, **({'mask': mask} if mask else {})))
    return [int(e['address'], 16) for e in out if isinstance(e, dict) and 'address' in e]


def le(word):
    return struct.pack('<I', word).hex()


def disasm(lo, hi, chunk=0xc00):
    """Instructions of [lo, hi) as GhidraMCP dicts. disassemble_bytes returns at most 1000
    instructions per call, so longer ranges go in chunks."""
    ins, a = [], lo
    while a < hi:
        b = min(a + chunk, hi)
        # dry_run in the query string: in a POST body GhidraMCP ignores it and disassembles for real.
        out = json.loads(post('disassemble_bytes', {'start_address': f'0x{a:x}', 'end_address': f'0x{b - 1:x}'},
                              dry_run='true'))
        if not out.get('instructions'):
            raise SystemExit(json.dumps(out)[:500])
        ins += out['instructions']
        a = b
    return ins


def cmd_asm(lo, hi):
    for ins in disasm(addr(lo), addr(hi)):
        print(f'{ins["address"]}: {ins["mnemonic"]} {ins["operands"]}'.rstrip())


def cmd_func(a):
    print(get('get_function_by_address', address=f'0x{addr(a):x}'))


def cmd_xrefs(a):
    print(get('get_xrefs_to', address=f'0x{addr(a):x}', limit=500))


def cmd_refs(a):
    a = addr(a)
    print('ghidra xrefs:')
    print('  ' + get('get_xrefs_to', address=f'0x{a:x}', limit=500).replace('\n', '\n  '))
    target = (a >> 2) & 0x3ffffff
    for what, pat in (('jal', le(0x0c000000 | target)), ('j', le(0x08000000 | target)), ('data word', le(a))):
        hits = find(pat)
        print(f'{what} ({pat}): ' + (', '.join(f'{h:08x}' for h in hits) or 'none'))
    lo16 = a & 0xffff
    hits = [h for h in find(struct.pack('<H', lo16).hex()) if h % 4 == 0]
    print(f'addiu/lw lo16 ({lo16:04x}): {len(hits)} candidates (check the lui before each)'
          + (': ' + ', '.join(f'{h:08x}' for h in hits[:20]) if hits else ''))


def cmd_find(pattern, mask=''):
    hits = find(pattern, mask)
    print(', '.join(f'{h:08x}' for h in hits) or 'none')


def cmd_segments():
    print(get('list_segments', limit=100))


def cmd_mkfunc(a, name=''):
    ok = rosepan.create_function(addr(a), name)
    print(f'{addr(a):08x}: {"created" if ok else "FAILED"}')
    print(rosepan.save_program()[:120])


COMMANDS = {'asm': cmd_asm, 'func': cmd_func, 'xrefs': cmd_xrefs, 'refs': cmd_refs, 'find': cmd_find,
            'segments': cmd_segments, 'mkfunc': cmd_mkfunc}

if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] not in COMMANDS:
        raise SystemExit(__doc__)
    if not rosepan.ghidra_up():
        raise SystemExit(f'Ghidra (GhidraMCP) is not reachable at {rosepan.GHIDRA} with {rosepan.PROGRAM} open')
    COMMANDS[sys.argv[1]](*sys.argv[2:])
