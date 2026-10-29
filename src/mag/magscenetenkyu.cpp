/* ==========================================================================
 *  mag/magscenetenkyu.cpp
 *
 *  The scene's skies ("tenkyu", celestial sphere; RoR's
 *  "mag/magscenetenkyu.cpp", 0x13ff00..0x140990): a map pack's tenkyu.rpk
 *  -- an optional I3D_I3M motion and the sky's i3r -- loaded into a sky
 *  slot's m_sky with its textures in the scene's table, each node's y spin
 *  derived from the motion (its turn between time 0 and twice the length),
 *  turned once a frame and drawn behind everything at layer 490, centred
 *  under the camera.
 * ======================================================================== */
#include "mag/magscene.h"

#include <ctype.h>
#include <math.h>
#include <string.h>

#include "bed/bedpfh.h"
#include "i3d/i3dctx.h"
#include "i3d/i3denv.h"
#include "i3d/i3dlayer.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dview.h"
#include "i3d/i3mroot.h"
#include "i3d/i3rmodel.h"
#include "i3d/i3rroot.h"
#include "sdk/libgraph.h"
#include "utl/utlmisc.h"

/* PC: the ROM's libc strncasecmp (0x27a4e0); MSVC has no such name. */
static int tenkyuStrNCaseCmp(const char *a, const char *b, size_t n)
{
    for (; n != 0; n--, a++, b++)
    {
        int ca = tolower((u_char)*a);
        int cb = tolower((u_char)*b);
        if (ca != cb || ca == 0)
            return ca - cb;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Slot no's sky: its textures out of the scene's table, the model and the
// node buffers back to the game heap.
// callcheck: dbgAssertFailed (the assert handler inside ROR_ASSERT)
// ROR 0x0013ff00

void MagScene::releaseTenkyuTex(int no)
{
    MagSceneTenkyu *t = &m_tenkyu[no];
    if (t->m_sky)
    {
        int num = i3rGetNumTex(t->m_sky);
        for (int i = 0; i < num; i++)
        {
            const char *psz = i3rGetTexName(t->m_sky, i);
            ROR_ASSERT(psz!=NULL); /* 80 */
            int idx = m_tex_info.find(psz);
            if (idx >= 0)
                m_tex_info.release(idx);
        }
        g_utlMemory.free(t->m_sky);
        t->m_sky = NULL;
    }
    if (t->m_sky_mats)
    {
        g_utlMemory.free(t->m_sky_mats);
        t->m_sky_mats = NULL;
        t->m_sky_spin = NULL;
    }
    t->m_sky_num_node = 0;
}

// ──────────────────────────────────────────────────────────────────────
// The pack's tenkyu.rpk into slot no (none: nothing). The rpk's file 0 is
// the motion when it carries the I3D_I3M id, and the i3r follows it;
// the i3r is copied to the game heap, relocated, its textures added to
// the scene's table (sepia per m_sepia) and assigned. With a motion, one
// block holds the node matrices and the spins initTenkyuSpin fills.
// callcheck: dbgAssertFailed, BedPfh, strncasecmp (the assert handler inside the inline BedPfh::getFile; the local pack's constructor; the host's tenkyuStrNCaseCmp)
// ROR 0x00140030

void MagScene::loadTenkyuTex(int no, const BedPfh *pfh, i3dContext *ctx)
{
    int idx = pfh->getFileIndex("tenkyu.rpk");
    if (idx == -1)
        return;

    BedPfh rpk(pfh->getFile(idx));
    MagSceneTenkyu *t = &m_tenkyu[no];

    int n = 0;
    I3mRoot *motion = NULL;
    if (tenkyuStrNCaseCmp((const char *)rpk.getFile(0), "I3D_I3M", 7) == 0)
    {
        n = 1;
        motion = (I3mRoot *)rpk.getFile(0);
        i3mRelocate(motion);
    }

    int size = rpk.getFileSize(n);
    t->m_sky = (I3rRoot *)g_utlMemory.alloc(size, -1);
    memcpy(t->m_sky, rpk.getFile(n), size);
    i3rRelocate(t->m_sky);
    m_tex_info.addPfh(&rpk, t->m_sky, 1, m_sepia);
    m_tex_info.setTexTable(ctx);
    i3rAssignTex(t->m_sky, m_tex_info.m_tex_info, m_tex_info.m_num_tex);

    if (motion == NULL)
    {
        t->m_sky_spin = NULL;
        t->m_sky_num_node = 0;
        t->m_sky_mats = NULL;
        return;
    }

    t->m_sky_num_node = i3rGetNumNode(t->m_sky);
    /* PC: the ROM's block is num * 0x44 bytes, the 0x40-byte matrices then
     * the floats; sized from the host types. */
    t->m_sky_mats = (sceVu0FMATRIX *)g_utlMemory.alloc(
        t->m_sky_num_node * (int)(sizeof(sceVu0FMATRIX) + sizeof(float)), -1);
    t->m_sky_spin = (float *)(t->m_sky_mats + t->m_sky_num_node);
    initTenkyuSpin(t, motion);
}

// ──────────────────────────────────────────────────────────────────────
// Each sky node's spin: the model matrices at motion time 0 (kept in
// m_sky_mats, the pose drawn from) and at twice the motion's length; a
// node's spin is the angle between the two row-1 vectors spread over that
// time at 30 frames a second, negative when the turn goes the other way
// about y. One scratch block holds the pose, the local matrices, the end
// matrices and the motion-to-model node map (count first).
// callcheck: FUN_0026cad0 (acosf)
// ROR 0x00140390

void MagScene::initTenkyuSpin(MagSceneTenkyu *t, I3mRoot *motion)
{
    int num = t->m_sky_num_node;
    /* PC: the ROM's pose records are 0x30 bytes a node; host matrices. */
    u_char *buf = (u_char *)g_utlMemory.alloc(
        num * (int)(0x30 + 2 * sizeof(sceVu0FMATRIX) + sizeof(u_short)) + 2, -1);
    void *pose = buf;
    sceVu0FMATRIX *local = (sceVu0FMATRIX *)(buf + num * 0x30);
    sceVu0FMATRIX *end = local + num;
    u_short *map = (u_short *)(end + num);

    map[0] = (u_short)num;
    for (int i = 0; i < num; i++)
    {
        const char *name = i3mGetNodeName(motion, (u_short)i);
        int node = -1;
        if (name)
            node = i3rGetNodeIndexByName(t->m_sky, name);
        map[i + 1] = node < 0 ? 0xffff : (u_short)node;
    }

    i3rCopyLocalBasicTransform(t->m_sky, (I3dMatrixF4x4 *)local);
    i3mGetPose(motion, pose, 0x111, 0.0f);
    i3mPoseToMatrices(pose, local, map);
    i3rConvertLocalToModel(t->m_sky, (I3dMatrixF4x4 *)t->m_sky_mats, (const I3dMatrixF4x4 *)local);

    float len = i3mGetLength(motion);
    i3mGetPose(motion, pose, 0x111, len + len);
    i3mPoseToMatrices(pose, local, map);
    i3rConvertLocalToModel(t->m_sky, (I3dMatrixF4x4 *)end, (const I3dMatrixF4x4 *)local);

    for (int i = 0; i < num; i++)
    {
        alignas(16) float a[4];
        alignas(16) float b[4];
        alignas(16) float c[4];
        i3dVecNormalize2(a, t->m_sky_mats[i][1]);
        i3dVecNormalize2(b, end[i][1]);
        i3dVecCross2(c, a, b);
        float angle = acosf(i3dVecDot(a, b));
        float frames = (len + len) * 30.0f;
        if (c[1] < 0.0f)
            t->m_sky_spin[i] = -angle / frames;
        else
            t->m_sky_spin[i] = angle / frames;
    }

    g_utlMemory.free(buf);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014062c

void MagScene::rotateTenkyu()
{
    for (int no = 0; no < 4; no++)
    {
        MagSceneTenkyu *t = &m_tenkyu[no];
        for (int i = 0; i < t->m_sky_num_node; i++)
            i3dMatRotY(t->m_sky_spin[i], t->m_sky_mats[i], t->m_sky_mats[i]);
    }
}

// ──────────────────────────────────────────────────────────────────────
// Nothing without a sky. Layer 490 lit by a white ambient light alone,
// seen through the first view pushed out to 100000 with both depth bounds
// 0, the camera's matrix; alpha 0x44, test 0x13001. The skies are drawn at
// the camera's x / z, by their spun node matrices.
// callcheck: memset (the light description cleared)
// ROR 0x001406b8

void MagScene::drawSkyDome(i3dContext *ctx)
{
    MagSceneTenkyu *t = m_tenkyu;
    int i;
    for (i = 0; i < 4; i++, t++)
    {
        if (t->m_sky)
            break;
    }
    if (i == 4)
        return;

    I3dLayer *layer = i3dCtxCreateLayer(ctx, 490.0f);

    I3dLightDesc desc;
    memset(&desc, 0, sizeof(desc));
    desc.m_ambient[0] = 1.0f;
    desc.m_ambient[1] = 1.0f;
    desc.m_ambient[2] = 1.0f;
    i3dLayerSetLight(layer, i3dCtxCreateLight(ctx, &desc));

    MagViewParam view = m_view[0];
    view.m_far = 100000.0f;
    view.m_zmax = 0.0f;
    view.m_zmin = 0.0f;
    i3dLayerSetView(layer, i3dCtxCreateView(ctx, &view, m_camera_entity.getMatrix()));

    u_long *pkt = i3dLayerBeginPacket(layer);
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktSetReg(pkt, SCE_GS_FBA_1, 0);
    pkt = i3dPktSetReg(pkt, SCE_GS_CLAMP_1, 0);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x13001);
    i3dLayerEndPacket(layer, pkt);

    sceVu0FMATRIX world;
    i3dMatUnit(world);
    world[3][0] = m_camera_entity.m_trans[0];
    world[3][1] = 0.0f;
    world[3][2] = m_camera_entity.m_trans[2];

    /* The ROM draws four slots from the first one with a sky, past the
     * last slot when slot 0 is empty. PC: stop at the last slot. */
    for (; i < 4; i++, t++)
    {
        if (t->m_sky)
            utlDrawI3r(layer, t->m_sky, t->m_sky_mats, world, NULL);
    }
}
