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
 *  Not reconstructed yet: the async file slots (0x2437f8..0x245800), the
 *  default hooks, the cdrom driver's file functions and the file thread.
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

/* ---- the file layer --------------------------------------------------- */

void   rxFileCallDevice(int device, const char *root);              // ROR 0x002458b0
char  *rxFileGetDeviceRoot(int device);                             // ROR 0x00245900
int    rxFileCallLoadHook(const char *name, void *buf);             // ROR 0x00245948
int    rxFileCallSizeHook(const char *name);                        // ROR 0x00245988
RxFileLoadHook rxFileSetLoadHook(RxFileLoadHook hook);              // ROR 0x002459b8
RxFileSizeHook rxFileSetSizeHook(RxFileSizeHook hook);              // ROR 0x00245a00
int    rxFileDefaultSizeHook(const char *name);                     // ROR 0x00245a48
int    rxFileDefaultLoadHook(const char *name, void *buf);          // ROR 0x00245ae8
int    rxFileMakePath(char *out, const char *name);                 // ROR 0x00245cb0
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
void   rxFileCdInit(RxFileDriver *drv);                             // ROR 0x002468c0
void   rxFileCdTerm(void);                                          // ROR 0x00246950
void   rxFileCdProcess(void);                                       // ROR 0x002469a8
void   rxFileCdMakePath(char *out, const char *name);               // ROR 0x00246c70
int    rxFileCdControl(int cmd, int arg);                           // ROR 0x00246cb8
int    rxFileCdGetStatus(int what);                                 // ROR 0x00246d80
int    rxFileCdGetSize(const char *name);                           // ROR 0x00246e00
int    rxFileCdCreate(const char *name, int truncate);              // ROR 0x00246e78
int    rxFileCdRead(const char *name, void *buf, int offset, int size); // ROR 0x00246e80
int    rxFileCdWrite(const char *name, int offset, const void *buf, int size); // ROR 0x00246f68
void   rxFileCdInitCache(void);                                     // ROR 0x00247608
void   rxFileCdFlushCache(void);                                    // ROR 0x00247668
char  *rxFileCdGetRoot(void);                                       // ROR 0x00247e88
void   rxFileCdSetRoot(const char *root);                           // ROR 0x00247e98

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
void   rxFileCreateThread(int index, int priority, void (*proc)(void)); // ROR 0x002491c0
void   rxFileThreadProc(void);                                      // ROR 0x002495e0

/* device map and the HDD error hook */
void   rxFileSetDeviceMap(int device, int target);                  // ROR 0x002499b8
int    rxFileGetDeviceMap(int device);                              // ROR 0x00249a68
int    rxFileGetFallbackDevice(int device);                         // ROR 0x00249ae8
RxFileErrorHook rxFileGetErrorHook(void);                           // ROR 0x00249b10

#endif /* _RX_RXFILE_H */
