/* ==========================================================================
 *  eff/eff0.cpp
 *
 *  Eff0, the base of every Eff* effect class (0x16a914..0x16ac84), and the
 *  helpers the classes share (0x16ac84..0x16afcc): the per-kind draw-set
 *  cache, the 256-byte random permutation and its reader, the fog and
 *  ambient tints. EffBg1, the first class in link order, follows at
 *  0x16afcc. See eff/eff0.h.
 * ======================================================================== */
#include "eff/eff0.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "common/ror_assert.h"
#include "dog/doglightpool.h"
#include "eff/effman.h"
#include "i3d/i3denv.h"
#include "i3d/i3dlayer.h"
#include "mag/magscene.h"

/* The pack names table has one entry per kind (0x2f2cd0, 54 entries). */
#define EFF_NUM_KIND 54

static I3dDrawSet *s_effDrawSet[EFF_NUM_KIND];  /* 0x8282a0: renderPreMain's set per kind */
static u_char      s_effRandTable[0x100];       /* 0x828378 */
static int         s_effRandNo;                 /* 0x308050: effRand's cursor */

// ──────────────────────────────────────────────────────────────────────
// No pack, no kind, no countdown; the farthest depth.
// ROR 0x0016a914

Eff0::Eff0()
{
    m_life = -1;
    /* PC: the ROM stores the word 0x7fffffff, which the EE FPU reads as
     * its largest value (it has no NaN); on the host those bits are a NaN
     * every comparison in EffMan's sort fails, so the largest float. */
    m_z = FLT_MAX;
    m_pack = NULL;
    m_kind = 0;
    m_flags = 0;
    m_delay = 0;
    m_08 = 0;
    m_0c = 0;
}

// ROR 0x0016a950

Eff0::~Eff0()
{
    // callcheck: operator_delete (the deleting destructor's flag bit: what delete compiles to on the host)
    term();
}

// ──────────────────────────────────────────────────────────────────────
// The kind, the delay and the pack (one more user) from the param.
// ROR 0x0016a9ac

int Eff0::init(const EffParam *param)
{
    m_kind = param->m_kind;
    m_delay = param->m_08;
    EffPack *pack = param->m_pack;
    if (pack != NULL)
    {
        m_pack = pack;
        pack->m_ref++;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Drops the kind and the flags and releases its use of the pack (the
// pointer stays).
// ROR 0x0016a9e0

void Eff0::term()
{
    EffPack *pack = m_pack;
    m_kind = 0;
    m_flags = 0;
    if (pack != NULL)
        pack->m_ref--;
}

// ROR 0x0016aa04

void Eff0::setVisible(bool on)
{
    if (!on)
        m_flags |= EFF_FLAG_HIDDEN;
    else
        m_flags &= ~EFF_FLAG_HIDDEN;
    m_flags |= EFF_FLAG_ACTIVATE;
}

// ──────────────────────────────────────────────────────────────────────
// Waits out the delay (1 while it runs), applies a pending activation,
// counts the finish countdown down (finished at 0), then the class's step.
// ROR 0x0016aa2c

int Eff0::live()
{
    if (m_delay >= 0)
    {
        m_delay--;
        if (m_delay >= 0)
            return 1;
    }
    if (m_flags & EFF_FLAG_ACTIVATE)
        liveActivateExec();
    if (m_life >= 0)
    {
        m_life--;
        if (m_life < 1)
        {
            m_life = -1;
            m_flags |= EFF_FLAG_FINISHED;
        }
    }
    return liveMain();
}

// ──────────────────────────────────────────────────────────────────────
// Once the delay is over: into the depth-tested layer.
// ROR 0x0016aacc

void Eff0::render(RENDERPARAM *param)
{
    if (m_delay < 0)
    {
        param->m_cur = param->m_layer[0];
        renderPreMain(param);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The kind's draw set (made by createPacketEntry the first time in a
// frame), and the class's packet in a set node of key 0.
// ROR 0x0016ab10

void Eff0::renderPreMain(RENDERPARAM *param)
{
    if (s_effDrawSet[m_kind] == NULL)
        s_effDrawSet[m_kind] = createPacketEntry(param->m_cur);
    I3dDrawSet *set = s_effDrawSet[m_kind];
    i3dLayerSetDrawSet(param->m_cur, set);
    u_long *pkt = i3dLayerBeginSet(param->m_cur, 0.0f);
    pkt = renderMain(param, set, pkt);
    i3dLayerEndSet(param->m_cur, pkt);
}

// ──────────────────────────────────────────────────────────────────────
// A draw set of the pack's textures (the files after its models and
// motions); NULL when the pack has none.
// ROR 0x0016abe4

I3dDrawSet *Eff0::createPacketEntry(I3dLayer *layer)
{
    int tex[8];
    I3dTexSetDesc desc;
    desc.m_tex_no = tex;
    EffPack *pack = m_pack;
    /* PC: guard -- the ROM reads a pack-less effect's count through NULL. */
    if (pack == NULL || pack->m_num_tex == 0)
        return NULL;
    const EffPackFile *file = &pack->m_file[pack->m_num_i3r + pack->m_num_i3m];
    for (int i = 0; i < pack->m_num_tex; i++)
        tex[i] = file[i].m_tex;
    desc.m_depth = 1.0f;
    desc.m_tex_count = m_pack->m_num_tex;
    desc.m_sort_func = 0;
    desc.m_sort_order = 0;
    desc.m_sort_param = 0;
    return i3dLayerFindDrawSet(layer, &desc);
}

// ROR 0x0028adf8 (linkonce)

void Eff0::liveActivateExec()
{
    m_flags &= ~EFF_FLAG_ACTIVATE;
}

// ROR 0x0028ae18 (linkonce)

void Eff0::updateZDepth()
{
}

// ──────────────────────────────────────────────────────────────────────
// A shuffle of 0..255: each step takes the byte at a random distance
// (at least a quarter of what is left) past the last pick out of the
// remaining ones.
// ROR 0x0016ac84

void effInitRandTable()
{
    u_char work[0x100];
    for (int i = 0xff; i >= 0; i--)
        work[i] = (u_char)i;

    int pick = rand() % 0x100;
    for (int n = 0x100; n > 0; n--)
    {
        int quarter = n / 4;
        pick = (pick + quarter + rand() % (n - quarter * 2)) % n;
        s_effRandTable[0x100 - n] = work[pick];
        memmove(&work[pick], &work[pick + 1], n - pick - 1);
    }
    /* The last step already stored it (one byte left: pick 0). */
    s_effRandTable[0xff] = work[0];
    s_effRandNo = 0;
}

// ROR 0x0016ad94

u_char effRand()
{
    s_effRandNo = (s_effRandNo + 1) % 0x100;
    return s_effRandTable[s_effRandNo];
}

// ──────────────────────────────────────────────────────────────────────
// Once a frame before the effects render (EffMan::draw).
// ROR 0x0016add8

void effClearDrawSets()
{
    memset(s_effDrawSet, 0, sizeof(s_effDrawSet));
}

// ROR 0x0016ae00

void effFogColor(float *rgb, float depth)
{
    const I3dFog *fog = &g_effScene->m_fog[0];
    float fnear = fog->m_param[0];
    if (depth <= fnear)
        return;
    float ffar = fog->m_param[2];
    if (ffar <= depth)
    {
        rgb[0] *= fog->m_color[0];
        rgb[1] *= fog->m_color[1];
        rgb[2] *= fog->m_color[2];
        return;
    }
    float f = powf((depth - fnear) / (ffar - fnear), 2.0f);
    float amount = fog->m_param[1] + (fog->m_param[3] - fog->m_param[1]) * f;
    rgb[0] *= (fog->m_color[0] - 1.0f) * amount + 1.0f;
    rgb[1] *= (fog->m_color[1] - 1.0f) * amount + 1.0f;
    rgb[2] *= (fog->m_color[2] - 1.0f) * amount + 1.0f;
}

// ROR 0x0016af20

void effAmbientColor(float *rgb)
{
    // callcheck: FUN_00215a30 (the assert handler, inside the inline DogLightPool::getAmbientLight)
    const DogAmbientLight *light = g_effScene->m_light_pool.getAmbientLight(0);
    /* PC: guard -- after the failed assert the ROM reads through NULL. */
    if (light == NULL)
        return;
    rgb[0] *= light->m_color[0];
    rgb[1] *= light->m_color[1];
    rgb[2] *= light->m_color[2];
}
