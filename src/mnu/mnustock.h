/* ==========================================================================
 *  mnu/mnustock.h
 *
 *  MnuStock (mnu/mnustock.cpp, 0x19a52c..0x1a066c): the item box, where
 *  items move between Jennifer's 12 item slots (GamMan::m_itemHave) and
 *  the stock (GamMan::m_itemStock, the pool's used list).
 *  MagLifeMenuStock (mag/maglifemenu.h, menu 0x10002) holds one at +0x40.
 *  Class name from the wrapper (ror-symbol ctor / dtor); the file name from
 *  the data/mnurpk/mnustock.rpk it loads (no source string names it).
 *
 *  The same frame as MnuOpt: a SprMan of 138 sprites over mnustock.rpk
 *  and a scene window with system text 0x1b. processPad (exec) steps
 *  m_step: open the window, load the textures, set up the lists and the
 *  sprites, fade in with the button guides 4 / 5, run the page m_page
 *  (0 / 1 / 2: execPage0 / 1 / 2) until it returns 1, fade out, close.
 *
 *  The left side is the stock list: entry 0 is a fixed "----" item (id
 *  0xa00: taking it puts the slot's item back), then the stock items of
 *  the category m_category (0 all, 1..6 GamSysRec5::m_b8), a window of
 *  m_rows rows (sprites 7..0xf, their counts 0x10..0x18) at m_index /
 *  m_row, a scroll bar 0x37 and arrows 0x35 / 0x36. The right side is the
 *  slot grid (4 x 3, icons 0x4d.., counts 0x59..), the equipped item's
 *  slot marked by 0x65, the find target's by 0x66.
 * ======================================================================== */
#ifndef _MNU_MNUSTOCK_H
#define _MNU_MNUSTOCK_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "msg/msgman.h"
#include "spr/sprman.h"

struct BedPad;
struct DogTexInfo;
struct GamItemSlot;
struct I3dLayer;
struct MagScene;
struct MagSystem;

#define MNUSTOCK_LIST_MAX   0x100   /* m_list entries (the ROM does not bound the stock) */
#define MNUSTOCK_SLOT_MAX   12      /* GamMan's item slots */
#define MNUSTOCK_ROW_MAX    9       /* row sprites (m_rows of them show) */

/* EE 0x15b0 (MagLifeMenuStock +0x40..+0x15f0). layout: host -- SprMan
 * and MsgManSet hold pointers; the EE offsets are in the comments. */
struct MnuStock
{
    MnuStock();                                                          // ROR 0x0019a52c
    ~MnuStock();                                                         // ROR 0x0019a598

    void init(DogTexInfo *tex);                                          // ROR 0x0019a650
    void term();                                                         // ROR 0x0019a6b8
    int  isInit();                                                       // ROR 0x0019a730
    /* 0 while running, 1 when the box has closed, -1 before open. */
    int  processPad(BedPad *pad);                                        // ROR 0x0019a740
    void draw(I3dLayer *layer);                                          // ROR 0x0019a754

    /* 0, or -1 when already open / not open. */
    int  open();                                                         // ROR 0x0019a780
    int  close();                                                        // ROR 0x0019a7d8
    int  exec(BedPad *pad);                                              // ROR 0x0019a824
    int  initWork();                                                     // ROR 0x0019ae20
    void readMenuFlags();                                                // ROR 0x0019ae78
    /* The EE address of item id's anim table in the pattern image. */
    u_int getAnimTable(int id);                                          // ROR 0x0019af08
    int  getPatBase(int id);                                             // ROR 0x0019af34
    /* 1 when rows lie above (dir 0) / below (dir 1) the window. */
    int  canScroll(int dir);                                             // ROR 0x0019afb0
    void buildStockList(int category);                                   // ROR 0x0019b020
    /* Slot pointers: slot (>= 0) one, -1 all, -2 none; then every slot's
     * icon and count sprite. */
    void setSlotSprites(int slot);                                       // ROR 0x0019b384
    void setSlotDisp();                                                  // ROR 0x0019b7f4
    void updateRows();                                                   // ROR 0x0019bd6c
    /* List entry index into item slot slot; 1 when something moved. */
    int  swapItem(int slot, int index);                                  // ROR 0x0019beb0
    /* The slot holding id, -1 for none. */
    int  findSlot(int id);                                               // ROR 0x0019c048
    void setupLists(int mode);                                           // ROR 0x0019c0e4
    void setDisp();                                                      // ROR 0x0019c16c
    /* Scrolls the 8 lower list rows (sprites and the m_row_spr / m_row_spr2
     * order) by one: dir 0 up the screen (the cursor went down), 1 down. */
    void setRowMove(int dir);                                            // ROR 0x0019c630
    int  update();                                                       // ROR 0x0019ceac
    /* The pages: 0 while running, 1 once the box closes. */
    /* Page 0: the category tabs 0x2e.. (up / down), cross to the list
     * (page 1), circle closes. */
    int  execPage0(BedPad *pad);                                         // ROR 0x0019cf48
    void setRowName(int no, int index);                                  // ROR 0x0019dbc8
    void setRowCount(int no, int row, int index);                        // ROR 0x0019dcbc
    const char *getItemName(int id);                                     // ROR 0x0019dedc
    void copyMesText(char *dst, const char *src);                        // ROR 0x0019df78
    /* The description sprites 0x3e..0x45 for item slot (its text field
     * 0x25 in three lines, the three linked items' names, its name); NULL
     * sets only their clear rects. */
    void setDescText(const GamItemSlot *slot);                           // ROR 0x0019e010
    int  getScrollBarPos(int index, int count);                          // ROR 0x0019e43c
    /* Page 1: the stock list (up / down, L1 / R1 a window), cross picks the
     * entry for a slot (page 2) or swaps it in, circle back to page 0. */
    int  execPage1(BedPad *pad);                                         // ROR 0x0019e474
    int  testItemFlags(int id, u_int bits);                              // ROR 0x0019fb88
    int  execPage2(BedPad *pad);                                         // ROR 0x0019fbc8 (not written)

    /* PC: m_spr's sprite n (m_sprite is the SprMan's table). */
    SprSprite &sprite(int n) { return m_spr.m_sprite[n]; }
    /* PC: initWork's clear of the work words +0x04..+0x1c (exec step 0
     * repeats it when g_bss_7a3d4c is set). */
    void clearWork();

    int           m_open;           // ROR 0x0000: open / close
    int           m_category;       // ROR 0x0004: buildStockList's category (0 all)
    int           m_row;            // ROR 0x0008: the cursor's row in the window (0..m_rows-1)
    int           m_slot;           // ROR 0x000c: the slot setSlotSprites redraws after a swap
    int           m_index;          // ROR 0x0010: the cursor's list entry
    int           _14[3];           // ROR 0x0014: cleared with the work words
    int           m_category_num;   // ROR 0x0020: the categories (tabs 0x2e..), initWork sets 7
    int           m_rows;           // ROR 0x0024: the window's rows, 8
    int           m_28;             // ROR 0x0028: initWork sets 2
    int           m_count;          // ROR 0x002c: the list's entries
    int           _30[3];
    int           m_step;           // ROR 0x003c: exec's step
    int           m_page;           // ROR 0x0040: exec step 4's page (0..2)
    int           m_main_step;      // ROR 0x0044: the pages' step (open / exec's end clear it)
    int           _48[3];
    int           m_54;             // ROR 0x0054: open sets 3; role unknown
    MsgManSet     m_msg;            // ROR 0x0058: the window's text
    SprMan        m_spr;            // ROR 0x07d0: EE 0x8ec
    MagSystem    *m_system;         // ROR 0x10bc: MagSystem::m_current_system (its scene fades)
    I3dLayer     *m_layer;          // ROR 0x10c0: draw's layer
    int           m_row_spr[MNUSTOCK_ROW_MAX];  // ROR 0x10c4: the row sprites 7..0xf (+9: their counts)
    int           m_row_spr2[MNUSTOCK_ROW_MAX]; // ROR 0x10e8: sprites 0x19..0x21 (move table 0x2a30c8)
    MagScene     *m_scene;          // ROR 0x110c: MagLifeMenuStock::init stores the scene
    /* The list: [0] the none entry, then stock pool items. */
    GamItemSlot  *m_list[MNUSTOCK_LIST_MAX];    // ROR 0x1110
    GamItemSlot  *m_have[MNUSTOCK_SLOT_MAX];    // ROR 0x1510: GamMan's item slots
    int           m_init;           // ROR 0x1540: isInit
    int           m_list_num;       // ROR 0x1544: m_list's entries (as m_count)
    int           _1548;
    int           m_equip_slot;     // ROR 0x154c: the equipped item's slot, -1 none
    int           m_find_slot;      // ROR 0x1550: the find target's slot, -1 none
    int           m_fly_spr;        // ROR 0x1554: the work sprite flying the swapped icon
    int           _1558[2];
    int           m_flag_bit2;      // ROR 0x1560: sequence bit-word 1 bit 2
    int           m_flag_a;         // ROR 0x1564: bit 1
    int           m_flag_b;         // ROR 0x1568: bit 0
    int           m_flag_c;         // ROR 0x156c: bit 1 (init sets 1)
    int           m_flag_d;         // ROR 0x1570: bit 0
    int           m_fade_wait;      // ROR 0x1574: the arrows' fade-in wait: 0x1e, 5 from frame 10
    int           _1578;
    int           m_timer;          // ROR 0x157c: frames since the box opened (to 0x7fff)
    int           m_win;            // ROR 0x1580: the scene window
    int           _1584[11];
};
static_assert(offsetof(MnuStock, m_category) == 0x04, "m_category");
static_assert(offsetof(MnuStock, m_index) == 0x10, "m_index");
static_assert(offsetof(MnuStock, m_rows) == 0x24, "m_rows");
static_assert(offsetof(MnuStock, m_count) == 0x2c, "m_count");
static_assert(offsetof(MnuStock, m_step) == 0x3c, "m_step");
static_assert(offsetof(MnuStock, m_main_step) == 0x44, "m_main_step");
static_assert(offsetof(MnuStock, m_54) == 0x54, "m_54");
static_assert(offsetof(MnuStock, m_msg) == 0x58, "m_msg");
static_assert(offsetof(MnuStock, m_spr) >= 0x7d0, "m_spr");
static_assert(offsetof(MnuStock, m_scene) >= 0x110c, "m_scene");
static_assert(offsetof(MnuStock, m_list) >= 0x1110, "m_list");
static_assert(offsetof(MnuStock, m_init) >= 0x1540, "m_init");
static_assert(offsetof(MnuStock, m_fly_spr) >= 0x1554, "m_fly_spr");
static_assert(offsetof(MnuStock, m_win) >= 0x1580, "m_win");

#endif /* _MNU_MNUSTOCK_H */
