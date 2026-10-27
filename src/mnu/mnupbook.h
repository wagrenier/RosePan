/* ==========================================================================
 *  mnu/mnupbook.h
 *
 *  MnuPBook and MnuDiary (mnu/mnupbook.cpp, 0x196a9c..0x19a52c): the two
 *  books MagLifeMenuPBook (mag/maglifemenu.h) switches between -- the
 *  chapter picture books (pb*, the "ehon") and Jennifer's diary (dy*).
 *  Both read data/mnurpk/mnupbook/: mnupbook.rpk first (the ehonbg /
 *  ehontex01_tcc base), then the book's own <name>.rpk. setType picks the
 *  row of the book's page table whose type matches; init reads the book's
 *  script flags through GamMan::lookupFlagTable (key column 1 = the type)
 *  and the sequence words. Class names from the ELF's vtables (0x2e4710 /
 *  0x2e4778), methods ror-inferred.
 * ======================================================================== */
#ifndef _MNU_MNUPBOOK_H
#define _MNU_MNUPBOOK_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "mnu/mnubase.h"
#include "msg/msgman.h"

/* A row of MnuPBook's page table (data 0x29c0f8, 0x30-byte rows, type -1
 * ends). EE layout. */
struct MnuPBookPage
{
    int  m_type;                // ROR 0x00: the book's type (GamMan flag table key)
    int  m_04;                  // ROR 0x04
    int  m_numPages;            // ROR 0x08
    int  m_0c;                  // ROR 0x0c: 0, 1 or 2
    char m_name[0x20];          // ROR 0x10: the pack, data/mnurpk/mnupbook/<name>.rpk
};
static_assert(sizeof(MnuPBookPage) == 0x30, "EE 0x30");
static_assert(offsetof(MnuPBookPage, m_name) == 0x10, "m_name");

/* A row of MnuDiary's page table (data 0x29c368, 0x30-byte rows, type -1
 * ends): the name follows the page count. EE layout. */
struct MnuDiaryPage
{
    int  m_type;                // ROR 0x00
    int  m_04;                  // ROR 0x04
    int  m_numPages;            // ROR 0x08: MnuDiary::init keeps half of it + 1
    char m_name[0x24];          // ROR 0x0c: dy*
};
static_assert(sizeof(MnuDiaryPage) == 0x30, "EE 0x30");
static_assert(offsetof(MnuDiaryPage, m_name) == 0x0c, "m_name");

/* MnuPBookSheet::m_flags. A sheet carries page 2n on its front and 2n + 1
 * on its back. */
enum
{
    MNU_SHEET_FRONT_TEX  = 0x01,    /* the front texture is still to be put on the model */
    MNU_SHEET_BACK_TEX   = 0x02,    /* ... the back's */
    MNU_SHEET_NO_FRONT   = 0x04,    /* the front page is not written yet (no texture) */
    MNU_SHEET_NO_BACK    = 0x08,
    MNU_SHEET_BRK_FRONT  = 0x10,    /* the front is pb_pagebrk (a torn page), no text */
    MNU_SHEET_BRK_BACK   = 0x20,
};

/* One sheet of the picture book (EE 0x20, queuePages' pool block). Its
 * model is model slot n, its turn motions slots 2n (to the right) and
 * 2n + 1 (to the left). */
struct MnuPBookSheet
{
    u_int m_flags;              // ROR 0x00: MNU_SHEET_*
    int   m_frontTex;           // ROR 0x04: texture slot of the front page, -1 none yet
    int   m_backTex;            // ROR 0x08: of the back page
    int   m_motion;             // ROR 0x0c: the motion slot posing it
    int   m_frame;              // ROR 0x10: its frame, up to the motion's m_frames
    int   _14[3];
};
static_assert(sizeof(MnuPBookSheet) == 0x20, "EE 0x20");
static_assert(offsetof(MnuPBookSheet, m_frame) == 0x10, "m_frame");

/* MnuPBook's own mnuBase::m_flags bits. */
enum
{
    MNU_PBOOK_BUSY       = 0x00002, /* loading, or a sheet turning / the book closing */
    MNU_PBOOK_STEP_A     = 0x10000, /* a step's own work is done */
    MNU_PBOOK_STEP_B     = 0x20000, /* ... and the fade is (5 and up from getFadeState) */
    MNU_PBOOK_END        = 0x40000, /* the last written page is open: Circle closes */
    MNU_PBOOK_TURN_LEFT  = 0x80000,
    MNU_PBOOK_TURN_RIGHT = 0x100000,
};

/* The picture book (vtable 0x2e4710). EE 0x8c0 (MagLifeMenuPBook
 * +0x50..+0x910). layout: host -- polymorphic, EE offsets in comments.
 * processPad's steps: 0 waits the base files, opens the book's own pack
 * and queues its pages (queuePages); 1 poses the sheets (startTurns) and
 * fades in; 2 queues the hidden sides and opens the windows; 3 reads the
 * pad (updatePad) until it closes; 4 waits the fade out; 5 writes the
 * script words, pops the camera, terms and returns 1. */
struct MnuPBook : mnuBase
{
    MnuPBook();                                                          // ROR 0x00196a9c
    ~MnuPBook();                                                         // ROR 0x00196b2c

    void init(DogTexInfo *tex) override;                                 // ROR 0x00196bdc
    void term() override;                                                // ROR 0x00196d98
    /* 1 when the book is closed and termed (the menu ends). */
    int  processPad(BedPad *pad) override;                               // ROR 0x001976b8
    void draw(I3dLayer *layer) override;                                 // ROR 0x00197f70
    /* EE slot 7: a draw set of texture slots 0 and 1 only. */
    void setDrawSet(I3dLayer *layer) override;                           // ROR 0x00198290
    /* 1 when type has a row in the page table (kept, with type), else 0. */
    int  setType(int type);                                              // ROR 0x00196e54
    /* Flag 2 and the base files ehonbg / ehontex01_tcc. */
    void addBgFiles();                                                   // ROR 0x00196ea4

    void queuePages();                                                   // ROR 0x00196efc
    void startTurns();                                                   // ROR 0x0019722c
    void openWindows();                                                  // ROR 0x001973fc
    /* 0 when Circle closed the book, else 1. */
    int  updatePad(BedPad *pad);                                         // ROR 0x00197a74
    void updateSheets();                                                 // ROR 0x00197d28
    /* Texture slot tex (-1: the dummy page 0x20 / 0x21) on sheet n's side. */
    void setSheetTex(int n, int tex, int back);                          // ROR 0x00197e68
    void turnSheet(int n, int right);                                    // ROR 0x00197f18
    int  canTurn(int right);                                             // ROR 0x00198318
    int  isTurning(int n);                                               // ROR 0x00198420
    void queueSheetTex(int n, int front, int back);                      // ROR 0x0019844c
    /* The text of pages page and page + 1 into the text window. */
    void setPageText(int page);                                          // ROR 0x001985b0

    const MnuPBookPage *m_page; // ROR 0x110: setType's row, NULL before
    int       m_bookType;       // ROR 0x114: its type, -1 none
    int       m_118;            // ROR 0x118: flag table column 0 of the type (global 0x65 on close)
    int       m_11c;            // ROR 0x11c: the sequence word of column 4: pages written (row mode 2), 0 = torn (mode 1)
    int       m_endPage;        // ROR 0x120: the last page to read; -1 before queuePages
    int       m_turnWait;       // ROR 0x124: frames left of a turn's input lock
    int       m_curPage;        // ROR 0x128: the left page open (odd), -1 the cover
    MnuPBookSheet *m_sheets;    // ROR 0x12c: queuePages' pool block, term frees
    int       m_numSheets;      // ROR 0x130
    int       m_promptWindow;   // ROR 0x134: scene windows, -1 none: the close prompt
    int       m_textWindow;     // ROR 0x138: the page text
    char     *m_text;           // ROR 0x13c: a 0x200-byte pool block, the text window's message
    MsgManSet m_msg;            // ROR 0x140..0x8b8
    int       m_blink;          // ROR 0x8b8: 0..299, the turn arrows' animation
    u_char    _8bc[0x8c0 - 0x8bc];
};
static_assert(offsetof(MnuPBook, m_page) >= 0x110, "m_page");
static_assert(offsetof(MnuPBook, m_promptWindow) >= 0x134, "m_promptWindow");
static_assert(offsetof(MnuPBook, m_msg) >= 0x140, "m_msg");
static_assert(offsetof(MnuPBook, m_blink) >= 0x8b8, "m_blink");
static_assert(sizeof(MnuPBook) >= 0x8c0, "EE 0x8c0");

/* MnuDiarySheet::m_flags. */
enum
{
    MNU_DSHEET_USED  = 0x01,    /* the record holds a sheet (m_sheet) */
    MNU_DSHEET_SHOWN = 0x02,    /* draw draws it */
};

/* One of the diary's five sheet records (EE 0x20, queueFiles' pool block):
 * record r poses model slot r (sheet model 0 copied to 1..4). Sheet n
 * carries page 2n + 1 on its front and 2n + 2 on its back; motion 0 is
 * turn_r, 1 turn_l. */
struct MnuDiarySheet
{
    u_int m_flags;              // ROR 0x00: MNU_DSHEET_*
    int   _04[2];               // ROR 0x04: queueFiles -1, unused
    int   m_motion;             // ROR 0x0c: 0 or 1
    int   m_frame;              // ROR 0x10: up to the motion's m_frames
    int   m_sheet;              // ROR 0x14: the sheet it is
    int   _18[2];
};
static_assert(sizeof(MnuDiarySheet) == 0x20, "EE 0x20");
static_assert(offsetof(MnuDiarySheet, m_sheet) == 0x14, "m_sheet");

/* The diary (vtable 0x2e4778). EE 0x8f0 (MagLifeMenuPBook +0x910..
 * +0x1200). layout: host -- polymorphic, EE offsets in comments. Its
 * processPad has MnuPBook's six steps (and MnuPBook's m_flags bits), but
 * the pages lie on a fixed base model (slot 5: the open spread) and only
 * the sheets next to it are posed, from five records and five copies of
 * one sheet model; page textures load and unload as the spread moves.
 * The open spread is pages m_curPage (left, even) and m_curPage + 1. */
struct MnuDiary : mnuBase
{
    MnuDiary();                                                          // ROR 0x00198720
    ~MnuDiary();                                                         // ROR 0x001987b0

    void init(DogTexInfo *tex) override;                                 // ROR 0x00198860
    void term() override;                                                // ROR 0x00198a38
    int  processPad(BedPad *pad) override;                               // ROR 0x001990b0
    void draw(I3dLayer *layer) override;                                 // ROR 0x00199c94
    /* EE slot 7: a draw set of texture slots 0 and 1 only. */
    void setDrawSet(I3dLayer *layer) override;                           // ROR 0x00199fcc
    /* 1 when type has a row in the page table (kept, with type), else 0. */
    int  setType(int type);                                              // ROR 0x00198b04

    void queueFiles();                                                   // ROR 0x00198b54
    void startTurns();                                                   // ROR 0x00198ca8
    void openWindows();                                                  // ROR 0x00198eb4
    /* 0 when Circle closed the diary, else 1. */
    int  updatePad(BedPad *pad);                                         // ROR 0x0019946c
    void updateSheets();                                                 // ROR 0x00199714
    /* Page page's texture on side back of the model posing sheet n. */
    void setSheetTex(int n, int page, int back);                         // ROR 0x00199a08
    /* ... on the base model's. */
    void setBaseTex(int page, int back);                                 // ROR 0x00199ab4
    void turnSheet(int n, int right);                                    // ROR 0x00199b3c
    int  canTurn(int right);                                             // ROR 0x0019a054
    int  isTurning(int n);                                               // ROR 0x0019a174
    /* A record not in use, -1 none. */
    int  findFreeSheet();                                                // ROR 0x0019a1d0
    /* The record holding sheet n, -1 none. */
    int  findSheet(int n);                                               // ROR 0x0019a20c
    /* The texture slot holding page, -1 none. */
    int  findPageTex(int page);                                          // ROR 0x0019a24c
    /* Sheet n's two pages queued where no slot holds them. */
    void queueSheetTex(int n);                                           // ROR 0x0019a290
    void setPageText(int page);                                          // ROR 0x0019a3c0

    const MnuDiaryPage *m_page; // ROR 0x110: setType's row, NULL before
    int       m_bookType;       // ROR 0x114: its type, -1 none
    int       m_118;            // ROR 0x118: flag table column 0 of the type (global 0x65 on close)
    int       m_11c;            // ROR 0x11c: the sequence word of column 4: pages written
    int       m_turnWait;       // ROR 0x120: frames left of a turn's input lock
    int       m_curPage;        // ROR 0x124: the left page open: the word of column 7, made even
    MnuDiarySheet *m_sheets;    // ROR 0x128: queueFiles' pool block (5 records), term frees
    int       m_numSheets;      // ROR 0x12c: the row's page count / 2 + 1
    int       m_promptWindow;   // ROR 0x130: scene windows, -1 none: the close prompt
    int       m_textWindow;     // ROR 0x134: the page text
    char     *m_text;           // ROR 0x138: a 0x4c8-byte pool block, the text window's message
    MsgManSet m_msg;            // ROR 0x13c..0x8b4
    int       m_blink;          // ROR 0x8b4: 0..299, the turn arrows' animation
    int      *m_texPage;        // ROR 0x8b8: 19 words, the page each texture slot holds, -1 none
    u_char    _8bc[0x8f0 - 0x8bc];
};
static_assert(offsetof(MnuDiary, m_page) >= 0x110, "m_page");
static_assert(offsetof(MnuDiary, m_promptWindow) >= 0x130, "m_promptWindow");
static_assert(offsetof(MnuDiary, m_msg) >= 0x13c, "m_msg");
static_assert(offsetof(MnuDiary, m_texPage) >= 0x8b8, "m_texPage");
static_assert(sizeof(MnuDiary) >= 0x8f0, "EE 0x8f0");

#endif /* _MNU_MNUPBOOK_H */
