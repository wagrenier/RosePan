/* ==========================================================================
 *  utl/utlfind.h
 *
 *  utl/utlfind.cpp (ROR 0x1060e8..0x106658): the dog's item search. A
 *  UtlFind holds the uids (system data ids, ObjStat::m_kind) of the item
 *  Brown is told to fetch and its related items (utlGetItemUids);
 *  utlFindBest picks the object of those uids Brown smells best from a
 *  world position. The utlFindObj* accessors read a found object's
 *  room, uid, oid and position (each asserts "theHandle != 0").
 * ======================================================================== */
#ifndef _UTL_UTLFIND_H
#define _UTL_UTLFIND_H

#include <stddef.h>

struct FCHRstat;

/* EE 0x18. */
struct UtlFind
{
    int m_id[4];                // ROR 0x00: object uids (FCHRstat +0x128); -1 after utlFindClear
    int m_num;                  // ROR 0x10: how many of m_id count; 0 after utlFindClear
    int m_again;                // ROR 0x14: utlFindSetItem's arg; 0 divides a flag-2 item's score by 10
};
static_assert(offsetof(UtlFind, m_num) == 0x10, "m_num");
static_assert(offsetof(UtlFind, m_again) == 0x14, "m_again");
static_assert(sizeof(UtlFind) == 0x18, "UtlFind is 0x18 bytes on the EE");

/* An object's room number (FCHRstat::getParam 8), uid (u16 +0x128), id
 * (+0x100, what utlIsObj takes) and position; each asserts obj. */
int  utlFindObjWrdNo(FCHRstat *theHandle);                               // ROR 0x001060e8
int  utlFindObjUid(FCHRstat *theHandle);                                 // ROR 0x00106138
unsigned int utlFindObjId(FCHRstat *theHandle);                          // ROR 0x00106178
void utlFindObjPos(FCHRstat *theHandle, float *out);                     // ROR 0x001061b8
void utlFindClear(UtlFind *p);                                           // ROR 0x0010620c
/* 1 when one of the first m_num ids is positive. */
int  utlFindHasAny(UtlFind *p);                                          // ROR 0x00106240
/* m_again = arg, m_num = utlGetItemUids(p->m_id, 4, item). */
void utlFindSetItem(UtlFind *p, int item, int arg);                      // ROR 0x00106280
/* Nonzero when utlFindItemByUids finds an object of p's ids (it writes
 * that object's position over obj: see the .cpp). No callers reach it. */
int  utlFindMatchObj(UtlFind *p, FCHRstat *obj);                         // ROR 0x001062b4
/* The first of p's ids with an object in GamMan::m_vfchrList, NULL none. */
FCHRstat *utlFindFirstVFchr(UtlFind *p);                                 // ROR 0x001062d8
/* The object to fetch: of m_vfchrList's objects whose uid is one of p's
 * ids, those in room wrd when any is, scored smell range / (world
 * distance from pos / 100, height x10) and the best scoring >= 1 taken.
 * NULL pos: utlFindFirstVFchr. NULL none. */
FCHRstat *utlFindBest(UtlFind *p, const float *pos, int wrd);            // ROR 0x00106344

#endif /* _UTL_UTLFIND_H */
