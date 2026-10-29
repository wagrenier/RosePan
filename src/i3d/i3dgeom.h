/* ==========================================================================
 *  i3d/i3dgeom.h
 *
 *  The i3d geometry helpers (i3d/i3dgeom.cpp, ROR 0x233628..): point,
 *  line and segment distances, and the shape-vs-box tests the i3c casts
 *  use. Minimal: what i3d/i3cquery.cpp calls (i3dSegSegDist2 and
 *  i3dPointSegDist2 are still declared in lib/unassigned.h).
 * ======================================================================== */
#ifndef _I3D_I3DGEOM_H
#define _I3D_I3DGEOM_H

/* |a - b|^2 over xyz. */
float i3dPointDist2(const float *a, const float *b);                                // ROR 0x00233628
/* The closest points of the line p0 + s d0 (s free) and the segment
 * p1 + t d1 (t 0..1); either out pointer may be NULL. The squared distance. */
float i3dLineSegDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                      float *s, float *t);                                          // ROR 0x00233c10
/* Segment-segment closest points by region (s, t 0..1). */
float i3dSegSegDist2Rgn(const float *p0, const float *d0, const float *p1, const float *d1,
                        float *s, float *t);                                        // ROR 0x00234390

/* A line p + t d against the cylinder of radius cyl.w around cyl.xyz + s
 * axis (s 0..1), limited to a ray (t >= 0) / a segment (t 0..1): 0 a miss,
 * else flags (bit 0 flipped when the entry t is before the start, bit 4
 * when the exit t is past the end); the entry / exit t to t0 / t1 (either
 * may be NULL). */
int   i3dRayCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                         float *t0, float *t1);                                     // ROR 0x002365f8
int   i3dSegCylinderTest(const float *p, const float *d, const float *cyl, const float *axis,
                         float *t0, float *t1);                                     // ROR 0x00236678

/* The casts' box tests: the shape's point relative to the box centre (w:
 * the radius), its direction, the box half extents; nonzero when they may
 * touch. */
int   i3dSphereBoxTest(const float *rel, const float *dir, const float *half);      // ROR 0x002348f8
int   i3dSegBoxTest(const float *rel, const float *dir, const float *half);         // ROR 0x00234a50
int   i3dLineBoxTest(const float *rel, const float *dir, const float *half);        // ROR 0x00234ae8
int   i3dRayBoxTest(const float *rel, const float *dir, const float *half);         // ROR 0x00234b38
int   i3dCylSegBoxTest(const float *rel, const float *dir, const float *half);      // ROR 0x00234be8
int   i3dCylHalfBoxTest(const float *rel, const float *dir, const float *half);     // ROR 0x00234c30
int   i3dCylLineBoxTest(const float *rel, const float *dir, const float *half);     // ROR 0x00234ce8

#endif /* _I3D_I3DGEOM_H */
