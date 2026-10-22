# RosePan status (generated)

Generated 2026-10-22 14:49 at commit `df77790` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 543 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6412**, named **2177** (34%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 543, lib-match-low 133, lib-context 51, ror-inferred 758
- context-name ledger: 809 entries; overrides of wrong library matches: 5
- placed in a source file: 2039/6412 in 80 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00150000` | 0/272 (0%) | - |
| `00160000` | 0/268 (0%) | emy/emy_main.cpp |
| `001a0000` | 0/211 (0%) | mnu/mnusvld.cpp, mnu/mnuselect.cpp, mnu/mnudlg.cpp |
| `001d0000` | 0/181 (0%) | game/gamstat.cpp, game/gamtrg.cpp |
| `001f0000` | 0/221 (0%) | task/naga_chunk.cpp, task/tsk_bg_load.cpp, task/tsk_scene.cpp |
| `00120000` | 2/283 (1%) | mag/maglifebwnctrl.cpp, mag/maglifepcmode.cpp, mag/maglifemenu.cpp |
| `001e0000` | 2/169 (1%) | game/gamsnd.cpp |
| `00170000` | 3/251 (1%) | - |

## Reconstruction (src/)

- reconstructed RoR functions: **591**; stubs: **169**

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
| `src/lib/unassigned_stubs.cpp` | 0 | 69 |
| `src/mag/maglife.cpp` | 0 | 1 |
| `src/mag/maglifebwn.cpp` | 0 | 1 |
| `src/mag/maglifeman.cpp` | 0 | 2 |
| `src/mag/maglifepc.cpp` | 0 | 4 |
| `src/mag/magloader.cpp` | 0 | 7 |
| `src/mag/magscene.cpp` | 5 | 12 |
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
| `src/spr/s2d.cpp` | 86 | 0 |
| `src/spr/sprman.cpp` | 0 | 13 |
| `src/spr/sprtexture.cpp` | 0 | 3 |
| `src/system/sysnowloading.cpp` | 8 | 0 |
| `src/utl/utlfile.cpp` | 0 | 2 |
| `src/utl/utlfind.cpp` | 0 | 1 |
| `src/utl/utlgame.cpp` | 0 | 1 |
| `src/utl/utlmisc.cpp` | 24 | 0 |

## Translation units (one session each: `tus.py`)

- 151 TUs: todo 137, partial 5, verify 12, done 3
- next: **mag/magscenerender.cpp** (pinned in the queue)
- pinned queue: mag/magscenerender.cpp, unassigned@0021fb50, unassigned@00216ad0, spr/sprtexture.cpp, dog/dogtexinfo.cpp, spr/sprman.cpp
- verify: `bed/bedmemory.cpp` 16/16 -- All 16 functions written; nothing in the boot allocates yet (main is still the placeholder), so the screen stays black and the status is verify. A scratch host 
- verify: `i3d/i3dctx.cpp` 31/31 -- Status verify: the screen stays black because MagScene::draw (no layers) and i3dCtxCloseFrame (0x224798, draw-group module) are stubs, so nothing reaches the pa
- verify: `i3d/i3dvram.cpp` 37/37 -- Complete; nothing on screen shows it yet (screen black: the draw path is stubbed). A host test (init, reserve x3 frames with eviction, compact, end frame, reset
- verify: `mag/magscene2.cpp` 19/19 -- Complete but unseen: MagScene::start (magscene.cpp) and the GamMan loads are stubs, so no loader runs and the screen stays black (MagScene::draw is a stub too).
- verify: `mag/magsystem.cpp` 55/55 -- All 55 functions written; nothing on screen can show them yet (black: MagScene::draw and i3d are stubs; no caller of the actor methods runs). The Dog classes ar
- verify: `main.cpp` 11/11 -- Status verify: the smoke run boots for real (iopconf.cnf, eight IOP modules and system.ini read from the disc folder through host0:), runs mainInitGame and loop
- verify: `rx/rxfile.cpp` 168/168 -- Status verify: every function in 0x2437f8..0x249b88 is written, but nothing can show it: main 0x2045a8 is still the placeholder (black screen), and the rxthread
- verify: `rx/rxsys.cpp` 74/119 -- Status verify: nothing calls rxSysInit yet - main 0x2045a8 is still the placeholder in src/main.cpp, so the screenshot is the black placeholder frame. Next: mai
- verify: `rx/rxthread.cpp` 53/53 -- Complete; verify because main (unassigned@001fd424) is still the placeholder, so nothing calls rxThInit in the smoke run (black frame as before). A temporary ca
- verify: `spr/s2d.cpp` 86/86 -- All 86 functions written; nothing on screen can show them yet (MagScene::draw is a stub), so verify: the first session whose screenshot shows S2d sprites marks 
- verify: `system/sysnowloading.cpp` 8/8 -- Code complete, nothing on the boot path shows it yet: draw 0x2051cc is called only by the pfh loader 0x1be564 (GamMan load path, unassigned@001bde58), a stub, s
- verify: `utl/utlmisc.cpp` 24/24 -- Verify: nothing calls this yet (main 0x2045a8 is still the placeholder; the smoke run shows a black frame). When main is reconstructed it calls utlMemInit and i

Recent sessions:

- 2026-10-22 14:49 `system/sysnowloading.cpp` verify: SysNowLoading complete: pinned 0x204d38..0x2053f4, all 8 functions (init, term, loadImage, setST, makePacket, uploadImage, clearVram, draw) named and written; n
- 2026-10-22 14:38 `spr/s2d.cpp` verify: Completed spr/s2d.cpp: S2dMes::drawMapName (word-table map name in a scene text window with its frame sprite fading), S2dMovie drawMovie / draw / createLayer (i
- 2026-10-22 14:23 `spr/s2d.cpp` partial: Pinned spr/s2d.cpp (0x190a78..0x192ac0, the scene's 2D layers). Wrote S2dCineSco (cinemascope bars), S2dBg (s2dgen.rpk pictures, inline s2dBg requests), S2dMes 
- 2026-10-22 14:04 `mag/magscene2.cpp` verify: Pinned mag/magscene2.cpp to 0x137300..0x139540 and wrote all 19 functions: loadSystemData (options.txt, the PREMIERE.RPK font and common textures), load (a map 
- 2026-10-22 13:42 `mag/magscene2.cpp` partial: MagScene set-up half: init (fog records, light no, life manager, texture table with shadow01, sky slots), initRender (two view records, controller scene links, 
- 2026-10-22 13:24 `i3d/i3dvram.cpp` verify: i3d VRAM cache: all 37 functions written (block lists, alloc first-fit from the LRU tail, LRU eviction, coalesce, compact, end frame, reset, init, term, handle 
- 2026-10-22 13:15 `i3d/i3dctx.cpp` verify: i3d draw context: pinned i3d/i3dctx.cpp 0x21eb28-0x21fb30 and wrote all 32 functions (7 hidden ones created in Ghidra: default end hook, SetEndFunc, GetRestFrom
- 2026-10-22 12:55 `mag/magsystem.cpp` verify: MagSystem complete: the 12 actor methods (setActorLightID, setActorParent, getActorNodeMatrix x2 and ByName x2, getActorMotionMove, addActorTrans, releaseActorM

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `spr/sprman.cpp` | todo | 0/13/53 | 14.5 | 49 | 2 |
| `unassigned@0021fb50` | todo | 0/14/118 | 31.5 | 33 | 2 |
| `unassigned@001bde58` | todo | 0/4/56 | 9.8 | 26 | 0 |
| `game/gamman.cpp` | todo | 0/4/288 | 44.9 | 24 | 4 |
| `unassigned@00228c08` | todo | 0/5/465 | 91.8 | 21 | 2 |
| `unassigned@00164fc0` | todo | 0/6/433 | 108.5 | 21 | 3 |
| `mag/magscene.cpp` | todo | 0/10/58 | 15.7 | 20 | 2 |
| `bed/bedpad.cpp` | todo | 0/5/5 | 0.9 | 20 | 2 |
| `dog/dogtexinfo.cpp` | todo | 0/7/16 | 5.1 | 20 | 2 |
| `mag/magscenelight.cpp` | todo | 0/5/8 | 2.3 | 19 | 2 |
| `bed/bedpfh.cpp` | todo | 0/3/4 | 0.9 | 19 | 0 |
| `unassigned@001fd424` | todo | 0/3/75 | 27.9 | 18 | 3 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `00101750` | BedPfh::BedPfh | 11 | ror | stub | bed/bedpfh.cpp |
| `001be524` | FileReader::read | 10 | ror | stub | - |
| `001019f0` | BedPfh::getFileIndex | 7 | ror | stub | bed/bedpfh.cpp |
| `001be734` | FileReadReq::FileReadReq | 5 | ror | missing | - |
| `001be2dc` | FileReader::~FileReader | 5 | ror | missing | - |
| `001196d4` | MagLifeMan::add | 5 | ror | stub | mag/maglifeman.cpp |
| `0018d7ec` | SprMan::setFade | 5 | ror | stub | spr/sprman.cpp |
| `0018c190` | SprMan::setDispList | 4 | ror | stub | spr/sprman.cpp |
| `0013f980` | MagScene::setLightParam | 4 | ror | stub | mag/magscenelight.cpp |
| `0018c068` | SprMan::update | 4 | ror | stub | spr/sprman.cpp |
| `0018bd94` | SprMan::draw | 4 | ror | stub | spr/sprman.cpp |
| `001be2cc` | FileReader::FileReader | 4 | ror | missing | - |
| `0018bb8c` | SprMan::SprMan | 4 | ror | stub | spr/sprman.cpp |
| `00101ccc` | BedPad::update | 4 | ror | stub | bed/bedpad.cpp |
| `0018bcc0` | SprMan::init | 4 | ror | stub | spr/sprman.cpp |
| `0018c130` | SprMan::release | 4 | ror | stub | spr/sprman.cpp |
| `0018bc18` | SprMan::~SprMan | 4 | ror | stub | spr/sprman.cpp |
| `00237cc8` | i3dFlushCache | 4 | ror | stub | - |
| `00223260` | i3dLayerSetView | 4 | ror | stub | - |
| `0022b228` | I3rRoot__CheckIdString | 3 | lib-ctx | stub | - |
| `00101b50` | BedPad::setVibration | 3 | ror | stub | bed/bedpad.cpp |
| `0018febc` | SprTexture::update | 3 | ror | stub | spr/sprtexture.cpp |
| `00223240` | i3dLayerSetLight | 3 | ror | stub | - |
| `00223378` | i3dLayerBeginPacket | 3 | ror | stub | - |
| `0021b550` | i3dPktSetReg | 3 | ror | stub | - |
| `00223398` | i3dLayerEndPacket | 3 | ror | stub | - |
| `00115cec` | DogModel::release | 3 | ror | stub | dog/dogmodel.cpp |
| `0021a828` | i3rCopyClipSphere | 3 | lib-ctx | stub | - |
| `00215bb4` | dbgTraceVector4 | 3 | ror | stub | - |
| `00105f0c` | utlStrCopy2 | 3 | ror | stub | utl/utlfile.cpp |
| `0013f4d0` | MagScene::setLightNo | 2 | ror | stub | mag/magscenelight.cpp |
| `0020e3d8` | seqSetLanguage | 2 | ror | stub | - |
| `0018f520` | SprMan::fadeOut | 2 | ror | stub | spr/sprman.cpp |
| `001102b4` | DogEntityEx::setParent | 2 | ror | stub | dog/dogentity.cpp |
| `001163dc` | DogTexInfo::find | 2 | ror | stub | dog/dogtexinfo.cpp |
| `001174c4` | DogTexInfo::setTexFlag | 2 | ror | stub | dog/dogtexinfo.cpp |
| `00142b18` | MagCharaLoader::load | 2 | ror | stub | mag/magloader.cpp |
| `0018f744` | SprTexture::setList | 2 | ror | stub | spr/sprtexture.cpp |
| `0022b3d8` | i3rGetNodeIndexByName | 2 | lib-ctx | stub | - |
| `0011756c` | DogTexInfo::setTexTable | 2 | ror | stub | dog/dogtexinfo.cpp |

