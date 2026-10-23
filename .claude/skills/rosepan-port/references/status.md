# RosePan status (generated)

Generated 2026-10-22 21:58 at commit `460ef25` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 543 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6509**, named **2955** (45%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 543, lib-match-low 133, lib-context 134, ror-inferred 1453
- context-name ledger: 1587 entries; overrides of wrong library matches: 5
- placed in a source file: 2776/6509 in 89 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00150000` | 0/272 (0%) | - |
| `00160000` | 0/268 (0%) | emy/emy_main.cpp |
| `001a0000` | 0/211 (0%) | mnu/mnusvld.cpp, mnu/mnuselect.cpp, mnu/mnudlg.cpp |
| `001f0000` | 1/221 (0%) | task/naga_chunk.cpp, task/tsk_bg_load.cpp, task/tsk_scene.cpp |
| `001d0000` | 2/181 (1%) | game/gamman.cpp, game/gamstat.cpp, game/gamtrg.cpp |
| `001e0000` | 5/169 (3%) | game/gamsnd.cpp |
| `001c0000` | 9/188 (5%) | game/gamman.cpp |
| `00140000` | 16/293 (5%) | mag/magloader.cpp, mag/magsceneif.cpp, mag/magscenefocus.cpp |

## Reconstruction (src/)

- reconstructed RoR functions: **1326**; stubs: **223**

| file | reconstructed | stubs |
|---|---|---|
| `src/bed/bedmemory.cpp` | 16 | 0 |
| `src/bed/bedpad.cpp` | 0 | 5 |
| `src/bed/bedpfh.cpp` | 4 | 0 |
| `src/bed/bedsubmode.cpp` | 0 | 2 |
| `src/dog/dogactor.cpp` | 0 | 24 |
| `src/dog/dogentity.cpp` | 0 | 3 |
| `src/dog/doglightpool.cpp` | 29 | 0 |
| `src/dog/dogmodel.cpp` | 0 | 11 |
| `src/dog/dogmotionblend.cpp` | 0 | 1 |
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
| `src/lib/unassigned_stubs.cpp` | 0 | 95 |
| `src/mag/maglife.cpp` | 0 | 1 |
| `src/mag/maglifebwn.cpp` | 0 | 3 |
| `src/mag/maglifebwnctrl.cpp` | 0 | 1 |
| `src/mag/maglifefadekey.cpp` | 0 | 6 |
| `src/mag/maglifeman.cpp` | 12 | 0 |
| `src/mag/maglifemenu.cpp` | 0 | 5 |
| `src/mag/maglifemenusp.cpp` | 0 | 2 |
| `src/mag/maglifepc.cpp` | 0 | 11 |
| `src/mag/maglifewindow.cpp` | 0 | 14 |
| `src/mag/magloader.cpp` | 0 | 7 |
| `src/mag/magscene.cpp` | 89 | 0 |
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
| `src/rx/rxsnd.cpp` | 128 | 0 |
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

- 156 TUs: todo 131, partial 3, verify 30, done 2
- next: **rx/rxsnd.cpp** (continuing a partial TU)
- pinned queue: game/gamfile.cpp
- partial: `rx/rxsnd.cpp` 128/188 -- Screen black as before (sound has no picture; the boot does not reach SndMan bank, SE or stream calls yet). Left in this TU (about 60 functions): XSEQ (0x23aa08
- verify: `bed/bedmemory.cpp` 16/16 -- All 16 functions written; nothing in the boot allocates yet (main is still the placeholder), so the screen stays black and the status is verify. A scratch host 
- verify: `bed/bedpfh.cpp` 4/4 -- Complete; pinned 0x101738..0x101aec (0x101718 clears 7 words, not BedPfh). PC guard: a buffer without the RTPK id leaves the view empty (the ROM hangs in the as
- verify: `dog/doglightpool.cpp` 29/29 -- TU complete; verify because the boot screenshot is still black: GamMan::loadStage is a stub so MagScene::loadLight never fills the pool and nothing draws. Mark 
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
- verify: `mag/magscene.cpp` 89/89 -- Screen still black (textures never load; game/gamfile.cpp callers GamMan::loadSystemData / loadStage are stubs), and nothing calls this slice's new code in the 
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

- 2026-10-22 21:58 `dog/doglightpool.cpp` verify: Wrote the rest of DogLightPool (26 functions 0x1103dc..0x1123b0): init/term/create/alloc/newLight, the id and set-mask edits (allocId, addToSet, removeLight, re
- 2026-10-22 21:37 `rx/rxsnd.cpp` partial: Wrote 28 rx sound functions: SPU heap allocator and free with node helpers, EE to IOP copy (rxSndIopWrite with bounce buffers, rxSndIopCopy), SPU transfer, XVAB
- 2026-10-22 21:21 `rx/rxsnd.cpp` partial: Wrote 85 rx sound functions: init/term, RPC transport (bind, call, direct commands, command queue and flush), config, IOP and SPU heap hooks (3 hidden SPU hook 
- 2026-10-22 21:04 `mag/magscene.cpp` verify: Wrote the rest of mag/magscene.cpp: the wall collision (castSphere, castCylinder, hitWallsSphere, hitWallsCylinder, getGroundHeight; i3dPointSegDist2 named), th
- 2026-10-22 20:31 `mag/magscene.cpp` partial: Wrote 34 more mag/magscene.cpp functions: castRay and the new castWalls (the scene's first 0x2000 bytes are 256 wall segments, tested with i3dSegSegDist2 before
- 2026-10-22 20:19 `mag/magscene.cpp` partial: Wrote MagScene::MagScene 0x1360a0 (the scene's own field defaults: vector pairs, shadow mode -1, the 0x22a0.. scales, sky slots free, strips free, focus -1, the
- 2026-10-22 20:05 `mag/maglifeman.cpp` verify: MagLifeMan complete: all 12 functions (pinned 0x1194b4..0x119b34: clearPod, unlink, insertBefore, insertAfter, init, deleteAll, add, remove, find, removeNo, upd
- 2026-10-22 19:58 `mag/magscene.cpp` partial: Pinned mag/magscene.cpp 0x131184..0x137300 and wrote 21 of its 81 functions: MagScene::update (all ten sub modes: walking, text windows, menu fade out / menu / 

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `unassigned@00228c08` | todo | 0/8/222 | 54.9 | 34 | 1 |
| `unassigned@001beb70` | todo | 0/13/27 | 6.6 | 29 | 0 |
| `unassigned@00164fc0` | todo | 0/8/395 | 104.2 | 29 | 3 |
| `game/gamman.cpp` | todo | 0/5/409 | 82.8 | 26 | 4 |
| `unassigned@001810f4` | todo | 0/10/28 | 2.8 | 25 | 3 |
| `mag/maglifepc.cpp` | todo | 0/11/31 | 6.1 | 24 | 1 |
| `dog/dogactor.cpp` | todo | 0/24/139 | 30.3 | 24 | 0 |
| `unassigned@00140030` | todo | 0/4/6 | 2.7 | 24 | 3 |
| `mag/magloader.cpp` | todo | 0/7/15 | 4.7 | 22 | 2 |
| `dog/dogmodel.cpp` | todo | 0/11/67 | 10.0 | 21 | 0 |
| `unassigned@0020ff34` | todo | 0/3/53 | 23.7 | 20 | 0 |
| `mag/magscenelight.cpp` | todo | 0/5/8 | 2.3 | 20 | 2 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `0022b228` | I3rRoot__CheckIdString | 16 | lib-ctx | stub | - |
| `002155f4` | FUN_002155f4 | 15 | - | missing | - |
| `001be2dc` | FileReader::~FileReader | 8 | ror | missing | game/gamfile.cpp |
| `001be2cc` | FileReader::FileReader | 6 | ror | missing | game/gamfile.cpp |
| `00115cec` | DogModel::release | 6 | ror | stub | dog/dogmodel.cpp |
| `00143fd0` | MagScene::updateFog | 6 | ror | stub | - |
| `0014062c` | MagScene::rotateTenkyu | 6 | ror | stub | - |
| `00140c68` | MagScene::updateRecords853fc | 6 | ror | stub | - |
| `001be734` | FileReadReq::FileReadReq | 5 | ror | missing | game/gamfile.cpp |
| `0011ade4` | MagLifePC::live | 5 | ror | stub | mag/maglifepc.cpp |
| `0021a828` | i3rCopyClipSphere | 5 | lib-ctx | stub | - |
| `00105f0c` | utlStrCopy2 | 4 | ror | stub | utl/utlfile.cpp |
| `00101ccc` | BedPad::update | 4 | ror | stub | bed/bedpad.cpp |
| `001bebec` | GamFont::setFont | 4 | ror | stub | - |
| `0011034c` | FUN_0011034c | 4 | - | stub | dog/dogentity.cpp |
| `0013f980` | MagScene::setLightParam | 4 | ror | stub | mag/magscenelight.cpp |
| `001bedc0` | GamFont::setTarget | 4 | ror | stub | - |
| `001c0254` | GamFont::getGlyphWidth | 4 | ror | stub | - |
| `001e4908` | GamSndMan::findSnd | 4 | ror | stub | - |
| `00114c54` | FUN_00114c54 | 3 | - | missing | dog/dogmodel.cpp |
| `00101b50` | BedPad::setVibration | 3 | ror | stub | bed/bedpad.cpp |
| `0022b3d8` | i3rGetNodeIndexByName | 3 | lib-ctx | stub | - |
| `0010fa6c` | DogEntity::getMatrix | 3 | ror | stub | - |
| `0017772c` | FUN_0017772c | 3 | - | stub | - |
| `0013e548` | MagScene::updateCamera | 3 | ror | stub | - |
| `0013f1e4` | MagScene::clearGamFlag25 | 3 | ror | stub | - |
| `001102b4` | DogEntityEx::setParent | 3 | ror | stub | dog/dogentity.cpp |
| `0010bf00` | DogActor::setParam1d8 | 3 | ror | stub | dog/dogactor.cpp |
| `0021e1d8` | i3dPktDrawLineStrip | 3 | lib-ctx | stub | - |
| `001081a8` | DogMotionBlend::getNo | 3 | ror | stub | dog/dogmotionblend.cpp |
| `0012f790` | MagLifeFadeKey::releaseTexture | 3 | ror | stub | mag/maglifefadekey.cpp |
| `0010ff74` | DogEntity::normalizeRotate | 3 | ror | stub | - |
| `00215bb4` | dbgTraceVector4 | 3 | ror | stub | - |
| `00106db0` | utlNormalize3 | 3 | ror | stub | - |
| `001beb70` | GamFont::GamFont | 2 | ror | missing | - |
| `001a59b8` | FUN_001a59b8 | 2 | - | missing | - |
| `0021db18` | i3dPktDrawStrip | 2 | lib-ctx | stub | - |
| `0010d388` | DogActor::runMotionCmd | 2 | ror | stub | dog/dogactor.cpp |
| `0020e3d8` | seqSetLanguage | 2 | ror | stub | - |
| `0011af10` | MagLifePC::getState17Count | 2 | ror | stub | mag/maglifepc.cpp |

