/* ==========================================================================
 *  system/system.h
 *
 *  The globals file before main.cpp (system/system.cpp, pinned
 *  0x203d94..0x2043dc; RoR has no source string for it, the name is a
 *  guess): the frame timer SysTimer, the manager init / term pair main and
 *  GamMan call, a debug restart from host0:, and the global managers its
 *  static initializer 0x2041f8 constructs, in this order: the game heap
 *  (BedMemory 0x6e9800, defined in utl/utlmisc.cpp), MagSystem, GamMan,
 *  CmnTaskMan, Sequence, EmyManager, SndMan, RxMpeg (defined in
 *  snd/rxmpeg.cpp).
 * ======================================================================== */
#ifndef _SYSTEM_SYSTEM_H
#define _SYSTEM_SYSTEM_H

#include <stddef.h>

#include "sdk/scetypes.h"

/* A millisecond timer over GamMan::getSysTime (the frame counter m_vcount
 * x 1000 / 25). One instance: the task manager's +0x40, which main starts
 * and updates once per frame and CmnTaskMan::exec holds. EE 0x2c bytes,
 * every member a word: the EE layout is kept. */
struct SysTimer
{
    SysTimer();                                                          // ROR 0x00203d94
    ~SysTimer();                                                         // ROR 0x00203da8

    void reset();                                                        // ROR 0x00203dd0
    void start();                                                        // ROR 0x00203ea8
    void stop();                                                         // ROR 0x00203efc
    void read();                                                         // ROR 0x00203f38
    void update();                                                       // ROR 0x00204014
    void hold();                                                         // ROR 0x00204090

    int   m_running;            // ROR 0x00: start sets it, stop clears it
    u_int m_now;                // ROR 0x04: the last getSysTime read
    u_int m_prev;               // ROR 0x08: the read before
    int   m_elapsed;            // ROR 0x0c: the ms summed over every read
    u_int m_origin;             // ROR 0x10: the time reset saw (not read here)
    int   m_stopElapsed;        // ROR 0x14: m_elapsed when stop ran
    int   m_updates;            // ROR 0x18: update calls
    int   m_prevTime;           // ROR 0x1c: m_time before the last update
    int   m_time;               // ROR 0x20: running time (m_elapsed - m_paused)
    int   m_lastTime;           // ROR 0x24: m_time before the last update (hold: m_time)
    int   m_paused;             // ROR 0x28: the ms spent stopped
};
static_assert(sizeof(SysTimer) == 0x2c, "SysTimer EE size");
static_assert(offsetof(SysTimer, m_now) == 0x04, "SysTimer m_now");
static_assert(offsetof(SysTimer, m_prev) == 0x08, "SysTimer m_prev");
static_assert(offsetof(SysTimer, m_elapsed) == 0x0c, "SysTimer m_elapsed");
static_assert(offsetof(SysTimer, m_origin) == 0x10, "SysTimer m_origin");
static_assert(offsetof(SysTimer, m_stopElapsed) == 0x14, "SysTimer m_stopElapsed");
static_assert(offsetof(SysTimer, m_updates) == 0x18, "SysTimer m_updates");
static_assert(offsetof(SysTimer, m_prevTime) == 0x1c, "SysTimer m_prevTime");
static_assert(offsetof(SysTimer, m_time) == 0x20, "SysTimer m_time");
static_assert(offsetof(SysTimer, m_lastTime) == 0x24, "SysTimer m_lastTime");
static_assert(offsetof(SysTimer, m_paused) == 0x28, "SysTimer m_paused");

void sysInitManagers(void);                                              // ROR 0x00204148
void sysTermManagers(void);                                              // ROR 0x00204174
void sysExecHostElf(void);                                               // ROR 0x002041a0

#endif /* _SYSTEM_SYSTEM_H */
