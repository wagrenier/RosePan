/* ==========================================================================
 *  mag/magscenemenu.cpp
 *
 *  MagScene's menu glue (0x13f07c..0x13f49c; the run has no source-path
 *  string, so the file name is the port's): entering and leaving a menu
 *  through g_gamMan's pause, whether the item and map menus may open, the
 *  menu fade times the scripts set in g_sequence, and the system message
 *  window -- a system text expanded with its arguments into a window, the
 *  scene held in sub mode 5 until closeSysMsgWindow.
 * ======================================================================== */
#include "mag/magscene.h"

#include "game/gamman.h"
#include "msg/msgman.h"
#include "sequence/sequence.h"

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013f07c

void MagScene::gamEnterMenu(int menu)
{
    g_gamMan.pauseForMenu(menu);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013f09c

void MagScene::gamLeaveMenu()
{
    g_gamMan.resumeFromMenu(0);
}

// ──────────────────────────────────────────────────────────────────────
// Not while the PC is in state 7, 10 or 0x13.
// ROR 0x0013f0c0

int MagScene::canOpenItemMenu()
{
    int state = m_pc.m_state;
    if (state == 7 || state == 0x13 || state == 10)
        return 0;
    return g_gamMan.canOpenItemMenu() != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013f114

int MagScene::canOpenMenu10001()
{
    return g_gamMan.canOpenMapMenu();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013f130

void MagScene::setGamWord54()
{
    g_sequence.m_global[0x54 / 4] = 10;
}

// ──────────────────────────────────────────────────────────────────────
// Flag bit 0x4000000: no fade; bit 0x2000000: the script's time (global
// 0x170), else 15 frames.
// ROR 0x0013f144

int MagScene::getMenuFadeOutTime()
{
    u_int bits = g_sequence.m_bits[1];
    if (bits & 0x4000000)
        return 0;
    if (bits & 0x2000000)
        return g_sequence.m_global[0x170 / 4];
    return 15;
}

// ──────────────────────────────────────────────────────────────────────
// The same with global 0x16c.
// ROR 0x0013f194

int MagScene::getMenuFadeInTime()
{
    u_int bits = g_sequence.m_bits[1];
    if (bits & 0x4000000)
        return 0;
    if (bits & 0x2000000)
        return g_sequence.m_global[0x16c / 4];
    return 15;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013f1e4

void MagScene::clearGamFlag25()
{
    g_sequence.m_bits[1] &= ~0x2000000u;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013f210

int MagScene::pulseMenu1000a()
{
    m_sub_mode.push();
    m_sub_mode.m_step = -1;
    m_sub_mode.m_mode = 5;
    gamEnterMenu(0x1000a);
    gamLeaveMenu();
    m_sub_mode.pop();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// System text msg expanded with args (16 words; NULL leaves them unset, as
// the ROM does) into a 256 x 96 window at (128, 224), the scene held in
// sub mode 5 on the 0x1000a menu's pause. Returns the window (-1 none).
// callcheck: MsgMan, ~MsgMan (the ROM's two stack MsgMan; not written: plain zeroed records on the host)
// ROR 0x0013f280

int MagScene::openSysMsgWindow(int msg, const int *args, int fillMsb)
{
    char text[0x400];
    MsgExpand ex;
    ex.m_src = msgGetSysText(msg);
    if (args != NULL)
    {
        for (int i = 0; i < 16; i++)
            ex.m_args[i] = args[i];
    }
    ex.m_dst_size = sizeof(text);
    ex.m_dst = text;
    /* PORT: expandText clears the buffer first; while it is a stub the
     * window gets an empty text instead of stack garbage. */
    text[0] = '\0';
    {
        MsgMan mm[2] = {};
        mm[0].expandText(&ex);
    }
    text[sizeof(text) - 1] = '\0';

    int win = openWindow(0x100, 0x100, 0, 100);
    if (win >= 0)
    {
        setWindowFillMsb(win, fillMsb);
        updateWindowFont(win);
        setWindowSize(win, 0x100, 0x60);
        drawView(win, 0, 0, text, 0);
        setWindowPos(win, 0x80, 0xe0, 30000);
        setWindowWords84(win, 0, 0, 0x100, 0x60);
    }
    m_sub_mode.push();
    m_sub_mode.m_mode = 5;
    m_sub_mode.m_step = -1;
    m_sys_msg_window = win;
    gamEnterMenu(0x1000a);
    return win;
}

// ──────────────────────────────────────────────────────────────────────
// A text window slot is in use (GamMan::isMsgBusy, native 004).
// ROR 0x0013f444

int MagScene::isWindowOpen(int win)
{
    return m_window[win] != NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013f458

void MagScene::closeSysMsgWindow(int win)
{
    gamLeaveMenu();
    closeWindow(win);
    m_sub_mode.pop();
}
