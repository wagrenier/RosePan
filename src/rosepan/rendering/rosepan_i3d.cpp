/* ==========================================================================
 *  rosepan_i3d.cpp -- the native i3d draw module (see rosepan_i3d.h).
 * ======================================================================== */
#include "rosepan/rendering/rosepan_i3d.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <unordered_map>
#include <vector>

#include "i3d/i3dctx.h"
#include "i3d/i3dlayer.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dtex.h"
#include "rosepan/io/rosepan_log.h"
#include "rosepan/io/rosepan_paths.h"
#include "rosepan/rendering/rosepan_draw2d.h"
#include "rosepan/rendering/rosepan_renderer.h"

namespace
{
/* RoR's frame buffer (MagSystem: 512x448) and the GS draw offset its draw
 * environment sets (2048 - 256, 2048 - 224; MagViewParam m_offx / m_offy). */
constexpr int kFrameWidth = 512;
constexpr int kFrameHeight = 448;
constexpr int kDefaultOffX = (2048 - kFrameWidth / 2) << 4;
constexpr int kDefaultOffY = (2048 - kFrameHeight / 2) << 4;
/* The renderer's frame: RoR's 512 columns stretch to its 640, as the PS2
 * display stretches them. */
constexpr float kScaleX = 640.0f / (float)kFrameWidth;
constexpr float kScaleY = 448.0f / (float)kFrameHeight;

/* GS registers the module follows. */
constexpr uint64_t kRegTex0 = 0x06;
constexpr uint64_t kRegTex1 = 0x14;
constexpr uint64_t kRegXyOffset = 0x18;
constexpr uint64_t kRegScissor = 0x40;
constexpr uint64_t kRegAlpha = 0x42;
constexpr uint64_t kRegTest = 0x47;

/* The opening state i3dPktOpenChain writes (ALPHA mode 0x10, TEST_1). */
constexpr uint64_t kDefaultAlpha = 0x44;
constexpr uint64_t kDefaultTest = 0x5000d;

struct Vertex
{
    int      x, y;          /* XYZ2, 12.4 with the draw offset */
    uint8_t  rgba[4];       /* RGBAQ, PS2 scale */
    float    s, t, q;       /* ST and RGBAQ's Q */
    unsigned u, v;          /* UV, 10.4 texels */
};

enum RecordKind
{
    REC_PRIM,
    REC_REG,
    REC_TEXREGS
};

struct Record
{
    RecordKind      kind;
    unsigned        prim;           /* REC_PRIM: the PRIM register */
    int             count;          /* REC_PRIM: vertices */
    Vertex          v[4];
    uint64_t        reg, value;     /* REC_REG */
    const uint64_t *regs;           /* REC_TEXREGS */
};

struct Binding
{
    const I3dTexture *tex;
    const I3dClut    *clut;
    unsigned          clut_index;
};

struct TexKey
{
    const void *tex;
    const void *clut;
    unsigned    clut_index;
    int         tcc;
    bool operator<(const TexKey &o) const
    {
        if (tex != o.tex) return tex < o.tex;
        if (clut != o.clut) return clut < o.clut;
        if (clut_index != o.clut_index) return clut_index < o.clut_index;
        return tcc < o.tcc;
    }
};

/* The GS state a replay walks with (global on the GS, last write wins). */
struct GsState
{
    uint64_t        alpha = kDefaultAlpha;
    uint64_t        test = kDefaultTest;
    uint64_t        scissor = (uint64_t)(kFrameWidth - 1) << 16 | (uint64_t)(kFrameHeight - 1) << 48;
    int             offx = kDefaultOffX;
    int             offy = kDefaultOffY;
    const uint64_t *tex_regs = nullptr;
    /* A TEX0 written as a plain register (i3dPktSetReg) after the draw set's
     * registers: GS memory the module does not hold -- SprMan::drawFrameCopy's
     * other frame buffer, SPR_TEX_FRAME sprites. */
    bool            direct_tex0 = false;
};

std::map<uintptr_t, Record> s_records;
std::unordered_map<const uint64_t *, Binding> s_bindings;
std::map<TexKey, const void *> s_textures;
GsState s_state;
unsigned s_vu1_count;
bool s_logged_offscreen;
bool s_logged_notex;
bool s_logged_direct_tex0;

float FloatOf(uint32_t bits)
{
    float f;
    memcpy(&f, &bits, sizeof(f));
    return f;
}

/* ---- textures ---- */

const void *TextureFor(const Binding &b, int tcc)
{
    const TexKey key = {b.tex, b.clut, b.clut_index, tcc};
    auto it = s_textures.find(key);
    if (it != s_textures.end())
        return it->second;

    const I3dTexture *t = b.tex;
    RosePanImage2d image = {};
    image.pixels = t->m_level[0].m_data;
    image.psm = (int)((t->m_tex2 >> 20) & 0x3f);
    image.width = (int)t->m_width;
    image.height = (int)t->m_height;
    image.tcc = tcc;
    if (b.clut != nullptr && b.clut->m_data != nullptr)
    {
        const I3dClut *c = b.clut;
        const int bytes = c->m_psm == I3D_PSMCT32 ? 4 : c->m_psm == I3D_PSMCT24 ? 3 : 2;
        /* The palette TEX0 selects (i3dTexRegsSet): CBP steps one 16x16
         * CLUT image (256 colours) per 8-bit palette, and per 16 4-bit
         * ones, whose CSA picks 16 colours inside it. */
        const unsigned chunk = image.psm == I3D_PSMT4 ? b.clut_index >> 4 : b.clut_index;
        const unsigned first = chunk * 256u;
        image.clut = c->m_data + (size_t)first * (size_t)bytes;
        image.clut_colors = c->m_colors > first ? (int)(c->m_colors - first) : 0;
        image.clut_offset = image.psm == I3D_PSMT4 ? (int)(b.clut_index & 0xf) * 16 : 0;
        image.clut_psm = (int)c->m_psm;
        image.clut_csm2 = (int)((c->m_tex0 >> 55) & 1);
    }
    const void *handle = RosePan_Draw2dMakeTexture(&image);
    s_textures[key] = handle;
    return handle;
}

/* ---- drawing ---- */

float FrameX(const GsState &gs, int x)
{
    return ((float)(x - gs.offx) / 16.0f) * kScaleX;
}

float FrameY(const GsState &gs, int y)
{
    return ((float)(y - gs.offy) / 16.0f) * kScaleY;
}

/* The renderer's GS shadows from the replay state: ALPHA as it is, the
 * scissor in the renderer's 640x448 frame. */
void ApplyState(const GsState &gs)
{
    RosePan_RendererSetGsAlphaRegister(gs.alpha);
    const uint64_t x0 = (uint64_t)((double)(gs.scissor & 0x7ff) * kScaleX);
    const uint64_t x1 = (uint64_t)((double)(((gs.scissor >> 16) & 0x7ff) + 1) * kScaleX) - 1;
    const uint64_t y0 = (gs.scissor >> 32) & 0x7ff;
    const uint64_t y1 = (gs.scissor >> 48) & 0x7ff;
    RosePan_RendererSetGsScissorRegister(x0 | x1 << 16 | y0 << 32 | y1 << 48);
}

void RestoreState()
{
    RosePan_RendererSetGsAlphaRegister(kDefaultAlpha);
    RosePan_RendererSetGsScissorRegister((uint64_t)639 << 16 | (uint64_t)447 << 48);
}

/* Does the current state write colour at all? */
bool DrawsColour(const GsState &gs)
{
    /* ALPHA with A == B is (Cs - Cs) * C + D: D alone, nothing when D is
     * the destination (i3dPktSetAlphaMode 1). */
    const unsigned a = (unsigned)(gs.alpha & 3), b = (unsigned)((gs.alpha >> 2) & 3);
    const unsigned d = (unsigned)((gs.alpha >> 6) & 3);
    if (a == b && d == 1)
        return false;
    /* TEST: alpha test NEVER failing to keep the frame (ZB_ONLY / KEEP) --
     * i3dPktClearZ's Z-only sprite. */
    const unsigned ate = (unsigned)(gs.test & 1), atst = (unsigned)((gs.test >> 1) & 7);
    const unsigned afail = (unsigned)((gs.test >> 12) & 3);
    if (ate && atst == 0 && (afail == 0 || afail == 2))
        return false;
    return true;
}

void DrawPrim(const Record &r)
{
    const GsState &gs = s_state;
    if (!DrawsColour(gs))
        return;
    if (gs.offx == 0 && gs.offy == 0)
    {
        /* XYOFFSET 0: a render-target pass (MagSceneRender's offscreen
         * stages), not the display. */
        if (!s_logged_offscreen)
        {
            RosePan_LogPrintf("rosepan_i3d: offscreen pass (XYOFFSET 0) not drawn yet\n");
            s_logged_offscreen = true;
        }
        return;
    }

    const unsigned type = r.prim & 7;
    const bool iip = (r.prim & 0x08) != 0;
    const bool tme = (r.prim & 0x10) != 0;
    const bool fst = (r.prim & 0x100) != 0;

    const void *texture = nullptr;
    float tw = 1.0f, th = 1.0f;
    int decal = 0, linear = 1;
    if (tme && gs.direct_tex0)
    {
        /* Drawn with the last bound texture it would paint that texture over
         * the frame (the title's logo over the fader); left out until the
         * module keeps frame-buffer copies. */
        if (!s_logged_direct_tex0)
        {
            RosePan_LogPrintf("rosepan_i3d: textured primitive with a direct TEX0 (frame buffer) not drawn yet\n");
            s_logged_direct_tex0 = true;
        }
        return;
    }
    if (tme)
    {
        auto it = gs.tex_regs ? s_bindings.find(gs.tex_regs) : s_bindings.end();
        if (it == s_bindings.end() || it->second.tex == nullptr)
        {
            if (!s_logged_notex)
            {
                RosePan_LogPrintf("rosepan_i3d: textured primitive without a texture object\n");
                s_logged_notex = true;
            }
            return;
        }
        const uint64_t tex0 = gs.tex_regs[2];
        const uint64_t tex1 = gs.tex_regs[4];
        texture = TextureFor(it->second, (int)((tex0 >> 34) & 1));
        if (texture == nullptr)
            return;
        tw = (float)(1u << ((tex0 >> 26) & 0xf));
        th = (float)(1u << ((tex0 >> 30) & 0xf));
        decal = ((tex0 >> 35) & 3) == 1;
        linear = (int)((tex1 >> 5) & 1);
    }

    float xy[8], uv[8];
    uint8_t rgba[16];
    const int n = r.count;
    for (int i = 0; i < n; i++)
    {
        const Vertex &v = r.v[i];
        /* Flat shading takes the last vertex's colour. */
        const Vertex &c = iip ? v : r.v[n - 1];
        xy[i * 2 + 0] = FrameX(gs, v.x);
        xy[i * 2 + 1] = FrameY(gs, v.y);
        if (fst)
        {
            uv[i * 2 + 0] = (float)v.u / 16.0f / tw;
            uv[i * 2 + 1] = (float)v.v / 16.0f / th;
        }
        else
        {
            const float q = v.q != 0.0f ? v.q : 1.0f;
            uv[i * 2 + 0] = v.s / q;
            uv[i * 2 + 1] = v.t / q;
        }
        for (int k = 0; k < 4; k++)
            rgba[i * 4 + k] = decal && k < 3 ? 0x80 : c.rgba[k];
    }

    ApplyState(gs);
    switch (type)
    {
    case 6:                                     /* sprite */
    {
        if (n < 2)
            break;
        const float qxy[8] = {xy[0], xy[1], xy[2], xy[1], xy[0], xy[3], xy[2], xy[3]};
        if (tme)
        {
            const float quv[8] = {uv[0], uv[1], uv[2], uv[1], uv[0], uv[3], uv[2], uv[3]};
            RosePan_RendererDrawImageQuad(texture, qxy, quv, rgba[4], rgba[5], rgba[6], rgba[7],
                                          linear);
        }
        else
        {
            uint8_t q[16];
            for (int k = 0; k < 4; k++)
                memcpy(&q[k * 4], &rgba[4], 4);
            RosePan_RendererDrawSolidQuad(qxy, q);
        }
        break;
    }
    case 3:                                     /* triangle */
        if (n < 3)
            break;
        if (tme)
            RosePan_RendererDrawImageTriangles2D(texture, xy, uv, rgba, 3, linear);
        else
            RosePan_RendererDrawSolidTriangles2D(xy, rgba, 3);
        break;
    case 1:                                     /* line: textured lines draw in their colour */
        if (n >= 2)
        {
            const float lxy[4] = {xy[0], xy[1], xy[2], xy[3]};
            RosePan_RendererDrawLine2D(lxy, rgba, kScaleX);
        }
        break;
    case 0:                                     /* point: one frame pixel */
    {
        const float x = xy[0], y = xy[1];
        const float pxy[8] = {x, y, x + kScaleX, y, x, y + kScaleY, x + kScaleX, y + kScaleY};
        uint8_t q[16];
        for (int k = 0; k < 4; k++)
            memcpy(&q[k * 4], rgba, 4);
        RosePan_RendererDrawSolidQuad(pxy, q);
        break;
    }
    default:
        break;
    }
}

void ApplyReg(uint64_t reg, uint64_t value)
{
    switch (reg)
    {
    case kRegAlpha:    s_state.alpha = value; break;
    case kRegTest:     s_state.test = value; break;
    case kRegScissor:  s_state.scissor = value; break;
    case kRegTex0:     s_state.direct_tex0 = true; break;
    case kRegXyOffset:
        s_state.offx = (int)(value & 0xffff);
        s_state.offy = (int)((value >> 32) & 0xffff);
        break;
    default:           break;   /* TEX0 / TEX1 come through RosePan_I3dRecordTexRegs */
    }
}
}

void RosePan_I3dRecordGif(const uint64_t *pkt)
{
    const uint64_t tag = pkt[2];
    const uint64_t desc = pkt[3];
    const unsigned nloop = (unsigned)(tag & 0x7fff);
    const unsigned flg = (unsigned)((tag >> 58) & 3);
    unsigned nreg = (unsigned)(tag >> 60);
    if (nreg == 0)
        nreg = 16;

    Record r = {};
    r.kind = REC_PRIM;
    if ((tag >> 46) & 1)
        r.prim = (unsigned)((tag >> 47) & 0x7ff);

    /* The vertex queue: RGBAQ / ST / UV latch, XYZ2 kicks. */
    Vertex cur = {};
    cur.q = 1.0f;
    const uint64_t *data = pkt + 4;
    for (unsigned loop = 0; loop < nloop; loop++)
    {
        for (unsigned i = 0; i < nreg; i++)
        {
            const unsigned id = (unsigned)((desc >> (i * 4)) & 0xf);
            if (flg == 0)
            {
                /* PACKED: one qword per register, four words. */
                const int32_t *w = (const int32_t *)data;
                switch (id)
                {
                case 0x0: r.prim = (unsigned)w[0] & 0x7ff; break;
                case 0x1:
                    for (int k = 0; k < 4; k++)
                        cur.rgba[k] = (uint8_t)w[k];
                    break;
                case 0x2:
                    cur.s = FloatOf((uint32_t)w[0]);
                    cur.t = FloatOf((uint32_t)w[1]);
                    cur.q = FloatOf((uint32_t)w[2]);
                    break;
                case 0x3:
                    cur.u = (unsigned)w[0] & 0x3fff;
                    cur.v = (unsigned)w[1] & 0x3fff;
                    break;
                case 0x4:
                case 0x5:
                    cur.x = w[0] & 0xffff;
                    cur.y = w[1] & 0xffff;
                    if (r.count < 4)
                        r.v[r.count++] = cur;
                    break;
                default: break;
                }
                data += 2;
            }
            else
            {
                /* REGLIST: one 64-bit register value each. */
                const uint64_t value = *data++;
                switch (id)
                {
                case 0x0: r.prim = (unsigned)value & 0x7ff; break;
                case 0x1:
                    for (int k = 0; k < 4; k++)
                        cur.rgba[k] = (uint8_t)(value >> (k * 8));
                    cur.q = FloatOf((uint32_t)(value >> 32));
                    break;
                case 0x2:
                    cur.s = FloatOf((uint32_t)value);
                    cur.t = FloatOf((uint32_t)(value >> 32));
                    break;
                case 0x3:
                    cur.u = (unsigned)value & 0x3fff;
                    cur.v = (unsigned)(value >> 16) & 0x3fff;
                    break;
                case 0x4:
                case 0x5:
                    cur.x = (int)(value & 0xffff);
                    cur.y = (int)((value >> 16) & 0xffff);
                    if (r.count < 4)
                        r.v[r.count++] = cur;
                    break;
                default: break;
                }
            }
        }
    }
    s_records[(uintptr_t)pkt] = r;
}

void RosePan_I3dRecordReg(const void *at, uint64_t reg, uint64_t value)
{
    Record r = {};
    r.kind = REC_REG;
    r.reg = reg;
    r.value = value;
    s_records[(uintptr_t)at] = r;
}

void RosePan_I3dRecordTexRegs(const void *at, const uint64_t *regs)
{
    Record r = {};
    r.kind = REC_TEXREGS;
    r.regs = regs;
    s_records[(uintptr_t)at] = r;
}

void RosePan_I3dBindTexRegs(const uint64_t *regs, const void *tex, const void *clut,
                            unsigned int clut_index)
{
    s_bindings[regs] = Binding{(const I3dTexture *)tex, (const I3dClut *)clut, clut_index};
}

void RosePan_I3dReleaseObject(const void *obj)
{
    for (auto it = s_textures.begin(); it != s_textures.end();)
    {
        if (it->first.tex == obj || it->first.clut == obj)
            it = s_textures.erase(it);
        else
            ++it;
    }
}

void RosePan_I3dBeginFrame(void)
{
    s_records.clear();
    s_bindings.clear();
    s_state = GsState();
}

void RosePan_I3dReplay(const void *start, const void *end)
{
    if (start == nullptr || end == nullptr)
        return;
    bool drew = false;
    auto it = s_records.lower_bound((uintptr_t)start);
    for (; it != s_records.end() && it->first < (uintptr_t)end; ++it)
    {
        const Record &r = it->second;
        switch (r.kind)
        {
        case REC_REG:     ApplyReg(r.reg, r.value); break;
        case REC_TEXREGS: s_state.tex_regs = r.regs; s_state.direct_tex0 = false; break;
        case REC_PRIM:    DrawPrim(r); drew = true; break;
        }
    }
    if (drew)
        RestoreState();
}

void RosePan_I3dRecordVu1(const void *at, const void *data, unsigned int qwc)
{
    (void)at;
    (void)data;
    (void)qwc;
    if (s_vu1_count++ == 0)
        RosePan_LogPrintf("rosepan_i3d: VU1 strips reached; not drawn yet\n");
}

/* ---- the forced test ---- */

void RosePan_I3dForcedTest(i3dContext *ctx)
{
    static int s_enabled = -1;
    static std::vector<uint64_t> s_file;
    static I3dTexture s_tex[2];
    static I3dClut s_clut[2];
    static I3dTexEntry s_table[2];
    /* The two pictures (offsets in S2DMAN.RPK, tm2png --scan): the
     * "Now Loading" sheet (PSMT8, CSM1 CT32 CLUT) and a 512x512 PSMT4. */
    static const unsigned s_offsets[2] = {0x800, 0x11000};

    if (s_enabled < 0)
    {
        const char *env = getenv("ROSEPAN_I3D_TEST");
        s_enabled = env != NULL && env[0] == '1';
        if (!s_enabled)
            return;
        char path[1024];
        FILE *f = NULL;
        if (RosePan_PathResolveData("DATA/MNURPK/S2DMAN.RPK", path, sizeof(path)))
            f = fopen(path, "rb");
        if (f == NULL)
        {
            RosePan_LogPrintf("rosepan_i3d test: DATA/MNURPK/S2DMAN.RPK not found\n");
            s_enabled = 0;
            return;
        }
        fseek(f, 0, SEEK_END);
        const long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        s_file.resize(((size_t)size + 7) / 8);
        const size_t got = fread(s_file.data(), 1, (size_t)size, f);
        fclose(f);
        if (got != (size_t)size)
        {
            s_enabled = 0;
            return;
        }
        for (int i = 0; i < 2; i++)
        {
            u_char *tim2 = (u_char *)s_file.data() + s_offsets[i];
            i3dTexInit(&s_tex[i]);
            i3dClutInit(&s_clut[i]);
            i3dTexSetTim2(&s_tex[i], tim2, 0, 0);
            i3dClutSetTim2(&s_clut[i], tim2, 0, 0);
            s_table[i].pszTexName = NULL;
            s_table[i].pPicture = &s_tex[i].m_handle;
            s_table[i].m_clut = &s_clut[i].m_handle;
            s_table[i].m_0c = 0;
        }
        RosePan_LogPrintf("rosepan_i3d test: S2DMAN.RPK pictures %ux%u and %ux%u\n",
                          s_tex[0].m_width, s_tex[0].m_height, s_tex[1].m_width, s_tex[1].m_height);
    }
    if (!s_enabled)
        return;

    i3dCtxSetTexTable(ctx, s_table, 2);
    I3dLayer *layer = i3dCtxCreateLayer(ctx, 2000.0f);
    /* Picture 0 at full size on the left, picture 1 at half size beside it,
     * both modulated by 0x80 grey (the texel as it is). */
    const int boxes[2][4] = {{16, 96, 16 + 256, 96 + 256}, {288, 96, 288 + 208, 96 + 208}};
    for (int i = 0; i < 2; i++)
    {
        int tex_no = i;
        I3dTexSetDesc desc;
        i3dTexSetDescInit(&desc);
        desc.m_depth = (float)i;
        desc.m_tex_no = &tex_no;
        desc.m_tex_count = 1;
        I3dDrawSet *set = i3dLayerFindDrawSet(layer, &desc);
        i3dLayerSetDrawSet(layer, set);

        const u_int ox = 2048 - kFrameWidth / 2, oy = 2048 - kFrameHeight / 2;
        const u_int w = s_tex[i].m_width, h = s_tex[i].m_height;
        u_long *pkt = i3dLayerBeginSet(layer, 0.0f);
        pkt = i3dPktUseTexRegs(pkt, set, 0);
        pkt = i3dPktSetAlphaMode(pkt, 0x10);
        pkt = i3dPktTexSpriteUV(pkt,
                                i3dPackXYZ((ox + boxes[i][0]) << 4, (oy + boxes[i][1]) << 4, 0),
                                i3dPackXYZ((ox + boxes[i][2]) << 4, (oy + boxes[i][3]) << 4, 0),
                                i3dPackUV(0, 0), i3dPackUV(w << 4, h << 4),
                                i3dPackRGBAQ(1.0f, 0x80, 0x80, 0x80, 0x80));
        i3dLayerEndSet(layer, pkt);
    }
}
