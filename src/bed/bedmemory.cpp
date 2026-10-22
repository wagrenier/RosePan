/* ==========================================================================
 *  bed/bedmemory.cpp
 *
 *  BedMemory, RoR's block heap: create / format turn one buffer into a
 *  single free block; alloc finds a free block (first fit from the start,
 *  or from the end when (flags & 0x1c0) == 0x100), aligns its data to the
 *  mode's grain and cuts the rest off as a new free block; free marks the
 *  block free and merges it with free neighbours; realloc grows in place
 *  into a free next block, shrinks in place, or moves. Fresh buffers are
 *  filled with "$RUNTIME" and every allocation is cleared. See bedmemory.h.
 *
 *  PORT: the header is bigger on the host (pointers; BEDMEM_HEADER_SIZE
 *  stands for the EE's 0x20 everywhere), so the heap holds a little less.
 *  The split thresholds (0x40) stay the ROM's. Two ROM link slips in the
 *  realloc paths, which the ROM's own free would then catch with a hung
 *  assert, are mended (marked PC:). The assert handler reports and carries
 *  on (common/ror_assert.h); list walks that would then spin stop (PC:).
 * ======================================================================== */
#include "bed/bedmemory.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common/ror_assert.h"

/* EE 0x20: what a block's header takes in front of its data. */
#define BEDMEM_HEADER_SIZE ((int)sizeof(BEDMEMHEADER))

/* "$RUNTIME" in memory order: format's fill pattern. */
#define BEDMEM_FILL 0x454d49544e555224ULL

static BEDMEMHEADER *s_checkBlock;  /* bss 0x308244: the block check last visited */
static BEDMEMHEADER *s_infoBlock;   /* bss 0x308248: the block getInfo last visited */

/* The grain of an alignment mode (flags & 7; 7 = the heap's default).
 * Inlined into alignBlock and splitTail in the ROM. */
static int bedMemGrain(const BedMemory *heap, unsigned flags)
{
    unsigned mode = flags & 7;
    if (mode == 7) {
        mode = heap->m_id & 7;
    }
    switch (mode) {
    case 0:  return 0x20;
    case 1:  return 0x40;
    case 2:  return 0x80;
    default: return 4;
    }
}

/* Makes a header at h the start of a free block of chunk bytes. */
static void bedMemInitFree(BEDMEMHEADER *h, int chunk)
{
    h->m_chunk_size = chunk;
    h->m_line = -1;
    h->m_ptr = (unsigned char *)(h + 1);
    h->m_header_size = BEDMEM_HEADER_SIZE;
    h->m_flag = 0;
    h->m_filename = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Clears the record, mallocs size rounded down to 16 and formats it; the
// heap then owns the buffer.
// ROR 0x00100354

void BedMemory::create(int size, int id)
{
    m_last = NULL;
    m_id = id;
    m_buffer = NULL;
    m_size = 0;
    m_owner = 0;
    void *h = ::malloc((size >> 4) << 4);
    ROR_ASSERT(h != NULL);  /* 79 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    format(h, size, id);
    m_owner = 1;
    m_num_alloc = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Aligns theBuffer up to 16, fills it with "$RUNTIME" and makes it one free
// block led by the first header.
// ROR 0x001003f8

void BedMemory::format(void *theBuffer, int theSize, int id)
{
    ROR_ASSERT(theBuffer != NULL);  /* 97 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(theSize >= 256);     /* 98 */
    m_id = id;
    m_last = NULL;
    m_buffer = NULL;
    m_size = 0;
    m_owner = 0;
    m_raw_buffer = theBuffer;

    unsigned char *ptr = (unsigned char *)theBuffer;
    if (((uintptr_t)ptr & 15) != 0) {
        int pad = 16 - (int)((uintptr_t)ptr & 15);
        theSize -= pad;
        ptr += pad;
    }
    int size = (theSize >> 4) * 16;
    ROR_ASSERT((((intptr_t)ptr) & (15)) == 0);  /* 116 */

    uint64_t *fill = (uint64_t *)ptr;
    for (unsigned n = ((unsigned)theSize >> 4) * 2; n != 0; n--) {
        *fill++ = BEDMEM_FILL;
    }

    BEDMEMHEADER *h = (BEDMEMHEADER *)ptr;
    m_size = size;
    m_buffer = h;
    m_last = h;
    m_owner = 0;
    h->m_heap_id = (short)id;
    bedMemInitFree(h, size - BEDMEM_HEADER_SIZE);
    h->m_prev = NULL;
    h->m_next = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Frees the buffer when create malloc'd it and empties the record.
// ROR 0x00100568

void BedMemory::destroy()
{
    if (m_owner != 0) {
        ::free(m_raw_buffer);
    }
    m_owner = 0;
    m_last = NULL;
    m_buffer = NULL;
    m_size = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Moves a free block's data up to the grain of the align mode, keeping the
// distance back to the header in the word before the data. Returns the
// shift (0 when already aligned), or -1 when theSize no longer fits.
// ROR 0x001005ac

int BedMemory::alignBlock(BEDMEMHEADER *thePtr, int theSize, unsigned flags)
{
    ROR_ASSERT(thePtr != NULL);                   /* 169 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(((intptr_t)thePtr & 3) == 0);      /* 170 */
    ROR_ASSERT(theSize > 0);                      /* 172 */
    int gr = bedMemGrain(this, flags);

    unsigned mis = (unsigned)((uintptr_t)thePtr->m_ptr & (unsigned)(gr - 1));
    int shift = gr - (int)mis;
    if (mis == 0) {
        thePtr->m_header_size = BEDMEM_HEADER_SIZE;
        return 0;
    }
    if (theSize + shift > thePtr->m_chunk_size) {
        return -1;
    }
    unsigned char *newptr = thePtr->m_ptr + shift;
    ROR_ASSERT(((intptr_t)newptr & (gr - 1)) == 0);  /* 225 */
    thePtr->m_ptr = newptr;
    thePtr->m_header_size = shift + BEDMEM_HEADER_SIZE;
    ((int *)newptr)[-1] = shift + BEDMEM_HEADER_SIZE;
    thePtr->m_chunk_size -= shift;
    return shift;
}

// ──────────────────────────────────────────────────────────────────────
// For an allocation from the end: cuts an aligned block of theSize off the
// end of free block thePtr (linked after it) and returns it; when the rest
// would be too small, aligns thePtr itself and returns it.
// ROR 0x00100714

BEDMEMHEADER *BedMemory::splitTail(BEDMEMHEADER *thePtr, int theSize, unsigned flags)
{
    ROR_ASSERT(thePtr != NULL);                   /* 247 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(((intptr_t)thePtr & 3) == 0);      /* 248 */
    ROR_ASSERT(theSize > 0);                      /* 250 */
    int gr = bedMemGrain(this, flags);

    int rest = thePtr->m_chunk_size - theSize;
    int mis = (int)((uintptr_t)(thePtr->m_ptr + rest) & (unsigned)(gr - 1));
    if (mis < rest && rest - mis > 0x3f) {
        int front = rest - mis - BEDMEM_HEADER_SIZE;
        BEDMEMHEADER *header = (BEDMEMHEADER *)(thePtr->m_ptr + front);
        ROR_ASSERT((((intptr_t)(header + 1)) & (gr - 1)) == 0);  /* 313 */
        BEDMEMHEADER *next = thePtr->m_next;
        thePtr->m_chunk_size = front;
        thePtr->m_next = header;
        header->m_prev = thePtr;
        bedMemInitFree(header, theSize + mis);
        header->m_next = next;
        if (next != NULL) {
            next->m_prev = header;
        }
        return header;
    }

    /* The ROM applies alignBlock's shift a second time here, -1 included
     * (it then hands back a block one byte off): kept as written. Only
     * allocations from the end (flags & 0x1c0 == 0x100) come this way. */
    int shift = alignBlock(thePtr, theSize, flags);
    if (shift != 0) {
        unsigned char *ptr = thePtr->m_ptr;
        thePtr->m_header_size = shift + BEDMEM_HEADER_SIZE;
        thePtr->m_ptr = ptr + shift;
        ((int *)(ptr + shift))[-1] = shift + BEDMEM_HEADER_SIZE;
        thePtr->m_chunk_size -= shift;
    }
    return thePtr;
}

// ──────────────────────────────────────────────────────────────────────
// First fit from the start: the first free block of at least theSize that
// alignBlock can align.
// ROR 0x001008f0

BEDMEMHEADER *BedMemory::findFree(int theSize, unsigned flags)
{
    for (BEDMEMHEADER *p = m_buffer; p != NULL; p = p->m_next) {
        if ((p->m_flag & 1) == 0 && p->m_chunk_size >= theSize
            && alignBlock(p, theSize, flags) >= 0) {
            return p;
        }
    }
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// First fit from the end: walks back from the last block to the first free
// one splitTail can use.
// ROR 0x00100988

BEDMEMHEADER *BedMemory::findFreeFromEnd(int theSize, unsigned flags)
{
    BEDMEMHEADER *p = m_buffer;
    if (p == NULL) {
        return NULL;
    }
    while (p->m_next != NULL) {
        p = p->m_next;
    }
    for (; p != NULL; p = p->m_prev) {
        if ((p->m_flag & 1) == 0 && p->m_chunk_size >= theSize) {
            BEDMEMHEADER *h = splitTail(p, theSize, flags);
            if (h != NULL) {
                return h;
            }
        }
    }
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Allocates theSize bytes (rounded up to 16), cleared. A block found from
// the start gives its rest back as a free block when 0x40 bytes or more.
// ROR 0x00100a3c

void *BedMemory::alloc(int theSize, int flags)
{
    ROR_ASSERT(m_size > 0);         /* 406 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(m_buffer != NULL);   /* 407: the ROM's text is m_buffer > 0 */
    ROR_ASSERT(theSize > 0);        /* 411 */

    int size16 = ((theSize + 15) >> 4) * 16;
    bool fromStart = ((unsigned)flags & 0x1c0) != 0x100;
    BEDMEMHEADER *p = fromStart ? findFree(size16, (unsigned)flags)
                                : findFreeFromEnd(size16, (unsigned)flags);
    if (p == NULL) {
        check(0);
        ROR_ASSERT(0);              /* 436: out of memory */
        return NULL;
    }

    p->m_flag = 1;
    if (fromStart) {
        int rest = p->m_chunk_size - size16;
        if (rest >= 0x40) {
            BEDMEMHEADER *p2 = (BEDMEMHEADER *)(p->m_ptr + size16);
            p->m_chunk_size = size16;
            bedMemInitFree(p2, rest - BEDMEM_HEADER_SIZE);
            p2->m_prev = p;
            p2->m_next = p->m_next;
            if (p2->m_next != NULL) {
                p2->m_next->m_prev = p2;
            }
            p->m_next = p2;
            ROR_ASSERT(p2->m_next != p2);   /* 487 */
            ROR_ASSERT(p->m_next != p);     /* 488 */
        }
    } else {
        ROR_ASSERT(p->m_chunk_size >= theSize);  /* 454 */
        p->m_header_size = BEDMEM_HEADER_SIZE;
        ROR_ASSERT(p->m_next != p);              /* 456 */
    }

    memset(p->m_ptr, 0, p->m_chunk_size);
    m_num_alloc++;
    return p->m_ptr;
}

// ──────────────────────────────────────────────────────────────────────
// alloc, then tags the block's header with the caller's file, line and id.
// ROR 0x00100c84

void *BedMemory::allocDebug(int theSize, const char *filename, int line, int id, int flags)
{
    unsigned char *ptr = (unsigned char *)alloc(theSize, flags);
    if (ptr != NULL) {
        BEDMEMHEADER *h = (BEDMEMHEADER *)(ptr - ((int *)ptr)[-1]);
        h->m_id = (short)id;
        h->m_line = (short)line;
        h->m_filename = filename;
    }
    return ptr;
}

// ──────────────────────────────────────────────────────────────────────
// The header of an allocation: the word before the data is the distance
// back, and the header must agree.
// ROR 0x00100ce4

BEDMEMHEADER *BedMemory::getHeader(void *thePtr)
{
    int headersize = ((int *)thePtr)[-1];
    BEDMEMHEADER *h = (BEDMEMHEADER *)((unsigned char *)thePtr - headersize);
    ROR_ASSERT(h->m_header_size == headersize);  /* 534 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    return h;
}

// ──────────────────────────────────────────────────────────────────────
// realloc to a smaller size: when 0x40 bytes or more come free, they become
// a free block after this one (merged with a free next block).
// ROR 0x00100d30

void *BedMemory::shrink(void *thePtr, int theNewSize)
{
    ROR_ASSERT(thePtr != NULL);   /* 549 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(theNewSize > 0);   /* 550 */
    BEDMEMHEADER *h = getHeader(thePtr);
    ROR_ASSERT((((intptr_t)thePtr) % 4) == 0);  /* 556 */

    int size16 = ((theNewSize + 15) >> 4) * 16;
    int rest = h->m_chunk_size - size16;
    if (rest > 0x3f) {
        BEDMEMHEADER *next = h->m_next;
        BEDMEMHEADER *nh = (BEDMEMHEADER *)(h->m_ptr + size16);
        h->m_chunk_size = size16;
        h->m_next = nh;
        bedMemInitFree(nh, rest - BEDMEM_HEADER_SIZE);
        nh->m_prev = h;
        nh->m_id = 0;
        if (next == NULL) {
            nh->m_next = NULL;
        } else if (next->m_flag == 0) {
            nh->m_chunk_size += next->m_chunk_size + next->m_header_size;
            nh->m_next = next->m_next;
            if (nh->m_next != NULL) {
                nh->m_next->m_prev = nh;
            }
        } else {
            nh->m_next = next;
            next->m_prev = nh;  /* PC: the ROM leaves next->m_prev at h; its free would hang on prev->m_next == h */
        }
    }
    return thePtr;
}

// ──────────────────────────────────────────────────────────────────────
// Moves an allocation's header up to just before theNewPtr and frees the
// front as its own block (merged with a free previous block). No callers.
// ROR 0x00100ecc

void *BedMemory::trimFront(void *theOldPtr, void *theNewPtr)
{
    ROR_ASSERT(theOldPtr);                  /* 640 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(theNewPtr);                  /* 641 */
    ROR_ASSERT(theNewPtr > theOldPtr);      /* 642 */
    BEDMEMHEADER *h = getHeader(theOldPtr);
    ROR_ASSERT((((intptr_t)theNewPtr) % 4) == 0);  /* 648 */

    /* The ROM computes size - new - old (not size - (new - old)): kept; it
     * is dead code, and on the host the pointer words are truncated. */
    int size = h->m_chunk_size;
    int newsize = size - (int)(intptr_t)theNewPtr - (int)(intptr_t)theOldPtr;
    ROR_ASSERT(newsize > 0);                /* 652 */
    if (size - newsize < 0x40) {
        return theOldPtr;
    }

    BEDMEMHEADER *nh = (BEDMEMHEADER *)((unsigned char *)theNewPtr - BEDMEM_HEADER_SIZE);
    *nh = *h;
    nh->m_chunk_size = newsize;
    nh->m_ptr = (unsigned char *)theNewPtr;
    nh->m_header_size = BEDMEM_HEADER_SIZE;
    nh->m_prev = h;

    BEDMEMHEADER *prev = h->m_prev;
    h->m_chunk_size = (int)((unsigned char *)nh - BEDMEM_HEADER_SIZE - (unsigned char *)h);
    h->m_ptr = (unsigned char *)(h + 1);
    h->m_line = -1;
    h->m_header_size = BEDMEM_HEADER_SIZE;
    h->m_next = nh;
    h->m_heap_id = 0;
    h->m_flag = 0;
    h->m_filename = NULL;
    if (prev != NULL && prev->m_flag == 0) {
        prev->m_next = nh;
        prev->m_chunk_size += h->m_chunk_size + h->m_header_size;
        h->m_next->m_prev = prev;
    }
    return theNewPtr;
}

// ──────────────────────────────────────────────────────────────────────
// NULL allocates, a size below 1 frees; a smaller size shrinks in place, a
// larger one grows into a free next block when it fits, else moves.
// ROR 0x001010a4

void *BedMemory::realloc(void *thePtr, int theNewSize)
{
    if (thePtr == NULL) {
        if (theNewSize > 0) {
            return alloc(theNewSize, -1);
        }
        ROR_ASSERT(0);  /* 720 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        return NULL;
    }
    if (theNewSize < 1) {
        free(thePtr);
        return NULL;
    }

    BEDMEMHEADER *h = getHeader(thePtr);
    ROR_ASSERT((((intptr_t)thePtr) % 4) == 0);  /* 741 */
    int oldsize = h->m_chunk_size;
    int newsize = ((theNewSize + 15) >> 4) * 16;
    if (newsize == oldsize) {
        return thePtr;
    }
    if (newsize < oldsize) {
        return shrink(thePtr, theNewSize);
    }

    BEDMEMHEADER *next = h->m_next;
    if (next == NULL || next->m_flag != 0
        || newsize >= oldsize + next->m_chunk_size + next->m_header_size) {
        void *ptr = alloc(theNewSize, -1);
        if (ptr != NULL) {
            memcpy(ptr, thePtr, oldsize);
            free(thePtr);
        }
        return ptr;
    }

    int ns = next->m_chunk_size + next->m_header_size;
    ROR_ASSERT(newsize < oldsize + ns);  /* 785 */
    BEDMEMHEADER *after = next->m_next;
    int rest = oldsize + ns - newsize;
    if (rest < 0x40) {
        h->m_chunk_size += ns;
        h->m_next = after;
        if (after != NULL) {
            after->m_prev = h;
        }
    } else {
        BEDMEMHEADER *nh = (BEDMEMHEADER *)(h->m_ptr + newsize);
        h->m_chunk_size = newsize;
        bedMemInitFree(nh, rest - BEDMEM_HEADER_SIZE);
        nh->m_prev = h;
        nh->m_id = 0;
        nh->m_next = after;
        if (after != NULL) {
            after->m_prev = nh;
        }
        h->m_next = nh;  /* PC: the ROM leaves h->m_next at the swallowed header */
    }
    return thePtr;
}

// ──────────────────────────────────────────────────────────────────────
// Frees an allocation and merges it with a free next and previous block.
// ROR 0x00101328

void BedMemory::free(void *thePtr)
{
    ROR_ASSERT(thePtr != NULL);  /* 825 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    BEDMEMHEADER *h = getHeader(thePtr);
    int total = h->m_header_size;
    ROR_ASSERT(h->m_flag == 1);  /* 848 */

    BEDMEMHEADER *next = h->m_next;
    h->m_line = -1;
    total += h->m_chunk_size;
    h->m_flag = 0;
    h->m_filename = NULL;
    h->m_id = 0;
    if (next != NULL) {
        ROR_ASSERT(next != next->m_next);  /* 865 */
        if (next->m_flag == 0) {
            total += next->m_header_size + next->m_chunk_size;
            next = next->m_next;
        }
    }
    h->m_header_size = BEDMEM_HEADER_SIZE;
    h->m_chunk_size = total - BEDMEM_HEADER_SIZE;
    h->m_ptr = (unsigned char *)(h + 1);
    h->m_next = next;
    if (next != NULL) {
        next->m_prev = h;
        ROR_ASSERT(((intptr_t)next) == ((intptr_t)h) + total);  /* 891 */
    }

    BEDMEMHEADER *prev = h->m_prev;
    if (prev != NULL) {
        if (prev->m_next != h) {
            check(0);
            ROR_ASSERT(prev->m_next == h);  /* 905 */
        }
        if (prev->m_flag == 0) {
            int prevHeader = prev->m_header_size;
            prev->m_header_size = BEDMEM_HEADER_SIZE;
            prev->m_ptr = (unsigned char *)(prev + 1);
            prev->m_next = h->m_next;
            prev->m_chunk_size = total + prevHeader + prev->m_chunk_size - BEDMEM_HEADER_SIZE;
            if (prev->m_next != NULL) {
                prev->m_next->m_prev = prev;
            }
            ROR_ASSERT(prev != prev->m_next);  /* 921 */
        }
    }
    m_num_alloc--;
}

// ──────────────────────────────────────────────────────────────────────
// Walks the blocks: mode 1 looks at free blocks only, mode 2 also checks
// both links of each; asserts on a self-linked block and on link errors.
// ROR 0x00101518

void BedMemory::check(int mode)
{
    int errcnt = 0;
    for (BEDMEMHEADER *p = m_buffer; p != NULL; p = p->m_next) {
        if (mode != 1 || p->m_flag == 0) {
            if (mode == 2) {
                if (p->m_prev != NULL && p->m_prev->m_next != p) {
                    errcnt++;
                }
                if (p->m_next != NULL && p->m_next->m_prev != p) {
                    errcnt++;
                }
            }
            ROR_ASSERT(p != p->m_next);  /* 1032 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
            if (p == p->m_next) {
                break;  /* PC: the ROM's handler hangs here; do not spin */
            }
        }
        s_checkBlock = p;
    }
    ROR_ASSERT(errcnt == 0);  /* 1035 */
}

// ──────────────────────────────────────────────────────────────────────
// The largest free chunk, the free total and the heap size, into info
// (when given); asserts on a misaligned header or a bad m_flag.
// ROR 0x001015fc

void BedMemory::getInfo(BEDMEMINFO *info)
{
    int maxfree = 0;
    int total = 0;
    for (BEDMEMHEADER *p = m_buffer; p != NULL; p = p->m_next) {
        if (((intptr_t)p & 3) != 0) {
            check(0);
            ROR_ASSERT(0);  /* 1061 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
        }
        if (p->m_flag >= 2) {
            check(0);
            ROR_ASSERT(0);  /* 1069 */
        }
        if (p->m_flag == 0) {
            total += p->m_chunk_size;
            if (maxfree < p->m_chunk_size) {
                maxfree = p->m_chunk_size;
            }
        }
        ROR_ASSERT(p != p->m_next);  /* 1081 */
        if (p == p->m_next) {
            break;  /* PC: the ROM's handler hangs here; do not spin */
        }
        s_infoBlock = p;
    }
    if (info != NULL) {
        info->m_total_free = total;
        info->m_size = m_size;
        info->m_max_free = maxfree;
    }
}
