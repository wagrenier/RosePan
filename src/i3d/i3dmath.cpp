/* ==========================================================================
 *  i3d/i3dmath.cpp
 *
 *  The i3d library's vector / matrix module (ROR 0x236960..0x237ce8): VU0
 *  macro-mode helpers, written out as plain float code. See i3dmath.h for
 *  the conventions and the two copies the ROM links. Every function reads
 *  its inputs before it writes (the VU0 code holds them in registers), so
 *  outputs may alias inputs.
 * ======================================================================== */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "i3d/i3dmath.h"
#include "rx/rxsys.h"
#include "sdk/eekernel.h"
#include "sdk/libvu0.h"

/* rodata 0x303c00 / 0x303c50: the perspective numerator and the 12.4 scale
 * of the two RotTransPers copies. */
static const float s_pers_one = 1.0f;
static const float s_pers_fix = 16.0f;

/* rodata 0x303c10 / 0x303c60: the unit matrices i3dMatGetUnit /
 * i3dMatGetUnit2 return. */
static sceVu0FMATRIX s_unit = {
    { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f },
};
static sceVu0FMATRIX s_unit2 = {
    { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f },
};

/* ---- shared bodies: the ROM repeats each one as straight-line VU0 code ---- */

static void mathMatUnit(sceVu0FVECTOR *m)
{
    /* vmr32 of vf0 (0, 0, 0, 1) three times: rows 3, 2, 1, 0. */
    memset(m, 0, sizeof(sceVu0FMATRIX));
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
}

static void mathVecScale(float s, float *out, const float *in, int n)
{
    for (int i = 0; i < n; i++)
        out[i] = in[i] * s;
}

static void mathMatApply(float *out, sceVu0FVECTOR *m, const float *v)
{
    float r[4];
    for (int k = 0; k < 4; k++)
        r[k] = m[0][k] * v[0] + m[1][k] * v[1] + m[2][k] * v[2] + m[3][k] * v[3];
    memcpy(out, r, sizeof(r));
}

static void mathVecInterp(float t, float *out, const float *a, const float *b)
{
    /* vmulax by t, vmaddx by 1 - t: w is blended too (the vmove.w before
     * it is overwritten). */
    float u = 1.0f - t;
    for (int i = 0; i < 4; i++)
        out[i] = a[i] * t + b[i] * u;
}

static void mathVecClamp(float lo, float hi, float *out, const float *in)
{
    for (int i = 0; i < 4; i++)
    {
        float v = in[i] > lo ? in[i] : lo;     /* vmaxx */
        out[i] = v < hi ? v : hi;              /* vminibcx */
    }
}

static void mathVecCross(float *out, const float *a, const float *b)
{
    /* vopmula / vopmsub, w cleared first. */
    float x = a[1] * b[2] - b[1] * a[2];
    float y = a[2] * b[0] - b[2] * a[0];
    float z = a[0] * b[1] - b[0] * a[1];
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = 0.0f;
}

static float mathVecDot(const float *a, const float *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void mathVecNormalize(float *out, const float *in)
{
    /* vrsqrt of the squared length, w 0. */
    float q = 1.0f / sqrtf(mathVecDot(in, in));
    float x = in[0] * q, y = in[1] * q, z = in[2] * q;
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = 0.0f;
}

static void mathMatTranslate(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v)
{
    if (out != in)
    {
        memcpy(out[0], in[0], sizeof(sceVu0FVECTOR));
        memcpy(out[1], in[1], sizeof(sceVu0FVECTOR));
        memcpy(out[2], in[2], sizeof(sceVu0FVECTOR));
    }
    /* vadd.xyz: w stays in[3].w. */
    out[3][0] = in[3][0] + v[0];
    out[3][1] = in[3][1] + v[1];
    out[3][2] = in[3][2] + v[2];
    out[3][3] = in[3][3];
}

static void mathMatInverse(sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    /* The adjugate rows (cross products) over the determinant, transposed;
     * translation -t * R^-1; column w 0, 0, 0, 1. */
    sceVu0FVECTOR c5, c6, c7;
    float t[3] = { -in[3][0], -in[3][1], -in[3][2] };
    mathVecCross(c7, in[0], in[1]);
    mathVecCross(c5, in[1], in[2]);
    mathVecCross(c6, in[2], in[0]);
    float q = 1.0f / (c7[0] * in[2][0] + c7[1] * in[2][1] + c7[2] * in[2][2]);
    for (int i = 0; i < 3; i++)
    {
        out[i][0] = q * c5[i];
        out[i][1] = q * c6[i];
        out[i][2] = q * c7[i];
        out[i][3] = 0.0f;
    }
    for (int k = 0; k < 3; k++)
        out[3][k] = out[0][k] * t[0] + out[1][k] * t[1] + out[2][k] * t[2];
    out[3][3] = 1.0f;
}

static void mathMatInverseOrtho(sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    /* pextlw / pcpyld transpose the 3x3 part with column w (0, 0, 0); the
     * translation row is 0 - t * R^T, w 1. */
    sceVu0FMATRIX r;
    for (int i = 0; i < 3; i++)
    {
        for (int k = 0; k < 3; k++)
            r[i][k] = in[k][i];
        r[i][3] = 0.0f;
    }
    for (int k = 0; k < 3; k++)
        r[3][k] = 0.0f - (r[0][k] * in[3][0] + r[1][k] * in[3][1] + r[2][k] * in[3][2]);
    r[3][3] = 1.0f;
    memcpy(out, r, sizeof(r));
}

static void mathMatTranspose(sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    sceVu0FMATRIX r;
    for (int i = 0; i < 4; i++)
        for (int k = 0; k < 4; k++)
            r[i][k] = in[k][i];
    memcpy(out, r, sizeof(r));
}

static void mathMatMul(sceVu0FVECTOR *out, sceVu0FVECTOR *a, sceVu0FVECTOR *b)
{
    /* VU0 micro 0 with vf4..7 = a, vf8..11 = b, the result in vf8..11. */
    sceVu0FMATRIX r;
    for (int i = 0; i < 4; i++)
        for (int k = 0; k < 4; k++)
            r[i][k] = a[0][k] * b[i][0] + a[1][k] * b[i][1] + a[2][k] * b[i][2] + a[3][k] * b[i][3];
    memcpy(out, r, sizeof(r));
}

static void mathMatScale(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v, int rows)
{
    float s[4] = { v[0], v[1], v[2], v[3] };
    for (int i = 0; i < 4; i++)
        for (int k = 0; k < 4; k++)
            out[i][k] = i < rows ? in[i][k] * s[i] : in[i][k];
}

static void mathVecFtoi(int *out, const float *in, float scale)
{
    int r[4];
    for (int i = 0; i < 4; i++)
        r[i] = (int)(in[i] * scale);
    memcpy(out, r, sizeof(r));
}

static void mathVecItof(float *out, const int *in, float scale)
{
    float r[4];
    for (int i = 0; i < 4; i++)
        r[i] = (float)in[i] / scale;
    memcpy(out, r, sizeof(r));
}

/* ======================================================================
 *  The first copy
 * ====================================================================== */

/* ROR 0x00236960 */
void i3dVecZero(float *v)
{
    memset(v, 0, sizeof(sceVu0FVECTOR));
}

/* ROR 0x00236968 */
void i3dVu0MatZero(sceVu0FVECTOR *m)
{
    memset(m, 0, sizeof(sceVu0FMATRIX));
}

/* ROR 0x00236980 */
void i3dVecCopyW(float *out, const float *in)
{
    memmove(out, in, sizeof(sceVu0FVECTOR));
}

/* ROR 0x00236990 */
void i3dMatCopy(sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    memmove(out, in, sizeof(sceVu0FMATRIX));
}

/* ROR 0x002369b8 */
void i3dVu0MatUnit(sceVu0FVECTOR *m)
{
    mathMatUnit(m);
}

/* ROR 0x002369d8 */
void i3dVu0VecScale(float s, float *out, const float *in)
{
    mathVecScale(s, out, in, 4);
}

/* ROR 0x002369f0 */
void i3dVu0VecScaleXYZ(float s, float *out, const float *in)
{
    out[3] = in[3];
    mathVecScale(s, out, in, 3);
}

/* ROR 0x00236a08 */
void i3dVu0MatApply(float *out, sceVu0FVECTOR *m, const float *v)
{
    mathMatApply(out, m, v);
}

/* ROR 0x00236a38 */
void i3dVu0VecAdd(float *out, const float *a, const float *b)
{
    for (int i = 0; i < 4; i++)
        out[i] = a[i] + b[i];
}

/* ROR 0x00236a50 */
void i3dVu0VecSub(float *out, const float *a, const float *b)
{
    for (int i = 0; i < 4; i++)
        out[i] = a[i] - b[i];
}

/* ROR 0x00236a68 */
void i3dVu0VecNeg(float *out, const float *in)
{
    /* (vf0 - vf0) - v. */
    for (int i = 0; i < 4; i++)
        out[i] = 0.0f - in[i];
}

/* ROR 0x00236a80 */
void i3dVu0VecMul(float *out, const float *a, const float *b)
{
    for (int i = 0; i < 4; i++)
        out[i] = a[i] * b[i];
}

/* ROR 0x00236a98 */
void i3dVu0VecInterp(float t, float *out, const float *a, const float *b)
{
    mathVecInterp(t, out, a, b);
}

/* ROR 0x00236ac8 */
void i3dVu0VecClamp(float lo, float hi, float *out, const float *in)
{
    mathVecClamp(lo, hi, out, in);
}

/* ROR 0x00236af0 */
void i3dVecCross(float *out, const float *a, const float *b)
{
    mathVecCross(out, a, b);
}

/* ROR 0x00236b10 */
float i3dVu0VecDot(const float *a, const float *b)
{
    return mathVecDot(a, b);
}

/* ROR 0x00236b38 */
void i3dVecNormalize(float *out, const float *in)
{
    mathVecNormalize(out, in);
}

/* ROR 0x00236b60 */
void i3dVu0VecFtoi0(int *out, const float *in)
{
    mathVecFtoi(out, in, 1.0f);
}

/* ROR 0x00236b70 */
void i3dVu0VecFtoi4(int *out, const float *in)
{
    mathVecFtoi(out, in, 16.0f);
}

/* ROR 0x00236b80 */
void i3dVu0VecItof0(float *out, const int *in)
{
    mathVecItof(out, in, 1.0f);
}

/* ROR 0x00236b90 */
void i3dVu0VecItof4(float *out, const int *in)
{
    mathVecItof(out, in, 16.0f);
}

/* ROR 0x00236ba0 */
void i3dVu0MatTranslate(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v)
{
    mathMatTranslate(out, in, v);
}

/* ROR 0x00236bd8 */
void i3dVu0MatInverse(sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    mathMatInverse(out, in);
}

/* ROR 0x00236c78 */
void i3dVu0MatInverseOrtho(sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    mathMatInverseOrtho(out, in);
}

/* ROR 0x00236ce8 */
void i3dVu0MatTranspose(sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    mathMatTranspose(out, in);
}

/* ROR 0x00236d30 */
void i3dVu0MatMul(sceVu0FVECTOR *out, sceVu0FVECTOR *a, sceVu0FVECTOR *b)
{
    mathMatMul(out, a, b);
}

/* ROR 0x00236d78 */
void i3dVu0MatRotX(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    sceVu0FMATRIX rot;

    i3dVu0MatUnit(rot);
    rot[1][1] = cosf(angle);
    rot[1][2] = sinf(angle);
    rot[2][1] = -sinf(angle);
    rot[2][2] = cosf(angle);
    i3dVu0MatMul(out, rot, in);
}

/* ROR 0x00236e00 */
void i3dVu0MatRotY(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    sceVu0FMATRIX rot;

    i3dVu0MatUnit(rot);
    rot[0][0] = cosf(angle);
    rot[2][0] = sinf(angle);
    rot[0][2] = -sinf(angle);
    rot[2][2] = cosf(angle);
    i3dVu0MatMul(out, rot, in);
}

/* ROR 0x00236e88 */
void i3dVu0MatRotZ(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    sceVu0FMATRIX rot;

    i3dVu0MatUnit(rot);
    rot[0][0] = cosf(angle);
    rot[0][1] = sinf(angle);
    rot[1][0] = -sinf(angle);
    rot[1][1] = cosf(angle);
    i3dVu0MatMul(out, rot, in);
}

/* ROR 0x00236f10 */
void i3dVu0MatRotZYX(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *rot)
{
    i3dVu0MatRotZ(rot[2], out, in);
    i3dVu0MatRotY(rot[1], out, out);
    i3dVu0MatRotX(rot[0], out, out);
}

/* ROR 0x00236f60 */
void i3dVu0MatScale(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v)
{
    mathMatScale(out, in, v, 4);
}

/* ROR 0x00236f98 */
void i3dVu0MatScaleXYZ(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v)
{
    mathMatScale(out, in, v, 3);
}

/* ROR 0x00236fd0 */
void i3dVu0RotTransPers(int *out, sceVu0FVECTOR *m, const float *v, int mode)
{
    sceVu0FVECTOR p;
    float q;

    i3dVu0MatApply(p, m, v);
    q = s_pers_one / p[3];
    p[0] *= q;
    p[1] *= q;
    p[2] *= q;
    out[0] = (int)(p[0] * s_pers_fix);
    out[1] = (int)(p[1] * s_pers_fix);
    if (mode == 0)
    {
        out[3] = (int)(p[3] * s_pers_fix);
        out[2] = (int)(p[2] * s_pers_fix);
    }
    else if (mode == 1)
    {
        out[3] = (int)p[3];
        out[2] = (int)p[2];
    }
}

/* ROR 0x002370b0 */
void i3dVu0MatLookDir(sceVu0FVECTOR *out, const float *eye, const float *dir, const float *up)
{
    sceVu0FMATRIX m;
    sceVu0FVECTOR x;

    i3dVu0MatUnit(m);
    i3dVecCross(x, up, dir);
    i3dVecNormalize(m[0], x);
    i3dVecNormalize(m[2], dir);
    i3dVecCross(m[1], m[2], m[0]);
    i3dVu0MatTranslate(m, m, eye);
    i3dVu0MatInverse(out, m);
}

/* ROR 0x00237168 */
sceVu0FVECTOR *i3dMatGetUnit(void)
{
    return s_unit;
}

/* ROR 0x00237178 */
void i3dSysInit(void)
{
}

/* ======================================================================
 *  The second copy
 * ====================================================================== */

/* ROR 0x00237180 */
void i3dVecClear(float *v)
{
    memset(v, 0, sizeof(sceVu0FVECTOR));
}

/* ROR 0x00237188 */
void i3dVecClear2(float *v)
{
    memset(v, 0, sizeof(sceVu0FVECTOR));
}

/* ROR 0x00237190 */
void i3dVecSetOrigin(float *v)
{
    /* sqc2 vf0. */
    v[0] = v[1] = v[2] = 0.0f;
    v[3] = 1.0f;
}

/* ROR 0x00237198 */
void i3dVecSet(float x, float y, float z, float w, float *out)
{
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = w;
}

/* ROR 0x002371b0 */
void i3dVecSet2(float x, float y, float z, float w, float *out)
{
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = w;
}

/* ROR 0x002371c8 */
void i3dMatZero(sceVu0FVECTOR *m)
{
    memset(m, 0, sizeof(sceVu0FMATRIX));
}

/* ROR 0x002371e0 */
int i3dVecEqual(const float *a, const float *b)
{
    /* Two ld compares: bit for bit, so -0.0 != 0.0. */
    return memcmp(a, b, sizeof(sceVu0FVECTOR)) == 0;
}

/* ROR 0x00237210 */
int i3dVecEqual2(const float *a, const float *b)
{
    return memcmp(a, b, sizeof(sceVu0FVECTOR)) == 0;
}

/* ROR 0x00237240 */
int i3dMatEqual(sceVu0FVECTOR *a, sceVu0FVECTOR *b)
{
    return memcmp(a, b, sizeof(sceVu0FMATRIX)) == 0;
}

/* ROR 0x002372d0 */
void i3dVecCopy(float *out, const float *in)
{
    memmove(out, in, sizeof(sceVu0FVECTOR));
}

/* ROR 0x002372e0 */
void i3dVecCopy2(float *out, const float *in)
{
    memmove(out, in, sizeof(sceVu0FVECTOR));
}

/* ROR 0x002372f0 */
void i3dMatCopy2(sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    memmove(out, in, sizeof(sceVu0FMATRIX));
}

/* ROR 0x00237318 */
void i3dMatSetRows(sceVu0FVECTOR *out, const float *r0, const float *r1, const float *r2,
                   const float *r3)
{
    /* Each row is loaded right before its store, as the ROM does. */
    memmove(out[0], r0, sizeof(sceVu0FVECTOR));
    memmove(out[1], r1, sizeof(sceVu0FVECTOR));
    memmove(out[2], r2, sizeof(sceVu0FVECTOR));
    memmove(out[3], r3, sizeof(sceVu0FVECTOR));
}

/* ROR 0x00237340 */
void i3dMatUnit(sceVu0FVECTOR *m)
{
    mathMatUnit(m);
}

/* ROR 0x00237360 */
void i3dVecScale(float s, float *out, const float *in)
{
    mathVecScale(s, out, in, 4);
}

/* ROR 0x00237378 */
void i3dVecScale2(float s, float *out, const float *in)
{
    mathVecScale(s, out, in, 4);
}

/* ROR 0x00237390 */
void i3dVecScaleXYZ(float s, float *out, const float *in)
{
    out[3] = in[3];
    mathVecScale(s, out, in, 3);
}

/* ROR 0x002373a8 */
void i3dMatApply(float *out, sceVu0FVECTOR *m, const float *v)
{
    mathMatApply(out, m, v);
}

/* ROR 0x002373d8 */
void i3dVecAdd(float *out, const float *a, const float *b)
{
    for (int i = 0; i < 4; i++)
        out[i] = a[i] + b[i];
}

/* ROR 0x002373f0 */
void i3dVecAdd2(float *out, const float *a, const float *b)
{
    for (int i = 0; i < 4; i++)
        out[i] = a[i] + b[i];
}

/* ROR 0x00237408 */
void i3dVecSub(float *out, const float *a, const float *b)
{
    for (int i = 0; i < 4; i++)
        out[i] = a[i] - b[i];
}

/* ROR 0x00237420 */
void i3dVecSub2(float *out, const float *a, const float *b)
{
    for (int i = 0; i < 4; i++)
        out[i] = a[i] - b[i];
}

/* ROR 0x00237438 */
void i3dVecMul(float *out, const float *a, const float *b)
{
    for (int i = 0; i < 4; i++)
        out[i] = a[i] * b[i];
}

/* ROR 0x00237450 */
void i3dVecMul2(float *out, const float *a, const float *b)
{
    for (int i = 0; i < 4; i++)
        out[i] = a[i] * b[i];
}

/* ROR 0x00237468 */
void i3dVecNeg(float *out, const float *in)
{
    for (int i = 0; i < 4; i++)
        out[i] = 0.0f - in[i];
}

/* ROR 0x00237480 */
void i3dVecInterp(float t, float *out, const float *a, const float *b)
{
    mathVecInterp(t, out, a, b);
}

/* ROR 0x002374b0 */
void i3dVecInterp2(float t, float *out, const float *a, const float *b)
{
    mathVecInterp(t, out, a, b);
}

/* ROR 0x002374e0 */
void i3dVecClamp(float lo, float hi, float *out, const float *in)
{
    mathVecClamp(lo, hi, out, in);
}

/* ROR 0x00237508 */
void i3dVecClamp2(float lo, float hi, float *out, const float *in)
{
    mathVecClamp(lo, hi, out, in);
}

/* ROR 0x00237530 */
u_int i3dVecToRgba32(const float *color)
{
    /* Clamped to vf0.x / vf0.w (0 / 1), times 255.0, vftoi0, then ppach /
     * ppacb keep the low byte of each component. */
    float c[4];
    int n[4];

    mathVecClamp(0.0f, 1.0f, c, color);
    mathVecFtoi(n, c, 255.0f);
    return (u_int)(n[0] & 0xff) | (u_int)(n[1] & 0xff) << 8 | (u_int)(n[2] & 0xff) << 16
         | (u_int)(n[3] & 0xff) << 24;
}

/* ROR 0x00237560 */
void i3dVecCross2(float *out, const float *a, const float *b)
{
    mathVecCross(out, a, b);
}

/* ROR 0x00237580 */
float i3dVecDot(const float *a, const float *b)
{
    return mathVecDot(a, b);
}

/* ROR 0x002375a8 */
float i3dVecLength(const float *v)
{
    return sqrtf(mathVecDot(v, v));
}

/* ROR 0x002375d8 */
void i3dVecNormalize2(float *out, const float *in)
{
    mathVecNormalize(out, in);
}

/* ROR 0x00237600 */
void i3dVecFtoi0(int *out, const float *in)
{
    mathVecFtoi(out, in, 1.0f);
}

/* ROR 0x00237610 */
void i3dVecFtoi4(int *out, const float *in)
{
    mathVecFtoi(out, in, 16.0f);
}

/* ROR 0x00237620 */
void i3dVecItof0(float *out, const int *in)
{
    mathVecItof(out, in, 1.0f);
}

/* ROR 0x00237630 */
void i3dVecItof4(float *out, const int *in)
{
    mathVecItof(out, in, 16.0f);
}

/* ROR 0x00237640 */
void i3dMatTranslate(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v)
{
    mathMatTranslate(out, in, v);
}

/* ROR 0x00237678 */
void i3dMatInverse(sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    mathMatInverse(out, in);
}

/* ROR 0x00237718 */
void i3dMatInverseOrtho(sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    mathMatInverseOrtho(out, in);
}

/* ROR 0x00237788 */
void i3dMatTranspose(sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    mathMatTranspose(out, in);
}

/* ROR 0x002377d0 */
void i3dMatMul(sceVu0FVECTOR *out, sceVu0FVECTOR *a, sceVu0FVECTOR *b)
{
    mathMatMul(out, a, b);
}

/* ROR 0x00237818 */
void i3dMatRotX(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    sceVu0RotMatrixX(out, in, angle);
}

/* ROR 0x00237830 */
void i3dMatRotY(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    sceVu0RotMatrixY(out, in, angle);
}

/* ROR 0x00237848 */
void i3dMatRotZ(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    sceVu0RotMatrixZ(out, in, angle);
}

/* ROR 0x00237860 */
void i3dMatRotZYX(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *rot)
{
    i3dMatRotZ(rot[2], out, in);
    i3dMatRotY(rot[1], out, out);
    i3dMatRotX(rot[0], out, out);
}

/* ROR 0x002378b0 */
void i3dMatRotZXY(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *rot)
{
    i3dMatRotZ(rot[2], out, in);
    i3dMatRotX(rot[0], out, out);
    i3dMatRotY(rot[1], out, out);
}

/* ROR 0x00237900 */
void i3dMatRotXYZ(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *rot)
{
    i3dMatRotX(rot[0], out, in);
    i3dMatRotY(rot[1], out, out);
    i3dMatRotZ(rot[2], out, out);
}

/* ROR 0x00237950 */
void i3dMatRotXZY(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *rot)
{
    i3dMatRotX(rot[0], out, in);
    i3dMatRotZ(rot[2], out, out);
    i3dMatRotY(rot[1], out, out);
}

/* ROR 0x002379a0 */
void i3dMatRotYXZ(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *rot)
{
    i3dMatRotY(rot[1], out, in);
    i3dMatRotX(rot[0], out, out);
    i3dMatRotZ(rot[2], out, out);
}

/* ROR 0x002379f0 */
void i3dMatRotYZX(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *rot)
{
    i3dMatRotY(rot[1], out, in);
    i3dMatRotZ(rot[2], out, out);
    i3dMatRotX(rot[0], out, out);
}

/* ROR 0x00237a40 */
void i3dMatScale(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v)
{
    mathMatScale(out, in, v, 4);
}

/* ROR 0x00237a78 */
void i3dMatScaleXYZ(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v)
{
    mathMatScale(out, in, v, 3);
}

/* ROR 0x00237ab0 */
void i3dRotTransPers(int *out, sceVu0FVECTOR *m, const float *v, int mode)
{
    sceVu0FVECTOR p;

    i3dMatApply(p, m, v);
    i3dVecScaleXYZ(s_pers_one / p[3], p, p);
    out[0] = (int)(p[0] * s_pers_fix);
    out[1] = (int)(p[1] * s_pers_fix);
    if (mode == 0)
    {
        out[3] = (int)(p[3] * s_pers_fix);
        out[2] = (int)(p[2] * s_pers_fix);
    }
    else if (mode == 1)
    {
        out[3] = (int)p[3];
        out[2] = (int)p[2];
    }
}

/* ROR 0x00237ba0 */
void i3dMatLookDir(sceVu0FVECTOR *out, const float *eye, const float *dir, const float *up)
{
    sceVu0FMATRIX m;
    sceVu0FVECTOR x;

    i3dMatUnit(m);
    i3dVecCross2(x, up, dir);
    i3dVecNormalize2(m[0], x);
    i3dVecNormalize2(m[2], dir);
    i3dVecCross2(m[1], m[2], m[0]);
    i3dMatTranslate(m, m, eye);
    i3dMatInverse(out, m);
}

/* ROR 0x00237c58 */
sceVu0FVECTOR *i3dMatGetUnit2(void)
{
    return s_unit2;
}

/* ROR 0x00237c68 */
int i3dPrintf(const char *fmt, ...)
{
    char buf[0x400];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    /* PC: the ROM passes buf as rxSysPrintf's format; "%s" keeps a '%' in
     * the text from being read twice. */
    rxSysPrintf("%s", buf);
    fflush(stdout);
    return 0;
}

/* ROR 0x00237cc8 */
int i3dFlushCache(void)
{
    FlushCache(0);
    return 0;
}
