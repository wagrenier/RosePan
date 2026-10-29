/* ==========================================================================
 *  mnu/mnuselect.h
 *
 *  MnuTextList (mnu/mnuselect.cpp, 0x1a066c..0x1a1244): a list of up to 32
 *  text rows in one scene window with the message sprite 1 as its cursor.
 *  MnuTitle's movie viewer holds one (+0x4c68), TskFrameFunc kind 8 makes
 *  one with operator new (the script's choice list). Class name invented
 *  (ror-inferred); the file from the source path its allocator passes.
 *
 *  open allocates the row buffers, setItems fills them from "#0#0"-separated
 *  labels and a mask, processPad steps it once a frame (m_step: 0, 1 wait,
 *  2 select, 4 / 5 done -> returns 1), getSelect gives the chosen row (-1
 *  none yet, -3 cancelled), endSelect asks the window to close.
 * ======================================================================== */
#ifndef _MNU_MNUSELECT_H
#define _MNU_MNUSELECT_H

#include <stddef.h>

#include "sdk/eetypes.h"

struct BedPad;

/* setItems' mask: bits 0..31 the rows shown, and these. */
#define MNU_TEXT_LIST_HORIZONTAL    0x10000     /* rows side by side (left / right) */
#define MNU_TEXT_LIST_CANCEL        0x40000     /* circle cancels (getSelect -3) */

/* getSelect's results besides a row. */
#define MNU_TEXT_LIST_NONE          (-1)        /* nothing chosen yet */
#define MNU_TEXT_LIST_CANCELLED     (-3)
#define MNU_TEXT_LIST_CLOSED        (-2)        /* the list is not open */

/* EE 0x104 (the next MnuTitle word is +0x4d6c). layout: host -- m_text
 * (+0x84) is the first pointer; the offsets before it are exact. */
struct MnuTextList
{
    MnuTextList();                                                       // ROR 0x001a066c
    ~MnuTextList();                                                      // ROR 0x001a0684
    /* Non-zero once the list has closed (step's result). */
    int  processPad(BedPad *pad);                                        // ROR 0x001a06d4
    /* m_open 1, the defaults and the row buffers; -1 when already open. */
    int  open();                                                         // ROR 0x001a06e8
    /* The row buffers freed and m_open 0; -1 when not open. */
    int  close();                                                        // ROR 0x001a074c
    /* processPad's body: m_step. 0 while running, 1 once closed; -1 when
     * not open. */
    int  step(BedPad *pad);                                              // ROR 0x001a0788
    /* The words cleared and the defaults; returns 0. */
    int  clear();                                                        // ROR 0x001a0860
    void allocText();                                                    // ROR 0x001a08d4
    void freeText();                                                     // ROR 0x001a0950
    /* items: the labels, each ended by "#0#0", optionally led by "#<digit>"
     * (the row to start on); mask: the rows shown and the MNU_TEXT_LIST_*
     * flags. Ignored while a selection runs. */
    void setItems(const char *items, u_int mask);                        // ROR 0x001a09b8
    /* The chosen row (MNU_TEXT_LIST_NONE / _CANCELLED); _CLOSED when not
     * open. */
    int  getSelect();                                                    // ROR 0x001a0ba0
    /* Closes the window on the next step while a selection runs. */
    void endSelect();                                                    // ROR 0x001a0bb8
    /* An empty hook step calls after select; returns 0. */
    int  stepExtra(BedPad *pad);                                         // ROR 0x001a0bdc
    /* m_state: the window opened, the cursor moved, the rows drawn; 1 the
     * frame the window closes. */
    int  select(BedPad *pad);                                            // ROR 0x001a0be4

    int    m_open;              // ROR 0x00: open / close
    int    m_cur;               // ROR 0x04: the cursor (among the rows shown; horizontal: the row)
    int    m_08[6];             // ROR 0x08: cleared by clear (roles unknown)
    int    m_rows;              // ROR 0x20: the rows shown (setItems counts the mask's bits)
    int    m_24;                // ROR 0x24: clear sets 2 (unused here)
    int    m_28;                // ROR 0x28: clear sets 2 (unused here)
    u_char _2c[0x3c - 0x2c];
    int    m_step;              // ROR 0x3c: step's state
    int    m_pause;             // ROR 0x40: non-zero skips select (open clears; no writer seen)
    int    m_state;             // ROR 0x44: select's state: 0, 1 opening, 3 shown, 4 closing
    u_char _48[0x54 - 0x48];
    int    m_x;                 // ROR 0x54: window position (open 100, 100)
    int    m_y;                 // ROR 0x58
    int    m_z;                 // ROR 0x5c: window depth (open 0x7fff)
    int    m_window;            // ROR 0x60: MagScene window slot, -1 none
    int    m_select;            // ROR 0x64: getSelect's result
    int    m_horizontal;        // ROR 0x68: mask bit 16
    u_int  m_mask;              // ROR 0x6c: setItems' mask
    int    m_active;            // ROR 0x70: 1 from setItems until the window closes
    int    m_cancel;            // ROR 0x74: mask bit 18
    int    m_num;               // ROR 0x78: rows read by setItems
    int    m_line;              // ROR 0x7c: row height in pixels (0x12)
    int    m_80;                // ROR 0x80
    char  *m_text[32];          // ROR 0x84: 32-byte row buffers (allocText)
};
static_assert(offsetof(MnuTextList, m_cur) == 0x04, "m_cur");
static_assert(offsetof(MnuTextList, m_08) == 0x08, "m_08");
static_assert(offsetof(MnuTextList, m_rows) == 0x20, "m_rows");
static_assert(offsetof(MnuTextList, m_24) == 0x24, "m_24");
static_assert(offsetof(MnuTextList, m_28) == 0x28, "m_28");
static_assert(offsetof(MnuTextList, m_step) == 0x3c, "m_step");
static_assert(offsetof(MnuTextList, m_pause) == 0x40, "m_pause");
static_assert(offsetof(MnuTextList, m_state) == 0x44, "m_state");
static_assert(offsetof(MnuTextList, m_x) == 0x54, "m_x");
static_assert(offsetof(MnuTextList, m_y) == 0x58, "m_y");
static_assert(offsetof(MnuTextList, m_z) == 0x5c, "m_z");
static_assert(offsetof(MnuTextList, m_window) == 0x60, "m_window");
static_assert(offsetof(MnuTextList, m_select) == 0x64, "m_select");
static_assert(offsetof(MnuTextList, m_horizontal) == 0x68, "m_horizontal");
static_assert(offsetof(MnuTextList, m_mask) == 0x6c, "m_mask");
static_assert(offsetof(MnuTextList, m_active) == 0x70, "m_active");
static_assert(offsetof(MnuTextList, m_cancel) == 0x74, "m_cancel");
static_assert(offsetof(MnuTextList, m_num) == 0x78, "m_num");
static_assert(offsetof(MnuTextList, m_line) == 0x7c, "m_line");
static_assert(offsetof(MnuTextList, m_80) == 0x80, "m_80");
static_assert(offsetof(MnuTextList, m_text) >= 0x84, "m_text (host pointers)");

#endif /* _MNU_MNUSELECT_H */
