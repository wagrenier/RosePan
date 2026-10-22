---
name: rosepan-port
description: The workflow for reverse-engineering Rule of Rose (PS2, US SLUS_214.48, no .text symbols, not Fatal Frame's engine) and porting it into RosePan one translation unit per session, as automatically as possible -- the TU queue (tus.py) and session protocol, Ghidra via GhidraMCP, the ELF's own linkonce symbols and vtables (GCC 2.9x ABI), library names matched from symbolised references, naming from context with provenance tags, staging decompilation, reconstructing C++ into src/, stubbing the frontier, building/running with the visual test, and keeping status, docs and this skill current. Use it for any work in the RosePan repo or on Rule of Rose code - "continue the port", "next TU", "tu dog/dogactor.cpp", "decomp X", "reconstruct", a RoR address or class name, Ghidra naming, stubs, build or run problems - even when the user does not name the skill.
effort: xhigh
---

# RosePan: Rule of Rose decomp + PC port

RosePan reconstructs Rule of Rose (US retail `SLUS_214.48`, built 2006-07-27, PS2 EE) function by
function into C++ that runs on a PC platform layer. It is the third port by the same author, after
MioPan (Fatal Frame 2, `../MioPan`) and ReiPan (Fatal Frame 3, `../ReiPan`): the tools, the session
protocol and the hard-won rules come from ReiPan, the PS2 platform layer from both. **Rule of Rose
shares no game code with them** (Punchline's engine, not Tecmo's): there is no counterpart to port
from, so every game function is reconstructed from RoR's own binary. What RoR gives instead:
- its section table: 1,449 inline methods / templates named by their GCC 2 mangled symbols, 284
  vtables named by class (`_vt$8DogActor`), 227 class names;
- 73 `.cpp` source paths (`dog/dogactor.cpp`, `mag/maglifepc.cpp`) and the assert expressions with
  member names (`m_main.m_no < m_max_motion`), passed to the assert handler with the line number;
- the Sony SDK 3.0 libraries FF3 links and the ee-gcc 2.96 runtime FF2's debug build has, both
  symbolised: `libmatch.py` names the library code word for word.

This skill is the playbook and the project's memory: follow it, and keep it true ("Keeping this skill
alive").

**Work is cut into translation units (TUs), one per session.** A TU is one RoR object file, rebuilt as
one source file under `src/` (key = its path: `dog/dogactor.cpp`). Each session takes one TU (or one
slice of a big one) from claim to commit; nothing carries over in the conversation, everything carries
over in the repo: `tools/reverse/data/tus.json` (state, handoff notes, history, queue), this skill's
references, and git.

Read before working:
- `references/status.md` -- generated: seeding state, naming, placement, TU summary (next pick,
  active/partial/blocked, recent sessions), reconstruction, the frontier.
- `references/open-items.md` -- the user's priorities ("Next up"), decisions, cross-TU questions.
- Then as needed: `architecture.md` (what RoR is made of), `reconstruction.md` (how code is written,
  GCC 2 ABI), `hazards.md` (traps, most learnt on FF3). PS2 / GS / VU / renderer knowledge lives in
  MioPan's skill (`../MioPan/.claude/skills/miopan-reversing/SKILL.md`: "Renderer architecture",
  "Diagnosing it does not render", "VU0 macro-mode idioms") and ReiPan's hazards.md "Platform layer":
  read the relevant section, don't copy it here.

## Environment

- Ghidra 12.1.2 with the GhidraMCP plugin (HTTP `127.0.0.1:8089`), project `C:\Users\willi\Ghidra`,
  program `SLUS_214.48` (path `/Rule of Rose - US`) open. The tools use HTTP through
  `tools/reverse/rosepan.py` (`program` always in the query string). Offline (Ghidra down): status,
  export, sections / libmatch plans, tus list/check/finish, callcheck and build work from cached `work/`.
- Tools: `tools/reverse/` (run from there: `python <tool>.py`); every tool is in its README.
- Build: `python build.py` (MinGW + ninja, configures on first use) once the platform bootstrap host
  task has created the PC project. The game ELF sits at the repo root (gitignored).
- References, read-only: `../ReiPan` (FF3 port: platform layer, tools' ancestors, SDK 3.0 library
  names), `../MioPan` (FF2 port: `symbols.txt` / `functions.txt` with the ee-gcc 2.96 runtime and SDK
  names, its skill for PS2 hardware).

## One TU per session (the protocol)

Invocations: `/rosepan-port tu <TU>` (a named TU), `/rosepan-port next` (pick one). Any other request is
regular work: use the tools below, still finish with a commit.

1. **Claim and read.** `python tus.py claim <TU>` (or `python tus.py next --claim`), then
   `python tus.py brief <TU>` and read `work/tu/<slug>/brief.md` -- handoff notes from the last session
   first -- and the staged bundle `work/stage/tu_<slug>/` (decompilation, class role, callers,
   callees, strings per function). The brief's **slice** is this session's work (reached stubs, then
   frontier, then their same-TU callees; ~10 KB of code). A **host task** (the brief says "Host task":
   a key like `setup/...cpp` with no RoR functions, queued with `tus.py queue <key> --top --task TEXT`)
   skips steps 2-5: do what its text says, then check and finish like a TU.
2. **Boundaries.** A TU's extent is *inferred* until pinned. RoR's own source-path strings are facts
   where a function references them; class membership and link-order fill between anchors are guesses.
   Read the brief's identity section (`tuid.py`): anchored count, classes and where they were placed,
   callers. Settle the extent and pin it: `python tus.py define <file> <lo>-<hi> [...] --why "..."` (hi
   exclusive; a pinned TU is exactly its ranges, so list every range). An `unassigned@<addr>` run gets a
   real file first: a file of RoR's directory layout (`dog/`, `mag/`, `emy/`, `task/`, `utl/` ...) the
   code serves, named after its classes when they say so. If the slice belongs to another file, define
   that, `tus.py claim` it, re-brief. Naming the file must not stall the decompilation.
3. **Names.** Every slice function and direct callee you need gets a real name: `ctxcard.py <addrs>` for
   evidence, a batch JSON in `work/batches/` (take the highest existing number + 1), `python
   name_ctx.py <batch>` (tag `ror-inferred` or `lib-context`, a one-line `why`; it saves and runs
   `refresh.py`). Methods are `Class::method`: a vtable slot's class comes from `rtti.py`, a
   non-virtual method's from its callers passing the object in a0 and the class's file. The ELF's
   symbols (`ror-symbol`) are exact -- names, classes and parameter types to build on. Re-brief if
   names changed.
4. **Header first.** `src/<dir>/<file>.h`: classes and structs at RoR's EE offsets with `static_assert`
   on size and every used offset (GCC + MSVC prove them), globals, prototypes of what other TUs call,
   each member declared with `// ROR 0x...`. Classes with virtuals cannot keep EE offsets on the host
   (GCC 2 puts the vptr after the first polymorphic class's data, the hosts at offset 0, and pointers
   double): `layout: host` with the EE offsets in comments, or a plain EE-layout struct for data that
   comes from files (reconstruction.md "Types and layouts"). Outgoing calls get prototypes in *their*
   TU's header (create it minimal) so stubgen can stub them. No `ptr + 0x120` in finished code.
5. **Code.** Reconstruct the slice following `reconstruction.md`: RoR semantics, `ROR 0x<addr>` above
   every function, the class's methods as members in its vtable order, the asserts written back as the
   project's assert macro with their expression text. Delete this TU's stubs that live in other files
   (the brief lists them). Slice done and context still light: take the next functions (`tus.py brief`).
6. **Check.** `python tus.py check <TU> --run 15` -- build, stub undefined references, rebuild,
   callcheck, layoutcheck (GCC + MSVC), smoke run with the scripted pad `settings.smoke_pad` (reached
   stubs steer the next pick). Repeat until green; missing calls fixed or explained in a comment.
   **Look at `tools/reverse/work/shots/last.png`** (and the `shot_*.png` sequence) with the Read tool.
   **The picture is the test** (the user's rule, from ReiPan): logs, stub hits and callcheck show the
   code ran, not that it is right. A TU on the drawing path is `done` only when the screenshot shows
   what the reference shows (`python tm2png.py <file.tm2>` or `--scan <archive>` -> the game's own
   textures in `work/ref/`); a wrong picture means `partial` with the difference in the notes. Complete
   code that nothing on screen can show yet (its caller or the draw path is a stub) finishes `verify`:
   not picked again, and the first session whose screenshot shows its output marks it `done`. When a run
   needs more than the boot, extend `settings.smoke_pad` (the pad syntax is in build.py) and say how far
   it now gets in open-items.md.
   **Drawing never goes through the emulated GS** (the user's rule): a draw TU calls the native host
   draw module for its subsystem and grows it (architecture.md "PC platform layer").
7. **Record.** Facts -> `architecture.md`; traps -> `hazards.md`; cross-TU questions / decisions ->
   `open-items.md`; better ways of working -> this file / `reconstruction.md` / the tools README. If
   writes under `.claude/` are denied (autopilot), put the text in `--notes` after "For the
   references:", one item per target file, ready to paste; an interactive session copies it in.
8. **Finish.** `python tus.py finish <TU> --status done|verify|partial|blocked --summary "<what was
   done>" --notes "<handoff: what is left, decisions, traps, where to start>" --coauthor "<the
   Co-Authored-By line from the session instructions>"`. It builds (refuses red or a failed
   layoutcheck), records the session, refreshes status, and commits everything on the current branch
   (`git add -A`: check `git status` for files that are not yours first; refuses `main`). Then report in
   3-5 lines and **stop** -- the next TU belongs to a new session.

`partial` is normal for big TUs -- use it only when code is left to write or a picture is wrong;
complete code that cannot be seen yet is `verify`. `blocked` means a decision is needed -- say which in
the notes and in `open-items.md`. Never leave the tree dirty or the build red.

## Starting sessions

The user starts each session: `/rosepan-port next` in a new conversation, the desktop app's scheduled
task `rosepan-next-tu` (its prompt is `tools/reverse/routine.md`), or `tools/reverse/autopilot.py` in a
terminal. A session never starts other sessions, agents or scheduled tasks, and never edits
`autopilot.py` or the routine (the user does). Within a session, run the protocol without asking at each
step; pause only for decisions that are genuinely the user's (restructuring the tree, non-trivial
platform-layer changes, destructive git) -- and when no answer can come, decide, record the doubt in
`open-items.md`, and finish. Pushing each finished TU's commit (`git push origin HEAD`) is approved
(the user, 2026-10-22; open-items.md "Decisions").
An empty queue means re-assess, not "follow the frontier": the frontier score counts call sites from
written code that may never run (in ReiPan it led seven sessions into effects nobody could see). Refill
the queue from open-items.md "Next up", in the order a smoke run reaches the stubs.
Porting runs at effort `xhigh` (this skill's frontmatter; the user's rule: automated runs at xhigh or
above, since lower effort reconstructs worse). Keep that line when editing the frontmatter.

## Tools by step

| step | tool |
|---|---|
| queue, claim, brief, boundaries, check, finish | `tus.py list / next / claim / brief / define / queue / check / finish` |
| regenerate derived files | `refresh.py` (fetch + features when Ghidra is up, sections, rtti, export, status) |
| map code | `modview.py range\|tree`, `ror_functions.txt`, `stage.py class:Name` |
| name | `ctxcard.py`, `name_ctx.py`; one-time seeding `sections.py --apply`, `libmatch.py --apply` |
| symbols | `gcc2.py` (demangle / mangle GCC 2 names), `sections.py --list FILTER` |
| read code | `stage.py` (bundles), `dec.py`, `ghidra.py` (asm, refs, mkfunc), `hidden.py` (functions folded into the body before them), `mipsdis.py` (offline asm), `mipsemu.py` (run a leaf function over every input), `sinit.py` (tables a static initializer fills) |
| classes | `rtti.py` (`--class NAME`, `--vtable ADDR`): bases, slots, ctors/dtors, vptr offset |
| TU identity | `tuid.py <TU>` (also in the brief) |
| game data | `datadump.py` (an ELF table -> C++ initializers), `tm2png.py` (TIM2 -> PNG references, `--scan` an archive) |
| frontier | `build.py`, `stubgen.py --from-build`, `callcheck.py` (all wrapped by `tus.py check`) |
| type layouts | `layoutcheck.py` (GCC + MSVC asserts, missing asserts, host-type lint; in `check` / `finish`) |
| crashes | `gdbrun.py`: the game under gdb; a fault holds it, `--cmd` inspects, `--break` stops a hang |

Naming exists to make reconstruction possible; it is not the goal. Name what the slice needs (its
functions and their direct callees), then write code; callees you do not need yet become stubs.

## Naming conventions (Ghidra)

- Provenance tags on every name: `ror-symbol` (the ELF's own section symbol: exact, never renamed),
  `lib-match` / `lib-match-low` (libmatch.py), `lib-context` (library / runtime function named by
  reading), `ror-inferred` (game function named from its code). `data/context_names.json` is the ledger
  with evidence; `data/overrides.json` lists library matches replaced by hand.
- Style: RoR's own, as its symbols show -- `Class::method` with lowerCamel methods (`DogActor::getId`,
  `MagLifeWindow::processPad`), some PascalCase families (`EmyMain::ActiveMove`); classes by module
  prefix (`Dog*`, `MagLife*`, `Emy*`, `Tsk*`, `Eff*`, `Gam*`, `mnu*`, `S2d*`). Library functions their
  real ABI names. Ghidra keeps `::` literally; no whitespace (`operator_new`); overloads carry their
  parameters (`DogModel::setHide(bool)`). Free functions: the module prefix of their file (`utl*`,
  `snd*`, `rx*`).
- A name states what the code does, not a guess about game meaning; put uncertainty in `why`. Assert
  expressions name members (`m_max_motion`): use them for fields, and the assert's file / line for order.

## Keeping this skill alive

This file and its references are the project's memory across sessions. Updating them is part of
finishing any TU, not optional cleanup:

- **Generated facts** (`status.md`, `ror_functions.txt`): never hand-edit; `refresh.py` (run by
  `tus.py finish`) regenerates them.
- **Per-TU state and handoff** -> `tus.json` via `tus.py finish --notes` (and `define --why`). Write notes
  for a reader with no context: what is left, what was decided, where to start.
- **Established facts** about RoR (entry points, classes, structs, globals, a module's role, address
  ranges) -> `architecture.md`, with the RoR address. Replace what it supersedes; no contradictions.
- **Unconfirmed guesses and cross-TU questions** -> `open-items.md`, and resolve them there.
- **Anything that burned time** -> `hazards.md` as symptom / cause / fix.
- **A better way of working** -> this file or `reconstruction.md`, and `tools/reverse/README.md` for
  tools. If an instruction here turns out wrong, fix it in the same commit that discovered it.
- **New automation**: when the same multi-step manual work happens twice, write a script in
  `tools/reverse/` (use `rosepan.py` for paths and Ghidra access), document it in the README and here.
  Each TU should take less hand work than the last. ReiPan grew game-data tools this way (event-script
  decoders, table extractors, a warp / snapshot debug window); RoR will need its own.
- Keep this file under ~250 lines; move detail into references and point to it.

## References

| file | what | when |
|---|---|---|
| `references/status.md` | generated progress, seeding, TU summary, frontier | start of every session |
| `tools/reverse/data/tus.json` | TU state, ranges, handoff notes, history, queue | via `tus.py` |
| `references/open-items.md` | priorities, decisions, cross-TU questions, done log | choosing work; after finishing |
| `references/architecture.md` | RoR binary, address map, toolchain ABI, modules, classes, data | understanding a TU |
| `references/reconstruction.md` | file placement, annotations, GCC 2 ABI, types and layouts, stubs, verification | writing code |
| `references/hazards.md` | Ghidra / decompiler / layout / bookkeeping / shell / platform traps | when something behaves oddly |
| `tools/reverse/README.md` | every tool, pipeline, provenance tags, platform contract | running tools |
