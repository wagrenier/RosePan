/* ==========================================================================
 *  eekernel.h  (EE kernel / libgraph entry points — PC-port shim)
 *
 *  Declarations (not implementations) for the handful of EE runtime services
 *  the reconstruction calls: semaphores, cache/exception/DMAC handlers, the GS
 *  interrupt-mask register, and scePrintf.  On the PC these are stubbed/ported
 *  under src/sdk as the port progresses; here they only need to be declared so
 *  the callers compile.
 * ======================================================================== */

#ifndef _EEKERNEL_H
#define _EEKERNEL_H

#include "scetypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- threads / semaphores (libkernel) --------------------------------- */
/* Field layout verbatim from the prototype's debug info (types.txt). */
struct SemaParam
{
    int          currentCount;
    int          maxCount;
    int          initCount;
    int          numWaitThreads;
    unsigned int attr;
    unsigned int option;
};

int RosePan_CreateSema(struct SemaParam *param);
int RosePan_DeleteSema(int sema_id);
int RosePan_SignalSema(int sema_id);
int RosePan_iSignalSema(int sema_id);
int RosePan_WaitSema(int sema_id);
int RosePan_PollSema(int sema_id);

/* ---- threads (libkernel) ---------------------------------------------- */
struct ThreadParam
{
    int          status;

    void (*entry)(void *dummy);
    void        *stack;
    int          stackSize;
    void        *gpReg;
    int          initPriority;
    int          currentPriority;
    unsigned int attr;
    unsigned int option;
    int          waitType;
    int          waitId;
    int          wakeupCount;
};

int  RosePan_CreateThread(struct ThreadParam *param);
int  RosePan_StartThread(int thread_id, void *arg);

/* PORT: a host thread cannot be pre-empted from outside, so Terminate only
 * marks the slot dormant and Delete detaches it and gives the slot back.  Both
 * are safe for the one caller in the tree -- playPssEnd() tears down
 * videoDecMain(), which returns of its own accord as soon as the decoder
 * reports end of stream. */
int  RosePan_TerminateThread(int thread_id);
int  RosePan_DeleteThread(int thread_id);

int  RosePan_SuspendThread(int thread_id);

/* Claim the suspend mutex for the span the loader thread spends reading load
 * queue entries -- see the comment on these in eekernel.cpp. */
void RosePan_LoaderCriticalEnter(void);
void RosePan_LoaderCriticalLeave(void);
int  RosePan_ResumeThread(int thread_id);
int  RosePan_GetThreadId(void);
int  RosePan_SleepThread(void);
int  RosePan_WakeupThread(int thread_id);
int  RosePan_ChangeThreadPriority(int thread_id, int priority);
int  RosePan_ReferThreadStatus(int thread_id, struct ThreadParam *info);

/* ---- cache / exception / DMAC handlers -------------------------------- */
typedef void (*RosePanDebugHandler)(u_int stat, u_int cause, u_int epc,
                                   u_int bva, u_int bpa, u_long128 *gpr);
typedef int (*RosePanDmacHandler)(int channel);
typedef int (*RosePanDmacHandler2)(int channel, void *arg, void *addr);

void RosePan_FlushCache(int mode);
int  RosePan_SetDebugHandler(int cause, RosePanDebugHandler handler);
int  RosePan_AddDmacHandler(int channel, RosePanDmacHandler handler, int next);
int  RosePan_AddDmacHandler2(int channel, RosePanDmacHandler2 handler, int next, void *arg);
int  RosePan_EnableDmac(int channel);

/* ---- INTC handlers ----------------------------------------------------- */
/* Cause 2 is INTC_VBLANK_START, the only one the tree registers for: playpss.c
 * hangs its PAL frame-rate divider off it.
 *
 * PORT: the registry is real -- Add returns a handle Remove takes back, and
 * RosePan_IntcRaise() dispatches -- but NOTHING calls RosePan_IntcRaise() yet,
 * because the port's frame loop has no vblank interrupt to hang it off.  The
 * consequence is confined and named at the call site: videoDecMain() would park
 * on its semaphore for ever if a real decoder ever ran it in PAL mode. */
typedef int (*RosePanIntcHandler)(int cause);

int  RosePan_AddIntcHandler(int cause, RosePanIntcHandler handler, int next);
int  RosePan_RemoveIntcHandler(int cause, int handler_id);
int  RosePan_EnableIntc(int cause);
int  RosePan_DisableIntc(int cause);
void RosePan_IntcRaise(int cause);

/* Device reset helpers. */
int  RosePan_sceDevVif0Reset(void);
int  RosePan_sceDevVu0Reset(void);
int  RosePan_sceDevVif1Reset(void);
int  RosePan_sceDevVu1Reset(void);

/* Main memory size: a retail console's 32 MB (a devkit reports 128 MB, which
 * Rule of Rose's rx layer reads as "boot from host0:"). */
int  RosePan_GetMemorySize(void);

/* Interrupt disable / enable.  PORT: EE threads are host threads here, so the
 * sections the rx layer guards with DIntr ... EIntr (against its file thread
 * and alarm handlers) take one process-wide recursive mutex instead; the ROM
 * always pairs the two, so they nest. */
int  RosePan_DIntr(void);
int  RosePan_EIntr(void);

/* Run another ELF in place of this one.  The host cannot: it logs and exits. */
void RosePan_LoadExecPS2(const char *name, int argc, char **argv);

/* EE COP0 pipeline sync + interrupt enable (asm intrinsics on the target). */
#ifndef SYNC
#define SYNC(t) ((void)0)
#endif
#ifndef EI
#define EI()    ((void)0)
#endif

/* ---- GS interrupt-mask register --------------------------------------- */
u_long RosePan_sceGsGetIMR(void);
u_long RosePan_sceGsPutIMR(u_long imr);

/* scePrintf is provided as an ambient `#define scePrintf printf` by
 * pc_prefix.h (force-included), so it is intentionally not declared here. */

#ifdef __cplusplus
}
#endif

/* VU0 CFC2 (read a COP2 control register) — no VU0 on the host. */
#ifndef ROSEPAN_EEKERNEL_NO_COMPAT_MACROS
#define CreateSema             RosePan_CreateSema
#define DeleteSema             RosePan_DeleteSema
#define SignalSema             RosePan_SignalSema
#define iSignalSema            RosePan_iSignalSema
#define WaitSema               RosePan_WaitSema
#define PollSema               RosePan_PollSema
#define CreateThread           RosePan_CreateThread
#define StartThread            RosePan_StartThread
#define TerminateThread        RosePan_TerminateThread
#define DeleteThread           RosePan_DeleteThread
#define SuspendThread          RosePan_SuspendThread
#define ResumeThread           RosePan_ResumeThread
#define GetThreadId            RosePan_GetThreadId
#define SleepThread            RosePan_SleepThread
#define WakeupThread           RosePan_WakeupThread
#define ChangeThreadPriority   RosePan_ChangeThreadPriority
#define ReferThreadStatus      RosePan_ReferThreadStatus
#define FlushCache             RosePan_FlushCache
#define SetDebugHandler        RosePan_SetDebugHandler
#define AddDmacHandler         RosePan_AddDmacHandler
#define AddDmacHandler2        RosePan_AddDmacHandler2
#define EnableDmac             RosePan_EnableDmac
#define AddIntcHandler         RosePan_AddIntcHandler
#define RemoveIntcHandler      RosePan_RemoveIntcHandler
#define EnableIntc             RosePan_EnableIntc
#define DisableIntc            RosePan_DisableIntc
#define sceDevVif0Reset        RosePan_sceDevVif0Reset
#define sceDevVu0Reset         RosePan_sceDevVu0Reset
#define sceDevVif1Reset        RosePan_sceDevVif1Reset
#define sceDevVu1Reset         RosePan_sceDevVu1Reset
#define GetMemorySize          RosePan_GetMemorySize
#define DIntr                  RosePan_DIntr
#define EIntr                  RosePan_EIntr
#define LoadExecPS2            RosePan_LoadExecPS2
#define sceGsGetIMR            RosePan_sceGsGetIMR
#define sceGsPutIMR            RosePan_sceGsPutIMR
#endif

#ifndef _cfc2
#define _cfc2(reg) (0u)
#endif

#endif /* _EEKERNEL_H */
