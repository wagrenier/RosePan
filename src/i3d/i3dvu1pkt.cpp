/* ==========================================================================
 *  i3d/i3dvu1pkt.cpp
 *
 *  The i3d library's VU1 packet writers (ROR 0x21c960..0x21eb28): strips
 *  and sprites UNPACKed to VU1 and drawn by its microprograms. See
 *  i3dvu1pkt.h.
 *
 *  Written so far: the strip body and its two entries, the sprite family
 *  and the view-matrix helpers. Still stubs (lib/unassigned_stubs.cpp):
 *  the clipped strips i3dPktDrawStripCtx 0x21dd10, i3dPktDrawLineStrip
 *  0x21e1d8, their clip helpers 0x21db60 / 0x21db90, the unreferenced
 *  0x21cfb0..0x21db18 and the wire shapes 0x21e6f8..0x21eb24.
 *
 *  PORT: each writer writes the ROM's packet words (DMA tags address host
 *  memory by i3dDmaAddr; nothing on the host follows them) and records the
 *  primitives its microprogram would kick in the native i3d module,
 *  computed here on the CPU (RosePan_I3dRecordVu1Strip).
 * ======================================================================== */
#include "i3d/i3dvu1pkt.h"

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
