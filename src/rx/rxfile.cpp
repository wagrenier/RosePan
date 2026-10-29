/* ==========================================================================
 *  rx/rxfile.cpp
 *
 *  The rx middleware's file layer: async file slots, device drivers (the
 *  cdrom one reads ISO 9660 itself), contexts, dirs, whole-file loads, IOP
 *  reboot and module loads, and the file thread that serves the slots'
 *  requests. See rxfile.h.
 *
 *  The ROM saves the COP0 Status around every DIntr ... EIntr section and
 *  restores it after (so a section nested in another leaves interrupts
 *  off); the host's sections are a recursive mutex that nests by itself,
 *  so the save / restore is left out. Every spin-wait in the ROM also
 *  prints "DI error" when it finds interrupts disabled (the file thread
 *  could then never answer): rxFileSpin, empty on the host.
 * ======================================================================== */
#include "common/rosepan_stub.h"

#include "rx/rxfile.h"

#include <ctype.h>
#include <string.h>

#include "rx/rxsys.h"
#include "rx/rxthread.h"
#include "sdk/eekernel.h"
#include "sdk/fileio.h"
#include "sdk/libcdvd.h"
#include "sdk/sifcmd.h"
#include "sdk/sifdev.h"

/* ---- data ------------------------------------------------------------- */

static int s_verbose = 1;                       /* data 0x2dc1b0: print rxfile messages */
static const char *s_hostRootDefault = "host0:"; /* data 0x2dc1b4 */
static int s_hostFlags;                         /* data 0x2dc1b8: bit 0 = busy          */
static const char *s_cdRoot = "cdrom0:";        /* data 0x2dc1c0 */
static const char *s_cdIsoPrefix = "\\";        /* data 0x2dc1c4 */
static const char *s_cdIsoSuffix = ";1";        /* data 0x2dc1c8 */
static int s_cdFlags;                           /* data 0x2dc1cc: 1 tray was open, 2 reading */
static int s_cdReadPos;                         /* data 0x2dc1d0: byte position of the last read */
static int s_cdReadPos2;                        /* data 0x2dc1d4: never written            */
static int s_cdTrayReq;                         /* data 0x2dc1d8: 1 open, 2 close requested */
static RxFileCdFindFunc s_cdFind = rxFileCdFind; /* data 0x2dc1dc */
static int s_cdClockRead;                       /* data 0x2dc1e0: read the clock periodically */
static int s_cdClockTick;                       /* data 0x2dc1e4 */
static int s_cdClockCountdown;                  /* data 0x2dc1e8 */
static int s_cdClockInterval = 0x1e;            /* data 0x2dc1ec: ticks between reads      */
static sceCdCLOCK s_cdClock;                    /* data 0x2dc1f0 */
static int s_cdClockValid;                      /* data 0x2dc1f8 */
static int s_cdDual;                            /* data 0x2dc1fc: dual-layer disc          */
static u_int s_cdLayer1Start;                   /* data 0x2dc200 */
static int s_cdLayer;                           /* data 0x2dc204: layer the lookups use    */
static int s_cdLayerMode;                       /* data 0x2dc208: layer asked for          */
static int s_cdLayerPending;                    /* data 0x2dc20c: s_cdLayerMode changed    */
static int s_cdTrayTimer;                       /* data 0x2dc210 */
static long long s_cdLastSector = -1;           /* data 0x2dc218 (a doubleword): last read offset */
static const char *s_hddRootDefault = "host0:"; /* data 0x2dc220 */
static int s_hddFlags;                          /* data 0x2dc224: bit 0 = busy          */
static int s_deviceMap[RX_SYS_DEVICE_NUM] = { 0, 1, 2, 3 };             /* data 0x2dc228 */
static int s_fallbackDevice[RX_SYS_DEVICE_NUM] = { 0, 1, 2, -1 };       /* data 0x2dc238 */
static RxFileErrorHook s_errorHook;             /* data 0x2dc248 */
static int s_deviceThread[RX_SYS_DEVICE_NUM];   /* data 0x2dc250: file thread per device (all 0) */

RxFileSlot     rxFileSlots[64];                 /* bss 0x7dabb0 */
RxFileTag      rxFileTagPool[64];               /* bss 0x7ddfb0 */
RxFileTag     *rxFileTagFree;                   /* bss 0x7de3b0 */
RxFileMarker   rxFileMarkerPool[64];            /* bss 0x7de3b4 */
RxFileMarker  *rxFileMarkerFree;                /* bss 0x7de9b4 */
RxFileSegment  rxFileSegmentPool[256];          /* bss 0x7de9b8 */
RxFileSegment *rxFileSegmentFree;               /* bss 0x7df9b8 */

static RxFileDriver s_drivers[RX_SYS_DEVICE_NUM];   /* bss 0x7df9c0 (HOST 0x7df9c0, CDROM 0x7df9e4, HDD 0x7dfa2c) */
static char s_hostRoot[0x100];                  /* bss 0x7dfa50 */
static RxFileCdCacheNode s_cdCacheNodes[32];    /* bss 0x7dfb80 */
static RxFileCdCacheNode s_cdCacheUsed;         /* bss 0x7dfd80: sentinel of the used list */
static RxFileCdCacheNode *s_cdCacheFree;        /* bss 0x7dfd90 */
static int s_cdDiscRead;                        /* bss 0x7dfd94: a lookup read the disc (tray polling on) */
static int s_cdNeedSetup;                       /* bss 0x7dfd98: next read sets the media mode up */
static unsigned char s_cdCacheBuf[32][0x800];   /* bss 0x7dfdc0 */
static unsigned char s_cdTailBuf[4 * 0x800];    /* bss 0x7efdc0: unaligned head / tail sectors */
static char s_hddRoot[0x100];                   /* bss 0x7f1dc0 */
static int s_contextIndex;                      /* bss 0x7f1ec0 */
static RxFileContext s_contexts[2];             /* bss 0x7f1ec4 */
static RxFileDevInfo s_devInfo[RX_SYS_DEVICE_NUM];  /* bss 0x7f1edc */
static RxFileThread s_threads[1];               /* bss 0x7f6910 */

// The ROM's spin-wait step: warn ("DI error") when interrupts are off.
// PORT: no Status register on the host; the wait just spins.
static void rxFileSpin(void)
{
}

// PORT: the host's file thread runs beside the game, and a callback lowers
// pending part way through (rxFileOnGetSize sets the failed bit after it), so
// a waiter that sees pending reach 0 could read the slot before the callback
// is done: fileExists took a missing data/options.txt for there and the
// FileReader that followed retried it forever (hazards.md). The callbacks run
// inside the file thread's DIntr ... EIntr; taking the section once waits the
// last one out. The PS2's waiter cannot run before the callback returns.
static void rxFileWaitCallbacks(void)
{
    DIntr();
    EIntr();
}

/* ---- async file slots ------------------------------------------------- */

static RxFileSlot *rxFileGetSlot(int fd)
{
    if ((unsigned)(fd - 1) < 0x3f && rxFileSlots[fd].used != 0)
        return &rxFileSlots[fd];
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002437f8

void rxFileClose(int fd)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot != 0)
        rxFileSlotClose(slot, 0);
}

// ──────────────────────────────────────────────────────────────────────
// Close, also cancelling the request the file thread is serving.
// ROR 0x00243858

void rxFileAbort(int fd)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot != 0)
        rxFileSlotClose(slot, 1);
}

// ──────────────────────────────────────────────────────────────────────
// 0 closed; else 1, | 4 requests pending, | 8 at the end, | 2 paused.
// ROR 0x002438b8

int rxFileGetState(int fd)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot == 0)
        return 0;
    int state = slot->pending > 0 ? 5 : 1;
    if (slot->size <= slot->pos)
        state |= 8;
    if (slot->paused != 0)
        state |= 2;
    return state;
}

// ──────────────────────────────────────────────────────────────────────
// Read up to size bytes at the position (waits for the size to be known);
// returns the count asked for, clamped to the end. Without RX_FILE_NOWAIT
// it waits for the data too.
// ROR 0x00243938

int rxFileRead(int fd, void *buf, int size)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot == 0)
        return -1;
    if ((slot->flags & SCE_RDONLY) == 0)
        return -1;
    while ((slot->state & 2) == 0)
        rxFileSpin();             // callcheck: EIntr, rxSysPrintf (the DI check)

    unsigned int count = (unsigned int)size;
    unsigned int left = slot->size - slot->pos;
    if (left <= count)
        count = left;
    if ((int)count > 0 && slot->pos < slot->size)
    {
        unsigned char *dst = (unsigned char *)buf;
        int want = (int)count;
        do
        {
            int got = rxFileSlotTransfer(slot, RX_FILE_OP_READ, dst, want);
            dst += got;
            rxFileSpin();             // callcheck: EIntr, rxSysPrintf (the DI check)
            if (want - got < 1)
                break;
            want -= got;
        } while (slot->pos < slot->size);
    }
    if ((slot->flags & RX_FILE_NOWAIT) == 0)
        rxFileSync(fd, 0);
    return (int)count;
}

// ──────────────────────────────────────────────────────────────────────
// Write size bytes at the position; the size grows to the position.
// ROR 0x00243af8

int rxFileWrite(int fd, const void *buf, int size)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot == 0)
        return -1;
    if ((slot->flags & SCE_WRONLY) == 0)
        return -1;
    unsigned char *src = (unsigned char *)buf;
    int left = size;
    if (size > 0)
    {
        do
        {
            int got = rxFileSlotTransfer(slot, RX_FILE_OP_WRITE, src, left);
            src += got;
            left -= got;
            rxFileSpin();             // callcheck: EIntr, rxSysPrintf (the DI check)
        } while (left > 0);
    }
    if ((slot->flags & RX_FILE_NOWAIT) == 0)
        rxFileSync(fd, 0);
    if (slot->size < slot->pos)
        slot->size = slot->pos;
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// Whence 0 / 1 as lseek, 2 = size - offset; returns the position.
// ROR 0x00243c40

int rxFileSeek(int fd, int offset, int whence)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot == 0)
        return -1;
    if (whence == SCE_SEEK_CUR)
        slot->pos = slot->pos + offset;
    else if (whence == SCE_SEEK_SET)
        slot->pos = offset;
    else if (whence == SCE_SEEK_END)
        slot->pos = slot->size - offset;
    return (int)slot->pos;
}

// ──────────────────────────────────────────────────────────────────────
// Wait until the slot's requests are served (noWait: just look); returns
// whether some are still pending.
// ROR 0x00243cd8

int rxFileSync(int fd, int noWait)
{
    int busy = 0;
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot != 0)
    {
        do
        {
            busy = slot->pending > 0;
            rxFileSpin();             // callcheck: EIntr, rxSysPrintf (the DI check)
        } while (busy && noWait == 0);
    }
    return busy;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00243da8

int rxFileSetMarkers(int fd, const RxFileMarkerDef *defs, int count, void *arg)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot == 0)
        return 0;
    return rxFileSlotSetMarkers(slot, defs, count, arg);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00243e08

void rxFileEnableMarker(int fd, int id, int active)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot != 0)
        rxFileSlotEnableMarker(slot, id, active);
}

// ──────────────────────────────────────────────────────────────────────
// Pause / resume the ring refill (a slot opens paused); returns the old
// setting.
// ROR 0x00243e60

int rxFilePause(int fd, int pause)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot == 0)
        return 0;
    int old = slot->paused;
    slot->paused = (unsigned char)pause;
    if ((unsigned char)pause == 0)
        rxFileSlotKick(slot);
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// The ring's data at the read offset.
// ROR 0x00243ed8

unsigned char *rxFileGetRingData(int fd)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot == 0)
        return 0;
    return slot->ring + slot->ringRead;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00243f28

int rxFileGetRingReadPos(int fd)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot == 0)
        return -1;
    return (int)slot->ringRead;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00243f70

int rxFileGetRingSize(int fd)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot == 0)
        return 0;
    return (int)slot->ringSize;
}

// ──────────────────────────────────────────────────────────────────────
// Bytes in the ring; also restarts the refill if it can run.
// ROR 0x00243fb8

int rxFileGetRingFilled(int fd)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot == 0)
        return 0;
    int filled = (int)slot->ringFilled;
    rxFileSlotKick(slot);
    return filled;
}

// ──────────────────────────────────────────────────────────────────────
// The caller used size bytes of the ring: free them for the refill.
// ROR 0x00244028

void rxFileRingConsume(int fd, int size)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot == 0)
        return;
    int filled = (int)slot->ringFilled;
    int n = size < filled ? size : filled;
    if (n < 0)
        n = 0;
    slot->ringRead = (int)(slot->ringRead + n) % (int)slot->ringSize;
    DIntr();
    slot->ringFilled -= n;
    EIntr();
    rxFileSlotDropSegments(slot, n);
    rxFileSlotKick(slot);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00244178

int rxFileGetRingFilePos(int fd, int offset)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot == 0)
        return -1;
    return rxFileSlotSegmentPos(slot, offset);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002441d0

int rxFileSetTags(int fd, const RxFileTagDef *defs, int count)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot == 0)
        return 0;
    return rxFileSlotSetTags(slot, defs, count);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00244230

void rxFileEnableTag(int fd, int id, int active)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot != 0)
        rxFileSlotEnableTag(slot, id, active);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00244288

int rxFileGetRingTags(int fd, RxFileTagHit *out, int max, int size)
{
    RxFileSlot *slot = rxFileGetSlot(fd);
    if (slot == 0)
        return 0;
    return rxFileSlotGetTags(slot, out, max, size);
}

// ──────────────────────────────────────────────────────────────────────
// Open name on the current device: its dir, its search dir and the name,
// as rxFileMakePath joins them ("/" in front makes them absolute), without
// the driver's root (the driver adds it per request).
// ROR 0x002442e8

int rxFileOpenEx(const char *name, int flags, void *ring, int ringSize, int threshold)
{
    char path[256];

    int device = rxFileGetCurrent();
    rxFileSetLastName(device, name);
    char *search = rxFileGetSearchDir(device);
    if (search[0] == '/' || name[0] == '/')
        path[0] = '\0';
    else
        strcpy(path, rxFileGetDir(device));
    if (name[0] != '/')
        strcpy(path + strlen(path), rxFileGetSearchDir(device));
    strcpy(path + strlen(path), name);
    return rxFileOpenSlot(device, path, flags, ring, ringSize, threshold);
}

// ──────────────────────────────────────────────────────────────────────
// Take a free slot and ask the file thread to create (SCE_CREAT) or size
// the file; without RX_FILE_NOWAIT wait for the answer. Returns the fd, -1
// when no slot is free or the file does not open.
// ROR 0x00244410

int rxFileOpenSlot(int device, const char *path, int flags, void *ring, int ringSize, int threshold)
{
    int fd;

    DIntr();
    for (fd = 1; fd < 64; fd++)
    {
        if (rxFileSlots[fd].used == 0)
        {
            rxFileSlots[fd].used = 1;
            break;
        }
    }
    EIntr();
    if (fd == 64)
    {
        if (s_verbose != 0)
            rxSysPrintf("rxfile : failed to alloc slot\n");
        return -1;
    }

    RxFileSlot *slot = &rxFileSlots[fd];
    strcpy(slot->path, path);
    slot->paused = 1;
    slot->ringFilled = 0;
    slot->ring = (unsigned char *)ring;
    slot->state = 0;
    slot->ringSize = ringSize;
    slot->ringThreshold = threshold;
    slot->device = (unsigned char)device;
    slot->flags = flags;
    slot->pos = 0;
    slot->size = 0;
    slot->ringWrite = 0;
    slot->ringRead = 0;
    slot->markers = 0;
    slot->markerArg = 0;
    slot->tags = 0;
    slot->pending = 0;
    slot->segments.prev = &slot->segments;
    slot->segments.next = &slot->segments;
    slot->segments.pos = 0;
    slot->segments.size = 0;

    int retry = (flags >> 15) & 1;
    if ((flags & SCE_CREAT) != 0)
    {
        // A created file is never sized: rxFileWrite grows it.
        while (rxFilePostRequest(0, device, slot->path, RX_FILE_OP_CREATE, (flags >> 10) & 1,
                                 0, 0, retry, rxFileOnCreate, slot) == 0)
            rxFileSpin();             // callcheck: EIntr, rxSysPrintf (the DI check)
    }
    else
    {
        while (rxFilePostRequest(0, device, slot->path, RX_FILE_OP_SIZE, 0, 0, 0, retry,
                                 rxFileOnGetSize, slot) == 0)
            rxFileSpin();             // callcheck: EIntr, rxSysPrintf (the DI check)
    }
    DIntr();
    slot->pending++;
    EIntr();

    if ((flags & RX_FILE_NOWAIT) != 0)
        return fd;
    while (slot->pending > 0)
        rxFileSpin();             // callcheck: EIntr, rxSysPrintf (the DI check)
    rxFileWaitCallbacks();
    if ((slot->state & 4) != 0)
    {
        slot->used = 0;
        return -1;
    }
    return fd;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002447f8

void rxFileOnCreate(RxFileSlot *slot, int offset, int result)
{
    (void)offset;
    slot->state |= 1;
    slot->pending--;
    if (result < 1)
    {
        slot->state |= 4;
        if (s_verbose != 0)
            rxSysPrintf("rxfile : failed to open file '%s'\n", slot->path);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00244868

void rxFileOnGetSize(RxFileSlot *slot, int offset, int result)
{
    (void)offset;
    slot->size = result;
    slot->state |= 2;
    slot->pending--;
    rxFileSlotKick(slot);
    if (result < 1)
    {
        slot->state |= 4;
        if (s_verbose != 0)
            rxSysPrintf("rxfile : failed to open file '%s'\n", slot->path);
    }
}

// ──────────────────────────────────────────────────────────────────────
// Cancel the slot's queued requests (abort: also the one being served),
// wait for the rest, free its segments, markers and tags.
// ROR 0x002448f8

void rxFileSlotClose(RxFileSlot *slot, int abort)
{
    rxFileCancelRequests(slot->device, slot, abort);
    while (slot->pending > 0)
        rxFileSpin();             // callcheck: EIntr, rxSysPrintf (the DI check)
    rxFileWaitCallbacks();
    rxFileSlotFreeSegments(slot);
    rxFileSlotSetMarkers(slot, 0, 0, 0);
    rxFileSlotSetTags(slot, 0, 0);
    slot->used = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Refill the ring when it may: not paused, not at the end, nothing
// pending, the size known and the ring down to its threshold.
// ROR 0x002449c0

void rxFileSlotKick(RxFileSlot *slot)
{
    if (slot->paused == 0 && slot->pos < slot->size && slot->pending == 0
        && (slot->state & 2) != 0 && slot->ringFilled <= slot->ringThreshold
        && slot->pending == 0)
        rxFileSlotRefill(slot);
}

// ──────────────────────────────────────────────────────────────────────
// A caller's read / write: queue 1.
// ROR 0x00244a38

int rxFileSlotTransfer(RxFileSlot *slot, int op, void *buf, int size)
{
    return rxFileSlotRequest(slot, 1, op, buf, size);
}

// ──────────────────────────────────────────────────────────────────────
// Ask for the free part of the ring in whole sectors at its write offset,
// split where it wraps; returns the bytes asked for.
// ROR 0x00244a68

int rxFileSlotRefill(RxFileSlot *slot)
{
    int done = 0;
    unsigned int ringSize = slot->ringSize;
    unsigned int want = (ringSize - slot->ringFilled) & 0xfffff800;
    unsigned int write = slot->ringWrite;

    if ((int)want > 0)
    {
        while (slot->pos < slot->size)
        {
            int n = (int)want - done;
            if (ringSize <= write + n)
                n = (int)(ringSize - write);
            int got = rxFileSlotRequest(slot, 0, RX_FILE_OP_READ, slot->ring + write, n);
            ringSize = slot->ringSize;
            write += got;
            done += got;
            if (ringSize <= write)
                write = 0;
            if (got < n)
                return done;
            if ((int)want <= done)
                return done;
        }
    }
    return done;
}

// ──────────────────────────────────────────────────────────────────────
// Post a transfer in pieces that end at markers: an active marker inside
// the next piece whose pass limit is not reached stops it when its callback
// agrees (no callback: always). After each piece the markers it reached
// count a pass (past the limit they reset), and a stop at a marker jumps
// the position to the marker's target (-1: the end). Reads stop at the end.
// ROR 0x00244b40

int rxFileSlotRequest(RxFileSlot *slot, int queue, int op, void *buf, int size)
{
    int done = 0;
    if (size <= 0)
        return 0;

    unsigned int start = slot->pos;
    if (op != RX_FILE_OP_WRITE && slot->size <= start)
        return 0;

    for (;;)
    {
        int n = size - done;
        int stop = 0;
        for (RxFileMarker *m = slot->markers; m != 0; m = m->next)
        {
            RxFileMarkerDef *d = &m->def;
            if (d->active == 0 || d->pos < slot->pos || slot->pos + n < d->pos)
                continue;
            if (d->count < d->max || d->max == 0)
            {
                stop = d->callback != 0 ? d->callback(slot->markerArg, d) : 1;
                if (stop != 0)
                {
                    n = (int)(d->pos - slot->pos);
                    break;
                }
            }
        }

        int got = rxFileSlotRequestChunked(slot, queue, op, (unsigned char *)buf + done, n);
        done += got;
        for (RxFileMarker *m = slot->markers; m != 0; m = m->next)
        {
            RxFileMarkerDef *d = &m->def;
            if (d->active == 0 || d->pos < start || start + got < d->pos)
                continue;
            if (d->count < d->max || d->max == 0)
            {
                d->count++;
                if (d->pos == start + got && stop != 0)
                    slot->pos = d->jumpPos != -1 ? (unsigned int)d->jumpPos : slot->size;
            }
            else
            {
                d->count = 0;
            }
        }

        if (got < n)
            return done;
        if (size <= done)
            return done;
        start = slot->pos;
        if (op != RX_FILE_OP_WRITE && slot->size <= start)
            return done;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Refill reads stop at the end; a caller's read is posted in pieces of
// the device param's size when it has one.
// ROR 0x00244d70

int rxFileSlotRequestChunked(RxFileSlot *slot, int queue, int op, void *buf, int size)
{
    if (queue == 0 && slot->size < slot->pos + size)
        size = (int)(slot->size - slot->pos);
    int chunk = rxFileGetDeviceParam(slot->device);
    if (queue == 1 && op == RX_FILE_OP_READ && chunk > 0)
    {
        int done = 0;
        while (done < size)
        {
            if (slot->size <= slot->pos)
                break;
            int n = size - done;
            if (chunk <= n)
                n = chunk;
            int got = rxFileSlotPost(slot, 1, RX_FILE_OP_READ, (unsigned char *)buf + done, n);
            done += got;
            if (got < n)
                return done;
        }
        return done;
    }
    return rxFileSlotPost(slot, queue, op, buf, size);
}

// ──────────────────────────────────────────────────────────────────────
// Queue one transfer at the position and move the position past it;
// 0 when the queue is full.
// ROR 0x00244f20

int rxFileSlotPost(RxFileSlot *slot, int queue, int op, void *buf, int size)
{
    if (size <= 0)
        return 0;
    if (rxFilePostRequest(queue, slot->device, slot->path, op, (int)slot->pos, buf, size, 1,
                          rxFileOnTransfer, slot) == 0)
        return 0;
    slot->pos += size;
    DIntr();
    slot->pending++;
    EIntr();
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// A transfer was served: in ring mode the bytes join the ring.
// ROR 0x00245000

void rxFileOnTransfer(RxFileSlot *slot, int offset, int result)
{
    if (result > 0 && (slot->flags & RX_FILE_RING) != 0)
    {
        slot->ringFilled += result;
        slot->ringWrite = (slot->ringWrite + result) % slot->ringSize;
        rxFileSlotAddSegment(slot, offset, result);
    }
    slot->pending--;
}

// ──────────────────────────────────────────────────────────────────────
// Replace the slot's markers with count defs, kept sorted by position
// (as many as the pool has); returns how many were taken.
// ROR 0x00245078

int rxFileSlotSetMarkers(RxFileSlot *slot, const RxFileMarkerDef *defs, int count, void *arg)
{
    int taken = 0;

    DIntr();
    RxFileMarker *m;
    while ((m = slot->markers) != 0)
    {
        slot->markers = m->next;
        m->next = rxFileMarkerFree;
        rxFileMarkerFree = m;
    }
    for (int i = 0; i < count && rxFileMarkerFree != 0; i++)
    {
        RxFileMarker *node = rxFileMarkerFree;
        rxFileMarkerFree = node->next;
        RxFileMarker **link = &slot->markers;
        RxFileMarker *p = *link;
        if (p == 0 || defs[i].pos < p->def.pos)
        {
            node->next = p;
        }
        else
        {
            RxFileMarker *prev;
            do
            {
                prev = p;
                p = p->next;
            } while (p != 0 && p->def.pos <= defs[i].pos);
            node->next = p;
            link = &prev->next;
        }
        *link = node;
        taken++;
        node->def = defs[i];
    }
    EIntr();
    slot->markerArg = arg;
    return taken;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00245240

void rxFileSlotEnableMarker(RxFileSlot *slot, int id, int active)
{
    for (RxFileMarker *m = slot->markers; m != 0; m = m->next)
    {
        if (m->def.id == (unsigned int)id)
            m->def.active = (unsigned char)active;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Append a ring segment (dropped when the pool is empty).
// ROR 0x00245280

void rxFileSlotAddSegment(RxFileSlot *slot, int pos, int size)
{
    DIntr();
    RxFileSegment *seg = rxFileSegmentFree;
    if (seg != 0)
    {
        rxFileSegmentFree = seg->next;
        seg->pos = pos;
        seg->size = size;
        RxFileSegment *tail = slot->segments.prev;
        seg->next = &slot->segments;
        seg->prev = tail;
        tail->next = seg;
        seg->next->prev = seg;
    }
    EIntr();
}

// ──────────────────────────────────────────────────────────────────────
// Remove size bytes from the front of the segments.
// ROR 0x00245348

void rxFileSlotDropSegments(RxFileSlot *slot, int size)
{
    DIntr();
    RxFileSegment *end = &slot->segments;
    RxFileSegment *seg = end->next;
    if (seg != end)
    {
        int len = seg->size;
        while (len <= size)
        {
            RxFileSegment *next = seg->next;
            seg->prev->next = next;
            next->prev = seg->prev;
            seg->next = rxFileSegmentFree;
            rxFileSegmentFree = seg;
            size -= len;
            seg = next;
            if (seg == end)
            {
                EIntr();
                return;
            }
            len = seg->size;
        }
        seg->size = len - size;
        seg->pos = seg->pos + size;
    }
    EIntr();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00245468

void rxFileSlotFreeSegments(RxFileSlot *slot)
{
    DIntr();
    RxFileSegment *seg = slot->segments.next;
    while (seg != &slot->segments)
    {
        RxFileSegment *next = seg->next;
        seg->prev->next = next;
        next->prev = seg->prev;
        seg->next = rxFileSegmentFree;
        rxFileSegmentFree = seg;
        seg = next;
    }
    EIntr();
}

// ──────────────────────────────────────────────────────────────────────
// File position of the byte offset bytes past the ring read offset; -1
// past the segments.
// ROR 0x00245520

int rxFileSlotSegmentPos(RxFileSlot *slot, int offset)
{
    int pos = -1;

    DIntr();
    RxFileSegment *end = &slot->segments;
    for (RxFileSegment *seg = end->next; seg != end; seg = seg->next)
    {
        if (offset < seg->size)
        {
            pos = (int)seg->pos + offset;
            break;
        }
        offset -= seg->size;
    }
    EIntr();
    return pos;
}

// ──────────────────────────────────────────────────────────────────────
// Replace the slot's tags with count defs, kept sorted by position.
// ROR 0x00245600

int rxFileSlotSetTags(RxFileSlot *slot, const RxFileTagDef *defs, int count)
{
    int taken = 0;

    DIntr();
    RxFileTag *t;
    while ((t = slot->tags) != 0)
    {
        slot->tags = t->next;
        t->next = rxFileTagFree;
        rxFileTagFree = t;
    }
    for (int i = 0; i < count && rxFileTagFree != 0; i++)
    {
        RxFileTag *node = rxFileTagFree;
        rxFileTagFree = node->next;
        RxFileTag **link = &slot->tags;
        RxFileTag *p = *link;
        if (p == 0 || defs[i].pos < p->def.pos)
        {
            node->next = p;
        }
        else
        {
            RxFileTag *prev;
            do
            {
                prev = p;
                p = p->next;
            } while (p != 0 && p->def.pos <= defs[i].pos);
            node->next = p;
            link = &prev->next;
        }
        *link = node;
        taken++;
        node->def = defs[i];
    }
    EIntr();
    return taken;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002457a8

void rxFileSlotEnableTag(RxFileSlot *slot, int id, int active)
{
    for (RxFileTag *t = slot->tags; t != 0; t = t->next)
    {
        if (t->def.id == (unsigned int)id)
            t->def.active = (unsigned char)active;
    }
}

// ──────────────────────────────────────────────────────────────────────
// List the active tags inside the first size bytes of the ring (by its
// segments): their offset past the read offset and value, at most max
// stored; returns how many there are.
// ROR 0x002457e8

int rxFileSlotGetTags(RxFileSlot *slot, RxFileTagHit *out, int max, int size)
{
    RxFileSegment *end = &slot->segments;
    RxFileSegment *seg = end->next;
    int base = 0;
    unsigned int found = 0;

    if (seg == end || size < 1)
        return 0;
    for (;;)
    {
        unsigned int lo = seg->pos;
        int len = seg->size;
        unsigned int hi = lo + size;
        if (lo + len <= lo + size)
            hi = lo + len;
        for (RxFileTag *t = slot->tags; t != 0; t = t->next)
        {
            if (t->def.active == 0)
                continue;
            unsigned int p = t->def.pos;
            if (p < lo || hi <= p)
                continue;
            if (found < (unsigned int)max)
            {
                out[found].offset = base + (int)(p - lo);
                out[found].value = t->def.value;
            }
            found++;
        }
        seg = seg->next;
        size -= len;
        base += len;
        if (seg == end || size < 1)
            return (int)found;
    }
}

/* ---- the file layer --------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Set a device driver's root (-1 = the boot device): rxSysInit hands the
// boot path's device part here.
// ROR 0x002458b0

void rxFileCallDevice(int device, const char *root)
{
    if (device == -1)
        device = rxSysGetBootDevice();
    RxFileDriver *drv = rxFileGetDriver(device);
    if (drv != 0)
        drv->setRoot(root);     // callcheck: rxFileHostSetRoot (driver slot 7)
}

// ──────────────────────────────────────────────────────────────────────
// A device driver's root (-1 = the boot device).
// ROR 0x00245900

char *rxFileGetDeviceRoot(int device)
{
    if (device == -1)
        device = rxSysGetBootDevice();
    RxFileDriver *drv = rxFileGetDriver(device);
    if (drv != 0)
        return drv->getRoot();  // callcheck: rxFileHostGetRoot (driver slot 8)
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Load a whole file through the current context's load hook.
// ROR 0x00245948

int rxFileCallLoadHook(const char *name, void *buf)
{
    RxFileLoadHook hook = rxFileGetLoadHook();
    return hook(name, buf);
}

// ──────────────────────────────────────────────────────────────────────
// A file's size through the current context's size hook.
// ROR 0x00245988

int rxFileCallSizeHook(const char *name)
{
    RxFileSizeHook hook = rxFileGetSizeHook();
    return hook(name);
}

// ──────────────────────────────────────────────────────────────────────
// Install the current context's load hook (null = the default).
// ROR 0x002459b8

RxFileLoadHook rxFileSetLoadHook(RxFileLoadHook hook)
{
    RxFileLoadHook old = rxFileGetLoadHook();
    if (hook == 0)
        hook = rxFileDefaultLoadHook;
    rxFileSwapLoadHook(hook);
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// Install the current context's size hook (null = the default).
// ROR 0x00245a00

RxFileSizeHook rxFileSetSizeHook(RxFileSizeHook hook)
{
    RxFileSizeHook old = rxFileGetSizeHook();
    if (hook == 0)
        hook = rxFileDefaultSizeHook;
    rxFileSwapSizeHook(hook);
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// A file's size through a slot; -2 when it does not open.
// ROR 0x00245a48

int rxFileDefaultSizeHook(const char *name)
{
    int size = -2;
    if (name == 0)
        return -2;
    FlushCache(0);
    int fd = rxSysStreamOpen(name, SCE_RDONLY);
    if (fd != -1)
    {
        size = rxFileSeek(fd, 0, SCE_SEEK_END);
        rxFileClose(fd);
    }
    if (s_verbose != 0)
        rxSysPrintf("[seek]: %s (%dBytes)\n", name, size);
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// Load a whole file through a slot; its size, -2 when it does not open,
// -1 when it does not read whole.
// ROR 0x00245ae8

int rxFileDefaultLoadHook(const char *name, void *buf)
{
    int size = -2;
    if (name == 0)
        return -2;
    FlushCache(0);
    int fd = rxSysStreamOpen(name, SCE_RDONLY);
    if (fd != -1)
    {
        size = rxFileSeek(fd, 0, SCE_SEEK_END);
        rxFileSeek(fd, 0, SCE_SEEK_SET);
        if (rxFileRead(fd, buf, size) == size)
        {
            rxFileClose(fd);
            FlushCache(0);
        }
        else
        {
            size = -1;
            rxFileClose(fd);
        }
    }
    if (s_verbose != 0)
        rxSysPrintf("[read]: %s $%X (%dBytes)\n", name, (unsigned int)(size_t)buf, size);
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// Write a whole file (created, truncated) through a slot; size, or -1.
// ROR 0x00245be0

int rxFileDefaultSaveHook(const char *name, const void *buf, int size)
{
    FlushCache(0);
    int fd = rxSysStreamOpen(name, SCE_WRONLY | SCE_CREAT | SCE_TRUNC);
    if (fd == -1)
    {
        size = -1;
    }
    else
    {
        if (rxFileWrite(fd, buf, size) != size)
            size = -1;
        rxFileClose(fd);
    }
    if (s_verbose != 0)
        rxSysPrintf("[write]: %s $%X (%dBytes)\n", name, (unsigned int)(size_t)buf, size);
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// Build a device path: the current device's dir, then its search dir, then
// the name ("/" in front of the search dir or the name makes it absolute),
// and the driver puts its root in front.
// ROR 0x00245cb0

int rxFileMakePath(char *out, const char *name)
{
    char path[256];

    if (name == 0)
        return 0;

    RxFileDriver *drv = rxFileGetDriver(rxFileGetCurrent());
    path[0] = '\0';
    char *search = rxFileGetSearchDir(rxFileGetCurrent());
    if (search[0] != '/')
    {
        if (name[0] == '/')
            goto join;
        strcpy(path, rxFileGetDir(rxFileGetCurrent()));
    }
    if (name[0] != '/')
        strcpy(path + strlen(path), rxFileGetSearchDir(rxFileGetCurrent()));
join:
    strcpy(path + strlen(path), name);

    // The ROM reads the slot without checking the driver: MCARD (no driver)
    // is refused by rxFileSetDevice, so the current device always has one.
    void (*makePath)(char *, const char *) = drv->makePath;
    if (makePath != 0)
    {
        makePath(out, path);    // callcheck: rxFileHostMakePath (driver slot 0)
        FlushCache(0);
    }
    return makePath != 0;
}

// ──────────────────────────────────────────────────────────────────────
// Run another ELF from the current device (checks that it opens first);
// returns only when it does not: -2.
// ROR 0x00245db8

int rxFileLoadExec(const char *path, int argc, char **argv)
{
    char full[256];

    if (rxFileMakePath(full, path) == 0)
        return -2;
    int fd = rxSysOpen(full, SCE_RDONLY);
    if (fd < 0)
    {
        if (s_verbose != 0)
            rxSysPrintf("rxfile : Failed to open file '%s'\n", full);
        return -2;
    }
    rxSysClose(fd);
    rxSysPrintf("LoadExecPS2:%s\n", full);
    rxSysPrintf("argc = %d\n", argc);
    for (int i = 0; i < argc; i++)
        rxSysPrintf("argv[%d] =%s\n", i, argv[i]);
    sceSifExitCmd();
    LoadExecPS2(full, argc, argv);
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// Load a whole file into buf; returns its size, -2 when it does not open,
// -1 when it does not read. Reads 0x80 bytes more than the size.
// ROR 0x00245ed8

int rxFileLoad(const char *name, void *buf)
{
    char path[256];
    const char *msg;

    if (rxFileMakePath(path, name) == 0)
        return -2;

    int fd = rxSysOpen(path, SCE_RDONLY);
    if (fd < 0)
    {
        if (s_verbose != 0)
            rxSysPrintf("rxfile : Failed to open file '%s'\n", path);
        return -2;
    }

    int size = rxSysLseek(fd, 0, SCE_SEEK_END);
    if (size < 0)
    {
        rxSysClose(fd);
        if (s_verbose == 0)
            return -1;
        msg = "rxfile : Failed to seek file '%s'\n";
    }
    else
    {
        rxSysLseek(fd, 0, SCE_SEEK_SET);
        int got = rxSysRead(fd, buf, size + 0x80);
        if (got >= 0)
        {
            rxSysClose(fd);
            FlushCache(0);
            if (s_verbose != 0)
                rxSysPrintf("rxfile : Load file   '%s' (%d [%d] Bytes)\n", path, size, got);
            return size;
        }
        rxSysClose(fd);
        if (s_verbose == 0)
            return -1;
        msg = "rxfile : Failed to load file '%s'\n";
    }
    rxSysPrintf(msg, path);
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// Load an IOP module (args: arglen bytes of NUL-separated strings); 1 on
// success. Opens the file first to report a missing module by name.
// ROR 0x00246030

int rxFileLoadModule(const char *path, int arglen, const char *args)
{
    char full[256];
    const char *msg;

    if (rxFileMakePath(full, path) == 0)
        return 0;

    int fd = rxSysOpen(full, SCE_RDONLY);
    if (fd < 0)
    {
        if (s_verbose == 0)
            return 0;
        msg = "rxfile : Failed to open module '%s'\n";
    }
    else
    {
        rxSysClose(fd);
        if (s_verbose != 0)
            rxSysPrintf("rxfile : Load module '%s'\n", full);
        int n = 0;
        int pos = 0;
        while (pos < arglen)
        {
            const char *arg = args + pos;
            if (s_verbose != 0)
                rxSysPrintf("  arg[%d] '%s'\n", n, arg);
            n++;
            pos += (int)strlen(arg) + 1;
        }
        if (sceSifLoadModule(full, arglen, args) >= 0)
            return 1;
        if (s_verbose == 0)
            return 0;
        msg = "rxfile : Failed to load module '%s'\n";
    }
    rxSysPrintf(msg, full);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Load count modules without arguments; 0 at the first failure.
// ROR 0x00246178

int rxFileLoadModuleList(const char *const *names, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (rxFileLoadModule(names[i], 0, "") == 0)
            return 0;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Reboot the IOP with an IOPRP image and wait for it; 1 on success.
// ROR 0x002461f0

int rxFileRebootIop(const char *img)
{
    char path[256];

    if (rxFileMakePath(path, img) == 0)
        return 0;
    if (sceSifRebootIop(path) == 0)
    {
        if (s_verbose != 0)
            rxSysPrintf("rxfile : Failed to Reboot Iop '%s'\n", path);
        return 0;
    }
    while (sceSifSyncIop() == 0)
    {
    }
    if (s_verbose != 0)
        rxSysPrintf("rxfile : Reboot Iop '%s'\n", path);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Fill the three device drivers.
// ROR 0x00246278

void rxFileInit(void)
{
    rxFileHostInit(&s_drivers[RX_SYS_DEVICE_HOST]);
    rxFileCdInit(&s_drivers[RX_SYS_DEVICE_CDROM]);
    rxFileHddInit(&s_drivers[RX_SYS_DEVICE_HDD]);
}

// ──────────────────────────────────────────────────────────────────────
// The drivers' (empty) terms.
// ROR 0x002462b0

void rxFileTerm(void)
{
    rxFileHostTerm();
    rxFileCdTerm();
    rxFileHddTerm();
}

// ──────────────────────────────────────────────────────────────────────
// A device's driver: HOST, CDROM and HDD have one, MCARD none.
// ROR 0x002462d8

RxFileDriver *rxFileGetDriver(int device)
{
    if (device >= 0 && (device < RX_SYS_DEVICE_MCARD || device == RX_SYS_DEVICE_HDD))
        return &s_drivers[device];
    return 0;
}

/* ---- host0: driver ---------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00246318

void rxFileHostInit(RxFileDriver *drv)
{
    drv->makePath  = rxFileHostMakePath;
    drv->setRoot   = rxFileHostSetRoot;
    drv->control   = rxFileHostControl;
    drv->getStatus = rxFileHostGetStatus;
    drv->getSize   = rxFileHostGetSize;
    drv->create    = rxFileHostCreate;
    drv->read      = rxFileHostRead;
    drv->write     = rxFileHostWrite;
    drv->getRoot   = rxFileHostGetRoot;
    rxFileHostSetRoot(0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002463a0

void rxFileHostTerm(void)
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002463a8

void rxFileHostMakePath(char *out, const char *name)
{
    rxFileHostJoinRoot(out, name);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002463c0

int rxFileHostControl(int cmd, int arg)
{
    (void)cmd;
    (void)arg;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Status 1: busy in a file call.
// ROR 0x002463c8

int rxFileHostGetStatus(int what)
{
    if (what == 1)
        return s_hostFlags & 1;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002463f0

int rxFileHostGetSize(const char *name)
{
    char path[256];
    int size;

    s_hostFlags |= 1;
    rxFileHostJoinRoot(path, name);
    int fd = rxSysOpen(path, SCE_RDONLY);
    if (fd < 0)
    {
        if (s_verbose != 0)
            rxSysPrintf("rxfile : failed to open file '%s'\n", path);
        size = -2;
    }
    else
    {
        size = rxSysLseek(fd, 0, SCE_SEEK_END);
        rxSysClose(fd);
    }
    s_hostFlags &= ~1;
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// Create a file (truncate: also SCE_TRUNC); 1 on success.
// ROR 0x002464b0

int rxFileHostCreate(const char *name, int truncate)
{
    char path[256];
    int ret;

    s_hostFlags |= 1;
    rxFileHostJoinRoot(path, name);
    int mode = SCE_WRONLY | SCE_CREAT | SCE_TRUNC;
    if (truncate == 0)
        mode = SCE_WRONLY | SCE_CREAT;
    int fd = rxSysOpen(path, mode);
    if (fd < 0)
    {
        if (s_verbose != 0)
            rxSysPrintf("rxfile : failed to open file '%s'\n", path);
        ret = -2;
    }
    else
    {
        rxSysClose(fd);
        ret = 1;
    }
    s_hostFlags &= ~1;
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// Read size bytes at offset; returns size, -1 on a short read, -2 when the
// file does not open.
// ROR 0x00246560

int rxFileHostRead(const char *name, void *buf, int offset, int size)
{
    char path[256];

    s_hostFlags |= 1;
    rxFileHostJoinRoot(path, name);
    FlushCache(0);
    int fd = rxSysOpen(path, SCE_RDONLY);
    if (fd < 0)
    {
        if (s_verbose != 0)
            rxSysPrintf("rxfile : failed to open file '%s'\n", path);
        size = -2;
    }
    else
    {
        rxSysLseek(fd, offset, SCE_SEEK_SET);
        if (rxSysRead(fd, buf, size) < 1)
        {
            rxSysClose(fd);
            size = -1;
        }
        else
        {
            rxSysClose(fd);
            FlushCache(0);
        }
    }
    s_hostFlags &= ~1;
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// Write size bytes at offset; same results as rxFileHostRead.
// ROR 0x00246680

int rxFileHostWrite(const char *name, int offset, const void *buf, int size)
{
    char path[256];

    s_hostFlags |= 1;
    rxFileHostJoinRoot(path, name);
    FlushCache(0);
    int fd = rxSysOpen(path, SCE_WRONLY);
    if (fd < 0)
    {
        if (s_verbose != 0)
            rxSysPrintf("rxfile : failed to open file '%s'\n", path);
        size = -2;
    }
    else
    {
        rxSysLseek(fd, offset, SCE_SEEK_SET);
        if (rxSysWrite(fd, buf, size) < 1)
        {
            rxSysClose(fd);
            size = -1;
        }
        else
        {
            rxSysClose(fd);
            FlushCache(0);
        }
    }
    s_hostFlags &= ~1;
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002467a0

void rxFileHostJoinRoot(char *out, const char *name)
{
    strcpy(out, s_hostRoot);
    strcpy(out + strlen(out), name);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002467e8

char *rxFileHostGetRoot(void)
{
    return s_hostRoot;
}

// ──────────────────────────────────────────────────────────────────────
// null = "host0:".
// ROR 0x002467f8

void rxFileHostSetRoot(const char *root)
{
    if (root == 0)
    {
        strcpy(s_hostRoot, s_hostRootDefault);
        return;
    }
    strcpy(s_hostRoot, root);
}

/* ---- cdrom0: driver --------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Give the other rx threads a turn while the drive works.
// ROR 0x00246840

void rxFileCdWait(void)
{
    rxFileSpin();             // callcheck: EIntr, rxSysPrintf (the DI check)
    rxThYield();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00246890

void rxFileCdSyncWait(void)
{
    while (sceCdSync(1) != 0)
        rxFileCdWait();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002468c0

void rxFileCdInit(RxFileDriver *drv)
{
    drv->makePath  = rxFileCdMakePath;
    drv->setRoot   = rxFileCdSetRoot;
    drv->getStatus = rxFileCdGetStatus;
    drv->control   = rxFileCdControl;
    drv->getSize   = rxFileCdGetSize;
    drv->create    = rxFileCdCreate;
    drv->read      = rxFileCdRead;
    drv->write     = rxFileCdWrite;
    drv->getRoot   = rxFileCdGetRoot;
    rxFileCdSetRoot(0);
    rxFileCdInitCache();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00246950

void rxFileCdTerm(void)
{
}

// ──────────────────────────────────────────────────────────────────────
// The drive status once it answers.
// ROR 0x00246958

int rxFileCdStatusWait(void)
{
    int status;
    while ((status = sceCdStatus()) < 0)
        rxFileCdWait();
    return status;
}

// ──────────────────────────────────────────────────────────────────────
// Open (0) or close (1) the tray.
// ROR 0x00246988

void rxFileCdTray(int req)
{
    u_int count[4];
    sceCdTrayReq(req, count);
}

// ──────────────────────────────────────────────────────────────────────
// The file thread's work before every request: once a lookup read the
// disc, poll the tray every 0x400 steps (open: drop the cache, flag it);
// serve tray requests; read the drive's clock every s_cdClockInterval
// ticks while asked to.
// ROR 0x002469a8

void rxFileCdProcess(int step)
{
    if (s_cdDiscRead != 0)
    {
        s_cdTrayTimer += step;
        if (s_cdTrayTimer > 0x400)
        {
            if (rxFileCdStatusWait() == SCECdStatShellOpen)
            {
                rxFileCdFlushCache();
                s_cdFlags |= 1;
            }
            else
            {
                s_cdFlags &= ~1;
            }
            s_cdTrayTimer = 0;
        }
    }

    if ((s_cdTrayReq & 1) != 0)
    {
        DIntr();
        s_cdTrayReq &= ~1;
        EIntr();
        rxFileCdTray(0);
        rxFileCdSyncWait();
    }
    if ((s_cdTrayReq & 2) != 0)
    {
        DIntr();
        s_cdTrayReq &= ~2;
        EIntr();
        rxFileCdTray(1);
        rxFileCdSyncWait();
    }

    int tick = s_cdClockTick;
    if (s_cdClockRead != 0)
    {
        int now = rxThSync(-1);
        if (s_cdClockTick != now || s_cdClockValid == 0)
        {
            tick = now;
            s_cdClockCountdown--;
            sceCdCLOCK clock;
            if (s_cdClockCountdown < 0 && sceCdReadClock(&clock) != 0)
            {
                DIntr();
                s_cdClock = clock;
                EIntr();
                s_cdClockValid = 1;
                s_cdClockCountdown = s_cdClockInterval;
            }
        }
    }
    s_cdClockTick = tick;
}

// ──────────────────────────────────────────────────────────────────────
// "cdrom0:" and the ISO 9660 name.
// ROR 0x00246c70

void rxFileCdMakePath(char *out, const char *name)
{
    strcpy(out, s_cdRoot);
    rxFileCdIsoName(out + strlen(out), name);
}

// ──────────────────────────────────────────────────────────────────────
// Cmd 0: ask the file thread to open (arg != 0) or close the tray.
// ROR 0x00246cb8

int rxFileCdControl(int cmd, int arg)
{
    if (cmd == 0)
    {
        DIntr();
        if (arg == 0)
            s_cdTrayReq = (s_cdTrayReq & ~1) | 2;
        else
            s_cdTrayReq = (s_cdTrayReq & ~2) | 1;
        EIntr();
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Status 0: the tray was found open; 1: reading; 2: byte position of the
// last read.
// ROR 0x00246d80

int rxFileCdGetStatus(int what)
{
    if (what == 1)
        return (s_cdFlags >> 1) & 1;
    if (what > 1)
    {
        if (what == 2)
            return s_cdReadPos + s_cdReadPos2;
        return 0;
    }
    if (what != 0)
        return 0;
    return s_cdFlags & 1;
}

// ──────────────────────────────────────────────────────────────────────
// A file's size; -2 when it is not on the disc, -1 on errors.
// ROR 0x00246e00

int rxFileCdGetSize(const char *name)
{
    int lsn;
    int size;
    int r = s_cdFind(name, &lsn, &size);
    if (r == -2)
    {
        rxFileCdFlushCache();
        return -1;
    }
    if (r < -1)
        return r == -3 ? -2 : -1;
    if (r == 1)
        return size;
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// The disc is read-only.
// ROR 0x00246e78

int rxFileCdCreate(const char *name, int truncate)
{
    (void)name;
    (void)truncate;
    return -2;
}

// ──────────────────────────────────────────────────────────────────────
// Read size bytes at offset of a file; size, -2 when it is not on the
// disc, -1 on errors.
// ROR 0x00246e80

int rxFileCdRead(const char *name, void *buf, int offset, int size)
{
    int lsn;
    int fileSize;
    int r = s_cdFind(name, &lsn, &fileSize);
    if (r == -2)
    {
        rxFileCdFlushCache();
        return -1;
    }
    if (r < -1)
        return r == -3 ? -2 : -1;
    if (r != 1)
        return -1;
    r = rxFileCdReadAt(buf, (long long)(unsigned int)lsn * 0x800 + (unsigned int)offset, size);
    if (r < 0 || r != 1)
        return -1;
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// The disc is read-only.
// ROR 0x00246f68

int rxFileCdWrite(const char *name, int offset, const void *buf, int size)
{
    (void)name;
    (void)offset;
    (void)buf;
    (void)size;
    return -2;
}

// ──────────────────────────────────────────────────────────────────────
// ISO 9660 spelling: "\" in front unless absolute, "\" separators, upper
// case, ";1".
// ROR 0x00246f70

void rxFileCdIsoName(char *out, const char *name)
{
    out[0] = '\0';
    if (name[0] != '/')
        strcpy(out, s_cdIsoPrefix);
    strcpy(out + strlen(out), name);
    for (char *p = out; *p != '\0'; p++)
    {
        if (*p == '/')
            *p = '\\';
        else if ((unsigned char)(*p - 'a') < 26)
            *p = (char)(*p - 0x20);
    }
    strcpy(out + strlen(out), s_cdIsoSuffix);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00247048

int rxFileCdMapSector(int lsn)
{
    if (s_cdDual != 0 && s_cdLayer == 1)
        lsn += s_cdLayer1Start;
    return lsn;
}

// ──────────────────────────────────────────────────────────────────────
// Read size bytes at a disc offset: the whole sectors straight into the
// buffer from its first 64-byte boundary, moved down to where they belong,
// then the sectors left through s_cdTailBuf. Returns the last sector
// read's result.
// ROR 0x00247080

int rxFileCdReadBytes(void *buf, long long offset, int size)
{
    unsigned char *dst = (unsigned char *)buf;
    unsigned int head = (0u - ((unsigned int)(size_t)dst & 0x3f)) & 0x3f;
    unsigned char *aligned = dst + head;
    int skip = (int)(offset & 0x7ff);
    long long base = offset & ~0x7ffLL;
    int body = size - (int)head;
    int bulk = body < 1 ? 0 : (body & ~0x7ff);
    long long end = base + bulk;
    int done = 0;
    int from = skip;
    int r = 1;

    s_cdFlags |= 2;
    if (bulk > 0)
    {
        r = rxFileCdReadSectors(aligned, (int)(base / 0x800), bulk >> 11);
        if (r < 1)
            goto out;
        if (head != 0 || skip != 0)
        {
            memmove(dst, aligned + skip, (size_t)(bulk - skip));
            done = bulk - skip;
        }
        else
        {
            done = bulk;
        }
        from = 0;
    }
    if (done < size)
    {
        long long rest = ((size + offset + 0x7ff) & ~0x7ffLL) - end;
        r = rxFileCdReadSectors(s_cdTailBuf, (int)(end / 0x800), (int)(rest / 0x800));
        if (r > 0)
            memcpy(dst + done, s_cdTailBuf + from, (size_t)(size - done));
    }
out:
    s_cdFlags &= ~2;
    return r;
}

// ──────────────────────────────────────────────────────────────────────
// A read at the same offset as the last one first reads the sector 32
// before it (the drive then really seeks: how a failed read is retried).
// ROR 0x00247308

int rxFileCdReadAt(void *buf, long long offset, int size)
{
    int sector = (int)(offset / 0x800) - 0x20;
    if (s_cdLastSector == offset)
    {
        int r = rxFileCdReadSectors(s_cdTailBuf, sector < 0 ? 0 : sector, 1);
        if (r < 0)
            return r;
    }
    s_cdLastSector = offset;
    return rxFileCdReadBytes(buf, offset, size);
}

// ──────────────────────────────────────────────────────────────────────
// Read count sectors. The first read after a cache reset waits for the
// disc and sets the media mode and the dual-layer info. 1 read, -2 the tray
// opened or the disc went (cache dropped), -1 a read error.
// ROR 0x002473c8

int rxFileCdReadSectors(void *buf, int lsn, int count)
{
    sceCdRMode mode;
    int err;

    mode.trycount = 0;
    mode.spindlctrl = 0;
    mode.datapattern = 0;
    FlushCache(0);
    if (s_cdNeedSetup != 0)
    {
        while (sceCdDiskReady(SCECdNonblock) != SCECdComplete)
        {
            if (rxFileCdStatusWait() == SCECdStatShellOpen)
                goto changed;
            rxFileCdWait();
        }
        int type;
        while ((type = sceCdGetDiskType()) == SCECdDETCT)
            rxFileCdWait();
        rxSysPrintf("rxfile : disk type = %d\n", type);
        if (type == SCECdPS2DVD || type == SCECdDVDV)
            sceCdMmode(SCECdDVD);
        else
            sceCdMmode(SCECdCD);
        s_cdDual = 0;
        s_cdLayer1Start = 0;
        if (sceCdReadDvdDualInfo(&s_cdDual, &s_cdLayer1Start) == 0)
        {
            err = sceCdGetError();
            if (err >= 0x13)
            {
                if (err == SCECdErTRMOPN)
                    goto changed;
                goto error;
            }
            if (err > 0x10)
                goto changed;
            if (err != SCECdErNO)
                goto error;
        }
        if (s_cdDual != 0)
            rxSysPrintf("rxfile : dual mode\n");
        s_cdNeedSetup = 0;
    }

    s_cdReadPos = lsn << 11;
    while (sceCdRead(rxFileCdMapSector(lsn), count, buf, &mode) == 0)
    {
        if (rxFileCdStatusWait() == SCECdStatShellOpen)
            goto changed;
        rxFileCdWait();
    }
    rxFileCdSyncWait();
    FlushCache(0);
    err = sceCdGetError();
    if (err < 0x13)
    {
        if (err > 0x10)             // SCECdErOPENS, SCECdErNODISC
            goto changed;
        if (err == SCECdErNO)
            return 1;
        goto error;
    }
    if (err == SCECdErTRMOPN)
        goto changed;
error:
    rxSysPrintf("*** read error retry\n");
    return -1;
changed:
    rxFileCdFlushCache();
    return -2;
}

// ──────────────────────────────────────────────────────────────────────
// Put the 32 sector buffers on the free list, empty the used list.
// ROR 0x00247608

void rxFileCdInitCache(void)
{
    s_cdCacheFree = 0;
    for (int i = 0; i < 32; i++)
    {
        RxFileCdCacheNode *node = &s_cdCacheNodes[i];
        node->buf = s_cdCacheBuf[i];
        node->next = s_cdCacheFree;
        s_cdCacheFree = node;
    }
    s_cdNeedSetup = 1;
    s_cdCacheUsed.next = &s_cdCacheUsed;
    s_cdCacheUsed.prev = &s_cdCacheUsed;
    s_cdDiscRead = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Return every used sector buffer to the free list.
// ROR 0x00247668

void rxFileCdFlushCache(void)
{
    s_cdNeedSetup = 1;
    s_cdDiscRead = 0;
    RxFileCdCacheNode *node;
    while ((node = s_cdCacheUsed.next) != &s_cdCacheUsed)
    {
        s_cdCacheUsed.next = node->next;
        node->next = s_cdCacheFree;
        s_cdCacheFree = node;
    }
    s_cdCacheUsed.next = &s_cdCacheUsed;
    s_cdLastSector = -1;
    s_cdCacheUsed.prev = &s_cdCacheUsed;
}

// ──────────────────────────────────────────────────────────────────────
// The default lookup: the path table's place and size from the primary
// volume descriptor (sector 16), then the path. 1 found (sector and size),
// -2 the tray opened, -3 not found.
// ROR 0x002476d0

int rxFileCdFind(const char *name, int *lsn, int *size)
{
    char iso[256];
    int tableLsn;
    int tableSize;

    *lsn = 0;
    *size = 0;
    if (s_cdLayerPending != 0)
    {
        s_cdLayerPending = 0;
        s_cdLayer = s_cdLayerMode;
    }
    rxFileCdIsoName(iso, name);
    const char *path = iso;
    while (path[0] == '\\')
        path++;
    for (;;)
    {
        int r = rxFileCdReadCached(&tableLsn, 0x808c, 4);
        if (r == 1)
            break;
        if (r == -2)
            return -2;
    }
    s_cdDiscRead = 1;
    for (;;)
    {
        int r = rxFileCdReadCached(&tableSize, 0x8084, 4);
        if (r == 1)
            return rxFileCdFindDir(tableLsn << 11, tableSize, path, lsn, size);
        if (r == -2)
            return -2;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Walk the path table: each dir element is the record whose parent is the
// previous element's record number (the root is 1); the file is then
// looked up in the last dir's records.
// ROR 0x00247810

int rxFileCdFindDir(int tableOffset, int tableSize, const char *path, int *lsn, int *size)
{
    unsigned char rec[8];         // len, ext len, extent (4), parent (2)
    char want[256];
    char got[256];
    int r;

    while ((r = rxFileCdReadCached(rec, tableOffset, 8)) != 1)
    {
        if (r == -2)
            return -2;
    }
    unsigned int dirNo = 1;
    int at = (int)((rec[0] | rec[1] << 8) + 9) & ~1;
    unsigned int no = 2;
    while (at < tableSize)
    {
        if (rxFileCdIsLastElem(path))
        {
            int extent = rec[2] | rec[3] << 8 | rec[4] << 16 | rec[5] << 24;
            return rxFileCdFindInDir(extent << 11, path, lsn, size);
        }
        path = rxFileCdPathElem(want, path);
        int found = 0;
        do
        {
            while ((r = rxFileCdReadCached(rec, tableOffset + at, 8)) != 1)
            {
                if (r == -2)
                    return -2;
            }
            unsigned int len = rec[0] | rec[1] << 8;
            if ((unsigned int)(rec[6] | rec[7] << 8) == dirNo)
            {
                while ((r = rxFileCdReadCached(got, tableOffset + at + 8, (int)len)) != 1)
                {
                    if (r == -2)
                        return -2;
                }
                got[len] = '\0';
                if (strcmp(got, want) == 0)
                {
                    found = 1;
                    break;
                }
            }
            at += (int)(len + 9) & ~1;
            no++;
        } while (at < tableSize);
        if (found == 0)
            break;
        dirNo = no;
    }
    return -3;
}

// ──────────────────────────────────────────────────────────────────────
// Scan a dir's records (after ".") for a file of that name; records do not
// cross sectors, a zero length moves to the next sector.
// ROR 0x002479e0

int rxFileCdFindInDir(int dirOffset, const char *name, int *lsn, int *size)
{
    unsigned char rec[0x22];
    char got[256];
    int r;

    while ((r = rxFileCdReadCached(rec, dirOffset, 0x22)) != 1)
    {
        if (r == -2)
            return -2;
    }
    unsigned int dirSize = rec[10] | rec[11] << 8 | rec[12] << 16 | (unsigned int)rec[13] << 24;
    int at = (int)((rec[0] | rec[1] << 8) + 1) & ~1;
    for (;;)
    {
        while ((r = rxFileCdReadCached(rec, dirOffset + at, 0x22)) != 1)
        {
            if (r == -2)
                return -2;
        }
        unsigned int len = rec[0] | rec[1] << 8;
        int next;
        if (len == 0)
        {
            next = (at & ~0x7ff) + 0x800;
        }
        else
        {
            unsigned int nameLen = rec[0x20];
            if (nameLen != 0 && (rec[0x19] & 2) == 0)    // not a dir
            {
                while ((r = rxFileCdReadCached(got, dirOffset + at + 0x21, (int)nameLen)) != 1)
                {
                    if (r == -2)
                        return -2;
                }
                got[nameLen] = '\0';
                if (strcmp(got, name) == 0)
                {
                    *lsn = rec[2] | rec[3] << 8 | rec[4] << 16 | rec[5] << 24;
                    *size = rec[10] | rec[11] << 8 | rec[12] << 16 | rec[13] << 24;
                    return 1;
                }
            }
            if ((at & 0x7ff) + 0x22 > 0x800)
                next = (at & ~0x7ff) + 0x800;
            else
                next = at + (int)len;
        }
        at = next;
        if ((long long)dirSize <= at)
            return -3;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Copy the next path element (up to "\"); returns what follows it.
// ROR 0x00247bb0

const char *rxFileCdPathElem(char *out, const char *path)
{
    if (*path != '\0')
    {
        if (*path == '\\')
        {
            *out = '\0';
            return path + 1;
        }
        *out = *path;
        for (;;)
        {
            out++;
            path++;
            if (*path == '\0' || *path == '\\')
                break;
            *out = *path;
        }
    }
    *out = '\0';
    return *path == '\\' ? path + 1 : path;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00247c10

int rxFileCdIsLastElem(const char *path)
{
    while (*path != '\0' && *path != '\\')
        path++;
    return *path == '\0';
}

// ──────────────────────────────────────────────────────────────────────
// Copy size bytes at a disc offset through the sector cache: a hit moves
// the sector to the front, a miss takes a free buffer (or the least
// recently used one) and reads it until it reads. 1, or -2 when the tray
// opened.
// ROR 0x00247c60

int rxFileCdReadCached(void *out, int offset, int size)
{
    unsigned char *dst = (unsigned char *)out;
    int sector = offset / 0x800;
    int skip = offset - sector * 0x800;

    for (;;)
    {
        if (size < 1)
            return 1;
        int key = rxFileCdMapSector(sector);
        RxFileCdCacheNode *node = 0;
        for (RxFileCdCacheNode *n = s_cdCacheUsed.next; n != &s_cdCacheUsed; n = n->next)
        {
            if (n->sector == key)
            {
                node = n;
                break;
            }
        }
        if (node != 0)
        {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
        else
        {
            if (s_cdCacheFree == 0)
            {
                // Recycle the least recently used sector.
                RxFileCdCacheNode *lru = s_cdCacheUsed.prev;
                lru->prev->next = lru->next;
                lru->next->prev = lru->prev;
                lru->next = s_cdCacheFree;
                s_cdCacheFree = lru;
            }
            node = s_cdCacheFree;
            node->sector = key;
            for (;;)
            {
                int r = rxFileCdReadAt(node->buf, (long long)sector << 11, 0x800);
                if (r == 1)
                    break;
                if (r == -2)
                    return -2;
            }
            if (s_verbose != 0)
                rxSysPrintf("rxfile : completed to read sector '%d'\n", key);
            s_cdCacheFree = node->next;
        }
        node->prev = &s_cdCacheUsed;
        node->next = s_cdCacheUsed.next;
        s_cdCacheUsed.next = node;
        node->next->prev = node;

        while (skip < 0x800 && size > 0)
        {
            *dst++ = node->buf[skip++];
            size--;
        }
        skip = 0;
        sector++;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00247e88

char *rxFileCdGetRoot(void)
{
    return (char *)s_cdRoot;
}

// ──────────────────────────────────────────────────────────────────────
// The cdrom0: root is fixed.
// ROR 0x00247e98

void rxFileCdSetRoot(const char *root)
{
    (void)root;
}

// ──────────────────────────────────────────────────────────────────────
// Replace the file lookup; returns the old one.
// ROR 0x00247ea0

RxFileCdFindFunc rxFileCdSetFindFunc(RxFileCdFindFunc func)
{
    RxFileCdFindFunc old = s_cdFind;
    s_cdFind = func;
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// Turn the periodic clock read on or off (a change restarts it); returns
// the old setting.
// ROR 0x00247eb8

int rxFileCdSetClockRead(int enable)
{
    int old = s_cdClockRead;
    if (old != enable)
    {
        s_cdClockInterval = 0x1e;
        s_cdClockTick = 0;
        s_cdClockCountdown = 0;
        s_cdClockValid = 0;
        s_cdClockRead = enable;
    }
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00247f00

int rxFileCdGetClockRead(void)
{
    return s_cdClockRead;
}

// ──────────────────────────────────────────────────────────────────────
// The last clock read (an sceCdCLOCK); 0 when there is none.
// ROR 0x00247f10

int rxFileCdGetClock(void *clock)
{
    if (s_cdClockValid == 0 || s_cdClockRead == 0)
        return 0;
    if (clock != 0)
    {
        DIntr();
        memcpy(clock, &s_cdClock, sizeof(s_cdClock));
        EIntr();
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Pick the DVD layer (0 / 1) the next lookup switches to; returns the old
// mode, -1 for a bad one.
// ROR 0x00247fd0

int rxFileCdSetLayerMode(int mode)
{
    int old = s_cdLayerMode;
    if ((unsigned int)mode >= 2)
        return -1;
    s_cdLayerMode = mode;
    s_cdLayerPending = 1;
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248008

int rxFileCdGetLayerMode(void)
{
    return s_cdLayerMode;
}

/* ---- HDD driver ------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248018

void rxFileHddInit(RxFileDriver *drv)
{
    drv->makePath  = rxFileHddMakePath;
    drv->setRoot   = rxFileHddSetRoot;
    drv->control   = rxFileHddControl;
    drv->getStatus = rxFileHddGetStatus;
    drv->getSize   = rxFileHddGetSize;
    drv->create    = rxFileHddCreate;
    drv->read      = rxFileHddRead;
    drv->write     = rxFileHddWrite;
    drv->getRoot   = rxFileHddGetRoot;
    rxFileHddSetRoot(0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002480a0

void rxFileHddTerm(void)
{
}

// ──────────────────────────────────────────────────────────────────────
// On a drive error (-5, -6, -19, -65531) send HDD accesses to its fallback
// device and tell the error hook.
// ROR 0x002480a8

void rxFileHddOnError(const char *name, int error)
{
    if (error < -4 && (error > -7 || error == -0xfffb || error == -0x13))
    {
        if (s_verbose != 0)
            rxSysPrintf("rxFile: occured I/O error (%d), RX_FILE_DEVICE_HDD.\n", error);
        rxFileSetDeviceMap(RX_SYS_DEVICE_HDD, rxFileGetFallbackDevice(RX_SYS_DEVICE_HDD));
        if (rxFileGetErrorHook() != 0)
            rxFileGetErrorHook()(name, error);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248158

void rxFileHddMakePath(char *out, const char *name)
{
    rxFileHddJoinRoot(out, name);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248170

int rxFileHddControl(int cmd, int arg)
{
    (void)cmd;
    (void)arg;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Status 1: busy in a file call.
// ROR 0x00248178

int rxFileHddGetStatus(int what)
{
    if (what == 1)
        return s_hddFlags & 1;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002481a0

int rxFileHddGetSize(const char *name)
{
    char path[256];
    int size;

    s_hddFlags |= 1;
    rxFileHddJoinRoot(path, name);
    int fd = rxSysOpen(path, SCE_RDONLY);
    if (fd < 0)
    {
        rxFileHddOnError(name, fd);
        if (s_verbose != 0)
            rxSysPrintf("rxfile : failed to open file '%s'\n", path);
        size = -2;
    }
    else
    {
        size = rxSysLseek(fd, 0, SCE_SEEK_END);
        rxSysClose(fd);
    }
    s_hddFlags &= ~1;
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248270

int rxFileHddCreate(const char *name, int truncate)
{
    char path[256];
    int ret;

    s_hddFlags |= 1;
    rxFileHddJoinRoot(path, name);
    int mode = SCE_WRONLY | SCE_CREAT | SCE_TRUNC;
    if (truncate == 0)
        mode = SCE_WRONLY | SCE_CREAT;
    int fd = rxSysOpen(path, mode);
    if (fd < 0)
    {
        rxFileHddOnError(name, fd);
        if (s_verbose != 0)
            rxSysPrintf("rxfile : failed to open file '%s'\n", path);
        ret = -2;
    }
    else
    {
        rxSysClose(fd);
        ret = 1;
    }
    s_hddFlags &= ~1;
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248340

int rxFileHddRead(const char *name, void *buf, int offset, int size)
{
    char path[256];

    s_hddFlags |= 1;
    rxFileHddJoinRoot(path, name);
    FlushCache(0);
    int fd = rxSysOpen(path, SCE_RDONLY);
    if (fd < 0)
    {
        rxFileHddOnError(name, fd);
        if (s_verbose != 0)
            rxSysPrintf("rxfile : failed to open file '%s'\n", path);
        size = -2;
    }
    else
    {
        rxSysLseek(fd, offset, SCE_SEEK_SET);
        int got = rxSysRead(fd, buf, size);
        if (got < 1)
        {
            rxSysClose(fd);
            rxFileHddOnError(name, got);
            size = -1;
        }
        else
        {
            rxSysClose(fd);
            FlushCache(0);
        }
    }
    s_hddFlags &= ~1;
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248480

int rxFileHddWrite(const char *name, int offset, const void *buf, int size)
{
    char path[256];

    s_hddFlags |= 1;
    rxFileHddJoinRoot(path, name);
    FlushCache(0);
    int fd = rxSysOpen(path, SCE_WRONLY);
    if (fd < 0)
    {
        rxFileHddOnError(name, fd);
        if (s_verbose != 0)
            rxSysPrintf("rxfile : failed to open file '%s'\n", path);
        size = -2;
    }
    else
    {
        rxSysLseek(fd, offset, SCE_SEEK_SET);
        int put = rxSysWrite(fd, buf, size);
        if (put < 1)
        {
            rxSysClose(fd);
            rxFileHddOnError(name, put);
            size = -1;
        }
        else
        {
            rxSysClose(fd);
            FlushCache(0);
        }
    }
    s_hddFlags &= ~1;
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002485c0

void rxFileHddJoinRoot(char *out, const char *name)
{
    strcpy(out, s_hddRoot);
    strcpy(out + strlen(out), name);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248608

char *rxFileHddGetRoot(void)
{
    return s_hddRoot;
}

// ──────────────────────────────────────────────────────────────────────
// null = "host0:".
// ROR 0x00248618

void rxFileHddSetRoot(const char *root)
{
    if (root == 0)
    {
        strcpy(s_hddRoot, s_hddRootDefault);
        return;
    }
    strcpy(s_hddRoot, root);
}

/* ---- contexts, devices and dirs --------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Empty dirs and name stacks for every device; both contexts on HOST with
// the default hooks; context 0 current.
// ROR 0x00248660

void rxFileInitSlots(void)
{
    s_contextIndex = 0;
    for (int dev = 0; dev < RX_SYS_DEVICE_NUM; dev++)
    {
        RxFileDevInfo *info = &s_devInfo[dev];
        info->param = -1;
        info->dir[0] = '\0';
        for (int i = 0; i < 2; i++)
        {
            info->ctx[i].searchDir[0] = '\0';
            info->ctx[i].stackTop = 0x7ff;
            info->ctx[i].lastName[0] = '\0';
        }
    }
    for (int i = 0; i < 2; i++)
    {
        s_contexts[i].device = RX_SYS_DEVICE_HOST;
        s_contexts[i].loadHook = rxFileDefaultLoadHook;
        s_contexts[i].sizeHook = rxFileDefaultSizeHook;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Empty in the retail build (rxsys calls it every tick).
// ROR 0x00248708

void rxFileUpdate(void)
{
}

// ──────────────────────────────────────────────────────────────────────
// Select context 0 or 1; returns the previous one.
// ROR 0x00248710

int rxFileSetContext(int index)
{
    int old = s_contextIndex;
    if (index < 2 && index >= 0)
        s_contextIndex = index;
    else if (s_verbose != 0)
        rxSysPrintf("rxfile : unknow thread type '%d'\n", index);
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// Set the current context's device (-1 = the boot device); returns the
// previous one. MCARD has no driver and is refused.
// ROR 0x00248778

int rxFileSetCurrent(int device)
{
    RxFileContext *ctx = rxFileGetContext();
    int old = ctx->device;
    if (device == -1)
        device = rxSysGetBootDevice();
    if (rxSysIsDeviceReady(device) == 0)
    {
        if (s_verbose != 0)
            rxSysPrintf("rxfile : device disabled %d\n", device);
        return old;
    }
    if (device >= 0 && device != RX_SYS_DEVICE_MCARD && device <= RX_SYS_DEVICE_HDD)
    {
        ctx->device = device;
        if (s_verbose != 0)
            rxSysPrintf("rxfile : device changed %d\n", device);
        return old;
    }
    if (s_verbose != 0)
        rxSysPrintf("rxfile : unknown device type '%d'\n", device);
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// The current context's device.
// ROR 0x00248870

int rxFileGetCurrent(void)
{
    return rxFileGetContext()->device;
}

// ──────────────────────────────────────────────────────────────────────
// rxFileSetCurrent for a given context.
// ROR 0x00248890

int rxFileSetDevice(int ctx, int device)
{
    int *slot = &s_contexts[ctx].device;
    int old = *slot;
    if (device == -1)
        device = rxSysGetBootDevice();
    if (rxSysIsDeviceReady(device) == 0)
    {
        if (s_verbose != 0)
            rxSysPrintf("rxfile : device disabled %d\n", device);
        return old;
    }
    if (device >= 0 && device != RX_SYS_DEVICE_MCARD && device <= RX_SYS_DEVICE_HDD)
    {
        *slot = device;
        if (s_verbose != 0)
            rxSysPrintf("rxfile : device changed %d\n", device);
        return old;
    }
    if (s_verbose != 0)
        rxSysPrintf("rxfile : unknown device type '%d'\n", device);
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// A device's dir: lower case, '\' -> '/', a '/' added at the end.
// ROR 0x00248998

void rxFileSetDir(int device, const char *dir)
{
    char *out = rxFileGetDeviceInfo(device)->dir;
    if (dir == 0)
    {
        out[0] = '\0';
        return;
    }
    int n = 0;
    while (*dir != '\0')
    {
        char c = *dir;
        if (isupper((unsigned char)c))
            c += ' ';
        out[n] = c;
        if (c == '\\')
            out[n] = '/';
        n++;
        dir++;
    }
    out += n;
    *out = '\0';
    if (n > 0 && out[-1] != '/')
    {
        out[0] = '/';
        out[1] = '\0';
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248a60

char *rxFileGetDir(int device)
{
    return rxFileGetDeviceInfo(device)->dir;
}

// ──────────────────────────────────────────────────────────────────────
// Walk a dir onto the device's search dir element by element: "/" restarts
// at the root, "." stays, ".." (and "...") climbs one less than its dots,
// anything else is appended. A '/' is added at the end.
// ROR 0x00248a80

void rxFileSetSearchDir(int device, const char *dir)
{
    char elem[256];
    char *out = rxFileGetDirInfo(device)->searchDir;

    if (dir == 0)
    {
        out[0] = '\0';
        return;
    }
    while (*dir != '\0')
    {
        dir = rxFilePathNextElem(elem, dir);
        int dots = rxFilePathCountDots(elem);
        if (dots < 1 || strlen(out) < 3)
        {
            if (elem[0] == '/')
            {
                out[0] = '/';
                out[1] = '\0';
            }
            else
            {
                strcpy(out + strlen(out), elem);
            }
        }
        else
        {
            for (int i = 1; i < dots; i++)
                rxFilePathUp(out);
        }
    }
    int len = (int)strlen(out);
    if (len > 0 && out[len - 1] != '/')
    {
        out[len] = '/';
        out[len + 1] = '\0';
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248bb0

char *rxFileGetSearchDir(int device)
{
    return rxFileGetDirInfo(device)->searchDir;
}

// ──────────────────────────────────────────────────────────────────────
// Push the search dir onto the name stack.
// ROR 0x00248bc8

void rxFilePushSearchDir(int device)
{
    RxFileDirInfo *info = rxFileGetDirInfo(device);
    int top = info->stackTop - ((int)strlen(info->searchDir) + 1);
    if (top >= 0)
    {
        info->stackTop = top;
        strcpy(info->stack + top, info->searchDir);
        return;
    }
    if (s_verbose != 0)
        rxSysPrintf("rxfile : name stack is overflow\n", info);
}

// ──────────────────────────────────────────────────────────────────────
// Pop the name stack into the search dir.
// ROR 0x00248c50

void rxFilePopSearchDir(int device)
{
    RxFileDirInfo *info = rxFileGetDirInfo(device);
    if (info->stackTop < 0x7ff)
    {
        strcpy(info->searchDir, info->stack + info->stackTop);
        info->stackTop += (int)strlen(info->stack + info->stackTop) + 1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248cb8

void rxFileSetLastName(int device, const char *name)
{
    strcpy(rxFileGetDirInfo(device)->lastName, name);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248ce8

char *rxFileGetLastName(int device)
{
    return rxFileGetDirInfo(device)->lastName;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248d08

int rxFileSetDeviceParam(int device, int value)
{
    RxFileDevInfo *info = rxFileGetDeviceInfo(device);
    int old = info->param;
    info->param = value;
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248d38

int rxFileGetDeviceParam(int device)
{
    return rxFileGetDeviceInfo(device)->param;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248d58

RxFileLoadHook rxFileGetLoadHook(void)
{
    return rxFileGetContext()->loadHook;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248d78

RxFileLoadHook rxFileSwapLoadHook(RxFileLoadHook hook)
{
    RxFileContext *ctx = rxFileGetContext();
    RxFileLoadHook old = ctx->loadHook;
    ctx->loadHook = hook;
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248da8

RxFileSizeHook rxFileGetSizeHook(void)
{
    return rxFileGetContext()->sizeHook;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248dc8

RxFileSizeHook rxFileSwapSizeHook(RxFileSizeHook hook)
{
    RxFileContext *ctx = rxFileGetContext();
    RxFileSizeHook old = ctx->sizeHook;
    ctx->sizeHook = hook;
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// A device's record. The ROM also calls rxFileGetContext and drops the
// result.
// ROR 0x00248df8

RxFileDevInfo *rxFileGetDeviceInfo(int device)
{
    rxFileGetContext();
    return &s_devInfo[device];
}

// ──────────────────────────────────────────────────────────────────────
// A device's dir block for the current context.
// ROR 0x00248e30

RxFileDirInfo *rxFileGetDirInfo(int device)
{
    return &rxFileGetDeviceInfo(device)->ctx[s_contextIndex];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00248e70

RxFileContext *rxFileGetContext(void)
{
    return &s_contexts[s_contextIndex];
}

// ──────────────────────────────────────────────────────────────────────
// Copy one element of path, up to and including its '/', and return the
// rest. ROM quirk kept: the "./" test reads the byte after the copy, not
// the element, so it only fires on stale buffer contents.
// ROR 0x00248e98

const char *rxFilePathNextElem(char *out, const char *path)
{
    if (*path == '\0')
        return path;
    char c;
    while ((c = *path) != '\0')
    {
        *out = c;
        path++;
        out++;
        if (c == '/')
            break;
    }
    if (*out == '.' && out[1] == '/')
        out -= 2;
    *out = '\0';
    return path;
}

// ──────────────────────────────────────────────────────────────────────
// Drop the last element of a dir ("a/b/" -> "a/").
// ROR 0x00248f00

void rxFilePathUp(char *dir)
{
    int len = (int)strlen(dir);
    if (len > 1)
    {
        int i = len - 1;
        dir[i] = '\0';
        while (i > 0 && dir[i - 1] != '/')
        {
            i--;
            dir[i] = '\0';
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// The number of dots of an element made only of dots ("..", "../"), else 0.
// ROR 0x00248f88

int rxFilePathCountDots(const char *elem)
{
    if (*elem != '.')
        return 0;
    int n = 0;
    char c = '.';
    elem++;
    for (;;)
    {
        if (c != '.')
            return 0;
        c = *elem++;
        n++;
        if (c == '\0' || c == '/')
            return n;
    }
}

/* ---- the file thread -------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Start file thread 0 at priority 2 running the cdrom reader.
// ROR 0x00248fd8

void rxFileStartThread(void)
{
    rxFileCreateThread(0, 2, rxFileThreadProc);
}

// ──────────────────────────────────────────────────────────────────────
// Ask the file thread to stop, wait for it, delete it.
// ROR 0x00249000

void rxFileStopThread(void)
{
    s_threads[0].stop = 1;
    while (s_threads[0].stop > 0)
        rxFileSpin();             // callcheck: EIntr, rxSysPrintf (the DI check)
    rxThDelete(s_threads[0].thread);
}

// ──────────────────────────────────────────────────────────────────────
// Queue a request on queue 0 / 1 of the device's file thread; 0 when it
// is full.
// ROR 0x00249098

int rxFilePostRequest(int queue, int device, const char *path, int op, int offset, void *buf,
                      int size, int retryMissing, RxFileRequestCallback callback,
                      RxFileSlot *owner)
{
    RxFileThread *th = &s_threads[s_deviceThread[device]];
    return rxFileQueuePush(&th->queue[queue], device, path, op, offset, buf, size, retryMissing,
                           callback, owner);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002490e0

void rxFileCancelRequests(int device, RxFileSlot *owner, int abort)
{
    DIntr();
    RxFileThread *th = &s_threads[s_deviceThread[device]];
    for (int i = 0; i < 2; i++)
        rxFileQueueCancel(&th->queue[i], owner, abort);
    EIntr();
}

// ──────────────────────────────────────────────────────────────────────
// Set file thread index up (proc runs before every request) and start it
// at priority.
// ROR 0x002491c0

void rxFileCreateThread(int index, int priority, void (*proc)(int step))
{
    RxFileThread *th = &s_threads[index];
    th->thread = -1;
    th->proc = proc;
    th->step = priority + 1;
    th->stop = 0;
    rxFileQueueInit(&th->queue[0]);
    rxFileQueueInit(&th->queue[1]);
    while ((th->thread = rxThCreate(rxFileThreadMain, th->stack, sizeof(th->stack), priority, th))
           == -1)
    {
        if (s_verbose != 0)
            rxSysPrintf("rxfile : failed to create thread\n");
    }
    rxThSetState(th->thread, 1);
}

// ──────────────────────────────────────────────────────────────────────
// The file thread: run proc, then serve the first request of queue 0 (or
// else queue 1) through the mapped device's driver. A failed request stays
// queued and is tried again (a missing file only when it asked for that);
// a served one gets its callback unless it was cancelled. Yields when the
// queues are empty; once asked to stop it answers -1 and idles.
// ROR 0x002492b0

void rxFileThreadMain(void)
{
    int result = 0;

    if (s_verbose != 0)
        rxSysPrintf("rxfile : start execute request queue thread %d\n", rxThGetIndex());
    RxFileThread *th = (RxFileThread *)rxThGetParam(rxThGetIndex());

    while (th->stop == 0)
    {
        if (th->proc != 0)
            th->proc(th->step);     // callcheck: rxFileThreadProc
        DIntr();
        RxFileQueue *q = 0;
        RxFileRequest *req = 0;
        for (int i = 0; i < 2 && req == 0; i++)
        {
            q = &th->queue[i];
            while ((req = rxFileQueueFront(q)) != 0 && req->state == 1)
                rxFileQueuePop(q);
        }
        if (req == 0)
        {
            EIntr();
            rxThYield();
            continue;
        }

        req->busy = 1;
        int device = s_deviceMap[req->device];
        EIntr();
        RxFileDriver *drv = rxFileGetDriver(device);
        switch (req->op)
        {
        case RX_FILE_OP_CREATE:
            result = drv->create(req->path, req->offset);   // callcheck: rxFileHostCreate (driver slot 4)
            break;
        case RX_FILE_OP_SIZE:
            result = drv->getSize(req->path);               // callcheck: rxFileHostGetSize (driver slot 3)
            break;
        case RX_FILE_OP_READ:
            result = drv->read(req->path, req->buf, req->offset, req->size); // callcheck: rxFileHostRead (driver slot 5)
            break;
        case RX_FILE_OP_WRITE:
            result = drv->write(req->path, req->offset, req->buf, req->size); // callcheck: rxFileHostWrite (driver slot 6)
            break;
        default:
            // The ROM passes no type (a1 is left over); result stays.
            if (s_verbose != 0)
                rxSysPrintf("invalide request type %d\n", req->op);
            break;
        }
        if (result < 1 && (result != -2 || req->retryMissing != 0))
        {
            req->busy = 0;          // served again after proc
            continue;
        }
        DIntr();
        if (req->state == 0 && req->callback != 0)
            req->callback(req->owner, req->offset, result);
        rxFileQueuePop(q);
        EIntr();
    }
    th->stop = -1;
    for (;;)
        rxThYield();
}

// ──────────────────────────────────────────────────────────────────────
// The file thread's work.
// ROR 0x002495e0

void rxFileThreadProc(int step)
{
    rxFileCdProcess(step);
}

// ──────────────────────────────────────────────────────────────────────
// Empty and unreferenced.
// ROR 0x002495f8

void rxFileNop0(void)
{
}

// ──────────────────────────────────────────────────────────────────────
// Empty and unreferenced.
// ROR 0x00249600

void rxFileNop1(void)
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00249608

void rxFileQueueInit(RxFileQueue *q)
{
    q->tail = 0;
    q->head = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Append a request; 0 when the queue is full ("Queue Over!").
// ROR 0x00249618

int rxFileQueuePush(RxFileQueue *q, int device, const char *path, int op, int offset, void *buf,
                    int size, int retryMissing, RxFileRequestCallback callback, RxFileSlot *owner)
{
    if (rxFileQueueSpace(q) < 1)
    {
        rxSysPrintf("Queue Over!\n");
        return 0;
    }
    DIntr();
    int i = q->head;
    q->head = (i + 1) % 64;
    RxFileRequest *req = &q->req[i];
    req->device = device;
    req->owner = owner;
    req->path = path;
    req->op = op;
    req->offset = offset;
    req->buf = buf;
    req->size = size;
    req->retryMissing = retryMissing;
    req->state = 0;
    req->callback = callback;
    req->busy = 0;
    EIntr();
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00249778

RxFileRequest *rxFileQueueFront(RxFileQueue *q)
{
    if (rxFileQueueCount(q) < 1)
        return 0;
    return &q->req[q->tail];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002497c0

void rxFileQueuePop(RxFileQueue *q)
{
    if (rxFileQueueCount(q) > 0)
        q->tail = (q->tail + 1) % 64;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00249818

int rxFileQueueCount(RxFileQueue *q)
{
    int n = q->head - q->tail;
    if (n < 0)
        n += 64;
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// Free entries, one kept empty to tell full from empty.
// ROR 0x00249838

int rxFileQueueSpace(RxFileQueue *q)
{
    int n = 64 - rxFileQueueCount(q);
    return n - (n > 0);
}

// ──────────────────────────────────────────────────────────────────────
// Cancel owner's queued requests: each gets its callback with its size
// and is marked done; the one being served only when abort.
// ROR 0x00249868

void rxFileQueueCancel(RxFileQueue *q, RxFileSlot *owner, int abort)
{
    DIntr();
    for (int i = q->tail; i != q->head; i = (i + 1) % 64)
    {
        RxFileRequest *req = &q->req[i];
        if (req->owner != owner || req->state == 1)
            continue;
        if (req->busy == 1 && abort == 0)
            continue;
        if (req->callback != 0)
            req->callback(req->owner, req->offset, req->size);
        req->state = 1;
        req->callback = 0;
        req->owner = 0;
    }
    EIntr();
}

/* ---- device map and the HDD error hook -------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Map device onto target (both -1 = the boot device, clamped to 0..3).
// ROR 0x002499b8

void rxFileSetDeviceMap(int device, int target)
{
    if (device == -1)
        device = rxSysGetBootDevice();
    if (target == -1)
        target = rxSysGetBootDevice();
    if (device < 0)
        device = 0;
    else if (device > 3)
        device = 3;
    if (target < 0)
        target = 0;
    else if (target > 3)
        target = 3;
    s_deviceMap[device] = target;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00249a68

int rxFileGetDeviceMap(int device)
{
    if (device == -1)
        device = rxSysGetBootDevice();
    int i;
    if (device < 0)
        i = 0;
    else if (device < 4)
        i = device;
    else
        i = 3;
    return s_deviceMap[i];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00249ad0

void rxFileSetFallbackDevice(int device, int fallback)
{
    s_fallbackDevice[device] = fallback;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00249ae8

int rxFileGetFallbackDevice(int device)
{
    return s_fallbackDevice[device];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00249b00

void rxFileSetErrorHook(RxFileErrorHook hook)
{
    s_errorHook = hook;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00249b10

RxFileErrorHook rxFileGetErrorHook(void)
{
    return s_errorHook;
}

// ──────────────────────────────────────────────────────────────────────
// Change the file thread's priority; returns the old one.
// ROR 0x00249b20

int rxFileSetThreadPriority(int priority)
{
    int old = rxFileGetThreadPriority();
    rxThSetPriority(s_threads[0].thread, priority);
    s_threads[0].step = priority + 1;
    return old;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00249b78

int rxFileGetThreadPriority(void)
{
    return s_threads[0].step - 1;
}
