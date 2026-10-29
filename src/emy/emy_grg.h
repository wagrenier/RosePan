/* ==========================================================================
 *  emy/emy_grg.h
 *
 *  emy/emy_grg.cpp (0x153e40..0x155d54): GrgMain, enemy type 9, and its
 *  per-state outs GrgOut*. No source string names the file; it is the run
 *  from GrgMain::initOut to FshMain's destructor 0x155d54 (the Fsh file
 *  puts its main methods before FshMain::initOut), named like emy_bds.cpp.
 *  The ELF's linkonce symbols name the classes, their constructors,
 *  GetType and two empty Terms.
 *
 *  A roaming enemy of rooms 0x84..0x8b (getStatus 5; its area is the
 *  achr's param 8, set to -1 at initOut). States (m_out index, initOut):
 *  1 Catch (chases its target, then a catch motion by the secta cell), 2
 *  Walk (walks between the room's points, to an fchr of type 9 it turns to
 *  and hides in, or idles), 3 Bitten, 4 Disappear (hidden; reappears at a
 *  room point after a wait by the distance), 5 Event (hidden, script
 *  global 0x53 off), 6 Test. Decide inputs: 0 the room is its area while
 *  m_arm (with m_238 1 also the PC in sight), 1 m_arm clear and room 0x81
 *  or 16 appearances with the PC elsewhere, 4 the room stopped being its
 *  area, 5 / 6 the PC or the dog came into / left sight (in its area
 *  only), 7 never, 8 releaseHold (EmyManager::releaseEmyHold). The outs
 *  hand back 3 (hid in the fchr), 9 (done) or 10 (the chase failed).
 *  Script globals: 0x53 1 while it is about (0 in Event), 0x54 its area,
 *  0x55 -1 (after, every frame).
 * ======================================================================== */
#ifndef _EMY_EMY_GRG_H
#define _EMY_EMY_GRG_H

#include "sdk/eetypes.h"
#include "emy/emy_main.h"

/* Type 9 (vtable 0x2e2940, vptr at +0x200, EE 0x244). layout: host --
 * polymorphic; EE offsets in the comments. */
class GrgMain : public EmyMain
{
public:
    GrgMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x00288058
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~GrgMain();                                                          // EE slot 1 (+0x08), ROR 0x00154030
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x00154340
    int  GetType() { return 9; }                                         // EE slot 3 (+0x18), ROR 0x00288090
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x00154090
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x00154208
    void after();                                                        // EE slot 6 (+0x30), ROR 0x00154270
    /* The outs, the motion list, the state flags, area -1, m_arm 1. */
    void initOut();                                                      // ROR 0x00153e40
    /* With a PC, the current room is the achr's area (param 8). */
    bool isPCInArea();                                                   // ROR 0x00154408
    /* The current room is the achr's area (no PC test). */
    bool isAreaCurrent();                                                // ROR 0x00154474
    int  testInput7();                                                   // ROR 0x001544c8
    /* m_arm clear: room 0x81, or 16 appearances and the PC's room not its
     * area and the PC not in class 10; sets m_arm. */
    int  testInput1();                                                   // ROR 0x001544d0
    /* m_arm set and m_238 not 2 / 3: the room is its area (m_238 1: and
     * the PC in sight); clears m_arm. */
    int  testInput0();                                                   // ROR 0x00154584
    /* isPCInArea, and emyChkInSight(param 0x5f, pi) of the PC or the dog. */
    bool isTargetInSight();                                              // ROR 0x00154668
    /* isTargetInSight (EmyManager::testEmyView). */
    int  testView();                                                     // ROR 0x00154734
    /* The achr loses flags 0x10000000 and 0x80; m_released = 1 (input 8). */
    void releaseHold();                                                  // ROR 0x00154748

    /* selectOut: the current state's row has the input (decide reads
     * 0..8); EE 0x204.., as many words as the table has inputs. */
    int     m_input[12];        // ROR 0x204
    u_char  m_arm;              // ROR 0x234: input 0 armed (set by input 1 and initOut)
    u_char  m_in_area;          // ROR 0x235: the room was its area (input 4's edge)
    u_char  m_seen;             // ROR 0x236: the PC or the dog in sight (inputs 5 / 6)
    u_char  m_appear_count;     // ROR 0x237: GrgOutDisappear's reappearances
    /* PC: EE 0x238 is EmyMain::m_238 on the host (1 at initOut). */
    int     m_23c;              // ROR 0x23c: 0 at initOut, not read here
    int     m_released;         // ROR 0x240: releaseHold, input 8
};

/* The outs (EmyOut: vptr at +0x34, own data from +0x38; each vtable's slots
 * are Init, Exec, Term). The constructors are the ELF's linkonce
 * Xxx::Xxx(unsigned int, ACHRstat *, EmyMain *): EmyOut's and the vtable.
 * layout: host -- polymorphic. */

/* State 1 (vtable 0x2e2980, EE 0x38): chases the target (the PC, or the
 * dog by sight), then catches: motion 0xd, or on a cell flagged 0x20000
 * 0xb / 0xc by the PC below / above, and 0xf; motion 0xe and a new chase
 * when the target is not within pi / 4 ahead. layout: host -- polymorphic. */
class GrgOutCatch : public EmyOut
{
public:
    GrgOutCatch(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288178
    ~GrgOutCatch() {}                                                    // EE slot 1, ROR 0x002880c8
    void Init();                                                         // EE slot 2, ROR 0x00154780
    int  Exec();                                                         // EE slot 3, ROR 0x001547e8
    void Term();                                                         // EE slot 4, ROR 0x00154a18
};

/* State 3 (vtable 0x2e29b0, EE 0x40): motion 0x13, a wait of 60..180,
 * then motion 0x14 (0x16 with m_hit_back clear: never). layout: host --
 * polymorphic. */
class GrgOutBitten : public EmyOut
{
public:
    GrgOutBitten(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288258
    ~GrgOutBitten() {}                                                   // EE slot 1, ROR 0x002881a8
    void Init();                                                         // EE slot 2, ROR 0x00154a34
    int  Exec();                                                         // EE slot 3, ROR 0x00154a94
    void Term();                                                         // EE slot 4, ROR 0x00154b44

    int m_hit_back;             // ROR 0x38: 1 at Init: motion 0x14, else 0x16
    int m_wait;                 // ROR 0x3c: utlRandRange(60, 180)
};

/* State 4 (vtable 0x2e29e0, EE 0x5c): hidden for calcWait frames
 * (recomputed while the room differs from Init's), then placed at the
 * room's first point (table 0x2efc98) and shown; input 9 at the end of
 * the motion. layout: host -- polymorphic. */
class GrgOutDisappear : public EmyOut
{
public:
    GrgOutDisappear(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288338
    ~GrgOutDisappear() {}                                                // EE slot 1, ROR 0x00288288
    void Init();                                                         // EE slot 2, ROR 0x00154b60
    int  Exec();                                                         // EE slot 3, ROR 0x00154bd8
    void Term();                                                         // EE slot 4, ROR 0x00154f1c
    /* The current room 0x84..0x8b as a row 0..8 of the point tables. */
    int  getRoomIndex();                                                 // ROR 0x00154d78
    /* Half the x / z distance d to the room's point m_point:
     * utlRandRange(0.7 d, 1.3 d). */
    int  calcWait();                                                     // ROR 0x00154e20

    int m_count;                // ROR 0x3c: frames hidden
    int m_wait;                 // ROR 0x40: calcWait
    int m_room;                 // ROR 0x44: getStatus 5 at Init
    int m_point;                // ROR 0x48: the point of the room's row (always 0)
};

/* State 5 (vtable 0x2e2a10, EE 0x38): hidden, script global 0x53 = 0.
 * layout: host -- polymorphic. */
class GrgOutEvent : public EmyOut
{
public:
    GrgOutEvent(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288418
    ~GrgOutEvent() {}                                                    // EE slot 1, ROR 0x00288368
    void Init();                                                         // EE slot 2, ROR 0x00154f54
    int  Exec();                                                         // EE slot 3, ROR 0x00154f88
    void Term() {}                                                       // EE slot 4, ROR 0x00288448
};

/* State 2 (vtable 0x2e2a40, EE 0x40): by the room's rates (table
 * 0x2f0410) walks to another of its four points (0x2f01d0), to an fchr
 * of type 9 nearby (0x2f0430 / 0x2f08b0), turns to it, plays motion 0x21
 * / 0x23 with the fchr's motion 1 and hides (input 3), or idles (motions
 * 0x28, 0x2a / 0x2b, 0x29). layout: host -- polymorphic. */
class GrgOutWalk : public EmyOut
{
public:
    GrgOutWalk(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00288500
    ~GrgOutWalk() {}                                                     // EE slot 1, ROR 0x00288450
    void Init();                                                         // EE slot 2, ROR 0x00154f90
    int  Exec();                                                         // EE slot 3, ROR 0x00154fd8
    void Term();                                                         // EE slot 4, ROR 0x00155bf8
    /* A draw over the room's rates: walkToPoint (0), walkToFchr (1) or
     * startIdle (4), falling back in that order; the next step. */
    int  selectNext();                                                   // ROR 0x00155498
    /* The achr's area 0x84..0x8b as a row 0..8 (0x89: 5 below z 260). */
    int  getRoomIndex();                                                 // ROR 0x0015558c
    /* Another of the room's four points; moves there when reachable (1). */
    int  walkToPoint();                                                  // ROR 0x00155648
    /* The n-th fchr of type 9 near by the room's pick table; moves to its
     * point (1), 0 when there is none. */
    int  walkToFchr();                                                   // ROR 0x0015582c
    /* Motion 0x28 and utlRandRange(2, 6) idle loops; 4 (the idle step). */
    int  startIdle();                                                    // ROR 0x00155bb4

    u_char  m_point;            // ROR 0x38: the room point walked to last
    u_char  m_idle;             // ROR 0x39: idle loops left
    u_short m_room;             // ROR 0x3a: getRoomIndex at Init
    u_int   m_fchr;             // ROR 0x3c: walkToFchr's fchr oid
};

/* State 6 (vtable 0x2e2a70, EE 0x38): motions 0x15 and 0x16 in turn.
 * layout: host -- polymorphic. */
class GrgOutTest : public EmyOut
{
public:
    GrgOutTest(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002885e0
    ~GrgOutTest() {}                                                     // EE slot 1, ROR 0x00288530
    void Init();                                                         // EE slot 2, ROR 0x00155c14
    int  Exec();                                                         // EE slot 3, ROR 0x00155c2c
    void Term() {}                                                       // EE slot 4, ROR 0x00288610
};

#endif /* _EMY_EMY_GRG_H */
