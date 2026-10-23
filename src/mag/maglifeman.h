/* ==========================================================================
 *  mag/maglifeman.h
 *
 *  MagLifeMan (mag/maglifeman.cpp, 0x1194b4..0x119b34): the scene's list of
 *  life objects at +0x2330 -- a pool of MAX_LIFE (64) entries ("pods"),
 *  m_life_buf, doubly linked in priority order from m_top. MagScene's
 *  update runs the lives' slot 2 by level mask, its draw their slot 6 by
 *  priority group; MagScene::initRender registers the 2D layers with
 *  priorities 0x8000..0x800a.
 *
 *  m_top starts at m_life_buf[0] (init), which is still free: the first
 *  add takes entry 0 and finds itself there, so the list is never walked
 *  from a free pod once something is added. Walking an empty list reads
 *  entry 0's NULL life (the "plife" assert).
 * ======================================================================== */
#ifndef _MAG_MAGLIFEMAN_H
#define _MAG_MAGLIFEMAN_H

#include <stddef.h>

#include "sdk/eetypes.h"

struct MagLife;
struct I3dLayer;

#define MAX_LIFE 64

/* EE 0x10 (the stride of m_life_buf). layout: host -- pointers. */
struct MagLifeEntry
{
    MagLife      *m_life;       // ROR 0x00: NULL = a free entry
    int           m_priority;   // ROR 0x04: the list is sorted by it, low first; & 0xc000 is the draw group
    MagLifeEntry *m_prev;       // ROR 0x08: NULL at the head
    MagLifeEntry *m_next;       // ROR 0x0c: NULL at the tail
};
static_assert(offsetof(MagLifeEntry, m_priority) >= 0x04, "m_priority");
static_assert(offsetof(MagLifeEntry, m_prev) >= 0x08, "m_prev");
static_assert(offsetof(MagLifeEntry, m_next) >= 0x0c, "m_next");

/* EE 0x404. layout: host -- pointers. */
struct MagLifeMan
{
    /* Zeroes one pod. */
    void clearPod(MagLifeEntry *ppod);                                   // ROR 0x001194b4
    /* Takes ppod out of the list and clears it. */
    void unlink(MagLifeEntry *ppod);                                     // ROR 0x001194c8
    /* Links thePod2 in front of thePod1 (the same pod: makes it the head
     * when it has no prev). */
    void insertBefore(MagLifeEntry *thePod1, MagLifeEntry *thePod2);     // ROR 0x0011950c
    /* Links thePod2 after thePod1. */
    void insertAfter(MagLifeEntry *thePod1, MagLifeEntry *thePod2);      // ROR 0x001195ac
    void init();                                                         // ROR 0x00119630
    /* Deletes every life still held (slot 1, deleting), then init. */
    void deleteAll();                                                    // ROR 0x00119660
    /* Takes a free pod for life, linked before the first one of priority
     * >= priority; its index, -1 when the pool is full. */
    int  add(MagLife *life, int priority);                               // ROR 0x001196d4
    /* Unlinks life's entry; 1 when it was there. */
    int  remove(MagLife *life);                                          // ROR 0x00119810
    /* life's index in m_life_buf, -1 when absent. */
    int  find(MagLife *life);                                            // ROR 0x001198d0
    /* Unlinks entry theNo. */
    void removeNo(int theNo);                                            // ROR 0x00119908
    /* Runs slot 2 (live) of every life whose level word has a bit of mask. */
    void update(u_int mask);                                             // ROR 0x00119968
    /* Draws (slot 6) the shown lives whose priority & 0xc000 is group (0,
     * 0x4000, 0x8000) into layer. */
    void draw(I3dLayer *layer, u_int group);                             // ROR 0x00119a38

    MagLifeEntry  m_life_buf[MAX_LIFE]; // ROR 0x000
    MagLifeEntry *m_top;                // ROR 0x400: the first entry; init points it at m_life_buf
};
static_assert(offsetof(MagLifeMan, m_top) >= 0x400, "m_top");

#endif /* _MAG_MAGLIFEMAN_H */
