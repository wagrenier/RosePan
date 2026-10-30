/* ==========================================================================
 *  eff/effbreath.cpp
 *
 *  EffBreath1 / EffBreath1Space / EffBreath2 / EffBreath2Prim /
 *  EffBreath2Ground (0x16dcc8..0x16f71c): the actors' breath in a box and
 *  the ClrOut monster's breath stream and ground puffs. See eff/effbreath.h.
 * ======================================================================== */
#include "eff/effbreath.h"

#include <float.h>
#include <math.h>
#include <string.h>

#include "dog/dogactor.h"
#include "dog/dogcamera.h"
#include "eff/effcommon.h"
#include "eff/effman.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dvu1pkt.h"
#include "mag/magscene.h"
#include "sdk/libgraph.h"

/* EffBreath2Ground's four puffs: size and alpha factors and first frame. */
static const float s_groundScale[4] = { 1.0f, 0.9f, 0.8f, 0.7f };       // data 0x002f2278
static const float s_groundAlpha[4] = { 1.0f, 0.8f, 0.6f, 0.5f };       // data 0x002f2288
static const int   s_groundStart[4] = { 0, 6, 8, 12 };                  // data 0x002f2298

// ──────────────────────────────────────────────────────────────────────
// The box and the sprites' look from the param; the mouth offset from
// the head node (characters 0x24xx further out), the first puff 8 frames
// away, the ring started at the mouth.
// callcheck: dbgAssertFailed, i3rGetNodeIndexByName (in the inline getNodeIndexByName)
// ROR 0x0016dcc8

int EffBreath1::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_min[0] = param->m_breath1.m_min[0];
    m_min[1] = param->m_breath1.m_min[1];
    m_min[2] = param->m_breath1.m_min[2];
    m_max[0] = param->m_breath1.m_max[0];
    m_max[1] = param->m_breath1.m_max[1];
    m_max[2] = param->m_breath1.m_max[2];
    m_scale = param->m_breath1.m_scale;
    m_color[0] = param->m_breath1.m_color[0];
    m_color[1] = param->m_breath1.m_color[1];
    m_color[2] = param->m_breath1.m_color[2];
    m_color[3] = param->m_breath1.m_color[3];
    m_actor = param->m_breath1.m_actor;
    m_node = m_actor->getNodeIndexByName("Bip01 Head");
    if ((m_actor->m_id & 0xff00) == 0x2400)
    {
        m_mouth[2] = 0.0f;
        m_mouth[0] = 8.0f;
        m_mouth[1] = 25.0f;
        m_mouth[3] = 1.0f;
    }
    else
    {
        m_mouth[0] = 0.0f;
        m_mouth[1] = 14.0f;
        m_mouth[2] = 0.0f;
        m_mouth[3] = 1.0f;
    }
    m_alpha = 0.0f;
    m_frame = 62;
    m_alpha_step = 0.0f;
    m_head = 0;
    addPoint();
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The mouth onto the ring; every 70 frames a new puff while the actor is
// still in the box (else the next try in 15 frames), otherwise the puff
// fades in, out and grows. Never over (EffBreath1Space releases it).
// ROR 0x0016de40

int EffBreath1::liveMain()
{
    addPoint();
    m_frame = (m_frame + 1) % 70;
    if (m_frame == 0)
    {
        if (isActorInBox())
        {
            m_size = 8.0f;
            m_alpha_step = 0.06f;
            m_grow = 0.0f;
            m_alpha = 0.0f;
        }
        else
        {
            m_alpha = 0.0f;
            m_frame = 55;
            m_alpha_step = 0.0f;
            m_size = 0.0f;
            m_grow = 0.0f;
        }
    }
    else
    {
        m_alpha += m_alpha_step;
        if (1.0f <= m_alpha)
        {
            m_alpha = 1.0f;
            m_alpha_step = -0.015f;
        }
        m_grow += 0.015f;
        m_size += m_grow;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// While the puff shows: up to 8 points walked back along the ring from
// the newest, the first at the mouth and the rest 10 apart, each risen by
// ((frame + k) * (frame + k + 1) / 2) * 0.0035 (y grows downward); drawn
// farthest first as camera-facing sprites, point k (m_size + k(k+1)/2 *
// 0.015) * m_scale wide and, past the first, fainter by (9 - k) * 3 / 32.
// The camera matrix is the sprites' matrix, with its translation, as the
// ROM has it.
// ROR 0x0016df38

u_long *EffBreath1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    if (!(0.0f < m_alpha))
        return pkt;

    int n = 0;
    int seg = 0;
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));
    float rect[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    pkt = i3dPktBeginVu1Sprite(pkt, param->m_cur, 1);
    pkt = i3dPktVu1Rect(pkt, rect);

    DogCamera *cam = &g_effScene->m_camera_entity;
    const float step[EFF_BREATH1_POINTS] = { 0.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f };
    float len[EFF_BREATH1_POINTS];
    for (int i = 0; i < EFF_BREATH1_POINTS; i++)
        len[i] = m_point[i].m_len;

    sceVu0FVECTOR pos[EFF_BREATH1_POINTS];
    float depth[EFF_BREATH1_POINTS];
    for (int k = 0; k < EFF_BREATH1_POINTS; k++)
    {
        float d = step[k];
        if (seg < EFF_BREATH1_POINTS)
        {
            int cur;
            float l;
            for (;;)
            {
                cur = (m_head - seg + 8) % 8;
                l = len[cur];
                if (d <= l)
                    break;
                seg++;
                d -= l;
                if (seg > 7)
                    break;
            }
            if (seg > 7)
                break;
            const int prev = (cur + 7) % 8;
            n++;
            effMoveToward(l - d, pos[k], m_point[prev].m_pos, m_point[cur].m_pos);
            const int t = m_frame + k;
            pos[k][1] += (float)((t * t + t) / 2) * -0x1.cac082p-9f;   /* -0.0035 (0xbb656041) */
            len[cur] -= d;
        }
    }

    for (int i = 0; i < n; i++)
        depth[i] = effCalcViewDepth(cam, pos[i]);

    for (int left = n; left > 0; left--)
    {
        int best = 0;
        for (int j = 0; j < n; j++)
        {
            if (depth[best] < depth[j])
                best = j;
        }
        cam->callTransform();
        sceVu0FMATRIX m;
        memcpy(m, cam->m_matrix, sizeof(m));
        const float s = (m_size + (float)((best * best + best) / 2) * 0.015f) * m_scale;
        float v[4] = { s, s, 1.0f, 1.0f };
        i3dMatScale(m, m, v);
        pkt = i3dPktVu1Matrix(pkt, m);
        float alpha = m_alpha;
        if (best != 0)
            alpha = alpha * (float)(9 - best) * 0.75f * 0.125f;
        v[0] = m_color[0];
        v[1] = m_color[1];
        v[2] = m_color[2];
        v[3] = m_color[3] * alpha;
        effAmbientColor(v);
        pkt = i3dPktVu1Sprite(pkt, pos[best], v);
        /* PC: the ROM stores 0xffffffff, which the EE's FPU reads as its
         * most negative number (a NaN on the host, never picked again). */
        depth[best] = -FLT_MAX;
    }
    return i3dPktEndVu1Sprite(pkt);
}

// ──────────────────────────────────────────────────────────────────────
// The newest ring point's depth.
// ROR 0x0016e3ec

void EffBreath1::updateZDepth()
{
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, m_point[m_head].m_pos);
}

// ──────────────────────────────────────────────────────────────────────
// The mouth (m_mouth through the head node's world matrix) as the next
// ring point with its distance from the newest; within 3 on x and z and
// 5 on y of it, it keeps the newest's y (the ROM compares in doubles).
// callcheck: fptodp, dpcmp, dpsub (the EE's soft-double runtime for the fabs compares)
// ROR 0x0016e438

void EffBreath1::addPoint()
{
    DogActor *actor = m_actor;
    sceVu0FMATRIX *node = actor->getNodeMatrixPtr(m_node);
    actor->callTransform();
    sceVu0FMATRIX m;
    i3dMatMul(m, actor->m_matrix, *node);
    sceVu0FVECTOR p;
    i3dMatApply(p, m, m_mouth);

    const float *last = m_point[m_head].m_pos;
    if (fabs((double)(p[0] - last[0])) < 3.0 &&
        fabs((double)(p[2] - last[2])) < 3.0 &&
        fabs((double)(p[1] - last[1])) < 5.0)
        p[1] = last[1];

    const int next = (m_head + 1) % EFF_BREATH1_POINTS;
    m_point[next].m_pos[0] = p[0];
    m_point[next].m_pos[1] = p[1];
    m_point[next].m_pos[2] = p[2];
    m_point[next].m_pos[3] = p[3];
    i3dVecSub(p, p, m_point[m_head].m_pos);
    m_point[next].m_len = effVecLength(p);
    m_head = next;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016e65c

int EffBreath1::isActorInBox()
{
    const float *t = m_actor->m_trans;
    return m_min[0] <= t[0] && t[0] <= m_max[0] &&
           m_min[2] <= t[2] && t[2] <= m_max[2] &&
           m_min[1] <= t[1] && t[1] <= m_max[1];
}

// ──────────────────────────────────────────────────────────────────────
// The box from its two corners (any order); no actor has a breath yet.
// ROR 0x0016e6e4

int EffBreath1Space::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    for (int i = 0; i < 3; i++)
    {
        const float a = param->m_breath1space.m_box[0][i];
        const float b = param->m_breath1space.m_box[1][i];
        if (a <= b)
        {
            m_min[i] = a;
            m_max[i] = b;
        }
        else
        {
            m_min[i] = b;
            m_max[i] = a;
        }
    }
    m_scale = param->m_breath1space.m_scale;
    m_color[0] = param->m_breath1space.m_color[0];
    m_color[1] = param->m_breath1space.m_color[1];
    m_color[2] = param->m_breath1space.m_color[2];
    m_color[3] = param->m_breath1space.m_color[3];
    m_frame = 0;
    memset(m_eff, 0xff, sizeof(m_eff));
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// One group of 8 actors a frame: a breath for a loaded actor in the box
// that has none, the breath released when its actor's model is gone.
// Never over.
// callcheck: I3rRoot__CheckIdString, dbgAssertFailed (in the inline isAlive)
// ROR 0x0016e7cc

int EffBreath1Space::liveMain()
{
    m_frame++;
    const int first = (int)(m_frame & 0xf) * 8;
    for (int i = first; i < first + 8; i++)
    {
        DogActor *actor = &g_effScene->m_actors[i];
        if (m_eff[i] == -1)
        {
            if (actor->isAlive() && isActorInBox(actor))
                createBreath(i);
        }
        else if (!actor->isAlive())
        {
            g_effScene->m_eff_man.release(m_eff[i]);
            m_eff[i] = -1;
        }
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016e92c

void EffBreath1Space::createBreath(int no)
{
    EffParam p;
    p.m_kind = 8;
    p.m_08 = 0;
    p.m_breath1.m_min[0] = m_min[0];
    p.m_breath1.m_min[1] = m_min[1];
    p.m_breath1.m_min[2] = m_min[2];
    p.m_breath1.m_max[0] = m_max[0];
    p.m_breath1.m_max[1] = m_max[1];
    p.m_breath1.m_max[2] = m_max[2];
    p.m_breath1.m_scale = m_scale;
    p.m_breath1.m_color[0] = m_color[0];
    p.m_breath1.m_color[1] = m_color[1];
    p.m_breath1.m_color[2] = m_color[2];
    p.m_breath1.m_color[3] = m_color[3];
    p.m_breath1.m_actor = &g_effScene->m_actors[no];
    m_eff[no] = g_effScene->m_eff_man.createEff(&p);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed, i3rGetNodeIndexByName (in the inline getNodeIndexByName)
// ROR 0x0016e9f4

int EffBreath1Space::isActorInBox(DogActor *actor)
{
    if (actor->getNodeIndexByName("Bip01 Head") == -1)
        return 0;
    const float *t = actor->m_trans;
    return m_min[0] <= t[0] && t[0] <= m_max[0] &&
           m_min[2] <= t[2] && t[2] <= m_max[2] &&
           m_min[1] <= t[1] && t[1] <= m_max[1];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016eb00

int EffBreath2::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_pos = param->m_breath.m_pos;
    m_dir = param->m_breath.m_dir;
    m_frames = param->m_breath.m_frames;
    m_scale = param->m_breath.m_scale;
    m_drag = param->m_breath.m_drag;
    m_34 = param->m_breath.m_20;
    m_color[0] = param->m_breath.m_color[0];
    m_color[1] = param->m_breath.m_color[1];
    m_color[2] = param->m_breath.m_color[2];
    m_color[3] = param->m_breath.m_color[3];
    m_rand = effRand();
    m_frame = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// On frames 1, 3, 4, 6, 7 of every 8 (the bits of 0xda), a puff from the
// caller's position along its direction turned up to +-15 degrees /
// m_scale about x and about z, the texture half alternating; over after
// m_frames frames or when finished.
// ROR 0x0016eb98

int EffBreath2::liveMain()
{
    m_frame++;
    if ((0xda >> (m_frame & 7)) & 1)
    {
        const int rx = effRand();
        const float s = m_scale;
        const int rz = effRand();
        sceVu0FVECTOR dir = { m_dir[0], m_dir[1], m_dir[2], 0.0f };
        sceVu0FVECTOR out = { dir[0], dir[1], dir[2], 0.0f };
        /* pi / 12 (0x3e860a91) less pi / 6 (0x3f060a91) times the roll. */
        const float az = (0x1.0c1522p-2f - ((float)rz * 0x1.0c1522p-1f) / 255.0f) / m_scale;
        sceVu0FMATRIX m;
        i3dMatUnit(m);
        i3dMatRotX((0x1.0c1522p-2f - ((float)rx * 0x1.0c1522p-1f) / 255.0f) / s, m, m);
        i3dMatRotZ(az, m, m);
        i3dMatApply(out, m, dir);

        EffParam p;
        p.m_kind = 0xb;
        p.m_08 = 0;
        p.m_breath2prim.m_pos[0] = m_pos[0];
        p.m_breath2prim.m_pos[1] = m_pos[1];
        p.m_breath2prim.m_pos[2] = m_pos[2];
        p.m_breath2prim.m_dir[0] = out[0];
        p.m_breath2prim.m_dir[1] = out[1];
        p.m_breath2prim.m_dir[2] = out[2];
        p.m_breath2prim.m_scale = m_scale;
        p.m_breath2prim.m_drag = m_drag;
        p.m_breath2prim.m_30 = m_34;
        p.m_breath2prim.m_color[0] = m_color[0];
        p.m_breath2prim.m_color[1] = m_color[1];
        p.m_breath2prim.m_color[2] = m_color[2];
        p.m_breath2prim.m_color[3] = m_color[3];
        p.m_breath2prim.m_half = m_frame & 1;
        g_effScene->m_eff_man.createEff(&p);
    }
    if (m_frame < m_frames && !(m_flags & EFF_FLAG_FINISHED))
        return 1;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Its speed 7.5 * m_scale along the direction.
// ROR 0x0016edc0

int EffBreath2Prim::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_pos[0] = param->m_breath2prim.m_pos[0];
    m_pos[1] = param->m_breath2prim.m_pos[1];
    m_pos[2] = param->m_breath2prim.m_pos[2];
    m_pos[3] = 1.0f;
    m_color[0] = param->m_breath2prim.m_color[0];
    m_color[1] = param->m_breath2prim.m_color[1];
    m_color[2] = param->m_breath2prim.m_color[2];
    m_color[3] = param->m_breath2prim.m_color[3];
    m_vel[0] = param->m_breath2prim.m_dir[0] * 7.5f * param->m_breath2prim.m_scale;
    m_vel[1] = param->m_breath2prim.m_dir[1] * 7.5f * param->m_breath2prim.m_scale;
    m_vel[3] = 0.0f;
    m_vel[2] = param->m_breath2prim.m_dir[2] * 7.5f * param->m_breath2prim.m_scale;
    m_50 = param->m_breath2prim.m_30;
    m_drag = param->m_breath2prim.m_drag;
    /* PC: the ROM adds 5 to whatever the heap left at 0x54 (nothing sets
     * it before); the host starts from 0. */
    m_size = 0.0f + 5.0f;
    m_half = param->m_breath2prim.m_half;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// It moves; over once below the floor (y past 0, y grows downward), else
// slowed on x / z by 1 - m_drag / 40, pulled down by m_drag / 2 and grown 2.5.
// ROR 0x0016eeb4

int EffBreath2Prim::liveMain()
{
    i3dVecAdd(m_pos, m_pos, m_vel);
    if (!(m_pos[1] <= 0.0f))
        return 0;
    float f = 1.0f - m_drag * 0x1.999998p-6f;      /* 0.025 (0x3ccccccc) */
    if (!(f <= 1.0f))
        f = 1.0f;
    m_vel[0] *= f;
    m_vel[2] *= f;
    m_vel[1] += m_drag * 0.5f;
    m_size += 2.5f;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// A camera-facing sprite m_size wide, texture 0's m_half half, lit and
// fogged at its depth. The camera matrix is the sprite's matrix, with its
// translation, as the ROM has it.
// ROR 0x0016ef74

u_long *EffBreath2Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    DogCamera *cam = &g_effScene->m_camera_entity;
    cam->callTransform();
    sceVu0FMATRIX m;
    memcpy(m, cam->m_matrix, sizeof(m));
    float color[4] = { m_color[0], m_color[1], m_color[2], m_color[3] };
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));
    float v[4] = { (float)m_half * 0.5f, 0.0f, (float)(m_half + 1) * 0.5f, 1.0f };
    pkt = i3dPktBeginVu1Sprite(pkt, param->m_cur, 1);
    pkt = i3dPktVu1Rect(pkt, v);
    v[0] = m_size;
    v[1] = m_size;
    v[2] = 1.0f;
    v[3] = 1.0f;
    i3dMatScale(m, m, v);
    effAmbientColor(color);
    effFogColor(color, m_z);
    pkt = i3dPktVu1Matrix(pkt, m);
    pkt = i3dPktVu1Sprite(pkt, m_pos, color);
    return i3dPktEndVu1Sprite(pkt);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016f160

void EffBreath2Prim::updateZDepth()
{
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, m_pos);
}

// ──────────────────────────────────────────────────────────────────────
// Weak; 2 above the given point (y grows downward), at least 17 frames;
// the puffs but the first at random offsets up to 0.6 * m_size on x and
// z, each at a random y angle.
// ROR 0x0016f1a0

int EffBreath2Ground::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_flags |= EFF_FLAG_WEAK;
    m_pos[0] = param->m_ground.m_pos[0];
    m_pos[1] = param->m_ground.m_pos[1] - 2.0f;
    m_pos[3] = 1.0f;
    m_pos[2] = param->m_ground.m_pos[2];
    m_size = param->m_ground.m_size;
    m_color[0] = param->m_ground.m_color[0];
    m_color[1] = param->m_ground.m_color[1];
    m_color[2] = param->m_ground.m_color[2];
    m_color[3] = param->m_ground.m_color[3];
    m_frames = 17;
    m_frame = 0;
    if (17 <= param->m_ground.m_frames)
        m_frames = param->m_ground.m_frames;
    for (int i = 0; i < 4; i++)
    {
        /* 1.2 (0x3f999999) and 0.6 (0x3f199999), truncated. */
        m_puff[i].m_x = m_size * (((float)effRand() / 255.0f) * 0x1.333332p+0f - 0x1.333332p-1f);
        m_puff[i].m_z = m_size * (((float)effRand() / 255.0f) * 0x1.333332p+0f - 0x1.333332p-1f);
        /* pi as 0x40490fda. */
        m_puff[i].m_rot = ((((float)effRand() * 360.0f) / 255.0f) * 0x1.921fb4p+1f) / 180.0f;
    }
    m_puff[0].m_x = 0.0f;
    m_puff[0].m_z = 0.0f;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// m_frames frames; finished early (EFF_FLAG_FINISHED), it skips to the
// fade out 15 before the end.
// ROR 0x0016f380

int EffBreath2Ground::liveMain()
{
    m_frame++;
    if (m_frame >= m_frames)
        return 0;
    if ((m_flags & EFF_FLAG_FINISHED) && m_frame < m_frames - 15)
        m_frame = m_frames - 15;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Texture 1, drawn with the depth test passing greater-or-equal (TEST_1
// 0x51001, back to 0x5000d); each puff from its start frame. In the last
// 16 frames a puff counts its frames from its own start, so the later
// ones fade out later (and are cut off at the end).
// ROR 0x0016f3c8

u_long *EffBreath2Ground::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 1));
    pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x51001);
    float rect[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    pkt = i3dPktBeginVu1Sprite(pkt, param->m_cur, 1);

    EffBreath2GroundDraw d;
    d.m_pkt = i3dPktVu1Rect(pkt, rect);
    d.m_puff = m_puff;
    d.m_param = param;
    d.m_depth = effCalcViewDepth(&g_effScene->m_camera_entity, m_pos);
    for (int i = 0; i < 4; i++)
    {
        if (s_groundStart[i] <= m_frame)
        {
            d.m_alpha = s_groundAlpha[i];
            d.m_scale = s_groundScale[i];
            d.m_frame = (m_frames - m_frame < 16) ? m_frame - s_groundStart[i] : m_frame;
            d.m_pkt = renderPuff(&d);
        }
        d.m_puff++;
    }
    pkt = i3dPktEndVu1Sprite(d.m_pkt);
    return i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x5000d);
}

// ──────────────────────────────────────────────────────────────────────
// The puff lying flat (turned -90 degrees about x, then its y angle),
// m_size * its factor wide, grown in over the effect's first 2 frames; its
// alpha in over its first 2 frames and out over the last 15; lit and
// fogged at the centre's depth.
// ROR 0x0016f55c

u_long *EffBreath2Ground::renderPuff(EffBreath2GroundDraw *d)
{
    sceVu0FMATRIX m;
    i3dMatUnit(m);
    float grow = (float)m_frame * 0.5f;
    if (1.0f < grow)
        grow = 1.0f;
    const float s = m_size * d->m_scale * grow;
    float v[4] = { s, s, 1.0f, 1.0f };                      // 0x2f2230
    i3dMatScale(m, m, v);
    i3dMatRotX(-0x1.921fb2p+0f, m, m);                      /* -pi / 2 (0xbfc90fd9) */
    i3dMatRotY(d->m_puff->m_rot, m, m);

    /* PC: the ROM leaves w unset. */
    const float pos[4] = { m_pos[0] + d->m_puff->m_x, m_pos[1], m_pos[2] + d->m_puff->m_z, 1.0f };
    float color[4] = { m_color[0], m_color[1], m_color[2], m_color[3] };
    float alpha;
    if (d->m_frame < 2)
        alpha = (float)d->m_frame * 0.5f;
    else
    {
        const int left = m_frames - d->m_frame;
        alpha = (left < 15) ? (float)left / 15.0f : 1.0f;
    }
    color[3] = color[3] * (alpha * d->m_alpha);
    effAmbientColor(color);
    effFogColor(color, d->m_depth);
    d->m_pkt = i3dPktVu1Matrix(d->m_pkt, m);
    return i3dPktVu1Sprite(d->m_pkt, pos, color);
}
