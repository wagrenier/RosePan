/* ==========================================================================
 *  mag/maglifepc.cpp
 *
 *  MagLifePC, the player character's controller: its state machine over
 *  the embedded modes (changeState / selectMode), the per-frame live, the
 *  pad reading (processPad: health, move level, stance, the dog orders),
 *  the event messages, stamina, the sub-mode stack and the tuning options.
 *  0x11ab40..0x11c464 (pinned 2026-10-27; the asserts name this file).
 * ======================================================================== */

#include "mag/maglifepc.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bed/bedpad.h"
#include "common/ror_assert.h"
#include "common/rosepan_stub.h"
#include "dog/dogactor.h"
#include "dog/dogcamera.h"
#include "lib/unassigned.h"
#include "mag/maglifebwnctrl.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "utl/utlgame.h"
#include "utl/utlmath.h"

/* The footstep sounds by floor kind (playStepSe adds the frame parity:
 * the odd id is the other foot); kind 0 has none. */
// ROR 0x002e9ef8
static const int s_pcStepSe[12] = {
    -100, 151, 145, 147, 165, 161, 163, 159, 149, 157, 155, 167,
};

/* Written by readStick when a run starts (g_magSystem's frame count);
 * nothing reads it. */
// ROR 0x00297140
static int s_runStartFrame;

// ──────────────────────────────────────────────────────────────────────
// Unreferenced: back to the state setState saved, which is cleared; the
// mode is reselected without its init.
// ROR 0x0011ab40
void MagLifePC::restoreState()
{
    clearMotionSeq();
    int prev[4];
    for (int i = 0; i < 4; i++)
    {
        prev[i] = m_prev_state[i];
        m_prev_state[i] = 0;
    }
    m_state = prev[0];
    m_04 = prev[1];
    m_08 = prev[2];
    m_0c = prev[3];
    selectMode(0);
}

// ──────────────────────────────────────────────────────────────────────
// The health class: m_health_override when set; 3 / 4 for the modes 1 / 3;
// 2 while the PC param ratio is at most 0.3; else 1 at stamina <= 300, 0.
// ROR 0x0011ab88
int MagLifePC::calcHealth()
{
    int health = m_health_override;
    if (health < 0)
    {
        health = 3;
        if (m_mode != 1)
        {
            health = 4;
            if (m_mode != 3)
            {
                int cur = getPcParam1b();
                int max = getPcParam1c();
                health = 2;
                if (0x1.333332p-2f < (float)cur / (float)max) /* 0x3e999999 */
                    health = (m_stamina < 301);
            }
        }
    }
    return health;
}

// ──────────────────────────────────────────────────────────────────────
// The left stick: its direction into m_stick_x / y, the change into
// m_stick_change. 0 inside the dead zone, 2 beyond 0.97 unless the health
// class is 2, else 1.
// ROR 0x0011ac24
int MagLifePC::readStick(BedPad *pad)
{
    int level = 0;
    float mag = pad->getDir(&m_stick_x, &m_stick_y);
    m_stick_change = pad->getDirChange();
    if (m_dead_zone <= mag)
    {
        m_stick_mag = mag;
        if (0x1.f0a3d6p-1f < mag) /* 0x3f7851eb, 0.97 */
        {
            if (m_health != 2)
            {
                m_84 = 0;
                if (m_move_level_prev != 2)
                    s_runStartFrame = g_magSystem.m_frame;
                return 2;
            }
            m_84 = 0;
        }
        else
        {
            m_84 = 0;
        }
        level = 1;
    }
    else
    {
        m_84 = 0;
        m_stick_mag = 0.0f;
    }
    return level;
}

// ──────────────────────────────────────────────────────────────────────
// The stance: with 0x800 (R1) held, the weapon type + 1; 0 without; -1
// (keep the last) in state 10. Mode 2 and the health classes 3 / 4 have
// none.
// ROR 0x0011acf4
int MagLifePC::readStance(BedPad *pad)
{
    int stance = 0;
    if (m_mode != 2 && m_health < 3)
    {
        int st = m_state;
        if (st < 7 || (unsigned)(st - 10) < 2)
        {
            if (st == 10)
                stance = -1;
            else if (pad->m_hold & 0x800)
                stance = m_weapon_type + 1;
        }
    }
    return stance;
}

// ──────────────────────────────────────────────────────────────────────
// One pad frame's status words, each keeping its last value.
// ROR 0x0011ad6c
void MagLifePC::updatePadStatus(BedPad *pad)
{
    m_health_prev = m_health;
    m_health = calcHealth();
    m_move_level_prev = m_move_level;
    m_move_level = readStick(pad);
    m_stance_prev = m_stance;
    int stance = readStance(pad);
    m_stance = stance;
    if (stance < 0)
        m_stance = m_stance_prev;
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 2: each frame with an actor. The shadow view's blend (scene
// +0x22b0) is 0.1 in mode 3, else climbs back to 1 by 0.05. Then the
// footsteps, the mode's live (state 11 picks its attack target instead; no
// mode re-enters the state), and the motion's root movement outside the
// moving states.
// ROR 0x0011ade4
void MagLifePC::live()
{
    if (!m_actor)
        return;
    MagScene *scene = m_scene;
    if (m_mode == 3)
    {
        scene->m_22b0 = 0x1.999998p-4f; /* 0x3dcccccc */
    }
    else
    {
        float f = scene->m_22b0;
        if (1.0f < f)
            scene->m_22b0 = 1.0f;
        else if (f < 1.0f)
            scene->m_22b0 = f + 0x1.999998p-5f; /* 0x3d4ccccc */
    }
    m_04++;
    playStepSe(s_pcStepSe);
    if (m_state == 11)
        beginAttack();
    else if (m_mode_cur)
        m_mode_cur->live(this);
    else
        changeState(m_state, 0, 0);
    m_mode_frame++;
    if (!isMoveState())
        applyMotionMove();
    m_pad_seen = 0;
}

// ROR 0x0011af04
bool MagLifePC::isWordDcSet()
{
    return m_stance != 0;
}

// ROR 0x0011af10
int MagLifePC::getState17Count()
{
    int count = -1;
    if (m_state == 0x17)
        count = m_04;
    return count;
}

// ──────────────────────────────────────────────────────────────────────
// An order to Brown: state 15 go, 16 come, 17 stay. flags & 1 plays one of
// the PC's voice lines for it, flags & 2 passes it to MagLifeBwnCtrl.
// ROR 0x0011af30
void MagLifePC::orderDog(int state, u_int flags)
{
    u_int toDog = flags & 2;
    u_int voice = flags & 1;
    MagLifeBwnCtrl *ctrl = &m_scene->m_bwn_ctrl;
    switch (state)
    {
    case 15:
        if (voice)
            utlPlaySeRand3(0x26, 0x27, 0x28);
        if (toDog)
            ctrl->orderGo(0);
        return;
    case 16:
        if (voice)
            utlPlaySeRand4(0x25, 0x38, 0x23, 0x24);
        if (toDog)
            ctrl->orderCome(0);
        return;
    case 17:
        if (voice)
            utlPlaySeRand3(0x29, 0x2a, 0x37);
        if (toDog)
            ctrl->orderStay(0);
        return;
    }
    ROR_ASSERT(0);  /* 1046 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
}

// ──────────────────────────────────────────────────────────────────────
// Picks up the nearest item within 50: state 19 (Item) with its object id.
// ROR 0x0011b07c
void MagLifePC::pickUpNearItem()
{
    MagLifePCItemData itemdata[10];
    for (int i = 0; i < 10; i++)
    {
        itemdata[i].m_pos[0] = 0.0f;
        itemdata[i].m_pos[3] = 1.0f;
        itemdata[i].m_pos[2] = 0.0f;
        itemdata[i].m_pos[1] = 0.0f;
    }
    if (collectNearItems(50.0f, itemdata, 10) > 0)
    {
        ROR_ASSERT(itemdata[0].m_obj_id != 0);  /* 1065 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        changeState(19, itemdata[0].m_obj_id, 0);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The stick's direction in the world: while the stick moved more than
// m_stick_threshold, its Y angle plus the camera's, wrapped to -pi..pi;
// the last one otherwise.
// ROR 0x0011b128
float MagLifePC::getStickRotY()
{
    if (m_stick_threshold < m_stick_change)
    {
        float dir[4] = { m_stick_x, 0.0f, -m_stick_y, 1.0f };
        float rot = utlVecToRotY(dir) + m_scene->m_camera_entity.getRotY();
        if (rot < -DOG_PI)
            rot += DOG_PI2;
        else if (DOG_PI <= rot)
            rot -= DOG_PI2;
        m_stick_roty = rot;
    }
    return m_stick_roty;
}

// ──────────────────────────────────────────────────────────────────────
// Whether the PC can turn from one angle to the other without the turn
// motion: always while blending in, else within +-153 degrees.
// ROR 0x0011b204
int MagLifePC::canTurnDirect(float from, float to)
{
    if (isBlendIn())
        return 1;
    float d = to - from;
    if (d < -DOG_PI)
        d += DOG_PI2;
    else if (DOG_PI <= d)
        d -= DOG_PI2;
    const float limit = 0x1.55ce28p+1f; /* 0x402ae714, 153 degrees */
    return -limit <= d && d < limit;
}

// ROR 0x0011b2c4
int MagLifePC::padMode(BedPad *pad)
{
    if (!m_mode_cur)
        return 0;
    return m_mode_cur->pad(this, pad);
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 16: the pad in states 1..11. The status words first; a stance
// change reloads the motions and arms (state 11) or disarms (state 0).
// Then cross attacks from the attack modes, the mode's pad in state 10;
// below it L1 reloads, triangle / circle / square order Brown (in the Ride
// mode, state 9, the order goes out directly), and the mode reads the pad
// until it returns 1 or 0.
// ROR 0x0011b30c
void MagLifePC::processPad(BedPad *pad)
{
    m_pad_seen = 1;
    if (!m_actor)
        return;
    int st = m_state;
    if ((unsigned)(st - 1) >= 11)
        return;
    m_weapon_type = getWeaponType(getEquipItem());
    updatePadStatus(pad);
    if (m_stance_prev != m_stance)
    {
        m_f4 = -1;
        initMotion();
        if (m_stance_prev == 0)
        {
            changeState(11, 0, 0);
            m_08 = 0;
            return;
        }
        if (m_stance == 0)
        {
            changeState(0, 0, 0);
            return;
        }
    }
    if (st >= 11)
        return;
    if (m_mode != 1 && m_mode != 3 && (pad->m_trig & 0x4000) && (unsigned)(st - 5) < 2)
    {
        changeState(10, 0, 0);
        return;
    }
    if (st == 10)
    {
        if (m_mode_cur)
            m_mode_cur->pad(this, pad);
        return;
    }
    if ((pad->m_trig & 0x400) && canReload())
    {
        changeState(20, 0, 0);
        return;
    }
    if (m_mode != 1 && utlTestSeqBit34() && m_scene->m_bwn_ctrl.m_ctrl_level == 0)
    {
        unsigned short trig = pad->m_trig;
        int order = -1;
        if (trig & 0x1000)
        {
            order = 0;
        }
        else
        {
            if (trig & 0x8000)
                order = 2;
            if (trig & 0x2000)
                order = 1;
        }
        if (order >= 0)
        {
            if (st != 9)
            {
                changeState(order + 15, 0, 0);
                return;
            }
            if (m_mode_frame < 0)
                return;
            orderDog(order + 15, 3);
            m_mode_frame = -100;
            return;
        }
    }
    int r;
    do
    {
        r = padMode(pad);
        if (r == 1)
            return;
    } while (r > 1);
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 14: an event message.
//    0 / 1   hold (state 8; counted, the last release: state 13)
//    2 7 10 12  ride (state 9 with msg)   6 9 11 13  to the Ride mode
//    14..17  message14..17   18 / 19 / 20  stamina: get / set / at least 300
//    21      move the actor to arg (x, y, z, rot y) if the sweep is free;
//            the rotation always (returns 1 when moved)
//    22      1 riding, or hurt with sub state 4 / 5 / 17
//    23      the weapon override (arg's high half) and m_1b0, then the
//            motions again   24  the health override
// ROR 0x0011b5e0
int MagLifePC::message(int msg, const float *arg)
{
    DogActor *actor = m_actor;
    if (!actor)
        return (int)0xfffffe93;
    /* PC: the int messages carry their value in the pointer argument */
    int value = (int)(intptr_t)arg;
    switch (msg)
    {
    case 0:
        changeState(8, 0, 0);
        m_mode_hold.m_04++;
        break;
    case 1:
        if (--m_mode_hold.m_04 == 0)
        {
            changeState(13, 0, 0);
            return 0;
        }
        break;
    case 2:
    case 7:
    case 10:
    case 12:
        changeState(9, msg, 0);
        return 0;
    case 3:
    case 4:
    case 5:
    case 8:
    default:
        ROR_ASSERT(0);  /* 1607 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        break;
    case 6:
    case 9:
    case 11:
    case 13:
        m_mode_ride.message(this, msg);
        return 0;
    case 14:
        message14();
        return 0;
    case 15:
        message15();
        return 0;
    case 16:
        message16();
        return 0;
    case 17:
        message17();
        return 0;
    case 18:
        return m_stamina;
    case 19:
        m_stamina = value;
        break;
    case 20:
        if (m_stamina > 299)
            return 0;
        m_stamina = 300;
        break;
    case 21:
    {
        if (!actor)
            return 0;
        float from[4], delta[4], out[4];
        for (int i = 0; i < 4; i++)
            from[i] = actor->m_trans[i];
        delta[0] = arg[0] - from[0];
        delta[2] = arg[2] - from[2];
        delta[3] = 1.0f;
        delta[1] = arg[1] - from[1];
        out[0] = out[1] = out[2] = 0.0f;
        out[3] = 1.0f;
        float hit = sweepMove(m_sweep_range + m_sweep_radius, m_sweep_radius, from, delta, out);
        if (hit < 0.0f)
        {
            /* the ROM copies all four words: m_trans[3] takes the rotation */
            actor = m_actor;
            actor->m_dirty = 1;
            for (int i = 0; i < 4; i++)
                actor->m_trans[i] = arg[i];
        }
        dbgTraceVector4(arg);
        float rot = arg[3];
        if (rot < -DOG_PI)
            rot += DOG_PI2;
        else if (DOG_PI <= rot)
            rot -= DOG_PI2;
        actor = m_actor;
        actor->m_rotate[1] = rot;
        actor->normalizeRotate();
        actor->m_dirty = 1;
        actor->setBaseRotateY(rot);
        return hit < 0.0f;
    }
    case 22:
        if (actor)
        {
            if (m_state == 9)
                return 1;
            if (m_state != 14)
                return 0;
            if ((unsigned)(m_08 - 4) < 2)
                return 1;
            if (m_08 == 0x11)
                return 1;
            return 0;
        }
        break;
    case 23:
        m_weapon_override = value >> 16;
        m_1b0 = (short)value;
        initMotion();
        return 0;
    case 24:
        m_health_override = value;
        break;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// MagScene::parseOption's PC tuning options. A failed range assert falls
// through to the store, as on the ROM.
// ROR 0x0011b964
void MagLifePC::setParam(float theValue, int id)
{
    switch (id)
    {
    case 6:
        ROR_ASSERT(theValue >= 0.0f && theValue < 1.0f);  /* 1619 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        m_dead_zone = theValue;
        break;
    case 7:
        ROR_ASSERT(theValue > 0.0f && theValue < 90.0f);  /* 1623 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        m_cos_1fc = cosf((theValue * DOG_PI) / 180.0f);
        break;
    case 8:
        ROR_ASSERT(theValue > 0.0f && theValue < 90.0f);  /* 1627 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        m_cos_200 = cosf((theValue * DOG_PI) / 180.0f);
        break;
    case 9:
        ROR_ASSERT(theValue >= 0.0f && theValue < 10.0f);  /* 1631 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        m_204 = theValue;
        break;
    case 10:
        ROR_ASSERT(theValue > 0.0f && theValue < 180.0f);  /* 1636 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        m_208 = theValue / 180.0f;
        break;
    case 11:
        ROR_ASSERT(theValue > 0.0f && theValue < 1.0f);  /* 1642 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        m_stick_threshold = theValue;
        break;
    case 12:
        ROR_ASSERT(theValue > 0.0f && theValue < 1000.0f);  /* 1646 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        m_sweep_range = theValue;
        break;
    case 13:
        ROR_ASSERT(theValue > 0.0f && theValue < 1000.0f);  /* 1650 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        m_sweep_radius = theValue;
        break;
    case 14:
        m_218 = theValue;
        break;
    default:
        ROR_ASSERT(0);  /* 1657 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        return;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Unreferenced: saves the state quad on the sub-mode stack.
// ROR 0x0011bc64
void MagLifePC::pushSubMode()
{
    ROR_ASSERT(m_submode_level >= 0 && m_submode_level < MAX_SUBMODE_STACK);  /* 1666 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    int *slot = m_submode_stack[m_submode_level];
    slot[0] = m_state;
    slot[1] = m_04;
    slot[2] = m_08;
    slot[3] = m_0c;
    m_submode_level++;
}

// ──────────────────────────────────────────────────────────────────────
// Unreferenced: the state quad back from the sub-mode stack.
// ROR 0x0011bcd8
void MagLifePC::popSubMode()
{
    ROR_ASSERT(m_submode_level > 0 && m_submode_level <= MAX_SUBMODE_STACK);  /* 1674 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    int level = m_submode_level - 1;
    m_submode_level = level;
    const int *slot = m_submode_stack[level];
    m_state = slot[0];
    m_04 = slot[1];
    m_08 = slot[2];
    m_0c = slot[3];
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 10: binds actor (NULL unbinds) and starts in state 0.
// ROR 0x0011bd4c
void MagLifePC::setActor(DogActor *actor, int no, int mode)
{
    m_actor = actor;
    m_no = no;
    m_mode = mode;
    m_move_level_prev = -1;
    m_health = calcHealth();
    if (m_scene->getFadeState() == 5)
        m_78 = 0;
    else
        m_78 = 1;
    changeState(0, 0, 0);
}

// ──────────────────────────────────────────────────────────────────────
// Running: every 11th frame costs 10 stamina.
// ROR 0x0011bdc0
void MagLifePC::drainStamina()
{
    if (m_stamina_tick < 0)
        m_stamina_tick = 0;
    int tick = m_stamina_tick + 1;
    m_stamina_tick = tick;
    if (10 < tick)
    {
        m_stamina_tick = 0;
        int stamina = m_stamina - 10;
        if (stamina < 0)
            stamina = 0;
        m_stamina = stamina;
    }
}

// ROR 0x0011be00
void MagLifePC::recoverStamina()
{
    int stamina = m_stamina + 1;
    m_stamina = stamina;
    if (1000 < stamina)
        m_stamina = 1000;
    if (m_stamina < 0)
        m_stamina = 0;
}

// ROR 0x0011be30
int MagLifePC::isBlendIn()
{
    if (m_actor)
        return m_actor->isBlendIn();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 12: after a map jump. The saved state goes, the PC is back in
// state 0 with the status words cleared, and the equipped item's actor
// (a held weapon) returns to its 'wait2' motion.
// ROR 0x0011be60
void MagLifePC::reset4MapJump()
{
    for (int i = 0; i < 4; i++)
        m_prev_state[i] = 0;
    clearGoal();
    changeState(0, 0, 0);
    m_mode_hold.m_04 = 0;
    m_mode_ride.m_04 = 0;
    m_stance_prev = 0;
    m_move_level_prev = 0;
    m_health_prev = 0;
    u_int chr = (u_int)getEquipItemChrNo();
    if (chr < 0x80)
    {
        DogActor *item = &m_scene->m_actors[chr];
        int no = item->getMotionNoBySuffix("wait2");
        item->changeMotion(0x1.11112Ep-6f, no, 1, -1, -1); /* 0x3c888897 */
    }
}

// ──────────────────────────────────────────────────────────────────────
// MagScene::MagScene builds the scene's PC (+0x2adc0). The base runs
// first; the embedded modes get their vptrs (the Attacking / Hold words
// are cleared by their own constructors, Ride's here).
// ROR 0x0011bf0c
// callcheck: MagLifeControlChara (the base constructor runs implicitly)
MagLifePC::MagLifePC()
{
    m_84 = 0;
    memset(m_submode_stack, 0, sizeof(m_submode_stack));
    m_stamina = 1000;
    m_f4 = -1;
    m_fc = -1;
    m_100 = -1;
    m_stamina_tick = 0;
    m_submode_level = 0;
    m_stance = 0;
    m_move_level = 0;
    m_health = 0;
    m_stance_prev = 0;
    m_move_level_prev = 0;
    m_health_prev = 0;
    m_pad_seen = 0;
    m_118 = 0;
    m_mode_cur = NULL;
    m_mode_ride.m_04 = 0;
    m_debug_dash = 1;
    m_weapon_override = -1;
    m_1b0 = -1;
    m_health_override = -1;
    m_target_pos[3] = 1.0f;
    m_target_pos[2] = 0.0f;
    m_target_pos[1] = 0.0f;
    m_target_pos[0] = 0.0f;
    m_1d0[3] = 1.0f;
    m_1d0[2] = 0.0f;
    m_1d0[1] = 0.0f;
    m_1d0[0] = 0.0f;
    m_dead_zone = 0x1.999998p-4f;        /* 0x3dcccccc, 0.1 */
    m_204 = 0x1.1eb85p-4f;               /* 0x3d8f5c28, 0.07 */
    m_208 = 0.9f;                        /* 0x3f666666 */
    m_stick_threshold = 0x1.0624dcp-10f; /* 0x3a83126e, 0.001 */
    m_sweep_range = 45.0f;
    m_sweep_radius = 20.0f;
    m_218 = 0.0f;
    m_weapon_type = 0;
    /* PC: the PC's motion sequence rows (0x2961e0: "base", "run_out",
     * "wait", "tired" ...) come as host data with their reader,
     * MagLifeControlChara::updateMotionSeq (mag/maglife.cpp, a stub);
     * NULL until then. */
    setMotionSeqTable(NULL);
    m_04 = -1;
    m_state = -1;
    m_cos_1fc = cosf(0x1.657186p-4f);   /* 0x3db2b8c3, 5 degrees */
    m_cos_200 = cosf(0x1.8da7e2p+0f);   /* 0x3fc6d3f1, 89 degrees */
}

// ──────────────────────────────────────────────────────────────────────
// Enters theMode with arg: the motion sequence stops, the base's slot 9
// saves the old state, the new mode's init runs. The same non-zero state
// is left alone unless forced.
// ROR 0x0011c1e8
void MagLifePC::changeState(int theMode, int arg, int force)
{
    ROR_ASSERT(theMode >= 0);  /* 2340 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    if (m_state == theMode && theMode != 0 && force == 0)
        return;
    clearMotionSeq();
    setState(theMode, arg);
    selectMode(1);
}

// ──────────────────────────────────────────────────────────────────────
// The mode object of m_state (11 and others: none), with doInit its init.
// ROR 0x0011c2ac
void MagLifePC::selectMode(int doInit)
{
    m_stick_change = 9999.0f;
    m_mode_frame = 0;
    switch (m_state)
    {
    case 0:  m_mode_cur = &m_mode_null; break;
    case 1:  m_mode_cur = &m_mode_wait; break;
    case 2:  m_mode_cur = &m_mode_walk; break;
    case 3:  m_mode_cur = &m_mode_run; break;
    case 4:  m_mode_cur = &m_mode_turn; break;
    case 5:  m_mode_cur = &m_mode_atk_wait; break;
    case 6:  m_mode_cur = &m_mode_atk_move; break;
    case 7:  m_mode_cur = &m_mode_roped; break;
    case 8:  m_mode_cur = &m_mode_hold; break;
    case 9:  m_mode_cur = &m_mode_ride; break;
    case 10: m_mode_cur = &m_mode_attacking; break;
    case 12: m_mode_cur = &m_mode_hold_attack; break;
    case 13: m_mode_cur = &m_mode_unhold; break;
    case 14: m_mode_cur = &m_mode_damage; break;
    case 15: m_mode_cur = &m_mode_dog_go; break;
    case 16: m_mode_cur = &m_mode_dog_come; break;
    case 17: m_mode_cur = &m_mode_dog_stay; break;
    case 18: m_mode_cur = &m_mode_hug; break;
    case 19: m_mode_cur = &m_mode_item; break;
    case 20: m_mode_cur = &m_mode_reload; break;
    default: m_mode_cur = NULL; break;
    }
    if (doInit && m_mode_cur)
        m_mode_cur->init(this);
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 13: the world ends -- state -1 through slot 9, no mode, full
// stamina.
// ROR 0x0011c418
void MagLifePC::termWorld()
{
    setState(-1, 0);
    m_mode_cur = NULL;
    m_stamina = 1000;
}
