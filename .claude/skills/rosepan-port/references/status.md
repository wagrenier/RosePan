# RosePan status (generated)

Generated 2026-10-22 12:14 at commit `2dfd1d9` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 543 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6396**, named **1784** (28%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 543, lib-match-low 133, lib-context 34, ror-inferred 380
- context-name ledger: 414 entries; overrides of wrong library matches: 5
- placed in a source file: 1793/6396 in 77 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00120000` | 0/283 (0%) | mag/maglifebwnctrl.cpp, mag/maglifepcmode.cpp, mag/maglifemenu.cpp |
| `00140000` | 0/293 (0%) | mag/magloader.cpp, mag/magscenefocus.cpp, mag/magsceneif.cpp |
| `00150000` | 0/272 (0%) | - |
| `00160000` | 0/268 (0%) | emy/emy_main.cpp |
| `00180000` | 0/310 (0%) | mcd/mcdman.cpp, snd/rxmpeg.cpp, msg/msgman.cpp |
| `00190000` | 0/223 (0%) | spr/sprtexture.cpp |
| `001a0000` | 0/211 (0%) | mnu/mnusvld.cpp, mnu/mnuselect.cpp, mnu/mnudlg.cpp |
| `001b0000` | 0/81 (0%) | mnu/mnuSelectLanguage.cpp |

## Reconstruction (src/)

- reconstructed RoR functions: **346**; stubs: **28**

| file | reconstructed | stubs |
|---|---|---|
| `src/bed/bedmemory.cpp` | 16 | 0 |
| `src/game/gamman.cpp` | 0 | 4 |
| `src/lib/unassigned_stubs.cpp` | 0 | 8 |
| `src/mag/magscenecamera.cpp` | 0 | 1 |
| `src/mag/magsystem.cpp` | 0 | 7 |
| `src/main.cpp` | 11 | 0 |
| `src/rx/rxfile.cpp` | 168 | 0 |
| `src/rx/rxsys.cpp` | 74 | 5 |
| `src/rx/rxthread.cpp` | 53 | 0 |
| `src/sequence/sequence.cpp` | 0 | 1 |
| `src/system/sysnowloading.cpp` | 0 | 2 |
| `src/utl/utlmisc.cpp` | 24 | 0 |

## Translation units (one session each: `tus.py`)

- 143 TUs: todo 140, verify 6, done 2
- next: **mag/magsystem.cpp** (frontier score 34)
- pinned queue: (empty)
- verify: `bed/bedmemory.cpp` 16/16 -- All 16 functions written; nothing in the boot allocates yet (main is still the placeholder), so the screen stays black and the status is verify. A scratch host 
- verify: `main.cpp` 11/11 -- Status verify: the smoke run boots for real (iopconf.cnf, eight IOP modules and system.ini read from the disc folder through host0:), runs mainInitGame and loop
- verify: `rx/rxfile.cpp` 168/168 -- Status verify: every function in 0x2437f8..0x249b88 is written, but nothing can show it: main 0x2045a8 is still the placeholder (black screen), and the rxthread
- verify: `rx/rxsys.cpp` 74/119 -- Status verify: nothing calls rxSysInit yet - main 0x2045a8 is still the placeholder in src/main.cpp, so the screenshot is the black placeholder frame. Next: mai
- verify: `rx/rxthread.cpp` 53/53 -- Complete; verify because main (unassigned@001fd424) is still the placeholder, so nothing calls rxThInit in the smoke run (black frame as before). A temporary ca
- verify: `utl/utlmisc.cpp` 24/24 -- Verify: nothing calls this yet (main 0x2045a8 is still the placeholder; the smoke run shows a black frame). When main is reconstructed it calls utlMemInit and i

Recent sessions:

- 2026-10-22 12:14 `main.cpp` verify: main.cpp (0x2043dc..0x204d38): RoR main and its 10 helpers written - argument parsing (-l language, -m stage), the rx bring-up booting from host0:, game heap an
- 2026-10-22 11:53 `bed/bedmemory.cpp` verify: BedMemory block heap: create, format, destroy, alignBlock, splitTail, findFree, findFreeFromEnd, alloc, allocDebug, getHeader, shrink, trimFront, realloc, free,
- 2026-10-22 11:43 `utl/utlmisc.cpp` verify: utl/utlmisc.cpp pinned at 0x105650..0x105e50 (assert line 22 at 0x105650) and all 24 functions written: utlIsTccName, utlFtoi15, the name/value text parser utlP
- 2026-10-22 11:31 `rx/rxthread.cpp` verify: rx thread layer complete: 53 functions (11 hidden ones created in Ghidra) -- scheduler with dispatch alarm, rxThYield / rxThRequest handshake, rx thread slots, 
- 2026-10-22 11:15 `rx/rxfile.cpp` verify: rx file layer complete: async file slots (fd 1..63, ring buffers, markers, tags, segments), default load/size/save hooks, rxFileLoadExec, the cdrom driver's ISO
- 2026-10-22 10:43 `rx/rxfile.cpp` partial: rx middleware file layer, first slice: pinned rx/rxfile.cpp 0x2437f8-0x249b88 and rx/rxthread.cpp 0x249b88-0x24b490; reconstructed the device driver tables, the
- 2026-10-22 10:26 `rx/rxsys.cpp` verify: rx middleware system layer: pinned rx/rxsys.cpp 0x23ff58-0x2437f8 and reconstructed 60 functions - rxSysInit (hardware reset, boot device, memory hooks, rx subs
- 2026-10-22 10:05 `setup/platform_bootstrap.cpp` done: PC project created from ReiPan's platform layer: src/rosepan and src/sdk copied and renamed ReiPan to RosePan (identifiers, env variables, files, shader macros)

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `mag/magsystem.cpp` | todo | 0/7/49 | 7.4 | 34 | 5 |
| `game/gamman.cpp` | todo | 0/4/288 | 44.9 | 24 | 4 |
| `unassigned@001fd424` | todo | 0/3/75 | 27.9 | 18 | 3 |
| `system/sysnowloading.cpp` | todo | 0/2/4 | 0.8 | 13 | 2 |
| `unassigned@001e46b8` | todo | 0/2/253 | 78.3 | 12 | 2 |
| `sequence/sequence.cpp` | todo | 0/1/13 | 6.2 | 6 | 1 |
| `mag/magscenecamera.cpp` | todo | 0/1/13 | 7.1 | 6 | 1 |
| `unassigned@002053f4` | todo | 0/1/43 | 36.7 | 6 | 1 |
| `unassigned@00164fc0` | todo | 0/1/433 | 108.5 | 6 | 1 |
| `unassigned@0020ff34` | todo | 0/1/53 | 23.7 | 1 | 0 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `001178b8` | MagSystem::setLanguage | 2 | ror | stub | mag/magsystem.cpp |
| `00205394` | SysNowLoading::term | 2 | ror | stub | system/sysnowloading.cpp |
| `00117c58` | MagSystem::resetDBuff | 2 | ror | stub | mag/magsystem.cpp |
| `00117ed0` | MagSystem::term | 1 | ror | stub | mag/magsystem.cpp |
| `00203ea8` | SysTimer::start | 1 | ror | stub | - |
| `001e71fc` | CmnTaskMan::addTask | 1 | ror | stub | - |
| `00204014` | SysTimer::update | 1 | ror | stub | - |
| `0020f98c` | Sequence::exec | 1 | ror | stub | sequence/sequence.cpp |
| `001c526c` | GamMan::update | 1 | ror | stub | game/gamman.cpp |
| `001e7a20` | CmnTaskMan::exec | 1 | ror | stub | - |
| `001182dc` | MagSystem::update | 1 | ror | stub | mag/magsystem.cpp |
| `001c53d8` | GamMan::postUpdate | 1 | ror | stub | game/gamman.cpp |
| `0017f164` | SndMan::init | 1 | ror | stub | - |
| `002155a0` | debugNop | 1 | ror | stub | - |
| `002431c0` | rxSysInitExceptionScreen | 1 | ror | stub | rx/rxsys.cpp |
| `00243560` | rxSysUpdateDevices | 1 | ror | stub | rx/rxsys.cpp |
| `00241a00` | rxSysOpenTty | 1 | ror | stub | rx/rxsys.cpp |
| `00242960` | rxSysSetDebugHandlers | 1 | ror | stub | rx/rxsys.cpp |
| `00204148` | sysInitManagers | 1 | ror | stub | - |
| `00117dc8` | MagSystem::init | 1 | ror | stub | mag/magsystem.cpp |
| `002052ac` | SysNowLoading::init | 1 | ror | stub | system/sysnowloading.cpp |
| `00117f80` | MagSystem::setSceneFlags | 1 | ror | stub | mag/magsystem.cpp |
| `0013c928` | MagScene::initCamera | 1 | ror | stub | mag/magscenecamera.cpp |
| `00117e18` | MagSystem::start | 1 | ror | stub | mag/magsystem.cpp |
| `0020e3d8` | seqSetLanguage | 1 | ror | stub | - |
| `001c1b48` | GamMan::loadSystemData | 1 | ror | stub | game/gamman.cpp |
| `001c204c` | GamMan::loadStage | 1 | ror | stub | game/gamman.cpp |
| `002413d8` | rxSysIniFind | 1 | ror | stub | rx/rxsys.cpp |

