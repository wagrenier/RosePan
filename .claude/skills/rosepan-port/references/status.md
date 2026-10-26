# RosePan status (generated)

Generated 2026-10-26 07:18 at commit `d75810a` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 543 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6660**, named **3908** (59%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 543, lib-match-low 133, lib-context 247, ror-inferred 2310
- context-name ledger: 2557 entries; overrides of wrong library matches: 5
- placed in a source file: 3257/6660 in 95 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00150000` | 0/272 (0%) | - |
| `00160000` | 11/268 (4%) | emy/emy_main.cpp |
| `00140000` | 23/293 (8%) | mag/magloader.cpp, mag/magsceneif.cpp, mag/magscenefocus.cpp |
| `001f0000` | 23/221 (10%) | task/naga_chunk.cpp, task/tsk_bg_load.cpp, task/tsk_scene.cpp |
| `00170000` | 38/251 (15%) | snd/sndman.cpp |
| `001e0000` | 32/169 (19%) | game/gamsnd.cpp |
| `00120000` | 73/283 (26%) | mag/maglifebwnctrl.cpp, mag/maglifepcmode.cpp, mag/maglifemenu.cpp |
| `00100000` | 115/301 (38%) | dog/dogactor.cpp, utl/utlgame.cpp, utl/utlmisc.cpp |

## Reconstruction (src/)

- reconstructed RoR functions: **2036**; stubs: **439**

| file | reconstructed | stubs |
|---|---|---|
| `src/bed/bedmemory.cpp` | 16 | 0 |
| `src/bed/bedpad.cpp` | 14 | 0 |
| `src/bed/bedpfh.cpp` | 4 | 0 |
| `src/bed/bedsubmode.cpp` | 0 | 2 |
| `src/dog/dogactor.cpp` | 5 | 27 |
| `src/dog/dogentity.cpp` | 0 | 3 |
| `src/dog/doglightpool.cpp` | 29 | 0 |
| `src/dog/dogmodel.cpp` | 0 | 12 |
| `src/dog/dogmotionblend.cpp` | 0 | 1 |
| `src/dog/dogtexinfo.cpp` | 16 | 0 |
| `src/game/gamfile.cpp` | 18 | 0 |
| `src/game/gamman.cpp` | 101 | 56 |
| `src/game/gamsnd.cpp` | 0 | 14 |
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
| `src/lib/unassigned_stubs.cpp` | 5 | 244 |
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
| `src/mag/magloader.cpp` | 0 | 7 |
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
| `src/sequence/seqcall.cpp` | 42 | 1 |
| `src/sequence/sequence.cpp` | 19 | 0 |
| `src/sequence/seqvm.cpp` | 143 | 0 |
| `src/snd/rxmpeg.cpp` | 67 | 0 |
| `src/snd/sndman.cpp` | 52 | 0 |
| `src/snd/unitque.cpp` | 9 | 0 |
| `src/spr/s2d.cpp` | 86 | 0 |
| `src/spr/sprman.cpp` | 53 | 0 |
| `src/spr/sprtexture.cpp` | 21 | 0 |
| `src/system/sysnowloading.cpp` | 8 | 0 |
| `src/utl/utlfile.cpp` | 2 | 0 |
| `src/utl/utlfind.cpp` | 0 | 1 |
| `src/utl/utlgame.cpp` | 0 | 2 |
| `src/utl/utlmisc.cpp` | 24 | 0 |
| `src/utl/utltm2.cpp` | 1 | 1 |

## Translation units (one session each: `tus.py`)

- 157 TUs: todo 115, partial 12, verify 38, done 8
- next: **sequence/seqcall.cpp** (pinned in the queue)
- pinned queue: sequence/seqcall.cpp, unassigned@0020ff34, game/gamman.cpp, unassigned@001fd424, unassigned@001e46b8
- partial: `game/gamman.cpp` 100/409 -- Screen still black: nothing in this slice draws, and boot stage 99 hits loadStageSnd's default (no packs, as in the ROM). The boot's blockers are the natives se
- partial: `sequence/seqcall.cpp` 42/43 -- Left: native 252 only (sound, jump table data 0x301280, 201 cases, 0xd2c bytes; about 20 GamSndMan callees FUN_001e1620..FUN_001e5e58 plus FUN_001c7500 / 75bc /
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
- verify: `mag/maglifemenu.cpp` 45/53 -- Complete, nothing on screen yet: the smoke run now opens the title (MagScene::openMenu 0x10005 -> MagLifeMenuTitle) and reaches MnuTitle::isInit / init / draw /
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
- verify: `sequence/sequence.cpp` 19/19 -- All 19 functions written; the smoke run executes Sequence::exec every frame with empty pools (screen black as before: no image is loaded because GamMan::loadSys
- verify: `sequence/seqvm.cpp` 143/143 -- Status verify: complete code, screen still black. New in the boot: the registered threads run and reach seqFunc004 0x205570 and seqFunc029 0x208b98 (stubs in se
- verify: `setup/i3d_draw_module.cpp` 0/0 -- Hook recorded in open-items Decisions (The native i3d hook). The committed screenshot is the forced test (ROSEPAN_I3D_TEST=1): S2DMAN.RPK @0x800 (PSMT8, Now Loa
- verify: `snd/sndman.cpp` 52/52 -- Done as verify: SndMan::update runs every frame now, but every rxSnd call (rx/rxsnd.h, unassigned@00237ce8) is a stub, so nothing is heard and nothing on screen
- verify: `spr/s2d.cpp` 86/86 -- All 86 functions written; nothing on screen can show them yet (MagScene::draw is a stub), so verify: the first session whose screenshot shows S2d sprites marks 
- verify: `spr/sprman.cpp` 53/53 -- Verify: nothing calls SprMan::draw with m_active set yet. S2dMovie / S2dMes / S2dBg start only from MagScene::update 0x1327dc (stub), and textures never load (f
- verify: `spr/sprtexture.cpp` 21/21 -- Pinned 0x18f5b0..0x190a78. The code is complete; the smoke run reaches it (S2dMes / S2dMovie loadTexture) but every list ends in the error state because fileGet
- verify: `system/sysnowloading.cpp` 8/8 -- Code complete, nothing on the boot path shows it yet: draw 0x2051cc is called only by the pfh loader 0x1be564 (GamMan load path, unassigned@001bde58), a stub, s
- verify: `utl/utlmisc.cpp` 24/24 -- Verify: nothing calls this yet (main 0x2045a8 is still the placeholder; the smoke run shows a black frame). When main is reconstructed it calls utlMemInit and i

Recent sessions:

- 2026-10-26 07:18 `sequence/seqcall.cpp` partial: Script native 020 (script math, 62-case jump table data 0x3004a0: sin / cos of degrees and radians, atan2f, + - / *, float-int conversions, sqrtf, Y-rotated off
- 2026-10-26 06:57 `sequence/seqcall.cpp` partial: Script natives 100 (actor motions, 33 cases: setActorMotion at 1/60, chrSetMotion, TskMtnMove / TskMtnMove2 tasks via chrMtnMove / chrMtnMove2, blends by name a
- 2026-10-26 01:59 `sequence/seqcall.cpp` partial: Script natives 253 (movies over the TskMovie task on GamMan m_movieTaskId: start, kill, wait, frame count) and 281 (scene effects: focus lerps, actor eye ops, E
- 2026-10-26 01:46 `sequence/seqcall.cpp` partial: Script natives 016 (camera: trans / rotate / lens / path position / follow actor / mode / push-pop / TskCamera), 023 (area triggers: GamArea addTrg / startTrgs 
- 2026-10-26 01:12 `sequence/seqcall.cpp` partial: Script natives 006 (messages: openMsg, openSysMsg, message mode, message sprite 3, the item-use frame task kind 0x15), 106 (actor motion speed by scene or achr)
- 2026-10-26 01:03 `sequence/seqcall.cpp` partial: Script natives 152 (object queries, 51 cases: actor trans / rot / scale, object flags and work words, system record fields and flags, polar steps, attach / deta
- 2026-10-26 00:43 `sequence/seqcall.cpp` partial: Script natives 007 (message params), 009 (kill message task), 012 (lights and fog, 63 cases), 014 (sky model hide), 019 (pad 0 queries), 028 (releases), 036 (th
- 2026-10-26 00:11 `sequence/seqcall.cpp` partial: Script native 151 (object control, 58 cases, table data 0x300eb0), the first native a New Game press reaches; named and declared its 24 callees (GamMan setObjPo

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `unassigned@001e6cac` | partial | 1/29/220 | 68.8 | 164 | 5 |
| `game/gamman.cpp` | partial | 100/56/409 | 82.8 | 95 | 2 |
| `unassigned@001d859c` | todo | 0/29/75 | 19.9 | 93 | 3 |
| `game/gamsnd.cpp` | todo | 0/15/72 | 23.9 | 84 | 9 |
| `mag/magscenelight.cpp` | todo | 0/6/8 | 2.3 | 51 | 4 |
| `unassigned@001beb70` | todo | 0/13/27 | 6.6 | 44 | 3 |
| `unassigned@00143d3c` | todo | 0/15/602 | 129.5 | 42 | 2 |
| `unassigned@00164fc0` | todo | 0/14/395 | 104.2 | 41 | 4 |
| `unassigned@00184f2c` | todo | 0/5/84 | 10.0 | 37 | 3 |
| `mag/maglifewindow.cpp` | todo | 0/14/29 | 3.7 | 33 | 4 |
| `unassigned@001ab474` | todo | 0/28/119 | 74.0 | 31 | 0 |
| `unassigned@0013e2ec` | partial | 1/11/15 | 4.4 | 30 | 3 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `001e7d7c` | CmnTaskMan::getTask | 46 | ror | stub | - |
| `001be734` | FileReadReq::FileReadReq | 31 | ror | missing | game/gamfile.cpp |
| `001e7724` | CmnTaskMan::killTask | 30 | ror | stub | - |
| `002155f4` | FUN_002155f4 | 22 | - | missing | - |
| `001e71fc` | CmnTaskMan::addTask | 15 | ror | stub | - |
| `0013f980` | MagScene::setLightParam | 14 | ror | stub | mag/magscenelight.cpp |
| `001be2dc` | FileReader::~FileReader | 13 | ror | missing | game/gamfile.cpp |
| `001e4458` | GamSndMan::loadSePack | 12 | ror | stub | game/gamsnd.cpp |
| `001be2cc` | FileReader::FileReader | 11 | ror | missing | game/gamfile.cpp |
| `001dc48c` | GamSysData::getField | 11 | ror | stub | - |
| `0013fb3c` | MagScene::copyLightSet | 9 | ror | stub | mag/magscenelight.cpp |
| `001dd00c` | GamSysData::testFlags | 8 | ror | stub | - |
| `001f184c` | TskFrameFunc::TskFrameFunc | 8 | ror | stub | - |
| `001e46b8` | GamSndMan::freeSePack | 7 | ror | stub | game/gamsnd.cpp |
| `00144334` | gamSetBwnFollow | 7 | ror | stub | - |
| `001db520` | FCHRstat::setParam | 7 | ror | stub | - |
| `00187464` | MsgManSet::close | 7 | ror | stub | - |
| `001d95c4` | ObjStat::setHide | 7 | ror | stub | - |
| `001d9f10` | ObjStat::clearSecta | 6 | ror | stub | - |
| `00115cec` | DogModel::release | 6 | ror | stub | dog/dogmodel.cpp |
| `00143fd0` | MagScene::updateFog | 6 | ror | stub | - |
| `0014062c` | MagScene::rotateTenkyu | 6 | ror | stub | - |
| `00140c68` | MagScene::updateRecords853fc | 6 | ror | stub | - |
| `001832e8` | msgGetSysText | 6 | ror | stub | - |
| `001d5da0` | GamMan::relinkObj | 6 | ror | stub | - |
| `001d1d5c` | GamMan::setObjParam | 6 | ror | stub | game/gamman.cpp |
| `0010ff74` | DogEntity::normalizeRotate | 5 | ror | stub | - |
| `00143d7c` | MagScene::setFog | 5 | ror | stub | - |
| `001cc044` | GamMan::addVFchr | 5 | ror | stub | game/gamman.cpp |
| `0011ade4` | MagLifePC::live | 5 | ror | stub | mag/maglifepc.cpp |
| `001874f4` | MsgManSet::setMessage | 5 | ror | stub | - |
| `001e0c2c` | GamArea::ctl_trg_flg | 5 | ror | stub | - |
| `0021a828` | i3rCopyClipSphere | 5 | lib-ctx | stub | - |
| `001d9830` | ObjStat::load | 4 | ror | stub | - |
| `001bebec` | GamFont::setFont | 4 | ror | stub | - |
| `001dd5e8` | GamArea::addTrg | 4 | ror | stub | - |
| `001cc250` | GamMan::addChr | 4 | ror | stub | game/gamman.cpp |
| `0011034c` | FUN_0011034c | 4 | - | stub | dog/dogentity.cpp |
| `0010fc7c` | DogEntity::setRotate | 4 | ror | stub | - |
| `001875c0` | MsgManSet::getState | 4 | ror | stub | - |

