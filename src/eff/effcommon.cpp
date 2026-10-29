/* ==========================================================================
 *  eff/effcommon.cpp
 *
 *  The helpers the Eff* classes share (0x16fb70..0x1704a4). See
 *  eff/effcommon.h.
 * ======================================================================== */
#include "eff/effcommon.h"

#include <math.h>
#include <string.h>

#include "bed/bedmemory.h"
#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "dog/dogcamera.h"
#include "i3d/i3dmath.h"
#include "i3d/i3mroot.h"
#include "i3d/i3rroot.h"
#include "i3d/i3rtype.h"
#include "utl/utlmisc.h"

/* The float bits 0x00800001 (just above FLT_MIN): effMatMakeUpright's
 * "nothing left" length. */
static const float EFF_TINY = 0x1.000002p-126f;

/* The ROM's turn constants (0x40490fd9 / 0x40c90fd9, a little above the
 * float nearest pi / 2pi). */
static const float EFF_PI     = 0x1.921fb2p+1f;
static const float EFF_TWO_PI = 0x1.921fb2p+2f;

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016fb70

int effStrIndex(const char *str, const char *sub)
{
    int len = (int)strlen(str);
    int sublen = (int)strlen(sub);
    for (int i = 0; i < len; i++)
    {
        int n = 0;
        if (sublen > 0 && str[i] == sub[0])
        {
            n = 1;
            while (n < sublen && str[i + n] == sub[n])
                n++;
        }
        if (n == sublen)
            return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016fc20

float effVecLength(const float *v)
{
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016fc74

void effMatMakeUpright(sceVu0FVECTOR *m)
{
    float sx = effVecLength(m[0]);
    float sz = effVecLength(m[2]);
    m[2][1] = 0.0f;
    if (effVecLength(m[2]) <= EFF_TINY)
    {
        m[2][0] = 0.0f;
        m[2][2] = 1.0f;
    }
    else
    {
        i3dVecNormalize2(m[2], m[2]);
    }
    m[1][0] = 0.0f;
    m[1][1] = 1.0f;
    m[1][2] = 0.0f;
    i3dVecCross2(m[0], m[1], m[2]);
    i3dVecScaleXYZ(sx, m[0], m[0]);
    i3dVecScaleXYZ(sz, m[2], m[2]);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016fd4c

void effMatMulRotZ(float angle, sceVu0FVECTOR *m)
{
    sceVu0FMATRIX rot;
    i3dMatUnit(rot);
    i3dMatRotZ(angle, rot, rot);
    i3dMatMul(m, m, rot);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016fda0

void effMatFromAxisZ(sceVu0FVECTOR *m, const float *dir)
{
    float up[4];
    memset(up, 0, sizeof(up));
    if (dir[1] > 0.999999f)
        up[2] = -1.0f;
    else if (dir[1] < -0.999999f)
        up[2] = 1.0f;
    else
        up[1] = 1.0f;
    i3dVecCross2(m[0], up, dir);
    i3dVecCross2(m[1], dir, m[0]);
    memcpy(m[2], dir, sizeof(sceVu0FVECTOR));
    m[3][3] = 1.0f;
    m[3][0] = 0.0f;
    m[3][2] = 0.0f;
    m[3][1] = 0.0f;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016fe88

float effWrapAngle(float a)
{
    while (a < -EFF_PI)
        a += EFF_TWO_PI;
    while (EFF_PI < a)
        a -= EFF_TWO_PI;
    return a;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016ff18

float effCalcViewDepth(DogCamera *cam, const float *pos)
{
    float d[4];
    cam->callTransform(); /* EE slot 2 */
    i3dVecSub(d, pos, cam->m_matrix[3]);
    return i3dVecDot(cam->m_matrix[2], d);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016ff78

void effMoveToward(float d, float *out, const float *from, const float *to)
{
    float v[4];
    i3dVecSub(v, to, from);
    i3dVecNormalize2(v, v);
    i3dVecScale(d, v, v);
    i3dVecAdd(out, v, from);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016ffec

u_short *effMakeNodeMap(I3rRoot *root, const I3mRoot *motion)
{
    int num = i3mGetNumNode(motion);
    u_short *map = (u_short *)g_utlMemory.alloc(num * 2 + 2, -1);
    map[0] = (u_short)num;
    for (int i = 0; i < num; i++)
    {
        int n = i3rGetNodeIndexByName(root, i3mGetNodeName(motion, (u_short)i));
        map[i + 1] = (n < 0) ? 0xffff : (u_short)n;
    }
    return map;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001700b0

I3rItem *effFindChild52(const I3rItem *item, const char *name, int *index)
{
    int num = i3rGetNumChild52(item);
    for (int i = 0; i < num; i++)
    {
        if (strstr(i3rGetChildName52(item, i), name) != NULL)
        {
            if (index != NULL)
                *index = i;
            return i3rGetChild52(item, i);
        }
    }
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed and i3rGetNodeIndexByName are DogModel::getNodeIndexByName's (inline in dog/dogmodel.h); getNodeWorldPos is DogActor's EE slot 18
// ROR 0x00170158

void effProbeNode(EffActorProbe *probe, int n, const char *name)
{
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    DogActor *actor = probe->m_actor;
    /* DogModel::getNodeIndexByName, inlined (dog/dogmodel.h 305). */
    int node = actor->getNodeIndexByName(name);
    actor->getNodeWorldPos(pos, node); /* EE slot 18 */
    if (pos[0] < probe->m_center[0] - probe->m_half[0] || probe->m_center[0] + probe->m_half[0] < pos[0])
        return;
    if (pos[2] < probe->m_center[2] - probe->m_half[2] || probe->m_center[2] + probe->m_half[2] < pos[2])
        return;
    if (pos[1] < probe->m_center[1] - probe->m_half[1] || probe->m_center[1] + probe->m_half[1] < pos[1])
        return;
    probe->m_hit |= 1u << (n & 0x1f);
    probe->m_pos[n][0] = pos[0];
    probe->m_pos[n][1] = pos[1];
    probe->m_pos[n][2] = pos[2];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001702e4

int effProbeActorInBox(const EffActorProbe *probe)
{
    const float *p = probe->m_actor->m_trans;
    float hxz = probe->m_half[0] + probe->m_margin;
    if (p[0] < probe->m_center[0] - hxz || probe->m_center[0] + hxz < p[0])
        return 0;
    if (p[2] < probe->m_center[2] - hxz || probe->m_center[2] + hxz < p[2])
        return 0;
    if (p[1] < probe->m_center[1] - probe->m_half[1] || probe->m_center[1] + probe->m_half[1] < p[1])
        return 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001703c0

void effProbeActorLimbs(EffActorProbe *probe)
{
    DogActor *actor = probe->m_actor;
    float t = actor->getMotionFrame();
    if (t < 0.0f)
        return;
    u_int ev = actor->getMotionEvent(t, t + 1.0f, -1);
    if (ev & 0x10000000)
        effProbeNode(probe, 0, "Bip01 L Toe0");
    if (ev & 0x20000000)
        effProbeNode(probe, 1, "Bip01 R Toe0");
    if (ev & 0x4000000)
        effProbeNode(probe, 2, "Bip01 L Finger0");
    if (ev & 0x8000000)
        effProbeNode(probe, 3, "Bip01 R Finger0");
}
