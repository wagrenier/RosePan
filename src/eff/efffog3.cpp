/* ==========================================================================
 *  eff/efffog3.cpp
 *
 *  EffFog3, EffFog3Dark, EffFog3WeakUp (0x172678..0x172e4c): the fog that
 *  thickens while enemies are about. See eff/efffog3.h.
 * ======================================================================== */
#include "eff/efffog3.h"

#include <string.h>

#include "bed/bedpfh.h"
#include "common/ror_assert.h"
#include "eff/effman.h"
#include "emy/emy_manager.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"

/* ---- data ------------------------------------------------------------- */

const EffFog3Rec *g_effFog3Rec;     /* 0x298e18 */
int               g_effFog3Flags;   /* 0x298e14 */

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00172678

void gamCreateEff1a(void)
{
    EffParam param;
    memset(&param, 0, sizeof(param));   /* PC: zeroed (the ROM sets these two) */
    param.m_kind = 0x1a;
    param.m_08 = 0;
    g_magSystem.m_scene.m_eff_man.createEff(&param);
}

// ──────────────────────────────────────────────────────────────────────
// Enemy types 0..3 are down in state 4, types 4 and 5 in state 6; 6..10
// are never counted, a type past 10 always is.
// ROR 0x001726b4

int effFog3AnyEmyAwake(void)
{
    for (SeqNode<ACHRstat *> *node = g_gamMan.m_achrList.m_used.begin();
         node != g_gamMan.m_achrList.m_used.end(); node = node->m_next)
    {
        u_int oid = node->m_data->m_oid;
        if (gamOidType(oid) != 2)
            continue;
        switch (g_emyManager.getEmyType(oid))
        {
        case 0:
        case 1:
        case 2:
        case 3:
            if (g_emyManager.getEmyWord3c(oid) != 4)
                return 1;
            break;
        case 4:
            if (g_emyManager.getEmyWord3c(oid) != 6)
                return 1;
            break;
        case 5:
            if (g_emyManager.getEmyWord3c(oid) != 6)
                return 1;
            break;
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            break;
        default:
            return 1;
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001727ec

int effFog3AllEmyDown(void)
{
    for (SeqNode<ACHRstat *> *node = g_gamMan.m_achrList.m_used.begin();
         node != g_gamMan.m_achrList.m_used.end(); node = node->m_next)
    {
        u_int oid = node->m_data->m_oid;
        if (gamOidType(oid) != 2)
            continue;
        int state, down;
        switch (g_emyManager.getEmyType(oid))
        {
        case 0:
            state = g_emyManager.getEmyWord3c(oid);
            down = 4;
            break;
        case 1:
            state = g_emyManager.getEmyWord3c(oid);
            down = 4;
            break;
        case 2:
            state = g_emyManager.getEmyWord3c(oid);
            down = 4;
            break;
        case 3:
            state = g_emyManager.getEmyWord3c(oid);
            down = 4;
            break;
        case 4:
        case 5:
            state = g_emyManager.getEmyWord3c(oid);
            down = 6;
            break;
        default:
            continue;
        }
        if (state != down)
            return 0;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The record's fog, built on the map's (MagScene::m_fog[1]): its colour,
// the map's distances scaled, the near amount moved toward the far one,
// the far amount scaled. addFog moves the scene's target by the
// difference from the map's fog (on) or by its negation (off).
// ROR 0x00172934

void effFog3Shift(int on, int frames)
{
    const I3dFog &map = g_magSystem.m_scene.m_fog[1];
    const EffFog3Rec *rec = g_effFog3Rec;

    I3dFog fog;
    fog.m_param[0] = map.m_param[0] * rec->m_dist_scale;
    fog.m_param[1] = map.m_param[1] + (map.m_param[3] - map.m_param[1]) * rec->m_amount0_t;
    fog.m_param[2] = map.m_param[2] * rec->m_dist_scale;
    fog.m_param[3] = map.m_param[3] * rec->m_amount1_scale;
    fog.m_color[0] = (float)rec->m_rgb[0] / 255.0f;
    fog.m_color[1] = (float)rec->m_rgb[1] / 255.0f;
    fog.m_color[2] = (float)rec->m_rgb[2] / 255.0f;
    fog.m_color[3] = 0.0f;

    I3dFog delta;
    for (int i = 0; i < 4; i++)
    {
        if (on == 0)
        {
            delta.m_param[i] = map.m_param[i] - fog.m_param[i];
            delta.m_color[i] = map.m_color[i] - fog.m_color[i];
        }
        else
        {
            delta.m_param[i] = fog.m_param[i] - map.m_param[i];
            delta.m_color[i] = fog.m_color[i] - map.m_color[i];
        }
    }
    g_magSystem.m_scene.addFog(&delta, frames);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00172ac8

EffFog3::~EffFog3()
{
    g_effFog3Rec = NULL;    // callcheck: ~Eff0, operator_delete (the base destructor and the deleting variant, compiler-made)
}

// ──────────────────────────────────────────────────────────────────────
// The record of the current chapter (g_bss_7a3210) from the pack's table,
// the file after its models, motions and textures (the last record when
// none matches); the fog clear, a check every 30 frames, and dark at
// once if an enemy is already awake.
// ROR 0x00172b30

int EffFog3::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;

    int chapter = g_bss_7a3210;
    BedPfh pfh(m_pack->m_data);     // callcheck: clear, BedPfh (the ROM clears the stack copy, then constructs it)
    int n = m_pack->m_num_i3r + m_pack->m_num_i3m + m_pack->m_num_tex;
    const EffFog3Rec *rec = (const EffFog3Rec *)pfh.getFile(n);   // callcheck: dbgAssertFailed (getFile's inline asserts, bedpfh.h lines 85 / 86)
    g_effFog3Rec = rec;
    while (rec->m_chapter != -1 && rec->m_chapter != chapter)
        g_effFog3Rec = ++rec;

    g_effFog3Flags = 0;
    m_interval = 30;
    m_frame = 0;
    if (effFog3AnyEmyAwake())
    {
        g_effFog3Flags |= EFF_FOG3_DARK;
        effFog3Shift(1, 0);
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Every m_interval frames: clear and an enemy awake -> EffFog3Dark; dark
// and every enemy down -> EffFog3WeakUp 150 frames on, and 180 frames to
// the next check.
// ROR 0x00172cb8

int EffFog3::liveMain()
{
    m_frame++;
    if (m_frame % m_interval != 0)
        return 1;

    int interval = 30;
    EffParam param;
    memset(&param, 0, sizeof(param));   /* PC: zeroed (the ROM sets kind and m_08) */
    if ((g_effFog3Flags & EFF_FOG3_DARK) == 0)
    {
        if (effFog3AnyEmyAwake())
        {
            param.m_kind = 0x1b;
            param.m_08 = 0;
            g_effScene->m_eff_man.createEff(&param);
        }
    }
    else if (effFog3AllEmyDown())
    {
        param.m_kind = 0x1c;
        param.m_08 = 150;
        g_effScene->m_eff_man.createEff(&param);
        interval = 180;
    }
    m_interval = interval;
    m_frame = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00172d98

int EffFog3Dark::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;
    if ((g_effFog3Flags & EFF_FOG3_DARK) == 0)
    {
        g_effFog3Flags |= EFF_FOG3_DARK;
        effFog3Shift(1, 30);
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00172de8

int EffFog3WeakUp::init(const EffParam *param)
{
    return Eff0::init(param) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00172e08

int EffFog3WeakUp::liveMain()
{
    if (g_effFog3Flags & EFF_FOG3_DARK)
    {
        g_effFog3Flags &= ~EFF_FOG3_DARK;
        effFog3Shift(0, 90);
    }
    return 0;
}
