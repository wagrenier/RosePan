# RosePan status (generated)

Generated 2026-10-22 13:15 at commit `99f858c` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 543 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6408**, named **1936** (30%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 543, lib-match-low 133, lib-context 48, ror-inferred 520
- context-name ledger: 568 entries; overrides of wrong library matches: 5
- placed in a source file: 1881/6408 in 79 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00120000` | 0/283 (0%) | mag/maglifebwnctrl.cpp, mag/maglifepcmode.cpp, mag/maglifemenu.cpp |
| `00140000` | 0/293 (0%) | mag/magloader.cpp, mag/magscenefocus.cpp, mag/magsceneif.cpp |
| `00150000` | 0/272 (0%) | - |
| `00160000` | 0/268 (0%) | emy/emy_main.cpp |
| `00190000` | 0/223 (0%) | spr/sprtexture.cpp |
| `001a0000` | 0/211 (0%) | mnu/mnusvld.cpp, mnu/mnuselect.cpp, mnu/mnudlg.cpp |
| `001b0000` | 0/81 (0%) | mnu/mnuSelectLanguage.cpp |
| `001d0000` | 0/181 (0%) | game/gamstat.cpp, game/gamtrg.cpp |

## Reconstruction (src/)

- reconstructed RoR functions: **432**; stubs: **91**

| file | reconstructed | stubs |
|---|---|---|
| `src/bed/bedmemory.cpp` | 16 | 0 |
| `src/bed/bedpad.cpp` | 0 | 5 |
| `src/dog/dogactor.cpp` | 0 | 4 |
| `src/dog/dogentity.cpp` | 0 | 1 |
| `src/game/gamman.cpp` | 0 | 4 |
| `src/i3d/i3dctx.cpp` | 31 | 0 |
| `src/i3d/i3dvram.cpp` | 0 | 8 |
| `src/lib/unassigned_stubs.cpp` | 0 | 32 |
| `src/mag/maglifepc.cpp` | 0 | 1 |
| `src/mag/magscene.cpp` | 0 | 4 |
| `src/mag/magscene2.cpp` | 0 | 16 |
| `src/mag/magscenecamera.cpp` | 0 | 2 |
| `src/mag/magscenelight.cpp` | 0 | 3 |
| `src/mag/magscenerender.cpp` | 0 | 1 |
| `src/mag/magsystem.cpp` | 55 | 0 |
| `src/main.cpp` | 11 | 0 |
| `src/rx/rxfile.cpp` | 168 | 0 |
| `src/rx/rxsys.cpp` | 74 | 5 |
| `src/rx/rxthread.cpp` | 53 | 0 |
| `src/sequence/sequence.cpp` | 0 | 1 |
| `src/snd/rxmpeg.cpp` | 0 | 1 |
| `src/system/sysnowloading.cpp` | 0 | 2 |
| `src/utl/utlgame.cpp` | 0 | 1 |
| `src/utl/utlmisc.cpp` | 24 | 0 |

## Translation units (one session each: `tus.py`)

- 147 TUs: todo 142, verify 8, done 2
- next: **i3d/i3dvram.cpp** (pinned in the queue)
- pinned queue: i3d/i3dvram.cpp
- verify: `bed/bedmemory.cpp` 16/16 -- All 16 functions written; nothing in the boot allocates yet (main is still the placeholder), so the screen stays black and the status is verify. A scratch host 
- verify: `i3d/i3dctx.cpp` 31/31 -- Status verify: the screen stays black because MagScene::draw (no layers) and i3dCtxCloseFrame (0x224798, draw-group module) are stubs, so nothing reaches the pa
- verify: `mag/magsystem.cpp` 55/55 -- All 55 functions written; nothing on screen can show them yet (black: MagScene::draw and i3d are stubs; no caller of the actor methods runs). The Dog classes ar
- verify: `main.cpp` 11/11 -- Status verify: the smoke run boots for real (iopconf.cnf, eight IOP modules and system.ini read from the disc folder through host0:), runs mainInitGame and loop
- verify: `rx/rxfile.cpp` 168/168 -- Status verify: every function in 0x2437f8..0x249b88 is written, but nothing can show it: main 0x2045a8 is still the placeholder (black screen), and the rxthread
- verify: `rx/rxsys.cpp` 74/119 -- Status verify: nothing calls rxSysInit yet - main 0x2045a8 is still the placeholder in src/main.cpp, so the screenshot is the black placeholder frame. Next: mai
- verify: `rx/rxthread.cpp` 53/53 -- Complete; verify because main (unassigned@001fd424) is still the placeholder, so nothing calls rxThInit in the smoke run (black frame as before). A temporary ca
- verify: `utl/utlmisc.cpp` 24/24 -- Verify: nothing calls this yet (main 0x2045a8 is still the placeholder; the smoke run shows a black frame). When main is reconstructed it calls utlMemInit and i

Recent sessions:

- 2026-10-22 13:15 `i3d/i3dctx.cpp` verify: i3d draw context: pinned i3d/i3dctx.cpp 0x21eb28-0x21fb30 and wrote all 32 functions (7 hidden ones created in Ghidra: default end hook, SetEndFunc, GetRestFrom
- 2026-10-22 12:55 `mag/magsystem.cpp` verify: MagSystem complete: the 12 actor methods (setActorLightID, setActorParent, getActorNodeMatrix x2 and ByName x2, getActorMotionMove, addActorTrans, releaseActorM
- 2026-10-22 12:41 `mag/magsystem.cpp` partial: MagSystem: 40 of 52 functions written - ctor/dtor, V-blank callbacks, initGraph (pads, VIF1, GS reset, 512x448 double buffer, i3d version check, i3d context on 
- 2026-10-22 12:14 `main.cpp` verify: main.cpp (0x2043dc..0x204d38): RoR main and its 10 helpers written - argument parsing (-l language, -m stage), the rx bring-up booting from host0:, game heap an
- 2026-10-22 11:53 `bed/bedmemory.cpp` verify: BedMemory block heap: create, format, destroy, alignBlock, splitTail, findFree, findFreeFromEnd, alloc, allocDebug, getHeader, shrink, trimFront, realloc, free,
- 2026-10-22 11:43 `utl/utlmisc.cpp` verify: utl/utlmisc.cpp pinned at 0x105650..0x105e50 (assert line 22 at 0x105650) and all 24 functions written: utlIsTccName, utlFtoi15, the name/value text parser utlP
- 2026-10-22 11:31 `rx/rxthread.cpp` verify: rx thread layer complete: 53 functions (11 hidden ones created in Ghidra) -- scheduler with dispatch alarm, rxThYield / rxThRequest handshake, rx thread slots, 
- 2026-10-22 11:15 `rx/rxfile.cpp` verify: rx file layer complete: async file slots (fd 1..63, ring buffers, markers, tags, segments), default load/size/save hooks, rxFileLoadExec, the cdrom driver's ISO

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `mag/magscene2.cpp` | todo | 0/17/31 | 15.4 | 42 | 5 |
| `i3d/i3dvram.cpp` | todo | 0/8/37 | 4.3 | 37 | 3 |
| `game/gamman.cpp` | todo | 0/4/288 | 44.9 | 24 | 4 |
| `bed/bedpad.cpp` | todo | 0/5/5 | 0.9 | 20 | 2 |
| `unassigned@0021fb50` | todo | 0/9/118 | 31.5 | 19 | 2 |
| `unassigned@001fd424` | todo | 0/3/75 | 27.9 | 18 | 3 |
| `unassigned@00228c08` | todo | 0/4/465 | 91.8 | 18 | 2 |
| `mag/magscene.cpp` | todo | 0/4/63 | 16.2 | 14 | 2 |
| `system/sysnowloading.cpp` | todo | 0/2/4 | 0.8 | 13 | 2 |
| `unassigned@00164fc0` | todo | 0/2/433 | 108.5 | 12 | 2 |
| `unassigned@001e46b8` | todo | 0/2/253 | 78.3 | 12 | 2 |
| `mag/magscenelight.cpp` | todo | 0/3/9 | 2.4 | 11 | 1 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `0013f980` | MagScene::setLightParam | 4 | ror | stub | mag/magscenelight.cpp |
| `00101ccc` | BedPad::update | 4 | ror | stub | bed/bedpad.cpp |
| `00237cc8` | i3dFlushCache | 4 | ror | stub | - |
| `00228970` | i3dVramReset | 4 | ror | stub | i3d/i3dvram.cpp |
| `00228618` | i3dVramEndFrame | 4 | ror | stub | i3d/i3dvram.cpp |
| `00228820` | i3dVramCompact | 4 | ror | stub | i3d/i3dvram.cpp |
| `00101b50` | BedPad::setVibration | 3 | ror | stub | bed/bedpad.cpp |
| `0020e3d8` | seqSetLanguage | 2 | ror | stub | - |
| `00228ae0` | i3dVramTerm | 2 | ror | stub | i3d/i3dvram.cpp |
| `002289f8` | i3dVramInit | 2 | ror | stub | i3d/i3dvram.cpp |
| `00228650` | i3dVramReserve | 2 | ror | stub | i3d/i3dvram.cpp |
| `001102b4` | DogEntityEx::setParent | 2 | ror | stub | dog/dogentity.cpp |
| `0022b3d8` | i3rGetNodeIndexByName | 2 | lib-ctx | stub | - |
| `00228940` | i3dVramSetBase | 2 | ror | stub | i3d/i3dvram.cpp |
| `00228958` | i3dVramSetSize | 2 | ror | stub | i3d/i3dvram.cpp |
| `00205394` | SysNowLoading::term | 2 | ror | stub | system/sysnowloading.cpp |
| `00136d14` | MagScene::~MagScene | 1 | ror | stub | mag/magscene2.cpp |
| `00101aec` | BedPad::~BedPad | 1 | ror | stub | bed/bedpad.cpp |
| `002243a0` | i3dGroupInit | 1 | ror | stub | - |
| `00203ea8` | SysTimer::start | 1 | ror | stub | - |
| `001e71fc` | CmnTaskMan::addTask | 1 | ror | stub | - |
| `00204014` | SysTimer::update | 1 | ror | stub | - |
| `0020f98c` | Sequence::exec | 1 | ror | stub | sequence/sequence.cpp |
| `001c526c` | GamMan::update | 1 | ror | stub | game/gamman.cpp |
| `001e7a20` | CmnTaskMan::exec | 1 | ror | stub | - |
| `001c53d8` | GamMan::postUpdate | 1 | ror | stub | game/gamman.cpp |
| `0013caf0` | MagScene::setCameraMode | 1 | ror | stub | mag/magscenecamera.cpp |
| `0013fa50` | MagScene::getLightParam | 1 | ror | stub | mag/magscenelight.cpp |
| `001316b8` | MagScene::setPC | 1 | ror | stub | mag/magscene2.cpp |
| `00103484` | utlIsVibrationOn | 1 | ror | stub | utl/utlgame.cpp |
| `00138ed0` | MagScene::loadModel | 1 | ror | stub | mag/magscene2.cpp |
| `0013d5ac` | MagScene::getCameraMode | 1 | ror | stub | mag/magscene2.cpp |
| `00134944` | MagScene::setViewParam | 1 | ror | stub | mag/magscene.cpp |
| `00101bcc` | BedPad::init | 1 | ror | stub | bed/bedpad.cpp |
| `00237c68` | i3dPrintf | 1 | lib-ctx | stub | - |
| `0017f164` | SndMan::init | 1 | ror | stub | - |
| `00135234` | MagScene::drawView | 1 | ror | stub | mag/magscene.cpp |
| `00138bc4` | MagScene::loadModelByName | 1 | ror | stub | mag/magscene2.cpp |
| `001377f0` | MagScene::load | 1 | ror | stub | mag/magscene2.cpp |
| `002155a0` | debugNop | 1 | ror | stub | - |

