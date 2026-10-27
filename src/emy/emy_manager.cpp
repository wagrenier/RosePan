/* ==========================================================================
 *  emy/emy_manager.cpp
 *
 *  EmyManager (0x16265c..0x164118): the task that owns the enemies. Up to
 *  64 EmyMain objects in m_emy, created by type in createEmy (the eleven
 *  classes' inline constructors), run each frame by exec -- the group
 *  spawner first, then every enemy's function pointer, group step and
 *  virtual slots 4..6 -- and a long row of small wrappers that find an
 *  enemy by oid for GamMan, the scripts and the scene tasks. Its .data
 *  holds the per-type tables 0x2f12a0..0x2f14a0 (footsteps, distances,
 *  voices).
 * ======================================================================== */
#include "emy/emy_manager.h"

#include <string.h>

#include "game/gamman.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"

/* The footstep sound ids an enemy's actor gets (MagScene::m_actor_snd), 12
 * per table by sector sound slot, -1 none (data 0x2f12a0..0x2f1360). */
static int s_emyFootSnd[12] = {                                          // data 0x002f12a0: types 0, 2..5
    -1, 0x0c00036b, 0x0c00036a, 0x0c00036a, 0x0c00036b, 0x0c00036c,
    0x0c00036c, 0x0c00036b, 0x0c00036a, 0x0c00036c, 0x0c00036c, 0x0c00036c,
};
static int s_pigFootSnd[12] = {                                          // data 0x002f12d0: type 1
    -1, 0x0c00036e, 0x0c00036d, 0x0c00036d, 0x0c00036e, 0x0c00036f,
    0x0c00036f, 0x0c00036e, 0x0c00036d, 0x0c00036f, 0x0c00036f, 0x0c00036f,
};
static int s_hfmFootSnd[12] = {                                          // data 0x002f1300: type 6
    -1, 0x1f000547, 0x1f000549, 0x1f000549, 0x1f000547, 0x1f000547,
    0x1f000547, 0x1f000547, 0x1f000549, 0x1f000547, 0x1f000547, 0x1f000547,
};
static int s_norFootSnd[12] = {                                          // data 0x002f1330: type 10
    -1, 0x1f020628, 0x1f02062a, 0x1f02062a, 0x1f020628, 0x1f020628,
    0x1f020628, 0x1f020628, 0x1f02062a, 0x1f020628, 0x1f020628, 0x1f020628,
};

/* Per-type values by GetType (data 0x2f1360 / 0x2f1390 / 0x2f13c0 /
 * 0x2f1470); roles not read yet. */
static const float s_emyTypeDist[EMY_TYPE_MAX] = {                       // data 0x002f1360
    30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 40.0f, 50.0f, 50.0f, 50.0f, 60.0f, 70.0f, 50.0f,
};
static const float s_emyTypeDist2[EMY_TYPE_MAX] = {                      // data 0x002f1390
    30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 40.0f, 50.0f, 50.0f, 50.0f, 60.0f, 70.0f, 50.0f,
};
static const float s_emyTypeRate[EMY_TYPE_MAX] = {                       // data 0x002f13c0
    0x1.333332p-2f /* 0x3e999999 */, 0.7f, 0.5f, 0x1.999998p-3f /* 0x3e4ccccc */,
    0.5f, 0.7f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f,
};
static const int s_emyTypeFlag[EMY_TYPE_MAX] = {                         // data 0x002f1470
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

/* Voice SE ids by kind (rows; kind 7 and above the last) and the enemy's
 * voice byte (columns), types 0..3 (data 0x2f13f0..0x2f1470). */
static const u_int s_emyVoiceSE[8][4] = {                                // data 0x002f13f0
    { 0x0c000382, 0x0c00038a, 0x0c000382, 0x0c00038a },
    { 0x0c000384, 0x0c00038c, 0x0c000384, 0x0c00038c },
    { 0x0c000385, 0x0c00038d, 0x0c000385, 0x0c00038d },
    { 0x460d0001, 0x460d0001, 0x460d0001, 0x460d0001 },
    { 0x460d0002, 0x460d0002, 0x460d0002, 0x460d0002 },
    { 0x460d0003, 0x460d0003, 0x460d0003, 0x460d0003 },
    { 0x460d0004, 0x460d0004, 0x460d0004, 0x460d0004 },
    { 0x460d0006, 0x460d0006, 0x460d0006, 0x460d0006 },
};

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016265c

EmyManager::EmyManager()
{
    clear();
    memset(m_max_num, 100, sizeof(m_max_num));
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00162698

void EmyManager::clear()
{
    memset(m_emy, 0, sizeof(m_emy));
    memset(m_work, 0, sizeof(m_work));
    m_oid_on[0] = 0;
    m_oid_on[1] = 0;
    m_oid[0] = 0;
    m_oid[1] = 0;
    memset(m_grp, 0, sizeof(EmyGroup) * EMY_GROUP_MAX);
    m_oid_count = 0;
    m_level = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00162700 // callcheck: operator_delete (the deleting variant)

EmyManager::~EmyManager()
{
    releaseAll();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016275c

int EmyManager::exec(int id, int time, int arg)
{
    (void)id;
    (void)time;
    (void)arg;
    spawnGroups();
    updateEmys();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Groups 1..7, while on and no member is appearing and fewer than m_max
// are out: the first idle member (state 0) starts waiting (the group's
// first one at once, the others a random part of the interval); with no
// idle one, the gone member (state 5) whose respawn number is next comes
// back (state 4).
// ROR 0x0016278c

void EmyManager::spawnGroups()
{
    for (int no = 1; no < EMY_GROUP_MAX; no++)
    {
        EmyGroup *grp = &m_grp[no];
        if (grp->m_active == 0 || grp->m_busy != 0 || grp->m_alive >= grp->m_max)
            continue;
        int i;
        for (i = 0; i < EMY_MAX; i++)
        {
            EmyMain *emy = m_emy[i];
            if (emy == NULL || emy->m_grp_no != no || emy->m_grp_state != 0)
                continue;
            if (grp->m_first == 0)
            {
                emy->m_grp_wait = 0;
                grp->m_first = 1;
            }
            else
            {
                int half = grp->m_interval / 2;
                emy->m_grp_wait = (short)utlRandRange(half, grp->m_interval + half);
            }
            emy->m_grp_state = 1;
            grp->m_busy = 1;
            grp->m_alive++;
            break;
        }
        if (i != EMY_MAX)
            continue;
        for (i = 0; i < EMY_MAX; i++)
        {
            EmyMain *emy = m_emy[i];
            if (emy == NULL || emy->m_grp_no != no || emy->m_grp_state != 5
                || emy->m_grp_respawn != grp->m_respawn)
                continue;
            float pos[4];
            emy->m_param->getPos(pos);
            /* The ROM builds a 0x28-byte record on the stack here -- 3, the
             * position with the floor height as y, -1.0f, 100.0f, 90 -- and
             * never passes it on; only the floor query is left of it. */
            g_gamMan.getFloorY(pos);
            int half = grp->m_interval / 2;
            emy->m_grp_wait = (short)utlRandRange(half, grp->m_interval + half);
            emy->m_grp_state = 4;
            emy->m_grp_1d2 = 0;
            emy->m_grp_respawn = 0;
            grp->m_busy = 1;
            grp->m_respawn++;
            grp->m_alive++;
            break;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// Each enemy: its function pointer; a group member its group step, and
// only once it is out (state 3) the rest; managed ones then count their
// damage end down and run their out and slots 4..6, or while damaged
// (param flags 0x200) only stop their out.
// ROR 0x001629c0

void EmyManager::updateEmys()
{
    for (int i = 0; i < EMY_MAX; i++)
    {
        EmyMain *emy = m_emy[i];
        if (emy == NULL)
            continue;
        emy->callFunc();
        if (emy->m_grp_no != 0)
        {
            EmyGroup *grp = &m_grp[emy->m_grp_no];
            emy->updateGroup(grp->m_f, grp->m_motion, grp->m_pos);
            if (emy->m_grp_state != 3)
                continue;
        }
        if (emy->m_manage_sw == 0)
            continue;
        if ((emy->m_param->m_flags & 0x200) == 0)
        {
            if (emy->m_damage_end > 0)
                emy->countDamageEnd();
            emy->updateOut();
            emy->decide();
            emy->exec();
            emy->after();
        }
        else
        {
            emy->stopOut();
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00162afc

void EmyManager::setSlotWord24(int no, int v)
{
    m_grp[no].m_active = v;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00162b14

void EmyManager::countGrpDead(int no)
{
    m_grp[no].m_dead++;
    m_grp[no].m_alive--;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00162b50

void EmyManager::clearGrpBusy(int no)
{
    m_grp[no].m_busy = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00162b68

void EmyManager::setGroup(float f, int a, int b, int c, int count, int d, const float *pos)
{
    if (a >= 9 || b >= 0x11 || count >= 0x11)
        return;
    EmyGroup *grp = &m_grp[a];
    if (c < 60)
        c = 60;
    grp->m_active = 1;
    grp->m_first = 0;
    grp->m_max = b;
    grp->m_interval = c;
    grp->m_count = count;
    grp->m_motion = d;
    grp->m_pos[0] = pos[0];
    grp->m_pos[1] = pos[1];
    grp->m_pos[2] = pos[2];
    grp->m_f = f;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00162c2c

u_int EmyManager::addEmy(int type, short name, ACHRstat *chr, const _tagEnemyParam *prm,
                         const _tagEnemyTransition *trans, float *pos, u_int oid, int group)
{
    return createEmy(type, name, chr, prm, trans, pos, oid, group, 1, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00162c5c

u_int EmyManager::addEmyAlt(int type, short name, ACHRstat *chr, const _tagEnemyParam *prm,
                            const _tagEnemyTransition *trans, float *pos, u_int oid, short carry)
{
    return createEmy(type, name, chr, prm, trans, pos, oid, 0, 0, carry);
}

// ──────────────────────────────────────────────────────────────────────
// A free slot, an oid (oid 0 and makeOid: 0x9100006f + m_oid_count, at
// most 16), no other enemy of the type with this oid and fewer than the
// type's limit; then the type's class (the ELF's inline XxxMain
// constructors), its start state (type 0 without makeOid: state 4 and the
// carried item word as param 0x5e), its param, its held item (types 0..2),
// its actor's footstep table (all but 7..9), its group and its first out.
// ROR 0x00162c88 // callcheck: EmyMain, initOut, operator_new (new XxxMain: the inline constructors call EmyMain's and initOut; set_achr is inline too)

u_int EmyManager::createEmy(int type, short name, ACHRstat *chr, const _tagEnemyParam *prm,
                            const _tagEnemyTransition *trans, float *pos, u_int oid,
                            int group, int makeOid, short carry)
{
    if (group > 8)
        return 0;
    if (type > 10)
        return 0;
    int slot = 0;
    while (m_emy[slot] != NULL)
    {
        slot++;
        if (slot == EMY_MAX)
            return 0;
    }
    if (oid == 0)
    {
        if (makeOid == 0)
            return 0;
        if (m_oid_count > 15)
            return 0;
        oid = m_oid_count + 0x9100006f;
        g_gamMan.set_achr(oid, chr);
        m_oid_count++;
    }
    u_int num = 0;
    for (int i = 0; i < EMY_MAX; i++)
    {
        EmyMain *emy = m_emy[i];
        if (emy != NULL && emy->GetType() == type)
        {
            if (emy->m_oid == oid)
                return 0;
            num++;
        }
    }
    if (m_max_num[type] != 0 && m_max_num[type] < num)
        return 0;

    u_short kind = (u_short)name;
    int *footSnd = NULL;
    EmyMain *emy;
    switch (type)
    {
    case 0:
        emy = m_emy[slot] = new DwfMain(oid, kind, chr, prm, trans, pos);
        if (makeOid == 0)
        {
            emy->ForceSetState(4);
            chr->setParam(0x5e, &carry);
            emy->setStartState(4, trans);
        }
        else
        {
            emy->setStartState(prm->m_start_state, trans);
        }
        emy->loadParam(prm);
        emy->createHeldItem();
        footSnd = s_emyFootSnd;
        break;
    case 1:
        emy = m_emy[slot] = new PigMain(oid, kind, chr, prm, trans, pos);
        emy->setStartState(prm->m_start_state, trans);
        emy->loadParam(prm);
        emy->createHeldItem();
        footSnd = s_pigFootSnd;
        break;
    case 2:
        emy = m_emy[slot] = new GotMain(oid, kind, chr, prm, trans, pos);
        emy->setStartState(prm->m_start_state, trans);
        emy->loadParam(prm);
        emy->createHeldItem();
        footSnd = s_emyFootSnd;
        break;
    case 3:
        emy = m_emy[slot] = new FshMain(oid, kind, chr, prm, trans, pos);
        emy->setStartState(prm->m_start_state, trans);
        emy->loadParam(prm);
        footSnd = s_emyFootSnd;
        break;
    case 4:
        emy = m_emy[slot] = new BdsMain(oid, kind, chr, prm, trans, pos);
        emy->setStartState(prm->m_start_state, trans);
        emy->loadParam(prm);
        footSnd = s_emyFootSnd;
        break;
    case 5:
        emy = m_emy[slot] = new BdlMain(oid, kind, chr, prm, trans, pos);
        emy->setStartState(prm->m_start_state, trans);
        emy->loadParam(prm);
        footSnd = s_emyFootSnd;
        break;
    case 6:
    {
        HfmMain *hfm = new HfmMain(oid, kind, chr, prm, trans, pos);
        emy = m_emy[slot] = hfm;
        emy->setStartState(prm->m_start_state, trans);
        emy->loadParam(prm);
        hfm->initAfterParam();
        footSnd = s_hfmFootSnd;
        break;
    }
    case 7:
        emy = m_emy[slot] = new MrtMain(oid, kind, chr, prm, trans, pos);
        emy->setStartState(prm->m_start_state, trans);
        emy->loadParam(prm);
        break;
    case 8:
        emy = m_emy[slot] = new ClrMain(oid, kind, chr, prm, trans, pos);
        emy->setStartState(prm->m_start_state, trans);
        emy->loadParam(prm);
        break;
    case 9:
        emy = m_emy[slot] = new GrgMain(oid, kind, chr, prm, trans, pos);
        emy->setStartState(prm->m_start_state, trans);
        emy->loadParam(prm);
        break;
    case 10:
        emy = m_emy[slot] = new NorMain(oid, kind, chr, prm, trans, pos);
        emy->setStartState(prm->m_start_state, trans);
        emy->loadParam(prm);
        footSnd = s_norFootSnd;
        break;
    default:
        return 0;
    }
    if (footSnd != NULL)
        g_magSystem.m_scene.m_actor_snd[g_gamMan.getActorNo(oid)] = footSnd;
    emy->setGroup(group);
    emy->selectOut();
    if (group == 0)
        m_grp[0].m_motion = 30;
    g_gamMan.setActorBindMode(oid, 1);
    return oid;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001634d4 // callcheck: EmyMain::~EmyMain (virtual deleting destructor)

void EmyManager::releaseObj(u_int oid)
{
    for (int i = 0; i < EMY_MAX; i++)
    {
        EmyMain *emy = m_emy[i];
        if (emy == NULL || emy->m_oid != oid)
            continue;
        delete emy;
        m_emy[i] = NULL;
        m_work[i] = 0;
        return;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163570 // callcheck: EmyMain::~EmyMain (virtual deleting destructor)

void EmyManager::releaseAll()
{
    for (int i = 0; i < EMY_MAX; i++)
    {
        if (m_emy[i] != NULL)
            delete m_emy[i];
    }
    clear();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001635e0

void EmyManager::resetWork()
{
    m_oid_on[0] = 0;
    m_oid_on[1] = 0;
    m_oid[0] = 0;
    m_oid[1] = 0;
    memset(m_grp, 0, sizeof(EmyGroup) * EMY_GROUP_MAX);
    m_oid_count = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163624

void EmyManager::notifyEmy(u_int oid, int bits)
{
    EmyMain *emy = findEmy(oid);
    if (emy != NULL)
        emy->notify(bits);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016366c

void EmyManager::setEmyActive(u_int oid, int on)
{
    EmyMain *emy = findEmy(oid);
    if (emy != NULL)
        emy->setManageSw(on);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001636a4

EmyMain *EmyManager::findEmy(u_int oid)
{
    for (int i = 0; i < EMY_MAX; i++)
    {
        EmyMain *emy = m_emy[i];
        if (emy != NULL && emy->m_oid == oid)
            return emy;
    }
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001636e4

bool EmyManager::hasOid(int which)
{
    if (which != 0)
        return m_oid_on[1] != 0;
    return m_oid_on[0] != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163700

void EmyManager::setOid(int which, u_int oid)
{
    if (which == 0)
    {
        m_oid[0] = oid;
        m_oid_on[0] = 1;
        return;
    }
    m_oid[1] = oid;
    m_oid_on[1] = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163720

void EmyManager::clearOid(int which)
{
    if (which != 0)
    {
        m_oid[1] = 0;
        m_oid_on[1] = 0;
        return;
    }
    m_oid[0] = 0;
    m_oid_on[0] = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016373c

bool EmyManager::testEmyHittable(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return false;
    return emy->testHittable();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163770

bool EmyManager::testEmyStateFlag(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return false;
    return emy->testStateFlag();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001637a4

int EmyManager::isEmyVisible(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0;
    return emy->IsVisible();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001637cc

int EmyManager::isEmyTarget(u_int oid)
{
    for (int i = 0; i < EMY_MAX; i++)
    {
        EmyMain *emy = m_emy[i];
        if (emy == NULL)
            continue;
        switch (emy->GetType())
        {
        case 0:
        case 1:
        case 2:
            if (emy->m_state == 0xb)
            {
                if (emy->GetEmyOut()->m_3c == oid)
                    return 1;
            }
            else if (emy->m_state == 0xc)
            {
                if (emy->GetEmyOut()->m_38 == oid)
                    return 1;
            }
            break;
        default:
            break;
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001638fc

float EmyManager::getEmyTypeDist(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0.0f;
    return s_emyTypeDist[emy->GetType()];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163958

float EmyManager::getEmyTypeDist2(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0.0f;
    return s_emyTypeDist2[emy->GetType()];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001639b4

float EmyManager::getEmyTypeRate(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0.0f;
    return s_emyTypeRate[emy->GetType()];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163a10

bool EmyManager::isEmyDamaging(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return false;
    return emy->IsDamaging();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163a44

void EmyManager::cancelEmyDamage(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy != NULL)
        emy->CancelDamage();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163a8c

bool EmyManager::isEmyState2(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return false;
    return emy->GetState2() != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163ab8

void EmyManager::getOids(int *a, int *b)
{
    *a = (int)m_oid[0];
    *b = (int)m_oid[1];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163acc

void EmyManager::setEmyDownFlg(u_int oid, int v)
{
    EmyMain *emy = findEmy(oid);
    if (emy != NULL)
        emy->SetDownFlg(v);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163af8

int EmyManager::getEmyDamage(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0;
    return emy->GetDamage();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163b20

int EmyManager::getEmyWord3c(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0;
    return emy->GetState();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163b48

bool EmyManager::setEmyWord238(u_int oid, int v)
{
    EmyMain *emy = findEmy(oid);
    if (emy != NULL)
        emy->m_238 = v;
    return emy != NULL;
}

// ──────────────────────────────────────────────────────────────────────
// No type check: the ROM calls GrgMain's code on any enemy.
// ROR 0x00163b80

int EmyManager::testEmyView(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 1;
    return static_cast<GrgMain *>(emy)->testView();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163bb8

void EmyManager::releaseEmyHold(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy != NULL)
        static_cast<GrgMain *>(emy)->releaseHold();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163be4

int EmyManager::getEmyType(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0;
    return emy->GetType();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163c24

void EmyManager::setEmyWord18(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy != NULL)
        emy->SetDownSupine();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163c50

void EmyManager::clearEmyWord18(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy != NULL)
        emy->SetDownProne();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163c74

u_int EmyManager::getEmyVoiceSE(u_int oid, int kind)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0;
    if ((u_int)emy->GetType() >= 4)
        return 0;
    if ((u_int)kind > 6)
        kind = 7;
    return s_emyVoiceSE[kind][emy->GetVoice()];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163d90

int EmyManager::getEmyWord1d8(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0;
    return emy->RefLP();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163db8

int EmyManager::getEmyTypeFlag(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0;
    return s_emyTypeFlag[emy->GetType()];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163e10

void EmyManager::pauseEmy(u_int oid, int on)
{
    EmyMain *emy = findEmy(oid);
    if (emy != NULL)
        emy->pauseMove(on);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163e48 // callcheck: stop (inside the inline EmyMain::InactiveMove)

void EmyManager::stopEmyMove(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy != NULL)
        emy->InactiveMove();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163e74

void EmyManager::setEmyMode6(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy != NULL)
        emy->SetDamageEnd();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163ea0

u_int EmyManager::getEmySE(u_int oid, int no)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0;
    return emy->getSE(no);
}

// ──────────────────────────────────────────────────────────────────────
// No type check: the ROM calls HfmMain's code on any enemy.
// ROR 0x00163ee4

void EmyManager::subEmyFreeCnt(u_int oid, int n)
{
    EmyMain *emy = findEmy(oid);
    if (emy != NULL)
        static_cast<HfmMain *>(emy)->subFreeCnt(n);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163f1c

int EmyManager::getEmyWord238(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0;
    return emy->m_238;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163f44

int EmyManager::getEmyHoldWord24c(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0;
    if (emy->GetType() != 10)
        return 0;
    return static_cast<NorMain *>(emy)->m_24c;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163f9c

int EmyManager::getEmyHoldWord23c(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0;
    if (emy->GetType() != 6)
        return 0;
    return static_cast<HfmMain *>(emy)->m_23c;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00163ff4

int EmyManager::getEmyHoldWord248(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return 0;
    if (emy->GetType() != 8)
        return 0;
    return static_cast<ClrMain *>(emy)->m_248;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016404c

void EmyManager::resetEmyOut(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy != NULL)
        emy->resetOut();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00164078

bool EmyManager::isEmyFlag30(u_int oid)
{
    EmyMain *emy = findEmy(oid);
    if (emy == NULL)
        return false;
    return emy->testNotifyHit();
}

// ──────────────────────────────────────────────────────────────────────
// Every slot holding the oid's enemy (the ROM does not stop at the first).
// ROR 0x001640ac

void EmyManager::setEmyWork(u_int oid, int v)
{
    for (int i = 0; i < EMY_MAX; i++)
    {
        EmyMain *emy = m_emy[i];
        if (emy != NULL && emy->m_oid == oid)
            m_work[i] = v;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001640e8

int EmyManager::countEmyWork()
{
    int n = 0;
    for (int i = EMY_MAX - 1; i >= 0; i--)
    {
        if (m_work[i] != 0)
            n++;
    }
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// The ELF's linkonce copy of EmyMain's inline InactiveMove (stopEmyMove
// inlines it): defined here, where its only caller is, because
// TskEnemyMove's class is not complete in emy/emy_main.h.
// ROR 0x0028a93c

void EmyMain::InactiveMove()
{
    m_move->stop();
}
