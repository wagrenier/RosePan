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
