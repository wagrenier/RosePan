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
 *  walk for the model cull and its result, and records the model once at
 *  its layer's packet for the native module (RosePan_I3dRecordModel),
 *  which walks the same nodes, entries and parts and draws them through
 *  the renderer, with the camera made from the layer's view.
 * ======================================================================== */
#include "i3d/i3rdraw.h"

#include <math.h>
#include <string.h>

#include "i3d/i3dctx.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dview.h"
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

    (void)opt;      // PC: i3rDrawSetupBlend's input; the host module reads no blend weights yet
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
    const I3rItem *meshes = child[2].getChildren();
    const I3rItem *node = child[3].getChildren();
    for (u_int i = 0; i < child[3].getNumChild(); i++)
    {
        const I3rItem *part = node[i].getChildren();
        const u_char *disp = (const u_char *)part[0].getData();

        if (disp != NULL && disp[I3R_NODE_DISP_SHOWN] == 0)
            continue;
        const I3rItem *entry = part[1].getChildren();
        for (u_int e = 0; e < part[1].getNumChild(); e++)
        {
            const I3rNodeEntryData *ne = (const I3rNodeEntryData *)entry[e].getData();

            if (ne->m_enable == 0)
                continue;
            /* PC: i3rDrawSetupBlend(&entry[e], opt, work) and the node
             * matrix world * nodes[i] feed the part writers; the host
             * module multiplies its own. */
            const I3rItem *parts = meshes[ne->m_mesh].getChildren();
            /* PC: i3rDrawMesh45 / i3rDrawMesh46 bone-box test each part
             * (clip 2 only) and return 0 when it is off screen; the host
             * counts a part with data as drawn. */
            if (parts[1].m_data != 0)
                drawn = 1;
            if (parts[0].m_data != 0)
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
                               ctx->m_tex_table, (int)ctx->m_tex_table_count);
        p[0] = p[1] = p[2] = p[3] = 0;
        i3dLayerEndPacket(layer, p + 4);
    }

    // callcheck: i3rDrawMesh45, i3rDrawMesh46, i3rDrawSetupBlend, sceDmaSync (the VU1 packet writers and their DMA wait: the native module draws the model, PORT: in the file header)
    // PC: sceDmaSync on fromSPR (0x1000d000): the packet writers' scratchpad
    // transfers; the host made none.
    return drawn ? I3R_DRAW_OK : I3R_DRAW_CULLED;
}
