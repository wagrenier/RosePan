/* ==========================================================================
 *  utl/utlfind.h
 *
 *  utl/utlfind.cpp: search helpers. Minimal: the 0x18-byte record
 *  utlFindClear resets (four ids, -1 = none, and two words).
 * ======================================================================== */
#ifndef _UTL_UTLFIND_H
#define _UTL_UTLFIND_H

#include <stddef.h>

/* EE 0x18. Field roles unverified. */
struct UtlFind
{
    int m_id[4];                // ROR 0x00: -1 after utlFindClear
    int m_10;                   // ROR 0x10: 0 after utlFindClear
    int m_14;                   // ROR 0x14: 0 after utlFindClear
};
static_assert(offsetof(UtlFind, m_10) == 0x10, "m_10");
static_assert(offsetof(UtlFind, m_14) == 0x14, "m_14");
static_assert(sizeof(UtlFind) == 0x18, "UtlFind is 0x18 bytes on the EE");

void utlFindClear(UtlFind *p);                                           // ROR 0x0010620c

#endif /* _UTL_UTLFIND_H */
