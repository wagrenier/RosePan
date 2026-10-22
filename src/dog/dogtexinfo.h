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

struct BedPfh;
struct i3dContext;

/* EE 0x20 (the scene's sky slots follow at +0x20). layout: host --
 * pointers. */
struct DogTexInfo
{
    void init(int theMax);                                               // ROR 0x001162ac
    /* Finds the texture by name or takes a free record and builds it from
     * the image; returns its index, -1 when full. */
    int  add(const void *image, const char *name);                      // ROR 0x00116b5c
    /* The record named theName, -1 when none. */
    int  find(const char *theName) const;                                // ROR 0x001163dc
    /* Adds a texture from a loaded image under a name; returns its index. */
    int  addData(const char *theName, void *data, int a, int b);         // ROR 0x00116d14
    /* Adds every texture of a pack. */
    int  addPfh(const BedPfh *pfh, int a, int b, int c);                 // ROR 0x001170b8
    /* Passes record theNo's picture and the flag to 0x219d80. */
    void setTexFlag(int theNo, int flag);                                // ROR 0x001174c4
    /* Trims m_num_tex past the last used record and hands the table to the
     * i3d context (i3dCtxSetTexTable). */
    void setTexTable(i3dContext *p);                                     // ROR 0x0011756c
    /* Drops a reference to texture theNo; the last one frees it. */
    void release(int theNo);                                             // ROR 0x00116618
    /* Makes texture dst use src's picture (both by name). */
    void copy(const char *dst, const char *src);                         // ROR 0x001173d4
    /* Record theN of m_tex_info (16 bytes, a name pointer first). */
    void *getTexInfo(int theN);                                          // ROR 0x001161e4

    int   m_max_tex;            // ROR 0x00: theMax
    int   m_num_tex;            // ROR 0x04: records in use (assert m_num_tex <= m_max_tex)
    void *m_tex_info;           // ROR 0x08: theMax 16-byte records, a name pointer first
    void *m_tex_data;           // ROR 0x0c: theMax 0x198-byte texture records, 8-aligned
    int   m_dirty;              // ROR 0x10: release / copy set it; MagScene::draw then calls setTexTable
    unsigned char _14[0x20 - 0x14];
};
static_assert(offsetof(DogTexInfo, m_num_tex) == 0x04, "m_num_tex");
static_assert(offsetof(DogTexInfo, m_tex_info) == 0x08, "m_tex_info");
static_assert(offsetof(DogTexInfo, m_tex_data) >= 0x0c, "m_tex_data");
static_assert(offsetof(DogTexInfo, m_dirty) >= 0x10, "m_dirty");

#endif /* _DOG_DOGTEXINFO_H */
