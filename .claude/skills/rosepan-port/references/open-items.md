# Open items -- priorities, decisions, cross-TU questions, done log

Edit this as work happens: move items to Done with the commit, add questions when something is unclear,
delete what is obsolete. Per-TU state and handoff notes live in `tools/reverse/data/tus.json`
(`tus.py`); `status.md` (generated) has the numbers and the next pick; this file has the judgement.

## Next up (in order; the pinned queue in tus.json encodes it, `tus.py next` follows it)

**PC BLOCKERS FIRST (the user, 2026-10-27, after mag/maglifepc.cpp): the three TUs the player
character needs to animate and move lead the pinned queue, ahead of Brown and the rest:**
  1. mag/maglifepc3.cpp (pinned 0x11d720..0x11e0d8): initMotion 0x11dd40 and the motion loads
     (loadWeaponMotion 0x11dc64, MOTIONPK.RPK), the setMotionSub helpers, getSubStateClass,
     isMoveState.
  2. mag/maglifepcmode.cpp (pinned 0x120788..0x1233d8, 70 functions; several sessions): the modes'
     init / live / pad, now hand-written stubs. WRITTEN 2026-10-27 (verify): every mode's init / live / pad
     (Null .. Hug, Ride::message); the attack / item / Brown helpers they call are named and stubbed
     (MagLifePC::startAttack 0x12c31c, getWeaponHitPos, getHitFrameRange, hitTarget, takeItem, reload,
     MagLifeBwnCtrl::checkHug / requestHug, utlPlaySe 0x1026b0, utlCollectEmyInfo 0x103a90). The 200 s New Game run binds no actor to the PC, so none of it runs
     yet. **Not in any queued TU but needed for the PC to animate:**
     MagLifeControlChara::changeMotionSeq 0x11a320 / changeMotionSeqAlt 0x11a27c / updateMotionSeq
     0x11a14c (mag/maglife.cpp, stubs; the modes start every motion through them).
  3. mag/maglifepcmove.cpp: WRITTEN 2026-10-27 (verify; all 12 functions, pinned 0x11e4c4..0x120788,
     moveBy 0x11ee00 included). Its stubbed callees, needed for the PC to collide and follow the
     floor: MagLifeControlChara::followGround 0x119e1c / updateSectaCells 0x11a6b0 (mag/maglife.cpp),
     MagLifePC::testObjHit 0x11ccd4 / slideOffObj 0x11cd20 (maglifepc2 territory, unassigned@0011cb70;
     both a few lines). Until then the PC walks through objects and keeps its height.
  The small getters in mag/maglifepc2.cpp territory (getEquipItem, getPcParam1b / 1c,
  getWeaponType, canReload; stubs in src/lib/unassigned_stubs.cpp) are not queued; write them
  when a session's run needs real values (calcHealth gives class 2 until then).

**TOP PRIORITY, RE-PINNED (the user, 2026-10-27): the TUs missing to be in game and render in 3D lead the
queue, ahead of everything else (the enemy split, menus, subtitles).** The pinned queue in tus.json is
the stubs the 140 s New Game run reaches on the 3D path, in the order they block a picture:
  a. dog/dogentity.cpp -- WRITTEN 2026-10-27 (verify; pinned 0x10fc7c..0x1103dc + linkonce), and
     dog/dogcamera.cpp -- WRITTEN 2026-10-27 (verify; all 8 functions: getMatrix now returns the
     real view matrix). Open: who starts a camera move (writes DogCamera +0x1d0 / +0x190..+0x1c0)
     is not found yet -- not updateCamera (its mode 6 only calls updateMove): look in the camera
     natives / setCameraMode's mode 6 caller.
     The queue entry unassigned@0010f944 is stale (tus.py next skips it: the run is now these two files).
  b. i3d/i3rmesh.cpp WRITTEN 2026-10-27 (verify; all 14 functions, the layer TEST / ALPHA feed the
     native mesh draw). Was: unassigned@00228c08 -- SPLIT 2026-10-27: i3d/i3rtype.cpp 0x22a6c0..0x22b1b0 (the per-type
     accessors, written, verify) and i3d/i3rmesh.cpp 0x228c08..0x22a6c0 (the mesh packet builder with
     the i3dMatGetScaleTrans bridge; queued right after unassigned@0021fb50, since it only feeds VIF /
     GIF packets: the session that writes i3rBuildMeshPacket decides what of it the native mesh draw
     keeps -- the per-layer packet cache 0x228f50, texture / light / fog choices -- and what becomes
     a PC: comment).
  c. dog/dogmotionblend.cpp (getPose, getNodeRate, stepLoop ...), and the i3m pose player --
     WRITTEN 2026-10-27 as i3d/i3mpose.cpp (partial: everything the game calls is written; left are
     0x230688 / 0x230df8, quaternion spline helpers with no direct callers -- not worth a session
     before the draw path). The rest of the old unassigned@0022e840 run is now i3d/i3cquery.cpp
     (the i3c casts DogModel calls; stubs) and i3d/i3dgeom.cpp (distance tests MagScene calls);
     neither is on the draw path, so neither is queued. Unverified on screen until utlDrawI3r draws.
  d. unassigned@00106658 (utlDrawI3r 0x106760, utlVecDist), then the host task
     **setup/i3d_mesh_draw.cpp** (the native i3d module's 3D mesh path, forced-test first --
     WRITTEN 2026-10-27, verify: ROSEPAN_I3D_MESH_TEST=1 draws Amanda textured, skinned, upright;
     see "Mesh draw" in Open questions for what it leaves), then
     unassigned@0021fb50 -- WRITTEN 2026-10-27 as i3d/i3rdraw.cpp (partial: i3rDrawModel records
     the model and a camera made from the layer's I3dView for the native module; i3rBuildMeshPacket
     is a PC: no-op; the VU1 writers 0x220098..0x222ce0 are not ported. **The picture book's cover
     model now draws in game** ("The Little Princess", upright, matching PB010200_00000800; the 200 s
     run, frames ~3300..4200), and the opened book's page stack; left: the right-hand page and the
     dark MNUPBOOK background, both other paths) and
     unassigned@0021c960 -- WRITTEN 2026-10-27 as i3d/i3dvu1pkt.cpp (partial): the VU1 sprites
     and i3dPktDrawStrip / Color draw through the native module (screen-space strips computed on
     the CPU). **The picture book now matches the reference**: the MNUPBOOK page behind the cover
     model, the arrow / circle prompts (progress/20261027-2000_i3d_i3dvu1pkt_book_cover.png).
     Left: i3dPktDrawStripCtx / LineStrip and their clip helpers, the wire shapes, the
     unreferenced 0x21cfb0..0x21db18. Doubts: a strip transforms with the view the last
     i3dPktCallView named (the ROM uses whatever the layer's CALLed view left in VU1 0x3e2; strips
     have no layer argument); untextured strips draw without depth.
  e. mag/magscenecamera.cpp WRITTEN 2026-10-27 (verify; all 13 functions, pinned 0x13c664..0x13e2ec:
     setCameraMode's six modes, the path camera, save / restoreCamera; the book screen is unchanged
     and no map draws yet to show a path camera). unassigned@0013e2ec WRITTEN 2026-10-27 (verify): updateCamera
     and its two look helpers extend mag/magscenecamera.cpp to 0x13f07c (its assert names that file);
     the menu glue 0x13f07c..0x13f49c is mag/magscenemenu.cpp (gamEnterMenu, canOpen*, the fade
     times, openSysMsgWindow). Next: unassigned@00140c68 + unassigned@00140990 (drawFocus and its records: define them together as
     mag/magscenefocus.cpp).
  f. In game: mag/maglifepc.cpp -- WRITTEN 2026-10-27 (verify; pinned 0x11ab40..0x11c464: the
     state machine, live, processPad, message; setActor now binds the PC after the film). What it
     needs next to move the PC: mag/maglifepc3.cpp (initMotion 0x11dd40, its motion loads),
     mag/maglifepcmode.cpp (the 20 modes' init / live / pad, all stubs -- Null, Wait, Walk2D,
     Run2D first) and mag/maglifepcmove.cpp (applyMotionMove 0x120668, sweepMove 0x11ea04);
     the trivial g_gamMan getters (getEquipItem 0x11c520, getPcParam1b / 1c 0x11c464 / 0x11c4c0)
     are stubs in src/lib/unassigned_stubs.cpp (calcHealth divides 0 by 0 until then: health 2).
     Then task/naga_chunk.cpp (event packs, so scenes
     play), mag/maglifebwn.cpp + unassigned@001258a4 (renderPostProcess) + mag/maglifebwnctrl.cpp
     (Brown), unassigned@00166a1c (the effect objects: Obj176a3c, FUN_00177830 in MagScene::draw).
Re-assess from the run's reached stubs (work/run_relwithdebinfo.log) once the first model draws.
emy/emy_hfm.cpp was mid-session when this was set; it finishes first (an active claim goes before the
queue), then the queue rules.

**TOP PRIORITY (the user, 2026-10-26): get in game -- the first room on screen after New Game.**
**3D first (the user, 2026-10-26):** the 3D path leads the queue -- model load (dog/dogmodel.cpp,
dog/dogactor.cpp), lights, sky / fog, then the draw side: MagScene::draw's reached stubs
(drawSkyDome 0x1406b8, drawFocus 0x140e88, DogCamera::getLensFov) and the VU1 strip writer
i3dPktDrawStrip 0x21db18 into the native i3d draw module, which grows 3D meshes (never the emulated GS).
Supersedes everything below. Test: `tus.py check <TU> --run 200` (`settings.smoke_pad`
1800:START,2400:START,2700:CROSS,3100:START*900 presses New Game, then holds START through the
opening film: TskMovie state 0x67 fades the film out when START is held -- the user, 2026-10-26: skip
cinematics so runs do not rewatch them; the film is cut at about frame 3600 and the run reaches the
post-film stage setup, MagLifePC::initMotion / setActor, about two minutes sooner); the picture decides.
Done so far (2026-10-26): the New Game crash (setup/newgame_crash.cpp: Sequence::addEntry cut a script
address to an int, hazards.md; plus a PORT guard in DogModel::getNodeIndexByName), and TskMovie
(task/tsk_chr.cpp, `partial`): **the opening film plays to its end** ("Once upon a time...", the
ceiling lamps, Jennifer at the window, a "March 1930" card; silent -- the IOP sound stand-in). After
the film nothing draws: no model loads yet. **Priorities, in this order** (the queue in tus.json
follows it; reached stubs in run order):
  1. `task/tsk_chr.cpp` -- TskMsgMan written 2026-10-26 (it opens the film's narration subtitles,
     frames ~2950..; nothing shows until MsgMan / MsgManSet are written -- `msg/msgman.cpp` 0x18330c..
     and unassigned@00184f2c 0x18741c..0x18761c: queue them when the subtitles matter; they are not on
     the path to the first room), TskMtnMove2 / chrMtnMove2 / chrSetMotion written; then the rest of the
     TU's reached / frontier functions (TskFrameFunc 0x1f184c, TskMovePointPath, chrRotStart).
  2. `task/tsk_scene.cpp` (pinned 0x1f6c64..0x1fc4f0 2026-10-26; was unassigned@001f82e4 and
     unassigned@001f6c64) -- PARTIAL 2026-10-26: TskSnd (all kinds), TskCamera, gamSystemDataLoaded /
     gamStage2Loaded written (nothing on screen shows them: sound is the IOP stand-in); second session
     (2026-10-26) wrote scenePlayStart and the whole ScenePlay family 0x1f6c64..0x1f83f4 except the
     unreferenced ScenePlay::loadCamera / loadFocus; a third session (2026-10-26) wrote
     scenePlayStartSimple, PLScenePlay, scnPlaySEDelayed, TskAttack (setup, exec, term, scnAttackStart;
     its six steps live in unassigned@001fd424 and are stubs), scnChrRequest, scnDamageStart and
     TskDamage's exec / term; a fourth (2026-10-26) wrote TskDamage::setup with getReactFrames 0x1fa768,
     kill 0x1fa9c4 and pickAttrBit 0x1faa70 (its nine steps live in unassigned@001fd424 and are stubs).
     A fifth (2026-10-26) wrote the rest: scnChrRequestDog / PC, the damage roll scnCalcDamage
     0x1fb624 / scnCalcPcDamage 0x1fb5d4, TskDamage::endReact 0x1fbb04, scnGetMaxFreeMem 0x1fbba4,
     loadCamera / loadFocus; the TU is verify (0x1fc47c / 0x1fc490 moved to task/naga_chunk.cpp). The
     200 s New Game run reached none of them yet (it ends on the film's "March 1930" card).
     **scenePlayStart parses event packs with `task/naga_chunk.cpp`'s NagaChunk reader, stubbed
     (find returned 0, so a scene loaded nothing and ended at once)**: task/naga_chunk.cpp WRITTEN
     2026-10-27 (verify, pinned 0x1fc47c..0x1fce0c: reader and writer). Still before any scene plays:
     debug/motnamecnv0.cpp (motNameFindChara / GetCategory / Convert, stubbed) and
     DogActor::setBaseRotateY 0x10ed2c / utlStrToLower 0x218338 / MagScene::setFocusData 0x1417e4 (stubs).
  3. **`dog/dogmodel.cpp`** -- WRITTEN 2026-10-26 (all 66 functions, `verify`). The New Game load
     path now runs loadPack -> setRoot -> i3rRelocate, and stops in the i3r library stubs: the item
     tree relocator **i3rItemRelocate 0x22c478 and the type fixups (unassigned@0022b4b8)**,
     i3rBuildPackets 0x21a3e8 / i3rAssignTex 0x21a5b0 / i3rCopyLocalBasicTransform 0x21a700 /
     i3rConvertLocalToModel 0x21a760 (unassigned@0021a3e8), then the draw utlDrawI3r 0x106760 ->
     0x21fe68. **2026-10-26: the relocator is written** -- i3d/i3ritem.cpp (i3rItemRelocate /
     Unrelocate, all 90 type fixup pairs; partial: the per-type accessors are left) and
     i3d/i3croot.cpp (i3cRelocate and the i3c lookups, verify), checked offline on the disc's 1602
     models. **2026-10-26: unassigned@0021a3e8 is written as i3d/i3rmodel.cpp (verify)** -- packets, textures, STQ entities, node transforms;
     the packet builder i3rBuildMeshPacket 0x21fb50 (unassigned@0021fb50) is a stub (DMA packets: the host draw module should read the mesh items instead). Was: Next on the 3D path: unassigned@0021a3e8 (i3rBuildPackets / i3rAssignTex / the node
     transforms), and DogActor::load 0x10cd5c (dog/dogactor.cpp, item 5): two runs that session
     reached the post-film setup but no model load, DogActor::load being a stub on that road. Was: DogModel::loadPack 0x1158ec
     / loadData 0x115790 / setupNodeFlags 0x11550c: the model loader. Without it no map or character exists, so **this is the gate to the
     first 3D picture**. Pinned first in the queue (the user, 2026-10-26: 3D rendering ahead of the
     menu fix below).
  4. **The picture book (mnu/, split 2026-10-26 from unassigned@00192ac0 into mnu/mnubase.cpp,
     mnu/mnumap.cpp, mnu/mnupbook.cpp, mnu/mnustock.cpp).** mnu/mnubase.cpp WRITTEN (verify).
     mnu/mnupbook.cpp: **all of MnuPBook WRITTEN 2026-10-27** (processPad's six steps, page queue,
     sheet turns, pad, text, draw, setDrawSet); **all of MnuDiary WRITTEN 2026-10-27** (`verify`: New
     Game's smoke run never opens a diary; types 0x1000 / 0x1024 / 0x102a).**The gate is open:** the 200 s run opens the book (~frame 3240),
     turns pages (RIGHT held), Circle closes it (~frame 6000), and the game goes on to the narration
     ("Suddenly, the girl was all alone...", TskMsgMan; its window frame shows, the text is blank:
     MsgMan / MsgManSet stubs). `settings.smoke_pad` now ends `4200:RIGHT*1700,6000:CIRCLE,6300:CIRCLE`.
     **Still black while the book is open:** the native i3d draw module needs the VU1 sprite packets
     mnuBase::drawSprite writes (i3dPktBeginVu1Sprite 0x21e5a0, 0x21e488 / i3dPktVu1Matrix 0x21e428 /
     0x21e690, end 0x21e688, unassigned@0021c960: stubs that now hand the packet back -- record a
     textured quad per sprite from the matrix, uv and colour) and drawModel's pose player (i3mGetLength
     0x22e958, i3mGetPose 0x22e960, i3mPoseToMatrices 0x22f7a0, unassigned@0022e840, stubs) and
     utlDrawI3r 0x106760 (stub). GamMan::lookupFlagTable 0x1d83d0 is a stub returning 0, so every book
     opens with the type's words read from global 0 (page state is not the real one). Reference:
     work/ref/MNUPBOOK and work/ref/PB010200 (tm2png --scan of DATA/MNURPK/MNUPBOOK).
  5. **`dog/dogactor.cpp`** -- PARTIAL 2026-10-27: load, loadPackMotion, loadMotion, the record
     table, posture, release paths, root track / getRotY, setMotion / changeMotion, resetShadow
     written (the 200 s run passes them with no DogActor assert and still reaches frame 6840).
     Second session 2026-10-27: draw / drawModel / drawShadow, the posture update 0x10df54,
     runMotionCmd and its helpers, checkName, setEyeTex / updateEye written (no assert in the
     200 s run; still black: utlDrawI3r 0x106760, DogMotionBlend::getPose 0x1081b0 and the i3m
     pose player unassigned@0022e840 are stubs -- **those are now the gate to an actor on screen**,
     with the native i3d draw module). **Third session 2026-10-27: the TU is written (147 of 147,
     `verify`)** -- getMotionMove / getMotionEvent, the root base readers, the blend entry starts
     (setMotionSub*), the node scales and matrices, shareModel, DogActor(int), and the ELF's inline
     accessors out of line. dog/dogmotionblend.cpp written 2026-10-27 (`verify`: getPose, getNodeRate,
     step, stepLoop, the time ratios, setBlendRate). Left on the actor path: the i3m pose player (unassigned@0022e840, incl. i3mGetBlendPose 0x22eab0) and
     utlDrawI3r. The getNodeIndexByName PORT guard stays (GamMan::attachObj asks unloaded actors).
  6. `mag/magscenelight.cpp` -- WRITTEN 2026-10-27 (all 10, `verify`; pinned 0x13f49c..0x13ff00):
     loadLight, setDefaultLight, setShadowLight, the set / param front ends, addLight. The 200 s run
     passes setLightNo / setLightFlags / setLightParam / copyLightSet with no assert; nothing on screen
     can show a light until a model draws (utlDrawI3r 0x106760, the i3m pose player). Open: addLight's
     loop walks nine sets (bit 8 would assert in addToSet) -- kept as the ROM has it.
  7. `mag/magscenetenkyu.cpp` -- VERIFY 2026-10-27 (the sky: load / spin / rotate / drawSkyDome;
     cannot show until utlDrawI3r 0x106760 draws). 2026-10-27: 0x140990..0x141e44 pinned and WRITTEN
     as `mag/magscenefocus.cpp` (verify: the focus / depth-of-field effect and its text track; needs
     the render-target item in Open questions before it can show). 2026-10-27: `unassigned@00143d3c`'s first
     0x12e4 bytes are pinned and WRITTEN as `mag/magscenecmd.cpp` (verify: fog fade, Brown's glue
     and magBwnSeqCommand; nothing on screen shows fog until a model draws, and the
     MagLifeBwnCtrl methods it calls are stubs in mag/maglifebwnctrl.cpp). The rest is now
     `unassigned@00145020` (the enemy classes Hfm..Pig): split it per enemy file (emy/). **EmyManager
     is done 2026-10-27**: emy/emy_manager.cpp (0x16265c..0x164118, all 58, `verify` -- no enemy is
     created in the New Game run yet). Doubts: the file name (no source string; emy_main.cpp style);
     createEmy's transition table is GamMan's m_image[place] as the callers pass it (check addChr's t1
     when EmyMain::setStartState is written); EmyMain keeps the derived word +0x238 on the host
     (setEmyWord238 writes it on any type); setGroup accepts group 8, which the ROM writes over +0x328
     (the host has a ninth group). EmyMain's ctor / dtor (0x164118 / 0x164208, now unassigned@00164118)
     belong to emy/emy_main.cpp with 0x164364.. -- define them together when that file is written.
     **2026-10-27: the enemy split began** -- `emy/emy_hfm.cpp` pinned 0x145020..0x149d18 (HfmMain and
     its outs; PARTIAL: HfmMain whole and HfmOutWalk written, the other eight outs' Init / Exec /
     Term stubbed in the file). Each enemy file opens with its XxxMain::initOut (Bds 0x149d18,
     Bdl 0x14b3bc, Mrt 0x14cde4, Clr 0x14dc7c, Nor 0x150b24, Grg 0x153e40, Fsh 0x156620,
     Got 0x1583d4, Pig 0x15a17c, Dwf 0x15c304?). Doubts: `_tagEnemyTransition` keeps a host
     pointer at +0xc (where the table comes from -- GamMan's image data or the ELF -- is not
     read; a file-loaded table needs a converter); utlRandRangeF 0x1044b8 returns 0.0, not lo,
     when the draw is 0 (kept as the ROM has it).
  8. VERIFY 2026-10-27: `game/gamtrg.cpp` (pinned 0x1dd1e8..0x1e0df0, was unassigned@001dd5e8): all of
     GamArea written, update 0x1dee78 included. The New Game run (140 s) runs update every frame to the end
     without a fault (black stage, the circle icon); no trigger fires that the screen could show yet. Mark done
     when a stage draws and a trg thread starts (walk into a rect). Doubt: the one-shot walk fix (hazards.md).
  9. PARTIAL 2026-10-27: `game/gamman2.cpp` (pinned 0x1d3b44..0x1d84c8, was unassigned@001d3b44):
     the object update list (relinkObj, queueObjUpdate), lookupFlagTable with the real table (the
     picture book now reads row 0x1038; closing it adds item 0xa17, bit 0x29), isSeqPair0Pending, the
     radius / bounds helpers and every secta query written. Left: checkMenuReq (+0x1d4370, 0x1d4540),
     the hit walk 0x1d7c20 with 0x1d4b9c / 0x1d50a8 / testHitAt / placeAchr / 0x1d5610..0x1d5ec4, and
     writeSaveData / readSaveData. **Open: 2 of 6 200 s runs this session died with an access violation
     right after the book closes (~frame 6060; the last first-hit stubs are i3cCastRay and
     i3dPktDrawStrip): one with the full table, one with only the book's lookups forced to 0. The
     other four reached the end (~6780), three of them with the full table.** Intermittent and seen
     with both variants, so likely an older race, not this TU; the two ROSEPAN_CRASH_LOG=1 runs did
     not crash, and gdbrun never boots here (the options.txt loop from the start). Next time it
     happens, rerun with build.py --env ROSEPAN_CRASH_LOG=1 until it faults and addr2line the stack.
  Also reached, lower: EmyManager::resetWork, Obj176a3c::createEff (the effect manager in
  unassigned@00164fc0), chrMtnMove, MagLifePC / MagLifeBwn::reset, i3rCopyClipSphere /
  i3rReplaceStqMapEntity. Re-assess the order from the reached stubs after each of 4..6.
  Risk: one New Game run of two stalled black after the Atlus logo (the boot movie race, item 10e /
  10f, rx/rxthread.cpp): retry a stalled run before judging a TU; if it blocks the checks, the
  rxthread lost wake-up becomes a host task ahead of the list.

**TOP PRIORITY (the user, 2026-10-25): the title menu working and on screen.** This supersedes the
render-first list below (most of it is written, `verify`). How the ROM gets there (traced 2026-10-25):
main -> mainInitGame -> `GamMan::loadSystemData` 0x1c1b48 / `GamMan::loadStage(99)` 0x1c204c (99 is the
default boot stage) -> loadStage reads `data/SEQ/W099/W000.WRD` (a pfh: 19 KB on the disc) and hands it
to the sequence engine (`0x20eda4(0x7a4440, ...)`) -> each frame `GamMan::update` and the sequence VM
(`Sequence::exec` 0x20f98c) run the stage script -> the script's system call 0x16 (handler 0x20694c,
table 0x2d9140) does `MagScene::fadeIn(0, 7, 0)` and `MagScene::requestMenu(0x10005)` -> MagScene::update
-> `openMenu(0x10005)` (written) selects MagLifeMenuTitle (MagScene +0x36640; vtable 0x2e17d8) -> its
slots 9 / 12 / 6 (0x12d2e8 init, 0x12d314 pad, 0x12d32c draw) forward to the title object at +0x40,
code 0x1a91f4 / 0x1a92a0 / 0x1a972c (unassigned@001a6830, the mnu/ menu code: SprMan, SndMan, RxMpeg,
data/mnurpk/mnuboard.rpk). The other menu ids: 0x10000 item, 0x10001 map, 0x10002 stock, 0x10003 save,
0x10004 load, 0x10006 option, 0x10007 movie. Missing today (all stubs or unwritten), queued in run order:
  1. DONE 2026-10-25 (`partial`): `sequence/seqcall.cpp` (pinned 0x2053f4..0x20e3d8) -- the 512 script
     native functions (table data 0x2d90f8) with the system call seqFunc018 0x20694c written (8 / 0x16 =
     title); 36 other handlers are stubs, written when a script reaches them. seqSetLanguage /
     seqGetDataDir / seqMakeGlobalPath are written in sequence/sequence.cpp (0x20e3d8.. now pinned to it).
  2. DONE 2026-10-25 (`verify`): `sequence/sequence.cpp` -- all 19 functions (pools, image loaders, exec / runImage /
     reapThreads); sysInitManagers 0x204148 written ahead in lib/unassigned_stubs.cpp (it calls Sequence::init).
     exec runs every frame with empty pools until GamMan::loadSystemData / loadStage call loadSystemImage /
     loadStageImage (gamman, item 4); the VM entry points (seqVm*, startThread, setupImage) are item 3.
  3. DONE 2026-10-25 (`partial`): `sequence/seqvm.cpp` (pinned 0x20ff34..0x2155a0, was
     unassigned@0020ff34) -- the VM: all 99 opcode handlers, seqVmRun, the image / thread state
     functions, startThread / execThread / waitFrames / jumpTo / killThreads / findThread. Left: the
     other thread starters and queries 0x211830..0x213e78 and 0x214e98..0x215430 (callers are natives
     not written yet). **Tested on the real scripts** with `ROSEPAN_SEQ_TEST=1` (src/rosepan/
     rosepan_seqtest.cpp loads the ELFR image of DATA/SEQ/G000.GRB and of W099/W000.WRD as GamMan
     will): G000's main is an init routine -- system call 0x1e, ~80 x native 110 (registrations, a1 =
     pointers into the image), system call 0x14 with a1 0..0x12 (handlers), 0x15 -- and returns (ra 0
     ends the thread, as on the PS2). W099's main calls 18 (0x1e, 1), 32 (0xa, string), 1, 2, 33 (5),
     27 (4, 1, 0x50000001), 27 (4, 0x201, 0x50800002), 21 (3, 0x50000001), 21 (4, 0x50800002),
     21 (2, 4 / 0x100 / 0x100000) and returns: it does not reach system call 0x16 itself -- the title
     comes later, from what natives 27 / 21 set up (0x5..... ids are image-1 thread ids) or from
     GamMan reacting to onStageSeqEnd. With the stubs returning 0 nothing waits. So after GamMan's
     loads (item 4), the natives the boot scripts reach come next: seqFunc027, 021, 033 (jump tables
     data 0x3005c0 / 0x300ac0), 110, 001, 002, 032, and system calls 0x14 / 0x15 / 0x1e.
  4. PARTIAL 2026-10-25: `game/gamman.cpp` -- the constructor, init, initGameState, loadSystemData,
     loadStage, update, postUpdate, setOption, setVolume / switchVolume, setCameraMode and the small
     ones are written. The smoke run now loads the system pack and W099, runs G000's and W099's
     main threads (natives 110, 032, 001, 002, 033, 027, 021 reached) and reaches
     `GamMan::onStageSeqEnd` 0x1c4168 and `afterSeqExec` 0x1cd25c every frame: **those two (and
     setImage 0x1cbdfc) are the next gamman slice** -- onStageSeqEnd is the likeliest road from the
     W099 script to the title (it calls setOption 0x1c9c1c too). Still stubs on the load path:
     loadStageSnd 0x1c2f50, resetObj 0x1d1410, releaseChr 0x1ccc30; the GamSndMan / GamSysData /
     WrdTable::load callees live in other files.
     Session 2 (2026-10-25, `partial`): onStageSeqEnd, afterSeqExec + execStageReq, setImage,
     resetObj, pause / resume / enterMenu and their helpers, the object lookups (isObj, getAchr /
     getFchr / getVFchr, getActorNo) and ctlObjParam are written; none of it draws, the screen is
     still black. The boot's script still ends without system call 0x16 (no title): the reached
     stubs on that road are the natives seqFunc027 / 021 / 033 / 110 / 001 / 002 / 032 (item 3's
     list, sequence/seqcall.cpp) -- they decide what W099 does next, so they come before more
     gamman code. Still stubs on gamman's load path: loadStageSnd 0x1c2f50 (reached every boot).
     Session 3 (2026-10-25, `partial`): loadStageSnd, the item code (addItem / delItem / equipItem /
     selectItem / hasItem / getItemCount / canAddItem / addStock / delStock, makeOid, loadItemSe),
     releaseChr with releaseFchr / releaseVFchr, getObjParam, releaseTenkyus, ctlObj1cbb80 written;
     still nothing draws. The boot's next blockers are still the natives (seqFunc027 / 021 / 033 /
     110 / 001 / 002 / 032): queue sequence/seqcall.cpp ahead of more gamman. Open: GamItemSlot
     holds a host pointer (m_rec), so a save that copies the slots raw needs a converter.
  4b. PARTIAL 2026-10-25 (session 2 of `sequence/seqcall.cpp`): the boot's natives 001, 002, 021,
     027 (205-case object service, data 0x300700), 032, 033 (fades / volume / cine bars, data
     0x300ac0) and 110 (data 0x300d20) are written. The smoke run gets through all of them; still
     black. Reached next, in boot order: **`Sequence::queueThread` 0x2120ac** (sequence/seqvm.cpp,
     queued first: native 110 case 1 / 30 -- G000's ~80 registrations -- queue threads through it,
     so no registered thread starts until it is written; callEntry / callEntryNow / endThread /
     endThreadOf 0x2129b8.. are its neighbours), then `GamSysData::getField` 0x1dc48c
     (unassigned, after gamman), `GamMan::createObjByModel` / `addChr` / `setMurder` /
     `setLightFlag` (game/gamman.cpp).
  4c. DONE 2026-10-25 (`verify`, sequence/seqvm.cpp complete, 143/143): queueThread and the
     other starters / queries / save data are written. G000's registered threads now start: the
     smoke run newly reaches **seqFunc004** 0x205570 and **seqFunc029** 0x208b98 (natives, write
     next in sequence/seqcall.cpp), then `MagLifeFader::start`, `MagScene::gamEnterMenu`,
     `getMenuFadeOutTime` and `MagLifeMenuItem::isInit / init / processPad`: the **item menu**
     (0x10000) opens, not the title. A trace showed it is not system call 1 (seqFunc018 case 1);
     candidates: MagScene::update mode 1's pad test (pad0 +0x7c bit 3 with BedPad::update a
     stub) or seqFunc018 0x07 / 0x09 with a1 = 0x10000. Find which first -- the title needs the
     right menu. Still black.
  4d. PARTIAL 2026-10-25 (session 3 of `sequence/seqcall.cpp`): seqFunc004 (wait until, 19
     cases) and seqFunc029 (stage changes) written, with GamMan::requestStage 0x1cd044,
     resetTenkyuFlags, isMsgBusy, MagScene::isWindowOpen, DogActor::isModeSet. **Resolved 4c:
     the title IS requested.** A temporary trace in MagScene::requestMenu showed one call,
     id 0x10005 (the title), sub mode 0. The "MagLifeMenuItem::isInit / init / processPad" stub
     hits are not the item menu: every embedded menu on the host is a plain MagLifeMenuBase
     (magscene.h "PORT: host MagLifeMenuBase objects"), whose non-virtual methods carry
     MagLifeMenuItem's addresses, so openMenu(0x10005) runs those stubs. **Next blocker:
     `mag/maglifemenu.cpp`** -- make MagLifeMenuBase's slots 6 / 9..12 virtual and give
     m_menu_title its class MagLifeMenuTitle (vtable 0x2e17d8; slots 6 / 9 / 12 at 0x12d32c /
     0x12d2e8 / 0x12d314 forward to the title object at +0x40), then the title code
     (unassigned@001a6830, item 9). Still black.
  4e. DONE 2026-10-25 (`verify`): `mag/maglifemenu.cpp` -- MagLifeMenuBase is polymorphic on the
     host (slots 9..12 pure, render = MagLife slot 6) and every scene menu has its class
     (magscene.h m_menu_*), each forwarding to its menu object at +0x40 (MnuTitle, MnuItem ...,
     lib/unassigned.h; MnuSvld / MnuSelectLanguage in src/mnu/). The smoke run now opens the
     **title**: MnuTitle::isInit / init / draw / processPad 0x1a9298 / 0x1a91f4 / 0x1a972c /
     0x1a92a0 are the reached stubs. Still black: **next is `unassigned@001a6830`** (queued
     first): MnuTitle's init / processPad / draw and what they call (SprMan, mnuboard.rpk, the
     s100000.str stream); MnuTitle's constructor 0x1a90ec is not written (the object starts zeroed
     in the global scene -- write it as a real constructor when the class gets members, it runs
     before main). MnuTitle::processPad returns -1 (closes the menu) until init has set +0x4d6c.
  5. DONE 2026-10-26 (`verify`): `system/system.cpp` (pinned 0x203d94..0x2043dc, the tail of
     unassigned@001fd424) -- SysTimer (all 8 methods), sysInit / TermManagers, sysExecHostElf; the six
     global managers moved there from lib/unassigned_stubs.cpp. Nothing on screen shows the timer: its
     reader is CmnTaskMan::exec (item 6, a stub). unassigned@001fd424 (now 0x1fd424..0x203d94, the
     TskEnemyMove / TskEnemyTurn enemy tasks) was dropped from the queue: off the title / New Game
     path. Doubt: the file name system/system.cpp is a guess (RoR has no source string for it).
  6. DONE 2026-10-26 (`verify`): `task/tsk_man.cpp` (pinned 0x1e7094..0x1e7f28, cut from
     unassigned@001e6d54) -- CmnTaskMan complete (25 functions: addTask, exec, the kill / pause / find
     calls, the node and live-id helpers) plus TskTest / TskLogger. CmnITask is polymorphic on the host
     with its six slots; classes whose virtuals are not written run the host defaults (exec ends at
     once). getTask now returns int (callers compare with 0). The boot and title menu look as before
     (nothing on screen draws through it yet). Next in the old range: the Tsk* classes 0x1e7f28..
     0x1f8034 (unassigned@001e7f28: TskMtnMove / 2, TskMove*, TskFrameFunc, TskMsgMan, TskMovie ...),
     each class's get_task_type / exec, in the order the New Game road reaches them; 0x1e6d54 /
     0x1e6e00 are GamSndMan 3D-volume helpers (callers playSE / playStr) for game/gamsnd.cpp.
  6b. VERIFY 2026-10-26: `game/gamsnd.cpp` (pinned 0x1e0df0..0x1e7094 with the 3D volume helpers
     0x1e6d54 / 0x1e6e00) -- all of GamSndMan written over two sessions (74 functions): the tables,
     volumes, SE packs, playSE / playStr (both forms), funcSE / funcStr, the BGM push and resume, and
     the 3D sounds (put_* / del_* / update3d / update3dVolume, stopObjSnds). Nothing in the boot reaches
     the 3D code and no sound is audible, so it is `verify` until a run reaches a room with 3D sounds
     (any put_*_3d_pos from the scripts' native 252). Doubts: update3d's object key is a sum of
     differences (ROM behaviour kept); the out-of-range stream fade's TskSndParam m_snd is unset in
     the ROM (the port sets the stream); set_str_3d_snd_prm's no-entry path changes nothing (setStrSnd
     searches for the same missing entry). Earlier doubt: one smoke run in five went black after the
     Shirogumi logo instead of playing MV00_AD; not seen in this session's runs (a 15 s run only reaches
     frame 300, black between Now Loading and the disclaimer -- that is timing, not the bug).
  7. DONE 2026-10-25 (`done`, see 10b): `bed/bedpad.cpp` -- BedPad::init / update: the menu needs pad
     input to "work"; `settings.smoke_pad` now presses through the title.
  8. DONE 2026-10-25 (`verify`, see 4e): `mag/maglifemenu.cpp` -- MagLifeMenuTitle's wrappers and the MagLifeMenuBase methods.
  9. DONE 2026-10-25 (`verify`, now `mnu/mnutitle.cpp` 0x1a90ec..0x1ab474; the rest of
     unassigned@001a6830 -- MnuOpt, MnuBoard2D, MnuMovie, MnuNow, MnuItem -- is still unassigned): `unassigned@001a6830` -- the title screen itself (init / pad / draw at 0x1a91f4.., mnuboard.rpk).
     All 20 MnuTitle functions written, plus utlStrCopy / utlStrCopy2 / utlTm2GetPicture (their stubs
     broke every SprTexture load: hazards.md). The plain boot stays black: execLogo waits on
     **MnuMcd::init / processPad / startCheck / term** (unassigned@001a0950, the memory card check;
     queued first). A run with the logo flag forced to 1 (data 0x2a8400, temporary, not committed)
     showed the main menu right -- titlebg2 and New Game / Load Game / Option at their places
     (progress/20261025-2016_mnu_mnutitle.cpp_menu-testflag.png) -- plus one wrong layer: the whole
     ti_06 text sheet 1:1 at the top left, drawn by no MnuTitle sprite (all 15 dumped). Likely a
     primitive whose TEX0 is written directly (a frame-buffer texture: SprMan::drawFrameCopy /
     SPR_TEX_FRAME, or a scene effect) that rosepan_i3d replays with the last bound texture; find it
     when the title shows for real. After MnuMcd: the logo movies need RxMpeg (item 10; isEnd /
     getStatus must report the end or execLogo waits), then a smoke_pad start press.
  9b. DONE 2026-10-25 (`verify`): `mnu/mnumcd.cpp` (pinned 0x1a1244..0x1a4bd8; MnuTextList's
     0x1a066c..0x1a1244 pinned as `mnu/mnuselect.cpp`) -- all 39 MnuMcd functions (exec, the seven
     check modes, execCmd, updateMessage). Still black: execLogo's check now waits at
     **McdMan::request** 0x187e08 (a stub returning 0: no command is ever taken). Next:
     **`mcd/mcdman.cpp`** (queued first; its extent is not pinned: McdMan 0x187724..~0x18bb88, the
     file string at 0x187a28; ror_functions puts 0x187724..0x187c30 in unassigned@00184f2c) --
     request / getStatus / update and the getInfo / getDir handlers over src/sdk/libmc.cpp, so that
     a PC with no card file answers "no card" or an empty formatted card. Then MsgMan / MsgManSet
     (msg/msgman.cpp 0x18330c.., 0x18741c..) for the texts, or the check runs with empty windows.
  9c. DONE 2026-10-25 (`done`): `mcd/mcdman.cpp` (pinned 0x187724..0x18bb8c) -- all 42 McdMan
     functions, plus sceMcGetSlotMax / Seek / Flush / Unformat / Chdir / Rename / GetEntSpace in
     src/sdk/libmc.cpp. **The plain boot is no longer black**: the card check passes and execLogo
     shows the "Sony Computer Entertainment Inc. Presents" logo, matching its texture
     (work/ref/MNUTITLE/MNUTITLE_000c2000.png). It then stays on it: execLogo step 6 waits for
     MagScene::getFadeState() >= 5, which reads m_fader.m_state, and **MagLifeFader::start 0x12f060**
     is a stub. Next (queued first): **`unassigned@0012f008`** (0x12f008..0x12f2e8, MagLifeFader:
     start / stop and its live; likely mag/maglifefader.cpp, before mag/maglifefadekey.cpp), then
     the Punchline / Shirogumi logos (sprites 5 ..), CP_LOGO.PSS (RxMpeg, item 10). MsgMan is
     still a stub: the check's windows are empty (a PC card with no save answers without prompts).
  9d. DONE 2026-10-25 (`done`): `mag/maglifefader.cpp` (pinned 0x12f008..0x12f298; 0x12f298..
     0x12fca8 pinned as mag/maglifefadekey.cpp) -- MagLifeFader's constructor, start, stop, live,
     render. The logo now fades in and out over 7 frames as it should, then the Atlus disclaimer
     (MNUTITLE_00000800) fades in and out. It needed a native-module fix: SprMan::drawFrameCopy
     (S2dMovie's layers 900 / 1000, every frame) writes TEX0 directly and rosepan_i3d replayed it
     with the last bound texture -- the logo again, over the fader (also the "ti_06 sheet at the top
     left" of item 9). Such primitives are now skipped (hazards.md). Next (queued first):
     **`unassigned@001810f4`** -- RxMpeg::open 0x181150 / play 0x18154c / isEnd 0x18176c: execLogo
     step 8 plays CP_LOGO.PSS and waits for isEnd. A host stand-in that reports the end unblocks the
     Punchline / Shirogumi logos and the title.
  10. DONE 2026-10-25 (`done`): `snd/rxmpeg.cpp` (pinned 0x1810f4..0x1830fc, was
     unassigned@001810f4) and `snd/unitque.cpp` (0x1830fc..0x1832e8) -- the whole movie player and
     library over the host libmpeg (FFmpeg), no stand-in: CP_LOGO.PSS plays through (Punchline,
     then "Published by Atlus USA", letterboxed at y 64 as the ROM's transfer puts it) and closes,
     then MV00_AD.PSS (the 252 MB attract film, ~5 minutes) plays at the pace of its audio -- silent:
     the IOP sound stand-in only drains ADPCM streams now (hazards.md). Renamed from what the code
     does: RxMpeg::isEnd -> isReady, stop -> run, checkSkip -> checkNearEnd (S2dMovie::isMovieEnd /
     stopMovie keep their names but mean "ready" / "run"). Also written: GamMan::getStatus 0x1c6ccc
     and getPlayMinutes 0x1c6c98 (startPaused reads the brightness level through it). **Next
     blocker: `bed/bedpad.cpp` (queued first)** -- execLogo leaves MV00_AD on a Start press (BedPad
     m_7a bit 3) or after its five minutes; BedPad::update is a stub, so the title ("press start",
     then the main menu) waits for the film's end; then `settings.smoke_pad` presses Start.
  10b. DONE 2026-10-25 (`done`): `bed/bedpad.cpp` (pinned 0x101aec..0x102490: BedPad and its stick /
     direction helpers) over a host libpad2 + libvib (sockets backed by libpad's shim, so
     ROSEPAN_AUTOPAD reaches it). **The title menu is on screen through real input**: Start during
     MV00_AD skips it, "Press START button" shows, a second Start opens New Game / Load Game / Option
     (progress/ of this commit; matches the forced-flag picture of item 9 minus its stray text
     sheet). `settings.smoke_pad` = 1800:START,2400:START (pad *reads*, one per BedPad::update on
     port 0) needs a ~75 s run (`tus.py check <TU> --run 75`); a 15 s run ends on the disclaimer.
     Next: **a New Game press** (Cross, m_trig bit 14, with the cursor on New Game: append e.g.
     2700:CROSS to smoke_pad) and queue the stubs it reaches, in order. Seen once, not reproduced:
     a run pressing Start every 120 reads from read 240 showed "Now Loading" from frame 180 on and
     failed to open data/options.txt every frame (rxfile) -- the New Game load path, or a boot that
     raced; if a New Game press ends there, MagScene2's options.txt read (mag/magscene2.cpp) and
     the stage load are the first suspects.
  10c. DONE 2026-10-25 (`verify`): `mnu/mnusvld.cpp` (pinned 0x1a59b8..0x1a90ec; the queue's
     unassigned@001a6830 was its tail) -- all 40 MnuSvld functions, the save / load screen, plus
     GamMan::showMesSprite / setMesSpritePos / fadeIn/OutMesSprite (0x1c5094..0x1c526c, written ahead
     in game/gamman.cpp). A Load Game press (pad 1800:START,2400:START,2700:DOWN,2800:CROSS, ~100 s)
     shows the screen: background, "Load Game" and the list frame match work/ref/mnusvld; the PC card
     has no save, so MnuMcd::checkLoad stops at its "no data" prompt (3100:RIGHT,3200:CROSS quits back
     to the title menu, seen). Not seen yet: the slot list rows (need a save on the card) and every
     window text (**MsgMan / MsgManSet are stubs**: msg/msgman.cpp 0x18330c.. and 0x18741c..). Saving
     needs **GamMan::writeSaveData 0x1d6078 / readSaveData 0x1d68a0** (unassigned@001d3b44, stubs).
     Still next on the main road: the New Game press (2700:CROSS) and what it reaches.
  10d. PARTIAL 2026-10-26 (session 4 of `sequence/seqcall.cpp`): a New Game press
     (smoke pad 1800:START,2400:START,2700:CROSS, ~130 s run) reaches native **151** (object
     control, now written; 24 GamMan / GamSysData callees named and stubbed). Its calls so far
     (oids 0x50000001 / 0x50800002 / 0x62000003 / 0x51800005, ops 1 / 5 / 10 / 40) all stop at
     GamMan::isObj: the objects do not exist because GamMan::createObjByModel / addChr are stubs.
     **New blocker: plain runs crash (access violation) right after the New Game press**, after
     GamMan::setPcAnim, 3 of 3 runs, also with native 151 disabled (so not 151); under gdbrun it does
     NOT crash (timing?), and there it goes on to natives 155, 252, 039, 012, 152, 038, 019 and
     GamMan::initStage2 / startStage2. Find the fault first (a crash handler / minidump in the
     platform layer would help, gdb hides it), then GamMan::createObjByModel 0x1ca8e8 / addChr so
     the stage objects exist, then those natives.
  10e. PARTIAL 2026-10-26 (session 5 of `sequence/seqcall.cpp`): natives 007, 009, 012 (lights / fog,
     63 cases), 014, 019 (pad), 028, 036, 038, 039 written, with GamMan setMsgParam / killMsgTask /
     releasePcMotion / pushFog / popFog / saveLightSets / restoreLightSets / setLightFlag (now (theD,
     theS)) in game/gamman.cpp. Under gdbrun (pad 1800:START,2400:START,2700:CROSS) the title menu shows
     right and New Game runs 012 / 019 / 038 / 039 without stub hits; the screen is then black (stage 2
     load, nothing drawn). **Reached stubs after New Game, in order**: seqFunc155, seqFunc252,
     magBwnSeqCommand 0x144ae0 (native 039's Brown commands, unassigned@00143d3c), seqFunc152, GamArea::
     stopKind / clearKind, GamMan clearPool20b00 / releaseObjs / initStage2 / startStage2 -- write 155,
     252, 152 next in this TU, then GamMan's stage-2 start and createObjByModel / addChr (gamman).
     **Boot blocker (environment / platform, not this TU)**: 5 of 6 plain runs this session stalled
     black after the Atlus logo (MV00_AD never shows a first picture; hazards.md "A crash that gdbrun
     does not reproduce"); the New Game crash of 10d was seen once. A smoke run that must reach New
     Game may need several tries or gdbrun until the movie-start race is found (snd/rxmpeg.cpp /
     sdk/libmpeg.cpp). Decision taken without the user: queue nothing new for it -- the user decides
     whether the movie race goes before more natives.
  10f. PARTIAL 2026-10-26 (session 6): natives 152 (object queries, 51 cases) and 155 (vfchr objects,
     24 cases) written; GamMan hasVFchrParam / isInOidList / isObjRecRef / testObjFlags and utlAddPolar
     written ahead, the rest of their callees declared (stubs). Not seen running: two plain runs (100 /
     170 s, pad 1800:START,2400:START,2700:CROSS) reached the title menu, then froze right after the
     New Game press; a gdbrun stalled at boot. **The boot stall is a lost vsync wake-up in rx/
     rxthread.cpp, not the movie** (hazards.md, "A crash that gdbrun does not reproduce", session 6).
     Doubt recorded for the user: the stall now blocks every visual test past the title; a host task
     for rxThDispatchAlarm / s_yieldSema (platform layer) probably belongs before more natives -- not
     queued without the user. Next natives here: 252 (sound, 201 cases, ~40 unnamed GamSndMan callees:
     a session of its own), then 156, 020, 100, 212.
  10g. DONE 2026-10-26 (session 13 of `sequence/seqcall.cpp`, now `verify`): native 252 (sound, 67
     handlers) written -- every entry of the native table is code now. Its ~25 GamSndMan callees are
     named from GamMan's inline forwarders and stubbed in game/gamsnd.cpp (stopObjSe / releaseObjSe
     renamed del_se_3d_pos_chr / del_str_3d_pos_chr); GamMan's SE pack swaps 0x1c7500..0x1c76cc
     written ahead. The boot smoke run stops at NowLoading, so nothing reaches 252 yet. The sound side
     it drives is game/gamsnd.cpp (funcSE 0x1e5e58 / funcStr 0x1e5660 / playSE / playStr are the core),
     a TU of its own once the stage-2 start runs.
  10h. PARTIAL 2026-10-26 (`game/gamman.cpp` session 4): object creation written -- addChr,
     addVFchr (+ allocVFchr), createObjByModel, createObjByName (+ getChrRpk, setObjLight), setObj,
     setObjParam, setObjPos / getObjPos / getObjRot / getObjRotY / setObjDir / getObjDirTo,
     startAchrPlaceTask, ctlObj1cb8e4 / addObjMarker, openMsg, showMsgSprite, setSysParam;
     ObjStat::ObjStat 0x1d87f4 written ahead in lib/unassigned_stubs.cpp (members ObjStat
     +0x40..+0x1a0, ACHRstat +0x3e4 / +0x3f4). A New Game run (pad 1800:START,2400:START,2700:CROSS,
     --run 140) now creates the PC object and **crashes in MagScene::loadModel**: MagCharaLoader::
     getIndex / load (0x142c88 / 0x142b18) are stubs, load returns NULL, MagScene::loadActor parses
     it (asserts p, thePtr, m_top, then an access violation). **Queued first: `mag/magloader.cpp`**
     (pinned 0x141e44..0x14313c: MagLoaderIndex, MagCharaLoader init / load / getIndex / getId,
     MagMapLoader init / loadByName), then **`unassigned@001d859c`** (ObjStat / ACHRstat / FCHRstat
     init / setPos / getPos / setRot and GamSysData set / getRec / getField 0x1dbef8.. -- every
     object's state and every system-data model id come from there; seqcall's model buffers start at
     0xffff until getField is written). Still in gamman: loadMapUnit 0x1ca0dc with 0x1c9e20 /
     0x1ca38c, loadChapterSe 0x1c2848, 0x1c4610, the stage-2 start (initStage2 / startStage2 /
     resetStage / releaseObjs / clearPool20b00).
  10i. DONE 2026-10-26 (`verify`): `mag/magloader.cpp` -- all 17 functions (MagLoaderIndex,
     MagCharaLoader, MagMapLoader; utlStrEqualNoCase 0x106010 written ahead in
     lib/unassigned_stubs.cpp). The New Game run no longer crashes in MagScene::loadModel: no loader
     or loadActor assert fires, and the PC's model reaches **DogActor::load 0x10cd5c** (a stub) --
     dog/dogactor.cpp is the next blocker for the actor, with `unassigned@001d859c` (ObjStat /
     ACHRstat / GamSysData) next in the queue. A plain run then still dies with an access violation
     right after GamMan::setPcAnim / ACHRstat::setParam (the 10d crash); under gdbrun it does not
     crash and goes on to GamMan::initStage2 / startStage2 (stubs). Nothing past the title menu
     draws yet, so the loader is not visible.
  10j. PARTIAL 2026-10-26: `game/gamstat.cpp` (pinned 0x1d84c8..0x1dd1e8, was unassigned@001d859c;
     0x1dd1e8..0x1dd558 -- a GamMan member constructor, its deleting destructor, GamArea::init -- left
     for game/gamtrg.cpp). Written: ObjStat (ctor, getPos / getRot / setPos / setRot, setSecta,
     syncActorPos / findSecta / updatePos, base getParam / setParam, setHide, releaseActor, save / load
     through an EE-layout ObjStatSave, clearSecta, work words, the ELF's accessors), ACHRstat (init,
     initParamFuncs, getParam / setParam, killTasks, pauseTasks, setMotionSpeed), FCHRstat (init,
     get / setParam, the position / rotation twins), gamWrdConvPos, GamSysData (set, getRec, getField,
     the flag functions). The New Game run (pad 1800:START,2400:START,2700:CROSS, 140 s) now gets
     through getField (0xf, the model ids; not checked by a trace) and runs init / setParam / setHide on the PC
     (50000001), 62000003 and 51800005 -- then still dies with the 10d access violation. Entry traces
     in every gamstat function showed the fault comes after the last ObjStat::setHide returns, outside
     this file. All objects get scene actor 0 (MagSystem::loadModel / DogActor::load are stubs), so
     they share one actor. Left in this TU: GamSysData::setField 0x1dc070 (stub), the helpers
     0x1d84c8..0x1d87f4, 0x1d88fc, 0x1d8adc..0x1d8c50, 0x1d8fec, 0x1d9e9c, 0x1d9f40..0x1da1b4,
     0x1dac8c, 0x1db19c, 0x1db8e8..0x1dbdf0, the hidden 0x1d96e0 (clears +0x110 / +0x114).
     Kept ROM quirk: ACHRstat::setParam's base params fall through into param 0x32 (m_428 takes the
     data's first u_short after ObjStat::setParam; no break in the ROM).
  10k. PARTIAL 2026-10-26 (`game/gamman.cpp` session 5): setMurder, loadChapterSe, loadMapUnit with
     showMapUnits 0x1c9e20 / loadMapPack 0x1ca38c / resetAVFchrMotions 0x1c4610, setPcStartPos with
     setPcPos / setPcRot 0x1cb520 / 0x1cb57c, storePcTransform, onSubSeqEnd, initStage2 with
     reloadVFchrModels 0x1c387c / setupPcDogStage2 0x1cd954 / reloadAchrModel 0x1caf78, setObjRot,
     setKindFlags, moveAchr, resetPcStat, setPcAnim / setPcJnfc, clearPool20b00. The 10d access
     violation after New Game is unchanged: the last commit without this session's code dies the same
     way (last stub GamMan::relinkObj 0x1d5da0), so it is not in this slice. Under gdb a run once
     reached initStage2 and faulted in Sequence::loadSubImage (fixed here, hazards.md). Open: who
     writes GamMan +0x18bc4 (the object bits onSubSeqEnd applies; allocDebug memory, not cleared by
     init), +0x22a5c (the BGM record) and +0x22060 / +0x22070 (the kept PC / dog start transforms).
  10l. PARTIAL 2026-10-26 (`game/gamman.cpp` session 6): switchEnemies, pushCamera / popCamera,
     loadVFchrModel(ByName), changeObjModel, addChrGroup, startStage2, resetStage, releaseObjs,
     releaseAllChrs / releaseAll / term, releaseVFchrs, clearChrSectaPtrs, clearOidListImages,
     openSysMsg (its first argument is a message number, not text: seqcall fixed), testAchrFlag;
     ACHRstat +0x3ec is a word array m_attr[4]. New Game now runs resetStage (GamSndMan::reset,
     GamArea::clear reached) and still dies after GamMan::relinkObj 0x1d5da0 (unassigned@001d3b44,
     the next thing to write or debug with gdbrun.py). Resolved 2026-10-27 (emy/emy_manager.cpp): that stack
     word is createEmy's makeOid, which EmyManager::addEmy 0x162c2c itself passes as 1.
  Unknown until the VM runs: what W099's script does before system call 0x16 (logo movie, language
  select `mnu/mnuSelectLanguage.cpp`, waits on pad). Decode it with the VM when item 2 is written and
  queue what it reaches. The test is the picture: the title menu on screen matching its textures
  (`python tm2png.py --scan data/mnurpk/...` into work/ref/), then a New Game press via smoke_pad.

**TOP PRIORITY (the user, 2026-10-22, live during the rx sound run): get something to render first.**
Queued at the top of tus.json in this order: (1) DONE 2026-10-22: DogLightPool::createLight
0x11262c (now `dog/doglightpool.cpp`, `verify` since the second session: every function written; the
`lightstatus[i] != NULL` asserts are gone from the boot), (2)
`mag/magscene.cpp` -- MagScene::update 0x1327dc: **written 2026-10-22 (`partial`)**, it runs every
frame (sub mode 0, the PC is not bound); MagLifeMan (`mag/maglifeman.cpp`) **written 2026-10-22
(`verify`)**: all 12 functions, MagLife is polymorphic on the host (Decisions), so the scene's S2d
layers now run their live / render each frame -- the screen stays black because their textures
never load (item 3); openMenu, the constructor and the window functions are left in magscene.cpp, (3)
`game/gamfile.cpp` -- fileGetSize 0x1be768 (the .rpk reads 0 bytes, so every SprTexture list
ends in error). The rx sound library (unassigned@00237ce8) is parked `partial` with names only
until these are done.

**Done 2026-10-22 (`verify`): the native i3d draw module** (`setup/i3d_draw_module.cpp`,
`src/rosepan/rendering/rosepan_i3d.*`; hook under Decisions). `ROSEPAN_I3D_TEST=1` draws two S2DMAN.RPK
pictures through the whole i3d path and matches work/ref/S2DMAN; the plain boot is still black because
SprMan::draw (and the scene's layers) write no 2D primitive yet. The first boot screenshot that shows an
i3d draw marks it `done`. Next on the draw path: `spr/sprman.cpp` (SprMan::draw), then the VU1 strip
writers (`i3dPktDrawStrip` 0x21db18, a stub: RosePan_I3dRecordVu1 is their named hook).

0. **TOP PRIORITY (the user, 2026-10-22): the TUs needed to start rendering something**, queued at
   the top of tus.json in this order. Today every frame is black: the frame loop runs, but
   MagScene::draw, the i3d group / layer code and SprMan are stubs, and nothing else draws.
   - **A. First picture, the now-loading screen** (it draws straight through sceGs / sceDma, no
     i3d):
     1. `system/sysnowloading.cpp`: SysNowLoading::init 0x2052ac / term 0x205394 and the helpers
        0x204d38..0x2051cc (0x204e98 / 0x204e68 the packet, 0x205078 the LoadImage upload,
        0x2050e8, the draw 0x2051cc that rxSys's IOP wait loop and the file reader call). Settle the
        boundary (unassigned@00204d38 may belong to it) and get LOADING.BIN onto the screen: the
        first frame that proves the GS path end to end.
        **Written 2026-10-22 (`verify`)**: pinned 0x204d38..0x2053f4, all 8 functions, drawn through
        the new native module `rosepan_draw2d` (no GS). A forced draw proved it
        (progress/20261022-1448_system_sysnowloading.cpp_forced_draw.png: the picture's frames in the
        bottom-right corner, stepping every 8 frames, as LOADING.BIN shows). The boot screenshot is
        still black: draw's only caller is the pfh loader **0x1be564** (GamMan's load path,
        unassigned@001bde58), a stub. **Real since 2026-10-22** (`game/gamfile.cpp`, main's object is
        now the extern `g_nowLoading`): the screen shows once GamMan's loads run. Reconstruct GamMan's loads; it reads main's object, so
        main.cpp's `static SysNowLoading *s_nowLoading` (sdata 0x2d8ffc) becomes an extern global then.
   - **B. The scene's 2D layers through i3d** (the S2dMovie loading.tm2 / S2dMes sprites; s2d.cpp
     is written and waiting):
     2. `mag/magscenerender.cpp`: MagScene::draw 0x13bcc0 (its layers, light sets, view, calls
        S2dMovie::drawMovie / draw and the S2d render slots).
     3. `unassigned@0021fb50`: the i3d groups / layers / sort (i3dLayerInit, i3dGroupInit,
        i3dCtxRunGroup, i3dCtxCloseFrame, i3dGroupAddLayer, the i3dLayerSet* / Begin / EndPacket
        setters, i3dCtxCreateView 0x226c58) -- i3dGroupInit and i3dCtxCloseFrame are reached stubs
        every frame.
        **Written 2026-10-22 (`verify`) as `i3d/i3dlayer.cpp`** (pinned 0x222f50..0x224de0, 80
        functions incl. 21 created in Ghidra): every frame now builds real groups, layers, draw sets
        and a sorted DMA chain (no crash over the smoke run); the screen stays black because nothing
        on the i3d path reaches a host draw module yet. The rest of B.3, i3dCtxCreateView, is in
        `unassigned@00224de0` (queued first; its stub returns NULL, hence the `pViewStatus` asserts).
        The old key `unassigned@0021fb50` now names only the model packet builders 0x21fb50..0x222f50
        (dropped from the queue). Bridges with the ROM's code for the packet module's small helpers
        (i3dPktSetReg / DmaTag / RefTexRegs / SetAlphaMode / OpenChain, i3dDrawSetInit and its list
        helpers, i3dTexSetDescInit, i3dDrawSetUseTex, i3dBatchClearTextures, i3dPsmBits,
        i3dVecCopyW, i3dMatGetUnit) sit in lib/unassigned_stubs.cpp until B.4 replaces them, and the
        stubbed packet writers (i3dPktSprite / Line / DrawStrip* / DrawLineStrip) return pkt.
        **i3dCtxCreateView written 2026-10-22 (`verify`)** as `i3d/i3dview.cpp` (pinned
        0x226278..0x227a98, 19 functions incl. the hidden sphere test 0x226ad8): real views every
        frame, the `pViewStatus` asserts are gone, and i3dPktCallView is bridged (layers CALL their
        view's packet). The camera matrix is a placeholder: `DogEntity::getMatrix` 0x10fa6c is a stub
        that now returns the unit matrix (the ROM runs the entity's transform slot and returns the
        i3dMatInverse 0x237678 it writes at +0xf0, a member of the camera class past DogEntity --
        write it with the camera class). The next asserts each frame: `lightstatus[i] != NULL`
        (magscenerender.cpp:982, DogLightPool::createLight 0x11262c is a stub). Both copies of
        the vector module are real since 2026-10-22 (`i3d/i3dmath.cpp`, `verify`: every i3dVec* /
        i3dMat* bridge and stub is gone).
        `i3d/i3dtim2.cpp` 0x224de0..0x225018 is pinned (todo, not queued).
        **No picture comes from the i3d path until a host draw module consumes it** (Decisions: no
        GS emulation): decide with B.4 / B.7 where `rosepan_i3d` hooks in -- the natural point is
        i3dCtxRunGroup's walk (the sorted layer / set order) or the packet writers themselves.
     4. `unassigned@00216ad0`: i3d packets and texture uploads (i3dPktOpenChain, i3dPktSetReg,
        i3dTexMakeUpload, i3dTexRegsSet ...).
        **Packet writers written 2026-10-22 (`verify`)** as `i3d/i3dpacket.cpp` (pinned
        0x21b220..0x21c960, 54 functions incl. 20 created in Ghidra): every bridge of the packet
        module is gone (tags, SetReg, alpha, OpenChain, draw sets, the call tags) and the sprite /
        line / point / triangle writers and packers are real. Black screen: still no host consumer.
        Left in unassigned@00216ad0 (still queued here): the debug-menu code 0x216ad0..~0x218470
        (DBGFLAG.BIN strings: belongs to a debug/ file, not i3d), the texture objects and GS
        transfers 0x218470..0x21a120 (i3dTexMakeUpload ...), fog 0x21a120..0x21a3e8 (i3dFogInit is
        a reached stub; I3dFogState is in i3dpacket.h until the fog file exists), the i3r helpers,
        the lights (i3dCtxCreateLight 0x21b148), and the VU1 strip uploaders 0x21c960..0x21eb28.
        **The host draw hook** is still undecided (the user's call): the REGLIST / *F writers
        here are the natural place for a 2D host consumer (each one is one GS primitive with its
        PRIM word); the strips need a VU1 replacement or a hook above the packets.
        **Fog and lights written 2026-10-22 (`verify`)** as `i3d/i3denv.cpp` (pinned
        0x21a120..0x21a3e8 + 0x21ac80..0x21b220, 12 functions; one file for both is a guess, the i3r
        helpers sit between). i3dFogInit is no longer a stub; i3dCtxCreateLight is real, so the
        `lightstatus[i] != NULL` asserts now wait only on DogLightPool::createLight 0x11262c
        (dog/doglightpool.cpp). Left in unassigned@00216ad0: the debug menu, the texture objects /
        GS transfers, the i3r helpers.
        **Texture objects written 2026-10-22 (`verify`)** as `i3d/i3dtex.cpp` (pinned
        0x2184c0..0x21a120, 33 functions): i3dTexMakeUpload / ClutMakeUpload / TexMakeMove /
        ClutMakeMove are real (the four stubs and the i3dPsmBits bridge are gone), and the setters
        DogTexInfo calls (i3dTexInit / SetTim2 / SetDesc / SetTcc, i3dClutInit / SetTim2, Term)
        exist. I3dTexObject (i3dlayer.h) became I3dTexture / I3dClut (i3dtex.h);
        I3dVramHandle::m_10 is m_kind. unassigned@00216ad0 now holds only the debug menu and
        utility code 0x216ad0..0x2184c0 (not on the draw path): dropped from the queue.
        **Queued first now: `i3d/i3dtim2.cpp`** (0x224de0..0x225018, five small TIM2 helpers;
        header i3dtim2.h written): its stubs return NULL, and i3dTexSetTim2 / i3dClutSetTim2
        dereference the picture, so write it before DogTexInfo::addData runs.
     5. `spr/sprtexture.cpp`: SprTexture (setList / update; replace the update bridge).
        **Written 2026-10-22 (`verify`)**, pinned 0x18f5b0..0x190a78 (21 functions; the ctor / clear /
        getters before setList are SprTexture's): the update bridge is gone and S2dMes / S2dMovie's
        loads run the real state machine. FileReader's asynchronous helpers 0x1be338..0x1be4c8 and
        FileReader::close / BedPfh::clear are ROM-code bridges (a stubbed isReady would never report
        ready and the loadTexture loops would spin for ever). Every list still ends in the error
        state (update -1, the callers move on): **fileGetSize 0x1be768 is a stub returning 0** (so
        the .rpk reads 0 bytes) and **BedPfh::BedPfh / getFileIndex** (bed/bedpfh.cpp) are stubs.
        Write those three before DogTexInfo (6) can be seen getting textures. **BedPfh written 2026-10-22 (`verify`)** as `bed/bedpfh.cpp` (pinned 0x101738..0x101aec, all 4 methods; the clear bridge is gone; a buffer without "RTPK" leaves the view empty instead of reading garbage tables): fileGetSize 0x1be768 is now the one left. Doubt: a real
        fileGetSize also wakes MagScene::loadActorAnim's pack read through FileReader::read (a stub:
        the 0x800 header stays uninitialised) -- write FileReader::read (0x1be524 / 0x1be564, the
        pfh loader of A.1) in the same session.
     6. `dog/dogtexinfo.cpp`: the texture table SprTexture loads into (DogTexInfo::init / add are
        reached).
        **Written 2026-10-22 (`verify`)**, pinned 0x1161e4..0x11764c (16 functions; architecture.md
        "The texture table"). MagScene::init's init / add now run (no asserts); addData /
        setTexTable wait on the SprTexture loads, which still end in error (fileGetSize 0x1be768,
        BedPfh::BedPfh / getFileIndex are stubs). utlTm2GetSize 0x104e1c is a ROM-code bridge in
        lib/unassigned_stubs.cpp until utl/utltm2.cpp (unassigned@00104970) is written;
        utlTm2ToSepia, i3rGetNumTex / i3rGetTexName are stubs (nothing passes sepia or an i3r yet).
        I3dTexEntry's first two fields are now pszTexName / pPicture (the ROM's names).
     7. `spr/sprman.cpp`: SprMan (init / update / draw, the pattern-table host format).
        **Written 2026-10-22 (`verify`)**, pinned 0x18bb8c..0x18f5b0, all 53 functions (architecture.md
        "The sprite manager"). Still black: S2dMovie / S2dMes / S2dBg only start from MagScene::update
        0x1327dc (a stub), and their textures never load (fileGetSize 0x1be768, BedPfh::BedPfh /
        getFileIndex are stubs). A forced run (MagScene::update stepping m_movie, sprite 1 forced on;
        not committed) showed the loading anim right: loading.tm2's four 256 x 64 bands (v 0 / 64 / 128
        / 192), 6 frames each, looping, at (32, 370) px -- and getTexRecord NULL, so no packet. Next for
        a picture: MagScene::update (or the S2d step calls in it) plus fileGetSize / BedPfh.
   After A, compare the screenshot with LOADING.BIN's picture; after B, with work/ref/S2DMAN.
   Then re-assess (the M1..M5 list below).
1. ~~`setup/ghidra_seed.cpp`~~ (done 2026-10-22: see Done).
2. ~~`setup/platform_bootstrap.cpp`~~ (done 2026-10-22: see Done and "Platform bootstrap").
3. Then **re-assess** with the user (the queue will be empty). Proposed path, the way ReiPan's paid off
   ("reach the game, then 3D"; each milestone named by the picture that proves it):
   - **M1 boot**: `main` 0x2045a8 and what it calls to the first presented frame -- the IOP reboot and
     module loads (served by the host IOP), the rx file layer that reads the disc, the system init.
   - **M2 first screens**: the company logo movie (`CP_LOGO.PSS`), the loading screen
     (`cdrom0:\DATA\CMNTEX\LOADING.BIN`), the language select (`mnu/mnuSelectLanguage.cpp`).
   - **M3 the title menu** (`MagLifeMenuTitle`, `mag/maglifemenu.cpp`) and a New Game.
   - **M4 a room in 3D**: the Dog* object layer (`dog/`), the scene (`mag/magscene*.cpp`), the map
     (`data/bg/mapunit.bin`).
   - **M5 Jennifer and Brown on screen**: `MagLifePC*`, `MagLifeBwn*`.
   Queue the TUs a smoke run reaches, in that order; `settings.smoke_pad` grows with each milestone.
4. (2026-10-22, the rxsys session) `rx/rxsys.cpp` is written (`verify`) but nothing calls it: `main`
   (0x2045a8, in `unassigned@001fd424`) is the next M1 step -- define its file (`main.cpp`? it has no
   source string), replace the placeholder GameMain in `src/main.cpp`, and call `rxSysSetIopImage`,
   `rxSysSetBootPath`, `rxSysInit`, the `rxSysSetMemFunc*` setters. **Then rxfile at once**: on a CDROM
   boot `rxSysLoadIopModules` loops `while (!rxSysReadIopConf(...))` and `while (!rxFileRebootIop(...))`,
   and the rxfile stubs return 0 -- the boot hangs until `rxFileLoad` / `rxFileRebootIop` /
   `rxFileLoadModule` answer (the host has no IOP: `sceSifRebootIop` / `sceSifLoadModule` succeed in
   src/sdk; `rxFileLoad` needs the disc data, see "The disc data").
5. (2026-10-22, the rxfile sessions) `rx/rxfile.cpp` is complete (`verify`): every function in
   0x2437f8..0x249b88 is written -- async slots, default hooks, cdrom ISO 9660 reader, file thread and
   its queues. Nothing runs it yet: `main` (unassigned@001fd424) is still the placeholder, so the next
   M1 step really is `main`. Then **`rx/rxthread.cpp`** (pinned 0x249b88..0x24b490, todo; its rxTh*
   functions are stubs in src/rx/rxthread.cpp today): ~~done 2026-10-22, see Done~~ -- the file thread
   now gets created and dispatched; DIntr / EIntr exist in src/sdk (a recursive mutex).
6. (2026-10-22, decided by the user) **Boot from host0: on the loose disc files** -- see Decisions.
   When `main` is reconstructed, its boot path must make rxSys pick HOST: pass a `host0:` path to
   `rxSysSetBootPath` (e.g. "host0:SLUS_214.48": BootDev "" and BootDir "" -> the data root), not the
   PS2's `cdrom0:\SLUS_214.48;1`; with no boot path `rxSysDetectDevice` picks CDROM because the
   host reports 32 MB. Check on the first run that `host0:` opens of `IOPCONF.CNF`, `SYSTEM.INI`,
   `modules/ioprp300.img` and `DATA/...` resolve through `RosePan_PathResolveData` into the data
   root (lower-case names from the game, upper-case on the disc: fine on Windows, not on a
   case-sensitive host).

7. (2026-10-22, the utlmisc session) `utl/utlmisc.cpp` is written (`verify`): main's `utlMemInit`
   creates the game heap, a `BedMemory`, and `utlMemAlloc` / `utlMemRealloc` / `utlMemFree` are the
   rx memory hooks. **`bed/bedmemory.cpp` (0x100354..) is only stubs** (its alloc returns NULL): when
   `main` is reconstructed and calls `utlMemInit`, reconstruct bedmemory in the same or the next
   session, before anything allocates through the hooks.
   ~~Done 2026-10-22 (`verify`), see Done~~: the heap works (a host test of alloc / free / realloc /
   aligned allocs kept the list consistent and merged back to one block); the first run in which main
   calls `utlMemInit` and allocates proves it on screen.

8. (2026-10-22, the main session) **`main.cpp` is written (`verify`)**: main runs on the host, boots
   from host0: (iopconf.cnf, eight IOP modules, system.ini read from the disc folder), creates the heap,
   runs mainInitGame through the now-loading object and loops at 60 fps; every game call after rxSys
   is a stub, so the screen is black. Next, in the order the run reaches them (`tus.py next` now ranks
   reached stubs, see hazards "The frontier counts..."):
   - **`system/sysnowloading.cpp`** (0x204d38..0x2053f4: init, term, the LOADING.BIN loader, the
     sprite packet, the per-frame draw 0x2051cc) -- small, and it has a reference picture
     (architecture.md "Disc and data files": four "Now Loading" frames). It only shows once
     0x1be564 (the pfh loader inside GamMan's loads) calls its draw, so pair it with GamMan's load path.
   - ~~**`mag/magsystem.cpp`**~~ (2026-10-22, `partial`, then `verify` the same day): the frame runs
     on MagSystem's V-blank semaphore at 30 fps (m_v_interval 2) and the PC `sceGsSyncV(0)` in
     `mainLoop` is gone; all 55 functions written (see item 9).
   - `game/gamman.cpp` (`GamMan::init`, `loadSystemData`, `loadStage`, `update`), the globals file
     (unassigned@001fd424's tail: `sysInitManagers`, `SysTimer`; it owns the global objects now kept in
     lib/unassigned_stubs.cpp), the task manager (unassigned@001e46b8), the sequence engine
     (unassigned@0020eda4, 65 functions: an overlay-style loader of 0x18000..0x30000-byte images and a
     cooperative thread scheduler -- "mst", "trg", "imdy", "single").
   `-m N` on the RosePan command line picks the boot stage (mainParseArgs); without it the ROM's
   default is 99.

9. (2026-10-22, the magsystem sessions) **`mag/magsystem.cpp` is complete (`verify`)**: the second
   session wrote the 12 actor methods (setActorLightID / Parent / Trans / Rotate x2, getActorNodeMatrix
   x2 and ByName x2, getActorMotionMove, addActorTrans, releaseActorMotion) over a minimal Dog layer:
   `src/dog/dogentity.h` / `dogmodel.h` / `dogactor.h` (DogEntity <- DogEntityEx <- DogModel <-
   DogActor, host structs, **not polymorphic yet**: the EE vptr word at +0x94 is kept as padding so
   DogEntity's data keeps its EE offsets; setAttachNode (slot 10) and getNodeWorldMatrix (slot 17) are
   plain members), and `MagScene::m_actors[128]`. The first dog/ TU that needs a virtual call makes
   the four classes polymorphic in slot order (slot lists in the headers' comments) and drops the
   `_ee_vptr` padding and the exact DogEntity offset asserts. Stubs it left: DogEntity::setRotate /
   normalizeRotate (unassigned@0010f944, in lib/unassigned_stubs.cpp: really dog/ code -- define the
   range when a dog TU starts), DogEntityEx::setParent, DogActor::getMotionMove / addTrans /
   releaseMotion / getNodeWorldMatrix, i3rGetNodeIndexByName, dbgTraceVector3.
   The next reached stubs after this session, in run order: `MagScene::MagScene` / `init` / `start` /
   `initRender` / `update` / `draw` (mag/magscene2.cpp, magscenerender.cpp), the i3d library
   (unassigned@00216ad0: `i3dCtxBeginFrame` / `EndFrame` / `GetPacket`, two bridges), `BedPad::init` /
   `update` (bed/bedpad.cpp), `SndMan::update`, `RxMpeg::setFrameParity`, `rxMpegSetVCount`.

10. (2026-10-22, the i3dctx session) **`i3d/i3dctx.cpp` is written (`verify`)**: the i3d context
   (0x21eb28..0x21fb30, 32 functions with 7 hidden ones created in Ghidra); the i3dGetStructSize and
   i3dCtxGetRest bridges are gone (the real rest passes MagSystem's per-frame assert). **Queued next:
   `i3d/i3dvram.cpp`** (pinned 0x227a98..0x228c08, header written, analysis in architecture.md "i3d"):
   its stubbed `i3dVramInit` leaves the heap sentinels' links NULL, so the first frame that draws a
   layer would walk NULL in `i3dCtxMakeVramTransfer`. Write it before the draw-group module
   (0x2239b8..0x224d88: i3dGroupInit, i3dCtxRunGroup, i3dCtxCloseFrame, i3dLayerInit,
   i3dGroupAddLayer, i3dSortList, i3dBatchAppend, i3dTexRegsSet*) and before `MagScene::draw`.

11. (2026-10-22, the i3dvram session) **`i3d/i3dvram.cpp` is written (`verify`)**: all 37 functions
   (29 named i3dVram* in Ghidra, batch i3dvram_001); a host test of init / reserve / evict / compact /
   end frame / reset kept every node accounted for and the sizes summing to the heap. The heaps now
   get real sentinels, so `i3dCtxMakeVramTransfer` can walk them. Nothing on screen shows it yet: the
   next reached stubs in run order are the draw-group module (0x2239b8..0x224d88: `i3dGroupInit`,
   `i3dCtxCloseFrame`, ...) and `MagScene::draw` / `postDraw`; the texture objects at 0x219a98..
   (unassigned@00216ad0) call i3dVramHandleInit / Release.

12. (2026-10-22, the first magscene2 session) **`mag/magscene2.cpp` is `partial`**: 12 of its 31
   functions written (init, initRender, setPC, setBwn, clearPC, postDraw, setActorAnim,
   setCameraTarget, initCamera, getCameraMode, releaseTenkyu / All); the scene map is in
   architecture.md. Left: the constructor 0x1360a0 (0xc74: 128 DogActors, the sky DogModels, the
   twelve MagLifeMenu* objects, the 2D layers, 0x2000 records at +0x437e0..), the destructor
   0x136d14, `load` 0x1377f0 and the rest of the inferred range. The inferred range
   0x1316b8..0x13ff00 also holds mag/magscene.cpp's update / setViewParam / drawView and the light
   and camera files' functions: **pin the extent** (`tus.py define`) before the next slice. Open:
   the classes at scene +0x41850 / +0x41890 / +0x418f0 / +0x837e0 / +0x84aa8 / +0x8542c (named by
   their constructors in lib/unassigned.h) and four FUN_ callees of initRender / releaseTenkyu.
   MagLife* classes are host structs, not polymorphic yet (mag/maglife.h, like dog/dogentity.h).
   Next reached stubs in run order: MagLifeMan::init / add, DogTexInfo::init / add, the S2d inits,
   then `MagScene::draw` / `update` and the i3d draw-group module.

13. (2026-10-22, the second magscene2 session) **`mag/magscene2.cpp` is pinned
   (0x137300..0x139540) and complete (`verify`)**: all 19 functions (loadSystemData, load, the map
   unit loaders, the actor finders, shareActor / loadActor / loadModel / loadModelByName,
   releaseActor, loadActorAnim, loadTexture, releaseTenkyu / All, setActorAnim). The functions the
   first session wrote outside that range moved to the files that own their addresses:
   setPC / setBwn / init / initRender / clearPC and the constructor / destructor stubs to
   `mag/magscene.cpp`, postDraw / setCameraTarget to magscenerender.cpp, initCamera /
   getCameraMode to magscenecamera.cpp, the addLight stub to magscenelight.cpp. **magscene.cpp is
   not pinned yet**: its asserts span 0x131a28..0x135f44; taking 0x1316b8..0x137300 (incl. the
   constructor 0x1360a0 and destructor 0x136d14, no strings) is a guess -- pin it when its TU
   starts. Nothing on screen shows the loaders until `MagScene::start` (magscene.cpp) and
   GamMan's loads run; their stubs now include BedPfh (ctor, getFileName, getFileIndex: tiny,
   bed/bedpfh.cpp), FileReader::read / close (unassigned@001bde58), utlStrCopy / utlStrCopy2
   (utl/utlfile.cpp), the MagCharaLoader / MagMapLoader methods (mag/magloader.cpp) and the
   DogModel / DogActor / DogTexInfo load methods. PC deviations in load: the _c.bin record
   pointers live in a game-heap array (freed by releaseTenkyu), and the clip-sphere reads of the
   second / collision models are skipped when they have no root.

14. (2026-10-22, the s2d sessions) **`spr/s2d.cpp` is pinned (0x190a78..0x192ac0) and complete
   (`verify`)**: all 86 functions (S2dCineSco, S2dBg, S2dMes incl. drawMapName, S2dMovie incl. its
   draw / movie wrappers and four uncalled item setters 0x192978..0x192a70, created in Ghidra this
   session) over a minimal `SprMan` / `SprTexture` (spr/sprman.h, spr/sprtexture.h: all stubs, a
   bridge for SprTexture::update). The callees are stubs: MagScene's windows / sub mode
   (magscene.cpp), RxMpeg (lib/unassigned.h), the i3d layer setters, i3dPktSetReg, i3dCtxCreateView,
   WrdTable::get, DogEntity::getMatrix. MagScene::m_camera_entity (+0x2ab70) is a host DogEntity
   (0xa0) but getMatrix returns +0xf0: whoever writes getMatrix must give the scene the real camera
   class first. Open: the file name (spr/s2d.cpp is a guess: the classes could be one file each); S2dMes's
   EE size (its map-name buffer, at most 0x200); the pattern-table format (EE word images in
   s2d.cpp; SprMan's TU decides the host form and resolves SprDispDef::m_anim_ee). Nothing shows
   until SprMan (spr/sprman.cpp) and MagScene::draw are written: **spr/sprman.cpp +
   spr/sprtexture.cpp are the next step for any 2D picture** (the now-loading screen too uses
   loading.tm2 via S2dMovie).

15. (2026-10-22, the magscenerender sessions) **`mag/magscenerender.cpp` is pinned
   (0x139540..0x13c328) and `verify`**: all 20 functions written (draw, its helpers and
   makeActorShadow; see architecture.md "The scene's frame" and "Projected shadows"). Nothing on
   screen can show it yet: no actor in the boot has the 0x60 shadow flags, and every layer goes into
   stubbed i3d functions. The screen stays black: every layer goes into stubbed i3d functions --
   **the next step for any picture is unchanged: the draw-group / layer module (unassigned@0021fb50,
   queue item B.3)**, then i3dCtxCreateView 0x226c58, i3dCtxCreateLight 0x21b148,
   DogLightPool::createLight (its asserts fire every frame while it returns NULL). PC stand-ins this
   session: the MagScene constructor stub clears the 0x2000 strip records as the ROM's loop at
   0x136ab4 does; drawStrips' path buffer (bss 0x6646f0) holds 64 records (unbounded in the ROM).
   magscene2.cpp's load now copies mapparam word 10 as a float (it is lwc1 / swc1 in the ROM).

- (2026-10-26, i3d/i3rmodel.cpp) The host keeps i3r pointers as 32-bit distances from the field; i3rReplaceStqMapEntity
  and i3rSetMatEntity0..2 point model slots at objects the game owns (MagScene +0x2aa90, DogActor +0x470), so the
  distance must fit in 32 bits: i3rSetFarPtr logs "[i3r] ... too far" when it does not. If that line ever shows,
  the slots need a side table. Also: i3rUnrelocate would turn such a slot into a bogus offset (the ROM does the same).
  **2026-10-27: it shows** -- DogActor::setEyeTex points the eye material's entity 0 at m_eye_uv inside
  the global MagScene actor array (image .bss, 0x7ff6...) while the model sits on the heap. i3rSetFarPtr now
  leaves such a slot NULL and logs once; the eye texture offset is lost until a side table (or the draw
  module reading the actor's record directly) exists. Decide before the renderer reads material entities.
  Also utlRandRange 0x10423c is a stub returning 0, so every eye blinks every 6 frames.

## Decisions

- 2026-10-27 (mag/maglifepc.cpp session, no user present): five small MagLifeControlChara functions
  of mag/maglife.cpp were written there (the constructor 0x11a888, setState 0x119fe0,
  clearMotionSeq, setMotionSeqTable, clearGoal) because MagLifePC cannot hold a state without them;
  the rest of maglife.cpp stays stubs. MagLifePC::reset / command became reset4MapJump / message
  (the base slots' ror-symbol names). The 20 MagLifePCMode classes' virtuals are hand-written stubs
  in mag/maglifepcmode.cpp: stubgen cannot see a missing vtable's slots, so a session that adds a
  polymorphic class must stub its out-of-line virtuals itself (hazards.md).

- 2026-10-27 (session decision, dog/dogentity.cpp; no answer could come): the Dog classes stay
  non-polymorphic on the host and every constructor stores its class's EE vtable address in the +0x94
  word (DOG_VT_* in dogentity.h), which DogEntity::callTransform switches on for slot 2. Real C++
  virtuals would move every field off its EE offset. Open: DogModel::transform / getWorldPos still call
  parent->transform() as DogModel's (dogmodel.cpp); switch them to callTransform when a model hangs
  from a camera or a plain entity. A further slot needed through a base pointer gets its own call*
  dispatcher the same way.

- (2026-10-27, game/gamtrg.cpp) GamAreaTrg +0x54 holds a pointer to a registFtb entry in the ROM; the record stays
  EE layout (the scripts pass it from their image), so the host keeps the entry's node index + 1 there
  (GamArea::ftbOf). The class name GamArea stays (invented before the file was known; gamtrg.cpp suggests GamTrg).

- 2026-10-27 (session decision, dog/dogactor.cpp): written beside the TU because its load path
  runs them: DogMotionBlend's constructor 0x108588, reset 0x10737c and getNo (dog/dogmotionblend.cpp;
  0x108588 / 0x10737c lie in unassigned@00108588 / unassigned@00106658, the class is that file's --
  the next session on those ranges should pin dog/dogmotionblend.cpp to start at 0x10737c if the
  functions between fit), i3rGetNumChild52 0x22aec0 / i3rGetChildName52 0x22af08 (i3d/i3ritem.cpp),
  BedPfh::getUserData (line 81 inline). DogActor now has a constructor (0x10eebc) that ends in clear:
  unloaded actors start with m_mode -1 (isModeSet true) as in the ROM, no longer 0. PORT guard in
  MagScene::setActorAnim for m_pcActor -1 (hazards.md); remove it when MagLifePC::setActor sets the
  PC actor. DogModel::getNodeIndexByName's PORT guard stays: GamMan::attachObj still asks actors with
  no model. utlVecRotY 0x106bc8 is a stub (only looping type-4 root tracks use it).

- 2026-10-26 (session decision, i3d/i3mroot.cpp): **i3m motion files keep their file offsets on
  the host** too (from the root; `i3mPtr(root, field)`), like the i3c files. i3mRelocate /
  i3mUnrelocate keep the ROM's walk with a delta of 0 and leave the relocation u16 at 1. Whoever
  writes DogActor::loadMotion / unpack, MagScene::loadTenkyuTex, 0x1782cc, 0x193a1c and the curve
  code that reads i3m keys (0x22e840.., e.g. 0x22e958 reads the length) must resolve i3m fields
  through i3mPtr, never treat them as pointers. The roles of +0x10, +0x20, +0x28 and the node's +4
  are unknown (the curve code should tell).
- 2026-10-26 (session decision, i3d/i3croot.cpp): **i3c collision files keep their file offsets on
  the host** (from the root; `i3cPtr(root, field)` resolves them), unlike i3r's field-relative
  pointers. i3cRelocate walks the file as the ROM does but only turns -1 into 0 and keeps the
  relocation byte. Why: the ROM's tree relocator tests each value against the root's address (not
  relocated yet), which a host address cannot mimic, and every i3c reader reaches the root (the
  cast query's m_root). The casts (0x2312a0..0x2337e0) must follow this when written.
- 2026-10-26 (session decision, i3d/i3ritem.cpp): **the nine fixup pairs at 0x22a6c0..0x22aec0**
  (types 0x25 0x26 0x29 0x2a 0x2d 0x4a 0x4d 0x52 0x59, inside unassigned@00228c08's Ghidra bodies)
  are written in i3ritem.cpp with the tables, not in their own file; the session that takes
  unassigned@00228c08 must not write them again (move them if that file turns out to own them).
  2026-10-27: the run became i3d/i3rtype.cpp (accessors) + i3d/i3rmesh.cpp; the fixups stay in
  i3ritem.cpp (static, beside their tables), the accessors i3rGetChild25 / i3rGetNumChild52 /
  i3rGetChild52 / i3rGetChildName52 moved to i3rtype.cpp, and i3ritem.h includes i3rtype.h.
  Doubt: the file names i3rtype / i3rmesh are guesses (no source string in the run).

- 2026-10-25 (session decision, sequence/seqvm.cpp): **script memory words keep host pointers beside
  memory.** The VM's loads and stores move 32-bit words (as in the ROM), so a host pointer a script
  stores (sw of a register, setupImage's m_global / m_bits slots) would lose its top half on Win64.
  seqVmStoreWord records the full value of any word that does not fit in 32 bits in a map keyed by
  the word's address; seqVmLoadWord gives it back while the word still holds its low half; the gp
  area copies (thread save / load) carry the entries along. addu / subu wrap at 32 bits when both
  operands are 32-bit values, else add as pointers. Alternative not taken: keep every script-visible
  allocation below 2 GB (the IOP arena's approach) -- natives also return pointers to exe globals.

- 2026-10-25 (session decision, the VM TU may revisit): **script registers are `intptr_t` on the host.**
  The sequence scripts are MIPS code the VM interprets with raw EE addresses (its lw / sw handlers add the
  register to the offset and dereference, e.g. 0x210a08); the native functions get pointers in a1..a3
  (strings, float blocks, word blocks). So the VM keeps its 32 registers as intptr_t holding host
  addresses, and the native table is `intptr_t f(intptr_t a0..a3, t0, t1)` (sequence/seqcall.h). The
  VM must relocate the image's own address constants (lui / addiu pairs, jal targets >= 0x8160 are
  image offsets: base + target - 0x8160) and keep SEQ_RET_* comparisons on the low 32 bits.

- **RXSNDDRV.IRX is a silent host stand-in** (2026-10-22, the rx/rxsnd.cpp session; decided
  unattended, platform layer). `src/sdk/iop_rxsnd.cpp` registers RPC 0xd000 from RosePan_IopBoot and
  answers every call with a zeroed reply (0x11 term done, 0x41 XVAB transfer done, the queued
  status lists empty), except IOP heap alloc 0x20: a bump allocator over fake IOP addresses
  0x100000..0x200000 so rxSndIopAlloc does not fail; XVAB open 0x50 echoes the bank id (the client
  returns the reply as the id); and every SE voice a queued 0x80 / 0x81 starts is listed once with
  status 0 in the next SE status (0x83), so it ends at once and its slot frees (added 2026-10-22,
  second rxsnd session: otherwise all 64 voices stay "playing" and rxSndSePlay runs out). ADPCM
  streams still get free 0 from the zero status, so SndMan never writes to them: a stand-in that
  reports the ring free and the stream ended after rxSndAdpcmEnd is the next step if streaming must
  progress without sound. No sound is produced. Real sound needs a host
  driver that answers play / status commands and mixes voices (SE from XVAB banks, ADPCM streams
  from SndMan's xws files) -- the user's call when sound becomes a priority.
- **MagScene::~MagScene stays a stub** (2026-10-22, the magscene constructor session; decided
  unattended). The ROM never destroys the scene; on the host g_magSystem's static teardown under
  std::exit is the only caller, with the game threads still running and the scene's member lives
  (fader, controllers, windows, 2D layers) still in m_life_man, which MagLifeMan::deleteAll would
  delete as if heap objects. Writing it needs a decision: skip it at exit (a host exit flag), or run
  term() 0x131f74 first. The ROM body is short (checkLightNo(-1), releaseTenkyuAll, the sky
  models' DogModel::release, DogTexInfo::term, MagLifeMan::deleteAll, the loaders' / light pool's
  frees). Update (2026-10-22, magscene session 4): ~MagScene is now written as an empty body with
  the ROM's steps in its comment (callcheck explained), so it is no longer a stub; the decision
  above stands -- add the explicit part only behind a host exit decision. Constructors of members that the host has no C++ constructor for (DogActor 0x10eebc,
  DogModel 0x114c54, the controllers, the fades, the 13 menus) are left to their TUs; the scene's
  constructor does not imitate them.

- **MagLife is polymorphic on the host** (2026-10-22, the maglifeman session; decided unattended,
  required: MagLifeMan runs slots 1 / 2 / 6 / 8 on lives of any class). mag/maglife.h declares
  slots 1 (~MagLife), 2 live, 6 render(I3dLayer *), 7 setHide, 8 getHide as virtuals with the ROM's
  base bodies; the S2d layers' slot 2 is now `live` (was `update`, renamed in Ghidra too) and the
  subclasses' live / render are `override`. Slots 3..5 and 9.. stay plain members on the class
  that defines them until a TU needs them virtual (getName is pure in MagLife0: declaring it makes
  every life abstract). MagLife's EE offsets are comments with >= asserts now (host vptr at 0).
  Lives the host does not know yet (MagLifeFader, MagLife130da4, the menus) run the base's live /
  render. Open: MagLifeMan::deleteAll deletes every life still registered -- the scene's member
  lives (S2d layers, fader) must be removed before the scene's term (0x131f74) reaches it, or
  the host deletes non-heap objects; check when ~MagScene / 0x131f74 are written.

- **The scene's embedded menus are host `MagLifeMenuBase` members** (2026-10-22, magscene session
  4; decided unattended): `m_menu_item` .. `m_menu_sellang` in EE order (classes and offsets in the
  comments of mag/magscene.h), so openMenu can point m_pMenuBase at them. When a menu TU gives its
  class a host type, change that member's type; the 0x1000b / 0x1000c.. menus are `new
  MagLifeMenuSpecial` / `MagLifeMenuSpecial2` (mag/maglifemenusp.h, constructors stubbed).

- **Menus run through the base's slots on the host** (2026-10-22, the magscene session; decided
  in-session): `MagLifeMenuBase` (mag/maglifemenu.h) declares slots 1 / 9..12 (dtor, init, term,
  isInit, processPad) as plain members whose stubs carry MagLifeMenuItem's addresses; the scene
  calls them through `m_pMenuBase`. The first menu TU makes the class polymorphic (as the Dog
  decision below). `MagLifeWindow` likewise: closeWindow deletes through the host destructor.

- **SprMan's table host format** (2026-10-22, the sprman session, decided unattended): the owner's
  pattern / anim / frame / move / key tables stay one writable EE word image (datadump.py --words);
  SprMan::m_pattern is its host copy and the host-only m_pattern_ee its EE base, and SprMan::eePtr
  resolves every EE pointer inside it (setDispList's m_anim_ee, the anims' frames, the moves' keys).
  A future owner whose tables span several .data images needs another resolver -- revisit then.
  The sprite record keeps EE offsets up to +0x134 and holds host pointers from +0x138.

- One TU per session, run by the user (`/rosepan-port next`, the routine, or autopilot.py); sessions run
  the protocol without check-ins and never start other sessions (user's rule, carried from ReiPan).
- Automated runs at effort xhigh or above (user's rule).
- The visual test decides done (user's rule): `verify` for complete code nothing can show yet.
- Work happens on the work branch `ror-port`; `tus.py finish` refuses `main`. The remote is
  `https://github.com/wagrenier/RosePan` (created by the user 2026-10-22).
- **Pushing is approved** (the user, 2026-10-22, as in ReiPan): sessions and the routine push each
  TU's commit with `git push origin HEAD` after `tus.py finish`. Never force-push, rewrite history,
  delete branches or tags; a failed push is reported, not retried with other flags.
- Ghidra names are the C++ names (`Class::method`), kept with `::` in flat names (2026-10-22).
- **The game data is the user's extracted disc, as loose files, and the port boots from `host0:`**
  (the user, 2026-10-22). The folder is `C:/Users/willi/OneDrive/BIG SCHOOL/Documents/RosePan`
  (DATA/, MODULES/, MOVIE/, IOPCONF.CNF, SYSTEM.CNF, SYSTEM.INI and the same ELF; not an .iso), set
  as `settings.data_dir` in tools/reverse/data/tus.json: `rosepan.GAME_DATA` falls back to it when
  `$ROSEPAN_DATA` is unset, and build.py passes it to the smoke run as `ROSEPAN_DATA_DIR`. RoR's
  HOST driver reads files by name through sceOpen; the cdrom driver (which walks ISO 9660 sectors
  itself) stays written for parity but is not on the host's boot path, and libcdvd is not taught to
  build an image.

- **No GS emulation on the draw path** (the user, 2026-10-22): the emulated GS's packet path performs
  badly, so RoR's drawing goes to native host draw modules built as the draw TUs come (SprMan, the
  now-loading screen, fonts, the i3d layers and meshes); textures become GPU textures owned by RoR's
  texture records. Design in architecture.md "PC platform layer". The first draw TU (likely
  `system/sysnowloading.cpp` or `spr/sprman.cpp`) creates `src/rosepan/rendering/rosepan_draw2d.*`;
  the i3d draw-group module (0x2239b8..0x224d88) creates the layer queue. Anything that still has to
  go through the emulated GS is listed here as performance debt.
- **The ROM's global operator new / delete are not the host's** (decided in the utlmisc session,
  2026-10-22, unconfirmed with the user): 0x105b54 / 0x105b7c / 0x105ba4 / 0x105bc4 are written as
  `utlOperatorNew*` / `utlOperatorDelete*` in utl/utlmisc.cpp and do not replace `::operator new`.
  A new-expression in reconstructed code uses the host heap: host objects are bigger than the EE's,
  and the platform layer allocates before main creates the game heap. Revisit if game code relies on
  the game heap's bookkeeping (BedMemory::check, the debug tags).
- **Asserts**: `ROR_ASSERT(expr)` in `src/common/ror_assert.h` prints the ROM handler's line and
  continues (2026-10-22).

- **Dog vtable slots before the classes are polymorphic** (2026-10-22, the magscenerender
  session; decided in-session): MagScene's draw calls ten DogEntity / DogModel / DogActor slots.
  They are plain members (declared on the class that defines the slot, with the slot number in the
  comment) and are called on the object's known class: actors as DogActor, sky models as DogModel,
  the camera as DogCamera, a parent or root entity as DogModel. Making the classes polymorphic
  stays with the first dog/ TU that writes these methods (it then drops the `_ee_vptr` padding and
  the exact DogEntity offset asserts, see item 9).

- **The native i3d hook** (2026-10-22, the i3d draw module session; decided in-session as the task
  text recommended): the i3d library keeps writing its GS packets; each 2D primitive writer
  (i3dpacket.cpp, REGLIST and PACKED *F families, and i3dPktClearZ) passes its freshly written GIF tag
  and data to `RosePan_I3dRecordGif`, which turns them into host vertices keyed by the packet address;
  i3dPktSetReg records ALPHA_1 / TEST_1 / SCISSOR_1 / XYOFFSET_1, i3dPktRefTexRegs the draw set's TEX
  packet, i3dPktCallView the view's SCISSOR_1. The walk that links packets into the DMA chain
  (i3dCtxRunGroup's packet nodes, i3dNodeStatePacket / ChanPacket, i3dCtxCloseFrame's opening tags)
  calls `RosePan_I3dReplay(start, end)`, so the host draws in the chain's order. i3dTexRegsSet binds a
  TEX packet to its I3dTexture / I3dClut / palette; the first draw decodes it (rosepan_draw2d
  `RosePan_Draw2dMakeTexture`, PSMT4 / T8 / CT32 / 24 / 16, CSM1 / CSM2), cached per object and dropped
  by i3dTexRelease / i3dClutRelease. Not yet: VU1 strips (named hook `RosePan_I3dRecordVu1`), CALLed
  channel state packets (VU1 / fog state), XYOFFSET 0 passes (MagSceneRender's offscreen stages:
  skipped and logged), ALPHA modes the renderer cannot express (mode 2, Cs opaque, draws as an alpha
  blend: no opaque blend mode in the renderer yet), TEXA (CT16 alpha bit -> 0x80 / 0, CT24 opaque).

## Platform bootstrap (FF3 pieces dropped or still to adapt, 2026-10-22)

Dropped: ReiPan's warp / snapshot window (`debug/reipan_warp.*`), the FF3 3D glue
(`rendering/reipan_graph3d.*`), the IMG_BD.BIN / CMP file-load service (`io/reipan_fileload.*`), the
Debug menu's FF3 light cone, the IOP boot's call into FF3's iopsys.irx `start()`. Still FF3-shaped,
to adapt when a TU reaches them:
- `sdk/libcdvd.cpp` reads sectors out of `IMG_BD.BIN` (sceCdRead by LSN); RoR reads loose files
  (`cdrom0:\DATA\...`): sceCdSearchFile / sceCdRead must map onto the data root's files.
- `io/rosepan_moviestream.cpp` falls back to an IMG_BD.BIN span; RoR's PSS movies are loose files
  (`mov/*.PSS`, `CP_LOGO.PSS`), so only its named-file path applies.
- `io/rosepan_paths.cpp` `RosePan_PathResolveData` tries FF3's "../bin/data" / "bin2" spellings.
- `sdk/iop_libsd.cpp`, `iop_voice.cpp`, `iop_reverb.cpp` serve FF3's own IOP sound RPCs; RoR's sound
  (rx middleware, `snd/`) may talk to Sony's libsd RPC directly. Decide when `snd/` is reached.
- The renderer's 3D path (`RosePan_Renderer*Mesh*`, VU1 lighting shaders) is shaped for FF2/FF3's
  SGD models; RoR's model format is its own (M4).
- `RosePan_DebugLightCone*` and other FF3 debug flags in `rosepan_debug.h` remain, unused.
- The data folder: the user's extracted disc (Decisions, 2026-10-22), from `settings.data_dir`
  or `$ROSEPAN_DATA`; build.py passes it as `ROSEPAN_DATA_DIR`.

## Open questions

- **Render targets in the native i3d module** (2026-10-27, mag/magscenefocus.cpp): the focus effect
  (efffocus, FRAME 0x138), the actor shadows / close-up (shadow01, FRAME 0x118) and any other
  FRAME_1-retargeted layer draw into a VRAM work buffer and read it back as a texture with no
  main-memory image, and the focus shrink samples the frame buffer itself (a direct TEX0; the module
  logs "textured primitive with a direct TEX0 (frame buffer) not drawn yet"). The module records
  neither, so these effects draw nothing on PC. A host task should map FRAME_1 fbp to off-screen
  render targets and VRAM-only textures (I3dTexture block, no m_data) to them, plus a frame-buffer copy
  for the shrink. A platform-layer change: the user's call when to queue it.

- **Path camera ray** (2026-10-27, mag/magscenecamera.cpp): scoreCameraPath 0x13d8f8 with flags bit 1
  calls castRay(hit, camera point, target, -1) -- the target position itself as the ray's dir, and
  counts a hit at t in 0..1. Kept as the ROM has it. updateCamera is written now: all its three
  calls and seekCameraPath's pass flags 0 (asm checked), so no caller reaches the ray -- dead code
  in the retail build, the question is moot unless another caller turns up.

- **Path camera look** (2026-10-27, mag/magscenecamera.cpp): clampCameraLook 0x13e2ec (updateCamera
  modes 4 / 5, a path record with a look letter at +0x1f) normalises the target's world position
  itself, not target - camera, before utlVecToRotX / Y (asm checked: a straight copy of the vector).
  Kept as the ROM has it; the look is then the direction of the actor from the world origin,
  clamped to +-limit around the record's m_look_rot. When a map with such a record draws, compare
  the camera with the PS2 -- if it looks wrong only there, the ROM may mean target - camera.

- **mag/magscenemenu.cpp is the port's name** (2026-10-27): 0x13f07c..0x13f49c has no source-path
  string (no asserts). Its neighbours are magscenecamera.cpp (updateCamera, asserted) and
  magscenelight.cpp (checkLightNo 0x13f49c); the run is MagScene's GamMan menu glue and the system message window, so it may really be the tail of magscenecamera.cpp or
  a file of its own. Rename if a later finding says otherwise.

- **Mesh draw** (2026-10-27, rosepan_i3dmesh.cpp, setup/i3d_mesh_draw.cpp verify). i3d/i3rdraw.cpp
  (2026-10-27) did (1) and (2): i3rDrawModel records at a 2-qword packet of the layer, the camera
  is m_camera plus m_rect_proj remade as reverse Z with y negated (the rect viewport's y diagonal is
  positive: NDC y runs down); the book cover shows upright and unmirrored, so the y / handedness
  question is settled for this view. Still open: (3)..(5) below, the blend weights (opt /
  i3rDrawSetupBlend 0x220670: type-0x59 children, the module reads none), and the per-part bone-box
  cull (i3rDrawMesh46 0x220290: i3dViewTestBoneBox; the host counts a part with data as drawn).
  Was: left for the session that writes i3rDrawModel 0x21fe68 (unassigned@0021fb50): (1) call
  RosePan_I3dRecordModel from it at the layer's packet with its world / node arguments and the
  context's texture table; (2) a camera record from the layer's I3dView (i3dPktCallView: m_camera
  is the row-vector world -> camera, the projection must become the renderer's reverse-Z form;
  RosePan_I3dRecordCamera takes the pair), and the host projection's y / handedness must be
  rechecked against the game's own view (the forced test builds its own: up = -y); (3) lights and
  fog: the colours are the strip colours x one fixed light (Shade), not mag/magscenelight.cpp's
  lights nor the fog (magscenecmd.cpp); (4) the 0x45 part of a combined mesh (FUN_00220098) is not
  drawn; (5) alpha: DONE 2026-10-27 (i3d/i3rmesh.cpp): the host sets each material's TEST /
  ALPHA from its first layer through i3rPktLayerTest / i3rPktLayerAlpha (chain defaults 0x5000d /
  0x44 where the layer writes none; the ROM would keep the previous layer's value instead). Every
  amd.mdl layer is blend 0x10 (= 0x44) with no alpha test, so Amanda's picture is unchanged; a
  model whose layers set m_atst / m_blend will show it. Only layer 0 is read (materials seen so far
  have one). (6) i3d/i3rmesh.cpp is written as the ROM does (the material packet cache and VIF
  chains, verify: its callers are the unported VU1 writers). For (3): amd.mdl's layers are
  m_shade 1 (lit) with m_ambient 1, so the scene light set (I3dLayer m_light, i3rPktLayerLights'
  five lists) is what the ROM shades them with. Doubt: the strip colour block (0x17) is read as RGB
  vertex colour; values are 0.98..1.0 on amd.mdl, which also fits a lighting factor.

- utl/utlmath.cpp (2026-10-27) is the port's name for 0x106658..0x10737c: no source string names
  the run; most of it is vector math, but utlConvSndId and the i3r node display wrappers sit at its
  start and could be a separate object. utlVecRotY is now the copy form 0x1069b0 and the in-place
  matrix form 0x106bc8 is utlVecRotMatY (DogActor::getRootMove calls the latter). The node display
  word (utlSetI3rNodeDisp / utlSetI3rDispControl) holds a pointer to a DogModel / DogActor record;
  on the host it is self-relative (i3rSetPtr), so i3rDrawModel 0x21fe68 must read it with
  getData() when it is written.

- mag/magscenecmd.cpp (2026-10-27) is the port's name for 0x143d3c..0x145020: no source string
  names the run (fog fade, the debug window ctor, Brown's glue). magsceneif.cpp's asserts end right
  before it but its 13 functions serve debug/motnamecnv only; rename if a better anchor turns up.
  The tiny wrappers magBwnReturn0 / 0b / 0c and magBwnNop are named by position, not role.

- MnuSprite / VU1 sprite names (2026-10-27, mnu/mnupbook.cpp): the picture book's sprite records hold
  (0, 0, 1, 0.875) at +0x10 (a 512 x 448 page on a 512 x 512 texture) and (1, 1, 1, 1) at +0x20, so
  +0x10 is the uv rectangle and +0x20 the colour (MnuSprite fixed). drawSprite hands +0x10 to
  0x21e488 (named i3dPktVu1Color) and +0x20 to i3dPktVu1Sprite 0x21e690's third argument (named
  uv): those names look swapped. Settle it when the native VU1 sprite path is written (read the VU1
  sprite microprogram: which of VU addresses 0x3f1 / the per-sprite vector it uses for ST).

- CHARA.RPK + 0x5368980 (2026-10-26, i3d/i3ritem.cpp offline check): the one model of 1602 whose
  item names do not land on strings (its tree and data offsets are fine). Probably a pack entry that
  is not contiguous in the RPK (the next model starts 0x18d80 later); check when RPK entries are
  parsed (DogModel::loadPack's pack reader) rather than scanned.
- I3D_I3C data (2026-10-26): no raw "I3D_I3C" exists in the disc's RPK / ARA files, so
  i3cRelocate could not be checked offline. Find which loader hands DogModel::loadData its i3c
  (pack file 2 of a model pack? a decompressed area?) and check the layout in i3croot.h on it.

- TskDamage chr 3 / 4 (2026-10-26, task/tsk_scene.cpp): TskDamage::setup sets no step for chr words
  3 and 4, and the ROM constructor does not clear m_step, so the ROM's exec calls whatever the heap
  left there. scnChrRequest sends chr 3 / 4 to scnChrRequestDog / PC instead, so setup should never
  see them; the PC keeps m_step NULL and ends the task. If a smoke run ever shows a TskDamage with chr
  3 / 4, find who starts it.
- scenePlayStartSimple (2026-10-26, task/tsk_scene.cpp): it reads no "frame" chunk and never calls
  ScenePlay::setFrames, so the ROM's ScenePlay runs on whatever word the heap left at m_frame (the
  ROM constructor does not write it). The PC constructor zeroes it, so the task ends on its first exec
  and releases the pack's effect at once. When seqcall's native 0x18 is reached, check whether the
  effect must outlive the task (then give the simple ScenePlay a large frame count or keep the
  effect alive) -- the screenshot decides.
- scnChrRequestDog (2026-10-26, task/tsk_scene.cpp): Brown's reaction is magBwnHurt(1) for damage of
  26..100+ percent of his max LP and magBwnHurt(0) below 26 percent -- read off the ROM as written
  (react 2 for share < 26, inu mode 1 for 26..50, 0 above). If Brown's hurt reaction looks inverted
  in play, recheck 0x1fae14's compares (slti 0x33 / 0x1a) before changing it.
- scnChrRequest (2026-10-26): the chr 0..2 / 5..10 path reads object chrOid's position into a stack
  vector and never uses it; TskDamage::setup (0x1f9e58) gets a pointer to the words {a2, word, chr}
  right after it -- check whether setup reads below the pointer (the position) before trusting
  ScnDamageParam's three words.

- TskMoveSectaFindPath (2026-10-26, task/tsk_chr.cpp): only aim (0x1ed1d0) writes a step pointer,
  and only stepTurn; the constructor overwrites it after its first aim, but an aim from exec or
  stepSlideHit that plays a turn motion leaves stepTurn for good, and stepTurn only re-aims -- so once a
  leg turns by motion the achr stops walking. Kept as the ROM does. The ROM also never writes m_target
  (+0x240, which stepMotion aims at) nor m_flags (+0x2a8): the port aims stepMotion at the current
  cell's point and zeroes the flags. Check both the first time a script walks an achr through native
  211 cases 30 / 40..51 on screen.
- TskMoveTypeRound (2026-10-26, task/tsk_chr.cpp): stepPoint (modes 1 / 3) computes its off-ring push
  with the sign opposite to stepObj / stepObjMotion (inward when inside the ring). Mode 3 subtracts it
  (so it pushes outward, as the others do); mode 1 adds it, so a mode-1 achr inside the ring is pulled
  further in. Kept as the ROM does. The ROM also leaves m_flags (+0x18) and m_lastPos (+0x70) unset:
  the port zeroes the flags and starts m_lastPos at the actor's position. Check both the first time a
  script circles an achr on screen (native 211 cases 10 / 11).
- TskMovePointPath / TskMoveTypeRound (same session): a mode the constructor's switch does not know
  (TskMovePointPath >= 8, TskMoveTypeRound >= 4; from script global 15's shift) sets no step, so the ROM
  would call garbage. The port uses stepOnce / stepPoint. GamMan::placeAchr now returns int (the ROM
  returns the hit flag; it is still a stub that returns 0).

- TskSysFunc / TskSeqEvent / TskAttach (2026-10-26, task/tsk_chr.cpp): three ROM garbage reads made
  defined -- TskSysFunc kinds outside 0..1 leave the step unset (the port ends the task); TskSeqEvent's
  collision record leaves m_colPrm words 2, 3, 5..11 as stack garbage (the port keeps the achr's
  values); TskAttach passes an untouched stack matrix when the node is missing (the port zeroes it).
  TskAttach's third argument is the node name (seqcall 0x3c a3), no longer an int.

- TskChrRot::stepFrames (2026-10-26, task/tsk_chr.cpp): each frame the ROM writes the heading into word
  +0x6c of whatever task runs in ACHRstat::m_task[2]. That is TskMtnMove's m_rotY, but TskMtnMove2's
  m_step (its speed / (1/60)). The port keeps the ROM's write for both classes (through dynamic_cast).
  Unconfirmed: a turn without a motion that runs while native 100's TskMtnMove2 moves the achr may change
  that move's speed. Check it the first time a screenshot shows a turn while the achr is walking.
- TskChrRot::stepMotion (same session): with m_restore set it starts motion 1 through chrMtnMove2, then
  kills m_task[2] at once. That is the task it just started, so the motion change stays but the moving
  stops. The port does the same. If idle motions look frozen after a turn, look here first.
- TskAvoidMove / TskMovePhysic / TskGenFrameFunc (2026-10-26, task/tsk_chr.cpp): ROM garbage made defined.
  TskAvoidMove leaves m_achr unset for an oid that is not an achr, then ORs flag 0x4000 through it; the
  port starts it at NULL and skips the write. With no hit record and no point it sets no step; the port
  ends the task. TskMovePhysic counts m_count, m_flags and m_speed on from the heap words, and for
  anything but an fchr reads an unset actor and sets no step; the port zeroes them and ends non-fchrs at
  once. TskGenFrameFunc kinds other than 1 set no step; the port ends them. chrIsNear leaves
  UtlHitCheck's fifth word (m_10) to the stack (0 here: the sphere test). Unconfirmed: aimAway and
  stepAway start the push one eighth turn past the cell they found and keep m_dir at the first
  heading. The port keeps this. If a pushed achr slides the wrong way, check here first.

- task/tsk_man.cpp (2026-10-26): the file name is a guess (no source string; RoR's task/ files are
  tsk_scene, tsk_bg_load, naga_chunk). Rename freely if a string turns up. Also unknown: CmnTaskMan +0x0c..
  +0x40, +0x6c..+0x84, +0x688..+0x6c0 (no code in the file touches them), the list at +4 (findTask reads
  it, nothing adds to it), node flag 0x10 (all callers pass 0x11).

- MagSceneRayHit size (2026-10-26, sequence/seqcall.cpp session 9): native 156 copies castRay's hit
  record out as 0x70 bytes (3 x 0x20 + 0x10 struct-copy code) while magscene.h asserts 0x60 (from
  drawBlobShadows' clear). The extra 0x10 is never initialised in the ROM's frame, so the record is
  likely 0x70 with an unused tail; seqcall.cpp wraps it in SeqRayHit (0x70) for now. Settle it when
  MagScene::castRay's model code (DogModel::castRay 0x11440c) is written.
- MagLifeFadeKey vs MagLifeFadeKeyHole (2026-10-25, found in the mag/maglifefader.cpp session): the
  scene's +0x41890 object is built by 0x12f298, which stores `_vt$18MagLifeFadeKeyHole` 0x2e1cf8
  (rtti.py: MagLifeFadeKeyHole : MagLife, slots 2 0x12f84c, 6 0x12f95c). The code names its
  methods `MagLifeFadeKey::*` (ror-inferred, from the file name). Unless the file holds a second
  class, the session that writes mag/maglifefadekey.cpp should rename the class (and its methods) to
  MagLifeFadeKeyHole, the ELF's own name. Not changed yet.
- Host frame-buffer copies (2026-10-25): rosepan_i3d now skips textured primitives with a direct
  TEX0 (SprMan::drawFrameCopy, SPR_TEX_FRAME sprites). drawFrameCopy puts the other frame buffer
  over the screen at alpha 0x80 every frame on S2dMovie's layer 1 -- on the PS2 a one-frame-late
  copy, presumably for the movie / blur effects. If a screen later looks wrong where the ROM shows a
  copy of the previous frame, the module needs to keep the last presented frame as a texture.

- McdMan (mcd/mcdman.cpp, 2026-10-25): (1) cmdCreateSave skips the main file when it is already
  there (sub 200) and then writes any missing dtNN file from m_data, which only the main-file step
  allocates: the ROM writes from NULL; the host sceMcWrite answers sceMcResNoEntry for a NULL buffer,
  so such a save fails with NotOpen instead of crashing. Kept as the ROM; revisit if a real save hits
  it. (2) hasSpace compares free KB (+0x84) with 0xed minus the byte sizes of the listed entries --
  units look mixed; written as the ROM. (3) The host libmc has no current directory: sceMcChdir only
  checks the directory and answers "/" (nothing calls McdMan's chdir command yet).

- RESOLVED (2026-10-25, gamman session 2): **bss 0x7b2060 is `g_sequence.m_bits`** (0x7a4440 +
  0xdc20, stored by Sequence::init through the struct base, which a scan for offset 0x2060 cannot
  see) and 0x7b205c is `m_global`. `g_bss_7b2060` and its zeroed stand-in are gone; code reads
  `g_sequence.m_bits[n]` / `m_global[n]`. MagScene's glue (magscene.h "GamMan glue") still says
  *(0x7b2060): read it as the sequence bitset when that code is written.
- (2026-10-25) **GamMan methods past 0x1d3b44**: `GamMan::placeAchr` 0x1d544c and `relinkObj` 0x1d5da0
  are called with GamMan in a0 (resume, resetObj, ObjStat::setHide) but sit in unassigned@001d3b44
  (ObjStat's run). Either gamman.cpp's pinned end (switchVolume 0x1d3ac8) is early or a second file
  (an object / secta file) holds GamMan methods. Decide when unassigned@001d3b44 is defined.
- (2026-10-25) `bed/bedpfh.h` inline line numbers: GamMan::loadSystemData / loadStage assert line
  71 (0x47) in the inline whose result is m_top itself (utlMemRealloc's pointer), and line 72 in
  getTotalSize. The header has getNumFile at 71 and getTop at 73; either the line-71 inline is a
  second m_top getter or the two are swapped. Only the comments are affected.
- (2026-10-25) GamMan's option words: setOption 0x1c9c1c's cases are named from what they touch
  (0 a brightness word bss 0x780d04, 1 the scene life +0x418f0's volume, 2 / 3 funcSnd 0xf / 0xe,
  6 rxSndSetOutputMode, 7 MagSystem::m_vib_on). architecture.md's "GamMan option 0x12"
  (utlIsVibrationOn) is another numbering; reconcile when the options menu is written.

- (2026-10-25) `sequence/seqvm.cpp`: the file name is invented (no source string, no assert); it holds
  Sequence's remaining methods, so it may be part of sequence.cpp. 0x2155a0..0x215e1c after it (debugNop,
  the assert handler 0x215a30, dbgTraceVector3 / 4, the fatal print 0x2155f4) is debug code of another
  file, left unassigned (key unassigned@0020ff34 now names only that run).
- (2026-10-25) The VM keeps ROM quirks: `beq zero, rt` always branches, sltiu's immediate is
  zero-extended, mult is unsigned, jal keeps 16 bits of target, killThreads(0) copies the image-0
  list's sentinel to the ended list (its loop has no body), seqVmResume loads a0..a3 from the slot's
  a2..t1. Unknown: who sets g_seqReturnPc (bss 0x7b4db0, SEQ_RET_RETURN continues there); what
  header +0x20 / +0x24 and g_seqData2 are for.
- (2026-10-25) seqFunc032 (0x208dc4) tests its string pointer `< 0x8000000` (EE RAM): on the host
  that test fails for every heap pointer -- drop it (PC comment) when the function is written.

- (2026-10-25) sequence/seqcall.cpp: the file name is invented (no source string). The boundary between
  it and sequence/sequence.cpp is a guess too: seqSetLanguage .. Sequence::term (0x20e3d8..0x20e690) went
  to sequence.cpp because the Sequence constructor sits right before Sequence::init; the folder buffer data
  0x2d9910 lies between the table's data and the VM's, so it fits either file.
- (2026-10-25) GamMan +0x40 is named `GamArea` (invented: a rect list +0xc290 tested against the PC). The
  GamMan item methods (addItem / delItem / addStock ...) are named from their code: the slots +0x1a6c0
  (12 x 0x10, id +8, count +0xa) are the held items, the list +0x1a7f0 (up to 0x14e) the stock -- check
  against the item / stock menus when they run.

- **The i3r run's file split** (2026-10-25, the i3rroot session, decided unattended):
  0x228c08..0x22e840 has no source strings. Pinned only `i3d/i3rroot.cpp` 0x22b1b0..0x22b4b8 (the
  I3D_BIN root). Guesses for the rest: the mesh packet builder 0x228c08..0x22a7d0, per-type
  accessor / fixup clusters (one file per item type, or one generated file?), I3D_I3M
  0x22b4b8..0x22ba20, I3D_I3C 0x22ba20..0x22c1d8, the item tree core (bisect 0x22c1f8, relocators
  0x22c2a0..0x22c5a0) -- next: the item core with the two 90-entry fixup tables, needed before any
  model relocates. The root data's +0x10 / +0x14 0x40-byte tables and +0x24 value have unknown
  roles (names i3rGetRootMatrix10 / 14, i3rGetRootParam24 are placeholders).

- (2026-10-22, the user, live) **The Now Loading screen shows for about 1 frame, then black.** Why: SysNowLoading::draw is only called from FileReader::update 0x1be564's synchronous wait loop, which runs until isReady reports the read done; host reads finish at once, so it draws once per read. The boot then has almost nothing to read: GamMan::loadSystemData 0x1c1b48 and GamMan::loadStage 0x1c204c are stubs, so main tears the loading screen down right away and the frame loop shows the (still empty) scene. Expected until GamMan's loads are real; not a draw bug. If the screen should stay up like on the PS2, that is a decision for the user (e.g. a minimum display time in the PC read path), not ROM behaviour.
- (2026-10-22, magscene session 3; answered in session 4) **Who fills the scene's 256 wall
  segments?** `MagScene::addPrivateColli` 0x135e20 (MAX_PRIVATE_COLLI 256; called from 0x1d080c /
  0x1d091c / 0x1d0c64, unassigned code at 0x1d0..., likely event scripts) and removePrivateColli
  0x135f44 (from 0x1ccc30); a segment with equal end heights is free. Still open: which event
  commands those callers are. Also: `MagScene::term` takes the scene's member lives off the manager
  before deleteAll (a PC guard: the host's delete on a member corrupts the heap); check what the ROM
  relies on there when term is first reached.

- (2026-10-22, the sndman session) **Sound has no host backend.** `snd/sndman.cpp` is written
  (`verify`) and SndMan::update now runs every frame, but every rx sound library call
  (`rx/rxsnd.h`) talks to the silent IOP stand-in src/sdk/iop_rxsnd.cpp (the EE side is complete
  since 2026-10-25; the stand-in ignores the PCM commands 0xb0 / 0xb1), so nothing is heard and streams never report an end
  (status 0). Whether to
  reconstruct rxSnd's EE side over a host IOP sound module (ReiPan's src/sdk iop_libsd / iop_voice) or
  to replace it with a host mixer at the rxSnd API is the user's call; until then sound TUs finish
  `verify`. The rxSnd field named "attr" (IOP command 0x82 / 0xa2 / 0xb2, parameter 2) may be pan or an
  effect send: unconfirmed (rxSndPcmEnd pads a mode 3 PCM stream with it, so for PCM it may be a
  sample value). Also unknown (2026-10-25): queued 0x31 parameter 0 (rxSndSetSysMix sends 0xfcc /
  0xffc, MMIX-like bits) and parameter 2 (rxSndSetSysMute, 0x7fff pairs), command 0xb4
  (rxSndPcmSetParamB4) and 0xa4 parameter 5 (rxSndAdpcmSetChannelParam5); all are unreferenced.
- (2026-10-22, the sndman session) The rest of **unassigned@00164fc0** (0x164fc0..0x17f0d0) still
  mixes several files: EmyMain / EmyOut code to ~0x16a914, the Eff* effect classes (Eff0 vtable
  0x2e3250 .. EffWater3, 0x16a914..~0x17c0f0; FUN_00176afc and FUN_00177830 are reached stubs from
  them), SctLayer 0x17c0f0..~0x17c8b8, unread code 0x17c8b8..0x17e3d8 (role not checked), WrdSrv / WrdTable (data/wrd/WRD.BIN,
  0x17e3d8..0x17f0d0). Define those files before porting from it.
  2026-10-26: 0x17e540..0x17f0d0 is now `wrd/wrdsrv.cpp` (WrdSrv, verify: tested on WRD.BIN stages 1 and 13 by a temporary
  self-test -- rooms, names, index, routes, conversions right -- but nothing on screen shows it until a
  room caption draws; WrdTable renamed to the ELF's
  WrdSrv). Doubts: the file name / directory is a guess (no source string); 0x17e3d8 / 0x17e464 /
  0x17e4d0 (a strcmp lookup over 0x20-byte named records with member lists, setting / clearing bits
  in each member's +8; called by sectorSetFlags / ClearFlags) were left with the sector code before
  them. Next pick in the rest: the effect manager Obj176a3c (0x176a3c..~0x1788ec, scene +0x837e0)
  -- its update / draw 0x17772c / 0x177830 and init 0x176afc are reached every frame.

- (2026-10-22, the i3dmath session) **Naming of the vector module's two copies**: the older
  sessions gave five first-copy functions the plain names (i3dVecZero, i3dVecCopyW, i3dMatCopy,
  i3dVecCross, i3dVecNormalize) while the second copy has i3dVecCopy / i3dVecCross2 /
  i3dVecNormalize2 ...; the rest of the first copy is i3dVu0*. Kept as is (renaming touches i3denv /
  i3dlayer / i3dview / i3dpacket); a later cleanup could rename the five to i3dVu0* in Ghidra and src.
  Also unchecked against the ROM: the VU0 micro 0 product order (taken from the old bridge) -- the
  first 3D picture will show it.

- **The platform's libgraph.h swaps two macro pairs** (2026-10-22, the magsystem session):
  `SCE_GS_INTERLACE` 0 / `SCE_GS_NOINTERLACE` 1 and `SCE_GS_FIELD` 0 / `SCE_GS_FRAME` 1, where the
  SDK has NOINTERLACE 0, INTERLACE 1, FRAME 0, FIELD 1. magsystem.cpp passes the ROM's numbers (0, 1,
  2, 0) with a comment; main.cpp's mainInitGraph spells `SCE_GS_FRAME` (= 1 here): check it against
  the ROM's 0x20491c. Harmless today (the host reads only the NTSC / PAL mode), but fix the header
  before anything reads the interlace or field mode.
- **MagSystem open points** (2026-10-22): MagSystemInfo +0x0 / +0x4 and MagSystemPadRef's last three
  words have no reader found; MagSystem::term frees the static packet buffer 0x308580 through the game
  heap (never reached: mainExit follows an endless loop); 0x7b40c0's class (`RxMpeg`, a movie player
  next to snd/rxmpeg.cpp) and `SndMan::update` are names from use, unverified; the MagScene method
  names come from what MagSystem passes them (ror-inferred, low confidence for load / draw / view).

- **Spot cone units** (2026-10-22, the i3denv session): i3dLightSetSpot 0x21af50 reads the cone at
  +0x30 / +0x34 as half angles in radians (cosf / sinf of them), while mag/magscene.h's
  MagShadowLight comment calls them tangents (tan - 0.1, at most 0.7). Check makeActorShadow's
  values when the spot shadow path runs; one of the two comments is wrong. Light kinds 1, 3, 4
  (point, "dir2", "vec3") have roles unverified: read the VU1 lighting microcode or a caller
  (DogLightPool::createLight) to name their fields.
- **The i3d packets on the host** (2026-10-22, the i3dctx session; decided in-session, unconfirmed with
  the user): i3d builds EE DMA chains (VIF / GIF codes, REFs of the ROM's VU microprograms at data
  0x2912b0 / 0x2949f0). The context keeps host pointers and the tags' address words are the low 28
  bits of host pointers (`i3dDmaAddr`), because the host's `sceDmaSend` only records the chain start
  and nothing walks it. Before M4 the user should decide how the 3D path reaches the screen: a host
  interpreter of i3d's chains (needs real addresses: put the packet buffer in emulated EE RAM and
  store EE addresses, plus a VU1-microcode replacement), or a host renderer fed above the packets
  (at i3d's draw-set / model level, the way ReiPan's renderer took FF3's meshes). The packet-maths
  code in i3dctx.cpp is written so either can follow.
- **The disc data**: found (Decisions, 2026-10-22). Still open: RoR's archive formats (`.RPK`, the
  DATA/ subfolders); the ELF reads `cdrom0:\DATA\...` and `data/...` paths (architecture.md "Disc
  and data files").
- **The rx middleware** (rxfile, rxThread, snd/rxmpeg.cpp's partner): whose library is it, where does it
  start below 0x24b490, and which files does it split into? No symbols, no reference: reconstruct like
  game code, under `rx/`.
- **libpad2, libdbc, libvib** have no symbolised reference (FF2 / FF3 use libpad): their functions stay
  `FUN_` in the library range. The platform layer must still answer them (libpad2's API differs from
  libpad's: `scePad2*`). Name the ones RoR calls by reading (`lib-context`) when a TU needs them.
- **`CmnITask`**: base of 32 classes, but no vtable of its own -- a pure interface? Check a Tsk* ctor.
- **0x2046d4**: is it the main loop? (called once from main, calls into many modules).
- 68 `(unassigned)` runs in ror_functions.txt after the first export: they become TUs as sessions reach
  them; do not define them all up front.

- **rxsys details left open**: the memory hooks' signatures (0x105ca0 is installed as hooks 0 and 1;
  0x105de0, 0x105e08 = the operator-delete target) -- name them when the allocator TU is read; the four
  pools `rxSysInitModules` builds (0x7dabb0.., element layouts unknown); `s_bootDev[0] == 'a'` adds
  usbd.irx (which host device starts with 'a'?); `rxSysReadRomDate` has no BIOS on the host (date 0).

- **rxthread details left open** (2026-10-22): nothing in the ELF calls rxThSetPrioIndex,
  rxThSetInterval, rxThLockRequest / rxThUnlockRequest, the getters or rxThDummy0/1 (the defaults
  {0, 2, 4} and 4 HSYNCs hold); check snd/rxmpeg.cpp's rxThPostCall argument types (intptr_t on the
  host) when it is reconstructed; decide whether the host should keep rx threads behind the main
  thread (gate them on rxThYield) if a race shows up -- today they run concurrently.

- **rxfile details left open**: what the device record's first word (`rxFileSetDeviceParam`, -1 at init)
  means; who sets the HDD error
  hook 0x2dc248 (no setter found in rxfile's range); how the platform's sceOpen resolves `cdrom0:`
  paths (`RosePan_PathResolveData`; check once main runs).

- **bedmemory host deviations** (2026-10-22, decided in an unattended run): the two realloc link
  slips (architecture.md "The block heap") are mended on the host, since the ROM's own free would hang
  on them; splitTail's double shift is kept as written. Revisit if a caller passing flags 0x100 shows
  misaligned blocks. Also: the host header is 56 bytes (EE 0x20), so the heap holds slightly less;
  utlMemInit's probe size may need raising once host objects (bigger than EE ones) fill it.

- **Projected shadow open points** (2026-10-22, makeActorShadow): the shadow view's up vector at
  data 0x298cf0 reads (1, -1, 1, 0) -- odd for an up vector; check i3dMatLookDir's use of it when
  that module is written. The MagScene constructor's value for +0x2040 (m_shadow_mode) comes from a
  loop register (s8, li 3 then decremented; most likely -1) -- confirm when the constructor is
  written; the host stub leaves it 0 (mode 0 = the point-light path), harmless while no actor has
  the 0x60 flags. The light-set mask layout is settled (2026-10-22, dog/doglightpool.cpp): masks at
  +0x48, one word per kind, 0x18 per set; +0x50 is the POINT word. DogLightPool is fully written
  (2026-10-22, `verify`: +0x30 m_used, +0x108 m_data; lines 129..132 / 140..143 are the non-const /
  const getters). Open: DogLightBuffer::setupDesc 0x110578 reads its direct / point counts from
  +0x5e0 / +0x600, past the counts at +0x5c0 (a ROM bug or a bigger struct; nothing calls it, the
  port uses m_num); the pool's ~MagScene term call is not run on the host (static destruction order).
  It shows on screen once MagScene::loadLight fills the pool (GamMan::loadStage is still a stub).

- **main.cpp open points** (2026-10-22): `mainInitGame` passes bss 0x7812ac to `seqSetLanguage` (role
  unknown; kept as `g_bss_7812ac`), and sets bss 0x7a3d48 / 0x7a3d4c = 1 and data 0x296198 = 2 after the
  boot load (`g_bss_*` / `g_data_*` in lib/unassigned.h until the game modes that read them are
  written). Class names `CmnTaskMan`, `SysTimer`, `SndMan`, `Sequence`, `MagScene` (for MagSystem's
  +0x12540 member) are inferred, not from the ELF: confirm or rename when their TUs are read.
  rxsys.cpp has a function Ghidra folded at 0x243620 (`j 0x248870`, between rxSysInitFileDevice and
  rxSysSetFileDevice; probably rxSysGetFileDevice) -- create it in Ghidra (`ghidra.py mkfunc`) when
  rxsys is next touched.

## Done

- 2026-10-22: `game/gamfile.cpp` (0x1be2cc..0x1beb70 pinned, all 21 functions, `verify`): the file
  reader (FileReader / FileReadReq, the request state machine 0x1be564 that draws the now-loading
  screen, fileGetSize, fileExists, clock helpers). The FileReader bridges and the fileGetSize /
  fileExists stubs are gone; main's now-loading object is the extern `g_nowLoading`. Black screen:
  every FileReader::read caller that runs in the boot (GamMan::loadSystemData / loadStage, MagScene's
  loaders behind MagScene::start) is still a stub. What is left of unassigned@001bde58: the
  language-select menu code 0x1bde58..0x1be2cc (mnusllg.rpk, SprMan calls: belongs to
  mnu/mnuSelectLanguage.cpp, whose functions start at ~0x1bd028 -- pin it with that TU) and GamFont
  0x1beb70..0x1c05c0 (game/gamfont.cpp is the natural key; GamFont::drawChar is sprman's text path).

- 2026-10-22: host task `setup/i3d_draw_module.cpp` (`verify`): the native i3d module
  (rosepan_i3d.*), rosepan_draw2d's general TIM2 / GS-format decoder, the renderer's
  `RosePan_RendererDrawImageTriangles2D`, the forced test `ROSEPAN_I3D_TEST=1`. tm2png.py now applies
  CSM1 to 4-bit CLUTs of 32+ colours (S2DMAN_00011000 was drawn with the wrong half of its palette).

- 2026-10-22: `spr/s2d.cpp` (0x190a78..0x192ac0 pinned): 72 of 82 functions written (`partial`),
  86 names (SprMan, SprTexture, S2d*); datadump.py `--words` for raw EE tables. Black screen: the
  sprite manager and the scene draw are stubs.

- 2026-10-22: `mag/magsystem.cpp` (0x117654..0x1194b4 pinned): 40 of 52 functions written
  (`partial`): constructor / destructor, the V-blank callbacks, initGraph (pads, VIF1, GS reset,
  512x448 double buffer, the i3d version check, the i3d context on the static packet buffer),
  start / term, the frame (update, begin / end / kick packet, waitVSync, the storeImage BMP dump), the
  scene / light / camera / model forwarders, the PC / Bwn binders. 0x11871c, 0x1191e0, 0x1192c8
  created in Ghidra; about 80 names (MagSystem, MagScene, BedPad, i3d, utlRand*, rxMpeg). Platform:
  `vif0ResetVu0` (a no-op, src/sdk/eekernel); the game heap is now the global `g_utlMemory`. main's PC
  `sceGsSyncV` is gone: the loop runs at 30 fps on the semaphore. Black screen (scene and i3d stubs).

- 2026-10-22: `main.cpp` (0x2043dc..0x204d38 pinned): main and its 10 helpers written (`verify`):
  argument parsing, the rx bring-up from host0:, the game init around the now-loading object, the frame
  loop, device init / exit, the debug exception handler. Also `rxSysInitFileDevice` (rx/rxsys.cpp),
  host `sceMcEnd`, libdbc and libpad2 Init / End shims; 46 names. The run reaches mainLoop at 60 fps;
  black screen (all draw code is stubs).

- 2026-10-22: `bed/bedmemory.cpp` (0x100354..0x101718 pinned): all 16 BedMemory functions written
  (`verify`: nothing in the boot allocates yet); 9 helpers named (format, alignBlock, splitTail,
  findFree, findFreeFromEnd, getHeader, shrink, trimFront, getInfo). Black screen as before.

- 2026-10-22: `rx/rxthread.cpp` (0x249b88..0x24b490): all 53 functions written (`verify`; 11 were
  folded into neighbours and created in Ghidra): scheduler, dispatch alarm, slots, call queue, yield
  callbacks, vblank / HSYNC counters. Platform: SetAlarm / iSetAlarm / ReleaseAlarm (a host timer
  thread), ReferSemaStatus / iReferSemaStatus (count + waiters), iPollSema, iChangeThreadPriority,
  ExitDeleteThread (parks), VSync, sceGsGetGParam, and libgraph's vblank thread raising INTC cause 2.
  A temporary call from main showed 60 vblanks a second, one per rxThVSync(0), ~262 HSYNCs a frame.
  rxSysPrintWait / rxSysThreadCallback now take the real arguments (intptr_t *args / int yielding).

- 2026-10-22: `rx/rxfile.cpp` pinned 0x2437f8..0x249b88 and `rx/rxthread.cpp` 0x249b88..0x24b490;
  rxfile `partial`: drivers, host0: / HDD drivers, cdrom table and sector-cache init, contexts, dirs,
  rxFileLoad / LoadModule / RebootIop, plus rxsys's sce* file wrappers 0x242500..0x242660. Platform:
  `sceSifLoadModule` added to src/sdk/sifdev (succeeds). 14 driver-slot / thread functions created in
  Ghidra.

- 2026-10-22: `rx/rxsys.cpp` (0x23ff58..0x2437f8 pinned): 60 functions -- rxSysInit, the IOP module
  loader (iopconf.cnf), boot path, system.ini, rxSysPrintf, semaphores, memory hooks, rxfile wrappers;
  rxfile / rxthread callees named and stubbed (src/lib/unassigned_stubs.cpp). Platform: `GetMemorySize`
  (32 MB, retail) and `sceDevVu1Reset` added to src/sdk/eekernel. `verify`: main is still the placeholder.

- 2026-10-22: PC project created (`setup/platform_bootstrap.cpp`): ReiPan's platform layer as
  `src/rosepan` + `src/sdk`, CMake (target RosePan, sibling extern ReiPan then MioPan), placeholder
  `src/main.cpp`; green build, 60 Hz empty black window in the smoke run, layoutcheck clean.

- 2026-10-22: Ghidra seeded (`setup/ghidra_seed.cpp`): 1,449 ELF symbols, 1,642 functions created
  (sections, vtable slots, library code, tail-jump targets), 677 library names, `main` / crt0 /
  four FF3-reference collisions named by hand. Counts in architecture.md "Naming sources"; the traps it
  found (tail targets folded into callers, merged FF3 reference bodies) in hazards.md.
- 2026-10-22: RosePan created from ReiPan's learning: the rosepan-port skill and references, the tools
  in `tools/reverse/` (GCC 2 demangler, section symbols, GCC 2 RTTI with vptr offsets, the library
  matcher, file inference from RoR's own source strings, the TU queue, build / check / autopilot), the
  address map in `data/layout.json`, and the first two host tasks.
