/* ==========================================================================
 *  eff/effmuzzleflash.h
 *
 *  EffMuzzleFlash1 (kind 0x26), code 0x1788ec..0x1790e4 (after
 *  eff/effman.cpp, before EffReload1; no source string names the file --
 *  named after the class, in the alphabetical Eff* link order).
 *
 *  A gun's shot (MagLifePC::createMuzzleEff, at the gun's gun_mazzle node):
 *  two camera-facing sprites at the muzzle and a point light.
 *  - The flash (texture 0, ALPHA_1 0x48: additive): size 10, growing 20 a
 *    frame; alpha 0x7f00 / 32768 falling by a velocity that starts at
 *    -0x3200 and changes by -0x180 a frame (gone in 3 frames). It lights a
 *    point light (MagScene::addLight kind 2, sets 0 / 1) at the muzzle
 *    whose grey is alpha x 150 / 32768; the light is removed once the
 *    flash is gone.
 *  - The smoke (texture 1, ALPHA_1 0x44): size 40, growing 2.5 a frame,
 *    rising 2.25 a frame (y points down) and turning 1.5 degrees a frame,
 *    pushed one unit along the camera's z axis; alpha 0x6000 / 32768
 *    falling by a velocity from -0x700, +0x40 a frame (about 30 frames).
 *  Both start at the same random roll (17 / 20 degrees times effRand() %
 *  18). The two are drawn far to near. The effect is over when both
 *  alphas are below 0x100.
 * ======================================================================== */
#ifndef _EFF_EFFMUZZLEFLASH_H
#define _EFF_EFFMUZZLEFLASH_H

#include <stddef.h>

#include "eff/eff0.h"
#include "sdk/libvu0.h"

struct DogCamera;

/* One of the two sprites (EE 0x10). */
struct EffMuzzlePart
{
    float m_rot;                // ROR 0x00: the roll, radians
    float m_size;               // ROR 0x04: the sprite's width and height
    float m_yoff;               // ROR 0x08: added to the position's y (the smoke's rise)
    short m_alpha;              // ROR 0x0c: / 32768; drawn while >= 0x100
    short m_alpha_vel;          // ROR 0x0e: added to m_alpha each frame
};
static_assert(offsetof(EffMuzzlePart, m_yoff) == 0x08, "EffMuzzlePart::m_yoff");
static_assert(offsetof(EffMuzzlePart, m_alpha) == 0x0c, "EffMuzzlePart::m_alpha");
static_assert(offsetof(EffMuzzlePart, m_alpha_vel) == 0x0e, "EffMuzzlePart::m_alpha_vel");
static_assert(sizeof(EffMuzzlePart) == 0x10, "EffMuzzlePart is 0x10 bytes on the EE");

/* What drawSmoke / drawFlash hand drawPart (their stack, EE 0x18).
 * layout: host -- pointers. */
struct EffMuzzleDraw
{
    EffMuzzlePart    *m_part;   // ROR 0x00
    DogCamera        *m_cam;    // ROR 0x04: the scene's camera
    const I3dDrawSet *m_set;    // ROR 0x08
    u_long           *m_pkt;    // ROR 0x0c: in and out
    float             m_push;   // ROR 0x10: along the camera's z axis (smoke 1, flash 0)
    short             m_tex;    // ROR 0x14: the pack texture (smoke 1, flash 0)
};

/* kind 0x26; EE 0x70, vtable 0x2e41a0. layout: host. */
class EffMuzzleFlash1 : public Eff0
{
public:
    EffMuzzleFlash1() {}                                                    // ROR 0x0028ca08 (linkonce; inlined in createEff)
    virtual ~EffMuzzleFlash1() {}                                           // ROR 0x0028c958 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x001788ec
    virtual int  liveMain();                                                // ROR 0x00178ac0
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00178c84
    virtual void updateZDepth();                                            // ROR 0x001790a4

    /* part's size += grow, its alpha += its velocity, the velocity +=
     * accel (a0 is this, unused). */
    void stepPart(float grow, EffMuzzlePart *part, short accel);            // ROR 0x00178c54
    /* The smoke when visible (ALPHA_1 0x44, texture 1). */
    u_long *drawSmoke(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00178dd8
    /* The flash when visible (ALPHA_1 0x48, texture 0). */
    u_long *drawFlash(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00178e68
    /* One sprite: the camera's matrix rolled by the part's angle and
     * scaled by its size, at m_pos + z axis x m_push + (0, m_yoff, 0). */
    void drawPart(EffMuzzleDraw *draw);                                     // ROR 0x00178eec

    u_char        _24[0x30 - 0x24];
    sceVu0FVECTOR m_pos;        // ROR 0x30: EffParam m_muzzle.m_pos, w 1
    EffMuzzlePart m_flash;      // ROR 0x40
    EffMuzzlePart m_smoke;      // ROR 0x50
    int           m_light;      // ROR 0x60: the point light, -1 once removed
};

#endif /* _EFF_EFFMUZZLEFLASH_H */
