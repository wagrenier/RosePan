/* ==========================================================================
 *  debug/naga_debug.cpp
 *
 *  EventTestTask, the event test overlay, and its file loader (see the
 *  header). Unreferenced in the retail ROM. Its text is formatted into
 *  a stack buffer and dropped: the draw calls were stripped from retail,
 *  so the pages show nothing even when the task runs.
 * ======================================================================== */
#include "debug/naga_debug.h"
#include "common/rosepan_stub.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bed/bedmemory.h"
#include "bed/bedpad.h"
#include "game/gamfile.h"
#include "game/gamman.h"
#include "mag/magscenecmd.h"
#include "mag/magsystem.h"
#include "utl/utlmisc.h"
#include "wrd/wrdsrv.h"

static const char s_file[] = "debug/naga_debug.cpp";

/* data 0x2db2a0: the lines' scroll step, a quarter of it taken off each
 * frame (nothing in retail sets it). */
static int s_nagaDbgScroll = 0;
/* data 0x2db2a4: the overlay's mode -- 0 off, 1 / 2 the info pages (pad 0
 * R3 cycles 0..2), 3 the menu (pad 1 SELECT toggles 3 / 0). */
static int s_nagaDbgMode = 0;
/* data 0x2db2a8: the smallest largest-free-block of the game heap seen on
 * page 2; mode 0 resets it. */
static int s_nagaDbgMinFree = 0x40000000;
/* data 0x2db2ac: counts down to 0x40 (a fade; nothing raises it). */
static int s_nagaDbgFade = 0x40;
/* data 0x2db2b0: a flash's frame, 0 idle; 1..9 the rise, ends at 200. */
static int s_nagaDbgFlash = 0;
/* data 0x2db2b4: the switches page's item 2; the scene's m_22b8 is its
 * inverse. */
static int s_nagaDbgSceneSwitch = 1;
/* data 0x2db2b8: the menu toggles it; while set exec pins the light colour
 * to 0.99, clears the scene's m_2aa84 and sets the view far to 10000. */
static int s_nagaDbgFixView = 0;
/* data 0x2db2bc = 1 is not read by this file. */

/* data 0x2db2c0: the chapter page's captions; its draw is stripped, so
 * only the table is left. */
[[maybe_unused]] static const char *const s_nagaDbgChapterName[13] = {
    "Title", "Chapter 1", "Chapter 2", "Chapter 3", "Chapter 4",
    "Chapter 5a", "Chapter 5b", "Chapter 6", "Chapter 7", "Chapter 8",
    "Chapter 10", "Chapter 11", "Chapter 12",
};
/* data 0x2db2f8: the chapter page's chapters for requestStage. */
static const int s_nagaDbgChapter[13] = { 99, 1, 2, 3, 4, 5, 13, 6, 7, 8, 10, 11, 12 };

/* bss 0x7b4e08..0x7b5aa8: text the constructor clears (first byte of each
 * string, the ROM's inlined copy of ""), and the lines' timers and
 * positions disp ages. Sizes from the address gaps. */
static char s_nagaDbgText0[0x20];               // bss 0x7b4e08
static char s_nagaDbgText1[0x20];               // bss 0x7b4e28
static char s_nagaDbgText2[0x80];               // bss 0x7b4e48: disp prints it with %s
static char s_nagaDbgLine[40][0x40];            // bss 0x7b4ec8
static int  s_nagaDbgLineValue[40];             // bss 0x7b5968: frames left, 0 the line is off
static int  s_nagaDbgLinePos[40];               // bss 0x7b5a08: the line's y, scrolled up by s_nagaDbgScroll

/* The ROM's wrap of an angle into -pi..pi, compared in double. */
static float nagaDbgWrapRad(float r)
{
    if ((double)r < -3.141592653589793)
        r = (float)((double)r + 6.283185307179586);
    if ((double)r > 3.141592653589793)
        r = (float)((double)r - 6.283185307179586);
    return r;
}

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


// ──────────────────────────────────────────────────────────────────────
// ROR 0x00216ad0
// callcheck: dpadd, dpcmp, dpdiv, dpsub, dptofp, fptodp (the EE's soft double: the host's double arithmetic)

void EventTestTask::disp()
{
    char buf[0x40];
    float pos[4];
    float rot[4];

    if (s_nagaDbgFade > 0x40)
        s_nagaDbgFade--;
    for (int i = 0; i < 40; i++) {
        if (s_nagaDbgLineValue[i] != 0)
            s_nagaDbgLineValue[i]--;
    }
    if (s_nagaDbgFlash != 0) {
        s_nagaDbgFlash++;
        if (s_nagaDbgFlash < 10) {
            float t = nagaDbgWrapRad((float)s_nagaDbgFlash / 10.0f * 3.1415925f * 0.5f);
            (void)sinf(t);
        } else if (s_nagaDbgFlash >= 0x78 && s_nagaDbgFlash >= 200) {
            s_nagaDbgFlash = 0;
        }
    }

    int step = s_nagaDbgScroll >> 2;
    s_nagaDbgScroll -= step;
    for (int i = 0; i < 40; i++) {
        if (s_nagaDbgLineValue[i] == 0)
            continue;
        s_nagaDbgLinePos[i] -= step;
        if (s_nagaDbgMode < 2) {
            if (s_nagaDbgLinePos[i] < 0x1f)
                s_nagaDbgLineValue[i] = 0;
        } else if (s_nagaDbgLinePos[i] < 0x51) {
            s_nagaDbgLineValue[i] = 0;
        }
    }

    if (s_nagaDbgMode == 3) {
        menu();
        return;
    }
    if (s_nagaDbgMode == 0)
        s_nagaDbgMinFree = 0x40000000;

    if (s_nagaDbgMode >= 2 && (int)g_gamMan.m_flags >= 0) {
        /* The frame count (blinking every 15 frames: the test is left, its
         * use stripped). */
        sprintf(buf, "%04d", (u_int)g_gamMan.m_vcount % 10000);

        /* The PC's position and heading in degrees. */
        memcpy(pos, g_gamMan.m_pcPos, sizeof(pos));
        memcpy(rot, g_gamMan.m_pcRot, sizeof(rot));
        sprintf(buf, "%6.1f %6.1f %6.1f (%6.1f)", pos[0], pos[1], pos[2],
                (rot[1] * 180.0f) / 3.141592653589793);
        /* The dog's actor. */
        if (g_gamMan.m_dogChrNo >= 0) {
            DogActor &dog = g_magSystem.m_scene.m_actors[g_gamMan.m_dogChrNo];
            float heading = dog.m_rotate[1];
            sprintf(buf, "%6.1f %6.1f %6.1f (%6.1f)", dog.m_trans[0], dog.m_trans[1],
                    dog.m_trans[2], (heading * 180.0f) / 3.141592653589793);
        }

        /* The PC's position in the world. */
        int room = g_gamMan.getWrdNo();
        if (g_wrdSrv.get(room) == NULL) {
            sprintf(buf, "(%d IS INVALID AREA)", room);
        } else {
            g_wrdSrv.cnvPosL2W(pos, room, pos);
            sprintf(buf, "%7.1f %7.1f %7.1f", pos[0], pos[1], pos[2]);
        }

        /* The game heap: largest free block, free bytes, the smallest
         * largest block seen. */
        BEDMEMINFO mem;
        g_utlMemory.getInfo(&mem);
        if (mem.m_max_free < s_nagaDbgMinFree)
            s_nagaDbgMinFree = mem.m_max_free;
        sprintf(buf, "%8d", mem.m_max_free);
        sprintf(buf, "%8d", mem.m_total_free);
        sprintf(buf, "%8d", s_nagaDbgMinFree);

        /* The packet buffer's use, in percent of 0x1ae000. */
        MagSystemInfo info = g_magSystem.m_info;
        sprintf(buf, "%.2f%%", ((float)(info.m_trash_size - info.m_trash_rest) / 1761280.0f) * 100.0f);
        sprintf(buf, "World %d", g_gamMan.m_chapter);
        int area = g_gamMan.getMapArea();
        sprintf(buf, "(%s)", area == 0 ? "MAP_AIRPLANE" : area == 1 ? "MAP_ORPH" : "-1");

        /* PC and dog parameters 0x1b / 0x1c (getPcParam1b / 1c's pair). */
        magBwnIsLevelAuto();
        u_short param[2];
        g_gamMan.getPcParam(0x1b, &param[1]);
        g_gamMan.getPcParam(0x1c, &param[0]);
        sprintf(buf, "%d", param[1]);
        g_gamMan.getDogParam(0x1b, &param[1]);
        g_gamMan.getDogParam(0x1c, &param[0]);
        sprintf(buf, "%d", param[1]);

        /* The camera's heading (a compass, its draw stripped). */
        const float *cam = g_magSystem.m_scene.m_camera_entity.m_rotate;
        pos[0] = cam[0];
        pos[1] = cam[1];
        pos[2] = cam[2];
        (void)sinf(pos[1]);
        (void)cosf(pos[1]);
    }

    if (s_nagaDbgMode > 0 && (int)g_gamMan.m_flags >= 0) {
        /* The script's step (global 0x27) and the event lines (their draw
         * stripped: the loop only reads the timers). */
        sprintf(buf, "%5d", g_sequence.m_global[0x27]);
        sprintf(buf, "%s", s_nagaDbgText2);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00217260

void EventTestTask::menu()
{
    char buf[0x40];
    const u_short repeat = g_magSystem.m_pad[1].m_repeat;
    int next;

    switch (m_state) {
    case 0:     /* the list of pages */
        moveCursor(8, 10);
        dispCursor(0x50);
        if (!(repeat & 0x4000))
            return;
        switch (m_cursor) {
        case 0:
            sprintf(buf, "data/seq/w%03d/DBGFLAG.BIN", g_gamMan.m_chapter);
            m_file = (NagaDbgBinHead *)nagaDbgLoadFile(buf);
            m_entries = (NagaDbgEntry *)(m_file + 1);
            if (m_file->m_count == 0)
                return;
            m_cursor = 0;
            m_state = 1;
            return;
        case 1:
            sprintf(buf, "data/seq/w%03d/DBGSTEP.BIN", g_gamMan.m_chapter);
            m_file = (NagaDbgBinHead *)nagaDbgLoadFile(buf);
            m_entries = (NagaDbgEntry *)(m_file + 1);
            if (m_file->m_count == 0)
                return;
            m_cursor = 0;
            m_state = 2;
            return;
        case 2:
            sprintf(buf, "data/seq/DBGITEM.BIN");
            m_file = (NagaDbgBinHead *)nagaDbgLoadFile(buf);
            m_entries = (NagaDbgEntry *)(m_file + 1);
            if (m_file->m_count == 0)
                return;
            m_cursor = 0;
            m_state = 5;
            return;
        case 3:
            next = 6;
            break;
        case 4:
            next = 4;
            break;
        case 5:
            sprintf(buf, "data/seq/w%03d/DBGAREA.BIN", g_gamMan.m_chapter);
            m_file = (NagaDbgBinHead *)nagaDbgLoadFile(buf);
            m_entries = (NagaDbgEntry *)(m_file + 1);
            if (m_file->m_count == 0)
                return;
            next = 3;
            break;
        case 6:
            g_magSystem.m_scene.requestMenu(0x10003, 0);
            s_nagaDbgMode = 0;
            return;
        case 7:
            m_state = 7;
            return;
        default:
            return;
        }
        m_cursor = 0;
        m_state = next;
        return;

    case 1:     /* event flags: select toggles the sequence bit */
        moveCursor(m_file->m_count, 17);
        for (int i = 0; i < 17 && m_top + i < m_file->m_count; i++)
            sprintf(buf, "%05d", m_entries[m_top + i].m_value);
        dispCursor(0x3c);
        dispScroll(m_top, 17, m_file->m_count);
        if (repeat & 0x4000) {
            u_int bit = (u_int)m_entries[m_cursor].m_value;
            u_int mask = 1u << (bit & 0x1f);
            u_int *word = &g_sequence.m_bits[bit >> 5];
            if (*word & mask)
                *word &= ~mask;
            else
                *word |= mask;
        }
        if (!(repeat & 0x2000))
            return;
        m_state = 0;
        m_cursor = 0;
        break;

    case 2:     /* steps: select sets global 0x27 */
        moveCursor(m_file->m_count, 17);
        for (int i = 0; i < 17 && m_top + i < m_file->m_count; i++)
            sprintf(buf, "%05d", m_entries[m_top + i].m_value);
        dispCursor(0x3c);
        dispScroll(m_top, 17, m_file->m_count);
        if (repeat & 0x4000) {
            g_sequence.m_global[0x27] = m_entries[m_cursor].m_value;
            sprintf(buf, "[Set Step] %s", m_entries[m_cursor].m_name);
        }
        if (!(repeat & 0x2000))
            return;
        m_state = 0;
        m_cursor = 1;
        break;

    case 3:     /* areas: select warps to the room */
        moveCursor(m_file->m_count, 17);
        for (int i = 0; i < 17 && m_top + i < m_file->m_count; i++)
            sprintf(buf, "%05d", m_entries[m_top + i].m_value);
        dispCursor(0x3c);
        dispScroll(m_top, 17, m_file->m_count);
        if (repeat & 0x4000) {
            int words[4];
            memset(words, 0, sizeof(words));
            g_gamMan.pause(0x20);
            memcpy(g_gamMan.m_sysWord, words, sizeof(words));
            g_gamMan.requestStage(2, m_entries[m_cursor].m_value, 0, -1, 0xcccccccdu);
            s_nagaDbgMode = 0;
        }
        if (!(repeat & 0x2000))
            return;
        m_state = 0;
        m_cursor = 5;
        break;

    case 4:     /* chapters: select loads the chapter */
        moveCursor(13, 17);
        dispCursor(0x3c);
        if (repeat & 0x4000) {
            g_gamMan.requestStage(1, -1, -1, s_nagaDbgChapter[m_cursor], 0xccccccccu);
            s_nagaDbgMode = 0;
            return;
        }
        if (!(repeat & 0x2000))
            return;
        m_file = NULL;
        m_cursor = 4;
        m_state = 0;
        m_top = 0;
        return;

    case 5:     /* items: select gives or takes the item */
        moveCursor(m_file->m_count, 17);
        for (int i = 0; i < 17 && m_top + i < m_file->m_count; i++) {
            g_gamMan.hasItem((short)m_entries[m_top + i].m_value, 0);
            sprintf(buf, "%05d", m_entries[m_top + i].m_value);
        }
        dispCursor(0x3c);
        dispScroll(m_top, 17, m_file->m_count);
        if (repeat & 0x4000) {
            short item = (short)m_entries[m_cursor].m_value;
            if (g_gamMan.hasItem(item, 0) == 0)
                g_gamMan.addItem(item, 0);
            else
                g_gamMan.delItem(item, 0);
        }
        if (!(repeat & 0x2000))
            return;
        m_state = 0;
        m_cursor = 2;
        break;

    case 6: {   /* switches */
        ObjStat *pc = g_gamMan.m_pcPtr;
        bool pcFlag = pc != NULL && (pc->m_flags & 0x400) != 0;
        u_int dogOid = g_gamMan.m_dogPtr != NULL ? g_gamMan.m_dogPtr->m_oid : 0;
        int dogFlag = g_gamMan.testObjFlags(dogOid, 0x400);
        int *output;
        g_gamMan.getStatus(0x15, &output);      // &m_optOutput; [1] is the vibration option
        moveCursor(5, 17);
        dispCursor(0x3c);
        if (repeat & 0x4000) {
            switch (m_cursor) {
            case 0:
                if (pc != NULL) {
                    if (pcFlag)
                        pc->m_flags &= ~0x400u;
                    else
                        pc->m_flags |= 0x400;
                }
                return;
            case 1:
                dogOid = g_gamMan.m_dogPtr != NULL ? g_gamMan.m_dogPtr->m_oid : 0;
                if (dogFlag == 0)
                    g_gamMan.setObj(dogOid, 0x400);
                else
                    g_gamMan.resetObj(dogOid, 0x400);
                return;
            case 2:
                s_nagaDbgSceneSwitch = 1 - s_nagaDbgSceneSwitch;
                g_magSystem.m_scene.m_22b8 = s_nagaDbgSceneSwitch == 0;
                return;
            case 3:
                s_nagaDbgFixView = 1 - s_nagaDbgFixView;
                return;
            case 4:
                g_gamMan.setOption(7, output[1] == 0 ? 1 : 0);
                return;
            default:
                return;
            }
        }
        if (!(repeat & 0x2000))
            return;
        m_file = NULL;
        m_cursor = 3;
        m_state = 0;
        m_top = 0;
        return;
    }

    case 7:
        if (repeat & 0x4000)
            m_state = 0;
        return;

    default:
        return;
    }
    /* Back from a list page: free its file. */
    m_top = 0;
    utlMemFree(m_file);
    m_file = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00217bec
// callcheck: dpadd, dpcmp, dpsub, dptofp, fptodp (the EE soft double: the double compares in nagaDbgWrapRad)

void EventTestTask::dispCursor(int x)
{
    (void)x;
    (void)sinf(nagaDbgWrapRad((float)m_frame * 0.25f));
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00217cac

void EventTestTask::dispScroll(int top, int page, int count)
{
    (void)top; (void)page; (void)count;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00217cb4

bool EventTestTask::isUp()
{
    return m_hold_up > 8 || (g_magSystem.m_pad[1].m_trig & 0x10) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00217cf0

bool EventTestTask::isDown()
{
    return m_hold_down > 8 || (g_magSystem.m_pad[1].m_trig & 0x40) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00217d2c

bool EventTestTask::isLeft()
{
    return m_hold_left > 8 || (g_magSystem.m_pad[1].m_trig & 0x80) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00217d68

bool EventTestTask::isRight()
{
    return m_hold_right > 8 || (g_magSystem.m_pad[1].m_trig & 0x20) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00217da4

bool EventTestTask::moveCursor(int count, int page)
{
    int old = m_cursor;

    if (isUp())
        m_cursor--;
    if (isDown())
        m_cursor++;
    if (page != 0) {
        if (isLeft()) {
            m_cursor -= page;
            m_top -= page;
        }
        if (isRight()) {
            m_cursor += page;
            m_top += page;
        }
    }
    if (m_cursor < 0)
        m_cursor = 0;
    else if (m_cursor > count - 1)
        m_cursor = count - 1;

    int top = 0;
    if (page < count) {
        if (m_top < 0)
            top = 0;
        else if (m_top > count - page)
            top = count - page;
        else
            top = m_top;
    }
    m_top = top;
    if (m_cursor < top)
        m_top = m_cursor;
    else if (top + page < m_cursor + 1)
        m_top = m_cursor - page + 1;
    return old != m_cursor;
}
