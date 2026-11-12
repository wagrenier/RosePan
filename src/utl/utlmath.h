/* ==========================================================================
 *  utl/utlmath.h
 *
 *  utl/utlmath.cpp (ROR 0x106658..0x10737c, between utl/utlfind.cpp and
 *  dog/dogmotionblend.cpp): the map-area sound id swap, the i3r draw and
 *  node display wrappers, and the vector helpers -- rotations, dot / cross,
 *  lengths, distances and a direction's angles. RoR names no file here;
 *  utlmath is named for its majority.
 *
 *  Vectors are four floats (sceVu0FVECTOR or a float pointer to one); the
 *  helpers on the VU0 library read x, y, z only. Angles are radians; the
 *  rotation wraps and atan turns use the ROM's own pi (0x40490fdc, a bit
 *  over M_PI as a float) and pi/2 (0x3fc90fda, a bit under).
 * ======================================================================== */
#ifndef _UTL_UTLMATH_H
#define _UTL_UTLMATH_H

#include "i3d/i3rroot.h"
#include "sdk/libvu0.h"

struct I3dLayer;

/* Swaps a sound id 0x0a00xxxx / 0x0a01xxxx / 0x0a02xxxx (its column, map
 * area 0 / 1 / 2) for its twin in the current map area's column
 * (GamMan::getMapArea; the table at data 0x295f08). Other ids, ids of the
 * current area, no area (-1) or an id not in the table pass unchanged. */
u_int utlConvSndId(u_int id);                                            // ROR 0x00106658

/* Draws an i3r model into layer with its node matrices (NULL: none) and
 * world matrix, the thread switch held off around it (i3rDrawModel
 * 0x21fe68). opt is NULL or a record of a mask and five floats
 * (DogActor::drawModel's {0x1f, v x5} for a model with an animated value;
 * role unverified). Returns i3rDrawModel's result (0xd: culled). */
int   utlDrawI3r(I3dLayer *layer, I3rRoot *root, sceVu0FMATRIX *nodes, sceVu0FVECTOR *world,
                 const void *opt);                                       // ROR 0x00106760
/* Points the data of slot 0 of each of the root's nodes (child 3, type
 * 0x2a) at disp. Returns 0. */
int   utlSetI3rDispControl(I3rRoot *root, void *disp);                   // ROR 0x001067d8
/* Node n's slot-0 data set to disp: 0, or 1 when the node or its slot is
 * missing. */
int   utlSetI3rNodeDisp(I3rRoot *root, void *disp, int n);               // ROR 0x0010686c
/* Node n's slot-0 data, NULL when missing. */
void *utlGetI3rNodeDisp(I3rRoot *root, int n);                           // ROR 0x001068d4

/* Copy forms: out = in turned by angle about one axis (cosf / sinf); out
 * may be in. out[3] is not set by the ROM (an unwritten stack word); the
 * host copies in[3]. */
void  utlVecRotX(float angle, float *out, const float *in);              // ROR 0x00106920
void  utlVecRotY(float angle, float *out, const float *in);              // ROR 0x001069b0
void  utlVecRotZ(float angle, float *out, const float *in);              // ROR 0x00106a40
/* sqrt(x * x + y * y + z * z) of v, on the FPU. */
float utlVecLength3(const float *v);                                     // ROR 0x00106ad0
/* Matrix forms: v (four floats) in place by sceVu0RotMatrixX / Y / Z of
 * angle wrapped once into -pi..pi, then sceVu0ApplyMatrix. */
void  utlVecRotMatX(float angle, float *v);                              // ROR 0x00106b24
void  utlVecRotMatY(float angle, float *v);                              // ROR 0x00106bc8
void  utlVecRotMatZ(float angle, float *v);                              // ROR 0x00106c6c
/* sceVu0InnerProduct(a, b): the dot product of x, y, z. */
float utlVecDot(const float *a, const float *b);                         // ROR 0x00106d10
/* sceVu0OuterProduct(out, a, b): out = a x b. */
void  utlVecCross(float *out, const float *a, const float *b);           // ROR 0x00106d24
/* out[i] = a[i] * (1 - t) + b[i] * t, four floats. */
void  utlLerp4(float t, float *out, const float *a, const float *b);      // ROR 0x00106d40
/* v[3] = 0, then v normalised in place (sceVu0Normalize). */
void  utlNormalize3(float *v);                                           // ROR 0x00106db0
/* sqrt(utlVecDot(v, v)). */
float utlVecLength(const float *v);                                      // ROR 0x00106dd0
/* The squared distance of a and b (x, y, z). */
float utlVecDistSq(const float *a, const float *b);                      // ROR 0x00106e14
/* The distance of a and b: sqrt(utlVecDistSq). */
float utlVecDist(const float *a, const float *b);                        // ROR 0x00106e70
/* The same distance with utlVecDistSq written out (a separate ROM body). */
float utlVecDistInline(const float *a, const float *b);                  // ROR 0x00106eb4
/* The distance of a and b on the ground plane (y ignored). */
float utlVecDistXZ(const float *a, const float *b);                      // ROR 0x00106f38
/* The distance from a to b; *rot = the y rotation of b - a (the
 * utlVecToRotY rules). */
float utlVecDistRotY(const float *a, float *rot, const float *b);        // ROR 0x00106fb4
/* The y rotation of a direction: atan(x / z), turned half a turn when
 * z < 0, into -pi..pi; +-pi/2 for z == 0, 0 for x == z == 0. */
float utlVecToRotY(const float *dir);                                    // ROR 0x001070f8
/* The x rotation of a direction: asin(-y); 0 when y == 0 with z != 0,
 * pi/2 for x == z == 0. dir is expected normalised. */
float utlVecToRotX(const float *dir);                                    // ROR 0x001071c8
/* dir normalised in place (utlNormalize3: dir[3] = 0), then rot = its x
 * rotation (asin(-y)), y rotation (utlVecToRotY) and z 0; rot[3] is
 * left alone. x == z == 0 gives (pi/2, 0, 0) whatever the sign of y. */
void  utlVecToRot(float *dir, float *rot);                               // ROR 0x00107248

#endif /* _UTL_UTLMATH_H */
