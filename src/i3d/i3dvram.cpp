/* ==========================================================================
 *  i3d/i3dvram.cpp
 *
 *  The i3d VRAM cache (ROR 0x227a98..0x228c08): a heap of GS memory units
 *  cut into blocks, each block a node on up to six intrusive circular lists
 *  headed by the heap's sentinel block. See i3dvram.h.
 *
 *  A frame reserves its textures / CLUTs (i3dVramReserve): blocks already
 *  held are locked and moved to the LRU front; handles without one get a
 *  block from the first run of unlocked blocks long enough (address order,
 *  starting at the LRU tail), or, when there is none, from evicting unlocked
 *  blocks oldest first. The context then compacts the heap (i3dVramCompact)
 *  and turns the new / moved lists into transfers; i3dVramEndFrame empties
 *  the per-frame lists.
 *
 *  Names are inferred from the code (no symbols, no asserts in this file).
 * ======================================================================== */
#include "i3d/i3dvram.h"

static int i3dVramCoalesce(I3dVramHeap *heap, I3dVramBlock *blk);

/* ---- lists ---------------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Takes a block off list `list`; it is left linked to itself.
// ROR 0x00227a98

static void i3dVramUnlink(I3dVramBlock *blk, int list)
{
    I3dVramLink *l = &blk->m_link[list];
    I3dVramBlock *prev = l->m_prev;
    I3dVramBlock *next;

    prev->m_link[list].m_next = l->m_next;
    next = l->m_next;
    l->m_next = blk;
    next->m_link[list].m_prev = prev;
    l->m_prev = blk;
}

// ──────────────────────────────────────────────────────────────────────
// Puts a block on list `list` just after `head`.
// ROR 0x00227ac8

static void i3dVramLinkAfter(I3dVramBlock *blk, int list, I3dVramBlock *head)
{
    I3dVramBlock *next = head->m_link[list].m_next;

    blk->m_link[list].m_next = next;
    next->m_link[list].m_prev = blk;
    head->m_link[list].m_next = blk;
    blk->m_link[list].m_prev = head;
}

// ──────────────────────────────────────────────────────────────────────
// Moves a block on list `list` to just after `head`.
// ROR 0x00227af0

static void i3dVramMoveAfter(I3dVramBlock *blk, int list, I3dVramBlock *head)
{
    i3dVramUnlink(blk, list);
    i3dVramLinkAfter(blk, list, head);
}

/* ---- blocks and handles ---------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Gives a block to a handle.
// ROR 0x00227b38

static void i3dVramBlockSetOwner(I3dVramBlock *blk, I3dVramHandle *owner)
{
    blk->m_owner = owner;
    blk->m_flags = (blk->m_flags & ~I3D_VRAM_FREE) | I3D_VRAM_OWNED;
    i3dVramHandleSetBlock(owner, blk);
}

// ──────────────────────────────────────────────────────────────────────
// Makes a block free again; its owner no longer holds it.
// ROR 0x00227b70

static void i3dVramBlockRelease(I3dVramBlock *blk)
{
    blk->m_flags = (blk->m_flags & ~I3D_VRAM_OWNED) | I3D_VRAM_FREE;
    if (blk->m_owner != NULL)
    {
        i3dVramHandleSetBlock(blk->m_owner, NULL);
        blk->m_owner = NULL;
    }
}

// ──────────────────────────────────────────────────────────────────────
// An empty free block on no list.
// ROR 0x00227bc0

static int i3dVramBlockInit(I3dVramBlock *blk)
{
    u_int i;

    for (i = 0; i < I3D_VRAM_LISTS; i++)
    {
        blk->m_link[i].m_next = blk;
        blk->m_link[i].m_prev = blk;
    }
    blk->m_owner    = NULL;
    blk->m_flags    = I3D_VRAM_FREE;
    blk->m_addr     = 0;
    blk->m_size     = 0;
    blk->m_old_addr = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00227c08

static int i3dVramBlockUnlinkAll(I3dVramBlock *blk)
{
    u_int i;

    for (i = 0; i < I3D_VRAM_LISTS; i++)
        i3dVramUnlink(blk, i);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00227c58

void i3dVramHandleSetBlock(I3dVramHandle *handle, I3dVramBlock *blk)
{
    handle->m_block = blk;
}

// ──────────────────────────────────────────────────────────────────────
// Gives the handle's block back to the heap (it stays in place, free).
// ROR 0x00227c60

int i3dVramHandleRelease(I3dVramHandle *handle)
{
    if (handle->m_block != NULL)
        i3dVramBlockRelease(handle->m_block);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00227c90

int i3dVramHandleInit(I3dVramHandle *handle)
{
    handle->m_stamp = 0;
    handle->m_image = NULL;
    handle->m_size  = 0;
    handle->m_block = NULL;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00227ca8 (no callers in the ROM)

int i3dVramHandleTerm(I3dVramHandle *handle)
{
    i3dVramHandleRelease(handle);
    return 0;
}

/* ---- the heap's blocks ------------------------------------------------------ */

// ──────────────────────────────────────────────────────────────────────
// A free block of `size` units from the node stack, on the address list
// after `after` and at the LRU tail. The ROM does not check for an empty
// stack.
// ROR 0x00227cc8

static I3dVramBlock *i3dVramNewBlock(I3dVramHeap *heap, u_int size, I3dVramBlock *after)
{
    I3dVramBlock *blk;

    heap->m_free_count--;
    blk = heap->m_free[heap->m_free_count];
    i3dVramBlockInit(blk);
    blk->m_size = size;
    i3dVramLinkAfter(blk, I3D_VRAM_LIST_ADDR, after);
    i3dVramLinkAfter(blk, I3D_VRAM_LIST_LRU, heap->m_sentinel.m_link[I3D_VRAM_LIST_LRU].m_prev);
    return blk;
}

// ──────────────────────────────────────────────────────────────────────
// Returns a block's node to the stack.
// ROR 0x00227d58

static int i3dVramFreeBlock(I3dVramHeap *heap, I3dVramBlock *blk)
{
    i3dVramBlockRelease(blk);
    i3dVramBlockUnlinkAll(blk);
    heap->m_free[heap->m_free_count] = blk;
    heap->m_free_count++;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00227db8

static void i3dVramAddEvict(I3dVramHeap *heap, I3dVramBlock *blk)
{
    blk->m_flags |= I3D_VRAM_EVICT;
    i3dVramLinkAfter(blk, I3D_VRAM_LIST_EVICT, &heap->m_sentinel);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00227de8

static void i3dVramLock(I3dVramHeap *heap, I3dVramBlock *blk)
{
    blk->m_flags |= I3D_VRAM_LOCKED;
    i3dVramLinkAfter(blk, I3D_VRAM_LIST_LOCK, &heap->m_sentinel);
    heap->m_locked += blk->m_size;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00227e40

static int i3dVramClearEvict(I3dVramHeap *heap)
{
    I3dVramBlock *blk = heap->m_sentinel.m_link[I3D_VRAM_LIST_EVICT].m_next;
    I3dVramBlock *next;

    while (blk != &heap->m_sentinel)
    {
        next = blk->m_link[I3D_VRAM_LIST_EVICT].m_next;
        blk->m_flags &= ~I3D_VRAM_EVICT;
        i3dVramUnlink(blk, I3D_VRAM_LIST_EVICT);
        blk = next;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00227ea8

static int i3dVramClearLock(I3dVramHeap *heap)
{
    I3dVramBlock *blk = heap->m_sentinel.m_link[I3D_VRAM_LIST_LOCK].m_next;
    I3dVramBlock *next;

    while (blk != &heap->m_sentinel)
    {
        next = blk->m_link[I3D_VRAM_LIST_LOCK].m_next;
        blk->m_flags &= ~I3D_VRAM_LOCKED;
        i3dVramUnlink(blk, I3D_VRAM_LIST_LOCK);
        heap->m_locked -= blk->m_size;
        blk = next;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00227f30

static int i3dVramClearNew(I3dVramHeap *heap)
{
    I3dVramBlock *blk = heap->m_sentinel.m_link[I3D_VRAM_LIST_NEW].m_next;
    I3dVramBlock *next;

    while (blk != &heap->m_sentinel)
    {
        next = blk->m_link[I3D_VRAM_LIST_NEW].m_next;
        blk->m_flags &= ~I3D_VRAM_NEW;
        i3dVramUnlink(blk, I3D_VRAM_LIST_NEW);
        blk = next;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00227f98

static int i3dVramClearMove(I3dVramHeap *heap)
{
    I3dVramBlock *blk = heap->m_sentinel.m_link[I3D_VRAM_LIST_MOVE].m_next;
    I3dVramBlock *next;

    while (blk != &heap->m_sentinel)
    {
        next = blk->m_link[I3D_VRAM_LIST_MOVE].m_next;
        blk->m_flags &= ~I3D_VRAM_MOVED;
        i3dVramUnlink(blk, I3D_VRAM_LIST_MOVE);
        blk = next;
    }
    return 0;
}

/* ---- allocation ------------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Gives a free block to a handle, splitting what it does not need into a
// free block after it, and puts it on the new list (to upload). 1 when the
// split fails, 0xf when the block is too small, else 0.
// ROR 0x00228000

static int i3dVramAssign(I3dVramHeap *heap, I3dVramBlock *blk, I3dVramHandle *handle)
{
    u_int want = handle->m_size;

    if (want > blk->m_size)
        return 0xf;
    if (want < blk->m_size)
    {
        if (i3dVramNewBlock(heap, blk->m_size - want, blk) == NULL)
            return 1;
        blk->m_size = want;
    }
    i3dVramBlockSetOwner(blk, handle);
    blk->m_flags |= I3D_VRAM_NEW;
    i3dVramLinkAfter(blk, I3D_VRAM_LIST_NEW, &heap->m_sentinel);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Folds the address-order blocks from first's successor up to (not
// including) `end` into `first`, frees `first` and moves it to the LRU tail.
// ROR 0x002280a8

static I3dVramBlock *i3dVramMergeRun(I3dVramHeap *heap, I3dVramBlock *first, I3dVramBlock *end)
{
    I3dVramBlock *blk = first->m_link[I3D_VRAM_LIST_ADDR].m_next;
    I3dVramBlock *next;

    while (blk != end)
    {
        next = blk->m_link[I3D_VRAM_LIST_ADDR].m_next;
        first->m_size += blk->m_size;
        i3dVramFreeBlock(heap, blk);
        blk = next;
    }
    i3dVramBlockRelease(first);
    i3dVramMoveAfter(first, I3D_VRAM_LIST_LRU, heap->m_sentinel.m_link[I3D_VRAM_LIST_LRU].m_prev);
    return first;
}

// ──────────────────────────────────────────────────────────────────────
// Frees every block on the evict list and makes one free block of their
// total size on the address list after `after`.
// ROR 0x00228138

static I3dVramBlock *i3dVramFreeEvicted(I3dVramHeap *heap, I3dVramBlock *after)
{
    I3dVramBlock *blk = heap->m_sentinel.m_link[I3D_VRAM_LIST_EVICT].m_next;
    I3dVramBlock *next;
    u_int size = 0;

    while (blk != &heap->m_sentinel)
    {
        next = blk->m_link[I3D_VRAM_LIST_EVICT].m_next;
        size += blk->m_size;
        i3dVramFreeBlock(heap, blk);
        blk = next;
    }
    return i3dVramNewBlock(heap, size, after);
}

// ──────────────────────────────────────────────────────────────────────
// Evicts unlocked blocks, oldest first -- free ones, then held ones -- until
// `want` units are gathered, and turns them into one free block after
// `after`. NULL when even every unlocked block is not enough.
// ROR 0x002281c0

static I3dVramBlock *i3dVramEvictLru(I3dVramHeap *heap, u_int want, I3dVramBlock *after)
{
    I3dVramBlock *sentinel = &heap->m_sentinel;
    I3dVramBlock *blk;
    I3dVramBlock *prev;
    u_int size = 0;
    int found = 0;

    i3dVramClearEvict(heap);
    for (blk = sentinel->m_link[I3D_VRAM_LIST_LRU].m_prev; blk != sentinel; blk = prev)
    {
        prev = blk->m_link[I3D_VRAM_LIST_LRU].m_prev;
        if ((blk->m_flags & I3D_VRAM_LOCKED) || !(blk->m_flags & I3D_VRAM_FREE))
            continue;
        i3dVramAddEvict(heap, blk);
        size += blk->m_size;
        if (size >= want)
        {
            found = 1;
            break;
        }
    }
    if (!found)
    {
        for (blk = sentinel->m_link[I3D_VRAM_LIST_LRU].m_prev; blk != sentinel; blk = prev)
        {
            prev = blk->m_link[I3D_VRAM_LIST_LRU].m_prev;
            if ((blk->m_flags & I3D_VRAM_LOCKED) || (blk->m_flags & I3D_VRAM_FREE))
                continue;
            i3dVramAddEvict(heap, blk);
            size += blk->m_size;
            if (size >= want)
            {
                found = 1;
                break;
            }
        }
        if (!found)
            return NULL;
    }
    return i3dVramFreeEvicted(heap, after);
}

// ──────────────────────────────────────────────────────────────────────
// 1 when the run of evict-flagged address neighbours around `blk` (forward
// from it, then backward from its predecessor) reaches `want` units.
// ROR 0x00228320 (no callers in the ROM)

int i3dVramEvictRunFits(I3dVramHeap *heap, I3dVramBlock *blk, u_int want)
{
    I3dVramBlock *sentinel = &heap->m_sentinel;
    I3dVramBlock *p;
    u_int size = 0;

    if (blk != sentinel && (blk->m_flags & I3D_VRAM_EVICT))
    {
        for (p = blk; ; )
        {
            size += p->m_size;
            if (size >= want)
                return 1;
            p = p->m_link[I3D_VRAM_LIST_ADDR].m_next;
            if (p == sentinel || !(p->m_flags & I3D_VRAM_EVICT))
                break;
        }
    }
    blk = blk->m_link[I3D_VRAM_LIST_ADDR].m_prev;
    if (blk == sentinel || !(blk->m_flags & I3D_VRAM_EVICT))
        return 0;
    for (;;)
    {
        size += blk->m_size;
        if (size >= want)
            return 1;
        blk = blk->m_link[I3D_VRAM_LIST_ADDR].m_prev;
        if (blk == sentinel || !(blk->m_flags & I3D_VRAM_EVICT))
            return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// In the run of evict-flagged address neighbours containing `blk`, the
// window [*first, *end) of at least `want` units holding the fewest owned
// units; both NULL when no window is long enough.
// ROR 0x002283e8 (no callers in the ROM)

void i3dVramEvictBestWindow(I3dVramHeap *heap, I3dVramBlock *blk, u_int want,
                            I3dVramBlock **first, I3dVramBlock **end)
{
    I3dVramBlock *sentinel = &heap->m_sentinel;
    I3dVramBlock *p;
    I3dVramBlock *tail;
    u_int best = 0x7fffffff;
    u_int owned = 0;
    u_int total = 0;
    u_int flags;
    u_int f;
    u_int sz;

    *first = NULL;
    *end   = NULL;

    // Back to the start of the run.
    for (p = blk->m_link[I3D_VRAM_LIST_ADDR].m_prev;
         p != sentinel && (p->m_flags & I3D_VRAM_EVICT);
         p = blk->m_link[I3D_VRAM_LIST_ADDR].m_prev)
        blk = p;
    flags = blk->m_flags;

    // Slide the window's start along the run; its tail only moves forward.
    tail = blk;
    for (;;)
    {
        if (!(flags & I3D_VRAM_EVICT))
            return;
        f = tail->m_flags;
        if (f & I3D_VRAM_EVICT)
        {
            while (total < want)
            {
                sz = tail->m_size;
                tail = tail->m_link[I3D_VRAM_LIST_ADDR].m_next;
                if (!(f & I3D_VRAM_FREE))
                    owned += sz;
                total += sz;
                f = tail->m_flags;
                if (!(f & I3D_VRAM_EVICT))
                    break;
            }
        }
        if (total < want)
            return;
        if (owned < best)
        {
            *first = blk;
            *end   = tail;
            best   = owned;
        }
        sz = blk->m_size;
        blk = blk->m_link[I3D_VRAM_LIST_ADDR].m_next;
        if (!(flags & I3D_VRAM_FREE))
            owned -= sz;
        total -= sz;
        flags = blk->m_flags;
    }
}

// ──────────────────────────────────────────────────────────────────────
// A block for a handle that has none: the first run of unlocked blocks of
// at least m_size units, walking the address list from the block at the
// LRU tail round to it again; otherwise blocks evicted by age. The block's
// free neighbours are absorbed, then it is assigned. NULL when nothing fits.
// ROR 0x00228528

static I3dVramBlock *i3dVramAlloc(I3dVramHeap *heap, I3dVramHandle *handle)
{
    I3dVramBlock *sentinel = &heap->m_sentinel;
    I3dVramBlock *start = sentinel->m_link[I3D_VRAM_LIST_LRU].m_prev;
    I3dVramBlock *blk = start;
    I3dVramBlock *run = NULL;
    u_int want = handle->m_size;
    u_int size = 0;

    for (;;)
    {
        if (!(blk->m_flags & I3D_VRAM_LOCKED))
        {
            size += blk->m_size;
            if (run == NULL)
                run = blk;
            if (size >= want)
            {
                blk = i3dVramMergeRun(heap, run, blk->m_link[I3D_VRAM_LIST_ADDR].m_next);
                break;
            }
        }
        else
        {
            size = 0;
            run  = NULL;
        }
        blk = blk->m_link[I3D_VRAM_LIST_ADDR].m_next;
        if (blk == sentinel)
        {
            // A run does not wrap round VRAM.
            blk  = sentinel->m_link[I3D_VRAM_LIST_ADDR].m_next;
            size = 0;
            run  = NULL;
        }
        if (blk == start)
        {
            blk = i3dVramEvictLru(heap, want, sentinel);
            break;
        }
    }
    if (blk == NULL)
        return NULL;
    i3dVramCoalesce(heap, blk);
    if (i3dVramAssign(heap, blk, handle) != 0)
        return NULL;
    return blk;
}

// ──────────────────────────────────────────────────────────────────────
// Empties the new, move and lock lists.
// ROR 0x00228618

int i3dVramEndFrame(I3dVramHeap *heap)
{
    i3dVramClearNew(heap);
    i3dVramClearMove(heap);
    i3dVramClearLock(heap);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Locks the n handles for this frame. When the units already locked plus
// those of the handles not locked yet (each counted once, by m_stamp) do
// not fit the heap, 0xf; otherwise the held blocks are locked and moved to
// the LRU front, then the others get blocks.
// ROR 0x00228650

int i3dVramReserve(I3dVramHeap *heap, I3dVramHandle **handles, u_int n)
{
    I3dVramHandle *h;
    I3dVramBlock *blk;
    u_int need = heap->m_locked;
    u_int locked;
    u_int i;
    int stamp;

    stamp = ++heap->m_stamp;
    for (i = 0; i < n; i++)
    {
        h = handles[i];
        locked = 0;
        if (h->m_block != NULL)
            locked = (h->m_block->m_flags & I3D_VRAM_LOCKED) != 0;
        if (!locked && h->m_stamp != stamp)
        {
            h->m_stamp = stamp;
            need += h->m_size;
        }
    }
    if (need > heap->m_size)
        return 0xf;

    i3dVramClearEvict(heap);
    for (i = 0; i < n; i++)
    {
        h = handles[i];
        blk = h->m_block;
        heap->m_requested += h->m_size;
        if (blk != NULL && !(blk->m_flags & I3D_VRAM_LOCKED))
        {
            i3dVramLock(heap, blk);
            i3dVramMoveAfter(blk, I3D_VRAM_LIST_LRU, &heap->m_sentinel);
        }
    }
    for (i = 0; i < n; i++)
    {
        h = handles[i];
        if (h->m_block == NULL)
        {
            blk = i3dVramAlloc(heap, h);
            i3dVramHandleSetBlock(h, blk);
            // PC: the ROM locks the result unchecked; a NULL (fragmented
            // heap) would write through address 0x30 there.
            if (blk == NULL)
                continue;
            i3dVramLock(heap, blk);
            i3dVramMoveAfter(blk, I3D_VRAM_LIST_LRU, &heap->m_sentinel);
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Lays the blocks out again from m_base in address order. Blocks on the new
// list only count toward m_new_size; held blocks whose address changed go
// on the move list -- at its front when they move down, at its back when
// they move up.
// ROR 0x00228820

int i3dVramCompact(I3dVramHeap *heap)
{
    I3dVramBlock *sentinel = &heap->m_sentinel;
    I3dVramBlock *blk = sentinel->m_link[I3D_VRAM_LIST_ADDR].m_next;
    u_int addr;
    u_int flags;
    u_int old;
    u_int size;

    if (blk == sentinel)
        return 0;
    addr = heap->m_base;
    for (;;)
    {
        flags = blk->m_flags;
        old  = blk->m_addr;
        size = blk->m_size;
        blk->m_addr     = addr;
        blk->m_old_addr = old;
        if (flags & I3D_VRAM_NEW)
            heap->m_new_size += size;
        else if (!(flags & I3D_VRAM_FREE) && addr != old)
        {
            heap->m_moved_size += size;
            blk->m_flags |= I3D_VRAM_MOVED;
            i3dVramLinkAfter(blk, I3D_VRAM_LIST_MOVE,
                             addr < old ? sentinel : sentinel->m_link[I3D_VRAM_LIST_MOVE].m_prev);
        }
        blk = blk->m_link[I3D_VRAM_LIST_ADDR].m_next;
        if (blk == sentinel)
            break;
        addr += size;
    }
    return 0;
}

/* ---- the heap ----------------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// One free block of the whole heap.
// ROR 0x00228920

static void i3dVramNewFreeAll(I3dVramHeap *heap)
{
    i3dVramNewBlock(heap, heap->m_size, &heap->m_sentinel);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00228940

void i3dVramSetBase(I3dVramHeap *heap, u_int base)
{
    heap->m_base = base;
    i3dVramReset(heap);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00228958

void i3dVramSetSize(I3dVramHeap *heap, u_int size)
{
    heap->m_size = size;
    i3dVramReset(heap);
}

// ──────────────────────────────────────────────────────────────────────
// Frees every block (their handles lose them); one free block of m_size
// remains.
// ROR 0x00228970

int i3dVramReset(I3dVramHeap *heap)
{
    I3dVramBlock *sentinel = &heap->m_sentinel;
    I3dVramBlock *blk;
    I3dVramBlock *next;

    i3dVramClearLock(heap);
    for (blk = sentinel->m_link[I3D_VRAM_LIST_ADDR].m_next; blk != sentinel; blk = next)
    {
        next = blk->m_link[I3D_VRAM_LIST_ADDR].m_next;
        i3dVramFreeBlock(heap, blk);
    }
    i3dVramNewFreeAll(heap);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002289e0 (no callers in the ROM)

int i3dVramClearStats(I3dVramHeap *heap)
{
    heap->m_moved_size = 0;
    heap->m_requested  = 0;
    heap->m_new_size   = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// A heap over n nodes with an n-entry free stack; base and size 0 (the
// context sets them, which resets the heap).
// ROR 0x002289f8

int i3dVramInit(I3dVramHeap *heap, I3dVramBlock *nodes, u_int n,
                 I3dVramBlock **free_stack)
{
    u_int i;

    heap->m_base   = 0;
    heap->m_size   = 0;
    heap->m_locked = 0;
    heap->m_stamp  = 0;
    heap->m_nodes      = nodes;
    heap->m_node_count = n;
    i3dVramBlockInit(&heap->m_sentinel);
    heap->m_sentinel.m_flags |= I3D_VRAM_SENTINEL;
    heap->m_free       = free_stack;
    heap->m_free_count = n;
    heap->m_requested  = 0;
    heap->m_new_size   = 0;
    heap->m_moved_size = 0;
    for (i = 0; i < n; i++)
    {
        i3dVramBlockInit(nodes);
        *free_stack++ = nodes++;
    }
    i3dVramNewFreeAll(heap);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00228ae0

int i3dVramTerm(I3dVramHeap *heap)
{
    u_int i;

    for (i = 0; i < heap->m_node_count; i++)
        i3dVramBlockUnlinkAll(&heap->m_nodes[i]);
    i3dVramBlockUnlinkAll(&heap->m_sentinel);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Absorbs the free blocks next to `blk` in address order, after it then
// before it.
// ROR 0x00228b50

static int i3dVramCoalesce(I3dVramHeap *heap, I3dVramBlock *blk)
{
    I3dVramBlock *sentinel = &heap->m_sentinel;
    I3dVramBlock *p;
    I3dVramBlock *next;
    u_int size = blk->m_size;

    for (p = blk->m_link[I3D_VRAM_LIST_ADDR].m_next;
         p != sentinel && (p->m_flags & I3D_VRAM_FREE); p = next)
    {
        next = p->m_link[I3D_VRAM_LIST_ADDR].m_next;
        size += p->m_size;
        i3dVramFreeBlock(heap, p);
    }
    for (p = blk->m_link[I3D_VRAM_LIST_ADDR].m_prev;
         p != sentinel && (p->m_flags & I3D_VRAM_FREE); p = next)
    {
        next = p->m_link[I3D_VRAM_LIST_ADDR].m_prev;
        size += p->m_size;
        i3dVramFreeBlock(heap, p);
    }
    blk->m_size = size;
    return 0;
}
