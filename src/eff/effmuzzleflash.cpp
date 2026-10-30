/* ==========================================================================
 *  eff/effmuzzleflash.cpp
 *
 *  EffMuzzleFlash1 (0x1788ec..0x1790e4): a shot's flash, smoke and light.
 *  See eff/effmuzzleflash.h.
 * ======================================================================== */
#include "eff/effmuzzleflash.h"

#include <string.h>

#include "dog/dogcamera.h"
#include "dog/doglightpool.h"
#include "eff/effman.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dvu1pkt.h"
#include "mag/magscene.h"
#include "sdk/libgraph.h"

static const float EFF_PI = 0x1.921fb4p+1f;     /* 3.1415925, the ROM's pi */

/* An alpha / velocity short to a 0..1 alpha (1 / 32768). */
static const float EFF_MUZZLE_ALPHA = 0x1p-15f;

// ──────────────────────────────────────────────────────────────────────
// The muzzle position from the param, both parts' start, a random roll
// (r * 17 degrees for the flash, r * 20 for the smoke, r = effRand() %
// 18), and a point light in light sets 0 and 1 at the muzzle, black.
// callcheck: dbgAssertFailed (DogLightPool::getPointLight's inline asserts, line 131)
// ROR 0x001788ec

int EffMuzzleFlash1::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;

    u_char r = effRand();
    m_pos[0] = param->m_muzzle.m_pos[0];
    m_pos[1] = param->m_muzzle.m_pos[1];
    m_pos[2] = param->m_muzzle.m_pos[2];
    m_pos[3] = 1.0f;
    m_flash.m_alpha = 0x7f00;
    m_flash.m_size = 10.0f;
    m_flash.m_alpha_vel = (short)0xce00;
    m_smoke.m_size = 40.0f;
    m_smoke.m_alpha = 0x6000;
    m_smoke.m_alpha_vel = (short)0xf900;
    m_flash.m_yoff = 0.0f;
    m_smoke.m_yoff = 0.0f;
    u_int turn = (u_char)(r % 18);
    m_flash.m_rot = ((float)(turn * 17) * EFF_PI) / 180.0f;
    m_smoke.m_rot = ((float)(turn * 20) * EFF_PI) / 180.0f;

    m_light = g_effScene->addLight(DOG_LIGHT_POINT, 3);
    /* PORT: the ROM writes light -1 (before the array) when none is free;
     * the host skips the light. */
    if (m_light < 0)
        return 1;
    DogPointLight *light = g_effScene->m_light_pool.getPointLight(m_light);
    memcpy(light->m_pos, m_pos, sizeof(light->m_pos));
    light->m_color[0] = 0.0f;
    light->m_color[1] = 0.0f;
    light->m_color[2] = 0.0f;
    light->m_color[3] = 0.0f;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The flash grows 20 (its alpha velocity -0x180) and sets the light's
// grey to its alpha x 150; once gone the light is removed. The smoke grows
// 2.5 (velocity +0x40), rises 2.25 and turns 1.5 degrees. 0 when both are
// gone.
// callcheck: dbgAssertFailed (DogLightPool::getPointLight's inline asserts, line 131)
// ROR 0x00178ac0

int EffMuzzleFlash1::liveMain()
{
    if (m_flash.m_alpha < 0x100)
    {
        if (m_light != -1)
        {
            g_effScene->m_light_pool.removeLight(DOG_LIGHT_POINT, m_light);
            m_light = -1;
        }
    }
    else
    {
        stepPart(20.0f, &m_flash, -0x180);
        /* PORT: no light when init found none free (see init). */
        if (m_light >= 0)
        {
            DogPointLight *light = g_effScene->m_light_pool.getPointLight(m_light);
            light->m_color[3] = 0.0f;
            float grey = (float)m_flash.m_alpha * 150.0f * EFF_MUZZLE_ALPHA;
            light->m_color[0] = grey;
            light->m_color[2] = grey;
            light->m_color[1] = grey;
        }
    }

    if (m_smoke.m_alpha >= 0x100)
    {
        stepPart(2.5f, &m_smoke, 0x40);
        m_smoke.m_yoff -= 2.25f;
        m_smoke.m_rot += 0x1.acee9cp-6f;    /* 1.5 degrees */
        if (m_smoke.m_alpha >= 0x100)
            return 1;
    }
    return m_flash.m_alpha >= 0x100;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00178c54

void EffMuzzleFlash1::stepPart(float grow, EffMuzzlePart *part, short accel)
{
    short vel = part->m_alpha_vel;
    part->m_alpha_vel = (short)(accel + vel);
    part->m_alpha = (short)(part->m_alpha + vel);
    part->m_size += grow;
}

// ──────────────────────────────────────────────────────────────────────
// The two sprites far to near: the smoke's depth is taken at its risen
// position.
// ROR 0x00178c84

u_long *EffMuzzleFlash1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    u_long *p = i3dPktCallView(pkt, param->m_cur);
    const float rect[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    p = i3dPktBeginVu1Sprite(p, param->m_cur, 1);
    p = i3dPktVu1Rect(p, rect);

    DogCamera *cam = &g_effScene->m_camera_entity;
    sceVu0FVECTOR v;
    memcpy(v, m_pos, sizeof(v));
    float flash = effCalcViewDepth(cam, v);
    v[1] += m_smoke.m_yoff;
    float smoke = effCalcViewDepth(cam, v);
    if (flash < smoke)
    {
        p = drawSmoke(param, set, p);
        p = drawFlash(param, set, p);
    }
    else
    {
        p = drawFlash(param, set, p);
        p = drawSmoke(param, set, p);
    }
    return i3dPktEndVu1Sprite(p);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00178dd8

u_long *EffMuzzleFlash1::drawSmoke(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    if (m_smoke.m_alpha < 0x100)
        return pkt;
    EffMuzzleDraw draw;
    draw.m_cam = &g_effScene->m_camera_entity;
    draw.m_set = set;
    draw.m_pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    draw.m_part = &m_smoke;
    draw.m_push = 1.0f;
    draw.m_tex = 1;
    drawPart(&draw);
    return draw.m_pkt;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00178e68

u_long *EffMuzzleFlash1::drawFlash(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    if (m_flash.m_alpha < 0x100)
        return pkt;
    EffMuzzleDraw draw;
    draw.m_cam = &g_effScene->m_camera_entity;
    draw.m_set = set;
    draw.m_pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x48);
    draw.m_part = &m_flash;
    draw.m_push = 0.0f;
    draw.m_tex = 0;
    drawPart(&draw);
    return draw.m_pkt;
}

// ──────────────────────────────────────────────────────────────────────
// The matrix's row 3 is the sprite's centre too (the ROM builds it there).
// callcheck: DogEntity::callTransform (the camera's transform slot, called through its vtable)
// ROR 0x00178eec

void EffMuzzleFlash1::drawPart(EffMuzzleDraw *draw)
{
    DogCamera *cam = draw->m_cam;
    u_long *pkt = draw->m_pkt;
    cam->callTransform();
    sceVu0FMATRIX m;
    memcpy(m, cam->m_matrix, sizeof(m));
    const EffMuzzlePart *part = draw->m_part;
    const float scale[4] = { part->m_size, part->m_size, 1.0f, 1.0f };
    const float color[4] = { 1.0f, 1.0f, 1.0f, (float)part->m_alpha * EFF_MUZZLE_ALPHA };
    effMatMulRotZ(part->m_rot, m);
    i3dMatScale(m, m, scale);
    float push = draw->m_push;
    m[3][0] = m_pos[0] + m[2][0] * push;
    m[3][1] = m_pos[1] + m[2][1] * push + part->m_yoff;
    m[3][2] = m_pos[2] + m[2][2] * push;
    m[3][3] = m_pos[3];
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(draw->m_set), draw->m_tex));
    pkt = i3dPktVu1Matrix(pkt, m);
    draw->m_pkt = i3dPktVu1Sprite(pkt, m[3], color);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001790a4

void EffMuzzleFlash1::updateZDepth()
{
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, m_pos);
}
