/* ==========================================================================
 *  eff/effsteam.h
 *
 *  The vapour effects, code 0x179428..0x17a918 (between EffReload1 and
 *  EffWater1; no source string names the file -- EffCloud1 follows the
 *  steam classes against the alphabetical link order, so it is taken to
 *  share their file).
 *
 *  - EffSteam1 (kinds 0x29 / 0x2d): a vent. Over a period of m_frames +
 *    m_pause frames (a random phase at the start) it puffs one EffSteam1Prim
 *    every third frame of the first m_frames, along its z axis turned by a
 *    random roll and up to 15 degrees of yaw; the later in the burst, the
 *    quicker the puff fades. Nothing while hidden.
 *  - EffSteam1Prim (kinds 0x2a / 0x2e): one puff, a camera-facing sprite
 *    flying along its direction (slowing x 0.95 a frame) and growing 4.5 a
 *    frame; its alpha rises 0.03 a frame from 0.4 to 1, then falls by
 *    m_fade x 0.07 a frame until it is gone.
 *  - EffSteam2 (kind 0x2b): a rising mist. Every 180..210 frames (a random
 *    first wait of up to 150) one EffSteam2Prim within m_radius / 2 of the
 *    centre, m_radius / 2 toward -y.
 *  - EffSteam2Prim (kind 0x2c): one wisp for 150 frames: a camera-facing
 *    quad turning slowly, moving m_size / 300 a frame toward -y, fading in
 *    and out over 15 frames, its four corners' alphas cycling through the
 *    table s_wispAlpha with a phase each.
 *  - EffCloud1 (kind 0x11): a cloud sprite drifting m_speed a frame along
 *    a y angle for m_dist, fading in and out over 60 frames and with the
 *    fog's distance, then (m_frames % 30 frames later) starting again from
 *    a random point within m_size / 2 of its origin.
 *
 *  The sprite classes draw texture 0 of their pack with ALPHA_1 0x44 (Cs
 *  blended by As over Cd), coloured by the scene's ambient light 0.
 * ======================================================================== */
#ifndef _EFF_EFFSTEAM_H
#define _EFF_EFFSTEAM_H

#include "eff/eff0.h"
#include "sdk/libvu0.h"

/* kinds 0x29, 0x2d; EE 0x90, vtable 0x2e43d0. layout: host. */
class EffSteam1 : public Eff0
{
public:
    EffSteam1() {}                                                          // ROR 0x0028cb40 (linkonce; inlined in createEff)
    virtual ~EffSteam1() {}                                                 // ROR 0x0028ca90 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00179428
    virtual int  liveMain();                                                // ROR 0x0017955c
    virtual void render(RENDERPARAM *param) {}                              // ROR 0x0028cb78 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt) { return NULL; } // ROR 0x0028cb80 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *layer) { return NULL; } // ROR 0x0028cb70 (linkonce)

    int           m_frame;      // ROR 0x24: frame of the period, 0..m_period - 1
    int           m_frames;     // ROR 0x28: puffing frames of each period (>= 0)
    int           m_period;     // ROR 0x2c: m_frames + the pause (>= 1)
    sceVu0FMATRIX m_mat;        // ROR 0x30: the vent's rotation (x then y), row 3 its position
    float         m_size;       // ROR 0x70: EffParam m_steam1.m_size
    float         m_speed;      // ROR 0x74
    float         m_color[4];   // ROR 0x78
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

    u_char        _24[0x30 - 0x24];
    sceVu0FVECTOR m_pos;        // ROR 0x30: w 1
    sceVu0FVECTOR m_vel;        // ROR 0x40: per frame, w 0
    float         _50;
    float         m_size;       // ROR 0x54: 20 at the start, + 4.5 a frame
    float         m_scale;      // ROR 0x58: the sprite is m_size * m_scale wide and high
    float         m_alpha;      // ROR 0x5c: 0.4 at the start; gone at 0
    float         m_alpha_add;  // ROR 0x60: 0.03 rising, -0.07 * m_fade falling
    float         m_fade;       // ROR 0x64
    float         m_color[4];   // ROR 0x68: alpha times m_alpha
};

/* kind 0x2b; EE 0x3c, vtable 0x2e42f0. layout: host. */
class EffSteam2 : public Eff0
{
public:
    EffSteam2() {}                                                          // ROR 0x0028cd18 (linkonce; inlined in createEff)
    virtual ~EffSteam2() {}                                                 // ROR 0x0028cc68 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00179b9c
    virtual int  liveMain();                                                // ROR 0x00179c30
    virtual void render(RENDERPARAM *param) {}                              // ROR 0x0028cd50 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt) { return NULL; } // ROR 0x0028cd58 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *layer) { return NULL; } // ROR 0x0028cd48 (linkonce)

    float  m_pos[3];            // ROR 0x24
    float  m_radius;            // ROR 0x30
    float  m_alpha;             // ROR 0x34
    u_char m_wait;              // ROR 0x38: frames to the next wisp
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

    u_char        _24[0x30 - 0x24];
    sceVu0FVECTOR m_pos;        // ROR 0x30: w 1
    float         m_size;       // ROR 0x40
    float         m_alpha;      // ROR 0x44
    u_char        m_age;        // ROR 0x48: frames lived, over at 150
};

/* kind 0x11; EE 0x80, vtable 0x2e4210. layout: host.
 * Eff0::m_0c is its state (0 drifting, 10 waiting) and m_08 last frame's
 * (copied at the top of liveMain; it draws only when that was 0). */
class EffCloud1 : public Eff0
{
public:
    EffCloud1() {}                                                          // ROR 0x0028cef0 (linkonce; inlined in createEff)
    virtual ~EffCloud1() {}                                                 // ROR 0x0028ce40 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0017a2fc
    virtual int  liveMain();                                                // ROR 0x0017a534
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0017a5ec
    virtual void updateZDepth();                                            // ROR 0x0017a8d8

    /* m_pos = m_origin + a random point within m_size / 2; a new m_spin. */
    void resetPos();                                                        // ROR 0x0017a410

    u_char        _24[0x30 - 0x24];
    sceVu0FVECTOR m_pos;        // ROR 0x30: w 1
    float         m_origin[3];  // ROR 0x40
    float         m_vel[3];     // ROR 0x4c: m_speed along the y angle
    float         m_size;       // ROR 0x58
    int           m_spin;       // ROR 0x5c: effRand(), the sprite's roll offset (frames)
    float         m_color[4];   // ROR 0x60
    int           m_frame;      // ROR 0x70: frames since the start
    int           m_frames;     // ROR 0x74: frames it drifts: m_dist / m_speed + 1
    int           m_wait;       // ROR 0x78: frame it starts again: m_frames + m_frames % 30
};

#endif /* _EFF_EFFSTEAM_H */
