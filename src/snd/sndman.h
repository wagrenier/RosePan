/* ==========================================================================
 *  snd/sndman.h
 *
 *  The game's sound manager, one global at bss 0x7b2580 (constructed by
 *  0x17f0d0 in the static initializer 0x2041f8, initialised by main through
 *  SndMan::init, which starts the rx sound middleware rxSndInit). The class
 *  name follows snd/sndman.cpp, whose anchor 0x180230 comes right after
 *  these functions -- unverified. Minimal: only what main reaches.
 * ======================================================================== */
#ifndef _SND_SNDMAN_H
#define _SND_SNDMAN_H

#include <stddef.h>

/* EE size at least 0x1b0c (the constructor writes +0x1b08).
 * layout: host -- only the first two members are placed. */
struct SndMan
{
    void init();                                                         // ROR 0x0017f164
    void update();                                                       // ROR 0x0017f200

    int  m_state;               // ROR 0x00: 0 after construction, 1 after init
    char m_bootName[0x20];      // ROR 0x04: main copies argv[0] here (the constructor clears +0x24 next)
};
static_assert(offsetof(SndMan, m_bootName) == 0x04, "m_bootName");

extern SndMan g_sndMan;         /* bss 0x7b2580 */

#endif /* _SND_SNDMAN_H */
