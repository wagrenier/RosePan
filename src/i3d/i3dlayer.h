/* ==========================================================================
 *  i3d/i3dlayer.h
 *
 *  The i3d draw groups and layers (i3d/i3dlayer.cpp, ROR 0x222f50..0x224de0).
 *
 *  A frame's packets are not written in draw order: every piece of packet
 *  becomes a node (a sort key and a link) in a draw group, and the group is
 *  sorted and linked into the frame's DMA chain when the frame closes
 *  (i3dCtxCloseFrame -> i3dCtxRunGroup). A node is either
 *  - a packet node (EE 0x10): m_data = the packet's first qword, m_end =
 *    where its writer stopped; the run links it with a NEXT tag and goes on
 *    from m_end, or
 *  - a call node (EE 0x20, I3dCallNode): m_data NULL, m_func a callback the
 *    run calls; a non-zero result stops the group. The callbacks link a
 *    packet that also forgets a channel's state (the i3dGroup*ChanPacket
 *    pair), CALL a channel's state packet only when it changed (the
 *    i3dGroup*State* family; the context keeps the last state per channel),
 *    run a sub group, print, call a function, or end the run.
 *  A group's sort function comes from the table at data 0x3031f0 (none, or
 *  radix / cocktail sort of the keys, ascending or descending).
 *
 *  A layer (i3dCtxCreateLayer) has its own group in the frame's root group
 *  and draws through i3dCtxDrawTexturedLayer (a call node of that group):
 *  its packets are grouped by draw set -- the textures and group one run of
 *  packets needs, found by hashing a texture-set description -- and each
 *  set's packets go into the set's own group. Layer state (light set, fog,
 *  view) is what the packet builders CALL before drawing.
 *
 *  The library has no symbols and no asserts: every name is inferred from
 *  the code. Packet positions are EE addresses in the ROM (aligned starts
 *  without the segment bits, heads in the uncached-accelerated segment);
 *  the host keeps host pointers (i3dctx.h).
 * ======================================================================== */
#ifndef _I3D_I3DLAYER_H
#define _I3D_I3DLAYER_H

#include <stddef.h>

#include "i3d/i3dctx.h"

struct I3dNode;
struct I3dGroup;
struct I3dLight;
struct I3dView;

/* A call node's callback: 0 = go on with the next node. */
typedef int (*I3dNodeFunc)(i3dContext *ctx, I3dNode *node);

/* A list node (EE 0x10) and the head of every node list: groups and layers
 * start with the same four words, and an empty list's m_last points at the
 * head itself, so appending writes the head's m_first (i3dListInit).
 * layout: host -- pointers. */
struct I3dNode
{
    u_int    m_key;             // ROR 0x00: the sort key
    I3dNode *m_next;            // ROR 0x04
    void    *m_data;            // ROR 0x08: a packet node's start, a layer node's draw set; NULL in a call node
    union
    {
        u_long      *m_end;     // ROR 0x0c: a packet node: where its packet ends (the next tag goes there)
        I3dNodeFunc  m_func;    // ROR 0x0c: a call node: its callback
        I3dNode     *m_last;    // ROR 0x0c: a list head: its last node (the head itself when empty)
    };
};
static_assert(offsetof(I3dNode, m_key) == 0, "I3dNode::m_key");

/* A call node (EE 0x20). The payload words depend on the callback.
 * layout: host -- pointers. */
struct I3dCallNode
{
    I3dNode m_node;                         // ROR 0x00: m_data NULL, m_func the callback
    union
    {
        I3dGroup   *m_group;                // ROR 0x10: i3dNodeRunGroup's sub group
        const char *m_text;                 // ROR 0x10: i3dNodePrint's string
        struct
        {
            int     m_channel;              // ROR 0x10
            u_int   m_state;                // ROR 0x14: the state packet's DMA address
            u_long *m_start;                // ROR 0x18: i3dNodeStatePacket's packet
            u_long *m_end;                  // ROR 0x1c
        } m_state;                          // i3dNodeState / StatePacket / ClearState
        struct
        {
            int     m_channel;              // ROR 0x10
            u_long *m_start;                // ROR 0x14
            u_long *m_end;                  // ROR 0x18
        } m_chan;                           // i3dNodeChanPacket
        struct
        {
            int   (*m_func)(void *, void *);// ROR 0x10
            void   *m_arg0;                 // ROR 0x14
            void   *m_arg1;                 // ROR 0x18
        } m_call;                           // i3dNodeCall
    };
};
static_assert(offsetof(I3dCallNode, m_node) == 0, "I3dCallNode::m_node");

/* A draw group (EE 0x30, from the packet tail; i3dGroupInit 0x2243a0).
 * layout: host -- pointers; the first four words match I3dNode. */
struct I3dGroup
{
    u_int       m_key;          // ROR 0x00
    I3dNode    *m_first;        // ROR 0x04
    void       *m_08;           // ROR 0x08: always NULL (i3dListInit)
    I3dNode    *m_last;         // ROR 0x0c: the group itself (as a node) when empty
    i3dContext *m_ctx;          // ROR 0x10
    int         m_sort_func;    // ROR 0x14: index into the sort table (data 0x3031f0)
    int         m_sort_order;   // ROR 0x18: 1 ascending, 2 descending, else unsorted
    int         m_sort_param;   // ROR 0x1c: the sort function's third argument (unused by them)
    I3dNode    *m_open;         // ROR 0x20: the node a Begin* opened, closed by its End*
    u_int       m_count;        // ROR 0x24: nodes
    u_int       m_bytes;        // ROR 0x28: bytes of packet in its packet nodes; 0 = nothing to draw
};
static_assert(offsetof(I3dGroup, m_first) == offsetof(I3dNode, m_next), "a group's head is a node");
static_assert(offsetof(I3dGroup, m_last) == offsetof(I3dNode, m_last), "a group's head is a node");

/* A draw set (EE 0x38 in 4 qwords; i3dDrawSetInit 0x21b320): one texture
 * set's register packets and the group its packets go into. Sets of a
 * layer hang in a circular list per hash bucket (I3dLayer::m_sets).
 * layout: host -- pointers. */
struct I3dDrawSet
{
    u_int           m_hash;         // ROR 0x00: i3dLayerFindDrawSet's hash of the description
    I3dDrawSet     *m_next;         // ROR 0x04: the bucket's circular list
    I3dDrawSet     *m_prev;         // ROR 0x08
    I3dGroup       *m_group;        // ROR 0x0c: the packets
    u_long        **m_tex_regs;     // ROR 0x10: per slot, its texture register packet (6 qwords)
    int            *m_tex_used;     // ROR 0x14: per slot, non-zero = in use
    struct I3dLayer *m_layer;       // ROR 0x18
    I3dDrawSet     *m_reserved_next;// ROR 0x1c: the context's list of reserved sets
    float           m_depth;        // ROR 0x20: the description's depth; equal depth and sort func = one run
    u_int          *m_tex_index;    // ROR 0x24: per slot, an index into the texture table
    u_int           m_tex_count;    // ROR 0x28: slots
    u_int           m_sort_func;    // ROR 0x2c: the group's sort function (description +0xc)
    u_int           m_sort_order;   // ROR 0x30: description +0x10
    u_int           m_sort_param;   // ROR 0x34: description +0x14
};
static_assert(offsetof(I3dDrawSet, m_hash) == 0, "I3dDrawSet::m_hash");

/* A layer (EE 0xe60, from the packet tail; i3dLayerInit 0x223878).
 * layout: host -- pointers; the first four words match I3dNode. */
struct I3dLayer
{
    u_int       m_key;              // ROR 0x00
    I3dNode    *m_first;            // ROR 0x04: the draw-set nodes (key = the set's depth)
    void       *m_08;               // ROR 0x08: always NULL
    I3dNode    *m_last;             // ROR 0x0c
    u_int       m_count;            // ROR 0x10
    I3dDrawSet *m_sets[0x100];      // ROR 0x14: hash buckets of draw sets
    u_int       m_414[0x100];       // ROR 0x414: cleared by i3dLayerInit; no reader found
    i3dContext *m_ctx;              // ROR 0x814
    I3dGroup   *m_set_group;        // ROR 0x818: the current draw set's group (Begin / EndSet)
    I3dGroup   *m_group;            // ROR 0x81c: the layer's group in its parent
    I3dGroup   *m_sub_group;        // ROR 0x820: i3dLayerCreateSubLayer's group, made on first use
    I3dDrawSet *m_default_set;      // ROR 0x824: the set of the empty description
    I3dDrawSet *m_cur_set;          // ROR 0x828
    I3dLight   *m_light;            // ROR 0x82c
    void       *m_fog;              // ROR 0x830: i3dCtxSetFog's result
    I3dView    *m_view;             // ROR 0x834
    u_long      m_user[0x62][2];    // ROR 0x838: i3dLayerSetUser / SetUser2 (no caller found)
};
static_assert(offsetof(I3dLayer, m_first) == offsetof(I3dNode, m_next), "a layer's head is a node");
static_assert(offsetof(I3dLayer, m_last) == offsetof(I3dNode, m_last), "a layer's head is a node");

/* A texture-set description (EE 0x18): what i3dLayerFindDrawSet hashes
 * (every word but the pointer) and compares. layout: host -- a pointer. */
struct I3dTexSetDesc
{
    float  m_depth;             // ROR 0x00: the set's sort key in its layer
    int   *m_tex_no;            // ROR 0x04: the texture numbers
    u_int  m_tex_count;         // ROR 0x08
    u_int  m_sort_func;         // ROR 0x0c: the set group's sort function
    u_int  m_sort_order;        // ROR 0x10
    u_int  m_sort_param;        // ROR 0x14
};
static_assert(offsetof(I3dTexSetDesc, m_depth) == 0, "I3dTexSetDesc::m_depth");

/* The texture and CLUT objects i3dTexRegsSet reads (I3dTexture, I3dClut)
 * are in i3d/i3dtex.h. */

/* A model material as i3dLayerFindMaterialDrawSet reads it (the model code
 * at 0x228f50 passes it). Roles from that one reader; unverified.
 * layout: host -- pointers. */
struct I3rTexRef
{
    u_char _00[0x1c];
    int    m_tex_no;            // ROR 0x1c: -1 = none
};
/* layout: host -- a pointer. */
struct I3rTexRefEntry
{
    I3rTexRef *m_tex;           // ROR 0x00
    u_int      _04[3];          // ROR 0x04
};
/* layout: host -- a pointer. */
struct I3rTexList
{
    u_int           _00;        // ROR 0x00
    u_int           m_count;    // ROR 0x04: low 24 bits
    I3rTexRefEntry *m_entry;    // ROR 0x08
};
/* layout: host -- plain words (the EE offsets in the comments). */
struct I3rMatParam
{
    float  m_depth;             // ROR 0x00
    u_int  m_sort_param;        // ROR 0x04
    u_char _08[2];              // ROR 0x08
    u_char m_sort_func;         // ROR 0x0a
    u_char m_sort_order;        // ROR 0x0b
};
/* layout: host -- pointers. */
struct I3rMaterial
{
    I3rMatParam *m_param;       // ROR 0x00
    void        *_04;           // ROR 0x04
    I3rTexList  *m_tex;         // ROR 0x08
};

/* ---- layers ---- */

u_long  *i3dPktLayerState(u_long *pkt, I3dLayer *layer);                           // ROR 0x00222f50
int      i3dLayerDrawLine(I3dLayer *layer, const float *from, const float *to,
                          const void *color);                                       // ROR 0x00222f88
int      i3dLayerDrawWireSphere(I3dLayer *layer, sceVu0FVECTOR *matrix, const float *center,
                                float radius);                                      // ROR 0x00223050
int      i3dLayerDrawWireBox(I3dLayer *layer, sceVu0FVECTOR *matrix, const float *center,
                             const void *extent, const void *color);               // ROR 0x002230e8
int      i3dLayerDrawWireShape(I3dLayer *layer, void *a, void *b, void *c);        // ROR 0x00223190
int      i3dLayerSetDrawSet(I3dLayer *layer, I3dDrawSet *set);                     // ROR 0x00223228
int      i3dLayerSetLight(I3dLayer *layer, I3dLight *light);                       // ROR 0x00223240
int      i3dLayerSetFog(I3dLayer *layer, void *fog);                               // ROR 0x00223250
int      i3dLayerSetView(I3dLayer *layer, I3dView *view);                          // ROR 0x00223260
I3dDrawSet *i3dLayerGetDrawSet(I3dLayer *layer);                                   // ROR 0x00223270
I3dDrawSet *i3dLayerGetCurDrawSet(I3dLayer *layer);                                // ROR 0x00223278
I3dLight *i3dLayerGetLight(I3dLayer *layer);                                       // ROR 0x00223280
void    *i3dLayerGetFog(I3dLayer *layer);                                          // ROR 0x00223288
I3dView *i3dLayerGetView(I3dLayer *layer);                                         // ROR 0x00223290
/* Opens a packet node with a sort key in the current draw set; its packet. */
u_long  *i3dLayerBeginSet(I3dLayer *layer, float key);                             // ROR 0x00223298
u_long  *i3dLayerBeginSetKey(I3dLayer *layer, u_int key);                          // ROR 0x002232d0
int      i3dLayerEndSet(I3dLayer *layer, u_long *end);                             // ROR 0x002232f0
u_long  *i3dLayerBeginSetUnaligned(I3dLayer *layer, float key);                    // ROR 0x00223308
u_long  *i3dLayerBeginSetUnalignedKey(I3dLayer *layer, u_int key);                 // ROR 0x00223340
int      i3dLayerEndSetUnaligned(I3dLayer *layer, u_long *end);                    // ROR 0x00223360
/* The layer group's packet before its draw sets (key 0) and after them
 * (key 2, the "post" packet); the draw call node has key 1. */
u_long  *i3dLayerBeginPacket(I3dLayer *layer);                                     // ROR 0x00223378
int      i3dLayerEndPacket(I3dLayer *layer, u_long *end);                          // ROR 0x00223398
u_long  *i3dLayerBeginPacketUnaligned(I3dLayer *layer);                            // ROR 0x002233b0
int      i3dLayerEndPacketUnaligned(I3dLayer *layer, u_long *end);                 // ROR 0x002233d0
u_long  *i3dLayerBeginPostPacket(I3dLayer *layer);                                 // ROR 0x002233e8
int      i3dLayerEndPostPacket(I3dLayer *layer, u_long *end);                      // ROR 0x00223408
u_long  *i3dLayerBeginPostPacketUnaligned(I3dLayer *layer);                        // ROR 0x00223420
int      i3dLayerEndPostPacketUnaligned(I3dLayer *layer, u_long *end);             // ROR 0x00223440
int      i3dLayerSetUser(I3dLayer *layer, int n, u_long value);                    // ROR 0x00223458
int      i3dLayerSetUser2(I3dLayer *layer, int n, u_long value);                   // ROR 0x00223470
u_long   i3dLayerGetUser(I3dLayer *layer, int n);                                  // ROR 0x00223488
u_long   i3dLayerGetUser2(I3dLayer *layer, int n);                                 // ROR 0x00223498
I3dDrawSet *i3dLayerFindMaterialDrawSet(I3dLayer *layer, const I3rMaterial *mat,
                                        const I3dDrawSet *base);                   // ROR 0x002234a8
I3dDrawSet *i3dLayerFindDrawSet(I3dLayer *layer, const I3dTexSetDesc *desc);       // ROR 0x00223558
I3dLayer *i3dLayerCreateSubLayer(I3dLayer *layer, float key);                      // ROR 0x00223768
int      i3dLayerAddVramReset(I3dLayer *layer, float key);                         // ROR 0x00223808
int      i3dLayerInit(I3dLayer *layer, i3dContext *ctx);                           // ROR 0x00223878

/* ---- groups ---- */

void     i3dListInit(I3dNode *head);                                               // ROR 0x002239b8
u_long  *i3dGroupBeginPacket(I3dGroup *group, u_int key);                          // ROR 0x002239d0
int      i3dGroupEndPacket(I3dGroup *group, u_long *end);                          // ROR 0x00223a20
u_long  *i3dGroupBeginPacketUnaligned(I3dGroup *group, u_int key);                 // ROR 0x00223a68
int      i3dGroupEndPacketUnaligned(I3dGroup *group, u_long *end);                 // ROR 0x00223aa8
u_long  *i3dGroupBeginStatePacket(I3dGroup *group, u_int key, int channel, u_int state); // ROR 0x00223ad8
int      i3dGroupEndStatePacket(I3dGroup *group, u_long *end);                     // ROR 0x00223b40
u_long  *i3dGroupBeginStatePacketUnaligned(I3dGroup *group, u_int key, int channel,
                                           u_int state);                           // ROR 0x00223b88
int      i3dGroupEndStatePacketUnaligned(I3dGroup *group, u_long *end);            // ROR 0x00223be0
u_long  *i3dGroupBeginChanPacket(I3dGroup *group, u_int key, int channel);         // ROR 0x00223c10
int      i3dGroupEndChanPacket(I3dGroup *group, u_long *end);                      // ROR 0x00223c78
u_long  *i3dGroupBeginChanPacketUnaligned(I3dGroup *group, u_int key, int channel); // ROR 0x00223cc0
int      i3dGroupEndChanPacketUnaligned(I3dGroup *group, u_long *end);             // ROR 0x00223d18
int      i3dGroupAppend(I3dGroup *dst, I3dGroup *src);                             // ROR 0x00223d48
void     i3dRadixScatter(I3dNode *buckets, I3dNode *list, u_int shift);            // ROR 0x00223da0
void     i3dRadixScatterDesc(I3dNode *buckets, I3dNode *list, u_int shift);        // ROR 0x00223de0
I3dNode *i3dSortRadix(I3dNode *list, int order);                                   // ROR 0x00223e20
I3dNode *i3dSortSmall(I3dNode *list, int order);                                   // ROR 0x00224050
I3dNode *i3dSortNone(I3dNode *list, int order, int param, u_int count);            // ROR 0x002242e0
I3dNode *i3dSortDefault(I3dNode *list, int order, int param, u_int count);         // ROR 0x002242e8
int      i3dGroupSort(I3dGroup *group);                                            // ROR 0x00224300
I3dNode *i3dSortList(I3dNode *list, int order, int param, u_int count);            // ROR 0x00224368
int      i3dGroupInit(I3dGroup *group, i3dContext *ctx, int sort_func, int sort_order,
                      int sort_param);                                             // ROR 0x002243a0
int      i3dNodeRunGroup(i3dContext *ctx, I3dNode *node);                          // ROR 0x00224418
int      i3dNodeEnd(i3dContext *ctx, I3dNode *node);                               // ROR 0x00224438
int      i3dNodeState(i3dContext *ctx, I3dNode *node);                             // ROR 0x00224440
int      i3dNodeStatePacket(i3dContext *ctx, I3dNode *node);                       // ROR 0x002244f0
int      i3dNodeClearState(i3dContext *ctx, I3dNode *node);                        // ROR 0x002245f8
int      i3dNodeChanPacket(i3dContext *ctx, I3dNode *node);                        // ROR 0x00224620
int      i3dNodePrint(i3dContext *ctx, I3dNode *node);                             // ROR 0x00224670
int      i3dNodeCall(i3dContext *ctx, I3dNode *node);                              // ROR 0x00224698
int      i3dCtxRunGroup(i3dContext *ctx, I3dGroup *group);                         // ROR 0x002246c8
int      i3dCtxCloseFrame(i3dContext *ctx);                                        // ROR 0x00224798
void     i3dGroupAddGroup(I3dGroup *group, u_int key, I3dGroup *sub);              // ROR 0x00224878
void     i3dGroupAddEnd(I3dGroup *group, u_int key);                               // ROR 0x002248c0
void     i3dGroupAddState(I3dGroup *group, u_int key, int channel, u_int state);   // ROR 0x00224900
void     i3dGroupAddStatePacket(I3dGroup *group, u_int key, int channel, u_int state,
                                u_long *start, u_long *end);                       // ROR 0x00224948
void     i3dGroupAddClearState(I3dGroup *group, u_int key, int channel);           // ROR 0x00224998
void     i3dGroupAddChanPacket(I3dGroup *group, u_int key, int channel, u_long *start,
                               u_long *end);                                       // ROR 0x002249e0
void     i3dGroupAddPrint(I3dGroup *group, u_int key, const char *text);           // ROR 0x00224a30
void     i3dGroupAddCall(I3dGroup *group, u_int key, int (*func)(void *, void *),
                         void *arg0, void *arg1);                                  // ROR 0x00224ad8

/* ---- texture registers ---- */

/* Fills a 6-qword A+D packet (TEX0 / TEX1 / TEX2 / MIPTBP1 / MIPTBP2) from
 * a texture and CLUT object and the VRAM addresses their blocks have. */
void     i3dTexRegsSet(u_long *regs, I3dVramHandle *tex, I3dVramHandle *clut,
                       u_int clut_index);                                          // ROR 0x00224b28
void     i3dTexRegsSetNone(u_long *regs);                                          // ROR 0x00224d88

#endif /* _I3D_I3DLAYER_H */
