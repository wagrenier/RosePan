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

/* One blend entry (the asserts' m_sub[i]): a second motion over the
 * nodes of its mask. EE 0x30, plain words. */
struct DogMotionBlendEntry
{
    int   m_node_mask[4];       // ROR 0x00: bit n = node n takes this motion's pose (getPose, getNodeRate)
    float m_speed;              // ROR 0x10: reset 1/60 (as DogMotionBlend::m_speed); a fade sets 0
    float m_frame;              // ROR 0x14: reset 0
    float m_18;                 // ROR 0x18: reset 1.0; unused by this file
    float m_1c;                 // ROR 0x1c: reset -1.0 (none): a ratio of the length -- the stop frame
                                //   while playing (stepLoop), the held frame while fading, the fade's rate
    int   m_no;                 // ROR 0x20: a motion index, -1 none (DogActor::releaseMotion drops gone ones)
    int   m_state;              // ROR 0x24: 0 playing, 2 fading (setBlendRate)
    int   m_time;               // ROR 0x28: the fade's frames
    int   m_count;              // ROR 0x2c: frames of the fade done (step counts up to m_time)

    /* Inline in the ROM (the constructor and reset repeat it). */
    void clear()
    {
        m_no = -1;
        m_frame = 0.0f;
        m_speed = 0x1.11112Ep-6f; /* 0x3c888897 */
        m_1c = -1.0f;
        m_18 = 1.0f;
        for (int i = 0; i < 4; i++)
            m_node_mask[i] = 0;
    }
};
static_assert(offsetof(DogMotionBlendEntry, m_speed) == 0x10, "m_speed");
static_assert(offsetof(DogMotionBlendEntry, m_1c) == 0x1c, "m_1c");
static_assert(offsetof(DogMotionBlendEntry, m_no) == 0x20, "m_no");
static_assert(offsetof(DogMotionBlendEntry, m_state) == 0x24, "m_state");
static_assert(offsetof(DogMotionBlendEntry, m_time) == 0x28, "m_time");
static_assert(offsetof(DogMotionBlendEntry, m_count) == 0x2c, "m_count");
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
    /* The motion and every entry stopped (speed 0). */
    void stop();                                                         // ROR 0x00108178
    /* The current motion's pose at m_frame into thePosture (0x30-byte
     * records), then each entry's motion replaces the nodes of its mask;
     * the node count (0 without a motion). */
    int  getPose(void *thePosture);                                            // ROR 0x001081b0
    /* The blend rate of the first entry whose mask has node theNode:
     * (m_count + 1) / m_time while it fades, else (or none) 1.0. */
    float getNodeRate(int theNode);                                      // ROR 0x001078dc
    /* m_frame += m_speed * m_0c, clamped: 1 at the end, 2 at the start,
     * 3 zero length or stopped; the entries step too (wrapping). */
    int  step();                                                         // ROR 0x00107e30
    /* The same, wrapping: 1 when it wrapped forward (or is stopped), 2
     * backward; the entries stop at their m_1c ratio. */
    int  stepLoop();                                                     // ROR 0x001079d8
    /* m_frame / the motion's length, in 0..1 (0 without a motion). */
    float getTimeRatio();                                                // ROR 0x00107560
    /* m_frame = the motion's length * r (r in 0..1). 0. */
    int  setTimeRatio(float theRatio);                                          // ROR 0x00107678
    /* Entry theS (0..3) fades to rate theR over theTime frames (m_state
     * 2); rate 0 again at the end of a fade frees it. */
    void setBlendRate(float theRatio, int theNo, int theTime);                // ROR 0x00108428

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
    int   m_14;                 // ROR 0x14: DogActor::setMotionTickTime clears it (with m_10 the tick time)
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
