/* ==========================================================================
 *  rx/rxsys.h
 *
 *  The rx middleware's system layer (ROR 0x23ff58..0x2437f8, no source-path
 *  string: the file name comes from its "rxSys:" / "rxsys :" messages). It
 *  brings the console up for the game: hardware reset, the boot device and
 *  path, the IOP reboot and module list (iopconf.cnf), system.ini, the
 *  memory hooks the game installs, and rxSysPrintf, the printf every rx and
 *  game module reports through. main (0x2045a8) calls rxSysSetIopImage,
 *  rxSysSetBootPath, the rxSysSetMemFunc* setters and rxSysInit.
 * ======================================================================== */
#ifndef _RX_RXSYS_H
#define _RX_RXSYS_H

#include <stdint.h>

#include "sdk/eekernel.h"
#include "sdk/sifrpc.h"

/* Boot devices: the index into the device-ready flags and the "Device[%s]"
 * names (0x2dbc38). rxSysDetectDevice picks HOST on a devkit (more than
 * 32 MB), CDROM on a retail console. */
enum RX_SYS_DEVICE
{
    RX_SYS_DEVICE_HOST  = 0,
    RX_SYS_DEVICE_CDROM = 1,
    RX_SYS_DEVICE_MCARD = 2,
    RX_SYS_DEVICE_HDD   = 3,
    RX_SYS_DEVICE_NUM   = 4,
};

/* The four memory hooks (data 0x2dbe8c..0x2dbe98), read directly by the
 * game's allocators (0x1830fc, 0x182818 ...). Their signatures differ (main
 * installs 0x105ca0 twice, 0x105de0 and 0x105e08), so they are kept as plain
 * code pointers and cast at the call. Unset ones point at rxSysNoMemFunc. */
typedef void (*RxSysMemFunc)(void);
extern RxSysMemFunc g_rxSysMemFunc[4];          /* data 0x2dbe8c */

/* An IOP RPC client with its call semaphore (rxSysBindRpc / rxSysCallRpc;
 * no caller in the retail ELF). EE: the semaphore at 0, the client data at
 * 0x40. layout: host -- sceSifClientData holds pointers. */
struct RxSysRpc
{
    int              sema;      // ROR 0x00
    sceSifClientData client;    // ROR 0x40
};

/* The print filter rxSysPrintf asks first: 0 drops the line. */
typedef int (*RxSysPrintHook)(const char *text);

int          rxSysNoMemFunc(void);                              // ROR 0x0023ff58
RxSysMemFunc rxSysSetMemFunc0(RxSysMemFunc func);               // ROR 0x0023ff80
RxSysMemFunc rxSysSetMemFunc1(RxSysMemFunc func);               // ROR 0x0023ffb0
RxSysMemFunc rxSysSetMemFunc2(RxSysMemFunc func);               // ROR 0x0023ffe0
RxSysMemFunc rxSysSetMemFunc3(RxSysMemFunc func);               // ROR 0x00240010

int         rxSysStrLenToEol(const char *s);                    // ROR 0x00240040
int         rxSysStrLenToSpace(const char *s);                  // ROR 0x00240080
char       *rxSysSkipEol(char *s);                              // ROR 0x002400c0
char       *rxSysSkipSpace(char *s);                            // ROR 0x00240100
void        rxSysStrCopyN(char *dst, const char *src, int n);   // ROR 0x00240140
void        rxSysPathCopy(char *dst, const char *src, int n);   // ROR 0x00240180
int         rxSysIsModuleLoaded(const char *name);              // ROR 0x002401e8
char       *rxSysPathFileName(char *path);                      // ROR 0x00240280
char       *rxSysPathExt(char *path);                           // ROR 0x00240338
int         rxSysIsCommentLine(const char *line);               // ROR 0x002403c0
int         rxSysSkipModule(char *path);                        // ROR 0x002403f0
void        rxSysAddModule(const char *name);                   // ROR 0x00240440
void        rxSysAddModuleList(const char *const *names);       // ROR 0x00240490
void        rxSysSetCdMode(void);                               // ROR 0x002404d8
void        rxSysRebootIop(const char *img);                    // ROR 0x00240520
int         rxSysLoadModuleList(char *conf);                    // ROR 0x002405d8
int         rxSysReadIopConf(char *buf, const char *dir);       // ROR 0x002407b0
void        rxSysCdInit(int mode);                              // ROR 0x00240838
void        rxSysLoadIopModules(void);                          // ROR 0x00240890
int         rxSysIsDeviceReady(int device);                     // ROR 0x00240a88
int         rxSysGetBootDevice(void);                           // ROR 0x00240ab0
void        rxSysDetectDevice(void);                            // ROR 0x00240ac0
int         rxSysFindString(const char *s, const char *const *table); // ROR 0x00240b40
void        rxSysReadSystemIni(void);                           // ROR 0x00240bb0
void        rxSysResetHardware(void);                           // ROR 0x00240df0
void        rxSysReadRomDate(void);                             // ROR 0x00240e60
void        rxSysInit(void);                                    // ROR 0x00240eb8
int         rxSysIniFind(char *out, const char *text, const char *section, const char *key); // ROR 0x002413d8
char       *rxSysIniFindSection(char *text, const char *section);  // ROR 0x002410b8
char       *rxSysIniReadKey(char *out, char *line);             // ROR 0x002411d0
char       *rxSysIniFindKey(char *body, const char *key);       // ROR 0x00241298
int         rxSysIniCountKeys(char *body);                      // ROR 0x00241558
int         rxSysIniKeyAt(char *out, char *body, int index);    // ROR 0x00241620
int         rxSysIniFindKeyCount(char *text, const char *section);  // ROR 0x00241720
int         rxSysIniFindKeyAt(char *out, char *text, const char *section, int index); // ROR 0x00241758
int         rxSysIniGetString(char *out, const char *section, const char *key); // ROR 0x002417c0
int         rxSysIniGetKeyCount(const char *section);           // ROR 0x00241808
int         rxSysIniGetKeyAt(char *out, const char *section, int index); // ROR 0x00241848
void        rxSysTtyWrite(const char *buf, int size);           // ROR 0x002419a8
int         rxSysBindTtyRpc(sceSifClientData *client, unsigned int id); // ROR 0x00241890
void        rxSysBindTty(void);                                 // ROR 0x00241968
void        rxSysOpenTty(void);                                 // ROR 0x00241a00
void        rxSysCloseTty(void);                                // ROR 0x00241a80
void        rxSysSetIopImage(const char *img);                  // ROR 0x00241b20
const char *rxSysGetProductNumber(void);                        // ROR 0x00241b90
/* Closes the tty, the semaphore, the file devices, the IOP RPC and the
 * threads (SndMan::term). */
void        rxSysTerm(void);                                    // ROR 0x00241ac8
int         rxSysGetVMode(void);                                // ROR 0x00241b50
int         rxSysGetFrameRate(void);                            // ROR 0x00241b60
int         rxSysGetCountry(void);                              // ROR 0x00241bb0
int         rxSysGetMemorySize(void);                           // ROR 0x00241c00
int         rxSysIsCdromPath(const char *path);                 // ROR 0x00241c18
char       *rxSysPathSkipDevice(char *path);                    // ROR 0x00241ca0
void        rxSysSetBootPath(char *path);                       // ROR 0x00241d68
RxSysPrintHook rxSysSetPrintHook(RxSysPrintHook hook);          // ROR 0x00241f08
int         rxSysPrintWait(intptr_t *args);                     // ROR 0x00241f20
int         rxSysIsTtyOpen(void);                               // ROR 0x00241f28
int         rxSysPrintf(const char *fmt, ...);                  // ROR 0x00241f58
void        rxSysInitEnd(void);                                 // ROR 0x00242150
void        rxSysTermEnd(void);                                 // ROR 0x00242158
int         rxSysBindRpc(RxSysRpc *rpc, unsigned int id);       // ROR 0x00242188
void        rxSysUnbindRpc(RxSysRpc *rpc);                      // ROR 0x00242268
void        rxSysCallRpc(RxSysRpc *rpc, int fno, void *send, int sendSize,
                         void *recv, int recvSize);             // ROR 0x002422a8
void        rxSysRpcDone(void *rpc);                            // ROR 0x00242368
void        rxSysTermSema(void);                                // ROR 0x002423d0
int         rxSysCreateSema(int max, int init);                 // ROR 0x00242160
void        rxSysInitSema(void);                                // ROR 0x00242390
void        rxSysLockPrint(void);                               // ROR 0x00242400
void        rxSysUnlockPrint(void);                             // ROR 0x00242460
void        rxSysCheckCdPath(const char *path);                 // ROR 0x00242500
int         rxSysOpen(const char *path, int mode);              // ROR 0x00242528
int         rxSysClose(int fd);                                 // ROR 0x00242578
int         rxSysRead(int fd, void *buf, int size);             // ROR 0x002425b0
int         rxSysWrite(int fd, const void *buf, int size);      // ROR 0x00242608
int         rxSysLseek(int fd, int offset, int whence);         // ROR 0x00242660
int         rxSysIoctl(int fd, int req, void *arg);             // ROR 0x002426b8
void        rxSysLockConsole(void);                             // ROR 0x00242710
void        rxSysUnlockConsole(void);                           // ROR 0x00242728
void        rxSysExceptionHandler(u_int stat, u_int cause, u_int epc, u_int bva, u_int bpa,
                                  u_long128 *gpr);              // ROR 0x00242740
void        rxSysSetDebugHandlers(void);                        // ROR 0x00242960
void        rxSysRestoreDebugHandlers(void);                    // ROR 0x00242a10
void        rxSysHexDump(const char *p, int size);              // ROR 0x00242aa8
void        rxSysInitExceptionScreen(void);                     // ROR 0x002431c0
void        rxSysLockSema1(void);                               // ROR 0x00242480
void        rxSysUnlockSema1(void);                             // ROR 0x002424e0
void        rxSysInitUser(void);                                // ROR 0x002433d8
void        rxSysTermUser(void);                                // ROR 0x002433e0
void        rxSysThreadCallback(int yielding);                  // ROR 0x002433e8
void        rxSysInitModules(void);                             // ROR 0x00243400
void        rxSysTermModules(void);                             // ROR 0x002434f0
/* rxSysThreadCallback's yielding flag picks rxfile's context: 1 while the
 * main thread yields to the rx thread, 0 when it runs again. */
int         rxSysSetFileContext(int index);                     // ROR 0x00243560
int         rxSysDeviceControl(int device, int cmd, int arg);   // ROR 0x00243578
int         rxSysDeviceStatus(int device, int what);            // ROR 0x002435c8
int         rxSysInitFileDevice(int device);                    // ROR 0x00243608
int         rxSysGetFileDevice(void);                           // ROR 0x00243620
int         rxSysSetFileDevice(int slot, int device);           // ROR 0x00243638
void        rxSysSetFileDir(const char *dir);                   // ROR 0x00243650
char       *rxSysGetFileDir(void);                              // ROR 0x00243680
void        rxSysSetFileSearchDir(const char *dir);             // ROR 0x002436a0
char       *rxSysGetFileSearchDir(void);                        // ROR 0x002436d0
void        rxSysPushSearchDir(void);                           // ROR 0x002436f0
void        rxSysPopSearchDir(void);                            // ROR 0x00243710
char       *rxSysGetLastName(void);                             // ROR 0x00243730
int         rxSysSetDeviceParam(int value);                     // ROR 0x00243750
int         rxSysStreamOpen(const char *name, int flags);       // ROR 0x00243788
int         rxSysStreamOpenRing(const char *name, void *ring, int ringSize, int threshold); // ROR 0x002437a8
int         rxSysStreamOpenPath(int device, const char *path, int flags, void *ring,
                                int ringSize, int threshold);   // ROR 0x002437d8

#endif /* _RX_RXSYS_H */
