/* ==========================================================================
 *  emy/emy_clr.cpp
 *
 *  ClrMain (enemy type 8) and its outs (0x14dc7c..0x150b24). The usual
 *  enemy loop: decide picks an input of the transition table (only inputs
 *  the current state's row has; the last true test wins, as MrtMain), exec
 *  moves the state graph by it, after switches outs on a change and runs
 *  the current one. A hit (notify bit 1) restarts the current state and
 *  counts in m_hit_count; a kill (bit 2) forces state 4. after also ticks
 *  the hit points the spit leaves.
 * ======================================================================== */
#include "emy/emy_clr.h"

#include <math.h>
#include <string.h>

#include "common/rosepan_stub.h"

#include "dog/dogactor.h"
#include "eff/effman.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "mag/magscenecmd.h"
#include "mag/magsystem.h"
#include "sequence/sequence.h"
#include "snd/sndman.h"
#include "task/tsk_chr.h"
#include "task/tsk_man.h"
#include "task/tsk_scene.h"
#include "utl/utlrand.h"

/* The heights a ClrOut* climbs to / drops to (y grows downward). */
static const float CLR_TOP_Y    = -0x1.5b5892p+8f;  /* 0xc3adac49, -347.346 */
static const float CLR_FLOOR1_Y = -0x1.a53950p+5f;  /* 0xc2529ca8, -52.653 */
static const float CLR_FLOOR2_Y = -0x1.856248p+6f;  /* 0xc2c2b124, -97.346 */

/* The oid of the object a ClrOut* drags along with motion 2 (the
 * script's global 0x1e2). */
static u_int clrPropOid()
{
    return (u_int)g_sequence.m_global[0x1e2];
}

/* The prop's motion 2. The ROM also passes 32769 in f13 (unread); the
 * name register is unset in ClrOutUp (NULL here) and NULL in ClrOutMove. */
static void clrPropMotion(u_int prop)
{
    chrSetMotionByOid(0x1.111110p-6f /* 0x3c888888, 1 / 60 */, prop, 2, -1, 0, NULL);
}

static u_int clrPCOid()
{
    ObjStat *pc = g_gamMan.m_pcPtr;
    return pc != NULL ? pc->m_oid : 0;
}

static u_int clrDogOid()
{
    ObjStat *dog = g_gamMan.m_dogPtr;
    return dog != NULL ? dog->m_oid : 0;
}

/* The outs' decide off / on with the achr's flag 0x400 (the ROM inlines
 * the pair at every use). */
static void clrHold(EmyOut *out)
{
    out->m_emy->setDecideSw(0);
    g_gamMan.setObj(out->m_oid, 0x400);
}

static void clrRelease(EmyOut *out)
{
    out->m_emy->setDecideSw(1);
    g_gamMan.resetObj(out->m_oid, 0x400);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014dc7c // callcheck: operator_new, EmyOut (new ClrOutXxx: the inline constructors call EmyOut::EmyOut)
// States 3 and 5 get one ClrOutUp each.

void ClrMain::initOut()
{
    m_motion_list[2] = 6;
    m_motion_list[9] = 0xc;
    m_phase          = 5;
    m_motion_list[1] = 5;
    m_motion_list[8] = 0xc;
    m_motion_list[7] = 0x12;
    m_height         = 1;
    m_pc_seen        = 0;
    m_244            = 0;
    m_motion_list[0] = 1;
    m_state_flg[0]   = 0;
    m_state_flg[1]   = 1;
    m_state_flg[2]   = 1;
    m_state_flg[3]   = 1;
    m_state_flg[4]   = 1;
    m_state_flg[5]   = 1;
    m_wait = utlRandRange(90, 210);
    m_param->getPos(m_base_pos);
    m_base_pos[1] = CLR_FLOOR1_Y;
    m_param->setPos(m_base_pos);

    m_out[1] = new ClrOutMove(m_oid, m_param, this);
    m_out[2] = new ClrOutAtk(m_oid, m_param, this);
    m_out[3] = new ClrOutUp(m_oid, m_param, this);
    m_out[4] = new ClrOutDeathUp(m_oid, m_param, this);
    m_out[5] = new ClrOutUp(m_oid, m_param, this);
    m_hit_next = 0;
    for (int i = 0; i < 64; i++)
        m_hit[i].m_used = 0;
    m_hit_count = 0;
    m_hold      = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014de54 // callcheck: ~EmyMain, operator_delete (the base destructor and the deleting variant are the compiler's)

ClrMain::~ClrMain()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014deb4 // callcheck: testInput13, testInput12, testInput11, testInput10, testInput9, isPCInSight, testInput3, testInput4, testInput5, testInput6, testInput7, testInput8
// Inputs 13..9, then the sight edge: coming into sight is input 0 or 1,
// leaving it input 2, staying in sight runs tests 3..8. Every test the
// row has runs; the last true one wins.

void ClrMain::decide()
{
    if (m_decide_sw == 0)
        return;
    if (m_input_flg_motion == 0)
        return;
    if (m_req != -1)
        return;

    if (m_input[13] && testInput13())
        m_req = 0xd;
    if (m_input[12] && testInput12())
        m_req = 0xc;
    if (m_input[11] && testInput11())
        m_req = 0xb;
    if (m_input[10] && testInput10())
        m_req = 0xa;
    if (m_input[9] && testInput9())
        m_req = 9;

    u_char seen = (u_char)isPCInSight();
    if (m_pc_seen == 0) {
        m_pc_seen = seen;
        if (seen == 0)
            return;
        if (m_input[0])
            m_req = 0;
        if (m_input[1])
            m_req = 1;
        return;
    }
    m_pc_seen = seen;
    if (seen == 0) {
        if (m_input[2])
            m_req = 2;
        return;
    }
    if (m_input[3] && testInput3())
        m_req = 3;
    if (m_input[4] && testInput4())
        m_req = 4;
    if (m_input[5] && testInput5())
        m_req = 5;
    if (m_input[6] && testInput6())
        m_req = 6;
    if (m_input[7] && testInput7())
        m_req = 7;
    if (m_input[8] && testInput8())
        m_req = 8;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e0b4
// Notify bit 2 (killed) forces state 4 (not through the graph); bit 1
// (hit) with no input restarts the current state and counts the hit; a
// state out of 1..5 counts as no move.

void ClrMain::exec()
{
    m_changed = 0;
    if (m_notify & 2) {
        m_changed = 1;
        m_state   = 4;
    } else if (m_req == -1) {
        if (m_notify & 1) {
            m_map->m_cur = m_state;
            m_changed    = 1;
            m_hit_count++;
        }
    } else {
        int state = m_map->next(m_req);
        if (state >= 6 || state < 0)
            state = 0;
        if (state != 0) {
            m_state     = state;
            m_changed   = 1;
            m_hit_count = 0;
            m_pc_seen   = 0;
        }
    }
    m_notify = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e16c

void ClrMain::after()
{
    int no = g_gamMan.getActorNo(m_oid);
    g_magSystem.m_scene.m_actor_flags[(u_short)no] &= ~0x60u;
    updateHitPoints();
    EmyOut *out = m_emy_out;
    if (m_changed) {
        out->Term();
        selectOut();
        out = m_emy_out;
    }
    m_req = out->Exec();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e224
// The ROM returns 0 (the slot's value is never read).

void ClrMain::selectOut()
{
    if (m_state >= 1 && m_state <= 5) {
        m_emy_out = m_out[m_state];
        m_emy_out->Init();
    }
    int n = m_trans->m_inputs;
    const char *row = m_trans->m_table + m_state * n;
    // PORT: the ROM writes as many words as the table has inputs; the
    // host array holds 14 (EE 0x208..0x240).
    if (n > 14)
        n = 14;
    for (int i = 0; i < n; i++)
        m_input[i] = row[i] != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e2ec

int ClrMain::testInput3()
{
    return EmyMain::isPCStateClass1();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e300

int ClrMain::testInput4()
{
    return EmyMain::isPCStateClass1();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e314

int ClrMain::testInput5()
{
    return EmyMain::isPCAhead();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e328

int ClrMain::testInput6()
{
    return EmyMain::isPCAhead();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e33c

int ClrMain::testInput7()
{
    return EmyMain::isPCClass7OffAxis();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e350

int ClrMain::testInput8()
{
    return EmyMain::isPCClass7OffAxis();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e364

int ClrMain::testInput13()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e36c

int ClrMain::testInput12()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e374

int ClrMain::testInput11()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e37c

int ClrMain::testInput10()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e384

int ClrMain::testInput9()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e38c
// The range is 0 in the ROM (f12 cleared).

int ClrMain::isPCInSight()
{
    return emyChkInSight(0.0f, 0x1.2d97c6p+1f /* 0x4016cbe3, 3 pi / 4 */, 1, clrPCOid(), m_param);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e3d0

int ClrMain::addHitPoint(float radius, const float *pos, int wait, int life, int loud)
{
    for (int pass = 0; pass < 2; pass++) {
        int lo = pass == 0 ? m_hit_next : 0;
        int hi = pass == 0 ? 64 : m_hit_next;
        for (int i = lo; i < hi; i++) {
            ClrHitPoint &hp = m_hit[i];
            if (hp.m_used)
                continue;
            hp.m_used   = 1;
            hp.m_wait   = wait;
            hp.m_life   = life;
            hp.m_loud   = loud;
            hp.m_radius = radius;
            hp.m_pos[0] = pos[0];
            hp.m_pos[1] = pos[1];
            hp.m_pos[2] = pos[2];
            m_hit_next = i + 1;
            if (m_hit_next > 63)
                m_hit_next = 0;
            return i;
        }
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e4d4 // callcheck: playSE (GamSndMan's, by its overload name)
// A record's SE plays the frame its wait reaches 1. Once the wait is
// over the life counts down (the record frees at 0) and the PC, then the
// dog, standing in it frees it; the dog's hit replaces the PC's bit
// (the ROM stores 2, not | 2). Then chr word 0x32 on each one hit.

u_int ClrMain::updateHitPoints()
{
    u_int hit = 0;
    u_int pc  = clrPCOid();
    u_int dog = clrDogOid();
    for (int i = 0; i < 64; i++) {
        ClrHitPoint &hp = m_hit[i];
        if (!hp.m_used)
            continue;
        if (hp.m_wait == 1) {
            if (hp.m_loud != 0 || i % 20 == 0)
                g_gamMan.m_sndMan.playSE(0x1f010594, false, NULL, 0);
        }
        if (hp.m_wait >= 1) {
            hp.m_wait--;
            continue;
        }
        if (hp.m_life < 1)
            hp.m_used = 0;
        else
            hp.m_life--;
        if (emyChkPointReach(hp.m_radius, hp.m_pos, 1, pc) == 1) {
            hit = 1;
            hp.m_used = 0;
        }
        if (emyChkPointReach(hp.m_radius, hp.m_pos, 1, dog) == 1) {
            hit = 2;
            hp.m_used = 0;
        }
    }
    if (hit & 1)
        scnChrRequest(0xb, m_oid, 0, 0, 0x32, pc, 0);
    if (hit & 2)
        scnChrRequest(0xb, m_oid, 0, 0, 0x32, dog, 0);
    return hit;
}

// ──── ClrOutUp ────────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e73c

void ClrOutUp::Init()
{
    m_prop = clrPropOid();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e750 // callcheck: setDecideSw, setObj (clrHold / clrRelease / clrDeathFlags / clrPropMotion: the pairs the ROM inlines)
// Phase 5 starts the climb with decide off, 6 restarts it, 7 runs it;
// once out of sight phase 0 (a Move next), a new wait, input 0xd.

int ClrOutUp::Exec()
{
    ClrMain *emy = static_cast<ClrMain *>(m_emy);
    switch (emy->m_phase) {
    case 5:
        emy->m_phase = 6;
        clrHold(this);
        /* fall through */
    case 6:
        execUp(1);
        emy->m_phase = 7;
        return -1;
    case 7:
        if (execUp(0) != 1)
            return -1;
        emy->m_phase = 0;
        emy->m_wait  = utlRandRange(30, 150);
        return 0xd;
    default:
        return -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e834

void ClrOutUp::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014e850 // callcheck: chrSetMotionByOid, setDecideSw, setObj (clrHold / clrRelease / clrDeathFlags / clrPropMotion: the pairs the ROM inlines); playSE (GamSndMan's, by its overload name)
// Step 1 climbs 4 a frame with motion 10 (looping), the prop's motion
// every 4th frame, SE 0x1f010596 every 20th; at the top it parks at
// (-800, top, -800), plays motion 1 and reports done the same frame, so
// step 2 (120 frames, the prop every 3rd) and 3 do not run from Exec
// (ROM). A hit while climbing keeps decide off.

bool ClrOutUp::execUp(int start)
{
    bool done = false;
    clrHold(this);
    if (start == 1)
        m_up_step = 0;
    switch (m_up_step) {
    case 0:
        m_up_step  = 1;
        m_hit_seen = 0;
        m_90       = 0;
        setMotion(10, 0x100);
        m_count = 0;
        /* fall through */
    case 1:
        if (g_gamMan.isAchrMotionEnd(m_oid)) {
            m_emy->m_input_flg_motion = 1;
            setMotion(10, 0x100);
        }
        m_achr->getPos(m_pos);
        m_count += 1.0f;
        if (((int)m_count & 3) == 0)
            clrPropMotion(m_prop);
        if ((int)m_count % 20 == 0)
            g_gamMan.m_sndMan.playSE(0x1f010596, false, NULL, 0);
        m_pos[1] -= 4.0f;
        if (m_pos[1] <= CLR_TOP_Y) {
            done = true;
            m_pos[0] = -800.0f;
            m_pos[1] = CLR_TOP_Y;
            m_pos[2] = -800.0f;
            setMotion(1, 0x100);
            m_up_step = 2;
            m_count   = 0;
        }
        m_achr->setPos(m_pos);
        break;
    case 2:
        if ((int)m_count % 3 == 0)
            clrPropMotion(m_prop);
        if ((int)m_count > 120)
            m_up_step = 3;
        m_count += 1.0f;
        break;
    case 3:
        m_up_step = 9999;
        break;
    case 9999:
        done = true;
        break;
    default:
        break;
    }
    if (m_hit_seen == 1)
        clrHold(this);
    if (m_hit_seen == 0 && (m_emy->m_notify & 1))
        m_hit_seen = 1;
    return done;
}

// ──── ClrOutDeathUp ───────────────────────────────────────────────────

/* The achr leaves the hit tests: flag 0x40 off, 0x10000000 and 0x400 on. */
static void clrDeathFlags(EmyOut *out)
{
    g_gamMan.resetObj(out->m_oid, 0x40);
    g_gamMan.setObj(out->m_oid, 0x10000000);
    g_gamMan.setObj(out->m_oid, 0x400);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014eb54 // callcheck: resetObj, setObj (clrHold / clrRelease / clrDeathFlags / clrPropMotion: the pairs the ROM inlines)

void ClrOutDeathUp::Init()
{
    ClrMain *emy = static_cast<ClrMain *>(m_emy);
    clrDeathFlags(this);
    if (emy->m_phase == 5)
        emy->m_phase = 6;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014ebcc // callcheck: resetObj, setObj (clrHold / clrRelease / clrDeathFlags / clrPropMotion: the pairs the ROM inlines)
// As ClrOutUp's phases 5..7, but it hands back nothing when out of sight.

int ClrOutDeathUp::Exec()
{
    ClrMain *emy = static_cast<ClrMain *>(m_emy);
    switch (emy->m_phase) {
    case 5:
        clrDeathFlags(this);
        emy->m_phase = 6;
        break;
    case 6:
        execUp(1);
        emy->m_phase = 7;
        break;
    case 7:
        clrDeathFlags(this);
        if (execUp(0) != 1)
            return -1;
        emy->m_phase = 0;
        emy->m_wait  = utlRandRange(30, 150);
        break;
    default:
        break;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014ecf4

void ClrOutDeathUp::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014ed10 // callcheck: setDecideSw, setObj (clrHold / clrRelease / clrDeathFlags / clrPropMotion: the pairs the ROM inlines)
// Climbs 2 a frame with motion 10; at the top motion 1, done.

bool ClrOutDeathUp::execUp(int start)
{
    clrHold(this);
    if (start == 1)
        m_up_step = 0;
    switch (m_up_step) {
    case 0:
        m_up_step = 1;
        m_90      = 0;
        setMotion(10, 0x100);
        break;
    case 1:
        break;
    case 2:
        m_up_step = 9999;
        return true;
    case 9999:
        return true;
    default:
        return false;
    }
    m_achr->getPos(m_pos);
    float y = m_pos[1] - 2.0f;
    m_pos[1] = y;
    if (y <= CLR_TOP_Y) {
        m_pos[1] = CLR_TOP_Y;
        setMotion(1, 0x100);
        m_up_step = 2;
    }
    m_achr->setPos(m_pos);
    return y <= CLR_TOP_Y;
}

// ──── ClrOutMove ──────────────────────────────────────────────────────

/* The voices the wait plays (EmyOut::tickVoice / playSE numbers). The ROM
 * picks utlRandRange(0, 3) of the second table, one past its three words:
 * the fourth is the next word of .data (0x2ef8bc). */
static const int s_clrWaitVoice[6] = { 0x3e, 0x3f, 0x40, 0x4c, 0x47, 0x4e }; /* 0x2ef898 */
static const int s_clrSpitVoice[4] = { 0x41, 0x42, 0x43, 0x44 };             /* 0x2ef8b0 */

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014ee40 // callcheck: resetObj, setDecideSw (clrHold / clrRelease / clrDeathFlags / clrPropMotion: the pairs the ROM inlines)

void ClrOutMove::Init()
{
    clrRelease(this);
    selectTarget(100);
    m_prop = clrPropOid();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014ee9c // callcheck: resetObj, setDecideSw (clrHold / clrRelease / clrDeathFlags / clrPropMotion: the pairs the ROM inlines); getTargetNodePos (its result is overwritten by the PC's position, ROM)
// Phase 0: a spot -- half the time anywhere in +-340, else within 60 of
// the PC (clamped to +-340) -- is set at once (it is hanging out of
// sight), the achr faces the PC, a 60..150 frame wait; 1 the wait with
// its voice (the spit voice at 10 left), then the spit; 2 ten frames;
// 3 / 4 the drop to height 1; 5 its voice stopped, input 0xd. Phase 8
// ends at once.

int ClrOutMove::Exec()
{
    ClrMain *emy = static_cast<ClrMain *>(m_emy);
    switch (emy->m_phase) {
    case 0:
        clrRelease(this);
        emy->m_hold = 1;
        if (utlRandRange(0, 100) < 0x32) {
            m_achr->getPos(m_pos);
            m_pos[0] = utlRandRangeF(-340.0f, 340.0f);
            m_pos[2] = utlRandRangeF(-340.0f, 340.0f);
        } else {
            float pc[4];
            getTargetNodePos(pc);
            g_gamMan.m_pcPtr->getPos(pc);
            m_achr->getPos(m_pos);
            m_pos[0] = pc[0] + utlRandRangeF(-60.0f, 60.0f);
            m_pos[2] = pc[2] + utlRandRangeF(-60.0f, 60.0f);
            if (m_pos[0] < -340.0f)
                m_pos[0] = -340.0f;
            else if (m_pos[0] > 340.0f)
                m_pos[0] = 340.0f;
            if (m_pos[2] < -340.0f)
                m_pos[2] = -340.0f;
            else if (m_pos[2] > 340.0f)
                m_pos[2] = 340.0f;
        }
        m_achr->setPos(m_pos);
        m_rot[1] = 0;
        m_achr->setRot(m_rot);
        turnToPC(1);
        m_wait = utlRandRange(60, 150);
        emy->m_phase = 1;
        return -1;
    case 1:
        m_wait--;
        if (m_wait == 10) {
            stopVoice();
            m_voice = playSE(s_clrSpitVoice[utlRandRange(0, 3)]);
        } else if (m_wait >= 11) {
            tickVoice(4, 20, s_clrWaitVoice[utlRandRange(0, 5)]);
        }
        if (m_wait >= 0)
            return -1;
        emy->m_phase = 2;
        m_count = 10.0f;
        createDropEff();
        return -1;
    case 2:
        m_count -= 1.0f;
        if (m_count < 0.0f) {
            m_count = 0;
            emy->m_phase  = 3;
            emy->m_height = 1;
        }
        return -1;
    case 3:
        execDown(1);
        emy->m_phase = 4;
        return -1;
    case 4:
        if (execDown(0) != 1)
            return -1;
        emy->m_phase = 5;
        return -1;
    case 5:
        g_sndMan.stopSe(m_voice);
        return 0xd;
    case 8:
        emy->m_phase = 5;
        return 0xd;
    default:
        return -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014f220

void ClrOutMove::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014f23c // callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (the double wrap is emyWrapAngle)

int ClrOutMove::turnToPC(int arg)
{
    (void)arg;
    float pc[4], pos[4], rot[4];
    g_gamMan.m_pcPtr->getPos(pc);
    m_achr->getPos(pos);
    float a = atan2f(pc[0] - pos[0], pc[2] - pos[2]);
    m_achr->getRot(rot);
    rot[1] = emyWrapAngle(a - rot[1]);
    m_achr->setRot(rot);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014f360 // callcheck: chrSetMotionByOid (clrHold / clrRelease / clrDeathFlags / clrPropMotion: the pairs the ROM inlines); playSE (GamSndMan's, by its overload name)
// Step 0 picks the floor (m_height), notes the start and starts the
// prop; 1 falls (y = start + (0.8 t)^2 + t + 1), the prop's motion every
// 3rd frame, SE 0x1f010597 every 10th, hitting the PC (chr word 0x31,
// SE 0x1f0105ac) and the dog within 60 once each; at the floor motion 9;
// 2 once it ends motion 1, done.

bool ClrOutMove::execDown(int start)
{
    ClrMain *emy = static_cast<ClrMain *>(m_emy);
    if (start == 1)
        m_down_step = 0;
    switch (m_down_step) {
    case 0:
        m_90        = 0;
        m_down_step = 1;
        switch (emy->m_height) {
        case 0: m_floor_y = 0;            break;
        case 1: m_floor_y = CLR_FLOOR1_Y; break;
        case 2: m_floor_y = CLR_FLOOR2_Y; break;
        default: break;
        }
        emy->m_268 = 4;
        m_achr->getPos(m_start);
        m_count   = 0;
        m_hit_pc  = 0;
        m_hit_dog = 0;
        clrPropMotion(m_prop);
        return false;
    case 1:
    {
        m_achr->getPos(m_pos);
        float t = m_count;
        float k = t * 0x1.999998p-1f; /* 0x3f4ccccc, 0.8 */
        m_count = t + 1.0f;
        m_pos[1] = m_start[1] + k * k + t + 1.0f;
        if ((int)m_count % 3 == 0)
            clrPropMotion(m_prop);
        if ((int)m_count % 10 == 0)
            g_gamMan.m_sndMan.playSE(0x1f010597, false, NULL, 0);
        if (m_floor_y <= m_pos[1]) {
            m_pos[1] = m_floor_y;
            setMotion(9, 0);
            m_down_step = 2;
            m_count     = 0;
        }
        m_achr->setPos(m_pos);
        if (m_hit_pc == 0) {
            u_int pc = clrPCOid();
            if (chrIsNear(60.0f, m_oid, pc)) {
                scnChrRequest(0xb, m_oid, 0, 0, 0x31, pc, 0);
                m_hit_pc = 1;
                g_gamMan.m_sndMan.playSE(0x1f0105ac, false, NULL, 0);
            }
        }
        if (m_hit_dog == 0) {
            u_int dog = clrDogOid();
            if (chrIsNear(60.0f, m_oid, dog)) {
                scnChrRequest(0xb, m_oid, 0, 0, 0x31, dog, 0);
                m_hit_dog = 1;
            }
        }
        return false;
    }
    case 2:
        if (!g_gamMan.isAchrMotionEnd(m_oid))
            return false;
        m_emy->m_input_flg_motion = 1;
        m_hit_pc    = 0;
        m_down_step = 9999;
        setMotion(1, 0x100);
        return true;
    case 9999:
        return true;
    default:
        return false;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014f72c // callcheck: memset (the first parameter block is cleared, the second is not: EffParam eff = {} here)
// Effect 0xa rising from the spot, effect 0xc on the ground 10 above it
// (20 frames late, 100 long) and its hit point: radius 25, live after
// 20 frames for 100, loud.

void ClrOutMove::createDropEff()
{
    m_eff_pos[0] = m_pos[0];
    m_eff_pos[1] = m_pos[1];
    m_eff_pos[2] = m_pos[2];
    m_eff_dir[1] = 1.0f;
    m_eff_pos[3] = 0;
    m_eff_dir[0] = 0;
    m_eff_dir[2] = 0;
    m_eff_dir[3] = 0;

    EffParam breath;
    memset(&breath, 0, sizeof(breath));
    breath.m_kind = 0xa;
    breath.m_08   = 0;
    breath.m_breath.m_pos = m_eff_pos;
    breath.m_breath.m_dir = m_eff_dir;
    breath.m_breath.m_scale = 0.5f;
    breath.m_breath.m_drag  = 1.0f;
    breath.m_breath.m_20    = 1.0f;
    for (int i = 0; i < 4; i++)
        breath.m_breath.m_color[i] = 1.0f;
    breath.m_breath.m_frames = 8;
    g_magSystem.m_scene.m_eff_man.createEff(&breath);

    EffParam ground = {};       // PORT: the ROM leaves the words it does not set as they were
    ground.m_kind = 0xc;
    ground.m_08   = 0x14;
    ground.m_ground.m_pos[0] = m_pos[0];
    ground.m_ground.m_pos[1] = -10.0f;
    ground.m_ground.m_pos[2] = m_pos[2];
    ground.m_ground.m_size     = 100.0f;
    ground.m_ground.m_color[0] = 1.0f;
    ground.m_ground.m_color[1] = 1.0f;
    ground.m_ground.m_color[2] = 1.0f;
    ground.m_ground.m_color[3] = 0.5f;
    ground.m_ground.m_frames   = 100;
    g_magSystem.m_scene.m_eff_man.createEff(&ground);
    static_cast<ClrMain *>(m_emy)->addHitPoint(25.0f, ground.m_ground.m_pos, ground.m_08,
                                               ground.m_ground.m_frames, 1);
}

// ──── ClrOutAtk ───────────────────────────────────────────────────────

/* emitSpit's frame of nine (0..8), shared by every Clr (data 0x298dd0). */
static int s_clrSpitFrame = 0;

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014f88c // callcheck: resetObj, setDecideSw (clrHold / clrRelease / clrDeathFlags / clrPropMotion: the pairs the ROM inlines)

void ClrOutAtk::Init()
{
    selectTarget(100);
    selectAttack(1);
    clrRelease(this);
    m_last = -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014f8e8 // callcheck: setDecideSw, fptodp, dpcmp, dpadd, dpsub, dptofp (clrHold / clrRelease inline the decide pair; the double wrap is emyWrapAngle)
// Step 1 (attack 0x14) and 4 wait for the TskAttack (m_task[3]) to end,
// 2 (0x12 / 0x13) counts 10 frames before decide comes back, a hit
// there picks again; 3 spits while the task runs for m_time frames;
// 5 the arm swing: turns m_turn a frame and hits what the arm touches
// in frames 11..28 (chr word 0x33, its SE 7). Being hit holds decide in
// steps 1, 3 and 4. When a step ends, the next attack.

int ClrOutAtk::Exec()
{
    ClrMain *emy = static_cast<ClrMain *>(m_emy);
    int ret = -1;
    switch (m_step) {
    case 1:
        if (emy->m_hit_count > 0)
            clrHold(this);
        if (g_taskMan.getTask(m_achr->m_task[3]) != 0)
            goto done;
        clrHold(this);
        break;
    case 2:
        m_count--;
        if (m_count < 1) {
            m_step  = 3;
            m_count = 0;
            clrRelease(this);
            g_gamMan.m_sndMan.playSE(0x1f0105a7, false, NULL, 0);
        }
        if (g_taskMan.getTask(m_achr->m_task[3]) == 0) {
            clrHold(this);
            ret = selectAttack(0);
        }
        if (emy->m_notify & 1) {
            clrHold(this);
            ret = selectAttack(0);
            m_last = m_attack;
        }
        goto done;
    case 3:
        if (emy->m_hit_count > 0)
            clrHold(this);
        m_time--;
        if (m_time < 1)
            m_step = 4;
        if (g_taskMan.getTask(m_achr->m_task[3]) != 0) {
            m_count++;
            emitSpit();
            goto done;
        }
        clrHold(this);
        break;
    case 4:
        if (emy->m_hit_count > 0)
            clrHold(this);
        if (g_taskMan.getTask(m_achr->m_task[3]) != 0) {
            if (!g_gamMan.isAchrMotionEnd(m_oid))
                goto done;
            m_emy->m_input_flg_motion = 1;
        }
        break;
    case 5:
    {
        int no = g_gamMan.getActorNo(m_oid);
        float rot[4];
        rot[0] = 0;
        rot[2] = 0;
        rot[3] = 0;             // PORT: the ROM leaves w as the stack had it
        rot[1] = emyWrapAngle(g_magSystem.m_scene.m_actors[no].getRotY() + m_turn);
        m_achr->setRot(rot);
        m_frame++;
        clrHold(this);
        if (m_frame > 10 && m_frame < 0x1d) {
            if (m_hit_pc == 0 && isArmTouching(clrPCOid())) {
                m_hit_pc = 1;
                playSE(7);
                scnChrRequest(0xb, m_oid, 0, 0, 0x33, clrPCOid(), 0);
            }
            if (m_hit_dog == 0 && isArmTouching(clrDogOid())) {
                m_hit_pc = 1;           // ROM: the PC's flag, so the dog is hit every frame
                playSE(7);
                scnChrRequest(0xb, m_oid, 0, 0, 0x33, clrDogOid(), 0);
            }
        }
        if (!g_gamMan.isAchrMotionEnd(m_oid))
            goto done;
        m_emy->m_input_flg_motion = 1;
        clrRelease(this);
        g_gamMan.setObj(m_oid, 0x40);
        g_gamMan.resetObj(m_oid, 0x10000000);
        g_gamMan.resetObj(m_oid, 0x400);
        break;
    }
    default:
        goto done;
    }
    ret = selectAttack(0);
done:
    execExtra(1);
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00150a24

void ClrOutAtk::Term()
{
    termCommon();
}

/* A TskAttack of kind on the target (motion, word 7, the float word 10;
 * words 8 / 9 are 1 / 5). PORT: the ROM leaves words 1, 3..5 as the
 * stack had them; zero here. */
static void clrStartAttack(ClrOutAtk *out, int kind, int motion, int w7, float f10)
{
    int prm[11] = {};
    prm[0] = (int)out->m_target;
    prm[2] = kind;
    prm[6] = motion;
    prm[7] = w7;
    prm[8] = 1;
    prm[9] = 5;
    memcpy(&prm[10], &f10, sizeof(float));
    scnAttackStart(out->m_oid, prm);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014febc // callcheck: resetObj, setDecideSw, setObj, FUN_001f9d90, playSE (clrHold / clrRelease / clrDeathFlags inline the pairs; FUN_001f9d90 is scnAttackStart through clrStartAttack; playSE is GamSndMan's overload)
// Gives up (input 0xd) with a chance by the hits taken: 2+ 90 %, 1 70 %,
// none 40 % (never on Init's pick). Else 0x14 (40 %), 0x12 (40 %) or
// 0x13 (20 %), 0x14 again after any other; hurt, 80 % of the time the
// arm swing toward the PC's side instead (3 left, 4 right) when it is
// not ahead.

int ClrOutAtk::selectAttack(int first)
{
    ClrMain *emy = static_cast<ClrMain *>(m_emy);
    m_hit_dog = 0;
    m_hit_pc  = 0;
    int chance;
    bool roll = true;
    if (emy->m_hit_count >= 2)
        chance = 90;
    else if (emy->m_hit_count != 0)
        chance = 70;
    else {
        chance = 40;
        roll   = first == 0;
    }
    if (roll && utlRandRange(0, 99) < chance) {
        clrHold(this);
        m_last = -1;
        return 0xd;
    }

    int r = utlRandRange(0, 99);
    int a = 0x14;
    if (r > 0x27) {
        a = 0x12;
        if (r > 0x4f)
            a = 0x13;
    }
    if (m_last >= 0 && m_last != 0x14)
        a = 0x14;
    if (emy->m_damage != 0 && utlRandRange(0, 99) < 0x50) {
        int side = getPCSide();
        if (side == 1)
            a = 3;
        if (side == 2)
            a = 4;
    }
    m_attack = a;
    switch (a) {
    case 3:
        m_step  = 5;
        m_right = 1;
        setMotion(a, 0);
        m_turn = -0x1.85f032p-4f;   /* 0xbdc2f819, -0.0952 */
        clrDeathFlags(this);
        break;
    case 4:
        m_right = 0;
        m_step  = 5;
        setMotion(a, 0);
        m_turn = 0x1.85f032p-4f;    /* 0x3dc2f819, 0.0952 */
        clrDeathFlags(this);
        break;
    case 0x12:
        clrStartAttack(this, 0x2f, 0x12, 0xf, 25.0f);
        m_count = 10;
        m_step  = 2;
        m_time  = 120;
        clrHold(this);
        break;
    case 0x13:
        clrStartAttack(this, 0x30, 0x13, 0xf, 25.0f);
        m_count = 10;
        m_step  = 2;
        m_time  = 300;
        clrHold(this);
        break;
    case 0x14:
        clrRelease(this);
        clrStartAttack(this, 0x2e, 0x14, 10, 80.0f);
        g_gamMan.m_sndMan.playSE(0x1f0105ac, false, NULL, 0);
        m_count = 0;
        m_step  = 1;
        break;
    }
    clrRelease(this);
    m_last = m_attack;
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00150290 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName, memset (DogModel::getNodeIndexByName inline; the kind 0xc block is not cleared in the ROM)
// Each call moves the spit frame on (0..8). Node p_eff's position and z
// axis go to that frame's record, effect 0xa reads them (created on
// frames 0 and 4); on frames 1, 4 and 7 effect 0xc lands under node
// p_gero3 (y -3, or -65 inside x -380..-190, z -15..15) 10 frames late,
// with its hit point (radius 80, live 200 frames).

void ClrOutAtk::emitSpit()
{
    int no = m_achr->m_actor;
    if (no < 0)
        return;
    if (s_clrSpitFrame < 8)
        s_clrSpitFrame++;
    else
        s_clrSpitFrame = 0;
    int k = s_clrSpitFrame;
    DogActor &actor = g_magSystem.m_scene.m_actors[no];

    int node = actor.getNodeIndexByName("p_eff");
    if (node >= 0) {
        sceVu0FMATRIX m;
        actor.getNodeWorldMatrix(m, node);
        for (int i = 0; i < 4; i++) {
            m_eff_pos[k][i] = m[3][i];
            m_eff_dir[k][i] = m[2][i];
        }
        EffParam breath;
        memset(&breath, 0, sizeof(breath));
        breath.m_kind = 0xa;
        breath.m_08   = 0;
        breath.m_breath.m_pos = m_eff_pos[k];
        breath.m_breath.m_dir = m_eff_dir[k];
        breath.m_breath.m_scale = 0.5f;
        breath.m_breath.m_drag  = 1.0f;
        breath.m_breath.m_20    = 1.0f;
        for (int i = 0; i < 4; i++)
            breath.m_breath.m_color[i] = 1.0f;
        breath.m_breath.m_frames = 8;
        if (k == 0 || k == 4)
            g_magSystem.m_scene.m_eff_man.createEff(&breath);
    }
    if (k != 1 && k != 4 && k != 7)
        return;
    node = actor.getNodeIndexByName("p_gero3");
    if (node < 0)
        return;
    sceVu0FMATRIX m;
    actor.getNodeWorldMatrix(m, node);
    float x = m[3][0];
    float z = m[3][2];
    float y = -3.0f;
    if (-380.0f < x && x < -190.0f && -15.0f < z && z < 15.0f)
        y = -65.0f;
    EffParam ground = {};       // PORT: the ROM leaves the words it does not set as they were
    ground.m_kind = 0xc;
    ground.m_08   = 10;
    ground.m_ground.m_pos[0] = x;
    ground.m_ground.m_pos[1] = y;
    ground.m_ground.m_pos[2] = z;
    ground.m_ground.m_size     = 100.0f;
    ground.m_ground.m_color[0] = 1.0f;
    ground.m_ground.m_color[1] = 1.0f;
    ground.m_ground.m_color[2] = 1.0f;
    ground.m_ground.m_color[3] = 0.5f;
    ground.m_ground.m_frames   = 200;
    g_magSystem.m_scene.m_eff_man.createEff(&ground);
    static_cast<ClrMain *>(m_emy)->addHitPoint(80.0f, ground.m_ground.m_pos, ground.m_08,
                                               ground.m_ground.m_frames, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001506a0 // callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (the soft-float doubles, C doubles here)

int ClrOutAtk::getPCSide()
{
    const double third = 1.0471975511965976;   /* 0x3ff0c152382d7365, pi / 3 */
    int no = g_gamMan.getActorNo(m_oid);
    float base[4], pc[4];
    getBasePos(base);
    emyGetPCPos(pc);
    float a = atan2f(pc[0] - base[0], pc[2] - base[2]);
    a = emyWrapAngle(a - g_magSystem.m_scene.m_actors[no].getRotY());
    if ((double)a < third && (double)a > -third)
        return 0;
    return a <= 0.0f ? 1 : 2;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00150804 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline: emyGetNodePos)

bool ClrOutAtk::isArmTouching(u_int target)
{
    float spine[4], arm[4];
    emyGetNodePos(g_gamMan.getActorNo(target), "Bip01 Spine1", spine);
    float r = g_gamMan.getObjRadius(target, NULL);
    emyGetNodePos(g_gamMan.getActorNo(m_oid), m_right ? "nodel_p_arm_r" : "nodel_p_arm_l", arm);
    return emyChkNearXZY(r + 32.0f, arm, spine) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00150a1c

int ClrOutAtk::execExtra(int arg)
{
    (void)arg;
    return 0;
}

// ──── ClrOutWin ───────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00150a40

void ClrOutWin::Init()
{
    m_range = utlRandRangeF(100.0f, 200.0f);
    emyStartChase(10.0f, this, m_target);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00150a90 // callcheck: stop (TskEnemyMove::stop on the enemy's move task)

int ClrOutWin::Exec()
{
    if (m_step == 0 &&
        emyChkChase(m_range, 0x1.921fb4p-1f /* 0x3f490fda, pi / 4 */, this, m_target, 1) != 0) {
        m_emy->m_move->stop();
        setMotion(0x15, 0x100);
        m_step = 1;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00150b08

void ClrOutWin::Term()
{
    termCommon();
}
