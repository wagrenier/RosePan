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

/* The picture book (vtable 0x2e4710). EE 0x8c0 (MagLifeMenuPBook
 * +0x50..+0x910). layout: host -- polymorphic, EE offsets in comments. */
struct MnuPBook : mnuBase
{
    MnuPBook();                                                          // ROR 0x00196a9c
    ~MnuPBook();                                                         // ROR 0x00196b2c

    void init(DogTexInfo *tex) override;                                 // ROR 0x00196bdc
    void term() override;                                                // ROR 0x00196d98
    int  processPad(BedPad *pad) override;                               // ROR 0x001976b8
    void draw(I3dLayer *layer) override;                                 // ROR 0x00197f70
    /* 1 when type has a row in the page table (kept, with type), else 0. */
    int  setType(int type);                                              // ROR 0x00196e54
    /* Flag 2 and the base files ehonbg / ehontex01_tcc. */
    void addBgFiles();                                                   // ROR 0x00196ea4

    const MnuPBookPage *m_page; // ROR 0x110: setType's row, NULL before
    int       m_bookType;       // ROR 0x114: its type, -1 none
    int       m_118;            // ROR 0x118: flag table column 0 of the type
    int       m_11c;            // ROR 0x11c: the sequence word of column 4
    int       m_120;            // ROR 0x120: -1
    int       m_124;            // ROR 0x124: init 0
    int       m_lastPage;       // ROR 0x128: the word of column 7, less 1, made odd
    void     *m_12c;            // ROR 0x12c: a pool block term frees
    int       m_130;            // ROR 0x130
    int       m_window0;        // ROR 0x134: scene windows, -1 none
    int       m_window1;        // ROR 0x138
    void     *m_work;           // ROR 0x13c: a 0x200-byte pool block
    MsgManSet m_msg;            // ROR 0x140..0x8b8
    int       m_8b8;            // ROR 0x8b8: init 0
    u_char    _8bc[0x8c0 - 0x8bc];
};
static_assert(offsetof(MnuPBook, m_page) >= 0x110, "m_page");
static_assert(offsetof(MnuPBook, m_window0) >= 0x134, "m_window0");
static_assert(offsetof(MnuPBook, m_msg) >= 0x140, "m_msg");
static_assert(offsetof(MnuPBook, m_8b8) >= 0x8b8, "m_8b8");
static_assert(sizeof(MnuPBook) >= 0x8c0, "EE 0x8c0");

/* The diary (vtable 0x2e4778). EE 0x8f0 (MagLifeMenuPBook +0x910..
 * +0x1200). layout: host -- polymorphic, EE offsets in comments. */
struct MnuDiary : mnuBase
{
    MnuDiary();                                                          // ROR 0x00198720
    ~MnuDiary();                                                         // ROR 0x001987b0

    void init(DogTexInfo *tex) override;                                 // ROR 0x00198860
    void term() override;                                                // ROR 0x00198a38
    int  processPad(BedPad *pad) override;                               // ROR 0x001990b0
    void draw(I3dLayer *layer) override;                                 // ROR 0x00199c94
    /* 1 when type has a row in the page table (kept, with type), else 0. */
    int  setType(int type);                                              // ROR 0x00198b04

    const MnuDiaryPage *m_page; // ROR 0x110: setType's row, NULL before
    int       m_bookType;       // ROR 0x114: its type, -1 none
    int       m_118;            // ROR 0x118: flag table column 0 of the type
    int       m_11c;            // ROR 0x11c: the sequence word of column 4
    int       m_120;            // ROR 0x120: init 0
    int       m_lastPage;       // ROR 0x124: the word of column 7, made even
    void     *m_128;            // ROR 0x128: a pool block term frees
    int       m_numSheets;      // ROR 0x12c: the row's page count / 2 + 1
    int       m_window0;        // ROR 0x130: scene windows, -1 none
    int       m_window1;        // ROR 0x134
    void     *m_work;           // ROR 0x138: a 0x4c8-byte pool block
    MsgManSet m_msg;            // ROR 0x13c..0x8b4
    int       m_8b4;            // ROR 0x8b4: init 0
    int      *m_slots;          // ROR 0x8b8: 0x4c bytes (19 words) of -1
    u_char    _8bc[0x8f0 - 0x8bc];
};
static_assert(offsetof(MnuDiary, m_page) >= 0x110, "m_page");
static_assert(offsetof(MnuDiary, m_window0) >= 0x130, "m_window0");
static_assert(offsetof(MnuDiary, m_msg) >= 0x13c, "m_msg");
static_assert(offsetof(MnuDiary, m_slots) >= 0x8b8, "m_slots");
static_assert(sizeof(MnuDiary) >= 0x8f0, "EE 0x8f0");

#endif /* _MNU_MNUPBOOK_H */
