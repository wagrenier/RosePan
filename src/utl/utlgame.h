/* ==========================================================================
 *  utl/utlgame.h
 *
 *  utl/utlgame.cpp (ROR 0x1024d0..0x1041c4): the game's one-call queries
 *  and commands on top of GamMan, its sound table, EmyManager and the
 *  scene's actors -- sound ids by table index (tbl_sound), object / item
 *  / door searches over GamMan's object lists, the enemy info buffers the
 *  PC's attacks and Brown's commands sort, and the dog's / Brown's params.
 * ======================================================================== */
#ifndef _UTL_UTLGAME_H
#define _UTL_UTLGAME_H

#include <stddef.h>

struct FCHRstat;

/* ---- the sound table ---- */

/* One row of tbl_sound (data 0x2e6978, 200 rows of EE 0x10; m_tag is the
 * asserts' name, == the row). layout: host -- m_name is a pointer. */
struct UtlSoundRec
{
    unsigned int m_se;          // ROR 0x0: the SE id GamSndMan::playSE takes
    int          m_link;        // ROR 0x4: another row to use instead, -1 none (none in the retail table)
    int          m_tag;         // ROR 0x8: the row's own index
    const char  *m_name;        // ROR 0xc: "SE_SAVE_FINISH" ...
};
#define UTL_SOUND_MAX 200

/* Row no's name (following m_link). */
const char *utlGetSoundName(int no);                                     // ROR 0x001024d0
/* Row theN's SE id (following m_link); 0 when theN >= 200. */
unsigned int utlGetSoundSE(unsigned int theN);                           // ROR 0x001025c0
/* Plays sound id at pos (NULL: no position) on the sound object 0x79a0c0;
 * id is an SE id, not a table row. */
void utlPlaySe3d(int id, const float *pos);                              // ROR 0x0010254c
/* Plays table row id: at pos, or with no position after delay frames
 * (delay > 0: 0x1f97c8 scnPlaySEDelayed), or now. */
void utlPlaySe(int id, int delay, const float *pos);                     // ROR 0x001026b0
/* Row no: plays it unless it is playing / stops it if it is. */
void utlPlaySeIfIdle(int no);                                            // ROR 0x00102758
void utlStopSe(int no);                                                  // ROR 0x001027d4
/* Plays row no when utlRandRange(0, range) < chance; returns whether. */
int  utlPlaySeChance(int no, int chance, int range);                     // ROR 0x0010284c
/* One of two / three / four rows at random (utlRandRange 0..100 per
 * row), played without a position. */
void utlPlaySeRand2(int a, int b);                                       // ROR 0x001028a8
void utlPlaySeRand3(int a, int b, int c);                                // ROR 0x00102918
void utlPlaySeRand4(int a, int b, int c, int d);                         // ROR 0x001029c4

/* ---- objects ---- */

/* GamMan::getObjPos(oid, out). */
void utlGetObjPos(float *out, unsigned int oid);                         // ROR 0x00102ab8
/* EmyManager::isEmyDamaging / testEmyStateFlag (assert theOID != 0). */
int  utlIsEmyDamaging(unsigned int theOID);                              // ROR 0x00102adc
int  utlTestEmyStateFlag(unsigned int theOID);                           // ROR 0x00102b24

/* The searches below return uid << 32 | oid of the object found (its
 * position into out), 0 for none: the ROM returns the 64-bit value in v0.
 * uid is the object's system data id (ObjStat::m_kind, +0x128). */
/* For each of the n uids: utlFindFreeObjByUid, then utlFindItemByUid; the
 * first found. utlFindMatchObj's tail call. */
unsigned long long utlFindItemByUids(float *out, const int *uids, int n); // ROR 0x00102b6c
/* GamMan::m_avfchrList: the first object of uid when the uid's system
 * record has no flag 2, else the last one. */
unsigned long long utlFindItemByUid(float *out, unsigned int uid);       // ROR 0x00102bf0
/* GamMan::m_vfchrList's first object of uid, NULL none. */
FCHRstat *utlFindVFchrByUid(unsigned int uid);                           // ROR 0x00102d54
/* GamMan::m_fchrList's first object of uid while the uid's system record
 * has no flag 2. */
unsigned long long utlFindFreeObjByUid(float *out, unsigned int uid);    // ROR 0x0010315c
/* Item's three related uids (system data fields 0x10..0x12) and item
 * itself into out (table 6: -1 instead); returns 4, or 3 for table 6. */
int  utlGetItemUids(int *out, int theMax, unsigned int item);            // ROR 0x00103240
/* The object's item (getObjParam 2) into the inventory, the object
 * released. No callers. */
void utlTakeObjItem(unsigned int oid);                                   // ROR 0x0010332c
/* Oid type 5 -> 1, 6 -> 2, else 0. No callers. */
int  utlGetOidTypeKind(unsigned int oid);                                // ROR 0x00103390
/* GamSysData::setFlags(id, 2) / testFlags(id, 2). */
void utlSetItemFlag2(short id);                                          // ROR 0x001033b8
int  utlIsItemFlag2(short id);                                           // ROR 0x001033f0
/* GamMan::markObjTaken / isObj on the global GamMan. */
void utlMarkObjTaken(unsigned int oid);                                  // ROR 0x00103428
int  utlIsObj(unsigned int oid);                                         // ROR 0x0010344c

/* utlFindDoorBetween's answer. EE 0x24. layout: host -- m_obj is a
 * pointer; EE offsets in the comments. */
struct UtlDoorInfo
{
    float     m_pos[4];         // ROR 0x00: the door object's position
    int       m_room[2];        // ROR 0x10: the two rooms it links (ctlObjParam 0xc)
    float     m_rot_y;          // ROR 0x18: its heading (+0x1a4), wrapped into -pi .. pi
    FCHRstat *m_obj;            // ROR 0x1c
    int       m_kind;           // ROR 0x20: 1 / 2 / 3 its record flag 8 / 0x10 / 0x20, else 0 (3 turns the heading by pi)
};

/* The door object (kind 9) linking rooms a and b, the nearest to pos;
 * 1 found. */
int utlFindDoorBetween(UtlDoorInfo *out, int a, int b, const float *pos); // ROR 0x00102dc0

/* ---- the game state ---- */

/* g_sequence.m_bits[1] & 4: script flag 34 (MagLifePC::processPad's dog
 * commands need it). */
int  utlTestSeqBit34(void);                                              // ROR 0x0010346c
/* GamMan's option 0x12 (asked through 0x1c6ccc) != 0: the controller
 * vibration switch MagSystem gates BedPad::setVibration on. */
int  utlIsVibrationOn(void);                                             // ROR 0x00103484
/* GamMan::getStatus(5): m_wrdNo, the current room. */
int  utlGetWrdNo(void);                                                  // ROR 0x001034b8
/* GamMan::getActorNo(oid). */
int  utlGetActorNo(unsigned int oid);                                    // ROR 0x001034e4
/* With no menu open: GamMan::pause(0x100000) and m_flags |= 0x400;
 * returns whether it paused. */
int  utlPauseForMenu(void);                                              // ROR 0x00103de4
/* GamMan's PC equip item / PC actor numbers (assert id < 128). */
int  utlGetPcEquipItemChrID(void);                                       // ROR 0x00103e4c
int  utlGetPcChrNo(void);                                                // ROR 0x00103e94

/* ---- enemies ---- */

/* One enemy of utlCollectEmyInfo (EE 0x40 bytes). */
struct UtlEnemyInfo
{
    float        m_pos[4];      // ROR 0x00: the 'Bip01 Spine1' node, else m_trans
    float        m_trans[4];    // ROR 0x10: the actor's m_trans
    unsigned int m_obj_id;      // ROR 0x20: the object's oid (type bits 23..27 == 2)
    int          m_24;          // ROR 0x24: the object's uid (u16 +0x128)
    int          m_actor_no;    // ROR 0x28: the object's actor number (+0x108)
    float        m_radius;      // ROR 0x2c: EmyManager::getEmyTypeDist
    float        m_dist;        // ROR 0x30: utlCollectEmyInfoAt: the distance from its pos
    float        m_rot;         // ROR 0x34: utlCollectEmyInfoAt: the angle from pos less rot
    int          _38;
    unsigned int m_flags;       // ROR 0x3c: the object's word +0x120 (0x400 / 0x2000 skip it)
};
static_assert(sizeof(UtlEnemyInfo) == 0x40, "UtlEnemyInfo is 0x40 bytes on the EE");
static_assert(offsetof(UtlEnemyInfo, m_trans) == 0x10, "m_trans");
static_assert(offsetof(UtlEnemyInfo, m_obj_id) == 0x20, "m_obj_id");
static_assert(offsetof(UtlEnemyInfo, m_24) == 0x24, "m_24");
static_assert(offsetof(UtlEnemyInfo, m_actor_no) == 0x28, "m_actor_no");
static_assert(offsetof(UtlEnemyInfo, m_radius) == 0x2c, "m_radius");
static_assert(offsetof(UtlEnemyInfo, m_dist) == 0x30, "m_dist");
static_assert(offsetof(UtlEnemyInfo, m_rot) == 0x34, "m_rot");
static_assert(offsetof(UtlEnemyInfo, m_flags) == 0x3c, "m_flags");

/* The asserts' UtlEnemyInfoBuf (EE 0x804 bytes on the caller's stack). */
struct UtlEnemyInfoBuf
{
    enum { MAX_INFO = 32 };
    UtlEnemyInfo m_info[MAX_INFO];  // ROR 0x000
    int          m_count;           // ROR 0x800: utlCollectEmyInfo's count
};
static_assert(offsetof(UtlEnemyInfoBuf, m_count) == 0x800, "m_count");
static_assert(sizeof(UtlEnemyInfoBuf) == 0x804, "UtlEnemyInfoBuf is 0x804 bytes on the EE");

/* The live enemies (flag 0: those without 0x400, else without 0x2000),
 * at most 32, into eib; returns the count. */
int utlCollectEmyInfo(UtlEnemyInfoBuf *eib, int flag);                   // ROR 0x00103a90
/* utlCollectEmyInfo seen from pos: each one's distance (m_dist) and
 * angle less rot (m_rot) by utlVecDistRotY, keeping those within radius
 * (radius <= 0: none); kind 0 no type distance, 1 / 2
 * EmyManager::getEmyTypeDist / Dist2 into m_radius. */
int utlCollectEmyInfoAt(UtlEnemyInfoBuf *eib, const float *pos, int kind, int flag,
                        float rot, float radius);                      // ROR 0x0010376c
/* The items (GamMan::m_avfchrList; flag: not those with 0x20 in +0x120),
 * at most 32, into an UtlEnemyInfoBuf: position, m_obj_id, m_24. Returns
 * the count (m_count untouched). */
int utlCollectItemInfo(UtlEnemyInfoBuf *eib, int flag);                  // ROR 0x00103ccc
/* The actor of enemy oid: its 'Bip01 Spine1' world position into out;
 * returns EmyManager::getEmyTypeDist (kind != 0) or getEmyTypeDist2, 0
 * without an actor. No callers. */
float utlGetEmySpinePos(float *out, unsigned int oid, int kind);         // ROR 0x00103510
/* The entry of theInfo with the least m_dist among those within +-theR
 * of m_rot; a single entry is 0 whatever its angle, none -1. */
int utlFindNearestEmyInRange(float theR, const UtlEnemyInfoBuf &theInfo); // ROR 0x00103654

/* ---- the dog / Brown (GamMan::m_dogPtr's object) ---- */

/* The dog's params 0x1b / 0x1c through GamMan::get/setDogParam
 * (MagLifeBwn::calcHealth's ratio; reset tops 0x1b up). */
void utlSetDogParam1b(int v);                                            // ROR 0x00103edc
int  utlGetDogParam1b(void);                                             // ROR 0x00103f08
int  utlGetDogParam1c(void);                                             // ROR 0x00103f38
/* EmyManager::getEmyHoldWord248 / getEmyWord238 of the enemy oid. */
int  utlGetEmyHold(unsigned int oid);                                    // ROR 0x00103f68
int  utlGetEmyWord238(unsigned int oid);                                 // ROR 0x00103f88
/* EmyManager::getEmyWord3c(oid) == 7 / == 4. */
int  utlIsEmyState7(unsigned int oid);                                   // ROR 0x00103fac
int  utlIsEmyState4(unsigned int oid);                                   // ROR 0x00103fd8
/* GamMan::setObjRadius(the dog's oid, r), then testHitAt(pos, the dog's
 * oid, 0, 1): non-zero on a hit, the result's word +0x10 into *hit. */
int  utlTestDogHitAt(const float *pos, unsigned int *hit, float r);      // ROR 0x00104004
/* GamMan::getObjRadius / setObjRadius of the dog (oid 0 without one). */
float utlGetDogRadius(void);                                             // ROR 0x001040a0
void utlSetDogRadius(float r);                                           // ROR 0x001040dc
/* GamMan::getObjParam(oid, 2): the object's kind (0x401 / 0x421 ...). */
int  utlGetObjKind(unsigned int oid);                                    // ROR 0x0010411c
/* GamMan::setObj(the dog's oid, 0x40); both values of on do the same in
 * the ROM. */
void utlSetBwnObjFlag(int on);                                           // ROR 0x00104150

#endif /* _UTL_UTLGAME_H */
