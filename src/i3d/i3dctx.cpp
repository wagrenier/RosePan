/* ==========================================================================
 *  i3d/i3dctx.cpp
 *
 *  The i3d draw context (ROR 0x21eb28..0x21fb30): hardware set-up (the VU
 *  microprograms), the context over the packet buffer, the frame's packet
 *  positions, layers and groups taken from the packet tail, and the texture
 *  batching that keeps the two VRAM heaps (i3d/i3dvram.cpp) in step with
 *  what each run of draw sets needs. See i3dctx.h.
 *
 *  PORT: the ROM keeps the packet positions as EE addresses in the
 *  uncached-accelerated segment (| 0x30000000) and strips those bits with
 *  & 0x0fffffff before arithmetic; the host keeps plain host pointers. DMA
 *  tags still get an address word (i3dDmaAddr), but the host's sceDmaSend
 *  only records the chain's start and nothing walks it yet (open-items.md:
 *  the i3d packets and the host renderer). The objects the context takes
 *  from the packet tail are host-sized (PC:). The DMAC register writes of
 *  i3dInitHardware have no host counterpart and are kept as comments.
 * ======================================================================== */
#include "i3d/i3dctx.h"
#include "i3d/i3dlayer.h"

#include <stdint.h>
#include <string.h>

#include "lib/unassigned.h"
#include "rosepan/rendering/rosepan_i3d.h"
#include "sdk/libdma.h"
#include "sdk/libgraph.h"

/* ---- DMA tags, VIF codes, GIF tags ---------------------------------------- */

#define I3D_VIF_FLUSH   0x11000000u
#define I3D_VIF_STROW   0x30000000u
#define I3D_VIF_STCOL   0x31000000u
#define I3D_VIF_DIRECT  0x50000000u

/* GIF tag, low word: NREG 1, EOP; NLOOP is or-ed in. High word: A+D. */
#define I3D_GIF_AD1_EOP 0x1000000000008000ull
#define I3D_GIF_AD      0x0eull

#define I3D_GS_TEX0_1    0x06ull
#define I3D_GS_TEXFLUSH  0x3full

/* Qwords from just after a tag at `tag` to `end`. */
static inline u_long i3dQwc(const void *tag, const void *end)
{
    return (u_long)(u_int)(((const u_char *)end - (const u_char *)tag - 0x10) >> 4);
}

/* ---- data ----------------------------------------------------------------- */

/* data 0x302ba0: the sizes of the library's 36 structures on the EE, which
 * the game compares with its own copy before anything else
 * (MagSystem::initGraph). Entries 26 and 27 are i3dContext. */
static const int s_i3dStructSize[0x24] = {
    0x20,  0x130, 0x130, 0x1e,  0x38,    0x38,    0x6,  0x18, 0x8,
    0x20,  0x20,  0x20,  0x20,  0x40,    0x40,    0x40, 0x40, 0x20,
    0x4d0, 0x3b0, 0x80,  0xa0,  0x8,     0x44,    0x18, 0x20, 0x12168,
    0x12168, 0x10, 0x18, 0x60,  0x400,   0x400,   0x18, 0x20, 0x38,
};

/* data 0x2f2b70: the TEX0_1 value i3dCtxMakeVramTransfer sends after the
 * transfers (CLD 2: the GS reloads its CLUT buffer on the next CBP change;
 * unverified that this is its purpose). */
static const u_long s_i3dReloadTex0 = 0x4000000445304000ull;

/* ---- hardware -------------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Loads the VU1 microprogram (with STCOL 1.0 / STROW 0) through VIF1 and
// the VU0 one through VIF0, each as a REF of the ROM's code and waited for,
// then sets the scratchpad channels up.
// ROR 0x0021eb28

int i3dInitHardware(void)
{
    alignas(16) u_long vu1[0x100 / sizeof(u_long)];
    alignas(16) u_long vu0[0x100 / sizeof(u_long)];
    u_long *p;
    sceDmaChan *ch;

    i3dSysInit();
    // PC: REG_DMAC_CTRL |= 2 (RELE, release cycle stealing); no host DMAC.
    p = i3dMakeVu1MicroPacket(vu1);
    p[0] = I3D_DMA_END;
    p[1] = 0;
    i3dFlushCache();
    sceGsSyncPath(0, 0);
    ch = sceDmaGetChan(1);
    ch->chcr.TTE = 1;
    sceDmaSync(ch, 0, 0);
    sceDmaSend(ch, vu1);
    sceDmaSync(ch, 0, 0);
    sceGsSyncPath(0, 0);

    p = i3dMakeVu0MicroPacket(vu0);
    p[0] = I3D_DMA_END;
    p[1] = 0;
    i3dFlushCache();
    sceGsSyncPath(0, 0);
    ch = sceDmaGetChan(0);
    ch->chcr.TTE = 1;
    sceDmaSync(ch, 0, 0);
    sceDmaSend(ch, vu0);
    sceDmaSync(ch, 0, 0);
    sceGsSyncPath(0, 0);

    // PC: REG_DMAC_PCR |= 0x300 and REG_DMAC_STAT = 0x300 (the scratchpad
    // channels 8 / 9), sync.l / sync.p, then one quadword from scratchpad
    // 0x70000000 to a stack quadword and back (fromSPR / toSPR, CHCR 0x100).
    // No host counterpart.
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The default end-of-packet hook: a DMA end tag.
// ROR 0x0021ecf8

u_long *i3dCtxDefaultEndFunc(u_long *pkt, void *arg)
{
    (void)arg;
    pkt[1] = 0;
    pkt[0] = I3D_DMA_END;
    return pkt + 2;
}

/* ---- the context ------------------------------------------------------------ */

// ──────────────────────────────────────────────────────────────────────
// Splits vram_size pages from vram_base between the heaps: 1/16 + 1 pages
// for CLUTs (in 4-block units, 8 per page) after the texture pages.
// ROR 0x0021ed10

int i3dCtxSetup(i3dContext *ctx, int vram_base, u_int vram_size)
{
    u_int clut = (vram_size >> 4) + 1;
    int tex = (int)(vram_size - clut);

    ctx->m_tex_vram_size = tex;
    ctx->m_tex_vram_base = vram_base;
    ctx->m_clut_vram_size = (int)clut * 8;
    ctx->m_clut_vram_base = (vram_base + tex) * 8;
    i3dVramSetBase(&ctx->m_tex_vram, vram_base);
    i3dVramSetSize(&ctx->m_tex_vram, ctx->m_tex_vram_size);
    i3dVramSetBase(&ctx->m_clut_vram, ctx->m_clut_vram_base);
    i3dVramSetSize(&ctx->m_clut_vram, ctx->m_clut_vram_size);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Writes the VRAM transfers both heaps' last compaction asked for at pkt:
// a VIF FLUSH, the moves (GS local copies, in list order so that they do
// not overwrite each other), the uploads of newly placed images, a
// TEXFLUSH, and a TEX0 that makes the GS reload its CLUT. Returns the
// position for the next tag.
// ROR 0x0021ed90

u_long *i3dCtxMakeVramTransfer(i3dContext *ctx, u_long *pkt)
{
    I3dVramBlock *end;
    I3dVramBlock *b;
    u_long *p;
    u_long *q;

    ((u_int *)pkt)[2] = I3D_VIF_FLUSH;
    ((u_int *)pkt)[3] = 0;
    p = pkt + 2;
    pkt[0] = i3dQwc(pkt, p) | I3D_DMA_CNT;

    end = &ctx->m_tex_vram.m_sentinel;
    for (b = end->m_link[I3D_VRAM_LIST_MOVE].m_next; b != end; b = b->m_link[I3D_VRAM_LIST_MOVE].m_next)
        p = i3dTexMakeMove(b->m_owner->m_image, p, b->m_old_addr << 5, b->m_addr << 5);
    for (b = end->m_link[I3D_VRAM_LIST_NEW].m_next; b != end; b = b->m_link[I3D_VRAM_LIST_NEW].m_next)
        p = i3dTexMakeUpload(b->m_owner->m_image, p, b->m_addr << 5);

    end = &ctx->m_clut_vram.m_sentinel;
    for (b = end->m_link[I3D_VRAM_LIST_MOVE].m_next; b != end; b = b->m_link[I3D_VRAM_LIST_MOVE].m_next)
        p = i3dClutMakeMove(b->m_owner->m_image, p, b->m_old_addr << 2, b->m_addr << 2);
    for (b = end->m_link[I3D_VRAM_LIST_NEW].m_next; b != end; b = b->m_link[I3D_VRAM_LIST_NEW].m_next)
        p = i3dClutMakeUpload(b->m_owner->m_image, p, b->m_addr << 2);

    /* CNT 2 | NOP, DIRECT 2 | GIF A+D x1 | TEXFLUSH */
    q = p + 6;
    ((u_int *)p)[2] = 0;
    p[5] = I3D_GS_TEXFLUSH;
    p[4] = 0;
    p[3] = I3D_GIF_AD;
    p[2] = i3dQwc(p + 2, q) | I3D_GIF_AD1_EOP;
    ((u_int *)p)[3] = (u_int)(((u_char *)q - ((u_char *)p + 0xc) - 4) >> 4) | I3D_VIF_DIRECT;
    p[0] = i3dQwc(p, q) | I3D_DMA_CNT;

    /* CNT 2 | NOP, DIRECT 2 | GIF A+D x1 | TEX0_1 */
    p = q;
    q = p + 6;
    ((u_int *)p)[2] = 0;
    p[4] = s_i3dReloadTex0;
    p[5] = I3D_GS_TEX0_1;
    p[3] = I3D_GIF_AD;
    p[2] = i3dQwc(p + 2, q) | I3D_GIF_AD1_EOP;
    ((u_int *)p)[3] = (u_int)(((u_char *)q - ((u_char *)p + 0xc) - 4) >> 4) | I3D_VIF_DIRECT;
    p[0] = i3dQwc(p, q) | I3D_DMA_CNT;
    return q;
}

// ──────────────────────────────────────────────────────────────────────
// Sends the frame's packet on VIF1 (unreferenced in RoR: MagSystem kicks
// it itself).
// ROR 0x0021f028

int i3dCtxKickPacket(i3dContext *ctx)
{
    void *pkt = i3dCtxGetPacket(ctx);
    sceDmaChan *ch = sceDmaGetChan(1);

    ch->chcr.TTE = 1;
    sceDmaSync(ch, 0, 0);
    if (pkt != NULL)
        sceDmaSend(ch, pkt);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// A REF of the VU0 microprogram (data 0x2949f0, 0x9c qwords).
// PC: the address is the ROM's; the host has no VU and does not read it.
// ROR 0x0021f098

u_long *i3dMakeVu0MicroPacket(u_long *pkt)
{
    pkt[1] = 0;
    pkt[0] = ((u_long)0x002949f0 << 32) | I3D_DMA_REF | 0x9c;
    return pkt + 2;
}

// ──────────────────────────────────────────────────────────────────────
// A REF of the VU1 microprogram (data 0x2912b0, 0x354 qwords), then STCOL
// (1, 1, 1, 1) and STROW (0, 0, 0, 0).
// PC: the address is the ROM's; the host has no VU and does not read it.
// ROR 0x0021f0d8

u_long *i3dMakeVu1MicroPacket(u_long *pkt)
{
    u_int *w = (u_int *)pkt;
    float *f = (float *)pkt;

    pkt[1] = 0;
    pkt[0] = ((u_long)0x002912b0 << 32) | I3D_DMA_REF | 0x354;
    w[6] = 0;
    w[7] = I3D_VIF_STCOL;
    f[8] = 1.0f;
    f[9] = 1.0f;
    f[10] = 1.0f;
    f[11] = 1.0f;
    w[12] = 0;
    w[13] = 0;
    w[14] = 0;
    w[15] = I3D_VIF_STROW;
    w[16] = 0;
    w[17] = 0;
    w[18] = 0;
    w[19] = 0;
    pkt[2] = i3dQwc(pkt + 2, pkt + 10) | I3D_DMA_CNT;
    return pkt + 10;
}

// ──────────────────────────────────────────────────────────────────────
// The library's own size of structure i (-1 past the table).
// ROR 0x0021f1a8

int i3dGetStructSize(unsigned int i)
{
    if (i < 0x24)
        return s_i3dStructSize[i];
    return -1;
}

// ROR 0x0021f1d0

int i3dCtxSetEndFunc(i3dContext *ctx, I3dEndFunc func, void *arg)
{
    ctx->m_end_func = func;
    ctx->m_end_arg = arg;
    return 0;
}

// ROR 0x0021f1e8

int i3dCtxSetTexTable(i3dContext *ctx, I3dTexEntry *table, u_int count)
{
    ctx->m_tex_table = table;
    ctx->m_tex_table_count = count;
    return 0;
}

// ROR 0x0021f1f8

void *i3dCtxGetPacket(i3dContext *ctx)
{
    return ctx->m_packet;
}

// ──────────────────────────────────────────────────────────────────────
// Bytes still free between the packet head and the tail (0 once they
// crossed).
// ROR 0x0021f200

int i3dCtxGetRest(i3dContext *ctx)
{
    int rest = (int)(ctx->m_tail - ctx->m_head);
    if (rest < 0)
        rest = 0;
    return rest;
}

// ROR 0x0021f228

int i3dCtxGetRestFrom(i3dContext *ctx, u_char *head)
{
    int rest = (int)(ctx->m_tail - head);
    if (rest < 0)
        rest = 0;
    return rest;
}

// ──────────────────────────────────────────────────────────────────────
// Bytes by which the head ran past the tail (0 when it did not).
// ROR 0x0021f250

int i3dCtxGetOverrun(i3dContext *ctx)
{
    int rest = (int)(ctx->m_tail - ctx->m_head);
    if (rest >= 0)
        return 0;
    return -rest;
}

// ──────────────────────────────────────────────────────────────────────
// Takes qwc qwords from the top of the frame's share.
// ROR 0x0021f280

void *i3dCtxAllocTail(i3dContext *ctx, int qwc)
{
    ctx->m_tail = (u_char *)((uintptr_t)(ctx->m_tail - qwc * 16) & ~(uintptr_t)0xf);
    return ctx->m_tail;
}

// ROR 0x0021f2a0

void i3dCtxAllocHead(i3dContext *ctx, int qwc)
{
    ctx->m_head = (u_char *)((uintptr_t)(ctx->m_head + qwc * 16 + 0xf) & ~(uintptr_t)0xf);
}

// ROR 0x0021f2c0

u_char *i3dCtxGetAlignedHead(i3dContext *ctx)
{
    // PC: the ROM also drops the cache-mode bits (& 0x0fffff80).
    return (u_char *)((uintptr_t)(i3dCtxGetHead(ctx) + 0x7f) & ~(uintptr_t)0x7f);
}

// ROR 0x0021f2f0

void i3dCtxSetAlignedHead(i3dContext *ctx, u_char *head)
{
    // PC: the ROM or-s in the uncached-accelerated segment (0x30000000).
    i3dCtxSetHead(ctx, (u_char *)((uintptr_t)(head + 0x7f) & ~(uintptr_t)0x7f));
}

// ROR 0x0021f320

u_char *i3dCtxGetHead(i3dContext *ctx)
{
    return ctx->m_head;
}

// ROR 0x0021f328

int i3dCtxSetHead(i3dContext *ctx, u_char *head)
{
    ctx->m_head = head;
    return 0;
}

// ROR 0x0021f338

int i3dCtxResetVram(i3dContext *ctx)
{
    i3dVramReset(&ctx->m_tex_vram);
    i3dVramReset(&ctx->m_clut_vram);
    return 0;
}

/* ---- texture batching --------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Emits a run: links the open tag to the VRAM transfers, points every
// reserved draw set's texture slots at the addresses its textures now
// have (a slot whose index is past the table gets no texture), and runs
// the run's draw group, whose packets follow.
// ROR 0x0021f370

int i3dCtxFlushTextures(i3dContext *ctx, I3dDrawSet *set)
{
    u_char *head = i3dCtxGetHead(ctx);
    u_long *tag = ctx->m_chain;
    u_long *cur;

    tag[1] = 0;
    tag[0] = ((u_long)i3dDmaAddr(head) << 32) | I3D_DMA_NEXT;
    cur = i3dCtxMakeVramTransfer(ctx, (u_long *)head);
    ctx->m_chain = cur;
    i3dCtxSetHead(ctx, (u_char *)(cur + 2));

    for (I3dDrawSet *s = ctx->m_reserved; s != NULL; s = s->m_reserved_next)
    {
        u_int n = s->m_tex_count;
        for (u_int i = 0; i < n; i++)
        {
            if (s->m_tex_used[i] == 0)
                continue;
            u_long *regs = s->m_tex_regs[i];
            if (s->m_tex_index[i] < ctx->m_tex_table_count)
            {
                I3dTexEntry *e = &ctx->m_tex_table[s->m_tex_index[i]];
                i3dTexRegsSet(regs, e->m_tex, e->m_clut, e->m_0c);
            }
            else
            {
                i3dTexRegsSetNone(regs);
            }
        }
    }
    ctx->m_reserved = NULL;
    i3dCtxRunGroup(ctx, set->m_group);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// A layer's draw callback. The layer's draw sets, sorted, are taken in
// runs of equal depth and second key; a run's textures are reserved set by
// set, and when the heaps are full the sets so far are emitted and the
// reservation starts again from the set that did not fit. A set whose
// textures do not fit even alone is drawn without textures and flags the
// frame (i3dCtxEndFrame returns I3D_ERR_VRAM). A node without a draw set
// resets the heaps.
// ROR 0x0021f4d8

int i3dCtxDrawTexturedLayer(I3dLayer *layer)
{
    i3dContext *ctx = layer->m_ctx;
    I3dVramHeap *tex = &ctx->m_tex_vram;
    I3dVramHeap *clut = &ctx->m_clut_vram;
    I3dNode *node;

    layer->m_last->m_next = NULL;
    if (layer->m_count == 0)
        return 0;

    node = i3dSortList(layer->m_first, 1, 0, layer->m_count);
    while (node != NULL)
    {
        I3dDrawSet *first = (I3dDrawSet *)node->m_data;
        if (first == NULL)
        {
            i3dVramReset(tex);
            i3dVramReset(clut);
            node = node->m_next;
            continue;
        }

        float depth = first->m_depth;
        u_int key = first->m_sort_func;
        I3dDrawSet *set = first;

        i3dVramEndFrame(tex);
        i3dVramEndFrame(clut);
        for (;;)
        {
            if (set->m_group->m_bytes != 0 && i3dCtxReserveTextures(ctx, set) == I3D_ERR_FULL)
            {
                if (set != first)
                {
                    // Emit the sets before this one and retry it in a new run
                    // (the ROM re-tests depth and key first; this set passed them).
                    i3dVramCompact(tex);
                    i3dVramCompact(clut);
                    i3dCtxFlushTextures(ctx, first);
                    first = set;
                    i3dVramEndFrame(tex);
                    i3dVramEndFrame(clut);
                    continue;
                }
                ctx->m_vram_overflow = 1;
                i3dBatchClearTextures(first);
            }
            if (set != first)
                i3dGroupAppend(first->m_group, set->m_group);
            node = node->m_next;
            if (node == NULL)
                break;
            set = (I3dDrawSet *)node->m_data;
            if (set == NULL || set->m_depth != depth || set->m_sort_func != key)
                break;
        }
        i3dVramCompact(tex);
        i3dVramCompact(clut);
        i3dCtxFlushTextures(ctx, first);
    }
    return 0;
}

/* ---- frames ---------------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Takes the next frame's share of the buffer (frames round-robin), the
// head 128-aligned, and makes the frame's root group.
// ROR 0x0021f6a0

int i3dCtxBeginFrame(i3dContext *ctx)
{
    u_int count = ctx->m_frame_count;
    u_int index = ctx->m_frame_index;
    u_char *head;

    ctx->m_frames_begun++;
    head = ctx->m_buf + (int)(ctx->m_buf_size * index) / (int)count;
    ctx->m_frame_index = index + 1;
    ctx->m_tail = ctx->m_buf + (int)(ctx->m_buf_size * (index + 1)) / (int)count;
    // PC: the ROM or-s in the uncached-accelerated segment (0x30000000).
    ctx->m_head = (u_char *)((uintptr_t)(head + 0x7f) & ~(uintptr_t)0x7f);
    if (index + 1 >= count)
        ctx->m_frame_index = 0;
    ctx->m_packet = NULL;
    ctx->m_root = i3dCtxCreateGroup(ctx, 1, 1, 0);
    RosePan_I3dBeginFrame();                       // PC: the native i3d module
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Builds the frame's DMA chain (i3dCtxCloseFrame runs the root group and
// the end hook); I3D_ERR_VRAM when some draw set's textures never fit.
// ROR 0x0021f760

int i3dCtxEndFrame(i3dContext *ctx)
{
    ctx->m_frame_stat[0] = 0;
    ctx->m_frame_stat[1] = 0;
    ctx->m_frame_stat[2] = 0;
    ctx->m_frame_stat[3] = 0;
    i3dFlushCache();
    ctx->m_vram_overflow = 0;
    i3dCtxCloseFrame(ctx);
    i3dFlushCache();
    if (ctx->m_vram_overflow == 0)
        return 0;
    return I3D_ERR_VRAM;
}

// ROR 0x0021f7c8

int i3dCtxSetFloatParam(i3dContext *ctx, float value)
{
    ctx->m_float_param = value;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// A new layer in this frame's root group. key orders the layers: its float
// bits made to sort as unsigned (negative values flipped, positive ones
// with the sign bit set).
// ROR 0x0021f7e0

I3dLayer *i3dCtxCreateLayer(i3dContext *ctx, float key)
{
    u_int bits;
    I3dLayer *layer;

    memcpy(&bits, &key, sizeof(bits));
    // PC: the ROM takes 0xe6 qwords (EE 0xe60).
    layer = (I3dLayer *)i3dCtxAllocTail(ctx, I3D_QWC_OF(I3dLayer));
    i3dLayerInit(layer, ctx);
    i3dGroupAddGroup(ctx->m_root, bits ^ ((u_int)((int)bits >> 31) | 0x80000000u), layer->m_group);
    return layer;
}

// ROR 0x0021f850

I3dGroup *i3dCtxCreateGroup(i3dContext *ctx, int sort_func, int sort_order, int sort_param)
{
    // PC: the ROM takes 3 qwords (EE 0x30).
    I3dGroup *group = (I3dGroup *)i3dCtxAllocTail(ctx, I3D_QWC_OF(I3dGroup));
    i3dGroupInit(group, ctx, sort_func, sort_order, sort_param);
    return group;
}

// ROR 0x0021f8c8

int i3dCtxTerm(i3dContext *ctx)
{
    i3dVramTerm(&ctx->m_tex_vram);
    i3dVramTerm(&ctx->m_clut_vram);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// A context over buf (size bytes, count frames): both heaps over their
// 0x200 nodes, the default end hook.
// ROR 0x0021f900

int i3dCtxInit(i3dContext *ctx, void *buf, int size, int count)
{
    memset(ctx, 0, sizeof(*ctx));
    i3dVramInit(&ctx->m_tex_vram, ctx->m_tex_nodes, 0x200, ctx->m_tex_free);
    i3dVramInit(&ctx->m_clut_vram, ctx->m_clut_nodes, 0x200, ctx->m_clut_free);
    ctx->m_frame_count = (u_int)count;
    ctx->m_buf = (u_char *)buf;
    ctx->m_buf_size = size;
    ctx->m_float_param = 0.35f;
    ctx->m_reserved = NULL;
    ctx->m_end_func = i3dCtxDefaultEndFunc;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Reserves a draw set's textures and CLUTs in VRAM: those of its used
// slots with a valid table index, minus the static ones, at most 0x200 of
// each. I3D_ERR_FULL when they do not fit; otherwise a set that uses any
// texture joins the context's reserved list for i3dCtxFlushTextures.
// ROR 0x0021f9c8

int i3dCtxReserveTextures(i3dContext *ctx, I3dDrawSet *set)
{
    I3dVramHandle *tex[0x200];
    I3dVramHandle *clut[0x200];
    u_int ntex = 0;
    u_int nclut = 0;
    int used = 0;

    for (u_int i = 0; i < set->m_tex_count; i++)
    {
        if (set->m_tex_used[i] == 0)
            continue;
        u_int index = set->m_tex_index[i];
        if (index >= ctx->m_tex_table_count)
            continue;
        I3dTexEntry *e = &ctx->m_tex_table[index];
        I3dVramHandle *t = e->m_tex;
        I3dVramHandle *c = e->m_clut;
        if (t != NULL)
        {
            used = 1;
            if (t->m_static == 0)
            {
                if (ntex >= 0x200)
                    return I3D_ERR_FULL;
                tex[ntex++] = t;
            }
        }
        if (c != NULL)
        {
            used = 1;
            if (c->m_static == 0)
            {
                if (nclut >= 0x200)
                    return I3D_ERR_FULL;
                clut[nclut++] = c;
            }
        }
    }
    if (ntex != 0 && i3dVramReserve(&ctx->m_tex_vram, tex, ntex) == I3D_ERR_FULL)
        return I3D_ERR_FULL;
    if (nclut != 0 && i3dVramReserve(&ctx->m_clut_vram, clut, nclut) == I3D_ERR_FULL)
        return I3D_ERR_FULL;
    if (used)
    {
        set->m_reserved_next = ctx->m_reserved;
        ctx->m_reserved = set;
    }
    return 0;
}
