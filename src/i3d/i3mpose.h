/* ==========================================================================
 *  i3d/i3mpose.h
 *
 *  The i3m pose player (i3d/i3mpose.cpp, ROR 0x22e840..0x2312a0): a
 *  motion's keys sampled at a time into pose records, blends of two
 *  motions or of a sampled motion into a pose, weighted pose mixes, pose
 *  records to node matrices, and the quaternion helpers DogActor uses.
 *
 *  A pose is one I3mPose per motion node, in the motion's node order. A
 *  key (I3mNode::m_key, m_key_stride u16 each) holds indices:
 *    [0] its time and [1] its inverse length (1 / the next key's time
 *        minus its own) in the root's float table,
 *    [2] position, [3] rotation, [4] scale in the float4 table, or the
 *        s16x4 table per I3mRoot::m_flags (I3M_*),
 *    then a spline's two tangent keys: [5..7] and [8..10] ([4..5] and
 *        [6..7] without scale).
 *  The sample flags pick a mode per channel: 0x00f position, 0x0f0
 *  rotation, 0xf00 scale; 0 steps to the nearer key, 1 lerps (slerps),
 *  2 a Hermite spline (squad). Every call site in the ROM passes 0x111.
 * ======================================================================== */
#ifndef _I3D_I3MPOSE_H
#define _I3D_I3MPOSE_H

#include <stddef.h>

#include "i3d/i3mroot.h"
#include "sdk/libvu0.h"

/* One node's pose. EE 0x30, quadword aligned on the EE. */
struct I3mPose
{
    float m_pos[4];             // ROR 0x00: translation (w from the key)
    float m_rot[4];             // ROR 0x10: rotation quaternion (x, y, z, w)
    float m_scale[4];           // ROR 0x20: scale (1, 1, 1, 1 for I3M_NO_SCALE)
};
static_assert(offsetof(I3mPose, m_rot) == 0x10, "I3mPose::m_rot");
static_assert(offsetof(I3mPose, m_scale) == 0x20, "I3mPose::m_scale");
static_assert(sizeof(I3mPose) == 0x30, "I3mPose is 0x30 bytes on the EE");

/* The sample flags' channel modes (see above). */
#define I3M_POS_MASK    0x00f
#define I3M_ROT_MASK    0x0f0
#define I3M_SCALE_MASK  0xf00
#define I3M_SPLINE_ALL  0x222

/* ---- one node ---- */

/* The key whose span holds t: the first key at or before its time, the
 * last at or after its time, NULL without keys. PC: takes the root (the
 * ROM passes the float table) to resolve the node's key offset. */
const u_short *i3mFindKey(float t, const I3mRoot *root, const I3mNode *node); // ROR 0x00230380
/* key sampled at t into out; 0 without a key or root, else 1. */
int   i3mSampleKey(float t, const u_short *key, I3mPose *out, u_int flags,
                   const I3mRoot *root);                                 // ROR 0x00230160
/* One key's values into out. */
void  i3mDecodeKey(I3mPose *out, const u_short *key, const I3mRoot *root); // ROR 0x0022f8d0
/* The key and the next one into out[0], out[1]. */
void  i3mDecodeKeyPair(I3mPose *out, const u_short *key, const I3mRoot *root); // ROR 0x0022f9d8
/* The pair, and the tangent keys into out[2], out[3] for a spline mode. */
void  i3mDecodeKeySpline(I3mPose *out, const u_short *key, const I3mRoot *root,
                         u_int flags);                                   // ROR 0x0022fb90
/* key and the next one mixed per channel at w (1 = key, 0 = the next). */
void  i3mInterpKey(const u_short *key, const I3mRoot *root, I3mPose *out, u_int flags,
                   float w);                                             // ROR 0x0022fd58
/* One node of two motions: root1 at t1, root2 at t2, lerped at r. */
void  i3mGetBlendNodePose(float t1, float t2, float r, I3mPose *pose, const I3mRoot *root1,
                          const I3mNode *node1, const I3mRoot *root2, const I3mNode *node2,
                          u_int flags);                                  // ROR 0x0022e840

/* ---- whole poses (one record per node of root / root1) ---- */

/* The motion's length in frames (the root's m_length). */
float i3mGetLength(const I3mRoot *root);                                 // ROR 0x0022e958
/* Every node's keys sampled at time t (a time per node: ...Times). */
void  i3mGetPose(I3mRoot *root, void *pose, int flags, float t);         // ROR 0x0022e960
void  i3mGetPoseTimes(I3mRoot *root, void *pose, const float *t, int flags); // ROR 0x0022ea08
/* Two motions' poses (root1 at t1, root2 at t2) blended at rate r into
 * pose: r = 1 keeps root1 (i3mPoseLerp). Nodes pair by index. */
void  i3mGetBlendPose(I3mRoot *root1, void *pose, I3mRoot *root2, int flags, float t1,
                      float t2, float r);                                // ROR 0x0022eab0
void  i3mGetBlendPoseRates(I3mRoot *root1, void *pose, I3mRoot *root2, const float *r,
                           int flags, float t1, float t2);               // ROR 0x0022eb78
void  i3mGetBlendPoseTimes(I3mRoot *root1, void *pose, const float *t1, I3mRoot *root2,
                           const float *t2, int flags, float r);         // ROR 0x0022ec48
void  i3mGetBlendPoseTimesRates(I3mRoot *root1, void *pose, const float *t1, I3mRoot *root2,
                                const float *t2, const float *r, int flags); // ROR 0x0022ed28
/* A motion sampled at t lerped into the pose already there at r (r = 1
 * keeps the pose). */
void  i3mBlendPose(I3mRoot *root, void *pose, int flags, float t, float r); // ROR 0x0022ee08
void  i3mBlendPoseRates(I3mRoot *root, void *pose, const float *r, int flags, float t); // ROR 0x0022eed8
void  i3mBlendPoseTimes(I3mRoot *root, void *pose, const float *t, int flags, float r); // ROR 0x0022efa0
void  i3mBlendPoseTimesRates(I3mRoot *root, void *pose, const float *t, const float *r,
                             int flags);                                 // ROR 0x0022f070
/* 1 when both motions have the same node names in the same order. */
int   i3mIsSameNodes(const I3mRoot *root1, const I3mRoot *root2);        // ROR 0x0022f160

/* ---- pose records ---- */

void  i3mPoseClear(void *pose, int n);                                   // ROR 0x0022f220
/* pose += src * w per node (the rotation subtracted when it points away). */
void  i3mPoseAccum(float w, void *pose, int n, const void *src);         // ROR 0x0022f240
void  i3mPoseAccumNodes(void *pose, int n, const void *src, const float *w); // ROR 0x0022f358
void  i3mPoseNormalizeRot(void *pose, int n);                            // ROR 0x0022f478
/* The weighted sum of count poses, rotations renormalized. */
void  i3mPoseMix(void *pose, int n, int count, const void *const *src, const float *w); // ROR 0x0022f4d0
void  i3mPoseMixNodes(void *pose, int n, int count, const void *const *src,
                      const float *const *w);                            // ROR 0x0022f570
/* One record: out = a * t + b * (1 - t), the rotation slerped (shorter
 * arc). out may be a or b. */
void  i3mPoseLerp(float t, void *out, const void *a, const void *b);     // ROR 0x0022f610
/* Pose records to 0x40-byte node matrices through map (u16 count, then a
 * model node per motion node, 0xffff = none): rows scaled by the scale,
 * the translation in row 3. */
void  i3mPoseToMatrices(const void *pose, sceVu0FMATRIX *mats, const u_short *map); // ROR 0x0022f7a0

/* ---- quaternions (x, y, z, w) ---- */

/* PC: the ROM's three take and return VU0 registers (vf1, vf2, vf3.x). */
void  i3mHermite(float w, float *out, const float *a0, const float *a1, const float *a2,
                 const float *a3);                                       // ROR 0x0022f870
void  i3mQuatSlerpShort(float w, float *out, const float *a, const float *b); // ROR 0x0022fc38
void  i3mQuatSlerpNorm(float w, float *out, const float *a, const float *b);  // ROR 0x00230258
/* The product a * b. out may be a or b. */
void  i3mQuatMul(float *out, const float *a, const float *b);            // ROR 0x00230470
void  i3mQuatLog(float *out, const float *q);                            // ROR 0x002304e8
void  i3mQuatExp(float *out, const float *v);                            // ROR 0x00230570
float i3mQuatDot(const float *a, const float *b);                        // ROR 0x00230610
void  i3mQuatNormalize(float *out, const float *in);                     // ROR 0x00230660
/* Unreferenced in the ROM; the ...Q forms pass quadwords in GPRs there. */
void  i3mQuatSlerpShortMem(float w, float *out, const float *a, const float *b); // ROR 0x00230688
void  i3mQuatSlerpShortQ(float w, float *out, const float *a, const float *b);   // ROR 0x002307c0
void  i3mQuatLerpShort(float w, float *out, const float *a, const float *b);     // ROR 0x002308f0
void  i3mQuatSlerpNormMem(float w, float *out, const float *a, const float *b);  // ROR 0x00230998
void  i3mQuatSlerpNormQ(float w, float *out, const float *a, const float *b);    // ROR 0x00230ae8
/* slerp(slerp(w, c, d), slerp(w, a, b), 2w(1 - w)). */
void  i3mQuatSquadMem(float w, float *out, const float *a, const float *b, const float *c,
                      const float *d);                                   // ROR 0x00230c10
void  i3mQuatSquadQ(float w, float *out, const float *a, const float *b, const float *c,
                    const float *d);                                     // ROR 0x00230cf8
/* Rotation rows from q, row 3 = trans. */
void  i3mQuatToMatrix(sceVu0FVECTOR *m, const float *q, const float *trans); // ROR 0x00230d70
void  i3mQuatFromMatrix(float *out, const sceVu0FVECTOR *m);             // ROR 0x00230df8
/* The squad control point at q from its neighbours. */
void  i3mQuatSquadCtrl(float *out, const float *prev, const float *q, const float *next); // ROR 0x00231098
/* From the euler angles rot (x, y, z). */
void  i3mQuatFromEuler(float *out, const float *rot);                    // ROR 0x00231198

#endif /* _I3D_I3MPOSE_H */
