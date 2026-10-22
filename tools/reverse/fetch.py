"""Dump what the offline tools need from Ghidra into work/.

python fetch.py

Writes work/hash_ror.json (function starts, names, opcode hashes) and work/ror_tags.json (the provenance
tag of every tagged function). Read-only.
"""
import rosepan

raw = rosepan.http('GET', 'get_bulk_function_hashes', None, filter='all', limit=50000)
rosepan.WORK.mkdir(parents=True, exist_ok=True)
(rosepan.WORK / 'hash_ror.json').write_text(raw, encoding='utf-8')
print(f'{rosepan.PROGRAM}: {raw.count(chr(34) + "hash" + chr(34))} functions')

tags = {}
for tag in rosepan.ALL_TAGS:
    for addr in rosepan.tagged(tag):
        tags[addr] = tag
rosepan.save_json(rosepan.WORK / 'ror_tags.json', tags)
print(f'{rosepan.PROGRAM} tags: {len(tags)}')
