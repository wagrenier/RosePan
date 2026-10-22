# RosePan status (generated)

Generated 2026-10-22 09:49 at commit `d7dc277` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

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

- reconstructed RoR functions: **0**; stubs: **0**

## Translation units (one session each: `tus.py`)

- 139 TUs: todo 140, done 1
- next: **setup/platform_bootstrap.cpp** (pinned in the queue)
- pinned queue: setup/platform_bootstrap.cpp

Recent sessions:

- 2026-10-22 09:49 `setup/ghidra_seed.cpp` done: Ghidra seeded: the ELF's 1,449 linkonce symbols applied (ror-symbol), 1,642 functions created (sections, 95 vtable slot targets, 114 library functions, 21 tail-

## Frontier (called by reconstructed code, not written yet)

- empty: nothing reconstructed calls an unwritten function yet. Start from `main` (crt0 `_start` calls it) or the queue in `references/open-items.md`.

