/* ==========================================================================
 *  rx/rxfile.h
 *
 *  The rx middleware's file layer (ROR 0x2437f8..0x249b88, no source-path
 *  string: named after its "rxfile : ..." messages). Three pieces:
 *  - device drivers: a table of nine functions per device (HOST, CDROM, HDD;
 *    MCARD has none) that turns a name into a device path and reads, writes
 *    or sizes a whole file;
 *  - two contexts ("thread type" 0 / 1: the caller's and the file thread's),
 *    each with a current device and a load / size hook pair;
 *  - per device and context, a current dir, a search dir and a name stack.
 *  rxFileLoad / rxFileLoadModule / rxFileRebootIop build the path from the
 *  current device's dirs and go through rxsys's sce* wrappers.
 *  Beside them, the async file slots (0x2437f8..0x245800): 63 open files
 *  ("fd" 1..63) whose create / size / read / write requests go to the file
 *  thread's queues (0x249000..0x2499b8), with an optional ring buffer the
 *  thread keeps filled, position markers that split, count and loop reads,
 *  and data tags reported per ring segment. The default load / size / save
 *  hooks use them. The cdrom driver reads the ISO 9660 file system itself
 *  (primary volume descriptor, path table, directory records) through a
 *  32-sector LRU cache.
 * ======================================================================== */
#ifndef _RX_RXFILE_H
#define _RX_RXFILE_H

#include <stddef.h>

#include "rx/rxsys.h"

/* Device driver (data 0x7df9c0, 0x24 bytes each on the EE, indexed by
 * RX_SYS_DEVICE; MCARD's entry 0x7dfa08 stays empty).
 * layout: host -- a table of code pointers built at run time. */
struct RxFileDriver
{
    void  (*makePath)(char *out, const char *name);                     // ROR 0x00
    int   (*getStatus)(int what);                                       // ROR 0x04
    int   (*control)(int cmd, int arg);                                 // ROR 0x08
    int   (*getSize)(const char *name);                                 // ROR 0x0c
    int   (*create)(const char *name, int truncate);                    // ROR 0x10
    int   (*read)(const char *name, void *buf, int offset, int size);   // ROR 0x14
    int   (*write)(const char *name, int offset, const void *buf, int size); // ROR 0x18
    void  (*setRoot)(const char *root);                                 // ROR 0x1c
    char *(*getRoot)(void);                                             // ROR 0x20
};

typedef int (*RxFileLoadHook)(const char *name, void *buf);
typedef int (*RxFileSizeHook)(const char *name);

/* A context ("thread type" 0 / 1, data 0x7f1ec4, 12 bytes each on the EE).
 * layout: host -- holds code pointers. */
struct RxFileContext
{
    int            device;      // ROR 0x0, RX_SYS_DEVICE
    RxFileLoadHook loadHook;    // ROR 0x4, default rxFileDefaultLoadHook
    RxFileSizeHook sizeHook;    // ROR 0x8, default rxFileDefaultSizeHook
};

/* Per device and context: search dir and the name stack it is pushed on.
 * EE layout (plain data). */
struct RxFileDirInfo
{
    char searchDir[0x80];       // ROR 0x000
    char stack[0x800];          // ROR 0x080, grows down from stackTop
    int  stackTop;              // ROR 0x880, 0x7ff when empty
    char lastName[0x80];        // ROR 0x884
};
static_assert(sizeof(RxFileDirInfo) == 0x904, "RxFileDirInfo size");
static_assert(offsetof(RxFileDirInfo, searchDir) == 0x000, "RxFileDirInfo::searchDir");
static_assert(offsetof(RxFileDirInfo, stack) == 0x080, "RxFileDirInfo::stack");
static_assert(offsetof(RxFileDirInfo, stackTop) == 0x880, "RxFileDirInfo::stackTop");
static_assert(offsetof(RxFileDirInfo, lastName) == 0x884, "RxFileDirInfo::lastName");

/* Per device (data 0x7f1edc). EE layout (plain data). */
struct RxFileDevInfo
{
    int           param;        // ROR 0x00, -1 after rxFileInitSlots; meaning open
    char          dir[0x80];    // ROR 0x04, lower case, '/' separators, ends in '/'
    RxFileDirInfo ctx[2];       // ROR 0x84, one per context
};
static_assert(sizeof(RxFileDevInfo) == 0x128c, "RxFileDevInfo size");
static_assert(offsetof(RxFileDevInfo, param) == 0x00, "RxFileDevInfo::param");
static_assert(offsetof(RxFileDevInfo, dir) == 0x04, "RxFileDevInfo::dir");
static_assert(offsetof(RxFileDevInfo, ctx) == 0x84, "RxFileDevInfo::ctx");

/* The cdrom driver's sector cache node (data 0x7dfb80, 16 bytes each on the
 * EE, 32 of them; the used list is circular through a sentinel at 0x7dfd80).
 * layout: host -- pointers. */
struct RxFileCdCacheNode
{
    RxFileCdCacheNode *prev;    // ROR 0x0
    RxFileCdCacheNode *next;    // ROR 0x4, also the free-list link
    unsigned char     *buf;     // ROR 0x8, 0x800 bytes
    int                sector;  // ROR 0xc
};

/* Called by rxFileHddOnError with the name and the I/O error. */
typedef void (*RxFileErrorHook)(const char *name, int error);

/* The cdrom driver's file lookup (data 0x2dc1dc, default rxFileCdFind):
 * 1 found, -2 tray opened, -3 not found. */
typedef int (*RxFileCdFindFunc)(const char *name, int *lsn, int *size);

/* ---- async file slots ------------------------------------------------- */

/* Open flags: SCE_RDONLY / WRONLY / CREAT / TRUNC plus the rx bits. */
#define RX_FILE_NOWAIT  0x8000      /* return before the request is served  */
#define RX_FILE_RING    0x10000     /* keep the slot's ring buffer filled   */

/* Request ops (RxFileRequest::op), served through the device driver. */
enum
{
    RX_FILE_OP_CREATE = 0,
    RX_FILE_OP_SIZE   = 1,
    RX_FILE_OP_READ   = 2,
    RX_FILE_OP_WRITE  = 3,
};

/* A position marker as the caller passes it (20 bytes on the EE; copied
 * into a pool node). A read crossing an active marker whose callback
 * accepts stops at it, counts a pass, and continues at jumpPos (-1: the
 * end): stream loops. layout: host -- holds a code pointer. */
struct RxFileMarkerDef
{
    unsigned int  pos;          // ROR 0x00, file position
    unsigned char active;       // ROR 0x04
    unsigned char id;           // ROR 0x05, for rxFileEnableMarker
    unsigned char count;        // ROR 0x06, passes so far
    unsigned char max;          // ROR 0x07, passes that stop (0: all)
    int           jumpPos;      // ROR 0x08
    int         (*callback)(void *arg, RxFileMarkerDef *marker); // ROR 0x0c, 0: always stop
    int           user;         // ROR 0x10
};

/* Marker pool node (bss 0x7de3b4, 64, 0x18 bytes on the EE). layout: host. */
struct RxFileMarker
{
    RxFileMarker   *next;       // ROR 0x00, sorted by pos
    RxFileMarkerDef def;        // ROR 0x04
};

/* A data tag as the caller passes it (12 bytes). EE layout (plain data). */
struct RxFileTagDef
{
    unsigned int  pos;          // ROR 0x0, file position
    unsigned char active;       // ROR 0x4
    unsigned char id;           // ROR 0x5, for rxFileEnableTag
    unsigned char pad[2];       // ROR 0x6
    int           value;        // ROR 0x8
};
static_assert(sizeof(RxFileTagDef) == 0xc, "RxFileTagDef size");
static_assert(offsetof(RxFileTagDef, pos) == 0x0, "RxFileTagDef::pos");
static_assert(offsetof(RxFileTagDef, active) == 0x4, "RxFileTagDef::active");
static_assert(offsetof(RxFileTagDef, id) == 0x5, "RxFileTagDef::id");
static_assert(offsetof(RxFileTagDef, value) == 0x8, "RxFileTagDef::value");

/* Tag pool node (bss 0x7ddfb0, 64, 0x10 bytes on the EE). layout: host. */
struct RxFileTag
{
    RxFileTag   *next;          // ROR 0x0, sorted by pos
    RxFileTagDef def;           // ROR 0x4
};

/* What rxFileGetRingTags reports: the tag's offset past the ring read
 * position, and its value. EE layout (plain data). */
struct RxFileTagHit
{
    int offset;                 // ROR 0x0
    int value;                  // ROR 0x4
};
static_assert(sizeof(RxFileTagHit) == 0x8, "RxFileTagHit size");
static_assert(offsetof(RxFileTagHit, offset) == 0x0, "RxFileTagHit::offset");
static_assert(offsetof(RxFileTagHit, value) == 0x4, "RxFileTagHit::value");

/* What one transfer put in the ring: file position and size, in a circular
 * list per slot (pool bss 0x7de9b8, 256, 0x10 bytes on the EE; the free
 * list links through next). layout: host. */
struct RxFileSegment
{
    RxFileSegment *prev;        // ROR 0x0
    RxFileSegment *next;        // ROR 0x4
    unsigned int   pos;         // ROR 0x8
    int            size;        // ROR 0xc
};

/* An open file (bss 0x7dabb0, 64 of 0xd0 bytes on the EE; slot 0 is never
 * handed out). layout: host -- holds pointers. */
struct RxFileSlot
{
    int            _00;         // ROR 0x00, untouched
    unsigned char  used;        // ROR 0x04
    unsigned char  paused;      // ROR 0x05, ring refill paused (set at open)
    unsigned char  device;      // ROR 0x06, RX_SYS_DEVICE
    char           path[0x80];  // ROR 0x08
    unsigned int   flags;       // ROR 0x88, open flags
    unsigned int   pos;         // ROR 0x8c, position the requests reached
    unsigned int   size;        // ROR 0x90, file size once known
    unsigned char *ring;        // ROR 0x94
    unsigned int   ringSize;    // ROR 0x98
    unsigned int   ringThreshold; // ROR 0x9c, refill while filled <= this
    unsigned int   ringWrite;   // ROR 0xa0
    unsigned int   ringRead;    // ROR 0xa4
    unsigned int   ringFilled;  // ROR 0xa8
    RxFileMarker  *markers;     // ROR 0xac
    void          *markerArg;   // ROR 0xb0, first argument of the callbacks
    RxFileSegment  segments;    // ROR 0xb4, sentinel of the segment list
    RxFileTag     *tags;        // ROR 0xc4
    volatile unsigned int state; // ROR 0xc8, 1 created, 2 size known, 4 failed
    volatile int   pending;     // ROR 0xcc, requests not served yet
};

/* Called by the file thread when a request is served (or cancelled): the
 * slot, the request's file offset, and the driver's result (the size for a
 * cancel). */
typedef void (*RxFileRequestCallback)(RxFileSlot *slot, int offset, int result);

/* A request in a file thread queue (0x2c bytes on the EE). layout: host. */
struct RxFileRequest
{
    volatile int state;         // ROR 0x00, 0 queued, 1 done or cancelled
    int          device;        // ROR 0x04, before the device map
    const char  *path;          // ROR 0x08, the slot's path
    int          op;            // ROR 0x0c, RX_FILE_OP_*
    int          offset;        // ROR 0x10, file offset (create: truncate)
    void        *buf;           // ROR 0x14
    int          size;          // ROR 0x18
    int          retryMissing;  // ROR 0x1c, retry a -2 (not found) answer
    volatile int busy;          // ROR 0x20, being served
    RxFileRequestCallback callback; // ROR 0x24
    RxFileSlot  *owner;         // ROR 0x28
};

/* 64-entry ring of requests (0xb08 bytes on the EE). layout: host. */
struct RxFileQueue
{
    RxFileRequest req[64];      // ROR 0x000
    volatile int  head;         // ROR 0xb00, next push
    volatile int  tail;         // ROR 0xb04, next to serve
};

/* The file thread (bss 0x7f6910, 0x11620 bytes on the EE): queue 0 for the
 * ring refills and opens, queue 1 for rxFileRead / rxFileWrite. layout: host. */
struct RxFileThread
{
    int           step;         // ROR 0x00, priority + 1, passed to proc
    void        (*proc)(int step); // ROR 0x04, run before every request
    int           thread;       // ROR 0x08, rx thread index
    volatile int  stop;         // ROR 0x0c, 1 asks to stop, -1 stopped
    unsigned char stack[0x10000]; // ROR 0x10
    RxFileQueue   queue[2];     // ROR 0x10010
};

/* The pools rxSysInitModules threads into free lists (bss 0x7dabb0..). */
extern RxFileSlot     rxFileSlots[64];          // bss 0x7dabb0
extern RxFileTag      rxFileTagPool[64];        // bss 0x7ddfb0
extern RxFileTag     *rxFileTagFree;            // bss 0x7de3b0
extern RxFileMarker   rxFileMarkerPool[64];     // bss 0x7de3b4
extern RxFileMarker  *rxFileMarkerFree;         // bss 0x7de9b4
extern RxFileSegment  rxFileSegmentPool[256];   // bss 0x7de9b8
extern RxFileSegment *rxFileSegmentFree;        // bss 0x7df9b8

void   rxFileClose(int fd);                                         // ROR 0x002437f8
void   rxFileAbort(int fd);                                         // ROR 0x00243858
int    rxFileGetState(int fd);                                      // ROR 0x002438b8
int    rxFileRead(int fd, void *buf, int size);                     // ROR 0x00243938
int    rxFileWrite(int fd, const void *buf, int size);              // ROR 0x00243af8
int    rxFileSeek(int fd, int offset, int whence);                  // ROR 0x00243c40
int    rxFileSync(int fd, int noWait);                              // ROR 0x00243cd8
int    rxFileSetMarkers(int fd, const RxFileMarkerDef *defs, int count, void *arg); // ROR 0x00243da8
void   rxFileEnableMarker(int fd, int id, int active);              // ROR 0x00243e08
int    rxFilePause(int fd, int pause);                              // ROR 0x00243e60
unsigned char *rxFileGetRingData(int fd);                           // ROR 0x00243ed8
int    rxFileGetRingReadPos(int fd);                                // ROR 0x00243f28
int    rxFileGetRingSize(int fd);                                   // ROR 0x00243f70
int    rxFileGetRingFilled(int fd);                                 // ROR 0x00243fb8
void   rxFileRingConsume(int fd, int size);                         // ROR 0x00244028
int    rxFileGetRingFilePos(int fd, int offset);                    // ROR 0x00244178
int    rxFileSetTags(int fd, const RxFileTagDef *defs, int count);  // ROR 0x002441d0
void   rxFileEnableTag(int fd, int id, int active);                 // ROR 0x00244230
int    rxFileGetRingTags(int fd, RxFileTagHit *out, int max, int size); // ROR 0x00244288
int    rxFileOpenEx(const char *name, int flags, void *ring, int ringSize, int threshold); // ROR 0x002442e8
int    rxFileOpenSlot(int device, const char *path, int flags, void *ring, int ringSize, int threshold); // ROR 0x00244410
void   rxFileOnCreate(RxFileSlot *slot, int offset, int result);    // ROR 0x002447f8
void   rxFileOnGetSize(RxFileSlot *slot, int offset, int result);   // ROR 0x00244868
void   rxFileSlotClose(RxFileSlot *slot, int abort);                // ROR 0x002448f8
void   rxFileSlotKick(RxFileSlot *slot);                            // ROR 0x002449c0
int    rxFileSlotTransfer(RxFileSlot *slot, int op, void *buf, int size); // ROR 0x00244a38
int    rxFileSlotRefill(RxFileSlot *slot);                          // ROR 0x00244a68
int    rxFileSlotRequest(RxFileSlot *slot, int queue, int op, void *buf, int size); // ROR 0x00244b40
int    rxFileSlotRequestChunked(RxFileSlot *slot, int queue, int op, void *buf, int size); // ROR 0x00244d70
int    rxFileSlotPost(RxFileSlot *slot, int queue, int op, void *buf, int size); // ROR 0x00244f20
void   rxFileOnTransfer(RxFileSlot *slot, int offset, int result);  // ROR 0x00245000
int    rxFileSlotSetMarkers(RxFileSlot *slot, const RxFileMarkerDef *defs, int count, void *arg); // ROR 0x00245078
void   rxFileSlotEnableMarker(RxFileSlot *slot, int id, int active); // ROR 0x00245240
void   rxFileSlotAddSegment(RxFileSlot *slot, int pos, int size);   // ROR 0x00245280
void   rxFileSlotDropSegments(RxFileSlot *slot, int size);          // ROR 0x00245348
void   rxFileSlotFreeSegments(RxFileSlot *slot);                    // ROR 0x00245468
int    rxFileSlotSegmentPos(RxFileSlot *slot, int offset);          // ROR 0x00245520
int    rxFileSlotSetTags(RxFileSlot *slot, const RxFileTagDef *defs, int count); // ROR 0x00245600
void   rxFileSlotEnableTag(RxFileSlot *slot, int id, int active);   // ROR 0x002457a8
int    rxFileSlotGetTags(RxFileSlot *slot, RxFileTagHit *out, int max, int size); // ROR 0x002457e8

/* ---- the file layer --------------------------------------------------- */

void   rxFileCallDevice(int device, const char *root);              // ROR 0x002458b0
char  *rxFileGetDeviceRoot(int device);                             // ROR 0x00245900
int    rxFileCallLoadHook(const char *name, void *buf);             // ROR 0x00245948
int    rxFileCallSizeHook(const char *name);                        // ROR 0x00245988
RxFileLoadHook rxFileSetLoadHook(RxFileLoadHook hook);              // ROR 0x002459b8
RxFileSizeHook rxFileSetSizeHook(RxFileSizeHook hook);              // ROR 0x00245a00
int    rxFileDefaultSizeHook(const char *name);                     // ROR 0x00245a48
int    rxFileDefaultLoadHook(const char *name, void *buf);          // ROR 0x00245ae8
int    rxFileDefaultSaveHook(const char *name, const void *buf, int size); // ROR 0x00245be0
int    rxFileMakePath(char *out, const char *name);                 // ROR 0x00245cb0
int    rxFileLoadExec(const char *path, int argc, char **argv);     // ROR 0x00245db8
int    rxFileLoad(const char *name, void *buf);                     // ROR 0x00245ed8
int    rxFileLoadModule(const char *path, int arglen, const char *args); // ROR 0x00246030
int    rxFileLoadModuleList(const char *const *names, int count);   // ROR 0x00246178
int    rxFileRebootIop(const char *img);                            // ROR 0x002461f0
void   rxFileInit(void);                                            // ROR 0x00246278
void   rxFileTerm(void);                                            // ROR 0x002462b0
RxFileDriver *rxFileGetDriver(int device);                          // ROR 0x002462d8

/* host0: driver */
void   rxFileHostInit(RxFileDriver *drv);                           // ROR 0x00246318
void   rxFileHostTerm(void);                                        // ROR 0x002463a0
void   rxFileHostMakePath(char *out, const char *name);             // ROR 0x002463a8
int    rxFileHostControl(int cmd, int arg);                         // ROR 0x002463c0
int    rxFileHostGetStatus(int what);                               // ROR 0x002463c8
int    rxFileHostGetSize(const char *name);                         // ROR 0x002463f0
int    rxFileHostCreate(const char *name, int truncate);            // ROR 0x002464b0
int    rxFileHostRead(const char *name, void *buf, int offset, int size); // ROR 0x00246560
int    rxFileHostWrite(const char *name, int offset, const void *buf, int size); // ROR 0x00246680
void   rxFileHostJoinRoot(char *out, const char *name);             // ROR 0x002467a0
char  *rxFileHostGetRoot(void);                                     // ROR 0x002467e8
void   rxFileHostSetRoot(const char *root);                         // ROR 0x002467f8

/* cdrom0: driver */
void   rxFileCdWait(void);                                          // ROR 0x00246840
void   rxFileCdSyncWait(void);                                      // ROR 0x00246890
void   rxFileCdInit(RxFileDriver *drv);                             // ROR 0x002468c0
void   rxFileCdTerm(void);                                          // ROR 0x00246950
int    rxFileCdStatusWait(void);                                    // ROR 0x00246958
void   rxFileCdTray(int req);                                       // ROR 0x00246988
void   rxFileCdProcess(int step);                                   // ROR 0x002469a8
void   rxFileCdMakePath(char *out, const char *name);               // ROR 0x00246c70
int    rxFileCdControl(int cmd, int arg);                           // ROR 0x00246cb8
int    rxFileCdGetStatus(int what);                                 // ROR 0x00246d80
int    rxFileCdGetSize(const char *name);                           // ROR 0x00246e00
int    rxFileCdCreate(const char *name, int truncate);              // ROR 0x00246e78
int    rxFileCdRead(const char *name, void *buf, int offset, int size); // ROR 0x00246e80
int    rxFileCdWrite(const char *name, int offset, const void *buf, int size); // ROR 0x00246f68
void   rxFileCdIsoName(char *out, const char *name);                // ROR 0x00246f70
int    rxFileCdMapSector(int lsn);                                  // ROR 0x00247048
int    rxFileCdReadBytes(void *buf, long long offset, int size);    // ROR 0x00247080
int    rxFileCdReadAt(void *buf, long long offset, int size);       // ROR 0x00247308
int    rxFileCdReadSectors(void *buf, int lsn, int count);          // ROR 0x002473c8
void   rxFileCdInitCache(void);                                     // ROR 0x00247608
void   rxFileCdFlushCache(void);                                    // ROR 0x00247668
int    rxFileCdFind(const char *name, int *lsn, int *size);         // ROR 0x002476d0
int    rxFileCdFindDir(int tableOffset, int tableSize, const char *path, int *lsn, int *size); // ROR 0x00247810
int    rxFileCdFindInDir(int dirOffset, const char *name, int *lsn, int *size); // ROR 0x002479e0
const char *rxFileCdPathElem(char *out, const char *path);          // ROR 0x00247bb0
int    rxFileCdIsLastElem(const char *path);                        // ROR 0x00247c10
int    rxFileCdReadCached(void *out, int offset, int size);         // ROR 0x00247c60
char  *rxFileCdGetRoot(void);                                       // ROR 0x00247e88
void   rxFileCdSetRoot(const char *root);                           // ROR 0x00247e98
RxFileCdFindFunc rxFileCdSetFindFunc(RxFileCdFindFunc func);        // ROR 0x00247ea0
int    rxFileCdSetClockRead(int enable);                            // ROR 0x00247eb8
int    rxFileCdGetClockRead(void);                                  // ROR 0x00247f00
int    rxFileCdGetClock(void *clock);                               // ROR 0x00247f10
int    rxFileCdSetLayerMode(int mode);                              // ROR 0x00247fd0
int    rxFileCdGetLayerMode(void);                                  // ROR 0x00248008

/* HDD driver (same shape as host0:, plus the I/O error handler) */
void   rxFileHddInit(RxFileDriver *drv);                            // ROR 0x00248018
void   rxFileHddTerm(void);                                         // ROR 0x002480a0
void   rxFileHddOnError(const char *name, int error);               // ROR 0x002480a8
void   rxFileHddMakePath(char *out, const char *name);              // ROR 0x00248158
int    rxFileHddControl(int cmd, int arg);                          // ROR 0x00248170
int    rxFileHddGetStatus(int what);                                // ROR 0x00248178
int    rxFileHddGetSize(const char *name);                          // ROR 0x002481a0
int    rxFileHddCreate(const char *name, int truncate);             // ROR 0x00248270
int    rxFileHddRead(const char *name, void *buf, int offset, int size); // ROR 0x00248340
int    rxFileHddWrite(const char *name, int offset, const void *buf, int size); // ROR 0x00248480
void   rxFileHddJoinRoot(char *out, const char *name);              // ROR 0x002485c0
char  *rxFileHddGetRoot(void);                                      // ROR 0x00248608
void   rxFileHddSetRoot(const char *root);                          // ROR 0x00248618

/* contexts, devices and dirs */
void   rxFileInitSlots(void);                                       // ROR 0x00248660
void   rxFileUpdate(void);                                          // ROR 0x00248708
int    rxFileSetContext(int index);                                 // ROR 0x00248710
int    rxFileSetCurrent(int device);                                // ROR 0x00248778
int    rxFileGetCurrent(void);                                      // ROR 0x00248870
int    rxFileSetDevice(int ctx, int device);                        // ROR 0x00248890
void   rxFileSetDir(int device, const char *dir);                   // ROR 0x00248998
char  *rxFileGetDir(int device);                                    // ROR 0x00248a60
void   rxFileSetSearchDir(int device, const char *dir);             // ROR 0x00248a80
char  *rxFileGetSearchDir(int device);                              // ROR 0x00248bb0
void   rxFilePushSearchDir(int device);                             // ROR 0x00248bc8
void   rxFilePopSearchDir(int device);                              // ROR 0x00248c50
void   rxFileSetLastName(int device, const char *name);             // ROR 0x00248cb8
char  *rxFileGetLastName(int device);                               // ROR 0x00248ce8
int    rxFileSetDeviceParam(int device, int value);                 // ROR 0x00248d08
int    rxFileGetDeviceParam(int device);                            // ROR 0x00248d38
RxFileLoadHook rxFileGetLoadHook(void);                             // ROR 0x00248d58
RxFileLoadHook rxFileSwapLoadHook(RxFileLoadHook hook);             // ROR 0x00248d78
RxFileSizeHook rxFileGetSizeHook(void);                             // ROR 0x00248da8
RxFileSizeHook rxFileSwapSizeHook(RxFileSizeHook hook);             // ROR 0x00248dc8
RxFileDevInfo *rxFileGetDeviceInfo(int device);                     // ROR 0x00248df8
RxFileDirInfo *rxFileGetDirInfo(int device);                        // ROR 0x00248e30
RxFileContext *rxFileGetContext(void);                              // ROR 0x00248e70
const char *rxFilePathNextElem(char *out, const char *path);        // ROR 0x00248e98
void   rxFilePathUp(char *dir);                                     // ROR 0x00248f00
int    rxFilePathCountDots(const char *elem);                       // ROR 0x00248f88

/* the file thread */
void   rxFileStartThread(void);                                     // ROR 0x00248fd8
void   rxFileStopThread(void);                                      // ROR 0x00249000
int    rxFilePostRequest(int queue, int device, const char *path, int op, int offset,
                         void *buf, int size, int retryMissing,
                         RxFileRequestCallback callback, RxFileSlot *owner); // ROR 0x00249098
void   rxFileCancelRequests(int device, RxFileSlot *owner, int abort); // ROR 0x002490e0
void   rxFileCreateThread(int index, int priority, void (*proc)(int step)); // ROR 0x002491c0
void   rxFileThreadMain(void);                                      // ROR 0x002492b0
void   rxFileThreadProc(int step);                                  // ROR 0x002495e0
void   rxFileNop0(void);                                            // ROR 0x002495f8
void   rxFileNop1(void);                                            // ROR 0x00249600
void   rxFileQueueInit(RxFileQueue *q);                             // ROR 0x00249608
int    rxFileQueuePush(RxFileQueue *q, int device, const char *path, int op, int offset,
                       void *buf, int size, int retryMissing,
                       RxFileRequestCallback callback, RxFileSlot *owner); // ROR 0x00249618
RxFileRequest *rxFileQueueFront(RxFileQueue *q);                    // ROR 0x00249778
void   rxFileQueuePop(RxFileQueue *q);                              // ROR 0x002497c0
int    rxFileQueueCount(RxFileQueue *q);                            // ROR 0x00249818
int    rxFileQueueSpace(RxFileQueue *q);                            // ROR 0x00249838
void   rxFileQueueCancel(RxFileQueue *q, RxFileSlot *owner, int abort); // ROR 0x00249868

/* device map and the HDD error hook */
void   rxFileSetDeviceMap(int device, int target);                  // ROR 0x002499b8
int    rxFileGetDeviceMap(int device);                              // ROR 0x00249a68
void   rxFileSetFallbackDevice(int device, int fallback);           // ROR 0x00249ad0
int    rxFileGetFallbackDevice(int device);                         // ROR 0x00249ae8
void   rxFileSetErrorHook(RxFileErrorHook hook);                    // ROR 0x00249b00
RxFileErrorHook rxFileGetErrorHook(void);                           // ROR 0x00249b10
int    rxFileSetThreadPriority(int priority);                       // ROR 0x00249b20
int    rxFileGetThreadPriority(void);                               // ROR 0x00249b78

#endif /* _RX_RXFILE_H */
