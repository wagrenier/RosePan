/* ==========================================================================
 *  mag/maglifenoise.h
 *
 *  The screen noise (mag/maglifenoise.cpp, pinned 0x130a2c..0x130ee8):
 *  MagLifeNoise <- MagLife (vtable _vt$12MagLifeNoise 0x2e1e20, vptr at
 *  +0x14), the scene's +0x418f0 (MagLifeMan key 0x8004). While m_volume is
 *  set it lays four full-screen sprites of the scene's common texture 1
 *  over the frame, each with a random UV window (sizes 400, 600, 888,
 *  1000) and an alpha that grows with m_volume -- a grain whose strength
 *  GamMan::setVolume drives (its scaled volume, 0 when switched off).
 *
 *  EE virtual slots: 1 ~MagLifeNoise (linkonce 0x2852f4), 2 MagLife::live,
 *  3 MagLife0::attached, 4 MagLife0::getLevel, 5 getName (linkonce
 *  0x2853bc, NULL), 6 render, 7 MagLife::setHide, 8 MagLife::getHide.
 * ======================================================================== */
#ifndef _MAG_MAGLIFENOISE_H
#define _MAG_MAGLIFENOISE_H

#include <stddef.h>

#include "mag/maglife.h"

struct I3dLayer;

/* The frame rect render hands the packet writers (EE 0x1c, a stack
 * record): GS pixels with the draw offset; the writers shift them to 12.4. */
struct MagLifeNoiseRect
{
    int m_x;                    // ROR 0x00: 0x700 (the frame's left edge with XYOFFSET)
    int m_y;                    // ROR 0x04: 0x720
    int _08;                    // ROR 0x08: 0, not read
    int _0c;                    // ROR 0x0c: 0, not read
    int m_w;                    // ROR 0x10: 0x200
    int m_h;                    // ROR 0x14: 0x1c0
    int m_volume;               // ROR 0x18: MagLifeNoise::m_volume
};
static_assert(offsetof(MagLifeNoiseRect, m_w) == 0x10, "m_w");
static_assert(offsetof(MagLifeNoiseRect, m_volume) == 0x18, "m_volume");
static_assert(sizeof(MagLifeNoiseRect) == 0x1c, "MagLifeNoiseRect is 0x1c bytes on the EE");

/* EE 0x38 bytes. layout: host -- polymorphic (EE vptr at +0x14). */
struct MagLifeNoise : MagLife
{
    MagLifeNoise();                                                      // ROR 0x00130da4
    /* linkonce: m_tex_no = tex_no. */
    MagLifeNoise(int tex_no);                                            // ROR 0x00285378
    /* EE slot 5 (+0x28), linkonce. */
    const char *getName() { return NULL; }                               // ROR 0x002853bc
    /* EE slot 6 (+0x30): the four noise sprites while m_volume is set. */
    void render(I3dLayer *layer) override;                               // ROR 0x00130de8

    /* linkonce */
    void init(int tex_no) { m_tex_no = tex_no; }                         // ROR 0x002853b4
    void setVolume(int volume);                                          // ROR 0x00130de0

    /* The four layers over rect (nothing at volume 0); the position after. */
    u_long *writeNoise(u_long *pkt, const MagLifeNoiseRect *rect);       // ROR 0x00130c34
    /* One layer: ALPHA_1 with FIX = fix (mode 1: Cd + (Cd - Cs) * fix,
     * 0: the blend toward Cs), a sprite over rect sampling a w x h window
     * at a random offset. */
    u_long *writeNoiseLayer(u_long *pkt, const MagLifeNoiseRect *rect, int w, int h,
                            int fix, int mode);                          // ROR 0x00130a2c

    u_char _28[0x30 - 0x28];
    int    m_volume;            // ROR 0x30: the noise strength (GamMan::setVolume), 0 = nothing drawn
    int    m_tex_no;            // ROR 0x34: the noise texture (MagScene::initRender: common texture 1)
};
static_assert(offsetof(MagLifeNoise, m_volume) >= 0x30, "m_volume");
static_assert(offsetof(MagLifeNoise, m_tex_no) >= 0x34, "m_tex_no");

#endif /* _MAG_MAGLIFENOISE_H */
