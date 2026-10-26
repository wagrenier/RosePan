# RosePan status (generated)

Generated 2026-10-26 13:51 at commit `ee7aedb` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 543 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6660**, named **4079** (61%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 543, lib-match-low 133, lib-context 247, ror-inferred 2481
- context-name ledger: 2728 entries; overrides of wrong library matches: 5
- placed in a source file: 3346/6660 in 96 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00150000` | 0/272 (0%) | - |
| `00160000` | 14/268 (5%) | emy/emy_main.cpp |
| `001f0000` | 25/221 (11%) | task/naga_chunk.cpp, task/tsk_bg_load.cpp, task/tsk_scene.cpp |
| `00140000` | 35/293 (12%) | mag/magloader.cpp, mag/magsceneif.cpp, mag/magscenefocus.cpp |
| `00170000` | 42/251 (17%) | snd/sndman.cpp |
| `00120000` | 73/283 (26%) | mag/maglifebwnctrl.cpp, mag/maglifepcmode.cpp, mag/maglifemenu.cpp |
| `00100000` | 116/301 (39%) | dog/dogactor.cpp, utl/utlgame.cpp, utl/utlmisc.cpp |
| `001e0000` | 67/169 (40%) | game/gamsnd.cpp |

## Reconstruction (src/)

- reconstructed RoR functions: **2429**; stubs: **406**

| file | reconstructed | stubs |
|---|---|---|
| `src/bed/bedmemory.cpp` | 16 | 0 |
| `src/bed/bedpad.cpp` | 14 | 0 |
| `src/bed/bedpfh.cpp` | 4 | 0 |
| `src/bed/bedsubmode.cpp` | 0 | 2 |
| `src/dog/dogactor.cpp` | 5 | 28 |
| `src/dog/dogentity.cpp` | 0 | 3 |
| `src/dog/doglightpool.cpp` | 29 | 0 |
| `src/dog/dogmodel.cpp` | 0 | 12 |
| `src/dog/dogmotionblend.cpp` | 0 | 1 |
| `src/dog/dogtexinfo.cpp` | 16 | 0 |
| `src/game/gamfile.cpp` | 18 | 0 |
| `src/game/gamman.cpp` | 411 | 0 |
| `src/game/gamsnd.cpp` | 0 | 46 |
| `src/game/gamstat.cpp` | 54 | 7 |
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
| `src/lib/unassigned_stubs.cpp` | 5 | 235 |
| `src/mag/maglife.cpp` | 0 | 1 |
| `src/mag/maglifebwn.cpp` | 0 | 3 |
| `src/mag/maglifebwnctrl.cpp` | 0 | 1 |
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
| `src/mag/magscenedebug.cpp` | 0 | 1 |
| `src/mag/magscenefocus.cpp` | 2 | 1 |
| `src/mag/magscenelight.cpp` | 0 | 7 |
| `src/mag/magsceneoption.cpp` | 0 | 1 |
| `src/mag/magscenerender.cpp` | 20 | 0 |
| `src/mag/magscenetenkyu.cpp` | 0 | 1 |
| `src/mag/magsystem.cpp` | 55 | 0 |
| `src/main.cpp` | 11 | 0 |
| `src/mcd/mcdman.cpp` | 42 | 0 |
| `src/mnu/mnumcd.cpp` | 39 | 0 |
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
| `src/utl/utlfile.cpp` | 2 | 0 |
| `src/utl/utlfind.cpp` | 0 | 1 |
| `src/utl/utlgame.cpp` | 0 | 2 |
| `src/utl/utlmisc.cpp` | 24 | 0 |
| `src/utl/utltm2.cpp` | 1 | 1 |

## Translation units (one session each: `tus.py`)

- 157 TUs: todo 113, partial 11, verify 42, done 8
- next: **game/gamstat.cpp** (continuing a partial TU)
- pinned queue: unassigned@001d859c, unassigned@0020ff34, unassigned@001e46b8
- partial: `game/gamstat.cpp` 49/73 -- Left: GamSysData::setField 0x1dc070 (stub, a tangled switch that reads into data for most fields), helpers 0x1d84c8..0x1d87f4, 0x1d88fc, 0x1d8adc..0x1d8c50, 0x1
- verify: `bed/bedmemory.cpp` 16/16 -- All 16 functions written; nothing in the boot allocates yet (main is still the placeholder), so the screen stays black and the status is verify. A scratch host 
- verify: `bed/bedpfh.cpp` 4/4 -- Complete; pinned 0x101738..0x101aec (0x101718 clears 7 words, not BedPfh). PC guard: a buffer without the RTPK id leaves the view empty (the ROM hangs in the as
- verify: `dog/doglightpool.cpp` 29/29 -- TU complete; verify because the boot screenshot is still black: GamMan::loadStage is a stub so MagScene::loadLight never fills the pool and nothing draws. Mark 
- verify: `dog/dogtexinfo.cpp` 16/16 -- Pinned 0x1161e4..0x11764c, all 16 functions written. Smoke run: MagScene::init's init / add run without asserts; screen still black (SprMan::draw and the scene 
- verify: `game/gamfile.cpp` 18/21 -- Black screen, as before: the code is complete but no boot path shows it yet; every FileReader::read caller that runs in the boot (GamMan::loadSystemData / loadS
- verify: `game/gamman.cpp` 409/409 -- All 409 gamman functions are written; status verify because nothing in the TU draws in the 15 s boot, which still ends on the Atlus disclaimer (correct for that
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
- verify: `mag/maglifemenu.cpp` 45/53 -- Complete, nothing on screen yet: the smoke run now opens the title (MagScene::openMenu 0x10005 -> MagLifeMenuTitle) and reaches MnuTitle::isInit / init / draw /
- verify: `mag/magloader.cpp` 17/17 -- Status verify: all 17 functions written, nothing past the title menu draws yet. A New Game run (build.py --run 140 --pad 1800:START,2400:START,2700:CROSS) no lo
- verify: `mag/magscene.cpp` 89/89 -- Screen still black (textures never load; game/gamfile.cpp callers GamMan::loadSystemData / loadStage are stubs), and nothing calls this slice's new code in the 
- verify: `mag/magscene2.cpp` 19/19 -- Complete but unseen: MagScene::start (magscene.cpp) and the GamMan loads are stubs, so no loader runs and the screen stays black (MagScene::draw is a stub too).
- verify: `mag/magscenerender.cpp` 20/20 -- All 20 functions written; verify because nothing on screen can show it: no boot actor has the 0x60 shadow flags and all i3d layer / view / light functions are s
- verify: `mag/magsystem.cpp` 55/55 -- All 55 functions written; nothing on screen can show them yet (black: MagScene::draw and i3d are stubs; no caller of the actor methods runs). The Dog classes ar
- verify: `main.cpp` 11/11 -- Status verify: the smoke run boots for real (iopconf.cnf, eight IOP modules and system.ini read from the disc folder through host0:), runs mainInitGame and loop
- verify: `mnu/mnumcd.cpp` 39/39 -- Status verify: code complete, nothing on screen yet. The boot (MnuTitle::execLogo, check mode 0) now runs MnuMcd::exec, opens its two windows and waits at check
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
- verify: `utl/utlmisc.cpp` 24/24 -- Verify: nothing calls this yet (main 0x2045a8 is still the placeholder; the smoke run shows a black frame). When main is reconstructed it calls utlMemInit and i

Recent sessions:

- 2026-10-26 13:51 `system/system.cpp` verify: Defined system/system.cpp (0x203d94..0x2043dc, the tail of unassigned@001fd424: the globals file before main.cpp) and wrote SysTimer (ctor, dtor, reset, start, 
- 2026-10-26 13:44 `game/gamman.cpp` verify: GamMan slice 9 (last): the ELF inline accessors 0x28eb68..0x28f7e4 written out of line (3D sound forwarders, track status / volume, list pointers, PC / dog oid,
- 2026-10-26 13:31 `game/gamman.cpp` partial: GamMan slice 8: the ELF inline accessors 0x28e254..0x28ee00 written out of line (flags, chr numbers and pointers, task ids, getSysTime / getElapsedSysTime, set_
- 2026-10-26 10:28 `game/gamman.cpp` partial: GamMan slice 7: the native 151 object helpers - addObjColliSegment / Box / Quad (private walls, param 0x18 and 0x1003), setObjAttr / clearObjAttr, placeObjAt wi
- 2026-10-26 10:09 `game/gamman.cpp` partial: GamMan slice 6: the stage resets and object teardown - resetStage, releaseObjs (kinds 0..3 by oid image), releaseAllChrs / releaseAll / term, releaseVFchrs, cle
- 2026-10-26 09:53 `game/gamman.cpp` partial: GamMan slice 5: setMurder, loadChapterSe, loadMapUnit with showMapUnits / loadMapPack / resetAVFchrMotions, setPcStartPos with setPcPos / setPcRot, storePcTrans
- 2026-10-26 09:06 `game/gamstat.cpp` partial: game/gamstat.cpp pinned (0x1d84c8..0x1dd1e8, was unassigned@001d859c) with gamstat.h: ObjStat (ctor, pos / rot, secta cells, base params, setHide, releaseActor,
- 2026-10-26 08:26 `mag/magloader.cpp` verify: mag/magloader.cpp: reconstruct all 17 functions (MagLoaderIndex create/add/find/clear/release, MagCharaLoader ctor/init/initFromPacks/load/getIndex/getId/getCat

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `unassigned@001e6d54` | partial | 1/34/219 | 68.6 | 206 | 6 |
| `game/gamsnd.cpp` | todo | 0/47/73 | 24.0 | 189 | 9 |
| `unassigned@00143d3c` | todo | 0/20/602 | 129.5 | 54 | 2 |
| `unassigned@00164fc0` | todo | 0/18/395 | 104.2 | 51 | 4 |
| `mag/magscenelight.cpp` | todo | 0/6/8 | 2.3 | 51 | 4 |
| `unassigned@001beb70` | todo | 0/13/27 | 6.6 | 44 | 3 |
| `dog/dogactor.cpp` | partial | 5/28/146 | 31.5 | 41 | 2 |
| `unassigned@001d3b44` | partial | 2/11/37 | 18.4 | 37 | 1 |
| `unassigned@00184f2c` | todo | 0/5/84 | 10.0 | 37 | 3 |
| `unassigned@001dd5e8` | todo | 0/12/18 | 14.0 | 35 | 0 |
| `mag/maglifewindow.cpp` | todo | 0/14/29 | 3.7 | 33 | 4 |
| `unassigned@001ab474` | todo | 0/28/119 | 74.0 | 31 | 0 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `001e7d7c` | CmnTaskMan::getTask | 58 | ror | stub | - |
| `001be734` | FileReadReq::FileReadReq | 54 | ror | missing | game/gamfile.cpp |
| `001e7724` | CmnTaskMan::killTask | 35 | ror | stub | - |
| `002155f4` | FUN_002155f4 | 22 | - | missing | - |
| `001e71fc` | CmnTaskMan::addTask | 19 | ror | stub | - |
| `001be2dc` | FileReader::~FileReader | 18 | ror | missing | game/gamfile.cpp |
| `001e4458` | GamSndMan::loadSePack | 17 | ror | stub | game/gamsnd.cpp |
| `001be2cc` | FileReader::FileReader | 16 | ror | missing | game/gamfile.cpp |
| `0013f980` | MagScene::setLightParam | 14 | ror | stub | mag/magscenelight.cpp |
| `001e5660` | GamSndMan::funcStr | 11 | ror | stub | game/gamsnd.cpp |
| `001e0c2c` | GamArea::ctl_trg_flg | 10 | ror | stub | - |
| `0013fb3c` | MagScene::copyLightSet | 9 | ror | stub | mag/magscenelight.cpp |
| `001f184c` | TskFrameFunc::TskFrameFunc | 9 | ror | stub | - |
| `001d5da0` | GamMan::relinkObj | 9 | ror | stub | - |
| `00144334` | gamSetBwnFollow | 8 | ror | stub | - |
| `001e46b8` | GamSndMan::freeSePack | 7 | ror | stub | game/gamsnd.cpp |
| `0017ea44` | WrdTable::get | 7 | ror | stub | - |
| `001e5e58` | GamSndMan::funcSE | 7 | ror | stub | game/gamsnd.cpp |
| `00187464` | MsgManSet::close | 7 | ror | stub | - |
| `001d5bc8` | GamMan::queueObjUpdate | 7 | ror | stub | - |
| `0010ff74` | DogEntity::normalizeRotate | 6 | ror | stub | - |
| `00115cec` | DogModel::release | 6 | ror | stub | dog/dogmodel.cpp |
| `001e1620` | GamSndMan::unpush_snd | 6 | ror | stub | game/gamsnd.cpp |
| `001e6734` | GamSndMan::funcSnd | 6 | ror | stub | game/gamsnd.cpp |
| `00143fd0` | MagScene::updateFog | 6 | ror | stub | - |
| `0014062c` | MagScene::rotateTenkyu | 6 | ror | stub | - |
| `00140c68` | MagScene::updateRecords853fc | 6 | ror | stub | - |
| `001832e8` | msgGetSysText | 6 | ror | stub | - |
| `00218470` | rorDebugPrintf | 5 | ror | missing | - |
| `001e7414` | CmnTaskMan::pauseTask | 5 | ror | stub | - |
| `001dd5e8` | GamArea::addTrg | 5 | ror | stub | - |
| `00143d7c` | MagScene::setFog | 5 | ror | stub | - |
| `0011ade4` | MagLifePC::live | 5 | ror | stub | mag/maglifepc.cpp |
| `001874f4` | MsgManSet::setMessage | 5 | ror | stub | - |
| `001e4908` | GamSndMan::findSnd | 5 | ror | stub | game/gamsnd.cpp |
| `001d8064` | GamMan::findSectaCell | 5 | ror | stub | - |
| `001634d4` | EmyManager::releaseObj | 5 | ror | stub | - |
| `0021a828` | i3rCopyClipSphere | 5 | lib-ctx | stub | - |
| `001bebec` | GamFont::setFont | 4 | ror | stub | - |
| `001ddc80` | GamArea::stopTrgs | 4 | ror | stub | - |

