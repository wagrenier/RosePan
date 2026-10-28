# Rule of Rose architecture -- what is known

Living document: add facts as they are established, correct them when they turn out wrong, cite the RoR
address. Unverified guesses belong in `open-items.md`, not here. Established 2026-10-22 from the ELF
(sections, strings, libmatch.py, rtti.py) unless a later date says otherwise.

## Contents
- Binary and toolchain
- Address map
- Naming sources
- Program flow
- The rx file layer
- The rx thread layer
- Modules (RoR's own source files)
- Classes
- Key functions and globals
- Disc and data files
- PC platform layer
- The title screen (mnu/mnutitle.cpp)
- The task manager (task/tsk_man.cpp)
- The script tasks (task/tsk_chr.cpp)

## Binary and toolchain

- `SLUS_214.48`: Rule of Rose, US retail (Atlus; developed by Punchline), PS2 EE (MIPS R5900), file dated
  2006-07-27; `main` prints the build date "Jul 27 2006" "22:25:45" and a version "0.1.%sn.%s.%s".
- Entry 0x100008 (`_start`, Sony crt0), **gp = 0x30fff0** (`.reginfo`). Ghidra shows gp-relative
  globals as offsets from it and keeps no xrefs for them.
- **Compiler: an ee-gcc 2.9x** (GCC 2 C++ ABI: `name__<class><params>` mangling, `_$_` destructors,
  `__tf` type-info functions, 8-byte `{delta, index, pfn}` vtable entries, `.gnu.linkonce` sections) --
  the same family as FF2's 2.96-ee, not FF3's GCC 3. `.mdebug.eabi64` is empty: symbols were stripped,
  **but the section header table was kept** (1,795 sections), so every linkonce section still carries its
  mangled symbol. Game code is C++ (`.cpp` paths throughout).
- Sony libraries (version strings in the ELF): libcdvd 3000, libdbc 3020, libdma 3000, libgraph 3000,
  libipu 3000, libkernl 3000, libmc 3020, libmpeg 3000, libpad2 3020, libscf 3000, libvib 3000,
  libvu0 3000 -- the SDK 3.0 versions FF3 (SLES_538.25) links, except libpad2 / libdbc / libvib, which FF3
  and FF2 do not use (no reference: their functions stay unmatched).
- IOP modules the EE loads: sio2man, padman, mcman, mcserv, cdvdman, cdvdfsv, fileio, sifcmd, usbd
  (`.irx` names), after rebooting the IOP with `ioprp300.img` (main). No game-specific IRX name appears.
- 44 VU1 microprograms in `.DVP.overlay..*` sections (Ghidra makes 50 overlay spaces of them and the
  other DVP sections); `.vutext` 0x291230..0x2953b0.

## Address map

| range | contents |
|---|---|
| `0x100000`-`0x100258` | crt0 (`_start` 0x100008, `_exit`) |
| `0x100258`-`~0x24b490` | game code; ends with the unsymbolised "rx" middleware: `rx/rxsys.cpp` 0x23ff58..0x2437f8, `rx/rxfile.cpp` 0x2437f8..0x249b88, `rx/rxthread.cpp` 0x249b88..0x24b490 (all pinned); below 0x23ff58 an IOP RPC client (0x23f960.., unprefixed "binding IOP module" strings) whose owner is open |
| `0x24b490`-`0x280e88` | Sony SDK 3.0 libraries (libmc, libmpeg / IPU / CSC, libdbc, libpad2, libvib, libdma, libgraph, libvu0, devvu0 / devvif), EE kernel stubs and libc (klib, intr, thread, sif, tty, deci2, filestub, initsys), libgcc2 / fp-bit / dp-bit, newlib (`data/layout.json` lib_ranges) |
| `0x280e88`-`0x29122c` | named `.gnu.linkonce.t` sections: the game's inline methods (satellites, ride with their class) |
| `0x291230`-`0x2953b0` | `.vutext` (VU microcode) |
| `0x2953b0`-`0x295eb8` | named linkonce sections of the C++ runtime (type_info, `__user_type_info`, `__si_type_info`, `__class_type_info`, exception, bad_cast ...): library |
| `0x295f00`-`0x2e014c` | `.data` |
| `0x2e014c`-`0x2e0a7c` | `.ctors` (4 initializers: 0x17f090, 0x20439c, 0x2155d4, 0x2159f0), `.dtors` (3), `.eh_frame` |
| `0x2e0a80`-`0x2e5100` | named `.gnu.linkonce.d._vt$<class>` sections: the 284 vtables |
| `0x2e5300`-`0x307f70` | `.rodata`; `.gcc_except_table` 0x307f80 |
| `0x308000`-`0x828e94` | `.sbss` 0x308000, `.bss` 0x308100 (5.1 MB) |

## Naming sources

Seeded 2026-10-22 (`setup/ghidra_seed.cpp`; snapshot of the names before it:
`work/snapshots/20261022_094006.json`). Ghidra went from 5,728 to 7,370 functions.

| source | count | tool | tag |
|---|---|---|---|
| the ELF's named linkonce sections (inline methods, templates, ctors / dtors / `__tf`) | 1,449 named (1,411 created; 37 existed as `FUN_`, 1 trial) | `sections.py` | `ror-symbol` |
| vtable slot targets in .text (the out-of-line virtuals), class known, name not | 763 (95 created) | `rtti.py`, `sections.py --apply` | -- |
| tail-jump targets after a `jr ra` that Ghidra had folded into their callers | 21 created (0x1bdc94, 0x105434, 0x173610.. ; 3 more targets are labels inside functions) | `sections.py --apply` | -- |
| library / runtime code matched to FF3's SDK 3.0 libs and FF2's ee-gcc 2.96 runtime | 687 planned: 544 `lib-match` + 133 `lib-match-low` applied (114 functions created), 2 existing names kept (`_StartThread`, and `entry` -> `_start` by hand), 5 skipped as merged reference bodies / duplicate claims | `libmatch.py` | `lib-match` / `-low` |
| named by reading during the seeding | 10: `main`, `_start`, `_exit`, `Exit`, `sceSifLoadElfPart`, `sceSifLoadElf`, `kCopy_initsys`, `kFindAddress`, `Copy_exit`, `kCopy_exit` (4 overrides of FF3-reference names) | `name_ctx.py` | `ror-inferred` / `lib-context` |
| Ghidra's own syscall-stub names | 76 | Ghidra analysis | (none) |
| source-path strings anchoring functions to files | 473 functions, 73 `.cpp` files; 1,221 game functions placed | `export.py` | -- |

## Program flow

- `_start` 0x100008 (crt0): `_InitSys` 0x267798, `FlushCache`, the init hook (`jalr s0`), `atexit`
  0x2781e8, then `main` 0x2045a8 (`jal` at 0x100238), then exit.
- `main` 0x2045a8 (`main.cpp`, 0x2043dc..0x204d38, written 2026-10-22; `mainGame` in src): `__main`,
  `mainParseArgs` 0x2043dc (`-l fre|ger|spa|ita` -> `MagSystem::setLanguage` 1..4, `-m N` / `-mN` -> the
  boot stage, 99 by default), `snprintf` 0x279b70 of the version line "0.1.%sn.%s.%s" (number "1.01" from
  data 0x2d9e18, `__DATE__`, `__TIME__`) into bss 0x7b45d0, the rx bring-up below, `utlMemInit` and the
  memory hooks, `strcpy` of argv[0] into `SndMan` +0x4, `rxSysSetDebugHandlers` 0x242960,
  `rxSysInitFileDevice(1)` 0x243608, then `mainInitGame(stage)` 0x2046d4, `mainLoop` 0x204864 (never
  returns), `mainExit` 0x20483c.
- `mainInitGame`: `mainInitDevices` 0x20499c (`SndMan::init` -> `rxSndInit` 0x237d80, `sceDbcInit`,
  `scePad2Init(0)`, `sceMcInit`), `mainInitGraph` 0x20491c (NTSC interlaced frame, a 640x224 double
  buffer), `sysInitManagers` 0x204148 (`Sequence::init`, `GamMan::init`), `MagSystem::init`; then a new
  `SysNowLoading` (0x2a0 bytes): `init`, `MagSystem::setSceneFlags(1,1,1,1,1,1,0)`,
  `MagScene::initCamera` (empty), `MagSystem::start` (V-blank semaphore and callback 0x117654),
  `seqSetLanguage`, `GamMan::loadSystemData` 0x1c1b48, `GamMan::loadStage(stage)` 0x1c204c (unless -1),
  `term` + delete; bss 0x7a3d48 / 0x7a3d4c = 1, data 0x296198 = 2, `MagSystem::resetDBuff` twice. The
  now-loading frame itself is drawn (0x2051cc) from 0x1be564, inside the pfh loader GamMan's loads use.
- `mainLoop`: `SysTimer::start` on the task manager's timer (0x7a3dc0 = 0x7a3d80 + 0x40),
  `CmnTaskMan::addTask(1, -1, 1, 0, &EmyManager 0x7b2140, 0, &id 0x7b45c0, 0)`, RCNT0 mode 0x83, then
  per frame: RCNT0 = 0, `SysTimer::update`, `Sequence::exec` 0x20f98c, `GamMan::update` 0x1c526c,
  `CmnTaskMan::exec` 0x1e7a20, `MagSystem::update` 0x1182dc (the V-blank wait, draw, frame counter),
  `GamMan::postUpdate` (empty).
- **The globals file** before main.cpp: `system/system.cpp` (pinned 0x203d94..0x2043dc, written
  2026-10-26; no source string, the file name is a guess). Its static initializer 0x2041f8 (entries
  `_GLOBAL_$I$` 0x20439c / `$D$` 0x2043bc) constructs, in order, and destroys in reverse: game heap
  `BedMemory` 0x6e9800, `MagSystem` 0x6e9840, `GamMan` 0x7812c0, `CmnTaskMan` 0x7a3d80, `Sequence`
  0x7a4440, `EmyManager` 0x7b2140, `SndMan` 0x7b2580, `RxMpeg` 0x7b40c0. In src the six managers are
  defined in system.cpp (the heap stays in utl/utlmisc.cpp, g_rxMpeg in snd/rxmpeg.cpp). Also there:
  `sysInitManagers` 0x204148 / `sysTermManagers` 0x204174 (Sequence then GamMan), `sysExecHostElf`
  0x2041a0 (debug restart from host0:main.ee.elf, no callers), and **SysTimer** (0x2c bytes, all words,
  EE layout kept; the task manager's +0x40): a ms timer over `GamMan::getSysTime` (m_vcount x 1000 /
  25, inlined): ctor 0x203d94 -> reset 0x203dd0, start 0x203ea8 (adds the stopped time to +0x28),
  stop 0x203efc (no callers), read 0x203f38 (adds |now - prev| to +0xc), update 0x204014 (main, once a
  frame: +0x18 count, +0x20 time = elapsed - paused, the old one to +0x1c / +0x24), hold 0x204090
  (CmnTaskMan::exec: +0x24 = +0x20). The code before it, 0x1fd424..0x203d94 (still
  unassigned@001fd424), is enemy behaviour tasks: TskEnemyMove (vtable 0x2e5040), TskEnemyTurn
  (0x2e5080) and their motion / turn helpers -- a task/ file (or two).
- The rx bring-up main runs, in order: `rxSysSetIopImage("IOPRP300.IMG")` 0x241b20,
  `rxSysSetBootPath("cdrom0:")` 0x241d68 (boot device CDROM, dir, name), `rxSysResetHardware` via
  `rxSysInit` 0x240eb8: VIF/VU/GS-path/DMA reset, `rxThInit`, memory hooks reset, boot device (more than
  32 MB = devkit = HOST, else CDROM), `rxSysInitModules` (pools, `rxFileInit`, the rxfile thread,
  `rxThAddCallback`), file devices and dir, `rxSysLoadIopModules` (default module table, SIF, CD,
  IOP reboot with `modules/ioprp300.img`, then every `.irx` in `modules/iopconf.cnf` via
  `rxFileLoadModule`), `system.ini` [SYSTEM] NUMBER / VERSION / VMODE / COUNTRY / LANGUAGE / WARNING
  (optional), the save-data name = region prefix (BI / BA / BE by COUNTRY) + NUMBER. Then main installs
  the game's memory hooks (`rxSysSetMemFunc0..3`: 0x105ca0, 0x105ca0, 0x105de0, 0x105e08).

## The rx file layer (`rx/rxfile.cpp`, 2026-10-22)

- **Drivers** (bss 0x7df9c0, 9 code pointers each, indexed by RX_SYS_DEVICE): makePath, getStatus,
  control, getSize, create, read(name, buf, offset, size), write(name, offset, buf, size), setRoot,
  getRoot. HOST 0x7df9c0 (root bss 0x7dfa50, default "host0:"), CDROM 0x7df9e4 (root fixed
  "cdrom0:"; "\" and ";1" sit next to it at 0x2dc1c4 / 0x2dc1c8: the ISO name form, rxFileCdIsoName;
  create / write return -2), HDD 0x7dfa2c (a host0: copy whose
  errors -5 / -6 / -19 / -65531 remap HDD to its fallback device, data 0x2dc238 = {0,1,2,-1}, and call
  the error hook 0x2dc248). MCARD has no driver. `rxFileCallDevice` = the driver's setRoot (rxsys passes
  the boot path's device part).
- **Contexts** ("thread type" 0 / 1, bss 0x7f1ec4, index 0x7f1ec0): device + load hook
  (default 0x245ae8: whole-file read through rxsys's 0x243788.. async calls) + size hook (default
  0x245a48). `rxFileGetCurrent` = the current context's device.
- **Dirs** (bss 0x7f1edc, 0x128c per device): param word (-1), dir (lower-cased, '/'), and per context
  a search dir, a 0x800-byte name stack (top 0x880, 0x7ff empty) and a last name. `rxFileMakePath` =
  dir + search dir + name ("/" makes either absolute) behind the driver's root.
- `rxFileLoad` / `rxFileLoadModule` / `rxFileRebootIop` open through rxsys's print-locked sce*
  wrappers (`rxSysOpen` 0x242528 .. `rxSysLseek` 0x242660). `rxFileLoad` reads size + 0x80 bytes.
- **Async file slots** (bss 0x7dabb0, 64 x 0xd0, fd 1..63; pools for markers 0x7de3b4, ring segments
  0x7de9b8 and tags 0x7ddfb0, threaded by rxSysInitModules): `rxSysStreamOpen(name, flags)` 0x243788 ->
  `rxFileOpenEx` 0x2442e8 -> `rxFileOpenSlot` 0x244410, which posts a create (SCE_CREAT, then never
  sized) or size request and waits unless flag 0x8000 (RX_FILE_NOWAIT). Flag 0x10000 (RX_FILE_RING):
  the file thread keeps the slot's ring buffer filled in whole sectors while filled <= threshold;
  `rxFileGetRingData / RingConsume / GetRingFilled` read it, segments map ring bytes to file positions.
  A slot opens *paused* (`rxFilePause(fd, 0)` starts the refill). Markers {pos, active, id, count, max,
  jumpPos, callback, user} split reads and jump the position (stream loops); tags {pos, active, id,
  value} are reported per ring window (`rxFileGetRingTags`). The default load / size / save hooks
  (0x245ae8 / 0x245a48 / 0x245be0) are open + seek(0, 2) + read / write + close on a slot. Users
  outside rx: snd (sndman, rxmpeg) and the code at 0x17fd58..0x182c08 and 0x1be36c..0x1be8a8.
- **The file thread** (bss 0x7f6910, one; `rxFileCreateThread(0, 2, rxFileThreadProc)` via rxThCreate
  with a 64 KB stack): two 64-entry queues of 0x2c-byte requests {state, device, path, op 0 create /
  1 size / 2 read / 3 write, offset, buf, size, retryMissing, busy, callback, owner} -- queue 0 for
  opens and ring refills, queue 1 for rxFileRead / rxFileWrite (split into the device param's chunk
  size). Device -> thread table 0x2dc250 (all 0); requests go through the device map 0x2dc228 to the
  driver. A failed request stays queued and is retried after the next proc call (a -2 "not found"
  only when retryMissing: async opens wait for the file to appear). Before every request the thread
  runs `rxFileThreadProc(priority + 1)` -> `rxFileCdProcess`. Stop: `rxFileStopThread` 0x249000.
- **The cdrom driver reads ISO 9660 itself**: `rxFileCdFind` (pointer 0x2dc1dc, replaceable by
  `rxFileCdSetFindFunc`) reads the primary volume descriptor at 0x8000 (path table 0x808c / 0x8084),
  walks the path table by parent number, scans the last dir's records for "NAME;1" (`rxFileCdIsoName`:
  "\" + upper case + ";1"). Reads go through a 32-sector LRU cache (nodes {prev, next, buf, sector} at
  0x7dfb80, buffers 0x7dfdc0, free list 0x7dfd90, used list sentinel 0x7dfd80) and `rxFileCdReadBytes`
  (64-byte-aligned sector reads plus a 4-sector tail buffer 0x7efdc0). The first read waits for the
  disc, sets sceCdMmode (DVD for 0x14 / 0xfe) and reads the dual-layer info (layer 1 start added to
  every LSN when layer 1 is selected, `rxFileCdSetLayerMode`). A repeated read at the same offset first
  reads 32 sectors earlier (forced seek). Once a lookup read the disc, the thread polls sceCdStatus every
  0x400 steps (shell open: cache dropped, status 0 set); `rxFileCdControl(0, open)` queues tray
  requests; `rxFileCdSetClockRead(1)` makes it read the drive clock every 30 ticks
  (`rxFileCdGetClock`). **On the host this needs an ISO image behind sceCdRead** (open-items.md).
- `s_verbose` data 0x2dc1b0 = 1: the retail build prints every rxfile message.

## The rx thread layer (`rx/rxthread.cpp`, 0x249b88..0x24b490, 2026-10-22)

A scheduler over the EE kernel's priority threads (all 53 functions written, `verify`):
- **Priorities**: base = the main thread's current priority at `rxThInit` 0x249ea0 (sdata 0x2dc264);
  offsets `s_prioIndex` sdata 0x2dc278 = {0, 2, 4} for main / worker / rx threads + idle
  (`rxThSetPrioIndex` 0x24b048 keeps them increasing).
- **Threads**: the idle thread (stack bss 0x818240, 0x400, spins) and the worker (stack 0x808240,
  0x10000, suspended except while the main thread yields; runs the 32-entry call queue bss 0x807ff0 that
  `rxThPostCall` 0x249e08 fills -- snd/rxmpeg.cpp's user). Up to 8 rx threads in slots bss 0x807f70
  `{short id, short state, int count, int priority, void *param}`; `rxThCreate` starts them at the low
  level with state 0 (not dispatched until `rxThSetState(i, 1)`).
- **Semaphores** bss 0x807f40: yield (max 1; the main thread waits on it), dispatch (a counter:
  `rxThDisableDispatch` signals, `rxThEnableDispatch` polls), request lock (the worker's
  `rxThLockRequest` / `rxThUnlockRequest`).
- **The dispatch alarm** `rxThDispatchAlarm` 0x24a678, every `s_interval` HSYNCs (sdata 0x2dc274 = 4,
  `rxThSetInterval` clamps 4..256): unless dispatch is disabled, the next dispatched slot gets the CPU on
  every (priority + 1)-th pass that lands on it (so "priority" is a slowdown); then, if the main thread
  waits and the request lock is 0, it polls the request (sdata 0x2dc270, args bss 0x8081f0) and signals
  the yield semaphore when it answers non-zero. Re-arms while rx threads exist or a request is pending.
- **rxThYield** 0x24a508 on the main thread: request "vblank count changed" unless one is set, run the 8
  yield callbacks (bss 0x808200; rxsys installs `rxSysThreadCallback`) with 1, resume the worker, wait,
  suspend it, callbacks with 0. On an rx thread: lower itself below the main thread. `rxThVSync`,
  `rxThGsSyncPath`, `rxThDmaSync` (mode 0) are yields with their own requests.
- **Frame pacing**: `rxThVBlankInit` 0x24b278 adds an INTC cause-2 handler (vblank count sdata 0x2dc290,
  field 0x2dc288 from GS CSR when interlaced) and runs timer 0 on HBLANK. `rxThSync(mode)` 0x24b3f8:
  -1 = the vblank count, 0 = wait one vblank, 1 = no wait, n = wait until n vblanks since the last read;
  each returns the HSYNCs since the last read (`rxThReadHSync`).
- `rxThFindCallback` 0x24a218 always returns -1 (a delay-slot bug): duplicates are not refused and
  `rxThRemoveCallback` removes nothing.

## The block heap (`bed/bedmemory.cpp`, 0x100354..0x101718, 2026-10-22)

- `BedMemory` (EE 0x1c): +0 `m_buffer` (first block; the assert names it), +4 set to the first block by
  format (no reader), +8 the buffer as given (destroy frees it when +0x18 `m_owner`), +0xc `m_size`
  (multiple of 16), +0x10 `m_id` (low 3 bits = default align mode), +0x14 live block count.
- Blocks on a doubly linked list, each led by a `BEDMEMHEADER` (EE 0x20): +0 `m_next`, +4 `m_prev`,
  +8 `m_chunk_size`, +0xc data pointer, +0x10 short (the heap id, written by format only), +0x12
  `m_flag` (1 used), +0x14 id / +0x16 line (-1 none) / +0x18 file name (allocDebug's tags), +0x1c
  `m_header_size` = data - header. The word before the data always holds that distance: getHeader
  0x100ce4 finds the header from a pointer with it.
- alloc 0x100a3c(size, flags): size rounded up to 16; flags & 7 = align mode (0 0x20, 1 0x40, 2 0x80,
  3-6 4 bytes, 7 the heap default); (flags & 0x1c0) == 0x100 searches from the end (findFreeFromEnd /
  splitTail), else first fit from the start (findFree / alignBlock). A rest of 0x40 bytes or more
  becomes a free block. Every allocation is memset 0; format fills a new buffer with "$RUNTIME".
- free merges with a free next and previous block; realloc shrinks in place (shrink 0x100d30), grows
  into a free next block, or allocs + copies + frees. getInfo 0x1015fc returns {largest free, free total,
  m_size}. trimFront 0x100ecc has no callers (and computes size - new - old).
- ROM slips: realloc's grow-in-place leaves h->m_next at the swallowed header, shrink next to a used
  block leaves next->m_prev at h, splitTail applies alignBlock's shift twice (-1 included). The host
  mends the first two (`// PC:`), keeps the third.

## The pad (`bed/bedpad.cpp`, 0x101aec..0x102490, 2026-10-25)

- `BedPad` (0x90, no vtable) is one libpad2 socket: `init(port)` opens it on the static DMA buffer
  `zDmaBuf` 0x308280 (0x100 per port; scePad2SocketParam option 2, port, slot 0), `term` deletes it,
  the destructor terms an inited pad. `update` 0x101ccc (MagSystem, twice per frame path) reads only on
  `scePad2GetState == 1`, straight into the object (+0x00 buttons active-low, +0x02..+0x05 right X / Y,
  left X / Y), and builds the views: +0x78 `m_hold`, +0x7a `m_trig` (pressed this frame), +0x7c
  `m_repeat` (trig, plus `m_hold` every `m_repeat_interval` +0x38 frames once held `m_repeat_wait`
  +0x34 frames; MagSystem sets 15 / 10), +0x28 last hold, +0x2c hold count, +0x30 frame count, +0x7e
  the stick bytes (0x80 when disconnected). Bits are the report's: 0x1 select, 0x8 start, 0x10 up,
  0x20 right, 0x40 down, 0x80 left, 0x2000 circle, 0x4000 cross.
- `updateDir` 0x102188: d-pad, else left stick past 0.7 (dead zone 0x60..0xa0, scale 96 / 95) -> a
  second view in d-pad bits, +0x68 held / +0x6c trig / +0x70 `m_dir_repeat` (menu cursors: MnuTitle,
  MnuMcd), vector +0x4c / +0x50 (y negative up), length +0x54, quadrant +0x60 and a wiggle counter
  +0x64 (quadrant changes < 15 frames apart). `calcDirChange` (+0x58) compares with the latched
  direction +0x40..+0x48 (MagSystem::setPC clears it); update latches when it is >= 0.001.
- Accessors other modules call: `getRightX/Y` 0x101e64 / 0x101eec, `getLeftX/Y` 0x101f74 / 0x101ffc,
  `getDir(&x, &y)` 0x102084 (returns the length; the player / Brown control 0x11ac24.. and seqFunc019),
  `getDirChange` 0x10209c.
- `setVibration(small, big)` 0x101b50 -> libvib `sceVibSetActParam` 0x26bb10 (profile bit 0 small
  on / off, bit 1 big 0..255; a negative argument leaves that motor out). libpad2 names placed:
  scePad2CreateSocket 0x26b280, DeleteSocket 0x26b388, Read 0x26b3e0, GetState 0x26b588.

## The system object (`mag/magsystem.cpp`, 0x117654..0x1194b4, 2026-10-22)

- `MagSystem` (bss 0x6e9840, EE 0x97a80): +0 frame count (bit 0 = draw buffer), +4 motor frames,
  +8 vibration option (`utlIsVibrationOn` 0x103484 = GamMan option 0x12), +0xc `m_info` (0x28: heap
  largest / total free, packet size / rest / overrun "m_trash_*", RCNT0 times), +0x34
  `m_store_image_filename` / +0x38 request (endPacket dumps the frame as a 512x448 BMP), +0x40 two
  `BedPad` (0x90 each; bed/bedpad.cpp, libpad2 socket +0x20, port +0x24), +0x160 VIF1 `sceDmaChan`,
  +0x170 `sceGsDBuff` (512x448, PSMCT32, Z PSMZ16S, frame pages 0x70 / 0, Z at 0xe0), +0x3a0 the i3d
  context (0x12168), +0x12508 the packet buffer (static bss 0x308580, 0x35c000 = two frames of
  0x1ae000), +0x12540 `MagScene` (0x85500), +0x97a40 two pad slots, +0x97a60 PC actor, +0x97a64 Bwn
  actor, +0x97a68 last PC actor, +0x97a6c language / +0x97a70 language still free.
- Statics: sdata 0x296190 `m_current_system`, 0x296194 V-blank count, data 0x296198 V-blanks per frame
  (2; main sets 2 again), 0x29619c `m_v_sema` (-1). The callback 0x117654 counts V-blanks, passes the
  count to `rxMpegSetVCount`, and signals the semaphore every 2nd one: **the game runs at 30 fps**.
- The frame (`update` 0x1182dc): motor countdown, `BedPad::update` x2, `RxMpeg::setFrameParity`,
  `MagScene::update(pad0, pad1)`, `SndMan::update`, `i3dCtxBeginFrame`, `MagScene::draw(ctx, dbuff,
  frame)`, `postDraw`, assert rest != 0, endPacket (rest / overrun / heap info, `i3dCtxEndFrame`),
  waitVSync (rxThVSync while a movie plays, WaitSema + PollSema drain, VIF1 DMA sync, `sceGsSwapDBuff`
  0x2599e8), kickPacket (`sceDmaSend(VIF1, i3dCtxGetPacket)`), frame++, `utlIncFrameCount`.
- **i3d** (unassigned@00216ad0 up to rxSnd 0x237d80; "i3d_package_version 050121-stable-050826"):
  the packet / draw library under the scene. initGraph compares the game's copy of its 36 struct sizes
  (rodata 0x2e90a8) with `i3dGetStructSize` 0x21f1a8 (data 0x302ba0) and hangs on a mismatch.
  Files pinned so far (2026-10-22): **`i3d/i3dctx.cpp`** 0x21eb28..0x21fb30 (written, verify),
  **`i3d/i3dlayer.cpp`** 0x222f50..0x224de0 (written, verify), **`i3d/i3dtex.cpp`**
  0x2184c0..0x21a120 (written, verify), **`i3d/i3dtim2.cpp`** 0x224de0..0x225018 (written, verify:
  7 functions -- i3dTim2CheckHeader 0x224de0 (TIM2 -> 1, CLT2 -> 2; the ROM ors the TIM2 byte
  tests), GetPicture, the hidden i3dTim2IsClutOnly 0x224ec0 and i3dTim2GetClutCount 0x224fb0,
  GetMipSize, GetImage (level sizes at picture +0x40), GetClut; the check and the two hidden ones
  have no callers), **`i3d/i3dview.cpp`** 0x226278..0x227a98 (written, verify) and
  **`i3d/i3dvram.cpp`** 0x227a98..0x228c08 (written, verify), **`i3d/i3dpacket.cpp`**
  0x21b220..0x21c960 (written, verify; below). Between tim2 and view,
  0x225018..0x226278 (unassigned): animation curve sampling (0x225018 binary search over key times,
  0x225080 / 0x225148 scalar / vector segment eval) and vertex morph blending (0x225460..0x2261b8),
  called from the model packet builders. Modules seen around them: the wire-shape strip builders
  0x21e6f8 / 0x21e8f8 / 0x21e9f8 before 0x21eb28 (0x21e1d8 the line strip), model packet builders
  0x21fb50..0x222f50 (unassigned@0021fb50: per material a state packet node, i3dGroupBegin*Packet),
  the texture objects (i3dtex.cpp, below), the fog state 0x21a120..0x21a3e8, i3r model helpers 0x21a3e8..0x21ac80, lights
  0x21ac80..0x21b220, the packet writers (i3dpacket.cpp), TIM2 helpers from 0x224de0 (an unreferenced TIM2 / CLT header
  check, picture lookup 0x224e70, size 0x224ed0), the view (below), the vector module with the i3d
  system calls at its ends (below).
  - **The vector / matrix module** (`i3d/i3dmath.cpp`, 0x236960..0x237ce8, 95 functions incl. 35
    hidden ones created in Ghidra, written 2026-10-22, verify; conventions in i3dmath.h). Two copies,
    **not** byte-identical: the first 0x236960..0x237178 (i3dVu0* names, plus the older plain names
    i3dVecZero / i3dVecCopyW / i3dMatCopy / i3dVecCross / i3dVecNormalize) builds rotations from
    cosf / sinf (i3dVu0MatRotX / Y / Z, one order RotZYX) and ends with i3dVu0RotTransPers 0x236fd0,
    i3dVu0MatLookDir 0x2370b0, i3dMatGetUnit (rodata 0x303c10), i3dSysInit; the second
    0x237180..0x237c58 wraps sceVu0RotMatrixX / Y / Z (i3dMatRotX 0x237818 ...), has all six rotation
    orders (i3dMatRotZYX 0x237860 .. i3dMatRotYZX 0x2379f0, named in applied order), clear / set /
    bitwise-compare helpers 0x237180..0x2372cc, i3dVecToRgba32 0x237530, i3dVecLength 0x2375a8, and
    byte-copy twins inside it (i3dVecAdd / Add2, Sub / Sub2, Mul / Mul2, Scale / Scale2, Interp,
    Clamp, Set, Clear, Equal, i3dMatGetUnit2 0x237c58 -> 0x303c60). The view uses the first copy;
    DogEntity / DogActor / MagScene the second. Row-vector convention: i3dMatMul(out, a, b) runs VU0
    micro 0 (out row i = sum_j b[i][j] * a[j]), so p * out = (p * b) * a; a rotation (angle, out, in)
    is out = in * R, which matches the platform's sceVu0RotMatrixX (row 1 = (0, c, s, 0)). Float
    arguments come first (f12). RotTransPers: p = v * m, xyz / w (1.0 at 0x303c00 / 0x303c50), x y
    in 12.4 (16.0 at 0x303c04 / 0x303c54), mode 0 z w in 12.4, mode 1 z w truncated. i3dPrintf
    0x237c68 is vsprintf into 1 KB + rxSysPrintf + fflush(stdout). i3dMatGetScaleTrans 0x228c38 (in
    i3d/i3rmesh.cpp, the mesh packet builder, not written yet) stays a bridge.
  - **The i3r model files** (I3D_BIN / I3D_I3M / I3D_I3C; 0x228c08..0x22e840, 2026-10-25). An
    I3D_BIN file (`i3d/i3rroot.cpp` 0x22b1b0..0x22b4b8, written, verify; types in i3rroot.h): id
    "I3D_BIN\0" (rodata 0x3039d0), +8 version 0x00100001, +0xc relocated flag, +0x10 the root
    item (type 0x52). Every item is 16 bytes {data, info (bit 31 no data fixup, 30..24 type,
    23..0 child count), children, name}; child lists are sorted by name (0x22c1f8 bisects with
    strcmp). In the file item fields are offsets from the root item, pointers inside an item's data
    offsets from that data. i3rRelocate 0x22b330 (DogModel::loadPack and 9 more callers) ->
    i3rSetRelocate 0x22b280 -> i3rItemRelocate 0x22c478 (recursive; per-type data fixups in the
    90-entry table 0x2db4c8, unfixups 0x2db360 for i3rItemUnrelocate 0x22c328; i3rIsItemType
    0x22c2a0 rejects 13 ids) then i3rBuildPackets 0x21a3e8 (i3rBuildMeshPacket 0x21fb50 per mesh into child 6's buffer).
    Root data (type 0x52, fixups 0x22ad20 / 0x22adf8): +0x10 / +0x14 tables of 0x40-byte records,
    +0x18 texture names (count u16 +0x2c), +0x1c u16 node indices and +0x20 their names (count
    u16 +0x2e), +0x24 a plain value. Root child 3 = the nodes (i3rGetNumNode), child 2 the meshes,
    child 6 the packet slots. The rest of the run is per-type code, each type a cluster of small
    accessors (`*(item+8)` = its children) plus its fixup pair -- ~450 functions Ghidra folded
    (hidden.py), e.g. type 0x4d's fixups 0x22a6c0 / 0x22a730 sit inside the mesh packet builder
    0x228c08..0x22a7d0 (0x228f50: a per-layer cache of mesh packets hashed on mesh / light / fog
    / view). **The item core** (`i3d/i3ritem.cpp` 0x22c1f8..0x22e840, pinned 2026-10-26): i3rItemFindChild
    0x22c1f8, i3rIsItemType, the relocators, and all 90 fixup pairs (`i3rRelocDataXX` /
    `i3rUnrelocDataXX`, Ghidra functions made at every table word 2026-10-26; 0x22c320 / 0x22c318
    the shared no-op). Every fixup is one of: a field at +off holding an offset from the data start,
    or an array of such entries (array pointer at +a, count a u_int / u_short / u_char at +c). Only
    25 types have pointers (0x08 / 0x1d / 0x38 / 0x2d / 0x46 / 0x4d / 0x52 have arrays; 0x38 two
    arrays of one count); the rest are `jr ra`. Checked offline on 1602 models in the disc's RPKs
    (7.1 M pointers, all in the file; names may be Shift-JIS). Nine pairs live at 0x22a6c0..0x22aec0
    (types 0x25 0x26 0x29 0x2a 0x2d 0x4a 0x4d 0x52 0x59) and are written in i3ritem.cpp too.
    **The per-type accessors** (`i3d/i3rtype.cpp` 0x22a6c0..0x22b1b0, pinned and written 2026-10-27,
    verify; 78 functions, 38 of them hidden ones created in Ghidra): per type, in the order 0x4d 0x29
    0x4a 0x26 0x59 0x25 0x2d 0x2a 0x52, the fixup pair then one-line reads of the item's child slot k
    (`item->m_children[k]`): the slot itself, data set, child count, n-th child, list begin / end,
    find / name / by-name / has through i3rItemFindChild. Named `i3r<Op><type>[_<k>]` (i3rtype.h).
    The layouts they show: 0x52 the root (slots 0..7; 0 materials, 2 meshes, 3 nodes with a count and
    n-th child only, 6 the packet slot), 0x26 a layer (slots 0..3; 1 its entity slots, a type 0x29 with
    slots 0..2), 0x2d a combined mesh (slots 0 / 1), 0x2a a node (slot 0 its data item, slot 1 a named
    list), 0x4d (lists 0 / 1, slot 2) and 0x59 (list 0) for the packet builders. i3rRelocData26's
    Ghidra body ran to 0x22a978; the function ends at 0x22a960. The mesh packet builder before it is
    **`i3d/i3rmesh.cpp`** 0x228c08..0x22a6c0 (written 2026-10-27, verify): i3rGetMaterialPacket
    0x228f50 caches a material's packet per layer (I3dLayer +0x414 = 256 buckets of I3rMeshPacket,
    key material + texture frame + light + fog + view; an I3rMeshOpt with a uv override or a depth
    offset is never cached) and builds it at the context head: view / fog (0x2291c8), then per
    material layer (type 0x26, I3rLayerData now 0x38 bytes known) its texture REF, GS state
    0x229380 (TEST from 0x22a448: m_atst via the ATST table 0x2db350 {1,0,1,4,7,2,3,6,5}, AREF =
    m_alpha_ref * 128, AFAIL codes 0x10 / 0x0f / 0x18 -> 1 / 2 / 3, m_date, m_ztst; ALPHA from
    0x22a638: m_blend 0x10 / 0x11 / 0x12 = 0x44 / 0x48 / 0x42, 0x2x the same with Ad), the layer's
    0x4a register overrides (masked with I3dLayer::m_user of the same register), the light set
    0x229678 (I3dLightDesc's five lists; m_shade 2 = half vectors with the view's eye direction),
    the STQ map matrices 0x229b70 (I3rStqMapEntity +0x41 kind, +0x40 slot; kind 1 projects through
    the view camera and the bias matrix 0x303800) and the uv transform 0x22a050 (layer children 0 /
    1: offset / scale and pivot / scale / rotation tracks read with i3rTrackGetValue 0x2251f0 at
    time 0). The material data (type 0x25) is I3rMaterialData: +0 depth, +4 sort param, +8 texture
    frame, +0xa sort func, +0xb sort order -- i3dLayerFindMaterialDrawSet reads the same item.
    **The mesh data** (read 2026-10-27 from i3rBuildMeshPacket 0x21fb50, i3rDrawModel 0x21fe68,
    0x220290 / 0x222560 / 0x225670 / 0x225ec8 and checked on amd.mdl; the host reader is
    rosepan_i3dmesh.cpp): node (0x2a) [1] = entries (0x59: +0 -> u16 bone table, +4 s16 mesh
    index, +6 u16 bone count, +8 enable byte); node [0] data +0x25 = shown (no data = shown).
    Mesh 0x2d [0] 0x46 (data +0 -> per-slot offset (inverse bind) matrices, +0xc u16 count) -> [0]
    -> 0x4c groups (data +0 -> the u16 bone slot, +5 flags: 8 = rigid: drawn with world x
    nodes[bones[slot]] in i3d order, i.e. p * node * world; otherwise skinned) -> [0] -> 0x4d
    (+0xc s16 material) -> [0] -> 0x56 batches: [4] positions (VU1 0x1dc), [2] normals (0x22c),
    up to 80 each; [5] bone records (0x31: +0 -> {s32 pool byte offset, f32 weight} pairs, +4 u16
    slot, +6 u16 pair count padded with zero weights): skinned pool = sum of w * (p * offset[slot] *
    nodes[bones[slot]]), normals through that matrix's normalised rows; [0] -> 0x30 -> [0] ->
    0x47 strips (data +0 -> V2-16 list unpacked at 3, +4 qwc, +5 vertex count 3..20 = the row of
    the 0x302c40 table whose qwords set VU1 0..2 and MSCAL 1; list element x = pool index with
    bit 15 = no triangle at this vertex, y bit 15 = (n-2, n-1, n) faces front, else reversed --
    1 exception in 6,000 triangles), strip [0] colours V3/V4 at 0x17 and [1] STQ V2-32 at 0x2b
    (q from STCOL 1.0), both TOPS-relative. Blocks (0x33): {+0 -> data, +4 u16 qwc, +6 u16
    count, +8 kind: 0 V4-32 qwords, 1 a VIF stream}; 0x226060 rebases the stream's unpack
    addresses in place once (first = base, the rest + base). root data +0x14 = per-node
    model-space bind matrices (i3rDrawModel's default node table), +0x10 the local ones.
    The accessors between the fixups in i3ritem.cpp's own extent (0x22c840.., most call
    i3rItemFindChild) are not written.

    **The model helpers** (`i3d/i3rmodel.cpp` 0x21a3e8..0x21ac80, pinned 2026-10-26, verify; types in
    i3rmodel.h): root data +0x00 the clip sphere (xyz centre, +0x0c radius), +0x10 one 0x40 basic
    local matrix per node; root child 0 = materials (type 0x25: +8 s16 texture frame, +0xc
    animated flag) -> [0] -> layers (0x26: +0x18 texture name, +0x1c its index set by i3rAssignTex;
    the layer's child 3 holds STQ entity slots, entity +0x41 the id i3rReplaceStqMapEntity
    matches, EE 0x50); child 2 = meshes (0x2d combined mesh -> [0] 0x46 via its child 0 / [1] 0x45
    via its child 1 -> 0x4c entries -> child 0's children = what i3rBuildMeshPacket 0x21fb50 takes);
    child 3 = nodes (0x2a, data = s16 parent index, always below the node's own); child 6 = type
    0x53 {buffer, qwc}. i3rConvertLocalToModel multiplies parent x local with VU0 micro 0 (the
    i3dVu0MatMul convention). A material's child 1 holds three entity slots (i3rGet/SetMatEntity0..2;
    DogActor::setEyeTex sets slot 0 to its own record at +0x470). Checked offline on all 2565
    models on the disc (2026-10-26).
    **I3D_I3M** motion files (`i3d/i3mroot.cpp` 0x22b4b8..0x22ba20, pinned 2026-10-26, verify; types
    in i3mroot.h): id "I3D_I3M " (0x303a68), +8 version 0x20001, +0x12 u16 per key, +0x14 node
    count, +0x16 relocation u16 (1 = offsets), +0x18 float length in frames, +0x1c nodes (12 bytes:
    name, +4 u8 key stride, +6 u16 key count, +8 keys), +0x10 u16 flags (1 packed rotations, 2 no
    scale, 4 splines play linear, 8 packed positions, 0x10 packed scales; 0x1f in every file seen),
    +0x20 a float4 table, +0x24 a float table, +0x28 an s16x4 1.15 table, +0x30/+0x40 packed
    position scale/offset, +0x50/+0x60 packed scale scale/offset. A key is u16 indices: time and
    inverse span length (float table), position, rotation quaternion, scale, then spline tangents.
    The pose player is `i3d/i3mpose.cpp` 0x22e840..0x2312a0 (pinned 2026-10-27; i3mpose.h has the
    key layout and the sample-flag modes: 0xf position, 0xf0 rotation, 0xf00 scale; 0 step, 1
    lerp/slerp, 2 Hermite/squad; the game passes 0x111). A pose is 0x30 bytes per node: position,
    quaternion, scale; i3mPoseToMatrices makes row-vector matrices (rows scaled, translation row 3).
    The run after it was split by role (inferred, 2026-10-27): `i3d/i3cquery.cpp` 0x2312a0..0x233628
    (i3cCastSphere / Ray / Cylinder and worker 0x232120) and `i3d/i3dgeom.cpp` 0x233628..0x236960
    (point / segment / triangle distance tests). All offsets from the root, 0 = none. 14,647 raw I3D_I3M
    in the disc's DATA tree (BG/MAPUNIT.BIN ...), all version 0x20001 and in offset form; the host
    keeps the offsets (i3mPtr), as for i3c. **I3D_I3C** (`i3d/i3croot.cpp`
    0x22ba20..0x22c1f8, pinned 2026-10-26, verify): id "I3D_I3C\0" (0x303a70), +0x10 relocation
    byte (1 = offsets), +0x12 both-faces, +0x14 node count, +0x16 group count, +0x18 nodes (12 bytes:
    name, u16 part count, parts), +0x1c group names; a part is 0x30 bytes with +0x24 / +0x2c
    offsets and +0x28 a tree of 12-byte nodes (u16 count, bit 15 leaf; +8 children or leaf data).
    All offsets from the root, -1 = none. No raw I3D_I3C exists in the disc's RPK / ARA files
    (compressed somewhere). Curve / interpolation maths 0x22e840..0x231198, collision casts
    0x2312a0..0x2337e0 (i3cCastRay 0x232508 ...), distance maths (i3dPointSegDist2, i3dSegSegDist2)
    ..0x23695c. Host pointers in these files: hazards.md "Types and layouts".
  - **Texture and CLUT objects** (`i3d/i3dtex.cpp`, 0x2184c0..0x21a120, written 2026-10-22, verify;
    33 functions incl. 4 hidden TEX1 setters 0x219dd0..0x219e30). Both start with their
    I3dVramHandle (+0 m_image = the object, +4 size: texture pages / CLUT 4-block units, +0x10 kind
    1 texture / 2 CLUT, +0x14 static). A texture (0x130): +0x18 TEX0 (no TBP0 base, no PSM), +0x20
    TEX1, +0x28 TEX2 (PSM << 20), +0x30 / +0x38 MIPTBP1 / 2, +0x40 seven 0x20-byte levels {BITBLTBUF
    (SBP = DBP = block offset), TRXREG, data, qwc, block offset, buffer width}, +0x120 levels, +0x124
    width, +0x128 height. A CLUT (0x38): +0x18 TEX0 bits (CPSM << 51 | CLD 4 | 0x13 << 20), +0x20 TEX2
    bits (CPSM), +0x28 palettes, +0x2c colours, +0x30 upload PSM (24-bit colours: CT24 into a CT32
    CLUT), +0x34 data. Made by DogTexInfo: i3dTexSetTim2 0x2193f0 / i3dClutSetTim2 0x219ab0 from a
    TIM2 picture (levels laid out with i3dPsmBlockEnd 0x218e10, the GS page block tables 0x302220..),
    i3dTexSetDesc 0x2197c8 for an image already in VRAM (static, no upload). Transfers: host -> VRAM
    0x218fa8 (A+D BITBLTBUF / TRXPOS / TRXREG / TRXDIR, IMAGE GIF + REF tags of <= 0x7fff qwords),
    CLUT 0x2190c8 (16-wide, 256 colours per image), page moves 0x2192c8 (TRXDIR 2); a moved CLUT is
    uploaded again. PSM tables: i3dPsmTexelBits 0x218d58 (data bits) vs i3dPsmBits 0x218db8
    (storage bits). An unreferenced PSMT8 / PSMT4 -> PSMCT32 in-place converter i3dTexSwizzle
    0x218778 with its block / page helpers (tables 0x301e78..0x3020b8). Before 0x2184c0: utility code
    (0x21838c a strncpy that pads, 0x218450 an angle wrap by pi, 0x218470 an empty varargs function
    called from game code -- a disabled debug print), then the debug menu (DBGFLAG.BIN) from 0x216ad0.
  - **The frame environment** (`i3d/i3denv.cpp`, 0x21a120..0x21a3e8 fog and 0x21ac80..0x21b220
    lights, written 2026-10-22, verify). Fog: `I3dFog` = colour (clamped 0..1) + near distance, fog
    amount at near (0..1), far distance, amount at far; i3dCtxSetFog 0x21a158 takes 2 tail qwords
    (`I3dFogState`: FOGCOL = r | g << 8 | b << 16 at +0x10, a ramp F(z) = base + scale / z with
    F 255 = no fog at +0x8 / +0xc, solved through F(near) / F(far)), and i3dPktMakeFog 0x21a270
    writes a RET-tagged packet at the context head (STCYCL 1 1, UNPACK V4-32 to TOPS of a VU1
    command buffer: 0x11 kick (A+D FOGCOL), 0x190 copy the ramp qword to VU address 0x3f8, 0x0a end;
    then MSCAL 1) whose address the layer CALLs (i3dPktCallFog). Lights: a description and a created
    light share one 0x40 layout: ambient qword, then (count, list) pairs at +0x10 / +0x18 / +0x20 /
    +0x28 / +0x30 for five kinds -- parallel (0x20: direction normalized, colour), point (0x20,
    copied), spot (0x40: position, direction normalized, colour, inner / outer half angles in
    radians clamped to 0.001..pi/2 - 0.001 with inner <= 0.95 outer, turned into cos outer,
    1 / (cos inner - cos outer), cos^2 outer, 1 / sin outer), kind 3 (0x30: direction normalized +
    two vectors) and kind 4 (0x30, copied); +0x38 the context on a created light.
    i3dCtxCreateLight 0x21b148 builds them in the order parallel, point, spot, kind 3, kind 4,
    skipping kinds with a NULL list. The normalizer is 0x2375d8 (i3dVecNormalize2, the second
    vector-module copy).
  - **The packet writers** (`i3d/i3dpacket.cpp`, 0x21b220..0x21c960; 20 of its 54 functions were
    folded into the one before and created in Ghidra): draw-set list helpers, i3dPktDmaTag (tag top
    byte 0x10 CNT / 0x30 REF / 0x50 CALL), i3dPktDirectTag 0x21b4d0 (VIF FLUSHA + DIRECT qwc, the
    prefix of every GIF packet), i3dPktSetReg (A+D), ALPHA modes (jump table 0x3027c0), i3dPktClearZ
    0x21b6c8 (a TEST_1 = alpha NEVER / fail ZB_ONLY full-screen sprite), and one writer per GS
    primitive in two families: *F writers 0x21b7c8..0x21c0fc take float vectors (PACKED GIF tags at
    data 0x302850..0x302898; XYZ = ftoi4(v * screen scale 0x2db340 + offset 0x2db330), set by
    i3dPktSetScreenScale / Offset 0x21c120 / 0x21c100; colour = ftoi0(v * (255,255,255,128)) or
    textured * 128 (data 0x3026c0 / 0x3026b0); UV ftoi4), and REGLIST writers 0x21c140..0x21c8c4 on
    values from i3dPackXYZ / RGBAQ / UV / ST (PRIM first; some leave a NOP slot unwritten). Every
    primitive sets ABE; the GIF tag's PRIM decides the name (Flat / Gouraud, Tex UV (FST) / ST).
    0x21c960.. is `i3d/i3dvu1pkt.cpp` (VU1 packets, below).
  - **VU0 microprogram** (data 0x2949f0, `python vudis.py 2949f0 9c <addr>`): 0 the 4x4 multiply
    (vf4..7 = a, vf8..11 = b, result in vf8..11); 0xc8 a subroutine: vf12..15 = box rows (vf8..11)
    x matrix (vf4..7); 0x96 box culling (corners = vf15 +- vf12 +- vf13 +- vf14, CLIP each, vi2 =
    AND of the 6-bit outcodes: non-zero = all outside one plane); 0x69 the same with vi2 = OR (any
    corner outside).
  - **The view** (`i3d/i3dview.cpp`, I3dView 0x400 bytes from the tail): i3dCtxCreateView(ctx,
    MagViewParam, camera matrix) copies the record's frustum (near, far, l, r, b, t) and screen part
    (zmax / 16, zmin / 16, x0, x1, y0, y1, offx, offy), widens degenerate ranges (0x226410 to 0.5
    pixel, 0x226498 to near / 10000), and builds three perspective frusta onto the NDC box
    +-(w / (w + 2)) x +-(h / (h + 2)) x +-1: the guard band (l / r / b / t extended so the screen
    rectangle's offset x / y map to 64 .. 4031.94; +0x160 proj, +0x1a0 camera * proj, +0x1e0
    viewport), the screen rectangle (+0x220 / +0x260 / +0x2a0) and the draw frustum between them at
    ctx +0x12128 (0.35) of the way to the guard band (+0x120 proj, +0x40 camera * proj, +0xa0
    viewport, +0x0 world -> screen, +0x80 / +0x90 the viewport's scale / offset). Viewports map the
    NDC box to screen x / y and z zmax / 16 (near) .. zmin / 16 (far), the z range widened by 1/4096.
    +0x2e0 / +0x320 side-plane matrices (inward normals of the rectangle's / guard band's frustum
    through the eye), +0x360 depth matrix (z - n, f - z, (z - n) / (f - n), 1), +0x3a0 the inverse
    camera, +0x3e0 -normalize(inverse row 2), +0x3f0 SCISSOR_1 from the record's unwidened
    rectangle, +0x3f8 the packet address. The packet: REF of the view's first 10 qwords (UNPACK to
    VU1 6 + TOPS), RET with 6 qwords to 0 + TOPS (0x11 / 2 header, A+D SCISSOR_1, rows 400 / 0x3fa /
    0x3f9 and 400 / 0x3ec / 0x3e2 round the eye direction), UNPACK S-32 (10, 0) to 0x10, MSCAL 1.
    Tests: i3dViewTestBox 0x2268b0 (0 out / 1 inside the guard band / 2 clip), i3dViewTestBoneBox
    0x226920 (bone boxes' AABB), i3dViewTestSphere 0x226ad8 (hidden, no caller), i3dViewGetDepth
    0x226c18. i3dCtxCreateOrthoView 0x2273d8 (no caller) is the same body with i3dMatMapBox
    projections and no plane matrices.
  - **Groups and nodes** (`i3d/i3dlayer.cpp`): a group (0x30: list head key / first / 0 / last,
    +0x10 ctx, +0x14 sort function index into data 0x3031f0 {none, i3dSortList, i3dSortList, NULL},
    +0x18 order 1 asc / 2 desc, +0x1c param, +0x20 the open node, +0x24 count, +0x28 packet bytes).
    An empty list's last node is its head (appending writes head->first). Nodes: packet nodes (0x10:
    +8 start, +0xc end; the run links them with NEXT tags) and call nodes (0x20: +8 = 0, +0xc
    callback, args from +0x10): run sub group 0x224418 (layers in the root), stop 0x224438 (returns
    1; i3dCtxRunGroup appends it with key -1 ascending / 0 otherwise, then sorts), channel packet
    0x224620 (clears ctx +0x12158[ch], links +0x14..+0x18), state CALL 0x224440 / state + packet
    0x2244f0 (a DMA CALL of the channel's state packet only when ctx +0x12158[ch] differs), clear
    state, print, user call (0x224698: func(a, b), non-zero stops). Begin/End pairs per kind, each in
    an aligned flavour (start = head 128-aligned, head after = 128-aligned past the end tag) and an
    unaligned one. i3dSortList: < 32 nodes a cocktail sort of (key, node) pairs in the scratchpad,
    else a 4-pass 8-bit LSD radix sort over two 256-bucket arrays (0x70000000 / 0x70002000).
  - **Layers** (0xe60): list head of draw-set nodes (1 qword: key = sortable depth, +8 the set),
    +0x14 256 hash buckets of draw sets (circular {+4 next, +8 prev}; a hit moves to the front),
    +0x414 256 words cleared and never read, +0x814 ctx, +0x818 the current set's group (Begin /
    EndSet write there), +0x81c the layer's own group (key 0 i3dLayerBeginPacket, key 1 the draw
    call node = i3dCtxDrawTexturedLayer, key 2 the post packet, key 3 the sub-layer group +0x820),
    +0x824 default set (empty description), +0x828 current set, +0x82c light / +0x830 fog / +0x834
    view, +0x838 0x62 doubleword pairs (setters 0x223458.., no caller). A draw set (0x38): hash,
    links, group (from the description's +0xc / +0x10 / +0x14), +0x10 per-slot 6-qword TEX0 / TEX1 /
    TEX2 / MIPTBP1 / MIPTBP2 packets, +0x14 use counts, +0x18 layer, +0x20 depth, +0x24 texture
    numbers, +0x28 count. The texture-set description (0x18) is hashed word by word (not the
    pointer) plus its texture numbers, folded to a byte. i3dTexRegsSet adds TBP0 (block << 5) to
    TEX0 and the three MIPTBP fields, CBP (CLUT block << 2, plus the CLUT index scaled by the CLUT
    size for PSMT8 / PSMT4; CSA = index & 0xf for PSMT4) -- the bits per pixel from 0x218db8.
  - **The context** (`i3dContext`, EE 0x12168): +0 texture VRAM heap (GS pages), +0x70 CLUT heap
    (4-block units; i3dCtxSetup(ctx, 0x158, 0xa8) gives textures 0xa8 - 0xb = 0x9d pages from page
    0x158 and CLUTs the 0xb pages after them), +0xe0 the frame's root group, +0xe4 / +0xe8 frames and
    next frame, +0xec / +0xf0 buffer, +0xf4 head (packets grow up) / +0xf8 tail (work objects grow
    down: groups 0x30, layers 0xe60, list nodes), +0xfc / +0x100 texture table (16-byte entries: +4
    texture handle, +8 CLUT handle), +0x118 the frame's packet, +0x11c / +0x911c 0x200 VRAM nodes per
    heap, +0x1211c reserved draw sets, +0x12120 end hook (default: DMA END tag), +0x1212c VRAM
    overflow (EndFrame returns 9), +0x12154 the open DMA tag the next run links from.
  - **A frame**: BeginFrame takes buffer share idx of count (head 128-aligned in the uncached
    segment), makes the root group; MagScene::draw makes layers (`i3dCtxCreateLayer(ctx, float
    key)`, key bits made unsigned-sortable) and sets each one's light set / fog / view
    (`i3dLayerSetLight` / `SetFog` / `SetView` 0x223240 / 0x223250 / 0x223260: layer +0x82c / +0x830 /
    +0x834; getters 0x223270..0x223290 read +0x824..+0x834; the view from `i3dCtxCreateView`
    0x226c58(ctx, MagViewParam, camera matrix), the light sets from 0x11262c over the light pool)
    and register packets (`i3dLayerBeginPacket` 0x223378 -> `i3dPktSetReg(pkt, reg, value)`
    0x21b550, one A+D write, 3 qwords -> `i3dLayerEndPacket` 0x223398, both on the layer's group
    +0x81c); EndFrame -> 0x224798 opens the chain (0x21b698), runs the
    root group (0x2246c8: sort, link packets, call node callbacks), then the end hook. A layer's
    callback `i3dCtxDrawTexturedLayer` 0x21f4d8 sorts its draw sets and reserves their textures in
    runs of equal (+0x20 float, +0x2c key); full heaps flush the run so far (compact, VRAM transfer
    packet: GS local copies of moved blocks, uploads of new ones, TEXFLUSH, a CLUT-reload TEX0 from
    data 0x2f2b70; then each set's TEX0 / TEX1 / MIPTBP A+D packets via 0x224b28).
  - **The VRAM heap** (0x70; nodes 0x44 on six intrusive {prev, next} lists: 0 address order, 1 LRU,
    2 locked, 3 evict, 4 new, 5 moved; flags 1 free, 0x10 owned, 0x100 locked, 0x1000 evict, 0x10000
    new, 0x100000 moved, 0x1000000 sentinel): reserve = lock resident blocks, allocate the rest
    first-fit from the LRU tail along address order, else evict unlocked LRU blocks into one block
    at the start and let compaction move everything (moved down -> list head, up -> tail).
    0x2281c0's third argument (a2) is the sentinel: 0x228528 leaves it there from its loop.
    Unreferenced: 0x2289e0 (clears +0x64..+0x6c), 0x228320 / 0x2283e8 (evict-window searches).
    Block nodes come from a stack the ROM never checks for underflow (0x227cc8); i3dVramReserve
    locks i3dVramAlloc's result unchecked (a NULL on a fragmented heap faults on the EE; the port
    skips it, marked PC:). Handles sit at +0 of the 0x130 texture / CLUT objects (0x219e58 memsets
    0x130, then i3dVramHandleInit 0x227c90; release 0x219a98 / 0x219c70 -> i3dVramHandleRelease).
  - **Hardware**: i3dInitHardware sends REFs of the VU1 microprogram (data 0x2912b0, 0x354 qwords,
    plus STCOL 1.0 / STROW 0) on VIF1 and the VU0 one (data 0x2949f0, 0x9c qwords) on VIF0.
  - **VU1 packets** (`i3d/i3dvu1pkt.cpp` 0x21c960..0x21eb28, 2026-10-27): the VU1 image's entry 8
    (MSCAL 8) is a dispatcher: it jumps to the address in TOPS+0.x (a writer puts it there with
    UNPACK S-32); MSCAL 1 runs a command list at TOPS (qwords: program, args; program 400 = 0x190
    copies state, 0 ends). Permanent state: the CALLed view's world -> screen at 0x3e2..0x3e5,
    world -> clip 0x3e6... **Sprites** (mnuBase): i3dPktBeginVu1Sprite 0x21e5a0 (view, fog CALLs;
    GIF template to 0x3f4: NREG 4 FOG / ST / RGBAQ / XYZ2, PRIM 0x4d fan | TME << 4 | FGE << 5;
    colour scale to 0x3f5 = data 0x3026b0 (128) textured / 0x3026c0 (255, 128 alpha)),
    i3dPktVu1Rect 0x21e488 (u0, v0, u1, v1 to 0x3f1), i3dPktVu1Matrix 0x21e428 (three rows to
    0x3ee), i3dPktVu1Sprite 0x21e690 (colour TOPS+1, centre TOPS+2, 0x712 to TOPS+0, MSCAL 8):
    microprogram 0x712 = corners centre +- 0.5 row 0 +- 0.5 row 1 (loi 0.5), through 0x3e2,
    Q divide, ST = rectangle corners, colour * scale ftoi0; (-0.5, -0.5) takes (u0, v0). Unreferenced
    helpers 0x21e4e8 / 0x21e520 upload the view inverse (scaled) as a billboard matrix. **Strips**:
    i3dPktDrawStrip 0x21db18 (pkt, matrix, count, stq, colours, xyz, adc) and i3dPktDrawStripColor
    0x21db40 (one colour instead) shuffle registers and jump to 0x21c960: set-up (matrix to 0x3d2,
    GIF template, scale), chunks of <= 0x50 positions inline to 0x1dc, STQ / colours inline behind a
    NEXT tag and REF'd 20 at a time (two shared), ADC as vertex indices (bit 15 = no kick; default
    lists data 0x3026d0), then a REF of the 4-qword command 0x302970 / 0x3029b0 whose GIF tag is a
    triangle strip (PRIM 0x4c / 0x5c with STQ).
- The scene's first 0x2000 bytes are 256 **wall segments** (two (x, y, z, w) ends each):
  `MagScene::castWalls` 0x133a0c tests a level ray against those whose height range covers the ray's
  y with `i3dSegSegDist2` 0x234050 (squared segment distance 0 = a hit) before `castRay` 0x133454
  falls back to the sky slots' collision models (`DogModel::castRay` 0x11440c on model 2).
  They are the scene's **private collision** (MAX_PRIVATE_COLLI 256): `addPrivateColli(p1, p2)`
  0x135e20 takes the first free one (free = equal end heights, so p1.y != p2.y is asserted),
  `removePrivateColli` 0x135f44 zeroes both heights. The other wall tests: `hitWallsSphere`
  0x133b80 (radius, height range covering pos.y) and `hitWallsCylinder` 0x13386c (radius, height h <
  0 below pos), both with `i3dPointSegDist2` 0x2337e0 on the segment taken at pos's height; the
  movers call `castSphere` 0x133658 (walls, then sky slot 0's DogModel::castSphere 0x1145c0) and
  `castCylinder` 0x133d10 (walls, then DogModel::castCylinder 0x1149d4 on one or all sky slots).
- The scene's 0x2000 **strip records** (+0x437e0, MagSceneStrip) are a debug-draw queue drawStrips
  empties: adders 0x133e94 (type 0 strip with colours, life), 0x133efc (type 1), 0x133f68 (type 2
  line strip, one RGBA colour as r, g, b / 255, a / 128), 0x134114 (3 wire box), 0x1342bc (4),
  0x134460 (5 wire sphere, radius in m_color[0]), setStripColor 0x1344c4, addStripRecord 0x134624.
  Only types 0 and 2 have callers (0x11e0d8 / 0x11ea04, the PC mover); the rest sit uncalled in the
  ROM (six of them were folded into 0x133f68's body until session 4 split them).
- The scene (`MagScene`, +0x12540): 128 actors of 0x4d0 at +0x2920 (vptr +0x94), actor flags at
  +0x29120, light set +0x2050, view objects +0x2734[4], camera mode +0x2ad50, `MagLifePC` at
  +0x2adc0 (the player; vptr +0x14), `MagLifeBwn` at +0x2afe0 (Brown). Chapter archives
  `data/ch/NN/CHAPNN.RPK` (sdata 0x2961a8) feed `MagSystem::loadModel`.
  Scene map from the constructor 0x1360a0 (magscene2 session, 2026-10-22; word index x 4 = offset):
  +0x2330 `MagLifeMan` (64 x 16-byte entries + head, priority-sorted), +0x29520 `DogTexInfo`
  (256 textures, shadow01 from rodata 0x2ed500 first), +0x29540 four sky ("tenkyu") slots of 0x520
  (buffer +0, three DogModels at +0x10 stride 0x1a0, id +0x4f0 (-1 free), sky i3r +0x504, its
  node matrices +0x508, spins +0x50c, node count +0x510; released by 0x1386d4 / 0x13ff00), +0x2aa10 two 0x38-byte view records (near 5,
  far 2000, +-5.12 x +-4.48, z 65530..1, screen 0..511 x 0..447, offset 1792 / 1824), +0x2aae0
  four 0x20-byte i3d fog records (colour vector + four floats; i3dCtxSetFog 0x21a158), camera
  target pos / at at +0x2ada0 / +0x2adb0, `MagLifePC` +0x2adc0 (<= 0x220), `MagLifeBwn` +0x2afe0
  (0xd40), `MagLifeBwnCtrl` +0x2bd20 (0x130; its state word at +0 is MagSystem::getBwnState's),
  the menu words +0x2be50 (`m_menu_id`, -1 none), +0x2be54 (arg), +0x2be58 (`m_menu_result`, the
  menu's last processPad), +0x2be5c (`m_pMenuBase`, the asserts' name), the MagLifeMenu* objects
  +0x2be60..0x40780 (Item, Map, Stock, Save, Load, Title, Opt, Movie,
  Mcd, Now, Board2D, SelLang, each vptr +0x14), the faders `MagLifeFader` +0x41850
  (mag/maglifefader.cpp 0x12f008..0x12f298; fade kinds 0 / 4, colour +0x38 0 / 0xff; frame +0x30,
  time +0x34; states 1 / 8 opaque, 2..3 alpha 0x80 -> 0 then 4, 6..7 alpha 0 -> 0x80 over time - 2
  then 8 after time, 4 / 5 nothing; one full-screen untextured sprite at key 0 in the life layer,
  ALPHA 0x44) and `MagLifeFadeKey` (the ELF's vtable says MagLifeFadeKeyHole) +0x41890 (0x12f298;
  kinds 1..3, the kagiana_tcc keyhole of data/evntex/EVNTEX.RPK), a life of an unnamed class at
  +0x418f0 (0x130da4), the 2D layers `S2dCineSco`
  +0x41930, `S2dBg` +0x42260, `S2dMes` +0x42bb0 (all MagLife, level 7, life priorities 0x8006 /
  0x8000 / 0x800a), the current fade record +0x43700 (six words: kind, mode, time, -, -, level;
  `MagScene::setFade` 0x1311f4, `getFadeState` 0x131550 maps the fader state to 0..6) and the
  system message window +0x43718, objects at +0x837e0 (ctor 0x176a3c), `S2dMovie` +0x84aa8 (spr/s2d.cpp), +0x8542c
  (0x17c0f0), `MagSceneIFMap` / `MagSceneIFChara` at +0x854e0 / +0x854e8, Brown / PC actor
  numbers +0x853b4 / +0x853b8. Every MagLife has `m_scene` at +0x20 and the vptr at +0x14;
  MagLifeControlChara's bound actor is +0x58 (`m_actor`).
- **The scene's constructor** (0x1360a0, written 2026-10-22): besides its members' constructors it
  sets 256 vector pairs at +0x0 to (0, 0, 0, 1) (resetEntities clears their y), the shadow mode -1,
  +0x22a0 0.8 / +0x22a4 1.03 / +0x22a8 0.4 / +0x22b0 1 / +0x22b4 1, +0x22bc[4] -1, every sky slot's
  m_id -1, the strip records free, m_focus_actor -1, +0x2a9c8 3, +0x2aa84 1, +0x854b0 (0.1, 0.1,
  0.1, 1), +0x854d4 / d5 1, and m_pc_no **0** (not -1). The camera's inline constructor gives a
  pi/3 lens (+0x140, 0x3f860a92) and 115 at +0x144; DogEntity's inline one gives (0, 0, 0, 1)
  translation / rotation / offset / +0x80, scale 1, dirty. Text windows (MagLifeWindow, 0xe0 bytes,
  ctor 0x1308bc) keep their position at +0x30, size +0x94 / +0x98, alpha ramp +0xac..+0xbc, font
  +0xc4, font texture number +0xc0; MagScene::setWindowShow passes its flag to setHide.
- **The scene's frame** (`mag/magscenerender.cpp`, pinned 0x139540..0x13c328, 2026-10-22;
  all 20 written, `verify`). `MagScene::draw(ctx, dbuff, frame)` 0x13bcc0:
  setTexTable when DogTexInfo +0x10 (dirty) is set; rebuilds both view records' frustum from the
  camera's lens angle (DogCamera +0x140 -> getLensFov 0x10fac0, a 13-point curve; tan(fov / 2) x
  -0.5 x near, height x 0.75) when it differs from +0x2aa80; one view (camera matrix) and the fog
  (+0x2aa84 / fog record 0); then **eight render stages** at depth 500, one per light set
  (`DogLightPool::createLight` 0x11262c on the pool at +0x2050; sets 1 and 2 scaled by +0x22a8),
  each with FBA 0, ALPHA 0, TEST 0x5001d, TEXA 0x80, PABE 0. getDrawFlags 0x13963c (sub mode word
  +0x22d0: 8 -> nothing, 7 -> 0x200, else 0x101): bits 0xff00 = actors (collectActors 0x13b2ac:
  alive, shown, flag bit 3 == bit 9; key 0x10000 parented + 65535 - camera distance, PC 0;
  sortActors ascending; drawActors into the stage of each actor's light id, mirror pass first);
  bit 0 = the world (strip records +0x437e0 x 0x2000 of 0x20 bytes, the sky dome 0x1406b8 at 490,
  the sky models' first model, MagLifeMan::draw groups 0 / 0x4000, the blob shadows at 800 when
  +0x2a9c8 bit 1, then the projected shadows (actors with flags & 0x60 == 0x60, layers 700 / 705 +
  10 n, at most 10, the STQ map +0x2aa90 on the sky's second models) or, when +0x2a9c4 >= 0, that
  actor's close-up at 1000 (a 256-wide target, FRAME fbp 0x118), and 0x177830 on +0x837e0). Always:
  S2dMovie::drawMovie, a layer at 900 (light set 6, TEST 0x5000d) for MagLifeMan group 0x8000, the
  focus effect 0x140e88, S2dMovie::draw. The mirror: mapparam.bbb words 9 / 10 at +0x85494 (axis
  mode 1 z, 2 y floor, 3 x) / +0x85498 (the plane, a float); drawActorMirror reflects the root
  entity when the camera is on the actor's side. The camera at +0x2ab70 is a `DogCamera`
  (dog/dogcamera.h; vtable 0x2e1fb8, view matrix +0xf0, angle +0x140); actors are drawn with its
  m_rotate (+0x2abc0) and m_trans (+0x2abb0). Dog vtable slots seen here: DogModel 2 transform,
  4 getParent, 8 / 9 setHide / isHide, 11 cull 0x114e38, 12 draw (0x1151f4; DogActor 0x10e5c0),
  13 drawUnitMatrix 0x1152a8; DogActor 18 getNodeWorldPos, 19 drawModel, 20 drawShadow.
- **Projected shadows** (`MagScene::makeActorShadow` 0x139b4c): per shadowed actor a layer at 700 +
  10 n drawn into the 256 x 256 target at fbp 0x118 (sprite clear to black, FBA 1, the actor's
  DogActor::drawShadow under an ambient-only light (0, 0, 1, 0), then a post packet
  (`i3dLayerBeginPostPacket` 0x2233e8, draw-group slot 2) with four black border lines,
  `i3dPktLine` 0x21c538). The light: scene +0x2040 `m_shadow_mode` (set with +0x2044 by
  `MagScene::setShadowLight` 0x13fbe4; < 0 = none, the constructor's value): 1 = the pool's
  directional light (+0x2044 points at it; eye ten clip radii back), else the actor's nearest point
  light (`DogLightPool::findNearestPointLight` 0x1129cc over the actor's light set; DogActor +0x160
  / +0x164 / +0x168 = current light, previous light, frame counter: a change fades the old light out
  over 30 frames and the new one in over 30; the eye is kept three clip radii away and at y <= 0).
  Writes the STQ map (+0x2aa90 = texture matrix [[1,0,0,0],[0,1,0,0],[t,t,2t,0],[0,0,0,1]] x the
  light's view, t = tan of the clip sphere's half angle) and the scene's light record +0x2000
  (`MagShadowLight`, 0x40: directional = direction, colour (0,0,0,I); spot = position, direction,
  colour, cones +0x30 = t - 0.1 / +0x34 = t capped 0.7; I = m_22a0 x m_2a9cc, for points x 1e6 /
  distance x fade), which setShadowLight hangs into the shadow layers' light description +0x2a9d0
  (kind list 1 for directional, 3 for spot). View record data 0x298cb0, up vector 0x298cf0 = (1, -1,
  1, 0). `DogLightPool` (+0x2050, `dog/doglightpool.cpp` pinned 0x1103dc..0x112b08): per-kind light
  arrays at +0x00 and counts at +0x18 (kinds from the assert strings at 0x2e86d0..0x2e87c0:
  DOG_LIGHT_AMBIENT 0 (0x10 stride, a colour), DIRECT 1 (0x20: direction, colour), POINT 2 (0x20:
  position, colour), SPOT 3 (0x40)); eight light sets, `m_set_mask[set][kind]` at +0x48 (6 words =
  0x18 per set; bit i = light i). `createLight` 0x11262c takes the set's first ambient (x scale, alpha
  1), up to 8 direct and 16 point lights into an I3dLightDesc and calls i3dCtxCreateLight; its flag
  argument (MagScene +0x22cc) runs `dogLightToGray` 0x110bb8 (luminance 0.299 / 0.587 / 0.114) over
  every colour -- a monochrome scene mode. `findNearestPointLight` starts from 0x7fffffff (rodata
  0x2e87dc: the PS2's largest float, a NaN on the host; the port uses FLT_MAX).
  The rest of the pool (written 2026-10-22, `verify`): +0x30 `m_used[kind]` = OR of every set's mask
  (allocId's free list), +0x108 `m_data`. `create` 0x110d68 (MagScene::loadLight) allocates one
  0x580 block -- 8 ambient, 8 direct, 16 point, 8 spot (m_num 8/8/16/8, kinds 4 / 5 none) -- not
  cleared; `term` 0x110b78 frees it. Ids: `allocId` 0x111120 (first bit free in m_used), `addToSet`
  0x1112a8, `removeLight` 0x1111e4, `resetSet` 0x110f78 (set < 0 = init + create), `buildSet`
  0x111064. Light fields by `setLightParam` 0x111584 / `getLightParam` 0x111b5c (param 0 position,
  1 direction, 2 colour, 5 spot cone angles in degrees -> radians at spot +0x30 / +0x34, with the
  ROM's pi 0x40490fdc); `setSetLightParam` 0x112134 / `getSetLightParam` 0x1123b0 address the n-th
  light of a set through the getters 0x110660..0x110988. doglightpool.h lines 129..132 are the
  non-const per-kind getters, 140..143 their const twins. Per-kind tables: capacity 0x2e85d8
  {8,8,16,8,0,0}, light size 0x2e87e0 {0x10,0x20,0x20,0x40,0x30,0x30}. `DogLightBuffer` (copyToBuffer
  0x111460, copyLights 0x1103dc, setupDesc 0x110578): a pool set copied with its own I3dLightDesc at
  +0x580 -- no callers in the ROM.
- **The scene's loaders** (`mag/magscene2.cpp`, pinned 0x137300..0x139540, 2026-10-22; `verify`):
  - `MagScene::loadSystemData` 0x137330 (start's first call): data/options.txt -> `parseOption`
    (magsceneoption.cpp, buckets at +0x4371c); data/PREMIERE.RPK's 2048-byte header, then the font
    file (`fnt/fnt16US.bin`, a pointer variable at data 0x298b80) to +0x2908 and file 0 (the common
    textures) to +0x2914, added with `DogTexInfo::addPfh`; texture numbers r_shadow / noise at
    +0x290c[2] (initRender hands noise to the +0x418f0 object) and bit8a..bit8d at +0x28f4[4]
    (bit8e is in the table, unused); then the two loaders' init.
  - Loaders (`mag/magloader.cpp`, names descriptive): `MagCharaLoader` at +0x215c (CHARA.RPK;
    character id = (category << 10) + n, < 0x5800, 22 categories; getIndex 0x142c88 / getId
    0x142dac / load 0x142b18; name index `MagLoaderIndex` at +0xcc, find 0x142500 case-insensitive)
    and `MagMapLoader` at +0x224c (data/bg/mapunit.bin, loadByName 0x142fd0). Both start with a
    `FileReader` (+0) and its `FileReadReq` (+8). Written 2026-10-26 (all 17, 0x141e44..0x14313c):
    CHARA.RPK is a pack of 20 packs named PC, BWN, ACTION, ITEM, FIX, ENEMY, DOOR, INST,
    CHAP01..CHAP12 (table data 0x2eed80); init reads its 2048-byte header, then the first 0x2000
    bytes of each sub-pack, and appends each header's entries with the sub-pack's offset added, so
    `m_num_chara[20]` (+0x2c) / `m_chara_start[20]` (+0x7c) give each pack's entry range. Id
    category -> pack: 8 PC, 9 BWN, 0 ACTION, 3 ITEM, 2 FIX, 5 ENEMY, 6 DOOR, 4 INST, 10..21
    CHAP01..12 (1 and 7 none). The file stays open from init (m_close 0); load reads at the
    entry's offset into a 64-aligned game-heap buffer. `MagLoaderIndex` (EE 0x24): m_num,
    m_data (one heap block: sizes, offsets, name pointers, name-length bytes, then the names;
    calcSize = num * 13 + name bytes on the EE), m_data_size, m_size, m_offset, m_name,
    m_name_len, m_name_size, m_name_buf; create 0x141f54 (max 1024 entries), add 0x1420f8
    (rebuilds the block, optional exchange sort of the new tail by strncmp 32), clear 0x14257c
    (words 0..6 only), release 0x14259c. initFromPacks 0x142908 (no callers) reads the loose
    packs data/ch/pc/PC.RPK .. data/ch/12/CHAP12.RPK (table data 0x298d80) instead.
  - `MagScene::load(ctx, id, data, copy)` 0x1377f0 takes a map unit pack into the first free sky
    slot (no check when all four are taken): file 0 is the model pack (its name minus the extension
    names the other files), loaded by DogModel slot 14 into m_model[0]; the model pack's file 2 is
    m_model[1] (slot 15) whose STQ map goes to +0x2aa90; `<name>_c.bin` (head word: count in the low
    half, the rest offsets turned into record pointers in place), `mapparam.bbb` (int words: view
    far -> setViewFar 0x131618, fog RGB / 255 and four params (/1, /100, /1, /100) -> m_fog[1] and
    setFog 0x143d7c, scene words +0x854a4 / +0x85494 / +0x85498 when the file is over 0x20 bytes,
    a factor /100 kept in 0..1 at +0x2a9cc), `<name>_col.i3s.i3r` + the next file -> m_model[2],
    `<name>_l.bin` -> the slot's light data (loadLight 0x13f658), sky textures (0x140030, with a
    context), the pack's `sector.sec` -> +0x8542c (0x17c1f4). Ends with setDefaultLight 0x13f8e4
    and resetEntities 0x135fa4. Slot fields: +0x4f4 _c.bin, +0x4f8 count, +0x4fc records, +0x500
    light data.
  - Actors: findFreeActor / findActorById (DogActor id +0x1a0) / findActorByName; an actor whose
    model another actor already shows is *shared* (DogActor slot 21 `shareModel`: master +0x118,
    ref count +0x104) instead of loaded; `loadActor` 0x138ce8 loads with `DogActor::load(pfh,
    textures, keep ? 2 : 1, m_sepia)`, then setId, 5 at +0x1d8, `ready()` (+0xf0 = 0, +0xf4 = -1).
    A second flag word per actor sits at +0x29320. `loadModel` / `loadModelByName` take the pack
    from CHARA.RPK; the rpk MagSystem passes is not read.
- **Packs** (`bed/bedpfh.h`, BedPfh 0x101750..): magic "RTPK", 0x20-byte header (total size +4,
  flags +0xa, file count +0xe), then per-file tables by flag: sizes (1), offsets from the header (2),
  16-bit attributes (0x10, 0x20 ...), NUL-separated names (0x8000). Loaders read the first 2048
  bytes, assert `getFileOffset(0) == 2048`, then read single files.
- **The file reader** (`game/gamfile.cpp`, pinned 0x1be2cc..0x1beb70, written 2026-10-22, `verify`;
  file name a guess): `FileReader` {request, own fd} runs a `FileReadReq` (0x24: fd, step, name,
  seek offset / whence, buffer / size, close flag) as an open / seek / read / close state machine
  (`FileReader::update` 0x1be564; `read` 0x1be524 resets the step, `resume` 0x1be548 does not) over
  rxSysStreamOpen(0x8001) / rxFileSeek / rxFileRead / rxFileClose. m_step is (step << 1) | waiting:
  a step with nothing to do adds 2, an issued one adds 1 and the next +1 comes once isReady
  (rxFileSync no-wait) is idle. Returns 0 once past the close step, 1 while busy (async callers poll
  next frame). A waiting reader draws main's now-loading object (`g_nowLoading`, sdata 0x2d8ffc,
  0x2051cc) when one is up, else rxThSync(1) + SndMan::update. `fileGetSize` 0x1be768 (open, seek
  to end, close, each waited out), `fileExists` 0x1be84c (open flags 1). Uncalled: three rxFileCd
  clock wrappers 0x1be880.., `fileClockToSeconds` 0x1be8bc / `fileClockDiff` 0x1bea18 (binary
  sceCdCLOCK bytes; the month table 0x2b8da8 has 0 for December, a ROM quirk kept).
- 0x7b40c0: the movie player `RxMpeg` (`snd/rxmpeg.cpp` 0x1810f4..0x1830fc, `done` 2026-10-25;
  ctor 0x1810f4 from the static initializer 0x2041f8; snd/rxmpeg.h has every member): +0 open, +4
  playing (S2dMovie::drawMovie draws while 1), +0x10 / +0x14 hold at a frame, +0x2c / +0x30 the
  frame buffer width / height (0x200 x 0x180), +0x34 faded out, +0x3c frame parity, +0x44 the name,
  +0xc4 the brightness level `startPaused` saves (GamMan status 0x10) and `close` restores (option
  0). A movie: `open`, `play(name, w, h)` (held: the ring fills) -> `isReady` 0x18176c -> `run`
  0x181644 (scene fadeIn under sequence bit 0x8000000) -> `getStatus` 1 -> `close`; `playNow`
  0x1814ac runs at once (MnuMovie). `checkNearEnd` 0x181780: fewer than 0x2800 bytes left (fades
  the scene out once). Under it a C library (`rxMpeg*`, data 0x298f00.., sceMpeg at bss 0x6e9200,
  audio channel bss 0x6e9288): movie/<name> through an rx file ring (0x60000, primed at 0x30000),
  sceMpegDemuxPss 2 x 16 KB a frame from `RxMpeg::update` (SndMan::update), the video stream into a
  0x80000 que (snd/unitque.cpp) that the IPU reads in 2 KB DMA units (the Nodata callback), two
  RGBA32 pictures (voData) flipped every other frame, the SShd audio (every disc movie: ADPCM
  0x10, 48 kHz stereo except DEATH_J 44.1 kHz; CP_LOGO has none) into a 0x20000 que fed to an
  rxSnd stream. Status 1 = idle or ended (no picture left and the file spent). `RxMpeg::draw`
  uploads the shown picture into the frame buffer (0xe00 / 0x12c0, 0 on odd frames), 64 lines down
  for pictures under 448: the movie is letterboxed 512 x 336 at y 64. On the host: the shim libmpeg
  (FFmpeg) decodes, the picture goes through `RosePan_I3dRecordMovie`, the decode loop runs on the
  main thread (rxMpegDecodePoll) -- see the banner of snd/rxmpeg.cpp. Disc: 40 movies in MOVIE/.
- **Room table** `WrdSrv` g_wrdSrv 0x298ee0 (wrd/wrdsrv.cpp 0x17e540..0x17f0d0; the class is the ELF's,
  vtable 0x2e4538, built by the file initializer 0x17f038): `load(stage)` 0x17e5c0 (from
  GamMan::loadStage; 99 keeps what is loaded) reads file "%03d" of data/wrd/WRD.BIN (RTPK, 26 stage files
  001..013, 098..) and `setup` 0x17e798 keeps a game-heap copy. Stage file: header {u16 0x101, u16 flags
  (bit 0 = offsets not relocated), size, index count (247: room numbers 0..246), room count (49)}, the
  index (u32 offsets, 0 = no room; padded to 16), then 0x40-byte rooms: +0 number, +4 id name
  "o1_cway", +8 floor (Shift-JIS), +0xc floor ("First Floor"), +0x10 caption ("Hallway"), +0x14 second
  caption ("Rickety Shed"), +0x18 floor number (0 outside, 1, 2, 3 attic), +0x1c map area, +0x20 the
  room's world offset, +0x2c / +0x30 the linked room numbers, +0x34 its 0x10-byte search record.
  `get(n)` 0x17ea44 by room number (gamGetWrdString 0x1db8e8: 0 / 1, 4 / 2 / 3 = +8 / +0xc / +0x10 /
  +0x14; "<map_none>" without a room), cnvPosL2W / W2L add / subtract +0x20, `findRoute` 0x17ec1c a
  breadth-first route over the links (one caller 0x127b28), `findByName` 0x17ea84 (no callers). The
  host builds WrdEntry records with pointers beside the file instead of relocating it (wrdsrv.h).
- `BedSubMode` (bed/bedsubmode.cpp asserts `m_depth < MAX_STACK` / `m_depth > 0`): a current qword
  at +0, a stack of 4 at +0x10, m_depth +0x50; push 0x100258 / pop 0x1002c0. The scene has one at
  +0x22d0 (`MagScene::pushSubMode` 0x132744 pushes and sets mode 8 / -1, `popSubMode` 0x132784): the
  movie skip runs in sub mode 8. Its four words are the scene's mode, step (-1 on entry), the
  requested menu id and the PC's mode << 16 | number (`requestMenu` 0x1357b0).
- **The scene's update** (`MagScene::update(pad0, pad1)` 0x1327dc, mag/magscene.cpp, 2026-10-22):
  S2dMovie::update, then by sub mode -- 0 the first frame (fog fade, the +0x837e0 objects,
  MagLifeMan::update(2), the PC's live, Brown, the actors' motion commands and footsteps, camera,
  sky billboards / rotation, the +0x853fc records) and 1 once the PC is bound; 1 walking (pad 0
  m_7c bit 3 the item menu 0x10000, bit 0 menu 0x10001, m_7a bits 8 / 9 changeCamera, the PC takes
  pad 0, Brown's command layer pad 1; the PC's state 0x17 goes to 6); 2 text windows (closes those
  with m_wait_key 0, or > 0 on pad m_hold (+0x78) bit 14; back to 1 when none waits); 3 fade out
  (getMenuFadeOutTime, fade kind 0 mode 6) then openMenu -> 7; 7 the menu takes pad 0 until
  processPad returns non-zero, then closeMenu -> 4; 4 fade in (mode 2) -> 1; 5 the system message
  (pad m_repeat (+0x7c) bits 13 / 14 close it); 6 the camera hangs from the PC and turns over 2400 counts; 8
  paused (lives of group 4 only); 9 the debug windows (magscenedebug.cpp). Lives run by group:
  MagLifeMan::update(mask) calls slot 2 of the lives whose level word has a bit of mask.
- **MagLifeMan** (`mag/maglifeman.cpp`, 0x1194b4..0x119b34, 2026-10-22; scene +0x2330, EE 0x404):
  64 pods of 0x10 `{life, priority, prev, next}` and the head m_top at +0x400. init zeroes the
  pods and points m_top at pod 0 (free; the first add takes pod 0 and links it to itself as the
  head). add takes the lowest free pod and links it before the first pod with priority >= its
  own (insertBefore 0x11950c) or after the last (insertAfter 0x1195ac); remove / removeNo unlink
  (0x1194c8, then clearPod 0x1194b4); find is a linear search of the pool (index, -1).
  update(mask): slot 2 when life->m_level (+0x10) & mask; draw(layer, group): for pods with
  priority & 0xc000 == group, slot 6 render(layer) unless slot 8 getHide (+0x24). deleteAll
  0x119660 (from ~MagScene and the scene term 0x131f74) runs slot 1 with flags 3 (deleting) on
  every life still in the pool, then init. An empty list (m_top NULL after removing the last)
  faults in the ROM's next add (insertAfter(NULL)); the game never empties it.
  MagLife base slots (linkonce): 1 ~MagLife0 0x282aa0, 2 MagLife0::live 0x282ad4 (m_04++),
  3 attached 0x282834 (empty), 4 getLevel 0x28283c (0), 5 getName (pure, 0x272a30), 6
  MagLife::render 0x282844 (empty), 7 setHide 0x28284c / 8 getHide 0x282854 (+0x24).
- **The scene text windows** (mag/magscene.cpp asserts `MAX_WINDOW`, `m_window[ n ]`): 4 slots at
  scene +0x2734 of 0xe0-byte MagLife windows, each with a font record at +0x2744 + n * 0x6c over the
  language font (+0x2908) and font texture m_font_tex_no[n]: `openWindow(w, h, a3, life)` 0x134730
  -> slot, `closeWindow` 0x1348b0, `fadeInWindow` / `fadeOutWindow(no, time, from, to)` 0x135088 /
  0x13511c, `setWindowShow` 0x1351b0 (slot 7), `getWindowTextWidth` 0x13555c, `updateWindowFont`
  0x135594. MagScene::setViewParam / drawView (0x134944 / 0x135234) take the same window numbers.

- **The 2D layers** (`spr/s2d.cpp`, pinned 0x190a78..0x192ac0, 2026-10-22; no source string, spr/
  from the link order after spr/sprtexture.cpp): each holds a sprite manager (`SprMan`,
  spr/sprman.cpp, EE 0x8ec: +4 active, +8 `SprTexture` loader (EE 0x438, DogTexInfo at its +0x414),
  +0x4ac the 0x154-byte sprite records, +0x4b0 the pattern table, +0x4b4 count, +0x4d0 / +0x4d4 the
  i3d layers to draw into) followed by the MagSystem pointer, and the same phase machine (open ->
  phase 0/3 start -> 4 process each frame -> 6/7 end). `S2dCineSco` (EE 0x92c): two black bars
  (sprites 1, 2) faded in / out by fadeIn / fadeOut 0x190d90 / 0x190e98 (black clear 0x299b70 ->
  opaque 0x299b80) -- the cinemascope letterbox. `S2dBg` (EE 0x940): up to seven pictures
  `sgbg0N_tcc` from data/mnurpk/s2dgen.rpk, files named by setTexName 0x1913c4, driven by the inline
  `s2dBg*` requests on +0x93c. `S2dMes` (16 sprites): wait.tm2 / wait2.tm2 from
  data/mnurpk/s2dman.rpk (the key-wait / write / select-cursor icons, the pad buttons, the message
  frames), 14 items mapped to sprites by data 0x29bb70, set by 0x191ba8..0x1920c4 which the inline
  setIcon* / setBtn* / setMessageSprite* call; the map-name caption (`drawMapName` 0x191938) puts
  word-table record +0x93c's text in a scene text window (0x200 x 0x100, centred on x 0x100 at y
  0x168) and fades it in / out with sprite 2 (its frame, offset 0x9c0 / 0x17c0).
  `S2dMovie` (EE 0x90c, no vtable): loading.tm2 from s2dman.rpk and the movie. MagScene::draw calls
  `drawMovie` 0x19241c (while RxMpeg +4 == 1: a layer at key 890 that RxMpeg::draw fills) after the
  3D layers and `draw` 0x1924e0 last (layers at 900 / 1000 into SprMan +0x4d0 / +0x4d4, then
  SprMan::draw); both pass light set 0 and the scene view, and set TEST_1 0x5000d (alpha GREATER 0,
  Z GEQUAL). `createLayer` 0x1925f0 = i3dCtxCreateLayer + a view from scene view 0 and the camera
  (+0x2ab70), which both callers overwrite at once. Movie wrappers: playMovie 0x192834 / 0x192850
  (name -> RxMpeg +0x44, open, play), isMovieEnd 0x1928b8, stopMovie 0x1928d4 (pushSubMode + stop),
  getMovieStatus 0x192904, closeMovie 0x192920 (close, popSubMode). 0x192978..0x192ac0 are four item
  setters (one item -> sprite 1, data 0x29bfb0) with no caller. Fade colours: data 0x2999b0 (grey, alpha 0) -> 0x299990 (alpha 0x80). The pattern
  tables are nested records with EE pointers inside, kept as writable EE word images
  (datadump.py --words) with their EE base in SprMan::m_pattern_ee (see "The sprite manager").

- **The sprite manager** (`spr/sprman.cpp`, pinned 0x18bb8c..0x18f5b0, 2026-10-22, `verify`; 53
  functions, all written): `SprMan` (EE 0x8ec; +0x440 a `GamFont`, +0x4c4 grey 0x80, +0x4c8 saved
  volume flag, +0x4d8 the open packet, +0x4dc the draw sets, +0x4e4 the font file, +0x8e8 font from
  the pack). Each sprite record (EE 0x154, spr/sprman.h has every field) runs on the owner's table
  image: **patterns** (0x14: texture id or -98 plain colour / -99 frame buffer, u, v, w, h in 1/16
  texels), **anims** (0xc: id, frame count (< 0: counted and written back), frames pointer; id -1
  ends), **frames** (0x1c: time | flags << 16 (0x10000 size from the frame, 0x20000 / 0x40000 flip
  U / V), pattern, x y z, w h (0x3e70 keep, -16 the pattern's); the word after the last frame holds
  the end code in its top byte: 1 stop, 2 hold, 3 loop, 4 jump to the anim in its word 2), **moves**
  (0xc like anims) and **keys** (0x20: type 0 ease / 1 ease from here / 2 linear / 3 linear from here
  / 5 halt, time, from[3], to[3] floats in pixels; -1 ends, -2 repeats). `draw` 0x18bd94 steps per
  sprite motion (updateMove 0x18d1e8: modes 1 ease, 2 speed capped at 8, 5 key table), anim
  (updateAnim 0x18d640), colour fade (setFade 0x18d7ec / updateFade 0x18dafc: modes 1..5, 0x100 |
  mode ping-pongs), then drawSprite 0x18e05c: position = parent's (m_parent > 0, drawn after the
  top level) + std + off + frame, the waits of states 2 / 3 / 4, and one GS sprite through
  i3dPktSprite / TexSpriteUV at XYZ + (0x7000, 0x7200); text sprites render m_text into their
  texture image with the GamFont (drawText 0x18ed44, Shift-JIS, "#0" = newline) and i3dTexRelease it
  for re-upload. drawFrameCopy 0x18bef8 puts the other frame buffer (TBP 0 / 0xe00 by
  MagSystem::m_frame & 1) over the screen on layer 1. `update` only steps the CLUT animations.
  volumeOn / volumeOff / saveVolume / restoreVolume 0x18f024..0x18f0b4 wrap
  `GamMan::switchVolume` 0x1d3ac8 (GamMan +0x22214 the saved volume, -1 none; setVolume 0x1d3a38).
- **GamFont** (0x1beb70..~0x1c0534, unassigned@001bde58, class named in the sprman session): a
  0x6c-byte software font rasterizer -- setFont 0x1bebec (font file: cell w / h bytes +0xe / +0xf,
  glyph bits +0x10, first code +0xc, flags +0xb), setTarget 0x1bedc0 (pixels, w, h, 4 / 8 bits),
  fillRect 0x1bef24, drawChar 0x1bf024, setColor 0x1bff64, getGlyph 0x1bffcc (bsearch / Shift-JIS
  walk), getGlyphLeft / getGlyphWidth 0x1c020c / 0x1c0254. Declared in lib/unassigned.h; constructor
  and destructor are written inline there, the rest are stubs.

- **The sprite texture loader** (`spr/sprtexture.cpp`, pinned 0x18f5b0..0x190a78, 2026-10-22,
  `verify`): `SprTexture` (EE 0x438) loads a list of `SprTexFile` (EE 0x18: id 0..59, file name,
  texture name, size, flags, buffer; id < 0 or name "" ends it) into the DogTexInfo at +0x414,
  one state per `update` call (0x18febc: 0 busy, 1 done, -1 after an error; state +0x11c).
  setList 0x18f744 picks the start: no pack -> loose files (5); `m_mode` 0 (+0x434, setListMode)
  -> fileGetSize + read the whole .rpk (3); mode 1 -> read its first 4 KB (1), then each file from
  the disc at the pack's offset. Entry flags: 1 = add while loading (else loadTex 0x18fbb0 adds it
  later from the pack), 4 = buffer borrowed from the pack, 8 = loadTex's RTCM decode (freed).
  Tables: +0x12c DogTexInfo numbers by load slot, +0x21c slot by id (-1), +0x30c the DogTexInfo
  16-byte record by id (`DogTexInfo::getTexInfo` 0x1161e4). The reads run through `updateRead`
  0x190618, a FileReader stepped through open / seek / read / close with the asynchronous helpers
  0x1be338..0x1be4c8 (rxSysStreamOpen, rxFileRead / Seek / Close, `isReady` = rxFileSync(fd, 1) ==
  0), waiting with rxThSync(1) + SndMan::update. **RTCM** (id string 0x2f7a90): a run-length
  format, header 0x20 (+8 decoded size, +0xc file size with the header, +0x10 short unit size),
  then records of a kind byte and a 24-bit big-endian count: kind 0 one unit repeated, kind 1
  count units (`decodeRtcm` 0x190904). When the list is done and +0x418 (an i3dContext) is set,
  DogTexInfo::setTexTable(ctx).

## The scene's lights (`mag/magscenelight.cpp`, 0x13f49c..0x13ff00, 2026-10-27)

MagScene's front end to its DogLightPool (+0x2050). **A map's `_l.bin`** (MagSceneTenkyu +0x500,
`MagLightFile`): +0 direct count, +4 point count, +8 the shadow light's index (counting the
directional light first, then the points), +0x10 ambient rgb, +0x20 0x20-byte lights (rgb, then a
vec4: directional = direction; point = position with w = intensity, which scales the rgb). Only
**one** directional light is read even when the count is larger. `loadLight` 0x13f658 restarts
the pool (term + create): ambient (alpha 1) into sets 0 and 1, the directional and point lights
into set 1. MagScene::load then calls `setDefaultLight` 0x13f8e4: a 0.75 grey ambient into set 6
(the movie / 2D layer's set). `setShadowLight(kind, n)` 0x13fbe4: m_shadow_light = empty
description, ambient `ShadowAmbient` (data 0x2eea80, zero, asserted 16-aligned), m_shadow_mode =
kind; 1 hangs +0x2000 as its one parallel light (src = the pool's directional light n), 2 / 3 as
its one spot light (src = point / spot light n), other kinds src NULL. `setLightFlags(theNum, set)`
0x13f528 = buildSet, and for set 0 the shadow kind by the first kind present: point, spot, 5,
directional. `addLight(type, mask)` 0x13fe64 walks nine mask bits (0..8; the pool has 8 sets).
setLightParam / getLightParam forward to the pool's global (no < 0) or set-relative accessors.

## The scene's skies (`mag/magscenetenkyu.cpp`, 0x13ff00..0x140990, 2026-10-27)

A map pack may hold `tenkyu.rpk`, a pack of its own: file 0 an `I3D_I3M` motion when its head
says so (compared with libc `strncasecmp` 0x27a4e0, n 7), then the sky's i3r. `loadTenkyuTex`
0x140030 (called by `MagScene::load` when a context is passed) copies the i3r to the game heap,
relocates it, adds its textures to `m_tex_info` (sepia = `m_sepia`) and assigns them. With a motion,
one heap block of num_node x 0x44 holds the model matrices (+0x508) then one float per node
(+0x50c): `initTenkyuSpin` 0x140390 poses the motion at 0 and at 2 x its length (pose flags 0x111;
scratch = num x 0xb2 + 2: 0x30-byte poses, local, end matrices, u16 node map with the count first)
and stores each node's turn (acos of the normalized row-1 vectors, signed by the cross product's y)
divided by 2 x length x 30 -- radians per frame. `rotateTenkyu` 0x14062c applies it with
`i3dMatRotY` every update; `drawSkyDome` 0x1406b8 draws at layer 490 with a white ambient-only
light, view 0 with far 100000 and zmin = zmax = 0, ALPHA_1 0x44 / FBA 0 / CLAMP 0 / TEST_1 0x13001,
world = unit at the camera's x / z (y 0). ROM bug: its draw loop runs four slots starting at the
first slot with a sky (reads past slot 3 when slot 0 is empty); the port stops at slot 3.
0x140990 opens the focus code (mag/magscenefocus.cpp, next section).

## The scene's focus effect (`mag/magscenefocus.cpp`, 0x140990..0x141e44, 2026-10-27)

A depth-of-field pass. `MagSceneFocus m_focus` at MagScene +0x853bc (EE 0x70; the assert text names
`m_focus.m_texindex`): +0 mode (bit 0 far1 plane, bit 1 far2 plane; setFocusData sets 3), +4 the
`efffocus` texture index (-1 = off; drawFocus returns), +8 the layer-key bias, +0xc val[3] = alpha
(1), far1 (300), far2 (600), +0x18 the track text, +0x1c size, +0x20 next line offset, +0x24 frame
counter, +0x28 wait frame, +0x40 three MagLerp. `efffocus` is a VRAM-only I3dTexDesc (256 x 256 CT32
at block 0x2700 = FRAME 0x138, bilinear), like shadow01 (block 0x2300 = FRAME 0x118, the shadow /
close-up target). drawFocus (last thing of MagScene::draw before the movie) runs, per plane, a
**shrink** (`shrinkFocusFrame` 0x140fdc: layer 860 + bias, FRAME 0x138, TEX0 the frame being drawn
-- block 0 on odd frames, 0x70 << 5 on even ones -- one hand-written REGLIST sprite 512 x 448 ->
256 x 256 by `magPktUVSprite` 0x140990) and a **plane** (`drawFocusPlane` 0x141184: the scene view
m_view[0], ambient-white light, efffocus draw set, one VU1 sprite at the camera's eye + dist along
its z, scaled to the frustum's size at dist, colour (1,1,1,alpha), TEST 0x5000d (z GEQUAL) so only
what lies beyond dist is overlaid). far2 is drawn twice (mode bit 1), far1 once. **The focus track**
(setFocusData: an event pack's file 2, tsk_scene) is text, CR LF lines `frame command args`,
lowercased, split at spaces / tabs: `start far1|far2|far1far2`, `term`, `setalpha FROM TO N` (FROM < 0
= where it is), `setdist PLANE FROM TO N`, `movealpha TO N`, `movedist PLANE TO N`; a line whose frame
is ahead becomes the wait; an empty line ends the effect (setFocus 0). Native 281 op 13 drives the
same values. PC: neither pass draws on the host yet (no FRAME_1 targets / VRAM-only textures in the
native i3d module).

## The scene's camera (`mag/magscenecamera.cpp`, 0x13c664..0x13f07c, 2026-10-27)

MagScene's camera words +0x2ad50..+0x2ad98 (the assert texts name m_cam_map_no +0x2ad68 and
m_cam_path_no +0x2ad64): mode +0x2ad50, +0x2ad54 m_cam_move_frame (the 30-frame move's frame; changeCamera clears it), target actor +0x2ad58,
distance band max / min +0x2ad5c / +0x2ad60 (600 / 500), saveCamera's count +0x2ad80 and copies
+0x2ad84..+0x2ad98. `setCameraMode(mode, arg, param)` 0x13caf0 (jump table 0x2ee750): 0 free, 1 free
with a target actor, 2 the camera a child of actor arg at (0, -50, -50) tilted -0.5, 3..5 a map camera
on a path of a sky pack's _c.bin (MagSceneTenkyu +0x4fc) -- 3 slot 0's path 0, 4 arg = map << 8 | path
(checked against getCameraMaxPath, the magscene.h line-816 inline), 5 the same plus param[0] / [1] /
1000 as the path position; the path's float +4 (degrees) becomes DogCamera +0x140. With no path in
slot 0, modes 3..5 make the camera a child of the actor at (0, -150, -500) tilted -0.17. A path record
(`MagSceneCRec`): +0 m_pos_num, +1 loop, +2 alt (one-point paths with two views, +3 the side, the
second view's point 4 on), +4 fov, +0x1c..0x1e tag letters (step speed, distance, band), +0x20 / +0x28
the camera's segment and fraction, +0x24 / +0x2c changeCamera's goal, points from +0x30 (x, y, z, w =
length of the segment ending there). The target is the actor's m_trans 120 up. seekCameraPath
(native 016 case 7) places the camera and iterates scoreCameraPath 0x13d8f8 (one step toward the
distance band, 1e6 - squared error) until it settles; changeCamera 0x13d5e4 (pad m_7a bits 8 / 9)
sets the goal two projected lengths ahead and ORs 0x10 into the mode, which updateCamera 0x13e548
plays out over 30 frames (lerping seg + t to goal_seg + goal_t, looking at the actor), then copies the
goal and clears the bit (a one-point path just drops the bit). updateCamera per mode: 1 lookAt the
actor; 3 scores every path of the four sky slots into its +0x2c word (the goal word reused; the
current path +200000) and takes the best; 4 slides the current path (scoreCameraPath), 5 holds it; 6
DogCamera::updateMove (so updateCamera is not the writer of DogCamera's move words +0x190..+0x1d0 --
mode 6's setter is). A one-point two-view path is two 0x40-byte records back to back (header + one
point); the second is the view while +3 is set. A record's +0x1f letter (n = c - 'a') limits the
look instead of lookAt: +-(n / 8) * 30 degrees in x and +-(n % 8) * 10 in y around the floats
+0x10 / +0x14 (clampCameraLook 0x13e2ec, getCameraLookLimit 0x13e4c0). After any mode 3..5 path
change the new path's fov becomes the lens angle.

MagScene's menu glue (`mag/magscenemenu.cpp`, 0x13f07c..0x13f49c, no path string): gamEnterMenu(menu)
= g_gamMan.pauseForMenu(menu) (requestMenu passes its id; the system message 0x1000a),
gamLeaveMenu = resumeFromMenu(0); the menu fade times are 0 with g_sequence.m_bits[1] bit
0x4000000, globals 0x170 (out) / 0x16c (in) with bit 0x2000000, else 15; openSysMsgWindow expands a
system text (msgGetSysText) through MsgMan::expandText 0x184128 (record: 16 args, src +0x40, dst
+0x44, size +0x48) into a 256 x 96 window at (128, 224) and holds the scene in sub mode 5.

## Fog and Brown's command glue (`mag/magscenecmd.cpp`, 0x143d3c..0x145020, 2026-10-27)

No source string names the run (the port named it): it follows `mag/magsceneif.cpp`'s asserts and
ends where the Hfm enemy classes start. Fog: `MagScene::m_fog[4]` (+0x2aae0, I3dFog 0x20 each) --
[0] the fog drawn, [2] / [3] a fade's from / to; `setFog` 0x143d7c with frames > 0 sets from =
drawn, to = fog, `m_fog_frames` (+0x2ab64) = frames, `m_fog_frame` (+0x2ab60) = 0, else draws fog
at once; `updateFog` 0x143fd0 (MagScene::update) blends linearly at frame / frames. Wrappers: a
colour + four floats 0x143d3c, `addFog` 0x143eec (target + delta). Brown: every `gam*Bwn*` /
`magBwn*` function forwards to the scene's MagLifeBwnCtrl (bss 0x727aa0 = scene +0x2bd20). Its
states (m_state; the four-char codes op 3 returns): 0 NULL, 1 WAIT, 2 FOLW, 3 STAY, 4 FIND,
5 FDED, 6 MOVE, 7 MVED, 8 ROTA, 9 RTED, 10 DAMG (requestHurt), 11 BARK (sub object at +0xf0),
12 HUG; `setLevel` 0x12b7cc (+0x6c) takes MAG_BWNCTRL_LEVEL_AUTO 0 .. MANUAL 2 (gamSetBwnFollow:
on 0 / off 2; gamSetBwnWait: on 1 / off 0). Script native 039 -> `magBwnSeqCommand` 0x144ae0:
ops 3, 0x14 rot, 0x15 pos, 0x1f moveTo, 0x20 / 0x23 wait until +0x80 (SEQ_RET_WAIT), 0x21 /
0x22 angle (+0x40, state 0 / 8), 0x24 returns 1; four-char ops ifsc / gfrs / gfit (find result,
+0x68 / +0x7c / +0x44), stfd / clfd (find item, again = AGIN/agin/1; cancel), amsb (an actor's
suffix motion), eqds (show / hide the PC's equip item), sepo (MagScene +0x22cc `m_sepia`: the sepia
flag texture and light loads pass), sepc (DogTexInfo::setSepiaColor, clamped 255), ilts / adlt
(lights), spmn (special menus, table 0x2ef298: KLCK / SPM0 0x1000b, BVIL / SPM1 0x1000c, BLND
0x1000d, BFML 0x1000e, BCPL 0x1000f, after utlPauseForMenu), whti / whto (white fades, level 7),
rcst (commandPC 0x14), gmnr (the menu result +0x2be58).

## The player's controller (`mag/maglifepc.cpp`, 0x11ab40..0x11c464, 2026-10-27)

- `MagLifePC` (EE 0x220, scene +0x2adc0) is a state machine: `m_state` (MagLife0 +0) picks one of 20
  mode objects embedded at +0x138..+0x1a8 (`mag/maglifepcmode.h`: each is only a vptr, a few carry
  words; three EE slots init(pc) / live(pc) / pad(pc, pad), names from the ror-symbols
  `MagLifePCMode::init` / `::pad` and `MagLifePCModeHold::live`). States: 0 Null, 1 Wait, 2 Walk2D,
  3 Run2D, 4 Turn, 5 AtkWait2D, 6 AtkMove2D, 7 Roped, 8 Hold, 9 Ride, 10 Attacking, 11 (no mode:
  `beginAttack` picks a target, then 5), 12 HoldAttack, 13 Unhold, 14 Damage, 15 DogGo, 16 DogCome,
  17 DogStay, 18 Hug, 19 Item, 20 Reload.
- `changeState(theMode, arg, force)` 0x11c1e8 (45 callers, most in the modes): clearMotionSeq, the
  base's slot 9 `MagLifeControlChara::setState` 0x119fe0 (saves the state quad at +0x30 when
  m_state != 0; m_08 = arg, m_04 = -1), `selectMode(1)` 0x11c2ac (current mode + its init).
- MagLifeControlChara's slot names are ror-symbols: slot 12 `reset4MapJump`, 14 `message` -- so
  MagLifePC's overrides are `reset4MapJump` 0x11be60 and `message` 0x11b5e0 (renamed from reset /
  command). Messages: 0/1 hold, 2/7/10/12 ride, 6/9/11/13 to the Ride mode, 0x12..0x14 stamina,
  0x15 move the actor (sweep), 0x16 riding / hurt query, 0x17 weapon override, 0x18 health override.
- processPad (states 1..11): status words via updatePadStatus -- health class m_health (+0xe4,
  calcHealth: override, mode 1 -> 3, 3 -> 4, PC param 0x1b / 0x1c ratio <= 0.3 -> 2, stamina),
  move level m_move_level (+0xe0, readStick: dead zone +0x1f8, run above 0.97), stance m_stance
  (+0xdc, R1 0x800 held -> weapon type + 1). A stance change arms (state 11) or disarms (0); cross
  (0x4000) in 5 / 6 attacks; L1 (0x400) reloads; triangle / circle / square order Brown (states
  15 / 16 / 17, needs script flag 34 and MagLifeBwnCtrl level 0); else the mode's pad until it
  returns 0 or 1.
- live: the shadow blend scene +0x22b0 (0.1 in mode 3, else back to 1 by 0.05), footsteps
  (`playStepSe` with the 12 floor-kind sound ids at 0x2e9ef8), the mode's live, root motion outside
  the moving states (`isMoveState`: 1..10, 14).
- The PC's motion sequence table (0x18-byte rows at 0x2961e0: "base", "run_out", "wait", "tired",
  "w_wait", "hold_in" ... "sude_in_ml", 60+ rows) is passed to MagLifeControlChara +0x5c; it is not
  on the host yet (NULL) -- it comes with updateMotionSeq's TU.
- Tuning words (MagScene::parseOption options 6..14 -> `setParam`): +0x1f8 stick dead zone 0.1,
  +0x1fc / +0x200 cos 5 / 89 degrees, +0x204 0.07, +0x208 0.9, +0x20c 0.001 (stick change),
  +0x210 / +0x214 45 / 20 (move sweep range / radius).

## The texture table (`dog/dogtexinfo.cpp`, 0x1161e4..0x11764c, 2026-10-22)

- **DogTexInfo** (EE 0x20; the scene's at +0x29520, 256 entries): +0 m_max_tex, +4 m_num_tex,
  +8 m_tex_info (theMax 16-byte name records = the i3d context's texture table `I3dTexEntry`:
  pszTexName, pPicture, CLUT, a word the i3d regs use; names from the asserts), +0xc m_tex_data
  (theMax 0x198-byte `DogTexData`: I3dTexture +0, I3dClut +0x130, the TIM2 +0x168, own flag
  +0x16c, reference count +0x16e, name[40] +0x170), +0x10 m_dirty (every change; MagScene::draw
  then calls setTexTable), +0x14..+0x1c a sepia colour (0xff 0xdc 0x98) that addPfh's `sepia`
  argument applies to each TIM2 (`utlTm2ToSepia` 0x10559c: luminance 0x4d/0x96/0x1d times the
  factor >> 16; its caller `setSepiaColor` 0x11626c is called from 0x144ae0).
- Records are found by name. addData (TIM2) counts a reference when the name is there (unless
  flag 2), else takes the first empty record (findFree), copies the TIM2 to the heap when flags is
  not 0, and builds the texture / CLUT (tcc < 0: utlIsTccName); add (an I3dTexDesc already in
  VRAM, e.g. shadow01 256x256 CT32 at block 0x2300) does not count references. release drops one;
  the last terms the i3d objects, leaves the name "" and clears the record. releaseModel /
  releasePfh release by an i3r model's / a pack's names and trim m_num_tex; setTexTable trims past
  the last record with a picture and calls i3dCtxSetTexTable(ctx, m_tex_info, m_num_tex).
- addPfh(pfh, i3r, flags, sepia): every pack file under its name (i3r NULL), or for each texture
  the model names (`i3rGetNumTex` 0x22b428 / `i3rGetTexName` 0x22b438, DogModel's inline get_nTex /
  getTexName) the pack file of that name; -3 when the pack lacks one.

## The model (`dog/dogmodel.cpp`, 0x11440c..0x1160a8 + inlines 0x281cbc..0x2827c8, 2026-10-26)

DogModel (EE 0x1a0) = DogEntityEx + an i3r model. Fields by the ROM's assert names: m_ref_count
+0x104, m_tex_info +0x10c, m_load_flag +0x110 (LOAD_NONE 0; 1 owns m_pack_file, 2 owns root + i3c,
3 share of m_original +0x118, 4 = loadPack / loadData: caller's data), m_pack_file +0x114,
m_i3r_root +0x11c, a second root from pack file 2 at +0x120, m_i3c_root +0x124 (collision, from
loadData's third argument: the scene's <name>_col file), m_colli_tbl +0x128 (u_short model node per
i3c node), one block m_mat_local +0x12c / m_mat_model +0x130 / m_mat_flag +0x134 (nNode * 0x84
bytes from g_utlMemory), shadow light words +0x160.., a 0x30 display control at +0x170 whose address
utlSetI3rDispControl 0x1067d8 writes into every material. Model packs: file 0 the TIM2 pack
(DogTexInfo::addPfh with the sepia flag), file 1 the i3r, file 2 optional. DogEntityEx +0xa0 is the
local matrix; DogEntity +0x80 is a rotation added to m_rotate (DogEntity::makeLocalMatrix 0x10fd18,
order scale, Z, X, Y, translate, offset). A model hung from a parent node turns a fresh local matrix
by (x -> -x, y <-> -z) first. Billboards: m_flag100 bits 1..3 (on / with X turn / towards a point),
used by slot 11 transformBillboard. Casts build an I3cQuery (root, node matrices, colli table, group,
hit callback 0x114034 / 0x1141c8, hit record) for i3cCastRay 0x232508 / Sphere 0x2324b8 / Cylinder
0x2325a8 with 64 unit matrices; the hit record is MagSceneRayHit (t at +0x0c, m_50 group).

## The actor's motions (`dog/dogactor.cpp`, 0x108674..0x10f860, 2026-10-27, partial)

DogActor (EE 0x4d0) = DogModel + motions. Names from the ROM's asserts: m_flag_motion_data +0x1a8
(-1 none, 0 records point into packs, 1 FLAG_MOTION_EACH: each i3m its own block),
m_pack_motion[4] +0x1ac (bank 0 = the model pack's file 1, never released by pointer; 1..3 =
loadPackMotion's heap blocks), m_max_motion +0x1c0, m_motion_data +0x1c4 (0x20-byte records: i3m
+0, param words +4, root track +8, m_tag_data +0xc, +0x10 a byte flag, id +0x14, bank +0x18, name
+0x1c), m_posture +0x1cc (one block of nNode * 0x62 + 2: current pose records, previous ones at
+0x1d0, m_assign_tbl +0x1d4 = count + model node per motion node). Two 0xe0 DogMotionBlend states:
m_motion +0x1e4 (no, speed, frame, the view m_max_motion +0x18 / m_motion +0x1c, four 0x30 blend
entries at +0x20) and the previous one +0x2c4 (savePosture). A motion pack file is either an i3m
(i3m word +0xc is its size; when the pack file is longer a root track follows) or an RTPK of parts
(0 i3m, 1 params: word 0 to +0x10, the rest to +4, 2 root track, 3 tag track); flag 0x10 packs
carry the ids in the files' user data (& 0xfff). Root tracks: numframe, type (stride by type: 0/4
four floats, 1 six, 2 two, 3 three, 5/6 one), 8 bytes, then frames; getRootMove samples one at a
15-bit fixed-point time and adds the whole track's move once past either end. getRotY = m_base_rot
y (+0x3c4) + m_rot_offset y (DogEntity +0x84, our m_80) + the root track's turn, unless m_mode bit
1 (isBlendIn) gives m_rotate y. load: reset, mode 2 copies the pack (m_model_copy +0x1a4), slot 14
loadPack(file 0) then allocMatrix, loadMotion(file 1, bank -1). The two loads (slots 14 / 15) end in
setupEye (the first node named *eye*).

The per-frame player (written 2026-10-27, second session): `runMotionCmd` 0x10d388 (MagScene::updateActors)
runs the command list m_motion_cmd[8] +0x42c from m_motion_cmd_idx +0x44c (DOG_MAX_MOTION_CMD 8,
asserted; +0x450 = the command started its motion, +0x428 = the last step's result). A word: op =
bits 28..31 (1 jump to command op1, 2 play once then next, 3 loop and hold, 4 as 2 with a chance of
bits 12..21 out of 1000), flags = bits 22..27 (1 look up first, 4 start at the current time, 8 scale
it to the new length, 0x20 stop at the end and hold), op1 = bits 0..9 (a motion id or index). Ops
0 / 5..15 do nothing, so a list must end in a jump or 0x20 (the ROM spins otherwise; the host gives up
after a pass). Without a list, m_mode bit 0 steps the motion alone by m_motion_flags +0x3f0 (1 loop;
else at the end 2 changes to the motion in +0x3f4, none of 1 / 2 / 4 stops). A loop wrap turns the
heading by the root track's last key w (forward +, backward -). m_interpolation +0x1d8 (assert name)
is the blend-in length: m_f4 +0xf4 counts frames, the rate is (m_f4 + 1) / m_interpolation, or a
percentage per frame from the record's m_param curve when its first word equals m_interpolation;
getBlendInOffset fades the saved trans-Y +0x420 and rot +0x424 into the new root. `draw` (slot 12)
updates the posture every m_45c-th frame: DogMotionBlend::getPose into m_posture, per node a pose
lerp with the previous pose by getNodeBlendRate, the control node (+0x3d4, flag +0x3d8) quaternion
from the euler m_3e0 (set, or multiplied in), i3mPoseToMatrices into m_mat_local, setNodeScale's
nodes (> 0) scaled, convertLocalToModel; then billboard / transform, `drawModel` (slot 19:
utlDrawI3r with m_mat_model; the m_attention node's material points at the pulsing record +0x4a0
for the draw; a model with m_16c (a DogModelAnim: value stepping between bounds) passes utlDrawI3r a
{0x1f, value x5} record), and the root turn m_rot_y_aar into the heading. `drawShadow` (slot 20)
draws m_i3r_root2 (the pack's file 2: the shadow model) when present. updateEye blinks: texture
state 2 for 4 frames, 1, 0, then a new 60..90 frame period; setEyeTex points the eye material's
entity 0 at m_eye_uv with v = {0, 0.3203, 0.6484}. checkName compares theName with the model pack's
file 0 name up to its '.'.

The rest of DogActor (third session, 2026-10-27; the TU is complete): `getMotionMove` 0x109348 is the
root move between two frames (non-looping motions clamp to the last key; a looping one returns 2 and
the track's whole turn when the step crossed the end, or its negation going backwards); only a track
type with bit 0x10 keeps the y move. The **tag track** (record +0x0c) is not a DogMotionTrack: word 0
the frame count | 0x80000000, then one u_int of event bits per frame; `getMotionEvent` 0x10bcc8 ORs the
words after frame t0 up to t1, wrapping (the argument: < 0 the current motion, < 0x10000 an id, else a
record index). m_base_trans 0x3b0 / m_base_rotate 0x3c0 are where the root track started
(`resetRootBase` 0x10ac98 via `getRootBase` 0x10a2d0); addTransRotY / setTransRotY 0x10a090 / 0x10a1c8
set the position and a heading net of the root's current turn. Blend entries (DogMotionBlend +0x20,
four of 0x30): +0x00..0x0f a 128-bit node mask, +0x10 speed, +0x14 frame, +0x1c a rate in 0..1
(-1 none), +0x20 motion record, +0x24 a state (2 = fading), +0x28 the fade time, +0x2c its count;
`setMotionSub` / `setMotionSubBySuffix` 0x10b328 / 0x10b620 start one over the nodes named in a
NULL-terminated list, keeping their current pose as the previous one. The ELF names the class body's
inlines: getMotion(int, tag_I3mPosture_s *, float, I3M_INTER) wraps i3mGetPose, getBlendMotion wraps
i3mGetBlendPose 0x22eab0, i3mGetNodeIndexByName(int, char const *) the i3m lookup.
DogMotionBlend (dog/dogmotionblend.cpp, written 2026-10-27): the asserts call the motion fields m_main
and the entries m_sub[4], m_frame m_main.m_time. getPose 0x1081b0 samples the main motion (i3mGetPose
flags 0x111) and then each entry's into a 128-node stack block and copies it over the masked nodes
(i3mPoseLerp with t 0) -- the fade itself is the actor's, through getNodeRate 0x1078dc ((m_count + 1) /
m_time while fading, else 1). step 0x107e30 clamps (1 end, 2 start, 3 zero length or speed 0);
stepLoop 0x1079d8 wraps (1 forward, also when speed is 0; 2 backward; +-1e-5 of the end snaps to 0)
and stops a playing entry at its m_1c ratio; a fading entry (setBlendRate 0x108428) holds frame m_1c *
length at speed 0 and counts m_count up to m_time; setBlendRate with the same rate 0 after the fade
frees the entry. The i3m pose / quaternion ops below it are written (i3d/i3mpose.cpp, 2026-10-27).

## Modules (RoR's own source files)

73 `.cpp` paths survive in assert / error strings (export.py anchors on them), plus headers whose inline
functions assert (`.h`). Directories and files:

| dir | files |
|---|---|
| `bed/` | bedmemory, bedpad, bedpfh (+.h), bedsubmode |
| `debug/` | mimu_debug, motnamecnv, motnamecnv0, naga_debug |
| `dog/` | dogactor (+.h), dogcamera, dogentity, doglight, doglightpool (+.h), dogmodel (+.h), dogmotionblend (+.h), dogtexinfo |
| `emy/` | emy_main |
| `game/` | gamman, gamsnd, gamstat, gamtrg |
| `mag/` | maglife, maglifebwn (+.h), maglifebwnctrl (+.h), maglifebwnmode, maglifefadekey, maglifeman, maglifemenu, maglifemenusp, maglifepc, maglifepc2, maglifepc3, maglifepcbtl, maglifepceff, maglifepcmode, maglifepcmove, maglifepcmsg, maglifetest, maglifewindow, magloader, magscene (+.h), magscene2, magscenecamera, magscenedebug, magscenefocus, magsceneif, magscenelight, magsceneoption, magscenerender, magscenetenkyu, magsystem (+.h) |
| `mcd/` | mcdman |
| `mnu/` | mnuSelectLanguage, mnudlg, mnuselect, mnusvld (named by class: mnubase, mnumap, mnupbook, mnustock, mnumcd, mnutitle) |
| `msg/` | msgman |
| `sequence/` | sequence |
| `snd/` | rxmpeg, sndman, unitque |
| `spr/` | sprman (+.h), sprtexture |
| `system/` | sysnowloading |
| `task/` | naga_chunk, tsk_bg_load, tsk_scene |
| `utl/` | utlbtl, utlfile, utlfind, utlgame, utlmath (0x106658..0x10737c: `utlConvSndId` -- a sound id 0x0a0Nxxxx swapped for its twin of the current map area, `GamMan::getMapArea` column of the 54-row table 0x295f08; `utlDrawI3r` = `i3rDrawModel` 0x21fe68 with dispatch held off; node display words; vector rotations / dot / cross / lengths / distances / direction to angles with the ROM's pi 0x40490fdc and pi/2 0x3fc90fda), utlmisc (0x105650..0x105e50: text helpers, `utlParseParam` name/value lines, the frame counter, operator new/delete, the game heap), utltm2 |

Files without an assert are not named by the binary (`main`, the rx middleware, every file whose code
never asserts): sessions name them (`tus.py define --why`).

## Classes

- 284 vtables, one per polymorphic class (`rtti.py`; no secondary `_vt$A$B` tables: no multiple
  inheritance with virtual bases on the polymorphic side). Root classes: DisplayMessage, DogEntity, Eff0,
  EmyMain, EmyOut, GamMan, GamSndMan, MagLife0, MagLifeBwnMode, MagLifePCMode, MagSceneIF, ObjStat,
  ScenePlaySingle, SctLayer, SeManager, VoiceManager, WrdSrv, mnuBase (+ the runtime's exception,
  type_info).
- Families by prefix: MagLife* / MagScene* 79, Eff* 43 (all `: Eff0`), Tsk* 30, and per-enemy state
  machines `<Type>Main : EmyMain` + `<Type>Out<State> : EmyOut` for the types Bdl, Bds, Clr, Dwf, Fsh,
  Got, Grg, Hfm, Mrt, Nor, Pig (EmyOut has 77 subclasses). `CmnITask` is the base of 32 classes but has
  no vtable of its own in the ELF (abstract; only its pause slot 0x28a694 has a body). Its six slots
  are in "The task manager" below.
- **Enemies (emy/, 2026-10-27).** `EmyManager` (emy/emy_manager.cpp 0x16265c..0x164118, global
  0x7b2140, a CmnITask of kind 3 added by main; EE 0x430): m_emy[64] at +4, per-type limits +0x104
  (filled with bytes of 100: no limit), an oid pair +0x134..+0x13c, level +0x140, made-oid count
  +0x144, EmyGroup[8] (0x3c) at +0x148, a work word per slot +0x330. Its exec (slot 4) runs
  spawnGroups (groups 1..7: wake an idle member or respawn a gone one) and updateEmys (per enemy: the
  ptmf at +0x1dc, the group step 0x165074, then slots 4, 5, 6 while managed (+8) and not damaged).
  `createEmy` 0x162c88 news the class by type -- 0 Dwf 0x274, 1 Pig 0x270, 2 Got 0x270, 3 Fsh 0x25c,
  4 Bds 0x228, 5 Bdl 0x228, 6 Hfm 0x240, 7 Mrt 0x23c, 8 Clr 0xa80, 9 Grg 0x244, 10 Nor 0x258 (EE
  sizes) -- through the ELF's inline `XxxMain::XxxMain(u_int oid, u_short name, ACHRstat *,
  _tagEnemyParam const *, _tagEnemyTransition const *, float *)` = EmyMain ctor 0x164118 + vtable +
  `XxxMain::initOut` (builds the EmyOut objects; Nor's takes the param; Clr's is its slot 7), then
  setStartState 0x164364 (param +0x10), loadParam 0x164508, footstep tables (data 0x2f12a0..) into
  MagScene::m_actor_snd. EmyMain's slots: 1 dtor, 2 selectOut (current state's out), 3 GetType, 4
  decide, 5 exec (default 0x164fc0), 6 after; fields from the ELF's accessors (oid +0, name +4,
  manage sw +8, param achr +0x20, state +0x3c, out +0x44, group +0x1c8.., damage end +0x1e4, move /
  turn tasks +0x1f0 / +0x1f8, vptr +0x200). The oid wrappers (0x163624..0x164078) do no type checks
  except the hold words (+0x23c type 6, +0x248 type 8, +0x24c type 10).
- **The enemy loop (2026-10-27, emy/emy_hfm.cpp).** Every `XxxMain` runs the same pattern: `decide`
  (slot 4) writes a transition-table input into `m_req` +0x24 (-1 none; nothing while `m_decide_sw`
  +0x28 is 0 or a request is pending), testing only inputs the current state's row has (`m_input[]`
  at +0x204 of the derived class, filled by `selectOut`); `exec` (slot 5) moves the state graph
  `EmyStateMap` +0x1c (EE 0xa8, a GamIdMap per state, current state +0xa4; `next` 0x16968c) --
  notify bit 2 (killed) forces the class's dead state, bit 1 (hit) restarts the state -- and sets
  `m_changed` +0x40; `after` (slot 6) calls the old out's Term, `selectOut` (slot 2: `m_out[state]`
  +0x48, 40 pointers, and its Init) and stores the out's Exec result in `m_req`. `_tagEnemyTransition`:
  +0 rows (states), +1 columns (inputs), +0xc the row bytes (next state, 0 none). EmyMain also has
  the motion list +0xec (GetMotionList / SetMotionList, ints) with a byte per entry at +0x128, the
  motion-ended input word +0x2c (SetInputFlgMotion) and `m_agl` +0x1c4 (set_agl: the motion speed x 60).
  **EmyOut** (ctor 0x166a1c `(oid, achr, emy)`): oid +0, step +4, achr +8, emy +0xc, target-is-dog
  +0x10, target oid +0x14, bitten +0x1c, move done +0x24 (the move task's flag), looping SE +0x2c (-1),
  +0x30 = utlRandRange(8, 24); vptr +0x34, slots 2 Init, 3 Exec (returns the next input, -1 none),
  4 Term (the ELF's `HfmOutTest::Term`); the common Term is `termCommon` 0x166ad0. Helpers
  (unassigned@00166a1c): `emyChkInSight` 0x169bac (a0 useAngle, a1 target oid, a2 achr, f12 range,
  f13 angle), `emyCanReach` 0x16976c (secta cells), `emyStartMove` 0x1670dc (mode, out, pos: the
  enemy's TskEnemyMove), `EmyOut::setMotion` 0x1680a0 / `setWalkMotion` 0x1682b0 (chrMtnMove2Achr),
  `selectTarget` 0x166da4 (PC or dog), `emyStartChase` 0x166f50 (f12 stop range, out, target oid: the
  move task follows an object), `EmyOut::playSE` 0x1694cc (EmyMain::getSE(no) at getBasePos, returns
  the handle) / `tickVoice` 0x1693ec (lo, hi, se: a voice every lo..hi frames into +0x2c) /
  `stopVoice` 0x16947c, `emyChkHitReach` 0x169f94 (f12 range, oid, n p_hit nodes, target: the attack
  hit test), `emyVecLength` 0x169a14, `EmyMain::stopAllSE` 0x165570. The outs' attacks land through
  `scnChrRequest(0xb, emy oid, 0, 0, word, victim oid, 0)` (Hfm slap words 0x26 / 0x27 chained /
  0x28 close; 0x2d the dog thrown off), after `getPCStateClass() != 9` (PC) or `getBwnState()` not
  10 / 0xd (dog). Hfm Exec results: 7 the state is over, 8 the chase gave up. **HfmMain** (type 6): states 1 Walk, 2 Slap, 3 Bitten, 4 Break,
  5 Counter, 6 Ride, 7 Dead, 8 Win, 9 Test; inputs 1 the dog barked (magBwnBarkQuery; also 60..120
  frames into +0x234), 2 slap chance on motion end (0/10/30/50/70/100 % by try), 3 / 4 the PC came
  into / left sight (param 0x5f range, pi), 5 a_lp under a quarter, 6 PC state class 4. Its words:
  +0x22c / +0x230 a counter and limit (SetFreeCnt is +0x230), +0x234 the count subFreeCnt lowers,
  +0x238 IsMissing, +0x23c ChkRideSide.
- The Dog* object layer: DogEntity (vptr +0x94) <- DogEntityEx <- DogModel <- DogActor / DogCamera
  (`DogActor` 21 virtual slots); DogEntity::reset 0x280ec8 writes 16-byte vectors at +0x40 / +0x50 / +0x70
  (quadword `sq`). Layout (2026-10-22, the magsystem actor methods; `src/dog/*.h`): DogEntity +0x00
  world matrix, +0x40 translation, +0x50 rotation (radians, `normalizeRotate` 0x10ff74 wraps to
  [-pi, pi)), +0x60 scale, +0x70 offset (y = TransOfsY), +0x80 a (0,0,0,1) vector of unknown role,
  +0x90 dirty flag, +0x94 vptr; DogEntityEx +0xa0 local matrix, +0xe0 parent (`setParent` 0x1102b4
  refuses cycles and clears the offset). dog/dogentity.cpp (2026-10-27) = 0x10fc7c..0x1103dc: the local
  matrix is unit, scale, rotZ, rotX, rotY of m_rotate + m_80 (each wrapped to [-pi, pi)), translate
  m_trans then m_trans_ofs; DogEntityEx::transform 0x11034c = the parent's slot 2, then parent * local
  (row vectors). The host keeps the class in the vptr word (reconstruction.md, polymorphic classes).
  Files link alphabetically here: dogactor, dogcamera (0x10f860), dogentity (0x10fc7c), doglightpool
  (0x1103dc);
  dog/dogcamera.cpp (2026-10-27) = 0x10f860..0x10fc7c: DogCamera +0xf0 view matrix (getMatrix
  0x10fa6c, filed under DogEntity by its symbol: slot 2 transform, then i3dMatInverse of the world
  matrix), +0x134 move frame / +0x1d0 move length (updateMove 0x10f944 lerps translation +0x1b0 ->
  +0x190 and rotation +0x1c0 -> +0x1a0 by frame / length, utlLerp4 0x106d40), +0x140 lens angle /
  +0x144 its scale unit, +0x148 / +0x150 / +0x160 / +0x170 the saved angle, translation, rotation,
  scale with +0x180 m_saved (saveCamera 0x10f860 / restoreCamera 0x10f8cc, one level, asserted);
  getRotY 0x10fc28 = utlVecToRotY of the world z axis laid flat; getLensFov 0x10fac0 maps the angle
  through 15-degree steps of a 13-entry degree table with modff 0x26c8e0.
  DogModel +0x11c `m_i3r_root`, +0x124 i3c root, +0x128 colli table, +0x130 node matrices (0x40 each),
  +0x134 node flag, +0x138 attach node, +0x13c attention, +0x140 hide, +0x144 light id; DogActor +0x1a0
  id, +0x1c0 / +0x1c4 motion count / `m_motion_data` (0x20 each, banks 0..3), +0x3c0 base rotation,
  +0x1e4 `m_motion` (DogMotionBlend: motion index, speed in frames per frame -- the scene stores
  fps x 1/60 --, frame), +0x3f0 / +0x3f4 setMotion's two flag words,
  +0x3d0 RotYAAR, +0x3d4 / +0x3d8 control node / flag, +0x420 TransOfsY save, +0x424 RotOfsY; EE size
  0x4d0. Virtual slots: 10 = `DogModel::setAttachNode`, 17 = `DogActor::getNodeWorldMatrix` 0x10eab8
  (m_matrix x node matrix through 0x2377d0, a VU0 micro call). The many tiny accessors are the ELF's
  linkonce symbols (0x280e88..), so their field offsets are exact.
- vptr offsets (from constructor stores): most common +0x34 (78 classes, the EmyOut family), +0x20,
  +0x00 (40), +0x14, +0x04, +0x200 (the EmyMain family), +0x94 (Dog*).
- GamMan has 181 inline methods (the game manager; `game/gamman.cpp`), S2dMes 47, DogModel 44, EmyMain 44,
  DogActor 43, ObjStat 42.

## Key functions and globals

| address | what | evidence |
|---|---|---|
| 0x100008 | `_start` (crt0) | libmatch (FF2 crt0.s) |
| 0x2045a8 | `main` (`mainGame` in src/main.cpp): see Program flow | crt0's call; build-date strings |
| 0x240eb8 | `rxSysInit`: console bring-up (see Program flow) | strings; `rx/rxsys.cpp` |
| 0x241f58 | `rxSysPrintf`: the middleware's and game's printf (151 callers): print hook 0x2db9d0, then `scePrintf`, or a buffered host tty (file 0x2dba34 / IOP tty RPC) | `rx/rxsys.cpp` |
| 0x2dbe8c | `g_rxSysMemFunc[4]`: memory hooks the game's allocators call through (0x1830fc, 0x182818 ...) | `rx/rxsys.cpp` |
| 0x215a30 | the assert handler `(file, line, expr)`: prints "Assertion Failed: %s(%d)   %s\n" through 0x260110 and loops for ever | 1,483 call sites |
| 0x105ba4 | `operator delete` (`utlOperatorDelete` in src): `if (p) utlMemFree(p)`; the deleting destructors tail-jump to it. `operator new` 0x105b54, `new[]` 0x105b7c, `delete[]` 0x105bc4 | 277 call sites; `DogEntity::~DogEntity` 0x280f34; `utl/utlmisc.cpp` |
| 0x6e9800 | the game heap: a `BedMemory` (bed/bedmemory.cpp 0x100354..) that `utlMemInit` 0x105be4 (called by main) creates over the largest malloc (probe from 0x1755100 down); `utlMemAlloc` 0x105ca0 / `utlMemRealloc` 0x105de0 / `utlMemFree` 0x105e08 are main's rxSys memory hooks | `utl/utlmisc.cpp` |
| 0x2155f4 | a second assert handler, same `(file, line, expr)` and message but through `scePrintf` (27 call sites, e.g. snd/sndman.cpp); also `ROR_ASSERT` in src | sndman |
| 0x215a30 | (see above) the assert handler; src writes asserts as `ROR_ASSERT(expr)` (common/ror_assert.h: reports and continues) | |
| 0x267798 | `_InitSys` | libmatch (FF2 initsys.c) |
| 0x6e9840 | `MagSystem` (mag/magsystem.cpp): frame, V-blank semaphore `m_v_sema` (sdata 0x29619c), scene member at +0x12540 (0x6fbd80) | asserts; main |
| 0x7812c0 | `GamMan` (game/gamman.cpp 0x1c05c0..0x1d3b44, `partial`; EE 0x22ab8, vptr +0x22ab4): common data and stage loads, per-frame game update. Fields are named from the ELF's ~180 inline accessors (sections.py --list GamMan; mipsdis shows each one's offset): +0x21f40 getCurStat, +0x21f48 getSysFlg, +0x21f4c chkSysFlg2, +0x21f50 getChapter (the stage), +0x21f60 getVCount, +0x21fdc / e8 / ec the PC / dog / murderer chr numbers, +0x22020 / 30 / 40 PC pos / rot, dog pos, +0x22090.. the three ObjStat pointers, +0x222a0..+0x222c0 the task ids, +0x22a80 getEvtPack. Members: GamArea +0x40, GamSndMan +0x18e00, the item vector +0x1a6c0 (12 slots) and nine node pools of sequence.h's template (item stock +0x1a7c0 x0x14e, Achr / Fchr / VFchr / AVFchr lists +0x1d340 / +0x1d980 / +0x1dcc0 / +0x1ef00). loadSystemData reads the seqMakeGlobalPath(1) pack whole (file 0 GamSysData, 1 / 2 SE packs 0000.se / 6400.se, 3 registSnd, 4 the system script image, 5 registStrSnd); loadStage reads data/SEQ/W%03d/W000.WRD (header <= 3 sectors, then whole: 0 the stage script image, 1 the object table GamObjTable (version +0x1c: 1 = 0x50-byte records, else 0x30), 2 -> +0x1a680, 4 -> the event pack) and sets the camera to (0, 1000, 0), rot x -pi/2. **Objects** (2026-10-25): an oid (OID_STATE) is list << 28 (0 vfchr, 1 achr, 2 fchr) | type << 23 | slot (0..0x1ff); achr pointers by slot at +0x21340 (0x80), fchr at +0x21540 (0x280, slots 1..0x27f), vfchr found by walking +0x1dcc0's list (getFchr tries it first for type 5); `isObj` 0x1d046c, `getActorNo` 0x1cbe38 (ObjStat +0x108). The object table's 0x50-byte records (GamObjRec) are read / written through `ctlObjParam` 0x1c9920 (20 ops, table 0x2fbba0). **Stage change**: the scripts set flags2 bit 2 (and +0x2221c the kind: 3 the stage-2 images, 5 a full reset 0x1cdbf0); `afterSeqExec` 0x1cd25c (end of Sequence::exec) runs it and `execStageReq` 0x1cd7c8 loads by getCurStat (1 loadStage(getChapter)) and copies +0x21f7c..+0x21f88 into the sequence globals [1..4]. pause / resume 0x1c6a40 / 0x1c65e8 clear / set running bits in getSysFlg by mask (0x100000 menus, 0x100004 PC and actor tasks, 0x100104 the dog, 0x80000 Brown's controller, 0x200000 the dog's hide) **Items** (slice 3): a slot / stock record is GamItemSlot {GamSysData record, oid, short id (0xa00 none), u_short count}; the system data record's flag 0x20000 marks a stacking item (count 1..99); addItem 0x1c7e40 also sets the sequence bit lookupFlagTable(2, id, 3) (table data 0x2fc1f8, 8-word rows) and GamSysData flag 4; the stock is a 0x14e-node pool (used list +0x1a7f0). New oids come from `makeOid` 0x1c6fe0: image << 30 | type << 23 | (+0x21f7c & 0x7f) << 16 | a number counted down from init's table +0x225f4 (types 2 / 0,1,3 / other / 10..15 each their counter). **Stage SE** `loadStageSnd` 0x1c2f50: by chapter one 1xxx.se pair (1000/1100, 1001/1102, 1001/1101, 1001/1103, 1002/1104) and 3200..3210.se from data/snd/xv/, loaded under the file name (boot stage 99 loads none); a held item's pack 640N.se by `loadItemSe` 0x1c772c. | vtable; main |
| 0x781300 | `GamArea` (game/gamtrg.cpp 0x1dd1e8..0x1e0df0, `verify`: all written; class name invented, file from cloneTrg's allocDebug tag 0x2fd630): GamMan +0x40, EE 0x18b60. Three inline node pools of 0xb0-byte `GamAreaTrg` records (0xc0 nodes): +0x0 the listed triggers (0x100; addTrg, hasId), +0xc1c0 the active ones (0x100, used +0xc290; activateTrg 0x1de104 copies a listed one with flag 0x1000000 in, ordered by the u_short +0x38, resolving +0x44 / +0x48 oids to actor numbers +0x4c / +0x50 and the ftb key +0x24 to a pointer +0x54), +0x18380 update's up-to-8 starts of a frame; +0x18b40 a heap pool of 0x40 registFtb entries (0x30: key, 9 values), +0x18b58 the active list's last / update's current node, +0x18b5c update's count. A trigger: rect x0 z0 x1 z1 +0x0, flags +0x20 (0x1000000 activate, 0x4000001 checkPc, 4 timer +0x40 = frame count + +0x28, 0x20 ftb, 0x82c00 / 0x5000 objects, 0x20000000 fired, pause 0x8000), id +0x2c (an oid, makeOid(0, 0xd, image)), its "trg" thread id +0x30 / entry +0x34 (execThread(id, entry, +0x44, 2, 0, id, "trg")). isInPolygon 0x1de8ac is a crossing test over (x, z) point pointers (also called by 0x1d4b9c). **update** 0x1dee78 (every frame, GamMan flags bit 2; pos / rot the PC's): each active trigger's condition bits are the low 0xc31fffff of its flags, collected from 0x3000000 and compared whole: 1 / 2 enter / leave the rect, 0x100 rect and |y - +0x14| <= 200, 0x1000 actor +0x4c within the radius +0x3c of the PC (xz, |dy| < 150), 0x400 / 0x800 / 0x80000 the two actors near / (0x80800) near or apart, 8 rot y within +0x1c of the angle +0x18, 0x2000 / 0x4000 the direction actor->actor2 / PC->actor, 0x10 canCheckPc (Cross), 4 the timer, 0x20 the ftb entry's script bits (negative = must be clear), 0x8000 m_global[+0x5c] op +0x60 against +0x58, 0x10000 script bit +0x54, 0x40 / 0x80 GamMan flags 0x2000 / flags2 0x80 with m_global[0x11] / [0x29] == u_short +0x64, 0x200 an fchr of that kind near (released), 0x40000 the polygon rect corners + (+0x80, +0x88); 0x400000 adds Brown's conditions from +0x70 (0x40000 found oid, 0x100000 / 0x200000 magBwnReturn0b / 0c == +0x68 / +0x6c, 4 the height). 0x80000000 is the edge bit (0x10000000 / 0x800000 set when fired, cleared on the other side); GamMan flags2 0x8000000 holds most tests off; 0x40000000 a one-shot (leaves the active list); the held angle goes to m_global[8] for a flag-1 trigger (sdata 0x2d8f64). The fired list then starts the trg threads unless a menu is open (0x4000000 ones not twice while two rects hold; no 0x8000000: set 0x20000000, GamMan::resume(0x8000), TskFrameFunc 0x16 after 16 frames, flags2 |= 0x400800, and stop the walk). | GamMan member |
| 0x7a3d80 | `CmnTaskMan`: task list, its `SysTimer` at +0x40 (EE 0x6c0 bytes; task/tsk_man.cpp) | main; 66 addTask call sites |
| 0x7a4440 | `Sequence` (sequence/sequence.cpp 0x20e3d8..0x20ff34, the VM sequence/seqvm.cpp 0x20ff34..0x2155a0): event sequences ("mst", "trg", "imdy", "single"). The scripts are **MIPS code** the VM 0x214c24 interprets: registers at bss 0x7b4700 (32 words, v0 0x7b4708, a0 0x7b4710), opcode tables data 0x2d9958 (primary) / 0x2d9a58 (SPECIAL), loads / stores on raw addresses; a jal below the image calls native function (target - 0x144) / 0x40 of the table data 0x2d90f8 (sequence/seqcall.cpp, 512 entries, 471 empty), jal targets >= 0x8160 are image offsets (base 0x7b47d0). Native returns 0xcccccccc rerun the call next frame, ..cd continue next frame, ..ce jump (Sequence::jumpTo 0x2113a4 sets the target 0x7b47b0), ..ca end the thread, ..c9 return to 0x7b4db0 + 4. Members: +0x0 images [4] (0 system G000.GRB 0x20000, 1 stage W.WRD 0x18000, 2 a second script 0x30000, [3] 0; game heap), +0x10 per-image thread pools [4] (new'd, 0x40 nodes of 0xc0), +0x40 the start list (startThread 0x211e84 queues, exec moves into the image pools), +0x6200 the ended list (runImage copies threads whose seqVmRun returned pc 0; reapThreads 0x20f4fc frees them next frame), +0xc3c0 a 0x100-entry pool of 0x10-byte entries; each pool = free list +0x0 / used list +0xd0 (sentinel node + count after it) / max +0x1a0 / nodes +0x1c0 (entries: +0x0 / +0x1c / +0x38 / +0x40), nodes {prev, next, data at +0x10}. Thread record 0xb0: flags +0 (0x40 pause 0x8000, 0x80 resume slot +0xc), id +4 (top two bits = image; main threads 0x04000000 / 0x44000000 / 0x84000000), pc +8, GamArea trigger id +0x20, arg +0x28, VM stack +0xac. +0xdc08 image bases [5] (index = current image 0x7b4dd0), +0xdc1c the global variables (0x800 bytes), +0xdc20 a bitset (0x800), +0xdc24 flags (1 = exec skips). VM state bss: 0x7b4640 / 0x7b4650 running node / end, 0x7b47d0 image + 0x30, 0x7b4910 globals, 0x7b4920 call save slots (0xcc), 0x7b4d20 stacks 0x10000 (32 x 2 KB, use words 0x7b4d30), data 0x2d9e08 image saves 3 x 0x780 | main |
| 0x7b205c / 0x7b2060 | `g_sequence.m_global` / `m_bits` (Sequence 0x7a4440 + 0xdc1c / + 0xdc20, set by Sequence::init): GamMan, MagScene and the menus read the script globals ([1..4] stage-change words, [5] / [7] cleared by a stage load) and the flag bitset (word 0 bit 0x8000 chapter-6 start, word 1 bits 2 / 4 the dog follows, 0x100; word 2 bit 0x8000 the debug restart that restores the PC / dog) through these addresses | sequence |
| 0x7b2580 | `SndMan` (`snd/sndman.cpp` 0x17f0d0..0x1810f4, `verify`; class name from the file): sound manager over rxSnd. 65 XVAB bank handles (+0x2c) and their source buffers (+0x130), 48 slots `SndInfo` of 0x84 bytes at +0x234 (type 1 ADPCM stream ids 0..1, type 8 SE ids 2..47; id bits at +0x24 / +0x28), master-volume fade +0x1af8..+0x1b10, TskSnd id +0x1b14; +0x4 holds argv[0]. Requests set a slot state (0 start, 2 play, 3 set, 4/5 pause, 6 resume, 7 stop, 10 reopen) that `SndMan::update` (each frame: slots, RxMpeg::update, rxSndUpdate, fade) acts on. Streams read `data/snd/xws/<name>` through an rx file ring (bss 0x668980 / 0x6a8d80, 0x40000 each; a looping stream has an end marker jumping to 0) into `rxSndAdpcmWrite`, 48000 Hz. Sound ids go through `GamSndMan` (GamMan +0x18e00 = 0x79a0c0, `game/gamsnd.h`: 0x38-byte entries {name[0x1c], id, bank, prog, note}) and `utlConvSndId` 0x106658 | asserts `strSndInfo->uniPtr.sndFStream.fStream / fd`; main |
| 0x237ce8..0x23ff58 | the rx sound library (`rx/rxsnd.cpp`, pinned, `verify`: every function written except two unreferenced trivia 0x2384b0 (empty) and 0x23a910 (returns its argument)): the EE client of **RXSNDDRV.IRX** (disc MODULES/, RPC 0xd000, client 0x7ca300, bound by `rxSndBindRpc` 0x23f9b0 until `serve` is set; on the host `src/sdk/iop_rxsnd.cpp` serves it, open-items Decisions). Status replies (layouts in rxsnd.h): BGM 0x104 {id, status}[32] + value[32] + count, SE 0x104 {id, status}[64] + count, PCM 0x184 {id, status, s16, posHi, posLo}[32] + count, ADPCM 0x304 {id, status, free, posHi, posLo}[48] + count; status bits 2 playing, 4 ended, 8 paused. Hidden SPU hook setters 0x239048 / 0x239068 / 0x239088 created. Transport: `rxSndRpcCommand` 0x23fc18 (one direct call, fno = command, send 0x7c9300 / reply 0x7c9b00, double-banked by `rxSndGetBufferBank` 0x2db638, which the rxTh yield callback 0x237ce8 sets) and a command queue 0x7ca340 (0x1000; headers {cmd, replySize, dataSize}, replies from 0x7cb340+4) flushed as fno 0x200 by `rxSndQueueFlush` 0x23feb8; each call WaitSema's 0x2db9c0, `rxSndRpcEndFunc` 0x23fac8 signals it. `rxSndUpdate`: flush, queue the status queries 0x72 (BGM) / 0x83 (SE) / 0xb3 (PCM) / 0xa3 (ADPCM), flush, apply them. Command families: 0x2x IOP heap, 0x4x XVAB, 0x7x BGM (32 slots 0x18 at 0x7bbbc0), 0x8x SE (64 voices 0x14 at 0x7bbed0), 0xbx PCM (32 x 0x3c at 0x7bd804, position callbacks 0x7bc400), 0xax ADPCM (48 x 0x50 at 0x7c03c8, callbacks 0x7befc0). Volumes: master 0x7b5ac4 x family master x voice >> 14. Heap hooks 0x2db95c..0x2db970 (IOP alloc / alloc mode 1 / free, SPU alloc / alloc mode 1 / free), SPU heap 0x5010..+0x1baff0: block nodes 0x7b6b80 on a circular list (m_prev up the addresses, m_next down; a block's address is the running sum of sizes), first fit in the bank's slot (`RxSndSpuSlot` 0x7b83a0 or the whole region 0x7b97a0), mode 0 lowest / 1 highest / 2 at an address. `rxSndIopWrite` 0x2387b8 is the EE -> IOP copy (SIF DMA when 16-aligned, else bounce buffers 0x7b6b40 + IOP command 0x23). XVAB / XSEQ files: {magic, version, size} then chunks {tag, size} (version >= 0x200: 16-byte header and chunks {tag, ?, size, ?}); samples are the HEAD chunk's (byte 1 set: after the 16-aligned file) or SMPL's, sent to SPU by command 0x40 (type 3 EE -> SPU; types 1 / 2 / 4 / 5 have their own unreferenced senders). More commands: 0x22 {1 / 2} IOP heap (unknown), 0x30 {kind, value} system (0 debug flags, 1 frame rate 50 / 60, 2 output mode, 3..5 unknown), queued 0x31 {id, 1, L, R}, queued 0x32 {core 0..1, attr, value} the SPU2 effect (0 mode 0..9 as libsd reverb modes, 2 depth L / R, 3 delay, 4 feedback; `rxSndSetEffectMode` / `Depth` 0x23a080 / 0x23a0b8 are what SndMan calls), 0x60 / 0x61 / 0x63 XSEQ open / free / set (table 0x7bafc0, 256 x 0xc {flags, iop, size}), 0x70 {slot, xseq} BGM play, 0x73 / 0x74 {u8 slot, u8 channel, 3, L, R} BGM channel volumes, 0x90 / 0x92 / 0x93 read and 0x91 sets 48-word per-voice tables. `rxSndCentToPitch` 0x238408 uses two float tables 0x2db640 (2^(n/12), n -48..48) and 0x2db7c8 (2^(c/1200), c 0..99). `rxSndIopRead` 0x2389c8 mirrors IopWrite with sceSifGetOtherData and the EE bounce buffers 0x7b6b00 (bank * 0x20). ADPCM streams: per channel an IOP ring (m_bufSize) and SPU buffer (m_spuSize); writes stage 16-byte blocks in 0x7c1300 / 0x7c5300, clear their flags and set flags 3 on the last block before each SPU-buffer wrap, then announce whole 0x40 units with 0xa1; `rxSndAdpcmEnd` pads with the work buffer and writes the end block 0x2db980. PCM streams (`rxSndPcmOpen` 0x23cca8, reached only through the sound driver wrapper 0x18182c): an IOP ring of blocks (mode 0: 0x800, modes 1..3: 0x1000; mode 1 keeps left halves in the first blocks * 0x800 bytes, right after), opened by 0xb0 {u8 id, u8 mode, iop, blocks}; `rxSndPcmWrite` 0x23d078 copies into the ring and announces whole blocks with 0xb1 {u8 id, u8 count, seq[count]} (running sequence numbers); 0xb2 {id, param, value} sets 0 close, 1 pause, 2 attr, 4 volume, 5 end; position callbacks are (block seq, offset). A table at 0x298f68 pairs the PCM and ADPCM open / close / get-free / write / pause functions (PCM first) | `rxsnd :` strings; SndMan |
| 0x269fe0.. | libdbc (`sceDbcInit` / `sceDbcEnd`) and libpad2 (`scePad2Init` 0x26b1d8 / `scePad2End`) between libipu and libpad: named by reading; `sceMcEnd` is 0x24d448 (libmatch had sceMcChangeThreadPriority) | strings; main |
| 0x100248 | `_exit`: crt0's `j Exit` thunk; `Exit` is 0x267b98 | FF2 symbols at the same addresses |

## Game objects (GamMan, game/gamman.cpp, 2026-10-26)

- An object is an oid (list bits 28..29: 0 vfchr, 1 achr, 2 fchr; type bits 23..27; slot 0..8) and an
  ObjStat-derived record: `ACHRstat` (EE 0x480, vtable 0x2e4810, `m_achr[slot]` +0x21340 and the achr
  list +0x1d340), `FCHRstat` (EE 0x2c0, vtable 0x2e47f8, `m_fchr[slot]` +0x21540 and the fchr list
  +0x1d980) or a vfchr: an FCHRstat built in place in the 0x180-slot storage +0x1f248 (slot oids
  +0x1f24c), on the vfchr list +0x1dcc0 and, when its room (param 8) is the loaded one, the avfchr list
  +0x1ef00. vfchr records are never deleted.
- Construction: `ObjStat::ObjStat(a1, oid, actor, kind)` 0x1d87f4 (8-node pool +0x40, oid +0x100,
  actor +0x108, kind +0x128), the class's inline constructor (achr: task ids +0x3d4..+0x3e8 = -1), then
  `ACHRstat::init` 0x1d9908 + `initParamFuncs` 0x1d9b2c or `FCHRstat::init` 0x1db1f0 (system data record
  by kind), `setPos` 0x1d89c4 / 0x1db7cc. Type-2 achr objects (enemies) also get
  `EmyManager::addEmy` 0x162c2c with the enemy type (0..10) from system data field 0x72.
- Models: `createObjByModel` 0x1ca8e8 (`MagSystem::loadModel`, which returns the actor number) and
  `createObjByName` 0x1ca72c (`MagScene::loadModelByName` from the archive of group kind >> 10:
  table 0x2fbed0 -> data/ch/action/ACTION.RPK, target/TRG, fix/FIX, item/ITEM, inst/INST,
  enemy/ENEMY, door/DOOR, (7 none), pc/PC, bwn/BWN, 01/CHAP01 .. 12/CHAP12.RPK). Model 0x800 and the
  model named "target" get scale 9.9999994e-8 (hidden markers). Light id: 1 for types 0..3 / 5, 0 for
  4 / 9 (`setObjLight` 0x1ca598).
- Markers: `ctlObj1cb8e4` 0x1cb8e4 -> `addObjMarker` 0x1cb970: an Obj176a3c effect 0x32 {x, y, z, 50}
  bound to the oid in the pool +0x20b00 (released by ctlObj1cbb80).
- **game/gamman2.cpp** (pinned 0x1d3b44..0x1d84c8, 2026-10-27; declared in game/gamman.h): GamMan's
  methods past switchVolume. The **object update list** +0x2246c (a 0x100-node pool, record {oid, cell}):
  queueObjUpdate 0x1d5bc8 moves an oid to its tail, relinkObj 0x1d5da0 takes it off; the hit walk
  0x1d7c20 iterates it and calls a GCC 2 pointer-to-member at +0x225d0 ({0, -1, fn}: 0x1d4b9c or
  0x1d50a8, data 0x2fc1d8) with the query position +0x225b0 and radius +0x225a0 (testHitAt / placeAchr
  set them). The **flag table** data 0x2fc1f8 (15 rows of 8 words, -1 row ends): col 0 the 0x2000xx /
  0x1000xx book id, 1 the MnuPBook / MnuDiary type (0x1038 = pb010200, the opening book), 2 its item id
  (0xa17..), 3 the item's sequence bit (addItem), 4 / 7 sequence globals the book reads, 6 a sequence
  bit; lookupFlagTable 0x1d83d0 (keyCol 99 = row number, keyCol 5 always -1). The **secta world** is
  MagScene +0x8542c (bss 0x7811ac, Obj17c0f0): a flag word, a buffer and four lists of 0x18 bytes at
  +0x08 (MagSectaList: +0x4 loaded count, +0x8 the 0x80-byte sector records, +0xc the 0x90-byte cells
  GamSecta); 0x17c340 answers one cell per list (radius 40, findSectaCell 200), GamMan +0x22ab0 the
  list the last query used; cell heights 0x17dd5c, paths 0x17d298 (request {list, from[4], to[4],
  path}). **The hit tests** (written 2026-10-27): hitTestObjs 0x1d7c20 skips the caller's oid, a second
  oid and type-1 objects while getBwnState is 0xd; m_hitFunc is hitAtPoint 0x1d4b9c (testHitAt,
  placeAchr, testAchrHit 0x1d5610, testPcHitAt 0x1d56f4) or hitAtSweep 0x1d50a8 (set only by
  setupDogSweep 0x1d58d8, which has no callers). The object's kind is param 0x18's word 0: 1 its corner
  quad (param 0x1003, GamArea::isInPolygon, depth 10), 2 / 3 circles (2 from its Bip01 Spine1, 3 also
  round its Bip01 Neck: result +0x18 = 1), 4 a test that can never pass, 5 / 6 never hit. The result
  record (ChrHitRec, 0x20) gets +0x10 oid, +0x14 depth, +0x18 the far flag; a type-2 achr hit gets a
  TskAvoidMove(8, 30) on its m_taskId and the record's oid becomes the tester's. **checkMenuReq**
  0x1d3b44: the switch on global 0x15 (jump table 0x2fc100, 11 entries) has only case 3; global
  0x11's low half is the requested item (0xa17..0xa19, 0xa1b..0xa21, 0xab6, 0xacc..0xace open the
  book of lookupFlagTable(2, item, 0); 0xad0..0xad2 the map 0x10001 when the map area is 0 / 2 / 1);
  m_22a84 is the open menu (2 book, 1 map, -1 none). GamMan +0x21ffc / +0x22000 are motion speeds
  restoreAfterMenu 0x1d4370 puts back (DogActor +0x1e8 = m_motion.m_speed). bss 0x7a3208 / 0x7a320c
  are g_gamMan.m_flags / m_flags2, 0x7b2064 is Sequence::m_flags. **The save image** (MnuSvld's 0x3000
  bytes; writeSaveData 0x1d6078 / readSaveData 0x1d68a0, written 2026-10-27): header "FEB_SAVE", +0x8
  g_versionNumber, +0x10 chapter, +0x14 inuMode, +0x18 0x2d10, +0x1c play minutes, +0x20 / +0x24 equip /
  find-target item, +0x28 version 0x10 (reader: nothing below 7; older versions have shorter sections);
  +0x40 saveGlobals, saveBits, 8 bytes, PC and dog ObjStat::save (0xb0 each); then from that point:
  +0x0 12 item words (id | num << 16), +0x30 stock count, +0x38 stock words, +0x56c 'N', +0x57c
  m_1f240 (0x169 words), +0xb20 object resets (6-byte {oid, flags, m_0c} from the object table ->
  +0x18bc0), +0xee8 0x26 vfchrs of image 1 (0x20 each: oid, kind, flags, param 8, 0x1000, 0x43 pos),
  +0x13a8 every vfchr's {oid, flags} (-> +0x18bc4), +0x1fa8 the 12 option words +0x21f8c.., +0x1fd8 the
  GamSndMan get_snd_prm BGM record (-> GamMan +0x22a5c: its writer), +0x1ffc 0x1c7 item records' flag
  bytes (& 0xd5; the reader loads them signed). Sequence bits word 8 bit 0x1000 turns the read into a
  New Game reset. findSectaCellFrom 0x1d7fb8 (no callers) searches a list from a cell via 0x17ded0.
- **game/gamstat.cpp** (pinned 0x1d84c8..0x1dd1e8, 2026-10-26; game/gamstat.h): ObjStat / ACHRstat /
  FCHRstat code and GamSysData. ObjStat's fields are named by the ELF's accessor inlines 0x28f874..
  0x28fdb8 (ror-symbol): +0x100 o_oid, +0x104 o_stat, +0x108 o_chr_id (actor), +0x114 the secta cell
  (o_secta_id = cell +0), +0x118 o_smell_range, +0x11c o_scale, +0x126 a_lp, +0x128 o_uid (the system
  data id), +0x12a.. o_smell_uid_1..3, +0x130 o_area_no, +0x138 o_inu_func, +0x150 the collision words
  (getColPrm, 0x30 bytes; +0x220 init's copy, getColPrmKeep), +0x190 pos, +0x1a0 rot, +0x1b0 work[16].
  **One param id space** (gamstat.h GamParam): 1..0x21 are the accessors in ACHRstat::initParamFuncs'
  table of GCC 2 method pointers at +0x280 (getParam dispatches through it), 0x22.. per class, 0x1000..
  0x1005 ObjStat's (base getParam 0x1d9044 / setParam 0x1d932c). GamSysData::getField 0x1dc48c uses
  the same ids for the record fields: 0xb / 0xc the names (English +0x20, Japanese +0), 0xf the model
  id (+0x46, o_chr_id_low), 0x1c..0x21 the stats at +0x80 of a character record (max_lp, st, def, avoid,
  agi, neck_rot_range in degrees: field 0x21 returns radians). GamSysData: ten tables (record id =
  table << 9 | record): 0..3 characters (records 0xa0, 0xa0, 0xc0, 0xe0; flags +0x90, table 2 a second
  word +0x94), 4..9 objects (0xc0, table 5 0x200; flags +0xb0); FCHRstat::init reads only table 5.
  An achr's move params (read by TskMoveTypeGen 0x1e9a3c): 0x1100 the walk motion, 0x1103 its speed
  per 60th, 0x1101 / 0x1102 the right / left turn motions (-1 none), 0x1104 the turn speed, 0x1105 the
  angle from straight behind within which a turn plays the turn motion.
  ObjStat::setPos / getPos / setRot were named ACHRstat::* before: the base setter calls them for fchrs
  too (FCHRstat has its own four). The secta cells come from GamMan::findSectaCell 0x1d8064 (stub);
  the floor height from GamMan::getSectaCellY 0x1d817c (100000 = no cell; ObjStat::getPosSectaY
  0x1d8c50 passes a NULL cell when the object already has one, so it gets 100000 -- the ROM's).
  **GamSysData::setField** 0x1dc070 writes few fields (table 1: smell uids, max lp; 2: 0x4f..0x58,
  0x70..0x72; 3: 0x75 / 0x76; 4..8: 0x1d / 0x1e; 5 also 0x31 / 0x42; 8 0x39) and for every table but
  1 copies the smell uids 0x10..0x12 (table 9 also 0x1d / 0x1e) *out* into data: shared read tails.
  GamIdMap 0x1d84c8..0x1d87f4 (invented name): a u_short key / value map, parallel arrays, value 0 free.
  **Map screen helpers** 0x1dbb38..0x1dbef8 (slice 3; the menu 0x1949bc / 0x196878 calls them with
  GamMan +0x18c00, the port passes GamMan): `gamMapConvWorld` 0x1dbb74 / `gamMapConvLocal` 0x1dbc5c turn
  a room position (+ the WRD offset for World) into map pixels by GamMan::getMapArea's row of data
  0x2fc748 (GamMapScale, 3 areas: x = mulX * -x / divX + addX, z = mulZ * z / divZ + addZ);
  `gamGetMapFloor` 0x1dbd90 picks the map page (4 when the room word +0x18dd0 is 4, else 1 with flags2
  0x40000, else 1/2/3 by script bits word 6 bits 0/1/2, else +0x18dd0); `gamGetMapMarkers` 0x1dbdf0
  fills two 0x20-byte GamMapMarker (kind 1 the PC +0x22020/+0x22030, kind 2 Brown via gamGetBwnPos /
  gamGetBwnRot 0x144418 / 0x14448c). ACHRstat +0x3d0 is a pointer whose +0xc is an actor no
  (ChrMotionRef, writer not found; read by isMotionTaskEnd 0x1d9f40 through chrMotionRefEnd 0x1e8f88).
- **Map units** (gamman session 5): `GamMan::loadMapUnit` 0x1ca0dc (native 027) reads the first
  0x1800 bytes of data/bg/mapunit.bin (an RTPK, one file per unit, named without extension), then
  the unit's file into a keep buffer (+0x225e8), `loadMapPack` 0x1ca38c checks the RTPK magic,
  `MagSystem::loadScene`s it into a sky slot and records {mode << 30, unit index, slot} in the pool
  +0x1d240 (GamTenkyuRec). Modes 2 / 3 pair two units: `showMapUnits` 0x1c9e20 shows one kind, hides
  the other (the sky slot's model[0] m_hide) and points m_mapModelNo (+0x1d300) at the shown one.
- **Stage 2** (`initStage2` 0x1c47c4, change kind 3): data/SEQ/W<ch>/A<room>/A<room>.ARA, an RTPK of
  n files: file 0 the actor model ids (-1 ends; every W001 pack has none), the actor files from n-5
  down, n-4 the image-2 script (ELFR, Sequence::loadSubImage), n-2 the room data for gamSetWrdRoom
  0x1dbcf0 (464 bytes). Loaded actors are hidden and recorded in the pool +0x1f880 (GamActorRec).
- **PC start** (`setPcStartPos` 0x1c3d38) by +0x1a684: 0xcccccccc none, 0xcccccccb the kept start
  transforms +0x22060 / +0x22070, -n the n-th record of this room in W000.WRD file 2 (0x20 bytes: room,
  pos +0x10, rotY +0x1c), else a type-9 fchr oid: 50 units in front of it. `storePcTransform`
  0x1cd404 puts the dog 35 units to the PC's side (heading - pi/2) and tells Brown's controller.
- **Stage resets** (gamman session 6): `resetStage` 0x1cdbf0 (change kind 5, a new chapter) unsets the
  room (+0x21f7c = -1), closes the menu, camera mode 1, clears the PC / dog, the object records' 0x8000
  bits, then (script bits word 7 sign clear) the held item, the image oids of the list +0x1f240 and
  bits word 6 bits 0..3; sounds (`GamSndMan::reset` 0x1e1fa4), callbacks +0x222c4.., messages, effects,
  Sequence::stopImages(1), `releaseAllChrs` 0x1ced18 (every achr / fchr deleted; with bits word 2
  0x8000 the PC and dog stay) and the area triggers. `releaseObjs(kind, keepFchrs)` 0x1cdf5c (kinds
  0 / 1 from task/tsk_bg_load.cpp, 2 from releaseStage2) picks by the oid's image (bits 30..31): kind
  2 drops all but image 1, kind 3 only image 3, else all. term 0x1c0d44 = releaseAll + init.
- **Camera stack** (`pushCamera` 0x1c5ffc / `popCamera` 0x1c621c, native 016): three GamCamRec of
  0x30 at +0x22470 (count +0x22530): MagSystem::getCameraMode's mode & 0xf (0..5 only) and number,
  +0x21f78, the scene camera entity's (MagScene +0x2ab70, bss 0x7268f0) trans / rotate xyz and lens
  angle (+0x140), and for map cameras 3..5 the path point (MagScene 0x13d4d0 / 0x13d52c: the path
  record +0x20 int + +0x28 fraction). Pop sets modes 3..5 as setCameraMode(mode, +0x21f78, {-1, -1,
  number}).
- **Private collisions** (gamman session 7, native 151): `addObjColliSegment` 0x1d080c / `Box`
  0x1d091c / `Quad` 0x1d0c64 add walls with `MagScene::addPrivateColli` (p0 at the top y, p1 at top -
  height - 1) and store {kind 5 one wall / 6 four, shape 0 / 4, -height, top y, radius, ids} as param
  0x18 (GamObjColli); box / quad also put the four x / z corners in param 0x1003 and the position. The
  radius word is (sx - sy + sz) / 3 for a box, (z1 - x3 + z3) / 3 for a quad -- the ROM's formula;
  fitObjBounds 0x1d480c redoes it from the corners.
- **Weapon ammo** (gamman session 7): the equipped item 0xa3e takes ammo 0xa42, 0xa43 takes 0xa3f;
  m_selItemNum (+0x1d200) is the loaded count, 6 at most. `findReloadSlot` 0x1c8604 / `reloadEquip`
  0x1c870c move counts from ammo slots into it (emptied slots `clearItemSlot` 0x1c8214).
- **Map area**: the WRD entry's word 0x1c (`GamMan::getMapArea` 0x1c6c38; gamSetWrdRoom keeps it in
  +0x18dc0). `canOpenMapMenu` 0x1c97f0 wants map item 0xad0 in area 0, 0xad1 in 2, 0xad2 in 1, and
  `canOpenItemMenu` 0x1c97c8 (m_flags 4, m_flags2 0x800 clear).
- **Attach** (`attachObj` 0x1d1a5c): MagSystem::setActorParent(actor, parent actor, node index by
  name); an achr parent records the child in m_link (+0x254); `detachObj` 0x1d1bec undoes it.
  `setActorBindMode` 0x1d2428 sets bits 5..6 of MagScene m_actor_flags[actor].
- **Enemy groups** (`addChrGroup` 0x1cc560, native 100 case 8): EmyManager::setGroup 0x162b68 (group
  a < 9 at stride 0x3c), then count achrs built with oid 0: addEmy -> createEmy 0x162c88 makes their
  oids 0x9100006f + n (addEmy passes makeOid 1; the twin addEmyAlt 0x162c5c, unreferenced, 0) and
  stores them with the inline GamMan::set_achr.
- **Object queries** (gamman session 9): `collectAchrs` 0x1d2e2c / `collectFchrs` 0x1d327c count the
  objects of a type (-1 any) whose ObjStat m_flags (+0x120) has a bit of need and none of exclude,
  copying them into a caller's SeqPool when given (the fchr one walks the fchr and avfchr lists);
  `collectAchrsNear` 0x1d2ff0 / `collectFchrsNear` 0x1d35ac add an x / z distance < radius and store
  the distance in ObjStat +0x144 (m_dist). collectFchrsNear is the one the game uses (maglifepc2, the
  emy code 0x15xxxx, GamArea 0x1ddf48, checkMenuReq).
- **Script callbacks** m_seqPair (+0x222c4, four {b, a} pairs): [0] ehon, [1] menu, [2] seq_evt,
  [3] jump_area_act (the ELF's regist_call_back_* / clear_*); `check_event_after_menu` is pair 1 set
  and getSysFlg bit 0x8000 clear.
- GamMan +0x18c00 is a WRD block whose words the port keeps as GamMan members (the gam*Wrd* functions
  take GamMan or nothing); getMapObjPtr returns GamMan on the host.

## Disc and data files

Paths the ELF names (the disc dump has not been looked at yet: `ROSEPAN_DATA`):
- `cdrom0:\DATA\CMNTEX\LOADING.BIN`; `data/bg/mapunit.bin`, `data/%s/w%03d/door_obj.bin`,
  `data/%s/w%03d/jump_pos_tbl.bin`, `data/eff/eff.bin`, `data/snd/xv/g000_tbl.bin`,
  `data/snd/xws/str_tbl.bin`, `data/unq/uniq.bin`, `data/wrd/WRD.BIN`, `fnt/fnt16US.bin`.
- Debug data: `host0:main.ee.elf`, `data/seq/DBGITEM.BIN`, `data/seq/w%03d/DBGFLAG.BIN` / `DBGAREA.BIN` /
  `DBGSTEP.BIN`.
- Movies: 41 `.PSS` names (`CP_LOGO.PSS`, `mov/MV00_OPE.PSS` .. `mov/MV05_*.PSS`, `DEATH_B.PSS`, ...).
- `DATA/CMNTEX/LOADING.BIN` (66,784 bytes) is raw GS data, not TIM2: 256x256 PSMT8 pixels (0x10000,
  uploaded to dbp 0x1a40) then a 16x16 CT32 palette (CSM1 order; dbp 8000), then 0xe0 zero bytes. The
  picture holds four 256x64 "Now Loading" frames with a girl icon; 0x204e98 picks frame n by V = n / 4.
  Reference PNG: decode it as above into work/ref/LOADING/ (the 2026-10-22 main session did it inline).
  `SysNowLoading` (system/sysnowloading.cpp 0x204d38..0x2053f4): init 0x2052ac allocates the image
  (line 304) and packet (305), GIF channel, `sceGsSetDefDBuff(this, CT32, 512, 448, GEQUAL, Z16S,
  clear)` with clear Q 1.0, makePacket(0), clearVram 0x2050e8 (sixteen 256 KB CT32 uploads of zeros:
  all 4 MB of GS memory), loadImage 0x204d38, RCNT0 mode 0x83. draw 0x2051cc (per V-blank, from
  0x1be564 only): uploadImage 0x205078, SyncV, half offset, swap, DMA the packet, frame++, every 8
  frames makePacket((frame / 8) % 4). The sprite: GS (0x8200, 0x8980)..(0x8e00, 0x8c80) = frame
  (288, 376)..(480, 424) of the 512x448 buffer, ST (0, n/4)..(1, (n+1)/4), DECAL, bilinear, ABE 0.
- Textures are TIM2 (`utl/utltm2.cpp`; names such as loading.tm2, option.tm2, texmoji.tm2, map01_tcc.tm2,
  `%s_%02d.tm2`): `tm2png.py` converts them, `--scan` carves them out of archives.

## PC platform layer

Created 2026-10-22 (host task `setup/platform_bootstrap.cpp`) from ReiPan's, renamed ReiPan -> RosePan
throughout (identifiers, env variables, files, shader macros):
- `src/sdk/` -- the PS2 SDK on the host: libgraph (V-blank pacing, `sceGsResetGraph` picks 50/60 Hz),
  libdma / libvif / libvu0 (`vu0_lib.cpp`, `vu0_intrin.h`), libpad (+ `ROSEPAN_AUTOPAD`), libpad2 + libvib
  (`libpad2.cpp`: a socket is a port; port 0 reads through libpad's shim minus its 2-byte header, so
  the scripted smoke pad reaches BedPad; port 1 is never linked; vibration state re-sent each read), libmc,
  libcdvd, libmpeg (+ FFmpeg), libsd / libsdr, sif / sifrpc / sifdev / fileio, eekernel threads and
  semaphores, the host IOP (`iop_host`, `iop_sif`, `iop_libsd`, `iop_voice`, `iop_reverb`) and
  `pc_prefix.h` (force-included EE base types).
- `src/rosepan/` -- `os/` (host main thread vs game thread `RosePanGame`, pacing, time, rand),
  `io/` (log, paths, input, file, movie stream), `gs/` (GS memory and registers), `rendering/`
  (SDL3 GPU renderer: 2D GS primitives, the 3D mesh path, present, screenshots via
  `ROSEPAN_SHOT_DIR` / `ROSEPAN_SHOT_EVERY`; video planes), `rosepan_ui/` (ImGui menus: performance,
  graphics settings, data folder), config (`rosepan.ini` in `%APPDATA%/Mikompilation/RosePan/`),
  memory (emulated EE RAM), profiler.
- `src/common/rosepan_stub.h` -- `ROSEPAN_STUB` / `ROSEPAN_BRIDGE`.
- `src/main.cpp` -- RoR's main file (2026-10-22): the process `main(argc, argv)` opens the log and runs
  `GameMain` on the game thread (`RosePan_HostMain`); GameMain builds argv with argv[0] =
  "cdrom0:\SLUS_214.48;1" plus the host's own arguments (so `-m N` picks the boot stage) and calls
  `mainGame` (the ROM's main). PC deviations there: boot path "host0:" and file device -1 (the boot
  device, HOST) instead of "cdrom0:" / 1; `mainLoop` brackets each frame with
  `RosePan_RendererBeginFrame` / `EndFrame`; MagSystem::update paces the loop on its V-blank
  semaphore (30 fps). The renderer opens the window lazily on the first BeginFrame; a frame
  with no draws presents black.
- **The draw path is native, not GS emulation** (the user's rule, 2026-10-22). Feeding the game's GIF /
  DMA packets to the emulated GS (`rosepan/gs/`, the 2D GS-primitive path) costs too much per frame, so
  RoR's drawing is built so it never needs it. The design, grown TU by TU on the draw path:
  - **Keep RoR's decisions, replace its output.** The game code that decides what is drawn (which
    sprites / models / layers, positions, colours, fades, sort keys, blend and test modes, texture
    choice) is reconstructed as the ROM has it. The last step -- writing GIF tags and GS registers
    into a packet -- becomes a call into a host draw module under `src/rosepan/rendering/` (one per
    RoR subsystem: e.g. `rosepan_draw2d.*` for SprMan / the now-loading sprites / fonts,
    `rosepan_i3d.*` for the i3d layers and meshes), marked `// PC:` with the ROM function it
    replaces. Prefer batches (one call per layer / sprite list / mesh), not one per primitive.
  - **Layers and order stay RoR's.** The i3d context's layers, groups and sort lists decide the
    order; the host module records draws per layer and submits them in that order at the frame end,
    so priorities and transparency sort as on the PS2.
  - **Textures live on the GPU, keyed by their owner.** A texture becomes a GPU texture when RoR
    loads or registers it (DogTexInfo / i3d texture records, SprTexture), cached by that record and
    released with it -- not uploaded into emulated GS VRAM and decoded back per draw. The i3d VRAM
    allocator (`i3d/i3dvram.cpp`) stays written for parity and bookkeeping; its transfers become
    no-ops or cache updates on the host.
  - **Packet builders that only feed the GS** are written only as far as they carry decisions;
    pure packet assembly (GIF tags, register pairs, DMA chaining) is not reconstructed for the
    host path -- say so in a `// PC:` comment with the ROM address, and callcheck explanation.
  - The renderer's existing direct entry points are the starting set: `RosePan_RendererDrawTexturedQuad*`
    / `DrawFontQuad` (2D), `BeginMeshStream` / `AppendMeshTriangle` / `CommitMeshStream` and the
    resident meshes (3D, MioPan's "Renderer architecture": decode a model once, draw slices). Extend
    them (sprite batches, per-layer queues, texture handles instead of `sceGsTex0`) as RoR needs.
  - **`src/rosepan/rendering/rosepan_draw2d.*`** (2026-10-22, first user SysNowLoading): the 2D
    module. `RosePan_Draw2dMakeT8Texture` decodes PSMT8 + CT32 CLUT (CSM1 swizzle, `opaque` for ABE 0)
    into a GPU texture through the renderer's `RosePan_RendererCreateImageTexture` (content-hashed,
    salted, in the texture cache for the session; no release yet); `RosePan_Draw2dSprite` draws a
    `RosePanSprite2d` (frame-buffer pixels + normalised ST, as RoR decided them) through
    `RosePan_RendererDrawImageQuad`, stretching RoR's 512-wide frame buffer to the renderer's 640x448
    frame (the PS2 display stretches 512 to the full width). A draw outside `mainLoop` (the loading
    screen inside the loads) brackets itself with `RosePan_RendererBeginFrame` / `EndFrame`.
  - **`src/rosepan/rendering/rosepan_i3d.*`** (2026-10-22): the i3d module. The 2D writers record host
    primitives by packet address; the chain walk (i3dCtxRunGroup and the state / channel packet
    nodes) replays each linked range, so layer / draw-set / sort-key order is the ROM's; GS state
    (ALPHA, TEST, SCISSOR, XYOFFSET, the TEX packet) is followed as the GS holds it, default draw
    offset (1792, 1824) for the 512x448 frame, x stretched 640/512. Textures: i3dTexRegsSet's objects,
    decoded from main memory on first use. `ROSEPAN_I3D_TEST=1` (MagSystem::update, PC) draws two
    S2DMAN.RPK pictures through i3dTexSetTim2 / a texture table / a layer / a draw set /
    i3dPktTexSpriteUV. Details and gaps: open-items.md Decisions "The native i3d hook".
  - **The mesh path** (`rosepan_i3dmesh.*`, 2026-10-27, setup/i3d_mesh_draw.cpp): an i3r model
    draw is a record like a 2D primitive (`RosePan_I3dRecordModel(at, root, world, nodes, tex,
    n)`, the camera before it `RosePan_I3dRecordCamera`: renderer row-major view + reverse-Z
    projection) and replays through the renderer's mesh stream. It walks i3rDrawModel 0x21fe68's
    tree and unpacks each mesh block into a host VU1 memory as VIF1 would (STCYCL / STMASK /
    STROW / STCOL / UNPACK with mask and fill), then reads the strips from there; the CPU skinning
    of 0x225670 is redone on the host. `ROSEPAN_I3D_MESH_TEST=1` (MagSystem::update) draws
    CHARA.RPK CHAP01's first model (amd.mdl, Amanda) with a turning camera: textured, skinned,
    depth-tested, right side up (progress/ screenshot of 2026-10-27).
  - The emulated GS path remains only as a fallback for something not bridged yet; anything that
    lands there is a known performance debt to list in open-items.md.
- Data root: the disc's root copied file by file (the folder holding `DATA/`); `LooksLikeDataRoot`
  checks `DATA/CMNTEX`. `RosePan_PathResolveData` still carries FF3's "../bin/data" spellings; the RoR
  file TU decides how `cdrom0:\DATA\...` and relative `data/...` names map onto it.
- Removed as FF3-only: `debug/reipan_warp.*` (chapter warps / snapshots), `rendering/reipan_graph3d.*`
  (FF3 SGD model glue; `g_dbg_flatcolor` / `g_dbg_nolighting` now live in the renderer),
  `io/reipan_fileload.*` (IMG_BD.BIN + CMP decoder), the UI's warp window and FF3 light-cone menu, the
  IOP boot's call into FF3's iopsys.irx `start()`. Still FF3-shaped but harmless until a TU reaches
  them: see open-items.md "Platform bootstrap".

## The sequence script VM (sequence/seqvm.cpp, 2026-10-25)

- **Script files**: DATA/SEQ/G000.GRB (system) and W<stage>/W<n>.WRD (stage) are RTPK packs; the
  script image is the file starting "ELFR101" (G000: file 4 of 6, 0x118c0 bytes; the others are
  "UNQ_" tables, two XVAB sound banks, "cursor_m...", "M00"). Image header: +0xc / +0x14 / +0x18
  sizes (+0xc may be negative), +0x10 the gp area's link address, +0x1c, +0x20, +0x24 (link
  addresses 0x41xxxx / 0x45xxxx), code at +0x30 linked at script address 0x8160. Data = image +
  [+0x14]; the global variables / bitset pointers go in the words at data + [+0x18] - 0x20 / - 0x10
  (the scripts read them gp-relative). gp = image + [+0xc] + [+0x14] + [+0x18] + 0x8000, sp = gp +
  0x780: the 0x780 bytes above gp are the stack, saved per thread.
- **Decoding** (seqVmRun 0x214c24): word 0x0c102031 ends the thread; opcode 0 -> SPECIAL table data
  0x2d9a58 (0x2c entries, funct 0x2c.. falls into the COP1 table), else the primary table data
  0x2d9958 with (rs, rt, imm16, word); COP1 0x210d3c dispatches fmt S / W through data 0x2d9b08 with
  (fs, ft, fd, fmt), handles mfc1 / mtc1 / bc1f / bc1t and 0x45800000 (end). Registers bss 0x7b4700
  (32), FPU 0x7b4680, pc 0x7b4780, hi / lo 0x7b4790 / 0x7b47a0, the taken jump 0x7b47b0, the delayed
  branch 0x7b47c0 (| 0x80000000, moved to 0x7b47b0 after the delay slot), per-image state bss
  0x7b4850 (3 x 0x30), per-image registers data 0x2d9c08 (3 x 0x80).
- **Script semantics** (the compiler's, not MIPS): all loads / stores are words (lb / lbu offset * 4,
  lwl / swl / swr odd), lui 0x41 / 0x45 load the image's relocated data base (0x42 / 0x46 + 0x10000),
  jal keeps 16 bits of target (targets < 0x8160 are native calls (t - 0x144) / 0x40), mthi / mtlo read
  rd, mult / multu unsigned, div / divu signed, every MMI word is rd = rs * rt, the unary FPU ops read
  ft, a branch to 0 ends the thread (a thread's first ra is 0, so its entry function's return ends it).
- **Threads**: a record (0xb0) keeps flags, id, pc, call save slot +0xc, name +0x10, wait count +0x1c
  (Sequence::waitFrames 0x211318), area +0x20, arg +0x28, registers +0x2c, a 2 KB stack slot +0xac
  (seqVmAllocStack 0x2149a8: 32 slots in a 0x10000 block, cursor data 0x2d9950). A native's
  SEQ_RET_WAIT / YIELD / JUMP saves the registers and the stack (seqVmSaveThread 0x2147e0) and the run
  returns the pc to continue at; seqVmLoadThread restores them next frame. Sequence::execThread
  0x2113dc runs a thread at once in the record at Sequence +0xdc30 (flag 0x100), around the caller's
  registers; GamArea::checkPc calls it. Thread names: "mst" (startThread), "trg", "imdy", "single".
- **Native 151 0x209d4c, object control** (jump table data 0x300eb0, 58 cases; the first native a New
  Game press reaches): t1 is the oid (nothing unless GamMan::isObj), a0 the operation: 1 / 6 setObjPos,
  2 setObjRot, 3 setObjScale (a1's bits are the float), 5 setObjDir, 10 / 11 set / addObjParam, 12 / 13
  setObj / resetObj, 15..17 GamSysData setField / set- / clearFlags(2) on record t1, 18 / 19 object
  attribute bits, 20 placeObjAt, 25 / 26 setKindFlags on record a1, 30 / 31 object work words (+0x1b0),
  40 bit 1, 41 actor bind bits, 50..54 the shape param 0x18 (GamObjColli: m_shape 0 none / 1 17 / 2 25 /
  3 45 / 4 m_radius), 53 a rotated box -> corners param 0x1003, 55..57 private collisions (segment, box,
  quad). Objects' getters / setters dispatch on the oid's list (bits 28..29: 1 achr, 0 vfchr, 2 fchr).
- **Native 152 0x20a368, object queries** (table data 0x300fa0, 51 cases; nothing unless t1 is live,
  except 0x19 / 0x1a on system record t1): 1 / 2 / 3 the actor's m_trans / m_rotate / m_scale into a1
  (3 indexes the scene actors by t1 itself, a ROM slip), 0x14 getObjParam(t1, a1, a2), 0x15
  testObjFlags 0x1d1720 (ObjStat +0x120), 0x16 testAchrFlag 0x1d17c8 (word bits >> 24 of +0x3ec), 0x19
  getField, 0x1a testFlags / testFlags94 0x1dd194 (table 2 +0x94), 0x1e getObjWork 0x1d217c, 0x28..0x2a
  utlAddPolar 0x1f6b28 from the translation (angle = its y + pi wrapped in double, a ROM slip for the
  rotation?) into a1 with w 1, 0x2b / 0x2c attachObj / detachObj 0x1d1a5c / 0x1d1bec
  (MagSystem::setActorParent to a named node), 0x32 Sequence::hasThreadNamed.
- **Native 155 0x20a7d0, vfchr objects** (table data 0x301090, 24 cases): 1 FCHRstat::setParam 0x1db520
  (getParam's counterpart) on vfchr t1, 2..5 addVFchr unless it exists (4 / 5 from a system record's
  model field 0xf / name field 0xb; when the vfchr's room is the current one, getStatus 5 = +0x21f7c,
  loadVFchrModel 0x1caa14 / loadVFchrModelByName 0x1cabd4 load it; 4 skips oids in the +0x1f240 list
  and rooms no object record names), 10 releaseVFchrs 0x1cea44, 11 releaseVFchr, 20 / 21 / 22 GamMan's
  inlines cnvPosL2W / cnvPosW2L / getWrdOffset (0x28f300..) over gamWrdConvPos 0x1dba3c (the WRD
  table's room converters 0x17ebac / 0x17eb3c with the room +0x21f7c; this = GamMan+0x18c00 unused),
  23 hasVFchrParam(2, a1) 0x1d2d7c.
- **Natives 006 / 106 / 200 and 281's helpers** (session 8, 2026-10-26): 006 messages (jump table data
  0x2ff980, cases 1..0x15): 1 / 2 / 5 GamMan::openMsg 0x1cf854 (restarts the 0x9c0-byte message task on
  m_msgTaskId +0x222a0 with text, arg, m_msgParam, 1, m_msgMode +0x225d8, time 0xb4 or a2; 1 first cuts
  the text at 0x1fe), 3 openSysMsg 0x1cfa5c (openSysMsgWindow into +0x2227c), 4 m_msgMode = a1, 10 / 11
  showMsgSprite 0x1cf6bc (S2dMes sprite 3), 0x14 m_flags2 |= 0x80000 and a TskFrameFunc kind 0x15 on
  m_itemUseTaskId +0x222c0, 0x15 waits for it. 106: motion speed by the scene (1) or
  ACHRstat::setMotionSpeed 0x1db1b8 (2, keeps it at +0x418). 200: achr turns over the TskChrRot task
  (ctor 0x1f0434) on ACHRstat::m_task[1] +0x3d8 -- chrRotStart 0x1f0e90 (angle, oid, frames; 0 stops),
  chrRotTo 0x1f0f9c (heading at speed), chrRotToIn 0x1f10a4 (heading in frames), chrRotFrames 0x1f6b8c
  |angle / (speed * 0.05233)|, GamMan::getObjDirTo 0x1cfb48 (heading between two objects via 0x217f10).
  281 ops 200 / 201 go to seqSectorFlags 0x20de34 (sectorSetFlags 0x17c45c / sectorClearFlags 0x17c4f4
  on the scene's sector.sec object +0x8542c); seqGetIdPair 0x20dea8 reads the 50 signed byte pairs at
  data 0x3015e0.
- **Native 020** (session 12, 2026-10-26): script math, jump table data 0x3004a0 (62 words; Ghidra
  cannot recover it, read the asm). 1..4 take an angle as float bits in a1 and store to a2 (1 / 2 sin /
  cos of degrees in double with pi and 180 from data 0x300598); the rest store to the float or vector
  at a1 from a2, a3, t0: 5 atan2f, 10..13 + - / *, 20 float -> int (trunc), 21 int -> float, 30
  sqrtf(a2 + a3), 50 a2 + a3 turned about Y by the angle at t0, 51..57 VU0 add / sub / normalize / dot
  (into a1's x, from a1 and a2) / cross / length / scale-xyz, 58 copy, 59 build {a2, a3, t0, 0}, 60
  fmodf(x, 0x40c90fda), 61 random unit vector (rand - 0x4e7fffff each axis). Library: sceVu0AddVector
  0x25b878, sceVu0SubVector 0x25b890, newlib wrappers atan2f 0x26cb00, fmodf 0x26cb18, sqrtf 0x26cb48.
- **Native 252** (session 13, 2026-10-26): sound, jump table data 0x301280 (201 words, 67 handlers,
  131 entries return 0); all into GamSndMan (GamMan +0x18e00), args a1 the SE / stream id or a name.
  1 / 2 funcSnd 0xd, 6 / 7 0xe / 0xf (master volumes), 4 / 5 0x11 / 0x12, 42..44 0x1c..0x1e, 47 0x19 with
  atoi of a1's first two characters, 8 setReverb, 3 sets GamSndMan +0 (the 3D centre mode); SEs 10..32
  (10 / 12 / 13 / 14 play only while no movie task runs on GamMan m_movieTaskId), streams 52..92, 48 /
  100 / 101 / 102 the GamMan SE pack swaps 0x1c7500 / 0x1c75bc / 0x1c766c / 0x1c76cc, 200 a stream
  started with flag 0x100 then looped. 25 / 28 / 52 / 68 / 70 / 80 / 90 / 91 return the callee's value,
  81 / 82 wait until / while funcStr(0x16) (a stream plays). **GamSndMan's method names** come from
  GamMan's inline forwarders (ELF symbols 0x28e7a8..0x28ee48: this + 0x18e00, some add a trailing 0 in
  t0 / t2): unpush_snd 0x1e1620, put/del_se_3d_pos(_chr) 0x1e239c / 0x1e2704 / 0x1e2aa0 / 0x1e2c40,
  put/del_str_3d_pos(_chr) 0x1e2db4 / 0x1e3138 / 0x1e34ec / 0x1e3698, loadSePack(char*, int) 0x1e4540,
  getStrNO 0x1e4ab8, setReverb 0x1e4cc4, get_se_oid 0x1e4d04, playSE 0x1e4ebc / 0x1e50c8, playStr
  0x1e5250 / 0x1e5428, set_str/se_3d_snd_prm 0x1e5540 / 0x1e5de4, funcStr 0x1e5660, getStrNoByChan
  0x1e5be0, funcSE 0x1e5e58 (a switch over the 0x30 SE records at +0xd40, 0x28 bytes: 1 / 8 stop, 2 / 3
  / 5 / 0xb setSe, 4 loop, 0x13 / 0x14 / 0x15 a TskSnd(0, arg) over a TskSndParam whose words 3, 8, 9
  it fills (task ids +0x1710 / +0x1714), 0x16 returns 1 when the SE plays), isPlayEnvStr 0x1e6cac.
  GamMan::getWrdNo 0x1c6c28 is the inline read of +0x21f7c (m_wrdNo); loadChapterSe 0x1c2848 reads
  data/snd/xv/32NN.se by chapter.
- **Natives 100 / 212** (session 11, 2026-10-26): 100 actor motions (table data 0x300c90, 33 entries;
  a2 motion or name, a3 p1d8, t0 -> float speed, t1 bit 1 -> flag 0x100). The achr's motion task lives
  on ACHRstat m_task[2] +0x3dc: chrKillMotionTask 0x1f61ec kills it; chrMtnMove 0x1f59b4 / chrMtnMove2
  0x1f5bdc replace it with a TskMtnMove (0xc0, ctor 0x1e7f28, vtable 0x2e4910) / TskMtnMove2 (0x100,
  ctor 0x1e8fc4, vtable 0x2e48d0); chrSetMotion 0x1f6308 is MagScene::changeActorMotion keeping the
  heading; chrChangeMotionBySuffix 0x1f5d90 uses DogActor::getMotionNoBySuffix 0x10aa5c (record word
  +0x14 of findMotionBySuffix 0x10a8b8: the name after its first '_', any case). GamMan::getObjRotY
  0x1d0758 -> DogActor::getRotY 0x10a650. DogActor::getMotionLength 0x10d0e8 and
  MagScene::getActorMotionLength 0x1333e4 return a float (f0; the i3m root's float +0x18 via 0x22e958).
  212 enemies (table 0x3011b0, 51 entries) on g_emyManager: findEmy 0x1636a4 scans 64 enemy pointers at
  +4 (first word = oid); 8 EmyGroup records of 0x3c at +0x148 (case 24 is the inline SetGrpMotion); GamMan::switchEnemies 0x1c5a5c (case 50).
- **Natives 253 / 281** (session 10, 2026-10-26): 253 movies (table data 0x3015b0, 11 cases) on the
  TskMovie task (0x80, ctor 0x1f438c, vtable 0x2e4c50: sound 0x1d, SndMan::stopAll(8), name at +0x10,
  m_state +8 = 100, S2dMovie::playMovie) kept in GamMan::m_movieTaskId +0x222b4: 1 start (yields), 2 kill
  + 5 frames, 3 listed?, 4 wait while gone from getTask but CmnTaskMan::findTask 0x1e79c0 still finds it
  with state <= 100, 5 rxMpegGetFrameCount. 281 scene effects: 13 the focus (mag/magscenefocus.cpp:
  setFocus 0x140b20, startLerp 0x140e60 fills a MagLerp {from, to, frame, frames}, setFocusLerp 0x140d94;
  values +0x853c8[3], lerps +0x853fc[3]); 21 DogActor eye ops 0x10f70c / 73c / 7a0 / 808 on +0x480..0x494
  (setEyeTex 0x10f5e4 finds the model node named "eye"); 38 / 0..49 via seqGetIdPair build a 0x80-byte
  EffParam for Obj176a3c::createEff 0x176cc8 (scene +0x837e0, the Eff* factory, 0x110 entries at +4) and
  GamMan::addObjFx 0x1cba0c enters {oid, no} in pool +0x20b00 (findObjFx 0x1cbafc reads it back); 98
  Eff0::setVisible 0x16aa04 (slot 10, shared by all 43 Eff classes); 100 / 101 ctlObj1cb8e4 / ctlObj1cbb80.
- **Natives 016 / 023 / 026 / 156 / 202 / 211** (session 9, 2026-10-26): 016 the camera (table data
  0x3001c0, 52 cases) on MagScene::m_camera_entity +0x2ab70: 1 / 2 set m_trans / m_rotate (+dirty), 3 / 4
  read them, 5 / 6 DogCamera::setLensScale 0x10fc08 / getLensScale 0x10fbd4 (m_angle = s * m_144 *
  0.901526), 7 MagScene::seekCameraPath 0x13d240 (unless GamMan flags 0x20000), 8 m_21f70 =
  getActorNo(a1) (the actor camera mode 2 follows), 10 / 11 setCameraMode, 20 / 21 GamMan::pushCamera
  0x1c5ffc / popCamera 0x1c621c (a 3-deep stack of 0x30-byte records at +0x22470, count +0x22530), 50 /
  51 a TskCamera (0xc0, ctor 0x1fbc0c) on m_camTaskId. 023 area triggers on GamMan::m_area: 1..5
  GamArea::addTrg 0x1dd5e8 (a 0xb0-byte GamAreaTrg from the script image, flags |= 0x3000000, rect
  corners ordered when flags & 0x103; 2..5 get a fresh oid makeOid(0, 0xd, image -1 / 0 / 1 / 2)), 10 startTrgs 0x1de4e4, 11
  stopTrgs 0x1ddc80, 30 registFtb 0x1de6fc (key -> up to 8 values in the pool *(GamArea +0x18b40)). 026
  a TskFrameFunc kind 8 over a1..t0 on m_frameTaskId (1) or m_selectTaskId (2). 156 ray tests via
  MagScene::castRay(hit, p, q, 0): 1 p = a1, q = a2; 2 p = a1 - 10 y, q = p + (sin, cos)(*a2) * *a3 in
  x / y; 3 from GamMan::getObjPos 0x1d05f4 + 10 y along getObjRot 0x1d06b8's y; returns 0 <= t <= 1,
  copies 0x70 bytes of hit record out. 202 (table 0x301110, 26 cases) GamMan::moveAchr 0x1d2a14 (1 by a
  vector, 2 by the float word a2 along the heading wrapped to -pi..pi in doubles) and TskMoveTypeGen
  (0xc0, ctor 0x1e9a3c, vtable 0x2e4ad0) with mode 0..6 (+8 for achr flags 0x200000), rate 1/60
  (0x3c888888). 211: TskMoveTypeRound 0x1eb63c, TskMovePointPath 0x1ec344, TskMoveSectaFindPath
  0x1ec940, TskMoveSecta 0x1ed998 with script globals [13] kind, [14] rate x 10000, [15] mode shift; op
  800 ACHRstat::killTasks 0x1da07c. Every movement task lives on ACHRstat::m_task[0] +0x3d4, killed first.
- **Natives written 2026-10-26** (session 5): 007 a0 1 GamMan::setMsgParam 0x1cf7d0 (eight words over
  +0x22280, the message window's 10, 10, 0xffff, 0x100, 0x100 ...); 009 a0 1 killMsgTask 0x1cf95c; 012
  lights and fog (table data 0x2ff9e0, 501 entries, 63 cases): three runs over the same (light type,
  which) pairs -- 1 / 0xa.. / 0x34 setLightParam(type, t1, which, a1, 0), 0x46..0x54 the same with
  no = a2, 0x64..0x98 getLightParam -- 0x12c MagSystem::setActorLightID, 0x190 / 0x191 GamMan::pushFog /
  popFog (a three-slot stack of I3dFog at +0x22540, m_color[0] -1 = free), 0x192 get the scene fog
  (+0x2aae0), 0x193 set it or fade it over a2 frames (TskFrameFunc kind 0xc with the stack copy's
  address), 0x194 / 0x195 the near / far pair, 0x3c a TskAttach task (vtable 0x2e4bd0, ctor 0x1f5244:
  oid, its actor, arg, *value), 0xc8 scene +0x2aa84, 0xc9 setLightFlag, 0xca..0xcf GamMan::
  saveLightSets / restoreLightSets 0x1d2494 / 0x1d25e4 (light set 0 <-> 7 under getSysFlg 0x8000000,
  1 <-> 6 under 0x10000000, set 1 only while 0x4000000), 0x1f4 setSceneFlags(a1[0..5], 0); 014 a0 1
  hides / shows the first sky slot's model (m_tenkyu[0].m_model[0].m_hide = a1 == 0); 019 pad 0
  queries (table data 0x300290: m_hold bits, m_repeat bits, m_dir_repeat bits 4..7, the direction past
  0.5, all-of-mask held / pressed, any held, the stick * 128); 028 releases (sky domes kind 2 / 3 / 0,
  releaseChr, GamMan::releasePcMotion 0x1c4f70); 036 Sequence::getThreadArg; 038 object table records
  through ctlObjParam (table data 0x300b00); 039 Brown: 'eqds' toggles flags2 0x4000000, 100 / 0x65
  set / get the dog mode, the rest go to magBwnSeqCommand 0x144ae0 (a command switch on the scene's
  MagLifeBwnCtrl, not written). **GamMan::setLightFlag 0x1d2724 takes (theD, theS)**: marks set theD in
  getSysFlg (0 0x2000000, 1 0x4000000, 6 0x8000000, 7 0x10000000) and copies light set theS over it
  (MagScene::copyLightSet 0x13fb3c, mag/magscenelight.cpp: asserts theD / theS < 8, theD != theS).

## The scene's menus (mag/maglifemenu.cpp, 0x12cf3c..0x12d9c8, 2026-10-25)

- **MagLifeMenuBase** (EE 0x40, abstract: no vtable of its own): +0x30 the scene (the asserts' m_scene;
  host m_menu_scene), +0x34 m_type, +0x38 requestMenu's argument. Slots 9 init(DogTexInfo *), 10 term,
  11 isInit, 12 processPad(BedPad *) (non-zero closes), 6 render(layer); getName (5) is 0 in all.
- **Wrappers**: Item, Map, Stock, Save, Load, Title, Opt, Movie, Mcd, Now, SelLang, Board2D hold a
  menu object at +0x40 and forward to it; init first copies the scene into the object (Mcd's does
  not). The objects: MnuItem 0x1aeacc.., MnuMap (mnuBase) 0x194908.., MnuStock 0x19a650.., MnuSvld
  (save and load, m_save +0x24e8 set by the wrappers' constructors) 0x1a5af4.., MnuOpt 0x1ab598..,
  MnuMovie 0x1adfb0.., MnuTitle 0x1a91f4.. (contains an MnuMcd +0, MnuSvld +0x1400, MnuOpt +0x3b80,
  SprMan +0x4df0), MnuMcd 0x1a134c.., MnuNow 0x1ae520.., MnuBoard2D 0x1ad824.., MnuSelectLanguage
  0x1bdc00... Their EE sizes come from the scene's member spacing (+0x2be60..0x41850).
- **mnuBase** (mnu/mnubase.cpp 0x192ac0..0x1947a4; vtable 0x2e4640, vptr +0x10c, EE 0x110): +0 type,
  +4 flags (bit 0 = inited, mnuBase::isInit; 2, 0x10 = volume lowered, 0x40000), +8 scene, +0xc tex,
  +0x10 -1, +0x14 step (-1 idle), +0x18 file list (MnuFile 0x30-byte rows {.., kind +8, arg +0xc,
  name +0x10}; max +0x1c, count +0x20), +0x24 pack path (0x40), +0x64 the pack's first 0x1000 bytes,
  FileReader +0x68, FileReadReq +0x70, slots: models +0x94 (0x20 each, count +0x98), motions +0x9c
  (0x10, +0xa0), textures +0xa4 (0x10, +0xa8), draw set +0xac, packet +0xb0, camera matrix +0xc0,
  view size at the far plane +0x100 / +0x104, saved view far +0x108. Slots 2 init, 3 term, 4 isInit,
  5 processPad, 6 draw, 7 setDrawSet 0x193140, 8 getType, 9 getFlags, 10 openPack 0x1938a8, 11
  closePack 0x193920. setupWork 0x192ba4 carves one mnuAlloc block; addFile 0x19397c queues a file.
  **Written 2026-10-26 (all 38 functions).** File kinds (row +8 & 0xf): 1 i3r model -> model slot
  {root, work block nodes*0xb2+2 = pose 0x30 + local 0x40 + model 0x40 matrices + u16 node map, size},
  2 i3m motion -> motion slot {root, length*30}, 4 TIM2 -> texture slot {tm2, size, DogTexInfo
  record, draw-set index}; bit 0x10 reads waiting. flags 4 = the head row is being read, 8 = drop it
  when read. **updateLoad 0x193a1c is the loader the books' processPad polls** (finishes the head
  row, pops it, starts the next read at the pack directory's offset). Draw helpers: pushCamera /
  popCamera 0x192e94 / 0x192f3c (GamMan camera stack, scene camera reset, view far), beginDraw
  0x192f68 / endDraw 0x1930f8 (layer set + a VU1 sprite packet: i3dPktBeginVu1Sprite 0x21e5a0,
  i3dPktVu1Color / Matrix / Sprite 0x21e488 / 0x21e428 / 0x21e690, end 0x21e688), useTex 0x193210,
  drawSprite 0x193258 (a 0x34-byte rect: pivot, size, uv +0x10, colour +0x20, angle +0x30; the books fixed the order), drawModel
  0x19368c (i3m pose 0x22e960 -> matrices 0x22f7a0 -> i3rConvertLocalToModel -> utlDrawI3r).
  **The mnu pool**: mnuInitPool 0x1944d0 allocates the 1 MB buffer (sdata 0x29bfb4, BedMemory heap
  0x6e9800; MagScene::initRender): 128 blocks {address, size, flags: 1 valid, 2 in use} at its start,
  the first covering +0x800..; mnuAlloc 0x194588 (best fit, 16-byte granules) / mnuFree 0x194688
  (marks free, no merge) / mnuMergeFree 0x1946e4 (setupWork); mnuTermPool 0x194548. MnuMap, MnuPBook, MnuDiary
  derive mnuBase (mnu/mnumap.cpp 0x1947a4.., mnu/mnupbook.cpp 0x196a9c..0x19a52c; MnuStock
  0x19a52c..0x1a066c is not a mnuBase).
- **MnuPBook** (mnu/mnupbook.cpp, written 2026-10-27): processPad 0x1976b8 polls updateLoad and
  updateSheets 0x197d28 every frame, then steps on m_step (m_10 = last step, draw runs for 2..4):
  0 waits the base files (ehonbg / ehontex01_tcc from mnupbook.rpk) and the fade, closes that pack,
  opens data/mnurpk/mnupbook/<row name>.rpk and queuePages 0x196efc; 1 startTurns 0x19722c
  (pushCamera(512, pi/4), lowerVolume, poses the sheets) and fadeIn(15, 7, 0); 2 openWindows
  0x1973fc (text window 512 x 256 at (0, 32), prompt window 64 x 32 at (246, 410) with sys text
  0x1a, message sprite 4 at (220, 408)); 3 updatePad 0x197a74; 4 waits the fade out; 5 writes
  global 0x11 = flag table column 2, global 0x65 = column 0 (+0x118), clears bit-word 1 bit 26, sets
  bit 25 when GamMan::isSeqPair0Pending 0x1d44e4, clears the column-6 bit, popCamera, term, returns 1.
  **Sheets** (0x20 records at +0x12c, (pages + 1) / 2 of them): flags (1 / 2 texture to apply, 4 / 8
  side unwritten, 0x10 / 0x20 torn: pb_pagebrk), front / back texture slots (2..0x21; 0x20 / 0x21
  are pb_pagedmy00 / 01), motion slot (2n = %s%d_turn_r, 2n + 1 = turn_l; model slot n =
  %s%d_base.i3s), frame. Every loaded motion's m_frames is doubled (half speed). Row mode (+0x0c): 2 =
  only the first global-col-4 pages are written, 1 = sheet 7 torn until that word is set. Pad: hold
  Right (0x20) / Left (0x80) or the left stick past 0.75 turns (10-frame lock +0x124, se 8); Circle
  (trig 0x2000) closes once flag 0x40000 (last page open) is set; se 2 on close. A sheet's texture goes
  in through i3rGetChild52(root item, side) -> i3rGetChild25(.., 0): material data +0x1c = the
  DogTexInfo record, side data +8 (u16) += 1. Draw: ehonbg full screen (uv 0..1 x 0..0.875), turn
  arrows 28 x 16 at (100, 412) / (384, 412) from texture slot 0 rows v 48 / 0 (three cells cycle
  while turning), TEST_1 0x503fd, then the sheet models at rot (-60 deg, 180 deg, 0), pos (25, 8, 220).
- **MnuDiary** (mnu/mnupbook.cpp, written 2026-10-27): MnuPBook's six processPad steps and flag
  bits with its own helpers (queueFiles 0x198b54, startTurns 0x198ca8, openWindows 0x198eb4 which also
  calls setWindowSize, updatePad 0x19946c, updateSheets 0x199714). The open spread (m_curPage +0x124,
  even, = global col 7 & ~1) lies on a fixed **base model** (slot 5, <name>_base.i3s); only the sheets
  next to it are posed, from **five 0x20-byte records** at +0x128 {flags 1 used / 2 shown, -1, -1,
  motion 0 = <name>01_turn_r / 1 = turn_l, frame, sheet} over model slots 0..4 (<name>01_base.i3s in
  slot 0, copyModel'd to 1..4). Sheet n carries pages 2n + 1 / 2n + 2 (dy_page%02d_tcc, texture slots
  3..0x12; +0x8b8 holds the page each slot has, -1 none); a_diary goes to slot 2. updateSheets: a
  turning record's 4th frame shows the sheet it uncovers, end - 4 frees the one it covers, the end
  writes the laid page onto the base model; page textures beyond the first unposed sheet each side
  are freed. Text: header #r#L#T#[50#]50#f00#w001#F20 + sys texts row+4 + page - 1 (skipped when "##")
  and + page, 0x4c8-byte buffer +0x138; setPageText(-1) while turning. Draw as MnuPBook's but the
  diary sits at (21, 8, 220) and draws the base model then the shown records. Slot 7 setDrawSet is
  MnuPBook's.
- **MagLifeMenuPBook** (0x100000 / 0x200000 types): init looks the type up (getTypeValue) and asks
  MnuPBook (+0x50) then MnuDiary (+0x910) setType (page tables 0x29c0f8: 12 picture books {type
  0x1038.., ?, pages, 0/1/2, pack name pb*} / 0x29c368: 3 diaries {type 0x1000 / 0x1024 / 0x102a, ?,
  pages, name dy*}, 0x30-byte rows, -1 ends); the one that knows the page becomes +0x40 and runs
  through mnuBase's virtuals. Both books open data/mnurpk/mnupbook/mnupbook.rpk (ehonbg,
  ehontex01_tcc) then <name>.rpk; init reads GamMan::lookupFlagTable(1, type, 0 / 4 / 6 / 7) (4 and
  7 index the sequence globals, 6 a sequence flag bit). **The New Game run shows a picture book after
  the post-film setup (2026-10-26)**: its processPad is the gate to the first room.
- **MagLifeMenuBoard3D** (0x80000 types): loads model getTypeValue(type) (0x10c5..) as a scene actor,
  saves the scene camera (MagScene::saveCamera 0x13c930 / restoreCamera 0x13ca10), puts the camera at
  (0, -200, 500) rotated (-0.1, pi, 0) and the actor at the origin, fades in 30 frames, closes on pad
  bit 13 after a fade out.
- **Type table** data 0x298318 (267 rows + -1): 0x40001.. -> themselves (2D boards), 0x80001.. ->
  0x10c5.. (3D board models), 0x100001.. / 0x200001.. -> page ids 0x1000.., 0x400001.. -> 0.

## The memory card check (mnu/mnumcd.cpp, 0x1a1244..0x1a4bd8, 2026-10-25)

- **MnuMcd** (EE 0x1400; MnuTitle +0, MagLifeMenuMcd +0x40): open +0, init +4, per-mode yes / no
  cursors +0x08..+0x20 (2 = no prompt), exec state +0x24 (0 open two scene windows 0x200 x 0x100 at
  view (0x3c, 0x82), McdMan::init, MsgManSet::open; 1 McdMan::loadData(0); 4 idle; 2 run the check
  mode +0x2c; 5..7 close and return 1), next state +0x28 (startCheck sets 2), requested mode +0x30,
  mode step +0x34, MagSystem* +0x58, blink counter +0x64, slot +0x74, slot states +0x78[2], "check
  slot 2 too" +0x88 (only init writes it: 0), worker command +0x8c / queued +0x90, McdMan +0xc0 (EE
  0xac0), MsgManSet +0xb80 (2 x 0x3bc), windows +0x1300[2], save file size / buffer / path / dir
  +0x1308 / +0x130c / +0x1310 / +0x1350, m_result +0x1390, flags +0x13ac (1 freeze, 2 hide, 4 font,
  8 blink, 0x10 message, 0x20 choice), message +0x13b0, choice +0x13b4, text mode +0x13b8.
- **Check modes** (startCheck(mode)): 0 checkBoot (MnuTitle::execLogo; ok when either slot is
  unformatted or has our directory / space, else a "continue?" prompt), 1 checkBootDebug (Japanese
  texts drawn straight with MagSystem::drawView, rodata 0x2f88e0..), 2 checkSave, 3 checkLoad
  (hasSaveData: an entry dtNNNNNNNNNN), 4 checkInfo, 5 execSave (format cmd 0xc, mkdir 6, overwrite
  prompt, write 8), 6 execLoad (read 10, then testChecksum: row sums +0x2d20 / column sums +0x2d10 of
  the 0x2d10-byte image). MnuMcd commands -> McdMan commands: 1 getInfo (0), 4 getDir (0x12 on
  /BASLUS-21448/*), 6 -> 0x11, 8 -> 0x13, 10 -> 0x14, 0xc -> 8. Slot states from McdMan's result
  (+0x58): getInfo 1 / 0x15 -> 1 (3 when McdMan +0x84 is 0), 7 -> 4, 8 / 0xe -> 2, else 8; getDir 1 ->
  5 with icon.sys (0x3c4 bytes) else 7, 5 -> 6, 7 -> 4, 0xf -> 7. Messages: msgGetSysText 0x1d..0x2f
  for MnuMcd messages 1..0x17, 0x30..0x39 for the choices (data 0x298fe8, 0x1d4 texts).
- **McdMan** (mcd/mcdman.cpp, 0x187724..0x18bb8c, written 2026-10-25; EE 0xac0): request(n)
  0x187e08 takes command word n (init fills +0 with n + 1) when idle (+0x5c 0; 6 when busy), status
  +0x64 1 busy / 2 done / 3 error, result +0x58 = the index of init's ERR_ name (+0x680: 1 NoErr ..
  0x15 ChangedCard). update 0x187c4c dispatches commands 1..0x16 (0x187e44..0x18b2bc): 1..0x11 are one
  libmc call each (GetInfo, GetSlotMax, GetEntSpace, Chdir, Close, Delete with '*' over the dir table,
  DeleteNow, Flush, Format, GetDir (19 entries), Mkdir, Open, Read, Rename, Seek, Unformat, Write),
  each the same step machine on +0x60 (0 start, 3 issue, 4 sceMcSync poll (mode +0xc8 = 1), 5 end;
  sync result -2 retries the issue up to 8 times); 0x12..0x16 chain them on +0x68: create the save
  (/BASLUS-21448: slime1.ico from data/mnurpk/mcdicon.rpk, icon.sys built at +0x6e8, the main file
  BASLUS-21448 and dt00..dt11 ________ of 0x3000 zero bytes, each skipped when the dir already has it
  at that size), read the dir, write a file (a data file goes through the temporary name dtNN1___),
  read a file, fill the card (/FEBDUMMY, debug). Fields: port +0x70, slot +0x74, type +0x7c, free
  KB +0x84, format +0x88, count +0x90, fd +0x94, open mode +0x9c, size / buffer +0xb0 / +0xb4, path
  +0x114, name +0x154, dir table +0x1c0 (rows of 0x40). The unnamed libmc calls by SDK 3.0 link
  order: 0x24d9e0 sceMcRead, 0x24daf8 sceMcWrite, 0x24df80 sceMcGetDir, 0x24e160 sceMcChdir,
  0x24e550 sceMcSetFileInfo, 0x24e720 sceMcRename, 0x24e948 sceMcGetEntSpace.
- **MsgMan** (msg/msgman.cpp, 0x18330c..; EE 0x3bc; not written): a text in a scene window (m_strBuf,
  optional voice stream). **MsgManSet** 0x18741c..0x18761c: two of them found by window number.

## The save / load screen (mnu/mnusvld.cpp, 0x1a59b8..0x1a90ec, 2026-10-25)

- **MnuSvld** (EE 0x2780; MagLifeMenuSave / Load +0x40, MnuTitle +0x1400, MagScene has one): an
  MnuMcd at +0 (its methods are called on this), open +0x1400, nine cursors +0x1404 ([1] slot, [2] row
  on screen) with counts +0x1428 ([1] 12, [2] 5, [8] 36), m_step +0x144c, m_sub +0x1450, m_sub_step
  +0x1454, SprMan +0x1474 (73 sprites, data/mnurpk/mnusvld.rpk: tex00..03, texmoji), MagSystem* +0x1d60,
  layer +0x1d64, MsgManSet +0x1d68, m_save +0x24e8 (1 save, 0 load), 12 slot records MnuSvldSlot
  +0x2508 (stride 0x2c: two flags, chapter, play hours / minutes, modify date y m d h m s; -1 = empty),
  init +0x2734, decide +0x273c (1 decided, 2 cancelled), m_svldBuf +0x2740 (0x3000-byte save image,
  the assert names it), scene window +0x2744, m_loaded +0x2754 (MnuTitle::execLoad). Data: texture
  list 0x2a4530, pattern image 0x2a45c0..0x2a64e0 (move table 0x2a6438, fade colours 0x2a60d8 /
  0x2a60e8), display list 0x2a64e0 (73 sprites), clear rects 0x2a7460 / 0x2a7470, text buffers bss
  0x6e9450 (times), 0x6e94a0 (date), 0x6e9520 (title), 0x6e95a0 (corrupted).
- **Card format**: one directory per slot, /BASLUS-21448/dtSSabCCHHMM (slot, flag a = 0, flag b =
  sequence bits word 0 bit 31, chapter 0..12, play time; all zero while bits word 8 bit 0x1000);
  makeSaveDir uses the existing dt<SS> directory's name or dtSS________. The save image is GamMan
  0x1d6078 (writeSaveData) + MnuMcd::makeChecksum; loading hands it to GamMan 0x1d68a0 (readSaveData).
  Chapter titles are system texts 0x1c6 + chapter; flag b puts them in brackets.
- **Flow**: exec step 0 opens a 0x200 x 0xe0 window (text 0x19), backs up S2dMes sprites 0..13, 1..3
  MnuMcd::init / setTextMode (2 load, 1 save), setDisp, button guides (S2dMes 4 / 5 via
  GamMan::showMesSprite / setMesSpritePos), fade in; 4 runs m_sub: 1 execCheck (check mode 3 load / 2
  save, then readSlots), 0 execList (check mode 4, cursor, decide = BedPad +0x7a bit 14, cancel bit
  13), 2 execSave (check mode 5; sets sequence bits word 2 bit 0x10000), 3 execLoad (check mode 6,
  readSaveData, m_loaded); 999 or a failed check fades out; closing sets m_global[0x15] = 7 (save) / 8
  (load). On a PC card with no save, check mode 3 stops at the "no data" prompt (checkLoad step 0x28):
  RIGHT + CROSS quits back to the title (seen 2026-10-25).

## The title screen (mnu/mnutitle.cpp, 0x1a90ec..0x1ab474, 2026-10-25)

- **MnuTitle** (EE 0x5700, MagLifeMenuTitle +0x40): MnuMcd +0, MnuSvld +0x1400 (m_save 0: load),
  MnuOpt +0x3b80, a text list +0x4c68 (MnuTextList, code 0x1a066c..0x1a0bb8, name invented), open word
  +0x4d6c, nine cursors +0x4d70 with item counts +0x4d94, timer +0x4db8, m_mode +0x4dc4 (processPad's
  step), m_sub +0x4dc8 (the sub menu), m_step +0x4dcc, SprMan +0x4df0 (EE 0x8ec), MagSystem* +0x56dc,
  layer +0x56e0, DogTexInfo* +0x56e4, the s100000.str stream id +0x56e8, m_result +0x56ec, init flag
  +0x56f4. Data: texture list 0x2a7798 (data/mnurpk/mnutitle.rpk: titlebg, titlebg2, ti_06 = the text
  sheet, sce, punch, siro, keikoku = the disclaimer), pattern image 0x2a7858..0x2a80c0, display list
  0x2a80c0 (15 sprites), logo flag 0x2a8400, logo sprites / frame counts 0x2a8408 / 0x2a8418, movie
  viewer pages 0x2a8428 (8 x 0x78).
- **Sprites**: 2..5 the logos (sce, punch, siro by pattern), 6 titlebg (press start), 7 titlebg2 (the
  menu background), 8 "Press START button", 9..11 New Game / Load Game / Option (anim 1 = selected),
  12..14 a three-way choice (Easy / Normal / Hard on the sheet; execSelect, unreached).
- **Flow**: processPad mode 0 silences the game (SprMan::saveVolume / volumeOff, GamMan::setOption(0,
  4), GamSndMan::resetVolume), 1 loads the textures (waits), 2 setDisp, 3 runs: sub 0 execLogo (first
  boot: MnuMcd check, logos 2 and 5 for 90 / 150 frames, CP_LOGO.PSS and MV00_AD.PSS through g_rxMpeg
  with MagSystem::m_v_interval 1, then press start over s100000.str; 0x708 idle frames replay the
  movie), sub 1 execMain (cursor with BedPad +0x70 bits 0x10 / 0x40, cross = +0x7a bit 14; result 2 new
  game, 3 load (g_sequence.m_global[0x17] = 0x10004, sub 3 execLoad), 4 options (0x10006, sub 4
  execOption), 1 when g_sequence.m_bits[1] bit 0x80 is clear). Results 1 / 2 close the title: mode 6
  calls GamMan::resetPcStat and processPad returns m_result to MagLifeMenuTitle. Subs 2 (test000.pss
  test), 5 (movie viewer) and 6 are set by no retail code.

## The task manager (task/tsk_man.cpp, 0x1e7094..0x1e7f28, 2026-10-26)

- `CmnITask` slots (8-byte GCC 2 entries, the function word at +4 of each): 1 `get_task_type` (+0x08, the
  kind), 2 `pause(on)` (+0x10), 3 `init` (+0x18, addTask), 4 `exec(id, time, arg)` (+0x20; time = the
  timer's m_lastTime, +0x64; non-zero ends the task), 5 `term` (+0x28, when the node is freed), 6 dtor.
  Kinds seen: 4 TskTest / TskLogger; pauseOthers spares 1, 2, 5, 7, 0xf, 0x11, 0x12, 0x17; killStageTasks
  spares 1..3, 0x11; setNodePause only tells kinds 5, 0x10, 0x13, 0x14; GamMan kills 5 / 0x13 / 0x14.
- `CmnTaskMan` (g_taskMan 0x7a3d80, EE 0x6c0): +0 the node list sorted by next-run frame, +4 a second list
  findTask searches first (nothing in the file adds to it), +8 the next id (from 2), +0x40 SysTimer (its
  m_updates +0x58 is the frame count addTask's start -1 means), +0x84 a kill queue of 128 ids with its end
  pointer at +0x284, +0x288 the live id table (256 words, -1 ends, -2 removed; exec rewrites it every frame,
  getTask reads it, addLiveId stops at 0xfe).
- Node (0x40, operator new): +0 task, +4 next, +8 state (0 run / 1 paused / 2 dead), +0xc id, +0x10 the
  caller's id word, +0x14 flags (bit 0: the node owns the task -- every caller passes 0x11 or 1; 0x10 is
  never read here), +0x18 arg, +0x20 kind, +0x24 start, +0x28 period, +0x2c last frame (start + frames - 1,
  0 forever), +0x30 last run, +0x34 next run.
- `addTask(group, start, period, frames, task, arg, &id, flags)`: group (a1) is never read.
- `exec` 0x1e7a20: hold the timer, step each due node (state 0: exec, next += period, ends on non-zero or
  past its last frame; state 1: only moves on; state 2: freed), apply the kill queue (applyKillQueue
  shortens the queue by its last word on a match, not the matched one -- a ROM quirk kept), sweep dead
  nodes, rewrite the live table, `GamMan::execPauseReq`. `killTask` kills a node in the list at once
  (killListed) and only queues ids it cannot find -- the node exec is stepping is out of the list then.
- Freeing (freeNode / termNode): term, then a plain `operator delete` of an owned task (no destructor
  call in the ROM; the host deletes through the virtual destructor).
- **Host defaults**: task classes whose virtuals are not written yet get CmnITask's host defaults (kind
  0, exec returns 1: the task ends on its first step, as when the manager was a stub), so a script that
  waits on getTask(id) does not hang on a stub class. A class's real exec replaces that.

## Attack / damage tasks (task/tsk_scene.cpp, 2026-10-26)

- Achr attacks and hits run as tasks on the achr's `m_task[3]` (ACHRstat +0x3e0): `scnAttackStart`
  0x1f9d90 (killTasks, then TskAttack kind 0x15) and `scnDamageStart` 0x1fa5bc (pauses the enemy, then
  TskDamage kind 0x16 and effect kind 1 40 below the achr). `scnChrRequest` 0x1fac60 dispatches on chr
  (0..10, or 11 = from the oid's kind): 3 the dog (0x1fae14), 4 the PC (0x1fb084), the rest damage.
- TskAttack's steps (picked by the kind word param[2]: 0x12..0x14, 0x15, 0x26..0x28, 0x2e..0x30) and
  most of TskDamage's live in the file after task/naga_chunk.cpp (unassigned@001fd424, 0x1fd424..
  0x200000), not in tsk_scene.cpp; they call scnChrRequest back.
- The damage roll `scnCalcDamage` 0x1fb624 (attacker, target, attack word): attacker's ST (param 0x1d,
  PC via getPcParam) x the word's percent (fixed or utlRandRange per word; 4 / 5 / 0xb use utlRandMean /
  utlRandPeak) / 100 x the weapon's system field 0x2b, + field 0x2a, - target kind's DEF (field 0x1e) x
  90..110 / 100, floored at 0. The weapon: the PC's equipped item (GamMan +0x2220c, none 0xa00 or word
  7), a kind-2 object's param 0x66 (none 0 / 0xa00); kind-5 attackers return field 0x2b x 70..100% x
  field 0x2a. Hits on the dog (`scnChrRequestDog`, only in inu mode 1) take LP and send Brown a hurt
  request (magBwnHurt -> MagLifeBwnCtrl state 10); hits on the PC (`scnChrRequestPC`) take LP and pick
  the PC state from data 0x2feea0 by word (2: front 5 / back 4; 9 / 10 become 0xb / 0xc while the
  attacking enemy holds), 0 LP -> state 3 and a TskFrameFunc kind 0xe on GamMan::m_frameTaskId.
- 0x1fc47c / 0x1fc490 (unreferenced) are the inline ctor / dtor of the chunk writer that
  task/naga_chunk.cpp's 0x1fc4f0 fills (+0x400 count, +0x504 buffer, +0x50c offset): naga_chunk.cpp
  starts at 0x1fc47c, not 0x1fc4f0.

## The script tasks (task/tsk_chr.cpp, 0x1e7f28..0x1f6c64, 2026-10-26)

Unanchored (no source string): the run between tsk_man.cpp and tsk_scene.cpp, pinned by its classes.
0x1f6c64.. (DisplayMessage, SeManager, VoiceManager, ScenePlaySingle, ScenePlay) belongs to
tsk_scene.cpp with 0x1f82e4.. (scenePlayStart). Holds the achr motion / movement tasks (TskMtnMove,
TskMtnMove2, TskMoveType*, TskMovePointPath, TskMoveSecta*, TskMoveAdj, TskRotAdj, TskMovePhysic,
TskAvoidMove, TskChrRot, TskAttach) with their chr* starters, and the script tasks TskMovie, TskMsgMan,
TskSeqEvent, TskSysFunc, TskFrameFunc / TskGenFrameFunc.
- **Step functions as member-function pointers**: TskMtnMove keeps a GCC 2 PMF {short delta, short
  index (-1 non-virtual), pfn} at +0x80, copied from data 0x2fd940.. by the constructor; exec calls it
  and returns its result. Written as `int (TskMtnMove::*)()` on the host. The other Tsk* classes with a
  table of step functions (TskFrameFunc 0x2fe420) use the same idiom.
- **Secta walks**: TskMoveSecta (kind 0xc) runs legs on ACHRstat::m_task[3] (+0x3e0), each a
  TskMoveSectaFindPath (kind 0xb) over GamMan::findSectaPath 0x1d81f0 (a GamSectaPath: 127 cell
  pointers, the count at +0x1fc; a cell's point at GamSecta +0x10) from GamMan::getObjSectaNo 0x1d7b08;
  the leg reports into the owner's status word +0xc (4 no way, bit 2 done). Both GamMan functions are
  stubs in unassigned@001d3b44.
- **Achr task slots**: ACHRstat::m_task[2] (+0x3dc) is the motion task (chrMtnMove / chrChangeMotion
  replace it, addTask flags 0x11 = owned); m_task[1] the turn task; m_task[0] the move type task.
  Non-achr objects get a TskMtnMove with actor -1 whose step (0x1e8fbc) ends it at once.
- **Angles**: the ROM wraps headings with an inline that compares in double against +-pi (the libgcc
  fptodp / dpcmp / dpadd / dpsub calls): chrWrapPi / chrNormPi in the source. rotY >= 32769.0
  (0x47000100) means "keep the actor's heading".
- **TskMovie** (kind 0, native 253's film task): constructor fades the music (funcSnd 0x1d), stops
  SndMan slots 8, fades S2dMes sprite 3, sets script bit word1 0x8000000, S2dMovie::playMovie, and
  MagSystem::m_v_interval = 1 (60 fps while the film runs; 2 again at the end). START held skips
  (pad m_hold bit 8 through MagSystem +0x97a40). Word 1 bit 0x80000000 set by the script suppresses
  the final fade-in. Seen working 2026-10-26: the New Game opening film plays to its end.
- **TskMsgMan** (kind 0x10, EE 0x9c0, GamMan::openMsg's task m_msgTaskId): **the narration
  subtitles of the opening film** ("Once upon a time,", "there was a precious little girl." -- frames
  ~2950 / ~3090 of a New Game run; the film's own handwritten cards are part of the movie). Text kept
  as "#C" + text at +4 (the strings carry their own "#R" codes), a scene window opened from
  GamMan::m_msgParam (p[0..1] position, p[3..4] size, p[5] the setMessage flag, p[6] life; p[2] gets
  0x8001), MsgManSet at +0x224. exec's end rule by mode +0x204: 1 time frames, 2 text out + CROSS
  (m_trig bit 14) after 10 frames, 3 either, 4 text out; MsgManSet::release (0x18761c) at the end.
  Nothing shows until msg/msgman.cpp (MsgMan / MsgManSet) is written.
- **TskMtnMove2** (kind 5, EE 0x100, chrMtnMove2): TskMtnMove's twin moving through DogActor::
  getMotionMove / addTrans with the root move turned by the actor's m_base_rotate[1] (a VU0 RotMatrixY)
  instead of a stored heading; its hit step tests from the "Bip01 Spine1" node (chrGetNodeWorldPos
  0x1f5558, DogActor vtable slot 18 getNodeWorldPos) and 1.25 radii ahead (GamMan::getObjRadius), and
  starts a TskAvoidMove (0x1ef184, not written) on ACHRstat::m_taskId (+0x3e8) when the actor stands
  in something. PMF at +0xcc.
- **TskFrameFunc** (kind 0x11, EE 0x80; the script's / GamMan's multi-frame jobs, written
  2026-10-26): constructor switch on kind (jump table data 0x2fe420, 23 entries) copies the job's
  words to +0x20.. and a PMF from 0x2fe2c0.. into +8; exec just calls it. Kinds: 2..5 keyhole fades
  (setFade kind 1 / 2, mode 2 in / 6 out), 6 system message 0xd / 0xe, 7 a door opening (PC motion
  pc_ / jnfc_door{r,o,l,t}_open__{x,y} by the side the PC stands on -- |wrap(angle - pi - doorRotY)|
  < 0.1 picks r (o for obj kind 0x1201) else l (t) --, camera at MagScene +0x2ab70 set out along the
  door's heading, PC placed 50 in front, GamMan +0x22050 = the door point), 8 a MnuTextList choice
  into script global 0x2d, 9 enterMenuPause, 10 PC rot 0, 11 a system call 0x34 pair's thread then
  requestStage(2), 12 fog lerp, 14 game over (DEATH_J.PSS film, reset, stage 99), 15 / 16 S2dBg
  picture 0 in / out (inlined s2dBg* requests on +0x93c), 0x11 / 0x16 clear GamMan m_flags2 bits,
  0x12 nothing, 0x13 an item found (windows 0x11 / 0x13, item menu when 12 are held), 0x15 the PC
  state-class 5..7 message (windows 0xb / 0x16). Kinds 0, 1, 13, 20 set no step.
- **TskSysFunc** (kind 0x17, EE 0x80, seqcall 0x22 on GamMan::m_sysTaskId): the **map-name caption**.
  Kinds 0..1 store map no a3 (+0x18) and the times 30 / 15 / 27 (+0x1c in, +0x20 out, +0x24 hold);
  the step 0x1f42c4 (folded into exec by Ghidra until split) calls the inlined S2dMes setMapNumber /
  setMapNameFadeIn (MagScene +0x42bb0 + 0x93c / 0x944 / 0x94c), holds, then setMapNameFadeOut. term
  of kind 1 clears a caption still shown. Written 2026-10-26.
- **TskSeqEvent** (EE 0x80, GamMan::startAchrPlaceTask's ACHRstat::m_placeTask): kind 0 with radius
  > 1 grows the achr's collision to radius over frames (m_colPrm [0] = 2, [1] = 4, [4] = radius * t;
  the ROM writes the other nine words from an uninitialised stack record). PMF at +4; 0x1f4910 (its
  stepNone) was folded into exec.
- **TskAttach** (EE 0x40, seqcall light case 0x3c): each frame light *value of type 2 follows node
  name (a3) of oid's actor: MagSystem::getActorNodeMatrixByName, the matrix's translation row to
  MagScene::setLightParam(2, light, 0, v, 0). Never ends by itself.
- **chrRot starters** (0x1f11a0..0x1f1670): chrRotFromTo, chrRotStartFrom (chrRotStart from a given
  heading), chrRotMotion / chrRotMotionBySign / chrRotToMotion (a turn played by a turn motion through
  TskChrRot's stepMotion). All replace ACHRstat::m_task[1]; their callers are the move tasks
  (TskMoveTypeGen, TskMoveSecta*) and 0x14a860 / 0x14bf1c / 0x154fd8.
- Frame request list: data 0x2d8f68 (count) / 0x2d8f70 (32 oids), filled by chrStepAwayFrom 0x1f68bc
  (achr a1 steps one unit away from object a0; a0 recorded; only reference a table word at 0x2e50f4),
  cleared each frame by gamClearFrameReq (only 8 words, while the lookup scans all 32).
- **Limbs**: ACHRstat::m_attr[0] bits 0..4 = Bip01 Neck / R UpperArm / L UpperArm / R Thigh / L Thigh;
  chrRestoreLimb 0x1f5680 (from 0x165074) clears the bit and calls DogActor::setNodeScale(1.0, node)
  0x10eba0: a 5-entry node / float list at DogActor +0x3f8 / +0x40c where 1.0 frees an entry (the
  setter of other values, likely the limb loss, not found yet).
- Last chr* helpers (2026-10-26): chrMtnMove2Achr 0x1f603c (10 callers 0x146944..0x168844) with the
  folded, uncalled chrSetMtnMove2Step 0x1f616c; chrSetMotionByOid 0x1f627c (0x14e850, 0x14f360); the
  uncalled chrCreateChr 0x1f6694 (no < 4 achr, < 10 fchr) and chrCreateFchr6 0x1f67a0 (fchr type 6,
  params 0x32 / 0x33).

## The sound state (game/gamsnd.cpp, 0x1e0df0..0x1e7094, 2026-10-26)

- `GamSndMan` (GamMan +0x18e00 = 0x79a0c0, EE 0x1878, vptr +0x1874, one virtual: the destructor).
  +0 the listener mode (0 the PC: GamMan +0x22020 pos / +0x22030 rot; 1 the camera: MagScene +0x2ab70
  m_trans / m_rotate; loadStage sets 1). +0x40 / +0xb0 the free / used 3D sound lists of sequence.h's node-pool
  template (+0x120 the node count 0x20, 32 nodes of 0x60 at +0x140, the sound `GamSnd3d` at node +0x10). +0xd40 / +0xd44 / +0xd48 a record range {begin, end, 0x30} over 0x30
  `GamSndRec` (0x28) at +0xd80, indexed by SndMan id: +0 flags (1 stream, 8 playStr's bool -- isPlayEnvStr,
  0x10 loop set, 0x80 kept, 0x100 a BGM stream), +4 / +8 volume, +0xc SE id, +0x14 stream number, +0x18
  SndMan id. +0x1540 the SE table (0x400 x 0x38, allocDebug line 81), +0x1544 the stream table (900,
  line 82; hashed from (str % 100000) / 2, -1 free). +0x1550 `m_defEntry`, a GamSndEntry
  with only the 3D words set (vol / pan 0x80, far 0x6a4, near 100, nearVol 0x80, farVol 0) for sounds
  without an entry. +0x1588 / +0x1590 / +0x1598 the master / SE / stream
  volume words; +0x15a0 four 0x24-byte BGM push levels and at +0x1630 a fifth, the record while nothing
  is pushed (get_snd_prm 0x28fdf8) (`GamBgmRec`: SE, stream, a second stream fadeOut3dSnds
  keeps, each with volL / volR), +0x1654 a second SE per level, +0x1684 the push depth (-1 none; pushSnd
  only pushes from -1), +0x1688 eight looping SEs, count +0x1708; task ids +0x170c (master fade),
  +0x1710 / +0x1714 (SE fades), +0x1718 (stream fade); +0x171c / +0x1720 / +0x1724 the reverb (setReverb: rxSndSetEffectMode / Depth; 4, 0x80,
  0x80 at resetVolume); +0x1728 the SE pack id loaded in each of the 65 banks; +0x182c the last
  loadSePack(name, nowait) name ("BBII.se", read from data/snd/xv/); +0x186c the 3D scale (0.5); +0x1870 flags (bit 0 set by stopBgm, cleared by resumeBgm).
- SE packs are named "BBII.se": bank BB, pack id II (`parseSePackName` 0x1e43e0); `loadSePackData`
  0x1e44b4 stops SEs (funcSnd 0x11), `SndMan::loadBank(bank, data, nowait)` -- the second argument is the
  XVAB *data* (rxSndXvabLoad's), not a name.
- SE ids: id >> 24 == 0xb are BGM-class SEs (pushed with the music); 0xb030352 is special-cased (looped
  on resume, never pushed). The option levels 0..8 scale the SE / stream master volume to 10..100..128
  (funcSnd 0xe / 0xf, at least 10).
- Fades are `TskSnd` tasks over a `TskSndParam` block (task/tsk_scene.h, written 2026-10-26): kind 0 SE,
  1 stream, 2 / 3 / 4 the master / SE / stream master volume (funcSnd 0xd / 0xe / 0xf), 5 a delayed
  playSE, 6 an SE faded out (funcSE 0x13, 10 frames) after +0x14 frames, 7 play stream +0x1c's name once
  the old one stopped; target +4 / +8, frames +0x14, start volume +0x20 / +0x24; +0x28 on a stream fade
  at 0 / 0: funcStr 0x17 instead of 1. The exec step (stepFade) blends start -> target by frame / frames
  and runs only while TskSnd +0x40 is 100 (101 ends; nothing found writes it). `TskSnd::getVolume`
  0x1f9658 returns the fade's *target* (+0x84 / +0x88), not the volume now. Task kind 0x12.
- **3D sounds** (2026-10-26): `GamSnd3d` (0x50): +0 m_pos (room coordinates, update3d), +0x10 m_src
  (put_*_3d_pos: pos + the WRD room offset; _chr: the offset), +0x20 the SE / stream, +0x24 its record
  (SndMan id), +0x28 flags (1 stream, 2 heard only in its room / dropped by stopObjSnds, 4 follows
  object +0x2c, 0x20 start now, 0x40 restarted by update3dVolume, 0x200 one-shot), +0x2c oid (0 a fixed
  position), +0x30 the room (WRD), +0x34 the entry (NULL: m_defEntry), +0x38 the sort key (3000 at
  put), +0x3c / +0x40 the volume set. put_* sort the used list by the xz-then-y length of m_src; the
  _chr forms are once per (oid, sound). `update3d` 0x1e3c24 (skipped while flags bit 0 or GamMan
  m_flags2 bit 31): fixed sounds key |dx|, object ones the *sum* dx + dy + dz (as the ROM does); beyond
  15000 on x or z an SE stops / a stream fades over 10 frames (m_strTaskId); the 3 nearest SEs or 4
  nearest streams go to `update3dVolume` 0x1e386c: gamSnd3dVolume, eased ((new + old) * 0.75 when new
  is 0 or > 0x20 off), times m_3dScale outside its room, SE volume > 0xff -> 0; an ended sound restarts
  (playSE / playStr 7-argument forms) unless a one-shot (0x200, or 0x20 | 0x40), which is deleted.
- The 7-argument playSE 0x1e50c8 is (se, volL, volR, pitch, attr, delay, flags): a delay hands it to a
  TskSnd kind 5 (TskSndParam +0x10 attr, +0x18 pitch) starting at CmnTaskMan frame + delay; playStr
  0x1e5428 is (str, volL, volR, pitch, loop, id, flags).
- `gamSnd3dVolume` 0x1e6e00: the entry's distance fade (+0x2e up to distance +0x2c, +0x30 from +0x2a,
  linear between) times `gamSndEarGain` 0x1e6d54 per ear (+-pi/3 off the facing; 0.3 behind .. 1.0 ahead).

## task/tsk_scene.cpp (0x1f6c64..0x1fc4f0, pinned 2026-10-26)

Anchored by "task/tsk_scene.cpp" at 0x1f8034 / 0x1f81a8 (assert lines 790 / 852, the scene's camera and
focus file loads); ScenePlay's vtable slots sit on both sides (0x1f7d54..0x1f82e4), so the run from
tsk_chr.cpp's end to naga_chunk.cpp is one file. Classes: DisplayMessage, SeManager, VoiceManager,
ScenePlaySingle, ScenePlay (0x2e4dd0), PLScenePlay (0x2e4d90), TskSnd (0x2e4e70), TskAttack (0x2e4eb0),
TskDamage (0x2e4ef0), TskCamera (0x2e4f30); free functions scenePlayStart 0x1f83f4, scenePlayStartSimple
0x1f8cec, scnChrRequest 0x1fac60, gamSystemDataLoaded 0x1f8fb8 (empty), gamStage2Loaded 0x1fbbd0 (a
free-memory debug print). 0x1fc47c / 0x1fc490 (a reset and a destructor freeing +0x504) belong to a class
not identified yet.
- `TskCamera` (kind 0 of the task list; native 016 case 50 builds it as `TskCamera(a2, a1)`, a1 the
  script's words): kind 0 = look around with the left stick (words 1..3 the centre rotation, 4..6 / 7..9
  min / max, 10 the speed; plus a random drift, both smoothed 0.9 / 0.1 and 0.85 / 0.1) written into the
  scene camera (MagScene +0x2ab70 = bss 0x7268f0) m_rotate each frame; kind 1 = one set-up: words 0 / 1
  two objects turned to face each other (word 2: both), the camera mode 1 at pitch -0.2, 30 to the side
  of object 0 and 70 back from object 1's head (160 up), pulled 15 in when MagScene::castRay hits. Both
  run until killed (exec returns 0). 0x25b8c0 is `sceVu0ScaleVector` (lib-context).
- Event scenes (written 2026-10-26): `scenePlayStart(name, oids, withEff)` (natives 0x46 returns it /
  0x47) finds `name/name` (`scnMakeScenePath`: the name up to its '.') in GamMan's event pack (+0x22a80);
  that file is a pfh: file 0 a NagaChunk table (16-byte entries {name[8], offset, ?}, zero-terminated;
  `find` scans on from the cursor, so callers rewind), file 1 an effect (Obj176a3c kind 0xd, data
  pointer at param +0x10; only withEff), file 2 the focus track (`MagScene::setFocusData`, size from the
  pfh's size table). Chunks: "map" (int; 0xcccc once the motion names were converted -- written back
  into the pack), "CHARA" -> "THREAD" per actor -> "name" (model; loaded with an oid / arg picked from
  `motNameGetCategory`: 0 (1,0) 0x2000, 1 (1,1) 0x2400, 2 (1,3) 0, 3 (2,5) 0xc00, 4 (2,4) 0x800, 5 (1,2)
  0x1400, 6 (2,9) 0x1800, 8+ (1,3) 0x400 * (cat - 7) + 0x2400, unless the oids array's entry is a live
  object), "data" (0x48-byte `ScenePlayRecord`s run by a `ScenePlaySingle`), "STR" (up to 19 {name,
  key} motion names for `motNameConvert`); "frame" (the length), "MESSAGE" -> "strings" (lines). Kills
  kind-5 tasks first, clears GamMan m_flags2 bit 31, sets g_sequence.m_flags 0x400, and adds the
  ScenePlay (kind 0xf) with flags 0x11 on GamMan::m_frameTaskId. Record kinds: 1 place + Y rotation, 2
  motion (speed rec/60), 3 SE (panned volume or at the actor's Bip01 node; SeManager stops it after its
  frames), 4 voice stream, 5 message line (window 0x20,0x140 0x1c0x0x50), 6 / 7 scene fade in / out
  (skipped while one runs), 8 / 9 show / hide. ScenePlay::term releases the models it loaded.
