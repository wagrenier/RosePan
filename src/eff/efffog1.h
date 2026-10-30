/* ==========================================================================
 *  eff/efffog1.h
 *
 *  EffFog1 (kind 0x17), code 0x1716bc..0x171bfc (after eff/efffire2.cpp,
 *  before eff/efffog2.cpp; no source string names the file -- one file per
 *  numbered effect family, as eff/efffog3.cpp): a camera-facing fog bank
 *  m_scale wide and high that drifts m_speed a frame along the camera's
 *  x axis (moved in renderMain) and goes back to its start every 540
 *  frames (from a random phase). Its four corners take the fog and
 *  ambient tint of its depth; their alphas follow a 9-key cycle (one key
 *  every 60 frames: 0, 0, 0.5, 1, 0.6, 1, 0.6, 1, 0.5, 0) times m_alpha,
 *  the left corners 45 frames ahead of the right ones, so it fades in
 *  and out as it goes. Texture 0, ALPHA_1 0x44. It never ends by itself.
 * ======================================================================== */
#ifndef _EFF_EFFFOG1_H
#define _EFF_EFFFOG1_H

#include "eff/eff0.h"
#include "sdk/libvu0.h"

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

    u_char        _24[0x30 - 0x24];
    sceVu0FVECTOR m_pos;        // ROR 0x30: the centre now, w 1
    sceVu0FVECTOR m_start;      // ROR 0x40: where it goes back to
    sceVu0FVECTOR m_scale;      // ROR 0x50: width, height, 1, 1
    float         m_alpha;      // ROR 0x60
    float         m_speed;      // ROR 0x64: the drift per frame
    int           m_frame;      // ROR 0x68: 0..539
    u_char        _6c[0x70 - 0x6c];
};

#endif /* _EFF_EFFFOG1_H */
