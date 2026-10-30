/* ==========================================================================
 *  rxsnddrv.cpp  (RXSNDDRV.IRX 0x0000..0x0b08: entry, tick thread, dispatcher)
 *
 *  _start clears the module's bss, brings up libsd and SIF RPC, initialises
 *  every subsystem, starts the RPC server and the "sound double buffer" thread
 *  and stays resident.  The tick thread runs once per V-blank: flip the
 *  command bank, then every subsystem's tick in a fixed order.  The dispatcher
 *  is one switch on the EE's command numbers (rx/rxsnd.h); command 0x200 is a
 *  batch of queued commands {cmd, replySize, dataSize, data} closed by cmd 0,
 *  each one's reply laid after the last.
 * ======================================================================== */

#include "iop/rxsnddrv/rxsnddrv.h"

#include "intrman.h"
#include "libsd.h"
#include "loadcore.h"
#include "sifcmd.h"
#include "sysmem.h"
#include "thbase.h"
#include "vblank.h"

#include <stdio.h>
#include <string.h>

static const u_short s_version = 0x0101;    // IOP 0x0000dd64: module version 1.01

static int          s_tickThread;           // IOP 0x0000e4f0
static volatile int s_cmdBank;              // IOP 0x0000e4f4: the bank commands write
static volatile int s_stopRequest;          // IOP 0x0000e4f8
static volatile int s_stopped;              // IOP 0x0000e4fc
static u_char       s_zero[0x1000];         // IOP 0x0000e500: zeros for SPU clears

// IOP 0x00000000
int RxSndDrvStart(void)
{
    FlushDcache();
    if (RxSndDrvGetDebugFlags() & RXSNDDRV_DBG_INFO)
        printf("rxsnddrv : clear BSS section %08X - %08X\n", 0xe4f0, 0x38970);
    /* PC: the bss is the host's zero-initialised statics; nothing to clear. */
    sceSdInit(0);
    sceSifInitRpc(0);
    memset(s_zero, 0, sizeof(s_zero));
    RxSndDrvSysInit();
    RxSndDrvMixInit();
    RxSndDrvEffectInit();
    RxSndDrvDmaInit();
    RxSndDrvDmaInit2();
    RxSndDrvXvabInit();
    RxSndDrvXseqInit();
    RxSndDrvVoiceInit();
    RxSndDrvNoteInit();
    RxSndDrvSeInit();
    RxSndDrvBgmInit();
    RxSndDrvAdpcmInit();
    RxSndDrvPcmInit();
    RxSndDrvRpcStart();
    RxSndDrvStartTick();
    if (RxSndDrvGetDebugFlags() & RXSNDDRV_DBG_INFO)
        printf("Installed sound driver version 1.00\n");
    if (RxSndDrvGetDebugFlags() & RXSNDDRV_DBG_INFO)
        printf("free memory    : %X byte\n", QueryTotalFreeMemSize());
    if (RxSndDrvGetDebugFlags() & RXSNDDRV_DBG_INFO)
        printf("free max block : %X byte\n", QueryMaxFreeMemSize());
    return 0;
}

/* Command 0x10, and the end of _start: create and start the tick thread. */
// IOP 0x00000190
int RxSndDrvStartTick(void)
{
    ThreadParam param;

    s_tickThread  = 0;
    s_cmdBank     = 0;
    s_stopRequest = 0;
    s_stopped     = 0;
    param.attr         = TH_C;
    param.entry        = (void *)RxSndDrvTickThread;
    param.initPriority = 0x23;
    param.stackSize    = 0x1000;
    param.option       = 0;
    s_tickThread = CreateThread(&param);
    if (s_tickThread <= 0)
    {
        printf("failed to create sound double buffer thread\n");
        return 0;
    }
    StartThread(s_tickThread, 0);
    return 1;
}

/* Command 0x11: ask the tick thread to stop; answers whether it has. */
// IOP 0x00000238
int RxSndDrvStopTick(void)
{
    s_stopRequest = 1;
    return s_stopped;
}

/* The bank the tick applies: the one the commands of the last frame went to. */
// IOP 0x00000254
int RxSndDrvTickBank(void)
{
    return s_cmdBank == 0;
}

// IOP 0x00000268
int RxSndDrvCmdBank(void)
{
    return s_cmdBank;
}

/* Command 0x12. */
// IOP 0x0000027c
int RxSndDrvGetVersion(void)
{
    return s_version;
}

// IOP 0x0000028c
u_char *RxSndDrvZeroBuffer(void)
{
    return s_zero;
}

// IOP 0x0000029c
void RxSndDrvTickThread(void)
{
    while (!s_stopRequest)
    {
        int intr;

        /* PC: the host's WaitVblankEnd answers < 0 at shutdown. */
        if (WaitVblankEnd() < 0)
            return;
        CpuSuspendIntr(&intr);
        s_cmdBank = s_cmdBank == 0;
        CpuResumeIntr(intr);
        RxSndDrvSysTick();
        RxSndDrvMixTick();
        RxSndDrvEffectTick();
        RxSndDrvBgmTick();
        RxSndDrvSeTick();
        RxSndDrvNoteTick();
        RxSndDrvAdpcmTick();
        RxSndDrvVoiceTick();
        RxSndDrvPcmTick();
    }
    s_stopped = 1;
}

/* The arguments as the commands pack them: words, with bytes and halves
 * read out of the first words. */
static inline int ArgByte(const u_int *data, int i)
{
    return ((const u_char *)data)[i];
}

static inline int ArgHalf(const u_int *data, int i)
{
    return ((const short *)data)[i];
}

// IOP 0x00000360
u_int RxSndDrvCommand(u_int cmd, u_char *reply, const u_int *data)
{
    u_int ret = 0;
    int   intr;

    switch (cmd)
    {
    case 0x10:
        return RxSndDrvStartTick();
    case 0x11:
        return RxSndDrvStopTick();
    case 0x12:
        return RxSndDrvGetVersion();

    /* The IOP heap, for the EE's rxSndIopAlloc / Free. */
    case 0x20:
        CpuSuspendIntr(&intr);
        ret = (u_int)(uintptr_t)AllocSysMemory((int)data[0], (int)data[1],
                                               (void *)(uintptr_t)data[2]);
        CpuResumeIntr(intr);
        return ret;
    case 0x21:
        CpuSuspendIntr(&intr);
        FreeSysMemory((void *)(uintptr_t)data[0]);
        CpuResumeIntr(intr);
        return 0;
    case 0x22:
        CpuSuspendIntr(&intr);
        switch (data[0])
        {
        case 0: ret = QueryMemSize();          break;
        case 1: ret = QueryMaxFreeMemSize();   break;
        case 2: ret = QueryTotalFreeMemSize(); break;
        }
        CpuResumeIntr(intr);
        return ret;
    case 0x23:
    {
        /* {dst, src, size}: an overlap-safe copy inside IOP memory. */
        u_char       *dst  = (u_char *)(uintptr_t)data[0];
        const u_char *src  = (const u_char *)(uintptr_t)data[1];
        int           size = (int)data[2];

        if (src <= dst)
        {
            dst += size;
            src += size;
            for (int i = 0; i < (int)data[2]; i++)
                *--dst = *--src;
        }
        else
        {
            for (int i = 0; i < (int)data[2]; i++)
                *dst++ = *src++;
        }
        return 0;
    }

    case 0x30:
        RxSndDrvSysSet(data);
        return 0;
    case 0x31:
        RxSndDrvMixSet((int)data[0], data + 1);
        return 0;
    case 0x32:
        RxSndDrvEffectSet((int)data[0], data + 1);
        return 0;

    /* {type, a, b, size}: one SPU transfer on queue 1; answers its id. */
    case 0x40:
        ret = (u_int)RxSndDrvDmaPush(1, (char)data[0], data[1], data[2], (int)data[3], NULL, NULL);
        RxSndDrvDmaKick();
        return ret;
    case 0x41:
        return (u_int)RxSndDrvDmaBusy((int)data[0]);

    case 0x50:
        return (u_int)RxSndDrvXvabOpen(data[0], data[1], data[2]);
    case 0x51:
        return (u_int)RxSndDrvXvabClose(data[0]);
    case 0x60:
        return (u_int)RxSndDrvXseqOpen(data[0], data[1]);
    case 0x61:
        return (u_int)RxSndDrvXseqClose(data[0]);
    case 0x63:
        RxSndDrvXseqCmd63(data[0], data[1], data[2]);
        return 0;

    case 0x70:
        return (u_int)RxSndDrvBgmOpen(data[0], data[1]);
    case 0x71:
        RxSndDrvBgmSet(data[0], data + 1);
        return 0;
    case 0x72:
        *(int *)(reply + 0x100) = RxSndDrvBgmStatus(reply, reply + 0x80);
        return 0;
    case 0x73:
        RxSndDrvBgmCmd73((char)data[0], ArgByte(data, 1), data + 1);
        return 0;
    case 0x74:
        RxSndDrvBgmCmd74((char)data[0], ArgByte(data, 1), data + 1);
        return 0;

    case 0x80:
        return (u_int)RxSndDrvSePlay((char)data[0], ArgByte(data, 1), ArgByte(data, 2), ArgByte(data, 3));
    case 0x81:
        return (u_int)RxSndDrvSePlay2((char)data[0], ArgByte(data, 1), ArgByte(data, 2), ArgByte(data, 3));
    case 0x82:
        RxSndDrvSeSet(data[0], data + 1);
        return 0;
    case 0x83:
        *(int *)(reply + 0x100) = RxSndDrvSeStatus(reply);
        return 0;

    case 0x90:
        RxSndDrvCmd90(reply);
        return 0;
    case 0x91:
        RxSndDrvCmd91(data[0], data[1]);
        return 0;
    case 0x92:
        RxSndDrvCmd92(reply);
        return 0;
    case 0x93:
        RxSndDrvCmd93(data[0], reply);
        return 0;

    case 0xa0:
        return (u_int)RxSndDrvAdpcmOpen((char)data[0], ArgHalf(data, 3), data + 4, ArgByte(data, 2),
                                        data[2], (int)data[3], ArgHalf(data, 2));
    case 0xa1:
        RxSndDrvAdpcmData((char)data[0], (int)data[1], ArgByte(data, 1));
        return 0;
    case 0xa2:
        RxSndDrvAdpcmSet(data[0], data + 1);
        return 0;
    case 0xa3:
        *(int *)(reply + 0x300) = RxSndDrvAdpcmStatus(reply);
        return 0;
    case 0xa4:
        RxSndDrvAdpcmChannelSet(ArgHalf(data, 0), ArgHalf(data, 1), data + 1);
        return 0;

    case 0xb0:
        return (u_int)RxSndDrvPcmOpen((char)data[0], ArgByte(data, 1), data[1], data[2]);
    case 0xb1:
        RxSndDrvPcmData((char)data[0], data + 1, ArgByte(data, 1));
        return 0;
    case 0xb2:
        RxSndDrvPcmSet(data[0], data + 1);
        return 0;
    case 0xb3:
        *(int *)(reply + 0x180) = RxSndDrvPcmStatus(reply);
        return 0;

    case 0x200:
        /* The queued commands; their own answers are dropped. */
        while (data[0] != 0)
        {
            RxSndDrvCommand(data[0], reply, data + 3);
            reply += data[1];
            data   = (const u_int *)((const u_char *)data + data[2] + 0xc);
        }
        return 0;
    }

    if (RxSndDrvGetDebugFlags() & RXSNDDRV_DBG_ERROR)
        printf("snddrv : invalide RPC command '%d'\n", cmd);
    return 0;
}
