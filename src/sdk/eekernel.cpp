/* ==========================================================================
 *  eekernel.cpp  (EE kernel / interrupt helpers -- PC-port shim)
 * ======================================================================== */

#define ROSEPAN_EEKERNEL_NO_COMPAT_MACROS
#include "eekernel.h"

#include "../rosepan/os/rosepan_time.h"   // RosePan_Sleep
#include "../rosepan/os/rosepan_thread.h" // threads / semaphores / mutex / TLS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROSEPAN_MAX_SEMAS   128
#define ROSEPAN_MAX_THREADS 64
#define ROSEPAN_MAIN_THREAD_ID 1

typedef struct HostSema
{
    int id;
    RosePan_Sema *sem;
    int maxCount;
} HostSema;

typedef struct HostThread
{
    int id;
    struct ThreadParam param;
    void *arg;
    RosePan_Thread *thread;
    RosePan_Sema *wake;
    int started;
} HostThread;

static HostSema s_semas[ROSEPAN_MAX_SEMAS];
static HostThread s_threads[ROSEPAN_MAX_THREADS];
static RosePan_Tls *s_threadTls;
static RosePan_Mutex *s_loaderSuspendMutex;
static int s_loaderSuspendDepth;
static u_long s_gsImr = 0;

static HostSema *FindSema(int sema_id)
{
    int i;

    for (i = 0; i < ROSEPAN_MAX_SEMAS; i++)
    {
        if (s_semas[i].id == sema_id)
        {
            return &s_semas[i];
        }
    }

    return 0;
}

static HostThread *FindThread(int thread_id)
{
    int i;

    for (i = 0; i < ROSEPAN_MAX_THREADS; i++)
    {
        if (s_threads[i].id == thread_id)
        {
            return &s_threads[i];
        }
    }

    return 0;
}

static RosePan_Tls *GetThreadTls(void)
{
    if (s_threadTls == 0)
    {
        s_threadTls = RosePan_TlsCreate();
    }
    return s_threadTls;
}

static RosePan_Mutex *GetLoaderSuspendMutex(void)
{
    if (s_loaderSuspendMutex == 0)
    {
        s_loaderSuspendMutex = RosePan_MutexCreate();
    }

    return s_loaderSuspendMutex;
}

/* On the PS2, SuspendThread() genuinely freezes the loader thread, so the game
 * thread could safely reorder the load queue underneath it.  Here the loader is
 * a real host thread, and SuspendThread() is only a mutex -- which excludes
 * nothing unless the loader holds it too.  These let the loader claim that same
 * mutex for the span in which it is reading queue entries, so a concurrent
 * FileLoadReq() cannot swap slot contents out from under it. */
void RosePan_LoaderCriticalEnter(void)
{
    RosePan_Mutex *mutex = GetLoaderSuspendMutex();

    if (mutex != 0)
    {
        RosePan_MutexLock(mutex);
    }
}

void RosePan_LoaderCriticalLeave(void)
{
    if (s_loaderSuspendMutex != 0)
    {
        RosePan_MutexUnlock(s_loaderSuspendMutex);
    }
}

static void ThreadPassSuspendGate(void)
{
    RosePan_Mutex *mutex = s_loaderSuspendMutex;

    if (mutex != 0)
    {
        RosePan_MutexLock(mutex);
        RosePan_MutexUnlock(mutex);
    }
}

static void HostThreadMain(void *data)
{
    HostThread *thread = (HostThread *)data;

    RosePan_TlsSet(GetThreadTls(), thread);

    if (thread->param.entry != 0)
    {
        void (*entry)(void *) = (void (*)(void *))thread->param.entry;
        entry(thread->arg);
    }
}

extern "C" {

int RosePan_CreateSema(struct SemaParam *param)
{
    int i;
    int initCount = param != 0 ? param->initCount : 0;

    for (i = 0; i < ROSEPAN_MAX_SEMAS; i++)
    {
        if (s_semas[i].id == 0)
        {
            s_semas[i].sem = RosePan_SemaCreate(initCount);
            if (s_semas[i].sem == 0)
            {
                return -1;
            }

            s_semas[i].id = i + 1;
            s_semas[i].maxCount = param != 0 ? param->maxCount : 0;
            return s_semas[i].id;
        }
    }

    return -1;
}

int RosePan_DeleteSema(int sema_id)
{
    HostSema *sema = FindSema(sema_id);

    if (sema == 0)
    {
        return -1;
    }

    if (sema->sem != 0)
    {
        RosePan_SemaDestroy(sema->sem);
    }

    memset(sema, 0, sizeof(*sema));
    return 0;
}

int RosePan_SignalSema(int sema_id)
{
    HostSema *sema = FindSema(sema_id);

    if (sema == 0 || sema->sem == 0)
    {
        return -1;
    }

    RosePan_SemaSignal(sema->sem);
    return 0;
}

int RosePan_iSignalSema(int sema_id)
{
    return RosePan_SignalSema(sema_id);
}

int RosePan_WaitSema(int sema_id)
{
    HostSema *sema = FindSema(sema_id);

    if (sema == 0 || sema->sem == 0)
    {
        return -1;
    }

    RosePan_SemaWait(sema->sem);
    ThreadPassSuspendGate();
    return 0;
}

int RosePan_PollSema(int sema_id)
{
    HostSema *sema = FindSema(sema_id);

    if (sema == 0 || sema->sem == 0)
    {
        return -1;
    }

    return RosePan_SemaTryWait(sema->sem) ? 0 : -1;
}

int RosePan_CreateThread(struct ThreadParam *param)
{
    int i;

    if (param == 0 || param->entry == 0)
    {
        return -1;
    }

    for (i = 0; i < ROSEPAN_MAX_THREADS; i++)
    {
        if (s_threads[i].id == 0)
        {
            s_threads[i].id = i + 2;
            s_threads[i].param = *param;
            s_threads[i].param.status = 0;
            s_threads[i].param.currentPriority = param->initPriority;
            s_threads[i].arg = 0;
            s_threads[i].thread = 0;
            s_threads[i].wake = RosePan_SemaCreate(0);
            s_threads[i].started = 0;

            if (s_threads[i].wake == 0)
            {
                memset(&s_threads[i], 0, sizeof(s_threads[i]));
                return -1;
            }

            return s_threads[i].id;
        }
    }

    return -1;
}

int RosePan_StartThread(int thread_id, void *arg)
{
    HostThread *thread = FindThread(thread_id);

    if (thread == 0 || thread->started)
    {
        return -1;
    }

    thread->arg = arg;
    thread->thread = RosePan_ThreadCreate(HostThreadMain, thread, "ee-thread");
    if (thread->thread == 0)
    {
        return -1;
    }

    thread->started = 1;
    thread->param.status = 1;
    return 0;
}

/* PORT: there is no way to pre-empt a host thread from outside, so this marks
 * the slot dormant and leaves the thread to return on its own.  playPssEnd() is
 * the only caller and videoDecMain() has already returned by then -- it exits
 * the moment the decoder reports end of stream. */
int RosePan_TerminateThread(int thread_id)
{
    HostThread *thread = FindThread(thread_id);

    if (thread == 0)
    {
        return -1;
    }

    thread->param.status = 0;
    return 0;
}

int RosePan_DeleteThread(int thread_id)
{
    HostThread *thread = FindThread(thread_id);

    if (thread == 0)
    {
        return -1;
    }

    if (thread->thread != 0)
    {
        RosePan_ThreadDetach(thread->thread);
    }

    if (thread->wake != 0)
    {
        RosePan_SemaDestroy(thread->wake);
    }

    memset(thread, 0, sizeof(*thread));
    return 0;
}

int RosePan_SuspendThread(int thread_id)
{
    RosePan_Mutex *mutex;

    if (FindThread(thread_id) == 0)
    {
        return -1;
    }

    mutex = GetLoaderSuspendMutex();
    if (mutex == 0)
    {
        return -1;
    }

    RosePan_MutexLock(mutex);
    s_loaderSuspendDepth++;
    return 0;
}

int RosePan_ResumeThread(int thread_id)
{
    if (FindThread(thread_id) == 0 || s_loaderSuspendMutex == 0)
    {
        return -1;
    }

    if (s_loaderSuspendDepth > 0)
    {
        s_loaderSuspendDepth--;
        RosePan_MutexUnlock(s_loaderSuspendMutex);
    }

    return 0;
}

int RosePan_GetThreadId(void)
{
    HostThread *thread = (HostThread *)RosePan_TlsGet(GetThreadTls());
    return thread != 0 ? thread->id : ROSEPAN_MAIN_THREAD_ID;
}

/* The main thread is not a HostThread, but the loader wakes it
 * (FileLoadCancelAll, FileLoadCancelWait*, FileLoadReqEEWait sleep on it), so it
 * gets a wakeup counter of its own, as every EE thread has one. */
static RosePan_Sema *GetMainWake(void)
{
    static RosePan_Sema *wake = RosePan_SemaCreate(0);
    return wake;
}

int RosePan_SleepThread(void)
{
    HostThread *thread = (HostThread *)RosePan_TlsGet(GetThreadTls());

    if (thread == 0)
    {
        RosePan_Sema *wake = GetMainWake();

        if (wake == 0)
        {
            RosePan_Sleep(1);
            return 0;
        }
        RosePan_SemaWait(wake);
        ThreadPassSuspendGate();
        return 0;
    }

    if (thread->wake == 0)
    {
        RosePan_Sleep(1);
        return 0;
    }

    RosePan_SemaWait(thread->wake);
    ThreadPassSuspendGate();
    return 0;
}

int RosePan_WakeupThread(int thread_id)
{
    HostThread *thread = FindThread(thread_id);

    if (thread == 0 && thread_id == ROSEPAN_MAIN_THREAD_ID)
    {
        RosePan_Sema *wake = GetMainWake();

        if (wake == 0)
        {
            return -1;
        }
        RosePan_SemaSignal(wake);
        return 0;
    }

    if (thread == 0 || thread->wake == 0)
    {
        return -1;
    }

    RosePan_SemaSignal(thread->wake);
    return 0;
}

int RosePan_ChangeThreadPriority(int thread_id, int priority)
{
    HostThread *thread = FindThread(thread_id);

    if (thread == 0)
    {
        return -1;
    }

    thread->param.currentPriority = priority;
    return 0;
}

int RosePan_ReferThreadStatus(int thread_id, struct ThreadParam *info)
{
    HostThread *thread;

    if (info != 0)
    {
        memset(info, 0, sizeof(*info));
        thread = FindThread(thread_id);
        if (thread != 0)
        {
            *info = thread->param;
        }
        else if (thread_id == ROSEPAN_MAIN_THREAD_ID)
        {
            info->status = 1;
            info->initPriority = 32;
            info->currentPriority = 32;
        }
    }

    return 0;
}

void RosePan_FlushCache(int mode)
{
    (void)mode;
}

int RosePan_SetDebugHandler(int cause, RosePanDebugHandler handler)
{
    (void)cause;
    (void)handler;
    return 0;
}

int RosePan_AddDmacHandler(int channel, RosePanDmacHandler handler, int next)
{
    (void)channel;
    (void)handler;
    (void)next;
    return 0;
}

int RosePan_AddDmacHandler2(int channel, RosePanDmacHandler2 handler, int next, void *arg)
{
    (void)channel;
    (void)handler;
    (void)next;
    (void)arg;
    return 0;
}

int RosePan_EnableDmac(int channel)
{
    (void)channel;
    return 0;
}

/* --------------------------------------------------------------------------
 *  INTC handlers
 *
 *  Real registry, no source of interrupts.  See the note in eekernel.h: the
 *  one registration in the tree is playpss.c's PAL frame-rate divider on cause
 *  2 (vblank start), and nothing calls RosePan_IntcRaise() because the port's
 *  frame loop has no vblank interrupt.  Keeping the registry honest means a
 *  future vblank hook is one call away rather than a rewrite.
 * ------------------------------------------------------------------------ */

#define ROSEPAN_MAX_INTC 16

typedef struct
{
    int               id;
    int               cause;
    RosePanIntcHandler handler;
} HostIntc;

static HostIntc s_intc[ROSEPAN_MAX_INTC];
static int      s_intc_enabled[32];
static int      s_intc_next_id = 1;

int RosePan_AddIntcHandler(int cause, RosePanIntcHandler handler, int next)
{
    int i;

    (void)next;

    if (handler == 0)
    {
        return -1;
    }

    for (i = 0; i < ROSEPAN_MAX_INTC; i++)
    {
        if (s_intc[i].id == 0)
        {
            s_intc[i].id      = s_intc_next_id++;
            s_intc[i].cause   = cause;
            s_intc[i].handler = handler;
            return s_intc[i].id;
        }
    }

    return -1;
}

int RosePan_RemoveIntcHandler(int cause, int handler_id)
{
    int i;

    for (i = 0; i < ROSEPAN_MAX_INTC; i++)
    {
        if (s_intc[i].id == handler_id && s_intc[i].cause == cause)
        {
            memset(&s_intc[i], 0, sizeof(s_intc[i]));
            return 0;
        }
    }

    return -1;
}

int RosePan_EnableIntc(int cause)
{
    if (cause >= 0 && cause < 32)
    {
        s_intc_enabled[cause] = 1;
    }

    return 0;
}

int RosePan_DisableIntc(int cause)
{
    if (cause >= 0 && cause < 32)
    {
        s_intc_enabled[cause] = 0;
    }

    return 0;
}

void RosePan_IntcRaise(int cause)
{
    int i;

    if (cause < 0 || cause >= 32 || s_intc_enabled[cause] == 0)
    {
        return;
    }

    for (i = 0; i < ROSEPAN_MAX_INTC; i++)
    {
        if (s_intc[i].id != 0 && s_intc[i].cause == cause)
        {
            s_intc[i].handler(cause);
        }
    }
}

int RosePan_sceDevVif0Reset(void)
{
    return 0;
}

int RosePan_sceDevVu0Reset(void)
{
    return 0;
}

int RosePan_sceDevVif1Reset(void)
{
    return 0;
}

int RosePan_sceDevVu1Reset(void)
{
    return 0;
}

int RosePan_GetMemorySize(void)
{
    return 0x2000000;
}

/* SDL mutexes are recursive, which is what lets DIntr ... EIntr nest. */
static RosePan_Mutex *s_intrMutex;

int RosePan_DIntr(void)
{
    if (s_intrMutex == 0)
    {
        s_intrMutex = RosePan_MutexCreate();
    }
    RosePan_MutexLock(s_intrMutex);
    return 1;
}

int RosePan_EIntr(void)
{
    RosePan_MutexUnlock(s_intrMutex);
    return 1;
}

void RosePan_LoadExecPS2(const char *name, int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("LoadExecPS2 '%s': cannot run another ELF on the host, exiting\n",
           name != 0 ? name : "");
    exit(0);
}

u_long RosePan_sceGsGetIMR(void)
{
    return s_gsImr;
}

u_long RosePan_sceGsPutIMR(u_long imr)
{
    u_long old = s_gsImr;
    s_gsImr = imr;
    return old;
}

}
