/* ==========================================================================
 *  rx/rxfile.h
 *
 *  The rx middleware's file layer ("rxfile : ..." messages, ROR 0x2437f8 on;
 *  its extent is not pinned yet): file slots and devices, whole-file loads,
 *  IOP reboot and module loads, the request thread. Only what rxsys calls is
 *  declared so far; the TU that reconstructs rxfile completes it.
 * ======================================================================== */
#ifndef _RX_RXFILE_H
#define _RX_RXFILE_H

int  rxFileCallDevice(int device, const char *arg);             // ROR 0x002458b0
int  rxFileLoad(const char *name, void *buf);                   // ROR 0x00245ed8
int  rxFileLoadModule(const char *path, int arglen, const char *args); // ROR 0x00246030
int  rxFileRebootIop(const char *img);                          // ROR 0x002461f0
void rxFileInit(void);                                          // ROR 0x00246278
void rxFileInitSlots(void);                                     // ROR 0x00248660
int  rxFileGetCurrent(void);                                    // ROR 0x00248870
int  rxFileSetDevice(int slot, int device);                     // ROR 0x00248890
void rxFileSetDir(int ctx, const char *dir);                    // ROR 0x00248998
char *rxFileGetDir(int ctx);                                    // ROR 0x00248a60
void rxFileSetSearchDir(int ctx, const char *dir);              // ROR 0x00248a80
char *rxFileGetSearchDir(int ctx);                              // ROR 0x00248bb0
void rxFileStartThread(void);                                   // ROR 0x00248fd8

#endif /* _RX_RXFILE_H */
