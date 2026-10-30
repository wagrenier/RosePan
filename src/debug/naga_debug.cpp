/* ==========================================================================
 *  debug/naga_debug.cpp
 *
 *  EventTestTask, the event test overlay, and its file loader (see the
 *  header). Unreferenced in the retail ROM. Not written yet: disp
 *  0x216ad0 and the menu helpers 0x217260..0x217f10.
 * ======================================================================== */
#include "debug/naga_debug.h"
#include "common/rosepan_stub.h"

#include "bed/bedpad.h"
#include "game/gamfile.h"
#include "mag/magsystem.h"
#include "utl/utlmisc.h"

static const char s_file[] = "debug/naga_debug.cpp";

/* bss 0x2db2a4: the overlay's mode -- 0 off, 1 / 2 the info pages (pad 0
 * R3 cycles 0..2), 3 the menu (pad 1 SELECT toggles 3 / 0). */
static int s_nagaDbgMode;
/* bss 0x2db2b8: the menu toggles it; while set exec pins the light colour
 * to 0.99, clears the scene's m_2aa84 and sets the view far to 10000. */
static int s_nagaDbgFixView;

/* bss 0x7b4e08..0x7b5a08: text the constructor clears (first byte of each
 * string, the ROM's inlined copy of ""), read by disp / the menu. Sizes
 * from the address gaps. */
static char s_nagaDbgText0[0x20];               // bss 0x7b4e08
static char s_nagaDbgText1[0xa0];               // bss 0x7b4e28
static char s_nagaDbgLine[40][0x40];            // bss 0x7b4ec8
static int  s_nagaDbgLineValue[40];             // bss 0x7b5968

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00216768
// callcheck: FileReadReq, FileReader, ~FileReader (inline in game/gamfile.h: the locals' constructors and destructor)

void *nagaDbgLoadFile(const char *theName)
{
    FileReader reader;
    int size = fileGetSize(theName);
    void *buf = g_utlMemory.allocDebug(size, s_file, 111, 0, 2);
    FileReadReq req;
    req.m_08 = 1;
    req.m_name = theName;
    req.m_offset = 0;
    req.m_whence = 0;
    req.m_buf = buf;
    req.m_size = size;
    req.m_close = 1;
    reader.read(&req, 0);
    return buf;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00216828

EventTestTask::EventTestTask()
{
    s_nagaDbgText1[0] = '\0';
    s_nagaDbgMode = 0;
    s_nagaDbgText0[0] = '\0';
    for (int i = 0; i < 40; i++) {
        s_nagaDbgLineValue[i] = 0;
        s_nagaDbgLine[i][0] = '\0';
    }
    m_hold_left = 0;
    m_hold_right = 0;
    m_hold_down = 0;
    m_hold_up = 0;
    m_top = 0;
    m_frame = 0;
    m_file = NULL;
    m_entries = NULL;
    m_state = 0;
    m_cursor = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002168f8

int EventTestTask::exec(int id, int time, int arg)
{
    (void)id; (void)time; (void)arg;
    BedPad *pad1 = g_magSystem.m_pad_ref[1].m_pad;

    if (pad1->m_hold & 0x10)
        m_hold_up++;
    else
        m_hold_up = 0;
    if (pad1->m_hold & 0x40)
        m_hold_down++;
    else
        m_hold_down = 0;
    if (pad1->m_hold & 0x80)
        m_hold_left++;
    else
        m_hold_left = 0;
    if (pad1->m_hold & 0x20)
        m_hold_right++;
    else
        m_hold_right = 0;
    m_frame++;

    if (g_magSystem.m_pad_ref[0].m_pad->m_repeat & 4)       // R3
        s_nagaDbgMode = (s_nagaDbgMode + 1) % 3;
    if (pad1->m_repeat & 1)                                 // SELECT
        s_nagaDbgMode = (s_nagaDbgMode == 3) ? 0 : 3;

    if (s_nagaDbgFixView != 0) {
        g_magSystem.setLightColor(0, 0, 2, 0.99f, 0.99f, 0.99f, 1.0f);
        g_magSystem.m_scene.m_2aa84 = 0;
        g_magSystem.m_scene.setViewFar(10000.0f);
    }
    disp();
    return 0;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00216ad0
int EventTestTask::disp()
{
    ROSEPAN_STUB("EventTestTask::disp", 0x00216ad0);
    return {};
}
