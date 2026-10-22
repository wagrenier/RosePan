/* ==========================================================================
 *  dog/doglightpool.h
 *
 *  DogLightPool (dog/doglightpool.cpp, 0x1103dc..0x112b08): the scene's
 *  lights at MagScene +0x2050 -- per kind (DOG_LIGHT_AMBIENT, DIRECT,
 *  POINT, SPOT ...) a light array and its count, and eight light sets,
 *  each a bit mask per kind of the lights it uses. MagScene::draw turns
 *  set i into the i3d light of render stage i (createLight). EE size at
 *  most 0x10c (the scene's next member is at +0x215c).
 * ======================================================================== */
#ifndef _DOG_DOGLIGHTPOOL_H
#define _DOG_DOGLIGHTPOOL_H

#include "common/ror_assert.h"
#include "lib/unassigned.h"

/* The light kinds (the assert texts name AMBIENT, DIRECT, POINT and SPOT;
 * the arrays have six, DOG_MAX_LIGHT_... in the asserts). */
enum
{
    DOG_LIGHT_AMBIENT = 0,
    DOG_LIGHT_DIRECT = 1,
    DOG_LIGHT_POINT  = 2,
    DOG_LIGHT_SPOT   = 3,
    DOG_LIGHT_KIND   = 6,
};

/* The number of light sets (the asserts' "theSetNo >= 0 && theSetNo < 8"). */
#define DOG_LIGHT_SET_MAX 8

/* An ambient light (stride 0x10): its colour. */
struct DogAmbientLight
{
    sceVu0FVECTOR m_color;      // ROR 0x00
};
static_assert(sizeof(DogAmbientLight) == 0x10, "DogAmbientLight is 0x10 bytes on the EE");

/* A directional light (stride 0x20), copied into an I3dParallelLight. */
struct DogDirectLight
{
    sceVu0FVECTOR m_dir;        // ROR 0x00
    sceVu0FVECTOR m_color;      // ROR 0x10
};
static_assert(sizeof(DogDirectLight) == 0x20, "DogDirectLight is 0x20 bytes on the EE");

/* A point light (stride 0x20), copied into an I3dPointLight. */
struct DogPointLight
{
    sceVu0FVECTOR m_pos;        // ROR 0x00
    sceVu0FVECTOR m_color;      // ROR 0x10
};
static_assert(sizeof(DogPointLight) == 0x20, "DogPointLight is 0x20 bytes on the EE");

/* r = g = b = the colour's luminance (0.299 r + 0.587 g + 0.114 b). */
void dogLightToGray(float *color);                                     // ROR 0x00110bb8

/* layout: host -- pointers; EE offsets in the comments. */
struct DogLightPool
{
    /* The i3d light of light set `set`: its first ambient light (scaled
     * by scale, alpha 1), up to 8 direct and 16 point lights; with gray
     * set (the scene's +0x22cc word) every colour turns to its luminance.
     * NULL on failure. */
    I3dLight *createLight(i3dContext *ctx, int set, int gray, float scale); // ROR 0x0011262c
    /* The point light of light set `set` nearest to pos, -1 when the set
     * has none. */
    int findNearestPointLight(int set, const float *pos);               // ROR 0x001129cc

    /* Light n of a kind (inline; doglightpool.h lines 140..142). */
    DogAmbientLight *getAmbientLight(int n)
    {
        ROR_ASSERT(n>=0 && n<m_num[DOG_LIGHT_AMBIENT]); /* 140 */
        ROR_ASSERT(m_light[DOG_LIGHT_AMBIENT]); /* 140 */
        return (DogAmbientLight *)m_light[DOG_LIGHT_AMBIENT] + n;
    }
    DogDirectLight *getDirectLight(int n)
    {
        ROR_ASSERT(n>=0 && n<m_num[DOG_LIGHT_DIRECT]); /* 141 */
        ROR_ASSERT(m_light[DOG_LIGHT_DIRECT]); /* 141 */
        return (DogDirectLight *)m_light[DOG_LIGHT_DIRECT] + n;
    }
    /* Line 142 repeats line 131's point getter; the name of either is a
     * guess (a const twin?). */
    DogPointLight *getPointLightAt(int n)
    {
        ROR_ASSERT(n>=0 && n<m_num[DOG_LIGHT_POINT]); /* 142 */
        ROR_ASSERT(m_light[DOG_LIGHT_POINT]); /* 142 */
        return (DogPointLight *)m_light[DOG_LIGHT_POINT] + n;
    }

    /* Point light n (inline; doglightpool.h line 131). */
    DogPointLight *getPointLight(int n)
    {
        ROR_ASSERT(n>=0 && n<m_num[DOG_LIGHT_POINT]); /* 131 */
        ROR_ASSERT(m_light[DOG_LIGHT_POINT]); /* 131 */
        return (DogPointLight *)m_light[DOG_LIGHT_POINT] + n;
    }

    void   *m_light[DOG_LIGHT_KIND]; // ROR 0x00: per kind, its lights (AMBIENT 0x10 apart, DIRECT / POINT 0x20, SPOT 0x40)
    int     m_num[DOG_LIGHT_KIND];   // ROR 0x18: per kind, the count
    u_char  _30[0x48 - 0x30];        // ROR 0x30: unverified (6 words, one per kind?)
    u_int   m_set_mask[DOG_LIGHT_SET_MAX][DOG_LIGHT_KIND]; // ROR 0x48: per light set and kind, bit i = light i in the set
    u_char  _108[0x10c - 0x108];     // ROR 0x108: unverified
};

#endif /* _DOG_DOGLIGHTPOOL_H */
