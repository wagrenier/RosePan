/* ==========================================================================
 *  rosepan_i3dmesh.cpp -- the native i3d module's 3D mesh path (see
 *  rosepan_i3dmesh.h for the model tree it walks).
 *
 *  The ROM builds VIF chains of the mesh blocks (i3rBuildMeshPacket
 *  0x21fb50) and VU1 microprogram 1 (data 0x2912b0) turns each strip into
 *  GS triangles.  Here the blocks are unpacked into a host copy of VU1
 *  memory exactly as VIF1 would write them, and each strip is read back
 *  from there: the index list picks positions and normals from the batch's
 *  pools, colours and STQ come per strip vertex.  The strip colours are lit
 *  on the CPU as microprogram 1 lights them, from the material packet's
 *  light set (LayerLight); its fog and clipping are not reproduced
 *  (open-items.md "Mesh draw").
 * ======================================================================== */
#include "rosepan/rendering/rosepan_i3dmesh.h"

#include <math.h>
#include <string.h>

#include "i3d/i3dctx.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dtex.h"
#include "i3d/i3rdraw.h"
#include "i3d/i3rmesh.h"
#include "i3d/i3rmodel.h"
#include "i3d/i3rroot.h"
#include "lib/unassigned.h"
#include "rosepan/io/rosepan_log.h"
#include "rosepan/rendering/rosepan_i3d.h"
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
/* The GS state a material chain opens with (i3dPktOpenChain). */
constexpr u_long kDefaultAlpha = 0x44;
constexpr u_long kDefaultTest = 0x5000d;

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

/* What a group blends with (i3rDrawMeshBlend 0x2210f0): i3rDrawModel's
 * opt, the entry's weight lists (i3rDrawSetupBlend) and the group's flags
 * (& 3 set, or the group is not blended). */
struct Blend
{
    const I3rBlendOpt  *opt;
    const I3rBlendList *list;
    u_int               flags;
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
bool s_logged_group;
bool s_logged_lights;

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

/* The slot of blend channel `ch` at base: through i3rBlendVertices when
 * the opt names the channel (i3rDrawMeshBlend: positions bit 0, normals
 * bit 1, strip colours bit 3, STQ bit 4), else its own block -- also when
 * i3rBlendVertices leaves the own block (group flags without 1 / 2). */
void LoadChannel(const I3rItem &slot, unsigned base, const Blend *blend, int ch)
{
    if (blend != nullptr && (blend->opt->m_mask & (1u << ch)) != 0 && slot.getNumChild() != 0)
    {
        static float buf[kVuSize][4];
        I3rVertexRef ref = {};
        i3rBlendVertices(blend->opt->m_time[ch], &ref, buf[0], slot.getChildren(), slot.getNumChild(),
                         &blend->list[ch], blend->flags);
        if (ref.m_blended != 0 && ref.m_data != nullptr)
        {
            /* The ROM UNPACKs them as V4-32 to the pool (0x2261b8). */
            const float *q = (const float *)ref.m_data;
            for (unsigned i = 0; i < ref.m_count && i < kVuSize; i++)
                memcpy(s_vu.mem[(base + i) % kVuSize], q + i * 4, 16);
            return;
        }
    }
    LoadSlot(slot, base);
}

void VuVec3(unsigned addr, float out[3])
{
    const uint32_t *q = s_vu.mem[addr % kVuSize];
    out[0] = AsFloat(q[0]);
    out[1] = AsFloat(q[1]);
    out[2] = AsFloat(q[2]);
}

void VuVec4(unsigned addr, float out[4])
{
    VuVec3(addr, out);
    out[3] = AsFloat(s_vu.mem[addr % kVuSize][3]);
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

/* Material `index`'s layer list (root [0]: the material's child 0), or
 * NULL. Its child count is the writers' layer count (i3rDrawGroup45 reads
 * it from the list item's m_info). */
const I3rItem *MaterialLayers(const I3rItem *materials, unsigned num_mat, int index)
{
    if (materials == nullptr || index < 0 || (unsigned)index >= num_mat)
        return nullptr;
    const I3rItem &mat = materials[index];
    return mat.getNumChild() != 0 ? &mat.getChildren()[0] : nullptr;
}

/* The texture of material `index`'s layer `u`: its m_tex_index into the
 * texture table. */
const void *MaterialTexture(const I3rItem *materials, unsigned num_mat, int index, unsigned u,
                            const I3dTexEntry *tex, int tex_count)
{
    const I3rItem *layers = MaterialLayers(materials, num_mat, index);
    if (layers == nullptr || u >= layers->getNumChild() || tex == nullptr)
        return nullptr;
    const u_char *layer = (const u_char *)layers->getChildren()[u].getData();
    if (layer == nullptr)
        return nullptr;
    int tex_index;
    memcpy(&tex_index, layer + 0x1c, sizeof(tex_index));   /* I3rLayerData::m_tex_index */
    if (tex_index < 0 || tex_index >= tex_count || tex[tex_index].pPicture == nullptr)
        return nullptr;
    const I3dTexEntry &e = tex[tex_index];
    return RosePan_I3dTextureFor(e.pPicture, e.m_clut, e.m_0c, 1);
}

/* Whether material `index`'s layer `u` maps STQ through the scene's shadow
 * entity: an STQ map slot (the layer's child 3) of kind 3, the one
 * MagScene's +0x2aa90 replaces (i3rPktLayerStqMaps: kind 3 brings its own
 * matrix, makeActorShadow's texture matrix x light view). */
bool LayerHasShadowStq(const I3rItem *materials, unsigned num_mat, int index, unsigned u)
{
    const I3rItem *layers = MaterialLayers(materials, num_mat, index);
    if (layers == nullptr || u >= layers->getNumChild() || layers->getChildren()[u].getNumChild() < 4)
        return false;
    const I3rItem &maps = layers->getChildren()[u].getChildren()[3];
    for (u_int i = 0; i < maps.getNumChild(); i++)
    {
        const I3rStqMapEntity *e = (const I3rStqMapEntity *)maps.getChildren()[i].getData();
        if (e != nullptr && e->m_id == 3 && e->m_40 != 0)
            return true;
    }
    return false;
}

/* The GS state of material `index` as its layer `u`'s packet sets it
 * (i3rPktLayerTest / i3rPktLayerAlpha, i3d/i3rmesh.cpp), over the chain's
 * defaults (i3dPktOpenChain: ALPHA 0x44, TEST 0x5000d) where the layer
 * writes none. The ROM keeps the previous layer's register instead; the
 * host starts each layer from the defaults. */
void ApplyMaterialState(const I3rItem *materials, unsigned num_mat, int index, unsigned u)
{
    u_long test = kDefaultTest;
    u_long alpha = kDefaultAlpha;
    const I3rItem *layers = MaterialLayers(materials, num_mat, index);
    const I3rLayerData *layer = layers != nullptr && u < layers->getNumChild()
                                    ? (const I3rLayerData *)layers->getChildren()[u].getData()
                                    : nullptr;
    if (layer != nullptr)
    {
        u_long regs[4][2];
        u_long *end = i3rPktLayerAlpha(i3rPktLayerTest(regs[0], layer), layer);
        for (const u_long *r = regs[0]; r < end; r += 2)
        {
            if (r[1] == I3D_GS_TEST_1)
                test = r[0];
            else if (r[1] == I3D_GS_ALPHA_1)
                alpha = r[0];
        }
    }
    RosePan_RendererSetGsTestRegister(test);
    RosePan_RendererSetGsAlphaRegister(alpha);
}

/* A fixed light for a model drawn with no light set (the forced mesh
 * test): from above and in front, in world space. */
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

/* How a material layer's strips are lit: what VU1 microprogram 1 does
 * with i3rPktLayerState's colour block and i3rPktLayerLights' lists. Per
 * layer an accumulator per normal is cleared (micro 0x15e), each light
 * list adds into it -- shade 1: parallel 0x39 max(0, n.d) c, point 0xc8
 * c max(0, n.L) / |L|^2 (L to the light, not normalised), spot 0x19a c
 * cone clamp01(n.l) / |L| with l = L / |L| and cone = clamp01((-l.dir -
 * cone 0) x cone 1) (i3dLightSetSpot's cos outer, 1 / (cos inner - cos
 * outer)); shade 2: parallel 0x66 c max(0, P(n.h)) with h the half vector
 * i3rPktLayerLights makes, point 0x108 c max(0, P(n.h)) / |L| with h =
 * normalize(L / |L| + eye), P(x) = s0 x + s1 x^2 + s2 x^4 + s3 x^8 of the
 * layer's m_specular -- and the strip program (0x3fc) gives each vertex
 * its strip colour x (accumulator + ambient) x the colour scale (128
 * textured, 255 not), clamped. All of that runs on four components: the
 * vertex alpha is strip colour w x (accumulator w + ambient w) x 128, so a
 * model's translucency is in its strip colours (the butterfly boxes'
 * "glass" meshes are white at alpha 0) and a projected shadow's darkness
 * in its light (MagScene::m_shadow_light: black, the strength in the
 * colour's w, ambient w 0, so a surface turned away from the light or
 * outside the spot takes no shadow). The normals are rotated by the batch
 * matrix unnormalised, as the VU does; the host does the sums in world
 * space, the same products for a matrix of uniform scale. */
struct LayerLight
{
    const RosePanI3dLightSet *set;      /* NULL: no light set (Shade) */
    int   shade;                        /* I3rLayerData::m_shade: 0 unlit, 1 lights, 2 specular */
    float ambient[4];                   /* the colour block's ambient qword */
    float poly[4];                      /* shade 2: m_specular */
    float half[ROSEPAN_I3D_MAX_PARALLEL][3];   /* shade 2: the parallel lights' half vectors */
    float ceiling;                      /* the clamp, in the mesh stream's units (128 = 1.0) */
};

void Normalize3(float v[3])
{
    const float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > 0.0f)
        for (int i = 0; i < 3; i++)
            v[i] /= len;
}

float Dot3(const float *a, const float *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float Clamp01f(float x)
{
    return x > 0.0f ? (x < 1.0f ? x : 1.0f) : 0.0f;
}

float SpecularPoly(const float *s, float x)
{
    const float x2 = x * x;
    const float x4 = x2 * x2;
    return s[0] * x + s[1] * x2 + s[2] * x4 + s[3] * x4 * x4;
}

/* Layer `d`'s lighting under `set` (i3rPktLayerState's ambient choice and
 * i3rPktLayerLights' half vectors). */
void SetupLayerLight(LayerLight &l, const I3rLayerData *d, bool textured, const RosePanI3dLightSet *set)
{
    memset(&l, 0, sizeof(l));
    l.set = set;
    l.shade = d != nullptr ? d->m_shade : 0;
    /* MINI against the strip program's ceiling, 255: under a texture the
     * scale is 128, so overbright up to 255 / 128. */
    l.ceiling = textured ? 255.0f / 128.0f : 1.0f;
    l.ambient[3] = 1.0f;
    if (l.shade == 0)
        l.ambient[0] = l.ambient[1] = l.ambient[2] = 1.0f;
    else if (d->m_ambient != 0 && set != nullptr)
        memcpy(l.ambient, set->ambient, sizeof(l.ambient));
    if (l.shade == 2 && set != nullptr)
    {
        memcpy(l.poly, d->m_specular, sizeof(l.poly));
        for (int i = 0; i < set->parallel_count; i++)
        {
            for (int c = 0; c < 3; c++)
                l.half[i][c] = set->parallel_dir[i][c] + set->eye_dir[c];
            Normalize3(l.half[i]);
        }
    }
}

/* The factor vertex (p, n) (model space) multiplies its strip colour by
 * under l, through the batch matrix m. */
void LightVertex(const LayerLight &l, const float *m, const float p[3], const float n[3], float out[4])
{
    if (l.set == nullptr)
    {
        out[0] = out[1] = out[2] = l.shade == 0 ? 1.0f : Shade(m, n);
        out[3] = 1.0f;
        return;
    }
    memcpy(out, l.ambient, sizeof(l.ambient));
    if (l.shade == 0)
        return;
    const RosePanI3dLightSet &s = *l.set;
    float nw[3], pw[3];
    for (int c = 0; c < 3; c++)
    {
        nw[c] = n[0] * m[c] + n[1] * m[4 + c] + n[2] * m[8 + c];
        pw[c] = p[0] * m[c] + p[1] * m[4 + c] + p[2] * m[8 + c] + m[12 + c];
    }
    for (int i = 0; i < s.parallel_count; i++)
    {
        float f;
        if (l.shade == 1)
            f = Dot3(nw, s.parallel_dir[i]);
        else
            f = SpecularPoly(l.poly, Dot3(nw, l.half[i]));
        if (f > 0.0f)
            for (int c = 0; c < 4; c++)
                out[c] += f * s.parallel_color[i][c];
    }
    for (int i = 0; i < s.point_count; i++)
    {
        float L[3];
        for (int c = 0; c < 3; c++)
            L[c] = s.point_pos[i][c] - pw[c];
        const float len2 = Dot3(L, L);
        if (!(len2 > 0.0f))
            continue;
        float f;
        if (l.shade == 1)
        {
            f = Dot3(nw, L) / len2;
        }
        else
        {
            const float inv = 1.0f / sqrtf(len2);
            float h[3];
            for (int c = 0; c < 3; c++)
                h[c] = L[c] * inv + s.eye_dir[c];
            Normalize3(h);
            f = SpecularPoly(l.poly, Dot3(nw, h));
            f = f > 0.0f ? f * inv : 0.0f;
        }
        if (f > 0.0f)
            for (int c = 0; c < 4; c++)
                out[c] += f * s.point_color[i][c];
    }
    /* Shade 1's spot, the only one in use: spots come from the shadow light,
     * whose materials are all shade 1 (shade 2's micro 0x212 is not done).
     * The micro first drops a batch whose bounding sphere misses the cone;
     * per vertex that is the same zero. */
    for (int i = 0; l.shade == 1 && i < s.spot_count; i++)
    {
        float L[3];
        for (int c = 0; c < 3; c++)
            L[c] = s.spot_pos[i][c] - pw[c];
        const float len2 = Dot3(L, L);
        if (!(len2 > 0.0f))
            continue;
        const float inv = 1.0f / sqrtf(len2);
        for (int c = 0; c < 3; c++)
            L[c] *= inv;
        const float *cone = s.spot_cone[i];
        const float in_cone = Clamp01f((-Dot3(L, s.spot_dir[i]) - cone[0]) * cone[1]);
        const float f = in_cone * Clamp01f(Dot3(nw, L)) * inv;
        if (f > 0.0f)
            for (int c = 0; c < 4; c++)
                out[c] += f * s.spot_color[i][c];
    }
}

/* What a 0x4d mesh draws its batches with: its material and, per material
 * layer, the batch strip group that layer draws (0xff = none). */
struct MeshLayers
{
    const I3rItem     *materials;
    unsigned           num_mat;
    int                mat;
    const u_char      *group;           /* the mesh data's +0x08 list; NULL = layer 0 draws group 0 */
    unsigned           count;           /* the material's layers */
    const I3dTexEntry *tex;
    int                tex_count;
    const RosePanI3dLightSet *lights;   /* the material packet's light set; NULL none */
};

/* One 0x30 strip group of a batch (its child 0 list) as triangles, over the
 * pools DrawBatch loaded, lit as `light` says. */
void DrawStripGroup(const I3rItem &group, const float *local_world, const void *texture, const Blend *blend,
                    const LayerLight &light)
{
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
        LoadChannel(item.getChildren()[0], kVuColours, blend, I3R_BLEND_COLOUR);
        LoadChannel(item.getChildren()[1], kVuStq, blend, I3R_BLEND_STQ);
        float pos[20][3], nrm[20][3], rgb[20][3];
        float alpha[20];
        float st[20][2];
        uint16_t ix[20][2];
        const int n = s->m_count > 20 ? 20 : s->m_count;
        for (int v = 0; v < n; v++)
        {
            memcpy(ix[v], idx + v * 4, 4);
            const unsigned k = ix[v][0] & 0x7fff;
            VuVec3(kVuPositions + k, pos[v]);
            VuVec3(kVuNormals + k, nrm[v]);
            float c[4], lit[4];
            VuVec4(kVuColours + v, c);
            LightVertex(light, local_world, pos[v], nrm[v], lit);
            for (int j = 0; j < 3; j++)
            {
                const float x = c[j] * lit[j];
                rgb[v][j] = x > 0.0f ? (x < light.ceiling ? x : light.ceiling) : 0.0f;
            }
            /* The colour scale's w is 128 on every layer, the host's 1.0. */
            alpha[v] = c[3] * lit[3];
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
                tri[j].alpha = alpha[o];
            }
            RosePan_RendererAppendMeshTriangle(stream, tri);
        }
    }
    RosePan_RendererCommitMeshStream(stream);
}

/* One 0x56 batch: its pools, then per material layer the strip group the
 * mesh names for it, with that layer's texture and GS state -- the CALL
 * pairs i3rPktCallStrips 0x221080 writes (the layer's packet, then the
 * batch's chain of group mesh->group[u]). */
void DrawBatch(const I3rItem &batch, const float *local_world, const MeshLayers &ml, const Skin *skin,
               const Blend *blend)
{
    if (batch.getType() != kTypeVertexBatch || batch.getNumChild() < 5)
        return;
    const I3rItem *slot = batch.getChildren();
    /* Blended first, then skinned (i3rDrawMeshBlend's 0x225670 skins the
     * blended pools). */
    LoadChannel(slot[4], kVuPositions, blend, I3R_BLEND_POSITION);
    LoadChannel(slot[2], kVuNormals, blend, I3R_BLEND_NORMAL);
    if (skin != nullptr && batch.getNumChild() >= 6 && slot[5].getNumChild() != 0)
        SkinBatch(slot[5], *skin);
    const unsigned num_group = slot[0].getNumChild();
    const unsigned count = ml.group != nullptr ? ml.count : 1;
    for (unsigned u = 0; u < count; u++)
    {
        const unsigned g = ml.group != nullptr ? ml.group[u] : 0;
        if (g == 0xff)
            continue;                                           /* the ROM's -1: this layer draws nothing here */
        if (g >= num_group)
        {
            if (!s_logged_group)
            {
                RosePan_LogPrintf("rosepan_i3dmesh: layer %u names group %u of %u\n", u, g, num_group);
                s_logged_group = true;
            }
            continue;
        }
        /* A shadow receiver layer (the map's shadow model, drawn after
         * makeActorShadow's pass): the renderer projects the pass's map
         * over it instead of the layer's texture. */
        const bool receiver = LayerHasShadowStq(ml.materials, ml.num_mat, ml.mat, u);
        if (receiver && !RosePan_I3dShadowReceiverReady())
            continue;
        ApplyMaterialState(ml.materials, ml.num_mat, ml.mat, u);
        if (receiver)
        {
            static bool s_logged_receiver;
            if (!s_logged_receiver)
            {
                RosePan_LogPrintf("rosepan_i3dmesh: shadow receiver layer drawn\n");
                s_logged_receiver = true;
            }
            RosePan_RendererShadowBeginReceiver();
        }
        const I3rItem *layers = MaterialLayers(ml.materials, ml.num_mat, ml.mat);
        const I3rLayerData *d = layers != nullptr && u < layers->getNumChild()
                                    ? (const I3rLayerData *)layers->getChildren()[u].getData()
                                    : nullptr;
        LayerLight light;
        SetupLayerLight(light, d, d != nullptr && d->m_tex_index >= 0, ml.lights);
        if (light.shade != 0 && ml.lights != nullptr && !s_logged_lights)
        {
            const RosePanI3dLightSet &s = *ml.lights;
            RosePan_LogPrintf("rosepan_i3dmesh: first lit layer: shade %d, ambient %.3f %.3f %.3f, "
                              "%d parallel, %d point, %d other\n",
                              light.shade, light.ambient[0], light.ambient[1], light.ambient[2],
                              s.parallel_count, s.point_count, s.other_count);
            s_logged_lights = true;
        }
        DrawStripGroup(slot[0].getChildren()[g], local_world,
                       MaterialTexture(ml.materials, ml.num_mat, ml.mat, u, ml.tex, ml.tex_count), blend, light);
        if (receiver)
            RosePan_RendererShadowEndReceiver();
    }
}

/* One group's 0x4d meshes (the group's child 0 list): per mesh its
 * material and layer list, then its batches. `ref`: only its mesh (and
 * batch); NULL = all. */
void DrawMeshList(const I3rItem &list, const I3rItem *materials, unsigned num_mat, const I3dTexEntry *tex,
                  int tex_count, const float *local_world, const Skin *skin, const Blend *blend,
                  const RosePanI3dMeshRef *ref, const RosePanI3dLightSet *lights)
{
    const I3rItem *mesh4d = list.getChildren();
    for (u_int m = 0; m < list.getNumChild(); m++)
    {
        if (ref != nullptr && m != ref->mesh)
            continue;
        const u_char *md = (const u_char *)mesh4d[m].getData();
        if (md == nullptr || mesh4d[m].getNumChild() == 0)
            continue;
        MeshLayers ml = {materials, num_mat, 0, nullptr, 0, tex, tex_count, lights};
        short mat;
        memcpy(&mat, md + 0x0c, sizeof(mat));
        ml.mat = mat;
        const I3rItem *layers = MaterialLayers(materials, num_mat, mat);
        ml.count = layers != nullptr ? layers->getNumChild() : 0;
        ml.group = ml.count != 0 ? (const u_char *)i3rPtr((const u_int *)(md + 0x08)) : nullptr;
        const I3rItem &batches = mesh4d[m].getChildren()[0];
        for (u_int b = 0; b < batches.getNumChild(); b++)
            if (ref == nullptr || ref->batch < 0 || b == (u_int)ref->batch)
                DrawBatch(batches.getChildren()[b], local_world, ml, skin, blend);
    }
}

/* The walk of RosePan_I3dMeshDraw; `ref` limits it to one mesh. */
void DrawModel(const I3rRoot *root, const float *world, const float *nodes, const I3dTexEntry *tex,
               int tex_count, const I3rBlendOpt *opt, const RosePanI3dMeshRef *ref,
               const RosePanI3dLightSet *lights)
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
        if (ref != nullptr && i != ref->node)
            continue;
        if (node[i].getNumChild() < 2)
            continue;
        const I3rItem *part = node[i].getChildren();
        const u_char *nd = (const u_char *)part[0].getData();
        if (nd != nullptr && nd[0x25] == 0)
            continue;                                           /* hidden (i3rDrawModel) */
        const I3rItem *entry = part[1].getChildren();
        for (u_int e = 0; e < part[1].getNumChild(); e++)
        {
            if (ref != nullptr && e != ref->entry)
                continue;
            const NodeEntry *ne = (const NodeEntry *)entry[e].getData();
            if (ne == nullptr || ne->m_enable == 0 || ne->m_mesh < 0 || (unsigned)ne->m_mesh >= num_mesh)
                continue;
            const u_short *bones = (const u_short *)i3rPtr(&ne->m_bones);
            const I3rItem &mesh = meshes[ne->m_mesh];
            /* The entry's blend weights (i3rDrawModel's i3rDrawSetupBlend);
             * a group blends when its flags & 3 is set and there is an opt
             * (the group writers' test). */
            I3rBlendList lists[I3R_BLEND_CHANNELS];
            memset(lists, 0, sizeof(lists));
            if (opt != nullptr)
                i3rDrawSetupBlend(&entry[e], opt, lists);
            /* Child 1, type 0x45: rigid, every group with the node's own
             * matrix (i3rDrawMesh45 / 0x220838); drawn first, as the ROM. */
            if (mesh.getNumChild() >= 2 && mesh.getChildren()[1].m_data != 0 && nodes != nullptr &&
                (ref == nullptr || ref->part == 0x45))
            {
                const I3rItem &m45 = mesh.getChildren()[1];
                if (m45.getNumChild() >= 2)
                {
                    float node_world[16];
                    MatMul(node_world, nodes + (size_t)i * 16, world);
                    const I3rItem &groups = m45.getChildren()[1];
                    for (u_int g = 0; g < groups.getNumChild(); g++)
                    {
                        if (ref != nullptr && g != ref->group)
                            continue;
                        const I3rItem &group = groups.getChildren()[g];
                        const u_char *gd = (const u_char *)group.getData();
                        /* The 0x45 group's flags: its data's first byte. */
                        const Blend blend = {opt, lists, gd != nullptr ? gd[0] : 0u};
                        if (group.getNumChild() != 0)
                            DrawMeshList(group.getChildren()[0], materials, num_mat, tex, tex_count,
                                         node_world, nullptr,
                                         opt != nullptr && (blend.flags & 3) != 0 ? &blend : nullptr, ref,
                                         lights);
                    }
                }
            }
            if (mesh.getNumChild() < 1 || mesh.getChildren()[0].m_data == 0 ||
                (ref != nullptr && ref->part != 0x46))
                continue;
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
                if (ref != nullptr && g != ref->group)
                    continue;
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
                const Blend blend = {opt, lists, gd[5]};
                DrawMeshList(group[g].getChildren()[0], materials, num_mat, tex, tex_count, local_world,
                             rigid ? nullptr : skinned, opt != nullptr && (gd[5] & 3) != 0 ? &blend : nullptr, ref,
                             lights);
            }
        }
    }
    /* PC: the next draws (the 2D replay sets ALPHA but not TEST) start
     * from the chain defaults again. */
    RosePan_RendererSetGsTestRegister(kDefaultTest);
    RosePan_RendererSetGsAlphaRegister(kDefaultAlpha);
}
}

void RosePan_I3dMeshDraw(const I3rRoot *root, const float *world, const float *nodes,
                         const I3dTexEntry *tex, int tex_count, const I3rBlendOpt *opt)
{
    DrawModel(root, world, nodes, tex, tex_count, opt, nullptr, nullptr);
}

void RosePan_I3dMeshDrawRef(const I3rRoot *root, const float *world, const float *nodes,
                            const I3dTexEntry *tex, int tex_count, const I3rBlendOpt *opt,
                            const RosePanI3dMeshRef *ref, const RosePanI3dLightSet *lights)
{
    if (ref != nullptr)
        DrawModel(root, world, nodes, tex, tex_count, opt, ref, lights);
}
