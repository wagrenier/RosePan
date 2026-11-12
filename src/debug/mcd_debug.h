/* ==========================================================================
 *  debug/mcd_debug.h
 *
 *  debug/mcd_debug.cpp (file name invented: no source string; pinned
 *  0x2155f4..0x215a30, between seqvm.cpp and mimu_debug.cpp): the second
 *  assert handler, which mnu / msg / snd asserts call, and a leftover
 *  memory-card menu test nothing calls -- it prints a "futuki" banner and
 *  steps a MnuMcd held in data 0x2d9e80 with pad 0. The file's static
 *  initializer constructs and destroys that MnuMcd.
 * ======================================================================== */
#ifndef _DEBUG_MCD_DEBUG_H
#define _DEBUG_MCD_DEBUG_H

class MnuMcd;

/* Prints "Assertion Failed: file(line)   expr" and loops for ever on the
 * ROM; the same code as dbgAssertFailed (0x215a30). The port's ROR_ASSERT
 * prints the same line inline (common/ror_assert.h), so no code calls it. */
void mcdDbgAssertFailed(const char *file, int line, const char *expr);  // ROR 0x002155f4
/* The memory-card menu test, one step per call; no callers. */
void mcdDbgMenuTest();                                                   // ROR 0x00215634

/* data 0x2d9e80: the menu the test drives. */
extern MnuMcd g_mcdDbgMenu;

#endif /* _DEBUG_MCD_DEBUG_H */
