/* ==========================================================================
 *  emy/emy_main.h
 *
 *  EmyMain, the base of the eleven enemy classes (DwfMain .. NorMain, their
 *  files not decided yet: lib/unassigned.h), and EmyOut, the base of their
 *  per-state behaviours. RoR's own source string "emy/emy_main.cpp" anchors
 *  0x164364 / 0x164508; EmyMain's constructor / destructor 0x164118 /
 *  0x164208 sit just before them, its other methods in unassigned@00164fc0.
 *  The ELF's linkonce accessors name most fields (EmyMain::GetOid ...).
 * ======================================================================== */
#ifndef _EMY_EMY_MAIN_H
#define _EMY_EMY_MAIN_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "game/gamstat.h"

struct _tagEnemyTransition;     /* the state transition table EmyMain is built with; layout not read yet */
struct TskEnemyMove;
struct TskEnemyTurn;

/* The enemy's parameter block (GamMan::setEmyParam's word, script data).
 * Only the start state is read so far; the rest is EmyMain::loadParam's. */
struct _tagEnemyParam
{
    u_char _00[0x10];
    int    m_start_state;       // ROR 0x10: createEmy's state for setStartState
};
static_assert(offsetof(_tagEnemyParam, m_start_state) == 0x10, "m_start_state");

/* An enemy's behaviour for one state (EE: data to 0x34, vptr at +0x34,
 * vtable 0x2e3220; constructor 0x166a1c, destructor 0x166a74).
 * layout: host -- polymorphic; EE offsets in the comments. Only the words
 * EmyManager reads are placed. */
class EmyOut
{
public:
    virtual ~EmyOut();                                                   // EE slot 1 (+0x08), ROR 0x00166a74

    u_int m_38;                 // ROR 0x38: an oid its state 0xc works on (EmyManager::isEmyTarget)
    u_int m_3c;                 // ROR 0x3c: an oid its state 0xb works on
};

/* The enemy base (vtable 0x2e31e0, vptr at +0x200 after EmyMain's data).
 * One per enemy, newed by EmyManager::createEmy as the class of its type.
 * layout: host -- polymorphic; EE offsets in the comments. */
class EmyMain
{
public:
    EmyMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,
            const _tagEnemyTransition *trans, float *pos);              // ROR 0x00164118
    virtual ~EmyMain();                                                  // EE slot 1 (+0x08), ROR 0x00164208
    /* Picks the out of the current state (m_emy_out) and starts it;
     * createEmy calls it once the enemy is set up. */
    virtual void selectOut() = 0;                                        // EE slot 2 (+0x10)
    /* The enemy type, createEmy's switch value: 0 Dwf, 1 Pig, 2 Got, 3 Fsh,
     * 4 Bds, 5 Bdl, 6 Hfm, 7 Mrt, 8 Clr, 9 Grg, 10 Nor. */
    virtual int  GetType() = 0;                                          // EE slot 3 (+0x18)
    /* updateEmys, every frame while managed and not damaged: slots 4, 5, 6
     * in this order. Slot 4 picks the next state. */
    virtual void decide() = 0;                                           // EE slot 4 (+0x20)
    virtual void exec();                                                 // EE slot 5 (+0x28), ROR 0x00164fc0
    virtual void after() = 0;                                            // EE slot 6 (+0x30)

    /* createEmy: the start state and the transition table. */
    void setStartState(int state, const _tagEnemyTransition *trans);    // ROR 0x00164364
    void loadParam(const _tagEnemyParam *prm);                           // ROR 0x00164508
    /* The out's per-frame step (0x168520) when there is one. */
    void updateOut();                                                    // ROR 0x00165038
    /* The out's +0x1c = 0 when there is one (while damaged). */
    void stopOut();                                                      // ROR 0x00165060
    /* The group appear / respawn steps by m_grp_state; f, motion and pos
     * are the group's (EmyManager::m_grp). */
    int  updateGroup(float f, int motion, const float *pos);             // ROR 0x00165074
    bool testHittable();                                                 // ROR 0x001656b4
    bool testStateFlag();                                                // ROR 0x00165710
    /* m_notify |= bits; 1, 2, 4 and 8 also set m_damage. */
    void notify(int bits);                                               // ROR 0x001661dc
    /* The item the enemy carries (param 0x66; 0xa00 none) as an object. */
    void createHeldItem();                                               // ROR 0x0016621c
    void setGroup(int no);                                               // ROR 0x00166364
    void setManageSw(int on);                                            // ROR 0x00166378
    /* Calls the member-function pointer at +0x1dc. */
    void callFunc();                                                     // ROR 0x00166434
    void pauseMove(int on);                                              // ROR 0x001664a4
    void resetOut();                                                     // ROR 0x00166518
    /* m_damage_end - 1; at 0 the param loses flag 0x400. */
    void countDamageEnd();                                               // ROR 0x00166534
    u_int getSE(int no);                                                 // ROR 0x001665b8
    bool testNotifyHit();                                                // ROR 0x00166a0c

    /* The ELF's linkonce accessors (inline in the class body). */
    u_int    GetOid() { return m_oid; }                                  // ROR 0x0028a86c
    int      GetManageSw() { return m_manage_sw; }                       // ROR 0x0028a864
    ACHRstat *GetParamAddr() { return m_param; }                         // ROR 0x0028a874
    EmyOut  *GetEmyOut() { return m_emy_out; }                           // ROR 0x0028a87c
    int      GetDamageEnd() { return m_damage_end; }                     // ROR 0x0028a89c
    void     SetDamageEnd() { m_damage_end = 6; }                        // ROR 0x0028a8a4
    int      RefLP() { return m_lp; }                                    // ROR 0x0028a8e0
    int      GetGrpNo() { return m_grp_no; }                             // ROR 0x0028a8f0
    u_char   GetVoice() { return m_voice; }                              // ROR 0x0028a900
    void     InactiveMove();                                             // ROR 0x0028a93c
    int      GetState() { return m_state; }                              // ROR 0x0028aa00
    void     ForceSetState(int state) { m_state = state; }               // ROR 0x0028aa08
    int      GetDamage() { return m_damage; }                            // ROR 0x0028aa10
    void     SetDownSupine() { m_down_supine = 1; }                      // ROR 0x0028aa18
    void     SetDownProne() { m_down_supine = 0; }                       // ROR 0x0028aa24
    int      IsVisible() { return m_visible; }                           // ROR 0x0028aa34
    bool     IsDamaging() { return (m_param->m_flags & 0x200) != 0; }    // ROR 0x0028aa3c
    void     CancelDamage() { m_param->m_flags &= ~0x400u; }             // ROR 0x0028aa50
    int      GetState2() { return m_state2; }                            // ROR 0x0028aa70
    void     SetDownFlg(int v) { m_down_flg = v; }                       // ROR 0x0028aa78

    u_int         m_oid;            // ROR 0x00
    u_short       m_name;           // ROR 0x04: GetName (the system data kind)
    u_char        m_parsonality;    // ROR 0x06: GetParsonality
    u_char        m_voice;          // ROR 0x07: GetVoice (EmyManager::getEmyVoiceSE's column)
    int           m_manage_sw;      // ROR 0x08: updateEmys runs it while non-zero
    int           m_visible;        // ROR 0x0c: change_visible / IsVisible
    int           m_down_flg;       // ROR 0x10: SetDownFlg
    int           m_damage;         // ROR 0x14: GetDamage
    int           m_down_supine;    // ROR 0x18: SetDownSupine / SetDownProne
    ACHRstat     *m_param;          // ROR 0x20: GetParamAddr (its achr)
    u_int         m_notify;         // ROR 0x30: notify's bits (1 hit, 2 killed ...)
    int           m_state;          // ROR 0x3c: GetState
    EmyOut       *m_emy_out;        // ROR 0x44: GetEmyOut (the current state's out)
    int           m_state2;         // ROR 0x150: GetState2 / SetState2
    int           m_grp_no;         // ROR 0x1c8: GetGrpNo (0 = no group; GetGroupPtr points here)
    int           m_grp_state;      // ROR 0x1cc: 0 idle, 1 waiting, 3 out, 4 respawning, 5 gone (updateGroup)
    short         m_grp_wait;       // ROR 0x1d0: frames until it appears
    short         m_grp_1d2;        // ROR 0x1d2: updateGroup's respawn frame count
    int           m_grp_respawn;    // ROR 0x1d4: the group's respawn number it waits for
    int           m_lp;             // ROR 0x1d8: RefLP
    int           m_damage_end;     // ROR 0x1e4: GetDamageEnd / SetDamageEnd (6)
    TskEnemyMove *m_move;           // ROR 0x1f0: StartMove / ActiveMove / InactiveMove
    TskEnemyTurn *m_turn;           // ROR 0x1f8: StartTurn / ActiveTurn / InactiveTurn
    /* PC: a word of the derived classes (EmyMain's EE data ends at 0x200);
     * EmyManager::setEmyWord238 / getEmyWord238 use it on any enemy, so
     * the host keeps it in the base. */
    int           m_238;            // ROR 0x238
};
static_assert(offsetof(EmyMain, m_param) >= 0x20, "m_param");

#endif /* _EMY_EMY_MAIN_H */
