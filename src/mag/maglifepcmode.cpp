/* ==========================================================================
 *  mag/maglifepcmode.cpp
 *
 *  The PC's modes (0x120788..0x1233d8, pinned): the shared root-motion
 *  step, then each MagLifePCMode*'s init / live / pad in vtable order.
 *  States and their modes: mag/maglifepcmode.h. The motions they start
 *  are rows of the PC's motion sequence table (0x2961e0, the asserts'
 *  m_tbl_motion_state): 1..3 run_out, 5 wait, 6 tired, 7 w_wait,
 *  8 tired_out, 38..40 walk_in, 41..46 walk, 47 run_in, 49 run, 50 / 52 /
 *  54 the 180-degree turns, 116.. / 121.. the ev201 / ev501 event poses,
 *  159..163 free_a..free_e. Health (MagLifePC::calcHealth) picks the
 *  column: 0 normal, 1 tired, 2 the w_ (wounded) set, 3 / 4 the events.
 * ======================================================================== */
#include "mag/maglifepcmode.h"

#include "bed/bedpad.h"
#include "common/ror_assert.h"
#include "common/rosepan_stub.h"
#include "dog/dogactor.h"
#include "lib/unassigned.h"
#include "mag/maglifebwnctrl.h"
#include "mag/maglifepc.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "utl/utlgame.h"
#include "utl/utlmath.h"

/* Wait::init: run_out by health after a run, else the health's wait row. */
static const int s_wait_after_run[3] = { 1, 2, 3 };                     // ROR 0x002ead00
static const int s_wait_motion[5] = { 5, 6, 7, 116, 121 };               // ROR 0x002ead10
/* Walk2D::init by health 0..2: walk, walk_in from a stand, and the walk
 * row without the walk_in lead. */
static const int s_walk_motion[3] = { 41, 42, 43 };                      // ROR 0x002ead68
static const int s_walk_in_motion[3] = { 38, 39, 40 };                   // ROR 0x002ead78
static const int s_walk_cont_motion[3] = { 44, 45, 46 };                 // ROR 0x002ead88
/* Wait::pad at health 3 / 4 (healthsw 1 / 2): the event pose by stick
 * direction (none, left, right, up, down). The ROM indexes rows 1..2 of
 * a five-int table at 0x2973cc; these are those rows. */
static const int s_event_motion[2][5] = {                                // ROR 0x002973e0
    { 116, 117, 118, 119, 120 },        /* ev201_wait, _l, _r, _up, _dwn */
    { 121, 122, 123, 124, 125 },        /* ev501_wait, _l, _r, _up, _dwn */
};

/* AtkWait2D::init by weapon class (+ 4 at health 2) and the target's
 * height class 0..4: the weapon in / wait rows 56..115. */
static const int s_atk_wait_motion[8][5] = {                             // ROR 0x002ead98
    { 0x38, 0x39, 0x3a, 0x3b, 0x3c },
    { 0x42, 0x43, 0x44, 0x45, 0x46 },
    { 0x4c, 0x4d, 0x4e, 0x4f, 0x50 },
    { 0x56, 0x57, 0x58, 0x59, 0x5a },
    { 0x60, 0x61, 0x62, 0x63, 0x64 },
    { 0x65, 0x66, 0x67, 0x68, 0x69 },
    { 0x6a, 0x6b, 0x6c, 0x6d, 0x6e },
    { 0x6f, 0x70, 0x71, 0x72, 0x73 },
};

/* ---- the shared step ---- */

// ROR 0x00120788
void MagLifePCMode::moveBy(MagLifePC *pc, const float *move, float trans_ofs_y, float rot)
{
    pc->moveBy(move, trans_ofs_y, rot);
}

// ROR 0x001207a8
void MagLifePCMode::applyRootMotion(MagLifePC *pc)
{
    DogActor *actor = pc->m_actor;
    float move[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float rot[4];
    float trans_ofs_y = 0.0f;
    rot[0] = 0.0f;
    if (actor->getModeArg() < 0)
    {
        float frame = actor->getMotionFrame();
        if (0.0f <= frame)
        {
            float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            float step = pc->getHealthMotionStep();
            /* the root track keys at 120 Hz */
            actor->getMotionMove(frame + frame, (frame + step) + (frame + step), move, rot, pos);
            trans_ofs_y = pos[1];
        }
    }
    move[3] = 1.0f;
    utlVecRotMatY(actor->m_rotate[1], move);
    moveBy(pc, move, trans_ofs_y, rot[0]);
}

// ROR 0x001208b8
void MagLifePCMode::live(MagLifePC *pc)
{
    applyRootMotion(pc);
    if (pc->isMotionSeqEnd())
        pc->changeState(0, 0, 0);
}

/* ---- Null (state 0): picks the state the PC stands in ---- */

// ROR 0x00120900
void MagLifePCModeNull::init(MagLifePC *pc)
{
    if (!pc->m_actor)
        return;
    int state, arg;
    if (pc->m_mode == 1 || pc->m_mode == 3)
    {
        state = 7;
        arg = 0;
    }
    else if (pc->m_mode_hold.m_04)
    {
        state = 8;
        arg = 0;
    }
    else if (pc->m_mode_ride.m_04)
    {
        state = 9;
        arg = 3;
    }
    else if (pc->m_stance)
    {
        state = 5;
        arg = 0;
    }
    else if (pc->m_prev_state[0] == 14)
    {
        state = 14;
        arg = 0x11;
    }
    else
    {
        pc->changeState(1, 0, 0);
        return;
    }
    pc->changeState(state, arg, 0);
}

// ROR 0x001209b0
void MagLifePCModeNull::live(MagLifePC *pc)
{
    if (pc->m_actor)
        pc->changeState(1, 0, 0);
}

/* ---- Wait (state 1) ---- */

// ROR 0x001209e0
void MagLifePCModeWait::init(MagLifePC *pc)
{
    int health = pc->m_health;
    int motion;
    if (pc->m_move_level_prev == 2)
        motion = s_wait_after_run[health < 3 ? health : 2];
    else
        motion = s_wait_motion[health];
    pc->changeMotionSeq(motion, -1);
    pc->setWaitSubMotion(0);
}

// ROR 0x00120a54
void MagLifePCModeWait::updateIdle(MagLifePC *pc)
{
    int health = pc->m_health;
    if (pc->m_health_prev != health)
    {
        if (pc->m_health_prev == 1 && health == 0)
            pc->changeMotionSeq(8, -1);     /* tired_out */
        else
            pc->changeMotionSeq(0, -1);
    }
    if (health != 0 || (pc->m_mode_frame + 1) % 5400 != 0)
        return;
    u_int pick = utlRandRange(0, 7);
    if (pick >= 5)
        return;
    if (pick != 4)
    {
        pc->changeMotionSeq(pick + 159, -1);    /* free_a..free_d */
        return;
    }
    /* free_e only in room 0x35, from x 1000 on */
    float pos[4];
    pos[0] = 0.0f;
    pos[3] = 1.0f;
    pos[2] = 0.0f;
    pos[1] = 0.0f;
    pc->getPosition(pos);
    if (utlGetWrdNo() == 0x35 && 1000.0f <= pos[0])
    {
        pc->changeMotionSeq(163, -1);
        return;
    }
    pc->changeMotionSeq(utlRandRange(0, 3) + 159, -1);
}

// ROR 0x00120b9c
void MagLifePCModeWait::live(MagLifePC *pc)
{
    pc->recoverStamina();
    applyRootMotion(pc);
}

// ROR 0x00120bdc
int MagLifePCModeWait::pad(MagLifePC *pc, BedPad *thePad)
{
    if (thePad->m_trig & 0x4000)
    {
        pc->pickUpNearItem();
        return 1;
    }
    float roty = pc->getStickRotY();
    int direct = pc->canTurnDirect(pc->m_actor->m_rotate[1], roty);
    int state = 5;
    if (pc->m_stance == 0)
    {
        int health = pc->m_health;
        int healthsw;
        if (health == 3)
            healthsw = 1;
        else if (health == 4)
            healthsw = 2;
        else
            healthsw = 0;
        if (healthsw)
        {
            const float lim = 0x1.333332p-2f;   /* 0.3 */
            int dir = 0;
            if (0.0f < pc->m_stick_mag)
            {
                if (lim < pc->m_stick_x)
                    dir = 2;
                else if (pc->m_stick_x < -lim)
                    dir = 1;
                else if (lim < pc->m_stick_y)
                    dir = 4;
                else if (pc->m_stick_y < -lim)
                    dir = 3;
            }
            ROR_ASSERT(1 <= healthsw && healthsw <= 2);  /* 471 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
            pc->changeMotionSeq(s_event_motion[healthsw - 1][dir], -1);
            return 0;
        }
        if (pc->m_stick_mag <= 0.0f)
        {
            updateIdle(pc);
            return 0;
        }
        if (!direct && health < 2)
            state = 4;
        else if (pc->m_move_level == 1)
            state = 2;
        else if (pc->m_move_level == 2)
            state = 3;
        else
        {
            ROR_ASSERT(0);  /* 500 */
            return 2;
        }
    }
    pc->changeState(state, 0, 0);
    return 2;
}

/* ---- the moving modes ---- */

// ROR 0x00120dec
void MagLifePCModeMove::steer(MagLifePC *pc)
{
    DogActor *actor = pc->m_actor;
    float d = pc->getStickRotY() - actor->m_rotate[1];
    if (d < -DOG_PI)
        d += DOG_PI2;
    else if (DOG_PI <= d)
        d -= DOG_PI2;
    float lim = 0x1.999998p-3f;         /* 0.2 */
    if (pc->m_health == 2)
        lim = 0x1.1eb85p-3f;            /* 0.14 */
    if (d < -lim)
        d = -lim;
    else if (lim < d)
        d = lim;
    float r = actor->m_rotate[1] + d;
    actor->m_rotate[1] = r;
    if (r < -DOG_PI)
        actor->m_rotate[1] = r + DOG_PI2;
    else if (DOG_PI <= r)
        actor->m_rotate[1] = r - DOG_PI2;
    actor->m_dirty = 1;
    actor->addBaseRotY(d);
    applyRootMotion(pc);
}

/* ---- Walk2D (state 2) ---- */

// ROR 0x00120f60
void MagLifePCModeWalk2D::init(MagLifePC *pc)
{
    int health = pc->m_health;
    int prev = pc->m_move_level_prev;
    if (health > 2)
        health = 2;
    if (!pc->isBlendIn() && prev != 2 && pc->m_prev_state[0] != 4)
    {
        if (prev == 0)
            pc->changeMotionSeq(s_walk_in_motion[health], -1);
        else
            pc->changeMotionSeq(s_walk_cont_motion[health], -1);
        return;
    }
    pc->changeMotionSeq(s_walk_motion[health], -1);
}

// ROR 0x00121054
void MagLifePCModeWalk2D::move(MagLifePC *pc, BedPad *thePad)
{
    steer(pc);
}

// ROR 0x00121070
void MagLifePCModeWalk2D::live(MagLifePC *pc)
{
}

// ROR 0x00121078
int MagLifePCModeWalk2D::pad(MagLifePC *pc, BedPad *thePad)
{
    int state;
    if (pc->m_stance != 0)
        state = 5;
    else
    {
        if (pc->m_stick_mag <= 0.0f)
        {
            pc->changeState(1, 0, 0);
            return 0;
        }
        float roty = pc->getStickRotY();
        if (!pc->canTurnDirect(pc->m_actor->m_rotate[1], roty) && pc->m_health < 2)
            state = 4;
        else if (pc->m_move_level != 2)
        {
            move(pc, thePad);
            return 1;
        }
        else
            state = 3;
    }
    pc->changeState(state, 0, 0);
    return 2;
}

/* ---- Run2D (state 3) ---- */

// ROR 0x00121168
void MagLifePCModeRun2D::init(MagLifePC *pc)
{
    if (pc->m_move_level_prev == 0)
        pc->changeMotionSeq(47, -1);    /* run_in */
    else
        pc->changeMotionSeq(49, -1);    /* run */
}

// ROR 0x001211ac
void MagLifePCModeRun2D::move(MagLifePC *pc, BedPad *thePad)
{
    steer(pc);
}

// ROR 0x001211c8
void MagLifePCModeRun2D::live(MagLifePC *pc)
{
    pc->drainStamina();
}

// ROR 0x001211e4
int MagLifePCModeRun2D::pad(MagLifePC *pc, BedPad *thePad)
{
    int state = 5;
    if (pc->m_stance == 0)
    {
        if (pc->m_stick_mag <= 0.0f)
        {
            pc->changeState(1, 0, 0);
            return 0;
        }
        float roty = pc->getStickRotY();
        state = 4;
        if (pc->canTurnDirect(pc->m_actor->m_rotate[1], roty))
        {
            if (pc->m_move_level != 1)
            {
                move(pc, thePad);
                return 1;
            }
            state = 2;
        }
    }
    pc->changeState(state, 0, 0);
    return 2;
}

/* ---- Turn (state 4): the 180-degree turn of the move level ---- */

// ROR 0x001212c8
void MagLifePCModeTurn::init(MagLifePC *pc)
{
    switch (pc->m_move_level_prev)
    {
    case 0: pc->changeMotionSeq(50, -1); break;     /* turn_r180a */
    case 1: pc->changeMotionSeq(52, -1); break;     /* walk_turn_r180a */
    case 2: pc->changeMotionSeq(54, -1); break;     /* run_turn_r180a */
    }
    m_start_roty = pc->getStickRotY();
}

// ROR 0x00121348
void MagLifePCModeTurn::move(MagLifePC *pc, BedPad *thePad)
{
    applyRootMotion(pc);
}

// ROR 0x00121364
void MagLifePCModeTurn::live(MagLifePC *pc)
{
}

// ROR 0x0012136c
int MagLifePCModeTurn::pad(MagLifePC *pc, BedPad *thePad)
{
    float d = pc->getStickRotY() - m_start_roty;
    if (d < -DOG_PI)
        d += DOG_PI2;
    else if (DOG_PI <= d)
        d -= DOG_PI2;
    const float lim = 0x1.41b2f8p-2f;   /* pi / 10 */
    bool same = false;
    if (-lim <= d && d < lim)
        same = true;
    if (!pc->isMotionSeqEnd() && same)
    {
        move(pc, thePad);
        return 1;
    }
    int state;
    if (pc->m_move_level == 1)
        state = 2;
    else if (pc->m_move_level == 2)
        state = 3;
    else
    {
        pc->changeState(1, 0, 0);
        return 0;
    }
    pc->changeState(state, 0, 0);
    return 2;
}

/* ---- AtkWait2D (state 5): the stance held, turning to the target ---- */

// ROR 0x001214b4
void MagLifePCModeAtkWait2D::init(MagLifePC *pc)
{
    int height = pc->getAttackHeight();
    int hhh = height;
    if (height < 0)
    {
        pc->m_target_no = 0;
        hhh = 0;
    }
    ROR_ASSERT(0 <= hhh && hhh < 5);    /* 844 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    int wc = pc->m_weapon_type;
    ROR_ASSERT(0 <= wc && wc < 4);      /* 846 */
    int prev = pc->m_prev_state[0];
    if (pc->m_health == 2)
        wc += 4;
    /* back from an attack, a dog order or the stance walk: the row's
     * alternative (no in-motion) */
    if (prev == 10 || prev == 15 || prev == 16 || prev == 17 || prev == 6)
        pc->changeMotionSeqAlt(s_atk_wait_motion[wc][hhh], -1);
    else
        pc->changeMotionSeq(s_atk_wait_motion[wc][hhh], -1);
    pc->clearSubMotion();
    pc->m_08 = 0;
}

// ROR 0x00121604
float MagLifePCModeAtkWait2D::getTurnStep(float d)
{
    if (-0x1.921fb6p+0f < d && d <= 0x1.921fb6p+0f)     /* +-pi/2 */
    {
        if (0x1.999998p-5f < d || d < -0x1.999998p-5f)  /* 0.05 */
            return 0.0f <= d ? 0x1.999998p-4f : -0x1.999998p-4f;    /* 0.1 */
        return 0.0f;
    }
    return 0.0f;
}

// ROR 0x001216a4
void MagLifePCModeAtkWait2D::live(MagLifePC *pc)
{
    /* m_08: the turn to the target is over */
    if (pc->m_08 != 0)
        return;
    if (pc->m_target_no == 0)
    {
        pc->m_08 = 1;
        return;
    }
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    pc->getTargetSpinePos(pos, pc->m_target_no, 0);
    float roty, rotx;
    float step = getTurnStep(pc->calcTargetAngle(&roty, &rotx, pos));
    if (step == 0.0f)
    {
        pc->m_08 = 1;
        return;
    }
    DogActor *actor = pc->m_actor;
    float r = actor->m_rotate[1] + step;
    actor->m_rotate[1] = r;
    if (r < -DOG_PI)
        actor->m_rotate[1] = r + DOG_PI2;
    else if (DOG_PI <= r)
        actor->m_rotate[1] = r - DOG_PI2;
    actor->m_dirty = 1;
    pc->m_actor->addBaseRotY(step);
}

// ROR 0x001217c0
int MagLifePCModeAtkWait2D::pad(MagLifePC *pc, BedPad *thePad)
{
    /* nothing until the in-motion has played */
    if (0.0f <= pc->m_actor->getMotionTime())
    {
        int state;
        if (pc->m_stance == 0)
            state = 0;
        else if (pc->m_stick_mag <= 0.0f)
        {
            applyRootMotion(pc);
            return 0;
        }
        else
            state = 6;
        pc->changeState(state, 0, 0);
        return 2;
    }
    applyRootMotion(pc);
    return 0;
}

/* ---- AtkMove2D (state 6): walking with the stance held ---- */

// ROR 0x00121858
void MagLifePCModeAtkMove2D::init(MagLifePC *pc)
{
    pc->setStanceSubMotion();
    pc->m_action_motion = -1;
}

// ROR 0x00121890
void MagLifePCModeAtkMove2D::live(MagLifePC *pc)
{
}

// ROR 0x00121898
int MagLifePCModeAtkMove2D::pad(MagLifePC *pc, BedPad *thePad)
{
    int state = 1;
    if (pc->m_stance != 0)
    {
        if (0.0f < pc->m_stick_mag)
        {
            pc->moveAttackStance(thePad);
            return 0;
        }
        state = 5;
    }
    pc->changeState(state, 0, 0);
    return 2;
}

/* ---- Attacking (state 10): the swing, its hit test, the combo ---- */

// ROR 0x001218fc
void MagLifePCModeAttacking::init(MagLifePC *pc)
{
    m_04 = 0;
    m_08 = 0;
    m_0c = (int)pc->startAttack(0);
}

// ROR 0x00121934
void MagLifePCModeAttacking::live(MagLifePC *pc)
{
    if (!pc->m_actor->isModeSet())
    {
        /* the swing ended: the next combo step when one was asked for */
        if (m_08 != 0)
        {
            m_08 = 0;
            if (m_04 < 2)
            {
                m_04++;
                m_0c = (int)pc->startAttack(m_04);
                if ((m_0c & 0xffff) == 4)   /* the kick ends the combo */
                {
                    m_08 = 0;
                    m_04 = 1;
                }
                return;
            }
        }
        else
            m_08 = 0;
        m_04 = 0;
        pc->changeState(5, 2, 0);
        return;
    }
    applyRootMotion(pc);
    if ((short)(m_0c >> 16) == 1)
        return;
    float from, to;
    pc->getHitFrameRange(&from, &to);
    float frame = pc->m_actor->getMotionFrame();
    if (frame < from || to < frame)
        return;
    float hit_pos[8][4];
    float hit_radius[8];
    for (int i = 0; i < 8; i++)
    {
        hit_pos[i][0] = 0.0f;
        hit_pos[i][3] = 1.0f;
        hit_pos[i][2] = 0.0f;
        hit_pos[i][1] = 0.0f;
    }
    for (int i = 0; i < 8; i++)
        hit_radius[i] = pc->getWeaponHitPos(hit_pos[i], m_0c & 0xffff, i);
    UtlEnemyInfoBuf eib;
    for (int i = 0; i < 32; i++)
    {
        UtlEnemyInfo &e = eib.m_info[i];
        e.m_pos[2] = 0.0f;
        e.m_pos[1] = 0.0f;
        e.m_pos[0] = 0.0f;
        e.m_pos[3] = 1.0f;
        e.m_trans[3] = 1.0f;
        e.m_trans[0] = 0.0f;
        e.m_trans[2] = 0.0f;
        e.m_trans[1] = 0.0f;
    }
    eib.m_count = 0;
    int num = utlCollectEmyInfo(&eib, 0);
    for (int i = 0; i < num; i++)
    {
        ROR_ASSERT(eib.m_info[i].m_obj_id != 0);   /* 1161 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        if (utlIsEmyDamaging(eib.m_info[i].m_obj_id))
            continue;
        dbgTraceVector4(eib.m_info[i].m_pos);
        /* the weapon's hit spheres against the enemy's, the first hit counts */
        for (int j = 0; j < 8 && 0.0f < hit_radius[j]; j++)
        {
            if (utlVecDist(eib.m_info[i].m_pos, hit_pos[j]) < eib.m_info[i].m_radius + hit_radius[j])
            {
                pc->hitTarget(eib.m_info[i].m_obj_id, m_04, 0);
                int se = pc->getAttackSeNo(m_0c & 0xffff, m_04, 1);
                if (0 < se)
                    utlPlaySe(se, 0, 0);
                break;
            }
        }
    }
}

// ROR 0x00121c64
int MagLifePCModeAttacking::pad(MagLifePC *pc, BedPad *thePad)
{
    /* cross with the stance held late in the swing asks for the next step */
    if ((thePad->m_trig & 0x4000) && (thePad->m_hold & 0x800) &&
        0x1.666666p-1f <= pc->m_actor->getMotionTimeRatio())    /* 0.7 */
        m_08 = 1;
    return 1;
}

/* ---- Damage (state 14) ---- */

enum
{
    MAG_DAMAGE_NONE = 0,
    MAG_MAX_DAMAGE = 18,
};

// ROR 0x00121cd8
void MagLifePCModeDamage::init(MagLifePC *pc)
{
    int type = pc->m_08;
    /* 0x11 replays the last damage (MagLifePCModeNull::init's arg) */
    if (type != 0x11)
        m_04 = type;
    ROR_ASSERT(MAG_DAMAGE_NONE < type && type < MAG_MAX_DAMAGE);   /* 1259 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    pc->clearSubMotion();
    switch (type)
    {
    case 1:
        utlPlaySe(0x1e, 0, 0);
        pc->changeMotionSeq(0x1c, -1);
        m_08 = 0x1c;
        break;
    case 2:
        utlPlaySe(0x1f, 0, 0);
        if (pc->m_health == 2)
        {
            pc->changeMotionSeq(0x1e, -1);
            m_08 = 0x1e;
        }
        else
        {
            pc->changeMotionSeq(0x1d, -1);
            m_08 = 0x1d;
        }
        break;
    case 3:
        utlPlaySe(0x22, 0, 0);
        m_08 = -1;
        break;
    case 4:
        utlPlaySe(0x20, 0, 0);
        utlPlaySe(1, 0, 0);
        pc->changeMotionSeq(0x20, -1);
        m_08 = 0x20;
        break;
    case 5:
        utlPlaySe(0x20, 0, 0);
        utlPlaySe(1, 0, 0);
        pc->changeMotionSeq(0x23, -1);
        m_08 = 0x23;
        break;
    case 7:
        pc->changeMotionSeq(0xb, -1);
        m_08 = 0xb;
        break;
    case 9:
        pc->changeMotionSeq(0x8b, -1);
        m_08 = 0x8b;
        break;
    case 10:
        pc->changeMotionSeq(0x8c, -1);
        m_08 = 0x8c;
        break;
    case 0xb:
        pc->changeMotionSeq(0x92, -1);
        m_08 = 0x92;
        break;
    case 0xc:
        pc->changeMotionSeq(0x8d, -1);
        m_08 = 0x8d;
        break;
    case 0xd:
        pc->changeMotionSeq(0x97, -1);
        m_08 = 0x97;
        break;
    case 0xe:
        pc->changeMotionSeq(0x9b, -1);
        m_08 = 0x9b;
        break;
    case 0xf:
        pc->changeMotionSeq(0x95, -1);
        m_08 = 0x95;
        break;
    case 0x10:
        pc->changeMotionSeq(0x96, -1);
        m_08 = 0x96;
        break;
    case 0x11:
        if (0 <= m_08)
            pc->changeMotionSeqAlt(m_08, -1);
        break;
    default:
        m_08 = -1;
        ROR_ASSERT(0);  /* 1395 */
        break;
    }
    g_magSystem.startVibration(6);
}

// ROR 0x00121fc8
void MagLifePCModeDamage::live(MagLifePC *pc)
{
    applyRootMotion(pc);
    if (!pc->isMotionSeqEnd())
    {
        if (pc->m_actor && pc->m_actor->m_motion.getNo() < 2)
            pc->changeState(1, 0, 0);
        return;
    }
    if (pc->m_mode == 1 || pc->m_mode == 3)
        pc->changeState(7, 0, 0);
    else if (pc->m_mode_hold.m_04)
        pc->changeState(8, 0, 0);
    else if (pc->m_mode_ride.m_04)
        pc->changeState(9, 3, 0);
    else if (pc->m_stance == 0)
        pc->changeState(1, 0, 0);
    else
        pc->changeState(5, 0, 0);
}

/* ---- Item (state 19): reach for an item at its height, take it ---- */

// ROR 0x001220f0
void MagLifePCModeItem::init(MagLifePC *pc)
{
    u_int oid = (u_int)pc->m_08;
    ROR_ASSERT(oid != 0);           /* 1486 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    utlGetObjPos(pos, oid);
    DogActor *actor = pc->m_actor;
    ROR_ASSERT(actor != NULL);      /* 1492 */
    /* y grows downward: how far above the feet the item is */
    float height = -(pos[1] - actor->m_trans[1]);
    int motion = 0x81;
    if (50.0f <= height)
    {
        motion = 0x83;
        if (100.0f <= height)
        {
            motion = 0x85;
            if (160.0f <= height)
                motion = 0x87;
        }
    }
    pc->m_0c = motion;
    pc->changeMotionSeq(motion, -1);
}

// ROR 0x001221ec
void MagLifePCModeItem::live(MagLifePC *pc)
{
    int end = pc->isMotionSeqEnd();
    MagScene *scene = pc->m_scene;
    /* m_0c 0x81 / 0x83 / 0x85 / 0x87 reach, + 1 the rise after it */
    if (((pc->m_0c - 0x81) & 1) != 0)
    {
        if (end)
            pc->changeState(1, 0, 0);
        return;
    }
    if (!end)
        return;
    pc->m_0c++;
    pc->changeMotionSeq(pc->m_0c, -1);
    MagLifePCItemData items[10];
    for (int i = 0; i < 10; i++)
    {
        items[i].m_pos[0] = 0.0f;
        items[i].m_pos[3] = 1.0f;
        items[i].m_pos[2] = 0.0f;
        items[i].m_pos[1] = 0.0f;
    }
    if (pc->collectNearItems(50.0f, items, 10) <= 0)
        return;
    int msg;
    int args[16];
    const int *argp = NULL;
    if (!pc->canTakeItem(items[0].m_obj_id))
        msg = 0x12;
    else
    {
        utlPlaySe(3, 0, 0);
        pc->takeItem(items[0].m_obj_id);
        argp = args;
        args[0] = items[0].m_item_id;
        if (items[0].m_item_id == 0xa64)
        {
            /* the EE passes (0x11, args, 1) too; pulseMenu1000a reads none */
            scene->pulseMenu1000a();
            return;
        }
        msg = 0x11;
    }
    scene->openSysMsgWindow(msg, argp, 1);
}

/* ---- the dog orders (states 15 go, 16 come, 17 stay) ---- */

// ROR 0x00122338
void MagLifePCModeDogGo::init(MagLifePC *pc)
{
    bool armed = pc->isWordDcSet();
    int motion;
    if (pc->m_mode == 3)
        motion = 0x7e;
    else if (!armed)
        motion = pc->m_health == 2 ? 0x15 : 0xf;
    else
        motion = 0x12;
    pc->changeMotionSeq(motion, -1);
    pc->orderDog(15, 1);
    pc->m_08 = 1;
}

// ROR 0x001223c0
void MagLifePCModeDogGo::live(MagLifePC *pc)
{
    int work = pc->m_08;
    int end = pc->isMotionSeqEnd();
    if (work == 1)
    {
        if (end)
        {
            pc->orderDog(15, 2);
            pc->changeState(0, 0, 0);
        }
    }
    else if (work == 2 && end)
        pc->changeState(0, 0, 0);
}

// ROR 0x0012245c
void MagLifePCModeDogCome::init(MagLifePC *pc)
{
    pc->m_move_level_prev = 0;
    pc->m_move_level = 0;
    if (pc->m_weapon_type != 3)
    {
        /* Brown 100..130 ahead within 45 degrees: a hug instead */
        MagLifeBwnCtrl *ctrl = &pc->m_scene->m_bwn_ctrl;
        float bwn[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        ctrl->getBwnPosition(bwn);
        float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        pc->getPosition(pos);
        float rot[4];
        float dist = utlVecDistRotY(pos, rot, bwn);
        float d = pc->getRotY() - rot[0];
        if (d < -DOG_PI)
            d += DOG_PI2;
        else if (DOG_PI <= d)
            d -= DOG_PI2;
        const float lim = 0x1.921fb6p-1f;   /* pi / 4 */
        if (-lim <= d && d < lim && 100.0f <= dist && dist < 130.0f && 0 < ctrl->checkHug(0))
        {
            pc->changeState(18, 0, 0);
            return;
        }
    }
    bool armed = pc->isWordDcSet();
    int motion;
    if (pc->m_mode == 3)
        motion = 0x7f;
    else if (!armed)
        motion = pc->m_health == 2 ? 0x16 : 0x10;
    else
        motion = 0x13;
    pc->changeMotionSeq(motion, -1);
    pc->orderDog(16, 1);
    pc->m_08 = 1;
}

// ROR 0x00122668
void MagLifePCModeDogCome::live(MagLifePC *pc)
{
    if ((u_int)(pc->m_08 - 1) < 2 && pc->isMotionSeqEnd())
    {
        pc->orderDog(16, 2);
        pc->changeState(0, 0, 0);
    }
}

// ROR 0x001226d0
void MagLifePCModeDogStay::init(MagLifePC *pc)
{
    pc->orderDog(17, 3);
    bool armed = pc->isWordDcSet();
    int motion;
    if (pc->m_mode == 3)
        motion = 0x80;
    else if (!armed)
        motion = pc->m_health == 2 ? 0x17 : 0x11;
    else
        motion = 0x14;
    pc->changeMotionSeq(motion, -1);
    pc->m_08 = 1;
}

// ROR 0x00122758
void MagLifePCModeDogStay::live(MagLifePC *pc)
{
    int work = pc->m_08;
    int end = pc->isMotionSeqEnd();
    if (work == 1)
    {
        if (end)
            pc->changeState(0, 0, 0);
    }
    else if (work == 2 && end)
        pc->changeState(0, 0, 0);
}

/* ---- Hold (state 8): held by an enemy; wiggling the stick breaks free ---- */

// ROR 0x001227e4
void MagLifePCModeHold::init(MagLifePC *pc)
{
    pc->m_move_level_prev = 0;
    pc->m_move_level = 0;
    pc->clearSubMotion();
    pc->changeMotionSeq(10, -1);
    pc->m_08 = 0;
}

// ROR 0x00122828
int MagLifePCModeHold::pad(MagLifePC *pc, BedPad *thePad)
{
    int work = pc->m_08;
    int end = pc->isMotionSeqEnd();
    if (16 <= thePad->m_quad_flips)
    {
        pc->changeState(12, 0, 0);
        return 2;
    }
    if (work == 0)
    {
        if (pc->m_stick_mag <= 0.0f)
            pc->changeMotionSeq(10, -1);
        else
        {
            pc->changeMotionSeq(12, -1);    /* the struggle */
            pc->m_08 = 1;
        }
        return 1;
    }
    if (end)
    {
        pc->changeMotionSeq(10, -1);
        pc->m_08 = 0;
    }
    return 1;
}

// ROR 0x001228fc
void MagLifePCModeHoldAttack::init(MagLifePC *pc)
{
    pc->changeMotionSeq(13, -1);
    utlPlaySe(0x2e, 0, 0);
}

// ROR 0x00122930
void MagLifePCModeUnhold::init(MagLifePC *pc)
{
    pc->changeMotionSeq(14, -1);
}

/* ---- Ride (state 9). pc->m_08 is the state's arg, pc->m_0c the phase
 * (0 riding, | 1 the stick's motion, 2 getting off). ---- */

// ROR 0x00122954
void MagLifePCModeRide::init(MagLifePC *pc)
{
    switch (pc->m_08)
    {
    case 2:
        pc->changeMotionSeq(0x89, -1);
        m_08 = 1;
        break;
    case 3:
    {
        /* back on after a damage: the kind's riding row */
        int motion;
        switch (m_08)
        {
        case 1: motion = 0x8a; break;
        case 2: motion = 0x91; break;
        case 3: motion = 0x98; break;
        case 4: motion = 0x9c; break;
        default:
            ROR_ASSERT(0);  /* 2077 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
            motion = -1;
            break;
        }
        if (0 <= motion)
            pc->changeMotionSeq(motion, -1);
        break;
    }
    case 7:
        pc->changeMotionSeq(0x90, -1);
        m_08 = 2;
        break;
    case 10:
        pc->changeMotionSeq(0x98, -1);
        m_08 = 3;
        break;
    case 12:
        pc->changeMotionSeq(0x9c, -1);
        m_08 = 4;
        break;
    default:
        ROR_ASSERT(0);  /* 2082 */
        break;
    }
    m_04 = 1;
    pc->m_0c = 0;
}

// ROR 0x00122ad4
void MagLifePCModeRide::live(MagLifePC *pc)
{
    applyRootMotion(pc);
    if (2 <= pc->m_0c)
    {
        if (pc->isMotionSeqEnd())
            pc->changeState(0, 0, 0);
        return;
    }
    if (pc->m_0c <= 0 || !pc->isMotionSeqEnd())
        return;
    switch (m_08)
    {
    case 1: pc->changeMotionSeq(0x8a, -1); break;
    case 2: pc->changeMotionSeq(0x91, -1); break;
    case 3: pc->changeMotionSeq(0x98, -1); break;
    case 4: pc->changeMotionSeq(0x9c, -1); break;
    }
    pc->m_0c = 0;
}

// ROR 0x00122bd4
int MagLifePCModeRide::pad(MagLifePC *pc, BedPad *thePad)
{
    if (pc->m_0c < 1 && 0.0f < pc->m_stick_mag)
    {
        int seq = pc->getMotionSeq();
        int motion;
        if ((u_int)(seq - 0x8a) < 4)
            motion = 0x8e;
        else if ((u_int)(seq - 0x91) < 2)
            motion = 0x93;
        else if (seq == 0x98)
            motion = 0x99;
        else if (seq == 0x9c)
            motion = 0x9d;
        else
            return 1;
        pc->changeMotionSeq(motion, -1);
        pc->m_0c |= 1;
    }
    return 1;
}

// ROR 0x00122c9c
void MagLifePCModeRide::message(MagLifePC *pc, int msg)
{
    if (1 < pc->m_0c)
        return;
    pc->m_08 = msg;
    int motion;
    switch (msg)
    {
    case 6:   motion = 0x8f; break;
    case 9:   motion = 0x94; break;
    case 0xb: motion = 0x9a; break;
    case 0xd: motion = 0x9e; break;
    default:
        ROR_ASSERT(0);  /* 2225 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        return;
    }
    pc->changeMotionSeq(motion, -1);
    pc->m_0c = 2;
    m_04 = 0;
}

/* ---- Roped (state 7): tied up in m_mode 1 / 3, the stick struggles ---- */

enum { MAG_CTRL_PC_ROPED5 = 3 };

// ROR 0x00122d70
void MagLifePCModeRoped::init(MagLifePC *pc)
{
    int motion = 0;
    if (pc->m_mode == 1)
        motion = 0x74;
    else if (pc->m_mode == 3)
        motion = 0x79;
    else
        ROR_ASSERT(0);  /* 2249 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    pc->changeMotionSeq(motion, -1);
}

// ROR 0x00122dec
void MagLifePCModeRoped::live(MagLifePC *pc)
{
    pc->recoverStamina();
    applyRootMotion(pc);
}

// ROR 0x00122e2c
int MagLifePCModeRoped::pad(MagLifePC *pc, BedPad *thePad)
{
    if (pc->m_stance != 0)
    {
        pc->changeState(5, 0, 0);
        return 2;
    }
    int base = 0x74;
    if (pc->m_mode != 1)
    {
        int flag = pc->m_mode;
        ROR_ASSERT(flag == MAG_CTRL_PC_ROPED5);    /* 2295 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        base = 0x79;
    }
    /* a new pose only from the rest pose or once the last one ended */
    if (!pc->isMotionSeqEnd() && pc->getMotionSeq() != base)
        return 1;
    const float lim = 0x1.333332p-2f;   /* 0.3 */
    int dir = 0;
    if (0.0f < pc->m_stick_mag)
    {
        if (lim < pc->m_stick_x)
            dir = 2;
        else if (pc->m_stick_x < -lim)
            dir = 1;
        else if (lim < pc->m_stick_y)
            dir = 4;
        else if (pc->m_stick_y < -lim)
            dir = 3;
    }
    pc->changeMotionSeq(base + dir, -1);
    if (dir != 0)
    {
        int r = utlRandRange(0, 99);
        int se;
        if (r < 15)
            se = 0x3f;
        else if (r < 35)
            se = 0x32;
        else if (r < 55)
            se = 0x33;
        else if (r < 70)
            se = 0x2e;
        else if (r < 85)
            se = 0x34;
        else
            se = -1;
        if (0 <= se)
            utlPlaySe(se, 15, 0);
    }
    return 1;
}

/* ---- Reload (state 20) ---- */

// ROR 0x00122fc4
void MagLifePCModeReload::init(MagLifePC *pc)
{
    pc->clearSubMotion();
    pc->changeMotionSeq(4, -1);
    pc->reload();
}

// ROR 0x00123004
void MagLifePCModeReload::live(MagLifePC *pc)
{
    if (!pc->isMotionSeqEnd())
        return;
    u_int chr = (u_int)pc->getEquipItemChrNo();
    if (chr < 0x80)
    {
        DogActor *item = &pc->m_scene->m_actors[chr];
        int no = item->getMotionNoBySuffix("wait2");
        item->changeMotion(0x1.11112Ep-6f, no, 1, -1, -1); /* 0x3c888897 */
    }
    pc->changeState(0, 0, 0);
}

/* ---- Hug (state 18): turn to Brown, then one of the two hugs ---- */

// ROR 0x001230a8
void MagLifePCModeHug::init(MagLifePC *pc)
{
    pc->m_08 = 0;
    pc->changeMotionSeq(5, -1);
    pc->m_move_level_prev = 0;
    pc->m_move_level = 0;
    pc->m_scene->m_bwn_ctrl.requestHug(0);
}

// ROR 0x001230fc
void MagLifePCModeHug::live(MagLifePC *pc)
{
    /* m_08: 0 turning, 1 / 2 the hug picked, 3 hugging */
    int work = pc->m_08;
    MagLifeBwnCtrl *ctrl = &pc->m_scene->m_bwn_ctrl;
    if (work != 0)
    {
        if ((u_int)(work - 1) < 2)
        {
            if (ctrl->checkHug(work) < 1)
                return;
            pc->changeMotionSeq(work + 0x18, -1);
            ctrl->requestHug(work);
            pc->m_08 = 3;
            return;
        }
        if (pc->isMotionSeqEnd())
            pc->changeState(0, 0, 0);
        return;
    }
    float dir[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ctrl->getBwnPosition(dir);
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    pc->getPosition(pos);
    dir[0] -= pos[0];
    dir[1] -= pos[1];
    dir[2] -= pos[2];
    dir[3] = 1.0f;
    float d = utlVecToRotY(dir) - pc->getRotY();
    if (d < -DOG_PI)
        d += DOG_PI2;
    else if (DOG_PI <= d)
        d -= DOG_PI2;
    if (d == 0.0f)
    {
        work = utlRandRange(1, 2);
        pc->m_08 = work;
        ROR_ASSERT(work == 1 || work == 2);    /* 2522 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        return;
    }
    const float lim = 0x1.999998p-5f;   /* 0.05 */
    if (lim < d)
        d = lim;
    if (d < -lim)
        d = -lim;
    /* the EE reads the actor through the vtable's getActor (slot 11) */
    DogActor *actor = pc->m_actor;
    float r = actor->m_rotate[1] + d;
    actor->m_rotate[1] = r;
    if (r < -DOG_PI)
        actor->m_rotate[1] = r + DOG_PI2;
    else if (DOG_PI <= r)
        actor->m_rotate[1] = r - DOG_PI2;
    actor->m_dirty = 1;
    pc->m_actor->addBaseRotY(d);
}
