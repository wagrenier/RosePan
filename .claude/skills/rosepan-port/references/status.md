# RosePan status (generated)

Generated 2026-10-22 14:04 at commit `8be131e` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 543 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6408**, named **2045** (32%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 543, lib-match-low 133, lib-context 51, ror-inferred 626
- context-name ledger: 677 entries; overrides of wrong library matches: 5
- placed in a source file: 1895/6408 in 79 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00150000` | 0/272 (0%) | - |
| `00160000` | 0/268 (0%) | emy/emy_main.cpp |
| `001a0000` | 0/211 (0%) | mnu/mnusvld.cpp, mnu/mnuselect.cpp, mnu/mnudlg.cpp |
| `001d0000` | 0/181 (0%) | game/gamstat.cpp, game/gamtrg.cpp |
| `001f0000` | 0/221 (0%) | task/naga_chunk.cpp, task/tsk_bg_load.cpp, task/tsk_scene.cpp |
| `00180000` | 2/310 (1%) | mcd/mcdman.cpp, snd/rxmpeg.cpp, msg/msgman.cpp |
| `00120000` | 2/283 (1%) | mag/maglifebwnctrl.cpp, mag/maglifepcmode.cpp, mag/maglifemenu.cpp |
| `00170000` | 2/251 (1%) | - |

## Reconstruction (src/)

- reconstructed RoR functions: **497**; stubs: **135**

| file | reconstructed | stubs |
|---|---|---|
| `src/bed/bedmemory.cpp` | 16 | 0 |
| `src/bed/bedpad.cpp` | 0 | 5 |
| `src/bed/bedpfh.cpp` | 0 | 3 |
| `src/dog/dogactor.cpp` | 0 | 11 |
| `src/dog/dogentity.cpp` | 0 | 2 |
| `src/dog/dogmodel.cpp` | 0 | 4 |
| `src/dog/dogtexinfo.cpp` | 0 | 7 |
| `src/game/gamman.cpp` | 0 | 4 |
| `src/i3d/i3dctx.cpp` | 31 | 0 |
| `src/i3d/i3dvram.cpp` | 37 | 0 |
| `src/lib/unassigned_stubs.cpp` | 0 | 55 |
| `src/mag/maglife.cpp` | 0 | 1 |
| `src/mag/maglifebwn.cpp` | 0 | 1 |
| `src/mag/maglifeman.cpp` | 0 | 2 |
| `src/mag/maglifepc.cpp` | 0 | 4 |
| `src/mag/magloader.cpp` | 0 | 7 |
| `src/mag/magscene.cpp` | 5 | 6 |
| `src/mag/magscene2.cpp` | 19 | 0 |
| `src/mag/magscenecamera.cpp` | 2 | 1 |
| `src/mag/magscenelight.cpp` | 0 | 6 |
| `src/mag/magsceneoption.cpp` | 0 | 1 |
| `src/mag/magscenerender.cpp` | 2 | 1 |
| `src/mag/magscenetenkyu.cpp` | 0 | 1 |
| `src/mag/magsystem.cpp` | 55 | 0 |
| `src/main.cpp` | 11 | 0 |
| `src/rx/rxfile.cpp` | 168 | 0 |
| `src/rx/rxsys.cpp` | 74 | 5 |
| `src/rx/rxthread.cpp` | 53 | 0 |
| `src/sequence/sequence.cpp` | 0 | 1 |
| `src/snd/rxmpeg.cpp` | 0 | 1 |
| `src/system/sysnowloading.cpp` | 0 | 2 |
| `src/utl/utlfile.cpp` | 0 | 2 |
| `src/utl/utlfind.cpp` | 0 | 1 |
| `src/utl/utlgame.cpp` | 0 | 1 |
| `src/utl/utlmisc.cpp` | 24 | 0 |

## Translation units (one session each: `tus.py`)

- 149 TUs: todo 136, partial 5, verify 10, done 3
- next: **unassigned@00190a78** (frontier score 30)
- pinned queue: (empty)
- verify: `bed/bedmemory.cpp` 16/16 -- All 16 functions written; nothing in the boot allocates yet (main is still the placeholder), so the screen stays black and the status is verify. A scratch host 
- verify: `i3d/i3dctx.cpp` 31/31 -- Status verify: the screen stays black because MagScene::draw (no layers) and i3dCtxCloseFrame (0x224798, draw-group module) are stubs, so nothing reaches the pa
- verify: `i3d/i3dvram.cpp` 37/37 -- Complete; nothing on screen shows it yet (screen black: the draw path is stubbed). A host test (init, reserve x3 frames with eviction, compact, end frame, reset
- verify: `mag/magscene2.cpp` 19/19 -- Complete but unseen: MagScene::start (magscene.cpp) and the GamMan loads are stubs, so no loader runs and the screen stays black (MagScene::draw is a stub too).
- verify: `mag/magsystem.cpp` 55/55 -- All 55 functions written; nothing on screen can show them yet (black: MagScene::draw and i3d are stubs; no caller of the actor methods runs). The Dog classes ar
- verify: `main.cpp` 11/11 -- Status verify: the smoke run boots for real (iopconf.cnf, eight IOP modules and system.ini read from the disc folder through host0:), runs mainInitGame and loop
- verify: `rx/rxfile.cpp` 168/168 -- Status verify: every function in 0x2437f8..0x249b88 is written, but nothing can show it: main 0x2045a8 is still the placeholder (black screen), and the rxthread
- verify: `rx/rxsys.cpp` 74/119 -- Status verify: nothing calls rxSysInit yet - main 0x2045a8 is still the placeholder in src/main.cpp, so the screenshot is the black placeholder frame. Next: mai
- verify: `rx/rxthread.cpp` 53/53 -- Complete; verify because main (unassigned@001fd424) is still the placeholder, so nothing calls rxThInit in the smoke run (black frame as before). A temporary ca
- verify: `utl/utlmisc.cpp` 24/24 -- Verify: nothing calls this yet (main 0x2045a8 is still the placeholder; the smoke run shows a black frame). When main is reconstructed it calls utlMemInit and i

Recent sessions:

- 2026-10-22 14:04 `mag/magscene2.cpp` verify: Pinned mag/magscene2.cpp to 0x137300..0x139540 and wrote all 19 functions: loadSystemData (options.txt, the PREMIERE.RPK font and common textures), load (a map 
- 2026-10-22 13:42 `mag/magscene2.cpp` partial: MagScene set-up half: init (fog records, light no, life manager, texture table with shadow01, sky slots), initRender (two view records, controller scene links, 
- 2026-10-22 13:24 `i3d/i3dvram.cpp` verify: i3d VRAM cache: all 37 functions written (block lists, alloc first-fit from the LRU tail, LRU eviction, coalesce, compact, end frame, reset, init, term, handle 
- 2026-10-22 13:15 `i3d/i3dctx.cpp` verify: i3d draw context: pinned i3d/i3dctx.cpp 0x21eb28-0x21fb30 and wrote all 32 functions (7 hidden ones created in Ghidra: default end hook, SetEndFunc, GetRestFrom
- 2026-10-22 12:55 `mag/magsystem.cpp` verify: MagSystem complete: the 12 actor methods (setActorLightID, setActorParent, getActorNodeMatrix x2 and ByName x2, getActorMotionMove, addActorTrans, releaseActorM
- 2026-10-22 12:41 `mag/magsystem.cpp` partial: MagSystem: 40 of 52 functions written - ctor/dtor, V-blank callbacks, initGraph (pads, VIF1, GS reset, 512x448 double buffer, i3d version check, i3d context on 
- 2026-10-22 12:14 `main.cpp` verify: main.cpp (0x2043dc..0x204d38): RoR main and its 10 helpers written - argument parsing (-l language, -m stage), the rx bring-up booting from host0:, game heap an
- 2026-10-22 11:53 `bed/bedmemory.cpp` verify: BedMemory block heap: create, format, destroy, alignBlock, splitTail, findFree, findFreeFromEnd, alloc, allocDebug, getHeader, shrink, trimFront, realloc, free,

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `unassigned@00190a78` | todo | 0/5/223 | 63.5 | 30 | 5 |
| `unassigned@001bde58` | todo | 0/4/56 | 9.8 | 26 | 0 |
| `game/gamman.cpp` | todo | 0/4/288 | 44.9 | 24 | 4 |
| `unassigned@00228c08` | todo | 0/5/465 | 91.8 | 21 | 2 |
| `bed/bedpad.cpp` | todo | 0/5/5 | 0.9 | 20 | 2 |
| `dog/dogtexinfo.cpp` | todo | 0/7/16 | 5.1 | 20 | 2 |
| `unassigned@00164fc0` | todo | 0/5/433 | 108.5 | 20 | 3 |
| `unassigned@0021fb50` | todo | 0/9/118 | 31.5 | 19 | 2 |
| `bed/bedpfh.cpp` | todo | 0/3/4 | 0.9 | 19 | 0 |
| `mag/magscenelight.cpp` | todo | 0/5/8 | 2.3 | 19 | 2 |
| `unassigned@001fd424` | todo | 0/3/75 | 27.9 | 18 | 3 |
| `mag/maglifeman.cpp` | todo | 0/2/10 | 1.5 | 16 | 2 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `00101750` | BedPfh::BedPfh | 11 | ror | stub | bed/bedpfh.cpp |
| `001be524` | FileReader::read | 10 | ror | stub | - |
| `001019f0` | BedPfh::getFileIndex | 7 | ror | stub | bed/bedpfh.cpp |
| `001be734` | FileReadReq::FileReadReq | 5 | ror | missing | - |
| `001be2dc` | FileReader::~FileReader | 5 | ror | missing | - |
| `001196d4` | MagLifeMan::add | 5 | ror | stub | mag/maglifeman.cpp |
| `0013f980` | MagScene::setLightParam | 4 | ror | stub | mag/magscenelight.cpp |
| `001be2cc` | FileReader::FileReader | 4 | ror | missing | - |
| `00101ccc` | BedPad::update | 4 | ror | stub | bed/bedpad.cpp |
| `00237cc8` | i3dFlushCache | 4 | ror | stub | - |
| `0022b228` | I3rRoot__CheckIdString | 3 | lib-ctx | stub | - |
| `00101b50` | BedPad::setVibration | 3 | ror | stub | bed/bedpad.cpp |
| `00115cec` | DogModel::release | 3 | ror | stub | dog/dogmodel.cpp |
| `0021a828` | i3rCopyClipSphere | 3 | lib-ctx | stub | - |
| `00215bb4` | dbgTraceVector4 | 3 | ror | stub | - |
| `00105f0c` | utlStrCopy2 | 3 | ror | stub | utl/utlfile.cpp |
| `0013f4d0` | MagScene::setLightNo | 2 | ror | stub | mag/magscenelight.cpp |
| `0020e3d8` | seqSetLanguage | 2 | ror | stub | - |
| `001102b4` | DogEntityEx::setParent | 2 | ror | stub | dog/dogentity.cpp |
| `001163dc` | DogTexInfo::find | 2 | ror | stub | dog/dogtexinfo.cpp |
| `001174c4` | DogTexInfo::setTexFlag | 2 | ror | stub | dog/dogtexinfo.cpp |
| `00142b18` | MagCharaLoader::load | 2 | ror | stub | mag/magloader.cpp |
| `0022b3d8` | i3rGetNodeIndexByName | 2 | lib-ctx | stub | - |
| `0011756c` | DogTexInfo::setTexTable | 2 | ror | stub | dog/dogtexinfo.cpp |
| `0011bd4c` | MagLifePC::setActor | 2 | ror | stub | mag/maglifepc.cpp |
| `00205394` | SysNowLoading::term | 2 | ror | stub | system/sysnowloading.cpp |
| `001256d4` | MagLifeBwn::setActor | 2 | ror | stub | mag/maglifebwn.cpp |
| `00115790` | DogModel::loadData | 2 | ror | stub | dog/dogmodel.cpp |
| `00136d14` | MagScene::~MagScene | 1 | ror | stub | dog/dogmodel.cpp |
| `00101aec` | BedPad::~BedPad | 1 | ror | stub | bed/bedpad.cpp |
| `00109ed0` | DogActor::releaseModel | 1 | ror | stub | dog/dogactor.cpp |
| `0021a120` | i3dFogInit | 1 | ror | stub | - |
| `00119630` | MagLifeMan::init | 1 | ror | stub | mag/maglifeman.cpp |
| `001162ac` | DogTexInfo::init | 1 | ror | stub | dog/dogtexinfo.cpp |
| `00116b5c` | DogTexInfo::add | 1 | ror | stub | dog/dogtexinfo.cpp |
| `0010620c` | utlFindClear | 1 | ror | stub | utl/utlfind.cpp |
| `0011c418` | MagLifePC::termWorld | 1 | ror | stub | mag/maglifepc.cpp |
| `002243a0` | i3dGroupInit | 1 | ror | stub | - |
| `00203ea8` | SysTimer::start | 1 | ror | stub | - |
| `001e71fc` | CmnTaskMan::addTask | 1 | ror | stub | - |

