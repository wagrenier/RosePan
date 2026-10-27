/* ==========================================================================
 *  dog/dogmotionblend.h
 *
 *  DogMotionBlend (dog/dogmotionblend.cpp): the motion state a DogActor
 *  keeps twice, at +0x1e4 (m_motion, the current one) and +0x2c4 (the one
 *  before; savePosture copies it): the motion number, speed and frame, a
 *  view on the actor's motion records (m_motion / m_max_motion, the names
 *  in this header's asserts) and four blend entries. EE 0xe0 (the two sit
 *  0xe0 apart).
 * ======================================================================== */
#ifndef _DOG_DOGMOTIONBLEND_H
#define _DOG_DOGMOTIONBLEND_H

#include <stddef.h>

#include "common/ror_assert.h"
#include "sdk/eetypes.h"

struct DogMotionData;

/* One blend entry. EE 0x30, plain words. Roles beyond what the reset
 * writes are unverified. */
struct DogMotionBlendEntry
{
    int   m_00[4];              // ROR 0x00: reset 0
    float m_speed;              // ROR 0x10: reset 1/60 (as DogMotionBlend::m_speed)
    float m_frame;              // ROR 0x14: reset 0
    float m_18;                 // ROR 0x18: reset 1.0
    float m_1c;                 // ROR 0x1c: reset -1.0
    int   m_no;                 // ROR 0x20: a motion index, -1 none (DogActor::releaseMotion drops gone ones)
    u_char _24[0x30 - 0x24];

    /* Inline in the ROM (the constructor and reset repeat it). */
    void clear()
    {
        m_no = -1;
        m_frame = 0.0f;
        m_speed = 0x1.11112Ep-6f; /* 0x3c888897 */
        m_1c = -1.0f;
        m_18 = 1.0f;
        for (int i = 0; i < 4; i++)
            m_00[i] = 0;
    }
};
static_assert(offsetof(DogMotionBlendEntry, m_speed) == 0x10, "m_speed");
static_assert(offsetof(DogMotionBlendEntry, m_no) == 0x20, "m_no");
static_assert(sizeof(DogMotionBlendEntry) == 0x30, "DogMotionBlendEntry is 0x30 bytes on the EE");

#define DOG_MOTION_BLEND_MAX 4

/* EE 0xe0. layout: host -- m_motion is a pointer; EE offsets in the
 * comments. */
struct DogMotionBlend
{
    /* No motion, the entries cleared; the view (m_motion) is the owner's. */
    DogMotionBlend();                                                    // ROR 0x00108588
    /* The current motion number (m_no). */
    int getNo();                                                         // ROR 0x001081a8
    /* Without records: no motion, every entry cleared. With them: the
     * current motion and each entry whose record has no i3m any more. */
    void reset();                                                        // ROR 0x0010737c

    /* The record of motion n (the header's lines 122 / 123: two inlines
     * with the same checks; DogActor::setMotion uses both). Defined in
     * dog/dogactor.h, after DogMotionData. */
    inline DogMotionData &getData(int n);
    /* Entry n's motion number (the header's line 257). */
    int getBlendNo(int n)
    {
        ROR_ASSERT(n>=0 && n<=4); /* 257 */
        return m_blend[n].m_no;
    }

    int   m_no;                 // ROR 0x00: the motion index (DogActor::setMotion), -1 none
    float m_speed;              // ROR 0x04: frames per frame (MagScene::setActorMotionSpeed: fps / 60); 0 = stopped
    float m_frame;              // ROR 0x08: setMotion clears it (or the motion's length with flag 0x80)
    float m_0c;                 // ROR 0x0c: the constructor's 1.0
    int   m_10;                 // ROR 0x10: the constructor's -1
    u_char _14[0x18 - 0x14];
    int   m_max_motion;         // ROR 0x18: the owner's record count (DogActor::setMaxMotion)
    DogMotionData *m_motion;    // ROR 0x1c: the owner's records
    DogMotionBlendEntry m_blend[DOG_MOTION_BLEND_MAX]; // ROR 0x20
};
static_assert(offsetof(DogMotionBlend, m_speed) == 0x04, "m_speed");
static_assert(offsetof(DogMotionBlend, m_frame) == 0x08, "m_frame");
static_assert(offsetof(DogMotionBlend, m_max_motion) == 0x18, "m_max_motion");
static_assert(offsetof(DogMotionBlend, m_motion) >= 0x1c, "m_motion");
static_assert(offsetof(DogMotionBlend, m_blend) >= 0x20, "m_blend");
static_assert(sizeof(DogMotionBlend) >= 0xe0, "DogMotionBlend is 0xe0 bytes on the EE");

#endif /* _DOG_DOGMOTIONBLEND_H */
