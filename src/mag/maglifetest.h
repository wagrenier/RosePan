/* ==========================================================================
 *  mag/maglifetest.h
 *
 *  A test controller (mag/maglifetest.cpp, pinned 0x130ee8..0x131184 +
 *  the linkonce 0x2853c4..0x285450): MagLifeTest <- MagLifeControlChara
 *  (vtable _vt$11MagLifeTest 0x2e1e70, vptr at +0x14). Bound to an actor,
 *  R2 (pad bit 0x200) plays a fixed chain of the actor's motions -- 0xac,
 *  then 5 (flags 0x40) after 43 frames, then 1 (flags 1, the loop) after 5
 *  more -- and every frame the actor moves by its motion's root movement,
 *  turned by its base heading. Nothing in the ROM constructs one: a
 *  developer's motion check left in the retail build.
 *
 *  EE virtual slots: 1 ~MagLifeTest (linkonce 0x2853c4), 2 live, 3..4
 *  MagLife0's, 5 getName (linkonce 0x285448, NULL), 6..8 MagLife's, 9..15
 *  MagLifeControlChara's, 16 processPad.
 * ======================================================================== */
#ifndef _MAG_MAGLIFETEST_H
#define _MAG_MAGLIFETEST_H

#include <stddef.h>

#include "mag/maglife.h"

struct BedPad;

/* EE 0x88 (the constructor's last field +0x84). layout: host --
 * polymorphic in the ROM (vptr +0x14); EE offsets in the comments. */
struct MagLifeTest : MagLifeControlChara
{
    MagLifeTest();                                                       // ROR 0x0013114c
    /* EE slot 1 (+0x08): restores _vt$8MagLife0, flags & 1 deletes
     * (linkonce). The type-info function __tf11MagLifeTest (0x2853f8) is
     * the host compiler's RTTI. */
    // ROR 0x002853c4
    ~MagLifeTest() override {}

    /* EE slot 2 (+0x10): the motion chain by m_phase, then the root
     * movement while the actor's mode has no argument. */
    void live() override;                                                // ROR 0x00130ee8
    /* EE slot 5 (+0x28): no name (linkonce). */
    // ROR 0x00285448
    const char *getName() { return NULL; }
    /* EE slot 16 (+0x80): R2 pressed starts the chain (m_phase 1). */
    void processPad(BedPad *pad);                                        // ROR 0x00131120

    int       _7c;
    int       m_phase;          // ROR 0x80: 0 idle, 1 start, 2 / 3 waiting to change motion
    float     m_rot_y_aar;      // ROR 0x84: addTrans's third argument; the constructor clears it, nothing sets it
};
static_assert(offsetof(MagLifeTest, m_phase) >= 0x80, "m_phase");
static_assert(offsetof(MagLifeTest, m_rot_y_aar) >= 0x84, "m_rot_y_aar");

#endif /* _MAG_MAGLIFETEST_H */
