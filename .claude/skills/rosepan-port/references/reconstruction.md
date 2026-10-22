# Writing Rule of Rose code into RosePan

How a function goes from Ghidra into `src/`. Accuracy to RoR beats elegance: this is a reconstruction,
not a rewrite. Most rules here were learnt on ReiPan (FF3) and hold for any PS2 port; the GCC 2 ABI
sections are RoR's own. When something here proves wrong or incomplete, fix this file.

## Where code goes

- `ror_functions.txt` (export.py) gives each function's **inferred file**, and the TU key is that path
  under `src/`: `dog/dogactor.cpp` -> `src/dog/dogactor.cpp`. Keep RoR's directory layout (`bed/`,
  `debug/`, `dog/`, `emy/`, `game/`, `mag/`, `mcd/`, `mnu/`, `msg/`, `sequence/`, `snd/`, `spr/`,
  `system/`, `task/`, `utl/`) and RoR's own file names (`mnuSelectLanguage.cpp` keeps its case).
- **Anchored is a fact, filled is a guess.** A function that references `"dog/dogactor.cpp"` is in that
  file; functions between two anchors of one file, or a class's methods placed by majority, are not
  proven. Settle a TU's extent from the brief's identity section and pin it with
  `tus.py define <file> <lo>-<hi> --why "..."`.
- Files RoR does not name (no assert in them: the `rx` file / thread middleware just below the SDK
  libraries -- pinned as rx/rxsys.cpp, rx/rxfile.cpp, rx/rxthread.cpp -- `main` at
  0x2045a8, unanchored runs): a descriptive file in the directory the code serves (the middleware under
  `rx/`), recorded with `define --why` and in `architecture.md`.
- Inline methods and template instances (the named linkonce sections, data/layout.json
  `satellite_ranges`) have no file of their own: they are written in their class's header (inline, as
  the original was) or its .cpp, with their `ROR 0x...` annotation either way.
- Library code (data/layout.json `lib_ranges`: Sony SDK, kernel stubs, libgcc, newlib, the C++ runtime)
  is never reconstructed: `src/sdk/` replaces the SDK, the host libc the rest. Its names matter only so
  callers read right.
- Never put game code under `src/rosepan/` or `src/sdk/` (the platform layer). Platform changes are
  allowed when needed; keep them minimal and call them out in the commit summary.

## Every function carries its address

```cpp
// ──────────────────────────────────────────────────────────────────────
// One line on what it does and why, in RoR terms.
// ROR 0x0010eebc; DogActor's constructor (stores _vt$8DogActor at +0x94).

DogActor::DogActor(...)
{
```

- `ROR 0x<8 hex digits>` is load-bearing: `status.py` / `tus.py` count reconstructed functions by it,
  `callcheck.py` compares the callees at that address. Exactly one per function, in the comment block
  directly above it (blank lines allowed), on its own line -- never trailing a one-line definition (it
  then belongs to the next function), never in a prototype comment or a file header (the next
  definition inherits it), never two addresses on one line (`ROR 0xA / 0xB` counts only A). Refer to
  other addresses as plain `0x1e6b98`.
- Name functions exactly as Ghidra has them (`Class::method`; the source drops nothing). Renaming in
  code means renaming in Ghidra too (`name_ctx.py`).
- Globals and statics: note the RoR address (`/* bss 0x6e9800 */`, `/* sdata gp-0x7ff0 */`).
- Assert line numbers: the handler gets (file, line, expression); keep the ROM's line as a trailing
  `/* NNN */` on the statement that asserts -- it orders statements and anchors later matching.

## The GCC 2.9x ABI (RoR's compiler; FF2 used the same family)

- **Names** are mangled the GCC 2 way (`setId__8DogActori`); `gcc2.py` demangles. A member function with
  no parameters has nothing after the class (`getId__C8DogActor`, `C` = const); free functions are
  `name__F<params>`.
- **Constructors**: one function per constructor (no C1/C2 twins as in GCC 3), named
  `Class::Class`; it calls base constructors, stores the vtable, initialises members.
- **Destructors**: one function `Class::~Class(this, int flags)` (`_$_<class>`): it re-installs the
  class's vtable, destroys members and bases, and when `flags & 1` deletes the object (a tail jump to
  the operator-delete wrapper 0x105ba4, `if (p) free(p)`). Write a normal C++ destructor; the deleting
  part is what `delete p` compiles to on the host. Explain the invisible calls with
  `// callcheck: ~Base, FUN_00105ba4 (base destructor, deleting variant)`.
- **Virtual tables**: `_vt$<class>` holds 8-byte entries `{short delta, short index, void *pfn}`.
  Entry 0 is the type-info function `__tf<class>`; entry i (from 1) is the i-th virtual in declaration
  order, a virtual destructor taking one slot where it is declared. A virtual call in the decompile:
  `(**(code **)(*(int *)(this + VPTR) + 8*i + 4))(this + *(short *)(*(int *)(this + VPTR) + 8*i))` --
  slot i, `this` adjusted by the delta. `rtti.py --class X` lists the slots with their targets.
- **The vptr is not at offset 0.** GCC 2 places it after the data members of the first class in the
  hierarchy that declares a virtual function (`DogEntity`'s at +0x94, inherited by DogEntityEx,
  DogModel, DogActor, DogCamera; `EmyOut`'s at +0x34). `rtti.py` reads it from the constructors' stores.
  MSVC and MinGW put it at offset 0, so a polymorphic host class cannot keep the EE offsets (see
  "Types and layouts").
- **RTTI**: `__tf<class>` functions build `type_info` nodes on first use; `dynamic_cast` / `typeid` call
  the runtime (`__dynamic_cast`, `__rtti_si` ...). Never reconstruct `__tf` functions: the host compiler
  makes its own RTTI. A cast the ROM does through the runtime becomes `dynamic_cast` in source.
- **new / delete**: watch what `operator new` is (a game allocator or `__builtin_new` from libgcc);
  `new (operator_new(N)) T(...)` patterns in the decompile become `new T(...)` on the host, but never
  copy an EE size N into a host allocation (host objects are bigger: pointers, vptr).
- **Static initialisation**: four objects have a `_GLOBAL_$I$` initializer in `.ctors` (0x17f090,
  0x20439c, 0x2155d4, 0x2159f0; `.dtors` has the matching three). Each marks a file with file-scope C++
  objects; on the host the same `static T obj;` runs its constructor before main -- so that constructor
  must be real, not a stub (a stub logs before the logger exists).
- **Arguments**: EE GCC passes the first float arguments in f12.., whatever their position, ints and
  pointers in a0..t3 (a0 = `this` for members). Ghidra's argument order for mixed float / int
  signatures is unreliable: read the registers before the `jal`.
- **64-bit registers**: `long` / `u_long` are 64-bit on the EE (`ld` / `sd`); `(int)param` casts in
  decompiles are often 64-bit register noise. Quadword `lq` / `sq` move 16-byte vectors.

## Semantics

- Follow RoR's code. There is no source to port from; ReiPan and MioPan are references for the PS2
  side only (how the SDK behaves, how GS / VIF / VU packets are built, how the platform layer bridges
  them), never for game logic.
- **Assert macros**: an assert compiles to `if (!(expr)) assert_handler(__FILE__, __LINE__, "expr")`
  (handler 0x215a30: prints "Assertion Failed: %s(%d)   %s" and hangs). Write it back as the project's
  assert macro (`ROR_ASSERT`, src/common/ror_assert.h; explain the handler call with `// callcheck: FUN_00215a30 (...)`) with the ROM's expression text -- the expression names the members (`m_max_motion`,
  `m_top->m_num_file`) the code uses, which is how fields get their names. Keep the condition exactly;
  the host macro reports and continues (a hung assert helps nobody on PC; say so once in the macro).
- **Classes are real.** Every class with a vtable has its real name in the ELF; its virtuals are slots
  (`rtti.py`), and a function whose callers pass the object in a0 and that touches only its fields is a
  non-virtual member -- write `void DogActor::update()` called `actor->update()`, not a free function
  taking the object. Declare every member in the class with `// ROR 0x...` so stubgen can stub it.
  Inline methods named in the ELF are written inline in the header when the original inlined them
  (they were defined in the class body: that is why GCC emitted them as linkonce).
- Ghidra pitfalls: variadic calls lose arguments (recover them from `stage.py --asm`); comma-and-`&&`
  chains are ladders of `if`s; a callee Ghidra typed `void f(void)` loses its arguments in callers
  (read the asm before each `jal`); tail calls (`j` to another function) are calls (`features.py` lists
  them as tail jumps); switch tables on bytes decompile into goto soup (check a rewrite with
  `mipsemu.py --grid` over every input).
- Keep the game's constants as constants with a comment (sound ids, frame counts, states); name them
  as enums once their meaning is certain. Enums in RoR's symbols (`ECLR_UpDn`, `_EMY_STATE2`, `OID_STATE`,
  `SND_FUNC_NO`, `MSG_MODE`) are real names: use them.
- EE addresses in game code (load targets, packet buffers, scratchpad 0x70000000) go through the platform
  layer's host-pointer translation at the point of use. Hardware goes through the SDK replacements
  (`sceGs*`, `sceDma*`, `sceSif*`, libvu0) -- never talk to SDL from game code. Where a PS2 behaviour has
  to be emulated in game code, mark it `// PC:` with the reason.
- `printf` goes to the log; keep the game's messages (they are often the best names).

## Types and layouts

Types are rediscovered piece by piece; write each one down as code the moment reconstructed code
touches it, and make the compilers prove its layout.

- **Recreate the type, never an offset.** `*(int *)(obj + 0x1cc)` is not finished code: define (or
  extend) the struct / class and use the field. If a layout cannot be settled in the session, name the
  known fields and pad the rest (`u_char _1d0[0x38];`), never a bare offset.
- **Where:** the header of the TU that owns the object (its class, its constructor, its global), even
  when that TU is not reconstructed yet (create the header minimal). File formats: the loader's header.
- **Field names**: the assert expressions (`m_motion_data[n].m_i3m_root`), RoR's `m_` member style,
  then what the code does with them. Unverified fields say so (`// unverified`).
- **Size evidence**: constructor clears and copies, `memcpy` / copy-loop lengths, array strides
  (`base + i * 0x3c`), allocation sizes (the operator-new argument in the caller), distance to the next
  global in bss / data. Write the EE size in the header comment (`/* EE 0x220 */`).
- **Every struct carries asserts** right after it: `static_assert(sizeof(T) == 0x..)` and
  `static_assert(offsetof(T, field) == 0x..)` for each field reconstructed code touches (all of them for
  file formats and DMA data). Structs holding pointers cannot keep the EE size: assert the offsets before
  the first pointer and say `layout: host -- <why>` in the comment above, or keep EE addresses as
  `u_int` when the bytes come from a file or go to the hardware. `layoutcheck.py` (run by
  `tus.py check`, required by `finish`) compiles every game header on its own with MinGW GCC and MSVC
  and fails on structs without asserts.
- **Polymorphic classes** (a vtable in the ROM): the host vptr sits at offset 0 and is 8 bytes, GCC 2's
  sits after the first polymorphic class's data and is 4 bytes, so offsets after it differ. Write the
  class in C++ with its virtuals in EE slot order (`// EE slot 3 (+0x18)`), its members in EE order with
  their EE offsets in comments, `layout: host` above it, and `static_assert(offsetof(...) >= ...)` lower
  bounds where useful. Data that must keep EE bytes (a file image, a DMA packet) never lives inside such
  a class: give it a plain struct with exact asserts.
- **Host types** (the EE is GCC 2.9x MIPS; the hosts are MinGW GCC and MSVC on x64, both LLP64):
  - `long` is 64-bit on the EE, 32-bit on both hosts: EE `long` / `u_long` -> `int64_t` / `u_long`
    (uint64_t); never plain `long` (layoutcheck flags it).
  - Pointers: 4 bytes on the EE, 8 on the host.
  - 16-byte alignment: on the EE `sceVu0FVECTOR` / `FMATRIX` / `IVECTOR`, `u_long128` and anything moved
    with `lq` / `sq` are 16-aligned, which pads whatever precedes them. Use the platform layer's aligned
    typedefs (or `alignas(16)`), never a plain `float v[4]` (4-aligned, shifts every later field).
  - Bitfields: MSVC *and* MinGW (MS layout by default) open a new storage unit whenever the declared
    type changes; EE GCC packs by the declared type's alignment. One type per bitfield run, the type
    whose unit reproduces the ROM's bytes, and assert the size. For GS / VIF / DMA registers prefer the
    SDK's shift / mask macros.
  - Enums: 4 bytes everywhere, but when the ROM stores one in a byte / short use that integer type and
    name the enum in a comment.
  - `wchar_t` (EE 4, Windows 2), `long double`, `__attribute__((packed))` (GCC only; use `#pragma pack`
    on both compilers): not in game types. `char` sign matters: `s_char` / `u_char`.
  - Tail padding: MinGW reuses a base class's tail padding for the derived class's first field, MSVC does
    not: assert `>=` on derived host-layout classes.
  - Float literals: read every literal's bits from the ELF; EE-assembled constants are not always the
    host's round-to-nearest (`0x3e4cccd0` is not `0.2f`): write odd ones as hex floats.
  - EE float division by zero gives +-MAX (no inf / NaN) and `vsqrt` takes `|x|`: guard such spots with a
    `// PC:` helper where the ROM relies on it.

## Stubs and the frontier

- Called but not yet written -> `python stubgen.py --from-build work/build_relwithdebinfo.log` (run by
  `tus.py check`). It writes `ROSEPAN_STUB("Name", 0xADDR)` bodies (log once, return `{}`) into the
  callee's inferred file, so the build links and a run shows which missing pieces were reached.
- **Bridges** (`ROSEPAN_BRIDGE("Name", 0xADDR)`): temporary stand-ins that *work* but are not RoR's code
  (built from the platform layer or ReiPan / MioPan pieces), so later TUs run on real data. The tools
  count them as stubs; delete the bridge when its TU is reconstructed. Never write `ROR 0x...` in a
  bridge file.
- **Stub hazards**: a stub returns `{}` -- null pointers, zero counts, "not ready". A stub that leaves an
  out-parameter unset hands the caller stack garbage; a stub getter returning null crashes its first
  real consumer; a stub on a readiness path (`IsReady`, `IsLoadEnd`, `Wait*`) returning 0 hangs the
  caller's loop for ever. When a run hangs or crashes right after a TU, look at the stubs on that path
  first (hazards.md "Reconstruction bookkeeping").
- Declarations first: stubgen needs a prototype in a header (members: in their class, with
  `// ROR 0x...`). Constructors, destructors and data symbols are stubbed by hand.
- Replace the whole stub definition when reconstructing; the `// stub of ROR 0x...` comment goes with
  it. When a TU defines a function, grep `src/` for local declarations of its name (they are overloads in
  C++ and can silently keep linking to an old stub) and include the real header instead.
- The **frontier** (callees of reconstructed code not yet written) plus the stubs the last smoke run
  reached decide the next TU after the pinned queue (`tus.py next`, `status.md`).

## Verify and commit

`python tus.py check <TU> --run 15` does steps 1-4, `python tus.py finish ...` steps 5-6:

1. Build green (errors and undefined references summarised; undefined references become stubs).
2. `callcheck.py` on the TU's file -- no *missing* RoR calls, or a `// callcheck: A, B (why)` comment
   (names first, one parenthesis last, all on the `callcheck:` line). *Extra* calls are informational.
3. `layoutcheck.py` -- headers compile on their own under MinGW GCC and MSVC with their asserts holding,
   game sources compile under MSVC, every struct has asserts (or a `layout: host` note), no host types.
4. Smoke run (`build.py --run 15` with `settings.smoke_pad`; output in `work/run_relwithdebinfo.log`):
   reached stubs are listed and rank first for the next pick. **Look at the screen**: the run saves a
   frame every 60 presented frames as `work/shots/shot_<frame>.png` plus `last.png`; open them with the
   Read tool. Once a TU draws something, compare it with the game's own textures (`tm2png.py`). Say in
   the report what the shots show; a wrong picture is a bug to fix or record, never a pass.
5. `refresh.py` -- regenerates `ror_functions.txt` and `status.md`.
6. One commit per session, made by `finish` (refuses a red build, a failed layoutcheck and the main
   branch); its message lists the reconstructed and stubbed functions and the screenshot.
