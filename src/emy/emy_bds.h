/* ==========================================================================
 *  emy/emy_bds.h
 *
 *  emy/emy_bds.cpp (0x149d18..0x14b3bc): BdsMain, enemy type 4, and its
 *  per-state outs BdsOut*. No source string names the file; it is the run
 *  from BdsMain::initOut to BdlMain::initOut (every enemy file opens with
 *  its initOut), named like emy_hfm.cpp. The ELF's linkonce symbols name
 *  the classes, their constructors and GetType.
 *
 *  BdsMain's states (m_out index, initOut): 1 Walk, 2 Search, 3 Atk,
 *  4 Win, 5 Stared, 6 Dead (exec forces it when killed). Its decide inputs
 *  (the transition table's columns): 0 the PC left the area (once it had
 *  been in it), 1 never, 2 hit (notify bit 1), 3 the PC came into sight,
 *  4 lost from sight, 5 the PC's state class is 4. The outs hand back
 *  4 (Search lost the target), 6 (Search reached it, Stared's bark ended)
 *  and 7 (Search's chase ended otherwise).
 * ======================================================================== */
#ifndef _EMY_EMY_BDS_H
#define _EMY_EMY_BDS_H

#include "sdk/eetypes.h"
#include "emy/emy_main.h"

/* Type 4 (vtable 0x2e2280, vptr at +0x200, EE 0x228). layout: host --
 * polymorphic; EE offsets in the comments. */
class BdsMain : public EmyMain
{
public:
    BdsMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x0028621c
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~BdsMain();                                                          // EE slot 1 (+0x08), ROR 0x00149eb0
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x0014a158
    int  GetType() { return 4; }                                         // EE slot 3 (+0x18), ROR 0x00286254
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x00149f10
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x0014a034
    void after();                                                        // EE slot 6 (+0x30), ROR 0x0014a0dc
    /* The outs, the motion list and the state flags. */
    void initOut();                                                      // ROR 0x00149d18
    bool isPCSameArea();                                                 // ROR 0x0014a21c
    /* decide's input 1 test: always 0. */
    int  returnFalse();                                                  // ROR 0x0014a230
    bool isPCStateClass4();                                              // ROR 0x0014a240
    int  isPCInSight();                                                  // ROR 0x0014a268

    u_char m_pc_seen;           // ROR 0x204: decide: the PC is in sight (inputs 3 / 4); exec clears it
    u_char m_in_area;           // ROR 0x205: decide: the PC was in the area (input 0); exec clears it
    int    m_input[8];          // ROR 0x208: selectOut: the current state's row has the input (decide)
};

/* The outs (EmyOut: vptr at +0x34, own data from +0x38; each vtable's slots
 * are Init, Exec, Term). The constructors are the ELF's linkonce
 * Xxx::Xxx(unsigned int, ACHRstat *, EmyMain *): EmyOut's and the vtable.
 * layout: host -- polymorphic. */

/* State 1 (vtable 0x2e22c0, EE 0x4c): roams around its home point.
 * layout: host -- polymorphic. */
class BdsOutWalk : public EmyOut
{
public:
    BdsOutWalk(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028630c
    ~BdsOutWalk() {}                                                     // EE slot 1, ROR 0x0028625c
    void Init();                                                         // EE slot 2, ROR 0x0014a2cc
    int  Exec();                                                         // EE slot 3, ROR 0x0014a2f8
    void Term();                                                         // EE slot 4, ROR 0x0014a7f8
    /* HfmOutWalk::getRoamPos's code with a +-500 offset. */
    void getRoamPos(float *pos);                                         // ROR 0x0014a450
    /* r < 86: walk motion 1 / 0x1c / 0x1d in place (step 2), else motion
     * 0x14 (step 3). */
    void startIdle(int r);                                               // ROR 0x0014a700
    /* chrMtnMove2Achr of motion at the emy's speed, flags 0x100. */
    void setWalkMotion(int motion);                                      // ROR 0x0014a78c
    void setStep(int step);                                              // ROR 0x0014a814

    int m_cnt;                  // ROR 0x38: frames in the step (EE 0x3c..0x4c unused)
};

/* State 5 (vtable 0x2e22f0, EE 0x3c): faces the barking dog and cowers
 * while it barks. layout: host -- polymorphic. */
class BdsOutStared : public EmyOut
{
public:
    BdsOutStared(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002863ec
    ~BdsOutStared() {}                                                   // EE slot 1, ROR 0x0028633c
    void Init();                                                         // EE slot 2, ROR 0x0014a830
    int  Exec();                                                         // EE slot 3, ROR 0x0014a860
    void Term();                                                         // EE slot 4, ROR 0x0014a9cc
    /* The dog's direction from getBasePos less the actor's facing,
     * wrapped to -pi..pi, into *angle (when not NULL); 1 within
     * +-range / 2. */
    int  isDogAhead(float range, float *angle);                          // ROR 0x0014a9e8

    u_short m_cnt;              // ROR 0x38: frames of step 3
};

/* State 2 (vtable 0x2e2320, EE 0x38): chases the target. layout: host --
 * polymorphic. */
class BdsOutSearch : public EmyOut
{
public:
    BdsOutSearch(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002864cc
    ~BdsOutSearch() {}                                                   // EE slot 1, ROR 0x0028641c
    void Init();                                                         // EE slot 2, ROR 0x0014ab38
    int  Exec();                                                         // EE slot 3, ROR 0x0014ab74
    void Term();                                                         // EE slot 4, ROR 0x0014ac24
};

/* State 3 (vtable 0x2e2350, EE 0x44): chases to 75, then pecks
 * (motion 0x20, one hit window) or attacks three times (motion 0x21).
 * layout: host -- polymorphic. */
class BdsOutAtk : public EmyOut
{
public:
    BdsOutAtk(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002865ac
    ~BdsOutAtk() {}                                                      // EE slot 1, ROR 0x002864fc
    void Init();                                                         // EE slot 2, ROR 0x0014ac40
    int  Exec();                                                         // EE slot 3, ROR 0x0014ac98
    void Term();                                                         // EE slot 4, ROR 0x0014b180

    int m_cnt;                  // ROR 0x38: frames in the attack
    int m_hit_pc;               // ROR 0x3c: the PC was hit this attack
    int m_hit_dog;              // ROR 0x40: the dog was hit
};

/* State 4 (vtable 0x2e2380, EE 0x3c): motion 0x15 with a looping SE.
 * layout: host -- polymorphic. */
class BdsOutWin : public EmyOut
{
public:
    BdsOutWin(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028668c
    ~BdsOutWin() {}                                                      // EE slot 1, ROR 0x002865dc
    void Init();                                                         // EE slot 2, ROR 0x0014b19c
    int  Exec();                                                         // EE slot 3, ROR 0x0014b1f4
    void Term();                                                         // EE slot 4, ROR 0x0014b25c

    /* Exec's chase range; nothing sets it, and Init's step 1 keeps Exec
     * from reading it. */
    float m_range;              // ROR 0x38
};

/* State 6 (vtable 0x2e23b0, EE 0x38). layout: host -- polymorphic. */
class BdsOutDead : public EmyOut
{
public:
    BdsOutDead(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028676c
    ~BdsOutDead() {}                                                     // EE slot 1, ROR 0x002866bc
    void Init();                                                         // EE slot 2, ROR 0x0014b278
    int  Exec();                                                         // EE slot 3, ROR 0x0014b398
    void Term();                                                         // EE slot 4, ROR 0x0014b3a0
};

#endif /* _EMY_EMY_BDS_H */
