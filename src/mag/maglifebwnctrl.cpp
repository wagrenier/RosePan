/* ==========================================================================
 *  mag/maglifebwnctrl.cpp
 *
 *  MagLifeBwnCtrl, Brown's command layer: the state requests over the
 *  embedded mode objects (requestState / applyMode / live), the script
 *  natives' setters (position, rotation, find, bark, level), the PC's dog
 *  orders (come / stay / go, with the tests they make around the PC) and
 *  the hug check; the modes: Null (picks the next state), Wait (idles,
 *  follows, barks), Stay, FindEnd, Move / MoveEnd, Rot / RotEnd, Damage,
 *  Hug, Wound and the small Bark helpers.
 *  0x1267c0..0x12bc20 (pinned 2026-10-28; the asserts name this file).
 *  Left as stubs: Follow::live, Find::init / live, FindEnd::live,
 *  Bark::live and their helpers (0x127558, 0x128724, 0x128854, 0x128e6c,
 *  0x1267c0, 0x1267d8).
 * ======================================================================== */

#include "mag/maglifebwnctrl.h"

#include "common/ror_assert.h"
#include "common/rosepan_stub.h"
#include "dog/dogactor.h"
#include "lib/unassigned.h"
#include "mag/maglifebwn.h"
#include "mag/maglifepc.h"
#include "mag/magscene.h"
#include "utl/utlgame.h"
#include "utl/utlmath.h"

/* The angle wrapped into -pi .. pi (one step, as the ROM does). */
static inline float wrapAngle(float a)
{
    if (a < -DOG_PI)
        a += 6.283186f;
    else if (DOG_PI <= a)
        a -= 6.283186f;
    return a;
}

/* The collectors' buffer as the ROM's callers set it up: every entry's
 * two points (0, 0, 0, 1), no count. */
static void clearEmyInfoBuf(UtlEmyInfoBuf &eib)
{
    for (int i = 0; i < 32; i++)
    {
        UtlEmyInfo &e = eib.m_info[i];
        e.m_pos[2] = 0.0f;
        e.m_pos[1] = 0.0f;
        e.m_pos[0] = 0.0f;
        e.m_pos[3] = 1.0f;
        e.m_trans[3] = 1.0f;
        e.m_trans[2] = 0.0f;
        e.m_trans[1] = 0.0f;
        e.m_trans[0] = 0.0f;
    }
    eib.m_num = 0;
}

/* Brown's idle actions in Wait (MagLifeBwn::message 1's motion). */
// ROR 0x002ebe10
static const int s_waitActions[5] = { 18, 3, 4, 5, 6 };

/* orderCome's answers when Brown is already at the PC. */
// ROR 0x002ec448
static const int s_comeActions[6] = { 3, 4, 5, 6, 39, 18 };

// ──────────────────────────────────────────────────────────────────────
// MagLifeBwnCtrlMode helpers
// ──────────────────────────────────────────────────────────────────────

// ROR 0x001267f4
int MagLifeBwnCtrlMode::countEmyAround(MagLifeBwnCtrl &theBwnCtrl, float radius)
{
    if (theBwnCtrl.m_bwn->getHealthStatus() >= 3)
        return 0;
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    theBwnCtrl.m_bwn->getPosition(pos);
    UtlEmyInfoBuf eib;
    clearEmyInfoBuf(eib);
    return utlCollectEmyInfoAt(&eib, pos, 0, 1, 0.0f, radius);
}

// ROR 0x001268b4
int MagLifeBwnCtrlMode::checkBark(MagLifeBwnCtrl &theBwnCtrl)
{
    MagLifeBwn *bwn = theBwnCtrl.m_bwn;
    if (bwn == NULL || bwn->getHealthStatus() >= 3)
        return 0;
    if (countEmyAround(theBwnCtrl, 200.0f) < 1)
        return 0;
    theBwnCtrl.requestState(11, 0, 0);
    return 1;
}

// Phase 0 starts a turn towards m_angle (MagLifeBwn mode 7, the turn
// motion by sixths of pi), phase 1 waits for it; within 2 degrees: state 9.
// ROR 0x00126928
void MagLifeBwnCtrlMode::turnToAngle(MagLifeBwnCtrl &theBwnCtrl)
{
    if (theBwnCtrl.m_bwn == NULL)
        return;
    float d = wrapAngle(theBwnCtrl.m_angle - theBwnCtrl.m_bwn->getRotY());
    int phase = theBwnCtrl.m_08;
    if (phase == 0)
    {
        if (-0.03490659f < d && d < 0.03490659f)
        {
            theBwnCtrl.requestState(9, 0, 0);
            return;
        }
        int turn;
        if (-1.0471976f < d && d < 0.0f)
            turn = 0;
        else if (-2.0943952f < d && d < 0.0f)
            turn = 1;
        else if (d < 0.0f)
            turn = 2;
        else if (0.0f < d && d < 1.0471976f)
            turn = 3;
        else if (0.0f < d && d < 2.0943952f)
            turn = 4;
        else
            turn = 5;
        theBwnCtrl.m_bwn->message(7, 0, turn); /* EE slot 14 */
        theBwnCtrl.m_08 = 1;
        return;
    }
    if (phase != 1)
        return;
    if (-0.03490659f < d && d < 0.03490659f)
    {
        theBwnCtrl.requestState(9, 0, 0);
        return;
    }
    if (theBwnCtrl.m_bwn->m_state == 0)
        theBwnCtrl.m_08 = 0;
}

// ──────────────────────────────────────────────────────────────────────
// The modes
// ──────────────────────────────────────────────────────────────────────

// ROR 0x00126ba0
void MagLifeBwnCtrlModeNull::init(MagLifeBwnCtrl &theBwnCtrl)
{
    if (theBwnCtrl.m_bwn != NULL)
        live(theBwnCtrl); /* EE slot 2 */
}

// The next state in level AUTO: Wound at health 4, the mission's state,
// a bark, else Wait.
// ROR 0x00126bd8
void MagLifeBwnCtrlModeNull::live(MagLifeBwnCtrl &theBwnCtrl)
{
    if (theBwnCtrl.m_ctrl_level != MAG_BWNCTRL_LEVEL_AUTO)
        return;
    /* PC: the ROM reads the health through a NULL m_bwn here (low memory
     * on the EE); the host would fault. */
    if (theBwnCtrl.m_bwn == NULL)
        return;
    if (theBwnCtrl.m_bwn->getHealthStatus() == 4)
    {
        theBwnCtrl.requestState(13, 0, 0);
        return;
    }
    switch (theBwnCtrl.m_mission)
    {
    case MISSION_FIND:
        theBwnCtrl.requestState(4, 0, 0);
        return;
    case MISSION_STAY:
        theBwnCtrl.requestState(3, 0, 0);
        return;
    case MISSION_HUG:
        theBwnCtrl.requestState(12, -1, 0);
        return;
    }
    if (checkBark(theBwnCtrl) == 0)
        theBwnCtrl.requestState(1, 0, 0);
}

// ROR 0x00126ccc
void MagLifeBwnCtrlModeWait::init(MagLifeBwnCtrl &theBwnCtrl)
{
    MagLifeBwn *bwn = theBwnCtrl.m_bwn;
    if (bwn == NULL)
        return;
    int state;
    if (theBwnCtrl.m_mission == MISSION_FIND)
        state = 4;
    else if (theBwnCtrl.m_mission == MISSION_STAY)
        state = 3;
    else
    {
        if (theBwnCtrl.m_pc == NULL)
        {
            bwn->message(2, 0, 0); /* EE slot 14: Brown's Wait */
            return;
        }
        float pc[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        theBwnCtrl.m_pc->getPosition(pc);
        float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        if (theBwnCtrl.m_bwn == NULL)
            return;
        theBwnCtrl.m_bwn->getPosition(pos);
        if (170.0f < utlVecDist(pos, pc))
            state = 2;
        else if (theBwnCtrl.m_bwn->getHealthStatus() == 4)
            state = 13;
        else if (theBwnCtrl.m_bwn->getHealthStatus() == 3)
        {
            bwn->message(2, 0, 0);
            return;
        }
        else if (countEmyAround(theBwnCtrl, 500.0f) > 0)
            state = 11;
        else
        {
            theBwnCtrl.m_bwn->message(2, 0, 0);
            return;
        }
    }
    theBwnCtrl.requestState(state, 0, 0);
}

// Follows once the PC is 170 away, barks at enemies within 500 (every 30
// frames), plays an idle action every 300 frames (half the time), sits at
// 1801 frames and lies down at 3601; the neck looks at the PC while
// Brown's Wait sequence row (2) plays.
// ROR 0x00126e68
void MagLifeBwnCtrlModeWait::live(MagLifeBwnCtrl &theBwnCtrl)
{
    if (theBwnCtrl.m_pc == NULL)
        return;
    float pc[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    theBwnCtrl.m_pc->getPosition(pc);
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (theBwnCtrl.m_bwn == NULL)
        return;
    theBwnCtrl.m_bwn->getPosition(pos);
    if (170.0f <= utlVecDist(pos, pc))
    {
        theBwnCtrl.requestState(2, 0, 0);
        return;
    }
    if (theBwnCtrl.m_bwn->getHealthStatus() < 3 && theBwnCtrl.m_mode_count % 30 == 0 &&
        countEmyAround(theBwnCtrl, 500.0f) > 0)
    {
        theBwnCtrl.requestState(11, 0, 0);
        return;
    }
    MagLifeBwn *bwn = theBwnCtrl.m_bwn;
    if (bwn->m_70 != 0)
        return;
    if (bwn->getHealthStatus() == 4)
    {
        theBwnCtrl.requestState(13, 0, 0);
        return;
    }
    int count = theBwnCtrl.m_mode_count;
    if (count >= 3601)
    {
        bwn->message(4, 0, 0); /* Fuse */
        return;
    }
    if (count >= 1801)
    {
        bwn->message(3, 0, 0); /* Sit */
        return;
    }
    if (count % 300 == 0)
    {
        u_int r = (u_int)utlRandRange(0, 10);
        if (r < 5)
        {
            theBwnCtrl.m_bwn->setNeckControl(0);
            theBwnCtrl.m_bwn->m_state = -1;
            theBwnCtrl.m_bwn->m_04 = -1;
            theBwnCtrl.m_bwn->message(1, s_waitActions[r], 0);
        }
    }
    if (theBwnCtrl.m_bwn->getNeckControl() == 0 && theBwnCtrl.m_bwn->getMotionSeq() == 2)
        theBwnCtrl.m_bwn->setNeckControl(2);
    if (theBwnCtrl.m_bwn->getNeckControl() < 1)
        return;
    if (theBwnCtrl.m_bwn->getMotionSeq() != 2)
        return;
    theBwnCtrl.m_bwn->setNeckControl(2);
    float look[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    theBwnCtrl.m_pc->getPosition(look);
    look[1] -= 80.0f;
    theBwnCtrl.m_bwn->setNeckPos(look);
}

// ROR 0x0012712c
void MagLifeBwnCtrlModeFollow::init(MagLifeBwnCtrl &theBwnCtrl)
{
    if (theBwnCtrl.m_bwn == NULL)
        return;
    theBwnCtrl.m_bwn->resetMove();
    theBwnCtrl.m_08 = 0;
    theBwnCtrl.m_60 = 0;
    m_04 = 0;
}

// ROR 0x00127a74
void MagLifeBwnCtrlModeStay::init(MagLifeBwnCtrl &theBwnCtrl)
{
    theBwnCtrl.m_08 = 0;
    if (theBwnCtrl.m_bwn != NULL)
        theBwnCtrl.m_mission = MISSION_STAY;
}

// Phase 0 asks Brown to sit (his mode 3) until he takes it.
// ROR 0x00127a90
void MagLifeBwnCtrlModeStay::live(MagLifeBwnCtrl &theBwnCtrl)
{
    if (theBwnCtrl.m_bwn == NULL)
        return;
    if (theBwnCtrl.m_08 == 0 && theBwnCtrl.m_bwn->message(3, 0, 0) >= 0)
        theBwnCtrl.m_08 = 1;
    if (theBwnCtrl.m_bwn->getHealthStatus() == 4)
        theBwnCtrl.requestState(13, 0, 0);
}

// The found object still there: taken (m_find_result 1), else back to
// Wait (-1).
// ROR 0x0012890c
void MagLifeBwnCtrlModeFindEnd::init(MagLifeBwnCtrl &theBwnCtrl)
{
    theBwnCtrl.m_mission = MISSION_NONE;
    if (utlIsObj(theBwnCtrl.m_found_obj) == 0)
    {
        theBwnCtrl.m_find_result = -1;
        theBwnCtrl.requestState(1, 0, 0);
        return;
    }
    theBwnCtrl.m_find_result = 1;
    utlMarkObjTaken(theBwnCtrl.m_found_obj);
    theBwnCtrl.m_08 = 0;
}

// ROR 0x00128b8c
void MagLifeBwnCtrlModeMove::init(MagLifeBwnCtrl &theBwnCtrl)
{
    theBwnCtrl.m_state_done = 0;
    if (theBwnCtrl.m_bwn->searchPath(theBwnCtrl.m_target) < 0)
        return;
    theBwnCtrl.m_bwn->message(6, 0, 0); /* Path */
    theBwnCtrl.m_08 = 0;
    theBwnCtrl.m_0c = 0;
}

// ROR 0x00128bf0
void MagLifeBwnCtrlModeMove::live(MagLifeBwnCtrl &theBwnCtrl)
{
    MagLifeBwn *bwn = theBwnCtrl.m_bwn;
    if (bwn->isPathMoveEnd())
    {
        theBwnCtrl.requestState(7, 0, 0);
        return;
    }
    if (bwn->getUnmovableCount() != 0)
        theBwnCtrl.requestState(7, 0, 0);
}

// ROR 0x00128c4c
void MagLifeBwnCtrlModeMoveEnd::init(MagLifeBwnCtrl &theBwnCtrl)
{
    theBwnCtrl.m_state_done = 1;
    theBwnCtrl.requestState(0, 0, 0);
}

// ROR 0x00128c88
void MagLifeBwnCtrlModeRot::init(MagLifeBwnCtrl &theBwnCtrl)
{
    theBwnCtrl.m_08 = 0;
    theBwnCtrl.m_state_done = 0;
}

// ROR 0x00128cb0
void MagLifeBwnCtrlModeRotEnd::init(MagLifeBwnCtrl &theBwnCtrl)
{
    theBwnCtrl.m_state_done = 1;
    theBwnCtrl.requestState(0, 0, 0);
}

// The hurt reaction (requestHurt's react in m_08): Brown's motion 0x6d /
// 0x6e, none for 3 / 4; 2 is not expected.
// ROR 0x00128cec
void MagLifeBwnCtrlModeDamage::init(MagLifeBwnCtrl &theBwnCtrl)
{
    int motion = 0;
    switch (theBwnCtrl.m_08)
    {
    case 0:
        motion = 0x6d;
        break;
    case 1:
        motion = 0x6e;
        break;
    case 2:
        ROR_ASSERT(0); /* 2038 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        break;
    case 3:
    case 4:
        break;
    default:
        ROR_ASSERT(0); /* 2048 */
        break;
    }
    theBwnCtrl.m_bwn->m_state = -1;
    theBwnCtrl.m_bwn->m_04 = -1;
    theBwnCtrl.m_bwn->message(1, motion, 1);
}

// ROR 0x00128dbc
void MagLifeBwnCtrlModeDamage::live(MagLifeBwnCtrl &theBwnCtrl)
{
    if (theBwnCtrl.m_bwn->isActionEnd())
    {
        theBwnCtrl.requestState(1, 0, 0);
        return;
    }
    ROR_ASSERT(!theBwnCtrl.m_bwn->isMotionStateEndless()); /* 2067 */  // callcheck: FUN_00215a30 (the assert handler)
}

// ROR 0x00128e20
void MagLifeBwnCtrlModeBark::init(MagLifeBwnCtrl &theBwnCtrl)
{
    m_28 = 0;
    m_answer = 0;
    utlSetBwnObjFlag(1);
    if (theBwnCtrl.m_08 == 0)
        theBwnCtrl.m_08 = 0;
    else
        theBwnCtrl.m_08 = 14;
}

// ROR 0x00129f50
int MagLifeBwnCtrlModeBark::query(MagLifeBwnCtrl &theBwnCtrl, int v)
{
    if (theBwnCtrl.m_08 != 11)
        return -1;
    return m_answer == v ? 1 : -1;
}

// ROR 0x00129f78
void MagLifeBwnCtrlModeBark::set(MagLifeBwnCtrl &theBwnCtrl, int v)
{
    theBwnCtrl.m_0c = v;
    theBwnCtrl.m_08 = 12;
}

// Hug 1 / 2: Brown's motion 0x7f / 0x80; else (mode 0) Brown's Wait while
// the live walks him up to the PC.
// ROR 0x00129f98
void MagLifeBwnCtrlModeHug::init(MagLifeBwnCtrl &theBwnCtrl)
{
    int mode = m_mode;
    if ((u_int)(mode - 1) < 2)
        theBwnCtrl.m_bwn->message(1, mode + 0x7e, 1);
    else
    {
        m_mode = 0;
        theBwnCtrl.m_bwn->m_state = -1;
        theBwnCtrl.m_bwn->m_04 = -1;
        theBwnCtrl.m_bwn->message(1, 2, 1);
    }
    theBwnCtrl.m_mission = MISSION_HUG;
}

// Mode 0 slides Brown to 100 from the PC (at most 1 a frame); 1 / 2 end
// the mission with the motion.
// ROR 0x0012a03c
void MagLifeBwnCtrlModeHug::live(MagLifeBwnCtrl &theBwnCtrl)
{
    if (m_mode == 0)
    {
        float pc[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        theBwnCtrl.m_pc->getPosition(pc);
        float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        theBwnCtrl.m_bwn->getPosition(pos);
        float step = utlVecDist(pc, pos) - 100.0f;
        if (step == 0.0f)
            return;
        if (1.0f < step)
            step = 1.0f;
        float dir[4] = { pc[0] - pos[0], pc[1] - pos[1], pc[2] - pos[2], 1.0f };
        utlNormalize3(dir);
        dir[0] *= step;
        dir[3] = 1.0f;
        dir[1] *= step;
        dir[2] *= step;
        DogActor *actor = theBwnCtrl.m_bwn->m_actor; /* EE slot 11 getActor */
        actor->m_trans[0] += dir[0];
        actor->m_trans[1] += dir[1];
        actor->m_trans[3] = 1.0f;
        actor->m_trans[2] += dir[2];
        actor->m_dirty = 1;
        return;
    }
    if ((u_int)(m_mode - 1) >= 2)
        return;
    if (theBwnCtrl.m_bwn->isActionEnd() == 0 && theBwnCtrl.m_bwn->isMotionStateEndless() == 0)
        return;
    theBwnCtrl.m_mission = MISSION_NONE;
    theBwnCtrl.requestState(0, 0, 0);
}

// Brown's wounded wait (motion "dmgl_wait"), unless it plays already.
// ROR 0x0012a21c
void MagLifeBwnCtrlModeWound::init(MagLifeBwnCtrl &theBwnCtrl)
{
    DogActor *actor = theBwnCtrl.m_bwn->m_actor; /* EE slot 11 getActor */
    ROR_ASSERT(actor); /* 2931 */  // callcheck: FUN_00215a30 (the assert handler)
    if (actor->getMotionNoBySuffix("dmgl_wait") != actor->m_motion.getNo())
    {
        theBwnCtrl.m_bwn->m_state = -1;
        theBwnCtrl.m_bwn->m_04 = -1;
        theBwnCtrl.m_bwn->message(1, 0x82, 0);
    }
    theBwnCtrl.m_08 = 0;
}

// Health below 3 again: Brown's get-up motion (0x70), then state 0.
// ROR 0x0012a2e8
void MagLifeBwnCtrlModeWound::live(MagLifeBwnCtrl &theBwnCtrl)
{
    MagLifeBwn *bwn = theBwnCtrl.m_bwn;
    if (theBwnCtrl.m_08 == 0)
    {
        if (bwn->getHealthStatus() < 3)
        {
            bwn->m_state = -1;
            bwn->m_04 = -1;
            theBwnCtrl.m_bwn->message(1, 0x70, 0);
            theBwnCtrl.m_08 = 1;
        }
        return;
    }
    if (bwn->isMotionSeqEnd())
        theBwnCtrl.requestState(0, 0, 0);
}

// ──────────────────────────────────────────────────────────────────────
// MagLifeBwnCtrl
// ──────────────────────────────────────────────────────────────────────

// ROR 0x0012a394
void MagLifeBwnCtrl::applyMode()
{
    int state = m_state;
    switch (state)
    {
    case 0:
        m_mode_cur = &m_mode_null;
        break;
    case 1:
        m_mode_cur = &m_mode_wait;
        break;
    case 2:
        m_mode_cur = &m_mode_follow;
        break;
    case 3:
        m_mode_cur = &m_mode_stay;
        break;
    case 4:
        m_mode_cur = &m_mode_find;
        break;
    case 5:
        m_mode_cur = &m_mode_find_end;
        break;
    case 6:
        m_mode_cur = &m_mode_move;
        break;
    case 7:
        m_mode_cur = &m_mode_move_end;
        break;
    case 8:
        m_mode_cur = &m_mode_rot;
        break;
    case 9:
        m_mode_cur = &m_mode_rot_end;
        break;
    case 10:
        m_mode_cur = &m_mode_damage;
        break;
    case 11:
        m_mode_cur = &m_mode_bark;
        break;
    case 12:
        m_mode_cur = &m_mode_hug;
        break;
    case 13:
        m_mode_cur = &m_mode_wound;
        break;
    default:
        m_mode_cur = NULL;
        ROR_ASSERT(0); /* 3056 */  // callcheck: FUN_00215a30 (the assert handler)
        break;
    }
    m_mode_count = 0;
    if (m_mode_cur == NULL)
        return;
    /* PC: the ROM sets the neck control through m_bwn unchecked. */
    if (m_bwn != NULL)
        m_bwn->setNeckControl(state == 1 ? 2 : 0);
    m_mode_cur->init(*this); /* EE slot 1 */
}

// ROR 0x0012a56c
void MagLifeBwnCtrl::requestState(int theMode, int arg, int force)
{
    ROR_ASSERT(theMode >= 0); /* 3083 */  // callcheck: FUN_00215a30 (the assert handler)
    utlSetBwnObjFlag(1);
    /* PC: the ROM asks m_bwn's floor even while Brown is unbound (a read
     * through NULL on the EE); unbound counts as no flagged cell. */
    if (theMode != 13 && m_bwn != NULL && m_bwn->testFloor(0.0f, NULL) >= 1)
        return;
    if (m_state == theMode && force == 0)
        return;
    m_state = theMode;
    m_08 = arg;
    m_04 = -1;
    applyMode();
}

// ROR 0x0012a61c
void MagLifeBwnCtrl::live()
{
    if (m_ctrl_level == MAG_BWNCTRL_LEVEL_MANUAL)
        return;
    if (m_mode_cur == NULL)
    {
        requestState(0, 0, 1);
        if (m_mode_cur == NULL)
            return;
    }
    m_mode_count++;
    m_mode_cur->live(*this); /* EE slot 2 */
}

// ROR 0x0012a6a8
void MagLifeBwnCtrl::requestHurt(int react)
{
    requestState(10, react, 0);
}

// ROR 0x0012a6cc
int MagLifeBwnCtrl::isHurt()
{
    return m_state == 10;
}

// ROR 0x0012a6dc
int MagLifeBwnCtrl::barkQuery(int v)
{
    if (m_state == 11)
        return m_mode_bark.query(*this, v);
    return -1;
}

// ROR 0x0012a71c
void MagLifeBwnCtrl::barkSet(int v)
{
    if (m_state == 11)
        m_mode_bark.set(*this, v);
}

// ROR 0x0012a758
void MagLifeBwnCtrl::setFindItem(int item, int request, int again)
{
    m_find_item = item;
    if (item >= 0)
        utlFindSetItem(&m_find, item, again);
    if (request != 0)
    {
        requestState(4, 0, 1);
        return;
    }
    m_mission = MISSION_NONE;
}

// ROR 0x0012a7cc
void MagLifeBwnCtrl::cancelFind()
{
    if ((u_int)(m_state - 4) < 2)
    {
        m_mission = MISSION_NONE;
        requestState(1, 0, 1);
    }
}

// ROR 0x0012a808
void MagLifeBwnCtrl::setPos(const float *v)
{
    if (m_bwn == NULL || m_bwn->m_actor == NULL) /* EE slot 11 getActor */
        return;
    DogActor *actor = m_bwn->m_actor;
    actor->m_trans[0] = v[0];
    actor->m_trans[1] = v[1];
    actor->m_trans[2] = v[2];
    actor->m_trans[3] = v[3];
    actor->m_dirty = 1;
    requestState(0, 0, 0);
}

// ROR 0x0012a89c
void MagLifeBwnCtrl::setRot(const float *v)
{
    if (m_bwn == NULL || m_bwn->m_actor == NULL) /* EE slot 11 getActor */
        return;
    DogActor *actor = m_bwn->m_actor;
    actor->m_rotate[0] = v[0];
    actor->m_rotate[1] = v[1];
    actor->m_rotate[2] = v[2];
    actor->m_rotate[3] = v[3];
    actor->normalizeRotate();
    actor->m_dirty = 1;
    actor->m_base_rotate[0] = v[0];
    actor->m_base_rotate[1] = v[1];
    actor->m_base_rotate[2] = v[2];
    actor->m_base_rotate[3] = v[3];
    actor->setAddRotY(0.0f);
    requestState(0, 0, 0);
}

// ROR 0x0012a974
void MagLifeBwnCtrl::getPos(float *out)
{
    if (m_bwn == NULL || m_bwn->m_actor == NULL) /* EE slot 11 getActor */
        return;
    DogActor *actor = m_bwn->m_actor;
    out[0] = actor->m_trans[0];
    out[1] = actor->m_trans[1];
    out[2] = actor->m_trans[2];
    out[3] = actor->m_trans[3];
}

// ROR 0x0012a9ec
void MagLifeBwnCtrl::getRot(float *out)
{
    if (m_bwn == NULL || m_bwn->m_actor == NULL) /* EE slot 11 getActor */
        return;
    DogActor *actor = m_bwn->m_actor;
    out[0] = actor->m_rotate[0];
    out[1] = actor->m_rotate[1];
    out[2] = actor->m_rotate[2];
    out[3] = actor->m_rotate[3];
    out[1] = actor->getRotY();
}

// ROR 0x0012aa70
void MagLifeBwnCtrl::moveTo(const float *xyz)
{
    m_target[0] = xyz[0];
    m_target[3] = 1.0f;
    m_target[1] = xyz[1];
    m_target[2] = xyz[2];
    requestState(6, 0, 0);
}

// ROR 0x0012aac0
void MagLifeBwnCtrl::setAngle(float a)
{
    m_angle = a;
    requestState(0, 0, 0);
}

// ROR 0x0012aae8
void MagLifeBwnCtrl::rotateTo(float a)
{
    m_angle = a;
    requestState(8, 0, 0);
}

// ROR 0x0012ab10
int MagLifeBwnCtrl::isStateDone()
{
    return m_state_done;
}

// ROR 0x0012ab20
int MagLifeBwnCtrl::findItemAhead()
{
    UtlEmyInfoBuf eib;
    clearEmyInfoBuf(eib);
    int num = utlCollectItemInfo(&eib, 1);
    if (num <= 0)
        return 0;
    ROR_ASSERT(m_pc); /* 3511 */  // callcheck: FUN_00215a30 (the assert handler)
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pc->getPosition(pos);
    float best = 1e+06f;
    float fwd[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    utlVecRotMatY(m_pc->getRotY(), fwd);
    int found = -1;
    for (int i = 0; i < num; i++)
    {
        UtlEmyInfo &e = eib.m_info[i];
        float dy = e.m_pos[1] - pos[1];
        if (!(-100.0f < dy && dy < 100.0f))
            continue;
        float dir[4] = { e.m_pos[0] - pos[0], e.m_pos[1] - pos[1], e.m_pos[2] - pos[2], 1.0f };
        utlNormalize3(dir);
        float dv = utlVecDot(dir, fwd);
        if (dv < 0.7f)
            continue;
        ROR_ASSERT(dv > 0.0f && dv <= 1.0f); /* 3536 */  // callcheck: FUN_00215a30 (the assert handler)
        float dist = utlVecDist(e.m_pos, pos);
        e.m_dist = dist;
        float w = dist / dv;
        if (w < best)
        {
            best = w;
            found = i;
        }
    }
    if (found < 0)
        return 0;
    UtlEmyInfo &e = eib.m_info[found];
    m_target[0] = e.m_pos[0];
    m_target[1] = e.m_pos[1];
    m_target[2] = e.m_pos[2];
    m_target[3] = e.m_pos[3];
    m_found_obj = e.m_obj_id;
    m_found_oid = e.m_24;
    return 1;
}

// ROR 0x0012adec
int MagLifeBwnCtrl::findFloorAhead()
{
    if (m_pc == NULL)
        return 0;
    int result = 0;
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pc->getPosition(pos);
    float dir[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    utlVecRotMatY(m_pc->getRotY(), dir);
    for (int i = 1; i <= 10; i++)
    {
        float d = (float)i * 300.0f;
        float p[4] = { dir[0] * d + pos[0], dir[1] * d + pos[1], dir[2] * d + pos[2], 1.0f };
        GamSecta *hit[4];
        FUN_0017c340(&m_scene->m_8542c, p, hit, 0, 100.0f, 0.0f);
        if (hit[0] == NULL)
            return result;
        result = 1;
        m_target[0] = p[0];
        m_target[1] = p[1];
        m_target[2] = p[2];
        m_target[3] = p[3];
    }
    return result;
}

// ROR 0x0012af70
int MagLifeBwnCtrl::isEmyAheadOfPC()
{
    if (m_bwn == NULL || m_bwn->getHealthStatus() >= 3)
        return 0;
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    UtlEmyInfoBuf eib;
    clearEmyInfoBuf(eib);
    if (m_pc == NULL)
        return 0;
    m_pc->getPosition(pos);
    float ahead[4] = { 0.0f, 0.0f, 500.0f, 1.0f };
    utlVecRotMatY(m_pc->getRotY(), ahead);
    pos[0] += ahead[0];
    pos[2] += ahead[2];
    pos[1] += ahead[1];
    pos[3] = 1.0f;
    return utlCollectEmyInfoAt(&eib, pos, 0, 1, 0.0f, 500.0f) > 0;
}

// ROR 0x0012b0d0
int MagLifeBwnCtrl::isEmyNearPC()
{
    if (m_bwn == NULL || m_bwn->getHealthStatus() >= 3 || m_pc == NULL)
        return 0;
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pc->getPosition(pos);
    UtlEmyInfoBuf eib;
    clearEmyInfoBuf(eib);
    return utlCollectEmyInfoAt(&eib, pos, 0, 1, 0.0f, 500.0f) > 0;
}

// ROR 0x0012b19c
int MagLifeBwnCtrl::aimAtPC()
{
    float pc[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pc->getPosition(pc);
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_bwn->getPosition(pos);
    float dir[4] = { pc[0] - pos[0], pc[1] - pos[1], pc[2] - pos[2], 1.0f };
    float rot = utlVecToRotY(dir);
    float d = wrapAngle(m_bwn->getRotY() - rot);
    if (-0.052359883f <= d && d <= 0.052359883f)
        return 0;
    m_angle = rot;
    return 1;
}

// ROR 0x0012b300
int MagLifeBwnCtrl::isFarFromPC()
{
    float pc[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pc->getPosition(pc);
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_bwn->getPosition(pos);
    float dy = pos[1] - pc[1];
    if (-100.0f <= dy && dy <= 100.0f && utlVecDist(pos, pc) < 130.0f)
        return 0;
    return 1;
}

// ROR 0x0012b3d0
int MagLifeBwnCtrl::isTooNearPC()
{
    float pc[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pc->getPosition(pc);
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_bwn->getPosition(pos);
    float dy = pos[1] - pc[1];
    if (-100.0f <= dy && dy <= 100.0f && 95.0f <= utlVecDist(pos, pc))
        return 0;
    return 1;
}

// Come: Brown follows when far, turns to the PC, backs off when too near,
// else answers with an action.
// ROR 0x0012b4a0
void MagLifeBwnCtrl::orderCome(int a1)
{
    if (m_bwn == NULL || m_bwn->getHealthStatus() >= 4)
        return;
    m_bwn->playSe(0xb);
    m_mission = MISSION_NONE;
    if (isFarFromPC())
    {
        requestState(2, 0, 1);
        return;
    }
    if (aimAtPC())
    {
        requestState(8, 0, 0);
        return;
    }
    if (isTooNearPC())
    {
        m_bwn->message(1, 0x44, 1); /* EE slot 14 */
        return;
    }
    m_bwn->m_state = -1;
    m_bwn->m_04 = -1;
    int actions[6];
    for (int i = 0; i < 6; i++)
        actions[i] = s_comeActions[i];
    int r = utlRandRange(0, 5);
    m_bwn->message(1, actions[r], 1);
}

// ROR 0x0012b5f8
void MagLifeBwnCtrl::orderStay(int a1)
{
    if (m_bwn == NULL || m_bwn->getHealthStatus() >= 4)
        return;
    if (m_bwn->testFloor(0.0f, NULL) == 0)
        requestState(3, 0, 0);
}

// Go: the pending find first, then a bark at enemies ahead of / around the
// PC, an item ahead (find), floor ahead (move there), else follow.
// ROR 0x0012b65c
int MagLifeBwnCtrl::orderGo(int a1)
{
    if (m_bwn == NULL)
        return 0;
    if (m_bwn->getHealthStatus() > 3)
        return 0;
    m_mission = MISSION_NONE;
    if (utlFindHasAny(&m_find))
    {
        requestState(4, 0, 1);
        return 1;
    }
    int state;
    if (isEmyAheadOfPC())
    {
        m_bwn->playSe(0xb);
        state = 11;
    }
    else if (isEmyNearPC())
    {
        m_bwn->playSe(0xb);
        state = 11;
    }
    else if (findItemAhead())
    {
        requestState(4, 1, 1);
        return 1;
    }
    else if (findFloorAhead())
    {
        m_bwn->playSe(0xb);
        state = 6;
    }
    else
    {
        m_bwn->playSe(0xb);
        m_mission = MISSION_NONE;
        state = 2;
    }
    requestState(state, 0, 0);
    return 1;
}

// ROR 0x0012b788
void MagLifeBwnCtrl::requestHug(int arg)
{
    requestState(12, arg, 1);
    if (arg >= 0)
        m_mode_hug.setMode(arg);
}

// ROR 0x0012b7cc
void MagLifeBwnCtrl::setLevel(int theLevel)
{
    ROR_ASSERT(theLevel >= MAG_BWNCTRL_LEVEL_AUTO && theLevel <= MAG_BWNCTRL_LEVEL_MANUAL); /* 3922 */  // callcheck: FUN_00215a30 (the assert handler)
    if (m_ctrl_level == theLevel)
        return;
    m_ctrl_level = theLevel;
    if (theLevel == MAG_BWNCTRL_LEVEL_AUTO)
    {
        /* PC: the ROM resumes through m_bwn unchecked. */
        if (m_bwn != NULL)
            m_bwn->resume();
        requestState(0, 0, 1);
    }
    else if (theLevel == MAG_BWNCTRL_LEVEL_MANUAL && m_bwn != NULL)
        m_bwn->suspend();
}

// ROR 0x0012b870
int MagLifeBwnCtrl::checkHug(int phase)
{
    if (m_bwn == NULL)
        return -3;
    if (m_state == 3)
        return -2;
    if (phase == 0)
    {
        if (isFarFromPC())
            return 0;
        return aimAtPC() == 0 ? 1 : 0;
    }
    if ((u_int)(phase - 1) >= 2)
        return -1;
    float pc[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pc->getPosition(pc);
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_bwn->getPosition(pos);
    float rot[4];
    float d = utlVecDistRotY(pos, rot, pc) - 100.0f;
    if (!(-1.0f <= d && d <= 1.0f))
        return 0;
    float a = wrapAngle(rot[0] - m_pc->getRotY());
    if (-3.1101768f <= a && a <= 3.1101768f)
        return 0;
    return 1;
}

// ROR 0x0012ba2c
void MagLifeBwnCtrl::getBwnPosition(float *out)
{
    if (m_bwn != NULL)
        m_bwn->getPosition(out);
}

// ROR 0x0012ba54
int MagLifeBwnCtrl::isBarkPhase3()
{
    return m_state == 11 && m_08 == 3;
}

// ROR 0x0012ba80
MagLifeBwnCtrl::MagLifeBwnCtrl()
{
    /* MagLife0 / MagLife */
    m_level = 1;
    m_state = 0;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_04 = -1;
    m_state = 0;
    m_hide = 0;
    m_scene = NULL;

    m_target[3] = 1.0f;
    m_target[2] = 0.0f;
    m_target[1] = 0.0f;
    m_target[0] = 0.0f;
    m_angle = 0.0f;
    m_find_item = 0;
    utlFindClear(&m_find);
    m_60 = 0;
    m_ctrl_level = MAG_BWNCTRL_LEVEL_AUTO;
    m_bwn = NULL;
    m_pc = NULL;
    m_mission = MISSION_NONE;
    m_find_result = 0;
    m_state_done = 0;
    m_mode_cur = NULL;
    m_mode_find.m_pos[3] = 1.0f;
    m_mode_find.m_pos[2] = 0.0f;
    m_mode_find.m_pos[1] = 0.0f;
    m_mode_find.m_pos[0] = 0.0f;
    m_mode_bark.m_pos[3] = 1.0f;
    m_mode_bark.m_pos[0] = 0.0f;
    m_mode_bark.m_pos[2] = 0.0f;
    m_mode_bark.m_pos[1] = 0.0f;
    utlFindClear(&m_find);
}

// ROR 0x00283d54
void MagLifeBwnCtrl::setMission(int m)
{
    ROR_ASSERT(m >= MISSION_NONE && m <= MISSION_HUG); /* mag/maglifebwnctrl.h 385 */  // callcheck: FUN_00215a30 (the assert handler)
    m_mission = m;
}

// ---- stubs (not reconstructed yet) ----

// stub of ROR 0x00127150
void MagLifeBwnCtrlModeFollow::live(MagLifeBwnCtrl &theBwnCtrl)
{
    ROSEPAN_STUB("MagLifeBwnCtrlModeFollow::live", 0x00127150);
}

// stub of ROR 0x00127b28
void MagLifeBwnCtrlModeFind::init(MagLifeBwnCtrl &theBwnCtrl)
{
    ROSEPAN_STUB("MagLifeBwnCtrlModeFind::init", 0x00127b28);
}

// stub of ROR 0x00127e68
void MagLifeBwnCtrlModeFind::live(MagLifeBwnCtrl &theBwnCtrl)
{
    ROSEPAN_STUB("MagLifeBwnCtrlModeFind::live", 0x00127e68);
}

// stub of ROR 0x00128970
void MagLifeBwnCtrlModeFindEnd::live(MagLifeBwnCtrl &theBwnCtrl)
{
    ROSEPAN_STUB("MagLifeBwnCtrlModeFindEnd::live", 0x00128970);
}

// stub of ROR 0x00129028
void MagLifeBwnCtrlModeBark::live(MagLifeBwnCtrl &theBwnCtrl)
{
    ROSEPAN_STUB("MagLifeBwnCtrlModeBark::live", 0x00129028);
}
