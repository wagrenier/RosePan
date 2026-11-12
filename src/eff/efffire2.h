/* ==========================================================================
 *  eff/efffire2.h
 *
 *  EffFire2 (kind 0x16), code 0x17145c..0x1716bc (after eff/efffire1.cpp,
 *  before eff/efffog1.cpp; no source string names the file -- one file per
 *  numbered effect family, as eff/efffog3.cpp): a fire sprite placed in
 *  the world, not facing the camera -- rotated by the param's z x y
 *  angles, m_scale wide and high -- cycling the four quarters of texture 0
 *  every 4 frames, white with alpha m_alpha (ALPHA_1 0x44). It never ends
 *  by itself.
 * ======================================================================== */
#ifndef _EFF_EFFFIRE2_H
#define _EFF_EFFFIRE2_H

#include "eff/eff0.h"
#include "sdk/libvu0.h"

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

    int           m_frame;      // ROR 0x24: frames since init
    float         m_alpha;      // ROR 0x28
    u_char        _2c[0x30 - 0x2c];
    sceVu0FMATRIX m_matrix;     // ROR 0x30: rotation times scale, no translation
    sceVu0FVECTOR m_pos;        // ROR 0x60: w 1 on the host (the ROM leaves it unset)
};

#endif /* _EFF_EFFFIRE2_H */
