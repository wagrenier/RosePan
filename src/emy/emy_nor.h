/* ==========================================================================
 *  emy/emy_nor.h
 *
 *  emy/emy_nor.cpp (0x150b24..0x153e40): NorMain, enemy type 10, and its
 *  per-state outs NorOut*. No source string names the file; it is the run
 *  from NorMain::initOut to GrgMain::initOut (every enemy file opens with
 *  its initOut), named like emy_bds.cpp. The ELF's linkonce symbols name
 *  the classes, their constructors, GetType and three empty Terms.
 *
 *  A fighting enemy that grabs the PC. States (m_out index, initOut):
 *  1 Attack (claw, grab, lunge or scream by a pattern table), 2 Walk
 *  (wander near its spot), 3 Bitten (the dog's bite: wait, then hit
 *  back), 4 Dead, 5 Counter, 6 BrownEv (the event motion, script global
 *  0x193 while it plays), 7 Coax, 8 Test. Decide inputs: 0 / 1 the PC or
 *  the dog came into / left sight, 2 the dog barks, 3 a motion ended (a
 *  growing chance), 4 low life, 5 flee (once: the PC under half its life,
 *  a minute passed or four hits given), 6 never, 7 the PC's class 4. The
 *  outs hand back 8 (done) or 9 (the chase failed).
 * ======================================================================== */
#ifndef _EMY_EMY_NOR_H
#define _EMY_EMY_NOR_H

#include "sdk/eetypes.h"
#include "emy/emy_main.h"

/* Type 10 (vtable 0x2e2778, vptr at +0x200, EE 0x258). layout: host --
 * polymorphic; EE offsets in the comments. */
class NorMain : public EmyMain
{
public:
    NorMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x00287868
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut(prm);
    }
    ~NorMain();                                                          // EE slot 1 (+0x08), ROR 0x00150df8
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x00151158
    int  GetType() { return 10; }                                        // EE slot 3 (+0x18), ROR 0x002878b0
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x00150e58
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x0015102c
    void after();                                                        // EE slot 6 (+0x30), ROR 0x001510dc
    /* 1 unless fled, or its life at least half of m_max_lp, under 3601
     * frames and the PC at least 3 / 4 of its max life. No caller found. */
    virtual int isPressed();                                             // EE slot 7 (+0x38), ROR 0x00151460
    /* The outs, the motion list, the state flags, the collision shape 3,
     * m_func = func_count (the frame clock), the PC's max life. */
    void initOut(const _tagEnemyParam *prm);                             // ROR 0x00150b24
    int  testInput4();                                                   // ROR 0x00151218
    /* Flee, once (see the file comment); the obj flag 0x800 follows m_fled. */
    int  testInput5();                                                   // ROR 0x0015122c
    int  testInput6();                                                   // ROR 0x0015130c
    int  testInput7();                                                   // ROR 0x00151314
    /* emyChkInSight(param 0x5f, pi) of the PC or of the dog. */
    bool isTargetInSight();                                              // ROR 0x00151328
    /* m_func_count - 1000: 0, or under 200 modulo 700. */
    bool isCoaxTime();                                                   // ROR 0x001513ec
    /* m_bite_wait - 1; true while not negative. */
    int  countBiteWait();                                                // ROR 0x00151448
    /* The actor's p_hit node, unused (no references: dead code). */
    void getHitNodePos();                                                // ROR 0x00151508
    /* The next attack mode of the pattern (a new pattern at its end). */
    int  nextAttack();                                                   // ROR 0x001515c4

    /* selectOut: the current state's row has the input (decide reads
     * 0..7); EE 0x204.., as many words as the table has inputs. */
    int     m_input[10];        // ROR 0x204
    u_char  m_seen;             // ROR 0x22c: the PC or the dog in sight (decide's edge)
    int     m_fled;             // ROR 0x230: testInput5 fired
    u_short m_pc_max_lp;        // ROR 0x234: the PC's param 0x1c at initOut
    u_short m_max_lp;           // ROR 0x236: _tagEnemyParam::m_14
    /* PC: EE 0x238, decide's motion-end count, is EmyMain::m_238 on the host. */
    int     m_hits;             // ROR 0x23c: hits given to the PC (claw, squeeze, counter)
    int     m_bite_wait;        // ROR 0x248: decide's bark input: utlRandRange(30, 60)
    int     m_grab_back;        // ROR 0x24c: the grab came from behind (EmyManager::getEmyHoldWord24c)
    int     m_pattern;          // ROR 0x250: nextAttack's pattern 0..3
    int     m_pattern_pos;      // ROR 0x254: its position
};

/* The outs (EmyOut: vptr at +0x34, own data from +0x38; each vtable's slots
 * are Init, Exec, Term). The constructors are the ELF's linkonce
 * Xxx::Xxx(unsigned int, ACHRstat *, EmyMain *): EmyOut's and the vtable.
 * layout: host -- polymorphic. */

/* State 1 (vtable 0x2e27c0, EE 0x7c): the attack by m_mode: 0 claw, 1 grab
 * the PC (steps 0..8), 2 lunge, 3 scream. layout: host -- polymorphic. */
class NorOutAttack : public EmyOut
{
public:
    NorOutAttack(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002879a0
    ~NorOutAttack() {}                                                   // EE slot 1, ROR 0x002878f0
    void Init();                                                         // EE slot 2, ROR 0x00151674
    int  Exec();                                                         // EE slot 3, ROR 0x001516ac
    void Term();                                                         // EE slot 4, ROR 0x00151744
    /* The mode and its chase (or the scream's turn). */
    void start();                                                        // ROR 0x001517e0
    void restartClaw();                                                  // ROR 0x0015195c
    int  execLunge();                                                    // ROR 0x001519b8
    /* target's spine within 55 of the p_hit node (no references). */
    bool isTouching(u_int target);                                       // ROR 0x00151d4c
    int  execScream();                                                   // ROR 0x00151f1c
    int  execClaw();                                                     // ROR 0x001520cc
    void releasePC();                                                    // ROR 0x0015246c
    int  execGrab();                                                     // ROR 0x00152544
    void grabTry();                                                      // ROR 0x00152748
    void grabHoldStart();                                                // ROR 0x00152b24
    void grabHold();                                                     // ROR 0x00152bb4
    void grabSqueeze();                                                  // ROR 0x00152c60
    void grabSqueezeEnd();                                               // ROR 0x00152cfc
    void grabReleaseEnd();                                               // ROR 0x00152da4
    void grabMissEnd();                                                  // ROR 0x00152e40
    void grabStruggle();                                                 // ROR 0x00152e90
    /* The PC held at the enemy (commandPC 0x15), else the enemy moved. */
    void placePC();                                                      // ROR 0x00152f3c

    u_int m_chase_max;          // ROR 0x38: utlRandPeakHi(10, 150, 3)
    u_int m_chase_cnt;          // ROR 0x3c: cleared, never counted (the claw waits for the arrival)
    int   m_mode;               // ROR 0x40: 0 claw, 1 grab, 2 lunge, 3 scream
    int   m_count;              // ROR 0x48: frames of the step
    int   m_back;               // ROR 0x50: the grab came from behind
    int   m_struggle;           // ROR 0x54: frames the PC struggles (class 3)
    int   m_hold;               // ROR 0x58: frames of the hold, utlRandRange(5, 18)
    int   m_squeeze;            // ROR 0x5c: squeezes left, utlRandRange(1, 3)
    int   m_hit_pc;             // ROR 0x60
    int   m_hit_dog;            // ROR 0x64
    float m_grab_pos[3];        // ROR 0x68: 60 ahead at the grab (not read here)
    float m_rot;                // ROR 0x74: the PC's rot y while held (turned by pi from the front)
    int   m_scream_wait;        // ROR 0x78: the scream's last motion, utlRandRange(45, 60) frames
};

/* State 3 (vtable 0x2e27f0, EE 0x48): after the bite, waits m_bite_wait
 * with its voice, then hits back (motion 0x18) and kicks the dog.
 * layout: host -- polymorphic. */
class NorOutBitten : public EmyOut
{
public:
    NorOutBitten(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00287a80
    ~NorOutBitten() {}                                                   // EE slot 1, ROR 0x002879d0
    void Init();                                                         // EE slot 2, ROR 0x00153030
    int  Exec();                                                         // EE slot 3, ROR 0x00153068
    void Term();                                                         // EE slot 4, ROR 0x00153340

    int m_kick;                 // ROR 0x38: frames to the dog's kick (23)
    int m_count;                // ROR 0x3c: frames of the hit
    int m_hit_pc;               // ROR 0x40
    int m_44;                   // ROR 0x44: cleared at Init, never read
};

/* State 2 (vtable 0x2e2820, EE 0x4c): wanders to random reachable points
 * near its spot, a look round (motion 0xc) when none, waits 20..50.
 * layout: host -- polymorphic. */
class NorOutWalk : public EmyOut
{
public:
    NorOutWalk(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00287b60
    ~NorOutWalk() {}                                                     // EE slot 1, ROR 0x00287ab0
    void Init();                                                         // EE slot 2, ROR 0x0015335c
    int  Exec();                                                         // EE slot 3, ROR 0x00153398
    void Term();                                                         // EE slot 4, ROR 0x001536dc
    /* 0. */
    short getMode();                                                     // ROR 0x001536cc

    short m_mode;               // ROR 0x3a: getMode, never read
    u_int m_walks;              // ROR 0x40: walks since the last wait
    int   m_time;               // ROR 0x44: utlRandRange(1000, 2000), counted down, never read
    int   m_wait;               // ROR 0x48: utlRandRange(20, 50)
};

/* State 4 (vtable 0x2e2850, EE 0x3c): motion 1, damage off.
 * layout: host -- polymorphic. */
class NorOutDead : public EmyOut
{
public:
    NorOutDead(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00287c40
    ~NorOutDead() {}                                                     // EE slot 1, ROR 0x00287b90
    void Init();                                                         // EE slot 2, ROR 0x001536f8
    int  Exec();                                                         // EE slot 3, ROR 0x0015373c
    void Term();                                                         // EE slot 4, ROR 0x00153744
};

/* State 6 (vtable 0x2e2880, EE 0x38): motion 1 once, script global 0x193
 * set while it plays. layout: host -- polymorphic. */
class NorOutBrownEv : public EmyOut
{
public:
    NorOutBrownEv(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00287d20
    ~NorOutBrownEv() {}                                                  // EE slot 1, ROR 0x00287c70
    void Init();                                                         // EE slot 2, ROR 0x00153760
    int  Exec();                                                         // EE slot 3, ROR 0x001537a0
    void Term() {}                                                       // EE slot 4, ROR 0x00287d50
};

/* State 5 (vtable 0x2e28b0, EE 0x44): a hit back (motion 0x2a) at the PC
 * or the dog. layout: host -- polymorphic. */
class NorOutCounter : public EmyOut
{
public:
    NorOutCounter(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00287e08
    ~NorOutCounter() {}                                                  // EE slot 1, ROR 0x00287d58
    void Init();                                                         // EE slot 2, ROR 0x001537fc
    int  Exec();                                                         // EE slot 3, ROR 0x00153864
    void Term();                                                         // EE slot 4, ROR 0x00153b18

    int m_count;                // ROR 0x38: frames
    int m_hit_pc;               // ROR 0x3c
    int m_hit_dog;              // ROR 0x40
};

/* State 7 (vtable 0x2e28e0, EE 0x3c): turns to the PC and coaxes
 * (motions 0x12, 0x13) while NorMain::isCoaxTime. layout: host --
 * polymorphic. */
class NorOutCoax : public EmyOut
{
public:
    NorOutCoax(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00287ee8
    ~NorOutCoax() {}                                                     // EE slot 1, ROR 0x00287e38
    void Init();                                                         // EE slot 2, ROR 0x00153b34
    int  Exec();                                                         // EE slot 3, ROR 0x00153b78
    void Term() {}                                                       // EE slot 4, ROR 0x00287f18
};

/* State 8 (vtable 0x2e2910, EE 0x38): motion 0x21, then 0x28 at every
 * motion end. layout: host -- polymorphic. */
class NorOutTest : public EmyOut
{
public:
    NorOutTest(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00287fd0
    ~NorOutTest() {}                                                     // EE slot 1, ROR 0x00287f20
    void Init();                                                         // EE slot 2, ROR 0x00153d00
    int  Exec();                                                         // EE slot 3, ROR 0x00153d24
    void Term() {}                                                       // EE slot 4, ROR 0x00288000
};

#endif /* _EMY_EMY_NOR_H */
