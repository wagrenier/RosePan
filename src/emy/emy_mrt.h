/* ==========================================================================
 *  emy/emy_mrt.h
 *
 *  emy/emy_mrt.cpp (0x14cde4..0x14dc7c): MrtMain, enemy type 7, and its
 *  per-state outs MrtOut*. No source string names the file; it is the run
 *  from MrtMain::initOut to ClrMain::initOut (every enemy file opens with
 *  its initOut), named like emy_bds.cpp. The ELF's linkonce symbols name
 *  the classes, their constructors and GetType.
 *
 *  MrtMain's states (m_out index, initOut): 1 Win, 2 Serch (the PC),
 *  3 Serch (the PC or the dog: one more MrtOutSerch, which reads the
 *  state), 4 Atk. MrtOutYyy has a vtable and code but no initOut builds it.
 *  Its decide inputs: 2 the PC came into sight (within 10), 3 pad 0's
 *  triangle was pressed (also forced, whatever the state's row says),
 *  4 never (testInput4 returns 0; lost from sight takes it unconditionally),
 *  5 the PC's state class is 1, 6 the PC ahead, 7 the PC in class 7 off
 *  axis. The outs hand back 0 (Serch lost the target, Atk's chase ended
 *  without sight) and 1 (Serch saw it, Atk ended).
 * ======================================================================== */
#ifndef _EMY_EMY_MRT_H
#define _EMY_EMY_MRT_H

#include "sdk/eetypes.h"
#include "emy/emy_main.h"

/* Type 7 (vtable 0x2e2540, vptr at +0x200, EE 0x23c). layout: host --
 * polymorphic; EE offsets in the comments. */
class MrtMain : public EmyMain
{
public:
    MrtMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x00286e34
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~MrtMain();                                                          // EE slot 1 (+0x08), ROR 0x0014cef8
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x0014d240
    int  GetType() { return 7; }                                         // EE slot 3 (+0x18), ROR 0x00286e6c
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x0014cf58
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x0014d13c
    void after();                                                        // EE slot 6 (+0x30), ROR 0x0014d1c4
    /* The outs and the motion list. */
    void initOut();                                                      // ROR 0x0014cde4
    /* Pad 0's m_trig bit 0x1000 (SCE_PADRup, triangle). */
    bool isTrianglePressed();                                            // ROR 0x0014d308
    int  testInput4();                                                   // ROR 0x0014d320
    int  isPCStateClass1();                                              // ROR 0x0014d328
    int  isPCAhead();                                                    // ROR 0x0014d33c
    int  isPCClass7OffAxis();                                            // ROR 0x0014d350
    int  isPCInSight();                                                  // ROR 0x0014d384

    u_char m_pc_seen;           // ROR 0x204: decide: the PC is in sight; exec clears it on a change
    /* selectOut: the current state's row has the input (decide reads
     * 2..7); EE 0x208..0x238, as many words as the table has inputs. */
    int    m_input[12];         // ROR 0x208
    /* initOut also clears the byte at 0x238 (EmyMain::m_238 on the host). */
};

/* The outs (EmyOut: vptr at +0x34, own data from +0x38; each vtable's slots
 * are Init, Exec, Term). The constructors are the ELF's linkonce
 * Xxx::Xxx(unsigned int, ACHRstat *, EmyMain *): EmyOut's and the vtable.
 * layout: host -- polymorphic. */

/* No state (vtable 0x2e2580): initOut never builds it. layout: host --
 * polymorphic. */
class MrtOutYyy : public EmyOut
{
public:
    MrtOutYyy(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00286f24
    ~MrtOutYyy() {}                                                      // EE slot 1, ROR 0x00286e74
    void Init();                                                         // EE slot 2, ROR 0x0014d3cc
    int  Exec();                                                         // EE slot 3, ROR 0x0014d3f8
    void Term();                                                         // EE slot 4, ROR 0x0014d400
};

/* States 2 and 3 (vtable 0x2e25b0, EE 0x3c): chases the PC (2) or the PC
 * or the dog (3). layout: host -- polymorphic. */
class MrtOutSerch : public EmyOut
{
public:
    MrtOutSerch(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00287004
    ~MrtOutSerch() {}                                                    // EE slot 1, ROR 0x00286f54
    void Init();                                                         // EE slot 2, ROR 0x0014d41c
    int  Exec();                                                         // EE slot 3, ROR 0x0014d4a4
    void Term();                                                         // EE slot 4, ROR 0x0014d500
    /* emyChkChase(100, pi): 1 in sight, 0 lost, -1 still chasing. The
     * two are the same code. */
    int  execChasePC();                                                  // ROR 0x0014d51c
    int  execChaseAny();                                                 // ROR 0x0014d598
};

/* State 4 (vtable 0x2e25e0, EE 0x50): closes in, turns to the target,
 * lunges at it (TskMoveTypeGen mode 0xd) hitting what it touches, then
 * motions 0x14, 0x17, 0x16. layout: host -- polymorphic. */
class MrtOutAtk : public EmyOut
{
public:
    MrtOutAtk(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002870e4
    ~MrtOutAtk() {}                                                      // EE slot 1, ROR 0x00287034
    void Init();                                                         // EE slot 2, ROR 0x0014d614
    int  Exec();                                                         // EE slot 3, ROR 0x0014d65c
    void Term();                                                         // EE slot 4, ROR 0x0014db88

    int   m_hit_pc;             // ROR 0x38: the PC was hit this lunge
    int   m_hit_dog;            // ROR 0x3c: the dog was hit
    float m_aim[4];             // ROR 0x40: the target's position when the turn started
};

/* State 1 (vtable 0x2e2610, EE 0x3c): follows the target and plays motion
 * 0x15 once within a random range. layout: host -- polymorphic. */
class MrtOutWin : public EmyOut
{
public:
    MrtOutWin(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002871c4
    ~MrtOutWin() {}                                                      // EE slot 1, ROR 0x00287114
    void Init();                                                         // EE slot 2, ROR 0x0014dba4
    int  Exec();                                                         // EE slot 3, ROR 0x0014dbf4
    void Term();                                                         // EE slot 4, ROR 0x0014dc60

    float m_range;              // ROR 0x38: utlRandRangeF(100, 200) at Init
};

#endif /* _EMY_EMY_MRT_H */
