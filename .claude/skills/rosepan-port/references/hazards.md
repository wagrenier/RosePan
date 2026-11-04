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

- **Register-shuffling tail calls lose their arguments** (2026-10-27, i3dPktDrawStrip 0x21db18):
  a stub that moves t0..t2 around and `j`s into a body decompiles as `void f(void) { body(); }`.
  Read the asm (`ghidra.py asm`) of the wrapper and of a call site for the real argument list.
- **vudis.py: MSCAL n is an instruction index, and entry 8 of the i3d VU1 image only dispatches**
  to the address in TOPS+0.x; find the real program in the writer's UNPACK S-32 word (0x712 for
  sprites). `--all` now prints every loaded run (it used to stop at the first gap).
- **A 2D draw on the host hides a mesh the PS2 kept in front** (2026-10-27, the picture book):
  symptom, the menu page sprite covers the book model; cause, the ROM Z-tests the page (TEST_1
  0x5000d, GEQUAL) at the far plane while the replay order draws it after the model; fix, VU1
  strips carry clip z (rosepan_i3d DrawStrip -> RosePan_RendererDrawImageTriangles2DDepth).

## Ghidra / GhidraMCP (HTTP on 127.0.0.1:8089..8104, found by project)

- **Several Ghidras, one per game.** The first to start binds 8089, the next ones 8090.., so 8089 is
  not RoR's by right: the tools find the instance holding `C:\Users\willi\Ghidra` (`ghidra_mcp.py`,
  `data/ghidra.json`); raw curl must take the port from `python ghidra_mcp.py which`. A project is
  open in one Ghidra only (its `.lock`); a plain `ghidraRun` opens the last project used, which may be
  another game's: start this one by its `.gpr` (`python ghidra_mcp.py launch`).

- **The IOP program and the EE caches** (2026-10-30, `rxsnddrv.elf` imported next to SLUS_214.48).
  `ROSEPAN_PROGRAM=rxsnddrv.elf` points every tool at the IOP driver, but the caches and the ledger are
  keyed by address alone: `name_ctx.py` (`data/context_names.json`), `fetch.py` / `features.py` /
  `export.py` / `refresh.py` (`work/*_ror.json`, `ror_tags.json`, status) and so `tus.py check / finish`
  would mix IOP addresses into the EE's data, and `ctxcard.py` / `stage.py` read the EE caches (wrong
  evidence for an IOP address). Safe with it: `dec.py` and `ghidra.py` (asm, func, xrefs, refs, find,
  segments; mkfunc writes to the program named). Anything else waits for the IOP pipeline decision
  (open-items.md, `setup/iop_rxsnddrv.cpp`); never run `tus.py finish` with the variable set.
- **Small constants look like addresses in `rxsnddrv.elf`.** It is loaded at 0 (the IRX's own link
  addresses), so any value below 0x38970 can be taken for a pointer: `sceSifRegisterRpc(&sd,
  &DAT_0000d000, ...)` is RPC number 0xd000, not data. Read such operands as numbers unless the
  code dereferences them.
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

- **Ghidra chains a void callee's v0** (2026-10-26): i3rBuildPackets' decompilation feeds each
  0x21fb50 call's "return" into the next, but the asm passes the same saved s2 (the buffer start)
  every time and 0x21fb50 never sets v0. When a decompiled loop threads a value through calls,
  read the asm for the argument register before porting the chain.
- **The brief's class role can be wrong for an unnamed function; rtti.py is the authority**
  (2026-10-26): the tsk_chr.cpp brief listed FUN_001edd90 as TskMoveAdj's constructor, but
  `rtti.py --class TskMoveSecta` shows it is TskMoveSecta's exec (slot 4). Check the vtable slots
  before writing a class's methods.
- **A dispatch table full of two-instruction functions.** The script native table data 0x2d90f8 has
  512 entries, 471 of them `jr ra; move v0,zero` with no Ghidra function, and four real handlers hidden
  inside the bodies before them (2026-10-25). Read the table words from the ELF, classify each target by
  its first words (return 0 / lui+ori return code / real code), `mkfunc` the real ones, and let one host
  function stand for the identical empty ones (seqcall.cpp seqFuncNone). Generate the C++ initializer
  from the classification instead of typing 512 entries.
- **RoR: the sequence VM's opcode handlers had no functions** (2026-10-25): three tables (data 0x2d9958
  primary, 0x2d9a58 SPECIAL, 0x2d9b08 COP1) point at 99 short handlers that Ghidra left as bare code
  (its "FUN_0020ff34" covered only the nop). The SPECIAL table is 0x2c entries: the COP1 table starts
  where SPECIAL funct 0x2c would be, so a dump of 64 SPECIAL entries shows FPU handlers. Dump each table
  from the ELF, `create_function` every target, name them from the table index.
- **A body swallows the unreferenced function after it.** Code reached only through data -- vtable
  slots, callback tables, dispatch tables, hook lists, step chains kept as function pointers, functions
  after a tail call -- was never made a function, so the previous function's body runs on over it. The
  decompile stops at the first `jr ra` while the callee list shows calls it never made, and callcheck
  reports them MISSING. Tells: a size in ror_functions.txt far above the decompile; a callback address
  that lands inside another function; a Body that starts before its Entry (`ghidra.py func`). Fix:
  `ghidra.py asm` the tail, find the prologue (`addiu sp,sp,-N`) after `jr ra; delay`, `ghidra.py refs`
  the address, `ghidra.py mkfunc` it, name it in a batch. Before reading a class, `ghidra.py func` every
  slot `rtti.py --class` lists; before reading a dispatcher, every word of its table.
  In RoR's libraries (i3d, 2026-10-22) it is common: `hidden.py <lo> <hi>` over i3d/i3dctx.cpp found
  7 functions (unreferenced setters, the default end hook, a layer callback), i3dvram 3 more. Run it
  on every pinned range before naming; a callback's body is also where Ghidra puts "From <addr> [no
  function]" xrefs.
  hidden.py misses entries that follow a `b` (not `jr ra`) or sit after a lone padding nop: in
  i3mpose 0x230688..0x231198 it reported 1 of 8 hidden quaternion functions (2026-10-28).
  Same in i3cquery (2026-10-29): the 0x28-byte cast entry 0x2331c8 sat after a `b` at the end of
  0x232be8's body; found only through `ghidra.py refs` of an uncalled prim test (an addiu lo16 hit).
  Since 2026-10-29 it flags code after a spin (a backward `b` over nops: the assert handlers'
  endless loop, which hid dbgTraceMatrix 0x215a70 and the memory card test 0x215634); other `b`s land
  on jump-table cases, so they stay unflagged -- unless the code after the `b` (nops skipped) is a frame
  setup `addiu sp, sp, -N`: since 2026-10-29 that is flagged too (i3dgeom: 0x236348 and 0x236870 after
  a `b` into the epilogue, and 0x233de8, code in no function at all, all missed before). A callcheck
  MISSING on a tiny wrapper whose Ghidra size is too big is the same trap. Over a
  body far bigger than its decompile, `mipsdis.py` it and look for every `addiu sp,sp,-N` prologue.
- **Tail calls are not calls to Ghidra.** `j <func>` at a function's end is missing from Ghidra's callee
  list ("0 callers"); features.py records it as a tail jump and callcheck counts it. A `j` to an
  address with its own prologue inside a body is a separate function: mkfunc it. A run of tiny tail-call
  wrappers can decompile one call off: read `ghidra.py asm` for the whole run.
- **An argument register passed through untouched**: a callee reads a2 that its caller never sets, so
  the caller's caller's argument flows through -- give the function that parameter.
- **Ghidra drops arguments** to callees it typed `void f(void)`, and orders float arguments wrongly
  (EABI: floats in f12.., ints in a0..). Read the asm before every `jal` whose arguments matter.
- **A stubbed poll turns a step-per-call loader into a hang.** Loaders that step a state machine per
  call (SprTexture::update / updateRead) wait on helpers such as FileReader::isReady; stubgen's `return
  {}` reads as "not ready" for ever, and callers that spin (`while (update() == 0)`) hang the boot.
  Fix: make the polled helpers ROM-code bridges in the same session (spr/sprtexture.cpp did for
  0x1be338..0x1be4c8), and check every stub a new state machine polls before replacing a bridge.
- **A member object GCC 2 never constructs** (BedPfh inside SprTexture: the ROM's constructor only
  calls BedPfh::clear) has no default constructor in C++ when its class declares one with arguments.
  Give it an empty inline `T() {}` with a PC: comment rather than calling the stubbed one.
- **Variadic unpackers**: GCC spills each variadic argument to an 8-byte slot; floats are promoted to
  double (`dptofp` reads). `va_arg(ap, double)` for those, `int` for words, `void *` for pointers.
- **Switch tables on bytes decompile into goto soup**: get the ROM's answer for every input with
  `mipsemu.py --grid`, compile the C rewrite with MinGW in a scratch harness and diff all pairs.
- **"Could not recover jumptable ... Too many branches"** (TskFrameFunc::TskFrameFunc 0x1f184c, a
  0xe98-byte switch): the decompile shows only the dispatch. run_script_inline (a JumpTable override)
  is disabled in GhidraMCP. Read the table words from the ELF (data 0x2fe420), then `ghidra.py asm`
  the whole body to a scratch file and read each case; big cases are mostly soft-double angle wraps
  (fptodp / dpcmp / dpadd / dpsub / dptofp around +-pi constants) that collapse to chrWrapPi.
- **VU0 macro code** decompiles into `extraout_vfN` noise: reconstruct from the asm. `vaddz.x` adds only
  z (an x/z distance, not 3D); `vsqrt` is sqrt(|x|); vf0 = (0,0,0,1) and constant registers are set once
  at start-up -- decode the raw word before trusting a reading (MioPan skill "VU0 macro-mode idioms").
- **ROM loops that reuse their counter** (an inner loop's `li s1, N` in a delay slot): keep the jump,
  guard any pointer that can run past its array on the host.
- **A ROM table reader that stops at a null field misses rows**: end tables where the row id stops
  matching its index or the key is empty.
- **Function-pointer tables filled in code** (rxfile's driver init functions, 2026-10-22) point at
  `LAB_` addresses: two-instruction slot functions (`jr ra; li v0,-2`, a getter) were never made
  functions, and one thread entry (0x2495e0) sat inside the previous body. Every `LAB_` stored into a
  table in a decompile is a function: `ghidra.py mkfunc` it before naming the table's slots.
- **Tiny wrappers folded into the function before them** (rxfile, 2026-10-22: 13 entries such as
  0x243858 close-with-abort, 0x2438b8 state, 0x243e08 / 0x244178 / 0x2441d0 / 0x244230 / 0x244288
  slot-check wrappers, 0x2457e8, 0x247ea0, 0x247fd0, 0x248008, 0x249ad0, 0x249b00). rx writes many
  `if (fd valid) return f(slot, ...)` wrappers reached only from other modules' code Ghidra had not
  linked; the decompile of the body before them is short and its size is not. `python hidden.py <lo>
  <hi>` over the TU's range lists them; mkfunc every one before naming. Ghidra also typed some of their
  targets `void f(void)` and dropped the arguments (0x243da8 -> 0x245078): read the asm.
- **VU0 helpers that pass values in VU registers** (i3d/i3mpose.cpp, 2026-10-27): 0x22f870 (Hermite),
  0x22fc38 / 0x230258 (slerps) take vf1 / vf2 / vf3.x and return vf1; Ghidra shows them as
  `void f(float)` with `in_vf1` reads, and the caller's decompile drops the hand-off. Read the
  caller's asm for which lqc2 fills vf1 / vf2 before the jal; write them as pointer functions with
  a PC: note. Likewise the i3m float-argument functions (t, r in f12..f14, pointers in a0..t2):
  Ghidra's parameter lists for them were wrong in every pose function -- take the order from the asm.
- **A ROM read of the byte after a copy** (`rxFilePathNextElem` 0x248e98 tests `*out == '.'` after
  copying, i.e. the uninitialised next byte): keep the quirk with a comment, it only fires on stale
  buffer contents.

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
- **"Every XxxMain::initOut opens its enemy file" is not a rule** (2026-10-29): Mrt .. Grg do, but Fsh puts
  FshMain's destructor 0x155d54 and its other vtable slots before FshMain::initOut 0x156620. End an
  enemy file at the next class's first vtable slot or initOut, whichever is lower (`rtti.py --class`). Dwf does the same
  (dtor 0x15c2a4 before initOut 0x15c304); Got and Pig open with initOut, their dtor right after.
- **A stub inside a TU marked done is never picked**: `grep -rn "stub of ROR" src` and the run log's
  `[stub] ... reached` lines find them; claim the TU by name.

## Types and layouts

- **A DogModel inline hidden by a DogActor overload** (2026-10-29, mag/magsceneif.cpp): code that
  inlines DogModel::get_nNode() / getNodeName(i) on an actor fails to build ("no matching function
  for DogActor::get_nNode()"), because DogActor declares get_nNode(int) / getNodeName(int, int) and
  C++ name lookup stops there. The ROM's asserts name dog/dogmodel.h, so it is the base inline: call
  it qualified, actor.DogModel::get_nNode().

- **A game pointer stored into an i3r file word** (2026-10-27, utl/utlmath.cpp): the EE writes an
  address straight into a relocated field (a node slot's m_data = &DogModel::m_disp_control). On the
  host relocated fields are self-relative (i3rroot.h), so a raw store would truncate a 64-bit pointer
  and the reader would add it to the field address. Fix: store with i3rSetPtr, read with getData();
  the target must lie within 2 GB of the file (game-heap objects do; a static global may not).

- **Lower-bound asserts need the fields before them** (2026-10-27, EmyMain): a host-layout class that
  places only the members its code reads (EmyMain: 25 of ~0x200 bytes) fails `offsetof(...) >= 0x1c8`
  for late fields. Assert lower bounds only where the members before are complete; leave the rest to
  the EE offsets in the comments.

- **A param struct smaller than what the getter copies** (2026-10-27): GamObjColli was declared 0x28
  bytes (the fields the callers read), but ObjStat::getParam(0x18 / 0x1005) copies m_colPrm, 12 words
  (0x30): five host callers overflowed their stack local by 8 bytes. Fix: size a getter's out struct
  from the copy in the getter (memcpy size / the member's array), not from the fields used; GamObjColli
  is 0x30 now.
- **Ghidra's `ptr + n` on an `int *` is n words, not n bytes.** Symptom: a table walk reads
  `piVar1 = *(int **)(this + 0x110) + 0xc;` and the row size is taken as 12. Cause: the decompiler
  prints typed pointer arithmetic; on `int *` + 0xc is 0x30 bytes. Fix: read the row size from the
  asm (`addiu v, v, 0x30`) or a raw dump of the table (`datadump.py --words`) before declaring the
  row struct (2026-10-26: MnuPBook / MnuDiary page tables, first written down as 12-byte rows).
- **A bss global with "no writer" may be a member of a global struct.** Symptom: loads of
  `lui at, 0x7b; lw v, 0x2060(at)` everywhere, and a .text scan for stores with offset 0x2060 finds
  none. Cause: the word is a member (Sequence +0xdc20 at 0x7a4440) stored through the struct's base
  register (`sw v, -0x23e0(s0)` or similar). Fix: subtract the nearby globals' bases (address map,
  bss table) from the address before calling it unwritten (2026-10-25: 0x7b2060 = g_sequence.m_bits).

- **A forwarding wrapper's arity comes from its callers, not its body.** `MagSystem::setCameraMode`
  0x118880 only adds 0x12540 to a0 and jumps: it was declared with one argument, but GamMan passes
  mode, a camera number and a 3-word pointer through a1..a3. When a stub or a 2-line forwarder is
  declared, check one caller's a1..a3 / t0..t1 setup before fixing the signature.
- **GamMan's inline accessors name its fields.** `sections.py --list <Class>` lists every linkonce
  symbol; for a big class without asserts, disassembling the short ones (`mipsdis.py <addr> +<size>`,
  a lui / lw pair each) maps dozens of offsets to RoR's own names in one pass.

- **MinGW is not "GCC layout".** MinGW-w64 defaults to `-mms-bitfields`: a bitfield run that changes
  declared type opens a new unit on MinGW *and* MSVC, while EE GCC packs it.
- **`long` is 32-bit on both Windows compilers**; the EE's is 64-bit.
- **A plain `float[4]` is not a VU0 vector** (4- vs 16-aligned); offset asserts catch it.
- **Tail padding differs between the host compilers**: assert `>=` on derived host-layout classes.
- **Giving a POD base a constructor moves its derived classes' fields (MinGW)** (2026-10-22, the
  magscene session): DogEntity (0x98 bytes of data, 16-aligned to 0xa0) got an inline constructor;
  being non-POD, MinGW (Itanium ABI) then placed DogEntityEx's first field in its tail padding, at
  0x98, and every `offsetof(DogEntityEx / DogModel, ...)` assert failed. Fix: spell the tail padding
  as a field (`u_int _98[2]`) before adding a constructor to a padded base.
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
- **An int 0x7fffffff stored into a float field is the EE's largest value, a NaN on the host** (Eff0::Eff0
  sets Eff0::m_z this way; EffMan's depth sort compares it): every host comparison with it is false, so
  sorts and range tests break. Write FLT_MAX with a PC: comment.
- **0xffffffff stored into a float is the EE's most negative value, a NaN on the host** (EffBreath1::renderMain
  marks drawn sprites so in its farthest-first pick): write -FLT_MAX with a PC: comment.
- **Twin functions that differ only in an `ldl` / `sdl` copy are overloads on the matrix type**: one
  writes an aligned `sceVu0FMATRIX` straight through, the other builds it in an aligned temporary and
  copies it out unaligned -- an `I3dMatrixF4x4 *` (4-aligned struct; DogModel's symbols overload on
  both). Declare both overloads, not two invented names (MagSystem::getActorNodeMatrix, 2026-10-22).

- **Ghidra drops VU0 macro details** (bc fields, dest masks, vcallms bodies): `_vmulabc(a, b)` hides
  which field and which register. Read the asm (`ghidra.py asm`) for any function with lqc2 / vcallms,
  and the microprogram with `vudis.py` (2026-10-22, the view: micro 0 is a matrix multiply, 0x69 /
  0x96 box clip tests).
- **A newly written function dereferences what a stub returns**: the smoke run died in i3dMatCopy the
  moment i3dCtxCreateView stopped being a stub, because `DogEntity::getMatrix` (stub) returned NULL.
  When a reached stub's result now flows into real code, give the stub a valid PC placeholder (a unit
  matrix, an empty object) with a comment, or write it.
- **Two byte-identical copies of a library module carry the same names**: the i3d vector / matrix code
  exists at 0x2369xx and 0x2372xx; the second copy had the names (i3dMatMul ...), so the first got
  `i3dVu0*` for the collisions. Check `ror_functions.txt` for an existing name before naming a math
  helper.
- **Files the ROM relocates in place cannot hold host pointers** (2026-10-25, i3rroot): the i3r
  files turn 32-bit offsets into absolute pointers in the same words; an x64 pointer does not fit.
  Convention (i3rroot.h): a relocated field holds the signed distance from the field itself to its
  target (0 = NULL); read with `i3rPtr(&field)`, write with `i3rSetPtr`. Every reader of i3r data
  (the per-type accessors, i3rBuildPackets, DogModel) must go through it -- never cast a field.
  For a small table one class owns and reads through its own methods, building host records with
  pointers beside the untouched file is simpler (WrdSrv::setup, wrd/wrdsrv.cpp, 2026-10-26).
  The i3c collision files take a third way (i3croot.h, 2026-10-26): the host keeps the file's
  offsets (from the root) and every reader resolves them with `i3cPtr(root, field)`; i3cRelocate
  only turns -1 into 0. Its tree relocator adds the root only to values below the root's address
  (an already-relocated guard) -- a test with no host meaning. The casts (0x2312a0..) must read
  through i3cPtr; the query carries the root.

## Reconstruction bookkeeping

- **A string-copy stub makes every texture the last one** (2026-10-25, mnu/mnutitle.cpp): with
  utlStrCopy 0x105e50 a stub, DogTexInfo::addData never stored a name, so find / findFree handed out
  the same free entry for every texture and each load overwrote it -- all seven title sprites drew the
  disclaimer (the list's last file). Symptom: every sprite of a SprMan shows one picture. Check
  SprTexture's m_tex_no[] (all equal = this) before suspecting the renderer.

- **A stub's untouched out-buffer becomes garbage once its consumer is real** (2026-10-26, gamman
  session 4): GamSysData::getField is a stub, so seqModelOf's u_short buffer kept stack garbage; while
  GamMan::createObjByModel was a stub nothing read it, the real one handed the garbage to
  MagScene::loadModel (assert theID >= 0, then an out-of-range actor). Initialise such buffers to the
  ROM's "none" (0xffff for model ids) with a PORT comment until the callee is written.
- **work/ is gitignored: an overwritten batch file is gone** (2026-10-26): take the next batch number
  from a listing made right before writing (ls work/batches | sort -V), never from memory; the names
  themselves survive in data/context_names.json.
- **ROM bug kept out: releaseObjs frees vfchr nodes into the fchr pool** (2026-10-26, gamman session
  6): 0x1cdf5c's last loop walks the vfchr list (+0x1dcd0) for kinds 0 / 1 but deletes the object
  (which lives in the +0x1f248 storage, not the heap) and frees its node with the fchr pool's inline
  (+0x1d980 / +0x1d990 counts). The host destroys in place and returns the node to the vfchr pool
  (PORT comment there). Look for the same copy-paste when a release loop's pool offsets do not match
  the list it walks.
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
- **ROM data the code writes back must not be `const` on the host** (sprman session): SprMan's
  countAnims / countMoves store computed counts into the owner's tables, which s2d.cpp had declared
  `static const u_int` (a write into .rdata faults). Before keeping a .data image const, grep its
  readers for stores.
- **A flag Ghidra shows as "set once, tested at the loop end" may really never be cleared** (sprman,
  updateMove 0x18d1e8: the halt key's flag is set inside a do-while whose head is after the clear, so
  one halt key runs the loop through every later key): read the asm for where the loop branches back
  before "fixing" it; keep the ROM's behaviour and mark it `ROM:`.
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

- **A library version check behind stubs hangs the boot** (MagSystem::initGraph compares 36 i3d struct
  sizes with `i3dGetStructSize` and loops for ever on a mismatch; a `{}` stub returns 0): bridge such
  queries with the ROM's own table. Likewise an assert on a stub's result every frame floods the log
  (`r != 0` on `i3dCtxGetRest`): bridge with the common-case value.

- **A class with members named `free` / `realloc` / `alloc`** (BedMemory): inside its members an
  unqualified `free(p)` calls the member, not the C library. Write `::malloc` / `::free` there.

- **A record indexed by a failed id** (2026-10-26, GamSndMan::playSE / playStr): the ROM writes
  m_rec[id] with id = SndMan's -1 on failure, i.e. into the padding before the inline records. On the
  host that is the previous member: guard `id < 0` with a `// PC:` note. Look for the same pattern
  wherever a slot id from SndMan / rx indexes an array.
- **A pointer only another switch case sets** (GamSndMan::funcSE case 6 reads `set[4]`, set only by
  case 5 of the same call, so always NULL): the PS2 reads low memory, the host faults; guard it.

## callcheck / stubgen

- **A callcheck note stops at its first parenthesis** (2026-10-28, emy/emy_hfm.cpp): in
  `// callcheck: a, b (why), c` only a and b count, so c stays MISSING. List every name first, then
  one parenthesised reason. Calls folded into a file-static helper (hfmHitTest) need such a note on
  every caller.

- **A `ROR 0x...` inside a comment block counts as the next function's annotation** (2026-10-28,
  i3d/i3rdraw.cpp): a list of host-replaced ROM functions in the file header made callcheck check the
  first function below it (a PC helper) against the last listed address. Put such lists after the
  last function of the file.

- **"undefined reference to vtable for X" that stubgen skips** (2026-10-27, mag/maglifepc.cpp):
  a new polymorphic class whose out-of-line virtuals live in an unwritten TU. GCC emits the vtable
  with the first non-inline virtual (the key function); none defined means no vtable, and the linker
  names the vtable, not the functions, so stubgen ("data symbol: define it by hand") writes nothing.
  Fix: hand-write `ROSEPAN_STUB` bodies for every out-of-line virtual in the class's own file (a
  script over the header's `// ROR 0x...` lines does it; see mag/maglifepcmode.cpp).

- **A load loop spins on a stub.** The S2d inits (`S2dMes::loadTexture`, `S2dMovie::loadTexture`)
  `while (!SprTexture::update())`: a generated stub's 0 would hang MagScene::initRender for ever.
  spr/sprtexture.cpp has a hand-written `ROSEPAN_BRIDGE` returning 1 until the loader is written
  (2026-10-22).

- **A stubbed packet writer's `return {}` is a NULL packet cursor.** Once i3d's groups were real
  (i3d/i3dlayer.cpp), every Begin / write / End sequence ran `pkt = i3dPktSetReg(pkt, ...)` into a stub
  and handed NULL to the End call, which would move the context's head to 0x80. Give writers that
  only advance a cursor a bridge with the ROM's code (i3dPktSetReg / DmaTag / OpenChain ...), and make
  the big ones' stubs `return pkt;` with a PC: line (2026-10-22). The same holds for init stubs whose
  object is used right after (i3dDrawSetInit: an uninitialised set's group pointer).

- **A call the optimiser proves dead gets no stub.** `rxSysPrintf` calls `rxThIsDispatchDisabled` only
  when the static tty fd 0x2dba34 is >= 0; nothing in the build wrote it yet (its writer `rxSysOpenTty`
  is a stub), so GCC folded the branch and no undefined reference -- hence no stub -- appeared. It shows
  up once the writer is real; stubgen handles it then (2026-10-22).

- **A folded dead function shows as MISSING calls.** gamman's placeObjAt, clearObjAttr and setObjScale
  each have an unreferenced function after them (0x1cfe1c, 0x1d1958, 0x1d1f14: no jal, j or data
  word) that Ghidra counts in their body, so callcheck lists its callees (syncActorPos, FUN_001d9f40,
  FUN_001d8c50) as missing. `ghidra.py asm` across the body and `ghidra.py refs` on the second entry
  tell; explain it in the `// callcheck:` line rather than calling the dead code (2026-10-26).
- **Your own helpers hide RoR's calls**: a `static inline` wrapper of a RoR callee makes every caller
  report it MISSING. Write the inlined code out as the ROM has it.
- **`// callcheck:` syntax**: names first, separated by commas, one parenthesis last, all on the line
  (`// callcheck: A, B (why A; why B)`); `::`, `(` or `;` end the list; a wrapped line needs its own
  `// callcheck:`.
- **A member destructor is named `~Class` in a callcheck comment**, not `Class::~Class` (`::` ends
  the list, so the destructor reads as an extra `Class` call): `// callcheck: ~SprMan, operator_delete
  (member destructor, deleting variant)` (2026-10-22, the s2d session).
- **ALL-CAPS callees read as macros** and are skipped: explain a real call to one.
- **A goto label in column 0 breaks the definition parser**: indent labels.
- **A constructor with an initializer list parses as a function named after the base class**: its
  explanation still has to name the base constructor.
- **stubgen finds one declaration per line**: write prototype blocks one per line.
- **stubgen and overloads**: for an undefined overload it copies the first declaration of that name, so
  two overloads get two identical stubs (redefinition error), and an overload whose name already has a
  definition is never stubbed (the link stays red with the same undefined reference each pass). Write
  those stubs by hand with their own signature and ROR address (2026-10-26, native 252: GamSndMan
  playSE / playStr / loadSePack).
- **stubgen places stubs by inference**: a stub can land in a wrong or new file; move it, delete the
  stray file.
- **stubgen glues `public:` into the stub** when a class's first method follows an access label
  (`class X
{
public:
    void f();`): the stub starts with `public:` and a void method gets
  `return {};`. Declare minimal classes as `struct` without labels (or a label on a line of its own
  after a member); delete the broken stubs and the files stubgen created, then run check again
  (2026-10-22, main session).
- **The frontier counts what the port replaces wholesale.** The assert handler 0x215a30 (1,483 call
  sites, written back as `ROR_ASSERT`) made `tus.py next` pick the sequence engine run
  unassigned@0020eda4 by "frontier x44"; `settings.frontier_ignore` in data/tus.json now lists it
  (tus.py and status.py skip those callees). Add any other such callee there (2026-10-22).

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
- **`work/shots/last.png` can come out 0 bytes** (seen 2026-10-26 07:15; the shot_*.png of the same
  run were fine) and `finish` copies it to progress/ as is. Check its size after `check`; rerun the
  check when it is empty.
- **`tus.py queue <key> --task TEXT` makes `<key>` a host task** (its brief skips boundaries / names /
  code). Never use it to annotate a TU with code; write the reason in open-items.md. To undo:
  `tus.Model()`, pop the entry's `task`, `save()` (2026-10-22).
- **Defining a file inside an `unassigned@` run leaves the rest under new keys**: the run before keeps
  the old key (now a different TU) and the run after gets `unassigned@<its first address>`; fix the
  pinned queue (`tus.py queue <old> --remove`, queue the new key) in the same session.
- **Batch file numbers collide**: `ls work/batches` and take the highest number + 1 (work/ is
  gitignored, so an overwritten batch leaves no trace).

## Shell and Python on Windows
- **A python heredoc's "\n" became a real newline** inside a C string (the build broke with an
  unterminated printf): the Bash tool's heredoc plus Python escaping is two levels. Fix: write C
  string escapes with the Edit tool, or read the file back before building.

- **A long C++ heredoc in one Bash call failed to parse** ("unexpected EOF while looking for matching
  quote", 2026-10-26) and nothing was written. Write big code blocks with the Write tool (scratchpad
  file), then splice them in with a short python -c.

- A batch JSON for name_ctx.py with a PS2 path in a `why` (`cdrom0:\DATA\...`) fails to parse: a
  quoted heredoc keeps one backslash, and `\D` is not a JSON escape. Write paths with `/` in `why`.

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
- **`gdbrun.py --help` has no help: it launches the game under gdb** and blocks the shell. Read its
  docstring instead.
- **A stub name can lie about which object runs.** A host placeholder class (the embedded menus
  are all plain MagLifeMenuBase, whose methods carry MagLifeMenuItem's addresses) logs the stand-in
  class's stub for every object of it: "MagLifeMenuItem::init reached" meant the title menu was
  opening. To know what was asked for, put a temporary printf (the log only catches stdout) at the
  request site, run `build.py --run 6`, and revert it before `finish`.

## Build

- **MinGW must be on PATH** or `cc1plus` fails with no diagnostics -- use build.py.
- **shadercross file lock** ("being used by another process") on parallel shader builds: rerun
  (build.py retries once).
- `ld: cannot open output file RosePan.exe: Permission denied`: the game is still running -- possibly
  another session's play test from the same `cmake-build-relwithdebinfo` (2026-10-28: a session ran
  `ROSEPAN_CRASH_LOG=1 ./RosePan.exe` twice in a row during the aspect task). Never kill it; build and
  run the visual test from another config (`build.py --config release --run N ...`, its own
  `cmake-build-release`, deps reused from ReiPan) and say so in the notes.
- `-fno-strict-aliasing` is required (MioPan's release-only crash).
- **layoutcheck compiles every header on its own**: a header that relies on what its .c included first
  fails; give it its own includes.
- **The build never runs MSVC**; only layoutcheck does (syntax only): MSVC-only errors show up there.
- **A raw NUL byte in a character constant breaks MSVC only**: 13cb7db (the aspect task) left
  `dir[0] != '<NUL>'` twice in rosepan_renderer.cpp; MinGW compiled it, the user's Visual Studio build
  stopped on C2137 "empty character constant" (2026-10-29). Cause: a C escape `\0` written through a
  shell command can land as the byte itself (seen again while fixing it: a Python heredoc's `\\0`
  wrote a NUL). Write source text with Edit / Write, or build the byte as `bytes([92]) + b'0'`.
  build.py now refuses NUL bytes in src/ before compiling; layoutcheck never saw it (headers only).
- **A smoke run that exits at once with 0x20474343** is an uncaught C++ exception -- seen when another
  game process held the log file. Check `tasklist` for another RosePan.exe; never kill another
  session's process.
- **`exited early with code 3221225477`** is an access violation: `python gdbrun.py` (with the same pad)
  gives the backtrace; the game thread is RosePanGame.

- **Renaming ReiPan -> RosePan must cover the shaders' `.hlsli` includes** and all three spellings
  (`REIPAN_`, `ReiPan`, `reipan`): the `.hlsl` files were renamed but the `.hlsli` kept
  `ReiPanVu1*` / `REIPAN_*`, and shadercross failed with "unknown type name RosePanVu1LightImage"
  (2026-10-22, bootstrap). The prebuilt `.msl` / `.spv` keep old names in comments only.

- **FIXED 2026-10-29: the boot hung on "Now Loading" retrying `data/options.txt`** (100k+ "rxfile :
  failed to open file 'host0:data/options.txt'" lines from MagScene::loadSystemData; 1 run in 4..16
  since 2026-10-25, gdbrun too). Cause: the host's rx file thread runs beside the game thread, and
  rxFileOnGetSize lowers `pending` before it sets the failed bit (`state |= 4`); rxFileOpenSlot's
  waiter could see pending 0 and read state in between, so fileExists said yes to the missing file
  and the FileReader's 0x8001 open (NOWAIT, retry a missing file) retried it forever. The PS2's
  waiter cannot run before the callback returns. Fix: rxFileWaitCallbacks (DIntr + EIntr, which
  waits out the callback the file thread runs inside its section) after the waits in rxFileOpenSlot
  and rxFileSlotClose. Proven by a 20 ms sleep after the `pending--`: 4 of 4 runs hung without the
  fix, 4 of 4 booted with it. Any new host wait on a slot field that a callback writes needs the same.
- **Sequence::loadSubImage copies 0x30000 bytes whatever the image's size** (as the ROM does). The
  ARA packs' stage-2 images are 12..31 KB near the end of a ~90 KB pack buffer: the PS2 reads what
  follows, the host faulted in memmove past the heap block (GamMan::initStage2, 2026-10-26). The
  caller copies the image into a zeroed 0x30000 buffer first (PORT in initStage2); loadStageImage /
  loadSystemImage copy 0x18000 / 0x20000 the same way and are fine only while their files are
  big enough.
- **Under gdb the New Game path takes another road**: the same pad script asserted m_i3r_root
  (dog/dogmodel.h 94) and maglifemenu.cpp 358 early, then timed out, while build.py runs reach the
  10d crash. Compare stub logs before trusting a gdb backtrace for a build.py crash.

## Platform layer (ReiPan's, written for FF2 and FF3)
- **(History: the stand-in was deleted 2026-10-30 when the reconstruction replaced it; the lesson
  holds for any IOP fake.) The rxSnd IOP stand-in (src/sdk/iop_rxsnd.cpp) is silent, and a writer that waits for room
  stalls**: rxSndAdpcmGetFree is the IOP buffer size minus what the 0xa3 status says it holds, and
  an empty status left every ADPCM stream full after its first 8 KB, so the movie's audio que
  filled and back-pressured the demuxer (the attract movie froze on frame 71). The stand-in now
  drains each open ADPCM stream at its own rate (16 bytes per 28 samples per channel) and lists it
  playing. Trap inside it: queued command 0xa2 is {id, param, value} for close (param 0) and
  pause / volume / the rest (1..7) -- closing on every 0xa2 closed the stream each frame. PCM
  streams (0xb0..) still never drain (no disc movie uses PCM).
- **The host libmpeg feeds its decoder from the demuxer; a que the ROM empties by IPU DMA has to
  be emptied by what the decoder parsed**, in the ROM's own units: rxMpegNodataCallback retires
  min(2 KB-rounded readable, 2 KB-rounded parsed). Retiring odd counts broke the que's 2 KB read
  alignment, left a sliver under 0x800 before the wrap that no DMA could take, and froze the logo
  movie at its 103rd picture.

- **RoR: script pointers in 32-bit words** (2026-10-25). Symptom to expect: a script that stores a
  pointer (sw) and loads it back crashes on a truncated host address. Cause: the scripts' memory is
  32-bit words, host pointers are 64-bit. Fix in place: sequence/seqvm.cpp seqVmLoadWord /
  seqVmStoreWord / seqVmCopyArea (open-items Decisions). Any new code that writes a pointer into
  script memory (a native filling a script buffer) must go through seqVmStoreWord, not a plain store.

- **`sdk/libgraph.h` swaps `SCE_GS_INTERLACE` (0) and `SCE_GS_NOINTERLACE` (1)** against Sony's
  SDK (interlace = 1). The host's `sceGsResetGraph` only stores the value, so nothing breaks yet, but
  code written with the macro passes the opposite of the ROM: write the ROM's literal with a comment
  (main.cpp's `mainInitGraph` does), or fix the header and every caller in one commit.
- **The host's `SetDebugHandler` returns 0**, not the old handler: code that saves and restores
  handlers (mainSetExceptionHandlers / mainResetExceptionHandlers, rxSysSetDebugHandlers) restores
  nothing on the host.
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
- **The rx scheduler runs on real host threads** (2026-10-22, rxthread): priorities are no-ops, so rx
  threads run alongside the main thread, not only in its yields. What must keep working is the
  handshake: the dispatch alarm is a host timer thread (src/sdk SetAlarm, handlers under DIntr), the
  vblank count comes from libgraph's vblank thread raising INTC cause 2 (started by AddIntcHandler(2)),
  and ReferSemaStatus reports count and waiters. Spins that would hold a core sleep (`// PC:`). Host
  traps: SuspendThread / ResumeThread are still the FF3 loader mutex -- while the main thread is not
  yielding it holds it, so *another* thread's WaitSema / SleepThread blocks after waking until the next
  yield (a main-thread spin waiting on such a thread would deadlock); TerminateThread / DeleteThread do
  not stop a host thread (a deleted rx thread keeps looping in rxThYield, sleeping); ExitDeleteThread
  parks the host thread for ever. The ROM's rxThPostCall publishes the write index before filling the
  entry: reordered on the host.
- **Two host threads**: the OS main thread pumps SDL events, the game runs on its own thread; SDL
  "main thread only" calls go through the platform's run-on-main-thread helper.

- **A 4-bit texture with half its colours black or transparent** (2026-10-22, the i3d module): its
  CLUT is in CSM1 order too. i3d uploads every CLUT 16 colours a row (i3dPktClutUpload), and the GS
  reads a CSM1 CLUT in 8x2 tiles, so a 4-bit palette's colours 8..15 are the second row's first eight
  entries (index bits 3 and 4 swapped, as for 8-bit images). TIM2s with 32+ colours store them that
  way (S2DMAN.RPK @0x11000: entries 8..15 are zero). rosepan_draw2d and tm2png.py both apply it now.
- **i3d draws on the host only through the replay** (rosepan_i3d.h): a new packet writer, or a
  register a writer sets some other way than i3dPktSetReg, draws nothing until it records itself;
  a packet range the walk links without calling `RosePan_I3dReplay` is skipped.
- **A bounce buffer the ROM relies on being aligned** (2026-10-25, rxSndIopRead): EE bss is laid out
  with alignments host statics do not get. rxSndIopRead recurses into its own bounce buffer and only
  stops when that buffer is 16-byte aligned (0x7b6b00 on the EE); unaligned on the host it recurses
  for ever. Give such buffers `alignas(16)` (or whatever the ROM address implies).
- **RoR: a full-screen picture drawn again over everything** (2026-10-25, mag/maglifefader.cpp: the
  logo stayed opaque while the fader's sprites reached the renderer with the right alpha). Cause:
  SprMan::drawFrameCopy (S2dMovie's layer 1, every frame) writes TEX0 with i3dPktSetReg to sample
  the other frame buffer; rosepan_i3d only binds textures through RosePan_I3dRecordTexRegs, so it
  drew that sprite with the last bound texture -- the logo, on a layer after the fader. Fix:
  ApplyReg marks a direct TEX0 and DrawPrim skips textured primitives until the next draw set's
  registers (logged once). How it was found: a temporary log of every sprite DrawPrim draws (type,
  tme, rgba, texture pointer) and of each RosePan_I3dReplay range, against MagLifeMan::draw's life
  order -- the same texture pointer twice per frame, once before and once after the fader.

## Half-written model loads (2026-10-26)

- Symptom: New Game crashes (access violation) right after "NULL != m_mat_local" / "m_mat_flag" /
  MagScene "pmat" asserts. Cause: DogModel's loader is written but i3rItemRelocate (0x22c478) is a
  stub, so the i3r item tree keeps file offsets and i3rGetNumNode returns garbage; allocMatrix's
  alloc fails and later code walks NULL node arrays. Fix in place: PC guards in allocMatrix,
  setupNodeFlags and MagScene's node billboard loop. The relocator is written (i3d/i3ritem.cpp,
  2026-10-26); remove the guards once a run that reaches the load shows sane node counts.
- Only some New Game runs reach the model loads (two of four 200 s runs did, by timing); judge a
  load-path change on a run whose log shows i3rAssignTex.
  (2026-10-26: two more 200 / 240 s runs reached none -- the post-film setup runs, then
  DogActor::load 0x10cd5c is a stub; the i3ritem session checked the relocator offline instead,
  emulating it over the disc's 1602 RPK models.)

## A stub that starts writing exposes bad indices (2026-10-27)

- Symptom: once DogActor::loadPackMotion was written, the 200 s New Game run crashed (call to
  address 0) in MnuPBook::openWindows; the narration had opened window 3, the book found no free
  window. Cause: GamMan::setPcAnim passes m_pcActor, still -1 while MagLifePC::setActor is a stub;
  MagScene::setActorAnim then ran on m_actors[-1], which on the host lies over MagScene::m_window,
  and DogMotionBlend::reset's clears wrote 0xffffffff into the window slots. Fix: a PORT range
  guard in MagScene::setActorAnim. Lesson: when a stub becomes real code, look for callers that
  index arrays with values other stubs leave at -1.
- How it was found: compare against a HEAD baseline run (git stash the sources), trace the
  suspect calls, then a gdb hardware watchpoint on the corrupted word. **On Windows gdb arms a
  watchpoint only on the thread that set it**: break in game code first (tbreak MagScene::update),
  then watch -l, not from starti.
- Symptom: right after a build that was killed part-way (a stopped tus.py check), the exe faults
  in __do_global_ctors at startup with an empty log. Cause: a broken object file. Fix: delete the
  touched .obj files and RosePan.exe under cmake-build-relwithdebinfo and rebuild.

## A "never read" argument that passes through in a register (2026-10-27)

- Symptom: callers pass a word the callee's decompilation never reads (seqFunc023 gave GamArea::addTrg a fourth
  word -1, 0, 1, 2; the port dropped it as unused).
- Cause: the callee leaves the register untouched and calls on with it: addTrg never touches a3, so it is
  GamMan::makeOid's third argument (the image). Ghidra shows neither side.
- Fix: before dropping an argument, read the callee's asm for the jal sites it reaches with that register
  unchanged (`ghidra.py asm`), and the callee's callees' parameter counts.

## A list walk that frees the node it stands on (2026-10-27)

- Symptom: GamArea::update 0x1dee78 moves a one-shot trigger (flag 0x40000000) from the active list to the free list
  in the middle of its walk, then steps to the node's m_next -- now the free list's sentinel. The walk only ends at
  the active list's sentinel, so the ROM would go round the free list (stale triggers included) for ever.
- Cause: an inline SeqPool::remove inside a for loop that advances through the removed node. The ROM saves the
  old next (sp+0xa0) but never reads it; presumably no retail script sets 0x40000000.
- Fix: the port saves next before the body and goes on from it (PORT comment). Check every inlined pool remove
  inside a walk the same way: read where the asm reloads the iterator (ghidra.py asm around the loop's branch).

## i3r mesh data: pointers where a value is expected (2026-10-27)

- Symptom: a skinned model drew as scattered limbs, then every rigid part on one bone.
- Cause: the 0x4c group's bone slot is not its data's first word but what that word points at
  (`*(u16 *)*data` in 0x220290; the word is a relocated pointer, 8 = "data + 8"), and the
  non-flag-8 groups are CPU-skinned per batch (0x225670), not drawn with the world matrix.
- Fix: read every first word of an i3r data record as a candidate pointer (i3rPtr) and check
  the decompiler's double dereference. Model packs nest: CHARA.RPK -> chapter -> model entry
  (".mdl" + ".MOT") -> .mdl (TIM2 pack, i3r, collision); flags 0x8002 packs have offsets and no
  sizes (getFileSize returns -1).

## A file kept open changes the boot (2026-10-28)

Symptom: after a TU that opens a file and keeps it open (as the ROM does), the New Game run stays
black after the title with no new stubs, no assert and no crash. Cause: unknown host-side effect of
a long-held rx stream slot (open-items.md Open questions, eff/effman.cpp). Fix for now: close after
each read (PC: comment). How it was found: bisect with an environment switch around the new code
(getenv in init, build.py --run 90), then gdbrun --break + "thread apply all bt" showed no thread
blocked, so it is game logic, not a lock.

## Data words that are 0 in the i3r file (2026-10-28)

Symptom: a crash in i3rBuildMaterialPacket (null read at +0x41) the first time it ran on the host,
in MagScene::drawActorShadow; then, once guarded, the book in Jennifer's hand vanished.
Cause: items whose m_data is 0 -- an STQ map entity that i3rReplaceStqMapEntity fills at run time,
a 0x56 batch's child 3 (the box) -- give getData() NULL on the host; the ROM reads EE address 0
(zeros) and goes on, so the code never tests for it. The host skipped the batch, dropping the mesh.
Fix: read such data as the ROM's zeros (kind 0 = skipped; the box centre = the local origin), with
a PORT comment, never skip the draw.

## The double buffer's draw envs were all zero (2026-10-28)

Symptom: after MagScene's off-screen shadow pass the receivers (and any 2D) replayed with XYOFFSET 0
and SCISSOR 0, so the native i3d module skipped them as off-screen; the map's shadow model drew
white once the offset was forced. Cause: src/sdk/libgraph.cpp sceGsSetDefDBuff was a memset stub;
MagSystem only overrides the FBPs, so putDrawEnv re-sent an env of zeros. Fix: the stub now fills
draw0 / draw1 as the SDK does (FRAME, ZBUF, XYOFFSET centred on 2048, SCISSOR, PRMODECONT,
COLCLAMP, DTHE, TEST); putDrawEnv also records FRAME. Side effect, expected: the letterbox bars and
message windows drawn after a pass now show.

## An STQ slot too far for a 32-bit relative field (2026-10-28)

Symptom: i3rReplaceStqMapEntity ran but the slot read back NULL; stdout (not the log) had
"[i3r] ... too far from the model slot". Cause: i3r fields hold the distance from the field (32
bits); MagScene (a global) is more than 2 GB from the model heap. Fix: I3R_FAR_PTR marker + side
table keyed by the field address (i3rroot.h / i3rroot.cpp), used by i3rSetFarPtr. The unrelocate
path (0x22c328) does not know the marker; a model unrelocated with a far slot needs it.

## A script entry kept in a 32-bit record word (2026-10-28)

Symptom: the plain run exits 0xC0000005 about 2 s after the picture book, not every time; gdbrun
never faults. ROSEPAN_CRASH_LOG=1 + addr2line: GamArea::checkPc -> Sequence::execThread ->
seqFindEntry reading 0x64dfbac4. Cause: the script fills its own GamAreaTrg record with sw, so
m_entry (int, EE offset 0x34) keeps only the low half of a host code pointer, while m_base (native
018 case 0x1e) is the full host pointer. Fix: seqImagePc takes the in-image offset in 32 bits
(seqvm.cpp). Any other script-filled record holding a code address goes the same way. Also: a native
whose cases look plausible can still be shifted -- read its jump table (native 021, data 0x3005c0).

## A smoke run that goes black after the publisher logo (2026-10-29)

Symptom: a 240 s run's screenshots are all black (4 KB PNGs) from frame ~1680 to the end, no stub or
error in the log. Cause: not the code -- two reruns of the same build reached the title and the
intro (cause of the stall unknown; seen once, on eff/effcommon.cpp's session). Fix: before blaming a change, rerun once and
compare the shot sizes with a baseline build (git stash -u, build.py --run 60 with the same pad, pop).

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
  generously and confirm with a shot before blaming the code. In RoR the count is BedPad::update
  calls on port 0 (scePad2Read -> libpad's scePadRead), and the same script landed at different shot
  frames from run to run (2026-10-25: Start at read 2400 opened the menu by shot 2640 in a 70 s run;
  a 50 s run had only reached shot 2160): give the run time to spare.

## Tools

- **stubgen takes an access label as part of the next declaration.** Symptom: a generated stub starts
  with `public:` and the build fails ("expected unqualified-id before 'public'"). Cause: stubgen copies
  the text from the previous member up to the declaration, and `public:` sits in between. Fix: in a
  minimal header for another TU, declare the class as a `struct` (no labels), or put the label on the
  line after a member, and keep each declaration on one line (2026-10-22, bed/bedmemory.h).

- **A crash that gdbrun does not reproduce** (2026-10-26). **Solved (setup/newgame_crash.cpp,
  2026-10-26): a script address cut to 32 bits.** Native 110 case 11 registers an entry with a script
  address (a1), and on the host script addresses are host pointers (the image lives in the game heap,
  0x1xxxxxxxxxx); SeqEntry::m_value was an int, so callEntry later queued a thread at the cut address
  and seqFindEntry read 0x554905c0 (`ROSEPAN_CRASH_LOG=1` + addr2line found it in one run). Rule: every
  value a script passes that can be an address (a pc, an entry, a pointer into the image) stays
  intptr_t from the VM register to where it is used; grep seqcall.cpp for `(int)a` casts when a
  native stores one. Where a1..a3 are used as script addresses check the native's ROM (`lw` vs `ld`
  does not tell: the EE keeps 32-bit pointers). The rest of this entry is the earlier hunt.
  Symptom: the plain smoke run exits with
  0xC0000005 right after a New Game press, every time; `gdbrun.py` with the same pad runs on without a
  fault. Cause: unknown yet (timing, or a fault gdb passes as first-chance in a DLL). Fix so far: bisect
  by disabling the suspect (a temporary early return) in plain runs -- native 151 was cleared that way.
  Note the run log is not flushed at the fault: the last logged stub is not necessarily the last call.
  2026-10-26 (session 5): the crash came back once (run 1 of 6) and never again; the other five plain
  runs (and a gdbrun) instead **stalled black after the Atlus logo**: MV00_AD never shows a first
  picture, so Start (read 1800) never skips it and the title never opens. The game's own logs
  (%APPDATA%/Mikompilation/RosePan/rosepan.N.log, three kept) show the difference: a good run opens the
  film's ADPCM ('open ADPCM 47') ~22 s after `rxSysUpdateDevices`, a stalled one logs nothing after it.
  Not file speed (the 252 MB file reads in 0.5 s), not the memory card (unchanged), not the crash filter
  (stalls with it off). A gdbrun later in the session got through (title, New Game, natives). Suspect a
  race in the host movie start (snd/rxmpeg.cpp over sdk/libmpeg.cpp: first picture vs the demux / audio
  open). **`ROSEPAN_CRASH_LOG=1`** (os/rosepan_host.cpp) now prints a fault's code, address and stack as
  exe offsets to stderr (build.py keeps them in work/run_<config>.log); resolve them with MinGW's
  addr2line -f -C -e RosePan.exe <0x140000000 + offset>. Pass it with build.py --env.
  2026-10-26 (session 6): a gdbrun stalled the same way; `gdbrun.py --break` then showed **it is not the
  movie**: the main thread waits in MagSystem::waitVSync -> rxThVSync -> rxThYield -> WaitSema
  (s_yieldSema), the vblank thread ticks, s_request = rxThVSyncRequest, s_threadCount 1, s_yielding 1,
  and the dispatch alarm is live and re-arming (s_alarms[0] = rxThDispatchAlarm, id 7227; so dispatch
  is not disabled). So rxThDispatchAlarm never signals s_yieldSema: either rxThSync(-1) stops moving,
  s_requestSema's count is not 0, or iReferSemaStatus(s_yieldSema) reports no waiter (a host race
  between WaitSema's waiter count and the alarm thread). Next: in a held game print the three semas'
  status through the host sema table and rxThSync's counter (gdb could not evaluate the statics by
  name in that session: use `p 's_yieldSema'` style quoting, it worked for s_alarm). Plain 170 s runs
  can also freeze right after New Game (last log GamMan::setPcAnim, no frames after 2700) -- probably
  the same lost wake-up.
  2026-10-30 (the intro skip): one of six runs went black after CP_LOGO **with frames still presenting**
  (a shot every 0.5 s, i.e. v_interval 1, so execLogo had passed step 0x11): MV00_AD was opened but
  RxMpeg::isReady never came (the ring never primed), and execLogo waits at step 0x12 for ever. So at
  least one form of this stall is the movie ring, not waitVSync. The PC skip (mnu/mnutitle.cpp) also
  waits for isReady there before closing; `[game] skip_intro = 1` never opens the movie at launch.
- **Hidden functions inside GamMan bodies**: 0x1d1e88 (setObjScale) holds a float getter at 0x1d1f14,
  0x1d18c4 (clearObjAttr) one at 0x1d1958 (achr tail call 0x1d9f40); Ghidra's sizes include them.


- **A derived member with a base member's name hides it silently.** Symptom: gamman.cpp's actor
  shrink / scale (gamShrinkActor, the oid scale setter) and seqcall.cpp's scale query wrote and read
  DogActor's node scale values (0x40c) instead of DogEntity::m_scale (0x60); the build stayed green.
  Cause: DogActor declared its own m_scale. Fix: name fields after the ROM's asserts
  (m_scale_node_no / m_scale_node_val), and before adding a field to a derived class grep the bases
  for the name. Same trap for methods: DogActor::getNodeIndexByName(int, const char *) hid
  DogModel's one-argument version (build error) -- the ELF calls it i3mGetNodeIndexByName.
- **Header inline definitions do not count as reconstructed.** Symptom: a TU's ELF inlines (GamMan
  0x28e254, getWrdNo 0x1c6c28) stayed `todo` in the brief though written inline in the header with
  their `ROR 0x...` comment. Cause: `rosepan.reconstructed()` scans only .c / .cpp (headers carry an
  annotation on every declaration). Fix: define the ELF's inline copies out of line in the class's
  .cpp under their `// ROR 0x...` line (gamstat.cpp's and gamman.cpp's "ELF's inline accessors"
  blocks); a type_info function (`__tf`) gets a comment-only annotation.
- **A bss word right after GamMan is GamMan.** 0x7a3208 / 0x7a320c are g_gamMan (0x7812c0) +
  0x21f48 / +0x21f4c, m_flags / m_flags2 reached through the global: check `addr - 0x7812c0` against
  GamMan's members before declaring a `g_bss_` stand-in.

- **A packet-writer stub returning NULL crashes the next real writer** (2026-10-27, the picture book).
  Symptom: access violation right after `i3dPktBeginVu1Sprite reached`. Cause: stubgen's `return {}`
  hands mnuBase::beginDraw a NULL packet and the written i3dPktRefTexRegs / i3dPktSetReg write through
  it. Fix: packet stubs on a written draw path return their `pkt` argument (the five VU1 sprite stubs
  do now); likewise text getters return "" (msgGetSysText), never NULL, while their TU is unwritten.
  Find such crashes fast with `gdbrun.py --pad SCRIPT` (the backtrace lands in work/gdb/log.txt).
- **Never leave a stray `cat > file` in a shell command** (2026-10-27): with no stdin redirect it waits
  for input and a background check hangs silently for its whole timeout. Pass `< /dev/null` to long
  background runs.
- **"Removing unreachable block" around sqrtf is harmless** (2026-10-27): the inline `sqrt.S` is
  followed by a `c.eq.S f, f` NaN check and a `jal 0x26cb48` (sqrtf's errno path) Ghidra drops; the
  decompiled branches are complete. A comparison that looks impossible (GamMan::hitAtPoint kind 4: the
  x / z distance must exceed the radius and the 3D one stay below it) is the ROM's, not Ghidra's:
  confirm in the asm, then reproduce it.
- **The Bash tool rejects a long inline `python - <<'EOF'` script** whose body has unbalanced quotes
  in comments (2026-10-27: "unexpected EOF while looking for matching"); write big edits with the
  Edit tool or a script file instead.

## i3r items described with host pointers (2026-10-27)

- Symptom: i3dLayerFindMaterialDrawSet took an `I3rMaterial` struct with `I3rTexList *` /
  `I3rTexRefEntry *` members -- it compiled, but on the host those words are self-relative u32
  offsets in the file, so any call would have read garbage.
- Cause: the struct was written from one reader's offsets (`mat+8`, `+4`, `+8`) without noticing
  they are I3rItem fields (m_children, m_info, m_children of the child).
- Fix: when a reader walks +0 / +4 / +8 of model data, it is an I3rItem: use getData() /
  getChildren() / getNumChild() and the type's data struct (i3rmodel.h), never a pointer struct.
- Again 2026-10-28: I3dBoneBoxes (i3dViewTestBoneBox's bones, a 0x46 part's data) was declared
  `layout: host` with an `sceVu0FVECTOR *` at +4, and the index as `u_short **`; on the 64-bit host
  m_count moved to +0x16. Data that comes from an i3r file is always plain EE layout with u_int
  relocated fields read through i3rPtr, even when only one ROM function reads it.

## A class whose virtuals are all unwritten links as "vtable for X" (2026-10-28)

- Symptom: after declaring the Eff classes with out-of-line virtual overrides, the link failed with
  33 "undefined reference to vtable for EffX", and stubgen skipped every one ("data symbol: define it
  by hand"); no member stubs were written.
- Cause: GCC emits a class's vtable with its key function (the first non-inline virtual); while that is
  undefined the linker only names the vtable, never the members, so --from-build has no names.
- Fix: pass the members to stubgen by name: `python stubgen.py EffX::init() EffX::liveMain() ...`
  (generate the list from the header's class bodies). Out-of-line constructors / destructors still need
  hand stubs. Also: a `// callcheck:` comment cannot name `A::B` (its regex stops at the colon) -- write
  the bare names (`Eff0`, `EffBg1` for constructors).


## A render-target pass drawn on the screen (2026-10-28)

- Symptom: after the picture book (the 200 s run, frames 6180+) a big untextured white figure filled
  the screen, seen from above; the scene's other 2D layers stayed black.
- Cause: it was Brown's shadow model (his pack's file 2, DogModel m_i3r_root2: materials bwn1, one
  unnamed, bwn3_tcc, never i3rAssignTex'd, so every layer reads m_tex_index -1). MagScene::makeActorShadow
  draws it from the light into the 256 x 256 shadow target (FRAME fbp 0x118, XYOFFSET 0); the host
  replay skipped XYOFFSET 0 for prims and strips but not for model records. And MagScene::putDrawEnv
  restores the display env by DMA reference, which the replay never saw, so after any off-screen pass
  XYOFFSET stayed 0 and the rest of the frame's 2D was skipped.
- Fix: rosepan_i3d's DrawModel skips XYOFFSET 0 too; putDrawEnv records the env's XYOFFSET / SCISSOR /
  TEST (RosePan_I3dRecordReg, PC: comment). Lesson: an untextured model may be a pass whose target is
  not the screen -- check the layer's FRAME / XYOFFSET before chasing textures. Found by logging, in
  MaterialTexture, the misses with the layer's texture name and comparing with DogTexInfo::addPfh's names.

## A black 3D scene that is a camera nobody placed (2026-10-28)

- Symptom: after the picture book every frame was black, though MagScene::draw ran with the world
  flag and the actors and map were alive and textured.
- Cause: i3rDrawModel culled every model (I3R_DRAW_CULLED 0xd) because the scene camera was still
  its default -- mode 0, no parent, at (0, 1000, 0) looking along +y. Neither the script nor a PC
  bind had placed it yet. The shadow passes, which have their own views, did draw.
- Fix / method: log utlDrawI3r's result per model and the camera (mode, parent, world position,
  view matrix) every 60 frames before chasing textures or the mesh module. ROSEPAN_TEST_CAM=1
  (MagScene::testCamera) aims such a camera at actor 0, which shows whether the draw path works on
  its own. Note: a smoke run can take a different path from run to run. One of four 200 s runs went
  black at the title (frame ~1860) and never reached the book, so compare against a second run before
  reading anything into a single black run.

## A write through a member the ROM leaves NULL at boot (2026-10-28)

Symptom: after msg/msgman.cpp was written, every frame after the boot logos was black, the main loop
kept waiting on V-sync and the ROM log went silent ~3 s in -- no crash report, not even under gdbrun.
Cause: the boot memory card check (MnuMcd) opens and closes its MsgManSet before the scene's S2dMes has
sprites; MsgMan::close hides the three message icons through S2dMes::setSpriteState, which writes
through m_spr.m_sprite (NULL then). On the EE that lands in low memory and nothing happens.
Fix: a PC: guard where the write starts (msgShowSprite in msg/msgman.cpp). To find such a stall, add
scePrintf traces to the new TU's entry points (they reach the run log; stderr does not) and run
build.py --run 45: the last trace before the log goes quiet names the call; compare a stashed baseline run.

## A sprite of a stubbed buffer covers the screen (2026-10-29)

- Symptom: after writing MagLifeWindow, the 240 s smoke run went black at frame 1680 and stayed
  black (the baseline showed the title at 1800, the book, the bus-stop room).
- Cause: the window's render draws its GamFont buffer as a texture. With GamFont::setTarget still a
  stub the font has no buffer (m_dst_w / m_dst_h 0), so setFontManager set m_uv to 0..0 and the
  sprite stretched one texel over the whole 640 x 448 window, opaque, every frame.
- Fix: a PC guard in MagLifeWindow::render skipped the sprite while m_buf_w <= 0 (removed with the
  GamFont TU, 2026-10-29). Method: compare brightness per shot against a baseline run of the previous commit
  (git stash, same --pad, PIL ImageStat over work/shots), then toggle the suspect draw with an
  environment variable for one run: three runs located it in 15 minutes. A new draw whose data
  comes from a stubbed producer is the first suspect for a black screen.

## A string table dumped as JSON escapes (2026-10-29)

- Symptom: `datadump.py ... S:text` printed non-ASCII bytes as `\u0082`; pasted into C++ that is a
  universal character name, which GCC / MSVC store as UTF-8 -- two or three bytes, not the ROM's one.
- Cause: the S field used `json.dumps(s.decode('latin-1'))`.
- Fix (msg/msgsystext.cpp session): S now prints a C literal with `\x` escapes and a `" "` split
  before a following hex digit -- the ROM's bytes exactly. RoR's English texts carry Shift-JIS
  punctuation (0x81 0x68 quotes, 0x81 0x66 apostrophe, 0x81 0x40 space): never re-encode game text.

## A VU0 vcallms address is a byte address in Ghidra (2026-10-29)

- Symptom: `python vudis.py 2949f0 9c 0xaf0` prints nothing for i3rSkinVertices' `_vcallms(0xaf0)`.
- Cause: Ghidra shows the vcallms operand as a byte address (index x 8); vudis.py takes the
  instruction index.
- Fix: divide by 8 (`vudis.py 2949f0 9c 0x15e 0x172 0x186` for the skin microprograms).

## A ROM loop that never advances its source (2026-10-29)

- i3rBlendAddVertices 0x2253e0 (i3rBlendVertices, group flags 2 without 1) moves only its counter:
  every output qword gets w x the target block's first qword. Kept as the ROM does it; if a blended
  mesh with plain-qword targets looks collapsed, that is the ROM's behaviour, not a port bug (check
  on the PS2 before "fixing" it).


## An empty last.png after a long smoke run (2026-10-29)

- Symptom: after `tus.py check --run 240` the Read tool says `work/shots/last.png` is empty (0 bytes),
  while every `shot_<frame>.png` is fine. Cause: the run is stopped while the port is writing
  last.png. Fix: look at the highest `shot_<frame>.png` instead, and copy it over last.png before
  `tus.py finish` (progress_shot copies last.png into progress/).

## callcheck reads a column-0 label before "if (" as a function (2026-10-29)

Symptom: callcheck reports a function named "if" with every RoR call MISSING. Cause: a goto label at
column 0 directly followed by an if statement (label:
    if (...)) matches its definition pattern,
so the real function's body ends at the label. Fix: put another statement first after the label (or
restructure); other col-0 labels are fine.

## A ROM loop that walks past its array (2026-10-30)

Symptom: MagLifeMenuSpecial::render step 5 (0x12e178) clears m_attention on "actors" taken from
+0x50, +0x54, +0x58 -- m_actor_no[4], m_sel and m_cur[0] -- where Special2's twin walks the dials
(+0x44..). Cause: a ROM bug (the base pointer is the last actor, not the first dial); on the PS2 a
dial angle like -0x4000 as an actor index writes far outside m_actors. Fix: keep the ROM's three
words but skip any outside 0..127 (the host must not scribble), with a comment. Check a decompiled
loop's start pointer in the asm when its twin starts elsewhere.

## Models that pop at the 4:3 edges on a wide window (2026-10-30)

- Symptom: on a 16:9 window, a door (any model) past the original 4:3 picture's sides vanished and
  reappeared as the camera turned, though the extra width showed the rest of the world.
- Cause: the host widens every display camera by the view extend (rosepan_i3d.cpp REC_CAMERA ->
  RosePan_RendererWidenProjection), but the game culls in i3dViewTestBox / BoneBox / Sphere against
  the view record's 4:3 rectangle (I3dView m_rect_clip / m_rect_planes): a model wholly in the wide
  margin was I3D_VIEW_OUT before it was drawn.
- Fix: i3dCtxCreateView widens only m_rect_clip (clip x / y divided by the extend, as the host does)
  and m_rect_planes for views with a non-zero record offset (the off-screen shadow / close-up views
  have 0 and keep their shape). The projections stay the ROM's: i3rHostCamera and the CPU strips
  build from m_rect_proj / m_rect_viewport, so widening those would widen twice. Any new host-side
  widening (a new aspect knob) has to reach i3dViewHostExtend too.

## Translucent geometry that hides the player (2026-10-30)

- Symptom (the user, in the orphanage): light shafts drew over Jennifer so she could not be seen
  behind them; effects in general had depth trouble; her projected shadow sank into the floor in
  patches.
- Cause: the renderer (inherited from FF3, where AFAIL was always KEEP) ignored TEST's AFAIL and the
  2D replay never passed TEST or ZBUF on. RoR writes "no Z" through the alpha test instead of ZMSK:
  ATST NEVER + FB_ONLY (0x51001 / 0x31001, i3r m_fail_only 0x10) and its cut-outs' 0x5140d (GREATER
  0 + FB_ONLY), so every translucent mesh and effect sprite wrote depth on the host. The shadow
  receiver is its own model on the floor, which the PS2's 16-bit Z let pass GEQUAL and the host's
  float depth did not.
- Fix: architecture.md "Depth state". Method: log every distinct (TEST, ALPHA, ZBUF) the mesh path
  and the replay see (one line each, a std::set) over a smoke run -- 26 lines told the whole story.
  Read the TEST bits (ATE 0, ATST 1-3, AREF 4-11, AFAIL 12-13, ZTE 16, ZTST 17-18) before trusting a
  comment that a register value is "the default".
- Trap met on the way: a build in a git worktree (.claude/worktrees/<name>) does not find the
  sibling checkouts cmake looks in for shadercross and FFmpeg (../MioPan resolves inside .claude),
  so HLSL edits are silently not compiled -- the run uses the committed .spv / .dxil. Configure it
  with -DSHADERCROSS_EXECUTABLE=E:/Programming/Git/Github/MioPan/extern/shadercross/windows/shadercross.exe
  and check that resources/shaders/{spirv,dxil}/ changed before trusting a shader test.

## A flag index the EE wraps into its RAM mirror (2026-10-30)

- Symptom (the user, picking up an item after "There's a creepy doll tied to the pillar"): SIGSEGV
  in GamMan::endPcAction (0x1d4540) setting `g_sequence.m_bits[flag >> 5]`, flag = 0xffffffff.
- Cause: the scripts give every item -1 as its FCHRstat param 0x40 (seqFunc155 cases 3 / 4: no
  flag); endPcAction, the only reader, tests only `!= 0`. On the EE `m_bits + 0x1ffffffc` wraps
  32 bits into the uncached mirror at 0x20000000 and sets bit 31 of m_bits[-1], its BedMemory
  header size; on the host it is 512 MB past the array. Fix: skip a bit outside m_bits' 0xa00
  bytes, with a comment. Other `m_bits[n >> 5]` writers (seqcall.cpp's set / clear natives,
  gamman.cpp, mnuitem.cpp, mnupbook.cpp) have no such guard: the same -1 there would fault the same
  way, so check the value before blaming the reconstruction.
- A negative index into an EE array that the PS2 survives is a mirror wrap (0x20000000 uncached,
  0x30000000 uncached-accelerated): compute what the EE address hits before deciding what the host
  should do.
- Trap met on the way: gdbdrive died with "'utf-8' codec can't decode byte 0xa2" while writing
  crash.txt (`bt full` of a frame with the game's SJIS text; to_string decodes as UTF-8), and gdb
  -batch took the held game with it. gdbdrive now catches every exception, reruns an undecodable
  command through gdb's log file, and holds the game when run / continue itself errs.

## Disk writes in the mixer thread hold the IOP back (2026-10-30)

- Symptom: with ROSEPAN_AUDIO_DUMP set, the opening movie (ADPCM stream 47) opened at 184 s instead
  of 38 s; frames 1680..10560 were black and the log was silent for 2.5 minutes.
- Cause: the first version of the dump called fflush after every 256-frame block, inside
  MixBlock, which runs with the voice lock held. The IOP side (libsd calls from the RXSNDDRV tick,
  the stream DMA) waits on that lock, so every 5 ms a disk write stalled the driver, and the movie
  start, which waits on stream status, crawled.
- Fix: a 1 MB stdio buffer and no per-block flush (src/sdk/iop_voice.cpp). Anything new in the
  mixer thread (logging, file I/O, allocation) must stay off the voice lock or be bounded; when
  the boot timeline shifts, compare the time of "rxsnd : open ADPCM '47'" with a run without
  the change (38 s at ~60 fps on this machine).

## A placeholder light that looks plausible (2026-10-30)

- Symptom: the bus stop rendered neutral grey; on an emulator it is blue (the user). Nothing looked
  broken, so it survived a week of verify sessions.
- Cause: rosepan_i3dmesh.cpp lit every strip with one fixed grey direction (Shade) while the
  scene's light set (DogLightPool -> i3dCtxCreateLight -> I3rMeshPacket m_light) never reached the
  native draw. RoR's night scenes get their colour from the ambient and the moon light, not the
  textures. Fix: the mesh record copies the material packet's light set and the module lights as
  VU1 microprogram 1 does (architecture.md "Mesh lighting and fog").
- Same family: the renderer's fog came from ReiPan as "last value wins", read once at submit; RoR
  fogs per material packet, so it is snapshotted per draw now (ApplyGsDrawEnv). Any other global
  the renderer reads at submit instead of per draw has the same problem once RoR changes it mid-frame.
- When a host stand-in fills a gap the ROM fills with data (a fixed light, a default fog, a constant
  colour), name it as a stand-in in the code and in open-items.md, so the screenshot check does not
  mistake it for the real thing.

## Sounds cut short: a libsd register nothing modelled (2026-10-31)

- Symptom (the user, playing): one-shot effects stop early -- the door slam, Jennifer's scream;
  audiocheck shows each picture-book page-turn SE ending dead ~0.14 s in. The log looked clean.
- Cause: src/sdk/iop_libsd.cpp answered every SD_VP_* read from its write mirror, and nothing
  writes SD_VP_ENVX, so it read 0 for ever. RXSNDDRV acts on it: RxSndDrvVoiceTick (IOP 0x35c0)
  releases an auto-free voice (every SE and BGM note) after 9 ticks of ENVX 0, and
  RxSndDrvAdpcmUpdate (IOP 0xa3ec) ends a stream whose last data has come the first tick its voice
  reads 0, dropping the buffered tail. The driver matched the ROM; the host did not.
- Fix: ENVX comes from the voice engine's envelope (RosePan_VoiceGetEnvx: 0 once the voice has
  ended or its release has run out; 1 for a voice keyed on but not yet mixed, since the mixer works
  in passes). Any libsd register the ROM *reads* needs a model, not a mirror: check what a driver
  does with the value before trusting a stored 0.
- Stale lead met on the way: "sceSifSetDma: dst=0000000000101c40 ... transfer dropped" lines come
  from runs before 2026-10-30, when the silent stand-in iop_rxsnd.cpp handed out a fake IOP heap at
  0x100000 (under IOP_MAP_LIMIT). The reconstruction's buffers are real arena addresses (0x420000+).

## Host defaults that hide data: vertex alpha and sky depth (2026-11-04)

- Symptom (the user, playing): a butterfly display case's glass is a solid grey sheet; in some
  outdoor rooms the sky is drawn over the fogged distance.
- Cause 1: rosepan_i3dmesh.cpp passed `tri.alpha = 1.0f`, so the strip colours' w never reached the
  renderer. VU1 multiplies all four components (architecture.md "Mesh lighting and fog"); the
  case's glass is white at alpha 0, a quad the GS never shows. Lit by the scene it came out grey.
- Cause 2: drawSkyDome's view has zmin = zmax = 0, so the ROM's dome sits at Z 0, the clear value,
  and occludes nothing. The host camera ignores the screen z range, so the dome wrote its real depth
  and everything further away failed the depth test. Fix: `far_z` in rosepan_i3d.cpp (ZMSK).
- Scanning the data found these faster than reading code: walk MAPUNIT.BIN / CHARA.RPK's RTPK trees,
  list each I3D_BIN's material layers, unpack the strip colour blocks, and list what is not 1.0.
- Testing traps, each cost a long run:
  - ROSEPAN_I3D_MESH_TEST's camera mirrors the view, so back-face culling is inverted and a model
    draws inside-out; its header read (2048 bytes) only reaches the first entries of a big pack;
    models with a .MOT sit in their rest pose (the specimen case's lid over the glass).
  - The bus stop's sky is nearly black, so a screenshot diff with a threshold sees nothing even when
    the sky covers the backdrop. Paint the suspect draw a loud colour, and pull a dome in with a
    world scale (centred on the camera) to force the case.
  - A counter on "views with an empty z range" also counts the shadow casters.
