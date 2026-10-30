/* ==========================================================================
 *  eff/efffoot.h
 *
 *  The footstep effects, code 0x172e4c..0x173df8 (after eff/efffog3.cpp,
 *  before eff/effgeneric.cpp; no source string names the file -- named
 *  after the classes, in the alphabetical Eff* link order):
 *  - EffFoot1 (kind 0x1d): a box on the floor. Each frame the PC's actor
 *    and the dog's (MagScene::getCurDog) are probed: an actor standing in
 *    the box (effProbeActorInBox, margin 100) whose toe / finger motion
 *    events fire over its next frame (effProbeActorLimbs) gets one
 *    EffFoot1Prim at each such limb. It never ends by itself.
 *  - EffFoot1Prim (kind 0x1e): m_mode 0 is a camera-facing puff that grows
 *    ((frame + 15) * m_scale), its centre half its size toward -y from the
 *    foot, and fades out over 20 frames; modes 1 and 2 end at once and draw nothing.
 *  - EffFoot2 (kind 0x1f, one at a time: EffMan::m_no_1f): footprints for
 *    the PC's actor and the dog's, a kind-0x20 EffFoot2Prim at each limb
 *    whose motion event fires, while the actor's work record (file statics
 *    0x828648 the PC, 0x828618 the dog) has prints left. init sets both
 *    counts to 0 and addPrint only decrements them: nothing else in the
 *    ROM is found writing them, so no print is ever made.
 *  - EffFoot2Prim (kind 0x20): a print lying flat on the floor, 2 toward
 *    -y from the given y, turned by the actor's y angle, faded in over 15 frames,
 *    kept to frame 165 (at once when it is finished early) and faded out
 *    by frame 180. EFF_FOOT2_DOG picks the paw (a square, texture v
 *    0.75..1) over the shoe (three times as long as wide, v 0..0.75).
 * ======================================================================== */
#ifndef _EFF_EFFFOOT_H
#define _EFF_EFFFOOT_H

#include <stddef.h>

#include "eff/eff0.h"
#include "sdk/libvu0.h"

struct DogActor;

/* EffFoot1Prim::m_mode: the puff; the other two end at once. */
#define EFF_FOOT1_PUFF   0

/* EffFoot2Prim::m_type bit: the dog's paw print (else the PC's shoe). */
#define EFF_FOOT2_DOG    0x10

/* kind 0x1d; EE 0x4c, vtable 0x2e3e90. layout: host. */
class EffFoot1 : public Eff0
{
public:
    EffFoot1() {}                                                           // ROR 0x0028c300 (linkonce; inlined in createEff)
    virtual ~EffFoot1() {}                                                  // ROR 0x0028c250 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00172e4c
    virtual int  liveMain();                                                // ROR 0x00172f4c
    virtual void render(RENDERPARAM *) {}                                   // ROR 0x0028c330 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *, const I3dDrawSet *, u_long *) { return NULL; } // ROR 0x0028c338 (linkonce)

    /* Puffs at the actor's limbs that step in the box. */
    void probeActor(DogActor *actor);                                       // ROR 0x001730d4

    float m_center[3];          // ROR 0x24: the box's centre
    float m_size[3];            // ROR 0x30: its sizes (handed to the probe as its half sizes)
    int   m_tex;                // ROR 0x3c: } EffFoot1Prim's m_tex, m_mode, m_scale, m_alpha
    int   m_mode;               // ROR 0x40: }
    float m_scale;              // ROR 0x44: }
    float m_alpha;              // ROR 0x48: }
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

    int     liveMode0();                                                    // ROR 0x00173380
    int     liveMode1();                                                    // ROR 0x00173610
    int     liveMode2();                                                    // ROR 0x00173620
    u_long *renderMode0(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x001733cc
    u_long *renderMode1(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00173618
    u_long *renderMode2(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00173628

    u_char        _24[0x30 - 0x24];
    sceVu0FVECTOR m_pos;        // ROR 0x30: the limb, w 1
    int           m_tex;        // ROR 0x40: the pack texture
    int           m_mode;       // ROR 0x44: EFF_FOOT1_PUFF, 1, 2
    int           m_frame;      // ROR 0x48: frames since init
    float         m_scale;      // ROR 0x4c: the puff's growth per frame
    float         m_alpha;      // ROR 0x50
    u_char        _54[0x60 - 0x54];
};

/* One actor's print state (file statics 0x828618 the dog, 0x828648 the
 * PC; 0x30 apart, only the first three words are read). */
struct EffFoot2Work
{
    u_int m_type;               // ROR 0x00: the prints' EffFoot2Prim::m_type (EFF_FOOT2_DOG for the dog)
    int   m_count;              // ROR 0x04: prints left (addPrint decrements it; init sets 0)
    float m_scale;              // ROR 0x08: the prints' size, 10
    u_char _0c[0x30 - 0x0c];
};
static_assert(offsetof(EffFoot2Work, m_count) == 0x04, "m_count");
static_assert(offsetof(EffFoot2Work, m_scale) == 0x08, "m_scale");
static_assert(sizeof(EffFoot2Work) == 0x30, "EffFoot2Work");

/* kind 0x1f; EE 0x24, vtable 0x2e3db0. layout: host. */
class EffFoot2 : public Eff0
{
public:
    EffFoot2() {}                                                           // ROR 0x0028c4d0 (linkonce; inlined in createEff)
    virtual ~EffFoot2() {}                                                  // ROR 0x0028c420 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00173630
    virtual int  liveMain();                                                // ROR 0x00173688
    virtual void render(RENDERPARAM *) {}                                   // ROR 0x0028c508 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *, const I3dDrawSet *, u_long *) { return NULL; } // ROR 0x0028c510 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *) { return NULL; }      // ROR 0x0028c500 (linkonce)

    /* A print at each limb whose motion event fires over the next frame. */
    void checkSteps(DogActor *actor, EffFoot2Work *work);                   // ROR 0x00173840
    /* A print at node name (y the actor's), one off work's count. */
    void addPrint(DogActor *actor, const char *name, EffFoot2Work *work);   // ROR 0x00173948
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

    u_int         m_type;       // ROR 0x24: EFF_FOOT2_DOG
    u_char        _28[0x30 - 0x28];
    sceVu0FVECTOR m_pos;        // ROR 0x30: y 2 less than given, w 1
    float         m_rot;        // ROR 0x40: the y angle
    float         m_scale;      // ROR 0x44: the width
    int           m_frame;      // ROR 0x48: frames since init
    u_char        _4c[0x50 - 0x4c];
};

#endif /* _EFF_EFFFOOT_H */
