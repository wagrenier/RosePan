/* ==========================================================================
 *  rx/rxfile.cpp
 *
 *  The rx middleware's file layer: device drivers, contexts, dirs, whole-file
 *  loads, IOP reboot and module loads. See rxfile.h.
 *
 *  Not reconstructed yet in this file: the async file slots and their
 *  request queue (0x2437f8..0x245800), the default load / size / save hooks
 *  (0x245a48..0x245c40), rxFileLoadExec (0x245db8), the cdrom driver's
 *  reader, sector cache and ISO lookup (0x2469a8..0x247f10, except its
 *  table, cache init and the small slots) and the file thread
 *  (0x248fd8..0x249b88: rxFileCreateThread, its request handlers, the
 *  thread priority helpers).
 * ======================================================================== */
#include "common/rosepan_stub.h"

#include "rx/rxfile.h"

#include <ctype.h>
#include <string.h>

#include "rx/rxsys.h"
#include "sdk/fileio.h"
#include "sdk/sifdev.h"

/* ---- data ------------------------------------------------------------- */

static int s_verbose = 1;                       /* data 0x2dc1b0: print rxfile messages */
static const char *s_hostRootDefault = "host0:"; /* data 0x2dc1b4 */
static int s_hostFlags;                         /* data 0x2dc1b8: bit 0 = busy          */
static const char *s_cdRoot = "cdrom0:";        /* data 0x2dc1c0 */
static int s_cdFlags;                           /* data 0x2dc1cc: bits 0 / 1 = status    */
static int s_cdQueued0;                         /* data 0x2dc1d0 */
static int s_cdQueued1;                         /* data 0x2dc1d4 */
static long long s_cdLastSector = -1;           /* data 0x2dc218 (a doubleword)          */
static const char *s_hddRootDefault = "host0:"; /* data 0x2dc220 */
static int s_hddFlags;                          /* data 0x2dc224: bit 0 = busy          */
static int s_deviceMap[RX_SYS_DEVICE_NUM] = { 0, 1, 2, 3 };             /* data 0x2dc228 */
static int s_fallbackDevice[RX_SYS_DEVICE_NUM] = { 0, 1, 2, -1 };       /* data 0x2dc238 */
static RxFileErrorHook s_errorHook;             /* data 0x2dc248 */

static RxFileDriver s_drivers[RX_SYS_DEVICE_NUM];   /* bss 0x7df9c0 (HOST 0x7df9c0, CDROM 0x7df9e4, HDD 0x7dfa2c) */
static char s_hostRoot[0x100];                  /* bss 0x7dfa50 */
static RxFileCdCacheNode s_cdCacheNodes[32];    /* bss 0x7dfb80 */
static RxFileCdCacheNode s_cdCacheUsed;         /* bss 0x7dfd80: sentinel of the used list */
static RxFileCdCacheNode *s_cdCacheFree;        /* bss 0x7dfd90 */
static int s_cdCacheCount;                      /* bss 0x7dfd94 */
static int s_cdCacheReset;                      /* bss 0x7dfd98 */
static unsigned char s_cdCacheBuf[32][0x800];   /* bss 0x7dfdc0 */
static char s_hddRoot[0x100];                   /* bss 0x7f1dc0 */
static int s_contextIndex;                      /* bss 0x7f1ec0 */
static RxFileContext s_contexts[2];             /* bss 0x7f1ec4 */
static RxFileDevInfo s_devInfo[RX_SYS_DEVICE_NUM];  /* bss 0x7f1edc */

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
// Status 0 / 1: flag bits; status 2: requests queued.
// ROR 0x00246d80

int rxFileCdGetStatus(int what)
{
    if (what == 1)
        return (s_cdFlags >> 1) & 1;
    if (what > 1)
    {
        if (what == 2)
            return s_cdQueued0 + s_cdQueued1;
        return 0;
    }
    if (what != 0)
        return 0;
    return s_cdFlags & 1;
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
    s_cdCacheReset = 1;
    s_cdCacheUsed.next = &s_cdCacheUsed;
    s_cdCacheUsed.prev = &s_cdCacheUsed;
    s_cdCacheCount = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Return every used sector buffer to the free list.
// ROR 0x00247668

void rxFileCdFlushCache(void)
{
    s_cdCacheReset = 1;
    s_cdCacheCount = 0;
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
// The file thread's work.
// ROR 0x002495e0

void rxFileThreadProc(void)
{
    rxFileCdProcess();
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
// ROR 0x00249ae8

int rxFileGetFallbackDevice(int device)
{
    return s_fallbackDevice[device];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00249b10

RxFileErrorHook rxFileGetErrorHook(void)
{
    return s_errorHook;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00246cb8
int rxFileCdControl(int cmd, int arg)
{
    ROSEPAN_STUB("rxFileCdControl", 0x00246cb8);
    return {};
}

// stub of ROR 0x00246e00
int rxFileCdGetSize(const char *name)
{
    ROSEPAN_STUB("rxFileCdGetSize", 0x00246e00);
    return {};
}

// stub of ROR 0x00246c70
void rxFileCdMakePath(char *out, const char *name)
{
    ROSEPAN_STUB("rxFileCdMakePath", 0x00246c70);
}

// stub of ROR 0x002469a8
void rxFileCdProcess(void)
{
    ROSEPAN_STUB("rxFileCdProcess", 0x002469a8);
}

// stub of ROR 0x00246e80
int rxFileCdRead(const char *name, void *buf, int offset, int size)
{
    ROSEPAN_STUB("rxFileCdRead", 0x00246e80);
    return {};
}

// stub of ROR 0x002491c0
void rxFileCreateThread(int index, int priority, void (*proc)(void))
{
    ROSEPAN_STUB("rxFileCreateThread", 0x002491c0);
}

// stub of ROR 0x00245ae8
int rxFileDefaultLoadHook(const char *name, void *buf)
{
    ROSEPAN_STUB("rxFileDefaultLoadHook", 0x00245ae8);
    return {};
}

// stub of ROR 0x00245a48
int rxFileDefaultSizeHook(const char *name)
{
    ROSEPAN_STUB("rxFileDefaultSizeHook", 0x00245a48);
    return {};
}
