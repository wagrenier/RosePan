#include "rosepan_host.h"

#include <cstdlib>
#include <deque>
#include <mutex>
#include <string>

#include <SDL3/SDL.h>

// See rosepan_host.h for the split.  Everything here is either main-thread
// (HostMain's loop, Enqueue) or game-thread (PollEvent) code; the queue and
// the flags are the only state the two share.

namespace {

// An event as the game thread receives it.  SDL owns the strings of text and
// drop events only until the main thread's next pump, so the text the game
// can read is copied here; the pointers nobody in the port reads (drop data,
// IME candidates, clipboard mime types) are cleared rather than copied.
struct QueuedEvent
{
    SDL_Event   event;
    std::string text;
};

std::mutex              g_queue_lock;
std::deque<QueuedEvent> g_queue;

bool             g_threaded = false;
SDL_AtomicInt    g_game_done;
int              g_game_result = 0;
int            (*g_game_main)(void) = nullptr;

// Long enough that the game can finish what it is doing and leave through
// its own std::exit; short enough that a game stuck in a wait cannot keep a
// closed window's process alive.
const Uint64 kQuitGraceMs = 3000;

// The game's stack.  The EE main thread used to be the process's main thread
// (1 MB on Windows); the game's frames are not small, so give it room.
const Sint64 kGameStackBytes = 8 * 1024 * 1024;

void Enqueue(const SDL_Event &event)
{
    QueuedEvent queued;
    queued.event = event;

    switch (event.type)
    {
    case SDL_EVENT_TEXT_INPUT:
        queued.text = (event.text.text != nullptr) ? event.text.text : "";
        break;
    case SDL_EVENT_TEXT_EDITING:
        queued.text = (event.edit.text != nullptr) ? event.edit.text : "";
        break;
    case SDL_EVENT_TEXT_EDITING_CANDIDATES:
        queued.event.edit_candidates.candidates = nullptr;
        queued.event.edit_candidates.num_candidates = 0;
        break;
    case SDL_EVENT_DROP_BEGIN:
    case SDL_EVENT_DROP_FILE:
    case SDL_EVENT_DROP_TEXT:
    case SDL_EVENT_DROP_COMPLETE:
    case SDL_EVENT_DROP_POSITION:
        queued.event.drop.source = nullptr;
        queued.event.drop.data = nullptr;
        break;
    case SDL_EVENT_CLIPBOARD_UPDATE:
        queued.event.clipboard.mime_types = nullptr;
        queued.event.clipboard.num_mime_types = 0;
        break;
    default:
        break;
    }

    std::lock_guard<std::mutex> guard(g_queue_lock);
    g_queue.push_back(std::move(queued));
}

int SDLCALL GameThread(void *)
{
    g_game_result = g_game_main();
    SDL_SetAtomicInt(&g_game_done, 1);
    /* Wake the main thread's wait so it sees the flag now. */
    SDL_Event wake;
    SDL_zero(wake);
    wake.type = SDL_EVENT_USER;
    SDL_PushEvent(&wake);
    return g_game_result;
}

}  // namespace

int RosePan_HostMain(int (*game_main)(void))
{
    const char *single = std::getenv("ROSEPAN_SINGLE_THREAD");
    if (single != nullptr && single[0] != '\0' && single[0] != '0')
    {
        return game_main();
    }

    /* The first SDL_Init marks this as SDL's main thread.  Gamepads are
     * brought up here too: SDL's device notifications arrive as window
     * messages of the thread that initialised them, and only this one pumps.
     * Audio and video are only warmed up; their users (iop_voice.cpp, the
     * renderer) still init them, which just counts up a reference. */
    if (!SDL_Init(SDL_INIT_EVENTS))
    {
        SDL_Log("RosePan host: SDL_Init(EVENTS) failed (%s); running single-threaded",
                SDL_GetError());
        return game_main();
    }
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
        SDL_Log("RosePan host: SDL_Init(VIDEO): %s", SDL_GetError());
    }
    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
    {
        SDL_Log("RosePan host: SDL_Init(GAMEPAD): %s", SDL_GetError());
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        SDL_Log("RosePan host: SDL_Init(AUDIO): %s", SDL_GetError());
    }

    g_game_main = game_main;
    SDL_SetAtomicInt(&g_game_done, 0);
    g_threaded = true;

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_THREAD_CREATE_ENTRY_FUNCTION_POINTER,
                           (void *)GameThread);
    SDL_SetStringProperty(props, SDL_PROP_THREAD_CREATE_NAME_STRING, "RosePanGame");
    SDL_SetNumberProperty(props, SDL_PROP_THREAD_CREATE_STACKSIZE_NUMBER,
                          kGameStackBytes);
    SDL_Thread *game = SDL_CreateThreadWithProperties(props);
    SDL_DestroyProperties(props);
    if (game == nullptr)
    {
        SDL_Log("RosePan host: game thread not created (%s); running single-threaded",
                SDL_GetError());
        g_threaded = false;
        return game_main();
    }
    SDL_DetachThread(game);

    /* The pump.  SDL_WaitEventTimeout also runs what the game thread queued
     * through SDL_RunOnMainThread (it wakes the wait).  A window drag holds
     * this loop inside Windows' modal loop; that is the point -- the game
     * thread is not held with it. */
    Uint64 quit_at = 0;
    while (SDL_GetAtomicInt(&g_game_done) == 0)
    {
        SDL_Event event;
        if (SDL_WaitEventTimeout(&event, 50))
        {
            if (event.type == SDL_EVENT_USER)
            {
                continue;
            }
            if (event.type == SDL_EVENT_QUIT && quit_at == 0)
            {
                quit_at = SDL_GetTicks();
            }
            Enqueue(event);
        }

        /* The game thread exits the process when it reads the quit event; if
         * it is stuck somewhere that does not read events, leave without it. */
        if (quit_at != 0 && SDL_GetTicks() - quit_at > kQuitGraceMs)
        {
            SDL_Log("RosePan host: the game did not exit after quit; exiting");
            std::_Exit(0);
        }
    }

    return g_game_result;
}

void RosePan_HostRunOnMainThread(void (*fn)(void *), void *arg)
{
    if (!g_threaded)
    {
        fn(arg);
        return;
    }
    /* Inline when already on the main thread (SDL checks). */
    if (!SDL_RunOnMainThread(fn, arg, true))
    {
        SDL_Log("RosePan host: SDL_RunOnMainThread failed (%s)", SDL_GetError());
    }
}

bool RosePan_HostPollEvent(SDL_Event *event)
{
    if (!g_threaded)
    {
        return SDL_PollEvent(event);
    }

    /* Holds the strings of the event handed out last, until the next call. */
    static QueuedEvent current;

    {
        std::lock_guard<std::mutex> guard(g_queue_lock);
        if (g_queue.empty())
        {
            return false;
        }
        current = std::move(g_queue.front());
        g_queue.pop_front();
    }

    if (current.event.type == SDL_EVENT_TEXT_INPUT)
    {
        current.event.text.text = current.text.c_str();
    }
    else if (current.event.type == SDL_EVENT_TEXT_EDITING)
    {
        current.event.edit.text = current.text.c_str();
    }
    *event = current.event;
    return true;
}

void RosePan_HostPumpEvents(void)
{
    if (!g_threaded)
    {
        SDL_PumpEvents();
    }
}
