/* ==========================================================================
 *  i3d/i3dpacket.cpp
 *
 *  The i3d library's GS packet writers (ROR 0x21b220..0x21c960): draw-set
 *  list helpers, DMA tags, register writes, the blend / Z-clear packets and
 *  one writer per GS primitive. See i3dpacket.h.
 *
 *  Every writer leaves the DMA tag at pkt (written last in the ROM: it
 *  needs the qword count) and its GIF tag at pkt + 0x10, then the register
 *  data qword by qword. The *F writers convert float vectors with the
 *  vector module's VU0 macro helpers (i3dVecMul / Add / Ftoi0 / Ftoi4, the
 *  module's second copy at 0x2372d0..).
 *
 *  PORT: the packets carry the ROM's VIF / GIF words; the host's DMA does
 *  not follow chains and nothing consumes them yet (no GS emulation, see
 *  open-items "The i3d packets on the host"). The tags' address words are
 *  the low 28 bits of host pointers (i3dDmaAddr).
 * ======================================================================== */
#include "i3d/i3dpacket.h"

#include <string.h>

#include "lib/unassigned.h"

/* Qword n of a packet, as the four words the VU0 helpers write. */
#define I3D_PKT_QW(pkt, n) ((int *)((pkt) + 2 * (n)))

/* GIF tags: NLOOP 1, EOP, PRE with the primitive, NREG registers; the
 * REGLIST ones (FLG 1) take PRIM as their first register. */
#define I3D_GIF_AD         0x1000000000008001ull   /* PACKED, NREG 1: A+D */
#define I3D_GIF_REGLIST(n) (((u_long)(n) << 60) | (1ull << 58) | 0x8001ull)

/* The screen transform i3dPktConvXYZ applies. */
static sceVu0FVECTOR s_screenOffset = { 0.0f, 0.0f, 0.0f, 0.0f };   /* data 0x2db330 */
static sceVu0FVECTOR s_screenScale  = { 1.0f, 1.0f, 1.0f, 1.0f };   /* data 0x2db340 */

/* Colour scales of i3dPktConvRGBA: textured (1.0 = 0x80, the GS's
 * modulate neutral) and flat (alpha 1.0 = 0x80). */
static const sceVu0FVECTOR s_texColorScale = { 128.0f, 128.0f, 128.0f, 128.0f };  /* data 0x3026b0 */
static const sceVu0FVECTOR s_colorScale    = { 255.0f, 255.0f, 255.0f, 128.0f };  /* data 0x3026c0 */

/* ---- draw sets ---- */

// ROR 0x0021b220
void i3dDrawSetInsertAfter(I3dDrawSet *set, I3dDrawSet *at)
{
    I3dDrawSet *next = at->m_next;
    set->m_next = next;
    next->m_prev = set;
    at->m_next = set;
    set->m_prev = at;
}

// ROR 0x0021b238
void i3dDrawSetMoveAfter(I3dDrawSet *set, I3dDrawSet *at)
{
    set->m_prev->m_next = set->m_next;
    set->m_next->m_prev = set->m_prev;
    I3dDrawSet *next = at->m_next;
    set->m_next = next;
    next->m_prev = set;
    at->m_next = set;
    set->m_prev = at;
}

// ROR 0x0021b260
u_long *i3dDrawSetUseTex(I3dDrawSet *set, int n)
{
    set->m_tex_used[n]++;
    return set->m_tex_regs[n];
}

// ROR 0x0021b288
int i3dBatchClearTextures(I3dDrawSet *set)
{
    for (u_int i = 0; i < set->m_tex_count; i++)
        if (set->m_tex_used[i] != 0)
            i3dTexRegsSetNone(set->m_tex_regs[i]);
    return 0;
}

// ROR 0x0021b300
void i3dTexSetDescInit(I3dTexSetDesc *desc)
{
    desc->m_sort_param = 0;
    desc->m_depth = 0.0f;
    desc->m_tex_no = NULL;
    desc->m_tex_count = 0;
    desc->m_sort_func = 0;
    desc->m_sort_order = 0;
}

// ROR 0x0021b320
int i3dDrawSetInit(I3dDrawSet *set, I3dLayer *layer, const I3dTexSetDesc *desc, u_int hash)
{
    i3dContext *ctx = layer->m_ctx;
    set->m_hash = hash;
    set->m_next = set;
    set->m_prev = set;
    set->m_group = i3dCtxCreateGroup(ctx, desc->m_sort_func, desc->m_sort_order, desc->m_sort_param);
    set->m_layer = layer;
    set->m_tex_regs = NULL;
    set->m_reserved_next = NULL;
    set->m_tex_index = NULL;
    set->m_tex_count = desc->m_tex_count;
    set->m_depth = desc->m_depth;
    set->m_sort_func = desc->m_sort_func;
    set->m_sort_order = desc->m_sort_order;
    set->m_sort_param = desc->m_sort_param;
    if (desc->m_tex_count != 0)
    {
        // PC: host-sized arrays (the ROM: count words each, rounded to qwords).
        u_int n = desc->m_tex_count;
        u_int *index = (u_int *)i3dCtxAllocTail(ctx, (int)((n * sizeof(u_int) + 15) >> 4));
        set->m_tex_regs = (u_long **)i3dCtxAllocTail(ctx, (int)((n * sizeof(u_long *) + 15) >> 4));
        set->m_tex_used = (int *)i3dCtxAllocTail(ctx, (int)((n * sizeof(int) + 15) >> 4));
        set->m_tex_index = index;
        for (u_int i = 0; i < n; i++)
        {
            index[i] = (u_int)desc->m_tex_no[i];
            set->m_tex_regs[i] = (u_long *)i3dCtxAllocTail(ctx, 6);
            set->m_tex_used[i] = 0;
        }
    }
    return 0;
}

/* ---- tags and register writes ---- */

// ROR 0x0021b490
u_long *i3dPktDmaTag(u_long *pkt, u_int qwc, u_int id, const void *addr, u_int vif0, u_int vif1)
{
    pkt[0] = ((u_long)i3dDmaAddr(addr) << 32) | (u_long)(u_int)(id << 24) | (qwc & 0xffff);
    pkt[1] = ((u_long)vif1 << 32) | vif0;
    return pkt + 2;
}

// ROR 0x0021b4d0
u_long *i3dPktDirectTag(u_long *pkt, u_int qwc, u_int id, const void *addr)
{
    qwc &= 0xffff;
    return i3dPktDmaTag(pkt, qwc, id & 0xff, addr, I3D_VIF_FLUSHA, qwc | I3D_VIF_DIRECT);
}

// ROR 0x0021b4f8
u_long *i3dPktRefTexRegs(u_long *pkt, u_long *regs)
{
    return i3dPktDirectTag(pkt, 6, 0x30, regs);
}

// ROR 0x0021b518
u_long *i3dPktUseTexRegs(u_long *pkt, I3dDrawSet *set, int n)
{
    return i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(set, n));
}

// ROR 0x0021b550
u_long *i3dPktSetReg(u_long *pkt, u_long reg, u_long value)
{
    pkt[5] = reg;
    pkt[2] = I3D_GIF_AD;
    pkt[4] = value;
    pkt[3] = I3D_GS_A_D;
    return i3dPktDirectTag(pkt, 2, 0x10, NULL) + 4;
}

// ROR 0x0021b5a0
u_long *i3dPktCallView(u_long *pkt, I3dLayer *layer)
{
    if (layer->m_view == NULL)
        return pkt;
    pkt[1] = 0;
    pkt[0] = ((u_long)layer->m_view->m_packet << 32) | I3D_DMA_CALL;
    return pkt + 2;
}

// ROR 0x0021b5d0
u_long *i3dPktCallFog(u_long *pkt, I3dLayer *layer)
{
    I3dFogState *fog = (I3dFogState *)layer->m_fog;
    if (fog == NULL)
        return pkt;
    pkt[1] = 0;
    pkt[0] = ((u_long)fog->m_packet << 32) | I3D_DMA_CALL;
    return pkt + 2;
}

// ROR 0x0021b600
u_long *i3dPktSetAlphaMode(u_long *pkt, int mode)
{
    // ALPHA_1 = (A - B) * C + D with A / B / C / D in bits 0 / 2 / 4 / 6:
    // 0x44 (Cs - Cd) * As + Cd, 0x48 Cs * As + Cd, 0x42 Cd - Cs * As; the
    // 0x2x modes take the destination alpha (C = Ad); 1 keeps Cd.
    u_long alpha = 0;
    if (mode == 0)
        return pkt;
    switch (mode)
    {
    case 0x01: alpha = 0x40; break;
    case 0x02: alpha = 0; break;
    case 0x10: alpha = 0x44; break;
    case 0x11: alpha = 0x48; break;
    case 0x12: alpha = 0x42; break;
    case 0x20: alpha = 0x54; break;
    case 0x21: alpha = 0x58; break;
    case 0x22: alpha = 0x52; break;
    }
    return i3dPktSetReg(pkt, I3D_GS_ALPHA_1, alpha);
}

// ROR 0x0021b698
u_long *i3dPktOpenChain(u_char *pkt)
{
    // TEST_1 0x5000d: alpha test GEQUAL 0, Z test GEQUAL.
    return i3dPktSetReg(i3dPktSetAlphaMode((u_long *)pkt, 0x10), I3D_GS_TEST_1, 0x5000d);
}

// ROR 0x0021b6c8
u_long *i3dPktClearZ(u_long *pkt)
{
    // TEST_1 0x32001: alpha test NEVER failing to ZB_ONLY, Z test GREATER.
    pkt = i3dPktSetReg(pkt, I3D_GS_TEST_1, 0x32001);
    u_long *data = i3dPktDirectTag(pkt, 3, 0x10, NULL);
    data[3] = 0;                                    // XYZ2 0, 0
    data[0] = I3D_GIF_REGLIST(4);
    data[1] = (I3D_GS_NOP << 12) | (I3D_GS_XYZ2 << 8) | (I3D_GS_XYZ2 << 4) | I3D_GS_PRIM;
    data[2] = 6;                                    // PRIM: sprite
    data[4] = 0xffffffffull;                        // XYZ2 0xffff, 0xffff (12.4)
    // data[5], the NOP, is not written.
    return i3dPktSetReg(data + 6, I3D_GS_TEST_1, 0x5000d);
}

/* ---- primitives from float vectors (PACKED) ---- */

// ROR 0x0021b740
void i3dPktConvXYZ(int *out, const float *xyz)
{
    sceVu0FVECTOR v;
    i3dVecMul(v, xyz, s_screenScale);
    i3dVecAdd(v, v, s_screenOffset);
    i3dVecFtoi4(out, v);
}

// ROR 0x0021b790
void i3dPktConvRGBA(int *out, const float *rgba, const float *scale)
{
    sceVu0FVECTOR v;
    i3dVecMul2(v, rgba, scale);
    i3dVecFtoi0(out, v);
}

// ROR 0x0021b7c8
u_long *i3dPktFlatSpriteF(u_long *pkt, const float *xyz0, const float *xyz1, const float *rgba)
{
    pkt[2] = 0x3023400000008001ull;                 // PRIM 0x46, NREG 3
    pkt[3] = 0x551;
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 2), rgba, s_colorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 3), xyz0);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 4), xyz1);
    return i3dPktDirectTag(pkt, 4, 0x10, NULL) + 2 * 4;
}

// ROR 0x0021b860
u_long *i3dPktTexSpriteF(u_long *pkt, const float *xyz0, const float *xyz1, const float *uv0,
                         const float *uv1, const float *rgba)
{
    pkt[3] = 0x53531;
    pkt[2] = 0x50ab400000008001ull;                 // data 0x302850: PRIM 0x156, NREG 5
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 2), rgba, s_texColorScale);
    i3dVecFtoi4(I3D_PKT_QW(pkt, 3), uv0);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 4), xyz0);
    i3dVecFtoi4(I3D_PKT_QW(pkt, 5), uv1);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 6), xyz1);
    return i3dPktDirectTag(pkt, 6, 0x10, NULL) + 2 * 6;
}

// ROR 0x0021b928
u_long *i3dPktFlatTriangleF(u_long *pkt, const float *xyz0, const float *xyz1, const float *xyz2,
                            const float *rgba)
{
    pkt[3] = 0x5551;
    pkt[2] = 0x4021c00000008001ull;                 // data 0x302858: PRIM 0x43, NREG 4
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 2), rgba, s_colorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 3), xyz0);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 4), xyz1);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 5), xyz2);
    return i3dPktDirectTag(pkt, 5, 0x10, NULL) + 2 * 5;
}

// ROR 0x0021b9d0
u_long *i3dPktTexTriangleF(u_long *pkt, const float *xyz0, const float *xyz1, const float *xyz2,
                           const float *uv0, const float *uv1, const float *uv2, const float *rgba)
{
    pkt[3] = 0x5353531;
    pkt[2] = 0x70a9c00000008001ull;                 // data 0x302860: PRIM 0x153, NREG 7
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 2), rgba, s_texColorScale);
    i3dVecFtoi4(I3D_PKT_QW(pkt, 3), uv0);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 4), xyz0);
    i3dVecFtoi4(I3D_PKT_QW(pkt, 5), uv1);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 6), xyz1);
    i3dVecFtoi4(I3D_PKT_QW(pkt, 7), uv2);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 8), xyz2);
    return i3dPktDirectTag(pkt, 8, 0x10, NULL) + 2 * 8;
}

// ROR 0x0021bac8
u_long *i3dPktGouraudTriangleF(u_long *pkt, const float *xyz0, const float *xyz1,
                               const float *xyz2, const float *rgba0, const float *rgba1,
                               const float *rgba2)
{
    pkt[3] = 0x515151;
    pkt[2] = 0x6025c00000008001ull;                 // data 0x302868: PRIM 0x4b, NREG 6
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 2), rgba0, s_colorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 3), xyz0);
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 4), rgba1, s_colorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 5), xyz1);
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 6), rgba2, s_colorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 7), xyz2);
    return i3dPktDirectTag(pkt, 7, 0x10, NULL) + 2 * 7;
}

// ROR 0x0021bbb8
u_long *i3dPktGouraudTexTriangleF(u_long *pkt, const float *xyz0, const float *xyz1,
                                  const float *xyz2, const float *uv0, const float *uv1,
                                  const float *uv2, const float *rgba0, const float *rgba1,
                                  const float *rgba2)
{
    pkt[2] = 0x90adc00000008001ull;                 // data 0x302870: PRIM 0x15b, NREG 9
    pkt[3] = 0x513513513ull;                        // data 0x302878
    i3dVecFtoi4(I3D_PKT_QW(pkt, 2), uv0);
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 3), rgba0, s_texColorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 4), xyz0);
    i3dVecFtoi4(I3D_PKT_QW(pkt, 5), uv1);
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 6), rgba1, s_texColorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 7), xyz1);
    i3dVecFtoi4(I3D_PKT_QW(pkt, 8), uv2);
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 9), rgba2, s_texColorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 10), xyz2);
    return i3dPktDirectTag(pkt, 10, 0x10, NULL) + 2 * 10;
}

// ROR 0x0021bce0
u_long *i3dPktFlatLineF(u_long *pkt, const float *xyz0, const float *xyz1, const float *rgba)
{
    pkt[2] = 0x3020c00000008001ull;                 // PRIM 0x41, NREG 3
    pkt[3] = 0x551;
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 2), rgba, s_colorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 3), xyz0);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 4), xyz1);
    return i3dPktDirectTag(pkt, 4, 0x10, NULL) + 2 * 4;
}

// ROR 0x0021bd78
u_long *i3dPktTexLineF(u_long *pkt, const float *xyz0, const float *xyz1, const float *uv0,
                       const float *uv1, const float *rgba)
{
    pkt[3] = 0x53531;
    pkt[2] = 0x50a8c00000008001ull;                 // data 0x302880: PRIM 0x151, NREG 5
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 2), rgba, s_texColorScale);
    i3dVecFtoi4(I3D_PKT_QW(pkt, 3), uv0);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 4), xyz0);
    i3dVecFtoi4(I3D_PKT_QW(pkt, 5), uv1);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 6), xyz1);
    return i3dPktDirectTag(pkt, 6, 0x10, NULL) + 2 * 6;
}

// ROR 0x0021be40
u_long *i3dPktGouraudLineF(u_long *pkt, const float *xyz0, const float *xyz1, const float *rgba0,
                           const float *rgba1)
{
    pkt[3] = 0x5151;
    pkt[2] = 0x4024c00000008001ull;                 // data 0x302888: PRIM 0x49, NREG 4
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 2), rgba0, s_colorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 3), xyz0);
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 4), rgba1, s_colorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 5), xyz1);
    return i3dPktDirectTag(pkt, 5, 0x10, NULL) + 2 * 5;
}

// ROR 0x0021bef8
u_long *i3dPktGouraudTexLineF(u_long *pkt, const float *xyz0, const float *xyz1,
                              const float *uv0, const float *uv1, const float *rgba0,
                              const float *rgba1)
{
    pkt[3] = 0x513513;
    pkt[2] = 0x60acc00000008001ull;                 // data 0x302890: PRIM 0x159, NREG 6
    i3dVecFtoi4(I3D_PKT_QW(pkt, 2), uv0);
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 3), rgba0, s_texColorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 4), xyz0);
    i3dVecFtoi4(I3D_PKT_QW(pkt, 5), uv1);
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 6), rgba1, s_texColorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 7), xyz1);
    return i3dPktDirectTag(pkt, 7, 0x10, NULL) + 2 * 7;
}

// ROR 0x0021bfe8
u_long *i3dPktFlatPointF(u_long *pkt, const float *xyz, const float *rgba)
{
    pkt[2] = 0x2020400000008001ull;                 // PRIM 0x40, NREG 2
    pkt[3] = 0x51;
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 2), rgba, s_colorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 3), xyz);
    return i3dPktDirectTag(pkt, 3, 0x10, NULL) + 2 * 3;
}

// ROR 0x0021c068
u_long *i3dPktTexPointF(u_long *pkt, const float *xyz, const int *uv, const float *rgba)
{
    pkt[2] = 0x30a8400000008001ull;                 // PRIM 0x150, NREG 3
    pkt[3] = 0x513;
    i3dVecCopy((float *)I3D_PKT_QW(pkt, 2), (const float *)uv);
    i3dPktConvRGBA(I3D_PKT_QW(pkt, 3), rgba, s_texColorScale);
    i3dPktConvXYZ(I3D_PKT_QW(pkt, 4), xyz);
    return i3dPktDirectTag(pkt, 4, 0x10, NULL) + 2 * 4;
}

// ROR 0x0021c100
void i3dPktSetScreenOffset(const float *offset)
{
    i3dVecCopy(s_screenOffset, offset);
}

// ROR 0x0021c120
void i3dPktSetScreenScale(const float *scale)
{
    i3dVecCopy(s_screenScale, scale);
}

/* ---- primitives from packed values (REGLIST) ---- */

// ROR 0x0021c140
u_long *i3dPktSprite(u_long *pkt, u_long xyz0, u_long xyz1, u_long rgbaq)
{
    pkt[7] = xyz1;
    pkt[2] = I3D_GIF_REGLIST(4);
    pkt[6] = xyz0;
    pkt[5] = rgbaq;
    pkt[3] = 0x5510;
    pkt[4] = 0x46;                                  // sprite, ABE
    return i3dPktDirectTag(pkt, 3, 0x10, NULL) + 2 * 3;
}

// ROR 0x0021c1a0
u_long *i3dPktTexSpriteUV(u_long *pkt, u_long xyz0, u_long xyz1, u_long uv0, u_long uv1,
                          u_long rgbaq)
{
    pkt[9] = xyz1;
    pkt[5] = rgbaq;
    pkt[8] = uv1;
    pkt[2] = I3D_GIF_REGLIST(6);
    pkt[7] = xyz0;
    pkt[3] = 0x535310;
    pkt[6] = uv0;
    pkt[4] = 0x156;                                 // sprite, TME, ABE, FST
    return i3dPktDirectTag(pkt, 4, 0x10, NULL) + 2 * 4;
}

// ROR 0x0021c208
u_long *i3dPktTexSpriteST(u_long *pkt, u_long xyz0, u_long xyz1, u_long st0, u_long st1,
                          u_long rgbaq)
{
    pkt[9] = xyz1;
    pkt[5] = rgbaq;
    pkt[8] = st1;
    pkt[2] = I3D_GIF_REGLIST(6);
    pkt[7] = xyz0;
    pkt[3] = 0x525210;
    pkt[6] = st0;
    pkt[4] = 0x56;                                  // sprite, TME, ABE
    return i3dPktDirectTag(pkt, 4, 0x10, NULL) + 2 * 4;
}

// ROR 0x0021c270
u_long *i3dPktTriangle(u_long *pkt, u_long xyz0, u_long xyz1, u_long xyz2, u_long rgbaq)
{
    pkt[8] = xyz2;
    pkt[5] = rgbaq;
    pkt[2] = I3D_GIF_REGLIST(6);
    pkt[6] = xyz0;
    pkt[7] = xyz1;
    pkt[3] = 0xf55510;
    pkt[4] = 0x43;                                  // triangle, ABE
    // pkt[9], the NOP, is not written.
    return i3dPktDirectTag(pkt, 4, 0x10, NULL) + 2 * 4;
}

// ROR 0x0021c2d8
u_long *i3dPktTexTriangleUV(u_long *pkt, u_long xyz0, u_long xyz1, u_long xyz2, u_long uv0,
                            u_long uv1, u_long uv2, u_long rgbaq)
{
    pkt[11] = xyz2;
    pkt[5] = rgbaq;
    pkt[6] = uv0;
    pkt[2] = I3D_GIF_REGLIST(8);
    pkt[7] = xyz0;
    pkt[8] = uv1;
    pkt[3] = 0x53535310;
    pkt[9] = xyz1;
    pkt[10] = uv2;
    pkt[4] = 0x153;                                 // triangle, TME, ABE, FST
    return i3dPktDirectTag(pkt, 5, 0x10, NULL) + 2 * 5;
}

// ROR 0x0021c348
u_long *i3dPktTexTriangleST(u_long *pkt, u_long xyz0, u_long xyz1, u_long xyz2, u_long st0,
                            u_long st1, u_long st2, u_long rgbaq)
{
    pkt[11] = xyz2;
    pkt[5] = rgbaq;
    pkt[6] = st0;
    pkt[2] = I3D_GIF_REGLIST(8);
    pkt[7] = xyz0;
    pkt[8] = st1;
    pkt[3] = 0x52525210;
    pkt[9] = xyz1;
    pkt[10] = st2;
    pkt[4] = 0x53;                                  // triangle, TME, ABE
    return i3dPktDirectTag(pkt, 5, 0x10, NULL) + 2 * 5;
}

// ROR 0x0021c3b8
u_long *i3dPktGouraudTriangle(u_long *pkt, u_long xyz0, u_long xyz1, u_long xyz2,
                              u_long rgbaq0, u_long rgbaq1, u_long rgbaq2)
{
    pkt[11] = xyz2;
    pkt[6] = rgbaq0;
    pkt[8] = rgbaq1;
    pkt[2] = I3D_GIF_REGLIST(8);
    pkt[7] = xyz0;
    pkt[10] = rgbaq2;
    pkt[3] = 0x515151f0;
    pkt[9] = xyz1;
    pkt[4] = 0x4b;                                  // triangle, IIP, ABE
    // pkt[5], the NOP, is not written.
    return i3dPktDirectTag(pkt, 5, 0x10, NULL) + 2 * 5;
}

// ROR 0x0021c428
u_long *i3dPktGouraudTexTriangleUV(u_long *pkt, u_long xyz0, u_long xyz1, u_long xyz2,
                                   u_long uv0, u_long uv1, u_long uv2, u_long rgbaq0,
                                   u_long rgbaq1, u_long rgbaq2)
{
    pkt[13] = xyz2;
    pkt[5] = uv0;
    pkt[6] = rgbaq0;
    pkt[2] = I3D_GIF_REGLIST(10);
    pkt[7] = xyz0;
    pkt[8] = uv1;
    pkt[3] = 0x5135135130ull;
    pkt[4] = 0x15b;                                 // triangle, IIP, TME, ABE, FST
    pkt[10] = xyz1;
    pkt[9] = rgbaq1;
    pkt[11] = uv2;
    pkt[12] = rgbaq2;
    return i3dPktDirectTag(pkt, 6, 0x10, NULL) + 2 * 6;
}

// ROR 0x0021c4b0
u_long *i3dPktGouraudTexTriangleST(u_long *pkt, u_long xyz0, u_long xyz1, u_long xyz2,
                                   u_long st0, u_long st1, u_long st2, u_long rgbaq0,
                                   u_long rgbaq1, u_long rgbaq2)
{
    pkt[13] = xyz2;
    pkt[5] = st0;
    pkt[6] = rgbaq0;
    pkt[2] = I3D_GIF_REGLIST(10);
    pkt[7] = xyz0;
    pkt[8] = st1;
    pkt[3] = 0x5125125120ull;
    pkt[4] = 0x5b;                                  // triangle, IIP, TME, ABE
    pkt[10] = xyz1;
    pkt[9] = rgbaq1;
    pkt[11] = st2;
    pkt[12] = rgbaq2;
    return i3dPktDirectTag(pkt, 6, 0x10, NULL) + 2 * 6;
}

// ROR 0x0021c538
u_long *i3dPktLine(u_long *pkt, u_long xyz0, u_long xyz1, u_long rgbaq)
{
    pkt[7] = xyz1;
    pkt[2] = I3D_GIF_REGLIST(4);
    pkt[6] = xyz0;
    pkt[5] = rgbaq;
    pkt[3] = 0x5510;
    pkt[4] = 0x41;                                  // line, ABE
    return i3dPktDirectTag(pkt, 3, 0x10, NULL) + 2 * 3;
}

// ROR 0x0021c598
u_long *i3dPktTexLineUV(u_long *pkt, u_long xyz0, u_long xyz1, u_long uv0, u_long uv1,
                        u_long rgbaq)
{
    pkt[9] = xyz1;
    pkt[5] = rgbaq;
    pkt[8] = uv1;
    pkt[2] = I3D_GIF_REGLIST(6);
    pkt[7] = xyz0;
    pkt[3] = 0x535310;
    pkt[6] = uv0;
    pkt[4] = 0x151;                                 // line, TME, ABE, FST
    return i3dPktDirectTag(pkt, 4, 0x10, NULL) + 2 * 4;
}

// ROR 0x0021c600
u_long *i3dPktTexLineST(u_long *pkt, u_long xyz0, u_long xyz1, u_long st0, u_long st1,
                        u_long rgbaq)
{
    pkt[9] = xyz1;
    pkt[5] = rgbaq;
    pkt[8] = st1;
    pkt[2] = I3D_GIF_REGLIST(6);
    pkt[7] = xyz0;
    pkt[3] = 0x525210;
    pkt[6] = st0;
    pkt[4] = 0x51;                                  // line, TME, ABE
    return i3dPktDirectTag(pkt, 4, 0x10, NULL) + 2 * 4;
}

// ROR 0x0021c668
u_long *i3dPktGouraudLine(u_long *pkt, u_long xyz0, u_long xyz1, u_long rgbaq0, u_long rgbaq1)
{
    pkt[9] = xyz1;
    pkt[8] = rgbaq1;
    pkt[2] = I3D_GIF_REGLIST(6);
    pkt[6] = rgbaq0;
    pkt[7] = xyz0;
    pkt[3] = 0x5151f0;
    pkt[4] = 0x49;                                  // line, IIP, ABE
    // pkt[5], the NOP, is not written.
    return i3dPktDirectTag(pkt, 4, 0x10, NULL) + 2 * 4;
}

// ROR 0x0021c6d0
u_long *i3dPktGouraudTexLineUV(u_long *pkt, u_long xyz0, u_long xyz1, u_long uv0, u_long uv1,
                               u_long rgbaq0, u_long rgbaq1)
{
    pkt[10] = xyz1;
    pkt[6] = rgbaq0;
    pkt[8] = uv1;
    pkt[2] = I3D_GIF_REGLIST(8);
    pkt[7] = xyz0;
    pkt[9] = rgbaq1;
    pkt[3] = 0xf5135130ull;
    pkt[5] = uv0;
    pkt[4] = 0x159;                                 // line, IIP, TME, ABE, FST
    // pkt[11], the NOP, is not written.
    return i3dPktDirectTag(pkt, 5, 0x10, NULL) + 2 * 5;
}

// ROR 0x0021c740
u_long *i3dPktGouraudTexLineST(u_long *pkt, u_long xyz0, u_long xyz1, u_long st0, u_long st1,
                               u_long rgbaq0, u_long rgbaq1)
{
    pkt[10] = xyz1;
    pkt[6] = rgbaq0;
    pkt[8] = st1;
    pkt[2] = I3D_GIF_REGLIST(8);
    pkt[7] = xyz0;
    pkt[9] = rgbaq1;
    pkt[3] = 0xf5125120ull;
    pkt[5] = st0;
    pkt[4] = 0x59;                                  // line, IIP, TME, ABE
    // pkt[11], the NOP, is not written.
    return i3dPktDirectTag(pkt, 5, 0x10, NULL) + 2 * 5;
}

// ROR 0x0021c7b0
u_long *i3dPktPoint(u_long *pkt, u_long xyz, u_long rgbaq)
{
    pkt[6] = xyz;
    pkt[2] = I3D_GIF_REGLIST(4);
    pkt[5] = rgbaq;
    pkt[3] = 0xf510;
    pkt[4] = 0x40;                                  // point, ABE
    // pkt[7], the NOP, is not written.
    return i3dPktDirectTag(pkt, 3, 0x10, NULL) + 2 * 3;
}

// ROR 0x0021c808
u_long *i3dPktTexPointUV(u_long *pkt, u_long xyz, u_long uv, u_long rgbaq)
{
    pkt[7] = xyz;
    pkt[2] = I3D_GIF_REGLIST(4);
    pkt[5] = uv;
    pkt[6] = rgbaq;
    pkt[3] = 0x5130;
    pkt[4] = 0x150;                                 // point, TME, ABE, FST
    return i3dPktDirectTag(pkt, 3, 0x10, NULL) + 2 * 3;
}

// ROR 0x0021c868
u_long *i3dPktTexPointST(u_long *pkt, u_long xyz, u_long st, u_long rgbaq)
{
    pkt[7] = xyz;
    pkt[2] = I3D_GIF_REGLIST(4);
    pkt[5] = st;
    pkt[6] = rgbaq;
    pkt[3] = 0x5120;
    pkt[4] = 0x50;                                  // point, TME, ABE
    return i3dPktDirectTag(pkt, 3, 0x10, NULL) + 2 * 3;
}

/* ---- packers ---- */

static inline u_int i3dFloatBits(float f)
{
    u_int bits;
    memcpy(&bits, &f, sizeof(bits));
    return bits;
}

// ROR 0x0021c8c8
u_long i3dPackRGBAQ(float q, u_int r, u_int g, u_int b, u_int a)
{
    return r | ((u_long)i3dFloatBits(q) << 32) | ((u_long)g << 8) | ((u_long)b << 16)
         | ((u_long)a << 24);
}

// ROR 0x0021c900
u_long i3dPackST(float s, float t)
{
    return (u_long)i3dFloatBits(s) | ((u_long)i3dFloatBits(t) << 32);
}

// ROR 0x0021c928
u_long i3dPackUV(u_int u, u_int v)
{
    return u | ((u_long)v << 16);
}

// ROR 0x0021c940
u_long i3dPackXYZ(u_int x, u_int y, u_int z)
{
    return x | ((u_long)y << 16) | ((u_long)z << 32);
}
