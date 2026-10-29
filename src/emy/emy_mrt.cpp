/* ==========================================================================
 *  emy/emy_mrt.cpp
 *
 *  MrtMain (enemy type 7) and its outs (0x14cde4..0x14dc7c). The usual
 *  enemy loop (as BdlMain): decide picks an input of the transition table
 *  (only inputs the current state's row has), exec moves the state graph
 *  by it, after switches outs on a change and runs the current one. The
 *  outs do the state's work and hand the next input back from Exec.
 *  Unlike the other enemies, decide does not return at the first input it
 *  finds: every test runs and the last true one wins.
 * ======================================================================== */
#include "emy/emy_mrt.h"

#include <math.h>

#include "common/rosepan_stub.h"

#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "mag/magscenecmd.h"
#include "mag/magsystem.h"
#include "snd/sndman.h"
#include "task/tsk_chr.h"
#include "task/tsk_man.h"
#include "task/tsk_scene.h"

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014cde4 // callcheck: operator_new, EmyOut (new MrtOutXxx: the inline constructors call EmyOut::EmyOut)
// States 2 and 3 get one MrtOutSerch each; it reads m_state for which.

void MrtMain::initOut()
{
    m_motion_list[1] = 1;
    m_motion_list[0] = 1;
    m_motion_list[7] = 7;
    m_motion_list[3] = 7;
    m_motion_list[2] = 3;
    m_pc_seen = 0;

    m_out[2] = new MrtOutSerch(m_oid, m_param, this);
    m_out[3] = new MrtOutSerch(m_oid, m_param, this);
    m_out[4] = new MrtOutAtk(m_oid, m_param, this);
    m_out[1] = new MrtOutWin(m_oid, m_param, this);
    m_238     = 0;              // ROM: a byte store at +0x238
    m_pc_seen = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014cef8 // callcheck: ~EmyMain, operator_delete (the base destructor and the deleting variant are the compiler's)

MrtMain::~MrtMain()
{
}

/* decide's chain of tests, inputs 4, 5, 6, 7, 3 in this order: each one
 * the state's row has and whose test is true overwrites m_req. */
static void mrtDecideChain(MrtMain *emy)
{
    if (emy->m_input[4] && emy->testInput4())
        emy->m_req = 4;
    if (emy->m_input[5] && emy->isPCStateClass1())
        emy->m_req = 5;
    if (emy->m_input[6] && emy->isPCAhead())
        emy->m_req = 6;
    if (emy->m_input[7] && emy->isPCClass7OffAxis())
        emy->m_req = 7;
    if (emy->m_input[3] && emy->isTrianglePressed())
        emy->m_req = 3;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014cf58 // callcheck: testInput4, isPCStateClass1, isPCAhead, isPCClass7OffAxis, isTrianglePressed (the chain is inlined twice: mrtDecideChain)
// The chain, then the triangle forces input 3; otherwise the sight edge:
// coming into sight is input 2, leaving it input 4, staying in sight runs
// the chain again.

void MrtMain::decide()
{
    if (m_decide_sw == 0)
        return;
    if (m_input_flg_motion == 0)
        return;
    if (m_req != -1)
        return;

    mrtDecideChain(this);
    if (isTrianglePressed() == 1) {
        m_req = 3;
        return;
    }

    int seen = isPCInSight();
    if (m_pc_seen) {
        m_pc_seen = (u_char)seen;
        if (!(u_char)seen) {
            if (m_input[4])
                m_req = 4;
            return;
        }
        mrtDecideChain(this);
        return;
    }
    m_pc_seen = (u_char)seen;
    if (!(u_char)seen)
        return;
    if (m_input[2])
        m_req = 2;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d13c
// Notify bit 2 (killed) only restarts the current state's out; a state
// out of 1..5 counts as no move. m_notify stays set when nothing applied.

void MrtMain::exec()
{
    m_changed = 0;
    if (m_notify & 2) {
        m_notify  = 0;
        m_changed = 1;
        return;
    }
    if (m_req == -1)
        return;
    m_notify = 0;
    int state = m_map->next(m_req);
    if (state >= 6 || state < 0)
        state = 0;
    if (state != 0) {
        m_state   = state;
        m_changed = 1;
        m_pc_seen = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d1c4

void MrtMain::after()
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
// ROR 0x0014d240
// The ROM returns 0 (the slot's value is never read).

void MrtMain::selectOut()
{
    if (m_state >= 1 && m_state <= 4) {
        m_emy_out = m_out[m_state];
        m_emy_out->Init();
    }
    int n = m_trans->m_inputs;
    const char *row = m_trans->m_table + m_state * n;
    // PORT: the ROM writes as many words as the table has inputs; the
    // host array holds 12 (EE 0x208..0x238).
    if (n > 12)
        n = 12;
    for (int i = 0; i < n; i++)
        m_input[i] = row[i] != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d308

bool MrtMain::isTrianglePressed()
{
    return (g_magSystem.m_pad[0].m_trig >> 12 & 1) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d320

int MrtMain::testInput4()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d328

int MrtMain::isPCStateClass1()
{
    return EmyMain::isPCStateClass1();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d33c

int MrtMain::isPCAhead()
{
    return EmyMain::isPCAhead();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d350

int MrtMain::isPCClass7OffAxis()
{
    return EmyMain::isPCClass7OffAxis();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d384

int MrtMain::isPCInSight()
{
    ObjStat *pcObj = g_gamMan.m_pcPtr;
    u_int pc = 0;
    if (pcObj != NULL)
        pc = pcObj->m_oid;
    return emyChkInSight(10.0f, 0x1.921fb4p+2f /* 0x40c90fda, 2 pi */, 1, pc, m_param);
}

// ──── MrtOutYyy ───────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d3cc

void MrtOutYyy::Init()
{
    selectTarget(100);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d3f8

int MrtOutYyy::Exec()
{
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d400

void MrtOutYyy::Term()
{
    termCommon();
}

// ──── MrtOutSerch ─────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d41c

void MrtOutSerch::Init()
{
    int state = m_emy->m_state;
    if (state == 2) {
        selectTarget(0);
        emyStartChase(10.0f, this, m_target);
        return;
    }
    if (state != 3)
        return;
    selectTarget(100);
    emyStartChase(10.0f, this, m_target);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d4a4

int MrtOutSerch::Exec()
{
    int state = m_emy->m_state;
    if (state == 2)
        return execChasePC();
    if (state == 3)
        return execChaseAny();
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d500

void MrtOutSerch::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d51c

int MrtOutSerch::execChasePC()
{
    int r = emyChkChase(100.0f, 0x1.921fb4p+1f /* 0x40490fda, pi */, this, m_target, 0);
    if (r == 1)
        return 1;
    if (r == 2)
        return 0;
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d598

int MrtOutSerch::execChaseAny()
{
    int r = emyChkChase(100.0f, 0x1.921fb4p+1f /* 0x40490fda, pi */, this, m_target, 0);
    if (r == 1)
        return 1;
    if (r == 2)
        return 0;
    return -1;
}

// ──── MrtOutAtk ───────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d614

void MrtOutAtk::Init()
{
    selectTarget(100);
    m_hit_pc  = 0;
    m_hit_dog = 0;
    emyStartChase(10.0f, this, m_target);
    m_step = 0;
}

/* Exec's step 2 hit test: once per lunge, target within 50 of the enemy
 * (chrIsNear) takes damage word 0x18 with SE se. */
static void mrtAtkHit(MrtOutAtk *out, int *hit, ObjStat *obj, int se)
{
    if (*hit)
        return;
    u_int target = 0;
    if (obj != NULL)
        target = obj->m_oid;
    if (chrIsNear(50.0f, out->m_oid, target)) {
        *hit = 1;
        g_sndMan.playSe(se);
        scnChrRequest(0xb, out->m_oid, 0, 0, 0x18, target, 0);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014d65c // callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp, operator_new, chrIsNear, playSe, scnChrRequest (the double angle wrap is emyWrapAngle; the task new expressions; step 2's hit test, twice in mrtAtkHit)
// Steps: 0 chase until within 200 and pi / 8 ahead (lost: input 0), then
// note the target's position, turn to it over 16 frames (TskChrRot) and
// play motion 0xb; 1 once the turn is done and motion 0xb is over, lunge
// to the noted position (TskMoveTypeGen mode 0xd, kind 7, in the achr's
// move task slot); 2 while the lunge runs, hit the PC (SE 0x30) and the
// dog (SE 0x19) within 50, then motion 0x14; 3 motion 0x17; 4 motion
// 0x16; 5 input 1 once it ends.

int MrtOutAtk::Exec()
{
    int *move_id = &m_achr->m_task[0];
    int no = g_gamMan.getActorNo(m_oid);
    switch (m_step) {
    case 0:
    {
        int r = emyChkChase(200.0f, 0x1.921fb4p-2f /* 0x3ec90fda, pi / 8 */, this, m_target, 1);
        if (r == 0)
            return -1;
        if (r != 1)
            return 0;
        if (m_target_dog == 0)
            g_gamMan.m_pcPtr->getPos(m_aim);
        else
            g_gamMan.m_dogPtr->getPos(m_aim);
        float pos[4], rot[4];
        m_achr->getPos(pos);
        float a = atan2f(m_aim[0] - pos[0], m_aim[2] - pos[2]);
        m_achr->getRot(rot);
        a = emyWrapAngle(a - rot[1]);
        // 0x47000100: 32769, keep the object's heading.
        TskChrRot *rotTask = new TskChrRot(a, 0x1.000200p+15f, 1.0f, m_oid, 0x10, -1, 1, 1);
        g_taskMan.addTask(1, -1, 1, 0, rotTask, 0, NULL, 1);
        // The ROM also passes the oid in t1, which setActorMotion does not read.
        g_magSystem.m_scene.setActorMotion(0x1.11112ep-6f /* 0x3c888897 */, no, 0xb, 0, 0);
        m_step = 1;
        return -1;
    }
    case 1:
        if (!g_gamMan.isAchrTurnDone(m_oid))
            return -1;
        if (g_magSystem.m_scene.getActorMotion(no) == 0xb)
            return -1;
        if (g_taskMan.getTask(*move_id) != 0) {
            g_taskMan.killTask(*move_id);
            *move_id = -1;
        }
        {
            TskMoveTypeGen *moveTask = new TskMoveTypeGen(m_oid, 0xd, (intptr_t)m_aim, 7, 1.0f,
                                                          0x1.111110p-6f /* 0x3c888888, 1 / 60 */);
            g_taskMan.addTask(1, -1, 1, 0, moveTask, 0, move_id, 0x11);
        }
        m_step = 2;
        return -1;
    case 2:
        if (g_taskMan.getTask(m_achr->m_task[0]) == 0) {
            m_emy->m_input_flg_motion = 1;
            setMotion(0x14, 0);
            m_step = 3;
            return -1;
        }
        mrtAtkHit(this, &m_hit_pc, g_gamMan.m_pcPtr, 0x30);
        mrtAtkHit(this, &m_hit_dog, g_gamMan.m_dogPtr, 0x19);
        return -1;
    case 3:
        if (!g_gamMan.isAchrMotionEnd(m_oid))
            return -1;
        m_emy->m_input_flg_motion = 1;
        setMotion(0x17, 0);
        m_step = 4;
        return -1;
    case 4:
        if (!g_gamMan.isAchrMotionEnd(m_oid))
            return -1;
        m_emy->m_input_flg_motion = 1;
        setMotion(0x16, 0);
        m_step = 5;
        return -1;
    case 5:
        if (!g_gamMan.isAchrMotionEnd(m_oid))
            return -1;
        m_emy->m_input_flg_motion = 1;
        return 1;
    default:
        return -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014db88

void MrtOutAtk::Term()
{
    termCommon();
}

// ──── MrtOutWin ───────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014dba4

void MrtOutWin::Init()
{
    m_range = utlRandRangeF(100.0f, 200.0f);
    emyStartChase(10.0f, this, m_target);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014dbf4

int MrtOutWin::Exec()
{
    if (m_step == 0 &&
        emyChkChase(m_range, 0x1.921fb4p-1f /* 0x3f490fda, pi / 4 */, this, m_target, 1) != 0) {
        setMotion(0x15, 0x100);
        m_step = 1;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014dc60

void MrtOutWin::Term()
{
    termCommon();
}
