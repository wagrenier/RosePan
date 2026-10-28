/* ==========================================================================
 *  i3d/i3rdraw.cpp
 *
 *  The i3r model draw (ROR 0x21fb50..0x222f50): i3rBuildMeshPacket and
 *  i3rDrawModel. See i3rdraw.h.
 *
 *  PORT: the ROM draws a model by writing VU1 packets: per visible node it
 *  multiplies the node matrix into a work block, sets up the blend weights
 *  (i3rDrawSetupBlend 0x220670), and hands each combined mesh's two parts
 *  to i3rDrawMesh45 0x220098 / i3rDrawMesh46 0x220290, which bone-box test
 *  them and write a matrix packet and the per-material chains (0x220838,
 *  i3rDrawGroupSkinned 0x220b58, i3rDrawGroupRigid 0x220cf8 and their
 *  strip / batch writers to 0x222ce0) into the layer's draw-set groups.
 *  None of that reaches a GS on the host. i3rDrawModel keeps the ROM's
 *  walk for the model cull and its result (i3rDrawMesh45 / 46 keep their
 *  part culls), and records the model once at its layer's packet for the
 *  native module (RosePan_I3dRecordModel), which walks the same nodes,
 *  entries and parts and draws them through the renderer, with the camera
 *  made from the layer's view.
 *
 *  The writers below the parts: see the end of this file.
 * ======================================================================== */
#include "i3d/i3rdraw.h"

#include <math.h>
#include <string.h>

#include "i3d/i3dctx.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dview.h"
#include "i3d/i3rtype.h"
#include "lib/unassigned.h"
#include "rosepan/rendering/rosepan_i3d.h"

/* PC: the host camera of a view -- RosePan_I3dRecordCamera's pair: the
 * view's camera matrix as is (world -> camera, row-vector; camera z is the
 * depth, as the ROM's projections divide by it), and a projection of the
 * screen rectangle's frustum (m_rect_proj: the rectangle onto NDC +-s,
 * near / far onto -1..1) remade for the renderer: x / y scaled so the
 * rectangle fills NDC +-1, y negated when the view's NDC y runs down the
 * screen (the renderer's runs up), and z remapped to the renderer's reverse Z
 * (1 at near, 0 at far). With A = rect_proj[2][2] = (n + f) / (f - n) and
 * B = rect_proj[3][2] = -2nf / (f - n): n / (f - n) = (A - 1) / 2 and
 * nf / (f - n) = -B / 2. */
static void i3rHostCamera(const I3dView *view, float *out_view, float *out_proj)
{
    const sceVu0FVECTOR *p = view->m_rect_proj;
    float s[2];

    memcpy(out_view, view->m_camera, sizeof(float) * 16);
    memset(out_proj, 0, sizeof(float) * 16);
    /* The rectangle maps to NDC +-(w / (w + 2)) (the ROM keeps a pixel on
     * each side), and m_rect_viewport's diagonal is (w + 2) / 2 = v, so
     * the factor back to +-1 is v / (v - 1). The diagonal's y sign says
     * which way NDC y runs on the screen: positive = down, as the GS's. */
    for (int c = 0; c < 2; c++)
    {
        float v = fabsf(view->m_rect_viewport[c][c]);
        s[c] = v > 1.0f ? v / (v - 1.0f) : 1.0f;
    }
    if (view->m_rect_viewport[1][1] > 0.0f)
        s[1] = -s[1];
    out_proj[0 * 4 + 0] = p[0][0] * s[0];
    out_proj[2 * 4 + 0] = p[2][0] * s[0];
    out_proj[1 * 4 + 1] = p[1][1] * s[1];
    out_proj[2 * 4 + 1] = p[2][1] * s[1];
    out_proj[2 * 4 + 2] = (1.0f - p[2][2]) * 0.5f;
    out_proj[3 * 4 + 2] = -p[3][2] * 0.5f;
    out_proj[2 * 4 + 3] = 1.0f;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0021fb50
// PC: the ROM writes each mesh's VIF chains here once, at relocation --
// per 0x4c group the position / normal pool unpacks (VU1 0x1dc / 0x22c),
// per 0x56 batch and 0x47 strip the colour and STQ unpacks (VU1 0x17 /
// 0x2b, 0x225460 / 0x226060 / 0x2261b8) and an MSCAL of the strip program
// (0x302c40 + flags * 0x30) -- and stores their addresses in the group's
// and batches' data words for the draw-time writers. The host's mesh draw
// reads the items themselves (rosepan_i3dmesh.cpp rebases the blocks
// relative to their first unpack), so no packet is built and the data
// words keep the file's values.

void i3rBuildMeshPacket(I3rItem *mesh, void *buf)
{
    // callcheck: FUN_00225460, FUN_00226060, FUN_002261b8 (the VIF unpack writers: no packets on the host)
    (void)mesh;
    (void)buf;
}

/* i3rDrawModel's work block (EE 0x2b0 on its stack, passed to the part
 * writers and their group / strip writers): the fields the cull reads.
 * layout: host -- pointers; the EE offsets are in the comments. */
struct I3rDrawWork
{
    sceVu0FMATRIX           m_node_world;   // ROR 0x00: world * the node's matrix
    /* ROR 0x40: the 0x46 rigid group's bone matrix; 0x80: the 0x45 data's
     * first qword; 0x90: the matrix packet's address (writers only). */
    sceVu0FVECTOR          *m_world;        // ROR 0x94: i3rDrawModel's world
    sceVu0FMATRIX          *m_nodes;        // ROR 0x98: the node matrices
    const I3rItem          *m_root;         // ROR 0x9c: the root item (materials: child 0)
    const I3rNodeEntryData *m_entry;        // ROR 0xa0: the entry being drawn
    /* ROR 0xa4: the 0x46 part's data (i3rDrawMesh46 stores it). */
    const u_char           *m_disp;         // ROR 0xa8: the node's display record
    const void             *m_opt;          // ROR 0xac: the blend words
    /* ROR 0xf4: I3rBlendList[5], i3rDrawSetupBlend's (PC: the native
     * module's own, at replay). */
};
static_assert(offsetof(I3rDrawWork, m_world) >= 0x40, "I3rDrawWork::m_world");

// ──────────────────────────────────────────────────────────────────────
// The rigid part of a combined mesh (type 0x45): child 0's data is a box
// in the node's space, tested when the model crosses the guard band; then
// per group (child 1's children, data byte 0 = flags) the 0x4d meshes of
// its child 0 are written with the node matrix (0x220838, which also
// box-tests each mesh with its own box when clipping). Returns the clip
// state it drew with, I3D_VIEW_OUT when culled.
// ROR 0x00220098
// PC: only the cull and the result; the native module draws the part
// (RosePan_I3dRecordModel, rosepan_i3dmesh.cpp), so the matrix packet
// (an UNPACK to VU1 0x3d2: the matrix, its inverse, the normalised axes
// and their transpose) and the group writers have no host form.

static int i3rDrawMesh45(I3dLayer *layer, const I3rItem *part, int clip, I3rDrawWork *work)
{
    if (clip == I3D_VIEW_CLIP)
    {
        sceVu0FMATRIX box;

        i3dVu0MatMul(box, work->m_node_world, (sceVu0FVECTOR *)part->getChildren()[0].getData());
        clip = i3dViewTestBox(layer->m_view, box);
        if (clip == I3D_VIEW_OUT)
            return I3D_VIEW_OUT;
    }
    // callcheck: i3dCtxGetHead, i3dPktDmaTag, i3dVecNormalize, i3dMatCopy, i3dVu0MatInverse, i3dVu0MatTranspose, i3dCtxSetHead, i3rDrawGroup45 (the VU1 matrix packet and group writers: PC, above)
    return clip;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00220670
// PC: the ROM calls it from i3rDrawModel's entry walk into the work block
// (+0xf4) for the packet writers; the host's writers are the native mesh
// module, which calls it per entry at replay with the recorded opt.

void i3rDrawSetupBlend(const I3rItem *entry, const I3rBlendOpt *opt, I3rBlendList blend[I3R_BLEND_CHANNELS])
{
    if (i3rGetNumChild59(entry) == 0)
        return;
    const I3rItem *begin = i3rGetChildBegin59(entry);
    const I3rItem *end = i3rGetChildEnd59(entry);
    u_int mask = 0;
    float time[I3R_BLEND_CHANNELS];

    if (opt != NULL)
    {
        mask = opt->m_mask;
        for (int ch = 0; ch < I3R_BLEND_CHANNELS; ch++)
            time[ch] = opt->m_time[ch];
    }
    for (u_int ch = 0; ch < I3R_BLEND_CHANNELS; ch++)
    {
        I3rBlendList *list = &blend[ch];
        int num = 0;
        int index = 0;

        for (const I3rItem *it = begin; it != end; it++, index++)
        {
            float w = (mask & (1u << ch)) != 0 ? i3rBlendGetWeight(time[ch], it) : *(const float *)it->getData();

            if (w != 0.0f)
            {
                /* PC: the ROM has room for 8 and does not check. */
                if (num < 8)
                {
                    list->m_entry[num].m_index = index;
                    list->m_entry[num].m_weight = w;
                }
                num++;
            }
        }
        list->m_num = num < 8 ? num : 8;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The skinned part of a combined mesh (type 0x46): its data is the bone
// boxes (I3dBoneBoxes), tested through the entry's bone slots when the
// model crosses the guard band. Entries of up to 0x40 bones get their
// bone matrices pre-multiplied into the scratchpad (0x225ec8: offsets[i]
// * nodes[bones[i]]; work +0x250 says so); then per group a matrix packet
// and i3rDrawGroupSkinned, or for a rigid group (flag 8) the slot's bone
// matrix times the world through VU0 and i3rDrawGroupRigid. Returns the
// clip state it drew with, I3D_VIEW_OUT when culled.
// ROR 0x00220290
// PC: only the cull and the result, as i3rDrawMesh45; the native module
// skins the batches itself (rosepan_i3dmesh.cpp SkinBatch).

static int i3rDrawMesh46(I3dLayer *layer, const I3rItem *part, int clip, I3rDrawWork *work)
{
    if (clip == I3D_VIEW_CLIP)
    {
        clip = i3dViewTestBoneBox(layer->m_view, &work->m_entry->m_bones, (const I3dBoneBoxes *)part->getData(),
                                  work->m_world, work->m_nodes);
        if (clip == I3D_VIEW_OUT)
            return I3D_VIEW_OUT;
    }
    // callcheck: FUN_00225ec8, i3dCtxGetHead, i3dPktDmaTag, i3dVecNormalize, i3dMatCopy, i3dVu0MatInverse, i3dVu0MatTranspose, i3dCtxSetHead, i3rDrawGroupRigid, i3rDrawGroupSkinned (the scratchpad bone matrices and the VU1 writers: PC, above)
    return clip;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0021fe68

int i3rDrawModel(I3dLayer *layer, I3rRoot *root, sceVu0FMATRIX *nodes, sceVu0FVECTOR *world,
                 const void *opt)
{
    const I3rRootData *rd = (const I3rRootData *)root->m_item.getData();
    I3rItem *child = root->m_item.getChildren();
    I3dView *view = layer->m_view;
    int clip = I3D_VIEW_CLIP;
    int drawn = 0;

    if (nodes == NULL)
    {
        /* Root child 4's data: the model's box (three half axes and the
         * centre, model space). */
        sceVu0FMATRIX box;

        i3dVu0MatMul(box, world, (sceVu0FVECTOR *)child[4].getData());
        clip = i3dViewTestBox(view, box);
        if (clip == I3D_VIEW_OUT)
            return I3R_DRAW_CULLED;
    }
    if (nodes == NULL)
        nodes = (sceVu0FMATRIX *)i3rPtr(&rd->m_matrix14);

    /* The ROM's walk: a node is drawn unless its display record says
     * hidden; each enabled entry draws its combined mesh's parts (child 1:
     * type 0x45, child 0: type 0x46) when they have data, and the model
     * counts as drawn when a part's writer did not cull it. */
    I3rDrawWork work;
    work.m_world = world;
    work.m_nodes = nodes;
    work.m_root = &root->m_item;
    work.m_opt = opt;
    const I3rItem *meshes = child[2].getChildren();
    const I3rItem *node = child[3].getChildren();
    for (u_int i = 0; i < child[3].getNumChild(); i++)
    {
        const I3rItem *part = node[i].getChildren();
        const u_char *disp = (const u_char *)part[0].getData();

        if (disp != NULL && disp[I3R_NODE_DISP_SHOWN] == 0)
            continue;
        work.m_disp = disp;
        if (part[1].getNumChild() == 0)
            continue;
        i3dVu0MatMul(work.m_node_world, world, nodes[i]);
        const I3rItem *entry = part[1].getChildren();
        for (u_int e = 0; e < part[1].getNumChild(); e++)
        {
            const I3rNodeEntryData *ne = (const I3rNodeEntryData *)entry[e].getData();

            if (ne->m_enable == 0)
                continue;
            work.m_entry = ne;
            /* PC: i3rDrawSetupBlend(&entry[e], opt, work + 0xf4) loads the
             * blend weights for the writers; the native module calls it at
             * replay with the opt recorded below. */
            const I3rItem *parts = meshes[ne->m_mesh].getChildren();
            if (parts[1].m_data != 0 && i3rDrawMesh45(layer, &parts[1], clip, &work) != I3D_VIEW_OUT)
                drawn = 1;
            if (parts[0].m_data != 0 && i3rDrawMesh46(layer, &parts[0], clip, &work) != I3D_VIEW_OUT)
                drawn = 1;
        }
    }

    if (drawn)
    {
        /* PC: the native i3d module -- the camera and the model recorded
         * at a small packet of the layer, so the replay draws the model in
         * the layer's order. */
        float host_view[16], host_proj[16];
        i3dContext *ctx = layer->m_ctx;

        i3rHostCamera(view, host_view, host_proj);
        u_long *pkt = i3dLayerBeginPacket(layer);
        u_long *p = i3dPktDmaTag(pkt, 2, 0x10, NULL, 0, 0);
        RosePan_I3dRecordCamera(p, host_view, host_proj);
        RosePan_I3dRecordModel(p + 2, root, (const float *)world, (const float *)nodes,
                               ctx->m_tex_table, (int)ctx->m_tex_table_count, (const I3rBlendOpt *)opt);
        p[0] = p[1] = p[2] = p[3] = 0;
        i3dLayerEndPacket(layer, p + 4);
    }

    // callcheck: i3rDrawSetupBlend, sceDmaSync (the blend weights are set up by the native module at replay, from the recorded opt; the DMA wait: no packets, PORT: in the file header)
    // PC: sceDmaSync on fromSPR (0x1000d000): the packet writers' scratchpad
    // transfers; the host made none.
    return drawn ? I3R_DRAW_OK : I3R_DRAW_CULLED;
}

// ──────────────────────────────────────────────────────────────────────
//  The writers below the parts, and what the native module
//  (rosepan_i3dmesh.cpp) does in their place:
//  - ROR 0x00220838 i3rDrawGroup45: the 0x45 part's groups; per 0x4d mesh
//    a box test at clip 2, the material packet, then one of the writers
//    below. Host: the 0x45 branch of RosePan_I3dMeshDraw -> DrawMeshList
//    with the node's matrix (no per-mesh cull: off-screen meshes are
//    clipped by the renderer).
//  - ROR 0x00220b58 i3rDrawGroupSkinned, ROR 0x00220cf8 i3rDrawGroupRigid:
//    the 0x46 part's groups (flag 8 = rigid). Host: the 0x46 branch, the
//    rigid slot's bone matrix or SkinBatch.
//  - ROR 0x00220fb0 i3rPktDrawHeader, ROR 0x00221080 i3rPktCallStrips: a
//    batch's VU1 header (the +0x80 vector, MSCAL 0x15) and the DMA CALLs of
//    its strips' prebuilt chains: per material layer u (the layer list's
//    child count) whose byte in the 0x4d mesh's +0x08 list is not -1, a
//    CALL of the layer's packet (the material packet's m_layer_pkt[u]) and
//    one of the batch's strip group of that byte. Host: DrawBatch the same
//    per layer -- the layer's TEST / ALPHA and texture, then the group's
//    strips (DrawStripGroup).
//  - ROR 0x00221ab0 i3rDrawMesh45Clip, ROR 0x00221d18 i3rDrawMesh45Sorted,
//    ROR 0x00221f50 i3rDrawMesh45SortedClip, ROR 0x002221e8
//    i3rDrawMeshSkinned, ROR 0x00222560 i3rDrawMeshSkinnedSorted,
//    ROR 0x00222850 i3rDrawMeshRigidClip, ROR 0x00222aa0
//    i3rDrawMeshRigidSorted, ROR 0x00222ce0 i3rDrawMeshRigidSortedClip:
//    the per-batch variants -- clip 2 box-tests each batch, a material
//    whose packet +0x14 is set gets a depth key per batch
//    (i3dViewGetDepth). Host: DrawBatch draws every batch with the
//    renderer's depth buffer; no depth-keyed order yet (open-items.md
//    "Mesh draw").
//  - ROR 0x002210f0 i3rDrawMeshBlend: the writers' choice for a group with
//    flags & 3 when i3rDrawModel has an opt. Per batch it builds the pools
//    on the EE: the positions (opt mask bit 0) and normals (bit 1) through
//    i3rBlendVertices with the work block's lists 0 / 1, then for a
//    skinned group (flags & 0xc == 4) 0x225670 skins them; per strip the
//    colours (bit 3) and STQ (bit 4) the same way with lists 3 / 4; a
//    channel without its bit gets the slot's own block (0x225460). Then
//    UNPACKs of those to VU1 0x1dc / 0x22c / 0x17 / 0x2b, the batch header
//    and the strips' MSCALs -- or, with a depth-keyed material, one state
//    packet per batch keyed by i3dViewGetDepth. Host: DrawBatch's
//    LoadChannel calls i3rBlendVertices for the channels opt names and
//    writes the result into the host VU memory; SkinBatch then skins it.
