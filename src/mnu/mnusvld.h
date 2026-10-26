/* ==========================================================================
 *  mnu/mnusvld.h
 *
 *  MnuSvld (mnu/mnusvld.cpp, 0x1a59b8..0x1a90ec): the memory card save /
 *  load screen, one class for both. MagLifeMenuSave and MagLifeMenuLoad
 *  (mag/maglifemenu.h) hold one at +0x40 and set m_save (1 save, 0 load)
 *  in their constructors; MnuTitle has one at +0x1400. Class name from the
 *  file (ror-inferred).
 *
 *  It is a MnuMcd (the card work, at +0) with a SprMan of 73 sprites over
 *  data/mnurpk/mnusvld.rpk and a scene window for the card messages.
 *  processPad (exec) steps m_step: open the window and set up the sprites,
 *  fade in, run the sub screen m_sub until it leaves, fade out, close. The
 *  sub screens: 0 the slot list (execList), 1 the card check that fills the
 *  list (execCheck), 2 save into the chosen slot (execSave), 3 load from it
 *  (execLoad); each steps m_sub_step and returns what to run next (999
 *  leave). Three more (execIdle, execConfirm, execGrid) are reached by no
 *  code.
 *
 *  A save is one card directory per slot, /BASLUS-21448/dtSSabCCHHMM: slot
 *  SS, then two flags (a: always 0, b: 1 when sequence bit 0x80000000 of
 *  word 0 is set), chapter CC, play time HH:MM. readSlots parses the names
 *  back into m_slot[] (MnuSvldSlot) with the directory's modify date.
 *
 *  The sprites (display list data 0x2a64e0): 1 the list (scrolls), 5..16
 *  the rows' frames, 17..28 the rows, 29..40 the chapter pictures, 41..52
 *  the play times, 53 the cursor, 58 / 59 / 70..72 the frame, 60 / 61 the
 *  up / down arrows, 62 the save / load title, 64..67 the info texts (date,
 *  chapter title, corrupted), 69 the busy mark.
 * ======================================================================== */
#ifndef _MNU_MNUSVLD_H
#define _MNU_MNUSVLD_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "mnu/mnumcd.h"
#include "msg/msgman.h"
#include "spr/sprman.h"

struct BedPad;
struct DogTexInfo;
struct I3dLayer;
struct MagScene;
struct MagSystem;

/* A save slot as the list shows it (EE 0x2c, 12 at MnuSvld +0x2508):
 * eleven ints, all -1 when the slot is empty (clearSlots). */
struct MnuSvldSlot
{
    int m_flag[2];              // ROR 0x00: the name's two flag digits; [0] >= 0 = used, 1 = corrupted
    int m_chapter;              // ROR 0x08: 0..12 (the chapter picture and title)
    int m_hour;                 // ROR 0x0c: play time
    int m_minute;               // ROR 0x10
    int m_year;                 // ROR 0x14: the directory's modify date, local time (year - 2000)
    int m_month;                // ROR 0x18
    int m_day;                  // ROR 0x1c
    int m_dhour;                // ROR 0x20
    int m_dminute;              // ROR 0x24
    int m_dsecond;              // ROR 0x28
};
static_assert(offsetof(MnuSvldSlot, m_chapter) == 0x08, "m_chapter");
static_assert(offsetof(MnuSvldSlot, m_hour) == 0x0c, "m_hour");
static_assert(offsetof(MnuSvldSlot, m_minute) == 0x10, "m_minute");
static_assert(offsetof(MnuSvldSlot, m_year) == 0x14, "m_year");
static_assert(offsetof(MnuSvldSlot, m_dhour) == 0x20, "m_dhour");
static_assert(offsetof(MnuSvldSlot, m_dsecond) == 0x28, "m_dsecond");
static_assert(sizeof(MnuSvldSlot) == 0x2c, "MnuSvldSlot is 0x2c bytes on the EE");

#define MNUSVLD_SLOT_MAX 12

/* EE 0x2780 (MagLifeMenuSave +0x40..+0x27c0). layout: host -- MnuMcd and
 * SprMan hold pointers; the EE offsets are in the comments. */
struct MnuSvld
{
    MnuSvld();                                                           // ROR 0x001a59b8
    ~MnuSvld();                                                          // ROR 0x001a5a34

    void init(DogTexInfo *tex);                                          // ROR 0x001a5af4
    void term();                                                         // ROR 0x001a5bf4
    int  isInit();                                                       // ROR 0x001a5c78
    /* 0 while running, 1 when the screen has closed, -1 before open. */
    int  processPad(BedPad *pad);                                        // ROR 0x001a5cc4
    void draw(I3dLayer *layer);                                          // ROR 0x001a5cd8

    void loadTexture();                                                  // ROR 0x001a5c80
    int  open();                                                         // ROR 0x001a5d04
    int  close();                                                        // ROR 0x001a5d74
    void showWindow(int on);                                             // ROR 0x001a5da0
    int  exec(BedPad *pad);                                              // ROR 0x001a5ed0
    int  initWork();                                                     // ROR 0x001a65f0
    void setDisp();                                                      // ROR 0x001a665c
    int  update();                                                       // ROR 0x001a66f0
    void *makeSaveData();                                                // ROR 0x001a6764
    int  applyLoadData();                                                // ROR 0x001a67d8
    int  isSlotUsed(int slot);                                           // ROR 0x001a6830
    void makeFileName(int slot, char *buf);                              // ROR 0x001a6858
    void makeLoadName(int slot, char *buf);                              // ROR 0x001a68e0
    void makeSaveName(int slot, char *buf);                              // ROR 0x001a6968
    void makeSaveDir(int slot, char *buf);                               // ROR 0x001a6ab8
    void clearSlots();                                                   // ROR 0x001a6bf8
    void setSlotDisp(int row, int slot);                                 // ROR 0x001a6c90
    void setSlotsDisp();                                                 // ROR 0x001a6fe8
    void readSlots();                                                    // ROR 0x001a7088
    void scrollList(int dir);                                            // ROR 0x001a7418
    /* row 12 = every row; mode 0 dim, 1 flash, 2 clear, 3 frame grey, 4
     * frame full. */
    void setRowColor(int row, int mode);                                 // ROR 0x001a7498
    void setInfo(int slot);                                              // ROR 0x001a7c54
    /* The sub screens (m_sub); see the file comment for what they return. */
    int  execIdle(BedPad *pad);                                          // ROR 0x001a7f40
    int  execList(BedPad *pad);                                          // ROR 0x001a8000
    int  execCheck(BedPad *pad);                                         // ROR 0x001a8730
    int  execSave(BedPad *pad);                                          // ROR 0x001a891c
    int  execLoad(BedPad *pad);                                          // ROR 0x001a8b7c
    int  execConfirm(BedPad *pad);                                       // ROR 0x001a8d44
    int  execGrid(BedPad *pad);                                          // ROR 0x001a8e84
    int  empty1a90dc(int item);                                          // ROR 0x001a90dc
    int  empty1a90e4(int item);                                          // ROR 0x001a90e4

    /* PC: m_spr's sprite n (m_sprite is the SprMan's table). */
    SprSprite &sprite(int n) { return m_spr.m_sprite[n]; }

    MnuMcd      m_mcd;          // ROR 0x0000: the card work (MnuSvld calls its methods on this)
    int         m_open;         // ROR 0x1400: open / close
    /* Cursors and their counts (as MnuTitle's): [1] the slot, [2] its row
     * on screen, [5] execConfirm's, [8] execGrid's; counts [1] 12 slots,
     * [2] 5 rows shown, [8] 36. */
    int         m_cursor[9];    // ROR 0x1404
    int         m_cursor_num[9]; // ROR 0x1428
    int         m_step;         // ROR 0x144c: exec's step
    int         m_sub;          // ROR 0x1450: the sub screen
    int         m_sub_step;     // ROR 0x1454: the sub screen's step
    int         _1458[6];
    int         m_1470;         // ROR 0x1470: open sets 6; role unknown
    SprMan      m_spr;          // ROR 0x1474: EE 0x8ec
    MagSystem  *m_system;       // ROR 0x1d60: MagSystem::m_current_system (its scene fades)
    I3dLayer   *m_layer;        // ROR 0x1d64: draw's layer
    MsgManSet   m_msg;          // ROR 0x1d68: the window's text
    int         m_result;       // ROR 0x24e0: the last card check's MnuMcd::m_result
    int         m_24e4;         // ROR 0x24e4: open sets 3; role unknown
    int         m_save;         // ROR 0x24e8: 1 the save screen, 0 the load screen (the wrappers' constructors)
    int         m_grid_state;   // ROR 0x24ec: execGrid's (0 moving, 1 / 2 decided); open clears it
    int         m_24f0;         // ROR 0x24f0: initWork clears it
    int         m_24f4;         // ROR 0x24f4: exec sets 1 after the fade in; execGrid tests 2
    int         _24f8;
    int         m_24fc;         // ROR 0x24fc: open / execConfirm clear it
    int         m_2500;         // ROR 0x2500: open clears it
    int         m_2504;         // ROR 0x2504: open clears it
    MnuSvldSlot m_slot[MNUSVLD_SLOT_MAX]; // ROR 0x2508
    u_char      _2718[0x2730 - 0x2718];
    MagScene   *m_scene;        // ROR 0x2730: MagLifeMenuSave / Load::init store the scene
    int         m_init;         // ROR 0x2734: isInit
    int         m_slot_ok;      // ROR 0x2738: execList: the card answered with a usable state
    int         m_decide;       // ROR 0x273c: execList: 0 choosing, 1 decided, 2 cancelled
    void       *m_svldBuf;      // ROR 0x2740: the 0x3000-byte save image (the asserts name it)
    int         m_win;          // ROR 0x2744: the scene window
    int         m_win_shown;    // ROR 0x2748: showWindow's state
    int         m_accessed;     // ROR 0x274c: set once a card check or save / load finished
    int         _2750;
    int         m_loaded;       // ROR 0x2754: 1 after a game was loaded (MnuTitle::execLoad reads it)
    u_char      _2758[0x2780 - 0x2758];
};
static_assert(offsetof(MnuSvld, m_mcd) == 0, "m_mcd");
static_assert(offsetof(MnuSvld, m_open) >= 0x1400, "m_open");
static_assert(offsetof(MnuSvld, m_spr) >= 0x1474, "m_spr");
static_assert(offsetof(MnuSvld, m_msg) >= 0x1d68, "m_msg");
static_assert(offsetof(MnuSvld, m_save) >= 0x24e8, "m_save");
static_assert(offsetof(MnuSvld, m_slot) >= 0x2508, "m_slot");
static_assert(offsetof(MnuSvld, m_scene) >= 0x2730, "m_scene");
static_assert(offsetof(MnuSvld, m_loaded) >= 0x2754, "m_loaded");

/* Card date conversions (no object). */
int mnuBcdToInt(u_int b);                                                // ROR 0x001a7040
int mnuIntToBcd(int n);                                                  // ROR 0x001a7060
/* The info line's chapter title: the text's leading full-width spaces,
 * then the rest in brackets. */
char *mnuSvldMakeTitle(char *buf, const char *text);                     // ROR 0x001a7b90

#endif /* _MNU_MNUSVLD_H */
