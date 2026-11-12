/* ==========================================================================
 *  emy/emy_dwf.cpp
 *
 *  DwfMain (enemy type 0) and the outs DwfOut* it shares with the Pig and
 *  the Got (0x15c2a4..0x16265c). DwfMain is the Pig's twin: the same
 *  decide / exec / after, the same tests gated by the next state, the
 *  same finders. What is the Dwf's own: its part kinds (0xbbf..0xbc3),
 *  state 14 (Irritate) is blocked with attr bits 8 and 0x10, its sight
 *  test skips the same-area check and caps the range at param 0x5f, and
 *  initOut also picks a voice and the collision shape 1.
 *
 *  Not written yet (stubs in lib/unassigned_stubs.cpp): DwfOutAtkNear,
 *  Fort and Dead.
 * ======================================================================== */
#include "emy/emy_dwf.h"

#include <math.h>
#include <string.h>

#include "common/rosepan_stub.h"
#include "dog/dogactor.h"

#include "emy/emy_manager.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "mag/magscenecmd.h"
#include "mag/magsystem.h"
#include "sequence/sequence.h"
#include "snd/sndman.h"
#include "task/tsk_enemy.h"
#include "task/tsk_man.h"
#include "task/tsk_scene.h"
#include "utl/utlrand.h"

static u_int dwfFindPickupItem(ACHRstat *achr);
static u_int dwfFindPart(ACHRstat *achr);

static const float DWF_PI    = 0x1.921fb4p+1f;  /* 0x40490fda */
static const float DWF_PI_2  = 0x1.921fb4p+0f;  /* 0x3fc90fda */
static const float DWF_PI_4  = 0x1.921fb4p-1f;  /* 0x3f490fda */
static const float DWF_PI_8  = 0x1.921fb4p-2f;  /* 0x3ec90fda */
static const float DWF_PI_10 = 0x1.41b2f6p-2f;  /* 0x3ea0d97b */
static const float DWF_PI_5  = 0x1.41b2f6p-1f;  /* 0x3f20d97b */
static const float DWF_PI_6  = 0x1.0c1522p-1f;  /* 0x3f060a91 */
/* A chase stops between this share of its range and all of it. */
static const float DWF_RANGE_LO = 0x1.999998p-1f; /* 0x3f4ccccc */

/* The script globals the scripted win reads (bss 0x7b205c + 0x5cc /
 * 0x5d0, g_sequence.m_global): the scene wants it, the scene ends it. */
enum { DWF_GLOBAL_WIN = 0x173, DWF_GLOBAL_WIN_END = 0x174 };

/* DwfOutStared: the stare's frames (0x2f1068) and its end input per type
 * (0x2f1070). */
static const u_short s_dwf_stare_limit[3] = { 50, 100, 150 };       // ROR 0x002f1068
static const int     s_dwf_stare_end[3]   = { 0x16, 0x16, 0x16 };   // ROR 0x002f1070
/* DwfOutWin's scripted spot (0x2f1090; y 0x43e2a666, 453.3). */
static const float   s_dwf_win_pos[4] = { -70.0f, 0x1.c54cccp+8f, -1780.0f, 0.0f }; // ROR 0x002f1090
/* DwfOutIrritate's end input per type (0x2f11a8). */
static const int     s_dwf_irritate_end[3] = { 0x17, 0x17, 0x17 };  // ROR 0x002f11a8
/* DwfOutAtkNear per type: the swing's reach by the item's class (field
 * 0x28 4 / other), the chase ranges of the claw and the grab, the inputs
 * of a miss and an end, the PC's struggle frames, the grab's reach; the
 * ride motion by side. */
static const float   s_dwf_near_reach4[3] = { 55.0f, 75.0f, 78.0f };   // ROR 0x002f0fc0
static const float   s_dwf_near_reach[3]  = { 55.0f, 80.0f, 72.0f };   // ROR 0x002f0fd0
static const float   s_dwf_near_claw[3]   = { 60.0f, 87.0f, 100.0f };  // ROR 0x002f0fe0
static const float   s_dwf_near_grab[3]   = { 90.0f, 100.0f, 110.0f }; // ROR 0x002f0ff0
static const int     s_dwf_near_miss[3]   = { 0x17, 0x17, 0x17 };      // ROR 0x002f1000
static const int     s_dwf_near_end[3]    = { 0x16, 0x16, 0x16 };      // ROR 0x002f1010
static const int     s_dwf_near_hold[3]   = { 15, 20, 18 };            // ROR 0x002f1020
static const float   s_dwf_grab_reach[3]  = { 45.0f, 65.0f, 65.0f };   // ROR 0x002f1030
static const int     s_dwf_grab_ride[2]   = { 0x39, 0x38 };            // ROR 0x002f1040
/* DwfOutFort per type: the leap's reach (+ 20 to start it), the ride
 * motion by side, the struggle frames, the inputs. */
static const float   s_dwf_fort_reach[3]  = { 80.0f, 90.0f, 90.0f };   // ROR 0x002f1148
static const int     s_dwf_fort_ride[2]   = { 0x39, 0x38 };            // ROR 0x002f1158
static const int     s_dwf_fort_hold[3]   = { 15, 20, 18 };            // ROR 0x002f1160
static const int     s_dwf_fort_quit[3]   = { 0x17, 0x17, 0x17 };      // ROR 0x002f1170
static const int     s_dwf_fort_end[3]    = { 0x16, 0x16, 0x16 };      // ROR 0x002f1180
/* DwfOutDead's input once up again. */
static const int     s_dwf_dead_end[3]    = { 0x17, 0x17, 0x17 };      // ROR 0x002f11f8

static u_int dwfPCOid()
{
    ObjStat *pc = g_gamMan.m_pcPtr;
    return pc != NULL ? pc->m_oid : 0;
}

static u_int dwfDogOid()
{
    ObjStat *dog = g_gamMan.m_dogPtr;
    return dog != NULL ? dog->m_oid : 0;
}

/* chk_motion_end inline: the achr's motion ended sets the emy's motion
 * input. */
static bool dwfMotionEnd(EmyOut *out)
{
    if (!g_gamMan.isAchrMotionEnd(out->m_oid))
        return false;
    out->m_emy->m_input_flg_motion = 1;
    return true;
}

/* The held item (param 0x67) handed over by the throw task: no longer
 * carried (resetObj 0x4000000), the achr's item params cleared (kind
 * 0xa00 = none). */
static void dwfReleaseThrown(EmyOut *out)
{
    u_int held;
    out->m_achr->getParam(0x67, &held);
    g_gamMan.resetObj(held, 0x4000000);
    short none = 0xa00;
    out->m_achr->setParam(0x66, &none);
    held = 0;
    out->m_achr->setParam(0x67, &held);
}

/* The throw: TskAttack kind 0x15 at the out's target (words 3 / 4: pi / 2
 * and 20). PORT: the ROM leaves word 1 as the stack had it; zero here. */
static void dwfStartThrow(EmyOut *out)
{
    int prm[5] = {};
    float angle = DWF_PI_2;
    float dist  = 20.0f;
    prm[0] = (int)out->m_target;
    prm[2] = 0x15;
    memcpy(&prm[3], &angle, sizeof(float));
    memcpy(&prm[4], &dist, sizeof(float));
    scnAttackStart(out->m_oid, prm);
}

/* The scripted win keeps the enemy out of decide and the secta lists. */
static void dwfWinHold(EmyOut *out)
{
    out->m_emy->setDecideSw(0);
    g_gamMan.setObj(out->m_oid, 0x400);
    g_gamMan.resetObj(out->m_oid, 0x40);
    g_gamMan.setObj(out->m_oid, 0x10000000);
    g_gamMan.setObj(out->m_oid, 0x400);
}

/* Inline in the outs: the Got (type 2) holding the scissors (kind 0xa2e)
 * plays their own motion instead. */
static int dwfScissorsMotion(EmyOut *out, int kind, int motion, int scissors)
{
    if (kind != 2)
        return motion;
    u_int held;
    out->m_achr->getParam(0x67, &held);
    return g_gamMan.getObjKind(held) == 0xa2e ? scissors : motion;
}

/* The grab (DwfOutAtkNear mode 2, DwfOutFort), inline in both: the PC
 * may be caught unless already held (commandPC 0x16) and only with game
 * flag 4 without 0x2000. */
static bool dwfGrabAllowed()
{
    if (g_magSystem.m_scene.commandPC(0x16, NULL) != 0)
        return false;
    if (g_gamMan.m_flags & 0x2000)
        return false;
    return (g_gamMan.m_flags & 4) != 0;
}

/* The catch: the PC's node by side (*node is set even when the side's
 * EmyManager slot is taken: false then); decide off, off the secta lists,
 * the slot, state2 1, the achr at the origin parented to the node, the PC
 * told (commandPC 0). */
static bool dwfGrabCatch(EmyOut *out, int no, int targetNo, int back, int *node)
{
    DogActor &target = g_magSystem.m_scene.m_actors[targetNo];
    *node = target.getNodeIndexByName(back == 0 ? "p_sp_f" : "p_sp_b");
    if (g_emyManager.hasOid(back))
        return false;
    out->m_emy->setDecideSw(0);
    g_gamMan.resetObj(out->m_oid, 0x40);
    g_gamMan.setObj(out->m_oid, 0x10000000);
    g_gamMan.setObj(out->m_oid, 0x400);
    g_emyManager.setOid(back, out->m_oid);
    out->m_emy->m_state2 = 1;
    float pos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float rot[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    out->m_achr->ObjStat::setPos(pos);
    out->m_achr->ObjStat::setRot(rot);
    g_magSystem.setActorParent(no, targetNo, *node);
    g_magSystem.m_scene.commandPC(0, NULL);
    return true;
}

/* Thrown off: the PC freed (commandPC 1), unparented, the slot cleared,
 * on the floor behind the PC's node, the fall motion by side. */
static void dwfGrabRelease(EmyOut *out, int no, int *node, int *back)
{
    g_magSystem.m_scene.commandPC(1, NULL);
    g_magSystem.setActorParent(no, -1, 0);
    g_emyManager.clearOid(*back);
    out->m_emy->m_state2 = 0;
    out->snapToTarget(node, back);
    g_gamMan.setObj(out->m_oid, 0x4000000);
    out->changeMotion(*back == 0 ? 0x3b : 0x3a, 0);
}

/* Back on the secta lists, collisions on. */
static void dwfGrabFlagsOff(EmyOut *out)
{
    g_gamMan.setObj(out->m_oid, 0x40);
    g_gamMan.resetObj(out->m_oid, 0x10000000);
    g_gamMan.resetObj(out->m_oid, 0x400);
    g_gamMan.resetObj(out->m_oid, 0x4000000);
}

/* Decide off while it gets up. */
static void dwfHoldOff(EmyOut *out)
{
    out->m_emy->setDecideSw(0);
    g_gamMan.setObj(out->m_oid, 0x400);
}

/* Up again: decide on (the ROM sets it twice), the input end. */
static int dwfGetUpEnd(EmyOut *out, int end)
{
    out->m_emy->setDecideSw(1);
    out->m_emy->setDecideSw(1);
    g_gamMan.resetObj(out->m_oid, 0x400);
    return end;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015c2a4 // callcheck: ~EmyMain, operator_delete (the base destructor and the deleting variant are the compiler's)

DwfMain::~DwfMain()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015c304 // callcheck: operator_new, EmyOut (new XxxOut: the inline constructors call EmyOut::EmyOut)

void DwfMain::initOut()
{
    m_motion_list[2]  = 3;
    m_motion_list[8]  = 0x2e;
    m_motion_list[9]  = 0x24;
    m_motion_list[11] = 8;
    m_motion_list[12] = 9;
    m_motion_list[13] = 0xe;
    m_motion_list[14] = 0xf;
    m_motion_list[3]  = 7;
    m_motion_list[4]  = 0xb;
    m_motion_list[6]  = 2;
    m_motion_list[1]  = 10;
    m_motion_list[0]  = 1;
    m_motion_list[10] = 1;
    m_state_flg[0]    = 1;
    m_state_flg[2]    = 1;
    m_state_flg[7]    = 1;
    m_state_flg[3]    = 1;
    m_state_flg[11]   = 1;
    m_state_flg[10]   = 1;
    m_state_flg[12]   = 1;
    m_state_flg[5]    = 1;
    m_seen            = 0;
    m_265             = 0;
    m_264             = 0;
    m_state_flg[1]    = 0;
    m_state_flg[8]    = 0;
    m_state_flg[9]    = 0;
    m_state_flg[4]    = 0;
    m_state_flg[6]    = 0;
    m_state_flg[13]   = 0;
    m_state_flg[14]   = 1;
    m_state_flg[15]   = 0;

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
    m_out[12] = new DwfOutParts(m_oid, m_param, this);
    m_out[13] = new DwfOutFort(m_oid, m_param, this);
    m_out[14] = new DwfOutIrritate(m_oid, m_param, this);
    m_out[15] = new DwfOutTest(m_oid, m_param, this);

    GamObjColli colli;
    m_param->getParam(GAM_PRM_COL_PRM, &colli);
    colli.m_shape = 1;
    m_param->setParam(GAM_PRM_COL_PRM, &colli);

    m_parsonality = (u_char)utlRandRange(0, 2);
    m_voice       = (u_char)utlRandRange(0, 3);
    m_pickup      = dwfFindPickupItem(m_param);
    m_part        = dwfFindPart(m_param);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015c710 // callcheck: peek (isBlocked, inline in the ROM)
// PigMain::decide: only with decide on and the motion input word set;
// every test runs and the last true one wins: 21 down to 13, the sight
// edge (5 into sight, 6 out of it; while it stays in sight 12 down to 7),
// then the notify bits (4 bit 1, 3 bit 8, 2 bit 4) and 1. On GamMan's
// reset frame both targets are looked up again first.

void DwfMain::decide()
{
    if (g_gamMan.m_flags & 0x40000) {
        m_pickup = dwfFindPickupItem(m_param);
        m_part   = dwfFindPart(m_param);
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
// ROR 0x0015cc84
// Notify bit 2 (killed) forces state 4; bit 1 (hit) with no input restarts
// the current state; an input the graph has no move for keeps it.

void DwfMain::exec()
{
    m_changed = 0;
    if (m_notify & 2) {
        m_state      = 4;
        m_map->m_cur = 4;
        m_changed    = 1;
    } else if (m_req != -1) {
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
    } else if (m_notify & 1) {
        m_map->m_cur = m_state;
        m_changed    = 1;
    }
    m_notify = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015cd30

void DwfMain::after()
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
// ROR 0x0015cdac
// The ROM returns 0 (the slot's value is never read).

void DwfMain::selectOut()
{
    if (m_state > 0 && m_state <= 15) {
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

bool DwfMain::isBlocked(int input)
{
    int next = m_map->peek(input);
    if (next == 0xb && m_pickup == 0)
        return true;
    if (next == 0xc && m_part == 0)
        return true;
    u_int attr = m_param->m_attr[0];
    return next == 0xe && (attr & 8) && (attr & 0x10);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015ce74

bool DwfMain::isPCSameArea()
{
    return EmyMain::isPCSameArea();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015ce88 // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::testBarkReact()
{
    if (isBlocked(1))
        return 0;
    return EmyMain::testBarkReact();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015cf30
// Unlike the Pig and the Got, no same-area test; both sight tests run.

bool DwfMain::isTargetInSight()
{
    float range;
    m_param->getParam(0x5f, &range);
    int useAngle;
    if ((m_param->m_attr[0] & 1) == 0 && m_state != 1) {
        useAngle = 1;
    } else {
        float cls;
        switch (g_magSystem.getPCStateClass()) {
        case 2:
        case 3:
        case 7:
            cls = 200.0f;
            break;
        case 6:
            cls = 100.0f;
            break;
        default:
            cls = 60.0f;
            break;
        }
        useAngle = 0;
        if (range > cls)
            range = cls;
    }
    int pc  = emyChkInSight(range, DWF_PI, useAngle, dwfPCOid(), m_param);
    int dog = emyChkInSight(range, DWF_PI, useAngle, dwfDogOid(), m_param);
    return pc != 0 || dog != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015d08c // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::isPCStateClass1()
{
    if (isBlocked(7))
        return 0;
    return EmyMain::isPCStateClass1();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015d134 // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::isPCAhead()
{
    if (isBlocked(8))
        return 0;
    return EmyMain::isPCAhead();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015d1dc // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::isPCStateClass2()
{
    if (isBlocked(9))
        return 0;
    return EmyMain::isPCStateClass2();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015d284 // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::isPCClass7OffAxis()
{
    if (isBlocked(10))
        return 0;
    return EmyMain::isPCClass7OffAxis();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015d32c // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::isPCNear()
{
    if (isBlocked(0xb))
        return 0;
    return EmyMain::isPCNear();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015d3e0 // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::isPCOrDogInSight()
{
    if (isBlocked(0xc))
        return 0;
    return EmyMain::isPCOrDogInSight();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015d488 // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::hasOid0Twice()
{
    if (isBlocked(0xd))
        return 0;
    return EmyMain::hasOid0Twice();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015d530 // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::isLowLife()
{
    if (isBlocked(0xe))
        return 0;
    return EmyMain::isLowLife();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015d5d8 // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::isHalfLife()
{
    if (isBlocked(0xf))
        return 0;
    return EmyMain::isHalfLife();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015d680 // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::testAttr8And10()
{
    if (isBlocked(0x10))
        return 0;
    u_int attr = m_param->m_attr[0];
    return (attr & 8) != 0 && (attr & 0x10) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015d738 // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::testAttr2And4()
{
    if (isBlocked(0x11))
        return 0;
    u_int attr = m_param->m_attr[0];
    return (attr & 2) != 0 && (attr & 4) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015d7f0 // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::testAttr1()
{
    if (isBlocked(0x12))
        return 0;
    return m_param->m_attr[0] & 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015d890 // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::testAttr2Or4()
{
    if (isBlocked(0x13))
        return 0;
    u_int attr = m_param->m_attr[0];
    return (attr & 2) != 0 || (attr & 4) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015d948 // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::testAttr8Or10()
{
    if (isBlocked(0x14))
        return 0;
    u_int attr = m_param->m_attr[0];
    return (attr & 8) != 0 || (attr & 0x10) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015da00 // callcheck: peek (isBlocked, inline in the ROM)

int DwfMain::isPCStateClass4()
{
    if (isBlocked(0x15))
        return 0;
    return EmyMain::isPCStateClass4();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015daa8 // callcheck: operator_new[], operator_delete[] (the 0x40 list nodes)
// pigFindPickupItem: the type-5 fchrs within 700 with flag 0x800000 and
// without 0x4000000; of those with an oid, without object flag 1 and that
// no other enemy targets, the reachable one with the smallest |dx| + |dz|
// under 700 + 700. 0 none.

static u_int dwfFindPickupItem(ACHRstat *achr)
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
// ROR 0x0015dd74 // callcheck: operator_new[], operator_delete[] (the 0x40 list nodes)
// pigFindPart with the Dwf's kinds: 0xbbf bit 1, 0xbc3 bit 2, 0xbc1 bit 4,
// 0xbc2 bit 8, 0xbc0 bit 0x10 (system data pairs 0x4f / 0x50 .. 0x57 /
// 0x58). Nothing while it holds an item (param 0x67) or has attr bits 2
// and 4, or 8 and 0x10; the reachable one with the smallest dx^2 + dz^2
// under 980000. 0 none.

static u_int dwfFindPart(ACHRstat *achr)
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
        if (kind == 0xbbf && (achr->m_attr[0] & 1))
            field = 0x4f;
        else if (kind == 0xbc3 && (achr->m_attr[0] & 2))
            field = 0x51;
        else if (kind == 0xbc1 && (achr->m_attr[0] & 4))
            field = 0x53;
        else if (kind == 0xbc2 && (achr->m_attr[0] & 8))
            field = 0x55;
        else if (kind == 0xbc0 && (achr->m_attr[0] & 0x10))
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

// ──── DwfOutDisappear ─────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015e214

void DwfOutDisappear::Init()
{
    g_gamMan.hideObj(m_oid, 1);
    changeMotion(0x1d, 0x100);
    m_emy->m_visible = 0;
    g_gamMan.resetObj(m_oid, 0x40);
    g_gamMan.setObj(m_oid, 0x10000000);
    g_gamMan.setObj(m_oid, 0x400);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015e2a0

void DwfOutDisappear::Term()
{
    g_gamMan.hideObj(m_oid, 0);
    m_emy->m_visible = 1;
    g_gamMan.setObj(m_oid, 0x40);
    g_gamMan.resetObj(m_oid, 0x10000000);
    g_gamMan.resetObj(m_oid, 0x400);
    termCommon();
}

// ──── DwfOutAppear ────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015e324

void DwfOutAppear::Init()
{
    m_emy->m_visible = 0;
    m_achr->m_flags |= 0x400;
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015e344 // callcheck: isAchrMotionEnd (chk_motion_end inlined: dwfMotionEnd), GetGrpMotion (inline)
// Shown, out of the secta lists, the enemy's group's motion; when it
// ends, back in the lists and visible: input 22.

int DwfOutAppear::Exec()
{
    if (m_step == 0) {
        g_gamMan.hideObj(m_oid, 0);
        g_gamMan.resetObj(m_oid, 0x40);
        g_gamMan.setObj(m_oid, 0x10000000);
        g_gamMan.setObj(m_oid, 0x400);
        changeMotion(g_emyManager.GetGrpMotion(m_emy->m_grp_no), 0);
        m_step = 1;
    } else if (m_step == 1 && dwfMotionEnd(this)) {
        g_gamMan.setObj(m_oid, 0x40);
        g_gamMan.resetObj(m_oid, 0x10000000);
        g_gamMan.resetObj(m_oid, 0x400);
        m_emy->m_visible = 1;
        m_achr->m_flags &= ~0x400u;
        m_achr->m_flags &= ~0x2000u;
        return 0x16;
    }
    return -1;
}

// ──── DwfOutWait ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015e4b0

void DwfOutWait::Init()
{
    if (m_emy->GetType() == 2)
        setGotWalkMotion();
    else
        setWalkMotion();
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015e514

int DwfOutWait::Exec()
{
    return -1;
}

// ──── DwfOutWalk ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015e51c
// A type other than 0..2 keeps m_kind (no out of another type is a Dwf's).

void DwfOutWalk::Init()
{
    switch (m_emy->GetType()) {
    case 0:
        m_kind = 0;
        break;
    case 1:
        m_kind = 1;
        break;
    case 2:
        m_kind = 2;
        break;
    }
    if (m_kind == 2)
        setGotWalkMotion();
    else
        setWalkMotion();
    selectTarget(100);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015e5bc // callcheck: isAchrMotionEnd, getParam, getObjKind (chk_motion_end and the scissors test inlined: dwfMotionEnd, dwfScissorsMotion)
// Step 0 picks a point within +-500 of home (param 0x59; y the floor at
// home) and walks there when it is reachable and more than 100 away on
// both axes (step 1), else walks in place (step 2). Step 1 waits for the
// move. Step 2, after 20 frames: half the time a new point, 10% the idle
// motion (step 3), else 10 more frames. Step 3 walks again when the idle
// ends. Steps 4..6 (another idle, then a look round) are never entered in
// the ROM either.

int DwfOutWalk::Exec()
{
    m_count++;
    switch (m_step) {
    case 0: {
        float home[4];
        m_achr->getParam(0x59, home);
        float to[4];
        to[0] = home[0] + (float)utlRandRange(-500, 500);
        to[1] = home[1];
        to[2] = home[2] + (float)utlRandRange(-500, 500);
        to[1] = g_gamMan.getFloorY(home);
        bool walk = false;
        if (emyCanReach(home, to)) {
            float base[4];
            getBasePos(base);
            float dx = base[0] - to[0];
            if (dx < -100.0f || dx > 100.0f) {
                float dz = base[2] - to[2];
                walk = dz < -100.0f || dz > 100.0f;
            }
        }
        if (walk) {
            emyStartMove(0.0f, this, to);
            setStep(1);
        } else {
            if (m_kind == 2)
                setGotWalkMotion();
            else
                setWalkMotion();
            setStep(2);
        }
        break;
    }
    case 1:
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        setStep(0);
        if (m_kind == 2)
            setGotWalkMotion();
        else
            setWalkMotion();
        break;
    case 2: {
        if (m_count < 0x14)
            return -1;
        int r = utlRandRange(0, 99);
        if (r < 50) {
            setStep(0);
            return -1;
        }
        if (r >= 60) {
            m_count = 10;
            return -1;
        }
        u_int attr = m_achr->m_attr[0];
        int motion;
        if ((attr & 8) && (attr & 0x10))
            motion = 10;
        else
            motion = dwfScissorsMotion(this, m_kind, 0x14, 0x4f);
        changeMotion(motion, 0);
        setStep(3);
        break;
    }
    case 3:
        if (!dwfMotionEnd(this))
            return -1;
        setStep(0);
        if (m_kind == 2)
            setGotWalkMotion();
        else
            setWalkMotion();
        break;
    case 4: {
        if (!dwfMotionEnd(this))
            return -1;
        setStep(5);
        u_int attr = m_achr->m_attr[0];
        int motion;
        if ((attr & 8) && (attr & 0x10))
            motion = 10;
        else
            motion = dwfScissorsMotion(this, m_kind, 0x17, 0x4d);
        changeMotion(motion, 0x100);
        break;
    }
    case 5:
        if (m_count > 0x3b) {
            if (utlRandRange(0, 99) < 30) {
                setStep(6);
                changeMotion(dwfScissorsMotion(this, m_kind, 0x18, 0x4e), 0);
            } else {
                m_count = 0;
            }
        }
        break;
    case 6:
        if (!dwfMotionEnd(this))
            return -1;
        setStep(0);
        if (m_kind == 2)
            setGotWalkMotion();
        else
            setWalkMotion();
        break;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015ea8c

void DwfOutWalk::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015eaa8

void DwfOutWalk::setStep(int step)
{
    if (m_step != step) {
        m_step  = step;
        m_count = 0;
    }
}

// ──── DwfOutSerch ─────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015eac4

void DwfOutSerch::Init()
{
    selectTarget(100);
    emyStartChase(10.0f, this, m_target);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015eb00
// PORT: for a type other than 0..2 the ROM returns a register left over
// (no such enemy builds this out); the host returns 22 for any type.

int DwfOutSerch::Exec()
{
    if (emyChkChase(200.0f, DWF_PI, this, m_target, 0) == 0)
        return -1;
    m_emy->m_move->stop();
    return 0x16;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015eba8

void DwfOutSerch::Term()
{
    termCommon();
}

// ──── DwfOutPickup ────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015ebc4

void DwfOutPickup::Init()
{
    switch (m_emy->GetType()) {
    case 0:
        m_kind = 0;
        break;
    case 1:
        m_kind = 1;
        break;
    case 2:
        m_kind = 2;
        break;
    }
    m_step = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015ec30 // callcheck: isAchrMotionEnd, getParam, getObjKind (chk_motion_end and the scissors test inlined: dwfMotionEnd, dwfScissorsMotion)
// Step 1: with free hands (no item, no attr bits 1 or 2, not both 8 and
// 0x10) chases the nearest pickup item to 20 (step 2); otherwise, or with
// none, the idle motion by the attr bits (step 6). Step 2: reached, motion
// 0x19 (step 3); else input 23. Step 3: the item attached (params 0x66 /
// 0x67, step 4) or not (step 6), motion 0x1a. Step 4 stops the item (its
// params 5 and 6). Step 5: input 22 when the motion ends; step 6: input
// 23. The two per-type inputs (data 0x2f0f10 / 0x2f0f20) are 23 and 22 for
// every type. Step 0 (never set) falls into step 1 when a motion ends.

int DwfOutPickup::Exec()
{
    switch (m_step) {
    case 0:
        if (!dwfMotionEnd(this))
            return -1;
        /* fall through */
    case 1: {
        u_int held;
        m_achr->getParam(0x67, &held);
        u_int attr = m_achr->m_attr[0];
        if (held == 0 && (attr & 2) == 0 && (attr & 1) == 0 && !((attr & 8) && (attr & 0x10))) {
            m_3c = dwfFindPickupItem(m_achr);
            if (m_3c != 0) {
                emyStartChase(20.0f, this, m_3c);
                m_step = 2;
                return -1;
            }
        }
        attr = m_achr->m_attr[0];
        int motion;
        if ((attr & 8) && (attr & 0x10))
            motion = 10;
        else if ((attr & 8) || (attr & 0x10))
            motion = 1;
        else
            motion = dwfScissorsMotion(this, m_kind, 0x14, 0x4f);
        changeMotion(motion, 0);
        m_step = 6;
        break;
    }
    case 2:
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        if (m_move_done == 1) {
            changeMotion(0x19, 0);
            m_step = 3;
            return -1;
        }
        return 0x17;
    case 3:
        if (dwfMotionEnd(this)) {
            if (emyAttachItem(m_oid, m_3c) == 0) {
                m_step = 6;
            } else {
                short kind = g_gamMan.getObjKind(m_3c);
                m_achr->setParam(0x66, &kind);
                m_achr->setParam(0x67, &m_3c);
                m_step = 4;
            }
            changeMotion(0x1a, 0);
        }
        break;
    case 4: {
        FCHRstat *item = g_gamMan.getFchr(m_3c);
        float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        item->setParam(5, zero);
        item->setParam(6, zero);
        m_step = 5;
        break;
    }
    case 5:
        if (dwfMotionEnd(this))
            return 0x16;
        break;
    case 6:
        if (!dwfMotionEnd(this))
            return -1;
        return 0x17;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015ef78

void DwfOutPickup::Term()
{
    termCommon();
}

// ──── DwfOutAtkNear ───────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015ef94

void DwfOutAtkNear::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015efb0 // callcheck: getObjKind, getField (the item's fields 0x1b and 0x28)
// With an item: mode 0, the reach by the item's class (field 0x28: 4
// 0x2f0fc0, else 0x2f0fd0) plus 70 / 50 / 20 by its size (field 0x1b 3 /
// 2 / other); the Got's scissors 197. Without: mode 2 (0x2f0ff0) unless
// both asks of grab slot 0 say taken (the ROM asks slot 0 twice), the
// dog is the target, or the attr dice fail; else mode 1 (0x2f0fe0). The
// chase stops between 80 % and all of the range. A type other than 0..2
// keeps m_kind.

void DwfOutAtkNear::Init()
{
    switch (m_emy->GetType()) {
    case 1:
        m_kind = 1;
        break;
    case 0:
        m_kind = 0;
        break;
    case 2:
        m_kind = 2;
        break;
    }
    m_tries = 0;
    m_idles = 0;
    m_hits  = 0;
    m_hold  = 0;
    selectNearTarget(100);
    u_short combo;
    m_achr->getParam(0x6a, &combo);
    m_combo = combo == 0 ? 2 : utlRandPeakHi(1, combo, 4);

    u_int held;
    m_achr->getParam(0x67, &held);
    if (held != 0) {
        short kind = g_gamMan.getObjKind(held);
        u_short size, cls;
        g_gamMan.m_sysData.getField(kind, 0x1b, &size);
        g_gamMan.m_sysData.getField(kind, 0x28, &cls);
        u_int target = m_target;
        float reach = cls == 4 ? s_dwf_near_reach4[m_kind] : s_dwf_near_reach[m_kind];
        if (m_kind == 2 && kind == 0xa2e)
            m_range = 197.0f;
        else if (size == 3)
            m_range = reach + 70.0f;
        else if (size == 2)
            m_range = reach + 50.0f;
        else
            m_range = reach + 20.0f;
        emyStartChase(utlRandRangeF(m_range * DWF_RANGE_LO, m_range), this, target);
        m_mode = 0;
        m_step = 0;
        return;
    }

    bool grab = false;
    if (!(g_emyManager.hasOid(0) && g_emyManager.hasOid(0)) && m_target_dog == 0) {
        if (utlRandRange(0, 99) < 0x32 && (m_achr->m_attr[0] & 0x800000))
            grab = true;
        else if (m_achr->m_attr[1] & 1)
            grab = true;
    }
    u_int target = m_target;
    if (grab) {
        float range = s_dwf_near_grab[m_kind];
        emyStartChase(utlRandRangeF(range * DWF_RANGE_LO, range), this, target);
        m_mode = 2;
        m_step = 0;
        return;
    }
    float range = s_dwf_near_claw[m_kind];
    emyStartChase(utlRandRangeF(range * DWF_RANGE_LO, range), this, target);
    m_hit_dog = 0;
    m_step    = 0;
    m_mode    = 1;
    m_hit_pc  = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015f2fc

int DwfOutAtkNear::Exec()
{
    switch (m_mode) {
    case 0:
        return execThrow();
    case 1:
        return execClaw();
    case 2:
        return execGrab();
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015f37c // callcheck: isAchrMotionEnd, getObjKind, getParam (chk_motion_end inlined: dwfMotionEnd; the scissors test inline)
// Step 0: the chase ends: arrived, the throw (step 1), else input 23.
// Step 1: the throw task gone, decide on; after a throw that hit (always:
// startThrow counts it) or 60 %: 20 % an idle (step 2), else a new chase;
// otherwise another throw. The ROM's r > 100 branch (motion 6 or the
// scissors' 0xc, step 3) never runs. Step 2: the throw once m_target is
// in sight within the range (pi / 4), else at a motion's end 5 % (up to
// 3) another idle, then a new chase. Step 3: 20 % once motion 6, then a
// new chase.

int DwfOutAtkNear::execThrow()
{
    switch (m_step) {
    case 0:
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        if (m_move_done != 1)
            return s_dwf_near_miss[m_kind];
        break;
    case 1: {
        if (g_taskMan.getTask(m_achr->m_task[3]) != 0)
            return -1;
        m_emy->setDecideSw(1);
        int r = utlRandRange(0, 99);
        if (m_hits <= 0 && r <= 0x3b)
            break;
        m_hits = 0;
        if (r > 100) {
            changeMotion(dwfScissorsMotion(this, m_kind, 6, 0xc), 0);
            m_step = 3;
            return -1;
        }
        if (utlRandRange(0, 99) < 0x14) {
            if (m_kind == 2)
                setGotWalkMotion();
            else
                setBaseMotion();
            m_step = 2;
            return -1;
        }
        m_step = 0;
        selectNearTarget(100);
        emyStartChase(utlRandRangeF(m_range * DWF_RANGE_LO, m_range), this, m_target);
        return -1;
    }
    case 2:
        if (emyChkInSight(m_range, DWF_PI_4, 1, m_target, m_achr) != 0)
            break;
        if (!dwfMotionEnd(this))
            return -1;
        if (m_tries < 3 && utlRandRange(0, 99) < 5) {
            if (m_kind == 2)
                setGotWalkMotion();
            else
                setBaseMotion();
            m_tries++;
            return -1;
        }
        m_step = 0;
        selectNearTarget(100);
        emyStartChase(utlRandRangeF(m_range * DWF_RANGE_LO, m_range), this, m_target);
        m_tries = 0;
        return -1;
    case 3:
        if (!dwfMotionEnd(this))
            return -1;
        if (m_idles == 0 && utlRandRange(0, 99) < 0x14) {
            changeMotion(6, 0);
            m_step = 3;
            m_idles++;
            return -1;
        }
        m_step = 0;
        selectNearTarget(100);
        emyStartChase(utlRandRangeF(m_range * DWF_RANGE_LO, m_range), this, m_target);
        m_idles = 0;
        return -1;
    default:
        return -1;
    }
    if (startThrow() != 0) {
        m_step = 1;
        return -1;
    }
    return s_dwf_near_miss[m_kind];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015f744 // callcheck: FUN_001f9d90 (scnAttackStart), getField
// Decide stays off while the task runs. The parameter words: [0] the
// target, [1] the type, [2] the attack kind, [5] the scissors, [10] the
// reach. The Got's scissors cut once (0x14, 50 % on the first throw:
// m_hits is already counted then, so only when it was -1) then stab
// (0x13); any other item is swung (0x12) at the reach of its class.
// PORT: the ROM leaves the words it does not set as the stack had them.

int DwfOutAtkNear::startThrow()
{
    u_int held;
    m_achr->getParam(0x67, &held);
    if (held == 0)
        return 0;
    m_hits++;
    m_emy->setDecideSw(0);
    int prm[12] = {};
    prm[0] = (int)m_target;
    prm[1] = m_kind;
    short kind = g_gamMan.getObjKind(held);
    float reach;
    if (m_kind == 2 && kind == 0xa2e) {
        prm[5] = (int)held;
        if (m_hits == 0) {
            prm[2] = 0x13;
            if (utlRandRange(0, 99) < 0x32) {
                prm[2] = 0x14;
                m_hits = 1;
            }
        } else if (m_hits == 1) {
            prm[2] = 0x14;
        } else {
            prm[2] = 0x13;
        }
        reach = m_range;
    } else {
        prm[2] = 0x12;
        u_short cls;
        g_gamMan.m_sysData.getField(kind, 0x28, &cls);
        reach = cls == 4 ? s_dwf_near_reach4[m_kind] : s_dwf_near_reach[m_kind];
    }
    memcpy(&prm[10], &reach, sizeof(float));
    scnAttackStart(m_oid, prm);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015f8ac // callcheck: isAchrMotionEnd, getPCStateClass, getBwnState, emyChkHitReach, scnChrRequest (chk_motion_end inlined: dwfMotionEnd)
// Step 0: the chase ends: arrived, the claw (step 1), else input 23.
// Step 1: the wind-up's end, the swipe (0x40; 0x3d with attr bits 8 and
// 0x10). Step 2: frames 5..15 of the swipe hit the PC (unless class 9)
// or the dog (unless barking state 10 / 0xd) within 12: SE 5 and damage
// 0xd to m_target (with attr 8 and 0x10 damage 0x11 and the round ends).
// At the swipe's end (no PC hit after frame 14 ends the round) another
// swipe, or the recovery 0x41 / 0x3e (step 3). Step 3: 50 % input 22;
// else to 70 % with attr 0x10 but not 8 motion 6 (step 5), else an idle
// (step 4). Steps 4 / 5: the claw again once m_target is within 60
// (pi / 4), else at a motion's end another idle (10 % up to 3 / 20 % once)
// or a new chase.

int DwfOutAtkNear::execClaw()
{
    switch (m_step) {
    case 0:
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        if (m_move_done == 1 && startClaw() != 0) {
            m_step = 1;
            return -1;
        }
        return s_dwf_near_miss[m_kind];
    case 1:
        if (dwfMotionEnd(this)) {
            u_int attr = m_achr->m_attr[0];
            changeMotion((attr & 8) && (attr & 0x10) ? 0x3d : 0x40, 0);
            m_claw_frame = 0;
            m_step = 2;
        }
        return -1;
    case 2: {
        m_claw_frame++;
        if (4 < m_claw_frame && m_claw_frame < 0x10) {
            if (m_hit_pc == 0 && g_magSystem.getPCStateClass() != 9) {
                if (emyChkHitReach(12.0f, m_oid, 1, dwfPCOid()) != 0) {
                    playSE(5);
                    m_hit_pc = 1;
                    m_hits++;
                    int damage = 0xd;
                    u_int attr = m_achr->m_attr[0];
                    if ((attr & 8) && (attr & 0x10)) {
                        damage = 0x11;
                        m_hits = m_combo;
                    }
                    scnChrRequest(0xb, m_oid, 0, 0, damage, m_target, 0);
                }
            }
            if (m_hit_dog == 0 && g_magSystem.getBwnState() != 10 && g_magSystem.getBwnState() != 0xd) {
                if (emyChkHitReach(12.0f, m_oid, 1, dwfDogOid()) != 0) {
                    playSE(5);
                    m_hit_dog = 1;
                    scnChrRequest(0xb, m_oid, 0, 0, 0xd, m_target, 0);
                }
            }
        }
        if (!dwfMotionEnd(this))
            return -1;
        if (m_claw_frame > 0xe && m_hit_pc == 0)
            m_hits = m_combo;
        u_int attr = m_achr->m_attr[0];
        int motion;
        if (m_hits < m_combo) {
            m_hit_dog = 0;
            m_hit_pc  = 0;
            motion = (attr & 8) && (attr & 0x10) ? 0x3d : 0x40;
            playSE(0xb);
            m_claw_frame = 0;
        } else {
            motion = (attr & 8) && (attr & 0x10) ? 0x3e : 0x41;
            m_step = 3;
        }
        changeMotion(motion, 0);
        return -1;
    }
    case 3: {
        if (!dwfMotionEnd(this))
            return -1;
        m_hits = 0;
        int r = utlRandRange(0, 99);
        if (r < 0x32)
            return s_dwf_near_end[m_kind];
        u_int attr = m_achr->m_attr[0];
        if (r < 0x46 && (attr & 8) == 0 && (attr & 0x10) != 0) {
            changeMotion(6, 0);
            m_step = 5;
        } else {
            setBaseMotion();
            m_step = 4;
            m_tries++;
        }
        return -1;
    }
    case 4:
        if (emyChkInSight(60.0f, DWF_PI_4, 1, m_target, m_achr) == 0) {
            if (!dwfMotionEnd(this))
                return -1;
            if (m_tries < 3 && utlRandRange(0, 99) < 10) {
                setBaseMotion();
                m_tries++;
                return -1;
            }
            selectNearTarget(100);
            float range = s_dwf_near_claw[m_kind];
            emyStartChase(utlRandRangeF(range * DWF_RANGE_LO, range), this, m_target);
            m_step  = 0;
            m_tries = 0;
            return -1;
        }
        break;
    case 5:
        if (!dwfMotionEnd(this))
            return -1;
        if (emyChkInSight(60.0f, DWF_PI_4, 1, m_target, m_achr) == 0) {
            if (m_idles == 0 && utlRandRange(0, 99) < 0x14) {
                changeMotion(6, 0);
                m_step = 5;
                m_idles++;
                return -1;
            }
            selectNearTarget(100);
            float range = s_dwf_near_claw[m_kind];
            emyStartChase(utlRandRangeF(range * DWF_RANGE_LO, range), this, m_target);
            m_step  = 0;
            m_idles = 0;
            return -1;
        }
        break;
    default:
        return -1;
    }
    if (startClaw() == 0)
        return s_dwf_near_miss[m_kind];
    m_step = 1;
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015fee8

int DwfOutAtkNear::startClaw()
{
    u_int attr = m_achr->m_attr[0];
    if (attr & 1)
        return 0;
    changeMotion((attr & 8) && (attr & 0x10) ? 0x3c : 0x3f, 0);
    playSE(0xb);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015ff54 // callcheck: isAchrMotionEnd, getTask, dbgAssertFailed, i3rGetNodeIndexByName, fptodp, dpcmp, setObj, resetObj, setDecideSw, setOid, clearOid, setPos, setRot, setActorParent, commandPC, scnChrRequest, hasOid, snapToTarget (chk_motion_end inlined: dwfMotionEnd; the grab: dwfGrabCatch / dwfGrabRelease / dwfGrabFlagsOff / dwfHoldOff; DogModel::getNodeIndexByName inline)
// Step 0: the chase ends: arrived and not attr bits 8 + 0x10 or 2 + 4,
// the leap (step 1); else input 23. Step 1: frames 17..23 of the leap
// catch the PC within the type's reach (0x2f1030, pi / 5, all round) from
// the front (angle under pi / 4) or behind (over 3 pi / 4), not from the
// side; ride motion 0x39 / 0x38 (step 2). The leap's end without a catch
// falls to step 7. Step 2: 4 % a bite (SE 5, damage 0xf; step 3 waits for
// the task); the PC struggling (class 3) longer than 0x2f1020 frames
// throws it off (motion 0x3b / 0x3a, step 4). Steps 4..7: the fall
// (motions 0x2b..0x2d, down), 0x34, 0x30, then input 22.

int DwfOutAtkNear::execGrab()
{
    int no = g_gamMan.getActorNo(m_oid);
    switch (m_step) {
    case 0: {
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        u_int attr = m_achr->m_attr[0];
        if (m_move_done == 1 && !((attr & 8) && (attr & 0x10)) && !((attr & 2) && (attr & 4))) {
            playSE(0xb);
            changeMotion(0x48, 0);
            m_frame = 0;
            m_step  = 1;
            return -1;
        }
        return s_dwf_near_miss[m_kind];
    }
    case 1: {
        m_frame++;
        if (dwfMotionEnd(this)) {
            dwfGrabFlagsOff(this);
            changeMotion(0x30, 0);
            dwfHoldOff(this);
            m_step = 7;
            return -1;
        }
        if (m_frame - 0x11 > 6)
            return -1;
        if (emyChkInSight(s_dwf_grab_reach[m_kind], DWF_PI_5, 0, m_target, m_achr) == 0)
            return -1;
        if (!dwfGrabAllowed())
            return -1;
        int targetNo = g_gamMan.getActorNo(m_target);
        double angle = (double)getTargetAngle();
        if (angle < 0.7853981633974483) {
            m_back = 0;
        } else {
            if (angle <= 2.356194490192345)
                return -1;
            m_back = 1;
        }
        if (!dwfGrabCatch(this, no, targetNo, m_back, &m_node))
            return -1;
        changeMotion(s_dwf_grab_ride[m_back], 0x100);
        m_step = 2;
        return -1;
    }
    case 2:
        if (utlRandRange(0, 99) < 4) {
            playSE(5);
            scnChrRequest(0xb, m_oid, 0, 0, 0xf, m_target, 0);
            m_step = 3;
        }
        if (g_magSystem.getPCStateClass() == 3 && ++m_hold > s_dwf_near_hold[m_kind]) {
            m_hold = 0;
            dwfGrabRelease(this, no, &m_node, &m_back);
            m_step = 4;
        }
        return -1;
    case 3:
        if (g_taskMan.getTask(m_achr->m_task[3]) != 0)
            return -1;
        changeMotion(s_dwf_grab_ride[m_back], 0x100);
        m_step = 2;
        return -1;
    case 4:
        if (dwfMotionEnd(this)) {
            dwfGrabFlagsOff(this);
            changeMotion(0x2b + utlRandRange(0, 2), 0);
            m_emy->m_down_flg = 1;
            m_step = 5;
        }
        return -1;
    case 5:
        if (dwfMotionEnd(this)) {
            changeMotion(0x34, 0);
            m_step = 6;
        }
        return -1;
    case 6:
        dwfHoldOff(this);
        if (dwfMotionEnd(this)) {
            changeMotion(0x30, 0);
            m_emy->m_down_flg = 0;
            m_step = 7;
        }
        return -1;
    case 7:
        dwfHoldOff(this);
        if (dwfMotionEnd(this))
            return dwfGetUpEnd(this, s_dwf_near_end[m_kind]);
        return -1;
    }
    return -1;
}

// ──── DwfOutAtkFar ────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001606ec

void DwfOutAtkFar::Init()
{
    m_wait = utlRandRange(0x5a, 0x96);
    selectNearTarget(100);
    u_int held;
    m_achr->getParam(0x67, &held);
    u_int attr = m_achr->m_attr[0];
    if (!((attr & 8) && (attr & 0x10)) && held != 0) {
        m_step = 0;
        emyStartChase(150.0f, this, m_target);
    } else {
        m_step = 3;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00160784 // callcheck: isAchrMotionEnd, getParam, resetObj, setParam (chk_motion_end inlined: dwfMotionEnd; the item release: dwfReleaseThrown)
// Step 0: the chase ends: reached, the throw (startThrow), else input 23.
// Step 1: the throw task (the achr's m_task[3]) gone, the item released,
// decide back on: input 22. Step 2 (nothing sets it): the throw once the
// target is in sight within 150 / pi / 8, else at a motion's end 40 % of
// three times the base motion, then a new chase. Step 3: input 23.

int DwfOutAtkFar::Exec()
{
    int ret = -1;
    switch (m_step) {
    case 0:
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        if (m_move_done != 1)
            return 0x17;
        break;
    case 1:
        if (g_taskMan.getTask(m_achr->m_task[3]) != 0)
            return ret;
        dwfReleaseThrown(this);         /* the ROM turns decide on between resetObj and setParam */
        m_emy->setDecideSw(1);
        return 0x16;
    case 2:
        if (emyChkInSight(150.0f, DWF_PI_8, 1, m_target, m_achr) == 0) {
            if (!dwfMotionEnd(this))
                return ret;
            if (m_tries < 3 && utlRandRange(0, 99) < 0x28) {
                setBaseMotion();
                m_tries++;
                return ret;
            }
            m_step = 0;
            selectNearTarget(100);
            emyStartChase(150.0f, this, m_target);
            m_tries = 0;
            return ret;
        }
        break;
    case 3:
        return 0x17;
    default:
        return -1;
    }
    startThrow(&ret);
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016097c

void DwfOutAtkFar::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00160998 // callcheck: FUN_001f9d90 (scnAttackStart through dwfStartThrow)
// Decide stays off while the throw runs.

void DwfOutAtkFar::startThrow(int *ret)
{
    u_int held;
    m_achr->getParam(0x67, &held);
    if (held == 0) {
        *ret = 0x16;
        return;
    }
    m_step = 1;
    m_emy->setDecideSw(0);
    dwfStartThrow(this);
    playSE(0xb);
}

// ──── DwfOutStared ────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00160a44
// The voice's countdown m_30 restarts at 0 (the first tickVoice plays).
// A type other than 0..2 keeps m_kind and m_motion.

void DwfOutStared::Init()
{
    switch (m_emy->GetType()) {
    case 1:
        m_kind   = 1;
        m_motion = 0x4f;
        break;
    case 0:
        m_kind   = 0;
        m_motion = 0x56;
        break;
    case 2:
        m_kind   = 2;
        m_motion = 0x53;
        break;
    }
    emyStartTurn(DWF_PI_8, this, 1);
    m_step  = 0;
    m_count = 0;
    m_quiet = 0;
    int r   = utlRandRange(0, 2);
    m_30    = 0;
    m_limit = s_dwf_stare_limit[r];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00160b08 // callcheck: getParam, getObjKind (the scissors test inline)
// Step 0: the turn done, the stare motion: 0x26 with attr bits 8 and
// 0x10, the Got's scissors 0x50, else 0x25 (30 %) or the type's own.
// Step 1: voice; m_limit / 10 frames after the bark stops, or m_limit
// frames of barking, end it.

int DwfOutStared::Exec()
{
    if (m_step == 0) {
        if (m_turn_done == 0)
            return -1;
        m_emy->InactiveTurn();
        u_int attr = m_achr->m_attr[0];
        int motion = -1;
        if ((attr & 8) && (attr & 0x10)) {
            motion = 0x26;
        } else if (m_kind == 2) {
            u_int held;
            m_achr->getParam(0x67, &held);
            if (g_gamMan.getObjKind(held) == 0xa2e)
                motion = 0x50;
        }
        if (motion < 0)
            motion = utlRandRange(0, 99) > 0x1d ? m_motion : 0x25;
        changeMotion(motion, 0x100);
        m_step = 1;
        return -1;
    }
    if (m_step != 1)
        return -1;
    tickVoice(8, 0x18, 9);
    bool done;
    if (!magBwnIsBarkPhase3()) {
        if (m_quiet == 0) {
            m_quiet = 1;
            m_count = 0;
        }
        m_count++;
        done = (u_int)(m_limit / 10) < (u_int)m_count;
    } else {
        m_count++;
        done = m_limit < m_count;
    }
    if (!done)
        return -1;
    stopVoice();
    return s_dwf_stare_end[m_kind];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00160c9c

void DwfOutStared::Term()
{
    termCommon();
}

// ──── DwfOutWin ───────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00160cb8
// The target is kept unless m_target_dog is above 1 (never: 0 or 1). The
// scripted win leaves m_step as it was.

void DwfOutWin::Init()
{
    if (g_sequence.m_global[DWF_GLOBAL_WIN] == 0) {
        if ((u_int)m_target_dog > 1)
            selectTarget(100);
        m_range = utlRandRangeF(100.0f, 200.0f);
        emyStartChase(m_range, this, m_target);
        m_step = 0;
    } else {
        changeMotion(0x44, 0);
        selectNearTarget(100);
        m_count = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00160d50

int DwfOutWin::Exec()
{
    if (g_sequence.m_global[DWF_GLOBAL_WIN] == 0)
        execChase();
    else
        execScripted();
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00160d94
// Arrived: the gloat (0x15 with the looping SE 0xd; 10 with attr bits 8
// and 0x10).

void DwfOutWin::execChase()
{
    if (m_step != 0 || m_move_done == 0)
        return;
    m_emy->m_move->stop();
    u_int attr = m_achr->m_attr[0];
    if ((attr & 8) && (attr & 0x10)) {
        changeMotion(10, 0x100);
    } else {
        changeMotion(0x15, 0x100);
        m_se = playSE(0xd);
        g_sndMan.setSeLoop(m_se, 1);
    }
    m_step = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00160e44 // callcheck: isAchrMotionEnd, getTask, setObj, resetObj, getParam, setParam, setPos, FUN_001f9d90 (chk_motion_end: dwfMotionEnd; the hold: dwfWinHold; the release: dwfReleaseThrown; the throw: dwfStartThrow)
// Step 0: motion 0x44 over and over at the fixed spot (SE 2 on frame
// 17) until global 0x174 or a hit, then motion 8 (step 1). Step 1 at its
// end throws (SE 0xb, step 2); step 2 releases the item once the throw
// task ends, motion 0xd (step 4, held for ever).

void DwfOutWin::execScripted()
{
    switch (m_step) {
    case 0:
        if (g_sequence.m_global[DWF_GLOBAL_WIN_END] != 0 || m_emy->m_damage != 0) {
            changeMotion(8, 0);
            m_step = 1;
            return;
        }
        if (dwfMotionEnd(this)) {
            m_count = 0;
            changeMotion(0x44, 0);
            static_cast<ObjStat *>(m_achr)->setPos(s_dwf_win_pos);
            return;
        }
        if (++m_count == 0x11)
            playSE(2);
        break;
    case 1:
        dwfWinHold(this);
        m_emy->setDecideSw(0);
        if (dwfMotionEnd(this)) {
            dwfStartThrow(this);
            playSE(0xb);
            m_step = 2;
        }
        break;
    case 2:
        dwfWinHold(this);
        if (g_taskMan.getTask(m_achr->m_task[3]) == 0) {
            dwfReleaseThrown(this);
            m_step = 4;
            changeMotion(0xd, 0);
        }
        break;
    case 4:
        dwfWinHold(this);
        break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00161160

void DwfOutWin::Term()
{
    termCommon();
}

// ──── DwfOutParts ─────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016117c

void DwfOutParts::Init()
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
    changeMotion(motion, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001611e0

int DwfOutParts::Exec()
{
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001611e8

void DwfOutParts::Term()
{
    termCommon();
}

// ──── DwfOutTest ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00161204

void DwfOutTest::Init()
{
    m_step = 0;
    setMotion(6, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00161228

int DwfOutTest::Exec()
{
    return -1;
}

// ──── DwfOutFort ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00161230 // callcheck: fptodp, dpcmp (the double compare)
// The PC is the target; m_back from where the enemy stands (behind the PC
// from pi / 2). A type other than 0..2 keeps m_kind.

void DwfOutFort::Init()
{
    switch (m_emy->GetType()) {
    case 1:
        m_kind = 1;
        break;
    case 0:
        m_kind = 0;
        break;
    case 2:
        m_kind = 2;
        break;
    }
    m_hold = 0;
    m_target_dog = 0;
    m_target = dwfPCOid();
    m_back = (double)getTargetAngle() >= 1.5707963267948966;
    emyStartTurn(DWF_PI_8, this, 0);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00161300 // callcheck: isAchrMotionEnd, getTask, dbgAssertFailed, i3rGetNodeIndexByName, fptodp, dpcmp, setObj, resetObj, setDecideSw, setOid, clearOid, setPos, setRot, setActorParent, commandPC, scnChrRequest, hasOid, snapToTarget (chk_motion_end inlined: dwfMotionEnd; the grab: dwfGrabCatch / dwfGrabRelease / dwfGrabFlagsOff / dwfHoldOff; DogModel::getNodeIndexByName inline)
// Input 23 at once with attr bits 8 and 0x10 or 2 and 4. Step 0: the turn
// to the PC, the leap when it comes near (startLeap: step 1), else on the
// walk motion (step 0xb) until the PC leaves pi / 6 of its facing, then
// turn again. Step 1: frames 17..23 of the leap catch the PC within
// 0x2f1148 (pi / 6, all round) from the front (angle under pi / 2) or
// behind; ride motion 0x39 / 0x38 (step 3); the leap's end without a
// catch falls to step 10. Step 3: 4 % a bite (SE 5, damage 0xf; step 4
// waits for the task); the PC struggling (class 3) longer than 0x2f1160
// frames throws it off (step 5). Steps 5, 6, 7, 10: the fall, 0x34,
// 0x30, input 22. Steps 2, 8 and 9 are empty.

int DwfOutFort::Exec()
{
    int no = g_gamMan.getActorNo(m_oid);
    u_int attr = m_achr->m_attr[0];
    if ((attr & 8) && (attr & 0x10))
        return s_dwf_fort_quit[m_kind];
    if ((attr & 2) && (attr & 4))
        return s_dwf_fort_quit[m_kind];

    switch (m_step) {
    case 0:
        if (startLeap()) {
            m_step = 1;
        } else if (m_turn_done != 0) {
            m_emy->InactiveTurn();
            resetWalkMotion();
            m_step = 0xb;
        }
        break;
    case 1: {
        m_frame++;
        if (dwfMotionEnd(this)) {
            dwfGrabFlagsOff(this);
            changeMotion(0x30, 0);
            m_step = 10;
            break;
        }
        if (m_frame - 0x11 > 6)
            return -1;
        if (emyChkInSight(s_dwf_fort_reach[m_kind], DWF_PI_6, 0, m_target, m_achr) == 0)
            break;
        if (!dwfGrabAllowed())
            return -1;
        int targetNo = g_gamMan.getActorNo(m_target);
        m_back = (double)getTargetAngle() >= 1.5707963267948966;
        if (!dwfGrabCatch(this, no, targetNo, m_back, &m_node))
            return -1;
        changeMotion(s_dwf_fort_ride[m_back], 0x100);
        m_step = 3;
        break;
    }
    case 2:
    case 8:
    case 9:
        break;
    case 3:
        if (utlRandRange(0, 99) < 4) {
            playSE(5);
            scnChrRequest(0xb, m_oid, 0, 0, 0xf, m_target, 0);
            m_step = 4;
        }
        if (g_magSystem.getPCStateClass() != 3)
            return -1;
        if (++m_hold > s_dwf_fort_hold[m_kind]) {
            m_hold = 0;
            dwfGrabRelease(this, no, &m_node, &m_back);
            m_step = 5;
        }
        break;
    case 4:
        if (g_taskMan.getTask(m_achr->m_task[3]) != 0)
            return -1;
        changeMotion(s_dwf_fort_ride[m_back], 0x100);
        m_step = 3;
        break;
    case 5:
        if (dwfMotionEnd(this)) {
            dwfGrabFlagsOff(this);
            changeMotion(0x2b + utlRandRange(0, 2), 0);
            m_emy->m_down_flg = 1;
            m_step = 6;
        }
        break;
    case 6:
        if (dwfMotionEnd(this)) {
            changeMotion(0x34, 0);
            m_step = 7;
        }
        break;
    case 7:
        dwfHoldOff(this);
        if (dwfMotionEnd(this)) {
            changeMotion(0x30, 0);
            m_emy->m_down_flg = 0;
            m_step = 10;
        }
        break;
    case 10:
        dwfHoldOff(this);
        if (dwfMotionEnd(this))
            return dwfGetUpEnd(this, s_dwf_fort_end[m_kind]);
        break;
    case 0xb:
        if (startLeap()) {
            m_step = 1;
            break;
        }
        if (emyChkInAngle(DWF_PI_6, this, m_target) != 0)
            return -1;
        emyStartTurn(DWF_PI_8, this, 0);
        m_step = 0;
        break;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00161a90

void DwfOutFort::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00161aac
// The turn stops with the leap.

bool DwfOutFort::startLeap()
{
    int seen = emyChkInSight(s_dwf_fort_reach[m_kind] + 20.0f, DWF_PI_8, 1, m_target, m_achr);
    if (seen != 0) {
        playSE(0xb);
        changeMotion(0x48, 0);
        m_frame = 0;
        dropItem();
        m_emy->InactiveTurn();
    }
    return seen != 0;
}

// ──── DwfOutIrritate ──────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00161b48
// A type other than 0..2 keeps m_kind.

void DwfOutIrritate::Init()
{
    switch (m_emy->GetType()) {
    case 1:
        m_kind = 1;
        break;
    case 0:
        m_kind = 0;
        break;
    case 2:
        m_kind = 2;
        break;
    }
    emyStartTurn(DWF_PI_10, this, 0);
    m_step   = 0;
    m_motion = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00161bd4 // callcheck: isAchrMotionEnd (chk_motion_end inlined: dwfMotionEnd)
// Steps: 0 the turn to the PC; 1 closing in until it sees m_target
// within the reach + 50 (pi / 4); 3 an idle or sidestep; each ends in
// pickMove, whose 0 (the PC sees it) ends the out. Step 2 repeats
// m_motion m_count times (unreachable: see pickMove).

int DwfOutIrritate::Exec()
{
    u_int attr = m_achr->m_attr[0];
    if ((attr & 8) || (attr & 0x10))
        return s_dwf_irritate_end[m_kind];
    switch (m_step) {
    case 0:
        if (m_turn_done == 0)
            return -1;
        m_emy->InactiveTurn();
        break;
    case 1: {
        int r = emyChkChase(getReach() + 50.0f, DWF_PI_4, this, m_target, 1);
        if (r == 0)
            return -1;
        m_emy->InactiveTurn();
        if (r != 1)
            return s_dwf_irritate_end[m_kind];
        break;
    }
    case 2:
        if (!dwfMotionEnd(this))
            return -1;
        if (--m_count != 0) {
            changeMotion(m_motion, 0);
            return -1;
        }
        emyStartTurn(DWF_PI_10, this, 0);
        m_step = 0;
        return -1;
    case 3:
        if (!dwfMotionEnd(this))
            return -1;
        emyStartTurn(DWF_PI_10, this, 0);
        m_step = 0;
        return -1;
    default:
        return -1;
    }
    if (pickMove() != 0)
        return -1;
    return s_dwf_irritate_end[m_kind];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00161d8c

void DwfOutIrritate::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00161da8
// By the PC's equipped item (0xa00 none): 0xa3b and 0xb59 170; 0xa39,
// 0xa3a, 0xa3c 130; 0xa36..0xa38, 0xa3d 110; else 50.

float DwfOutIrritate::getReach()
{
    float reach;
    switch (g_gamMan.m_equipItem) {
    case 0xa3b:
    case 0xb59:
        reach = 170.0f;
        break;
    case 0xa39:
    case 0xa3a:
    case 0xa3c:
        reach = 130.0f;
        break;
    case 0xa36:
    case 0xa37:
    case 0xa38:
    case 0xa3d:
        reach = 110.0f;
        break;
    default:
        reach = 50.0f;
        break;
    }
    return reach + 10.0f;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00161e74

void dwfOffsetByRot(float front, float side, EmyOut *out, float *pos)
{
    int no = g_gamMan.getActorNo(out->m_oid);
    float rot = g_magSystem.m_scene.m_actors[no].getRotY();
    float s = sinf(rot);
    float c = cosf(rot);
    pos[0] = pos[0] + front * s + side * c;
    pos[2] = pos[2] + (front * c - side * s);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00161f28
// The PC sees it within the reach (all round): 0, m_count 0. The ROM then
// looks for a nonzero m_count three times -- it was just cleared, so the
// motion table 0x2f11b8 (6, 0x10, 0x11) and step 2 are never used.
// Farther than the reach + 200: a chase to the PC (step 1). Else by
// chance: 10 % the last motion (2 if none), 20 % motion 2, 25 % 0x10
// again or 0x11, 25 % 0x11 again or 0x10, 20 % 6 after any motion (2
// the first time); SE 0xd unless 2 (step 3).

int DwfOutIrritate::pickMove()
{
    float base[4], pc[4];
    getBasePos(base);
    emyGetPCPos(pc);
    float reach = getReach();
    if (emyChkInSight(reach, DWF_PI, 0, dwfPCOid(), m_achr) != 0) {
        m_count = 0;
        return 0;
    }
    if (emyChkNearXZ(reach + 200.0f, base, pc) == 0) {
        emyStartChase(10.0f, this, dwfPCOid());
        m_step = 1;
        return 1;
    }
    int r = utlRandRange(0, 99);
    int motion;
    if (r < 10)
        motion = m_motion != 0 ? m_motion : 2;
    else if (r < 30)
        motion = 2;
    else if (r < 55)
        motion = m_motion == 0x10 ? 0x10 : 0x11;
    else if (r < 80)
        motion = m_motion == 0x11 ? 0x11 : 0x10;
    else
        motion = m_motion != 0 ? 6 : 2;
    m_motion = motion;
    changeMotion(motion, 0);
    if (motion != 2)
        playSE(0xd);
    m_step = 3;
    return 1;
}

// ──── DwfOutDead ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00162118 // callcheck: utlRandPeakLo (the revival draw), setParam
// No revival (step 3) with no revivals left, in a group, with attr bit 1
// and bits 2 + 4 or 8 + 0x10, or with bits 2 + 4 and 8 + 0x10. Otherwise
// it spends some revivals and waits (step 0). A type other than 0..2 keeps
// m_kind.

void DwfOutDead::Init()
{
    switch (m_emy->GetType()) {
    case 1:
        m_kind = 1;
        break;
    case 0:
        m_kind = 0;
        break;
    case 2:
        m_kind = 2;
        break;
    }
    float pos[4];
    m_achr->getPos(pos);
    float floor = g_gamMan.getFloorY(pos);
    EffParam eff = {};          // PORT: the ROM leaves the words it does not set as they were
    eff.m_kind = 3;
    eff.m_08   = 0;
    eff.m_hit.m_pos[0] = pos[0];
    eff.m_hit.m_pos[1] = floor;
    eff.m_hit.m_pos[2] = pos[2];
    eff.m_hit.m_1c    = 0;
    eff.m_hit.m_20    = -1.0f;
    eff.m_hit.m_24    = 0;
    eff.m_hit.m_scale = 100.0f;
    eff.m_hit.m_2c    = 0x5a;
    g_magSystem.m_scene.m_eff_man.createEff(&eff);
    dropItem();
    m_emy->m_visible = 0;
    g_gamMan.resetObj(m_oid, 0x40);
    g_gamMan.setObj(m_oid, 0x10000000);
    g_gamMan.setObj(m_oid, 0x400);
    m_wait = 0;
    m_emy->stopAllSE();
    m_achr->m_flags |= 0x400;
    m_achr->m_flags |= 0x2000;
    changeMotion(m_emy->m_down_supine != 0 ? 0x12 : 0x13, 0x100);

    short left;
    m_achr->getParam(0x5e, &left);
    if (left == 0 || m_emy->m_grp_no != 0) {
        m_step = 3;
        return;
    }
    u_int attr = m_achr->m_attr[0];
    bool down = (attr & 8) && (attr & 0x10);
    if ((attr & 2) && (attr & 4)) {
        if ((attr & 1) || down) {
            m_step = 3;
            return;
        }
    } else if (down && (attr & 1)) {
        m_step = 3;
        return;
    }
    m_step = 0;
    left -= (short)utlRandPeakLo(1, left, 4);
    m_achr->setParam(0x5e, &left);
    m_wait = utlRandPeakLo(0xfa, 0x1d4c, 4);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00162394 // callcheck: isAchrMotionEnd (chk_motion_end inlined: dwfMotionEnd)
// Step 0: the wait's end, the life back (EmyMain's m_lp). Lying on its
// back it rolls over first (0x34, step 1); then with attr bits 8 and 0x10
// it is up at once (motion 10, input 23), else it stands (0x30, step 2,
// input 23 at its end). Step 3: the death for good, counted once (step 4
// idles).

int DwfOutDead::Exec()
{
    switch (m_step) {
    case 0: {
        if (--m_wait != 0)
            return -1;
        m_achr->m_flags &= ~0x2000u;
        int lp = m_emy->m_lp;
        m_achr->setParam(GAM_PRM_A_LP, &lp);
        if (m_emy->m_down_supine != 0) {
            m_step = 1;
            changeMotion(0x34, 0);
            return -1;
        }
        u_int attr = m_achr->m_attr[0];
        if (!((attr & 8) && (attr & 0x10)))
            break;
        m_emy->m_visible = 1;
        goto up;
    }
    case 1: {
        if (!dwfMotionEnd(this))
            return -1;
        u_int attr = m_achr->m_attr[0];
        if (!((attr & 8) && (attr & 0x10)))
            break;
        m_emy->m_visible = 1;
        goto up;
    }
    case 2:
        if (dwfMotionEnd(this)) {
            m_emy->m_visible = 1;
            m_achr->m_flags &= ~0x400u;
            g_gamMan.setObj(m_oid, 0x40);
            g_gamMan.resetObj(m_oid, 0x10000000);
            g_gamMan.resetObj(m_oid, 0x400);
            return s_dwf_dead_end[m_kind];
        }
        return -1;
    case 3:
        leaveGroup();
        g_sequence.m_global[m_emy->m_name + 0x33]++;
        m_step = 4;
        return -1;
    default:
        return -1;
    }
    m_step = 2;
    changeMotion(0x30, 0);
    return -1;

up:
    m_achr->m_flags &= ~0x400u;
    g_gamMan.setObj(m_oid, 0x40);
    g_gamMan.resetObj(m_oid, 0x10000000);
    g_gamMan.resetObj(m_oid, 0x400);
    changeMotion(10, 0);
    return s_dwf_dead_end[m_kind];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00162630

void DwfOutDead::Term()
{
    m_achr->m_flags &= ~0x400u;
    termCommon();
}
