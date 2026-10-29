/* ==========================================================================
 *  mag/maglifebwnctrl.cpp
 *
 *  MagLifeBwnCtrl, Brown's command layer: the state requests over the
 *  embedded mode objects (requestState / applyMode / live), the script
 *  natives' setters (position, rotation, find, bark, level), the PC's dog
 *  orders (come / stay / go, with the tests they make around the PC) and
 *  the hug check; the modes: Null (picks the next state), Wait (idles,
 *  follows, barks), Follow, Stay, Find (to the item, through the doors on
 *  the way to its room) / FindEnd, Move / MoveEnd, Rot / RotEnd, Damage,
 *  Bark (to the nearest enemy, the bite, the bark and its answer), Hug and
 *  Wound.
 *  0x1267c0..0x12bc20 (pinned 2026-10-28; the asserts name this file).
 * ======================================================================== */

#include "mag/maglifebwnctrl.h"

#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "lib/unassigned.h"
#include "mag/maglifebwn.h"
#include "mag/maglifepc.h"
#include "mag/magscene.h"
#include "utl/utlgame.h"
#include "utl/utlmath.h"
#include "wrd/wrdsrv.h"

#include <string.h>

/* The angle wrapped into -pi .. pi (one step, as the ROM does). */
static inline float wrapAngle(float a)
{
    if (a < -DOG_PI)
        a += 6.283186f;
    else if (DOG_PI <= a)
        a -= 6.283186f;
    return a;
}

/* MagLifeBwn::message's int argument carrying a float's bits (mode 8, the
 * rotate's heading: the ROM passes the float in a GPR). */
static inline int floatArg(float f)
{
    int i;
    memcpy(&i, &f, sizeof i);
    return i;
}

/* The collectors' buffer as the ROM's callers set it up: every entry's
 * two points (0, 0, 0, 1), no count. */
static void clearEmyInfoBuf(UtlEnemyInfoBuf &eib)
{
    for (int i = 0; i < 32; i++)
    {
        UtlEnemyInfo &e = eib.m_info[i];
        e.m_pos[2] = 0.0f;
        e.m_pos[1] = 0.0f;
        e.m_pos[0] = 0.0f;
        e.m_pos[3] = 1.0f;
        e.m_trans[3] = 1.0f;
        e.m_trans[2] = 0.0f;
        e.m_trans[1] = 0.0f;
        e.m_trans[0] = 0.0f;
    }
    eib.m_count = 0;
}

/* Brown's idle actions in Wait (MagLifeBwn::message 1's motion). */
// ROR 0x002ebe10
static const int s_waitActions[5] = { 18, 3, 4, 5, 6 };

/* Brown's answer to each Find error (ERR_CANNOT_FIND_ITEM .. ERR_STUCK). */
// ROR 0x002ec028
static const int s_findErrMotion[ERR_END] = { 132, 133, 134, 135, 136, 137, 132 };

/* orderCome's answers when Brown is already at the PC. */
// ROR 0x002ec448
static const int s_comeActions[6] = { 3, 4, 5, 6, 39, 18 };

// ──────────────────────────────────────────────────────────────────────
// MagLifeBwnCtrlMode helpers
// ──────────────────────────────────────────────────────────────────────

// ROR 0x001267c0
int MagLifeBwnCtrlModeFind::isFindTarget(FCHRstat *obj, UtlFind *find)
{
    return utlFindMatchObj(find, obj);
}

// ROR 0x001267d8
FCHRstat *MagLifeBwnCtrlModeFind::findBest(UtlFind *find, int wrd, const float *pos)
{
    return utlFindBest(find, pos, wrd);
}

// ROR 0x001267f4
int MagLifeBwnCtrlMode::countEmyAround(MagLifeBwnCtrl &theBwnCtrl, float radius)
{
    if (theBwnCtrl.m_bwn->getHealthStatus() >= 3)
        return 0;
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    theBwnCtrl.m_bwn->getPosition(pos);
    UtlEnemyInfoBuf eib;
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

// Phase 0 sorts out flagged cells (MagLifeBwn::testFloor): Brown on one
// and the PC not, he paths to the PC; both on one, he waits. Phase 2 walks
// straight at the PC while the line to him is clear (40 wide), phase 4 by
// path; Wait again within 105 / once the path ends.
// ROR 0x00127150
void MagLifeBwnCtrlModeFollow::live(MagLifeBwnCtrl &theBwnCtrl)
{
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float pc[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (theBwnCtrl.m_bwn == NULL)
        return;
    theBwnCtrl.m_bwn->getPosition(pos);
    theBwnCtrl.m_pc->getPosition(pc);
    float dist = utlVecDistXZ(pos, pc);
    int step = m_04 & 1;
    switch (m_04 & 0xfe)
    {
    case 0:
        if (step == 0)
        {
            int pcFloor = theBwnCtrl.m_pc->testFloor(0.0f, NULL);
            if (theBwnCtrl.m_bwn->testFloor(0.0f, NULL) != 0)
            {
                if (pcFloor == 0)
                {
                    if (theBwnCtrl.m_pc->testFloor(50.0f, pc) != 0 &&
                        theBwnCtrl.m_pc->testFloor(-50.0f, pc) != 0)
                        theBwnCtrl.m_pc->getPosition(pc);
                    theBwnCtrl.m_target[0] = pc[0];
                    theBwnCtrl.m_target[1] = pc[1];
                    theBwnCtrl.m_target[2] = pc[2];
                    theBwnCtrl.m_target[3] = pc[3];
                    if (theBwnCtrl.m_bwn->searchPath(pc) >= 0)
                    {
                        theBwnCtrl.m_bwn->message(6, 0, 0); /* EE slot 14: Path */
                        m_04 = 5;
                        return;
                    }
                }
                theBwnCtrl.m_bwn->message(1, 2, 0);
                m_04 = 1;
                return;
            }
            if (pcFloor != 0)
            {
                theBwnCtrl.m_bwn->message(1, 2, 0);
                return;
            }
        }
        else
        {
            if (dist < 105.0f)
            {
                theBwnCtrl.requestState(1, 0, 0);
                return;
            }
            if (theBwnCtrl.m_pc->testFloor(0.0f, NULL) != 0)
                return;
            if (500.0f < dist)
            {
                m_04 = 4;
                return;
            }
            if (dist <= 170.0f)
                return;
        }
        m_04 = 2;
        return;
    case 2:
        if (step == 0)
        {
            theBwnCtrl.m_08 = 0;
            if (theBwnCtrl.m_scene->m_8542c.m_list[0].canWalkStraight(pos, pc, 40.0f) != 0)
            {
                m_04 = 3;
                return;
            }
        }
        else if (theBwnCtrl.m_bwn->m_unmovable_count == 0)
        {
            int pcFloor = theBwnCtrl.m_pc->testFloor(0.0f, NULL);
            if (pcFloor == 0)
            {
                if (500.0f < dist)
                {
                    m_04 = 4;
                    return;
                }
            }
            else if (theBwnCtrl.m_bwn->testFloor(75.0f, NULL) != pcFloor)
            {
                m_04 = 0;
                return;
            }
            followPC(theBwnCtrl);
            return;
        }
        m_04 = 4;
        return;
    case 4:
        if (step == 0)
        {
            theBwnCtrl.m_target[0] = pc[0];
            theBwnCtrl.m_target[1] = pc[1];
            theBwnCtrl.m_target[2] = pc[2];
            theBwnCtrl.m_target[3] = pc[3];
            if (theBwnCtrl.m_bwn->searchPath(pc) < 0)
            {
                m_04 = 0;
                return;
            }
            theBwnCtrl.m_bwn->m_state = -1;
            theBwnCtrl.m_bwn->m_04 = -1;
            theBwnCtrl.m_bwn->message(6, 4, 0);
            m_04 = 5;
            return;
        }
        if (theBwnCtrl.m_bwn->m_path_end == 0 && theBwnCtrl.m_bwn->m_unmovable_count == 0 &&
            170.0f < dist)
            return;
        theBwnCtrl.requestState(1, 0, 0);
        return;
    default:
        ROR_ASSERT(0); /* 735 */  // callcheck: dbgAssertFailed (the assert handler)
        return;
    }
}

// Step 0 picks: Wait within 105, else a walk (MagLifeBwn mode 5: 0
// straight, 1 / 4 bearing left / right within 20 degrees) or a turn by
// sixths of pi (mode 7). Step 1 walks until the PC is near, the heading
// drifts past 60 degrees (36 when farther than 157.5) once a motion step
// ended, or Brown leaves the walk (his state 5); step 2 turns until he
// faces the PC within pi/80 (pi/40 while moving).
// ROR 0x00127558
void MagLifeBwnCtrlModeFollow::followPC(MagLifeBwnCtrl &theBwnCtrl)
{
    if (theBwnCtrl.m_bwn == NULL)
        return;
    int still = theBwnCtrl.m_bwn->m_move_result == 0;
    if (still)
        theBwnCtrl.m_08 = 0;
    float reach = 0.0f;
    int next = 0;
    float tol = 0.0f;
    float to[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (theBwnCtrl.m_60 == 0)
    {
        if (theBwnCtrl.m_pc == NULL)
            return;
        reach = 105.0f;
        next = 1;
        theBwnCtrl.m_pc->getPosition(theBwnCtrl.m_target);
        to[0] = theBwnCtrl.m_target[0];
        to[1] = theBwnCtrl.m_target[1];
        to[2] = theBwnCtrl.m_target[2];
        to[3] = theBwnCtrl.m_target[3];
        tol = still ? 0.03926991f : 0.07853982f;
    }
    else
        ROR_ASSERT(0); /* 789 */  // callcheck: dbgAssertFailed (the assert handler)
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    theBwnCtrl.m_bwn->getPosition(pos);
    float dist = utlVecDistXZ(pos, to);
    to[0] -= pos[0];
    to[2] -= pos[2];
    to[3] = 1.0f;
    to[1] -= pos[1];
    float d = wrapAngle(utlVecToRotY(to) - theBwnCtrl.m_bwn->getRotY());
    MagLifeBwn *bwn = theBwnCtrl.m_bwn;
    int bwnState = bwn->m_state;
    switch (theBwnCtrl.m_08)
    {
    case 0:
    {
        if (dist < reach)
        {
            theBwnCtrl.requestState(next, 0, 0);
            return;
        }
        int walk;
        if (-tol < d && d < tol)
            walk = 0;
        else if (-0.34906587f < d && d < 0.0f)
            walk = 1;
        else if (0.0f < d && d < 0.34906587f)
            walk = 4;
        else
        {
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
            bwn->message(7, 0, turn); /* EE slot 14 */
            theBwnCtrl.m_08 = 2;
            return;
        }
        bwn->message(5, 0, walk);
        theBwnCtrl.m_08 = 1;
        return;
    }
    case 1:
        if (dist < reach)
        {
            theBwnCtrl.m_08 = 0;
            return;
        }
        if (bwn->m_actor->m_motion_result != 0) /* EE slot 11 getActor */
        {
            if (d < -1.0471976f || 1.0471976f < d)
            {
                theBwnCtrl.m_08 = 0;
                return;
            }
            if (reach * 1.5f < dist && (d < -0.62831855f || 0.62831855f < d))
            {
                theBwnCtrl.m_08 = 0;
                return;
            }
        }
        if (bwnState != 5)
            theBwnCtrl.m_08 = 0;
        return;
    case 2:
        if (-tol < d && d < tol)
        {
            bwn->message(5, 0, 0);
            theBwnCtrl.m_08 = 1;
        }
        if (theBwnCtrl.m_bwn->m_state == 0)
            theBwnCtrl.m_08 = 0;
        return;
    }
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

/* MagLifeBwn::searchPath's failures as Find errors. */
static int findErrOfPath(int r)
{
    if (r == -1)
        return ERR_NO_PATH;
    if (r == -2)
        return ERR_NO_CELL;
    return ERR_STUCK;
}

// Arg 0 (the controller's m_08) picks the item (utlFindBest from Brown's
// world position): in this room Brown paths to it (m_0c -1 / -2: phase 0);
// in another the first door on the route there (phase 2). Arg 1 (orderGo's
// findItemAhead) paths to m_target. Failures leave the error in m_0c and
// phase 12.
// ROR 0x00127b28
void MagLifeBwnCtrlModeFind::init(MagLifeBwnCtrl &theBwnCtrl)
{
    theBwnCtrl.m_mission = MISSION_FIND;
    theBwnCtrl.m_find_result = 0;
    int r;
    if (theBwnCtrl.m_08 == 0)
    {
        theBwnCtrl.m_state_done = 0;
        theBwnCtrl.m_found_obj = 0;
        theBwnCtrl.m_found_oid = -1;
        int wrd = utlGetWrdNo();
        float world[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        const float *t = theBwnCtrl.m_bwn->getTransPtr();
        float local[4] = { t[0], t[1], t[2], t[3] };
        g_wrdSrv.cnvPosL2W(world, wrd, local);
        FCHRstat *obj = findBest(&theBwnCtrl.m_find, wrd, world);
        if (obj == NULL)
        {
            theBwnCtrl.m_0c = ERR_CANNOT_FIND_ITEM;
            theBwnCtrl.m_08 = 12;
            return;
        }
        int objWrd = utlFindObjWrdNo(obj);
        if (objWrd != wrd)
        {
            WrdRoute areapath;
            if (g_wrdSrv.findRoute(wrd, objWrd, &areapath) == 0)
            {
                theBwnCtrl.m_0c = ERR_NO_ROUTE;
                theBwnCtrl.m_08 = 12;
                return;
            }
            ROR_ASSERT(areapath.m_num >= 2); /* 1147 */  // callcheck: dbgAssertFailed (the assert handler)
            /* (an empty loop over areapath's rooms 1 .. m_num - 1: a debug
             * print compiled out) */
            UtlDoorInfo door;
            door.m_pos[2] = 0.0f;
            door.m_pos[1] = 0.0f;
            door.m_pos[0] = 0.0f;
            door.m_pos[3] = 1.0f;
            if (utlFindDoorBetween(&door, wrd, areapath.m_room[1]->m_id, local) == 0)
            {
                theBwnCtrl.m_0c = ERR_NO_DOOR;
                theBwnCtrl.m_08 = 12;
                return;
            }
            m_pos[0] = door.m_pos[0];
            m_pos[1] = door.m_pos[1];
            m_pos[2] = door.m_pos[2];
            m_pos[3] = door.m_pos[3];
            m_door_kind = (short)door.m_kind;
            m_door_rot = door.m_rot_y;
            m_side = door.m_room[0] == wrd;
            m_side_to = door.m_room[0] == wrd;
            theBwnCtrl.m_08 = 2;
            return;
        }
        theBwnCtrl.m_found_obj = utlFindObjId(obj);
        theBwnCtrl.m_found_oid = utlFindObjUid(obj);
        utlFindObjPos(obj, theBwnCtrl.m_target);
        r = theBwnCtrl.m_bwn->searchPath(theBwnCtrl.m_target);
        if (r < 0)
        {
            theBwnCtrl.m_0c = findErrOfPath(r);
            theBwnCtrl.m_08 = 12;
            return;
        }
        theBwnCtrl.m_bwn->playSe(0xb);
        theBwnCtrl.m_bwn->message(6, 3, 0); /* EE slot 14: Path */
        theBwnCtrl.m_0c = r != 0 ? -1 : -2;
    }
    else
    {
        r = theBwnCtrl.m_bwn->searchPath(theBwnCtrl.m_target);
        if (r < 0)
        {
            theBwnCtrl.m_0c = findErrOfPath(r);
            theBwnCtrl.m_08 = 12;
            return;
        }
        theBwnCtrl.m_bwn->playSe(0xb);
        theBwnCtrl.m_bwn->message(6, 3, 0);
        theBwnCtrl.m_0c = -1;
    }
    theBwnCtrl.m_08 = 0;
}

// The controller's m_08 is phase << 1 | step:
//  0 path to the item: within 100 (y -150 .. 50 ignored) turn to it (8);
//    the path's end short of it: sniff (motion 0x26) and wait (16);
//  2 a door on the way: path to 95 before / behind it (3), at it: 6;
//  4 a sniff (motion 0x42) between door tries, back to 2;
//  6 turn to the door's heading (7), then 10 with the door's kind;
//  8 turn to the item (9), then 10 with -1;
// 10 the result in m_0c: < 0 FindEnd; 0 through the door (14, motion
//    0x1f); else Wait's motion and wait (16);
// 12 give up (m_0c the error): Brown's answer, then Wait or Follow;
// 14 slide through the door to 95 past it (1 a frame), 15 once there;
// 16.. wait out 1800 frames. 30 frames without a move give up (ERR_STUCK).
// ROR 0x00127e68
void MagLifeBwnCtrlModeFind::live(MagLifeBwnCtrl &theBwnCtrl)
{
    float d[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    theBwnCtrl.m_bwn->getPosition(pos);
    int step = theBwnCtrl.m_08 & 1;
    MagLifeBwn *bwn;
    switch (theBwnCtrl.m_08 & 0xfe)
    {
    case 0:
    {
        float to[4] = { theBwnCtrl.m_target[0] - pos[0], theBwnCtrl.m_target[1] - pos[1],
                        theBwnCtrl.m_target[2] - pos[2], 1.0f };
        if (-150.0f <= to[1] && to[1] <= 50.0f)
            to[1] = 0.0f;
        if (utlVecLength(to) < 100.0f)
        {
            theBwnCtrl.m_08 = 8;
            return;
        }
        bwn = theBwnCtrl.m_bwn;
        if (bwn->m_path_end != 0)
        {
            bwn->message(1, 0x26, 0); /* EE slot 14 */
            theBwnCtrl.m_08 = 16;
            break;
        }
        goto stuck;
    }
    case 2:
        if (step == 0)
        {
            theBwnCtrl.m_bwn->playSe(0xb);
            m_side &= 1;
            if (searchDoorSide(theBwnCtrl, theBwnCtrl.m_target, m_side) < 0)
            {
                if (searchDoorSide(theBwnCtrl, theBwnCtrl.m_target, m_side != 1) < 0)
                {
                    theBwnCtrl.m_0c = ERR_DOOR_SIDE;
                    theBwnCtrl.m_08 = 12;
                    return;
                }
                m_side_to = m_side ^ 1;
            }
            else
                m_side_to = m_side;
            if (isAtDoor(pos) == 0)
            {
                theBwnCtrl.m_bwn->message(6, 3, 0);
                theBwnCtrl.m_08 = 3;
                return;
            }
            theBwnCtrl.m_08 = 6;
            return;
        }
        if (isAtDoor(pos) != 0 || theBwnCtrl.m_bwn->m_path_end != 0)
        {
            theBwnCtrl.m_08 = 6;
            return;
        }
        bwn = theBwnCtrl.m_bwn;
        goto stuck;
    case 4:
        bwn = theBwnCtrl.m_bwn;
        if (step == 0)
        {
            bwn->message(1, 0x42, 0);
            theBwnCtrl.m_08 = 5;
        }
        else if (bwn->m_path_end != 0 || 30 < bwn->m_unmovable_count || bwn->isMotionSeqEnd())
            theBwnCtrl.m_08 = 2;
        break;
    case 6:
        bwn = theBwnCtrl.m_bwn;
        if (step != 0)
        {
            if (bwn->m_path_end != 0)
            {
                theBwnCtrl.m_0c = m_door_kind;
                theBwnCtrl.m_08 = 10;
                break;
            }
            if (30 < bwn->m_unmovable_count || bwn->getMotionSeq() == 2)
                theBwnCtrl.m_08 = 4;
            break;
        }
        {
            float rot = m_door_rot;
            if (m_side != 0)
                rot -= DOG_PI;
            rot = wrapAngle(rot);
            bwn->message(8, floatArg(rot), 0); /* the rotate's heading */
        }
        theBwnCtrl.m_08 = 7;
        break;
    case 8:
        if (step == 0)
        {
            d[0] = theBwnCtrl.m_target[0] - pos[0];
            d[3] = 1.0f;
            d[1] = theBwnCtrl.m_target[1] - pos[1];
            d[2] = theBwnCtrl.m_target[2] - pos[2];
            theBwnCtrl.m_bwn->message(8, floatArg(utlVecToRotY(d)), 0);
            theBwnCtrl.m_08 = 9;
            break;
        }
        bwn = theBwnCtrl.m_bwn;
        if (bwn->m_path_end != 0)
        {
            theBwnCtrl.m_0c = -1;
            theBwnCtrl.m_08 = 10;
            break;
        }
    stuck:
        if (30 < bwn->m_unmovable_count)
        {
            theBwnCtrl.m_0c = ERR_STUCK;
            theBwnCtrl.m_08 = 12;
            return;
        }
        break;
    case 10:
        bwn = theBwnCtrl.m_bwn;
        bwn->m_state = -1;
        bwn->m_04 = -1;
        if (theBwnCtrl.m_0c == 0)
        {
            theBwnCtrl.m_bwn->setNeckControl(0);
            theBwnCtrl.m_bwn->clearNeckMotion();
            theBwnCtrl.m_bwn->message(1, 0x1f, 0);
            theBwnCtrl.m_08 = 14;
        }
        else if (theBwnCtrl.m_0c < 0)
            theBwnCtrl.requestState(5, 0, 0);
        else
        {
            theBwnCtrl.m_bwn->message(1, 2, 0);
            theBwnCtrl.m_08 = 16;
        }
        theBwnCtrl.m_mode_count = 0;
        break;
    case 12:
        bwn = theBwnCtrl.m_bwn;
        if (step == 0)
        {
            bwn->playSe(8);
            theBwnCtrl.m_find_result = -1;
            theBwnCtrl.m_mission = MISSION_NONE;
            u_int work2 = (u_int)theBwnCtrl.m_0c;
            ROR_ASSERT(ERR_CANNOT_FIND_ITEM <= work2 && work2 < ERR_END); /* 1521 */  // callcheck: dbgAssertFailed (the assert handler)
            theBwnCtrl.m_bwn->m_state = -1;
            theBwnCtrl.m_bwn->m_04 = -1;
            theBwnCtrl.m_bwn->message(1, s_findErrMotion[work2], 1);
            theBwnCtrl.m_08 = 13;
            break;
        }
        {
            int state = 1;
            if (bwn->m_path_end == 0 && theBwnCtrl.m_mode_count < 901)
            {
                if (bwn->getMotionSeq() != 2)
                {
                    float pc[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                    theBwnCtrl.m_pc->getPosition(pc);
                    if (utlVecDist(pos, pc) <= 300.0f)
                        break;
                    state = 2;
                }
            }
            theBwnCtrl.requestState(state, 0, 0);
        }
        break;
    case 14:
        if (step == 0)
        {
            d[0] = pos[0] - m_pos[0];
            d[3] = 1.0f;
            d[1] = pos[1] - m_pos[1];
            d[2] = pos[2] - m_pos[2];
            utlVecRotMatY(-m_door_rot, d);
            float move = (m_side_to == 0 ? -95.0f : 95.0f) - d[2];
            if (move < -1.0f)
                move = -1.0f;
            if (1.0f < move)
                move = 1.0f;
            d[0] = 0.0f;
            d[1] = 0.0f;
            d[2] = move;
            d[3] = 1.0f;
            utlVecRotMatY(m_door_rot, d);
            DogActor *actor = theBwnCtrl.m_bwn->m_actor; /* EE slot 11 getActor */
            actor->m_trans[0] += d[0];
            actor->m_trans[1] += d[1];
            actor->m_trans[3] = 1.0f;
            actor->m_trans[2] += d[2];
            actor->m_dirty = 1;
            if (move != 1.0f && move != -1.0f)
                theBwnCtrl.m_08 = 15;
        }
        if (theBwnCtrl.m_mode_count < 1801)
            break;
        theBwnCtrl.m_mission = MISSION_NONE;
        theBwnCtrl.requestState(1, 0, 0);
        break;
    default:
        if (1800 < theBwnCtrl.m_mode_count)
        {
            theBwnCtrl.m_mission = MISSION_NONE;
            theBwnCtrl.requestState(1, 0, 0);
        }
        break;
    }
    /* The sniff's footfalls (motion sequence 0x20, frames 0 and 10). */
    if (theBwnCtrl.m_bwn->getMotionSeq() != 0x20)
        return;
    float f = theBwnCtrl.m_bwn->m_actor->getMotionFrame(); /* EE slot 11 getActor */
    if ((0.0f <= f && f < 1.0f) || (10.0f <= f && f < 11.0f))
        theBwnCtrl.m_bwn->playSe(0x13);
}

// ROR 0x00128724
int MagLifeBwnCtrlModeFind::isAtDoor(const float *pos)
{
    float d[4] = { pos[0] - m_pos[0], pos[1] - m_pos[1], pos[2] - m_pos[2], 1.0f };
    if (d[1] < -50.0f || 50.0f < d[1])
        return 0;
    utlVecRotMatY(-m_door_rot, d);
    if (d[0] < -30.0f || 30.0f < d[0])
        return 0;
    if (m_side_to != 0)
        return 76.0f < d[2] && d[2] <= 100.0f;
    return -100.0f <= d[2] && d[2] < -76.0f;
}

// ROR 0x00128854
int MagLifeBwnCtrlModeFind::searchDoorSide(MagLifeBwnCtrl &theBwnCtrl, float *out, int side)
{
    out[2] = side != 0 ? 95.0f : -95.0f;
    out[3] = 1.0f;
    out[0] = 0.0f;
    out[1] = 0.0f;
    utlVecRotMatY(m_door_rot, out);
    out[0] += m_pos[0];
    out[1] += m_pos[1];
    out[3] = 1.0f;
    out[2] += m_pos[2];
    return theBwnCtrl.m_bwn->searchPath(out);
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

// Phase 0 plays the pick-up by the item's height (motion 0x28 from 40
// below Brown up, 0x26 to 80 below, 0x29 lower) and marks the state done;
// phase 1 gives up when the object went, the footfalls at frames 10, 25
// and 42.
// ROR 0x00128970
void MagLifeBwnCtrlModeFindEnd::live(MagLifeBwnCtrl &theBwnCtrl)
{
    if (theBwnCtrl.m_08 == 0)
    {
        float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        theBwnCtrl.m_bwn->getPosition(pos);
        float dy = theBwnCtrl.m_target[1] - pos[1];
        theBwnCtrl.m_bwn->m_state = -1;
        theBwnCtrl.m_bwn->m_04 = -1;
        int motion = 0x26;
        if (-40.0f <= dy)
            motion = 0x28;
        else if (dy <= -80.0f)
            motion = 0x29;
        theBwnCtrl.m_bwn->message(1, motion, 0); /* EE slot 14 */
        theBwnCtrl.m_state_done = 1;
        theBwnCtrl.m_08 = 1;
        return;
    }
    if (theBwnCtrl.m_08 != 1)
    {
        ROR_ASSERT(0); /* 1916 */  // callcheck: dbgAssertFailed (the assert handler)
        return;
    }
    if (utlIsObj(theBwnCtrl.m_found_obj) == 0)
        theBwnCtrl.requestState(1, 0, 0);
    float f = theBwnCtrl.m_bwn->m_actor->getMotionFrame(); /* EE slot 11 getActor */
    if ((10.0f <= f && f < 11.0f) || (25.0f <= f && f < 26.0f) || (42.0f <= f && f < 43.0f))
        theBwnCtrl.m_bwn->playSe(0xb);
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
        ROR_ASSERT(0); /* 2038 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
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
    ROR_ASSERT(!theBwnCtrl.m_bwn->isMotionStateEndless()); /* 2067 */  // callcheck: dbgAssertFailed (the assert handler)
}

// ROR 0x00128e20
void MagLifeBwnCtrlModeBark::init(MagLifeBwnCtrl &theBwnCtrl)
{
    m_target_kind = 0;
    m_bite_target_oid = 0;
    utlSetBwnObjFlag(1);
    if (theBwnCtrl.m_08 == 0)
        theBwnCtrl.m_08 = 0;
    else
        theBwnCtrl.m_08 = 14;
}

// ROR 0x00128e6c
int MagLifeBwnCtrlModeBark::findNearestEmy(MagLifeBwnCtrl &theBwnCtrl, UtlEnemyInfoBuf *eib, int where)
{
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (theBwnCtrl.m_pc == NULL || theBwnCtrl.m_bwn == NULL)
        return 0;
    switch (where)
    {
    case 0:
        theBwnCtrl.m_bwn->getPosition(pos);
        break;
    case 1:
        theBwnCtrl.m_pc->getPosition(pos);
        break;
    case 2:
    {
        theBwnCtrl.m_pc->getPosition(pos);
        float ahead[4] = { 0.0f, 0.0f, 500.0f, 1.0f };
        utlVecRotMatY(theBwnCtrl.m_pc->getRotY(), ahead);
        pos[0] += ahead[0];
        pos[2] += ahead[2];
        pos[3] = 1.0f;
        pos[1] += ahead[1];
        break;
    }
    default:
        ROR_ASSERT(0); /* 2129 */  // callcheck: dbgAssertFailed (the assert handler)
        break;
    }
    int num = utlCollectEmyInfoAt(eib, pos, 0, 1, 0.0f, 500.0f);
    if (num < 1)
        return -1;
    if (num == 1)
        return 0;
    int found = -1;
    float best = 100000.0f;
    for (int i = 0; i < num; i++)
    {
        float dist = eib->m_info[i].m_dist;
        if (dist < best)
        {
            best = dist;
            found = i;
        }
    }
    return found;
}

/* Bark::live's phases (the ROM copies their names, MODE_START ..
 * MODE_MOVING4ENEMY, from 0x2ec230 for a debug print it never makes; the
 * assert's MODE_END is 18). The names here say what each phase does. */
enum
{
    BARK_START = 0,             // the nearest enemy around Brown
    BARK_TURNED = 1,            // the turn at it ended: growl (200 away) or bite
    BARK_BITE = 2,
    BARK_BITING = 3,
    BARK_APPROACH = 4,          // a way to m_pos
    BARK_APPROACHING = 5,
    BARK_FACE = 6,              // turn at the enemy
    BARK_FACING = 7,
    BARK_GROWL = 8,
    BARK_GROWLING = 9,
    BARK_BARK = 10,
    BARK_BARKING = 11,          // barkQuery's phase: Brown keeps facing it, m_bark_dist - 20 away
    BARK_ANSWER = 12,           // barkSet's phase
    BARK_ANSWERING = 13,
    BARK_AHEAD = 14,            // Bark::init with an arg: the enemy ahead of the PC
    BARK_GO = 15,               // a way to an enemy too far to bark at
    BARK_GOING = 16,
    BARK_REST = 17,
    BARK_MODE_END = 18,
};

/* The picked enemy into the mode: its kind, its oid, its position into the
 * controller's m_target. */
static void takeBarkTarget(MagLifeBwnCtrlModeBark &mode, MagLifeBwnCtrl &theBwnCtrl,
                           const UtlEnemyInfo &e)
{
    mode.m_target_kind = e.m_24;
    mode.m_bite_target_oid = e.m_obj_id;
    theBwnCtrl.m_target[0] = e.m_trans[0];
    theBwnCtrl.m_target[1] = e.m_trans[1];
    theBwnCtrl.m_target[2] = e.m_trans[2];
    theBwnCtrl.m_target[3] = e.m_trans[3];
}

// Brown at an enemy: picks the nearest one (around Brown, or ahead of the
// PC when entered with an arg) and goes to m_bark_dist from it (kinds 0x401
// 135, 0x421 110), turns to it and barks, turning and edging to keep the
// distance while barkQuery can ask; barkSet's answer plays 0x71 / 0x79.
// Other kinds get a turn and a growl, or a bite when near, or are walked
// to when 200 away. Health 4: state 13, 3: state 0; a target leaving its
// state ends the bark.
// ROR 0x00129028
void MagLifeBwnCtrlModeBark::live(MagLifeBwnCtrl &theBwnCtrl)
{
    int *work = &theBwnCtrl.m_08;
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    int answer = theBwnCtrl.m_0c;
    MagLifeBwn *bwn = theBwnCtrl.m_bwn;
    bwn->getPosition(pos);
    ROR_ASSERT((u_int)*work < BARK_MODE_END); /* 2191: work >= MODE_START && work < MODE_END */  // callcheck: dbgAssertFailed (the assert handler)

    int health = bwn->getHealthStatus();
    if (health == 4)
    {
        theBwnCtrl.requestState(13, 0, 0);
        return;
    }
    if (health == 3)
    {
        theBwnCtrl.requestState(0, 0, 0);
        return;
    }
    if (m_bite_target_oid != 0 && m_target_kind != 0)
    {
        int gone = 0;
        if (m_target_kind == 0x401)
            gone = utlIsEmyState4(m_bite_target_oid);
        else if (m_target_kind == 0x421)
            gone = utlIsEmyState7(m_bite_target_oid);
        if (gone != 0)
            theBwnCtrl.requestState(0, 0, 0);
    }
    if (*work != BARK_START && *work != BARK_AHEAD && m_bite_target_oid != 0 &&
        utlTestEmyStateFlag(m_bite_target_oid) == 0)
        *work = BARK_START;

    UtlEnemyInfoBuf eib;
    float rot[4];
    float t[4];
    switch (*work)
    {
    case BARK_START:
    {
        clearEmyInfoBuf(eib);
        int i = findNearestEmy(theBwnCtrl, &eib, 0);
        if (i < 0)
        {
            theBwnCtrl.requestState(0, 0, 0);
            return;
        }
        const UtlEnemyInfo &e = eib.m_info[i];
        takeBarkTarget(*this, theBwnCtrl, e);
        int near;
        if (m_target_kind == 0x401)
        {
            near = e.m_dist < 140.0f;
            m_bark_dist = 135.0f;
        }
        else if (m_target_kind != 0x421 || utlGetEmyWord238(m_bite_target_oid) != 0)
        {
            if (200.0f <= utlVecDistRotY(pos, rot, theBwnCtrl.m_target))
            {
                *work = BARK_GO;
                return;
            }
            bwn->message(8, floatArg(rot[0]), 0); /* EE slot 14: turn to it */
            *work = BARK_TURNED;
            return;
        }
        else
        {
            near = e.m_dist < 115.0f;
            m_bark_dist = 110.0f;
        }
        *work = near ? BARK_FACE : BARK_APPROACH;
        break;
    }
    case BARK_TURNED:
    {
        if (bwn->isActionEnd() == 0)
        {
            if (bwn->getMotionSeq() == 2)
                *work = BARK_START;
            return;
        }
        clearEmyInfoBuf(eib);
        int i = findNearestEmy(theBwnCtrl, &eib, 0);
        if (i < 0)
        {
            theBwnCtrl.requestState(0, 0, 0);
            return;
        }
        *work = 200.0f <= eib.m_info[i].m_dist ? BARK_GROWL : BARK_BITE;
        break;
    }
    case BARK_BITE:
        bwn->m_state = -1;
        bwn->m_04 = -1;
        bwn->message(1, 0x2a, 1);
        *work = BARK_BITING;
        break;
    case BARK_BITING:
    {
        if (theBwnCtrl.m_mode_count % 20 == 0)
            bwn->playSe(13);
        if (bwn->isActionEnd() == 0)
            return;
        ROR_ASSERT(m_bite_target_oid != 0); /* 2466 */
        if (utlGetEmyHold(m_bite_target_oid) != 0)
        {
            *work = BARK_START;
            return;
        }
        int kind = utlGetObjKind(m_bite_target_oid);
        if (kind != 0x401 && kind != 0x421)
        {
            if (utlRandRange(0, 10) == 0)
            {
                *work = BARK_REST;
                break;
            }
            if (utlRandRange(0, 10) != 0)
            {
                *work = BARK_BITE;
                return;
            }
        }
        *work = BARK_START;
        break;
    }
    case BARK_APPROACH:
        pos[0] -= theBwnCtrl.m_target[0];
        pos[1] -= theBwnCtrl.m_target[1];
        pos[2] -= theBwnCtrl.m_target[2];
        pos[3] = 1.0f;
        utlNormalize3(pos);
        pos[0] = pos[0] * m_bark_dist + theBwnCtrl.m_target[0];
        pos[3] = 1.0f;
        pos[1] = pos[1] * m_bark_dist + theBwnCtrl.m_target[1];
        pos[2] = pos[2] * m_bark_dist + theBwnCtrl.m_target[2];
        m_pos[0] = pos[0];
        m_pos[1] = pos[1];
        m_pos[2] = pos[2];
        m_pos[3] = 1.0f;
        if (bwn->searchPath(pos) < 0)
        {
            *work = BARK_START;
            break;
        }
        bwn->message(6, 4, 0); /* walk the path */
        *work = BARK_APPROACHING;
        return;
    case BARK_APPROACHING:
    {
        float d = utlVecDist(pos, theBwnCtrl.m_target);
        if (!(m_bark_dist - 3.0f <= d && d <= m_bark_dist + 3.0f) && bwn->isActionEnd() == 0)
            return;
        *work = BARK_FACE;
        break;
    }
    case BARK_FACE:
    {
        t[2] = 0.0f;
        t[1] = 0.0f;
        t[0] = 0.0f;
        t[3] = 1.0f;
        utlGetObjPos(t, m_bite_target_oid);
        t[0] -= pos[0];
        t[3] = 1.0f;
        t[1] -= pos[1];
        t[2] -= pos[2];
        float heading = utlVecToRotY(t);
        float d = wrapAngle(heading - bwn->getRotY());
        if (-0.052359883f <= d && d <= 0.052359883f)
        {
            *work = BARK_BARK;
            return;
        }
        bwn->message(8, floatArg(heading), 0); /* turn to it */
        *work = BARK_FACING;
        break;
    }
    case BARK_FACING:
    {
        t[2] = 0.0f;
        t[1] = 0.0f;
        t[0] = 0.0f;
        t[3] = 1.0f;
        utlGetObjPos(t, m_bite_target_oid);
        dbgTraceVector4(t);
        dbgTraceVector4(theBwnCtrl.m_target);
        t[0] -= theBwnCtrl.m_target[0];
        t[3] = 1.0f;
        t[1] -= theBwnCtrl.m_target[1];
        t[2] -= theBwnCtrl.m_target[2];
        utlVecLength(t); /* unused in the ROM */
        if (bwn->isActionEnd() == 0)
            return;
        clearEmyInfoBuf(eib);
        int i = findNearestEmy(theBwnCtrl, &eib, 0);
        if (i < 0)
        {
            theBwnCtrl.requestState(0, 0, 0);
            return;
        }
        if (eib.m_info[i].m_obj_id != m_bite_target_oid ||
            m_bark_dist + 3.0f < utlVecDist(pos, eib.m_info[i].m_trans))
        {
            *work = BARK_START;
            return;
        }
        *work = BARK_BARK;
        return;
    }
    case BARK_GROWL:
        bwn->message(1, 0x1c, 1);
        *work = BARK_GROWLING;
        break;
    case BARK_GROWLING:
        if (bwn->isActionEnd() != 0 || bwn->getMotionSeq() != 0x1c)
            *work = BARK_START;
        break;
    case BARK_BARK:
        bwn->m_state = -1;
        bwn->m_04 = -1;
        bwn->message(1, 0x76, 1);
        utlSetBwnObjFlag(0);
        *work = BARK_BARKING;
        break;
    case BARK_BARKING:
    {
        t[2] = 0.0f;
        t[1] = 0.0f;
        t[0] = 0.0f;
        t[3] = 1.0f;
        utlGetObjPos(t, m_bite_target_oid);
        t[0] -= pos[0];
        t[3] = 1.0f;
        t[1] -= pos[1];
        t[2] -= pos[2];
        /* a degree a frame at most */
        float d = wrapAngle(utlVecToRotY(t) - bwn->getRotY());
        if (d < -0.017453294f)
            d = -0.017453294f;
        if (0.017453294f < d)
            d = 0.017453294f;
        DogActor *actor = bwn->m_actor; /* EE slot 11 getActor */
        actor->m_rotate[1] += d;
        float r = actor->m_rotate[1];
        if (r < -DOG_PI)
            actor->m_rotate[1] = r + 6.283186f;
        else if (DOG_PI <= r)
            actor->m_rotate[1] = r - 6.283186f;
        actor->m_dirty = 1;
        bwn->m_actor->addBaseRotY(d);
        /* edge to m_bark_dist - 20, a unit a frame at most */
        float move = utlVecLength(t) - (m_bark_dist - 20.0f);
        if (move != 0.0f)
        {
            if (1.0f < move)
                move = 1.0f;
            if (move < -1.0f)
                move = -1.0f;
            float dir[4] = { t[0], t[1], t[2], t[3] };
            utlNormalize3(dir);
            dir[0] *= move;
            dir[1] *= move;
            dir[2] *= move;
            dir[3] = 1.0f;
            actor = bwn->m_actor;
            actor->m_trans[0] += dir[0];
            actor->m_trans[1] += dir[1];
            actor->m_trans[3] = 1.0f;
            actor->m_trans[2] += dir[2];
            actor->m_dirty = 1;
        }
        break;
    }
    case BARK_ANSWER:
        bwn->m_state = -1;
        bwn->m_04 = -1;
        bwn->message(1, answer == 0 ? 0x71 : 0x79, 1);
        *work = BARK_ANSWERING;
        break;
    case BARK_ANSWERING:
        if (bwn->isActionEnd() == 0)
            return;
        utlSetBwnObjFlag(1);
        *work = BARK_REST;
        break;
    case BARK_AHEAD:
    {
        clearEmyInfoBuf(eib);
        int i = findNearestEmy(theBwnCtrl, &eib, 2);
        if (i < 0)
        {
            *work = BARK_START;
            break;
        }
        takeBarkTarget(*this, theBwnCtrl, eib.m_info[i]);
        theBwnCtrl.m_target[1] = pos[1];
        float d = utlVecDistRotY(pos, rot, theBwnCtrl.m_target);
        int near;
        if (m_target_kind == 0x401)
        {
            near = d < 140.0f;
            m_bark_dist = 135.0f;
        }
        else if (m_target_kind == 0x421)
        {
            near = d < 115.0f;
            m_bark_dist = 110.0f;
        }
        else
        {
            if (200.0f <= d)
            {
                *work = BARK_GO;
                return;
            }
            bwn->message(8, floatArg(rot[0]), 0);
            *work = BARK_TURNED;
            return;
        }
        *work = near ? BARK_FACE : BARK_APPROACH;
        break;
    }
    case BARK_GO:
        theBwnCtrl.m_target[1] = pos[1];
        if (bwn->searchPath(theBwnCtrl.m_target) < 0)
        {
            *work = BARK_START;
            break;
        }
        bwn->message(6, 4, 0);
        *work = BARK_GOING;
        return;
    case BARK_GOING:
        theBwnCtrl.m_target[1] = pos[1];
        if (utlVecDistRotY(pos, rot, theBwnCtrl.m_target) <= 300.0f)
        {
            bwn->message(8, floatArg(rot[0]), 0);
            *work = BARK_TURNED;
            return;
        }
        if (bwn->isActionEnd() != 0)
            *work = BARK_START;
        break;
    case BARK_REST:
        if (theBwnCtrl.m_mode_count % 300 != 0)
            *work = BARK_START;
        break;
    default:
        ROR_ASSERT(0); /* 2785 */
        break;
    }
}

// ROR 0x00129f50
int MagLifeBwnCtrlModeBark::query(MagLifeBwnCtrl &theBwnCtrl, int v)
{
    if (theBwnCtrl.m_08 != 11)
        return -1;
    return m_bite_target_oid == (u_int)v ? 1 : -1;
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
    ROR_ASSERT(actor); /* 2931 */  // callcheck: dbgAssertFailed (the assert handler)
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
        ROR_ASSERT(0); /* 3056 */  // callcheck: dbgAssertFailed (the assert handler)
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
    ROR_ASSERT(theMode >= 0); /* 3083 */  // callcheck: dbgAssertFailed (the assert handler)
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
    UtlEnemyInfoBuf eib;
    clearEmyInfoBuf(eib);
    int num = utlCollectItemInfo(&eib, 1);
    if (num <= 0)
        return 0;
    ROR_ASSERT(m_pc); /* 3511 */  // callcheck: dbgAssertFailed (the assert handler)
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pc->getPosition(pos);
    float best = 1e+06f;
    float fwd[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    utlVecRotMatY(m_pc->getRotY(), fwd);
    int found = -1;
    for (int i = 0; i < num; i++)
    {
        UtlEnemyInfo &e = eib.m_info[i];
        float dy = e.m_pos[1] - pos[1];
        if (!(-100.0f < dy && dy < 100.0f))
            continue;
        float dir[4] = { e.m_pos[0] - pos[0], e.m_pos[1] - pos[1], e.m_pos[2] - pos[2], 1.0f };
        utlNormalize3(dir);
        float dv = utlVecDot(dir, fwd);
        if (dv < 0.7f)
            continue;
        ROR_ASSERT(dv > 0.0f && dv <= 1.0f); /* 3536 */  // callcheck: dbgAssertFailed (the assert handler)
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
    UtlEnemyInfo &e = eib.m_info[found];
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
        m_scene->m_8542c.findCells(p, hit, 0, 100.0f, 0.0f);
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
    UtlEnemyInfoBuf eib;
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
    UtlEnemyInfoBuf eib;
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
    ROR_ASSERT(theLevel >= MAG_BWNCTRL_LEVEL_AUTO && theLevel <= MAG_BWNCTRL_LEVEL_MANUAL); /* 3922 */  // callcheck: dbgAssertFailed (the assert handler)
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
    ROR_ASSERT(m >= MISSION_NONE && m <= MISSION_HUG); /* mag/maglifebwnctrl.h 385 */  // callcheck: dbgAssertFailed (the assert handler)
    m_mission = m;
}
