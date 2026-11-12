#ifndef ROSEPAN_HOST_H
#define ROSEPAN_HOST_H

// The host's two threads.  The OS main thread owns the window: SDL's
// subsystems are initialised there, the window and GPU device are created
// there, and it does nothing but pump window messages.  The game -- the EE
// main thread, everything main() used to run -- runs on a thread of its own
// and receives the window's events through a queue.
//
// Why: on Windows, dragging or resizing a window runs a modal message loop
// inside SDL_PumpEvents until the mouse is released.  With the game loop on
// the thread that pumps, the game and its rendering stopped for the length
// of every drag.  Now only the pumping thread is held; the game keeps running
// and presenting (SDL_GPU's swapchain is acquired on the game thread, which
// D3D12 and Vulkan allow; only claiming the window is done on its thread).
//
// ROSEPAN_SINGLE_THREAD=1 runs the game on the main thread as before (for a
// debugger session where one thread is easier to follow).

union SDL_Event;

// Program entry: runs game_main on the game thread (or inline when
// single-threaded) and pumps events on the calling thread until the game exits.
int  RosePan_HostMain(int (*game_main)(void));

// Runs fn(arg) on the main thread and waits for it (inline when called from
// it, or when single-threaded).  For what SDL requires on the window's
// thread: SDL_Init(VIDEO), creating the window, claiming it for the GPU
// device, fullscreen changes, OS dialogs.
void RosePan_HostRunOnMainThread(void (*fn)(void *), void *arg);

// The game thread's SDL_PollEvent: the next event the main thread received.
// Strings of text events stay valid until the next call.
bool RosePan_HostPollEvent(SDL_Event *event);

// SDL_PumpEvents where it is allowed (the main thread pumps by itself when
// the game has its own thread).
void RosePan_HostPumpEvents(void);

#endif /* ROSEPAN_HOST_H */
