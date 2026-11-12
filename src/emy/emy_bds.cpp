/* ==========================================================================
 *  emy/emy_bds.cpp
 *
 *  BdsMain (enemy type 4) and its outs (0x149d18..0x14b3bc). The usual
 *  enemy loop (as HfmMain): decide picks an input of the transition table
 *  (only inputs the current state's row has), exec moves the state graph
 *  by it, after switches outs on a change and runs the current one. The
 *  outs do the state's work and hand the next input back from Exec.
 * ======================================================================== */
#include "emy/emy_bds.h"

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
// ROR 0x00149d18 // callcheck: operator_new, EmyOut (new BdsOutXxx: the inline constructors call EmyOut::EmyOut)

void BdsMain::initOut()
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

    m_out[1] = new BdsOutWalk(m_oid, m_param, this);
    m_out[2] = new BdsOutSearch(m_oid, m_param, this);
    m_out[3] = new BdsOutAtk(m_oid, m_param, this);
    m_out[4] = new BdsOutWin(m_oid, m_param, this);
    m_out[5] = new BdsOutStared(m_oid, m_param, this);
    m_out[6] = new BdsOutDead(m_oid, m_param, this);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00149eb0 // callcheck: ~EmyMain, operator_delete (the base destructor and the deleting variant are the compiler's)

BdsMain::~BdsMain()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00149f10

void BdsMain::decide()
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
    if (m_input[1] && returnFalse()) {
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
// ROR 0x0014a034

void BdsMain::exec()
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
// ROR 0x0014a0dc

void BdsMain::after()
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
// ROR 0x0014a158
// The ROM returns 0 (the slot's value is never read).

void BdsMain::selectOut()
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
// ROR 0x0014a21c

bool BdsMain::isPCSameArea()
{
    return EmyMain::isPCSameArea();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014a230

int BdsMain::returnFalse()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014a240

bool BdsMain::isPCStateClass4()
{
    return g_magSystem.getPCStateClass() == 4;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014a268

int BdsMain::isPCInSight()
{
    float range = 0.0f;
    m_param->getParam(0x5f, &range);
    ObjStat *pcObj = g_gamMan.m_pcPtr;
    u_int pc = 0;
    if (pcObj != NULL)
        pc = pcObj->m_oid;
    return emyChkInSight(range, 0x1.921fb4p+1f /* 0x40490fda, pi */, 1, pc, m_param);
}

// ──── BdsOutWalk ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014a2cc

void BdsOutWalk::Init()
{
    setWalkMotion(1);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014a2f8
// Steps: 0 choose (roam to a point at least 100 from the origin, or
// idle), 1 moving, 2 walking in place for 30 frames, 3 until the motion
// ends.

int BdsOutWalk::Exec()
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
// ROR 0x0014a450
// Tries the random offset at four scales, each in the four quadrants
// (+x +z, -x +z, -x -z, +x -z).

void BdsOutWalk::getRoamPos(float *pos)
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
// ROR 0x0014a700

void BdsOutWalk::startIdle(int r)
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
// ROR 0x0014a78c
// The ROM also passes 32769 (0x47000100) in f13 and 1 in t0, which
// chrMtnMove2Achr does not read.

void BdsOutWalk::setWalkMotion(int motion)
{
    float agl = m_emy->m_agl;
    float speed = agl == 0.0f ? 0x1.111110p-6f /* 0x3c888888, 1 / 60 */ : agl * 0x1.111110p-6f;
    chrMtnMove2Achr(speed, m_oid, motion, -1, 0x100);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014a7f8

void BdsOutWalk::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014a814

void BdsOutWalk::setStep(int step)
{
    if (m_step != step) {
        m_step = step;
        m_cnt  = 0;
    }
}

// ──── BdsOutStared ────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014a830
// Exec through the vtable (slot 3); its input is dropped.

void BdsOutStared::Init()
{
    m_step = 0;
    Exec();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014a860
// Steps: 0 turn to the dog unless it is within pi / 6 ahead, 1 while the
// dog barks, until m_move_done (only a move task sets it: the turn does
// not), then motion 0x17; 2 until the bark ends; 3 voice while the dog
// barks again, input 6 after 60 frames of it (back to step 0 when it
// stops). A bark ending in step 1 hands back input 6.

int BdsOutStared::Exec()
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
// ROR 0x0014a9cc

void BdsOutStared::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014a9e8 // callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (the double angle wrap, emyWrapAngle)

int BdsOutStared::isDogAhead(float range, float *angle)
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

// ──── BdsOutSearch ────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014ab38

void BdsOutSearch::Init()
{
    selectTarget(100);
    emyStartChase(10.0f, this, m_target);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014ab74
// In sight within 200: input 6; the target lost (2): 4; any other end: 7.

int BdsOutSearch::Exec()
{
    int r = emyChkChase(200.0f, 0x1.921fb4p+1f /* 0x40490fda, pi */, this, m_target, 0);
    if (r == 0)
        return -1;
    int input = r == 1 ? 6 : r == 2 ? 4 : 7;
    m_emy->m_move->stop();
    return input;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014ac24

void BdsOutSearch::Term()
{
    termCommon();
}

// ──── BdsOutAtk ───────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014ac40

void BdsOutAtk::Init()
{
    m_hit_dog = 0;
    m_hit_pc  = 0;
    selectTarget(100);
    m_step = 0;
    emyStartChase(75.0f, this, m_target);
    m_cnt = 0;
    playSE(0xb);
}

/* PC: the hit test the attack steps inline: the PC (unless its state
 * class is 9) and the dog (unless Brown is in state 10 or 13), each once
 * per attack, within 20 of the enemy's three hit nodes; word is the
 * scnChrRequest damage word. */
static void bdsAtkHit(BdsOutAtk *out, int word)
{
    if (!out->m_hit_pc && g_magSystem.getPCStateClass() != 9) {
        ObjStat *pcObj = g_gamMan.m_pcPtr;
        u_int pc = 0;
        if (pcObj != NULL)
            pc = pcObj->m_oid;
        if (emyChkHitReach(20.0f, out->m_oid, 3, pc)) {
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
        if (emyChkHitReach(20.0f, out->m_oid, 3, dog)) {
            out->m_hit_dog = 1;
            scnChrRequest(0xb, out->m_oid, 0, 0, word, dog, 0);
            out->playSE(1);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014ac98 // callcheck: emyChkHitReach, getBwnState, getPCStateClass, playSE, scnChrRequest (inlined four times; bdsAtkHit here)
// Steps: 0 chase until the target is within 150 and pi / 3 ahead, then
// 66 %: motion 0x20 (step 1, hits on frames 12..16, word 0x1d, 16
// frames), else motion 0x21 (step 2, hits on frames 12..16, 32..35 and
// 53..56, word 0x1e, 56 frames); 3 until the motion ends, then chase again.
// The ROM tests the windows as int-to-float compares of the bools.

int BdsOutAtk::Exec()
{
    switch (m_step) {
    case 0:
    {
        if (!emyChkInSight(150.0f, 0x1.0c1522p+0f /* 0x3f860a91, pi / 3 */, 1, m_target, m_achr))
            return -1;
        m_emy->m_move->stop();
        int motion;
        if (utlRandRange(0, 99) < 0x42) {
            m_step = 1;
            motion = 0x20;
        } else {
            motion = 0x21;
            m_step = 2;
        }
        setMotion(motion, 0);
        m_emy->setDecideSw(0);
        return -1;
    }
    case 1:
        m_cnt++;
        if (m_cnt >= 12 && m_cnt <= 16)
            bdsAtkHit(this, 0x1d);
        if (m_cnt == 0x10)
            m_step = 3;
        return -1;
    case 2:
        m_cnt++;
        if ((m_cnt >= 12 && m_cnt <= 16) || (m_cnt >= 0x20 && m_cnt <= 0x23) ||
            (m_cnt >= 0x35 && m_cnt <= 0x38))
            bdsAtkHit(this, 0x1e);
        if (m_cnt == 0x38)
            m_step = 3;
        return -1;
    case 3:
        if (!g_gamMan.isAchrMotionEnd(m_oid))
            return -1;
        m_emy->m_input_flg_motion = 1;
        m_hit_dog = 0;
        m_hit_pc  = 0;
        m_emy->setDecideSw(1);
        m_step = 0;
        emyStartChase(75.0f, this, m_target);
        m_cnt = 0;
        return -1;
    default:
        return -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b180

void BdsOutAtk::Term()
{
    termCommon();
}

// ──── BdsOutWin ───────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b19c

void BdsOutWin::Init()
{
    setMotion(0x15, 0x100);
    m_se = playSE(0xd);
    g_sndMan.setSeLoop(m_se, 1);
    m_step = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b1f4
// Init leaves step 1, so the chase test never runs (m_range is never set).

int BdsOutWin::Exec()
{
    if (m_step == 0 &&
        emyChkChase(m_range, 0x1.921fb4p-1f /* 0x3f490fda, pi / 4 */, this, m_target, 1) != 0) {
        m_emy->m_move->stop();
        m_step = 1;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b25c

void BdsOutWin::Term()
{
    termCommon();
}

// ──── BdsOutDead ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b278

void BdsOutDead::Init()
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
    changeMotion(m_emy->m_down_supine ? 10 : 0xb, 0x100);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b398

int BdsOutDead::Exec()
{
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014b3a0

void BdsOutDead::Term()
{
    termCommon();
}
