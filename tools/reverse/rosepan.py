"""Shared paths, the address map and the GhidraMCP HTTP client for the reversing tools. Not a CLI.

Every path can be overridden with an environment variable. The game ELF sits at the repo root (never
committed); ReiPan and MioPan are sibling checkouts used only as *library* references (libmatch.py):
Rule of Rose shares no game code with Fatal Frame, but it links the same Sony SDK 3.0 libraries as FF3
and the same ee-gcc 2.9x runtime as FF2.
"""
import json
import os
import re
import urllib.parse
import urllib.request
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
REPO = TOOLS.parents[1]
WORK = TOOLS / 'work'   # regenerated: Ghidra dumps, features, stages, shots, logs (gitignored)
DATA = TOOLS / 'data'   # results worth keeping: name ledger, TU state, layout

ELF = Path(os.environ.get('ROSEPAN_ELF', REPO / 'SLUS_214.48'))
FUNCTIONS_TXT = REPO / 'ror_functions.txt'
# Program name as Ghidra reports it (project path /Rule of Rose - US).
PROGRAM = os.environ.get('ROSEPAN_PROGRAM', 'SLUS_214.48')
GHIDRA = os.environ.get('GHIDRA_MCP_URL', 'http://127.0.0.1:8089').rstrip('/') + '/'

# Library references (read-only, offline): an ELF plus a functions.txt-style list with sizes.
REIPAN = Path(os.environ.get('ROSEPAN_REIPAN', REPO.parent / 'ReiPan'))
MIOPAN = Path(os.environ.get('ROSEPAN_MIOPAN') or next(
    (p for p in (REPO.parent / 'MioPan', REPO.parent / 'MioPan-Mikompilation') if p.exists()), REPO.parent / 'MioPan'))
REFERENCES = (
    # (label, ELF, function list, note)
    ('ff3', REIPAN / 'SLES_538.25', REIPAN / 'ff3_functions.txt', 'Sony SDK 3.0 libraries (same versions as RoR)'),
    ('ff2', MIOPAN / 'SLES_523.84', MIOPAN / 'functions.txt', 'ee-gcc 2.96 runtime, newlib, SDK 2.8 (debug symbols)'),
)

# The game's disc files (unpacked), for data tools (tm2png.py --scan, rawfile-style readers) and the
# smoke run's data root: $ROSEPAN_DATA, else settings.data_dir in data/tus.json (the user's copy of the
# extracted disc; the game boots from host0: on these loose files, open-items.md Decisions).
def _game_data():
    if os.environ.get('ROSEPAN_DATA'):
        return Path(os.environ['ROSEPAN_DATA'])
    try:
        d = json.loads((TOOLS / 'data' / 'tus.json').read_text(encoding='utf-8'))['settings'].get('data_dir')
    except (OSError, ValueError, KeyError):
        d = None
    return Path(d) if d and Path(d).is_dir() else None


GAME_DATA = _game_data()
SRC = REPO / 'src'
PLATFORM_DIRS = ('rosepan', 'sdk')        # the PC platform layer: not RoR game code
SKILL_DIR = REPO / '.claude' / 'skills' / 'rosepan-port'

# Host toolchains: MinGW GCC builds (build.py); MSVC only syntax-checks game code (layoutcheck.py).
MINGW_BIN = os.environ.get('ROSEPAN_MINGW_BIN', r'C:\Users\willi\AppData\Local\Programs\CLion 2\bin\mingw\bin')
NINJA = os.environ.get('ROSEPAN_NINJA', r'C:\Users\willi\AppData\Local\Programs\Python\Python314\Scripts\ninja.exe')
VCVARS = os.environ.get('ROSEPAN_VCVARS', '')    # vcvars64.bat; empty = find it with vswhere
EXE_NAME = 'RosePan'

# Provenance tags (Ghidra function tags) on every name the tools write.
SYMBOL_TAGS = ('ror-symbol',)                      # the ELF's own symbols (named linkonce sections)
MATCH_TAGS = ('lib-match', 'lib-match-low')        # library code matched to a symbolised reference ELF
CONTEXT_TAGS = ('lib-context', 'ror-inferred')     # named by reading: a library / runtime function, game code
ALL_TAGS = SYMBOL_TAGS + MATCH_TAGS + CONTEXT_TAGS
GENERIC_NAME = re.compile(r'^(FUN_|thunk_FUN_)')

# Source annotation above every reconstructed function, and the stub macros (src/common/rosepan_stub.h).
ANNOT = 'ROR'
ANNOT_RE = re.compile(r'ROR 0x([0-9a-fA-F]{8})')
STUB_RE = re.compile(r'ROSEPAN_(?:STUB|BRIDGE)\("([^"]+)",\s*0x([0-9a-fA-F]+)\)')

LAYOUT_JSON = DATA / 'layout.json'
TUS_JSON = DATA / 'tus.json'


def load_json(path, default):
    try:
        with open(path, encoding='utf-8') as f:
            return json.load(f)
    except FileNotFoundError:
        return default


def save_json(path, obj, indent=1):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(obj, f, indent=indent, sort_keys=isinstance(obj, dict))
        f.write('\n')


# ---- the address map ------------------------------------------------------------------------------
# data/layout.json: {"lib_ranges": [[lo, hi, what], ...], "satellite_ranges": [[lo, hi, what], ...]}
# (hex strings). Library ranges are replaced on PC (src/sdk, host libc); never a TU, never frontier.
# libmatch.py --ranges proposes them. Satellites: the named .gnu.linkonce.t sections (inline methods,
# template instances) -- they have no TU of their own and ride with their class's TU.

def _ranges(key):
    return [(int(lo, 16), int(hi, 16)) for lo, hi, *_ in load_json(LAYOUT_JSON, {}).get(key, [])]


LIB_RANGES = _ranges('lib_ranges')
SATELLITE_RANGES = _ranges('satellite_ranges')


def in_lib(addr):
    return any(lo <= addr < hi for lo, hi in LIB_RANGES)


def is_satellite(addr):
    return any(lo <= addr < hi for lo, hi in SATELLITE_RANGES)


def tu_key(inferred_file):
    """TU key (path under src/, 'dog/dogmodel.cpp') for an inferred file, or None for library objects
    and unknown files. RoR's file names come from its own __FILE__ strings, already relative."""
    if not inferred_file or inferred_file.startswith('('):
        return None
    if inferred_file.startswith('lib/'):
        return None
    return inferred_file


def tu_ranges():
    """[(lo, hi, tu_key)] from data/tus.json: address ranges sessions have pinned to a TU.
    They override export.py's inference."""
    out = []
    for key, tu in load_json(TUS_JSON, {}).get('tus', {}).items():
        for lo, hi in tu.get('ranges', []):
            out.append((int(lo, 16), int(hi, 16), key))
    return sorted(out)


def src_path_for(inferred_file):
    """Where RosePan keeps an inferred source file under src/ (None for unknown runs)."""
    if not inferred_file or inferred_file.startswith('('):
        return None
    return SRC / inferred_file


# ---- Ghidra ---------------------------------------------------------------------------------------

def http(method, endpoint, program=None, body=None, timeout=300, **query):
    """Call GhidraMCP. `program` always goes in the query string: in a POST body it is silently
    ignored and the write lands on whichever program is active."""
    program = program or PROGRAM
    url = GHIDRA + endpoint + '?' + urllib.parse.urlencode({'program': program, **query})
    data = json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(url, data=data, method=method, headers={'Content-Type': 'application/json'})
    return urllib.request.urlopen(req, timeout=timeout).read().decode('utf-8', 'replace')


def ghidra_up():
    """True when the GhidraMCP server answers and has the RoR program open."""
    try:
        urllib.request.urlopen(GHIDRA + 'check_connection', timeout=3).read()
    except OSError:
        return False
    try:
        return PROGRAM in http('GET', 'list_open_programs', timeout=10)
    except OSError:
        return False


def function_names(program=None):
    """{'0010baf8': 'Name', ...} for every function in the program."""
    out = {}
    for line in http('GET', 'list_functions', program).splitlines():
        m = re.match(r'(.*) at ([0-9a-f]{8})\s*$', line)
        if m:
            out[m.group(2)] = m.group(1)
    return out


def tagged(tag, program=None):
    """Addresses ('0010baf8' form) of every function carrying `tag`."""
    d = json.loads(http('GET', 'search_functions_by_tag', program, tag=tag, limit=20000))
    return {f['address'].lower().replace('0x', '').zfill(8) for f in d.get('functions', [])}


def save_program(tries=6):
    """Save the program, retrying while a previous write still holds the transaction lock."""
    import time
    out = ''
    for _ in range(tries):
        out = http('POST', 'save_program', None, None)
        if 'Unable to lock' not in out:
            return out
        time.sleep(3)
    return out


# Ghidra writes. Never probe them with dry_run (GhidraMCP ignores it in a POST body, and create_function
# ignored it even in the query string). Every writer saves with save_program() when its batch is done.

def has_function(addr):
    """True when a function STARTS at addr (get_function_by_address answers for any address inside a body)."""
    out = http('GET', 'get_function_by_address', None, address=f'0x{addr:x}', timeout=60)
    m = re.search(r'Entry:\s*([0-9a-fA-F]+)', out)
    return bool(m) and int(m.group(1), 16) == addr


def create_function(addr, name='', tries=4):
    """Create a function at addr (disassembling first); retries while a save holds the lock. True if a
    function starts there afterwards."""
    import time
    for _ in range(tries):
        out = http('POST', 'create_function', None, {'address': f'0x{addr:x}', 'disassemble_first': True,
                                                     **({'name': name} if name else {})})
        if 'Unable to lock' not in out:
            break
        time.sleep(3)
    return has_function(addr)


def rename(addr, name):
    """Rename the function at addr ('strict_mode: off': GhidraMCP's naming-style policy rejects real
    names). Returns '' on success, else the server's answer."""
    out = http('POST', 'rename_function_by_address', None,
               {'function_address': f'0x{addr:x}', 'new_name': name, 'strict_mode': 'off'})
    return '' if '"success"' in out else out[:200]


def set_tag(addr, tag, comment=None):
    """Give the function at addr provenance `tag`, removing any other provenance tag."""
    old = json.loads(http('GET', 'get_function_tags', None, function=f'0x{addr:x}')).get('tags', [])
    drop = [t['name'] for t in old if t['name'] in ALL_TAGS and t['name'] != tag]
    if drop:
        http('POST', 'remove_function_tag', None, {'function': f'0x{addr:x}', 'tags': ','.join(drop)})
    if not any(t['name'] == tag for t in old):
        http('POST', 'add_function_tag', None, {'function': f'0x{addr:x}', 'tags': tag})
    return [t['name'] for t in old if t['name'] in ALL_TAGS]


TAG_COMMENTS = {
    'ror-symbol': "RoR's own symbol: the ELF names this function's .gnu.linkonce.t section (sections.py).",
    'lib-match': 'Library/runtime code matched word for word (relocations masked) to a symbolised '
                 'reference ELF (libmatch.py).',
    'lib-match-low': 'Library match on a short body or a disputed name: a hint, verify before relying on it.',
    'lib-context': 'Library/runtime function named by reading it against the SDK / a reference.',
    'ror-inferred': 'Name inferred from RoR code and context (strings, asserts, vtable, callers).',
}


def ensure_tags():
    for tag, comment in TAG_COMMENTS.items():
        http('POST', 'create_function_tag', None, {'name': tag, 'comment': comment})


def decompile(addr, program=None):
    raw = http('GET', 'decompile_function', program, address=f'0x{addr:08x}' if isinstance(addr, int) else addr,
               timeout=120)
    try:
        raw = json.loads(raw).get('result', raw)
    except ValueError:
        pass
    return raw.replace('\r\n', '\n').strip()


# ---- cached state (works offline) ----------------------------------------------------------------

def cached_names():
    """{'0010baf8': 'Name'} from the last fetch/features run (work/feat_ror.json), for offline use."""
    try:
        feats = json.load(open(WORK / 'feat_ror.json', encoding='utf-8'))['functions']
    except FileNotFoundError:
        return {}
    return {f'{f["addr"]:08x}': f['name'] for f in feats}


def functions():
    """{addr int: {'name', 'size', 'file', 'prov'}} from ror_functions.txt (export.py)."""
    out, cur = {}, None
    if not FUNCTIONS_TXT.exists():
        return out
    for line in FUNCTIONS_TXT.read_text(encoding='utf-8').splitlines():
        if line.startswith('// FILE -- '):
            cur = line[len('// FILE -- '):].strip()
            cur = None if cur.startswith('(') else cur
        elif (m := re.match(r'/\* ([0-9a-f]{8}) ([0-9a-f]{8}) \*/ (.+?)\(\)\{\}(?:\s+// (\S+))?\s*$', line)):
            out[int(m.group(1), 16)] = {'name': m.group(3), 'size': int(m.group(2), 16),
                                        'file': cur, 'prov': m.group(4) or ''}
    return out


def game_sources():
    for path in SRC.rglob('*'):
        if path.suffix in ('.c', '.cpp', '.cc') and path.relative_to(SRC).parts[0] not in PLATFORM_DIRS:
            yield path


def reconstructed():
    """{addr int: (path, line)} for every game function in src/ annotated `ROR 0x<addr>`."""
    out = {}
    if not SRC.exists():
        return out
    for path in game_sources():
        for i, line in enumerate(path.read_text(encoding='utf-8', errors='replace').splitlines(), 1):
            for m in ANNOT_RE.finditer(line):
                out.setdefault(int(m.group(1), 16), (path, i))
    return out


def stubs():
    """{addr int: (path, name)} for every ROSEPAN_STUB (and ROSEPAN_BRIDGE: temporary stand-ins that work
    but are not RoR's code, still to reconstruct) in game code."""
    out = {}
    if not SRC.exists():
        return out
    for path in game_sources():
        for m in STUB_RE.finditer(path.read_text(encoding='utf-8', errors='replace')):
            out[int(m.group(2), 16)] = (path, m.group(1))
    return out


def display(name):
    """A name as people read it: GCC 2 mangled names demangled, everything else unchanged."""
    import gcc2
    d = gcc2.demangle(name)
    return d.display if d else name
