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
#include "mag/maglifepc.h"
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

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00122c9c
void MagLifePCModeRide::message(MagLifePC *pc, int msg)
{
    ROSEPAN_STUB("MagLifePCModeRide::message", 0x00122c9c);
}

// ---- the modes' virtuals (hand-written stubs: their vtables need every
// out-of-line slot defined, which stubgen cannot see) ----

// stub of ROR 0x001214b4
void MagLifePCModeAtkWait2D::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeAtkWait2D::init", 0x001214b4);
}

// stub of ROR 0x001216a4
void MagLifePCModeAtkWait2D::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeAtkWait2D::live", 0x001216a4);
}

// stub of ROR 0x001217c0
int MagLifePCModeAtkWait2D::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeAtkWait2D::pad", 0x001217c0);
    return {};
}

// stub of ROR 0x00121858
void MagLifePCModeAtkMove2D::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeAtkMove2D::init", 0x00121858);
}

// stub of ROR 0x00121890
void MagLifePCModeAtkMove2D::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeAtkMove2D::live", 0x00121890);
}

// stub of ROR 0x00121898
int MagLifePCModeAtkMove2D::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeAtkMove2D::pad", 0x00121898);
    return {};
}

// stub of ROR 0x001218fc
void MagLifePCModeAttacking::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeAttacking::init", 0x001218fc);
}

// stub of ROR 0x00121934
void MagLifePCModeAttacking::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeAttacking::live", 0x00121934);
}

// stub of ROR 0x00121c64
int MagLifePCModeAttacking::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeAttacking::pad", 0x00121c64);
    return {};
}

// stub of ROR 0x00121cd8
void MagLifePCModeDamage::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDamage::init", 0x00121cd8);
}

// stub of ROR 0x00121fc8
void MagLifePCModeDamage::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDamage::live", 0x00121fc8);
}

// stub of ROR 0x001220f0
void MagLifePCModeItem::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeItem::init", 0x001220f0);
}

// stub of ROR 0x001221ec
void MagLifePCModeItem::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeItem::live", 0x001221ec);
}

// stub of ROR 0x00122338
void MagLifePCModeDogGo::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDogGo::init", 0x00122338);
}

// stub of ROR 0x001223c0
void MagLifePCModeDogGo::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDogGo::live", 0x001223c0);
}

// stub of ROR 0x0012245c
void MagLifePCModeDogCome::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDogCome::init", 0x0012245c);
}

// stub of ROR 0x00122668
void MagLifePCModeDogCome::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDogCome::live", 0x00122668);
}

// stub of ROR 0x001226d0
void MagLifePCModeDogStay::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDogStay::init", 0x001226d0);
}

// stub of ROR 0x00122758
void MagLifePCModeDogStay::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeDogStay::live", 0x00122758);
}

// stub of ROR 0x001227e4
void MagLifePCModeHold::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeHold::init", 0x001227e4);
}

// stub of ROR 0x00122828
int MagLifePCModeHold::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeHold::pad", 0x00122828);
    return {};
}

// stub of ROR 0x001228fc
void MagLifePCModeHoldAttack::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeHoldAttack::init", 0x001228fc);
}

// stub of ROR 0x00122930
void MagLifePCModeUnhold::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeUnhold::init", 0x00122930);
}

// stub of ROR 0x00122954
void MagLifePCModeRide::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeRide::init", 0x00122954);
}

// stub of ROR 0x00122ad4
void MagLifePCModeRide::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeRide::live", 0x00122ad4);
}

// stub of ROR 0x00122bd4
int MagLifePCModeRide::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeRide::pad", 0x00122bd4);
    return {};
}

// stub of ROR 0x00122d70
void MagLifePCModeRoped::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeRoped::init", 0x00122d70);
}

// stub of ROR 0x00122dec
void MagLifePCModeRoped::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeRoped::live", 0x00122dec);
}

// stub of ROR 0x00122e2c
int MagLifePCModeRoped::pad(MagLifePC *pc, BedPad *thePad)
{
    ROSEPAN_STUB("MagLifePCModeRoped::pad", 0x00122e2c);
    return {};
}

// stub of ROR 0x00122fc4
void MagLifePCModeReload::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeReload::init", 0x00122fc4);
}

// stub of ROR 0x00123004
void MagLifePCModeReload::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeReload::live", 0x00123004);
}

// stub of ROR 0x001230a8
void MagLifePCModeHug::init(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeHug::init", 0x001230a8);
}

// stub of ROR 0x001230fc
void MagLifePCModeHug::live(MagLifePC *pc)
{
    ROSEPAN_STUB("MagLifePCModeHug::live", 0x001230fc);
}
