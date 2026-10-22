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
6. (2026-10-22) **The cdrom driver reads ISO 9660 sectors itself** (architecture.md "The rx file
   layer"), so a CDROM boot needs sceCdRead to serve a disc *image*: src/sdk/libcdvd.cpp still reads
   FF3's IMG_BD.BIN. Decide (the user's call: platform): point sceCdRead at an `.iso` named by
   `ROSEPAN_DATA`, or boot from host0: (the HOST driver reads loose files through sceOpen) and leave
   the cdrom driver for real-disc parity only.

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
- The data folder: no RoR disc dump was found near the exe; the smoke run starts with the fallback
  root. Set `ROSEPAN_DATA` (build.py passes it as `ROSEPAN_DATA_DIR`) to the disc root with `DATA/`.

## Open questions

- **The disc data**: where is the dump (`ROSEPAN_DATA`), and what are RoR's archive formats? The ELF
  reads `cdrom0:\DATA\...` and `data/...` paths (architecture.md "Disc and data files"). Needed by M1.
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
  means; whether the cdrom driver turns names into ISO form with "\" and ";1"; who sets the HDD error
  hook 0x2dc248 (no setter found in rxfile's range); how the platform's sceOpen resolves `cdrom0:`
  paths (`RosePan_PathResolveData`; check once main runs).

## Done

- 2026-10-22: `rx/rxthread.cpp` (0x249b88..0x24b490): all 61 functions written (`verify`; 11 were
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
