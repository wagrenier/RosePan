// dog/dogmotionblend.cpp -- DogMotionBlend, the motion state of a DogActor.
// Written with dog/dogactor.cpp (2026-10-27): its loader and release paths
// run the constructor and reset. Both sit just before dog/dogmotionblend.cpp's
// inferred range (0x108588 is unassigned@00108588, 0x10737c is in
// unassigned@00106658's tail); the class is this file's.
#include "common/rosepan_stub.h"

#include "dog/dogmotionblend.h"
#include "dog/dogactor.h"
#include "i3d/i3mroot.h"
#include "lib/unassigned.h"

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
// Written with dog/dogactor.cpp's runMotionCmd (2026-10-27).
// ROR 0x00108178

void DogMotionBlend::stop()
{
    m_speed = 0.0f;
    for (int i = DOG_MOTION_BLEND_MAX - 1; i >= 0; i--)
        m_blend[i].m_speed = 0.0f;
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

// ──────────────────────────────────────────────────────────────────────
// The asserts below read m_main.m_no / m_sub[i].m_no / m_main.m_time in the
// ROM: the source kept the motion fields (m_no, m_speed, m_frame) as an
// entry m_main and the entries as m_sub[4]. The text is the ROM's with this
// header's names. Each function repeats the length lookup with its own line
// numbers (a macro or an inline in the source).
// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x00107560

float DogMotionBlend::getTimeRatio()
{
    if (m_no < 0)
        return 0.0f;
    ROR_ASSERT(m_no >= 0 && m_no < m_max_motion); /* 83 */
    I3mRoot *root = m_motion[m_no].m_i3m_root;
    ROR_ASSERT(root); /* 84: m_motion[m_main.m_no].m_i3m_root */
    float mf = root != NULL ? i3mGetLength(root) : 0.0f;  // PC: NULL only where the ROM hangs in the assert
    ROR_ASSERT(mf >= 0.0f); /* 87 */
    float r = 0.0f;
    if (m_frame > r)
    {
        r = 1.0f;
        if (m_frame <= mf)
            r = m_frame / mf;
    }
    return r;
}

// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x00107678

int DogMotionBlend::setTimeRatio(float theRatio)
{
    ROR_ASSERT(0.0f <= theRatio && theRatio <= 1.0f); /* 113 */
    if (m_no >= 0)
    {
        ROR_ASSERT(m_no >= 0 && m_no < m_max_motion); /* 116 */
        I3mRoot *root = m_motion[m_no].m_i3m_root;
        ROR_ASSERT(root); /* 117: m_motion[m_main.m_no].m_i3m_root */
        float mf = root != NULL ? i3mGetLength(root) : 0.0f;
        ROR_ASSERT(mf >= 0.0f); /* 120 */
        m_frame = mf * theRatio;
    }
    return 0;
}

// ROR 0x001078dc

float DogMotionBlend::getNodeRate(int theNode)
{
    int i;
    for (i = 0; i < DOG_MOTION_BLEND_MAX; i++)
    {
        if (m_blend[i].m_no >= 0 && (m_blend[i].m_node_mask[theNode / 32] >> (theNode & 31) & 1))
            break;
    }
    if (i >= DOG_MOTION_BLEND_MAX)
        return 1.0f;
    DogMotionBlendEntry &e = m_blend[i];
    if (e.m_state == 2)
    {
        if (e.m_time < 1)
            return 1.0f;
        float r = (float)(e.m_count + 1) / (float)e.m_time;
        if (r < 0.0f)
            return 0.0f;
        if (r <= 1.0f)
            return r;
    }
    return 1.0f;
}

// One entry's frame step, shared by step and stepLoop (inline in the ROM:
// both repeat it). A fading entry holds its m_1c ratio and counts the fade.
static void dogMotionBlendFade(DogMotionBlendEntry &e, float mf)
{
    e.m_frame = e.m_1c * mf;
    e.m_speed = 0.0f;
    if (e.m_count < e.m_time)
        e.m_count++;
    else
        e.m_count = e.m_time;
}

// callcheck: FUN_00215a30, dogMotionBlendFade (the assert handler; the fade step inline)
// ROR 0x001079d8

int DogMotionBlend::stepLoop()
{
    ROR_ASSERT(m_motion); /* 207 */
    if (m_motion == NULL)
        return 0;   // PC: the ROM hangs in the assert
    int ret = 0;
    if (m_no >= 0)
    {
        ROR_ASSERT(m_no >= 0 && m_no < m_max_motion); /* 238 */
        I3mRoot *root = m_motion[m_no].m_i3m_root;
        ROR_ASSERT(root); /* 239: m_motion[m_main.m_no].m_i3m_root */
        float mf = root != NULL ? i3mGetLength(root) : 0.0f;
        ROR_ASSERT(mf >= 0.0f); /* 242 */
        float d = m_speed * m_0c;
        if (mf < d)
            d = mf;
        if (mf == 0.0f)
        {
            m_frame = 0.0f;
        }
        else
        {
            float t = m_frame + d;
            m_frame = t;
            if (d > 0.0f)
            {
                float r = t - mf;
                if (-1e-05f <= r && r <= 1e-05f)
                {
                    m_frame = 0.0f;
                    ret = 1;
                }
                else if (r > 0.0f)
                {
                    m_frame -= mf;
                    ret = 1;
                }
            }
            else if (d >= 0.0f)
            {
                ret = 1;    // stopped: the ROM reports a wrap
            }
            else if (t < 0.0f)
            {
                m_frame = t + mf;
                ret = 2;
            }
            ROR_ASSERT(0.0f <= m_frame && m_frame < mf); /* 287: m_main.m_time */
        }
    }
    for (int i = 0; i < DOG_MOTION_BLEND_MAX; i++)
    {
        DogMotionBlendEntry &e = m_blend[i];
        if (e.m_no < 0)
            continue;
        ROR_ASSERT(e.m_no >= 0 && e.m_no < m_max_motion); /* 298: m_sub[i] */
        I3mRoot *root = m_motion[e.m_no].m_i3m_root;
        ROR_ASSERT(root); /* 299: m_motion[m_sub[i].m_no].m_i3m_root */
        float mf = root != NULL ? i3mGetLength(root) : 0.0f;
        ROR_ASSERT(mf >= 0.0f); /* 302 */
        if (e.m_state == 0)
        {
            if (mf == 0.0f)
            {
                e.m_frame = 0.0f;
                continue;
            }
            float d = e.m_speed * m_0c;
            float t = e.m_frame + d;
            e.m_frame = t;
            if (d > 0.0f)
            {
                if (e.m_1c >= 0.0f && e.m_1c * mf <= t)
                    e.m_frame = e.m_1c * mf;
                if (e.m_frame >= mf)
                    e.m_frame -= mf;
            }
            else if (d < 0.0f)
            {
                if (e.m_1c >= 0.0f && t <= e.m_1c * mf)
                    e.m_frame = e.m_1c * mf;
                if (e.m_frame < 0.0f)
                    e.m_frame += mf;
            }
        }
        else if (e.m_state == 2)
        {
            dogMotionBlendFade(e, mf);
        }
    }
    return ret;
}

// callcheck: FUN_00215a30, dogMotionBlendFade (the assert handler; the fade step inline)
// ROR 0x00107e30

int DogMotionBlend::step()
{
    ROR_ASSERT(m_motion); /* 365 */
    if (m_motion == NULL)
        return 0;   // PC: the ROM hangs in the assert
    int ret = 0;
    if (m_no >= 0)
    {
        ROR_ASSERT(m_no >= 0 && m_no < m_max_motion); /* 394 */
        I3mRoot *root = m_motion[m_no].m_i3m_root;
        ROR_ASSERT(root); /* 395: m_motion[m_main.m_no].m_i3m_root */
        float mf = root != NULL ? i3mGetLength(root) : 0.0f;
        ROR_ASSERT(mf >= 0.0f); /* 398 */
        if (mf == 0.0f)
        {
            m_frame = 0.0f;
            ret = 3;
        }
        else
        {
            float d = m_speed * m_0c;
            float t = m_frame + d;
            m_frame = t;
            if (d > 0.0f)
            {
                if (mf <= t)
                {
                    m_frame = mf;
                    ret = 1;
                }
            }
            else if (d < 0.0f)
            {
                if (t < 0.0f)
                {
                    m_frame = 0.0f;
                    ret = 2;
                }
            }
            else
            {
                ret = 3;
            }
        }
    }
    for (int i = 0; i < DOG_MOTION_BLEND_MAX; i++)
    {
        DogMotionBlendEntry &e = m_blend[i];
        if (e.m_no < 0)
            continue;
        ROR_ASSERT(e.m_no >= 0 && e.m_no < m_max_motion); /* 443: m_sub[i] */
        I3mRoot *root = m_motion[e.m_no].m_i3m_root;
        ROR_ASSERT(root); /* 444: m_motion[m_sub[i].m_no].m_i3m_root */
        float mf = root != NULL ? i3mGetLength(root) : 0.0f;
        ROR_ASSERT(mf >= 0.0f); /* 447 */
        if (e.m_state == 0)
        {
            if (mf == 0.0f)
            {
                e.m_frame = 0.0f;
                continue;
            }
            float t = e.m_frame + e.m_speed * m_0c;
            e.m_frame = t;
            if (e.m_speed > 0.0f)
            {
                if (mf <= t)
                    e.m_frame = t - mf;
            }
            else if (t < 0.0f)
            {
                e.m_frame = t + mf;
            }
        }
        else if (e.m_state == 2)
        {
            dogMotionBlendFade(e, mf);
        }
    }
    return ret;
}

// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x001081b0

int DogMotionBlend::getPose(void *thePosture)
{
    if (m_no < 0)
        return 0;
    ROR_ASSERT(thePosture); /* 522 */
    ROR_ASSERT(m_motion); /* 523 */
    ROR_ASSERT((m_no >= 0) && (m_no < m_max_motion)); /* 528: m_main.m_no */
    I3mRoot *root = m_motion[m_no].m_i3m_root;
    if (root == NULL)
        return 0;   // PC: the ROM reads the NULL record's node count
    int maxnode = i3mGetNumNode(root);
    ROR_ASSERT(maxnode <= (((int)(sizeof(int)*8))*4)); /* 532 */
    /* 128 nodes of 0x30 bytes: the ROM's 0x1800-byte stack block. */
    u_char s_pose[((int)(sizeof(int) * 8)) * 4 * 0x30];
    u_char *out = (u_char *)thePosture;
    i3mGetPose(root, thePosture, 0x111, m_frame);
    for (int s = 0; s < DOG_MOTION_BLEND_MAX; s++)
    {
        DogMotionBlendEntry &e = m_blend[s];
        int no = e.m_no;
        if (no < 0)
            continue;
        ROR_ASSERT(no >= 0 && no < m_max_motion); /* 556 */
        ROR_ASSERT(m_motion[no].m_i3m_root); /* 557 */
        if (m_motion[no].m_i3m_root == NULL)
            continue;   // PC: the ROM hangs in the assert
        i3mGetPose(m_motion[no].m_i3m_root, s_pose, 0x111, e.m_frame);
        int bits = 0;
        for (int i = 0; i < maxnode; i++)
        {
            if ((i & 31) == 0)
                bits = e.m_node_mask[i / 32];
            /* t 0: the entry's pose replaces the node's (the actor blends
             * the two with getNodeRate afterwards). */
            if (bits & 1)
                i3mPoseLerp(0.0f, out + i * 0x30, out + i * 0x30, s_pose + i * 0x30);
            bits >>= 1;
        }
    }
    return maxnode;
}

// callcheck: FUN_00215a30, fptodp, dpcmp (the assert handler; the 0.0 double compare)
// ROR 0x00108428

void DogMotionBlend::setBlendRate(float theRatio, int theNo, int theTime)
{
    ROR_ASSERT(theNo >= 0 && theNo < 4); /* 585 */
    ROR_ASSERT(theRatio >= 0.0 && theRatio <= 1.0f); /* 586 */
    ROR_ASSERT(theTime >= 0 && theTime < 1000); /* 587 */
    if (theNo < 0 || theNo >= DOG_MOTION_BLEND_MAX)
        return;     // PC: the ROM hangs in the assert
    DogMotionBlendEntry &e = m_blend[theNo];
    if (e.m_1c != theRatio)
    {
        e.m_1c = theRatio;
        e.m_time = theTime;
        e.m_count = 0;
        e.m_state = 2;
    }
    else if (theRatio == 0.0f && e.m_no >= 0 && e.m_time <= e.m_count)
    {
        e.m_no = -1;
    }
}
