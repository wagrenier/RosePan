/* ==========================================================================
 *  mag/maglifeman.h
 *
 *  MagLifeMan (mag/maglifeman.cpp): the scene's list of life objects at
 *  +0x2330 -- a pool of MAX_LIFE (64) entries, m_life_buf, linked in
 *  priority order from the head pointer after it. MagScene::initRender
 *  registers the 2D layers with priorities 0x8000..0x800a.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEMAN_H
#define _MAG_MAGLIFEMAN_H

#include <stddef.h>

struct MagLife;

/* EE 0x10. layout: host -- pointers. */
struct MagLifeEntry
{
    MagLife      *m_life;       // ROR 0x00: NULL = a free entry
    int           m_priority;   // ROR 0x04: the list is sorted by it, low first
    int           m_08;         // ROR 0x08: 0 when added; role unknown
    MagLifeEntry *m_next;       // ROR 0x0c
};
static_assert(offsetof(MagLifeEntry, m_priority) >= 0x04, "m_priority");
static_assert(offsetof(MagLifeEntry, m_next) >= 0x0c, "m_next");

/* EE 0x404. layout: host -- pointers. */
struct MagLifeMan
{
    void init();                                                         // ROR 0x00119630
    int  add(MagLife *life, int priority);                               // ROR 0x001196d4

    MagLifeEntry  m_life_buf[64];   // ROR 0x000: MAX_LIFE entries
    MagLifeEntry *m_top;            // ROR 0x400: the first entry; init points it at m_life_buf
};
static_assert(offsetof(MagLifeMan, m_top) >= 0x400, "m_top");

#endif /* _MAG_MAGLIFEMAN_H */
