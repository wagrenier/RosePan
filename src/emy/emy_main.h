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

struct TskEnemyMove;
struct TskEnemyTurn;
struct GamIdMap;
class  EmyMain;

/* The state transition table an enemy is built with (createEmy's trans;
 * EmyMain::setStartState builds its EmyStateMap from it, the classes'
 * selectOut read the current state's row). A row per state, a column per
 * decide input; a byte is the state the input leads to, 0 none.
 * layout: host -- the pointer; EE offsets in the comments. */
struct _tagEnemyTransition
{
    u_char      m_states;       // ROR 0x00: rows
    u_char      m_inputs;       // ROR 0x01: columns
    const char *m_table;        // ROR 0x0c: m_states x m_inputs
};

/* A row of the kind move table (data 0x2f18b8, EE 0x18; EmyMain::getMoveParam):
 * emyStartMove / emyStartChase copy words 0..4 into the move record
 * (TskEnemyMove +0xa98 / +0xaa0 / +0xaa8 / +0xab0 / +0xab4). Roles not read yet. */
struct EmyKindMoveParam
{
    u_int m_w[6];               // ROR 0x00
};
static_assert(sizeof(EmyKindMoveParam) == 0x18, "EmyKindMoveParam");

/* The enemy's state graph (EE 0xa8, newed by setStartState; emy/emy_out.cpp):
 * a GamIdMap per state from the transition table and the current state.
 * layout: host -- the map pointers; EE offsets in the comments. */
struct EmyStateMap
{
    /* No maps; state current. */
    EmyStateMap(int state);                                              // ROR 0x00169538
    /* Deletes every state's map. */
    ~EmyStateMap();                                                      // ROR 0x0016957c
    /* The map of state (NULL none). */
    GamIdMap *getMap(int state);                                         // ROR 0x00169604
    /* map becomes state's; m_count + 1. */
    void addMap(int state, GamIdMap *map);                               // ROR 0x00169614
    /* The current state's map finds input: the state it leads to becomes
     * current and is returned; 0 when there is none. */
    int  next(int input);                                                // ROR 0x0016968c

    int       m_count;          // ROR 0x00: addMap's count
    GamIdMap *m_map[40];        // ROR 0x04: a map per state
    int       m_cur;            // ROR 0xa4: the current state
};

/* The enemy's parameter block (GamMan::setEmyParam's word, script data).
 * Only the start state is read so far; the rest is EmyMain::loadParam's. */
struct _tagEnemyParam
{
    u_char _00[0x10];
    int    m_start_state;       // ROR 0x10: createEmy's state for setStartState
};
static_assert(offsetof(_tagEnemyParam, m_start_state) == 0x10, "m_start_state");

/* An enemy's behaviour for one state (EE: data to 0x34, vptr at +0x34,
 * vtable 0x2e3220; constructor 0x166a1c, destructor 0x166a74; its methods
 * 0x166a1c.. are unassigned@00166a1c). The classes' outs (HfmOutWalk ...)
 * are built by XxxMain::initOut; the class's after() runs the current
 * one: Init when its state starts, Exec every frame (its result is the
 * next decide input, -1 none), Term when the state ends.
 * layout: host -- polymorphic; EE offsets in the comments. */
class EmyOut
{
public:
    EmyOut(u_int oid, ACHRstat *achr, EmyMain *emy);                    // ROR 0x00166a1c
    virtual ~EmyOut();                                                   // EE slot 1 (+0x08), ROR 0x00166a74
    virtual void Init() = 0;                                             // EE slot 2 (+0x10)
    virtual int  Exec() = 0;                                             // EE slot 3 (+0x18)
    virtual void Term() = 0;                                             // EE slot 4 (+0x20)

    /* The common Term: decide on, resetObj(0x400), the move task stopped,
     * the turn task off, the achr's tasks killed, motion input on, the
     * looping SE m_se stopped. */
    void termCommon();                                                   // ROR 0x00166ad0
    /* A node of the enemy's actor (x / z) at the object's height. */
    void getBasePos(float *pos);                                         // ROR 0x00166b74
    /* The target actor's node "Bip01 Spine1" (x, z) at the target
     * object's height. */
    void getTargetNodePos(float *pos);                                   // ROR 0x00166c8c
    /* getBasePos plus dist along the enemy actor's rot y. */
    void getFrontPos(float dist, float *pos);                            // ROR 0x00167d88
    /* |the angle from the target actor's facing to the enemy|, 0..pi:
     * pi / 2 and more means the enemy is behind it. */
    float getTargetAngle();                                              // ROR 0x00167e38
    /* m_target = the PC or (by sight within 60 and rate %) the dog;
     * m_target_dog says which. */
    void selectTarget(int rate);                                         // ROR 0x00166da4
    /* selectTarget(rate), or (100) the PC when its spine is nearer in x / z
     * than the dog's. */
    void selectNearTarget(int rate);                                     // ROR 0x00166eb4
    /* chrMtnMove2Achr at the emy's speed (m_agl / 60); flags 0 clears the
     * emy's motion input word. */
    void setMotion(int motion, int flags);                               // ROR 0x001680a0
    /* The walk motion: the motion list's [0], or [1] with achr +0x3ec bits
     * 8 and 0x10 both set. */
    void setWalkMotion();                                                // ROR 0x001682b0
    /* With no SE in m_se, counts m_30 down; below 0 it restarts at
     * utlRandRange(lo, hi) and plays SE se into m_se (1 then). */
    int  tickVoice(int lo, int hi, int se);                              // ROR 0x001693ec
    /* m_se stopped (-1), m_30 = utlRandRange(8, 24) again. */
    void stopVoice();                                                    // ROR 0x0016947c
    /* The emy's SE no (EmyMain::getSE) at getBasePos; the SE's handle. */
    int  playSE(int no);                                                 // ROR 0x001694cc

    /* The ELF's linkonce accessors. */
    u_int get_target_oid() { return m_target; }                          // ROR 0x0028aac8
    void  ClearBittenFlg() { m_bitten = 0; }                             // ROR 0x0028ab2c

    u_int     m_oid;            // ROR 0x00: the enemy's oid
    int       m_step;           // ROR 0x04: the out's own step
    ACHRstat *m_achr;           // ROR 0x08
    EmyMain  *m_emy;            // ROR 0x0c
    int       m_target_dog;     // ROR 0x10: selectTarget picked the dog
    u_int     m_target;         // ROR 0x14: get_target_oid
    int       m_bitten;         // ROR 0x1c: ClearBittenFlg
    int       m_20;             // ROR 0x20
    int       m_move_done;      // ROR 0x24: the move task's result (emyStartMove clears it; 2 ends emyChkChase)
    int       m_turn_done;      // ROR 0x28: the turn task's result (emyStartTurn clears it)
    int       m_se;             // ROR 0x2c: a looping SE, -1 none (termCommon stops it)
    int       m_30;             // ROR 0x30: utlRandRange(8, 24) at construction

    /* PC: words of the derived outs (EE data after the vptr); EmyManager
     * reads them on any out, so the host keeps them in the base. */
    u_int m_38;                 // ROR 0x38: an oid its state 0xc works on (EmyManager::isEmyTarget)
    u_int m_3c;                 // ROR 0x3c: an oid its state 0xb works on
};

/* Free helpers of the out code (unassigned@00166a1c). */
/* Fills the type's move record (data 0x2f1c90..) and starts the enemy's
 * TskEnemyMove to pos (range: the record's last word, which the task adds
 * to the radii); out->m_move_done is set when it arrives. */
void emyStartMove(float range, EmyOut *out, const float *pos);           // ROR 0x001670dc
/* Starts the enemy's TskEnemyTurn toward the PC (dog 0) or the dog;
 * out->m_turn_done is set when it is done. */
void emyStartTurn(float arg, EmyOut *out, int dog);                      // ROR 0x00167230
/* The chase's end test: 2 when target is gone or the move ended
 * (m_move_done 2), 1 when the enemy sees it (emyChkInSight); both stop the
 * move task. 0 otherwise. */
int  emyChkChase(float range, float angle, EmyOut *out, u_int target,
                 int useAngle);                                          // ROR 0x001679b4
/* a and b within r of each other in x / z. */
int  emyChkNearXZ(float r, const float *a, const float *b);              // ROR 0x00169a98
/* emyChkNearXZ and less than 100 apart in y. */
int  emyChkNearXZY(float r, const float *a, const float *b);             // ROR 0x00169b0c
/* oid's position; an achr's x / z from its Bip01 Spine1. */
void emyGetSEPos(u_int oid, float *pos);                                 // ROR 0x0016a594
/* The PC's / the dog's Bip01 Spine1 x / z, y 0. */
void emyGetPCPos(float *pos);                                            // ROR 0x0016a6b4
void emyGetDogPos(float *pos);                                           // ROR 0x0016a7ac
/* 1 when to is reachable from from over the secta cells (same cell, or
 * the cell path test), 0 when either is off the cells. */
int  emyCanReach(const float *from, const float *to);                    // ROR 0x0016976c
/* Hands the held item object item to the enemy oid. */
int  emyAttachItem(u_int oid, u_int item);                               // ROR 0x00169810
/* The enemy achr sees target: the two actors' nodes within range and,
 * with useAngle, within angle of the actor's facing. */
int  emyChkInSight(float range, float angle, int useAngle, u_int target,
                   ACHRstat *achr);                                      // ROR 0x00169bac
/* emyStartMove's record with a target oid: the TskEnemyMove follows
 * target until within range (the achr's tasks are killed first). */
void emyStartChase(float range, EmyOut *out, u_int target);              // ROR 0x00166f50
/* sqrt(x*x + y*y + z*z). */
float emyVecLength(const float *v);                                      // ROR 0x00169a14
/* One of the first nodes "p_hit" .. (table 0x298de8; fewer than 9) of
 * oid's actor is within range + target's radius of target's node. */
int  emyChkHitReach(float range, u_int oid, int nodes, u_int target);    // ROR 0x00169f94

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
    /* The four sound records at +0x154 (EE 0x1c each): each playing one is
     * stopped (funcSE, or funcStr from kind 3) and cleared. */
    void stopAllSE();                                                    // ROR 0x00165570
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
    /* The move record of the enemy object's kind (system data field 0x20
     * picks a row of the table at 0x2f18b8). */
    const EmyKindMoveParam *getMoveParam();                              // ROR 0x001669a4
    bool testNotifyHit();                                                // ROR 0x00166a0c
    /* Param 8 (the area) of the PC (GamMan::m_pcPtr) is the enemy's; 0 without a PC. */
    bool isPCSameArea();                                                 // ROR 0x00165734
    /* The achr's a_lp is under a quarter of m_lp. */
    bool isLowLife();                                                    // ROR 0x00166108
    /* m_decide_sw: the classes' decide does nothing while it is 0. */
    void setDecideSw(int on);                                            // ROR 0x0016649c

    /* The ELF's linkonce accessors (inline in the class body). */
    u_int    GetOid() { return m_oid; }                                  // ROR 0x0028a86c
    int      GetManageSw() { return m_manage_sw; }                       // ROR 0x0028a864
    ACHRstat *GetParamAddr() { return m_param; }                         // ROR 0x0028a874
    EmyOut  *GetEmyOut() { return m_emy_out; }                           // ROR 0x0028a87c
    int      GetMotionList(int no) { return m_motion_list[no]; }         // ROR 0x0028a8b0
    void     SetMotionList(int no, int motion) { m_motion_list[no] = motion; } // ROR 0x0028a8c0
    void     SetInputFlgMotion(int v) { m_input_flg_motion = v; }        // ROR 0x0028a8d0
    void     set_agl(float agl) { m_agl = agl; }                         // ROR 0x0028a85c
    float    RefAGL() { return m_agl; }                                  // ROR 0x0028a8d8
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
    EmyStateMap  *m_map;            // ROR 0x1c: setStartState's state graph
    ACHRstat     *m_param;          // ROR 0x20: GetParamAddr (its achr)
    int           m_req;            // ROR 0x24: the decide input to apply, -1 none
    int           m_decide_sw;      // ROR 0x28: setDecideSw (1 at construction)
    int           m_input_flg_motion; // ROR 0x2c: SetInputFlgMotion; 1 when the out's motion ended (decide tests & 0x11)
    u_int         m_notify;         // ROR 0x30: notify's bits (1 hit, 2 killed ...)
    int           m_state;          // ROR 0x3c: GetState
    int           m_changed;        // ROR 0x40: exec changed the state (after() then switches outs)
    EmyOut       *m_emy_out;        // ROR 0x44: GetEmyOut (the current state's out)
    EmyOut       *m_out[40];        // ROR 0x48: the out of each state (initOut)
    const _tagEnemyTransition *m_trans; // ROR 0xe8
    int           m_motion_list[15];  // ROR 0xec: GetMotionList / SetMotionList (initOut fills them per class)
    u_char        m_motion_flg[16];   // ROR 0x128: a byte per motion-list entry (initOut sets them; role not read)
    int           m_state2;         // ROR 0x150: GetState2 / SetState2
    float         m_agl;            // ROR 0x1c4: set_agl / RefAGL (the motion speed x 60; 0 = 1)
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
