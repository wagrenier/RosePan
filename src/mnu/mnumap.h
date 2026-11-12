/* ==========================================================================
 *  mnu/mnumap.h
 *
 *  MnuMap (mnu/mnumap.cpp, 0x1947a4..0x196a9c): the map screen
 *  MagLifeMenuMap (mag/maglifemenu.h) runs. It reads
 *  data/mnurpk/mnumap.rpk: map01_tcc (texture slot 0: the markers, the
 *  area and floor labels, the frame lines) and map_bg_tcc (slot 1: the
 *  frame), then the floor's map as 128 x 512 strips <name>_NN.tm2 in
 *  slots 2.. (queueStrips keeps the ones near the scroll column loaded).
 *  The floor comes from the map table (data 0x29bfb8): one row per area
 *  and floor, in order, so R1 / L1 step to the neighbouring row of the
 *  same area. The map is shown through a 384 x 256 window at (64, 112),
 *  at scale 0.5 (768 x 512 map pixels) or zoomed in at 1 (384 x 256).
 *  processPad's steps: 0 waits the fade, 1 waits the two base textures
 *  then fades in, 2 waits the strips, 3 fades the strips in and shows the
 *  help, 4 reads the pad (updatePad) until it closes, 5 waits the fade
 *  out, 6..9 change the floor (or centre on the PC): fade the strips out,
 *  swap the row, queue its strips, fade them in; 10 pops the camera and
 *  terms. Class name from the ELF's vtable (0x2e46a8), methods
 *  ror-inferred.
 * ======================================================================== */
#ifndef _MNU_MNUMAP_H
#define _MNU_MNUMAP_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "mnu/mnubase.h"
#include "msg/msgman.h"

/* A row of the map table (data 0x29bfb8, area -1 ends). EE layout. */
struct MnuMapRow
{
    int  m_area;                // ROR 0x00: GamMan::getMapArea
    int  m_floor;               // ROR 0x04: gamGetMapFloor (0 for a room whose WRD floor word is 0)
    char m_name[0x10];          // ROR 0x08: the strips' base name, "map_o1"
    int  m_numStrips;           // ROR 0x18: 128-pixel strips of the map
    int  m_label;               // ROR 0x1c: the floor label's row in map01_tcc
};
static_assert(sizeof(MnuMapRow) == 0x20, "EE 0x20");
static_assert(offsetof(MnuMapRow, m_name) == 0x08, "m_name");
static_assert(offsetof(MnuMapRow, m_numStrips) == 0x18, "m_numStrips");
static_assert(offsetof(MnuMapRow, m_label) == 0x1c, "m_label");

/* A row of toMapPos' scale table (rodata 0x2f7dc0, 13 rows): rows 0..2
 * the areas, 3..12 the attic rooms. EE layout. map x = m_mapW *
 * -(x + room x) / m_worldW + m_x; map y = m_mapH * (z + room z) /
 * m_worldH + m_y. */
struct MnuMapScale
{
    float m_worldW;             // ROR 0x00
    float m_worldH;             // ROR 0x04
    float m_mapW;               // ROR 0x08
    float m_mapH;               // ROR 0x0c
    float m_x;                  // ROR 0x10
    float m_y;                  // ROR 0x14
    float _18[2];
};
static_assert(sizeof(MnuMapScale) == 0x20, "EE 0x20");
static_assert(offsetof(MnuMapScale, m_x) == 0x10, "m_x");

/* MnuMap's own mnuBase::m_flags bits. */
enum
{
    MNU_MAP_BUSY      = 0x0000002,  /* no input: loading, fading, scrolling or closing */
    MNU_MAP_ZOOM      = 0x0010000,  /* zoomed in (scale 1), else 0.5 */
    MNU_MAP_ZOOMING   = 0x0020000,  /* the zoom runs (m_animTime) */
    MNU_MAP_SCROLLING = 0x0040000,  /* a scroll runs (cleared, never set in this file) */
    MNU_MAP_NEXT      = 0x0080000,  /* the floor change goes to the next row */
    MNU_MAP_PREV      = 0x0100000,  /* ... to the previous row */
    MNU_MAP_CENTRE    = 0x0200000,  /* ... stays, the view centres on the PC */
    MNU_MAP_MOVED_X   = 0x0400000,  /* the last scroll went right (cleared going left) */
    MNU_MAP_MOVED_Y   = 0x0800000,  /* ... went down */
    MNU_MAP_NONE      = 0x1000000,  /* no row for this area and floor: N O MAP */
};

/* The map menu (vtable 0x2e46a8). EE 0x8e0 (MagLifeMenuMap
 * +0x40..+0x920). layout: host -- polymorphic, EE offsets in comments. */
struct MnuMap : mnuBase
{
    MnuMap();                                                            // ROR 0x001947a4
    ~MnuMap();                                                           // ROR 0x00194858

    void init(DogTexInfo *tex) override;                                 // ROR 0x00194908
    void term() override;                                                // ROR 0x00194960
    /* 1 when the map is closed and termed (the menu ends). */
    int  processPad(BedPad *pad) override;                               // ROR 0x00194fe4
    void draw(I3dLayer *layer) override;                                 // ROR 0x00195e90

    /* init's work: the row, the markers, the room-name window, the files. */
    void open();                                                         // ROR 0x001949bc
    /* The camera and the volume, once the base textures are in. */
    void start();                                                        // ROR 0x00194fa4
    /* Step 4's input; 0 = close. */
    int  updatePad(BedPad *pad);                                         // ROR 0x00195430
    /* pos (x, z, +0x00 / +0x08) of the room to map pixels. */
    void toMapPos(float *pos);                                           // ROR 0x00196878
    /* The view's size in map pixels into out[0], out[1]. */
    void getViewSize(float *out);                                        // ROR 0x001959ec
    void clampScroll(float *pos);                                        // ROR 0x0019586c
    int  isScrollInRange(float x, float y);                              // ROR 0x00195958
    /* The strips near the scroll column queued, the far ones freed; 1. */
    int  queueStrips();                                                  // ROR 0x00195a34
    void showHelp(int show);                                             // ROR 0x00195b78
    /* 0..1, the cosine ease of m_animTime / m_animLen. */
    float getScrollRate();                                               // ROR 0x00196790
    float getZoomRate();                                                 // ROR 0x00196804
    /* The viewed row is the PC's. */
    int  isCurrentFloor();                                               // ROR 0x00196760

    const MnuMapRow *m_row;     // ROR 0x110: the floor shown
    int       m_mapW;           // ROR 0x114: its strips * 128 (the PC's row's, kept on a floor change)
    int       m_mapH;           // ROR 0x118: 512
    float     m_scroll[2];      // ROR 0x11c: the view's top left in map pixels
    float     m_scrollFrom[2];  // ROR 0x124: where the zoom started from
    int       m_animTime;       // ROR 0x12c: 0..m_animLen
    int       m_animLen;        // ROR 0x130: 18 for a zoom
    int       m_openTime;       // ROR 0x134: -1, then 0..18: the frame lines grow
    int       m_alpha;          // ROR 0x138: 0..15, the strips' alpha
    int       m_alphaStep;      // ROR 0x13c: +1 / -1 while it fades, 0
    int       m_moveCount;      // ROR 0x140: 0..11 while the view scrolls
    int       m_nameWindow;     // ROR 0x144: scene windows, -1 none: the room name
    int       m_helpWindow;     // ROR 0x148: the help line
    MsgManSet m_msg;            // ROR 0x150..0x8c8
    const MnuMapRow *m_pcRow;   // ROR 0x8c8: the PC's row
    float     m_pcPos[2];       // ROR 0x8cc: map pixels, -100000 none
    float     m_pcAngle;        // ROR 0x8d4: -pi..pi
    float     m_dogPos[2];      // ROR 0x8d8: Brown's
};
static_assert(offsetof(MnuMap, m_row) >= 0x110, "m_row");
static_assert(offsetof(MnuMap, m_scroll) >= 0x11c, "m_scroll");
static_assert(offsetof(MnuMap, m_openTime) >= 0x134, "m_openTime");
static_assert(offsetof(MnuMap, m_nameWindow) >= 0x144, "m_nameWindow");
static_assert(offsetof(MnuMap, m_msg) >= 0x150, "m_msg");
static_assert(offsetof(MnuMap, m_pcRow) >= 0x8c8, "m_pcRow");
static_assert(offsetof(MnuMap, m_dogPos) >= 0x8d8, "m_dogPos");
static_assert(sizeof(MnuMap) >= 0x8e0, "EE 0x8e0");

#endif /* _MNU_MNUMAP_H */
