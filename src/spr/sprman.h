/* ==========================================================================
 *  spr/sprman.h
 *
 *  The sprite manager (spr/sprman.cpp, 0x18bb8c..): a table of 2D sprite
 *  records (0x154 bytes each, from the game heap), the SprTexture loader
 *  that fills their textures, and up to two i3d layers to draw into. The
 *  S2d layers (spr/s2d.h) each own one. RoR names no class here (no
 *  vtable): SprMan is named after the file.
 *  Minimal: what spr/s2d.cpp touches. Its functions are stubs until the
 *  sprman TU is written.
 * ======================================================================== */
#ifndef _SPR_SPRMAN_H
#define _SPR_SPRMAN_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "spr/sprtexture.h"

struct I3dLayer;

/* A sprite record, EE 0x154. EE layout for the words below; the record
 * holds an anim table pointer at +0x138 (setAnim), so the sprman TU will
 * have to make it a host struct. Roles from setFade / setDispList and the
 * S2dMes setters. */
struct SprSprite
{
    int    m_pri;               // ROR 0x00: update runs 0 first, then > 0; < 0 = unused
    u_int  m_flags;             // ROR 0x04: bit 1 (2) = process off (S2dMes::setSpriteProcessOff)
    int    _08;
    int    m_disp;              // ROR 0x0c: drawn when 1
    int    m_state;             // ROR 0x10: 0 off, 1 on, 2 fading in, 4 fading out
    int    m_fade_mode;         // ROR 0x14: setFade's mode
    int    m_color[4];          // ROR 0x18: RGBA, 0x80 = full
    int    m_fade_from[4];      // ROR 0x28
    int    m_fade_to[4];        // ROR 0x38
    int    _48[4];
    int    m_std[3];            // ROR 0x58: the standard point x, y, z
    int    m_off[3];            // ROR 0x64: the offset point x, y, z
    u_char _70[0x100 - 0x70];
    int    m_fade_wait;         // ROR 0x100: fadeOut stores the time
    u_char _104[0x154 - 0x104];
};
static_assert(offsetof(SprSprite, m_fade_wait) == 0x100, "m_fade_wait");
static_assert(offsetof(SprSprite, m_state) == 0x10, "m_state");
static_assert(offsetof(SprSprite, m_color) == 0x18, "m_color");
static_assert(offsetof(SprSprite, m_std) == 0x58, "m_std");
static_assert(offsetof(SprSprite, m_off) == 0x64, "m_off");
static_assert(sizeof(SprSprite) == 0x154, "SprSprite is 0x154 bytes on the EE");

/* A display-list record (setDispList walks them until m_no == -1), EE
 * 0x34, from the S2d layers' .data. m_anim_ee is the EE address of the
 * sprite's anim list inside the layer's pattern table (PORT: kept as the
 * EE address; the sprman TU resolves it against the table's EE base). */
struct SprDispDef
{
    int   m_no;                 // ROR 0x00: the sprite, -1 ends the list
    int   m_d8;                 // ROR 0x04: -> sprite +0xd8
    int   m_pri;                // ROR 0x08: -> sprite +0x00
    int   _0c;
    int   _10;
    int   m_disp;               // ROR 0x14: -> sprite +0x0c
    int   m_anim;               // ROR 0x18: setAnim's anim
    int   m_state;              // ROR 0x1c: -> sprite +0x10
    int   _20;
    int   m_off[3];             // ROR 0x24: -> sprite +0x64..+0x6c
    u_int m_anim_ee;            // ROR 0x30: EE address of the anim list
};
static_assert(offsetof(SprDispDef, m_off) == 0x24, "m_off");
static_assert(sizeof(SprDispDef) == 0x34, "SprDispDef is 0x34 bytes on the EE");

/* EE 0x8ec. layout: host -- holds the SprTexture and pointers. */
struct SprMan
{
    SprMan();                                                            // ROR 0x0018bb8c
    ~SprMan();                                                           // ROR 0x0018bc18

    /* Allocates num sprite records; 0 on success. */
    int  init(int num);                                                  // ROR 0x0018bcc0
    int  release();                                                      // ROR 0x0018c130
    void setDispList(const SprDispDef *def);                             // ROR 0x0018c190
    int  update();                                                       // ROR 0x0018c068
    void draw();                                                         // ROR 0x0018bd94
    /* Non-zero when anim is past the sprite's anim count. */
    int  setAnim(int no, int anim);                                      // ROR 0x0018c4d8
    void setFade(int no, int mode, int time, const int *from, const int *to); // ROR 0x0018d7ec
    void fadeInSprite(int no, int a, int time, int b, int c,
                      const int *from, const int *to);                  // ROR 0x0018f1f4
    void fadeIn(int no, int time, const int *from, const int *to);      // ROR 0x0018f338
    void fadeOut(int no, int time, const int *from, const int *to);     // ROR 0x0018f520
    void fadeOutDefault(int no, int time);                               // ROR 0x0018f41c

    int         m_00;           // ROR 0x000: S2dMes::init sets 1
    int         m_active;       // ROR 0x004: update / draw run when set
    SprTexture  m_tex;          // ROR 0x008
    u_char      _440[0x6c];     // ROR 0x440: an object built by 0x1beb70
    SprSprite  *m_sprite;       // ROR 0x4ac: m_num records
    const u_int *m_pattern;     // ROR 0x4b0: the layer's pattern table (EE image)
    int         m_num;          // ROR 0x4b4
    u_char      _4b8[0x4d0 - 0x4b8];
    I3dLayer   *m_layer[2];     // ROR 0x4d0: where draw puts the sprites
    u_char      _4d8[0x8ec - 0x4d8];
};

/* The fade colours (RGBA ints) the S2d layers pass: grey 0x80 with alpha 0
 * and with alpha 0x80. */
extern const int g_sprColorClear[4];    /* data 0x2999b0 */
extern const int g_sprColorOpaque[4];   /* data 0x299990 */

#endif /* _SPR_SPRMAN_H */
