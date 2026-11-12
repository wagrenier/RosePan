/* ==========================================================================
 *  debug/naga_debug.h
 *
 *  debug/naga_debug.cpp (pinned 0x216768..0x217f10 and the linkonce
 *  symbols 0x2911cc..0x291228; RoR's source string anchors
 *  nagaDbgLoadFile's allocDebug): EventTestTask, a developer's event test
 *  overlay run as a task. Its exec step counts the second pad's d-pad hold
 *  frames, steps a display mode on pad 0 R3 (0, 1, 2) and pad 1 SELECT (3
 *  and back to 0), and calls disp, which prints world / area / position
 *  pages and in mode 3 runs a menu over the event flags, steps, items and
 *  areas listed in data/seq/w%03d/DBGFLAG.BIN, DBGSTEP.BIN, DBGAREA.BIN and
 *  data/seq/DBGITEM.BIN (read whole with nagaDbgLoadFile). Nothing in the
 *  retail ROM constructs the task (no reference to its constructor or
 *  vtable outside them).
 * ======================================================================== */
#ifndef _DEBUG_NAGA_DEBUG_H
#define _DEBUG_NAGA_DEBUG_H

#include <stddef.h>

#include "task/tsk_man.h"

/* Reads the whole file theName into a new debug block of the game heap
 * (allocDebug, flags 2) and returns the block. */
void *nagaDbgLoadFile(const char *theName);                              // ROR 0x00216768

/* The debug BIN files (DBGFLAG / DBGSTEP / DBGAREA / DBGITEM.BIN): a
 * 0x40-byte head whose word +0x08 is the entry count, then the entries.
 * File data, EE layout. */
struct NagaDbgBinHead
{
    int  m_00[2];               // ROR 0x00: not read
    int  m_count;               // ROR 0x08: entries after the head
    int  m_0c[13];              // ROR 0x0c: not read
};
static_assert(sizeof(NagaDbgBinHead) == 0x40, "NagaDbgBinHead size");
static_assert(offsetof(NagaDbgBinHead, m_count) == 0x08, "m_count");

/* One entry: its caption ("[Set Step] %s" prints it) and its value -- a
 * flag bit of the sequence, a step for global 0x27, a room for
 * requestStage, an item id. */
struct NagaDbgEntry
{
    char m_name[0x3c];          // ROR 0x00
    int  m_value;               // ROR 0x3c
};
static_assert(sizeof(NagaDbgEntry) == 0x40, "NagaDbgEntry size");
static_assert(offsetof(NagaDbgEntry, m_value) == 0x3c, "m_value");

/* EventTestTask : CmnITask (vtable 0x2e50c0; kind 2). EE 0x2c bytes (the
 * constructor clears up to +0x28; no allocation site gives the size).
 * layout: host -- vptr and pointers; EE offsets in the comments. */
struct EventTestTask : CmnITask
{
    EventTestTask();                                                     // ROR 0x00216828
    ~EventTestTask() override {}                                         // ROR 0x002168bc

    int  get_task_type() override { return 2; }                         // ROR 0x0029121c
    void init() override {}                                              // ROR 0x002168f0
    /* Counts the hold frames, steps the display mode, runs disp; never
     * ends (returns 0). */
    int  exec(int id, int time, int arg) override;                       // ROR 0x002168f8
    void term() override {}                                              // ROR 0x00291224

    /* The overlay pages and, in mode 3, the debug menu. Retail keeps the
     * text formatting and the reads; the draw calls are stripped. */
    void disp();                                                         // ROR 0x00216ad0
    /* The menu (pad 1: m_repeat 0x4000 select, 0x2000 back): page 0 the
     * list of pages, 1 event flags, 2 steps, 3 areas (rooms), 4 chapters,
     * 5 items, 6 switches, 7 an empty page. */
    void menu();                                                         // ROR 0x00217260
    /* The cursor's pulse (sinf of m_frame / 4); x is the column, unused
     * in retail. */
    void dispCursor(int x);                                              // ROR 0x00217bec
    /* The scroll bar; empty in retail. */
    void dispScroll(int top, int page, int count);                       // ROR 0x00217cac
    /* Up / down / left / right: held over 8 frames or pressed now. */
    bool isUp();                                                         // ROR 0x00217cb4
    bool isDown();                                                       // ROR 0x00217cf0
    bool isLeft();                                                       // ROR 0x00217d2c
    bool isRight();                                                      // ROR 0x00217d68
    /* Moves m_cursor over count entries (left / right by a page when page
     * is not 0), keeps m_top so the cursor shows; true when it moved. */
    bool moveCursor(int count, int page);                                // ROR 0x00217da4

    int   m_frame;              // ROR 0x04: exec count
    NagaDbgBinHead *m_file;     // ROR 0x08: the menu's loaded BIN file
    NagaDbgEntry   *m_entries;  // ROR 0x0c: m_file + 0x40
    int   m_state;              // ROR 0x10: the menu's page (0 the file list)
    int   m_cursor;             // ROR 0x14: the menu's cursor
    int   m_top;                // ROR 0x18: the first entry shown
    int   m_hold_up;            // ROR 0x1c: frames pad 1 UP (0x10) has been held, 0 when released
    int   m_hold_down;          // ROR 0x20: DOWN (0x40)
    int   m_hold_left;          // ROR 0x24: LEFT (0x80)
    int   m_hold_right;         // ROR 0x28: RIGHT (0x20)
};

#endif /* _DEBUG_NAGA_DEBUG_H */
