/* ==========================================================================
 *  emy/emy_bdl.cpp
 *
 *  BdlMain (enemy type 5) and its outs (0x14b3bc..0x14cde4), BdsMain's
 *  twin (emy_bds.cpp; the differences are listed in emy_bdl.h). The usual
 *  enemy loop (as HfmMain): decide picks an input of the transition table
 *  (only inputs the current state's row has), exec moves the state graph
 *  by it, after switches outs on a change and runs the current one. The
 *  outs do the state's work and hand the next input back from Exec.
 * ======================================================================== */
#include "emy/emy_bdl.h"

#include <math.h>

#include "common/rosepan_stub.h"

#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magscenecmd.h"
#include "mag/magsystem.h"
#include "snd/sndman.h"
#include "task/tsk_chr.h"
#include "task/tsk_scene.h"

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b3bc // callcheck: operator_new, EmyOut (new BdlOutXxx: the inline constructors call EmyOut::EmyOut)

void BdlMain::initOut()
{
    m_motion_list[0] = 1;
    m_motion_list[1] = 1;
    m_motion_list[2] = 3;
    m_motion_list[3] = 3;
    m_motion_list[4] = 3;
    m_motion_list[5] = 3;
    m_motion_list[7] = 0x20;
    m_motion_list[8] = 1;
    m_motion_list[9] = 1;
    m_state_flg[1] = 0;
    m_state_flg[2] = 1;
    m_state_flg[3] = 1;
    m_state_flg[4] = 0;
    m_state_flg[5] = 1;
    m_state_flg[6] = 0;
    m_pc_seen = 0;
    m_in_area = 0;

    m_out[1] = new BdlOutWalk(m_oid, m_param, this);
    m_out[2] = new BdlOutSearch(m_oid, m_param, this);
    m_out[3] = new BdlOutAtk(m_oid, m_param, this);
    m_out[4] = new BdlOutWin(m_oid, m_param, this);
    m_out[5] = new BdlOutStared(m_oid, m_param, this);
    m_out[6] = new BdlOutDead(m_oid, m_param, this);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b554 // callcheck: ~EmyMain, operator_delete (the base destructor and the deleting variant are the compiler's)

BdlMain::~BdlMain()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b5b4

void BdlMain::decide()
{
    if (m_decide_sw == 0)
        return;
    if (m_input_flg_motion == 0)
        return;
    if (m_req != -1)
        return;

    if (m_input[0]) {
        if (!isPCSameArea()) {
            if (m_in_area) {
                m_in_area = 0;
                m_req = 0;
            }
            return;
        }
        m_in_area = 1;
    }
    if (m_input[1] && testBarkReact()) {
        m_req = 1;
        return;
    }
    if (m_input[2] && (m_notify & 1)) {
        m_req = 2;
        return;
    }

    int seen = isPCInSight();
    if (!m_pc_seen) {
        if (seen) {
            m_pc_seen = 1;
            if (m_input[3]) {
                m_req = 3;
                return;
            }
        }
    } else if (!seen) {
        m_pc_seen = 0;
        if (m_input[4]) {
            m_req = 4;
            return;
        }
    }
    if (m_input[5] && isPCStateClass4())
        m_req = 5;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b6d8

void BdlMain::exec()
{
    m_changed = 0;
    if (m_notify & 2) {
        m_state = 6;
        m_map->m_cur = 6;
        m_changed = 1;
    } else if (m_req != -1) {
        int state = m_map->next(m_req);
        m_in_area = 0;
        m_pc_seen = 0;
        if (state == 0) {
            m_map->m_cur = m_state;
        } else {
            m_state = state;
            m_changed = 1;
        }
    } else if (m_notify & 1) {
        m_map->m_cur = m_state;
        m_changed = 1;
    }
    m_notify = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b780

void BdlMain::after()
{
    EmyOut *out = m_emy_out;
    if (m_changed) {
        out->Term();
        selectOut();
        out = m_emy_out;
    }
    m_req = out->Exec();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b7fc
// The ROM returns 0 (the slot's value is never read).

void BdlMain::selectOut()
{
    if (m_state >= 1 && m_state < 7) {
        m_emy_out = m_out[m_state];
        m_emy_out->Init();
    }
    int n = m_trans->m_inputs;
    const char *row = m_trans->table() + m_state * n;
    // PORT: the ROM writes as many words as the table has inputs; the
    // host array holds 8.
    if (n > 8)
        n = 8;
    for (int i = 0; i < n; i++)
        m_input[i] = row[i] != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b8c0

bool BdlMain::isPCSameArea()
{
    return EmyMain::isPCSameArea();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b8d4

int BdlMain::testBarkReact()
{
    return EmyMain::testBarkReact();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b8e8

bool BdlMain::isPCStateClass4()
{
    return g_magSystem.getPCStateClass() == 4;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b910

int BdlMain::isPCInSight()
{
    float range = 0.0f;
    m_param->getParam(0x5f, &range);
    ObjStat *pcObj = g_gamMan.m_pcPtr;
    u_int pc = 0;
    if (pcObj != NULL)
        pc = pcObj->m_oid;
    return emyChkInSight(range, 0x1.921fb4p+1f /* 0x40490fda, pi */, 1, pc, m_param);
}

// ──── BdlOutWalk ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b974

void BdlOutWalk::Init()
{
    setWalkMotion(1);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b9a0
// Steps: 0 choose (roam to a point at least 100 from the origin, or
// idle), 1 moving, 2 walking in place for 30 frames, 3 until the motion
// ends.

int BdlOutWalk::Exec()
{
    int cnt = m_cnt++;
    switch (m_step) {
    case 0:
    {
        int r = utlRandRange(1, 100);
        if (r <= 80) {
            float pos[4];
            getRoamPos(pos);
            // ROM: the length of the point itself, not of the way to it.
            if (emyVecLength(pos) >= 100.0f) {
                emyStartMove(0.0f, this, pos);
                setStep(1);
                return -1;
            }
        }
        startIdle(r);
        return -1;
    }
    case 1:
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        break;
    case 2:
        if (cnt + 1 < 30)
            return -1;
        break;
    case 3:
        if (!g_gamMan.isAchrMotionEnd(m_oid))
            return -1;
        m_emy->m_input_flg_motion = 1;
        break;
    default:
        return -1;
    }
    setStep(0);
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014baf8
// Tries the random offset at four scales, each in the four quadrants
// (+x +z, -x +z, -x -z, +x -z).

void BdlOutWalk::getRoamPos(float *pos)
{
    const float scale[4] = {    // built on the stack
        1.0f, 0x1.666666p-1f /* 0.7 */, 0x1.999998p-2f /* 0.4 */, 0x1.999998p-4f /* 0.1 */,
    };
    float from[4];
    float home[4];
    float p[4];

    getBasePos(from);
    m_achr->getParam(0x59, home);
    float dx = utlRandRangeF(-500.0f, 500.0f);                           // 0x43fa0000
    float dz = utlRandRangeF(-500.0f, 500.0f);
    for (int i = 0; i < 4; i++) {
        float ox = dx * scale[i];
        float oz = dz * scale[i];
        for (int j = 0; j < 4; j++)
            p[j] = home[j];
        p[0] = home[0] + ox;
        p[2] = home[2] + oz;
        if (emyCanReach(from, p))
            goto found;
        p[0] = home[0] - ox;
        if (emyCanReach(from, p))
            goto found;
        p[2] = home[2] - oz;
        if (emyCanReach(from, p))
            goto found;
        p[0] = home[0] + ox;
        if (emyCanReach(from, p))
            goto found;
    }
    for (int j = 0; j < 4; j++)
        pos[j] = home[j];
    return;

found:
    for (int j = 0; j < 4; j++)
        pos[j] = p[j];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014bdbc

void BdlOutWalk::startIdle(int r)
{
    if (r < 0x56) {
        int r2 = utlRandRange(1, 100);
        int motion = 1;
        if (r2 > 30)
            motion = r2 > 60 ? 0x1d : 0x1c;
        setWalkMotion(motion);
        setStep(2);
        return;
    }
    setMotion(0x14, 0);
    setStep(3);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014be48
// The ROM also passes 32769 (0x47000100) in f13 and 1 in t0, which
// chrMtnMove2Achr does not read.

void BdlOutWalk::setWalkMotion(int motion)
{
    float agl = m_emy->m_agl;
    float speed = agl == 0.0f ? 0x1.111110p-6f /* 0x3c888888, 1 / 60 */ : agl * 0x1.111110p-6f;
    chrMtnMove2Achr(speed, m_oid, motion, -1, 0x100);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014beb4

void BdlOutWalk::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014bed0

void BdlOutWalk::setStep(int step)
{
    if (m_step != step) {
        m_step = step;
        m_cnt  = 0;
    }
}

// ──── BdlOutStared ────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014beec
// Exec through the vtable (slot 3); its input is dropped.

void BdlOutStared::Init()
{
    m_step = 0;
    Exec();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014bf1c
// Steps: 0 turn to the dog unless it is within pi / 6 ahead, 1 while the
// dog barks, until m_move_done (only a move task sets it: the turn does
// not), then motion 0x17; 2 until the bark ends; 3 voice while the dog
// barks again, input 6 after 60 frames of it (back to step 0 when it
// stops). A bark ending in step 1 hands back input 6.

int BdlOutStared::Exec()
{
    switch (m_step) {
    case 0:
    {
        float angle;
        if (!isDogAhead(0x1.0c1522p-1f /* 0x3f060a91, pi / 6 */, &angle)) {
            int motion = angle < 0.0f ? 8 : 9;
            // The ROM passes 1 in a3 too, which chrRotMotion does not read.
            chrRotMotion(angle, 1.5f, 0x1.000200p+15f /* 0x47000100, the object's heading */,
                         m_oid, motion, 0);
            m_step = 1;
            return -1;
        }
        break;
    }
    case 1:
        if (!magBwnIsBarkPhase3())
            return 6;
        if (m_move_done == 0)
            return -1;
        break;
    case 2:
        if (!magBwnIsBarkPhase3()) {
            m_cnt  = 0;
            m_step = 3;
        }
        return -1;
    case 3:
        tickVoice(8, 0x18, 9);
        if (!magBwnIsBarkPhase3()) {
            m_step = 0;
            return -1;
        }
        if (++m_cnt < 0x3c)
            return -1;
        stopVoice();
        return 6;
    default:
        return -1;
    }
    setMotion(0x17, 0x100);
    m_step = 2;
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014c088

void BdlOutStared::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014c0a4 // callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (the double angle wrap, emyWrapAngle)

int BdlOutStared::isDogAhead(float range, float *angle)
{
    float base[4], dog[4];
    getBasePos(base);
    emyGetDogPos(dog);
    float rot = g_magSystem.m_scene.m_actors[m_achr->m_actor].getRotY();
    float a = emyWrapAngle(atan2f(dog[0] - base[0], dog[2] - base[2]) - rot);
    if (angle != NULL)
        *angle = a;
    return -(range * 0.5f) <= a && a <= range * 0.5f;
}

// ──── BdlOutSearch ────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014c1f4

void BdlOutSearch::Init()
{
    selectTarget(100);
    emyStartChase(10.0f, this, m_target);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014c230
// In sight within 300: input 6; the target lost (2): 4; any other end: 7.

int BdlOutSearch::Exec()
{
    int r = emyChkChase(300.0f, 0x1.921fb4p+1f /* 0x40490fda, pi */, this, m_target, 0);
    if (r == 0)
        return -1;
    int input = r == 1 ? 6 : r == 2 ? 4 : 7;
    m_emy->m_move->stop();
    return input;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014c2e0

void BdlOutSearch::Term()
{
    termCommon();
}

// ──── BdlOutAtk ───────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014c2fc
// Mode 1 waits for the target ahead (step 1); the others turn first.

void BdlOutAtk::Init()
{
    selectTarget(100);
    m_cnt = 0;
    u_short mode;
    m_achr->getParam(0x68, &mode);
    m_mode = mode;
    if (mode == 1) {
        m_step = 1;
    } else {
        emyStartTurn(0x1.921fb4p-1f /* 0x3f490fda, pi / 4 */, this, 0);
        m_step = 0;
    }
}

/* The sector tests of Exec's steps 0 and 2: the world angle from the
 * enemy to the target (atan2 of x / z) against the mode's bounds, compared
 * as doubles (data 0x2ef6a8..0x2ef720). Modes 2..5 are 6pi / 8 wide
 * sectors; step 0 takes any other mode as in the sector, step 2 as out. */
#define BDL_PI8  0x1.921fb54442d18p-2   /*   pi / 8 */
#define BDL_3PI8 0x1.2d97c7f3321d2p+0   /* 3 pi / 8 */
#define BDL_5PI8 0x1.f6a7a2955385ep+0   /* 5 pi / 8 */
#define BDL_7PI8 0x1.5fdbbe9bba775p+1   /* 7 pi / 8 */

static int bdlAtkInSector(int mode, double a)
{
    switch (mode) {
    case 2:  return a > -BDL_PI8 && a < BDL_5PI8;
    case 3:  return a > BDL_3PI8 || a < -BDL_7PI8;
    case 4:  return a > BDL_7PI8 || a < -BDL_3PI8;
    case 5:  return a > -BDL_5PI8 && a < BDL_PI8;
    default: return 1;
    }
}

static int bdlAtkInSectorAgain(int mode, double a)
{
    switch (mode) {
    case 2:  return a > -BDL_PI8 && a < BDL_5PI8;
    case 3:  return a > BDL_3PI8 || a < -BDL_7PI8;
    // ROM: mode 4's lower bound is 5pi / 8 here (step 0: -3pi / 8), so
    // step 2 takes almost any angle as back in the sector.
    case 4:  return a > BDL_7PI8 || a < BDL_5PI8;
    case 5:  return a > -BDL_5PI8 && a < BDL_PI8;
    default: return 0;
    }
}

/* The world angle from getBasePos to the target's node. */
static float bdlAtkTargetAngle(BdlOutAtk *out)
{
    float base[4], target[4];
    out->getBasePos(base);
    out->getTargetNodePos(target);
    return atan2f(target[0] - base[0], target[2] - base[2]);
}

/* 66 %: motion 0x20 (step 3), else motion 0x21 (step 4). */
static void bdlAtkStart(BdlOutAtk *out)
{
    out->m_hit_dog = 0;
    out->m_hit_pc  = 0;
    int motion;
    if (utlRandRange(0, 99) < 0x42) {
        out->m_step = 3;
        motion = 0x20;
    } else {
        out->m_step = 4;
        motion = 0x21;
    }
    out->setMotion(motion, 0);
    out->m_emy->setDecideSw(0);
    out->playSE(0xb);
}

/* PC: the hit test the attack steps inline: the PC (unless its state
 * class is 9) and the dog (unless Brown is in state 10 or 13), each once
 * per attack, within 40 of the enemy's four hit nodes; word is the
 * scnChrRequest damage word. */
static void bdlAtkHit(BdlOutAtk *out, int word)
{
    if (!out->m_hit_pc && g_magSystem.getPCStateClass() != 9) {
        ObjStat *pcObj = g_gamMan.m_pcPtr;
        u_int pc = 0;
        if (pcObj != NULL)
            pc = pcObj->m_oid;
        if (emyChkHitReach(40.0f, out->m_oid, 4, pc)) {
            out->m_hit_pc = 1;
            scnChrRequest(0xb, out->m_oid, 0, 0, word, pc, 0);
            out->playSE(1);
        }
    }
    if (!out->m_hit_dog && g_magSystem.getBwnState() != 10 && g_magSystem.getBwnState() != 0xd) {
        ObjStat *dogObj = g_gamMan.m_dogPtr;
        u_int dog = 0;
        if (dogObj != NULL)
            dog = dogObj->m_oid;
        if (emyChkHitReach(40.0f, out->m_oid, 4, dog)) {
            out->m_hit_dog = 1;
            scnChrRequest(0xb, out->m_oid, 0, 0, word, dog, 0);
            out->playSE(1);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014c370 // callcheck: fptodp, dpcmp, atan2f, getBasePos, getTargetNodePos, utlRandRange, playSE, emyChkHitReach, getBwnState, getPCStateClass, scnChrRequest (inlined: the double sector tests twice in bdlAtkTargetAngle / bdlAtkInSector, the attack start twice in bdlAtkStart, the hit test four times in bdlAtkHit)
// Steps: 0 turning: out of the sector, stand (motion 1, step 2); in it,
// once the turn ends, attack (turn result 1) or hand back input 7;
// 1 (mode 1) attack once the target is within 200 and pi / 4 ahead;
// 2 standing until the target is in the sector again, then turn (step 0);
// 3 motion 0x20, hits on frames 12..16 (word 0x1f), 16 frames;
// 4 motion 0x21, hits on frames 12..16, 32..35 and 53..56 (word 0x20),
// 56 frames; 5 until the motion ends, then step 1 (mode 1) or turn again.
// The ROM tests the windows as int-to-float compares of the bools.

int BdlOutAtk::Exec()
{
    switch (m_step) {
    case 0:
        if (!bdlAtkInSector(m_mode, bdlAtkTargetAngle(this))) {
            m_emy->InactiveTurn();
            setMotion(1, 0);
            m_step = 2;
            return -1;
        }
        if (m_turn_done == 0)
            return -1;
        m_emy->InactiveTurn();
        if (m_turn_done != 1)
            return 7;
        bdlAtkStart(this);
        return -1;
    case 1:
        if (!emyChkInSight(200.0f, 0x1.921fb4p-1f /* 0x3f490fda, pi / 4 */, 1, m_target, m_achr))
            return -1;
        bdlAtkStart(this);
        return -1;
    case 2:
        if (bdlAtkInSectorAgain(m_mode, bdlAtkTargetAngle(this))) {
            emyStartTurn(0x1.921fb4p-1f /* 0x3f490fda, pi / 4 */, this, 0);
            m_step = 0;
        }
        return -1;
    case 3:
        m_cnt++;
        if (m_cnt >= 12 && m_cnt <= 16)
            bdlAtkHit(this, 0x1f);
        if (m_cnt == 0x10)
            m_step = 5;
        return -1;
    case 4:
        m_cnt++;
        if ((m_cnt >= 12 && m_cnt <= 16) || (m_cnt >= 0x20 && m_cnt <= 0x23) ||
            (m_cnt >= 0x35 && m_cnt <= 0x38))
            bdlAtkHit(this, 0x20);
        if (m_cnt == 0x38)
            m_step = 5;
        return -1;
    case 5:
        if (!g_gamMan.isAchrMotionEnd(m_oid))
            return -1;
        m_emy->m_input_flg_motion = 1;
        m_hit_dog = 0;
        m_hit_pc  = 0;
        m_emy->setDecideSw(1);
        if (m_mode == 1) {
            m_step = 1;
        } else {
            emyStartTurn(0x1.921fb4p-1f /* 0x3f490fda, pi / 4 */, this, 0);
            m_step = 0;
        }
        m_cnt = 0;
        return -1;
    default:
        return -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014cbb8

void BdlOutAtk::Term()
{
    termCommon();
}

// ──── BdlOutWin ───────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014cbd4

void BdlOutWin::Init()
{
    setMotion(0x15, 0x100);
    m_se = playSE(0xd);
    g_sndMan.setSeLoop(m_se, 1);
    m_step = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014cc2c
// Init leaves step 1, so the chase test never runs (m_range is never set).

int BdlOutWin::Exec()
{
    if (m_step == 0 &&
        emyChkChase(m_range, 0x1.921fb4p-1f /* 0x3f490fda, pi / 4 */, this, m_target, 1) != 0) {
        m_emy->m_move->stop();
        m_step = 1;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014cc94

void BdlOutWin::Term()
{
    termCommon();
}

// ──── BdlOutDead ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014ccb0

void BdlOutDead::Init()
{
    float pos[3];
    m_achr->getParam(5, pos);
    EffParam eff = {};          // PORT: the ROM leaves the words it does not set as they were
    eff.m_kind = 3;
    eff.m_08   = 0;
    eff.m_hit.m_pos[0] = pos[0];
    eff.m_hit.m_pos[1] = pos[1];
    eff.m_hit.m_pos[2] = pos[2];
    eff.m_hit.m_1c    = 0;
    eff.m_hit.m_20    = -1.0f;
    eff.m_hit.m_24    = 0;
    eff.m_hit.m_scale = 100.0f;
    eff.m_hit.m_2c    = 0x5a;
    g_magSystem.m_scene.m_eff_man.createEff(&eff);
    m_emy->m_visible = 0;
    g_gamMan.resetObj(m_oid, 0x40);
    g_gamMan.setObj(m_oid, 0x10000000);
    g_gamMan.setObj(m_oid, 0x400);
    m_emy->stopAllSE();
    m_achr->m_flags |= 0x400;
    m_achr->m_flags |= 0x2000;
    changeMotion(0x24, 0x100);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014cdc0

int BdlOutDead::Exec()
{
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014cdc8

void BdlOutDead::Term()
{
    termCommon();
}
