"""Decompile functions over GhidraMCP: python dec.py <addr|name> [...]"""
import sys

import rosepan

sys.stdout.reconfigure(encoding='utf-8', errors='replace')

for target in sys.argv[1:]:
    print(f'===== {rosepan.PROGRAM} {target}')
    print(rosepan.decompile(target))
    print()
