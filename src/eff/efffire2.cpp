/* ==========================================================================
 *  eff/efffire2.cpp
 *
 *  EffFire2 (0x17145c..0x1716bc): a fire sprite placed in the world. See
 *  eff/efffire2.h.
 * ======================================================================== */
#include "eff/efffire2.h"

#include "dog/dogcamera.h"
#include "eff/effman.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dvu1pkt.h"
#include "mag/magscene.h"
#include "sdk/libgraph.h"

// ──────────────────────────────────────────────────────────────────────
// The matrix (rotate z x y, scale m_scale in x and y), the position and
// the alpha.
// ROR 0x0017145c

int EffFire2::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    float v[4];
    v[0] = param->m_fire2.m_rot[0];
    v[1] = param->m_fire2.m_rot[1];
    v[2] = param->m_fire2.m_rot[2];
    v[3] = 0.0f;
    i3dMatUnit(m_matrix);
    i3dMatRotZXY(m_matrix, m_matrix, v);
    v[0] = param->m_fire2.m_scale;
    v[1] = param->m_fire2.m_scale;
    v[2] = 1.0f;
    v[3] = 1.0f;
    i3dMatScale(m_matrix, m_matrix, v);
    m_pos[0] = param->m_fire2.m_pos[0];
    m_pos[1] = param->m_fire2.m_pos[1];
    m_pos[2] = param->m_fire2.m_pos[2];
    /* PC: the ROM leaves w as operator new left it. */
    m_pos[3] = 1.0f;
    m_frame = 0;
    m_alpha = param->m_fire2.m_alpha;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00171534

int EffFire2::liveMain()
{
    m_frame++;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Quarter (m_frame >> 2) & 3 of texture 0 (left to right, then down).
// ROR 0x00171548

u_long *EffFire2::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));
    pkt = i3dPktBeginVu1Sprite(pkt, param->m_cur, 1);
    const u_int cell = ((u_int)m_frame >> 2) & 3;
    const u_int row = cell >> 1;
    float rect[4];
    rect[1] = (float)row * 0.5f;
    rect[0] = (float)(int)(cell - row * 2) * 0.5f;
    rect[3] = rect[1] + 0.5f;
    rect[2] = rect[0] + 0.5f;
    pkt = i3dPktVu1Rect(pkt, rect);
    const float color[4] = { 1.0f, 1.0f, 1.0f, m_alpha };
    pkt = i3dPktVu1Matrix(pkt, m_matrix);
    pkt = i3dPktVu1Sprite(pkt, m_pos, color);
    return i3dPktEndVu1Sprite(pkt);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017167c

void EffFire2::updateZDepth()
{
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, m_pos);
}
