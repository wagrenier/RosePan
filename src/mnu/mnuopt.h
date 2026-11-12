/* ==========================================================================
 *  mnu/mnuopt.h
 *
 *  MnuOpt (mnu/mnuopt.cpp, 0x1ab474..0x1ad6b4): the options screen.
 *  MagLifeMenuOpt (mag/maglifemenu.h, menu 0x10006) holds one at +0x40;
 *  MnuTitle has one at +0x3b80 (its "Option" item). Class name from the
 *  wrapper (ror-inferred); no source string names the file.
 *
 *  A SprMan of 65 sprites over data/mnurpk/mnuopt.rpk and a scene window
 *  with system text 0x1c. processPad (exec) steps m_step: open the window,
 *  load the textures, set up the sprites and fade in, run execMain until
 *  it returns 1, fade out, close. execMain edits GamMan's option block
 *  (getStatus 0x15: &m_optOutput) in place, six rows (labels as the
 *  screen shows them):
 *    0 Sound: Stereo / Monaural (0 / 1; setOption 6)    sprites 0x15 / 0x16
 *    1 BGM vol. 1..9 (+0x1c; setOption 2)               sprites 0x22..0x2a
 *    2 SE vol. 1..9 (+0x18; setOption 3)                sprites 0x19..0x21
 *    3 Contrast 1..9 (+0x10; setOption 0)               sprites 0x2b..0x33
 *    4 Noir Filter 1..9 (+0x0c; setOption 1)            sprites 0x34..0x3c
 *    5 Vibration: On / Off (m_value 0 / 1; setOption 7) sprites 0x17 / 0x18
 *  The chosen value of each row draws full, the others dim (alpha 0x28);
 *  sprite 0x40 is the scribble cursor on the two-choice rows 0 and 5.
 *  Triangle (0x2000) leaves; when a direction was pressed (GamMan m_flags2
 *  0x400) every option is set again first. SprMan::volumeOn is called
 *  only while the Noir Filter row is chosen.
 * ======================================================================== */
#ifndef _MNU_MNUOPT_H
#define _MNU_MNUOPT_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "msg/msgman.h"
#include "spr/sprman.h"

struct BedPad;
struct DogTexInfo;
struct I3dLayer;
struct MagScene;
struct MagSystem;

/* The option rows' values as GamMan keeps them (getStatus 0x15 gives
 * &GamMan::m_optOutput; layout as GamMan +0x21f8c..). */
struct MnuOptValues
{
    int m_output;               // ROR 0x00: GamMan m_optOutput (option 6)
    int m_vibration;            // ROR 0x04: m_optVibration (option 7)
    int _08;
    int m_master;               // ROR 0x0c: m_optMasterLevel (option 1)
    int m_bright;               // ROR 0x10: m_optBrightLevel (option 0)
    int _14;
    int m_se;                   // ROR 0x18: m_optSeLevel (option 3)
    int m_str;                  // ROR 0x1c: m_optStrLevel (option 2)
};
static_assert(offsetof(MnuOptValues, m_master) == 0x0c, "m_master");
static_assert(offsetof(MnuOptValues, m_bright) == 0x10, "m_bright");
static_assert(offsetof(MnuOptValues, m_se) == 0x18, "m_se");
static_assert(offsetof(MnuOptValues, m_str) == 0x1c, "m_str");

#define MNUOPT_ROW_MAX 6

/* EE 0x10f0 (MagLifeMenuOpt +0x40..+0x1130). layout: host -- SprMan and
 * MsgManSet hold pointers; the EE offsets are in the comments. */
struct MnuOpt
{
    MnuOpt();                                                            // ROR 0x001ab474
    ~MnuOpt();                                                           // ROR 0x001ab4e0

    void init(DogTexInfo *tex);                                          // ROR 0x001ab598
    void term();                                                         // ROR 0x001ab5e4
    int  isInit();                                                       // ROR 0x001ab65c
    /* 0 while running, 1 when the screen has closed, -1 before open. */
    int  processPad(BedPad *pad);                                        // ROR 0x001ab66c
    void draw(I3dLayer *layer);                                          // ROR 0x001ab680

    /* 0, or -1 when already open / not open. */
    int  open();                                                         // ROR 0x001ab6ac
    int  close();                                                        // ROR 0x001ab6f4
    int  exec(BedPad *pad);                                              // ROR 0x001ab720
    int  initWork();                                                     // ROR 0x001aba44
    void setDisp();                                                      // ROR 0x001aba98
    int  update();                                                       // ROR 0x001abb24
    void fadeInItems();                                                  // ROR 0x001abb58
    /* 0 while editing, 1 once triangle closed it. */
    int  execMain(BedPad *pad);                                          // ROR 0x001ac5b0

    /* PC: m_spr's sprite n (m_sprite is the SprMan's table). */
    SprSprite &sprite(int n) { return m_spr.m_sprite[n]; }
    /* PC: sprite n's colour (RGBA). */
    void setColor(int n, int r, int g, int b, int a);

    int           m_open;           // ROR 0x0000: open / close
    int           m_cursor;         // ROR 0x0004: the row (0..5)
    /* The rows' values as shown (the same order as the rows): [0] sound,
     * [1] BGM, [2] SE, [3] contrast, [4] noir filter, [5] vibration off. */
    int           m_value[MNUOPT_ROW_MAX]; // ROR 0x0008
    int           _20;              // ROR 0x0020: cleared by initWork
    /* Cursor counts: [0] 6 rows, [1] 2, [2..4] 9, [5] 9, [6] 2 (only [0]
     * is read). */
    int           m_cursor_num[7];  // ROR 0x0024
    int           _40;
    int           m_step;           // ROR 0x0044: exec's step
    int           m_pause;          // ROR 0x0048: execMain is skipped while set (open clears it; nothing sets it)
    int           m_main_step;      // ROR 0x004c: execMain's step
    int           _50;
    int           m_54;             // ROR 0x0054: open sets 1; role unknown
    MsgManSet     m_msg;            // ROR 0x0058: the window's text
    SprMan        m_spr;            // ROR 0x07d0: EE 0x8ec
    MagSystem    *m_system;         // ROR 0x10bc: MagSystem::m_current_system (its scene fades)
    I3dLayer     *m_layer;          // ROR 0x10c0: draw's layer
    int           m_result;         // ROR 0x10c4: 1 once triangle was pressed
    MagScene     *m_scene;          // ROR 0x10c8: MagLifeMenuOpt::init stores the scene
    int           m_init;           // ROR 0x10cc: isInit
    int           m_vib_time;       // ROR 0x10d0: 0xf when vibration was switched on, 0 off (not read here)
    int           m_10d4;           // ROR 0x10d4: execMain sets -1; role unknown
    int           m_timer;          // ROR 0x10d8: execMain's frames before input is taken
    MnuOptValues *m_values;         // ROR 0x10dc: GamMan's option block
    int           m_win;            // ROR 0x10e0: the scene window
    int           m_vib_off;        // ROR 0x10e4: vibration was off: switching it on pulses the pad
};
static_assert(offsetof(MnuOpt, m_cursor) == 0x04, "m_cursor");
static_assert(offsetof(MnuOpt, m_value) == 0x08, "m_value");
static_assert(offsetof(MnuOpt, m_cursor_num) == 0x24, "m_cursor_num");
static_assert(offsetof(MnuOpt, m_step) == 0x44, "m_step");
static_assert(offsetof(MnuOpt, m_main_step) == 0x4c, "m_main_step");
static_assert(offsetof(MnuOpt, m_54) == 0x54, "m_54");
static_assert(offsetof(MnuOpt, m_msg) == 0x58, "m_msg");
static_assert(offsetof(MnuOpt, m_spr) >= 0x7d0, "m_spr");
static_assert(offsetof(MnuOpt, m_scene) >= 0x10c8, "m_scene");
static_assert(offsetof(MnuOpt, m_vib_off) >= 0x10e4, "m_vib_off");

#endif /* _MNU_MNUOPT_H */
