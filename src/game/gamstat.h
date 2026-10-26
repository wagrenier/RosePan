/* ==========================================================================
 *  game/gamstat.h
 *
 *  game/gamstat.cpp (0x1d84c8..0x1dd1e8): the game objects' state --
 *  ObjStat (vtable 0x2e4828, vptr at +0x258) and the two kinds GamMan
 *  makes, ACHRstat (actors, vtable 0x2e4810, EE 0x480) and FCHRstat (field
 *  objects, vtable 0x2e47f8, EE 0x2c0) -- and GamSysData, the system
 *  pack's record tables (GamMan +0x22220).
 *
 *  The params: getParam / setParam take one id space, which the ELF's
 *  inline accessors name (ObjStat::get_o_oid ... ACHRstat::
 *  get_a_neck_rot_range, the table ACHRstat::initParamFuncs fills): 1..0x21
 *  as GamParam below, 0x22.. per class. GamSysData::getField uses the same
 *  ids for the record fields the params come from.
 * ======================================================================== */
#ifndef _GAME_GAMSTAT_H
#define _GAME_GAMSTAT_H

#include <stddef.h>
#include <string.h>

#include "sdk/eetypes.h"
#include "sequence/sequence.h"

/* The param ids, named after the ELF's accessors (get_o_* / get_a_*). */
enum GamParam
{
    GAM_PRM_O_OID              = 0x01,
    GAM_PRM_O_UID              = 0x02,  // the system data id
    GAM_PRM_O_CHR_ID           = 0x03,  // the scene actor
    GAM_PRM_O_SECTA_ID         = 0x04,
    GAM_PRM_O_POS              = 0x05,
    GAM_PRM_O_ROT              = 0x06,
    GAM_PRM_O_STAT             = 0x07,
    GAM_PRM_O_AREA_NO          = 0x08,
    GAM_PRM_O_INU_FUNC         = 0x0a,  // "inu": the dog
    GAM_PRM_O_NAME_ENG         = 0x0b,
    GAM_PRM_O_NAME_JPN         = 0x0c,
    GAM_PRM_O_ICON_NO          = 0x0d,
    GAM_PRM_O_CHR_ID_HIGH      = 0x0e,
    GAM_PRM_O_CHR_ID_LOW       = 0x0f,  // the record's model id, 0xffff none
    GAM_PRM_O_SMELL_UID_1      = 0x10,
    GAM_PRM_O_SMELL_UID_2      = 0x11,
    GAM_PRM_O_SMELL_UID_3      = 0x12,
    GAM_PRM_O_SMELL_RANGE      = 0x13,
    GAM_PRM_O_SCALE            = 0x14,
    GAM_PRM_O_MENU_OFFSET_POS  = 0x15,
    GAM_PRM_O_MENU_OFFSET_ROT  = 0x16,
    GAM_PRM_O_NODE_NAME        = 0x17,
    GAM_PRM_COL_PRM            = 0x18,  // ObjStat::getColPrm: the collision words
    GAM_PRM_A_MOVE_TYPE        = 0x19,
    GAM_PRM_A_OFFSET_ROT       = 0x1a,
    GAM_PRM_A_LP               = 0x1b,
    GAM_PRM_A_MAX_LP           = 0x1c,
    GAM_PRM_A_ST               = 0x1d,
    GAM_PRM_A_DEF              = 0x1e,
    GAM_PRM_A_AVOID            = 0x1f,
    GAM_PRM_A_AGI              = 0x20,
    GAM_PRM_A_NECK_ROT_RANGE   = 0x21,  // degrees in the record, radians from getField
};

/* ---- the system pack's records (file data, plain EE layout) ---- */

/* The 0x80 bytes every record starts with. */
struct GamSysRec
{
    char    m_name_jpn[0x20];       // ROR 0x00: field 0xc (its address; tables 7 / 8 copy it)
    char    m_name_eng[0x20];       // ROR 0x20: field 0xb (its address; tables 7 / 8 copy it)
    u_short m_40;                   // ROR 0x40: field 2
    u_short m_icon_no;              // ROR 0x42: field 0xd
    u_short m_chr_id_high;          // ROR 0x44: field 0xe
    u_short m_chr_id_low;           // ROR 0x46: field 0xf, the model id
    u_short m_smell_uid[3];         // ROR 0x48: fields 0x10..0x12
    u_short _4e;
    float   m_menu_offset_pos[4];   // ROR 0x50: field 0x15
    float   m_menu_offset_rot[4];   // ROR 0x60: field 0x16
    float   m_smell_range;          // ROR 0x70: field 0x13 (table 5)
    char    m_node_name[8];         // ROR 0x74: field 0x17
    u_char  _7c[4];
};
static_assert(sizeof(GamSysRec) == 0x80, "GamSysRec");
static_assert(offsetof(GamSysRec, m_name_eng) == 0x20, "m_name_eng");
static_assert(offsetof(GamSysRec, m_40) == 0x40, "m_40");
static_assert(offsetof(GamSysRec, m_chr_id_low) == 0x46, "m_chr_id_low");
static_assert(offsetof(GamSysRec, m_smell_uid) == 0x48, "m_smell_uid");
static_assert(offsetof(GamSysRec, m_menu_offset_pos) == 0x50, "m_menu_offset_pos");
static_assert(offsetof(GamSysRec, m_menu_offset_rot) == 0x60, "m_menu_offset_rot");
static_assert(offsetof(GamSysRec, m_smell_range) == 0x70, "m_smell_range");
static_assert(offsetof(GamSysRec, m_node_name) == 0x74, "m_node_name");

/* A character's stats (+0x80 of tables 0..3; ACHRstat keeps a pointer). */
struct GamSysStats
{
    u_short m_max_lp;               // ROR 0x00: field 0x1c
    u_short m_st;                   // ROR 0x02: field 0x1d
    u_short m_def;                  // ROR 0x04: field 0x1e
    u_short m_avoid;                // ROR 0x06: field 0x1f
    u_short m_agi;                  // ROR 0x08: field 0x20
    u_short m_neck_rot_range;       // ROR 0x0a: field 0x21, degrees
};
static_assert(sizeof(GamSysStats) == 0xc, "GamSysStats");

/* Tables 0..3 (records 0xa0, 0xa0, 0xc0, 0xe0): characters. */
struct GamSysChrRec
{
    GamSysRec   m_head;             // ROR 0x00
    GamSysStats m_stats;            // ROR 0x80
    u_char      _8c[4];
    u_int       m_flags;            // ROR 0x90: setFlags / testFlags
};
static_assert(offsetof(GamSysChrRec, m_stats) == 0x80, "m_stats");
static_assert(offsetof(GamSysChrRec, m_flags) == 0x90, "m_flags");

/* Table 2 (0xc0). */
struct GamSysRec2
{
    GamSysChrRec m_chr;             // ROR 0x00
    u_int        m_flags2;          // ROR 0x94: setFlags2 / testFlags94
    u_short      m_98[10];          // ROR 0x98: fields 0x4f..0x58
    u_short      m_ac[3];           // ROR 0xac: fields 0x70..0x72
    u_char       _b2[0xe];
};
static_assert(sizeof(GamSysRec2) == 0xc0, "GamSysRec2");
static_assert(offsetof(GamSysRec2, m_flags2) == 0x94, "m_flags2");
static_assert(offsetof(GamSysRec2, m_98) == 0x98, "m_98");
static_assert(offsetof(GamSysRec2, m_ac) == 0xac, "m_ac");

/* Table 3 (0xe0). */
struct GamSysRec3
{
    GamSysChrRec m_chr;             // ROR 0x00
    char         m_94[0x20];        // ROR 0x94: field 0x74
    char         m_b4[0x20];        // ROR 0xb4: field 0x73
    u_short      m_d4;              // ROR 0xd4: field 0x75
    u_short      m_d6;              // ROR 0xd6: field 0x76
    u_char       _d8[8];
};
static_assert(sizeof(GamSysRec3) == 0xe0, "GamSysRec3");
static_assert(offsetof(GamSysRec3, m_94) == 0x94, "m_94");
static_assert(offsetof(GamSysRec3, m_b4) == 0xb4, "m_b4");
static_assert(offsetof(GamSysRec3, m_d4) == 0xd4, "m_d4");

/* Tables 4..9 (0xc0; 5 is 0x200, GamSysRec5): objects. */
struct GamSysObjRec
{
    GamSysRec m_head;               // ROR 0x00
    char      m_80[8];              // ROR 0x80: field 0x19
    char      m_88[8];              // ROR 0x88: field 0x1a
    float     m_90[4];              // ROR 0x90: field 0x1c
    u_short   m_a0;                 // ROR 0xa0: field 0x1b
    u_short   _a2;
    u_short   m_a4;                 // ROR 0xa4: field 0x1d
    u_short   m_a6;                 // ROR 0xa6: field 0x1e
    u_char    _a8[8];
    u_int     m_flags;              // ROR 0xb0: setFlags / testFlags
    u_short   m_b4;                 // ROR 0xb4: field 0x39 (table 8), 0x3b (9), 0x22 (5)
    u_short   m_b6;                 // ROR 0xb6: field 0x23 (table 5)
    u_char    _b8[8];
};
static_assert(sizeof(GamSysObjRec) == 0xc0, "GamSysObjRec");
static_assert(offsetof(GamSysObjRec, m_80) == 0x80, "m_80");
static_assert(offsetof(GamSysObjRec, m_90) == 0x90, "m_90");
static_assert(offsetof(GamSysObjRec, m_a0) == 0xa0, "m_a0");
static_assert(offsetof(GamSysObjRec, m_a4) == 0xa4, "m_a4");
static_assert(offsetof(GamSysObjRec, m_flags) == 0xb0, "m_flags");
static_assert(offsetof(GamSysObjRec, m_b4) == 0xb4, "m_b4");

/* Table 5 (0x200): the field objects FCHRstat::init reads. */
struct GamSysRec5
{
    GamSysRec m_head;               // ROR 0x00
    char      m_80[8];              // ROR 0x80: field 0x19
    char      m_88[8];              // ROR 0x88: field 0x1a
    float     m_90[4];              // ROR 0x90: field 0x1c
    u_short   m_a0;                 // ROR 0xa0: field 0x1b
    u_short   _a2;
    u_short   m_a4;                 // ROR 0xa4: field 0x1d
    u_short   m_a6;                 // ROR 0xa6: field 0x1e
    u_char    _a8[8];
    u_int     m_flags;              // ROR 0xb0
    u_short   m_b4;                 // ROR 0xb4: field 0x22
    u_short   m_b6;                 // ROR 0xb6: field 0x23
    u_short   m_b8;                 // ROR 0xb8: field 0x24
    char      m_ba[0x100];          // ROR 0xba: field 0x25 (its address)
    u_short   m_1ba;                // ROR 0x1ba: field 0x2f
    u_short   m_1bc[4];             // ROR 0x1bc: fields 0x26..0x29
    int       m_1c4;                // ROR 0x1c4: field 0x2a
    int       m_1c8;                // ROR 0x1c8: field 0x2b
    u_short   m_1cc[3];             // ROR 0x1cc: fields 0x2c..0x2e
    u_short   _1d2;
    int       m_1d4;                // ROR 0x1d4: field 0x30
    u_short   m_1d8;                // ROR 0x1d8: field 0x31
    u_char    _1da[0x22];
    int       m_1fc;                // ROR 0x1fc: field 0x42
};
static_assert(sizeof(GamSysRec5) == 0x200, "GamSysRec5");
static_assert(offsetof(GamSysRec5, m_flags) == 0xb0, "m_flags");
static_assert(offsetof(GamSysRec5, m_b8) == 0xb8, "m_b8");
static_assert(offsetof(GamSysRec5, m_ba) == 0xba, "m_ba");
static_assert(offsetof(GamSysRec5, m_1ba) == 0x1ba, "m_1ba");
static_assert(offsetof(GamSysRec5, m_1bc) == 0x1bc, "m_1bc");
static_assert(offsetof(GamSysRec5, m_1c4) == 0x1c4, "m_1c4");
static_assert(offsetof(GamSysRec5, m_1cc) == 0x1cc, "m_1cc");
static_assert(offsetof(GamSysRec5, m_1d4) == 0x1d4, "m_1d4");
static_assert(offsetof(GamSysRec5, m_1d8) == 0x1d8, "m_1d8");
static_assert(offsetof(GamSysRec5, m_1fc) == 0x1fc, "m_1fc");

/* The system pack's file 0 (GamMan +0x22220, EE 0x5c; class name
 * invented): ten tables past the 0x40-byte header (offsets +0x10..+0x34),
 * a record id is table << 9 | record. layout: host -- pointers. */
struct GamSysData
{
    void set(u_char *data);                                              // ROR 0x001dbef8
    /* The record of id: table id >> 9 of the ten, record id & 0x1ff. */
    const void *getRec(short id);                                        // ROR 0x001dbfa0
    /* Ors bits into the record's flag word (+0x90 / +0xb0). */
    void setFlags(short id, u_int bits);                                 // ROR 0x001dcd38
    /* The record's flag word, 0 for a bad table. */
    u_int getFlags(short id);                                            // ROR 0x001dcf28
    /* The record's flag word & bits != 0 (0x20000: the item stacks). */
    int  testFlags(short id, u_int bits);                                // ROR 0x001dd00c
    /* Field field (a GamParam id or a table's own) of record id into out:
     * u_shorts, words, vectors, strings, or the address of a name (fields
     * 0xb / 0xc / 0x25: a pointer-sized slot on the host). */
    void getField(int id, int field, void *out);                         // ROR 0x001dc48c
    /* getField's counterpart: field of record id from *data (u_short
     * fields; table 0 reads into data instead, as the ROM does). */
    void setField(int id, int field, const void *data);                  // ROR 0x001dc070
    /* Clears bits in the record's flag word (setFlags' counterpart). */
    void clearFlags(short id, u_int bits);                               // ROR 0x001dce30
    /* Table 2 records only: the second flag word +0x94 (a2 unused). */
    void setFlags2(short id, int unused, u_int bits);                    // ROR 0x001dd118
    void clearFlags2(short id, int unused, u_int bits);                  // ROR 0x001dd154
    /* Table 2 records only: +0x94 & bits != 0 (a2 unused). */
    int  testFlags94(short id, int unused, u_int bits);                  // ROR 0x001dd194

    u_char *m_table[10];            // ROR 0x00: the record tables
    u_char *m_data;                 // ROR 0x58: the file
};

/* A secta cell (GamMan::findSectaCell's answer; built by 0x17c340, not
 * reconstructed): plain EE layout. */
struct GamSecta
{
    int   m_id;                     // ROR 0x00: ObjStat::get_o_secta_id
    int   _04;
    u_int m_flags;                  // ROR 0x08: bit 2, findSectaCell refuses it
    int   _0c[4];
    float m_x0;                     // ROR 0x1c: the bounds an object leaves (x0 <= x <= x1, z0 <= z <= z1)
    float m_z0;                     // ROR 0x20
    float m_x1;                     // ROR 0x24
    float m_z1;                     // ROR 0x28
};
static_assert(offsetof(GamSecta, m_flags) == 0x08, "m_flags");
static_assert(offsetof(GamSecta, m_x0) == 0x1c, "m_x0");
static_assert(offsetof(GamSecta, m_z1) == 0x28, "m_z1");

/* ObjStat::save's block: +0x100..+0x1b0 as the EE lays it out (GamMan
 * keeps the PC's and the dog's across a stage change). The cell words
 * are written 0: load clears them. */
struct ObjStatSave
{
    u_int  m_oid;                   // ROR 0x00 (+0x100)
    int    m_stat;                  // ROR 0x04
    int    m_actor;                 // ROR 0x08
    u_int  m_cells[3];              // ROR 0x0c: +0x10c / +0x110 / +0x114
    float  m_smellRange;            // ROR 0x18
    float  m_scale;                 // ROR 0x1c
    u_int  m_flags;                 // ROR 0x20
    u_short m_124;                  // ROR 0x24
    u_short m_lp;                   // ROR 0x26
    short  m_kind;                  // ROR 0x28
    u_short m_smellUid[3];          // ROR 0x2a
    int    m_areaNo;                // ROR 0x30
    int    m_134;                   // ROR 0x34
    int    m_inuFunc;               // ROR 0x38
    int    m_13c;                   // ROR 0x3c
    int    m_140;                   // ROR 0x40
    int    m_144;                   // ROR 0x44
    int    m_148;                   // ROR 0x48
    int    m_14c;                   // ROR 0x4c
    int    m_colPrm[12];            // ROR 0x50
    float  m_180[4];                // ROR 0x80
    float  m_pos[4];                // ROR 0x90
    float  m_rot[4];                // ROR 0xa0
};
static_assert(sizeof(ObjStatSave) == 0xb0, "ObjStatSave is ObjStat +0x100..+0x1b0");
static_assert(offsetof(ObjStatSave, m_flags) == 0x20, "m_flags");
static_assert(offsetof(ObjStatSave, m_kind) == 0x28, "m_kind");
static_assert(offsetof(ObjStatSave, m_areaNo) == 0x30, "m_areaNo");
static_assert(offsetof(ObjStatSave, m_148) == 0x48, "m_148");
static_assert(offsetof(ObjStatSave, m_colPrm) == 0x50, "m_colPrm");
static_assert(offsetof(ObjStatSave, m_pos) == 0x90, "m_pos");

/* A game object's state (vtable 0x2e4828, vptr at +0x258). Members are
 * named after the ELF's accessors where one reads them. layout: host --
 * EE offsets in the comments. */
struct ObjStat
{
    /* The vptr (ObjStat's), the 8-node pool at +0x40, oid, actor, kind,
     * a1 into o_stat and the words +0x110 / +0x114 / +0x130 / +0x140
     * cleared. The ROM passes a node count (0x40 / 8) it never reads. */
    ObjStat(int a1, u_int oid, int actor, short kind);                   // ROR 0x001d87f4
    /* EE slot 1 (+0x08): the deleting destructor. */
    virtual ~ObjStat() {}                                                // ROR 0x001d88c8
    /* MagScene::releaseActor(m_actor) when it has one, m_actor = -1; a
     * type-5 object also drops its GamMan pool 0x20b00 records. */
    void releaseActor();                                                 // ROR 0x001d966c
    /* The actor's trans (or +0x190 without one) into out[0..2]. */
    void getPos(float *out);                                             // ROR 0x001d890c
    /* The actor's rotation (or +0x1a0 without one) into out[0..2]. */
    void getRot(float *out);                                             // ROR 0x001d8964
    /* pos (x, y, z) into +0x190 and the actor's trans; an object with an
     * actor and no cell, or off the secta lists, gets its cell. */
    void setPos(const float *pos);                                       // ROR 0x001d89c4
    /* rot into +0x1a0 and the actor's rotation. */
    void setRot(const float *rot);                                       // ROR 0x001d8a8c
    /* The cell into +0x110 / +0x114; off the secta lists (flag 0x40) also
     * +0x10c and the entry position, queued for GamMan unless flag
     * 0x10000000. Returns 1 off the lists. */
    int  setSecta(GamSecta *cell);                                       // ROR 0x001d8d28
    /* The actor's position into +0x190 and out (x, y, z, w); a new cell
     * when it left the current one. */
    void updatePos(float *out);                                          // ROR 0x001d8f04
    /* The actor's position into +0x190; a new cell when it left the
     * current one's bounds. */
    void syncActorPos();                                                 // ROR 0x001d8dc4
    /* The cell of +0x190 from scratch. */
    void findSecta();                                                    // ROR 0x001d8ea8
    /* Hides (hide) / shows the actor through its setHide; with bit 0x40
     * of m_flags showing re-finds the cell, hiding re-links it. */
    void setHide(int hide);                                              // ROR 0x001d95c4
    /* Refreshes position and rotation, then copies +0x100..+0x1b0 to out
     * (an ObjStatSave). Returns 0xb0. */
    int  save(void *out);                                                // ROR 0x001d96ec
    /* save's inverse: the 0xb0 bytes back to +0x100, the cell words
     * cleared. Returns 0xb0. */
    int  load(const void *in);                                           // ROR 0x001d9830
    /* Clears the cell words (+0x10c too when all), then kills the tasks
     * (an achr's: every caller passes one). */
    void clearSecta(int all);                                            // ROR 0x001d9f10
    /* The three cell pointers cleared (no list is touched). */
    void clearSectaPtrs();                                               // ROR 0x001d88fc
    /* The base params (1..0x18, 0x1000..0x1005). */
    void getParam(int which, void *out);                                 // ROR 0x001d9044
    void setParam(int which, const void *data);                          // ROR 0x001d932c
    /* Work word no (+0x1b0 + no * 4); setWork stores one. */
    int  getWork(int no);                                                // ROR 0x001db17c
    void setWork(int no, int value);                                     // ROR 0x001db18c
    /* +0x100, the object id (the ELF's inline). */
    // ROR 0x0028f92c
    int  getOID() const { return (int)m_oid; }

    /* The ELF's param accessors (ACHRstat::initParamFuncs' table). */
    // ROR 0x0028f874
    void get_o_oid(void *out) { *(u_int *)out = m_oid; }
    // ROR 0x0028f880
    void get_o_uid(void *out) { *(short *)out = m_kind; }
    // ROR 0x0028f88c
    void get_o_chr_id(void *out) { *(int *)out = m_actor; }
    // ROR 0x0028f898
    void get_o_secta_id(void *out) { *(int *)out = m_secta != NULL ? m_secta->m_id : -1; }
    // ROR 0x0028f8b0
    void get_o_pos(void *out) { getPos((float *)out); }
    // ROR 0x0028f8cc
    void get_o_rot(void *out) { getRot((float *)out); }
    // ROR 0x0028f8e8
    void get_o_stat(void *out) { *(int *)out = m_stat; }
    // ROR 0x0028f8f4
    void get_o_area_no(void *out) { *(int *)out = m_areaNo; }
    // ROR 0x0028f900
    void get_o_inu_func(void *out) { *(int *)out = m_inuFunc; }
    // ROR 0x0028f90c
    void get_o_scale(void *out) { *(float *)out = m_scale; }
    // ROR 0x0028f918
    void get_o_smell_range(void *out) { *(float *)out = m_smellRange; }
    // ROR 0x0028fa04
    void getColPrm(void *out);
    // ROR 0x0028fa38
    void getColPrmKeep(void *out);

    SeqFixedPool<int, 8> m_nodes;   // ROR 0x40: free +0x40, used +0x50, max +0x60, nodes +0x80 (role unverified)
    u_int     m_oid;                // ROR 0x100: o_oid
    int       m_stat;               // ROR 0x104: o_stat (the constructor's a1; 0 from addChr / addVFchr)
    int       m_actor;              // ROR 0x108: o_chr_id, the scene actor, -1 none
    GamSecta *m_sectaLast;          // ROR 0x10c: the last cell entered off the secta lists
    GamSecta *m_110;                // ROR 0x110: setSecta's cell; findSecta clears it first
    GamSecta *m_secta;              // ROR 0x114: the current cell (o_secta_id), NULL none
    float     m_smellRange;         // ROR 0x118: o_smell_range (the record's)
    float     m_scale;              // ROR 0x11c: o_scale
    u_int     m_flags;              // ROR 0x120: 1 hidden, 0x40 off the secta lists, 0x10000 the PC / dog, 0x200000, 0x10000000 not queued
    u_short   m_124;                // ROR 0x124: param 0x1002
    u_short   m_lp;                 // ROR 0x126: a_lp (the record's +0x80 at init)
    short     m_kind;               // ROR 0x128: o_uid, the system data id
    u_short   m_smellUid[3];        // ROR 0x12a: o_smell_uid_1..3
    int       m_areaNo;             // ROR 0x130: o_area_no
    int       m_134;                // ROR 0x134: FCHRstat param 0x40
    int       m_inuFunc;            // ROR 0x138: o_inu_func, -1 after init
    int       m_13c;                // ROR 0x13c: -1 after init
    int       m_140;                // ROR 0x140: addVFchr sets 1
    int       m_144;                // ROR 0x144
    int       m_148;                // ROR 0x148: param 0x1000 (the model number)
    int       m_14c;                // ROR 0x14c
    int       m_colPrm[12];         // ROR 0x150: getColPrm (param 0x18): [0] kind (2 after init), [1] shape, [4] radius
    float     m_180[4];             // ROR 0x180: param 0x1001
    float     m_pos[4];             // ROR 0x190: o_pos
    float     m_rot[4];             // ROR 0x1a0: o_rot
    int       m_work[16];           // ROR 0x1b0: getWork / setWork
    float     m_quad[8];            // ROR 0x1f0: param 0x1003 (four x / z corners); setSecta puts the entry position in [0..3]
    char      m_name[0x10];         // ROR 0x210: param 0x1004
    int       m_colPrmKeep[12];     // ROR 0x220: getColPrmKeep (param 0x1005): init's copy of m_colPrm
    int       _250;                 // ROR 0x250
    int       m_link;               // ROR 0x254: the oid of an object shown with this one, -1 none
};

/* An actor character (vtable 0x2e4810, EE 0x480 bytes, built by
 * 0x1cc250). layout: host. */
struct ACHRstat : ObjStat
{
    typedef void (ACHRstat::*ParamFunc)(void *out);

    /* Inline in the ROM (addChr): the task ids -1 around the vptr store. */
    ACHRstat(u_int oid, int actor, short kind) : ObjStat(0, oid, actor, kind)
    {
        // PORT: the ROM leaves these to init and the heap; the host
        // starts them cleared (plain data from m_paramFunc to the end).
        for (int i = 0; i < 0x22; i++)
            m_paramFunc[i] = NULL;
        memset(m_param, 0, (char *)(this + 1) - (char *)m_param);
        m_taskId = -1;
        m_task[0] = -1;
        m_task[1] = -1;
        m_task[2] = -1;
        m_task[3] = -1;
        m_placeTask = -1;
    }
    /* The record of kind (GamSysData::getRec) and the defaults; flags from
     * the record | 0x200040. */
    void init(short kind);                                               // ROR 0x001d9908
    /* The param table +0x280: params 1..0x21 through the ELF's accessors. */
    void initParamFuncs();                                               // ROR 0x001d9b2c
    /* Pauses (on) / resumes its tasks: +0x3d4, +0x3e0, +0x3d8 and, with
     * all, +0x3dc. */
    void pauseTasks(int on, int all);                                    // ROR 0x001da130
    /* Param which into out: 1..0x21 through m_paramFunc, its own
     * 0x22..0x1201, the base 0x1000..0x1005. */
    void getParam(int which, void *out);                                 // ROR 0x001da228
    /* Stores *data into the param which (0x32 the equipped item ...). */
    void setParam(int which, const void *data);                          // ROR 0x001da740
    /* The motion speed into +0x418 and the actor's
     * MagScene::setActorMotionSpeed (native 106). */
    void setMotionSpeed(float speed);                                    // ROR 0x001db1b8
    /* Kills its four tasks (m_task[2], [0], [3], [1]) and sets them -1. */
    void killTasks();                                                    // ROR 0x001da07c

    /* The ELF's param accessors over the record. */
    // ROR 0x0028fbb4
    void get_o_name_eng(void *out) { if (m_rec != NULL) *(const char **)out = m_rec->m_head.m_name_eng; }
    // ROR 0x0028fbcc
    void get_o_name_jpn(void *out) { if (m_rec != NULL) *(const char **)out = m_rec->m_head.m_name_jpn; }
    // ROR 0x0028fbe0
    void get_o_icon_no(void *out) { if (m_rec != NULL) *(u_short *)out = m_rec->m_head.m_icon_no; }
    // ROR 0x0028fbfc
    void get_o_chr_id_high(void *out) { if (m_rec != NULL) *(u_short *)out = m_rec->m_head.m_chr_id_high; }
    // ROR 0x0028fc18
    void get_o_chr_id_low(void *out) { if (m_rec != NULL) *(u_short *)out = m_rec->m_head.m_chr_id_low; }
    // ROR 0x0028fc34
    void get_o_smell_uid_1(void *out) { if (m_rec != NULL) *(u_short *)out = m_rec->m_head.m_smell_uid[0]; }
    // ROR 0x0028fc50
    void get_o_smell_uid_2(void *out) { if (m_rec != NULL) *(u_short *)out = m_rec->m_head.m_smell_uid[1]; }
    // ROR 0x0028fc6c
    void get_o_smell_uid_3(void *out) { if (m_rec != NULL) *(u_short *)out = m_rec->m_head.m_smell_uid[2]; }
    // ROR 0x0028fc88
    void get_o_menu_offset_pos(void *out);
    // ROR 0x0028fca4
    void get_o_menu_offset_rot(void *out);
    // ROR 0x0028fcc0
    void get_o_node_name(void *out);
    // ROR 0x0028fcec
    void get_a_move_type(void *out) { *(int *)out = m_task[0]; }
    // ROR 0x0028fcf8
    void get_a_offset_rot(void *out) { *(float *)out = m_offsetRot; }
    // ROR 0x0028fd04
    void get_a_lp(void *out) { *(u_short *)out = m_lp; }
    // ROR 0x0028fd10
    void get_a_max_lp(void *out) { if (m_stats != NULL) *(u_short *)out = m_stats->m_max_lp; }
    // ROR 0x0028fd2c
    void get_a_st(void *out) { if (m_stats != NULL) *(u_short *)out = m_stats->m_st; }
    // ROR 0x0028fd48
    void get_a_def(void *out) { if (m_stats != NULL) *(u_short *)out = m_stats->m_def; }
    // ROR 0x0028fd64
    void get_a_avoid(void *out) { if (m_stats != NULL) *(u_short *)out = m_stats->m_avoid; }
    // ROR 0x0028fd80
    void get_a_agi(void *out) { if (m_stats != NULL) *(u_short *)out = m_stats->m_agi; }
    // ROR 0x0028fd9c
    void get_a_neck_rot_range(void *out) { if (m_stats != NULL) *(u_short *)out = m_stats->m_neck_rot_range; }

    ParamFunc m_paramFunc[0x22];    // ROR 0x280: 8 bytes each (GCC 2 {delta, index, pfn}); [0] and [9] none
    int       m_param[16];          // ROR 0x390: params 0x22..0x31, words
    int       _3d0;
    int       m_task[4];            // ROR 0x3d4: task ids, -1 none; [0] a_move_type, [1] the turn task (chrRotStart)
    int       m_placeTask;          // ROR 0x3e4: startAchrPlaceTask's TskSeqEvent, -1 none
    int       m_taskId;             // ROR 0x3e8: resetObj kills it
    u_int     m_attr[4];            // ROR 0x3ec: setObjAttr / testAchrFlag's words (bits >> 24 picks one, the low 24 bits test); [2] bit 0: switchEnemies hid it (setObj clears it)
    float     m_offsetRot;          // ROR 0x3fc: a_offset_rot (param 0x1a)
    const GamSysChrRec *m_rec;      // ROR 0x400: the system record (tables 0..3)
    const GamSysStats  *m_stats;    // ROR 0x404: its stats (+0x80)
    float     m_408;                // ROR 0x408: param 0x1201
    int       m_40c;                // ROR 0x40c: param 0x1100, 2 after init
    int       m_410;                // ROR 0x410: param 0x1101, -1 after init
    int       m_414;                // ROR 0x414: param 0x1102, -1 after init
    float     m_motionSpeed;        // ROR 0x418: param 0x1103, setMotionSpeed; 1 after init
    float     m_41c;                // ROR 0x41c: param 0x1104, 1 after init
    float     m_420;                // ROR 0x420: param 0x1105, pi after init
    const GamSysChrRec *m_rec2;     // ROR 0x424: the record again (init reads it)
    u_short   m_428;                // ROR 0x428: param 0x32 (the equipped item)
    u_short   m_42a;                // ROR 0x42a: param 0x33; init (table 0): the record's max lp
    u_short   m_42c[8];             // ROR 0x42c: params 0x34..0x38 (one each), 0x59 (all 16 bytes); table 0: [1], [2] 0x80
    int       m_43c;                // ROR 0x43c: param 0x67
    u_short   m_440;                // ROR 0x440: param 0x5a
    u_short   _442;
    float     m_444;                // ROR 0x444: param 0x5b
    int       m_448;                // ROR 0x448: param 0x5c
    u_short   m_44c;                // ROR 0x44c: param 0x5d
    u_short   m_44e;                // ROR 0x44e: param 0x5e
    float     m_450;                // ROR 0x450: param 0x5f
    u_short   m_454[4];             // ROR 0x454: params 0x60..0x63
    u_int     m_45c;                // ROR 0x45c: param 0x64 (set from a u_short)
    float     m_460;                // ROR 0x460: param 0x65
    u_short   m_464;                // ROR 0x464: param 0x66; 0xa00 (no item) for table 2
    u_short   m_466;                // ROR 0x466: param 0x68
    u_short   m_468[7];             // ROR 0x468: params 0x69..0x6f
};

/* A field character (vtable 0x2e47f8, EE 0x2c0 bytes). layout: host. */
struct FCHRstat : ObjStat
{
    /* Inline in the ROM (addChr / addVFchr). */
    FCHRstat(u_int oid, int actor, short kind)
        : ObjStat(0, oid, actor, kind), m_rec(NULL), m_stats(NULL), m_rec2(NULL), m_28c(0), m_290(0)
    {
        // PORT: the ROM leaves these to init and the heap.
        for (int i = 0; i < 4; i++)
            m_294[i] = 0;
    }
    /* The record of kind (table 5) and the defaults. */
    void init(short kind);                                               // ROR 0x001db1f0
    /* ObjStat's position and rotation twins; setPos also finds a cell
     * for an object without an actor. */
    void setPos(const float *pos);                                       // ROR 0x001db7cc
    void getPos(float *out);                                             // ROR 0x001db71c
    void setRot(const float *rot);                                       // ROR 0x001db898
    void getRot(float *out);                                             // ROR 0x001db774
    /* One of its params by which (0x41 the type-9 number, 0x18 ...). */
    void getParam(int which, void *out);                                 // ROR 0x001db31c
    /* getParam's counterpart: *data into the field by which (0x1000 the
     * model number, 0x1004 its name, 0x40, 3 the actor ...). */
    void setParam(int which, const void *data);                          // ROR 0x001db520

    const GamSysRec5  *m_rec;       // ROR 0x280: the table-5 record, NULL for other tables
    const void        *m_stats;     // ROR 0x284: the record's +0x80
    const GamSysRec5  *m_rec2;      // ROR 0x288: the record again
    int       m_28c;                // ROR 0x28c: params 0x20 / 0x36 / 0x41 (words), 0x32 (u_short); -1 for table 9
    int       m_290;                // ROR 0x290: params 0x33 / 0x37 / 0x3c (words), 0x21 (u_short); 1 for table 5
    int       m_294[4];             // ROR 0x294: [0] params 0x34 / 0x38 / 0x3f (-1 for table 9), [1] 0x35 / 0x3a; 0x43 all four
};

/* The WRD room coordinates (the ROM passes GamMan +0x18c00 as a this it
 * never reads): op 0 world to local, 1 local to world (the WRD table's
 * converters with the room GamMan +0x21f7c), 2 the room's x, 0, z
 * offset into a. The ELF's inlines GamMan::cnvPosW2L / cnvPosL2W /
 * getWrdOffset call it. */
void gamWrdConvPos(int op, float *a, float *b);                          // ROR 0x001dba3c
/* The room offset's x / z of GamMan's room, 0 without a WRD entry (the
 * ROM's this, GamMan +0x18c00, is not read). */
float gamGetWrdOfsX(void);                                               // ROR 0x001db9c4
float gamGetWrdOfsZ(void);                                               // ROR 0x001dba00
/* The WRD table entry of GamMan's room (WrdTable::get): its words 0x18 /
 * 0x1c into GamMan +0x18dd0 / +0x18dc0, +0x18dd0 = 0 without an entry.
 * The ROM's this is GamMan +0x18c00; initStage2 passes the room's
 * A%03d.ARA file as a second argument it does not read. */
struct GamMan;
void gamSetWrdRoom(GamMan *gam, const void *file);                      // ROR 0x001dbcf0

#endif /* _GAME_GAMSTAT_H */
