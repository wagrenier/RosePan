# RosePan status (generated)

Generated 2026-10-29 13:22 at commit `553fc8f` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 543 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **7180**, named **6345** (88%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 543, lib-match-low 133, lib-context 263, ror-inferred 4743
- context-name ledger: 5044 entries; overrides of wrong library matches: 5
- placed in a source file: 5492/7180 in 138 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00150000` | 8/272 (3%) | - |
| `00140000` | 152/293 (52%) | emy/emy_hfm.cpp, mag/magscenecmd.cpp, mag/magloader.cpp |
| `001b0000` | 45/82 (55%) | mnu/mnuitem.cpp, game/gamfile.cpp, game/gamfont.cpp |
| `001a0000` | 155/214 (72%) | mnu/mnusvld.cpp, mnu/mnumcd.cpp, mnu/mnuitem.cpp |
| `00160000` | 215/268 (80%) | emy/emy_manager.cpp, emy/emy_out.cpp, emy/emy_main.cpp |
| `001f0000` | 190/234 (81%) | task/tsk_chr.cpp, task/tsk_scene.cpp, task/naga_chunk.cpp |
| `00190000` | 187/227 (82%) | spr/s2d.cpp, mnu/mnupbook.cpp, mnu/mnubase.cpp |
| `00170000` | 215/251 (86%) | sct/sctlayer.cpp, eff/effman.cpp, snd/sndman.cpp |

## Reconstruction (src/)

- reconstructed RoR functions: **4958**; stubs: **358**

| file | reconstructed | stubs |
|---|---|---|
| `src/bed/bedmemory.cpp` | 16 | 0 |
| `src/bed/bedpad.cpp` | 14 | 0 |
| `src/bed/bedpfh.cpp` | 4 | 0 |
| `src/bed/bedsubmode.cpp` | 3 | 0 |
| `src/debug/mcd_debug.cpp` | 2 | 0 |
| `src/debug/mimu_debug.cpp` | 9 | 0 |
| `src/debug/motnamecnv0.cpp` | 0 | 2 |
| `src/dog/dogactor.cpp` | 147 | 0 |
| `src/dog/dogcamera.cpp` | 8 | 0 |
| `src/dog/dogentity.cpp` | 13 | 0 |
| `src/dog/doglightpool.cpp` | 29 | 0 |
| `src/dog/dogmodel.cpp` | 24 | 0 |
| `src/dog/dogmotionblend.cpp` | 11 | 0 |
| `src/dog/dogtexinfo.cpp` | 16 | 0 |
| `src/eff/eff0.cpp` | 16 | 0 |
| `src/eff/effcameraanim1.cpp` | 3 | 0 |
| `src/eff/efffog3.cpp` | 10 | 0 |
| `src/eff/effman.cpp` | 32 | 0 |
| `src/emy/emy_hfm.cpp` | 76 | 0 |
| `src/emy/emy_main.cpp` | 52 | 0 |
| `src/emy/emy_manager.cpp` | 59 | 0 |
| `src/emy/emy_out.cpp` | 58 | 0 |
| `src/game/gamfile.cpp` | 18 | 0 |
| `src/game/gamfont.cpp` | 28 | 0 |
| `src/game/gamman.cpp` | 411 | 0 |
| `src/game/gamman2.cpp` | 35 | 0 |
| `src/game/gamsnd.cpp` | 74 | 0 |
| `src/game/gamstat.cpp` | 79 | 0 |
| `src/game/gamtrg.cpp` | 23 | 0 |
| `src/i3d/i3cquery.cpp` | 29 | 0 |
| `src/i3d/i3croot.cpp` | 12 | 0 |
| `src/i3d/i3dctx.cpp` | 31 | 0 |
| `src/i3d/i3denv.cpp` | 12 | 0 |
| `src/i3d/i3dgeom.cpp` | 0 | 12 |
| `src/i3d/i3dlayer.cpp` | 80 | 0 |
| `src/i3d/i3dmath.cpp` | 95 | 0 |
| `src/i3d/i3dpacket.cpp` | 54 | 0 |
| `src/i3d/i3dtex.cpp` | 33 | 0 |
| `src/i3d/i3dtim2.cpp` | 7 | 0 |
| `src/i3d/i3dview.cpp` | 19 | 0 |
| `src/i3d/i3dvram.cpp` | 37 | 0 |
| `src/i3d/i3dvu1pkt.cpp` | 17 | 0 |
| `src/i3d/i3mpose.cpp` | 46 | 0 |
| `src/i3d/i3mroot.cpp` | 12 | 0 |
| `src/i3d/i3rblend.cpp` | 18 | 0 |
| `src/i3d/i3rdraw.cpp` | 19 | 0 |
| `src/i3d/i3ritem.cpp` | 444 | 0 |
| `src/i3d/i3rmesh.cpp` | 18 | 0 |
| `src/i3d/i3rmodel.cpp` | 14 | 0 |
| `src/i3d/i3rroot.cpp` | 14 | 0 |
| `src/i3d/i3rtype.cpp` | 78 | 0 |
| `src/lib/unassigned_stubs.cpp` | 10 | 292 |
| `src/mag/maglife.cpp` | 27 | 0 |
| `src/mag/maglifebwn.cpp` | 35 | 0 |
| `src/mag/maglifebwnctrl.cpp` | 73 | 0 |
| `src/mag/maglifebwnmode.cpp` | 0 | 18 |
| `src/mag/maglifefadekey.cpp` | 0 | 6 |
| `src/mag/maglifefader.cpp` | 5 | 0 |
| `src/mag/maglifeman.cpp` | 12 | 0 |
| `src/mag/maglifemenu.cpp` | 45 | 0 |
| `src/mag/maglifemenusp.cpp` | 0 | 12 |
| `src/mag/maglifepc.cpp` | 29 | 0 |
| `src/mag/maglifepc2.cpp` | 28 | 0 |
| `src/mag/maglifepc3.cpp` | 22 | 0 |
| `src/mag/maglifepcbtl.cpp` | 0 | 3 |
| `src/mag/maglifepceff.cpp` | 0 | 3 |
| `src/mag/maglifepcmode.cpp` | 64 | 0 |
| `src/mag/maglifepcmove.cpp` | 26 | 0 |
| `src/mag/maglifewindow.cpp` | 16 | 0 |
| `src/mag/magloader.cpp` | 17 | 0 |
| `src/mag/magscene.cpp` | 89 | 0 |
| `src/mag/magscene2.cpp` | 19 | 0 |
| `src/mag/magscenecamera.cpp` | 16 | 0 |
| `src/mag/magscenecmd.cpp` | 36 | 0 |
| `src/mag/magscenedebug.cpp` | 0 | 1 |
| `src/mag/magscenefocus.cpp` | 16 | 0 |
| `src/mag/magscenelight.cpp` | 10 | 0 |
| `src/mag/magscenemenu.cpp` | 12 | 0 |
| `src/mag/magsceneoption.cpp` | 0 | 1 |
| `src/mag/magscenerender.cpp` | 20 | 0 |
| `src/mag/magscenetenkyu.cpp` | 5 | 0 |
| `src/mag/magsystem.cpp` | 55 | 0 |
| `src/main.cpp` | 11 | 0 |
| `src/mcd/mcdman.cpp` | 42 | 0 |
| `src/mnu/mnubase.cpp` | 36 | 0 |
| `src/mnu/mnumcd.cpp` | 39 | 0 |
| `src/mnu/mnuopt.cpp` | 15 | 0 |
| `src/mnu/mnupbook.cpp` | 43 | 0 |
| `src/mnu/mnuselect.cpp` | 14 | 0 |
| `src/mnu/mnusvld.cpp` | 41 | 0 |
| `src/mnu/mnutitle.cpp` | 20 | 0 |
| `src/msg/msgman.cpp` | 124 | 0 |
| `src/msg/msgsystext.cpp` | 1 | 0 |
| `src/rx/rxfile.cpp` | 168 | 0 |
| `src/rx/rxsnd.cpp` | 222 | 0 |
| `src/rx/rxsys.cpp` | 74 | 6 |
| `src/rx/rxthread.cpp` | 53 | 0 |
| `src/sct/sctlayer.cpp` | 35 | 0 |
| `src/sequence/seqcall.cpp` | 43 | 0 |
| `src/sequence/sequence.cpp` | 19 | 0 |
| `src/sequence/seqvm.cpp` | 144 | 0 |
| `src/snd/rxmpeg.cpp` | 67 | 0 |
| `src/snd/sndman.cpp` | 52 | 0 |
| `src/snd/unitque.cpp` | 9 | 0 |
| `src/spr/s2d.cpp` | 86 | 0 |
| `src/spr/sprman.cpp` | 53 | 0 |
| `src/spr/sprtexture.cpp` | 21 | 0 |
| `src/system/sysnowloading.cpp` | 8 | 0 |
| `src/system/system.cpp` | 11 | 0 |
| `src/task/naga_chunk.cpp` | 30 | 0 |
| `src/task/tsk_chr.cpp` | 151 | 0 |
| `src/task/tsk_enemy.cpp` | 37 | 0 |
| `src/task/tsk_man.cpp` | 25 | 0 |
| `src/task/tsk_scene.cpp` | 76 | 0 |
| `src/utl/utlbtl.cpp` | 0 | 1 |
| `src/utl/utlfile.cpp` | 2 | 0 |
| `src/utl/utlfind.cpp` | 10 | 0 |
| `src/utl/utlgame.cpp` | 49 | 0 |
| `src/utl/utlmath.cpp` | 25 | 0 |
| `src/utl/utlmisc.cpp` | 24 | 0 |
| `src/utl/utlmisc2.cpp` | 10 | 0 |
| `src/utl/utlrand.cpp` | 10 | 0 |
| `src/utl/utltm2.cpp` | 1 | 1 |
| `src/wrd/wrdsrv.cpp` | 11 | 0 |

## Translation units (one session each: `tus.py`)

- 169 TUs: todo 96, partial 2, verify 102, done 17
- next: **i3d/i3dgeom.cpp** (frontier score 20)
- pinned queue: unassigned@0010f944, unassigned@0022e840, unassigned@00106658, unassigned@0021fb50, unassigned@0021c960, unassigned@0013e2ec, unassigned@001258a4, unassigned@00166a1c
- verify: `bed/bedmemory.cpp` 16/16 -- All 16 functions written; nothing in the boot allocates yet (main is still the placeholder), so the screen stays black and the status is verify. A scratch host 
- verify: `bed/bedpfh.cpp` 4/4 -- Complete; pinned 0x101738..0x101aec (0x101718 clears 7 words, not BedPfh). PC guard: a buffer without the RTPK id leaves the view empty (the ROM hangs in the as
- verify: `bed/bedsubmode.cpp` 3/3 -- Complete; nothing on screen shows it yet. push / pop are reached only by the system message window (MagScene::openSysMsgWindow / closeSysMsgWindow, pulseMenu100
- verify: `debug/mcd_debug.cpp` 2/5 -- Nothing calls these functions in the retail ROM, so no screenshot can show them; the smoke run still reaches the disclaimer screen unchanged. mcdDbgMenuTest sta
- verify: `dog/dogactor.cpp` 147/147 -- Status verify: the 15 s smoke run ends on the boot disclaimer and the 200 s New Game run stays black after the film because the pose and draw path below DogActo
- verify: `dog/dogcamera.cpp` 8/10 -- Status verify: the 200 s New Game run (second try; the first looped the boot logos, the known boot race) plays the opening film to Once upon a time, then ends b
- verify: `dog/dogentity.cpp` 11/16 -- Status verify: the 200 s New Game run ends black (one small ring at the bottom) with no assert or crash; the DogEntity stubs no longer appear, the camera (paren
- verify: `dog/doglightpool.cpp` 29/29 -- TU complete; verify because the boot screenshot is still black: GamMan::loadStage is a stub so MagScene::loadLight never fills the pool and nothing draws. Mark 
- verify: `dog/dogmodel.cpp` 24/69 -- Status verify: nothing on screen can show a model yet. On New Game the loader runs (loadPack, setRoot, i3rRelocate, i3rAssignTex) but i3rItemRelocate 0x22c478 i
- verify: `dog/dogmotionblend.cpp` 11/11 -- Verify: the smoke run stays on the disclaimer screen and no actor is drawn yet, because the i3m pose player (unassigned@0022e840: i3mGetPose 0x22e960, i3mGetBle
- verify: `dog/dogtexinfo.cpp` 16/16 -- Pinned 0x1161e4..0x11764c, all 16 functions written. Smoke run: MagScene::init's init / add run without asserts; screen still black (SprMan::draw and the scene 
- verify: `eff/eff0.cpp` 14/14 -- Verify: the 240 s run reaches the bus-stop room and EffMan now creates and deletes EffLight1 (kind 0x21) and EffCameraAnim1 (0xd) through the real Eff0, but eve
- verify: `eff/efffog3.cpp` 10/10 -- Code complete; nothing on screen shows it yet: the 240 s smoke run ends in the bus-stop room with no enemies, so the fog correctly stays the map's (frame mean e
- verify: `eff/effman.cpp` 32/32 -- Verify: the 200 s run creates EffLight1 (kind 0x21) and EffCameraAnim1 (kind 0xd) during the picture book; their init is a stub returning 0, so createEff delete
- verify: `emy/emy_hfm.cpp` 75/75 -- Whole TU written (75 functions, no stubs left in this file). Status verify: the 15 s smoke run still ends on the Atlus disclaimer and creates no enemy, so no Hf
- verify: `emy/emy_main.cpp` 53/88 -- Verify: the 15 s smoke run stops at the disclaimer screen, no enemy exists yet; the first run that reaches a room with an enemy checks it (createEmy, then the c
- verify: `emy/emy_manager.cpp` 58/58 -- Verify: the 15 s smoke run shows the boot disclaimer as before (EmyManager now runs its exec every frame with no enemies; no crash); no enemy is created on the 
- verify: `emy/emy_out.cpp` 58/58 -- No code left in the pinned range. Status verify: no enemy runs in the 15 s smoke run (boot disclaimer only, same picture as before), so nothing on screen shows 
- verify: `game/gamfile.cpp` 18/21 -- Black screen, as before: the code is complete but no boot path shows it yet; every FileReader::read caller that runs in the boot (GamMan::loadSystemData / loadS
- verify: `game/gamman.cpp` 409/409 -- All 409 gamman functions are written; status verify because nothing in the TU draws in the 15 s boot, which still ends on the Atlus disclaimer (correct for that
- verify: `game/gamman2.cpp` 37/37 -- All 37 functions of the TU are written. Nothing on screen shows them: the smoke run never opens the save screen (boot disclaimer only). The first session that d
- verify: `game/gamsnd.cpp` 74/75 -- Code complete; verify until a run reaches a room whose scripts put 3D sounds (native 252 put_*_3d_pos) -- the boot smoke run (logos, disclaimer, MV00_AD film, u
- verify: `game/gamstat.cpp` 74/74 -- All code of the TU is written; nothing in it draws at boot, so verify. The map helpers run when the map menu (0x1949bc / 0x196878, unassigned) opens: the first 
- verify: `game/gamtrg.cpp` 22/22 -- Status verify: all 22 GamArea functions are written. The 140 s New Game run calls update every frame to the end without a fault, but the stage does not draw (bl
- verify: `i3d/i3cquery.cpp` 29/29 -- All 29 functions written; none of this session's six has a caller in the ROM, and the called casts still stop at the i3dgeom box-test stubs, so verify: mark don
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
- verify: `i3d/i3dvu1pkt.cpp` 17/21 -- Complete except 0x21cfb0..0x21d378 (four functions: a REF-tag helper, an ADC index builder, two strip writers): no code or data references in the ROM, left unwr
- verify: `i3d/i3mpose.cpp` 46/46 -- Whole TU written. Nothing in the ROM reaches 0x230688..0x231198 (no jal, j, data word or lui/addiu pair), so no picture can show it; the rest of the TU (key dec
- verify: `i3d/i3mroot.cpp` 12/12 -- Pinned from unassigned@0022b4b8 to i3d/i3mroot.cpp. Host keeps root-relative offsets (i3mPtr), like i3c: the relocators walk with delta 0 and keep +0x16 at 1 (o
- verify: `i3d/i3rblend.cpp` 18/18 -- Seen: the 240 s New Game run reaches the bus-stop room; the shot is pixel-identical to the GamFont progress shot, so nothing on screen shows these yet (uv track
- verify: `i3d/i3rdraw.cpp` 19/19 -- Verify: mark done once the forest shadow is compared with real-game footage (direction, darkness, softness). Doubts in open-items.md Projected shadow, host: the
- verify: `i3d/i3ritem.cpp` 426/426 -- Nothing left to write in i3ritem.cpp. Status verify: none of the accessors has a caller in the ROM, and the smoke run still ends on the boot disclaimer (no mode
- verify: `i3d/i3rmesh.cpp` 14/14 -- Verify: the packet builder's callers (i3rDrawGroupSkinned / Rigid, 0x220838) are not ported, so only i3rPktLayerTest / i3rPktLayerAlpha run, through rosepan_i3d
- verify: `i3d/i3rmodel.cpp` 14/14 -- Verify: nothing on screen reaches it yet. The 200 s New Game run still ends black after the film; DogActor::load 0x10cd5c (dog/dogactor.cpp) is the stub that ke
- verify: `i3d/i3rroot.cpp` 14/14 -- Verify: no model loads in the smoke run yet, so nothing can show it (screen still black, as before). Port trap: the ROM relocates i3r files in place into absolu
- verify: `i3d/i3rtype.cpp` 96/96 -- Verify: complete code, nothing on screen can show it (the 15 s smoke run shows the boot disclaimer; no model draws yet). The nine fixup pairs inside this extent
- verify: `mag/maglife.cpp` 27/49 -- Verify: the 15 s smoke run ends on the boot disclaimer, so no controller runs yet; the first session whose run reaches a room with the PC or Brown moving checks
- verify: `mag/maglifebwn.cpp` 29/42 -- Brown's live now runs in the smoke boot (MagLifeBwnModeWait::init and MagLifeBwnMode::live reached) but nothing on screen shows him yet: the screenshot is the b
- verify: `mag/maglifebwnctrl.cpp` 70/94 -- Complete code, nothing on screen shows it: the 15 s smoke run ends on the boot disclaimer and Brown is not reachable yet, so the whole file stays verify until a
- verify: `mag/maglifeman.cpp` 12/12 -- Status verify: the S2d layers now run live and render every frame (one i3d primitive reaches rosepan_i3d without a texture object), but the screenshot stays bla
- verify: `mag/maglifemenu.cpp` 45/53 -- Complete, nothing on screen yet: the smoke run now opens the title (MagScene::openMenu 0x10005 -> MagLifeMenuTitle) and reaches MnuTitle::isInit / init / draw /
- verify: `mag/maglifepc.cpp` 27/27 -- Verify: the 200 s New Game run binds the PC after the film (setActor -> changeState(0) -> MagLifePCModeNull::init reached) and ends on the picture book's page s
- verify: `mag/maglifepc2.cpp` 25/25 -- Nothing in the boot smoke run picks up an item, attacks or reloads, so the code is unseen: verify. The 240 s run still reaches the first room (Jennifer on the f
- verify: `mag/maglifepc3.cpp` 14/14 -- Status verify: the smoke run ends on the boot disclaimer screen, before MagScene::setPC binds the player, so nothing on screen shows these motions yet; the firs
- verify: `mag/maglifepcmode.cpp` 57/70 -- Every function of the TU is written; nothing on screen can show it yet: the 15 s smoke run ends on the boot disclaimer and the 200 s New Game run binds no actor
- verify: `mag/maglifepcmove.cpp` 12/18 -- Status verify: the 200 s New Game run ends on a dark film frame with no assert or crash; no MagLifePC code moves anything on screen yet (applyMotionMove runs fr
- verify: `mag/magloader.cpp` 17/17 -- Status verify: all 17 functions written, nothing past the title menu draws yet. A New Game run (build.py --run 140 --pad 1800:START,2400:START,2700:CROSS) no lo
- verify: `mag/magscene.cpp` 89/89 -- Screen still black (textures never load; game/gamfile.cpp callers GamMan::loadSystemData / loadStage are stubs), and nothing calls this slice's new code in the 
- verify: `mag/magscene2.cpp` 19/19 -- Complete but unseen: MagScene::start (magscene.cpp) and the GamMan loads are stubs, so no loader runs and the screen stays black (MagScene::draw is a stub too).
- verify: `mag/magscenecamera.cpp` 16/16 -- 200 s New Game run: no assert or crash, ends on the open picture book as before (progress shot); no map draws yet, so no path camera can be seen: verify. Traps 
- verify: `mag/magscenecmd.cpp` 36/36 -- Verify: the 200 s run reaches gamSetBwnFollow (MagLifeBwnCtrl::setLevel stub hit) and still ends black after the narration frame (~6000) as before; fog shows on
- verify: `mag/magscenefocus.cpp` 16/16 -- Verify: the 15 s smoke run shows the disclaimer screen as before; nothing on that path turns focus on (setFocusData runs from an event pack with a file 2, seqFu
- verify: `mag/magscenelight.cpp` 10/10 -- Build, callcheck and layoutcheck green. The 200 s New Game run passes the four previously reached stubs (setLightNo, setLightFlags, setLightParam, copyLightSet)
- verify: `mag/magscenemenu.cpp` 12/12 -- No source-path string in the run: the file name is the port's (open-items.md Open questions). Nothing on screen shows it yet: the 200 s New Game run still ends 
- verify: `mag/magscenerender.cpp` 20/20 -- All 20 functions written; verify because nothing on screen can show it: no boot actor has the 0x60 shadow flags and all i3d layer / view / light functions are s
- verify: `mag/magscenetenkyu.cpp` 5/5 -- Verify: the 200 s run passes the film with no assert; the frame after it is black as before (no model draws: utlDrawI3r 0x106760 and the i3m pose player i3mGetP
- verify: `mag/magsystem.cpp` 55/55 -- All 55 functions written; nothing on screen can show them yet (black: MagScene::draw and i3d are stubs; no caller of the actor methods runs). The Dog classes ar
- verify: `main.cpp` 11/11 -- Status verify: the smoke run boots for real (iopconf.cnf, eight IOP modules and system.ini read from the disc folder through host0:), runs mainInitGame and loop
- verify: `mnu/mnubase.cpp` 36/38 -- Verify: the 200 s New Game run inits the picture book through this loader (setupWork, addFile, openPack read the pack with no file error) and stops in the stubs
- verify: `mnu/mnumcd.cpp` 39/39 -- Status verify: code complete, nothing on screen yet. The boot (MnuTitle::execLogo, check mode 0) now runs MnuMcd::exec, opens its two windows and waits at check
- verify: `mnu/mnupbook.cpp` 43/43 -- Nothing left to write in this TU. verify: the smoke run never opens a diary (types 0x1000 / 0x1024 / 0x102a), and the picture book on the same path is still bla
- verify: `mnu/mnuselect.cpp` 14/14 -- verify: no text list is on screen in the 200 s smoke run (it reaches the first outdoor narration, Suddenly the girl was all alone); MnuTextList shows in the tit
- verify: `mnu/mnusvld.cpp` 41/41 -- Status verify: a Load Game press (pad 1800:START,2400:START,2700:DOWN,2800:CROSS, about 100 s) opens the screen and it matches work/ref/mnusvld (background, Loa
- verify: `mnu/mnutitle.cpp` 20/20 -- Status verify: the code is complete but the plain boot stays black because execLogo waits at step 1 on MnuMcd::startCheck (a stub returning 0; MnuMcd init / pro
- verify: `msg/msgman.cpp` 124/124 -- Status verify: in the bus-stop room the message window shows MsgMan's key icon (S2dMes sprite 0) but no text, because the window's text path is stubbed: MagLife
- verify: `rx/rxfile.cpp` 168/168 -- Status verify: every function in 0x2437f8..0x249b88 is written, but nothing can show it: main 0x2045a8 is still the placeholder (black screen), and the rxthread
- verify: `rx/rxsnd.cpp` 222/222 -- Complete apart from two unreferenced trivia (0x2384b0 empty, 0x23a910 returns its argument). Screen black as before: sound has no picture and the boot reaches n
- verify: `rx/rxsys.cpp` 74/119 -- Status verify: nothing calls rxSysInit yet - main 0x2045a8 is still the placeholder in src/main.cpp, so the screenshot is the black placeholder frame. Next: mai
- verify: `rx/rxthread.cpp` 53/53 -- Complete; verify because main (unassigned@001fd424) is still the placeholder, so nothing calls rxThInit in the smoke run (black frame as before). A temporary ca
- verify: `sct/sctlayer.cpp` 35/35 -- verify: a temporary self-test on two MAPUNIT.BIN packs (not committed) found every flat and sloped cell at its point, gave neighbour-chain paths both ways and s
- verify: `sequence/seqcall.cpp` 43/43 -- TU complete: all 43 functions written; verify because no smoke run reaches native 252 (the boot pad stops at NowLoading) and sound has no picture. Decisions: st
- verify: `sequence/sequence.cpp` 19/19 -- All 19 functions written; the smoke run executes Sequence::exec every frame with empty pools (screen black as before: no image is loaded because GamMan::loadSys
- verify: `sequence/seqvm.cpp` 144/146 -- Status verify: complete code, screen still black. New in the boot: the registered threads run and reach seqFunc004 0x205570 and seqFunc029 0x208b98 (stubs in se
- verify: `setup/i3d_draw_module.cpp` 0/0 -- Hook recorded in open-items Decisions (The native i3d hook). The committed screenshot is the forced test (ROSEPAN_I3D_TEST=1): S2DMAN.RPK @0x800 (PSMT8, Now Loa
- verify: `setup/i3d_mesh_draw.cpp` 0/0 -- Verify: only the forced test (ROSEPAN_I3D_MESH_TEST=1, MagSystem::update) shows it; the screenshot shows Amanda in bind pose, the dress and hair textures matchi
- verify: `snd/sndman.cpp` 52/52 -- Done as verify: SndMan::update runs every frame now, but every rxSnd call (rx/rxsnd.h, unassigned@00237ce8) is a stub, so nothing is heard and nothing on screen
- verify: `spr/s2d.cpp` 86/86 -- All 86 functions written; nothing on screen can show them yet (MagScene::draw is a stub), so verify: the first session whose screenshot shows S2d sprites marks 
- verify: `spr/sprman.cpp` 53/53 -- Verify: nothing calls SprMan::draw with m_active set yet. S2dMovie / S2dMes / S2dBg start only from MagScene::update 0x1327dc (stub), and textures never load (f
- verify: `spr/sprtexture.cpp` 21/21 -- Pinned 0x18f5b0..0x190a78. The code is complete; the smoke run reaches it (S2dMes / S2dMovie loadTexture) but every list ends in the error state because fileGet
- verify: `system/sysnowloading.cpp` 8/8 -- Code complete, nothing on the boot path shows it yet: draw 0x2051cc is called only by the pfh loader 0x1be564 (GamMan load path, unassigned@001bde58), a stub, s
- verify: `system/system.cpp` 11/14 -- Complete except the three static-init functions 0x2041f8 / 0x20439c / 0x2043bc, which the host compiler generates (the globals' definition order in system.cpp r
- verify: `task/naga_chunk.cpp` 30/30 -- Complete code, nothing on screen yet: the 15 s smoke run shows the boot disclaimer (unchanged) and no event scene is reached before the film ends. First session
- verify: `task/tsk_chr.cpp` 150/169 -- All of task/tsk_chr.cpp is now written; the inline dtors and empty terms the brief still lists as todo are in tsk_chr.h. Status verify: nothing on screen reache
- verify: `task/tsk_enemy.cpp` 37/40 -- Nothing on the smoke path starts an enemy (the 15 s run ends on the boot disclaimer, same as before), so the code is verify: the first session whose run reaches
- verify: `task/tsk_man.cpp` 25/25 -- Pinned 0x1e7094..0x1e7f28 out of unassigned@001e6d54 (file name a guess, open-items). CmnITask slots: 1 get_task_type, 2 pause, 3 init, 4 exec(id, time, arg) no
- verify: `task/tsk_scene.cpp` 76/93 -- Visual test: the 15 s smoke run ends on the disclaimer screen as before; nothing in this TU is reached yet (hits need the first room and an attack), so the TU i
- verify: `utl/utlfind.cpp` 10/10 -- Complete code, nothing on screen shows it yet: the 15 s smoke run reaches the disclaimer (drawn right); utlFindBest / utlFindSetItem run only when Brown is told
- verify: `utl/utlgame.cpp` 49/49 -- Complete; verify because nothing on screen shows it yet. The 240 s New Game run reaches the forest bus stop with Jennifer as before (no assert from this file; u
- verify: `utl/utlmath.cpp` 25/25 -- Nothing on screen can show this yet: after the film the stage stays black (a ring indicator only) because utlDrawI3r ends in the i3rDrawModel stub 0x21fe68; the
- verify: `utl/utlmisc.cpp` 24/24 -- Verify: nothing calls this yet (main 0x2045a8 is still the placeholder; the smoke run shows a black frame). When main is reconstructed it calls utlMemInit and i
- verify: `utl/utlmisc2.cpp` 10/10 -- Verify: the 240 s New Game run ends in the bus-stop room with Suddenly, the girl was all alone... as before (progress shot); the headings feed GamArea::update, 
- verify: `utl/utlrand.cpp` 10/10 -- Verify: the 15 s smoke run ends on the boot disclaimer as before; utlRandReset now runs from MagSystem's constructor (no stub hit) but no random draw shows on s
- verify: `wrd/wrdsrv.cpp` 11/14 -- Verified by a temporary self-test (not committed) loading WRD.BIN stages 1 and 13: 49 rooms of 247 index slots, names, floors, captions, offsets, get / findByNa

Recent sessions:

- 2026-10-29 13:22 `i3d/i3cquery.cpp` verify: i3cquery complete: swept-sphere triangle tests 0x2325d0 (segment) / 0x232be8 (ray) as one worker, their casts 0x2331c8 (hidden in 0x232be8, created in Ghidra) /
- 2026-10-29 13:12 `i3d/i3cquery.cpp` partial: i3c collision casts: i3cCast worker 0x232120, tree walk 0x231ed0, shape transform, ray / sphere / cylinder triangle tests with their mode wrappers, the seven ca
- 2026-10-29 12:35 `utl/utlmisc2.cpp` verify: Split unassigned@00216ad0: pinned utl/utlmisc2.cpp 0x217f10..0x2184c0 and wrote all 10 functions: utlAtan2 (stub replaced), utlDirDist and utlStrNCopy (moved fr
- 2026-10-29 12:24 `eff/efffog3.cpp` verify: EffFog3, EffFog3Dark, EffFog3WeakUp and gamCreateEff1a (10 functions, 0x172678..0x172e4c, pinned out of unassigned@0016fb70): the per-stage enemy fog. init pick
- 2026-10-29 11:58 `utl/utlfind.cpp` verify: utl/utlfind.cpp reconstructed: all 10 functions (UtlFind fields m_num / m_again named, FUN_001062d8 named utlFindFirstVFchr), extent pinned 0x1060e8-0x106658; u
- 2026-10-29 11:39 `mnu/mnuselect.cpp` verify: MnuTextList written whole: ctor, dtor, open, close, clear, processPad and its step state machine, allocText and freeText, setItems (labels split on #0#0, mask f
- 2026-10-29 11:27 `bed/bedsubmode.cpp` verify: BedSubMode written: push 0x100258 and pop 0x1002c0 (save / restore the four-word record on a 4-deep stack, asserts at lines 25 / 35) and reset 0x100320 (named, 
- 2026-10-29 10:59 `msg/msgsystext.cpp` done: msgGetSysText 0x1832e8 and its 0x1d4 system texts (data 0x298fe8) written as msg/msgsystext.cpp, byte-exact; stub removed. datadump.py S fields now print byte-e

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `i3d/i3dgeom.cpp` | todo | 0/14/47 | 12.7 | 20 | 0 |
| `unassigned@00149d18` | todo | 0/62/431 | 98.3 | 12 | 0 |
| `mag/maglifefadekey.cpp` | todo | 0/6/9 | 2.5 | 9 | 0 |
| `unassigned@00104970` | todo | 0/1/4 | 1.2 | 7 | 1 |
| `unassigned@00172e4c` | todo | 0/29/51 | 14.1 | 7 | 1 |
| `debug/motnamecnv0.cpp` | todo | 0/2/2 | 0.3 | 7 | 1 |
| `unassigned@00130a2c` | todo | 0/1/5 | 1.2 | 7 | 1 |
| `mnu/mnustock.cpp` | todo | 0/5/35 | 24.3 | 5 | 0 |
| `mnu/mnumap.cpp` | todo | 0/4/18 | 8.7 | 5 | 0 |
| `mnu/mnunow.cpp` | todo | 0/5/15 | 1.2 | 5 | 0 |
| `mnu/mnumovie.cpp` | todo | 0/5/15 | 1.4 | 5 | 0 |
| `mnu/mnuitem.cpp` | todo | 0/5/53 | 60.4 | 5 | 0 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `001be734` | FileReadReq::FileReadReq | 63 | ror | missing | game/gamfile.cpp |
| `001be2dc` | FileReader::~FileReader | 30 | ror | missing | game/gamfile.cpp |
| `001be2cc` | FileReader::FileReader | 25 | ror | missing | game/gamfile.cpp |
| `0012f790` | MagLifeFadeKey::releaseTexture | 3 | ror | stub | mag/maglifefadekey.cpp |
| `00234050` | i3dSegSegDist2 | 3 | ror | stub | i3d/i3dgeom.cpp |
| `00192ac0` | mnuBase::mnuBase | 2 | ror | missing | mnu/mnubase.cpp |
| `00192b3c` | mnuBase::~mnuBase | 2 | ror | missing | mnu/mnubase.cpp |
| `0012c31c` | MagLifePC::startAttack | 2 | ror | stub | mag/maglifepcbtl.cpp |
| `00234390` | i3dSegSegDist2Rgn | 2 | ror | stub | i3d/i3dgeom.cpp |
| `002337e0` | i3dPointSegDist2 | 2 | ror | stub | i3d/i3dgeom.cpp |
| `0012ab18` | MagLifeBwnCtrl::reset | 2 | ror | missing | mag/maglifebwnctrl.cpp |
| `0010456c` | utlHitCheck | 2 | ror | stub | utl/utlbtl.cpp |
| `00104e1c` | utlTm2GetSize | 2 | ror | stub | - |
| `0010559c` | utlTm2ToSepia | 2 | ror | stub | utl/utltm2.cpp |
| `0021628c` | motNameFindChara | 1 | ror | stub | debug/motnamecnv0.cpp |
| `002162e0` | motNameGetCategory | 1 | ror | stub | - |
| `00216184` | motNameConvert | 1 | ror | stub | debug/motnamecnv0.cpp |
| `00281cbc` | DogModel::__tf | 1 | sym | missing | dog/dogmodel.cpp |
| `0012f2f0` | MagLifeFadeKey::loadTexture | 1 | ror | stub | mag/maglifefadekey.cpp |
| `0012f824` | MagLifeFadeKey::start | 1 | ror | stub | mag/maglifefadekey.cpp |
| `0012ca28` | MagLifePC::getAttackHeight | 1 | ror | stub | - |
| `00233628` | i3dPointDist2 | 1 | ror | stub | i3d/i3dgeom.cpp |
| `0012c9c0` | MagLifePC::setState14 | 1 | ror | stub | - |
| `0012bedc` | MagLifePC::calcTargetAngle | 1 | ror | stub | - |
| `002413d8` | rxSysIniFind | 1 | ror | stub | rx/rxsys.cpp |
| `0012c014` | MagLifePC::getHitFrameRange | 1 | ror | stub | - |
| `0012bc98` | MagLifePC::getWeaponHitPos | 1 | ror | stub | mag/maglifepcbtl.cpp |
| `00233c10` | i3dLineSegDist2 | 1 | ror | stub | i3d/i3dgeom.cpp |
| `00241a00` | rxSysOpenTty | 1 | ror | stub | rx/rxsys.cpp |
| `001f9e38` | FUN_001f9e38 | 1 | - | missing | task/tsk_scene.cpp |
| `00236678` | i3dSegCylinderTest | 1 | ror | stub | i3d/i3dgeom.cpp |
| `0014313c` | MagScene::updateDebug | 1 | ror | stub | mag/magscenedebug.cpp |
| `0012a6a0` | MagLifeBwnCtrl::processPad | 1 | ror | missing | mag/maglifebwnctrl.cpp |
| `002365f8` | i3dRayCylinderTest | 1 | ror | stub | i3d/i3dgeom.cpp |
| `0015c304` | DwfMain::initOut | 1 | ror | stub | - |
| `0015a17c` | PigMain::initOut | 1 | ror | stub | - |
| `001583d4` | GotMain::initOut | 1 | ror | stub | - |
| `00156620` | FshMain::initOut | 1 | ror | stub | - |
| `00149d18` | BdsMain::initOut | 1 | ror | stub | - |
| `0014b3bc` | BdlMain::initOut | 1 | ror | stub | - |

