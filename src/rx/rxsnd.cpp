/* ==========================================================================
 *  rx/rxsnd.cpp
 *
 *  The rx sound library: the EE client of RXSNDDRV.IRX (RPC 0xd000). See
 *  rxsnd.h for the command families. Written so far: init / term, the
 *  transport (bind, call, direct commands, the command queue), the config,
 *  the IOP / SPU heap hooks, the per-frame status update of every family,
 *  volumes, stop / close and the set commands. Left: XVAB load / free, XSEQ,
 *  BGM / SE play, PCM, ADPCM open / write / end, the SPU heap allocator.
 *
 *  PC: the IOP side is a host stand-in (src/sdk/iop_rxsnd.cpp) registered
 *  at the IOP boot; without it rxSndRpcInit would bind for ever. The RPC
 *  completes on the caller's thread, so rxSndRpcEndFunc signals the
 *  semaphore before sceSifCallRpc returns, as the interrupt did.
 * ======================================================================== */
#include "rx/rxsnd.h"
#include "common/rosepan_stub.h"

#include "rx/rxsys.h"
#include "rx/rxthread.h"
#include "sdk/eekernel.h"
#include "sdk/sifrpc.h"

#include <string.h>

/* ---- data ------------------------------------------------------------- */

static int             s_bufferBank;               /* sdata 0x2db638 */
static RxSndBgCallHook s_bgCallHook;               /* sdata 0x2db958 */
static RxSndAllocFunc  s_iopAllocHook      = rxSndDefIopAlloc;      /* sdata 0x2db95c */
static RxSndAllocFunc  s_iopAllocMode1Hook = rxSndDefIopAllocMode1; /* sdata 0x2db960 */
static RxSndFreeFunc   s_iopFreeHook       = rxSndDefIopFree;       /* sdata 0x2db964 */
static RxSndAllocFunc  s_spuAllocHook      = rxSndDefSpuAlloc;      /* sdata 0x2db968 */
static RxSndAllocFunc  s_spuAllocMode1Hook = rxSndDefSpuAllocMode1; /* sdata 0x2db96c */
static RxSndFreeFunc   s_spuFreeHook       = rxSndDefSpuFree;       /* sdata 0x2db970 */
static int             s_rpcSema = -1;             /* sdata 0x2db9c0 */
static int             s_queuePos;                 /* sdata 0x2db9c4: bytes queued */
static int             s_queueReplyPos;            /* sdata 0x2db9c8: reply bytes reserved */

static RxSndConfig s_config;                       /* bss 0x7b5ac0 */
static u_char      s_workBuffer[0x1000];           /* bss 0x7b5b00 */
static RxSndIopBuf s_iopBuf;                       /* bss 0x7b6b00 */

static RxSndSpuNode   s_spuNodes[256];             /* bss 0x7b6b80 */
static RxSndSpuNode   s_spuList;                   /* bss 0x7b8380: sentinel of the block list */
static RxSndSpuNode  *s_spuFreeNodes;              /* bss 0x7b8398 */
static u_int          s_spuHeapSize;               /* bss 0x7b839c */
static RxSndSpuSlot   s_spuSlots[256];             /* bss 0x7b83a0 */
static RxSndSpuRegion s_spuRegion;                 /* bss 0x7b97a0 */
static RxSndXvab      s_xvab[256];                 /* bss 0x7b97c0 */

static RxSndBgm  s_bgm[32];                        /* bss 0x7bbbc0 */
static RxSndBgm *s_bgmActive;                      /* bss 0x7bbec0 */
static RxSndBgm *s_bgmStopping;                    /* bss 0x7bbec4: stopped, the IOP still lists it */
static RxSndBgm *s_bgmFree;                        /* bss 0x7bbec8 */
static short     s_bgmMasterVolL;                  /* bss 0x7bbecc */
static short     s_bgmMasterVolR;                  /* bss 0x7bbece */

static RxSndSeVoice  s_se[64];                     /* bss 0x7bbed0 */
static RxSndSeVoice *s_seActive;                   /* bss 0x7bc3d0 */
static RxSndSeVoice *s_seStopping;                 /* bss 0x7bc3d4 */
static RxSndSeVoice *s_seFree;                     /* bss 0x7bc3d8 */
static short         s_seMasterVolL;               /* bss 0x7bc3dc */
static short         s_seMasterVolR;               /* bss 0x7bc3de */
static int           s_se_3e0;                     /* bss 0x7bc3e0: set -1 (unverified) */
static int           s_se_3e4;                     /* bss 0x7bc3e4: set -1 (unverified) */

static RxSndPosCallback  s_pcmCallbacks[256];      /* bss 0x7bc400 */
static RxSndPosCallback *s_pcmCallbackFree;        /* bss 0x7bd800 */
static RxSndPcm          s_pcm[32];                /* bss 0x7bd804 */
static RxSndPcm         *s_pcmActive;              /* bss 0x7bdf84 */
static RxSndPcm         *s_pcmStopping;            /* bss 0x7bdf88 */
static RxSndPcm         *s_pcmFree;                /* bss 0x7bdf8c */
static short             s_pcmMasterVolL;          /* bss 0x7bdf90 */
static short             s_pcmMasterVolR;          /* bss 0x7bdf92 */

static RxSndPosCallback  s_adpcmCallbacks[256];    /* bss 0x7befc0 */
static RxSndPosCallback *s_adpcmCallbackFree;      /* bss 0x7c03c0 */
static RxSndAdpcm        s_adpcm[48];              /* bss 0x7c03c8 */
static RxSndAdpcm       *s_adpcmActive;            /* bss 0x7c12c8 */
static RxSndAdpcm       *s_adpcmStopping;          /* bss 0x7c12cc */
static RxSndAdpcm       *s_adpcmFree;              /* bss 0x7c12d0 */
static short             s_adpcmMasterVolL;        /* bss 0x7c12d4 */
static short             s_adpcmMasterVolR;        /* bss 0x7c12d6 */
static int               s_adpcm_2d8;              /* bss 0x7c12d8: set 0 (unverified) */
static int               s_adpcm_2dc;              /* bss 0x7c12dc: set 0 (unverified) */

/* The direct commands' buffers, one bank per rxTh yield state. */
static u_int            s_rpcSend[2][0x100];       /* bss 0x7c9300 */
static u_int            s_rpcReply[2][0x100];      /* bss 0x7c9b00 */
static sceSifClientData s_rpcClient;               /* bss 0x7ca300 */
/* PC: 0x40 more than the ROM's 0x1000: the end marker after a full queue
 * and the flush's rounded size run into the reply area that follows on
 * the EE. */
static u_char           s_queue[0x1040];           /* bss 0x7ca340 */
static u_char           s_queueReply[0x1000];      /* bss 0x7cb340 */

static inline int rxSndClamp(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/* ---- init / term ------------------------------------------------------ */

// ROR 0x00237ce8
void rxSndThreadCallback(int yielding)
{
    rxSndSetBufferBank(yielding);
}

// ROR 0x00237d00
void rxSndCheckBgCall(const char *func)
{
    if (rxThIsYielding() == 1 && rxThIsRequestLocked() == 0 && rxThIsDispatchDisabled() == 0
        && (rxSndGetDebugFlags() & 2))
        rxSysPrintf("*** rxsndut : BG thread called! %s\n", func);
}

// ROR 0x00237d80
void rxSndInit(void)
{
    rxSndInitModules();
    rxThAddCallback(rxSndThreadCallback);
    rxSndSetBgCallHook(rxSndCheckBgCall);
}

// ROR 0x00237db0
void rxSndTerm(void)
{
    rxThRemoveCallback(rxSndThreadCallback);
    rxSndTermModules();
}

// ROR 0x00237dd0
void rxSndInitModules(void)
{
    rxSndRpcInit();
    rxSndInitConfig();
    rxSndIopBufInit();
    rxSndSpuHeapInit();
    rxSndXvabInit();
    rxSndXseqInit();
    rxSndBgmInit();
    rxSndSeInit();
    rxSndPcmInit();
    rxSndAdpcmInit();
}

// ROR 0x00237e30
void rxSndTermModules(void)
{
    /* Command 0x11 until the IOP reports it has stopped everything. */
    while (rxSndRpcCommand(0x11, NULL, 0, NULL, 0) != 0)
        ;
    rxSndAdpcmTerm();
    rxSndPcmTerm();
    rxSndSeTerm();
    rxSndBgmTerm();
    rxSndXseqTerm();
    rxSndXvabTerm();
    rxSndSpuHeapTerm();
    rxSndIopBufTerm();
    rxSndTermConfig();
    rxSndRpcTerm();
}

// ROR 0x00237eb0
void rxSndUpdate(void)
{
    rxSndQueueFlush();

    rxSndQueueBegin(0x72, sizeof(RxSndBgmStatus), 0);
    const RxSndBgmStatus *bgm = (const RxSndBgmStatus *)rxSndQueueGetReply();
    rxSndQueueEnd();
    rxSndQueueBegin(0x83, sizeof(RxSndSeStatus), 0);
    const RxSndSeStatus *se = (const RxSndSeStatus *)rxSndQueueGetReply();
    rxSndQueueEnd();
    rxSndQueueBegin(0xb3, sizeof(RxSndPcmStatus), 0);
    const RxSndPcmStatus *pcm = (const RxSndPcmStatus *)rxSndQueueGetReply();
    rxSndQueueEnd();
    rxSndQueueBegin(0xa3, sizeof(RxSndAdpcmStatus), 0);
    const RxSndAdpcmStatus *adpcm = (const RxSndAdpcmStatus *)rxSndQueueGetReply();
    rxSndQueueEnd();

    rxSndQueueFlush();
    rxSndBgmUpdate(bgm);
    rxSndSeUpdate(se);
    rxSndPcmUpdate(pcm);
    rxSndAdpcmUpdate(adpcm);
}

// ROR 0x00238038
u_char rxSndGetDebugFlags(void)
{
    return s_config.m_debugFlags;
}

// ROR 0x00238188
void rxSndSetMasterVolume(int volL, int volR)
{
    volL = rxSndClamp(volL, -0x80, 0x7f);
    volR = rxSndClamp(volR, -0x80, 0x7f);
    s_config.m_masterVolR = (short)volR;
    s_config.m_masterVolL = (short)volL;
    rxSndBgmSetMasterVolume(rxSndBgmGetMasterVolumeL(), rxSndBgmGetMasterVolumeR());
    rxSndSeSetMasterVolume(rxSndSeGetMasterVolumeL(), rxSndSeGetMasterVolumeR());
    rxSndPcmSetMasterVolume(rxSndPcmGetMasterVolumeL(), rxSndPcmGetMasterVolumeR());
    rxSndAdpcmSetMasterVolume(rxSndAdpcmGetMasterVolumeL(), rxSndAdpcmGetMasterVolumeR());
}

// ROR 0x00238268
int rxSndSetBufferBank(int bank)
{
    int old = s_bufferBank;

    if (bank < 0)
        s_bufferBank = 0;
    else if (bank < 2)
        s_bufferBank = bank;
    else
        s_bufferBank = 1;
    return old;
}

// ROR 0x00238478
void rxSndInitConfig(void)
{
    s_config.m_masterVolR = 0x80;
    s_config.m_debugFlags = 7;
    s_config.m_1          = 1;
    s_config.m_2          = 0x3c;
    s_config.m_masterVolL = 0x80;
}

// ROR 0x002384a8
void rxSndTermConfig(void)
{
}

// ROR 0x002384b8
int rxSndGetBufferBank(void)
{
    return s_bufferBank;
}

// ROR 0x002384c8
short rxSndGetMasterVolumeL(void)
{
    return s_config.m_masterVolL;
}

// ROR 0x002384d8
short rxSndGetMasterVolumeR(void)
{
    return s_config.m_masterVolR;
}

// ROR 0x002384e8
void *rxSndGetWorkBuffer(void)
{
    return s_workBuffer;
}

// ROR 0x002384f8
void rxSndWait(void)
{
}

// ROR 0x00238500
void rxSndSetBgCallHook(RxSndBgCallHook hook)
{
    s_bgCallHook = hook;
}

// ROR 0x00238510
void rxSndCallBgCallHook(const char *func)
{
    if (s_bgCallHook != NULL)
        s_bgCallHook(func);
}

/* ---- IOP / SPU memory ------------------------------------------------- */

/* Command 0x20 {mode, size, 0}: the IOP address, 0 when the heap is full. */
// ROR 0x002385b8
u_int rxSndDefIopAlloc(int size)
{
    u_int req[3];
    u_int bytes = (size + 0x3f) & ~0x3f;

    req[0] = 0;
    req[1] = bytes;
    req[2] = 0;
    u_int addr = rxSndRpcCommand(0x20, NULL, 0, req, sizeof(req));
    if (addr == 0)
    {
        if (rxSndGetDebugFlags() & 1)
            rxSysPrintf("rxsnd : can't allocate IOP heap '%d'\n", bytes);
        addr = 0;
    }
    return addr;
}

// ROR 0x00238630
u_int rxSndDefIopAllocMode1(int size)
{
    u_int req[3];
    u_int bytes = (size + 0x3f) & ~0x3f;

    req[0] = 1;
    req[1] = bytes;
    req[2] = 0;
    u_int addr = rxSndRpcCommand(0x20, NULL, 0, req, sizeof(req));
    if (addr == 0)
    {
        if (rxSndGetDebugFlags() & 1)
            rxSysPrintf("rxsnd : can't allocate IOP heap '%d'\n", bytes);
        addr = 0;
    }
    return addr;
}

// ROR 0x002386b0
void rxSndDefIopFree(u_int addr)
{
    u_int req[1];

    req[0] = addr;
    rxSndRpcCommand(0x21, NULL, 0, req, sizeof(req));
}

// ROR 0x00238740
u_int rxSndIopAlloc(int size)
{
    return s_iopAllocHook(size);
}

// ROR 0x00238768
u_int rxSndIopAllocMode1(int size)
{
    return s_iopAllocMode1Hook(size);
}

// ROR 0x00238790
void rxSndIopFree(u_int addr)
{
    s_iopFreeHook(addr);
}

// ROR 0x00238d28
void rxSndIopBufInit(void)
{
    memset(&s_iopBuf, 0, sizeof(s_iopBuf));
    s_iopBuf.m_buf0 = rxSndIopAlloc(0x20);
    s_iopBuf.m_buf1 = rxSndIopAlloc(0x20);
}

// ROR 0x00238d78
void rxSndIopBufTerm(void)
{
    if (s_iopBuf.m_buf0 != 0)
        rxSndIopFree(s_iopBuf.m_buf0);
    if (s_iopBuf.m_buf1 != 0)
        rxSndIopFree(s_iopBuf.m_buf1);
}

// ROR 0x00238ff0
u_int rxSndDefSpuAlloc(int size)
{
    return rxSndSpuHeapAlloc(0, 0, size);
}

// ROR 0x00239010
u_int rxSndDefSpuAllocMode1(int size)
{
    return rxSndSpuHeapAlloc(1, 0, size);
}

// ROR 0x00239030
void rxSndDefSpuFree(u_int addr)
{
    rxSndSpuHeapFree(addr);
}

// ROR 0x00239048
void rxSndSetSpuAllocHook(RxSndAllocFunc func)
{
    s_spuAllocHook = func != NULL ? func : rxSndDefSpuAlloc;
}

// ROR 0x00239068
void rxSndSetSpuAllocMode1Hook(RxSndAllocFunc func)
{
    s_spuAllocMode1Hook = func != NULL ? func : rxSndDefSpuAllocMode1;
}

// ROR 0x00239088
void rxSndSetSpuFreeHook(RxSndFreeFunc func)
{
    s_spuFreeHook = func != NULL ? func : rxSndDefSpuFree;
}

// ROR 0x002390a8
u_int rxSndSpuAlloc(int size)
{
    return s_spuAllocHook(size);
}

// ROR 0x002390d0
u_int rxSndSpuAllocMode1(int size)
{
    return s_spuAllocMode1Hook(size);
}

// ROR 0x002390f8
void rxSndSpuFree(u_int addr)
{
    s_spuFreeHook(addr);
}

// ROR 0x00239288
int rxSndXvabPoll(int handle, int wait)
{
    for (;;)
    {
        u_int req[1];

        req[0] = handle;
        int busy = (int)rxSndRpcCommand(0x41, NULL, 0, req, sizeof(req));
        if (busy == 0)
            return 0;
        if (!wait)
            return busy;
        for (int i = 9999; i >= 0; i--)
            rxSndWait();
    }
}

// ROR 0x00239380
void rxSndSpuHeapInit(void)
{
    s_spuHeapSize  = 0x1baff0;
    s_spuFreeNodes = NULL;
    for (int i = 0; i < 256; i++)
    {
        s_spuNodes[i].m_prev = s_spuFreeNodes;  /* the free list links through +0x4 */
        s_spuFreeNodes       = &s_spuNodes[i];
    }

    /* One free block covering SPU 0x5010..0x1c0000 on the circular list. */
    RxSndSpuNode *node = rxSndSpuHeapNewNode();
    node->m_prev    = &s_spuList;
    node->m_next    = &s_spuList;
    s_spuList.m_prev = node;
    node->m_prev->m_next = node;
    node->m_size    = 0x1baff0;
    node->m_addr    = 0x5010;
    node->m_8       = 0;
    node->m_c       = 0;

    for (int i = 0; i < 256; i++)
    {
        s_spuSlots[i].m_index = i;
        s_spuSlots[i].m_used  = 0;
    }

    s_spuRegion.m_addr = 0x5010;
    s_spuRegion.m_10   = 0;
    s_spuRegion.m_0    = -1;
    s_spuRegion.m_4    = 1;
    s_spuRegion.m_size = 0x1baff0;
    for (int i = 1; i >= 0; i--)
        s_spuRegion.m_14[i] = -1;
}

// ROR 0x002394a0
void rxSndSpuHeapTerm(void)
{
}

// ROR 0x00239be8
RxSndSpuNode *rxSndSpuHeapNewNode(void)
{
    RxSndSpuNode *node = s_spuFreeNodes;

    if (node != NULL)
    {
        s_spuFreeNodes = node->m_prev;
        return node;
    }
    return NULL;
}

/* ---- XVAB / XSEQ ------------------------------------------------------ */

// ROR 0x0023a170
void rxSndXvabInit(void)
{
}

// ROR 0x0023a178
void rxSndXvabTerm(void)
{
}

// ROR 0x0023a720
int rxSndXvabCheck(int id, int wait)
{
    if ((u_int)id >= 0x100)
    {
        if (rxSndGetDebugFlags() & 1)
            rxSysPrintf("rxsnd : out of range XVAB ID '%d'\n", id);
        return 0;
    }

    RxSndXvab *xvab = &s_xvab[id];
    if (!(xvab->m_flags & 1))
    {
        if (rxSndGetDebugFlags() & 1)
            rxSysPrintf("rxsnd : specified XVAB ID '%d' is unused\n", id);
        return 0;
    }
    if (xvab->m_transfer < 0)
        return 0;

    int busy = rxSndXvabPoll(xvab->m_transfer, wait);
    if (busy == 0)
        xvab->m_transfer = -1;
    return busy;
}

// ROR 0x0023b018
void rxSndXseqInit(void)
{
}

// ROR 0x0023b020
void rxSndXseqTerm(void)
{
}

// ROR 0x0023b028
u_int rxSndReadU32(const u_int *p)
{
    return *p;
}

/* ---- BGM -------------------------------------------------------------- */

// ROR 0x0023b238
void rxSndBgmSetMasterVolume(int volL, int volR)
{
    s_bgmMasterVolL = (short)rxSndClamp(volL, -0x80, 0x80);
    s_bgmMasterVolR = (short)rxSndClamp(volR, -0x80, 0x80);
    for (RxSndBgm *bgm = s_bgmActive; bgm != NULL; bgm = bgm->m_next)
        rxSndBgmSetVolume(bgm->m_id, bgm->m_volL, bgm->m_volR);
}

// ROR 0x0023b3b8
void rxSndBgmStop(int id, int arg)
{
    rxSndCallBgCallHook("rxSndBgmStop");

    RxSndBgm **link = &s_bgmActive;
    RxSndBgm  *bgm;
    for (;;)
    {
        bgm = *link;
        if (bgm == NULL)
            return;
        if (bgm->m_id == id)
            break;
        link = &bgm->m_next;
    }

    rxSndQueueBegin(0x71, 0, sizeof(RxSndCmdSet));
    RxSndCmdSet *cmd = (RxSndCmdSet *)rxSndQueueGetData();
    cmd->m_param = 0;
    cmd->m_value = arg != 0 ? 1 : 2;
    cmd->m_id    = bgm->m_id;
    rxSndQueueEnd();

    bgm->m_flags  = 0;
    *link         = bgm->m_next;
    bgm->m_next   = s_bgmStopping;
    s_bgmStopping = bgm;
}

// ROR 0x0023b840
void rxSndBgmSetVolume(int id, int volL, int volR)
{
    if ((u_int)id >= 0x20 || !(s_bgm[id].m_flags & 1))
        return;

    RxSndBgm *bgm = &s_bgm[id];
    volL = rxSndClamp(volL, -0x80, 0x80);
    volR = rxSndClamp(volR, -0x80, 0x80);
    bgm->m_volL = (short)volL;
    bgm->m_volR = (short)volR;
    int masterL = rxSndGetMasterVolumeL();
    int famL    = s_bgmMasterVolL;
    int masterR = rxSndGetMasterVolumeR();
    int famR    = s_bgmMasterVolR;

    rxSndQueueBegin(0x71, 0, sizeof(RxSndCmdSet));
    RxSndCmdSet *cmd = (RxSndCmdSet *)rxSndQueueGetData();
    cmd->m_vol[0] = (short)(masterL * famL * volL >> 14);
    cmd->m_vol[1] = (short)(masterR * famR * volR >> 14);
    cmd->m_id     = bgm->m_id;
    cmd->m_param  = 3;
    rxSndQueueEnd();
}

// ROR 0x0023bb78
void rxSndBgmInit(void)
{
    s_bgmMasterVolR = 0x80;
    s_bgmActive     = NULL;
    s_bgmStopping   = NULL;
    s_bgmFree       = NULL;
    s_bgmMasterVolL = 0x80;
    for (int i = 0; i < 0x20; i++)
    {
        s_bgm[i].m_id   = i;
        s_bgm[i].m_next = s_bgmFree;
        s_bgmFree       = &s_bgm[i];
    }
}

// ROR 0x0023bbd0
void rxSndBgmTerm(void)
{
}

// ROR 0x0023bbd8
void rxSndBgmUpdate(const RxSndBgmStatus *status)
{
    /* Playing slots take the IOP's status; an ended one stops itself
     * unless m_keep is set. */
    RxSndBgm *next;
    for (RxSndBgm *bgm = s_bgmActive; bgm != NULL; bgm = next)
    {
        next = bgm->m_next;
        for (int i = 0; i < status->m_count; i++)
        {
            const RxSndStatusEntry *e = &status->m_entry[i];
            if (e->m_id == (u_int)bgm->m_id)
            {
                u_char st = e->m_status;
                bgm->m_flags = (bgm->m_flags & ~0xeu) | (st & 2) | (st & 8) | (st & 4);
                bgm->m_value = status->m_value[i];
                if ((st & 4) && bgm->m_keep == 0)
                    rxSndBgmStop(e->m_id, 0);
                break;
            }
        }
    }

    /* Stopped slots go back to the free list once the IOP drops them. */
    RxSndBgm **link = &s_bgmStopping;
    for (RxSndBgm *bgm = *link; bgm != NULL; bgm = *link)
    {
        bool listed = false;
        for (int i = 0; i < status->m_count; i++)
        {
            if (status->m_entry[i].m_id == (u_int)bgm->m_id)
            {
                listed = true;
                break;
            }
        }
        if (listed)
        {
            link = &bgm->m_next;
            continue;
        }
        *link       = bgm->m_next;
        bgm->m_next = s_bgmFree;
        s_bgmFree   = bgm;
    }
}

// ROR 0x0023bd50
short rxSndBgmGetMasterVolumeL(void)
{
    return s_bgmMasterVolL;
}

// ROR 0x0023bd60
short rxSndBgmGetMasterVolumeR(void)
{
    return s_bgmMasterVolR;
}

/* ---- SE --------------------------------------------------------------- */

// ROR 0x0023bd70
void rxSndSeSetMasterVolume(int volL, int volR)
{
    s_seMasterVolL = (short)rxSndClamp(volL, -0x80, 0x80);
    s_seMasterVolR = (short)rxSndClamp(volR, -0x80, 0x80);
    for (RxSndSeVoice *voice = s_seActive; voice != NULL; voice = voice->m_next)
        rxSndSeSetVolume(voice->m_id, voice->m_volL, voice->m_volR);
}

// ROR 0x0023c0f8
void rxSndSeStop(int id, int arg)
{
    rxSndCallBgCallHook("rxSndSeStop");

    RxSndSeVoice **link = &s_seActive;
    RxSndSeVoice  *voice;
    for (;;)
    {
        voice = *link;
        if (voice == NULL)
            return;
        if (voice->m_id == id)
            break;
        link = &voice->m_next;
    }

    rxSndQueueBegin(0x82, 0, sizeof(RxSndCmdSet));
    RxSndCmdSet *cmd = (RxSndCmdSet *)rxSndQueueGetData();
    cmd->m_param = 0;
    cmd->m_value = arg != 0 ? 1 : 2;
    cmd->m_id    = voice->m_id;
    rxSndQueueEnd();

    voice->m_flags = 0;
    *link          = voice->m_next;
    voice->m_next  = s_seStopping;
    s_seStopping   = voice;
}

// ROR 0x0023c228
u_int rxSndSeGetStatus(int id)
{
    if ((u_int)id > 0x3f || !(s_se[id].m_flags & 1))
        return 0;
    return s_se[id].m_flags;
}

// ROR 0x0023c328
void rxSndSeSetAttr(int id, int value)
{
    if ((u_int)id >= 0x40 || !(s_se[id].m_flags & 1))
        return;

    rxSndQueueBegin(0x82, 0, sizeof(RxSndCmdSet));
    RxSndCmdSet *cmd = (RxSndCmdSet *)rxSndQueueGetData();
    cmd->m_value = value;
    cmd->m_id    = s_se[id].m_id;
    cmd->m_param = 2;
    rxSndQueueEnd();
}

// ROR 0x0023c3c0
void rxSndSeSetVolume(int id, int volL, int volR)
{
    if ((u_int)id >= 0x40 || !(s_se[id].m_flags & 1))
        return;

    RxSndSeVoice *voice = &s_se[id];
    volL = rxSndClamp(volL, -0x80, 0x80);
    volR = rxSndClamp(volR, -0x80, 0x80);
    voice->m_volL = (short)volL;
    voice->m_volR = (short)volR;
    int masterL = rxSndGetMasterVolumeL();
    int famL    = s_seMasterVolL;
    int masterR = rxSndGetMasterVolumeR();
    int famR    = s_seMasterVolR;

    rxSndQueueBegin(0x82, 0, sizeof(RxSndCmdSet));
    RxSndCmdSet *cmd = (RxSndCmdSet *)rxSndQueueGetData();
    cmd->m_vol[0] = (short)(masterL * famL * volL >> 14);
    cmd->m_vol[1] = (short)(masterR * famR * volR >> 14);
    cmd->m_id     = voice->m_id;
    cmd->m_param  = 3;
    rxSndQueueEnd();
}

// ROR 0x0023c510
void rxSndSeSetPitch(int id, float pitch)
{
    if ((u_int)id >= 0x40 || !(s_se[id].m_flags & 1))
        return;

    rxSndQueueBegin(0x82, 0, sizeof(RxSndCmdSet));
    RxSndCmdSet *cmd = (RxSndCmdSet *)rxSndQueueGetData();
    cmd->m_value = (int)(pitch * 4096.0f);
    cmd->m_id    = s_se[id].m_id;
    cmd->m_param = 4;
    rxSndQueueEnd();
}

// ROR 0x0023c910
void rxSndSeInit(void)
{
    s_seMasterVolR = 0x80;
    s_se_3e4       = -1;
    s_seActive     = NULL;
    s_seStopping   = NULL;
    s_seFree       = NULL;
    s_seMasterVolL = 0x80;
    s_se_3e0       = -1;
    for (int i = 0; i < 0x40; i++)
    {
        s_se[i].m_id   = i;
        s_se[i].m_next = s_seFree;
        s_seFree       = &s_se[i];
    }
}

// ROR 0x0023c970
void rxSndSeTerm(void)
{
}

// ROR 0x0023c978
void rxSndSeUpdate(const RxSndSeStatus *status)
{
    /* A voice the IOP no longer plays is marked ended (4) and stops itself
     * unless m_keep is set. */
    RxSndSeVoice *next;
    for (RxSndSeVoice *voice = s_seActive; voice != NULL; voice = next)
    {
        next = voice->m_next;
        for (int i = 0; i < status->m_count; i++)
        {
            const RxSndStatusEntry *e = &status->m_entry[i];
            if (e->m_id == (u_int)voice->m_id)
            {
                u_char st = e->m_status;
                voice->m_flags = (voice->m_flags & ~0xau) | (st & 2) | (st & 8);
                if (!(st & 2))
                    voice->m_flags |= 4;
                if (!(st & 2) && voice->m_keep == 0)
                    rxSndSeStop(voice->m_id, 0);
                break;
            }
        }
    }

    RxSndSeVoice **link = &s_seStopping;
    for (RxSndSeVoice *voice = *link; voice != NULL; voice = *link)
    {
        bool listed = false;
        for (int i = 0; i < status->m_count; i++)
        {
            if (status->m_entry[i].m_id == (u_int)voice->m_id)
            {
                listed = true;
                break;
            }
        }
        if (listed)
        {
            link = &voice->m_next;
            continue;
        }
        *link         = voice->m_next;
        voice->m_next = s_seFree;
        s_seFree      = voice;
    }
}

// ROR 0x0023cae8
short rxSndSeGetMasterVolumeL(void)
{
    return s_seMasterVolL;
}

// ROR 0x0023caf8
short rxSndSeGetMasterVolumeR(void)
{
    return s_seMasterVolR;
}

/* ---- PCM -------------------------------------------------------------- */

// ROR 0x0023cb08
void rxSndPcmSetMasterVolume(int volL, int volR)
{
    s_pcmMasterVolL = (short)rxSndClamp(volL, -0x80, 0x80);
    s_pcmMasterVolR = (short)rxSndClamp(volR, -0x80, 0x80);
    for (RxSndPcm *pcm = s_pcmActive; pcm != NULL; pcm = pcm->m_next)
        rxSndPcmSetVolume(pcm->m_id, pcm->m_volL, pcm->m_volR);
}

// ROR 0x0023d7e0
void rxSndPcmSetVolume(int id, int volL, int volR)
{
    if ((u_int)id > 0x1f)
        return;
    RxSndPcm *pcm = &s_pcm[id];
    if (!(pcm->m_status & 1))
        return;

    pcm->m_volL = (short)rxSndClamp(volL, -0x80, 0x80);
    pcm->m_volR = (short)rxSndClamp(volR, -0x80, 0x80);
    int masterL = rxSndGetMasterVolumeL();
    int famL    = s_pcmMasterVolL;
    int masterR = rxSndGetMasterVolumeR();
    int famR    = s_pcmMasterVolR;

    rxSndQueueBegin(0xb2, 0, sizeof(RxSndCmdSet));
    RxSndCmdSet *cmd = (RxSndCmdSet *)rxSndQueueGetData();
    cmd->m_vol[0] = (short)(masterL * famL * pcm->m_volL >> 14);
    cmd->m_vol[1] = (short)(masterR * famR * pcm->m_volR >> 14);
    cmd->m_id     = pcm->m_id;
    cmd->m_param  = 4;
    rxSndQueueEnd();
}

// ROR 0x0023d908
void rxSndPcmInit(void)
{
    s_pcmCallbackFree = NULL;
    for (int i = 0; i < 256; i++)
    {
        s_pcmCallbacks[i].m_next = s_pcmCallbackFree;
        s_pcmCallbackFree        = &s_pcmCallbacks[i];
    }

    s_pcmMasterVolR = 0x80;
    s_pcmActive     = NULL;
    s_pcmStopping   = NULL;
    s_pcmFree       = NULL;
    s_pcmMasterVolL = 0x80;
    for (int i = 0; i < 0x20; i++)
    {
        s_pcm[i].m_id     = i;
        s_pcm[i].m_status = 0;
        s_pcm[i].m_next   = s_pcmFree;
        s_pcmFree         = &s_pcm[i];
    }
}

// ROR 0x0023d998
void rxSndPcmTerm(void)
{
}

// ROR 0x0023d9a0
void rxSndPcmUpdate(const RxSndPcmStatus *status)
{
    /* Playing streams take the IOP's status and fire the position callbacks
     * they have passed. */
    for (RxSndPcm *pcm = s_pcmActive; pcm != NULL; pcm = pcm->m_next)
    {
        for (int i = 0; i < status->m_count; i++)
        {
            const RxSndPcmStatusEntry *e = &status->m_entry[i];
            if (e->m_id != (u_int)pcm->m_id)
                continue;

            u_char st     = e->m_status;
            pcm->m_status = (u_char)((pcm->m_status & 0xf1) | (st & 2) | (st & 8) | (st & 4));
            pcm->m_20     = e->m_2;
            if (e->m_status & 2)
            {
                RxSndPosCallback **link = &pcm->m_callbacks;
                for (RxSndPosCallback *cb = *link; cb != NULL; cb = *link)
                {
                    if ((int)cb->m_posHi < e->m_posHi
                        || ((int)cb->m_posHi == e->m_posHi && (int)cb->m_posLo <= e->m_posLo))
                    {
                        if (cb->m_func != NULL)
                            cb->m_func(pcm->m_id, cb->m_arg);
                        *link             = cb->m_next;
                        cb->m_next        = s_pcmCallbackFree;
                        s_pcmCallbackFree = cb;
                    }
                    else
                    {
                        link = &cb->m_next;
                    }
                }
            }
            break;
        }
    }

    /* Stopped streams give back their IOP buffer once the IOP drops them. */
    RxSndPcm **link = &s_pcmStopping;
    for (RxSndPcm *pcm = *link; pcm != NULL; pcm = *link)
    {
        bool listed = false;
        for (int i = 0; i < status->m_count; i++)
        {
            if (status->m_entry[i].m_id == (u_int)pcm->m_id)
            {
                listed = true;
                break;
            }
        }
        if (listed)
        {
            link = &pcm->m_next;
            continue;
        }
        if ((pcm->m_alloc & 1) && pcm->m_iopBuf != 0)
            rxSndIopFree(pcm->m_iopBuf);
        *link       = pcm->m_next;
        pcm->m_next = s_pcmFree;
        s_pcmFree   = pcm;
    }
}

// ROR 0x0023dbb0
short rxSndPcmGetMasterVolumeL(void)
{
    return s_pcmMasterVolL;
}

// ROR 0x0023dbc0
short rxSndPcmGetMasterVolumeR(void)
{
    return s_pcmMasterVolR;
}

/* ---- ADPCM ------------------------------------------------------------ */

// ROR 0x0023dbd0
void rxSndAdpcmSetMasterVolume(int volL, int volR)
{
    s_adpcmMasterVolL = (short)rxSndClamp(volL, -0x80, 0x80);
    s_adpcmMasterVolR = (short)rxSndClamp(volR, -0x80, 0x80);
    for (RxSndAdpcm *s = s_adpcmActive; s != NULL; s = s->m_next)
        rxSndAdpcmSetVolume(s->m_id, s->m_volL, s->m_volR);
}

// ROR 0x0023e020
void rxSndAdpcmClose(int id)
{
    rxSndCallBgCallHook("rxSndAdpcmClose");

    RxSndAdpcm **link = &s_adpcmActive;
    RxSndAdpcm  *s;
    for (;;)
    {
        s = *link;
        if (s == NULL)
            return;
        if (s->m_id == (u_int)id)
            break;
        link = &s->m_next;
    }

    rxSndQueueBegin(0xa2, 0, sizeof(RxSndCmdSet));
    RxSndCmdSet *cmd = (RxSndCmdSet *)rxSndQueueGetData();
    cmd->m_value = 0;
    cmd->m_id    = s->m_id;
    cmd->m_param = 0;
    rxSndQueueEnd();

    while (s->m_callbacks != NULL)
    {
        RxSndPosCallback *cb = s->m_callbacks;
        s->m_callbacks       = cb->m_next;
        cb->m_next           = s_adpcmCallbackFree;
        s_adpcmCallbackFree  = cb;
    }
    s->m_status     = 0;
    *link           = s->m_next;
    s->m_next       = s_adpcmStopping;
    s_adpcmStopping = s;
}

// ROR 0x0023e158
u_int rxSndAdpcmGetStatus(int id)
{
    if ((u_int)id < 0x30 && (s_adpcm[id].m_status & 1))
        return s_adpcm[id].m_status;
    return 0;
}

// ROR 0x0023e1a0
int rxSndAdpcmGetFreeSize(int id, int raw)
{
    if ((u_int)id >= 0x30 || !(s_adpcm[id].m_status & 1))
        return 0;

    RxSndAdpcm *s    = &s_adpcm[id];
    int         free = s->m_bufSize - s->m_bufFill;
    if (s->m_mode < 3)
    {
        if (s->m_mode > 0)
            free *= 2;
    }
    else if (s->m_mode == 0x1000000)
    {
        free *= s->m_channels;
    }
    if (raw == 0)
        free -= s->m_channels * 0x80;
    return free;
}

// ROR 0x0023e240
int rxSndAdpcmGetFree(int id)
{
    return rxSndAdpcmGetFreeSize(id, 0);
}

// ROR 0x0023eef8
void rxSndAdpcmPause(int id, int pause)
{
    if ((u_int)id >= 0x30 || !(s_adpcm[id].m_status & 1))
        return;

    rxSndQueueBegin(0xa2, 0, sizeof(RxSndCmdSet));
    RxSndCmdSet *cmd = (RxSndCmdSet *)rxSndQueueGetData();
    cmd->m_value = pause;
    cmd->m_id    = s_adpcm[id].m_id;
    cmd->m_param = 1;
    rxSndQueueEnd();
}

// ROR 0x0023ef98
void rxSndAdpcmSetAttr(int id, int value)
{
    if ((u_int)id >= 0x30 || !(s_adpcm[id].m_status & 1))
        return;

    rxSndQueueBegin(0xa2, 0, sizeof(RxSndCmdSet));
    RxSndCmdSet *cmd = (RxSndCmdSet *)rxSndQueueGetData();
    cmd->m_value = value;
    cmd->m_id    = s_adpcm[id].m_id;
    cmd->m_param = 2;
    rxSndQueueEnd();
}

// ROR 0x0023f038
void rxSndAdpcmSetVolume(int id, int volL, int volR)
{
    if ((u_int)id >= 0x30 || !(s_adpcm[id].m_status & 1))
        return;

    RxSndAdpcm *s = &s_adpcm[id];
    s->m_volL = (short)rxSndClamp(volL, -0x80, 0x80);
    s->m_volR = (short)rxSndClamp(volR, -0x80, 0x80);
    int   masterL = rxSndGetMasterVolumeL();
    int   famL    = s_adpcmMasterVolL;
    int   masterR = rxSndGetMasterVolumeR();
    short outR    = (short)(masterR * s_adpcmMasterVolR * s->m_volR >> 14);
    short outL    = (short)(masterL * famL * s->m_volL >> 14);

    if (s->m_mode < 4 && s->m_mode > 0)
    {
        /* Modes 1..3 take the volume per channel: left on channel 0, right
         * on channel 1. */
        rxSndQueueBegin(0xa4, 0, sizeof(RxSndCmdChannelSet));
        RxSndCmdChannelSet *ch = (RxSndCmdChannelSet *)rxSndQueueGetData();
        ch->m_vol[0]  = outL;
        ch->m_id      = s->m_id;
        ch->m_vol[1]  = 0;
        ch->m_channel = 0;
        ch->m_param   = 3;
        ch->m_6       = 0;
        rxSndQueueEnd();

        rxSndQueueBegin(0xa4, 0, sizeof(RxSndCmdChannelSet));
        ch = (RxSndCmdChannelSet *)rxSndQueueGetData();
        ch->m_vol[1]  = outR;
        ch->m_id      = s->m_id;
        ch->m_channel = 1;
        ch->m_param   = 3;
        ch->m_6       = 0;
        ch->m_vol[0]  = 0;
        rxSndQueueEnd();
    }
    else
    {
        rxSndQueueBegin(0xa2, 0, sizeof(RxSndCmdSet));
        RxSndCmdSet *cmd = (RxSndCmdSet *)rxSndQueueGetData();
        cmd->m_vol[1] = outR;
        cmd->m_id     = s->m_id;
        cmd->m_param  = 3;
        cmd->m_vol[0] = outL;
        rxSndQueueEnd();
    }
}

// ROR 0x0023f310
void rxSndAdpcmSetPitch(int id, float pitch)
{
    if ((u_int)id >= 0x30 || !(s_adpcm[id].m_status & 1))
        return;

    rxSndQueueBegin(0xa2, 0, sizeof(RxSndCmdSet));
    RxSndCmdSet *cmd = (RxSndCmdSet *)rxSndQueueGetData();
    cmd->m_param = 4;
    cmd->m_value = (int)(pitch * 4096.0f);
    cmd->m_id    = s_adpcm[id].m_id;
    rxSndQueueEnd();
}

// ROR 0x0023f648
void rxSndAdpcmInit(void)
{
    s_adpcmCallbackFree = NULL;
    for (int i = 0; i < 256; i++)
    {
        s_adpcmCallbacks[i].m_next = s_adpcmCallbackFree;
        s_adpcmCallbackFree        = &s_adpcmCallbacks[i];
    }

    s_adpcmMasterVolR = 0x80;
    s_adpcmActive     = NULL;
    s_adpcmStopping   = NULL;
    s_adpcmFree       = NULL;
    s_adpcmMasterVolL = 0x80;
    for (int i = 0; i < 0x30; i++)
    {
        s_adpcm[i].m_id     = (u_char)i;
        s_adpcm[i].m_status = 0;
        s_adpcm[i].m_next   = s_adpcmFree;
        s_adpcmFree         = &s_adpcm[i];
    }
    s_adpcm_2dc = 0;
    s_adpcm_2d8 = 0;
}

// ROR 0x0023f6e0
void rxSndAdpcmTerm(void)
{
}

// ROR 0x0023f6e8
void rxSndAdpcmUpdate(const RxSndAdpcmStatus *status)
{
    for (RxSndAdpcm *s = s_adpcmActive; s != NULL; s = s->m_next)
    {
        for (int i = 0; i < status->m_count; i++)
        {
            const RxSndAdpcmStatusEntry *e = &status->m_entry[i];
            if (e->m_id != s->m_id)
                continue;

            u_char st   = e->m_status;
            s->m_status = (u_char)((s->m_status & 0xf1) | (st & 2) | (st & 8) | (st & 4));
            s->m_bufFill = s->m_bufSize - e->m_free;
            /* The ROM ORs the two words and zero-extends (or; dsll32; dsrl32). */
            s->m_position = (u_long)(u_int)(e->m_posHi | e->m_posLo);
            if (e->m_status & 2)
            {
                /* Fire the callbacks the play position has passed. */
                RxSndPosCallback **link = &s->m_callbacks;
                for (RxSndPosCallback *cb = *link; cb != NULL; cb = *link)
                {
                    if (cb->m_posHi < e->m_posHi
                        || (cb->m_posHi == e->m_posHi && cb->m_posLo < e->m_posLo))
                    {
                        if (cb->m_func != NULL)
                            cb->m_func(s->m_id, cb->m_arg);
                        *link               = cb->m_next;
                        cb->m_next          = s_adpcmCallbackFree;
                        s_adpcmCallbackFree = cb;
                    }
                    else
                    {
                        link = &cb->m_next;
                    }
                }
            }
            break;
        }
    }

    /* Closed streams give back their IOP / SPU buffers once the IOP drops
     * them. */
    RxSndAdpcm **link = &s_adpcmStopping;
    for (RxSndAdpcm *s = *link; s != NULL; s = *link)
    {
        bool listed = false;
        for (int i = 0; i < status->m_count; i++)
        {
            if (status->m_entry[i].m_id == s->m_id)
            {
                listed = true;
                break;
            }
        }
        if (listed)
        {
            link = &s->m_next;
            continue;
        }
        if ((s->m_alloc & 1) && s->m_iopBuf != 0)
            rxSndIopFree(s->m_iopBuf);
        if ((s->m_alloc & 2) && s->m_spuBuf != 0)
            rxSndSpuFree(s->m_spuBuf);
        *link       = s->m_next;
        s->m_next   = s_adpcmFree;
        s_adpcmFree = s;
    }
}

// ROR 0x0023f940
short rxSndAdpcmGetMasterVolumeL(void)
{
    return s_adpcmMasterVolL;
}

// ROR 0x0023f950
short rxSndAdpcmGetMasterVolumeR(void)
{
    return s_adpcmMasterVolR;
}

/* ---- transport -------------------------------------------------------- */

// ROR 0x0023f960
int rxSndCreateSema(int maxCount, int initCount)
{
    struct SemaParam param;

    memset(&param, 0, sizeof(param));
    param.maxCount  = maxCount;
    param.initCount = initCount;
    return CreateSema(&param);
}

// ROR 0x0023f9b0
int rxSndBindRpc(sceSifClientData *cd, u_int rpcNo)
{
    if (rxSndGetDebugFlags() & 4)
        rxSysPrintf("binding IOP module %08X ...\n", rpcNo);
    do
    {
        if (sceSifBindRpc(cd, rpcNo, 0) < 0)
        {
            if (rxSndGetDebugFlags() & 1)
                rxSysPrintf("bind error '%08X'\n", rpcNo);
            return 0;
        }
        for (int i = 9999; i >= 0; i--)
            rxSndWait();
    } while (cd->serve == NULL);
    FlushCache(0);
    if (rxSndGetDebugFlags() & 4)
        rxSysPrintf("... binding completed\n");
    s_rpcSema = rxSndCreateSema(1, 1);
    return 1;
}

// ROR 0x0023faa8
void rxSndUnbindRpc(sceSifClientData *cd)
{
    (void)cd;
    DeleteSema(s_rpcSema);
}

/* The RPC's end function (interrupt context on the EE: iSignalSema, then
 * sync.p and ei, which the host does not need). */
// ROR 0x0023fac8
void rxSndRpcEndFunc(void)
{
    iSignalSema(s_rpcSema);
}

// ROR 0x0023faf8
void rxSndCallRpc(sceSifClientData *cd, u_int fno, void *send, int sendSize,
                  void *reply, int replySize)
{
    rxThDisableDispatch();
    WaitSema(s_rpcSema);
    FlushCache(0);
    sceSifCallRpc(cd, fno, 1, send, sendSize, reply, replySize, rxSndRpcEndFunc, NULL);
    rxThEnableDispatch();
    while (sceSifCheckStatRpc(&cd->rpcd) == 1)
        ;
    FlushCache(0);
}

// ROR 0x0023fbc0
void rxSndRpcInit(void)
{
    while (rxSndBindRpc(&s_rpcClient, 0xd000) == 0)
        ;
}

// ROR 0x0023fbf8
void rxSndRpcTerm(void)
{
    rxSndUnbindRpc(&s_rpcClient);
}

// ROR 0x0023fc18
u_int rxSndRpcCommand(u_int cmd, void *reply, int replySize, const void *send, int sendSize)
{
    const u_int *src = (const u_int *)send;
    for (int i = 0; i < (sendSize + 3) / 4; i++)
        s_rpcSend[rxSndGetBufferBank()][i] = *src++;

    int sendBank  = rxSndGetBufferBank();
    int replyBank = rxSndGetBufferBank();
    rxSndCallRpc(&s_rpcClient, cmd, s_rpcSend[sendBank], (sendSize + 0x3f) & ~0x3f,
                 s_rpcReply[replyBank], (replySize + 0x43) & ~0x3f);

    u_int *dst = (u_int *)reply;
    for (int i = 0; i < (replySize + 3) / 4; i++)
        *dst++ = s_rpcReply[rxSndGetBufferBank()][1 + i];
    return s_rpcReply[rxSndGetBufferBank()][0];
}

// ROR 0x0023fda8
void rxSndQueueBegin(u_int cmd, int replySize, int dataSize)
{
    if ((u_int)(s_queuePos + dataSize + 0xc) > 0x1000 || (u_int)(s_queueReplyPos + replySize) > 0x1000)
        rxSndQueueFlush();

    RxSndQueueHeader *head = (RxSndQueueHeader *)&s_queue[s_queuePos];
    head->m_dataSize  = dataSize;
    head->m_cmd       = cmd;
    head->m_replySize = replySize;
}

// ROR 0x0023fe40
void *rxSndQueueGetReply(void)
{
    return &s_queueReply[s_queueReplyPos];
}

// ROR 0x0023fe58
void *rxSndQueueGetData(void)
{
    return &s_queue[s_queuePos + sizeof(RxSndQueueHeader)];
}

// ROR 0x0023fe70
void rxSndQueueEnd(void)
{
    RxSndQueueHeader *head = (RxSndQueueHeader *)&s_queue[s_queuePos];
    s_queuePos      += head->m_dataSize + sizeof(RxSndQueueHeader);
    s_queueReplyPos += head->m_replySize;
}

/* Sends the queue as one call (fno 0x200), closed by an empty header; the
 * replies land in s_queueReply from offset 4 (word 0 is the call's own). */
// ROR 0x0023feb8
void rxSndQueueFlush(void)
{
    int pos      = s_queuePos;
    int replyPos = s_queueReplyPos;

    if (s_queuePos > 0)
    {
        RxSndQueueHeader *end = (RxSndQueueHeader *)&s_queue[s_queuePos];
        end->m_dataSize  = 0;
        s_queuePos      += sizeof(RxSndQueueHeader);
        end->m_cmd       = 0;
        end->m_replySize = 0;
        rxSndCallRpc(&s_rpcClient, 0x200, s_queue, (pos + 0x4b) & ~0x3f,
                     s_queueReply, (replyPos + 0x3f) & ~0x3f);
        s_queuePos      = 0;
        s_queueReplyPos = 4;
    }
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x002394e0
u_int rxSndSpuHeapAlloc(int mode, int arg, int size)
{
    ROSEPAN_STUB("rxSndSpuHeapAlloc", 0x002394e0);
    return {};
}

// stub of ROR 0x002398f8
void rxSndSpuHeapFree(u_int addr)
{
    ROSEPAN_STUB("rxSndSpuHeapFree", 0x002398f8);
}
