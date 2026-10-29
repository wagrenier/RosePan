/* ==========================================================================
 *  emy/emy_nor.cpp
 *
 *  NorMain (enemy type 10) and its outs (0x150b24..0x153e40). The usual
 *  enemy loop: decide picks an input of the transition table (only inputs
 *  the current state's row has), exec moves the state graph by it, after
 *  switches outs on a change and runs the current one. A hit (notify bit
 *  1) restarts the current state; a kill (bit 2) forces state 4. Its
 *  m_func is EmyMain::func_count, so m_func_count is a frame clock (flee
 *  after 3601 frames, the coax windows).
 * ======================================================================== */
#include "emy/emy_nor.h"

#include <math.h>
#include <string.h>

#include "common/rosepan_stub.h"

#include "dog/dogactor.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "mag/magscenecmd.h"
#include "mag/magsystem.h"
#include "sequence/sequence.h"
#include "task/tsk_enemy.h"
#include "task/tsk_scene.h"
#include "utl/utlrand.h"

static const float NOR_PI      = 0x1.921fb4p+1f;   /* 0x40490fda */
static const float NOR_PI_4    = 0x1.921fb4p-1f;   /* 0x3f490fda */
static const float NOR_PI_8    = 0x1.921fb4p-2f;   /* 0x3ec90fda */
static const float NOR_CLAW_RANGE  = 88.5f;                 /* 0x42b10000 */
static const float NOR_GRAB_RANGE  = 0x1.37fffep+7f;        /* 0x431bffff, 156 */
static const float NOR_LUNGE_RANGE = 386.0f;                /* 0x43c10000 */

static u_int norPCOid()
{
    ObjStat *pc = g_gamMan.m_pcPtr;
    return pc != NULL ? pc->m_oid : 0;
}

static u_int norDogOid()
{
    ObjStat *dog = g_gamMan.m_dogPtr;
    return dog != NULL ? dog->m_oid : 0;
}

/* The outs' decide off / on with the achr's flag 0x400 (the ROM inlines
 * the pair at every use). */
static void norHold(EmyOut *out)
{
    out->m_emy->setDecideSw(0);
    g_gamMan.setObj(out->m_oid, 0x400);
}

static void norRelease(EmyOut *out)
{
    out->m_emy->setDecideSw(1);
    g_gamMan.resetObj(out->m_oid, 0x400);
}

/* chk_motion_end inline: the achr's motion ended sets the emy's motion
 * input. */
static bool norMotionEnd(EmyOut *out)
{
    if (!g_gamMan.isAchrMotionEnd(out->m_oid))
        return false;
    out->m_emy->m_input_flg_motion = 1;
    return true;
}

/* The grab's object flags back (Term, the lunge's and the release's end). */
static void norGrabFlagsOff(EmyOut *out)
{
    g_gamMan.resetObj(out->m_oid, 0x4000000);
    g_gamMan.setObj(out->m_oid, 0x40);
    g_gamMan.resetObj(out->m_oid, 0x10000000);
    g_gamMan.resetObj(out->m_oid, 0x400);
}

/* The hit tests the outs inline: once per attack, the PC (not in class 9)
 * or the dog (not in bwn states 10 / 0xd) within range of the first
 * nodes p_hit..; xz picks emyChkHitReachXZ. A PC hit can count in
 * NorMain::m_hits. */
static void norHitPC(EmyOut *out, int *hit, float range, int nodes, bool xz, int se, int word,
                     bool count)
{
    if (*hit || g_magSystem.getPCStateClass() == 9)
        return;
    u_int pc = norPCOid();
    if (xz ? emyChkHitReachXZ(range, out->m_oid, nodes, pc) : emyChkHitReach(range, out->m_oid, nodes, pc)) {
        if (count)
            static_cast<NorMain *>(out->m_emy)->m_hits++;
        out->playSE(se);
        *hit = 1;
        scnChrRequest(0xb, out->m_oid, 0, 0, word, pc, 0);
    }
}

static void norHitDog(EmyOut *out, int *hit, float range, int nodes, bool xz, int se, int word)
{
    if (*hit || g_magSystem.getBwnState() == 10 || g_magSystem.getBwnState() == 0xd)
        return;
    u_int dog = norDogOid();
    if (xz ? emyChkHitReachXZ(range, out->m_oid, nodes, dog) : emyChkHitReach(range, out->m_oid, nodes, dog)) {
        out->playSE(se);
        *hit = 1;
        scnChrRequest(0xb, out->m_oid, 0, 0, word, dog, 0);
    }
}

/* The dog is spared while the state can flee (input 5) and has not. */
static bool norDogHittable(EmyOut *out)
{
    NorMain *nor = static_cast<NorMain *>(out->m_emy);
    return nor->m_input[5] == 0 || nor->m_fled != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00150b24 // callcheck: operator_new, EmyOut (new NorOutXxx: the inline constructors call EmyOut::EmyOut)

void NorMain::initOut(const _tagEnemyParam *prm)
{
    m_motion_list[3]  = 7;
    m_motion_list[10] = 0x17;
    m_motion_list[9]  = 0x17;
    m_motion_list[7]  = 0x1c;
    m_motion_list[11] = 8;
    m_motion_list[2]  = 3;
    m_motion_list[12] = 9;
    m_state_flg[5]    = 1;
    m_seen            = 0;
    m_motion_list[0]  = 1;
    m_motion_list[1]  = 1;
    m_motion_list[4]  = 1;
    m_motion_list[5]  = 1;
    m_motion_list[6]  = 1;
    m_motion_list[8]  = 1;
    m_state_flg[0]    = 0;
    m_state_flg[1]    = 1;
    m_state_flg[2]    = 1;
    m_state_flg[3]    = 1;
    m_state_flg[4]    = 0;
    m_state_flg[6]    = 0;
    m_state_flg[7]    = 0;
    m_state_flg[8]    = 0;

    m_out[1] = new NorOutAttack(m_oid, m_param, this);
    m_out[2] = new NorOutWalk(m_oid, m_param, this);
    m_out[3] = new NorOutBitten(m_oid, m_param, this);
    m_out[4] = new NorOutDead(m_oid, m_param, this);
    m_out[5] = new NorOutCounter(m_oid, m_param, this);
    m_out[6] = new NorOutBrownEv(m_oid, m_param, this);
    m_out[7] = new NorOutCoax(m_oid, m_param, this);
    m_out[8] = new NorOutTest(m_oid, m_param, this);

    GamObjColli colli;
    m_param->getParam(GAM_PRM_COL_PRM, &colli);
    colli.m_shape = 3;
    m_param->setParam(GAM_PRM_COL_PRM, &colli);

    m_fled = 0;
    m_238  = 0;
    m_hits = 0;
    g_gamMan.getPcParam(GAM_PRM_A_MAX_LP, &m_pc_max_lp);
    m_func_count = 0;
    m_func       = &EmyMain::func_count;
    m_pattern_pos = 9;
    m_max_lp      = prm->m_14;
    m_pattern     = 0;
    g_sequence.m_global[0x194] = 0;
    g_sequence.m_global[0x193] = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00150df8 // callcheck: ~EmyMain, operator_delete (the base destructor and the deleting variant are the compiler's)

NorMain::~NorMain()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00150e58 // callcheck: testInput5, testInput7, testInput6, testInput4, isTargetInSight
// Input 5 (flee) and a won input-3 draw end it at once; the others run
// in the order 7, 6, 4, 3, 2 and the last true one wins, then the sight
// edge (0 into sight, 1 out of it).

void NorMain::decide()
{
    if (m_decide_sw == 0)
        return;
    if (m_req != -1)
        return;

    if (m_input[5] && testInput5()) {
        m_req = 5;
        return;
    }
    if (m_input[7] && testInput7())
        m_req = 7;
    if (m_input[6] && testInput6())
        m_req = 6;
    if (m_input[4] && testInput4())
        m_req = 4;
    if (m_input[3] && (m_input_flg_motion & 0x11) == 1) {
        /* PORT: the ROM builds the rate table on the stack. */
        static const int rate[6] = { 0, 50, 30, 40, 60, 100 };
        m_238++;
        if (utlRandRange(0, 99) < rate[m_238]) {
            m_238 = 0;
            m_req = 3;
            return;
        }
    }
    if (m_input[2] && magBwnBarkQuery(m_oid) != -1) {
        m_bite_wait = utlRandRange(0x1e, 0x3c);
        m_req = 2;
    }

    u_char seen = (u_char)isTargetInSight();
    if (m_seen == 0) {
        m_seen = seen;
        if (seen != 0 && m_input[0])
            m_req = 0;
    } else {
        m_seen = seen;
        if (seen == 0 && m_input[1])
            m_req = 1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015102c
// Notify bit 2 (killed) forces state 4; bit 1 (hit) with no input restarts
// the current state; an input the graph has no move for keeps it.

void NorMain::exec()
{
    m_changed = 0;
    if (m_notify & 2) {
        m_state      = 4;
        m_map->m_cur = 4;
        m_changed    = 1;
        m_notify     = 0;
    } else if (m_req == -1) {
        if (m_notify & 1) {
            m_map->m_cur = m_state;
            m_changed    = 1;
        }
    } else {
        int state = m_map->next(m_req);
        m_seen = 0;
        if (state == 0) {
            m_map->m_cur = m_state;
        } else {
            m_state   = state;
            m_changed = 1;
        }
    }
    m_notify = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001510dc

void NorMain::after()
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
// ROR 0x00151158
// The ROM returns 0 (the slot's value is never read).

void NorMain::selectOut()
{
    int n = m_trans->m_inputs;
    const char *row = m_trans->m_table + m_state * n;
    // PORT: the ROM writes as many words as the table has inputs; the
    // host array holds 10 (EE 0x204..0x22c).
    if (n > 10)
        n = 10;
    for (int i = 0; i < n; i++)
        m_input[i] = row[i] != 0;
    if (m_state > 0 && m_state < 9) {
        m_emy_out = m_out[m_state];
        m_emy_out->Init();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00151218

int NorMain::testInput4()
{
    return EmyMain::isLowLife();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015122c

int NorMain::testInput5()
{
    int ret = 0;
    u_short lp;
    g_gamMan.getPcParam(GAM_PRM_A_LP, &lp);
    if (m_fled != 0)
        return 0;
    if ((u_int)lp < (u_int)(m_pc_max_lp >> 1) || m_func_count >= 0xe11 || m_hits >= 4) {
        ret = 1;
        m_param->m_flags |= 0x400;
        m_damage_end = 6;
        m_fled = 1;
    }
    if (m_fled == 0)
        g_gamMan.setObj(m_oid, 0x800);
    else
        g_gamMan.resetObj(m_oid, 0x800);
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015130c

int NorMain::testInput6()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00151314

int NorMain::testInput7()
{
    return EmyMain::isPCStateClass4();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00151328
// Both tests run (no short cut in the ROM).

bool NorMain::isTargetInSight()
{
    float range;
    m_param->getParam(0x5f, &range);
    int pc  = emyChkInSight(range, NOR_PI, 1, norPCOid(), m_param);
    int dog = emyChkInSight(range, NOR_PI, 1, norDogOid(), m_param);
    return pc != 0 || dog != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001513ec

bool NorMain::isCoaxTime()
{
    int t = m_func_count - 1000;
    if (t < 0)
        return false;
    if (t == 0)
        return true;
    return t % 700 < 200;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00151448

int NorMain::countBiteWait()
{
    m_bite_wait--;
    return m_bite_wait >= 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00151460

int NorMain::isPressed()
{
    u_short pc_lp, lp;
    g_gamMan.getPcParam(GAM_PRM_A_LP, &pc_lp);
    u_int pc_max = m_pc_max_lp;
    int   clock  = m_func_count;
    m_param->getParam(GAM_PRM_A_LP, &lp);
    if (m_fled != 0)
        return 0;
    if ((m_max_lp >> 1) <= lp && clock < 0xe11 && (u_int)((int)(pc_max * 3) >> 2) <= pc_lp)
        return 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00151508 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogActor::getNodeIndexByName inline)
// No references: the matrix is read and dropped.

void NorMain::getHitNodePos()
{
    int no = m_param->m_actor;
    if (no < 0)
        return;
    DogActor &actor = g_magSystem.m_scene.m_actors[no];
    int node = actor.getNodeIndexByName("p_hit");
    if (node >= 0) {
        sceVu0FMATRIX m;
        actor.getNodeWorldMatrix(m, node);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001515c4
// The attack patterns (0x298dd8, lengths 0x2efa60): 0 claw, 1 grab,
// 2 lunge, 3 scream.

static const int s_nor_pattern0[10] = { 2, 0, 3, 0, 1, 1, 2, 0, 3, 2 };  // ROR 0x002efa70
static const int s_nor_pattern1[8]  = { 1, 2, 0, 1, 3, 0, 1, 2 };        // ROR 0x002efa98
static const int s_nor_pattern2[7]  = { 2, 0, 0, 2, 1, 3, 0 };           // ROR 0x002efab8
static const int s_nor_pattern3[3]  = { 2, 2, 0 };                       // ROR 0x002efad8
static const int *const s_nor_patterns[4] = {                            // ROR 0x00298dd8
    s_nor_pattern0, s_nor_pattern1, s_nor_pattern2, s_nor_pattern3,
};
static const int s_nor_pattern_len[4] = { 10, 8, 7, 3 };                 // ROR 0x002efa60

int NorMain::nextAttack()
{
    int len = s_nor_pattern_len[m_pattern];
    if (++m_pattern_pos == len) {
        int r = utlRandRange(0, 99);
        if (r < 0x1e)
            m_pattern = 0;
        else if (r < 0x3c)
            m_pattern = 1;
        else if (r < 0x5a)
            m_pattern = 2;
        else
            m_pattern = 3;
        m_pattern_pos = 0;
    }
    return s_nor_patterns[m_pattern][m_pattern_pos];
}

// ══════════════════════════════════════════════════════════════════════
// NorOutAttack (state 1)

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00151674

void NorOutAttack::Init()
{
    selectTarget(100);
    m_emy->m_state2 = 0;
    start();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001516ac

int NorOutAttack::Exec()
{
    switch (m_mode) {
    case 0:  return execClaw();
    case 1:  return execGrab();
    case 2:  return execLunge();
    case 3:  return execScream();
    default: return -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00151744

void NorOutAttack::Term()
{
    termCommon();
    g_sequence.m_global[0x194] = 0;
    g_gamMan.resetObj(m_oid, 0x4000000);
    g_gamMan.setObj(m_oid, 0x40);
    g_gamMan.resetObj(m_oid, 0x10000000);
    g_gamMan.resetObj(m_oid, 0x400);
    m_emy->setDecideSw(1);
    g_gamMan.resetObj(m_oid, 0x400);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001517e0
// The pattern's mode, or (the row has input 5) claw / grab at even odds
// once fled, the claw before. The scream turns to the PC instead of a
// chase.

void NorOutAttack::start()
{
    NorMain *nor = static_cast<NorMain *>(m_emy);
    float range = 0.0f;         // PORT: the ROM leaves f20 unset for a mode out of 0..3 (none in the tables)
    m_chase_cnt = 0;
    m_count     = 0;
    m_struggle  = 0;
    if (nor->m_input[5] == 0) {
        m_mode = nor->nextAttack();
        switch (m_mode) {
        case 0:
            m_chase_max = utlRandPeakHi(10, 0x96, 3);
            m_step = 0;
            range  = NOR_CLAW_RANGE;
            break;
        case 1:
            m_step = 0;
            range  = NOR_GRAB_RANGE;
            break;
        case 2:
            m_hit_pc  = 0;
            m_hit_dog = 0;
            m_count   = 0;
            m_step    = 0;
            range     = NOR_LUNGE_RANGE;
            break;
        case 3:
            m_scream_wait = 0;
            emyStartTurn(NOR_PI_8, this, 0);
            m_step = 0;
            m_emy->setDecideSw(0);
            return;
        }
    } else {
        int r = utlRandRange(0, 99);
        if (nor->m_fled == 0)
            r = 0;
        if (r < 0x32) {
            m_mode = 0;
            m_chase_max = utlRandPeakHi(10, 0x96, 3);
            m_step = 0;
            range  = NOR_CLAW_RANGE;
        } else {
            m_step = 0;
            m_mode = 1;
            range  = NOR_GRAB_RANGE;
        }
    }
    emyStartChase(range, this, m_target);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015195c

void NorOutAttack::restartClaw()
{
    m_chase_cnt = 0;
    m_count     = 0;
    m_struggle  = 0;
    m_mode      = 0;
    m_chase_max = utlRandPeakHi(10, 0x96, 3);
    m_step      = 0;
    emyStartChase(NOR_CLAW_RANGE, this, m_target);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001519b8 // callcheck: emyChkHitReachXZ, getBwnState, getPCStateClass, isAchrMotionEnd, playSE, resetObj, scnChrRequest, setDecideSw, setObj (norHitPC / norHitDog / norMotionEnd, inline in the ROM)
// On arrival the lunge (motion 0x2b): decide off frames 15..55, hits
// frames 20..50; 8 at its end, 9 when the chase failed.

int NorOutAttack::execLunge()
{
    if (m_step == 0) {
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        if (m_move_done != 1)
            return 9;
        g_gamMan.setObj(m_oid, 0x20000000);
        g_gamMan.resetObj(m_oid, 0x40);
        g_gamMan.setObj(m_oid, 0x10000000);
        g_gamMan.setObj(m_oid, 0x400);
        g_gamMan.setObj(m_oid, 0x4000000);
        playSE(0x54);
        playSE(0x62);
        setMotion(0x2b, 0);
        m_step = 1;
        return -1;
    }
    if (m_step != 1)
        return -1;

    m_count++;
    if (m_count > 0xe && m_count < 0x38)
        norHold(this);
    else
        norRelease(this);
    if (m_count > 0x13 && m_count < 0x33) {
        norHitPC(this, &m_hit_pc, 55.0f, 4, true, 0x51, 0x23, false);
        norHitDog(this, &m_hit_dog, 55.0f, 4, true, 0x51, 0x23);
    }
    if (norMotionEnd(this)) {
        norRelease(this);
        g_gamMan.setObj(m_oid, 0x40);
        g_gamMan.resetObj(m_oid, 0x10000000);
        g_gamMan.resetObj(m_oid, 0x400);
        g_gamMan.resetObj(m_oid, 0x4000000);
        return 8;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00151d4c // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (emyGetNodePos's DogModel::getNodeIndexByName inline)

bool NorOutAttack::isTouching(u_int target)
{
    float spine[3], hit[3];
    emyGetNodePos(g_gamMan.getActorNo(target), "Bip01 Spine1", spine);
    emyGetNodePos(g_gamMan.getActorNo(m_oid), "p_hit", hit);
    return emyChkNearXZ(55.0f, hit, spine) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00151f1c // callcheck: isAchrMotionEnd, setObj, resetObj, playStr (norHold / norRelease / norMotionEnd inline in the ROM; playStr is g_gamMan.m_sndMan)
// After the turn: stream 0x3b8 and motion 0x12 (global 0x194 set while it
// screams), then 0x13 looping for 45..60 frames, then the next attack.

int NorOutAttack::execScream()
{
    norHold(this);
    if (m_step == 0) {
        if (m_turn_done != 0) {
            m_emy->m_turn->m_active = 0;
            float pos[4];
            getBasePos(pos);
            g_gamMan.m_sndMan.playStr(0x3b8, 1, pos, false, 0);
            setMotion(0x12, 0);
            g_sequence.m_global[0x194] = 1;
            m_step = 1;
        }
    } else if (m_step == 1) {
        if (norMotionEnd(this)) {
            setMotion(0x13, 0x100);
            m_step = 2;
            m_scream_wait = utlRandRange(0x2d, 0x3c);
        }
    } else if (m_step == 2) {
        if (--m_scream_wait < 1) {
            norRelease(this);
            m_emy->setDecideSw(1);
            m_achr->m_flags |= 0x400;
            m_emy->m_damage_end = 6;
            g_sequence.m_global[0x194] = 0;
            start();
        }
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001520cc // callcheck: emyChkHitReachXZ, getBwnState, getPCStateClass, isAchrMotionEnd, playSE, resetObj, scnChrRequest, setDecideSw, setObj (norHitPC / norHitDog / norMotionEnd, inline in the ROM)
// On arrival (or m_chase_cnt reaching m_chase_max, which nothing counts)
// the claw (motion 0x1c): hits frames 19..25, step 3 from frame 25, then
// the next attack at its end.

int NorOutAttack::execClaw()
{
    switch (m_step) {
    case 0:
        if (m_move_done == 2) {
            m_emy->m_move->stop();
            return 9;
        }
        if (m_move_done != 1 && m_chase_cnt < m_chase_max)
            return -1;
        m_emy->m_move->stop();
        playSE(0x53);
        playSE(99);
        m_hit_dog = 0;
        m_hit_pc  = 0;
        setMotion(0x1c, 0);
        m_count = 0;
        m_step  = 1;
        return -1;
    case 1:
        m_count++;
        if ((u_int)m_count < 0xf)
            norRelease(this);
        else
            norHold(this);
        if (m_count > 0x12 && m_count < 0x1a) {
            norHitPC(this, &m_hit_pc, 20.0f, 2, true, 0x50, 0x21, true);
            if (norDogHittable(this))
                norHitDog(this, &m_hit_dog, 20.0f, 2, true, 0x50, 0x21);
        }
        if (m_count == 0x19)
            m_step = 3;
        return -1;
    case 3:
        m_count++;
        if ((u_int)m_count < 0x1e)
            norHold(this);
        else
            norRelease(this);
        if (!norMotionEnd(this))
            return -1;
        norRelease(this);
        m_count = 0;
        start();
        return -1;
    default:
        return -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015246c

void NorOutAttack::releasePC()
{
    stopVoice();
    m_emy->m_state2 = 0;
    if (m_back == 0)
        g_magSystem.m_scene.commandPC(6, NULL);
    else
        g_magSystem.m_scene.commandPC(9, NULL);
    playSE(0x5b);
    if (static_cast<NorMain *>(m_emy)->m_input[5] == 0)
        playSE(0x65);
    else
        playSE(100);
    setMotion(0x23, 0);
    m_step = 6;
    m_emy->setDecideSw(1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00152544 // callcheck: findCells (SctWorld::findCells through g_magSystem.m_scene.m_8542c)
// The dog's bark while it holds the PC (steps 3, 8) makes it let go.
// Steps (table 0x2efb40): 0 the chase, then the grab's start (motion 0x21)
// when a cell lies 63 ahead (else the claw), 1 try, 2 / 3 hold, 4 / 5
// squeeze, 6 released, 7 missed (nothing sets it), 8 the PC struggles.

int NorOutAttack::execGrab()
{
    if ((m_step == 3 || m_step == 8) && magBwnBarkQuery(m_oid) != -1) {
        magBwnBarkSet(1);
        releasePC();
        return -1;
    }
    switch (m_step) {
    case 0: {
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        if (m_move_done != 1)
            return 9;
        float pos[4];
        getFrontPos(63.0f, pos);
        GamSecta *hit[4];
        g_magSystem.m_scene.m_8542c.findCells(pos, hit, 0, 40.0f, 0.0f);
        if (hit[1] == NULL) {
            restartClaw();
            return -1;
        }
        playSE(0x54);
        playSE(0x61);
        g_gamMan.resetObj(m_oid, 0x40);
        g_gamMan.setObj(m_oid, 0x10000000);
        g_gamMan.setObj(m_oid, 0x4000000);
        setMotion(0x21, 0);
        m_count = 0;
        m_step  = 1;
        return -1;
    }
    case 1: grabTry();        break;
    case 2: grabHoldStart();  break;
    case 3: grabHold();       break;
    case 4: grabSqueeze();    break;
    case 5: grabSqueezeEnd(); break;
    case 6: grabReleaseEnd(); break;
    case 7: grabMissEnd();    break;
    case 8: grabStruggle();   break;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00152748 // callcheck: fptodp, dpcmp, dpsub, dpadd, dptofp, isAchrMotionEnd, setObj, resetObj (the double compare, emyWrapAngle, norMotionEnd / norHold / norRelease / norGrabFlagsOff inline in the ROM)
// Frames 21..25 of motion 0x21: the PC within 80 and pi / 4 of its facing,
// not already held (commandPC 0x16), system flag 4 without 0x2000: the
// grab from the front (commandPC 2, motion 0x28, the PC turned to face
// it) or from behind (7, 0x29). At the motion end the next attack.

void NorOutAttack::grabTry()
{
    m_count++;
    if ((u_int)m_count < 0xb)
        norRelease(this);
    else
        norHold(this);

    if (norMotionEnd(this)) {
        norGrabFlagsOff(this);
        norRelease(this);
        start();
        return;
    }
    if (!(m_count > 0x14 && m_count < 0x1a))
        return;
    if (emyChkInSight(80.0f, NOR_PI_4, 1, m_target, m_achr) == 0)
        return;
    if (g_magSystem.m_scene.commandPC(0x16, NULL) != 0)
        return;
    if (g_gamMan.m_flags & 0x2000)
        return;
    if ((g_gamMan.m_flags & 4) == 0)
        return;

    m_emy->setDecideSw(0);
    g_gamMan.resetObj(m_oid, 0x40);
    g_gamMan.setObj(m_oid, 0x10000000);
    g_gamMan.setObj(m_oid, 0x4000000);
    m_rot = g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(m_oid)].getRotY();
    g_gamMan.getActorNo(m_target);  // the ROM drops the result
    m_back = (double)getTargetAngle() >= 1.5707963267948966;
    static_cast<NorMain *>(m_emy)->m_grab_back = m_back;
    int motion;
    if (m_back == 0) {
        m_emy->m_state2 = 3;
        g_magSystem.m_scene.commandPC(2, NULL);
        motion = 0x28;
        getFrontPos(60.0f, m_grab_pos);
        m_rot = emyWrapAngle((float)((double)m_rot - 3.141592653589793));
    } else {
        m_emy->m_state2 = 4;
        g_magSystem.m_scene.commandPC(7, NULL);
        motion = 0x29;
        getFrontPos(60.0f, m_grab_pos);
    }
    setMotion(motion, 0);
    m_struggle = 0;
    m_count    = 0;
    m_step     = 2;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00152b24 // callcheck: isAchrMotionEnd (norMotionEnd, inline in the ROM)

void NorOutAttack::grabHoldStart()
{
    if (norMotionEnd(this)) {
        setMotion(m_back != 0 ? 0x25 : 0x24, 0x100);
        m_step = 3;
        m_hold = utlRandRange(5, 0x12);
        return;
    }
    placePC();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00152bb4

void NorOutAttack::grabHold()
{
    placePC();
    tickVoice(0xc, 0x1e, 0x58);
    if (--m_hold < 1) {
        m_squeeze = utlRandRange(1, 3);
        setMotion(0x1d, 0);
        m_step = 4;
    } else if (g_magSystem.getPCStateClass() == 3) {
        setMotion(m_back != 0 ? 0x27 : 0x26, 0x100);
        m_step = 8;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00152c60

void NorOutAttack::grabSqueeze()
{
    tickVoice(0xc, 0x1e, 0x58);
    if (++m_count == 3) {
        static_cast<NorMain *>(m_emy)->m_hits++;
        scnChrRequest(0xb, m_oid, 0, 0, 0x22, m_target, 0);
        m_count = 0;
        m_step = --m_squeeze == 0 ? 2 : 5;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00152cfc // callcheck: isAchrMotionEnd (norMotionEnd, inline in the ROM)

void NorOutAttack::grabSqueezeEnd()
{
    tickVoice(0xc, 0x1e, 0x58);
    if (!norMotionEnd(this))
        return;
    if (g_magSystem.getPCStateClass() == 3) {
        setMotion(m_back != 0 ? 0x27 : 0x26, 0x100);
        m_step = 8;
    } else {
        setMotion(0x1d, 0);
        m_step = 4;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00152da4 // callcheck: isAchrMotionEnd, setObj, resetObj (norMotionEnd / norGrabFlagsOff, inline in the ROM)

void NorOutAttack::grabReleaseEnd()
{
    if (norMotionEnd(this)) {
        norGrabFlagsOff(this);
        start();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00152e40 // callcheck: isAchrMotionEnd (norMotionEnd, inline in the ROM)

void NorOutAttack::grabMissEnd()
{
    if (norMotionEnd(this))
        start();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00152e90

void NorOutAttack::grabStruggle()
{
    tickVoice(0xc, 0x1e, 0x58);
    if (g_magSystem.getPCStateClass() == 3) {
        if (++m_struggle > 0x50)
            releasePC();
    } else {
        setMotion(m_back != 0 ? 0x25 : 0x24, 0x100);
        m_step = 3;
        m_hold = utlRandRange(5, 0x12);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00152f3c // callcheck: findCells (SctWorld::findCells through g_magSystem.m_scene.m_8542c)
// The PC held at the enemy's base (facing m_rot), else 30 behind it (the
// enemy then moves there, ROM), else the enemy onto the target's spine
// when a cell is there.

void NorOutAttack::placePC()
{
    float pos[4];
    getBasePos(pos);
    pos[3] = m_rot;
    if (g_magSystem.m_scene.commandPC(0x15, pos) != 0)
        return;
    getFrontPos(-30.0f, pos);
    if (g_magSystem.m_scene.commandPC(0x15, pos) != 0) {
        m_achr->ObjStat::setPos(pos);
        return;
    }
    getTargetNodePos(pos);
    GamSecta *hit[4];
    g_magSystem.m_scene.m_8542c.findCells(pos, hit, 0, 40.0f, 0.0f);
    if (hit[1] != NULL)
        m_achr->ObjStat::setPos(pos);
}

// ══════════════════════════════════════════════════════════════════════
// NorOutBitten (state 3)

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00153030

void NorOutBitten::Init()
{
    m_step = 0;
    setMotion(0x17, 0x100);
    m_44     = 0;
    m_hit_pc = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00153068 // callcheck: emyChkHitReach, getPCStateClass, playSE, scnChrRequest, isAchrMotionEnd, setDecideSw, setObj, resetObj (norHitPC / norHold / norMotionEnd, inline in the ROM)
// While the dog stays on it (bark query), waits m_bite_wait frames with
// its voice, then hits back (motion 0x18): the PC frames 29..43, the dog
// kicked off at frame 23 (bark phase off); 8 at the end, or as soon as the
// dog lets go before the hit.

int NorOutBitten::Exec()
{
    int ret = -1;
    if (magBwnBarkQuery(m_oid) == -1 && m_step == 0) {
        magBwnBarkSet(1);
        return 8;
    }
    if (m_step == 0) {
        tickVoice(10, 0x19, 0x52);
        if (!static_cast<NorMain *>(m_emy)->countBiteWait()) {
            stopVoice();
            norHold(this);
            setMotion(0x18, 0);
            m_count = 0;
            m_kick  = 0;
            playSE(0x56);
            playSE(99);
            norHold(this);
            m_step = 1;
        }
    } else if (m_step == 1) {
        if (++m_count < 0x30)
            norHold(this);
        else
            norRelease(this);
        if (m_count > 0x1c && m_count < 0x2c)
            norHitPC(this, &m_hit_pc, 40.0f, 3, false, 0x51, 0x24, false);
        if (++m_kick == 0x17) {
            // PORT: the ROM reads the dog's oid without a NULL test.
            scnChrRequest(0xb, m_oid, 0, 0, 0x25, norDogOid(), 0);
            magBwnBarkSet(0);
        }
        if (norMotionEnd(this)) {
            ret = 8;
            norRelease(this);
        }
    }
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00153340

void NorOutBitten::Term()
{
    termCommon();
}

// ══════════════════════════════════════════════════════════════════════
// NorOutWalk (state 2)

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015335c

void NorOutWalk::Init()
{
    m_mode = getMode();
    m_time = utlRandRange(1000, 2000);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00153398 // callcheck: utlRandRangeF, getFloorY, emyCanReach, emyStartMove, isAchrMotionEnd (norMotionEnd, inline in the ROM)
// Step 0 tries three points near the base (one 80..120 to the -x side,
// two 120..160 ahead in z; the first by a draw), walks to the first
// reachable one (step 1), else looks round (motion 0xc, step 4). After
// a look up to two more tries at even odds, then a wait of 20..50 frames
// (step 3) with the walk motion.

int NorOutWalk::Exec()
{
    m_time--;
    switch (m_step) {
    case 0: {
        float base[4] = { 0.0f, 0.0f, 0.0f, 0.0f };  // PORT: getBasePos leaves [3]; the ROM copies it unset
        int  used_a = 0, used_b = 0;
        int  tries;
        getBasePos(base);
        int r = utlRandRange(0, 0x59);
        for (tries = 0; tries < 3; tries++) {
            float p[4];
            for (int i = 0; i < 4; i++)
                p[i] = base[i];
            if (!used_a && r < 0x1e) {
                p[0] += utlRandRangeF(-120.0f, -80.0f);
                p[2] += utlRandRangeF(80.0f, 120.0f);
                used_a = 1;
            } else if (!used_b && r < 0x3c) {
                p[0] += utlRandRangeF(-20.0f, 20.0f);
                p[2] += utlRandRangeF(120.0f, 160.0f);
                used_b = 1;
            } else {
                /* The ROM sets a third flag here it never reads. */
                p[0] += utlRandRangeF(-20.0f, 20.0f);
                p[2] += utlRandRangeF(120.0f, 160.0f);
            }
            p[1] += g_gamMan.getFloorY(base);
            if (emyCanReach(base, p)) {
                emyStartMove(0.0f, this, p);
                m_step = 1;
                return -1;
            }
        }
        if (m_walks >= 2)
            utlRandRange(0, 99);        // the ROM draws a number it does not use
        setMotion(0xc, 0);
        m_step = 4;
        break;
    }
    case 1:
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        m_walks = 0;
        m_step  = 0;
        break;
    case 3:
        if (--m_wait < 0) {
            m_step = 0;
            m_walks++;
        }
        break;
    case 4:
        if (norMotionEnd(this)) {
            if (++m_walks < 3 && utlRandRange(0, 99) < 0x32) {
                m_step = 0;
            } else {
                setWalkMotion();
                m_step = 3;
                m_wait = utlRandRange(0x14, 0x32);
            }
        }
        break;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001536cc

short NorOutWalk::getMode()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001536dc

void NorOutWalk::Term()
{
    termCommon();
}

// ══════════════════════════════════════════════════════════════════════
// NorOutDead (state 4)

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001536f8

void NorOutDead::Init()
{
    m_achr->m_flags |= 0x400;
    m_achr->m_flags |= 0x2000;
    setMotion(1, 0x100);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015373c

int NorOutDead::Exec()
{
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00153744

void NorOutDead::Term()
{
    termCommon();
}

// ══════════════════════════════════════════════════════════════════════
// NorOutBrownEv (state 6)

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00153760

void NorOutBrownEv::Init()
{
    setMotion(1, 0);
    m_step = 0;
    g_sequence.m_global[0x193] = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001537a0 // callcheck: isAchrMotionEnd (norMotionEnd, inline in the ROM)

int NorOutBrownEv::Exec()
{
    if (!norMotionEnd(this))
        return -1;
    g_sequence.m_global[0x193] = 0;
    return 8;
}

// ══════════════════════════════════════════════════════════════════════
// NorOutCounter (state 5)

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001537fc

void NorOutCounter::Init()
{
    m_emy->m_state2 = 0;
    playSE(0x56);
    playSE(99);
    setMotion(0x2a, 0);
    m_count   = 0;
    m_hit_dog = 0;
    m_hit_pc  = 0;
    m_step    = 0;
    selectTarget(100);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00153864 // callcheck: emyChkHitReach, getBwnState, getPCStateClass, playSE, scnChrRequest, isAchrMotionEnd, setDecideSw, setObj, resetObj (norHitPC / norHitDog / norHold / norMotionEnd, inline in the ROM)
// Motion 0x2a with decide off: hits frames 17..38, step 2 at frame 38, 8
// at the motion end.

int NorOutCounter::Exec()
{
    if (m_step == 0) {
        m_count++;
        norHold(this);
        if (m_count > 0x10 && m_count < 0x27) {
            norHitPC(this, &m_hit_pc, 65.0f, 4, false, 0x50, 0x24, true);
            if (norDogHittable(this))
                norHitDog(this, &m_hit_dog, 65.0f, 4, false, 0x50, 0x24);
        }
        if (m_count == 0x26)
            m_step = 2;
        return -1;
    }
    if (m_step != 2)
        return -1;
    m_count++;
    norHold(this);
    if (!norMotionEnd(this))
        return -1;
    norRelease(this);
    return 8;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00153b18

void NorOutCounter::Term()
{
    termCommon();
}

// ══════════════════════════════════════════════════════════════════════
// NorOutCoax (state 7)

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00153b34

void NorOutCoax::Init()
{
    emyStartTurn(NOR_PI_8, this, 0);
    m_step = 0;
    m_emy->setDecideSw(0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00153b78 // callcheck: isAchrMotionEnd, setObj, resetObj (norHold / norRelease / norMotionEnd, inline in the ROM)
// After the turn SE 0x59 and motion 0x12, then 0x13 looping; out of the
// coax window motion 0x14 and 8 at its end.

int NorOutCoax::Exec()
{
    norHold(this);
    if (!static_cast<NorMain *>(m_emy)->isCoaxTime() && m_step != 2) {
        setMotion(0x14, 0);
        m_step = 2;
    }
    switch (m_step) {
    case 0:
        if (m_turn_done != 0) {
            m_emy->m_turn->m_active = 0;
            playSE(0x59);
            setMotion(0x12, 0);
            m_step = 1;
        }
        break;
    case 1:
        if (norMotionEnd(this))
            setMotion(0x13, 0x100);
        break;
    case 2:
        if (norMotionEnd(this)) {
            norRelease(this);
            m_emy->setDecideSw(1);
            return 8;
        }
        break;
    }
    return -1;
}

// ══════════════════════════════════════════════════════════════════════
// NorOutTest (state 8)

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00153d00

void NorOutTest::Init()
{
    m_step = 0;
    setMotion(0x21, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00153d24 // callcheck: isAchrMotionEnd (norMotionEnd, inline in the ROM)

int NorOutTest::Exec()
{
    if (m_step >= 0 && m_step <= 2 && norMotionEnd(this)) {
        setMotion(0x28, 0);
        m_step = m_step == 2 ? 0 : m_step + 1;
    }
    return -1;
}
