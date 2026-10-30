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

/* Page 0's cursor areas (m_work[0]). */
#define MNUITEM_AREA_EQUIP  0       /* the equipped item */
#define MNUITEM_AREA_FIND   1       /* the find target */
#define MNUITEM_AREA_SLOTS  2       /* the 4 x 3 slot grid */
#define MNUITEM_AREA_SLOT12 3       /* the slot-12 item (0xa23) */

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
    /* The frame fade, the notice message, the life gauges; the sprites
     * and the text run. */
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
    /* Sprite 0x12: the name (system data field 0xc) of slot no's item. */
    void setNameText(int no);                                            // ROR 0x001bc790
    /* Field 0xc of item id when its flag 4 is set, else question marks. */
    const char *getKnownName(int id);                                    // ROR 0x001bcb84
    /* Sprites 0x6f..0x71: the names of the items in fields 0x10..0x12 of
     * slot no's item (with m_flag_c); anim 1 runs them. */
    void setLinkTexts(int no, int anim);                                 // ROR 0x001bcc08
    /* Fades the panel 0x6a..0x71 in when slot no's item has linked items,
     * out when it has none. */
    void showLinkPanel(int no);                                          // ROR 0x001bd028
    /* One of slot no's stacking items used: its count, the slot cleared at
     * the last; the slots redrawn. */
    void useSlotItem(int no);                                            // ROR 0x001af578
    /* Slot icons whose item record has none of bits dimmed (alpha 0x28),
     * the others full; 0x10008 shows them all full. */
    void dimSlots(u_int bits);                                           // ROR 0x001b05ec
    void buildFlagList();                                                // ROR 0x001baff4

    /* PC: m_spr's sprite n (m_sprite is the SprMan's table). */
    SprSprite &sprite(int n) { return m_spr.m_sprite[n]; }

    int           m_open;           // ROR 0x0000: open / close
    /* Cleared together (initWork, open after the boot load, processPad);
     * [0] 2, [3] 0 at open. Page 0: [0] the cursor's area (MNUITEM_AREA_*),
     * [1] its slot (0..11 in a 4 x 3 grid, 12 the slot-12 item; page 7
     * puts that slot's item away). [2] page 1's slot, [5] page 2's, [4]
     * page 7's choice (0 yes, 1 no). */
    int           m_work[10];       // ROR 0x0004
    int           m_2c;             // ROR 0x002c: initWork 4; unverified
    int           m_30;             // ROR 0x0030: initWork 2; unverified
    int           m_34;             // ROR 0x0034: initWork 2; unverified
    int           _38[7];
    int           m_step;           // ROR 0x0054: processPad's step
    int           m_page;           // ROR 0x0058: execPage's page (0..13)
    int           m_page_step;      // ROR 0x005c: the page's step
    /* The page page n returns to when it ends (its caller sets it: page 0
     * sets 3's, 4's and 10's to 0, pages 1 / 2 set 3's and 8's). */
    int           m_page_ret[14];   // ROR 0x0060
    SprMan        m_spr;            // ROR 0x0098: EE 0x8ec, the menu
    SprMan        m_spr_find;       // ROR 0x0984: findpt.rpk's one sprite
    SprMan        m_spr3;           // ROR 0x1270: drawn first; not set up here
    MagSystem    *m_system;         // ROR 0x1b5c: MagSystem::m_current_system (its scene fades)
    MagScene     *m_scene;          // ROR 0x1b60: MagLifeMenuItem::init stores the scene
    MnuTextList   m_list;           // ROR 0x1b64: EE 0x104
    MsgManSet     m_msg;            // ROR 0x1c68: the window's text
    DogTexInfo   *m_tex;            // ROR 0x23e0: init's texture table
    /* Life in percent, [0] the PC, [1] the dog: updateFrame steps the
     * shown rate one per frame toward the target (calcLifeRate sets both). */
    int           m_rate[2];        // ROR 0x23e4: shown on the gauges
    int           m_rate_target[2]; // ROR 0x23ec
    /* The gauges' levels [level][PC, dog] (open: 100, 70, 30): at or above
     * level 0 anim 1, 1 anim 1, 2 anim 2, below anim 3 (updateFrame). */
    int           m_rate_level[3][2]; // ROR 0x23f4
    int           m_flag_bit2;      // ROR 0x240c: sequence bit-word 1 bit 2
    int           m_flag_a;         // ROR 0x2410: bit 1 (frame sprite 0xf)
    int           m_flag_b;         // ROR 0x2414: bit 0, cleared in slot 7 (sprite 10, the equipped item)
    int           m_flag_c;         // ROR 0x2418: bit 1 (sprite 11, the find target)
    int           m_flag_d;         // ROR 0x241c: bit 0 (the slots)
    int           m_link_shown;     // ROR 0x2420: showLinkPanel's panel is in (open / step 1 clear it)
    int           m_equip_slot;     // ROR 0x2424: findSlot(GamMan m_equipItem), -1 none
    int           m_find_slot;      // ROR 0x2428: findSlot(GamMan m_findTargetItem), -1 none
    u_int         m_find_item;      // ROR 0x242c: slot 13's id (GamMan m_findTargetItem)
    u_int         m_slot12_item;    // ROR 0x2430: slot 12's id (open: 0xa23)
    GamItemSlot  *m_slot[MNUITEM_SLOT_MAX]; // ROR 0x2434: GamMan's item-have entries
    int           m_frame_timer;    // ROR 0x2464: updateFrame's frames since step 3 (to 18000)
    int           m_use_timer;      // ROR 0x2468: page 5's frames between ammo rounds (3)
    int           m_246c;           // ROR 0x246c: open clears it
    int           m_bit28;          // ROR 0x2470: sequence bit 0x10000000 at step 3 (page 9)
    int           m_bit29;          // ROR 0x2474: bit 0x20000000 (page 11; the frame stays hidden)
    int           _2478[2];
    int           m_init;           // ROR 0x2480: isInit
    int           m_use_num;        // ROR 0x2484: page 5: the ammo slot's count while it empties
    int           m_use_moves;      // ROR 0x2488: page 5: rounds still to move (6 - m_gun_num at most)
    int           m_248c;           // ROR 0x248c: step 0 clears it
    /* step 3: 1 with sequence bit-word 1 bit 3, 2 with bit 4, else 0;
     * updateFrame opens system message 6 / 7 at frame 150 for 1 / 2. */
    int           m_notice;         // ROR 0x2490
    int           m_notice_open;    // ROR 0x2494: the message is up (cross / circle close it)
    int           m_ready;          // ROR 0x2498: page 0 takes input (frame 60, or the message closed)
    int           m_249c;           // ROR 0x249c: page 0 closes the menu while not m_ready
    int           m_frame_mode;     // ROR 0x24a0: showFrame's mode
    /* buildFlagList: the system texts (column 5) of flag table 99's rows
     * whose sequence bit (column 3) is set, row 0 always; their row
     * numbers in m_flag_rows, -1 after the last (the ROM does not bound
     * it; 0x26e4 follows). */
    char          m_flag_text[0x200]; // ROR 0x24a4
    int           _26a0;
    int           m_flag_rows[0x10]; // ROR 0x26a4
    int           m_find_spr;       // ROR 0x26e4: the find-target frame sprite shown (page 5 sets 0x21)
    int           _26e8[5];
    int           m_win2;           // ROR 0x26fc: a second window, closed at the end when >= 0
    int           m_win;            // ROR 0x2700: the scene window
    int           _2704;
    int           m_gun_idx;        // ROR 0x2708: page 5: GamMan::getEquipSlot (scePrintf "m_gun_idx")
    int           m_gun_num;        // ROR 0x270c: page 5: GamMan::getSelItemNum, the gun's rounds
};
static_assert(offsetof(MnuItem, m_work) == 0x04, "m_work");
static_assert(offsetof(MnuItem, m_2c) == 0x2c, "m_2c");
static_assert(offsetof(MnuItem, m_step) == 0x54, "m_step");
static_assert(offsetof(MnuItem, m_page) == 0x58, "m_page");
static_assert(offsetof(MnuItem, m_page_step) == 0x5c, "m_page_step");
static_assert(offsetof(MnuItem, m_page_ret) == 0x60, "m_page_ret");
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
static_assert(offsetof(MnuItem, m_flag_text) >= 0x24a4, "m_flag_text");
static_assert(offsetof(MnuItem, m_flag_rows) >= 0x26a4, "m_flag_rows");
static_assert(offsetof(MnuItem, m_find_spr) >= 0x26e4, "m_find_spr");
static_assert(offsetof(MnuItem, m_gun_idx) >= 0x2708, "m_gun_idx");
static_assert(offsetof(MnuItem, m_win) >= 0x2700, "m_win");
static_assert(sizeof(MnuItem) >= 0x2710, "EE 0x2710");

#endif /* _MNU_MNUITEM_H */
