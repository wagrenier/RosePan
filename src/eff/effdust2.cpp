/* ==========================================================================
 *  eff/effdust2.cpp
 *
 *  EffDust2 / EffDust2Prim (0x1704a4..0x170ca4): the dust that rises
 *  along a segment. See eff/effdust2.h.
 * ======================================================================== */
#include "eff/effdust2.h"

#include "dog/dogcamera.h"
#include "eff/effman.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dvu1pkt.h"
#include "mag/magscene.h"
#include "sdk/libgraph.h"

static const float EFF_PI = 0x1.921fb4p+1f;     /* 3.1415925, the ROM's pi */
static const float EFF_2PI = 0x1.921fb4p+2f;    /* 6.283185 */

/* The puff's texture rect and its alpha keys, one every 30 frames
 * (data 0x2f2390). */
static const float s_primRect[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
static const float s_primAlpha[6] = { 0.0f, 0x1.999998p-1f, 1.0f, 0x1.999998p-1f, 0.0f, 0.0f };   /* 0.79999995 */

// ──────────────────────────────────────────────────────────────────────
// The segment, the scatter radii, the puffs' colour and size; 12 puffs
// started 8 frames apart.
// ROR 0x001704a4

int EffDust2::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_from[0] = param->m_dust2.m_from[0];
    m_from[1] = param->m_dust2.m_from[1];
    m_from[2] = param->m_dust2.m_from[2];
    m_from[3] = 1.0f;
    m_spread0 = param->m_dust2.m_spread0;
    m_to[0] = param->m_dust2.m_to[0];
    m_to[1] = param->m_dust2.m_to[1];
    m_to[2] = param->m_dust2.m_to[2];
    m_to[3] = 1.0f;
    m_spread1 = param->m_dust2.m_spread1;
    m_color[0] = param->m_dust2.m_color[0];
    m_color[1] = param->m_dust2.m_color[1];
    m_color[2] = param->m_dust2.m_color[2];
    m_color[3] = param->m_dust2.m_color[3];
    m_step = 0;
    m_size = param->m_dust2.m_size;
    for (int i = 0; i < 12; i++)
        m_no[i] = createPrim(i * 8);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// A puff whose effect has ended is replaced at once. Never over.
// ROR 0x00170594

int EffDust2::liveMain()
{
    for (int i = 0; i < 12; i++)
    {
        /* PC: createEff returns -1 when it fails; the ROM then reads the
         * word before the table (EffMan::m_scene, never NULL), so the
         * slot is never refilled -- the host keeps that. */
        if (m_no[i] >= 0 && g_effScene->m_eff_man.m_eff[m_no[i]] == NULL)
            m_no[i] = createPrim(0);
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// A kind-0x13 puff at t along the segment (m_step's eighth plus up to an
// eighth more at random), moved by the z row of a random rotation scaled
// to up to the radius at t; its alpha times 0.2 + 0.8 (1 - the point's
// projection on the segment, 0..1), so the puffs fade toward m_to.
// ROR 0x00170624

int EffDust2::createPrim(int delay)
{
    m_step += 3;
    const float t = ((float)effRand() * 0.125f) / 255.0f + (float)(m_step & 7) * 0.125f;

    sceVu0FMATRIX m;
    float v[4];
    i3dMatUnit(m);
    v[0] = ((float)effRand() * EFF_2PI) / 255.0f - EFF_PI;
    v[1] = ((float)effRand() * EFF_2PI) / 255.0f;
    v[2] = 0.0f;
    v[3] = 0.0f;
    i3dMatRotZXY(m, m, v);
    const float s = m_spread0 + ((m_spread1 - m_spread0) * t * (float)effRand()) / 255.0f;
    v[0] = s;
    v[1] = s;
    v[2] = s;
    v[3] = 1.0f;
    i3dMatScale(m, m, v);
    i3dVecSub(v, m_to, m_from);
    v[3] = 1.0f;
    i3dVecScale(t, v, v);
    i3dVecAdd(v, m_from, v);

    EffParam p;
    p.m_kind = 0x13;
    p.m_08 = delay;
    p.m_dust2prim.m_pos[0] = v[0] + m[2][0];
    p.m_dust2prim.m_pos[1] = v[1] + m[2][1];
    p.m_dust2prim.m_pos[2] = v[2] + m[2][2];
    p.m_dust2prim.m_size = m_size;
    p.m_dust2prim.m_color[0] = m_color[0];
    p.m_dust2prim.m_color[1] = m_color[1];
    p.m_dust2prim.m_color[2] = m_color[2];
    p.m_dust2prim.m_color[3] = m_color[3];

    float dir[4], rel[4];
    i3dVecSub(dir, m_to, m_from);
    const float len = effVecLength(dir);
    i3dVecNormalize2(dir, dir);
    v[0] = p.m_dust2prim.m_pos[0];
    v[1] = p.m_dust2prim.m_pos[1];
    v[2] = p.m_dust2prim.m_pos[2];
    dir[3] = 1.0f;
    i3dVecSub(rel, v, m_from);
    i3dVecScale(1.0f / len, rel, rel);
    rel[3] = 1.0f;
    const float d = i3dVecDot(dir, rel);
    float f = 0.0f;
    if (d <= 1.0f)
    {
        f = 1.0f;
        if (0.0f <= d)
            f = 1.0f - d;
    }
    p.m_dust2prim.m_color[3] *= f * 0x1.99999ap-1f + 0x1.999998p-3f;    /* 0.8, 0.19999999 */
    return g_effScene->m_eff_man.createEff(&p);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017093c

int EffDust2Prim::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_pos[0] = param->m_dust2prim.m_pos[0];
    m_pos[1] = param->m_dust2prim.m_pos[1];
    m_pos[2] = param->m_dust2prim.m_pos[2];
    m_pos[3] = 1.0f;
    m_size = param->m_dust2prim.m_size;
    m_color[0] = param->m_dust2prim.m_color[0];
    m_color[1] = param->m_dust2prim.m_color[1];
    m_color[2] = param->m_dust2prim.m_color[2];
    m_color[3] = param->m_dust2prim.m_color[3];
    m_frame = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// 150 frames rising 0.025 a frame.
// ROR 0x001709c8

int EffDust2Prim::liveMain()
{
    m_frame++;
    m_pos[1] += -0x1.999998p-6f;     /* -0.024999999 */
    return m_frame < 150;
}

// ──────────────────────────────────────────────────────────────────────
// A camera-facing sprite m_size wide turned 0.06 degrees a frame,
// texture 0 (ALPHA_1 0x48); its alpha m_color's times the 30-frame keys,
// times 0.9 in frames with bit 2 set.
// ROR 0x001709f4

u_long *EffDust2Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x48);
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));
    pkt = i3dPktBeginVu1Sprite(pkt, param->m_cur, 1);
    pkt = i3dPktVu1Rect(pkt, s_primRect);

    sceVu0FMATRIX m;
    i3dMatUnit(m);
    i3dMatRotZ(((float)m_frame * 0x1.eb851ep-5f * EFF_PI) / 180.0f, m, m);  /* 0.06 */
    DogCamera *cam = &g_effScene->m_camera_entity;
    cam->callTransform();
    i3dMatMul(m, m, cam->m_matrix);
    const float scale[4] = { m_size, m_size, 1.0f, 1.0f };
    i3dMatScale(m, m, scale);

    const int frame = m_frame;
    const int k = frame / 30;
    float color[4];
    color[0] = m_color[0];
    color[1] = m_color[1];
    color[2] = m_color[2];
    color[3] = m_color[3] * (s_primAlpha[k] + ((s_primAlpha[k + 1] - s_primAlpha[k]) * (float)(frame % 30)) / 30.0f);
    if (frame & 4)
        color[3] *= 0x1.ccccccp-1f;     /* 0.9 (0x3f666666) */
    pkt = i3dPktVu1Matrix(pkt, m);
    pkt = i3dPktVu1Sprite(pkt, m_pos, color);
    return i3dPktEndVu1Sprite(pkt);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00170c64

void EffDust2Prim::updateZDepth()
{
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, m_pos);
}
