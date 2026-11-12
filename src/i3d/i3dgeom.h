/* ==========================================================================
 *  i3d/i3dgeom.h
 *
 *  The i3d geometry tests (i3d/i3dgeom.cpp, ROR 0x233628..0x236960): the
 *  closest points of points, lines, rays and segments, the shape-vs-box
 *  tests the i3c casts use, and the shape-vs-shape hit tests utlHitCheck
 *  picks from.
 *
 *  Shapes are float[4] vectors (xyz; w the radius where there is one):
 *  - Line p + t d (t free), Ray (t >= 0), Seg (t 0..1), Point / Sphere p;
 *  - Cyl* is the shape grown by the radius in its start point's w: CylLine
 *    an infinite cylinder, CylHalf (= ray) and CylSeg a capsule;
 *  - Cylinder is the one around cyl + s axis (s 0..1, radius cyl.w).
 *  Distances are squared; out pointers may be NULL. The intersection tests
 *  return 0 for a miss, else 0x111 with bit 0 cleared when the entry t is
 *  before the shape's start and bit 4 when the exit t is past its end.
 * ======================================================================== */
#ifndef _I3D_I3DGEOM_H
#define _I3D_I3DGEOM_H

/* ---- distances ---- */

/* |a - b|^2 over xyz. */
float i3dPointDist2(const float *a, const float *b);                                // ROR 0x00233628
/* The squared distance from p to the line / ray / segment a + t d; t to *t. */
float i3dPointLineDist2(const float *p, const float *a, const float *d, float *t);  // ROR 0x00233650
float i3dPointRayDist2(const float *p, const float *a, const float *d, float *t);   // ROR 0x00233718
float i3dPointSegDist2(const float *p, const float *a, const float *d, float *t);   // ROR 0x002337e0
/* The closest points of p0 + s d0 (a line) and p1 + t d1 (a line / ray /
 * segment): s and t out, the squared distance back. */
float i3dLineLineDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                       float *s, float *t);                                         // ROR 0x002338c8
float i3dLineRayDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                      float *s, float *t);                                          // ROR 0x00233a58
float i3dLineSegDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                      float *s, float *t);                                          // ROR 0x00233c10
/* The same for a ray p0 + s d0 (s >= 0) against a ray (t >= 0) / a segment,
 * and for two segments (s, t 0..1). */
float i3dRayRayDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                     float *s, float *t);                                           // ROR 0x00233de8
float i3dRaySegDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                     float *s, float *t);                                           // ROR 0x00234050
float i3dSegSegDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                     float *s, float *t);                                           // ROR 0x00234390

/* ---- box tests ---- */

/* The casts' box tests: the shape's point relative to the box centre (w:
 * the radius), its direction, the box half extents; nonzero when they may
 * touch (i3dSphereBoxTest 1, the separating-axis tests -2). */
int   i3dSphereBoxTest(const float *rel, const float *dir, const float *half);      // ROR 0x002348f8
int   i3dSegBoxTest(const float *rel, const float *dir, const float *half);         // ROR 0x00234a50
int   i3dLineBoxTest(const float *rel, const float *dir, const float *half);        // ROR 0x00234ae8
int   i3dRayBoxTest(const float *rel, const float *dir, const float *half);         // ROR 0x00234b38
int   i3dCylSegBoxTest(const float *rel, const float *dir, const float *half);      // ROR 0x00234be8
int   i3dCylHalfBoxTest(const float *rel, const float *dir, const float *half);     // ROR 0x00234c30
int   i3dCylLineBoxTest(const float *rel, const float *dir, const float *half);     // ROR 0x00234ce8

/* ---- hit tests: 1 when the shapes touch ---- */

int   i3dSphereSphereTest(const float *a, const float *b);                          // ROR 0x00234d40
int   i3dSphereLineTest(const float *sph, const float *p, const float *d);          // ROR 0x00234d80
int   i3dSphereRayTest(const float *sph, const float *p, const float *d);           // ROR 0x00234dc0
int   i3dSphereSegTest(const float *sph, const float *p, const float *d);           // ROR 0x00234e00
int   i3dSphereCylLineTest(const float *sph, const float *p, const float *d);       // ROR 0x00234e40
int   i3dSphereCylHalfTest(const float *sph, const float *p, const float *d);       // ROR 0x00234e88
int   i3dSphereCylSegTest(const float *sph, const float *p, const float *d);        // ROR 0x00234ed0
/* A capsule p0 + s d0 (radius p0.w) against a line / ray / segment p1 + t d1,
 * then against a grown one (radius p1.w too). */
int   i3dCylSegLineTest(const float *p0, const float *d0, const float *p1, const float *d1);    // ROR 0x00234f18
int   i3dCylSegRayTest(const float *p0, const float *d0, const float *p1, const float *d1);     // ROR 0x00234f70
int   i3dCylSegSegTest(const float *p0, const float *d0, const float *p1, const float *d1);     // ROR 0x00234fc8
int   i3dCylSegCylLineTest(const float *p0, const float *d0, const float *p1, const float *d1); // ROR 0x00235020
int   i3dCylSegCylHalfTest(const float *p0, const float *d0, const float *p1, const float *d1); // ROR 0x00235080
int   i3dCylSegCylSegTest(const float *p0, const float *d0, const float *p1, const float *d1);  // ROR 0x002350e0
/* The flat-capped cylinder p..p + d (radius p.w) against a sphere. */
int   i3dCylinderSphereTest(const float *p, const float *d, const float *sph);      // ROR 0x002354b8
/* The flat-capped cylinder cyl..cyl + axis (radius cyl.w) against a line /
 * ray / segment p + t d, then against a grown ray / segment (radius p.w:
 * the cylinder grows by it, its height by twice it). */
int   i3dFlatCylinderLineTest(const float *cyl, const float *axis, const float *p, const float *d);   // ROR 0x002355c8
int   i3dFlatCylinderRayTest(const float *cyl, const float *axis, const float *p, const float *d);    // ROR 0x00235608
int   i3dFlatCylinderSegTest(const float *cyl, const float *axis, const float *p, const float *d);    // ROR 0x00235648
int   i3dFlatCylinderCylLineTest(const float *cyl, const float *axis, const float *p, const float *d); // ROR 0x00236348
int   i3dFlatCylinderCylHalfTest(const float *cyl, const float *axis, const float *p, const float *d); // ROR 0x00236400
int   i3dFlatCylinderCylSegTest(const float *cyl, const float *axis, const float *p, const float *d);  // ROR 0x002364f0

/* ---- intersections: entry / exit t to t0 / t1 ---- */

/* A line / ray / segment p + t d against a sphere (the Cyl* forms grow the
 * sphere by p.w). */
int   i3dLineSphereTest(const float *p, const float *d, const float *sph, float *t0, float *t1);    // ROR 0x00235128
int   i3dRaySphereTest(const float *p, const float *d, const float *sph, float *t0, float *t1);     // ROR 0x00235228
int   i3dSegSphereTest(const float *p, const float *d, const float *sph, float *t0, float *t1);     // ROR 0x002352a8
int   i3dCylSegSphereTest(const float *p, const float *d, const float *sph, float *t0, float *t1);  // ROR 0x00235350
int   i3dCylLineSphereTest(const float *p, const float *d, const float *sph, float *t0, float *t1); // ROR 0x002353c8
int   i3dCylHalfSphereTest(const float *p, const float *d, const float *sph, float *t0, float *t1); // ROR 0x00235440

/* Two unit vectors u, v perpendicular to the unit vector n (xyz only). */
void  i3dVecOrthoBasis(float *u, float *v, const float *n);                         // ROR 0x00235688

/* A line / ray / segment p + t d against the capsule around cyl + s axis
 * (s 0..1, radius cyl.w); i3dCylLineCylinderTest grows it by p.w. */
int   i3dLineCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                          float *t0, float *t1);                                    // ROR 0x00235be8
int   i3dRayCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                         float *t0, float *t1);                                     // ROR 0x002365f8
int   i3dSegCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                         float *t0, float *t1);                                     // ROR 0x00236678
int   i3dCylLineCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                             float *t0, float *t1);                                 // ROR 0x00236718
int   i3dCylHalfCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                             float *t0, float *t1);                                 // ROR 0x002367a0
int   i3dCylSegCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                            float *t0, float *t1);                                  // ROR 0x00236870

/* A line p + t d against the flat-capped cylinder of height h and radius r
 * centred on centre along axis; then the one around cyl..cyl + axis
 * (radius cyl.w) and its ray / segment forms. */
int   i3dLineFlatCylinderCoreTest(float h, float r, const float *p, const float *d,
                                  const float *centre, const float *axis,
                                  float *t0, float *t1);                            // ROR 0x00235768
int   i3dLineFlatCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                              float *t0, float *t1);                                // ROR 0x00236168
int   i3dRayFlatCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                             float *t0, float *t1);                                 // ROR 0x00236228
int   i3dSegFlatCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                             float *t0, float *t1);                                 // ROR 0x002362a8

#endif /* _I3D_I3DGEOM_H */
