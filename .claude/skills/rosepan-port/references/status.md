# RosePan status (generated)

Generated 2026-10-22 18:35 at commit `f38b2c6` by `tools/reverse/status.py` (`refresh.py` runs it). Do not edit by hand.

## Ghidra seeding (one-time: sections.py --apply, libmatch.py --apply)

- ELF symbols (named linkonce sections): 1449; tagged ror-symbol in Ghidra: 1449
- library matches planned: 687; tagged lib-match / -low in Ghidra: 543 / 133
- classes (named vtables): 284

## Naming (game code; library ranges excluded)

- game functions: **6496**, named **2579** (40%); library functions: 1017
- by provenance: ror-symbol 1449, lib-match 543, lib-match-low 133, lib-context 90, ror-inferred 1121
- context-name ledger: 1211 entries; overrides of wrong library matches: 5
- placed in a source file: 2453/6496 in 87 files

Least-named game-code regions (64 KB blocks):

| block | named | inferred files |
|---|---|---|
| `00150000` | 0/272 (0%) | - |
| `00160000` | 0/268 (0%) | emy/emy_main.cpp |
| `001a0000` | 0/211 (0%) | mnu/mnusvld.cpp, mnu/mnuselect.cpp, mnu/mnudlg.cpp |
| `001f0000` | 0/221 (0%) | task/naga_chunk.cpp, task/tsk_bg_load.cpp, task/tsk_scene.cpp |
| `00120000` | 2/283 (1%) | mag/maglifebwnctrl.cpp, mag/maglifepcmode.cpp, mag/maglifemenu.cpp |
| `001d0000` | 2/181 (1%) | game/gamman.cpp, game/gamstat.cpp, game/gamtrg.cpp |
| `001e0000` | 2/169 (1%) | game/gamsnd.cpp |
| `00170000` | 3/251 (1%) | - |

## Reconstruction (src/)

- reconstructed RoR functions: **999**; stubs: **167**

| file | reconstructed | stubs |
|---|---|---|
| `src/bed/bedmemory.cpp` | 16 | 0 |
| `src/bed/bedpad.cpp` | 0 | 5 |
| `src/bed/bedpfh.cpp` | 0 | 4 |
| `src/dog/dogactor.cpp` | 0 | 15 |
| `src/dog/dogentity.cpp` | 0 | 3 |
| `src/dog/dogmodel.cpp` | 0 | 8 |
| `src/dog/dogtexinfo.cpp` | 16 | 0 |
| `src/game/gamman.cpp` | 0 | 5 |
| `src/i3d/i3dctx.cpp` | 31 | 0 |
| `src/i3d/i3denv.cpp` | 12 | 0 |
| `src/i3d/i3dlayer.cpp` | 80 | 0 |
| `src/i3d/i3dmath.cpp` | 95 | 0 |
| `src/i3d/i3dpacket.cpp` | 54 | 0 |
| `src/i3d/i3dtex.cpp` | 33 | 0 |
| `src/i3d/i3dtim2.cpp` | 7 | 0 |
| `src/i3d/i3dview.cpp` | 19 | 0 |
| `src/i3d/i3dvram.cpp` | 37 | 0 |
| `src/lib/unassigned_stubs.cpp` | 0 | 78 |
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
| `src/spr/sprman.cpp` | 53 | 0 |
| `src/spr/sprtexture.cpp` | 21 | 0 |
| `src/system/sysnowloading.cpp` | 8 | 0 |
| `src/utl/utlfile.cpp` | 0 | 2 |
| `src/utl/utlfind.cpp` | 0 | 1 |
| `src/utl/utlgame.cpp` | 0 | 1 |
| `src/utl/utlmisc.cpp` | 24 | 0 |
| `src/utl/utltm2.cpp` | 0 | 1 |

## Translation units (one session each: `tus.py`)

- 162 TUs: todo 139, partial 5, verify 24, done 2
- next: **unassigned@001bde58** (frontier score 81)
- pinned queue: (empty)
- verify: `bed/bedmemory.cpp` 16/16 -- All 16 functions written; nothing in the boot allocates yet (main is still the placeholder), so the screen stays black and the status is verify. A scratch host 
- verify: `dog/dogtexinfo.cpp` 16/16 -- Pinned 0x1161e4..0x11764c, all 16 functions written. Smoke run: MagScene::init's init / add run without asserts; screen still black (SprMan::draw and the scene 
- verify: `i3d/i3dctx.cpp` 31/31 -- Status verify: the screen stays black because MagScene::draw (no layers) and i3dCtxCloseFrame (0x224798, draw-group module) are stubs, so nothing reaches the pa
- verify: `i3d/i3denv.cpp` 12/12 -- Pinned 0x21a120..0x21a3e8 + 0x21ac80..0x21b220 as one file (a guess: the i3r helpers 0x21a3e8..0x21ac80 sit between; split into i3dfog / i3dlight if evidence ap
- verify: `i3d/i3dlayer.cpp` 80/80 -- Every frame now builds real groups, layers and a sorted DMA chain; the smoke run is stable and still black: nothing on the i3d path reaches a host draw module (
- verify: `i3d/i3dmath.cpp` 95/95 -- Nothing on screen can show it yet (boot still black; the i3d path has no host draw consumer, and the per-frame lightstatus asserts wait on DogLightPool::createL
- verify: `i3d/i3dpacket.cpp` 54/54 -- All packet-module bridges are gone. Black screen: nothing on the host consumes i3d packets (no GS emulation); the user still has to choose the host draw hook (o
- verify: `i3d/i3dtex.cpp` 33/33 -- Screen still black: nothing creates a texture yet (DogTexInfo is stubbed) and no host draw module consumes i3d packets, hence verify. Next: i3d/i3dtim2.cpp (que
- verify: `i3d/i3dtim2.cpp` 7/7 -- Complete code; the smoke screenshot stays black because no host draw module consumes the i3d texture objects yet, so nothing on screen shows TIM2 decoding. The 
- verify: `i3d/i3dview.cpp` 19/19 -- Status verify: views are built every frame (host test: screen centre 2047.5, frustum corner on the rectangle corner, depth 4094.6..1.06, scissor 0..511 x 0..447
- verify: `i3d/i3dvram.cpp` 37/37 -- Complete; nothing on screen shows it yet (screen black: the draw path is stubbed). A host test (init, reserve x3 frames with eviction, compact, end frame, reset
- verify: `mag/magscene2.cpp` 19/19 -- Complete but unseen: MagScene::start (magscene.cpp) and the GamMan loads are stubs, so no loader runs and the screen stays black (MagScene::draw is a stub too).
- verify: `mag/magscenerender.cpp` 20/20 -- All 20 functions written; verify because nothing on screen can show it: no boot actor has the 0x60 shadow flags and all i3d layer / view / light functions are s
- verify: `mag/magsystem.cpp` 55/55 -- All 55 functions written; nothing on screen can show them yet (black: MagScene::draw and i3d are stubs; no caller of the actor methods runs). The Dog classes ar
- verify: `main.cpp` 11/11 -- Status verify: the smoke run boots for real (iopconf.cnf, eight IOP modules and system.ini read from the disc folder through host0:), runs mainInitGame and loop
- verify: `rx/rxfile.cpp` 168/168 -- Status verify: every function in 0x2437f8..0x249b88 is written, but nothing can show it: main 0x2045a8 is still the placeholder (black screen), and the rxthread
- verify: `rx/rxsys.cpp` 74/119 -- Status verify: nothing calls rxSysInit yet - main 0x2045a8 is still the placeholder in src/main.cpp, so the screenshot is the black placeholder frame. Next: mai
- verify: `rx/rxthread.cpp` 53/53 -- Complete; verify because main (unassigned@001fd424) is still the placeholder, so nothing calls rxThInit in the smoke run (black frame as before). A temporary ca
- verify: `setup/i3d_draw_module.cpp` 0/0 -- Hook recorded in open-items Decisions (The native i3d hook). The committed screenshot is the forced test (ROSEPAN_I3D_TEST=1): S2DMAN.RPK @0x800 (PSMT8, Now Loa
- verify: `spr/s2d.cpp` 86/86 -- All 86 functions written; nothing on screen can show them yet (MagScene::draw is a stub), so verify: the first session whose screenshot shows S2d sprites marks 
- verify: `spr/sprman.cpp` 53/53 -- Verify: nothing calls SprMan::draw with m_active set yet. S2dMovie / S2dMes / S2dBg start only from MagScene::update 0x1327dc (stub), and textures never load (f
- verify: `spr/sprtexture.cpp` 21/21 -- Pinned 0x18f5b0..0x190a78. The code is complete; the smoke run reaches it (S2dMes / S2dMovie loadTexture) but every list ends in the error state because fileGet
- verify: `system/sysnowloading.cpp` 8/8 -- Code complete, nothing on the boot path shows it yet: draw 0x2051cc is called only by the pfh loader 0x1be564 (GamMan load path, unassigned@001bde58), a stub, s
- verify: `utl/utlmisc.cpp` 24/24 -- Verify: nothing calls this yet (main 0x2045a8 is still the placeholder; the smoke run shows a black frame). When main is reconstructed it calls utlMemInit and i

Recent sessions:

- 2026-10-22 18:35 `i3d/i3dmath.cpp` verify: i3d vector / matrix module pinned 0x236960..0x237ce8 and complete: all 95 functions (35 hidden ones created in Ghidra, batch i3dmath_001), both copies of the VU
- 2026-10-22 18:24 `spr/sprman.cpp` verify: SprMan complete: all 53 functions of 0x18bb8c..0x18f5b0 (init, draw, the motion / anim / fade / CLUT steppers, drawSprite to i3dPktSprite / TexSpriteUV, text in
- 2026-10-22 17:51 `dog/dogtexinfo.cpp` verify: DogTexInfo written whole: init / term / clear, find / findFree, add (VRAM desc), addData (TIM2 with refcount and heap copy), addPfh (pack or i3r-named textures,
- 2026-10-22 17:41 `setup/i3d_draw_module.cpp` verify: Native i3d draw module rosepan_i3d: the 2D packet writers record host primitives by packet address, the DMA-chain walk replays them in the ROM order; i3dTexRegs
- 2026-10-22 17:26 `spr/sprtexture.cpp` verify: SprTexture reconstructed: all 21 functions of 0x18f5b0..0x190a78 (ctor, clear, getters, setList, release, loadTex, the update and updateRead state machines, RTC
- 2026-10-22 17:09 `i3d/i3dtim2.cpp` verify: All 7 TIM2 helpers reconstructed: i3dTim2CheckHeader, GetPicture, IsClutOnly, GetMipSize, GetImage, GetClutCount, GetClut (two hidden functions created in Ghidr
- 2026-10-22 17:05 `i3d/i3dtex.cpp` verify: Pinned i3d/i3dtex.cpp 0x2184c0..0x21a120 out of unassigned@00216ad0 and wrote all 33 functions (4 hidden TEX1 setters created in Ghidra): texture and CLUT objec
- 2026-10-22 16:32 `i3d/i3denv.cpp` verify: i3d fog and lights: i3dFogInit, i3dCtxSetFog with its fog ramp and FOGCOL, the VU1 fog packet i3dPktMakeFog, i3dLightDescInit, i3dCtxCreateLight and its five ki

| TU (by frontier score) | state | done/stubs/total | KB | score | reached stubs |
|---|---|---|---|---|---|
| `unassigned@001bde58` | todo | 0/17/56 | 9.8 | 81 | 5 |
| `bed/bedpfh.cpp` | todo | 0/4/4 | 0.9 | 38 | 3 |
| `unassigned@00164fc0` | todo | 0/7/433 | 108.5 | 29 | 4 |
| `game/gamman.cpp` | todo | 0/5/409 | 82.8 | 26 | 4 |
| `mag/maglifeman.cpp` | todo | 0/3/10 | 1.5 | 24 | 3 |
| `unassigned@00228c08` | todo | 0/5/222 | 54.9 | 24 | 1 |
| `mag/magscene.cpp` | todo | 0/10/44 | 11.3 | 20 | 2 |
| `bed/bedpad.cpp` | todo | 0/5/5 | 0.9 | 20 | 2 |
| `mag/magscenelight.cpp` | todo | 0/5/8 | 2.3 | 19 | 2 |
| `unassigned@001fd424` | todo | 0/3/75 | 27.9 | 18 | 3 |
| `unassigned@0010f944` | todo | 0/4/12 | 2.2 | 18 | 2 |
| `dog/dogmodel.cpp` | todo | 0/10/69 | 14.6 | 17 | 1 |

## Frontier (called by reconstructed code, not written yet)

| RoR addr | name | call sites | provenance | state | inferred file |
|---|---|---|---|---|---|
| `00101750` | BedPfh::BedPfh | 12 | ror | stub | bed/bedpfh.cpp |
| `0022b228` | I3rRoot__CheckIdString | 10 | lib-ctx | stub | - |
| `001be524` | FileReader::read | 10 | ror | stub | - |
| `001019f0` | BedPfh::getFileIndex | 9 | ror | stub | bed/bedpfh.cpp |
| `001be2dc` | FileReader::~FileReader | 6 | ror | missing | - |
| `001be2cc` | FileReader::FileReader | 5 | ror | missing | - |
| `001be734` | FileReadReq::FileReadReq | 5 | ror | missing | - |
| `0021a828` | i3rCopyClipSphere | 5 | lib-ctx | stub | - |
| `001196d4` | MagLifeMan::add | 5 | ror | stub | mag/maglifeman.cpp |
| `001be4c8` | FileReader::isReady | 5 | ror | stub | - |
| `0013f980` | MagScene::setLightParam | 4 | ror | stub | mag/magscenelight.cpp |
| `00101ccc` | BedPad::update | 4 | ror | stub | bed/bedpad.cpp |
| `001c0254` | GamFont::getGlyphWidth | 4 | ror | stub | - |
| `0010ff74` | DogEntity::normalizeRotate | 3 | ror | stub | - |
| `00101b50` | BedPad::setVibration | 3 | ror | stub | bed/bedpad.cpp |
| `001be768` | fileGetSize | 3 | ror | stub | - |
| `0010fa6c` | DogEntity::getMatrix | 3 | ror | stub | - |
| `0017f200` | SndMan::update | 3 | ror | stub | - |
| `00119a38` | MagLifeMan::draw | 3 | ror | stub | mag/maglifeman.cpp |
| `0022b3d8` | i3rGetNodeIndexByName | 3 | lib-ctx | stub | - |
| `0021e1d8` | i3dPktDrawLineStrip | 3 | lib-ctx | stub | - |
| `00115cec` | DogModel::release | 3 | ror | stub | dog/dogmodel.cpp |
| `00215bb4` | dbgTraceVector4 | 3 | ror | stub | - |
| `00105f0c` | utlStrCopy2 | 3 | ror | stub | utl/utlfile.cpp |
| `001d3ac8` | GamMan::switchVolume | 2 | ror | stub | game/gamman.cpp |
| `0013f4d0` | MagScene::setLightNo | 2 | ror | stub | mag/magscenelight.cpp |
| `0022b428` | i3rGetNumTex | 2 | ror | stub | - |
| `0022b438` | i3rGetTexName | 2 | ror | stub | - |
| `0010559c` | utlTm2ToSepia | 2 | ror | stub | utl/utltm2.cpp |
| `0020e3d8` | seqSetLanguage | 2 | ror | stub | - |
| `001102b4` | DogEntityEx::setParent | 2 | ror | stub | dog/dogentity.cpp |
| `001bedc0` | GamFont::setTarget | 2 | ror | stub | - |
| `001bebec` | GamFont::setFont | 2 | ror | stub | - |
| `00142b18` | MagCharaLoader::load | 2 | ror | stub | mag/magloader.cpp |
| `00228c38` | i3dMatGetScaleTrans | 2 | lib-ctx | stub | - |
| `001bffcc` | GamFont::getGlyph | 2 | ror | stub | - |
| `00104e1c` | utlTm2GetSize | 2 | ror | stub | - |
| `00105e50` | utlStrCopy | 2 | ror | stub | utl/utlfile.cpp |
| `0021db18` | i3dPktDrawStrip | 2 | lib-ctx | stub | - |
| `001bf024` | GamFont::drawChar | 2 | ror | stub | - |

