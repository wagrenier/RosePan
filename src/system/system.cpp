/* ==========================================================================
 *  system/system.cpp
 *
 *  The globals file before main.cpp (pinned 0x203d94..0x2043dc; the file
 *  name is a guess, RoR has no source string for it): SysTimer, the manager
 *  init / term pair, the host0: restart and the global managers; see
 *  system.h.
 * ======================================================================== */
#include "system/system.h"

#include <stdio.h>

#include "game/gamman.h"
#include "lib/unassigned.h"
#include "mag/magsystem.h"
#include "rx/rxthread.h"
#include "sdk/eekernel.h"
#include "sdk/fileio.h"
#include "sequence/sequence.h"
#include "snd/sndman.h"

// ---- the global managers ----
// The ROM's static initializer 0x2041f8 (entries 0x20439c / 0x2043bc)
// constructs them in this order and destroys them in reverse; on the host
// the definitions' order does the same within this file. The heap
// (utl/utlmisc.cpp) and RxMpeg (snd/rxmpeg.cpp) live in their own files.

MagSystem  g_magSystem;         /* bss 0x6e9840 */
GamMan     g_gamMan;            /* bss 0x7812c0 */
CmnTaskMan g_taskMan;           /* bss 0x7a3d80 */
Sequence   g_sequence;          /* bss 0x7a4440 */
EmyManager g_emyManager;        /* bss 0x7b2140 */
SndMan     g_sndMan;            /* bss 0x7b2580 */

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00203d94

SysTimer::SysTimer()
{
    reset();
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: operator_delete (GCC 2's delete flag; the host's delete frees)
// ROR 0x00203da8

SysTimer::~SysTimer()
{
}

// ──────────────────────────────────────────────────────────────────────
// Stopped, every count zero, the three times at now.
// ROR 0x00203dd0

void SysTimer::reset()
{
    u_int now;

    m_running = 0;
    now = g_gamMan.getSysTime();
    m_paused = 0;
    m_origin = now;
    m_now = now;
    m_prev = now;
    m_elapsed = 0;
    m_stopElapsed = 0;
    m_updates = 0;
    m_prevTime = 0;
    m_time = 0;
    m_lastTime = 0;
}

// ──────────────────────────────────────────────────────────────────────
// A start after a stop adds the time spent stopped to m_paused.
// ROR 0x00203ea8

void SysTimer::start()
{
    if (m_running == 0)
    {
        read();
        m_paused += m_elapsed - m_stopElapsed;
    }
    m_running = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00203efc

void SysTimer::stop()
{
    if (m_running != 0)
    {
        read();
        m_stopElapsed = m_elapsed;
    }
    m_running = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Adds the ms since the last read to m_elapsed (the distance either way:
// the ROM takes the absolute difference).
// ROR 0x00203f38

void SysTimer::read()
{
    u_int now;

    m_prev = m_now;
    now = g_gamMan.getSysTime();
    m_now = now;
    if (m_prev <= now)
        m_elapsed += now - m_prev;
    else
        m_elapsed += m_prev - now;
}

// ──────────────────────────────────────────────────────────────────────
// Once per frame (mainLoop): counts the call, reads, and while running
// moves m_time on (the old one to m_lastTime and m_prevTime).
// ROR 0x00204014

void SysTimer::update()
{
    int last;

    m_updates++;
    read();
    if (m_running != 0)
    {
        last = m_time;
        m_lastTime = last;
        m_time = m_elapsed - m_paused;
        m_prevTime = last;
    }
}

// ──────────────────────────────────────────────────────────────────────
// CmnTaskMan::exec: while running, the last time catches up with the
// current one.
// ROR 0x00204090

void SysTimer::hold()
{
    if (m_running != 0)
        m_lastTime = m_time;
}

// ──────────────────────────────────────────────────────────────────────
// mainInitGame (and GamMan's reset) build the sequence pools, then GamMan.
// ROR 0x00204148

void sysInitManagers(void)
{
    g_sequence.init();
    g_gamMan.init();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00204174

void sysTermManagers(void)
{
    g_sequence.term();
    g_gamMan.term();
}

// ──────────────────────────────────────────────────────────────────────
// A debug restart: waits until host0:main.ee.elf can be opened, syncs the
// rx threads and runs it. Nothing calls it in the ROM.
// ROR 0x002041a0

void sysExecHostElf(void)
{
    char path[256];
    int  fd;

    sprintf(path, "host0:main.ee.elf");
    do
        fd = sceOpen(path, 1);
    while (fd < 0);
    sceClose(fd);
    rxThSync(0);
    LoadExecPS2(path, 0, NULL);
}
