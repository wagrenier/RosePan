/* ==========================================================================
 *  emy/emy_fsh.cpp
 *
 *  FshMain (enemy type 3) and its outs (0x155d54..0x1583d4). The usual
 *  enemy loop: decide picks an input of the transition table (only inputs
 *  the current state's row has), exec moves the state graph by it, after
 *  switches outs on a change and runs the current one. A hit (notify bit
 *  1) restarts the current state; a kill (bit 2) forces state 4. Unlike
 *  the other enemies its decide waits for the motion input word, so it
 *  re-decides only when a motion has ended.
 * ======================================================================== */
#include "emy/emy_fsh.h"

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
#include "snd/sndman.h"
#include "task/tsk_enemy.h"
#include "task/tsk_scene.h"
#include "utl/utlrand.h"

static const float FSH_PI   = 0x1.921fb4p+1f;   /* 0x40490fda */
static const float FSH_PI_4 = 0x1.921fb4p-1f;   /* 0x3f490fda */
static const float FSH_PI_8 = 0x1.921fb4p-2f;   /* 0x3ec90fda */

/* FshOutAtk's motions by pose: idle (0x2f0bf8), attack (0x2f0c08) and
 * the rest after an attack or a rise / lowering (0x2f0c18). */
static const int s_fsh_atk_idle[4]   = { 0x2a, 0x28, 0x2c, 0x2d };  // ROR 0x002f0bf8
static const int s_fsh_atk_attack[4] = { 0x2b, 0x29, 0x29, 0x29 };  // ROR 0x002f0c08
static const int s_fsh_atk_rest[4]   = { 0x1e, 0x21, 0x21, 0x21 };  // ROR 0x002f0c18

/* FshOutStared's frames after the bark stops (0x2f0c78): all 0 in the
 * ROM, so the stare ends the frame after. */
static const u_short s_fsh_stare_limit[3] = { 0, 0, 0 };            // ROR 0x002f0c78

static u_int fshPCOid()
{
    ObjStat *pc = g_gamMan.m_pcPtr;
    return pc != NULL ? pc->m_oid : 0;
}

static u_int fshDogOid()
{
    ObjStat *dog = g_gamMan.m_dogPtr;
    return dog != NULL ? dog->m_oid : 0;
}

/* chk_motion_end inline: the achr's motion ended sets the emy's motion
 * input. */
static bool fshMotionEnd(EmyOut *out)
{
    if (!g_gamMan.isAchrMotionEnd(out->m_oid))
        return false;
    out->m_emy->m_input_flg_motion = 1;
    return true;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00155d54 // callcheck: ~EmyMain, operator_delete (the base destructor and the deleting variant are the compiler's)

FshMain::~FshMain()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00155db4 // callcheck: isPCStateClass4, testAttr8Or10, testAttr2Or4, testAttr1, testAttr2And4, testAttr8And10, isHalfLife, isLowLife, isTargetInSight, isPCNear, isPCClass7OffAxis, isPCStateClass2, isPCAhead, isPCStateClass1, isPCTouching, testBarkReact
// Only with decide on and the motion input word set. Every test runs and
// the last true one wins: 18 down to 11, the sight edge (4 into sight, 5
// out of it; while it stays in sight 10 down to 6), then 3, 2, 1.

void FshMain::decide()
{
    if (m_decide_sw == 0)
        return;
    if (m_input_flg_motion == 0)
        return;
    if (m_req != -1)
        return;

    if (m_input[18] && isPCStateClass4())
        m_req = 0x12;
    if (m_input[17] && testAttr8Or10())
        m_req = 0x11;
    if (m_input[16] && testAttr2Or4())
        m_req = 0x10;
    if (m_input[15] && testAttr1())
        m_req = 0xf;
    if (m_input[14] && testAttr2And4())
        m_req = 0xe;
    if (m_input[13] && testAttr8And10())
        m_req = 0xd;
    if (m_input[12] && isHalfLife())
        m_req = 0xc;
    if (m_input[11] && isLowLife())
        m_req = 0xb;

    u_char seen = (u_char)isTargetInSight();
    if (m_seen == 0) {
        m_seen = seen;
        if (seen != 0 && m_input[4])
            m_req = 4;
    } else {
        m_seen = seen;
        if (seen == 0) {
            if (m_input[5])
                m_req = 5;
        } else {
            if (m_input[10] && isPCNear())
                m_req = 10;
            if (m_input[9] && isPCClass7OffAxis())
                m_req = 9;
            if (m_input[8] && isPCStateClass2())
                m_req = 8;
            if (m_input[7] && isPCAhead())
                m_req = 7;
            if (m_input[6] && isPCStateClass1())
                m_req = 6;
        }
    }

    if (m_input[3] && isPCTouching())
        m_req = 3;
    if (m_input[2] && (m_notify & 1))
        m_req = 2;
    if (m_input[1] && testBarkReact())
        m_req = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00156048
// Notify bit 2 (killed) forces state 4; bit 1 (hit) with no input restarts
// the current state; an input the graph has no move for keeps it. A move
// to state 2 (Appear) while visible goes on by input 0x14.

void FshMain::exec()
{
    m_changed = 0;
    if (m_notify & 2) {
        m_state      = 4;
        m_map->m_cur = 4;
        m_changed    = 1;
    } else if (m_req == -1) {
        if (m_notify & 1) {
            m_map->m_cur = m_state;
            m_changed    = 1;
        }
    } else {
        m_notify = 0;
        int state = m_map->next(m_req);
        if (state == 2 && m_visible != 0)
            state = m_map->next(0x14);
        m_258  = 0;
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
// ROR 0x00156140

void FshMain::after()
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
// ROR 0x001561bc
// The ROM returns 0 (the slot's value is never read).

void FshMain::selectOut()
{
    if (m_state > 0 && m_state <= 12) {
        m_emy_out = m_out[m_state];
        m_emy_out->Init();
    }
    int n = m_trans->m_inputs;
    const char *row = m_trans->m_table + m_state * n;
    // PORT: the ROM writes as many words as the table has inputs; the
    // host array holds 21 (EE 0x204..0x258).
    if (n > 21)
        n = 21;
    for (int i = 0; i < n; i++)
        m_input[i] = row[i] != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00156284

bool FshMain::isPCSameArea()
{
    return EmyMain::isPCSameArea();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00156298

int FshMain::testBarkReact()
{
    return EmyMain::testBarkReact();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001562ac // callcheck: isPCSameArea (FshMain's wrapper)
// Both tests run (no short cut in the ROM).

bool FshMain::isTargetInSight()
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
    int pc  = emyChkInSight(range, FSH_PI, useAngle, fshPCOid(), m_param);
    int dog = emyChkInSight(range, FSH_PI, useAngle, fshDogOid(), m_param);
    return pc != 0 || dog != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015640c

int FshMain::isPCTouching()
{
    float base[4], pc[4];
    getBasePos(base);
    emyGetPCPos(pc);
    float r = g_gamMan.getObjRadius(m_oid, NULL);
    r += g_gamMan.getObjRadius(fshPCOid(), NULL);
    return emyChkNearXZ(r + 10.0f, base, pc);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001564c0

int FshMain::isPCStateClass1()
{
    return EmyMain::isPCStateClass1();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001564d4

int FshMain::isPCAhead()
{
    return EmyMain::isPCAhead();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001564e8

int FshMain::isPCStateClass2()
{
    return EmyMain::isPCStateClass2();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001564fc

int FshMain::isPCClass7OffAxis()
{
    return EmyMain::isPCClass7OffAxis();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00156510

int FshMain::isPCNear()
{
    return EmyMain::isPCNear();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00156524

bool FshMain::isLowLife()
{
    return EmyMain::isLowLife();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00156538

bool FshMain::isHalfLife()
{
    return EmyMain::isHalfLife();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015654c

int FshMain::testAttr8And10()
{
    u_int attr = m_param->m_attr[0];
    return (attr & 8) != 0 && (attr & 0x10) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00156578

int FshMain::testAttr2And4()
{
    u_int attr = m_param->m_attr[0];
    return (attr & 2) != 0 && (attr & 4) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001565a4

int FshMain::testAttr1()
{
    return m_param->m_attr[0] & 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001565b4

int FshMain::testAttr2Or4()
{
    u_int attr = m_param->m_attr[0];
    return (attr & 2) != 0 || (attr & 4) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001565e0

int FshMain::testAttr8Or10()
{
    u_int attr = m_param->m_attr[0];
    return (attr & 8) != 0 || (attr & 0x10) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015660c

int FshMain::isPCStateClass4()
{
    return EmyMain::isPCStateClass4();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00156620 // callcheck: operator_new, EmyOut (new FshOutXxx: the inline constructors call EmyOut::EmyOut)

void FshMain::initOut()
{
    m_motion_list[8]  = 0x28;
    m_motion_list[2]  = 3;
    m_motion_list[11] = 8;
    m_motion_list[12] = 9;
    m_motion_list[9]  = 0x18;
    m_state_flg[5]    = 1;
    m_motion_list[0]  = 1;
    m_motion_list[1]  = 1;
    m_motion_list[3]  = 1;
    m_motion_list[4]  = 1;
    m_motion_list[5]  = 1;
    m_motion_list[6]  = 1;
    m_motion_list[7]  = 1;
    m_motion_list[10] = 1;
    m_state_flg[0]    = 1;
    m_state_flg[2]    = 1;
    m_state_flg[8]    = 1;
    m_seen            = 0;
    m_258             = 0;
    m_state_flg[1]    = 0;
    m_state_flg[10]   = 0;
    m_state_flg[11]   = 0;
    m_state_flg[4]    = 0;
    m_state_flg[7]    = 0;
    m_state_flg[12]   = 0;

    // PORT: the ROM builds only the first six words on the stack.
    GamObjColli colli;
    memset(&colli, 0, sizeof(colli));
    colli.m_kind   = 2;
    colli.m_shape  = 4;
    colli.m_08     = 0.0f;
    colli.m_0c     = 0.0f;
    colli.m_radius = 10.0f;
    colli._14      = 0;
    m_param->setParam(GAM_PRM_COL_PRM, &colli);
    m_parsonality = (u_char)utlRandRange(0, 2);

    m_out[1]  = new FshOutDisappear(m_oid, m_param, this);
    m_out[2]  = new FshOutAppear(m_oid, m_param, this);
    m_out[3]  = new FshOutAtk(m_oid, m_param, this);
    m_out[4]  = new FshOutDead(m_oid, m_param, this);
    m_out[5]  = new FshOutStared(m_oid, m_param, this);
    m_out[6]  = new FshOutCollision(m_oid, m_param, this);
    m_out[7]  = new FshOutWin(m_oid, m_param, this);
    m_out[8]  = new FshOutSerch(m_oid, m_param, this);
    m_out[9]  = new FshOutRound(m_oid, m_param, this);
    m_out[10] = new FshOutWalk(m_oid, m_param, this);
    m_out[11] = new FshOutWait(m_oid, m_param, this);
    m_out[12] = new FshOutTest(m_oid, m_param, this);
}

// ──── FshOutDisappear ─────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015694c

void FshOutDisappear::Init()
{
    g_gamMan.hideObj(m_oid, 1);
    setMotion(0x16, 0x100);
    m_emy->m_visible = 0;
    g_gamMan.resetObj(m_oid, 0x40);
    g_gamMan.setObj(m_oid, 0x10000000);
    g_gamMan.setObj(m_oid, 0x400);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001569d8

void FshOutDisappear::Term()
{
    g_gamMan.hideObj(m_oid, 0);
    m_emy->m_visible = 1;
    g_gamMan.setObj(m_oid, 0x40);
    g_gamMan.resetObj(m_oid, 0x10000000);
    g_gamMan.resetObj(m_oid, 0x400);
    termCommon();
}

// ──── FshOutAppear ────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00156a5c

void FshOutAppear::Init()
{
    m_emy->m_visible = 0;
    m_achr->m_flags |= 0x400;
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00156a7c // callcheck: isAchrMotionEnd (chk_motion_end inlined: fshMotionEnd)

int FshOutAppear::Exec()
{
    if (m_step == 0) {
        g_gamMan.hideObj(m_oid, 0);
        setMotion(0x17, 0);
        m_step = 1;
    } else if (m_step == 1 && fshMotionEnd(this)) {
        m_emy->m_visible = 1;
        m_achr->m_flags &= ~0x400u;
        return 0x13;
    }
    return -1;
}

// ──── FshOutAtk ───────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00156b48
// Starts raised when the last motion was one of a raised pose.

void FshOutAtk::Init()
{
    m_wait  = utlRandRange(0x32, 0x50);
    m_count = 0;
    switch (m_achr->m_13c) {
    case 0x1c:
    case 0x1e:
    case 0x20:
    case 0x23:
    case 0x2a:
    case 0x2b:
        m_pose   = 1;
        m_raised = 1;
        break;
    default:
        m_raised = 0;
        m_pose   = 0;
        break;
    }
    m_step = 0;
    setMotion(s_fsh_atk_idle[m_pose], 0x100);
    m_hit_pc  = 0;
    m_hit_dog = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00156c00 // callcheck: getSeState, playSE, isAchrMotionEnd (the rise / lowering start is inlined three times; chk_motion_end inlined: fshMotionEnd)
// Steps: 0 idle (hits) for m_wait, then attack at 30 % (step 1) or a new
// pose and wait (rest motion, step 2); 1 the attack (hits) to its end,
// then a new pose (step 2); 2 the rest for m_wait (no hits), then attack
// at 30 % or idle again (step 0); 3 the rise / lowering to its end, then
// the stored motion, SE and step. Poses other than 0 are raised: a pose
// change across 0 plays 0x23 (rise) or 0x22 (lower) first.

int FshOutAtk::Exec()
{
    switch (m_step) {
    case 0:
    {
        if (++m_count < m_wait) {
            checkHit();
            return -1;
        }
        m_count = 0;
        if (utlRandRange(0, 99) < 0x1e) {
            setMotion(s_fsh_atk_attack[m_pose], 0);
            m_se   = playSE(0xb);
            m_step = 1;
            return -1;
        }
        if (g_sndMan.getSeState(m_se) == -1)
            m_se = playSE(0xc);
        m_pose = (u_short)utlRandRange(0, 3);
        m_wait = utlRandRange(0x1e, 0x96);
        if (m_raised == 0) {
            if (m_pose != 0) {
                setMotion(0x23, 0);
                m_raised = 1;
                goto change0;
            }
        } else if (m_pose == 0) {
            setMotion(0x22, 0);
            m_raised = 0;
            goto change0;
        }
        setMotion(s_fsh_atk_rest[m_pose], 0x100);
        m_step = 2;
        return -1;
    change0:
        m_next_flags  = 0x100;
        m_next_step   = 2;
        m_step        = 3;
        m_next_motion = s_fsh_atk_rest[m_pose];
        m_next_se     = 0xc;
        return -1;
    }
    case 1:
        if (!fshMotionEnd(this)) {
            checkHit();
            return -1;
        }
        m_count = 0;
        m_pose  = (u_short)utlRandRange(0, 3);
        m_wait  = utlRandRange(0x1e, 0x3c);
        if (m_raised == 0) {
            if (m_pose != 0) {
                setMotion(0x23, 0);
                m_raised = 1;
                goto change1;
            }
        } else if (m_pose == 0) {
            setMotion(0x22, 0);
            m_raised = 0;
            goto change1;
        }
        setMotion(s_fsh_atk_rest[m_pose], 0x100);
        m_step = 2;
        return -1;
    change1:
        m_step = 3;
        if (g_sndMan.getSeState(m_se) == -1)
            m_se = playSE(0xc);
        m_next_flags  = 0x100;
        m_next_motion = s_fsh_atk_rest[m_pose];
        m_next_step   = 2;
        m_next_se     = 0xc;
        return -1;
    case 2:
    {
        if (++m_count < m_wait)
            return -1;
        m_pose = (u_short)utlRandRange(0, 3);
        int motion;
        int attack = utlRandRange(0, 99) <= 0x1d;
        if (!attack) {
            m_next_flags = 0x100;
            m_count      = 0;
            motion       = s_fsh_atk_idle[m_pose];
        } else {
            m_next_flags = 0;
            motion       = s_fsh_atk_attack[m_pose];
        }
        m_wait = utlRandRange(0x32, 0x50);
        if (m_raised == 0) {
            if (m_pose != 0) {
                setMotion(0x23, 0);
                m_raised = 1;
                goto change2;
            }
        } else if (m_pose == 0) {
            setMotion(0x22, 0);
            m_raised = 0;
            goto change2;
        }
        if (attack) {
            setMotion(motion, 0);
            m_step = 1;
        } else {
            setMotion(motion, 0x100);
            m_step = 0;
        }
        m_se = playSE(0xb);
        return -1;
    change2:
        m_step = 3;
        if (g_sndMan.getSeState(m_se) == -1)
            m_se = playSE(0xc);
        m_next_step   = attack;
        m_next_se     = 0xb;
        m_next_motion = motion;
        return -1;
    }
    case 3:
        if (!fshMotionEnd(this))
            return -1;
        setMotion(m_next_motion, m_next_flags);
        if (g_sndMan.getSeState(m_se) == -1)
            m_se = playSE(m_next_se);
        m_step = m_next_step;
        return -1;
    default:
        return -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001570e0

void FshOutAtk::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001570fc

void FshOutAtk::checkHit()
{
    if (g_magSystem.getPCStateClass() != 9 && m_hit_pc < 2) {
        u_int pc = fshPCOid();
        if (isTouching(0, pc)) {
            m_hit_pc++;
            m_se = playSE(0);
            scnChrRequest(0xb, m_oid, 0, 0, 0x17, pc, 0);
        }
    }
    if (!magBwnIsHurt() && m_hit_dog < 2) {
        u_int dog = fshDogOid();
        if (isTouching(1, dog)) {
            m_hit_dog++;
            m_se = playSE(0);
            scnChrRequest(0xb, m_oid, 0, 0, 0x17, dog, 0);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157230 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogActor::getNodeIndexByName inline)
// The dog pass is a leftover: magBwnNop does nothing, so it tests the
// same points against the spine matrix's first row with its w (0 for a
// rotation) + 10 as the range.

bool FshOutAtk::isTouching(int dog, u_int target)
{
    DogActor &actor = g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(target)];
    int node = actor.getNodeIndexByName("Bip01 Spine1");
    sceVu0FMATRIX m;
    actor.getNodeWorldMatrix(m, node);
    float spine[3] = { m[3][0], m[3][1], m[3][2] };

    float rot = g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(m_oid)].getRotY();
    float base[4];
    getBasePos(base);

    int hit = 0;
    float p[3];
    for (int i = 0; i < 4; i++) {
        float d = (float)i * 20.0f + -30.0f;
        p[1] = base[1] + 0.0f;
        p[0] = base[0] + sinf(rot) * d;
        p[2] = base[2] + cosf(rot) * d;
        hit |= emyChkNearXZ(g_gamMan.getObjRadius(target, NULL) + 10.0f, p, spine);
    }
    int hit_dog = 0;
    if (dog == 1) {
        magBwnNop();
        for (int i = 0; i < 4; i++) {
            float d = (float)i * 20.0f + -30.0f;
            p[1] = base[1] + 0.0f;
            p[0] = base[0] + sinf(rot) * d;
            p[2] = base[2] + cosf(rot) * d;
            hit_dog |= emyChkNearXZ(m[0][3] + 10.0f, p, m[0]);
        }
    }
    return hit != 0 || hit_dog != 0;
}

// ──── FshOutWait ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001574e4

void FshOutWait::Init()
{
    setWalkMotion();
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015750c

int FshOutWait::Exec()
{
    return -1;
}

// ──── FshOutWalk ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157514

void FshOutWalk::Init()
{
    setWalkMotion();
    selectTarget(100);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157548 // callcheck: isAchrMotionEnd (chk_motion_end inlined: fshMotionEnd)
// Steps: 0 a random point within 500 of its spot (on the spot's floor):
// reachable from the spot and over 100 away in both x and z, move there
// (1), else wait (2); 1 until it arrives; 2 20 frames, then half the time
// a new point, else 10 more frames; 3 (never set) until the motion ends.

int FshOutWalk::Exec()
{
    int count = m_count++;
    switch (m_step) {
    case 0:
    {
        float home[4], to[4], base[4];
        m_achr->getParam(0x59, home);
        to[0] = home[0] + (float)utlRandRange(-500, 500);
        to[1] = home[1];
        to[2] = home[2] + (float)utlRandRange(-500, 500);
        to[1] = g_gamMan.getFloorY(home);
        bool move = false;
        if (emyCanReach(home, to)) {
            getBasePos(base);
            float dx = base[0] - to[0];
            if (dx < 0.0f ? dx < -100.0f : 100.0f < dx) {
                float dz = base[2] - to[2];
                move = dz < 0.0f ? dz < -100.0f : 100.0f < dz;
            }
        }
        if (move) {
            emyStartMove(0.0f, this, to);
            setStep(1);
        } else {
            setWalkMotion();
            setStep(2);
        }
        return -1;
    }
    case 1:
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        setStep(0);
        setWalkMotion();
        return -1;
    case 2:
        if (count + 1 < 0x14)
            return -1;
        if (utlRandRange(0, 99) < 0x32)
            setStep(0);
        else
            m_count = 10;
        return -1;
    case 3:
        if (!fshMotionEnd(this))
            return -1;
        setStep(0);
        setWalkMotion();
        return -1;
    default:
        return -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001577dc

void FshOutWalk::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001577f8

void FshOutWalk::setStep(int step)
{
    if (m_step != step) {
        m_step  = step;
        m_count = 0;
    }
}

// ──── FshOutSerch ─────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157814

void FshOutSerch::Init()
{
    selectTarget(100);
    emyStartChase(10.0f, this, m_target);
    m_emy->setSE(1, 2, 0x5a);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157864

int FshOutSerch::Exec()
{
    int r = emyChkChase(10.0f, FSH_PI, this, m_target, 0);
    if (r == 1)
        return 0x13;
    if (r == 0)
        return -1;
    return 0x14;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001578c4

void FshOutSerch::Term()
{
    termCommon();
}

// ──── FshOutRound ─────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001578e0

void FshOutRound::Init()
{
    m_emy->setSE(1, 2, 0x5a);
    selectTarget(100);
    emyStartChase(10.0f, this, m_target);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157934

int FshOutRound::Exec()
{
    int r = emyChkChase(30.0f, FSH_PI, this, m_target, 0);
    if (r == 0)
        return -1;
    m_emy->m_move->stop();
    return r == 1 ? 0x13 : 0x14;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001579b4

void FshOutRound::Term()
{
    termCommon();
}

// ──── FshOutStared ────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001579d0

void FshOutStared::Init()
{
    emyStartTurn(FSH_PI_8, this, 1);
    m_step  = 0;
    m_count = 0;
    m_quiet = 0;
    m_limit = s_fsh_stare_limit[utlRandRange(0, 2)];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157a30
// Steps: 0 the turn, then the stare motion (0x1a with attr bits 8 and
// 0x10, else 0x2e or at 30 % 0x19); 1 voice; m_limit frames after the
// bark stops, or 100 frames of it, end it.

int FshOutStared::Exec()
{
    if (m_step == 0) {
        if (m_turn_done != 0) {
            m_emy->InactiveTurn();
            u_int attr = m_achr->m_attr[0];
            int motion;
            if ((attr & 8) != 0 && (attr & 0x10) != 0)
                motion = 0x1a;
            else
                motion = utlRandRange(0, 99) > 0x1d ? 0x2e : 0x19;
            setMotion(motion, 0x100);
            m_step = 1;
        }
    } else if (m_step == 1) {
        tickVoice(8, 0x18, 9);
        if (!magBwnIsBarkPhase3()) {
            if (m_quiet == 0) {
                m_quiet = 1;
                m_count = 0;
            }
            if (++m_count > m_limit) {
                stopVoice();
                return 0x13;
            }
        } else if (++m_count > 100) {
            stopVoice();
            return 0x13;
        }
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157b74

void FshOutStared::Term()
{
    termCommon();
}

// ──── FshOutCollision ─────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157b90 // callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (the double angle wrap, emyAbsAngle)

void FshOutCollision::Init()
{
    float pc[4], base[4], rot[4];
    emyGetPCPos(pc);
    getBasePos(base);
    float a = atan2f(pc[0] - base[0], pc[2] - base[2]);
    m_achr->getRot(rot);
    a = emyAbsAngle(a - rot[1]);
    int motion = (double)a >= 1.5707963267948966 ? 0x20 : 0x1d;
    playSE(8);
    setMotion(motion, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157df8 // callcheck: isAchrMotionEnd (chk_motion_end inlined: fshMotionEnd)

int FshOutCollision::Exec()
{
    if (!fshMotionEnd(this))
        return -1;
    return 0x13;
}

// ──── FshOutWin ───────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157e48
// The target is kept unless m_target_dog is above 1 (never: it is 0 or 1).

void FshOutWin::Init()
{
    if ((u_int)m_target_dog > 1)
        selectTarget(100);
    m_range = utlRandRangeF(100.0f, 200.0f);
    emyStartChase(10.0f, this, m_target);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157eb4

int FshOutWin::Exec()
{
    if (m_step == 0 && emyChkChase(m_range, FSH_PI_4, this, m_target, 1) != 0) {
        setMotion(1, 0x100);
        m_se = playSE(0xd);
        g_sndMan.setSeLoop(m_se, 1);
        m_step = 1;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157f44

void FshOutWin::Term()
{
    termCommon();
}

// ──── FshOutTest ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157f60

void FshOutTest::Init()
{
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00157f68 // callcheck: isAchrMotionEnd (chk_motion_end inlined: fshMotionEnd)

int FshOutTest::Exec()
{
    switch (m_step) {
    case 0:
        break;
    case 1:
        if (!fshMotionEnd(this))
            return -1;
        setMotion(0x2c, 0);
        m_step = 2;
        return -1;
    case 2:
        if (!fshMotionEnd(this))
            return -1;
        setMotion(0x2d, 0);
        m_step = 3;
        return -1;
    case 3:
        if (!fshMotionEnd(this))
            return -1;
        setMotion(0x28, 0);
        m_step = 4;
        return -1;
    case 4:
        if (!fshMotionEnd(this))
            return -1;
        break;
    default:
        return -1;
    }
    setMotion(0x2a, 0);
    m_step = 1;
    return -1;
}

// ──── FshOutDead ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001580b4
// Step 0 (the revival, unreachable: Init always leaves step 1): after
// m_wait frames the achr's life back and shown, 0x13. Step 1: out of its
// group and the kill counted, then nothing (step 2).

int FshOutDead::Exec()
{
    if (m_step == 1) {
        leaveGroup();
        g_sequence.m_global[m_emy->m_name + 0x33]++;
        m_step = 2;
    } else if (m_step == 0) {
        if (--m_wait == 0) {
            m_achr->m_flags &= ~0x400u;
            int lp = m_emy->m_lp;
            m_achr->setParam(GAM_PRM_A_LP, &lp);
            m_emy->m_visible = 1;
            m_achr->m_flags &= ~0x2000u;
            g_gamMan.setObj(m_oid, 0x40);
            g_gamMan.resetObj(m_oid, 0x10000000);
            g_gamMan.resetObj(m_oid, 0x400);
            return 0x13;
        }
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001581ec

void FshOutDead::Term()
{
    m_achr->m_flags &= ~0x400u;
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00158218 // callcheck: utlRandPeakLo (the carried-revival draw), setParam
// With revivals left (param 0x5e) and no group it draws a wait and spends
// some of them as if to revive (step 0), but the ROM then stores step 1
// unconditionally, so it never does.

void FshOutDead::Init()
{
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
    m_emy->m_visible = 0;
    g_gamMan.resetObj(m_oid, 0x40);
    g_gamMan.setObj(m_oid, 0x10000000);
    g_gamMan.setObj(m_oid, 0x400);
    m_wait = 0;
    m_emy->stopAllSE();
    m_achr->m_flags |= 0x400;
    m_achr->m_flags |= 0x2000;
    setMotion(m_emy->m_down_supine != 0 ? 0xb : 0xa, 0x100);

    short left;
    m_achr->getParam(0x5e, &left);
    if (left == 0 || m_emy->m_grp_no != 0) {
        m_step = 1;
    } else {
        m_step = 0;
        left -= (short)utlRandPeakLo(1, (u_short)left, 4);
        m_achr->setParam(0x5e, &left);
        m_wait = utlRandPeakLo(0xfa, 0x1d4c, 4);
    }
    m_step = 1;
}
