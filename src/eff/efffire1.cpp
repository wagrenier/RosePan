/* ==========================================================================
 *  eff/efffire1.cpp
 *
 *  EffFire1 / EffFire1Ex (0x170ca4..0x17145c): the candle flames. See
 *  eff/efffire1.h.
 * ======================================================================== */
#include "eff/efffire1.h"

#include <string.h>

#include "dog/dogactor.h"
#include "dog/dogcamera.h"
#include "eff/effman.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dvu1pkt.h"
#include "mag/magscene.h"
#include "sdk/libgraph.h"

/* The flicker's 96 steps, one bit per 2 frames (stack-built in liveMain). */
static const u_int s_flickerBits[3] = { 0x52a53d2a, 0xa5656951, 0x56512b2a };

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00170ca4

int EffFire1::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_pos[0] = param->m_fire1.m_pos[0];
    m_pos[1] = param->m_fire1.m_pos[1];
    m_pos[2] = param->m_fire1.m_pos[2];
    m_pos[3] = 1.0f;
    m_color[0] = param->m_fire1.m_color[0];
    m_color[1] = param->m_fire1.m_color[1];
    m_color[2] = param->m_fire1.m_color[2];
    m_color[3] = param->m_fire1.m_color[3];
    m_near = param->m_fire1.m_near;
    m_frame = effRand();
    m_flicker = 1.0f;
    m_fade = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The flicker: down 0.25 a frame (not below 0), up 0.75 (not above 1)
// where the pattern has a 1. Never over.
// ROR 0x00170d44

int EffFire1::liveMain()
{
    m_frame++;
    m_flicker += -0.25f;
    if (m_flicker < 0.0f)
        m_flicker = 0.0f;
    const u_int bit = ((u_int)m_frame >> 1) % 96;
    if (s_flickerBits[bit >> 5] & (1u << (bit & 0x1f)))
    {
        m_flicker += 0.75f;
        if (1.0f < m_flicker)
            m_flicker = 1.0f;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Nothing when hidden with no fade running. The flame: the camera matrix
// made upright, 5 x 10, 5 above m_pos, sheet frame (m_frame / 6) & 7,
// white, its alpha m_fade / 5 while a fade runs. The glow: the camera
// matrix, 100 x 100, 2 above m_pos pulled m_near + 10 toward the eye,
// m_color with its alpha times the fade and 0.85 + 0.15 m_flicker.
// ROR 0x00170e14

u_long *EffFire1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    if ((m_flags & (EFF_FLAG_HIDDEN | EFF_FLAG_ACTIVATE)) == EFF_FLAG_HIDDEN)
        return pkt;
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktBeginVu1Sprite(pkt, param->m_cur, 1);
    float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    if (m_flags & EFF_FLAG_ACTIVATE)
        color[3] = (float)m_fade / 5.0f;
    const float fade = color[3];
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));

    DogCamera *cam = &g_effScene->m_camera_entity;
    cam->callTransform();
    sceVu0FMATRIX m;
    memcpy(m, cam->m_matrix, sizeof(m));
    static const float flameScale[4] = { 5.0f, 10.0f, 1.0f, 1.0f };    /* data 0x2f2400 */
    effMatMakeUpright(m);
    i3dMatScale(m, m, flameScale);
    m[3][0] = m_pos[0];
    m[3][1] = m_pos[1] + -5.0f;
    m[3][2] = m_pos[2];
    m[3][3] = m_pos[3];
    pkt = i3dPktVu1Matrix(pkt, m);
    const u_int cell = (u_int)(m_frame / 6) & 7;
    const u_int row = cell >> 2;
    float rect[4];
    rect[1] = (float)row * 0.5f;
    rect[0] = (float)(int)(cell - row * 4) * 0.25f;
    rect[3] = rect[1] + 0.5f;
    rect[2] = rect[0] + 0.25f;
    pkt = i3dPktVu1Rect(pkt, rect);
    pkt = i3dPktVu1Sprite(pkt, m[3], color);

    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x48);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 1));
    memcpy(m, cam->m_matrix, sizeof(m));
    float eye[4];
    memcpy(eye, m[3], sizeof(eye));
    static const float glowScale[4] = { 100.0f, 100.0f, 1.0f, 1.0f };
    i3dMatScale(m, m, glowScale);
    m[3][0] = m_pos[0];
    m[3][1] = m_pos[1] + -0x1.fffffep+0f;     /* -1.9999999 */
    m[3][2] = m_pos[2];
    m[3][3] = m_pos[3];
    effMoveToward(m_near + 10.0f, m[3], m[3], eye);
    pkt = i3dPktVu1Matrix(pkt, m);
    rect[0] = 0.0f;
    rect[2] = 1.0f;
    rect[1] = 0.0f;
    rect[3] = 1.0f;
    pkt = i3dPktVu1Rect(pkt, rect);
    color[0] = m_color[0];
    color[1] = m_color[1];
    color[2] = m_color[2];
    color[3] = fade * m_color[3] * (m_flicker * 0x1.333332p-3f + 0x1.b33332p-1f);  /* 0.14999999, 0.84999996 */
    pkt = i3dPktVu1Sprite(pkt, m[3], color);
    return i3dPktEndVu1Sprite(pkt);
}

// ──────────────────────────────────────────────────────────────────────
// The glow's depth, less its pull, so the flame sorts in front.
// ROR 0x001711fc

void EffFire1::updateZDepth()
{
    float pos[4];
    pos[0] = m_pos[0];
    pos[1] = m_pos[1] + -0x1.fffffep+0f;      /* -1.9999999 */
    pos[2] = m_pos[2];
    pos[3] = m_pos[3];
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, pos) - (m_near + 10.0f);
}

// ──────────────────────────────────────────────────────────────────────
// The fade setVisible starts: hidden counts m_fade up to 5, shown down to
// 0, then the pending flag is cleared.
// ROR 0x00171278

void EffFire1::liveActivateExec()
{
    if (m_flags & EFF_FLAG_HIDDEN)
    {
        if (++m_fade < 5)
            return;
    }
    else
    {
        if (0 < --m_fade)
            return;
    }
    m_flags &= ~EFF_FLAG_ACTIVATE;
}

// ──────────────────────────────────────────────────────────────────────
// The actor, its node name and the glow's colour; no pull, m_frame 0.
// ROR 0x001712bc

int EffFire1Ex::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_actor = param->m_achr.m_actor;
    memcpy(m_node, param->m_achr.m_node, sizeof(m_node));
    m_color[0] = param->m_achr.m_color[0];
    m_color[1] = param->m_achr.m_color[1];
    m_color[2] = param->m_achr.m_color[2];
    m_color[3] = param->m_achr.m_color[3];
    m_pos[3] = 1.0f;
    m_near = 0.0f;
    m_frame = 0;
    m_fade = 0;
    /* PC: the ROM leaves m_flicker as operator new left it (liveMain
     * clamps it from the first frame on); the host starts it at 0. */
    m_flicker = 0.0f;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// m_pos follows the actor's node, then EffFire1's depth.
// callcheck: i3rGetNodeIndexByName, dbgAssertFailed (in the inline DogModel::getNodeIndexByName)
// ROR 0x00171378

void EffFire1Ex::updateZDepth()
{
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    DogActor *actor = m_actor;
    /* PC: the ROM reads through a NULL actor (native 281 op 38 with an
     * actor number that has none); the host keeps m_pos. */
    if (actor != NULL)
    {
        /* DogModel::getNodeIndexByName, inlined (dog/dogmodel.h 305). */
        int node = actor->getNodeIndexByName(m_node);
        actor->getNodeWorldPos(pos, node); /* EE slot 18 */
        m_pos[0] = pos[0];
        m_pos[1] = pos[1];
        m_pos[2] = pos[2];
    }
    EffFire1::updateZDepth();
}
