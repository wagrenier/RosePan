/* ==========================================================================
 *  task/tsk_enemy.cpp
 *
 *  The enemy behaviour tasks (pinned 0x20067c..0x203d94); see
 *  tsk_enemy.h. Written so far: TskEnemyTurn complete; TskEnemyMove's
 *  lifecycle (start / stop / suspend / resume / pause / term), exec's
 *  frame, pickEvent, tickVoice and getTargetPos. Left: plan 0x200988,
 *  step 0x200d18 and the route helpers 0x201164..0x202e20.
 * ======================================================================== */
#include "task/tsk_enemy.h"
#include "common/rosepan_stub.h"

#include <math.h>

#include "dog/dogactor.h"
#include "emy/emy_main.h"
#include "emy/emy_manager.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "sct/sctlayer.h"
#include "snd/sndman.h"
#include "utl/utlrand.h"

/* The enemy actor's translation, set by TskEnemyMove::exec each frame
 * for its route helpers (bss 0x6e97d8). */
static float s_movePos[3];

/* The voices of types 6 (16) and 10 (4): TskEnemyMove's (data 0x2ff5c0)
 * and TskEnemyTurn's (data 0x2ff6a8). */
static const int s_moveVoice6[16] = {
    0x15, 0x24, 0x2b, 0x2c, 0x18, 0x19, 0x1a, 0x1b,
    0x15, 0x24, 0x2b, 0x1f, 0x20, 0x2c, 0x24, 0x15,
};
static const int s_moveVoice10[4] = { 0x55, 0x5a, 0x5f, 0x60 };       /* 0x2ff600 */
static const int s_turnVoice6[16] = {
    0x15, 0x24, 0x2b, 0x2c, 0x18, 0x19, 0x1a, 0x1b,
    0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x15,
};
static const int s_turnVoice10[4] = { 0x55, 0x5a, 0x5f, 0x60 };       /* 0x2ff6e8 */

/* The motion speed a suspended task leaves its actor (0x33d6bf94). */
static const float TSK_ENEMY_STILL = 0x1.ad7f28p-24f;

/* PC: the wrap of an angle into -pi..pi the ROM inlines (in double). */
static float tskEnemyWrapAngle(float d)
{
    if ((double)d < -3.141592653589793)
        d = (float)((double)d + 6.283185307179586);
    if ((double)d > 3.141592653589793)
        d = (float)((double)d - 6.283185307179586);
    return d;
}

/* PC: the angle from pos to to, less the actor's facing, wrapped. */
static float tskEnemyAngleTo(DogActor *dog, const float *pos, const float *to)
{
    return tskEnemyWrapAngle(atan2f(to[0] - pos[0], to[2] - pos[2]) - dog->getRotY());
}

/* PC: the PC's speed by its state class the two getTargetPos inline: 6 /
 * 7 by MagSystem's PC word 0xe4, 8 2.5, else 0. */
static float tskEnemyPCLead()
{
    int cls = g_magSystem.getPCStateClass();
    if (cls == 7)
        return g_magSystem.getPCWordE4() == 0 ? 0x1.bb13b0p+2f /* 90 / 13 */
                                              : 0x1.499998p+1f; /* 2.575 */
    if (cls == 6)
        return g_magSystem.getPCWordE4() == 0 ? 0x1.db6db6p+1f /* 26 / 7 */
                                              : 0x1.499998p+1f;
    if (cls == 8)
        return 2.5f;
    return 0.0f;
}

/* PC: the PC's oid, 0 without one (GamMan::m_pcPtr, bss 0x7a3350). */
static u_int tskEnemyPCOid()
{
    return g_gamMan.m_pcPtr != NULL ? g_gamMan.m_pcPtr->m_oid : 0;
}

// ──────────────────────────────────────────────────────────────────────
// TskEnemyMove
// ──────────────────────────────────────────────────────────────────────
// ROR 0x002006b0

TskEnemyMove::TskEnemyMove()
{
    m_pause = 0;
    m_active = 0;
    m_dog = NULL;
    /* PORT: see the header; m_se too (term reads it). */
    m_se = -1;
    m_wait0 = 0;
    m_wait1 = 0;
    m_wait3 = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00200684

void TskEnemyMove::term()
{
    if (m_se >= 0)
        g_sndMan.stopSe(m_se);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00200700 // callcheck: GamMan::getAchr (the achr's actor no)

void TskEnemyMove::start(const EmyMoveParam *prm)
{
    m_active = 1;
    m_pause = 0;
    m_oid = prm->m_oid;
    m_actor = g_gamMan.getAchr(m_oid)->m_actor;
    m_step = 0;
    m_dog = &g_magSystem.m_scene.m_actors[m_actor];
    m_done = prm->m_done;
    *m_done = 0;
    m_restart = 1;
    m_target = prm->m_target;
    m_pos[0] = prm->m_pos[0];
    m_pos[1] = prm->m_pos[1];
    m_pos[2] = prm->m_pos[2];
    m_48 = prm->_0e;
    m_mode = prm->m_mode;
    m_speed = prm->m_agl * 0x1.11112ep-6f; /* 0x3c888897, 1 / 60 */
    m_walkSpeed = prm->m_walk * prm->m_agl;
    m_runSpeed = prm->m_run * prm->m_agl;
    m_dist = prm->m_dist;
    m_angle = prm->m_angle;
    m_walk = prm->m_walk;
    m_motionFlags = 0x40;
    m_motion = 0;
    m_68 = 0;
    m_6c = 0;
    m_idleMotion = prm->m_parsonality + 3;
    m_74 = 0;
    m_carry = prm->m_carry;
    m_turn = prm->m_turn;
    m_keepRadius = g_gamMan.getObjKeepRadius(m_oid, NULL);
    float r = m_target == 0 ? 0.0f : g_gamMan.getObjRadius(m_target, NULL);
    m_stopRange = r + m_keepRadius + prm->m_range;
    m_layer = g_emyManager.getEmyTypeFlag(m_oid);
    m_kind0 = (int)prm->m_kind[0];
    m_kind1 = (int)prm->m_kind[1];
    m_kind2 = (int)prm->m_kind[2];
    m_kind3 = (int)prm->m_kind[3];
    m_kind4 = prm->m_kind4;
    m_voiceWait = utlRandRange(15, 30);
    m_se = -1;
    m_type = prm->m_type;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002008f0

int TskEnemyMove::exec(int id, int time, int arg)
{
    (void)id; (void)time; (void)arg;
    if (m_pause == 0 && m_active != 0) {
        s_movePos[0] = m_dog->m_trans[0];
        s_movePos[1] = m_dog->m_trans[1];
        s_movePos[2] = m_dog->m_trans[2];
        float pos[4];
        emyGetSEPos(m_oid, pos);
        if (plan(pos))
            step(pos);
        tickVoice(pos);
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00200bc4

int TskEnemyMove::pickEvent()
{
    int ev = 0;
    if (m_step != 0)
        return 0;
    if (m_restart != 0) {
        m_restart = 0;
        return 1;
    }
    if (m_kind0 != 0 && m_74 == 0) {
        if (--m_wait0 < 0) {
            m_wait0 = utlRandRange(m_kind0 * 80, m_kind0 * 120) / 100;
            ev = 4;
        }
    }
    if (m_kind3 != 0) {
        if (--m_wait3 < 0) {
            m_wait3 = utlRandRange(m_kind2 * 80, m_kind2 * 120) / 100;
            ev = 3;
        }
    }
    if (m_kind1 != 0) {
        if (--m_wait1 < 0) {
            m_wait1 = utlRandRange(m_kind1 * 80, m_kind1 * 120) / 100;
            ev = 2;
        }
    }
    return ev;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00202ec8

void TskEnemyMove::pause(int on)
{
    if (m_active == 0)
        return;
    if (on == 0) {
        if (m_dog != NULL)
            m_dog->m_motion.m_speed = m_speed;
    } else if (m_dog != NULL) {
        m_dog->m_motion.m_speed = TSK_ENEMY_STILL;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00202f10

void TskEnemyMove::stop()
{
    m_active = 0;
    g_sndMan.stopSe(m_se);
    m_se = -1;
    g_emyManager.setEmyWork(m_oid, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00202f60

void TskEnemyMove::suspend()
{
    m_pause = 1;
    g_sndMan.stopSe(m_se);
    if (m_active != 0 && m_dog != NULL)
        m_dog->m_motion.m_speed = TSK_ENEMY_STILL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00202fbc

void TskEnemyMove::resume()
{
    m_pause = 0;
    if (m_active != 0 && m_dog != NULL)
        m_dog->m_motion.m_speed = m_speed;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00202fe4

void TskEnemyMove::tickVoice(const float *pos)
{
    if (m_se >= 0) {
        if (g_sndMan.getSeState(m_se) != -1)
            return;
        m_se = -1;
        g_emyManager.setEmyWork(m_oid, 0);
    }
    if (--m_voiceWait > 0)
        return;
    if (emyChkNearXZ(60.0f, pos, m_goal))
        return;
    m_voiceWait = utlRandRange(30, 80);
    if (g_emyManager.countEmyWork() > 3)
        return;
    /* Types 7..9 (and below 0) mark the work word without a voice: with
     * m_se still -1 nothing clears it before stop, as in the ROM. */
    if (m_type == 6)
        m_se = emyPlaySE(m_oid, s_moveVoice6[utlRandRange(0, 15)]);
    else if (m_type == 10)
        m_se = emyPlaySE(m_oid, s_moveVoice10[utlRandRange(0, 3)]);
    else if (m_type >= 0 && m_type < 6)
        m_se = emyPlaySE(m_oid, 12);
    g_emyManager.setEmyWork(m_oid, 1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0020313c // callcheck: getPCStateClass, getPCWordE4, dbgAssertFailed, i3rGetNodeIndexByName, sqrtf (inlined: tskEnemyPCLead, emyGetNodePos's DogModel::getNodeIndexByName, sqrt.s)

void TskEnemyMove::getTargetPos(u_int target, float *pos)
{
    if (target != tskEnemyPCOid()) {
        emyGetSEPos(target, pos);
        return;
    }
    g_gamMan.getObjPos(target, pos);
    float node[4];
    emyGetNodePos(g_gamMan.getActorNo(target), "Bip01 Spine1", node);
    pos[0] = node[0];
    pos[2] = node[2];

    float self[4];
    self[0] = pos[0];
    self[1] = pos[1];
    self[2] = pos[2];
    emyGetSEPos(m_oid, self);
    float dx = pos[0] - self[0];
    float dz = pos[2] - self[2];
    float frames = sqrtf(dx * dx + dz * dz) / m_walk;
    if (frames > 250.0f)
        frames = 250.0f;
    float lead = tskEnemyPCLead() * frames;
    /* The enemy actor's facing, not the PC's, as the ROM reads it. */
    float rot = m_dog->getRotY();
    float led[4];
    led[0] = pos[0] + sinf(rot) * lead;
    led[1] = pos[1];
    led[2] = pos[2] + cosf(rot) * lead;
    GamSecta *hit[4];
    g_magSystem.m_scene.m_8542c.findCells(led, hit, 0, 40.0f, 0.0f);
    if (hit[m_layer] != NULL) {
        pos[0] = led[0];
        pos[2] = led[2];
        pos[1] = led[1];
    }
}

// ──────────────────────────────────────────────────────────────────────
// TskEnemyTurn
// ──────────────────────────────────────────────────────────────────────
// ROR 0x00203468

void TskEnemyTurn::term()
{
    if (m_se >= 0)
        g_sndMan.stopSe(m_se);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00203494

void TskEnemyTurn::init()
{
    if (m_se >= 0)
        g_sndMan.stopSe(m_se);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002034c0

TskEnemyTurn::TskEnemyTurn()
{
    m_dog = NULL;
    m_active = 0;
    /* PORT: the ROM leaves these as the heap had them; init (addTask,
     * before any start) reads m_se, exec m_pause. */
    m_se = -1;
    m_pause = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0020350c // callcheck: GamMan::getAchr (the achr's actor no)

void TskEnemyTurn::start(const EmyTurnParam *prm)
{
    m_active = 1;
    m_oid = prm->m_oid;
    m_actor = g_gamMan.getAchr(m_oid)->m_actor;
    m_dog = &g_magSystem.m_scene.m_actors[m_actor];
    m_done = prm->m_done;
    *m_done = 0;
    m_target = prm->m_target;
    m_right = 1;
    m_motion = 0;
    m_speed = prm->m_agl * 0x1.11112ep-6f; /* 0x3c888897, 1 / 60 */
    m_range = prm->m_arg;
    m_voiceWait = utlRandRange(15, 30);
    m_se = -1;
    m_type = prm->m_type;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002035d8

int TskEnemyTurn::exec(int id, int time, int arg)
{
    (void)id; (void)time; (void)arg;
    if (m_pause == 0 && m_active != 0) {
        if (!g_gamMan.isObj(m_target)) {
            *m_done = 2;
            m_targetPos[0] = 0.0f;
            m_targetPos[2] = 0.0f;
            m_targetPos[1] = 0.0f;
        } else {
            float pos[4];
            emyGetSEPos(m_oid, pos);
            getTargetPos(m_target, m_targetPos);
            setTurnMotion(pos);
            turn(pos);
            tickVoice(pos);
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00203684 // callcheck: atan2f, getRotY, fptodp, dpcmp, dpadd, dpsub, dptofp (tskEnemyAngleTo: the double wrap inlined)

void TskEnemyTurn::turn(const float *pos)
{
    float d = tskEnemyAngleTo(m_dog, pos, m_targetPos);
    if (d > 0.0f) {
        if (m_right != 0)
            setTurnMotion(pos);
    } else if (m_right == 0) {
        setTurnMotion(pos);
    }
    if (!g_magSystem.m_scene.m_actors[m_actor].isModeSet())
        setTurnMotion(pos);
    if (d < 0.0f)
        d = -d;
    if (d < m_range)
        *m_done = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00203828 // callcheck: atan2f, getRotY, fptodp, dpcmp, dpadd, dpsub, dptofp (tskEnemyAngleTo: the double wrap inlined)

void TskEnemyTurn::setTurnMotion(const float *pos)
{
    int motion;
    if (tskEnemyAngleTo(m_dog, pos, m_targetPos) > 0.0f) {
        m_right = 0;
        motion = 9;
    } else {
        motion = 8;
        m_right = 1;
    }
    if (m_motion != motion) {
        m_dog->changeMotion(m_speed, motion, 1, -1, -1);
        m_motion = motion;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0020395c

void TskEnemyTurn::pause(int on)
{
    if (m_active == 0)
        return;
    if (on == 0) {
        if (m_dog != NULL)
            m_dog->m_motion.m_speed = m_speed;
    } else if (m_dog != NULL) {
        m_dog->m_motion.m_speed = TSK_ENEMY_STILL;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002039a4

void TskEnemyTurn::suspend()
{
    m_pause = 1;
    g_sndMan.stopSe(m_se);
    if (m_active != 0 && m_dog != NULL)
        m_dog->m_motion.m_speed = TSK_ENEMY_STILL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00203a00

void TskEnemyTurn::resume()
{
    m_pause = 0;
    if (m_active != 0 && m_dog != NULL)
        m_dog->m_motion.m_speed = m_speed;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00203a28

void TskEnemyTurn::tickVoice(const float *pos)
{
    if (m_se >= 0) {
        if (g_sndMan.getSeState(m_se) != -1)
            return;
        m_se = -1;
    }
    if (--m_voiceWait > 0)
        return;
    if (emyChkNearXZ(180.0f, pos, m_targetPos))
        return;
    m_voiceWait = utlRandRange(30, 80);
    if (m_type == 6)
        m_se = emyPlaySE(m_oid, s_turnVoice6[utlRandRange(0, 15)]);
    else if (m_type == 10)
        m_se = emyPlaySE(m_oid, s_turnVoice10[utlRandRange(0, 3)]);
    else if (m_type >= 0 && m_type < 6)
        m_se = emyPlaySE(m_oid, 12);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00203b44 // callcheck: getPCStateClass, getPCWordE4, dbgAssertFailed, i3rGetNodeIndexByName (inlined: tskEnemyPCLead, emyGetNodePos's DogModel::getNodeIndexByName)

void TskEnemyTurn::getTargetPos(u_int target, float *pos)
{
    if (target != tskEnemyPCOid()) {
        emyGetSEPos(target, pos);
        return;
    }
    g_gamMan.getObjPos(target, pos);
    float node[4];
    emyGetNodePos(g_gamMan.getActorNo(target), "Bip01 Spine1", node);
    pos[0] = node[0];
    pos[2] = node[2];
    float lead = tskEnemyPCLead() * 3.0f;
    /* The enemy actor's facing, as in TskEnemyMove::getTargetPos. */
    float rot = m_dog->getRotY();
    pos[0] += sinf(rot) * lead;
    pos[2] += cosf(rot) * lead;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00200988
int TskEnemyMove::plan(const float *pos)
{
    ROSEPAN_STUB("TskEnemyMove::plan", 0x00200988);
    return {};
}

// stub of ROR 0x00200d18
void TskEnemyMove::step(const float *pos)
{
    ROSEPAN_STUB("TskEnemyMove::step", 0x00200d18);
}
