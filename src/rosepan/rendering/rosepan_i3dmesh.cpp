/* ==========================================================================
 *  rosepan_i3dmesh.cpp -- the native i3d module's 3D mesh path (see
 *  rosepan_i3dmesh.h for the model tree it walks).
 *
 *  The ROM builds VIF chains of the mesh blocks (i3rBuildMeshPacket
 *  0x21fb50) and VU1 microprogram 1 (data 0x2912b0) turns each strip into
 *  GS triangles.  Here the blocks are unpacked into a host copy of VU1
 *  memory exactly as VIF1 would write them, and each strip is read back
 *  from there: the index list picks positions and normals from the batch's
 *  pools, colours and STQ come per strip vertex.  What microprogram 1 does
 *  beyond the transform -- its lighting, fog and clipping -- is not
 *  reproduced: the colours are the strip's own, lit by one fixed direction
 *  (open-items.md "Mesh draw").
 * ======================================================================== */
#include "rosepan/rendering/rosepan_i3dmesh.h"

#include <math.h>
#include <string.h>

#include "i3d/i3dctx.h"
#include "i3d/i3dtex.h"
#include "i3d/i3rroot.h"
#include "rosepan/io/rosepan_log.h"
#include "rosepan/rendering/rosepan_renderer.h"

namespace
{
/* Where i3rBuildMeshPacket unpacks each block (VU1 qword addresses; the
 * strip ones relative to TOPS, which is 0 here). */
constexpr unsigned kVuPositions = 0x1dc;
constexpr unsigned kVuNormals = 0x22c;
constexpr unsigned kVuColours = 0x017;
constexpr unsigned kVuStq = 0x02b;
constexpr unsigned kVuSize = 1024;

/* Item types the walk checks. */
constexpr unsigned kTypeVertexBatch = 0x56;
constexpr unsigned kTypeStrip = 0x47;

/* A 0x33 block's data (EE layout, from the file). */
struct Block
{
    u_int   m_data;             // ROR 0x00: relocated: the block
    u_short m_qwc;              // ROR 0x04
    u_short m_count;            // ROR 0x06: the vertices it is for
    u_char  m_kind;             // ROR 0x08: 0 V4-32 qwords, 1 a VIF stream
};
static_assert(offsetof(Block, m_kind) == 0x08, "Block::m_kind");

/* A 0x47 strip's data. */
struct Strip
{
    u_int   m_list;             // ROR 0x00: relocated: the V2-16 index list
    u_char  m_qwc;              // ROR 0x04
    u_char  m_count;            // ROR 0x05: strip vertices (3..20)
};
static_assert(offsetof(Strip, m_count) == 0x05, "Strip::m_count");

/* A 0x59 node entry's data. */
struct NodeEntry
{
    u_int   m_bones;            // ROR 0x00: relocated: u16 node indices
    short   m_mesh;             // ROR 0x04: into root [2]
    u_short m_num_bone;         // ROR 0x06
    u_char  m_enable;           // ROR 0x08
};
static_assert(offsetof(NodeEntry, m_enable) == 0x08, "NodeEntry::m_enable");

/* A 0x31 bone record's data (a skinned batch's [5] children). */
struct BoneRecord
{
    u_int   m_pairs;            // ROR 0x00: relocated: {pool byte offset, weight} pairs
    u_short m_slot;             // ROR 0x04: the bone slot (the entry's bone table, the offsets)
    u_short m_count;            // ROR 0x06: pairs (padded with zero weights)
};
static_assert(offsetof(BoneRecord, m_count) == 0x06, "BoneRecord::m_count");

/* What a skinned batch blends with: the matrix of bone slot i is
 * offsets[i] * nodes[bones[i]] (0x225ec8 / 0x225670's product). */
struct Skin
{
    const float   *nodes;
    const u_short *bones;
    unsigned       num_bone;
    unsigned       num_node;
    const float   *offsets;
};

/* The host copy of VU1 data memory and the VIF1 registers a block uses. */
struct HostVu
{
    uint32_t mem[kVuSize][4];
    uint32_t row[4];
    uint32_t col[4];
    unsigned cl, wl;
    uint32_t mask;
};

HostVu s_vu;
bool s_logged_vif;
bool s_logged_kind;

float AsFloat(uint32_t bits)
{
    float f;
    memcpy(&f, &bits, sizeof(f));
    return f;
}

/* One UNPACK: `num` qwords from `data` to `addr`, through the cycle and the
 * mask as VIF1 writes them (VIF mode 0: no offset / difference).  Returns
 * the bytes of data it read. */
size_t Unpack(const u_char *data, unsigned cmd, unsigned num, unsigned addr, bool usn)
{
    HostVu &vu = s_vu;
    const unsigned vn = (cmd >> 2) & 3, vl = cmd & 3;
    const bool masked = (cmd & 0x10) != 0;
    const unsigned comps = vn + 1;
    const unsigned csize = vl == 0 ? 4 : vl == 1 ? 2 : 1;
    const unsigned esize = vl == 3 ? 2 : comps * csize;
    size_t used = 0;
    for (unsigned i = 0; i < num; i++)
    {
        unsigned dst, pos;
        bool from_data;
        if (vu.wl <= vu.cl)
        {
            /* Skipping (or plain) write: every element is data. */
            dst = addr + (i / vu.wl) * vu.cl + i % vu.wl;
            pos = i % vu.wl;
            from_data = true;
        }
        else
        {
            /* Filling write: CL of every WL qwords are data, the rest fill. */
            dst = addr + i;
            pos = i % vu.wl;
            from_data = pos < vu.cl;
        }
        uint32_t v[4] = {0, 0, 0, 0};
        if (from_data)
        {
            const u_char *e = data + used * esize;
            for (unsigned k = 0; k < comps && vl != 3; k++)
            {
                if (vl == 0)
                    memcpy(&v[k], e + k * 4, 4);
                else if (vl == 1)
                {
                    uint16_t h;
                    memcpy(&h, e + k * 2, 2);
                    v[k] = usn ? (uint32_t)h : (uint32_t)(int32_t)(int16_t)h;
                }
                else
                    v[k] = usn ? (uint32_t)e[k] : (uint32_t)(int32_t)(int8_t)e[k];
            }
            if (comps == 1)
                v[1] = v[2] = v[3] = v[0];
            used++;
        }
        uint32_t *q = vu.mem[dst % kVuSize];
        const unsigned r = pos < 3 ? pos : 3;
        for (unsigned f = 0; f < 4; f++)
        {
            const unsigned m = masked ? (vu.mask >> (r * 8 + f * 2)) & 3 : 0;
            if (m == 0)
            {
                if (from_data)
                    q[f] = v[f];
            }
            else if (m == 1)
                q[f] = vu.row[f];
            else if (m == 2)
                q[f] = vu.col[r];
        }
    }
    return (used * esize + 3) & ~(size_t)3;
}

/* A VIF stream of qwc qwords, its unpack addresses rebased onto `base` the
 * way i3rBuildMeshPacket's 0x226060 patches them (the first unpack's
 * address becomes base, the others move with it).  Taken relative to the
 * first unpack's own address, so a stream already patched in place reads
 * the same. */
void RunVif(const u_char *p, unsigned qwc, unsigned base)
{
    HostVu &vu = s_vu;
    const u_char *end = p + (size_t)qwc * 16;
    bool first = true;
    unsigned first_addr = 0;
    vu.cl = vu.wl = 1;                          /* the REF tag's STCYCL 1, 1 */
    while (p + 4 <= end)
    {
        uint32_t code;
        memcpy(&code, p, 4);
        const unsigned cmd = code >> 24;
        if ((cmd & 0x60) == 0x60)
        {
            unsigned num = (code >> 16) & 0xff;
            if (num == 0)
                num = 256;
            const unsigned addr = code & 0x3ff;
            if (first)
            {
                first_addr = addr;
                first = false;
            }
            p += 4;
            p += Unpack(p, cmd, num, base + addr - first_addr, (code & 0x4000) != 0);
            continue;
        }
        switch (cmd)
        {
        case 0x00: p += 4; break;               /* NOP */
        case 0x01: vu.cl = code & 0xff; vu.wl = (code >> 8) & 0xff; p += 4; break;
        case 0x20: memcpy(&vu.mask, p + 4, 4); p += 8; break;
        case 0x30: memcpy(vu.row, p + 4, 16); p += 20; break;
        case 0x31: memcpy(vu.col, p + 4, 16); p += 20; break;
        default:
            if (!s_logged_vif)
            {
                RosePan_LogPrintf("rosepan_i3dmesh: VIF code %08x in a mesh block not handled\n", code);
                s_logged_vif = true;
            }
            return;
        }
    }
}

/* A slot holding one 0x33 block (an 03 list of one): unpack it at base. */
void LoadSlot(const I3rItem &slot, unsigned base)
{
    if (slot.getNumChild() == 0)
        return;
    const Block *b = (const Block *)slot.getChildren()[0].getData();
    if (b == nullptr)
        return;
    const u_char *data = (const u_char *)i3rPtr(&b->m_data);
    if (data == nullptr)
        return;
    if (b->m_kind == 0)
    {
        for (unsigned i = 0; i < b->m_qwc; i++)
            memcpy(s_vu.mem[(base + i) % kVuSize], data + i * 16, 16);
    }
    else if (b->m_kind == 1)
        RunVif(data, b->m_qwc, base);
    else if (!s_logged_kind)
    {
        RosePan_LogPrintf("rosepan_i3dmesh: mesh block kind %u not handled\n", b->m_kind);
        s_logged_kind = true;
    }
}

void VuVec3(unsigned addr, float out[3])
{
    const uint32_t *q = s_vu.mem[addr % kVuSize];
    out[0] = AsFloat(q[0]);
    out[1] = AsFloat(q[1]);
    out[2] = AsFloat(q[2]);
}

/* out = a * b, row-major (p * a * b). */
void MatMul(float *out, const float *a, const float *b)
{
    float r[16];
    for (int i = 0; i < 4; i++)
        for (int k = 0; k < 4; k++)
            r[i * 4 + k] = a[i * 4 + 0] * b[0 * 4 + k] + a[i * 4 + 1] * b[1 * 4 + k] +
                           a[i * 4 + 2] * b[2 * 4 + k] + a[i * 4 + 3] * b[3 * 4 + k];
    memcpy(out, r, sizeof(r));
}

/* A skinned batch's pools (0x225670, on VU0 in the ROM): every vertex the
 * bone records name becomes the weighted sum of its position (w = 1) and
 * normal through each record's slot matrix -- the normal through that
 * matrix's rows normalised.  The results replace the pools. */
void SkinBatch(const I3rItem &records, const Skin &skin)
{
    float acc_p[80][3] = {}, acc_n[80][3] = {};
    bool touched[80] = {};
    const I3rItem *rec = records.getChildren();
    for (u_int r = 0; r < records.getNumChild(); r++)
    {
        const BoneRecord *br = (const BoneRecord *)rec[r].getData();
        if (br == nullptr || br->m_slot >= skin.num_bone || skin.bones[br->m_slot] >= skin.num_node)
            continue;
        const u_char *pairs = (const u_char *)i3rPtr(&br->m_pairs);
        if (pairs == nullptr)
            continue;
        float m[16];
        MatMul(m, skin.offsets + (size_t)br->m_slot * 16, skin.nodes + (size_t)skin.bones[br->m_slot] * 16);
        float rot[9];
        for (int row = 0; row < 3; row++)
        {
            const float *v = &m[row * 4];
            const float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
            for (int c = 0; c < 3; c++)
                rot[row * 3 + c] = len > 0.0f ? v[c] / len : 0.0f;
        }
        for (u_int i = 0; i < br->m_count; i++)
        {
            int32_t off;
            float w;
            memcpy(&off, pairs + i * 8, 4);
            memcpy(&w, pairs + i * 8 + 4, 4);
            const int k = off / 16;
            if (w == 0.0f || k < 0 || k >= 80)
                continue;
            float p[3], n[3];
            VuVec3(kVuPositions + k, p);
            VuVec3(kVuNormals + k, n);
            for (int c = 0; c < 3; c++)
            {
                acc_p[k][c] += w * (p[0] * m[c] + p[1] * m[4 + c] + p[2] * m[8 + c] + m[12 + c]);
                acc_n[k][c] += w * (n[0] * rot[c] + n[1] * rot[3 + c] + n[2] * rot[6 + c]);
            }
            touched[k] = true;
        }
    }
    for (int k = 0; k < 80; k++)
    {
        if (!touched[k])
            continue;
        memcpy(s_vu.mem[kVuPositions + k], acc_p[k], sizeof(acc_p[k]));
        memcpy(s_vu.mem[kVuNormals + k], acc_n[k], sizeof(acc_n[k]));
    }
}

/* The layer texture of material `index` (root [0]): its first layer's
 * m_tex_index into the texture table. */
const void *MaterialTexture(const I3rItem *materials, unsigned num_mat, int index,
                            const I3dTexEntry *tex, int tex_count)
{
    if (materials == nullptr || index < 0 || (unsigned)index >= num_mat || tex == nullptr)
        return nullptr;
    const I3rItem &mat = materials[index];
    if (mat.getNumChild() == 0)
        return nullptr;
    const I3rItem &layers = mat.getChildren()[0];
    if (layers.getNumChild() == 0)
        return nullptr;
    const u_char *layer = (const u_char *)layers.getChildren()[0].getData();
    if (layer == nullptr)
        return nullptr;
    int tex_index;
    memcpy(&tex_index, layer + 0x1c, sizeof(tex_index));   /* I3rLayerData::m_tex_index */
    if (tex_index < 0 || tex_index >= tex_count || tex[tex_index].pPicture == nullptr)
        return nullptr;
    const I3dTexEntry &e = tex[tex_index];
    return RosePan_I3dTextureFor(e.pPicture, e.m_clut, e.m_0c, 1);
}

/* A fixed light for the host colours until the scene lights are read:
 * from above and in front, in world space. */
float Shade(const float *m, const float n[3])
{
    const float wx = n[0] * m[0] + n[1] * m[4] + n[2] * m[8];
    const float wy = n[0] * m[1] + n[1] * m[5] + n[2] * m[9];
    const float wz = n[0] * m[2] + n[1] * m[6] + n[2] * m[10];
    const float len = sqrtf(wx * wx + wy * wy + wz * wz);
    if (!(len > 0.0f))
        return 1.0f;
    const float lx = 0.30f, ly = -0.80f, lz = -0.52f;      /* y is down in RoR's world */
    const float d = (wx * lx + wy * ly + wz * lz) / len;
    return 0.55f + 0.45f * (d > 0.0f ? d : 0.0f);
}

/* One 0x56 batch: its pools, then every strip as triangles. */
void DrawBatch(const I3rItem &batch, const float *local_world, const void *texture, const Skin *skin)
{
    if (batch.getType() != kTypeVertexBatch || batch.getNumChild() < 5)
        return;
    const I3rItem *slot = batch.getChildren();
    LoadSlot(slot[4], kVuPositions);
    LoadSlot(slot[2], kVuNormals);
    if (skin != nullptr && batch.getNumChild() >= 6 && slot[5].getNumChild() != 0)
        SkinBatch(slot[5], *skin);
    if (slot[0].getNumChild() == 0)
        return;
    const I3rItem &group = slot[0].getChildren()[0];            /* 0x30 */
    if (group.getNumChild() == 0)
        return;
    const I3rItem &list = group.getChildren()[0];
    const I3rItem *strips = list.getChildren();
    int total = 0;
    for (u_int i = 0; i < list.getNumChild(); i++)
    {
        const Strip *s = (const Strip *)strips[i].getData();
        if (s != nullptr && s->m_count >= 3)
            total += (s->m_count - 2) * 3;
    }
    if (total == 0)
        return;
    const unsigned stream = RosePan_RendererBeginMeshStream(nullptr, total, local_world, nullptr,
                                                           texture);
    if (stream == 0)
        return;
    for (u_int i = 0; i < list.getNumChild(); i++)
    {
        const I3rItem &item = strips[i];
        const Strip *s = (const Strip *)item.getData();
        if (item.getType() != kTypeStrip || s == nullptr || s->m_count < 3 || item.getNumChild() < 2)
            continue;
        const u_char *idx = (const u_char *)i3rPtr(&s->m_list);
        if (idx == nullptr)
            continue;
        LoadSlot(item.getChildren()[0], kVuColours);
        LoadSlot(item.getChildren()[1], kVuStq);
        float pos[20][3], nrm[20][3], rgb[20][3];
        float st[20][2];
        uint16_t ix[20][2];
        const int n = s->m_count > 20 ? 20 : s->m_count;
        for (int v = 0; v < n; v++)
        {
            memcpy(ix[v], idx + v * 4, 4);
            const unsigned k = ix[v][0] & 0x7fff;
            VuVec3(kVuPositions + k, pos[v]);
            VuVec3(kVuNormals + k, nrm[v]);
            float c[3];
            VuVec3(kVuColours + v, c);
            const float light = Shade(local_world, nrm[v]);
            for (int j = 0; j < 3; j++)
                rgb[v][j] = c[j] * light;
            const uint32_t *q = s_vu.mem[(kVuStq + v) % kVuSize];
            const float qq = AsFloat(q[2]) != 0.0f ? AsFloat(q[2]) : 1.0f;
            st[v][0] = AsFloat(q[0]) / qq;
            st[v][1] = AsFloat(q[1]) / qq;
        }
        for (int v = 2; v < n; v++)
        {
            if (ix[v][0] & 0x8000)
                continue;                                       /* no kick at this vertex */
            /* y bit 15: (v-2, v-1, v) faces front; checked against the
             * normals over a whole model (6,000 triangles, 1 exception). */
            int order[3] = {v - 2, v - 1, v};
            if ((ix[v][1] & 0x8000) == 0)
            {
                order[1] = v;
                order[2] = v - 1;
            }
            RosePanMeshVertexInput tri[3] = {};
            for (int j = 0; j < 3; j++)
            {
                const int o = order[j];
                tri[j].position = pos[o];
                tri[j].normal = nrm[o];
                tri[j].rgb = rgb[o];
                tri[j].s = st[o][0];
                tri[j].t = st[o][1];
                tri[j].alpha = 1.0f;
            }
            RosePan_RendererAppendMeshTriangle(stream, tri);
        }
    }
    RosePan_RendererCommitMeshStream(stream);
}
}

void RosePan_I3dMeshDraw(const I3rRoot *root, const float *world, const float *nodes,
                         const I3dTexEntry *tex, int tex_count)
{
    if (root == nullptr || world == nullptr || root->m_item.getNumChild() < 4)
        return;
    const I3rItem *child = root->m_item.getChildren();
    const u_char *root_data = (const u_char *)root->m_item.getData();
    if (nodes == nullptr && root_data != nullptr)
        nodes = (const float *)i3rPtr((const u_int *)(root_data + 0x14));  /* I3rRootData::m_matrix14 */

    /* PC: VU1 starts each model with i3dInitHardware's STCOL 1.0 / STROW 0. */
    for (int i = 0; i < 4; i++)
    {
        s_vu.col[i] = 0x3f800000;
        s_vu.row[i] = 0;
    }
    s_vu.mask = 0;

    const I3rItem *materials = child[0].getChildren();
    const unsigned num_mat = child[0].getNumChild();
    const I3rItem *meshes = child[2].getChildren();
    const unsigned num_mesh = child[2].getNumChild();
    const I3rItem *node = child[3].getChildren();
    const unsigned num_node = child[3].getNumChild();
    for (unsigned i = 0; i < num_node; i++)
    {
        if (node[i].getNumChild() < 2)
            continue;
        const I3rItem *part = node[i].getChildren();
        const u_char *nd = (const u_char *)part[0].getData();
        if (nd != nullptr && nd[0x25] == 0)
            continue;                                           /* hidden (i3rDrawModel) */
        const I3rItem *entry = part[1].getChildren();
        for (u_int e = 0; e < part[1].getNumChild(); e++)
        {
            const NodeEntry *ne = (const NodeEntry *)entry[e].getData();
            if (ne == nullptr || ne->m_enable == 0 || ne->m_mesh < 0 || (unsigned)ne->m_mesh >= num_mesh)
                continue;
            const u_short *bones = (const u_short *)i3rPtr(&ne->m_bones);
            const I3rItem &mesh = meshes[ne->m_mesh];
            if (mesh.getNumChild() < 1 || mesh.getChildren()[0].m_data == 0)
                continue;                                       /* 0x45 (the other part): not drawn yet */
            const I3rItem &m46 = mesh.getChildren()[0];
            if (m46.getNumChild() == 0)
                continue;
            /* The 0x46 data: +0 the per-slot offset matrices. */
            Skin skin = {nodes, bones, ne->m_num_bone, num_node,
                         (const float *)i3rPtr((const u_int *)m46.getData())};
            const Skin *skinned = skin.nodes != nullptr && skin.bones != nullptr && skin.offsets != nullptr
                                      ? &skin : nullptr;
            const I3rItem &groups = m46.getChildren()[0];
            const I3rItem *group = groups.getChildren();
            for (u_int g = 0; g < groups.getNumChild(); g++)
            {
                const u_char *gd = (const u_char *)group[g].getData();
                if (gd == nullptr || group[g].getNumChild() == 0)
                    continue;
                float local_world[16];
                memcpy(local_world, world, sizeof(local_world));
                /* Flag 8: rigid, one node; otherwise the batches are skinned
                 * into model space (0x220290's two branches). */
                const bool rigid = (gd[5] & 8) != 0;
                if (rigid && bones != nullptr && nodes != nullptr)
                {
                    /* The data's first word points at the u16 bone slot. */
                    const u_short *slot = (const u_short *)i3rPtr((const u_int *)gd);
                    const u_short slot_no = slot != nullptr ? *slot : 0xffff;
                    if (slot_no < ne->m_num_bone && bones[slot_no] < num_node)
                        MatMul(local_world, nodes + (size_t)bones[slot_no] * 16, world);
                }
                const I3rItem &list = group[g].getChildren()[0];
                const I3rItem *mesh4d = list.getChildren();
                for (u_int m = 0; m < list.getNumChild(); m++)
                {
                    const u_char *md = (const u_char *)mesh4d[m].getData();
                    if (md == nullptr || mesh4d[m].getNumChild() == 0)
                        continue;
                    short mat;
                    memcpy(&mat, md + 0x0c, sizeof(mat));
                    const void *texture = MaterialTexture(materials, num_mat, mat, tex, tex_count);
                    const I3rItem &batches = mesh4d[m].getChildren()[0];
                    for (u_int b = 0; b < batches.getNumChild(); b++)
                        DrawBatch(batches.getChildren()[b], local_world, texture, rigid ? nullptr : skinned);
                }
            }
        }
    }
}
