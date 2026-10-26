/* ==========================================================================
 *  game/gamstat.cpp
 *
 *  The game objects' state (pinned 0x1d84c8..0x1dd1e8, between RoR's own
 *  game/gamstat.cpp and game/gamtrg.cpp anchors): ObjStat and its
 *  ACHRstat / FCHRstat kinds -- position, rotation, secta cell, params --
 *  and GamSysData, the system pack's record tables. See gamstat.h.
 * ======================================================================== */
#include "game/gamstat.h"
#include "common/rosepan_stub.h"

#include <string.h>

#include "game/gamman.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"

/* The scene actor an object drives (MagScene's array at bss 0x6fe6a0). */
static inline DogActor &gamActor(int no)
{
    return g_magSystem.m_scene.m_actors[no];
}

// ──────────────────────────────────────────────────────────────────────
// The pool at +0x40 is built as SeqPool::build does (the ROM first empties
// both lists).
// ROR 0x001d87f4

ObjStat::ObjStat(int a1, u_int oid, int actor, short kind)
{
    m_nodes.build(m_nodes.m_storage, 8);
    m_oid = oid;
    m_kind = kind;
    m_actor = actor;
    m_stat = a1;
    m_140 = 0;
    m_secta = NULL;
    m_110 = NULL;
    m_areaNo = 0;
    // PORT: the ROM leaves the rest to init and to what the heap held;
    // the host starts it cleared, so an object read before init is defined.
    m_sectaLast = NULL;
    m_smellRange = 0.0f;
    m_scale = 0.0f;
    m_flags = 0;
    m_124 = 0;
    m_lp = 0;
    m_smellUid[0] = m_smellUid[1] = m_smellUid[2] = 0;
    m_134 = 0;
    m_inuFunc = 0;
    m_13c = 0;
    m_144 = 0;
    m_148 = 0;
    m_14c = 0;
    memset(m_colPrm, 0, sizeof(m_colPrm));
    memset(m_180, 0, sizeof(m_180));
    memset(m_pos, 0, sizeof(m_pos));
    memset(m_rot, 0, sizeof(m_rot));
    memset(m_work, 0, sizeof(m_work));
    memset(m_quad, 0, sizeof(m_quad));
    memset(m_name, 0, sizeof(m_name));
    memset(m_colPrmKeep, 0, sizeof(m_colPrmKeep));
    _250 = 0;
    m_link = -1;
}

// ──────────────────────────────────────────────────────────────────────
// Written ahead for GamMan::clearChrSectaPtrs.
// ROR 0x001d88fc

void ObjStat::clearSectaPtrs()
{
    m_sectaLast = NULL;
    m_secta = NULL;
    m_110 = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d890c

void ObjStat::getPos(float *out)
{
    if (m_actor != -1)
    {
        const DogActor &actor = gamActor(m_actor);
        out[0] = actor.m_trans[0];
        out[1] = actor.m_trans[1];
        out[2] = actor.m_trans[2];
        return;
    }
    out[0] = m_pos[0];
    out[1] = m_pos[1];
    out[2] = m_pos[2];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d8964

void ObjStat::getRot(float *out)
{
    if (m_actor != -1)
    {
        const DogActor &actor = gamActor(m_actor);
        out[0] = actor.m_rotate[0];
        out[1] = actor.m_rotate[1];
        out[2] = actor.m_rotate[2];
        return;
    }
    out[0] = m_rot[0];
    out[1] = m_rot[1];
    out[2] = m_rot[2];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d89c4

void ObjStat::setPos(const float *pos)
{
    m_pos[0] = pos[0];
    m_pos[1] = pos[1];
    m_pos[2] = pos[2];
    if (m_actor == -1)
        return;
    DogActor &actor = gamActor(m_actor);
    actor.m_trans[0] = m_pos[0];
    actor.m_trans[1] = m_pos[1];
    actor.m_trans[2] = m_pos[2];
    actor.m_dirty = 1;
    if (m_secta == NULL || (m_flags & 0x40) != 0)
        setSecta(g_gamMan.findSectaCell(m_pos, m_secta, gamOidType(m_oid) == 2));
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d8a8c

void ObjStat::setRot(const float *rot)
{
    m_rot[0] = rot[0];
    m_rot[1] = rot[1];
    m_rot[2] = rot[2];
    if (m_actor != -1)
        g_magSystem.setActorRotate(m_actor, m_rot);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d8d28

int ObjStat::setSecta(GamSecta *cell)
{
    if ((m_flags & 0x40) == 0)
    {
        m_110 = cell;
        m_secta = cell;
        return 0;
    }
    if ((m_flags & 0x10000000) == 0)
        g_gamMan.queueObjUpdate(cell, m_oid);
    m_secta = cell;
    m_110 = cell;
    if (cell != NULL)
    {
        m_sectaLast = cell;
        m_quad[3] = m_pos[3];
        m_quad[0] = m_pos[0];
        m_quad[1] = m_pos[1];
        m_quad[2] = m_pos[2];
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d8dc4

void ObjStat::syncActorPos()
{
    if (m_actor == -1)
        return;
    const DogActor &actor = gamActor(m_actor);
    m_pos[0] = actor.m_trans[0];
    m_pos[1] = actor.m_trans[1];
    m_pos[2] = actor.m_trans[2];
    GamSecta *cell = m_secta;
    if (cell != NULL && !(m_pos[0] < cell->m_x0) && !(cell->m_x1 < m_pos[0]) &&
        !(m_pos[2] < cell->m_z0) && m_pos[2] <= cell->m_z1)
        return;
    GamSecta *next = g_gamMan.findSectaCell(m_pos, cell, gamOidType(m_oid) == 2);
    if (next != NULL)
        setSecta(next);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d8ea8

void ObjStat::findSecta()
{
    GamSecta *cell = g_gamMan.findSectaCell(m_pos, m_secta, gamOidType(m_oid) == 2);
    if (cell != NULL)
    {
        m_110 = NULL;
        setSecta(cell);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d8f04

void ObjStat::updatePos(float *out)
{
    // PORT: the ROM reads actor -1's slot (before the array) for an object
    // without an actor; its callers (the PC, the dog) always have one.
    if (m_actor != -1)
    {
        const DogActor &actor = gamActor(m_actor);
        m_pos[0] = actor.m_trans[0];
        m_pos[1] = actor.m_trans[1];
        m_pos[2] = actor.m_trans[2];
    }
    out[0] = m_pos[0];
    out[1] = m_pos[1];
    out[2] = m_pos[2];
    out[3] = m_pos[3];
    GamSecta *cell = m_secta;
    if (cell != NULL && !(m_pos[0] < cell->m_x0) && !(cell->m_x1 < m_pos[0]) &&
        !(m_pos[2] < cell->m_z0) && m_pos[2] <= cell->m_z1)
        return;
    GamSecta *next = g_gamMan.findSectaCell(m_pos, cell, gamOidType(m_oid) == 2);
    if (next != NULL)
        setSecta(next);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d9044

void ObjStat::getParam(int which, void *out)
{
    switch (which)
    {
    case GAM_PRM_O_OID:
        *(u_int *)out = m_oid;
        break;
    case GAM_PRM_O_UID:
        *(short *)out = m_kind;
        break;
    case GAM_PRM_O_CHR_ID:
        *(int *)out = m_actor;
        break;
    case GAM_PRM_O_SECTA_ID:
        *(int *)out = m_secta != NULL ? m_secta->m_id : -1;
        break;
    case GAM_PRM_O_POS:
        getPos((float *)out);
        break;
    case GAM_PRM_O_ROT:
        getRot((float *)out);
        break;
    case GAM_PRM_O_STAT:
        *(int *)out = m_stat;
        break;
    case GAM_PRM_O_AREA_NO:
        *(int *)out = m_areaNo;
        break;
    case GAM_PRM_O_INU_FUNC:
        *(int *)out = m_inuFunc;
        break;
    case GAM_PRM_O_SMELL_UID_1:
    case GAM_PRM_O_SMELL_UID_2:
    case GAM_PRM_O_SMELL_UID_3:
        *(u_short *)out = m_smellUid[which - GAM_PRM_O_SMELL_UID_1];
        break;
    case GAM_PRM_O_SMELL_RANGE:
        *(float *)out = m_smellRange;
        break;
    case GAM_PRM_O_SCALE:
        *(float *)out = m_scale;
        break;
    case GAM_PRM_COL_PRM:
        memcpy(out, m_colPrm, sizeof(m_colPrm));
        break;
    case 0x1000:
        *(int *)out = m_148;
        break;
    case 0x1001:
        memcpy(out, m_180, sizeof(m_180));
        break;
    case 0x1002:
        *(u_short *)out = m_124;
        break;
    case 0x1003:
        memcpy(out, m_quad, sizeof(m_quad));
        break;
    case 0x1004:
        strncpy((char *)out, m_name, 0x10);
        break;
    case 0x1005:
        memcpy(out, m_colPrmKeep, sizeof(m_colPrmKeep));
        break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Position and rotation go through ObjStat's setters, also for an fchr.
// ROR 0x001d932c

void ObjStat::setParam(int which, const void *data)
{
    switch (which)
    {
    case GAM_PRM_O_OID:
        m_oid = *(const u_int *)data;
        break;
    case GAM_PRM_O_UID:
        m_kind = *(const short *)data;
        break;
    case GAM_PRM_O_CHR_ID:
        m_actor = *(const int *)data;
        break;
    case GAM_PRM_O_POS:
        setPos((const float *)data);
        break;
    case GAM_PRM_O_ROT:
        setRot((const float *)data);
        break;
    case GAM_PRM_O_STAT:
        m_stat = *(const int *)data;
        break;
    case GAM_PRM_O_AREA_NO:
        m_areaNo = *(const int *)data;
        break;
    case GAM_PRM_O_SMELL_UID_1:
    case GAM_PRM_O_SMELL_UID_2:
    case GAM_PRM_O_SMELL_UID_3:
        m_smellUid[which - GAM_PRM_O_SMELL_UID_1] = *(const u_short *)data;
        break;
    case GAM_PRM_O_SMELL_RANGE:
        m_smellRange = *(const float *)data;
        break;
    case GAM_PRM_O_SCALE:
        m_scale = *(const float *)data;
        break;
    case GAM_PRM_COL_PRM:
        memcpy(m_colPrmKeep, data, sizeof(m_colPrmKeep));
        memcpy(m_colPrm, data, sizeof(m_colPrm));
        break;
    case 0x1000:
        m_148 = *(const int *)data;
        break;
    case 0x1001:
        memcpy(m_180, data, sizeof(m_180));
        break;
    case 0x1002:
        m_124 = *(const u_short *)data;
        break;
    case 0x1003:
        memcpy(m_quad, data, sizeof(m_quad));
        break;
    case 0x1004:
        strncpy(m_name, (const char *)data, 0x10);
        break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d95c4

void ObjStat::setHide(int hide)
{
    if (hide == 0)
    {
        if ((m_flags & 0x40) != 0)
            findSecta();
    }
    else if ((m_flags & 0x40) != 0)
    {
        g_gamMan.relinkObj(m_oid);
    }
    if (m_actor != -1)
        gamActor(m_actor).setHide(hide != 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d966c

void ObjStat::releaseActor()
{
    if (m_actor != -1)
        g_magSystem.m_scene.releaseActor(m_actor);
    m_actor = -1;
    if (gamOidType(m_oid) == 5)
        g_gamMan.ctlObj1cbb80(m_oid);
}

// ──────────────────────────────────────────────────────────────────────
// The ROM copies +0x100..+0x1b0 as 22 doublewords; the host writes the
// same block field by field in its EE layout (ObjStatSave).
// ROR 0x001d96ec

int ObjStat::save(void *out)
{
    if (m_actor != -1)
    {
        const DogActor &actor = gamActor(m_actor);
        m_pos[0] = actor.m_trans[0];
        m_pos[1] = actor.m_trans[1];
        m_pos[2] = actor.m_trans[2];
        m_rot[0] = actor.m_rotate[0];
        m_rot[1] = actor.m_rotate[1];
        m_rot[2] = actor.m_rotate[2];
    }
    ObjStatSave *s = (ObjStatSave *)out;
    s->m_oid = m_oid;
    s->m_stat = m_stat;
    s->m_actor = m_actor;
    s->m_cells[0] = s->m_cells[1] = s->m_cells[2] = 0;  // PORT: host pointers; load clears them
    s->m_smellRange = m_smellRange;
    s->m_scale = m_scale;
    s->m_flags = m_flags;
    s->m_124 = m_124;
    s->m_lp = m_lp;
    s->m_kind = m_kind;
    memcpy(s->m_smellUid, m_smellUid, sizeof(m_smellUid));
    s->m_areaNo = m_areaNo;
    s->m_134 = m_134;
    s->m_inuFunc = m_inuFunc;
    s->m_13c = m_13c;
    s->m_140 = m_140;
    s->m_144 = m_144;
    s->m_148 = m_148;
    s->m_14c = m_14c;
    memcpy(s->m_colPrm, m_colPrm, sizeof(m_colPrm));
    memcpy(s->m_180, m_180, sizeof(m_180));
    memcpy(s->m_pos, m_pos, sizeof(m_pos));
    memcpy(s->m_rot, m_rot, sizeof(m_rot));
    return 0xb0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d9830

int ObjStat::load(const void *in)
{
    const ObjStatSave *s = (const ObjStatSave *)in;
    m_oid = s->m_oid;
    m_stat = s->m_stat;
    m_actor = s->m_actor;
    m_smellRange = s->m_smellRange;
    m_scale = s->m_scale;
    m_flags = s->m_flags;
    m_124 = s->m_124;
    m_lp = s->m_lp;
    m_kind = s->m_kind;
    memcpy(m_smellUid, s->m_smellUid, sizeof(m_smellUid));
    m_areaNo = s->m_areaNo;
    m_134 = s->m_134;
    m_inuFunc = s->m_inuFunc;
    m_13c = s->m_13c;
    m_140 = s->m_140;
    m_144 = s->m_144;
    m_148 = s->m_148;
    m_14c = s->m_14c;
    memcpy(m_colPrm, s->m_colPrm, sizeof(m_colPrm));
    memcpy(m_180, s->m_180, sizeof(m_180));
    memcpy(m_pos, s->m_pos, sizeof(m_pos));
    memcpy(m_rot, s->m_rot, sizeof(m_rot));
    m_secta = NULL;
    m_sectaLast = NULL;
    m_110 = NULL;
    return 0xb0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d9908

void ACHRstat::init(short kind)
{
    u_short id = (u_short)kind;
    u_int   table = id >> 9;

    m_colPrm[0] = 2;
    m_colPrm[1] = 2;
    memcpy(m_colPrmKeep, m_colPrm, sizeof(m_colPrm));
    m_41c = 1.0f;
    m_420 = 0x1.921fb4p+1f;     // 0x40490fda, pi less one ulp
    m_link = -1;
    m_secta = NULL;
    m_110 = NULL;
    m_13c = -1;
    m_inuFunc = -1;
    m_40c = 2;
    m_motionSpeed = 1.0f;
    m_410 = -1;
    m_414 = -1;
    m_rec = NULL;
    m_stats = NULL;
    if (table <= 3)
    {
        const GamSysChrRec *rec = (const GamSysChrRec *)g_gamMan.m_sysData.getRec((short)id);
        m_rec2 = rec;
        if (rec != NULL)
        {
            m_rec = rec;
            m_stats = &rec->m_stats;
        }
        if (table == 0)
        {
            m_42c[1] = 0x80;
            m_42c[2] = 0x80;
            m_lp = m_rec2->m_stats.m_max_lp;
            m_42a = m_lp;
            m_flags = m_rec2->m_flags;
            m_428 = 0;
        }
        else
        {
            m_lp = m_rec2->m_stats.m_max_lp;
            m_flags = m_rec2->m_flags;
            if (table == 2)
            {
                m_464 = 0xa00;
                m_stat = 0;
            }
        }
    }
    if (m_rec != NULL)
    {
        m_smellUid[0] = m_rec->m_head.m_smell_uid[0];
        m_smellUid[1] = m_rec->m_head.m_smell_uid[1];
        m_smellUid[2] = m_rec->m_head.m_smell_uid[2];
        m_smellRange = m_rec->m_head.m_smell_range;
    }
    m_flags |= 0x200040;
}

// ──────────────────────────────────────────────────────────────────────
// Params 1..0x21 through the ELF's accessors (8-byte GCC 2 method
// pointers {0, -1, function} in the ROM); 0 and 9 have none.
// ROR 0x001d9b2c // callcheck: memset (the loop of NULL method pointers)

void ACHRstat::initParamFuncs()
{
    for (int i = 0; i < 0x22; i++)
        m_paramFunc[i] = NULL;
    m_paramFunc[GAM_PRM_O_OID] = &ObjStat::get_o_oid;
    m_paramFunc[GAM_PRM_O_UID] = &ObjStat::get_o_uid;
    m_paramFunc[GAM_PRM_O_CHR_ID] = &ObjStat::get_o_chr_id;
    m_paramFunc[GAM_PRM_O_SECTA_ID] = &ObjStat::get_o_secta_id;
    m_paramFunc[GAM_PRM_O_POS] = &ObjStat::get_o_pos;
    m_paramFunc[GAM_PRM_O_ROT] = &ObjStat::get_o_rot;
    m_paramFunc[GAM_PRM_O_STAT] = &ObjStat::get_o_stat;
    m_paramFunc[GAM_PRM_O_AREA_NO] = &ObjStat::get_o_area_no;
    m_paramFunc[GAM_PRM_O_INU_FUNC] = &ObjStat::get_o_inu_func;
    m_paramFunc[GAM_PRM_COL_PRM] = &ObjStat::getColPrm;
    m_paramFunc[GAM_PRM_O_NAME_ENG] = &ACHRstat::get_o_name_eng;
    m_paramFunc[GAM_PRM_O_NAME_JPN] = &ACHRstat::get_o_name_jpn;
    m_paramFunc[GAM_PRM_O_ICON_NO] = &ACHRstat::get_o_icon_no;
    m_paramFunc[GAM_PRM_O_CHR_ID_HIGH] = &ACHRstat::get_o_chr_id_high;
    m_paramFunc[GAM_PRM_O_CHR_ID_LOW] = &ACHRstat::get_o_chr_id_low;
    m_paramFunc[GAM_PRM_O_SMELL_UID_1] = &ACHRstat::get_o_smell_uid_1;
    m_paramFunc[GAM_PRM_O_SMELL_UID_2] = &ACHRstat::get_o_smell_uid_2;
    m_paramFunc[GAM_PRM_O_SMELL_UID_3] = &ACHRstat::get_o_smell_uid_3;
    m_paramFunc[GAM_PRM_O_SMELL_RANGE] = &ObjStat::get_o_smell_range;
    m_paramFunc[GAM_PRM_O_SCALE] = &ObjStat::get_o_scale;
    m_paramFunc[GAM_PRM_O_MENU_OFFSET_POS] = &ACHRstat::get_o_menu_offset_pos;
    m_paramFunc[GAM_PRM_O_MENU_OFFSET_ROT] = &ACHRstat::get_o_menu_offset_rot;
    m_paramFunc[GAM_PRM_O_NODE_NAME] = &ACHRstat::get_o_node_name;
    m_paramFunc[GAM_PRM_A_MOVE_TYPE] = &ACHRstat::get_a_move_type;
    m_paramFunc[GAM_PRM_A_OFFSET_ROT] = &ACHRstat::get_a_offset_rot;
    m_paramFunc[GAM_PRM_A_LP] = &ACHRstat::get_a_lp;
    m_paramFunc[GAM_PRM_A_MAX_LP] = &ACHRstat::get_a_max_lp;
    m_paramFunc[GAM_PRM_A_ST] = &ACHRstat::get_a_st;
    m_paramFunc[GAM_PRM_A_DEF] = &ACHRstat::get_a_def;
    m_paramFunc[GAM_PRM_A_AVOID] = &ACHRstat::get_a_avoid;
    m_paramFunc[GAM_PRM_A_AGI] = &ACHRstat::get_a_agi;
    m_paramFunc[GAM_PRM_A_NECK_ROT_RANGE] = &ACHRstat::get_a_neck_rot_range;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d9f10

void ObjStat::clearSecta(int all)
{
    m_secta = NULL;
    m_110 = NULL;
    if (all != 0)
        m_sectaLast = NULL;
    // The ROM calls ACHRstat::killTasks on this; every caller passes an achr.
    static_cast<ACHRstat *>(this)->killTasks();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001da07c

void ACHRstat::killTasks()
{
    if (g_taskMan.getTask(m_task[2]) != NULL)
        g_taskMan.killTask(m_task[2]);
    if (g_taskMan.getTask(m_task[0]) != NULL)
        g_taskMan.killTask(m_task[0]);
    if (g_taskMan.getTask(m_task[3]) != NULL)
        g_taskMan.killTask(m_task[3]);
    if (g_taskMan.getTask(m_task[1]) != NULL)
        g_taskMan.killTask(m_task[1]);
    m_task[1] = -1;
    m_task[2] = -1;
    m_task[0] = -1;
    m_task[3] = -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001da130

void ACHRstat::pauseTasks(int on, int all)
{
    if (all != 0)
        g_taskMan.pauseTask(m_task[2], on);
    g_taskMan.pauseTask(m_task[0], on);
    g_taskMan.pauseTask(m_task[3], on);
    g_taskMan.pauseTask(m_task[1], on);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001da228

void ACHRstat::getParam(int which, void *out)
{
    if (which < 0x22)
    {
        (this->*m_paramFunc[which])(out);
        return;
    }
    if (which >= 0x22 && which <= 0x31)
    {
        *(int *)out = m_param[which - 0x22];
        return;
    }
    if (which >= 0x34 && which <= 0x38)
    {
        *(u_short *)out = m_42c[which - 0x34];
        return;
    }
    if (which >= 0x60 && which <= 0x63)
    {
        *(u_short *)out = m_454[which - 0x60];
        return;
    }
    if (which >= 0x69 && which <= 0x6f)
    {
        *(u_short *)out = m_468[which - 0x69];
        return;
    }
    if (which >= 0x1000 && which <= 0x1005)
    {
        ObjStat::getParam(which, out);
        return;
    }
    switch (which)
    {
    case 0x32: *(u_short *)out = m_428; break;
    case 0x33: *(u_short *)out = m_42a; break;
    case 0x59: memcpy(out, m_42c, sizeof(m_42c)); break;
    case 0x5a: *(u_short *)out = m_440; break;
    case 0x5b: *(float *)out = m_444; break;
    case 0x5c: *(int *)out = m_448; break;
    case 0x5d: *(u_short *)out = m_44c; break;
    case 0x5e: *(u_short *)out = m_44e; break;
    case 0x5f: *(float *)out = m_450; break;
    case 0x64: *(u_int *)out = m_45c; break;
    case 0x66: *(u_short *)out = m_464; break;
    case 0x67: *(int *)out = m_43c; break;
    case 0x68: *(u_short *)out = m_466; break;
    case 0x1100: *(int *)out = m_40c; break;
    case 0x1101: *(int *)out = m_410; break;
    case 0x1102: *(int *)out = m_414; break;
    case 0x1103: *(float *)out = m_motionSpeed; break;
    case 0x1104: *(float *)out = m_41c; break;
    case 0x1105: *(float *)out = m_420; break;
    case 0x1201: *(float *)out = m_408; break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The base params fall through into param 0x32's store (no break in the
// ROM): after ObjStat::setParam, m_428 also takes data's first u_short.
// ROR 0x001da740

void ACHRstat::setParam(int which, const void *data)
{
    if (which >= 0x22 && which <= 0x31)
    {
        m_param[which - 0x22] = *(const int *)data;
        return;
    }
    if (which >= 0x34 && which <= 0x38)
    {
        m_42c[which - 0x34] = *(const u_short *)data;
        return;
    }
    if (which >= 0x60 && which <= 0x63)
    {
        m_454[which - 0x60] = *(const u_short *)data;
        return;
    }
    if (which >= 0x69 && which <= 0x6f)
    {
        m_468[which - 0x69] = *(const u_short *)data;
        return;
    }
    switch (which)
    {
    case GAM_PRM_O_OID:
    case GAM_PRM_O_UID:
    case GAM_PRM_O_CHR_ID:
    case GAM_PRM_O_POS:
    case GAM_PRM_O_ROT:
    case GAM_PRM_O_STAT:
    case GAM_PRM_O_SMELL_RANGE:
    case GAM_PRM_O_SCALE:
    case GAM_PRM_COL_PRM:
    case 0x1000:
    case 0x1001:
    case 0x1002:
    case 0x1003:
    case 0x1004:
        ObjStat::setParam(which, data);
        /* fall through */
    case 0x32:
        m_428 = *(const u_short *)data;
        break;
    case GAM_PRM_O_AREA_NO:
        m_areaNo = *(const int *)data;
        break;
    case GAM_PRM_O_INU_FUNC:
        m_inuFunc = *(const int *)data;
        break;
    case GAM_PRM_A_OFFSET_ROT:
        m_offsetRot = *(const float *)data;
        break;
    case GAM_PRM_A_LP:
        m_lp = *(const u_short *)data;
        break;
    case 0x33: m_42a = *(const u_short *)data; break;
    case 0x59: memcpy(m_42c, data, sizeof(m_42c)); break;
    case 0x5a: m_440 = *(const u_short *)data; break;
    case 0x5b: m_444 = *(const float *)data; break;
    case 0x5c: m_448 = *(const int *)data; break;
    case 0x5d: m_44c = *(const u_short *)data; break;
    case 0x5e: m_44e = *(const u_short *)data; break;
    case 0x5f: m_450 = *(const float *)data; break;
    case 0x64: m_45c = *(const u_short *)data; break;
    case 0x65: m_460 = *(const float *)data; break;
    case 0x66: m_464 = *(const u_short *)data; break;
    case 0x67: m_43c = *(const int *)data; break;
    case 0x68: m_466 = *(const u_short *)data; break;
    case 0x1100: m_40c = *(const int *)data; break;
    case 0x1101: m_410 = *(const int *)data; break;
    case 0x1102: m_414 = *(const int *)data; break;
    case 0x1103: m_motionSpeed = *(const float *)data; break;
    case 0x1104: m_41c = *(const float *)data; break;
    case 0x1105: m_420 = *(const float *)data; break;
    case 0x1201: m_408 = *(const float *)data; break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001db17c

int ObjStat::getWork(int no)
{
    return m_work[no];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001db18c

void ObjStat::setWork(int no, int value)
{
    m_work[no] = value;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001db19c

void ObjStat::addWork(int no, int value)
{
    m_work[no] += value;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d8fec

void ObjStat::placeAt(int where)
{
    float pos[4];
    setSecta(g_gamMan.getSectaCell(where, pos, 0));
    if (m_secta != NULL)
        setPos(pos);
}

// ──────────────────────────────────────────────────────────────────────
// Param 6 adds into a copy and then stores data itself (the ROM's). Param
// 7 copies the word, 0x5c adds a float into the word setParam stores as an
// int. 0x4f..0x58 add into the system record's (table 2) fields.
// ROR 0x001dac8c // callcheck: sceVu0AddVector (the vector adds, written out)

void ACHRstat::addParam(int which, const void *data)
{
    const float *f = (const float *)data;
    const u_short h = *(const u_short *)data;
    float v[4];

    if (which >= 0x22 && which <= 0x31)
    {
        m_param[which - 0x22] += *(const int *)data;
        return;
    }
    if (which >= 0x34 && which <= 0x38)
    {
        m_42c[which - 0x34] += h;
        return;
    }
    if (which >= 0x4f && which <= 0x58)
    {
        ((GamSysRec2 *)m_rec2)->m_98[which - 0x4f] += h;
        return;
    }
    if (which >= 0x69 && which <= 0x6f)
    {
        m_468[which - 0x69] += h;
        return;
    }
    switch (which)
    {
    case GAM_PRM_O_POS:
        ObjStat::getPos(v);
        v[0] += f[0];
        v[1] += f[1];
        v[2] += f[2];
        v[3] += f[3];
        ObjStat::setPos(v);
        break;
    case GAM_PRM_O_ROT:
        ObjStat::getRot(v);
        v[0] += f[0];
        v[1] += f[1];
        v[2] += f[2];
        v[3] += f[3];
        ObjStat::setRot(f);
        break;
    case GAM_PRM_O_STAT:
        m_stat = *(const int *)data;
        break;
    case GAM_PRM_O_SCALE:
        m_scale += f[0];
        break;
    case GAM_PRM_A_LP:
        m_lp += h;
        break;
    case 0x33: m_42a += h; break;
    case 0x5a: m_440 += h; break;
    case 0x5b: m_444 += f[0]; break;
    case 0x5c:
    {
        float w;
        memcpy(&w, &m_448, sizeof(w));
        w += f[0];
        memcpy(&m_448, &w, sizeof(w));
        break;
    }
    case 0x5d: m_44c += h; break;
    case 0x5e: m_44e += h; break;
    case 0x5f: m_450 += f[0]; break;
    case 0x62: m_454[2] += h; break;
    case 0x63: m_454[3] += h; break;
    case 0x64: m_45c += h; break;
    case 0x65: m_460 += f[0]; break;
    case 0x68: m_466 += h; break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001db1b8

void ACHRstat::setMotionSpeed(float speed)
{
    m_motionSpeed = speed;
    g_magSystem.m_scene.setActorMotionSpeed(speed, m_actor);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001db1f0

void FCHRstat::init(short kind)
{
    u_short id = (u_short)kind;

    m_colPrm[1] = 2;
    m_colPrm[0] = 2;
    memcpy(m_colPrmKeep, m_colPrm, sizeof(m_colPrm));
    m_link = -1;
    m_secta = NULL;
    m_110 = NULL;
    m_inuFunc = -1;
    m_13c = -1;
    m_flags = 0;
    m_rec = NULL;
    switch (id >> 9)
    {
    case 4:
        m_flags = 0;
        break;
    case 5:
    {
        const GamSysRec5 *rec = (const GamSysRec5 *)g_gamMan.m_sysData.getRec((short)id);
        m_134 = 0;
        m_rec2 = rec;
        m_290 = (m_290 & ~0xffff) | 1;     /* the ROM stores a u_short */
        m_flags = rec->m_flags;
        m_stats = rec->m_80;
        m_rec = rec;
        break;
    }
    case 9:
        m_294[0] = -1;
        m_28c = -1;
        break;
    }
    if (m_rec != NULL)
    {
        m_smellUid[0] = m_rec->m_head.m_smell_uid[0];
        m_smellUid[1] = m_rec->m_head.m_smell_uid[1];
        m_smellUid[2] = m_rec->m_head.m_smell_uid[2];
        m_smellRange = m_rec->m_head.m_smell_range;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001db31c

void FCHRstat::getParam(int which, void *out)
{
    if ((which >= 1 && which <= 8) || which == 10 || (which >= 0xe && which <= 0x14) ||
        which == GAM_PRM_COL_PRM || (which >= 0x1000 && which <= 0x1005))
    {
        ObjStat::getParam(which, out);
        return;
    }
    switch (which)
    {
    case 0x1f:
        *(u_short *)out = m_lp;
        break;
    case 0x20:
    case 0x36:
    case 0x41:
        *(int *)out = m_28c;
        break;
    case 0x21:
        *(u_short *)out = (u_short)m_290;
        break;
    case 0x32:
        *(u_short *)out = (u_short)m_28c;
        break;
    case 0x33:
    case 0x37:
    case 0x3c:
        *(int *)out = m_290;
        break;
    case 0x34:
    case 0x38:
    case 0x3f:
        *(int *)out = m_294[0];
        break;
    case 0x35:
    case 0x3a:
        *(int *)out = m_294[1];
        break;
    case 0x40:
        *(int *)out = m_134;
        break;
    case 0x43:
        memcpy(out, m_294, sizeof(m_294));
        break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Unlike ACHRstat's, this setter cannot set the oid (param 1).
// ROR 0x001db520

void FCHRstat::setParam(int which, const void *data)
{
    if ((which >= 2 && which <= 8) || which == 10 || which == 0x13 || which == 0x14 ||
        which == GAM_PRM_COL_PRM || (which >= 0x1000 && which <= 0x1004))
    {
        ObjStat::setParam(which, data);
        return;
    }
    switch (which)
    {
    case 0x1f:
        m_lp = *(const u_short *)data;
        break;
    case 0x20:
    case 0x36:
    case 0x41:
        m_28c = *(const int *)data;
        break;
    case 0x21:                      /* the ROM stores a u_short */
        m_290 = (m_290 & ~0xffff) | *(const u_short *)data;
        break;
    case 0x32:                      /* the ROM stores a u_short */
        m_28c = (m_28c & ~0xffff) | *(const u_short *)data;
        break;
    case 0x33:
    case 0x37:
    case 0x3c:
        m_290 = *(const int *)data;
        break;
    case 0x34:
    case 0x38:
    case 0x3f:
        m_294[0] = *(const int *)data;
        break;
    case 0x35:
    case 0x3a:
        m_294[1] = *(const int *)data;
        break;
    case 0x40:
        m_134 = *(const int *)data;
        break;
    case 0x43:
        memcpy(m_294, data, sizeof(m_294));
        break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001db71c

void FCHRstat::getPos(float *out)
{
    if (m_actor != -1)
    {
        const DogActor &actor = gamActor(m_actor);
        out[0] = actor.m_trans[0];
        out[1] = actor.m_trans[1];
        out[2] = actor.m_trans[2];
        return;
    }
    out[0] = m_pos[0];
    out[1] = m_pos[1];
    out[2] = m_pos[2];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001db774

void FCHRstat::getRot(float *out)
{
    if (m_actor != -1)
    {
        const DogActor &actor = gamActor(m_actor);
        out[0] = actor.m_rotate[0];
        out[1] = actor.m_rotate[1];
        out[2] = actor.m_rotate[2];
        return;
    }
    out[0] = m_rot[0];
    out[1] = m_rot[1];
    out[2] = m_rot[2];
}

// ──────────────────────────────────────────────────────────────────────
// ObjStat::setPos but the cell is looked for with or without an actor.
// ROR 0x001db7cc

void FCHRstat::setPos(const float *pos)
{
    m_pos[0] = pos[0];
    m_pos[1] = pos[1];
    m_pos[2] = pos[2];
    if (m_actor != -1)
    {
        DogActor &actor = gamActor(m_actor);
        actor.m_trans[0] = m_pos[0];
        actor.m_trans[1] = m_pos[1];
        actor.m_trans[2] = m_pos[2];
        actor.m_dirty = 1;
    }
    if (m_secta == NULL || (m_flags & 0x40) != 0)
        setSecta(g_gamMan.findSectaCell(m_pos, m_secta, gamOidType(m_oid) == 2));
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001db898

void FCHRstat::setRot(const float *rot)
{
    m_rot[0] = rot[0];
    m_rot[1] = rot[1];
    m_rot[2] = rot[2];
    if (m_actor != -1)
        g_magSystem.setActorRotate(m_actor, m_rot);
}

// ──────────────────────────────────────────────────────────────────────
// Written ahead for GamMan::startStage2.
// ROR 0x001db9c4

float gamGetWrdOfsX(void)
{
    const WrdEntry *e = g_wrdTable.get(g_gamMan.getWrdNo());
    if (e == NULL)
        return 0.0f;
    return e->m_ofs[0];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dba00

float gamGetWrdOfsZ(void)
{
    const WrdEntry *e = g_wrdTable.get(g_gamMan.getWrdNo());
    if (e == NULL)
        return 0.0f;
    return e->m_ofs[2];
}

// ──────────────────────────────────────────────────────────────────────
// A jump table (data 0x2fc730) over ops 0..4; 3 and 4 do nothing.
// ROR 0x001dba3c

void gamWrdConvPos(int op, float *a, float *b)
{
    switch ((u_int)op)
    {
    case 0:
        g_wrdTable.cnvPosW2L(a, b, g_gamMan.getWrdNo());
        break;
    case 1:
        g_wrdTable.cnvPosL2W(a, g_gamMan.getWrdNo(), b);
        break;
    case 2:
    {
        const WrdEntry *e = g_wrdTable.get(g_gamMan.getWrdNo());
        // PORT: the ROM reads low memory when no table is loaded.
        if (e == NULL)
            break;
        a[1] = 0.0f;
        a[0] = e->m_ofs[0];
        a[2] = e->m_ofs[2];
        break;
    }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dbef8

void GamSysData::set(u_char *data)
{
    m_data = data;
    u_char *base = data + 0x40;
    const int *ofs = (const int *)(data + 0x10);
    for (int i = 0; i < 10; i++)
        m_table[i] = base + (ofs[i] - ofs[0]);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dbfa0

const void *GamSysData::getRec(short id)
{
    u_short u = (u_short)id;
    u_int   no = u & 0x1ff;
    switch (u >> 9)
    {
    case 0:  return m_table[0] + no * 0xa0;
    case 1:  return m_table[1] + no * 0xa0;
    case 2:  return m_table[2] + no * 0xc0;
    case 3:  return m_table[3] + no * 0xe0;
    case 4:  return m_table[4] + no * 0xc0;
    case 5:  return m_table[5] + no * 0x200;
    case 6:  return m_table[6] + no * 0xc0;
    case 7:  return m_table[7] + no * 0xc0;
    case 8:  return m_table[8] + no * 0xc0;
    case 9:  return m_table[9] + no * 0xc0;
    default: return NULL;
    }
}

/* The flag word every flag function addresses: +0x90 of a character
 * record (tables 0..3), +0xb0 of an object record (4..9); NULL for a bad
 * table. Each ROM function computes it inline. */
static u_int *sysFlagWord(GamSysData *sys, short id)
{
    u_char *rec = (u_char *)sys->getRec(id);
    if (rec == NULL)
        return NULL;
    if (((u_short)id >> 9) <= 3)
        return &((GamSysChrRec *)rec)->m_flags;
    return &((GamSysObjRec *)rec)->m_flags;
}

// ──────────────────────────────────────────────────────────────────────
// Field field of record id into out. The record's id is its own table's
// layout; fields outside a table's list leave out alone.
// ROR 0x001dc48c

void GamSysData::getField(int id, int field, void *out)
{
    u_short u = (u_short)id;
    u_int   table = u >> 9;
    if (table > 9)
        return;
    if (table == 5 && m_table[5] == NULL)
        return;
    const GamSysRec *head = (const GamSysRec *)getRec((short)u);

    /* The head every table shares. */
    switch (field)
    {
    case 2:
        *(u_short *)out = head->m_40;
        return;
    case GAM_PRM_O_NAME_ENG:
    case GAM_PRM_O_NAME_JPN:
    {
        const char *name = field == GAM_PRM_O_NAME_ENG ? head->m_name_eng : head->m_name_jpn;
        if (table == 7 || table == 8)
            strncpy((char *)out, name, 0x20);
        else
            *(const char **)out = name;     // PORT: a host pointer (the ROM's slot is a word)
        return;
    }
    case GAM_PRM_O_ICON_NO:      *(u_short *)out = head->m_icon_no; return;
    case GAM_PRM_O_CHR_ID_HIGH:  *(u_short *)out = head->m_chr_id_high; return;
    case GAM_PRM_O_CHR_ID_LOW:   *(u_short *)out = head->m_chr_id_low; return;
    case GAM_PRM_O_SMELL_UID_1:  *(u_short *)out = head->m_smell_uid[0]; return;
    case GAM_PRM_O_SMELL_UID_2:  *(u_short *)out = head->m_smell_uid[1]; return;
    case GAM_PRM_O_SMELL_UID_3:  *(u_short *)out = head->m_smell_uid[2]; return;
    case GAM_PRM_O_MENU_OFFSET_POS:
        memcpy(out, head->m_menu_offset_pos, 0x10);
        return;
    case GAM_PRM_O_MENU_OFFSET_ROT:
        memcpy(out, head->m_menu_offset_rot, 0x10);
        return;
    case GAM_PRM_O_NODE_NAME:
        strncpy((char *)out, head->m_node_name, 8);
        return;
    }

    if (table <= 3)
    {
        const GamSysChrRec *rec = (const GamSysChrRec *)head;
        switch (field)
        {
        case GAM_PRM_A_MAX_LP: *(u_short *)out = rec->m_stats.m_max_lp; return;
        case GAM_PRM_A_ST:     *(u_short *)out = rec->m_stats.m_st; return;
        case GAM_PRM_A_DEF:    *(u_short *)out = rec->m_stats.m_def; return;
        case GAM_PRM_A_AVOID:  *(u_short *)out = rec->m_stats.m_avoid; return;
        case GAM_PRM_A_AGI:    *(u_short *)out = rec->m_stats.m_agi; return;
        case GAM_PRM_A_NECK_ROT_RANGE:
            /* degrees to radians in double precision, as the ROM's libgcc calls */
            *(float *)out = (float)((double)(float)rec->m_stats.m_neck_rot_range *
                                    3.141592653589793 / 180.0);
            return;
        }
        if (table == 2)
        {
            const GamSysRec2 *r2 = (const GamSysRec2 *)head;
            if (field >= 0x4f && field <= 0x58)
                *(u_short *)out = r2->m_98[field - 0x4f];
            else if (field >= 0x70 && field <= 0x72)
                *(u_short *)out = r2->m_ac[field - 0x70];
        }
        else if (table == 3)
        {
            const GamSysRec3 *r3 = (const GamSysRec3 *)head;
            switch (field)
            {
            case 0x73: strncpy((char *)out, r3->m_b4, 0x20); break;
            case 0x74: strncpy((char *)out, r3->m_94, 0x20); break;
            case 0x75: *(u_short *)out = r3->m_d4; break;
            case 0x76: *(u_short *)out = r3->m_d6; break;
            }
        }
        return;
    }

    /* Tables 4..9: the object records. */
    const GamSysObjRec *rec = (const GamSysObjRec *)head;
    switch (field)
    {
    case 0x19: strncpy((char *)out, rec->m_80, 8); return;
    case 0x1a: strncpy((char *)out, rec->m_88, 8); return;
    case 0x1b: *(u_short *)out = rec->m_a0; return;
    case 0x1c: memcpy(out, rec->m_90, 0x10); return;
    case 0x1d: *(u_short *)out = rec->m_a4; return;
    case 0x1e: *(u_short *)out = rec->m_a6; return;
    }
    if (table == 8 && field == 0x39)
        *(u_short *)out = rec->m_b4;
    else if (table == 9 && field == 0x3b)
        *(u_short *)out = rec->m_b4;
    else if (table == 5)
    {
        const GamSysRec5 *r5 = (const GamSysRec5 *)head;
        switch (field)
        {
        case GAM_PRM_O_SMELL_RANGE: *(float *)out = r5->m_head.m_smell_range; break;
        case 0x22: *(u_short *)out = r5->m_b4; break;
        case 0x23: *(u_short *)out = r5->m_b6; break;
        case 0x24: *(u_short *)out = r5->m_b8; break;
        case 0x25: *(const char **)out = r5->m_ba; break;  // PORT: a host pointer
        case 0x26: case 0x27: case 0x28: case 0x29:
            *(u_short *)out = r5->m_1bc[field - 0x26];
            break;
        case 0x2a: *(int *)out = r5->m_1c4; break;
        case 0x2b: *(int *)out = r5->m_1c8; break;
        case 0x2c: case 0x2d: case 0x2e:
            *(u_short *)out = r5->m_1cc[field - 0x2c];
            break;
        case 0x2f: *(u_short *)out = r5->m_1ba; break;
        case 0x30: *(int *)out = r5->m_1d4; break;
        case 0x31: *(u_short *)out = r5->m_1d8; break;
        case 0x42: *(int *)out = r5->m_1fc; break;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dcd38

void GamSysData::setFlags(short id, u_int bits)
{
    u_int *w = sysFlagWord(this, id);
    if (w != NULL)
        *w |= bits;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dce30

void GamSysData::clearFlags(short id, u_int bits)
{
    u_int *w = sysFlagWord(this, id);
    if (w != NULL)
        *w &= ~bits;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dcf28

u_int GamSysData::getFlags(short id)
{
    u_int *w = sysFlagWord(this, id);
    return w != NULL ? *w : 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dd00c

int GamSysData::testFlags(short id, u_int bits)
{
    u_int *w = sysFlagWord(this, id);
    return w != NULL && (*w & bits) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dd118

void GamSysData::setFlags2(short id, int unused, u_int bits)
{
    (void)unused;
    if (((u_short)id >> 9) == 2)
        ((GamSysRec2 *)getRec(id))->m_flags2 |= bits;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dd154

void GamSysData::clearFlags2(short id, int unused, u_int bits)
{
    (void)unused;
    if (((u_short)id >> 9) == 2)
        ((GamSysRec2 *)getRec(id))->m_flags2 &= ~bits;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dd194

int GamSysData::testFlags94(short id, int unused, u_int bits)
{
    (void)unused;
    if (((u_short)id >> 9) == 2)
        return (((const GamSysRec2 *)getRec(id))->m_flags2 & bits) != 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The ELF's accessors that copy blocks (inline in the ROM).

// ROR 0x0028fa04
void ObjStat::getColPrm(void *out)
{
    memcpy(out, m_colPrm, sizeof(m_colPrm));
}

// ROR 0x0028fa38
void ObjStat::getColPrmKeep(void *out)
{
    memcpy(out, m_colPrmKeep, sizeof(m_colPrmKeep));
}

// ROR 0x0028fc88
void ACHRstat::get_o_menu_offset_pos(void *out)
{
    if (m_rec != NULL)
        memcpy(out, m_rec->m_head.m_menu_offset_pos, 0x10);
}

// ROR 0x0028fca4
void ACHRstat::get_o_menu_offset_rot(void *out)
{
    if (m_rec != NULL)
        memcpy(out, m_rec->m_head.m_menu_offset_rot, 0x10);
}

// ROR 0x0028fcc0
void ACHRstat::get_o_node_name(void *out)
{
    if (m_rec != NULL)
        strcpy((char *)out, m_rec->m_head.m_node_name);
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x001dc070
void GamSysData::setField(int id, int field, const void *data)
{
    ROSEPAN_STUB("GamSysData::setField", 0x001dc070);
}

// stub of ROR 0x001dbcf0
void gamSetWrdRoom(GamMan *gam, const void *file)
{
    ROSEPAN_STUB("gamSetWrdRoom", 0x001dbcf0);
}

// stub of ROR 0x001d9f08
void ACHRstat::empty1d9f08()
{
    ROSEPAN_STUB("ACHRstat::empty1d9f08", 0x001d9f08);
}

// stub of ROR 0x001d9f84
int ACHRstat::isMotionEnd()
{
    ROSEPAN_STUB("ACHRstat::isMotionEnd", 0x001d9f84);
    return {};
}

// stub of ROR 0x001da05c
int ACHRstat::isTurnDone()
{
    ROSEPAN_STUB("ACHRstat::isTurnDone", 0x001da05c);
    return {};
}

// stub of ROR 0x001db8e8
const char * gamGetWrdString(int which, int wrd)
{
    ROSEPAN_STUB("gamGetWrdString", 0x001db8e8);
    return {};
}
