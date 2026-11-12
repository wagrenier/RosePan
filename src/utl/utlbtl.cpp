/* ==========================================================================
 *  utl/utlbtl.cpp
 *
 *  The battle checks. See utlbtl.h.
 *
 *  The sight checks compare against float constants one ulp below the
 *  round decimal (0x3e999999 for 0.3, 0x3f599999 for 0.85): written as
 *  the ROM's values. Their sqrt is sqrt.s with the sqrtf fallback for a
 *  NaN, i.e. sqrtf.
 * ======================================================================== */
#include "utl/utlbtl.h"

#include <math.h>
#include <string.h>

#include "common/ror_assert.h"
#include "i3d/i3dgeom.h"
#include "i3d/i3dmath.h"
#include "sdk/libvu0.h"
#include "utl/utlrand.h"

/* ---- text ------------------------------------------------------------- */

/* ROR 0x0010452c */
void utlInitSightCheck(UtlSightCheck *theHitCheck)
{
    memset(theHitCheck, 0, sizeof(UtlSightCheck));
}

/* ROR 0x0010454c */
void utlInitHitCheck(UtlHitCheck *theHitCheck)
{
    memset(theHitCheck, 0, sizeof(UtlHitCheck));
}

/* ROR 0x0010456c */ // callcheck: dbgAssertFailed (the ROR_ASSERT handler)
int utlHitCheck(UtlHitCheck *theHitCheck)
{
    sceVu0FVECTOR dir;
    sceVu0FVECTOR axis;

    ROR_ASSERT(theHitCheck);                                    /* line 60 */
    ROR_ASSERT(theHitCheck->m_origin_pos);                      /* line 61 */
    ROR_ASSERT(theHitCheck->m_target_pos);                      /* line 62 */

    if (theHitCheck->m_dir == NULL) {
        /* No sweep: the sphere at the origin against the target, a
         * capsule rising m_10 on y when that is set. */
        if (theHitCheck->m_10 == 0.0f) {
            return i3dSphereSphereTest(theHitCheck->m_origin_pos, theHitCheck->m_target_pos);
        }
        dir[0] = 0.0f;
        dir[1] = theHitCheck->m_10;
        dir[2] = 0.0f;
        dir[3] = 0.0f;
        return i3dSphereCylSegTest(theHitCheck->m_origin_pos, theHitCheck->m_target_pos, dir);
    }

    /* A ray of m_len from the origin against the target sphere, or the
     * target's flat cylinder of height m_10. */
    i3dVecNormalize2(dir, theHitCheck->m_dir);
    i3dVecScaleXYZ(theHitCheck->m_len, dir, dir);
    if (theHitCheck->m_10 == 0.0f) {
        return i3dSphereRayTest(theHitCheck->m_target_pos, theHitCheck->m_origin_pos, dir);
    }
    axis[0] = 0.0f;
    axis[1] = theHitCheck->m_10;
    axis[2] = 0.0f;
    axis[3] = 0.0f;
    return i3dFlatCylinderRayTest(theHitCheck->m_target_pos, axis, theHitCheck->m_origin_pos, dir);
}

/* ROR 0x001046b4 */ // callcheck: dbgAssertFailed (the ROR_ASSERT handler)
int utlSightCheck(UtlSightCheck *theHitCheck)
{
    sceVu0FVECTOR aim;
    sceVu0FVECTOR to;
    float dist;
    float dot;
    float angle;
    int ret = 0;

    ROR_ASSERT(theHitCheck->m_pos_self);                        /* line 148 */
    ROR_ASSERT(theHitCheck->m_pos_aim);                         /* line 149 */
    sceVu0SubVector(aim, (float *)theHitCheck->m_pos_aim, (float *)theHitCheck->m_pos_self);
    sceVu0Normalize(aim, aim);
    ROR_ASSERT(theHitCheck->m_pos_target);                      /* line 153 */
    sceVu0SubVector(to, (float *)theHitCheck->m_pos_target, (float *)theHitCheck->m_pos_self);
    dist = sqrtf(to[0] * to[0] + to[1] * to[1] + to[2] * to[2]);
    sceVu0Normalize(to, to);
    dot = sceVu0InnerProduct(aim, to);

    switch (theHitCheck->m_type) {
    case 0:
        if (dist <= 100.0f && dot > 0.29999998f)
            ret = 1;
        return ret;
    case 1:
        if (dist <= 120.0f && dot > 0.29999998f)
            ret = 1;
        return ret;
    case 2:
        if (dist > theHitCheck->m_width + (float)theHitCheck->m_range)
            return ret;
        if (dot <= 0.0f)
            return ret;
        angle = acosf(dot);
        break;
    case 3:
        /* The cone turns by up to +-100 degrees (utlRandMean of four
         * draws: mostly near 0). */
        if (dist > 500.0f)
            return ret;
        if (dot <= 0.0f)
            return ret;
        angle = acosf(dot);
        angle += (((float)utlRandMean(-10000, 10000, 4) / 100.0f) * 3.1415925f) / 180.0f;
        break;
    default:
        return ret;
    }
    if (dist * tanf(angle) <= theHitCheck->m_width)
        ret = 1;
    return ret;
}

/* ROR 0x00104970 */
int utlSightCheck2(UtlSightCheck *theHitCheck)
{
    sceVu0FVECTOR aim;
    sceVu0FVECTOR to;
    float dist;
    float dot;

    sceVu0SubVector(aim, (float *)theHitCheck->m_pos_aim, (float *)theHitCheck->m_pos_self);
    sceVu0Normalize(aim, aim);
    sceVu0SubVector(to, (float *)theHitCheck->m_pos_target, (float *)theHitCheck->m_pos_self);
    dist = sqrtf(to[0] * to[0] + to[1] * to[1] + to[2] * to[2]);
    sceVu0Normalize(to, to);
    dot = sceVu0InnerProduct(aim, to);

    if (theHitCheck->m_type == 0) {
        return dist <= 100.0f && dot > 0.84999996f;
    }
    if (theHitCheck->m_type == 0x100) {
        if (dist > 250.0f || dot <= 0.0f)
            return 0;
        return dist * tanf(acosf(dot)) <= theHitCheck->m_width;
    }
    return 0;
}

/* ROR 0x00104ad0 */
int utlSightCheck3(UtlSightCheck *theHitCheck)
{
    sceVu0FVECTOR aim;
    sceVu0FVECTOR to;
    float dist;
    float dot;

    sceVu0SubVector(aim, (float *)theHitCheck->m_pos_aim, (float *)theHitCheck->m_pos_self);
    sceVu0Normalize(aim, aim);
    sceVu0SubVector(to, (float *)theHitCheck->m_pos_target, (float *)theHitCheck->m_pos_self);
    dist = sqrtf(to[0] * to[0] + to[1] * to[1] + to[2] * to[2]);
    sceVu0Normalize(to, to);
    dot = sceVu0InnerProduct(aim, to);

    if (theHitCheck->m_type == 0) {
        return dist <= 50.0f && dot > 0.84999996f;
    }
    if (theHitCheck->m_type == 6) {
        /* The ROM's band test can never reject (beyond 200 and nearer than
         * 100 at once); kept as compiled. */
        if (dist > 200.0f && dist < 100.0f)
            return 0;
        if (dot <= 0.0f)
            return 0;
        return dist * tanf(acosf(dot)) <= theHitCheck->m_width;
    }
    return 0;
}

/* ROR 0x00104c44 */
int utlSightCheck4(UtlSightCheck *theHitCheck)
{
    sceVu0FVECTOR aim;
    sceVu0FVECTOR to;
    float dist;
    float dot;

    sceVu0SubVector(aim, (float *)theHitCheck->m_pos_aim, (float *)theHitCheck->m_pos_self);
    sceVu0Normalize(aim, aim);
    sceVu0SubVector(to, (float *)theHitCheck->m_pos_target, (float *)theHitCheck->m_pos_self);
    dist = sqrtf(to[0] * to[0] + to[1] * to[1] + to[2] * to[2]);
    sceVu0Normalize(to, to);
    dot = sceVu0InnerProduct(aim, to);

    switch (theHitCheck->m_type) {
    case 0:
        return dist <= 50.0f && dot > 0.5f;
    case 4:
        return dist <= 100.0f && dot > 0.5f;
    case 2:
        if (dist > (float)theHitCheck->m_range)
            return 0;
        break;
    case 5:
        if (dist > 250.0f || dist < 150.0f)
            return 0;
        break;
    default:
        return 0;
    }
    if (dot <= 0.0f)
        return 0;
    return dist * tanf(acosf(dot)) <= theHitCheck->m_width;
}
