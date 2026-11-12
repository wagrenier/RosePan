// mag/magscenedebug.cpp -- the scene's debug windows: up to four levels
// (m_dbg_window[0..3]), each a MagLifeWindowDebug in one of the scene's
// MAX_WINDOW text-window slots, run by MagScene::update in sub mode 9 from
// the top level down. Nothing in the retail ROM calls openDebugWindow, so
// every level stays -1 and updateDebug does nothing.

#include "mag/magscene.h"

#include "bed/bedpad.h"
#include "common/ror_assert.h"
#include "mag/maglifewindow.h"

// ──────────────────────────────────────────────────────────────────────
// Runs the open debug windows from level 3 down: live, then processPad;
// the first window whose processPad returns nonzero takes the pads.
// ROR 0x0014313c // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagScene::updateDebug(BedPad *pad0, BedPad *pad1)
{
    for (int i = 3; i >= 0; i--)
    {
        if (m_dbg_window[i] < 0)
            continue;
        ROR_ASSERT(m_window[m_dbg_window[i]]); /* 32 */
        m_window[m_dbg_window[i]]->live(); /* EE slot 2 */
        /* EE slot 12: MagLifeWindow::processPad (returns 0) in every
         * window the ROM opens here; the host has no processPad slot. */
        if (m_window[m_dbg_window[i]]->processPad(*pad0, *pad1))
            return;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Opens theLevel's debug window in the first free slot: 0x200 x 0x100 with
// that slot's font buffer, on the life manager, fill MSB on, at (0, 0xa0,
// 30000). Returns the slot, -1 when all MAX_WINDOW are taken.
// ROR 0x0014324c // callcheck: dbgAssertFailed, operator_new (the assert handler, inside ROR_ASSERT; new MagLifeWindowDebug)

int MagScene::openDebugWindow(int theLevel)
{
    ROR_ASSERT(m_dbg_window[theLevel] < 0); /* 44 */

    int n;
    for (n = 0; n < MAX_WINDOW; n++)
        if (m_window[n] == NULL)
            break;
    if (n >= MAX_WINDOW)
        return -1;

    m_window[n] = new MagLifeWindowDebug();
    ROR_ASSERT(m_window[n]); /* 56 */
    m_window[n]->setup(0x200, 0x100);
    GamFont *font = &m_window_font[n];
    font->setFont(m_font_data);
    m_window[n]->setFontManager(font);
    m_window[n]->m_tex_no = m_font_tex_no[n];
    m_life_man.add(m_window[n], 0x8000);
    setWindowFillMsb(n, 1);
    updateWindowFont(n);
    setWindowSize(n, 0x200, 0x100);
    setWindowPos(n, 0, 0xa0, 30000);
    setWindowWords84(n, 0, 0, 0x200, 0x100);
    m_dbg_window[theLevel] = n;
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// Closes theLevel's debug window.
// ROR 0x00143430 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagScene::closeDebugWindow(int theLevel)
{
    ROR_ASSERT(m_dbg_window[theLevel] >= 0); /* 89 */
    closeWindow(m_dbg_window[theLevel]);
    m_dbg_window[theLevel] = -1;
}
