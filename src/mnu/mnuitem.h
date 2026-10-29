/* ==========================================================================
 *  mnu/mnuitem.h
 *
 *  MnuItem (mnu/mnuitem.cpp, 0x1ae954..0x1bdae4): the item menu.
 *  MagLifeMenuItem (mag/maglifemenu.h, menu 0x10000) holds one at +0x40.
 *  Class name from the ELF's symbols (MnuItem::MnuItem / ~MnuItem); no
 *  source string names the file.
 *
 *  Three sprite managers: m_spr (120 sprites over data/mnurpk/mnuitem.rpk,
 *  the pattern table 0x2acb10 and display list 0x2b6f40), m_spr_find (one
 *  sprite, data/mnurpk/findpt.rpk, while sequence bit 0x100 of bit-word 1
 *  is set) and m_spr3; draw draws m_spr3, m_spr_find, m_spr in that order.
 *  processPad steps m_step: open the window with system text 0x17 (0),
 *  textures and slots (1, 2 for findpt), sprites and fade in (3), the page
 *  handlers every frame (4) until one returns 1 (step 6), fade out (6, 7)
 *  and close (7, which may set the item's sequence flag).
 *
 *  Slots: m_slot[0..11] point at GamMan's twelve item-have entries
 *  (GamItemSlot, id 0xa00 = none); slot 12 is the item id m_slot12_item
 *  (0xa23), slot 13 the find target m_find_item. Sprites 0x30 + n are the
 *  slot icons (anim table 0x2b60c0, pattern base 0xd0 + 4 * (system data
 *  field 0xd - 1)), 0x3d + n their count text ("%2d", stacking items and
 *  0xa3e / 0xa43), 0x49 / 0x4f the equipped item and its cursor, 0x4b /
 *  0x50 the find target. showFrame fades the frame sprites by the four
 *  menu flags (+0x2410..: sequence bit-word 1 bits 1, 0, 1, 0).
 * ======================================================================== */
#ifndef _MNU_MNUITEM_H
#define _MNU_MNUITEM_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "mnu/mnuselect.h"
#include "msg/msgman.h"
#include "spr/sprman.h"

struct BedPad;
struct DogTexInfo;
struct GamItemSlot;
struct I3dLayer;
struct MagScene;
struct MagSystem;

/* Item ids the menu treats specially. */
#define MNUITEM_ID_NONE     0xa00   /* an empty slot (GamItemSlot::m_id) */
#define MNUITEM_ID_EMPTY    0xb3b   /* drawn as an empty slot */
#define MNUITEM_SLOT_MAX    12      /* GamMan's item-have slots */

/* EE 0x2710 (MagLifeMenuItem +0x40..+0x2750). layout: host -- three SprMan,
 * a MnuTextList and a MsgManSet hold pointers; the EE offsets are in the
 * comments. */
struct MnuItem
{
    MnuItem();                                                           // ROR 0x001ae954
    ~MnuItem();                                                          // ROR 0x001ae9f4

    void init(DogTexInfo *tex);                                          // ROR 0x001aeacc
    void initWork();                                                     // ROR 0x001aeb7c
    void term();                                                         // ROR 0x001aebc8
    int  isInit();                                                       // ROR 0x001aec48
    void loadTexList();                                                  // ROR 0x001aec50
    void draw(I3dLayer *layer);                                          // ROR 0x001aed08
    /* 0, or -1 when already open / not open. */
    int  open();                                                         // ROR 0x001aed5c
    int  close();                                                        // ROR 0x001aee50
    void loadTex();                                                      // ROR 0x001aee8c
    void freeTexFiles();                                                 // ROR 0x001aef14
    void releaseTex();                                                   // ROR 0x001aef9c
    void calcLifeRate();                                                 // ROR 0x001af024
    void readMenuFlags();                                                // ROR 0x001af130
    /* Slot no (every slot when < 0) from GamMan's item-have vector. */
    void setSlot(int no);                                                // ROR 0x001af21c
    /* The slot holding id: 0..11, 13 the find target, else 12; -1 for
     * MNUITEM_ID_NONE. */
    int  findSlot(u_int id);                                             // ROR 0x001af3f0
    /* mode 0 only: life rates, slots, equipped and find-target slots. */
    void refreshSlots(int mode);                                         // ROR 0x001af44c
    /* EE address of the item's anim table in the pattern image. */
    u_int getAnimTable(int id);                                          // ROR 0x001af4d0
    int  getPatBase(int id);                                             // ROR 0x001af4fc
    void setSlotSprites();                                               // ROR 0x001af5e0
    void setEquipSprites();                                              // ROR 0x001af928
    /* 0 hides the frame, 1 fades it in over 10 frames, 2 over 1. */
    void showFrame(int mode);                                            // ROR 0x001aff5c
    int  testSlotFlags(int no, u_int bits);                              // ROR 0x001b070c
    void setDisp();                                                      // ROR 0x001b0784
    /* 0 while open, 1 when the menu has closed, -1 before open. */
    int  processPad(BedPad *pad);                                        // ROR 0x001b099c
    void updateFrame(BedPad *pad);                                       // ROR 0x001b113c
    int  getSlotItem(int no);                                            // ROR 0x001b1b34
    int  getZero();                                                      // ROR 0x001b1b98
    /* The page handler of m_page; one returning 1 starts the fade out. */
    void execPage(BedPad *pad);                                          // ROR 0x001b1ba0
    int  execPage0(BedPad *pad);                                         // ROR 0x001b1cd4
    int  execPage1(BedPad *pad);                                         // ROR 0x001b2f64
    int  execPage2(BedPad *pad);                                         // ROR 0x001b3534
    int  execPage8(BedPad *pad);                                         // ROR 0x001b3e20
    int  execPage3(BedPad *pad);                                         // ROR 0x001b3e28
    int  execPage4(BedPad *pad);                                         // ROR 0x001b4488
    int  execPage5(BedPad *pad);                                         // ROR 0x001b5c20
    int  execPage7(BedPad *pad);                                         // ROR 0x001b64c4
    int  execPage6(BedPad *pad);                                         // ROR 0x001ba3a0
    int  execPage9(BedPad *pad);                                         // ROR 0x001ba8e4
    int  execPage10(BedPad *pad);                                        // ROR 0x001bb12c
    int  execPage11(BedPad *pad);                                        // ROR 0x001bb4f0
    int  execPage12(BedPad *pad);                                        // ROR 0x001bbcfc
    int  execPage13(BedPad *pad);                                        // ROR 0x001bc370

    /* PC: m_spr's sprite n (m_sprite is the SprMan's table). */
    SprSprite &sprite(int n) { return m_spr.m_sprite[n]; }

    int           m_open;           // ROR 0x0000: open / close
    /* Cleared together (initWork, open after the boot load, processPad);
     * [0] 2, [3] 0 at open. Roles not known yet (the page handlers). */
    int           m_work[10];       // ROR 0x0004
    int           m_2c;             // ROR 0x002c: initWork 4; unverified
    int           m_30;             // ROR 0x0030: initWork 2; unverified
    int           m_34;             // ROR 0x0034: initWork 2; unverified
    int           _38[7];
    int           m_step;           // ROR 0x0054: processPad's step
    int           m_page;           // ROR 0x0058: execPage's page (0..13)
    int           m_page_step;      // ROR 0x005c: the page's step
    int           _60[14];
    SprMan        m_spr;            // ROR 0x0098: EE 0x8ec, the menu
    SprMan        m_spr_find;       // ROR 0x0984: findpt.rpk's one sprite
    SprMan        m_spr3;           // ROR 0x1270: drawn first; not set up here
    MagSystem    *m_system;         // ROR 0x1b5c: MagSystem::m_current_system (its scene fades)
    MagScene     *m_scene;          // ROR 0x1b60: MagLifeMenuItem::init stores the scene
    MnuTextList   m_list;           // ROR 0x1b64: EE 0x104
    MsgManSet     m_msg;            // ROR 0x1c68: the window's text
    DogTexInfo   *m_tex;            // ROR 0x23e0: init's texture table
    int           m_pc_rate;        // ROR 0x23e4: the PC's life in percent
    int           m_dog_rate;       // ROR 0x23e8: the dog's life in percent
    int           m_pc_rate2;       // ROR 0x23ec: calcLifeRate writes the same
    int           m_dog_rate2;      // ROR 0x23f0
    /* open: 100, 100, 70, 70, 30, 30 (pairs, PC then dog); unverified as
     * the life gauge's thresholds. */
    int           m_rate_level[6];  // ROR 0x23f4
    int           m_flag_bit2;      // ROR 0x240c: sequence bit-word 1 bit 2
    int           m_flag_a;         // ROR 0x2410: bit 1 (frame sprite 0xf)
    int           m_flag_b;         // ROR 0x2414: bit 0, cleared in slot 7 (sprite 10, the equipped item)
    int           m_flag_c;         // ROR 0x2418: bit 1 (sprite 11, the find target)
    int           m_flag_d;         // ROR 0x241c: bit 0 (the slots)
    int           m_2420;           // ROR 0x2420: open / step 1 clear it
    int           m_equip_slot;     // ROR 0x2424: findSlot(GamMan m_equipItem), -1 none
    int           m_find_slot;      // ROR 0x2428: findSlot(GamMan m_findTargetItem), -1 none
    u_int         m_find_item;      // ROR 0x242c: slot 13's id (GamMan m_findTargetItem)
    u_int         m_slot12_item;    // ROR 0x2430: slot 12's id (open: 0xa23)
    GamItemSlot  *m_slot[MNUITEM_SLOT_MAX]; // ROR 0x2434: GamMan's item-have entries
    int           m_2464;           // ROR 0x2464: step 3 clears it
    int           _2468;
    int           m_246c;           // ROR 0x246c: open clears it
    int           m_bit28;          // ROR 0x2470: sequence bit 0x10000000 at step 3 (page 9)
    int           m_bit29;          // ROR 0x2474: bit 0x20000000 (page 11; the frame stays hidden)
    int           _2478[2];
    int           m_init;           // ROR 0x2480: isInit
    int           _2484[2];
    int           m_248c;           // ROR 0x248c: step 0 clears it
    int           m_2490;           // ROR 0x2490: step 3: 1 with bit 3, 2 with bit 4, else 0
    int           m_2494;           // ROR 0x2494: step 3 clears it
    int           m_2498;           // ROR 0x2498: step 3 clears it
    int           m_249c;           // ROR 0x249c
    int           m_frame_mode;     // ROR 0x24a0: showFrame's mode
    u_char        _24a4[0x26fc - 0x24a4];
    int           m_win2;           // ROR 0x26fc: a second window, closed at the end when >= 0
    int           m_win;            // ROR 0x2700: the scene window
    u_char        _2704[0x2710 - 0x2704];
};
static_assert(offsetof(MnuItem, m_work) == 0x04, "m_work");
static_assert(offsetof(MnuItem, m_2c) == 0x2c, "m_2c");
static_assert(offsetof(MnuItem, m_step) == 0x54, "m_step");
static_assert(offsetof(MnuItem, m_page) == 0x58, "m_page");
static_assert(offsetof(MnuItem, m_page_step) == 0x5c, "m_page_step");
static_assert(offsetof(MnuItem, m_spr) == 0x98, "m_spr");
static_assert(offsetof(MnuItem, m_spr_find) >= 0x984, "m_spr_find");
static_assert(offsetof(MnuItem, m_spr3) >= 0x1270, "m_spr3");
static_assert(offsetof(MnuItem, m_system) >= 0x1b5c, "m_system");
static_assert(offsetof(MnuItem, m_scene) >= 0x1b60, "m_scene");
static_assert(offsetof(MnuItem, m_msg) >= 0x1c68, "m_msg");
static_assert(offsetof(MnuItem, m_tex) >= 0x23e0, "m_tex");
static_assert(offsetof(MnuItem, m_slot) >= 0x2434, "m_slot");
static_assert(offsetof(MnuItem, m_init) >= 0x2480, "m_init");
static_assert(offsetof(MnuItem, m_frame_mode) >= 0x24a0, "m_frame_mode");
static_assert(offsetof(MnuItem, m_win) >= 0x2700, "m_win");
static_assert(sizeof(MnuItem) >= 0x2710, "EE 0x2710");

#endif /* _MNU_MNUITEM_H */
