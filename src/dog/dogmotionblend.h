/* ==========================================================================
 *  dog/dogmotionblend.h
 *
 *  DogMotionBlend (dog/dogmotionblend.cpp): the motion state a DogActor
 *  keeps at +0x1e4 (its m_motion; the header's asserts name it). Minimal:
 *  the three words DogActor::setMotion writes and the scene reads.
 * ======================================================================== */
#ifndef _DOG_DOGMOTIONBLEND_H
#define _DOG_DOGMOTIONBLEND_H

#include <stddef.h>

/* EE at least 0xc; plain words. */
struct DogMotionBlend
{
    /* The current motion number (m_no). */
    int getNo();                                                         // ROR 0x001081a8

    int   m_no;                 // ROR 0x00: the motion index (DogActor::setMotion)
    float m_speed;              // ROR 0x04: frames per frame (MagScene::setActorMotionSpeed: fps / 60); 0 = stopped
    float m_frame;              // ROR 0x08: setMotion clears it (or the motion's length with flag 0x80)
};
static_assert(offsetof(DogMotionBlend, m_speed) == 0x04, "m_speed");
static_assert(offsetof(DogMotionBlend, m_frame) == 0x08, "m_frame");

#endif /* _DOG_DOGMOTIONBLEND_H */
