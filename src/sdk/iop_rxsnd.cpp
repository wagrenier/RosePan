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
 *  IOP heap" and its callers get distinct non-zero addresses.  No sound is
 *  produced.  Playing sound means answering the play / status commands for
 *  real and mixing the voices on the host (open-items.md).
 * ======================================================================== */

#include "sifcmd.h"

#include <string.h>

#define RXSND_RPC_NO        0xd000

#define RXSND_IOP_HEAP_BASE 0x00100000u   /* fake IOP addresses, never dereferenced */
#define RXSND_IOP_HEAP_END  0x00200000u

static sceSifQueueData s_queue;
static sceSifServeData s_serve;
static u_int           s_recv[0x1100 / 4];     /* the client sends up to 0x1040 bytes */
static u_int           s_reply[0x1100 / 4];
static u_int           s_heapNext = RXSND_IOP_HEAP_BASE;

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

    return s_reply;
}

void RosePan_IopRxSndStart(void)
{
    sceSifSetRpcQueue(&s_queue, 0);
    sceSifRegisterRpc(&s_serve, RXSND_RPC_NO, RxSndDriverCall, s_recv, NULL, NULL, &s_queue);
    sceSifRpcLoop(&s_queue);
}
