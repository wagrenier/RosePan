/* ==========================================================================
 *  i3d/i3rroot.h
 *
 *  The i3d model file (I3D_BIN, an "i3r" root; i3d/i3rroot.cpp, ROR
 *  0x22b1b0..0x22b4b8): its header, the item tree it is made of, and the
 *  root data's texture / node name lookups DogModel and DogTexInfo use.
 *
 *  An I3D_BIN file: a 0x10-byte header (id "I3D_BIN\0", version
 *  0x00100001, the relocation state), then the root item at +0x10. Every
 *  item is 16 bytes: its data, its info word (bit 31 = the data needs no
 *  fixup, bits 30..24 the item type, bits 23..0 the child count), its
 *  children (an array of items) and its name. The root item has type 0x52.
 *  In the file every pointer is an offset: an item's data / children /
 *  name from the root item (+0x10), the pointers inside an item's data
 *  from the start of that data. i3rRelocate turns them into pointers, the
 *  item tree recursively (i3rItemRelocate 0x22c478, its per-type data
 *  fixups in table 0x2db4c8; the reverse 0x22c328 / 0x2db360).
 *
 *  PORT: the fields stay 32 bits (the file's own bytes), so a relocated
 *  field cannot hold a host pointer. On the host a relocated field holds
 *  the distance from the field itself to its target (0 stays NULL), and
 *  every reader goes through i3rPtr(); the relocators write i3rSetPtr().
 *  The file only needs to be under 2 GB.
 * ======================================================================== */
#ifndef _I3D_I3RROOT_H
#define _I3D_I3RROOT_H

#include <stddef.h>
#include <stdint.h>

#ifndef I3D_OK
#define I3D_OK 0
#endif

/* PORT: a field pointing outside the file further than 32 bits reach (an
 * STQ map entity the game owns: MagScene's, DogActor's eye uv) holds
 * I3R_FAR_PTR -- a distance no file under 2 GB has -- and its target sits
 * in a host side table keyed by the field's address (i3rroot.cpp). */
#define I3R_FAR_PTR 0x80000000u
void *i3rFarPtrGet(const u_int *field);
void  i3rFarPtrSet(u_int *field, const void *p);

/* A relocated pointer field (see PORT above): NULL for 0, else the field's
 * own address plus the signed distance it holds. */
static inline void *i3rPtr(const u_int *field)
{
    if (*field == I3R_FAR_PTR)
        return i3rFarPtrGet(field);
    return *field != 0 ? (void *)((const u_char *)field + (int)*field) : NULL;
}

/* Stores p into a relocated field: the distance from the field, 0 for NULL. */
static inline void i3rSetPtr(u_int *field, const void *p)
{
    *field = p != NULL ? (u_int)(int)((const u_char *)p - (const u_char *)field) : 0;
}

/* One node of the item tree. EE 0x10, from the file: plain EE layout. */
struct I3rItem
{
    u_int m_data;               // ROR 0x00: the item's data (type-specific)
    u_int m_info;               // ROR 0x04: bit 31 no data fixup, 30..24 type, 23..0 children
    u_int m_children;           // ROR 0x08: m_info & 0xffffff items
    u_int m_name;               // ROR 0x0c: the item's name (sorted per list: lookups bisect)

    u_int    getType() const     { return (m_info >> 24) & 0x7f; }
    u_int    getNumChild() const { return m_info & 0xffffff; }
    void    *getData() const     { return i3rPtr(&m_data); }
    I3rItem *getChildren() const { return (I3rItem *)i3rPtr(&m_children); }
    const char *getName() const  { return (const char *)i3rPtr(&m_name); }
};
static_assert(offsetof(I3rItem, m_data) == 0x00, "I3rItem::m_data");
static_assert(offsetof(I3rItem, m_info) == 0x04, "I3rItem::m_info");
static_assert(offsetof(I3rItem, m_children) == 0x08, "I3rItem::m_children");
static_assert(offsetof(I3rItem, m_name) == 0x0c, "I3rItem::m_name");
static_assert(sizeof(I3rItem) == 0x10, "I3rItem is 0x10 bytes on the EE");

/* The root item's type (the byte I3rRoot__CheckIdString tests at +0x17). */
#define I3R_TYPE_ROOT 0x52
/* The file version i3rSetRelocate accepts. */
#define I3R_VERSION 0x00100001

/* An I3D_BIN file (tag_I3rRoot_s in DogModel's symbols). EE 0x20, the
 * header and the root item; the rest of the file follows. */
struct I3rRoot
{
    char    m_id[8];            // ROR 0x00: "I3D_BIN\0" (rodata 0x3039d0)
    u_int   m_version;          // ROR 0x08: I3R_VERSION
    u_int   m_relocated;        // ROR 0x0c: 1 after i3rRelocate, 0 after i3rUnrelocate
    I3rItem m_item;             // ROR 0x10: type 0x52; offsets in the file are from here
};
static_assert(offsetof(I3rRoot, m_version) == 0x08, "I3rRoot::m_version");
static_assert(offsetof(I3rRoot, m_relocated) == 0x0c, "I3rRoot::m_relocated");
static_assert(offsetof(I3rRoot, m_item) == 0x10, "I3rRoot::m_item");
static_assert(sizeof(I3rRoot) == 0x20, "I3rRoot is 0x20 bytes on the EE");

/* The root item's data (type 0x52). From the file: plain EE layout. Only
 * the fields this file and the type's fixup pair (0x22ad20 / 0x22adf8)
 * touch; the fixups relocate +0x10..+0x20 and the name tables' entries,
 * not +0x24. The record may be longer (unverified). */
struct I3rRootData
{
    float  m_clip_center[3];    // ROR 0x00: the clip sphere's centre (i3rCopyClipSphere)
    float  m_clip_radius;       // ROR 0x0c: and its radius
    u_int  m_local_basic;       // ROR 0x10: one 0x40 matrix per node, the basic local transforms
                                //           (i3rCopyLocalBasicTransform, i3rGetRootMatrix10)
    u_int  m_matrix14;          // ROR 0x14: records of 0x40 (i3rGetRootMatrix14; role unknown)
    u_int  m_tex_name;          // ROR 0x18: m_num_tex name pointers (the TIM2 names)
    u_int  m_node_index;        // ROR 0x1c: m_num_node_name u_short node indices
    u_int  m_node_name;         // ROR 0x20: m_num_node_name name pointers, one per index
    u_int  m_param24;           // ROR 0x24: a value (not relocated; meaning unknown)
    u_char m_28[4];             // ROR 0x28: not read here
    u_short m_num_tex;          // ROR 0x2c
    u_short m_num_node_name;    // ROR 0x2e
};
static_assert(offsetof(I3rRootData, m_clip_radius) == 0x0c, "I3rRootData::m_clip_radius");
static_assert(offsetof(I3rRootData, m_local_basic) == 0x10, "I3rRootData::m_local_basic");
static_assert(offsetof(I3rRootData, m_matrix14) == 0x14, "I3rRootData::m_matrix14");
static_assert(offsetof(I3rRootData, m_tex_name) == 0x18, "I3rRootData::m_tex_name");
static_assert(offsetof(I3rRootData, m_node_index) == 0x1c, "I3rRootData::m_node_index");
static_assert(offsetof(I3rRootData, m_node_name) == 0x20, "I3rRootData::m_node_name");
static_assert(offsetof(I3rRootData, m_param24) == 0x24, "I3rRootData::m_param24");
static_assert(offsetof(I3rRootData, m_num_tex) == 0x2c, "I3rRootData::m_num_tex");
static_assert(offsetof(I3rRootData, m_num_node_name) == 0x2e, "I3rRootData::m_num_node_name");
static_assert(sizeof(I3rRootData) == 0x30, "I3rRootData: the 0x30 bytes known");

/* ROR 0x0022b1b0: the index of name in a table of count relocated name
 * pointers, -1 when absent. */
int   i3rFindName(const u_int *names, u_int count, const char *name);  // ROR 0x0022b1b0
/* ROR 0x0022b228: I3D_OK when the file has the I3D_BIN id and a root item
 * of type 0x52, 3 otherwise. */
int   I3rRoot__CheckIdString(const I3rRoot *root);                     // ROR 0x0022b228
/* ROR 0x0022b280: on = 1 relocates the file and builds its packets
 * (i3rBuildPackets), on = 0 clears them and turns the pointers back into
 * offsets; nothing when already in that state. 3 bad id, 4 bad version,
 * an item relocator's error, else I3D_OK. */
int   i3rSetRelocate(I3rRoot *root, int on);                           // ROR 0x0022b280
/* ROR 0x0022b330 / 0x0022b350: i3rSetRelocate(root, 1 / 0); I3D_OK. */
int   i3rRelocate(I3rRoot *root);                                      // ROR 0x0022b330
int   i3rUnrelocate(I3rRoot *root);                                    // ROR 0x0022b350
/* ROR 0x0022b370: the root's node count (its child 3's children). */
int   i3rGetNumNode(const I3rRoot *root);                              // ROR 0x0022b370
/* ROR 0x0022b388: the name of node index, NULL when no name has it. */
const char *i3rGetNodeNameByIndex(const I3rRoot *root, int index);     // ROR 0x0022b388
/* ROR 0x0022b3d8: the node index of the named node, -1 when the model has none. */
int   i3rGetNodeIndexByName(I3rRoot *root, const char *name);          // ROR 0x0022b3d8
/* ROR 0x0022b428 / 0x0022b438: the root's texture count and its n-th
 * texture name (the TIM2 names a DogTexInfo holds them under). */
int   i3rGetNumTex(const I3rRoot *root);                               // ROR 0x0022b428
const char *i3rGetTexName(const I3rRoot *root, int n);                // ROR 0x0022b438
/* ROR 0x0022b450: the index of the named texture, -1 when absent. No callers. */
int   i3rGetTexIndexByName(const I3rRoot *root, const char *name);     // ROR 0x0022b450
/* ROR 0x0022b478..0x0022b4a0: the root data's +0x24 value and records i
 * of its two 0x40-byte tables. No callers. */
u_int i3rGetRootParam24(const I3rRoot *root);                          // ROR 0x0022b478
void *i3rGetRootMatrix10(const I3rRoot *root, int i);                  // ROR 0x0022b488
void *i3rGetRootMatrix14(const I3rRoot *root, int i);                  // ROR 0x0022b4a0

#endif /* _I3D_I3RROOT_H */
