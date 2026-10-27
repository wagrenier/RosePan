# RosePan status (generated)

Generated 2026-10-27 17:03 at commit `5c6d62b` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 543 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6829**, named **4981** (73%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 543, lib-match-low 133, lib-context 253, ror-inferred 3379
- context-name ledger: 3672 entries; overrides of wrong library matches: 5
- placed in a source file: 4332/6829 in 111 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00150000` | 8/272 (3%) | - |
| `00170000` | 52/251 (21%) | snd/sndman.cpp, wrd/wrdsrv.cpp |
| `00120000` | 90/283 (32%) | mag/maglifebwnctrl.cpp, mag/maglifepcmode.cpp, mag/maglifemenu.cpp |
| `00160000` | 96/268 (36%) | emy/emy_manager.cpp, emy/emy_main.cpp |
| `00140000` | 119/293 (41%) | emy/emy_hfm.cpp, mag/magscenecmd.cpp, mag/magloader.cpp |
| `001b0000` | 41/81 (51%) | game/gamfile.cpp, mnu/mnuSelectLanguage.cpp |
| `00110000` | 155/257 (60%) | mag/magsystem.cpp, dog/doglightpool.cpp, mag/maglifepc.cpp |
| `001a0000` | 142/214 (66%) | mnu/mnusvld.cpp, mnu/mnumcd.cpp, mnu/mnutitle.cpp |

## Reconstruction (src/)

- reconstructed RoR functions: **3471**; stubs: **391**

| file | reconstructed | stubs |
|---|---|---|
| `src/bed/bedmemory.cpp` | 16 | 0 |
| `src/bed/bedpad.cpp` | 14 | 0 |
| `src/bed/bedpfh.cpp` | 4 | 0 |
| `src/bed/bedsubmode.cpp` | 0 | 2 |
| `src/debug/motnamecnv0.cpp` | 0 | 2 |
| `src/dog/dogactor.cpp` | 147 | 0 |
| `src/dog/dogentity.cpp` | 13 | 0 |
| `src/dog/doglightpool.cpp` | 29 | 0 |
| `src/dog/dogmodel.cpp` | 24 | 0 |
| `src/dog/dogmotionblend.cpp` | 4 | 7 |
| `src/dog/dogtexinfo.cpp` | 16 | 0 |
| `src/emy/emy_hfm.cpp` | 46 | 10 |
| `src/emy/emy_main.cpp` | 1 | 21 |
| `src/emy/emy_manager.cpp` | 59 | 0 |
| `src/game/gamfile.cpp` | 18 | 0 |
| `src/game/gamman.cpp` | 411 | 0 |
| `src/game/gamman2.cpp` | 35 | 0 |
| `src/game/gamsnd.cpp` | 74 | 0 |
| `src/game/gamstat.cpp` | 79 | 0 |
| `src/game/gamtrg.cpp` | 23 | 0 |
| `src/i3d/i3croot.cpp` | 12 | 0 |
| `src/i3d/i3dctx.cpp` | 31 | 0 |
| `src/i3d/i3denv.cpp` | 12 | 0 |
| `src/i3d/i3dlayer.cpp` | 80 | 0 |
| `src/i3d/i3dmath.cpp` | 95 | 0 |
| `src/i3d/i3dpacket.cpp` | 54 | 0 |
| `src/i3d/i3dtex.cpp` | 33 | 0 |
| `src/i3d/i3dtim2.cpp` | 7 | 0 |
| `src/i3d/i3dview.cpp` | 19 | 0 |
| `src/i3d/i3dvram.cpp` | 37 | 0 |
| `src/i3d/i3mroot.cpp` | 12 | 0 |
| `src/i3d/i3ritem.cpp` | 160 | 0 |
| `src/i3d/i3rmodel.cpp` | 14 | 0 |
| `src/i3d/i3rroot.cpp` | 14 | 0 |
| `src/lib/unassigned_stubs.cpp` | 10 | 264 |
| `src/mag/maglife.cpp` | 0 | 1 |
| `src/mag/maglifebwn.cpp` | 0 | 3 |
| `src/mag/maglifebwnctrl.cpp` | 0 | 17 |
| `src/mag/maglifefadekey.cpp` | 0 | 6 |
| `src/mag/maglifefader.cpp` | 5 | 0 |
| `src/mag/maglifeman.cpp` | 12 | 0 |
| `src/mag/maglifemenu.cpp` | 45 | 0 |
| `src/mag/maglifemenusp.cpp` | 0 | 12 |
| `src/mag/maglifepc.cpp` | 0 | 11 |
| `src/mag/maglifewindow.cpp` | 0 | 14 |
| `src/mag/magloader.cpp` | 17 | 0 |
| `src/mag/magscene.cpp` | 89 | 0 |
| `src/mag/magscene2.cpp` | 19 | 0 |
| `src/mag/magscenecamera.cpp` | 2 | 3 |
| `src/mag/magscenecmd.cpp` | 36 | 0 |
| `src/mag/magscenedebug.cpp` | 0 | 1 |
| `src/mag/magscenefocus.cpp` | 2 | 1 |
| `src/mag/magscenelight.cpp` | 10 | 0 |
| `src/mag/magsceneoption.cpp` | 0 | 1 |
| `src/mag/magscenerender.cpp` | 20 | 0 |
| `src/mag/magscenetenkyu.cpp` | 5 | 0 |
| `src/mag/magsystem.cpp` | 55 | 0 |
| `src/main.cpp` | 11 | 0 |
| `src/mcd/mcdman.cpp` | 42 | 0 |
| `src/mnu/mnubase.cpp` | 36 | 0 |
| `src/mnu/mnumcd.cpp` | 39 | 0 |
| `src/mnu/mnupbook.cpp` | 43 | 0 |
| `src/mnu/mnusvld.cpp` | 41 | 0 |
| `src/mnu/mnutitle.cpp` | 20 | 0 |
| `src/rx/rxfile.cpp` | 168 | 0 |
| `src/rx/rxsnd.cpp` | 222 | 0 |
| `src/rx/rxsys.cpp` | 74 | 6 |
| `src/rx/rxthread.cpp` | 53 | 0 |
| `src/sequence/seqcall.cpp` | 43 | 0 |
| `src/sequence/sequence.cpp` | 19 | 0 |
| `src/sequence/seqvm.cpp` | 143 | 0 |
| `src/snd/rxmpeg.cpp` | 67 | 0 |
| `src/snd/sndman.cpp` | 52 | 0 |
| `src/snd/unitque.cpp` | 9 | 0 |
| `src/spr/s2d.cpp` | 86 | 0 |
| `src/spr/sprman.cpp` | 53 | 0 |
| `src/spr/sprtexture.cpp` | 21 | 0 |
| `src/system/sysnowloading.cpp` | 8 | 0 |
| `src/system/system.cpp` | 11 | 0 |
| `src/task/tsk_chr.cpp` | 151 | 0 |
| `src/task/tsk_man.cpp` | 25 | 0 |
| `src/task/tsk_scene.cpp` | 76 | 0 |
| `src/utl/utlbtl.cpp` | 0 | 1 |
| `src/utl/utlfile.cpp` | 2 | 0 |
| `src/utl/utlfind.cpp` | 0 | 1 |
| `src/utl/utlgame.cpp` | 0 | 6 |
| `src/utl/utlmisc.cpp` | 24 | 0 |
| `src/utl/utltm2.cpp` | 1 | 1 |
| `src/wrd/wrdsrv.cpp` | 11 | 0 |

## Translation units (one session each: `tus.py`)

- 163 TUs: todo 108, partial 14, verify 62, done 9
- next: **dog/dogcamera.cpp** (pinned in the queue)
- pinned queue: dog/dogcamera.cpp, unassigned@0010f944, unassigned@00228c08, dog/dogmotionblend.cpp, unassigned@0022e840, unassigned@00106658, setup/i3d_mesh_draw.cpp, unassigned@0021fb50, unassigned@0021c960, mag/magscenecamera.cpp, unassigned@0013e2ec, unassigned@00140c68, unassigned@00140990, mag/maglifepc.cpp, task/naga_chunk.cpp, mag/maglifebwn.cpp, unassigned@001258a4, mag/maglifebwnctrl.cpp, unassigned@00166a1c
- partial: `emy/emy_hfm.cpp` 46/75 -- Left in emy/emy_hfm.cpp (hand stubs at the end, return -1): HfmOutRide::start 0x146ae4 and its four phases execPhase0..3 (0x146d88, 0x1473e4, 0x147840, 0x1484ec
- partial: `i3d/i3ritem.cpp` 138/168 -- Status partial: left in i3ritem.cpp are the per-type accessors between the fixups (FUN_0022c840 .. FUN_0022e7c8 and the small getters hidden after each fixup's 
- verify: `bed/bedmemory.cpp` 16/16 -- All 16 functions written; nothing in the boot allocates yet (main is still the placeholder), so the screen stays black and the status is verify. A scratch host 
- verify: `bed/bedpfh.cpp` 4/4 -- Complete; pinned 0x101738..0x101aec (0x101718 clears 7 words, not BedPfh). PC guard: a buffer without the RTPK id leaves the view empty (the ROM hangs in the as
- verify: `dog/dogactor.cpp` 147/147 -- Status verify: the 15 s smoke run ends on the boot disclaimer and the 200 s New Game run stays black after the film because the pose and draw path below DogActo
- verify: `dog/dogentity.cpp` 11/16 -- Status verify: the 200 s New Game run ends black (one small ring at the bottom) with no assert or crash; the DogEntity stubs no longer appear, the camera (paren
- verify: `dog/doglightpool.cpp` 29/29 -- TU complete; verify because the boot screenshot is still black: GamMan::loadStage is a stub so MagScene::loadLight never fills the pool and nothing draws. Mark 
- verify: `dog/dogmodel.cpp` 24/69 -- Status verify: nothing on screen can show a model yet. On New Game the loader runs (loadPack, setRoot, i3rRelocate, i3rAssignTex) but i3rItemRelocate 0x22c478 i
- verify: `dog/dogtexinfo.cpp` 16/16 -- Pinned 0x1161e4..0x11764c, all 16 functions written. Smoke run: MagScene::init's init / add run without asserts; screen still black (SprMan::draw and the scene 
- verify: `emy/emy_manager.cpp` 58/58 -- Verify: the 15 s smoke run shows the boot disclaimer as before (EmyManager now runs its exec every frame with no enemies; no crash); no enemy is created on the 
- verify: `game/gamfile.cpp` 18/21 -- Black screen, as before: the code is complete but no boot path shows it yet; every FileReader::read caller that runs in the boot (GamMan::loadSystemData / loadS
- verify: `game/gamman.cpp` 409/409 -- All 409 gamman functions are written; status verify because nothing in the TU draws in the 15 s boot, which still ends on the Atlus disclaimer (correct for that
- verify: `game/gamman2.cpp` 37/37 -- All 37 functions of the TU are written. Nothing on screen shows them: the smoke run never opens the save screen (boot disclaimer only). The first session that d
- verify: `game/gamsnd.cpp` 74/75 -- Code complete; verify until a run reaches a room whose scripts put 3D sounds (native 252 put_*_3d_pos) -- the boot smoke run (logos, disclaimer, MV00_AD film, u
- verify: `game/gamstat.cpp` 74/74 -- All code of the TU is written; nothing in it draws at boot, so verify. The map helpers run when the map menu (0x1949bc / 0x196878, unassigned) opens: the first 
- verify: `game/gamtrg.cpp` 22/22 -- Status verify: all 22 GamArea functions are written. The 140 s New Game run calls update every frame to the end without a fault, but the stage does not draw (bl
- verify: `i3d/i3croot.cpp` 12/12 -- Status verify: DogModel::loadData calls i3cRelocate, setupColliTbl and i3cSetBothFaceEnable, but no smoke run reaches a model load yet and no raw I3D_I3C exists
- verify: `i3d/i3dctx.cpp` 31/31 -- Status verify: the screen stays black because MagScene::draw (no layers) and i3dCtxCloseFrame (0x224798, draw-group module) are stubs, so nothing reaches the pa
- verify: `i3d/i3denv.cpp` 12/12 -- Pinned 0x21a120..0x21a3e8 + 0x21ac80..0x21b220 as one file (a guess: the i3r helpers 0x21a3e8..0x21ac80 sit between; split into i3dfog / i3dlight if evidence ap
- verify: `i3d/i3dlayer.cpp` 80/80 -- Every frame now builds real groups, layers and a sorted DMA chain; the smoke run is stable and still black: nothing on the i3d path reaches a host draw module (
- verify: `i3d/i3dmath.cpp` 95/95 -- Nothing on screen can show it yet (boot still black; the i3d path has no host draw consumer, and the per-frame lightstatus asserts wait on DogLightPool::createL
- verify: `i3d/i3dpacket.cpp` 54/54 -- All packet-module bridges are gone. Black screen: nothing on the host consumes i3d packets (no GS emulation); the user still has to choose the host draw hook (o
- verify: `i3d/i3dtex.cpp` 33/33 -- Screen still black: nothing creates a texture yet (DogTexInfo is stubbed) and no host draw module consumes i3d packets, hence verify. Next: i3d/i3dtim2.cpp (que
- verify: `i3d/i3dtim2.cpp` 7/7 -- Complete code; the smoke screenshot stays black because no host draw module consumes the i3d texture objects yet, so nothing on screen shows TIM2 decoding. The 
- verify: `i3d/i3dview.cpp` 19/19 -- Status verify: views are built every frame (host test: screen centre 2047.5, frustum corner on the rectangle corner, depth 4094.6..1.06, scissor 0..511 x 0..447
- verify: `i3d/i3dvram.cpp` 37/37 -- Complete; nothing on screen shows it yet (screen black: the draw path is stubbed). A host test (init, reserve x3 frames with eviction, compact, end frame, reset
- verify: `i3d/i3mroot.cpp` 12/12 -- Pinned from unassigned@0022b4b8 to i3d/i3mroot.cpp. Host keeps root-relative offsets (i3mPtr), like i3c: the relocators walk with delta 0 and keep +0x16 at 1 (o
- verify: `i3d/i3rmodel.cpp` 14/14 -- Verify: nothing on screen reaches it yet. The 200 s New Game run still ends black after the film; DogActor::load 0x10cd5c (dog/dogactor.cpp) is the stub that ke
- verify: `i3d/i3rroot.cpp` 14/14 -- Verify: no model loads in the smoke run yet, so nothing can show it (screen still black, as before). Port trap: the ROM relocates i3r files in place into absolu
- verify: `mag/maglifeman.cpp` 12/12 -- Status verify: the S2d layers now run live and render every frame (one i3d primitive reaches rosepan_i3d without a texture object), but the screenshot stays bla
- verify: `mag/maglifemenu.cpp` 45/53 -- Complete, nothing on screen yet: the smoke run now opens the title (MagScene::openMenu 0x10005 -> MagLifeMenuTitle) and reaches MnuTitle::isInit / init / draw /
- verify: `mag/magloader.cpp` 17/17 -- Status verify: all 17 functions written, nothing past the title menu draws yet. A New Game run (build.py --run 140 --pad 1800:START,2400:START,2700:CROSS) no lo
- verify: `mag/magscene.cpp` 89/89 -- Screen still black (textures never load; game/gamfile.cpp callers GamMan::loadSystemData / loadStage are stubs), and nothing calls this slice's new code in the 
- verify: `mag/magscene2.cpp` 19/19 -- Complete but unseen: MagScene::start (magscene.cpp) and the GamMan loads are stubs, so no loader runs and the screen stays black (MagScene::draw is a stub too).
- verify: `mag/magscenecmd.cpp` 36/36 -- Verify: the 200 s run reaches gamSetBwnFollow (MagLifeBwnCtrl::setLevel stub hit) and still ends black after the narration frame (~6000) as before; fog shows on
- verify: `mag/magscenelight.cpp` 10/10 -- Build, callcheck and layoutcheck green. The 200 s New Game run passes the four previously reached stubs (setLightNo, setLightFlags, setLightParam, copyLightSet)
- verify: `mag/magscenerender.cpp` 20/20 -- All 20 functions written; verify because nothing on screen can show it: no boot actor has the 0x60 shadow flags and all i3d layer / view / light functions are s
- verify: `mag/magscenetenkyu.cpp` 5/5 -- Verify: the 200 s run passes the film with no assert; the frame after it is black as before (no model draws: utlDrawI3r 0x106760 and the i3m pose player i3mGetP
- verify: `mag/magsystem.cpp` 55/55 -- All 55 functions written; nothing on screen can show them yet (black: MagScene::draw and i3d are stubs; no caller of the actor methods runs). The Dog classes ar
- verify: `main.cpp` 11/11 -- Status verify: the smoke run boots for real (iopconf.cnf, eight IOP modules and system.ini read from the disc folder through host0:), runs mainInitGame and loop
- verify: `mnu/mnubase.cpp` 36/38 -- Verify: the 200 s New Game run inits the picture book through this loader (setupWork, addFile, openPack read the pack with no file error) and stops in the stubs
- verify: `mnu/mnumcd.cpp` 39/39 -- Status verify: code complete, nothing on screen yet. The boot (MnuTitle::execLogo, check mode 0) now runs MnuMcd::exec, opens its two windows and waits at check
- verify: `mnu/mnupbook.cpp` 43/43 -- Nothing left to write in this TU. verify: the smoke run never opens a diary (types 0x1000 / 0x1024 / 0x102a), and the picture book on the same path is still bla
- verify: `mnu/mnusvld.cpp` 41/41 -- Status verify: a Load Game press (pad 1800:START,2400:START,2700:DOWN,2800:CROSS, about 100 s) opens the screen and it matches work/ref/mnusvld (background, Loa
- verify: `mnu/mnutitle.cpp` 20/20 -- Status verify: the code is complete but the plain boot stays black because execLogo waits at step 1 on MnuMcd::startCheck (a stub returning 0; MnuMcd init / pro
- verify: `rx/rxfile.cpp` 168/168 -- Status verify: every function in 0x2437f8..0x249b88 is written, but nothing can show it: main 0x2045a8 is still the placeholder (black screen), and the rxthread
- verify: `rx/rxsnd.cpp` 222/222 -- Complete apart from two unreferenced trivia (0x2384b0 empty, 0x23a910 returns its argument). Screen black as before: sound has no picture and the boot reaches n
- verify: `rx/rxsys.cpp` 74/119 -- Status verify: nothing calls rxSysInit yet - main 0x2045a8 is still the placeholder in src/main.cpp, so the screenshot is the black placeholder frame. Next: mai
- verify: `rx/rxthread.cpp` 53/53 -- Complete; verify because main (unassigned@001fd424) is still the placeholder, so nothing calls rxThInit in the smoke run (black frame as before). A temporary ca
- verify: `sequence/seqcall.cpp` 43/43 -- TU complete: all 43 functions written; verify because no smoke run reaches native 252 (the boot pad stops at NowLoading) and sound has no picture. Decisions: st
- verify: `sequence/sequence.cpp` 19/19 -- All 19 functions written; the smoke run executes Sequence::exec every frame with empty pools (screen black as before: no image is loaded because GamMan::loadSys
- verify: `sequence/seqvm.cpp` 143/143 -- Status verify: complete code, screen still black. New in the boot: the registered threads run and reach seqFunc004 0x205570 and seqFunc029 0x208b98 (stubs in se
- verify: `setup/i3d_draw_module.cpp` 0/0 -- Hook recorded in open-items Decisions (The native i3d hook). The committed screenshot is the forced test (ROSEPAN_I3D_TEST=1): S2DMAN.RPK @0x800 (PSMT8, Now Loa
- verify: `snd/sndman.cpp` 52/52 -- Done as verify: SndMan::update runs every frame now, but every rxSnd call (rx/rxsnd.h, unassigned@00237ce8) is a stub, so nothing is heard and nothing on screen
- verify: `spr/s2d.cpp` 86/86 -- All 86 functions written; nothing on screen can show them yet (MagScene::draw is a stub), so verify: the first session whose screenshot shows S2d sprites marks 
- verify: `spr/sprman.cpp` 53/53 -- Verify: nothing calls SprMan::draw with m_active set yet. S2dMovie / S2dMes / S2dBg start only from MagScene::update 0x1327dc (stub), and textures never load (f
- verify: `spr/sprtexture.cpp` 21/21 -- Pinned 0x18f5b0..0x190a78. The code is complete; the smoke run reaches it (S2dMes / S2dMovie loadTexture) but every list ends in the error state because fileGet
- verify: `system/sysnowloading.cpp` 8/8 -- Code complete, nothing on the boot path shows it yet: draw 0x2051cc is called only by the pfh loader 0x1be564 (GamMan load path, unassigned@001bde58), a stub, s
- verify: `system/system.cpp` 11/14 -- Complete except the three static-init functions 0x2041f8 / 0x20439c / 0x2043bc, which the host compiler generates (the globals' definition order in system.cpp r
- verify: `task/tsk_chr.cpp` 150/169 -- All of task/tsk_chr.cpp is now written; the inline dtors and empty terms the brief still lists as todo are in tsk_chr.h. Status verify: nothing on screen reache
- verify: `task/tsk_man.cpp` 25/25 -- Pinned 0x1e7094..0x1e7f28 out of unassigned@001e6d54 (file name a guess, open-items). CmnITask slots: 1 get_task_type, 2 pause, 3 init, 4 exec(id, time, arg) no
- verify: `task/tsk_scene.cpp` 76/93 -- Visual test: the 15 s smoke run ends on the disclaimer screen as before; nothing in this TU is reached yet (hits need the first room and an attack), so the TU i
- verify: `utl/utlmisc.cpp` 24/24 -- Verify: nothing calls this yet (main 0x2045a8 is still the placeholder; the smoke run shows a black frame). When main is reconstructed it calls utlMemInit and i
- verify: `wrd/wrdsrv.cpp` 11/14 -- Verified by a temporary self-test (not committed) loading WRD.BIN stages 1 and 13: 49 rooms of 247 index slots, names, floors, captions, offsets, get / findByNa

Recent sessions:

- 2026-10-27 17:03 `dog/dogentity.cpp` verify: DogEntity / DogEntityEx written: setRotate, setAddRotY, makeLocalMatrix, normalizeRotate, getMatrixZ, lookAt 0x110144, getMatrixPos / getMatrixRot (two unrefere
- 2026-10-27 16:34 `emy/emy_hfm.cpp` partial: Wrote HfmOutSlap (Init, Exec: chase, slaps 0x14/0x15/0x16 with hit windows and scnChrRequest on PC and dog, chaining, Term), HfmOutBreak, HfmOutWalk2 (Init, Exe
- 2026-10-27 16:22 `game/gamtrg.cpp` verify: GamArea::update 0x1dee78 written: every active trigger's condition bits (rect enter/leave, height, actor distance and direction, PC facing, Cross, timer, ftb sc
- 2026-10-27 15:45 `game/gamman2.cpp` verify: Save image written: writeSaveData 0x1d6078 and readSaveData 0x1d68a0 (FEB_SAVE version 0x10, every section at its EE offset through memcpy, older versions read 
- 2026-10-27 11:06 `game/gamman2.cpp` partial: Hit tests and checkMenuReq written: hitTestObjs 0x1d7c20 over the update list through the member pointer m_hitFunc (hitAtPoint 0x1d4b9c, hitAtSweep 0x1d50a8), t
- 2026-10-27 10:50 `game/gamman2.cpp` partial: Pinned 0x1d3b44..0x1d84c8 as game/gamman2.cpp (GamMan methods past switchVolume, was unassigned@001d3b44). Written: the object update list (relinkObj, queueObjU
- 2026-10-27 09:41 `game/gamtrg.cpp` partial: GamArea (GamMan +0x40, the area triggers) written in game/gamtrg.cpp: constructor, init, cloneTrg, addTrg, stopKind, clear, stopTrgs, clearKind, findFchrOid, ac
- 2026-10-27 09:24 `emy/emy_hfm.cpp` partial: Split the enemy run: pinned emy/emy_hfm.cpp 0x145020..0x149d18 (HfmMain + its outs). Wrote HfmMain whole (initOut, dtor, initAfterParam, decide, exec, after, se

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `unassigned@00166a1c` | todo | 0/36/342 | 94.7 | 144 | 8 |
| `unassigned@00184f2c` | todo | 0/6/84 | 10.0 | 70 | 5 |
| `unassigned@0022e840` | todo | 0/13/99 | 32.1 | 55 | 5 |
| `unassigned@001beb70` | todo | 0/13/27 | 6.6 | 54 | 5 |
| `emy/emy_main.cpp` | partial | 2/23/88 | 11.0 | 51 | 0 |
| `unassigned@00106658` | todo | 0/13/25 | 3.3 | 49 | 4 |
| `mag/maglifepc.cpp` | todo | 0/11/31 | 6.1 | 49 | 6 |
| `unassigned@0021c960` | todo | 0/11/21 | 8.4 | 44 | 6 |
| `unassigned@0013e2ec` | partial | 1/11/15 | 4.4 | 44 | 5 |
| `unassigned@002155a0` | todo | 0/3/13 | 2.1 | 40 | 1 |
| `unassigned@001fcbf8` | todo | 0/6/12 | 0.7 | 40 | 2 |
| `unassigned@00103edc` | todo | 0/5/23 | 1.6 | 38 | 2 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `001be734` | FileReadReq::FileReadReq | 61 | ror | missing | game/gamfile.cpp |
| `001be2dc` | FileReader::~FileReader | 27 | ror | missing | game/gamfile.cpp |
| `001be2cc` | FileReader::FileReader | 23 | ror | missing | game/gamfile.cpp |
| `002155f4` | FUN_002155f4 | 22 | - | missing | - |
| `0010423c` | FUN_0010423c | 20 | - | stub | - |
| `001832e8` | msgGetSysText | 12 | ror | stub | - |
| `00215bb4` | dbgTraceVector4 | 11 | ror | stub | - |
| `001680a0` | EmyOut::setMotion | 11 | ror | stub | - |
| `001874f4` | MsgManSet::setMessage | 11 | ror | stub | - |
| `00187464` | MsgManSet::close | 11 | ror | stub | - |
| `00164118` | EmyMain::EmyMain | 11 | ror | stub | emy/emy_main.cpp |
| `001fccf4` | NagaChunk::find | 10 | ror | stub | - |
| `0018741c` | MsgManSet::open | 10 | ror | stub | - |
| `00164364` | EmyMain::setStartState | 10 | ror | stub | emy/emy_main.cpp |
| `001fcdb8` | NagaChunk::getData | 9 | ror | stub | - |
| `00164508` | EmyMain::loadParam | 9 | ror | stub | emy/emy_main.cpp |
| `00166a1c` | EmyOut::EmyOut | 9 | ror | stub | - |
| `00176cc8` | Obj176a3c::createEff | 8 | ror | stub | - |
| `00217f10` | utlAtan2 | 8 | ror | stub | - |
| `00166ad0` | EmyOut::termCommon | 8 | ror | stub | - |
| `0022e958` | i3mGetLength | 8 | ror | stub | - |
| `00106760` | utlDrawI3r | 8 | ror | stub | - |
| `001694cc` | EmyOut::playSE | 8 | ror | stub | - |
| `0016976c` | emyCanReach | 8 | ror | stub | - |
| `001875c0` | MsgManSet::getState | 7 | ror | stub | - |
| `00218470` | rorDebugPrintf | 6 | ror | missing | - |
| `00166da4` | EmyOut::selectTarget | 6 | ror | stub | - |
| `001fca84` | NagaChunk::NagaChunk | 6 | ror | missing | task/naga_chunk.cpp |
| `001fcbf8` | NagaChunk::rewind | 6 | ror | stub | - |
| `001fcaac` | NagaChunk::~NagaChunk | 6 | ror | missing | task/naga_chunk.cpp |
| `00202f10` | TskEnemyMove::stop | 6 | ror | stub | - |
| `00140c68` | MagScene::updateRecords853fc | 6 | ror | stub | - |
| `0022a988` | FUN_0022a988 | 6 | - | missing | - |
| `0022a978` | FUN_0022a978 | 6 | - | missing | - |
| `0017c340` | FUN_0017c340 | 5 | - | stub | - |
| `001044b8` | utlRandRangeF | 5 | ror | stub | - |
| `001874ac` | MsgManSet::update | 5 | ror | stub | - |
| `0018330c` | MsgMan::MsgMan | 5 | ror | missing | msg/msgman.cpp |
| `00183b24` | MsgMan::~MsgMan | 5 | ror | missing | msg/msgman.cpp |
| `0011ade4` | MagLifePC::live | 5 | ror | stub | mag/maglifepc.cpp |

