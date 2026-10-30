/* ==========================================================================
 *  mag/maglifebwnmode.cpp
 *
 *  Brown's modes: the per-state objects MagLifeBwn embeds (+0xcf0..+0xd30).
 *  MagLifeBwn::applyState runs init on entry and term when another state
 *  is requested; MagLifeBwn::live runs live each frame. The motions are
 *  rows of Brown's motion sequence table (changeMotionSeq): 2 wait, 7 / 0xf
 *  sit, 0xc / 0x10 fuse (lie down), 0x2b.. turns, 0x45 / 0x4e / 0x57 /
 *  0x60 the walk gaits, 0x69 / 0x6b floor steps, 0x6a / 0x6c waiting on one.
 *  0x125bd4..0x1267c0 (pinned 2026-10-30; the asserts name this file).
 *  Not written: the GCC 2 type_info functions MagLifeBwnMode::__tf 0x28347c
 *  / MagLifeBwnModePath::__tf 0x283674 (no RTTI is read on the host).
 * ======================================================================== */

#include <string.h>

#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "mag/maglifebwn.h"

/* The walk rows by gait (m_08 0..3; 1 when Brown limps, health > 2). */
static const int s_walk_motion[4] = { 0x57, 0x4e, 0x60, 0x45 };

/* The rotate's angle bounds (the ROM's floats, a hair under pi / 2, pi / 4
 * and 2 degrees). */
#define BWN_ROT_90  0x1.921fb4p+0f
#define BWN_ROT_45  0x1.921fb4p-1f
#define BWN_ROT_END 0x1.1df46cp-5f

/* d into -DOG_PI..DOG_PI by one turn. */
static float wrapAngle(float d)
{
    if (d < -DOG_PI)
        d += DOG_PI2;
    else if (DOG_PI <= d)
        d -= DOG_PI2;
    return d;
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 2, the shared one: the motion runs to its end, then state 0.
// ROR 0x00125bd4
int MagLifeBwnMode::live(MagLifeBwn &bwn)
{
    bwn.stepMotion();
    if (bwn.isMotionSeqEnd())
        bwn.changeMode(0, 0, 0);
    return 0;
}

// ROR 0x00125c24
void MagLifeBwnModeNull::init(MagLifeBwn &bwn)
{
}

// ──────────────────────────────────────────────────────────────────────
// Wait: 0x25 after the wall state, else by the floor step under Brown.
// ROR 0x00125c2c
void MagLifeBwnModeWait::init(MagLifeBwn &bwn)
{
    int step = bwn.testFloorStep(0.0f, NULL);
    bwn.m_70 = step;
    int motion;
    if (bwn.m_prev_state[0] == 9)
        motion = 0x25;
    else if (step == 2)
        motion = 0x6a;
    else if (step == 3)
        motion = 0x6c;
    else
        motion = 2;
    bwn.changeMotionSeq(motion, -1);
    m_04 = 0;
    m_08 = 0;
    bwn.updateLimp();
}

// ──────────────────────────────────────────────────────────────────────
// Sit: not on a floor step (waits instead); 0xf straight from fuse.
// ROR 0x00125cb8
void MagLifeBwnModeSit::init(MagLifeBwn &bwn)
{
    if ((u_int)(bwn.m_70 - 2) < 2)
    {
        bwn.changeMode(2, 0, 0);
        return;
    }
    bwn.setNeckControl(0);
    bwn.clearLegMotion();
    bwn.changeMotionSeq(bwn.m_prev_state[0] == 4 ? 0xf : 7, -1);
}

// Unforced: the sitting row 8 stands up first (row 9), except into fuse.
// Forced: the stand-up motion runs to its end.
// ROR 0x00125d38
int MagLifeBwnModeSit::term(MagLifeBwn &bwn, int force)
{
    if (force == 0)
    {
        if (bwn.m_next_state[0] == 4)
            return 1;
        if (bwn.getMotionSeq() == 8)
        {
            bwn.changeMotionSeq(9, -1);
            return 0;
        }
        return 1;
    }
    bwn.stepMotion();
    return bwn.isMotionSeqEnd() != 0;
}

// ──────────────────────────────────────────────────────────────────────
// Fuse (lie down): as sit; 0x10 straight from sit.
// ROR 0x00125db4
void MagLifeBwnModeFuse::init(MagLifeBwn &bwn)
{
    if ((u_int)(bwn.m_70 - 2) < 2)
    {
        bwn.changeMode(2, 0, 0);
        return;
    }
    bwn.setNeckControl(0);
    bwn.clearLegMotion();
    bwn.changeMotionSeq(bwn.m_prev_state[0] == 3 ? 0x10 : 0xc, -1);
}

// As sit's: the lying row 0xd gets up first (row 0xe), except into sit.
// ROR 0x00125e34
int MagLifeBwnModeFuse::term(MagLifeBwn &bwn, int force)
{
    if (force == 0)
    {
        if (bwn.m_next_state[0] == 3)
            return 1;
        if (bwn.getMotionSeq() == 0xd)
        {
            bwn.changeMotionSeq(0xe, -1);
            return 0;
        }
        return 1;
    }
    bwn.stepMotion();
    return bwn.isMotionSeqEnd() != 0;
}

// ──────────────────────────────────────────────────────────────────────
// Move: the walk row of gait m_08 (the limp's when health > 2) + m_0c.
// ROR 0x00125eb0
void MagLifeBwnModeMove::init(MagLifeBwn &bwn)
{
    int gait = bwn.m_08;
    if (2 < bwn.m_health_status)
        gait = 1;
    /* PC: the ROM indexes its stack table with m_08 unchecked. */
    m_motion = s_walk_motion[(u_int)gait < 4 ? gait : 0] + bwn.m_0c;
    bwn.changeMotionSeq(m_motion, -1);
    bwn.updateLimp();
    m_step = 0;
}

// A floor step 75 ahead switches to the step motion (0x6b on step kind 3,
// 0x69 on 2) until the floor is flat again.
// ROR 0x00125f48
int MagLifeBwnModeMove::live(MagLifeBwn &bwn)
{
    int step = bwn.testFloorStep(75.0f, NULL);
    bwn.m_70 = step;
    if (m_step < 0x2000)
    {
        if (m_step < 0x1000)
        {
            if (step == 3 || step == 2)
            {
                bwn.changeMotionSeq(step == 3 ? 0x6b : 0x69, -1);
                m_step = 0x1000;
            }
        }
        else if (step == 0)
        {
            bwn.changeMotionSeq(m_motion, -1);
            m_step = 0;
        }
    }
    bwn.stepMotion();
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Path: along searchPath's line; first turn to it.
// ROR 0x00126008
void MagLifeBwnModePath::init(MagLifeBwn &bwn)
{
    m_phase = 0;
    bwn.setMotionSeqEnd();
    bwn.aimAtPath();
    bwn.updateLimp();
}

// 0: turn to the line, then walk (gait m_08; 4: 2, or 0 within 500 of
// the goal; the limp's at health > 2). 1: the blocked turn (0x43) to its
// end, then aim again. 2: follow the line, with the floor-step motions
// (both probes, 75 and 37.5 ahead, must agree for 0x6b and for flat).
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00126044
int MagLifeBwnModePath::live(MagLifeBwn &bwn)
{
    if (m_phase == 0)
    {
        int r = bwn.turnToPath();
        if (0 < r)
        {
            u_int type = bwn.m_08;
            if (bwn.m_health_status < 3)
            {
                if (type == 4)
                    type = bwn.m_goal_dist <= 500.0f ? 0 : 2;
            }
            else
                type = 1;
            ROR_ASSERT(0 <= type && type < 4); /* 377 */
            /* PC: past the assert the ROM reads its stack table anyway. */
            m_motion = s_walk_motion[type < 4 ? type : 0] + bwn.m_0c;
            bwn.changeMotionSeq(m_motion, -1);
            bwn.updateLimp();
            m_step = 0;
            m_phase = 2;
        }
        else if (r < 0)
        {
            bwn.changeMotionSeq(0x43, -1);
            m_phase = 1;
        }
    }
    else if (m_phase == 1)
    {
        int r = bwn.stepMotion();
        if (bwn.isMotionSeqEnd() || r < 1)
        {
            bwn.aimAtPath();
            m_phase = 0;
        }
    }
    else if (m_phase == 2)
    {
        bwn.followPath();
        int step_far = bwn.testFloorStep(75.0f, NULL);
        bwn.m_70 = step_far;
        int step_near = bwn.testFloorStep(37.5f, NULL);
        if (m_step < 0x2000)
        {
            if (m_step < 0x1000)
            {
                int motion;
                if (step_far == 3 && step_near == 3)
                    motion = 0x6b;
                else if (step_far == 2)
                    motion = 0x69;
                else
                    return 1;
                bwn.changeMotionSeq(motion, -1);
                m_step = 0x1000;
            }
            else if (step_far == 0 && step_near == 0)
            {
                bwn.changeMotionSeq(m_motion, -1);
                m_step = 0;
            }
        }
    }
    else
    {
        ROR_ASSERT(0); /* 468 */
    }
    return 1;
}

// Unforced: refused on a floor step; the walk rows 0x60..0x67 stop first
// (row 0x68). Forced: on a step it walks off it, else the stop runs out.
// ROR 0x001262a4
int MagLifeBwnModePath::term(MagLifeBwn &bwn, int force)
{
    if (force == 0)
    {
        if (m_step == 0x1000)
            return 0;
        if ((u_int)(bwn.getMotionSeq() - 0x60) < 8)
        {
            bwn.changeMotionSeq(0x68, -1);
            return 0;
        }
        return 1;
    }
    if (m_step == 0x1000)
    {
        bwn.followPath();
        int step = bwn.testFloorStep(75.0f, NULL);
        bwn.m_70 = step;
        if (step == 0)
            m_step = 0;
        return 1;
    }
    bwn.stepMotion();
    return bwn.isMotionSeqEnd() != 0;
}

// ──────────────────────────────────────────────────────────────────────
// Turn: row 0x2b + m_0c; the shared live runs it out.
// ROR 0x00126364
void MagLifeBwnModeTurn::init(MagLifeBwn &bwn)
{
    bwn.changeMotionSeq(bwn.m_0c + 0x2b, -1);
    bwn.updateLimp();
}

// ──────────────────────────────────────────────────────────────────────
// One: a single motion, row m_08 (from row 0).
// ROR 0x001263a0
void MagLifeBwnModeOne::init(MagLifeBwn &bwn)
{
    bwn.m_path_end = 0;
    bwn.changeMotionSeq(0, -1);
    bwn.changeMotionSeq(bwn.m_08, -1);
    bwn.clearLegMotion();
}

// m_0c 0: back to state 0 at its end; else it holds there (m_path_end).
// ROR 0x001263ec
int MagLifeBwnModeOne::live(MagLifeBwn &bwn)
{
    if (bwn.m_0c == 0)
        return MagLifeBwnMode::live(bwn);
    if (bwn.m_path_end == 0)
    {
        bwn.stepMotion();
        if (bwn.isMotionSeqEnd())
            bwn.m_path_end = 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Wall: row 0x1d / 0x1f / 0x21 / 0x23 by m_08 % 4.
// ROR 0x00126450
void MagLifeBwnModeWall::init(MagLifeBwn &bwn)
{
    static const int s_wall_motion[4] = { 0x1d, 0x1f, 0x21, 0x23 };
    /* PC: a negative m_08 makes the ROM's remainder index below its table. */
    int i = bwn.m_08 % 4;
    bwn.changeMotionSeq(s_wall_motion[i < 0 ? 0 : i], -1);
    bwn.clearLegMotion();
}

// ──────────────────────────────────────────────────────────────────────
// Rotate: to the heading in m_08's float bits (MagLifeBwnCtrl's message 8).
// ROR 0x001264d8
void MagLifeBwnModeRotate::init(MagLifeBwn &bwn)
{
    memcpy(&m_target_rot, &bwn.m_08, sizeof m_target_rot);
    startTurn(bwn);
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x001264f8
void MagLifeBwnModeRotate::startTurn(MagLifeBwn &bwn)
{
    ROR_ASSERT((m_target_rot >= -DOG_PI) && (m_target_rot < DOG_PI)); /* 632 */
    bwn.m_path_end = 0;
    float d = wrapAngle(m_target_rot - bwn.getRotY());
    m_rot_left = d;
    int motion;
    if (d < -BWN_ROT_90)
        motion = 0x39;
    else if (d < -BWN_ROT_45)
        motion = 0x2c;
    else if (d < -BWN_ROT_END)
        motion = 0x2b;
    else if (BWN_ROT_90 < d)
        motion = 0x3a;
    else if (BWN_ROT_45 < d)
        motion = 0x2f;
    else if (BWN_ROT_END < d)
        motion = 0x2e;
    else
    {
        bwn.m_08 = 1;
        bwn.m_path_end = 1;
        return;
    }
    bwn.changeMotionSeq(motion, -1);
    bwn.updateLimp();
}

// Ends (m_path_end) within 2 degrees or once the angle left changes sign;
// a blocked step falls back to the wait row. Each turn motion's end
// starts the next.
// ROR 0x0012668c
int MagLifeBwnModeRotate::live(MagLifeBwn &bwn)
{
    if (bwn.m_path_end != 0)
        return 0;
    if (bwn.stepMotion() == -1)
    {
        bwn.changeMotionSeq(2, -1);
        return 0;
    }
    float d = wrapAngle(m_target_rot - bwn.getRotY());
    if ((d <= -BWN_ROT_END || BWN_ROT_END <= d) && 0.0f <= d * m_rot_left)
        m_rot_left = d;
    else
    {
        bwn.m_path_end = 1;
        m_rot_left = d;
    }
    if (bwn.isMotionSeqEnd())
        startTurn(bwn);
    return 0;
}
