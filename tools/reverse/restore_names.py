"""Snapshot and restore function names.

python restore_names.py --snapshot [FILE]   save every current name (default work/snapshots/<time>.json)
python restore_names.py <snapshot.json>     rename every function whose name differs back (tags untouched)

Take a snapshot before any bulk write (sections.py / libmatch.py --apply, a big name_ctx batch).
"""
import sys
import time

import rosepan

if '--snapshot' in sys.argv:
    rest = [a for a in sys.argv[1:] if a != '--snapshot']
    path = rest[0] if rest else rosepan.WORK / 'snapshots' / f'{time.strftime("%Y%m%d_%H%M%S")}.json'
    names = rosepan.function_names()
    rosepan.save_json(path, {'program': rosepan.PROGRAM, 'names': names})
    print(f'{len(names)} names -> {path}')
    sys.exit(0)
snap = rosepan.load_json(sys.argv[1], {})
before = snap.get('names', snap)
now = rosepan.function_names()
changed = [(a, n) for a, n in before.items() if now.get(a) != n and a in now]
failed = []
for a, n in changed:
    err = rosepan.rename(int(a, 16), n)
    if err:
        failed.append((a, n, err))
print(f'restored {len(changed) - len(failed)}/{len(changed)}')
for f in failed:
    print('  FAILED', *f)
if changed:
    print(rosepan.save_program()[:120])
