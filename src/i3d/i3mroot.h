/* ==========================================================================
 *  i3d/i3mroot.h
 *
 *  The i3d motion file (an "i3m": I3D_I3M; i3d/i3mroot.cpp, ROR
 *  0x22b4b8..0x22ba20): its root, the node records, and the lookups
 *  DogActor uses (node count, names, keys).
 *
 *  An I3D_I3M file: the id "I3D_I3M\0" (rodata 0x303a68), the version
 *  0x20001 at +8, the motion's length in frames at +0x18 (a float), a
 *  relocation u16 at +0x16 (1 = the file holds offsets), the node table
 *  at +0x1c (12 bytes: a name, a key stride, a key count, the keys) and
 *  three value tables: +0x20 float4 vectors, +0x24 floats, +0x28 s16x4
 *  vectors in 1.15 fixed point. A key is u16 indices (i3d/i3mpose.h): its
 *  time and inverse length in the float table, then its position,
 *  rotation and scale in the float4 or the s16x4 table (flags +0x10; a
 *  packed position / scale is offset + value * scale from +0x30..+0x6f),
 *  then a spline's tangent keys. Key 0 sits at 0.0, the last at the
 *  length (on the disc). Every pointer is an offset from the root; 0
 *  means none.
 *  All 14,647 I3D_I3M files in the disc's DATA tree have version 0x20001
 *  and +0x16 = 1, every node inside the file (checked 2026-10-26).
 *
 *  PORT: as with the i3c files (i3croot.h), the host keeps the offsets.
 *  The fields stay 32 bits (the file's bytes) and cannot hold a host
 *  pointer, and every reader reaches the root. So i3mRelocate /
 *  i3mUnrelocate walk the file as the ROM does but change nothing, and
 *  keep the relocation u16 at 1; every reader resolves a field with
 *  i3mPtr(root, field).
 * ======================================================================== */
#ifndef _I3D_I3MROOT_H
#define _I3D_I3MROOT_H

#include <stddef.h>
#include <stdint.h>

#ifndef I3D_OK
#define I3D_OK 0
#endif

#define I3M_VERSION 0x20001

/* An i3m root. From the file: plain EE layout, the words this file reads. */
struct I3mRoot
{
    char    m_id[8];            // ROR 0x00: "I3D_I3M\0"
    u_int   m_version;          // ROR 0x08: I3M_VERSION
    int     m_size;             // ROR 0x0c: the i3m's own size (DogActor::loadMotion: a root track follows when its pack file is longer)
    u_short m_flags;            // ROR 0x10: I3M_* (31 in the files seen)
    u_short m_key_stride;       // ROR 0x12: u16 per key (i3mDecodeKeyPair's next key)
    u_short m_num_node;         // ROR 0x14: i3mGetNumNode
    u_short m_relocate;         // ROR 0x16: 1 while the file holds offsets (on the host: always)
    float   m_length;           // ROR 0x18: the motion's length in frames (MagScene::getActorMotionLength)
    u_int   m_node;             // ROR 0x1c: m_num_node I3mNode records
    u_int   m_20;               // ROR 0x20: the float4 vector table (16 bytes per index)
    u_int   m_float;            // ROR 0x24: the float table the keys index
    u_int   m_28;               // ROR 0x28: the s16x4 1.15 vector table (8 bytes per index)
    u_int   _2c;                // ROR 0x2c: not read
    float   m_pos_scale[4];     // ROR 0x30: a packed position = m_pos_offset + value * m_pos_scale
    float   m_pos_offset[4];    // ROR 0x40
    float   m_scale_scale[4];   // ROR 0x50: a packed scale, likewise
    float   m_scale_offset[4];  // ROR 0x60
};
static_assert(offsetof(I3mRoot, m_version) == 0x08, "I3mRoot::m_version");
static_assert(offsetof(I3mRoot, m_key_stride) == 0x12, "I3mRoot::m_key_stride");
static_assert(offsetof(I3mRoot, m_num_node) == 0x14, "I3mRoot::m_num_node");
static_assert(offsetof(I3mRoot, m_relocate) == 0x16, "I3mRoot::m_relocate");
static_assert(offsetof(I3mRoot, m_length) == 0x18, "I3mRoot::m_length");
static_assert(offsetof(I3mRoot, m_node) == 0x1c, "I3mRoot::m_node");
static_assert(offsetof(I3mRoot, m_20) == 0x20, "I3mRoot::m_20");
static_assert(offsetof(I3mRoot, m_float) == 0x24, "I3mRoot::m_float");
static_assert(offsetof(I3mRoot, m_28) == 0x28, "I3mRoot::m_28");
static_assert(offsetof(I3mRoot, m_flags) == 0x10, "I3mRoot::m_flags");
static_assert(offsetof(I3mRoot, m_pos_scale) == 0x30, "I3mRoot::m_pos_scale");
static_assert(offsetof(I3mRoot, m_pos_offset) == 0x40, "I3mRoot::m_pos_offset");
static_assert(offsetof(I3mRoot, m_scale_scale) == 0x50, "I3mRoot::m_scale_scale");
static_assert(offsetof(I3mRoot, m_scale_offset) == 0x60, "I3mRoot::m_scale_offset");
static_assert(sizeof(I3mRoot) == 0x70, "I3mRoot: the 0x70 bytes known");

/* I3mRoot::m_flags (i3mDecodeKey, i3mSampleKey). */
#define I3M_ROT_PACKED    0x01  /* rotations in the s16x4 table */
#define I3M_NO_SCALE      0x02  /* scale is (1, 1, 1, 1); keys have no scale index */
#define I3M_NO_SPLINE     0x04  /* spline modes play as linear ones */
#define I3M_POS_PACKED    0x08  /* positions in the s16x4 table */
#define I3M_SCALE_PACKED  0x10  /* scales in the s16x4 table */

/* A motion node (named after a model node). EE 0xc. */
struct I3mNode
{
    u_int   m_name;             // ROR 0x00: the node's name
    u_char  m_key_stride;       // ROR 0x04: u16 per key (i3mFindKey's step)
    u_char  _05;                // ROR 0x05: not read
    u_short m_num_key;          // ROR 0x06: i3mGetNodeNumKey
    u_int   m_key;              // ROR 0x08: m_num_key keys of m_key_stride u16
};
static_assert(offsetof(I3mNode, m_key_stride) == 0x04, "I3mNode::m_key_stride");
static_assert(offsetof(I3mNode, m_num_key) == 0x06, "I3mNode::m_num_key");
static_assert(offsetof(I3mNode, m_key) == 0x08, "I3mNode::m_key");
static_assert(sizeof(I3mNode) == 0x0c, "I3mNode is 0xc bytes on the EE");

/* An i3m field resolved from its root (see PORT above): NULL for 0. */
static inline void *i3mPtr(const I3mRoot *root, u_int off)
{
    return off != 0 ? (void *)((const u_char *)root + off) : NULL;
}

/* ROR 0x0022b4b8: I3D_OK when the file has the I3D_I3M id, else 3. */
int   I3mRoot__CheckIdString(const I3mRoot *root);                      // ROR 0x0022b4b8
/* ROR 0x0022b4e8: a field relocated by a delta unless 0 (see PORT). */
void  i3mRelocField(u_int *field, int delta);                           // ROR 0x0022b4e8
/* ROR 0x0022b500: whether the file has version I3M_VERSION. */
int   I3mRoot__CheckVersion(const I3mRoot *root);                       // ROR 0x0022b500
/* ROR 0x0022b518 / 0x0022b5f8: the whole file; 3 for a bad id, 4 for a
 * bad version, else I3D_OK (also when already in that state). */
int   i3mRelocate(I3mRoot *root);                                       // ROR 0x0022b518
int   i3mUnrelocate(I3mRoot *root);                                     // ROR 0x0022b5f8
/* ROR 0x0022b6f8: the node count, -1 for a bad id. */
int   i3mGetNumNode(const I3mRoot *root);                               // ROR 0x0022b6f8
/* ROR 0x0022b730: the name of node i, NULL past the count. */
const char *i3mGetNodeName(const I3mRoot *root, int i);                 // ROR 0x0022b730
/* ROR 0x0022b790: the index of the named node, -1 when absent (no id check). */
int   i3mGetNodeIndexByName(const I3mRoot *root, const char *name);     // ROR 0x0022b790
/* ROR 0x0022b820: node i, NULL past the count. No callers. */
const I3mNode *i3mGetNode(const I3mRoot *root, int i);                  // ROR 0x0022b820
/* ROR 0x0022b878: node i's key count, 0xffff past the count. No callers. */
int   i3mGetNodeNumKey(const I3mRoot *root, int i);                     // ROR 0x0022b878
/* ROR 0x0022b8d8: the time of node i's key k, 0.0 past either count. No callers. */
float i3mGetNodeKeyTime(const I3mRoot *root, int i, int k);             // ROR 0x0022b8d8
/* ROR 0x0022b970: every key time of node i into out; i, or 0xffff past
 * the count. No callers. */
int   i3mGetNodeKeyTimes(const I3mRoot *root, float *out, int i);       // ROR 0x0022b970

#endif /* _I3D_I3MROOT_H */
