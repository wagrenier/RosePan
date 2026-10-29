/* ==========================================================================
 *  i3d/i3croot.h
 *
 *  The i3d collision file (an "i3c": I3D_I3C, DogModel's m_i3c_root;
 *  i3d/i3croot.cpp, ROR 0x22ba20..0x22c1f8): its root, the node and part
 *  records, the part trees, and the lookups DogModel and the casts use.
 *
 *  An I3D_I3C file: the id "I3D_I3C\0" (rodata 0x303a70), a relocation
 *  byte at +0x10 (1 = the file holds offsets), the node records (12
 *  bytes: a name, a part count, the parts) and the group name table. A
 *  part (0x30 bytes) holds a tree of 12-byte nodes (+0x28: a count, bit
 *  15 = leaf, and the children or the leaf's data) and two more pointers.
 *  Every pointer is an offset from the root; -1 and 0 mean none.
 *
 *  PORT: the host keeps the offsets. The fields stay 32 bits (the file's
 *  bytes), and the ROM's tree relocator only adds the root to values below
 *  the root's address (a guard against relocating twice) that has no
 *  meaning for a host address. So i3cRelocate / i3cUnrelocate walk the
 *  file as the ROM does but only turn -1 into 0 and keep the relocation
 *  byte; every reader resolves a field with i3cPtr(root, field).
 * ======================================================================== */
#ifndef _I3D_I3CROOT_H
#define _I3D_I3CROOT_H

#include <stddef.h>
#include <stdint.h>

#ifndef I3D_OK
#define I3D_OK 0
#endif

/* An i3c root. From the file: plain EE layout, the words this file reads. */
struct I3cRoot
{
    char    m_id[8];            // ROR 0x00: "I3D_I3C\0"
    u_char  _08[8];             // ROR 0x08: not read here
    u_char  m_relocate;         // ROR 0x10: 1 while the file holds offsets (on the host: always)
    u_char  _11;
    u_char  m_both_face;        // ROR 0x12: i3cSetBothFaceEnable
    u_char  _13;
    u_short m_num_node;         // ROR 0x14: i3cGetNumNode
    u_short m_num_group;        // ROR 0x16: i3cGetNumGroup
    u_int   m_node;             // ROR 0x18: m_num_node I3cNode records
    u_int   m_group_name;       // ROR 0x1c: m_num_group name offsets
};
static_assert(offsetof(I3cRoot, m_relocate) == 0x10, "I3cRoot::m_relocate");
static_assert(offsetof(I3cRoot, m_both_face) == 0x12, "I3cRoot::m_both_face");
static_assert(offsetof(I3cRoot, m_num_node) == 0x14, "I3cRoot::m_num_node");
static_assert(offsetof(I3cRoot, m_num_group) == 0x16, "I3cRoot::m_num_group");
static_assert(offsetof(I3cRoot, m_node) == 0x18, "I3cRoot::m_node");
static_assert(offsetof(I3cRoot, m_group_name) == 0x1c, "I3cRoot::m_group_name");
static_assert(sizeof(I3cRoot) == 0x20, "I3cRoot: the 0x20 bytes known");

/* A collision node (named after a model node: DogModel::setupColliTbl). EE 0xc. */
struct I3cNode
{
    u_int   m_name;             // ROR 0x00: the node's name
    u_short m_num_part;         // ROR 0x04
    u_short m_enable;           // ROR 0x06: 0 = i3cCast skips the node
    u_int   m_part;             // ROR 0x08: m_num_part I3cPart records
};
static_assert(offsetof(I3cNode, m_num_part) == 0x04, "I3cNode::m_num_part");
static_assert(offsetof(I3cNode, m_enable) == 0x06, "I3cNode::m_enable");
static_assert(offsetof(I3cNode, m_part) == 0x08, "I3cNode::m_part");
static_assert(sizeof(I3cNode) == 0x0c, "I3cNode is 0xc bytes on the EE");

/* A node's part: a box (centre, half extents) around its tree, its group,
 * and its vertex array (16-byte points the leaf triangles index). EE 0x30. */
struct I3cPart
{
    float   m_center[4];        // ROR 0x00: the box centre (i3cCast)
    float   m_half[4];          // ROR 0x10: the box half extents
    int     m_group;            // ROR 0x20: the group (I3cQuery::m_group)
    u_int   m_24;               // ROR 0x24: an offset: 8-byte records a triangle's +6 indexes (0x2333b0)
    u_int   m_tree;             // ROR 0x28: the part's I3cTree root
    u_int   m_vertex;           // ROR 0x2c: the vertex array (sceVu0FVECTOR each)
};
static_assert(offsetof(I3cPart, m_half) == 0x10, "I3cPart::m_half");
static_assert(offsetof(I3cPart, m_group) == 0x20, "I3cPart::m_group");
static_assert(offsetof(I3cPart, m_24) == 0x24, "I3cPart::m_24");
static_assert(offsetof(I3cPart, m_tree) == 0x28, "I3cPart::m_tree");
static_assert(offsetof(I3cPart, m_vertex) == 0x2c, "I3cPart::m_vertex");
static_assert(sizeof(I3cPart) == 0x30, "I3cPart is 0x30 bytes on the EE");

/* A part tree's node: its box relative to the parent's (centre = parent
 * centre + parent half * m_ofs / 127, half = parent half * m_size / 127),
 * then children or a leaf's triangles. EE 0xc. */
struct I3cTree
{
    u_short m_info;             // ROR 0x00: bit 15 leaf (low 15 bits: the triangle count), else the child count
    s_char  m_ofs[3];           // ROR 0x02: the centre offset, x y z
    u_char  m_size[3];          // ROR 0x05: the half extents, x y z
    u_int   m_child;            // ROR 0x08: the m_info children (I3cTree), or the leaf's I3cTri array
};
static_assert(offsetof(I3cTree, m_ofs) == 0x02, "I3cTree::m_ofs");
static_assert(offsetof(I3cTree, m_size) == 0x05, "I3cTree::m_size");
static_assert(offsetof(I3cTree, m_child) == 0x08, "I3cTree::m_child");
static_assert(sizeof(I3cTree) == 0x0c, "I3cTree is 0xc bytes on the EE");

/* A leaf's triangle: three indices into the part's vertex array. EE 8. */
struct I3cTri
{
    u_short m_vtx[3];           // ROR 0x00
    u_short m_attr;             // ROR 0x06: an index into the part's m_24 records (0x2333b0)
};
static_assert(offsetof(I3cTri, m_attr) == 0x06, "I3cTri::m_attr");
static_assert(sizeof(I3cTri) == 0x08, "I3cTri is 8 bytes on the EE");

#define I3C_TREE_LEAF 0x8000

/* An i3c field resolved from its root (see PORT above): NULL for 0. */
static inline void *i3cPtr(const I3cRoot *root, u_int off)
{
    return off != 0 ? (void *)((const u_char *)root + off) : NULL;
}

/* ROR 0x0022ba20: I3D_OK when the file has the I3D_I3C id, else 3. */
int   I3cRoot__CheckIdString(const I3cRoot *root);                      // ROR 0x0022ba20
/* ROR 0x0022ba50 / 0x0022bae8: a part tree's offsets, recursively. */
void  i3cTreeRelocate(I3cTree *tree, I3cRoot *root);                    // ROR 0x0022ba50
void  i3cTreeUnrelocate(I3cTree *tree, I3cRoot *root);                  // ROR 0x0022bae8
/* ROR 0x0022bba0 / 0x0022bdb0: the whole file; nothing when already in
 * that state. 0. */
int   i3cRelocate(I3cRoot *root);                                       // ROR 0x0022bba0
int   i3cUnrelocate(I3cRoot *root);                                     // ROR 0x0022bdb0
int   i3cGetNumNode(const I3cRoot *root);                               // ROR 0x0022bfd8
/* ROR 0x0022bfe0: the name of node i, NULL past the count. */
const char *i3cGetNodeName(const I3cRoot *root, int i);                 // ROR 0x0022bfe0
int   i3cGetNumGroup(const I3cRoot *root);                              // ROR 0x0022c010
/* ROR 0x0022c018: the name of group i, NULL past the count. */
const char *i3cGetGroupName(const I3cRoot *root, int i);                // ROR 0x0022c018
/* ROR 0x0022c048 / 0x0022c0d8: the index of the named node / group, -1
 * when absent. No callers. */
int   i3cGetNodeIndexByName(const I3cRoot *root, const char *name);     // ROR 0x0022c048
int   i3cGetGroupIndexByName(const I3cRoot *root, const char *name);    // ROR 0x0022c0d8
/* ROR 0x0022c1d8: sets the both-faces byte; whether it was 1. */
int   i3cSetBothFaceEnable(I3cRoot *root, int flag);                    // ROR 0x0022c1d8

#endif /* _I3D_I3CROOT_H */
