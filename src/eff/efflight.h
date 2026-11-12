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
 *  EffLight3 is lightning: no picture of its own, it flashes the sky
 *  model's light3_%02d nodes and the scene's lights (see the class).
 * ======================================================================== */
#ifndef _EFF_EFFLIGHT_H
#define _EFF_EFFLIGHT_H

#include "dog/doglightpool.h"
#include "dog/dogmodel.h"
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

/* EffLight3's light table: per light kind, the scene lights it copies
 * (m_src: the bit of light set 1's mask) and the light MagScene::addLight
 * gave it for each (m_id), ended by m_src / m_id -1 (init clears it). */
struct EffLight3Copy
{
    int m_src;                  // ROR 0x00
    int m_id;                   // ROR 0x04
};
static_assert(sizeof(EffLight3Copy) == 8, "EffLight3Copy is 8 bytes on the EE");

/* kind 0x23; EE 0x3a0, vtable 0x2e3fe0.
 *
 * Lightning: draws nothing (its render virtuals are empty). Every 90..210
 * frames (16..136 for the first) it plays SE 0xb030352 and picks a 4-bit
 * pattern (13, 3, 11 or 7); in the next 16 frames, every 4th frame whose
 * bit is set starts a flash: 5 frames that brighten the sky model's
 * light3_%02d nodes (m_disp) and, by mode (EffParam word 0x10):
 *  - mode 0: copies of light set 1's direct, point and spot lights at up
 *    to 2x / 4x their colour (black between flashes), and an ambient that
 *    drops to 0.3 of the scene's and comes back to 0.9 over 30 frames;
 *  - mode 1: the ambient at up to 2x during the flash (1x at rest).
 * The destructor gives the scene back its ambient and removes the copies.
 * layout: host. */
class EffLight3 : public Eff0
{
public:
    EffLight3();                                                            // ROR 0x0017562c
    virtual ~EffLight3();                                                   // ROR 0x00175664
    virtual int  init(const EffParam *param);                               // ROR 0x00175804
    virtual int  liveMain();                                                // ROR 0x00175dc4
    virtual void renderPreMain(RENDERPARAM *param) {}                       // ROR 0x0028c868 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt) { return NULL; } // ROR 0x0028c870 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *layer) { return NULL; } // ROR 0x0028c860 (linkonce)

    /* Light set 1's lights of kind type copied into m_copy[type]. */
    void addLights(int type);                                               // ROR 0x00176430
    /* Flash pattern n (0..3): 13, 3, 11, 7. */
    u_int getFlashPattern(int n);                                           // ROR 0x001764fc
    /* The sky model's light3_00, light3_01 ... nodes display by m_disp. */
    /* PORT: the ROM always points them at m_disp; the host destructor
     * passes a static copy instead of the dying object (efflight.cpp). */
    void setSkyNodeDisp(const DogDispControl *disp);                        // ROR 0x0017653c
    /* The scene's ambient light 0 = m_ambient times scale. */
    void setAmbientScale(float scale);                                      // ROR 0x001765e8

    EffLight3Copy  m_copy[DOG_MAX_LIGHT_TYPE][16]; // ROR 0x024: per light kind (only DIRECT, POINT, SPOT filled)
    u_char         _324[0x330 - 0x324];
    sceVu0FVECTOR  m_ambient;       // ROR 0x330: the scene's ambient light 0 colour at init
    DogDispControl m_disp;          // ROR 0x340: the sky nodes' display (m_color the flash, m_10 1 while one runs)
    u_int          m_pattern;       // ROR 0x370: getFlashPattern's bits
    int            m_pattern_bit;   // ROR 0x374: the next bit of m_pattern
    int            m_flash;         // ROR 0x378: 5..0 while a flash runs, -1 none
    int            m_ambient_fade;  // ROR 0x37c: 30..0 after a flash (mode 0), -1 none
    float          m_ambient_rest;  // ROR 0x380: the ambient scale at rest (0.9; mode 1: 1)
    float          m_ambient_flash; // ROR 0x384: at a flash's start (0.3; mode 1: 2)
    int            m_timer;         // ROR 0x388: frames since the last thunder
    int            m_interval;      // ROR 0x38c: frames to the next one
    int            m_mode;          // ROR 0x390: EffParam word 0x10, -1 = not initialised
};

#endif /* _EFF_EFFLIGHT_H */
