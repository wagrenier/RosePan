/* ==========================================================================
 *  emy/emy_pig.h
 *
 *  emy/emy_pig.cpp (0x15a17c..0x15c2a4): PigMain, enemy type 1, and its
 *  own outs PigOutParts, PigOutTackle and PigOutTest. No source string
 *  names the file; it is the run from PigMain::initOut to DwfMain's
 *  destructor (every enemy file opens with its initOut, Dwf with its
 *  destructor), named like emy_got.cpp. The ELF's linkonce symbols name
 *  the classes, their constructors, GetType and PigOutTest::Term.
 *
 *  The Pig is the Got's twin (same decide, tests and finders) with its
 *  own part kinds, a tackle and a sixteenth state. Its states (m_out
 *  index, initOut): 1 DwfOutDisappear, 2 Appear, 3 AtkNear, 4 Dead, 5
 *  Stared, 6 Win, 7 Serch, 8 Walk, 9 Wait, 10 AtkFar, 11 Pickup, 12
 *  PigOutParts, 13 PigOutTackle, 14 DwfOutFort, 15 Irritate, 16
 *  PigOutTest. Decide's inputs are the Got's (1..21, emy/emy_got.h); the
 *  tackle returns inputs 22 (done) and 23 (the chase failed). Every input
 *  is dropped when it would lead to state 11 with no pickup item
 *  (m_pickup) or to state 12 with no part (m_part).
 * ======================================================================== */
#ifndef _EMY_EMY_PIG_H
#define _EMY_EMY_PIG_H

#include "sdk/eetypes.h"
#include "emy/emy_main.h"

/* Type 1 (vtable 0x2e2dc0, vptr at +0x200, EE 0x270). layout: host --
 * polymorphic; EE offsets in the comments. */
class PigMain : public EmyMain
{
public:
    PigMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x00289470
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~PigMain();                                                          // EE slot 1 (+0x08), ROR 0x0015a580
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x0015abc4
    int  GetType() { return 1; }                                         // EE slot 3 (+0x18), ROR 0x002894a8
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x0015a5e0
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x0015aa8c
    void after();                                                        // EE slot 6 (+0x30), ROR 0x0015ab48
    /* The outs, the motion list, the state flags, the two targets, a
     * random parsonality 0..2. */
    void initOut();                                                      // ROR 0x0015a17c
    bool isPCSameArea();                                                 // ROR 0x0015ac8c
    int  testBarkReact();                                                // ROR 0x0015aca0
    /* GotMain::isTargetInSight's body. */
    bool isTargetInSight();                                              // ROR 0x0015ad18
    /* The tests below return 0 when the input's next state is blocked
     * (isBlocked), else EmyMain's test or the attr bits. */
    int  isPCStateClass1();                                              // ROR 0x0015ae78
    int  isPCAhead();                                                    // ROR 0x0015aef0
    int  isPCStateClass2();                                              // ROR 0x0015af68
    int  isPCClass7OffAxis();                                            // ROR 0x0015afe0
    int  isPCNear();                                                     // ROR 0x0015b058
    int  isPCOrDogInSight();                                             // ROR 0x0015b0dc
    int  hasOid0Twice();                                                 // ROR 0x0015b154
    int  isLowLife();                                                    // ROR 0x0015b1cc
    int  isHalfLife();                                                   // ROR 0x0015b244
    int  testAttr8And10();                                               // ROR 0x0015b2bc
    int  testAttr2And4();                                                // ROR 0x0015b34c
    int  testAttr1();                                                    // ROR 0x0015b3dc
    int  testAttr2Or4();                                                 // ROR 0x0015b454
    int  testAttr8Or10();                                                // ROR 0x0015b4e4
    int  isPCStateClass4();                                              // ROR 0x0015b574
    /* Inline in every test and in decide: the input's next state (peek) is
     * 11 with no pickup item or 12 with no part. */
    bool isBlocked(int input);

    /* selectOut: the current state's row has the input (decide reads 1..21);
     * EE 0x204.., as many words as the table has inputs (the tackle's 22
     * and 23 included). PC: EE 0x238 (EmyMain::m_238 on the host) is
     * m_input[13] here. */
    int     m_input[24];        // ROR 0x204
    u_char  m_264;              // ROR 0x264: cleared at initOut and on a state change, not read here
    u_char  m_265;              // ROR 0x265: cleared at initOut and on a state change, not read here
    u_char  m_seen;             // ROR 0x266: the PC or the dog in sight (decide's edge)
    u_int   m_pickup;           // ROR 0x268: pigFindPickupItem (state 11's gate)
    u_int   m_part;             // ROR 0x26c: pigFindPart (state 12's gate)
};

/* The Pig's own outs (EmyOut: vptr at +0x34, own data from +0x38; each
 * vtable's slots are Init, Exec, Term). layout: host -- polymorphic. */

/* State 12 (vtable 0x2e2e00, EE 0x3c): holds a part: motion 10 with attr
 * bits 8 and 0x10, 1 with one of them, else 0x14; never ends by itself.
 * layout: host -- polymorphic. */
class PigOutParts : public EmyOut
{
public:
    PigOutParts(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002895c0
    ~PigOutParts() {}                                                    // EE slot 1, ROR 0x00289510
    void Init();                                                         // EE slot 2, ROR 0x0015bd58
    int  Exec();                                                         // EE slot 3, ROR 0x0015bdbc
    void Term();                                                         // EE slot 4, ROR 0x0015bdc4
};

/* State 13 (vtable 0x2e2e30, EE 0x44): chases the nearer target to 300,
 * then charges (motion 0x4d, SE 0xb), tackles (0x4c, 28 or 42 frames
 * with the object flags of a hit body: the PC and the dog each hit once
 * within 50, scene request 0x18) and recovers (0x4e); input 22 when done,
 * 23 when the chase ends without reaching. layout: host -- polymorphic. */
class PigOutTackle : public EmyOut
{
public:
    PigOutTackle(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002896a8
    ~PigOutTackle() {}                                                   // EE slot 1, ROR 0x002895f8
    void Init();                                                         // EE slot 2, ROR 0x0015bde0
    int  Exec();                                                         // EE slot 3, ROR 0x0015be28
    void Term();                                                         // EE slot 4, ROR 0x0015c164

    int m_hit_pc;               // ROR 0x38: the tackle hit the PC
    int m_hit_dog;              // ROR 0x3c: the tackle hit the dog
    int m_frames;               // ROR 0x40: the tackle's frames left
};

/* State 16 (vtable 0x2e2e60, EE 0x38): motion 0x36 over and over (steps 2
 * and 3, motions 0x34 and 0x30, are never reached in the ROM either).
 * layout: host -- polymorphic. */
class PigOutTest : public EmyOut
{
public:
    PigOutTest(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289788
    ~PigOutTest() {}                                                     // EE slot 1, ROR 0x002896d8
    void Init();                                                         // EE slot 2, ROR 0x0015c180
    int  Exec();                                                         // EE slot 3, ROR 0x0015c188
    void Term() {}                                                       // EE slot 4, ROR 0x002897b8
};

#endif /* _EMY_EMY_PIG_H */
