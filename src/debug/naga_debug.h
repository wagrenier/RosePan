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

#include "task/tsk_man.h"

/* Reads the whole file theName into a new debug block of the game heap
 * (allocDebug, flags 2) and returns the block. */
void *nagaDbgLoadFile(const char *theName);                              // ROR 0x00216768

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

    /* The overlay pages and, in mode 3, the debug menu. */
    int  disp();                                                         // ROR 0x00216ad0

    int   m_frame;              // ROR 0x04: exec count
    char *m_file;               // ROR 0x08: the menu's loaded BIN file (its word +0x08 is the entry count)
    char *m_entries;            // ROR 0x0c: m_file + 0x40, 0x40-byte entries
    int   m_state;              // ROR 0x10: the menu's page (0 the file list)
    int   m_cursor;             // ROR 0x14: the menu's cursor
    int   m_top;                // ROR 0x18: the first entry shown
    int   m_hold_up;            // ROR 0x1c: frames pad 1 UP (0x10) has been held, 0 when released
    int   m_hold_down;          // ROR 0x20: DOWN (0x40)
    int   m_hold_left;          // ROR 0x24: LEFT (0x80)
    int   m_hold_right;         // ROR 0x28: RIGHT (0x20)
};

#endif /* _DEBUG_NAGA_DEBUG_H */
