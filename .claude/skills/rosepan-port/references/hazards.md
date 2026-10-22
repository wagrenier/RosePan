# Hazards -- things that already cost time

Add an entry whenever something burns time and could again: symptom, cause, fix. Most entries below
were paid for on ReiPan (Fatal Frame 3, 2026-10-12..22) and MioPan (Fatal Frame 2); they are about
Ghidra, the EE, GCC, the host compilers and the platform layer, so they hold here. RoR-specific
entries are marked **RoR** with the date.

## RoR binary and naming (2026-10-22)

- **RoR: most inline methods have no Ghidra function.** Ghidra's analysis found 37 of the 1,449 named
  linkonce sections (nothing calls an inline virtual directly) and none of 95 vtable slot targets in
  .text. Until `sections.py --apply` runs, decompiles of neighbours can swallow them and `ghidra.py func`
  answers "No function found". features.py already treats the sections as functions offline.
- **RoR: Ghidra rejects whitespace in names**, not `::`, `<>`, `$`, `*`, `,` or `()` (tested on a scratch
  rename, reverted): `operator new` must be `operator_new`; gcc2.ghidra_name() does it.
- **RoR: two classes of "library" in FF2's symbol table are not libraries.** MioPan's symbols.txt also
  lists FF2's own game helpers without debug info (`CEditRoot::operator new`, `ReleaseSPUMemory`) and
  FF2's templates (`ctl::custom_allocator<...>`); FF2's game objects sit under both `/zero_rom/` and
  `/home/akira_koide/zero2np/`. libmatch.py keeps only crt0, the region from libexcep.c (0x27a998) up,
  and class-qualified names of the C++ runtime's own classes. Watch for this whenever a new reference
  joins REFERENCES.
- **RoR: identical library wrappers under two names** (`strtol` / `strtoul`, `vfprintf` / `vfiprintf`,
  `raise` / `__sigtramp`): their only difference is a jal target, which the mask hides. libmatch settles
  them by comparing callees with what RoR's callees were matched to; the rest stay lib-match-low with an
  `also` list: read the callee before trusting either name.
- **RoR: the ELF's section names are authoritative, Ghidra's `FUN_` is not.** Offline tools read names
  from features (Ghidra's + the sections); a tool that takes Ghidra's list alone shows `FUN_00280e88`
  for `DogEntity::__tf`. When comparing with "what Ghidra has", read `work/hash_ror.json` (sections.py
  `ghidra_functions()`), not cached names (features merges the sections in).
- **RoR: an inherited vtable slot is the base's method.** A derived class's vtable repeats its base's
  pointers for every slot it does not override; counting those as the derived class's members put
  DogActor in dog/dogmodel.cpp. Own slots = those that differ from the primary base's slot at that index
  (export.py).
- **RoR: code reached only by tail jumps was folded into its callers** (seeding, 2026-10-22). Symptom:
  nine `MagLifeMenu*::isInit` bodies ran past their linkonce section; `ghidra.py func` showed
  `Body: 001bdc94 - 00283f97` (an address set reaching back to the jump target) and the decompile showed
  the callee's field read inline (`return *(param_1 + 0x10c0)`). Cause: `j 0x1bdc94` with no function at
  0x1bdc94. Fix: a function at every tail-jump target that follows a `jr ra` (sections.py --apply does
  it; 21 created); targets that do not follow one are labels inside a function and stay. Creating the
  target shrinks the caller's body back by itself.
- **RoR: `get_function_by_address` answers for any address inside a body**, so "is there a function at
  X" must compare its `Entry:` with X (rosepan.has_function does).
- **RoR: the FF3 library reference has merged bodies.** ReiPan's Ghidra had kCopy + kFindAddress as one
  "kFindAddress", sceSifLoadElfPart + sceSifLoadElf as one "sceSifLoadElf", Copy + kCopy as one "kCopy"; matched
  word for word, such a name lands on the function *before* the real one, and the real one (matched
  from FF2) then collides with it. Many other FF3 bodies swallowed the *next* function under a correct
  name (FlushCache, the klib syscall stubs): harmless. libmatch skips a match when a function starting
  inside its body claims the same name, and keeps one address per name; FF2's duplicate statics get
  their file's stem (`kCopy_exit`).
- **RoR: the decompiler spells `::` as `__`** (`DogEntity___DogEntity` for `DogEntity::~DogEntity`) in
  its output, while Ghidra's names, ror_functions.txt and the tools keep `::`: grep for both.
- **RoR: header assert strings are not file anchors.** Inline header functions expanded into a .cpp
  carry `"dog/dogactor.h"`, `"bed/bedpfh.h"`: they say which headers a file includes, not which file the
  function is in. Only `.cpp` / `.c` strings anchor.

## Ghidra / GhidraMCP (HTTP on 127.0.0.1:8089)

- **`program` must be in the query string.** In a POST body it is ignored and the write lands on the
  active program -- this once renamed 69 FF2 functions in ReiPan. `rosepan.http()` does it right; raw
  curl must too.
- **`dry_run` is not a safe probe.** In a POST body it is ignored (the write happens); `create_function`
  ignored it even in the query string. Read endpoints are safe to probe; writes go through the tools,
  on purpose, and are saved by them. `disassemble_bytes` honours `dry_run=true` in the query string
  (ghidra.py asm relies on it).
- **Naming policy rejects names** ("Exit", token-subset collisions): pass `strict_mode: "off"`
  (rosepan.rename does). Renaming to `FUN_<addr>` makes the name default again.
- **"Unable to lock due to active transaction"**: a write or save while the previous save still holds
  the lock. The function is usually created anyway; check with `ghidra.py func`, retry the save
  (rosepan.save_program retries). Unsaved names are lost when Ghidra reloads: if a save failed, compare
  `data/context_names.json` with Ghidra before finishing and re-apply the difference.
- **`find` ignores the mask** (`search_byte_patterns`): search exact patterns; for every access to a gp
  global loop over the 32 rt registers.
- **gp-relative globals have no xrefs in Ghidra**: "no other writer" of an sdata global means nothing.
  RoR's gp = 0x30fff0; search the asm for the offset.
- **A bridge started before Ghidra**: `list_instances` is empty; `connect_instance("Ghidra")` connects.
- **Scripts are disabled** unless Ghidra runs with `GHIDRA_MCP_ALLOW_SCRIPTS=1`: the tools decode the
  ELF in Python instead.
- **Duplicate names are refused by name_ctx.py**; template spellings are fine in RoR's Ghidra (ReiPan saw
  a failure on one: always grep name_ctx's output for FAILED, and read its "named n/m" line).

## Functions Ghidra got wrong (the most common time sink in ReiPan)

- **A body swallows the unreferenced function after it.** Code reached only through data -- vtable
  slots, callback tables, dispatch tables, hook lists, step chains kept as function pointers, functions
  after a tail call -- was never made a function, so the previous function's body runs on over it. The
  decompile stops at the first `jr ra` while the callee list shows calls it never made, and callcheck
  reports them MISSING. Tells: a size in ror_functions.txt far above the decompile; a callback address
  that lands inside another function; a Body that starts before its Entry (`ghidra.py func`). Fix:
  `ghidra.py asm` the tail, find the prologue (`addiu sp,sp,-N`) after `jr ra; delay`, `ghidra.py refs`
  the address, `ghidra.py mkfunc` it, name it in a batch. Before reading a class, `ghidra.py func` every
  slot `rtti.py --class` lists; before reading a dispatcher, every word of its table.
- **Tail calls are not calls to Ghidra.** `j <func>` at a function's end is missing from Ghidra's callee
  list ("0 callers"); features.py records it as a tail jump and callcheck counts it. A `j` to an
  address with its own prologue inside a body is a separate function: mkfunc it. A run of tiny tail-call
  wrappers can decompile one call off: read `ghidra.py asm` for the whole run.
- **An argument register passed through untouched**: a callee reads a2 that its caller never sets, so
  the caller's caller's argument flows through -- give the function that parameter.
- **Ghidra drops arguments** to callees it typed `void f(void)`, and orders float arguments wrongly
  (EABI: floats in f12.., ints in a0..). Read the asm before every `jal` whose arguments matter.
- **Variadic unpackers**: GCC spills each variadic argument to an 8-byte slot; floats are promoted to
  double (`dptofp` reads). `va_arg(ap, double)` for those, `int` for words, `void *` for pointers.
- **Switch tables on bytes decompile into goto soup**: get the ROM's answer for every input with
  `mipsemu.py --grid`, compile the C rewrite with MinGW in a scratch harness and diff all pairs.
- **VU0 macro code** decompiles into `extraout_vfN` noise: reconstruct from the asm. `vaddz.x` adds only
  z (an x/z distance, not 3D); `vsqrt` is sqrt(|x|); vf0 = (0,0,0,1) and constant registers are set once
  at start-up -- decode the raw word before trusting a reading (MioPan skill "VU0 macro-mode idioms").
- **ROM loops that reuse their counter** (an inner loop's `li s1, N` in a delay slot): keep the jump,
  guard any pointer that can run past its array on the host.
- **A ROM table reader that stops at a null field misses rows**: end tables where the row id stops
  matching its index or the key is empty.

## File inference (which TU a function belongs to)

- **Inferred extents are guesses where no string anchors them.** Link-order fill between two anchors of
  one file is usually right; across files that RoR never names it is not. Settle the extent in the TU's
  first session and pin it (`tus.py define`); pinned ranges override inference.
- **`define` drops the whole inferred extent**: a pinned TU is exactly its ranges. Before defining a file
  that already has an inferred extent, read it from ror_functions.txt and pin all of it.
- **Renaming moves inferred boundaries**: export.py anchors classes on names, so a rename batch can move
  functions between TUs. Re-run `tus.Model().tu_of` for callees before writing their prototypes.
- **A queued `unassigned@` key goes stale** once a session defines that run under a file name; queue file
  keys when you can. `tus.py next` skips stale keys; an effectively empty queue means re-assess.
- **A stub inside a TU marked done is never picked**: `grep -rn "stub of ROR" src` and the run log's
  `[stub] ... reached` lines find them; claim the TU by name.

## Types and layouts

- **MinGW is not "GCC layout".** MinGW-w64 defaults to `-mms-bitfields`: a bitfield run that changes
  declared type opens a new unit on MinGW *and* MSVC, while EE GCC packs it.
- **`long` is 32-bit on both Windows compilers**; the EE's is 64-bit.
- **A plain `float[4]` is not a VU0 vector** (4- vs 16-aligned); offset asserts catch it.
- **Tail padding differs between the host compilers**: assert `>=` on derived host-layout classes.
- **A mid-struct pointer moves every later host offset**: asserts after it become `>=` bounds and the
  layout note says where host and EE part.
- **Allocations sized from the EE under-allocate host objects**: `new (operator_new(0x28)) T` copies
  the EE size; a class with a vptr and pointers is bigger on x64 and overwrites the next heap block.
  Use `sizeof(T)` with the EE size in a comment. Same for list containers that memcpy EE-sized records.
- **A caller's out-buffer sized from what it reads back**, not from what the callee writes, corrupts the
  host stack once the callee is real: size it from the callee's stores.
- **`param_1 != -0x240` in a decompile is a null test of `&obj->member`**: a sub-struct at that offset.
- **EE float literals are not always round-to-nearest**: read the bits, write hex floats.
- **EE float division by zero is not IEEE**: +-MAX, and 0 * MAX = 0; the host gives inf / NaN. Guard.

## Reconstruction bookkeeping

- **A stub that tolerated null becomes a member that does not.** When stubs turn into real members,
  check every caller the smoke run reaches for objects the host does not build yet; add `// PC:` guards.
- **Stubs that return a buffer pointer return null**, invisible until the next consumer is real: when a
  crash follows a stub returning `void *`, give that stub its arithmetic first.
- **A stub on a readiness path returning 0 hangs** the caller's loop (an event that never ends, a
  frozen camera, a black "Now Loading"): trace the wait (a temporary printf in the dispatcher with the
  command / state and result, deduplicated), then write the callee or return what the ROM returns in
  the common case.
- **A real function behind a polled stub can stall the boot**: the old stub answered "ready", the real
  one asks a library stub that answers 0. Check what callers poll before replacing a stub.
- **A file-scope object whose constructor is a stub runs that stub before main**: write the small
  constructor for real.
- **A zero placeholder table becomes a crash far from its cause** the moment code that reads it runs:
  extract tables from the ELF (`datadump.py`), never leave zero-filled stand-ins.
- **Callers' local prototypes of a stub disagree with the real signature**: everything builds as C++,
  so a local `void f(int)` is an overload and keeps linking to the stub. Grep `src/` for local
  declarations when a TU defines a function; include the header.
- **A name you pick may already exist at another address**: stubgen then resolves the new name to the
  other function's address. Read name_ctx's output; on a FAILED name pick another, delete the wrong stub,
  refresh, re-check.
- **Stale names after renaming**: stubgen, callcheck, stage and tus read cached `work/` data. name_ctx.py
  refreshes itself; after renaming by any other route run `refresh.py`. A stub written with address 0
  means the name was not in ror_functions.txt yet.
- **"No stub on that path was reached" can be wrong**: a helper behind a tail jump does not look like
  part of the path. Follow the draw call down to the renderer in the source and check each callee.
- **When a run loses ground after a TU**, diff the run log against a baseline run of the previous commit
  (`git stash -u`, build, run, `git stash pop`), and `git bisect` with a worktree per step when the
  cause is not obvious (judge each step by a measurement of the shots, e.g. mean brightness after a frame).

## callcheck / stubgen

- **A call the optimiser proves dead gets no stub.** `rxSysPrintf` calls `rxThIsDispatchDisabled` only
  when the static tty fd 0x2dba34 is >= 0; nothing in the build wrote it yet (its writer `rxSysOpenTty`
  is a stub), so GCC folded the branch and no undefined reference -- hence no stub -- appeared. It shows
  up once the writer is real; stubgen handles it then (2026-10-22).

- **Your own helpers hide RoR's calls**: a `static inline` wrapper of a RoR callee makes every caller
  report it MISSING. Write the inlined code out as the ROM has it.
- **`// callcheck:` syntax**: names first, separated by commas, one parenthesis last, all on the line
  (`// callcheck: A, B (why A; why B)`); `::`, `(` or `;` end the list; a wrapped line needs its own
  `// callcheck:`.
- **ALL-CAPS callees read as macros** and are skipped: explain a real call to one.
- **A goto label in column 0 breaks the definition parser**: indent labels.
- **A constructor with an initializer list parses as a function named after the base class**: its
  explanation still has to name the base constructor.
- **stubgen finds one declaration per line**: write prototype blocks one per line.
- **stubgen places stubs by inference**: a stub can land in a wrong or new file; move it, delete the
  stray file.

## Sessions

- **A session never launches other sessions or agents** (the auto-mode classifier blocks it as "Create
  Unsafe Agents"); the user starts each TU's session. Sessions also do not edit autopilot.py.
- **`tus.py finish` commits with `git add -A`**: any untracked file in the tree goes into the commit.
  Look at `git status` before finishing. Locked files make it fail (Visual Studio's `.vs/` is ignored);
  if finish fails after recording the session, remove the duplicate session entry from tus.json first.
- **Two sessions in one checkout race**: a scheduled run's `finish` swept another session's half-done
  files into its commit, and the other session's next commit reverted the TU. Do side work in a sibling
  worktree (`git worktree add ../RosePan-<name> -b <branch>`, configure its build with the main
  checkout's `_deps/*-src` as FETCHCONTENT_SOURCE_DIR_*, run tools with `ROSEPAN_ELF=<main>/SLUS_214.48`)
  and merge between TU sessions; the user tests the main checkout's build, so hand them the worktree's
  exe for a side fix.
- **Autopilot sessions run on a fixed allow-list**: writes under `.claude/` may be denied (put reference
  text in `--notes` under "For the references:"), no `rm`, no shell loops, no `git -C`. A denied call
  is final: use the listed tools.
- **No backticks in `--notes` / `--summary`**: the shell runs them.
- **Batch file numbers collide**: `ls work/batches` and take the highest number + 1 (work/ is
  gitignored, so an overwritten batch leaves no trace).

## Shell and Python on Windows

- **Heredocs mangle backslashes** (`\n` in C strings, regexes, `\\` in Python paths) and an apostrophe
  in a quoted heredoc breaks the Bash tool. Write code, JSON batches and scripts with the Write tool.
- **A NUL byte makes git treat a reference file as binary** and its line endings flip on every rewrite:
  never write raw control bytes into a .md or source; write escapes. A whole-file diff for a small edit
  means line endings changed.
- **Python text mode vs CRLF**: `open(p, encoding='utf-8', newline='')` and match the file's own line
  ends; the default Windows encoding is cp1252 (UTF-8 rules decode to mojibake, prints fail on
  Shift-JIS): tools call `sys.stdout.reconfigure(encoding='utf-8')`.
- **A long Bash command fails with `ENAMETOOLONG`** (a heredoc of hundreds of lines): nothing runs.
  Write the file first.
- **Never pipe `name_ctx.py` / `refresh.py` into `head`**: the closed pipe kills the refresh.
- `python -I` drops the script directory from `sys.path`: run the tools as `python tool.py` from
  tools/reverse.

## Build

- **MinGW must be on PATH** or `cc1plus` fails with no diagnostics -- use build.py.
- **shadercross file lock** ("being used by another process") on parallel shader builds: rerun
  (build.py retries once).
- `ld: cannot open output file RosePan.exe: Permission denied`: the game is still running.
- `-fno-strict-aliasing` is required (MioPan's release-only crash).
- **layoutcheck compiles every header on its own**: a header that relies on what its .c included first
  fails; give it its own includes.
- **The build never runs MSVC**; only layoutcheck does (syntax only): MSVC-only errors show up there.
- **A smoke run that exits at once with 0x20474343** is an uncaught C++ exception -- seen when another
  game process held the log file. Check `tasklist` for another RosePan.exe; never kill another
  session's process.
- **`exited early with code 3221225477`** is an access violation: `python gdbrun.py` (with the same pad)
  gives the backtrace; the game thread is RosePanGame.

- **Renaming ReiPan -> RosePan must cover the shaders' `.hlsli` includes** and all three spellings
  (`REIPAN_`, `ReiPan`, `reipan`): the `.hlsl` files were renamed but the `.hlsli` kept
  `ReiPanVu1*` / `REIPAN_*`, and shadercross failed with "unknown type name RosePanVu1LightImage"
  (2026-10-22, bootstrap). The prebuilt `.msl` / `.spv` keep old names in comments only.

## Platform layer (ReiPan's, written for FF2 and FF3)

- **SDK shims only answer what FF2 / FF3 asked.** RoR calls libraries they never did (libpad2, libdbc,
  libvib, the rx middleware's IOP use): a shim may return a placeholder. When RoR behaves as if hardware
  is missing, read the shim against Sony's semantics before blaming the reconstruction.
- **An EE wait on an IOP status never ends when nothing answers the RPC**: every SIF RPC RoR binds must
  be served by the host IOP, with the payload sizes the EE sends.
- **A missing memory-card directory must answer -4** (sceMcResNoEntry), not "0 entries".
- **Textured vertex colours above 0x80 brighten** (GS MODULATE: (Ct * Cf) >> 7, clamped): do not cap
  colours at 1.0 in host shaders.
- **The GS writes alpha-0 fragments** unless the alpha test rejects them; a host shader that discards
  `a <= 0` loses FIX-weighted blends.
- **Two GS origins**: packets built with the XYOFFSET constant take top-left pixels, packets built
  around 2048 take centre-relative coordinates. Derive a host quad's position from its packet's own
  constant.
- **Decode the primitive type from the GIF tag** before drawing a PC quad (strip vs fan vs sprite):
  `(tag >> 47) & 0x7ff`, type = low 3 bits.
- **SleepThread / SuspendThread semantics** on the host: the EE main thread needs its own wakeup
  counter; SuspendThread used as a mutex needs the platform's critical section.
- **Host bookkeeping keyed on a GS address must expire**: pages are reused by different owners (a work
  frame and a movie texture shared one page and the movie went black).
- **A host shader that replaces VU1 microcode drops the microcode's culling**: read the microcode for
  what it rejects, not only what it computes.
- **ELF data addresses are not in emulated RAM**: the port does not load the ELF image; RoR's statics
  are host statics. Rebuild such data as host tables (datadump.py) and pass host pointers.
- **A PSS movie froze for good after a hitch** in MioPan's decoder (fixed in ReiPan's copy): keep
  ReiPan's libmpeg_video.cpp.
- **Two host threads**: the OS main thread pumps SDL events, the game runs on its own thread; SDL
  "main thread only" calls go through the platform's run-on-main-thread helper.

## How to debug (the method that worked)

- **The picture is the test.** Logs, stub hits and callcheck show the code ran, not that it is right.
- **Measure before theorising** (MioPan): five plausible diagnoses of one invisible model were each
  disproved by the next measurement; the cause was one stub. Probes: capped, named with `__func__`,
  unfiltered (or say what they filter), per object not global -- and stripped before finishing.
- **Check the texture before the geometry**: a quad that reaches the renderer but shows nothing usually
  samples a page nothing uploaded (all zeros) -- find the uploader among the reached stubs.
- **A world-space debug line that draws nothing** may project through a camera the host never set: use
  the game's own camera matrices.
- **Timing-driven pad scripts drift**: pad reads count frames, load times move the game's frames; pad
  generously and confirm with a shot before blaming the code.
