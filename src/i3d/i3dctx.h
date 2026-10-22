/* ==========================================================================
 *  i3d/i3dctx.h
 *
 *  The i3d draw context (i3d/i3dctx.cpp, ROR 0x21eb28..0x21fb30), the core
 *  of the i3d library ("i3d_package_version 050121-stable-050826") under
 *  RoR's scene: one packet buffer split into frames, written from both ends
 *  -- DMA packets from the head up, the frame's work objects (draw groups,
 *  layers, list nodes) from the tail down -- and two VRAM heaps that cache
 *  textures and CLUTs (i3d/i3dvram.h).
 *
 *  A frame: i3dCtxBeginFrame takes the frame's share of the buffer and makes
 *  the root draw group; the scene creates layers in it (i3dCtxCreateLayer)
 *  and fills them; i3dCtxEndFrame runs the root group, whose layers draw
 *  themselves through i3dCtxDrawTexturedLayer: their texture sets are
 *  reserved in VRAM run by run and each run is emitted behind the VRAM
 *  transfers it needs (i3dCtxFlushTextures). The result is one DMA chain
 *  for VIF1 (i3dCtxGetPacket).
 *
 *  Names are inferred from the code and the API words MagSystem uses; the
 *  library has no symbols and no asserts.
 * ======================================================================== */
#ifndef _I3D_I3DCTX_H
#define _I3D_I3DCTX_H

#include <stddef.h>
#include <stdint.h>

#include "i3d/i3dvram.h"

struct i3dContext;

/* The draw groups, layers and draw sets live in i3d/i3dlayer.h. */
struct I3dNode;
struct I3dGroup;
struct I3dDrawSet;
struct I3dLayer;

/* ---- DMA tags (shared by the i3d files) ---- */

#define I3D_DMA_CNT     0x10000000u     /* tag id 1: qwc qwords follow */
#define I3D_DMA_NEXT    0x20000000u     /* tag id 2: qwc qwords follow, then the tag at ADDR */
#define I3D_DMA_REF     0x30000000u     /* tag id 3: qwc qwords at ADDR */
#define I3D_DMA_CALL    0x50000000u     /* tag id 5: the chain at ADDR, back at its RET */
#define I3D_DMA_END     0x70000000u     /* tag id 7: qwc qwords follow, then stop */

/* The address word of a DMA tag. PC: the ROM's & 0x0fffffff of an EE
 * address; the host's DMA does not follow chains (i3dctx.cpp). */
static inline u_int i3dDmaAddr(const void *p)
{
    return (u_int)((uintptr_t)p & 0x0fffffff);
}

/* PC: the qwords a host-sized object takes from the packet tail. */
#define I3D_QWC_OF(T) ((int)((sizeof(T) + 15) / 16))

/* A texture table entry (i3dCtxSetTexTable; EE 0x10 each). layout: host --
 * pointers. */
struct I3dTexEntry
{
    u_int          m_00;        // ROR 0x00: unverified
    I3dVramHandle *m_tex;       // ROR 0x04: the texture's VRAM handle (or NULL)
    I3dVramHandle *m_clut;      // ROR 0x08: the CLUT's VRAM handle (or NULL)
    u_int          m_0c;        // ROR 0x0c: i3dTexRegsSet's CLUT index
};
static_assert(offsetof(I3dTexEntry, m_00) == 0, "I3dTexEntry::m_00");

/* The end-of-packet hook: writes the chain's last tags at pkt, returns the
 * position after them. */
typedef u_long *(*I3dEndFunc)(u_long *pkt, void *arg);

/* The draw context. EE 0x12168 (entries 26 and 27 of the library's size
 * table). layout: host -- pointers, and the node arrays are host-sized; the
 * EE offsets are in the comments. The ROM keeps the packet positions as EE
 * addresses with the cache-mode bits (0x30000000) set; the host keeps host
 * pointers and drops those bits (PC:). */
struct i3dContext
{
    I3dVramHeap   m_tex_vram;               // ROR 0x00: textures, in GS pages
    I3dVramHeap   m_clut_vram;              // ROR 0x70: CLUTs, in 4-block units
    I3dGroup     *m_root;                   // ROR 0xe0: this frame's root draw group
    u_int         m_frame_count;            // ROR 0xe4: frames the buffer is split in
    u_int         m_frame_index;            // ROR 0xe8: the next frame's share
    u_char       *m_buf;                    // ROR 0xec
    int           m_buf_size;               // ROR 0xf0
    u_char       *m_head;                   // ROR 0xf4: packets grow up from here
    u_char       *m_tail;                   // ROR 0xf8: work objects grow down from here
    I3dTexEntry  *m_tex_table;              // ROR 0xfc
    u_int         m_tex_table_count;        // ROR 0x100
    int           m_tex_vram_base;          // ROR 0x104: GS page
    int           m_tex_vram_size;          // ROR 0x108: pages
    int           m_clut_vram_base;         // ROR 0x10c: 4-block units
    int           m_clut_vram_size;         // ROR 0x110: 4-block units
    int           m_frames_begun;           // ROR 0x114
    u_char       *m_packet;                 // ROR 0x118: this frame's DMA chain
    I3dVramBlock  m_tex_nodes[0x200];       // ROR 0x11c
    I3dVramBlock *m_tex_free[0x200];        // ROR 0x891c
    I3dVramBlock  m_clut_nodes[0x200];      // ROR 0x911c
    I3dVramBlock *m_clut_free[0x200];       // ROR 0x1191c
    I3dDrawSet   *m_reserved;               // ROR 0x1211c: sets reserved since the last flush
    I3dEndFunc    m_end_func;               // ROR 0x12120
    void         *m_end_arg;                // ROR 0x12124
    float         m_float_param;            // ROR 0x12128: 0.35; how far i3dCtxCreateView's draw frustum goes from the screen rectangle to the guard band
    int           m_vram_overflow;          // ROR 0x1212c: a set's textures never fit; EndFrame returns 9
    int           m_frame_stat[4];          // ROR 0x12130: cleared by i3dCtxEndFrame (unverified)
    u_char        _12140[0x14];             // ROR 0x12140..0x12153: unknown
    u_long       *m_chain;                  // ROR 0x12154: the open DMA tag the next packet is linked from
    u_int         m_chan_state[4];          // ROR 0x12158: per channel, the state packet last CALLed (i3dNodeState); cleared by i3dCtxCloseFrame
};
static_assert(offsetof(i3dContext, m_tex_vram) == 0x00, "i3dContext::m_tex_vram");
static_assert(offsetof(i3dContext, m_tex_vram) + sizeof(I3dVramHeap) == offsetof(i3dContext, m_clut_vram),
              "the two heaps are adjacent");

/* i3dCtxEndFrame's result when a draw set's textures could not fit. */
#define I3D_ERR_VRAM 9
/* i3dVramReserve / i3dCtxReserveTextures: did not fit. */
#define I3D_ERR_FULL 0xf

int      i3dInitHardware(void);                                          // ROR 0x0021eb28
u_long  *i3dCtxDefaultEndFunc(u_long *pkt, void *arg);                   // ROR 0x0021ecf8
int      i3dCtxSetup(i3dContext *ctx, int vram_base, u_int vram_size);   // ROR 0x0021ed10
u_long  *i3dCtxMakeVramTransfer(i3dContext *ctx, u_long *pkt);           // ROR 0x0021ed90
int      i3dCtxKickPacket(i3dContext *ctx);                              // ROR 0x0021f028
u_long  *i3dMakeVu0MicroPacket(u_long *pkt);                             // ROR 0x0021f098
u_long  *i3dMakeVu1MicroPacket(u_long *pkt);                             // ROR 0x0021f0d8
int      i3dGetStructSize(unsigned int i);                               // ROR 0x0021f1a8
int      i3dCtxSetEndFunc(i3dContext *ctx, I3dEndFunc func, void *arg);  // ROR 0x0021f1d0
int      i3dCtxSetTexTable(i3dContext *ctx, I3dTexEntry *table, u_int count); // ROR 0x0021f1e8
void    *i3dCtxGetPacket(i3dContext *ctx);                               // ROR 0x0021f1f8
int      i3dCtxGetRest(i3dContext *ctx);                                 // ROR 0x0021f200
int      i3dCtxGetRestFrom(i3dContext *ctx, u_char *head);               // ROR 0x0021f228
int      i3dCtxGetOverrun(i3dContext *ctx);                              // ROR 0x0021f250
void    *i3dCtxAllocTail(i3dContext *ctx, int qwc);                      // ROR 0x0021f280
void     i3dCtxAllocHead(i3dContext *ctx, int qwc);                      // ROR 0x0021f2a0
u_char  *i3dCtxGetAlignedHead(i3dContext *ctx);                          // ROR 0x0021f2c0
void     i3dCtxSetAlignedHead(i3dContext *ctx, u_char *head);            // ROR 0x0021f2f0
u_char  *i3dCtxGetHead(i3dContext *ctx);                                 // ROR 0x0021f320
int      i3dCtxSetHead(i3dContext *ctx, u_char *head);                   // ROR 0x0021f328
int      i3dCtxResetVram(i3dContext *ctx);                               // ROR 0x0021f338
int      i3dCtxFlushTextures(i3dContext *ctx, I3dDrawSet *set);          // ROR 0x0021f370
int      i3dCtxDrawTexturedLayer(I3dLayer *layer);                       // ROR 0x0021f4d8
int      i3dCtxBeginFrame(i3dContext *ctx);                              // ROR 0x0021f6a0
int      i3dCtxEndFrame(i3dContext *ctx);                                // ROR 0x0021f760
int      i3dCtxSetFloatParam(i3dContext *ctx, float value);              // ROR 0x0021f7c8
I3dLayer *i3dCtxCreateLayer(i3dContext *ctx, float key);                 // ROR 0x0021f7e0
I3dGroup *i3dCtxCreateGroup(i3dContext *ctx, int sort_func, int sort_order, int sort_param);       // ROR 0x0021f850
int      i3dCtxTerm(i3dContext *ctx);                                    // ROR 0x0021f8c8
int      i3dCtxInit(i3dContext *ctx, void *buf, int size, int count);    // ROR 0x0021f900
int      i3dCtxReserveTextures(i3dContext *ctx, I3dDrawSet *set);        // ROR 0x0021f9c8

#endif /* _I3D_I3DCTX_H */
