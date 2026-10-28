/* ==========================================================================
 *  utl/utlfind.h
 *
 *  utl/utlfind.cpp: search helpers. Minimal: the 0x18-byte record
 *  utlFindClear resets (four ids, -1 = none, and two words).
 * ======================================================================== */
#ifndef _UTL_UTLFIND_H
#define _UTL_UTLFIND_H

#include <stddef.h>

struct FCHRstat;

/* EE 0x18. */
struct UtlFind
{
    int m_id[4];                // ROR 0x00: object uids (FCHRstat +0x128); -1 after utlFindClear
    int m_10;                   // ROR 0x10: how many of m_id count; 0 after utlFindClear
    int m_14;                   // ROR 0x14: 0 after utlFindClear
};
static_assert(offsetof(UtlFind, m_10) == 0x10, "m_10");
static_assert(offsetof(UtlFind, m_14) == 0x14, "m_14");
static_assert(sizeof(UtlFind) == 0x18, "UtlFind is 0x18 bytes on the EE");

void utlFindClear(UtlFind *p);                                           // ROR 0x0010620c
/* 1 when one of the first m_10 ids is positive. */
int  utlFindHasAny(UtlFind *p);                                          // ROR 0x00106240
/* m_14 = arg, m_10 = 0x103240(p, 4, item) (the ids found for item). */
void utlFindSetItem(UtlFind *p, int item, int arg);                      // ROR 0x00106280
/* An object's room number (FCHRstat::getParam 8), uid (u16 +0x128), id
 * (+0x100, what utlIsObj takes) and position; each asserts obj. */
int  utlFindObjWrdNo(FCHRstat *obj);                                     // ROR 0x001060e8
int  utlFindObjUid(FCHRstat *obj);                                       // ROR 0x00106138
unsigned int utlFindObjId(FCHRstat *obj);                                // ROR 0x00106178
void utlFindObjPos(FCHRstat *obj, float *out);                           // ROR 0x001061b8
/* Nonzero when obj is one of p's first m_10 ids. */
int  utlFindMatchObj(UtlFind *p, FCHRstat *obj);                         // ROR 0x001062b4
/* The object to fetch: of those whose uid is one of p's ids, the first in
 * room wrd, else the one with the best weight (+0x118) over its world
 * distance from pos / 100. NULL none. */
FCHRstat *utlFindBest(UtlFind *p, const float *pos, int wrd);            // ROR 0x00106344

#endif /* _UTL_UTLFIND_H */
