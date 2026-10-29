/* ==========================================================================
 *  emy/emy_bdl.h
 *
 *  emy/emy_bdl.cpp (0x14b3bc..0x14cde4): BdlMain, enemy type 5, and its
 *  per-state outs BdlOut*. No source string names the file; it is the run
 *  from BdlMain::initOut to MrtMain::initOut (every enemy file opens with
 *  its initOut), named like emy_bds.cpp. The ELF's linkonce symbols name
 *  the classes, their constructors and GetType.
 *
 *  Bdl is BdsMain's twin (emy_bds.h): the same states, the same outs and
 *  mostly the same code. The differences: decide's input 1 is
 *  EmyMain::testBarkReact (Bds never takes it), Search ends within 300
 *  (Bds 200), Dead always plays motion 0x24, and BdlOutAtk turns to face
 *  the target from the side its mode (param 0x68) names before attacking.
 *
 *  BdlMain's states (m_out index, initOut): 1 Walk, 2 Search, 3 Atk,
 *  4 Win, 5 Stared, 6 Dead (exec forces it when killed). Its decide inputs:
 *  0 the PC left the area (once it had been in it), 1 the dog's bark
 *  reached it (testBarkReact), 2 hit (notify bit 1), 3 the PC came into
 *  sight, 4 lost from sight, 5 the PC's state class is 4. The outs hand
 *  back 4 (Search lost the target), 6 (Search reached it, Stared's bark
 *  ended) and 7 (Search's chase ended otherwise, Atk's turn failed).
 * ======================================================================== */
#ifndef _EMY_EMY_BDL_H
#define _EMY_EMY_BDL_H

#include "sdk/eetypes.h"
#include "emy/emy_main.h"

/* Type 5 (vtable 0x2e23e0, vptr at +0x200, EE 0x228). layout: host --
 * polymorphic; EE offsets in the comments. */
class BdlMain : public EmyMain
{
public:
    BdlMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x002867ec
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~BdlMain();                                                          // EE slot 1 (+0x08), ROR 0x0014b554
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x0014b7fc
    int  GetType() { return 5; }                                         // EE slot 3 (+0x18), ROR 0x00286824
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x0014b5b4
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x0014b6d8
    void after();                                                        // EE slot 6 (+0x30), ROR 0x0014b780
    /* The outs, the motion list and the state flags. */
    void initOut();                                                      // ROR 0x0014b3bc
    bool isPCSameArea();                                                 // ROR 0x0014b8c0
    int  testBarkReact();                                                // ROR 0x0014b8d4
    bool isPCStateClass4();                                              // ROR 0x0014b8e8
    int  isPCInSight();                                                  // ROR 0x0014b910

    u_char m_pc_seen;           // ROR 0x204: decide: the PC is in sight (inputs 3 / 4); exec clears it
    u_char m_in_area;           // ROR 0x205: decide: the PC was in the area (input 0); exec clears it
    int    m_input[8];          // ROR 0x208: selectOut: the current state's row has the input (decide)
};

/* The outs (EmyOut: vptr at +0x34, own data from +0x38; each vtable's slots
 * are Init, Exec, Term). The constructors are the ELF's linkonce
 * Xxx::Xxx(unsigned int, ACHRstat *, EmyMain *): EmyOut's and the vtable.
 * layout: host -- polymorphic. */

/* State 1 (vtable 0x2e2420, EE 0x4c): roams around its home point
 * (BdsOutWalk's code). layout: host -- polymorphic. */
class BdlOutWalk : public EmyOut
{
public:
    BdlOutWalk(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002868dc
    ~BdlOutWalk() {}                                                     // EE slot 1, ROR 0x0028682c
    void Init();                                                         // EE slot 2, ROR 0x0014b974
    int  Exec();                                                         // EE slot 3, ROR 0x0014b9a0
    void Term();                                                         // EE slot 4, ROR 0x0014beb4
    /* Home (param 0x59) +-500 at four scales, the first point
     * emyCanReach accepts, else home. */
    void getRoamPos(float *pos);                                         // ROR 0x0014baf8
    /* r < 86: walk motion 1 / 0x1c / 0x1d in place (step 2), else motion
     * 0x14 (step 3). */
    void startIdle(int r);                                               // ROR 0x0014bdbc
    /* chrMtnMove2Achr of motion at the emy's speed, flags 0x100. */
    void setWalkMotion(int motion);                                      // ROR 0x0014be48
    void setStep(int step);                                              // ROR 0x0014bed0

    int m_cnt;                  // ROR 0x38: frames in the step (EE 0x3c..0x4c unused)
};

/* State 5 (vtable 0x2e2450, EE 0x3c): faces the barking dog and cowers
 * while it barks. layout: host -- polymorphic. */
class BdlOutStared : public EmyOut
{
public:
    BdlOutStared(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002869bc
    ~BdlOutStared() {}                                                   // EE slot 1, ROR 0x0028690c
    void Init();                                                         // EE slot 2, ROR 0x0014beec
    int  Exec();                                                         // EE slot 3, ROR 0x0014bf1c
    void Term();                                                         // EE slot 4, ROR 0x0014c088
    /* The dog's direction from getBasePos less the actor's facing,
     * wrapped to -pi..pi, into *angle (when not NULL); 1 within
     * +-range / 2. */
    int  isDogAhead(float range, float *angle);                          // ROR 0x0014c0a4

    u_short m_cnt;              // ROR 0x38: frames of step 3
};

/* State 2 (vtable 0x2e2480, EE 0x38): chases the target. layout: host --
 * polymorphic. */
class BdlOutSearch : public EmyOut
{
public:
    BdlOutSearch(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00286a9c
    ~BdlOutSearch() {}                                                   // EE slot 1, ROR 0x002869ec
    void Init();                                                         // EE slot 2, ROR 0x0014c1f4
    int  Exec();                                                         // EE slot 3, ROR 0x0014c230
    void Term();                                                         // EE slot 4, ROR 0x0014c2e0
};

/* State 3 (vtable 0x2e24b0, EE 0x50): turns until the target lies in its
 * mode's sector, then pecks (motion 0x20) or attacks three times (motion
 * 0x21). layout: host -- polymorphic. */
class BdlOutAtk : public EmyOut
{
public:
    BdlOutAtk(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00286b7c
    ~BdlOutAtk() {}                                                      // EE slot 1, ROR 0x00286acc
    void Init();                                                         // EE slot 2, ROR 0x0014c2fc
    int  Exec();                                                         // EE slot 3, ROR 0x0014c370
    void Term();                                                         // EE slot 4, ROR 0x0014cbb8

    int m_cnt;                  // ROR 0x38: frames in the attack
    /* param 0x68: 1 attacks whatever it sees ahead; 2..5 only targets in
     * a fixed world-angle sector (Exec); others turn and attack. */
    int m_mode;                 // ROR 0x3c (EE 0x40..0x48 unused)
    int m_hit_pc;               // ROR 0x48: the PC was hit this attack
    int m_hit_dog;              // ROR 0x4c: the dog was hit
};

/* State 4 (vtable 0x2e24e0, EE 0x3c): motion 0x15 with a looping SE.
 * layout: host -- polymorphic. */
class BdlOutWin : public EmyOut
{
public:
    BdlOutWin(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00286cd4
    ~BdlOutWin() {}                                                      // EE slot 1, ROR 0x00286c24
    void Init();                                                         // EE slot 2, ROR 0x0014cbd4
    int  Exec();                                                         // EE slot 3, ROR 0x0014cc2c
    void Term();                                                         // EE slot 4, ROR 0x0014cc94

    /* Exec's chase range; nothing sets it, and Init's step 1 keeps Exec
     * from reading it. */
    float m_range;              // ROR 0x38
};

/* State 6 (vtable 0x2e2510, EE 0x38). layout: host -- polymorphic. */
class BdlOutDead : public EmyOut
{
public:
    BdlOutDead(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00286db4
    ~BdlOutDead() {}                                                     // EE slot 1, ROR 0x00286d04
    void Init();                                                         // EE slot 2, ROR 0x0014ccb0
    int  Exec();                                                         // EE slot 3, ROR 0x0014cdc0
    void Term();                                                         // EE slot 4, ROR 0x0014cdc8
};

#endif /* _EMY_EMY_BDL_H */
