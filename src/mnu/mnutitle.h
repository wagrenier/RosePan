/* ==========================================================================
 *  mnu/mnutitle.h
 *
 *  MnuTitle (mnu/mnutitle.cpp, 0x1a90ec..0x1ab474): the title screen.
 *  MagLifeMenuTitle (mag/maglifemenu.h) holds one at +0x40 and forwards
 *  init / term / isInit / processPad / render to it; MagScene opens it as
 *  menu 0x10005. Class name from the wrapper (ror-inferred).
 *
 *  It owns the menus the title runs in place -- the memory card check
 *  (MnuMcd), the load screen (MnuSvld), the options (MnuOpt) and a text
 *  list for the movie viewer -- and a SprMan with data/mnurpk/mnutitle.rpk
 *  (15 sprites: the backgrounds, the logos, "press start", the three
 *  items). processPad steps m_mode (load the textures, set up the sprites,
 *  run, close) and, while running, the sub menu m_sub: 0 the logos and the
 *  movies before "press start", 1 the three items, 3 load, 4 options; 2, 5
 *  and 6 (a movie test, the movie viewer, a three-way choice) are reached
 *  by no retail code. processPad returns m_result once the title closes.
 * ======================================================================== */
#ifndef _MNU_MNUTITLE_H
#define _MNU_MNUTITLE_H

#include <stddef.h>

#include "lib/unassigned.h"
#include "mnu/mnuopt.h"
#include "mnu/mnusvld.h"
#include "sdk/eetypes.h"
#include "spr/sprman.h"

struct BedPad;
struct DogTexInfo;
struct I3dLayer;
struct MagScene;
struct MagSystem;

/* A page of the movie viewer (data 0x2a8428, 8 pages of 0x78 bytes): the
 * list's labels, then per row the movie, what choosing it does (0 play, 1
 * the page before, 2 the page after) and its size. layout: host -- the
 * names are pointers. */
struct MnuTitleMoviePage
{
    int         m_num;          // ROR 0x00: rows
    const char *m_items;        // ROR 0x04: MnuTextList::setItems's labels
    const char *m_movie[7];     // ROR 0x08
    int         m_action[7];    // ROR 0x24
    int         m_size[7][2];   // ROR 0x40: w, h (w >= 640: MagSystem::resetDBuff384)
};
static_assert(offsetof(MnuTitleMoviePage, m_num) == 0x00, "m_num");

/* EE 0x5700 (MagLifeMenuTitle +0x40..+0x5740). layout: host -- SprMan and
 * the sub menus hold pointers; the EE offsets are in the comments. */
struct MnuTitle
{
    MnuTitle();                                                          // ROR 0x001a90ec
    ~MnuTitle();                                                         // ROR 0x001a9150

    void init(DogTexInfo *tex);                                          // ROR 0x001a91f4
    void term();                                                         // ROR 0x001a9258
    int  isInit();                                                       // ROR 0x001a9298
    /* -1 before open; 0 while running; m_result when the title closes. */
    int  processPad(BedPad *pad);                                        // ROR 0x001a92a0
    void draw(I3dLayer *layer);                                          // ROR 0x001a972c

    void loadTexture();                                                  // ROR 0x001a97b0
    /* 0, or -1 when already open. */
    int  open();                                                         // ROR 0x001a97f4
    int  close();                                                        // ROR 0x001a9828
    int  initCursors();                                                  // ROR 0x001a9844
    void setDisp();                                                      // ROR 0x001a988c
    int  update();                                                       // ROR 0x001a98dc

    /* The sub menus: 0 while running, 1 (2) when done. */
    int  execLogo(BedPad *pad);                                          // ROR 0x001a98fc
    int  execMain(BedPad *pad);                                          // ROR 0x001aa248
    int  execSelect(BedPad *pad);                                        // ROR 0x001aa7d0
    int  execMovieTest(BedPad *pad);                                     // ROR 0x001aabb4
    int  execLoad(BedPad *pad);                                          // ROR 0x001aaed0
    int  execOption(BedPad *pad);                                        // ROR 0x001aaf98
    int  execMovie(BedPad *pad);                                         // ROR 0x001ab04c

    /* PC: m_spr's sprite n (m_sprite is the SprMan's table). */
    SprSprite &sprite(int n) { return m_spr.m_sprite[n]; }

    MnuMcd      m_mcd;              // ROR 0x0000: the memory card check
    MnuSvld     m_svld;             // ROR 0x1400: the load screen (m_save 0)
    MnuOpt      m_opt;              // ROR 0x3b80: the options
    MnuTextList m_list;             // ROR 0x4c68: the movie viewer's list
    int         m_open;             // ROR 0x4d6c: open / close
    /* Cursors and their item counts: 0 the main items, 1..4 cleared by
     * the modes, 5 / 6 the movie viewer's page / row, 8 execSelect's. */
    int         m_cursor[9];        // ROR 0x4d70
    int         m_cursor_num[9];    // ROR 0x4d94: initCursors sets [0] 3, [1] 5, [2] 2
    int         m_timer;            // ROR 0x4db8: frame counter of the current step
    int         m_logo;             // ROR 0x4dbc: execLogo's picture (index into its table)
    int         m_idle;             // ROR 0x4dc0: execMain's frames without a choice
    int         m_mode;             // ROR 0x4dc4: processPad's step
    int         m_sub;              // ROR 0x4dc8: the sub menu (exec*)
    int         m_step;             // ROR 0x4dcc: the sub menu's step
    int         _4dd0[7];
    int         m_4dec;             // ROR 0x4dec: open sets 7; role unknown
    SprMan      m_spr;              // ROR 0x4df0: EE 0x8ec
    MagSystem  *m_system;           // ROR 0x56dc: MagSystem::m_current_system
    I3dLayer   *m_layer;            // ROR 0x56e0: draw's layer
    DogTexInfo *m_tex;              // ROR 0x56e4: init's (passed to the load / option screens)
    int         m_stream;           // ROR 0x56e8: the s100000.str stream, -1 none
    int         m_result;           // ROR 0x56ec: 1 timed out, 2 new game, 3 load, 4 options
    MagScene   *m_scene;            // ROR 0x56f0: MagLifeMenuTitle::init stores the scene
    int         m_init;             // ROR 0x56f4: isInit
};
static_assert(offsetof(MnuTitle, m_svld) >= 0x1400, "m_svld");
static_assert(offsetof(MnuTitle, m_opt) >= 0x3b80, "m_opt");
static_assert(offsetof(MnuTitle, m_open) >= 0x4d6c, "m_open");
static_assert(offsetof(MnuTitle, m_spr) >= 0x4df0, "m_spr");
static_assert(offsetof(MnuTitle, m_init) >= 0x56f4, "m_init");

#endif /* _MNU_MNUTITLE_H */
