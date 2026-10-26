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
    0x237c68 is vsprintf into 1 KB + rxSysPrintf + fflush(stdout). i3dMatGetScaleTrans 0x228c38 (just
    before, still in unassigned@00228c08) stays a bridge.
  - **The i3r model files** (I3D_BIN / I3D_I3M / I3D_I3C; 0x228c08..0x22e840, 2026-10-25). An
    I3D_BIN file (`i3d/i3rroot.cpp` 0x22b1b0..0x22b4b8, written, verify; types in i3rroot.h): id
    "I3D_BIN\0" (rodata 0x3039d0), +8 version 0x00100001, +0xc relocated flag, +0x10 the root
    item (type 0x52). Every item is 16 bytes {data, info (bit 31 no data fixup, 30..24 type,
    23..0 child count), children, name}; child lists are sorted by name (0x22c1f8 bisects with
    strcmp). In the file item fields are offsets from the root item, pointers inside an item's data
    offsets from that data. i3rRelocate 0x22b330 (DogModel::loadPack and 9 more callers) ->
    i3rSetRelocate 0x22b280 -> i3rItemRelocate 0x22c478 (recursive; per-type data fixups in the
    90-entry table 0x2db4c8, unfixups 0x2db360 for i3rItemUnrelocate 0x22c328; i3rIsItemType
    0x22c2a0 rejects 13 ids) then i3rBuildPackets 0x21a3e8 (0x21fb50 per mesh into child 6's slots).
    Root data (type 0x52, fixups 0x22ad20 / 0x22adf8): +0x10 / +0x14 tables of 0x40-byte records,
    +0x18 texture names (count u16 +0x2c), +0x1c u16 node indices and +0x20 their names (count
    u16 +0x2e), +0x24 a plain value. Root child 3 = the nodes (i3rGetNumNode), child 2 the meshes,
    child 6 the packet slots. The rest of the run is per-type code, each type a cluster of small
    accessors (`*(item+8)` = its children) plus its fixup pair -- ~450 functions Ghidra folded
    (hidden.py), e.g. type 77's fixups 0x22a6c0 / 0x22a730 sit inside the mesh packet builder
    0x228c08..0x22a7d0 (0x228f50: a per-layer cache of mesh packets hashed on mesh / light / fog
    / view). I3D_I3M roots from 0x22b4b8 (id check 0x22b4b8), I3D_I3C from 0x22ba20, curve /
    interpolation maths 0x22e840..0x231198, collision (i3dPointSegDist2, i3dSegSegDist2)
    0x231198..0x23695c. Host pointers in these files: hazards.md "Types and layouts".
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
    0x21c960.. is a different module: VU1 strip uploaders (VIF UNPACK 0x6c.. codes, i3dPktDrawStrip*,
    i3dPktDrawLineStrip, the wire shapes, matrix / vector uploads 0x21e428 / 0x21e488, the layer
    header 0x21e5a0, 0x21e690).
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
  (buffer +0, three DogModels at +0x10 stride 0x1a0, id +0x4f0 (-1 free), texture list +0x504,
  data +0x508; released by 0x1386d4 / 0x13ff00), +0x2aa10 two 0x38-byte view records (near 5,
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
    textures, keep ? 2 : 1, m_22cc)`, then setId, 5 at +0x1d8, `ready()` (+0xf0 = 0, +0xf4 = -1).
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
- **Word table** 0x298ee0: data/wrd/WRD.BIN loaded by 0x17e5c0 (from GamMan::loadStage);
  `WrdTable::get(n)` 0x17ea44 -> record n (count at the file's +8): four string pointers +0x08..+0x14
  (gamGetWrdString 0x1db8e8 which 0, 1 / 4 the map name, 2 the caption S2dMes shows, 3; "<map_none>"
  without a room or a record), +0x18 / +0x1c words gamSetWrdRoom 0x1dbcf0 copies to GamMan +0x18dd0 /
  +0x18dc0, +0x20 the room offset. Class name invented from the file.
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
| `mnu/` | mnuSelectLanguage, mnudlg, mnuselect, mnusvld |
| `msg/` | msgman |
| `sequence/` | sequence |
| `snd/` | rxmpeg, sndman, unitque |
| `spr/` | sprman (+.h), sprtexture |
| `system/` | sysnowloading |
| `task/` | naga_chunk, tsk_bg_load, tsk_scene |
| `utl/` | utlbtl, utlfile, utlfind, utlgame, utlmisc (0x105650..0x105e50: text helpers, `utlParseParam` name/value lines, the frame counter, operator new/delete, the game heap), utltm2 |

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
- The Dog* object layer: DogEntity (vptr +0x94) <- DogEntityEx <- DogModel <- DogActor / DogCamera
  (`DogActor` 21 virtual slots); DogEntity::reset 0x280ec8 writes 16-byte vectors at +0x40 / +0x50 / +0x70
  (quadword `sq`). Layout (2026-10-22, the magsystem actor methods; `src/dog/*.h`): DogEntity +0x00
  world matrix, +0x40 translation, +0x50 rotation (radians, `normalizeRotate` 0x10ff74 wraps to
  [-pi, pi)), +0x60 scale, +0x70 offset (y = TransOfsY), +0x80 a (0,0,0,1) vector of unknown role,
  +0x90 dirty flag, +0x94 vptr; DogEntityEx +0xe0 parent (`setParent` 0x1102b4 refuses cycles);
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
  `EmyManager::addEmy` 0x162c2c with the chr no from system data field 0x72.
- Models: `createObjByModel` 0x1ca8e8 (`MagSystem::loadModel`, which returns the actor number) and
  `createObjByName` 0x1ca72c (`MagScene::loadModelByName` from the archive of group kind >> 10:
  table 0x2fbed0 -> data/ch/action/ACTION.RPK, target/TRG, fix/FIX, item/ITEM, inst/INST,
  enemy/ENEMY, door/DOOR, (7 none), pc/PC, bwn/BWN, 01/CHAP01 .. 12/CHAP12.RPK). Model 0x800 and the
  model named "target" get scale 9.9999994e-8 (hidden markers). Light id: 1 for types 0..3 / 5, 0 for
  4 / 9 (`setObjLight` 0x1ca598).
- Markers: `ctlObj1cb8e4` 0x1cb8e4 -> `addObjMarker` 0x1cb970: an Obj176a3c effect 0x32 {x, y, z, 50}
  bound to the oid in the pool +0x20b00 (released by ctlObj1cbb80).
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
  a < 9 at stride 0x3c), then count achrs built with oid 0: addEmy (0x162c88) makes their oids
  0x9100006f + n and stores them in GamMan's m_achr table itself.
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
  +4 (first word = oid); 8 EmySlot records of 0x3c at +0x148; GamMan::switchEnemies 0x1c5a5c (case 50).
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
  corners ordered when flags & 0x103; 2..5 get a fresh oid makeOid(0, 0xd)), 10 startTrgs 0x1de4e4, 11
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
- **mnuBase** (vtable 0x2e4640, vptr +0x10c): +0 type, +4 flags (bit 0 = inited, mnuBase::isInit),
  +8 scene; slots 2 init, 3 term, 4 isInit, 5 processPad, 6 draw. MnuMap, MnuPBook, MnuDiary derive it.
- **MagLifeMenuPBook** (0x100000 / 0x200000 types): init looks the type up (getTypeValue) and asks
  MnuPBook (+0x50) then MnuDiary (+0x910) setType (page tables 0x29c0f8 / 0x29c368, 12-byte rows);
  the one that knows the page becomes +0x40 and runs through mnuBase's virtuals.
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

## The sound state (game/gamsnd.cpp, 0x1e0df0..0x1e7094, 2026-10-26)

- `GamSndMan` (GamMan +0x18e00 = 0x79a0c0, EE 0x1878, vptr +0x1874, one virtual: the destructor).
  +0 the listener mode (0 the PC: GamMan +0x22020 pos / +0x22030 rot; 1 the camera: MagScene +0x2ab70
  m_trans / m_rotate; loadStage sets 1). +0x40 / +0xb0 two 3D sound lists (a 0x60-byte sentinel node and
  a count at +0x60 each; prev at +0, next at +4), 32 nodes at +0x140 (node +0x30 the sound, +0x34 its
  record, +0x38 bit 0 a stream). +0xd40 / +0xd44 / +0xd48 a record range {begin, end, 0x30} over 0x30
  `GamSndRec` (0x28) at +0xd80, indexed by SndMan id: +0 flags (1 stream, 8 playStr's bool -- isPlayEnvStr,
  0x10 loop set, 0x80 kept, 0x100 a BGM stream), +4 / +8 volume, +0xc SE id, +0x14 stream number, +0x18
  SndMan id. +0x1540 the SE table (0x400 x 0x38, allocDebug line 81), +0x1544 the stream table (900,
  line 82; hashed from (str % 100000) / 2, -1 free). +0x1588 / +0x1590 / +0x1598 the master / SE / stream
  volume words; +0x15a0 four 0x24-byte BGM push levels (`GamBgmRec`: SE, stream, a second stream fadeOut3dSnds
  keeps, each with volL / volR), +0x1654 a second SE per level, +0x1684 the push depth (-1 none; pushSnd
  only pushes from -1), +0x1688 eight looping SEs, count +0x1708; task ids +0x170c (master fade),
  +0x1710 / +0x1714 (SE fades), +0x1718 (stream fade); +0x1728 the SE pack id loaded in each of the 65
  banks; +0x186c the 3D scale (0.5); +0x1870 flags (bit 0 set by stopBgm, cleared by resumeBgm).
- SE packs are named "BBII.se": bank BB, pack id II (`parseSePackName` 0x1e43e0); `loadSePackData`
  0x1e44b4 stops SEs (funcSnd 0x11), `SndMan::loadBank(bank, data, nowait)` -- the second argument is the
  XVAB *data* (rxSndXvabLoad's), not a name.
- SE ids: id >> 24 == 0xb are BGM-class SEs (pushed with the music); 0xb030352 is special-cased (looped
  on resume, never pushed). The option levels 0..8 scale the SE / stream master volume to 10..100..128
  (funcSnd 0xe / 0xf, at least 10).
- Fades are `TskSnd` tasks over a `TskSndParam` block (lib/unassigned.h): kind 0 SE, 1 stream, 2 / 3 / 4
  the master / SE / stream master volume; target +4 / +8, frames +0x14, current +0x20 / +0x24, +0x28 1
  stops a stream at the end (funcStr 0x15 / 0x1b). `TskSnd::getVolume` 0x1f9658 reads a fade's volume now.
- `gamSnd3dVolume` 0x1e6e00: the entry's distance fade (+0x2e up to distance +0x2c, +0x30 from +0x2a,
  linear between) times `gamSndEarGain` 0x1e6d54 per ear (+-pi/3 off the facing; 0.3 behind .. 1.0 ahead).
