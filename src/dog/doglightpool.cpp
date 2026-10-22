// dog/doglightpool.cpp -- the scene's light pool (0x1103dc..0x112b08).
//
// A DogLightPool keeps the stage's lights per kind (ambient, direct,
// point, spot ...) and eight light sets, each a bit mask per kind of the
// lights it uses. Each frame MagScene::draw has createLight turn set i
// into the i3d light of render stage i; makeActorShadow asks
// findNearestPointLight for the light an actor's shadow falls from.
//
// Written so far: createLight, findNearestPointLight and the grey helper
// they share; the pool's setup (0x1103dc..0x1123b0) is still to come.

#include "dog/doglightpool.h"

#include <float.h>

#include "common/ror_assert.h"
#include "i3d/i3denv.h"
#include "i3d/i3dmath.h"

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
// callcheck: FUN_00215a30 (the assert handler inside the get*Light inlines)
// ROR 0x0011262c

I3dLight *DogLightPool::createLight(i3dContext *ctx, int set, int gray, float scale)
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
                const DogPointLight *light = getPointLightAt(i);
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
// callcheck: FUN_00215a30 (the assert handler inside getPointLight)
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
