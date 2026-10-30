/* ==========================================================================
 *  task/tsk_attack.cpp
 *
 *  The attack and damage steps (pinned 0x1fd424..0x20067c, between
 *  task/tsk_bg_load.cpp and task/tsk_enemy.cpp; RoR has no source string
 *  for it): the member-function pointers TskAttack::setup and
 *  TskDamage::setup (task/tsk_scene.cpp) store, the damage's fall-spot
 *  pickers, and the unreferenced body-part cut (cutOffPart, its weapon
 *  drop and its spray follow) with the unreferenced TskAttack::stepReach.
 *  The classes are declared in task/tsk_scene.h.
 * ======================================================================== */
#include "task/tsk_scene.h"
#include "common/rosepan_stub.h"

#include <math.h>
#include <string.h>

#include "dog/dogactor.h"
#include "eff/effman.h"
#include "emy/emy_main.h"
#include "emy/emy_manager.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "task/tsk_chr.h"
#include "utl/utlrand.h"

/* chrMtnMove2's speed in every step (0x3c888888, 1/60). */
#define ATK_SPEED 0x1.11111p-6f

/* The PC's and the dog's oids, 0 without one (GamMan::m_pcPtr /
 * m_dogPtr, bss 0x7a3350 / 0x7a3354). */
static u_int atkPCOid()
{
    return g_gamMan.m_pcPtr != NULL ? g_gamMan.m_pcPtr->m_oid : 0;
}

static u_int atkDogOid()
{
    return g_gamMan.m_dogPtr != NULL ? g_gamMan.m_dogPtr->m_oid : 0;
}

/* PC: the hit test stepReach, stepKind12 and stepKind26 inline: oid's
 * nodes in reach of the PC (unless its state class is 9) and of the dog
 * (unless Brown's state is 10 or 13) start a chr 11 request on each; 1
 * when one hit. */
static int atkHitPCAndDog(float reach, u_int oid, int nodes, u_int self, int word)
{
    int hit = 0;
    u_int target = atkPCOid();

    if (g_magSystem.getPCStateClass() != 9 && emyChkHitReach(reach, oid, nodes, target) != 0)
    {
        hit = 1;
        scnChrRequest(11, self, 0, 0, word, target, 0);
    }
    target = atkDogOid();
    if (g_magSystem.getBwnState() != 10 && g_magSystem.getBwnState() != 13 &&
        emyChkHitReach(reach, oid, nodes, target) != 0)
    {
        hit = 1;
        scnChrRequest(11, self, 0, 0, word, target, 0);
    }
    return hit;
}

/* PC: the held object's hit parameters stepKind12 / stepKind26 read: the
 * object param 0x67 of oid, and its kind's (param 0x66) system fields
 * 0x22 (the node count) and 0x30 (the range). */
static void atkHeldReach(u_int oid, int *held, u_short *nodes, float *reach)
{
    u_short kind;

    g_gamMan.getObjParam(oid, 0x66, &kind);
    g_gamMan.getObjParam(oid, 0x67, held);
    g_gamMan.m_sysData.getField(kind, 0x22, nodes);
    g_gamMan.m_sysData.getField(kind, 0x30, reach);
}

// ══════════════════════════════════════════════════════════════════════
//  TskAttack
// ══════════════════════════════════════════════════════════════════════

// ──────────────────────────────────────────────────────────────────────
// Kind 0x12's frame, the reach tested from the achr's own first node.
// The heading passed after the speed (f13) is not read by chrMtnMove2.
// callcheck: emyChkHitReach, getBwnState, getPCStateClass, scnChrRequest (in the inline atkHitPCAndDog)
// ROR 0x001fd424

int TskAttack::stepReach()
{
    DogActor &actor = g_magSystem.m_scene.m_actors[m_actor];

    switch (m_phase)
    {
    case 0:
        emyPlaySE(m_oid, 0xb);
        actor.getRotY();
        chrMtnMove2(ATK_SPEED, m_oid, m_motion, -1, 0, NULL);
        m_phase = 1;
        m_frame = 0;
        break;
    case 1:
        if (m_hitFrom <= m_frame && m_frame <= m_hitTo)
        {
            if (atkHitPCAndDog(m_reach, m_oid, 1, m_oid, m_kind))
                emyPlaySE(m_oid, 1);
        }
        if (m_frame == m_hitTo)
            m_phase = 3;
        break;
    case 2:
        if (m_frame == 30)
            return 1;
        break;
    case 3:
        if (g_gamMan.isAchrMotionEnd(m_oid))
            return 1;
        break;
    case 999:
        return 1;
    }
    m_frame++;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The swing: the held object hits within the window (sound 3 on a hit).
// callcheck: emyChkHitReach, getBwnState, getPCStateClass, scnChrRequest (in the inline atkHitPCAndDog)
// callcheck: getObjParam, getField (in the inline atkHeldReach)
// ROR 0x001fd6bc

int TskAttack::stepKind12()
{
    DogActor &actor = g_magSystem.m_scene.m_actors[m_actor];

    switch (m_phase)
    {
    case 0:
        emyPlaySE(m_oid, 0xb);
        actor.getRotY();                /* f13, not read */
        chrMtnMove2(ATK_SPEED, m_oid, m_motion, -1, 0, NULL);
        m_frame = 0;
        m_phase++;
        break;
    case 1:
        if (m_hitFrom <= m_frame && m_frame <= m_hitTo)
        {
            int held;
            u_short nodes;
            float reach;

            atkHeldReach(m_oid, &held, &nodes, &reach);
            if (atkHitPCAndDog(reach, (u_int)held, nodes, m_oid, m_kind))
                emyPlaySE(m_oid, 3);
        }
        if (m_frame == m_hitTo)
            m_phase = 0x14;
        break;
    case 10:
        if (m_frame == 30)
            return 1;
        break;
    case 0x14:
        if (g_gamMan.isAchrMotionEnd(m_oid))
            return 1;
        break;
    case 999:
        return 1;
    }
    m_frame++;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The throw: at m_hitFrom the held object leaves the hand node p_hn_r with
// the thrower's rotation, plays its motion 1 and flies (TskMovePhysic).
// callcheck: dbgAssertFailed, i3rGetNodeIndexByName (in the inline getNodeIndexByName)
// callcheck: sinf, cosf, sqrtf, memset (the dropped velocity, see below)
// ROR 0x001fd9bc

int TskAttack::stepKind15()
{
    MagScene &scene = g_magSystem.m_scene;
    DogActor &actor = scene.m_actors[m_actor];

    switch (m_phase)
    {
    case 0:
        emyPlaySE(m_oid, 0xb);
        actor.getRotY();                /* f13, not read */
        chrMtnMove2(ATK_SPEED, m_oid, m_motion, -1, 0, NULL);
        m_frame = 0;
        m_phase++;
        break;
    case 1:
    {
        if (m_frame < m_hitFrom)
            break;

        int no = g_gamMan.getActorNo((u_int)m_throwOid);
        float rot[4];
        sceVu0FMATRIX mtx;

        g_magSystem.setActorParent(no, -1, 0);
        g_gamMan.getObjRot(m_oid, rot);
        DogActor &self = scene.m_actors[m_actor];
        int node = self.getNodeIndexByName("p_hn_r");  /* 305 */
        self.getNodeWorldMatrix(mtx, node);
        float pos[4] = { mtx[3][0] + 0.0f, mtx[3][1] + 0.0f, mtx[3][2] + 0.0f, 0.0f };
        g_gamMan.getObjRot(m_oid, rot);
        g_gamMan.setObjPos((u_int)m_throwOid, pos);
        /* The ROM builds a velocity here, (sin, cos(m_54) * horizontal
         * length, cos) of rot y times the speed word m_58, w 1, for the
         * TskMovePhysic constructor, which reads only the oid. */
        g_gamMan.setObjRot((u_int)m_throwOid, rot);
        scene.changeActorMotion(0x1.11112ep-6f, no, 1, 0, 0);
        // callcheck: operator_new (the new expression)
        TskMovePhysic *task = new TskMovePhysic((u_int)m_throwOid);
        g_taskMan.addTask(1, -1, 1, 0, task, 0, NULL, 1);
        m_phase = 0x14;
        break;
    }
    case 10:
        if (m_frame == 30)
            return 1;
        break;
    case 0x14:
        if (g_gamMan.isAchrMotionEnd(m_oid))
            return 1;
        break;
    case 999:
        return 1;
    }
    m_frame++;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kinds 0x26..0x28: the held object hits within the window; every frame
// of it plays a swing sound (0x10 on a hit, else 0xf).
// callcheck: emyChkHitReach, getBwnState, getPCStateClass, scnChrRequest (in the inline atkHitPCAndDog)
// callcheck: getObjParam, getField (in the inline atkHeldReach)
// ROR 0x001ffce0

int TskAttack::stepKind26()
{
    DogActor &actor = g_magSystem.m_scene.m_actors[m_actor];

    switch (m_phase)
    {
    case 0:
        emyPlaySE(m_oid, 0x13);
        actor.getRotY();                /* f13, not read */
        chrMtnMove2(ATK_SPEED, m_oid, m_motion, -1, 0, NULL);
        m_frame = 0;
        m_phase++;
        break;
    case 1:
        if (m_hitFrom <= m_frame && m_frame <= m_hitTo)
        {
            int held;
            u_short nodes;
            float reach;

            atkHeldReach(m_oid, &held, &nodes, &reach);
            int hit = atkHitPCAndDog(reach, (u_int)held, nodes, m_oid, m_kind);
            emyPlaySE(m_oid, hit ? 0x10 : 0xf);
        }
        if (m_frame == m_hitTo)
            m_phase = 0x14;
        break;
    case 0x14:
        if (g_gamMan.isAchrMotionEnd(m_oid))
            return 1;
        break;
    case 999:
        return 1;
    }
    m_frame++;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 0x2e: from m_hitFrom to m_hitTo, the nodes b_fg_l / b_fg_r within
// m_reach of m_victim hit it once; the task ends with the motion. The
// heading passed in f13 (the actor's m_rotate y) is not read.
// ROR 0x001ff494

int TskAttack::stepKind2e()
{
    switch (m_phase)
    {
    case 0:
        chrMtnMove2(ATK_SPEED, m_oid, m_motion, -1, 0, NULL);
        m_frame = 0;
        m_phase = 1;
        break;
    case 1:
        if (m_frame >= m_hitFrom)
            m_phase = 2;
        break;
    case 2:
        if (!g_gamMan.isAchrMotionEnd(m_oid))
        {
            if (m_frame > m_hitTo)
                break;
            if (!chrIsNearNode(m_reach, m_oid, (u_int)m_victim, "b_fg_l") &&
                !chrIsNearNode(m_reach, m_oid, (u_int)m_victim, "b_fg_r"))
                break;
            scnChrRequest(11, m_oid, 0, 0, m_kind, (u_int)m_victim, 0);
        }
        m_phase = 4;
        break;
    case 3:
        if (m_frame == 30)
            return 1;
        break;
    case 4:
        if (g_gamMan.isAchrMotionEnd(m_oid))
            return 1;
        break;
    case 999:
        return 1;
    }
    m_frame++;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 0x2f: as 0x2e with the nodes p_gero / p_gero1, hitting every
// frame of the window until the motion ends.
// ROR 0x001ff664

int TskAttack::stepKind2f()
{
    switch (m_phase)
    {
    case 0:
        chrMtnMove2(ATK_SPEED, m_oid, m_motion, -1, 0, NULL);
        m_frame = 0;
        m_phase = 1;
        break;
    case 1:
        if (m_frame >= m_hitFrom)
            m_phase = 2;
        break;
    case 2:
        if (g_gamMan.isAchrMotionEnd(m_oid))
        {
            m_phase = 4;
            break;
        }
        if (m_frame <= m_hitTo &&
            (chrIsNearNode(m_reach, m_oid, (u_int)m_victim, "p_gero") ||
             chrIsNearNode(m_reach, m_oid, (u_int)m_victim, "p_gero1")))
            scnChrRequest(11, m_oid, 0, 0, m_kind, (u_int)m_victim, 0);
        break;
    case 3:
        if (m_frame == 30)
            return 1;
        break;
    case 4:
        if (g_gamMan.isAchrMotionEnd(m_oid))
            return 1;
        break;
    case 999:
        return 1;
    }
    m_frame++;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 0x30: the same code as stepKind2f (only its frames differ, set by
// setup).
// ROR 0x001ff830

int TskAttack::stepKind30()
{
    switch (m_phase)
    {
    case 0:
        chrMtnMove2(ATK_SPEED, m_oid, m_motion, -1, 0, NULL);
        m_frame = 0;
        m_phase = 1;
        break;
    case 1:
        if (m_frame >= m_hitFrom)
            m_phase = 2;
        break;
    case 2:
        if (g_gamMan.isAchrMotionEnd(m_oid))
        {
            m_phase = 4;
            break;
        }
        if (m_frame <= m_hitTo &&
            (chrIsNearNode(m_reach, m_oid, (u_int)m_victim, "p_gero") ||
             chrIsNearNode(m_reach, m_oid, (u_int)m_victim, "p_gero1")))
            scnChrRequest(11, m_oid, 0, 0, m_kind, (u_int)m_victim, 0);
        break;
    case 3:
        if (m_frame == 30)
            return 1;
        break;
    case 4:
        if (g_gamMan.isAchrMotionEnd(m_oid))
            return 1;
        break;
    case 999:
        return 1;
    }
    m_frame++;
    return 0;
}

// ══════════════════════════════════════════════════════════════════════
//  TskDamage
// ══════════════════════════════════════════════════════════════════════

/* The fall spots: angles off the heading, distances, the states they
 * give (data 0x2ff230 / 0x2ff240 / 0x2ff250 behind, 0x2ff260 / 0x2ff270 /
 * 0x2ff280 in front). */
static const float s_fallBackAngle[3]  = { 0x1.921fb4p+1f, -0x1.921fb4p+0f, 0x1.921fb4p+0f };
static const float s_fallBackDist[3]   = { 0x1.e50c48p+5f, 0x1.8cdb22p+5f, 0x1.8cdb22p+5f };
static const int   s_fallBackState[3]  = { 5001, 5002, 5003 };
static const float s_fallFrontAngle[3] = { 0.0f, 0x1.921fb4p+0f, -0x1.921fb4p+0f };
static const float s_fallFrontDist[3]  = { 0x1.5a53f6p+5f, 0x1.8cdb22p+5f, 0x1.8cdb22p+5f };
static const int   s_fallFrontState[3] = { 5000, 5003, 5002 };

/* PC: the loop pickFallBack and pickFallFront each inline: the first spot
 * (dist along heading - angle, y 0) with a cell and no hit for oid. */
static int atkPickFall(TskDamage *task, const float *angle, const float *dist, const int *state)
{
    const double pi = 3.141592653589793, twoPi = 6.283185307179586;
    float rotY = g_magSystem.m_scene.m_actors[task->m_actor].getRotY();
    float pos[4];

    task->m_achr->ObjStat::getPos(pos);
    for (int i = 0; i < 3; i++)
    {
        float a = rotY - angle[i];
        if ((double)a < -pi)
            a = (float)((double)a + twoPi);
        if ((double)a > pi)
            a = (float)((double)a - twoPi);

        float at[4];
        at[0] = pos[0] + sinf(a) * dist[i];
        at[1] = 0.0f;
        at[2] = pos[2] + cosf(a) * dist[i];
        at[3] = 0.0f;
        if (g_gamMan.pickSectaCell(at, 0) != NULL &&
            g_gamMan.testHitAt(at, task->m_oid, 0, 0, NULL) == 0)
            return state[i];
    }
    task->m_flags |= 8;
    return task->m_frames;
}

// ──────────────────────────────────────────────────────────────────────
// Behind: pi, -pi/2, pi/2 at 60.6 / 49.6 / 49.6 -> 5001 / 5002 / 5003.
// callcheck: getRotY, getPos, sinf, cosf, pickSectaCell, testHitAt (in the inline atkPickFall)
// ROR 0x001fddb8 // callcheck: fptodp, dpcmp, dpadd, dptofp, dpsub (the soft-float doubles, C doubles here)

int TskDamage::pickFallBack()
{
    return atkPickFall(this, s_fallBackAngle, s_fallBackDist, s_fallBackState);
}

// ──────────────────────────────────────────────────────────────────────
// In front: 0, pi/2, -pi/2 at 43.3 / 49.6 / 49.6 -> 5000 / 5003 / 5002.
// callcheck: getRotY, getPos, sinf, cosf, pickSectaCell, testHitAt (in the inline atkPickFall)
// ROR 0x001fdf8c // callcheck: fptodp, dpcmp, dpadd, dptofp, dpsub (the soft-float doubles, C doubles here)

int TskDamage::pickFallFront()
{
    return atkPickFall(this, s_fallFrontAngle, s_fallFrontDist, s_fallFrontState);
}

/* PC: the voice every chain plays through scnPlaySEDelayed (now). */
static void atkVoice(u_int oid, int kind)
{
    scnPlaySEDelayed(g_emyManager.getEmyVoiceSE(oid, kind), 0);
}

/* PC: the down helper chr 0's states 21 / 31 / 60 / 70 inline: half the
 * time (with achr attribute 0x400000) the enemy's out is reset, then it
 * is marked down. */
static void atkDown(TskDamage *task)
{
    if ((task->m_achr->m_attr[0] & 0x400000) != 0 && utlRandRange(0, 99) < 0x32)
        g_emyManager.resetEmyOut(task->m_oid);
    g_emyManager.setEmyDownFlg(task->m_oid, 1);
}

// ──────────────────────────────────────────────────────────────────────
// chr 0 (the heavy hit): a stagger (0 behind / 10 in front) that may fall
// to a free spot (5000..5003), the down chains 20..23 / 30..33, the
// front / back hits 60 / 70, and the killed chains (10000 on). Without a
// motion task the heading is the achr's own, else 32769 (keep); either
// goes in f13, which chrMtnMove2 does not read.
// callcheck: getEmyVoiceSE, scnPlaySEDelayed (in the inline atkVoice)
// callcheck: resetEmyOut (in the inline atkDown)
// ROR 0x001fe160

int TskDamage::stepKind0()
{
    if (!g_gamMan.isAchrMotionEnd(m_oid))
    {
        if ((m_flags & 4) == 0)
        {
            m_frame++;
            return 0;
        }
    }
    else if (endReact())
    {
        return 1;
    }

    switch (m_frames)
    {
    case 0:
    {
        m_motion = 0x27;
        atkVoice(m_oid, 3);
        u_int attr = m_achr->m_attr[0];
        int pct = (attr & 0x800) != 0 ? 0x50 : (attr & 0x400) != 0 ? 0x28 : 10;
        if (utlRandRange(0, 99) < pct)
        {
            m_frames = pickFallBack() - 1;
            break;
        }
        m_flags |= 8;
        break;
    }
    case 10:
        m_motion = 0x27;
        atkVoice(m_oid, 3);
        if (utlRandRange(0, 99) < 10)
        {
            m_frames = pickFallFront() - 1;
            break;
        }
        m_flags |= 8;
        break;
    case 0x14:
        m_motion = 0x29;
        atkVoice(m_oid, 5);
        break;
    case 0x15:
        atkDown(this);
        m_achr->m_flags &= ~0x400u;
        m_motion = 0x2b;
        break;
    case 0x16:
        m_motion = utlRandRange(0, 2) + 0x2b;
        break;
    case 0x17:
        g_emyManager.setEmyDownFlg(m_oid, 0);
        m_achr->m_flags |= 0x400;
        m_motion = 0x2a;
        m_flags |= 8;
        break;
    case 0x1e:
        m_motion = 0x2f;
        atkVoice(m_oid, 5);
        break;
    case 0x1f:
        atkDown(this);
        m_achr->m_flags &= ~0x400u;
        m_motion = 0x31;
        break;
    case 0x20:
        m_motion = utlRandRange(0, 2) + 0x31;
        break;
    case 0x21:
        g_emyManager.setEmyDownFlg(m_oid, 0);
        m_achr->m_flags |= 0x400;
        m_motion = 0x30;
        m_flags |= 8;
        break;
    case 0x3c:
        atkDown(this);
        m_motion = 0x2e;
        atkVoice(m_oid, 4);
        break;
    case 0x3d:
        g_emyManager.setEmyDownFlg(m_oid, 0);
        m_motion = 0x30;
        m_flags |= 8;
        break;
    case 0x46:
        atkDown(this);
        m_motion = 0x28;
        atkVoice(m_oid, 4);
        break;
    case 0x47:
        g_emyManager.setEmyDownFlg(m_oid, 0);
        m_motion = 0x2a;
        m_flags |= 8;
        break;
    case 0x50:
        m_motion = 0x2e;
        m_flags |= 8;
        atkVoice(m_oid, 4);
        break;
    case 5000:
        m_motion = 0x49;
        m_flags |= 8;
        break;
    case 5001:
        m_motion = 6;                   /* the ROM's (li v1, 0x6 at 0x1fe450) */
        m_flags |= 8;
        break;
    case 5002:
        m_motion = 0x4a;
        m_flags |= 8;
        break;
    case 5003:
        m_motion = 0x4b;
        m_flags |= 8;
        break;
    case 10000:
    case 10010:
        m_motion = 0x27;
        atkVoice(m_oid, 6);
        break;
    case 10001:
        m_motion = 0x29;
        m_flags |= 8;
        break;
    case 10011:
        m_motion = 0x2f;
        m_flags |= 8;
        break;
    case 10020:
        m_motion = 0x29;
        m_flags |= 8;
        atkVoice(m_oid, 6);
        break;
    case 10030:
        m_motion = 0x2f;
        m_flags |= 8;
        atkVoice(m_oid, 6);
        break;
    case 10060:
    case 10080:
        m_motion = 0x2e;
        m_flags |= 8;
        atkVoice(m_oid, 6);
        break;
    case 10070:
        m_motion = 0x28;
        m_flags |= 8;
        atkVoice(m_oid, 6);
        break;
    default:
        break;
    }

    m_frames++;
    float rot[4] = {};
    if (m_achr->m_task[2] == -1)
        m_achr->ObjStat::getRot(rot);
    else
        rot[1] = 32769.0f;
    (void)rot[1];                       /* f13: not read by chrMtnMove2 */
    chrMtnMove2(ATK_SPEED, m_oid, m_motion, -1, 0, NULL);
    m_flags &= ~4u;
    m_frame++;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The held weapon (param 0x67) falls from the hand node p_hn_r when the
// cut part is kind 0xbc3, the achr is killed (flags 0x100) or has lost
// both legs (attributes 8 and 0x10), or a type-2 enemy holding the
// scissors (kind 0xa2e) loses part 0xbc3 / 0xbc1. Its params 0x66 / 0x67
// become 0xa00 / 0. The velocity (2, -15, 2) the ROM builds is not read
// by the TskMovePhysic constructor.
// callcheck: dbgAssertFailed, i3rGetNodeIndexByName (in the inline getNodeIndexByName)
// ROR 0x001fe87c

static void scnDropWeapon(ACHRstat *achr, u_int oid, int no, short item, u_int flags)
{
    int held;

    achr->getParam(0x67, &held);
    if (held == 0)
        return;

    u_int attr = achr->m_attr[0];
    int drop = (flags & 0x100) != 0 || ((attr & 8) != 0 && (attr & 0x10) != 0);
    if (g_emyManager.getEmyType(oid) == 2)
    {
        short kind = g_gamMan.getObjKind((u_int)held);
        if (kind == 0xa2e && (item == 0xbc3 || item == 0xbc1))
            drop = 1;
    }
    if (item != 0xbc3 && !drop)
        return;

    DogActor &actor = g_magSystem.m_scene.m_actors[no];
    sceVu0FMATRIX mtx;
    int node = actor.getNodeIndexByName("p_hn_r");  /* 305 */
    actor.getNodeWorldMatrix(mtx, node);
    float pos[4] = { mtx[3][0] + 0.0f, mtx[3][1] + 0.0f, mtx[3][2] + 0.0f, 0.0f };
    g_gamMan.setObjPos((u_int)held, pos);
    g_gamMan.resetObj((u_int)held, 0x4000000);
    g_magSystem.setActorParent(g_gamMan.getActorNo((u_int)held), -1, 0);
    // callcheck: operator_new (the new expression)
    TskMovePhysic *task = new TskMovePhysic((u_int)held);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, NULL, 1);

    u_short none = 0xa00;
    achr->setParam(0x66, &none);
    held = 0;
    achr->setParam(0x67, &held);
}

/* The body parts' item kinds by enemy type 0 / 1 / 2 (getEmyType; others
 * row 0): neck, right / left upper arm, right / left thigh (data 0x2ff2a0,
 * words). */
static const short s_partItem[3][5] = {
    { 0xbbf, 0xbc3, 0xbc1, 0xbc2, 0xbc0 },
    { 0xbd3, 0xbd7, 0xbd5, 0xbd6, 0xbd4 },
    { 0xbce, 0xbd2, 0xbd0, 0xbd1, 0xbcf },
};

/* (0, -1, 0): the kind-4 trail's up (data 0x2ff290). */
static const float s_down[4] = { 0.0f, -1.0f, 0.0f, 0.0f };

// ──────────────────────────────────────────────────────────────────────
// part 1 neck, 2 right / 4 left upper arm, 8 right / 0x10 left thigh: the
// node is scaled to 0, a blood spray (kind 2, 45 frames) follows
// m_cutPos / m_cutDir, and the part's item object is created at the node
// and thrown. With both legs gone the ROM fills a kind-4 trail from node
// Bip01 that it never creates. Unreferenced.
// callcheck: dbgAssertFailed, i3rGetNodeIndexByName (in the inline getNodeIndexByName)
// ROR 0x001feb38

void TskDamage::cutOffPart(ACHRstat *achr, int part)
{
    MagScene &scene = g_magSystem.m_scene;
    int no = achr->m_actor;
    DogActor &actor = scene.m_actors[no];
    int type = g_emyManager.getEmyType(m_oid);
    int row = type == 1 ? 1 : type == 2 ? 2 : 0;
    int node = -1;
    short item = 0;

    switch (part - 1)
    {
    case 0:
        node = actor.getNodeIndexByName("Bip01 Neck");  /* 305 */
        item = s_partItem[row][0];
        break;
    case 1:
        node = actor.getNodeIndexByName("Bip01 R UpperArm");  /* 305 */
        item = s_partItem[row][1];
        break;
    case 3:
        node = actor.getNodeIndexByName("Bip01 L UpperArm");  /* 305 */
        item = s_partItem[row][2];
        break;
    case 7:
        node = actor.getNodeIndexByName("Bip01 R Thigh");  /* 305 */
        item = s_partItem[row][3];
        break;
    case 15:
        node = actor.getNodeIndexByName("Bip01 L Thigh");  /* 305 */
        item = s_partItem[row][4];
        break;
    }

    float pos[4], rot[4];
    achr->ObjStat::getPos(pos);
    achr->ObjStat::getRot(rot);
    scnDropWeapon(achr, m_oid, no, item, m_flags);
    m_cutActor = &actor;
    switch (part - 1)
    {
    case 7:
        node = actor.getNodeIndexByName("Bip01 R Thigh");  /* 305 */
        break;
    case 15:
        node = actor.getNodeIndexByName("Bip01 L Thigh");  /* 305 */
        break;
    }
    m_cutNode = node;

    EffParam eff;
    memset(&eff, 0, sizeof(eff));       /* PC: zeroed (the ROM sets the words below) */
    eff.m_kind = 2;
    eff._04 = 4;
    eff.m_08 = 0;
    eff.m_spray2.m_pos = m_cutPos;
    eff.m_spray2.m_dir = m_cutDir;
    eff.m_spray2.m_scale = 1.0f;
    eff.m_spray2.m_frames = 0x2d;
    scene.m_eff_man.createEff(&eff);

    sceVu0FMATRIX mtx;
    u_int attr = achr->m_attr[0];
    if ((attr & 8) != 0 && (attr & 0x10) != 0 && (part == 8 || part == 0x10))
    {
        /* The trail block (kind 4, word 4 = 4: m_pos the Bip01 node's
         * position, m_up s_down, width 5..15, length 280..350) is filled
         * and dropped: no createEff follows in the ROM. */
        int root = actor.getNodeIndexByName("Bip01");  /* 305 */
        actor.getNodeWorldMatrix(mtx, root);
        (void)s_down;
        utlRandRangeF(5.0f, 15.0f);
        utlRandRangeF(280.0f, 350.0f);
    }

    actor.setNodeScale(0.0f, node);
    actor.getNodeWorldMatrix(mtx, node);
    pos[0] = mtx[3][0] + 0.0f;
    pos[1] = mtx[3][1] + 0.0f;
    pos[2] = mtx[3][2] + 0.0f;
    u_int obj = chrCreateItemObj(item, pos, rot);
    /* The velocity (1, -10, 1) the ROM copies from data 0x2ff190 is not
     * read by the TskMovePhysic constructor. */
    // callcheck: operator_new (the new expression)
    TskMovePhysic *task = new TskMovePhysic(obj);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, NULL, 1);
}

// ──────────────────────────────────────────────────────────────────────
// The cut node's world position and z row for the spray. Unreferenced.
// ROR 0x001ff25c

void TskDamage::updateCutSpray()
{
    sceVu0FMATRIX mtx;

    m_cutActor->getNodeWorldMatrix(mtx, m_cutNode);
    m_cutPos[0] = mtx[3][0];
    m_cutDir[3] = mtx[2][3];
    m_cutPos[1] = mtx[3][1];
    m_cutPos[2] = mtx[3][2];
    m_cutPos[3] = mtx[3][3];
    m_cutDir[0] = mtx[2][0];
    m_cutDir[1] = mtx[2][1];
    m_cutDir[2] = mtx[2][2];
}

/* PC: every step's head and tail below (the ROM inlines them). The head:
 * 1 when the task ends now, -1 to wait a frame (m_frame counted), 0 to
 * pick the next motion. */
static int atkReactHead(TskDamage *task)
{
    if (!g_gamMan.isAchrMotionEnd(task->m_oid))
    {
        if ((task->m_flags & 4) == 0)
        {
            task->m_frame++;
            return -1;
        }
        return 0;
    }
    return task->endReact() ? 1 : 0;
}

/* The tail: the next state, the motion (32769: keep the heading, in f13,
 * not read by chrMtnMove2). */
static int atkReactTail(TskDamage *task)
{
    task->m_frames++;
    chrMtnMove2(ATK_SPEED, task->m_oid, task->m_motion, -1, 0, NULL);
    task->m_flags &= ~4u;
    task->m_frame++;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// chr 1.
// callcheck: isAchrMotionEnd, endReact, chrMtnMove2 (in the inline atkReactHead / atkReactTail)
// ROR 0x001ff2e0

int TskDamage::stepKind1()
{
    int head = atkReactHead(this);
    if (head != 0)
        return head > 0 ? 1 : 0;

    switch (m_frames)
    {
    case 0:
    case 0x14:
    case 10000:
    case 10020:
        m_motion = 0x20;
        m_flags |= 8;
        break;
    case 10:
    case 0x1e:
    case 10010:
    case 10030:
        m_motion = 0x1d;
        m_flags |= 8;
        break;
    case 0x3c:
    case 10060:
        m_motion = 0x1f;
        m_flags |= 8;
        break;
    case 0x46:
    case 10070:
        m_motion = 0x1c;
        m_flags |= 8;
        break;
    }
    return atkReactTail(this);
}

// ──────────────────────────────────────────────────────────────────────
// chr 2.
// callcheck: isAchrMotionEnd, endReact, chrMtnMove2 (in the inline atkReactHead / atkReactTail)
// ROR 0x001ffbc0

int TskDamage::stepKind2()
{
    int head = atkReactHead(this);
    if (head != 0)
        return head > 0 ? 1 : 0;

    switch (m_frames)
    {
    case 0:
    case 10:
        m_motion = 0x19;
        m_flags |= 8;
        break;
    case 0x14:
        m_motion = 0x1c;
        m_flags |= 8;
        break;
    case 0x1e:
        m_motion = 0x1f;
        m_flags |= 8;
        break;
    }
    return atkReactTail(this);
}

// ──────────────────────────────────────────────────────────────────────
// chr 5.
// callcheck: isAchrMotionEnd, endReact, chrMtnMove2 (in the inline atkReactHead / atkReactTail)
// ROR 0x00200444

int TskDamage::stepKind5()
{
    int head = atkReactHead(this);
    if (head != 0)
        return head > 0 ? 1 : 0;

    switch (m_frames)
    {
    case 0:
    case 10:
        m_motion = 0x19;
        m_flags |= 8;
        break;
    case 0x14:
        m_motion = 0x1e;
        m_flags |= 8;
        break;
    case 0x1e:
        m_motion = 0x1f;
        m_flags |= 8;
        break;
    case 10000:
    case 10020:
        m_motion = 0x1a;
        m_flags |= 8;
        break;
    case 10010:
    case 10030:
        m_motion = 0x1b;
        m_flags |= 8;
        break;
    }
    return atkReactTail(this);
}

// ──────────────────────────────────────────────────────────────────────
// chr 6: motion 0x19 once more (p1d8 1), the state left as it is.
// callcheck: isAchrMotionEnd, endReact (in the inline atkReactHead)
// ROR 0x002005b8

int TskDamage::stepKind6()
{
    int head = atkReactHead(this);
    if (head != 0)
        return head > 0 ? 1 : 0;

    m_motion = 0x19;
    m_flags |= 8;
    chrMtnMove2(ATK_SPEED, m_oid, 0x19, 1, 0, NULL);
    m_flags &= ~4u;
    m_frame++;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// chr 7: on its end the enemy's free count drops by 8..16.
// callcheck: chrMtnMove2 (in the inline atkReactTail)
// ROR 0x001fffc0

int TskDamage::stepKind7()
{
    if (!g_gamMan.isAchrMotionEnd(m_oid))
    {
        if ((m_flags & 4) == 0)
        {
            m_frame++;
            return 0;
        }
    }
    else if (endReact())
    {
        g_emyManager.subEmyFreeCnt(m_oid, utlRandRange(8, 0x10));
        return 1;
    }

    switch (m_frames)
    {
    case 0:
    case 10:
        m_motion = 0x13;
        m_flags |= 8;
        break;
    case 0x14:
        m_motion = 0x12;
        m_flags |= 8;
        break;
    case 0x1e:
        m_motion = 0xf;
        m_flags |= 8;
        break;
    case 10000:
    case 10010:
    case 10020:
    case 10030:
        m_motion = 0x31;
        m_flags |= 8;
        break;
    }
    return atkReactTail(this);
}

// ──────────────────────────────────────────────────────────────────────
// chr 8.
// callcheck: isAchrMotionEnd, endReact, chrMtnMove2 (in the inline atkReactHead / atkReactTail)
// ROR 0x0020014c

int TskDamage::stepKind8()
{
    int head = atkReactHead(this);
    if (head != 0)
        return head > 0 ? 1 : 0;

    switch (m_frames)
    {
    case 0:
    case 10:
        m_motion = 0x13;
        m_flags |= 8;
        break;
    case 0x14:
    case 10000:
    case 10020:
        m_motion = 0x12;
        m_flags |= 8;
        break;
    case 0x1e:
    case 10010:
    case 10030:
        m_motion = 0x11;
        m_flags |= 8;
        break;
    case 0x46:
        m_motion = 0x15;
        m_flags |= 8;
        break;
    }
    return atkReactTail(this);
}

// ──────────────────────────────────────────────────────────────────────
// chr 9.
// callcheck: isAchrMotionEnd, endReact, chrMtnMove2 (in the inline atkReactHead / atkReactTail)
// ROR 0x001ff9fc

int TskDamage::stepKind9()
{
    int head = atkReactHead(this);
    if (head != 0)
        return head > 0 ? 1 : 0;

    switch (m_frames)
    {
    case 0:
        m_motion = 8;
        m_flags |= 8;
        break;
    case 10:
        m_motion = 7;
        m_flags |= 8;
        break;
    case 0x14:
    case 10000:
    case 10020:
        m_motion = 0xf;
        m_flags |= 8;
        break;
    case 0x1e:
    case 10010:
    case 10030:
        m_motion = 0xe;
        m_flags |= 8;
        break;
    case 0x28:
    case 10040:
        m_motion = 0x10;
        m_flags |= 8;
        break;
    case 0x32:
    case 10050:
        m_motion = 0x11;
        m_flags |= 8;
        break;
    }
    return atkReactTail(this);
}

// ──────────────────────────────────────────────────────────────────────
// chr 10.
// callcheck: isAchrMotionEnd, endReact, chrMtnMove2 (in the inline atkReactHead / atkReactTail)
// ROR 0x002002cc

int TskDamage::stepKindA()
{
    int head = atkReactHead(this);
    if (head != 0)
        return head > 0 ? 1 : 0;

    switch (m_frames)
    {
    case 0:
        m_motion = 0xe;
        m_flags |= 8;
        break;
    case 10:
        m_motion = 0xd;
        m_flags |= 8;
        break;
    case 0x14:
    case 10000:
    case 10020:
        m_motion = 0x15;
        m_flags |= 8;
        break;
    case 0x1e:
    case 10010:
    case 10030:
        m_motion = 0x16;
        m_flags |= 8;
        break;
    }
    return atkReactTail(this);
}
