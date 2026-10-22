# Open items -- priorities, decisions, cross-TU questions, done log

Edit this as work happens: move items to Done with the commit, add questions when something is unclear,
delete what is obsolete. Per-TU state and handoff notes live in `tools/reverse/data/tus.json`
(`tus.py`); `status.md` (generated) has the numbers and the next pick; this file has the judgement.

## Next up (in order; the pinned queue in tus.json encodes it, `tus.py next` follows it)

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
   - ~~**`mag/magsystem.cpp`**~~ (2026-10-22, `partial`): 40 of 52 written -- the frame runs on
     MagSystem's V-blank semaphore at 30 fps (m_v_interval 2) and the PC `sceGsSyncV(0)` in
     `mainLoop` is gone. Left: the 12 functions on the scene's actor array (see item 9).
   - `game/gamman.cpp` (`GamMan::init`, `loadSystemData`, `loadStage`, `update`), the globals file
     (unassigned@001fd424's tail: `sysInitManagers`, `SysTimer`; it owns the global objects now kept in
     lib/unassigned_stubs.cpp), the task manager (unassigned@001e46b8), the sequence engine
     (unassigned@0020eda4, 65 functions: an overlay-style loader of 0x18000..0x30000-byte images and a
     cooperative thread scheduler -- "mst", "trg", "imdy", "single").
   `-m N` on the RosePan command line picks the boot stage (mainParseArgs); without it the ROM's
   default is 99.

9. (2026-10-22, the magsystem session) **`mag/magsystem.cpp` is `partial`**: everything but the 12
   methods on the scene's 128 actors (0x2920 + no * 0x4d0, vptr +0x94 as DogEntity's) is written:
   0x117974 (actor +0x144), 0x118a18 (DogEntity 0x1102b4 + a virtual at +0x50 / +0x54, assert
   theNo >= 0), 0x118adc / 0x118c14 (the virtual at +0x88 / +0x8c: a 64-byte matrix of node n; asserts
   no / n), 0x118ccc / 0x118d9c (dog/dogmodel.h inline: m_i3r_root, 0x22b3d8 finds the node by name),
   0x118e6c / 0x118ea4 / 0x118f30 (DogActor 0x109348 / 0x10a038 / 0x109b88), 0x119234 (floats
   f12..f14 into actor +0x40, and 0x215b5c when no == m_pc_no_last), 0x1192c8 (created 2026-10-22:
   actor 0x10fc7c(x, y, z) and resets), 0x11939c (actor 0x10ff74 and resets). They need the actor
   class in the scene (`DogActor m_actors[128]`, host layout): do them with the first dog/ TU that
   defines DogEntity / DogModel / DogActor, or when a caller reaches them (game modes, not reached yet).
   The next reached stubs after this session, in run order: `MagScene::MagScene` / `init` / `start` /
   `initRender` / `update` / `draw` (mag/magscene2.cpp, magscenerender.cpp), the i3d library
   (unassigned@00216ad0: `i3dCtxBeginFrame` / `EndFrame` / `GetPacket`, two bridges), `BedPad::init` /
   `update` (bed/bedpad.cpp), `SndMan::update`, `RxMpeg::setFrameParity`, `rxMpegSetVCount`.

## Decisions

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

- **The ROM's global operator new / delete are not the host's** (decided in the utlmisc session,
  2026-10-22, unconfirmed with the user): 0x105b54 / 0x105b7c / 0x105ba4 / 0x105bc4 are written as
  `utlOperatorNew*` / `utlOperatorDelete*` in utl/utlmisc.cpp and do not replace `::operator new`.
  A new-expression in reconstructed code uses the host heap: host objects are bigger than the EE's,
  and the platform layer allocates before main creates the game heap. Revisit if game code relies on
  the game heap's bookkeeping (BedMemory::check, the debug tags).
- **Asserts**: `ROR_ASSERT(expr)` in `src/common/ror_assert.h` prints the ROM handler's line and
  continues (2026-10-22).

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

- **main.cpp open points** (2026-10-22): `mainInitGame` passes bss 0x7812ac to `seqSetLanguage` (role
  unknown; kept as `g_bss_7812ac`), and sets bss 0x7a3d48 / 0x7a3d4c = 1 and data 0x296198 = 2 after the
  boot load (`g_bss_*` / `g_data_*` in lib/unassigned.h until the game modes that read them are
  written). Class names `CmnTaskMan`, `SysTimer`, `SndMan`, `Sequence`, `MagScene` (for MagSystem's
  +0x12540 member) are inferred, not from the ELF: confirm or rename when their TUs are read.
  rxsys.cpp has a function Ghidra folded at 0x243620 (`j 0x248870`, between rxSysInitFileDevice and
  rxSysSetFileDevice; probably rxSysGetFileDevice) -- create it in Ghidra (`ghidra.py mkfunc`) when
  rxsys is next touched.

## Done

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
