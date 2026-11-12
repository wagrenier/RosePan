/* ==========================================================================
 *  debug/mcd_debug.cpp
 *
 *  The second assert handler and the memory-card menu test (pinned
 *  0x2155f4..0x215a30; the file name is invented): see mcd_debug.h. The
 *  test is unreferenced in the retail ROM, and its state machine has no
 *  way into the step that runs the menu (nothing sets state 3), so all it
 *  can do is print its banner and a separator line whenever a button or
 *  the left stick moves.
 * ======================================================================== */
#include "debug/mcd_debug.h"

#include <stdio.h>

#include "bed/bedpad.h"
#include "mag/magsystem.h"
#include "mnu/mnumcd.h"

/* data 0x2d9e80, constructed and destroyed by the file's static
 * initializer: __static_initialization_and_destruction_0$mcd_debug
 * 0x215998, _GLOBAL_$I$mcd_debug 0x2159f0, _GLOBAL_$D$mcd_debug 0x215a10. */
MnuMcd g_mcdDbgMenu;

/* bss 0x2db280..0x2db29c: the test's state. */
static int s_mcdDbgValue[3];    // bss 0x2db280: three values, 0 unbounded above, 1 and 2 kept to 0..6
static int s_mcdDbgCursor;      // bss 0x2db28c: which value up / down change, 0..2 (set to 3 on start)
static int s_mcdDbgMode;        // bss 0x2db290: 0 on boot, set to 1; the test runs in mode 1 only
static int s_mcdDbgPause;       // bss 0x2db294: never written; non-zero would stop the test
static int s_mcdDbgState;       // bss 0x2db298: 0 start, 1 wait, 2 idle, 3 run the menu, 4 close it

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002155f4; callcheck: scePrintf (printf on the host)

void mcdDbgAssertFailed(const char *file, int line, const char *expr)
{
    printf("Assertion Failed: %s(%d)   %s\n", file, line, expr);
    // PORT: the ROM loops for ever here (common/ror_assert.h).
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00215634; callcheck: scePrintf, ~BedPad (printf on the host; the unused local's destructor is implicit)

void mcdDbgMenuTest()
{
    MagSystem *sys = MagSystem::m_current_system;
    BedPad *pad = &sys->m_pad[0];
    BedPad unused;          // built and destroyed, never read (the ROM's stack 0x00..0x90)
    int changed = 0;

    (void)unused;
    if (s_mcdDbgMode == 0)
        s_mcdDbgMode = 1;
    if (s_mcdDbgMode != 1 || s_mcdDbgPause != 0)
        return;

    switch (s_mcdDbgState) {
    case 0:
        changed = 1;
        printf("--------------------------------------------------futuki\n");
        s_mcdDbgState = 1;
        s_mcdDbgValue[0] = 0;
        s_mcdDbgCursor = 3;
        s_mcdDbgValue[1] = 0;
        s_mcdDbgValue[2] = 0;
        break;
    case 1:
        if (pad->m_repeat & 0x8000)     // square
            s_mcdDbgState = 2;
        break;
    case 2:
        break;
    case 3:
        g_mcdDbgMenu.processPad(pad);
        if (pad->m_repeat & 0x8000)
            s_mcdDbgState = 4;
        break;
    case 4:
        g_mcdDbgMenu.term();
        s_mcdDbgState = 1;
        break;
    }

    // Up / down step the value under the cursor; a cursor of 3 (the
    // start value) has none, and only marks the input.
    if (pad->m_repeat & 0x10) {         // up
        changed = 1;
        if (s_mcdDbgCursor >= 0 && s_mcdDbgCursor < 3 && s_mcdDbgValue[s_mcdDbgCursor] > 0)
            s_mcdDbgValue[s_mcdDbgCursor]--;
    }
    if (pad->m_repeat & 0x40) {         // down
        changed = 1;
        if (s_mcdDbgCursor == 0)
            s_mcdDbgValue[0]++;
        else if ((s_mcdDbgCursor == 1 || s_mcdDbgCursor == 2) && s_mcdDbgValue[s_mcdDbgCursor] < 6)
            s_mcdDbgValue[s_mcdDbgCursor]++;
    }
    if (pad->m_repeat & 0x20) {         // right
        changed = 1;
        if (s_mcdDbgCursor < 2)
            s_mcdDbgCursor++;
    }
    if (pad->m_repeat & 0x80) {         // left
        changed = 1;
        if (s_mcdDbgCursor > 0)
            s_mcdDbgCursor--;
    }
    if (pad->m_repeat & 0x400)          // L1
        changed = 1;
    if (pad->m_repeat & 0x100)          // L2
        changed = 1;
    if (pad->getLeftX() > 0.099999994f)
        changed = 1;
    if (pad->getLeftX() < -0.099999994f)
        changed = 1;
    if (pad->getLeftY() > 0.099999994f)
        changed = 1;
    if (pad->getLeftY() < -0.099999994f)
        changed = 1;
    if (changed || (pad->m_repeat & 0x4000))    // cross
        printf("--------------------------------------------------\n");
}
