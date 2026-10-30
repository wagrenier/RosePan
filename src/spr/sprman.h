/* ==========================================================================
 *  spr/sprman.h
 *
 *  The sprite manager (spr/sprman.cpp, 0x18bb8c..0x18f5b0; RoR names no
 *  class here, no vtable: SprMan is named after the file). A table of 2D
 *  sprite records (0x154 bytes each on the EE, from the game heap), the
 *  SprTexture loader that fills their textures, a font object for text
 *  drawn into a sprite's texture, and up to two i3d layers to draw into.
 *  The S2d layers (spr/s2d.h) and some 20 menu / message objects each own
 *  one.
 *
 *  Each frame update steps the CLUT animations and draw steps every
 *  sprite's motion, animation and colour fade, then writes it into layer
 *  0's draw set as one GS sprite: plain colour (texture -98), a copy of the
 *  frame buffer (-99) or a texture of the list (the i3d packet writers
 *  hand it to the host renderer). Sprites with m_parent 0 go first, then
 *  the children (> 0), which are placed relative to their parent.
 *
 *  The tables a sprite runs on (patterns, anims and their frames, moves and
 *  their keys) are the owner's .data: one EE image whose words point into
 *  it by EE address. PORT: m_pattern is that image on the host and
 *  m_pattern_ee its EE base; eePtr() turns an EE address into a host
 *  pointer. The count words (< 0 in the data) are written back by
 *  countAnims / countMoves, so the image must be writable.
 * ======================================================================== */
#ifndef _SPR_SPRMAN_H
#define _SPR_SPRMAN_H

#include <stddef.h>

#include "lib/unassigned.h"
#include "sdk/eetypes.h"
#include "spr/sprtexture.h"

struct I3dDrawSet;
struct I3dLayer;

/* SprPattern::m_tex values that are no texture of the list. */
#define SPR_TEX_NONE    (-1)    /* nothing drawn */
#define SPR_TEX_COLOR   (-98)   /* a plain colour sprite */
#define SPR_TEX_FRAME   (-99)   /* the frame buffer as the texture (m_u = its base) */

/* A pattern: a texture and its rect, EE 0x14 (data). The rect is in
 * texels with 4 fraction bits. */
struct SprPattern
{
    int m_tex;                  // ROR 0x00: SprTexFile id, or SPR_TEX_*
    int m_u;                    // ROR 0x04
    int m_v;                    // ROR 0x08
    int m_w;                    // ROR 0x0c
    int m_h;                    // ROR 0x10
};
static_assert(offsetof(SprPattern, m_u) == 0x04, "m_u");
static_assert(offsetof(SprPattern, m_h) == 0x10, "m_h");
static_assert(sizeof(SprPattern) == 0x14, "SprPattern is 0x14 bytes on the EE");

/* SprFrame::m_ctrl: the frame's time (low 16 bits), flags (bits 16..23,
 * copied to SprSprite::m_frame_flags) and, in the word after the last
 * frame, the end code (top byte). */
#define SPR_FRAME_TIME(c)   ((c) & 0xffff)
#define SPR_FRAME_FLAGS     0x00ff0000u
#define SPR_FRAME_SIZE      0x00010000u /* the size comes from the frame, not the pattern */
#define SPR_FRAME_FLIP_U    0x00020000u
#define SPR_FRAME_FLIP_V    0x00040000u
#define SPR_FRAME_END       0xff000000u
#define SPR_END_STOP        0x01000000u /* the anim stops (m_disp 0) */
#define SPR_END_HOLD        0x02000000u /* stays on the last frame */
#define SPR_END_LOOP        0x03000000u /* back to frame 0 */
#define SPR_END_JUMP        0x04000000u /* to the anim in the end word's m_pos[0] */

/* SprFrame::m_w / m_h values that are no size. */
#define SPR_SIZE_KEEP       0x3e70      /* keep the sprite's draw size */
#define SPR_SIZE_PATTERN    (-0x10)     /* the pattern's size */

/* An animation frame, EE 0x1c (data). */
struct SprFrame
{
    u_int m_ctrl;               // ROR 0x00: SPR_FRAME_*
    int   m_pat;                // ROR 0x04: the pattern (+ the sprite's m_pat_base)
    int   m_pos[3];             // ROR 0x08: added to the sprite's position
    int   m_w;                  // ROR 0x14: draw size, or SPR_SIZE_*
    int   m_h;                  // ROR 0x18
};
static_assert(offsetof(SprFrame, m_pat) == 0x04, "m_pat");
static_assert(offsetof(SprFrame, m_pos) == 0x08, "m_pos");
static_assert(offsetof(SprFrame, m_w) == 0x14, "m_w");
static_assert(sizeof(SprFrame) == 0x1c, "SprFrame is 0x1c bytes on the EE");

/* An animation, EE 0x0c (data); m_id -1 ends the table. m_num < 0 is
 * counted by countAnims. */
struct SprAnim
{
    int   m_id;                 // ROR 0x00
    int   m_num;                // ROR 0x04: frames
    u_int m_frames;             // ROR 0x08: EE address of the SprFrames
};
static_assert(offsetof(SprAnim, m_frames) == 0x08, "m_frames");
static_assert(sizeof(SprAnim) == 0x0c, "SprAnim is 0x0c bytes on the EE");

/* SprMoveKey::m_type */
#define SPR_KEY_EASE        0   /* ease from m_from to m_to */
#define SPR_KEY_EASE_HERE   1   /* ease from the sprite's offset */
#define SPR_KEY_LINEAR      2
#define SPR_KEY_LINEAR_HERE 3
#define SPR_KEY_HALT        5   /* zeroes the speed and runs on to the next key (updateMove) */
#define SPR_KEY_END         (-1)
#define SPR_KEY_REPEAT      (-2)

/* A motion key, EE 0x20 (data); the type < 0 ends the move. Positions in
 * pixels. */
struct SprMoveKey
{
    int   m_type;               // ROR 0x00: SPR_KEY_*
    int   m_time;               // ROR 0x04: frames
    float m_from[3];            // ROR 0x08
    float m_to[3];              // ROR 0x14
};
static_assert(offsetof(SprMoveKey, m_from) == 0x08, "m_from");
static_assert(offsetof(SprMoveKey, m_to) == 0x14, "m_to");
static_assert(sizeof(SprMoveKey) == 0x20, "SprMoveKey is 0x20 bytes on the EE");

/* A move, EE 0x0c (data); m_id -1 ends the table. */
struct SprMove
{
    int   m_id;                 // ROR 0x00
    int   m_num;                // ROR 0x04: keys
    u_int m_keys;               // ROR 0x08: EE address of the SprMoveKeys
};
static_assert(offsetof(SprMove, m_keys) == 0x08, "m_keys");
static_assert(sizeof(SprMove) == 0x0c, "SprMove is 0x0c bytes on the EE");

/* SprSprite::m_state */
#define SPR_STATE_OFF       0
#define SPR_STATE_ON        1
#define SPR_STATE_WAIT_IN   2   /* hidden for m_fade_wait frames, then on */
#define SPR_STATE_SHOW_IN   3   /* shown, on after m_fade_wait frames */
#define SPR_STATE_WAIT_OUT  4   /* shown for m_fade_wait frames, then off */
#define SPR_STATE_TEXT      5   /* only draws pending text into its texture */
#define SPR_STATE_PLACE     6   /* placed (position, frame) but not drawn */
#define SPR_STATE_STOP      7   /* turns off at the next draw */

/* SprSprite::m_move_mode */
#define SPR_MOVE_NONE       0
#define SPR_MOVE_EASE       1
#define SPR_MOVE_SPEED      2
#define SPR_MOVE_STOP       3
#define SPR_MOVE_TABLE      5

/* A sprite record, EE 0x154. layout: host -- the table pointers from
 * +0x138 on; every field before keeps its EE offset. Positions are pixels
 * with 4 fraction bits (screen 640 x 480 = 0x2800 x 0x1e00). */
struct SprSprite
{
    int    m_parent;            // ROR 0x00: -1 unused, 0 top level, n > 0 placed on sprite n
    u_int  m_flags;             // ROR 0x04: bit 1 (2) = process off (S2dMes::setSpriteProcessOff)
    int    m_move_mode;         // ROR 0x08: SPR_MOVE_*
    int    m_disp;              // ROR 0x0c: the anim runs (0 off, 1 on, 2 frozen)
    int    m_state;             // ROR 0x10: SPR_STATE_*
    u_int  m_fade_mode;         // ROR 0x14: setFade's mode (low byte) and phase (0x100 / 0x101 / 0x102)
    int    m_color[4];          // ROR 0x18: RGBA, 0x80 = full
    int    m_fade_from[4];      // ROR 0x28
    int    m_fade_to[4];        // ROR 0x38
    int    m_fade_save[4];      // ROR 0x48: callers copy m_color here to fade from / to it (MnuItem)
    int    m_std[3];            // ROR 0x58: the standard point x, y, z
    int    m_off[3];            // ROR 0x64: the offset point x, y, z (the motion writes it)
    int    m_pos[3];            // ROR 0x70: drawSprite's position (children start from it)
    float  m_move_pos[3];       // ROR 0x7c: the motion's position in pixels
    float  m_move_speed[3];     // ROR 0x88
    float  m_move_accel[3];     // ROR 0x94
    int    m_move_from[3];      // ROR 0xa0
    int    m_move_to[3];        // ROR 0xac: m_off at the end of an ease
    int    m_u;                 // ROR 0xb8: the pattern's rect
    int    m_v;                 // ROR 0xbc
    int    m_w;                 // ROR 0xc0
    int    m_h;                 // ROR 0xc4
    int    m_draw_w;            // ROR 0xc8: the size on screen
    int    m_draw_h;            // ROR 0xcc
    int    m_fix_w;             // ROR 0xd0: overrides m_draw_w unless -1
    int    m_fix_h;             // ROR 0xd4
    int    m_pat_base;          // ROR 0xd8: added to the frames' pattern numbers
    float  m_fade_step[4];      // ROR 0xdc
    int    m_fade_count;        // ROR 0xec
    int    m_fade_time;         // ROR 0xf0
    int    m_anim_timer;        // ROR 0xf4
    int    m_move_timer;        // ROR 0xf8
    int    m_move_time;         // ROR 0xfc
    int    m_fade_wait;         // ROR 0x100: the SPR_STATE_* waits
    int    m_move_no;           // ROR 0x104
    int    m_move_key;          // ROR 0x108
    int    m_tex;               // ROR 0x10c: the current pattern's texture
    int    m_anim_no;           // ROR 0x110
    int    m_frame;             // ROR 0x114
    u_int  m_frame_flags;       // ROR 0x118: SPR_FRAME_SIZE / FLIP_U / FLIP_V
    int    m_clut;              // ROR 0x11c: the CLUT index drawSprite gives the texture
    int    m_clut_timer;        // ROR 0x120
    int    m_clut_step;         // ROR 0x124
    int    _128;
    int    m_clut_max;          // ROR 0x12c
    int    m_clut_interval;     // ROR 0x130: 0 = no CLUT animation
    int    m_anim_num;          // ROR 0x134
    SprAnim    *m_anim;         // ROR 0x138: the anim table
    int         m_move_num;     // ROR 0x13c
    SprMove    *m_move;         // ROR 0x140: the move table
    int         m_text_x;       // ROR 0x144: the text's origin in the texture
    int         m_text_y;       // ROR 0x148
    const char *m_text;         // ROR 0x14c: drawn into the texture at the next draw, then cleared
    const int  *m_clear_rect;   // ROR 0x150: x, y, w, h cleared first, then cleared
};
static_assert(offsetof(SprSprite, m_flags) == 0x04, "m_flags");
static_assert(offsetof(SprSprite, m_state) == 0x10, "m_state");
static_assert(offsetof(SprSprite, m_color) == 0x18, "m_color");
static_assert(offsetof(SprSprite, m_fade_from) == 0x28, "m_fade_from");
static_assert(offsetof(SprSprite, m_std) == 0x58, "m_std");
static_assert(offsetof(SprSprite, m_off) == 0x64, "m_off");
static_assert(offsetof(SprSprite, m_pos) == 0x70, "m_pos");
static_assert(offsetof(SprSprite, m_move_pos) == 0x7c, "m_move_pos");
static_assert(offsetof(SprSprite, m_move_from) == 0xa0, "m_move_from");
static_assert(offsetof(SprSprite, m_u) == 0xb8, "m_u");
static_assert(offsetof(SprSprite, m_pat_base) == 0xd8, "m_pat_base");
static_assert(offsetof(SprSprite, m_fade_step) == 0xdc, "m_fade_step");
static_assert(offsetof(SprSprite, m_fade_wait) == 0x100, "m_fade_wait");
static_assert(offsetof(SprSprite, m_tex) == 0x10c, "m_tex");
static_assert(offsetof(SprSprite, m_clut) == 0x11c, "m_clut");
static_assert(offsetof(SprSprite, m_anim_num) == 0x134, "m_anim_num");
static_assert(sizeof(SprSprite) >= 0x154, "SprSprite is 0x154 bytes on the EE");

/* A display-list record (setDispList walks them until m_no == -1), EE
 * 0x34, from the S2d layers' .data. m_anim_ee is the EE address of the
 * sprite's anim table inside the owner's table image (SprMan::eePtr). */
struct SprDispDef
{
    int   m_no;                 // ROR 0x00: the sprite, -1 ends the list
    int   m_pat_base;           // ROR 0x04: -> sprite m_pat_base
    int   m_parent;             // ROR 0x08: -> sprite m_parent
    int   _0c;
    int   _10;
    int   m_disp;               // ROR 0x14: -> sprite m_disp
    int   m_anim;               // ROR 0x18: setAnim's anim
    int   m_state;              // ROR 0x1c: -> sprite m_state
    int   _20;
    int   m_off[3];             // ROR 0x24: -> sprite m_off
    u_int m_anim_ee;            // ROR 0x30: EE address of the anim table
};
static_assert(offsetof(SprDispDef, m_off) == 0x24, "m_off");
static_assert(sizeof(SprDispDef) == 0x34, "SprDispDef is 0x34 bytes on the EE");

/* EE 0x8ec. layout: host -- holds the SprTexture, the font and pointers. */
struct SprMan
{
    SprMan();                                                            // ROR 0x0018bb8c
    ~SprMan();                                                           // ROR 0x0018bc18

    /* Allocates num sprite records; 0 on success. */
    int  init(int num);                                                  // ROR 0x0018bcc0
    int  initDefault();                                                  // ROR 0x0018bca8
    void initSprite(SprSprite *spr);                                     // ROR 0x0018c814
    /* Frees the records and the textures; -1 when there were none. */
    int  release();                                                      // ROR 0x0018c130
    void setDispList(const SprDispDef *def);                             // ROR 0x0018c190
    /* The CLUT animations; 0. */
    int  update();                                                       // ROR 0x0018c068
    /* Steps and draws every sprite; 1 when layer 0 has no draw set. */
    int  draw();                                                         // ROR 0x0018bd94
    void drawFrameCopy();                                                // ROR 0x0018bef8
    /* Gives the font object a font file (no 1); 0 on success. */
    int  setFont(const char *theName, int no);                           // ROR 0x0018c2e4

    /* Animations. setAnim / setSprAnim are non-zero when anim is past the
     * sprite's anim count. */
    int  setAnimTable(int no, SprAnim *anims);                           // ROR 0x0018c448
    int  setAnim(int no, int anim);                                      // ROR 0x0018c4d8
    int  setSprAnim(SprSprite *spr, int anim);                           // ROR 0x0018c54c
    int  countAnims(SprAnim *anims);                                     // ROR 0x0018c910
    int  countFrames(const SprFrame *frames);                            // ROR 0x0018c990
    void updateAnim(SprSprite *spr);                                     // ROR 0x0018d640

    /* Motion. */
    int  setMoveTable(int no, SprMove *moves);                           // ROR 0x0018c630
    int  setMove(int no, int move);                                      // ROR 0x0018c6c4
    int  setSprMove(SprSprite *spr, int move);                           // ROR 0x0018c738
    int  countMoves(SprMove *moves);                                     // ROR 0x0018c9d8
    int  countKeys(const SprMoveKey *keys);                              // ROR 0x0018ca58
    float calcAccel(float dist, int n);                                  // ROR 0x0018ca98
    void startEaseMove(int no, int time, const int *from, const int *to); // ROR 0x0018cac8
    void setEaseMove(SprSprite *spr, int time, const float *from, const float *to); // ROR 0x0018cd88
    void setLinearMove(SprSprite *spr, int time, const float *from, const float *to); // ROR 0x0018ceec
    void startSpeedMove(int no, int time, const float *speed, const float *accel); // ROR 0x0018cff0
    void startMoveKey(SprSprite *spr);                                   // ROR 0x0018d08c
    void updateMove(SprSprite *spr);                                     // ROR 0x0018d1e8

    /* Colour fades (RGBA ints, 0x80 = full). */
    void setFade(int no, int mode, int time, const int *from, const int *to); // ROR 0x0018d7ec
    void updateFade(SprSprite *spr);                                     // ROR 0x0018dafc
    void fadeInSprite(int no, int wait, int time, int anim, int move,
                      const int *from, const int *to);                  // ROR 0x0018f1f4
    void fadeInWait(int no, int wait, int time, const int *from, const int *to); // ROR 0x0018f2e8
    void fadeIn(int no, int time, const int *from, const int *to);      // ROR 0x0018f338
    void fadeOutDefault(int no, int time);                               // ROR 0x0018f41c
    void fadeOutColor(int no, int time, const int *from, const int *to); // ROR 0x0018f4a0
    void fadeOut(int no, int time, const int *from, const int *to);     // ROR 0x0018f520
    void showSprite(int no, int anim, int move);                         // ROR 0x0018f0ec
    void hideSprite(int no);                                             // ROR 0x0018f3e0

    void updateClut(SprSprite *spr);                                     // ROR 0x0018df20
    void *getTexImage(int no);                                           // ROR 0x0018dfdc
    int  isOnScreen(int x0, int y0, int x1, int y1);                     // ROR 0x0018e028
    void drawSprite(SprSprite *spr, int layer);                          // ROR 0x0018e05c
    /* 0 when layer n's packet is open (m_pkt). */
    int  beginDraw(int layer);                                           // ROR 0x0018ef40
    void endDraw(int layer);                                             // ROR 0x0018eff0

    /* Text drawn into a sprite's texture. */
    void clearTextRect(SprSprite *spr, void *image, int w, int h);       // ROR 0x0018ea9c
    void getTextLine(const char *text, char *line, int n);               // ROR 0x0018eba4
    int  getTextWidth(const char *text);                                 // ROR 0x0018ec58
    void drawText(SprSprite *spr, void *image, int w, int h);            // ROR 0x0018ed44

    /* The game volume around a sprite scene (GamMan::switchVolume). */
    static void volumeOn();                                              // ROR 0x0018f024
    static void volumeOff();                                             // ROR 0x0018f060
    void saveVolume();                                                   // ROR 0x0018f09c
    void restoreVolume();                                                // ROR 0x0018f0b4

    /* PC: an EE address in the table image -> its host address (NULL for
     * 0). */
    void *eePtr(u_int ee) const
    {
        return ee ? (u_char *)m_pattern + (ee - m_pattern_ee) : NULL;
    }

    int          m_00;          // ROR 0x000: S2dMes::init sets 1
    int          m_active;      // ROR 0x004: update / draw run when set
    SprTexture   m_tex;         // ROR 0x008
    GamFont      m_font[1];     // ROR 0x440: the text sprites' font
    SprSprite   *m_sprite;      // ROR 0x4ac: m_num records
    u_int       *m_pattern;     // ROR 0x4b0: the owner's table image, SprPatterns first
    int          m_num;         // ROR 0x4b4
    int          _4b8[3];
    int          m_back_color;  // ROR 0x4c4: drawFrameCopy's grey, 0x80
    int          m_volume_on;   // ROR 0x4c8: saveVolume
    int          m_4cc;         // ROR 0x4cc: the constructor clears it
    I3dLayer    *m_layer[2];    // ROR 0x4d0: 0 the sprites, 1 drawFrameCopy
    u_long      *m_pkt;         // ROR 0x4d8: the open draw set's packet position
    I3dDrawSet  *m_draw_set[2]; // ROR 0x4dc: by layer
    void        *m_font_data[1]; // ROR 0x4e4: setFont's font file
    u_char       _4e8[0x8e8 - 0x4e8];
    int          m_font_in_pack; // ROR 0x8e8: 1 = setFont takes the font from the texture pack
    u_int        m_pattern_ee;  // PC: m_pattern's EE address (see the file comment)
};

/* The fade colours (RGBA ints) the S2d layers pass: grey 0x80 with alpha 0
 * and with alpha 0x80. */
extern const int g_sprColorClear[4];    /* data 0x2999b0 */
extern const int g_sprColorOpaque[4];   /* data 0x299990 */

#endif /* _SPR_SPRMAN_H */
