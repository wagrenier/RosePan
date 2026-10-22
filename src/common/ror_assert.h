/* ==========================================================================
 *  common/ror_assert.h
 *
 *  RoR's assert. The ROM compiles each one to
 *      if (!(expr)) assert_handler(__FILE__, __LINE__, "expr");
 *  with the handler at 0x215a30, which prints
 *      "Assertion Failed: %s(%d)   %s\n"
 *  and then loops for ever. PORT: the host reports the same line and
 *  carries on -- a hung game helps nobody on PC -- so code after a failed
 *  assert runs; guard what would crash there with a PC: comment.
 *
 *  Write the ROM's expression text back verbatim (__null as NULL): it names
 *  the members the code uses.
 * ======================================================================== */
#ifndef _COMMON_ROR_ASSERT_H
#define _COMMON_ROR_ASSERT_H

#include <stdio.h>

#define ROR_ASSERT(expr)                                                         \
    do {                                                                         \
        if (!(expr))                                                             \
            printf("Assertion Failed: %s(%d)   %s\n", __FILE__, __LINE__, #expr); \
    } while (0)

#endif /* _COMMON_ROR_ASSERT_H */
