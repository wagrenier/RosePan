/* ==========================================================================
 *  i3d/i3mpose.cpp
 *
 *  The i3m pose player (ROR 0x22e840..0x2312a0): key search and decode,
 *  per-channel interpolation (step, lerp / slerp, Hermite / squad), the
 *  pose samplers and blends DogActor and DogMotionBlend call, weighted
 *  pose mixes, pose to node matrices, and the quaternion helpers. See
 *  i3mpose.h for the key layout and the sample flags.
 *
 *  The ROM is VU0 macro-mode code; it is written out as plain float code
 *  that keeps the ROM's operation order. Every function reads its inputs
 *  before it writes, so outputs may alias inputs.
 *
 *  Not written yet: 0x230688 and 0x230df8 (no direct callers; quaternion
 *  spline helpers around i3mQuatLog / i3mQuatExp). Three unreferenced
 *  leaves hide in the bodies before them: 0x230370 (a node's name word),
 *  0x230378 (a node's key count) and 0x230638 (i3mQuatDot of two
 *  quadwords passed by value); nothing reaches them.
 * ======================================================================== */
#include "i3d/i3mpose.h"

#include <math.h>
#include <string.h>

#include "i3d/i3dmath.h"

/* rodata 0x303a78.. : the slerp switch (0.9) and the Hermite basis. */
static const float s_slerp_max = 0.9f;
static const float s_hermite[4][4] = {
    { 2.0f, -2.0f, 1.0f, 1.0f },
    { -3.0f, 3.0f, -2.0f, -1.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f, 0.0f },
};

static inline float dot4(const float *a, const float *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

/* vrsqrt / vmulq: v scaled to unit length. */
static inline void normalize4(float *out, const float *v)
{
    float q = 1.0f / sqrtf(dot4(v, v));

    for (int i = 0; i < 4; i++)
        out[i] = v[i] * q;
}

/* ACC = a * t; ACC += b; out = ACC - b * t (vmulax / vmaddaw / vmsubx). */
static inline void lerp4(float t, float *out, const float *a, const float *b)
{
    for (int i = 0; i < 4; i++)
        out[i] = a[i] * t + b[i] - b[i] * t;
}

/* (a sin(th t) + b sin(th (1 - t))) / sin(th), th = acos(d). */
static inline void slerp4(float t, float d, float *out, const float *a, const float *b)
{
    float th = acosf(d);
    float inv = 1.0f / sinf(th);
    float sa = sinf(th * t);
    float sb = sinf(th * (1.0f - t));

    for (int i = 0; i < 4; i++)
        out[i] = (a[i] * sa + b[i] * sb) * inv;
}

/* One packed s16x4 1.15 vector (pextlh / psraw / vitof15), as offset +
 * value * scale when scale is given. */
static inline void unpack4(float *out, const short *v, const float *scale, const float *offset)
{
    for (int i = 0; i < 4; i++)
    {
        float f = v[i] * (1.0f / 32768.0f);
        out[i] = scale != NULL ? offset[i] + f * scale[i] : f;
    }
}

static inline const I3mNode *nodeAt(const I3mRoot *root, int i)
{
    return (const I3mNode *)i3mPtr(root, root->m_node) + i;
}

static inline const float *floatTable(const I3mRoot *root)
{
    return (const float *)i3mPtr(root, root->m_float);
}

// ──────────────────────────────────────────────────────────────────────
// Two motions, one node: both nodes' keys sampled, the second lerped
// into the first at r; the second alone when the first has no key.
// ROR 0x0022e840

void i3mGetBlendNodePose(float t1, float t2, float r, I3mPose *pose, const I3mRoot *root1,
                         const I3mNode *node1, const I3mRoot *root2, const I3mNode *node2,
                         u_int flags)
{
    I3mPose tmp;
    const u_short *k1, *k2;
    int ok1, ok2;

    k1 = i3mFindKey(t1, root1, node1);
    k2 = i3mFindKey(t2, root2, node2);
    ok1 = i3mSampleKey(t1, k1, pose, flags, root1);
    ok2 = i3mSampleKey(t2, k2, &tmp, flags, root2);
    if (ok1)
    {
        if (ok2)
            i3mPoseLerp(r, pose, pose, &tmp);
    }
    else if (ok2)
        *pose = tmp;
}

/* ROR 0x0022e958 */
float i3mGetLength(const I3mRoot *root)
{
    return root->m_length;
}

/* ROR 0x0022e960 */
void i3mGetPose(I3mRoot *root, void *pose, int flags, float t)
{
    I3mPose *out = (I3mPose *)pose;

    for (u_int i = 0; i < root->m_num_node; i++, out++)
        i3mSampleKey(t, i3mFindKey(t, root, nodeAt(root, i)), out, flags, root);
}

/* ROR 0x0022ea08 */
void i3mGetPoseTimes(I3mRoot *root, void *pose, const float *t, int flags)
{
    I3mPose *out = (I3mPose *)pose;

    for (u_int i = 0; i < root->m_num_node; i++, out++, t++)
        i3mSampleKey(*t, i3mFindKey(*t, root, nodeAt(root, i)), out, flags, root);
}

/* ROR 0x0022eab0 */
void i3mGetBlendPose(I3mRoot *root1, void *pose, I3mRoot *root2, int flags, float t1, float t2,
                     float r)
{
    I3mPose *out = (I3mPose *)pose;

    for (u_int i = 0; i < root1->m_num_node; i++, out++)
        i3mGetBlendNodePose(t1, t2, r, out, root1, nodeAt(root1, i), root2, nodeAt(root2, i), flags);
}

/* ROR 0x0022eb78 */
void i3mGetBlendPoseRates(I3mRoot *root1, void *pose, I3mRoot *root2, const float *r, int flags,
                          float t1, float t2)
{
    I3mPose *out = (I3mPose *)pose;

    for (u_int i = 0; i < root1->m_num_node; i++, out++, r++)
        i3mGetBlendNodePose(t1, t2, *r, out, root1, nodeAt(root1, i), root2, nodeAt(root2, i), flags);
}

/* ROR 0x0022ec48 */
void i3mGetBlendPoseTimes(I3mRoot *root1, void *pose, const float *t1, I3mRoot *root2,
                          const float *t2, int flags, float r)
{
    I3mPose *out = (I3mPose *)pose;
    int n = root1->m_num_node;

    for (int i = 0; i < n; i++, out++)
        i3mGetBlendNodePose(t1[i], t2[i], r, out, root1, nodeAt(root1, i), root2, nodeAt(root2, i),
                            flags);
}

/* ROR 0x0022ed28 */
void i3mGetBlendPoseTimesRates(I3mRoot *root1, void *pose, const float *t1, I3mRoot *root2,
                               const float *t2, const float *r, int flags)
{
    I3mPose *out = (I3mPose *)pose;
    int n = root1->m_num_node;

    for (int i = 0; i < n; i++, out++)
        i3mGetBlendNodePose(t1[i], t2[i], r[i], out, root1, nodeAt(root1, i), root2,
                            nodeAt(root2, i), flags);
}

// ──────────────────────────────────────────────────────────────────────
// A motion sampled into a temp record per node and lerped into the pose
// already there; nodes without keys keep their pose.
// ROR 0x0022ee08

void i3mBlendPose(I3mRoot *root, void *pose, int flags, float t, float r)
{
    I3mPose *out = (I3mPose *)pose;
    I3mPose tmp;

    for (u_int i = 0; i < root->m_num_node; i++, out++)
    {
        if (i3mSampleKey(t, i3mFindKey(t, root, nodeAt(root, i)), &tmp, flags, root))
            i3mPoseLerp(r, out, out, &tmp);
    }
}

/* ROR 0x0022eed8 */
void i3mBlendPoseRates(I3mRoot *root, void *pose, const float *r, int flags, float t)
{
    I3mPose *out = (I3mPose *)pose;
    I3mPose tmp;

    for (u_int i = 0; i < root->m_num_node; i++, out++, r++)
    {
        if (i3mSampleKey(t, i3mFindKey(t, root, nodeAt(root, i)), &tmp, flags, root))
            i3mPoseLerp(*r, out, out, &tmp);
    }
}

/* ROR 0x0022efa0 */
void i3mBlendPoseTimes(I3mRoot *root, void *pose, const float *t, int flags, float r)
{
    I3mPose *out = (I3mPose *)pose;
    I3mPose tmp;

    for (u_int i = 0; i < root->m_num_node; i++, out++, t++)
    {
        if (i3mSampleKey(*t, i3mFindKey(*t, root, nodeAt(root, i)), &tmp, flags, root))
            i3mPoseLerp(r, out, out, &tmp);
    }
}

/* ROR 0x0022f070 */
void i3mBlendPoseTimesRates(I3mRoot *root, void *pose, const float *t, const float *r, int flags)
{
    I3mPose *out = (I3mPose *)pose;
    I3mPose tmp;
    int n = root->m_num_node;

    for (int i = 0; i < n; i++, out++)
    {
        if (i3mSampleKey(t[i], i3mFindKey(t[i], root, nodeAt(root, i)), &tmp, flags, root))
            i3mPoseLerp(r[i], out, out, &tmp);
    }
}

/* ROR 0x0022f160 */
int i3mIsSameNodes(const I3mRoot *root1, const I3mRoot *root2)
{
    int n = i3mGetNumNode(root1);

    if (n != i3mGetNumNode(root2))
        return 0;
    for (int i = 0; i < n; i++)
    {
        if (strcmp(i3mGetNodeName(root1, i & 0xffff), i3mGetNodeName(root2, i & 0xffff)) != 0)
            return 0;
    }
    return 1;
}

/* ROR 0x0022f220 */
void i3mPoseClear(void *pose, int n)
{
    memset(pose, 0, n * sizeof(I3mPose));
}

// ──────────────────────────────────────────────────────────────────────
// pose += src * w, last node first. The rotation is subtracted when it
// points away from the sum so far (the shorter arc of a weighted mix).
// ROR 0x0022f240

void i3mPoseAccum(float w, void *pose, int n, const void *src)
{
    I3mPose *out = (I3mPose *)pose;
    const I3mPose *in = (const I3mPose *)src;
    float tmp[4];

    for (int i = n - 1; i >= 0; i--)
    {
        i3dVecScale(w, tmp, in[i].m_rot);
        if (i3mQuatDot(out[i].m_rot, tmp) < 0.0f)
            i3dVecSub(out[i].m_rot, out[i].m_rot, tmp);
        else
            i3dVecAdd(out[i].m_rot, out[i].m_rot, tmp);
        i3dVecScale(w, tmp, in[i].m_pos);
        i3dVecAdd(out[i].m_pos, out[i].m_pos, tmp);
        i3dVecScale(w, tmp, in[i].m_scale);
        i3dVecAdd(out[i].m_scale, out[i].m_scale, tmp);
    }
}

/* ROR 0x0022f358: i3mPoseAccum with a weight per node. */
void i3mPoseAccumNodes(void *pose, int n, const void *src, const float *w)
{
    I3mPose *out = (I3mPose *)pose;
    const I3mPose *in = (const I3mPose *)src;
    float tmp[4];

    for (int i = n - 1; i >= 0; i--)
    {
        i3dVecScale(w[i], tmp, in[i].m_rot);
        if (i3mQuatDot(out[i].m_rot, tmp) < 0.0f)
            i3dVecSub(out[i].m_rot, out[i].m_rot, tmp);
        else
            i3dVecAdd(out[i].m_rot, out[i].m_rot, tmp);
        i3dVecScale(w[i], tmp, in[i].m_pos);
        i3dVecAdd(out[i].m_pos, out[i].m_pos, tmp);
        i3dVecScale(w[i], tmp, in[i].m_scale);
        i3dVecAdd(out[i].m_scale, out[i].m_scale, tmp);
    }
}

/* ROR 0x0022f478 */
void i3mPoseNormalizeRot(void *pose, int n)
{
    I3mPose *out = (I3mPose *)pose;

    for (int i = n - 1; i >= 0; i--)
        i3mQuatNormalize(out[i].m_rot, out[i].m_rot);
}

/* ROR 0x0022f4d0 */
void i3mPoseMix(void *pose, int n, int count, const void *const *src, const float *w)
{
    i3mPoseClear(pose, n);
    for (int k = 0; k < count; k++)
        i3mPoseAccum(w[k], pose, n, src[k]);
    i3mPoseNormalizeRot(pose, n);
}

/* ROR 0x0022f570 */
void i3mPoseMixNodes(void *pose, int n, int count, const void *const *src, const float *const *w)
{
    i3mPoseClear(pose, n);
    for (int k = 0; k < count; k++)
        i3mPoseAccumNodes(pose, n, src[k], w[k]);
    i3mPoseNormalizeRot(pose, n);
}

// ──────────────────────────────────────────────────────────────────────
// out = a * t + b * (1 - t). The rotation takes the shorter arc (a's
// quaternion negated when the dot is negative): slerped while the dot is
// under 0.9, else lerped and normalized.
// ROR 0x0022f610
// callcheck: acosf, sinf (in slerp4)

void i3mPoseLerp(float t, void *out, const void *a, const void *b)
{
    I3mPose *o = (I3mPose *)out;
    const I3mPose *pa = (const I3mPose *)a;
    const I3mPose *pb = (const I3mPose *)b;
    float qa[4], qb[4], q[4], pos[4], scale[4];
    float d;

    for (int i = 0; i < 4; i++)
    {
        qa[i] = pa->m_rot[i];
        qb[i] = pb->m_rot[i];
    }
    d = dot4(qa, qb);
    if (d < 0.0f)
    {
        d = -d;
        for (int i = 0; i < 4; i++)
            qa[i] = qa[i] * -1.0f;
    }
    if (d < s_slerp_max)
        slerp4(t, d, q, qa, qb);
    else
    {
        lerp4(t, q, qa, qb);
        normalize4(q, q);
    }
    lerp4(t, pos, pa->m_pos, pb->m_pos);
    lerp4(t, scale, pa->m_scale, pb->m_scale);
    for (int i = 0; i < 4; i++)
    {
        o->m_rot[i] = q[i];
        o->m_pos[i] = pos[i];
        o->m_scale[i] = scale[i];
    }
}

// ──────────────────────────────────────────────────────────────────────
// Each mapped node's matrix: the quaternion's rotation, row i scaled by
// the scale's component i (w 0), the translation as row 3.
// ROR 0x0022f7a0

void i3mPoseToMatrices(const void *pose, sceVu0FMATRIX *mats, const u_short *map)
{
    const I3mPose *p = (const I3mPose *)pose;
    const I3mPose *end = p + map[0];

    for (map++; p != end; p++, map++)
    {
        if (*map == 0xffff)
            continue;
        float *m = &mats[*map][0][0];
        float x = p->m_rot[0], y = p->m_rot[1], z = p->m_rot[2], w = p->m_rot[3];
        float x2 = x + x, y2 = y + y, z2 = z + z;
        float xx = x2 * x, xy = x2 * y, xz = x2 * z;
        float yy = y2 * y, yz = y2 * z, zz = z2 * z;
        float xw = x2 * w, yw = y2 * w, zw = z2 * w;
        float r[3][4] = {
            { (1.0f - zz) - yy, xy + zw, xz - yw, 0.0f },
            { xy - zw, (1.0f - zz) - xx, yz + xw, 0.0f },
            { xz + yw, yz - xw, (1.0f - xx) - yy, 0.0f },
        };

        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 4; j++)
                m[i * 4 + j] = r[i][j] * p->m_scale[i];
        for (int j = 0; j < 4; j++)
            m[12 + j] = p->m_pos[j];
    }
}

// ──────────────────────────────────────────────────────────────────────
// The cubic Hermite basis at w applied to four vectors: a1 at w = 0, a0
// at w = 1 (w is the first key's weight), a3 / a2 the tangents.
// ROR 0x0022f870

void i3mHermite(float w, float *out, const float *a0, const float *a1, const float *a2,
                const float *a3)
{
    float t[3] = { w * w * w, w * w, w };
    float h[4];

    for (int i = 0; i < 4; i++)
        h[i] = s_hermite[0][i] * t[0] + s_hermite[1][i] * t[1] + s_hermite[2][i] * t[2]
             + s_hermite[3][i];
    for (int i = 0; i < 4; i++)
        out[i] = a1[i] * h[0] + a0[i] * h[1] + a3[i] * h[2] + a2[i] * h[3];
}

// ──────────────────────────────────────────────────────────────────────
// One key's position, rotation and scale: from the float4 table, or
// unpacked from the s16x4 table (position / scale as offset + value *
// scale from the root); scale (1, 1, 1, 1) for I3M_NO_SCALE.
// ROR 0x0022f8d0

void i3mDecodeKey(I3mPose *out, const u_short *key, const I3mRoot *root)
{
    u_short flags = root->m_flags;
    const float (*vec)[4] = (const float (*)[4])i3mPtr(root, root->m_20);
    const short (*packed)[4] = (const short (*)[4])i3mPtr(root, root->m_28);

    if (flags & I3M_POS_PACKED)
        unpack4(out->m_pos, packed[key[2]], root->m_pos_scale, root->m_pos_offset);
    else
        memcpy(out->m_pos, vec[key[2]], sizeof(out->m_pos));
    if (flags & I3M_ROT_PACKED)
        unpack4(out->m_rot, packed[key[3]], NULL, NULL);
    else
        memcpy(out->m_rot, vec[key[3]], sizeof(out->m_rot));
    if (flags & I3M_NO_SCALE)
    {
        for (int i = 0; i < 4; i++)
            out->m_scale[i] = 1.0f;
    }
    else if (flags & I3M_SCALE_PACKED)
        unpack4(out->m_scale, packed[key[4]], root->m_scale_scale, root->m_scale_offset);
    else
        memcpy(out->m_scale, vec[key[4]], sizeof(out->m_scale));
}

/* ROR 0x0022f9d8: the ROM decodes both keys channel by channel. */
void i3mDecodeKeyPair(I3mPose *out, const u_short *key, const I3mRoot *root)
{
    i3mDecodeKey(&out[0], key, root);
    i3mDecodeKey(&out[1], key + root->m_key_stride, root);
}

// ──────────────────────────────────────────────────────────────────────
// The pair, then for a spline mode the key's two tangent sets: out[2]
// from u16 [8..10] and out[3] from [5..7] ([6..7] / [4..5] without scale).
// ROR 0x0022fb90

void i3mDecodeKeySpline(I3mPose *out, const u_short *key, const I3mRoot *root, u_int flags)
{
    int no_scale;

    i3mDecodeKeyPair(out, key, root);
    if ((flags & I3M_SPLINE_ALL) == 0)
        return;
    no_scale = (root->m_flags >> 1) & 1;
    i3mDecodeKey(&out[2], key + (no_scale ? 4 : 6), root);
    i3mDecodeKey(&out[3], key + (no_scale ? 2 : 3), root);
}

// ──────────────────────────────────────────────────────────────────────
// Slerp of a / b at w by the shorter arc (a negated when the dot is
// negative); past 0.9 a lerp, normalized. The slerp is not normalized.
// ROR 0x0022fc38
// callcheck: acosf, sinf (in slerp4)

/* The body of the three shorter-arc slerps (0x22fc38, 0x230688,
 * 0x2307c0): copies that differ only in how the operands travel. */
static inline void slerpShort4(float w, float *out, const float *a, const float *b)
{
    float qa[4];
    float d = dot4(a, b);

    for (int i = 0; i < 4; i++)
        qa[i] = a[i];
    if (d < 0.0f)
    {
        d = -d;
        for (int i = 0; i < 4; i++)
            qa[i] = qa[i] * -1.0f;
    }
    if (d < s_slerp_max)
        slerp4(w, d, out, qa, b);
    else
    {
        lerp4(w, out, qa, b);
        normalize4(out, out);
    }
}

void i3mQuatSlerpShort(float w, float *out, const float *a, const float *b)
{
    slerpShort4(w, out, a, b);
}

// ──────────────────────────────────────────────────────────────────────
// A key pair mixed per channel at w (1 = the key, 0 = the next one):
// mode 0 steps to the nearer key, 1 lerps (slerps the rotation), 2 runs
// the Hermite spline (squad for the rotation); other modes keep the key.
// ROR 0x0022fd58
// callcheck: acosf, sinf (the squad's two inline slerps; here i3mQuatSlerpNorm)

void i3mInterpKey(const u_short *key, const I3mRoot *root, I3mPose *out, u_int flags, float w)
{
    I3mPose k[4];               /* the key, the next, the two tangent sets */
    int near_first = 0.5f < w;
    float v[4];

    i3mDecodeKeySpline(k, key, root, flags);

    /* position */
    switch (flags & I3M_POS_MASK)
    {
    case 0x000: memcpy(v, near_first ? k[0].m_pos : k[1].m_pos, sizeof(v)); break;
    case 0x001: lerp4(w, v, k[0].m_pos, k[1].m_pos); break;
    case 0x002: i3mHermite(w, v, k[0].m_pos, k[1].m_pos, k[2].m_pos, k[3].m_pos); break;
    default:    memcpy(v, k[0].m_pos, sizeof(v)); break;
    }
    memcpy(out->m_pos, v, sizeof(v));

    /* rotation */
    switch (flags & I3M_ROT_MASK)
    {
    case 0x000: memcpy(v, near_first ? k[0].m_rot : k[1].m_rot, sizeof(v)); break;
    case 0x010: i3mQuatSlerpShort(w, v, k[0].m_rot, k[1].m_rot); break;
    case 0x020:
    {
        /* squad: slerp(slerp(q0, q1), slerp(tangents), 2w(1 - w)), each
         * step lerped outside -0.9..0.9 and always normalized (the first
         * two inline in the ROM, the outer one i3mQuatSlerpNorm) */
        float q01[4];
        i3mQuatSlerpNorm(w, q01, k[0].m_rot, k[1].m_rot);
        i3mQuatSlerpNorm(w, v, k[2].m_rot, k[3].m_rot);
        i3mQuatSlerpNorm((w + w) * (1.0f - w), v, v, q01);
        break;
    }
    default:    memcpy(v, k[0].m_rot, sizeof(v)); break;
    }
    memcpy(out->m_rot, v, sizeof(v));

    /* scale */
    if (root->m_flags & I3M_NO_SCALE)
    {
        for (int i = 0; i < 4; i++)
            v[i] = 1.0f;
    }
    else
    {
        switch (flags & I3M_SCALE_MASK)
        {
        case 0x000: memcpy(v, near_first ? k[0].m_scale : k[1].m_scale, sizeof(v)); break;
        case 0x100: lerp4(w, v, k[0].m_scale, k[1].m_scale); break;
        case 0x200: i3mHermite(w, v, k[0].m_scale, k[1].m_scale, k[2].m_scale, k[3].m_scale); break;
        default:    memcpy(v, k[0].m_scale, sizeof(v)); break;
        }
    }
    memcpy(out->m_scale, v, sizeof(v));
}

// ──────────────────────────────────────────────────────────────────────
// The key's weight at t: 1 at or before its time, falling by its inverse
// length to 0 at the next key's. At 1 the key alone is decoded; else the
// pair is interpolated, spline modes as linear ones for I3M_NO_SPLINE.
// ROR 0x00230160

int i3mSampleKey(float t, const u_short *key, I3mPose *out, u_int flags, const I3mRoot *root)
{
    const float *tbl;
    float t0, w;

    if (key == NULL || root == NULL)
        return 0;
    tbl = floatTable(root);
    t0 = tbl[key[0]];
    w = 1.0f;
    if (!(t < t0))
    {
        w = 1.0f - (t - t0) * tbl[key[1]];
        if (w < 0.0f)
            w = 0.0f;
    }
    if (w == 1.0f)
    {
        i3mDecodeKey(out, key, root);
        return 1;
    }
    if ((flags & I3M_SPLINE_ALL) != 0 && (root->m_flags & I3M_NO_SPLINE) != 0)
        flags = (flags & ~I3M_SPLINE_ALL) | ((int)(flags & I3M_SPLINE_ALL) >> 1);
    i3mInterpKey(key, root, out, flags, w);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Slerp of a / b at w, a lerp when the dot is outside -0.9..0.9 (no
// shorter-arc flip); the result always normalized.
// ROR 0x00230258
// callcheck: acosf, sinf (in slerp4)

void i3mQuatSlerpNorm(float w, float *out, const float *a, const float *b)
{
    float d = dot4(a, b);
    float q[4];

    if (d < -s_slerp_max || s_slerp_max < d)
        lerp4(w, q, a, b);
    else
        slerp4(w, d, q, a, b);
    normalize4(out, q);
}

// ──────────────────────────────────────────────────────────────────────
// Binary search of a node's keys (m_key_stride u16 each) for the one
// whose span holds t.
// ROR 0x00230380

const u_short *i3mFindKey(float t, const I3mRoot *root, const I3mNode *node)
{
    const float *tbl = floatTable(root);
    u_int stride = node->m_key_stride;
    int n = node->m_num_key;
    const u_short *key, *p;
    int step;

    if (n == 0)
        return NULL;
    key = (const u_short *)i3mPtr(root, node->m_key);
    if (key == NULL)
        return NULL;
    if (t <= tbl[key[0]])
        return key;
    p = key + (n - 1) * stride;
    if (tbl[p[0]] <= t)
        return p;
    step = (n + 3) >> 2;
    p = key + (n >> 1) * stride;
    for (;;)
    {
        if (t < tbl[p[0]])
            p -= stride * step;
        else
        {
            if (t <= tbl[p[stride]])
                return p;
            p += stride * step;
        }
        if (step != 1)
            step >>= 1;
    }
}

/* ROR 0x00230470 */
void i3mQuatMul(float *out, const float *a, const float *b)
{
    float ax = a[0], ay = a[1], az = a[2], aw = a[3];
    float bx = b[0], by = b[1], bz = b[2], bw = b[3];

    out[0] = ay * bz - az * by + aw * bx + ax * bw;
    out[1] = az * bx - ax * bz + aw * by + ay * bw;
    out[2] = ax * by - ay * bx + aw * bz + az * bw;
    out[3] = aw * bw - ax * bx - ay * by - az * bz;
}

/* ROR 0x002304e8: (axis * angle / 2, 0); 0 when the sine is not positive. */
void i3mQuatLog(float *out, const float *q)
{
    float th = acosf(q[3]);
    float s = sinf(th);

    if (0.0f < s)
    {
        float k = th / s;
        out[0] = q[0] * k;
        out[1] = q[1] * k;
        out[2] = q[2] * k;
        out[3] = 0.0f;
    }
    else
    {
        for (int i = 0; i < 4; i++)
            out[i] = 0.0f;
    }
}

/* ROR 0x00230570: the inverse of i3mQuatLog; (0, 0, 0, 1) for a zero vector. */
void i3mQuatExp(float *out, const float *v)
{
    float len = sqrtf(i3dVecDot(v, v));

    if (0.0f < len)
    {
        float k = sinf(len) / len;
        out[0] = v[0] * k;
        out[1] = v[1] * k;
        out[2] = v[2] * k;
        out[3] = cosf(len);
    }
    else
    {
        out[0] = out[1] = out[2] = 0.0f;
        out[3] = 1.0f;
    }
}

/* ROR 0x00230610 */
float i3mQuatDot(const float *a, const float *b)
{
    return dot4(a, b);
}

/* ROR 0x00230660 */
void i3mQuatNormalize(float *out, const float *in)
{
    normalize4(out, in);
}

// ──────────────────────────────────────────────────────────────────────
// 0x230688..0x231098: a quaternion kit nothing in the game calls (no
// jal, pointer or lui/addiu pair reaches it); Ghidra had it folded into
// two bodies. The ...Q forms take and return their quaternions in
// 128-bit GPRs (qmtc2 / qmfc2); on the host they take pointers like the
// memory forms.

/* ROR 0x00230688: i3mQuatSlerpShort with a and b in memory. */
// callcheck: acosf, sinf (in slerp4)
void i3mQuatSlerpShortMem(float w, float *out, const float *a, const float *b)
{
    slerpShort4(w, out, a, b);
}

/* ROR 0x002307c0 */
// callcheck: acosf, sinf (in slerp4)
void i3mQuatSlerpShortQ(float w, float *out, const float *a, const float *b)
{
    slerpShort4(w, out, a, b);
}

/* ROR 0x002308f0: lerp by the shorter arc, not normalized. */
void i3mQuatLerpShort(float w, float *out, const float *a, const float *b)
{
    float na[4];

    if (i3mQuatDot(a, b) < 0.0f)
    {
        i3dVecScale(-1.0f, na, a);
        i3dVecInterp(w, out, na, b);
    }
    else
        i3dVecInterp(w, out, a, b);
}

/* ROR 0x00230998: i3mQuatSlerpNorm through the i3dVec calls (each side
 * scaled by its sine over sin(th) before the add). */
void i3mQuatSlerpNormMem(float w, float *out, const float *a, const float *b)
{
    float d = i3mQuatDot(a, b);

    if (d < -s_slerp_max || s_slerp_max < d)
        i3dVecInterp(w, out, a, b);
    else
    {
        float sa[4], sb[4];
        float th = acosf(d);
        float inv = 1.0f / sinf(th);
        float ka = sinf(th * w) * inv;
        float kb = sinf(th * (1.0f - w)) * inv;

        i3dVecScale(ka, sa, a);
        i3dVecScale(kb, sb, b);
        i3dVecAdd(out, sa, sb);
    }
    i3mQuatNormalize(out, out);
}

/* ROR 0x00230ae8: the VU0 copy of i3mQuatSlerpNorm 0x230258. */
// callcheck: acosf, sinf (in slerp4)
void i3mQuatSlerpNormQ(float w, float *out, const float *a, const float *b)
{
    float d = dot4(a, b);
    float q[4];

    if (d < -s_slerp_max || s_slerp_max < d)
        lerp4(w, q, a, b);
    else
        slerp4(w, d, q, a, b);
    normalize4(out, q);
}

// ──────────────────────────────────────────────────────────────────────
// Squad: slerp(w, a, b) and slerp(w, c, d) mixed by a slerp at 2w(1 - w)
// (w = 1 gives a). When c and d are the same quaternion without a NaN
// the second slerp is skipped and c used as it is.
// ROR 0x00230c10

void i3mQuatSquadMem(float w, float *out, const float *a, const float *b, const float *c,
                     const float *d)
{
    float ab[4], cd[4];
    const float *p = cd;

    i3mQuatSlerpNormMem(w, ab, a, b);
    if (c == d && c[3] == c[3] && c[0] == c[0] && c[1] == c[1] && c[2] == c[2])
        p = c;
    else
        i3mQuatSlerpNormMem(w, cd, c, d);
    i3mQuatSlerpNormMem((w + w) * (1.0f - w), out, p, ab);
}

/* ROR 0x00230cf8 */
void i3mQuatSquadQ(float w, float *out, const float *a, const float *b, const float *c,
                   const float *d)
{
    float ab[4], cd[4];

    i3mQuatSlerpNormQ(w, ab, a, b);
    i3mQuatSlerpNormQ(w, cd, c, d);
    i3mQuatSlerpNormQ((w + w) * (1.0f - w), out, cd, ab);
}

// ──────────────────────────────────────────────────────────────────────
// Rotation rows of the unit quaternion q (w of each row 0), row 3 copied
// from trans. Sums in the VU0's order: 1 - 2zz - 2yy etc.
// ROR 0x00230d70

void i3mQuatToMatrix(sceVu0FVECTOR *m, const float *q, const float *trans)
{
    float x2 = q[0] + q[0], y2 = q[1] + q[1], z2 = q[2] + q[2];
    float xx = x2 * q[0], xy = x2 * q[1], xz = x2 * q[2];
    float yy = y2 * q[1], yz = y2 * q[2];
    float zz = z2 * q[2];
    float xw = x2 * q[3], yw = y2 * q[3], zw = z2 * q[3];
    float one_zz = 1.0f - zz;

    m[0][0] = one_zz - yy;
    m[0][1] = xy + zw;
    m[0][2] = xz - yw;
    m[0][3] = 0.0f;
    m[1][0] = xy - zw;
    m[1][1] = one_zz - xx;
    m[1][2] = yz + xw;
    m[1][3] = 0.0f;
    m[2][0] = xz + yw;
    m[2][1] = yz - xw;
    m[2][2] = (1.0f - xx) - yy;
    m[2][3] = 0.0f;
    for (int i = 0; i < 4; i++)
        m[3][i] = trans[i];
}

// ──────────────────────────────────────────────────────────────────────
// The rotation of m as a unit quaternion with w >= 0: from the trace
// while w^2 >= 0.01, else from the largest of x^2, y^2, z^2 (ties go
// to z, then y).
// ROR 0x00230df8

void i3mQuatFromMatrix(float *out, const sceVu0FVECTOR *m)
{
    float m00 = m[0][0], m11 = m[1][1], m22 = m[2][2], m33 = m[3][3];
    float ww = (((m00 + m11) + m22) + m33) * 0.25f;
    float xx = (((m00 - m11) - m22) + m33) * 0.25f;
    float yy = (((m11 - m00) - m22) + m33) * 0.25f;
    float zz = (((-m00 - m11) + m22) + m33) * 0.25f;
    float q[4];
    float s, k;
    int big = 0;

    if (ww < 0.01f)
    {
        if (yy < xx)
            big = zz < xx ? 1 : 3;
        else
            big = zz < yy ? 2 : 3;
    }
    switch (big)
    {
    case 0:
        s = sqrtf(ww);
        k = 0.25f / s;
        q[3] = s;
        q[2] = (m[0][1] - m[1][0]) * k;
        q[0] = (m[1][2] - m[2][1]) * k;
        q[1] = (m[2][0] - m[0][2]) * k;
        break;
    case 1:
        s = sqrtf(xx);
        k = 0.25f / s;
        q[0] = s;
        q[3] = (m[1][2] - m[2][1]) * k;
        q[1] = (m[0][1] + m[1][0]) * k;
        q[2] = (m[2][0] + m[0][2]) * k;
        break;
    case 2:
        s = sqrtf(yy);
        k = 0.25f / s;
        q[1] = s;
        q[3] = (m[2][0] - m[0][2]) * k;
        q[0] = (m[0][1] + m[1][0]) * k;
        q[2] = (m[1][2] + m[2][1]) * k;
        break;
    default:
        s = sqrtf(zz);
        k = 0.25f / s;
        q[2] = s;
        q[3] = (m[0][1] - m[1][0]) * k;
        q[0] = (m[0][2] + m[2][0]) * k;
        q[1] = (m[2][1] + m[1][2]) * k;
        break;
    }
    if (q[3] < 0.0f)
        i3dVecNeg(q, q);
    i3mQuatNormalize(out, q);
}

// ──────────────────────────────────────────────────────────────────────
// The squad's inner control point at q between prev and next:
// q * exp(-(log(q* next) + log(q* prev)) / 4), q* the conjugate.
// ROR 0x00231098

void i3mQuatSquadCtrl(float *out, const float *prev, const float *q, const float *next)
{
    float conj[4], t[4], lp[4], ln[4];

    conj[0] = q[0] * -1.0f;
    conj[1] = q[1] * -1.0f;
    conj[2] = q[2] * -1.0f;
    conj[3] = q[3];
    i3mQuatMul(t, conj, prev);
    i3mQuatLog(lp, t);
    i3mQuatMul(t, conj, next);
    i3mQuatLog(ln, t);
    i3dVecAdd(t, ln, lp);
    i3dVecScale(-0.25f, t, t);
    i3mQuatExp(t, t);
    i3mQuatMul(out, q, t);
}

// ──────────────────────────────────────────────────────────────────────
// Half angles of rot's x, y, z; the ROM's sign pattern (z and w differ
// from the textbook xyz order) kept as written.
// ROR 0x00231198

void i3mQuatFromEuler(float *out, const float *rot)
{
    float hz = rot[2] * 0.5f, hy = rot[1] * 0.5f, hx = rot[0] * 0.5f;
    float sz = sinf(hz), cz = cosf(hz);
    float sy = sinf(hy), cy = cosf(hy);
    float sx = sinf(hx), cx = cosf(hx);
    float sxcy = sx * cy, cxsy = cx * sy;
    float cxcy = cx * cy, sxsy = sx * sy;

    out[0] = sxcy * cz - cxsy * sz;
    out[1] = cxsy * cz - sxcy * sz;
    out[2] = cxcy * sz - sxsy * cz;
    out[3] = cxcy * cz - sxsy * sz;
}
