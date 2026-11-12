/* ==========================================================================
 *  eff/efffog2.cpp
 *
 *  EffFog2 / EffFog2Prim (0x171bfc..0x172678): the creeping ground fog.
 *  See eff/efffog2.h.
 * ======================================================================== */
#include "eff/efffog2.h"

#include <math.h>
#include <string.h>

#include "dog/dogcamera.h"
#include "eff/effman.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dvu1pkt.h"
#include "mag/magscene.h"
#include "sdk/libgraph.h"

static const float EFF_PI = 0x1.921fb4p+1f;     /* 3.1415925, the ROM's pi */

/* The wisp's corner alpha keys, one every 20 frames (stack-built in
 * renderMain), its corners and their STQ (data 0x2f2580, 0x2f25c0). */
static const float s_wispAlpha[5] = { 0.0f, 1.0f, 1.0f, 1.0f, 0.0f };
static const sceVu0FVECTOR s_wispXyz[4] = {
    { -0.5f, -0.5f, 0.0f, 1.0f }, { 0.5f, -0.5f, 0.0f, 1.0f },
    { -0.5f, 0.5f, 0.0f, 1.0f },  { 0.5f, 0.5f, 0.0f, 1.0f },
};
static const sceVu0FVECTOR s_wispStq[4] = {
    { 0.0f, 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.0f },
};

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00171bfc

int EffFog2::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_frame = -1;
    m_pos[0] = param->m_fog2.m_pos[0];
    m_pos[1] = param->m_fog2.m_pos[1];
    m_pos[2] = param->m_fog2.m_pos[2];
    m_width = param->m_fog2.m_width;
    m_rot = effWrapAngle(param->m_fog2.m_rot);
    m_dist = param->m_fog2.m_dist;
    m_color[0] = param->m_fog2.m_color[0];
    m_color[1] = param->m_fog2.m_color[1];
    m_color[2] = param->m_fog2.m_color[2];
    m_color[3] = param->m_fog2.m_color[3];
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Every 30 frames a wisp at r m_width along the line (r -0.5..0.5, the
// random byte plus the frame count), heading m_rot + r * 90 degrees
// (the ROM's 90 is 90.03096, 0x42b40fda). Never over.
// ROR 0x00171c94

int EffFog2::liveMain()
{
    if (++m_frame % 30 != 0)
        return 1;
    const float r = (float)((effRand() + m_frame) % 255) / 255.0f - 0.5f;
    sceVu0FMATRIX m;
    i3dMatUnit(m);
    i3dMatRotY(m_rot, m, m);

    EffParam p;
    p.m_kind = 0x19;
    p.m_08 = 0;
    const float w = m_width;
    p.m_fog2prim.m_pos[0] = m_pos[0] + m[0][0] * w * r;
    p.m_fog2prim.m_pos[2] = m_pos[2] + m[0][2] * w * r;
    p.m_fog2prim.m_pos[1] = m_pos[1] + m[0][1] * w * r;
    p.m_fog2prim.m_rot = effWrapAngle(m_rot + (r * 0x1.681fb4p+6f * EFF_PI) / 180.0f);   /* 90.03096 */
    p.m_fog2prim.m_dist = m_dist;
    p.m_fog2prim.m_color[0] = m_color[0];
    p.m_fog2prim.m_color[1] = m_color[1];
    p.m_fog2prim.m_color[2] = m_color[2];
    p.m_fog2prim.m_color[3] = m_color[3];
    g_effScene->m_eff_man.createEff(&p);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// A random size; the rise's top m_to: y -0.4 m_size, out along the
// heading's z axis by (0.2 m_size + 15) % of the start's depth (-y);
// m_rise 1.5 frames per unit of that depth; m_speed the straight
// distance start..top over m_rise; m_frames m_rise plus the glide frames
// left to m_dist (counted from the flat distance).
// ROR 0x00171e28

int EffFog2Prim::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_size = ((float)effRand() * 40.0f) / 255.0f + 60.0f;
    m_color[0] = param->m_fog2prim.m_color[0];
    m_color[1] = param->m_fog2prim.m_color[1];
    m_color[2] = param->m_fog2prim.m_color[2];
    m_color[3] = param->m_fog2prim.m_color[3];
    m_start[0] = param->m_fog2prim.m_pos[0];
    m_start[1] = param->m_fog2prim.m_pos[1];
    m_start[2] = param->m_fog2prim.m_pos[2];
    const float size = m_size;
    const float depth = -m_start[1];
    sceVu0FMATRIX m;
    i3dMatUnit(m);
    i3dMatRotY(param->m_fog2prim.m_rot, m, m);
    const float out = ((size * 0x1.999998p-3f + 15.0f) * depth) / 100.0f;     /* 0.19999999 */
    float d[4];
    m_to[1] = -m_size * 0x1.999998p-2f;        /* 0.39999998 */
    d[3] = 1.0f;
    m_to[0] = m_start[0] + m[2][0] * out;
    m_to[2] = m_start[2] + m[2][2] * out;
    d[1] = m_to[1] - m_start[1];
    d[2] = m_to[2] - m_start[2];
    m_rise = (int)((depth * 150.0f) / 100.0f);
    m_pos[0] = m_start[0];
    m_pos[1] = m_start[1];
    m_pos[2] = m_start[2];
    m_pos[3] = 1.0f;
    m_frame = 0;
    d[0] = m_to[0] - m_start[0];
    const float len = effVecLength(d);
    d[1] = 0.0f;
    m_speed = len / (float)m_rise;
    const float flat = effVecLength(d);
    m_frames = m_rise + (int)((param->m_fog2prim.m_dist - flat) / m_speed);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Rising (state 0): a quarter circle from m_start to m_to, x / z by
// 1 - cos, y by sin of frame / m_rise * 90 degrees; at m_rise the flat
// direction times m_speed becomes the step (in m_start) and the state
// 10, gliding. Over at m_frames.
// ROR 0x00172058

int EffFog2Prim::liveMain()
{
    if (++m_frame >= m_frames)
        return 0;
    m_08 = m_0c;
    if (m_0c == 0)
    {
        const float from[4] = { m_start[0], m_start[1], m_start[2], 1.0f };
        const float to[4] = { m_to[0], m_to[1], m_to[2], 1.0f };
        const float c = cosf(((((float)m_frame * 90.0f) / (float)m_rise) * EFF_PI) / 180.0f);
        const float s = sinf(((((float)m_frame * 90.0f) / (float)m_rise) * EFF_PI) / 180.0f);
        float d[4];
        i3dVecSub(d, to, from);
        m_pos[0] = m_start[0] + d[0] * (1.0f - c);
        m_pos[1] = m_start[1] + d[1] * s;
        m_pos[2] = m_start[2] + d[2] * (1.0f - c);
        if (m_frame == m_rise)
        {
            d[1] = 0.0f;
            i3dVecNormalize2(d, d);
            i3dVecScale(m_speed, d, d);
            m_0c = 10;
            m_start[0] = d[0];
            m_start[1] = d[1];
            m_start[2] = d[2];
        }
    }
    else if (m_0c == 10)
    {
        m_pos[0] += m_start[0];
        m_pos[2] += m_start[2];
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// A camera-facing quad m_size wide at m_pos, rolled by (frame + 3 m_rise)
// * pi / 600, texture 0 (ALPHA_1 0x44); the colour m_color tinted by the
// fog at m_z and the ambient light; each corner's alpha times its key
// (corner i 20 frames on from corner i - 1) and the fade of the first and
// last 30 frames.
// ROR 0x00172284

u_long *EffFog2Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));

    DogCamera *cam = &g_effScene->m_camera_entity;
    cam->callTransform();
    sceVu0FMATRIX m;
    memcpy(m, cam->m_matrix, sizeof(m));
    const float scale[4] = { m_size, m_size, 1.0f, 1.0f };
    effMatMulRotZ(effWrapAngle((float)(m_frame + m_rise * 3) * 0x1.572548p-8f), m);   /* pi / 600 */
    i3dMatScale(m, m, scale);
    memcpy(m[3], m_pos, sizeof(m[3]));

    sceVu0FVECTOR color[4];
    memcpy(color[0], m_color, sizeof(color[0]));
    effFogColor(color[0], m_z);
    effAmbientColor(color[0]);
    memcpy(color[1], color[0], sizeof(color[1]));
    memcpy(color[2], color[0], sizeof(color[2]));
    memcpy(color[3], color[0], sizeof(color[3]));

    const int frame = m_frame;
    float fade = 1.0f;
    if (frame < 30)
        fade = (float)frame / 30.0f;
    else if (m_frames - frame < 30)
        fade = (float)(m_frames - frame) / 30.0f;
    for (int i = 0; i < 4; i++)
    {
        const int a = frame + i * 20;
        const int k = a % 80 / 20;
        color[i][3] = color[i][3] * (s_wispAlpha[k] + ((s_wispAlpha[k + 1] - s_wispAlpha[k]) * (float)(a % 20)) / 20.0f) * fade;
    }
    return i3dPktDrawStrip(pkt, m, 4, s_wispStq, color, s_wispXyz, NULL);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00172638

void EffFog2Prim::updateZDepth()
{
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, m_pos);
}
