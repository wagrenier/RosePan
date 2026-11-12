/* ==========================================================================
 *  dog/dogtexinfo.h
 *
 *  DogTexInfo (dog/dogtexinfo.cpp, ROR 0x1161e4..0x117654): a named texture
 *  table -- m_max_tex name records of 16 bytes (the i3d context's texture
 *  table, I3dTexEntry: name, texture, CLUT) and as many 0x198-byte texture
 *  records (DogTexData: the i3d texture and CLUT, the TIM2 they were built
 *  from, a reference count and the name), both from the game heap. Records
 *  are found by name; adding a name that is already there counts a
 *  reference, release drops one and frees the record with the last.
 *  The scene holds one at +0x29520 (256 entries, shadow01 first) and hands
 *  it to its 2D layers and models; setTexTable hands the records to the
 *  i3d context, which draws with texture indices into them.
 * ======================================================================== */
#ifndef _DOG_DOGTEXINFO_H
#define _DOG_DOGTEXINFO_H

#include <stddef.h>

#include "i3d/i3dctx.h"
#include "i3d/i3dtex.h"

struct BedPfh;
struct BedPfhHead;
struct DogModel;
struct I3rRoot;

#define DOG_TEX_NAME_MAX 40     /* assert strlen(theTexName) < 40 */

/* addData's flags. */
#define DOG_TEX_COPY     0x1    /* any flag: keep a heap copy of the TIM2 (freed with the record) */
#define DOG_TEX_NEW      0x2    /* add a new record even when the name is there */

/* One texture. EE 0x198 (init's allocation, release's memset). layout:
 * host -- the i3d objects hold pointers; EE offsets in the comments. The
 * texture comes first: the name record's pPicture points at the record. */
struct DogTexData
{
    I3dTexture  m_tex;          // ROR 0x000
    I3dClut     m_clut;         // ROR 0x130: NULL in the name record when the TIM2 has no CLUT
    void       *m_tm2;          // ROR 0x168: the TIM2 the texture reads (a heap copy when m_own)
    short       m_own;          // ROR 0x16c: 1 when m_tm2 is ours to free
    short       m_ref;          // ROR 0x16e: references (add / addData 1, a repeated addData +1)
    char        m_name[DOG_TEX_NAME_MAX]; // ROR 0x170
};
static_assert(offsetof(DogTexData, m_tex) == 0, "the texture starts the record");

/* EE 0x20 (the scene's sky slots follow at +0x20). layout: host --
 * pointers. */
struct DogTexInfo
{
    /* Zeroes the table and sets the sepia colour 0xff 0xdc 0x98. */
    void clear();                                                        // ROR 0x0011627c
    /* Frees every record and both arrays, then clear. */
    void term();                                                         // ROR 0x001164d0
    /* term, then theMax empty records. */
    void init(int theMax);                                               // ROR 0x001162ac
    /* The colour factors addPfh's sepia pass multiplies the luminance by. */
    void setSepiaColor(int r, int g, int b);                             // ROR 0x0011626c
    /* Record theN of m_tex_info. */
    I3dTexEntry *getTexInfo(int theN);                                   // ROR 0x001161e4
    /* The record named theName, -1 when none. */
    int  find(const char *theName) const;                                // ROR 0x001163dc
    /* The first record without a name, else the next new one; -1 when full. */
    int  findFree() const;                                               // ROR 0x0011646c
    /* A texture already in VRAM (no TIM2), under name; returns its index,
     * -1 when full. */
    int  add(const I3dTexDesc *desc, const char *name);                  // ROR 0x00116b5c
    /* A texture and CLUT from a TIM2 under theTexName; a name already there
     * counts a reference (unless DOG_TEX_NEW). tcc < 0: from the name
     * (utlIsTccName). Returns the index, -1 when full, -2 when the copy
     * cannot be allocated, 3 when the TIM2 is refused. */
    int  addData(const char *theTexName, void *theTM2, int flags, int tcc); // ROR 0x00116d14
    /* Every TIM2 of a pack (theI3r NULL), or the pack's TIM2s that the
     * model theI3r names, optionally turned sepia first. 0, or -1 / -2
     * when addData fails, -3 when the pack lacks a texture the model uses. */
    int  addPfh(const BedPfh *pfh, I3rRoot *theI3r, int flags, int sepia); // ROR 0x001170b8
    /* Drops a reference to texture theN; the last one frees it. */
    void release(int theN);                                              // ROR 0x00116618
    /* Releases the textures a model's i3r root names. */
    void releaseModel(DogModel *model);                                  // ROR 0x001167f4
    /* Releases the textures named by a pack's name table. */
    void releasePfh(const BedPfhHead *head);                             // ROR 0x001169d0
    /* Makes texture fname use ename's picture (both by name). */
    void copy(const char *fname, const char *ename);                     // ROR 0x001173d4
    /* Record theNo's texture-colour-component flag (i3dTexSetTcc). */
    void setTexFlag(int theNo, int tcc);                                 // ROR 0x001174c4
    /* Trims m_num_tex past the last record with a picture and hands the
     * table to the i3d context (i3dCtxSetTexTable). */
    void setTexTable(i3dContext *p);                                     // ROR 0x0011756c
    /* Empty in retail (a debug listing); DogModel::loadPack calls it when
     * a model's textures cannot be assigned. */
    void dump();                                                         // ROR 0x0011764c

    int          m_max_tex;     // ROR 0x00: theMax
    int          m_num_tex;     // ROR 0x04: records in use (assert m_num_tex <= m_max_tex)
    I3dTexEntry *m_tex_info;    // ROR 0x08: theMax name records (pszTexName, pPicture, m_clut, 0)
    DogTexData  *m_tex_data;    // ROR 0x0c: theMax texture records, 8-aligned
    int          m_dirty;       // ROR 0x10: set by every change; MagScene::draw then calls setTexTable
    int          m_sepia_r;     // ROR 0x14: 0xff
    int          m_sepia_g;     // ROR 0x18: 0xdc
    int          m_sepia_b;     // ROR 0x1c: 0x98
};
static_assert(offsetof(DogTexInfo, m_num_tex) == 0x04, "m_num_tex");
static_assert(offsetof(DogTexInfo, m_tex_info) == 0x08, "m_tex_info");
static_assert(offsetof(DogTexInfo, m_tex_data) >= 0x0c, "m_tex_data");
static_assert(offsetof(DogTexInfo, m_dirty) >= 0x10, "m_dirty");
static_assert(offsetof(DogTexInfo, m_sepia_r) >= 0x14, "m_sepia_r");

#endif /* _DOG_DOGTEXINFO_H */
