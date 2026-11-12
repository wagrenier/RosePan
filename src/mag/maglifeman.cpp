/* ==========================================================================
 *  mag/maglifeman.cpp
 *
 *  The scene's life manager (RoR's "mag/maglifeman.cpp", pinned
 *  0x1194b4..0x119b34): a pool of MAX_LIFE pods linked in priority order;
 *  add / remove / find register the lives, update runs their slot 2 and
 *  draw their slot 6 through MagLife's virtuals (mag/maglife.h).
 * ======================================================================== */
#include "mag/maglifeman.h"

#include <string.h>

#include "common/ror_assert.h"
#include "mag/maglife.h"

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001194b4

void MagLifeMan::clearPod(MagLifeEntry *ppod)
{
    ppod->m_priority = 0;
    ppod->m_life = NULL;
    ppod->m_prev = NULL;
    ppod->m_next = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001194c8

void MagLifeMan::unlink(MagLifeEntry *ppod)
{
    MagLifeEntry *prev = ppod->m_prev;
    MagLifeEntry *next = ppod->m_next;

    if (prev == NULL)
        m_top = next;
    else
        prev->m_next = next;
    if (next != NULL)
        next->m_prev = prev;
    clearPod(ppod);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011950c // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeMan::insertBefore(MagLifeEntry *thePod1, MagLifeEntry *thePod2)
{
    ROR_ASSERT(thePod1);
    ROR_ASSERT(thePod2);

    MagLifeEntry *prev = thePod1->m_prev;

    if (thePod1 == thePod2) {
        if (prev == NULL)
            m_top = thePod1;
        return;
    }
    if (prev == NULL)
        m_top = thePod2;
    else
        prev->m_next = thePod2;
    thePod2->m_prev = prev;
    thePod2->m_next = thePod1;
    thePod1->m_prev = thePod2;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001195ac // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeMan::insertAfter(MagLifeEntry *thePod1, MagLifeEntry *thePod2)
{
    if (thePod1 == thePod2)
        return;

    ROR_ASSERT(thePod1);
    ROR_ASSERT(thePod2);

    MagLifeEntry *next = thePod1->m_next;

    thePod1->m_next = thePod2;
    thePod2->m_prev = thePod1;
    thePod2->m_next = next;
    if (next != NULL)
        next->m_prev = thePod2;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00119630

void MagLifeMan::init()
{
    memset(m_life_buf, 0, sizeof(m_life_buf));
    m_top = &m_life_buf[0];
}

// ──────────────────────────────────────────────────────────────────────
// Every life still in the pool is deleted (slot 1 with flags 3), linked
// or not. The scene removes its member lives first (0x131f74 removes the
// fader just before calling this).
// ROR 0x00119660

void MagLifeMan::deleteAll()
{
    for (int i = 0; i < MAX_LIFE; i++) {
        if (m_life_buf[i].m_life != NULL) {
            delete m_life_buf[i].m_life;
            m_life_buf[i].m_life = NULL;
        }
    }
    init();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001196d4 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

int MagLifeMan::add(MagLife *life, int priority)
{
    int no;

    for (no = 0; no < MAX_LIFE; no++)
        if (m_life_buf[no].m_life == NULL)
            break;
    if (no >= MAX_LIFE)
        return -1;

    MagLifeEntry *pod = &m_life_buf[no];
    pod->m_life = life;
    pod->m_priority = priority;
    pod->m_prev = NULL;
    pod->m_next = NULL;

    MagLifeEntry *last = NULL;
    for (MagLifeEntry *ppod = m_top; ppod != NULL; ppod = ppod->m_next) {
        ROR_ASSERT(ppod >= &m_life_buf[0] && ppod < &m_life_buf[MAX_LIFE]);
        if (priority <= ppod->m_priority) {
            insertBefore(ppod, pod);
            return no;
        }
        last = ppod;
    }
    if (last == NULL) {         // PC: an emptied list (m_top NULL) faults in the ROM's insertAfter
        m_top = pod;
        return no;
    }
    insertAfter(last, pod);
    return no;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00119810 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

int MagLifeMan::remove(MagLife *life)
{
    for (MagLifeEntry *ppod = m_top; ppod != NULL; ppod = ppod->m_next) {
        ROR_ASSERT(ppod >= &m_life_buf[0] && ppod < &m_life_buf[MAX_LIFE]);
        MagLife *plife = ppod->m_life;
        ROR_ASSERT(plife);
        if (plife == life) {
            unlink(ppod);
            return 1;
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001198d0

int MagLifeMan::find(MagLife *life)
{
    for (int i = 0; i < MAX_LIFE; i++)
        if (m_life_buf[i].m_life == life)
            return i;
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// No callers in the ROM.
// ROR 0x00119908 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeMan::removeNo(int theNo)
{
    ROR_ASSERT(theNo >= 0 && theNo < MAX_LIFE);
    if (theNo >= 0)
        unlink(&m_life_buf[theNo]);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00119968 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeMan::update(u_int mask)
{
    for (MagLifeEntry *ppod = m_top; ppod != NULL; ppod = ppod->m_next) {
        ROR_ASSERT(ppod >= &m_life_buf[0] && ppod < &m_life_buf[MAX_LIFE]);
        MagLife *plife = ppod->m_life;
        ROR_ASSERT(plife);
        if (plife == NULL)      // PC: the ROM reads address 0x10 and goes on
            continue;
        if ((plife->m_level & mask) != 0)
            plife->live();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00119a38 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeMan::draw(I3dLayer *layer, u_int group)
{
    for (MagLifeEntry *ppod = m_top; ppod != NULL; ppod = ppod->m_next) {
        ROR_ASSERT(ppod >= &m_life_buf[0] && ppod < &m_life_buf[MAX_LIFE]);
        if ((u_int)(ppod->m_priority & 0xc000) != group)
            continue;
        MagLife *plife = ppod->m_life;
        ROR_ASSERT(plife);
        if (plife == NULL)      // PC: the ROM calls through address 0x14
            continue;
        if (plife->getHide() == 0)
            plife->render(layer);
    }
}
