/* ==========================================================================
 *  mag/maglifepc2.cpp
 *
 *  MagLifePC's items and attacks: the GamMan / EmyManager glue the modes
 *  call (PC params, the equipped item, ammunition, enemy queries), picking
 *  up items (collectNearItems, takeItem), the object collision helpers of
 *  the move sweep, the hit (hitTarget, the attack type of the item), the
 *  target's spine node, reload, the hit sounds and the attack height of an
 *  enemy by its distance. 0x11c464..0x11d720 (pinned 2026-10-29; the
 *  asserts name this file in 0x11c64c, 0x11caf8, 0x11ce5c, 0x11d1cc and
 *  0x11d3d0).
 * ======================================================================== */

#include "mag/maglifepc.h"

#include <string.h>

#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "eff/effman.h"
#include "emy/emy_manager.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "mag/magscene.h"
#include "sequence/sequence.h"
#include "task/tsk_chr.h"
#include "task/tsk_scene.h"
#include "utl/utlgame.h"
#include "utl/utlmath.h"

/* Items that are not weapons: none and the empty slot. */
enum { ITEM_NONE = 0, ITEM_EMPTY = 0xa00 };
/* The two items with the gun sounds (the loaded / empty pair 0x7f / 0x80,
 * the hit 0x83). */
enum { ITEM_GUN_A = 0xa3e, ITEM_GUN_B = 0xa43 };

/* The weapon sounds: row by item (s_attackSeRow), columns combo 0..2 (the
 * swing, kind 0) then the hit (kind != 0). */
// ROR 0x002ea340
static const short s_attackSe[7][4] = {
    {  72,  73,  74,  71 },
    {  90,  91,  92,  89 },
    { 108, 109, 110, 107 },
    { 137, 138, 139, 136 },
    {  81,  82,  83,  80 },
    {  99, 100, 101,  98 },
    { 117, 118, 119, 116 },
};

/* item -> s_attackSe row; -1 ends (its row -1: row 0, the hit 0x47). */
struct MagLifePCAttackSeRow
{
    int m_item;
    int m_row;
};
static_assert(sizeof(MagLifePCAttackSeRow) == 8, "MagLifePCAttackSeRow is 8 bytes on the EE");
// ROR 0x002ea378
static const MagLifePCAttackSeRow s_attackSeRow[] = {
    { 0xa3a, 6 }, { 0xa3c, 5 }, { 0xa38, 4 }, { 0xb35, 4 },
    { 0xa3d, 3 }, { 0xb25, 3 }, { 0xb36, 3 }, { 0xa39, 2 },
    { 0xa44, 2 }, { 0xb32, 2 }, { 0xa3b, 1 }, { 0xb27, 1 },
    { 0xb31, 1 }, { 0xb34, 1 }, { -1, -1 },
};

/* getAttackHeightByDist's records (EE 0x28; the ROM copies the table to
 * the stack). Each band holds the XZ distances [m_min, m_max); -1, -1
 * never matches. Bands 0..3 give heights 1, 0, 3, 2. */
struct MagLifePCHeightBand
{
    float m_max;
    float m_min;
};
static_assert(sizeof(MagLifePCHeightBand) == 8, "MagLifePCHeightBand is 8 bytes on the EE");
/* layout: host -- the name pointer (EE 0x28). */
struct MagLifePCHeightRec
{
    const char         *m_name;     // the enemy type's name (not read)
    int                 m_emy_type; // EmyManager::getEmyType
    MagLifePCHeightBand m_band[4];
};
// ROR 0x002ea450
static const MagLifePCHeightRec s_heightRec[10] = {
    { "DWF",      0,  { { -1.0f, -1.0f }, { 100000.0f, 200.0f }, { 200.0f, 130.0f }, { 130.0f, 0.0f } } },
    { "STRAYDOG", 10, { { -1.0f, -1.0f }, { 100000.0f, 100.0f }, { 100.0f, 40.0f },  { 40.0f, 0.0f } } },
    { "CLARA",    8,  { { -1.0f, -1.0f }, { 100000.0f, 0.0f },   { -1.0f, -1.0f },   { -1.0f, -1.0f } } },
    { "BIRD_L",   5,  { { 100000.0f, 100.0f }, { 100.0f, 0.0f }, { -1.0f, -1.0f },   { -1.0f, -1.0f } } },
    { "BIRD_S",   4,  { { -1.0f, -1.0f }, { 100000.0f, 0.0f },   { -1.0f, -1.0f },   { -1.0f, -1.0f } } },
    { "FISH",     3,  { { -1.0f, -1.0f }, { 100000.0f, 200.0f }, { 200.0f, 150.0f }, { 150.0f, 0.0f } } },
    { "GOAT",     2,  { { -1.0f, -1.0f }, { 100000.0f, 80.0f },  { 80.0f, 50.0f },   { 50.0f, 0.0f } } },
    { "PIG",      1,  { { -1.0f, -1.0f }, { 100000.0f, 150.0f }, { 150.0f, 0.0f },   { -1.0f, -1.0f } } },
    /* Hoffman (type 6, never matched by the search): 8 when his neck is
     * below -140 or he has no actor, else 9. */
    { "HOFFMAN",  6,  { { 100.0f, 0.0f }, { 100000.0f, 100.0f }, { -1.0f, -1.0f },   { -1.0f, -1.0f } } },
    { "HOFFMAN2", 6,  { { -1.0f, -1.0f }, { 100000.0f, 100.0f }, { 100.0f, 40.0f },  { 40.0f, 0.0f } } },
};
enum { HEIGHT_REC_TYPED = 8, HEIGHT_REC_HOFFMAN_LOW = 8, HEIGHT_REC_HOFFMAN = 9 };
enum { EMY_TYPE_HOFFMAN = 6 };
static const int s_bandHeight[4] = { 1, 0, 3, 2 };

// ──────────────────────────────────────────────────────────────────────
// GamMan glue
// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011c464

int MagLifePC::getPcParam1b()
{
    int value = 0;
    g_gamMan.getPcParam(0x1b, &value);
    return value;
}

// ROR 0x0011c494

void MagLifePC::setPcParam1b(int value)
{
    g_gamMan.setPcParam(0x1b, &value);
}

// ROR 0x0011c4c0

int MagLifePC::getPcParam1c()
{
    int value = 0;
    g_gamMan.getPcParam(0x1c, &value);
    return value;
}

// ROR 0x0011c4f0

int MagLifePC::getPcParam0()
{
    int value = 0;
    g_gamMan.getPcParam(0, &value);
    return value;
}

// ROR 0x0011c520

int MagLifePC::getEquipItem()
{
    return g_gamMan.m_equipItem;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011c52c

int MagLifePC::collectPcParam0AchrPos(float *out, int max)
{
    int param0 = getPcParam0();
    int count = 0;
    SeqList<ACHRstat *> &used = g_gamMan.m_achrList.m_used;
    for (SeqNode<ACHRstat *> *it = used.begin(); it != used.end(); it = it->m_next)
    {
        ACHRstat *achr = it->m_data;
        if ((achr->m_flags & 0x10000) != 0)
            continue;
        int value;
        achr->getParam(0, &value);
        if (param0 != value)
            continue;
        count++;
        achr->getPos(out);
        out += 4;
        if (count >= max)
            return count;
    }
    return count;
}

// ──────────────────────────────────────────────────────────────────────
// The circle is centred theRange - 5 ahead of the actor (its heading), so
// the actor stands just inside its back edge.
// ROR 0x0011c64c // callcheck: operator_new[], operator_delete[], dbgAssertFailed (new SeqNode[0x20]; the assert handler)

int MagLifePC::collectNearItems(float theRange, MagLifePCItemData *theData, int theMax)
{
    ROR_ASSERT(theRange >= 5.0f); /* 204 */
    ROR_ASSERT(theData); /* 205 */
    ROR_ASSERT(theMax >= 10); /* 206 */

    SeqPool<FCHRstat *> found;
    SeqNode<FCHRstat *> *nodes = new SeqNode<FCHRstat *>[0x20];
    found.build(nodes, 0x20);

    float pos[4];
    for (int i = 0; i < 4; i++)
        pos[i] = m_actor->m_trans[i];
    float ahead[4] = { 0.0f, 0.0f, theRange - 5.0f, 1.0f };
    utlVecRotMatY(m_actor->getRotY(), ahead);
    pos[0] += ahead[0];
    pos[1] += ahead[1];
    pos[2] += ahead[2];
    pos[3] = 1.0f;
    g_gamMan.collectFchrsNear(pos, theRange, &found, 5, 0x100000, 0x4008001);

    int count = 0;
    for (SeqNode<FCHRstat *> *it = found.m_used.begin(); it != found.m_used.end(); it = it->m_next)
    {
        FCHRstat *chr = it->m_data;
        chr->getPos(theData->m_pos);
        float dy = theData->m_pos[1] - pos[1];
        if (dy < -200.0f || dy > 30.0f)
            continue;
        if ((u_short)chr->m_kind == 0xffff)
            continue;
        theData->m_item_id = (u_short)chr->m_kind;
        count++;
        theData->m_obj_id = chr->m_oid;
        theData->m_actor = (short)chr->m_actor;
        theData++;
        if (count >= theMax)
            break;
    }
    delete[] nodes;
    return count;
}

// ──────────────────────────────────────────────────────────────────────
// The ROM passes the actor's position as a3, which scnChrRequest does not
// read (a pointer does not fit its int on the host).
// ROR 0x0011c974

void MagLifePC::hitTarget(u_int oid, int combo, int kind)
{
    int word = 7;
    if (kind == 0)
    {
        int item = getEquipItem();
        if (item < 0 || item == ITEM_EMPTY)
            combo = 0;
        word = getAttackType(item);
    }
    int damage = scnCalcPcDamage(oid, word);
    u_int pc = 0;
    if (g_gamMan.m_pcPtr != NULL)
        pc = g_gamMan.m_pcPtr->m_oid;
    scnChrRequest(0xb, pc, damage, 0, word, oid, combo);
}

// ROR 0x0011caf8 // callcheck: dbgAssertFailed (the assert handler)

int MagLifePC::getAttackType(int item)
{
    int attack_type = 2;
    if (item > 0)
    {
        attack_type = 0;
        g_gamMan.m_sysData.getField((u_short)item, 0x27, &attack_type);
        ROR_ASSERT(attack_type >= 0 && attack_type < 0x10000); /* 334 */
    }
    return attack_type;
}

// ROR 0x0011cb70

int MagLifePC::getWeaponType(int item)
{
    if (m_weapon_override >= 0)
        return m_weapon_override;
    switch (getAttackType(item))
    {
    case 5: return 3;
    case 4: return 2;
    case 6: return 1;
    default: return 0;
    }
}

// ROR 0x0011cbd0

int MagLifePC::countItems()
{
    return g_gamMan.countItems();
}

// ROR 0x0011cbf4

int MagLifePC::canTakeItem(u_int oid)
{
    return g_gamMan.canAddItem(oid, 0);
}

// ──────────────────────────────────────────────────────────────────────
// The script reads m_global 0x28..0x2a: 0x13 (an item was taken), the
// item and its object.
// ROR 0x0011cc14

int MagLifePC::takeItem(u_int oid)
{
    if (!canTakeItem(oid))
        return -1;
    int item = 0;
    g_gamMan.getObjParam(oid, 2, &item);
    g_gamMan.addItem((short)item, 0);
    g_sequence.m_global[0x28] = 0x13;
    g_sequence.m_global[0x29] = item;
    g_sequence.m_global[0x2a] = (int)oid;
    g_gamMan.endPcAction();
    g_gamMan.releaseChr(oid);
    return 0;
}

// ROR 0x0011ccd4

int MagLifePC::testObjHit(const float *pos, u_int *oid)
{
    float at[4];
    for (int i = 0; i < 4; i++)
        at[i] = pos[i];
    ChrHitRec hit;
    int r = g_gamMan.testPcHitAt(at, &hit);
    if (oid != NULL)
        *oid = hit.m_oid;
    return r;
}

// ROR 0x0011cd20

void MagLifePC::slideOffObj(float *out, const float *delta, u_int oid)
{
    float x = m_actor->m_trans[0];
    float z = m_actor->m_trans[2];
    float dir[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    utlGetObjPos(dir, oid);
    dir[0] -= x;
    dir[1] = 0.0f;
    dir[2] -= z;
    dir[3] = 1.0f;
    utlNormalize3(dir);
    float d = utlVecDot(dir, delta);
    for (int i = 0; i < 4; i++)
        out[i] = delta[i];
    out[3] = 1.0f;
    out[0] -= dir[0] * d;
    out[1] -= dir[1] * d;
    out[2] -= dir[2] * d;
    if (utlVecLength(out) < 1.0f)
        utlNormalize3(out);
}

// ROR 0x0011ce50

int MagLifePC::getEquipItemChrNo()
{
    return (short)g_gamMan.m_pcEquipItemChrID;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011ce5c

float MagLifePC::getTargetSpinePos(float *out, u_int oid, int flag)
{
    int no = (short)g_gamMan.getActorNo(oid);
    if ((u_int)no >= 0x80)
        return 0.0f;
    ROR_ASSERT(m_scene); /* 570 */
    DogActor &actor = m_scene->m_actors[no];
    int n = actor.getNodeIndexByName("p_fuckme");   // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)
    if (n < 0)
    {
        n = actor.getNodeIndexByName("Bip01 Spine1");
        ROR_ASSERT(n >= 0); /* 590 */
    }
    if (n >= 0)     /* PC: the ROM's assert never returns */
        actor.getNodeWorldPos(out, n);
    if (flag == 0)
        return g_emyManager.getEmyTypeDist2(oid);
    return g_emyManager.getEmyTypeDist(oid);
}

// ROR 0x0011d028

float MagLifePC::getEmyTypeRate(u_int oid)
{
    return g_emyManager.getEmyTypeRate(oid);
}

// ROR 0x0011d044

int MagLifePC::canReload()
{
    return g_gamMan.findReloadSlot(NULL) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011d06c

void MagLifePC::reload()
{
    g_gamMan.reloadEquip(-1);
    if (m_shots > 0)
    {
        EffParam param;
        memset(&param, 0, sizeof(param));   /* PC: the ROM leaves the rest stack garbage */
        param.m_kind = 0x27;
        param._04 = 0;
        param.m_08 = 0;
        for (int i = 0; i < 3; i++)
        {
            param.m_reload.m_pos[i] = m_actor->m_trans[i];
            param.m_reload.m_rot[i] = m_actor->m_rotate[i];
        }
        param.m_reload.m_shots = m_shots;
        m_scene->m_eff_man.createEff(&param);
    }
    m_shots = 0;
    int chr = getEquipItemChrNo();
    if (chr >= 0)
    {
        DogActor *item = &m_scene->m_actors[chr];
        int no = item->getMotionNoBySuffix("reload");
        item->changeMotion(0x1.11112Ep-6f, no, 0, -1, -1); /* 0x3c888897 */
    }
}

// ROR 0x0011d15c

int MagLifePC::getSelItemNum()
{
    return g_gamMan.getSelItemNum();
}

// ROR 0x0011d178

void MagLifePC::decSelItemNum()
{
    g_gamMan.decSelItemNum();
    m_shots++;
}

// ROR 0x0011d1b0

bool MagLifePC::testEmyHittable(u_int oid)
{
    return g_emyManager.testEmyHittable(oid);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011d1cc // callcheck: dbgAssertFailed (the assert handler)

int MagLifePC::getAttackSeNo(int theHeight, int theCombo, int kind)
{
    ROR_ASSERT(theHeight >= ATK_HEIGHT_NORMAL && theHeight <= ATK_HEIGHT_STOMP); /* 698 */
    ROR_ASSERT(theCombo >= 0 && theCombo < 3); /* 699 */
    const MagLifePCAttackSeRow *r;
    if (kind == 0)
    {
        if (theHeight == ATK_HEIGHT_STOMP)
            return 0x1c;
        int item = getEquipItem();
        if (item == ITEM_NONE || item == ITEM_EMPTY)
            return theHeight == 2 ? 0x43 : 0x41;
        if (item == ITEM_GUN_A || item == ITEM_GUN_B)
            return getSelItemNum() < 1 ? 0x80 : 0x7f;
        for (r = s_attackSeRow; r->m_item >= 0 && r->m_item != item; r++)
            ;
        return s_attackSe[r->m_row < 0 ? 0 : r->m_row][theCombo];
    }
    if (theHeight == ATK_HEIGHT_STOMP)
        return 0x44;
    int item = getEquipItem();
    if (item == ITEM_NONE || item == ITEM_EMPTY)
        return theHeight == 2 ? 0x44 : 0x42;
    if (item == ITEM_GUN_A || item == ITEM_GUN_B)
        return 0x83;
    for (r = s_attackSeRow; r->m_item >= 0 && r->m_item != item; r++)
        ;
    if (r->m_row < 0)
        return 0x47;
    return s_attackSe[r->m_row][3];
}

// ──────────────────────────────────────────────────────────────────────
// Types missing from the table use DWF's record.
// ROR 0x0011d3d0 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (the assert handler; DogModel::getNodeIndexByName inline)

int MagLifePC::getAttackHeightByDist(float dist, u_int oid)
{
    int type = g_emyManager.getEmyType(oid);
    if (type < 0)
        return -1;
    int rec;
    if (type == EMY_TYPE_HOFFMAN)
    {
        rec = HEIGHT_REC_HOFFMAN_LOW;
        int no = (short)g_gamMan.getActorNo(oid);
        if (no >= 0)
        {
            ROR_ASSERT(m_scene); /* 937 */
            DogActor &actor = m_scene->m_actors[no];
            int n = actor.getNodeIndexByName("Bip01 Neck");
            float neck[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            /* PC: without the model's matrices (not loaded) the neck stays 0 */
            if (actor.getModelMatrix() != NULL)
                actor.getNodeWorldPos(neck, n < 0 ? 0 : n);
            if (neck[1] >= -140.0f)
                rec = HEIGHT_REC_HOFFMAN;
        }
    }
    else
    {
        for (rec = 0; rec < HEIGHT_REC_TYPED; rec++)
            if (s_heightRec[rec].m_emy_type == type)
                break;
        if (rec >= HEIGHT_REC_TYPED)
            rec = 0;
    }

    const MagLifePCHeightBand *b = s_heightRec[rec].m_band;
    for (int i = 0; i < 4; i++)
    {
        if (dist < b[i].m_max && dist >= b[i].m_min)
            return s_bandHeight[i];
    }
    return -1;
}
