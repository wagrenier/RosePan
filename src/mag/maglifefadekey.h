/* ==========================================================================
 *  mag/maglifefadekey.h
 *
 *  The keyhole fade (mag/maglifefadekey.cpp, pinned 0x12f298..0x12fca8):
 *  MagLifeFadeKeyHole <- MagLife (vtable _vt$18MagLifeFadeKeyHole
 *  0x2e1cf8, vptr at +0x14), the scene's +0x41890 for fade kinds 1..3
 *  (MagScene::setFade) and the event's open / magnify / remove calls
 *  (MagScene::openFadeKey, setFadeKeyMagnify, removeFadeKey). One
 *  textured full-screen sprite of a picture from data/evntex/EVNTEX.RPK
 *  (registered as "kagiana_tcc" whatever its file), zoomed about the
 *  picture's centre by m_magnify, blended (Cs - Cd) * As + Cd. EVNTEX's
 *  three pictures (g1waynozoki: blind slats, kabeana1: a hole in a wall,
 *  kagiana1: the keyhole) are 512 x 512 8-bit with a 32-bit CLUT: black
 *  everywhere, the hole in their alpha (0 in the hole, 0x80 round it).
 *  m_state sets the sprite's colour and TEXA TA0 (MagLifeFader's scheme):
 *    1, 8   colour 0, TA0 0x80
 *    2      colour 0x80 * frame / time, TA0 0x80
 *    3      TA0 (time - frame) * 0x80 / time, colour 0x80
 *    4, 5   colour 0x80, TA0 0; m_magnify ramps over m_time
 *    6      TA0 0x80 * frame / time
 *    7      colour (time - frame) * 0x80 / time, TA0 0x80
 *  TEXA only acts on 16 / 24-bit texels and the colour multiplies black,
 *  so with these pictures every state shows the same mask; the opening is
 *  the magnify (MagScene::setFadeKeyMagnify) zooming into the hole.
 *  live steps 2 -> 3 -> 4 and 6 -> 7 -> 8 when m_frame passes m_time.
 *
 *  EE virtual slots: 1 ~MagLifeFadeKeyHole (linkonce 0x28506c), 2 live,
 *  3 MagLife0::attached, 4 MagLife0::getLevel, 5 getName (linkonce
 *  0x2850a8, NULL), 6 render, 7 MagLife::setHide, 8 MagLife::getHide.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEFADEKEY_H
#define _MAG_MAGLIFEFADEKEY_H

#include <stddef.h>

#include "mag/maglife.h"

struct DogTexInfo;
struct I3dLayer;
struct MagFadeParam;

/* EE at least 0x58 (the constructor's last word; the scene's next member
 * is at +0x60). layout: host -- polymorphic (EE vptr at +0x14). */
struct MagLifeFadeKeyHole : MagLife
{
    MagLifeFadeKeyHole();                                                // ROR 0x0012f298
    /* EE slot 1: restores _vt$8MagLife0, flags & 1 deletes. */
    ~MagLifeFadeKeyHole() override {}                                    // ROR 0x0028506c
    /* EE slot 2 (+0x10): counts the fade's frames, steps its states,
     * ramps the magnify in states 4, 5. */
    void live() override;                                                // ROR 0x0012f84c
    /* EE slot 5 (+0x28), linkonce. */
    const char *getName() { return NULL; }                               // ROR 0x002850a8
    /* EE slot 6 (+0x30): the picture over the whole screen. */
    void render(I3dLayer *layer) override;                               // ROR 0x0012f95c

    /* File fileno of EVNTEX.RPK into tex as kagiana_tcc (MagScene::setFade
     * passes 0). Returns 0. */
    int  loadTexture(DogTexInfo *tex, int fileno);                       // ROR 0x0012f2f0
    /* The file theName of EVNTEX.RPK into tex as kagiana_tcc. Returns 0. */
    int  loadTextureByName(DogTexInfo *tex, const char *theName);        // ROR 0x0012f518
    /* Releases the texture m_tex_no from theTexInfo (nothing when -1). */
    void releaseTexture(DogTexInfo *theTexInfo);                         // ROR 0x0012f790
    /* m_state = state over time frames, the magnify words 1. */
    void reset(int state, int time);                                     // ROR 0x0012f7f4
    /* State p->m_mode over p->m_time frames, from frame 0. */
    void start(const MagFadeParam *p);                                   // ROR 0x0012f824
    /* m_magnify to magnify over time frames (<= 0: at once). */
    void setMagnify(float magnify, int time);                            // ROR 0x0012fc80

    u_char _28[0x30 - 0x28];
    int    m_frame;             // ROR 0x30: frames since start / reset / setMagnify
    int    m_time;              // ROR 0x34: the fade's (or the magnify ramp's) length in frames
    u_char _38[0x40 - 0x38];
    int    m_tex_no;            // ROR 0x40: kagiana_tcc in the scene's table, -1 = not loaded
    float  m_magnify;           // ROR 0x44: the zoom, 1 = the picture's 512 x 448 centre fills the screen
    float  m_magnify_from;      // ROR 0x48: setMagnify's ramp start
    float  m_magnify_to;        // ROR 0x4c: setMagnify's ramp end
    float  m_50;                // ROR 0x50: horizontal shift of the view, -1..1 of the free width; only the constructor sets it (0)
    int    m_54;                // ROR 0x54: the constructor clears it; not read here
};
static_assert(offsetof(MagLifeFadeKeyHole, m_frame) >= 0x30, "m_frame");
static_assert(offsetof(MagLifeFadeKeyHole, m_time) >= 0x34, "m_time");
static_assert(offsetof(MagLifeFadeKeyHole, m_tex_no) >= 0x40, "m_tex_no");
static_assert(offsetof(MagLifeFadeKeyHole, m_magnify) >= 0x44, "m_magnify");
static_assert(offsetof(MagLifeFadeKeyHole, m_magnify_to) >= 0x4c, "m_magnify_to");
static_assert(offsetof(MagLifeFadeKeyHole, m_54) >= 0x54, "m_54");

#endif /* _MAG_MAGLIFEFADEKEY_H */
