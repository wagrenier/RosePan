/* ==========================================================================
 *  eff/efffire1.h
 *
 *  The candle flames, code 0x170ca4..0x17145c (after eff/effdust2.cpp,
 *  before eff/efffire2.cpp; no source string names the file -- one file
 *  per numbered effect family, as eff/efffog3.cpp):
 *  - EffFire1 (kind 0x14): an upright flame 5 x 10 standing 5 above
 *    m_pos (texture 0, 8 frames of a 4 x 2 sheet, 6 game frames each,
 *    ALPHA_1 0x44) and a camera-facing glow 100 wide 2 above it (texture
 *    1, ALPHA_1 0x48) pulled m_near + 10 toward the camera, m_color's
 *    colour, its alpha flickering between 0.85 and 1: m_flicker drops 0.25
 *    a frame and jumps 0.75 where a 96-bit pattern (one bit per 2 frames)
 *    has a 1. setVisible fades the flame in or out over 5 frames
 *    (liveActivateExec); hidden it draws nothing. It never ends by itself.
 *  - EffFire1Ex (kind 0x15, native 281 op 38): EffFire1 at an actor's
 *    node (the world position read again for each depth sort), no pull.
 * ======================================================================== */
#ifndef _EFF_EFFFIRE1_H
#define _EFF_EFFFIRE1_H

#include "eff/eff0.h"
#include "sdk/libvu0.h"

struct DogActor;

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

    u_char        _24[0x30 - 0x24];
    sceVu0FVECTOR m_pos;        // ROR 0x30: the flame's foot, w 1
    sceVu0FVECTOR m_color;      // ROR 0x40: the glow's colour, 0..1, alpha last
    int           m_frame;      // ROR 0x50: frames (from a random 0..255 for EffFire1, 0 for Ex)
    float         m_flicker;    // ROR 0x54: 0..1, the glow's alpha 0.85..1
    float         m_near;       // ROR 0x58: the glow's pull toward the camera, past 10
    int           m_fade;       // ROR 0x5c: the fade in / out counter, 0..5
};

/* kind 0x15; EE 0x90, vtable 0x2e39c0. layout: host. */
class EffFire1Ex : public EffFire1
{
public:
    EffFire1Ex() {}                                                         // ROR 0x0028bc08 (linkonce; inlined in createEff)
    virtual ~EffFire1Ex() {}                                                // ROR 0x0028bb58 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x001712bc
    virtual void updateZDepth();                                            // ROR 0x00171378

    DogActor *m_actor;          // ROR 0x60
    char      m_node[0x20];     // ROR 0x64: the node name
    u_char    _84[0x90 - 0x84];
};

#endif /* _EFF_EFFFIRE1_H */
