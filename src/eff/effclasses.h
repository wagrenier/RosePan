/* ==========================================================================
 *  eff/effclasses.h
 *
 *  The Eff classes EffMan::createEff (eff/effman.cpp) news, one per effect
 *  kind, declared only as far as createEff needs them: the constructor and
 *  destructor and the virtual slots each overrides (rtti.py; Eff0's slot
 *  names), with the EE object size createEff passes to operator new. Their
 *  code (0x16afcc..0x17b6d4 outside eff/effman.cpp, in alphabetical class
 *  link order) is not written yet: the overrides are stubs, and the data
 *  members past Eff0 are an undecoded block (m_body) of the EE size. When a
 *  class's own file is written, move its declaration into that file's
 *  header and include it here.
 *
 *  Constructors marked linkonce are the ELF's inline ones (createEff inlines
 *  them: Eff0::Eff0, then the class's vtable); the others are out of line.
 *  Kinds 7, 0xe..0x10, 0x24, 0x25, 0x28 and 0x30 have no class (createEff
 *  news nothing for them). EffFindItem1 (kind 0x32) lives in eff/effman.h.
 * ======================================================================== */
#ifndef _EFF_EFFCLASSES_H
#define _EFF_EFFCLASSES_H

#include "eff/eff0.h"
#include "eff/effbg.h"
#include "eff/effblood.h"
#include "eff/effcameraanim1.h"
#include "eff/efffog3.h"
#include "eff/efflight.h"
#include "eff/effmuzzleflash.h"
#include "eff/effreload.h"
#include "eff/effsteam.h"
#include "eff/effwater.h"

/* kinds 0x1..0x6 (EffBloodSpray1 .. EffBloodStay3Prim): eff/effblood.h. */

/* kind 0x8; EE 0x1a0, vtable 0x2e3720. layout: host. */
class EffBreath1 : public Eff0
{
public:
    EffBreath1() {}                                                         // ROR 0x0028b4c8 (linkonce; inlined in createEff)
    virtual ~EffBreath1() {}                                                // ROR 0x0028b418 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016dcc8
    virtual int  liveMain();                                                // ROR 0x0016de40
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016df38
    virtual void updateZDepth();                                            // ROR 0x0016e3ec

    u_char m_body[0x17c];                                                   // ROR 0x24..0x1a0: not decoded yet
};

/* kind 0x9; EE 0x254, vtable 0x2e3790. layout: host. */
class EffBreath1Space : public Eff0
{
public:
    EffBreath1Space() {}                                                    // ROR 0x0028b3d0 (linkonce; inlined in createEff)
    virtual ~EffBreath1Space() {}                                           // ROR 0x0028b320 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016e6e4
    virtual int  liveMain();                                                // ROR 0x0016e7cc
    virtual void render(RENDERPARAM *param);                                // ROR 0x0028b408 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028b410 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *layer);                 // ROR 0x0028b400 (linkonce)

    u_char m_body[0x230];                                                   // ROR 0x24..0x254: not decoded yet
};

/* kind 0xa; EE 0x54, vtable 0x2e36b0. layout: host. */
class EffBreath2 : public Eff0
{
public:
    EffBreath2() {}                                                         // ROR 0x0028b5a8 (linkonce; inlined in createEff)
    virtual ~EffBreath2() {}                                                // ROR 0x0028b4f8 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016eb00
    virtual int  liveMain();                                                // ROR 0x0016eb98
    virtual void render(RENDERPARAM *param);                                // ROR 0x0028b5d8 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028b5e0 (linkonce)

    u_char m_body[0x30];                                                    // ROR 0x24..0x54: not decoded yet
};

/* kind 0xb; EE 0x80, vtable 0x2e3640. layout: host. */
class EffBreath2Prim : public Eff0
{
public:
    EffBreath2Prim() {}                                                     // ROR 0x0028b698 (linkonce; inlined in createEff)
    virtual ~EffBreath2Prim() {}                                            // ROR 0x0028b5e8 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016edc0
    virtual int  liveMain();                                                // ROR 0x0016eeb4
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016ef74
    virtual void updateZDepth();                                            // ROR 0x0016f160

    u_char m_body[0x5c];                                                    // ROR 0x24..0x80: not decoded yet
};

/* kind 0xc; EE 0xb0, vtable 0x2e35d0. layout: host. */
class EffBreath2Ground : public Eff0
{
public:
    EffBreath2Ground() {}                                                   // ROR 0x0028b778 (linkonce; inlined in createEff)
    virtual ~EffBreath2Ground() {}                                          // ROR 0x0028b6c8 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016f1a0
    virtual int  liveMain();                                                // ROR 0x0016f380
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016f3c8

    u_char m_body[0x8c];                                                    // ROR 0x24..0xb0: not decoded yet
};

/* kind 0x12; EE 0xc0, vtable 0x2e38e0. layout: host. */
class EffDust2 : public Eff0
{
public:
    EffDust2() {}                                                           // ROR 0x0028b950 (linkonce; inlined in createEff)
    virtual ~EffDust2() {}                                                  // ROR 0x0028b8a0 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x001704a4
    virtual int  liveMain();                                                // ROR 0x00170594
    virtual void render(RENDERPARAM *param);                                // ROR 0x0028b988 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028b990 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *layer);                 // ROR 0x0028b980 (linkonce)

    u_char m_body[0x9c];                                                    // ROR 0x24..0xc0: not decoded yet
};

/* kind 0x13; EE 0x70, vtable 0x2e3870. layout: host. */
class EffDust2Prim : public Eff0
{
public:
    EffDust2Prim() {}                                                       // ROR 0x0028ba48 (linkonce; inlined in createEff)
    virtual ~EffDust2Prim() {}                                              // ROR 0x0028b998 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0017093c
    virtual int  liveMain();                                                // ROR 0x001709c8
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x001709f4
    virtual void updateZDepth();                                            // ROR 0x00170c64

    u_char m_body[0x4c];                                                    // ROR 0x24..0x70: not decoded yet
};

/* kind 0x14; EE 0x60, vtable 0x2e3a30. layout: host. */
class EffFire1 : public Eff0
{
public:
    EffFire1() {}                                                           // ROR 0x0028bb28 (linkonce; inlined in createEff)
    virtual ~EffFire1() {}                                                  // ROR 0x0028ba78 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00170ca4
    virtual int  liveMain();                                                // ROR 0x00170d44
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00170e14
    virtual void liveActivateExec();                                        // ROR 0x00171278
    virtual void updateZDepth();                                            // ROR 0x001711fc

    u_char m_body[0x3c];                                                    // ROR 0x24..0x60: not decoded yet
};

/* kind 0x15; EE 0x90, vtable 0x2e39c0. layout: host. */
class EffFire1Ex : public EffFire1
{
public:
    EffFire1Ex() {}                                                         // ROR 0x0028bc08 (linkonce; inlined in createEff)
    virtual ~EffFire1Ex() {}                                                // ROR 0x0028bb58 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x001712bc
    virtual void updateZDepth();                                            // ROR 0x00171378

    u_char m_body[0x30];                                                    // ROR 0x60..0x90: not decoded yet
};

/* kind 0x16; EE 0x70, vtable 0x2e3950. layout: host. */
class EffFire2 : public Eff0
{
public:
    EffFire2() {}                                                           // ROR 0x0028bce8 (linkonce; inlined in createEff)
    virtual ~EffFire2() {}                                                  // ROR 0x0028bc38 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0017145c
    virtual int  liveMain();                                                // ROR 0x00171534
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00171548
    virtual void updateZDepth();                                            // ROR 0x0017167c

    u_char m_body[0x4c];                                                    // ROR 0x24..0x70: not decoded yet
};

/* kind 0x17; EE 0x70, vtable 0x2e3cd0. layout: host. */
class EffFog1 : public Eff0
{
public:
    EffFog1() {}                                                            // ROR 0x0028bdc8 (linkonce; inlined in createEff)
    virtual ~EffFog1() {}                                                   // ROR 0x0028bd18 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x001716bc
    virtual int  liveMain();                                                // ROR 0x001717b8
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x001717f4
    virtual void updateZDepth();                                            // ROR 0x00171bbc

    u_char m_body[0x4c];                                                    // ROR 0x24..0x70: not decoded yet
};

/* kind 0x18; EE 0x50, vtable 0x2e3c60. layout: host. */
class EffFog2 : public Eff0
{
public:
    EffFog2() {}                                                            // ROR 0x0028bea8 (linkonce; inlined in createEff)
    virtual ~EffFog2() {}                                                   // ROR 0x0028bdf8 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00171bfc
    virtual int  liveMain();                                                // ROR 0x00171c94
    virtual void render(RENDERPARAM *param);                                // ROR 0x0028bee0 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028bee8 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *layer);                 // ROR 0x0028bed8 (linkonce)

    u_char m_body[0x2c];                                                    // ROR 0x24..0x50: not decoded yet
};

/* kind 0x19; EE 0x80, vtable 0x2e3bf0. layout: host. */
class EffFog2Prim : public Eff0
{
public:
    EffFog2Prim() {}                                                        // ROR 0x0028bfa0 (linkonce; inlined in createEff)
    virtual ~EffFog2Prim() {}                                               // ROR 0x0028bef0 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00171e28
    virtual int  liveMain();                                                // ROR 0x00172058
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00172284
    virtual void updateZDepth();                                            // ROR 0x00172638

    u_char m_body[0x5c];                                                    // ROR 0x24..0x80: not decoded yet
};

/* kinds 0x1a..0x1c (EffFog3, EffFog3Dark, EffFog3WeakUp): eff/efffog3.h. */

/* kind 0x1d; EE 0x4c, vtable 0x2e3e90. layout: host. */
class EffFoot1 : public Eff0
{
public:
    EffFoot1() {}                                                           // ROR 0x0028c300 (linkonce; inlined in createEff)
    virtual ~EffFoot1() {}                                                  // ROR 0x0028c250 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00172e4c
    virtual int  liveMain();                                                // ROR 0x00172f4c
    virtual void render(RENDERPARAM *param);                                // ROR 0x0028c330 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028c338 (linkonce)

    u_char m_body[0x28];                                                    // ROR 0x24..0x4c: not decoded yet
};

/* kind 0x1e; EE 0x60, vtable 0x2e3e20. layout: host. */
class EffFoot1Prim : public Eff0
{
public:
    EffFoot1Prim() {}                                                       // ROR 0x0028c3f0 (linkonce; inlined in createEff)
    virtual ~EffFoot1Prim() {}                                              // ROR 0x0028c340 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x001731f0
    virtual int  liveMain();                                                // ROR 0x00173274
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x001732d8
    virtual void updateZDepth();                                            // ROR 0x00173340

    u_char m_body[0x3c];                                                    // ROR 0x24..0x60: not decoded yet
};

/* kind 0x1f; EE 0x24, vtable 0x2e3db0. layout: host. */
class EffFoot2 : public Eff0
{
public:
    EffFoot2() {}                                                           // ROR 0x0028c4d0 (linkonce; inlined in createEff)
    virtual ~EffFoot2() {}                                                  // ROR 0x0028c420 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00173630
    virtual int  liveMain();                                                // ROR 0x00173688
    virtual void render(RENDERPARAM *param);                                // ROR 0x0028c508 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028c510 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *layer);                 // ROR 0x0028c500 (linkonce)
};

/* kind 0x20; EE 0x50, vtable 0x2e3d40. layout: host. */
class EffFoot2Prim : public Eff0
{
public:
    EffFoot2Prim() {}                                                       // ROR 0x0028c5c8 (linkonce; inlined in createEff)
    virtual ~EffFoot2Prim() {}                                              // ROR 0x0028c518 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00173a90
    virtual int  liveMain();                                                // ROR 0x00173b2c
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00173b68

    u_char m_body[0x2c];                                                    // ROR 0x24..0x50: not decoded yet
};

/* kinds 0x33, 0x34 (EffGeneric1, EffBg1): eff/effgeneric.h, eff/effbg.h. */

/* kinds 0x21..0x23 (EffLight1, EffLight2, EffLight3): eff/efflight.h. */

/* kind 0x26 (EffMuzzleFlash1): eff/effmuzzleflash.h. */

/* kind 0x27 (EffReload1): eff/effreload.h. */

/* kinds 0x11, 0x29..0x2e (EffCloud1, EffSteam1 .. EffSteam2Prim): eff/effsteam.h. */

/* kinds 0x2f, 0x31 (EffWater1, EffWater3): eff/effwater.h. */

#endif /* _EFF_EFFCLASSES_H */
