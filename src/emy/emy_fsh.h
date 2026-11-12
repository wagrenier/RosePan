/* ==========================================================================
 *  emy/emy_fsh.h
 *
 *  emy/emy_fsh.cpp (0x155d54..0x1583d4): FshMain, enemy type 3, and its
 *  per-state outs FshOut*. No source string names the file; it is the run
 *  from FshMain's destructor 0x155d54 (Fsh, like Dwf, puts its main
 *  virtuals before its initOut) to GotMain::initOut, named like
 *  emy_bds.cpp. The ELF's linkonce symbols name the classes, their
 *  constructors, GetType, FshOutDisappear::Exec and four empty Terms.
 *
 *  A crawling enemy that attacks from the floor: its collision is shape
 *  4, radius 10. States (m_out index, initOut): 1 Disappear (hidden,
 *  motion 0x16), 2 Appear (motion 0x17, then shown), 3 Atk (bites by a
 *  pose table, rising and lowering between poses), 4 Dead, 5 Stared (the
 *  dog barks at it), 6 Collision (bumped by the PC: a flinch by side),
 *  7 Win, 8 Serch and 9 Round (chases), 10 Walk (wanders round its
 *  spot), 11 Wait, 12 Test. Decide runs only while the motion input
 *  word is set; inputs: 1 the dog barks at it (testBarkReact), 2 hit
 *  (notify bit 1), 3 the PC touching it, 4 / 5 the PC or the dog
 *  came into / left sight, 6..10 while in sight the PC's class 1, the
 *  PC ahead, class 2, class 7 off axis, the PC near, 11 low life, 12
 *  half life, 13..17 the achr's attr bits (13 8 and 0x10, 14 2 and 4, 15
 *  1, 16 2 or 4, 17 8 or 0x10), 18 the PC's class 4; exec follows state 2
 *  by input 0x14 while visible. The outs hand back 0x13 (done) or 0x14
 *  (the chase failed).
 * ======================================================================== */
#ifndef _EMY_EMY_FSH_H
#define _EMY_EMY_FSH_H

#include "sdk/eetypes.h"
#include "emy/emy_main.h"

/* Type 3 (vtable 0x2e2aa0, vptr at +0x200, EE 0x25c). layout: host --
 * polymorphic; EE offsets in the comments. */
class FshMain : public EmyMain
{
public:
    FshMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x00288668
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~FshMain();                                                          // EE slot 1 (+0x08), ROR 0x00155d54
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x001561bc
    int  GetType() { return 3; }                                         // EE slot 3 (+0x18), ROR 0x002886a0
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x00155db4
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x00156048
    void after();                                                        // EE slot 6 (+0x30), ROR 0x00156140
    /* The outs, the motion list, the state flags, the collision (shape 4,
     * radius 10), a random parsonality 0..2. */
    void initOut();                                                      // ROR 0x00156620
    bool isPCSameArea();                                                 // ROR 0x00156284
    int  testBarkReact();                                                // ROR 0x00156298
    /* In the PC's area: emyChkInSight of the PC or the dog within pi;
     * with attr bit 0 or in state 1 without the angle test, over 60 (200
     * for the PC's classes 2, 3, 7, 100 for 6), else param 0x5f. */
    bool isTargetInSight();                                              // ROR 0x001562ac
    /* The PC's spine within both radii + 10 of getBasePos in x / z. */
    int  isPCTouching();                                                 // ROR 0x0015640c
    int  isPCStateClass1();                                              // ROR 0x001564c0
    int  isPCAhead();                                                    // ROR 0x001564d4
    int  isPCStateClass2();                                              // ROR 0x001564e8
    int  isPCClass7OffAxis();                                            // ROR 0x001564fc
    int  isPCNear();                                                     // ROR 0x00156510
    bool isLowLife();                                                    // ROR 0x00156524
    bool isHalfLife();                                                   // ROR 0x00156538
    /* The achr's attr word 0 bits. */
    int  testAttr8And10();                                               // ROR 0x0015654c
    int  testAttr2And4();                                                // ROR 0x00156578
    int  testAttr1();                                                    // ROR 0x001565a4
    int  testAttr2Or4();                                                 // ROR 0x001565b4
    int  testAttr8Or10();                                                // ROR 0x001565e0
    int  isPCStateClass4();                                              // ROR 0x0015660c

    /* selectOut: the current state's row has the input (decide reads 1..18,
     * exec 0x14); EE 0x204.., as many words as the table has inputs.
     * PC: EE 0x238 (EmyMain::m_238 on the host) is m_input[13] here. */
    int     m_input[21];        // ROR 0x204
    u_char  m_258;              // ROR 0x258: cleared at initOut and on a state change, not read here
    u_char  m_seen;             // ROR 0x259: the PC or the dog in sight (decide's edge)
};

/* The outs (EmyOut: vptr at +0x34, own data from +0x38; each vtable's slots
 * are Init, Exec, Term). The constructors are the ELF's linkonce
 * Xxx::Xxx(unsigned int, ACHRstat *, EmyMain *): EmyOut's and the vtable.
 * layout: host -- polymorphic. */

/* State 1 (vtable 0x2e2ae0, EE 0x38): hidden, motion 0x16, out of the
 * hit tests until Term. layout: host -- polymorphic. */
class FshOutDisappear : public EmyOut
{
public:
    FshOutDisappear(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288758
    ~FshOutDisappear() {}                                                // EE slot 1, ROR 0x002886a8
    void Init();                                                         // EE slot 2, ROR 0x0015694c
    int  Exec() { return -1; }                                           // EE slot 3, ROR 0x00288788
    void Term();                                                         // EE slot 4, ROR 0x001569d8
};

/* State 2 (vtable 0x2e2b10, EE 0x38): shown with motion 0x17; at its end
 * visible again, 0x13. layout: host -- polymorphic. */
class FshOutAppear : public EmyOut
{
public:
    FshOutAppear(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288840
    ~FshOutAppear() {}                                                   // EE slot 1, ROR 0x00288790
    void Init();                                                         // EE slot 2, ROR 0x00156a5c
    int  Exec();                                                         // EE slot 3, ROR 0x00156a7c
    void Term() {}                                                       // EE slot 4, ROR 0x00288870
};

/* State 3 (vtable 0x2e2b40, EE 0x5c): idles in a pose (0..3: motions
 * 0x2a / 0x28 / 0x2c / 0x2d, 0x2b / 0x29 attacks, 0x1e / 0x21 rests),
 * attacks at 30 %, and rises (0x23) or lowers (0x22) between pose 0 and
 * the others; the PC and the dog are hit up to twice each (chr word
 * 0x17). layout: host -- polymorphic. */
class FshOutAtk : public EmyOut
{
public:
    FshOutAtk(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288928
    ~FshOutAtk() {}                                                      // EE slot 1, ROR 0x00288878
    void Init();                                                         // EE slot 2, ROR 0x00156b48
    int  Exec();                                                         // EE slot 3, ROR 0x00156c00
    void Term();                                                         // EE slot 4, ROR 0x001570e0
    /* The PC (not in class 9) and the dog (not hurt) touching the body,
     * twice each at most: SE 0, scnChrRequest word 0x17. */
    void checkHit();                                                     // ROR 0x001570fc
    /* Four points along the enemy's facing (-30..30 from getBasePos)
     * within target's radius + 10 of its spine in x / z. */
    bool isTouching(int dog, u_int target);                              // ROR 0x00157230

    u_int   m_wait;             // ROR 0x38: frames of this pose
    u_int   m_count;            // ROR 0x3c
    u_short m_pose;             // ROR 0x40: 0..3, the tables' row
    u_short m_raised;           // ROR 0x42: 1 in poses 1..3 (motion 0x23 raised it)
    int     m_next_step;        // ROR 0x44: step 3's: the step after the rise / lowering
    int     m_next_motion;      // ROR 0x48: ... its motion
    int     m_next_flags;       // ROR 0x4c: ... its setMotion flags
    int     m_next_se;          // ROR 0x50: ... its SE (when none plays)
    int     m_hit_pc;           // ROR 0x54
    int     m_hit_dog;          // ROR 0x58
};

/* State 11 (vtable 0x2e2b70, EE 0x38): the walk motion, nothing else.
 * layout: host -- polymorphic. */
class FshOutWait : public EmyOut
{
public:
    FshOutWait(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288a08
    ~FshOutWait() {}                                                     // EE slot 1, ROR 0x00288958
    void Init();                                                         // EE slot 2, ROR 0x001574e4
    int  Exec();                                                         // EE slot 3, ROR 0x0015750c
    void Term() {}                                                       // EE slot 4, ROR 0x00288a38
};

/* State 10 (vtable 0x2e2ba0, EE 0x4c): moves to a point within 500 of
 * its spot (param 0x59) at least 100 away in x and z, or waits 20 frames.
 * layout: host -- polymorphic. */
class FshOutWalk : public EmyOut
{
public:
    FshOutWalk(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288b00
    ~FshOutWalk() {}                                                     // EE slot 1, ROR 0x00288a40
    void Init();                                                         // EE slot 2, ROR 0x00157514
    int  Exec();                                                         // EE slot 3, ROR 0x00157548
    void Term();                                                         // EE slot 4, ROR 0x001577dc
    /* A new step restarts m_count. */
    void setStep(int step);                                              // ROR 0x001577f8

    int   m_count;              // ROR 0x38: frames of the step
    float m_3c[4];              // ROR 0x3c: not used (the ROM keeps the point on the stack)
};

/* State 8 (vtable 0x2e2bd0, EE 0x38): chases the target to 10; 0x13 when
 * it sees it, 0x14 when the chase failed. layout: host -- polymorphic. */
class FshOutSerch : public EmyOut
{
public:
    FshOutSerch(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288be0
    ~FshOutSerch() {}                                                    // EE slot 1, ROR 0x00288b30
    void Init();                                                         // EE slot 2, ROR 0x00157814
    int  Exec();                                                         // EE slot 3, ROR 0x00157864
    void Term();                                                         // EE slot 4, ROR 0x001578c4
};

/* State 9 (vtable 0x2e2c00, EE 0x38): Serch's chase, ending within 30.
 * layout: host -- polymorphic. */
class FshOutRound : public EmyOut
{
public:
    FshOutRound(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288cc0
    ~FshOutRound() {}                                                    // EE slot 1, ROR 0x00288c10
    void Init();                                                         // EE slot 2, ROR 0x001578e0
    int  Exec();                                                         // EE slot 3, ROR 0x00157934
    void Term();                                                         // EE slot 4, ROR 0x001579b4
};

/* State 5 (vtable 0x2e2c30, EE 0x40): turns to the dog, then a stare
 * motion (0x1a, 0x19 or 0x2e) with its voice until the bark stops (or 100
 * frames). layout: host -- polymorphic. */
class FshOutStared : public EmyOut
{
public:
    FshOutStared(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288da0
    ~FshOutStared() {}                                                   // EE slot 1, ROR 0x00288cf0
    void Init();                                                         // EE slot 2, ROR 0x001579d0
    int  Exec();                                                         // EE slot 3, ROR 0x00157a30
    void Term();                                                         // EE slot 4, ROR 0x00157b74

    u_short m_count;            // ROR 0x38: frames of the stare
    u_short m_limit;            // ROR 0x3a: frames after the bark stops (table 0x2f0c78: always 0)
    int     m_quiet;            // ROR 0x3c: the bark stopped (m_count restarted)
};

/* State 6 (vtable 0x2e2c60, EE 0x38): SE 8 and a flinch, 0x1d with the PC
 * ahead (under pi / 2), 0x20 behind; 0x13 at its end. layout: host --
 * polymorphic. */
class FshOutCollision : public EmyOut
{
public:
    FshOutCollision(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288e80
    ~FshOutCollision() {}                                                // EE slot 1, ROR 0x00288dd0
    void Init();                                                         // EE slot 2, ROR 0x00157b90
    int  Exec();                                                         // EE slot 3, ROR 0x00157df8
    void Term() {}                                                       // EE slot 4, ROR 0x00288eb0
};

/* State 7 (vtable 0x2e2c90, EE 0x3c): chases the target to 100..200 and
 * stands there (motion 1, the looping SE 0xd). layout: host --
 * polymorphic. */
class FshOutWin : public EmyOut
{
public:
    FshOutWin(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288f68
    ~FshOutWin() {}                                                      // EE slot 1, ROR 0x00288eb8
    void Init();                                                         // EE slot 2, ROR 0x00157e48
    int  Exec();                                                         // EE slot 3, ROR 0x00157eb4
    void Term();                                                         // EE slot 4, ROR 0x00157f44

    float m_range;              // ROR 0x38: utlRandRangeF(100, 200)
};

/* State 12 (vtable 0x2e2cc0, EE 0x38): motions 0x2a, 0x2c, 0x2d, 0x28 in
 * turn. layout: host -- polymorphic. */
class FshOutTest : public EmyOut
{
public:
    FshOutTest(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289048
    ~FshOutTest() {}                                                     // EE slot 1, ROR 0x00288f98
    void Init();                                                         // EE slot 2, ROR 0x00157f60
    int  Exec();                                                         // EE slot 3, ROR 0x00157f68
    void Term() {}                                                       // EE slot 4, ROR 0x00289078
};

/* State 4 (vtable 0x2e2cf0, EE 0x3c): effect kind 3 on the floor, hidden,
 * motion 0x0b (supine) / 0x0a; leaves its group and counts the kill in
 * script global 0x33 + its name. layout: host -- polymorphic. */
class FshOutDead : public EmyOut
{
public:
    FshOutDead(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289130
    ~FshOutDead() {}                                                     // EE slot 1, ROR 0x00289080
    void Init();                                                         // EE slot 2, ROR 0x00158218
    int  Exec();                                                         // EE slot 3, ROR 0x001580b4
    void Term();                                                         // EE slot 4, ROR 0x001581ec

    int m_wait;                 // ROR 0x38: the revival wait (step 0; Init never leaves step 0)
};

#endif /* _EMY_EMY_FSH_H */
