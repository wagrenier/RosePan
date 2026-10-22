/* ==========================================================================
 *  dog/doglightpool.h
 *
 *  DogLightPool (dog/doglightpool.h asserts: "n>=0 && n<m_num[DOG_LIGHT_POINT]",
 *  "m_light[DOG_LIGHT_POINT]"): the scene's lights at MagScene +0x2050 --
 *  per kind (DOG_LIGHT_POINT ...) a light array and its count, grouped in
 *  light sets; MagScene::draw turns set i into the i3d light of render
 *  stage i. Code at 0x1125xx..0x1129cc (unassigned). Opaque until its TU:
 *  EE size at most 0x10c (the scene's next member is at +0x215c).
 * ======================================================================== */
#ifndef _DOG_DOGLIGHTPOOL_H
#define _DOG_DOGLIGHTPOOL_H

#include "lib/unassigned.h"

/* layout: host -- opaque. */
struct DogLightPool
{
    /* The i3d light of light set `set` (ambient scaled by scale; arg is
     * the scene's +0x22cc word, role unknown); NULL on failure. */
    I3dLight *createLight(i3dContext *ctx, int set, int arg, float scale); // ROR 0x0011262c

    u_char _data[0x10c];        // ROR 0x00..0x10b: unverified
};

#endif /* _DOG_DOGLIGHTPOOL_H */
