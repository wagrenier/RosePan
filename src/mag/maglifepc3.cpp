/* ==========================================================================
 *  mag/maglifepc3.cpp
 *
 *  MagLifePC's motions: the motion change by name (the "jnfc_" variants in
 *  actor mode 2), the weapon motion packs (data/ch/pc/pc/MOTIONPK.RPK, one
 *  file per weapon type) that initMotion loads, the upper-body sub motions
 *  the modes set, the one-shot action motion, and two small state queries.
 *  0x11d720..0x11e0d8 (pinned; the asserts name this file).
 * ======================================================================== */

#include "mag/maglifepc.h"

#include <stdint.h>
#include <string.h>

#include "bed/bedpfh.h"
#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "dog/dogmotionblend.h"
#include "game/gamfile.h"
#include "utl/utlmisc.h"

/* m_health's range (the ROM's assert calls it m_health_stat). */
enum { HEALTH_END = 5 };

/* The motion speed by health: the last two stop the motion. */
// ROR 0x002ea740
static const float s_healthMotionStep[HEALTH_END] = {
    1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
};

/* The upper body: setStanceSubMotion's nodes for stances 0..3. */
// ROR 0x00297160
static const char *s_upperBodyNodes[] = {
    "Bip01 Spine", "Bip01 Spine1", "Bip01 Neck", "Bip01 Head",
    "Bip01 L Clavicle", "Bip01 L UpperArm", "Bip01 L Forearm", "Bip01 L Hand", "b_fg_l",
    "Bip01 R Clavicle", "Bip01 R UpperArm", "Bip01 R Forearm", "Bip01 R Hand", "b_fg_r",
    NULL,
};

/* Both arms, right first (stance 4, the type 3 wait). */
// ROR 0x002971a0
static const char *s_armNodes[] = {
    "Bip01 R Clavicle", "Bip01 R UpperArm", "Bip01 R Forearm", "Bip01 R Hand", "b_fg_r",
    "Bip01 L Clavicle", "Bip01 L UpperArm", "Bip01 L Forearm", "Bip01 L Hand", "b_fg_l",
    NULL,
};

/* changeMotion's name buffer: "jnfc_" then the name past its first three
 * characters. */
// ROR 0x002971d0
static char s_jnfcName[0x40] = "jnfc_";

/* MOTIONPK.RPK's file by weapon type. */
// ROR 0x00297210
static const int s_weaponMotionPack[4] = { 0, 3, 2, 1 };

/* setStanceSubMotion's motion by m_stance (0: none). */
// ROR 0x00297220
static const int s_stanceSubMotion[5] = { -1, 0xe4, 0xbd, 0xd7, 0xa5 };

/* The right fingers alone (setWaitSubMotion). */
// ROR 0x00297238
static const char *s_rightFingerNodes[] = { "b_fg_r", NULL };

/* The wait motions by weapon type (sude: bare hands; initMotion tests
 * m_in). EE 0x18 bytes; layout: host -- pointers. */
struct MagLifePCWeaponMotion
{
    const char *m_in;
    const char *m_wait;
    const char *m_wait_hi;
    const char *m_wait_lo;
    const char *m_wait_ml;
    const char *m_wait_lo2;     // m_wait_lo again
};
// ROR 0x00298178
static const MagLifePCWeaponMotion s_weaponMotion[4] = {
    { "sude_in",  "sude_wait",  "sude_wait_hi",  "sude_wait_lo",  "sude_wait_ml",  "sude_wait_lo" },
    { "gun_in",   "gun_wait",   "gun_wait_hi",   "gun_wait_lo",   "gun_wait_ml",   "gun_wait_lo" },
    { "knife_in", "knife_wait", "knife_wait_hi", "knife_wait_lo", "knife_wait_ml", "knife_wait_lo" },
    { "ax_in",    "ax_wait",    "ax_wait_hi",    "ax_wait_lo",    "ax_wait_ml",    "ax_wait_lo" },
};

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011d720
float MagLifePC::getHealthMotionStep()
{
    ROR_ASSERT(0 <= m_health && m_health < HEALTH_END); /* 105: m_health_stat */
    return s_healthMotionStep[m_health] * m_actor->getMotionStep();
}

// ──────────────────────────────────────────────────────────────────────
// The by-number change was never written.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011d790
int MagLifePC::changeMotion(float speed, int theNo, u_int flags, int theNo2, int arg)
{
    (void)speed; (void)theNo; (void)flags; (void)theNo2; (void)arg;
    ROR_ASSERT(0); /* 186 */
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011d7c0
int MagLifePC::changeMotion(float speed, const char *theMotion, u_int flags,
                            const char *theMotion2, int arg)
{
    ROR_ASSERT(theMotion); /* 197 */
    ROR_ASSERT(m_actor); /* 198 */
    if (theMotion == NULL || m_actor == NULL)
        return -1;
    int no;
    int no2 = -1;
    if (m_mode == 2)
    {
        strcpy(s_jnfcName + 5, theMotion + 3);
        no = m_actor->getMotionNoByName(s_jnfcName);
        if (no < 0 && (no = m_actor->getMotionNoByName(theMotion)) < 0)
            return no;
        if (theMotion2 != NULL)
        {
            strcpy(s_jnfcName + 5, theMotion2 + 3);
            no2 = m_actor->getMotionNoByName(s_jnfcName);
            if (no2 < 0)
                no2 = m_actor->getMotionNoByName(theMotion2);
        }
    }
    else
    {
        no = m_actor->getMotionNoBySuffix(theMotion);
        if (no < 0)
            return no;
        if (theMotion2 != NULL)
            no2 = m_actor->getMotionNoBySuffix(theMotion2);
    }
    return m_actor->changeMotion(speed, no, flags, no2, arg);
}

// ──────────────────────────────────────────────────────────────────────
// DogActor::loadPackMotion's reading into a given bank: 1 when the actor
// took the motions, 0 otherwise (the block freed).
// callcheck: dbgAssertFailed, BedPfh, FileReadReq, FileReader, ~FileReader (the assert handler; the pfh constructors; inline in game/gamfile.h: the two constructors and the destructor)
// ROR 0x0011d990
int MagLifePC::loadMotion(int theLevel, const char *theFileName, int theIndex)
{
    ROR_ASSERT(theFileName); /* 252 */
    m_actor->releaseMotion(theLevel);
    FileReader reader;
    FileReadReq req;
    /* The pack's header: its first 2048 bytes, into a 64-aligned stack buffer. */
    u_char buf[2048 + 64];
    void *head = (void *)((((uintptr_t)buf) >> 6 << 6) + 64);
    req.m_08 = 1;
    req.m_name = theFileName;
    req.m_buf = head;
    req.m_size = 0x800;
    reader.read(&req, 0);
    BedPfh pfh1(head);
    ROR_ASSERT(pfh1.getFileOffset(0) == (2048)); /* 276 */
    ROR_ASSERT(theIndex >= 0 && theIndex < pfh1.getNumFile()); /* 278 */
    int size = pfh1.getFileSize(theIndex);
    if (size < 1)
    {
        ROR_ASSERT(0); /* 284 */
        return 0;
    }
    size = (size + 0x7ff) / 0x800 * 0x800;
    void *p = g_utlMemory.allocDebug(size, "mag/maglifepc3.cpp", 289, 0, 1);
    ROR_ASSERT(p != NULL); /* 290 */
    ROR_ASSERT((((intptr_t)p)%64)==0); /* 291: ((int)p) on the EE */
    /* PC: the ROM hangs in the assert; no block, no read. */
    if (p == NULL)
        return 0;
    req.m_offset = pfh1.getFileOffset(theIndex);
    req.m_close = 1;
    req.m_whence = 0;
    req.m_buf = p;
    req.m_size = size;
    reader.read(&req, 0);
    BedPfh pfh(p);
    if (m_actor->loadMotion(pfh, theLevel) > 0)
        return 1;
    g_utlMemory.free(p);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The bank is released even when stance 0 then loads nothing.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011dc64
void MagLifePC::loadWeaponMotion(int theIndex, int theLevel)
{
    int level = 3;
    if (theLevel >= 0)
        level = theLevel;
    m_actor->releaseMotion(level);
    if (theIndex < 0)
    {
        switch (m_stance)
        {
        case 0: return;
        case 1: theIndex = 0; break;
        case 2: theIndex = 3; break;
        case 3: theIndex = 2; break;
        case 4: theIndex = 1; break;
        default:
            ROR_ASSERT(0); /* 360 */
            theIndex = 0;
            break;
        }
    }
    m_actor->loadPackMotion("data/ch/pc/pc/MOTIONPK.RPK", theIndex, level);
}

// ──────────────────────────────────────────────────────────────────────
// The equipped weapon's type; its pack is loaded when the actor lacks the
// type's "_in" motion, and with no motion running the mode restarts.
// ROR 0x0011dd40
void MagLifePC::initMotion()
{
    if (m_actor == NULL)
        return;
    m_weapon_type = getWeaponType(getEquipItem());
    if (m_actor->findMotionBySuffix(s_weaponMotion[m_weapon_type].m_in) >= 0)
        return;
    loadWeaponMotion(s_weaponMotionPack[m_weapon_type], -1);
    setStanceSubMotion();
    if (m_actor->m_motion.getNo() < 1)
        selectMode(1);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011ddf4
int MagLifePC::playActionMotion(float speed, int theNo)
{
    ROR_ASSERT(theNo >= 0); /* 483 */
    if (m_action_motion < 0)
    {
        changeMotion(speed, theNo, 0, -1, -1);
        m_action_motion = theNo;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// m_action_motion takes the record index (findMotionByName), not the id.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011de78
int MagLifePC::playActionMotion(float speed, const char *theMotion)
{
    ROR_ASSERT(theMotion != NULL); /* 502 */
    int no = m_actor->findMotionByName(theMotion);
    if (m_action_motion < 0)
    {
        changeMotion(speed, theMotion, 0, NULL, -1);
        m_action_motion = no;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011df14
int MagLifePC::isActionMotionEnd()
{
    if (m_action_motion < 0)
        return 1;
    ROR_ASSERT(m_actor); /* 526 */
    if (m_actor->isModeSet())
        return 0;
    m_action_motion = -1;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011df7c
void MagLifePC::setStanceSubMotion()
{
    const char **nodes = m_stance == 4 ? s_armNodes : s_upperBodyNodes;
    m_actor->setMotionSub(0, s_stanceSubMotion[m_stance], nodes);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011dfd0
void MagLifePC::clearSubMotion()
{
    m_actor->setMotionSub(0, -1, NULL);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011dff8
void MagLifePC::setWaitSubMotion(int noGrip)
{
    if (m_weapon_type == 3)
    {
        m_actor->setMotionSub(0, 0xa5, s_armNodes);
        return;
    }
    if (m_weapon_type == 0 || noGrip)
    {
        m_actor->setMotionSub(0, -1, s_rightFingerNodes);
        return;
    }
    m_actor->setMotionSub(0, 0x19, s_rightFingerNodes);
}

// ──────────────────────────────────────────────────────────────────────
// MagSystem::getPCStateClass's answer for the sub states.
// ROR 0x0011e080
int MagLifePC::getSubStateClass()
{
    if (m_move_level_prev == 1)
        return 6;
    if (m_move_level_prev == 2)
        return 7;
    return 5;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011e0ac
int MagLifePC::isMoveState()
{
    return (u_int)(m_state - 1) < 10 || m_state == 14;
}
