/* ==========================================================================
 *  mag/maglifepcbtl.cpp
 *
 *  MagLifePC's fighting: the weapon's hit nodes, the target angle, the hit
 *  window of the attack motion, the attack motions (bare hands, gun,
 *  knife, axe by height and combo step), the attack height of a target,
 *  picking the target (beginAttack, state 11) and the attack itself
 *  (startAttack: the motion, and for the gun the shot along the target
 *  with its hit tests). 0x12bc20..0x12ccac (pinned 2026-10-30; the asserts
 *  name this file in 0x12bc20, 0x12bc98, 0x12c104, 0x12c1fc, 0x12c31c and
 *  0x12cb50).
 * ======================================================================== */

#include "mag/maglifepc.h"

#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "mag/magscene.h"
#include "utl/utlbtl.h"
#include "utl/utlgame.h"
#include "utl/utlmath.h"
#include "utl/utlrand.h"

/* The attacks' weapon classes (getWeaponType) and the table bound the
 * assert names. */
enum { WEAPON_HAND = 0, WEAPON_GUN = 1, WEAPON_KNIFE = 2, WEAPON_AXE = 3 };
enum { MAX_WEAPON_NODE = 8 };

/* The gun's sounds (utlPlaySe): the cock on targetting, the shot, the
 * empty click, the hit and the miss. */
enum { SE_GUN_READY = 0x7d, SE_GUN_SHOT = 0x7f, SE_GUN_EMPTY = 0x80,
       SE_GUN_HIT = 0x83, SE_GUN_MISS = 0x85 };

/* How far the targetting and the shot look for enemies. */
#define ATK_TARGET_RANGE 500.0f
#define ATK_SHOT_RANGE   800.0f

/* The bladed weapons' hit nodes along the blade (hitNo). */
// ROR 0x00298208
static const char *const s_weaponHitNode[MAX_WEAPON_NODE] = {
    "p_hit", "p_hit1", "p_hit2", "p_hit3", "p_hit4", "p_hit5", "p_hit6", "p_hit7",
};

/* The attack motions [weapon][height][combo]. */
// ROR 0x00298228
static const char *const s_attackMotion[4][5][3] = {
    {
        { "sude_atk", "sude_atk", "sude_atk" },
        { "sude_atk_hi", "sude_atk_hi", "sude_atk_hi" },
        { "sude_atk_lo", "sude_atk_lo", "sude_atk_lo" },
        { "sude_atk_ml", "sude_atk_ml", "sude_atk_ml" },
        { "sude_stomp", "sude_stomp", "sude_stomp" },
    },
    {
        { "gun_atk", "gun_atk", "gun_atk" },
        { "gun_atk_hi", "gun_atk_hi", "gun_atk_hi" },
        { "gun_atk_lo", "gun_atk_lo", "gun_atk_lo" },
        { "gun_atk_ml", "gun_atk_ml", "gun_atk_ml" },
        { "gun_stomp", "gun_stomp", "gun_stomp" },
    },
    {
        { "knife_atk1", "knife_atk2", "knife_atk3" },
        { "knife_atk1_hi", "knife_atk2_hi", "knife_atk3_hi" },
        { "knife_atk1_lo", "knife_atk2_lo", "knife_atk3_lo" },
        { "knife_atk1_ml", "knife_atk2_ml", "knife_atk3_ml" },
        { "knife_stomp", "knife_stomp", "knife_stomp" },
    },
    {
        { "ax_atk1", "ax_atk2", "ax_atk3" },
        { "ax_atk1_hi", "ax_atk2_hi", "ax_atk3_hi" },
        { "ax_atk1_lo", "ax_atk2_lo", "ax_atk3_lo" },
        { "ax_atk1_ml", "ax_atk2_ml", "ax_atk3_ml" },
        { "ax_stomp", "ax_stomp", "ax_stomp" },
    },
};

/* The hit windows in motion frames: the stomp (every weapon), bare hands
 * by height, the knife and the axe by height and combo. The gun has none
 * (0..1000). */
struct AtkFrameRange { float m_from, m_to; };
static_assert(sizeof(AtkFrameRange) == 8, "AtkFrameRange is 8 bytes on the EE");
// ROR 0x002ec9d0
static const AtkFrameRange s_stompFrames = { 25.0f, 30.0f };
// ROR 0x002ec9d8
static const AtkFrameRange s_handFrames[4] = {
    { 0.0f, 10.0f }, { 0.0f, 10.0f }, { 0.0f, 10.0f }, { 0.0f, 10.0f },
};
// ROR 0x002ec9f8
static const AtkFrameRange s_knifeFrames[4][3] = {
    { { 0.0f, 5.0f }, { 5.0f, 10.0f }, { 4.0f, 12.0f } },
    { { 0.0f, 5.0f }, { 5.0f, 10.0f }, { 4.0f, 12.0f } },
    { { 0.0f, 5.0f }, { 5.0f, 10.0f }, { 4.0f, 12.0f } },
    { { 0.0f, 5.0f }, { 5.0f, 10.0f }, { 4.0f, 12.0f } },
};
// ROR 0x002eca58
static const AtkFrameRange s_axeFrames[4][3] = {
    { { 2.0f, 7.0f }, { 2.0f, 7.0f }, { 6.0f, 15.0f } },
    { { 2.0f, 7.0f }, { 2.0f, 7.0f }, { 6.0f, 15.0f } },
    { { 2.0f, 7.0f }, { 2.0f, 7.0f }, { 6.0f, 15.0f } },
    { { 2.0f, 7.0f }, { 2.0f, 7.0f }, { 6.0f, 15.0f } },
};

/* The collectors' buffer as the ROM's callers set it up: every entry's
 * two points (0, 0, 0, 1), no count (as maglifebwnctrl.cpp's). */
static void clearEmyInfoBuf(UtlEnemyInfoBuf &eib)
{
    for (int i = 0; i < UtlEnemyInfoBuf::MAX_INFO; i++)
    {
        UtlEnemyInfo &e = eib.m_info[i];
        e.m_pos[0] = e.m_pos[1] = e.m_pos[2] = 0.0f;
        e.m_pos[3] = 1.0f;
        e.m_trans[0] = e.m_trans[1] = e.m_trans[2] = 0.0f;
        e.m_trans[3] = 1.0f;
    }
    eib.m_count = 0;
}

// ROR 0x0012bc20

float MagLifePC::getAttackBasePos(float *out)
{
    out[0] = m_actor->m_trans[0];
    out[1] = m_actor->m_trans[1] - 120.0f;
    out[2] = m_actor->m_trans[2];
    ROR_ASSERT(m_actor); /* 251 */  // callcheck: dbgAssertFailed (the assert handler)
    return m_actor->m_rotate[1];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0012bc98

float MagLifePC::getWeaponHitPos(float *out, int theHeight, int hitNo)
{
    if (m_1b0 >= 0)
        return 0.0f;
    theHeight &= 0xffff;
    int equipchrid = getEquipItemChrNo();
    float radius;
    const char *node;
    if (theHeight == ATK_HEIGHT_STOMP)
    {
        equipchrid = -1;
        radius = 50.0f;
        node = "Bip01 R Toe0";
    }
    else if (m_weapon_type == WEAPON_GUN)
    {
        if (equipchrid < 0)
            return 0.0f;
        radius = 1.0f;
        node = "gun_mazzle";
    }
    else if (m_weapon_type == WEAPON_KNIFE || m_weapon_type == WEAPON_AXE)
    {
        if (equipchrid < 0)
            return 0.0f;
        ROR_ASSERT(hitNo >= 0 && hitNo < MAX_WEAPON_NODE); /* 316: theHitNo >= 0 && theHitNo < MAX_WEAPON_NODE */
        radius = 5.0f;
        node = s_weaponHitNode[hitNo];
    }
    else if (theHeight == 2)
    {
        /* The bare-handed low attack is a kick. */
        node = hitNo == 0 ? "Bip01 R Calf" : "Bip01 R Toe0";
        ROR_ASSERT(equipchrid < 0); /* 333 */
        radius = 10.0f;
    }
    else
    {
        if (equipchrid >= 0)
            return 0.0f;
        radius = 10.0f;
        node = "b_fg_r";
    }

    /* The weapon item's own actor, else the PC's. */
    DogActor *actor = (u_int)equipchrid < 0x80 ? &m_scene->m_actors[equipchrid] : m_actor;
    ROR_ASSERT(actor); /* 362 */
    if (actor == NULL)      /* PC: the ROM's assert never returns */
        return 0.0f;
    int n = actor->getNodeIndexByName(node);   // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)
    if (n < 0)
        return 0.0f;
    sceVu0FMATRIX m;
    actor->getNodeWorldMatrix(m, n);
    out[0] = m[3][0];
    out[1] = m[3][1];
    out[2] = m[3][2];
    out[3] = m[3][3];
    return radius;
}

// ROR 0x0012bedc

float MagLifePC::calcTargetAngle(float *roty, float *rotx, const float *pos)
{
    float dir[4];
    dir[0] = pos[0] - m_actor->m_trans[0];
    dir[1] = pos[1] - m_actor->m_trans[1];
    dir[2] = pos[2] - m_actor->m_trans[2];
    dir[3] = 1.0f;
    float rot[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    utlVecToRot(dir, rot);
    *roty = rot[1];
    *rotx = rot[0];
    float d = *roty - m_actor->m_rotate[1];
    if (d < -3.141593f)
        d += 6.283186f;
    else if (d > 3.141593f)
        d -= 6.283186f;
    return d;
}

// ROR 0x0012c014

void MagLifePC::getHitFrameRange(float *from, float *to)
{
    AtkFrameRange def = { 0.0f, 1000.0f };
    const AtkFrameRange *r = &def;
    if (m_atk_height >= ATK_HEIGHT_STOMP)
        r = &s_stompFrames;
    else if (m_atk_weapon == WEAPON_HAND)
        r = &s_handFrames[m_atk_height];
    else if (m_atk_weapon == WEAPON_KNIFE)
        r = &s_knifeFrames[m_atk_height][m_atk_combo];
    else if (m_atk_weapon == WEAPON_AXE)
        r = &s_axeFrames[m_atk_height][m_atk_combo];
    *from = r->m_from;
    *to = r->m_to;
}

// ROR 0x0012c104

const char *MagLifePC::getAttackMotionName(int theWeapon, int theHeight, int theCombo)
{
    ROR_ASSERT(theCombo >= 0 && theCombo < 3); /* 592 */  // callcheck: dbgAssertFailed (the assert handler)
    ROR_ASSERT(theWeapon >= 0 && theWeapon < 4); /* 593 */
    ROR_ASSERT(theHeight >= 0 && theHeight < 5); /* 594 */
    if (theWeapon == WEAPON_GUN && getSelItemNum() < 1)
        return "gun_empty";
    return s_attackMotion[theWeapon][theHeight][theCombo];
}

// ROR 0x0012c1fc

void MagLifePC::playAttackMotion(int theWeapon, int theHeight, int theCombo)
{
    ROR_ASSERT(theCombo >= -1 && theCombo < 3); /* 657 */  // callcheck: dbgAssertFailed (the assert handler)
    ROR_ASSERT(theWeapon >= 0 && theWeapon < 4); /* 658 */
    ROR_ASSERT(theHeight >= 0 && theHeight < 5); /* 659 */
    m_atk_weapon = theWeapon;
    m_atk_height = theHeight;
    m_atk_combo = theCombo;
    changeMotion(1.0f / 60.0f, getAttackMotionName(theWeapon, theHeight, theCombo), 0, NULL, -1);
    clearSubMotion();
    utlPlaySe(getAttackSeNo(theHeight, theCombo, 0), 0, NULL);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0012c31c

u_int MagLifePC::startAttack(int combo)
{
    /* 685: m_submode.getMode() == MAG_PC_MODE_ATK_WAIT || m_submode.getMode() == MAG_PC_MODE_ATTACKING */
    ROR_ASSERT(m_state == 5 || m_state == 10);  // callcheck: dbgAssertFailed (the assert handler)

    UtlEnemyInfoBuf eib;
    if (m_prev_state[0] == 5)
    {
        /* The first blow from the stance: aim at the nearest enemy within
         * 45 degrees (the gun keeps beginAttack's target). */
        clearEmyInfoBuf(eib);
        int n = utlCollectEmyInfoAt(&eib, getTransPtr(), 0, 1, getRotY(), ATK_TARGET_RANGE);
        if (m_weapon_type != WEAPON_GUN)
        {
            int i = n >= 1 ? utlFindNearestEmyInRange(0.7853982f, eib) : -1;
            if (i >= 0)
            {
                m_target_node = eib.m_info[i].m_24;
                m_target_no = eib.m_info[i].m_obj_id;
                m_target_dist = getTargetSpinePos(m_target_pos, m_target_no, 0);
            }
            else
            {
                m_target_no = 0;
                if (m_1b0 >= 0)
                {
                    playAttackMotion(m_weapon_override, m_1b0, combo);
                    return (u_int)m_weapon_override << 16 | (u_int)m_1b0;
                }
                playAttackMotion(m_weapon_type, 0, combo);
                return (u_int)m_weapon_type << 16;
            }
        }
    }

    /* m_1b0 (message 0x17) forces the height and m_weapon_override's
     * weapon; else the target's height. */
    u_int ret;
    int height = m_1b0;
    if (height < 0)
    {
        height = 0;
        if (m_target_no != 0)
            height = calcAttackHeight(m_target_pos, m_target_no);
        ret = (u_int)m_weapon_type << 16 | (u_int)height;
        playAttackMotion(m_weapon_type, height, combo);
    }
    else
    {
        ret = (u_int)m_weapon_override << 16 | (u_int)height;
        playAttackMotion(m_weapon_override, height, combo);
    }

    if (height == ATK_HEIGHT_STOMP || m_weapon_type != WEAPON_GUN)
        return ret;

    /* The shot. */
    if (getSelItemNum() < 1)
    {
        utlPlaySe(SE_GUN_EMPTY, 0, NULL);
        return ret;
    }
    utlPlaySe(SE_GUN_SHOT, 0, NULL);
    createMuzzleEff();
    decSelItemNum();

    clearEmyInfoBuf(eib);
    int n = utlCollectEmyInfoAt(&eib, getTransPtr(), 2, 0, getRotY(), ATK_SHOT_RANGE);
    /* Nearest first (a bubble sort on m_dist). */
    if (n > 1)
    {
        for (int i = 0; i < n - 1; i++)
        {
            for (int j = n - 1; j > i; j--)
            {
                if (eib.m_info[j].m_dist < eib.m_info[j - 1].m_dist)
                {
                    UtlEnemyInfo t = eib.m_info[j];
                    eib.m_info[j] = eib.m_info[j - 1];
                    eib.m_info[j - 1] = t;
                }
            }
        }
    }

    /* A ray from the muzzle towards the target, turned by a random yaw
     * (utlRandMean of +-10 degrees). */
    UtlHitCheck hc;
    utlInitHitCheck(&hc);
    float origin[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    getWeaponHitPos(origin, (int)(ret & 0xffff), 0);
    float dir[4];
    dir[0] = m_target_pos[0] - origin[0];
    dir[1] = m_target_pos[1] - origin[1];
    dir[2] = m_target_pos[2] - origin[2];
    dir[3] = 1.0f;
    int spread = utlRandMean(-100000, 100000, 4);
    utlVecRotMatY((((float)spread * 3.141593f) / 180.0f) / 10000.0f, dir);
    hc.m_origin_pos = origin;
    hc.m_dir = dir;
    hc.m_len = ATK_SHOT_RANGE;
    hc.m_10 = 0.0f;

    /* Each enemy in front, nearest first; the bullet stops in one by its
     * type's rate, the less the farther. */
    int hits = 0;
    for (int i = 0; i < n; i++)
    {
        UtlEnemyInfo &e = eib.m_info[i];
        if (e.m_rot > -1.5707963f && e.m_rot < 1.5707963f)
        {
            e.m_pos[3] = e.m_radius;
            hc.m_target_pos = e.m_pos;
            if (utlHitCheck(&hc))
            {
                hits++;
                hitTarget(e.m_obj_id, 0, 0);
                float rate = getEmyTypeRate(e.m_obj_id);
                float dist = e.m_dist;
                if (rate * ((ATK_SHOT_RANGE - dist) / ATK_SHOT_RANGE) <= utlRandRangeF(0.0f, 1.0f))
                    break;
            }
        }
    }
    utlPlaySe(hits < 1 ? SE_GUN_MISS : SE_GUN_HIT, 0, NULL);
    return ret;
}

// ROR 0x0012c9c0

void MagLifePC::setState14(int arg, int word)
{
    if (m_actor != NULL && (m_state != 14 || m_08 > 7))
    {
        changeState(14, arg, 1);
        m_0c = word;
    }
}

// ROR 0x0012ca28

int MagLifePC::getAttackHeight()
{
    if (m_target_no == 0)
        return m_1b0 < 0 ? 0 : m_1b0;
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    getTargetSpinePos(pos, m_target_no, 0);
    return calcAttackHeight(pos, m_target_no);
}

// ROR 0x0012caa4

int MagLifePC::calcAttackHeight(const float *pos, u_int oid)
{
    float base[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    getAttackBasePos(base);
    float dist = utlVecDistXZ(base, pos);
    if (!testEmyHittable(oid) && dist <= 80.0f)
        return ATK_HEIGHT_STOMP;
    return getAttackHeightByDist(dist, oid);
}

// ROR 0x0012cb50

void MagLifePC::beginAttack()
{
    ROR_ASSERT(m_state == 11); /* 1199: m_submode.getMode() == MAG_PC_MODE_TARGETTING */  // callcheck: dbgAssertFailed (the assert handler)
    if (m_weapon_type == WEAPON_GUN)
        utlPlaySe(SE_GUN_READY, 0, NULL);
    m_target_no = 0;
    m_target_node = -1;

    /* The nearest enemy within 90 degrees of the heading. */
    UtlEnemyInfoBuf eib;
    clearEmyInfoBuf(eib);
    int n = utlCollectEmyInfoAt(&eib, getTransPtr(), 0, 1, getRotY(), ATK_TARGET_RANGE);
    int i = n >= 1 ? utlFindNearestEmyInRange(1.5707964f, eib) : -1;
    if (i >= 0)
    {
        const UtlEnemyInfo &e = eib.m_info[i];
        m_target_pos[0] = e.m_pos[0];
        m_target_pos[1] = e.m_pos[1];
        m_target_pos[2] = e.m_pos[2];
        m_target_pos[3] = e.m_pos[3];
        m_target_no = e.m_obj_id;
        m_target_node = e.m_24;
    }
    else
        m_target_no = 0;
    changeState(5, 0, 0);
}
