/* ==========================================================================
 *  snd/unitque.cpp
 *
 *  The byte ring of snd/unitque.h: 0x1830fc..0x1832e8, pinned in the
 *  snd/rxmpeg.cpp session (unitQueCreate's assert names the file; its only
 *  user is the movie player).
 * ======================================================================== */
#include "snd/unitque.h"

#include <string.h>

#include "common/ror_assert.h"
#include "rx/rxsys.h"

// ROR 0x001830fc
// callcheck: mcdDbgAssertFailed (the second assert handler is ROR_ASSERT)
UnitQue *unitQueCreate(int size)
{
    UnitQue *pQue = (UnitQue *)((void *(*)(int))g_rxSysMemFunc[0])(size + 0x40);
    if (pQue != NULL) {
        pQue->m_size = size;
        unitQueReset(pQue);
    }
    ROR_ASSERT(pQue);
    return pQue;
}

// ROR 0x0018316c
void unitQueDelete(UnitQue *pQue)
{
    if (pQue != NULL)
        ((void (*)(void *))g_rxSysMemFunc[3])(pQue);
}

// ROR 0x00183194
void unitQueReset(UnitQue *pQue)
{
    pQue->m_user = 0;
    pQue->m_write = 0;
    pQue->m_read = 0;
    pQue->m_count = 0;
}

// ROR 0x001831a8
int unitQueGetReadable(UnitQue *pQue)
{
    int n = pQue->m_size - pQue->m_read;
    if (pQue->m_count < n)
        n = pQue->m_count;
    return n;
}

// ROR 0x001831c4
int unitQueGetCount(UnitQue *pQue)
{
    return pQue->m_count;
}

// ROR 0x001831cc
int unitQueGetFree(UnitQue *pQue)
{
    return pQue->m_size - unitQueGetCount(pQue);
}

// ROR 0x001831f8
void unitQueWrite(UnitQue *pQue, const void *data, int len)
{
    const unsigned char *src = (const unsigned char *)data;
    unsigned char *base = (unsigned char *)pQue + sizeof(UnitQue);
    while (len > 0) {
        int n = pQue->m_size - pQue->m_write;
        if (len < n)
            n = len;
        if (n < 0)
            n = 0;
        len -= n;
        memcpy(base + pQue->m_write, src, n);
        src += n;
        pQue->m_count += n;
        pQue->m_write = (pQue->m_write + n) % pQue->m_size;
    }
}

// ROR 0x001832a4
unsigned char *unitQueGetReadPtr(UnitQue *pQue)
{
    return (unsigned char *)pQue + sizeof(UnitQue) + pQue->m_read;
}

// ROR 0x001832b8
void unitQueConsume(UnitQue *pQue, int len)
{
    pQue->m_count -= len;
    pQue->m_read = (pQue->m_read + len) % pQue->m_size;
}
