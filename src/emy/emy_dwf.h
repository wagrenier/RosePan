/* ==========================================================================
 *  emy/emy_dwf.h
 *
 *  emy/emy_dwf.cpp (not written yet: DwfMain's destructor 0x15c2a4 up to
 *  EmyManager 0x16265c, still in unassigned@0015a17c): the outs DwfOut*,
 *  built by DwfMain::initOut and shared with the Pig (PigMain::initOut)
 *  and the Got (GotMain::initOut). Only their declarations so far, so the
 *  Got and Pig can build them; their data members come with the Dwf
 *  session (EE sizes from GotMain::initOut's operator new). DwfMain itself
 *  is still declared in lib/unassigned.h.
 * ======================================================================== */
#ifndef _EMY_EMY_DWF_H
#define _EMY_EMY_DWF_H

#include "sdk/eetypes.h"
#include "emy/emy_main.h"

/* (vtable 0x2e2ed0, EE 0x38) layout: host -- polymorphic. */
class DwfOutDisappear : public EmyOut
{
public:
    DwfOutDisappear(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289990
    ~DwfOutDisappear() {}                                                // EE slot 1, ROR 0x002898e0
    void Init();                                                         // EE slot 2, ROR 0x0015e214
    int  Exec() { return -1; }                                           // EE slot 3, ROR 0x002899c0
    void Term();                                                         // EE slot 4, ROR 0x0015e2a0
};

/* (vtable 0x2e2f00, EE 0x38) layout: host -- polymorphic. */
class DwfOutAppear : public EmyOut
{
public:
    DwfOutAppear(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289a78
    ~DwfOutAppear() {}                                                   // EE slot 1, ROR 0x002899c8
    void Init();                                                         // EE slot 2, ROR 0x0015e324
    int  Exec();                                                         // EE slot 3, ROR 0x0015e344
    void Term() {}                                                       // EE slot 4, ROR 0x00289aa8
};

/* (vtable 0x2e2f30, EE 0x38) layout: host -- polymorphic. */
class DwfOutWait : public EmyOut
{
public:
    DwfOutWait(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289b60
    ~DwfOutWait() {}                                                     // EE slot 1, ROR 0x00289ab0
    void Init();                                                         // EE slot 2, ROR 0x0015e4b0
    int  Exec();                                                         // EE slot 3, ROR 0x0015e514
    void Term() {}                                                       // EE slot 4, ROR 0x00289b90
};

/* (vtable 0x2e2f60, EE 0x50) layout: host -- polymorphic. */
class DwfOutWalk : public EmyOut
{
public:
    DwfOutWalk(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289c58
    ~DwfOutWalk() {}                                                     // EE slot 1, ROR 0x00289b98
    void Init();                                                         // EE slot 2, ROR 0x0015e51c
    int  Exec();                                                         // EE slot 3, ROR 0x0015e5bc
    void Term();                                                         // EE slot 4, ROR 0x0015ea8c
};

/* (vtable 0x2e2f90, EE 0x38) layout: host -- polymorphic. */
class DwfOutSerch : public EmyOut
{
public:
    DwfOutSerch(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289d38
    ~DwfOutSerch() {}                                                    // EE slot 1, ROR 0x00289c88
    void Init();                                                         // EE slot 2, ROR 0x0015eac4
    int  Exec();                                                         // EE slot 3, ROR 0x0015eb00
    void Term();                                                         // EE slot 4, ROR 0x0015eba8
};

/* (vtable 0x2e2fc0, EE 0x40) layout: host -- polymorphic. */
class DwfOutPickup : public EmyOut
{
public:
    DwfOutPickup(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x00289e18
    ~DwfOutPickup() {}                                                   // EE slot 1, ROR 0x00289d68
    void Init();                                                         // EE slot 2, ROR 0x0015ebc4
    int  Exec();                                                         // EE slot 3, ROR 0x0015ec30
    void Term();                                                         // EE slot 4, ROR 0x0015ef78
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

/* (vtable 0x2e3020, EE 0x40) layout: host -- polymorphic. */
class DwfOutAtkFar : public EmyOut
{
public:
    DwfOutAtkFar(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028a034
    ~DwfOutAtkFar() {}                                                   // EE slot 1, ROR 0x00289f84
    void Init();                                                         // EE slot 2, ROR 0x001606ec
    int  Exec();                                                         // EE slot 3, ROR 0x00160784
    void Term();                                                         // EE slot 4, ROR 0x0016097c
};

/* (vtable 0x2e3050, EE 0x48) layout: host -- polymorphic. */
class DwfOutStared : public EmyOut
{
public:
    DwfOutStared(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028a114
    ~DwfOutStared() {}                                                   // EE slot 1, ROR 0x0028a064
    void Init();                                                         // EE slot 2, ROR 0x00160a44
    int  Exec();                                                         // EE slot 3, ROR 0x00160b08
    void Term();                                                         // EE slot 4, ROR 0x00160c9c
};

/* (vtable 0x2e3080, EE 0x40) layout: host -- polymorphic. */
class DwfOutWin : public EmyOut
{
public:
    DwfOutWin(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028a1f4
    ~DwfOutWin() {}                                                      // EE slot 1, ROR 0x0028a144
    void Init();                                                         // EE slot 2, ROR 0x00160cb8
    int  Exec();                                                         // EE slot 3, ROR 0x00160d50
    void Term();                                                         // EE slot 4, ROR 0x00161160
};

/* (vtable 0x2e30b0, EE size: DwfMain::initOut) layout: host -- polymorphic. */
class DwfOutParts : public EmyOut
{
public:
    DwfOutParts(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028a2d4
    ~DwfOutParts() {}                                                    // EE slot 1, ROR 0x0028a224
    void Init();                                                         // EE slot 2, ROR 0x0016117c
    int  Exec();                                                         // EE slot 3, ROR 0x001611e0
    void Term();                                                         // EE slot 4, ROR 0x001611e8
};

/* (vtable 0x2e30e0, EE size: DwfMain::initOut) layout: host -- polymorphic. */
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

/* (vtable 0x2e3140, EE 0x44) layout: host -- polymorphic. */
class DwfOutIrritate : public EmyOut
{
public:
    DwfOutIrritate(u_int oid, ACHRstat *achr, EmyMain *emy) : EmyOut(oid, achr, emy) {} // ROR 0x0028a584
    ~DwfOutIrritate() {}                                                 // EE slot 1, ROR 0x0028a4d4
    void Init();                                                         // EE slot 2, ROR 0x00161b48
    int  Exec();                                                         // EE slot 3, ROR 0x00161bd4
    void Term();                                                         // EE slot 4, ROR 0x00161d8c
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
