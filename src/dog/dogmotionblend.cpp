// dog/dogmotionblend.cpp -- DogMotionBlend, the motion state of a DogActor.
// Written with dog/dogactor.cpp (2026-10-27): its loader and release paths
// run the constructor and reset. Both sit just before dog/dogmotionblend.cpp's
// inferred range (0x108588 is unassigned@00108588, 0x10737c is in
// unassigned@00106658's tail); the class is this file's.

#include "dog/dogmotionblend.h"
#include "dog/dogactor.h"

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00108588

DogMotionBlend::DogMotionBlend()
{
    m_0c = 1.0f;
    m_no = -1;
    m_speed = 0x1.11112Ep-6f; /* 0x3c888897 */
    m_10 = -1;
    m_frame = 0.0f;
    /* PC: GCC 2 leaves the view to the owner (DogActor::clear sets it); a
     * host object starts without one. */
    m_max_motion = 0;
    m_motion = NULL;
    for (int i = 0; i < DOG_MOTION_BLEND_MAX; i++)
        m_blend[i].clear();
}

// ROR 0x001081a8

int DogMotionBlend::getNo()
{
    return m_no;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: DogMotionBlend::getNo (called; the entry clear inline)
// ROR 0x0010737c

void DogMotionBlend::reset()
{
    if (m_motion == NULL)
    {
        m_no = -1;
        m_speed = 0x1.11112Ep-6f; /* 0x3c888897 */
        m_frame = 0.0f;
        for (int i = 0; i < DOG_MOTION_BLEND_MAX; i++)
            m_blend[i].clear();
        return;
    }
    int no = getNo();
    if (no >= 0 && m_motion[no].m_i3m_root == NULL)
    {
        m_no = -1;
        m_speed = 0x1.11112Ep-6f;
        m_frame = 0.0f;
    }
    for (int i = 0; i < DOG_MOTION_BLEND_MAX; i++)
    {
        if (m_blend[i].m_no >= 0 && m_motion[m_blend[i].m_no].m_i3m_root == NULL)
            m_blend[i].clear();
    }
}
