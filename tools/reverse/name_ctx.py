"""Apply names worked out by reading the code, and keep the ledger.

python name_ctx.py <batch.json> [--no-refresh]
batch: [{"addr": "0010eebc", "name": "DogActor::DogActor", "tag": "ror-inferred" | "lib-context",
         "why": "evidence in one line"}, ...]

ror-inferred  a game function named from its code and context (strings, asserts, vtable slot, callers).
              Methods are 'Class::method' (Ghidra keeps the '::' literally); an overload that would
              collide gets its parameters without spaces: 'DogModel::setHide(bool)'.
lib-context   a library / runtime function named by reading it (Sony / libc / libgcc names).

The ELF's own symbols (tag ror-symbol) are never renamed. Replacing a lib-match name records the address
in data/overrides.json (libmatch.py --apply never restores it). Every name lands in
data/context_names.json with its evidence. Batch order does not matter: an entry whose name is still
held by another function the same batch renames is retried after the rest. Afterwards the program is
saved and refresh.py runs (fetch, features, export, status), so stubgen / callcheck / stage see the new
names; --no-refresh skips both. Never pipe this into `head`: a closed pipe kills the refresh.
"""
import json
import subprocess
import sys
from pathlib import Path

import rosepan

sys.stdout.reconfigure(encoding='utf-8', errors='replace')
args = [x for x in sys.argv[1:] if not x.startswith('--')]
batch = rosepan.load_json(args[0], [])
ledger = rosepan.load_json(rosepan.DATA / 'context_names.json', {})
overrides = rosepan.load_json(rosepan.DATA / 'overrides.json', {})
rosepan.ensure_tags()

names = rosepan.function_names()
taken = {}
for a, n in names.items():
    taken.setdefault(n, []).append(a)

renamed_in_batch = {e['addr'].lower().replace('0x', '').zfill(8) for e in batch}
done, failed, deferred = 0, [], []
queue = list(batch)
while queue:
    e = queue.pop(0)
    a = e['addr'].lower().replace('0x', '').zfill(8)
    ai = int(a, 16)
    if e['tag'] not in rosepan.CONTEXT_TAGS:
        failed.append((a, e['name'], 'unknown tag ' + e['tag']))
        continue
    if ' ' in e['name']:
        failed.append((a, e['name'], 'Ghidra rejects whitespace in names'))
        continue
    if a not in names:
        failed.append((a, e['name'], 'not a function start (python ghidra.py mkfunc first)'))
        continue
    others = [x for x in taken.get(e['name'], []) if x != a]
    if others and e not in deferred and all(x in renamed_in_batch for x in others):
        deferred.append(e)              # held by a function this batch renames: retry last
        queue.append(e)
        continue
    if others:
        failed.append((a, e['name'], 'name already used at ' + ','.join(others)))
        continue
    tags = json.loads(rosepan.http('GET', 'get_function_tags', None, function='0x' + a)).get('tags', [])
    old = [t['name'] for t in tags if t['name'] in rosepan.ALL_TAGS]
    if 'ror-symbol' in old:
        failed.append((a, e['name'], f'{names[a]} is the ELF\'s own symbol; not renamed'))
        continue
    if any(t in rosepan.MATCH_TAGS for t in old):
        overrides[a] = {'was': names[a], 'now': e['name']}
    if names[a] != e['name']:
        err = rosepan.rename(ai, e['name'])
        if err:
            failed.append((a, e['name'], err))
            continue
        if a in taken.get(names[a], []):          # the old name is free now (shifted runs)
            taken[names[a]].remove(a)
    rosepan.set_tag(ai, e['tag'])
    ledger[a] = {'name': e['name'], 'tag': e['tag'], 'why': e.get('why', ''),
                 'was': ledger.get(a, {}).get('was', names[a])}
    taken.setdefault(e['name'], []).append(a)
    done += 1

rosepan.save_json(rosepan.DATA / 'context_names.json', ledger)
rosepan.save_json(rosepan.DATA / 'overrides.json', overrides)
print(f'named {done}/{len(batch)}; ledger {len(ledger)}; overrides {len(overrides)}')
for x in failed:
    print('  FAILED', *x)
sys.stdout.flush()

if '--no-refresh' not in sys.argv and done:
    print(rosepan.save_program()[:120], flush=True)
    subprocess.run([sys.executable, str(Path(__file__).resolve().parent / 'refresh.py')],
                   cwd=Path(__file__).resolve().parent)
