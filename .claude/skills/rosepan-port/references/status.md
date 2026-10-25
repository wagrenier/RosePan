# RosePan status (generated)

Generated 2026-10-25 18:35 at commit `da9ec2d` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 543 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6655**, named **3410** (51%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 543, lib-match-low 133, lib-context 230, ror-inferred 1812
- context-name ledger: 2042 entries; overrides of wrong library matches: 5
- placed in a source file: 3060/6655 in 92 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00150000` | 0/272 (0%) | - |
| `001a0000` | 0/211 (0%) | mnu/mnusvld.cpp, mnu/mnuselect.cpp, mnu/mnudlg.cpp |
| `00160000` | 2/268 (1%) | emy/emy_main.cpp |
| `001f0000` | 10/221 (5%) | task/naga_chunk.cpp, task/tsk_bg_load.cpp, task/tsk_scene.cpp |
| `00140000` | 19/293 (6%) | mag/magloader.cpp, mag/magsceneif.cpp, mag/magscenefocus.cpp |
| `00120000` | 27/283 (10%) | mag/maglifebwnctrl.cpp, mag/maglifepcmode.cpp, mag/maglifemenu.cpp |
| `001e0000` | 23/169 (14%) | game/gamsnd.cpp |
| `00170000` | 35/251 (14%) | snd/sndman.cpp |

## Reconstruction (src/)

- reconstructed RoR functions: **1669**; stubs: **336**

| file | reconstructed | stubs |
|---|---|---|
| `src/bed/bedmemory.cpp` | 16 | 0 |
| `src/bed/bedpad.cpp` | 0 | 5 |
| `src/bed/bedpfh.cpp` | 4 | 0 |
| `src/bed/bedsubmode.cpp` | 0 | 2 |
| `src/dog/dogactor.cpp` | 0 | 25 |
| `src/dog/dogentity.cpp` | 0 | 3 |
| `src/dog/doglightpool.cpp` | 29 | 0 |
| `src/dog/dogmodel.cpp` | 0 | 11 |
| `src/dog/dogmotionblend.cpp` | 0 | 1 |
| `src/dog/dogtexinfo.cpp` | 16 | 0 |
| `src/game/gamfile.cpp` | 18 | 0 |
| `src/game/gamman.cpp` | 78 | 23 |
| `src/game/gamsnd.cpp` | 0 | 13 |
| `src/i3d/i3dctx.cpp` | 31 | 0 |
| `src/i3d/i3denv.cpp` | 12 | 0 |
| `src/i3d/i3dlayer.cpp` | 80 | 0 |
| `src/i3d/i3dmath.cpp` | 95 | 0 |
| `src/i3d/i3dpacket.cpp` | 54 | 0 |
| `src/i3d/i3dtex.cpp` | 33 | 0 |
| `src/i3d/i3dtim2.cpp` | 7 | 0 |
| `src/i3d/i3dview.cpp` | 19 | 0 |
| `src/i3d/i3dvram.cpp` | 37 | 0 |
| `src/i3d/i3rroot.cpp` | 14 | 0 |
| `src/lib/unassigned_stubs.cpp` | 1 | 144 |
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
| `src/rx/rxsnd.cpp` | 222 | 0 |
| `src/rx/rxsys.cpp` | 74 | 6 |
| `src/rx/rxthread.cpp` | 53 | 0 |
| `src/sequence/seqcall.cpp` | 12 | 29 |
| `src/sequence/sequence.cpp` | 19 | 0 |
| `src/sequence/seqvm.cpp` | 125 | 5 |
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

- 159 TUs: todo 129, partial 7, verify 33, done 2
- next: **sequence/seqvm.cpp** (pinned in the queue)
- pinned queue: sequence/seqvm.cpp, sequence/seqcall.cpp, unassigned@0020ff34, game/gamman.cpp, unassigned@001fd424, unassigned@001e46b8, bed/bedpad.cpp, mag/maglifemenu.cpp, unassigned@001a6830, snd/rxmpeg.cpp, unassigned@001810f4
- partial: `game/gamman.cpp` 77/409 -- Screen still black: nothing in this slice draws, and boot stage 99 hits loadStageSnd's default (no packs, as in the ROM). The boot's blockers are the natives se
- partial: `sequence/seqcall.cpp` 12/43 -- Left: 29 handler stubs in seqcall.cpp (004, 006, 007, 009, 012, 014, 016, 019, 020, 023, 026, 028, 029, 036, 038, 039, 100, 106, 151..281); write each when a de
- partial: `sequence/seqvm.cpp` 125/143 -- Left in this file: the other thread starters and queries 0x211830 (imdy), 0x211be4 (single), 0x2120ac, 0x212330 (trg), 0x212598, 0x2129b8, 0x212a5c, 0x212d08, 0
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
- verify: `i3d/i3rroot.cpp` 14/14 -- Verify: no model loads in the smoke run yet, so nothing can show it (screen still black, as before). Port trap: the ROM relocates i3r files in place into absolu
- verify: `mag/maglifeman.cpp` 12/12 -- Status verify: the S2d layers now run live and render every frame (one i3d primitive reaches rosepan_i3d without a texture object), but the screenshot stays bla
- verify: `mag/magscene.cpp` 89/89 -- Screen still black (textures never load; game/gamfile.cpp callers GamMan::loadSystemData / loadStage are stubs), and nothing calls this slice's new code in the 
- verify: `mag/magscene2.cpp` 19/19 -- Complete but unseen: MagScene::start (magscene.cpp) and the GamMan loads are stubs, so no loader runs and the screen stays black (MagScene::draw is a stub too).
- verify: `mag/magscenerender.cpp` 20/20 -- All 20 functions written; verify because nothing on screen can show it: no boot actor has the 0x60 shadow flags and all i3d layer / view / light functions are s
- verify: `mag/magsystem.cpp` 55/55 -- All 55 functions written; nothing on screen can show them yet (black: MagScene::draw and i3d are stubs; no caller of the actor methods runs). The Dog classes ar
- verify: `main.cpp` 11/11 -- Status verify: the smoke run boots for real (iopconf.cnf, eight IOP modules and system.ini read from the disc folder through host0:), runs mainInitGame and loop
- verify: `rx/rxfile.cpp` 168/168 -- Status verify: every function in 0x2437f8..0x249b88 is written, but nothing can show it: main 0x2045a8 is still the placeholder (black screen), and the rxthread
- verify: `rx/rxsnd.cpp` 222/222 -- Complete apart from two unreferenced trivia (0x2384b0 empty, 0x23a910 returns its argument). Screen black as before: sound has no picture and the boot reaches n
- verify: `rx/rxsys.cpp` 74/119 -- Status verify: nothing calls rxSysInit yet - main 0x2045a8 is still the placeholder in src/main.cpp, so the screenshot is the black placeholder frame. Next: mai
- verify: `rx/rxthread.cpp` 53/53 -- Complete; verify because main (unassigned@001fd424) is still the placeholder, so nothing calls rxThInit in the smoke run (black frame as before). A temporary ca
- verify: `sequence/sequence.cpp` 19/19 -- All 19 functions written; the smoke run executes Sequence::exec every frame with empty pools (screen black as before: no image is loaded because GamMan::loadSys
- verify: `setup/i3d_draw_module.cpp` 0/0 -- Hook recorded in open-items Decisions (The native i3d hook). The committed screenshot is the forced test (ROSEPAN_I3D_TEST=1): S2DMAN.RPK @0x800 (PSMT8, Now Loa
- verify: `snd/sndman.cpp` 52/52 -- Done as verify: SndMan::update runs every frame now, but every rxSnd call (rx/rxsnd.h, unassigned@00237ce8) is a stub, so nothing is heard and nothing on screen
- verify: `spr/s2d.cpp` 86/86 -- All 86 functions written; nothing on screen can show them yet (MagScene::draw is a stub), so verify: the first session whose screenshot shows S2d sprites marks 
- verify: `spr/sprman.cpp` 53/53 -- Verify: nothing calls SprMan::draw with m_active set yet. S2dMovie / S2dMes / S2dBg start only from MagScene::update 0x1327dc (stub), and textures never load (f
- verify: `spr/sprtexture.cpp` 21/21 -- Pinned 0x18f5b0..0x190a78. The code is complete; the smoke run reaches it (S2dMes / S2dMovie loadTexture) but every list ends in the error state because fileGet
- verify: `system/sysnowloading.cpp` 8/8 -- Code complete, nothing on the boot path shows it yet: draw 0x2051cc is called only by the pfh loader 0x1be564 (GamMan load path, unassigned@001bde58), a stub, s
- verify: `utl/utlmisc.cpp` 24/24 -- Verify: nothing calls this yet (main 0x2045a8 is still the placeholder; the smoke run shows a black frame). When main is reconstructed it calls utlMemInit and i

Recent sessions:

- 2026-10-25 18:35 `sequence/seqcall.cpp` partial: Script natives the boot reaches: seqFunc001 (new game state), 002 (default scene lights), 021 (GamMan state service, 32 cases), 027 (object service, 205 cases),
- 2026-10-25 18:23 `game/gamman.cpp` partial: GamMan slice 3: loadStageSnd (the chapter SE packs 1xxx / 3200..3210.se), the item code (addItem, delItem, delItemAll, equipItem with the held object on node p_
- 2026-10-25 18:10 `game/gamman.cpp` partial: GamMan slice 2: onStageSeqEnd, afterSeqExec and execStageReq (the stage change), setImage, resetObj, pause / resume / enterMenu with pauseForMenu, resumeFromMen
- 2026-10-25 17:39 `game/gamman.cpp` partial: GamMan boot path: constructor (nine node pools, item vector), init, initGameState, loadSystemData (system pack: GamSysData, SE packs, sound tables, system scrip
- 2026-10-25 17:15 `sequence/seqvm.cpp` partial: The sequence script VM: pinned sequence/seqvm.cpp 0x20ff34..0x2155a0 (was unassigned@0020ff34), created and named 104 Ghidra functions (the 99 opcode handlers o
- 2026-10-25 16:37 `sequence/sequence.cpp` verify: Sequence: constructor, init / term, the three image loaders, stopImages, the per-frame exec with runImage and reapThreads, releaseThread, the global / stage var
- 2026-10-25 16:23 `sequence/seqcall.cpp` partial: Script native functions: pinned sequence/seqcall.cpp 0x2053f4..0x20e3d8 (the 512-entry table data 0x2d90f8 the sequence VM calls), named 41 handlers seqFuncNNN 
- 2026-10-25 15:53 `i3d/i3rroot.cpp` verify: Pinned i3d/i3rroot.cpp 0x22b1b0-0x22b4b8 out of unassigned@00228c08 and wrote all 14 functions (3 hidden ones created in Ghidra): the I3D_BIN id check, i3rSetRe

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `game/gamsnd.cpp` | todo | 0/14/72 | 23.9 | 78 | 8 |
| `unassigned@001d859c` | todo | 0/20/75 | 19.9 | 69 | 3 |
| `unassigned@001e6cac` | todo | 0/12/220 | 68.8 | 50 | 5 |
| `game/gamman.cpp` | partial | 77/23/409 | 82.8 | 43 | 3 |
| `unassigned@00164fc0` | todo | 0/10/395 | 104.2 | 37 | 4 |
| `unassigned@00143d3c` | todo | 0/7/602 | 129.5 | 30 | 2 |
| `unassigned@001beb70` | todo | 0/13/27 | 6.6 | 29 | 0 |
| `mag/magscenelight.cpp` | todo | 0/5/8 | 2.3 | 28 | 3 |
| `unassigned@0010f944` | todo | 0/5/12 | 2.2 | 26 | 3 |
| `unassigned@001810f4` | todo | 0/10/28 | 2.8 | 25 | 3 |
| `dog/dogactor.cpp` | todo | 0/25/139 | 30.3 | 25 | 0 |
| `mag/maglifepc.cpp` | todo | 0/11/31 | 6.1 | 24 | 1 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `001be734` | FileReadReq::FileReadReq | 30 | ror | missing | game/gamfile.cpp |
| `002155f4` | FUN_002155f4 | 15 | - | missing | - |
| `001be2dc` | FileReader::~FileReader | 12 | ror | missing | game/gamfile.cpp |
| `001e4458` | GamSndMan::loadSePack | 12 | ror | stub | game/gamsnd.cpp |
| `001be2cc` | FileReader::FileReader | 10 | ror | missing | game/gamfile.cpp |
| `001e46b8` | GamSndMan::freeSePack | 7 | ror | stub | game/gamsnd.cpp |
| `001dd00c` | GamSysData::testFlags | 7 | ror | stub | - |
| `00144334` | gamSetBwnFollow | 7 | ror | stub | - |
| `001dc48c` | GamSysData::getField | 7 | ror | stub | - |
| `0013f980` | MagScene::setLightParam | 7 | ror | stub | mag/magscenelight.cpp |
| `001d95c4` | ObjStat::setHide | 7 | ror | stub | - |
| `001d9f10` | ObjStat::clearSecta | 6 | ror | stub | - |
| `00115cec` | DogModel::release | 6 | ror | stub | dog/dogmodel.cpp |
| `00143fd0` | MagScene::updateFog | 6 | ror | stub | - |
| `0014062c` | MagScene::rotateTenkyu | 6 | ror | stub | - |
| `00140c68` | MagScene::updateRecords853fc | 6 | ror | stub | - |
| `001d5da0` | GamMan::relinkObj | 6 | ror | stub | - |
| `001e7d7c` | CmnTaskMan::getTask | 5 | ror | stub | - |
| `0011ade4` | MagLifePC::live | 5 | ror | stub | mag/maglifepc.cpp |
| `001e71fc` | CmnTaskMan::addTask | 5 | ror | stub | - |
| `001e0c2c` | GamArea::ctl_trg_flg | 5 | ror | stub | - |
| `0021a828` | i3rCopyClipSphere | 5 | lib-ctx | stub | - |
| `0010ff74` | DogEntity::normalizeRotate | 4 | ror | stub | - |
| `001d9830` | ObjStat::load | 4 | ror | stub | - |
| `00105f0c` | utlStrCopy2 | 4 | ror | stub | utl/utlfile.cpp |
| `00101ccc` | BedPad::update | 4 | ror | stub | bed/bedpad.cpp |
| `001bebec` | GamFont::setFont | 4 | ror | stub | - |
| `001cc250` | GamMan::addChr | 4 | ror | stub | game/gamman.cpp |
| `0011034c` | FUN_0011034c | 4 | - | stub | dog/dogentity.cpp |
| `001bedc0` | GamFont::setTarget | 4 | ror | stub | - |
| `001c0254` | GamFont::getGlyphWidth | 4 | ror | stub | - |
| `001e4908` | GamSndMan::findSnd | 4 | ror | stub | game/gamsnd.cpp |
| `00114c54` | FUN_00114c54 | 3 | - | missing | dog/dogmodel.cpp |
| `00101b50` | BedPad::setVibration | 3 | ror | stub | bed/bedpad.cpp |
| `0010fa6c` | DogEntity::getMatrix | 3 | ror | stub | - |
| `001d966c` | ObjStat::releaseActor | 3 | ror | stub | - |
| `0017772c` | FUN_0017772c | 3 | - | stub | - |
| `0013e548` | MagScene::updateCamera | 3 | ror | stub | - |
| `0013f1e4` | MagScene::clearGamFlag25 | 3 | ror | stub | - |
| `001102b4` | DogEntityEx::setParent | 3 | ror | stub | dog/dogentity.cpp |

