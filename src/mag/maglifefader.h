/* ==========================================================================
 *  mag/maglifefader.h
 *
 *  The screen fader (mag/maglifefader.cpp, pinned 0x12f008..0x12f298):
 *  MagLifeFader <- MagLife (vtable _vt$12MagLifeFader 0x2e1ca8, vptr at
 *  +0x14), the scene's +0x41850 for fade kinds 0 (to black) and 4 (to
 *  0xff). A full-screen sprite of m_color whose alpha follows m_state:
 *    1      opaque (0x80)
 *    2, 3   fade in: alpha 0x80 -> 0 over m_time frames, then state 4
 *    4, 5   nothing drawn (4: a fade in has ended, or stop())
 *    6, 7   fade out: alpha 0 -> 0x80 over m_time - 2 frames, state 8 after m_time
 *    8      opaque (a fade out has ended)
 *  MagScene::getFadeState maps these to 0..6; MagScene::update and the
 *  title's logo wait on them.
 *
 *  EE virtual slots: 1 ~MagLifeFader (linkonce 0x284fe8), 2 live,
 *  3 MagLife0::attached, 4 MagLife0::getLevel, 5 getName (linkonce
 *  0x284fe0, NULL), 6 render, 7 MagLife::setHide, 8 MagLife::getHide.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEFADER_H
#define _MAG_MAGLIFEFADER_H

#include <stddef.h>

#include "mag/maglife.h"

struct I3dLayer;
struct MagFadeParam;

/* EE 0x3c bytes. layout: host -- polymorphic (EE vptr at +0x14). */
struct MagLifeFader : MagLife
{
    MagLifeFader();                                                      // ROR 0x0012f008
    /* EE slot 2 (+0x10): counts the fade's frames, ends it at m_time. */
    void live() override;                                                // ROR 0x0012f094
    /* EE slot 5 (+0x28), linkonce. */
    const char *getName() { return NULL; }                               // ROR 0x00284fe0
    /* EE slot 6 (+0x30): the full-screen sprite (nothing in states 4, 5). */
    void render(I3dLayer *layer) override;                               // ROR 0x0012f128

    /* State p->m_mode over p->m_time frames, from frame 0. */
    void start(const MagFadeParam *p);                                   // ROR 0x0012f060
    /* State 4: nothing drawn. */
    void stop();                                                         // ROR 0x0012f080

    u_char _28[0x30 - 0x28];
    int    m_frame;             // ROR 0x30: frames since start (live counts it in states 2, 3, 6, 7)
    int    m_time;              // ROR 0x34: the fade's length in frames (start)
    int    m_color;             // ROR 0x38: grey level 0..0xff: 0 for fade kind 0, 0xff for kind 4
};
static_assert(offsetof(MagLifeFader, m_frame) >= 0x30, "m_frame");
static_assert(offsetof(MagLifeFader, m_time) >= 0x34, "m_time");
static_assert(offsetof(MagLifeFader, m_color) >= 0x38, "m_color");

#endif /* _MAG_MAGLIFEFADER_H */
