/* ==========================================================================
 *  eff/efffog2.h
 *
 *  The creeping ground fog, code 0x171bfc..0x172678 (after
 *  eff/efffog1.cpp, before eff/efffog3.cpp; no source string names the
 *  file -- one file per numbered effect family, as eff/efffog3.cpp):
 *  - EffFog2 (kind 0x18): every 30 frames (from its first) an
 *    EffFog2Prim at a random point of a line m_width long through m_pos
 *    (along the x axis of its y angle m_rot), heading m_rot turned by up
 *    to +-45 degrees, going m_dist in all, colour m_color. It never ends
 *    by itself.
 *  - EffFog2Prim (kind 0x19): a camera-facing wisp 60..100 wide (random)
 *    that rises from its start -- y below 0, -y up -- for m_rise frames
 *    (1.5 a unit of depth) on a quarter-circle to y -0.4 of its size,
 *    moving out along its heading by (0.2 size + 15) % of the depth
 *    (1 - cos out, sin up), then glides flat on at the same speed until
 *    it has gone m_dist; it fades in over its first 30 frames and out over
 *    its last 30, each corner's alpha cycling 0, 1, 1, 1 (a key every 20
 *    frames, corner i 20 frames on from corner i - 1), and it rolls
 *    pi / 600 a frame. Eff0's m_0c is its state (0 rising, 10 gliding),
 *    m_08 the last frame's.
 * ======================================================================== */
#ifndef _EFF_EFFFOG2_H
#define _EFF_EFFFOG2_H

#include "eff/eff0.h"
#include "sdk/libvu0.h"

/* kind 0x18; EE 0x50, vtable 0x2e3c60. layout: host. */
class EffFog2 : public Eff0
{
public:
    EffFog2() {}                                                            // ROR 0x0028bea8 (linkonce; inlined in createEff)
    virtual ~EffFog2() {}                                                   // ROR 0x0028bdf8 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00171bfc
    virtual int  liveMain();                                                // ROR 0x00171c94
    virtual void render(RENDERPARAM *) {}                                   // ROR 0x0028bee0 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *, const I3dDrawSet *, u_long *) { return NULL; } // ROR 0x0028bee8 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *) { return NULL; }      // ROR 0x0028bed8 (linkonce)

    int   m_frame;              // ROR 0x24: frames since init (-1 at init)
    float m_pos[3];             // ROR 0x28: the line's centre
    float m_rot;                // ROR 0x34: the y angle, -pi..pi
    float m_width;              // ROR 0x38: the line's length
    float m_dist;               // ROR 0x3c: the wisps' m_dist
    float m_color[4];           // ROR 0x40: the wisps' colour
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

    u_char        _24[0x30 - 0x24];
    sceVu0FVECTOR m_pos;        // ROR 0x30: now, w 1
    float         m_start[3];   // ROR 0x40: the start; once gliding, the move per frame (y 0)
    float         m_to[3];      // ROR 0x4c: the top of the rise
    float         m_size;       // ROR 0x58: 60..100
    float         m_speed;      // ROR 0x5c: the rise's straight length / m_rise, the glide's step
    sceVu0FVECTOR m_color;      // ROR 0x60: 0..1, alpha last
    int           m_frame;      // ROR 0x70: frames since init
    int           m_rise;       // ROR 0x74: frames rising
    int           m_frames;     // ROR 0x78: frames in all
    u_char        _7c[0x80 - 0x7c];
};

#endif /* _EFF_EFFFOG2_H */
