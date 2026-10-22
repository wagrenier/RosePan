/* ==========================================================================
 *  dog/doglightpool.h
 *
 *  DogLightPool (dog/doglightpool.h asserts: "n>=0 && n<m_num[DOG_LIGHT_POINT]",
 *  "m_light[DOG_LIGHT_POINT]"): the scene's lights at MagScene +0x2050 --
 *  per kind (DOG_LIGHT_POINT ...) a light array and its count, grouped in
 *  light sets; MagScene::draw turns set i into the i3d light of render
 *  stage i. Code at 0x1125xx..0x1129cc (unassigned). Known: the per-kind
 *  arrays (+0x00) and counts (+0x18); the rest opaque until its TU. EE
 *  size at most 0x10c (the scene's next member is at +0x215c).
 * ======================================================================== */
#ifndef _DOG_DOGLIGHTPOOL_H
#define _DOG_DOGLIGHTPOOL_H

#include "common/ror_assert.h"
#include "lib/unassigned.h"

/* The light kinds (the assert texts name DIRECT, POINT and SPOT; the
 * arrays have six). */
enum
{
    DOG_LIGHT_DIRECT = 1,
    DOG_LIGHT_POINT  = 2,
    DOG_LIGHT_SPOT   = 3,
    DOG_LIGHT_KIND   = 6,
};

/* A point light (stride 0x20): its position first; the rest unverified. */
struct DogPointLight
{
    sceVu0FVECTOR m_pos;        // ROR 0x00
    sceVu0FVECTOR m_10;         // ROR 0x10: unverified (colour?)
};
static_assert(sizeof(DogPointLight) == 0x20, "DogPointLight is 0x20 bytes on the EE");

/* layout: host -- pointers; EE offsets in the comments. */
struct DogLightPool
{
    /* The i3d light of light set `set` (ambient scaled by scale; arg is
     * the scene's +0x22cc word, role unknown); NULL on failure. */
    I3dLight *createLight(i3dContext *ctx, int set, int arg, float scale); // ROR 0x0011262c
    /* The point light of light set `set` nearest to pos, -1 when the set
     * has none. */
    int findNearestPointLight(int set, const float *pos);               // ROR 0x001129cc

    /* Point light n (inline; doglightpool.h line 131). */
    DogPointLight *getPointLight(int n)
    {
        ROR_ASSERT(n>=0 && n<m_num[DOG_LIGHT_POINT]); /* 131 */
        ROR_ASSERT(m_light[DOG_LIGHT_POINT]); /* 131 */
        return (DogPointLight *)m_light[DOG_LIGHT_POINT] + n;
    }

    void   *m_light[DOG_LIGHT_KIND]; // ROR 0x00: per kind, its lights (DIRECT / POINT 0x20 apart, SPOT 0x40)
    int     m_num[DOG_LIGHT_KIND];   // ROR 0x18: per kind, the count
    u_char  _30[0x10c - 0x30];       // ROR 0x30: unverified (the light sets' masks from +0x50, 0x18 per set)
};

#endif /* _DOG_DOGLIGHTPOOL_H */
