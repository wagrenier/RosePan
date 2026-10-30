/* ==========================================================================
 *  eff/effsteam.cpp
 *
 *  EffSteam1 / EffSteam1Prim / EffSteam2 / EffSteam2Prim (0x179428..
 *  0x17a2fc) and EffCloud1 (0x17a2fc..0x17a918): the vapour effects. See
 *  eff/effsteam.h.
 * ======================================================================== */
#include "eff/effsteam.h"

#include <float.h>
#include <string.h>

#include "dog/dogcamera.h"
#include "eff/effman.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dvu1pkt.h"
#include "mag/magscene.h"
#include "sdk/libgraph.h"

static const float EFF_PI = 0x1.921fb4p+1f;     /* 3.1415925, the ROM's pi */
static const float EFF_2PI = 0x1.921fb4p+2f;    /* 6.283185 */

/* EffSteam2Prim's quad (data 0x2f2ed0..0x2f2fb0): the corner colours
 * before the fog and ambient (only [0] is used, then copied), the alpha
 * keys the corners cycle through, the corners and their STQ. */
static const sceVu0FVECTOR s_wispColor[4] = {
    { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f },
};
static const float s_wispAlpha[6] = { 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f };
static const sceVu0FVECTOR s_wispXyz[4] = {
    { -0.5f, -0.5f, 0.0f, 1.0f }, { 0.5f, -0.5f, 0.0f, 1.0f },
    { -0.5f, 0.5f, 0.0f, 1.0f },  { 0.5f, 0.5f, 0.0f, 1.0f },
};
static const sceVu0FVECTOR s_wispStq[4] = {
    { 0.0f, 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.0f },
};

// ──────────────────────────────────────────────────────────────────────
// The vent: its period, a random phase, its matrix (rotated x then y by
// the param's angles, at its position), the puffs' scales and colour.
// ROR 0x00179428

int EffSteam1::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_size = param->m_steam1.m_size;
    m_speed = param->m_steam1.m_speed;
    const int frames = param->m_steam1.m_frames;
    m_frames = frames < 0 ? 0 : frames;
    const int period = frames + param->m_steam1.m_pause;
    m_period = period < 1 ? 1 : period;
    const u_char r = effRand();
    m_frame = (int)(((float)m_period * (float)r) / 255.0f);
    const float rot[4] = { param->m_steam1.m_rot[0], param->m_steam1.m_rot[1], 0.0f, 0.0f };
    i3dMatUnit(m_mat);
    i3dMatRotZXY(m_mat, m_mat, rot);
    m_mat[3][0] = param->m_steam1.m_pos[0];
    m_mat[3][1] = param->m_steam1.m_pos[1];
    m_mat[3][2] = param->m_steam1.m_pos[2];
    memcpy(m_color, param->m_steam1.m_color, sizeof(m_color));
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Every third frame of the period's first m_frames, a puff (kind 0x2a)
// along the vent's z axis turned by up to 15 degrees of y and any z: its
// speed 0.75..1.1 times m_speed, its fade 1 - 0.001 n^2 for the burst's
// n-th puff (0.5 at least). Nothing while hidden. Never over.
// ROR 0x0017955c

int EffSteam1::liveMain()
{
    if (m_flags & EFF_FLAG_HIDDEN)
        return 1;
    m_frame = (m_frame + 1) % m_period;
    if (m_frame >= m_frames || m_frame % 3 != 0)
        return 1;
    const int n = m_frame / 3;

    float rot[4];
    rot[0] = 0.0f;
    rot[1] = ((((float)effRand() * 30.0f) / 255.0f - 15.0f) * EFF_PI) / 180.0f;
    rot[2] = ((((float)effRand() * 360.0f) / 255.0f - 180.0f) * EFF_PI) / 180.0f;
    rot[3] = 0.0f;
    sceVu0FMATRIX m;
    i3dMatUnit(m);
    i3dMatRotXYZ(m, m, rot);
    i3dMatMul(m, m_mat, m);

    EffParam param;
    memset(&param, 0, sizeof(param));   /* PC: zeroed (the ROM sets kind, m_08 and the fields below) */
    param.m_kind = 0x2a;
    param.m_08 = 0;
    param.m_steam1prim.m_pos = m_mat[3];
    param.m_steam1prim.m_dir[0] = m[2][0];
    param.m_steam1prim.m_dir[1] = m[2][1];
    param.m_steam1prim.m_dir[2] = m[2][2];
    const u_char r = effRand();
    param.m_steam1prim.m_size = m_size;
    param.m_steam1prim.m_speed = m_speed * (((float)r * 0x1.66666p-2f) / 255.0f + 0.75f);
    float fade = 1.0f - (float)(n * n) * 0x1.0624dcp-10f;
    memcpy(param.m_steam1prim.m_color, m_color, sizeof(m_color));
    if (fade < 0.5f)
        fade = 0.5f;
    param.m_steam1prim.m_fade = fade;
    g_effScene->m_eff_man.createEff(&param);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001797d8

int EffSteam1Prim::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_scale = param->m_steam1prim.m_size;
    memcpy(m_pos, param->m_steam1prim.m_pos, sizeof(m_pos));
    m_pos[3] = 1.0f;
    const float speed = param->m_steam1prim.m_speed * 15.0f;
    m_vel[0] = param->m_steam1prim.m_dir[0] * speed;
    m_vel[1] = param->m_steam1prim.m_dir[1] * speed;
    m_size = 20.0f;
    m_alpha = 0x1.999998p-2f;       /* 0.4 */
    m_alpha_add = 0x1.eb851ep-6f;   /* 0.03 */
    m_vel[3] = 0.0f;
    m_vel[2] = param->m_steam1prim.m_dir[2] * speed;
    m_fade = param->m_steam1prim.m_fade;
    memcpy(m_color, param->m_steam1prim.m_color, sizeof(m_color));
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The alpha steps (at 1 it turns to fading); while it is above 0 the
// puff slows, moves and grows. Over when the alpha reaches 0.
// ROR 0x001798d8

int EffSteam1Prim::liveMain()
{
    m_alpha += m_alpha_add;
    if (m_alpha <= 0.0f)
        return 0;
    if (1.0f <= m_alpha)
    {
        m_alpha = 1.0f;
        m_alpha_add = m_fade * -0x1.1eb85p-4f;     /* -0.07 */
    }
    i3dVecScale(0x1.e66666p-1f, m_vel, m_vel);     /* 0.95 */
    i3dVecAdd(m_pos, m_pos, m_vel);
    m_size += 4.5f;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// One camera-facing sprite m_size * m_scale wide at m_pos, texture 0, its
// colour times the ambient light, its alpha times m_alpha.
// ROR 0x00179998

u_long *EffSteam1Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    u_long *p = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    p = i3dPktCallView(p, param->m_cur);
    p = i3dPktRefTexRegs(p, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));
    const float rect[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    p = i3dPktBeginVu1Sprite(p, param->m_cur, 1);
    p = i3dPktVu1Rect(p, rect);

    DogCamera *cam = &g_effScene->m_camera_entity;
    cam->callTransform();
    sceVu0FMATRIX m;
    memcpy(m, cam->m_matrix, sizeof(m));
    float color[4] = { m_color[0], m_color[1], m_color[2], m_color[3] * m_alpha };
    const float size = m_size * m_scale;
    const float scale[4] = { size, size, 1.0f, 1.0f };
    i3dMatScale(m, m, scale);
    effAmbientColor(color);
    p = i3dPktVu1Matrix(p, m);
    p = i3dPktVu1Sprite(p, m_pos, color);
    return i3dPktEndVu1Sprite(p);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00179b5c

void EffSteam1Prim::updateZDepth()
{
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, m_pos);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00179b9c

int EffSteam2::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_pos[0] = param->m_steam2.m_pos[0];
    m_pos[1] = param->m_steam2.m_pos[1];
    m_pos[2] = param->m_steam2.m_pos[2];
    m_radius = param->m_steam2.m_radius;
    m_alpha = param->m_steam2.m_alpha;
    m_wait = (u_char)((effRand() * 150) / 255);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// When the wait runs out, a wisp (kind 0x2c) at a random point of a
// disc of radius m_radius / 2, m_radius / 2 toward -y, its alpha
// m_alpha times 0.5..1; the next in 180..210 frames. Never over.
// ROR 0x00179c30

int EffSteam2::liveMain()
{
    if (m_wait != 0)
    {
        m_wait--;
        return 1;
    }
    EffParam param;
    memset(&param, 0, sizeof(param));   /* PC: zeroed (the ROM sets kind, m_08 and the fields below) */
    param.m_kind = 0x2c;
    param.m_08 = 0;
    sceVu0FMATRIX m;
    i3dMatUnit(m);
    m[3][0] = (m_radius * (float)effRand()) / 510.0f;
    i3dMatRotY(((float)effRand() * EFF_2PI) / 255.0f - EFF_PI, m, m);
    const float radius = m_radius;
    param.m_steam2prim.m_size = radius;
    param.m_steam2prim.m_pos[0] = m_pos[0] + m[3][0];
    param.m_steam2prim.m_pos[2] = m_pos[2] + m[3][2];
    param.m_steam2prim.m_pos[1] = m_pos[1] + m[3][1] + -radius * 0.5f;
    const u_char r = effRand();
    /* the ROM computes the alpha in double */
    // callcheck: fptodp, dpsub, dpmul, dptofp (the soft-float double runtime, compiler-made)
    param.m_steam2prim.m_alpha = (float)((double)m_alpha * (1.0 - (double)((float)r / 510.0f)));
    g_effScene->m_eff_man.createEff(&param);
    m_wait = (u_char)((effRand() * 30) / 255 + 180);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00179dfc

int EffSteam2Prim::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_pos[0] = param->m_steam2prim.m_pos[0];
    m_pos[1] = param->m_steam2prim.m_pos[1];
    m_pos[2] = param->m_steam2prim.m_pos[2];
    m_pos[3] = 1.0f;
    m_alpha = param->m_steam2prim.m_alpha;
    m_age = 0;
    m_size = param->m_steam2prim.m_size;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// 150 frames moving m_size / 300 a frame toward -y.
// ROR 0x00179e70

int EffSteam2Prim::liveMain()
{
    if (++m_age >= 150)
        return 0;
    m_pos[1] += (m_size * -0x1.999998p-4f) / 30.0f;    /* -0.1 */
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// A camera-facing quad m_size wide at m_pos, rolled by (m_age + 100
// m_alpha) * pi / 600, texture 0; the colour the fog at m_z and the
// ambient light; the alpha m_alpha faded in over the first 15 frames and
// out over the last 15, each corner times s_wispAlpha (a key every 20
// frames, corner i one key on from corner i - 1).
// ROR 0x00179ed0

u_long *EffSteam2Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    u_long *p = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    p = i3dPktCallView(p, param->m_cur);
    p = i3dPktRefTexRegs(p, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));

    DogCamera *cam = &g_effScene->m_camera_entity;
    cam->callTransform();
    sceVu0FMATRIX m;
    memcpy(m, cam->m_matrix, sizeof(m));
    const float scale[4] = { m_size, m_size, 1.0f, 1.0f };
    effMatMulRotZ(((float)m_age + m_alpha * 100.0f) * 0x1.572548p-8f, m);   /* pi / 600 */
    i3dMatScale(m, m, scale);
    memcpy(m[3], m_pos, sizeof(m[3]));

    sceVu0FVECTOR color[4];
    memcpy(color, s_wispColor, sizeof(color));
    effFogColor(color[0], m_z);
    effAmbientColor(color[0]);
    memcpy(color[1], color[0], sizeof(color[1]));
    memcpy(color[2], color[0], sizeof(color[2]));
    memcpy(color[3], color[0], sizeof(color[3]));

    const u_int age = m_age;
    float alpha = m_alpha;
    if (age < 15)
        alpha = ((float)(int)age / 15.0f) * alpha;
    else if (age >= 0x88)
        alpha = ((float)(int)(150 - age) / 15.0f) * alpha;
    const float t = (float)(int)(age % 20) / 20.0f;
    const u_int key = (age / 20) & 0xff;
    for (int i = 0; i < 4; i++)
    {
        const int k = (int)(key + i) % 5;
        color[i][3] = (s_wispAlpha[k] + (s_wispAlpha[k + 1] - s_wispAlpha[k]) * t) * alpha;
    }
    return i3dPktDrawStrip(p, m, 4, s_wispStq, color, s_wispXyz, NULL);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017a2bc

void EffSteam2Prim::updateZDepth()
{
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, m_pos);
}

// ──────────────────────────────────────────────────────────────────────
// The origin, size and colour; the drift m_speed along the y angle; the
// frame count set to the end of the drift, so it starts with the wait.
// ROR 0x0017a2fc

int EffCloud1::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_origin[0] = param->m_cloud.m_pos[0];
    m_origin[1] = param->m_cloud.m_pos[1];
    m_origin[2] = param->m_cloud.m_pos[2];
    memcpy(m_color, param->m_cloud.m_color, sizeof(m_color));
    m_size = param->m_cloud.m_size;
    sceVu0FMATRIX m;
    i3dMatUnit(m);
    m[2][2] = param->m_cloud.m_speed;
    i3dMatRotY(param->m_cloud.m_rot, m, m);
    m_vel[0] = m[2][0];
    m_vel[1] = m[2][1];
    m_vel[2] = m[2][2];
    resetPos();
    m_pos[3] = 1.0f;
    const int frames = (int)(param->m_cloud.m_dist / param->m_cloud.m_speed) + 1;
    m_frames = frames;
    m_frame = frames;
    /* PC: the ROM stores the word 0x7fffffff (data 0x2f2fb0), the EE FPU's
     * largest value; on the host those bits are a NaN (eff/eff0.cpp). */
    m_z = FLT_MAX;
    m_wait = frames + frames % 30;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017a410

void EffCloud1::resetPos()
{
    sceVu0FMATRIX m;
    i3dMatUnit(m);
    m[3][0] = (m_size * (float)effRand()) / 510.0f;
    i3dMatRotX(((float)effRand() * EFF_2PI) / 255.0f - EFF_PI, m, m);
    i3dMatRotY(((float)effRand() * EFF_2PI) / 255.0f - EFF_PI, m, m);
    m_pos[0] = m_origin[0] + m[3][0];
    m_pos[1] = m_origin[1] + m[3][1];
    m_pos[2] = m_origin[2] + m[3][2];
    m_spin = effRand();
}

// ──────────────────────────────────────────────────────────────────────
// Drifting (state 0) for m_frames frames, then waiting (10) until frame
// m_wait, then from a new point again. Never over.
// ROR 0x0017a534

int EffCloud1::liveMain()
{
    const int state = m_0c;
    m_frame++;
    m_08 = state;
    if (state == 0)
    {
        if (m_frame < m_frames)
        {
            m_pos[0] += m_vel[0];
            m_pos[1] += m_vel[1];
            m_pos[2] += m_vel[2];
        }
        else
            m_0c = 10;
    }
    else if (state == 10 && m_wait <= m_frame)
    {
        resetPos();
        m_0c = 0;
        m_frame = 0;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// While drifting: a camera-facing sprite m_size wide at m_pos, rolled by
// (m_frame + m_spin) * pi / 900, texture 0, the colour m_color; its alpha
// fades out with the square of the depth between the fog's near and far
// distances (none from far on) and in / out over the drift's first and
// last 60 frames. Nothing when the alpha is 0.
// ROR 0x0017a5ec

u_long *EffCloud1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    if (m_08 != 0)
        return pkt;
    float color[4] = { m_color[0], m_color[1], m_color[2], m_color[3] };
    DogCamera *cam = &g_effScene->m_camera_entity;
    float depth = effCalcViewDepth(cam, m_pos);
    const I3dFog *fog = &g_effScene->m_fog[0];
    if (fog->m_param[2] <= depth)
        color[3] = 0.0f;
    else
    {
        const float fnear = fog->m_param[0];
        if (fnear < depth)
        {
            depth = (depth - fnear) / (fog->m_param[2] - fnear);
            color[3] = (1.0f - depth * depth) * m_color[3];
        }
    }
    if (color[3] <= 0.0f)
        return pkt;

    u_long *p = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    p = i3dPktCallView(p, param->m_cur);
    p = i3dPktRefTexRegs(p, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));
    const float rect[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    p = i3dPktBeginVu1Sprite(p, param->m_cur, 1);
    p = i3dPktVu1Rect(p, rect);

    cam->callTransform();
    sceVu0FMATRIX m;
    memcpy(m, cam->m_matrix, sizeof(m));
    const float scale[4] = { m_size, m_size, 1.0f, 1.0f };
    effMatMulRotZ(effWrapAngle((float)(m_frame + m_spin) * 0x1.c9870ap-9f), m);  /* pi / 900 */
    i3dMatScale(m, m, scale);
    if (m_frames - 60 < m_frame)
        color[3] *= (float)(m_frames - m_frame) / 60.0f;
    else if (m_frame < 60)
        color[3] *= (float)m_frame / 60.0f;
    p = i3dPktVu1Matrix(p, m);
    p = i3dPktVu1Sprite(p, m_pos, color);
    return i3dPktEndVu1Sprite(p);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017a8d8

void EffCloud1::updateZDepth()
{
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, m_pos);
}
