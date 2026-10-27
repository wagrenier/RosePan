/* ==========================================================================
 *  mag/magscenelight.cpp
 *
 *  The scene's lights (RoR's "mag/magscenelight.cpp", 0x13f49c..0x13ff00):
 *  MagScene's front end to its DogLightPool (+0x2050) -- a map's _l.bin
 *  into the pool, the default ambient light, light sets built, copied and
 *  their parameters read and written for the scripts, and the light the
 *  projected shadows come from (m_shadow_light, +0x2a9d0).
 * ======================================================================== */
#include "mag/magscene.h"

#include "i3d/i3dmath.h"

/* The shadow kinds setShadowLight takes (the assert's name; the pool's
 * kinds, setLightFlags passes 5 too). */
#define MAG_MAX_LIGHT_TYPE DOG_MAX_LIGHT_TYPE

/* data 0x2eea80: the shadow light's ambient colour, all zero. The ROM
 * asserts it is 16-byte aligned ("(((int)(&ShadowAmbient)) & 0x0f) == 0",
 * line 359, compiled out as unreachable). */
alignas(16) static const float ShadowAmbient[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT)
// ROR 0x0013f49c

void MagScene::checkLightNo(int theNo)
{
    ROR_ASSERT(theNo < (8)); /* 32 */
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT)
// ROR 0x0013f4d0

void MagScene::setLightNo(int theNo)
{
    ROR_ASSERT(theNo < (8)); /* 59 */
    m_light_pool.resetSet(theNo);
}

// ──────────────────────────────────────────────────────────────────────
// theNum is indexed by light kind (DOG_LIGHT_*).
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT)
// ROR 0x0013f528

void MagScene::setLightFlags(const int *theNum, int theNo)
{
    ROR_ASSERT(theNo < (8)); /* 90 */
    ROR_ASSERT(theNum); /* 92 */
    m_light_pool.buildSet(theNo, theNum);
    if (theNo == 0)
    {
        if (theNum[DOG_LIGHT_POINT] > 0)
            setShadowLight(DOG_LIGHT_POINT, 0);
        else if (theNum[DOG_LIGHT_SPOT] > 0)
            setShadowLight(DOG_LIGHT_SPOT, 0);
        else if (theNum[5] > 0)
            setShadowLight(5, 0);
        else if (theNum[DOG_LIGHT_DIRECT] > 0)
            setShadowLight(DOG_LIGHT_DIRECT, 0);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The pool starts over (term + create). Light 0 of the ambient kind goes
// into sets 0 and 1, the directional and point lights into set 1 only.
// The light m_shadow_no counts down to (the directional light first)
// becomes the shadow light.
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT)
// ROR 0x0013f658

void MagScene::loadLight(const void *theData)
{
    ROR_ASSERT(theData); /* 134 */
    const MagLightFile *data = (const MagLightFile *)theData;
    DogLightPool *pool = &m_light_pool;
    pool->term();
    pool->create();

    int direct_num = data->m_direct_num;
    int point_num = data->m_point_num;
    int shadow_no = data->m_shadow_no;

    int id = pool->allocId(DOG_LIGHT_AMBIENT);
    pool->addToSet(0, DOG_LIGHT_AMBIENT, id);
    pool->addToSet(1, DOG_LIGHT_AMBIENT, id);
    float v[4];
    v[0] = data->m_ambient[0];
    v[1] = data->m_ambient[1];
    v[2] = data->m_ambient[2];
    v[3] = 1.0f;
    dbgTraceVector4(v);
    setLightParam(DOG_LIGHT_AMBIENT, id, DOG_LIGHT_PARAM_COLOR, v, -1);

    const MagLightFileLight *light = data->m_light;
    if (direct_num > 0)
    {
        id = pool->allocId(DOG_LIGHT_DIRECT);
        pool->addToSet(1, DOG_LIGHT_DIRECT, id);
        v[0] = light->m_color[0];
        v[1] = light->m_color[1];
        v[2] = light->m_color[2];
        v[3] = 0.0f;
        dbgTraceVector4(v);
        setLightParam(DOG_LIGHT_DIRECT, id, DOG_LIGHT_PARAM_COLOR, v, -1);
        setLightParam(DOG_LIGHT_DIRECT, id, DOG_LIGHT_PARAM_DIR, light->m_vec, -1);
        if (shadow_no == 0)
            setShadowLight(DOG_LIGHT_DIRECT, id);
        light++;
        shadow_no--;
    }

    for (; point_num > 0; point_num--)
    {
        id = pool->allocId(DOG_LIGHT_POINT);
        pool->addToSet(1, DOG_LIGHT_POINT, id);
        float intensity = light->m_vec[3];
        v[0] = light->m_color[0] * intensity;
        v[1] = light->m_color[1] * intensity;
        v[3] = 0.0f;
        v[2] = light->m_color[2] * intensity;
        dbgTraceVector4(v);
        setLightParam(DOG_LIGHT_POINT, id, DOG_LIGHT_PARAM_COLOR, v, -1);
        setLightParam(DOG_LIGHT_POINT, id, DOG_LIGHT_PARAM_POS, light->m_vec, -1);
        light++;
        if (shadow_no == 0)
            setShadowLight(DOG_LIGHT_POINT, id);
        shadow_no--;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013f8e4

void MagScene::setDefaultLight()
{
    float color[4] = { 0.75f, 0.75f, 0.75f, 1.0f };
    int id = m_light_pool.allocId(DOG_LIGHT_AMBIENT);
    m_light_pool.addToSet(6, DOG_LIGHT_AMBIENT, id);
    setLightParam(DOG_LIGHT_AMBIENT, id, DOG_LIGHT_PARAM_COLOR, color, -1);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT)
// ROR 0x0013f980

void MagScene::setLightParam(int type, int idx, int which, const float *v, int theNo)
{
    ROR_ASSERT(theNo < (8)); /* 282 */
    if (theNo < 0)
        m_light_pool.setLightParam(type, idx, which, v);
    else
        m_light_pool.setSetLightParam(type, idx, which, v, theNo);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT)
// ROR 0x0013fa50

void MagScene::getLightParam(int type, int idx, int which, float *theA, int theNo)
{
    ROR_ASSERT(theNo < (8)); /* 304 */
    ROR_ASSERT(theA); /* 305 */
    if (theNo < 0)
        m_light_pool.getLightParam(type, idx, which, theA);
    else
        m_light_pool.getSetLightParam(type, idx, which, theA, theNo);
}

// ──────────────────────────────────────────────────────────────────────
// The ROM compares unsigned (a negative set fails "< (8)" too).
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT)
// ROR 0x0013fb3c

void MagScene::copyLightSet(int theD, int theS)
{
    ROR_ASSERT(theD >=0 && theD < (8)); /* 326 */
    ROR_ASSERT(theS >=0 && theS < (8)); /* 327 */
    ROR_ASSERT(theD != theS); /* 328 */
    m_light_pool.copySet(theD, theS);
}

// ──────────────────────────────────────────────────────────────────────
// m_shadow_light becomes an empty description with ShadowAmbient and, for
// a directional light, m_shadow_lightrec as its one parallel light; for a
// point or spot light, m_shadow_lightrec as its one spot light
// (makeActorShadow fills the record from m_shadow_src each shadow). The
// pool lookups are doglightpool.h's const accessors (lines 141..143).
// callcheck: FUN_00215a30 (the assert handler inside ROR_ASSERT); getDirectLight / getPointLight / getSpotLight are the ROM's inlined pool accessors
// ROR 0x0013fbe4

void MagScene::setShadowLight(int theLight, int theNo)
{
    ROR_ASSERT(theLight >= 0 && theLight < MAG_MAX_LIGHT_TYPE); /* 353 */
    ROR_ASSERT(theNo >= 0); /* 354 */
    const DogLightPool &pool = m_light_pool;
    i3dLightDescInit(&m_shadow_light);
    i3dVecCopy2(m_shadow_light.m_ambient, ShadowAmbient);
    m_shadow_mode = theLight;
    switch (theLight)
    {
    case DOG_LIGHT_DIRECT:
        m_shadow_light.m_parallel = (I3dParallelLight *)&m_shadow_lightrec;
        m_shadow_src = pool.getDirectLight(theNo)->m_dir;
        m_shadow_light.m_parallel_count = 1;
        break;
    case DOG_LIGHT_POINT:
        m_shadow_light.m_spot = (I3dSpotLight *)&m_shadow_lightrec;
        m_shadow_src = pool.getPointLight(theNo)->m_pos;
        m_shadow_light.m_spot_count = 1;
        break;
    case DOG_LIGHT_SPOT:
        m_shadow_light.m_spot = (I3dSpotLight *)&m_shadow_lightrec;
        m_shadow_src = pool.getSpotLight(theNo)->m_pos;
        m_shadow_light.m_spot_count = 1;
        break;
    default:
        m_shadow_src = NULL;
        break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The mask is walked over nine sets (0..8), one more than the pool has;
// addToSet asserts set 8 if a caller ever sets bit 8.
// ROR 0x0013fe64

int MagScene::addLight(int type, unsigned int mask)
{
    int id = m_light_pool.allocId(type);
    if (id >= 0)
    {
        unsigned int bit = 1;
        for (int set = 0; set < 9; set++, bit <<= 1)
        {
            if (mask & bit)
                m_light_pool.addToSet(set, type, id);
        }
    }
    return id;
}
