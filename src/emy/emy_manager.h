/* ==========================================================================
 *  emy/emy_manager.h
 *
 *  EmyManager: the task that owns every enemy (up to 64 EmyMain objects),
 *  creates them by type, runs them each frame and spawns the members of
 *  its enemy groups. One global at bss 0x7b2140 (system/system.cpp), added
 *  as a task by main. Code 0x16265c..0x164118; no source string, the file
 *  is named after the class in emy_main.cpp's style.
 * ======================================================================== */
#ifndef _EMY_EMY_MANAGER_H
#define _EMY_EMY_MANAGER_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "task/tsk_man.h"
#include "emy/emy_main.h"

/* One enemy group (EE 0x3c, at EmyManager +0x148). setGroup fills it;
 * spawnGroups wakes or respawns its members (EmyMain::m_grp_no == the
 * group's index; 0 is "no group" and only has a motion). */
struct EmyGroup
{
    int   m_max;                // ROR 0x00: members out at once (setGroup's b, < 0x11)
    int   m_interval;           // ROR 0x04: frames between appearances (c, at least 60): waits are random in c/2 .. 3c/2
    int   m_count;              // ROR 0x08: setGroup's count (< 0x11)
    int   m_motion;             // ROR 0x0c: SetGrpMotion / GetGrpMotion (native 212 case 24); createEmy sets group 0's to 30
    float m_pos[3];             // ROR 0x10: where the members appear
    int   _1c;                  // ROR 0x1c
    float m_f;                  // ROR 0x20: setGroup's float, passed to updateGroup
    int   m_active;             // ROR 0x24: setGroup / setSlotWord24 (native 212 case 30)
    int   m_busy;               // ROR 0x28: a member is appearing (clearGrpBusy ends it)
    int   m_first;              // ROR 0x2c: the first member came (it waits no frames)
    int   m_alive;              // ROR 0x30: members out (countGrpDead takes one)
    int   m_respawn;            // ROR 0x34: respawns so far (a gone member waits for its number)
    int   m_dead;               // ROR 0x38: countGrpDead's count
};
static_assert(sizeof(EmyGroup) == 0x3c, "EmyGroup EE size");
static_assert(offsetof(EmyGroup, m_motion) == 0x0c, "m_motion");
static_assert(offsetof(EmyGroup, m_pos) == 0x10, "m_pos");
static_assert(offsetof(EmyGroup, m_f) == 0x20, "m_f");
static_assert(offsetof(EmyGroup, m_active) == 0x24, "m_active");
static_assert(offsetof(EmyGroup, m_busy) == 0x28, "m_busy");
static_assert(offsetof(EmyGroup, m_first) == 0x2c, "m_first");
static_assert(offsetof(EmyGroup, m_alive) == 0x30, "m_alive");
static_assert(offsetof(EmyGroup, m_respawn) == 0x34, "m_respawn");
static_assert(offsetof(EmyGroup, m_dead) == 0x38, "m_dead");

enum { EMY_MAX = 64, EMY_TYPE_MAX = 12, EMY_GROUP_MAX = 8 };

/* EmyManager : CmnITask (vtable 0x2e31a0, EE 0x430; vptr at +0, as
 * CmnITask has no data). layout: host -- polymorphic, pointers; EE
 * offsets in the comments. The per-enemy wrappers find the enemy by oid
 * and do nothing (0 / false; testEmyView 1) when there is none. */
struct EmyManager : CmnITask
{
    EmyManager();                                                        // ROR 0x0016265c
    ~EmyManager();                                                       // EE slot 6 (+0x30), ROR 0x00162700

    int  get_task_type() { return 3; }                                   // EE slot 1 (+0x08), ROR 0x0028a6ec
    void init() {}                                                       // EE slot 3 (+0x18), ROR 0x0028a6f4
    /* spawnGroups, then updateEmys; never ends. */
    int  exec(int id, int time, int arg);                                // EE slot 4 (+0x20), ROR 0x0016275c
    void term() {}                                                       // EE slot 5 (+0x28), ROR 0x0028a6fc

    void clear();                                                        // ROR 0x00162698
    void spawnGroups();                                                  // ROR 0x0016278c
    void updateEmys();                                                   // ROR 0x001629c0
    /* Group a (< 9) set up: b (< 0x11), c (at least 60), count (< 0x11),
     * d, the position and f; on. */
    void setGroup(float f, int a, int b, int c, int count, int d,
                  const float *pos);                                     // ROR 0x00162b68
    void setSlotWord24(int no, int v);                                   // ROR 0x00162afc
    void countGrpDead(int no);                                           // ROR 0x00162b14
    void clearGrpBusy(int no);                                           // ROR 0x00162b50
    /* A new enemy of type (0..10) for the achr chr: its name (the system
     * data kind), its param and transition tables, the position. oid 0
     * (addChrGroup's achrs): one is made, 0x9100006f + a count, set in
     * GamMan's achr slot table and the achr's param 1. group is the
     * group it joins (addChr passes 0). Returns the oid, 0 when no enemy
     * was added. */
    u_int addEmy(int type, short name, ACHRstat *chr, const _tagEnemyParam *prm,
                 const _tagEnemyTransition *trans, float *pos, u_int oid,
                 int group = 0);                                         // ROR 0x00162c2c
    u_int addEmyAlt(int type, short name, ACHRstat *chr, const _tagEnemyParam *prm,
                    const _tagEnemyTransition *trans, float *pos, u_int oid,
                    short carry);                                        // ROR 0x00162c5c
    u_int createEmy(int type, short name, ACHRstat *chr, const _tagEnemyParam *prm,
                    const _tagEnemyTransition *trans, float *pos, u_int oid,
                    int group, int makeOid, short carry);                // ROR 0x00162c88
    /* Deletes the enemy whose oid this is. */
    void releaseObj(u_int oid);                                          // ROR 0x001634d4
    void releaseAll();                                                   // ROR 0x00163570
    /* Clears its stage work: the oid pair, the oid count and the groups. */
    void resetWork();                                                    // ROR 0x001635e0
    /* The enemy (none: nothing) gets EmyMain::notify(bits); TskDamage
     * sends 1 hit, 2 killed, 4 / 8 by its m_40, 0x10 for word 6. */
    void notifyEmy(u_int oid, int bits);                                 // ROR 0x00163624
    void setEmyActive(u_int oid, int on);                                // ROR 0x0016366c
    /* The enemy whose oid this is, NULL when none. */
    EmyMain *findEmy(u_int oid);                                         // ROR 0x001636a4
    EmyMain *FindEmy(u_int oid) { return findEmy(oid); }                 // ROR 0x0028a734
    bool hasOid(int which);                                              // ROR 0x001636e4
    void setOid(int which, u_int oid);                                   // ROR 0x00163700
    void clearOid(int which);                                            // ROR 0x00163720
    bool testEmyHittable(u_int oid);                                     // ROR 0x0016373c
    bool testEmyStateFlag(u_int oid);                                    // ROR 0x00163770
    int  isEmyVisible(u_int oid);                                        // ROR 0x001637a4
    /* 1 when an enemy of type 0..2 in state 0xb / 0xc has oid as its
     * out's target. */
    int  isEmyTarget(u_int oid);                                         // ROR 0x001637cc
    float getEmyTypeDist(u_int oid);                                     // ROR 0x001638fc
    float getEmyTypeDist2(u_int oid);                                    // ROR 0x00163958
    float getEmyTypeRate(u_int oid);                                     // ROR 0x001639b4
    bool isEmyDamaging(u_int oid);                                       // ROR 0x00163a10
    void cancelEmyDamage(u_int oid);                                     // ROR 0x00163a44
    bool isEmyState2(u_int oid);                                         // ROR 0x00163a8c
    /* Its two oids (+0x138 / +0x13c). */
    void getOids(int *a, int *b);                                        // ROR 0x00163ab8
    void setEmyDownFlg(u_int oid, int v);                                // ROR 0x00163acc
    int  getEmyDamage(u_int oid);                                        // ROR 0x00163af8
    int  getEmyWord3c(u_int oid);                                        // ROR 0x00163b20
    bool setEmyWord238(u_int oid, int v);                                // ROR 0x00163b48
    /* GrgMain::testView, 1 when there is no enemy. */
    int  testEmyView(u_int oid);                                         // ROR 0x00163b80
    void releaseEmyHold(u_int oid);                                      // ROR 0x00163bb8
    int  getEmyType(u_int oid);                                          // ROR 0x00163be4
    void setEmyWord18(u_int oid);                                        // ROR 0x00163c24
    void clearEmyWord18(u_int oid);                                      // ROR 0x00163c50
    /* Types 0..3: the SE id of kind (0..6, else 7) in the enemy's voice
     * column. */
    u_int getEmyVoiceSE(u_int oid, int kind);                            // ROR 0x00163c74
    int  getEmyWord1d8(u_int oid);                                       // ROR 0x00163d90
    int  getEmyTypeFlag(u_int oid);                                      // ROR 0x00163db8
    /* The enemy's achr tasks paused (on 1) or resumed and its move / turn
     * tasks with them. */
    void pauseEmy(u_int oid, int on);                                    // ROR 0x00163e10
    void stopEmyMove(u_int oid);                                         // ROR 0x00163e48
    void setEmyMode6(u_int oid);                                         // ROR 0x00163e74
    u_int getEmySE(u_int oid, int no);                                   // ROR 0x00163ea0
    void subEmyFreeCnt(u_int oid, int n);                                // ROR 0x00163ee4
    int  getEmyWord238(u_int oid);                                       // ROR 0x00163f1c
    /* The enemy's word +0x24c while its type is 10 (NorMain), else 0. */
    int  getEmyHoldWord24c(u_int oid);                                   // ROR 0x00163f44
    /* The enemy's word +0x23c while its type is 6 (HfmMain), else 0. */
    int  getEmyHoldWord23c(u_int oid);                                   // ROR 0x00163f9c
    /* The enemy's word +0x248 while its type is 8 (ClrMain), else 0. */
    int  getEmyHoldWord248(u_int oid);                                   // ROR 0x00163ff4
    void resetEmyOut(u_int oid);                                         // ROR 0x0016404c
    bool isEmyFlag30(u_int oid);                                         // ROR 0x00164078
    void setEmyWork(u_int oid, int v);                                   // ROR 0x001640ac
    int  countEmyWork();                                                 // ROR 0x001640e8

    void SetGrpMotion(int no, int motion) { m_grp[no].m_motion = motion; } // ROR 0x0028a704
    int  GetGrpMotion(int no) { return m_grp[no].m_motion; }             // ROR 0x0028a71c
    int  RefLevel() { return m_level; }                                  // ROR 0x0028a748

    EmyMain *m_emy[EMY_MAX];            // ROR 0x004
    u_int    m_max_num[EMY_TYPE_MAX];   // ROR 0x104: enemies of a type at once (0 = any); the constructor fills bytes of 100
    u_char   m_oid_on[2];               // ROR 0x134: setOid / clearOid
    u_int    m_oid[2];                  // ROR 0x138: getOids (GamMan +0x22004 / +0x22008)
    int      m_level;                   // ROR 0x140: RefLevel
    int      m_oid_count;               // ROR 0x144: oids createEmy made (at most 16)
    /* PC: setGroup accepts a = 8, whose group the ROM writes over +0x328..
     * (m_work); the host keeps a ninth group so it stays in bounds. */
    EmyGroup m_grp[EMY_GROUP_MAX + 1];  // ROR 0x148
    int      m_work[EMY_MAX];           // ROR 0x330: per enemy slot, setEmyWork / countEmyWork
};
static_assert(offsetof(EmyManager, m_emy) >= 0x004, "m_emy");
static_assert(offsetof(EmyManager, m_grp) >= 0x148, "m_grp");

extern EmyManager g_emyManager; /* bss 0x7b2140 */

#endif /* _EMY_EMY_MANAGER_H */
