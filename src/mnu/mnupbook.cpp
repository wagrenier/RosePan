/* ==========================================================================
 *  mnu/mnupbook.cpp
 *
 *  The picture book and the diary (0x196a9c..0x19a52c; RoR names no file
 *  here: mnu/ and the name from MnuPBook, whose pack directory both books
 *  read, between MnuMap and MnuStock). See mnu/mnupbook.h.
 *
 *  PORT: all of MnuPBook and MnuDiary.
 * ======================================================================== */
#include "mnu/mnupbook.h"

#include <stdio.h>
#include <string.h>

#include "bed/bedpad.h"
#include "game/gamman.h"
#include "i3d/i3dlayer.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3ritem.h"
#include "i3d/i3rroot.h"
#include "mag/magscene.h"
#include "sequence/sequence.h"
#include "snd/sndman.h"

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

/* The book's own files (rodata 0x2f8090..0x2f80f8, 0x2f8150 / 0x2f8168). */
static const char mnuPBookDummy0[] = "pb_pagedmy00_tcc";
static const char mnuPBookDummy1[] = "pb_pagedmy01_tcc";
static const char mnuPBookBaseFmt[] = "%s%d_base.i3s";
static const char mnuPBookTurnLFmt[] = "%s%d_turn_l.i3s";
static const char mnuPBookTurnRFmt[] = "%s%d_turn_r.i3s";
static const char mnuPBookPackFmt[] = "data/mnurpk/mnupbook/%s.rpk";
static const char mnuPBookBrkFmt[] = "pb_pagebrk%02d_tcc";
static const char mnuPBookPageFmt[] = "pb_page%02d_tcc";
/* The text window's format header (rodata 0x2f8178). */
static const char mnuPBookTextHead[] = "#r#L#T#f00#w001#F20";

/* The diary's own files (rodata 0x2f81e0..0x2f8228, 0x2f8298) and its
 * text header (built inline by setPageText). */
static const char mnuDiaryTex[] = "a_diary";
static const char mnuDiaryBaseFmt[] = "%s_base.i3s";
static const char mnuDiarySheetFmt[] = "%s01_base.i3s";
static const char mnuDiaryTurnRFmt[] = "%s01_turn_r.i3s";
static const char mnuDiaryTurnLFmt[] = "%s01_turn_l.i3s";
static const char mnuDiaryNoText[] = "##";
static const char mnuDiaryPageFmt[] = "dy_page%02d_tcc";
static const char mnuDiaryTextHead[] = "#r#L#T#[50#]50#f00#w001#F20";

/* The sheet a page sits on: page / 2, -1 for the cover (page -1). */
static inline int mnuPBookSheetOf(int page)
{
    return page < 0 ? -1 : page / 2;
}

// ──────────────────────────────────────────────────────────────────────
// MnuPBook
// ROR 0x00196a9c; callcheck: mnuBase, MsgMan (the inline base constructor; the MsgManSet members construct zeroed on the host)

MnuPBook::MnuPBook()
{
    m_endPage = -1;
    m_type = 7;
    m_promptWindow = -1;
    m_textWindow = -1;
    m_bookType = -1;
    m_118 = -1;
    m_11c = -1;
    m_sheets = NULL;
    m_text = NULL;
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
    m_curPage = n - 1;
    if ((m_curPage & 1) == 0)
        m_curPage = n - 2;
    m_blink = 0;
    m_turnWait = 0;
    m_step = 0;
    m_text = (char *)mnuAlloc(0x200);
    strcpy(m_packName, mnuPBookPack);
    openPack();
    addBgFiles();
}

// ROR 0x00196d98

void MnuPBook::term()
{
    if (m_text) {
        mnuFree(m_text);
        m_text = NULL;
    }
    if (m_sheets) {
        mnuFree(m_sheets);
        m_sheets = NULL;
    }
    m_numSheets = 0;
    restoreVolume();
    if (m_promptWindow != -1) {
        m_scene->closeWindow(m_promptWindow);
        m_promptWindow = -1;
    }
    if (m_textWindow != -1) {
        m_scene->closeWindow(m_textWindow);
        m_textWindow = -1;
    }
    m_msg.close();
    m_page = NULL;
    m_endPage = -1;
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
// Step 0 after the base files: the sheet records, their flags from the
// row's mode, then the dummy pages, the sheet models, each sheet's turn
// motion that leaves it where it lies (left of the open page: turn_l, the
// rest turn_r) and the page textures that show.
// ROR 0x00196efc

void MnuPBook::queuePages()
{
    char name[0x20];

    m_numSheets = (m_page->m_numPages + 1) / 2;
    m_sheets = (MnuPBookSheet *)mnuAlloc(m_numSheets * sizeof(MnuPBookSheet));
    int open = mnuPBookSheetOf(m_curPage);
    MnuPBookSheet *sh = m_sheets;
    int i = 0;
    for (; i <= open; i++, sh++) {
        sh->m_motion = i * 2 + 1;
        sh->m_flags = MNU_SHEET_FRONT_TEX | MNU_SHEET_BACK_TEX;
        sh->m_frontTex = -1;
        sh->m_backTex = -1;
        sh->m_frame = 0;
    }
    for (; i < m_numSheets; i++, sh++) {
        sh->m_frontTex = -1;
        sh->m_flags = MNU_SHEET_FRONT_TEX | MNU_SHEET_BACK_TEX;
        sh->m_motion = i * 2;
        sh->m_backTex = -1;
        sh->m_frame = 0;
    }

    m_endPage = m_page->m_numPages - 1;
    u_int mode = m_page->m_0c;
    if (mode & 2) {
        // pages written so far: the rest have no texture
        if (m_11c >= 1) {
            sh = m_sheets;
            for (i = 0; i < m_numSheets; i++, sh++) {
                int page = i * 2;
                if (m_11c <= page)
                    sh->m_flags |= MNU_SHEET_NO_FRONT;
                if (!(page + 1 < m_11c))
                    sh->m_flags |= MNU_SHEET_NO_BACK;
            }
            m_endPage = m_11c;
        }
    } else if (mode & 1) {
        // sheet 7 torn out until the word is set
        if (m_11c == 0)
            m_sheets[7].m_flags |= MNU_SHEET_BRK_FRONT | MNU_SHEET_BRK_BACK;
        else
            m_endPage = m_11c;
    }
    if (m_endPage <= m_curPage + 1)
        m_flags |= MNU_PBOOK_END;

    addFile(mnuPBookDummy0, MNU_FILE_TEX | MNU_FILE_WAIT, 0x20);
    addFile(mnuPBookDummy1, MNU_FILE_TEX | MNU_FILE_WAIT, 0x21);
    for (i = 0; i < m_numSheets; i++) {
        sprintf(name, mnuPBookBaseFmt, m_page->m_name, i);
        addFile(name, MNU_FILE_MODEL | MNU_FILE_WAIT, i);
    }
    for (i = 0; i <= open; i++) {
        sprintf(name, mnuPBookTurnLFmt, m_page->m_name, i);
        addFile(name, MNU_FILE_MOTION | MNU_FILE_WAIT, i * 2 + 1);
        queueSheetTex(i, 0, 1);
    }
    for (; i < m_numSheets; i++) {
        sprintf(name, mnuPBookTurnRFmt, m_page->m_name, i);
        addFile(name, MNU_FILE_MOTION | MNU_FILE_WAIT, i * 2);
        queueSheetTex(i, 1, 0);
    }
}

// ──────────────────────────────────────────────────────────────────────
// Step 1, everything of step 0 read: the menu camera, the game quietened,
// each loaded motion at half speed, every sheet posed at its motion's end,
// and the other turn motion queued.
// ROR 0x0019722c

void MnuPBook::startTurns()
{
    char name[0x20];
    int open = mnuPBookSheetOf(m_curPage);
    int i = 0;

    pushCamera(512.0f, 0.785398f);  // pi / 4
    lowerVolume();
    for (; i <= open; i++)
        m_motions[i * 2 + 1].m_frames <<= 1;
    for (; i < m_numSheets; i++)
        m_motions[i * 2].m_frames <<= 1;
    for (i = 0; i < m_numSheets; i++) {
        m_sheets[i].m_frame = m_motions[m_sheets[i].m_motion].m_frames;
        bindMotion(i, m_sheets[i].m_motion);
    }
    for (i = 0; i <= open; i++) {
        sprintf(name, mnuPBookTurnRFmt, m_page->m_name, i);
        addFile(name, MNU_FILE_MOTION, i * 2);
    }
    for (; i < m_numSheets; i++) {
        sprintf(name, mnuPBookTurnLFmt, m_page->m_name, i);
        addFile(name, MNU_FILE_MOTION, i * 2 + 1);
    }
}

// ──────────────────────────────────────────────────────────────────────
// Step 2, the second motions read: the hidden page sides queued, those
// motions at half speed, the page text window (512 x 256, hidden) and the
// close prompt (sys text 0x1a) with message sprite 4.
// ROR 0x001973fc

void MnuPBook::openWindows()
{
    int open = mnuPBookSheetOf(m_curPage);
    int i = 0;
    for (; i <= open; i++)
        queueSheetTex(i, 1, 0);
    for (; i < m_numSheets; i++)
        queueSheetTex(i, 0, 1);
    for (i = 0; i <= open; i++)
        m_motions[i * 2].m_frames <<= 1;
    for (; i < m_numSheets; i++)
        m_motions[i * 2 + 1].m_frames <<= 1;

    m_text[0] = '#';
    m_text[1] = '#';
    m_text[2] = '\0';
    m_msg.open();
    m_textWindow = m_scene->openWindow(0x200, 0x100, 0, -1);
    m_scene->setWindowPos(m_textWindow, 0, 0x20, 0x7fff);
    m_scene->setWindowWords84(m_textWindow, 0, 0, 0x200, 0x100);
    m_scene->setWindowFillMsb(m_textWindow, 1);
    m_scene->setWindowFill(m_textWindow, 0);
    m_scene->setWindowShow(m_textWindow, 0);
    m_scene->updateWindowFont(m_textWindow);
    setPageText(m_curPage);
    g_gamMan.setMesSpritePos(4, 0xdc, 0x198);
    g_gamMan.fadeInMesSprite(4, 0);
    g_gamMan.showMesSprite(4, 0, 0);

    m_msg.open();
    m_promptWindow = m_scene->openWindow(0x40, 0x20, 0, -1);
    m_scene->setWindowPos(m_promptWindow, 0xf6, 0x19a, 0x7fff);
    m_scene->setWindowWords84(m_promptWindow, 0, 0, 0x40, 0x20);
    m_scene->setWindowFillMsb(m_promptWindow, 1);
    m_scene->setWindowFill(m_promptWindow, 0);
    m_scene->setWindowShow(m_promptWindow, 1);
    m_scene->updateWindowFont(m_promptWindow);
    m_msg.setMessage(m_promptWindow, msgGetSysText(0x1a), 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001976b8

int MnuPBook::processPad(BedPad *pad)
{
    const u_int both = MNU_PBOOK_STEP_A | MNU_PBOOK_STEP_B;

    updateLoad();
    updateSheets();
    m_10 = m_step;
    switch (m_step) {
    case 0:
        if (!(m_flags & MNU_PBOOK_STEP_B) && m_scene->getFadeState() >= 5)
            m_flags |= MNU_PBOOK_STEP_B;
        if (!(m_flags & MNU_PBOOK_STEP_A) && m_numFiles == 0) {
            // the base files are in: on to the book's own pack
            m_flags |= MNU_PBOOK_STEP_A;
            closePack();
            sprintf(m_packName, mnuPBookPackFmt, m_page->m_name);
            openPack();
            queuePages();
            m_flags |= MNU_PBOOK_STEP_B;
        }
        if ((m_flags & both) == both)
            m_step = 1;
        return 0;
    case 1:
        if (m_numFiles == 0) {
            startTurns();
            m_scene->fadeIn(15, 7, 0);
            m_step = 2;
            m_flags &= ~both;
        }
        return 0;
    case 2:
        if (!(m_flags & MNU_PBOOK_STEP_B) && m_scene->getFadeState() > 4)
            m_flags |= MNU_PBOOK_STEP_B;
        if (!(m_flags & MNU_PBOOK_STEP_A) && m_numFiles == 0) {
            openWindows();
            m_flags |= MNU_PBOOK_STEP_A;
        }
        if ((m_flags & both) == both) {
            m_step = 3;
            m_flags &= ~MNU_PBOOK_BUSY;
        }
        return 0;
    case 3:
        if (updatePad(pad) == 0) {
            m_scene->fadeOut(15, 7, 0);
            g_sndMan.playSe(2);
            g_gamMan.showMesSprite(4, 0, 0);
            m_scene->setWindowShow(m_promptWindow, 1);
            m_step = 4;
        }
        return 0;
    case 4:
        if (m_scene->getFadeState() > 4)
            m_step = 5;
        return 0;
    case 5: {
        // the book read: its words back to the script
        g_sequence.m_global[0x44 / 4] = g_gamMan.lookupFlagTable(1, m_bookType, 2);
        g_sequence.m_global[0x194 / 4] = m_118;
        g_sequence.m_bits[1] &= ~0x4000000u;
        if (g_gamMan.isSeqPair0Pending())
            g_sequence.m_bits[1] |= 0x2000000u;
        u_int bit = g_gamMan.lookupFlagTable(1, m_bookType, 6);
        g_sequence.m_bits[bit >> 5] &= ~(1u << (bit & 0x1f));
        popCamera();
        term();
        return 1;
    }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Step 3's input. Right / left (the d-pad held or the left stick past
// 0.75) turn a sheet once it may (canTurn); Circle closes once the last
// page is open. 0 = close.
// ROR 0x00197a74

int MnuPBook::updatePad(BedPad *pad)
{
    if (m_flags & MNU_PBOOK_END) {
        g_gamMan.showMesSprite(4, 1, 0);
        m_scene->setWindowShow(m_promptWindow, 0);
    }
    if (m_turnWait > 0 && --m_turnWait == 0) {
        // the turn is over: the new pages' text
        m_flags &= ~(MNU_PBOOK_TURN_LEFT | MNU_PBOOK_TURN_RIGHT | MNU_PBOOK_BUSY);
        setPageText(m_curPage);
        if (m_endPage <= m_curPage + 1)
            m_flags |= MNU_PBOOK_END;
    }
    m_msg.update();
    if (!(m_flags & MNU_PBOOK_BUSY)) {
        if ((pad->m_trig & 0x2000) && (m_flags & MNU_PBOOK_END)) {   // Circle
            m_scene->setWindowShow(m_textWindow, 1);
            m_flags |= MNU_PBOOK_BUSY;
            return 0;
        }
        if (m_turnWait != 0)
            return 1;
        int turned = 0;
        if (((pad->m_hold & 0x20) || pad->getLeftX() >= 0.75f) && canTurn(1)) {    // right
            m_curPage += 2;
            turnSheet(mnuPBookSheetOf(m_curPage), 1);
            m_flags |= MNU_PBOOK_TURN_RIGHT | MNU_PBOOK_BUSY;
            g_sndMan.playSe(8);
            setPageText(-99);
            turned = 1;
        }
        // right held but not possible: left is still tried
        if (!turned && ((pad->m_hold & 0x80) || pad->getLeftX() <= -0.75f) && canTurn(0)) {   // left
            turnSheet(mnuPBookSheetOf(m_curPage), 0);
            m_curPage -= 2;
            m_flags |= MNU_PBOOK_TURN_LEFT | MNU_PBOOK_BUSY;
            g_sndMan.playSe(8);
            setPageText(-99);
        }
        if (!(m_flags & MNU_PBOOK_BUSY))
            return 1;
    }
    m_blink = (m_blink + 1) % 300;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Every frame: the sheets' turns advance, and page textures read since
// go onto their sheet models.
// ROR 0x00197d28

void MnuPBook::updateSheets()
{
    if (m_numSheets == 0)
        return;
    MnuPBookSheet *sh = m_sheets;
    for (int i = 0; i < m_numSheets; i++, sh++) {
        if (sh->m_frame < m_motions[sh->m_motion].m_frames)
            sh->m_frame++;
    }
    sh = m_sheets;
    for (int i = 0; i < m_numSheets; i++, sh++) {
        if ((sh->m_flags & MNU_SHEET_FRONT_TEX) && sh->m_frontTex != -1 && isTexLoaded(sh->m_frontTex)) {
            setSheetTex(i, sh->m_frontTex, 0);
            sh->m_flags &= ~MNU_SHEET_FRONT_TEX;
        }
        if ((sh->m_flags & MNU_SHEET_BACK_TEX) && sh->m_backTex != -1 && isTexLoaded(sh->m_backTex)) {
            setSheetTex(i, sh->m_backTex, 1);
            sh->m_flags &= ~MNU_SHEET_BACK_TEX;
        }
    }
}

// ROR 0x00197e68

void MnuPBook::setSheetTex(int n, int tex, int back)
{
    I3rItem *side = i3rGetChild52(&m_models[n].m_root->m_item, back != 0);
    I3rItem *mat = i3rGetChild25(side, 0);
    int rec;
    if (tex == -1)
        rec = m_texs[back ? 0x21 : 0x20].m_tex;
    else
        rec = m_texs[tex].m_tex;
    *(int *)((u_char *)mat->getData() + 0x1c) = rec;
    *(short *)((u_char *)side->getData() + 8) += 1;     // its change count
}

// ROR 0x00197f18

void MnuPBook::turnSheet(int n, int right)
{
    int motion = n * 2;
    if (right)
        motion++;
    MnuPBookSheet *sh = &m_sheets[n];
    sh->m_frame = 0;
    sh->m_motion = motion;
    bindMotion(n, motion);
    m_turnWait = 10;
}

// ──────────────────────────────────────────────────────────────────────
// The background page (texture slot 1) and the turn arrows (slot 0, a
// 28 x 16 cell of three frames that cycle while that side turns), then
// the sheet models: the book laid at -60 degrees about x, turned 180
// about y, at (25, 8, 220).
// ROR 0x00197f70

void MnuPBook::draw(I3dLayer *layer)
{
    if ((u_int)(m_10 - 2) >= 3)
        return;

    beginDraw(layer);
    useTex(1);
    MnuSprite spr;
    spr.m_x = 0.0f;
    spr.m_y = 0.0f;
    spr.m_w = 512.0f;
    spr.m_h = 448.0f;
    spr.m_uv[0] = 0.0f;
    spr.m_uv[1] = 0.0f;
    spr.m_uv[2] = 1.0f;
    spr.m_uv[3] = 0.875f;
    spr.m_color[0] = 1.0f;
    spr.m_color[1] = 1.0f;
    spr.m_color[2] = 1.0f;
    spr.m_color[3] = 1.0f;
    spr.m_angle = 0.0f;
    drawSprite(0.0f, 0.0f, &spr);

    useTex(0);
    if (m_curPage - 1 >= 0) {
        u_int cell = (m_flags & MNU_PBOOK_TURN_LEFT) ? ((u_int)m_blink >> 1) % 3 : 0;
        float v = (float)cell * 16.0f + 48.0f;
        spr.m_w = 28.0f;
        spr.m_h = 16.0f;
        spr.m_uv[0] = 0.0f;
        spr.m_uv[1] = v * (1.0f / 128.0f);
        spr.m_uv[2] = 0.21875f;
        spr.m_uv[3] = (v + 16.0f) * (1.0f / 128.0f);
        drawSprite(100.0f, 412.0f, &spr);
    }
    if (m_curPage + 2 < m_page->m_numPages) {
        u_int cell = (m_flags & MNU_PBOOK_TURN_RIGHT) ? ((u_int)m_blink >> 1) % 3 : 0;
        float v = (float)cell * 16.0f + 0.0f;
        spr.m_w = 28.0f;
        spr.m_h = 16.0f;
        spr.m_uv[0] = 0.0f;
        spr.m_uv[1] = v * (1.0f / 128.0f);
        spr.m_uv[2] = 0.21875f;
        spr.m_uv[3] = (v + 16.0f) * (1.0f / 128.0f);
        drawSprite(384.0f, 412.0f, &spr);
    }
    m_pkt = i3dPktSetReg(m_pkt, 0x47 /* TEST_1 */, 0x503fd);
    endDraw(layer);

    sceVu0FMATRIX world;
    float rot[4] = { -1.0471976f, 3.1415927f, 0.0f, 0.0f };     // rodata 0x2f8138
    i3dMatUnit(world);
    i3dMatRotZXY(world, world, rot);
    world[3][0] = 25.0f;
    world[3][1] = 8.0f;
    world[3][2] = 220.0f;
    for (int i = 0; i < m_numSheets; i++)
        drawModel(layer, world, i, m_sheets[i].m_motion, m_sheets[i].m_frame);
}

// ROR 0x00198290

void MnuPBook::setDrawSet(I3dLayer *layer)
{
    int texNo[2];
    texNo[0] = m_texs[0].m_tex;
    m_texs[0].m_set = 0;
    texNo[1] = m_texs[1].m_tex;
    m_texs[1].m_set = 1;
    I3dTexSetDesc desc;
    desc.m_depth = 0.0f;
    desc.m_tex_no = texNo;
    desc.m_tex_count = 2;
    desc.m_sort_func = 0;
    desc.m_sort_order = 0;
    desc.m_sort_param = 0;
    m_drawSet = i3dLayerFindDrawSet(layer, &desc);
    i3dLayerSetDrawSet(layer, m_drawSet);
}

// ROR 0x00198318

int MnuPBook::canTurn(int right)
{
    int n = mnuPBookSheetOf(m_curPage);
    if (!right) {
        if (n < 0)
            return 0;
        if (isTurning(n))
            return 0;
        n--;
        if (n < 0)
            return 1;
    } else {
        if (n + 1 >= m_numSheets)
            return 0;
        if (isTurning(n + 1))
            return 0;
        n += 2;
        if (n >= m_numSheets)
            return 1;
    }
    // the sheet coming into view has its pages (or none to wait for)
    u_int f = m_sheets[n].m_flags;
    return (f & (MNU_SHEET_FRONT_TEX | MNU_SHEET_NO_FRONT)) != MNU_SHEET_FRONT_TEX
        && (f & (MNU_SHEET_BACK_TEX | MNU_SHEET_NO_BACK)) != MNU_SHEET_BACK_TEX;
}

// ROR 0x00198420

int MnuPBook::isTurning(int n)
{
    const MnuPBookSheet *sh = &m_sheets[n];
    return sh->m_frame < m_motions[sh->m_motion].m_frames;
}

// ROR 0x0019844c

void MnuPBook::queueSheetTex(int n, int front, int back)
{
    char name[0x20];
    int page = n < 0 ? -1 : n * 2;
    int lo = 2;
    MnuPBookSheet *sh = &m_sheets[n];
    if (front && sh->m_frontTex == -1 && !(sh->m_flags & MNU_SHEET_NO_FRONT)) {
        sh->m_frontTex = findFreeTex(2, 0x21);
        sprintf(name, (sh->m_flags & MNU_SHEET_BRK_FRONT) ? mnuPBookBrkFmt : mnuPBookPageFmt, page);
        addFile(name, MNU_FILE_TEX, sh->m_frontTex);
        lo = sh->m_frontTex + 1;
    }
    if (back && sh->m_backTex == -1 && !(sh->m_flags & MNU_SHEET_NO_BACK)) {
        sh->m_backTex = findFreeTex(lo, 0x21);
        sprintf(name, (sh->m_flags & MNU_SHEET_BRK_BACK) ? mnuPBookBrkFmt : mnuPBookPageFmt, page + 1);
        addFile(name, MNU_FILE_TEX, sh->m_backTex);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The text of the open pages (page on the left, page + 1 on the right:
// sys texts from the row's +4 on) unless a side is absent or torn; a
// page below -1 leaves the header alone (setPageText(-99) while a sheet
// turns).
// ROR 0x001985b0

void MnuPBook::setPageText(int page)
{
    int n = mnuPBookSheetOf(page);
    memset(m_text, 0, 0x200);
    memcpy(m_text, mnuPBookTextHead, sizeof(mnuPBookTextHead));
    if (page >= 0 && !(m_sheets[n].m_flags & (MNU_SHEET_NO_BACK | MNU_SHEET_BRK_BACK))) {
        const char *t = msgGetSysText(m_page->m_04 + page);
        size_t at = strlen(m_text);
        memcpy(m_text + at, t, strlen(t));
    }
    if (page + 1 >= 0 && !(m_sheets[n + 1].m_flags & (MNU_SHEET_NO_FRONT | MNU_SHEET_BRK_FRONT))) {
        const char *t = msgGetSysText(m_page->m_04 + page + 1);
        size_t at = strlen(m_text);
        memcpy(m_text + at, t, strlen(t));
    }
    m_msg.setMessage(m_textWindow, m_text, 0);
}

// ──────────────────────────────────────────────────────────────────────
// MnuDiary
// ROR 0x00198720; callcheck: mnuBase, MsgMan (the inline base constructor; the MsgManSet members construct zeroed on the host)

MnuDiary::MnuDiary()
{
    m_11c = -1;
    m_type = 7;
    m_promptWindow = -1;
    m_textWindow = -1;
    m_bookType = -1;
    m_118 = -1;
    m_texPage = NULL;
    m_sheets = NULL;
    m_text = NULL;
    m_page = NULL;
}

// ROR 0x001987b0; callcheck: ~MsgMan, ~mnuBase, operator_delete (member / base destructors and the deleting variant: implicit on the host)

MnuDiary::~MnuDiary()
{
    MnuDiary::term();
}

// ──────────────────────────────────────────────────────────────────────
// The diary's work (0x10 files, 6 models, 2 data, 0x13 textures), its
// flags (always MNU_PBOOK_END), 19 page slots of -1, and the shared pack
// with the same base files as the picture book.
// ROR 0x00198860 // callcheck: openPack, strcpy (the ROM calls slot 10 through the vtable and inlines the copy)

void MnuDiary::init(DogTexInfo *tex)
{
    setupWork(tex, 0x10, 6, 2, 0x13);
    m_118 = g_gamMan.lookupFlagTable(1, m_bookType, 0);
    int v = g_sequence.m_global[g_gamMan.lookupFlagTable(1, m_bookType, 4)];
    m_flags |= MNU_PBOOK_END;
    m_11c = v;
    m_curPage = g_sequence.m_global[g_gamMan.lookupFlagTable(1, m_bookType, 7)] & ~1;
    m_texPage = (int *)mnuAlloc(0x4c);
    if (m_texPage) /* PC: the ROM does not check the block */
        memset(m_texPage, 0xff, 0x4c);
    int pages = m_page->m_numPages;
    m_blink = 0;
    m_flags |= MNU_PBOOK_BUSY;
    m_turnWait = 0;
    m_step = 0;
    m_numSheets = pages / 2 + 1;
    m_text = (char *)mnuAlloc(0x4c8);
    strcpy(m_packName, mnuPBookPack);
    openPack();
    addFile(mnuPBookBg, MNU_FILE_TEX, 1);
    addFile(mnuPBookBgTex, MNU_FILE_TEX, 0);
}

// ROR 0x00198a38

void MnuDiary::term()
{
    if (m_text) {
        mnuFree(m_text);
        m_text = NULL;
    }
    if (m_sheets) {
        mnuFree(m_sheets);
        m_sheets = NULL;
    }
    m_numSheets = 0;
    if (m_texPage) {
        mnuFree(m_texPage);
        m_texPage = NULL;
    }
    restoreVolume();
    if (m_promptWindow != -1) {
        m_scene->closeWindow(m_promptWindow);
        m_promptWindow = -1;
    }
    if (m_textWindow != -1) {
        m_scene->closeWindow(m_textWindow);
        m_textWindow = -1;
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

// ──────────────────────────────────────────────────────────────────────
// Step 0 after the base files: the five sheet records, then a_diary, the
// base model (slot 5), the open spread's neighbour pages, the sheet model
// (slot 0) and its two turn motions.
// ROR 0x00198b54

void MnuDiary::queueFiles()
{
    char name[0x20];

    m_sheets = (MnuDiarySheet *)mnuAlloc(5 * sizeof(MnuDiarySheet));
    MnuDiarySheet *sh = m_sheets;
    for (int i = 4; i >= 0; i--, sh++) {
        sh->m_motion = -1;
        sh->m_flags = 0;
        sh->_04[1] = -1;
        sh->_04[0] = -1;
        sh->m_frame = 0;
    }
    addFile(mnuDiaryTex, MNU_FILE_TEX, 2);
    sprintf(name, mnuDiaryBaseFmt, m_page->m_name);
    addFile(name, MNU_FILE_MODEL, 5);
    int n = (m_curPage + 1) / 2;
    queueSheetTex(n - 1);
    queueSheetTex(n);
    sprintf(name, mnuDiarySheetFmt, m_page->m_name);
    addFile(name, MNU_FILE_MODEL, 0);
    sprintf(name, mnuDiaryTurnRFmt, m_page->m_name);
    addFile(name, MNU_FILE_MOTION, 0);
    sprintf(name, mnuDiaryTurnLFmt, m_page->m_name);
    addFile(name, MNU_FILE_MOTION, 1);
}

// ──────────────────────────────────────────────────────────────────────
// Step 1, everything of step 0 read: the menu camera, the game quietened,
// the sheet model copied into slots 1..4, the motions at half speed, the
// sheets either side of the spread laid at their motion's end, the
// spread's pages on the base model, and the pages two sheets out queued.
// ROR 0x00198ca8

void MnuDiary::startTurns()
{
    pushCamera(512.0f, 0.785398f);  // pi / 4
    lowerVolume();
    for (int i = 1; i < 5; i++)
        copyModel(i, 0);
    for (int i = 0; i < m_numMotions; i++)
        m_motions[i].m_frames <<= 1;

    int n = (m_curPage + 1) / 2;
    // the sheet on the left (its back is page m_curPage), turned left
    int r = findFreeSheet();
    MnuDiarySheet *sh = &m_sheets[r];
    sh->m_motion = 1;
    sh->m_sheet = n - 1;
    sh->m_flags |= MNU_DSHEET_USED | MNU_DSHEET_SHOWN;
    sh->m_frame = m_motions[1].m_frames;
    bindMotion(r, 1);
    setSheetTex(n - 1, (n - 1) * 2 + 1, 0);
    setSheetTex(n - 1, (n - 1) * 2 + 2, 1);
    // the sheet on the right, turned right
    r = findFreeSheet();
    sh = &m_sheets[r];
    sh->m_sheet = n;
    sh->m_motion = 0;
    sh->m_flags |= MNU_DSHEET_USED | MNU_DSHEET_SHOWN;
    sh->m_frame = m_motions[0].m_frames;
    bindMotion(r, 0);
    setSheetTex(n, n * 2 + 1, 0);
    setSheetTex(n, n * 2 + 2, 1);

    setBaseTex(m_curPage, 0);
    setBaseTex(m_curPage + 1, 1);
    n = (m_curPage + 1) / 2;
    queueSheetTex(n + 1);
    queueSheetTex(n - 2);
}

// ──────────────────────────────────────────────────────────────────────
// Step 2: the page text window (512 x 256, hidden) and the close prompt
// (sys text 0x1a) with message sprite 4, as the picture book's.
// ROR 0x00198eb4

void MnuDiary::openWindows()
{
    m_text[0] = '#';
    m_text[1] = '#';
    m_text[2] = '\0';
    m_msg.open();
    m_textWindow = m_scene->openWindow(0x200, 0x100, 0, -1);
    m_scene->setWindowPos(m_textWindow, 0, 0x20, 0x7fff);
    m_scene->setWindowWords84(m_textWindow, 0, 0, 0x200, 0x100);
    m_scene->setWindowSize(m_textWindow, 0x200, 0x100);
    m_scene->setWindowFillMsb(m_textWindow, 1);
    m_scene->setWindowFill(m_textWindow, 0);
    m_scene->setWindowShow(m_textWindow, 0);
    m_scene->updateWindowFont(m_textWindow);
    setPageText(m_curPage);
    g_gamMan.setMesSpritePos(4, 0xdc, 0x198);
    g_gamMan.fadeInMesSprite(4, 0);
    g_gamMan.showMesSprite(4, 0, 0);

    m_msg.open();
    m_promptWindow = m_scene->openWindow(0x40, 0x20, 0, -1);
    m_scene->setWindowPos(m_promptWindow, 0xf6, 0x19a, 0x7fff);
    m_scene->setWindowWords84(m_promptWindow, 0, 0, 0x40, 0x20);
    m_scene->setWindowFillMsb(m_promptWindow, 1);
    m_scene->setWindowFill(m_promptWindow, 0);
    m_scene->setWindowShow(m_promptWindow, 1);
    m_scene->updateWindowFont(m_promptWindow);
    m_msg.setMessage(m_promptWindow, msgGetSysText(0x1a), 0);
}

// ──────────────────────────────────────────────────────────────────────
// MnuPBook::processPad's six steps with the diary's helpers.
// ROR 0x001990b0 // callcheck: openPack, closePack, term (the ROM calls slots 10, 11 and 3 through the vtable)

int MnuDiary::processPad(BedPad *pad)
{
    const u_int both = MNU_PBOOK_STEP_A | MNU_PBOOK_STEP_B;

    updateLoad();
    updateSheets();
    m_10 = m_step;
    switch (m_step) {
    case 0:
        if (!(m_flags & MNU_PBOOK_STEP_B) && m_scene->getFadeState() >= 5)
            m_flags |= MNU_PBOOK_STEP_B;
        if (!(m_flags & MNU_PBOOK_STEP_A) && m_numFiles == 0) {
            // the base files are in: on to the diary's own pack
            m_flags |= MNU_PBOOK_STEP_A;
            closePack();
            sprintf(m_packName, mnuPBookPackFmt, m_page->m_name);
            openPack();
            queueFiles();
            m_flags |= MNU_PBOOK_STEP_B;
        }
        if ((m_flags & both) == both)
            m_step = 1;
        return 0;
    case 1:
        if (m_numFiles == 0) {
            startTurns();
            m_flags &= ~both;
            m_scene->fadeIn(15, 7, 0);
            m_step = 2;
        }
        return 0;
    case 2:
        if (!(m_flags & MNU_PBOOK_STEP_B) && m_scene->getFadeState() >= 5)
            m_flags |= MNU_PBOOK_STEP_B;
        if (!(m_flags & MNU_PBOOK_STEP_A) && m_numFiles == 0) {
            m_flags |= MNU_PBOOK_STEP_A;
            openWindows();
        }
        if ((m_flags & both) == both) {
            m_step = 3;
            m_flags &= ~MNU_PBOOK_BUSY;
        }
        return 0;
    case 3:
        if (updatePad(pad) == 0) {
            m_scene->fadeOut(15, 7, 0);
            g_sndMan.playSe(2);
            g_gamMan.showMesSprite(4, 0, 0);
            m_scene->setWindowShow(m_promptWindow, 1);
            m_step = 4;
        }
        return 0;
    case 4:
        if (m_scene->getFadeState() > 4)
            m_step = 5;
        return 0;
    case 5: {
        // the diary read: its words back to the script
        g_sequence.m_global[0x44 / 4] = g_gamMan.lookupFlagTable(1, m_bookType, 2);
        g_sequence.m_global[0x194 / 4] = m_118;
        g_sequence.m_bits[1] &= ~0x4000000u;
        if (g_gamMan.isSeqPair0Pending())
            g_sequence.m_bits[1] |= 0x2000000u;
        u_int bit = g_gamMan.lookupFlagTable(1, m_bookType, 6);
        g_sequence.m_bits[bit >> 5] &= ~(1u << (bit & 0x1f));
        popCamera();
        term();
        return 1;
    }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Step 3's input, as the picture book's: right / left turn a sheet once
// it may (canTurn), Circle closes once the last written page is open.
// 0 = close.
// ROR 0x0019946c

int MnuDiary::updatePad(BedPad *pad)
{
    if (m_flags & MNU_PBOOK_END) {
        g_gamMan.showMesSprite(4, 1, 0);
        m_scene->setWindowShow(m_promptWindow, 0);
    }
    if (m_turnWait > 0 && --m_turnWait == 0) {
        // the turn is over: the new pages' text
        m_flags &= ~(MNU_PBOOK_TURN_LEFT | MNU_PBOOK_TURN_RIGHT | MNU_PBOOK_BUSY);
        setPageText(m_curPage);
        if (m_11c <= m_curPage + 1)
            m_flags |= MNU_PBOOK_END;
    }
    m_msg.update();
    if (!(m_flags & MNU_PBOOK_BUSY)) {
        if ((pad->m_trig & 0x2000) && (m_flags & MNU_PBOOK_END)) {   // Circle
            m_scene->setWindowShow(m_textWindow, 1);
            m_flags |= MNU_PBOOK_BUSY;
            return 0;
        }
        if (m_turnWait == 0) {
            int turned = 0;
            if (((pad->m_hold & 0x20) || pad->getLeftX() >= 0.75f) && canTurn(1)) {    // right
                int page = m_curPage;
                m_curPage = page + 2;
                turnSheet((page + 3) / 2 - 1, 1);
                m_flags |= MNU_PBOOK_TURN_RIGHT | MNU_PBOOK_BUSY;
                g_sndMan.playSe(8);
                setPageText(-1);
                turned = 1;
            }
            // right held but not possible: left is still tried
            if (!turned && ((pad->m_hold & 0x80) || pad->getLeftX() <= -0.75f) && canTurn(0)) {   // left
                turnSheet((m_curPage + 1) / 2 - 1, 0);
                m_curPage -= 2;
                m_flags |= MNU_PBOOK_TURN_LEFT | MNU_PBOOK_BUSY;
                g_sndMan.playSe(8);
                setPageText(-1);
            }
        }
        if (!(m_flags & MNU_PBOOK_BUSY))
            return 1;
    }
    m_blink = (m_blink + 1) % 300;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Every frame: each turning sheet advances; at its fourth frame the sheet
// it uncovers shows, four frames before its end the sheet it covers is
// let go, and at its end the page it laid down goes onto the base model.
// Then the page textures past the first unposed sheet each side are freed.
// ROR 0x00199714

void MnuDiary::updateSheets()
{
    MnuDiarySheet *sh = m_sheets;
    if (!sh)
        return;
    for (int i = 4; i >= 0; i--, sh++) {
        if (!(sh->m_flags & MNU_DSHEET_USED))
            continue;
        int motion = sh->m_motion;
        int frames = m_motions[motion].m_frames;
        if (sh->m_frame >= frames)
            continue;
        sh->m_frame++;
        if (frames == sh->m_frame) {
            if ((motion & 1) == 0)
                setBaseTex(sh->m_sheet * 2 + 1, 1);
            else
                setBaseTex(sh->m_sheet * 2 + 2, 0);
        } else if (frames - sh->m_frame == 4) {
            int r = findSheet((motion & 1) ? sh->m_sheet - 1 : sh->m_sheet + 1);
            if (r != -1)
                m_sheets[r].m_flags &= ~(MNU_DSHEET_USED | MNU_DSHEET_SHOWN);
        } else if (sh->m_frame == 4) {
            int r = findSheet((motion & 1) ? sh->m_sheet + 1 : sh->m_sheet - 1);
            if (r != -1)
                m_sheets[r].m_flags |= MNU_DSHEET_SHOWN;
        }
    }

    for (int n = (m_curPage + 1) / 2 - 2; n >= -1; n--) {
        if (findSheet(n) == -1) {
            for (int p = n * 2; p >= 0; p--) {
                int t = findPageTex(p);
                if (t != -1) {
                    freeTex(t);
                    m_texPage[t] = -1;
                }
            }
            break;
        }
    }
    for (int n = (m_curPage + 1) / 2 + 1; n < m_numSheets; n++) {
        if (findSheet(n) == -1) {
            for (int p = n * 2 + 3; p < m_page->m_numPages; p++) {
                int t = findPageTex(p);
                if (t != -1) {
                    freeTex(t);
                    m_texPage[t] = -1;
                }
            }
            return;
        }
    }
}

// ROR 0x00199a08

void MnuDiary::setSheetTex(int n, int page, int back)
{
    int r = findSheet(n);
    int t = findPageTex(page);
    if (r == -1 || t == -1)     /* PC: the ROM indexes slot -1 */
        return;
    I3rItem *side = i3rGetChild52(&m_models[r].m_root->m_item, back != 0);
    I3rItem *mat = i3rGetChild25(side, 0);
    *(int *)((u_char *)mat->getData() + 0x1c) = m_texs[t].m_tex;
    *(short *)((u_char *)side->getData() + 8) += 1;     // its change count
}

// ROR 0x00199ab4

void MnuDiary::setBaseTex(int page, int back)
{
    int t = findPageTex(page);
    if (t == -1)                /* PC: the ROM indexes slot -1 */
        return;
    I3rItem *side = i3rGetChild52(&m_models[5].m_root->m_item, back != 0);
    I3rItem *mat = i3rGetChild25(side, 0);
    *(int *)((u_char *)mat->getData() + 0x1c) = m_texs[t].m_tex;
    *(short *)((u_char *)side->getData() + 8) += 1;
}

// ──────────────────────────────────────────────────────────────────────
// Sheet n turns (right: from the right side over to the left); the sheet
// beyond it is laid where n lay, and the pages past it are queued.
// ROR 0x00199b3c

void MnuDiary::turnSheet(int n, int right)
{
    int r = findSheet(n);
    MnuDiarySheet *sh = &m_sheets[r];
    sh->m_motion = right != 0;
    sh->m_frame = 0;
    bindMotion(r, right != 0);
    setSheetTex(n, n * 2 + 1, 0);
    setSheetTex(n, n * 2 + 2, 1);
    m_turnWait = 10;
    setBaseTex(right ? n * 2 + 3 : n * 2, right);
    queueSheetTex(right ? n + 2 : n - 2);

    r = findFreeSheet();
    int next = right ? n + 1 : n - 1;
    int motion = right == 0;
    sh = &m_sheets[r];
    sh->m_sheet = next;
    sh->m_motion = motion;
    sh->m_flags |= MNU_DSHEET_USED;
    sh->m_frame = m_motions[motion].m_frames;
    bindMotion(r, motion);
    setSheetTex(next, next * 2 + 1, 0);
    setSheetTex(next, next * 2 + 2, 1);
}

// ──────────────────────────────────────────────────────────────────────
// The background page (texture slot 1) and the turn arrows (slot 0), as
// the picture book's, then the base model and the shown sheets: the diary
// laid at -60 degrees about x, turned 180 about y, at (21, 8, 220).
// ROR 0x00199c94

void MnuDiary::draw(I3dLayer *layer)
{
    if ((u_int)(m_10 - 2) >= 3)
        return;

    beginDraw(layer);
    useTex(1);
    MnuSprite spr;
    spr.m_x = 0.0f;
    spr.m_y = 0.0f;
    spr.m_w = 512.0f;
    spr.m_h = 448.0f;
    spr.m_uv[0] = 0.0f;
    spr.m_uv[1] = 0.0f;
    spr.m_uv[2] = 1.0f;
    spr.m_uv[3] = 0.875f;
    spr.m_color[0] = 1.0f;
    spr.m_color[1] = 1.0f;
    spr.m_color[2] = 1.0f;
    spr.m_color[3] = 1.0f;
    spr.m_angle = 0.0f;
    drawSprite(0.0f, 0.0f, &spr);

    useTex(0);
    if (m_curPage - 1 >= 0) {
        u_int cell = (m_flags & MNU_PBOOK_TURN_LEFT) ? ((u_int)m_blink >> 1) % 3 : 0;
        float v = (float)cell * 16.0f + 48.0f;
        spr.m_w = 28.0f;
        spr.m_h = 16.0f;
        spr.m_uv[0] = 0.0f;
        spr.m_uv[1] = v * (1.0f / 128.0f);
        spr.m_uv[2] = 0.21875f;
        spr.m_uv[3] = (v + 16.0f) * (1.0f / 128.0f);
        drawSprite(100.0f, 412.0f, &spr);
    }
    if (m_curPage + 2 < m_page->m_numPages) {
        u_int cell = (m_flags & MNU_PBOOK_TURN_RIGHT) ? ((u_int)m_blink >> 1) % 3 : 0;
        float v = (float)cell * 16.0f + 0.0f;
        spr.m_w = 28.0f;
        spr.m_h = 16.0f;
        spr.m_uv[0] = 0.0f;
        spr.m_uv[1] = v * (1.0f / 128.0f);
        spr.m_uv[2] = 0.21875f;
        spr.m_uv[3] = (v + 16.0f) * (1.0f / 128.0f);
        drawSprite(384.0f, 412.0f, &spr);
    }
    m_pkt = i3dPktSetReg(m_pkt, 0x47 /* TEST_1 */, 0x503fd);
    endDraw(layer);

    sceVu0FMATRIX world;
    float rot[4] = { -1.0471976f, 3.1415927f, 0.0f, 0.0f };     // inline constants
    i3dMatUnit(world);
    i3dMatRotZXY(world, world, rot);
    world[3][0] = 21.0f;
    world[3][1] = 8.0f;
    world[3][2] = 220.0f;
    drawModel(layer, world, 5, -1, 0);
    const MnuDiarySheet *sh = m_sheets;
    for (int i = 0; i < 5; i++, sh++) {
        if (sh->m_flags & MNU_DSHEET_SHOWN)
            drawModel(layer, world, i, sh->m_motion, sh->m_frame);
    }
}

// ROR 0x00199fcc

void MnuDiary::setDrawSet(I3dLayer *layer)
{
    int texNo[2];
    texNo[0] = m_texs[0].m_tex;
    m_texs[0].m_set = 0;
    texNo[1] = m_texs[1].m_tex;
    m_texs[1].m_set = 1;
    I3dTexSetDesc desc;
    desc.m_depth = 0.0f;
    desc.m_tex_no = texNo;
    desc.m_tex_count = 2;
    desc.m_sort_func = 0;
    desc.m_sort_order = 0;
    desc.m_sort_param = 0;
    m_drawSet = i3dLayerFindDrawSet(layer, &desc);
    i3dLayerSetDrawSet(layer, m_drawSet);
}

// ──────────────────────────────────────────────────────────────────────
// A turn may start when a record is free, the sheet to turn is still and
// the page it uncovers is loaded (none to wait for past either end).
// ROR 0x0019a054

int MnuDiary::canTurn(int right)
{
    if (findFreeSheet() == -1)
        return 0;
    int n = (m_curPage + 1) / 2;
    int page;
    if (!right) {
        if (n - 1 < 0)
            return 0;
        if (isTurning(n - 1))
            return 0;
        if (n - 2 < -1)
            return 1;
        page = (n - 2) * 2 + 2;
    } else {
        if (n >= m_numSheets)
            return 0;
        if (isTurning(n))
            return 0;
        if (n + 1 >= m_numSheets)
            return 1;
        page = (n + 1) * 2 + 1;
    }
    int t = findPageTex(page);
    if (t == -1)
        return 0;
    return isTexLoaded(t) != 0;
}

// ROR 0x0019a174

int MnuDiary::isTurning(int n)
{
    int r = findSheet(n);
    if (r == -1)
        return 0;
    const MnuDiarySheet *sh = &m_sheets[r];
    return sh->m_frame < m_motions[sh->m_motion].m_frames;
}

// ROR 0x0019a1d0

int MnuDiary::findFreeSheet()
{
    for (int i = 0; i < 5; i++) {
        if (!(m_sheets[i].m_flags & MNU_DSHEET_USED))
            return i;
    }
    return -1;
}

// ROR 0x0019a20c

int MnuDiary::findSheet(int n)
{
    for (int i = 0; i < 5; i++) {
        if ((m_sheets[i].m_flags & MNU_DSHEET_USED) && m_sheets[i].m_sheet == n)
            return i;
    }
    return -1;
}

// ROR 0x0019a24c

int MnuDiary::findPageTex(int page)
{
    for (int i = 0; i < m_numTexs; i++) {
        if (m_texPage[i] == page)
            return i;
    }
    return -1;
}

// ROR 0x0019a290

void MnuDiary::queueSheetTex(int n)
{
    char name[0x20];
    for (int page = n * 2 + 1; page <= n * 2 + 2; page++) {
        if (page < 0 || page >= m_page->m_numPages || findPageTex(page) != -1)
            continue;
        int t = findFreeTex(3, 0x12);
        if (t == -1)            /* PC: the ROM writes slot -1 */
            continue;
        m_texPage[t] = page;
        sprintf(name, mnuDiaryPageFmt, page);
        addFile(name, MNU_FILE_TEX, t);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The text of the open spread: sys texts from the row's +4, page - 1
// (skipped when it is "##") and page; a page below -1 leaves the header
// alone.
// ROR 0x0019a3c0

void MnuDiary::setPageText(int page)
{
    memset(m_text, 0, 0x4c8);
    memcpy(m_text, mnuDiaryTextHead, sizeof(mnuDiaryTextHead));
    if (page >= -1) {
        const char *t = msgGetSysText(m_page->m_04 + page - 1);
        if (t && strcmp(t, mnuDiaryNoText) != 0) {
            size_t at = strlen(m_text);
            memcpy(m_text + at, t, strlen(t));
        }
        t = msgGetSysText(m_page->m_04 + page);
        if (t) {
            size_t at = strlen(m_text);
            memcpy(m_text + at, t, strlen(t));
        }
    }
    m_msg.setMessage(m_textWindow, m_text, 0);
}
