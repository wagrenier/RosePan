/* ==========================================================================
 *  rx/rxthread.cpp
 *
 *  The rx middleware's thread layer: kernel wrappers, the scheduler
 *  (rxThInit, the dispatch alarm, rxThYield, rxThRequest), the rx thread
 *  slots, the worker's call queue, yield callbacks, and the vblank / HSYNC
 *  counters behind rxThSync. See rxthread.h.
 *
 *  The ROM masks interrupts by clearing COP0 Status.IE around its kernel
 *  i-calls and saves / restores Status around DIntr ... EIntr; the host's
 *  sections are a recursive mutex (src/sdk) and the i-calls are the plain
 *  ones, so neither is written here. Every spin-wait also prints "DI error"
 *  when it finds interrupts disabled (rxThSpin, empty on the host, as
 *  rxfile's).
 *
 *  PORT: EE threads are host threads, so the rx threads run alongside the
 *  main thread instead of only in its yields, and the priority changes do
 *  nothing. What keeps working is the handshake: the main thread's
 *  rxThYield blocks until the dispatch alarm (a host timer thread) sees
 *  its request answer, and rxThSync counts the vblanks libgraph's vblank
 *  thread raises. Spins that would hold a host core for ever sleep instead
 *  (marked PC:).
 * ======================================================================== */
#include "rx/rxthread.h"

#include "rosepan/os/rosepan_time.h"
#include "rx/rxsys.h"
#include "sdk/eekernel.h"
#include "sdk/eeregs.h"
#include "sdk/libdma.h"
#include "sdk/libgraph.h"

/* ---- data ------------------------------------------------------------- */

static int s_initCount;                         /* sdata 0x2dc260 */
static int s_basePriority;                      /* sdata 0x2dc264: the main thread's at rxThInit */
static volatile int s_callWrite;                /* sdata 0x2dc268 */
static volatile int s_callRead;                 /* sdata 0x2dc26c */
static RxThRequestFunc s_request;               /* sdata 0x2dc270 */
static int s_interval = 4;                      /* sdata 0x2dc274: alarm period, HSYNCs */
static int s_prioIndex[3] = { 0, 2, 4 };        /* sdata 0x2dc278: main, worker, low */
static int s_vblankHandler = -1;                /* sdata 0x2dc284 */
static int s_field;                             /* sdata 0x2dc288 */
static int s_vblankUsers;                       /* sdata 0x2dc28c */
static volatile unsigned s_vblankCount;         /* sdata 0x2dc290 */

static int s_yieldSema;                         /* bss 0x807f40: the main thread waits here */
static int s_dispatchSema;                      /* bss 0x807f44: count = dispatch disables  */
static int s_requestSema;                       /* bss 0x807f48: count = request locks      */
static int s_mainThread;                        /* bss 0x807f4c */
static int s_workerThread;                      /* bss 0x807f50 */
static int s_idleThread;                        /* bss 0x807f54 */
static int s_alarm = -1;                        /* bss 0x807f58 */
static int s_yieldTick;                         /* bss 0x807f5c */
static int s_yielding;                          /* bss 0x807f60 */
static int s_threadCount;                       /* bss 0x807f64 */
static int s_current;                           /* bss 0x807f68: slot of the last turn */
static RxThSlot s_slots[RX_TH_MAX];             /* bss 0x807f70 */
static RxThCall s_calls[RX_TH_CALL_MAX];        /* bss 0x807ff0 */
static intptr_t s_requestArgs[3];               /* bss 0x8081f0 */
static RxThCallback s_callbacks[RX_TH_CB_MAX];  /* bss 0x808200 */
static unsigned char s_workerStack[0x10000];    /* bss 0x808240 */
static unsigned char s_idleStack[0x400];        /* bss 0x818240 */
static unsigned s_lastVBlank;                   /* bss 0x818640: vblank count at the last read */
static int s_hsyncCarry;                        /* bss 0x818644 */

// The ROM's spin-wait step: warn ("DI error") when interrupts are off.
// PORT: no Status register on the host; the wait just spins.
static void rxThSpin(void)
{
}

/* ---- kernel wrappers -------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00249b88

int rxThCreateSema(int maxCount, int initCount)
{
    SemaParam param;
    param.maxCount = maxCount;
    param.initCount = initCount;
    return CreateSema(&param);
}

// ──────────────────────────────────────────────────────────────────────
// Create and start a thread with the caller's gp.
// ROR 0x00249bb0

int rxThStartThread(void (*entry)(void *), void *stack, int stackSize, int priority)
{
    ThreadParam param;
    param.entry = entry;
    param.stack = stack;
    param.stackSize = stackSize;
    param.gpReg = 0;            // PC: the ROM passes &_gp
    param.initPriority = priority;
    int id = CreateThread(&param);
    StartThread(id, 0);
    return id;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00249c08

void rxThKillThread(int id)
{
    TerminateThread(id);
    DeleteThread(id);
}

// ──────────────────────────────────────────────────────────────────────
// The worker thread: runs the call queue, one entry at a time, for ever.
// It is resumed only while the main thread yields.
// ROR 0x00249c30

void rxThWorkerMain(void *arg)
{
    (void)arg;
    for (;;)
    {
        int i = s_callRead;
        while (s_callWrite == i)
        {
            rxThSpin();             // callcheck: EIntr, rxSysPrintf (the DI check)
            RosePan_Sleep(1);       // PC: a host thread would hold a core here
        }
        s_calls[i].func(s_calls[i].args);
        s_callRead = (i + 1) & (RX_TH_CALL_MAX - 1);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The idle thread: the lowest priority, so the kernel always has a thread
// to run; it only does the DI check.
// ROR 0x00249cf0

void rxThIdleMain(void *arg)
{
    (void)arg;
    for (;;)
    {
        rxThSpin();                 // callcheck: EIntr, rxSysPrintf (the DI check)
        SleepThread();              // PC: nobody wakes it; the ROM spins
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00249d48

int rxThIsDispatchDisabled(void)
{
    SemaParam st;
    iReferSemaStatus(s_dispatchSema, &st);
    return st.currentCount > 0;
}

/* ---- the worker's call queue ------------------------------------------ */

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00249de0

int rxThGetCallCount(void)
{
    return (s_callWrite - s_callRead) & (RX_TH_CALL_MAX - 1);
}

// ──────────────────────────────────────────────────────────────────────
// Queue func(args) for the worker; 0 when the queue is full (31 entries).
// ROR 0x00249e08

int rxThPostCall(RxThCallFunc func, intptr_t a, intptr_t b, intptr_t c)
{
    int count = rxThGetCallCount();
    if (count != RX_TH_CALL_MAX - 1)
    {
        // PC: the ROM bumps the write index before it fills the entry (the
        // worker cannot run in between on the EE); the host's worker can, so
        // the entry is filled first.
        int i = s_callWrite;
        s_calls[i].func = func;
        s_calls[i].args[0] = a;
        s_calls[i].args[1] = b;
        s_calls[i].args[2] = c;
        s_callWrite = (i + 1) & (RX_TH_CALL_MAX - 1);
    }
    return count != RX_TH_CALL_MAX - 1;
}

/* ---- init / exit ------------------------------------------------------ */

// ──────────────────────────────────────────────────────────────────────
// First call: the vblank counter, empty slots and callbacks, the three
// semaphores, the idle thread and the (suspended) worker thread, at the
// calling thread's priority + the prio indices.
// ROR 0x00249ea0

void rxThInit(void)
{
    s_initCount++;
    if (s_initCount != 1)
        return;

    rxThVBlankInit();
    s_yielding = 0;
    for (int i = RX_TH_MAX - 1; i >= 0; i--)
        s_slots[i].id = -1;
    for (int i = RX_TH_CB_MAX - 1; i >= 0; i--)
        s_callbacks[i] = 0;
    s_current = 0;
    s_threadCount = 0;

    ThreadParam info;
    s_mainThread = GetThreadId();
    ReferThreadStatus(s_mainThread, &info);
    s_basePriority = info.currentPriority;

    s_yieldSema = rxThCreateSema(1, 0);
    s_dispatchSema = rxThCreateSema(0x10000, 0);
    s_requestSema = rxThCreateSema(0x10000, 0);
    s_callWrite = 0;
    s_callRead = 0;
    s_idleThread = rxThStartThread(rxThIdleMain, s_idleStack, sizeof(s_idleStack),
                                   s_basePriority + s_prioIndex[2]);
    s_workerThread = rxThStartThread(rxThWorkerMain, s_workerStack, sizeof(s_workerStack),
                                     s_basePriority + s_prioIndex[1]);
    SuspendThread(s_workerThread);
    s_request = 0;
    s_alarm = -1;
}

// ──────────────────────────────────────────────────────────────────────
// Last call: undo rxThInit.
// ROR 0x0024a058

void rxThExit(void)
{
    if (s_initCount <= 0)
        return;
    s_initCount--;
    if (s_initCount != 0)
        return;

    DIntr();
    if (s_alarm >= 0)
    {
        ReleaseAlarm(s_alarm);
        s_alarm = -1;
    }
    s_threadCount = 0;
    for (int i = 0; i < RX_TH_MAX; i++)
    {
        if (s_slots[i].id >= 0)
        {
            rxThKillThread(s_slots[i].id);
            s_slots[i].id = -1;
        }
    }
    EIntr();
    rxThKillThread(s_workerThread);
    rxThKillThread(s_idleThread);
    DeleteSema(s_requestSema);
    DeleteSema(s_dispatchSema);
    DeleteSema(s_yieldSema);
    rxThVBlankExit();
}

/* ---- yield callbacks -------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// yielding: 1 when the main thread enters rxThYield, 0 when it leaves.
// ROR 0x0024a1b8

void rxThRunCallbacks(int yielding)
{
    for (int i = 0; i < RX_TH_CB_MAX; i++)
    {
        if (s_callbacks[i] != 0)
            s_callbacks[i](yielding);   // callcheck: rxSysThreadCallback
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024a218

int rxThFindCallback(RxThCallback func)
{
    for (int i = 0; i < RX_TH_CB_MAX; i++)
    {
        // ROM bug: the match branch's delay slot loads -1 into the result,
        // so this never finds anything (rxThAddCallback does not refuse a
        // duplicate, rxThRemoveCallback removes nothing).
        if (s_callbacks[i] == func)
            return -1;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024a258

void rxThAddCallback(RxThCallback func)
{
    if (rxThFindCallback(func) >= 0)
        return;
    for (int i = 0; i < RX_TH_CB_MAX; i++)
    {
        if (s_callbacks[i] == 0)
        {
            s_callbacks[i] = func;
            return;
        }
    }
    scePrintf("rxthread: rxThAddCallback failed! (%08X) \n", (unsigned)(uintptr_t)func);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024a2e0

void rxThRemoveCallback(RxThCallback func)
{
    int i = rxThFindCallback(func);
    if (i >= 0)
        s_callbacks[i] = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024a310

int rxThIsYielding(void)
{
    return s_yielding;
}

/* ---- request lock (worker thread) ------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Hold the main thread in its yield: the alarm only checks the request
// while the request semaphore's count is 0.
// ROR 0x0024a320

void rxThLockRequest(void)
{
    if (GetThreadId() == s_workerThread)
        iSignalSema(s_requestSema);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024a3c8

void rxThUnlockRequest(void)
{
    if (GetThreadId() == s_workerThread)
        iPollSema(s_requestSema);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024a470

int rxThIsRequestLocked(void)
{
    SemaParam st;
    iReferSemaStatus(s_requestSema, &st);
    return st.currentCount > 0;
}

/* ---- scheduling ------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Main thread: run the rx threads until the request answers (by default
// until the next vblank). An rx thread: drop back below the main thread.
// The worker: nothing.
// ROR 0x0024a508

void rxThYield(void)
{
    rxThSpin();                     // callcheck: EIntr, rxSysPrintf (the DI check)
    int self = GetThreadId();
    if (self != s_mainThread)
    {
        if (self == s_workerThread)
            return;
        ChangeThreadPriority(s_mainThread, s_basePriority + s_prioIndex[0]);
        ChangeThreadPriority(self, s_basePriority + s_prioIndex[2]);
        RosePan_Sleep(1);           // PC: the priority change hands the CPU over on the EE
        return;
    }

    s_yieldTick = rxThSync(-1);
    if (s_request == 0)
        rxThRequest(rxThVSyncRequest, s_yieldTick, 0, 0);
    rxThRunCallbacks(1);
    s_yielding = 1;
    ResumeThread(s_workerThread);
    WaitSema(s_yieldSema);
    SuspendThread(s_workerThread);
    s_yielding = 0;
    rxThRunCallbacks(0);
}

// ──────────────────────────────────────────────────────────────────────
// The dispatch alarm. Unless dispatch is disabled: drop the last slot's
// thread, find the next dispatched slot; every (priority + 1)-th pass on
// it the thread gets the CPU instead of the main thread. Then wake the
// main thread when its request answers (and it waits, and no lock holds
// it), and re-arm while there are rx threads or a request.
// ROR 0x0024a678

void rxThDispatchAlarm(int id, unsigned short time, void *arg)
{
    (void)id;
    (void)time;
    (void)arg;

    s_alarm = -1;
    SemaParam st;
    iReferSemaStatus(s_dispatchSema, &st);
    if (st.currentCount > 0)
        return;                     // ExitHandler (EI)

    int cur = s_current;
    if (s_slots[cur].id >= 0)
        iChangeThreadPriority(s_slots[cur].id, s_basePriority + s_prioIndex[2]);
    for (int i = 0; i < RX_TH_MAX - 1; i++)
    {
        cur = (cur + 1) & (RX_TH_MAX - 1);
        if (s_slots[cur].id >= 0 && s_slots[cur].state != 0)
            break;
    }
    int next = s_mainThread;
    s_current = cur;
    RxThSlot *slot = &s_slots[cur];
    if (slot->id >= 0 && slot->state != 0)
    {
        slot->count++;
        if (slot->priority < slot->count)
        {
            slot->count = 0;
            next = slot->id;
            iChangeThreadPriority(s_mainThread, s_basePriority + s_prioIndex[2]);
        }
    }
    iChangeThreadPriority(next, s_basePriority + s_prioIndex[0]);
    iChangeThreadPriority(s_mainThread, s_basePriority + s_prioIndex[0]);

    if (s_request != 0)
    {
        iReferSemaStatus(s_requestSema, &st);
        if (st.currentCount == 0 && s_request(s_requestArgs) != 0)  // callcheck: rxThVSyncRequest
        {
            iReferSemaStatus(s_yieldSema, &st);
            if (st.numWaitThreads != 0)
            {
                iSignalSema(s_yieldSema);
                s_request = 0;
            }
        }
    }
    if (s_threadCount > 0 || s_request != 0)
        s_alarm = iSetAlarm((unsigned short)s_interval, rxThDispatchAlarm, 0);
}                                   // ExitHandler (EI)

// ──────────────────────────────────────────────────────────────────────
// Nesting: each call adds one to the dispatch semaphore.
// ROR 0x0024a8c8

void rxThDisableDispatch(void)
{
    iSignalSema(s_dispatchSema);
}

// ──────────────────────────────────────────────────────────────────────
// Undo one rxThDisableDispatch; the last one re-arms the alarm.
// ROR 0x0024a958

void rxThEnableDispatch(void)
{
    SemaParam st;
    DIntr();
    iPollSema(s_dispatchSema);
    iReferSemaStatus(s_dispatchSema, &st);
    if (st.currentCount < 1 && s_alarm < 0)
        s_alarm = SetAlarm((unsigned short)s_interval, rxThDispatchAlarm, 0);
    EIntr();
}

/* ---- rx thread slots -------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// The calling thread's slot, -1 when it is not an rx thread.
// ROR 0x0024aae0

int rxThGetIndex(void)
{
    short self = (short)GetThreadId();
    for (int i = 0; i < RX_TH_MAX; i++)
    {
        if (s_slots[i].id == self)
            return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// Free a slot and end its thread (itself: hand the CPU to the main thread
// and exit).
// ROR 0x0024ab40

void rxThDelete(int index)
{
    if (index < 0 || s_slots[index].id < 0)
        return;

    rxThDisableDispatch();
    short id = s_slots[index].id;
    s_slots[index].id = -1;
    s_threadCount--;
    if (id == (short)GetThreadId())
    {
        ChangeThreadPriority(s_mainThread, s_basePriority + s_prioIndex[0]);
        rxThEnableDispatch();
        ExitDeleteThread();
    }
    else
    {
        rxThKillThread(id);
    }
    rxThEnableDispatch();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024ac18

int rxThGetCount(void)
{
    return s_threadCount;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024ac28

void rxThSetPriority(int index, int priority)
{
    if (index >= 0)
        s_slots[index].priority = priority;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024ac48

int rxThGetPriority(int index)
{
    int priority = 0;
    if (index >= 0)
        priority = s_slots[index].priority;
    return priority;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024ac68

void *rxThGetParam(int index)
{
    void *param = 0;
    if (index >= 0)
        param = s_slots[index].param;
    return param;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024ac88

void rxThSetParam(int index, void *param)
{
    if (index >= 0)
        s_slots[index].param = param;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024aca8

int rxThGetState(int index)
{
    int state = 0;
    if (index >= 0)
        state = s_slots[index].state;
    return state;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024acc8

void rxThSetState(int index, short state)
{
    if (index >= 0)
        s_slots[index].state = state;
}

// ──────────────────────────────────────────────────────────────────────
// Start entry on a free slot at the lowest level, not dispatched yet
// (rxThSetState); -1 when all 8 are taken.
// ROR 0x0024ace8

int rxThCreate(void (*entry)(void), void *stack, int stackSize, int priority, void *param)
{
    if (s_threadCount >= RX_TH_MAX)
        return -1;

    // PC: entry takes no argument; StartThread passes 0 in a0 either way.
    int id = rxThStartThread((void (*)(void *))entry, stack, stackSize,
                             s_basePriority + s_prioIndex[2]);
    int index = 0;
    while (index < RX_TH_MAX && s_slots[index].id >= 0)
        index++;
    s_threadCount++;
    RxThSlot *slot = &s_slots[index];
    slot->count = 0;
    slot->priority = priority;
    slot->state = 0;
    slot->param = param;
    slot->id = (short)id;

    DIntr();
    if (s_alarm < 0)
        s_alarm = SetAlarm((unsigned short)s_interval, rxThDispatchAlarm, 0);
    EIntr();
    return index;
}

// ──────────────────────────────────────────────────────────────────────
// Main thread only: what its next rxThYield waits for (func(args) != 0).
// ROR 0x0024ae60

void rxThRequest(RxThRequestFunc func, intptr_t a, intptr_t b, intptr_t c)
{
    if (GetThreadId() != s_mainThread)
        return;

    s_request = func;
    s_requestArgs[0] = a;
    s_requestArgs[1] = b;
    s_requestArgs[2] = c;
    DIntr();
    if (s_alarm < 0)
        s_alarm = SetAlarm((unsigned short)s_interval, rxThDispatchAlarm, 0);
    EIntr();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024af58

void rxThDummy0(void)
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024af60

void rxThDummy1(void)
{
}

/* ---- waits through a yield -------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Request: the vblank count moved past args[0].
// ROR 0x0024af68

int rxThVSyncRequest(intptr_t *args)
{
    return (int)args[0] != rxThSync(-1);
}

// ──────────────────────────────────────────────────────────────────────
// mode 0: let the rx threads run until the next vblank. Returns the
// HSYNCs since the last read.
// ROR 0x0024afa0

int rxThVSync(int mode)
{
    if (mode == 0)
    {
        rxThRequest(rxThVSyncRequest, rxThSync(-1), 0, 0);
        rxThYield();
    }
    return rxThSync(1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024afe8

int rxThGetInterval(void)
{
    return s_interval;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024aff8

void rxThSetInterval(int hsyncs)
{
    if (hsyncs < 4)
        hsyncs = 4;
    if (hsyncs > 0x100)
        hsyncs = 0x100;
    scePrintf("rxThread: HSync Interval:%d\n", hsyncs);
    s_interval = hsyncs;
}

// ──────────────────────────────────────────────────────────────────────
// The priority offsets of the main thread, the worker and the rx threads,
// kept strictly increasing (lower priority).
// ROR 0x0024b048

void rxThSetPrioIndex(int mainIndex, int workerIndex, int lowIndex)
{
    if (!(mainIndex < workerIndex))
        workerIndex = mainIndex + 1;
    if (!(workerIndex < lowIndex))
        lowIndex = workerIndex + 1;
    s_prioIndex[0] = mainIndex;
    s_prioIndex[1] = workerIndex;
    s_prioIndex[2] = lowIndex;
    scePrintf("rxThread: PrioIndex %d %d %d\n", mainIndex, workerIndex, lowIndex);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024b0b0

void rxThInitOnce(void)
{
    rxThInit();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024b0c8

void rxThExitOnce(void)
{
    rxThExit();
}

// ──────────────────────────────────────────────────────────────────────
// Request: path 3 (GIF) is idle.
// ROR 0x0024b0e0

int rxThGsSyncRequest(intptr_t *args)
{
    (void)args;
    return sceGsSyncPath(1, 0) == 0;
}

// ──────────────────────────────────────────────────────────────────────
// mode 0: let the rx threads run until the GS paths are idle.
// ROR 0x0024b108

int rxThGsSyncPath(int mode)
{
    if (mode != 0)
        return sceGsSyncPath(mode, 0);
    rxThRequest(rxThGsSyncRequest, 0, 0, 0);
    rxThYield();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Request: DMA channel args[0] is idle.
// ROR 0x0024b158

int rxThDmaSyncRequest(intptr_t *args)
{
    return sceDmaSync(sceDmaGetChan((int)args[0]), 1, 0) == 0;
}

// ──────────────────────────────────────────────────────────────────────
// mode 0: let the rx threads run until DMA channel chan is idle.
// ROR 0x0024b188

int rxThDmaSync(int chan, int mode)
{
    if (mode != 0)
        return sceDmaSync(sceDmaGetChan(chan), mode, 0);
    rxThRequest(rxThDmaSyncRequest, chan, 0, 0);
    rxThYield();
    return 0;
}

/* ---- vblank and HSYNC counters ---------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// INTC cause 2: count the vblank and latch the field being shown.
// ROR 0x0024b1f8

int rxThVBlankHandler(int cause)
{
    (void)cause;
    s_vblankCount++;
    sceGsGParam *gp = sceGsGetGParam();
    if (gp->sceGsInterMode == 1)
        s_field = (int)(REG_GS_CSR >> 13) & 1;
    else
        s_field = 1;
    return 0;                       // ExitHandler (EI)
}

// ──────────────────────────────────────────────────────────────────────
// First call: the vblank handler, and timer 0 counting HBLANKs.
// ROR 0x0024b278

void rxThVBlankInit(void)
{
    s_vblankUsers++;
    if (s_vblankUsers != 1)
        return;

    s_vblankCount = 0;
    s_lastVBlank = 0;
    s_hsyncCarry = 0;
    s_vblankHandler = AddIntcHandler(2, rxThVBlankHandler, -1);
    EnableIntc(2);
    REG_RCNT0_MODE = 0x8b;          // CUE | GATS | CLKS = HBLANK
    REG_RCNT0_COUNT = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024b300

void rxThVBlankExit(void)
{
    if (s_vblankUsers <= 0)
        return;
    s_vblankUsers--;
    if (s_vblankUsers != 0)
        return;

    DisableIntc(2);
    RemoveIntcHandler(2, s_vblankHandler);
    s_vblankHandler = -1;
}

// ──────────────────────────────────────────────────────────────────────
// HSYNCs since the last read; restarts the count.
// ROR 0x0024b360

int rxThReadHSync(void)
{
    int hsyncs = (int)REG_RCNT0_COUNT;
    s_lastVBlank = s_vblankCount;
    hsyncs += s_hsyncCarry;
    REG_RCNT0_COUNT = 0;
    s_hsyncCarry = 0;
    return hsyncs;
}

// ──────────────────────────────────────────────────────────────────────
// Wait for the next vblank (the kernel's VSync while none was counted).
// ROR 0x0024b398

void rxThWaitVBlank(void)
{
    unsigned count = s_vblankCount;
    if (count == 0)
    {
        VSync();
        return;
    }
    while (s_vblankCount == count)
        RosePan_SleepNs(100000);    // PC: the ROM spins
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0024b3e8

int rxThGetField(void)
{
    return s_field;
}

// ──────────────────────────────────────────────────────────────────────
// mode -1: the vblank count. 0: wait for the next vblank, 1: no wait,
// n > 1: wait until n vblanks have passed since the last read; then the
// HSYNCs since the last read.
// ROR 0x0024b3f8

int rxThSync(int mode)
{
    if (mode == 0)
    {
        rxThWaitVBlank();
        return rxThReadHSync();
    }
    if (mode == 1)
        return rxThReadHSync();
    if (mode > 0)
    {
        while (s_vblankCount < (unsigned)(mode + s_lastVBlank))
            RosePan_SleepNs(100000);    // PC: the ROM spins
        return rxThReadHSync();
    }
    return (int)s_vblankCount;
}
