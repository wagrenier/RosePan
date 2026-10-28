/* ==========================================================================
 *  i3d/i3dview.h
 *
 *  The i3d view (i3d/i3dview.cpp, ROR 0x226278..0x227a98): what a layer
 *  draws through (i3dLayerSetView). i3dCtxCreateView turns a view record
 *  (frustum, depth range, GS screen rectangle and offset) and a camera
 *  matrix into a 0x400-byte block in the context tail -- the world ->
 *  screen and world -> clip matrices VU1 reads, the same pair for the GS
 *  guard band and for the screen rectangle, plane and depth matrices for
 *  the culling tests -- and a small packet (REF of the first 10 qwords,
 *  SCISSOR_1, the eye direction) the layer CALLs (i3dPktCallView).
 *
 *  Three frusta: the guard band (the frustum widened to screen x / y
 *  64..4031.94), the screen rectangle itself, and the draw one between them
 *  (the context's m_float_param, 0.35 in RoR, of the way to the guard band).
 *
 *  Matrices are row-vector: a point p (x, y, z, 1) maps to p * M
 *  (i3dVu0MatMul's order).
 * ======================================================================== */
#ifndef _I3D_I3DVIEW_H
#define _I3D_I3DVIEW_H

#include <stddef.h>

#include "i3d/i3dctx.h"

/* The view record i3dCtxCreateView reads (mag/magscene.h). */
struct MagViewParam;

/* A view. EE 0x400 (0x40 qwords of the context tail); plain data, so the
 * EE layout holds on the host. */
struct I3dView
{
    sceVu0FMATRIX m_world_screen;       // ROR 0x000: camera * proj * viewport (VU1, REF'd)
    sceVu0FMATRIX m_world_clip;         // ROR 0x040: camera * proj (VU1)
    sceVu0FVECTOR m_screen_scale;       // ROR 0x080: m_viewport's diagonal (VU1)
    sceVu0FVECTOR m_screen_offset;      // ROR 0x090: m_viewport's translation (VU1)
    sceVu0FMATRIX m_viewport;           // ROR 0x0a0: clip -> screen, the draw frustum
    sceVu0FMATRIX m_camera;             // ROR 0x0e0: the matrix given (world -> camera)
    sceVu0FMATRIX m_proj;               // ROR 0x120: the draw frustum's projection
    sceVu0FMATRIX m_guard_proj;         // ROR 0x160: the guard band's projection
    sceVu0FMATRIX m_guard_clip;         // ROR 0x1a0: camera * m_guard_proj
    sceVu0FMATRIX m_guard_viewport;     // ROR 0x1e0
    sceVu0FMATRIX m_rect_proj;          // ROR 0x220: the screen rectangle's projection
    sceVu0FMATRIX m_rect_clip;          // ROR 0x260: camera * m_rect_proj
    sceVu0FMATRIX m_rect_viewport;      // ROR 0x2a0
    sceVu0FMATRIX m_rect_planes;        // ROR 0x2e0: world -> distances to the rectangle's 4 side planes
    sceVu0FMATRIX m_guard_planes;       // ROR 0x320: the same for the guard band
    sceVu0FMATRIX m_depth;              // ROR 0x360: world -> (z - near, far - z, (z - near) / (far - near), 1)
    sceVu0FMATRIX m_inverse;            // ROR 0x3a0: camera -> world (row 2 at 0x3c0: the view axis)
    sceVu0FVECTOR m_eye_dir;            // ROR 0x3e0: -normalize(m_inverse[2])
    u_long        m_scissor;            // ROR 0x3f0: SCISSOR_1 from the record's rectangle
    u_int         m_packet;             // ROR 0x3f8: DMA address of the view's packet (low 28 bits)
    u_int         _3fc;                 // ROR 0x3fc: unused
};
static_assert(offsetof(I3dView, m_world_clip) == 0x040, "m_world_clip");
static_assert(offsetof(I3dView, m_screen_scale) == 0x080, "m_screen_scale");
static_assert(offsetof(I3dView, m_screen_offset) == 0x090, "m_screen_offset");
static_assert(offsetof(I3dView, m_viewport) == 0x0a0, "m_viewport");
static_assert(offsetof(I3dView, m_camera) == 0x0e0, "m_camera");
static_assert(offsetof(I3dView, m_proj) == 0x120, "m_proj");
static_assert(offsetof(I3dView, m_guard_proj) == 0x160, "m_guard_proj");
static_assert(offsetof(I3dView, m_guard_clip) == 0x1a0, "m_guard_clip");
static_assert(offsetof(I3dView, m_guard_viewport) == 0x1e0, "m_guard_viewport");
static_assert(offsetof(I3dView, m_rect_proj) == 0x220, "m_rect_proj");
static_assert(offsetof(I3dView, m_rect_clip) == 0x260, "m_rect_clip");
static_assert(offsetof(I3dView, m_rect_viewport) == 0x2a0, "m_rect_viewport");
static_assert(offsetof(I3dView, m_rect_planes) == 0x2e0, "m_rect_planes");
static_assert(offsetof(I3dView, m_guard_planes) == 0x320, "m_guard_planes");
static_assert(offsetof(I3dView, m_depth) == 0x360, "m_depth");
static_assert(offsetof(I3dView, m_inverse) == 0x3a0, "m_inverse");
static_assert(offsetof(I3dView, m_eye_dir) == 0x3e0, "m_eye_dir");
static_assert(offsetof(I3dView, m_scissor) == 0x3f0, "m_scissor");
static_assert(offsetof(I3dView, m_packet) == 0x3f8, "m_packet");
static_assert(sizeof(I3dView) == 0x400, "I3dView is 0x400 bytes on the EE");

/* The bones i3dViewTestBoneBox bounds: a skinned (type 0x46) part's data,
 * from the i3r file: plain EE layout, relocated fields read through
 * i3rPtr. m_count boxes (0x40 each, rows = three half axes and the centre)
 * in the space of the matrix each one's index picks. */
struct I3dBoneBoxes
{
    u_int          m_offsets;           // ROR 0x00: relocated: the per-slot offset matrices (0x225ec8)
    u_int          m_boxes;             // ROR 0x04: relocated: m_count * 4 rows
    u_char         _08[6];              // ROR 0x08: unverified
    u_short        m_count;             // ROR 0x0e
};
static_assert(offsetof(I3dBoneBoxes, m_boxes) == 0x04, "I3dBoneBoxes::m_boxes");
static_assert(offsetof(I3dBoneBoxes, m_count) == 0x0e, "I3dBoneBoxes::m_count");

/* The culling tests' answers. */
#define I3D_VIEW_OUT    0   /* outside: nothing to draw */
#define I3D_VIEW_IN     1   /* inside the guard band: no clipping needed */
#define I3D_VIEW_CLIP   2   /* crosses the guard band (or near / far): clip */

void     i3dViewMakePlanes(sceVu0FVECTOR *out, sceVu0FVECTOR *matrix, const float *lb,
                           const float *lt, const float *rb, const float *rt); // ROR 0x00226278
sceVu0FVECTOR *i3dViewGetCamera(I3dView *view);                          // ROR 0x00226370
sceVu0FVECTOR *i3dViewGetRectProj(I3dView *view);                        // ROR 0x00226380
sceVu0FVECTOR *i3dViewGetRectViewport(I3dView *view);                    // ROR 0x00226390
sceVu0FVECTOR *i3dViewGetInverse(I3dView *view);                         // ROR 0x002263a0
float    i3dLerpf(float a, float b, float t);                            // ROR 0x002263b0
void     i3dRemapPair(float a, float x0, float x1, float d, float v0, float v1,
                      float *out_a, float *out_d);                       // ROR 0x002263c0
void     i3dMatMapBox(const float *a, const float *b, const float *c, const float *d,
                      sceVu0FVECTOR *out);                               // ROR 0x00226538
void     i3dViewMakePerspective(const float *min, const float *max, const float *ndc_min,
                                const float *ndc_max, const float *scr_min, const float *scr_max,
                                sceVu0FVECTOR *proj, sceVu0FVECTOR *viewport); // ROR 0x00226628
void     i3dViewMakeOrtho(const float *min, const float *max, const float *ndc_min,
                          const float *ndc_max, const float *scr_min, const float *scr_max,
                          sceVu0FVECTOR *proj, sceVu0FVECTOR *viewport); // ROR 0x002267e8
int      i3dViewTestBox(I3dView *view, sceVu0FVECTOR *box);              // ROR 0x002268b0
int      i3dViewTestBoneBox(I3dView *view, const u_int *index, const I3dBoneBoxes *bones,
                            sceVu0FVECTOR *space, sceVu0FMATRIX *matrices);  // ROR 0x00226920
int      i3dViewTestSphere(I3dView *view, const float *center, float radius, float *depth); // ROR 0x00226ad8
float    i3dViewGetDepth(I3dView *view, const float *point);             // ROR 0x00226c18
I3dView *i3dCtxCreateView(i3dContext *ctx, MagViewParam *param, sceVu0FVECTOR *matrix); // ROR 0x00226c58
I3dView *i3dCtxCreateOrthoView(i3dContext *ctx, MagViewParam *param, sceVu0FVECTOR *matrix); // ROR 0x002273d8

#endif /* _I3D_I3DVIEW_H */
