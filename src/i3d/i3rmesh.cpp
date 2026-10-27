/* ==========================================================================
 *  i3d/i3rmesh.cpp
 *
 *  The i3r material packets (ROR 0x228c08..0x22a6c0). See i3rmesh.h.
 *
 *  A material packet is a chain of VIF / DMA tags at the context head:
 *  first the view and fog (i3rPktMeshEnv), then per layer of the material
 *  its texture registers (a REF of the draw set's slot), its GS state and
 *  colour block (i3rPktLayerState), its lights (i3rPktLayerLights), its
 *  STQ map matrices (i3rPktLayerStqMaps) and its uv transform with the
 *  strip program's flags (i3rPktLayerUvXform). The numbers written next to
 *  the data (400, 0x3f8, 0x168 ...) are VU1 addresses and words of the
 *  ROM's microprogram, kept as they are.
 *
 *  PORT: the VU1 writers that call i3rGetMaterialPacket are not on the
 *  host (i3d/i3rdraw.cpp); the native mesh draw takes each material's TEST
 *  and ALPHA from i3rPktLayerTest / i3rPktLayerAlpha. Pointers in the cache
 *  key are folded to their low 32 bits on the host.
 * ======================================================================== */
#include "i3d/i3rmesh.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "i3d/i3dctx.h"
#include "i3d/i3denv.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dview.h"
#include "i3d/i3rmodel.h"
#include "i3d/i3rtype.h"
#include "lib/unassigned.h"

/* GS registers written through A+D here (i3dpacket.h has the rest). */
#define I3R_GS_FOGCOL    0x3dull
#define I3R_GS_SCISSOR_1 0x40ull

/* ROR 0x002db350: the layer's alpha test (m_atst) -> ATST. PC: 16 bytes
 * (the ROM indexes with the raw byte; the bytes after the nine entries are
 * zero). */
static const u_char s_atst[16] = {1, 0, 1, 4, 7, 2, 3, 6, 5, 0, 0, 0, 0, 0, 0, 0};

/* ROR 0x003037c0: the qword a textured layer REFs after its texture
 * registers (VU1 word 10, MSCAL). */
alignas(16) static const u_int s_tex_kick[4] = {10, 0, 0x14000001, 0};

/* ROR 0x003037d0 / 0x003037e0: the colour scale of a textured / untextured
 * layer (RGB 128 = 1.0 under a texture; 255 without one; alpha 128). */
alignas(16) static const float s_colour_tex[4] = {128.0f, 128.0f, 128.0f, 128.0f};
alignas(16) static const float s_colour_flat[4] = {255.0f, 255.0f, 255.0f, 128.0f};

/* ROR 0x00303800: camera space -> projected texture space (x, y halved and
 * centred on 0.5). */
alignas(16) static sceVu0FMATRIX s_stq_bias = {
    {0.5f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.5f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.5f, 0.5f, 1.0f, 0.0f},
};

/* A layer's register override list (its child 2's child 0's data, type
 * 0x4a's slot): each record ORs its mask with the layer's user mask word
 * of the same register and takes the masked bits from the user value.
 * From the file: plain EE layout. */
struct I3rRegOverride
{
    u_int   m_reg;              // ROR 0x00: the GS register, also the I3dLayer::m_user index
    u_int   _04;
    u_long  m_value;            // ROR 0x08
    u_long  m_mask;             // ROR 0x10: set bits come from the layer's user value
};
static_assert(offsetof(I3rRegOverride, m_value) == 0x08, "I3rRegOverride::m_value");
static_assert(offsetof(I3rRegOverride, m_mask) == 0x10, "I3rRegOverride::m_mask");
static_assert(sizeof(I3rRegOverride) == 0x18, "I3rRegOverride is 0x18 bytes on the EE");

struct I3rRegOverrideList
{
    u_int   m_list;             // ROR 0x00: relocated: the records
    u_char  m_count;            // ROR 0x04
};
static_assert(offsetof(I3rRegOverrideList, m_count) == 0x04, "I3rRegOverrideList::m_count");

static u_long *i3rPktMeshEnv(u_long *pkt, I3dView *view, const I3dFogState *fog);
static u_long *i3rPktLayerState(u_long *pkt, I3dLayer *layer, const I3rItem *lay, const I3dFogState *fog,
                                const I3dLight *light, int textured, int not_first);
static u_long *i3rPktLayerLights(u_long *pkt, const I3rLayerData *d, I3dView *view, const I3dLight *light);
static u_long *i3rPktLayerStqMaps(u_long *pkt, const I3rItem *lay, I3dView *view, u_char *written);
static u_long *i3rPktLayerUvXform(float t, u_long *pkt, const I3rItem *lay, const I3dFogState *fog,
                                  const I3rMeshOpt *opt, int textured, u_char stq);

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00228c08

void i3rMeshOptClear(I3rMeshOpt *opt)
{
    i3dVecClear(opt->m_uv_scale);
    i3dVecClear(opt->m_uv_offset);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00228c38

void i3dMatGetScaleTrans(sceVu0FVECTOR *out, sceVu0FVECTOR *m)
{
    out[0][3] = 0.0f;
    out[0][0] = m[0][0];
    out[0][1] = m[1][1];
    out[0][2] = m[2][2];
    i3dVecCopy(out[1], m[3]);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00228c78

u_int i3rAlphaFailMode(u_int mode)
{
    if (mode == 0x10)
        return 1;
    if (mode < 0x11)
    {
        if (mode != 0xf)
            return 0;
        return 2;
    }
    if (mode != 0x18)
        return 0;
    return 3;
}

// ──────────────────────────────────────────────────────────────────────
// The packet of every layer of e's material, at the context head; each
// layer's DMA address goes to e->m_layer_pkt.
// ROR 0x00228ce8

static void i3rBuildMaterialPacket(I3rMeshPacket *e, I3dLayer *layer, const I3rMeshOpt *opt)
{
    u_int tex = 0;
    i3dContext *ctx = layer->m_ctx;
    I3dView *view = e->m_view;
    const I3dFogState *fog = e->m_fog;
    const I3dLight *light = e->m_light;
    u_long *pkt = (u_long *)i3dCtxGetHead(ctx);

    e->m_pkt = i3dDmaAddr(pkt);
    pkt = i3rPktMeshEnv(pkt, view, fog);
    const I3rItem *layers = e->m_material->getChildren();
    u_int count = layers->getNumChild();
    for (u_int i = 0; i < count; i++)
    {
        const I3rItem *lay = &layers->getChildren()[i];
        const I3rLayerData *d = (const I3rLayerData *)lay->getData();
        u_char stq = 0;

        e->m_layer_pkt[i] = i3dDmaAddr(pkt);
        int textured = d->m_tex_index >= 0;
        if (textured)
        {
            u_long *regs = i3dDrawSetUseTex(e->m_set, tex++);
            pkt = i3dPktDmaTag(pkt, 6, 0x30, regs, 0x1000101, 0x6c068001);
            pkt = i3dPktDmaTag(pkt, 0, 0x10, NULL, 0x64018000, 0x11);
            pkt = i3dPktDmaTag(pkt, 1, 0x30, s_tex_kick, 6, 0x60028007);
        }
        pkt = i3rPktLayerState(pkt, layer, lay, fog, light, textured, i != 0);
        pkt = i3rPktLayerLights(pkt, d, view, light);
        pkt = i3rPktLayerStqMaps(pkt, lay, view, &stq);
        pkt = i3rPktLayerUvXform(0.0f, pkt, lay, fog, opt, textured, stq);
    }
    i3dCtxSetHead(ctx, (u_char *)pkt);
}

// ──────────────────────────────────────────────────────────────────────
// Links e into the circular list before at.
// ROR 0x00228f10

static void i3rMeshPacketInsert(I3rMeshPacket *e, I3rMeshPacket *at)
{
    I3rMeshPacket *prev = at->m_prev;
    e->m_prev = prev;
    prev->m_next = e;
    at->m_prev = e;
    e->m_next = at;
}

// ──────────────────────────────────────────────────────────────────────
// Unlinks e and links it again before at.
// ROR 0x00228f28

static void i3rMeshPacketMoveBefore(I3rMeshPacket *e, I3rMeshPacket *at)
{
    I3rMeshPacket *prev = e->m_prev;
    I3rMeshPacket *next = e->m_next;
    next->m_prev = prev;
    prev->m_next = next;
    prev = at->m_prev;
    e->m_prev = prev;
    prev->m_next = e;
    at->m_prev = e;
    e->m_next = at;
}

// ──────────────────────────────────────────────────────────────────────
// A material's packet: looked up in the layer's 256 buckets by material,
// texture frame, light, fog and view (a hit moves to its bucket's front),
// else built. A draw with an option record that sets a uv override or a
// depth offset is never cached (its hash stays 0).
// ROR 0x00228f50

I3rMeshPacket *i3rGetMaterialPacket(I3dLayer *layer, const I3rItem *mat, const I3rMeshOpt *opt)
{
    I3rMeshPacket *head = NULL;
    u_int hash = 0;
    u_int bucket = 0;
    bool cached = true;
    const I3rMaterialData *md = (const I3rMaterialData *)mat->getData();
    u_int frame = (u_short)md->m_tex_frame;

    if (opt != NULL)
    {
        if (opt->m_uv_set == 0)
        {
            if (opt->m_depth != 0.0f)
                cached = false;
        }
        else
            cached = false;
    }
    if (cached)
    {
        // PORT: the ROM adds the pointers themselves; the host their low words.
        hash = (u_int)(uintptr_t)mat + frame + (u_int)(uintptr_t)layer->m_fog +
               (u_int)(uintptr_t)layer->m_view + (u_int)(uintptr_t)layer->m_light;
        hash += hash >> 16;
        hash += hash >> 8;
        bucket = hash & 0xff;
        head = layer->m_mesh_cache[bucket];
        if (head != NULL)
        {
            I3rMeshPacket *e = head;
            do
            {
                if (e->m_hash == hash && e->m_material == mat && e->m_frame == frame &&
                    e->m_light == layer->m_light && e->m_fog == layer->m_fog && e->m_view == layer->m_view)
                {
                    if (e == head)
                        return e;
                    i3rMeshPacketMoveBefore(e, head);
                    layer->m_mesh_cache[bucket] = e;
                    return e;
                }
                e = e->m_prev;
            } while (e != head);
        }
    }

    i3dContext *ctx = layer->m_ctx;
    I3rMeshPacket *e = (I3rMeshPacket *)i3dCtxAllocTail(ctx, I3D_QWC_OF(I3rMeshPacket));
    e->m_hash = hash;
    e->m_prev = e;
    e->m_next = e;
    I3dDrawSet *set = i3dLayerFindMaterialDrawSet(layer, mat, opt);
    e->m_set = set;
    e->m_group = set->m_group;
    e->m_sort = md->m_sort_func != 0;
    u_int count = mat->getChildren()->getNumChild();
    e->m_count = count;
    e->m_layer_pkt = (u_int *)i3dCtxAllocTail(ctx, (int)(count * 4 + 0xf) >> 4);
    e->m_fog = (I3dFogState *)layer->m_fog;
    e->m_light = layer->m_light;
    e->m_view = layer->m_view;
    e->m_material = mat;
    e->m_frame = frame;
    i3rBuildMaterialPacket(e, layer, opt);
    if (cached)
    {
        if (head != NULL)
            i3rMeshPacketInsert(e, head);
        layer->m_mesh_cache[bucket] = e;
    }
    return e;
}

// ──────────────────────────────────────────────────────────────────────
// The material packet's start: SCISSOR (and FOGCOL with a fog) through
// A+D, the fog state, the view's eye direction, its world -> screen and
// world -> clip matrices and screen scale / offset to VU1; a RET tag.
// ROR 0x002291c8

static u_long *i3rPktMeshEnv(u_long *pkt, I3dView *view, const I3dFogState *fog)
{
    u_char *base = (u_char *)pkt;
    u_char *at = base;
    u_char *p;

    ((u_int *)(base + 0x10))[0] = 0x11;
    ((u_int *)(base + 0x10))[1] = 2;
    ((u_long *)(base + 0x20))[0] = 0x1000000000008001ull;      // NLOOP 1, EOP, NREG 1
    ((u_long *)(base + 0x20))[1] = I3D_GS_A_D;
    ((u_long *)(base + 0x30))[0] = view->m_scissor;
    ((u_long *)(base + 0x30))[1] = I3R_GS_SCISSOR_1;
    if (fog != NULL)
    {
        ((u_long *)(base + 0x40))[0] = fog->m_fogcol;
        ((u_long *)(base + 0x40))[1] = I3R_GS_FOGCOL;
        *(u_short *)(base + 0x20) = 0x8002;                     // NLOOP 2
        ((u_int *)(base + 0x10))[1] += 1;
        at = base + 0x10;
    }
    ((u_int *)(at + 0x40))[0] = 400;
    ((u_int *)(at + 0x40))[1] = 0x3fa;
    if (fog == NULL)
    {
        ((u_int *)(at + 0x40))[2] = 0x3f9;
        p = at + 0x50;
    }
    else
    {
        ((u_int *)(at + 0x40))[2] = 0x3f8;
        memcpy(at + 0x50, fog, 0x10);
        p = at + 0x60;
    }
    memcpy(p, view->m_eye_dir, 0x10);
    ((u_int *)p)[4] = 400;
    ((u_int *)p)[6] = 0x3e2;
    ((u_int *)p)[5] = 0x3ec;
    i3dMatCopy((sceVu0FVECTOR *)(p + 0x20), view->m_world_screen);
    i3dMatCopy((sceVu0FVECTOR *)(p + 0x60), view->m_world_clip);
    i3dVecCopy((float *)(p + 0xa0), view->m_screen_scale);
    i3dVecCopy((float *)(p + 0xb0), view->m_screen_offset);
    u_int *end = (u_int *)(p + 0xc0);
    end[2] = 0;
    u_int qwc = (u_int)((u_char *)end - base) >> 4;
    end[0] = (qwc - 1) | 0x60028000;
    end[3] = 0x14000001;
    end[1] = 10;
    i3dPktDmaTag(pkt, qwc & 0xffff, 0x60, NULL, 0x1000101, (qwc - 1) * 0x10000 | 0x6c008000);
    return (u_long *)(p + 0xd0);
}

// ──────────────────────────────────────────────────────────────────────
// A layer's GS state through A+D -- TEST, ALPHA, the register overrides
// -- then its VU1 block: the strip GIF tag (PRIM: Gouraud fans unless the
// layer is flat, textured, fogged, blended), the colour scale, the
// ambient, the specular colour of a specular layer, and a marker word for
// every layer after the first.
// ROR 0x00229380

static u_long *i3rPktLayerState(u_long *pkt, I3dLayer *layer, const I3rItem *lay, const I3dFogState *fog,
                                const I3dLight *light, int textured, int not_first)
{
    u_char *base = (u_char *)pkt;
    u_char *regs = base + 0x30;
    const I3rLayerData *d = (const I3rLayerData *)lay->getData();
    u_char shade = d->m_shade;

    *(u_int *)(base + 0x10) = 0x11;
    u_long *p = i3rPktLayerTest((u_long *)regs, d);
    p = i3rPktLayerAlpha(p, d);
    if (i3rHasData26_2(lay))
    {
        I3rItem *slot = i3rGetSlot26_2(lay);
        if (i3rHasData4A(slot))
        {
            // PORT: the list word is read as a relative pointer (i3rroot.h).
            const I3rRegOverrideList *list = (const I3rRegOverrideList *)i3rGetSlot4A(slot)->getData();
            const I3rRegOverride *r = (const I3rRegOverride *)i3rPtr(&list->m_list);
            const I3rRegOverride *end = r + list->m_count;
            for (; r < end; r++)
            {
                u_long mask = r->m_mask | layer->m_user[r->m_reg][1];
                p[0] = (layer->m_user[r->m_reg][0] & mask) | (r->m_value & ~mask);
                p[1] = r->m_reg;
                p += 2;
            }
        }
    }
    int bytes = (int)((u_char *)p - regs);
    *(u_int *)(base + 0x14) = (u_int)(bytes >> 4) + 1;
    ((u_long *)(base + 0x20))[0] = (u_long)(bytes >> 4) | 0x1000000000008000ull;    // NREG 1, EOP
    ((u_long *)(base + 0x20))[1] = I3D_GS_A_D;

    u_char *q = (u_char *)p;
    ((u_int *)q)[0] = 400;
    ((u_int *)q)[1] = shade == 2 ? 0x3f8 : 0x3f7;
    ((u_int *)q)[2] = 0x3f4;
    u_int prim = (u_int)textured << 4 | (d->m_cull == 3 ? 5 : 0xd);
    prim |= fog != NULL ? 0x60 : 0x40;
    ((u_long *)(q + 0x10))[0] = (u_long)prim << 47 | 0x4000400000008000ull;      // NREG 4, PRE, EOP
    ((u_long *)(q + 0x10))[1] = 0x512a;
    memcpy(q + 0x20, textured ? s_colour_tex : s_colour_flat, 0x10);
    float *ambient = (float *)(q + 0x30);
    if (shade == 0)
    {
        ambient[0] = ambient[1] = ambient[2] = ambient[3] = 1.0f;
    }
    else if (d->m_ambient == 0)
    {
        ambient[0] = ambient[1] = ambient[2] = 0.0f;
        ambient[3] = 1.0f;
    }
    else
        memcpy(ambient, light->m_ambient, 0x10);
    u_char *r = q + 0x40;
    if (shade == 2)
    {
        memcpy(r, d->m_specular, 0x10);
        r += 0x10;
    }
    if (not_first)
    {
        *(u_int *)r = 0x15e;
        r += 0x10;
    }
    ((u_int *)r)[2] = 0;
    u_int qwc = (u_int)(r - base) >> 4;
    ((u_int *)r)[0] = (qwc - 1) | 0x60028000;
    ((u_int *)r)[3] = 0x14000001;
    ((u_int *)r)[1] = 10;
    i3dPktDmaTag(pkt, qwc & 0xffff, 0x10, NULL, 0x1000101, (qwc - 1) * 0x10000 | 0x6c008000);
    return (u_long *)(r + 0x10);
}

/* One of i3rPktLayerLights' lists after the parallel lights: groups of up
 * to four records of qw qwords (a VU1 address word, then the light's
 * qwords in the order given), each group in its own UNPACK. addr 0 copies
 * nothing (a shading byte the list has no address for). */
static u_char *i3rPktLightList(u_char *p, u_int count, const u_char *list, u_int stride, u_int qw,
                               u_int addr, const u_char *order)
{
    u_int i = 0;
    if (count == 0)
        return p;
    do
    {
        u_int end = i + 4 < count ? i + 4 : count;
        u_int n = end - i;
        u_int *rec;
        ((u_int *)p)[3] = n * qw * 0x10000 | 0x6c008000;
        p += 0x10;
        do
        {
            rec = (u_int *)p;
            if (addr != 0)
            {
                rec[0] = addr;
                for (u_int k = 0; k + 1 < qw; k++)
                    memcpy(p + 0x10 + k * 0x10, list + i * stride + order[k], 0x10);
            }
            i++;
            p += qw * 0x10;
        } while (i < end);
        ((u_int *)p)[1] = 0;
        ((u_int *)p)[0] = n * qw | 0x60018000;
        ((u_int *)p)[2] = 0x14000001;
    } while (i < count);
    return p;
}

// ──────────────────────────────────────────────────────────────────────
// A lit layer's light set to VU1: the parallel lights four at a time,
// their directions transposed (shading 2: the half vectors with the view's
// eye direction), then the point, spot and the two 0x30 kinds; a CNT tag.
// ROR 0x00229678

static u_long *i3rPktLayerLights(u_long *pkt, const I3rLayerData *d, I3dView *view, const I3dLight *light)
{
    static const u_char kPointOrder[] = {0x00, 0x10};
    static const u_char kSpotOrder[] = {0x00, 0x20, 0x10, 0x30};     // position, colour, direction, cone
    static const u_char kVec3Order[] = {0x00, 0x10, 0x20};
    u_char shade = d->m_shade;
    u_char *start = (u_char *)pkt;
    u_char *p = start;

    if (shade == 0)
        return pkt;
    u_int count = (u_int)light->m_parallel_count;
    if (count != 0)
    {
        u_int i = 0;
        do
        {
            u_int n = count - i;
            float *rows = (float *)(p + 0x20);      // x, y, z of four directions
            u_char *colour = p + 0x50;
            ((u_int *)p)[3] = 0x6c088000;
            if (n < 5)
                memset(p + 0x20, 0, 0x70);
            else
                n = 4;
            if (shade == 1)
            {
                ((u_int *)p)[4] = 0x39;
                for (u_int k = 0; k < n; k++, i++)
                {
                    const I3dParallelLight *l = &light->m_parallel[i];
                    rows[k] = l->m_dir[0];
                    rows[4 + k] = l->m_dir[1];
                    rows[8 + k] = l->m_dir[2];
                    memcpy(colour + k * 0x10, l->m_color, 0x10);
                }
            }
            else if (shade == 2)
            {
                ((u_int *)p)[4] = 0x66;
                for (u_int k = 0; k < n; k++, i++)
                {
                    const I3dParallelLight *l = &light->m_parallel[i];
                    float half[4];
                    i3dVu0VecAdd(half, l->m_dir, view->m_eye_dir);
                    i3dVecNormalize(half, half);
                    rows[k] = half[0];
                    rows[4 + k] = half[1];
                    rows[8 + k] = half[2];
                    memcpy(colour + k * 0x10, l->m_color, 0x10);
                }
            }
            p += 0x90;
            ((u_int *)p)[0] = 0x14000008;
            ((u_int *)p)[1] = 0;
            ((u_int *)p)[2] = 0;
        } while (i < count);
    }
    p = i3rPktLightList(p, (u_int)light->m_point_count, (const u_char *)light->m_point, sizeof(I3dPointLight),
                        3, shade == 1 ? 200 : shade == 2 ? 0x108 : 0, kPointOrder);
    p = i3rPktLightList(p, (u_int)light->m_spot_count, (const u_char *)light->m_spot, sizeof(I3dSpotLight),
                        5, shade == 1 ? 0x19a : shade == 2 ? 0x212 : 0, kSpotOrder);
    p = i3rPktLightList(p, (u_int)light->m_dir2_count, (const u_char *)light->m_dir2, sizeof(I3dDir2Light),
                        4, 700, kVec3Order);
    p = i3rPktLightList(p, (u_int)light->m_vec3_count, (const u_char *)light->m_vec3, sizeof(I3dVec3Light),
                        4, 0x2e4, kVec3Order);
    ((u_int *)p)[3] = 0;
    u_int qwc = (u_int)(p - start) >> 4;
    p += 0x10;
    i3dPktDmaTag(pkt, qwc & 0xffff, 0x10, NULL, 0x1000101, ((u_int *)start)[3]);
    return (u_long *)p;
}

/* A 4x4 texture matrix for VU1 0x3ed.. and the word that selects it. */
static u_char *i3rPktStqMatrix(u_char *p, const void *m, u_int select)
{
    ((u_long *)p)[1] = 0x3ee;
    ((u_long *)p)[0] = (u_long)(5 + 0x3ed) << 32 | 400;
    memcpy(p + 0x10, m, 0x40);
    ((u_int *)p)[0x14] = select;
    ((u_int *)p)[0x15] = 0x3ee;
    return p + 0x60;
}

// ──────────────────────────────────────────────────────────────────────
// A layer's STQ map entities (child 3) to VU1, four per UNPACK: kind 1
// projects from the view (its camera rotation times the bias matrix),
// kinds 3 / 4 bring their own matrix, kind 2 two rows; the entity's m_40
// (1 or 2) picks the VU1 slot. *written: whether one was set up.
// ROR 0x00229b70

static u_long *i3rPktLayerStqMaps(u_long *pkt, const I3rItem *lay, I3dView *view, u_char *written)
{
    const I3rItem &maps = lay->getChildren()[3];
    u_int count = maps.getNumChild();
    u_char any = 0;

    if (count != 0)
    {
        u_char *base = (u_char *)pkt;
        u_int *unpack = (u_int *)(base + 0xc);
        u_char *p;
        u_int i = 0;

        *(u_int *)(base + 8) = 0x1000101;
        for (;;)
        {
            p = (u_char *)(unpack + 1);
            u_int end = i + 4 < count ? i + 4 : count;
            do
            {
                const I3rStqMapEntity *e = (const I3rStqMapEntity *)maps.getChildren()[i].getData();
                u_char kind = e->m_id;
                u_char mode = e->m_40;
                if (kind != 0 && mode != 0)
                {
                    any = 1;
                    if (kind == 2)
                    {
                        *(u_int *)p = 0x34d;
                        memcpy(p + 0x10, e, 0x20);
                        p += 0x30;
                    }
                    else if (kind == 1)
                    {
                        sceVu0FMATRIX cam, m;
                        memcpy(cam, view->m_camera, 0x30);
                        cam[3][0] = cam[3][1] = cam[3][2] = 0.0f;
                        cam[3][3] = 1.0f;
                        i3dVu0MatMul(m, s_stq_bias, cam);
                        if (mode == 1)
                            p = i3rPktStqMatrix(p, m, 800);
                        else if (mode == 2)
                            p = i3rPktStqMatrix(p, m, 0x398);
                    }
                    else if (kind == 3)
                    {
                        if (mode == 1)
                            p = i3rPktStqMatrix(p, e, 0x321);
                        else if (mode == 2)
                            p = i3rPktStqMatrix(p, e, 0x39b);
                    }
                    else if (kind == 4)
                    {
                        if (mode == 1)
                            p = i3rPktStqMatrix(p, e, 800);
                        else if (mode == 2)
                            p = i3rPktStqMatrix(p, e, 0x398);
                    }
                }
                i++;
            } while (i < end);
            *(u_int *)p = 0;
            *unpack = ((u_int)(p + 0x10 - 4 - (u_char *)unpack) >> 4) << 16 | 0x6c008000;
            ((u_int *)(p + 0x10))[0] = 0x14000001;
            ((u_int *)(p + 0x10))[1] = 0;
            ((u_int *)(p + 0x10))[2] = 0;
            unpack = (u_int *)(p + 0x1c);
            if (i >= count)
                break;
        }
        *unpack = 0;
        *(u_long *)base = (u_long)((u_int)(p + 0x20 - 0x10 - base) >> 4) | I3D_DMA_CNT;
        pkt = (u_long *)(p + 0x20);
    }
    *written = any;
    return pkt;
}

// ──────────────────────────────────────────────────────────────────────
// A layer's cull sign, its uv offset and scale (child 0's tracks, or the
// option record's; both when the record sets them) and its uv matrix
// (child 1's pivot, scale and rotation tracks) to VU1; a RET tag; then
// the strip program's flags word (flat, textured, fogged) and the STQ
// map's VU1 slot.
// ROR 0x0022a050

static u_long *i3rPktLayerUvXform(float t, u_long *pkt, const I3rItem *lay, const I3dFogState *fog,
                                  const I3rMeshOpt *opt, int textured, u_char stq)
{
    u_char *base = (u_char *)pkt;
    u_char *p = base + 0x10;
    const I3rLayerData *d = (const I3rLayerData *)lay->getData();
    u_char cull = d->m_cull;
    alignas(16) sceVu0FVECTOR a;
    alignas(16) sceVu0FVECTOR b;
    alignas(16) sceVu0FVECTOR c;

    if (cull == 1 || cull == 2)
    {
        float sign = cull == 1 ? -1.0f : 1.0f;
        ((u_int *)p)[0] = 0x168;
        memcpy(p + 4, &sign, 4);
        p = base + 0x20;
    }
    else if (cull == 3)
    {
        *(u_int *)p = 0x690;
        p = base + 0x20;
    }

    const I3rItem *ch = lay->getChildren();
    if (ch[0].m_data == 0)
    {
        if (opt != NULL && opt->m_uv_set != 0)
        {
            *(u_int *)p = 0xb6;
            memcpy(p + 0x10, opt->m_uv_scale, 0x10);
            memcpy(p + 0x20, opt->m_uv_offset, 0x10);
            p += 0x30;
        }
    }
    else
    {
        const I3rItem *track = ch[0].getChildren();
        i3rTrackGetValue(&track[1], a, t);
        i3rTrackGetValue(&track[0], b, t);
        if (opt != NULL && opt->m_uv_set != 0)
        {
            i3dVu0VecMul(a, a, opt->m_uv_scale);
            i3dVu0VecMul(b, b, opt->m_uv_scale);
            i3dVu0VecAdd(b, b, opt->m_uv_offset);
        }
        *(u_int *)p = 0xb6;
        memcpy(p + 0x10, a, 0x10);
        memcpy(p + 0x20, b, 0x10);
        p += 0x30;
    }

    if (ch[1].m_data != 0)
    {
        const I3rItem *track = ch[1].getChildren();
        i3rTrackGetValue(&track[2], a, t);      // rotation (z)
        i3rTrackGetValue(&track[1], b, t);      // scale
        i3rTrackGetValue(&track[0], c, t);      // pivot
        float s = sinf(a[2]);
        float co = cosf(a[2]);
        alignas(16) sceVu0FMATRIX rot = {
            {co, s, 0.0f, 0.0f},
            {-s, co, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.5f, 0.5f, 0.0f, 1.0f},
        };
        alignas(16) sceVu0FMATRIX scale;
        alignas(16) sceVu0FMATRIX move;
        alignas(16) sceVu0FMATRIX m;
        memset(scale, 0, sizeof(scale));
        scale[0][0] = b[0];
        scale[1][1] = b[1];
        scale[2][2] = b[2];
        scale[3][3] = 1.0f;
        memset(move, 0, sizeof(move));
        move[0][0] = move[1][1] = move[2][2] = move[3][3] = 1.0f;
        move[3][0] = (c[0] - 0.5f) / b[0];
        move[3][1] = (c[1] - 0.5f) / b[1];
        i3dVu0MatMul(m, rot, scale);
        i3dVu0MatMul(m, m, move);
        *(u_int *)p = 0x389;
        memcpy(p + 0x10, m, 0x40);
        p += 0x50;
    }

    u_int qwc = (u_int)(p - base) >> 4;
    u_int unpack = qwc - 1 == 0 ? 0 : (qwc - 1) * 0x10000 | 0x6c0001ca;
    i3dPktDmaTag(pkt, qwc & 0xffff, 0x60, NULL, 0x1000101, unpack);
    u_int flags = cull != 3 ? 0x40264000 : 0x40224000;
    if (textured)
        flags |= 0x80000;
    if (fog != NULL)
        flags |= 0x100000;
    ((u_int *)p)[0] = (qwc + 0x1c9) | 0x60030000;
    ((u_int *)p)[1] = stq ? 0x3de : 0x3dc;
    ((u_int *)p)[2] = flags;
    ((u_int *)p)[3] = 0;
    return (u_long *)(p + 0x10);
}

// ──────────────────────────────────────────────────────────────────────
// TEST_1 from a layer: an alpha test that always fails with the layer's
// fail mode, or the layer's own test (AREF = m_alpha_ref * 128 clamped to
// 0..255), the destination alpha test, the depth test; ZTE always on.
// Nothing is written when the layer sets none of them.
// ROR 0x0022a448

u_long *i3rPktLayerTest(u_long *pkt, const I3rLayerData *d)
{
    u_long afail = 0;
    u_long ate = 0;
    u_long atst = 1;
    u_long aref = 0;
    u_long ztst = 2;
    u_long datm = 0;
    u_long date = 0;
    bool set = false;

    if (d->m_fail_only == 0)
    {
        if (d->m_atst != 0)
        {
            ate = 1;
            atst = s_atst[d->m_atst & 0xf];
            int ref = (int)(d->m_alpha_ref * 128.0f);
            if (ref < 0)
                ref = 0;
            if (ref > 0xff)
                ref = 0xff;
            aref = (u_long)ref;
            afail = i3rAlphaFailMode(d->m_afail);
            set = true;
        }
    }
    else
    {
        ate = 1;
        afail = i3rAlphaFailMode(d->m_fail_only);
        atst = 0;
        set = true;
    }
    switch (d->m_date)
    {
    case 2:
        date = 0;
        set = true;
        break;
    case 9:
        date = 1;
        datm = 1;
        set = true;
        break;
    case 10:
        date = 1;
        datm = 0;
        set = true;
        break;
    }
    switch (d->m_ztst)
    {
    case 1:    ztst = 0; set = true; break;
    case 2:    ztst = 1; set = true; break;
    case 0x13: ztst = 3; set = true; break;
    case 0x14: ztst = 2; set = true; break;
    }
    if (set)
    {
        pkt[0] = ate | ztst << 17 | afail << 12 | datm << 15 | atst << 1 | aref << 4 | date << 14 | 0x10000;
        pkt[1] = I3D_GS_TEST_1;
        pkt += 2;
    }
    return pkt;
}

// ──────────────────────────────────────────────────────────────────────
// ALPHA_1 from a layer's blend mode: 0x10 / 0x11 / 0x12 blend, add and
// subtract with the source alpha, 0x20 / 0x21 / 0x22 the same with the
// destination alpha, 1 keeps the destination, 2 the source. Mode 0
// writes nothing.
// ROR 0x0022a638

u_long *i3rPktLayerAlpha(u_long *pkt, const I3rLayerData *d)
{
    u_long alpha = 0;

    if (d->m_blend != 0)
    {
        switch (d->m_blend)
        {
        case 1:    alpha = 0x40; break;
        case 2:    alpha = 0; break;
        case 0x10: alpha = 0x44; break;
        case 0x11: alpha = 0x48; break;
        case 0x12: alpha = 0x42; break;
        case 0x20: alpha = 0x54; break;
        case 0x21: alpha = 0x58; break;
        case 0x22: alpha = 0x52; break;
        }
        pkt[0] = alpha;
        pkt[1] = I3D_GS_ALPHA_1;
        pkt += 2;
    }
    return pkt;
}
