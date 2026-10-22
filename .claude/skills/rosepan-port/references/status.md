# RosePan status (generated)

Generated 2026-10-22 09:29 at commit `(no commit yet)` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 0  **-> run `python sections.py --apply`**
- library matches planned: 696; tagged lib-match / -low in Ghidra: 0 / 0  **-> run `python libmatch.py --apply`**
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6239**, named **1404** (23%); library functions: 901
- by provenance: ror-symbol 0, lib-match 0, lib-match-low 0, lib-context 0, ror-inferred 0
- context-name ledger: 0 entries; overrides of wrong library matches: 0
- placed in a source file: 1199/6239 in 73 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00100000` | 0/299 (0%) | dog/dogactor.cpp, utl/utlgame.cpp, bed/bedmemory.cpp |
| `00110000` | 0/251 (0%) | mag/magsystem.cpp, dog/doglightpool.cpp, mag/maglifepc.cpp |
| `00120000` | 0/261 (0%) | mag/maglifebwnctrl.cpp, mag/maglifepcmode.cpp, mag/maglifemenu.cpp |
| `00130000` | 0/174 (0%) | mag/magscene.cpp, mag/magscene2.cpp, mag/magscenerender.cpp |
| `00140000` | 0/285 (0%) | mag/magloader.cpp, mag/magscenefocus.cpp, mag/magsceneif.cpp |
| `00150000` | 0/256 (0%) | - |
| `00160000` | 0/260 (0%) | emy/emy_main.cpp |
| `00170000` | 0/233 (0%) | - |

## Reconstruction (src/)

- reconstructed RoR functions: **0**; stubs: **0**

## Translation units (one session each: `tus.py`)

- 138 TUs: todo 140
- next: **setup/ghidra_seed.cpp** (pinned in the queue)
- pinned queue: setup/ghidra_seed.cpp, setup/platform_bootstrap.cpp

## Frontier (called by reconstructed code, not written yet)

- empty: nothing reconstructed calls an unwritten function yet. Start from `main` (crt0 `_start` calls it) or the queue in `references/open-items.md`.

