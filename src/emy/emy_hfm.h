/* ==========================================================================
 *  emy/emy_hfm.h
 *
 *  emy/emy_hfm.cpp (0x145020..0x149d18): HfmMain, enemy type 6, and its
 *  per-state outs HfmOut*. No source string names the file; it is the run
 *  from HfmMain::initOut to BdsMain::initOut (every enemy file opens with
 *  its initOut), named like emy_main.cpp. The ELF's linkonce symbols name
 *  the classes, their constructors and the accessors (SetFreeCnt,
 *  IsMissing, ChkRideSide ...).
 *
 *  HfmMain's states (m_out index, initOut): 1 Walk, 2 Slap, 3 Bitten,
 *  4 Break, 5 Counter, 6 Ride, 7 Dead (exec forces it when killed), 8 Win,
 *  9 Test. Its decide inputs (the transition table's columns): 1 the dog
 *  barked, 2 the slap chance, 3 the PC came into sight, 4 lost from
 *  sight, 5 low life, 6 the PC's state class is 4.
 * ======================================================================== */
#ifndef _EMY_EMY_HFM_H
#define _EMY_EMY_HFM_H

#include "sdk/eetypes.h"
#include "emy/emy_main.h"

/* Type 6 (vtable 0x2e2058, vptr at +0x200, EE 0x240). layout: host --
 * polymorphic; EE offsets in the comments. */
class HfmMain : public EmyMain
{
public:
    HfmMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x0028589c
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~HfmMain();                                                          // EE slot 1 (+0x08), ROR 0x0014529c
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x00145680
    int  GetType() { return 6; }                                         // EE slot 3 (+0x18), ROR 0x002858d4
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x001453c0
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x00145560
    void after();                                                        // EE slot 6 (+0x30), ROR 0x00145604
    /* HfmMain's own slot (Clr's slot 7 is its initOut); no caller found. */
    virtual int virtual7();                                              // EE slot 7 (+0x38), ROR 0x001457e8
    /* The outs, the motion list and the class's words. */
    void initOut();                                                      // ROR 0x00145020
    /* createEmy, after loadParam: the held item (param 0x67) as an object
     * handed to the enemy. */
    void initAfterParam();                                               // ROR 0x001452fc
    /* m_free_left - n, at least 0 (EmyManager::subEmyFreeCnt). */
    void subFreeCnt(int n);                                              // ROR 0x0014582c
    bool isPCSameArea();                                                 // ROR 0x00145744
    bool isPCStateClass4();                                              // ROR 0x00145760
    int  isPCInSight();                                                  // ROR 0x00145788
    /* ++m_free_cnt > m_free_max. */
    bool countFree();                                                    // ROR 0x001457f0
    void startFree(int max);                                             // ROR 0x00145808
    /* --m_free_left >= 0. */
    bool tickFreeLeft();                                                 // ROR 0x00145814

    /* The ELF's linkonce accessors. */
    void SetFreeCnt(int n) { m_free_cnt = n; }                           // ROR 0x002858dc
    int  IsMissing() { return m_238; }                                   // ROR 0x002858e4
    void SetMissing(bool on) { m_238 = on; }                             // ROR 0x002858ec
    int  ChkRideSide() { return m_23c; }                                 // ROR 0x002858f4
    void SetRideSide(bool on) { m_23c = on; }                            // ROR 0x002858fc

    int    m_input[8];          // ROR 0x204: selectOut: the current state's row has the input (decide)
    u_char m_pc_seen;           // ROR 0x228: decide: the PC is in sight (inputs 3 / 4); exec clears it
    u_char m_229;               // ROR 0x229
    u_char m_slap_try;          // ROR 0x22a: decide's slap tries (chance 0, 10, 30, 50, 70, 100 %)
    int    m_free_max;          // ROR 0x22c: startFree
    int    m_free_cnt;          // ROR 0x230: SetFreeCnt / countFree
    int    m_free_left;         // ROR 0x234: the dog's bark sets 60..120; subFreeCnt / tickFreeLeft
    /* m_238 (IsMissing) is EmyMain's on the host (EmyManager::getEmyWord238). */
    int    m_23c;               // ROR 0x23c: ChkRideSide; EmyManager::getEmyHoldWord23c
};

/* The outs (EmyOut: vptr at +0x34, own data from +0x38; each vtable's slots
 * are Init, Exec, Term). The constructors are the ELF's linkonce
 * Xxx::Xxx(unsigned int, ACHRstat *, EmyMain *): EmyOut's and the vtable.
 * layout: host -- polymorphic. */

/* State 1 (vtable 0x2e20a0, EE 0x4c): roams around its home point.
 * layout: host -- polymorphic. */
class HfmOutWalk : public EmyOut
{
public:
    HfmOutWalk(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002859b4
    ~HfmOutWalk() {}                                                     // EE slot 1, ROR 0x00285904
    void Init();                                                         // EE slot 2, ROR 0x00145844
    int  Exec();                                                         // EE slot 3, ROR 0x00145894
    void Term();                                                         // EE slot 4, ROR 0x00145d28
    /* A reachable point around the home (param 0x59), else the home. */
    void getRoamPos(float *pos);                                         // ROR 0x00145a10
    /* r < 91: walk in place (step 2), else motion 0xc (step 3). */
    void startIdle(int r);                                               // ROR 0x00145cc8
    void setStep(int step);                                              // ROR 0x00145d44

    int m_cnt;                  // ROR 0x38: frames in the step
};

/* State 2 (vtable 0x2e20d0, EE 0x68). layout: host -- polymorphic. */
class HfmOutSlap : public EmyOut
{
public:
    HfmOutSlap(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00285a94
    ~HfmOutSlap() {}                                                     // EE slot 1, ROR 0x002859e4
    void Init();                                                         // EE slot 2, ROR 0x00145d60
    int  Exec();                                                         // EE slot 3, ROR 0x00145dc0
    void Term();                                                         // EE slot 4, ROR 0x0014635c
};

/* State 4 (vtable 0x2e2100, EE 0x38). layout: host -- polymorphic. */
class HfmOutBreak : public EmyOut
{
public:
    HfmOutBreak(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00285b74
    ~HfmOutBreak() {}                                                    // EE slot 1, ROR 0x00285ac4
    void Init();                                                         // EE slot 2, ROR 0x00146378
    int  Exec();                                                         // EE slot 3, ROR 0x001463b8
    void Term();                                                         // EE slot 4, ROR 0x00146460
};

/* Not built by initOut (vtable 0x2e2130). layout: host -- polymorphic. */
class HfmOutWalk2 : public EmyOut
{
public:
    HfmOutWalk2(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00285c54
    ~HfmOutWalk2() {}                                                    // EE slot 1, ROR 0x00285ba4
    void Init();                                                         // EE slot 2, ROR 0x0014647c
    int  Exec();                                                         // EE slot 3, ROR 0x001464cc
    void Term();                                                         // EE slot 4, ROR 0x001469b0
};

/* State 6 (vtable 0x2e2160, EE 0x98). layout: host -- polymorphic. */
class HfmOutRide : public EmyOut
{
public:
    HfmOutRide(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00285d34
    ~HfmOutRide() {}                                                     // EE slot 1, ROR 0x00285c84
    void Init();                                                         // EE slot 2, ROR 0x001469e8
    int  Exec();                                                         // EE slot 3, ROR 0x00146a20
    void Term();                                                         // EE slot 4, ROR 0x00146ab8
};

/* State 7 (vtable 0x2e2190, EE 0x38). layout: host -- polymorphic. */
class HfmOutDead : public EmyOut
{
public:
    HfmOutDead(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00285e14
    ~HfmOutDead() {}                                                     // EE slot 1, ROR 0x00285d64
    void Init();                                                         // EE slot 2, ROR 0x00148d30
    int  Exec();                                                         // EE slot 3, ROR 0x00148e48
    void Term();                                                         // EE slot 4, ROR 0x00148e50
};

/* State 8 (vtable 0x2e21c0, EE 0x3c). layout: host -- polymorphic. */
class HfmOutWin : public EmyOut
{
public:
    HfmOutWin(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00285ef4
    ~HfmOutWin() {}                                                      // EE slot 1, ROR 0x00285e44
    void Init();                                                         // EE slot 2, ROR 0x00148e6c
    int  Exec();                                                         // EE slot 3, ROR 0x00148ed4
    void Term();                                                         // EE slot 4, ROR 0x00148f34
};

/* State 3 (vtable 0x2e21f0, EE 0x3c). layout: host -- polymorphic. */
class HfmOutBitten : public EmyOut
{
public:
    HfmOutBitten(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00285fd4
    ~HfmOutBitten() {}                                                   // EE slot 1, ROR 0x00285f24
    void Init();                                                         // EE slot 2, ROR 0x00148f50
    int  Exec();                                                         // EE slot 3, ROR 0x00148f74
    void Term();                                                         // EE slot 4, ROR 0x00149168
};

/* State 5 (vtable 0x2e2220, EE 0x44). layout: host -- polymorphic. */
class HfmOutCounter : public EmyOut
{
public:
    HfmOutCounter(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x002860b4
    ~HfmOutCounter() {}                                                  // EE slot 1, ROR 0x00286004
    void Init();                                                         // EE slot 2, ROR 0x00149184
    int  Exec();                                                         // EE slot 3, ROR 0x001491ec
    void Term();                                                         // EE slot 4, ROR 0x001494ac
};

/* State 9 (vtable 0x2e2250, EE 0x38). layout: host -- polymorphic. */
class HfmOutTest : public EmyOut
{
public:
    HfmOutTest(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00286194
    ~HfmOutTest() {}                                                     // EE slot 1, ROR 0x002860e4
    void Init();                                                         // EE slot 2, ROR 0x001494c8
    int  Exec();                                                         // EE slot 3, ROR 0x00149500
    void Term() {}                                                       // EE slot 4, ROR 0x002861c4
};

#endif /* _EMY_EMY_HFM_H */
