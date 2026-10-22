/* ==========================================================================
 *  bed/bedsubmode.h
 *
 *  BedSubMode (bed/bedsubmode.cpp): a four-word mode record with a stack
 *  of MAX_STACK (4) saved copies. MagScene keeps its sub mode in one at
 *  +0x22d0 (update switches on m_mode, pushSubMode / popSubMode save and
 *  restore it around sub mode 8).
 * ======================================================================== */
#ifndef _BED_BEDSUBMODE_H
#define _BED_BEDSUBMODE_H

#include <stddef.h>

/* EE 0x54, plain EE layout (ints only). */
struct BedSubMode
{
    /* Saves the record (asserts m_depth < MAX_STACK). */
    void push();                                                         // ROR 0x00100258
    /* Restores the last saved record (asserts m_depth > 0). */
    void pop();                                                          // ROR 0x001002c0

    int m_mode;                 // ROR 0x00: the mode
    int m_step;                 // ROR 0x04: a step inside the mode; -1 on entry
    int m_arg;                  // ROR 0x08: the mode's argument
    int m_arg2;                 // ROR 0x0c: a second argument
    int m_stack[4][4];          // ROR 0x10: the saved records
    int m_depth;                // ROR 0x50
};
static_assert(offsetof(BedSubMode, m_arg2) == 0x0c, "m_arg2");
static_assert(offsetof(BedSubMode, m_depth) == 0x50, "m_depth");
static_assert(sizeof(BedSubMode) == 0x54, "BedSubMode is 0x54 bytes on the EE");

#endif /* _BED_BEDSUBMODE_H */
