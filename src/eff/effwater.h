/* ==========================================================================
 *  eff/effwater.h
 *
 *  The water effects, code 0x17a918..0x17c0f0 (after eff/effsteam.cpp's
 *  EffCloud1, before sct/sctlayer.cpp; no source string names the file --
 *  named after the classes, the last of the alphabetical Eff* link order).
 *
 *  - EffWater1 (kind 0x2f): a model from data/eff/EFFWATER1.RPK (file 0,
 *    its textures added to the scene's DogTexInfo) at the param's position,
 *    turned about y and scaled, its texture scrolling (u and v each by a
 *    speed / 30 a frame, wrapped into -1..1) through the material entity 0
 *    of each node's first material. Drawn in renderPreMain with utlDrawI3r,
 *    every node's display control pointing at a record carrying the param's
 *    colour. Setting it visible / hidden steps m_fade over 5 frames
 *    (liveActivateExec; while it runs the x / z scale is times m_fade / 5,
 *    the ROM's direction, as EffLight1's).
 *  - EffWater3 (kind 0x31): rain over a box. The box's x / z extent is cut
 *    into cells of about 500 (200 in mode 1, EffParam m_water3.m_mode non-zero) units; each cell keeps
 *    8 layers of falling sprites (from m_top - m_height down to m_bottom,
 *    m_speed a frame; y points down, as the steam's rise toward -y shows)
 *    and 4 layers of ground ripples growing as their y runs 0 -> -3
 *    (0.5 a frame). A layer is one random offset tiled over the cell by a
 *    random spacing. The first EffWater3 alive also greys the scene's fog
 *    (MagScene::addFog toward colour 0.15 and nearer distances; the
 *    instance count is s_count). Texture 0 of its pack holds the pictures:
 *    the drop in u 0.5..1, v 0.25..1, the ripple v 0..0.125, the splash
 *    v 0.125..0.25.
 * ======================================================================== */
#ifndef _EFF_EFFWATER_H
#define _EFF_EFFWATER_H

#include <stddef.h>

#include "eff/eff0.h"
#include "sdk/libvu0.h"

struct I3rRoot;
struct I3dView;

/* kind 0x2f; EE 0x90, vtable 0x2e44b0. layout: host. */
class EffWater1 : public Eff0
{
public:
    EffWater1();                                                            // ROR 0x0017a918
    virtual ~EffWater1();                                                   // ROR 0x0017a94c
    virtual int  init(const EffParam *param);                               // ROR 0x0017a9c8
    virtual int  liveMain();                                                // ROR 0x0017ac5c
    virtual void renderPreMain(RENDERPARAM *param);                         // ROR 0x0017ae04
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt) { return NULL; } // ROR 0x0028cf70 (linkonce)
    virtual void liveActivateExec();                                        // ROR 0x0017adc0

    u_char        _24[0x30 - 0x24];
    sceVu0FVECTOR m_pos;        // ROR 0x30: w 1
    float         m_rot;        // ROR 0x40: the y angle
    float         m_scale;      // ROR 0x44: x, y and z
    float         _48[2];
    float         m_color[4];   // ROR 0x50: the display control's colour
    I3rRoot      *m_root;       // ROR 0x60: EFFWATER1.RPK's file 0 (game heap; the destructor frees it)
    float         m_uspeed;     // ROR 0x64: u scroll per second (/ 30 a frame)
    float         m_vspeed;     // ROR 0x68: v scroll per second
    float         _6c;
    float         m_uv[4];      // ROR 0x70: the material entity: u, v offsets (-1..1), 1, 1
    int           m_fade;       // ROR 0x80: 0..5, the activation step
};

/* One cell of EffWater3's grid, the drawn layers relative to its corner.
 * EE 0xd0, floats only. */
struct EffWater3Cell
{
    float m_rect[4];            // ROR 0x00: x0, z0, x1, z1
    float m_drop[8][4];         // ROR 0x10: x, y, z offsets and the spacing of a falling layer
    float m_ripple[4][4];       // ROR 0x90: x, y (0 -> -3), z offsets and the spacing of a ground layer
};
static_assert(offsetof(EffWater3Cell, m_drop) == 0x10, "m_drop");
static_assert(offsetof(EffWater3Cell, m_ripple) == 0x90, "m_ripple");
static_assert(sizeof(EffWater3Cell) == 0xd0, "EffWater3Cell is 0xd0 bytes on the EE");

/* kind 0x31; EE 0x60, vtable 0x2e4440. layout: host. */
class EffWater3 : public Eff0
{
public:
    EffWater3();                                                            // ROR 0x0017afd8
    virtual ~EffWater3();                                                   // ROR 0x0017b010
    virtual int  init(const EffParam *param);                               // ROR 0x0017b0a4
    virtual int  liveMain();                                                // ROR 0x0017b5e0
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0017b6d4

    /* drop = a falling layer: random x / z offsets within m_size wrapped
     * into the cell, y m_top - m_height, spacing m_size * 1..1.5. */
    void resetDrop(float *drop, const EffWater3Cell *cell);                 // ROR 0x0017b7cc
    /* ripple = a ground layer: as resetDrop over m_size * m_ripple_spread,
     * y 0. */
    void resetRipple(float *ripple, const EffWater3Cell *cell);             // ROR 0x0017b918
    /* The cell's box (y m_top..m_bottom) is in view (i3dViewTestBox 1 / 2). */
    bool isCellVisible(const EffWater3Cell *cell, I3dView *view);           // ROR 0x0017ba54
    /* The cell's sprites: drops, then ripples (m_ripple_alpha > 0), then
     * splashes (m_splash_alpha > 0). */
    u_long *drawCell(u_long *pkt, const EffWater3Cell *cell);               // ROR 0x0017bb50

    EffWater3Cell *m_cell;      // ROR 0x24: m_num_cell cells (game heap; the destructor frees it)
    int    m_num_cell;          // ROR 0x28: the constructor clears it; init sets the grid's size
    float  m_top;               // ROR 0x2c: the box's low y (the sky side)
    float  m_bottom;            // ROR 0x30: its high y (the ground)
    float  m_size;              // ROR 0x34: a layer's offset range and spacing (100; mode 1 120)
    float  m_speed;             // ROR 0x38: the fall per frame (30; mode 1 60)
    float  m_width;             // ROR 0x3c: the drop sprite's x scale (1; mode 1 2)
    float  m_height;            // ROR 0x40: its y scale (60; mode 1 120)
    float  m_bright;            // ROR 0x44: the sprites' rgb (0.5; mode 1 1)
    float  m_alpha;             // ROR 0x48: the drops' alpha (0.2)
    float  m_ripple_alpha;      // ROR 0x4c: 0 none (mode 1: 1)
    float  m_ripple_size;       // ROR 0x50: a ripple's size at y -3 (15)
    float  m_ripple_spread;     // ROR 0x54: the ripples' range, times m_size (1.5)
    float  m_splash_alpha;      // ROR 0x58: 0 none (1)
    float  m_splash_scale;      // ROR 0x5c: a splash's size, times the ripple's (1.75)
};

#endif /* _EFF_EFFWATER_H */
