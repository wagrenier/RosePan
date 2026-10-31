/* ==========================================================================
 *  iop_rxsnd.cpp  (RXSNDDRV.IRX, the rx sound driver -- host stand-in)
 *
 *  Rule of Rose's IOP sound module is not reconstructed (it is an IRX on
 *  the disc, MODULES/, not code in the ELF).  rx/rxsnd.cpp, its EE client,
 *  binds RPC 0xd000 until the service is up and waits on a semaphore that
 *  only the call's end function signals, so without a service the boot
 *  hangs.  This is a silent driver: it answers every call with a zeroed
 *  reply, which the client reads as
 *
 *    - 0x11 (stop everything): done, so rxSndTermModules ends;
 *    - 0x41 (XVAB transfer busy?): no, the transfer is done;
 *    - 0x200 (the queued commands): every status list empty, so stopped
 *      slots go back to their free lists and nothing reports playing;
 *
 *  except the IOP heap (0x20 alloc {mode, size, 0}): a bump allocator over a
 *  fake IOP address range, so rxSndIopAlloc does not print "can't allocate
 *  IOP heap" and its callers get distinct non-zero addresses; the XVAB and
 *  XSEQ opens (0x50 {id, iop, spu}, 0x60 {id, iop}), which answer the id;
 *  and the SE voices:
 *  every voice a queued 0x80 / 0x81 started is listed once in the next SE
 *  status (0x83) with status 0, i.e. ended at once, so rxSndSeUpdate stops
 *  it and its slot goes back to the free list; and the ADPCM streams (0xa0
 *  open, 0xa1 data, 0xa2 close, 0xa3 status): each open stream is listed
 *  playing, its IOP buffer draining at the stream's own rate, so the
 *  client keeps writing at the real pace (the movie player's demuxer waits
 *  on its audio que, which only drains through rxSndAdpcmWrite).  No sound
 *  is produced.  Playing sound means answering the play / status commands for
 *  real and mixing the voices on the host (open-items.md).
 *
 *  The reconstruction of the real driver (src/iop/rxsnddrv/) answers every
 *  command now and runs by default (2026-10-30); this stand-in runs only
 *  with ROSEPAN_RXSNDDRV=0 in the environment, a fallback while the
 *  reconstruction is being heard out.
 * ======================================================================== */

#include "sifcmd.h"
#include "iop/rxsnddrv/rxsnddrv.h"

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <chrono>

#define RXSND_RPC_NO        0xd000

#define RXSND_IOP_HEAP_BASE 0x00100000u   /* fake IOP addresses, never dereferenced */
#define RXSND_IOP_HEAP_END  0x00200000u

static sceSifQueueData s_queue;
static sceSifServeData s_serve;
static u_int           s_recv[0x1100 / 4];     /* the client sends up to 0x1040 bytes */
static u_int           s_reply[0x1100 / 4];
static u_int           s_heapNext = RXSND_IOP_HEAP_BASE;
static u_char          s_seStarted[64];        /* voices started since the last SE status */
static int             s_seStartedCount;

/* An ADPCM stream: the bytes per channel announced (0xa1) and drained. */
struct RxSndDriverAdpcm
{
    int    open;
    int    iopSize;             /* per channel */
    int    rate;                /* Hz, from the pitch */
    double queued;
    double drained;
    std::chrono::steady_clock::time_point last;
};
static RxSndDriverAdpcm s_adpcm[48];

/* The 0xa3 status: every open stream playing, with what its buffer can take
 * after draining 16 bytes per 28 samples (one VAG block) since last time. */
static void RxSndDriverAdpcmStatus(u_char *reply)
{
    const auto now = std::chrono::steady_clock::now();
    int count = 0;
    for (int id = 0; id < 48; id++)
    {
        RxSndDriverAdpcm *a = &s_adpcm[id];
        if (!a->open)
            continue;
        const double sec = std::chrono::duration<double>(now - a->last).count();
        a->last = now;
        a->drained += sec * a->rate * 16.0 / 28.0;
        if (a->drained > a->queued)
            a->drained = a->queued;     /* run dry: it waits for data */
        int held = (int)(a->queued - a->drained);
        int free = a->iopSize - held;
        if (free < 0)
            free = 0;
        u_char *e = reply + count * 0x10;
        e[0]                 = (u_char)id;
        e[1]                 = 2;       /* playing */
        *(int *)(e + 4)      = free;
        *(u_int *)(e + 8)    = 0;
        *(u_int *)(e + 0xc)  = (u_int)a->drained;
        count++;
    }
    *(int *)(reply + 0x300) = count;
}

/* Walks the queued commands of call 0x200: {cmd, replySize, dataSize} then
 * the data, closed by cmd 0; replies are laid out from offset 4. */
static void RxSndDriverQueue(const u_char *data, int size)
{
    int pos      = 0;
    int replyPos = 4;

    while (pos + 0xc <= size)
    {
        const u_int *head      = (const u_int *)(data + pos);
        u_int        cmd       = head[0];
        int          replySize = (int)head[1];
        int          dataSize  = (int)head[2];
        const u_char *arg      = data + pos + 0xc;

        if (cmd == 0 || dataSize < 0 || pos + 0xc + dataSize > size)
            break;
        if ((cmd == 0x80 || cmd == 0x81) && dataSize >= 1 && s_seStartedCount < 64)
            s_seStarted[s_seStartedCount++] = arg[0];
        if (cmd == 0xa0 && dataSize >= 0x10 && arg[0] < 48)
        {
            RxSndDriverAdpcm *a = &s_adpcm[arg[0]];
            a->open    = 1;
            a->iopSize = *(const int *)(arg + 8);
            a->rate    = *(const short *)(arg + 6) * 48000 / 4096;
            a->queued  = 0.0;
            a->drained = 0.0;
            a->last    = std::chrono::steady_clock::now();
        }
        if (cmd == 0xa1 && dataSize >= 8 && arg[0] < 48)
            s_adpcm[arg[0]].queued += *(const int *)(arg + 4);
        /* 0xa2 {id, param, value}: param 0 closes (1.. pause, volume ...). */
        if (cmd == 0xa2 && dataSize >= 8 && *(const u_int *)arg < 48 && *(const int *)(arg + 4) == 0)
            s_adpcm[*(const u_int *)arg].open = 0;
        if (cmd == 0xa3 && replySize >= 0x304 && replyPos + 0x304 <= (int)sizeof(s_reply))
            RxSndDriverAdpcmStatus((u_char *)s_reply + replyPos);
        if (cmd == 0x83 && replySize >= 0x104 && replyPos + 0x104 <= (int)sizeof(s_reply))
        {
            u_char *reply = (u_char *)s_reply + replyPos;
            for (int i = 0; i < s_seStartedCount; i++)
            {
                reply[i * 4]     = s_seStarted[i];
                reply[i * 4 + 1] = 0;
            }
            *(int *)(reply + 0x100) = s_seStartedCount;
            s_seStartedCount        = 0;
        }
        replyPos += replySize;
        pos      += 0xc + dataSize;
    }
}

static void *RxSndDriverCall(u_int fno, void *data, int size)
{
    const u_int *req = (const u_int *)data;

    memset(s_reply, 0, sizeof(s_reply));

    if (fno == 0x20 && req != NULL && size >= 8)
    {
        u_int bytes = (req[1] + 0x3f) & ~0x3fu;
        if (bytes != 0 && s_heapNext + bytes <= RXSND_IOP_HEAP_END)
        {
            s_reply[0]  = s_heapNext;
            s_heapNext += bytes;
        }
    }
    else if ((fno == 0x50 || fno == 0x60) && req != NULL && size >= 4)
    {
        s_reply[0] = req[0];
    }
    else if (fno == 0x200 && data != NULL)
    {
        RxSndDriverQueue((const u_char *)data, size);
    }

    return s_reply;
}

void RosePan_IopRxSndStart(void)
{
    const char *env = getenv("ROSEPAN_RXSNDDRV");
    if (env == NULL || env[0] != '0')
    {
        printf("iop: RXSNDDRV reconstruction\n");
        RxSndDrvStart();
        return;
    }

    printf("iop: RXSNDDRV silent stand-in (ROSEPAN_RXSNDDRV=0)\n");
    sceSifSetRpcQueue(&s_queue, 0);
    sceSifRegisterRpc(&s_serve, RXSND_RPC_NO, RxSndDriverCall, s_recv, NULL, NULL, &s_queue);
    sceSifRpcLoop(&s_queue);
}
