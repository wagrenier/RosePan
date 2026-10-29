/* ==========================================================================
 *  i3d/i3cquery.h
 *
 *  The i3c collision casts (i3d/i3cquery.cpp, ROR 0x2312a0..0x233628): a
 *  sphere, a line / ray / segment or a cylinder (an axis with a radius)
 *  cast against an i3c file's triangles, node by node.
 *
 *  A cast (i3cCast) takes the query, the shape (two vectors: a point with
 *  w = radius, then a direction) and a world matrix. For every enabled
 *  collision node it takes the node's model matrix (I3cQuery::m_mat through
 *  m_colli_tbl), inverts it and moves the shape into the node's frame
 *  (i3cTransformShape); then every part of the query's group is walked
 *  (i3cWalkTree): each tree box goes through the cast's box test (i3dgeom),
 *  each leaf triangle through its prim test (i3cPrim*), and a hit calls the
 *  query's m_func with the triangle's plane and the hit point.
 *
 *  Hit records: plane = (n, w), point = (p, w). The ray test passes the
 *  plane's unnormalised n with w = n.dir and the point with w = t; the
 *  sphere and cylinder tests the unit n with w = the shape's distance
 *  behind the plane and the point with w = a 0..1 depth ratio.
 * ======================================================================== */
#ifndef _I3D_I3CQUERY_H
#define _I3D_I3CQUERY_H

#include <stddef.h>

#include "i3d/i3croot.h"
#include "sdk/libvu0.h"

struct I3cQuery;
struct MagSceneRayHit;

/* A hit: code = node << 16 | what (3: the triangle's face; 0..2 a vertex,
 * 0x10..0x12 an edge, by index). 0 stops the cast (the tests return -1). */
typedef int (*I3cHitFunc)(I3cQuery *q, int code, const u_short *prim, const float *verts,
                          float *plane, float *point);
/* The optional box filter: 0 stops the cast. */
typedef int (*I3cBoxFunc)(I3cQuery *q, const I3cTree *tree, float *center, float *half, int node);

/* A collision cast's request (built on the stack by DogModel's casts).
 * layout: host -- pointers. */
struct I3cQuery
{
    I3cRoot        *m_root;       // ROR 0x00: m_i3c_root
    sceVu0FMATRIX  *m_mat;        // ROR 0x04: the model's node matrices (m_mat_model); NULL: one frame
    u_short        *m_colli_tbl;  // ROR 0x08: the collision node -> model node table (0xffff: none)
    int             m_group;      // ROR 0x0c: the parts' group cast when m_all_group is 0
    I3cHitFunc      m_func;       // ROR 0x10
    I3cBoxFunc      m_box_func;   // ROR 0x14: NULL for DogModel
    MagSceneRayHit *m_hit;        // ROR 0x18: the record the callback fills
    I3cPart        *m_part;       // ROR 0x1c: the part being walked (i3cCast writes it)
    int             m_all_group;  // ROR 0x20: nonzero: every part, whatever its group
};
static_assert(offsetof(I3cQuery, m_hit) >= 0x18, "m_hit");

/* The cast's box test: the shape's point relative to the box centre, its
 * direction, the box half extents; nonzero when they may touch. */
typedef int (*I3cBoxTest)(const float *rel, const float *dir, const float *half);
/* The cast's triangle test: 1 hit, 0 miss, -1 the callback stopped it.
 * scale: NULL, or the node's row lengths then their inverses. */
typedef int (*I3cPrimTest)(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                           int node, int both_face, const float *scale);

/* i3cCast's state for the tree walk. EE 0x18 (sp + 0x80 of i3cCast).
 * layout: host -- pointers. */
struct I3cCastCtx
{
    float          *m_scale;      // ROR 0x00: NULL, or the row lengths (16 bytes) then their inverses
    float          *m_shape;      // ROR 0x04: the shape in the node's frame (two vectors)
    I3cQuery       *m_query;      // ROR 0x08
    int             m_node;       // ROR 0x0c: the collision node
    I3cBoxTest      m_box_test;   // ROR 0x10
    I3cPrimTest     m_prim_test;  // ROR 0x14
};
static_assert(offsetof(I3cCastCtx, m_scale) == 0, "I3cCastCtx::m_scale");

/* The shape into a frame: out[0].xyz = m * (s[0], 1), out[1].xyz = the
 * rotation of s[1]; both w kept. */
void  i3cTransformShape(float *out, sceVu0FVECTOR *m, const float *s);              // ROR 0x002312a0
/* The ray tests: mode bit 0 t >= 0, bit 1 t <= 1. */
int   i3cPrimRay(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                 int node, int both_face, int mode);                                // ROR 0x002312e0
int   i3cPrimRayLine(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                     int node, int both_face, const float *scale);                  // ROR 0x002314c8
int   i3cPrimRayHalf(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                     int node, int both_face, const float *scale);                  // ROR 0x002314e8
int   i3cPrimRaySeg(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                    int node, int both_face, const float *scale);                   // ROR 0x00231508
int   i3cPrimSphere(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                    int node, int both_face, const float *scale);                   // ROR 0x00231528
/* The cylinder tests: mode 0 a line, 1 the 0x234050 edge distance, 2 a
 * segment (p .. p + d). */
int   i3cPrimCylinder(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                      int node, int both_face, const float *scale, int mode);       // ROR 0x00231948
int   i3cPrimCylinderLine(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                          int node, int both_face, const float *scale);             // ROR 0x00231e70
int   i3cPrimCylinderHalf(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                          int node, int both_face, const float *scale);             // ROR 0x00231e90
int   i3cPrimCylinderSeg(const float *shape, const I3cTri *prim, const float *verts, I3cQuery *q,
                         int node, int both_face, const float *scale);              // ROR 0x00231eb0
/* One tree box and what is under it: the hit count, -1 when stopped. */
int   i3cWalkTree(I3cCastCtx *ctx, const I3cTree *tree, const float *verts, const float *center,
                  const float *half, int both_face);                                // ROR 0x00231ed0
/* The casts' worker: the hit count (a stopped part adds -1). */
int   i3cCast(I3cQuery *q, const float *shape, sceVu0FMATRIX *mat, I3cBoxTest box_test,
              I3cPrimTest prim_test, int scale);                                    // ROR 0x00232120

/* The casts: a sphere (centre, w = radius), a ray / line / segment (pos,
 * dir) or a cylinder (centre, w = radius; dir) under the query and the
 * world matrix (DogModel passes unit ones). */
int   i3cCastSphere(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats);         // ROR 0x002324b8
int   i3cCastLine(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats);           // ROR 0x002324e0
int   i3cCastRay(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats);            // ROR 0x00232508
int   i3cCastSegment(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats);        // ROR 0x00232530
int   i3cCastCylinderLine(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats);   // ROR 0x00232558
int   i3cCastCylinderHalf(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats);   // ROR 0x00232580
int   i3cCastCylinder(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats);       // ROR 0x002325a8

/* A sphere (pos, w = r) moved along dir against a triangle, over 0..1
 * (Seg) or t >= 0 (Ray): the hit point with w = the sweep's t, the unit
 * plane with w = 1. */
int   i3cPrimSphereSweepSeg(const float *shape, const I3cTri *prim, const float *verts,
                            I3cQuery *q, int node, int both_face, const float *scale); // ROR 0x002325d0
int   i3cPrimSphereSweepRay(const float *shape, const I3cTri *prim, const float *verts,
                            I3cQuery *q, int node, int both_face, const float *scale); // ROR 0x00232be8
/* The sweep casts (row-scaled). No callers. */
int   i3cCastSphereSweepSeg(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats); // ROR 0x002331c8
int   i3cCastSphereSweepRay(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats); // ROR 0x002331f0

/* The inside-test epsilon (sdata 0x2db630, 0.001) and the row-scale switch
 * (0x2db634, 0) i3cCast reads. No callers in the ROM. */
void  i3cSetEpsilon(float eps);                                                     // ROR 0x00233218
float i3cGetEpsilon(void);                                                          // ROR 0x00233228
void  i3cSetScaleEnable(u_char on);                                                 // ROR 0x00233238
u_char i3cGetScaleEnable(void);                                                     // ROR 0x00233248

/* The barycentric weights of point in the triangle prim (indices into
 * verts): out.xyz, summing to 1. */
void  i3cTriBarycentric(float *out, const float *verts, const u_short *prim,
                        const float *point);                                        // ROR 0x00233258
/* A hit's attributes (call it from an I3cHitFunc with its arguments): the
 * triangle's three attribute records blended at point into out[5] (kind 1:
 * out[0..1] blended, out[2..4] = plane.xyz; 2: out[2..4] blended, out[0..1]
 * 0; 3: all five; else zeros). The kind; 0 when the part has none. No
 * callers. */
int   i3cGetHitAttr(float *out, const I3cQuery *q, const u_short *prim, const float *verts,
                    const float *plane, const float *point);                        // ROR 0x002333b0

#endif /* _I3D_I3CQUERY_H */
