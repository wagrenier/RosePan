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

## Done

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
