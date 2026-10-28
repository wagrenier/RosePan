/* ==========================================================================
 *  utl/utlgame.h
 *
 *  Game-state queries (utl/utlgame.cpp). Minimal: only what reconstructed
 *  code calls so far.
 * ======================================================================== */
#ifndef _UTL_UTLGAME_H
#define _UTL_UTLGAME_H

#include <stddef.h>

/* GamMan's option 0x12 (asked through 0x1c6ccc) != 0: the controller
 * vibration switch MagSystem gates BedPad::setVibration on. */
int utlIsVibrationOn(void);                                              // ROR 0x00103484
/* Plays sound id at pos (NULL: no position) on the sound object 0x79a0c0. */
void utlPlaySe3d(int id, const float *pos);                              // ROR 0x0010254c
/* GamMan::getActorNo(oid). */
int utlGetActorNo(unsigned int oid);                                     // ROR 0x001034e4
/* GamMan::getStatus(5): m_wrdNo, the current room. */
int utlGetWrdNo(void);                                                   // ROR 0x001034b8
/* With no menu open: GamMan::pause(0x100000) and bss 0x7a3208 |= 0x400;
 * returns whether it paused. */
int utlPauseForMenu(void);                                               // ROR 0x00103de4
/* GamMan's PC equip item / PC actor numbers (assert id < 128). */
int utlGetPcEquipItemChrID(void);                                        // ROR 0x00103e4c
int utlGetPcChrNo(void);                                                 // ROR 0x00103e94
/* One of three / four sound ids at random (0x10423c), played without a
 * position (0x1026b0). */
void utlPlaySeRand3(int a, int b, int c);                                // ROR 0x00102918
void utlPlaySeRand4(int a, int b, int c, int d);                         // ROR 0x001029c4
/* g_sequence.m_bits[1] & 4: script flag 34 (MagLifePC::processPad's dog
 * commands need it). */
int utlTestSeqBit34(void);                                               // ROR 0x0010346c
/* Plays sound id without a position: a > 0 through 0x1f97c8(se, a), else
 * GamSndMan::playSE(se, 1, pos) (pos != NULL: a position). */
void utlPlaySe(int id, int a, const float *pos);                         // ROR 0x001026b0
/* EmyManager::isEmyDamaging(theOID) (asserts theOID != 0). */
int utlIsEmyDamaging(unsigned int theOID);                               // ROR 0x00102adc
/* GamMan::getObjPos(oid, out). */
void utlGetObjPos(float *out, unsigned int oid);                         // ROR 0x00102ab8
/* EmyManager::testEmyStateFlag(oid); asserts oid != 0. */
int utlTestEmyStateFlag(unsigned int oid);                               // ROR 0x00102b24

/* One enemy of utlCollectEmyInfo (EE 0x40 bytes). */
struct UtlEmyInfo
{
    float        m_pos[4];      // ROR 0x00: the 'Bip01 Spine1' node, else m_trans
    float        m_trans[4];    // ROR 0x10: the actor's m_trans
    unsigned int m_obj_id;      // ROR 0x20: the object's oid (type bits 23..27 == 2)
    int          m_24;          // ROR 0x24: the object's u16 +0x128
    int          m_actor_no;    // ROR 0x28: the object's actor number (+0x108)
    float        m_radius;      // ROR 0x2c: EmyManager::getEmyTypeDist
    float        m_dist;        // ROR 0x30: utlCollectEmyInfoAt: the distance from its pos
    float        m_rot;         // ROR 0x34: utlCollectEmyInfoAt: the angle from pos less rot
    int          _38;
    unsigned int m_flags;       // ROR 0x3c: the object's word +0x120 (0x400 / 0x2000 skip it)
};
static_assert(sizeof(UtlEmyInfo) == 0x40, "UtlEmyInfo is 0x40 bytes on the EE");
static_assert(offsetof(UtlEmyInfo, m_obj_id) == 0x20, "m_obj_id");
static_assert(offsetof(UtlEmyInfo, m_radius) == 0x2c, "m_radius");
static_assert(offsetof(UtlEmyInfo, m_flags) == 0x3c, "m_flags");

/* The asserts' eib (EE 0x804 bytes on the caller's stack). */
struct UtlEmyInfoBuf
{
    UtlEmyInfo m_info[32];      // ROR 0x000
    int        m_num;           // ROR 0x800: utlCollectEmyInfo's count
};
static_assert(offsetof(UtlEmyInfoBuf, m_num) == 0x800, "m_num");

/* The live enemies (flag 0: those without 0x400, else without 0x2000),
 * at most 32, into eib; returns the count. */
int utlCollectEmyInfo(UtlEmyInfoBuf *eib, int flag);                     // ROR 0x00103a90
/* utlCollectEmyInfo seen from pos: each one's distance (m_info +0x30) and
 * angle less rot (+0x34) by utlVecDistRotY, keeping those within radius
 * (radius <= 0: none); kind 0 no type distance, 1 / 2
 * EmyManager::getEmyTypeDist / Dist2 into m_radius. */
int utlCollectEmyInfoAt(UtlEmyInfoBuf *eib, const float *pos, int kind, int flag,
                        float rot, float radius);                      // ROR 0x0010376c
/* The items (list 0x7a01d0; flag: not those with 0x20 in +0x120), at
 * most 32, into an UtlEmyInfoBuf: position, m_obj_id, m_24. Returns the
 * count (m_num untouched). */
int utlCollectItemInfo(UtlEmyInfoBuf *eib, int flag);                    // ROR 0x00103ccc
struct FCHRstat;

/* utlFindDoorBetween's answer. EE 0x24. layout: host -- m_obj is a
 * pointer; EE offsets in the comments. */
struct UtlDoorInfo
{
    float     m_pos[4];         // ROR 0x00: the door object's position
    int       m_room[2];        // ROR 0x10: the two rooms it links (ctlObjParam 0xc)
    float     m_rot_y;          // ROR 0x18: its heading (+0x1a4), wrapped into -pi .. pi
    FCHRstat *m_obj;            // ROR 0x1c
    int       m_kind;           // ROR 0x20: 0..3 from three ctlObjParam 2 tests (3 turns the heading by pi)
};

/* The door object (kind 9) linking rooms a and b, the nearest to pos;
 * 1 found. */
int utlFindDoorBetween(UtlDoorInfo *out, int a, int b, const float *pos); // ROR 0x00102dc0
/* GamMan::isObj / markObjTaken on the global GamMan. */
int  utlIsObj(unsigned int oid);                                         // ROR 0x0010344c
void utlMarkObjTaken(unsigned int oid);                                  // ROR 0x00103428

#endif /* _UTL_UTLGAME_H */
