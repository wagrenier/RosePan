/* ==========================================================================
 *  eff/efflight.cpp
 *
 *  EffLight1, EffLight2 (0x174dd8..0x17562c): glow sprites. EffLight3
 *  (0x17562c..0x176694): lightning. See eff/efflight.h.
 * ======================================================================== */
#include "eff/efflight.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "dog/dogcamera.h"
#include "eff/effman.h"
#include "game/gamman.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dvu1pkt.h"
#include "i3d/i3rroot.h"
#include "i3d/i3rtype.h"
#include "mag/magscene.h"
#include "sdk/libgraph.h"
#include "snd/sndman.h"

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

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017562c

EffLight3::EffLight3()
{
    m_mode = -1;    // callcheck: Eff0 (the base constructor, compiler-made)
}

// ──────────────────────────────────────────────────────────────────────
// PORT: the display the destructor leaves the sky nodes on. The ROM
// clears m_disp to these values and points the nodes at it while the
// object dies; on the host they keep the same values without pointing
// into freed memory.
static DogDispControl s_restDisp;

// ──────────────────────────────────────────────────────────────────────
// Once init ran: the sky nodes back to a plain display, the scene's
// ambient back to its colour, the light copies removed.
// ROR 0x00175664

EffLight3::~EffLight3()
{
    if (m_mode != -1)
    {
        m_disp.m_25 = 1;
        m_disp.m_24 = 1;
        m_disp.m_20 = 0;
        for (int i = 0; i < 4; i++)
        {
            m_disp.m_color[i] = 0.0f;
            m_disp.m_10[i] = 1.0f;
        }
        s_restDisp = m_disp;    // callcheck: ~Eff0, operator_delete (the base destructor and the deleting variant, compiler-made)
        setSkyNodeDisp(&s_restDisp);
        setAmbientScale(1.0f);
        DogLightPool *pool = &g_effScene->m_light_pool;
        for (int type = 0; type < DOG_MAX_LIGHT_TYPE; type++)
        {
            for (int i = 0; m_copy[type][i].m_id != -1; i++)
                pool->removeLight(type, m_copy[type][i].m_id);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// Mode 0 copies light set 1's direct, point and spot lights (their
// colours flash, liveMain); one liveMain at a far-off thunder sets the
// rest state, then the first thunder comes in 16..136 frames.
// ROR 0x00175804

int EffLight3::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    m_mode = param->m_light3.m_mode;
    m_disp.m_20 = 0;
    m_disp.m_25 = 1;
    for (int i = 0; i < 4; i++)
    {
        m_disp.m_color[i] = 0.0f;
        m_disp.m_10[i] = 1.0f;
    }
    m_disp.m_24 = 1;

    DogLightPool *pool = &g_effScene->m_light_pool;
    memcpy(m_ambient, pool->getAmbientLight(0)->m_color, sizeof(m_ambient));   // callcheck: dbgAssertFailed (the inline DogLightPool getters' asserts, doglightpool.h lines 129..132)
    memset(m_copy, 0xff, sizeof(m_copy));
    if (m_mode == 0)
    {
        addLights(DOG_LIGHT_DIRECT);
        addLights(DOG_LIGHT_POINT);
        addLights(DOG_LIGHT_SPOT);
        for (int i = 0; m_copy[DOG_LIGHT_DIRECT][i].m_src != -1; i++)
        {
            DogDirectLight *dst = pool->getDirectLight(m_copy[DOG_LIGHT_DIRECT][i].m_id);
            *dst = *pool->getDirectLight(m_copy[DOG_LIGHT_DIRECT][i].m_src);
        }
        for (int i = 0; m_copy[DOG_LIGHT_POINT][i].m_src != -1; i++)
        {
            DogPointLight *dst = pool->getPointLight(m_copy[DOG_LIGHT_POINT][i].m_id);
            *dst = *pool->getPointLight(m_copy[DOG_LIGHT_POINT][i].m_src);
        }
        for (int i = 0; m_copy[DOG_LIGHT_SPOT][i].m_src != -1; i++)
        {
            DogSpotLight *dst = pool->getSpotLight(m_copy[DOG_LIGHT_SPOT][i].m_id);
            *dst = *pool->getSpotLight(m_copy[DOG_LIGHT_SPOT][i].m_src);
        }
        m_ambient_rest = 0.9f;
        m_ambient_flash = 0.3f;
    }
    else
    {
        m_ambient_rest = 1.0f;
        m_ambient_flash = 2.0f;
    }

    m_timer = 0x58;
    m_interval = 0x5a;
    m_ambient_fade = 0;
    m_flash = 0;
    liveMain();
    m_timer = 0x10;
    m_interval = m_timer + (effRand() * 120) / 255;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The thunder every 90..210 frames and its pattern; a flash runs 5..0
// (the sky nodes lit by m_flash / 5, then mode 1's ambient or mode 0's
// light copies at 2x / 4x that), and after one mode 0's ambient comes
// back from m_ambient_flash to m_ambient_rest over 30 frames.
// ROR 0x00175dc4

int EffLight3::liveMain()
{
    DogLightPool *pool = &g_effScene->m_light_pool;    // callcheck: dbgAssertFailed (the inline DogLightPool getters' asserts, lines 130..132)

    if (++m_timer >= m_interval)
    {
        m_timer = 0;
        m_interval = (effRand() * 120) / 255 + 90;
        m_pattern = getFlashPattern(effRand() & 3);
        m_pattern_bit = 0;
        int se = g_gamMan.m_sndMan.playSE(0xb030352, false, NULL, 0);
        g_sndMan.setSeLoop(se, 0);
    }
    if (m_timer < 0x10 && (m_timer & 3) == 0)
    {
        if (m_pattern & (1u << m_pattern_bit))
        {
            m_flash = 5;
            m_ambient_fade = 30;
        }
        m_pattern_bit++;
    }

    if (m_flash >= 0)
    {
        if (m_flash < 1)
        {
            m_disp.m_10[0] = 0.0f;
            m_disp.m_color[2] = 0.0f;
            m_disp.m_color[1] = 0.0f;
            m_disp.m_color[0] = 0.0f;
            m_disp.m_10[2] = 0.0f;
            m_disp.m_10[1] = 0.0f;
        }
        else
        {
            float f = (float)m_flash / 5.0f;
            m_disp.m_10[0] = 1.0f;
            m_disp.m_10[2] = 1.0f;
            m_disp.m_10[1] = 1.0f;
            m_disp.m_color[0] = f;
            m_disp.m_color[2] = f;
            m_disp.m_color[1] = f;
        }
        setSkyNodeDisp(&m_disp);

        if (m_mode == 1)
            setAmbientScale(m_ambient_rest + ((m_ambient_flash - m_ambient_rest) * (float)m_flash) / 5.0f);
        else
        {
            float f = (float)m_flash;
            float s = (f + f) / 5.0f;
            for (int i = 0; m_copy[DOG_LIGHT_DIRECT][i].m_src != -1; i++)
            {
                const DogDirectLight *src = pool->getDirectLight(m_copy[DOG_LIGHT_DIRECT][i].m_src);
                DogDirectLight *dst = pool->getDirectLight(m_copy[DOG_LIGHT_DIRECT][i].m_id);
                dst->m_color[0] = src->m_color[0] * s;
                dst->m_color[1] = src->m_color[1] * s;
                dst->m_color[2] = src->m_color[2] * s;
            }
            s = ((float)m_flash * 4.0f) / 5.0f;
            for (int i = 0; m_copy[DOG_LIGHT_POINT][i].m_src != -1; i++)
            {
                const DogPointLight *src = pool->getPointLight(m_copy[DOG_LIGHT_POINT][i].m_src);
                DogPointLight *dst = pool->getPointLight(m_copy[DOG_LIGHT_POINT][i].m_id);
                dst->m_color[0] = src->m_color[0] * s;
                dst->m_color[1] = src->m_color[1] * s;
                dst->m_color[2] = src->m_color[2] * s;
            }
            s = ((float)m_flash * 4.0f) / 5.0f;
            for (int i = 0; m_copy[DOG_LIGHT_SPOT][i].m_src != -1; i++)
            {
                const DogSpotLight *src = pool->getSpotLight(m_copy[DOG_LIGHT_SPOT][i].m_src);
                DogSpotLight *dst = pool->getSpotLight(m_copy[DOG_LIGHT_SPOT][i].m_id);
                dst->m_color[0] = src->m_color[0] * s;
                dst->m_color[1] = src->m_color[1] * s;
                dst->m_color[2] = src->m_color[2] * s;
            }
        }
        m_flash--;
    }

    if (m_ambient_fade >= 0)
    {
        if (m_mode != 1)
            setAmbientScale(m_ambient_rest + ((m_ambient_flash - m_ambient_rest) * (float)m_ambient_fade) / 30.0f);
        m_ambient_fade--;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Each light of kind type in light set 1 (bits 0..15 of its mask): its
// bit, and a new light of that kind in set 1 (MagScene::addLight mask 2).
// ROR 0x00176430

void EffLight3::addLights(int type)
{
    u_int mask = g_effScene->m_light_pool.m_set_mask[1][type];
    int n = 0;
    for (int i = 0; i < 0x10; i++)
    {
        if (mask & (1u << i))
        {
            m_copy[type][n].m_src = i;
            m_copy[type][n].m_id = g_effScene->addLight(type, 2);
            n++;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001764fc

u_int EffLight3::getFlashPattern(int n)
{
    const u_int pattern[4] = { 0xd, 0x3, 0xb, 0x7 };
    return pattern[n];
}

// ──────────────────────────────────────────────────────────────────────
// The first name the model lacks ends the run.
// ROR 0x0017653c

void EffLight3::setSkyNodeDisp(const DogDispControl *disp)
{
    if (g_effScene == NULL)
        return;
    I3rRoot *root = g_effScene->m_tenkyu[0].m_model[0].getI3rRootPtr();
    if (root == NULL)
        return;
    char name[32];
    for (int i = 0;; i++)
    {
        sprintf(name, "light3_%02d", i);
        int n = i3rGetNodeIndexByName(root, name);
        if (n < 0)
            break;
        I3rItem *slot = i3rGetSlot2A(i3rGetChild52_3(&root->m_item, n));
        /* PORT: the ROM stores the pointer; the host field holds a far
         * pointer (the effect is not in the file). */
        i3rFarPtrSet(&slot->m_data, disp);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001765e8

void EffLight3::setAmbientScale(float scale)
{
    DogAmbientLight *light = g_effScene->m_light_pool.getAmbientLight(0);  // callcheck: dbgAssertFailed (its inline assert, line 129)
    light->m_color[0] = m_ambient[0] * scale;
    light->m_color[1] = m_ambient[1] * scale;
    light->m_color[2] = m_ambient[2] * scale;
}
