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
- **The globals file** before main.cpp (ends with the static-init pair 0x20439c / 0x2043bc; 0x2041f8
  constructs and destroys, in order): game heap `BedMemory` 0x6e9800, `MagSystem` 0x6e9840, `GamMan`
  0x7812c0, `CmnTaskMan` 0x7a3d80, `Sequence` 0x7a4440, `EmyManager` 0x7b2140, `SndMan` 0x7b2580,
  0x7b40c0 (ctor 0x1810f4, class unknown). In src these objects are defined in
  `lib/unassigned_stubs.cpp` until that file is defined (the heap stays in utl/utlmisc.cpp).
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
  Files pinned so far (2026-10-22): **`i3d/i3dctx.cpp`** 0x21eb28..0x21fb30 (written, verify) and
  **`i3d/i3dvram.cpp`** 0x227a98..0x228c08 (written, verify). Modules seen around them: matrix / camera
  helpers before 0x21eb28 (0x21e1d8), texture objects from 0x21fb30, GS transfer builders 0x218fa8 /
  0x2190c8 / 0x2192c8 and 0x219c88..0x219d68, packet heads 0x21b490..0x21b698, draw groups / layers /
  sort 0x2239b8..0x224d88, the i3d system calls 0x237178..0x237cc8 (i3dFlushCache, i3dPrintf).
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
    +0x834; getters 0x223270..0x223294 read +0x824..+0x834; the view from `i3dCtxCreateView`
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
  (0xd40), `MagLifeBwnCtrl` +0x2bd20 (0x140; its state word at +0 is MagSystem::getBwnState's),
  the MagLifeMenu* objects +0x2be60..0x40780 (Item, Map, Stock, Save, Load, Title, Opt, Movie,
  Mcd, Now, Board2D, SelLang, each vptr +0x14), life objects of unnamed classes at +0x41850
  (ctor 0x12f008), +0x41890 (0x12f298), +0x418f0 (0x130da4), the 2D layers `S2dCineSco`
  +0x41930, `S2dBg` +0x42260, `S2dMes` +0x42bb0 (all MagLife, level 7, life priorities 0x8006 /
  0x8000 / 0x800a), objects at +0x837e0 (ctor 0x176a3c), `S2dMovie` +0x84aa8 (spr/s2d.cpp), +0x8542c
  (0x17c0f0), `MagSceneIFMap` / `MagSceneIFChara` at +0x854e0 / +0x854e8, Brown / PC actor
  numbers +0x853b4 / +0x853b8. Every MagLife has `m_scene` at +0x20 and the vptr at +0x14;
  MagLifeControlChara's bound actor is +0x58 (`m_actor`).
- **The scene's frame** (`mag/magscenerender.cpp`, pinned 0x139540..0x13c328, 2026-10-22;
  `partial`: all but makeActorShadow 0x139b4c). `MagScene::draw(ctx, dbuff, frame)` 0x13bcc0:
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
    `FileReader` (+0) and its `FileReadReq` (+8).
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
- **The file reader** (unassigned@001bde58, 0x1be2cc..): `FileReader` {request, own fd} runs a
  `FileReadReq` (0x24: fd, step, name, seek offset / whence, buffer / size, close flag) as an
  open / seek / read / close state machine over rxSysStreamOpen / rxFileSeek / rxFileRead /
  rxFileClose; while it waits it draws the now-loading screen (0x2051cc) when one is up, else
  rxThSync + SndMan::update. `fileGetSize` 0x1be768, `fileExists` 0x1be84c.
- 0x7b40c0: a movie-player object (`RxMpeg`, ctor 0x1810f4; class name unverified): +0 open
  (`open` 0x181150 sets it, `close` 0x1816b0 clears it), +4 = 1 while a frame is there to draw,
  +0x2c / +0x30 width / height (0x200 x 0x180 by default), +0x44 the movie name (0x80 bytes).
  `play(name, w, h)` 0x18154c, `draw(I3dLayer*)` 0x1812a8, `stop` 0x181644, `getStatus` 0x18173c
  (-1 not open, then the rxmpeg library's status word: callers read 0 playing, 1 finished), `isEnd`
  0x18176c. Its users go through S2dMovie's wrappers (below); FUN_001aabb4 / 0x1ab04c play a movie,
  skip it on a button (stopMovie) and close it once finished.
- **Word table** 0x298ee0: data/wrd/WRD.BIN loaded by 0x17e5c0 (from GamMan::loadStage);
  `WrdTable::get(n)` 0x17ea44 -> record n (count at the file's +8), its text at record +0x10 (the map
  names S2dMes shows). Class name invented from the file.
- `BedSubMode` (bed/bedsubmode.cpp asserts `m_depth < MAX_STACK` / `m_depth > 0`): a current qword
  at +0, a stack of 4 at +0x10, m_depth +0x50; push 0x100258 / pop 0x1002c0. The scene has one at
  +0x22d0 (`MagScene::pushSubMode` 0x132744 pushes and sets mode 8 / -1, `popSubMode` 0x132784): the
  movie skip runs in sub mode 8.
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
  tables are nested records with EE pointers inside; they are kept as EE word images until SprMan
  is written (datadump.py --words).

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
  no vtable of its own in the ELF (a pure interface whose vtable was never emitted -- unverified).
- The Dog* object layer: DogEntity (vptr +0x94) <- DogEntityEx <- DogModel <- DogActor / DogCamera
  (`DogActor` 21 virtual slots); DogEntity::reset 0x280ec8 writes 16-byte vectors at +0x40 / +0x50 / +0x70
  (quadword `sq`). Layout (2026-10-22, the magsystem actor methods; `src/dog/*.h`): DogEntity +0x00
  world matrix, +0x40 translation, +0x50 rotation (radians, `normalizeRotate` 0x10ff74 wraps to
  [-pi, pi)), +0x60 scale, +0x70 offset (y = TransOfsY), +0x80 a (0,0,0,1) vector of unknown role,
  +0x90 dirty flag, +0x94 vptr; DogEntityEx +0xe0 parent (`setParent` 0x1102b4 refuses cycles);
  DogModel +0x11c `m_i3r_root`, +0x124 i3c root, +0x128 colli table, +0x130 node matrices (0x40 each),
  +0x134 node flag, +0x138 attach node, +0x13c attention, +0x140 hide, +0x144 light id; DogActor +0x1a0
  id, +0x1c0 / +0x1c4 motion count / `m_motion_data` (0x20 each, banks 0..3), +0x3c0 base rotation,
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
| 0x215a30 | (see above) the assert handler; src writes asserts as `ROR_ASSERT(expr)` (common/ror_assert.h: reports and continues) | |
| 0x267798 | `_InitSys` | libmatch (FF2 initsys.c) |
| 0x6e9840 | `MagSystem` (mag/magsystem.cpp): frame, V-blank semaphore `m_v_sema` (sdata 0x29619c), scene member at +0x12540 (0x6fbd80) | asserts; main |
| 0x7812c0 | `GamMan` (game/gamman.cpp, vptr +0x2ab4): common data and stage loads, per-frame game update | vtable; main |
| 0x7a3d80 | `CmnTaskMan` (name unverified): task list, its `SysTimer` at +0x40 | main; 66 addTask call sites |
| 0x7a4440 | `Sequence` (sequence/sequence.cpp and the run 0x20eda4..0x215e1c): event sequences ("mst", "trg", "imdy", "single") | main |
| 0x7b2580 | `SndMan` (name from snd/sndman.cpp, unverified): sound manager over rxSnd; +0x4 holds argv[0] (0x20 bytes) | main; rxsnd strings |
| 0x269fe0.. | libdbc (`sceDbcInit` / `sceDbcEnd`) and libpad2 (`scePad2Init` 0x26b1d8 / `scePad2End`) between libipu and libpad: named by reading; `sceMcEnd` is 0x24d448 (libmatch had sceMcChangeThreadPriority) | strings; main |
| 0x100248 | `_exit`: crt0's `j Exit` thunk; `Exit` is 0x267b98 | FF2 symbols at the same addresses |

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
  libdma / libvif / libvu0 (`vu0_lib.cpp`, `vu0_intrin.h`), libpad (+ `ROSEPAN_AUTOPAD`), libmc,
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
