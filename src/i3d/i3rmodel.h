/* ==========================================================================
 *  i3d/i3rmodel.h
 *
 *  The i3r model helpers (i3d/i3rmodel.cpp, ROR 0x21a3e8..0x21ac80, between
 *  i3denv.cpp's fog and lights): an I3D_BIN model's packets, texture
 *  assignment and texture animation, its STQ map entities, its clip
 *  sphere and its node transforms. See i3rroot.h for the file layout and
 *  the host's relative pointers.
 *
 *  The root item's children these walk (checked on CHARA.RPK's first
 *  model; types in hex):
 *    [0] 03 the materials: 25 material -> [0] 02 -> 26 layers (the layer's
 *        data names its texture; its [3]'s children hold STQ entities)
 *    [2] 03 the meshes: 2d combined mesh -> [0] 46 / [1] 45 -> 02 -> 4c ->
 *        [0] 02 -> the mesh items i3rBuildMeshPacket takes
 *    [3] 02 the nodes: 2a node (data: the parent index)
 *    [6] 53 the packet buffer (data: I3rPacketBuf)
 * ======================================================================== */
#ifndef _I3D_I3RMODEL_H
#define _I3D_I3RMODEL_H

#include <stddef.h>

#include "i3d/i3rroot.h"

struct I3dTexEntry;

/* i3d's matrix struct (tag_I3dMatrixF4x4_s in DogModel's symbols): the
 * same 16 floats as an sceVu0FMATRIX, but the ROM copies it with
 * unaligned ldl / sdl, so it is only 4-aligned. EE 0x40. */
struct I3dMatrixF4x4
{
    float m[4][4];
};
static_assert(sizeof(I3dMatrixF4x4) == 0x40, "I3dMatrixF4x4 is 0x40 bytes on the EE");

/* A material's data (item type 0x25). From the file: plain EE layout; only
 * the fields this file touches. */
struct I3rMaterialData
{
    float   m_depth;            // ROR 0x00: the draw set's depth (i3dLayerFindMaterialDrawSet)
    u_int   m_sort_param;       // ROR 0x04: the draw set's sort parameter
    short   m_tex_frame;        // ROR 0x08: the texture animation's frame (i3rUpdateTexAnim steps it)
    u_char  m_sort_func;        // ROR 0x0a: the draw set's sort function
    u_char  m_sort_order;       // ROR 0x0b: the draw set's sort order
    u_char  m_tex_anim;        // ROR 0x0c: non-zero = animated (i3rReplaceStqMapEntity sets 1)
    u_char  m_0d[3];            // ROR 0x0d: not read here
};
static_assert(offsetof(I3rMaterialData, m_tex_frame) == 0x08, "I3rMaterialData::m_tex_frame");
static_assert(offsetof(I3rMaterialData, m_tex_anim) == 0x0c, "I3rMaterialData::m_tex_anim");
static_assert(sizeof(I3rMaterialData) == 0x10, "I3rMaterialData is 0x10 bytes on the EE");

/* A material layer's data (item type 0x26). From the file: plain EE
 * layout; the fixup relocates +0x14 and +0x18. The bytes from +0x24 are
 * read by the material packets (i3d/i3rmesh.cpp), which map them to GS
 * TEST / ALPHA fields. Its children: [0] / [1] the uv tracks (offset and
 * scale; pivot, scale and rotation), [2] the register overrides (type
 * 0x4a), [3] the STQ map entities. */
struct I3rLayerData
{
    float   m_specular[4];      // ROR 0x00: to VU1 when m_shade is 2 (role from that use)
    float   m_alpha_ref;        // ROR 0x10: AREF / 128
    u_int   m_14;               // ROR 0x14: a relocated pointer (not read here)
    u_int   m_tex_name;         // ROR 0x18: the texture's name
    int     m_tex_index;        // ROR 0x1c: its index in the assigned table, -1 none (i3rAssignTex)
    u_char  m_20[4];            // ROR 0x20: not read
    int     m_blend;            // ROR 0x24: ALPHA_1 (i3rPktLayerAlpha); 0 = not written
    u_char  m_28[4];            // ROR 0x28: not read
    u_char  m_afail;            // ROR 0x2c: AFAIL of the alpha test (i3rAlphaFailMode)
    u_char  m_atst;             // ROR 0x2d: the alpha test, an index into 0x2db350; 0 = off
    u_char  m_2e;               // ROR 0x2e: not read
    u_char  m_ztst;             // ROR 0x2f: 1 NEVER, 2 ALWAYS, 0x13 GREATER, 0x14 GEQUAL; else not written
    u_char  m_cull;             // ROR 0x30: 1 / 2 the VU1 cull sign -1 / +1, 3 flat (no IIP)
    u_char  m_31;               // ROR 0x31: not read
    u_char  m_shade;            // ROR 0x32: 0 unlit, 1 lights, 2 lights with specular
    u_char  m_fail_only;        // ROR 0x33: non-zero = the test always fails with this AFAIL
    u_char  m_34;               // ROR 0x34: not read
    u_char  m_date;             // ROR 0x35: 2 DATE off, 9 / 10 DATE on (DATM 1 / 0)
    u_char  m_ambient;          // ROR 0x36: non-zero = the light's ambient, else black (lit layers)
    u_char  m_37;               // ROR 0x37: not read
};
static_assert(offsetof(I3rLayerData, m_alpha_ref) == 0x10, "I3rLayerData::m_alpha_ref");
static_assert(offsetof(I3rLayerData, m_tex_name) == 0x18, "I3rLayerData::m_tex_name");
static_assert(offsetof(I3rLayerData, m_tex_index) == 0x1c, "I3rLayerData::m_tex_index");
static_assert(offsetof(I3rLayerData, m_blend) == 0x24, "I3rLayerData::m_blend");
static_assert(offsetof(I3rLayerData, m_afail) == 0x2c, "I3rLayerData::m_afail");
static_assert(offsetof(I3rLayerData, m_atst) == 0x2d, "I3rLayerData::m_atst");
static_assert(offsetof(I3rLayerData, m_ztst) == 0x2f, "I3rLayerData::m_ztst");
static_assert(offsetof(I3rLayerData, m_cull) == 0x30, "I3rLayerData::m_cull");
static_assert(offsetof(I3rLayerData, m_shade) == 0x32, "I3rLayerData::m_shade");
static_assert(offsetof(I3rLayerData, m_fail_only) == 0x33, "I3rLayerData::m_fail_only");
static_assert(offsetof(I3rLayerData, m_date) == 0x35, "I3rLayerData::m_date");
static_assert(offsetof(I3rLayerData, m_ambient) == 0x36, "I3rLayerData::m_ambient");
static_assert(sizeof(I3rLayerData) == 0x38, "I3rLayerData: the 0x38 bytes known");

/* A node's data (item type 0x2a): the parent node's index first, < 0 for
 * a root node. Node records are 2 bytes apart in the file. */
struct I3rNodeData
{
    short   m_parent;           // ROR 0x00
};
static_assert(sizeof(I3rNodeData) == 2, "I3rNodeData: the 2 bytes known");

/* The model's packet buffer (item type 0x53, the root's child 6). */
struct I3rPacketBuf
{
    u_int   m_buf;              // ROR 0x00: relocated: the buffer
    u_int   m_qwc;              // ROR 0x04: its size in qwords
};
static_assert(offsetof(I3rPacketBuf, m_qwc) == 0x04, "I3rPacketBuf::m_qwc");
static_assert(sizeof(I3rPacketBuf) == 0x08, "I3rPacketBuf: the 8 bytes known");

/* An i3d STQ map entity (tag_I3rStqMapEntity_s in DogModel's symbols): a
 * layer's [3] child points at one; i3rReplaceStqMapEntity swaps in one
 * with the same id. The scene keeps one at +0x2aa90 (MagScene init sets
 * +0x40 = 1, the id 3; the scene's next field is at +0x2aae0). EE 0x50. */
struct I3rStqMapEntity
{
    u_char  m_00[0x40];         // ROR 0x00: not read here
    u_char  m_40;               // ROR 0x40: MagScene init sets 1; role unknown
    u_char  m_id;               // ROR 0x41: what i3rReplaceStqMapEntity matches
    u_char  m_42[0x0e];         // ROR 0x42: not read here
};
static_assert(offsetof(I3rStqMapEntity, m_40) == 0x40, "I3rStqMapEntity::m_40");
static_assert(offsetof(I3rStqMapEntity, m_id) == 0x41, "I3rStqMapEntity::m_id");
static_assert(sizeof(I3rStqMapEntity) == 0x50, "I3rStqMapEntity is 0x50 bytes on the EE");

/* i3rSetRelocate's hooks: build the packets of every mesh into the root's
 * packet buffer (child 6), and clear that buffer. I3D_OK. */
int   i3rBuildPackets(I3rRoot *root);                                    // ROR 0x0021a3e8
int   i3rClearPackets(I3rRoot *root);                                    // ROR 0x0021a558
/* Each layer's texture index from a name table of n records (DogTexInfo's):
 * the first record of the layer's texture name, -1 when absent. I3D_OK. */
int   i3rAssignTex(I3rRoot *root, const I3dTexEntry *tex, int n);        // ROR 0x0021a5b0
/* The nodes' basic local transforms into one matrix per node. I3D_OK. */
int   i3rCopyLocalBasicTransform(I3rRoot *root, I3dMatrixF4x4 *mat);     // ROR 0x0021a700
/* Local node matrices to model matrices down the node tree (a node's
 * model matrix is its parent's times its local one; a root node's is its
 * local one). The parents come first. I3D_OK. */
int   i3rConvertLocalToModel(I3rRoot *root, I3dMatrixF4x4 *model,
                             const I3dMatrixF4x4 *local);                // ROR 0x0021a760
/* The model's clip sphere: centre (w = 1) and radius. I3D_OK. */
int   i3rCopyClipSphere(I3rRoot *root, float *center, float *radius);    // ROR 0x0021a828
/* Steps the frame of every animated material. I3D_OK. */
int   i3rUpdateTexAnim(I3rRoot *root);                                   // ROR 0x0021a8e0
/* Points every layer STQ slot whose entity has entity's id at entity and
 * marks its material animated. I3D_OK. */
int   i3rReplaceStqMapEntity(I3rRoot *root, I3rStqMapEntity *entity);    // ROR 0x0021a938
/* A material item's entity slots (its child 1's children 0..2): the
 * getters return the slot's pointer, NULL when a slot is missing; the
 * setters store one, 0 on success, 1 when a slot is missing. No callers
 * of the getters / of slots 1 and 2's setters in the ROM (slot 0's getter
 * hides after i3rReplaceStqMapEntity's return). */
void *i3rGetMatEntity0(const I3rItem *mat);                              // ROR 0x0021aa10
void *i3rGetMatEntity1(const I3rItem *mat);                              // ROR 0x0021aa70
void *i3rGetMatEntity2(const I3rItem *mat);                              // ROR 0x0021aad0
int   i3rSetMatEntity0(I3rItem *mat, void *entity);                      // ROR 0x0021ab30
int   i3rSetMatEntity1(I3rItem *mat, void *entity);                      // ROR 0x0021aba0
int   i3rSetMatEntity2(I3rItem *mat, void *entity);                      // ROR 0x0021ac10

#endif /* _I3D_I3RMODEL_H */
