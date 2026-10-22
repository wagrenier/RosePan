# RosePan status (generated)

Generated 2026-10-22 10:05 at commit `7322367` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 544 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6353**, named **1405** (22%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 544, lib-match-low 133, lib-context 9, ror-inferred 1
- context-name ledger: 10 entries; overrides of wrong library matches: 4
- placed in a source file: 1221/6353 in 73 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00100000` | 0/301 (0%) | dog/dogactor.cpp, utl/utlgame.cpp, bed/bedmemory.cpp |
| `00110000` | 0/252 (0%) | mag/magsystem.cpp, dog/doglightpool.cpp, mag/maglifepc.cpp |
| `00120000` | 0/283 (0%) | mag/maglifebwnctrl.cpp, mag/maglifepcmode.cpp, mag/maglifemenu.cpp |
| `00130000` | 0/175 (0%) | mag/magscene.cpp, mag/magscene2.cpp, mag/magscenerender.cpp |
| `00140000` | 0/293 (0%) | mag/magloader.cpp, mag/magscenefocus.cpp, mag/magsceneif.cpp |
| `00150000` | 0/272 (0%) | - |
| `00160000` | 0/268 (0%) | emy/emy_main.cpp |
| `00170000` | 0/251 (0%) | - |

## Reconstruction (src/)

- reconstructed RoR functions: **1**; stubs: **0**

| file | reconstructed | stubs |
|---|---|---|
| `src/main.cpp` | 1 | 0 |

## Translation units (one session each: `tus.py`)

- 139 TUs: todo 138, partial 1, done 2
- next: **unassigned@00216ad0** (frontier score 9)
- pinned queue: (empty)

Recent sessions:

- 2026-10-22 10:05 `setup/platform_bootstrap.cpp` done: PC project created from ReiPan's platform layer: src/rosepan and src/sdk copied and renamed ReiPan to RosePan (identifiers, env variables, files, shader macros)
- 2026-10-22 09:49 `setup/ghidra_seed.cpp` done: Ghidra seeded: the ELF's 1,449 linkonce symbols applied (ror-symbol), 1,642 functions created (sections, 95 vtable slot targets, 114 library functions, 21 tail-

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `unassigned@00216ad0` | todo | 0/0/1064 | 208.0 | 9 | 0 |
| `unassigned@001fd424` | partial | 1/0/90 | 31.2 | 4 | 0 |
| `unassigned@001056f8` | todo | 0/0/23 | 1.8 | 4 | 0 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `002043dc` | FUN_002043dc | 1 | - | missing | - |
| `00241b20` | FUN_00241b20 | 1 | - | missing | - |
| `00241d68` | FUN_00241d68 | 1 | - | missing | - |
| `00240eb8` | FUN_00240eb8 | 1 | - | missing | - |
| `00105be4` | FUN_00105be4 | 1 | - | missing | - |
| `0023ff80` | FUN_0023ff80 | 1 | - | missing | - |
| `0023ffb0` | FUN_0023ffb0 | 1 | - | missing | - |
| `0023ffe0` | FUN_0023ffe0 | 1 | - | missing | - |
| `00240010` | FUN_00240010 | 1 | - | missing | - |
| `00242960` | FUN_00242960 | 1 | - | missing | - |
| `00243608` | FUN_00243608 | 1 | - | missing | - |
| `002046d4` | FUN_002046d4 | 1 | - | missing | - |
| `00204864` | FUN_00204864 | 1 | - | missing | - |
| `0020483c` | FUN_0020483c | 1 | - | missing | - |

