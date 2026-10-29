/* ==========================================================================
 *  i3d/i3rblend.h
 *
 *  The i3r model's animated values and vertex blocks (i3d/i3rblend.cpp,
 *  ROR 0x225018..0x226278): the key curves (a scalar type 0x58 and a
 *  vector type 0x34 one), tracks and blend weights read at a time, the
 *  blend targets summed into a vertex block, the bone-weighted skinning of
 *  a batch's position / normal pools (VU0 on the scratchpad in the ROM),
 *  and the VIF unpacks that send a block to VU1.
 *
 *  A vertex ref (I3rVertexRef) is what the mesh writers pass around for
 *  one block: the file's 0x33 block itself, or qwords computed into the
 *  draw buffer. The host mesh draw (rosepan_i3dmesh.cpp) calls the blend
 *  functions; the skin and unpack writers have their host counterpart
 *  there (SkinBatch, the block rebase), so nothing calls them on the PC.
 * ======================================================================== */
#ifndef _I3D_I3RBLEND_H
#define _I3D_I3RBLEND_H

#include <stddef.h>

#include "i3d/i3dmath.h"
#include "i3d/i3rroot.h"

struct I3rBlendList;

/* A scalar key curve (type 0x58 item data). From the file: plain EE
 * layout. Value at t: m_before before the first key, m_after from the
 * last, else slope * t + intercept of the key's pair. */
struct I3rCurveData
{
    float   m_after;            // ROR 0x00: the value from the last key on
    float   m_before;           // ROR 0x04: the value before the first key
    u_int   m_08;               // ROR 0x08: not read here
    u_int   m_pairs;            // ROR 0x0c: relocated: {slope, intercept} f32 per key
    u_int   m_keys;             // ROR 0x10: relocated: the f32 key times, ascending
    u_int   m_14;               // ROR 0x14: not read here
    u_short m_num_key;          // ROR 0x18
};
static_assert(offsetof(I3rCurveData, m_before) == 0x04, "I3rCurveData::m_before");
static_assert(offsetof(I3rCurveData, m_pairs) == 0x0c, "I3rCurveData::m_pairs");
static_assert(offsetof(I3rCurveData, m_keys) == 0x10, "I3rCurveData::m_keys");
static_assert(offsetof(I3rCurveData, m_num_key) == 0x18, "I3rCurveData::m_num_key");
static_assert(sizeof(I3rCurveData) == 0x1c, "I3rCurveData: the 0x1c bytes known");

/* A vector key curve (type 0x34 item data): the same with qwords. From
 * the file: plain EE layout. */
struct I3rVecCurveData
{
    float   m_after[4];         // ROR 0x00
    float   m_before[4];        // ROR 0x10
    u_int   m_20;               // ROR 0x20: not read here
    u_int   m_pairs;            // ROR 0x24: relocated: {slope, intercept} qword pairs per key
    u_int   m_keys;             // ROR 0x28: relocated: the f32 key times
    u_int   m_2c;               // ROR 0x2c: not read here
    u_short m_num_key;          // ROR 0x30
};
static_assert(offsetof(I3rVecCurveData, m_before) == 0x10, "I3rVecCurveData::m_before");
static_assert(offsetof(I3rVecCurveData, m_pairs) == 0x24, "I3rVecCurveData::m_pairs");
static_assert(offsetof(I3rVecCurveData, m_keys) == 0x28, "I3rVecCurveData::m_keys");
static_assert(offsetof(I3rVecCurveData, m_num_key) == 0x30, "I3rVecCurveData::m_num_key");
static_assert(sizeof(I3rVecCurveData) == 0x34, "I3rVecCurveData: the 0x34 bytes known");

/* A vertex block (type 0x33 item data). From the file: plain EE layout. */
struct I3rVertexBlock
{
    u_int   m_data;             // ROR 0x00: relocated: the qwords (kind 0) or a VIF stream (kind 1)
    u_short m_qwc;              // ROR 0x04
    u_short m_count;            // ROR 0x06: the vertices
    u_char  m_kind;             // ROR 0x08: 0 V4-32 qwords, 1 a VIF stream
};
static_assert(offsetof(I3rVertexBlock, m_qwc) == 0x04, "I3rVertexBlock::m_qwc");
static_assert(offsetof(I3rVertexBlock, m_count) == 0x06, "I3rVertexBlock::m_count");
static_assert(offsetof(I3rVertexBlock, m_kind) == 0x08, "I3rVertexBlock::m_kind");
static_assert(sizeof(I3rVertexBlock) == 0x0c, "I3rVertexBlock is 0x0c bytes on the EE");

/* A skin bone record (type 0x31 item data). From the file: plain EE
 * layout. The pairs are {s32 pool byte offset, f32 weight}, padded with
 * zero weights; the skin writers read m_count's low byte. */
struct I3rBoneRecord
{
    u_int   m_pairs;            // ROR 0x00: relocated
    u_short m_slot;             // ROR 0x04: the bone slot (the entry's bone table, the offsets)
    u_short m_count;            // ROR 0x06
};
static_assert(offsetof(I3rBoneRecord, m_slot) == 0x04, "I3rBoneRecord::m_slot");
static_assert(offsetof(I3rBoneRecord, m_count) == 0x06, "I3rBoneRecord::m_count");
static_assert(sizeof(I3rBoneRecord) == 0x08, "I3rBoneRecord is 0x08 bytes on the EE");

/* The block VU1 gets for one slot (EE: five words). layout: host --
 * m_data is a pointer. */
struct I3rVertexRef
{
    const void *m_data;         // ROR 0x00: the qwords (or the VIF stream, m_kind 1)
    u_int   m_qwc;              // ROR 0x04
    u_int   m_count;            // ROR 0x08: the vertices
    u_int   m_kind;             // ROR 0x0c: the 0x33 block's kind
    u_int   m_blended;          // ROR 0x10: 1 = computed into the draw buffer (V4-32 qwords)
};

/* ROR 0x00225018: the last of num ascending keys at or before t; -1
 * before the first. num must not be 0. */
int    i3rCurveFindKey(float t, const float *keys, int num);            // ROR 0x00225018
/* ROR 0x00225080: a scalar curve at t. */
float  i3rCurveGetValue(float t, const I3rCurveData *curve);            // ROR 0x00225080
/* ROR 0x00225108: a blend target's weight at t: its child 0's curve
 * when that child has data, else the target's own f32. */
float  i3rBlendGetWeight(float t, const I3rItem *target);               // ROR 0x00225108
/* ROR 0x00225148: a vector curve at t into out (a qword). */
void   i3rVecCurveGetValue(float t, const I3rVecCurveData *curve, float *out); // ROR 0x00225148
/* ROR 0x002251f0: an i3r item's vector at time t: its data when its child
 * 0 has no data, else child 0's vector curve at t. */
void   i3rTrackGetValue(const I3rItem *track, float *out, float t);     // ROR 0x002251f0
/* ROR 0x00225230: one qword per child of block's child 0: that child's
 * vector curve at t. */
void   i3rBlendEvalCurves(float t, const I3rItem *block, float *out);   // ROR 0x00225230
/* ROR 0x002252a8: adds w x the same curves into out. */
void   i3rBlendAddCurves(float t, float w, const I3rItem *block, float *out); // ROR 0x002252a8
/* ROR 0x00225378: zeroes num qwords. */
void   i3rQwordZero(float *buf, u_int num);                             // ROR 0x00225378
/* ROR 0x002253e0: out[i] += w x src[0] for num qwords -- the ROM never
 * advances src (see i3rblend.cpp). */
void   i3rBlendAddVertices(float w, float *out, const float *src, u_int num); // ROR 0x002253e0
/* ROR 0x00225460: ref = block's 0x33 data, not blended; m_data NULL when
 * the block has none. */
void   i3rVertexRefSet(I3rVertexRef *ref, const I3rItem *block);        // ROR 0x00225460
/* ROR 0x00225498: one blended slot: blocks[0] the mesh's own, [k] blend
 * target k. Group flags & 2: buf = the sum of list's weights x their
 * targets (flags & 1: each target's per-vertex curves at t); flags & 1
 * alone: buf = the own block's curves at t; else ref = the own block.
 * Returns the end of what it wrote into buf. PC: the list in place of the
 * ROM's entries and count. */
float *i3rBlendVertices(float t, I3rVertexRef *ref, float *buf, const I3rItem *blocks, u_int num_blocks,
                        const I3rBlendList *list, u_int flags);         // ROR 0x00225498
/* ROR 0x00225638: a ref not computed yet gets buf (its count qwords, kind
 * 0, blended); returns the end of its space, buf when already blended. */
float *i3rVertexRefAlloc(I3rVertexRef *ref, float *buf);                // ROR 0x00225638
/* ROR 0x00225670: skins a batch's pools: for every bone record in
 * [rec, rec_end) the matrix offsets[slot] x nodes[bones[slot]], and per
 * pair pos += w x (p x M), nrm += w x (n x M's rows 0..2 normalised), from
 * zero. The pools land in buf (128-byte aligned; refs not yet blended
 * point there); returns the end, aligned. */
float *i3rSkinVertices(I3rVertexRef *pos, I3rVertexRef *nrm, float *buf, const I3rItem *rec,
                       const I3rItem *rec_end, const sceVu0FVECTOR *nodes, const sceVu0FVECTOR *offsets,
                       const u_short *bones);                           // ROR 0x00225670
/* ROR 0x00225af0: the same with the slot matrices made beforehand
 * (i3rMakeSkinMatrices). */
float *i3rSkinVerticesPre(I3rVertexRef *pos, I3rVertexRef *nrm, float *buf, const I3rItem *rec,
                          const I3rItem *rec_end, const sceVu0FVECTOR *mats,
                          const sceVu0FVECTOR *nmats);                  // ROR 0x00225af0
/* ROR 0x00225ec8: per slot i < num, mats[i] = offsets[i] x nodes[bones[i]]
 * and nmats[i]'s rows 0..2 = its rows with xyz normalised. */
void   i3rMakeSkinMatrices(sceVu0FVECTOR *mats, sceVu0FVECTOR *nmats, const sceVu0FVECTOR *nodes,
                           const sceVu0FVECTOR *offsets, const u_short *bones, u_int num); // ROR 0x00225ec8
/* ROR 0x00225fd8: the mean xyz of the ref's m_count qwords, w 1. */
void   i3rVertexRefAverage(const I3rVertexRef *ref, float *out);       // ROR 0x00225fd8
/* ROR 0x00226060: a kind 1 ref's VIF stream, in place: the first UNPACK
 * goes to base, the others move by base; tops sets their FLG bit. */
void   i3rVertexRefRebase(I3rVertexRef *ref, u_int base, int tops);     // ROR 0x00226060
/* ROR 0x002261b8: a DMA REF tag to the ref's data with STCYCL 1,1 and,
 * kind 0, an UNPACK V4-32 to addr (tops: TOPS-relative). Returns the
 * packet end; nothing without data. */
u_long *i3rVertexRefWriteUnpack(const I3rVertexRef *ref, u_long *pkt, u_int addr, int tops); // ROR 0x002261b8

#endif /* _I3D_I3RBLEND_H */
