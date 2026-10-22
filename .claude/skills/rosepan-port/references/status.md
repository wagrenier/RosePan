# RosePan status (generated)

Generated 2026-10-22 10:26 at commit `2673262` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 544 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6357**, named **1490** (23%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 544, lib-match-low 133, lib-context 16, ror-inferred 86
- context-name ledger: 102 entries; overrides of wrong library matches: 4
- placed in a source file: 1340/6357 in 74 files

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

- reconstructed RoR functions: **61**; stubs: **22**

| file | reconstructed | stubs |
|---|---|---|
| `src/lib/unassigned_stubs.cpp` | 0 | 19 |
| `src/main.cpp` | 1 | 0 |
| `src/rx/rxsys.cpp` | 60 | 3 |

## Translation units (one session each: `tus.py`)

- 141 TUs: todo 139, partial 1, verify 1, done 2
- next: **unassigned@002437f8** (frontier score 26)
- pinned queue: (empty)
- verify: `rx/rxsys.cpp` 60/119 -- Status verify: nothing calls rxSysInit yet - main 0x2045a8 is still the placeholder in src/main.cpp, so the screenshot is the black placeholder frame. Next: mai

Recent sessions:

- 2026-10-22 10:26 `rx/rxsys.cpp` verify: rx middleware system layer: pinned rx/rxsys.cpp 0x23ff58-0x2437f8 and reconstructed 60 functions - rxSysInit (hardware reset, boot device, memory hooks, rx subs
- 2026-10-22 10:05 `setup/platform_bootstrap.cpp` done: PC project created from ReiPan's platform layer: src/rosepan and src/sdk copied and renamed ReiPan to RosePan (identifiers, env variables, files, shader macros)
- 2026-10-22 09:49 `setup/ghidra_seed.cpp` done: Ghidra seeded: the ELF's 1,449 linkonce symbols applied (ror-symbol), 1,642 functions created (sections, 95 vtable slot targets, 114 library functions, 21 tail-

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `unassigned@002437f8` | todo | 0/19/182 | 30.7 | 26 | 0 |
| `unassigned@001fd424` | partial | 1/0/90 | 31.2 | 4 | 0 |
| `unassigned@001056f8` | todo | 0/0/23 | 1.8 | 4 | 0 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `00248870` | rxFileGetCurrent | 4 | ror | stub | - |
| `0024ae60` | rxThRequest | 2 | ror | stub | - |
| `0024a508` | rxThYield | 2 | ror | stub | - |
| `00245ed8` | rxFileLoad | 2 | ror | stub | - |
| `002461f0` | rxFileRebootIop | 1 | ror | stub | - |
| `00246278` | rxFileInit | 1 | ror | stub | - |
| `00248fd8` | rxFileStartThread | 1 | ror | stub | - |
| `00248660` | rxFileInitSlots | 1 | ror | stub | - |
| `0024a258` | rxThAddCallback | 1 | ror | stub | - |
| `00248a80` | rxFileSetSearchDir | 1 | ror | stub | - |
| `002043dc` | FUN_002043dc | 1 | - | missing | - |
| `00105be4` | FUN_00105be4 | 1 | - | missing | - |
| `00242960` | FUN_00242960 | 1 | - | missing | rx/rxsys.cpp |
| `00243608` | rxSysInitFileDevice | 1 | ror | missing | rx/rxsys.cpp |
| `002046d4` | FUN_002046d4 | 1 | - | missing | - |
| `00204864` | FUN_00204864 | 1 | - | missing | - |
| `0020483c` | FUN_0020483c | 1 | - | missing | - |
| `00248a60` | rxFileGetDir | 1 | ror | stub | - |
| `002458b0` | rxFileCallDevice | 1 | ror | stub | - |
| `00248890` | rxFileSetDevice | 1 | ror | stub | - |
| `002413d8` | rxSysIniFind | 1 | ror | stub | rx/rxsys.cpp |
| `00248998` | rxFileSetDir | 1 | ror | stub | - |
| `00248bb0` | rxFileGetSearchDir | 1 | ror | stub | - |
| `00246030` | rxFileLoadModule | 1 | ror | stub | - |
| `00249d48` | rxThIsDispatchDisabled | 1 | ror | missing | - |
| `0024a8c8` | rxThDisableDispatch | 1 | ror | stub | - |
| `0024a958` | rxThEnableDispatch | 1 | ror | stub | - |
| `00241a00` | rxSysOpenTty | 1 | ror | stub | rx/rxsys.cpp |
| `00243560` | rxSysUpdateDevices | 1 | ror | stub | rx/rxsys.cpp |
| `0024b0b0` | rxThInitOnce | 1 | ror | stub | - |

