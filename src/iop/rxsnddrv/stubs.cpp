/* ==========================================================================
 *  stubs.cpp  (RXSNDDRV.IRX: the subsystems not reconstructed yet)
 *
 *  Each answers 0 and says once that it ran.  Replace a stub by its
 *  reconstruction in the subsystem's own file (rxsnddrv.h lists them) and
 *  delete it here.
 * ======================================================================== */

#include "iop/rxsnddrv/rxsnddrv.h"

#include <stdio.h>

static void RxSndDrvStubHit(int *once, const char *addr)
{
    if (!*once)
    {
        *once = 1;
        printf("rxsnddrv: stub IOP %s\n", addr);
    }
}

#define RXSNDDRV_STUB(addr) do { static int once; RxSndDrvStubHit(&once, addr); } while (0)

// stub of IOP 0x00006ab0
void RxSndDrvSeInit(void)
{
    RXSNDDRV_STUB("0x00006ab0");
}

// stub of IOP 0x00006bcc
void RxSndDrvSeSet(u_int voice, const u_int *data)
{
    (void)data;
    (void)voice;
    RXSNDDRV_STUB("0x00006bcc");
}

// stub of IOP 0x000084b0
int RxSndDrvSePlay(int voice, int xvab, int prog, int note)
{
    (void)note;
    (void)prog;
    (void)xvab;
    (void)voice;
    RXSNDDRV_STUB("0x000084b0");
    return 0;
}

// stub of IOP 0x00008658
int RxSndDrvSePlay2(int voice, int xvab, int prog, int note)
{
    (void)note;
    (void)prog;
    (void)xvab;
    (void)voice;
    RXSNDDRV_STUB("0x00008658");
    return 0;
}

// stub of IOP 0x00008800
int RxSndDrvSeStatus(u_char *reply)
{
    (void)reply;
    RXSNDDRV_STUB("0x00008800");
    return 0;
}

// stub of IOP 0x00008878
void RxSndDrvSeTick(void)
{
    RXSNDDRV_STUB("0x00008878");
}

// stub of IOP 0x000093c0
void RxSndDrvNoteTick(void)
{
    RXSNDDRV_STUB("0x000093c0");
}

// stub of IOP 0x000098e4
void RxSndDrvNoteInit(void)
{
    RXSNDDRV_STUB("0x000098e4");
}

// stub of IOP 0x00008f10: no NOTE yet, so no note starts
int RxSndDrvNotePlay(int xvab, int program, int key, int a, int b)
{
    (void)b;
    (void)a;
    (void)key;
    (void)program;
    (void)xvab;
    RXSNDDRV_STUB("0x00008f10");
    return -1;
}

// stub of IOP 0x00009974
void RxSndDrvNoteKeyOff(int note)
{
    (void)note;
    RXSNDDRV_STUB("0x00009974");
}

// stub of IOP 0x000099c8
void RxSndDrvNoteFree(int note)
{
    (void)note;
    RXSNDDRV_STUB("0x000099c8");
}

// stub of IOP 0x00009ab0
void RxSndDrvNoteSetSwitch(int note, int on)
{
    (void)on;
    (void)note;
    RXSNDDRV_STUB("0x00009ab0");
}

// stub of IOP 0x00009ae4
void RxSndDrvNoteSetVolume(int note, short l, short r)
{
    (void)r;
    (void)l;
    (void)note;
    RXSNDDRV_STUB("0x00009ae4");
}

// stub of IOP 0x00009b1c
void RxSndDrvNoteSetVelocity(int note, int velocity)
{
    (void)velocity;
    (void)note;
    RXSNDDRV_STUB("0x00009b1c");
}

// stub of IOP 0x00009bb0
void RxSndDrvNoteSetPan(int note, int pan)
{
    (void)pan;
    (void)note;
    RXSNDDRV_STUB("0x00009bb0");
}

// stub of IOP 0x00009c0c
void RxSndDrvNoteSetPitch(int note, int scale)
{
    (void)scale;
    (void)note;
    RXSNDDRV_STUB("0x00009c0c");
}

// stub of IOP 0x00009c40
void RxSndDrvNoteSetBend(int note, int bend)
{
    (void)bend;
    (void)note;
    RXSNDDRV_STUB("0x00009c40");
}

// stub of IOP 0x00009db4
u_int RxSndDrvNoteGetFlags(int note)
{
    (void)note;
    RXSNDDRV_STUB("0x00009db4");
    return 0;
}

// stub of IOP 0x00009eb0
int RxSndDrvAdpcmOpen(int id, int pitch, const u_int *data, int a, u_int iop, int size, int b)
{
    (void)b;
    (void)size;
    (void)iop;
    (void)a;
    (void)data;
    (void)pitch;
    (void)id;
    RXSNDDRV_STUB("0x00009eb0");
    return 0;
}

// stub of IOP 0x0000acac
void RxSndDrvAdpcmInit(void)
{
    RXSNDDRV_STUB("0x0000acac");
}

// stub of IOP 0x0000ad94
void RxSndDrvAdpcmData(int id, int size, int a)
{
    (void)a;
    (void)size;
    (void)id;
    RXSNDDRV_STUB("0x0000ad94");
}

// stub of IOP 0x0000aea0
void RxSndDrvAdpcmSet(u_int id, const u_int *data)
{
    (void)data;
    (void)id;
    RXSNDDRV_STUB("0x0000aea0");
}

// stub of IOP 0x0000b01c
void RxSndDrvAdpcmChannelSet(int a, int b, const u_int *data)
{
    (void)data;
    (void)b;
    (void)a;
    RXSNDDRV_STUB("0x0000b01c");
}

// stub of IOP 0x0000b160
int RxSndDrvAdpcmStatus(u_char *reply)
{
    (void)reply;
    RXSNDDRV_STUB("0x0000b160");
    return 0;
}

// stub of IOP 0x0000b1dc
void RxSndDrvAdpcmTick(void)
{
    RXSNDDRV_STUB("0x0000b1dc");
}

// stub of IOP 0x0000b4a0
void RxSndDrvPcmInit(void)
{
    RXSNDDRV_STUB("0x0000b4a0");
}

// stub of IOP 0x0000b5ac
void RxSndDrvPcmTick(void)
{
    RXSNDDRV_STUB("0x0000b5ac");
}

// stub of IOP 0x0000c944
int RxSndDrvPcmOpen(int id, int a, u_int b, u_int c)
{
    (void)c;
    (void)b;
    (void)a;
    (void)id;
    RXSNDDRV_STUB("0x0000c944");
    return 0;
}

// stub of IOP 0x0000cabc
void RxSndDrvPcmData(int id, const u_int *data, int a)
{
    (void)a;
    (void)data;
    (void)id;
    RXSNDDRV_STUB("0x0000cabc");
}

// stub of IOP 0x0000cc28
void RxSndDrvPcmSet(u_int id, const u_int *data)
{
    (void)data;
    (void)id;
    RXSNDDRV_STUB("0x0000cc28");
}

// stub of IOP 0x0000cd58
int RxSndDrvPcmStatus(u_char *reply)
{
    (void)reply;
    RXSNDDRV_STUB("0x0000cd58");
    return 0;
}
