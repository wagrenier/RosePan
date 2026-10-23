/* ==========================================================================
 *  mag/maglifewindow.h
 *
 *  MagLifeWindow (mag/maglifewindow.cpp; vtable _vt$13MagLifeWindow
 *  0x2e1d48, base MagLifePos <- MagLife, vptr at +0x14, constructor
 *  0x1308bc): a text window. The scene keeps up to MAX_WINDOW (4) of them
 *  at +0x2734 (MagScene::openWindow), each drawing into one of the scene's
 *  GamFont buffers. Minimal: what the scene touches.
 *  PORT: not polymorphic on the host yet (see mag/maglife.h); the ROM
 *  deletes windows through slot 1, the host with delete on this class.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEWINDOW_H
#define _MAG_MAGLIFEWINDOW_H

#include <stddef.h>

#include "mag/maglife.h"
#include "sdk/vu0_intrin.h"

struct GamFont;

/* layout: host -- polymorphic in the ROM (vptr +0x14); EE offsets in the
 * comments. EE size 0xe0 (MagScene::openWindow's operator_new). */
struct MagLifeWindow : MagLife
{
    MagLifeWindow();                                                     // ROR 0x001308bc
    /* EE slot 1 (+0x08), linkonce in the ROM. */
    ~MagLifeWindow();                                                    // ROR 0x00285234

    /* The window's size (also setViewParam's v[3] / v[4]), alpha 128, no
     * fade, a 0x200 x 0x200 buffer. */
    void setup(int w, int h);                                            // ROR 0x0012fe30
    /* An alpha ramp from -> to (0..0x7f, from + 1..0x80; from < 0: the
     * current alpha) over time frames. */
    void fadeIn(int time, int from, int to);                             // ROR 0x0012fe68
    /* The ramp from to (< 0: the current alpha) down to from. */
    void fadeOut(int time, int from, int to);                            // ROR 0x0012fef8
    /* Draws a string into the window font's buffer (asserts m_fntman and
     * the cell / buffer sizes). */
    void drawText(int x, int y, const char *str, int arg);               // ROR 0x00130258
    /* Draws str in colour (the font's setColor(color << 2)) from the
     * window's cursor, which it advances (asserts m_fntman). */
    void print(const char *str, int color);                              // ROR 0x001305ec
    /* Takes the GamFont the window draws with (asserts thefontManager). */
    void setFontManager(GamFont *thefontManager);                        // ROR 0x00130778
    /* The position as ints: x, y (getPos) or x, y, z (getPos3). */
    void getPos(int *x, int *y);                                         // ROR 0x00130214
    void getPos3(int *x, int *y, int *z);                                // ROR 0x00130230
    /* GamFont::getCharMetrics on the window's font. */
    void getCharMetrics(int code, int *left, int *width, int *extra);    // ROR 0x0012fca8
    /* The font's m_prop_on = on != 0. */
    void setProportional(int on);                                        // ROR 0x0012fd30
    void setFontWord30(int v);                                           // ROR 0x0012fd88
    void setFontWord34(int v);                                           // ROR 0x0012fddc

    u_char        _28[0x30 - 0x28];
    sceVu0FVECTOR m_pos;        // ROR 0x30: MagLifePos's position (setViewParam: x, y, z, 1)
    u_char        _40[0x80 - 0x40];
    int    m_wait_key;          // ROR 0x80: 0 = close at the next scene update, > 0 = close on pad bit 14, < 0 = stay (MagScene::update sub mode 2)
    int    m_84[4];             // ROR 0x84: MagScene::setWindowWords84 / getWindowWords84; roles unknown
    int    m_w;                 // ROR 0x94: setup / setViewParam
    int    m_h;                 // ROR 0x98: idem
    u_char _9c[0xc0 - 0x9c];
    int    m_tex_no;            // ROR 0xc0: the window font's texture in the scene's DogTexInfo (openWindow)
    GamFont *m_fntman;          // ROR 0xc4: setFontManager's font (the setters assert it)
    int    m_c8;                // ROR 0xc8: role unknown
    int    m_cc;                // ROR 0xcc: MagScene::getWindowWordCc; role unknown
    u_char _d0[0xe0 - 0xd0];
};
static_assert(offsetof(MagLifeWindow, m_pos) >= 0x30, "m_pos");
static_assert(offsetof(MagLifeWindow, m_wait_key) >= 0x80, "m_wait_key");
static_assert(offsetof(MagLifeWindow, m_w) >= 0x94, "m_w");
static_assert(offsetof(MagLifeWindow, m_84) >= 0x84, "m_84");
static_assert(offsetof(MagLifeWindow, m_tex_no) >= 0xc0, "m_tex_no");
static_assert(offsetof(MagLifeWindow, m_cc) >= 0xcc, "m_cc");

#endif /* _MAG_MAGLIFEWINDOW_H */
