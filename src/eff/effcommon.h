/* ==========================================================================
 *  eff/effcommon.h
 *
 *  The helpers the Eff* classes share (eff/effcommon.cpp, 0x16fb70..
 *  0x1704a4, between EffCameraAnim1 and EffDust2; the file name is
 *  inferred, no source-path string names it): vector and matrix helpers,
 *  the view depth the effects sort by, a node map from a motion to a
 *  model, and the actor probe the footstep / hand effects use to find
 *  which toes and fingers of an actor touch a box.
 * ======================================================================== */
#ifndef _EFF_EFFCOMMON_H
#define _EFF_EFFCOMMON_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "sdk/vu0_intrin.h"

struct DogActor;
struct DogCamera;
struct I3mRoot;
struct I3rItem;
struct I3rRoot;

/* The probe effProbeActorInBox / effProbeActorLimbs fill (the callers keep
 * it on the stack). EE 0x54. layout: host -- the actor pointer. */
struct EffActorProbe
{
    u_int     m_hit;            // ROR 0x00: bit n = limb n is in the box (effProbeActorLimbs)
    DogActor *m_actor;          // ROR 0x04
    float     m_center[3];      // ROR 0x08: the box centre
    float     m_half[3];        // ROR 0x14: its half sizes
    float     m_margin;         // ROR 0x20: added to the x / z half sizes by effProbeActorInBox
    float     m_pos[4][3];      // ROR 0x24: limb n's world position when bit n is set
};

/* The index of the first occurrence of sub in str, -1 when none.
 * Unreferenced. */
int   effStrIndex(const char *str, const char *sub);                     // ROR 0x0016fb70
/* The length of a 3-vector (sqrtf of its squares). */
float effVecLength(const float *v);                                      // ROR 0x0016fc20
/* m made upright, keeping the lengths of its x and z rows: z laid flat
 * (y = 0; a unit z when nothing is left), y = (0, 1, 0), x = y cross z. */
void  effMatMakeUpright(sceVu0FVECTOR *m);                               // ROR 0x0016fc74
/* m = m * a z rotation by angle (i3dMatMul's order). */
void  effMatMulRotZ(float angle, sceVu0FVECTOR *m);                      // ROR 0x0016fd4c
/* m with z = dir (w too), x = up cross dir, y = dir cross x, no
 * translation; up is (0, 1, 0), or z -1 / +1 when dir points up / down. */
void  effMatFromAxisZ(sceVu0FVECTOR *m, const float *dir);               // ROR 0x0016fda0
/* a brought into -pi..pi by whole turns. */
float effWrapAngle(float a);                                             // ROR 0x0016fe88
/* The depth of pos along cam's view axis: the camera's transform, then
 * dot(matrix row 2, pos - row 3). */
float effCalcViewDepth(DogCamera *cam, const float *pos);                // ROR 0x0016ff18
/* out = from + normalize(to - from) * d. */
void  effMoveToward(float d, float *out, const float *from, const float *to); // ROR 0x0016ff78
/* A new u_short table in g_utlMemory: [0] the motion's node count, then
 * for each of its nodes the model's node of that name (0xffff none). */
u_short *effMakeNodeMap(I3rRoot *root, const I3mRoot *motion);           // ROR 0x0016ffec
/* The first type-0x52 child of item whose name contains name (strstr),
 * its index stored in *index when index is given; NULL when none. */
I3rItem *effFindChild52(const I3rItem *item, const char *name, int *index); // ROR 0x001700b0
/* The probe actor's node name in world space; inside the probe's box (all
 * bounds inclusive) sets m_hit bit n and m_pos[n]. */
void  effProbeNode(EffActorProbe *probe, int n, const char *name);       // ROR 0x00170158
/* 1 when the probe actor's position (m_trans) lies in the box with
 * m_margin added to the x and z half sizes (the z half size is unused). */
int   effProbeActorInBox(const EffActorProbe *probe);                    // ROR 0x001702e4
/* The limbs whose motion events fire over the actor's next frame, probed:
 * event 0x10000000 the left toe (bit 0), 0x20000000 the right toe (1),
 * 0x4000000 the left finger (2), 0x8000000 the right finger (3). */
void  effProbeActorLimbs(EffActorProbe *probe);                          // ROR 0x001703c0

#endif /* _EFF_EFFCOMMON_H */
