/* ==========================================================================
 *  rx/rxthread.h
 *
 *  The rx middleware's thread layer (ROR 0x249b88..0x24b490): a scheduler
 *  on top of the EE kernel's priority threads.
 *
 *  - The main thread (the one that calls rxThInit) runs at the base
 *    priority + prio index 0. Up to 8 rx threads (rxThCreate; the file
 *    thread is one) start at the lowest level, base + prio index 2.
 *  - rxThYield on the main thread hands the CPU over: it leaves a request
 *    (a function the dispatch alarm polls; by default "the vblank count
 *    changed"), resumes the worker thread (which runs the call queue
 *    rxThPostCall fills) and waits on the yield semaphore.
 *  - The dispatch alarm (every 4..256 HSYNCs, rxThSetInterval) gives the rx
 *    threads turns round-robin: a thread runs on every (priority + 1)-th
 *    pass that lands on it, and the alarm wakes the main thread once its
 *    request answers non-zero. rxThDisableDispatch / rxThEnableDispatch
 *    nest (a semaphore used as a counter) and stop the rotation.
 *  - rxThSync reads the vblank counter (an INTC cause-2 handler) and timer
 *    0 counting HBLANKs: the frame pacing the game is built on.
 *
 *  An rx thread is an index into the slot table (bss 0x807f70).
 * ======================================================================== */
#ifndef _RX_RXTHREAD_H
#define _RX_RXTHREAD_H

#include <stddef.h>
#include <stdint.h>

#define RX_TH_MAX       8       /* rx thread slots      */
#define RX_TH_CALL_MAX  32      /* call queue entries   */
#define RX_TH_CB_MAX    8       /* yield callbacks      */

/* The arguments are words on the EE; intptr_t so a host pointer fits. */
typedef int  (*RxThRequestFunc)(intptr_t *args);
typedef void (*RxThCallFunc)(intptr_t *args);
typedef void (*RxThCallback)(int yielding);

/* One rx thread (bss 0x807f70, 8 x EE 0x10).
 * layout: host -- param is a pointer (EE +0xc, 4 bytes). */
struct RxThSlot
{
    short id;           // ROR +0x0: kernel thread id, -1 when free
    short state;        // ROR +0x2: 0 = not dispatched (rxThSetState)
    int   count;        // ROR +0x4: passes since its last turn
    int   priority;     // ROR +0x8: passes between turns (rxThSetPriority)
    void *param;        // ROR +0xc: rxThGetParam
};
static_assert(offsetof(RxThSlot, id) == 0x0, "RxThSlot.id");
static_assert(offsetof(RxThSlot, state) == 0x2, "RxThSlot.state");
static_assert(offsetof(RxThSlot, count) == 0x4, "RxThSlot.count");
static_assert(offsetof(RxThSlot, priority) == 0x8, "RxThSlot.priority");
static_assert(offsetof(RxThSlot, param) >= 0xc, "RxThSlot.param");

/* One entry of the worker's call queue (bss 0x807ff0, 32 x EE 0x10).
 * layout: host -- func is a pointer (EE +0x0, 4 bytes), the args words. */
struct RxThCall
{
    RxThCallFunc func;      // ROR +0x0
    intptr_t     args[3];   // ROR +0x4
};
static_assert(offsetof(RxThCall, func) == 0x0, "RxThCall.func");
static_assert(offsetof(RxThCall, args) >= 0x4, "RxThCall.args");

int  rxThCreateSema(int maxCount, int initCount);               // ROR 0x00249b88
int  rxThStartThread(void (*entry)(void *), void *stack, int stackSize,
                     int priority);                             // ROR 0x00249bb0
void rxThKillThread(int id);                                    // ROR 0x00249c08
void rxThWorkerMain(void *arg);                                 // ROR 0x00249c30
void rxThIdleMain(void *arg);                                   // ROR 0x00249cf0
int  rxThIsDispatchDisabled(void);                              // ROR 0x00249d48
int  rxThGetCallCount(void);                                    // ROR 0x00249de0
int  rxThPostCall(RxThCallFunc func, intptr_t a, intptr_t b, intptr_t c); // ROR 0x00249e08
void rxThInit(void);                                            // ROR 0x00249ea0
void rxThExit(void);                                            // ROR 0x0024a058
void rxThRunCallbacks(int yielding);                            // ROR 0x0024a1b8
int  rxThFindCallback(RxThCallback func);                       // ROR 0x0024a218
void rxThAddCallback(RxThCallback func);                        // ROR 0x0024a258
void rxThRemoveCallback(RxThCallback func);                     // ROR 0x0024a2e0
int  rxThIsYielding(void);                                      // ROR 0x0024a310
void rxThLockRequest(void);                                     // ROR 0x0024a320
void rxThUnlockRequest(void);                                   // ROR 0x0024a3c8
int  rxThIsRequestLocked(void);                                 // ROR 0x0024a470
void rxThYield(void);                                           // ROR 0x0024a508
void rxThDispatchAlarm(int id, unsigned short time, void *arg); // ROR 0x0024a678
void rxThDisableDispatch(void);                                 // ROR 0x0024a8c8
void rxThEnableDispatch(void);                                  // ROR 0x0024a958
int  rxThGetIndex(void);                                        // ROR 0x0024aae0
void rxThDelete(int index);                                     // ROR 0x0024ab40
int  rxThGetCount(void);                                        // ROR 0x0024ac18
void rxThSetPriority(int index, int priority);                  // ROR 0x0024ac28
int  rxThGetPriority(int index);                                // ROR 0x0024ac48
void *rxThGetParam(int index);                                  // ROR 0x0024ac68
void rxThSetParam(int index, void *param);                      // ROR 0x0024ac88
int  rxThGetState(int index);                                   // ROR 0x0024aca8
void rxThSetState(int index, short state);                      // ROR 0x0024acc8
int  rxThCreate(void (*entry)(void), void *stack, int stackSize,
                int priority, void *param);                     // ROR 0x0024ace8
void rxThRequest(RxThRequestFunc func, intptr_t a, intptr_t b, intptr_t c); // ROR 0x0024ae60
void rxThDummy0(void);                                          // ROR 0x0024af58
void rxThDummy1(void);                                          // ROR 0x0024af60
int  rxThVSyncRequest(intptr_t *args);                          // ROR 0x0024af68
int  rxThVSync(int mode);                                       // ROR 0x0024afa0
int  rxThGetInterval(void);                                     // ROR 0x0024afe8
void rxThSetInterval(int hsyncs);                               // ROR 0x0024aff8
void rxThSetPrioIndex(int mainIndex, int workerIndex, int lowIndex); // ROR 0x0024b048
void rxThInitOnce(void);                                        // ROR 0x0024b0b0
void rxThExitOnce(void);                                        // ROR 0x0024b0c8
int  rxThGsSyncRequest(intptr_t *args);                         // ROR 0x0024b0e0
int  rxThGsSyncPath(int mode);                                  // ROR 0x0024b108
int  rxThDmaSyncRequest(intptr_t *args);                        // ROR 0x0024b158
int  rxThDmaSync(int chan, int mode);                           // ROR 0x0024b188
int  rxThVBlankHandler(int cause);                              // ROR 0x0024b1f8
void rxThVBlankInit(void);                                      // ROR 0x0024b278
void rxThVBlankExit(void);                                      // ROR 0x0024b300
int  rxThReadHSync(void);                                       // ROR 0x0024b360
void rxThWaitVBlank(void);                                      // ROR 0x0024b398
int  rxThGetField(void);                                        // ROR 0x0024b3e8
int  rxThSync(int mode);                                        // ROR 0x0024b3f8

#endif /* _RX_RXTHREAD_H */
