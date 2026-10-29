/* ==========================================================================
 *  i3d/i3rblend.cpp
 *
 *  The i3r model's animated values and vertex blocks (ROR
 *  0x225018..0x226278): key curves, tracks and blend weights at a time,
 *  blend targets summed into a vertex block, the skinning of a batch's
 *  pools and the VIF unpacks of a block. See i3rblend.h.
 *
 *  The ROM skins on the scratchpad (positions at 0x70000000, normals at
 *  +0x800, the results at +0x1000 / +0x1800, DMA in and out) with three
 *  VU0 microprograms (VU0 0x15e / 0x172 / 0x186, one pair each, software
 *  pipelined). PC: the same sums over plain buffers.
 * ======================================================================== */
#include "i3d/i3rblend.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "i3d/i3dctx.h"
#include "i3d/i3rdraw.h"

/* ROR 0x00225018 */
int i3rCurveFindKey(float t, const float *keys, int num)
{
    int lo = 0;
    int hi = num - 1;
    int key;

    do
    {
        key = lo + ((hi - lo) >> 1);
        float d = t - keys[key];
        if (d < 0.0f)
        {
            hi = key - 1;
            key = hi;
        }
        else if (d > 0.0f)
        {
            lo = key + 1;
        }
        else
        {
            return key;
        }
    } while (lo <= hi);
    return key;
}

/* ROR 0x00225080 */
float i3rCurveGetValue(float t, const I3rCurveData *curve)
{
    u_int num = curve->m_num_key;

    if (num != 0)
    {
        int key = i3rCurveFindKey(t, (const float *)i3rPtr(&curve->m_keys), num);
        if (key >= 0)
        {
            if (key < (int)(num - 1))
            {
                const float *pair = (const float *)i3rPtr(&curve->m_pairs) + key * 2;
                return pair[0] * t + pair[1];
            }
            return curve->m_after;
        }
    }
    return curve->m_before;
}

/* ROR 0x00225108 */
float i3rBlendGetWeight(float t, const I3rItem *target)
{
    const I3rItem *curve = target->getChildren();

    if (curve->m_data != 0)
        return i3rCurveGetValue(t, (const I3rCurveData *)curve->getData());
    return *(const float *)target->getData();
}

/* ROR 0x00225148 */
void i3rVecCurveGetValue(float t, const I3rVecCurveData *curve, float *out)
{
    u_int num = curve->m_num_key;
    const float *v = curve->m_before;

    if (num != 0)
    {
        int key = i3rCurveFindKey(t, (const float *)i3rPtr(&curve->m_keys), num);
        if (key >= 0)
        {
            if (key < (int)(num - 1))
            {
                /* VU0: ACC = slope * t, out = ACC + intercept * vf0.w. */
                const float *pair = (const float *)i3rPtr(&curve->m_pairs) + key * 8;
                for (int i = 0; i < 4; i++)
                    out[i] = pair[i] * t + pair[4 + i];
                return;
            }
            v = curve->m_after;
        }
    }
    memcpy(out, v, sizeof(sceVu0FVECTOR));
}

/* ROR 0x002251f0 */
void i3rTrackGetValue(const I3rItem *track, float *out, float t)
{
    const I3rItem *curve = track->getChildren();

    if (curve->m_data == 0)
    {
        i3dVecCopyW(out, (const float *)track->getData());
        return;
    }
    i3rVecCurveGetValue(t, (const I3rVecCurveData *)curve->getData(), out);
}

/* ROR 0x00225230 */
void i3rBlendEvalCurves(float t, const I3rItem *block, float *out)
{
    const I3rItem &list = block->getChildren()[0];
    const I3rItem *it = list.getChildren();
    const I3rItem *end = it + list.getNumChild();

    do
    {
        i3rVecCurveGetValue(t, (const I3rVecCurveData *)it->getData(), out);
        it++;
        out += 4;
    } while (it != end);
}

/* ROR 0x002252a8 */
void i3rBlendAddCurves(float t, float w, const I3rItem *block, float *out)
{
    const I3rItem &list = block->getChildren()[0];
    const I3rItem *it = list.getChildren();
    const I3rItem *end = it + list.getNumChild();

    do
    {
        alignas(16) sceVu0FVECTOR v;
        i3rVecCurveGetValue(t, (const I3rVecCurveData *)it->getData(), v);
        out[0] += v[0] * w;
        out[1] += v[1] * w;
        out[2] += v[2] * w;
        out[3] += v[3] * w;
        it++;
        out += 4;
    } while (it != end);
}

/* ROR 0x00225378 */
void i3rQwordZero(float *buf, u_int num)
{
    /* The ROM stores four qwords a pass, then the rest. */
    memset(buf, 0, (size_t)num * sizeof(sceVu0FVECTOR));
}

/* ROR 0x002253e0 */
void i3rBlendAddVertices(float w, float *out, const float *src, u_int num)
{
    /* ROM: only the counter moves, so every output qword gets src[0] --
     * kept as is (blend targets as plain qwords: group flags 2 alone). */
    for (u_int i = 0; i < num; i++)
    {
        out[0] += src[0] * w;
        out[1] += src[1] * w;
        out[2] += src[2] * w;
        out[3] += src[3] * w;
        out += 4;
    }
}

/* ROR 0x00225460 */
void i3rVertexRefSet(I3rVertexRef *ref, const I3rItem *block)
{
    const I3rVertexBlock *data = (const I3rVertexBlock *)block->getData();

    if (data != NULL)
    {
        ref->m_data = i3rPtr(&data->m_data);
        ref->m_qwc = data->m_qwc;
        ref->m_count = data->m_count;
        ref->m_kind = data->m_kind;
        ref->m_blended = 0;
        return;
    }
    ref->m_data = NULL;
}

/* ROR 0x00225498 */
float *i3rBlendVertices(float t, I3rVertexRef *ref, float *buf, const I3rItem *blocks, u_int num_blocks,
                        const I3rBlendList *list, u_int flags)
{
    float *end;
    (void)num_blocks;

    if ((flags & 2) == 0)
    {
        const I3rVertexBlock *own = (const I3rVertexBlock *)blocks[0].getData();

        if ((flags & 1) == 0 || blocks[0].getChildren()[0].getNumChild() == 0)
        {
            ref->m_data = i3rPtr(&own->m_data);
            ref->m_qwc = own->m_qwc;
            ref->m_count = own->m_count;
            ref->m_kind = own->m_kind;
            ref->m_blended = 0;
            end = buf;
        }
        else
        {
            u_int count = own->m_count;
            i3rBlendEvalCurves(t, &blocks[0], buf);
            ref->m_data = buf;
            ref->m_blended = 1;
            ref->m_count = count;
            ref->m_qwc = count;
            ref->m_kind = 0;
            end = buf + count * 4;
        }
    }
    else
    {
        u_int count = ((const I3rVertexBlock *)blocks[0].getData())->m_count;
        ref->m_data = buf;
        ref->m_blended = 1;
        end = buf + count * 4;
        ref->m_qwc = count;
        ref->m_count = count;
        ref->m_kind = 0;
        i3rQwordZero(buf, count);

        /* PC: the ROM's loop takes entry 0 before testing the count (its
         * caller has a non-zero weight there); an empty list stays zero,
         * and the host list holds 8 (i3rDrawSetupBlend counts past it). */
        int num = list->m_num < 8 ? list->m_num : 8;
        for (int i = 0; i < num; i++)
        {
            const I3rBlendEntry &e = list->m_entry[i];
            const I3rItem *target = &blocks[e.m_index];

            if ((flags & 1) == 0)
            {
                const I3rVertexBlock *data = (const I3rVertexBlock *)target->getData();
                i3rBlendAddVertices(e.m_weight, buf, (const float *)i3rPtr(&data->m_data), count);
            }
            else
            {
                i3rBlendAddCurves(t, e.m_weight, target, buf);
            }
        }
    }
    i3dFlushCache();
    return end;
}

/* ROR 0x00225638 */
float *i3rVertexRefAlloc(I3rVertexRef *ref, float *buf)
{
    if (ref->m_blended == 0)
    {
        ref->m_data = buf;
        ref->m_kind = 0;
        ref->m_blended = 1;
        buf += ref->m_count * 4;
        ref->m_qwc = ref->m_count;
    }
    return buf;
}

/* out = a x b, row vectors (VU0: rows of a weight b's rows). */
static void i3rSkinMatMul(sceVu0FVECTOR *out, const sceVu0FVECTOR *a, const sceVu0FVECTOR *b)
{
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            out[r][c] = a[r][0] * b[0][c] + a[r][1] * b[1][c] + a[r][2] * b[2][c] + a[r][3] * b[3][c];
}

/* Rows 0..2 of m with xyz over their length (VU0 RSQRT); w from keep. */
static void i3rSkinNormalRows(sceVu0FVECTOR *out, const sceVu0FVECTOR *m, const sceVu0FVECTOR *keep)
{
    for (int r = 0; r < 3; r++)
    {
        float q = 1.0f / sqrtf(m[r][0] * m[r][0] + m[r][1] * m[r][1] + m[r][2] * m[r][2]);
        out[r][0] = m[r][0] * q;
        out[r][1] = m[r][1] * q;
        out[r][2] = m[r][2] * q;
        out[r][3] = keep[r][3];
    }
}

/* One bone record's pairs (the microprograms): pos_out += M x (w p),
 * all four rows; nrm_out += N x (w n), rows 0..2. Pools are byte offsets
 * into 0x800-byte areas. */
static void i3rSkinRecord(const I3rBoneRecord *br, const sceVu0FVECTOR *m, const sceVu0FVECTOR *n,
                          const u_char *pos_in, const u_char *nrm_in, u_char *pos_out, u_char *nrm_out)
{
    const u_char *pair = (const u_char *)i3rPtr(&br->m_pairs);
    u_int count = (u_char)br->m_count;

    for (u_int i = 0; i < count; i++, pair += 8)
    {
        int off;
        float w;
        memcpy(&off, pair, 4);
        memcpy(&w, pair + 4, 4);

        const float *p = (const float *)(pos_in + off);
        const float *v = (const float *)(nrm_in + off);
        float *po = (float *)(pos_out + off);
        float *no = (float *)(nrm_out + off);
        float wp[4] = {p[0] * w, p[1] * w, p[2] * w, p[3] * w};
        float wn[4] = {v[0] * w, v[1] * w, v[2] * w, v[3] * w};
        for (int c = 0; c < 4; c++)
        {
            po[c] += m[0][c] * wp[0] + m[1][c] * wp[1] + m[2][c] * wp[2] + m[3][c] * wp[3];
            no[c] += n[0][c] * wn[0] + n[1][c] * wn[1] + n[2][c] * wn[2];
        }
    }
}

/* The scratchpad pools of a skin pass (PC: 0x800 bytes each, as the
 * ROM's). */
alignas(16) static u_char s_skin_pool[4][0x800];

static float *i3rSkinFinish(I3rVertexRef *pos, I3rVertexRef *nrm, float *buf, u_int qwc)
{
    float *end = (float *)(((uintptr_t)buf + 0x7f) & ~(uintptr_t)0x7f);

    end = i3rVertexRefAlloc(pos, end);
    end = i3rVertexRefAlloc(nrm, end);
    /* The ROM DMAs the results back from the scratchpad. */
    memcpy((void *)pos->m_data, s_skin_pool[2], (size_t)qwc * 0x10);
    memcpy((void *)nrm->m_data, s_skin_pool[3], (size_t)qwc * 0x10);
    /* The ROM returns the end aligned, in the uncached-accelerated
     * segment (| 0x30000000). */
    return (float *)(((uintptr_t)end + 0x7f) & ~(uintptr_t)0x7f);
}

static void i3rSkinStart(const I3rVertexRef *pos, const I3rVertexRef *nrm, u_int qwc)
{
    memcpy(s_skin_pool[0], pos->m_data, (size_t)qwc * 0x10);
    i3rQwordZero((float *)s_skin_pool[2], qwc);
    memcpy(s_skin_pool[1], nrm->m_data, (size_t)qwc * 0x10);
    i3rQwordZero((float *)s_skin_pool[3], qwc);
}

/* ROR 0x00225670 */
float *i3rSkinVertices(I3rVertexRef *pos, I3rVertexRef *nrm, float *buf, const I3rItem *rec,
                       const I3rItem *rec_end, const sceVu0FVECTOR *nodes, const sceVu0FVECTOR *offsets,
                       const u_short *bones)
{
    u_int qwc = pos->m_count;
    // callcheck: i3rQwordZero, i3rVertexRefAlloc (in i3rSkinStart / i3rSkinFinish, shared with i3rSkinVerticesPre)

    /* VU0 keeps vf16..vf23 in its data memory over the pass (vsqi / vlqi). */
    i3rSkinStart(pos, nrm, qwc);
    do
    {
        const I3rBoneRecord *br = (const I3rBoneRecord *)rec->getData();
        const sceVu0FVECTOR *node = &nodes[bones[br->m_slot] * 4];
        alignas(16) sceVu0FVECTOR m[4];
        alignas(16) sceVu0FVECTOR n[3];

        i3rSkinMatMul(m, &offsets[br->m_slot * 4], node);
        i3rSkinNormalRows(n, m, node);
        i3rSkinRecord(br, m, n, s_skin_pool[0], s_skin_pool[1], s_skin_pool[2], s_skin_pool[3]);
        rec++;
    } while (rec != rec_end);
    return i3rSkinFinish(pos, nrm, buf, qwc);
}

/* ROR 0x00225af0 */
float *i3rSkinVerticesPre(I3rVertexRef *pos, I3rVertexRef *nrm, float *buf, const I3rItem *rec,
                          const I3rItem *rec_end, const sceVu0FVECTOR *mats, const sceVu0FVECTOR *nmats)
{
    u_int qwc = pos->m_count;
    // callcheck: i3rQwordZero, i3rVertexRefAlloc (in i3rSkinStart / i3rSkinFinish, shared with i3rSkinVerticesPre)

    i3rSkinStart(pos, nrm, qwc);
    do
    {
        const I3rBoneRecord *br = (const I3rBoneRecord *)rec->getData();

        i3rSkinRecord(br, &mats[br->m_slot * 4], &nmats[br->m_slot * 4], s_skin_pool[0], s_skin_pool[1],
                      s_skin_pool[2], s_skin_pool[3]);
        rec++;
    } while (rec != rec_end);
    return i3rSkinFinish(pos, nrm, buf, qwc);
}

/* ROR 0x00225ec8 */
void i3rMakeSkinMatrices(sceVu0FVECTOR *mats, sceVu0FVECTOR *nmats, const sceVu0FVECTOR *nodes,
                         const sceVu0FVECTOR *offsets, const u_short *bones, u_int num)
{
    for (u_int i = 0; i < num; i++)
    {
        alignas(16) sceVu0FVECTOR m[4];

        i3rSkinMatMul(m, &offsets[i * 4], &nodes[bones[i] * 4]);
        memcpy(&mats[i * 4], m, sizeof(m));
        /* The normal rows keep the offset rows' w (vmulq.xyz over them). */
        i3rSkinNormalRows(&nmats[i * 4], m, &offsets[i * 4]);
    }
}

/* ROR 0x00225fd8 */
void i3rVertexRefAverage(const I3rVertexRef *ref, float *out)
{
    const float *q = (const float *)ref->m_data;
    const float *end = q + ref->m_count * 4;
    float sum[4] = {0.0f, 0.0f, 0.0f, 1.0f};    // vf0

    do
    {
        sum[0] += q[0];
        sum[1] += q[1];
        sum[2] += q[2];
        q += 4;
    } while (q != end);
    float r = 1.0f / (float)ref->m_count;
    out[0] = sum[0] * r;
    out[1] = sum[1] * r;
    out[2] = sum[2] * r;
    out[3] = sum[3];
}

/* ROR 0x00226060 */
void i3rVertexRefRebase(I3rVertexRef *ref, u_int base, int tops)
{
    u_int *p = (u_int *)ref->m_data;

    if (p == NULL || ref->m_kind == 0 || ref->m_kind != 1)
        return;

    u_int *end = p + ref->m_qwc * 4;
    u_int cl = 1;
    bool first = true;

    while (p < end)
    {
        u_int code = *p;

        if (((code >> 24) & 0x60) == 0x60)
        {
            /* UNPACK: the first goes to base, the rest move by it. */
            if (first)
            {
                first = false;
                code = (code & 0xffff4000) | base;
            }
            else
            {
                code = (code & 0xffff7fff) + base;
            }
            *p = code;
            if (tops)
                *p |= 0x8000;
            if (cl == 0)
            {
                p++;
                continue;
            }

            u_int num = (code >> 16) & 0xff;
            u_int size;
            switch ((code >> 24) & 0xf)
            {
            case 0x0:                   // S-32
            case 0x5:                   // V2-16
                size = num * 4;
                break;
            case 0x4:                   // V2-32
                size = num * 8;
                break;
            case 0x8:                   // V3-32
                size = num * 12;
                break;
            case 0xc:                   // V4-32
                size = num * 16;
                break;
            default:
                /* PC: the ROM does not move on here and loops for ever. */
                return;
            }
            p = (u_int *)((u_char *)p + size + 4);
        }
        else if ((code >> 24) == 0x01)  // STCYCL
        {
            cl = code & 0xff;
            p++;
        }
        else if (code == 0x20000000)    // STMASK
        {
            p += 2;
        }
        else if (code == 0x30000000)    // STROW
        {
            p += 5;
        }
        else if (code == 0)             // NOP
        {
            p++;
        }
        else
        {
            /* PC: the ROM does not move on here and loops for ever. */
            return;
        }
    }
}

/* ROR 0x002261b8 */
u_long *i3rVertexRefWriteUnpack(const I3rVertexRef *ref, u_long *pkt, u_int addr, int tops)
{
    if (ref->m_data == NULL)
        return pkt;

    /* A DMA REF tag (id 3) to the data, then the VIF words. */
    u_long tag = ((u_long)(i3dDmaAddr(ref->m_data) & 0xfffffff) << 32) | 0x30000000u | (u_short)ref->m_qwc;
    if (ref->m_kind == 0)
    {
        u_int unpack = (tops ? 0x6c008000u : 0x6c000000u) | (ref->m_qwc << 16) | addr;
        pkt[0] = tag;
        pkt[1] = ((u_long)unpack << 32) | 0x01000101u;     // STCYCL 1,1; UNPACK V4-32
        return pkt + 2;
    }
    if (ref->m_kind == 1)
    {
        pkt[0] = tag;
        pkt[1] = 0x01000101u;                               // STCYCL 1,1; NOP
        return pkt + 2;
    }
    return pkt;
}
