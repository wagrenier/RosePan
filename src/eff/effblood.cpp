/* ==========================================================================
 *  eff/effblood.cpp
 *
 *  EffBloodSpray1 / 2 (0x16b62c..0x16c7c0), EffBloodStay1 / 2 / 3 / 3Prim
 *  (0x16c7c0..0x16dcc8): the blood effects. See eff/effblood.h.
 * ======================================================================== */
#include "eff/effblood.h"

#include <math.h>
#include <string.h>

#include "dog/dogcamera.h"
#include "eff/effman.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dvu1pkt.h"
#include "mag/magscene.h"
#include "sdk/libgraph.h"

int g_effBloodStay3 = -1;   /* 0x298e10 */

/* The drops' turns from the spray direction, radians about x then z
 * (EffBloodSpray1: data 0x2f1fa0, EffBloodSpray2: 0x2f2020; the ROM copies
 * them to the stack). */
static const float s_spray1Turn[9][2] = {
    {  0.0f,                0.0f                },
    {  0.0872664526104927f, 0.1745329052209854f },
    { -0.0872664526104927f, -0.1745329052209854f },
    {  0.1745329052209854f, -0.0872664526104927f },
    { -0.1745329052209854f, 0.0872664526104927f },
    {  0.2617993652820587f, 0.5235987305641174f },
    { -0.2617993652820587f, -0.5235987305641174f },
    {  0.5235987305641174f, -0.2617993652820587f },
    { -0.5235987305641174f, 0.2617993652820587f },
};
static const float s_spray2Turn[9][2] = {
    {  0.0f,                 0.0f                 },
    {  0.0872664526104927f,  0.0f                 },
    {  0.0f,                 0.0872664526104927f  },
    { -0.0872664526104927f,  0.0f                 },
    {  0.0f,                -0.0872664526104927f  },
    {  0.13962632417678833f, 0.13962632417678833f },
    {  0.13962632417678833f, -0.13962632417678833f },
    { -0.13962632417678833f, 0.13962632417678833f },
    { -0.13962632417678833f, -0.13962632417678833f },
};

/* The stains' delays (frames) and scales (EffBloodStay1, on the stack). */
static const int   s_stainDelay[4] = { 0, 15, 25, 40 };
static const float s_stainScale[4] = { 1.0f, 0x1.999998p-1f, 0.7f, 0x1.333332p-1f };

static const float EFF_PI = 0x1.921fb4p+1f;    /* 3.1415925, the ROM's pi */

/* A drop to draw and its view depth (the renders' stack pairs). EE 8.
 * layout: host -- only on the stack. */
struct EffDepthEntry
{
    int   m_no;
    float m_z;
};

/* entry[0..n) by depth, the farthest first (the renders' selection sort,
 * written out in both). */
static void effSortFarFirst(EffDepthEntry *entry, int n)
{
    for (int i = 0; i < n - 1; i++)
    {
        int best = i;
        for (int j = i + 1; j < n; j++)
        {
            if (entry[best].m_z < entry[j].m_z)
                best = j;
        }
        if (best != i)
        {
            EffDepthEntry t = entry[i];
            entry[i] = entry[best];
            entry[best] = t;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016b62c

EffBloodSpray1::EffBloodSpray1()
{
    memset(m_drop, 0, sizeof(m_drop));    // callcheck: Eff0 (the base constructor, compiler-made)
}

// ──────────────────────────────────────────────────────────────────────
// Nine drops from the param's point along its direction turned by the
// table (from a random row on): each slower (x 0.8), wider-growing (x 1.1)
// and quicker to fade (scale x 1.25) than the one before.
// ROR 0x0016b668

int EffBloodSpray1::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_frame = 0;
    float scale = param->m_hit.m_scale;
    const float size = scale * 4.5f;
    float grow = size * 0.5f;
    float speed = size * 3.0f;

    m_pos[0] = param->m_hit.m_pos[0];
    m_pos[1] = param->m_hit.m_pos[1];
    m_pos[2] = param->m_hit.m_pos[2];
    m_pos[3] = 1.0f;
    int turn = effRand() % 9;
    for (int i = 0; i < 9; i++)
    {
        EffBloodSpray1Drop *drop = &m_drop[i];
        sceVu0FVECTOR dir = { param->m_hit.m_1c, param->m_hit.m_20, param->m_hit.m_24, 0.0f };
        sceVu0FMATRIX m;
        i3dMatUnit(m);
        i3dMatRotX(s_spray1Turn[turn][0], m, m);
        i3dMatRotZ(s_spray1Turn[turn][1], m, m);
        i3dMatApply(dir, m, dir);
        turn = (turn + 1) % 9;

        drop->m_alpha = 0x7f00;
        drop->m_fade = 0;
        drop->m_fade_add = (short)((int)(scale * 192.0f) + 0x300);
        scale *= 1.25f;
        memcpy(drop->m_pos, m_pos, sizeof(drop->m_pos));
        memcpy(drop->m_vel, dir, sizeof(drop->m_vel));
        drop->m_speed = speed;
        speed *= 0x1.999998p-1f;
        drop->m_grow = grow;
        grow *= 0x1.199998p+0f;
        drop->m_size = size;
        drop->m_fall = (fabsf(drop->m_vel[1] * 0.5f) + 0.5f) * (param->m_hit.m_scale * 0.01f + 0.04f);
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// From the 9th frame the live drops fade (faster each frame); the live
// ones move, grow, slow and fall. Over when drop 0 is gone.
// ROR 0x0016b978

int EffBloodSpray1::liveMain()
{
    if (++m_frame > 8)
    {
        for (int i = 0; i < 9; i++)
        {
            EffBloodSpray1Drop *drop = &m_drop[i];
            if (drop->m_alpha <= 0xff)
                continue;
            if (drop->m_alpha < drop->m_fade)
                drop->m_alpha = 0;
            else
            {
                drop->m_alpha -= drop->m_fade;
                drop->m_fade += drop->m_fade_add;
            }
        }
    }
    for (int i = 0; i < 9; i++)
    {
        EffBloodSpray1Drop *drop = &m_drop[i];
        if (drop->m_alpha <= 0xff)
            continue;
        const float speed = drop->m_speed;
        drop->m_pos[1] += drop->m_vel[1] * speed;
        drop->m_pos[0] += drop->m_vel[0] * speed;
        drop->m_pos[2] += drop->m_vel[2] * speed;
        drop->m_size += drop->m_grow;
        drop->m_speed = speed * 0x1.f33332p-1f;
        drop->m_vel[1] += drop->m_fall;
    }
    return m_drop[0].m_alpha > 0xff;
}

// ──────────────────────────────────────────────────────────────────────
// The live drops far to near as camera-facing sprites 20 toward the eye,
// the colour the ambient and the fog at the effect's depth, the alpha the
// drop's; texture 0 for drops 0..4, 1 for 5..8. Nothing when none lives.
// ROR 0x0016ba7c

u_long *EffBloodSpray1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    DogCamera *cam = &g_effScene->m_camera_entity;
    EffDepthEntry entry[9];
    int n = 0;
    for (int i = 0; i < 9; i++)
    {
        if (m_drop[i].m_alpha > 0xff)
        {
            entry[n].m_no = i;
            entry[n].m_z = effCalcViewDepth(cam, m_drop[i].m_pos);
            n++;
        }
    }
    if (n <= 0)
        return pkt;
    effSortFarFirst(entry, n);

    cam->callTransform();
    float scale[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    u_long *p = i3dPktCallView(pkt, param->m_cur);
    p = i3dPktSetReg(p, SCE_GS_ALPHA_1, 0x44);
    const float rect[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    p = i3dPktBeginVu1Sprite(p, param->m_cur, 1);
    p = i3dPktVu1Rect(p, rect);
    effAmbientColor(color);
    effFogColor(color, m_z);

    int tex = -1;
    for (int k = 0; k < n; k++)
    {
        const EffBloodSpray1Drop *drop = &m_drop[entry[k].m_no];
        color[3] = (float)drop->m_alpha * (1.0f / 32768.0f);
        sceVu0FMATRIX m;
        memcpy(m, cam->m_matrix, sizeof(m));
        effMoveToward(20.0f, m[3], drop->m_pos, m[3]);
        scale[0] = scale[1] = drop->m_size;
        i3dMatScale(m, m, scale);
        const int t = entry[k].m_no >= 5;
        if (t != tex)
        {
            p = i3dPktRefTexRegs(p, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), t));
            tex = t;
        }
        p = i3dPktVu1Matrix(p, m);
        p = i3dPktVu1Sprite(p, m[3], color);
    }
    return i3dPktEndVu1Sprite(p);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016bdd4

void EffBloodSpray1::updateZDepth()
{
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, m_pos);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016be14

EffBloodSpray2::EffBloodSpray2()
{
    m_count = 0;    // callcheck: Eff0 (the base constructor, compiler-made)
    m_scale = 1.0f;
    m_frames = 0;
    m_next = 0;
    m_pos = NULL;
    m_dir = NULL;
    memset(m_drop, 0, sizeof(m_drop));
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016be70

int EffBloodSpray2::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_pos = param->m_spray2.m_pos;
    m_dir = param->m_spray2.m_dir;
    m_scale = param->m_spray2.m_scale;
    m_frames = param->m_spray2.m_frames;
    m_next = effRand();
    emitDrop();
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016bedc

void EffBloodSpray2::emitDrop()
{
    const int step = m_next % 7;
    const int turn = m_next % 9;

    sceVu0FVECTOR dir = { m_dir[0], m_dir[1], m_dir[2], 0.0f };
    i3dVecNormalize2(dir, dir);
    if (effVecLength(dir) <= 0.9f)
    {
        dir[0] = 0.0f;
        dir[1] = -1.0f;
        dir[2] = 0.0f;
        dir[3] = 0.0f;
    }

    float scale = m_scale;
    const float size = scale * 25.0f;
    float grow = size * 0x1.999998p-5f;
    float speed = size * 0.5f;
    for (int i = step; i > 0; i--)
    {
        scale *= 0.95f;
        grow *= 1.02f;
        speed *= 0.95f;
    }

    EffBloodSpray2Drop *drop = &m_drop[m_next % 16];
    drop->m_alpha = 0x7f00;
    drop->m_tex = (short)(((m_next >> 1) ^ 1) & 1);
    drop->m_age = 0;
    drop->m_fade = 0;
    drop->m_fade_add = (short)((int)((float)(step << 8) / scale) + 0x500);
    drop->m_pos[0] = m_pos[0];
    drop->m_pos[1] = m_pos[1];
    drop->m_pos[2] = m_pos[2];
    drop->m_pos[3] = 1.0f;

    sceVu0FMATRIX m;
    i3dMatUnit(m);
    i3dMatRotX(s_spray2Turn[turn][0], m, m);
    i3dMatRotZ(s_spray2Turn[turn][1], m, m);
    i3dMatApply(drop->m_vel, m, dir);
    drop->m_speed = speed;
    drop->m_grow = grow;
    drop->m_size = size;
    drop->m_fall = (fabsf(drop->m_vel[1] * 0x1.999998p-4f) + 0.75f) * (m_scale * 0.015f + 0.04f);
    m_next++;
}

// ──────────────────────────────────────────────────────────────────────
// The live drops age, fade from the 17th frame, move, grow, slow and fall.
// State 0: a drop every second frame until m_frames frames or the finish
// flag, then state 10: over when every drop is gone.
// ROR 0x0016c258

int EffBloodSpray2::liveMain()
{
    for (int i = 0; i < 17; i++)
    {
        EffBloodSpray2Drop *drop = &m_drop[i];
        if (drop->m_alpha <= 0xff)
            continue;
        if (++drop->m_age > 16)
        {
            if (drop->m_alpha < drop->m_fade)
            {
                drop->m_alpha = 0;
                continue;
            }
            drop->m_alpha -= drop->m_fade;
            drop->m_fade += drop->m_fade_add;
        }
        const float speed = drop->m_speed;
        drop->m_pos[1] += drop->m_vel[1] * speed;
        drop->m_pos[0] += drop->m_vel[0] * speed;
        drop->m_pos[2] += drop->m_vel[2] * speed;
        drop->m_size += drop->m_grow;
        drop->m_speed = speed * 0.95f;
        drop->m_vel[1] += drop->m_fall;
    }

    m_08 = m_0c;
    if (m_0c == 0)
    {
        if (++m_count < m_frames && (m_flags & EFF_FLAG_FINISHED) == 0)
        {
            if ((m_count & 1) == 0)
                emitDrop();
            return 1;
        }
        m_0c = 10;
        return 1;
    }
    if (m_0c != 10)
        return 1;
    int i = 0;
    while (i < 17 && m_drop[i].m_alpha < 0x100)
        i++;
    return i != 17;
}

// ──────────────────────────────────────────────────────────────────────
// As EffBloodSpray1's, without the pull toward the eye; the texture the
// drop's own.
// ROR 0x0016c400

u_long *EffBloodSpray2::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    DogCamera *cam = &g_effScene->m_camera_entity;
    EffDepthEntry entry[17];
    int n = 0;
    for (int i = 0; i < 17; i++)
    {
        if (m_drop[i].m_alpha > 0xff)
        {
            entry[n].m_no = i;
            entry[n].m_z = effCalcViewDepth(cam, m_drop[i].m_pos);
            n++;
        }
    }
    if (n <= 0)
        return pkt;
    effSortFarFirst(entry, n);

    cam->callTransform();
    float scale[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    u_long *p = i3dPktCallView(pkt, param->m_cur);
    p = i3dPktSetReg(p, SCE_GS_ALPHA_1, 0x44);
    const float rect[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    p = i3dPktBeginVu1Sprite(p, param->m_cur, 1);
    p = i3dPktVu1Rect(p, rect);
    effAmbientColor(color);
    effFogColor(color, m_z);

    short tex = -1;
    for (int k = 0; k < n; k++)
    {
        const EffBloodSpray2Drop *drop = &m_drop[entry[k].m_no];
        color[3] = (float)drop->m_alpha * (1.0f / 32768.0f);
        sceVu0FMATRIX m;
        memcpy(m, cam->m_matrix, sizeof(sceVu0FVECTOR) * 3);
        memcpy(m[3], drop->m_pos, sizeof(m[3]));
        scale[0] = scale[1] = drop->m_size;
        i3dMatScale(m, m, scale);
        if (drop->m_tex != tex)
        {
            p = i3dPktRefTexRegs(p, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), drop->m_tex));
            tex = drop->m_tex;
        }
        p = i3dPktVu1Matrix(p, m);
        p = i3dPktVu1Sprite(p, m[3], color);
    }
    return i3dPktEndVu1Sprite(p);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016c748

void EffBloodSpray2::updateZDepth()
{
    const float pos[4] = { m_pos[0], m_pos[1], m_pos[2], 1.0f };
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, pos);
}

// ──────────────────────────────────────────────────────────────────────
// 2..4 stains around a centre half the size along the param's direction
// from its point, at random offsets within +-0.4 of the size (stain 0 at
// the centre). Weak: EffMan may evict it when full.
// ROR 0x0016c7c0

int EffBloodStay1::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_flags |= EFF_FLAG_WEAK;
    const float dir[4] = { param->m_hit.m_1c, param->m_hit.m_20, param->m_hit.m_24, 0.0f };
    effMatFromAxisZ(m_matrix, dir);
    m_matrix[3][3] = 1.0f;
    m_matrix[3][0] = param->m_hit.m_pos[0] + m_matrix[2][0] * 0.5f;
    m_matrix[3][1] = param->m_hit.m_pos[1] + m_matrix[2][1] * 0.5f;
    m_matrix[3][2] = param->m_hit.m_pos[2] + m_matrix[2][2] * 0.5f;
    m_size = param->m_hit.m_scale;
    m_frame = 0;
    m_alpha = 1.0f;
    m_life = param->m_hit.m_2c;

    m_count = effRand() % 3 + 2;
    for (int i = 0; i < m_count; i++)
    {
        m_stain[i].m_x = m_size * (((float)effRand() / 255.0f) * 0x1.999998p-1f - 0x1.999998p-2f);
        m_stain[i].m_z = m_size * (((float)effRand() / 255.0f) * 0x1.999998p-1f - 0x1.999998p-2f);
        m_stain[i].m_rot = (((float)effRand() * 360.0f) / 255.0f) * EFF_PI / 180.0f;
    }
    m_stain[0].m_x = 0.0f;
    m_stain[0].m_z = 0.0f;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// State 0 for 120 frames (the stains grow in), 10 until m_life frames or
// the finish flag, 20 fading out by 0.01 a frame.
// ROR 0x0016ca00

int EffBloodStay1::liveMain()
{
    const int state = m_0c;
    m_frame++;
    m_08 = state;
    if (state == 10)
    {
        if ((m_life < 0 || m_frame < m_life) && (m_flags & EFF_FLAG_FINISHED) == 0)
            return 1;
        m_0c = 20;
    }
    else if (state == 0)
    {
        if (m_frame < 121)
            return 1;
        m_0c = 10;
    }
    else if (state == 20)
    {
        m_alpha -= 0.01f;
        if (m_alpha <= 0.0f)
            return 0;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The stains whose delay has passed, texture 0, with the depth test set
// to pass greater-or-equal for them (TEST_1 0x51001) and back to 0x5000d.
// ROR 0x0016cac0

u_long *EffBloodStay1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));
    pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x51001);
    const float rect[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    pkt = i3dPktBeginVu1Sprite(pkt, param->m_cur, 1);

    DRAW draw;
    draw.m_pkt = i3dPktVu1Rect(pkt, rect);
    draw.m_stain = m_stain;
    draw.m_param = param;
    draw.m_depth = effCalcViewDepth(&g_effScene->m_camera_entity, m_matrix[3]);
    if (m_count > 0 && s_stainDelay[0] < m_frame)
    {
        int i = 0;
        do
        {
            draw.m_size = m_size * s_stainScale[i];
            draw.m_age = m_frame - s_stainDelay[i];
            draw.m_pkt = drawStain(&draw);
            draw.m_stain++;
            if (++i >= m_count)
                break;
        } while (s_stainDelay[i] < m_frame);
    }
    pkt = i3dPktEndVu1Sprite(draw.m_pkt);
    return i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x5000d);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016ccb8

u_long *EffBloodStay1::drawStain(DRAW *draw)
{
    float grow = (float)draw->m_age / 20.0f;
    if (1.0f < grow)
        grow = 1.0f;
    sceVu0FMATRIX m;
    memcpy(m, m_matrix, sizeof(m));
    // callcheck: fptodp, dpmul, dptofp (the double sin product, compiler-made)
    const float s = (float)(sin((double)((grow * 45.0f + 45.0f) * EFF_PI / 180.0f)) * (double)draw->m_size);
    const float scale[4] = { s, s, 1.0f, 1.0f };
    i3dMatScale(m, m, scale);

    const float alpha = m_alpha * grow * 2.0f;
    float color[4] = { 1.0f, 1.0f, 1.0f, alpha <= 1.0f ? alpha : 1.0f };
    effAmbientColor(color);
    effFogColor(color, draw->m_depth);

    m[3][0] += draw->m_stain->m_x;
    m[3][1] += -2.0f;
    m[3][2] += draw->m_stain->m_z;
    draw->m_pkt = i3dPktVu1Matrix(draw->m_pkt, m);
    return i3dPktVu1Sprite(draw->m_pkt, m[3], color);
}

// ──────────────────────────────────────────────────────────────────────
// The first box is the centre plus stain 0's three words (x, z, angle added
// to x, y, z), 10 high; then each stain's box at x + its x and z + its
// angle -- the ROM's reads, ported as they are (stain 0's x / z are 0).
// ROR 0x0016cebc

int EffBloodStay1::isActorOnStain(DogActor *actor)
{
    const float half = m_size * 0.5f;
    EffActorProbe probe;
    probe.m_hit = 0;
    probe.m_actor = actor;
    probe.m_center[0] = m_matrix[3][0] + m_stain[0].m_x;
    probe.m_center[1] = m_matrix[3][1] + m_stain[0].m_z;
    probe.m_center[2] = m_matrix[3][2] + m_stain[0].m_rot;
    probe.m_half[0] = half;
    probe.m_half[1] = 10.0f;
    probe.m_half[2] = half;
    probe.m_margin = half;
    if (!effProbeActorInBox(&probe))
        return 0;
    for (int i = 0; i < m_count; i++)
    {
        probe.m_center[0] = m_matrix[3][0] + m_stain[i].m_x;
        probe.m_center[2] = m_matrix[3][2] + m_stain[i].m_rot;
        probe.m_hit = 0;
        probe.m_half[0] = half * s_stainScale[i];
        probe.m_half[2] = probe.m_half[0];
        effProbeActorLimbs(&probe);
        if (probe.m_hit & 0xf)
            return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016d01c

int EffBloodStay2::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    const float *pos = param->m_trail.m_pos;
    m_pos = pos;
    m_up = param->m_trail.m_up;
    m_width = param->m_trail.m_width;
    m_len_done = 0.0f;
    m_len = 0.0f;
    m_step = param->m_trail.m_length / 31.0f;
    m_last[0] = pos[0];
    m_last[1] = pos[1];
    m_last[2] = pos[2];
    m_last[3] = 1.0f;
    m_dir[0] = 0.0f;
    m_count = 0;
    m_dir[3] = 0.0f;
    m_dir[2] = 0.0f;
    m_dir[1] = 0.0f;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// State 0 until the caller moves (the first edge at the start), then 10:
// edge m_count follows the caller, and a segment is finished (the next
// edge starts there) when it is m_step long or turns more than 90 degrees
// less its share of m_step; -1 after 31.
// ROR 0x0016d0d0

int EffBloodStay2::liveMain()
{
    m_08 = m_0c;
    if (m_0c == -1)
        return 1;
    const sceVu0FVECTOR cur = { m_pos[0], m_pos[1], m_pos[2], 1.0f };
    const sceVu0FVECTOR up = { m_up[0], m_up[1], m_up[2], 0.0f };
    bool again;
    do
    {
        again = false;
        sceVu0FVECTOR d, dir;
        i3dVecSub(d, cur, m_last);
        const float len = effVecLength(d);
        if (0.0f < len)
            i3dVecNormalize2(dir, d);
        else
            memcpy(dir, m_dir, sizeof(dir));
        m_08 = m_0c;
        if (m_0c == 0)
        {
            if (0.0f < len)
            {
                setEdge(m_last, up, dir);
                again = true;
                m_v[m_count] = 0.0f;
                memcpy(m_dir, dir, sizeof(m_dir));
                m_0c = 10;
                m_count++;
            }
        }
        else if (m_0c == 10)
        {
            setEdge(cur, up, dir);
            m_v[m_count] = m_v[m_count - 1] + len / (m_width * 3.0f);
            m_len = m_len_done + len;
            // callcheck: fptodp, dpcmp (the double compare, compiler-made)
            if (m_step <= len
                || (double)i3dVecDot(dir, m_dir)
                       < cos((double)((90.0f - len * 90.0f / m_step) * EFF_PI / 180.0f)))
            {
                m_count++;
                memcpy(m_last, cur, sizeof(m_last));
                memcpy(m_dir, dir, sizeof(m_dir));
                again = true;
                m_len_done = m_len;
            }
            if (m_count > 31)
            {
                m_count = 31;
                again = false;
                m_0c = -1;
            }
        }
    } while (again);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016d3e4

void EffBloodStay2::setEdge(const float *at, const float *up, const float *dir)
{
    sceVu0FVECTOR side;
    i3dVecCross2(side, up, dir);
    i3dVecNormalize2(side, side);
    EffBloodStay2Edge *edge = &m_edge[m_count];
    edge->m_b[3] = 1.0f;
    edge->m_a[3] = 1.0f;
    i3dVecScale(m_width * 0.5f, edge->m_a, side);
    i3dVecAdd(edge->m_a, edge->m_a, at);
    i3dVecScale(-m_width * 0.5f, edge->m_b, side);
    i3dVecAdd(edge->m_b, edge->m_b, at);
}

// ──────────────────────────────────────────────────────────────────────
// One 4-vertex strip per segment (no depth write: ZBUF_1 0x13a0000e0),
// lifted half of m_up, texture 0 tiled along the trail (t = m_v). The
// colour's alpha is the previous segment's end: 127, toward the caller
// fading out over the last width (0 at the head).
// ROR 0x0016d4bc

u_long *EffBloodStay2::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    if (m_count <= 0)
        return pkt;
    pkt = i3dPktSetReg(pkt, SCE_GS_ZBUF_1, 0x13a0000e0ull);
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));
    pkt = i3dPktCallFog(pkt, param->m_cur);

    sceVu0FMATRIX m;
    i3dMatUnit(m);
    m[3][0] = m_up[0] * 0.5f;
    m[3][1] = m_up[1] * 0.5f;
    m[3][2] = m_up[2] * 0.5f;

    sceVu0FVECTOR xyz[4];
    memset(xyz, 0, sizeof(xyz));
    /* i3dPktDrawStripColor reads only color[0] (the ROM keeps four). */
    sceVu0FVECTOR color[4] = {
        { 255.0f, 255.0f, 255.0f, 127.0f }, { 255.0f, 255.0f, 255.0f, 127.0f },
        { 255.0f, 255.0f, 255.0f, 127.0f }, { 255.0f, 255.0f, 255.0f, 127.0f },
    };
    sceVu0FVECTOR stq[4] = {
        { 0.0f, 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.0f },
    };
    memcpy(xyz[2], m_edge[0].m_a, sizeof(xyz[2]));
    memcpy(xyz[3], m_edge[0].m_b, sizeof(xyz[3]));
    for (int i = 1; i <= m_count; i++)
    {
        memcpy(xyz[0], xyz[2], sizeof(xyz[0]));
        memcpy(xyz[1], xyz[3], sizeof(xyz[1]));
        memcpy(xyz[2], m_edge[i].m_a, sizeof(xyz[2]));
        memcpy(xyz[3], m_edge[i].m_b, sizeof(xyz[3]));
        float whole;
        modff(m_v[i - 1], &whole);
        const float t0 = m_v[i - 1] - whole;
        const float t1 = m_v[i] - whole;
        stq[0][1] = t0;
        stq[1][1] = t0;
        stq[2][1] = t1;
        stq[3][1] = t1;
        color[0][3] = color[2][3];
        color[1][3] = color[2][3];
        if (i == m_count)
        {
            color[2][3] = 0.0f;
            color[3][3] = 0.0f;
        }
        else
        {
            float a = (m_len - m_width * 3.0f * m_v[i]) / m_width;
            if (a > 1.0f)
                a = 1.0f;
            a = 1.0f < a ? 127.0f : a * 127.0f;
            color[2][3] = a;
            color[3][3] = a;
        }
        pkt = i3dPktDrawStripColor(pkt, m, 4, stq, color[0], xyz, NULL);
    }
    return pkt;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016d804

EffBloodStay3::~EffBloodStay3()
{
    g_effBloodStay3 = -1;   // callcheck: ~Eff0, operator_delete (the base destructor and the deleting variant, compiler-made)
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016d870

int EffBloodStay3::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    memset(m_count, 0, sizeof(m_count));
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016d8ac

int EffBloodStay3::liveMain()
{
    for (int i = 0; i < 2; i++)
    {
        if (m_count[i] > 0)
            return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016d8e8

int effSetBloodStay3(int n, int count)
{
    if (g_effBloodStay3 == -1)
    {
        EffParam param;
        memset(&param, 0, sizeof(param));   /* PC: zeroed (the ROM sets kind and m_08) */
        param.m_kind = 5;
        param.m_08 = 0;
        g_effBloodStay3 = g_effScene->m_eff_man.createEff(&param);
        if (g_effBloodStay3 == -1)
            return 0;
    }
    static_cast<EffBloodStay3 *>(g_effScene->m_eff_man.m_eff[g_effBloodStay3])->set(n, count);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016d9a8

int EffBloodStay3Prim::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_type = param->m_prim.m_type;
    m_flip = param->m_prim.m_flip;
    m_pos[0] = param->m_prim.m_pos[0];
    m_pos[1] = param->m_prim.m_pos[1];
    m_pos[2] = param->m_prim.m_pos[2];
    m_pos[3] = 1.0f;
    m_rot = param->m_prim.m_rot;
    m_frame = 0;
    m_alpha = param->m_prim.m_alpha;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016da30

int EffBloodStay3Prim::liveMain()
{
    return ++m_frame < 165;
}

// ──────────────────────────────────────────────────────────────────────
// A flat sprite (z down) turned m_rot about y, one half of texture 0
// (rect u, v, width, height: type 0 the left half, mirrored when m_flip;
// type 1 the right half, mirrored with m_flip bit 2); faded in over 15
// frames and out from frame 135 over 30. Another type draws a zero-size
// sprite from an unset rect (PC: zeros).
// ROR 0x0016da44

u_long *EffBloodStay3Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));
    pkt = i3dPktBeginVu1Sprite(pkt, param->m_cur, 1);

    float rect[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float size = 0.0f;
    if (m_type == 0 || m_type == 1)
    {
        if (m_type == 0)
        {
            rect[0] = m_flip == 0 ? 0.0f : 0.5f;
            rect[2] = m_flip == 0 ? 0.5f : -0.5f;
        }
        else if ((m_flip & 2) == 0)
        {
            rect[0] = 0.5f;
            rect[2] = 0.5f;
        }
        else
        {
            rect[0] = 1.0f;
            rect[2] = -0.5f;
        }
        rect[1] = 0.0f;
        rect[3] = 1.0f;
        size = 1.0f;
    }
    pkt = i3dPktVu1Rect(pkt, rect);

    float color[4] = { 1.0f, 1.0f, 1.0f, m_alpha };
    effAmbientColor(color);
    if (m_frame < 15)
        color[3] *= (float)m_frame / 15.0f;
    else if (m_frame >= 135)
        color[3] *= 1.0f - (float)(m_frame - 135) / 30.0f;

    float v[4];
    memset(v, 0, sizeof(v));
    v[1] = -1.0f;
    sceVu0FMATRIX m;
    effMatFromAxisZ(m, v);
    i3dMatRotY(m_rot, m, m);
    v[0] = size;
    v[1] = size;
    v[2] = 1.0f;
    v[3] = 1.0f;
    i3dMatScale(m, m, v);
    pkt = i3dPktVu1Matrix(pkt, m);
    pkt = i3dPktVu1Sprite(pkt, m_pos, color);
    return i3dPktEndVu1Sprite(pkt);
}
