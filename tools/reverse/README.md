# RosePan reversing tools

Rule of Rose (US retail `SLUS_214.48`, July 2006) ships without symbols for its `.text`, but the ELF keeps
its section header table: every inline method and template instance sits in a named
`.gnu.linkonce.t.<mangled symbol>` section (1,449), every vtable in `.gnu.linkonce.d._vt$<class>` (284),
and 73 `.cpp` source paths survive in assert strings. Its libraries are the Sony SDK 3.0 objects FF3 links
and the ee-gcc 2.96 runtime FF2's debug build has. These tools turn that into names, files and
translation units, and drive the one-TU-per-session port. They were carried over from ReiPan (Fatal
Frame 3), whose game-code matcher has no counterpart here: RoR has no sister game with symbols.

Run every tool from this directory: `python <tool>.py`. Paths come from `rosepan.py` and can be
overridden with environment variables:

| variable | default |
|---|---|
| `ROSEPAN_ELF` | `<repo>/SLUS_214.48` -- not distributed; dump it from your own disc |
| `ROSEPAN_PROGRAM` | `SLUS_214.48` (the program as Ghidra names it; project path `/Rule of Rose - US`) |
| `GHIDRA_MCP_PROJECT`, `GHIDRA_MCP_PORTS` | `data/ghidra.json`: the shared project `C:\Users\willi\Ghidra`, ports `8089:16` (8089..8104); the tools use whichever Ghidra holds the project |
| `GHIDRA_MCP_URL` | unset; pins one instance (`http://127.0.0.1:8089`) and skips the discovery |
| `ROSEPAN_REIPAN` / `ROSEPAN_MIOPAN` | `../ReiPan`, `../MioPan` (library references for libmatch.py, platform layer source) |
| `ROSEPAN_DATA` | the unpacked disc (unset until the data layout is known) |
| `ROSEPAN_MINGW_BIN`, `ROSEPAN_NINJA`, `ROSEPAN_VCVARS` | CLion's MinGW, Python's ninja, vswhere's vcvars64.bat |
| `ROSEPAN_CLAUDE` | the claude executable for autopilot.py |

## Pipeline

```
python fetch.py            # Ghidra -> work/hash_ror.json, work/ror_tags.json            (read-only)
python features.py         # ELF -> work/feat_ror.json: calls, tail jumps, strings, data refs (offline)
python sections.py         # ELF symbols -> work/sections.json; --apply creates + names them  (once)
python libmatch.py         # library names from reference ELFs; look at the plan, then --apply (once)
python rtti.py             # vtables -> work/classes.json: classes, bases, slots, ctors, vptr offset
python export.py           # file inference -> <repo>/ror_functions.txt
python status.py           # -> .claude/skills/rosepan-port/references/status.md
python refresh.py          # all of the above that is not a one-time write, in order
```

`sections.py --apply` and `libmatch.py --apply` are the one-time Ghidra seeding (host task
`setup/ghidra_seed.cpp`); take `python restore_names.py --snapshot` first.

## Tools

| script | what it does |
|---|---|
| `rosepan.py` | Paths, the address map (`data/layout.json`), the GhidraMCP client (`program` always in the query string) and its write helpers (create with lock retry, rename, tag, save with retry). Import it; not a CLI. |
| `ghidra_mcp.py` | Finds the Ghidra that holds the shared project among every Ghidra running (one per game: GhidraMCP binds 8089, or the next free port up to 8104 when another Ghidra has it) and talks to it; `rosepan.http()` / `ghidra_up()` go through it. `list` (every instance, `*` = RosePan's), `which` (what the tools resolve to, or why none), `launch` (start Ghidra on the project unless it runs). Config `data/ghidra.json`; the resolved port is cached in `work/ghidra_instance.json`. Same module as YamiPan's (`YamiPan/.claude/skills/yamipan-port/references/ghidra.md` has how the plugin serves). |
| `ghidra_bridge.py` | GhidraMCP's `bridge_mcp_ghidra.py` with start-up, `list_instances`, `connect_instance` and reconnect replaced by `ghidra_mcp.py`'s discovery, for a `ghidra` MCP entry (run with the bridge's venv python, `data/ghidra.json` `bridge_python`). RosePan has no such entry yet. |
| `elf32.py` | Minimal ELF32 reader: sections (with names), segments, VA->bytes, `_gp` from `.reginfo`. |
| `gcc2.py` | GCC 2.x name mangling: `demangle()` / `ghidra_name()` (the project's Ghidra spelling) / `mangle()`; `--selftest` demangles every named section (1,733/1,733). Mangling has no T/N-safe guarantee for repeated class params; `struct X` forces the by-value `G` marker. |
| `fetch.py` | Function starts, names and hashes, and every function's provenance tag, from Ghidra. |
| `features.py` | Per-function features decoded from the ELF: call order, tail jumps (`j` out of the body), function pointers, strings, floats, data refs, instruction mix. Adds the ELF's named sections Ghidra has no function for yet, and their names over Ghidra's `FUN_`. |
| `sections.py` | The ELF's own symbols. Plan (default), `--list [FILTER]`, `--layout` (satellite ranges), `--apply`: create functions at every named section, vtable slot target, `.ctors` / `.dtors` entry and tail-jump target that follows a `jr ra` (code Ghidra folded into its callers); name + tag the sections `ror-symbol`; save; refresh. Idempotent: run it again after anything that adds tail jumps. |
| `libmatch.py` | Library / runtime names: each reference library function (FF3's SDK 3.0 libs from ReiPan's `ff3_functions.txt`; FF2's runtime, newlib and SDK 2.8 from MioPan's `symbols.txt`) compared word for word with relocations masked, at RoR's function starts and after every `jr ra`. Unique matches only; identical bodies under several names are settled by their callees; FF2's duplicate statics get their file's stem (`kCopy_exit`); a match whose body holds a function claiming the same name (a merged FF3 body) and the weaker of two claims on one name are skipped; hand-named addresses are never touched. `--list` (`SKIP:` marks the skipped), `--ranges`, `--apply`. Applied 2026-10-22: 544 high, 133 low. |
| `rtti.py` | Classes from the named vtables (GCC 2 ABI: 8-byte `{delta, index, pfn}` entries, entry 0 the `__tf` type-info function): bases (the `__tf` functions each calls), slots with current names, ctors / dtors (functions that store the vtable) and the **vptr offset** those stores use. `--class NAME`, `--vtable ADDR`. |
| `export.py` | `ror_functions.txt`: every function with size, name and provenance, grouped into source files: pinned ranges, then string anchors (RoR's own `dir/file.cpp`), class anchors (a class's own methods / overridden slots / ctors follow its string-anchored file), then link-order fill. Writes `work/class_files.json`. |
| `status.py` | The generated status page (seeding state, naming, placement, reconstruction, TUs, frontier). |
| `refresh.py` | fetch + features (if Ghidra is up), sections, rtti, export, status. |
| `tus.py` | Translation units, one per session: `list`, `next [--claim]` (active, pinned queue, partial, frontier, `main`), `claim`, `brief` (handoff notes, identity evidence, classes, slice, calls out, library calls, neighbours + staged bundle), `define`, `queue [--task TEXT]` (host tasks), `check` (build, stubgen, callcheck, layoutcheck, smoke run with `settings.smoke_pad`), `finish` (record, refresh, commit; copies the last shot to `progress/`), `mark <tu> done|partial|... --why TEXT` (set another TU's state from what a run showed, kept in its history; marking verify TUs done from screenshots). State in `data/tus.json`; `settings.frontier_ignore` lists callees the frontier skips (the assert handler 0x215a30, replaced by `ROR_ASSERT`). |
| `tuid.py` | Is the TU the file its key says? Source-path strings (anchored count), other strings, classes (and where they were placed), name prefixes, caller / callee TUs. In every brief. |
| `stage.py` | `python stage.py <names\|addrs\|file:x.cpp\|class:Name\|range:lo-hi> [--asm]` -- per-function bundle in `work/stage/`: demangled ELF signature, class role and vptr offset, callers, callees, tail jumps, strings, floats, decompilation. |
| `ctxcard.py` | `--top N` most-called unnamed game functions (or given addresses): callers, callees, tail jumps, strings (asserts name fields), floats, class role, named neighbours with files, decompilation. |
| `name_ctx.py` | `python name_ctx.py <batch.json>` -- apply hand names (`ror-inferred` / `lib-context`) with evidence to Ghidra and `data/context_names.json`; refuses whitespace and duplicates, never renames a `ror-symbol`; replacing a library match records `data/overrides.json`; saves, then refresh.py. |
| `modview.py` | `range <lo> <hi>` / `tree <func> [depth]` -- one line per function: name, provenance, size, call sites, callees, strings. Offline. |
| `dec.py` | `python dec.py <addr\|name> ...` -- decompile over HTTP. |
| `ghidra.py` | `asm` (read-only disassembly of any range), `func`, `xrefs`, `refs` (xrefs + `jal`/`j` encodings + data words + low halves: "is it really unreferenced"), `find`, `segments`, `mkfunc` (create + save, lock retry). |
| `hidden.py` | `python hidden.py <lo> <hi>` -- functions Ghidra folded into the body before them: code after a return (`jr ra`, `j`, or an assert's `b .` spin) nothing branches to, `jal` targets that start no function. Run it over a TU's range before naming; then `ghidra.py mkfunc`. |
| `mipsdis.py` | Offline R5900 disassembler over the ELF, annotated with cached names; for code Ghidra has no function for. |
| `vudis.py` | `python vudis.py <image addr> <qwords> <micro addr> [...] \| --all` -- VU microcode disassembler: follows a microprogram image's VIF MPG codes in the ELF and prints upper \| lower instructions from each address to its [E] bit (vcallms takes the instruction index). The i3d VU0 image is `2949f0 9c`. |
| `mipsemu.py` | A tiny EE integer interpreter: run a leaf function over every input to check a C rewrite of a switch-table function; `Emu` with load / call hooks for non-leaf ones. |
| `i3racc.py` | `python i3racc.py list / mkfunc / batch <out> / cpp / apply` -- the 288 per-type i3r item accessors after the fixup pairs in i3d/i3ritem.cpp (0x22c5a0..0x22e840): matches each against ten exact instruction shapes (slot k), creates the hidden ones in Ghidra, writes the name batch and the C++ in i3rtype.cpp's style. A template for other families of generated one-liners. |
| `sinit.py` | Run a static initializer's straight-line stores symbolically and dump the table it fills. |
| `datadump.py` | `python datadump.py <addr> <count> "<fields>"` -- a `.data` / `.rodata` table as host C++ initializers (pointers printed as the symbols they point at; `S` strings as byte-exact C literals with `\x` escapes, up to 4 KB). `--words <lo> <hi> --var NAME`: the range as a raw EE word image (`const u_int NAME[]`, 8 per line, the words that point into the range listed) for tables whose reader is not written yet. |
| `tm2png.py` | TIM2 -> PNG (4/8-bit indexed with CSM1 CLUTs, 16/24/32-bit, PS2 alpha scaled): files, or `--scan` to carve every embedded TIM2 out of an archive. The reference a drawn screen is checked against. |
| `iop.py` | The IOP modules in `<game data>/MODULES`: `report` (default: every image and `.irx`, module name / version / code size, whether `iopconf.cnf` loads it, imports per library), `list IMG` (a ROMDIR image's entries with EXTINFO dates / versions), `extract [IMG]` (unpack `IOPRP*.IMG` to `work/iop/<img>/`), `irx FILE [--stubs]` (`.iopmod`, sections, imports named from PCSX2's `pcsx2/IopModuleNames.cpp` via `rosepan.PCSX2`, exports), `elf FILE [--base A]` (a linked MIPS R3000 ELF in `work/iop/elf/<file>.elf`: image at A, relocations applied when A != 0, import stubs / export entries / `_start` / `_gp` as symbols, ready for Ghidra's ELF loader). |
| `iopdec.py` | RXSNDDRV offline: `dump` decompiles every function of the `rxsnddrv.elf` program into `work/iop/dec/<addr>.c` plus `index.txt` (address, extent, name, FUN_ callees, printed strings; needs Ghidra), `show ADDR ...` and `index [FILTER]` read the cache offline. It sets `ROSEPAN_PROGRAM=rxsnddrv.elf` itself and never touches the SLUS caches or the name ledger. |
| `restore_names.py` | `--snapshot` every name before bulk writes; restore from a snapshot. |
| `build.py` | Build (MinGW + ninja, configures on first use, reuses ReiPan's / MioPan's fetched deps; refuses raw NUL bytes in `src/`, which MinGW accepts and MSVC does not), `--run N [--pad S] [--shot-every F] [--env K=V]` smoke run with screenshots and stub hits. Needs the bootstrapped PC project. |
| `stubgen.py` | Undefined references -> `ROSEPAN_STUB` definitions in their TU's file (address from a trailing `// ROR 0x...` on the declaration, else the Ghidra name, also `Class::Method`). |
| `callcheck.py` | Calls in each `ROR 0x...`-annotated function vs RoR's callees (jal + tail jumps), by name's last component or the source definition at that address; `// callcheck: A, B (why)` explains invisible calls (names may end in [], e.g. operator_new[] for a C++ new[]). |
| `layoutcheck.py` | Every game header on its own under MinGW GCC and MSVC so both evaluate the `static_assert` layouts; lints structs without asserts (or a `layout: host` note), plain `long`, `wchar_t`, `packed`. |
| `audiocheck.py` | The sound test without a listener: reads the raw mix a run dumps (`build.py --run N --env ROSEPAN_AUDIO_DUMP=<abs path>/work/audio/mix.raw`), prints RMS / peak dBFS per interval (`--every S`) and the stretches of sound and silence, writes `mix.wav` for the user to listen to. Audio time is wall time, not frames. |
| `gdbrun.py` / `gdbdrive.py` | The game under CLion's gdb: a fault holds it and writes `work/gdb/crash.txt`; `--cmd` inspects, `--break` stops a hang. |
| `autopilot.py` | The unattended chain: one headless `claude -p "/rosepan-port tu <TU>"` per TU on a fixed allow-list, verified (finished, committed, clean, green), usage limits waited out and the session resumed. Run it yourself from a terminal. |
| `routine.md` | The prompt for the desktop app's scheduled task `rosepan-next-tu` (preflight, one TU, push once approved). |

## Provenance tags

| tag | meaning |
|---|---|
| `ror-symbol` | RoR's own symbol: the ELF names the function's linkonce section. Authoritative; never renamed. |
| `lib-match` | Library / runtime code matched word for word (relocations masked) to a symbolised reference. |
| `lib-match-low` | A short body (3..5 instructions), no clean function end, or names the references disagree on: a hint. |
| `lib-context` | A library / runtime function named by reading it. |
| `ror-inferred` | A game function named from its code and context (asserts, strings, vtable, callers). |

`data/` holds what must survive: `tus.json` (TU state, queue, settings), `layout.json` (library and
satellite ranges), `context_names.json` (hand names with evidence), `overrides.json` (library matches
replaced by hand), `lib_matches.json` (applied library names).

## Ghidra naming convention

Ghidra names are C++ names as written in source: `DogEntity::reset`, `DogEntity::~DogEntity`,
`type_info::operator!=`; free functions plain (`main`). Ghidra keeps `::` literally in a flat name (no
namespaces), accepts templates and `$`, and rejects whitespace (`operator new` -> `operator_new`). Two
functions with the same qualified name carry their parameters without spaces:
`MagLifeWindow::processPad(BedPad const&,BedPad const&)`. `ror_functions.txt`, briefs and callcheck
read these names; `gcc2.py` turns the ELF's mangled symbols into them.

## Platform contract (what the tools need from the PC project)

The bootstrap host task copies ReiPan's platform layer; these hooks must survive the rename:

| hook | where (ReiPan) | used by |
|---|---|---|
| `cmake-build-<config>/RosePan/RosePan.exe`, target `RosePan`, `compile_commands.json` | CMakeLists.txt | build.py, layoutcheck.py, gdbrun.py |
| `ROSEPAN_SHOT_DIR` / `ROSEPAN_SHOT_EVERY`: save every Nth presented frame as `shot_<frame>.png` + `last.png` | rendering/reipan_renderer.cpp | build.py --run, tus.py finish (progress/) |
| `ROSEPAN_AUTOPAD`: scripted pad 0, `<read>:<BUTTON>[*<reads>],...`, host input masked | sdk/libpad.cpp | build.py --pad, settings.smoke_pad |
| `ROSEPAN_DATA_DIR`: the game's data folder | io/reipan_paths.cpp | build.py ($ROSEPAN_DATA) |
| `ROSEPAN_SKIP_INTRO`: 0 / 1 over rosepan.ini's `[game] skip_intro` (the boot logos and movies) | rosepan_config.cpp, mnu/mnutitle.cpp | build.py / gdbrun.py set 0 unless given, so the pad's timing holds |
| `ROSEPAN_RXSNDDRV_LOG=all`: log every RXSNDDRV RPC command (not the per-frame status polls 0x72 / 0x83 / 0xa3 / 0xb3) and every ADPCM stream start / underrun / end, instead of the first few | iop/rxsnddrv/rpc.cpp, adpcm.cpp | the run log |
| `ROSEPAN_AUDIO_DUMP`: a file path; the host mixer appends its output there as raw s16 stereo 48 kHz (1 MB buffered, so a killed run loses the last ~5 s) | sdk/iop_voice.cpp | audiocheck.py |
| `ROSEPAN_GDB_DIR`, the game thread named `RosePanGame` | os/reipan_host.cpp | gdbrun.py / gdbdrive.py |
| `[stub] <name> (ROR 0x%08x) reached` from `ROSEPAN_STUB` / `ROSEPAN_BRIDGE` | common/reipan_stub.h | tus.py (reached stubs rank first), status.py, stubgen.py |
| log lines `[W]` / `[E]` for warnings and errors on stdout | io/reipan_log.cpp | build.py --run summary |

## Things that bit (carried from ReiPan; RoR-specific ones in the skill's hazards.md)

- **`program` must be in the query string.** GhidraMCP ignores it in a POST body and writes to the active
  program. `rosepan.http` handles it. `dry_run` in a POST body is not honoured: never probe writes.
- **Writes collide with saves** ("Unable to lock due to active transaction"): `rosepan.create_function`
  and `save_program` retry; check with `ghidra.py func` afterwards.
- **Never pipe `name_ctx.py` / `refresh.py` into `head`**: the closed pipe kills the refresh and leaves
  stale names behind (stubs at address 0, callcheck "missing" calls).
- `python -I` drops the script directory from `sys.path`; run the tools as `python tool.py` from here.
