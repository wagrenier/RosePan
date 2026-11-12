/* ==========================================================================
 *  i3d/i3dview.cpp
 *
 *  The i3d view (ROR 0x226278..0x227a98): the frustum and viewport matrix
 *  builders, i3dCtxCreateView and its orthographic twin, the view's
 *  accessors and its culling tests (a box, a set of bone boxes, a sphere).
 *  See i3dview.h. The two view builders are two full copies in the ROM and
 *  stay so here.
 *
 *  The ROM does the box tests in VU0 microprograms (data 0x2949f0: micro
 *  0x69 / 0x96 over the subroutine at 200) and the sphere test with VU0
 *  macro code that reads the MAC sign flags; here they are scalar C with
 *  the same results (VU CLIP flag bits, sign tests).
 *
 *  PORT: the view's packet carries the ROM's VIF / GIF codes; the host's
 *  DMA does not follow chains and nothing on the host consumes them yet
 *  (no GS emulation), so it is kept only for i3dPktCallView's address.
 * ======================================================================== */
#include "i3d/i3dview.h"

#include <math.h>
#include <stdint.h>

#include "i3d/i3rroot.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "rosepan/rendering/rosepan_renderer.h"

/* The GS guard band the projections widen to, in screen units (x / y). */
#define I3D_GUARD_MIN 64.0f
#define I3D_GUARD_MAX 4031.9375f

#define I3D_GS_SCISSOR_1 0x40ull

/* i3dCtxCreateView's copy of a view record's screen part (record +0x18..
 * +0x34), the depth range scaled by 1/16. */
struct I3dViewScreen
{
    float m_zmax;               // record 0x18 / 16: the screen z at the near plane
    float m_zmin;               // record 0x1c / 16: at the far plane
    float m_x0;
    float m_x1;
    float m_y0;
    float m_y1;
    float m_offx;
    float m_offy;
};
static_assert(sizeof(I3dViewScreen) == 0x20, "I3dViewScreen: the ROM's 0x20-byte stack copy");

/* ... and of its frustum part (record +0x00..+0x14). */
struct I3dViewFrustum
{
    float m_near;
    float m_far;
    float m_left;
    float m_right;
    float m_bottom;
    float m_top;
};
static_assert(sizeof(I3dViewFrustum) == 0x18, "I3dViewFrustum: the ROM's 0x18-byte stack copy");

static inline void i3dVecSet(sceVu0FVECTOR v, float x, float y, float z)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

/* The VU CLIP judgement of v against |v.w|: +x 1, -x 2, +y 4, -y 8,
 * +z 0x10, -z 0x20. */
static int i3dClipFlags(const float *v)
{
    float w = fabsf(v[3]);
    int flags = 0;

    if (v[0] > w)  flags |= 0x01;
    if (v[0] < -w) flags |= 0x02;
    if (v[1] > w)  flags |= 0x04;
    if (v[1] < -w) flags |= 0x08;
    if (v[2] > w)  flags |= 0x10;
    if (v[2] < -w) flags |= 0x20;
    return flags;
}

/* The eight corners of box (rows: three half axes, the centre) times m
 * (VU0 micro 200 and the corner sums of micros 0x69 / 0x96). */
static void i3dBoxCorners(sceVu0FVECTOR *m, sceVu0FVECTOR *box, sceVu0FVECTOR corner[8])
{
    sceVu0FMATRIX bm;

    i3dVu0MatMul(bm, m, box);
    for (int i = 0; i < 8; i++)
    {
        float s0 = (i & 1) ? 1.0f : -1.0f;
        float s1 = (i & 2) ? 1.0f : -1.0f;
        float s2 = (i & 4) ? 1.0f : -1.0f;
        for (int k = 0; k < 4; k++)
            corner[i][k] = bm[3][k] + s0 * bm[0][k] + s1 * bm[1][k] + s2 * bm[2][k];
    }
}

/* p * m, field k (p with its w). */
static inline float i3dDot4(const float *p, sceVu0FVECTOR *m, int k)
{
    return p[0] * m[0][k] + p[1] * m[1][k] + p[2] * m[2][k] + p[3] * m[3][k];
}

// ──────────────────────────────────────────────────────────────────────
// The side planes of the frustum through the eye and its near-plane
// corners (bottom, right, top, left), inward normals as the columns of a
// matrix, times the camera matrix: world point -> four plane distances.
// ROR 0x00226278

void i3dViewMakePlanes(sceVu0FVECTOR *out, sceVu0FVECTOR *matrix, const float *lb,
                       const float *lt, const float *rb, const float *rt)
{
    sceVu0FVECTOR n[4];
    sceVu0FMATRIX m;

    i3dVecCross(n[0], lb, rb);
    i3dVecCross(n[1], rb, rt);
    i3dVecCross(n[2], rt, lt);
    i3dVecCross(n[3], lt, lb);
    for (int i = 0; i < 4; i++)
    {
        i3dVecNormalize(n[i], n[i]);
        m[0][i] = n[i][0];
        m[1][i] = n[i][1];
        m[2][i] = n[i][2];
        m[3][i] = 0.0f;
    }
    i3dVu0MatMul(out, m, matrix);
}

// ROR 0x00226370
sceVu0FVECTOR *i3dViewGetCamera(I3dView *view)
{
    return view->m_camera;
}

// ROR 0x00226380
sceVu0FVECTOR *i3dViewGetRectProj(I3dView *view)
{
    return view->m_rect_proj;
}

// ROR 0x00226390
sceVu0FVECTOR *i3dViewGetRectViewport(I3dView *view)
{
    return view->m_rect_viewport;
}

// ROR 0x002263a0
sceVu0FVECTOR *i3dViewGetInverse(I3dView *view)
{
    return view->m_inverse;
}

// ROR 0x002263b0
float i3dLerpf(float a, float b, float t)
{
    return a + (b - a) * t;
}

// ──────────────────────────────────────────────────────────────────────
// The linear function that is v0 at x0 and v1 at x1, at a and at d.
// ROR 0x002263c0

void i3dRemapPair(float a, float x0, float x1, float d, float v0, float v1,
                  float *out_a, float *out_d)
{
    float inv = 1.0f / (x1 - x0);

    *out_a = ((x1 - a) * v0 + (a - x0) * v1) * inv;
    *out_d = ((x1 - d) * v0 + (d - x0) * v1) * inv;
}

// ──────────────────────────────────────────────────────────────────────
// Widens the screen rectangle to at least half a pixel each way.
// ROR 0x00226410

static void i3dViewFixRect(I3dViewScreen *scr)
{
    float mid;

    if (scr->m_x1 < scr->m_x0 + 0.5f)
    {
        mid = (scr->m_x0 + scr->m_x1) * 0.5f;
        scr->m_x1 = mid + 0.25f;
        scr->m_x0 = mid - 0.25f;
    }
    if (scr->m_y1 < scr->m_y0 + 0.5f)
    {
        mid = (scr->m_y0 + scr->m_y1) * 0.5f;
        scr->m_y1 = mid + 0.25f;
        scr->m_y0 = mid - 0.25f;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Widens the frustum's sides to at least near / 10000 each way.
// ROR 0x00226498

static void i3dViewFixFrustum(I3dViewFrustum *fr)
{
    float min = fr->m_near * 0.0001f;
    float mid;

    if (fr->m_right < fr->m_left + min)
    {
        mid = (fr->m_left + fr->m_right) * 0.5f;
        fr->m_right = mid + min * 0.5f;
        fr->m_left = mid - min * 0.5f;
    }
    if (fr->m_top < fr->m_bottom + min)
    {
        mid = (fr->m_bottom + fr->m_top) * 0.5f;
        fr->m_top = mid + min * 0.5f;
        fr->m_bottom = mid - min * 0.5f;
    }
}

// ROR 0x00226530
static void i3dViewFixOrthoFrustum(I3dViewFrustum *fr)
{
    (void)fr;
}

// ──────────────────────────────────────────────────────────────────────
// The matrix that maps box a..b onto box c..d axis by axis.
// ROR 0x00226538

void i3dMatMapBox(const float *a, const float *b, const float *c, const float *d,
                  sceVu0FVECTOR *out)
{
    float ix = 1.0f / (a[0] - b[0]);
    float iy = 1.0f / (a[1] - b[1]);
    float iz = 1.0f / (a[2] - b[2]);

    out[3][3] = 1.0f;
    out[0][1] = 0.0f;
    out[0][2] = 0.0f;
    out[0][3] = 0.0f;
    out[1][0] = 0.0f;
    out[1][2] = 0.0f;
    out[1][3] = 0.0f;
    out[2][0] = 0.0f;
    out[2][1] = 0.0f;
    out[2][3] = 0.0f;
    out[3][0] = (a[0] * d[0] - b[0] * c[0]) * ix;
    out[0][0] = (c[0] - d[0]) * ix;
    out[3][1] = (a[1] * d[1] - b[1] * c[1]) * iy;
    out[1][1] = (c[1] - d[1]) * iy;
    out[3][2] = (a[2] * d[2] - b[2] * c[2]) * iz;
    out[2][2] = (c[2] - d[2]) * iz;
}

// ──────────────────────────────────────────────────────────────────────
// A perspective projection of the frustum min (left, bottom, near) .. max
// (right, top, far) onto the NDC box, and the NDC -> screen matrix; either
// output may be NULL.
// ROR 0x00226628

void i3dViewMakePerspective(const float *min, const float *max, const float *ndc_min,
                            const float *ndc_max, const float *scr_min, const float *scr_max,
                            sceVu0FVECTOR *proj, sceVu0FVECTOR *viewport)
{
    if (proj != NULL)
    {
        float l = min[0], b = min[1], n = min[2];
        float r = max[0], t = max[1], f = max[2];
        float ix = 1.0f / (l - r);
        float iy = 1.0f / (b - t);
        float iz = 1.0f / (n - f);

        proj[2][3] = 1.0f;
        proj[0][1] = 0.0f;
        proj[0][2] = 0.0f;
        proj[0][3] = 0.0f;
        proj[1][0] = 0.0f;
        proj[1][2] = 0.0f;
        proj[1][3] = 0.0f;
        proj[3][0] = 0.0f;
        proj[3][1] = 0.0f;
        proj[3][3] = 0.0f;
        proj[2][0] = (l * ndc_max[0] - r * ndc_min[0]) * ix;
        proj[0][0] = n * (ndc_min[0] - ndc_max[0]) * ix;
        proj[2][1] = (b * ndc_max[1] - t * ndc_min[1]) * iy;
        proj[1][1] = n * (ndc_min[1] - ndc_max[1]) * iy;
        proj[2][2] = (ndc_min[2] * n - ndc_max[2] * f) * iz;
        proj[3][2] = n * f * (ndc_max[2] - ndc_min[2]) * iz;
    }
    if (viewport != NULL)
    {
        sceVu0FVECTOR s0, s1;
        float dz;

        /* The screen z range widened by 1/4096 of itself each way. */
        i3dVecCopyW(s0, scr_min);
        i3dVecCopyW(s1, scr_max);
        dz = (s0[2] - s1[2]) * (1.0f / 4096.0f);
        s1[2] += dz;
        s0[2] -= dz;
        i3dMatMapBox(ndc_min, ndc_max, s0, s1, viewport);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The orthographic twin: the box min .. max mapped onto the NDC box.
// ROR 0x002267e8

void i3dViewMakeOrtho(const float *min, const float *max, const float *ndc_min,
                      const float *ndc_max, const float *scr_min, const float *scr_max,
                      sceVu0FVECTOR *proj, sceVu0FVECTOR *viewport)
{
    if (proj != NULL)
        i3dMatMapBox(min, max, ndc_min, ndc_max, proj);
    if (viewport != NULL)
    {
        sceVu0FVECTOR s0, s1;
        float dz;

        /* The screen z range widened by 1/4096 of itself each way. */
        i3dVecCopyW(s0, scr_min);
        i3dVecCopyW(s1, scr_max);
        dz = (s0[2] - s1[2]) * (1.0f / 4096.0f);
        s1[2] += dz;
        s0[2] -= dz;
        i3dMatMapBox(ndc_min, ndc_max, s0, s1, viewport);
    }
}

// ──────────────────────────────────────────────────────────────────────
// A box (rows: three half axes and the centre, in world space) against the
// view: OUT when all its corners lie outside one plane of the screen
// rectangle's clip volume (micro 0x96), IN when every corner is inside the
// guard band's (micro 0x69), CLIP otherwise.
// ROR 0x002268b0

int i3dViewTestBox(I3dView *view, sceVu0FVECTOR *box)
{
    sceVu0FVECTOR corner[8];
    int all = 0x3f, any = 0;

    i3dBoxCorners(view->m_rect_clip, box, corner);
    for (int i = 0; i < 8; i++)
        all &= i3dClipFlags(corner[i]);
    if (all != 0)
        return I3D_VIEW_OUT;

    i3dBoxCorners(view->m_guard_clip, box, corner);
    for (int i = 0; i < 8; i++)
        any |= i3dClipFlags(corner[i]);
    return any != 0 ? I3D_VIEW_CLIP : I3D_VIEW_IN;
}

// ──────────────────────────────────────────────────────────────────────
// The bounds of a skinned part: each bone box times the matrix its index
// picks, their axis-aligned bounds made a box in the space of `space`,
// then i3dViewTestBox.
// ROR 0x00226920

int i3dViewTestBoneBox(I3dView *view, const u_int *index, const I3dBoneBoxes *bones,
                       sceVu0FVECTOR *space, sceVu0FMATRIX *matrices)
{
    // PC: index is the entry's relocated bone-slot word (I3rNodeEntryData
    // m_bones), the boxes a relocated field of the part's data.
    const u_short *idx = (const u_short *)i3rPtr(index);
    sceVu0FVECTOR *boxes = (sceVu0FVECTOR *)i3rPtr(&bones->m_boxes);
    // PC: with no boxes the ROM bounds whatever vf12..vf15 held; the host
    // bounds the origin.
    float mn[3] = { 0.0f, 0.0f, 0.0f }, mx[3] = { 0.0f, 0.0f, 0.0f };
    sceVu0FMATRIX box;

    for (u_int i = 0; i < bones->m_count; i++)
    {
        sceVu0FMATRIX bm;

        i3dVu0MatMul(bm, matrices[idx[i]], &boxes[i * 4]);
        if (i == 0)
        {
            for (int k = 0; k < 3; k++)
                mn[k] = mx[k] = bm[3][k];
        }
        for (int c = 0; c < 8; c++)
        {
            float s0 = (c & 1) ? 1.0f : -1.0f;
            float s1 = (c & 2) ? 1.0f : -1.0f;
            float s2 = (c & 4) ? 1.0f : -1.0f;
            for (int k = 0; k < 3; k++)
            {
                float v = bm[3][k] + s0 * bm[0][k] + s1 * bm[1][k] + s2 * bm[2][k];
                if (v < mn[k]) mn[k] = v;
                if (v > mx[k]) mx[k] = v;
            }
        }
    }

    float half[3], mid[3];
    for (int k = 0; k < 3; k++)
    {
        half[k] = (mx[k] - mn[k]) * 0.5f;
        mid[k] = (mx[k] + mn[k]) * 0.5f;
    }
    for (int k = 0; k < 4; k++)
    {
        box[0][k] = space[0][k] * half[0];
        box[1][k] = space[1][k] * half[1];
        box[2][k] = space[2][k] * half[2];
        box[3][k] = space[0][k] * mid[0] + space[1][k] * mid[1] + space[2][k] * mid[2] + space[3][k];
    }
    return i3dViewTestBox(view, box);
}

// ──────────────────────────────────────────────────────────────────────
// A sphere against the view: OUT when it lies outside a side plane of the
// screen rectangle or beyond near / far; else its depth to *depth (when
// given), and CLIP when it crosses near / far or a guard band plane, IN
// otherwise. No caller in RoR.
// ROR 0x00226ad8

int i3dViewTestSphere(I3dView *view, const float *center, float radius, float *depth)
{
    for (int k = 0; k < 4; k++)
    {
        if (i3dDot4(center, view->m_rect_planes, k) + radius < 0.0f)
            return I3D_VIEW_OUT;
    }
    float to_near = i3dDot4(center, view->m_depth, 0);
    float to_far = i3dDot4(center, view->m_depth, 1);
    if (to_near + radius < 0.0f || to_far + radius < 0.0f)
        return I3D_VIEW_OUT;
    if (depth != NULL)
        *depth = i3dDot4(center, view->m_depth, 2);
    if (to_near - radius < 0.0f || to_far - radius < 0.0f)
        return I3D_VIEW_CLIP;
    for (int k = 0; k < 4; k++)
    {
        if (i3dDot4(center, view->m_guard_planes, k) - radius < 0.0f)
            return I3D_VIEW_CLIP;
    }
    return I3D_VIEW_IN;
}

// ──────────────────────────────────────────────────────────────────────
// A point's depth, 0 at near and 1 at far.
// ROR 0x00226c18

float i3dViewGetDepth(I3dView *view, const float *point)
{
    return i3dDot4(point, view->m_depth, 2);
}

/* PC: how much wider / taller than the record's rectangle the host shows a
 * view: rosepan_i3d.cpp widens the camera of every pass drawn to the display
 * (XYOFFSET not 0) by the renderer's view extend, so more world shows on a
 * wide window. The off-screen views (the shadow target's light camera, the
 * close-up) have a record offset of 0 and keep their shape. Never below 1: a
 * picture band's zoom only narrows what shows, so the ROM's cull is
 * conservative there. */
static void i3dViewHostExtend(const MagViewParam *param, float *ex, float *ey)
{
    *ex = 1.0f;
    *ey = 1.0f;
    if (param->m_offx == 0.0f && param->m_offy == 0.0f)
        return;
    RosePan_RendererGetViewExtend(ex, ey);
    if (*ex < 1.0f)
        *ex = 1.0f;
    if (*ey < 1.0f)
        *ey = 1.0f;
}

/* The SCISSOR_1 value of a screen rectangle (the ROM converts with cvt.w.s
 * and or-s the sign-extended words). */
static u_long i3dScissor(float x0, float x1, float y0, float y1)
{
    return (u_long)(int64_t)(int)x0 | ((u_long)(int64_t)(int)x1 << 16)
         | ((u_long)(int64_t)(int)y0 << 32) | ((u_long)(int64_t)(int)y1 << 48);
}

// ──────────────────────────────────────────────────────────────────────
// A view from a view record and a camera (world -> camera) matrix.
// ROR 0x00226c58

I3dView *i3dCtxCreateView(i3dContext *ctx, MagViewParam *param, sceVu0FVECTOR *matrix)
{
    I3dView *view = (I3dView *)i3dCtxAllocTail(ctx, I3D_QWC_OF(I3dView));
    sceVu0FVECTOR mn, mx, ndc_min, ndc_max, scr_min, scr_max;
    sceVu0FVECTOR lb, lt, rb, rt;
    sceVu0FMATRIX depth;
    float gx0, gx1, gy0, gy1;

    i3dMatCopy(view->m_camera, matrix);

    I3dViewScreen scr = {
        param->m_zmax * 0.0625f, param->m_zmin * 0.0625f,
        param->m_x0, param->m_x1, param->m_y0, param->m_y1, param->m_offx, param->m_offy,
    };
    I3dViewFrustum fr = {
        param->m_near, param->m_far, param->m_left, param->m_right, param->m_bottom, param->m_top,
    };
    /* The scissor takes the record's rectangle before the widening. */
    I3dViewScreen scissor = scr;

    i3dViewFixRect(&scr);
    i3dViewFixFrustum(&fr);

    float x0 = scr.m_x0 + scr.m_offx, x1 = scr.m_x1 + scr.m_offx;
    float y0 = scr.m_y0 + scr.m_offy, y1 = scr.m_y1 + scr.m_offy;
    i3dRemapPair(I3D_GUARD_MIN, x0, x1, I3D_GUARD_MAX, fr.m_left, fr.m_right, &gx0, &gx1);
    i3dRemapPair(I3D_GUARD_MIN, y0, y1, I3D_GUARD_MAX, fr.m_bottom, fr.m_top, &gy0, &gy1);

    /* NDC: the rectangle less a pixel on each side. */
    float sx = (scr.m_x1 - scr.m_x0) / ((scr.m_x1 - scr.m_x0) + 2.0f);
    float sy = (scr.m_y1 - scr.m_y0) / ((scr.m_y1 - scr.m_y0) + 2.0f);
    i3dVecSet(ndc_min, -sx, -sy, -1.0f);
    i3dVecSet(ndc_max, sx, sy, 1.0f);

    /* The guard band. */
    i3dVecSet(mn, gx0, gy0, fr.m_near);
    i3dVecSet(mx, gx1, gy1, fr.m_far);
    i3dVecSet(scr_min, I3D_GUARD_MIN, I3D_GUARD_MIN, scr.m_zmax);
    i3dVecSet(scr_max, I3D_GUARD_MAX, I3D_GUARD_MAX, scr.m_zmin);
    i3dViewMakePerspective(mn, mx, ndc_min, ndc_max, scr_min, scr_max,
                           view->m_guard_proj, view->m_guard_viewport);
    i3dVu0MatMul(view->m_guard_clip, view->m_guard_proj, matrix);

    /* The screen rectangle. */
    i3dVecSet(mn, fr.m_left, fr.m_bottom, fr.m_near);
    i3dVecSet(mx, fr.m_right, fr.m_top, fr.m_far);
    i3dVecSet(scr_min, x0, y0, scr.m_zmax);
    i3dVecSet(scr_max, x1, y1, scr.m_zmin);
    i3dViewMakePerspective(mn, mx, ndc_min, ndc_max, scr_min, scr_max,
                           view->m_rect_proj, view->m_rect_viewport);
    i3dVu0MatMul(view->m_rect_clip, view->m_rect_proj, matrix);

    /* PC: the rectangle's cull volume widened as the host widens the
     * camera (RosePan_RendererWidenProjection: clip x / y divided by the
     * extend), or a model past the 4:3 edges -- in the picture on a wide
     * window -- is culled and pops as the camera turns. m_rect_proj and
     * m_rect_viewport stay the ROM's: the host camera (i3rHostCamera) and
     * the CPU strips' projection are built from them. */
    float ex, ey;
    i3dViewHostExtend(param, &ex, &ey);
    for (int row = 0; row < 4; row++)
    {
        view->m_rect_clip[row][0] /= ex;
        view->m_rect_clip[row][1] /= ey;
    }

    /* The draw frustum: m_float_param of the way to the guard band. */
    float t = ctx->m_float_param;
    i3dVecSet(mn, i3dLerpf(fr.m_left, gx0, t), i3dLerpf(fr.m_bottom, gy0, t), fr.m_near);
    i3dVecSet(mx, i3dLerpf(fr.m_right, gx1, t), i3dLerpf(fr.m_top, gy1, t), fr.m_far);
    i3dVecSet(scr_min, i3dLerpf(x0, I3D_GUARD_MIN, t), i3dLerpf(y0, I3D_GUARD_MIN, t), scr.m_zmax);
    i3dVecSet(scr_max, i3dLerpf(x1, I3D_GUARD_MAX, t), i3dLerpf(y1, I3D_GUARD_MAX, t), scr.m_zmin);
    i3dViewMakePerspective(mn, mx, ndc_min, ndc_max, scr_min, scr_max,
                           view->m_proj, view->m_viewport);
    i3dMatGetScaleTrans(&view->m_screen_scale, view->m_viewport);
    i3dVu0MatMul(view->m_world_clip, view->m_proj, matrix);
    i3dVu0MatMul(view->m_world_screen, view->m_viewport, view->m_world_clip);

    /* The side planes of the rectangle's and the guard band's frusta.
     * PC: the rectangle's widened about its centre as m_rect_clip. */
    float wx = (fr.m_right - fr.m_left) * (ex - 1.0f) * 0.5f;
    float wy = (fr.m_top - fr.m_bottom) * (ey - 1.0f) * 0.5f;
    i3dVecSet(lb, fr.m_left - wx, fr.m_bottom - wy, fr.m_near);
    i3dVecSet(lt, fr.m_left - wx, fr.m_top + wy, fr.m_near);
    i3dVecSet(rb, fr.m_right + wx, fr.m_bottom - wy, fr.m_near);
    i3dVecSet(rt, fr.m_right + wx, fr.m_top + wy, fr.m_near);
    i3dViewMakePlanes(view->m_rect_planes, matrix, lb, lt, rb, rt);
    i3dVecSet(lb, gx0, gy0, fr.m_near);
    i3dVecSet(lt, gx0, gy1, fr.m_near);
    i3dVecSet(rb, gx1, gy0, fr.m_near);
    i3dVecSet(rt, gx1, gy1, fr.m_near);
    i3dViewMakePlanes(view->m_guard_planes, matrix, lb, lt, rb, rt);

    /* Camera z -> (z - near, far - z, (z - near) / (far - near), 1). */
    float range = fr.m_far - fr.m_near;
    i3dVecZero(depth[0]);
    i3dVecZero(depth[1]);
    depth[2][0] = 1.0f;
    depth[2][1] = -1.0f;
    depth[2][2] = 1.0f / range;
    depth[2][3] = 0.0f;
    depth[3][0] = -fr.m_near;
    depth[3][1] = fr.m_far;
    depth[3][2] = -fr.m_near / range;
    depth[3][3] = 1.0f;
    i3dVu0MatMul(view->m_depth, depth, matrix);

    view->m_scissor = i3dScissor(scissor.m_x0, scissor.m_x1, scissor.m_y0, scissor.m_y1);
    i3dVu0MatInverse(view->m_inverse, matrix);
    i3dVecNormalize(view->m_eye_dir, view->m_inverse[2]);
    i3dVu0VecNeg(view->m_eye_dir, view->m_eye_dir);

    /* The packet a layer CALLs: a REF of the first 10 qwords (STCYCL 1,1;
     * UNPACK V4-32 to VU1 6 + TOPS), then RET with UNPACK V4-32 of six
     * qwords to 0 + TOPS -- a header (0x11, 2), SCISSOR_1 through an A+D
     * GIF tag, two constant rows round the eye direction -- an UNPACK S-32
     * of (10, 0) to 0x10 + TOPS and MSCAL 1. Words 2-3, 0xf and 0x17 keep
     * what the buffer held, as in the ROM. */
    u_long *pkt = (u_long *)i3dCtxGetHead(ctx);
    view->m_packet = i3dDmaAddr(pkt);
    pkt = i3dPktDmaTag(pkt, 10, 0x30, view, 0x01000101, 0x6c0a8006);
    pkt = i3dPktDmaTag(pkt, 7, 0x60, NULL, 0, 0x6c068000);
    u_int *w = (u_int *)pkt;
    w[0] = 0x11;
    w[1] = 2;
    pkt[3] = 0xe;
    pkt[2] = 0x1000000000008001ull;
    pkt[5] = I3D_GS_SCISSOR_1;
    pkt[4] = view->m_scissor;
    w[0xd] = 0x3fa;
    w[0xe] = 0x3f9;
    w[0xc] = 400;
    i3dVecCopyW((float *)&w[0x10], view->m_eye_dir);
    w[0x1a] = 0;
    w[0x14] = 400;
    w[0x18] = 0x60028010;
    w[0x15] = 0x3ec;
    w[0x1b] = 0x14000001;
    w[0x16] = 0x3e2;
    w[0x19] = 10;
    i3dCtxSetHead(ctx, (u_char *)&w[0x1c]);
    return view;
}

// ──────────────────────────────────────────────────────────────────────
// The orthographic view (the record's frustum taken as a box): the same
// body as i3dCtxCreateView with the ortho builders and no plane matrices.
// No caller in RoR.
// ROR 0x002273d8

I3dView *i3dCtxCreateOrthoView(i3dContext *ctx, MagViewParam *param, sceVu0FVECTOR *matrix)
{
    I3dView *view = (I3dView *)i3dCtxAllocTail(ctx, I3D_QWC_OF(I3dView));
    sceVu0FVECTOR mn, mx, ndc_min, ndc_max, scr_min, scr_max;
    sceVu0FMATRIX depth;
    float gx0, gx1, gy0, gy1;

    i3dMatCopy(view->m_camera, matrix);

    I3dViewScreen scr = {
        param->m_zmax * 0.0625f, param->m_zmin * 0.0625f,
        param->m_x0, param->m_x1, param->m_y0, param->m_y1, param->m_offx, param->m_offy,
    };
    I3dViewFrustum fr = {
        param->m_near, param->m_far, param->m_left, param->m_right, param->m_bottom, param->m_top,
    };
    /* The scissor takes the record's rectangle before the widening. */
    I3dViewScreen scissor = scr;

    i3dViewFixRect(&scr);
    i3dViewFixOrthoFrustum(&fr);

    float x0 = scr.m_x0 + scr.m_offx, x1 = scr.m_x1 + scr.m_offx;
    float y0 = scr.m_y0 + scr.m_offy, y1 = scr.m_y1 + scr.m_offy;
    i3dRemapPair(I3D_GUARD_MIN, x0, x1, I3D_GUARD_MAX, fr.m_left, fr.m_right, &gx0, &gx1);
    i3dRemapPair(I3D_GUARD_MIN, y0, y1, I3D_GUARD_MAX, fr.m_bottom, fr.m_top, &gy0, &gy1);

    /* NDC: the rectangle less a pixel on each side. */
    float sx = (scr.m_x1 - scr.m_x0) / ((scr.m_x1 - scr.m_x0) + 2.0f);
    float sy = (scr.m_y1 - scr.m_y0) / ((scr.m_y1 - scr.m_y0) + 2.0f);
    i3dVecSet(ndc_min, -sx, -sy, -1.0f);
    i3dVecSet(ndc_max, sx, sy, 1.0f);

    /* The guard band. */
    i3dVecSet(mn, gx0, gy0, fr.m_near);
    i3dVecSet(mx, gx1, gy1, fr.m_far);
    i3dVecSet(scr_min, I3D_GUARD_MIN, I3D_GUARD_MIN, scr.m_zmax);
    i3dVecSet(scr_max, I3D_GUARD_MAX, I3D_GUARD_MAX, scr.m_zmin);
    i3dViewMakeOrtho(mn, mx, ndc_min, ndc_max, scr_min, scr_max,
                     view->m_guard_proj, view->m_guard_viewport);
    i3dVu0MatMul(view->m_guard_clip, view->m_guard_proj, matrix);

    /* The screen rectangle. */
    i3dVecSet(mn, fr.m_left, fr.m_bottom, fr.m_near);
    i3dVecSet(mx, fr.m_right, fr.m_top, fr.m_far);
    i3dVecSet(scr_min, x0, y0, scr.m_zmax);
    i3dVecSet(scr_max, x1, y1, scr.m_zmin);
    i3dViewMakeOrtho(mn, mx, ndc_min, ndc_max, scr_min, scr_max,
                     view->m_rect_proj, view->m_rect_viewport);
    i3dVu0MatMul(view->m_rect_clip, view->m_rect_proj, matrix);

    /* The draw frustum: m_float_param of the way to the guard band. */
    float t = ctx->m_float_param;
    i3dVecSet(mn, i3dLerpf(fr.m_left, gx0, t), i3dLerpf(fr.m_bottom, gy0, t), fr.m_near);
    i3dVecSet(mx, i3dLerpf(fr.m_right, gx1, t), i3dLerpf(fr.m_top, gy1, t), fr.m_far);
    i3dVecSet(scr_min, i3dLerpf(x0, I3D_GUARD_MIN, t), i3dLerpf(y0, I3D_GUARD_MIN, t), scr.m_zmax);
    i3dVecSet(scr_max, i3dLerpf(x1, I3D_GUARD_MAX, t), i3dLerpf(y1, I3D_GUARD_MAX, t), scr.m_zmin);
    i3dViewMakeOrtho(mn, mx, ndc_min, ndc_max, scr_min, scr_max,
                     view->m_proj, view->m_viewport);
    i3dMatGetScaleTrans(&view->m_screen_scale, view->m_viewport);
    i3dVu0MatMul(view->m_world_clip, view->m_proj, matrix);
    i3dVu0MatMul(view->m_world_screen, view->m_viewport, view->m_world_clip);

    /* PC: as in the ROM, no plane matrices: +0x2e0 / +0x320 keep what
     * the tail held (i3dViewTestSphere is not meant for this view). */

    /* Camera z -> (z - near, far - z, (z - near) / (far - near), 1). */
    float range = fr.m_far - fr.m_near;
    i3dVecZero(depth[0]);
    i3dVecZero(depth[1]);
    depth[2][0] = 1.0f;
    depth[2][1] = -1.0f;
    depth[2][2] = 1.0f / range;
    depth[2][3] = 0.0f;
    depth[3][0] = -fr.m_near;
    depth[3][1] = fr.m_far;
    depth[3][2] = -fr.m_near / range;
    depth[3][3] = 1.0f;
    i3dVu0MatMul(view->m_depth, depth, matrix);

    view->m_scissor = i3dScissor(scissor.m_x0, scissor.m_x1, scissor.m_y0, scissor.m_y1);
    i3dVu0MatInverse(view->m_inverse, matrix);
    i3dVecNormalize(view->m_eye_dir, view->m_inverse[2]);
    i3dVu0VecNeg(view->m_eye_dir, view->m_eye_dir);

    /* The packet a layer CALLs: a REF of the first 10 qwords (STCYCL 1,1;
     * UNPACK V4-32 to VU1 6 + TOPS), then RET with UNPACK V4-32 of six
     * qwords to 0 + TOPS -- a header (0x11, 2), SCISSOR_1 through an A+D
     * GIF tag, two constant rows round the eye direction -- an UNPACK S-32
     * of (10, 0) to 0x10 + TOPS and MSCAL 1. Words 2-3, 0xf and 0x17 keep
     * what the buffer held, as in the ROM. */
    u_long *pkt = (u_long *)i3dCtxGetHead(ctx);
    view->m_packet = i3dDmaAddr(pkt);
    pkt = i3dPktDmaTag(pkt, 10, 0x30, view, 0x01000101, 0x6c0a8006);
    pkt = i3dPktDmaTag(pkt, 7, 0x60, NULL, 0, 0x6c068000);
    u_int *w = (u_int *)pkt;
    w[0] = 0x11;
    w[1] = 2;
    pkt[3] = 0xe;
    pkt[2] = 0x1000000000008001ull;
    pkt[5] = I3D_GS_SCISSOR_1;
    pkt[4] = view->m_scissor;
    w[0xd] = 0x3fa;
    w[0xe] = 0x3f9;
    w[0xc] = 400;
    i3dVecCopyW((float *)&w[0x10], view->m_eye_dir);
    w[0x1a] = 0;
    w[0x14] = 400;
    w[0x18] = 0x60028010;
    w[0x15] = 0x3ec;
    w[0x1b] = 0x14000001;
    w[0x16] = 0x3e2;
    w[0x19] = 10;
    i3dCtxSetHead(ctx, (u_char *)&w[0x1c]);
    return view;
}
