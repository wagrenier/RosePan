/* ==========================================================================
 *  i3d/i3cquery.cpp
 *
 *  The i3c collision casts (ROR 0x2312a0..0x233628): see i3cquery.h. The
 *  ROM's triangle tests are VU0 macro code; they are written out here as
 *  plain float code over float[4] vectors (xyz; w where the ROM keeps it).
 *  The i3c file keeps its offsets on the host (i3croot.h "PORT"): every
 *  tree / vertex field is resolved with i3cPtr from the query's root.
 *
 *  The sweep casts (0x2331c8 / 0x2331f0) and the hit-attribute pair
 *  (0x233258 / 0x2333b0) have no callers in the ROM.
 * ======================================================================== */
#include <float.h>
#include <math.h>
#include <string.h>

#include "i3d/i3cquery.h"
#include "i3d/i3dgeom.h"
#include "i3d/i3dmath.h"
#include "lib/unassigned.h"

/* sdata 0x2db630: the inside tests' epsilon. */
static float s_i3c_eps = 0.001f;
/* sdata 0x2db634: i3cCast's row-scale path (off in the ROM: nothing sets it). */
static u_char s_i3c_scale_enable = 0;

/* rodata 0x303b40: a tree box's s8 / u8 step. */
static const float s_i3c_step = 1.0f / 127.0f;

/* ---- vector helpers: the VU0 sequences the tests repeat ---- */

static inline void cqSub(float *out, const float *a, const float *b)
{
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

static inline float cqDot(const float *a, const float *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

/* vopmula a, b; vopmsub out, b, a: out = a x b. */
static inline void cqCross(float *out, const float *a, const float *b)
{
    float x = a[1] * b[2] - b[1] * a[2];
    float y = a[2] * b[0] - b[2] * a[0];
    float z = a[0] * b[1] - b[0] * a[1];
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

/* A leaf triangle's vertices, times the row lengths when scaled (xyz; w kept). */
static void cqLoadTri(float v[3][4], const I3cTri *prim, const float *verts, const float *scale)
{
    for (int i = 0; i < 3; i++)
    {
        memcpy(v[i], verts + prim->m_vtx[i] * 4, sizeof(v[i]));
        if (scale != NULL)
        {
            v[i][0] *= scale[0];
            v[i][1] *= scale[1];
            v[i][2] *= scale[2];
        }
    }
}

/* ──────────────────────────────────────────────────────────────────────
 * out[0].xyz = s[0] * m (a point), out[1].xyz = s[1] * m's 3x3 (a
 * direction); the w words stay s[0].w (the radius) and s[1].w.
 * ROR 0x002312a0 */

void i3cTransformShape(float *out, sceVu0FVECTOR *m, const float *s)
{
    float p[3], d[3];
    for (int k = 0; k < 3; k++)
    {
        p[k] = m[0][k] * s[0] + m[1][k] * s[1] + m[2][k] * s[2] + m[3][k];
        d[k] = m[0][k] * s[4] + m[1][k] * s[5] + m[2][k] * s[6];
    }
    out[3] = s[3];
    out[7] = s[7];
    memcpy(out, p, sizeof(p));
    memcpy(out + 4, d, sizeof(d));
}

/* ──────────────────────────────────────────────────────────────────────
 * Ray (pos, dir) against a triangle: the plane n = e0 x e1 (unnormalised),
 * t = n.(v0 - pos) / n.dir, then the hit point inside the three edges (the
 * signs of the corner cross products, within the epsilon). mode bit 0
 * keeps t >= 0, bit 1 t <= 1. The plane goes out as (n, n.dir), the point
 * as (pos + t dir, t).
 * ROR 0x002312e0 */

int i3cPrimRay(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
               int node, int both_face, int mode)
{
    float v[3][4];
    cqLoadTri(v, prim, verts, NULL);
    const float *pos = shape;
    const float *dir = shape + 4;

    float e0[3], e1[3];
    cqSub(e0, v[1], v[0]);
    cqSub(e1, v[2], v[1]);
    float plane[4];
    cqCross(plane, e0, e1);
    float nd = cqDot(plane, dir);
    plane[3] = nd;
    float inv = 1.0f / nd;   /* vdiv: Q = 1 / n.dir */

    if (!both_face && nd < 0.0f)
        return 0;
    if (nd == 0.0f)
        return 0;

    float to[3];
    cqSub(to, v[0], pos);
    float t = cqDot(plane, to) * inv;
    if ((mode & 1) && t < 0.0f)
        return 0;
    if ((mode & 2) && 1.0f < t)   /* rodata 0x303b38 */
        return 0;

    float point[4];
    point[0] = dir[0] * t + pos[0];
    point[1] = dir[1] * t + pos[1];
    point[2] = dir[2] * t + pos[2];
    point[3] = t;

    float a[3], b[3], c[3];
    cqSub(a, v[0], point);
    cqSub(b, v[1], point);
    cqSub(c, v[2], point);
    float ab[3], bc[3], ca[3];
    cqCross(ab, a, b);
    cqCross(bc, b, c);
    cqCross(ca, c, a);
    /* vmaddw.xyz vf0 = (the three dots) + eps; the status S flag: any negative. */
    if (cqDot(ab, bc) + s_i3c_eps < 0.0f || cqDot(bc, ca) + s_i3c_eps < 0.0f ||
        cqDot(ca, ab) + s_i3c_eps < 0.0f)
        return 0;

    if (q == NULL)
        return 1;
    if (q->m_func != NULL && !q->m_func(q, (node << 16) + 3, prim->m_vtx, verts, plane, point))
        return -1;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// i3cPrimRay as a line (no t limit); the ROM overwrites the scale word (t2).
// ROR 0x002314c8

int i3cPrimRayLine(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                   int node, int both_face, const float *scale)
{
    (void)scale;
    return i3cPrimRay(shape, prim, verts, q, node, both_face, 0);
}

// ──────────────────────────────────────────────────────────────────────
// i3cPrimRay as a ray (t >= 0).
// ROR 0x002314e8

int i3cPrimRayHalf(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                   int node, int both_face, const float *scale)
{
    (void)scale;
    return i3cPrimRay(shape, prim, verts, q, node, both_face, 1);
}

// ──────────────────────────────────────────────────────────────────────
// i3cPrimRay as a segment (0 <= t <= 1).
// ROR 0x00231508

int i3cPrimRaySeg(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                  int node, int both_face, const float *scale)
{
    (void)scale;
    return i3cPrimRay(shape, prim, verts, q, node, both_face, 3);
}

/* ──────────────────────────────────────────────────────────────────────
 * Sphere (centre, w = radius) against a triangle. The unit plane's signed
 * distance must lie in -r..0 (both faces: -r..r). Inside the three edges
 * the hit point is the centre projected on the plane (w = |dist| / r, code
 * 3); else the nearest vertex (code 0..2) or edge point (0x10..0x12)
 * within r (w = dist^2 / r^2). The plane goes out with w = -dist.
 * ROR 0x00231528 */

int i3cPrimSphere(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                  int node, int both_face, const float *scale)
{
    float v[3][4];
    cqLoadTri(v, prim, verts, scale);

    float e[3][3];
    cqSub(e[0], v[1], v[0]);
    cqSub(e[1], v[2], v[1]);
    cqSub(e[2], v[0], v[2]);
    float plane[4];
    cqCross(plane, e[0], e[1]);
    float len2 = cqDot(plane, plane);
    /* PC: vrsqrt of 0 is +MAX on the VU (a degenerate triangle keeps n = 0). */
    float inv = len2 > 0.0f ? 1.0f / sqrtf(len2) : FLT_MAX;

    float d[3][3];
    cqSub(d[0], shape, v[0]);
    cqSub(d[1], shape, v[1]);
    cqSub(d[2], shape, v[2]);

    plane[0] *= inv;
    plane[1] *= inv;
    plane[2] *= inv;
    float dist = cqDot(plane, shape) - cqDot(plane, v[0]);
    plane[3] = dist;

    float r = shape[3];
    if (dist < -r)
        return 0;
    if (both_face ? r < dist : 0.0f < dist)
        return 0;

    int i;
    for (i = 0; i < 3; i++)
    {
        float side[3];
        cqCross(side, e[i], plane);
        if (0.0f <= cqDot(side, d[i]))
            break;
    }

    float point[4];
    int code;
    if (i == 3)
    {
        point[0] = shape[0] - plane[0] * dist;
        point[1] = shape[1] - plane[1] * dist;
        point[2] = shape[2] - plane[2] * dist;
        point[3] = r == 0.0f ? 0.0f : fabsf(dist) / r;
        code = (node << 16) + 3;
    }
    else
    {
        int best = -1;
        float r2 = r * r;
        float best_d2 = r2;
        for (i = 0; i < 3; i++)
        {
            float pt[4];
            int kind = 0;
            float de = cqDot(d[i], e[i]);
            if (de <= 0.0f)
            {
                memcpy(pt, v[i], sizeof(pt));
            }
            else
            {
                float ee = cqDot(e[i], e[i]);
                if (ee == 0.0f)
                    continue;
                if (ee <= de)
                {
                    /* The ROM takes vertex (i + 1) % 2, not % 3: v1, v0, v1. */
                    i3dVecCopyW(pt, v[(i + 1) % 2]);
                }
                else
                {
                    float s = de / ee;
                    pt[0] = e[i][0] * s;
                    pt[1] = e[i][1] * s;
                    pt[2] = e[i][2] * s;
                    pt[3] = 0.0f;
                    kind = 0x10;
                    i3dVu0VecAdd(pt, pt, v[i]);
                }
            }
            float d2 = i3dPointDist2(shape, pt);
            if (d2 <= best_d2)
            {
                best_d2 = d2;
                best = kind + i;
                memcpy(point, pt, sizeof(point));
            }
        }
        if (best < 0)
            return 0;
        code = (node << 16) + best;
        point[3] = best_d2 / r2;
    }

    if (q->m_func != NULL)
    {
        plane[3] = -dist;
        if (!q->m_func(q, code, prim->m_vtx, verts, plane, point))
            return -1;
    }
    return 1;
}

/* ──────────────────────────────────────────────────────────────────────
 * Cylinder (axis p + s d, radius p.w) against a triangle. The axis meets
 * the plane at t = d0 / (d0 - d1) (d0, d1: p's and p + d's plane
 * distances); a segment (mode 2) on one side of the plane takes the nearer
 * end, missing when both are farther than r. That point inside the edges
 * is projected on the plane (code 3); else the first edge whose distance
 * to the axis (mode 0 i3dLineSegDist2, 1 i3dRaySegDist2, 2
 * i3dSegSegDist2) is within r gives its closest edge point (code i, or
 * 0x10 | i strictly inside the edge). The point goes out with w = t, the
 * plane with w = n.d (mode 0) or -d0.
 * ROR 0x00231948 */

int i3cPrimCylinder(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                    int node, int both_face, const float *scale, int mode)
{
    const float *pos = shape;
    const float *dir = shape + 4;
    float depth = 0.0f;

    float v[3][4];
    cqLoadTri(v, prim, verts, scale);
    float e[3][4];
    for (int k = 0; k < 4; k++)
    {
        e[0][k] = v[1][k] - v[0][k];
        e[1][k] = v[2][k] - v[1][k];
    }
    float plane[4];
    cqCross(plane, e[0], e[1]);
    plane[3] = 0.0f;
    float nv0 = cqDot(plane, v[0]);

    if (mode == 0)
    {
        depth = i3dVu0VecDot(plane, dir);
        if (!both_face && depth < 0.0f)
            return 0;
    }
    float d0 = cqDot(plane, pos) - nv0;
    if (!both_face && 0.0f < d0)
        return 0;
    if (mode != 0)
        depth = -d0;

    for (int k = 0; k < 4; k++)
        e[2][k] = v[0][k] - v[2][k];
    float end[3] = { pos[0] + dir[0], pos[1] + dir[1], pos[2] + dir[2] };
    float d1 = cqDot(plane, end) - nv0;

    float t;
    if (mode != 2)
    {
        float den = d0 - d1;
        t = den == 0.0f ? 0.0f : d0 / den;
    }
    else if (!(0.0f <= d0 * d1))
    {
        float a = fabsf(d0);
        float b = fabsf(d1);
        float sum = a + b;
        t = sum == 0.0f ? 0.0f : a / sum;
    }
    else
    {
        float lim = sqrtf(cqDot(plane, plane)) * pos[3];
        float a = fabsf(d0);
        float b = fabsf(d1);
        if (lim < a && lim < b)
            return 0;
        t = a < b ? 0.0f : 1.0f;   /* rodata 0x303b3c */
    }

    float x[4];
    for (int k = 0; k < 4; k++)
        x[k] = dir[k] * t + pos[k];

    int i;
    for (i = 0; i < 3; i++)
    {
        float side[3], to[3];
        cqCross(side, e[i], plane);
        cqSub(to, x, v[i]);
        if (!(cqDot(side, to) < 0.0f))
            break;
    }

    float point[4];
    int code;
    if (i == 3)
    {
        float to[3];
        cqSub(to, x, v[0]);
        float len2 = cqDot(plane, plane);
        float inv = len2 > 0.0f ? 1.0f / sqrtf(len2) : FLT_MAX;   /* PC: as in i3cPrimSphere */
        plane[0] *= inv;
        plane[1] *= inv;
        plane[2] *= inv;
        float h = cqDot(plane, to);
        point[0] = x[0] - plane[0] * h;
        point[1] = x[1] - plane[1] * h;
        point[2] = x[2] - plane[2] * h;
        point[3] = x[3];
        code = (node << 16) + 3;
    }
    else
    {
        float r2 = pos[3] * pos[3];
        for (i = 0; i < 3; i++)
        {
            float c[3];
            cqCross(c, e[i], dir);
            if (cqDot(c, c) < s_i3c_eps)
                continue;
            float d2 = 0.0f;
            float s = 0.0f, u = 0.0f;
            if (mode == 1)
                d2 = i3dRaySegDist2(pos, dir, v[i], e[i], &s, &u);
            else if (mode == 2)
                d2 = i3dSegSegDist2(pos, dir, v[i], e[i], &s, &u);
            else if (mode == 0)
                d2 = i3dLineSegDist2(pos, dir, v[i], e[i], &s, &u);
            if (!(d2 <= r2))
                continue;
            int kind = i;
            if (u != 0.0f && u != 1.0f)
                kind = i | 0x10;
            for (int k = 0; k < 4; k++)
                point[k] = e[i][k] * u + v[i][k];
            code = (node << 16) + kind;
            break;
        }
        if (i == 3)
            return 0;
    }

    if (q->m_func == NULL)
        return 1;
    if (scale != NULL)
        i3dVu0VecMul(point, point, scale + 4);
    point[3] = t;
    plane[3] = depth;
    if (!q->m_func(q, code, prim->m_vtx, verts, plane, point))
        return -1;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// i3cPrimCylinder along a line (mode 0).
// ROR 0x00231e70

int i3cPrimCylinderLine(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                        int node, int both_face, const float *scale)
{
    return i3cPrimCylinder(shape, prim, verts, q, node, both_face, scale, 0);
}

// ──────────────────────────────────────────────────────────────────────
// i3cPrimCylinder, mode 1.
// ROR 0x00231e90

int i3cPrimCylinderHalf(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                        int node, int both_face, const float *scale)
{
    return i3cPrimCylinder(shape, prim, verts, q, node, both_face, scale, 1);
}

// ──────────────────────────────────────────────────────────────────────
// i3cPrimCylinder along a segment (mode 2).
// ROR 0x00231eb0

int i3cPrimCylinderSeg(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                       int node, int both_face, const float *scale)
{
    return i3cPrimCylinder(shape, prim, verts, q, node, both_face, scale, 2);
}

/* ──────────────────────────────────────────────────────────────────────
 * One tree node: its box from the parent's (centre + half * ofs / 127,
 * half * size / 127) through the cast's box test, the query's box filter,
 * then the children or the leaf's triangles. The count includes this box
 * (1 + what is below); -1 as soon as anything below stopped.
 * ROR 0x00231ed0 */

int i3cWalkTree(I3cCastCtx *ctx, const I3cTree *tree, const float *verts, const float *center,
                const float *half, int both_face)
{
    float bc[4], bh[4];
    for (int k = 0; k < 3; k++)
    {
        bh[k] = half[k] * s_i3c_step * (float)tree->m_size[k];
        bc[k] = center[k] + half[k] * s_i3c_step * (float)tree->m_ofs[k];
    }
    bc[3] = 0.0f;
    bh[3] = 0.0f;

    float rel[4];
    i3dVu0VecSub(rel, ctx->m_shape, bc);
    int hits = ctx->m_box_test(rel, ctx->m_shape + 4, bh) != 0;
    if (!hits)
        return hits;

    I3cQuery *q = ctx->m_query;
    u_int info = tree->m_info;
    if (q->m_box_func != NULL && !q->m_box_func(q, tree, bc, bh, ctx->m_node))
        return -1;

    void *child = i3cPtr(q->m_root, tree->m_child);
    if ((info & I3C_TREE_LEAF) == 0)
    {
        const I3cTree *sub = (const I3cTree *)child;
        for (int i = 0; i < (int)info; i++)
        {
            int r = i3cWalkTree(ctx, &sub[i], verts, bc, bh, both_face);
            hits += r;
            if (r < 0)
                return -1;
        }
    }
    else
    {
        const I3cTri *tri = (const I3cTri *)child;
        for (int i = 0; i < (int)(info & 0x7fff); i++)
        {
            int r = ctx->m_prim_test(ctx->m_shape, &tri[i], verts, q, ctx->m_node, both_face,
                                     ctx->m_scale);
            hits += r;
            if (r < 0)
                return -1;
        }
    }
    return hits;
}

/* The row-scale path: each of m's first three rows normalised, its length
 * to scale[k] and the inverse (0 for a zero row) to scale[4 + k]. */
static void cqNormalizeRows(sceVu0FVECTOR *m, float *scale)
{
    for (int k = 0; k < 3; k++)
    {
        float len = i3dVecLength(m[k]);
        scale[k] = len;
        if (len != 0.0f)
            len = 1.0f / len;   /* rodata 0x303b44 */
        scale[4 + k] = len;
        i3dVu0VecScale(len, m[k], m[k]);
    }
}

/* ──────────────────────────────────────────────────────────────────────
 * The casts' worker. Without node matrices (m_mat or m_colli_tbl NULL) the
 * shape goes once into the world matrix's frame; else per enabled node
 * into world * its model matrix (nodes the colli table maps to 0xffff are
 * skipped). Every part of the query's group (all when m_all_group) is
 * walked from its box; q->m_part names it for the callbacks. scale: the
 * shape test wants the frame's row lengths divided out (only while
 * s_i3c_scale_enable, which the ROM never sets).
 * callcheck: i3dVecLength, i3dVu0VecScale (in cqNormalizeRows: the ROM's two inline copies)
 * ROR 0x00232120 */

int i3cCast(I3cQuery *q, const float *shape, sceVu0FMATRIX *mat, I3cBoxTest box_test,
            I3cPrimTest prim_test, int scale)
{
    I3cRoot *root = q->m_root;
    int num_node = root->m_num_node;
    int all_group = q->m_all_group != 0;
    int both_face = root->m_both_face != 0;

    alignas(16) float sc[8];
    alignas(16) float local[8];
    I3cCastCtx ctx;
    ctx.m_scale = NULL;
    ctx.m_shape = local;
    ctx.m_query = q;
    ctx.m_node = 0;
    ctx.m_box_test = box_test;
    ctx.m_prim_test = prim_test;
    int hits = 0;
    int per_node = 1;

    if (scale)
    {
        if (!s_i3c_scale_enable)
        {
            scale = 0;
        }
        else
        {
            ctx.m_scale = sc;
            sc[7] = 1.0f;
            sc[3] = 1.0f;
        }
    }

    sceVu0FMATRIX m;
    if (q->m_colli_tbl == NULL || q->m_mat == NULL)
    {
        per_node = 0;
        i3dMatCopy(m, *mat);
        if (scale)
            cqNormalizeRows(m, sc);
        i3dVu0MatInverse(m, m);
        i3cTransformShape(local, m, shape);
    }

    const I3cNode *nodes = (const I3cNode *)i3cPtr(root, root->m_node);
    for (int i = 0; i < num_node; i++)
    {
        const I3cNode *node = &nodes[i];
        if (node->m_enable == 0)
            continue;
        if (per_node)
        {
            u_short idx = q->m_colli_tbl[i];
            if (idx == 0xffff)
                continue;
            i3dVu0MatMul(m, *mat, q->m_mat[idx]);
            if (scale)
                cqNormalizeRows(m, sc);
            i3dVu0MatInverse(m, m);
            i3cTransformShape(local, m, shape);
        }

        ctx.m_node = i;
        I3cPart *parts = (I3cPart *)i3cPtr(root, node->m_part);
        int num_part = node->m_num_part;
        for (int j = 0; j < num_part; j++)
        {
            I3cPart *part = &parts[j];
            if (!all_group && part->m_group != q->m_group)
                continue;
            q->m_part = part;
            const I3cTree *tree = (const I3cTree *)i3cPtr(root, part->m_tree);
            const float *verts = (const float *)i3cPtr(root, part->m_vertex);
            if (!scale)
            {
                hits += i3cWalkTree(&ctx, tree, verts, part->m_center, part->m_half, both_face);
            }
            else
            {
                alignas(16) float c[4], h[4];
                i3dVu0VecMul(c, part->m_center, sc);
                i3dVu0VecMul(h, part->m_half, sc);
                hits += i3cWalkTree(&ctx, tree, verts, c, h, both_face);
            }
        }
    }
    return hits;
}

// ──────────────────────────────────────────────────────────────────────
// A sphere cast (row-scaled).
// ROR 0x002324b8

int i3cCastSphere(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats)
{
    return i3cCast(q, shape, mats, i3dSphereBoxTest, i3cPrimSphere, 1);
}

// ──────────────────────────────────────────────────────────────────────
// A line cast. No callers.
// ROR 0x002324e0

int i3cCastLine(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats)
{
    return i3cCast(q, shape, mats, i3dLineBoxTest, i3cPrimRayLine, 0);
}

// ──────────────────────────────────────────────────────────────────────
// A ray cast (t >= 0).
// ROR 0x00232508

int i3cCastRay(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats)
{
    return i3cCast(q, shape, mats, i3dRayBoxTest, i3cPrimRayHalf, 0);
}

// ──────────────────────────────────────────────────────────────────────
// A segment cast (pos .. pos + dir). No callers.
// ROR 0x00232530

int i3cCastSegment(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats)
{
    return i3cCast(q, shape, mats, i3dSegBoxTest, i3cPrimRaySeg, 0);
}

// ──────────────────────────────────────────────────────────────────────
// A cylinder cast along a line (row-scaled). No callers.
// ROR 0x00232558

int i3cCastCylinderLine(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats)
{
    return i3cCast(q, shape, mats, i3dCylLineBoxTest, i3cPrimCylinderLine, 1);
}

// ──────────────────────────────────────────────────────────────────────
// A cylinder cast, mode 1 (row-scaled). No callers.
// ROR 0x00232580

int i3cCastCylinderHalf(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats)
{
    return i3cCast(q, shape, mats, i3dCylHalfBoxTest, i3cPrimCylinderHalf, 1);
}

// ──────────────────────────────────────────────────────────────────────
// A cylinder cast along its segment (row-scaled).
// ROR 0x002325a8

int i3cCastCylinder(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats)
{
    return i3cCast(q, shape, mats, i3dCylSegBoxTest, i3cPrimCylinderSeg, 1);
}

/* The two sweep tests: one ROM body each (0x2325d0 / 0x232be8), identical
 * but for the same-side miss and the two distance helpers. both_face is not
 * read (a back face always misses). */
static int cqPrimSphereSweep(const float *shape, const I3cTri *prim, const float *verts,
                             I3cQuery *q, int node, const float *scale, int seg)
{
    const float *pos = shape;
    const float *dir = shape + 4;

    float v[3][4];
    cqLoadTri(v, prim, verts, scale);
    float e[3][4];
    for (int k = 0; k < 4; k++)
    {
        e[0][k] = v[1][k] - v[0][k];
        e[1][k] = v[2][k] - v[1][k];
    }
    float plane[4];
    cqCross(plane, e[0], e[1]);
    plane[3] = 0.0f;
    if (cqDot(plane, dir) < 0.0f)
        return 0;
    float len2 = cqDot(plane, plane);
    float inv = len2 > 0.0f ? 1.0f / sqrtf(len2) : FLT_MAX;   /* PC: as in i3cPrimSphere */
    plane[0] *= inv;
    plane[1] *= inv;
    plane[2] *= inv;

    float r = pos[3];
    float to[3];
    cqSub(to, pos, v[0]);
    if (0.0f < cqDot(plane, to))
        return 0;

    /* The plane moved r behind the triangle: the centre's distances to it. */
    float back[3] = { v[0][0] - plane[0] * r, v[0][1] - plane[1] * r, v[0][2] - plane[2] * r };
    float nb = cqDot(plane, back);
    for (int k = 0; k < 4; k++)
        e[2][k] = v[0][k] - v[2][k];
    float end[3] = { pos[0] + dir[0], pos[1] + dir[1], pos[2] + dir[2] };
    float d0 = cqDot(plane, pos) - nb;
    float d1 = cqDot(plane, end) - nb;

    if (seg && 0.0f <= d0 * d1)
    {
        int miss;
        if (r < d0)
            miss = r < d1;
        else if (-r <= d0)
            miss = 0;
        else
            miss = d1 < -r;
        if (miss)
            return 0;
    }

    /* PC: the EE FPU's x / 0 is +-MAX; a negative t is clamped below anyway. */
    float den = d0 - d1;
    float t = den != 0.0f ? d0 / den : (0.0f < d0 ? FLT_MAX : 0.0f);
    if (t < 0.0f)
        t = 0.0f;

    float x[4];
    for (int k = 0; k < 4; k++)
        x[k] = dir[k] * t + pos[k];

    int i;
    for (i = 0; i < 3; i++)
    {
        float side[3];
        cqCross(side, e[i], plane);
        cqSub(to, x, v[i]);
        if (0.0f < cqDot(side, to))
            break;
    }

    float point[4];
    int code;
    if (i == 3)
    {
        float a[4];
        i3dVu0VecSub(a, x, v[0]);
        i3dVu0VecScale(i3dVu0VecDot(plane, a), a, plane);
        i3dVu0VecSub(point, x, a);
        code = (node << 16) + 3;
    }
    else
    {
        /* From the edge that failed, not from edge 0. */
        float r2 = r * r;
        float u = 0.0f;
        for (; i < 3; i++)
        {
            float d2 = seg ? i3dSegSegDist2(pos, dir, v[i], e[i], NULL, &u)
                           : i3dRaySegDist2(pos, dir, v[i], e[i], NULL, &u);
            if (d2 < r2)
                break;
        }
        if (i == 3)
            return 0;

        if (u <= 0.0f)
        {
            code = (node << 16) | i;
            memcpy(point, v[i], sizeof(point));
        }
        else if (1.0f <= u)   /* rodata 0x303b48 */
        {
            int j = (i + 1) % 3;
            code = (node << 16) | j;
            memcpy(point, v[j], sizeof(point));
        }
        else
        {
            code = (node << 16) | i | 0x10;
            for (int k = 0; k < 4; k++)
                point[k] = e[i][k] * u + v[i][k];
        }

        /* The time the sphere reaches the edge: the edge as a cylinder of radius r. */
        float cyl[4] = { v[i][0], v[i][1], v[i][2], r };
        int flags = seg ? i3dSegCylinderTest(pos, dir, cyl, e[i], &t, NULL)
                        : i3dRayCylinderTest(pos, dir, cyl, e[i], &t, NULL);
        if ((flags & 1) == 0)
            t = 0.0f;
    }

    if (q->m_func == NULL)
        return 1;
    if (scale != NULL)
        i3dVu0VecMul(point, point, scale + 4);
    plane[3] = 1.0f;
    point[3] = t;
    if (!q->m_func(q, code, prim->m_vtx, verts, plane, point))
        return -1;
    return 1;
}

/* ──────────────────────────────────────────────────────────────────────
 * A sphere swept over pos .. pos + dir: a same-side sweep whose ends are
 * both farther than r from the plane misses; edges by i3dSegSegDist2.
 * callcheck: i3dSegCylinderTest, i3dSegSegDist2, i3dVecCopyW, i3dVu0VecDot, i3dVu0VecMul, i3dVu0VecScale, i3dVu0VecSub (in cqPrimSphereSweep and cqLoadTri)
 * ROR 0x002325d0 */

int i3cPrimSphereSweepSeg(const float *shape, const I3cTri *prim, const float *verts,
                          I3cQuery *q, int node, int both_face, const float *scale)
{
    (void)both_face;
    return cqPrimSphereSweep(shape, prim, verts, q, node, scale, 1);
}

/* ──────────────────────────────────────────────────────────────────────
 * A sphere swept along a ray (no same-side miss); edges by i3dRaySegDist2.
 * callcheck: i3dRayCylinderTest, i3dRaySegDist2, i3dVecCopyW, i3dVu0VecDot, i3dVu0VecMul, i3dVu0VecScale, i3dVu0VecSub (in cqPrimSphereSweep and cqLoadTri)
 * ROR 0x00232be8 */

int i3cPrimSphereSweepRay(const float *shape, const I3cTri *prim, const float *verts,
                          I3cQuery *q, int node, int both_face, const float *scale)
{
    (void)both_face;
    return cqPrimSphereSweep(shape, prim, verts, q, node, scale, 0);
}

// ──────────────────────────────────────────────────────────────────────
// A swept-sphere cast over its segment (row-scaled). No callers.
// ROR 0x002331c8

int i3cCastSphereSweepSeg(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats)
{
    return i3cCast(q, shape, mats, i3dCylSegBoxTest, i3cPrimSphereSweepSeg, 1);
}

// ──────────────────────────────────────────────────────────────────────
// A swept-sphere cast along a ray (row-scaled). No callers.
// ROR 0x002331f0

int i3cCastSphereSweepRay(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats)
{
    return i3cCast(q, shape, mats, i3dCylHalfBoxTest, i3cPrimSphereSweepRay, 1);
}

// ROR 0x00233218

void i3cSetEpsilon(float eps)
{
    s_i3c_eps = eps;
}

// ROR 0x00233228

float i3cGetEpsilon(void)
{
    return s_i3c_eps;
}

// ROR 0x00233238

void i3cSetScaleEnable(u_char on)
{
    s_i3c_scale_enable = on;
}

// ROR 0x00233248

u_char i3cGetScaleEnable(void)
{
    return s_i3c_scale_enable;
}

/* ──────────────────────────────────────────────────────────────────────
 * Each weight is the area opposite its vertex: |b x c|, |c x a|, |a x b|
 * with a, b, c the vertices relative to the point, over their sum.
 * ROR 0x00233258 */

void i3cTriBarycentric(float *out, const float *verts, const u_short *prim, const float *point)
{
    float a[4], b[4], c[4];
    i3dVecSub(a, verts + prim[0] * 4, point);
    i3dVecSub(b, verts + prim[1] * 4, point);
    i3dVecSub(c, verts + prim[2] * 4, point);
    float bc[4], ca[4], ab[4];
    i3dVecCross2(bc, b, c);
    i3dVecCross2(ca, c, a);
    i3dVecCross2(ab, a, b);
    float w[4];
    w[0] = i3dVecLength(bc);
    w[1] = i3dVecLength(ca);
    w[2] = i3dVecLength(ab);
    w[3] = 0.0f;   /* PC: the ROM scales a stack word it never wrote */
    i3dVecScale(1.0f / (w[0] + w[1] + w[2]), out, w);
}

/* ──────────────────────────────────────────────────────────────────────
 * The triangle's I3cAttrRef (the part's m_attr[prim m_attr]) names three
 * records of the table after the root; they are blended with the point's
 * barycentric weights. The table holds at most five floats a record; the
 * kind picks which of them are read.
 * ROR 0x002333b0 */

int i3cGetHitAttr(float *out, const I3cQuery *q, const u_short *prim, const float *verts,
                  const float *plane, const float *point)
{
    const I3cRoot *root = q->m_root;
    const I3cAttrRef *refs = (const I3cAttrRef *)i3cPtr(root, q->m_part->m_attr);
    if (refs == NULL)
        return 0;
    const I3cAttrRef *ref = &refs[((const I3cTri *)prim)->m_attr];
    const u_char *table = (const u_char *)(root + 1);
    const float *r0 = (const float *)(table + ref->m_idx[0] * root->m_attr_size);
    const float *r1 = (const float *)(table + ref->m_idx[1] * root->m_attr_size);
    const float *r2 = (const float *)(table + ref->m_idx[2] * root->m_attr_size);

    float w[4];
    i3cTriBarycentric(w, verts, prim, point);
    int kind = (u_char)ref->m_kind >> 4;
    switch (kind)
    {
    case 1:
        for (int k = 0; k < 2; k++)
            out[k] = r2[k] * w[2] + r1[k] * w[1] + r0[k] * w[0];
        out[2] = plane[0];
        out[3] = plane[1];
        out[4] = plane[2];
        break;
    case 2:
        out[0] = 0.0f;
        out[1] = 0.0f;
        for (int k = 0; k < 3; k++)
            out[2 + k] = r2[k] * w[2] + r1[k] * w[1] + r0[k] * w[0];
        break;
    case 3:
        for (int k = 0; k < 5; k++)
            out[k] = r2[k] * w[2] + r1[k] * w[1] + r0[k] * w[0];
        break;
    default:
        memset(out, 0, 0x14);
        break;
    }
    return kind;
}
