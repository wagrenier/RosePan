/* ==========================================================================
 *  i3d/i3dmath.h
 *
 *  The i3d library's vector / matrix module (i3d/i3dmath.cpp, ROR
 *  0x236960..0x237ce8): VU0 macro-mode helpers over 16-byte vectors and
 *  4x4 matrices, plus i3dSysInit / i3dPrintf / i3dFlushCache at its end.
 *
 *  The ROM links the vector set twice. The first copy (0x236960..0x237178,
 *  names i3dVu0* where the second copy took the plain name) builds its
 *  rotations from cosf / sinf and has one rotation order; the second
 *  (0x237180..0x237c68) wraps sceVu0RotMatrixX / Y / Z and has all six
 *  orders, and adds the clear / set / compare / colour helpers. Several
 *  functions also appear twice inside the second copy (i3dVecAdd /
 *  i3dVecAdd2 ...): byte copies, kept as separate functions.
 *
 *  Conventions (all from the VU0 code):
 *  - Vectors are float[4] (xyzw); the ROM uses lq / sq, so they are
 *    16-aligned on the EE. Matrices are four row vectors (sceVu0FVECTOR *)
 *    used with row vectors: p' = p * M, row 3 the translation.
 *  - i3dVu0MatMul(out, a, b) = b * a: p * out = (p * b) * a. A rotation
 *    call rot(angle, out, in) gives out = in * R (rotate after in).
 *  - Outputs may alias inputs everywhere.
 *  - A float argument travels in f12 (f13 ...), so it comes first here, as
 *    in the rest of the project (i3dVecScale(s, out, in)).
 * ======================================================================== */
#ifndef _I3D_I3DMATH_H
#define _I3D_I3DMATH_H

/* ---- the first copy (0x236960..0x237178) ---- */

void  i3dVecZero(float *v);                                              // ROR 0x00236960
void  i3dVu0MatZero(sceVu0FVECTOR *m);                                   // ROR 0x00236968
/* Copies four words. */
void  i3dVecCopyW(float *out, const float *in);                          // ROR 0x00236980
void  i3dMatCopy(sceVu0FVECTOR *out, sceVu0FVECTOR *in);                 // ROR 0x00236990
void  i3dVu0MatUnit(sceVu0FVECTOR *m);                                   // ROR 0x002369b8
void  i3dVu0VecScale(float s, float *out, const float *in);              // ROR 0x002369d8
/* xyz scaled, w copied. */
void  i3dVu0VecScaleXYZ(float s, float *out, const float *in);           // ROR 0x002369f0
/* out = v.x * m[0] + v.y * m[1] + v.z * m[2] + v.w * m[3]. */
void  i3dVu0MatApply(float *out, sceVu0FVECTOR *m, const float *v);      // ROR 0x00236a08
void  i3dVu0VecAdd(float *out, const float *a, const float *b);          // ROR 0x00236a38
void  i3dVu0VecSub(float *out, const float *a, const float *b);          // ROR 0x00236a50
/* out = -in, all four components. */
void  i3dVu0VecNeg(float *out, const float *in);                         // ROR 0x00236a68
void  i3dVu0VecMul(float *out, const float *a, const float *b);          // ROR 0x00236a80
/* out = a * t + b * (1 - t), all four (t selects a). */
void  i3dVu0VecInterp(float t, float *out, const float *a, const float *b); // ROR 0x00236a98
/* Each component clamped to [lo, hi]. */
void  i3dVu0VecClamp(float lo, float hi, float *out, const float *in);   // ROR 0x00236ac8
/* out = a x b (xyz), w 0. */
void  i3dVecCross(float *out, const float *a, const float *b);           // ROR 0x00236af0
/* The xyz dot product. */
float i3dVu0VecDot(const float *a, const float *b);                      // ROR 0x00236b10
/* xyz normalized (times the reciprocal square root), w 0. */
void  i3dVecNormalize(float *out, const float *in);                      // ROR 0x00236b38
void  i3dVu0VecFtoi0(int *out, const float *in);                         // ROR 0x00236b60
void  i3dVu0VecFtoi4(int *out, const float *in);   /* 12.4 fixed */      // ROR 0x00236b70
void  i3dVu0VecItof0(float *out, const int *in);                         // ROR 0x00236b80
void  i3dVu0VecItof4(float *out, const int *in);   /* 12.4 fixed */      // ROR 0x00236b90
/* out = in with v.xyz added to the translation row. */
void  i3dVu0MatTranslate(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v); // ROR 0x00236ba0
/* The inverse of an affine matrix (3x3 part and translation). */
void  i3dVu0MatInverse(sceVu0FVECTOR *out, sceVu0FVECTOR *in);           // ROR 0x00236bd8
/* The inverse of a rigid matrix: the 3x3 part transposed, translation
 * -t * R^T, column w (0, 0, 0, 1). */
void  i3dVu0MatInverseOrtho(sceVu0FVECTOR *out, sceVu0FVECTOR *in);      // ROR 0x00236c78
void  i3dVu0MatTranspose(sceVu0FVECTOR *out, sceVu0FVECTOR *in);         // ROR 0x00236ce8
/* out row i = sum over j of b[i][j] * a[j] (VU0 micro 0). */
void  i3dVu0MatMul(sceVu0FVECTOR *out, sceVu0FVECTOR *a, sceVu0FVECTOR *b); // ROR 0x00236d30
void  i3dVu0MatRotX(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in); // ROR 0x00236d78
void  i3dVu0MatRotY(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in); // ROR 0x00236e00
void  i3dVu0MatRotZ(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in); // ROR 0x00236e88
/* Z by rot.z first, then Y, then X. */
void  i3dVu0MatRotZYX(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *rot); // ROR 0x00236f10
/* Row i scaled by v[i] (all four rows / rows 0..2, row 3 copied). */
void  i3dVu0MatScale(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v);    // ROR 0x00236f60
void  i3dVu0MatScaleXYZ(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v); // ROR 0x00236f98
/* p = v * m, divided by its w; out x, y in 12.4 fixed point. mode 0: z and
 * w in 12.4 too; mode 1: z and w truncated; other modes leave them. */
void  i3dVu0RotTransPers(int *out, sceVu0FVECTOR *m, const float *v, int mode); // ROR 0x00236fd0
/* A view matrix from an eye, a view direction and an up vector
 * (sceVu0CameraMatrix's construction). */
void  i3dVu0MatLookDir(sceVu0FVECTOR *out, const float *eye, const float *dir,
                       const float *up);                                 // ROR 0x002370b0
/* The unit matrix (rodata 0x303c10). */
sceVu0FVECTOR *i3dMatGetUnit(void);                                      // ROR 0x00237168
void  i3dSysInit(void);            /* empty in the ROM */                // ROR 0x00237178

/* ---- the second copy (0x237180..0x237c68) ---- */

void  i3dVecClear(float *v);                                             // ROR 0x00237180
void  i3dVecClear2(float *v);                                            // ROR 0x00237188
/* (0, 0, 0, 1). */
void  i3dVecSetOrigin(float *v);                                         // ROR 0x00237190
void  i3dVecSet(float x, float y, float z, float w, float *out);         // ROR 0x00237198
void  i3dVecSet2(float x, float y, float z, float w, float *out);        // ROR 0x002371b0
void  i3dMatZero(sceVu0FVECTOR *m);                                      // ROR 0x002371c8
/* 1 when the 16 bytes / 64 bytes are equal bit for bit. */
int   i3dVecEqual(const float *a, const float *b);                       // ROR 0x002371e0
int   i3dVecEqual2(const float *a, const float *b);                      // ROR 0x00237210
int   i3dMatEqual(sceVu0FVECTOR *a, sceVu0FVECTOR *b);                   // ROR 0x00237240
void  i3dVecCopy(float *out, const float *in);                           // ROR 0x002372d0
void  i3dVecCopy2(float *out, const float *in);                          // ROR 0x002372e0
void  i3dMatCopy2(sceVu0FVECTOR *out, sceVu0FVECTOR *in);                // ROR 0x002372f0
void  i3dMatSetRows(sceVu0FVECTOR *out, const float *r0, const float *r1, const float *r2,
                    const float *r3);                                    // ROR 0x00237318
void  i3dMatUnit(sceVu0FVECTOR *m);                                      // ROR 0x00237340
void  i3dVecScale(float s, float *out, const float *in);                 // ROR 0x00237360
void  i3dVecScale2(float s, float *out, const float *in);                // ROR 0x00237378
/* xyz scaled, w copied. */
void  i3dVecScaleXYZ(float s, float *out, const float *in);              // ROR 0x00237390
/* out = v.x * m[0] + v.y * m[1] + v.z * m[2] + v.w * m[3]. */
void  i3dMatApply(float *out, sceVu0FVECTOR *m, const float *v);         // ROR 0x002373a8
void  i3dVecAdd(float *out, const float *a, const float *b);             // ROR 0x002373d8
void  i3dVecAdd2(float *out, const float *a, const float *b);            // ROR 0x002373f0
void  i3dVecSub(float *out, const float *a, const float *b);             // ROR 0x00237408
void  i3dVecSub2(float *out, const float *a, const float *b);            // ROR 0x00237420
void  i3dVecMul(float *out, const float *a, const float *b);             // ROR 0x00237438
void  i3dVecMul2(float *out, const float *a, const float *b);            // ROR 0x00237450
/* out = -in, all four components. */
void  i3dVecNeg(float *out, const float *in);                            // ROR 0x00237468
void  i3dVecInterp(float t, float *out, const float *a, const float *b);  // ROR 0x00237480
void  i3dVecInterp2(float t, float *out, const float *a, const float *b); // ROR 0x002374b0
void  i3dVecClamp(float lo, float hi, float *out, const float *in);      // ROR 0x002374e0
void  i3dVecClamp2(float lo, float hi, float *out, const float *in);     // ROR 0x00237508
/* A 0..1 colour as an RGBA32 word: r in the low byte. */
u_int i3dVecToRgba32(const float *color);                                // ROR 0x00237530
void  i3dVecCross2(float *out, const float *a, const float *b);          // ROR 0x00237560
float i3dVecDot(const float *a, const float *b);                         // ROR 0x00237580
float i3dVecLength(const float *v);                                      // ROR 0x002375a8
void  i3dVecNormalize2(float *out, const float *in);                     // ROR 0x002375d8
void  i3dVecFtoi0(int *out, const float *in);                            // ROR 0x00237600
void  i3dVecFtoi4(int *out, const float *in);      /* 12.4 fixed */      // ROR 0x00237610
void  i3dVecItof0(float *out, const int *in);                            // ROR 0x00237620
void  i3dVecItof4(float *out, const int *in);      /* 12.4 fixed */      // ROR 0x00237630
void  i3dMatTranslate(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v); // ROR 0x00237640
void  i3dMatInverse(sceVu0FVECTOR *out, sceVu0FVECTOR *in);              // ROR 0x00237678
void  i3dMatInverseOrtho(sceVu0FVECTOR *out, sceVu0FVECTOR *in);         // ROR 0x00237718
void  i3dMatTranspose(sceVu0FVECTOR *out, sceVu0FVECTOR *in);            // ROR 0x00237788
void  i3dMatMul(sceVu0FVECTOR *out, sceVu0FVECTOR *a, sceVu0FVECTOR *b); // ROR 0x002377d0
/* sceVu0RotMatrixX / Y / Z (out = in * R). */
void  i3dMatRotX(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in);    // ROR 0x00237818
void  i3dMatRotY(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in);    // ROR 0x00237830
void  i3dMatRotZ(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in);    // ROR 0x00237848
/* The six rotation orders, named in the order they are applied. */
void  i3dMatRotZYX(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *rot); // ROR 0x00237860
void  i3dMatRotZXY(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *rot); // ROR 0x002378b0
void  i3dMatRotXYZ(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *rot); // ROR 0x00237900
void  i3dMatRotXZY(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *rot); // ROR 0x00237950
void  i3dMatRotYXZ(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *rot); // ROR 0x002379a0
void  i3dMatRotYZX(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *rot); // ROR 0x002379f0
void  i3dMatScale(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v);    // ROR 0x00237a40
void  i3dMatScaleXYZ(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v); // ROR 0x00237a78
void  i3dRotTransPers(int *out, sceVu0FVECTOR *m, const float *v, int mode); // ROR 0x00237ab0
/* A view matrix from an eye, a view direction and an up vector. */
void  i3dMatLookDir(sceVu0FVECTOR *out, const float *eye, const float *dir,
                    const float *up);                                    // ROR 0x00237ba0
/* The second unit matrix (rodata 0x303c60). */
sceVu0FVECTOR *i3dMatGetUnit2(void);                                     // ROR 0x00237c58
/* vsprintf into a 1 KB buffer, rxSysPrintf, fflush(stdout); returns 0. */
int   i3dPrintf(const char *fmt, ...);                                   // ROR 0x00237c68
/* FlushCache(0); returns 0. */
int   i3dFlushCache(void);                                               // ROR 0x00237cc8

#endif /* _I3D_I3DMATH_H */
