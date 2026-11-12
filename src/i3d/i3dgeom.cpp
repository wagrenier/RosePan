/* ==========================================================================
 *  i3d/i3dgeom.cpp
 *
 *  The i3d geometry tests (ROR 0x233628..0x236960): see i3dgeom.h. Most
 *  go through the i3dmath vector calls; i3dPointDist2, i3dSegSegDist2 and
 *  the box tests are VU0 macro code in the ROM, written out here as plain
 *  float code (xyz).
 *
 *  The closest-point family shares one shape: with w = p1 - p0,
 *  a = d0.d0, b = d0.d1, c = d1.d1, d = w.d0, e = w.d1, the lines' closest
 *  parameters are s = (c d - b e) / det, t = (b d - a e) / det with
 *  det = |a c - b b|; below 0.0001 the two count as parallel. Each variant
 *  clamps s / t to its shapes' ranges and evaluates the squared distance
 *  from the same dot products (the ROM returns its absolute value).
 *
 *  Two cylinder families: i3dLineCylinderTest and its forms take the
 *  capsule around cyl + s axis, the FlatCylinder ones (and
 *  i3dCylinderSphereTest) the flat-capped cylinder cyl..cyl + axis.
 * ======================================================================== */

#include <math.h>
#include <stddef.h>

#include "i3d/i3dgeom.h"
#include "i3d/i3dmath.h"

static float geomDot3(const float *a, const float *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00233628

float i3dPointDist2(const float *a, const float *b)
{
    float v[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return geomDot3(v, v);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00233650

float i3dPointLineDist2(const float *p, const float *a, const float *d, float *t)
{
    float w[4], dt[4];
    i3dVu0VecSub(w, p, a);
    float dd = i3dVu0VecDot(d, d);
    float s = 0.0f;
    if (0.0f < dd)
    {
        s = i3dVu0VecDot(w, d) / dd;
        i3dVu0VecScale(s, dt, d);
        i3dVu0VecSub(w, w, dt);
    }
    if (t != NULL)
        *t = s;
    return i3dVu0VecDot(w, w);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00233718

float i3dPointRayDist2(const float *p, const float *a, const float *d, float *t)
{
    float w[4], dt[4];
    i3dVu0VecSub(w, p, a);
    float s = i3dVu0VecDot(w, d);
    if (s <= 0.0f)
    {
        s = 0.0f;
    }
    else
    {
        s /= i3dVu0VecDot(d, d);
        i3dVu0VecScale(s, dt, d);
        i3dVu0VecSub(w, w, dt);
    }
    if (t != NULL)
        *t = s;
    return i3dVu0VecDot(w, w);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002337e0

float i3dPointSegDist2(const float *p, const float *a, const float *d, float *t)
{
    float w[4], dt[4];
    i3dVu0VecSub(w, p, a);
    float s = i3dVu0VecDot(w, d);
    if (s <= 0.0f)
    {
        s = 0.0f;
    }
    else
    {
        float dd = i3dVu0VecDot(d, d);
        if (dd <= s)
        {
            s = 1.0f;
            i3dVu0VecSub(w, w, d);
        }
        else
        {
            s /= dd;
            i3dVu0VecScale(s, dt, d);
            i3dVu0VecSub(w, w, dt);
        }
    }
    if (t != NULL)
        *t = s;
    return i3dVu0VecDot(w, w);
}

// ──────────────────────────────────────────────────────────────────────
// Folded into i3dPointSegDist2's body by Ghidra (no references).
// ROR 0x002338c8

float i3dLineLineDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                       float *s, float *t)
{
    float w[4];
    i3dVu0VecSub(w, p1, p0);
    float a = i3dVu0VecDot(d0, d0);
    float b = i3dVu0VecDot(d0, d1);
    float c = i3dVu0VecDot(d1, d1);
    float d = i3dVu0VecDot(w, d0);
    float ww = i3dVu0VecDot(w, w);
    float det = fabsf(a * c - b * b);
    float ss, tt, dist;
    if (0.0001f <= det)
    {
        float inv = 1.0f / det;
        float e = i3dVu0VecDot(w, d1);
        ss = (c * d - b * e) * inv;
        tt = (b * d - a * e) * inv;
        dist = ss * ((a * ss - b * tt) - (d + d)) + tt * ((c * tt - b * ss) + (e + e)) + ww;
    }
    else
    {
        ss = d / a;
        tt = 0.0f;
        dist = ww - d * ss;
    }
    if (s != NULL)
        *s = ss;
    if (t != NULL)
        *t = tt;
    return fabsf(dist);
}

// ──────────────────────────────────────────────────────────────────────
// Folded into i3dPointSegDist2's body by Ghidra (no references).
// ROR 0x00233a58

float i3dLineRayDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                      float *s, float *t)
{
    float w[4];
    i3dVu0VecSub(w, p1, p0);
    float a = i3dVu0VecDot(d0, d0);
    float b = i3dVu0VecDot(d0, d1);
    float c = i3dVu0VecDot(d1, d1);
    float d = i3dVu0VecDot(w, d0);
    float ww = i3dVu0VecDot(w, w);
    float det = fabsf(a * c - b * b);
    float ss, tt = 0.0f, dist;
    if (0.0001f <= det)
    {
        float e = i3dVu0VecDot(w, d1);
        tt = b * d - a * e;
        if (0.0f <= tt)
        {
            float inv = 1.0f / det;
            ss = (c * d - b * e) * inv;
            tt *= inv;
            dist = ss * ((a * ss - b * tt) - (d + d)) + tt * ((c * tt - b * ss) + (e + e)) + ww;
        }
        else
        {
            ss = d / a;
            tt = 0.0f;
            dist = ww - d * ss;
        }
    }
    else
    {
        ss = d / a;
        tt = 0.0f;
        dist = ww - d * ss;
    }
    if (s != NULL)
        *s = ss;
    if (t != NULL)
        *t = tt;
    return fabsf(dist);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00233c10

float i3dLineSegDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                      float *s, float *t)
{
    float w[4];
    i3dVu0VecSub(w, p1, p0);
    float a = i3dVu0VecDot(d0, d0);
    float b = i3dVu0VecDot(d0, d1);
    float c = i3dVu0VecDot(d1, d1);
    float d = i3dVu0VecDot(w, d0);
    float ww = i3dVu0VecDot(w, w);
    float det = fabsf(a * c - b * b);
    float ss, tt, dist;
    if (0.0001f <= det)
    {
        float e = i3dVu0VecDot(w, d1);
        tt = b * d - a * e;
        if (0.0f <= tt)
        {
            if (tt <= det)
            {
                ss = (c * d - b * e) * (1.0f / det);
                tt = tt * (1.0f / det);
                dist = tt * ((c * tt - b * ss) + e + e) + ss * ((a * ss - b * tt) - (d + d));
            }
            else
            {
                tt = 1.0f;
                ss = (b + d) / a;
                dist = (c - (b + d) * ss) + e + e;
            }
            dist += ww;
            goto done;
        }
    }
    ss = d / a;
    tt = 0.0f;
    dist = ww - d * ss;
done:
    if (s != NULL)
        *s = ss;
    if (t != NULL)
        *t = tt;
    return fabsf(dist);
}

// ──────────────────────────────────────────────────────────────────────
// Two rays (s, t >= 0; e is -(w.d1), as in the ROM). An end clamp tries
// the other ray's start against the ray first. Parallel rays (det below
// 0.0001) take s from d when it is positive, else t from e -- unclamped,
// as in the ROM.
// Outside every function in Ghidra (code between two bodies).
// ROR 0x00233de8

float i3dRayRayDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                     float *s, float *t)
{
    float w[4];
    i3dVu0VecSub(w, p1, p0);
    float a = i3dVu0VecDot(d0, d0);
    float b = i3dVu0VecDot(d0, d1);
    float c = i3dVu0VecDot(d1, d1);
    float d = i3dVu0VecDot(w, d0);
    float ww = i3dVu0VecDot(w, w);
    float det = fabsf(a * c - b * b);
    float e;
    float ss, tt, dist;
    if (0.0001f <= det)
    {
        e = -i3dVu0VecDot(w, d1);
        ss = c * d + b * e;
        tt = b * d + a * e;
        if (0.0f <= ss)
        {
            if (0.0f <= tt)
            {
                float inv = 1.0f / det;
                ss *= inv;
                tt *= inv;
                dist = ss * ((a * ss - b * tt) - (d + d)) + tt * ((c * tt - b * ss) - (e + e)) + ww;
                goto done;
            }
            if (d < 0.0f)
                goto starts;
            goto on_ray0;
        }
        if (0.0f <= tt)
        {
            if (e < 0.0f)
                goto starts;
            goto on_ray1;
        }
        if (0.0f < d)
            goto on_ray0;
        if (e <= 0.0f)
            goto starts;
        goto on_ray1;
    }
    if (b < 0.0f)
    {
        if (d <= 0.0f)
            goto starts;
        goto on_ray0;
    }
    if (0.0f < d)
        goto on_ray0;
    e = -i3dVu0VecDot(w, d1);

on_ray1:
    ss = 0.0f;
    tt = e / c;
    dist = ww - e * tt;
    goto done;
on_ray0:
    ss = d / a;
    tt = 0.0f;
    dist = ww - d * ss;
    goto done;
starts:
    ss = 0.0f;
    tt = 0.0f;
    dist = ww;
done:
    if (s != NULL)
        *s = ss;
    if (t != NULL)
        *t = tt;
    return fabsf(dist);
}

// ──────────────────────────────────────────────────────────────────────
// The ray p0 + s d0 (no clamp above) against the segment p1 + t d1 (here
// e is -(w.d1), as in the ROM).
// ROR 0x00234050

float i3dRaySegDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                     float *s, float *t)
{
    float w[4];
    i3dVu0VecSub(w, p1, p0);
    float a = i3dVu0VecDot(d0, d0);
    float b = i3dVu0VecDot(d0, d1);
    float c = i3dVu0VecDot(d1, d1);
    float d = i3dVu0VecDot(w, d0);
    float ww = i3dVu0VecDot(w, w);
    float det = fabsf(a * c - b * b);
    float e, ss, tt = 0.0f;
    if (0.0001f <= det)
    {
        e = -i3dVu0VecDot(w, d1);
        float sn = b * e + c * d;
        float tn = a * e + b * d;
        if (0.0f <= sn)
        {
            if (tn < 0.0f)
                goto t_zero;
            if (det < tn)
            {
                if (b <= -d)
                    goto s0_t1;
                goto s_t1;
            }
            ss = sn * (1.0f / det);
            tt = tn * (1.0f / det);
            ww += tt * ((c * tt - b * ss) - (e + e)) + ss * ((a * ss - b * tt) - (d + d));
            goto done;
        }
        if (0.0f < tn)
        {
            if (det < tn && 0.0f < b + d)
                goto s_t1;
        }
        else if (0.0f <= d)
        {
            ss = d / a;
            ww -= d * ss;
            goto done;
        }
        /* s = 0: the ray's start against the segment. */
        if (e <= 0.0f)
        {
            ss = 0.0f;
            tt = 0.0f;
            goto done;
        }
        if (c <= e)
            goto s0_t1;
        ss = 0.0f;
        tt = e / c;
        ww -= e * tt;
        goto done;
    }
    if (0.0f <= b)
    {
        e = -i3dVu0VecDot(w, d1);
        if (0.0f < b + d)
            goto s_t1;
        goto s0_t1;
    }
t_zero:
    tt = 0.0f;
    if (d <= 0.0f)
    {
        ss = 0.0f;
        goto done;
    }
    ss = d / a;
    ww -= d * ss;
    goto done;
s_t1:
    /* t = 1: the segment's end against the ray. */
    ss = (b + d) / a;
    tt = 1.0f;
    ww += (c - (b + d) * ss) - (e + e);
    goto done;
s0_t1:
    tt = 1.0f;
    ss = 0.0f;
    ww += c - (e + e);
done:
    if (s != NULL)
        *s = ss;
    if (t != NULL)
        *t = tt;
    return fabsf(ww);
}

// ──────────────────────────────────────────────────────────────────────
// Two segments. The ROM sorts (s, t) into nine regions (s below 0 / in
// 0..1 / past 1, the same for t) and walks a small state machine, an edge
// region falling to its neighbour; each region below is one case of it.
// e is -(w.d1).
// ROR 0x00234390

float i3dSegSegDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                     float *s, float *t)
{
    float w[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
    float a = geomDot3(d0, d0);
    float b = geomDot3(d0, d1);
    float c = geomDot3(d1, d1);
    float d = geomDot3(w, d0);
    float ww = geomDot3(w, w);
    float det = fabsf(a * c - b * b);
    float ss = 0.0f, tt = 0.0f, dist = 0.0f;

    if (0.0001f <= det)
    {
        float e = -geomDot3(w, d1);
        float sn = b * e + c * d;
        float tn = b * d + a * e;
        int rgn;
        if (0.0f <= sn)
        {
            if (sn <= det)
                rgn = tn < 0.0f ? 7 : (det < tn ? 3 : 0);
            else
                rgn = tn < 0.0f ? 8 : (det < tn ? 2 : 1);
        }
        else
        {
            rgn = tn < 0.0f ? 6 : (det < tn ? 4 : 5);
        }

        float bd = b + d;
        for (;;)
        {
            switch (rgn)
            {
            case 0: /* inside */
                ss = sn * (1.0f / det);
                tt = tn * (1.0f / det);
                dist = tt * ((c * tt - b * ss) - (e + e)) + ss * ((a * ss - b * tt) - (d + d)) + ww;
                goto done;
            case 1: /* s = 1 */
            {
                float be = b + e;
                ss = 1.0f;
                if (0.0f < be)
                {
                    if (c <= be)
                        goto s1_t1;
                    tt = be / c;
                    dist = a - be * tt - (d + d) + ww;
                    goto done;
                }
                goto s1_t0;
            }
            case 2: /* s past 1, t past 1 */
                if (a <= bd)
                {
                    rgn = 1;
                    continue;
                }
                if (0.0f < bd)
                    goto s_t1;
                goto s0_t1;
            case 3: /* t past 1 */
                if (bd <= 0.0f)
                    goto s0_t1;
                if (bd < a)
                    goto s_t1;
                goto s1_t1;
            case 4: /* s below 0, t past 1 */
                if (bd < 0.0f)
                {
                    rgn = 5;
                    continue;
                }
                if (a <= bd)
                    goto s1_t1;
                goto s_t1;
            case 5: /* s = 0 */
                if (0.0f < e)
                {
                    if (e < c)
                    {
                        ss = 0.0f;
                        tt = e / c;
                        dist = ww - e * tt;
                        goto done;
                    }
                    goto s0_t1;
                }
                goto s0_t0;
            case 6: /* s below 0, t below 0 */
                if (0.0f <= d)
                {
                    if (a <= d)
                        goto s1_t0;
                    goto s_t0;
                }
                rgn = 5;
                continue;
            case 7: /* t below 0 */
                if (d <= 0.0f)
                    goto s0_t0;
                if (d < a)
                    goto s_t0;
                goto s1_t0;
            case 8: /* s past 1, t below 0 */
                if (d < a)
                {
                    if (0.0f < d)
                        goto s_t0;
                    goto s0_t0;
                }
                rgn = 1;
                continue;
            }
        }
    s1_t1:
    {
        float f = bd + e;
        ss = 1.0f;
        tt = 1.0f;
        dist = (a + c + ww) - (f + f);
        goto done;
    }
    s_t1:
        tt = 1.0f;
        ss = bd / a;
        dist = c - bd * ss - (e + e) + ww;
        goto done;
    s0_t1:
        tt = 1.0f;
        ss = 0.0f;
        dist = c - (e + e) + ww;
        goto done;
    s_t0:
        tt = 0.0f;
        ss = d / a;
        dist = ww - d * ss;
        goto done;
    s1_t0:
        ss = 1.0f;
        tt = 0.0f;
        dist = a - (d + d) + ww;
        goto done;
    s0_t0:
        ss = 0.0f;
        tt = 0.0f;
        dist = ww;
        goto done;
    }

    /* Parallel: by the direction of d1 along d0. */
    if (b <= 0.0f)
    {
        if (d < 0.0f)
        {
            ss = 0.0f;
            dist = ww;
        }
        else if (a < d)
        {
            float e = geomDot3(w, d1);
            ss = 1.0f;
            if (a - d <= b)
            {
                float f = b + d - e;
                dist = (a + c + ww) - (f + f);
                tt = 1.0f;
            }
            else
            {
                tt = (a - d) / b;
                float f = b - e;
                dist = tt * (c * tt - (f + f)) + (a - (d + d)) + ww;
            }
        }
        else
        {
            ss = d / a;
            dist = ww - d * ss;
        }
    }
    else if (a <= d)
    {
        ss = 1.0f;
        dist = a - (d + d) + ww;
    }
    else if (0.0f <= d)
    {
        ss = d / a;
        dist = ww - d * ss;
    }
    else
    {
        float e = -geomDot3(w, d1);
        ss = 0.0f;
        if (-d < b)
        {
            tt = -d / b;
            dist = ww + tt * (c * tt - (e + e));
        }
        else
        {
            tt = 1.0f;
            dist = c - (e + e) + ww;
        }
    }
done:
    if (s != NULL)
        *s = ss;
    if (t != NULL)
        *t = tt;
    return fabsf(dist);
}

// ──────────────────────────────────────────────────────────────────────
// The squared distance from the sphere's centre to the box (the axes
// where it lies outside), against r^2; dir is not read.
// ROR 0x002348f8

int i3dSphereBoxTest(const float *rel, const float *dir, const float *half)
{
    (void)dir;
    float arel[4] = {fabsf(rel[0]), fabsf(rel[1]), fabsf(rel[2]), fabsf(rel[3])};
    float q[4];
    i3dVu0VecSub(q, arel, half);
    float r = arel[3];
    float sum = 0.0f;
    int out = 0, last = 0;
    for (int i = 0; i < 3; i++)
    {
        if (0.0f < q[i])
        {
            sum += q[i] * q[i];
            out++;
            last = i;
        }
    }
    if (out == 0)
        return 1;
    if (out == 1)
        return q[last] <= r; /* one axis out: the plain distance */
    return sum <= r * r;
}

/* The separating-axis tail the line box tests share: |m x h| against the
 * box extents ext projected across |h|, per axis (the ROM's VU0 sign flag
 * after the last subtract: 0 when any axis separates, else -2). */
static int geomSatCross(const float *m, const float *h, const float *ext)
{
    float ah[3] = {fabsf(h[0]), fabsf(h[1]), fabsf(h[2])};
    float cr[3] = {fabsf(h[1] * m[2] - h[2] * m[1]),
                   fabsf(h[2] * m[0] - h[0] * m[2]),
                   fabsf(h[0] * m[1] - h[1] * m[0])};
    float pr[3] = {ext[1] * ah[2] + ah[1] * ext[2],
                   ext[2] * ah[0] + ah[2] * ext[0],
                   ext[0] * ah[1] + ah[0] * ext[1]};
    for (int i = 0; i < 3; i++)
        if (pr[i] - cr[i] < 0.0f)
            return 0;
    return -2;
}

// ──────────────────────────────────────────────────────────────────────
// The segment's half vector h = dir / 2 and centre c = rel + h: the box
// axes first (|c| <= half + |h|), then the cross axes.
// ROR 0x00234a50

int i3dSegBoxTest(const float *rel, const float *dir, const float *half)
{
    float h[3], c[3];
    for (int i = 0; i < 3; i++)
    {
        h[i] = dir[i] * 0.5f;
        c[i] = rel[i] + h[i];
    }
    for (int i = 0; i < 3; i++)
        if ((half[i] + fabsf(h[i])) - fabsf(c[i]) < 0.0f)
            return 0;
    return geomSatCross(c, h, half);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00234ae8

int i3dLineBoxTest(const float *rel, const float *dir, const float *half)
{
    return geomSatCross(rel, dir, half);
}

// ──────────────────────────────────────────────────────────────────────
// An axis where the start lies outside the box and the ray points away
// separates; else the line test.
// ROR 0x00234b38

int i3dRayBoxTest(const float *rel, const float *dir, const float *half)
{
    for (int i = 0; i < 3; i++)
        if (half[i] < fabsf(rel[i]) && 0.0f <= rel[i] * dir[i])
            return 0;
    return i3dLineBoxTest(rel, dir, half);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00234be8

int i3dCylSegBoxTest(const float *rel, const float *dir, const float *half)
{
    float grown[4];
    for (int i = 0; i < 3; i++)
        grown[i] = half[i] + rel[3];
    grown[3] = 0.0f; /* not read */
    return i3dSegBoxTest(rel, dir, grown);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00234c30

int i3dCylHalfBoxTest(const float *rel, const float *dir, const float *half)
{
    float r = rel[3];
    for (int i = 0; i < 3; i++)
        if (r < fabsf(rel[i]) - half[i] && 0.0f <= rel[i] * dir[i])
            return 0;
    return i3dCylLineBoxTest(rel, dir, half);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00234ce8

int i3dCylLineBoxTest(const float *rel, const float *dir, const float *half)
{
    float grown[3] = {half[0] + rel[3], half[1] + rel[3], half[2] + rel[3]};
    return geomSatCross(rel, dir, grown);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00234d40

int i3dSphereSphereTest(const float *a, const float *b)
{
    float r = a[3] + b[3];
    return i3dPointDist2(a, b) <= r * r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00234d80

int i3dSphereLineTest(const float *sph, const float *p, const float *d)
{
    float r = sph[3];
    return i3dPointLineDist2(sph, p, d, NULL) <= r * r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00234dc0

int i3dSphereRayTest(const float *sph, const float *p, const float *d)
{
    float r = sph[3];
    return i3dPointRayDist2(sph, p, d, NULL) <= r * r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00234e00

int i3dSphereSegTest(const float *sph, const float *p, const float *d)
{
    float r = sph[3];
    return i3dPointSegDist2(sph, p, d, NULL) <= r * r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00234e40

int i3dSphereCylLineTest(const float *sph, const float *p, const float *d)
{
    float r = sph[3] + p[3];
    return i3dPointLineDist2(sph, p, d, NULL) <= r * r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00234e88

int i3dSphereCylHalfTest(const float *sph, const float *p, const float *d)
{
    float r = sph[3] + p[3];
    return i3dPointRayDist2(sph, p, d, NULL) <= r * r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00234ed0

int i3dSphereCylSegTest(const float *sph, const float *p, const float *d)
{
    float r = sph[3] + p[3];
    return i3dPointSegDist2(sph, p, d, NULL) <= r * r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00234f18

int i3dCylSegLineTest(const float *p0, const float *d0, const float *p1, const float *d1)
{
    float r = p0[3];
    return i3dLineSegDist2(p1, d1, p0, d0, NULL, NULL) <= r * r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00234f70

int i3dCylSegRayTest(const float *p0, const float *d0, const float *p1, const float *d1)
{
    float r = p0[3];
    return i3dRaySegDist2(p1, d1, p0, d0, NULL, NULL) <= r * r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00234fc8

int i3dCylSegSegTest(const float *p0, const float *d0, const float *p1, const float *d1)
{
    float r = p0[3];
    return i3dSegSegDist2(p1, d1, p0, d0, NULL, NULL) <= r * r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00235020

int i3dCylSegCylLineTest(const float *p0, const float *d0, const float *p1, const float *d1)
{
    float r = p0[3] + p1[3];
    return i3dLineSegDist2(p1, d1, p0, d0, NULL, NULL) <= r * r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00235080

int i3dCylSegCylHalfTest(const float *p0, const float *d0, const float *p1, const float *d1)
{
    float r = p0[3] + p1[3];
    return i3dRaySegDist2(p1, d1, p0, d0, NULL, NULL) <= r * r;
}

// ──────────────────────────────────────────────────────────────────────
// The only pair passed in its own order.
// ROR 0x002350e0

int i3dCylSegCylSegTest(const float *p0, const float *d0, const float *p1, const float *d1)
{
    float r = p0[3] + p1[3];
    return i3dSegSegDist2(p0, d0, p1, d1, NULL, NULL) <= r * r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00235128

int i3dLineSphereTest(const float *p, const float *d, const float *sph, float *t0, float *t1)
{
    float w[4];
    float r2 = sph[3] * sph[3];
    i3dVu0VecSub(w, sph, p);
    float a = i3dVu0VecDot(d, d);
    float b = i3dVu0VecDot(w, d);
    float c = i3dVu0VecDot(w, w) - r2;
    float disc = b * b - a * c;
    if (disc < 0.0f)
        return 0;
    float sq = sqrtf(disc);
    float inv = 1.0f / a;
    if (t0 != NULL)
        *t0 = (b - sq) * inv;
    if (t1 != NULL)
        *t1 = (b + sq) * inv;
    return 0x111;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00235228

int i3dRaySphereTest(const float *p, const float *d, const float *sph, float *t0, float *t1)
{
    float t[2];
    int hit = i3dLineSphereTest(p, d, sph, &t[0], &t[1]);
    if (hit == 0)
        return 0;
    if (t0 != NULL)
        *t0 = t[0];
    if (t1 != NULL)
        *t1 = t[1];
    if (t[0] < 0.0f)
    {
        if (t[1] < 0.0f)
            return 0;
        hit -= 1;
    }
    return hit;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002352a8

int i3dSegSphereTest(const float *p, const float *d, const float *sph, float *t0, float *t1)
{
    float t[2];
    int hit = i3dLineSphereTest(p, d, sph, &t[0], &t[1]);
    if (hit == 0)
        return 0;
    if (t0 != NULL)
        *t0 = t[0];
    if (t1 != NULL)
        *t1 = t[1];
    if (t[1] < 0.0f || 1.0f < t[0])
        return 0;
    if (t[0] < 0.0f)
        hit -= 1;
    if (1.0f < t[1])
        hit -= 0x10;
    return hit;
}

// ──────────────────────────────────────────────────────────────────────
// Folded into i3dSegSphereTest's body by Ghidra (no references).
// ROR 0x00235350

int i3dCylSegSphereTest(const float *p, const float *d, const float *sph, float *t0, float *t1)
{
    float grown[4];
    i3dVecCopyW(grown, sph);
    grown[3] += p[3];
    return i3dSegSphereTest(p, d, grown, t0, t1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002353c8

int i3dCylLineSphereTest(const float *p, const float *d, const float *sph, float *t0, float *t1)
{
    float grown[4];
    i3dVecCopyW(grown, sph);
    grown[3] += p[3];
    return i3dLineSphereTest(p, d, grown, t0, t1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00235440

int i3dCylHalfSphereTest(const float *p, const float *d, const float *sph, float *t0, float *t1)
{
    float grown[4];
    i3dVecCopyW(grown, sph);
    grown[3] += p[3];
    return i3dRaySphereTest(p, d, grown, t0, t1);
}

// ──────────────────────────────────────────────────────────────────────
// The capsule distance first, then the sphere's centre must lie within
// its radius of both cap planes (the planes through p and p + d across
// the unit axis).
// ROR 0x002354b8

int i3dCylinderSphereTest(const float *p, const float *d, const float *sph)
{
    float end[4], n[4], w[4];
    float r = p[3] + sph[3];
    if (r * r < i3dPointSegDist2(sph, p, d, NULL))
        return 0;
    i3dVu0VecAdd(end, p, d);
    i3dVecNormalize(n, d);
    i3dVu0VecSub(w, p, sph);
    if (sph[3] < i3dVu0VecDot(n, w))
        return 0;
    i3dVu0VecSub(w, sph, end);
    if (sph[3] < i3dVu0VecDot(n, w))
        return 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002355c8

int i3dFlatCylinderLineTest(const float *cyl, const float *axis, const float *p, const float *d)
{
    return i3dLineFlatCylinderTest(p, d, cyl, axis, NULL, NULL) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00235608

int i3dFlatCylinderRayTest(const float *cyl, const float *axis, const float *p, const float *d)
{
    return i3dRayFlatCylinderTest(p, d, cyl, axis, NULL, NULL) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00235648

int i3dFlatCylinderSegTest(const float *cyl, const float *axis, const float *p, const float *d)
{
    return i3dSegFlatCylinderTest(p, d, cyl, axis, NULL, NULL) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// u takes the plane of n's larger of x / y with z, normalised; v = n x u.
// ROR 0x00235688

void i3dVecOrthoBasis(float *u, float *v, const float *n)
{
    float x = n[0];
    float y = n[1];
    if (fabsf(y) <= fabsf(x))
    {
        float z = n[2];
        u[1] = 0.0f;
        float inv = 1.0f / sqrtf(z * z + x * x);
        u[0] = -z * inv;
        u[2] = n[0] * inv;
    }
    else
    {
        float z = n[2];
        u[0] = 0.0f;
        float inv = 1.0f / sqrtf(z * z + y * y);
        u[1] = n[2] * inv;
        u[2] = -n[1] * inv;
    }
    i3dVecCross(v, n, u);
}

// ──────────────────────────────────────────────────────────────────────
// The line in the cylinder's frame (u, v across the axis, n along it, the
// direction normalised so the roots come out as the line's t, centre at
// the origin, caps at n = +-h/2): along the axis the two cap planes, across
// it the side wall where the line lies between the caps; otherwise the cap
// planes' crossings that fall within the radius, then the side wall's roots
// between the two crossings, until two roots are found. Fewer than two is
// a miss.
// ROR 0x00235768

int i3dLineFlatCylinderCoreTest(float h, float r, const float *p, const float *d,
                                const float *centre, const float *axis, float *t0, float *t1)
{
    float u[4], v[4], n[4], w[4];
    float dl[4], e[3];
    float t[2];
    int found = 0;

    float r2 = r * r;
    i3dVecNormalize(n, axis);
    i3dVecOrthoBasis(u, v, n);
    dl[0] = i3dVu0VecDot(u, d);
    dl[1] = i3dVu0VecDot(v, d);
    dl[2] = i3dVu0VecDot(n, d);
    dl[3] = 0.0f;
    float inv = 1.0f / sqrtf(i3dVu0VecDot(dl, dl));
    i3dVu0VecScale(inv, dl, dl);
    i3dVu0VecSub(w, p, centre);
    e[0] = i3dVu0VecDot(u, w);
    e[1] = i3dVu0VecDot(v, w);
    e[2] = i3dVu0VecDot(n, w);
    float hh = h * 0.5f;

    if (1.0f - fabsf(dl[2]) <= 1e-07f)
    {
        /* Along the axis: inside the radius, the two cap planes. */
        if (r2 < e[1] * e[1] + e[0] * e[0])
            return 0;
        float k = -inv / dl[2];
        t[1] = (e[2] + hh) * k;
        t[0] = (e[2] - hh) * k;
        found = 2;
    }
    else if (fabsf(dl[2]) <= 1e-07f)
    {
        /* Across the axis: between the caps, the side wall. */
        if (hh < fabsf(e[2]))
            return 0;
        float A = dl[1] * dl[1] + dl[0] * dl[0];
        float B = e[1] * dl[1] + e[0] * dl[0];
        float disc = B * B - A * ((e[1] * e[1] + e[0] * e[0]) - r2);
        if (disc < 0.0f)
            return 0;
        float sq = sqrtf(disc);
        float k = -inv / A;
        t[1] = (B - sq) * k;
        t[0] = (B + sq) * k;
        found = 2;
    }
    else
    {
        float id = 1.0f / dl[2];
        float ta = (hh - e[2]) * id;  /* the cap at +h/2 */
        float tb = (-hh - e[2]) * id; /* the cap at -h/2 */
        float x = e[0] + ta * dl[0];
        float y = e[1] + ta * dl[1];
        if (y * y + x * x <= r2)
            t[found++] = ta * inv;
        x = e[0] + tb * dl[0];
        y = e[1] + tb * dl[1];
        if (y * y + x * x <= r2)
            t[found++] = tb * inv;
        if (found < 2)
        {
            float A = dl[1] * dl[1] + dl[0] * dl[0];
            float B = e[1] * dl[1] + e[0] * dl[0];
            float disc = B * B - A * ((e[1] * e[1] + e[0] * e[0]) - r2);
            if (disc < 0.0f || A == 0.0f)
                return 0;
            float sq = sqrtf(disc);
            float ia = 1.0f / A;
            float lo = ta < tb ? ta : tb;
            float hi = ta < tb ? tb : ta;
            float tw = (-B - sq) * ia;
            if (lo <= tw && tw <= hi)
                t[found++] = tw * inv;
            if (found < 2)
            {
                tw = (sq - B) * ia;
                if (lo <= tw && tw <= hi)
                    t[found++] = tw * inv;
            }
        }
    }

    if (found != 2)
        return 0;
    if (t[0] < t[1])
    {
        if (t0 != NULL)
            *t0 = t[0];
        if (t1 != NULL)
            *t1 = t[1];
    }
    else
    {
        if (t0 != NULL)
            *t0 = t[1];
        if (t1 != NULL)
            *t1 = t[0];
    }
    return 0x111;
}

// ──────────────────────────────────────────────────────────────────────
// The line in the capsule's frame (u, v across the axis, n along it, the
// direction normalised so the roots come out as the line's t): the side
// wall where its root lands within 0..|axis| along n, then the two end
// spheres for the roots beyond each end; the first two roots found are the
// entry / exit, sorted.
//
// PORT: the ROM leaves two cases undefined. A line along the axis (or of
// zero length) computes its t into locals it never stores, returning
// 0x111 / 0 with t0 / t1 unwritten; and a line meeting the infinite wall
// but missing the capsule finds fewer than two roots and returns 0x111
// with the root slots uninitialised. Here the first case still leaves the
// outputs alone (the callers start them at 0), the second returns 0 for no
// root and repeats a single (tangent) root.
// ROR 0x00235be8

int i3dLineCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                        float *t0, float *t1)
{
    float u[4], v[4], n[4], w[4];
    float dl[4], e[3];
    float t[2] = {0.0f, 0.0f};
    int found = 0;

    i3dVecNormalize(n, axis);
    i3dVecOrthoBasis(u, v, n);
    dl[0] = i3dVu0VecDot(u, d);
    dl[1] = i3dVu0VecDot(v, d);
    dl[2] = i3dVu0VecDot(n, d);
    dl[3] = 0.0f;
    float len = sqrtf(i3dVu0VecDot(dl, dl));
    float inv = 1.0f / len;
    i3dVu0VecScale(inv, dl, dl);
    i3dVu0VecSub(w, p, cyl);
    e[0] = i3dVu0VecDot(u, w);
    e[1] = i3dVu0VecDot(v, w);
    e[2] = i3dVu0VecDot(n, w);
    float r2 = cyl[3] * cyl[3];
    float h = sqrtf(i3dVu0VecDot(axis, axis));

    if (0.9999999f <= fabsf(dl[2]) || len < 1e-07f)
    {
        /* Along the axis (or no length): a hit when it runs inside the
         * radius and d has some component along the axis. */
        float along = i3dVu0VecDot(d, axis);
        float in = (r2 - e[0] * e[0]) - e[1] * e[1];
        if (along < 0.0f && 0.0f <= in)
            return 0x111;
        if (!(0.0f < along) || !(0.0f <= in))
            return 0;
        return 0x111;
    }

    float A = dl[1] * dl[1] + dl[0] * dl[0];
    float B = e[1] * dl[1] + e[0] * dl[0];
    float C = (e[1] * e[1] + e[0] * e[0]) - r2;
    float disc = B * B - A * C;
    if (disc < 0.0f)
        return 0;
    if (0.0f < disc)
    {
        float sq = sqrtf(disc);
        float ia = 1.0f / A;
        float ta = (-B - sq) * ia;
        float z = e[2] + ta * dl[2];
        if (0.0f <= z && z <= h)
            t[found++] = ta * inv;
        ta = (sq - B) * ia;
        z = e[2] + ta * dl[2];
        if (0.0f <= z && z <= h)
            t[found++] = ta * inv;
    }
    else
    {
        float ta = -B / A;
        float z = e[2] + ta * dl[2];
        if (0.0f <= z && z <= h)
        {
            if (t0 != NULL)
                *t0 = ta * inv;
            if (t1 != NULL)
                *t1 = ta * inv;
            return 0x111;
        }
    }

    if (found != 2)
    {
        /* The sphere at the start: |e + t dl|^2 = r^2, roots below 0. */
        B += e[2] * dl[2];
        C += e[2] * e[2];
        disc = B * B - C;
        if (0.0f < disc)
        {
            float sq = sqrtf(disc);
            float ta = -B - sq;
            if (e[2] + ta * dl[2] <= 0.0f)
                t[found++] = ta * inv;
            if (found < 2)
            {
                ta = sq - B;
                if (e[2] + ta * dl[2] <= 0.0f)
                    t[found++] = ta * inv;
            }
        }
        else if (disc == 0.0f)
        {
            float ta = -B;
            if (e[2] + ta * dl[2] <= 0.0f)
                t[found++] = ta * inv;
        }
    }
    if (found != 2)
    {
        /* The sphere at the end, roots past h. */
        B -= dl[2] * h;
        C += h * (h - (e[2] + e[2]));
        disc = B * B - C;
        if (0.0f < disc)
        {
            float sq = sqrtf(disc);
            float ta = -B - sq;
            if (h <= e[2] + ta * dl[2])
                t[found++] = ta * inv;
            if (found < 2)
            {
                ta = sq - B;
                if (h <= e[2] + ta * dl[2])
                    t[found++] = ta * inv;
            }
        }
        else if (disc == 0.0f)
        {
            float ta = -B;
            if (h <= e[2] + ta * dl[2])
                t[found++] = ta * inv;
        }
    }

    if (found == 0)
        return 0; /* PORT: see above */
    if (found == 1)
        t[1] = t[0];
    if (t[0] < t[1])
    {
        if (t0 != NULL)
            *t0 = t[0];
        if (t1 != NULL)
            *t1 = t[1];
    }
    else
    {
        if (t0 != NULL)
            *t0 = t[1];
        if (t1 != NULL)
            *t1 = t[0];
    }
    return 0x111;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00236168

int i3dLineFlatCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                            float *t0, float *t1)
{
    float centre[4];
    i3dVu0VecScale(0.5f, centre, axis);
    i3dVu0VecAdd(centre, centre, cyl);
    float h = sqrtf(i3dVu0VecDot(axis, axis));
    return i3dLineFlatCylinderCoreTest(h, cyl[3], p, d, centre, axis, t0, t1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00236228

int i3dRayFlatCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                           float *t0, float *t1)
{
    float t[2];
    int hit = i3dLineFlatCylinderTest(p, d, cyl, axis, &t[0], &t[1]);
    if (hit == 0 || t[1] < 0.0f)
        return 0;
    if (t[0] < 0.0f)
        hit ^= 1;
    if (t0 != NULL)
        *t0 = t[0];
    if (t1 != NULL)
        *t1 = t[1];
    return hit;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002362a8

int i3dSegFlatCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                           float *t0, float *t1)
{
    float t[2];
    int hit = i3dLineFlatCylinderTest(p, d, cyl, axis, &t[0], &t[1]);
    if (hit == 0 || 1.0f < t[0] || t[1] < 0.0f)
        return 0;
    if (t[0] < 0.0f)
        hit ^= 1;
    if (1.0f < t[1])
        hit ^= 0x10;
    if (t0 != NULL)
        *t0 = t[0];
    if (t1 != NULL)
        *t1 = t[1];
    return hit;
}

// ──────────────────────────────────────────────────────────────────────
// The infinite capsule around the line p + t d (radius p.w): the line
// against the cylinder grown by the radius (its height by twice it).
// Folded into i3dSegFlatCylinderTest's body by Ghidra (no references).
// ROR 0x00236348

int i3dFlatCylinderCylLineTest(const float *cyl, const float *axis, const float *p, const float *d)
{
    float centre[4];
    i3dVu0VecScale(0.5f, centre, axis);
    i3dVu0VecAdd(centre, centre, cyl);
    float h = sqrtf(i3dVu0VecDot(axis, axis));
    return i3dLineFlatCylinderCoreTest(h + (p[3] + p[3]), cyl[3] + p[3], p, d, centre, axis,
                                       NULL, NULL) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// The ray capsule p + t d (radius p.w): its start sphere inside the
// cylinder, else the line against the cylinder grown by the radius (its
// height by twice it), touching from t 0 on.
// ROR 0x00236400

int i3dFlatCylinderCylHalfTest(const float *cyl, const float *axis, const float *p, const float *d)
{
    float centre[4];
    float t[2];
    if (i3dCylinderSphereTest(cyl, axis, p) != 0)
        return 1;
    i3dVu0VecScale(0.5f, centre, axis);
    i3dVu0VecAdd(centre, centre, cyl);
    float h = sqrtf(i3dVu0VecDot(axis, axis));
    if (i3dLineFlatCylinderCoreTest(h + (p[3] + p[3]), cyl[3] + p[3], p, d, centre, axis,
                                    &t[0], &t[1]) == 0
        || t[1] < 0.0f)
        return 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The same for the capsule segment (t 0..1).
// ROR 0x002364f0

int i3dFlatCylinderCylSegTest(const float *cyl, const float *axis, const float *p, const float *d)
{
    float centre[4];
    float t[2];
    if (i3dCylinderSphereTest(cyl, axis, p) != 0)
        return 1;
    i3dVu0VecScale(0.5f, centre, axis);
    i3dVu0VecAdd(centre, centre, cyl);
    float h = sqrtf(i3dVu0VecDot(axis, axis));
    if (i3dLineFlatCylinderCoreTest(h + (p[3] + p[3]), cyl[3] + p[3], p, d, centre, axis,
                                    &t[0], &t[1]) == 0
        || 1.0f < t[0] || t[1] < 0.0f)
        return 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002365f8

int i3dRayCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                       float *t0, float *t1)
{
    float t[2] = {0.0f, 0.0f}; /* PORT: see i3dLineCylinderTest */
    int hit = i3dLineCylinderTest(p, d, cyl, axis, &t[0], &t[1]);
    if (hit == 0 || t[1] < 0.0f)
        return 0;
    if (t[0] < 0.0f)
        hit ^= 1;
    if (t0 != NULL)
        *t0 = t[0];
    if (t1 != NULL)
        *t1 = t[1];
    return hit;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00236678

int i3dSegCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                       float *t0, float *t1)
{
    float t[2] = {0.0f, 0.0f}; /* PORT: see i3dLineCylinderTest */
    int hit = i3dLineCylinderTest(p, d, cyl, axis, &t[0], &t[1]);
    if (hit == 0 || 1.0f < t[0] || t[1] < 0.0f)
        return 0;
    if (t[0] < 0.0f)
        hit ^= 1;
    if (1.0f < t[1])
        hit ^= 0x10;
    if (t0 != NULL)
        *t0 = t[0];
    if (t1 != NULL)
        *t1 = t[1];
    return hit;
}

// ──────────────────────────────────────────────────────────────────────
// Folded into i3dSegCylinderTest's body by Ghidra (no references).
// ROR 0x00236718

int i3dCylLineCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                           float *t0, float *t1)
{
    float grown[4];
    i3dVecCopyW(grown, cyl);
    grown[3] += p[3];
    return i3dLineCylinderTest(p, d, grown, axis, t0, t1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002367a0

int i3dCylHalfCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                           float *t0, float *t1)
{
    float grown[4];
    float t[2] = {0.0f, 0.0f}; /* PORT: see i3dLineCylinderTest */
    i3dVecCopyW(grown, cyl);
    grown[3] += p[3];
    int hit = i3dLineCylinderTest(p, d, grown, axis, &t[0], &t[1]);
    if (hit == 0 || t[1] < 0.0f)
        return 0;
    if (t[0] < 0.0f)
        hit ^= 1;
    if (t0 != NULL)
        *t0 = t[0];
    if (t1 != NULL)
        *t1 = t[1];
    return hit;
}

// ──────────────────────────────────────────────────────────────────────
// Folded into i3dCylHalfCylinderTest's body by Ghidra (no references).
// ROR 0x00236870

int i3dCylSegCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                          float *t0, float *t1)
{
    float grown[4];
    float t[2] = {0.0f, 0.0f}; /* PORT: see i3dLineCylinderTest */
    i3dVecCopyW(grown, cyl);
    grown[3] += p[3];
    int hit = i3dLineCylinderTest(p, d, grown, axis, &t[0], &t[1]);
    if (hit == 0 || 1.0f < t[0] || t[1] < 0.0f)
        return 0;
    if (t[0] < 0.0f)
        hit ^= 1;
    if (1.0f < t[1])
        hit ^= 0x10;
    if (t0 != NULL)
        *t0 = t[0];
    if (t1 != NULL)
        *t1 = t[1];
    return hit;
}
