/* ==========================================================================
 *  rx/rxsnd.h
 *
 *  The rx sound library (0x237ce8..~0x23f960, unassigned@00237ce8, no file
 *  string; "rxsnd :" messages): an EE client of an IOP sound module that
 *  queues commands and flushes them once a frame. XVAB banks of sound
 *  effects (SE voices), XSEQ sequences, BGM, PCM and ADPCM streams.
 *  Names from its own strings (rxSndSeStop, rxSndAdpcmClose ...) and from
 *  how SndMan uses them. Minimal: only what SndMan calls.
 * ======================================================================== */
#ifndef _RX_RXSND_H
#define _RX_RXSND_H

#include "sdk/eetypes.h"

void  rxSndInit(void);                                                   // ROR 0x00237d80
void  rxSndTerm(void);                                                   // ROR 0x00237db0
/* Once a frame: flushes the queued IOP commands. */
void  rxSndUpdate(void);                                                 // ROR 0x00237eb0
void  rxSndSetMasterVolume(int volL, int volR);                          // ROR 0x00238188

/* XVAB banks: Load returns the bank's id (id -1 takes a free one), -1 on
 * failure; Check is 1 while the transfer runs (wait 1 waits for it). */
int   rxSndXvabLoad(int id, const char *name, int arg2, int arg3);       // ROR 0x0023a1b8
int   rxSndXvabFree(int id, int arg);                                    // ROR 0x0023a5a8
int   rxSndXvabCheck(int id, int wait);                                  // ROR 0x0023a720

/* SE voices of an XVAB. Status: bit 0 open, bit 2 ended; 0 when closed. */
int   rxSndSePlay(int xvab, int prog, int note, int volL, int volR, int arg); // ROR 0x0023be68
void  rxSndSeStop(int voice, int arg);                                   // ROR 0x0023c0f8
u_int rxSndSeGetStatus(int voice);                                       // ROR 0x0023c228
void  rxSndSePause(int voice, int pause, int arg);                       // ROR 0x0023c260
/* IOP command 0x82, parameter 2 (pan or an effect level: not known). */
void  rxSndSeSetAttr(int voice, int value);                              // ROR 0x0023c328
void  rxSndSeSetVolume(int voice, int volL, int volR);                   // ROR 0x0023c3c0
void  rxSndSeSetPitch(int voice, float pitch);                           // ROR 0x0023c510

/* ADPCM streams fed from memory. Status: bit 0 open, bit 2 ended, bit 3
 * paused. GetFree: the bytes Write can take now. */
int   rxSndAdpcmOpen(int mode, int rate, int arg2, int arg3, int arg4, int arg5); // ROR 0x0023dcc8
void  rxSndAdpcmClose(int stream);                                       // ROR 0x0023e020
u_int rxSndAdpcmGetStatus(int stream);                                   // ROR 0x0023e158
int   rxSndAdpcmGetFree(int stream);                                     // ROR 0x0023e240
void  rxSndAdpcmWrite(int stream, const void *data, int offset, int size); // ROR 0x0023eb10
/* No more data: the stream plays out what it has. */
void  rxSndAdpcmEnd(int stream);                                         // ROR 0x0023eb28
void  rxSndAdpcmPause(int stream, int pause);                            // ROR 0x0023eef8
/* IOP command 0xa2, parameter 2 (the stream twin of rxSndSeSetAttr). */
void  rxSndAdpcmSetAttr(int stream, int value);                          // ROR 0x0023ef98
void  rxSndAdpcmSetVolume(int stream, int volL, int volR);               // ROR 0x0023f038
void  rxSndAdpcmSetPitch(int stream, float pitch);                       // ROR 0x0023f310

#endif /* _RX_RXSND_H */
