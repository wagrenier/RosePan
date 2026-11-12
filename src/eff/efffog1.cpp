/* ==========================================================================
 *  eff/efffog1.cpp
 *
 *  EffFog1 (0x1716bc..0x171bfc): a drifting camera-facing fog bank. See
 *  eff/efffog1.h.
 * ======================================================================== */
#include "eff/efffog1.h"

#include <string.h>

#include "dog/dogcamera.h"
#include "eff/effman.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dvu1pkt.h"
#include "mag/magscene.h"
#include "sdk/libgraph.h"

/* The bank's alpha keys, one every 60 frames (data 0x2f2470), the corner
 * colours before the fog and ambient (0x2f24a0; only [0] is used, then
 * copied), the corners and their STQ (0x2f24e0, 0x2f2520). */
static const float s_fogAlpha[10] = {
    0.0f, 0.0f, 0.5f, 1.0f, 0x1.333332p-1f, 1.0f, 0x1.333332p-1f, 1.0f, 0.5f, 0.0f,   /* 0.59999996 */
};
static const sceVu0FVECTOR s_fogColor[4] = {
    { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f },
};
static const sceVu0FVECTOR s_fogXyz[4] = {
    { -0.5f, -0.5f, 0.0f, 1.0f }, { 0.5f, -0.5f, 0.0f, 1.0f },
    { -0.5f, 0.5f, 0.0f, 1.0f },  { 0.5f, 0.5f, 0.0f, 1.0f },
};
static const sceVu0FVECTOR s_fogStq[4] = {
    { 0.0f, 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.0f },
};

// ──────────────────────────────────────────────────────────────────────
// The size, the position (m_raise: given as the bottom edge, -y up), the
// start it returns to, alpha, drift and a random phase 0..540.
// ROR 0x001716bc

int EffFog1::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_scale[0] = param->m_fog1.m_scale[0];
    m_scale[1] = param->m_fog1.m_scale[1];
    m_scale[2] = 1.0f;
    m_scale[3] = 1.0f;
    m_pos[0] = param->m_fog1.m_pos[0];
    m_pos[1] = param->m_fog1.m_pos[1];
    m_pos[2] = param->m_fog1.m_pos[2];
    m_pos[3] = 1.0f;
    if (param->m_fog1.m_raise != 0)
        m_pos[1] -= m_scale[1] * 0.5f;
    memcpy(m_start, m_pos, sizeof(m_start));
    m_alpha = param->m_fog1.m_alpha;
    m_speed = param->m_fog1.m_speed;
    m_frame = (effRand() * 540) / 255;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Back to the start every 540 frames. Never over.
// ROR 0x001717b8

int EffFog1::liveMain()
{
    m_frame = (m_frame + 1) % 540;
    if (m_frame == 0)
        memcpy(m_pos, m_start, sizeof(m_pos));
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The drift (along the camera matrix's x row, x and z only -- done here,
// once per drawn frame), then the quad: the camera matrix scaled to
// m_scale at m_pos; the corners' colour the fog at m_z and the ambient;
// the left corners' alpha the key 45 frames on, the right ones' this
// frame's, times m_alpha.
// ROR 0x001717f4

u_long *EffFog1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));

    DogCamera *cam = &g_effScene->m_camera_entity;
    cam->callTransform();
    sceVu0FMATRIX m;
    memcpy(m, cam->m_matrix, sizeof(m));
    m_pos[0] += m[0][0] * m_speed;
    m_pos[2] += m[0][2] * m_speed;
    i3dMatScale(m, m, m_scale);
    memcpy(m[3], m_pos, sizeof(m[3]));

    sceVu0FVECTOR color[4];
    memcpy(color, s_fogColor, sizeof(color));
    effFogColor(color[0], m_z);
    effAmbientColor(color[0]);
    memcpy(color[1], color[0], sizeof(color[1]));
    memcpy(color[2], color[0], sizeof(color[2]));
    memcpy(color[3], color[0], sizeof(color[3]));

    const int frame = m_frame;
    const int a = frame % 540 / 60;
    const int b = (frame + 45) % 540 / 60;
    const float left = (s_fogAlpha[b] + ((s_fogAlpha[b + 1] - s_fogAlpha[b]) * (float)((frame + 45) % 60)) / 60.0f) * m_alpha;
    color[2][3] = left;
    color[0][3] = left;
    const float right = (s_fogAlpha[a] + ((s_fogAlpha[a + 1] - s_fogAlpha[a]) * (float)(frame % 60)) / 60.0f) * m_alpha;
    color[3][3] = right;
    color[1][3] = right;
    return i3dPktDrawStrip(pkt, m, 4, s_fogStq, color, s_fogXyz, NULL);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00171bbc

void EffFog1::updateZDepth()
{
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, m_pos);
}
