/* ==========================================================================
 *  bed/bedmemory.h
 *
 *  BedMemory, RoR's block heap (bed/bedmemory.cpp, ROR 0x100354..0x101718):
 *  one buffer cut into blocks on a doubly linked list, each led by a
 *  BEDMEMHEADER (0x20 bytes on the EE) and followed by its data. The word
 *  just before a block's data always holds the distance back to its header
 *  (m_header_size: the header plus the alignment shift), which is how free
 *  and realloc find the header from a pointer. The game heap is one of
 *  them (utl/utlmisc.cpp, bss 0x6e9800).
 *
 *  Fields are named from the asserts where one names them (m_buffer,
 *  m_size, m_next, m_chunk_size, m_flag, m_header_size); the rest from
 *  what the code does with them.
 * ======================================================================== */
#ifndef _BED_BEDMEMORY_H
#define _BED_BEDMEMORY_H

#include <stddef.h>

/* A block header. EE 0x20 bytes, the block's data right behind it.
 * layout: host -- three pointers. The host keeps the EE field order plus
 * one pad word so that m_header_size stays the header's last word (the
 * word before the data of an unshifted block), as on the EE. */
struct BEDMEMHEADER
{
    BEDMEMHEADER  *m_next;        // ROR 0x00
    BEDMEMHEADER  *m_prev;        // ROR 0x04
    int            m_chunk_size;  // ROR 0x08: data bytes (after any alignment shift)
    unsigned char *m_ptr;         // ROR 0x0c: the data
    short          m_heap_id;     // ROR 0x10: the heap's m_id, written by format only (unverified role)
    unsigned short m_flag;        // ROR 0x12: 1 in use, 0 free
    short          m_id;          // ROR 0x14: allocDebug's id
    short          m_line;        // ROR 0x16: allocDebug's line, -1 when none
    const char    *m_filename;    // ROR 0x18: allocDebug's file name
    int            m_pad;         // PC: keeps m_header_size last (no EE counterpart)
    int            m_header_size; // ROR 0x1c: data - header (0x20 + the alignment shift)
};
static_assert(offsetof(BEDMEMHEADER, m_header_size) == sizeof(BEDMEMHEADER) - sizeof(int),
              "m_header_size must be the word before an unshifted block's data");
static_assert(sizeof(BEDMEMHEADER) % 8 == 0, "headers stay pointer-aligned");

/* BedMemory::getInfo's result. EE 0x0c. */
struct BEDMEMINFO
{
    int m_max_free;     // ROR 0x00: largest free chunk
    int m_total_free;   // ROR 0x04: all free chunks
    int m_size;         // ROR 0x08: the heap's m_size
};
static_assert(sizeof(BEDMEMINFO) == 0x0c, "BEDMEMINFO size");
static_assert(offsetof(BEDMEMINFO, m_max_free) == 0x00, "m_max_free");
static_assert(offsetof(BEDMEMINFO, m_total_free) == 0x04, "m_total_free");
static_assert(offsetof(BEDMEMINFO, m_size) == 0x08, "m_size");

/* The heap record. EE 0x1c. layout: host -- holds pointers.
 * alloc's flags: bits 0-2 the alignment mode (0 0x20, 1 0x40, 2 0x80,
 * 3-6 4 bytes, 7 the heap's m_id & 7; utlMemAlloc passes -1);
 * (flags & 0x1c0) == 0x100 takes the block from the end of the heap. */
struct BedMemory
{
    void  create(int size, int id);                                      // ROR 0x00100354
    void  format(void *theBuffer, int theSize, int id);                  // ROR 0x001003f8
    void  destroy();                                                     // ROR 0x00100568
    int   alignBlock(BEDMEMHEADER *thePtr, int theSize, unsigned flags); // ROR 0x001005ac
    BEDMEMHEADER *splitTail(BEDMEMHEADER *thePtr, int theSize, unsigned flags); // ROR 0x00100714
    BEDMEMHEADER *findFree(int theSize, unsigned flags);                 // ROR 0x001008f0
    BEDMEMHEADER *findFreeFromEnd(int theSize, unsigned flags);          // ROR 0x00100988
    void *alloc(int theSize, int flags);                                 // ROR 0x00100a3c
    void *allocDebug(int theSize, const char *filename, int line, int id, int flags); // ROR 0x00100c84
    BEDMEMHEADER *getHeader(void *thePtr);                               // ROR 0x00100ce4
    void *shrink(void *thePtr, int theNewSize);                          // ROR 0x00100d30
    void *trimFront(void *theOldPtr, void *theNewPtr);                   // ROR 0x00100ecc
    void *realloc(void *thePtr, int theNewSize);                         // ROR 0x001010a4
    void  free(void *thePtr);                                            // ROR 0x00101328
    void  check(int mode);                                               // ROR 0x00101518
    void  getInfo(BEDMEMINFO *info);                                     // ROR 0x001015fc

    BEDMEMHEADER *m_buffer;     // ROR 0x00: the first block (the buffer, 16-aligned)
    BEDMEMHEADER *m_last;       // ROR 0x04: set to the first block by format (no reader found)
    void         *m_raw_buffer; // ROR 0x08: the buffer as given (destroy frees it)
    int           m_size;       // ROR 0x0c: usable bytes, a multiple of 16
    int           m_id;         // ROR 0x10: create's id; its low 3 bits are the default align mode
    int           m_num_alloc;  // ROR 0x14: live blocks (alloc +1, free -1)
    int           m_owner;      // ROR 0x18: 1 when create malloc'd the buffer
};

#endif /* _BED_BEDMEMORY_H */
