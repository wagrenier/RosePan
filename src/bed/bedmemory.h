/* ==========================================================================
 *  bed/bedmemory.h
 *
 *  BedMemory, RoR's block heap (bed/bedmemory.cpp, ROR 0x100354..): one
 *  malloc'd buffer cut into blocks, each led by a BEDMEMHEADER (0x20 bytes
 *  on the EE: m_next, m_size, the id at +0x10, the tag words allocDebug
 *  stores at +0x14 / +0x16 / +0x18 m_filename). The game heap is one of
 *  them (utl/utlmisc.cpp, bss 0x6e9800).
 *
 *  Minimal: only what other TUs call. The fields come from create /
 *  format / destroy (0x100354, 0x1003f8, 0x100568) and are named from the
 *  asserts where one says so; the rest of the record is unread.
 *  layout: host -- holds pointers (0x1c bytes on the EE; nothing past
 *  +0x18 is touched by the functions read so far).
 * ======================================================================== */
#ifndef _BED_BEDMEMORY_H
#define _BED_BEDMEMORY_H

struct BEDMEMHEADER;

/* The heap record. layout: host -- holds pointers. */
struct BedMemory
{
    void  create(int size, int id);                                      // ROR 0x00100354
    void  destroy();                                                     // ROR 0x00100568
    void *alloc(int size, int align);                                    // ROR 0x00100a3c
    void *allocDebug(int size, const char *filename, int line, int id, int align); // ROR 0x00100c84
    void *realloc(void *ptr, int size);                                  // ROR 0x001010a4
    void  free(void *ptr);                                               // ROR 0x00101328
    void  check(int mode);                                               // ROR 0x00101518

    BEDMEMHEADER *m_top;        // ROR 0x00: first block
    BEDMEMHEADER *m_last;       // ROR 0x04: set to the first block by format (role unread)
    void         *m_buffer;     // ROR 0x08: the buffer as given (before 16-byte alignment)
    int           m_size;       // ROR 0x0c: usable bytes, a multiple of 16
    int           m_id;         // ROR 0x10: create's second argument, copied into the first header
    int           m_14;         // ROR 0x14: cleared by create
    int           m_owner;      // ROR 0x18: 1 when create malloc'd the buffer (destroy frees it)
};

#endif /* _BED_BEDMEMORY_H */
