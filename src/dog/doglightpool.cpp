// dog/doglightpool.cpp -- the scene's light pool (0x1103dc..0x112b08).
//
// A DogLightPool keeps the stage's lights per kind (ambient, direct,
// point, spot ...) and eight light sets, each a bit mask per kind of the
// lights it uses. Each frame MagScene::draw has createLight turn set i
// into the i3d light of render stage i; makeActorShadow asks
// findNearestPointLight for the light an actor's shadow falls from.
//
// MagScene::loadLight fills the pool: create gives one block of 8 ambient,
// 8 direct, 16 point and 8 spot lights, allocId / addToSet hand them to
// the sets, setLightParam writes their fields. A light's id within its
// kind is its bit in the masks; m_used keeps the bits any set holds.

#include "dog/doglightpool.h"

#include <float.h>
#include <string.h>

#include "common/ror_assert.h"
#include "i3d/i3denv.h"
#include "i3d/i3dmath.h"
#include "utl/utlmisc.h"

/* Per kind, the capacity of a DogLightBuffer (rodata 0x2e85d8) and the
 * size of one light (rodata 0x2e87e0). */
static const int s_light_max[DOG_MAX_LIGHT_TYPE]  = { 8, 8, 16, 8, 0, 0 };
static const int s_light_size[DOG_MAX_LIGHT_TYPE] = { 0x10, 0x20, 0x20, 0x40, 0x30, 0x30 };

/* The cone angles' degree <-> radian factor: the ROM's pi is 0x40490fdc,
 * one ulp above the float nearest pi. */
static const float s_pi = 0x1.921fb8p+1f;

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x001103dc

void DogLightBuffer::copyLights(int theType, const void *theLight, u_int mask)
{
    ROR_ASSERT(theType >= 0 && theType < DOG_MAX_LIGHT_TYPE); /* 76 */
    ROR_ASSERT(theLight); /* 77 */
    /* PC: the ROM indexes its tables with any type after the assert. */
    if ((u_int)theType >= DOG_MAX_LIGHT_TYPE)
        return;

    int n = 0;
    if (mask != 0)
    {
        u_char *dst;
        switch (theType)
        {
        case DOG_LIGHT_AMBIENT: dst = (u_char *)m_ambient; break;
        case DOG_LIGHT_DIRECT:  dst = (u_char *)m_direct;  break;
        case DOG_LIGHT_POINT:   dst = (u_char *)m_point;   break;
        case DOG_LIGHT_SPOT:    dst = (u_char *)m_spot;    break;
        default:
            ROR_ASSERT(0); /* 110 */
            dst = NULL;
            break;
        }

        int max  = s_light_max[theType];
        int size = s_light_size[theType];
        const u_char *src = (const u_char *)theLight;
        for (u_int bit = 1; bit != 0; bit <<= 1, src += size)
        {
            if (!(mask & bit))
                continue;
            n++;
            /* PC: kinds 4 and 5 have no array here (the ROM copies to
             * address 0 after the assert). */
            if (dst != NULL)
                memcpy(dst, src, size);
            dst += size;
            if (n >= max)
                break;
        }
    }
    m_num[theType] = n;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00110578

void DogLightBuffer::setupDesc()
{
    i3dLightDescInit(&m_desc);
    if (m_num[DOG_LIGHT_AMBIENT] > 0)
        i3dVecCopy2(m_desc.m_ambient, m_ambient[0].m_color);

    /* PORT: the ROM reads the direct count from the word at +0x5e0 and the
     * point count from +0x600, both past m_num (+0x5c0..0x5d8), and gives
     * the spot list the +0x5e0 count too. Nothing calls this (see
     * copyToBuffer); the port uses the counts copyLights stored. */
    if (m_num[DOG_LIGHT_DIRECT] > 0)
    {
        m_desc.m_parallel_count = m_num[DOG_LIGHT_DIRECT];
        m_desc.m_parallel = m_direct;
    }
    if (m_num[DOG_LIGHT_POINT] > 0)
    {
        m_desc.m_point_count = m_num[DOG_LIGHT_POINT];
        m_desc.m_point = m_point;
    }
    if (m_num[DOG_LIGHT_SPOT] > 0)
    {
        m_desc.m_spot_count = m_num[DOG_LIGHT_SPOT];
        m_desc.m_spot = m_spot;
    }
    m_desc.m_vec3 = NULL;
    m_desc.m_dir2_count = 0;
    m_desc.m_dir2 = NULL;
    m_desc.m_vec3_count = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00110614

void DogLightBuffer::clear()
{
    i3dLightDescInit(&m_desc);
    for (int i = DOG_MAX_LIGHT_TYPE - 1; i >= 0; i--)
        m_num[i] = 0;
}

// ──────────────────────────────────────────────────────────────────────
// The four set getters: one copy each in the ROM (lines 270, 294, 318,
// 342), the same text with the kind changed. The index of the theNo-th
// light of kind type in set theSetNo, -1 when the set has fewer.

static int dogLightFindInSet(const DogLightPool *pool, int type, int theSetNo, int theNo)
{
    for (int i = 0; i < pool->m_num[type]; i++)
    {
        if ((pool->m_set_mask[theSetNo][type] & (1u << (i & 31))) && --theNo < 0)
            return i;
    }
    return -1;
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00110660

DogAmbientLight *DogLightPool::getAmbientLight(int theSetNo, int theNo) const
{
    const int type = DOG_LIGHT_AMBIENT;
    ROR_ASSERT(m_light[type]); /* 270 */
    ROR_ASSERT(theSetNo >= 0 && theSetNo < 8); /* 271 */
    ROR_ASSERT(theNo >= 0 && theNo < m_num[type]); /* 272 */
    int i = dogLightFindInSet(this, type, theSetNo, theNo);
    return i < 0 ? NULL : (DogAmbientLight *)m_light[type] + i;
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011076c

DogDirectLight *DogLightPool::getDirectLight(int theSetNo, int theNo) const
{
    const int type = DOG_LIGHT_DIRECT;
    ROR_ASSERT(m_light[type]); /* 294 */
    ROR_ASSERT(theSetNo >= 0 && theSetNo < 8); /* 295 */
    ROR_ASSERT(theNo >= 0 && theNo < m_num[type]); /* 296 */
    int i = dogLightFindInSet(this, type, theSetNo, theNo);
    return i < 0 ? NULL : (DogDirectLight *)m_light[type] + i;
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00110878

DogPointLight *DogLightPool::getPointLight(int theSetNo, int theNo) const
{
    const int type = DOG_LIGHT_POINT;
    ROR_ASSERT(m_light[type]); /* 318 */
    ROR_ASSERT(theSetNo >= 0 && theSetNo < 8); /* 319 */
    ROR_ASSERT(theNo >= 0 && theNo < m_num[type]); /* 320 */
    int i = dogLightFindInSet(this, type, theSetNo, theNo);
    return i < 0 ? NULL : (DogPointLight *)m_light[type] + i;
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00110988

DogSpotLight *DogLightPool::getSpotLight(int theSetNo, int theNo) const
{
    const int type = DOG_LIGHT_SPOT;
    ROR_ASSERT(m_light[type]); /* 342 */
    ROR_ASSERT(theSetNo >= 0 && theSetNo < 8); /* 343 */
    ROR_ASSERT(theNo >= 0 && theNo < m_num[type]); /* 344 */
    int i = dogLightFindInSet(this, type, theSetNo, theNo);
    return i < 0 ? NULL : (DogSpotLight *)m_light[type] + i;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00110a94

void DogLightPool::init()
{
    for (int i = 0; i < DOG_MAX_LIGHT_TYPE; i++)
    {
        m_light[i] = NULL;
        m_num[i] = 0;
        m_used[i] = 0;
        for (int s = 0; s < DOG_LIGHT_SET_MAX; s++)
            m_set_mask[s][i] = 0;
    }
    m_data = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00110af8

void DogLightPool::freeLights()
{
    ROR_ASSERT(0); /* 388 */
    for (int i = 0; i < DOG_MAX_LIGHT_TYPE; i++)
    {
        if (m_light[i] != NULL)
            g_utlMemory.free(m_light[i]);
    }
    init();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00110b78

void DogLightPool::term()
{
    if (m_data != NULL)
        g_utlMemory.free(m_data);
    init();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00110bb8

void dogLightToGray(float *color)
{
    float y = color[0] * 0.299f + color[1] * 0.58699995f + color[2] * 0.11399999f;
    color[0] = y;
    color[2] = y;
    color[1] = y;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00110c0c

int DogLightPool::newLight(int theType)
{
    ROR_ASSERT(theType >= 0 && theType < DOG_MAX_LIGHT_TYPE); /* 432 */
    int *num = &m_num[theType];
    if (*num >= DOG_LIGHT_ID_MAX)
        return -1;

    int size = s_light_size[theType];
    void *a = g_utlMemory.alloc((*num + 1) * size, -1);
    if (a == NULL)
    {
        ROR_ASSERT(a); /* 439 */
        return -1;
    }
    if (m_light[theType] != NULL)
    {
        memcpy(a, m_light[theType], *num * size);
        g_utlMemory.free(m_light[theType]);
        m_light[theType] = NULL;
    }
    memset((u_char *)a + *num * size, 0, size);
    m_light[theType] = a;
    int id = theType * DOG_LIGHT_ID_MAX + *num;
    (*num)++;
    return id;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00110d60

int DogLightPool::getDataSize()
{
    return 8 * sizeof(DogAmbientLight) + 8 * sizeof(DogDirectLight)
         + 16 * sizeof(DogPointLight) + 8 * sizeof(DogSpotLight);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00110d68

int DogLightPool::create()
{
    ROR_ASSERT(m_data == NULL); /* 511 */
    init();
    int size = getDataSize();
    ROR_ASSERT(m_data == NULL); /* 518 */
    m_data = g_utlMemory.alloc(size, -1);
    ROR_ASSERT(m_data); /* 524 */

    u_char *data = (u_char *)m_data;
    m_num[5] = 0;
    m_light[DOG_LIGHT_AMBIENT] = data;
    m_light[DOG_LIGHT_DIRECT] = data + 8 * sizeof(DogAmbientLight);
    m_light[DOG_LIGHT_POINT] = data + 8 * sizeof(DogAmbientLight) + 8 * sizeof(DogDirectLight);
    m_num[4] = 0;
    m_light[DOG_LIGHT_SPOT] = data + 8 * sizeof(DogAmbientLight) + 8 * sizeof(DogDirectLight)
                            + 16 * sizeof(DogPointLight);
    m_num[DOG_LIGHT_POINT] = 16;
    m_num[DOG_LIGHT_SPOT] = 8;
    m_num[DOG_LIGHT_AMBIENT] = 8;
    m_num[DOG_LIGHT_DIRECT] = 8;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00110e64

void DogLightPool::alloc(const int *num)
{
    freeLights();
    for (int i = 0; i < DOG_MAX_LIGHT_TYPE; i++)
    {
        if (m_light[i] != NULL)
        {
            g_utlMemory.free(m_light[i]);
            m_light[i] = NULL;
        }
        m_num[i] = 0;
        if (num[i] != 0)
        {
            int size = s_light_size[i] * num[i];
            void *p = g_utlMemory.alloc(size, -1);
            ROR_ASSERT(p); /* 604 */
            /* PC: the ROM clears address 0 after the assert. */
            if (p != NULL)
                memset(p, 0, size);
            m_light[i] = p;
            m_num[i] = num[i];
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00110f78

void DogLightPool::resetSet(int theSet)
{
    if (theSet < 0)
    {
        init();
        create();
        return;
    }
    ROR_ASSERT(theSet >= 0 && theSet < 8); /* 632 */
    /* PC: the ROM writes past m_set_mask after the assert. */
    if (theSet >= DOG_LIGHT_SET_MAX)
        return;

    for (int i = 0; i < DOG_MAX_LIGHT_TYPE; i++)
        m_set_mask[theSet][i] = 0;
    for (int i = 0; i < DOG_MAX_LIGHT_TYPE; i++)
    {
        u_int used = 0;
        for (int s = 0; s < DOG_LIGHT_SET_MAX; s++)
            used |= m_set_mask[s][i];
        m_used[i] = used;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00111064

void DogLightPool::buildSet(int theSet, const int *num)
{
    /* PORT: theSet < 0 (resetSet's start-over) has the ROM OR the ids into
     * m_set_mask[-1], which is m_used: the same words here, so the
     * index stays as the ROM computes it. */
    resetSet(theSet);
    for (int i = 0; i < DOG_MAX_LIGHT_TYPE; i++)
    {
        for (int j = 0; j < num[i]; j++)
        {
            int id = allocId(i);
            (&m_set_mask[0][0])[theSet * DOG_MAX_LIGHT_TYPE + i] |= 1u << (id & 31);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00111120

int DogLightPool::allocId(int theType)
{
    ROR_ASSERT(theType >= 0 && theType < DOG_MAX_LIGHT_TYPE); /* 810 */
    for (int i = 0; i < m_num[theType]; i++)
    {
        u_int bit = 1u << (i & 31);
        if (!(m_used[theType] & bit))
        {
            m_used[theType] |= bit;
            return i;
        }
    }
    ROR_ASSERT(0); /* 825 */
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x001111e4

void DogLightPool::removeLight(int theType, int theNo)
{
    ROR_ASSERT(theNo >= 0); /* 834 */
    ROR_ASSERT(theType >= 0 && theType < DOG_MAX_LIGHT_TYPE); /* 836 */
    u_int mask = ~(1u << (theNo & 31));
    m_used[theType] &= mask;
    for (int s = 0; s < DOG_LIGHT_SET_MAX; s++)
        m_set_mask[s][theType] &= mask;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x001112a8

void DogLightPool::addToSet(int theSet, int theType, int theID)
{
    ROR_ASSERT(theSet >= 0 && theSet < 8); /* 851 */
    ROR_ASSERT(theID >= 0); /* 852 */
    ROR_ASSERT(theType >= 0 && theType < DOG_MAX_LIGHT_TYPE); /* 853 */
    u_int bit = 1u << (theID & 31);
    m_used[theType] |= bit;
    m_set_mask[theSet][theType] |= bit;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011139c

void DogLightPool::addToAllSets(int theType, int theID)
{
    ROR_ASSERT(theID >= 0); /* 867 */
    ROR_ASSERT(theType >= 0 && theType < DOG_MAX_LIGHT_TYPE); /* 868 */
    u_int bit = 1u << (theID & 31);
    m_used[theType] |= bit;
    for (int s = 0; s < DOG_LIGHT_SET_MAX; s++)
        m_set_mask[s][theType] |= bit;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00111460

void DogLightPool::copyToBuffer(DogLightBuffer *buf, const u_int *mask)
{
    for (int i = 0; i < DOG_MAX_LIGHT_TYPE; i++)
    {
        if (m_light[i] != NULL && mask[i] != 0)
            buf->copyLights(i, m_light[i], mask[i]);
    }
    buf->setupDesc();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00111528

void DogLightPool::copySet(int theDst, int theSrc)
{
    for (int i = 0; i < DOG_MAX_LIGHT_TYPE; i++)
        m_set_mask[theDst][i] = m_set_mask[theSrc][i];
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler, inside the get*Light inlines)
// ROR 0x00111584

void DogLightPool::setLightParam(int type, int n, int param, const float *v)
{
    float *dst = NULL;

    if (param == DOG_LIGHT_PARAM_CONE)
    {
        if (type == DOG_LIGHT_SPOT)
        {
            getSpotLight(n)->m_cone[0] = v[0] * s_pi / 180.0f;
            getSpotLight(n)->m_cone[1] = v[1] * s_pi / 180.0f;
            return;
        }
        ROR_ASSERT(0); /* 962 */
        return;
    }

    switch (param)
    {
    case DOG_LIGHT_PARAM_POS:
        if (type == DOG_LIGHT_POINT)
            dst = getPointLight(n)->m_pos;
        else if (type == DOG_LIGHT_SPOT)
            dst = getSpotLight(n)->m_pos;
        else
            ROR_ASSERT(0); /* 984 */
        break;
    case DOG_LIGHT_PARAM_DIR:
        if (type == DOG_LIGHT_DIRECT)
            dst = getDirectLight(n)->m_dir;
        else if (type == DOG_LIGHT_SPOT)
            dst = getSpotLight(n)->m_dir;
        else
            ROR_ASSERT(0); /* 1005 */
        break;
    case DOG_LIGHT_PARAM_COLOR:
        switch (type)
        {
        case DOG_LIGHT_AMBIENT: dst = getAmbientLight(n)->m_color; break;
        case DOG_LIGHT_DIRECT:  dst = getDirectLight(n)->m_color;  break;
        case DOG_LIGHT_POINT:   dst = getPointLight(n)->m_color;   break;
        case DOG_LIGHT_SPOT:    dst = getSpotLight(n)->m_color;    break;
        default:
            ROR_ASSERT(0); /* 1040 */
            break;
        }
        break;
    case 3:
        ROR_ASSERT(0); /* 1053 */
        break;
    case 4:
        ROR_ASSERT(0); /* 1066 */
        break;
    default:
        ROR_ASSERT(0); /* 1096 */
        break;
    }

    if (dst != NULL)
    {
        for (int i = 0; i < 4; i++)
            dst[i] = v[i];
    }
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler, inside the get*Light const inlines)
// ROR 0x00111b5c

void DogLightPool::getLightParam(int type, int n, int param, float *v) const
{
    const float *src = NULL;

    if (param == DOG_LIGHT_PARAM_CONE)
    {
        if (type == DOG_LIGHT_SPOT)
        {
            v[0] = getSpotLight(n)->m_cone[0] * 180.0f / s_pi;
            v[1] = getSpotLight(n)->m_cone[1] * 180.0f / s_pi;
            return;
        }
        ROR_ASSERT(0); /* 1126 */
        return;
    }

    switch (param)
    {
    case DOG_LIGHT_PARAM_POS:
        if (type == DOG_LIGHT_POINT)
            src = getPointLight(n)->m_pos;
        else if (type == DOG_LIGHT_SPOT)
            src = getSpotLight(n)->m_pos;
        else
            ROR_ASSERT(0); /* 1148 */
        break;
    case DOG_LIGHT_PARAM_DIR:
        if (type == DOG_LIGHT_DIRECT)
            src = getDirectLight(n)->m_dir;
        else if (type == DOG_LIGHT_SPOT)
            src = getSpotLight(n)->m_dir;
        else
            ROR_ASSERT(0); /* 1169 */
        break;
    case DOG_LIGHT_PARAM_COLOR:
        switch (type)
        {
        case DOG_LIGHT_AMBIENT: src = getAmbientLight(n)->m_color; break;
        case DOG_LIGHT_DIRECT:  src = getDirectLight(n)->m_color;  break;
        case DOG_LIGHT_POINT:   src = getPointLight(n)->m_color;   break;
        case DOG_LIGHT_SPOT:    src = getSpotLight(n)->m_color;    break;
        default:
            ROR_ASSERT(0); /* 1204 */
            break;
        }
        break;
    case 3:
        ROR_ASSERT(0); /* 1217 */
        break;
    case 4:
        ROR_ASSERT(0); /* 1230 */
        break;
    default:
        ROR_ASSERT(0); /* 1252 */
        break;
    }

    if (src != NULL)
    {
        for (int i = 0; i < 4; i++)
            v[i] = src[i];
    }
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00112134

void DogLightPool::setSetLightParam(int type, int n, int param, const float *v, int set)
{
    /* PC: the ROM goes on with a NULL light (n past the set's lights) and
     * writes at its field offset (0x10, 0x20 ...); a NULL light writes
     * nothing here. */
    float *dst = NULL;

    if (param == DOG_LIGHT_PARAM_CONE)
    {
        if (type != DOG_LIGHT_SPOT)
        {
            ROR_ASSERT(0); /* 1297 */
            return;
        }
        DogSpotLight *spot = getSpotLight(set, n);
        if (spot == NULL)
            return;
        spot->m_cone[0] = v[0] * s_pi / 180.0f;
        getSpotLight(set, n)->m_cone[1] = v[1] * s_pi / 180.0f;
        return;
    }

    switch (param)
    {
    case DOG_LIGHT_PARAM_POS:
        if (type == DOG_LIGHT_POINT)
            { DogPointLight *l = getPointLight(set, n); dst = l ? l->m_pos : NULL; }
        else if (type == DOG_LIGHT_SPOT)
            { DogSpotLight *l = getSpotLight(set, n); dst = l ? l->m_pos : NULL; }
        else
            ROR_ASSERT(0); /* 1313 */
        break;
    case DOG_LIGHT_PARAM_DIR:
        if (type == DOG_LIGHT_DIRECT)
            { DogDirectLight *l = getDirectLight(set, n); dst = l ? l->m_dir : NULL; }
        else if (type == DOG_LIGHT_SPOT)
            { DogSpotLight *l = getSpotLight(set, n); dst = l ? l->m_dir : NULL; }
        else
            ROR_ASSERT(0); /* 1328 */
        break;
    case DOG_LIGHT_PARAM_COLOR:
        if (type == DOG_LIGHT_AMBIENT)
            { DogAmbientLight *l = getAmbientLight(set, n); dst = l ? l->m_color : NULL; }
        else if (type == DOG_LIGHT_POINT)
            { DogPointLight *l = getPointLight(set, n); dst = l ? l->m_color : NULL; }
        else if (type == DOG_LIGHT_DIRECT)
            { DogDirectLight *l = getDirectLight(set, n); dst = l ? l->m_color : NULL; }
        else if (type == DOG_LIGHT_SPOT)
            { DogSpotLight *l = getSpotLight(set, n); dst = l ? l->m_color : NULL; }
        else
            ROR_ASSERT(0); /* 1351 */
        break;
    default:
        ROR_ASSERT(0); /* 1356 */
        break;
    }

    if (dst != NULL)
    {
        for (int i = 0; i < 4; i++)
            dst[i] = v[i];
    }
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x001123b0

void DogLightPool::getSetLightParam(int type, int n, int param, float *v, int set) const
{
    /* PC: a NULL light reads nothing (the ROM reads its field offset). */
    const float *src = NULL;

    if (param == DOG_LIGHT_PARAM_CONE)
    {
        if (type != DOG_LIGHT_SPOT)
        {
            ROR_ASSERT(0); /* 1383 */
            return;
        }
        const DogSpotLight *spot = getSpotLight(set, n);
        if (spot == NULL)
            return;
        v[0] = spot->m_cone[0] * 180.0f / s_pi;
        v[1] = getSpotLight(set, n)->m_cone[1] * 180.0f / s_pi;
        return;
    }

    switch (param)
    {
    case DOG_LIGHT_PARAM_POS:
        if (type == DOG_LIGHT_POINT)
            { const DogPointLight *l = getPointLight(set, n); src = l ? l->m_pos : NULL; }
        else if (type == DOG_LIGHT_SPOT)
            { const DogSpotLight *l = getSpotLight(set, n); src = l ? l->m_pos : NULL; }
        else
            ROR_ASSERT(0); /* 1399 */
        break;
    case DOG_LIGHT_PARAM_DIR:
        if (type == DOG_LIGHT_DIRECT)
            { const DogDirectLight *l = getDirectLight(set, n); src = l ? l->m_dir : NULL; }
        else if (type == DOG_LIGHT_SPOT)
            { const DogSpotLight *l = getSpotLight(set, n); src = l ? l->m_dir : NULL; }
        else
            ROR_ASSERT(0); /* 1414 */
        break;
    case DOG_LIGHT_PARAM_COLOR:
        if (type == DOG_LIGHT_AMBIENT)
            { const DogAmbientLight *l = getAmbientLight(set, n); src = l ? l->m_color : NULL; }
        else if (type == DOG_LIGHT_POINT)
            { const DogPointLight *l = getPointLight(set, n); src = l ? l->m_color : NULL; }
        else if (type == DOG_LIGHT_DIRECT)
            { const DogDirectLight *l = getDirectLight(set, n); src = l ? l->m_color : NULL; }
        else if (type == DOG_LIGHT_SPOT)
            { const DogSpotLight *l = getSpotLight(set, n); src = l ? l->m_color : NULL; }
        else
            ROR_ASSERT(0); /* 1437 */
        break;
    default:
        ROR_ASSERT(0); /* 1442 */
        break;
    }

    if (src != NULL)
    {
        for (int i = 0; i < 4; i++)
            v[i] = src[i];
    }
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler inside the get*Light const inlines)
// ROR 0x0011262c

I3dLight *DogLightPool::createLight(i3dContext *ctx, int set, int gray, float scale) const
{
    I3dLightDesc     desc;
    I3dParallelLight direct[8];
    I3dPointLight    point[16];

    i3dLightDescInit(&desc);

    /* the first ambient light of the set */
    u_int mask = m_set_mask[set][DOG_LIGHT_AMBIENT];
    if (mask != 0)
    {
        u_int bit = 1;
        for (int i = 0; i < 8; i++, bit <<= 1)
        {
            if (mask & bit)
            {
                const float *color = getAmbientLight(i)->m_color;
                if (scale == 1.0f)
                {
                    i3dVecCopy2(desc.m_ambient, color);
                }
                else
                {
                    desc.m_ambient[0] = color[0] * scale;
                    desc.m_ambient[1] = color[1] * scale;
                    desc.m_ambient[2] = color[2] * scale;
                }
                if (gray)
                    dogLightToGray(desc.m_ambient);
                break;
            }
        }
    }
    desc.m_ambient[3] = 1.0f;

    int n = 0;
    mask = m_set_mask[set][DOG_LIGHT_DIRECT];
    if (mask != 0)
    {
        u_int bit = 1;
        for (int i = 0; i < 8; i++, bit <<= 1)
        {
            if (mask & bit)
            {
                const DogDirectLight *light = getDirectLight(i);
                i3dVecCopy(direct[n].m_dir, light->m_dir);
                i3dVecCopy2(direct[n].m_color, light->m_color);
                if (gray)
                    dogLightToGray(direct[n].m_color);
                n++;
            }
        }
        desc.m_parallel_count = n;
        desc.m_parallel = direct;
    }

    n = 0;
    mask = m_set_mask[set][DOG_LIGHT_POINT];
    if (mask != 0)
    {
        u_int bit = 1;
        for (int i = 0; i < 16; i++, bit <<= 1)
        {
            if (mask & bit)
            {
                const DogPointLight *light = getPointLight(i);
                i3dVecCopy(point[n].m_pos, light->m_pos);
                i3dVecCopy2(point[n].m_color, light->m_color);
                if (gray)
                    dogLightToGray(point[n].m_color);
                n++;
            }
        }
        desc.m_point_count = n;
        desc.m_point = point;
    }

    return i3dCtxCreateLight(ctx, &desc);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler inside getPointLight)
// ROR 0x001129cc

int DogLightPool::findNearestPointLight(int set, const float *pos)
{
    /* PORT: the ROM starts from the word 0x7fffffff (rodata 0x2e87dc), the
     * largest float on the PS2's FPU and a NaN on the host: FLT_MAX keeps
     * the first light winning. */
    float best = FLT_MAX;
    int   nearest = -1;
    int   i = 0;
    for (u_int mask = m_set_mask[set][DOG_LIGHT_POINT]; mask != 0; mask >>= 1, i++)
    {
        if (!(mask & 1))
            continue;
        const float *p = getPointLight(i)->m_pos;
        float d = (p[0] - pos[0]) * (p[0] - pos[0])
                + (p[1] - pos[1]) * (p[1] - pos[1])
                + (p[2] - pos[2]) * (p[2] - pos[2]);
        if (d < best)
        {
            best = d;
            nearest = i;
        }
    }
    return nearest;
}
