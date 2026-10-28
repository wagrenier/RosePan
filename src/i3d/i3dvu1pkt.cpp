/* ==========================================================================
 *  i3d/i3dvu1pkt.cpp
 *
 *  The i3d library's VU1 packet writers (ROR 0x21c960..0x21eb28): strips
 *  and sprites UNPACKed to VU1 and drawn by its microprograms. See
 *  i3dvu1pkt.h.
 *
 *  Written: the strip body and its two entries, the clip helpers and the
 *  CPU-side line strips, the sprite family, the view-matrix helpers and
 *  the wire shapes. Not written: 0x21cfb0..0x21d378, four functions no
 *  code or data references (a REF-tag helper, an index-list builder and
 *  two strip writers that use them) -- dead in the retail ROM.
 *
 *  PORT: each writer writes the ROM's packet words (DMA tags address host
 *  memory by i3dDmaAddr; nothing on the host follows them) and records the
 *  primitives its microprogram would kick in the native i3d module,
 *  computed here on the CPU (RosePan_I3dRecordVu1Strip).
 * ======================================================================== */
#include "i3d/i3dvu1pkt.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "i3d/i3dctx.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "rosepan/rendering/rosepan_i3d.h"

/* The colour scales the microprograms multiply 0..1 colours by before
 * ftoi0: textured (TME: 1.0 = 0x80, the GS's modulate identity) or not. */
static const sceVu0FVECTOR s_texColorScale = { 128.0f, 128.0f, 128.0f, 128.0f };  /* data 0x3026b0 */
static const sceVu0FVECTOR s_colorScale    = { 255.0f, 255.0f, 255.0f, 128.0f };  /* data 0x3026c0 */

/* The strip's default ADC lists, one 0x30-byte entry per 18-vertex step:
 * vertex indices as V2-16 pairs, no skip bit (data 0x3026d0). */
static const u_int s_stripIndices[60] = {
    0x00010000, 0x00030002, 0x00050004, 0x00070006, 0x00090008, 0x000b000a, 0x000d000c, 0x000f000e,
    0x00110010, 0x00130012, 0x00000000, 0x00000000, 0x00130012, 0x00150014, 0x00170016, 0x00190018,
    0x001b001a, 0x001d001c, 0x001f001e, 0x00210020, 0x00230022, 0x00250024, 0x00000000, 0x00000000,
    0x00250024, 0x00270026, 0x00290028, 0x002b002a, 0x002d002c, 0x002f002e, 0x00310030, 0x00330032,
    0x00350034, 0x00370036, 0x00000000, 0x00000000, 0x00370036, 0x00390038, 0x003b003a, 0x003d003c,
    0x003f003e, 0x00410040, 0x00430042, 0x00450044, 0x00470046, 0x00490048, 0x00000000, 0x00000000,
    0x00490048, 0x004b004a, 0x004d004c, 0x004f004e, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

/* The 4-qword command each strip chunk REFs before its MSCAL 1 (the VIF
 * reads them as UNPACK data after the tag's 0x190 word): the program 400
 * copy arguments, then the strip's GIF tag -- PRIM 0x4c (triangle strip,
 * gouraud, ABE) or 0x5c with TME -- and the row / mask set-up (data
 * 0x302970 untextured, 0x3029b0 textured). */
static const u_int s_stripCmd[2][16] = {
    { 0x000003fb, 0x000003fa, 0x00000028, 0x0000003c, 0x00000050, 0x000003dc, 0x00000000, 0x00000000,
      0x00008000, 0x40264000, 0x0000512a, 0x60018004, 0x00000000, 0x20000000, 0xffffff3f, 0x70018001 },
    { 0x000003fb, 0x000003fa, 0x00000028, 0x0000003c, 0x00000050, 0x000003dc, 0x00000000, 0x00000000,
      0x00008000, 0x402e4000, 0x0000512a, 0x60018004, 0x00000000, 0x20000000, 0xffffff3f, 0x70018001 },
};

/* PC: the VU1 state the host writers mirror -- the CALLed view (0x3e2) and
 * what i3dPktBeginVu1Sprite / Vu1Rect / Vu1Matrix uploaded for the sprites. */
static struct
{
    const I3dView *view;
    unsigned       prim;           /* 0x3f4: the sprite GIF tag's PRIM */
    const float   *scale;          /* 0x3f5: the colour scale */
    float          rect[4];        /* 0x3f1 */
    float          rows[3][4];     /* 0x3ee */
} s_vu1 = { NULL, 0x4d, s_colorScale, { 0.0f, 0.0f, 1.0f, 1.0f },
            { { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f } } };

void i3dVu1SetView(const I3dView *view)
{
    s_vu1.view = view;
}

/* PC: v (x, y, z, w) through a row-vector matrix: m[0] x + m[1] y + m[2] z + m[3] w,
 * as the microprograms' MULAx / MADDAy / MADDAz / MADDw chains do. */
static void vu1Apply(float *out, const sceVu0FVECTOR *m, const float *v)
{
    float r[4];
    for (int k = 0; k < 4; k++)
        r[k] = m[0][k] * v[0] + m[1][k] * v[1] + m[2][k] * v[2] + m[3][k] * v[3];
    memcpy(out, r, sizeof(r));
}

/* PC: a world point to the GS screen with the mirrored view; false behind
 * the eye (the microprograms' clip test drops those primitives). */
static bool vu1Project(RosePanI3dVu1Vertex *out, const float *world)
{
    float p[4];
    vu1Apply(p, s_vu1.view->m_world_screen, world);
    if (p[3] <= 0.0f)
        return false;
    const float q = 1.0f / p[3];
    out->x = p[0] * q;
    out->y = p[1] * q;
    /* PC: the host's depth, from the same view's camera * projection. */
    float c[4];
    vu1Apply(c, s_vu1.view->m_world_clip, world);
    const float z = c[3] != 0.0f ? c[2] / c[3] : 1.0f;
    out->z = z < -1.0f ? -1.0f : z > 1.0f ? 1.0f : z;   /* the far-plane pages sit at +1 */
    return true;
}

/* PC: a 0..1 colour times the scale, ftoi0 (saturated as the GS takes 8 bits). */
static void vu1Color(u_char *out, const float *color, const float *scale)
{
    for (int k = 0; k < 4; k++)
    {
        int c = (int)(color[k] * scale[k]);
        out[k] = (u_char)(c < 0 ? 0 : c > 255 ? 255 : c);
    }
}

// ROR 0x0021c960

u_long *i3dPktDrawStripImpl(u_long *pkt, sceVu0FVECTOR *matrix, int count, const void *stq,
                            const float *color, const void *colors, const void *xyz,
                            const u_char *adc)
{
    static const float s_white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    const float *scale = stq != NULL ? s_texColorScale : s_colorScale;

    /* PC: the strip as the microprogram draws it: every vertex through the
     * matrix and the view, colours scaled, ADC from the flags. */
    if (s_vu1.view != NULL && count >= 3)
    {
        RosePanI3dVu1Vertex v[0x200];
        const int n = count < 0x200 ? count : 0x200;
        for (int i = 0; i < n; i++)
            v[i].kick = adc == NULL || adc[i] == 0;
        for (int i = 0; i < n; i++)
        {
            const float *p = (const float *)xyz + i * 4;
            float local[4] = { p[0], p[1], p[2], 1.0f };
            float world[4];
            vu1Apply(world, matrix, local);
            if (!vu1Project(&v[i], world))
            {
                for (int k = i; k < i + 3 && k < n; k++)
                    v[k].kick = 0;
            }
            if (stq != NULL)
            {
                const float *t = (const float *)stq + i * 4;
                v[i].s = t[0];
                v[i].t = t[1];
                v[i].q = t[2] != 0.0f ? t[2] : 1.0f;
            }
            else
            {
                v[i].s = v[i].t = 0.0f;
                v[i].q = 1.0f;
            }
            const float *c = colors != NULL ? (const float *)colors + i * 4
                                            : color != NULL ? color : s_white;
            vu1Color(v[i].rgba, c, scale);
        }
        RosePan_I3dRecordVu1Strip(pkt, stq != NULL ? 0x5c : 0x4c, v, n);
    }

    /* The set-up: STCYCL 1 1, UNPACK 9 qwords to TOPS -- two program 400
     * copies (the GIF template and colour scale to 0x3f4.., the matrix to
     * 0x3d2..), a 0 terminator, MSCAL 1. */
    pkt[1] = 0x1000000001000101ull;
    pkt[0] = 0x1000000b;
    int *w = (int *)pkt;
    w[4] = 0;
    w[7] = 0x6c098000;
    w[8] = 400;
    w[5] = 0;
    w[9] = 0x3f7;
    w[6] = 0;
    w[10] = 0x3f4;
    pkt[6] = stq == NULL ? 0x4026c00000008000ull : 0x402ec00000008000ull;
    pkt[7] = 0x512a;
    i3dVecCopy2((float *)(pkt + 8), scale);
    w[27] = 0;
    w[24] = 400;
    w[25] = 0x3d6;
    w[26] = 0x3d2;
    float *one = (float *)(pkt + 10);
    one[3] = 1.0f;
    one[2] = 1.0f;
    one[1] = 1.0f;
    one[0] = 1.0f;
    i3dMatCopy((sceVu0FVECTOR *)(pkt + 14), matrix);
    w[45] = 0;
    w[47] = 0;
    w[44] = 0x60018009;
    w[46] = 0x14000001;
    u_long *p = pkt + 0x18;

    if (colors == NULL)
    {
        /* One colour for all: into the row registers, filled by masked
         * UNPACKs of 0x50 vertices to 0x3c and 0x121. */
        p[0] = 0x10000004;
        p[1] = 0x3000000001005000ull;
        float *row = (float *)(p + 2);
        memcpy(row, color != NULL ? color : s_white, 4 * sizeof(float));
        int *q = (int *)(p + 4);
        q[0] = 0x20000000;          /* STMASK 0x55555555: every field from the row */
        q[1] = 0x55555555;
        q[2] = 0x7c50003c;
        q[3] = 0x7c500121;
        q[4] = 0x01000101;          /* STCYCL 1 1, STROW 0 */
        q[5] = 0;
        q[6] = 0;
        q[7] = 0x30000000;
        q[8] = 0;
        q[9] = 0;
        q[10] = 0;
        q[11] = 0;
        p = pkt + 0x22;
    }

    const char *v_xyz = (const char *)xyz;
    const char *v_stq = (const char *)stq;
    const char *v_col = (const char *)colors;
    u_int left = (u_int)count;
    for (;;)
    {
        const u_int n = left > 0x50 ? 0x50 : left;

        /* The positions inline: UNPACK n to 0x1dc. */
        p[0] = ((u_long)(n + 4) & 0xffff) | 0x10000000;
        p[1] = ((u_long)((n << 16) | 0x6c0001dc) << 32) | 0x10000000;
        for (u_int i = 0; i < n; i++)
            memcpy(p + 2 + i * 2, v_xyz + i * 16, 16);
        u_long *q = p + n * 2 + 2;
        int *qw = (int *)q;
        /* UNPACK 2 to TOPS (program 400: 0x3f4 -> 0x3f3), S-32 2 to TOPS + 2
         * (0x15, the vertex count), MSCAL 1. */
        qw[3] = 0x6c028000;
        qw[4] = 400;
        qw[0] = 0;
        qw[5] = 0x3f4;
        qw[6] = 0x3f3;
        qw[12] = 0x60028002;
        qw[15] = 0x14000001;
        qw[9] = 0x8000;
        qw[13] = 0x15;
        qw[1] = 0;
        qw[2] = 0;
        qw[8] = 0;
        qw[10] = 0;
        qw[11] = (int)n;
        qw[14] = 0;
        p = q + 10;

        /* The STQs and colours inline after a NEXT tag, REF'd per chunk. */
        u_int stqAddr = 0, colAddr = 0;
        if (stq != NULL)
        {
            for (int i = (int)n - 1; i >= 0; i--)
                memcpy(p + i * 2, v_stq + i * 16, 16);
            stqAddr = i3dDmaAddr(p);
            p += n * 2;
        }
        if (colors != NULL)
        {
            for (int i = (int)n - 1; i >= 0; i--)
                memcpy(p + i * 2, v_col + i * 16, 16);
            colAddr = i3dDmaAddr(p);
            p += n * 2;
        }
        q[9] = 0;
        q[8] = ((u_long)i3dDmaAddr(p) << 32) | 0x20000000;

        /* Chunks of 20 vertices, two shared with the next. */
        u_int i = 0;
        do {
            u_int m = n - i;
            u_int back = 0;
            if (m > 0x14)
            {
                m = 0x14;
                back = 2;
            }
            if (stq != NULL)
            {
                p[0] = ((u_long)(stqAddr + i * 16) << 32) | 0x30000000 | (m & 0xffff);
                p[1] = (u_long)((m << 16) | 0x6c008028) << 32;
                p += 2;
            }
            if (colors != NULL)
            {
                p[1] = (u_long)((m << 16) | 0x6c00803c) << 32;
                p[0] = ((u_long)(colAddr + i * 16) << 32) | 0x30000000 | (m & 0xffff);
                p += 2;
            }
            if (adc == NULL)
            {
                const u_int k = (m + 7) >> 3;
                p[1] = (u_long)((k << 19) | 0x66008050) << 32;
                p[0] = ((u_long)i3dDmaAddr(&s_stripIndices[(i / 18) * 12]) << 32) | 0x30000000 |
                       (k & 0xffff);
                p += 2;
            }
            else
            {
                /* The vertex indices with bit 15 (ADC) where the flag is set. */
                const u_int k = (m + 3) >> 2;
                p[0] = (k & 0xffff) | 0x10000000;
                p[1] = (u_long)((k << 18) | 0x65008050) << 32;
                u_int idx = i;
                u_int *d = (u_int *)(p + 2);
                for (u_int j = 0; j < k * 4; j++, idx++)
                    d[j] = adc[idx] != 0 ? idx + 0x8000 : idx;
                p += k * 2 + 2;
            }
            p[1] = 0x0000019068048000ull;
            p[0] = ((u_long)i3dDmaAddr(s_stripCmd[stq != NULL]) << 32) | 0x30000004;
            i += m - back;
            p[2] = 0x10000000;
            p[3] = (u_long)m | 0x1400000100000000ull;
            p += 4;
        } while (i != n);

        left -= n - 2;
        if (left < 3)
            break;
        const u_int step = (n - 2) * 16;
        if (stq != NULL)
            v_stq += step;
        if (colors != NULL)
            v_col += step;
        if (adc != NULL)
            adc += n - 2;
        v_xyz += step;
    }
    return p;
}

// ROR 0x0021db18

u_long *i3dPktDrawStrip(u_long *pkt, sceVu0FVECTOR *matrix, int count, const void *stq,
                        const void *colors, const void *xyz, const u_char *adc)
{
    return i3dPktDrawStripImpl(pkt, matrix, count, stq, NULL, colors, xyz, adc);
}

// ROR 0x0021db40

u_long *i3dPktDrawStripColor(u_long *pkt, sceVu0FVECTOR *matrix, int count, const void *stq,
                             const float *color, const void *xyz, const u_char *adc)
{
    return i3dPktDrawStripImpl(pkt, matrix, count, stq, color, NULL, xyz, adc);
}

// ROR 0x0021db60

u_int i3dVu0ClipPair(const float *a, const float *b)
{
    /* VCLIP a then b: each shifts the CLIP flags up 6 and sets +x -x +y -y
     * +z -z where the component lies outside +-|w|. */
    u_int clip = 0;
    const float *v[2] = { a, b };
    for (int i = 0; i < 2; i++)
    {
        const float w = fabsf(v[i][3]);
        u_int f = 0;
        for (int k = 0; k < 3; k++)
        {
            if (v[i][k] > w)
                f |= 1u << (k * 2);
            if (v[i][k] < -w)
                f |= 2u << (k * 2);
        }
        clip = (clip << 6) | f;
    }
    return clip;
}

// ROR 0x0021db90

int i3dClipSegment(float *a, float *b, int stq, int color)
{
    /* Plane k = axis k / 2, the +w side for even k, -w for odd: the end
     * outside it moves to the crossing; both outside rejects. */
    for (int k = 5; k >= 0; k--)
    {
        const float sign = (k & 1) != 0 ? 1.0f : -1.0f;
        const u_int clip = i3dVu0ClipPair(a, b);
        float *out = (clip & (1u << k)) != 0 ? b : NULL;
        if ((clip & (0x40u << k)) != 0)
        {
            out = a;
            if ((clip & (1u << k)) != 0)
                return 0;
        }
        if (out != NULL)
        {
            const int axis = k / 2;
            const float da = a[axis] + a[3] * sign;
            const float t = fabsf(da / ((b[axis] + b[3] * sign) - da));
            i3dVu0VecInterp(t, out, b, a);
            if (stq)
                i3dVu0VecInterp(t, out + 4, b + 4, a + 4);
            if (color)
                i3dVu0VecInterp(t, out + 8, b + 8, a + 8);
        }
    }
    return 1;
}

// ROR 0x0021dd10

u_long *i3dPktDrawStripCtx(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix, int count,
                           const void *stq, const void *colors, const void *xyz,
                           const u_char *adc)
{
    /* Each end: clip-space position, STQ, scaled colour (i3dClipSegment's
     * +0x00 / +0x10 / +0x20). */
    struct End { sceVu0FVECTOR pos, stq, color; } e[2];
    static_assert(sizeof(End) == 0x30, "End");
    static_assert(offsetof(End, stq) == 0x10 && offsetof(End, color) == 0x20, "End");
    const float *scale = stq == NULL ? s_colorScale : s_texColorScale;

    I3dView *view = i3dLayerGetView(layer);
    sceVu0FVECTOR *camera = i3dViewGetCamera(view);
    sceVu0FVECTOR *proj = i3dViewGetRectProj(view);
    sceVu0FVECTOR *viewport = i3dViewGetRectViewport(view);
    sceVu0FMATRIX m;
    if (matrix != NULL)
    {
        i3dVu0MatMul(m, camera, matrix);
        camera = m;
    }
    i3dVu0MatMul(m, proj, camera);

    for (int i = 0; i < count - 1; i++)
    {
        const float *p = (const float *)xyz + i * 4;
        i3dVu0MatApply(e[0].pos, m, p);
        i3dVu0MatApply(e[1].pos, m, p + 4);
        if (stq != NULL)
        {
            const float *t = (const float *)stq + i * 4;
            i3dVecCopyW(e[0].stq, t);
            i3dVecCopyW(e[1].stq, t + 4);
        }
        if (colors != NULL)
        {
            /* Clamped to 0..1, times the scale. */
            const float *c = (const float *)colors + i * 4;
            for (int j = 0; j < 2; j++)
                for (int k = 0; k < 4; k++)
                {
                    float v = c[j * 4 + k];
                    v = v < 0.0f ? 0.0f : v;
                    v = v > 1.0f ? 1.0f : v;
                    e[j].color[k] = v * scale[k];
                }
        }
        if ((adc != NULL && adc[i + 1] != 0) || (e[0].pos[3] < 0.0f && e[1].pos[3] < 0.0f))
            continue;
        if (!i3dClipSegment(e[0].pos, e[1].pos, stq != NULL, colors != NULL))
            continue;

        int s0[4], s1[4];
        i3dVu0RotTransPers(s0, viewport, e[0].pos, 0);
        i3dVu0RotTransPers(s1, viewport, e[1].pos, 0);
        const u_long xyz0 = i3dPackXYZ(s0[0], s0[1], s0[2]);
        const u_long xyz1 = i3dPackXYZ(s1[0], s1[1], s1[2]);
        if (colors == NULL)
        {
            if (stq != NULL)
                pkt = i3dPktGouraudTexLineST(pkt, xyz0, xyz1, i3dPackST(e[0].stq[0], e[0].stq[1]),
                                             i3dPackST(e[1].stq[0], e[1].stq[1]),
                                             i3dPackRGBAQ(e[0].stq[2], 0xff, 0xff, 0xff, 0x80),
                                             i3dPackRGBAQ(e[1].stq[2], 0xff, 0xff, 0xff, 0x80));
            else
                pkt = i3dPktLine(pkt, xyz0, xyz1, i3dPackRGBAQ(1.0f, 0xff, 0xff, 0xff, 0x80));
        }
        else
        {
            int c0[4], c1[4];
            i3dVu0VecFtoi0(c0, e[0].color);
            i3dVu0VecFtoi0(c1, e[1].color);
            if (stq == NULL)
                pkt = i3dPktGouraudLine(pkt, xyz0, xyz1,
                                        i3dPackRGBAQ(1.0f, c0[0], c0[1], c0[2], c0[3]),
                                        i3dPackRGBAQ(1.0f, c1[0], c1[1], c1[2], c1[3]));
            else
                pkt = i3dPktGouraudTexLineST(pkt, xyz0, xyz1, i3dPackST(e[0].stq[0], e[0].stq[1]),
                                             i3dPackST(e[1].stq[0], e[1].stq[1]),
                                             i3dPackRGBAQ(e[0].stq[2], c0[0], c0[1], c0[2], c0[3]),
                                             i3dPackRGBAQ(e[1].stq[2], c1[0], c1[1], c1[2], c1[3]));
        }
    }
    return pkt;
}

// ROR 0x0021e1d8

u_long *i3dPktDrawLineStrip(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix, int count,
                            const void *color, const void *xyz, const u_char *adc)
{
    const float *c = (const float *)color;
    const u_long rgbaq = i3dPackRGBAQ(1.0f, (int)(c[0] * 255.0f), (int)(c[1] * 255.0f),
                                      (int)(c[2] * 255.0f), (int)(c[3] * 128.0f));
    I3dView *view = i3dLayerGetView(layer);
    sceVu0FVECTOR *camera = i3dViewGetCamera(view);
    sceVu0FVECTOR *proj = i3dViewGetRectProj(view);
    sceVu0FVECTOR *viewport = i3dViewGetRectViewport(view);
    sceVu0FMATRIX m;
    if (matrix != NULL)
    {
        i3dVu0MatMul(m, camera, matrix);
        camera = m;
    }
    i3dVu0MatMul(m, proj, camera);

    for (int i = 0; i < count - 1; i++)
    {
        sceVu0FVECTOR a, b;
        const float *p = (const float *)xyz + i * 4;
        i3dVu0MatApply(a, m, p);
        i3dVu0MatApply(b, m, p + 4);
        if ((adc != NULL && adc[i + 1] != 0) || (a[3] < 0.0f && b[3] < 0.0f))
            continue;
        if (!i3dClipSegment(a, b, 0, 0))
            continue;
        int s0[4], s1[4];
        i3dVu0RotTransPers(s0, viewport, a, 0);
        i3dVu0RotTransPers(s1, viewport, b, 0);
        pkt = i3dPktLine(pkt, i3dPackXYZ(s0[0], s0[1], s0[2]), i3dPackXYZ(s1[0], s1[1], s1[2]),
                         rgbaq);
    }
    return pkt;
}

// ROR 0x0021e428

u_long *i3dPktVu1Matrix(u_long *pkt, sceVu0FVECTOR *matrix)
{
    /* Three rows to 0x3ee; the copy writes the fourth over what follows. */
    u_long *data = i3dPktDmaTag(pkt, 3, 0x10, NULL, 0x10000000, 0x6c0303ee);
    i3dMatCopy2((sceVu0FVECTOR *)data, matrix);
    memcpy(s_vu1.rows, matrix, sizeof(s_vu1.rows));    // PC: the VU1 mirror
    return data + 6;
}

// ROR 0x0021e488

u_long *i3dPktVu1Rect(u_long *pkt, const float *rect)
{
    u_long *data = i3dPktDmaTag(pkt, 1, 0x10, NULL, 0x10000000, 0x6c0103f1);
    i3dVecCopy((float *)data, rect);
    memcpy(s_vu1.rect, rect, sizeof(s_vu1.rect));      // PC: the VU1 mirror
    return data + 2;
}

// ROR 0x0021e4e8

u_long *i3dPktVu1ViewMatrix(u_long *pkt, I3dLayer *layer)
{
    return i3dPktVu1Matrix(pkt, i3dViewGetInverse(i3dLayerGetView(layer)));
}

// ROR 0x0021e520

u_long *i3dPktVu1ViewMatrixScaled(float sx, float sy, u_long *pkt, I3dLayer *layer)
{
    sceVu0FMATRIX m;
    i3dMatCopy2(m, i3dViewGetInverse(i3dLayerGetView(layer)));
    i3dVecScale(sx, m[0], m[0]);
    i3dVecScale(sy, m[1], m[1]);
    return i3dPktVu1Matrix(pkt, m);
}

// ROR 0x0021e5a0

u_long *i3dPktBeginVu1Sprite(u_long *pkt, I3dLayer *layer, int mode)
{
    const bool fog = i3dLayerGetFog(layer) != NULL;
    pkt = i3dPktCallView(pkt, layer);
    if (fog)
        pkt = i3dPktCallFog(pkt, layer);
    /* The GIF template to 0x3f4 (NREG 4: FOG, ST, RGBAQ, XYZ2; PRIM a fan,
     * gouraud, ABE, TME from the mode, FGE with fog), the colour scale to
     * 0x3f5. */
    u_long *data = i3dPktDmaTag(pkt, 2, 0x10, NULL, 0x01000101, 0x6c0203f4);
    const u_long prim = ((u_long)(mode & 0xff) << 4) | ((u_long)fog << 5) | 0x4d;
    data[1] = 0x512a;
    data[0] = (prim << 47) | 0x4000400000008000ull;
    const float *scale = (mode & 0xff) != 0 ? s_texColorScale : s_colorScale;
    memcpy(data + 2, scale, sizeof(sceVu0FVECTOR));
    s_vu1.prim = (unsigned)prim;                       // PC: the VU1 mirror
    s_vu1.scale = scale;
    return data + 4;
}

// ROR 0x0021e688

u_long *i3dPktEndVu1Sprite(u_long *pkt)
{
    return pkt;
}

// ROR 0x0021e690

u_long *i3dPktVu1Sprite(u_long *pkt, const float *pos, const float *color)
{
    /* PC: what microprogram 0x712 kicks -- the corners centre +- 0.5 row 0
     * +- 0.5 row 1 through the view, the rectangle's corners as ST, the
     * colour scaled; drawn as a strip (the PS2 draws the same quad as a fan). */
    if (s_vu1.view != NULL)
    {
        static const float s_corner[4][2] = {
            { -0.5f, -0.5f }, { 0.5f, -0.5f }, { -0.5f, 0.5f }, { 0.5f, 0.5f },
        };
        RosePanI3dVu1Vertex v[4];
        bool visible = true;
        for (int i = 0; i < 4; i++)
        {
            float world[4];
            for (int k = 0; k < 3; k++)
                world[k] = pos[k] + s_corner[i][0] * s_vu1.rows[0][k] +
                           s_corner[i][1] * s_vu1.rows[1][k];
            world[3] = 1.0f;
            visible = vu1Project(&v[i], world) && visible;
            v[i].s = s_corner[i][0] < 0.0f ? s_vu1.rect[0] : s_vu1.rect[2];
            v[i].t = s_corner[i][1] < 0.0f ? s_vu1.rect[1] : s_vu1.rect[3];
            v[i].q = 1.0f;
            vu1Color(v[i].rgba, color, s_vu1.scale);
            v[i].kick = 1;
        }
        if (visible)
            RosePan_I3dRecordVu1Strip(pkt, s_vu1.prim, v, 4);
    }

    /* STCYCL 1 1, UNPACK the colour and the centre to TOPS + 1, the
     * program address 0x712 to TOPS + 0, MSCAL 8 (the dispatcher). */
    pkt[1] = 0x6c02800101000101ull;
    pkt[0] = 0x10000003;
    memcpy(pkt + 2, color, sizeof(sceVu0FVECTOR));
    memcpy(pkt + 4, pos, sizeof(sceVu0FVECTOR));
    int *w = (int *)(pkt + 6);
    w[0] = 0x60018000;
    w[2] = 0x14000008;
    w[1] = 0x712;
    w[3] = 0;
    return pkt + 8;
}

/* The wire shapes' data: the sphere circles' colours (data 0x302a30), the
 * cube's corners in strip order and the ADC flags that skip its three
 * jumps (0x302a68 / 0x302aa8), the corner signs (0x302ab8: bit 0 picks
 * entry 0 / 1, bit 1 entry 0 / 2, bit 2 entry 0 / 4). */
static const float s_wireColor[3][4] = {
    { 1.0f, 0.25f, 0.25f, 1.0f }, { 0.25f, 1.0f, 0.25f, 1.0f }, { 0.25f, 0.25f, 1.0f, 1.0f },
};
static const int s_cubePath[16] = { 0, 1, 3, 2, 0, 4, 5, 7, 6, 4, 2, 6, 3, 7, 1, 5 };
static const u_char s_cubeAdc[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0 };
static const float s_cubeSign[5] = { -1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

// ROR 0x0021e6f8

u_long *i3dPktDrawWireSphere(float radius, u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix,
                             const float *center)
{
    /* Ten points: the centre, then the circle at 0, 45 .. 360 degrees. */
    sceVu0FVECTOR v[10];
    i3dVecCopyW(v[0], center);
    v[0][3] = 1.0f;
    for (int axis = 0; axis < 3; axis++)
    {
        for (int i = 1; i < 10; i++)
        {
            const float a = (float)(i - 1) * 0.7853982f;
            if (axis == 0)
            {
                v[i][0] = center[0] + cosf(a) * radius;
                v[i][1] = center[1];
                v[i][2] = center[2] + sinf(a) * radius;
            }
            else if (axis == 1)
            {
                v[i][0] = center[0] + sinf(a) * radius;
                v[i][1] = center[1] + cosf(a) * radius;
                v[i][2] = center[2];
            }
            else
            {
                v[i][0] = center[0];
                v[i][1] = center[1] + sinf(a) * radius;
                v[i][2] = center[2] + cosf(a) * radius;
            }
            v[i][3] = 1.0f;
        }
        pkt = i3dPktDrawLineStrip(pkt, layer, matrix, 10, s_wireColor[axis], v, NULL);
    }
    return pkt;
}

/* The eight corners sign * extent + centre, laid out in the strip's order
 * (inline in both callers in the ROM). */
static void wireCube(sceVu0FVECTOR *out, const float *center, const float *extent)
{
    sceVu0FVECTOR corner[8];
    for (int i = 0; i < 8; i++)
    {
        corner[i][0] = extent[0] * s_cubeSign[i & 1] + center[0];
        corner[i][1] = extent[1] * s_cubeSign[i & 2] + center[1];
        corner[i][2] = extent[2] * s_cubeSign[i & 4] + center[2];
        corner[i][3] = 1.0f;
    }
    for (int i = 0; i < 16; i++)
        memcpy(out[i], corner[s_cubePath[i]], sizeof(sceVu0FVECTOR));
}

// ROR 0x0021e8f8

u_long *i3dPktDrawWireBox(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix,
                          const float *center, const void *extent, const void *color)
{
    sceVu0FVECTOR v[16];
    wireCube(v, center, (const float *)extent);
    return i3dPktDrawLineStrip(pkt, layer, matrix, 16, color, v, s_cubeAdc);
}

// ROR 0x0021e9f8

u_long *i3dPktDrawWireShape(u_long *pkt, I3dLayer *layer, void *a, void *b, void *c)
{
    /* The unit cube through b, or b then a. */
    static const float s_zero[3] = { 0.0f, 0.0f, 0.0f };
    static const float s_one[3] = { 1.0f, 1.0f, 1.0f };
    sceVu0FVECTOR v[16];
    wireCube(v, s_zero, s_one);
    sceVu0FMATRIX m;
    if (a != NULL)
        i3dVu0MatMul(m, (sceVu0FVECTOR *)b, (sceVu0FVECTOR *)a);
    else
        i3dMatCopy(m, (sceVu0FVECTOR *)b);
    return i3dPktDrawLineStrip(pkt, layer, m, 16, c, v, s_cubeAdc);
}
