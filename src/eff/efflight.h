/* ==========================================================================
 *  eff/efflight.h
 *
 *  EffLight1 (kind 0x21), EffLight2 (0x22) and EffLight3 (0x23), code
 *  0x174dd8..0x176694, between EffGeneric1's helpers and EffFindItem1.
 *
 *  EffLight1 and EffLight2 are glows: one camera-facing sprite in the
 *  always-pass layer's blend (ALPHA_1 0x48), its colour and size from the
 *  param, fogged when the param asks (EFF_LIGHT_FOG). EffLight1 is pulled
 *  m_near toward the eye and flickers (alpha 64/128 or 72/128, switching
 *  every 3 frames); EffLight2 sits at its position, flickers less (0.5 or
 *  0.525 of its alpha every 4 frames), fades out with the square of its
 *  distance from the eye between m_fade_near and m_fade_far and is not
 *  drawn past that, into the always-pass layer (render).
 *
 *  Both fade through Eff0::setVisible over 5 frames (liveActivateExec):
 *  m_fade counts up to 5 while hidden and down to 0 while shown, and the
 *  alpha is m_fade / 5 while the activation runs -- the ROM's direction,
 *  ported as it is.
 *
 *  EffLight3 (the light3_%02d nodes and the scene's DogLightPool) is not
 *  written yet: its overrides are stubs and its members an undecoded block.
 * ======================================================================== */
#ifndef _EFF_EFFLIGHT_H
#define _EFF_EFFLIGHT_H

#include "eff/eff0.h"
#include "sdk/libvu0.h"

/* Eff0::m_flags bit the EffLight classes add. */
enum
{
    EFF_LIGHT_FOG = 0x8000,     /* tint by effFogColor at its depth */
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

    sceVu0FVECTOR m_pos;        // ROR 0x30: w 1
    float         m_scale[2];   // ROR 0x40: width, height
    sceVu0FVECTOR m_color;      // ROR 0x50: 0..1
    float         m_near;       // ROR 0x60: the pull toward the camera
    u_int         m_frame;      // ROR 0x64: frames lived (the flicker)
    int           m_fade;       // ROR 0x68: 0..5, the activation fade
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

    sceVu0FVECTOR m_pos;        // ROR 0x30: w 1
    sceVu0FVECTOR m_scale;      // ROR 0x40: width, height, 1, 1 (i3dMatScale's vector)
    sceVu0FVECTOR m_color;      // ROR 0x50: 0..1
    float         m_alpha;      // ROR 0x60: m_color's alpha, before the flicker and fade
    float         m_fade_near;  // ROR 0x64
    float         m_fade_far;   // ROR 0x68
    float         m_dist;       // ROR 0x6c: from the eye, set by render
    int           m_tex;        // ROR 0x70: the pack texture
    u_int         m_frame;      // ROR 0x74: frames lived (the flicker)
    int           m_fade;       // ROR 0x78: 0..5, the activation fade
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

#endif /* _EFF_EFFLIGHT_H */
