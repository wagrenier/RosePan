/* ==========================================================================
 *  emy/emy_got.h
 *
 *  emy/emy_got.cpp (0x1583d4..0x15a17c): GotMain, enemy type 2, and its
 *  own outs GotOutParts and GotOutTest. No source string names the file;
 *  it is the run from GotMain::initOut to PigMain::initOut (every enemy
 *  file opens with its initOut), named like emy_bds.cpp. The ELF's
 *  linkonce symbols name the classes, their constructors, GetType and
 *  GotOutTest::Term.
 *
 *  The Got shares the Dwf's outs (emy/emy_dwf.h); its states (m_out index,
 *  initOut): 1 DwfOutDisappear, 2 Appear, 3 AtkNear, 4 Dead, 5 Stared,
 *  6 Win, 7 Serch, 8 Walk, 9 Wait, 10 AtkFar, 11 Pickup, 12 GotOutParts,
 *  13 Fort, 14 Irritate, 15 GotOutTest. Decide runs only while the
 *  motion input word is set; inputs: 1 the dog barks at it, 2 / 3 / 4
 *  notify bits 4 / 8 / 1, 5 / 6 the PC or the dog came into / left sight,
 *  7..12 while in sight the PC's class 1, the PC ahead, class 2, class 7
 *  off axis, the PC near, the PC or the dog in sight (60), 13
 *  hasOid0Twice, 14 low life, 15 half life, 16..20 the achr's attr bits
 *  (16 8 and 0x10, 17 2 and 4, 18 1, 19 2 or 4, 20 8 or 0x10), 21 the PC's
 *  class 4. Every input is dropped when it would lead to state 11 with no
 *  pickup item (m_pickup) or to state 12 with no part (m_part).
 * ======================================================================== */
#ifndef _EMY_EMY_GOT_H
#define _EMY_EMY_GOT_H

#include "sdk/eetypes.h"
#include "emy/emy_main.h"

/* Type 2 (vtable 0x2e2d20, vptr at +0x200, EE 0x270). layout: host --
 * polymorphic; EE offsets in the comments. */
class GotMain : public EmyMain
{
public:
    GotMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x002891b0
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~GotMain();                                                          // EE slot 1 (+0x08), ROR 0x00158798
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x00158dd8
    int  GetType() { return 2; }                                         // EE slot 3 (+0x18), ROR 0x002891e8
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x001587f8
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x00158ca4
    void after();                                                        // EE slot 6 (+0x30), ROR 0x00158d5c
    /* The outs, the motion list, the state flags, the two targets, a
     * random parsonality 0..2. */
    void initOut();                                                      // ROR 0x001583d4
    bool isPCSameArea();                                                 // ROR 0x00158ea0
    int  testBarkReact();                                                // ROR 0x00158eb4
    /* FshMain::isTargetInSight's body: the PC or the dog within pi, over
     * param 0x5f, or with attr bit 0 or in state 1 without the angle test
     * over 60 (200 for the PC's classes 2, 3, 7, 100 for 6). */
    bool isTargetInSight();                                              // ROR 0x00158f2c
    /* The tests below return 0 when the input's next state is blocked
     * (isBlocked), else EmyMain's test or the attr bits. */
    int  isPCStateClass1();                                              // ROR 0x0015908c
    int  isPCAhead();                                                    // ROR 0x00159104
    int  isPCStateClass2();                                              // ROR 0x0015917c
    int  isPCClass7OffAxis();                                            // ROR 0x001591f4
    int  isPCNear();                                                     // ROR 0x0015926c
    int  isPCOrDogInSight();                                             // ROR 0x001592f0
    int  hasOid0Twice();                                                 // ROR 0x00159368
    int  isLowLife();                                                    // ROR 0x001593e0
    int  isHalfLife();                                                   // ROR 0x00159458
    int  testAttr8And10();                                               // ROR 0x001594d0
    int  testAttr2And4();                                                // ROR 0x00159560
    int  testAttr1();                                                    // ROR 0x001595f0
    int  testAttr2Or4();                                                 // ROR 0x00159668
    int  testAttr8Or10();                                                // ROR 0x001596f8
    int  isPCStateClass4();                                              // ROR 0x00159788
    /* Inline in every test and in decide: the input's next state (peek) is
     * 11 with no pickup item or 12 with no part. */
    bool isBlocked(int input);

    /* selectOut: the current state's row has the input (decide reads 1..21);
     * EE 0x204.., as many words as the table has inputs. PC: EE 0x238
     * (EmyMain::m_238 on the host) is m_input[13] here. */
    int     m_input[22];        // ROR 0x204
    int     m_25c[2];           // ROR 0x25c: not touched here
    u_char  m_264;              // ROR 0x264: cleared at initOut and on a state change, not read here
    u_char  m_seen;             // ROR 0x265: the PC or the dog in sight (decide's edge)
    u_int   m_pickup;           // ROR 0x268: gotFindPickupItem (state 11's gate)
    u_int   m_part;             // ROR 0x26c: gotFindPart (state 12's gate)
};

/* The Got's own outs (EmyOut: vptr at +0x34, own data from +0x38; each
 * vtable's slots are Init, Exec, Term). layout: host -- polymorphic. */

/* State 12 (vtable 0x2e2d60, EE 0x3c): picks up a part: motion 10 with
 * attr bits 8 and 0x10, 1 with one of them, else 0x4f holding item kind
 * 0xa2e, 0x14 otherwise; never ends by itself. layout: host --
 * polymorphic. */
class GotOutParts : public EmyOut
{
public:
    GotOutParts(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289300
    ~GotOutParts() {}                                                    // EE slot 1, ROR 0x00289250
    void Init();                                                         // EE slot 2, ROR 0x00159f98
    int  Exec();                                                         // EE slot 3, ROR 0x0015a034
    void Term();                                                         // EE slot 4, ROR 0x0015a03c
};

/* State 15 (vtable 0x2e2d90, EE 0x38): motion 0x36 over and over (steps 2
 * and 3, motions 0x34 and 0x30, are never reached in the ROM either).
 * layout: host -- polymorphic. */
class GotOutTest : public EmyOut
{
public:
    GotOutTest(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002893e8
    ~GotOutTest() {}                                                     // EE slot 1, ROR 0x00289338
    void Init();                                                         // EE slot 2, ROR 0x0015a058
    int  Exec();                                                         // EE slot 3, ROR 0x0015a060
    void Term() {}                                                       // EE slot 4, ROR 0x00289418
};

#endif /* _EMY_EMY_GOT_H */
