/* ==========================================================================
 *  i3d/i3dlayer.cpp
 *
 *  The i3d draw groups and layers (ROR 0x222f50..0x224de0): the layer
 *  helpers and setters, the node lists and their packet / call nodes, the
 *  sorts, the group run that links a frame's DMA chain, and the texture
 *  register packets. See i3dlayer.h.
 *
 *  PORT: as in i3dctx.cpp the ROM keeps packet positions as EE addresses
 *  (aligned packet starts without the segment bits, the head in the
 *  uncached-accelerated segment) and the host keeps host pointers; DMA tag
 *  address words come from i3dDmaAddr. The nodes, groups and layers taken
 *  from the packet tail are host-sized (I3D_QWC_OF). The sorts work in the
 *  scratchpad (0x70000000) in the ROM and in static buffers here.
 * ======================================================================== */
#include "i3d/i3dlayer.h"

#include <stdint.h>
#include <string.h>

#include "lib/unassigned.h"

/* GIF tag, low word: NREG 1, EOP; NLOOP is or-ed in. High word: A+D. */
#define I3D_GIF_AD1_EOP 0x1000000000008000ull
#define I3D_GIF_AD      0x0eull

#define I3D_GS_TEX0_1    0x06ull
#define I3D_GS_TEX1_1    0x14ull
#define I3D_GS_TEX2_1    0x16ull
#define I3D_GS_MIPTBP1_1 0x34ull
#define I3D_GS_MIPTBP2_1 0x36ull

/* A float's bits made to sort as unsigned (negative values flipped,
 * positive ones with the sign bit set), as every float key in the library. */
static inline u_int i3dFloatKey(float key)
{
    u_int bits;
    memcpy(&bits, &key, sizeof(bits));
    return bits ^ ((u_int)((int)bits >> 31) | 0x80000000u);
}

/* An empty list's last node is its head (i3dListInit). */
static inline I3dNode *i3dHead(void *list)
{
    return (I3dNode *)list;
}

/* The 128-byte aligned packet position at or after p. PC: the ROM also
 * strips (begin) or sets (end) the segment bits. */
static inline u_long *i3dAlign128(const void *p)
{
    return (u_long *)(((uintptr_t)p + 0x7f) & ~(uintptr_t)0x7f);
}

/* Appends a node to a group (every i3dGroup* adder ends this way). */
static inline void i3dGroupLink(I3dGroup *group, I3dNode *node)
{
    group->m_last->m_next = node;
    group->m_last = node;
    group->m_count++;
}

/* A call node from the group context's tail. PC: the ROM takes 0x20 bytes
 * (0x10 for i3dGroupAddEnd's). */
static inline I3dCallNode *i3dGroupNewCallNode(I3dGroup *group, u_int key, I3dNodeFunc func)
{
    I3dCallNode *node = (I3dCallNode *)i3dCtxAllocTail(group->m_ctx, I3D_QWC_OF(I3dCallNode));
    node->m_node.m_key = key;
    node->m_node.m_data = NULL;
    node->m_node.m_func = func;
    return node;
}

/* ---- layers --------------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// The layer state a packet draws with: a CALL of the view's packet, a
// CALL of the fog's, ALPHA mode 2.
// ROR 0x00222f50

u_long *i3dPktLayerState(u_long *pkt, I3dLayer *layer)
{
    pkt = i3dPktCallView(pkt, layer);
    pkt = i3dPktCallFog(pkt, layer);
    return i3dPktSetAlphaMode(pkt, 2);
}

// ──────────────────────────────────────────────────────────────────────
// A line from `from` to `to` in the layer's default draw set
// (MagScene::drawStrips' type 4 records).
// ROR 0x00222f88

int i3dLayerDrawLine(I3dLayer *layer, const float *from, const float *to, const void *color)
{
    sceVu0FVECTOR xyz[2];

    i3dLayerSetDrawSet(layer, i3dLayerGetDrawSet(layer));
    u_long *pkt = i3dLayerBeginSetUnalignedKey(layer, 0);
    pkt = i3dPktLayerState(pkt, layer);
    i3dVecCopyW(xyz[0], from);
    i3dVecCopyW(xyz[1], to);
    pkt = i3dPktDrawLineStrip(pkt, layer, i3dMatGetUnit(), 2, color, xyz, 0);
    i3dLayerEndSetUnaligned(layer, pkt);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// A wire sphere (three circles) round center in the default draw set
// (drawStrips' type 5 records).
// ROR 0x00223050

int i3dLayerDrawWireSphere(I3dLayer *layer, sceVu0FVECTOR *matrix, const float *center, float radius)
{
    i3dLayerSetDrawSet(layer, i3dLayerGetDrawSet(layer));
    u_long *pkt = i3dLayerBeginSetUnalignedKey(layer, 0);
    pkt = i3dPktLayerState(pkt, layer);
    pkt = i3dPktDrawWireSphere(radius, pkt, layer, matrix, center);
    i3dLayerEndSetUnaligned(layer, pkt);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// A wire box (centre and half extents) in the default draw set
// (drawStrips' type 3 records).
// ROR 0x002230e8

int i3dLayerDrawWireBox(I3dLayer *layer, sceVu0FVECTOR *matrix, const float *center,
                        const void *extent, const void *color)
{
    i3dLayerSetDrawSet(layer, i3dLayerGetDrawSet(layer));
    u_long *pkt = i3dLayerBeginSetUnalignedKey(layer, 0);
    pkt = i3dPktLayerState(pkt, layer);
    pkt = i3dPktDrawWireBox(pkt, layer, matrix, center, extent, color);
    i3dLayerEndSetUnaligned(layer, pkt);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The same around 0x21e9f8's wire shape. Nothing calls it.
// ROR 0x00223190

int i3dLayerDrawWireShape(I3dLayer *layer, void *a, void *b, void *c)
{
    i3dLayerSetDrawSet(layer, i3dLayerGetDrawSet(layer));
    u_long *pkt = i3dLayerBeginSetUnalignedKey(layer, 0);
    pkt = i3dPktLayerState(pkt, layer);
    pkt = i3dPktDrawWireShape(pkt, layer, a, b, c);
    i3dLayerEndSetUnaligned(layer, pkt);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Makes set current: the next Begin / EndSet write into its group.
// ROR 0x00223228

int i3dLayerSetDrawSet(I3dLayer *layer, I3dDrawSet *set)
{
    layer->m_cur_set = set;
    layer->m_set_group = set->m_group;
    return 0;
}

// ROR 0x00223240

int i3dLayerSetLight(I3dLayer *layer, I3dLight *light)
{
    layer->m_light = light;
    return 0;
}

// ROR 0x00223250

int i3dLayerSetFog(I3dLayer *layer, void *fog)
{
    layer->m_fog = fog;
    return 0;
}

// ROR 0x00223260

int i3dLayerSetView(I3dLayer *layer, I3dView *view)
{
    layer->m_view = view;
    return 0;
}

// The set of the empty description (i3dLayerInit).
// ROR 0x00223270

I3dDrawSet *i3dLayerGetDrawSet(I3dLayer *layer)
{
    return layer->m_default_set;
}

// ROR 0x00223278

I3dDrawSet *i3dLayerGetCurDrawSet(I3dLayer *layer)
{
    return layer->m_cur_set;
}

// ROR 0x00223280

I3dLight *i3dLayerGetLight(I3dLayer *layer)
{
    return layer->m_light;
}

// ROR 0x00223288

void *i3dLayerGetFog(I3dLayer *layer)
{
    return layer->m_fog;
}

// ROR 0x00223290

I3dView *i3dLayerGetView(I3dLayer *layer)
{
    return layer->m_view;
}

// ──────────────────────────────────────────────────────────────────────
// Opens a 128-aligned packet in the current draw set's group, sorted by
// key within the set; the caller writes from the returned position.
// ROR 0x00223298

u_long *i3dLayerBeginSet(I3dLayer *layer, float key)
{
    return i3dGroupBeginChanPacket(layer->m_set_group, i3dFloatKey(key), 0);
}

// ROR 0x002232d0

u_long *i3dLayerBeginSetKey(I3dLayer *layer, u_int key)
{
    return i3dGroupBeginChanPacket(layer->m_set_group, key, 0);
}

// ROR 0x002232f0

int i3dLayerEndSet(I3dLayer *layer, u_long *end)
{
    return i3dGroupEndChanPacket(layer->m_set_group, end);
}

// ROR 0x00223308

u_long *i3dLayerBeginSetUnaligned(I3dLayer *layer, float key)
{
    return i3dGroupBeginChanPacketUnaligned(layer->m_set_group, i3dFloatKey(key), 0);
}

// ROR 0x00223340

u_long *i3dLayerBeginSetUnalignedKey(I3dLayer *layer, u_int key)
{
    return i3dGroupBeginChanPacketUnaligned(layer->m_set_group, key, 0);
}

// ROR 0x00223360

int i3dLayerEndSetUnaligned(I3dLayer *layer, u_long *end)
{
    return i3dGroupEndChanPacketUnaligned(layer->m_set_group, end);
}

// ──────────────────────────────────────────────────────────────────────
// A packet in the layer's own group before its draw sets (key 0; the
// draw call node is key 1).
// ROR 0x00223378

u_long *i3dLayerBeginPacket(I3dLayer *layer)
{
    return i3dGroupBeginChanPacket(layer->m_group, 0, 0);
}

// ROR 0x00223398

int i3dLayerEndPacket(I3dLayer *layer, u_long *end)
{
    return i3dGroupEndChanPacket(layer->m_group, end);
}

// ROR 0x002233b0

u_long *i3dLayerBeginPacketUnaligned(I3dLayer *layer)
{
    return i3dGroupBeginChanPacketUnaligned(layer->m_group, 0, 0);
}

// ROR 0x002233d0

int i3dLayerEndPacketUnaligned(I3dLayer *layer, u_long *end)
{
    return i3dGroupEndChanPacketUnaligned(layer->m_group, end);
}

// ──────────────────────────────────────────────────────────────────────
// A packet in the layer's group after its draw sets (key 2).
// ROR 0x002233e8

u_long *i3dLayerBeginPostPacket(I3dLayer *layer)
{
    return i3dGroupBeginChanPacket(layer->m_group, 2, 0);
}

// ROR 0x00223408

int i3dLayerEndPostPacket(I3dLayer *layer, u_long *end)
{
    return i3dGroupEndChanPacket(layer->m_group, end);
}

// ROR 0x00223420

u_long *i3dLayerBeginPostPacketUnaligned(I3dLayer *layer)
{
    return i3dGroupBeginChanPacketUnaligned(layer->m_group, 2, 0);
}

// ROR 0x00223440

int i3dLayerEndPostPacketUnaligned(I3dLayer *layer, u_long *end)
{
    return i3dGroupEndChanPacketUnaligned(layer->m_group, end);
}

// ROR 0x00223458

int i3dLayerSetUser(I3dLayer *layer, int n, u_long value)
{
    layer->m_user[n][0] = value;
    return 0;
}

// ROR 0x00223470

int i3dLayerSetUser2(I3dLayer *layer, int n, u_long value)
{
    layer->m_user[n][1] = value;
    return 0;
}

// ROR 0x00223488

u_long i3dLayerGetUser(I3dLayer *layer, int n)
{
    return layer->m_user[n][0];
}

// ROR 0x00223498

u_long i3dLayerGetUser2(I3dLayer *layer, int n)
{
    return layer->m_user[n][1];
}

// ──────────────────────────────────────────────────────────────────────
// The draw set of a model material: its entries' texture numbers (those
// not -1, at most 16 in the ROM's stack array), its depth (plus base's
// when given) and sort words.
// ROR 0x002234a8

I3dDrawSet *i3dLayerFindMaterialDrawSet(I3dLayer *layer, const I3rMaterial *mat, const I3dDrawSet *base)
{
    int tex_no[16];
    I3dTexSetDesc desc;
    u_int n = 0;
    u_int count = mat->m_tex->m_count & 0xffffff;
    const I3rMatParam *param = mat->m_param;

    if (count != 0)
    {
        const I3rTexRefEntry *e = mat->m_tex->m_entry;
        const I3rTexRefEntry *end = e + count;
        do
        {
            if (e->m_tex->m_tex_no != -1)
                tex_no[n++] = e->m_tex->m_tex_no;
            e++;
        } while (e != end);
    }
    desc.m_depth = param->m_depth;
    desc.m_tex_no = tex_no;
    desc.m_tex_count = n;
    desc.m_sort_func = param->m_sort_func;
    desc.m_sort_order = param->m_sort_order;
    desc.m_sort_param = param->m_sort_param;
    if (base != NULL)
        desc.m_depth = param->m_depth + base->m_depth;
    return i3dLayerFindDrawSet(layer, &desc);
}

// ──────────────────────────────────────────────────────────────────────
// The layer's draw set for a description: hashed (every word but the
// texture pointer, plus the texture numbers, folded to a byte) into one
// of 256 circular lists; a match moves to the front of its bucket. A new
// set is made from the packet tail, put at its bucket's front and given a
// node in the layer's list keyed by its depth.
// ROR 0x00223558

I3dDrawSet *i3dLayerFindDrawSet(I3dLayer *layer, const I3dTexSetDesc *desc)
{
    u_int depth_bits;
    u_int count = desc->m_tex_count;
    u_int hash;

    memcpy(&depth_bits, &desc->m_depth, sizeof(depth_bits));
    hash = depth_bits + desc->m_sort_order + desc->m_sort_func + count + desc->m_sort_param;
    for (u_int i = 0; i < count; i++)
        hash += (u_int)desc->m_tex_no[i];
    hash += hash >> 16;
    hash += hash >> 8;

    I3dDrawSet **bucket = &layer->m_sets[hash & 0xff];
    I3dDrawSet *head = *bucket;
    if (head != NULL)
    {
        I3dDrawSet *set = head;
        do
        {
            if (set->m_hash == hash && set->m_depth == desc->m_depth && set->m_tex_count == count &&
                set->m_sort_func == desc->m_sort_func && set->m_sort_order == desc->m_sort_order &&
                set->m_sort_param == desc->m_sort_param)
            {
                u_int i = 0;
                while (i < count && set->m_tex_index[i] == (u_int)desc->m_tex_no[i])
                    i++;
                if (i >= count)
                {
                    if (set != head)
                    {
                        i3dDrawSetMoveAfter(set, head);
                        *bucket = set;
                    }
                    return set;
                }
            }
            set = set->m_next;
        } while (set != head);
    }

    // PC: the ROM takes 4 qwords (EE 0x40) for the set and 1 for its node.
    I3dDrawSet *set = (I3dDrawSet *)i3dCtxAllocTail(layer->m_ctx, I3D_QWC_OF(I3dDrawSet));
    i3dDrawSetInit(set, layer, desc, hash);
    if (head != NULL)
        i3dDrawSetInsertAfter(set, head);
    *bucket = set;

    I3dNode *node = (I3dNode *)i3dCtxAllocTail(layer->m_ctx, I3D_QWC_OF(I3dNode));
    node->m_data = set;
    layer->m_last->m_next = node;
    layer->m_last = node;
    node->m_key = i3dFloatKey(desc->m_depth);
    layer->m_count++;
    return set;
}

// ──────────────────────────────────────────────────────────────────────
// A layer drawn inside this one: the first call makes the sub group (a
// node with key 3 in the layer's group: after the post packet), then each
// call adds a new layer to it under a float key.
// ROR 0x00223768

I3dLayer *i3dLayerCreateSubLayer(I3dLayer *layer, float key)
{
    if (layer->m_sub_group == NULL)
    {
        layer->m_sub_group = i3dCtxCreateGroup(layer->m_ctx, 1, 1, 0);
        i3dGroupAddGroup(layer->m_group, 3, layer->m_sub_group);
    }
    // PC: the ROM takes 0xe6 qwords (EE 0xe60).
    I3dLayer *sub = (I3dLayer *)i3dCtxAllocTail(layer->m_ctx, I3D_QWC_OF(I3dLayer));
    i3dLayerInit(sub, layer->m_ctx);
    i3dGroupAddGroup(layer->m_sub_group, i3dFloatKey(key), sub->m_group);
    return sub;
}

// ──────────────────────────────────────────────────────────────────────
// A layer node without a draw set: i3dCtxDrawTexturedLayer resets both
// VRAM heaps when its sorted walk reaches it. Nothing calls it.
// ROR 0x00223808

int i3dLayerAddVramReset(I3dLayer *layer, float key)
{
    I3dNode *node = (I3dNode *)i3dCtxAllocTail(layer->m_ctx, I3D_QWC_OF(I3dNode));
    node->m_data = NULL;
    layer->m_last->m_next = node;
    node->m_key = i3dFloatKey(key);
    layer->m_last = node;
    layer->m_count++;
    return 0;
}

// PC: i3dGroupAddCall calls its function with two words; the ROM installs
// i3dCtxDrawTexturedLayer (one parameter) directly.
static int i3dLayerDrawCallback(void *layer, void *)
{
    return i3dCtxDrawTexturedLayer((I3dLayer *)layer);
}

// ──────────────────────────────────────────────────────────────────────
// A new layer: its own group (ascending) holding the draw call node at key
// 1, cleared tables and state, and the default draw set (the empty
// description) made current.
// ROR 0x00223878

int i3dLayerInit(I3dLayer *layer, i3dContext *ctx)
{
    I3dGroup *group = i3dCtxCreateGroup(ctx, 1, 1, 0);
    I3dTexSetDesc desc;

    i3dListInit(i3dHead(layer));
    layer->m_count = 0;
    for (u_int i = 0; i < 0x100; i++)
        layer->m_sets[i] = NULL;
    for (u_int i = 0; i < 0x100; i++)
        layer->m_414[i] = 0;
    layer->m_ctx = ctx;
    layer->m_group = group;
    layer->m_sub_group = NULL;
    layer->m_cur_set = NULL;
    layer->m_light = NULL;
    layer->m_fog = NULL;
    layer->m_view = NULL;
    for (u_int i = 0; i < 0x62; i++)
    {
        layer->m_user[i][0] = 0;
        layer->m_user[i][1] = 0;
    }
    i3dGroupAddCall(group, 1, i3dLayerDrawCallback, layer, NULL);

    i3dTexSetDescInit(&desc);
    desc.m_depth = 0.0f;
    desc.m_tex_no = NULL;
    desc.m_tex_count = 0;
    desc.m_sort_func = 0;
    desc.m_sort_order = 0;
    desc.m_sort_param = 0;
    I3dDrawSet *set = i3dLayerFindDrawSet(layer, &desc);
    layer->m_default_set = set;
    layer->m_cur_set = set;
    layer->m_set_group = set->m_group;
    return 0;
}

/* ---- node lists ------------------------------------------------------------ */

// ──────────────────────────────────────────────────────────────────────
// An empty list: no key, no nodes, the last node is the head itself.
// ROR 0x002239b8

void i3dListInit(I3dNode *head)
{
    head->m_last = head;
    head->m_key = 0;
    head->m_next = NULL;
    head->m_data = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// A packet node (linked directly by the run) whose packet starts at the
// head 128-aligned; returns that start.
// ROR 0x002239d0

u_long *i3dGroupBeginPacket(I3dGroup *group, u_int key)
{
    // PC: the ROM takes 1 qword.
    I3dNode *node = (I3dNode *)i3dCtxAllocTail(group->m_ctx, I3D_QWC_OF(I3dNode));
    u_long *start;

    group->m_open = node;
    node->m_key = key;
    group->m_last->m_next = node;
    start = i3dAlign128(i3dCtxGetHead(group->m_ctx));
    group->m_last = node;
    node->m_data = start;
    group->m_count++;
    return start;
}

// ──────────────────────────────────────────────────────────────────────
// Closes the open packet node at end (the next tag goes there): counts
// its bytes, and the head moves 128-aligned past that tag.
// ROR 0x00223a20

int i3dGroupEndPacket(I3dGroup *group, u_long *end)
{
    I3dNode *node = group->m_open;

    group->m_bytes += (u_int)((u_char *)end - (u_char *)node->m_data);
    node->m_end = end;
    i3dCtxSetHead(group->m_ctx, (u_char *)i3dAlign128(end + 2));
    return 0;
}

// ROR 0x00223a68

u_long *i3dGroupBeginPacketUnaligned(I3dGroup *group, u_int key)
{
    I3dNode *node = (I3dNode *)i3dCtxAllocTail(group->m_ctx, I3D_QWC_OF(I3dNode));
    u_long *start = (u_long *)i3dCtxGetHead(group->m_ctx);

    group->m_open = node;
    node->m_key = key;
    group->m_last->m_next = node;
    node->m_data = start;
    group->m_last = node;
    group->m_count++;
    return start;
}

// ROR 0x00223aa8

int i3dGroupEndPacketUnaligned(I3dGroup *group, u_long *end)
{
    I3dNode *node = group->m_open;

    group->m_bytes += (u_int)((u_char *)end - (u_char *)node->m_data);
    node->m_end = end;
    i3dCtxSetHead(group->m_ctx, (u_char *)(end + 2));
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// A node that CALLs a channel's state packet (when it changed) before its
// own 128-aligned packet.
// ROR 0x00223ad8

u_long *i3dGroupBeginStatePacket(I3dGroup *group, u_int key, int channel, u_int state)
{
    I3dCallNode *node = (I3dCallNode *)i3dCtxAllocTail(group->m_ctx, I3D_QWC_OF(I3dCallNode));
    u_long *start = i3dAlign128(i3dCtxGetHead(group->m_ctx));

    group->m_open = &node->m_node;
    node->m_state.m_channel = channel;
    node->m_state.m_state = state;
    node->m_node.m_key = key;
    node->m_node.m_data = NULL;
    node->m_state.m_start = start;
    node->m_node.m_func = i3dNodeStatePacket;
    i3dGroupLink(group, &node->m_node);
    return start;
}

// ROR 0x00223b40

int i3dGroupEndStatePacket(I3dGroup *group, u_long *end)
{
    I3dCallNode *node = (I3dCallNode *)group->m_open;

    group->m_bytes += (u_int)((u_char *)end - (u_char *)node->m_state.m_start);
    node->m_state.m_end = end;
    i3dCtxSetHead(group->m_ctx, (u_char *)i3dAlign128(end + 2));
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// i3dGroupBeginStatePacket at the head as it is (the model packet
// builders).
// ROR 0x00223b88

u_long *i3dGroupBeginStatePacketUnaligned(I3dGroup *group, u_int key, int channel, u_int state)
{
    I3dCallNode *node = (I3dCallNode *)i3dCtxAllocTail(group->m_ctx, I3D_QWC_OF(I3dCallNode));
    u_long *start = (u_long *)i3dCtxGetHead(group->m_ctx);

    group->m_open = &node->m_node;
    node->m_state.m_channel = channel;
    node->m_state.m_state = state;
    node->m_state.m_start = start;
    node->m_node.m_func = i3dNodeStatePacket;
    node->m_node.m_key = key;
    node->m_node.m_data = NULL;
    i3dGroupLink(group, &node->m_node);
    return start;
}

// ROR 0x00223be0

int i3dGroupEndStatePacketUnaligned(I3dGroup *group, u_long *end)
{
    I3dCallNode *node = (I3dCallNode *)group->m_open;

    group->m_bytes += (u_int)((u_char *)end - (u_char *)node->m_state.m_start);
    node->m_state.m_end = end;
    i3dCtxSetHead(group->m_ctx, (u_char *)(end + 2));
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// A node whose 128-aligned packet sets a channel's state itself, so the
// run forgets the channel's last state before linking it.
// ROR 0x00223c10

u_long *i3dGroupBeginChanPacket(I3dGroup *group, u_int key, int channel)
{
    I3dCallNode *node = (I3dCallNode *)i3dCtxAllocTail(group->m_ctx, I3D_QWC_OF(I3dCallNode));
    u_long *start = i3dAlign128(i3dCtxGetHead(group->m_ctx));

    group->m_open = &node->m_node;
    node->m_chan.m_channel = channel;
    node->m_node.m_key = key;
    node->m_node.m_data = NULL;
    node->m_node.m_func = i3dNodeChanPacket;
    node->m_chan.m_start = start;
    i3dGroupLink(group, &node->m_node);
    return start;
}

// ROR 0x00223c78

int i3dGroupEndChanPacket(I3dGroup *group, u_long *end)
{
    I3dCallNode *node = (I3dCallNode *)group->m_open;

    node->m_chan.m_end = end;
    group->m_bytes += (u_int)((u_char *)end - (u_char *)node->m_chan.m_start);
    i3dCtxSetHead(group->m_ctx, (u_char *)i3dAlign128(end + 2));
    return 0;
}

// ROR 0x00223cc0

u_long *i3dGroupBeginChanPacketUnaligned(I3dGroup *group, u_int key, int channel)
{
    I3dCallNode *node = (I3dCallNode *)i3dCtxAllocTail(group->m_ctx, I3D_QWC_OF(I3dCallNode));
    u_long *start = (u_long *)i3dCtxGetHead(group->m_ctx);

    group->m_open = &node->m_node;
    node->m_chan.m_channel = channel;
    node->m_chan.m_start = start;
    node->m_node.m_func = i3dNodeChanPacket;
    node->m_node.m_key = key;
    node->m_node.m_data = NULL;
    i3dGroupLink(group, &node->m_node);
    return start;
}

// ROR 0x00223d18

int i3dGroupEndChanPacketUnaligned(I3dGroup *group, u_long *end)
{
    I3dCallNode *node = (I3dCallNode *)group->m_open;

    node->m_chan.m_end = end;
    group->m_bytes += (u_int)((u_char *)end - (u_char *)node->m_chan.m_start);
    i3dCtxSetHead(group->m_ctx, (u_char *)(end + 2));
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Moves src's nodes to the end of dst (i3dCtxDrawTexturedLayer merges a
// run's draw sets into its first set's group); src is left empty.
// ROR 0x00223d48

int i3dGroupAppend(I3dGroup *dst, I3dGroup *src)
{
    if (src->m_count != 0)
    {
        dst->m_last->m_next = src->m_first;
        dst->m_last = src->m_last;
        dst->m_count += src->m_count;
        src->m_first = NULL;
        dst->m_bytes += src->m_bytes;
        src->m_last = i3dHead(src);
        src->m_bytes = 0;
        src->m_count = 0;
    }
    return 0;
}

/* ---- sorts ----------------------------------------------------------------- */

/* PC: the ROM's scratchpad: two 256-bucket arrays of list heads (0x70000000
 * and 0x70002000) for the radix sort, (key, node) pairs from 0x70000000 for
 * the small sort. */
static I3dNode s_radixBuckets[2][0x100];

/* layout: host -- a (key, node) pair of the small sort. */
struct I3dSortPair
{
    u_int    m_key;
    I3dNode *m_node;
};
static I3dSortPair s_sortPairs[0x800];

// ──────────────────────────────────────────────────────────────────────
// Appends each node of a list to bucket (key >> shift) & 0xff.
// ROR 0x00223da0

void i3dRadixScatter(I3dNode *buckets, I3dNode *list, u_int shift)
{
    while (list != NULL)
    {
        I3dNode *b = &buckets[(list->m_key >> shift) & 0xff];
        I3dNode *last = b->m_last;
        b->m_last = list;
        last->m_next = list;
        list = list->m_next;
    }
}

// The same on the inverted key: buckets in descending order.
// ROR 0x00223de0

void i3dRadixScatterDesc(I3dNode *buckets, I3dNode *list, u_int shift)
{
    while (list != NULL)
    {
        I3dNode *b = &buckets[(~list->m_key >> shift) & 0xff];
        I3dNode *last = b->m_last;
        b->m_last = list;
        last->m_next = list;
        list = list->m_next;
    }
}

// ──────────────────────────────────────────────────────────────────────
// LSD radix sort of the 32-bit keys, four 8-bit passes between two bucket
// arrays (stable); order 1 ascending, 2 descending. Returns the first node.
// ROR 0x00223e20

I3dNode *i3dSortRadix(I3dNode *list, int order)
{
    I3dNode *dst = s_radixBuckets[0];

    for (int pass = 0; pass < 4; pass++)
    {
        dst = s_radixBuckets[pass & 1];
        I3dNode *src = s_radixBuckets[~pass & 1];

        for (int i = 0; i < 0x100; i++)
        {
            dst[i].m_next = NULL;
            dst[i].m_last = &dst[i];
        }
        if (order == 1)
        {
            if (pass == 0)
            {
                i3dRadixScatter(dst, list, 0);
            }
            else
            {
                for (int i = 0; i < 0x100; i++)
                {
                    if (src[i].m_next != NULL)
                    {
                        src[i].m_last->m_next = NULL;
                        i3dRadixScatter(dst, src[i].m_next, pass << 3);
                    }
                }
            }
        }
        else if (order == 2)
        {
            if (pass == 0)
            {
                i3dRadixScatterDesc(dst, list, 0);
            }
            else
            {
                for (int i = 0; i < 0x100; i++)
                {
                    if (src[i].m_next != NULL)
                    {
                        src[i].m_last->m_next = NULL;
                        i3dRadixScatterDesc(dst, src[i].m_next, pass << 3);
                    }
                }
            }
        }
    }

    // Concatenate the last pass's buckets.
    I3dNode *first = NULL;
    I3dNode *tail = NULL;
    int i = 0;
    for (; i < 0x100; i++)
    {
        if (dst[i].m_next != NULL)
        {
            first = dst[i].m_next;
            tail = dst[i].m_last;
            i++;
            break;
        }
    }
    for (; i < 0x100; i++)
    {
        if (dst[i].m_next != NULL)
        {
            tail->m_next = dst[i].m_next;
            tail = dst[i].m_last;
        }
    }
    // PC: with an empty result (an order other than 1 / 2) the ROM writes
    // through a NULL tail; no caller does that.
    if (tail != NULL)
        tail->m_next = NULL;
    return first;
}

// ──────────────────────────────────────────────────────────────────────
// A cocktail-shaker sort of (key, node) pairs for short lists (i3dSortList
// takes it below 32 nodes), then relinks the nodes; order 1 ascending, 2
// descending, anything else keeps the order.
// ROR 0x00224050

I3dNode *i3dSortSmall(I3dNode *list, int order)
{
    if (list->m_next == NULL)
        return list;

    int n = 0;
    for (I3dNode *p = list; p != NULL; p = p->m_next)
    {
        s_sortPairs[n].m_key = p->m_key;
        s_sortPairs[n].m_node = p;
        n++;
    }

    I3dSortPair *lo = &s_sortPairs[0];
    I3dSortPair *hi = &s_sortPairs[n - 1];
    I3dSortPair tmp;
    if (order == 1)
    {
        for (;;)
        {
            I3dSortPair *swapped = NULL;
            for (I3dSortPair *p = lo; p < hi; p++)
            {
                if (p[1].m_key < p[0].m_key)
                {
                    tmp = p[0];
                    p[0] = p[1];
                    p[1] = tmp;
                    swapped = p;
                }
            }
            if (swapped == NULL)
                break;
            hi = swapped;
            swapped = NULL;
            for (I3dSortPair *p = hi; lo < p; p--)
            {
                if (p[0].m_key < p[-1].m_key)
                {
                    tmp = p[0];
                    p[0] = p[-1];
                    p[-1] = tmp;
                    swapped = p;
                }
            }
            if (swapped == NULL)
                break;
            lo = swapped;
        }
    }
    else if (order == 2)
    {
        for (;;)
        {
            I3dSortPair *swapped = NULL;
            for (I3dSortPair *p = lo; p < hi; p++)
            {
                if (p[0].m_key < p[1].m_key)
                {
                    tmp = p[0];
                    p[0] = p[1];
                    p[1] = tmp;
                    swapped = p;
                }
            }
            if (swapped == NULL)
                break;
            hi = swapped;
            swapped = NULL;
            for (I3dSortPair *p = hi; lo < p; p--)
            {
                if (p[-1].m_key < p[0].m_key)
                {
                    tmp = p[0];
                    p[0] = p[-1];
                    p[-1] = tmp;
                    swapped = p;
                }
            }
            if (swapped == NULL)
                break;
            lo = swapped;
        }
    }

    for (int i = 0; i < n - 1; i++)
        s_sortPairs[i].m_node->m_next = s_sortPairs[i + 1].m_node;
    s_sortPairs[n - 1].m_node->m_next = NULL;
    return s_sortPairs[0].m_node;
}

// Sort table slot 0: the list as it was built.
// ROR 0x002242e0

I3dNode *i3dSortNone(I3dNode *list, int, int, u_int)
{
    return list;
}

// Sort table slot 1.
// ROR 0x002242e8

I3dNode *i3dSortDefault(I3dNode *list, int order, int param, u_int count)
{
    return i3dSortList(list, order, param, count);
}

/* data 0x3031f0: a group's sort functions (slot 3 is NULL in the ROM). */
typedef I3dNode *(*I3dSortFunc)(I3dNode *list, int order, int param, u_int count);
static const I3dSortFunc s_i3dSortFuncs[4] = {
    i3dSortNone, i3dSortDefault, i3dSortList, NULL,
};

// ──────────────────────────────────────────────────────────────────────
// Terminates the group's list and sorts it with its sort function.
// ROR 0x00224300

int i3dGroupSort(I3dGroup *group)
{
    if (group->m_count != 0)
    {
        group->m_last->m_next = NULL;
        group->m_first = s_i3dSortFuncs[group->m_sort_func](group->m_first, group->m_sort_order,
                                                            group->m_sort_param, group->m_count);
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Sorts a terminated list by key: the small sort below 32 nodes, the
// radix sort from there.
// ROR 0x00224368

I3dNode *i3dSortList(I3dNode *list, int order, int, u_int count)
{
    if (count < 0x20)
        return i3dSortSmall(list, order);
    return i3dSortRadix(list, order);
}

// ──────────────────────────────────────────────────────────────────────
// A new group: an empty list, its context and sort.
// ROR 0x002243a0

int i3dGroupInit(I3dGroup *group, i3dContext *ctx, int sort_func, int sort_order, int sort_param)
{
    i3dListInit(i3dHead(group));
    group->m_ctx = ctx;
    group->m_sort_func = sort_func;
    group->m_sort_order = sort_order;
    group->m_sort_param = sort_param;
    group->m_bytes = 0;
    group->m_open = NULL;
    group->m_count = 0;
    return 0;
}

/* ---- the node callbacks ------------------------------------------------------ */

// ROR 0x00224418

int i3dNodeRunGroup(i3dContext *ctx, I3dNode *node)
{
    i3dCtxRunGroup(ctx, ((I3dCallNode *)node)->m_group);
    return 0;
}

// The node i3dCtxRunGroup appends: stops the run.
// ROR 0x00224438

int i3dNodeEnd(i3dContext *, I3dNode *)
{
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// When the channel's last state differs, links a CALL of the state packet
// at the head (the open tag goes after it).
// ROR 0x00224440

int i3dNodeState(i3dContext *ctx, I3dNode *node)
{
    I3dCallNode *n = (I3dCallNode *)node;
    u_int state = n->m_state.m_state;

    if (state != ctx->m_chan_state[n->m_state.m_channel])
    {
        ctx->m_chan_state[n->m_state.m_channel] = state;
        u_long *head = (u_long *)i3dCtxGetHead(ctx);
        u_long *tag = ctx->m_chain;
        tag[1] = 0;
        tag[0] = ((u_long)i3dDmaAddr(head) << 32) | I3D_DMA_NEXT;
        head[0] = ((u_long)state << 32) | I3D_DMA_CALL;
        head[1] = 0;
        i3dCtxSetHead(ctx, (u_char *)(head + 4));
        ctx->m_chain = head + 2;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The channel's state CALL as i3dNodeState (only when it changed), then
// the node's packet; the chain goes on from the packet's end.
// ROR 0x002244f0

int i3dNodeStatePacket(i3dContext *ctx, I3dNode *node)
{
    I3dCallNode *n = (I3dCallNode *)node;
    u_int state = n->m_state.m_state;
    u_long *tag = ctx->m_chain;

    if (state == ctx->m_chan_state[n->m_state.m_channel])
    {
        tag[1] = 0;
        tag[0] = ((u_long)i3dDmaAddr(n->m_state.m_start) << 32) | I3D_DMA_NEXT;
    }
    else
    {
        ctx->m_chan_state[n->m_state.m_channel] = state;
        u_long *head = (u_long *)i3dCtxGetHead(ctx);
        tag[1] = 0;
        tag[0] = ((u_long)i3dDmaAddr(head) << 32) | I3D_DMA_NEXT;
        head[0] = ((u_long)state << 32) | I3D_DMA_CALL;
        head[1] = 0;
        head[3] = 0;
        head[2] = ((u_long)i3dDmaAddr(n->m_state.m_start) << 32) | I3D_DMA_NEXT;
        i3dCtxSetHead(ctx, (u_char *)(head + 4));
    }
    ctx->m_chain = n->m_state.m_end;
    return 0;
}

// ROR 0x002245f8

int i3dNodeClearState(i3dContext *ctx, I3dNode *node)
{
    ctx->m_chan_state[((I3dCallNode *)node)->m_state.m_channel] = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Forgets the channel's state (the packet sets it) and links the packet.
// ROR 0x00224620

int i3dNodeChanPacket(i3dContext *ctx, I3dNode *node)
{
    I3dCallNode *n = (I3dCallNode *)node;
    u_long *tag = ctx->m_chain;

    ctx->m_chan_state[n->m_chan.m_channel] = 0;
    tag[1] = 0;
    tag[0] = ((u_long)i3dDmaAddr(n->m_chan.m_start) << 32) | I3D_DMA_NEXT;
    ctx->m_chain = n->m_chan.m_end;
    return 0;
}

// ROR 0x00224670

int i3dNodePrint(i3dContext *, I3dNode *node)
{
    i3dPrintf(((I3dCallNode *)node)->m_text);
    return 0;
}

// ROR 0x00224698

int i3dNodeCall(i3dContext *, I3dNode *node)
{
    I3dCallNode *n = (I3dCallNode *)node;
    return n->m_call.m_func(n->m_call.m_arg0, n->m_call.m_arg1) != 0;
}

/* ---- running groups -------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// Emits a group into the frame's chain: appends the stop node (key -1
// when ascending so it sorts last, else 0), sorts, then walks the nodes --
// runs of packet nodes are linked with NEXT tags, each call node's
// callback runs in turn -- until a callback returns non-zero.
// ROR 0x002246c8

int i3dCtxRunGroup(i3dContext *ctx, I3dGroup *group)
{
    i3dGroupAddEnd(group, group->m_sort_order == 1 ? 0xffffffffu : 0);
    i3dGroupSort(group);

    I3dNode *node = group->m_first;
    int stop;
    do
    {
        u_long *tag = ctx->m_chain;
        while (node->m_data != NULL)
        {
            tag[1] = 0;
            tag[0] = ((u_long)i3dDmaAddr(node->m_data) << 32) | I3D_DMA_NEXT;
            tag = node->m_end;
            node = node->m_next;
        }
        ctx->m_chain = tag;
        stop = node->m_func(ctx, node);
        node = node->m_next;
    } while (stop == 0);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The frame's DMA chain: the opening tags at the head (i3dPktOpenChain),
// the channel states cleared, the root group run, a NEXT to the end
// hook's tags.
// ROR 0x00224798

int i3dCtxCloseFrame(i3dContext *ctx)
{
    u_char *head = i3dCtxGetHead(ctx);

    // PC: the ROM keeps head & 0x0fffffff (the EE address for the DMA).
    ctx->m_packet = head;
    ctx->m_chain = i3dPktOpenChain(head);
    i3dCtxSetHead(ctx, (u_char *)(ctx->m_chain + 2));
    for (u_int i = 0; i < 4; i++)
        ctx->m_chan_state[i] = 0;
    i3dCtxRunGroup(ctx, ctx->m_root);

    head = i3dCtxGetHead(ctx);
    u_long *tag = ctx->m_chain;
    tag[1] = 0;
    tag[0] = ((u_long)i3dDmaAddr(head) << 32) | I3D_DMA_NEXT;
    i3dCtxSetHead(ctx, (u_char *)ctx->m_end_func((u_long *)head, ctx->m_end_arg));
    return 0;
}

/* ---- adding nodes ---------------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// A node that runs a sub group (a layer's group in its parent).
// ROR 0x00224878

void i3dGroupAddGroup(I3dGroup *group, u_int key, I3dGroup *sub)
{
    I3dCallNode *node = i3dGroupNewCallNode(group, key, i3dNodeRunGroup);
    node->m_group = sub;
    i3dGroupLink(group, &node->m_node);
}

// ROR 0x002248c0

void i3dGroupAddEnd(I3dGroup *group, u_int key)
{
    // PC: the ROM takes 1 qword (a bare node).
    I3dNode *node = (I3dNode *)i3dCtxAllocTail(group->m_ctx, I3D_QWC_OF(I3dNode));
    node->m_func = i3dNodeEnd;
    node->m_key = key;
    node->m_data = NULL;
    i3dGroupLink(group, node);
}

// ROR 0x00224900

void i3dGroupAddState(I3dGroup *group, u_int key, int channel, u_int state)
{
    I3dCallNode *node = i3dGroupNewCallNode(group, key, i3dNodeState);
    node->m_state.m_channel = channel;
    node->m_state.m_state = state;
    i3dGroupLink(group, &node->m_node);
}

// ROR 0x00224948

void i3dGroupAddStatePacket(I3dGroup *group, u_int key, int channel, u_int state,
                            u_long *start, u_long *end)
{
    I3dCallNode *node = i3dGroupNewCallNode(group, key, i3dNodeStatePacket);
    node->m_state.m_channel = channel;
    node->m_state.m_state = state;
    node->m_state.m_start = start;
    node->m_state.m_end = end;
    i3dGroupLink(group, &node->m_node);
}

// ROR 0x00224998

void i3dGroupAddClearState(I3dGroup *group, u_int key, int channel)
{
    I3dCallNode *node = i3dGroupNewCallNode(group, key, i3dNodeClearState);
    node->m_state.m_channel = channel;
    i3dGroupLink(group, &node->m_node);
}

// ROR 0x002249e0

void i3dGroupAddChanPacket(I3dGroup *group, u_int key, int channel, u_long *start, u_long *end)
{
    I3dCallNode *node = i3dGroupNewCallNode(group, key, i3dNodeChanPacket);
    node->m_chan.m_channel = channel;
    node->m_chan.m_start = start;
    node->m_chan.m_end = end;
    i3dGroupLink(group, &node->m_node);
}

// ──────────────────────────────────────────────────────────────────────
// A print node: the string is copied into the packet tail (16-byte
// rounded) just above the node.
// ROR 0x00224a30

void i3dGroupAddPrint(I3dGroup *group, u_int key, const char *text)
{
    i3dContext *ctx = group->m_ctx;
    size_t size = (strlen(text) + 0x10) & ~(size_t)0xf;
    char *copy = (char *)(ctx->m_tail - size);

    // PC: the ROM puts its 0x20-byte node right below the copy.
    ctx->m_tail = (u_char *)copy;
    strcpy(copy, text);
    I3dCallNode *node = i3dGroupNewCallNode(group, key, i3dNodePrint);
    node->m_text = copy;
    i3dGroupLink(group, &node->m_node);
}

// ROR 0x00224ad8

void i3dGroupAddCall(I3dGroup *group, u_int key, int (*func)(void *, void *), void *arg0, void *arg1)
{
    I3dCallNode *node = i3dGroupNewCallNode(group, key, i3dNodeCall);
    node->m_call.m_func = func;
    node->m_call.m_arg0 = arg0;
    node->m_call.m_arg1 = arg1;
    i3dGroupLink(group, &node->m_node);
}

/* ---- texture registers ----------------------------------------------------- */

// ──────────────────────────────────────────────────────────────────────
// The texture register packet of a draw-set slot: TEX0 / TEX1 / TEX2 /
// MIPTBP1 / MIPTBP2 from the texture object, the CLUT object's bits or-ed
// in, then the VRAM addresses the heaps gave: TBP0 and the mip levels from
// the texture's block (pages -> 64-word blocks), CBP from the CLUT's block
// (4-block units) plus the CLUT index for 8-bit textures (whole 256-entry
// CLUTs) or for 4-bit ones its high bits (16-entry CLUTs, CSA = the low
// four). A handle placed by hand (m_static) keeps the addresses it has.
// ROR 0x00224b28

void i3dTexRegsSet(u_long *regs, I3dVramHandle *tex, I3dVramHandle *clut, u_int clut_index)
{
    u_long cbp = 0;
    u_long csa = 0;
    u_long tbp = 0;

    regs[0] = I3D_GIF_AD1_EOP | 5;
    regs[1] = I3D_GIF_AD;
    regs[3] = I3D_GS_TEX0_1;
    regs[5] = I3D_GS_TEX1_1;
    regs[7] = I3D_GS_TEX2_1;
    regs[9] = I3D_GS_MIPTBP1_1;
    regs[11] = I3D_GS_MIPTBP2_1;
    if (tex == NULL)
    {
        regs[2] = 0;
        regs[4] = 0;
        regs[6] = 0;
        regs[8] = 0;
        regs[10] = 0;
    }
    else
    {
        I3dTexObject *t = (I3dTexObject *)tex;
        regs[2] = t->m_tex0;
        regs[4] = t->m_tex1;
        regs[6] = t->m_tex2;
        regs[8] = t->m_miptbp1;
        regs[10] = t->m_miptbp2;
        if (tex->m_static == 0 && tex->m_block != NULL)
            tbp = (u_long)(tex->m_block->m_addr << 5);
    }

    if (clut != NULL)
    {
        I3dTexObject *c = (I3dTexObject *)clut;
        regs[2] |= c->m_tex0;
        regs[6] |= c->m_tex1;
        u_long tex2 = regs[6];
        if (clut->m_static == 0 && clut->m_block != NULL)
            cbp = (u_long)(clut->m_block->m_addr << 2);

        int psm_bits = i3dPsmBits((int)((tex2 >> 20) & 0x3f));
        int cpsm_bits = i3dPsmBits((int)((tex2 >> 51) & 0xf));
        if (cpsm_bits == 0x20 && psm_bits == 8)
        {
            cbp += clut_index << 2;
        }
        else if (cpsm_bits == 0x20 && psm_bits == 4)
        {
            csa = clut_index & 0xf;
            cbp += (clut_index >> 4) << 2;
        }
        else if (cpsm_bits == 0x10 && psm_bits == 8)
        {
            cbp += clut_index << 1;
        }
        else if (cpsm_bits == 0x10 && psm_bits == 4)
        {
            csa = clut_index & 0xf;
            cbp += (clut_index >> 4) << 1;
        }
    }

    u_long mip = tbp | (tbp << 20) | (tbp << 40);
    regs[2] += tbp | (cbp << 37);
    regs[6] += csa << 56;
    regs[10] += mip;
    regs[8] += mip;
}

// ──────────────────────────────────────────────────────────────────────
// The same packet with every register zero (no texture).
// ROR 0x00224d88

void i3dTexRegsSetNone(u_long *regs)
{
    regs[2] = 0;
    regs[11] = I3D_GS_MIPTBP2_1;
    regs[4] = 0;
    regs[6] = 0;
    regs[0] = I3D_GIF_AD1_EOP | 5;
    regs[1] = I3D_GIF_AD;
    regs[8] = 0;
    regs[3] = I3D_GS_TEX0_1;
    regs[5] = I3D_GS_TEX1_1;
    regs[10] = 0;
    regs[7] = I3D_GS_TEX2_1;
    regs[9] = I3D_GS_MIPTBP1_1;
}
