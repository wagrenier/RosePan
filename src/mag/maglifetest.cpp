/* ==========================================================================
 *  mag/maglifetest.cpp
 *
 *  A test controller (RoR's own file name, from its assert; pinned
 *  0x130ee8..0x131184 + the linkonce 0x2853c4..0x285450): a
 *  MagLifeControlChara that plays a fixed motion chain on R2 and moves its
 *  actor by the motion's root movement; see maglifetest.h. Unreferenced in
 *  the ROM.
 * ======================================================================== */
#include "mag/maglifetest.h"

#include "bed/bedpad.h"
#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "utl/utlmath.h"

/* The frames since the chain's last motion change (shared by every
 * MagLifeTest; live counts it up even while idle). */
static int s_frames;                                                     // ROR 0x00298b78

/* The motion speed of the chain, 1/60 (0x3c888897). */
#define MAG_TEST_SPEED 0x1.11112Ep-6f

// callcheck: MagLifeControlChara (the base constructor runs implicitly)
// ROR 0x0013114c
MagLifeTest::MagLifeTest()
{
    m_rot_y_aar = 0.0f;
    m_phase = 0;
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 2. Phase 1 plays motion 0xac; 43 frames later motion 5 (flags
// 0x40), 5 frames after that motion 1 (flags 1) and back to idle. Then,
// while the actor's mode has no argument, the root movement of this
// frame's span of the motion (frames doubled, as getMotionMove takes
// them), flattened, turned by the base heading, goes into addTrans with
// the root's height.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00130ee8
void MagLifeTest::live()
{
    if (!m_actor)
        return;
    (void)m_actor->getRotY(); /* the ROM calls it and drops the result */
    s_frames++;
    DogActor *actor = m_actor;
    if (m_phase == 1) {
        actor->changeMotion(MAG_TEST_SPEED, 0xac, 0, -1, -1);
        s_frames = 0;
        m_phase = 2;
    } else if (m_phase == 2) {
        if (s_frames > 0x2a) {
            actor->changeMotion(MAG_TEST_SPEED, 5, 0x40, -1, -1);
            s_frames = 0;
            m_phase = 3;
        }
    } else if (m_phase == 3) {
        if (s_frames > 4) {
            actor->changeMotion(MAG_TEST_SPEED, 1, 1, -1, -1);
            s_frames = 0;
            m_phase = 0;
        }
    }

    float move[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float rot[4] = { 0.0f }; /* the ROM sets only word 0 */
    float ofs_y = 0.0f;
    if (m_actor->getModeArg() >= 0)
        return;
    float frame = m_actor->getMotionFrame();
    if (0.0f <= frame) {
        float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        float next = frame + m_actor->m_motion.m_speed / MAG_TEST_SPEED;
        m_actor->getMotionMove(frame + frame, next + next, move, rot, pos);
        ofs_y = pos[1];
    }
    move[1] = 0.0f;
    move[3] = 1.0f;
    float vry = m_actor->m_base_rotate[1];
    ROR_ASSERT((vry >= -DOG_PI) && (vry < DOG_PI)); /* 186 */
    utlVecRotMatY(vry, move);
    m_actor->addTrans(move, ofs_y, m_rot_y_aar);
}

// ROR 0x00131120
void MagLifeTest::processPad(BedPad *pad)
{
    if (m_actor && (pad->m_trig >> 9 & 1))
        m_phase = 1;
}
