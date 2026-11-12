/* ==========================================================================
 *  eff/effdust2.h
 *
 *  The rising dust, code 0x1704a4..0x170ca4 (after eff/effcommon.cpp,
 *  before eff/efffire1.cpp; no source string names the file -- one file
 *  per numbered effect family, as eff/efffog3.cpp):
 *  - EffDust2 (kind 0x12): keeps 12 EffDust2Prim alive along the segment
 *    m_from..m_to. init starts them 8 frames apart; each frame one that
 *    has ended is replaced at once. A puff starts at a point of the
 *    segment (the points step 3/8 of it each time, plus up to 1/8 at
 *    random) scattered a random direction by up to m_spread0..m_spread1
 *    (the radius grows along the segment), its alpha times 0.2..1 the
 *    nearer it lies to m_from. It never ends by itself.
 *  - EffDust2Prim (kind 0x13): a camera-facing sprite m_size wide that
 *    rises 0.025 a frame (toward -y) for 150 frames, turning 0.06 degrees
 *    a frame, its alpha keyed 0, 0.8, 1, 0.8, 0 every 30 frames and 10%
 *    lower in every other 4-frame run (a flicker).
 * ======================================================================== */
#ifndef _EFF_EFFDUST2_H
#define _EFF_EFFDUST2_H

#include "eff/eff0.h"
#include "sdk/libvu0.h"

/* kind 0x12; EE 0xc0, vtable 0x2e38e0. layout: host. */
class EffDust2 : public Eff0
{
public:
    EffDust2() {}                                                           // ROR 0x0028b950 (linkonce; inlined in createEff)
    virtual ~EffDust2() {}                                                  // ROR 0x0028b8a0 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x001704a4
    virtual int  liveMain();                                                // ROR 0x00170594
    virtual void render(RENDERPARAM *) {}                                   // ROR 0x0028b988 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *, const I3dDrawSet *, u_long *) { return NULL; } // ROR 0x0028b990 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *) { return NULL; }      // ROR 0x0028b980 (linkonce)

    /* A new puff starting after delay frames; its effect number. */
    int createPrim(int delay);                                              // ROR 0x00170624

    u_char        _24[0x30 - 0x24];
    sceVu0FVECTOR m_from;       // ROR 0x30: w 1
    float         m_spread0;    // ROR 0x40: the scatter radius at m_from
    u_char        _44[0x50 - 0x44];
    sceVu0FVECTOR m_to;         // ROR 0x50: w 1
    float         m_spread1;    // ROR 0x60: at m_to
    u_char        _64[0x70 - 0x64];
    sceVu0FVECTOR m_color;      // ROR 0x70: the puffs' colour
    float         m_size;       // ROR 0x80: the puffs' size
    u_short       m_step;       // ROR 0x84: +3 per puff; its low 3 bits pick the eighth of the segment
    int           m_no[12];     // ROR 0x88: the puffs' effect numbers
    u_char        _b8[0xc0 - 0xb8];
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

    u_char        _24[0x30 - 0x24];
    sceVu0FVECTOR m_pos;        // ROR 0x30: w 1
    float         m_size;       // ROR 0x40
    u_char        _44[0x50 - 0x44];
    sceVu0FVECTOR m_color;      // ROR 0x50: 0..1, alpha last
    int           m_frame;      // ROR 0x60: frames since init
    u_char        _64[0x70 - 0x64];
};

#endif /* _EFF_EFFDUST2_H */
