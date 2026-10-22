"""RoR's own symbols: the named .gnu.linkonce sections of the ELF (offline plan, Ghidra apply).

python sections.py                 plan: work/sections.json and what Ghidra still lacks (read-only)
python sections.py --list [FILTER] every named function: address, size, Ghidra name, demangled signature
python sections.py --layout        write the satellite ranges (the linkonce text spans) into data/layout.json
python sections.py --apply         create the missing functions -- every named section, every vtable slot
                                   target, every .ctors / .dtors entry, every tail-jump target that
                                   follows a `jr ra` -- name the sections' functions, tag them
                                   ror-symbol, save, refresh.py

The retail ELF kept its section header table: every inline method and template instance GCC emitted
into a .gnu.linkonce.t.<symbol> section is named by its mangled symbol (1,449 functions), and every
virtual table into .gnu.linkonce.d._vt$<class> (284). Nothing else in .text is named. Ghidra's
analysis finds few of these (nothing calls inline virtuals directly), so they are created here.
Ghidra names are gcc2.ghidra_name(): 'DogEntity::reset', 'DogEntity::~DogEntity'; overloads that
would collide carry their parameters ('MagLifeWindow::processPad(BedPad const&,...)' without spaces).
"""
import json
import sys
from collections import defaultdict

import gcc2
import rosepan
from elf32 import Elf

sys.stdout.reconfigure(encoding='utf-8', errors='replace')
OUT = rosepan.WORK / 'sections.json'


def load(elf=None):
    """{'functions': [...], 'vtables': [...], 'ctors': [...], 'dtors': [...]} from the ELF."""
    elf = elf or Elf(str(rosepan.ELF))
    funcs, vts = [], []
    for s in elf.sections:
        if s['name'].startswith('.gnu.linkonce.t.'):
            sym = s['name'][len('.gnu.linkonce.t.'):]
            funcs.append({'addr': s['addr'], 'size': s['size'], 'symbol': sym, 'd': gcc2.demangle(sym)})
        elif s['name'].startswith('.gnu.linkonce.d.'):
            sym = s['name'][len('.gnu.linkonce.d.'):]
            d = gcc2.demangle(sym)
            if d and d.kind == 'vtable':
                entries = []
                for i in range(s['size'] // 8):
                    w0, pfn = elf.u32(s['addr'] + 8 * i), elf.u32(s['addr'] + 8 * i + 4)
                    delta = w0 & 0xffff
                    entries.append({'delta': delta - 0x10000 if delta & 0x8000 else delta,
                                    'index': (w0 >> 16) & 0xffff, 'pfn': pfn})
                vts.append({'addr': s['addr'], 'size': s['size'], 'symbol': sym, 'class': d.qual,
                            'for': d.params, 'entries': entries})
    base = defaultdict(list)
    for f in funcs:
        if f['d']:
            base[gcc2.ghidra_name(f['d'])].append(f)
    for f in funcs:
        f['name'] = gcc2.ghidra_name(f['d'], overloaded=len(base[gcc2.ghidra_name(f['d'])]) > 1) \
            if f['d'] else f['symbol']
        f['display'] = f['d'].display if f['d'] else f['symbol']
        f['class'] = f['d'].qual if f['d'] else ''
        f['kind'] = f['d'].kind if f['d'] else '?'
        del f['d']

    def table(name):
        s = elf.section(name)
        if not s:
            return []
        words = [elf.u32(s['addr'] + 4 * i) for i in range(s['size'] // 4)]
        return [w for w in words if w not in (0, 0xffffffff)]
    return {'functions': funcs, 'vtables': vts, 'ctors': table('.ctors'), 'dtors': table('.dtors')}


def spans(funcs):
    """Contiguous runs of the linkonce text sections."""
    out = []
    for f in sorted(funcs, key=lambda f: f['addr']):
        if out and f['addr'] <= out[-1][1] + 0x20:
            out[-1][1] = max(out[-1][1], f['addr'] + f['size'])
        else:
            out.append([f['addr'], f['addr'] + f['size']])
    return out


def ghidra_functions():
    """{addr: name} of the functions Ghidra itself has (fetch.py's dump; features.py adds the sections)."""
    try:
        d = json.load(open(rosepan.WORK / 'hash_ror.json', encoding='utf-8'))['functions']
    except FileNotFoundError:
        return {}
    return {int(f['address'], 16): f['name'] for f in d if ':' not in f['address']}


def tail_targets(elf=None):
    """Targets of tail jumps (`j` out of a body) where Ghidra has no function but the address follows
    `jr ra; <delay>` (and nop padding): code reached only by tail calls, which Ghidra folds into the
    caller's body (a decompile then shows the callee's code inline: hazards.md). Targets that do not
    follow a `jr ra` are labels inside another function and are left alone."""
    elf = elf or Elf(str(rosepan.ELF))
    have = set(ghidra_functions())
    try:
        feat = json.load(open(rosepan.WORK / 'feat_ror.json', encoding='utf-8'))['functions']
    except FileNotFoundError:
        return []
    out = set()
    for f in feat:
        for t in f.get('tails', []):
            if t in have or not (elf.section_of(t) or '').startswith(('.text', '.gnu.linkonce.t')):
                continue
            a = t - 4
            while elf.u32(a) == 0:
                a -= 4
            if elf.u32(a - 4) == 0x03e00008:
                out.add(t)
    return sorted(out)


def plan(S):
    names = ghidra_functions()
    text = {f['addr'] for f in S['functions']}
    slots = sorted({e['pfn'] for v in S['vtables'] for e in v['entries'][1:] if e['pfn']} - text)
    missing = [f for f in S['functions'] if f['addr'] not in names]
    rename = [f for f in S['functions'] if f['addr'] in names and names[f['addr']] != f['name']]
    slot_missing = [a for a in slots if a not in names]
    init_missing = [a for a in S['ctors'] + S['dtors'] if a not in names]
    tails = tail_targets()
    print(f'{len(S["functions"])} named functions ({len(missing)} without a Ghidra function, {len(rename)} '
          f'to rename), {len(S["vtables"])} vtables with {len(slots)} slot targets in .text '
          f'({len(slot_missing)} without a function), .ctors {len(S["ctors"])} / .dtors {len(S["dtors"])} '
          f'({len(init_missing)} without a function), {len(tails)} tail-jump targets without a function')
    print('linkonce text spans: ' + ', '.join(f'{lo:08x}..{hi:08x}' for lo, hi in spans(S['functions'])))
    return missing, rename, slot_missing, init_missing


def apply(S):
    if not rosepan.ghidra_up():
        raise SystemExit(f'Ghidra (GhidraMCP at {rosepan.GHIDRA}) is not reachable with {rosepan.PROGRAM} open')
    rosepan.ensure_tags()
    current = {int(a, 16): n for a, n in rosepan.function_names().items()}
    made, named, failed = 0, 0, []
    for f in S['functions']:
        a = f['addr']
        if a not in current:
            if not rosepan.create_function(a, f['name']):
                failed.append((a, f['name'], 'could not create a function'))
                continue
            made += 1
        elif current[a] != f['name']:
            err = rosepan.rename(a, f['name'])
            if err:
                failed.append((a, f['name'], err))
                continue
        rosepan.set_tag(a, 'ror-symbol')
        named += 1
    text = {f['addr'] for f in S['functions']}
    extra = sorted(({e['pfn'] for v in S['vtables'] for e in v['entries'][1:] if e['pfn']} - text)
                   | set(S['ctors']) | set(S['dtors']) | set(tail_targets()))
    for a in extra:
        if a not in current:
            if rosepan.create_function(a):
                made += 1
            else:
                failed.append((a, '', 'could not create a function (vtable slot / ctors entry)'))
    print(f'created {made} functions, named {named}; failed {len(failed)}')
    for x in failed[:60]:
        print(f'  FAILED {x[0]:08x} {x[1]} {x[2]}')
    print(rosepan.save_program()[:120])
    import subprocess
    subprocess.run([sys.executable, str(rosepan.TOOLS / 'refresh.py')], cwd=rosepan.TOOLS)


if __name__ == '__main__':
    args = sys.argv[1:]
    S = load()
    rosepan.save_json(OUT, S)
    if '--list' in args:
        flt = args[args.index('--list') + 1] if len(args) > args.index('--list') + 1 else ''
        for f in S['functions']:
            line = f'{f["addr"]:08x} {f["size"]:#6x} {f["name"]:50} {f["display"]}'
            if flt in line:
                print(line)
        sys.exit(0)
    plan(S)
    if '--layout' in args:
        lay = rosepan.load_json(rosepan.LAYOUT_JSON, {})
        lay['satellite_ranges'] = [[f'{lo:08x}', f'{hi:08x}', 'named .gnu.linkonce.t sections (inline methods): '
                                    "they ride with their class's TU"]
                                   for lo, hi in spans(S['functions']) if not rosepan.in_lib(lo)]
        rosepan.save_json(rosepan.LAYOUT_JSON, lay)
        print(f'{rosepan.LAYOUT_JSON.relative_to(rosepan.REPO)}: {len(lay["satellite_ranges"])} satellite ranges')
    if '--apply' in args:
        apply(S)
