# RosePan status (generated)

Generated 2026-10-22 20:05 at commit `790de0d` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 543 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6498**, named **2798** (43%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 543, lib-match-low 133, lib-context 115, ror-inferred 1315
- context-name ledger: 1430 entries; overrides of wrong library matches: 5
- placed in a source file: 2751/6498 in 89 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00150000` | 0/272 (0%) | - |
| `00160000` | 0/268 (0%) | emy/emy_main.cpp |
| `001a0000` | 0/211 (0%) | mnu/mnusvld.cpp, mnu/mnuselect.cpp, mnu/mnudlg.cpp |
| `001f0000` | 1/221 (0%) | task/naga_chunk.cpp, task/tsk_bg_load.cpp, task/tsk_scene.cpp |
| `001d0000` | 2/181 (1%) | game/gamman.cpp, game/gamstat.cpp, game/gamtrg.cpp |
| `001e0000` | 5/169 (3%) | game/gamsnd.cpp |
| `00120000` | 10/283 (4%) | mag/maglifebwnctrl.cpp, mag/maglifepcmode.cpp, mag/maglifemenu.cpp |
| `001c0000` | 7/188 (4%) | game/gamman.cpp |

## Reconstruction (src/)

- reconstructed RoR functions: **1104**; stubs: **218**

| file | reconstructed | stubs |
|---|---|---|
| `src/bed/bedmemory.cpp` | 16 | 0 |
| `src/bed/bedpad.cpp` | 0 | 5 |
| `src/bed/bedpfh.cpp` | 4 | 0 |
| `src/bed/bedsubmode.cpp` | 0 | 2 |
| `src/dog/dogactor.cpp` | 0 | 19 |
| `src/dog/dogentity.cpp` | 0 | 3 |
| `src/dog/doglightpool.cpp` | 3 | 0 |
| `src/dog/dogmodel.cpp` | 0 | 8 |
| `src/dog/dogtexinfo.cpp` | 16 | 0 |
| `src/game/gamfile.cpp` | 18 | 0 |
| `src/game/gamman.cpp` | 0 | 5 |
| `src/game/gamsnd.cpp` | 0 | 1 |
| `src/i3d/i3dctx.cpp` | 31 | 0 |
| `src/i3d/i3denv.cpp` | 12 | 0 |
| `src/i3d/i3dlayer.cpp` | 80 | 0 |
| `src/i3d/i3dmath.cpp` | 95 | 0 |
| `src/i3d/i3dpacket.cpp` | 54 | 0 |
| `src/i3d/i3dtex.cpp` | 33 | 0 |
| `src/i3d/i3dtim2.cpp` | 7 | 0 |
| `src/i3d/i3dview.cpp` | 19 | 0 |
| `src/i3d/i3dvram.cpp` | 37 | 0 |
| `src/lib/unassigned_stubs.cpp` | 0 | 112 |
| `src/mag/maglife.cpp` | 0 | 1 |
| `src/mag/maglifebwn.cpp` | 0 | 2 |
| `src/mag/maglifebwnctrl.cpp` | 0 | 1 |
| `src/mag/maglifefadekey.cpp` | 0 | 3 |
| `src/mag/maglifeman.cpp` | 12 | 0 |
| `src/mag/maglifemenu.cpp` | 0 | 5 |
| `src/mag/maglifepc.cpp` | 0 | 7 |
| `src/mag/maglifewindow.cpp` | 0 | 1 |
| `src/mag/magloader.cpp` | 0 | 7 |
| `src/mag/magscene.cpp` | 21 | 10 |
| `src/mag/magscene2.cpp` | 19 | 0 |
| `src/mag/magscenecamera.cpp` | 2 | 2 |
| `src/mag/magscenedebug.cpp` | 0 | 1 |
| `src/mag/magscenelight.cpp` | 0 | 6 |
| `src/mag/magsceneoption.cpp` | 0 | 1 |
| `src/mag/magscenerender.cpp` | 20 | 0 |
| `src/mag/magscenetenkyu.cpp` | 0 | 1 |
| `src/mag/magsystem.cpp` | 55 | 0 |
| `src/main.cpp` | 11 | 0 |
| `src/rx/rxfile.cpp` | 168 | 0 |
| `src/rx/rxsys.cpp` | 74 | 6 |
| `src/rx/rxthread.cpp` | 53 | 0 |
| `src/sequence/sequence.cpp` | 0 | 1 |
| `src/snd/rxmpeg.cpp` | 0 | 1 |
| `src/snd/sndman.cpp` | 52 | 0 |
| `src/spr/s2d.cpp` | 86 | 0 |
| `src/spr/sprman.cpp` | 53 | 0 |
| `src/spr/sprtexture.cpp` | 21 | 0 |
| `src/system/sysnowloading.cpp` | 8 | 0 |
| `src/utl/utlfile.cpp` | 0 | 2 |
| `src/utl/utlfind.cpp` | 0 | 1 |
| `src/utl/utlgame.cpp` | 0 | 2 |
| `src/utl/utlmisc.cpp` | 24 | 0 |
| `src/utl/utltm2.cpp` | 0 | 2 |

## Translation units (one session each: `tus.py`)

- 156 TUs: todo 131, partial 5, verify 28, done 2
- next: **mag/magscene.cpp** (pinned in the queue)
- pinned queue: mag/magscene.cpp, game/gamfile.cpp
- partial: `dog/doglightpool.cpp` 3/29 -- Left: the pool setup, 26 functions 0x1103dc..0x1123b0 (load / alloc / set-mask edits, mostly called from mag/magscenelight.cpp; spot-light builders 0x111584 / 0
- partial: `mag/magscene.cpp` 21/81 -- Screen still black: update runs every frame in sub mode 0 (the PC is never bound), but MagLifeMan add / update / draw are stubs, so the S2d layers and the fader
- partial: `rx/rxsnd.cpp` 0/185 -- Nothing written yet; all rxSnd entry points are still stubs in lib/unassigned_stubs.cpp. Start from architecture.md row 0x237ce8 (transport and command families
- verify: `bed/bedmemory.cpp` 16/16 -- All 16 functions written; nothing in the boot allocates yet (main is still the placeholder), so the screen stays black and the status is verify. A scratch host 
- verify: `bed/bedpfh.cpp` 4/4 -- Complete; pinned 0x101738..0x101aec (0x101718 clears 7 words, not BedPfh). PC guard: a buffer without the RTPK id leaves the view empty (the ROM hangs in the as
- verify: `dog/dogtexinfo.cpp` 16/16 -- Pinned 0x1161e4..0x11764c, all 16 functions written. Smoke run: MagScene::init's init / add run without asserts; screen still black (SprMan::draw and the scene 
- verify: `game/gamfile.cpp` 18/21 -- Black screen, as before: the code is complete but no boot path shows it yet; every FileReader::read caller that runs in the boot (GamMan::loadSystemData / loadS
- verify: `i3d/i3dctx.cpp` 31/31 -- Status verify: the screen stays black because MagScene::draw (no layers) and i3dCtxCloseFrame (0x224798, draw-group module) are stubs, so nothing reaches the pa
- verify: `i3d/i3denv.cpp` 12/12 -- Pinned 0x21a120..0x21a3e8 + 0x21ac80..0x21b220 as one file (a guess: the i3r helpers 0x21a3e8..0x21ac80 sit between; split into i3dfog / i3dlight if evidence ap
- verify: `i3d/i3dlayer.cpp` 80/80 -- Every frame now builds real groups, layers and a sorted DMA chain; the smoke run is stable and still black: nothing on the i3d path reaches a host draw module (
- verify: `i3d/i3dmath.cpp` 95/95 -- Nothing on screen can show it yet (boot still black; the i3d path has no host draw consumer, and the per-frame lightstatus asserts wait on DogLightPool::createL
- verify: `i3d/i3dpacket.cpp` 54/54 -- All packet-module bridges are gone. Black screen: nothing on the host consumes i3d packets (no GS emulation); the user still has to choose the host draw hook (o
- verify: `i3d/i3dtex.cpp` 33/33 -- Screen still black: nothing creates a texture yet (DogTexInfo is stubbed) and no host draw module consumes i3d packets, hence verify. Next: i3d/i3dtim2.cpp (que
- verify: `i3d/i3dtim2.cpp` 7/7 -- Complete code; the smoke screenshot stays black because no host draw module consumes the i3d texture objects yet, so nothing on screen shows TIM2 decoding. The 
- verify: `i3d/i3dview.cpp` 19/19 -- Status verify: views are built every frame (host test: screen centre 2047.5, frustum corner on the rectangle corner, depth 4094.6..1.06, scissor 0..511 x 0..447
- verify: `i3d/i3dvram.cpp` 37/37 -- Complete; nothing on screen shows it yet (screen black: the draw path is stubbed). A host test (init, reserve x3 frames with eviction, compact, end frame, reset
- verify: `mag/maglifeman.cpp` 12/12 -- Status verify: the S2d layers now run live and render every frame (one i3d primitive reaches rosepan_i3d without a texture object), but the screenshot stays bla
- verify: `mag/magscene2.cpp` 19/19 -- Complete but unseen: MagScene::start (magscene.cpp) and the GamMan loads are stubs, so no loader runs and the screen stays black (MagScene::draw is a stub too).
- verify: `mag/magscenerender.cpp` 20/20 -- All 20 functions written; verify because nothing on screen can show it: no boot actor has the 0x60 shadow flags and all i3d layer / view / light functions are s
- verify: `mag/magsystem.cpp` 55/55 -- All 55 functions written; nothing on screen can show them yet (black: MagScene::draw and i3d are stubs; no caller of the actor methods runs). The Dog classes ar
- verify: `main.cpp` 11/11 -- Status verify: the smoke run boots for real (iopconf.cnf, eight IOP modules and system.ini read from the disc folder through host0:), runs mainInitGame and loop
- verify: `rx/rxfile.cpp` 168/168 -- Status verify: every function in 0x2437f8..0x249b88 is written, but nothing can show it: main 0x2045a8 is still the placeholder (black screen), and the rxthread
- verify: `rx/rxsys.cpp` 74/119 -- Status verify: nothing calls rxSysInit yet - main 0x2045a8 is still the placeholder in src/main.cpp, so the screenshot is the black placeholder frame. Next: mai
- verify: `rx/rxthread.cpp` 53/53 -- Complete; verify because main (unassigned@001fd424) is still the placeholder, so nothing calls rxThInit in the smoke run (black frame as before). A temporary ca
- verify: `setup/i3d_draw_module.cpp` 0/0 -- Hook recorded in open-items Decisions (The native i3d hook). The committed screenshot is the forced test (ROSEPAN_I3D_TEST=1): S2DMAN.RPK @0x800 (PSMT8, Now Loa
- verify: `snd/sndman.cpp` 52/52 -- Done as verify: SndMan::update runs every frame now, but every rxSnd call (rx/rxsnd.h, unassigned@00237ce8) is a stub, so nothing is heard and nothing on screen
- verify: `spr/s2d.cpp` 86/86 -- All 86 functions written; nothing on screen can show them yet (MagScene::draw is a stub), so verify: the first session whose screenshot shows S2d sprites marks 
- verify: `spr/sprman.cpp` 53/53 -- Verify: nothing calls SprMan::draw with m_active set yet. S2dMovie / S2dMes / S2dBg start only from MagScene::update 0x1327dc (stub), and textures never load (f
- verify: `spr/sprtexture.cpp` 21/21 -- Pinned 0x18f5b0..0x190a78. The code is complete; the smoke run reaches it (S2dMes / S2dMovie loadTexture) but every list ends in the error state because fileGet
- verify: `system/sysnowloading.cpp` 8/8 -- Code complete, nothing on the boot path shows it yet: draw 0x2051cc is called only by the pfh loader 0x1be564 (GamMan load path, unassigned@001bde58), a stub, s
- verify: `utl/utlmisc.cpp` 24/24 -- Verify: nothing calls this yet (main 0x2045a8 is still the placeholder; the smoke run shows a black frame). When main is reconstructed it calls utlMemInit and i

Recent sessions:

- 2026-10-22 20:05 `mag/maglifeman.cpp` verify: MagLifeMan complete: all 12 functions (pinned 0x1194b4..0x119b34: clearPod, unlink, insertBefore, insertAfter, init, deleteAll, add, remove, find, removeNo, upd
- 2026-10-22 19:58 `mag/magscene.cpp` partial: Pinned mag/magscene.cpp 0x131184..0x137300 and wrote 21 of its 81 functions: MagScene::update (all ten sub modes: walking, text windows, menu fade out / menu / 
- 2026-10-22 19:36 `dog/doglightpool.cpp` partial: Pinned dog/doglightpool.cpp 0x1103dc..0x112b08 (was unassigned@0011262c); reconstructed DogLightPool::createLight 0x11262c, findNearestPointLight 0x1129cc and d
- 2026-10-22 19:29 `rx/rxsnd.cpp` partial: Pinned rx/rxsnd.cpp 0x237ce8..0x23ff58 (was unassigned@00237ce8) and named 73 functions of the rx sound library (transport, queue, config, heaps, BGM/SE/PCM/ADP
- 2026-10-22 19:21 `snd/sndman.cpp` verify: SndMan, the sound manager: all 52 functions of 0x17f0d0..0x1810f4 (ctor, init/term/update, master-volume fade, 48 slots with stream and SE state machines, id ma
- 2026-10-22 18:50 `bed/bedpfh.cpp` verify: BedPfh, the RTPK pack view: constructor (tables by header flags: sizes, offsets, attribute rows, names), clear, getFileName, getFileIndex; header inlines getFla
- 2026-10-22 18:46 `game/gamfile.cpp` verify: game/gamfile.cpp pinned 0x1be2cc..0x1beb70 out of unassigned@001bde58 and written whole (21 functions): FileReader and FileReadReq, the request state machine Fi
- 2026-10-22 18:35 `i3d/i3dmath.cpp` verify: i3d vector / matrix module pinned 0x236960..0x237ce8 and complete: all 95 functions (35 hidden ones created in Ghidra, batch i3dmath_001), both copies of the VU

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `rx/rxsnd.cpp` | partial | 0/24/185 | 32.2 | 60 | 2 |
| `unassigned@00228c08` | todo | 0/6/222 | 54.9 | 27 | 1 |
| `game/gamman.cpp` | todo | 0/5/409 | 82.8 | 26 | 4 |
| `unassigned@001810f4` | todo | 0/10/28 | 2.8 | 25 | 3 |
| `unassigned@00164fc0` | todo | 0/7/395 | 104.2 | 24 | 3 |
| `bed/bedpad.cpp` | todo | 0/5/5 | 0.9 | 20 | 2 |
| `unassigned@0020ff34` | todo | 0/3/53 | 23.7 | 20 | 0 |
| `unassigned@0010f944` | todo | 0/5/12 | 2.2 | 20 | 2 |
| `mag/magscene.cpp` | partial | 21/14/81 | 24.4 | 19 | 1 |
| `unassigned@001beb70` | todo | 0/8/27 | 6.6 | 19 | 0 |
| `mag/magscenelight.cpp` | todo | 0/5/8 | 2.3 | 19 | 2 |
| `unassigned@0013e2ec` | todo | 0/10/15 | 4.4 | 19 | 1 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `002155f4` | FUN_002155f4 | 15 | - | missing | - |
| `0022b228` | I3rRoot__CheckIdString | 12 | lib-ctx | stub | - |
| `0023e240` | rxSndAdpcmGetFree | 8 | lib-ctx | stub | rx/rxsnd.cpp |
| `001be2dc` | FileReader::~FileReader | 6 | ror | missing | game/gamfile.cpp |
| `00143fd0` | MagScene::updateFog | 6 | ror | stub | - |
| `0014062c` | MagScene::rotateTenkyu | 6 | ror | stub | - |
| `00140c68` | MagScene::updateRecords853fc | 6 | ror | stub | - |
| `001be2cc` | FileReader::FileReader | 5 | ror | missing | game/gamfile.cpp |
| `001be734` | FileReadReq::FileReadReq | 5 | ror | missing | game/gamfile.cpp |
| `0021a828` | i3rCopyClipSphere | 5 | lib-ctx | stub | - |
| `0011ade4` | MagLifePC::live | 5 | ror | stub | mag/maglifepc.cpp |
| `001e4908` | GamSndMan::findSnd | 4 | ror | stub | - |
| `0013f980` | MagScene::setLightParam | 4 | ror | stub | mag/magscenelight.cpp |
| `001bedc0` | GamFont::setTarget | 4 | ror | stub | - |
| `00105f0c` | utlStrCopy2 | 4 | ror | stub | utl/utlfile.cpp |
| `0023e158` | rxSndAdpcmGetStatus | 4 | lib-ctx | stub | rx/rxsnd.cpp |
| `0023e020` | rxSndAdpcmClose | 4 | lib-ctx | stub | rx/rxsnd.cpp |
| `00101ccc` | BedPad::update | 4 | ror | stub | bed/bedpad.cpp |
| `001c0254` | GamFont::getGlyphWidth | 4 | ror | stub | - |
| `0011034c` | FUN_0011034c | 4 | - | stub | dog/dogentity.cpp |
| `0010ff74` | DogEntity::normalizeRotate | 3 | ror | stub | - |
| `00101b50` | BedPad::setVibration | 3 | ror | stub | bed/bedpad.cpp |
| `0010fa6c` | DogEntity::getMatrix | 3 | ror | stub | - |
| `001102b4` | DogEntityEx::setParent | 3 | ror | stub | dog/dogentity.cpp |
| `0013f1e4` | MagScene::clearGamFlag25 | 3 | ror | stub | - |
| `0022b3d8` | i3rGetNodeIndexByName | 3 | lib-ctx | stub | - |
| `0021e1d8` | i3dPktDrawLineStrip | 3 | lib-ctx | stub | - |
| `0023c0f8` | rxSndSeStop | 3 | lib-ctx | stub | rx/rxsnd.cpp |
| `00115cec` | DogModel::release | 3 | ror | stub | dog/dogmodel.cpp |
| `0023c228` | rxSndSeGetStatus | 3 | lib-ctx | stub | rx/rxsnd.cpp |
| `0017772c` | FUN_0017772c | 3 | - | stub | - |
| `0013e548` | MagScene::updateCamera | 3 | ror | stub | - |
| `00215bb4` | dbgTraceVector4 | 3 | ror | stub | - |
| `001d3ac8` | GamMan::switchVolume | 2 | ror | stub | game/gamman.cpp |
| `0013f4d0` | MagScene::setLightNo | 2 | ror | stub | mag/magscenelight.cpp |
| `001e71fc` | CmnTaskMan::addTask | 2 | ror | stub | - |
| `0022b428` | i3rGetNumTex | 2 | ror | stub | - |
| `0022b438` | i3rGetTexName | 2 | ror | stub | - |
| `0010559c` | utlTm2ToSepia | 2 | ror | stub | utl/utltm2.cpp |
| `0020e3d8` | seqSetLanguage | 2 | ror | stub | - |

