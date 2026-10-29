/* ==========================================================================
 *  mnu/mnuselect.cpp
 *
 *  The text list (0x1a066c..0x1a1244; RoR's own path, which allocText
 *  passes to the heap, between MnuStock's mnu/mnustock.cpp and
 *  mnu/mnumcd.cpp). See mnu/mnuselect.h.
 *
 *  PORT: all of MnuTextList.
 * ======================================================================== */
#include "mnu/mnuselect.h"

#include <string.h>

#include "bed/bedpad.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "spr/s2d.h"
#include "utl/utlmisc.h"

/* ---- data ---- */

/* The label separator (rodata 0x2f8530). */
static const char mnuTextListSep[] = "#0#0";

/* select's frames since the cursor last moved (sbss 0x2a4524) and its
 * "rows drawn" flag (sbss 0x2a4528: set before every draw, so the cursor
 * row is always drawn; the branch that skipped it is dead). Only select
 * uses them. */
static int mnuTextListMoveFrames;
static int mnuTextListDrawn;

/* The cursor: message sprite 1. */
#define MNU_TEXT_LIST_CURSOR    1

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a066c

MnuTextList::MnuTextList()
{
    m_open = 0;
    clear();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a0684; callcheck: operator_delete (the deleting variant: implicit on the host)

MnuTextList::~MnuTextList()
{
    close();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a06d4

int MnuTextList::processPad(BedPad *pad)
{
    return step(pad);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a06e8

int MnuTextList::open()
{
    if (m_open == 1)
        return -1;
    m_z = 0x7fff;
    m_open = 1;
    m_active = 0;
    m_window = -1;
    m_y = 100;
    m_horizontal = 0;
    m_step = 0;
    m_pause = 0;
    m_state = 0;
    m_x = 100;
    allocText();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a074c

int MnuTextList::close()
{
    if (m_open == 0)
        return -1;
    freeText();
    m_open = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a0788

int MnuTextList::step(BedPad *pad)
{
    if (m_open == 0)
        return -1;
    switch (m_step) {
    case 0:
        m_step = 1;
        return 0;
    case 1:
        m_step = 2;
        return 0;
    case 2:
        if (m_pause == 0 && select(pad) == 1)
            m_step = 4;
        stepExtra(pad);
        return 0;
    case 4:
        m_step = 5;
        /* fall through */
    case 3:
    case 5:
    default:
        return 1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a0860

int MnuTextList::clear()
{
    m_cur = 0;
    for (int i = 0; i < 6; i++)
        m_08[i] = 0;
    m_28 = 2;
    m_mask = 0;
    m_24 = 2;
    m_active = 0;
    m_rows = 1;
    m_window = -1;
    for (int i = 0; i < 32; i++)
        m_text[i] = NULL;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a08d4

void MnuTextList::allocText()
{
    if (m_text[0] != NULL)
        return;
    for (int i = 0; i < 32; i++)
        m_text[i] = (char *)g_utlMemory.allocDebug(0x20, "mnu/mnuselect.cpp", 0xb5, 0, 2);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a0950

void MnuTextList::freeText()
{
    if (m_text[0] == NULL)
        return;
    for (int i = 0; i < 32; i++) {
        utlMemFree(m_text[i]);
        m_text[i] = NULL;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Copies the labels into the row buffers (31 characters each, the rest
// of a longer label skipped) and counts the rows the mask shows.
// ROR 0x001a09b8

void MnuTextList::setItems(const char *items, u_int mask)
{
    if (m_open == 0 || m_active == 1)
        return;
    m_cancel = (int)mask >> 18 & 1;
    m_horizontal = (int)mask >> 16 & 1;
    m_active = 1;
    m_select = MNU_TEXT_LIST_NONE;
    m_mask = mask;
    m_num = 0;
    m_rows = 0;

    if (*items == '#') {
        if ((u_char)items[1] - '0' < 10u)
            m_cur = items[1] - '0';
        items += 2;
    }

    int len = 0;
    while (*items != '\0') {
        if (strncmp(mnuTextListSep, items, 4) == 0) {
            m_text[m_num][len] = '\0';
            int n = m_num++;
            if (m_num > 31)
                break;
            items += 4;
            len = 0;
            m_text[n + 1][0] = '\0';
        } else {
            m_text[m_num][len++] = *items++;
            if (len > 30) {
                m_text[m_num][len] = '\0';
                while (*items != '\0' && strncmp(mnuTextListSep, items, 4) != 0)
                    items++;
            }
        }
    }

    /* PC: with 32 labels RoR ends a 33rd row past m_text; skip it. */
    if (m_num < 32)
        m_text[m_num][len] = '\0';
    m_num++;
    int rows = 0;
    for (int i = 0; i < m_num; i++)
        rows += (int)m_mask >> (i & 0x1f) & 1;
    m_rows = rows;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a0ba0

int MnuTextList::getSelect()
{
    if (m_open == 0)
        return MNU_TEXT_LIST_CLOSED;
    return m_select;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a0bb8

void MnuTextList::endSelect()
{
    if (m_open != 0 && m_active == 1)
        m_state = 4;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a0bdc

int MnuTextList::stepExtra(BedPad *pad)
{
    (void)pad;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Opens a 512 x 256 window at m_x / m_y with the cursor sprite, moves the
// cursor (d-pad up / down, or left / right when horizontal, wrapping over
// the rows shown), cross chooses the row under it, circle cancels when
// allowed; the rows are redrawn when the window opens or the cursor moves
// (the cursor's row in colour 0, the others in 8).
// ROR 0x001a0be4

int MnuTextList::select(BedPad *pad)
{
    MagSystem *sys = MagSystem::m_current_system;
    MagScene *scene = &sys->m_scene;
    S2dMes &mes = g_magSystem.m_scene.m_mes;
    bool redraw = false;
    int result = 0;

    switch (m_state) {
    case 0:
        m_state = 1;
        mnuTextListMoveFrames = 0;
        mnuTextListDrawn = 0;
        /* fall through */
    case 1:
        if (m_active == 1) {
            m_window = scene->openWindow(0x200, 0x100, 0, -1);
            redraw = true;
            sys->setViewParam(m_window, m_x, m_y, m_z, 0x200, 0x100);
            scene->setWindowWords84(m_window, 0, 0, 0x200, 0x100);
            scene->getWindowWordCc(m_window, &m_line);
            m_line = 0x12;
            mes.setSpriteStdPoint(MNU_TEXT_LIST_CURSOR, m_x << 4, m_y << 4);
            mes.setSpriteOffPoint(MNU_TEXT_LIST_CURSOR, 0, 0);
            mes.setSpriteProcessOn(MNU_TEXT_LIST_CURSOR);
            mes.setSpriteColorStd(MNU_TEXT_LIST_CURSOR);
            mes.setSpriteState(MNU_TEXT_LIST_CURSOR, 1);
            mes.setSpriteDisp(MNU_TEXT_LIST_CURSOR, 1);
            mes.setSpriteAnim(MNU_TEXT_LIST_CURSOR, 0);
            m_state = 3;
        }
        break;
    case 3:
        sys->setViewParam(m_window, m_x, m_y, m_z, 0x200, 0x100);
        mes.setSpriteStdPoint(MNU_TEXT_LIST_CURSOR, m_x << 4, m_y << 4);
        break;
    case 4:
        scene->setWindowShow(m_window, 1);
        result = 1;
        scene->closeWindow(m_window);
        mes.setSpriteState(MNU_TEXT_LIST_CURSOR, 0);
        mes.setSpriteDisp(MNU_TEXT_LIST_CURSOR, 0);
        m_state = 1;
        m_active = 0;
        break;
    }
    if (m_active == 0)
        return result;

    if (pad->m_trig & 0x4000) {
        /* Cross: the m_cur-th row shown. */
        int k = 0;
        for (int i = 0; i < m_num; i++) {
            if (!((int)m_mask >> i & 1))
                continue;
            if (m_cur == k) {
                m_select = i;
                m_state = 4;
                break;
            }
            k++;
        }
    } else if (m_horizontal == 0 || m_horizontal == 1) {
        u_int prev = m_horizontal == 0 ? 0x10 : 0x80;     /* up / left */
        u_int next = m_horizontal == 0 ? 0x40 : 0x20;     /* down / right */
        if (pad->m_dir_repeat & prev) {
            mnuTextListMoveFrames = 0;
            redraw = true;
            m_cur = (m_cur < 1 ? m_rows : m_cur) - 1;
        }
        if (pad->m_dir_repeat & next) {
            mnuTextListMoveFrames = 0;
            redraw = true;
            if (m_cur < m_rows - 1)
                m_cur++;
            else
                m_cur = 0;
        }
    }
    if (m_cancel == 1 && (pad->m_trig & 0x2000)) {
        m_select = MNU_TEXT_LIST_CANCELLED;
        m_state = 4;
    }
    mnuTextListDrawn = 1;
    mnuTextListMoveFrames++;

    if (redraw) {
        scene->updateWindowFont(m_window);
        if (m_horizontal == 0) {
            int k = 0;
            for (int i = 0; i < m_num; i++) {
                if (!((int)m_mask >> i & 1))
                    continue;
                int color = 8;
                bool cur = k == m_cur;
                if (cur) {
                    color = 0;
                    mes.setSpriteOffPoint(MNU_TEXT_LIST_CURSOR, 0, m_line * k * 0x10);
                }
                if (!cur || mnuTextListDrawn != 0)
                    sys->drawView(m_window, 0x20, m_line * k, m_text[i], color);
                k++;
            }
        } else if (m_horizontal == 1) {
            int x = 0;
            for (int i = 0; i < m_num; i++) {
                if (!((int)m_mask >> i & 1))
                    continue;
                int color = 8;
                if (i == m_cur) {
                    color = 0;
                    mes.setSpriteOffPoint(MNU_TEXT_LIST_CURSOR, x << 4, 0);
                }
                if (i != m_cur || mnuTextListDrawn != 0)
                    sys->drawView(m_window, x + 0x20, 0, m_text[i], color);
                x += scene->getWindowTextWidth(m_window, m_text[i]);
            }
        }
    }
    return result;
}
