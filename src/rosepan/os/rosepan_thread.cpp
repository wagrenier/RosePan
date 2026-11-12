#include "rosepan_thread.h"

#include <stdlib.h>

#include <SDL3/SDL_thread.h>
#include <SDL3/SDL_mutex.h>

// The opaque handle types wrap the SDL primitives.  Threads carry a small
// trampoline so the caller's void(*)(void*) entry runs under SDL's SDLCALL
// int(void*) signature, with TLS-based identity handled by the SDK on top.

struct RosePan_Thread
{
    SDL_Thread        *sdl;
    RosePan_ThreadEntry entry;
    void              *arg;
};

struct RosePan_Sema
{
    SDL_Semaphore *sdl;
};

struct RosePan_Mutex
{
    SDL_Mutex *sdl;
};

struct RosePan_Tls
{
    SDL_TLSID id;
};

static int SDLCALL ThreadTrampoline(void *data)
{
    RosePan_Thread *t = (RosePan_Thread *)data;
    if (t != 0 && t->entry != 0)
    {
        t->entry(t->arg);
    }
    return 0;
}

extern "C" {

RosePan_Thread *RosePan_ThreadCreate(RosePan_ThreadEntry entry, void *arg,
                                   const char *name)
{
    if (entry == 0)
    {
        return 0;
    }

    RosePan_Thread *t = (RosePan_Thread *)malloc(sizeof(RosePan_Thread));
    if (t == 0)
    {
        return 0;
    }

    t->entry = entry;
    t->arg   = arg;
    t->sdl   = SDL_CreateThread(ThreadTrampoline, name != 0 ? name : "rosepan-thread", t);
    if (t->sdl == 0)
    {
        free(t);
        return 0;
    }

    return t;
}

void RosePan_ThreadDetach(RosePan_Thread *t)
{
    if (t == 0)
    {
        return;
    }
    if (t->sdl != 0)
    {
        SDL_DetachThread(t->sdl);
    }
    free(t);
}

RosePan_Sema *RosePan_SemaCreate(int initial_count)
{
    RosePan_Sema *s = (RosePan_Sema *)malloc(sizeof(RosePan_Sema));
    if (s == 0)
    {
        return 0;
    }
    s->sdl = SDL_CreateSemaphore((Uint32)(initial_count < 0 ? 0 : initial_count));
    if (s->sdl == 0)
    {
        free(s);
        return 0;
    }
    return s;
}

void RosePan_SemaDestroy(RosePan_Sema *s)
{
    if (s == 0)
    {
        return;
    }
    if (s->sdl != 0)
    {
        SDL_DestroySemaphore(s->sdl);
    }
    free(s);
}

void RosePan_SemaSignal(RosePan_Sema *s)
{
    if (s != 0 && s->sdl != 0)
    {
        SDL_SignalSemaphore(s->sdl);
    }
}

void RosePan_SemaWait(RosePan_Sema *s)
{
    if (s != 0 && s->sdl != 0)
    {
        SDL_WaitSemaphore(s->sdl);
    }
}

int RosePan_SemaTryWait(RosePan_Sema *s)
{
    if (s == 0 || s->sdl == 0)
    {
        return 0;
    }
    return SDL_TryWaitSemaphore(s->sdl) ? 1 : 0;
}

RosePan_Mutex *RosePan_MutexCreate(void)
{
    RosePan_Mutex *m = (RosePan_Mutex *)malloc(sizeof(RosePan_Mutex));
    if (m == 0)
    {
        return 0;
    }
    m->sdl = SDL_CreateMutex();
    if (m->sdl == 0)
    {
        free(m);
        return 0;
    }
    return m;
}

void RosePan_MutexDestroy(RosePan_Mutex *m)
{
    if (m == 0)
    {
        return;
    }
    if (m->sdl != 0)
    {
        SDL_DestroyMutex(m->sdl);
    }
    free(m);
}

void RosePan_MutexLock(RosePan_Mutex *m)
{
    if (m != 0 && m->sdl != 0)
    {
        SDL_LockMutex(m->sdl);
    }
}

void RosePan_MutexUnlock(RosePan_Mutex *m)
{
    if (m != 0 && m->sdl != 0)
    {
        SDL_UnlockMutex(m->sdl);
    }
}

RosePan_Tls *RosePan_TlsCreate(void)
{
    RosePan_Tls *k = (RosePan_Tls *)malloc(sizeof(RosePan_Tls));
    if (k == 0)
    {
        return 0;
    }
    // SDL_TLSID is zero-initialised until first use; SDL lazily assigns it.
    SDL_TLSID zero = {0};
    k->id = zero;
    return k;
}

void RosePan_TlsSet(RosePan_Tls *key, void *value)
{
    if (key != 0)
    {
        SDL_SetTLS(&key->id, value, 0);
    }
}

void *RosePan_TlsGet(RosePan_Tls *key)
{
    if (key == 0)
    {
        return 0;
    }
    return SDL_GetTLS(&key->id);
}

}
