/* ==========================================================================
 *  rosepan_i3dmesh.h
 *
 *  The native i3d draw module's 3D mesh path (rosepan_i3d.h records, this
 *  draws): an i3r model's mesh items drawn through the renderer's mesh
 *  stream, the walk i3rDrawModel 0x21fe68 makes, with no VU1 and no GS.
 *
 *  The model tree it reads (item types in hex; architecture.md "The i3r
 *  model files"):
 *    root [3] nodes: 2a node -> [0] its data (+0x25 display byte, or no
 *        data = shown), [1] its entries: data +0 a u16 bone table, +4 the
 *        mesh index, +8 an enable byte
 *    root [2] meshes: 2d combined mesh -> [0] 46 -> [0] 02 -> 4c bone
 *        groups (data: +0 -> the u16 bone slot, +5 flags: 8 = rigid on the node
 *        the entry's bone table names; otherwise skinned per batch)
 *        -> [0] 02 -> 4d meshes (data +0xc the material index into root
 *        [0]) -> [0] 02 -> 56 vertex batches
 *    56 batch: [4] the position pool (VU1 0x1dc), [2] the normal pool
 *        (VU1 0x22c), [0] -> 30 -> [0] 02 -> 47 strips
 *    47 strip: data {u32 the index list, u8 qwc, u8 vertex count}, the
 *        list V2-16 (x = pool index, bit 15 = no triangle at this vertex;
 *        y bit 15 = the triangle (n-2, n-1, n) faces front, else it is
 *        reversed); [0] the vertex colours (VU1 0x17, TOPS), [1] the STQ
 *        (VU1 0x2b, TOPS)
 *    33 block (each of those): data {u32 the data, u16 qwc, u16 count, u8
 *        kind}: kind 0 plain V4-32 qwords, kind 1 a VIF stream (STCYCL /
 *        STMASK / STROW / STCOL / UNPACK) unpacked here as VIF1 would
 *        (STCOL 1.0, STROW 0 from i3dInitHardware).
 * ======================================================================== */
#ifndef ROSEPAN_I3DMESH_H
#define ROSEPAN_I3DMESH_H

struct I3rRoot;
struct I3dTexEntry;
struct I3rBlendOpt;

/* Draws `root` with `world` (its local -> world matrix, i3d row-vector
 * 4x4: p' = p * M) and `nodes` (one 4x4 per node, model space; NULL = the
 * root data's own table, i3rDrawModel's default), its layers' textures
 * from `tex` (`tex_count` entries; the context's texture table, which
 * i3rAssignTex indexed).  `opt`: i3rDrawModel's blend words (NULL: none);
 * with them, groups whose flags & 3 is set blend their pools and strips
 * the way i3rDrawMeshBlend 0x2210f0 does.  The renderer's camera must be
 * set. */
void RosePan_I3dMeshDraw(const I3rRoot *root, const float *world, const float *nodes,
                         const I3dTexEntry *tex, int tex_count, const I3rBlendOpt *opt);

/* One 0x4d mesh of a model as i3rDrawModel's part writers file it into
 * its material's draw-set group (i3d/i3rdraw.cpp): node `node`'s entry
 * `entry`, its combined mesh's part `part` (0x45 rigid, 0x46 skinned),
 * that part's group `group`, the group's mesh `mesh`; `batch` one of the
 * mesh's 0x56 batches (a depth-keyed material's per-batch packet), or -1
 * for all of them. */
struct RosePanI3dMeshRef
{
    unsigned node, entry, part, group, mesh;
    int      batch;
};

/* RosePan_I3dMeshDraw for the one mesh (or batch) `ref` names. */
void RosePan_I3dMeshDrawRef(const I3rRoot *root, const float *world, const float *nodes,
                            const I3dTexEntry *tex, int tex_count, const I3rBlendOpt *opt,
                            const RosePanI3dMeshRef *ref);

/* The GPU texture for an i3d texture object (rosepan_i3d.cpp's cache). */
const void *RosePan_I3dTextureFor(const void *tex, const void *clut, unsigned clut_index, int tcc);

#endif /* ROSEPAN_I3DMESH_H */
