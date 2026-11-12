/* ==========================================================================
 *  emy/emy_pig.cpp
 *
 *  PigMain (enemy type 1) and its own outs (0x15a17c..0x15c2a4). The Got's
 *  twin: the same decide / exec / after, the same tests gated by the next
 *  state (11 Pickup needs an item, 12 Parts a part), the same sight test.
 *  What is the Pig's own: its part kinds (0xbd3..0xbd7, the Got's are
 *  0xbce..0xbd2), a pickup item must not have object flag 1 (the Got
 *  instead skips the scissors without attr bits 2 / 4), and a sixteenth
 *  state, the tackle.
 * ======================================================================== */
#include "emy/emy_pig.h"

#include <math.h>
#include <string.h>

#include "common/rosepan_stub.h"

#include "emy/emy_dwf.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magsystem.h"
#include "sequence/sequence.h"
#include "task/tsk_enemy.h"
#include "task/tsk_scene.h"
#include "utl/utlrand.h"

static u_int pigFindPickupItem(ACHRstat *achr);
static u_int pigFindPart(ACHRstat *achr);

static const float PIG_PI = 0x1.921fb4p+1f;     /* 0x40490fda */

static u_int pigPCOid()
{
    ObjStat *pc = g_gamMan.m_pcPtr;
    return pc != NULL ? pc->m_oid : 0;
}

static u_int pigDogOid()
{
    ObjStat *dog = g_gamMan.m_dogPtr;
    return dog != NULL ? dog->m_oid : 0;
}

/* chk_motion_end inline: the achr's motion ended sets the emy's motion
 * input. */
static bool pigMotionEnd(EmyOut *out)
{
    if (!g_gamMan.isAchrMotionEnd(out->m_oid))
        return false;
    out->m_emy->m_input_flg_motion = 1;
    return true;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015a17c // callcheck: operator_new, EmyOut (new XxxOut: the inline constructors call EmyOut::EmyOut)

void PigMain::initOut()
{
    m_motion_list[1]  = 10;
    m_motion_list[6]  = 2;
    m_motion_list[8]  = 0x2e;
    m_motion_list[9]  = 0x24;
    m_motion_list[11] = 8;
    m_motion_list[12] = 9;
    m_motion_list[13] = 0xe;
    m_motion_list[14] = 0xf;
    m_motion_list[2]  = 3;
    m_motion_list[3]  = 7;
    m_motion_list[4]  = 0xb;
    m_state_flg[15]   = 1;
    m_motion_list[0]  = 1;
    m_motion_list[10] = 1;
    m_state_flg[0]    = 1;
    m_state_flg[2]    = 1;
    m_state_flg[7]    = 1;
    m_state_flg[3]    = 1;
    m_state_flg[11]   = 1;
    m_state_flg[10]   = 1;
    m_state_flg[12]   = 1;
    m_state_flg[13]   = 1;
    m_state_flg[5]    = 1;
    m_seen            = 0;
    m_265             = 0;
    m_264             = 0;
    m_state_flg[1]    = 0;
    m_state_flg[8]    = 0;
    m_state_flg[9]    = 0;
    m_state_flg[4]    = 0;
    m_state_flg[6]    = 0;
    m_state_flg[14]   = 0;
    m_state_flg[16]   = 0;

    m_out[1]  = new DwfOutDisappear(m_oid, m_param, this);
    m_out[2]  = new DwfOutAppear(m_oid, m_param, this);
    m_out[3]  = new DwfOutAtkNear(m_oid, m_param, this);
    m_out[4]  = new DwfOutDead(m_oid, m_param, this);
    m_out[5]  = new DwfOutStared(m_oid, m_param, this);
    m_out[6]  = new DwfOutWin(m_oid, m_param, this);
    m_out[7]  = new DwfOutSerch(m_oid, m_param, this);
    m_out[8]  = new DwfOutWalk(m_oid, m_param, this);
    m_out[9]  = new DwfOutWait(m_oid, m_param, this);
    m_out[10] = new DwfOutAtkFar(m_oid, m_param, this);
    m_out[11] = new DwfOutPickup(m_oid, m_param, this);
    m_out[12] = new PigOutParts(m_oid, m_param, this);
    m_out[13] = new PigOutTackle(m_oid, m_param, this);
    m_out[14] = new DwfOutFort(m_oid, m_param, this);
    m_out[15] = new DwfOutIrritate(m_oid, m_param, this);
    m_out[16] = new PigOutTest(m_oid, m_param, this);

    m_pickup      = pigFindPickupItem(m_param);
    m_part        = pigFindPart(m_param);
    m_parsonality = (u_char)utlRandRange(0, 2);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015a580 // callcheck: ~EmyMain, operator_delete (the base destructor and the deleting variant are the compiler's)

PigMain::~PigMain()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015a5e0 // callcheck: peek (isBlocked, inline in the ROM)
// Only with decide on and the motion input word set. Every test runs and
// the last true one wins: 21 down to 13, the sight edge (5 into sight, 6
// out of it; while it stays in sight 12 down to 7), then the notify bits
// (4 bit 1, 3 bit 8, 2 bit 4) and 1. On GamMan's reset frame both
// targets are looked up again first.

void PigMain::decide()
{
    if (g_gamMan.m_flags & 0x40000) {
        m_pickup = pigFindPickupItem(m_param);
        m_part   = pigFindPart(m_param);
    }
    if (m_decide_sw == 0)
        return;
    if (m_input_flg_motion == 0)
        return;
    if (m_req != -1)
        return;

    if (m_input[21] && isPCStateClass4())
        m_req = 0x15;
    if (m_input[20] && testAttr8Or10())
        m_req = 0x14;
    if (m_input[19] && testAttr2Or4())
        m_req = 0x13;
    if (m_input[18] && testAttr1())
        m_req = 0x12;
    if (m_input[17] && testAttr2And4())
        m_req = 0x11;
    if (m_input[16] && testAttr8And10())
        m_req = 0x10;
    if (m_input[15] && isHalfLife())
        m_req = 0xf;
    if (m_input[14] && isLowLife())
        m_req = 0xe;
    if (m_input[13] && hasOid0Twice())
        m_req = 0xd;

    u_char seen = (u_char)isTargetInSight();
    if (m_seen == 0) {
        m_seen = seen;
        if (seen != 0 && m_input[5] && !isBlocked(5))
            m_req = 5;
    } else {
        m_seen = seen;
        if (seen == 0) {
            if (m_input[6] && !isBlocked(6))
                m_req = 6;
        } else {
            if (m_input[12] && isPCOrDogInSight())
                m_req = 0xc;
            if (m_input[11] && isPCNear())
                m_req = 0xb;
            if (m_input[10] && isPCClass7OffAxis())
                m_req = 10;
            if (m_input[9] && isPCStateClass2())
                m_req = 9;
            if (m_input[8] && isPCAhead())
                m_req = 8;
            if (m_input[7] && isPCStateClass1())
                m_req = 7;
        }
    }

    if (m_input[4] && (m_notify & 1) && !isBlocked(4))
        m_req = 4;
    if (m_input[3] && (m_notify & 8) && !isBlocked(3))
        m_req = 3;
    if (m_input[2] && (m_notify & 4) && !isBlocked(2))
        m_req = 2;
    if (m_input[1] && testBarkReact())
        m_req = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015aa8c
// Notify bit 2 (killed) forces state 4; bit 1 (hit) with no input restarts
// the current state; an input the graph has no move for keeps it.

void PigMain::exec()
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
        m_notify = 0;
        int state = m_map->next(m_req);
        m_265  = 0;
        m_264  = 0;
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
// ROR 0x0015ab48

void PigMain::after()
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
// ROR 0x0015abc4
// The ROM returns 0 (the slot's value is never read).

void PigMain::selectOut()
{
    if (m_state > 0 && m_state <= 16) {
        m_emy_out = m_out[m_state];
        m_emy_out->Init();
    }
    int n = m_trans->m_inputs;
    const char *row = m_trans->table() + m_state * n;
    // PORT: the ROM writes as many words as the table has inputs; the
    // host array holds 24 (EE 0x204..0x264).
    if (n > 24)
        n = 24;
    for (int i = 0; i < n; i++)
        m_input[i] = row[i] != 0;
}

// ──────────────────────────────────────────────────────────────────────
// Inline in every test below and in decide (no ROM address of its own).

bool PigMain::isBlocked(int input)
{
    int next = m_map->peek(input);
    return (next == 0xb && m_pickup == 0) || (next == 0xc && m_part == 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015ac8c

bool PigMain::isPCSameArea()
{
    return EmyMain::isPCSameArea();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015aca0 // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::testBarkReact()
{
    if (isBlocked(1))
        return 0;
    return EmyMain::testBarkReact();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015ad18 // callcheck: isPCSameArea (PigMain's wrapper)
// Both tests run (no short cut in the ROM).

bool PigMain::isTargetInSight()
{
    if (!isPCSameArea())
        return false;

    float range;
    int   useAngle;
    if ((m_param->m_attr[0] & 1) == 0 && m_state != 1) {
        useAngle = 1;
        m_param->getParam(0x5f, &range);
    } else {
        switch (g_magSystem.getPCStateClass()) {
        case 2:
        case 3:
        case 7:
            range = 200.0f;
            break;
        case 6:
            range = 100.0f;
            break;
        default:
            range = 60.0f;
            break;
        }
        useAngle = 0;
    }
    int pc  = emyChkInSight(range, PIG_PI, useAngle, pigPCOid(), m_param);
    int dog = emyChkInSight(range, PIG_PI, useAngle, pigDogOid(), m_param);
    return pc != 0 || dog != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015ae78 // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::isPCStateClass1()
{
    if (isBlocked(7))
        return 0;
    return EmyMain::isPCStateClass1();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015aef0 // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::isPCAhead()
{
    if (isBlocked(8))
        return 0;
    return EmyMain::isPCAhead();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015af68 // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::isPCStateClass2()
{
    if (isBlocked(9))
        return 0;
    return EmyMain::isPCStateClass2();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015afe0 // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::isPCClass7OffAxis()
{
    if (isBlocked(10))
        return 0;
    return EmyMain::isPCClass7OffAxis();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015b058 // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::isPCNear()
{
    if (isBlocked(0xb))
        return 0;
    return EmyMain::isPCNear();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015b0dc // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::isPCOrDogInSight()
{
    if (isBlocked(0xc))
        return 0;
    return EmyMain::isPCOrDogInSight();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015b154 // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::hasOid0Twice()
{
    if (isBlocked(0xd))
        return 0;
    return EmyMain::hasOid0Twice();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015b1cc // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::isLowLife()
{
    if (isBlocked(0xe))
        return 0;
    return EmyMain::isLowLife();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015b244 // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::isHalfLife()
{
    if (isBlocked(0xf))
        return 0;
    return EmyMain::isHalfLife();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015b2bc // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::testAttr8And10()
{
    if (isBlocked(0x10))
        return 0;
    u_int attr = m_param->m_attr[0];
    return (attr & 8) != 0 && (attr & 0x10) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015b34c // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::testAttr2And4()
{
    if (isBlocked(0x11))
        return 0;
    u_int attr = m_param->m_attr[0];
    return (attr & 2) != 0 && (attr & 4) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015b3dc // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::testAttr1()
{
    if (isBlocked(0x12))
        return 0;
    return m_param->m_attr[0] & 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015b454 // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::testAttr2Or4()
{
    if (isBlocked(0x13))
        return 0;
    u_int attr = m_param->m_attr[0];
    return (attr & 2) != 0 || (attr & 4) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015b4e4 // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::testAttr8Or10()
{
    if (isBlocked(0x14))
        return 0;
    u_int attr = m_param->m_attr[0];
    return (attr & 8) != 0 || (attr & 0x10) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015b574 // callcheck: peek (isBlocked, inline in the ROM)

int PigMain::isPCStateClass4()
{
    if (isBlocked(0x15))
        return 0;
    return EmyMain::isPCStateClass4();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015b5ec // callcheck: operator_new[], operator_delete[] (the 0x40 list nodes)
// gotFindPickupItem with another gate: the type-5 fchrs within 700 with
// flag 0x800000 and without 0x4000000; of those with an oid, without
// object flag 1 and that no other enemy targets, the reachable one with
// the smallest |dx| + |dz| under 700 + 700. 0 none.

static u_int pigFindPickupItem(ACHRstat *achr)
{
    SeqPool<FCHRstat *> fchrs;
    fchrs.build(new SeqNode<FCHRstat *>[0x40], 0x40);
    float pos[4];
    achr->getPos(pos);
    g_gamMan.collectFchrsNear(pos, 700.0f, &fchrs, 5, 0x800000, 0x4000000);

    float bestX = 700.0f;
    float bestZ = 700.0f;
    u_int found = 0;
    for (SeqNode<FCHRstat *> *node = fchrs.m_used.m_head.m_next; node != &fchrs.m_used.m_head;
         node = node->m_next) {
        u_int oid = node->m_data->m_oid;
        if (oid == 0 || g_gamMan.testObjFlags(oid, 1) || g_emyManager.isEmyTarget(oid))
            continue;
        float at[4];
        node->m_data->getPos(at);
        at[0] -= pos[0];
        at[2] -= pos[2];
        if (fabsf(at[0]) + fabsf(at[2]) < fabsf(bestX) + fabsf(bestZ) && emyCanReach(pos, at)) {
            bestX = at[0];
            bestZ = at[2];
            found = oid;
        }
    }
    delete[] fchrs.m_node;
    return found;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015b8b8 // callcheck: operator_new[], operator_delete[] (the 0x40 list nodes)
// gotFindPart with the Pig's kinds: 0xbd3 bit 1, 0xbd7 bit 2, 0xbd5 bit 4,
// 0xbd6 bit 8, 0xbd4 bit 0x10 (the same system data pairs 0x4f / 0x50 ..
// 0x57 / 0x58). Nothing while it holds an item (param 0x67) or has attr
// bits 2 and 4, or 8 and 0x10; the reachable one with the smallest
// dx^2 + dz^2 under 980000. 0 none.

static u_int pigFindPart(ACHRstat *achr)
{
    SeqPool<FCHRstat *> fchrs;
    fchrs.build(new SeqNode<FCHRstat *>[0x40], 0x40);

    u_int held;
    achr->getParam(0x67, &held);
    u_int attr = achr->m_attr[0];
    if (held != 0 || ((attr & 2) && (attr & 4)) || ((attr & 8) && (attr & 0x10))) {
        delete[] fchrs.m_node;
        return 0;
    }

    float bestX = 490000.0f;
    float bestZ = 490000.0f;
    float pos[4];
    achr->getPos(pos);
    u_int found = 0;
    g_gamMan.collectFchrsNear(pos, 700.0f, &fchrs, 5, 0, 0);
    for (SeqNode<FCHRstat *> *node = fchrs.m_used.m_head.m_next; node != &fchrs.m_used.m_head;
         node = node->m_next) {
        u_int oid = node->m_data->m_oid;
        if (oid == 0 || g_emyManager.isEmyTarget(oid))
            continue;
        short kind = node->m_data->m_kind;
        int field;
        if (kind == 0xbd3 && (achr->m_attr[0] & 1))
            field = 0x4f;
        else if (kind == 0xbd7 && (achr->m_attr[0] & 2))
            field = 0x51;
        else if (kind == 0xbd5 && (achr->m_attr[0] & 4))
            field = 0x53;
        else if (kind == 0xbd6 && (achr->m_attr[0] & 8))
            field = 0x55;
        else if (kind == 0xbd4 && (achr->m_attr[0] & 0x10))
            field = 0x57;
        else
            continue;
        short pair[2];
        g_gamMan.m_sysData.getField((u_short)achr->m_kind, field, &pair[0]);
        g_gamMan.m_sysData.getField((u_short)achr->m_kind, field + 1, &pair[1]);
        if (pair[0] != kind && pair[1] != kind)
            continue;
        float at[4];
        node->m_data->getPos(at);
        if (!emyCanReach(pos, at))
            continue;
        float dx2 = (at[0] - pos[0]) * (at[0] - pos[0]);
        float dz2 = (at[2] - pos[2]) * (at[2] - pos[2]);
        if (dx2 + dz2 < bestX + bestZ) {
            bestX = dx2;
            bestZ = dz2;
            found = oid;
        }
    }
    delete[] fchrs.m_node;
    return found;
}

// ──── PigOutParts ─────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015bd58

void PigOutParts::Init()
{
    u_int attr = m_achr->m_attr[0];
    int motion;
    if ((attr & 8) && (attr & 0x10))
        motion = 10;
    else if ((attr & 8) || (attr & 0x10))
        motion = 1;
    else
        motion = 0x14;
    m_step = 0;
    setMotion(motion, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015bdbc

int PigOutParts::Exec()
{
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015bdc4

void PigOutParts::Term()
{
    termCommon();
}

// ──── PigOutTackle ────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015bde0

void PigOutTackle::Init()
{
    selectNearTarget(100);
    m_hit_pc  = 0;
    m_hit_dog = 0;
    emyStartChase(300.0f, this, m_target);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015be28 // callcheck: isAchrMotionEnd (chk_motion_end inlined: pigMotionEnd)
// Step 0 waits for the chase (m_move_done 1 reached: charge motion 0x4d
// with SE 0xb; anything else: input 23). Step 1 starts the tackle after
// the charge: motion 0x4c, 28 frames (70%) or 42, the body flags on.
// Step 2 counts the frames down, hitting the PC (unless its class is 9)
// and the dog (unless Brown is in state 10 or 13) once each within 50;
// at 0 the flags go back and motion 0x4e plays. Step 3: input 22 when it
// ends.

int PigOutTackle::Exec()
{
    switch (m_step) {
    case 0:
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        if (m_move_done != 1)
            return 0x17;
        setMotion(0x4d, 0);
        m_se   = playSE(0xb);
        m_step = 1;
        break;
    case 1:
        if (pigMotionEnd(this)) {
            setMotion(0x4c, 0x100);
            m_frames = utlRandRange(0, 99) < 70 ? 0x1c : 0x2a;
            g_gamMan.resetObj(m_oid, 0x40);
            g_gamMan.setObj(m_oid, 0x10000000);
            g_gamMan.setObj(m_oid, 0x400);
            g_gamMan.setObj(m_oid, 0x4000000);
            m_step = 2;
        }
        break;
    case 2:
        if (--m_frames == 0) {
            g_gamMan.setObj(m_oid, 0x40);
            g_gamMan.resetObj(m_oid, 0x10000000);
            g_gamMan.resetObj(m_oid, 0x400);
            g_gamMan.resetObj(m_oid, 0x4000000);
            setMotion(0x4e, 0);
            m_step = 3;
            break;
        }
        if (m_hit_pc == 0 && g_magSystem.getPCStateClass() != 9) {
            u_int pc = pigPCOid();
            if (emyChkHitReach(50.0f, m_oid, 1, pc)) {
                m_hit_pc = 1;
                playSE(0);
                scnChrRequest(0xb, m_oid, 0, 0, 0x18, pc, 0);
            }
        }
        if (m_hit_dog == 0 && g_magSystem.getBwnState() != 10 && g_magSystem.getBwnState() != 0xd) {
            u_int dog = pigDogOid();
            if (emyChkHitReach(50.0f, m_oid, 1, dog)) {
                m_hit_dog = 1;
                playSE(0);
                scnChrRequest(0xb, m_oid, 0, 0, 0x18, dog, 0);
            }
        }
        break;
    case 3:
        if (pigMotionEnd(this))
            return 0x16;
        break;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015c164

void PigOutTackle::Term()
{
    termCommon();
}

// ──── PigOutTest ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015c180

void PigOutTest::Init()
{
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015c188 // callcheck: isAchrMotionEnd (chk_motion_end inlined: pigMotionEnd)
// GotOutTest::Exec with setMotion: step 1 restarts motion 0x36 and stays
// at step 1, so steps 2 (0x34) and 3 (0x30) never run in the ROM either.

int PigOutTest::Exec()
{
    switch (m_step) {
    case 0:
        setMotion(0x36, 0);
        m_step = 1;
        break;
    case 1:
        if (pigMotionEnd(this)) {
            setMotion(0x36, 0);
            m_step = 1;
        }
        break;
    case 2:
        if (pigMotionEnd(this)) {
            setMotion(0x34, 0);
            m_step = 3;
        }
        break;
    case 3:
        if (pigMotionEnd(this)) {
            setMotion(0x30, 0);
            m_step = 1;
        }
        break;
    }
    return -1;
}
