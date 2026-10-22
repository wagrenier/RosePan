/* ==========================================================================
 *  i3d/i3dvram.h
 *
 *  The i3d library's VRAM cache (i3d/i3dvram.cpp, ROR 0x227a98..0x228c08):
 *  a heap of GS memory cut into blocks, each block a node on up to six
 *  intrusive doubly linked lists at once -- address order, LRU, locked this
 *  frame, eviction candidates, newly assigned (to upload) and moved (to copy
 *  inside VRAM). A texture or CLUT that wants to be drawn reserves a block
 *  through its handle each frame (i3dVramReserve); blocks of textures not
 *  drawn lately are evicted, the heap is compacted, and the context turns the
 *  new / moved lists into GS transfers (i3dCtxMakeVramTransfer).
 *
 *  i3dContext holds two heaps: textures in GS pages, CLUTs in 4-block units
 *  (i3dCtxSetup). Names are inferred from the code (no symbols, no asserts).
 * ======================================================================== */
#ifndef _I3D_I3DVRAM_H
#define _I3D_I3DVRAM_H

#include <stddef.h>

struct I3dVramBlock;
struct I3dVramHandle;

/* The lists a block can be on (index into I3dVramBlock::m_link). */
enum
{
    I3D_VRAM_LIST_ADDR  = 0,    /* every block, in VRAM address order */
    I3D_VRAM_LIST_LRU   = 1,    /* most recently reserved first */
    I3D_VRAM_LIST_LOCK  = 2,    /* reserved this frame */
    I3D_VRAM_LIST_EVICT = 3,    /* chosen to be evicted */
    I3D_VRAM_LIST_NEW   = 4,    /* assigned this frame: upload */
    I3D_VRAM_LIST_MOVE  = 5,    /* moved by i3dVramCompact: copy */
    I3D_VRAM_LISTS      = 6
};

/* I3dVramBlock::m_flags. */
enum
{
    I3D_VRAM_FREE     = 0x1,        /* no owner */
    I3D_VRAM_OWNED    = 0x10,       /* m_owner holds it */
    I3D_VRAM_LOCKED   = 0x100,      /* on the lock list */
    I3D_VRAM_EVICT    = 0x1000,     /* on the evict list */
    I3D_VRAM_NEW      = 0x10000,    /* on the new list */
    I3D_VRAM_MOVED    = 0x100000,   /* on the move list */
    I3D_VRAM_SENTINEL = 0x1000000   /* the heap's list head (i3dVramInit) */
};

/* One list's links. EE 8. layout: host -- two pointers. */
struct I3dVramLink
{
    I3dVramBlock *m_prev;       // ROR 0x00
    I3dVramBlock *m_next;       // ROR 0x04
};

/* A block of VRAM. EE 0x44 (entry 23 of the library's size table).
 * layout: host -- pointers; EE offsets in the comments. */
struct I3dVramBlock
{
    I3dVramLink    m_link[I3D_VRAM_LISTS];  // ROR 0x00
    u_int          m_flags;                 // ROR 0x30: I3D_VRAM_*
    u_int          m_addr;                  // ROR 0x34: heap units, set by i3dVramCompact
    u_int          m_size;                  // ROR 0x38: heap units
    u_int          m_old_addr;              // ROR 0x3c: m_addr before the last compaction
    I3dVramHandle *m_owner;                 // ROR 0x40
};
static_assert(offsetof(I3dVramBlock, m_link) == 0, "I3dVramBlock::m_link");

/* What a texture or CLUT keeps to hold VRAM. EE 0x18 (entry 24 of the size
 * table; unverified that it is this struct). layout: host -- pointers. */
struct I3dVramHandle
{
    void         *m_image;      // ROR 0x00: the texture / CLUT the transfer builders read
    u_int         m_size;       // ROR 0x04: heap units wanted
    I3dVramBlock *m_block;      // ROR 0x08: the block held, NULL when none
    int           m_stamp;      // ROR 0x0c: the heap's m_stamp when last counted
    int           m_10;         // ROR 0x10: unverified
    int           m_static;     // ROR 0x14: non-zero = placed by hand, not cached (i3dCtxReserveTextures)
};
static_assert(offsetof(I3dVramHandle, m_image) == 0, "I3dVramHandle::m_image");

/* A VRAM heap. EE 0x70. layout: host -- pointers and the sentinel block. */
struct I3dVramHeap
{
    u_int          m_base;          // ROR 0x00: first unit (i3dVramSetBase)
    u_int          m_size;          // ROR 0x04: units (i3dVramSetSize)
    u_int          m_locked;        // ROR 0x08: units locked this frame
    int            m_stamp;         // ROR 0x0c: i3dVramReserve's call count
    I3dVramBlock  *m_nodes;         // ROR 0x10: the caller's node array
    u_int          m_node_count;    // ROR 0x14
    I3dVramBlock   m_sentinel;      // ROR 0x18: head of all six lists
    I3dVramBlock **m_free;          // ROR 0x5c: stack of unused nodes
    u_int          m_free_count;    // ROR 0x60
    u_int          m_requested;     // ROR 0x64: units asked for (statistics)
    u_int          m_new_size;      // ROR 0x68: units on the new list after compaction
    u_int          m_moved_size;    // ROR 0x6c: units moved by compaction
};
static_assert(offsetof(I3dVramHeap, m_base) == 0x00, "I3dVramHeap::m_base");
static_assert(offsetof(I3dVramHeap, m_size) == 0x04, "I3dVramHeap::m_size");
static_assert(offsetof(I3dVramHeap, m_locked) == 0x08, "I3dVramHeap::m_locked");
static_assert(offsetof(I3dVramHeap, m_stamp) == 0x0c, "I3dVramHeap::m_stamp");
static_assert(offsetof(I3dVramHeap, m_nodes) == 0x10, "I3dVramHeap::m_nodes");

/* ROR 0x00228618: empties the new, move and lock lists (m_locked back to 0). */
int  i3dVramEndFrame(I3dVramHeap *heap);                                 // ROR 0x00228618
/* ROR 0x00228650: locks n handles for this frame, allocating blocks for
 * those without one; 0xf when they do not fit, else 0. */
int  i3dVramReserve(I3dVramHeap *heap, I3dVramHandle **handles, u_int n); // ROR 0x00228650
/* ROR 0x00228820: reassigns addresses in address order from m_base; moved
 * owned blocks go on the move list. */
int  i3dVramCompact(I3dVramHeap *heap);                                  // ROR 0x00228820
void i3dVramSetBase(I3dVramHeap *heap, u_int base);                      // ROR 0x00228940
void i3dVramSetSize(I3dVramHeap *heap, u_int size);                      // ROR 0x00228958
/* ROR 0x00228970: frees every block, one free block of m_size remains. */
int  i3dVramReset(I3dVramHeap *heap);                                    // ROR 0x00228970
/* ROR 0x002289f8: a heap over n nodes and an n-entry free stack. */
int  i3dVramInit(I3dVramHeap *heap, I3dVramBlock *nodes, u_int n,
                 I3dVramBlock **free_stack);                             // ROR 0x002289f8
int  i3dVramTerm(I3dVramHeap *heap);                                     // ROR 0x00228ae0

#endif /* _I3D_I3DVRAM_H */
