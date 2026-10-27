/* ==========================================================================
 *  bed/bedpfh.h
 *
 *  BedPfh (bed/bedpfh.cpp; the name is in the assert text
 *  "pfh_head.getFileOffset(0) == (2048)", the header file in the inline
 *  methods' asserts): a view on a pack file ("RTPK"): a 0x20-byte header,
 *  then per-file tables chosen by the header's flags -- sizes (1),
 *  offsets from the header (2), 16-bit attributes (0x10, 0x20, ...) and
 *  the NUL-separated names (0x8000). The game reads packs whole or reads
 *  their first 2048 bytes and then single files (MagScene's loaders).
 * ======================================================================== */
#ifndef _BED_BEDPFH_H
#define _BED_BEDPFH_H

#include <stddef.h>

#include "common/ror_assert.h"
#include "sdk/eetypes.h"

/* The pack header, file data. EE 0x20. */
struct BedPfhHead
{
    u_int m_id;                 // ROR 0x00: 0x4b505452 "RTPK" (checkID)
    int   m_total_size;         // ROR 0x04: the whole pack's size
    short _08;
    short m_flags;              // ROR 0x0a: which tables follow (1 size, 2 offset, 0x10.. attributes, 0x8000 names)
    short _0c;
    short m_num_file;           // ROR 0x0e
    u_char _10[0x20 - 0x10];
};
static_assert(offsetof(BedPfhHead, m_total_size) == 0x04, "m_total_size");
static_assert(offsetof(BedPfhHead, m_flags) == 0x0a, "m_flags");
static_assert(offsetof(BedPfhHead, m_num_file) == 0x0e, "m_num_file");
static_assert(sizeof(BedPfhHead) == 0x20, "BedPfhHead is 0x20 bytes on the EE");

/* EE 0x14. layout: host -- pointers into the pack. The inline methods are
 * the ROM's (bed/bedpfh.h; their asserts carry its line numbers). */
struct BedPfh
{
    /* PC: GCC 2 builds a member BedPfh without a constructor call (its
     * owner calls clear, as SprTexture's does); C++ needs one to declare. */
    BedPfh() {}
    BedPfh(const void *p);                                               // ROR 0x00101750
    void clear();                                                        // ROR 0x00101738

    const char *getFileName(int theN) const;                             // ROR 0x00101928
    /* The index of the file named theName, -1 when none. */
    int  getFileIndex(const char *theName) const;                        // ROR 0x001019f0

    /* The ROM's line-70 inline: the constructor reads the flags through it. */
    int getFlags() const
    {
        ROR_ASSERT(m_top); /* 70 */
        return m_top->m_flags;
    }
    int getNumFile() const
    {
        ROR_ASSERT(m_top); /* 71 */
        return m_top->m_num_file;
    }
    int getTotalSize() const
    {
        ROR_ASSERT(m_top); /* 72 */
        return m_top->m_total_size;
    }
    /* The ROM's line-73 inline; the constructor's last check of m_top. */
    const BedPfhHead *getTop() const
    {
        ROR_ASSERT(m_top); /* 73 */
        return m_top;
    }
    bool checkID() const
    {
        ROR_ASSERT(m_top); /* 91 */
        return m_top->m_id == 0x4b505452; /* "RTPK" */
    }
    int getFileSize(int n) const { return m_size ? m_size[n] : -1; }
    int getFileOffset(int n) const { return m_offset ? m_offset[n] : -1; }
    /* The ROM's line-81 inline: file n's attribute word (its member is
     * m_user_data; DogActor::loadMotion takes motion ids from it). */
    int getUserData(int n) const
    {
        ROR_ASSERT(m_attr); /* 81: m_user_data */
        return m_attr[n];
    }
    void *getFile(int n) const
    {
        ROR_ASSERT(m_top); /* 85 */
        ROR_ASSERT(n >= 0 && n < m_top->m_num_file); /* 86 */
        return (u_char *)m_top + m_offset[n];
    }

    BedPfhHead  *m_top;         // ROR 0x00: the header
    int         *m_size;        // ROR 0x04: per-file sizes (flag 1)
    int         *m_offset;      // ROR 0x08: per-file offsets from m_top (flag 2)
    short       *m_attr;        // ROR 0x0c: per-file 16-bit attributes (flags 0x10..)
    const char  *m_name;        // ROR 0x10: the names, NUL-separated (flag 0x8000)
};
static_assert(offsetof(BedPfh, m_top) == 0, "m_top");

#endif /* _BED_BEDPFH_H */
