/* ==========================================================================
 *  eff/efffoot.cpp
 *
 *  EffFoot1 / EffFoot1Prim / EffFoot2 / EffFoot2Prim (0x172e4c..0x173df8):
 *  the dust puffs and footprints at the PC's and the dog's steps. See
 *  eff/efffoot.h.
 * ======================================================================== */
#include "eff/efffoot.h"

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

/* The print state of the dog (0x828618) and of the PC (0x828648). */
static EffFoot2Work s_dogWork;                                          // data 0x00828618
static EffFoot2Work s_pcWork;                                           // data 0x00828648

// ──────────────────────────────────────────────────────────────────────
// The box from its two corners (any order), and the puffs' parameters.
// ROR 0x00172e4c

int EffFoot1::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    for (int i = 0; i < 3; i++)
    {
        const float a = param->m_foot1.m_box[0][i];
        const float b = param->m_foot1.m_box[1][i];
        if (a <= b)
        {
            m_center[i] = a;
            m_size[i] = b;
        }
        else
        {
            m_center[i] = b;
            m_size[i] = a;
        }
        m_size[i] -= m_center[i];
        m_center[i] += m_size[i] * 0.5f;
    }
    m_tex = param->m_foot1.m_tex;
    m_mode = param->m_foot1.m_mode;
    m_scale = param->m_foot1.m_scale;
    m_alpha = param->m_foot1.m_alpha;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The PC's actor, then the dog's, probed when loaded and shown. Never over.
// ROR 0x00172f4c

int EffFoot1::liveMain()
{
    // callcheck: I3rRoot__CheckIdString, dbgAssertFailed (in the inlines isAlive and getCurDog)
    MagScene *scene = g_effScene;
    DogActor *pc = scene->m_pc.getActor();
    DogActor *dog = scene->getCurDog();
    if (pc != NULL && pc->isAlive() && !pc->isHide())
        probeActor(pc);
    if (dog->isAlive() && !dog->isHide())
        probeActor(dog);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// When the actor stands in the box (m_size goes in as the probe's half
// sizes, as the ROM has it; margin 100), a puff at each limb whose step
// event fires over its next frame.
// ROR 0x001730d4

void EffFoot1::probeActor(DogActor *actor)
{
    EffActorProbe probe;
    probe.m_hit = 0;
    probe.m_actor = actor;
    probe.m_center[0] = m_center[0];
    probe.m_center[1] = m_center[1];
    probe.m_center[2] = m_center[2];
    probe.m_half[0] = m_size[0];
    probe.m_half[1] = m_size[1];
    probe.m_half[2] = m_size[2];
    probe.m_margin = 100.0f;
    if (!effProbeActorInBox(&probe))
        return;

    EffParam p;
    p.m_kind = 0x1e;
    p.m_08 = 0;
    p.m_foot1prim.m_tex = m_tex;
    p.m_foot1prim.m_mode = m_mode;
    p.m_foot1prim.m_scale = m_scale;
    p.m_foot1prim.m_alpha = m_alpha;
    effProbeActorLimbs(&probe);
    for (int i = 0; i < 4; i++)
    {
        if (probe.m_hit & (1u << i))
        {
            p.m_foot1prim.m_pos[0] = probe.m_pos[i][0];
            p.m_foot1prim.m_pos[1] = probe.m_pos[i][1];
            p.m_foot1prim.m_pos[2] = probe.m_pos[i][2];
            g_effScene->m_eff_man.createEff(&p);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001731f0

int EffFoot1Prim::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_pos[0] = param->m_foot1prim.m_pos[0];
    m_pos[1] = param->m_foot1prim.m_pos[1];
    m_pos[2] = param->m_foot1prim.m_pos[2];
    m_pos[3] = 1.0f;
    m_tex = param->m_foot1prim.m_tex;
    m_mode = param->m_foot1prim.m_mode;
    m_scale = param->m_foot1prim.m_scale;
    m_frame = 0;
    m_alpha = param->m_foot1prim.m_alpha;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00173274

int EffFoot1Prim::liveMain()
{
    switch (m_mode)
    {
    case EFF_FOOT1_PUFF: return liveMode0();
    case 1:              return liveMode1();
    case 2:              return liveMode2();
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001732d8

u_long *EffFoot1Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    switch (m_mode)
    {
    case EFF_FOOT1_PUFF: return renderMode0(param, set, pkt);
    case 1:              return renderMode1(param, set, pkt);
    case 2:              return renderMode2(param, set, pkt);
    }
    return pkt;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00173340

void EffFoot1Prim::updateZDepth()
{
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, m_pos);
}

// ──────────────────────────────────────────────────────────────────────
// 20 frames: alive while 1 - frame / 20 stays above 0.
// ROR 0x00173380

int EffFoot1Prim::liveMode0()
{
    m_frame++;
    return 0.0f < (float)m_frame * -0x1.999998p-5f + 1.0f;     /* -0.05 (0xbd4ccccc) */
}

// ROR 0x00173610
int EffFoot1Prim::liveMode1() { return 0; }

// ROR 0x00173620
int EffFoot1Prim::liveMode2() { return 0; }

// ──────────────────────────────────────────────────────────────────────
// The puff: a camera-facing sprite (frame + 15) * m_scale wide, centred
// half that toward -y from the limb, pulled 20 toward the camera, its
// alpha m_alpha fading to 0 over 20 frames, lit by the ambient colour.
// The camera matrix gets the sprite's position as its row 3, as the ROM
// has it.
// ROR 0x001733cc

u_long *EffFoot1Prim::renderMode0(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), m_tex));
    float pos[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    pkt = i3dPktBeginVu1Sprite(pkt, param->m_cur, 1);
    pkt = i3dPktVu1Rect(pkt, pos);

    DogCamera *cam = &g_effScene->m_camera_entity;
    cam->callTransform();
    sceVu0FMATRIX m;
    memcpy(m, cam->m_matrix, sizeof(m));
    float color[4] = { 1.0f, 1.0f, 1.0f,
                       ((float)m_frame * -0x1.999998p-5f + 1.0f) * m_alpha };   /* -0.05 (0xbd4ccccc) */
    const float size = ((float)m_frame + 15.0f) * m_scale;
    const float scale[4] = { size, size, 1.0f, 1.0f };
    pos[0] = m_pos[0];
    pos[1] = m_pos[1] - size * 0.5f;
    pos[2] = m_pos[2];
    pos[3] = m_pos[3];
    effMoveToward(20.0f, pos, pos, m[3]);
    i3dMatScale(m, m, scale);
    m[3][0] = pos[0];
    m[3][1] = pos[1];
    m[3][2] = pos[2];
    m[3][3] = pos[3];
    effAmbientColor(color);
    pkt = i3dPktVu1Matrix(pkt, m);
    pkt = i3dPktVu1Sprite(pkt, m[3], color);
    return i3dPktEndVu1Sprite(pkt);
}

// ROR 0x00173618
u_long *EffFoot1Prim::renderMode1(RENDERPARAM *, const I3dDrawSet *, u_long *pkt) { return pkt; }

// ROR 0x00173628
u_long *EffFoot1Prim::renderMode2(RENDERPARAM *, const I3dDrawSet *, u_long *pkt) { return pkt; }

// ──────────────────────────────────────────────────────────────────────
// The two work records: the dog's paw prints (EFF_FOOT2_DOG), the PC's
// shoe prints, size 10, no prints left.
// ROR 0x00173630

int EffFoot2::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    s_dogWork.m_scale = 10.0f;
    s_pcWork.m_scale = 10.0f;
    s_dogWork.m_type = EFF_FOOT2_DOG;
    s_pcWork.m_type = 0;
    s_pcWork.m_count = 0;
    s_dogWork.m_count = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// While a work record has prints left, its actor's steps (the PC's, then
// the dog's) when loaded and shown. Never over.
// ROR 0x00173688

int EffFoot2::liveMain()
{
    // callcheck: I3rRoot__CheckIdString, dbgAssertFailed (in the inlines isAlive and getCurDog)
    if (0 < s_pcWork.m_count)
    {
        DogActor *pc = g_effScene->m_pc.getActor();
        if (pc != NULL && pc->isAlive() && !pc->isHide())
            checkSteps(pc, &s_pcWork);
    }
    if (0 < s_dogWork.m_count)
    {
        DogActor *dog = g_effScene->getCurDog();
        if (dog->isAlive() && !dog->isHide())
            checkSteps(dog, &s_dogWork);
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The actor's motion events over its next frame: one print per step bit
// (left toe, right toe, left finger, right finger; effProbeActorLimbs'
// bits). Nothing while no motion plays (frame < 0).
// ROR 0x00173840

void EffFoot2::checkSteps(DogActor *actor, EffFoot2Work *work)
{
    const float t = actor->getMotionFrame();
    if (t < 0.0f)
        return;
    const u_int ev = actor->getMotionEvent(t, t + 1.0f, -1);
    if (ev & 0x10000000)
        addPrint(actor, "Bip01 L Toe0", work);
    if (ev & 0x20000000)
        addPrint(actor, "Bip01 R Toe0", work);
    if (ev & 0x4000000)
        addPrint(actor, "Bip01 L Finger0", work);
    if (ev & 0x8000000)
        addPrint(actor, "Bip01 R Finger0", work);
}

// ──────────────────────────────────────────────────────────────────────
// A kind-0x20 print at the node's x / z, at the actor's own y, turned by
// its y angle, work's type and size; one print fewer left (the count is
// not checked again here: four steps in a frame may take it below 0).
// ROR 0x00173948

void EffFoot2::addPrint(DogActor *actor, const char *name, EffFoot2Work *work)
{
    // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (in the inline getNodeIndexByName)
    const float y = actor->m_trans[1];
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    actor->getNodeWorldPos(pos, actor->getNodeIndexByName(name));

    EffParam p;
    p.m_kind = 0x20;
    p.m_08 = 0;
    p.m_foot2prim.m_type = work->m_type;
    p.m_foot2prim.m_pos[0] = pos[0];
    p.m_foot2prim.m_pos[1] = y;
    p.m_foot2prim.m_pos[2] = pos[2];
    p.m_foot2prim.m_rot = actor->m_rotate[1];
    p.m_foot2prim.m_scale = work->m_scale;
    g_effScene->m_eff_man.createEff(&p);
    work->m_count--;
}

// ──────────────────────────────────────────────────────────────────────
// Weak (EffMan may evict it for a newer effect); y 2 less than given.
// ROR 0x00173a90

int EffFoot2Prim::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_flags |= EFF_FLAG_WEAK;
    m_type = param->m_foot2prim.m_type;
    m_pos[0] = param->m_foot2prim.m_pos[0];
    m_pos[1] = param->m_foot2prim.m_pos[1];
    m_pos[2] = param->m_foot2prim.m_pos[2];
    m_pos[3] = 1.0f;
    m_pos[1] += -2.0f;
    m_rot = param->m_foot2prim.m_rot;
    m_scale = param->m_foot2prim.m_scale;
    m_frame = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// 180 frames; finished early (EFF_FLAG_FINISHED), it skips to the fade
// out at 165.
// ROR 0x00173b2c

int EffFoot2Prim::liveMain()
{
    if ((m_flags & EFF_FLAG_FINISHED) && m_frame < 165)
        m_frame = 165;
    m_frame++;
    return m_frame < 180;
}

// ──────────────────────────────────────────────────────────────────────
// The print lying flat (its z axis up the world's y), turned by m_rot,
// m_scale wide and three times as long for the shoe; texture 0 at u
// 0..0.25 and v 0..0.75 (the shoe) or 0.75..1 (the paw; its u is 4..4.25,
// (m_type & 0x10) * 16 / 64 as the ROM has it, the same with the texture
// repeating). Lit and fogged, alpha in over 15 frames and out from 165,
// drawn with the depth test passing greater-or-equal (TEST_1 0x51001,
// back to 0x5000d).
// ROR 0x00173b68

u_long *EffFoot2Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };            // 0x2f27e0
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));
    pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x51001);

    float v[4];
    float length;
    if (m_type & EFF_FOOT2_DOG)
    {
        v[1] = 0.75f;
        v[3] = 1.0f;
        length = 1.0f;
    }
    else
    {
        v[1] = 0.0f;
        v[3] = 0.75f;
        length = 3.0f;
    }
    const int u = (m_type & EFF_FOOT2_DOG) * 0x10;
    v[0] = (float)u * 0.015625f;
    v[2] = (float)(u + 0x10) * 0.015625f;
    pkt = i3dPktBeginVu1Sprite(pkt, param->m_cur, 1);
    pkt = i3dPktVu1Rect(pkt, v);

    sceVu0FMATRIX m;
    v[0] = 0.0f;
    v[1] = 1.0f;
    v[2] = 0.0f;
    v[3] = 0.0f;
    effMatFromAxisZ(m, v);
    i3dMatRotY(m_rot, m, m);
    v[0] = m_scale;
    v[1] = m_scale * length;
    v[2] = 1.0f;
    v[3] = 1.0f;
    i3dMatScale(m, m, v);

    const sceVu0FVECTOR pos = { m_pos[0], m_pos[1], m_pos[2], m_pos[3] };
    const float depth = effCalcViewDepth(&g_effScene->m_camera_entity, m_pos);
    effAmbientColor(color);
    effFogColor(color, depth);
    float alpha = 1.0f;
    if (m_frame < 15)
        alpha = (float)m_frame / 15.0f;
    else if (165 <= m_frame)
        alpha = (float)(180 - m_frame) / 15.0f;
    color[3] = alpha;
    pkt = i3dPktVu1Matrix(pkt, m);
    pkt = i3dPktVu1Sprite(pkt, pos, color);
    pkt = i3dPktEndVu1Sprite(pkt);
    return i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x5000d);
}
