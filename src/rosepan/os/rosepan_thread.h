#ifndef ROSEPAN_THREAD_H
#define ROSEPAN_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

// Host threading/synchronisation wrapper.  Confines SDL threads/mutex/sema/TLS
// to the rosepan layer so the SDK's EE-kernel shim (and libgraph's vblank
// thread) don't call SDL directly.  These are neutral primitives; the SDK keeps
// its own sce*/EE naming, slot tables and scheduling policy on top.

typedef struct RosePan_Thread RosePan_Thread;
typedef struct RosePan_Sema   RosePan_Sema;
typedef struct RosePan_Mutex  RosePan_Mutex;
typedef struct RosePan_Tls    RosePan_Tls;

// Threads.  entry(arg) runs on the new thread; the handle is owned by the
// caller until RosePan_ThreadDetach.  Returns NULL on failure.
typedef void (*RosePan_ThreadEntry)(void *arg);
RosePan_Thread *RosePan_ThreadCreate(RosePan_ThreadEntry entry, void *arg,
                                   const char *name);
void           RosePan_ThreadDetach(RosePan_Thread *t);

// Counting semaphores.
RosePan_Sema *RosePan_SemaCreate(int initial_count);
void         RosePan_SemaDestroy(RosePan_Sema *s);
void         RosePan_SemaSignal(RosePan_Sema *s);
void         RosePan_SemaWait(RosePan_Sema *s);
int          RosePan_SemaTryWait(RosePan_Sema *s); // 1 if acquired, 0 otherwise

// Mutexes.
RosePan_Mutex *RosePan_MutexCreate(void);
void          RosePan_MutexDestroy(RosePan_Mutex *m);
void          RosePan_MutexLock(RosePan_Mutex *m);
void          RosePan_MutexUnlock(RosePan_Mutex *m);

// Thread-local storage of a single void* per key.
RosePan_Tls *RosePan_TlsCreate(void);
void        RosePan_TlsSet(RosePan_Tls *key, void *value);
void       *RosePan_TlsGet(RosePan_Tls *key);

#ifdef __cplusplus
}
#endif

#endif /* ROSEPAN_THREAD_H */
