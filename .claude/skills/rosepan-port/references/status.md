# RosePan status (generated)

Generated 2026-10-22 11:43 at commit `8e37b23` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 544 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6396**, named **1735** (27%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 544, lib-match-low 133, lib-context 28, ror-inferred 331
- context-name ledger: 359 entries; overrides of wrong library matches: 4
- placed in a source file: 1584/6396 in 76 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00110000` | 0/252 (0%) | mag/magsystem.cpp, dog/doglightpool.cpp, mag/maglifepc.cpp |
| `00120000` | 0/283 (0%) | mag/maglifebwnctrl.cpp, mag/maglifepcmode.cpp, mag/maglifemenu.cpp |
| `00130000` | 0/175 (0%) | mag/magscene.cpp, mag/magscene2.cpp, mag/magscenerender.cpp |
| `00140000` | 0/293 (0%) | mag/magloader.cpp, mag/magscenefocus.cpp, mag/magsceneif.cpp |
| `00150000` | 0/272 (0%) | - |
| `00160000` | 0/268 (0%) | emy/emy_main.cpp |
| `00170000` | 0/251 (0%) | - |
| `00180000` | 0/310 (0%) | mcd/mcdman.cpp, snd/rxmpeg.cpp, msg/msgman.cpp |

## Reconstruction (src/)

- reconstructed RoR functions: **319**; stubs: **10**

| file | reconstructed | stubs |
|---|---|---|
| `src/bed/bedmemory.cpp` | 0 | 7 |
| `src/main.cpp` | 1 | 0 |
| `src/rx/rxfile.cpp` | 168 | 0 |
| `src/rx/rxsys.cpp` | 73 | 3 |
| `src/rx/rxthread.cpp` | 53 | 0 |
| `src/utl/utlmisc.cpp` | 24 | 0 |

## Translation units (one session each: `tus.py`)

- 141 TUs: todo 138, partial 1, verify 4, done 2
- next: **bed/bedmemory.cpp** (frontier score 9)
- pinned queue: (empty)
- verify: `rx/rxfile.cpp` 168/168 -- Status verify: every function in 0x2437f8..0x249b88 is written, but nothing can show it: main 0x2045a8 is still the placeholder (black screen), and the rxthread
- verify: `rx/rxsys.cpp` 73/119 -- Status verify: nothing calls rxSysInit yet - main 0x2045a8 is still the placeholder in src/main.cpp, so the screenshot is the black placeholder frame. Next: mai
- verify: `rx/rxthread.cpp` 53/53 -- Complete; verify because main (unassigned@001fd424) is still the placeholder, so nothing calls rxThInit in the smoke run (black frame as before). A temporary ca
- verify: `utl/utlmisc.cpp` 24/24 -- Verify: nothing calls this yet (main 0x2045a8 is still the placeholder; the smoke run shows a black frame). When main is reconstructed it calls utlMemInit and i

Recent sessions:

- 2026-10-22 11:43 `utl/utlmisc.cpp` verify: utl/utlmisc.cpp pinned at 0x105650..0x105e50 (assert line 22 at 0x105650) and all 24 functions written: utlIsTccName, utlFtoi15, the name/value text parser utlP
- 2026-10-22 11:31 `rx/rxthread.cpp` verify: rx thread layer complete: 53 functions (11 hidden ones created in Ghidra) -- scheduler with dispatch alarm, rxThYield / rxThRequest handshake, rx thread slots, 
- 2026-10-22 11:15 `rx/rxfile.cpp` verify: rx file layer complete: async file slots (fd 1..63, ring buffers, markers, tags, segments), default load/size/save hooks, rxFileLoadExec, the cdrom driver's ISO
- 2026-10-22 10:43 `rx/rxfile.cpp` partial: rx middleware file layer, first slice: pinned rx/rxfile.cpp 0x2437f8-0x249b88 and rx/rxthread.cpp 0x249b88-0x24b490; reconstructed the device driver tables, the
- 2026-10-22 10:26 `rx/rxsys.cpp` verify: rx middleware system layer: pinned rx/rxsys.cpp 0x23ff58-0x2437f8 and reconstructed 60 functions - rxSysInit (hardware reset, boot device, memory hooks, rx subs
- 2026-10-22 10:05 `setup/platform_bootstrap.cpp` done: PC project created from ReiPan's platform layer: src/rosepan and src/sdk copied and renamed ReiPan to RosePan (identifiers, env variables, files, shader macros)
- 2026-10-22 09:49 `setup/ghidra_seed.cpp` done: Ghidra seeded: the ELF's 1,449 linkonce symbols applied (ror-symbol), 1,642 functions created (sections, 95 vtable slot targets, 114 library functions, 21 tail-

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `bed/bedmemory.cpp` | todo | 0/7/16 | 4.9 | 9 | 0 |
| `unassigned@001fd424` | partial | 1/0/90 | 31.2 | 4 | 0 |
| `unassigned@0020eda4` | todo | 0/0/65 | 28.1 | 1 | 0 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `00100a3c` | BedMemory::alloc | 2 | ror | stub | bed/bedmemory.cpp |
| `00100c84` | BedMemory::allocDebug | 2 | ror | stub | bed/bedmemory.cpp |
| `00100354` | BedMemory::create | 1 | ror | stub | bed/bedmemory.cpp |
| `00243560` | rxSysUpdateDevices | 1 | ror | stub | rx/rxsys.cpp |
| `00100568` | BedMemory::destroy | 1 | ror | stub | bed/bedmemory.cpp |
| `00241a00` | rxSysOpenTty | 1 | ror | stub | rx/rxsys.cpp |
| `002043dc` | FUN_002043dc | 1 | - | missing | - |
| `00242960` | FUN_00242960 | 1 | - | missing | rx/rxsys.cpp |
| `00243608` | rxSysInitFileDevice | 1 | ror | missing | rx/rxsys.cpp |
| `002046d4` | FUN_002046d4 | 1 | - | missing | - |
| `00204864` | FUN_00204864 | 1 | - | missing | - |
| `0020483c` | FUN_0020483c | 1 | - | missing | - |
| `001010a4` | BedMemory::realloc | 1 | ror | stub | bed/bedmemory.cpp |
| `00101328` | BedMemory::free | 1 | ror | stub | bed/bedmemory.cpp |
| `00101518` | BedMemory::check | 1 | ror | stub | bed/bedmemory.cpp |
| `00215a30` | FUN_00215a30 | 1 | - | missing | - |
| `002413d8` | rxSysIniFind | 1 | ror | stub | rx/rxsys.cpp |

