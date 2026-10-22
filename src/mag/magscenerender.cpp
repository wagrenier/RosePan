/* ==========================================================================
 *  mag/magscenerender.cpp
 *
 *  The scene's frame (RoR's "mag/magscenerender.cpp", 0x139540..0x13c328):
 *  MagScene::draw builds the i3d layers of one frame -- eight render
 *  stages at depth 500, one per light set of the light pool, sharing the
 *  camera's view and the map's fog -- and fills them: the actors (sorted
 *  far to near, mirrored when the map has a mirror plane), the strip
 *  records, the sky dome and the sky models, the life objects, the blob
 *  shadows (800), the projected shadows (700 + 10 n / 705 + 10 n) or the
 *  close-up of one actor (1000), the 2D movie and the overlay life objects
 *  (900). postDraw runs the PC's and Brown's motion sequences afterwards.
 *
 *  PORT: the Dog classes are not polymorphic on the host yet
 *  (dog/dogentity.h); the vtable slots the ROM calls here are plain
 *  members, called on the class the object is known to be (actors are
 *  DogActors, the sky models DogModels, the camera a DogCamera; a parent
 *  or a root entity is taken as a DogModel -- the classes that have the
 *  slots).
 * ======================================================================== */
#include "mag/magscene.h"

#include <math.h>
#include <string.h>

#include "common/ror_assert.h"
#include "common/rosepan_stub.h"
#include "sdk/libgraph.h"

/* data 0x298c40: the close-up view (a 256 x 256 target); drawCloseUp
 * writes its frustum. */
static MagViewParam s_closeup_view = {
    5.0f, 65535.0f, 0.0f, 0.0f, 0.0f, 0.0f, 65535.0f, 5.0f, 0.0f, 255.0f, 0.0f, 255.0f, 0.0f, 0.0f,
};
/* data 0x298c78: drawTenkyuRotated's view. */
static MagViewParam s_tenkyu_view = {
    1.0f, 65535.0f, 0.0f, 0.0f, 0.0f, 0.0f, 65535.0f, 1.0f, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f,
};
/* data 0x298cb0: makeActorShadow's view (the 256 x 256 shadow target; it
 * writes the frustum) and 0x298cf0 its up vector. */
static MagViewParam s_shadow_view = {
    1.0f, 65530.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 255.0f, 0.0f, 255.0f, 0.0f, 0.0f,
};
static const sceVu0FVECTOR s_shadow_up = { 1.0f, -1.0f, 1.0f, 0.0f };
/* data 0x298d00: the blob shadow's quad (x / z set from the actor's clip
 * radius each time) and 0x298d40 its texture coordinates. */
static sceVu0FVECTOR s_blob_xyz[4] = {
    { -100.0f, -1.0f, 100.0f, 1.0f },
    { 100.0f, -1.0f, 100.0f, 1.0f },
    { -100.0f, -1.0f, -100.0f, 1.0f },
    { 100.0f, -1.0f, -100.0f, 1.0f },
};
static sceVu0FVECTOR s_blob_stq[4] = {
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 1.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 1.0f, 0.0f },
    { 1.0f, 1.0f, 1.0f, 0.0f },
};
/* rodata 0x2edfb0 / 0x2edfc0 / 0x2ee040 / 0x2ee190 / 0x2ee1c0 */
static const sceVu0FVECTOR s_closeup_ambient = { 0.7f, 0.7f, 0.7f, 1.0f };
static const sceVu0FVECTOR s_up = { 0.0f, 1.0f, 0.0f, 0.0f };
static const sceVu0FVECTOR s_shadow_ambient = { 0.0f, 0.0f, 1.0f, 0.0f };
static const sceVu0FVECTOR s_origin = { 0.0f, 0.0f, 0.0f, 1.0f };
static const sceVu0FVECTOR s_path_color = { 1.0f, 0.0f, 0.0f, 1.0f };

/* bss 0x6646f0: the sky paths' points with w = 1, 0x100 per record (the
 * ROM indexes by record k without a bound). PC: MAX_PATH_REC records;
 * drawStrips stops at the bound. */
#define MAX_PATH_REC 64
static sceVu0FVECTOR s_path_xyz[MAX_PATH_REC][0x100];

/* A sky record's path point i (the points follow the 0x30-byte header). */
static inline const float *pathPoint(const MagSceneCRec *rec, int i)
{
    return ((const sceVu0FVECTOR *)(rec + 1))[i];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00139540

void MagScene::releaseTexture(int no)
{
    m_tex_info.release(no);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00139568

void MagScene::copyTexture(const char *dst, const char *src)
{
    m_tex_info.copy(dst, src);
}

// ──────────────────────────────────────────────────────────────────────
// One packet that sends the double buffer's draw environment again (by
// reference: FLUSH, DIRECT 1 with an 8-register A+D GIF tag, then DIRECT
// 8 from draw0 or draw1), after a pass that drew into another target.
// ROR 0x00139590

void MagScene::putDrawEnv(I3dLayer *layer, sceGsDBuff *dbuff, int frame)
{
    u_long *pkt = i3dLayerBeginPacket(layer);
    pkt = i3dPktDmaTag(pkt, 1, 0x10, NULL, 0x11000000, 0x50000001);
    pkt[0] = 0x1000000000008008UL;  /* NLOOP 8, EOP, one A+D register */
    pkt[1] = 0xe;
    const void *env = &dbuff->draw0;
    if (frame & 1)
        env = &dbuff->draw1;
    pkt = i3dPktDmaTag(pkt + 2, 8, 0x30, env, 0, 0x50000008);
    i3dLayerEndPacket(layer, pkt);
}

// ──────────────────────────────────────────────────────────────────────
// What draw does this frame: 0 in sub mode 8, 0x200 (only the actors with
// flag bit 3) in sub mode 7, else 0x101 (the actors and the world).
// ROR 0x0013963c

u_int MagScene::getDrawFlags()
{
    if (m_sub_mode.m_mode == 8)
        return 0;
    if (m_sub_mode.m_mode == 7)
        return 0x200;
    return 0x101;
}

// ──────────────────────────────────────────────────────────────────────
// The close-up of one actor: a layer at 1000 drawn into a 256-wide target
// (FRAME fbp 0x118) over a dark sprite, lit by a 0.7 grey ambient, seen
// from four clip radii away along -z with a frustum that just holds the
// clip sphere.
// callcheck: FUN_00215a30, FUN_0026cb48, i3rCopyClipSphere (the assert handler inside ROR_ASSERT; sqrtf; inside the inline DogModel::copyClipSphere); drawModel is the vtable slot 19
// ROR 0x00139664

void MagScene::drawCloseUp(i3dContext *ctx, DogActor *actor, DogEntity *src)
{
    I3dLightDesc light;
    i3dLightDescInit(&light);
    i3dVecCopy(light.m_ambient, s_closeup_ambient);

    I3dLayer *pRenderStage = i3dCtxCreateLayer(ctx, 1000.0f);
    ROR_ASSERT(pRenderStage != NULL); /* 174 */

    u_long *pkt = i3dLayerBeginPacket(pRenderStage);
    pkt = i3dPktSetReg(pkt, SCE_GS_FRAME_1, 0x40118);
    pkt = i3dPktSetReg(pkt, SCE_GS_ZBUF_1, 0x3a000200);
    pkt = i3dPktSetReg(pkt, SCE_GS_XYOFFSET_1, 0);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x5000d);
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0);
    pkt = i3dPktSprite(pkt, i3dPackXYZ(0, 0, 0), i3dPackXYZ(0x1000, 0x1000, 0),
                       i3dPackRGBAQ(1.0f, 0, 0, 0, 0x80));
    i3dLayerEndPacket(pRenderStage, pkt);

    sceVu0FVECTOR center;
    float radius[4];
    actor->copyClipSphere(center, radius);
    float dist = radius[0] * 4.0f;
    sceVu0FVECTOR ofs = { 0.0f, 0.0f, -dist, 1.0f };
    sceVu0FVECTOR eye;
    sceVu0FVECTOR dir;
    sceVu0FMATRIX view;
    i3dVecCopy(eye, ofs);
    i3dVecSub(dir, center, eye);
    i3dMatLookDir(view, eye, dir, s_up);

    float s = radius[0] / dist;
    s = s / sqrtf(1.0f - s * s);
    s_closeup_view.m_right = s * s_closeup_view.m_near;
    s_closeup_view.m_left = -s * s_closeup_view.m_near;
    s_closeup_view.m_bottom = s_closeup_view.m_left;
    s_closeup_view.m_top = s_closeup_view.m_right;

    i3dLayerSetLight(pRenderStage, i3dCtxCreateLight(ctx, &light));
    i3dLayerSetView(pRenderStage, i3dCtxCreateView(ctx, &s_closeup_view, view));
    actor->drawModel(pRenderStage, 1, src);
}

// ──────────────────────────────────────────────────────────────────────
// The sky models in a layer at 1000 seen through an entity's transform
// (rotation z, x, y then the translation, inverted). Nothing calls it.
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT)
// ROR 0x00139920

void MagScene::drawTenkyuRotated(i3dContext *ctx, I3dLight *light, const float *trans,
                                 const float *rot)
{
    I3dLayer *pRenderStage = i3dCtxCreateLayer(ctx, 1000.0f);
    ROR_ASSERT(pRenderStage != NULL); /* 295 */

    u_long *pkt = i3dLayerBeginPacket(pRenderStage);
    pkt = i3dPktSetReg(pkt, SCE_GS_FRAME_1, 0x200);
    pkt = i3dPktSetReg(pkt, SCE_GS_ZBUF_1, 0x3a000200);
    pkt = i3dPktSetReg(pkt, SCE_GS_XYOFFSET_1, 0);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x30000);
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0);
    pkt = i3dPktSprite(pkt, i3dPackXYZ(0, 0, 0), i3dPackXYZ(0, 0, 0),
                       i3dPackRGBAQ(1.0f, 0, 0, 0, 0x80));
    pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x70000);
    i3dLayerEndPacket(pRenderStage, pkt);
    i3dLayerSetLight(pRenderStage, light);

    sceVu0FMATRIX m;
    i3dMatUnit(m);
    i3dMatRotZ(rot[2], m, m);
    i3dMatRotX(rot[0], m, m);
    i3dMatRotY(rot[1], m, m);
    i3dMatTranslate(m, m, trans);
    i3dMatInverse(m, m);

    s_tenkyu_view.m_right = s_tenkyu_view.m_near * 0.5f;
    s_tenkyu_view.m_left = s_tenkyu_view.m_near * -0.5f;
    s_tenkyu_view.m_bottom = s_tenkyu_view.m_left;
    s_tenkyu_view.m_top = s_tenkyu_view.m_right;
    i3dLayerSetView(pRenderStage, i3dCtxCreateView(ctx, &s_tenkyu_view, m));
    drawTenkyu(pRenderStage);
}

// ──────────────────────────────────────────────────────────────────────
// One actor's shadow texture, seen from its light: a layer at 700 + 10 n
// drawn into the 256 x 256 target at fbp 0x118 -- cleared to black with
// alpha forced on, the actor's shadow model drawn by an ambient-only light,
// then a black border so the projection clamps to unlit. The light is the
// scene's directional light (mode 1: from ten clip radii back along its
// direction) or the actor's nearest point light (on a change, 30 frames
// fading the old one out and 30 the new one in; kept at least three clip
// radii from the actor and never above y 0). The view just holds the clip
// sphere; stq gets the projection (texture matrix x view), and the scene's
// shadow light record the light the layers at 705 + 10 n are lit by.
// callcheck: FUN_00215a30, FUN_0026cb48, i3rCopyClipSphere (the assert handler inside ROR_ASSERT; sqrtf; inside the inline DogModel::copyClipSphere); transform / drawShadow are vtable slots 2 / 20
// ROR 0x00139b4c

void MagScene::makeActorShadow(int theCount, i3dContext *ctx, DogActor *actor, I3rStqMapEntity *stq)
{
    ROR_ASSERT(theCount >= 0 && theCount < 10); /* 466 */

    I3dLightDesc light;
    i3dLightDescInit(&light);
    i3dVecCopy2(light.m_ambient, s_shadow_ambient);

    I3dLayer *pRenderStage = i3dCtxCreateLayer(ctx, (float)theCount * 10.0f + 700.0f);
    ROR_ASSERT(pRenderStage != NULL); /* 498 */

    u_long *pkt = i3dLayerBeginPacket(pRenderStage);
    pkt = i3dPktSetReg(pkt, SCE_GS_FRAME_1, 0x40118);
    pkt = i3dPktSetReg(pkt, SCE_GS_XYOFFSET_1, 0);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x31001);
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0);
    pkt = i3dPktSprite(pkt, i3dPackXYZ(0, 0, 0), i3dPackXYZ(0x1000, 0x1000, 0),
                       i3dPackRGBAQ(1.0f, 0, 0, 0, 0));
    pkt = i3dPktSetReg(pkt, SCE_GS_FBA_1, 1);
    i3dLayerEndPacket(pRenderStage, pkt);

    pkt = i3dLayerBeginPostPacket(pRenderStage);
    pkt = i3dPktSetReg(pkt, SCE_GS_FBA_1, 0);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x31001);
    pkt = i3dPktLine(pkt, i3dPackXYZ(0, 0, 0), i3dPackXYZ(0, 0xff0, 0),
                     i3dPackRGBAQ(1.0f, 0, 0, 0, 0));
    pkt = i3dPktLine(pkt, i3dPackXYZ(0, 0xff0, 0), i3dPackXYZ(0xff0, 0xff0, 0),
                     i3dPackRGBAQ(1.0f, 0, 0, 0, 0));
    pkt = i3dPktLine(pkt, i3dPackXYZ(0xff0, 0xff0, 0), i3dPackXYZ(0xff0, 0, 0),
                     i3dPackRGBAQ(1.0f, 0, 0, 0, 0));
    pkt = i3dPktLine(pkt, i3dPackXYZ(0xff0, 0, 0), i3dPackXYZ(0, 0, 0),
                     i3dPackRGBAQ(1.0f, 0, 0, 0, 0));
    i3dLayerEndPostPacket(pRenderStage, pkt);

    if (m_shadow_mode < 0)
        return;

    sceVu0FVECTOR center;
    float radius[4];
    actor->copyClipSphere(center, radius);

    sceVu0FVECTOR dir;      /* from the light to the actor */
    sceVu0FVECTOR eye;      /* the light */
    sceVu0FVECTOR world;
    sceVu0FVECTOR target;   /* the clip sphere's centre less the actor's offset */
    sceVu0FVECTOR v;
    float dist;
    float fade = 1.0f;
    if (m_shadow_mode == 1)
    {
        i3dVecNeg(dir, m_shadow_src);
        actor->transform();
        i3dMatApply(world, actor->m_matrix, center);
        i3dVecSub(target, world, actor->m_trans_ofs);
        target[3] = 1.0f;
        dist = radius[0] * 10.0f;
        i3dVecScale(dist, v, dir);
        i3dVecSub(eye, target, v);
    }
    else
    {
        int frame = actor->m_shadow_fade;
        actor->m_shadow_fade = frame + 1;
        int no;
        if ((u_int)frame < 60)
        {
            if (frame < 30)
            {
                fade = (float)(30 - frame) / 30.0f;
                no = actor->m_shadow_light_prev;
            }
            else
            {
                fade = (float)(frame - 30) / 30.0f;
                no = actor->m_shadow_light;
            }
        }
        else
        {
            v[0] = actor->m_trans[0];
            v[1] = actor->m_trans[1];
            v[2] = actor->m_trans[2];
            v[3] = actor->m_trans[3];
            int nearest = m_light_pool.findNearestPointLight(actor->getLightID(), v);
            if (nearest < 0)
                return;
            no = actor->m_shadow_light;
            if (nearest != no)
            {
                /* the old light this frame; the fade starts with the next */
                actor->m_shadow_light = nearest;
                actor->m_shadow_light_prev = no;
                actor->m_shadow_fade = 0;
                if (frame < 0)
                    actor->m_shadow_fade = 60;
            }
        }
        if (no < 0)
            return;

        DogPointLight *plight = m_light_pool.getPointLight(no);
        ROR_ASSERT(plight); /* 1095 */
        if (plight == NULL)  /* PC: the ROM reads through it */
            return;
        i3dVecCopy(eye, plight->m_pos);

        actor->transform();
        i3dMatApply(world, actor->m_matrix, center);
        i3dVecSub(target, world, actor->m_trans_ofs);
        target[3] = 1.0f;
        i3dVecSub(dir, target, eye);
        dist = sqrtf(i3dVecDot(dir, dir));
        if (dist < radius[0] * 3.0f)
        {
            i3dVecScale(radius[0] * 3.0f / dist, v, dir);
            i3dVecSub(eye, target, v);
            if (eye[1] > 0.0f)
                eye[1] = 0.0f;
            dist = radius[0] * 3.0f;
        }
    }

    sceVu0FMATRIX view;
    i3dMatLookDir(view, eye, dir, s_shadow_up);

    /* The frustum holds the clip sphere (t: the tangent of its half
     * angle); the texture matrix maps the view onto 0..1 with s = x + t z,
     * t = y + t z, q = 2 t z. */
    float s = radius[0] / dist;
    float t = s / sqrtf(1.0f - s * s);
    float near_z = s_shadow_view.m_near;
    sceVu0FMATRIX tex;
    memset(tex, 0, sizeof(tex));
    tex[0][0] = 1.0f;
    tex[1][1] = 1.0f;
    tex[2][0] = t;
    tex[2][1] = t;
    tex[2][2] = t + t;
    tex[3][3] = 1.0f;
    s_shadow_view.m_left = -t * near_z;
    s_shadow_view.m_right = t * near_z;
    s_shadow_view.m_bottom = -t * near_z;
    s_shadow_view.m_top = t * near_z;

    i3dLayerSetLight(pRenderStage, i3dCtxCreateLight(ctx, &light));
    i3dLayerSetView(pRenderStage, i3dCtxCreateView(ctx, &s_shadow_view, view));
    actor->drawShadow(pRenderStage);
    i3dMatMul((sceVu0FVECTOR *)stq, tex, view);

    sceVu0FVECTOR color = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (m_shadow_mode == 1)
    {
        color[3] = m_22a0 * m_2a9cc;
        i3dVecCopy(m_shadow_lightrec.m_00, m_shadow_src);
        i3dVecCopy2(m_shadow_lightrec.m_10, color);
    }
    else
    {
        color[3] = (1000000.0f / dist) * m_22a0 * fade * m_2a9cc;
        i3dVecCopy(m_shadow_lightrec.m_00, eye);
        i3dVecCopy(m_shadow_lightrec.m_10, dir);
        i3dVecCopy2(m_shadow_lightrec.m_color, color);
        if (t > 0.7f)
            t = 0.7f;
        m_shadow_lightrec.m_cone_outer = t;
        float inner = t - 0.1f;
        if (inner < 0.0f)
            inner = 0.0f;
        m_shadow_lightrec.m_cone_inner = inner;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The focused actor close up, then a layer that restores the draw
// environment.
// callcheck: FUN_00215a30, I3rRoot__CheckIdString (the assert handler inside ROR_ASSERT; inside the inline DogModel::isAlive)
// ROR 0x0013a464

void MagScene::drawFocusActor(i3dContext *ctx, sceGsDBuff *dbuff, int frame)
{
    if (m_focus_actor < 0)
        return;
    DogActor *actor = &m_actors[m_focus_actor];
    if (!actor->isAlive())
        return;

    drawCloseUp(ctx, actor, NULL);
    I3dLayer *renderstage = i3dCtxCreateLayer(ctx, 1000.0f);
    ROR_ASSERT(NULL != renderstage); /* 1211 */
    putDrawEnv(renderstage, dbuff, frame);
}

// ──────────────────────────────────────────────────────────────────────
// A textured quad (r_shadow) under every shown actor whose flags are 0x20
// of 0x60: centred under Bip01 (or the actor's translation), sized by the
// clip radius, dropped onto the ground castRay finds below.
// callcheck: FUN_00215a30, I3rRoot__CheckIdString, i3rGetNodeIndexByName (the assert handler inside ROR_ASSERT; inside the inline DogModel::isAlive; inside the inline DogModel::getNodeIndexByName); isHide / getNodeWorldPos are vtable slots 9 / 18
// ROR 0x0013a57c

void MagScene::drawBlobShadows(I3dLayer *layer)
{
    I3dTexSetDesc desc = { 1.0f, &m_common_tex_no[0], 1, 0, 0, 0 };
    I3dDrawSet *set = i3dLayerFindDrawSet(layer, &desc);
    i3dLayerSetDrawSet(layer, set);
    u_long *pkt = i3dLayerBeginSet(layer, 0.0f);
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(set, 0));

    for (int i = 0; i < 128; i++)
    {
        DogActor *actor = &m_actors[i];
        if (!actor->isAlive())
            continue;
        if (actor->isHide() || (m_actor_flags[i] & 0x60) != 0x20)
            continue;

        int node = actor->getNodeIndexByName("Bip01");
        sceVu0FVECTOR pos = { 0.0f, 0.0f, 0.0f, 1.0f };
        if (node < 0)
        {
            pos[2] = actor->m_trans[2];
            pos[0] = actor->m_trans[0];
            pos[1] = actor->m_trans[1];
        }
        else
        {
            actor->getNodeWorldPos(pos, node);
        }

        sceVu0FVECTOR dir = { 0.0f, 1.0f, 0.0f, 1.0f };
        MagSceneRayHit hit;
        for (int k = 0; k < 5; k++)
        {
            hit.m_vec[k][0] = 0.0f;
            hit.m_vec[k][1] = 0.0f;
            hit.m_vec[k][2] = 0.0f;
            hit.m_vec[k][3] = 1.0f;
        }
        hit.m_50 = -1;
        float d = castRay(&hit, pos, dir, -1);
        if (d >= 0.0f)
            pos[1] += d;
        else
            pos[1] = 0.0f;
        pos[1] -= 1.0f;

        float r = actor->getClipRadius();
        float h = r * 0.5f;
        float l = -r * 0.5f;
        s_blob_xyz[0][2] = h;
        s_blob_xyz[0][0] = l;
        s_blob_xyz[1][0] = h;
        s_blob_xyz[1][2] = h;
        s_blob_xyz[2][0] = l;
        s_blob_xyz[2][2] = l;
        s_blob_xyz[3][0] = h;
        s_blob_xyz[3][2] = l;

        sceVu0FMATRIX m;
        i3dMatUnit(m);
        i3dMatTranslate(m, m, pos);
        pkt = i3dPktDrawStrip(pkt, m, 4, s_blob_stq, NULL, s_blob_xyz, 0);
    }
    i3dLayerEndSet(layer, pkt);
}

// ──────────────────────────────────────────────────────────────────────
// The projected shadows of the shown actors whose flags are 0x60 of 0x60
// (at most ten: drawActorShadow asserts the count), through the second
// view with its depth range from m_22a4 / m_22b0.
// callcheck: FUN_00215a30, I3rRoot__CheckIdString (the assert handler inside ROR_ASSERT; inside the inline DogModel::isAlive); isHide is vtable slot 9
// ROR 0x0013a924

void MagScene::drawShadows(i3dContext *ctx, sceGsDBuff *dbuff, int frame)
{
    int count = 0;
    for (int i = 0; i < 128; i++)
    {
        DogActor *actor = &m_actors[i];
        if (!actor->isAlive() || actor->isHide() || (m_actor_flags[i] & 0x60) != 0x60)
            continue;

        float z = m_22a4;
        if (m_22b0 != 1.0f)
            z = (z - 1.0f) * m_22b0 + 1.0f;
        m_view[1].m_zmin = z;
        m_view[1].m_zmax = z * 65530.0f;

        I3dView *vs = i3dCtxCreateView(ctx, &m_view[1], m_camera_entity.getMatrix());
        ROR_ASSERT(vs != NULL); /* 1415 */
        drawActorShadow(i, count, ctx, dbuff, frame, vs);
        count++;
    }
}

// ──────────────────────────────────────────────────────────────────────
// One actor's shadow: makeActorShadow renders it from the light and sets
// the STQ map, then a layer at 705 + 10 n draws the sky's second models
// (the ones with the STQ map) with the shadow light.
// callcheck: FUN_00215a30, I3rRoot__CheckIdString (the assert handler inside ROR_ASSERT; inside the inline DogModel::isAlive)
// ROR 0x0013ab30

void MagScene::drawActorShadow(int theChara, int theCount, i3dContext *ctx, sceGsDBuff *dbuff,
                               int frame, I3dView *view)
{
    ROR_ASSERT(theChara >= 0 && theChara < 128); /* 1426 */
    ROR_ASSERT(theCount >= 0 && theCount < 10); /* 1427 */
    if (theChara < 0)
        return;

    makeActorShadow(theCount, ctx, &m_actors[theChara], &m_stq_map);
    I3dLayer *renderstage = i3dCtxCreateLayer(ctx, (float)theCount * 10.0f + 705.0f);
    ROR_ASSERT(renderstage != NULL); /* 1442 */
    putDrawEnv(renderstage, dbuff, frame);
    i3dLayerSetLight(renderstage, i3dCtxCreateLight(ctx, &m_shadow_light));
    i3dLayerSetView(renderstage, view);

    for (int i = 0; i < 4; i++)
    {
        DogModel *model = &m_tenkyu[i].m_model[1];
        if (!model->isAlive())
            continue;
        int r = i3rUpdateTexAnim(model->m_i3r_root);
        ROR_ASSERT(r == I3D_OK); /* 1530 */
        model->drawUnitMatrix(renderstage);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The strip records by type (0..2 into one packet, 3..5 through their own
// i3d calls), counting their lives down; then, when m_22b4 is set, every
// sky record's path as a line strip (m_22b4 1 copies the points first and
// becomes 2).
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT)
// ROR 0x0013ad24

void MagScene::drawStrips(I3dLayer *layer)
{
    bool any = false;
    for (int i = 0; i < 0x2000; i++)
    {
        if (m_strip[i].m_type != -1)
        {
            any = true;
            break;
        }
    }

    if (any)
    {
        sceVu0FMATRIX m;
        i3dMatUnit(m);
        i3dLayerSetDrawSet(layer, i3dLayerGetDrawSet(layer));
        u_long *pkt = i3dLayerBeginSet(layer, 0.0f);
        for (int i = 0; i < 0x2000; i++)
        {
            MagSceneStrip *st = &m_strip[i];
            switch (st->m_type)
            {
            case 0:
                ROR_ASSERT(m_strip[i].m_count > 0); /* 1578 */
                pkt = i3dPktDrawStrip(pkt, m, st->m_count, NULL, st->m_colors, st->m_data, 0);
                break;
            case 1:
                ROR_ASSERT(m_strip[i].m_count > 0); /* 1590 */
                pkt = i3dPktDrawStripCtx(pkt, layer, m, st->m_count, NULL, st->m_colors,
                                         st->m_data, 0);
                break;
            case 2:
                ROR_ASSERT(m_strip[i].m_count > 0); /* 1602 */
                pkt = i3dPktDrawLineStrip(pkt, layer, m, st->m_count, st->m_color, st->m_data, 0);
                break;
            }
            if (st->m_life > 0)
            {
                st->m_life--;
                if (st->m_life < 1)
                    clearStrip(i);
            }
        }
        i3dLayerEndSet(layer, pkt);

        for (int i = 0; i < 0x2000; i++)
        {
            MagSceneStrip *st = &m_strip[i];
            switch (st->m_type)
            {
            case 4:
            {
                sceVu0FVECTOR *v = (sceVu0FVECTOR *)st->m_data;
                i3dLayerDrawLine(layer, v[0], v[1], st);
                break;
            }
            case 3:
                i3dLayerDrawWireBox(layer, (sceVu0FVECTOR *)st->m_data2, s_origin, st->m_data, st);
                break;
            case 5:
                i3dLayerDrawWireSphere(layer, (sceVu0FVECTOR *)st->m_data2, s_origin, st->m_color[0]);
                break;
            }
        }
    }

    if (m_22b4 == 0)
        return;

    i3dLayerSetDrawSet(layer, i3dLayerGetDrawSet(layer));
    u_long *pkt = i3dLayerBeginSet(layer, 0.0f);
    for (int t = 0; t < 4; t++)
    {
        MagSceneTenkyu *pmd = &m_tenkyu[t];
        if (pmd->m_id < 0 || pmd->m_c_data == NULL)
            continue;
        for (int k = 0; k < pmd->m_c_num; k++)
        {
            ROR_ASSERT(pmd->m_path_data[k]); /* 1684 */
            const MagSceneCRec *rec = pmd->m_path_data[k];
            if (rec->m_00 < 2)
                continue;
            if (k >= MAX_PATH_REC)  /* PC: the ROM's buffer has no bound */
                break;
            if (m_22b4 == 1)
            {
                for (int j = 0; j < rec->m_00; j++)
                {
                    const float *p = pathPoint(rec, j);
                    s_path_xyz[k][j][0] = p[0];
                    s_path_xyz[k][j][1] = p[1];
                    s_path_xyz[k][j][2] = p[2];
                    s_path_xyz[k][j][3] = 1.0f;
                }
            }
            pkt = i3dPktDrawLineStrip(pkt, layer, NULL, rec->m_00, s_path_color, s_path_xyz[k], 0);
        }
    }
    i3dLayerEndSet(layer, pkt);
    if (m_22b4 == 1)
        m_22b4 = 2;
}

// ──────────────────────────────────────────────────────────────────────
// The four sky slots' first models (no camera cull).
// callcheck: FUN_00215a30, I3rRoot__CheckIdString (the assert handler inside ROR_ASSERT; inside the inline DogModel::isAlive)
// ROR 0x0013b20c

void MagScene::drawTenkyu(I3dLayer *layer)
{
    for (int i = 0; i < 4; i++)
    {
        DogModel *model = &m_tenkyu[i].m_model[0];
        if (model->isAlive())
            model->draw(layer, NULL, NULL);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The draw list: every live, shown actor whose flag bit 3 is flag3 (an
// actor whose parent is hidden is hidden too), keyed for sortActors --
// 0x10000 when it hangs from another actor, plus 65535 - its distance to
// the camera; the PC's key is 0. Returns the entries; *parented gets the
// parented ones.
// callcheck: FUN_00215a30, I3rRoot__CheckIdString (the assert handler inside ROR_ASSERT; inside the inline DogModel::isAlive); getParent / isHide / setHide are vtable slots 4 / 9 / 8
// ROR 0x0013b2ac

int MagScene::collectActors(int *parented, MagSceneDrawEntry *theArray, int theMax, int flag3)
{
    ROR_ASSERT(theArray); /* 1766 */
    ROR_ASSERT(128 <= theMax); /* 1767 */

    int n = 0;
    int num_parented = 0;
    int want = flag3 != 0;
    for (int i = 0; i < 128; i++)
    {
        DogActor *actor = &m_actors[i];
        if (!actor->isAlive())
            continue;
        if ((int)((m_actor_flags[i] >> 3) & 1) != want)
            continue;

        DogEntityEx *parent = actor->getParent();
        if (parent != NULL && static_cast<DogModel *>(parent)->isHide())
            actor->setHide(1);
        if (actor->isHide())
            continue;

        MagSceneDrawEntry *e = &theArray[n];
        if (parent == NULL)
        {
            e->m_parent_no = -1;
            e->m_key = 0;
        }
        else
        {
            e->m_key = 0x10000;
            num_parented++;
            e->m_parent_no = (int)(static_cast<DogActor *>(parent) - m_actors);
        }
        if (i == m_pc_no)
        {
            e->m_key = 0;
        }
        else
        {
            sceVu0FVECTOR eye;
            eye[0] = m_camera_entity.m_trans[0];
            eye[1] = m_camera_entity.m_trans[1];
            eye[2] = m_camera_entity.m_trans[2];
            eye[3] = m_camera_entity.m_trans[3];
            float d = utlVecDist(eye, actor->m_trans);
            if (d > 65535.0f)
                d = 65535.0f;
            if (d < 0.0f)
                d = 0.0f;
            e->m_key |= (int)(65535.0f - d) & 0xffff;
        }
        e->m_actor = actor;
        e->m_no = i;
        n++;
    }
    *parented = num_parented;
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// Bubble sort, ascending key: the far unparented actors first, the
// parented ones (drawn after their parents) last.
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT)
// ROR 0x0013b580

void MagScene::sortActors(MagSceneDrawEntry *theArray, int theMax)
{
    ROR_ASSERT(theArray); /* 1848 */
    ROR_ASSERT(128 >= theMax); /* 1849 */

    for (int i = 0; i < theMax - 1; i++)
    {
        for (int j = theMax - 1; j > i; j--)
        {
            if (theArray[j].m_key < theArray[j - 1].m_key)
            {
                MagSceneDrawEntry t = theArray[j - 1];
                theArray[j - 1] = theArray[j];
                theArray[j] = t;
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// The draw list into the render stages of the actors' light sets: first
// the mirror images (when the map has a mirror and this is not the flag-3
// pass), then the actors.
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT); draw is vtable slot 12
// ROR 0x0013b650

void MagScene::drawActors(MagSceneDrawEntry *theArray, int theMaxArray, I3dLayer **theRenderStage,
                          int theMaxStage, int flag3)
{
    ROR_ASSERT(theArray); /* 1886 */
    ROR_ASSERT(theMaxArray > 0); /* 1887 */
    ROR_ASSERT(theRenderStage); /* 1888 */
    ROR_ASSERT(theMaxStage > 0); /* 1889 */

    if (flag3 == 0 && m_mirror_axis != 0)
    {
        for (int i = 0; i < theMaxArray; i++)
        {
            DogActor *pactor = theArray[i].m_actor;
            ROR_ASSERT(pactor); /* 1904 */
            int lightsetno = pactor->getLightID();
            ROR_ASSERT(lightsetno < theMaxStage); /* 1907 */
            drawActorMirror(pactor, theRenderStage[lightsetno]);
        }
    }

    for (int i = 0; i < theMaxArray; i++)
    {
        DogActor *pactor = theArray[i].m_actor;
        ROR_ASSERT(pactor); /* 1921 */
        int lightsetno = pactor->getLightID();
        ROR_ASSERT(lightsetno < theMaxStage); /* 1925 */
        pactor->draw(theRenderStage[lightsetno], m_camera_entity.m_rotate, m_camera_entity.m_trans);
    }
}

// ──────────────────────────────────────────────────────────────────────
// An actor's mirror image when the camera is on the actor's side of the
// mirror plane: the root entity is reflected through the plane (its
// translation, its scale on the mirror axis negated; on a wall mirror its
// yaw too; on the floor mirror the world matrix's height), drawn, and put
// back.
// getParent, transform and draw are the vtable slots 4, 2 and 12.
// ROR 0x0013b870

void MagScene::drawActorMirror(DogActor *actor, I3dLayer *stage)
{
    static const int axis_of_mode[4] = { 0, 2, 1, 0 };

    sceVu0FVECTOR trans;
    trans[0] = actor->m_trans[0];
    trans[1] = actor->m_trans[1];
    trans[2] = actor->m_trans[2];
    int axis = axis_of_mode[m_mirror_axis];

    m_camera_entity.transform();
    float plane = m_mirror_plane;
    float cam = m_camera_entity.m_matrix[3][axis];
    bool apart;
    if (plane < trans[axis])
        apart = cam < plane;
    else
        apart = plane < cam;
    if (apart)
        return;

    DogEntityEx *root = actor;
    while (root->getParent() != NULL)
        root = root->getParent();

    sceVu0FVECTOR rot;
    sceVu0FVECTOR scale;
    trans[0] = root->m_trans[0];
    trans[1] = root->m_trans[1];
    trans[2] = root->m_trans[2];
    rot[0] = root->m_rotate[0];
    rot[1] = root->m_rotate[1];
    rot[2] = root->m_rotate[2];
    scale[0] = root->m_scale[0];
    scale[1] = root->m_scale[1];
    scale[2] = root->m_scale[2];

    if (m_mirror_axis == 2)
    {
        static_cast<DogModel *>(root)->transform();
        float save_t = trans[axis];
        float d = root->m_matrix[3][axis] - m_mirror_plane;
        trans[axis] = trans[axis] - (d + d);
        root->m_trans[0] = trans[0];
        root->m_trans[1] = trans[1];
        root->m_dirty = 1;
        root->m_trans[2] = trans[2];
        float save_s = scale[axis];
        scale[axis] = -save_s;
        root->m_scale[0] = scale[0];
        root->m_scale[1] = scale[1];
        root->m_dirty = 1;
        root->m_scale[2] = scale[2];

        actor->draw(stage, m_camera_entity.m_rotate, m_camera_entity.m_trans);

        trans[axis] = save_t;
        root->m_trans[0] = trans[0];
        root->m_trans[1] = trans[1];
        root->m_dirty = 1;
        root->m_trans[2] = trans[2];
        scale[axis] = save_s;
        root->m_scale[0] = scale[0];
        root->m_scale[1] = scale[1];
        root->m_dirty = 1;
        root->m_scale[2] = scale[2];
    }
    else
    {
        float save_t = trans[axis];
        float d = trans[axis] - m_mirror_plane;
        trans[axis] = trans[axis] - (d + d);
        root->m_trans[0] = trans[0];
        root->m_trans[1] = trans[1];
        root->m_dirty = 1;
        root->m_trans[2] = trans[2];
        float yaw = rot[1];
        root->m_rotate[0] = rot[0];
        root->m_rotate[1] = -yaw;
        root->m_rotate[2] = rot[2];
        root->normalizeRotate();
        root->m_dirty = 1;
        float save_s = scale[axis];
        scale[axis] = -save_s;
        root->m_scale[0] = scale[0];
        root->m_scale[1] = scale[1];
        root->m_dirty = 1;
        root->m_scale[2] = scale[2];

        actor->draw(stage, m_camera_entity.m_rotate, m_camera_entity.m_trans);

        trans[axis] = save_t;
        root->m_trans[0] = trans[0];
        root->m_trans[1] = trans[1];
        root->m_dirty = 1;
        root->m_trans[2] = trans[2];
        root->m_rotate[0] = rot[0];
        root->m_rotate[1] = yaw;
        root->m_rotate[2] = rot[2];
        root->normalizeRotate();
        root->m_dirty = 1;
        scale[axis] = save_s;
        root->m_scale[0] = scale[0];
        root->m_scale[1] = scale[1];
        root->m_dirty = 1;
        root->m_scale[2] = scale[2];
    }
}

// ──────────────────────────────────────────────────────────────────────
// One frame of the scene into the i3d context (see the file comment).
// dbuff and frame pick the draw environment the full-screen passes put
// back.
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT)
// ROR 0x0013bcc0

void MagScene::draw(i3dContext *ctx, sceGsDBuff *dbuff, int frame)
{
    I3dLayer *renderstage[8];
    I3dLight *lightstatus[8];
    MagSceneDrawEntry entries[128];
    int parented;

    if (m_tex_info.m_dirty)
        m_tex_info.setTexTable(ctx);

    if (m_2aa80 != m_camera_entity.m_angle)
    {
        m_2aa80 = m_camera_entity.m_angle;
        float t = tanf(m_camera_entity.getLensFov() * 0.5f);
        float w = t * -0.5f * m_view[0].m_near;
        float h = w * 0.75f;
        m_view[1].m_left = w;
        m_view[0].m_left = w;
        m_view[1].m_bottom = h;
        m_view[0].m_right = -w;
        m_view[1].m_right = -w;
        m_view[0].m_bottom = h;
        m_view[0].m_top = -h;
        m_view[1].m_top = -h;
    }

    I3dView *pViewStatus = i3dCtxCreateView(ctx, &m_view[0], m_camera_entity.getMatrix());
    ROR_ASSERT(pViewStatus != NULL); /* 2249 */
    void *pFogStatus = NULL;
    if (m_2aa84)
    {
        pFogStatus = i3dCtxSetFog(ctx, &m_fog[0]);
        ROR_ASSERT(pFogStatus != NULL); /* 2257 */
    }

    for (int i = 0; i < 8; i++)
    {
        float scale = ((u_int)(i - 1) < 2) ? m_22a8 : 1.0f;
        lightstatus[i] = m_light_pool.createLight(ctx, i, m_22cc, scale);
        ROR_ASSERT(lightstatus[i] != NULL); /* 2282 */
        renderstage[i] = i3dCtxCreateLayer(ctx, 500.0f);
        ROR_ASSERT(renderstage[i] != NULL); /* 2286 */
        i3dLayerSetLight(renderstage[i], lightstatus[i]);
        i3dLayerSetView(renderstage[i], pViewStatus);
        if (pFogStatus)
            i3dLayerSetFog(renderstage[i], pFogStatus);
        u_long *pkt = i3dLayerBeginPacket(renderstage[i]);
        pkt = i3dPktSetReg(pkt, SCE_GS_FBA_1, 0);
        pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0);
        pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x5001d);
        pkt = i3dPktSetReg(pkt, SCE_GS_TEXA, 0x8000000000UL);
        pkt = i3dPktSetReg(pkt, SCE_GS_PABE, 0);
        i3dLayerEndPacket(renderstage[i], pkt);
    }

    u_int flags = getDrawFlags();
    if (flags & 0xff00)
    {
        int flag3 = (flags >> 9) & 1;
        int n = collectActors(&parented, entries, 128, flag3);
        if (n > 0)
        {
            if (parented > 0)
                sortActors(entries, n);
            drawActors(entries, n, renderstage, 8, flag3);
        }
    }

    if (flags & 1)
    {
        drawStrips(renderstage[0]);
        drawSkyDome(ctx);
        drawTenkyu(renderstage[0]);
        m_life_man.draw(renderstage[0], 0);
        m_life_man.draw(renderstage[0], 0x4000);
        if (m_2a9c8 & 2)
        {
            I3dLayer *pRenderStage = i3dCtxCreateLayer(ctx, 800.0f);
            I3dLight *pLightStatus = lightstatus[0];
            ROR_ASSERT(pLightStatus != NULL); /* 2467 */
            i3dLayerSetLight(pRenderStage, pLightStatus);
            if (pFogStatus)
                i3dLayerSetFog(pRenderStage, pFogStatus);
            ROR_ASSERT(pViewStatus != NULL); /* 2476 */
            i3dLayerSetView(pRenderStage, pViewStatus);
            drawBlobShadows(pRenderStage);
        }
        if (m_focus_actor < 0)
            drawShadows(ctx, dbuff, frame);
        else
            drawFocusActor(ctx, dbuff, frame);
        FUN_00177830(&m_837e0, ctx);
    }

    m_movie.drawMovie(ctx, lightstatus[0], pViewStatus);

    I3dLayer *pRenderStage = i3dCtxCreateLayer(ctx, 900.0f);
    if (pRenderStage == NULL)
    {
        ROR_ASSERT(pRenderStage); /* 2530 */
    }
    else
    {
        i3dLayerSetLight(pRenderStage, lightstatus[6]);
        i3dLayerSetView(pRenderStage, pViewStatus);
        u_long *pkt = i3dLayerBeginPacket(pRenderStage);
        pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x5000d);
        i3dLayerEndPacket(pRenderStage, pkt);
        m_life_man.draw(pRenderStage, 0x8000);
    }
    drawFocus(ctx, frame);
    m_movie.draw(ctx, lightstatus[0], pViewStatus);
}

// ──────────────────────────────────────────────────────────────────────
// After the frame's draw: the PC's and Brown's motion sequences.
// callcheck: updateMotionSeq (inlined MagLifeControlChara::renderPostProcess calls it)
// ROR 0x0013c29c

void MagScene::postDraw()
{
    m_pc.renderPostProcess();
    m_bwn.renderPostProcess();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013c2ec

void MagScene::setCameraTarget(const float *pos, const float *at)
{
    m_camera_pos[0] = pos[0];
    m_camera_pos[1] = pos[1];
    m_camera_pos[2] = pos[2];
    m_camera_pos[3] = pos[3];
    m_camera_at[0] = at[0];
    m_camera_at[1] = at[1];
    m_camera_at[2] = at[2];
    m_camera_at[3] = at[3];
    m_camera_target_set = 1;
}
