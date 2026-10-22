# RosePan status (generated)

Generated 2026-10-22 15:47 at commit `97a11d8` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 543 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6434**, named **2320** (36%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 543, lib-match-low 133, lib-context 83, ror-inferred 869
- context-name ledger: 952 entries; overrides of wrong library matches: 5
- placed in a source file: 2106/6434 in 81 files

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

- reconstructed RoR functions: **689**; stubs: **219**

| file | reconstructed | stubs |
|---|---|---|
| `src/bed/bedmemory.cpp` | 16 | 0 |
| `src/bed/bedpad.cpp` | 0 | 5 |
| `src/bed/bedpfh.cpp` | 0 | 3 |
| `src/dog/dogactor.cpp` | 0 | 15 |
| `src/dog/dogentity.cpp` | 0 | 3 |
| `src/dog/dogmodel.cpp` | 0 | 8 |
| `src/dog/dogtexinfo.cpp` | 0 | 9 |
| `src/game/gamman.cpp` | 0 | 4 |
| `src/i3d/i3dctx.cpp` | 31 | 0 |
| `src/i3d/i3dlayer.cpp` | 80 | 0 |
| `src/i3d/i3dvram.cpp` | 37 | 0 |
| `src/lib/unassigned_stubs.cpp` | 0 | 108 |
| `src/mag/maglife.cpp` | 0 | 1 |
| `src/mag/maglifebwn.cpp` | 0 | 1 |
| `src/mag/maglifeman.cpp` | 0 | 3 |
| `src/mag/maglifepc.cpp` | 0 | 4 |
| `src/mag/magloader.cpp` | 0 | 7 |
| `src/mag/magscene.cpp` | 5 | 12 |
| `src/mag/magscene2.cpp` | 19 | 0 |
| `src/mag/magscenecamera.cpp` | 2 | 1 |
| `src/mag/magscenelight.cpp` | 0 | 6 |
| `src/mag/magsceneoption.cpp` | 0 | 1 |
| `src/mag/magscenerender.cpp` | 20 | 0 |
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

- 154 TUs: todo 139, partial 5, verify 14, done 2
- next: **unassigned@00224de0** (pinned in the queue)
- pinned queue: unassigned@00224de0, unassigned@00216ad0, spr/sprtexture.cpp, dog/dogtexinfo.cpp, spr/sprman.cpp
- verify: `bed/bedmemory.cpp` 16/16 -- All 16 functions written; nothing in the boot allocates yet (main is still the placeholder), so the screen stays black and the status is verify. A scratch host 
- verify: `i3d/i3dctx.cpp` 31/31 -- Status verify: the screen stays black because MagScene::draw (no layers) and i3dCtxCloseFrame (0x224798, draw-group module) are stubs, so nothing reaches the pa
- verify: `i3d/i3dlayer.cpp` 80/80 -- Every frame now builds real groups, layers and a sorted DMA chain; the smoke run is stable and still black: nothing on the i3d path reaches a host draw module (
- verify: `i3d/i3dvram.cpp` 37/37 -- Complete; nothing on screen shows it yet (screen black: the draw path is stubbed). A host test (init, reserve x3 frames with eviction, compact, end frame, reset
- verify: `mag/magscene2.cpp` 19/19 -- Complete but unseen: MagScene::start (magscene.cpp) and the GamMan loads are stubs, so no loader runs and the screen stays black (MagScene::draw is a stub too).
- verify: `mag/magscenerender.cpp` 20/20 -- All 20 functions written; verify because nothing on screen can show it: no boot actor has the 0x60 shadow flags and all i3d layer / view / light functions are s
- verify: `mag/magsystem.cpp` 55/55 -- All 55 functions written; nothing on screen can show them yet (black: MagScene::draw and i3d are stubs; no caller of the actor methods runs). The Dog classes ar
- verify: `main.cpp` 11/11 -- Status verify: the smoke run boots for real (iopconf.cnf, eight IOP modules and system.ini read from the disc folder through host0:), runs mainInitGame and loop
- verify: `rx/rxfile.cpp` 168/168 -- Status verify: every function in 0x2437f8..0x249b88 is written, but nothing can show it: main 0x2045a8 is still the placeholder (black screen), and the rxthread
- verify: `rx/rxsys.cpp` 74/119 -- Status verify: nothing calls rxSysInit yet - main 0x2045a8 is still the placeholder in src/main.cpp, so the screenshot is the black placeholder frame. Next: mai
- verify: `rx/rxthread.cpp` 53/53 -- Complete; verify because main (unassigned@001fd424) is still the placeholder, so nothing calls rxThInit in the smoke run (black frame as before). A temporary ca
- verify: `spr/s2d.cpp` 86/86 -- All 86 functions written; nothing on screen can show them yet (MagScene::draw is a stub), so verify: the first session whose screenshot shows S2d sprites marks 
- verify: `system/sysnowloading.cpp` 8/8 -- Code complete, nothing on the boot path shows it yet: draw 0x2051cc is called only by the pfh loader 0x1be564 (GamMan load path, unassigned@001bde58), a stub, s
- verify: `utl/utlmisc.cpp` 24/24 -- Verify: nothing calls this yet (main 0x2045a8 is still the placeholder; the smoke run shows a black frame). When main is reconstructed it calls utlMemInit and i

Recent sessions:

- 2026-10-22 15:47 `i3d/i3dlayer.cpp` verify: i3d/i3dlayer.cpp pinned 0x222f50..0x224de0 and written: all 80 functions (21 created in Ghidra, 75 names): layer setters / getters and Begin/End packet pairs, d
- 2026-10-22 15:24 `mag/magscenerender.cpp` verify: Wrote MagScene::makeActorShadow 0x139b4c, the last function of the TU: the shadow texture layer at 700 + 10 n (256 x 256 target, clear, drawShadow under an ambi
- 2026-10-22 15:15 `mag/magscenerender.cpp` partial: Pinned mag/magscenerender.cpp 0x139540..0x13c328 and wrote 19 of its 20 functions: MagScene::draw (eight light-set render stages, view and fog, actors collected
- 2026-10-22 14:49 `system/sysnowloading.cpp` verify: SysNowLoading complete: pinned 0x204d38..0x2053f4, all 8 functions (init, term, loadImage, setST, makePacket, uploadImage, clearVram, draw) named and written; n
- 2026-10-22 14:38 `spr/s2d.cpp` verify: Completed spr/s2d.cpp: S2dMes::drawMapName (word-table map name in a scene text window with its frame sprite fading), S2dMovie drawMovie / draw / createLayer (i
- 2026-10-22 14:23 `spr/s2d.cpp` partial: Pinned spr/s2d.cpp (0x190a78..0x192ac0, the scene's 2D layers). Wrote S2dCineSco (cinemascope bars), S2dBg (s2dgen.rpk pictures, inline s2dBg requests), S2dMes 
- 2026-10-22 14:04 `mag/magscene2.cpp` verify: Pinned mag/magscene2.cpp to 0x137300..0x139540 and wrote all 19 functions: loadSystemData (options.txt, the PREMIERE.RPK font and common textures), load (a map 
- 2026-10-22 13:42 `mag/magscene2.cpp` partial: MagScene set-up half: init (fog records, light no, life manager, texture table with shadow01, sky slots), initRender (two view records, controller scene links, 

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `unassigned@00216ad0` | todo | 0/35/125 | 31.9 | 133 | 7 |
| `unassigned@00228c08` | todo | 0/22/465 | 91.8 | 65 | 2 |
| `spr/sprman.cpp` | todo | 0/13/53 | 14.5 | 54 | 3 |
| `unassigned@00164fc0` | todo | 0/7/433 | 108.5 | 27 | 4 |
| `unassigned@001bde58` | todo | 0/4/56 | 9.8 | 26 | 0 |
| `game/gamman.cpp` | todo | 0/4/288 | 44.9 | 24 | 4 |
| `mag/maglifeman.cpp` | todo | 0/3/10 | 1.5 | 24 | 3 |
| `dog/dogtexinfo.cpp` | todo | 0/9/16 | 5.1 | 23 | 2 |
| `bed/bedpad.cpp` | todo | 0/5/5 | 0.9 | 20 | 2 |
| `mag/magscene.cpp` | todo | 0/10/44 | 11.3 | 20 | 2 |
| `bed/bedpfh.cpp` | todo | 0/3/4 | 0.9 | 19 | 0 |
| `mag/magscenelight.cpp` | todo | 0/5/8 | 2.3 | 19 | 2 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `0021b550` | i3dPktSetReg | 28 | ror | stub | - |
| `0021c940` | i3dPackXYZ | 14 | lib-ctx | stub | - |
| `00101750` | BedPfh::BedPfh | 11 | ror | stub | bed/bedpfh.cpp |
| `001be524` | FileReader::read | 10 | ror | stub | - |
| `0022b228` | I3rRoot__CheckIdString | 9 | lib-ctx | stub | - |
| `0021c8c8` | i3dPackRGBAQ | 7 | lib-ctx | stub | - |
| `001019f0` | BedPfh::getFileIndex | 7 | ror | stub | bed/bedpfh.cpp |
| `00226c58` | i3dCtxCreateView | 6 | ror | stub | - |
| `00237408` | i3dVecSub | 6 | lib-ctx | stub | - |
| `002372d0` | i3dVecCopy | 6 | lib-ctx | stub | - |
| `001be734` | FileReadReq::FileReadReq | 5 | ror | missing | - |
| `001be2dc` | FileReader::~FileReader | 5 | ror | missing | - |
| `0021a828` | i3rCopyClipSphere | 5 | lib-ctx | stub | - |
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
| `0021c538` | i3dPktLine | 4 | ror | stub | - |
| `0010ff74` | DogEntity::normalizeRotate | 3 | ror | stub | - |
| `00101b50` | BedPad::setVibration | 3 | ror | stub | bed/bedpad.cpp |
| `0021c140` | i3dPktSprite | 3 | lib-ctx | stub | - |
| `00237340` | i3dMatUnit | 3 | lib-ctx | stub | - |
| `0010fa6c` | DogEntity::getMatrix | 3 | ror | stub | - |
| `0018febc` | SprTexture::update | 3 | ror | stub | spr/sprtexture.cpp |
| `0021b148` | i3dCtxCreateLight | 3 | lib-ctx | stub | - |
| `002372e0` | i3dVecCopy2 | 3 | lib-ctx | stub | - |
| `0011756c` | DogTexInfo::setTexTable | 3 | ror | stub | dog/dogtexinfo.cpp |
| `00119a38` | MagLifeMan::draw | 3 | ror | stub | mag/maglifeman.cpp |
| `0022b3d8` | i3rGetNodeIndexByName | 3 | lib-ctx | stub | - |
| `0021e1d8` | i3dPktDrawLineStrip | 3 | lib-ctx | stub | - |
| `00115cec` | DogModel::release | 3 | ror | stub | dog/dogmodel.cpp |

