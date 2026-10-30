/* ==========================================================================
 *  emy/emy_dwf.h
 *
 *  emy/emy_dwf.cpp (0x15c2a4..0x16265c): DwfMain, enemy type 0, and the
 *  outs DwfOut*, which the Pig (PigMain::initOut) and the Got
 *  (GotMain::initOut) build too. No source string names the file; it is
 *  the run from DwfMain's destructor (Dwf, like Fsh, puts it before its
 *  initOut) up to EmyManager, named like emy_pig.cpp. The ELF's linkonce
 *  symbols name the classes, their constructors and GetType.
 *
 *  The Dwf is the Pig's elder twin: the same decide, tests and finders
 *  with its own part kinds (0xbbf..0xbc3) and fifteen states (m_out
 *  index, initOut): 1 Disappear, 2 Appear, 3 AtkNear, 4 Dead, 5 Stared,
 *  6 Win, 7 Serch, 8 Walk, 9 Wait, 10 AtkFar, 11 Pickup, 12 Parts, 13
 *  Fort, 14 Irritate, 15 Test. Decide's inputs are the Got's (1..21,
 *  emy/emy_got.h). Every input is dropped when it would lead to state 11
 *  with no pickup item (m_pickup), to state 12 with no part (m_part), or
 *  to state 14 with attr bits 8 and 0x10 both set.
 *
 *  The outs are shared by types 0..2, so they test the type (GetType)
 *  where the Got differs (its scissors, kind 0xa2e, have own motions).
 * ======================================================================== */
#ifndef _EMY_EMY_DWF_H
#define _EMY_EMY_DWF_H

#include "sdk/eetypes.h"
#include "emy/emy_main.h"

/* Type 0 (vtable 0x2e2e90, vptr at +0x200, EE 0x274). layout: host --
 * polymorphic; EE offsets in the comments. */
class DwfMain : public EmyMain
{
public:
    DwfMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x00289810
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~DwfMain();                                                          // EE slot 1 (+0x08), ROR 0x0015c2a4
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x0015cdac
    int  GetType() { return 0; }                                         // EE slot 3 (+0x18), ROR 0x00289848
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x0015c710
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x0015cc84
    void after();                                                        // EE slot 6 (+0x30), ROR 0x0015cd30
    /* The outs, the motion list, the state flags, the collision shape 1,
     * a random parsonality 0..2 and voice 0..3, the two targets. */
    void initOut();                                                      // ROR 0x0015c304
    bool isPCSameArea();                                                 // ROR 0x0015ce74
    int  testBarkReact();                                                // ROR 0x0015ce88
    /* The PC or the dog within param 0x5f in the facing's half circle;
     * with attr bit 1 or in state 1 all round, within the smaller of
     * param 0x5f and the PC state class's range. */
    bool isTargetInSight();                                              // ROR 0x0015cf30
    /* The tests below return 0 when the input's next state is blocked
     * (isBlocked), else EmyMain's test or the attr bits. */
    int  isPCStateClass1();                                              // ROR 0x0015d08c
    int  isPCAhead();                                                    // ROR 0x0015d134
    int  isPCStateClass2();                                              // ROR 0x0015d1dc
    int  isPCClass7OffAxis();                                            // ROR 0x0015d284
    int  isPCNear();                                                     // ROR 0x0015d32c
    int  isPCOrDogInSight();                                             // ROR 0x0015d3e0
    int  hasOid0Twice();                                                 // ROR 0x0015d488
    int  isLowLife();                                                    // ROR 0x0015d530
    int  isHalfLife();                                                   // ROR 0x0015d5d8
    int  testAttr8And10();                                               // ROR 0x0015d680
    int  testAttr2And4();                                                // ROR 0x0015d738
    int  testAttr1();                                                    // ROR 0x0015d7f0
    int  testAttr2Or4();                                                 // ROR 0x0015d890
    int  testAttr8Or10();                                                // ROR 0x0015d948
    int  isPCStateClass4();                                              // ROR 0x0015da00
    /* Inline in every test and in decide: the input's next state (peek) is
     * 11 with no pickup item, 12 with no part, or 14 with attr bits 8 and
     * 0x10. */
    bool isBlocked(int input);

    /* selectOut: the current state's row has the input (decide reads 1..21);
     * EE 0x204.., as many words as the table has inputs. PC: EE 0x238
     * (EmyMain::m_238 on the host) is m_input[13] here. */
    int     m_input[24];        // ROR 0x204
    u_char  m_264;              // ROR 0x264: cleared at initOut and on a state change, not read here
    u_char  m_265;              // ROR 0x265: cleared at initOut and on a state change, not read here
    u_char  m_seen;             // ROR 0x266: the PC or the dog in sight (decide's edge)
    int     m_268;              // ROR 0x268: not written by DwfMain
    u_int   m_pickup;           // ROR 0x26c: dwfFindPickupItem (state 11's gate)
    u_int   m_part;             // ROR 0x270: dwfFindPart (state 12's gate)
};

/* The outs (EmyOut: vptr at +0x34, own data from +0x38; each vtable's
 * slots are Init, Exec, Term). EE sizes from DwfMain::initOut's
 * operator new. */

/* State 1 (vtable 0x2e2ed0, EE 0x38): hidden, invisible and out of the
 * secta lists until the state ends. layout: host -- polymorphic. */
class DwfOutDisappear : public EmyOut
{
public:
    DwfOutDisappear(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289990
    ~DwfOutDisappear() {}                                                // EE slot 1, ROR 0x002898e0
    void Init();                                                         // EE slot 2, ROR 0x0015e214
    int  Exec() { return -1; }                                           // EE slot 3, ROR 0x002899c0
    void Term();                                                         // EE slot 4, ROR 0x0015e2a0
};

/* State 2 (vtable 0x2e2f00, EE 0x38): shown, the group's appear motion;
 * input 22 when it ends. layout: host -- polymorphic. */
class DwfOutAppear : public EmyOut
{
public:
    DwfOutAppear(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289a78
    ~DwfOutAppear() {}                                                   // EE slot 1, ROR 0x002899c8
    void Init();                                                         // EE slot 2, ROR 0x0015e324
    int  Exec();                                                         // EE slot 3, ROR 0x0015e344
    void Term() {}                                                       // EE slot 4, ROR 0x00289aa8
};

/* State 9 (vtable 0x2e2f30, EE 0x38): the walk motion in place, for
 * ever. layout: host -- polymorphic. */
class DwfOutWait : public EmyOut
{
public:
    DwfOutWait(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289b60
    ~DwfOutWait() {}                                                     // EE slot 1, ROR 0x00289ab0
    void Init();                                                         // EE slot 2, ROR 0x0015e4b0
    int  Exec();                                                         // EE slot 3, ROR 0x0015e514
    void Term() {}                                                       // EE slot 4, ROR 0x00289b90
};

/* State 8 (vtable 0x2e2f60, EE 0x50): wanders around its home (param
 * 0x59) by +-500, idling between walks; never ends by itself.
 * layout: host -- polymorphic. */
class DwfOutWalk : public EmyOut
{
public:
    DwfOutWalk(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289c58
    ~DwfOutWalk() {}                                                     // EE slot 1, ROR 0x00289b98
    void Init();                                                         // EE slot 2, ROR 0x0015e51c
    int  Exec();                                                         // EE slot 3, ROR 0x0015e5bc
    void Term();                                                         // EE slot 4, ROR 0x0015ea8c
    /* A new step restarts m_count. */
    void setStep(int step);                                              // ROR 0x0015eaa8

    int m_kind;                 // ROR 0x38: the enemy's type (GetType) at Init
    int m_count;                // ROR 0x3c: frames in the step
    int _40[4];                 // ROR 0x40: not used
};

/* State 7 (vtable 0x2e2f90, EE 0x38): chases its target until it sees
 * it within 200 or the chase ends; input 22. layout: host -- polymorphic. */
class DwfOutSerch : public EmyOut
{
public:
    DwfOutSerch(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289d38
    ~DwfOutSerch() {}                                                    // EE slot 1, ROR 0x00289c88
    void Init();                                                         // EE slot 2, ROR 0x0015eac4
    int  Exec();                                                         // EE slot 3, ROR 0x0015eb00
    void Term();                                                         // EE slot 4, ROR 0x0015eba8
};

/* State 11 (vtable 0x2e2fc0, EE 0x40): with free hands goes to the
 * nearest pickup item and takes it (input 22), else plays the idle
 * motion (input 23). layout: host -- polymorphic. */
class DwfOutPickup : public EmyOut
{
public:
    DwfOutPickup(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289e18
    ~DwfOutPickup() {}                                                   // EE slot 1, ROR 0x00289d68
    void Init();                                                         // EE slot 2, ROR 0x0015ebc4
    int  Exec();                                                         // EE slot 3, ROR 0x0015ec30
    void Term();                                                         // EE slot 4, ROR 0x0015ef78

    int m_kind;                 // ROR 0x38: the enemy's type (GetType) at Init
    /* The item it goes for: EE 0x3c, EmyOut::m_3c on the host (EmyManager::
     * isEmyTarget reads it in state 11). */
};

/* (vtable 0x2e2ff0, EE 0x84) layout: host -- polymorphic. */
class DwfOutAtkNear : public EmyOut
{
public:
    DwfOutAtkNear(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289f00
    ~DwfOutAtkNear() {}                                                  // EE slot 1, ROR 0x00289e50
    void Init();                                                         // EE slot 2, ROR 0x0015efb0
    int  Exec();                                                         // EE slot 3, ROR 0x0015f2fc
    void Term();                                                         // EE slot 4, ROR 0x0015ef94
};

/* State 10 (vtable 0x2e3020, EE 0x40): with an item in hand (and not
 * attr bits 8 and 0x10 both) chases the nearer target to 150 and throws
 * the item (TskAttack kind 0x15), input 22 once the throw task ends;
 * empty-handed input 23. layout: host -- polymorphic. */
class DwfOutAtkFar : public EmyOut
{
public:
    DwfOutAtkFar(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028a034
    ~DwfOutAtkFar() {}                                                   // EE slot 1, ROR 0x00289f84
    void Init();                                                         // EE slot 2, ROR 0x001606ec
    int  Exec();                                                         // EE slot 3, ROR 0x00160784
    void Term();                                                         // EE slot 4, ROR 0x0016097c
    /* The throw at m_target (step 1), or input 22 with no item. */
    void startThrow(int *ret);                                           // ROR 0x00160998

    int m_wait;                 // ROR 0x38: utlRandRange(90, 150) at Init, not read
    int m_tries;                // ROR 0x3c: idles in step 2 before a new chase (up to 3)
};

/* State 5 (vtable 0x2e3050, EE 0x48): turns to the dog, then stares
 * (with voice) while the dog barks and m_limit / 10 frames after, or
 * m_limit frames of barking; input 22. layout: host -- polymorphic. */
class DwfOutStared : public EmyOut
{
public:
    DwfOutStared(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028a114
    ~DwfOutStared() {}                                                   // EE slot 1, ROR 0x0028a064
    void Init();                                                         // EE slot 2, ROR 0x00160a44
    int  Exec();                                                         // EE slot 3, ROR 0x00160b08
    void Term();                                                         // EE slot 4, ROR 0x00160c9c

    u_short m_count;            // ROR 0x38: frames of the stare
    u_short m_limit;            // ROR 0x3a: 50, 100 or 150 (table 0x2f1068)
    int     m_kind;             // ROR 0x3c: the enemy's type (GetType) at Init
    int     m_motion;           // ROR 0x40: the type's stare motion (0x56, 0x4f, 0x53)
    int     m_quiet;            // ROR 0x44: the bark stopped (m_count restarted)
};

/* State 6 (vtable 0x2e3080, EE 0x40): the PC is down. Normally chases
 * the target to 100..200 and gloats (motion 0x15, looping SE 0xd).
 * With script global 0x173 set, the scripted win: motion 0x44 at a
 * fixed spot until global 0x174 or a hit, then motion 8 and the throw
 * of the held item (TskAttack kind 0x15), motion 0xd; out of the secta
 * lists throughout. layout: host -- polymorphic. */
class DwfOutWin : public EmyOut
{
public:
    DwfOutWin(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028a1f4
    ~DwfOutWin() {}                                                      // EE slot 1, ROR 0x0028a144
    void Init();                                                         // EE slot 2, ROR 0x00160cb8
    int  Exec();                                                         // EE slot 3, ROR 0x00160d50
    void Term();                                                         // EE slot 4, ROR 0x00161160
    void execChase();                                                    // ROR 0x00160d94
    void execScripted();                                                 // ROR 0x00160e44

    int   m_count;              // ROR 0x38: frames of motion 0x44 (SE 2 at 17)
    float m_range;              // ROR 0x3c: utlRandRangeF(100, 200)
};

/* State 12 (vtable 0x2e30b0, EE 0x3c): the idle motion by the attr
 * bits; never ends by itself. layout: host -- polymorphic. */
class DwfOutParts : public EmyOut
{
public:
    DwfOutParts(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028a2d4
    ~DwfOutParts() {}                                                    // EE slot 1, ROR 0x0028a224
    void Init();                                                         // EE slot 2, ROR 0x0016117c
    int  Exec();                                                         // EE slot 3, ROR 0x001611e0
    void Term();                                                         // EE slot 4, ROR 0x001611e8
};

/* State 15 (vtable 0x2e30e0, EE 0x38): motion 6, for ever.
 * layout: host -- polymorphic. */
class DwfOutTest : public EmyOut
{
public:
    DwfOutTest(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028a3bc
    ~DwfOutTest() {}                                                     // EE slot 1, ROR 0x0028a30c
    void Init();                                                         // EE slot 2, ROR 0x00161204
    int  Exec();                                                         // EE slot 3, ROR 0x00161228
    void Term() {}                                                       // EE slot 4, ROR 0x0028a3ec
};

/* (vtable 0x2e3110, EE 0x68) layout: host -- polymorphic. */
class DwfOutFort : public EmyOut
{
public:
    DwfOutFort(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028a4a4
    ~DwfOutFort() {}                                                     // EE slot 1, ROR 0x0028a3f4
    void Init();                                                         // EE slot 2, ROR 0x00161230
    int  Exec();                                                         // EE slot 3, ROR 0x00161300
    void Term();                                                         // EE slot 4, ROR 0x00161a90
};

/* State 14 (vtable 0x2e3140, EE 0x44): turns to the PC and keeps a
 * distance set by the PC's equipped item: closes in when farther than
 * the reach + 200, else idles or sidesteps (motions 2, 6, 0x10, 0x11);
 * input 23 once the PC sees it within the reach, or with attr bit 8 or
 * 0x10. layout: host -- polymorphic. */
class DwfOutIrritate : public EmyOut
{
public:
    DwfOutIrritate(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028a584
    ~DwfOutIrritate() {}                                                 // EE slot 1, ROR 0x0028a4d4
    void Init();                                                         // EE slot 2, ROR 0x00161b48
    int  Exec();                                                         // EE slot 3, ROR 0x00161bd4
    void Term();                                                         // EE slot 4, ROR 0x00161d8c
    /* The PC's weapon reach (by g_gamMan.m_equipItem) + 10. */
    float getReach();                                                    // ROR 0x00161da8
    /* The next move (steps 1 / 3, 1); 0 when the PC sees it. */
    int   pickMove();                                                    // ROR 0x00161f28

    int m_kind;                 // ROR 0x38: the enemy's type (GetType) at Init
    int m_motion;               // ROR 0x3c: the last idle motion
    int m_count;                // ROR 0x40: step 2's repeats (only pickMove's dead branch would set them)
};

/* (vtable 0x2e3170, EE 0x40) layout: host -- polymorphic. */
class DwfOutDead : public EmyOut
{
public:
    DwfOutDead(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028a664
    ~DwfOutDead() {}                                                     // EE slot 1, ROR 0x0028a5b4
    void Init();                                                         // EE slot 2, ROR 0x00162118
    int  Exec();                                                         // EE slot 3, ROR 0x00162394
    void Term();                                                         // EE slot 4, ROR 0x00162630
};

#endif /* _EMY_EMY_DWF_H */
