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
#include "eff/effcameraanim1.h"
#include "eff/efffog3.h"

/* kind 0x1; EE 0x280, vtable 0x2e33a0. layout: host. */
class EffBloodSpray1 : public Eff0
{
public:
    EffBloodSpray1();                                                       // ROR 0x0016b62c
    virtual ~EffBloodSpray1() {}                                            // ROR 0x0028ae78 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016b668
    virtual int  liveMain();                                                // ROR 0x0016b978
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016ba7c
    virtual void updateZDepth();                                            // ROR 0x0016bdd4

    u_char m_body[0x25c];                                                   // ROR 0x24..0x280: not decoded yet
};

/* kind 0x2; EE 0x590, vtable 0x2e3330. layout: host. */
class EffBloodSpray2 : public Eff0
{
public:
    EffBloodSpray2();                                                       // ROR 0x0016be14
    virtual ~EffBloodSpray2() {}                                            // ROR 0x0028af28 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016be70
    virtual int  liveMain();                                                // ROR 0x0016c258
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016c400
    virtual void updateZDepth();                                            // ROR 0x0016c748

    u_char m_body[0x56c];                                                   // ROR 0x24..0x590: not decoded yet
};

/* kind 0x3; EE 0xc0, vtable 0x2e3560. layout: host. */
class EffBloodStay1 : public Eff0
{
public:
    EffBloodStay1() {}                                                      // ROR 0x0028b088 (linkonce; inlined in createEff)
    virtual ~EffBloodStay1() {}                                             // ROR 0x0028afd8 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016c7c0
    virtual int  liveMain();                                                // ROR 0x0016ca00
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016cac0

    u_char m_body[0x9c];                                                    // ROR 0x24..0xc0: not decoded yet
};

/* kind 0x4; EE 0x4f0, vtable 0x2e34f0. layout: host. */
class EffBloodStay2 : public Eff0
{
public:
    EffBloodStay2() {}                                                      // ROR 0x0028b168 (linkonce; inlined in createEff)
    virtual ~EffBloodStay2() {}                                             // ROR 0x0028b0b8 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016d01c
    virtual int  liveMain();                                                // ROR 0x0016d0d0
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016d4bc

    u_char m_body[0x4cc];                                                   // ROR 0x24..0x4f0: not decoded yet
};

/* kind 0x5; EE 0x2c, vtable 0x2e3480. layout: host. */
class EffBloodStay3 : public Eff0
{
public:
    EffBloodStay3() {}                                                      // ROR 0x0028b1e8 (linkonce; inlined in createEff)
    virtual ~EffBloodStay3();                                               // ROR 0x0016d804
    virtual int  init(const EffParam *param);                               // ROR 0x0016d870
    virtual int  liveMain();                                                // ROR 0x0016d8ac
    virtual void render(RENDERPARAM *param);                                // ROR 0x0028b230 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028b238 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *layer);                 // ROR 0x0028b218 (linkonce)

    u_char m_body[0x8];                                                     // ROR 0x24..0x2c: not decoded yet
};

/* kind 0x6; EE 0x50, vtable 0x2e3410. layout: host. */
class EffBloodStay3Prim : public Eff0
{
public:
    EffBloodStay3Prim() {}                                                  // ROR 0x0028b2f0 (linkonce; inlined in createEff)
    virtual ~EffBloodStay3Prim() {}                                         // ROR 0x0028b240 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016d9a8
    virtual int  liveMain();                                                // ROR 0x0016da30
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016da44

    u_char m_body[0x2c];                                                    // ROR 0x24..0x50: not decoded yet
};

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

/* kind 0x33; EE 0x90, vtable 0x2e3f00. layout: host. */
class EffGeneric1 : public Eff0
{
public:
    EffGeneric1();                                                          // ROR 0x00173df8
    virtual ~EffGeneric1();                                                 // ROR 0x00173e48
    virtual int  init(const EffParam *param);                               // ROR 0x00173f3c
    virtual int  liveMain();                                                // ROR 0x00174438
    virtual void renderPreMain(RENDERPARAM *param);                         // ROR 0x00174534
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028c648 (linkonce)

    u_char m_body[0x6c];                                                    // ROR 0x24..0x90: not decoded yet
};

/* kind 0x34; EE 0x90, vtable 0x2e32c0. layout: host. */
class EffBg1 : public EffGeneric1
{
public:
    EffBg1();                                                               // ROR 0x0016afcc
    virtual ~EffBg1();                                                      // ROR 0x0016affc
    virtual int  init(const EffParam *param);                               // ROR 0x0016b060
    virtual int  liveMain();                                                // ROR 0x0016b348
    virtual void renderPreMain(RENDERPARAM *param);                         // ROR 0x0016b444
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028ae70 (linkonce)
};

/* kind 0x21; EE 0x70, vtable 0x2e40c0. layout: host. */
class EffLight1 : public Eff0
{
public:
    EffLight1() {}                                                          // ROR 0x0028c700 (linkonce; inlined in createEff)
    virtual ~EffLight1() {}                                                 // ROR 0x0028c650 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00174dd8
    virtual int  liveMain();                                                // ROR 0x00174e9c
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00174eb0
    virtual void liveActivateExec();                                        // ROR 0x0017512c

    u_char m_body[0x4c];                                                    // ROR 0x24..0x70: not decoded yet
};

/* kind 0x22; EE 0x80, vtable 0x2e4050. layout: host. */
class EffLight2 : public Eff0
{
public:
    EffLight2() {}                                                          // ROR 0x0028c7e0 (linkonce; inlined in createEff)
    virtual ~EffLight2() {}                                                 // ROR 0x0028c730 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00175170
    virtual int  liveMain();                                                // ROR 0x00175254
    virtual void render(RENDERPARAM *param);                                // ROR 0x00175268
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00175378
    virtual void liveActivateExec();                                        // ROR 0x001755e8

    u_char m_body[0x5c];                                                    // ROR 0x24..0x80: not decoded yet
};

/* kind 0x23; EE 0x3a0, vtable 0x2e3fe0. layout: host. */
class EffLight3 : public Eff0
{
public:
    EffLight3();                                                            // ROR 0x0017562c
    virtual ~EffLight3();                                                   // ROR 0x00175664
    virtual int  init(const EffParam *param);                               // ROR 0x00175804
    virtual int  liveMain();                                                // ROR 0x00175dc4
    virtual void renderPreMain(RENDERPARAM *param);                         // ROR 0x0028c868 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028c870 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *layer);                 // ROR 0x0028c860 (linkonce)

    u_char m_body[0x37c];                                                   // ROR 0x24..0x3a0: not decoded yet
};

/* kind 0x26; EE 0x70, vtable 0x2e41a0. layout: host. */
class EffMuzzleFlash1 : public Eff0
{
public:
    EffMuzzleFlash1() {}                                                    // ROR 0x0028ca08 (linkonce; inlined in createEff)
    virtual ~EffMuzzleFlash1() {}                                           // ROR 0x0028c958 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x001788ec
    virtual int  liveMain();                                                // ROR 0x00178ac0
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00178c84
    virtual void updateZDepth();                                            // ROR 0x001790a4

    u_char m_body[0x4c];                                                    // ROR 0x24..0x70: not decoded yet
};

/* kind 0x27; EE 0x90, vtable 0x2e4130. layout: host. */
class EffReload1 : public Eff0
{
public:
    EffReload1();                                                           // ROR 0x001790e4
    virtual ~EffReload1();                                                  // ROR 0x0017911c
    virtual int  init(const EffParam *param);                               // ROR 0x001791ac
    virtual int  liveMain();                                                // ROR 0x00179324
    virtual void renderPreMain(RENDERPARAM *param);                         // ROR 0x00179390
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028ca88 (linkonce)

    u_char m_body[0x6c];                                                    // ROR 0x24..0x90: not decoded yet
};

/* kinds 0x29, 0x2d; EE 0x90, vtable 0x2e43d0. layout: host. */
class EffSteam1 : public Eff0
{
public:
    EffSteam1() {}                                                          // ROR 0x0028cb40 (linkonce; inlined in createEff)
    virtual ~EffSteam1() {}                                                 // ROR 0x0028ca90 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00179428
    virtual int  liveMain();                                                // ROR 0x0017955c
    virtual void render(RENDERPARAM *param);                                // ROR 0x0028cb78 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028cb80 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *layer);                 // ROR 0x0028cb70 (linkonce)

    u_char m_body[0x6c];                                                    // ROR 0x24..0x90: not decoded yet
};

/* kinds 0x2a, 0x2e; EE 0x80, vtable 0x2e4360. layout: host. */
class EffSteam1Prim : public Eff0
{
public:
    EffSteam1Prim() {}                                                      // ROR 0x0028cc38 (linkonce; inlined in createEff)
    virtual ~EffSteam1Prim() {}                                             // ROR 0x0028cb88 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x001797d8
    virtual int  liveMain();                                                // ROR 0x001798d8
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00179998
    virtual void updateZDepth();                                            // ROR 0x00179b5c

    u_char m_body[0x5c];                                                    // ROR 0x24..0x80: not decoded yet
};

/* kind 0x2b; EE 0x3c, vtable 0x2e42f0. layout: host. */
class EffSteam2 : public Eff0
{
public:
    EffSteam2() {}                                                          // ROR 0x0028cd18 (linkonce; inlined in createEff)
    virtual ~EffSteam2() {}                                                 // ROR 0x0028cc68 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00179b9c
    virtual int  liveMain();                                                // ROR 0x00179c30
    virtual void render(RENDERPARAM *param);                                // ROR 0x0028cd50 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028cd58 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *layer);                 // ROR 0x0028cd48 (linkonce)

    u_char m_body[0x18];                                                    // ROR 0x24..0x3c: not decoded yet
};

/* kind 0x2c; EE 0x50, vtable 0x2e4280. layout: host. */
class EffSteam2Prim : public Eff0
{
public:
    EffSteam2Prim() {}                                                      // ROR 0x0028ce10 (linkonce; inlined in createEff)
    virtual ~EffSteam2Prim() {}                                             // ROR 0x0028cd60 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00179dfc
    virtual int  liveMain();                                                // ROR 0x00179e70
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00179ed0
    virtual void updateZDepth();                                            // ROR 0x0017a2bc

    u_char m_body[0x2c];                                                    // ROR 0x24..0x50: not decoded yet
};

/* kind 0x11; EE 0x80, vtable 0x2e4210. layout: host. */
class EffCloud1 : public Eff0
{
public:
    EffCloud1() {}                                                          // ROR 0x0028cef0 (linkonce; inlined in createEff)
    virtual ~EffCloud1() {}                                                 // ROR 0x0028ce40 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0017a2fc
    virtual int  liveMain();                                                // ROR 0x0017a534
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0017a5ec
    virtual void updateZDepth();                                            // ROR 0x0017a8d8

    u_char m_body[0x5c];                                                    // ROR 0x24..0x80: not decoded yet
};

/* kind 0x2f; EE 0x90, vtable 0x2e44b0. layout: host. */
class EffWater1 : public Eff0
{
public:
    EffWater1();                                                            // ROR 0x0017a918
    virtual ~EffWater1();                                                   // ROR 0x0017a94c
    virtual int  init(const EffParam *param);                               // ROR 0x0017a9c8
    virtual int  liveMain();                                                // ROR 0x0017ac5c
    virtual void renderPreMain(RENDERPARAM *param);                         // ROR 0x0017ae04
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0028cf70 (linkonce)
    virtual void liveActivateExec();                                        // ROR 0x0017adc0

    u_char m_body[0x6c];                                                    // ROR 0x24..0x90: not decoded yet
};

/* kind 0x31; EE 0x60, vtable 0x2e4440. layout: host. */
class EffWater3 : public Eff0
{
public:
    EffWater3();                                                            // ROR 0x0017afd8
    virtual ~EffWater3();                                                   // ROR 0x0017b010
    virtual int  init(const EffParam *param);                               // ROR 0x0017b0a4
    virtual int  liveMain();                                                // ROR 0x0017b5e0
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0017b6d4

    u_char m_body[0x3c];                                                    // ROR 0x24..0x60: not decoded yet
};

#endif /* _EFF_EFFCLASSES_H */
