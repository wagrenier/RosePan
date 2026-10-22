/* ==========================================================================
 *  dog/dogtexinfo.h
 *
 *  DogTexInfo (dog/dogtexinfo.cpp): a named texture table -- m_max_tex
 *  name records of 16 bytes and as many 0x198-byte i3d texture records,
 *  both from the game heap. The scene holds one at +0x29520 (256 entries,
 *  shadow01 first) and hands it to its 2D layers.
 * ======================================================================== */
#ifndef _DOG_DOGTEXINFO_H
#define _DOG_DOGTEXINFO_H

#include <stddef.h>

/* EE 0x20 (the scene's sky slots follow at +0x20). layout: host --
 * pointers. */
struct DogTexInfo
{
    void init(int theMax);                                               // ROR 0x001162ac
    /* Finds the texture by name or takes a free record and builds it from
     * the image; returns its index, -1 when full. */
    int  add(const void *image, const char *name);                      // ROR 0x00116b5c

    int   m_max_tex;            // ROR 0x00: theMax
    int   m_num_tex;            // ROR 0x04: records in use (assert m_num_tex <= m_max_tex)
    void *m_tex_info;           // ROR 0x08: theMax 16-byte records, a name pointer first
    void *m_tex_data;           // ROR 0x0c: theMax 0x198-byte texture records, 8-aligned
    unsigned char _10[0x20 - 0x10];
};
static_assert(offsetof(DogTexInfo, m_num_tex) == 0x04, "m_num_tex");
static_assert(offsetof(DogTexInfo, m_tex_info) == 0x08, "m_tex_info");
static_assert(offsetof(DogTexInfo, m_tex_data) >= 0x0c, "m_tex_data");

#endif /* _DOG_DOGTEXINFO_H */
