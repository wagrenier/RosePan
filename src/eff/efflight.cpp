/* ==========================================================================
 *  eff/efflight.cpp
 *
 *  EffLight1, EffLight2 (0x174dd8..0x17562c): glow sprites. EffLight3
 *  (0x17562c..0x176694) is still stubbed. See eff/efflight.h.
 * ======================================================================== */
#include "eff/efflight.h"

#include <math.h>
#include <string.h>

#include "dog/dogcamera.h"
#include "eff/effman.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dvu1pkt.h"
#include "mag/magscene.h"
#include "sdk/libgraph.h"

/* The sort depth both inits leave (the float bits 0x00800001, just above
 * FLT_MIN): drawn after every effect with a real depth. */
static const float EFF_LIGHT_Z = 0x1.000002p-126f;

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00174dd8

int EffLight1::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_pos[0] = param->m_light1.m_pos[0];
    m_pos[1] = param->m_light1.m_pos[1];
    m_pos[2] = param->m_light1.m_pos[2];
    m_pos[3] = 1.0f;
    m_scale[0] = param->m_light1.m_scale[0];
    m_scale[1] = param->m_light1.m_scale[1];
    for (int i = 0; i < 4; i++)
        m_color[i] = param->m_light1.m_color[i];
    m_near = param->m_light1.m_near;
    m_frame = 0;
    if (param->m_light1.m_fog != 0)
        m_flags |= EFF_LIGHT_FOG;
    m_fade = 0;
    m_z = EFF_LIGHT_Z;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00174e9c

int EffLight1::liveMain()
{
    m_frame++;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The sprite (texture 0) m_near toward the eye, the camera's matrix scaled
// by its size; the alpha the activation fade while one runs, times the
// flicker (64 or 72 / 128, every 3 frames), then the fog.
// ROR 0x00174eb0

u_long *EffLight1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    if ((m_flags & (EFF_FLAG_HIDDEN | EFF_FLAG_ACTIVATE)) == EFF_FLAG_HIDDEN)
        return pkt;

    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x48);
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));

    /* The rectangle, then the sprite's centre (the ROM reuses the buffer). */
    float pos[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    pkt = i3dPktBeginVu1Sprite(pkt, param->m_cur, 1);
    pkt = i3dPktVu1Rect(pkt, pos);

    DogCamera *cam = &g_effScene->m_camera_entity;
    cam->callTransform();
    sceVu0FMATRIX m;
    memcpy(m, cam->m_matrix, sizeof(m));
    float scale[4] = { m_scale[0], m_scale[1], 1.0f, 1.0f };
    float color[4] = { m_color[0], m_color[1], m_color[2], m_color[3] };
    if (m_flags & EFF_FLAG_ACTIVATE)
        color[3] = (float)m_fade / 5.0f;
    color[3] *= (float)(((m_frame / 3) & 1) * 8 + 0x40) * (1.0f / 128.0f);
    if (m_flags & EFF_LIGHT_FOG)
        effFogColor(color, effCalcViewDepth(cam, m_pos));

    effMoveToward(m_near, pos, m_pos, m[3]);
    i3dMatScale(m, m, scale);
    pkt = i3dPktVu1Matrix(pkt, m);
    pkt = i3dPktVu1Sprite(pkt, pos, color);
    return i3dPktEndVu1Sprite(pkt);
}

// ──────────────────────────────────────────────────────────────────────
// The 5-frame fade of setVisible: up to 5 while hidden, down to 0 while
// shown, then the activation is over.
// ROR 0x0017512c

void EffLight1::liveActivateExec()
{
    if ((m_flags & EFF_FLAG_HIDDEN) == 0)
    {
        if (--m_fade > 0)
            return;
    }
    else if (++m_fade < 5)
        return;
    m_flags &= ~EFF_FLAG_ACTIVATE;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00175170

int EffLight2::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_pos[0] = param->m_light2.m_pos[0];
    m_pos[1] = param->m_light2.m_pos[1];
    m_pos[2] = param->m_light2.m_pos[2];
    m_pos[3] = 1.0f;
    m_scale[0] = param->m_light2.m_scale[0];
    m_scale[1] = param->m_light2.m_scale[1];
    m_scale[2] = 1.0f;
    m_scale[3] = 1.0f;
    for (int i = 0; i < 4; i++)
        m_color[i] = param->m_light2.m_color[i];
    m_alpha = m_color[3];
    m_fade_near = param->m_light2.m_fade_near;
    m_fade_far = param->m_light2.m_fade_far;
    m_tex = param->m_light2.m_tex;
    m_z = EFF_LIGHT_Z;
    if (param->m_light2.m_fog != 0)
        m_flags |= EFF_LIGHT_FOG;
    m_fade = 0;
    m_frame = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00175254

int EffLight2::liveMain()
{
    m_frame++;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Once the delay is over and unless hidden: its distance from the eye,
// and inside m_fade_far into the always-pass layer.
// ROR 0x00175268

void EffLight2::render(RENDERPARAM *param)
{
    if (m_delay >= 0 || (m_flags & (EFF_FLAG_HIDDEN | EFF_FLAG_ACTIVATE)) == EFF_FLAG_HIDDEN)
        return;

    DogCamera *cam = &g_effScene->m_camera_entity;
    cam->callTransform();
    float eye[4] = { cam->m_matrix[3][0], cam->m_matrix[3][1], cam->m_matrix[3][2], cam->m_matrix[3][3] };
    float d[4];
    i3dVecSub(d, m_pos, eye);
    m_dist = effVecLength(d);
    if (m_dist < m_fade_far)
    {
        param->m_cur = param->m_layer[1];
        renderPreMain(param);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The sprite (texture m_tex) at m_pos, the camera's matrix scaled by
// m_scale; the alpha m_alpha times the flicker (0.5 or 0.525, every 4
// frames), less the square of the way from m_fade_near to m_fade_far,
// fogged, and times the activation fade while one runs.
// ROR 0x00175378

u_long *EffLight2::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x48);
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), m_tex));

    float rect[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    pkt = i3dPktBeginVu1Sprite(pkt, param->m_cur, 1);
    pkt = i3dPktVu1Rect(pkt, rect);

    DogCamera *cam = &g_effScene->m_camera_entity;
    cam->callTransform();
    sceVu0FMATRIX m;
    memcpy(m, cam->m_matrix, sizeof(m));
    i3dMatScale(m, m, m_scale);

    float color[4] = { m_color[0], m_color[1], m_color[2], m_color[3] };
    float alpha = ((float)((m_frame >> 2) & 1) * 0.025f + 0.5f) * m_alpha;
    if (m_fade_near < m_dist)
        alpha *= 1.0f - powf((m_dist - m_fade_near) / (m_fade_far - m_fade_near), 2.0f);
    color[3] = alpha;
    if (m_flags & EFF_LIGHT_FOG)
        effFogColor(color, effCalcViewDepth(cam, m_pos));
    if (m_flags & EFF_FLAG_ACTIVATE)
        color[3] = color[3] * (float)m_fade / 5.0f;

    pkt = i3dPktVu1Matrix(pkt, m);
    pkt = i3dPktVu1Sprite(pkt, m_pos, color);
    return i3dPktEndVu1Sprite(pkt);
}

// ──────────────────────────────────────────────────────────────────────
// EffLight1's fade.
// ROR 0x001755e8

void EffLight2::liveActivateExec()
{
    if ((m_flags & EFF_FLAG_HIDDEN) == 0)
    {
        if (--m_fade > 0)
            return;
    }
    else if (++m_fade < 5)
        return;
    m_flags &= ~EFF_FLAG_ACTIVATE;
}
