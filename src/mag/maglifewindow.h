/* ==========================================================================
 *  mag/maglifewindow.h
 *
 *  MagLifeWindow (mag/maglifewindow.cpp, 0x12fca8..0x1308bc + the linkonce
 *  accessors 0x285234..0x2852ec; vtable _vt$13MagLifeWindow 0x2e1d48, base
 *  MagLifePos <- MagLife, vptr at +0x14, constructor 0x1308bc): a text
 *  window. The scene keeps up to MAX_WINDOW (4) of them at +0x2734
 *  (MagScene::openWindow) and runs them in its life manager: each draws
 *  text into one of the scene's GamFont buffers (drawText / print) and,
 *  as a life, shows that buffer's texture as one sprite at m_pos (render),
 *  with an alpha fade and a life timer.
 *
 *  EE virtual slots: 1 ~MagLifeWindow (linkonce), 2 live, 3
 *  MagLife0::attached, 4 MagLife0::getLevel, 5 getName (linkonce, NULL),
 *  6 render, 7 MagLife::setHide, 8 MagLife::getHide, 9
 *  MagLifePos::updatePos, 10 MagLifePos::updateSpeed, 11 MagLifePos::move,
 *  12 processPad (linkonce, 0). The host declares 1, 2, 6, 7, 8 as
 *  virtuals (mag/maglife.h); the others are plain members.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEWINDOW_H
#define _MAG_MAGLIFEWINDOW_H

#include <stddef.h>

#include "mag/maglife.h"
#include "sdk/vu0_intrin.h"

struct BedPad;
struct GamFont;
struct I3dLayer;

/* MagLife with a position, a speed and an acceleration (vtable
 * _vt$10MagLifePos 0x2e1db8). Its constructor is inline (in
 * MagLifeWindow's); nothing else in the ROM derives from it.
 * layout: host -- polymorphic (EE vptr at +0x14); EE offsets in comments. */
struct MagLifePos : MagLife
{
    /* EE slot 9 (+0x48), linkonce: m_pos += m_speed (w 1). */
    // ROR 0x00285134
    void updatePos()
    {
        m_pos[0] += m_speed[0];
        m_pos[1] += m_speed[1];
        m_pos[3] = 1.0f;
        m_pos[2] += m_speed[2];
    }
    /* EE slot 10 (+0x50), linkonce: m_speed += m_accel (w 1). */
    // ROR 0x0028517c
    void updateSpeed()
    {
        m_speed[0] += m_accel[0];
        m_speed[1] += m_accel[1];
        m_speed[3] = 1.0f;
        m_speed[2] += m_accel[2];
    }
    /* EE slot 11 (+0x58), linkonce: nothing. */
    // ROR 0x002851c4
    void move() {}

    u_char        _28[0x30 - 0x28];
    sceVu0FVECTOR m_pos;        // ROR 0x30: x, y, z, 1 (MagScene::setViewParam; render's screen offset)
    sceVu0FVECTOR m_speed;      // ROR 0x40: updatePos adds it to m_pos (name inferred)
    sceVu0FVECTOR m_accel;      // ROR 0x50: updateSpeed adds it to m_speed (name inferred)
    sceVu0FVECTOR m_60;         // ROR 0x60: the constructor sets 0, 0, 0, 1; no reader found
    sceVu0FVECTOR m_70;         // ROR 0x70: idem
};
static_assert(offsetof(MagLifePos, m_pos) >= 0x30, "m_pos");
static_assert(offsetof(MagLifePos, m_accel) >= 0x50, "m_accel");
static_assert(offsetof(MagLifePos, m_70) >= 0x70, "m_70");

/* layout: host -- polymorphic in the ROM (vptr +0x14); EE offsets in the
 * comments. EE size 0xe0 (MagScene::openWindow's operator_new). */
struct MagLifeWindow : MagLifePos
{
    /* White, alpha 0x80, no texture (-1), no font, the vectors 0, 0, 0, 1. */
    MagLifeWindow();                                                     // ROR 0x001308bc
    /* EE slot 1 (+0x08), linkonce: restores _vt$8MagLife0; flags & 1
     * deletes (the host's delete). */
    // ROR 0x00285234
    ~MagLifeWindow() override {}

    /* EE slot 2 (+0x10): counts m_timer down to 0 (MagScene::update's sub
     * mode 2 closes the window at 0). */
    void live() override;                                                // ROR 0x0012ff84
    /* EE slot 6 (+0x30): steps the alpha fade, then draws the font
     * texture m_tex_no as one sprite at m_pos, m_w x m_h, from m_uv. */
    void render(I3dLayer *layer) override;                               // ROR 0x0012ff9c

    /* The window's size (also setViewParam's v[3] / v[4]), alpha 128, no
     * fade, a 0x200 x 0x200 buffer until setFontManager. */
    void setup(int w, int h);                                            // ROR 0x0012fe30
    /* An alpha ramp from -> to (0..0x7f, from + 1..0x80; from < 0: the
     * current alpha) over time frames. */
    void fadeIn(int time, int from, int to);                             // ROR 0x0012fe68
    /* The ramp from to (< 0: the current alpha) down to from. */
    void fadeOut(int time, int from, int to);                            // ROR 0x0012fef8
    /* Draws a Shift-JIS string into the font's buffer from (x, y) in
     * color, wrapping at m_buf_w and scrolling at m_buf_h; a leading 0x11
     * centres each line in m_w. Leaves the cursor after the text. */
    void drawText(int x, int y, const char *str, int color);             // ROR 0x00130258
    /* Draws str (ASCII; fixed half-cell advance) in color from the
     * cursor, which it advances. */
    void print(const char *str, int color);                              // ROR 0x001305ec
    /* Takes the GamFont the window draws with: its cell and buffer sizes,
     * the whole buffer as the sprite's uv. */
    void setFontManager(GamFont *thefontManager);                        // ROR 0x00130778
    /* GamFont::getTextWidth of theStr. */
    int  getTextWidth(const char *theStr);                               // ROR 0x0013086c
    /* The position as ints: x, y (getPos) or x, y, z (getPos3). */
    void getPos(int *x, int *y);                                         // ROR 0x00130214
    void getPos3(int *x, int *y, int *z);                                // ROR 0x00130230
    /* GamFont::getCharMetrics on the window's font. */
    void getCharMetrics(int code, int *left, int *width, int *extra);    // ROR 0x0012fca8
    /* The font's m_prop_on = on != 0. */
    void setProportional(int on);                                        // ROR 0x0012fd30
    /* The font's m_30 / m_34 (its fixed / proportional spacing). */
    void setFontWord30(int v);                                           // ROR 0x0012fd88
    void setFontWord34(int v);                                           // ROR 0x0012fddc

    /* The linkonce accessors (inline in the class). */
    /* EE slot 5 (+0x28) */
    // ROR 0x002852ec
    const char *getName() { return NULL; }
    /* EE slot 12 (+0x60) */
    // ROR 0x00285268
    int  processPad(const BedPad &, const BedPad &) { return 0; }
    // ROR 0x00285270
    void setFontTexNo(int no) { m_tex_no = no; }
    // ROR 0x00285278
    void getWH(int *w, int *h)
    {
        *w = m_w;
        *h = m_h;
    }
    // ROR 0x0028528c
    void setWH(int w, int h)
    {
        m_h = h;
        m_w = w;
    }
    // ROR 0x00285298
    void setUv(int u0, int v0, int u1, int v1)
    {
        m_uv[3] = v1;
        m_uv[0] = u0;
        m_uv[1] = v0;
        m_uv[2] = u1;
    }
    // ROR 0x002852ac
    void getUv(int *u0, int *v0, int *u1, int *v1)
    {
        *u0 = m_uv[0];
        *v0 = m_uv[1];
        *u1 = m_uv[2];
        *v1 = m_uv[3];
    }
    // ROR 0x002852d0
    void getFontH(int *h) { *h = m_ch_h; }
    // ROR 0x002852dc
    void setTimer(int timer) { m_timer = timer; }
    // ROR 0x002852e4
    int  getTimer() const { return m_timer; }

    int    m_timer;             // ROR 0x80: live counts it down while > 0; MagScene::update sub mode 2 closes at 0 (or on pad bit 14 while > 0), < 0 = stays
    int    m_uv[4];             // ROR 0x84: the sprite's u0, v0, u1, v1 in the font texture (setUv; setFontManager: the whole buffer)
    int    m_w;                 // ROR 0x94: the sprite's size (setup / setWH); drawText centres lines in m_w
    int    m_h;                 // ROR 0x98
    int    m_color[3];          // ROR 0x9c: the sprite's r, g, b (0xff)
    int    m_alpha;             // ROR 0xa8: the sprite's alpha (0x80); render sets it from the fade
    int    m_fade_time;         // ROR 0xac: frames left in the fade, -1 = none
    float  m_fade_step;         // ROR 0xb0: alpha per frame
    float  m_fade_max;          // ROR 0xb4: the fade's upper alpha
    float  m_fade_min;          // ROR 0xb8: its lower alpha
    float  m_fade_alpha;        // ROR 0xbc: the current alpha
    int    m_tex_no;            // ROR 0xc0: the font's texture in the scene's DogTexInfo (setFontTexNo), < 0 = nothing drawn
    GamFont *m_fntman;          // ROR 0xc4: setFontManager's font (the setters assert it)
    int    m_ch_w;              // ROR 0xc8: the font's glyph cell (GamFont m_char_w)
    int    m_ch_h;              // ROR 0xcc: the line height (m_char_h)
    int    m_buf_w;             // ROR 0xd0: the font's buffer (m_dst_w)
    int    m_buf_h;             // ROR 0xd4: (m_dst_h)
    int    m_cursor_x;          // ROR 0xd8: where drawText / print stopped
    int    m_cursor_y;          // ROR 0xdc
};
static_assert(offsetof(MagLifeWindow, m_timer) >= 0x80, "m_timer");
static_assert(offsetof(MagLifeWindow, m_uv) >= 0x84, "m_uv");
static_assert(offsetof(MagLifeWindow, m_w) >= 0x94, "m_w");
static_assert(offsetof(MagLifeWindow, m_fade_alpha) >= 0xbc, "m_fade_alpha");
static_assert(offsetof(MagLifeWindow, m_tex_no) >= 0xc0, "m_tex_no");
static_assert(offsetof(MagLifeWindow, m_ch_h) >= 0xcc, "m_ch_h");
static_assert(offsetof(MagLifeWindow, m_cursor_y) >= 0xdc, "m_cursor_y");

/* MagLifeWindowDebug (vtable _vt$18MagLifeWindowDebug 0x2e1fe8): the
 * scene's debug windows (MagScene::m_dbg_window, mag/magscenedebug.cpp).
 * Only getName (slot 5, 0x285844) differs from MagLifeWindow's; the host
 * has no getName slot. Constructor in mag/magscenecmd.cpp.
 * layout: host -- MagLifeWindow's, nothing added (EE 0xe0). */
struct MagLifeWindowDebug : MagLifeWindow
{
    MagLifeWindowDebug();                                                // ROR 0x00144214
};

#endif /* _MAG_MAGLIFEWINDOW_H */
