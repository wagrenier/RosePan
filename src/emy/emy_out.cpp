/* ==========================================================================
 *  emy/emy_out.cpp
 *
 *  EmyOut, the base of every enemy class's per-state outs (HfmOutWalk ...),
 *  and the emy* helpers the outs share (0x166a1c..0x16a914): the enemy's
 *  and its target's node positions, the move / chase / turn task starts,
 *  the sight and reach tests, the out's voice. No source string names the
 *  file; it is named after the class like emy_main / emy_hfm.
 * ======================================================================== */
#include "emy/emy_main.h"

#include <math.h>
#include <string.h>

#include "common/rosepan_stub.h"

#include "dog/dogactor.h"
#include "emy/emy_manager.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magscenecmd.h"
#include "mag/magsystem.h"
#include "snd/sndman.h"
#include "task/tsk_chr.h"
#include "task/tsk_man.h"

/* The move record's per-type data (EmyMain::GetType 0..10). */
struct EmyMoveSpeed
{
    float m_walk;               // ROR 0x00: emyStartMove / emyStartChase's speed
    float m_run;                // ROR 0x04: emyStartChase's second speed
    float m_turn;               // ROR 0x08: the turn rate
    float _0c;
};
static_assert(sizeof(EmyMoveSpeed) == 0x10, "EmyMoveSpeed");
static const EmyMoveSpeed s_moveSpeed[11] = {   /* 0x2f1c90 */
    { 0x1.7a5e34p-1f, 0x1.6e147ap+2f, 0x1.921fb4p-2f },  /* 0 Dwf */
    { 0x1.6872b0p-1f, 0x1.5eb850p+2f, 0x1.247426p-2f },  /* 1 Pig */
    { 0x1.ab020cp-1f, 0x1.9f9db2p+2f, 0x1.657184p-2f },  /* 2 Got */
    { 0x1.a1cac0p+0f, 0x1.a1cac0p+0f, 0x1.0c1522p-1f },  /* 3 Fsh */
    { 0x1.7a5e34p-1f, 0x1.7a5e34p-1f, 0x1.921fb4p-2f },  /* 4 Bds */
    { 0x1.7a5e34p-1f, 0x1.7a5e34p-1f, 0x1.921fb4p-3f },  /* 5 Bdl */
    { 0x1.341892p+2f, 0x1.ae978cp+2f, 0x1.0c1522p-1f },  /* 6 Hfm */
    { 0x1.1db22cp+0f, 0x1.1570a2p+3f, 0x1.2ec68ep-5f },  /* 7 Mrt */
    { 0.0f, 0.0f, 0.0f },                                /* 8 Clr */
    { 0.0f, 0.0f, 0.0f },                                /* 9 Grg */
    { 0x1.2f3b64p+2f, 0x1.c7851ep+3f, 0x1.921fb4p-2f },  /* 10 Nor */
};
static const int s_moveMode[11] = { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 };   /* 0x2f1d50 */
struct EmyMovePair
{
    float m_dist;               // ROR 0x00
    float m_angle;              // ROR 0x04: 0.3 pi
};
static_assert(sizeof(EmyMovePair) == 8, "EmyMovePair");
static const EmyMovePair s_movePair[11] = {     /* 0x2f1d80 */
    { 45.0f, 0x1.e28c70p-1f }, { 60.0f, 0x1.e28c70p-1f }, { 60.0f, 0x1.e28c70p-1f },
    { 60.0f, 0x1.e28c70p-1f }, { 60.0f, 0x1.e28c70p-1f }, { 60.0f, 0x1.e28c70p-1f },
    { 60.0f, 0x1.e28c70p-1f }, { 60.0f, 0x1.e28c70p-1f }, { 60.0f, 0x1.e28c70p-1f },
    { 60.0f, 0x1.e28c70p-1f }, { 60.0f, 0x1.e28c70p-1f },
};

/* The hit nodes emyChkHitReach walks (0x298de8, NULL-terminated). */
static const char *const s_hitNodes[10] = {
    "p_hit", "p_hit1", "p_hit2", "p_hit3", "p_hit4", "p_hit5", "p_hit6", "p_hit7", NULL, NULL,
};

/* PC: the ROM inlines this at every use -- DogModel::getNodeIndexByName
 * (its two asserts) and the actor's vtable slot 17 -- and keeps the
 * matrix's translation row. */
void emyGetNodePos(int no, const char *name, float *pos)
{
    DogActor &actor = g_magSystem.m_scene.m_actors[no];
    sceVu0FMATRIX m;
    actor.getNodeWorldMatrix(m, actor.getNodeIndexByName(name));
    pos[0] = m[3][0];
    pos[1] = m[3][1];
    pos[2] = m[3][2];
}

/* PC: the wrap of an angle into -pi..pi the ROM inlines (in double). */
float emyWrapAngle(float d)
{
    if ((double)d < -3.141592653589793)
        d = (float)((double)d + 6.283185307179586);
    if ((double)d > 3.141592653589793)
        d = (float)((double)d - 6.283185307179586);
    return d;
}

/* PC: EmyOut::mot_speed (linkonce 0x28acac) inlined: m_agl / 60, 1 / 60
 * for 0. */
static float emyMotSpeed(const EmyMain *emy)
{
    float agl = emy->m_agl;
    return agl == 0.0f ? 0x1.111110p-6f /* 0x3c888888, 1 / 60 */ : agl * 0x1.111110p-6f;
}

/* PC: the angle wrap the ROM inlines, then its size. */
float emyAbsAngle(float d)
{
    d = emyWrapAngle(d);
    return d < 0.0f ? -d : d;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166a1c

EmyOut::EmyOut(u_int oid, ACHRstat *achr, EmyMain *emy)
{
    m_oid    = oid;
    m_achr   = achr;
    m_emy    = emy;
    m_se     = -1;
    m_bitten = 0;
    m_20     = 0;
    m_30     = utlRandRange(8, 24);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166a74 // callcheck: operator_delete (the deleting variant is the compiler's)

EmyOut::~EmyOut()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166ad0

void EmyOut::termCommon()
{
    m_emy->setDecideSw(1);
    g_gamMan.resetObj(m_oid, 0x400);
    m_emy->m_move->stop();
    m_emy->m_turn->m_active = 0;
    m_achr->killTasks();
    m_emy->setDecideSw(1);
    m_emy->m_input_flg_motion = 1;
    m_emy->m_down_flg = 0;
    if (m_se >= 0) {
        g_sndMan.stopSe(m_se);
        m_se = -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166b74 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)

void EmyOut::getBasePos(float *pos)
{
    ACHRstat *achr = g_gamMan.getAchr(m_oid);
    emyGetNodePos(g_gamMan.getActorNo(m_oid), "Bip01 Spine1", pos);
    float obj[4];
    achr->getPos(obj);
    pos[1] = obj[1];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166c8c // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)

void EmyOut::getTargetNodePos(float *pos)
{
    ACHRstat *achr = g_gamMan.getAchr(m_target);
    emyGetNodePos(g_gamMan.getActorNo(m_target), "Bip01 Spine1", pos);
    float obj[4];
    achr->getPos(obj);
    pos[1] = obj[1];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166da4

void EmyOut::selectTarget(int rate)
{
    u_int dog = g_gamMan.m_dogPtr->m_oid;
    if (m_achr->m_attr[0] & 1)
        rate = 100;
    u_int pc = g_gamMan.m_pcPtr->m_oid;
    if (rate < 99
        && !emyChkInSight(60.0f, 0x1.921fb4p+2f /* 0x40c90fda, 2 pi */, 0, pc, m_achr)) {
        if (emyChkInSight(60.0f, 0x1.921fb4p+2f, 0, dog, m_achr) || utlRandRange(0, 99) >= rate) {
            m_target = dog;
            m_target_dog = 1;
            return;
        }
    }
    m_target = pc;
    m_target_dog = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166eb4

void EmyOut::selectNearTarget(int rate)
{
    float base[4], pc[4], dog[4];
    getBasePos(base);
    emyGetPCPos(pc);
    emyGetDogPos(dog);
    if ((pc[0] - base[0]) * (pc[0] - base[0]) + (pc[2] - base[2]) * (pc[2] - base[2])
        < (dog[0] - base[0]) * (dog[0] - base[0]) + (dog[2] - base[2]) * (dog[2] - base[2]))
        rate = 100;
    selectTarget(rate);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166f50

void emyStartChase(float range, EmyOut *out, u_int target)
{
    EmyMain *emy = out->m_emy;
    int type = emy->GetType();
    EmyMoveParam prm = {};
    prm.m_carry = 0;
    int item;
    if (type == 2) {
        out->m_achr->getParam(0x67, &item);
        if (item != 0 && g_gamMan.getObjKind(item) == 0xa2e)
            prm.m_carry = 1;
    }
    prm.m_oid         = out->m_oid;
    prm.m_done        = &out->m_move_done;
    prm.m_mode        = s_moveMode[type];
    prm.m_agl         = emy->m_agl;
    prm.m_walk        = s_moveSpeed[type].m_walk;
    prm.m_run         = s_moveSpeed[type].m_run;
    prm.m_dist        = s_movePair[type].m_dist;
    prm.m_angle       = s_movePair[type].m_angle;
    prm.m_turn        = s_moveSpeed[type].m_turn;
    prm.m_parsonality = emy->m_parsonality;
    prm.m_target      = target;
    prm.m_range       = range;
    const EmyKindMoveParam *kind = emy->getMoveParam();
    if (kind != NULL) {     /* PC: a guard from the stub days (getMoveParam never returns NULL) */
        prm.m_kind[0] = kind->m_w[0];
        prm.m_kind[1] = kind->m_w[1];
        prm.m_kind[2] = kind->m_w[2];
        prm.m_kind[3] = kind->m_w[3];
        prm.m_kind4   = kind->m_w[4];
    }
    prm.m_type = type;
    out->m_achr->killTasks();
    emy->m_move->start(&prm);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001670dc

void emyStartMove(float range, EmyOut *out, const float *pos)
{
    EmyMain *emy = out->m_emy;
    int type = emy->GetType();
    EmyMoveParam prm = {};
    prm.m_oid         = out->m_oid;
    prm.m_done        = &out->m_move_done;
    prm.m_mode        = s_moveMode[type];
    prm.m_agl         = emy->m_agl;
    prm.m_walk        = s_moveSpeed[type].m_walk;
    prm.m_run         = 0.0f;
    prm.m_dist        = s_movePair[type].m_dist;
    prm.m_angle       = s_movePair[type].m_angle;
    prm.m_turn        = s_moveSpeed[type].m_turn;
    prm.m_pos[0]      = pos[0];
    prm.m_pos[1]      = pos[1];
    prm.m_pos[2]      = pos[2];
    prm.m_parsonality = emy->m_parsonality;
    prm.m_carry       = 0;
    prm.m_target      = 0;
    prm.m_range       = range;
    const EmyKindMoveParam *kind = emy->getMoveParam();
    if (kind != NULL) {     /* PC: a guard from the stub days (getMoveParam never returns NULL) */
        prm.m_kind[0] = kind->m_w[0];
        prm.m_kind[1] = kind->m_w[1];
        prm.m_kind[2] = kind->m_w[2];
        prm.m_kind[3] = kind->m_w[3];
        prm.m_kind4   = kind->m_w[4];
    }
    prm.m_type = type;
    out->m_achr->killTasks();
    emy->m_move->start(&prm);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00167230

void emyStartTurn(float arg, EmyOut *out, int dog)
{
    EmyTurnParam prm;
    prm.m_done = &out->m_turn_done;
    prm.m_oid  = out->m_oid;
    EmyMain *emy = out->m_emy;
    prm.m_agl  = emy->m_agl;
    ObjStat *obj = dog ? g_gamMan.m_dogPtr : g_gamMan.m_pcPtr;
    prm.m_target = 0;
    if (obj != NULL)
        prm.m_target = obj->m_oid;
    prm.m_arg  = arg;
    prm.m_type = emy->GetType();
    out->m_achr->killTasks();
    out->m_emy->m_turn->start(&prm);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001672e8 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName, fptodp, dpcmp, dpadd, dpsub, dptofp (DogModel::getNodeIndexByName inline; the double wrap in emyAbsAngle)

int emyChkInAngle(float angle, EmyOut *out, u_int target)
{
    float to[4], from[4];
    emyGetNodePos(g_gamMan.getActorNo(target), "Bip01 Spine1", to);
    emyGetNodePos(g_gamMan.getActorNo(out->m_oid), "Bip01 Spine1", from);
    float a = atan2f(to[0] - from[0], to[2] - from[2]);
    float rot = g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(out->m_oid)].getRotY();
    return emyAbsAngle(a - rot) < angle;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016768c // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)

int EmyOut::isSameSecta(u_int target)
{
    float base[4], to[4];
    getBasePos(base);
    emyGetNodePos(g_gamMan.getActorNo(target), "Bip01 Spine1", to);
    GamSecta *a = g_gamMan.pickSectaCell(base, 0);
    GamSecta *b = g_gamMan.pickSectaCell(to, 0);
    return a != NULL && b != NULL && a->m_id == b->m_id;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001677d0

/* The Dwf / Pig / Got motion by m_parsonality (0..2). */
static const int s_attackMotion[3] = { 3, 4, 5 };   /* 0x2f1de0 */

int EmyOut::getAttackMotion(int *a, int *b)
{
    int motion;
    u_int attr = m_achr->m_attr[0];
    if ((attr & 8) && (attr & 0x10)) {
        motion = m_emy->m_motion_list[4];
        *a = m_emy->m_motion_list[13];
        *b = m_emy->m_motion_list[14];
    } else if ((attr & 8) || (attr & 0x10)) {
        motion = m_emy->m_motion_list[5];
    } else {
        int type = m_emy->GetType();
        if (type == 0 || type == 2 || type == 1) {
            int p = m_emy->m_parsonality;
            if (p >= 3)
                p = 0;
            motion = s_attackMotion[p];
            int item;
            m_achr->getParam(0x67, &item);
            if (g_gamMan.getObjKind(item) == 0xa2e) {
                int scissors;
                m_achr->getParam(0x67, &scissors);
                g_magSystem.m_scene.setActorMotion(0x1.11112ep-6f /* 0x3c888897 */,
                                                   g_gamMan.getActorNo(scissors), 4, 0, 0x100);
            }
        } else {
            motion = m_emy->m_motion_list[2];
        }
        *a = m_emy->m_motion_list[11];
        *b = m_emy->m_motion_list[12];
    }
    /* The ROM overwrites both on every path. */
    *a = -1;
    *b = -1;
    return motion;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001679b4

int emyChkChase(float range, float angle, EmyOut *out, u_int target, int useAngle)
{
    if (!g_gamMan.isObj(target) || out->m_move_done == 2) {
        out->m_emy->m_move->stop();
        return 2;
    }
    if (!emyChkInSight(range, angle, useAngle, target, out->m_achr))
        return 0;
    out->m_emy->m_move->stop();
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00167a64 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName, fptodp, dpcmp, dpadd, dpsub, dptofp (DogModel::getNodeIndexByName inline; the wrap in double)

int EmyOut::getTargetDir(float *vec, float *angle)
{
    if (g_taskMan.getTask(m_achr->m_task[0]) == 0 || !g_gamMan.isObj(m_target)) {
        m_achr->killTasks();
        return 2;
    }
    float to[4], base[4];
    emyGetNodePos(g_gamMan.getActorNo(m_target), "Bip01 Spine1", to);
    to[3] = 0.0f;   /* PC: the ROM leaves the w word unset */
    getBasePos(base);
    base[3] = 0.0f;
    sceVu0SubVector(vec, to, base);
    float a = atan2f(vec[0], vec[2]);
    float rot = g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(m_oid)].getRotY();
    *angle = emyWrapAngle(a - rot);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00167d88

void EmyOut::getFrontPos(float dist, float *pos)
{
    float rot = g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(m_oid)].getRotY();
    getBasePos(pos);
    pos[1] += 0.0f;
    pos[0] += sinf(rot) * dist;
    pos[2] += cosf(rot) * dist;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00167e38 // callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (the double wrap in emyAbsAngle)

float EmyOut::getTargetAngle()
{
    float target[4], base[4];
    if (m_target_dog == 0)
        emyGetPCPos(target);
    else if (m_target_dog == 1)
        emyGetDogPos(target);
    getBasePos(base);
    float a = atan2f(base[0] - target[0], base[2] - target[2]);
    int no = g_gamMan.getActorNo(m_target) & 0xffff;
    float rot = g_magSystem.m_scene.m_actors[no].getRotY();
    return emyAbsAngle(a - rot);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001680a0 // callcheck: getTask, getActorNo, getRotY (they only pick the f13 word chrMtnMove2Achr does not read)
// The ROM passes the actor's rot y in f13 (32768.5, 0x47000100, while
// the achr's motion task runs) and 1 in t0; chrMtnMove2Achr reads neither.

void EmyOut::setMotion(int motion, int flags)
{
    if (flags == 0)
        m_emy->m_input_flg_motion = 0;
    float agl = m_emy->m_agl;
    float speed = agl == 0.0f ? 0x1.111110p-6f /* 0x3c888888, 1 / 60 */ : agl * 0x1.111110p-6f;
    chrMtnMove2Achr(speed, m_oid, motion, -1, flags);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001681c8 // callcheck: getTask, getActorNo, getRotY (they only pick the f13 word chrMtnMove2Achr does not read)

void EmyOut::setMotionSpeed(float speed, int motion, int flags)
{
    if (flags == 0)
        m_emy->m_input_flg_motion = 0;
    chrMtnMove2Achr(speed, m_oid, motion, -1, flags);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001682b0
// The ROM also passes 32768.5 (0x47000100) in f13 and 1 in t0, which
// chrMtnMove2Achr does not read.

void EmyOut::setWalkMotion()
{
    int motion;
    if ((m_achr->m_attr[0] & 8) && (m_achr->m_attr[0] & 0x10))
        motion = m_emy->m_motion_list[1];
    else
        motion = m_emy->m_motion_list[0];
    float agl = m_emy->m_agl;
    float speed = agl == 0.0f ? 0x1.111110p-6f /* 0x3c888888, 1 / 60 */ : agl * 0x1.111110p-6f;
    chrMtnMove2Achr(speed, m_oid, motion, -1, 0x100);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016834c
// setWalkMotion's body (the ROM has it twice).

void EmyOut::resetWalkMotion()
{
    int motion;
    if ((m_achr->m_attr[0] & 8) && (m_achr->m_attr[0] & 0x10))
        motion = m_emy->m_motion_list[1];
    else
        motion = m_emy->m_motion_list[0];
    chrMtnMove2Achr(emyMotSpeed(m_emy), m_oid, motion, -1, 0x100);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001683e8

void EmyOut::setBaseMotion()
{
    int motion = 2;
    if ((m_achr->m_attr[0] & 8) && (m_achr->m_attr[0] & 0x10))
        motion = 10;
    chrMtnMove2Achr(emyMotSpeed(m_emy), m_oid, motion, -1, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00168478

void EmyOut::startBitten()
{
    if (m_18 != 0)
        return;
    if (!(m_achr->m_flags & 0x400)) {
        int bark = magBwnBarkQuery(m_oid);
        int no = bark == 1 ? 9 : bark == 0 ? 8 : 10;
        setMotion(m_emy->m_motion_list[no], 0x100);
        m_achr->m_flags |= 0x400;
    }
    m_bitten = 1;
    m_18 = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00168520

void EmyOut::tickBitten()
{
    if (m_bitten != 0) {
        m_bitten = 0;
        switch (m_emy->GetType()) {
        case 0: case 1: case 3:
            resetWalkMotion();
            break;
        case 2:
            setGotWalkMotion();
            break;
        case 4: case 5: case 6: case 7: case 8: case 9: case 10:
            setWalkMotion();
            break;
        }
    }
    m_18 = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001685c0 // callcheck: chrMtnMove2Achr, getTask, getActorNo, getRotY (setMotion inlined in the ROM, its f13 word)

void EmyOut::setGotMotion(int motion, int flags)
{
    setMotion(motion, flags);
    int item;
    m_achr->getParam(0x67, &item);
    if (g_gamMan.getObjKind(item) != 0xa2e)
        return;
    int sm;
    switch (motion) {
    case 10:   sm = 3; break;
    case 11:   sm = 4; break;
    case 0x4c: sm = 6; break;
    case 0x4d: sm = 7; break;
    case 0x4e: sm = 2; break;   /* PC: the ROM passes its caller's s3 here (unset) */
    case 0x4f: sm = 8; break;
    case 0x50: sm = 12; break;
    default:   sm = 2; break;
    }
    int scissors;
    m_achr->getParam(0x67, &scissors);
    g_magSystem.m_scene.setActorMotion(0x1.11112ep-6f /* 0x3c888897 */,
                                       g_gamMan.getActorNo(scissors), sm, 0, 0x100);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001687bc

void EmyOut::changeMotion(int motion, int flags)
{
    if (m_emy->GetType() == 2) {
        setGotMotion(motion, flags);
        return;
    }
    setMotion(motion, flags);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00168844

void EmyOut::setGotWalkMotion()
{
    int motion;
    u_int attr = m_achr->m_attr[0];
    if ((attr & 8) && (attr & 0x10)) {
        motion = m_emy->m_motion_list[1];
    } else {
        int item;
        m_achr->getParam(0x67, &item);
        if (g_gamMan.getObjKind(item) == 0xa2e) {
            motion = 10;
            int scissors;
            m_achr->getParam(0x67, &scissors);
            g_magSystem.m_scene.setActorMotion(0x1.11112ep-6f /* 0x3c888897 */,
                                               g_gamMan.getActorNo(scissors), 3, 0, 0x100);
        } else {
            motion = m_emy->m_motion_list[0];
        }
    }
    chrMtnMove2Achr(emyMotSpeed(m_emy), m_oid, motion, -1, 0x100);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016898c

void EmyOut::startMotion(int motion, int flags)
{
    setMotion(motion, flags);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001689a8

void EmyOut::testMotionEnd()
{
    if (g_gamMan.isAchrMotionEnd(m_oid))
        m_emy->m_input_flg_motion = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001689e8

void EmyOut::leaveGroup()
{
    EmyMain *emy = m_emy;
    if (emy->m_grp_no != 0) {
        emy->m_grp_state = 5;
        emy->m_grp_respawn = g_emyManager.countGrpDead(emy->m_grp_no);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00168a30 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName, fptodp, dpcmp, dpadd, dpsub, dptofp (DogModel::getNodeIndexByName inline; the wraps in double)

int EmyOut::slideToTarget(int *node, const int *back, int *count, int frames)
{
    int no = g_gamMan.getActorNo(m_target);
    DogActor &target = g_magSystem.m_scene.m_actors[no];
    float turn = *back == 0 ? -0x1.921fb4p+1f /* 0xc0490fda */ : 0.0f;
    *node = target.getNodeIndexByName(*back == 0 ? "p_sp_f" : "p_sp_b");
    sceVu0FMATRIX m;
    target.getNodeWorldMatrix(m, *node);

    /* The ROM sets the end frame's position and rotation to 0. */
    float pos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float rot[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    int done;
    if (++*count == frames) {
        done = 1;
    } else {
        float t = 1.0f / (float)(frames - *count);
        done = 0;
        getBasePos(pos);
        pos[0] += (m[3][0] - pos[0]) * t;
        pos[1] += (m[3][1] - pos[1]) * t;
        pos[2] += (m[3][2] - pos[2]) * t;
        float want = emyWrapAngle(target.getRotY() + turn);
        float cur = g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(m_oid)].getRotY();
        rot[1] = emyWrapAngle(cur + emyWrapAngle(want - cur) * t);
    }
    m_achr->setPos(pos);
    m_achr->setRot(rot);
    return done;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00168ea0 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName, fptodp, dpcmp, dpadd, dpsub, dptofp (DogModel::getNodeIndexByName inline; the wrap in double)

void EmyOut::snapToTarget(int *node, const int *back)
{
    int no = g_gamMan.getActorNo(m_target);
    DogActor &target = g_magSystem.m_scene.m_actors[no];
    float turn = *back == 0 ? 0.0f : 0x1.921fb4p+1f /* 0x40490fda */;
    *node = target.getNodeIndexByName(*back == 0 ? "p_sp_f" : "p_sp_b");
    sceVu0FMATRIX m;
    target.getNodeWorldMatrix(m, *node);
    float pos[4] = { m[3][0], m[3][1], m[3][2], 0.0f };
    pos[1] = g_gamMan.getFloorY(pos);
    m_achr->setPos(pos);
    float rot[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    rot[1] = emyWrapAngle((float)((double)(target.getRotY() + turn) + 3.141592653589793));
    m_achr->setRot(rot);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016913c // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)

void EmyOut::dropItem()
{
    int no = g_gamMan.getActorNo(m_oid);
    int item;
    m_achr->getParam(0x67, &item);
    if (item == 0)
        return;
    DogActor &actor = g_magSystem.m_scene.m_actors[no];
    int node = actor.getNodeIndexByName("p_hn_r");
    g_gamMan.resetObj(item, 0x4000000);
    int itemNo = g_gamMan.getActorNo(item);
    g_magSystem.setActorParent(itemNo, -1, 0);
    g_magSystem.m_scene.setActorMotion(0x1.11112ep-6f /* 0x3c888897 */, itemNo, 1, 0, 0x100);
    float ofs[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    sceVu0FMATRIX m;
    actor.getNodeWorldMatrix(m, node);
    float pos[4];
    pos[0] = m[3][0] + ofs[0];
    pos[2] = m[3][2] + ofs[2];
    pos[1] = m[3][1];
    pos[3] = 0.0f;
    pos[1] = g_gamMan.getFloorY(pos) + ofs[1];
    g_gamMan.setObjPos(item, pos);
    if (g_gamMan.getObjKind(item) == 0xa2e) {
        /* The scissors put down: emyAttachItem's four motions back. */
        m_emy->m_motion_list[0]  = 1;
        m_emy->m_motion_list[2]  = 3;
        m_emy->m_motion_list[11] = 8;
        m_emy->m_motion_list[12] = 9;
    }
    if (g_gamMan.testFchr100000(item))
        g_gamMan.setObj(item, 0x10);
    u_short none = 0xa00;
    m_achr->setParam(0x66, &none);
    item = 0;
    m_achr->setParam(0x67, &item);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001693a4

static const int s_selectRate[6] = { 0, 25, 50, 75, 100, 0 };   /* 0x2f1df0 */

int EmyOut::getSelectRate()
{
    u_short v[8];
    m_achr->getParam(0x5a, v);
    if (v[0] > 5)
        v[0] = 0;
    return s_selectRate[v[0]];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001693ec

int EmyOut::tickVoice(int lo, int hi, int se)
{
    if (m_se < 0) {
        if (--m_30 < 0) {
            m_30 = utlRandRange(lo, hi);
            m_se = playSE(se);
            return 1;
        }
    } else if (g_sndMan.getSeState(m_se) == -1) {
        m_se = -1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016947c

void EmyOut::stopVoice()
{
    g_sndMan.stopSe(m_se);
    m_se = -1;
    m_30 = utlRandRange(8, 24);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001694cc

int EmyOut::playSE(int no)
{
    float pos[4];
    getBasePos(pos);
    pos[3] = 0.0f;
    return g_gamMan.m_sndMan.playSE(m_emy->getSE(no), false, pos, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00169538 // callcheck: memset (the cleared map array)

EmyStateMap::EmyStateMap(int state)
{
    m_count = 0;
    memset(m_map, 0, sizeof(m_map));
    m_cur = state;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016957c // callcheck: ~GamIdMap, operator_delete (delete m_map[i]; the deleting variant is the compiler's)

EmyStateMap::~EmyStateMap()
{
    for (int i = 0; i < 40; i++) {
        if (m_map[i] != NULL)
            delete m_map[i];
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00169604

GamIdMap *EmyStateMap::getMap(int state)
{
    return m_map[state];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00169614

void EmyStateMap::addMap(int state, GamIdMap *map)
{
    m_map[state] = map;
    m_count++;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00169630 // callcheck: ~GamIdMap, operator_delete (delete m_map[state])

void EmyStateMap::removeMap(int state)
{
    if (m_map[state] != NULL)
        delete m_map[state];
    m_map[state] = NULL;
    m_count--;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016968c

int EmyStateMap::next(int input)
{
    if (m_cur == 0)
        return 0;
    GamIdMap *map = getMap(m_cur);
    if (map == NULL) {
        m_cur = 0;
        return 0;
    }
    int state = (int)map->find((u_short)input);
    if (state != 0)
        m_cur = state;
    return state;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001696fc

int EmyStateMap::peek(int input)
{
    if (m_cur == 0)
        return 0;
    GamIdMap *map = getMap(m_cur);
    if (map == NULL) {
        m_cur = 0;
        return 0;
    }
    return (int)map->find((u_short)input);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016976c

int emyCanReach(const float *from, const float *to)
{
    GamSecta *a = g_gamMan.pickSectaCell(from, 0);
    GamSecta *b = g_gamMan.pickSectaCell(to, 0);
    if (b == NULL || a == NULL)
        return 0;
    if (a->m_id == b->m_id)
        return 1;
    return g_gamMan.countSectaPath(a->m_id, b->m_id, 0) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00169810 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)

int emyAttachItem(u_int oid, u_int item)
{
    if (!g_gamMan.isObj(item))
        return 0;
    int no = g_gamMan.getActorNo(item);
    FCHRstat *fchr = g_gamMan.getFchr(item);
    if (fchr == NULL)
        return 0;
    if (g_gamMan.testObjFlags(item, 0x4000000))
        return 0;

    float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    fchr->setParam(5, zero);
    fchr->setParam(6, zero);
    int parent = g_gamMan.getActorNo(oid);
    int node = g_magSystem.m_scene.m_actors[parent].getNodeIndexByName("p_hn_r");
    g_magSystem.setActorParent(no, parent, node);
    g_gamMan.setObj(item, 0x4000000);
    g_magSystem.m_scene.setActorMotion(0x1.11112ep-6f /* 0x3c888897 */, no, 2, 0, 0);
    EmyMain *emy = g_emyManager.findEmy(oid);
    if (g_gamMan.getObjKind(item) == 0xa2e) {
        /* Kind 0xa2e (the Got's carried item) swaps four motions. */
        emy->m_motion_list[12] = 0xf;
        emy->m_motion_list[0]  = 10;
        emy->m_motion_list[2]  = 0xb;
        emy->m_motion_list[11] = 0xe;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00169a14 // callcheck: sqrtf (the NaN path of sqrt.s)

float emyVecLength(const float *v)
{
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00169a68 // callcheck: sceVu0SubVector (the difference written out)

float emyVecDistance(const float *a, const float *b)
{
    float d[4] = { a[0] - b[0], a[1] - b[1], a[2] - b[2], 0.0f };
    return emyVecLength(d);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00169a98

int emyChkNearXZ(float r, const float *a, const float *b)
{
    float dx = a[0] - b[0];
    if (dx < 0.0f)
        dx = -dx;
    float dz = a[2] - b[2];
    if (dz < 0.0f)
        dz = -dz;
    return dx < r && dz < r && dx * dx + dz * dz < r * r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00169b0c

int emyChkNearXZY(float r, const float *a, const float *b)
{
    float dx = a[0] - b[0];
    if (dx < 0.0f)
        dx = -dx;
    float dy = a[1] - b[1];
    if (dy < 0.0f)
        dy = -dy;
    float dz = a[2] - b[2];
    if (dz < 0.0f)
        dz = -dz;
    return dx < r && dz < r && dx * dx + dz * dz < r * r && dy < 100.0f;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00169bac // callcheck: dbgAssertFailed, i3rGetNodeIndexByName, fptodp, dpcmp, dpadd, dpsub, dptofp (DogModel::getNodeIndexByName inline; the double wrap in emyAbsAngle)

int emyChkInSight(float range, float angle, int useAngle, u_int target, ACHRstat *achr)
{
    float to[4], from[4];
    emyGetNodePos(g_gamMan.getActorNo(target), "Bip01 Spine1", to);
    u_int self;
    achr->getParam(1, &self);
    emyGetNodePos(g_gamMan.getActorNo(self), "Bip01 Spine1", from);
    if (!emyChkNearXZ(range, from, to))
        return 0;
    if (useAngle) {
        float a = atan2f(to[0] - from[0], to[2] - from[2]);
        float rot = g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(self)].getRotY();
        if (angle <= emyAbsAngle(a - rot))
            return 0;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00169f94 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)

int emyChkHitReach(float range, u_int oid, int nodes, u_int target)
{
    if (nodes >= 9)
        return 0;
    float to[4];
    emyGetNodePos(g_gamMan.getActorNo(target), "Bip01 Spine1", to);
    float r = g_gamMan.getObjRadius(target, NULL);
    for (int i = 0; i < nodes; i++) {
        float hit[4];
        emyGetNodePos(g_gamMan.getActorNo(oid), s_hitNodes[i], hit);
        if (emyChkNearXZY(range + r, hit, to))
            return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016a1dc // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)

int emyChkHitReachXZ(float range, u_int oid, int nodes, u_int target)
{
    if (nodes >= 9)
        return 0;
    float to[4];
    emyGetNodePos(g_gamMan.getActorNo(target), "Bip01 Spine1", to);
    float r = g_gamMan.getObjRadius(target, NULL);
    for (int i = 0; i < nodes; i++) {
        float hit[4];
        emyGetNodePos(g_gamMan.getActorNo(oid), s_hitNodes[i], hit);
        if (emyChkNearXZ(range + r, hit, to))
            return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016a424 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)

int emyChkPointReach(float range, const float *points, int n, u_int target)
{
    if (n >= 9)
        return 0;
    float to[4];
    emyGetNodePos(g_gamMan.getActorNo(target), "Bip01 Spine1", to);
    float r = g_gamMan.getObjRadius(target, NULL);
    for (int i = 0; i < n; i++, points += 3) {
        if (emyChkNearXZ(range + r, points, to))
            return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016a594 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)

void emyGetSEPos(u_int oid, float *pos)
{
    g_gamMan.getObjPos(oid, pos);
    if ((oid >> 28 & 3) == 1) {
        float node[4];
        emyGetNodePos(g_gamMan.getActorNo(oid), "Bip01 Spine1", node);
        pos[2] = node[2];
        pos[0] = node[0];
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016a6b4 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)

void emyGetPCPos(float *pos)
{
    u_int oid = 0;
    if (g_gamMan.m_pcPtr != NULL)
        oid = g_gamMan.m_pcPtr->m_oid;
    float node[4];
    emyGetNodePos(g_gamMan.getActorNo(oid), "Bip01 Spine1", node);
    pos[1] = 0.0f;
    pos[0] = node[0];
    pos[2] = node[2];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016a7ac // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)

void emyGetDogPos(float *pos)
{
    u_int oid = 0;
    if (g_gamMan.m_dogPtr != NULL)
        oid = g_gamMan.m_dogPtr->m_oid;
    float node[4];
    emyGetNodePos(g_gamMan.getActorNo(oid), "Bip01 Spine1", node);
    pos[1] = 0.0f;
    pos[0] = node[0];
    pos[2] = node[2];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016a8a4

int emyPlaySE(u_int oid, int no)
{
    float pos[4];
    emyGetSEPos(oid, pos);
    return g_gamMan.m_sndMan.playSE(g_emyManager.getEmySE(oid, no), false, pos, 0);
}
