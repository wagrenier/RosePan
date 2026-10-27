/* ==========================================================================
 *  mnu/mnupbook.cpp
 *
 *  The picture book and the diary (0x196a9c..0x19a52c; RoR names no file
 *  here: mnu/ and the name from MnuPBook, whose pack directory both books
 *  read, between MnuMap and MnuStock). See mnu/mnupbook.h.
 *
 *  PORT: written so far -- both books' constructors, destructors, setType,
 *  init and term. processPad / draw and their page helpers are stubs, as
 *  is all of mnuBase's loader (mnu/mnubase.cpp) they stand on.
 * ======================================================================== */
#include "mnu/mnupbook.h"

#include <string.h>

#include "game/gamman.h"
#include "mag/magscene.h"
#include "sequence/sequence.h"

/* ---- data ---- */

/* MnuPBook's books (data 0x29c0f8). */
static const MnuPBookPage mnuPBookPages[] = {
    { 0x1038, 0xee, 20, 2, "pb010200" },  // 0x0029c0f8
    { 0x1059, 0x102, 20, 0, "pb022000" },  // 0x0029c128
    { 0x1063, 0x114, 14, 0, "pb030000" },  // 0x0029c158
    { 0x106b, 0x122, 16, 0, "pb0402a0" },  // 0x0029c188
    { 0x1074, 0x132, 18, 0, "pb0544a0" },  // 0x0029c1b8
    { 0x107e, 0x144, 18, 0, "pb061200" },  // 0x0029c1e8
    { 0x1088, 0x156, 20, 0, "pb0800e0" },  // 0x0029c218
    { 0x1093, 0x16a, 18, 0, "pb084300" },  // 0x0029c248
    { 0x109d, 0x17c, 18, 1, "pb104800" },  // 0x0029c278
    { 0x10a8, 0x18e, 12, 0, "pb111000" },  // 0x0029c2a8
    { 0x10af, 0x1a2, 18, 1, "pb1143b0" },  // 0x0029c2d8
    { 0x10ba, 0x1b4, 18, 1, "pb1244b0" },  // 0x0029c308
    { -1, -1, 0, 0, "" },                  // 0x0029c338
};

/* MnuDiary's diaries (data 0x29c368). */
static const MnuDiaryPage mnuDiaryPages[] = {
    { 0x1000, 0x4a, 72, "dy044200" },     // 0x0029c368
    { 0x1024, 0x90, 12, "dy060000" },     // 0x0029c398
    { 0x102a, 0xa8, 28, "dy0812a0" },     // 0x0029c3c8
    { -1, -1, 0, "" },                     // 0x0029c3f8
};

/* The pack both books open first (init builds it in place). */
static const char mnuPBookPack[] = "data/mnurpk/mnupbook/mnupbook.rpk";

/* The base files (rodata 0x2f8078 / 0x2f8080, MnuDiary's copies 0x2f81c8 /
 * 0x2f81d0). */
static const char mnuPBookBg[] = "ehonbg";
static const char mnuPBookBgTex[] = "ehontex01_tcc";

// ──────────────────────────────────────────────────────────────────────
// MnuPBook
// ROR 0x00196a9c; callcheck: mnuBase, MsgMan (the inline base constructor; the MsgManSet members construct zeroed on the host)

MnuPBook::MnuPBook()
{
    m_120 = -1;
    m_type = 7;
    m_window0 = -1;
    m_window1 = -1;
    m_bookType = -1;
    m_118 = -1;
    m_11c = -1;
    m_12c = NULL;
    m_work = NULL;
    m_page = NULL;
}

// ROR 0x00196b2c; callcheck: ~MsgMan, ~mnuBase, operator_delete (member / base destructors and the deleting variant: implicit on the host)

MnuPBook::~MnuPBook()
{
    MnuPBook::term();
}

// ──────────────────────────────────────────────────────────────────────
// Lays out the work (0x40 files, 0x20 models, 0x40 data, 0x22 textures),
// reads the book's flags, and opens the shared pack with the base files
// queued; processPad loads them.
// ROR 0x00196bdc // callcheck: openPack, strcpy (the ROM calls slot 10 through the vtable and inlines the copy)

void MnuPBook::init(DogTexInfo *tex)
{
    setupWork(tex, 0x40, 0x20, 0x40, 0x22);
    m_118 = g_gamMan.lookupFlagTable(1, m_bookType, 0);
    m_11c = g_sequence.m_global[g_gamMan.lookupFlagTable(1, m_bookType, 4)];
    u_int bit = g_gamMan.lookupFlagTable(1, m_bookType, 6);
    if (g_sequence.m_bits[bit >> 5] & (1u << (bit & 0x1f)))
        m_flags |= 0x40000;
    int n = g_sequence.m_global[g_gamMan.lookupFlagTable(1, m_bookType, 7)];
    m_lastPage = n - 1;
    if ((m_lastPage & 1) == 0)
        m_lastPage = n - 2;
    m_8b8 = 0;
    m_124 = 0;
    m_step = 0;
    m_work = mnuAlloc(0x200);
    strcpy(m_packName, mnuPBookPack);
    openPack();
    addBgFiles();
}

// ROR 0x00196d98

void MnuPBook::term()
{
    if (m_work) {
        mnuFree(m_work);
        m_work = NULL;
    }
    if (m_12c) {
        mnuFree(m_12c);
        m_12c = NULL;
    }
    m_130 = 0;
    restoreVolume();
    if (m_window0 != -1) {
        m_scene->closeWindow(m_window0);
        m_window0 = -1;
    }
    if (m_window1 != -1) {
        m_scene->closeWindow(m_window1);
        m_window1 = -1;
    }
    m_msg.close();
    m_page = NULL;
    m_120 = -1;
    m_bookType = -1;
    m_118 = -1;
    m_11c = -1;
    mnuBase::term();
}

// ROR 0x00196e54

int MnuPBook::setType(int type)
{
    for (m_page = mnuPBookPages; m_page->m_type != -1; m_page++) {
        if (m_page->m_type == type)
            break;
    }
    if (m_page->m_type == -1)
        return 0;
    m_bookType = type;
    return 1;
}

// ROR 0x00196ea4

void MnuPBook::addBgFiles()
{
    m_flags |= 2;
    addFile(mnuPBookBg, 4, 1);
    addFile(mnuPBookBgTex, 4, 0);
}

// ──────────────────────────────────────────────────────────────────────
// MnuDiary
// ROR 0x00198720; callcheck: mnuBase, MsgMan (the inline base constructor; the MsgManSet members construct zeroed on the host)

MnuDiary::MnuDiary()
{
    m_11c = -1;
    m_type = 7;
    m_window0 = -1;
    m_window1 = -1;
    m_bookType = -1;
    m_118 = -1;
    m_slots = NULL;
    m_128 = NULL;
    m_work = NULL;
    m_page = NULL;
}

// ROR 0x001987b0; callcheck: ~MsgMan, ~mnuBase, operator_delete (member / base destructors and the deleting variant: implicit on the host)

MnuDiary::~MnuDiary()
{
    MnuDiary::term();
}

// ──────────────────────────────────────────────────────────────────────
// The diary's work (0x10 files, 6 models, 2 data, 0x13 textures), its
// flags (always flag 0x40000), 19 page slots of -1, and the shared pack
// with the same base files as the picture book.
// ROR 0x00198860 // callcheck: openPack, strcpy (the ROM calls slot 10 through the vtable and inlines the copy)

void MnuDiary::init(DogTexInfo *tex)
{
    setupWork(tex, 0x10, 6, 2, 0x13);
    m_118 = g_gamMan.lookupFlagTable(1, m_bookType, 0);
    int v = g_sequence.m_global[g_gamMan.lookupFlagTable(1, m_bookType, 4)];
    m_flags |= 0x40000;
    m_11c = v;
    m_lastPage = g_sequence.m_global[g_gamMan.lookupFlagTable(1, m_bookType, 7)] & ~1;
    m_slots = (int *)mnuAlloc(0x4c);
    if (m_slots) /* PC: the ROM does not check the block */
        memset(m_slots, 0xff, 0x4c);
    int pages = m_page->m_numPages;
    m_8b4 = 0;
    m_flags |= 2;
    m_120 = 0;
    m_step = 0;
    m_numSheets = pages / 2 + 1;
    m_work = mnuAlloc(0x4c8);
    strcpy(m_packName, mnuPBookPack);
    openPack();
    addFile(mnuPBookBg, 4, 1);
    addFile(mnuPBookBgTex, 4, 0);
}

// ROR 0x00198a38

void MnuDiary::term()
{
    if (m_work) {
        mnuFree(m_work);
        m_work = NULL;
    }
    if (m_128) {
        mnuFree(m_128);
        m_128 = NULL;
    }
    m_numSheets = 0;
    if (m_slots) {
        mnuFree(m_slots);
        m_slots = NULL;
    }
    restoreVolume();
    if (m_window0 != -1) {
        m_scene->closeWindow(m_window0);
        m_window0 = -1;
    }
    if (m_window1 != -1) {
        m_scene->closeWindow(m_window1);
        m_window1 = -1;
    }
    m_msg.close();
    m_page = NULL;
    m_11c = -1;
    m_bookType = -1;
    m_118 = -1;
    mnuBase::term();
}

// ROR 0x00198b04

int MnuDiary::setType(int type)
{
    for (m_page = mnuDiaryPages; m_page->m_type != -1; m_page++) {
        if (m_page->m_type == type)
            break;
    }
    if (m_page->m_type == -1)
        return 0;
    m_bookType = type;
    return 1;
}
