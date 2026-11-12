/* ==========================================================================
 *  rosepan_i3d.cpp -- the native i3d draw module (see rosepan_i3d.h).
 * ======================================================================== */
#include "rosepan/rendering/rosepan_i3d.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <unordered_map>
#include <vector>

#include "i3d/i3dctx.h"
#include "i3d/i3denv.h"
#include "i3d/i3dlayer.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dtex.h"
#include "i3d/i3dview.h"
#include "i3d/i3rdraw.h"
#include "i3d/i3rmodel.h"
#include "i3d/i3rroot.h"
#include "bed/bedpfh.h"
#include "rosepan/io/rosepan_log.h"
#include "rosepan/io/rosepan_paths.h"
#include "rosepan/rendering/rosepan_draw2d.h"
#include "rosepan/rendering/rosepan_i3dmesh.h"
#include "rosepan/rendering/rosepan_renderer.h"

namespace
{
/* RoR's frame buffer (MagSystem: 512x448) and the GS draw offset its draw
 * environment sets (2048 - 256, 2048 - 224; MagViewParam m_offx / m_offy). */
constexpr int kFrameWidth = 512;
constexpr int kFrameHeight = 448;
constexpr int kDefaultOffX = (2048 - kFrameWidth / 2) << 4;
constexpr int kDefaultOffY = (2048 - kFrameHeight / 2) << 4;
/* RoR's frame onto the renderer's (RosePan_RendererGetFrameSize): the 512
 * columns stretch to its width, as the PS2 display stretches them. */
float ScaleX()
{
    float w = 0.0f;
    RosePan_RendererGetFrameSize(&w, nullptr);
    return w / (float)kFrameWidth;
}

float ScaleY()
{
    float h = 0.0f;
    RosePan_RendererGetFrameSize(nullptr, &h);
    return h / (float)kFrameHeight;
}

/* GS registers the module follows. */
constexpr uint64_t kRegTex0 = 0x06;
constexpr uint64_t kRegTex1 = 0x14;
constexpr uint64_t kRegXyOffset = 0x18;
constexpr uint64_t kRegFrame = 0x4c;
constexpr uint64_t kRegScissor = 0x40;
constexpr uint64_t kRegAlpha = 0x42;
constexpr uint64_t kRegRgbaq = 0x01;
constexpr uint64_t kRegClamp = 0x08;
constexpr uint64_t kRegTest = 0x47;
constexpr uint64_t kRegZbuf = 0x4e;

/* The opening state i3dPktOpenChain writes (ALPHA mode 0x10, TEST_1). */
constexpr uint64_t kDefaultAlpha = 0x44;
constexpr uint64_t kDefaultTest = 0x5000d;
/* The display's ZBUF_1 (sceGsSetDefDBuff's: Z at 0xe0, PSMZ16S, ZMSK
 * clear). */
constexpr uint64_t kDefaultZbuf = 0x0a0000e0;
/* ZBUF_1's ZMSK: no depth write. */
constexpr uint64_t kZbufZmsk = 1ull << 32;

/* FRAME_1's FBP of MagScene::makeActorShadow's 256 x 256 shadow target. */
constexpr unsigned kShadowFbp = 0x118;
/* drawShadows' limit (makeActorShadow asserts theCount < 10). */
constexpr int kMaxShadows = 10;

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
    REC_TEXREGS,
    REC_MOVIE,
    REC_MODEL,
    REC_CAMERA,
    REC_STRIP,
    REC_MESH
};

struct Record
{
    RecordKind      kind;
    unsigned        prim;           /* REC_PRIM: the PRIM register */
    bool            has_rgbaq;      /* REC_PRIM: its data writes RGBAQ (else the register's) */
    int             count;          /* REC_PRIM: vertices */
    Vertex          v[4];
    uint64_t        reg, value;     /* REC_REG */
    const uint64_t *regs;           /* REC_TEXREGS */
    int             movie[8];       /* REC_MOVIE: x0, y0, x1, y1, frame w / h, picture w / h */
    size_t          index;          /* REC_MODEL / REC_CAMERA / REC_STRIP / REC_MESH: into
                                       s_models / s_cameras / s_strips / s_meshes */
};

struct ModelDraw
{
    const I3rRoot     *root;
    float              world[16];
    const float       *nodes;
    const I3dTexEntry *tex;
    int                tex_count;
    bool               has_opt;
    I3rBlendOpt        opt;
};

/* A model's one mesh (or batch), filed in its material's draw-set group
 * by i3rDrawModel's part writers, with its material packet's light set
 * (an index into s_light_sets, -1 none) and fog (I3dFogState's ramp and
 * FOGCOL; has_fog false = no FGE). far_z: its view's screen z range is
 * empty at 0 (DrawMesh). */
struct MeshDraw
{
    ModelDraw         model;
    RosePanI3dMeshRef ref;
    int               light;
    bool              far_z;
    bool              has_fog;
    float             fog_base, fog_scale;
    uint32_t          fogcol;
};

struct Camera
{
    float view[16];
    float projection[16];
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
    uint64_t        zbuf = kDefaultZbuf;
    uint64_t        scissor = (uint64_t)(kFrameWidth - 1) << 16 | (uint64_t)(kFrameHeight - 1) << 48;
    int             offx = kDefaultOffX;
    int             offy = kDefaultOffY;
    /* RGBAQ as a register: a primitive whose data has none draws with it
     * (MagLifeNoise's sprites, colour from i3dPktSetReg). */
    uint64_t        rgbaq = 0x3f80000080808080ull;
    /* CLAMP_1 as a register: WMS = WMT = 0 wraps the UVs (MagLifeNoise's
     * grain).  Starts at CLAMP, the edge clamp every other draw has had. */
    uint64_t        clamp = 0x5;
    unsigned        fbp = 0;            /* FRAME_1's FBP (0: the display's, not followed) */
    const uint64_t *tex_regs = nullptr;
    /* A TEX0 written as a plain register (i3dPktSetReg) after the draw set's
     * registers: GS memory the module does not hold -- SprMan::drawFrameCopy's
     * other frame buffer, SPR_TEX_FRAME sprites. */
    bool            direct_tex0 = false;
};

std::map<uintptr_t, Record> s_records;
std::unordered_map<const uint64_t *, Binding> s_bindings;
/* Never destroyed, deliberately (rosepan_log.cpp's loggers): std::exit runs
 * the game's static destructors, and ~CmnTaskMan's task terms release
 * textures into this (TskMsgMan::term -> i3dTexRelease) after this TU's
 * statics are gone. */
std::map<TexKey, const void *> &s_textures = *new std::map<TexKey, const void *>;
std::vector<ModelDraw> s_models;
std::vector<MeshDraw> s_meshes;
/* The frame's light sets, one per created light and view the meshes were
 * filed with (the ROM's lights live in the frame's context tail). */
std::vector<RosePanI3dLightSet> s_light_sets;
std::map<std::pair<const void *, const void *>, int> s_light_index;
std::vector<Camera> s_cameras;
std::vector<std::vector<RosePanI3dVu1Vertex>> s_strips;
GsState s_state;
bool s_logged_offscreen;

/* The projected shadows (MagScene::makeActorShadow / drawActorShadow): the
 * shadow pass the replay is in (the n-th FRAME fbp 0x118 write of the
 * frame), whether its caster episode is open in the renderer and whether
 * the receivers after it may draw (the pass drew a caster). */
int  s_shadow_pass = -1;
bool s_shadow_caster_open;
bool s_shadow_receivers;
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
    return ((float)(x - gs.offx) / 16.0f) * ScaleX();
}

float FrameY(const GsState &gs, int y)
{
    return ((float)(y - gs.offy) / 16.0f) * ScaleY();
}

/* The renderer's GS shadows from the replay state: ALPHA, TEST and ZBUF as
 * they are (the depth write follows ZMSK and TEST's FB_ONLY alpha test), the
 * scissor in the renderer's frame. */
void ApplyState(const GsState &gs)
{
    RosePan_RendererSetGsAlphaRegister(gs.alpha);
    RosePan_RendererSetGsTestRegister(gs.test);
    RosePan_RendererSetGsZbufRegister(gs.zbuf);
    const double sx = ScaleX(), sy = ScaleY();
    const uint64_t x0 = (uint64_t)((double)(gs.scissor & 0x7ff) * sx);
    const uint64_t x1 = (uint64_t)((double)(((gs.scissor >> 16) & 0x7ff) + 1) * sx) - 1;
    const uint64_t y0 = (uint64_t)((double)((gs.scissor >> 32) & 0x7ff) * sy);
    const uint64_t y1 = (uint64_t)((double)(((gs.scissor >> 48) & 0x7ff) + 1) * sy) - 1;
    RosePan_RendererSetGsScissorRegister(x0 | x1 << 16 | y0 << 32 | y1 << 48);
}

void RestoreState()
{
    RosePan_RendererSetGsAlphaRegister(kDefaultAlpha);
    RosePan_RendererSetGsTestRegister(kDefaultTest);
    RosePan_RendererSetGsZbufRegister(kDefaultZbuf);
    float w = 0.0f, h = 0.0f;
    RosePan_RendererGetFrameSize(&w, &h);
    RosePan_RendererSetGsScissorRegister((uint64_t)(w - 1.0f) << 16 | (uint64_t)(h - 1.0f) << 48);
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

/* The texture a primitive with TME draws with (the draw set's TEX
 * registers); false when it cannot be drawn. */
bool PrimTexture(const GsState &gs, bool tme, const void **out_texture, float *out_tw,
                 float *out_th, int *out_decal, int *out_linear)
{
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
        return false;
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
            return false;
        }
        const uint64_t tex0 = gs.tex_regs[2];
        const uint64_t tex1 = gs.tex_regs[4];
        texture = TextureFor(it->second, (int)((tex0 >> 34) & 1));
        if (texture == nullptr)
            return false;
        tw = (float)(1u << ((tex0 >> 26) & 0xf));
        th = (float)(1u << ((tex0 >> 30) & 0xf));
        decal = ((tex0 >> 35) & 3) == 1;
        linear = (int)((tex1 >> 5) & 1);
    }

    *out_texture = texture;
    *out_tw = tw;
    *out_th = th;
    *out_decal = decal;
    *out_linear = linear;
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
    if (!PrimTexture(gs, tme, &texture, &tw, &th, &decal, &linear))
        return;

    float xy[8], uv[8];
    uint8_t rgba[16];
    const int n = r.count;
    for (int i = 0; i < n; i++)
    {
        const Vertex &v = r.v[i];
        /* Flat shading takes the last vertex's colour. */
        Vertex c = iip ? v : r.v[n - 1];
        if (!r.has_rgbaq)
            for (int k = 0; k < 4; k++)
                c.rgba[k] = (uint8_t)(gs.rgbaq >> (k * 8));
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
            const int repeat = (gs.clamp & 0xf) == 0;   /* WMS = WMT = REPEAT */
            RosePan_RendererDrawImageQuadWrap(texture, qxy, quv, rgba[4], rgba[5], rgba[6],
                                              rgba[7], linear, repeat);
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
            RosePan_RendererDrawLine2D(lxy, rgba, ScaleX());
        }
        break;
    case 0:                                     /* point: one frame pixel */
    {
        const float x = xy[0], y = xy[1];
        const float pxy[8] = {x, y, x + ScaleX(), y, x, y + ScaleY(), x + ScaleX(), y + ScaleY()};
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

/* A VU1 strip: its kicked vertices' triangles, in one draw. */
void DrawStrip(const Record &r)
{
    const GsState &gs = s_state;
    if (r.index >= s_strips.size() || !DrawsColour(gs) || (gs.offx == 0 && gs.offy == 0))
        return;
    const std::vector<RosePanI3dVu1Vertex> &v = s_strips[r.index];
    const bool tme = (r.prim & 0x10) != 0;
    const bool iip = (r.prim & 0x08) != 0;
    const void *texture = nullptr;
    float tw = 1.0f, th = 1.0f;
    int decal = 0, linear = 1;
    if (!PrimTexture(gs, tme, &texture, &tw, &th, &decal, &linear))
        return;

    std::vector<float> xy, uv, z;
    std::vector<uint8_t> rgba;
    for (size_t i = 2; i < v.size(); i++)
    {
        if (!v[i].kick)
            continue;
        for (size_t k = i - 2; k <= i; k++)
        {
            const RosePanI3dVu1Vertex &p = v[k];
            const RosePanI3dVu1Vertex &c = iip ? p : v[i];
            xy.push_back((p.x - (float)gs.offx / 16.0f) * ScaleX());
            xy.push_back((p.y - (float)gs.offy / 16.0f) * ScaleY());
            const float q = p.q != 0.0f ? p.q : 1.0f;
            uv.push_back(p.s / q);
            uv.push_back(p.t / q);
            z.push_back(p.z);
            for (int j = 0; j < 4; j++)
                rgba.push_back(decal && j < 3 ? 0x80 : c.rgba[j]);
        }
    }
    if (xy.empty())
        return;
    const int n = (int)(xy.size() / 2);
    ApplyState(gs);
    /* TEST_1: ZTE with ZTST GEQUAL / GREATER (mnuBase's 0x5000d puts its
     * page on the far plane behind the book's models this way). */
    const bool ztest = ((gs.test >> 16) & 1) != 0 && ((gs.test >> 17) & 3) >= 2;
    if (tme)
        RosePan_RendererDrawImageTriangles2DDepth(texture, xy.data(), uv.data(), rgba.data(),
                                                  z.data(), n, linear, ztest);
    else
        RosePan_RendererDrawSolidTriangles2DDepth(xy.data(), rgba.data(), z.data(), n, ztest);
}

/* The smallest power of two exponent covering n. */
int CeilLog2(int n)
{
    int e = 0;
    while ((1 << e) < n && e < 12)
        e++;
    return e;
}

void DrawMovie(const Record &r)
{
    const int *m = r.movie;
    if (m[4] <= 0 || m[5] <= 0 || m[6] <= 0 || m[7] <= 0)
        return;
    const int tw = CeilLog2(m[6]), th = CeilLog2(m[7]);
    const void *texture = RosePan_RendererVideoTexture(tw, th);
    if (texture == nullptr)
        return;
    /* The transfer replaces the frame buffer's pixels: no blending, the
     * whole frame. */
    RestoreState();
    float fw = 0.0f, fh = 0.0f;
    RosePan_RendererGetFrameSize(&fw, &fh);
    const float sx = fw / (float)m[4], sy = fh / (float)m[5];
    const float x0 = m[0] * sx, x1 = m[2] * sx, y0 = m[1] * sy, y1 = m[3] * sy;
    const float s1 = (float)(m[2] - m[0]) / (float)(1 << tw);
    const float t1 = (float)(m[3] - m[1]) / (float)(1 << th);
    const float xy[8] = {x0, y0, x1, y0, x0, y1, x1, y1};
    const float uv[8] = {0.0f, 0.0f, s1, 0.0f, 0.0f, t1, s1, t1};
    RosePan_RendererDrawImageQuad(texture, xy, uv, 0x80, 0x80, 0x80, 0x80, 1);
}

/* A model drawn in an off-screen pass: in the shadow target it is the
 * shadow's caster -- the renderer's shadow map takes it (the episode opens
 * with the pass's first caster and is drawn from the camera it was queued
 * with, the light's); any other target is not drawn. */
bool BeginShadowCaster()
{
    if (s_state.fbp != kShadowFbp || s_shadow_pass < 0 || s_shadow_pass >= kMaxShadows)
        return false;
    if (!s_shadow_caster_open)
    {
        RosePan_RendererShadowBeginCaster();
        s_shadow_caster_open = true;
    }
    return true;
}

/* The pass ends (XYOFFSET back on the display: drawActorShadow's
 * putDrawEnv): the episode closes while the light's camera is still the
 * renderer's, which is what the receivers project through. */
void EndShadowCaster()
{
    if (!s_shadow_caster_open)
        return;
    RosePan_RendererShadowEndCaster();
    s_shadow_caster_open = false;
    s_shadow_receivers = true;
    static bool s_logged;
    if (!s_logged)
    {
        RosePan_LogPrintf("rosepan_i3d: shadow pass %d cast\n", s_shadow_pass);
        s_logged = true;
    }
}

void DrawModel(const Record &r)
{
    if (r.index >= s_models.size())
        return;
    const ModelDraw &m = s_models[r.index];
    /* XYOFFSET 0: an off-screen pass (makeActorShadow's shadow model into
     * the 256 x 256 shadow target, drawCloseUp), not the display. */
    if (s_state.offx == 0 && s_state.offy == 0 && !BeginShadowCaster())
        return;
    /* The mesh path's own state: opaque, depth tested, the whole frame;
     * the depth write still follows the layer's ZBUF. */
    RestoreState();
    RosePan_RendererSetGsZbufRegister(s_state.zbuf);
    RosePan_I3dMeshDraw(m.root, m.world, m.nodes, m.tex, m.tex_count, m.has_opt ? &m.opt : NULL);
}

void DrawMesh(const Record &r)
{
    if (r.index >= s_meshes.size())
        return;
    const MeshDraw &d = s_meshes[r.index];
    const ModelDraw &m = d.model;
    if (s_state.offx == 0 && s_state.offy == 0 && !BeginShadowCaster())
        return;                                 /* an off-screen pass, as DrawModel */
    RestoreState();
    /* MagScene::drawSkyDome's view (zmin = zmax = 0) puts every vertex at GS
     * Z 0, what the buffer clears to, so the ROM's sky leaves the depth
     * buffer as it was. The host camera keeps the real depth, where a dome
     * of radius 3000 would hide whatever lies past it (the fogged distance,
     * cut off by the sky), so the same result is spelled as no write.
     * makeActorShadow's view has that range too; its caster pass has no
     * depth target. */
    RosePan_RendererSetGsZbufRegister(d.far_z ? s_state.zbuf | kZbufZmsk : s_state.zbuf);
    /* VU1's fog: F = clamp(base + scale / w, 0, 255) per vertex (the
     * strip program's MADDq / MAX / MINI), w the clip w -- the camera
     * depth, as the host projection's (i3rHostCamera). */
    if (d.has_fog)
        RosePan_RendererSetFog(1, 0.0f, 255.0f, d.fog_base, d.fog_scale, (int)(d.fogcol & 0xff),
                               (int)((d.fogcol >> 8) & 0xff), (int)((d.fogcol >> 16) & 0xff));
    RosePan_I3dMeshDrawRef(m.root, m.world, m.nodes, m.tex, m.tex_count, m.has_opt ? &m.opt : NULL, &d.ref,
                           d.light >= 0 ? &s_light_sets[(size_t)d.light] : NULL);
    if (d.has_fog)
        RosePan_RendererSetFog(0, 0.0f, 255.0f, 255.0f, 0.0f, 0, 0, 0);
}

/* The index of `light` seen from `eye_dir` in s_light_sets, copied the
 * first time this frame. */
int LightSetFor(const I3dLight *light, const float *eye_dir)
{
    if (light == nullptr)
        return -1;
    const std::pair<const void *, const void *> key(light, eye_dir);
    const auto it = s_light_index.find(key);
    if (it != s_light_index.end())
        return it->second;
    RosePanI3dLightSet s = {};
    memcpy(s.ambient, light->m_ambient, sizeof(s.ambient));
    if (eye_dir != nullptr)
        memcpy(s.eye_dir, eye_dir, sizeof(s.eye_dir));
    if (light->m_parallel != nullptr)
    {
        s.parallel_count = light->m_parallel_count < ROSEPAN_I3D_MAX_PARALLEL ? light->m_parallel_count
                                                                               : ROSEPAN_I3D_MAX_PARALLEL;
        for (int i = 0; i < s.parallel_count; i++)
        {
            memcpy(s.parallel_dir[i], light->m_parallel[i].m_dir, sizeof(s.parallel_dir[i]));
            memcpy(s.parallel_color[i], light->m_parallel[i].m_color, sizeof(s.parallel_color[i]));
        }
    }
    if (light->m_point != nullptr)
    {
        s.point_count = light->m_point_count < ROSEPAN_I3D_MAX_POINT ? light->m_point_count
                                                                     : ROSEPAN_I3D_MAX_POINT;
        for (int i = 0; i < s.point_count; i++)
        {
            memcpy(s.point_pos[i], light->m_point[i].m_pos, sizeof(s.point_pos[i]));
            memcpy(s.point_color[i], light->m_point[i].m_color, sizeof(s.point_color[i]));
        }
    }
    if (light->m_spot != nullptr)
    {
        s.spot_count = light->m_spot_count < ROSEPAN_I3D_MAX_SPOT ? light->m_spot_count : ROSEPAN_I3D_MAX_SPOT;
        for (int i = 0; i < s.spot_count; i++)
        {
            memcpy(s.spot_pos[i], light->m_spot[i].m_pos, sizeof(s.spot_pos[i]));
            memcpy(s.spot_dir[i], light->m_spot[i].m_dir, sizeof(s.spot_dir[i]));
            memcpy(s.spot_color[i], light->m_spot[i].m_color, sizeof(s.spot_color[i]));
            memcpy(s.spot_cone[i], light->m_spot[i].m_cone, sizeof(s.spot_cone[i]));
        }
    }
    s.other_count = (light->m_dir2 != nullptr ? light->m_dir2_count : 0) +
                    (light->m_vec3 != nullptr ? light->m_vec3_count : 0);
    if (light->m_parallel_count > ROSEPAN_I3D_MAX_PARALLEL || light->m_point_count > ROSEPAN_I3D_MAX_POINT ||
        (light->m_spot != nullptr && light->m_spot_count > ROSEPAN_I3D_MAX_SPOT))
    {
        static bool s_logged;
        if (!s_logged)
        {
            RosePan_LogPrintf("rosepan_i3d: a light set has %d parallel / %d point / %d spot lights; %d / %d / %d kept\n",
                              light->m_parallel_count, light->m_point_count, light->m_spot_count,
                              s.parallel_count, s.point_count, s.spot_count);
            s_logged = true;
        }
    }
    const int index = (int)s_light_sets.size();
    s_light_sets.push_back(s);
    s_light_index[key] = index;
    return index;
}

void ApplyReg(uint64_t reg, uint64_t value)
{
    switch (reg)
    {
    case kRegAlpha:    s_state.alpha = value; break;
    case kRegTest:     s_state.test = value; break;
    case kRegZbuf:     s_state.zbuf = value; break;
    case kRegRgbaq:    s_state.rgbaq = value; break;
    case kRegClamp:    s_state.clamp = value; break;
    case kRegScissor:  s_state.scissor = value; break;
    case kRegTex0:     s_state.direct_tex0 = true; break;
    case kRegXyOffset:
        s_state.offx = (int)(value & 0xffff);
        s_state.offy = (int)((value >> 32) & 0xffff);
        if (s_state.offx != 0 || s_state.offy != 0)
            EndShadowCaster();
        break;
    case kRegFrame:
        s_state.fbp = (unsigned)(value & 0x1ff);
        if (s_state.fbp == kShadowFbp)
        {
            /* A new shadow pass; its receivers wait for its caster. */
            EndShadowCaster();
            s_shadow_pass++;
            s_shadow_receivers = false;
        }
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
                    r.has_rgbaq = true;
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
                    r.has_rgbaq = true;
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

void RosePan_I3dRecordMovie(const void *at, int x0, int y0, int x1, int y1,
                            int frame_w, int frame_h, int pic_w, int pic_h)
{
    Record r = {};
    r.kind = REC_MOVIE;
    const int m[8] = {x0, y0, x1, y1, frame_w, frame_h, pic_w, pic_h};
    memcpy(r.movie, m, sizeof(m));
    s_records[(uintptr_t)at] = r;
}

void RosePan_I3dRecordModel(const void *at, const I3rRoot *root, const float *world,
                            const float *nodes, const I3dTexEntry *tex, int tex_count,
                            const I3rBlendOpt *opt)
{
    ModelDraw m = {};
    m.root = root;
    memcpy(m.world, world, sizeof(m.world));
    m.nodes = nodes;
    m.tex = tex;
    m.tex_count = tex_count;
    m.has_opt = opt != NULL;
    if (opt != NULL)
        m.opt = *opt;
    Record r = {};
    r.kind = REC_MODEL;
    r.index = s_models.size();
    s_models.push_back(m);
    s_records[(uintptr_t)at] = r;
}

void RosePan_I3dRecordMesh(const void *at, const I3rRoot *root, const float *world,
                           const float *nodes, const I3dTexEntry *tex, int tex_count,
                           const I3rBlendOpt *opt, const RosePanI3dMeshRef *ref,
                           const I3dLight *light, const I3dView *view, const I3dFogState *fog)
{
    MeshDraw d = {};
    d.light = LightSetFor(light, view != NULL ? view->m_eye_dir : NULL);
    d.far_z = view != NULL && view->m_rect_viewport[2][2] == 0.0f && view->m_rect_viewport[3][2] == 0.0f;
    if (fog != NULL)
    {
        d.has_fog = true;
        d.fog_base = fog->m_ramp_base;
        d.fog_scale = fog->m_ramp_scale;
        d.fogcol = (uint32_t)fog->m_fogcol;
        static bool s_logged;
        if (!s_logged)
        {
            RosePan_LogPrintf("rosepan_i3d: first fogged mesh: F = %.3f + %.3f / w, FOGCOL %06x\n",
                              d.fog_base, d.fog_scale, (unsigned)d.fogcol);
            s_logged = true;
        }
    }
    d.model.root = root;
    memcpy(d.model.world, world, sizeof(d.model.world));
    d.model.nodes = nodes;
    d.model.tex = tex;
    d.model.tex_count = tex_count;
    d.model.has_opt = opt != NULL;
    if (opt != NULL)
        d.model.opt = *opt;
    d.ref = *ref;
    Record r = {};
    r.kind = REC_MESH;
    r.index = s_meshes.size();
    s_meshes.push_back(d);
    s_records[(uintptr_t)at] = r;
}

void RosePan_I3dRecordCamera(const void *at, const float *view, const float *projection)
{
    Camera c;
    memcpy(c.view, view, sizeof(c.view));
    memcpy(c.projection, projection, sizeof(c.projection));
    Record r = {};
    r.kind = REC_CAMERA;
    r.index = s_cameras.size();
    s_cameras.push_back(c);
    s_records[(uintptr_t)at] = r;
}

const void *RosePan_I3dTextureFor(const void *tex, const void *clut, unsigned clut_index, int tcc)
{
    if (tex == nullptr)
        return nullptr;
    return TextureFor(Binding{(const I3dTexture *)tex, (const I3dClut *)clut, clut_index}, tcc);
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
    s_models.clear();
    s_meshes.clear();
    s_light_sets.clear();
    s_light_index.clear();
    s_cameras.clear();
    s_strips.clear();
    s_state = GsState();
    /* The renderer drops its episodes with its frame. */
    s_shadow_pass = -1;
    s_shadow_caster_open = false;
    s_shadow_receivers = false;
}

int RosePan_I3dShadowReceiverReady(void)
{
    return s_shadow_receivers && !s_shadow_caster_open;
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
        case REC_PRIM:
            DrawPrim(r);
            drew = true;
            if (r.has_rgbaq && r.count > 0)
            {
                /* The register keeps the last colour the data wrote. */
                const Vertex &v = r.v[r.count - 1];
                uint32_t q;
                memcpy(&q, &v.q, sizeof(q));
                s_state.rgbaq = (uint64_t)v.rgba[0] | (uint64_t)v.rgba[1] << 8 |
                                (uint64_t)v.rgba[2] << 16 | (uint64_t)v.rgba[3] << 24 |
                                (uint64_t)q << 32;
            }
            break;
        case REC_MOVIE:   DrawMovie(r); drew = true; break;
        case REC_MODEL:   DrawModel(r); drew = true; break;
        case REC_MESH:    DrawMesh(r); drew = true; break;
        case REC_STRIP:   DrawStrip(r); drew = true; break;
        case REC_CAMERA:
            if (r.index < s_cameras.size())
            {
                /* The ROM's projection fills its 512x448 frame; drawn to
                 * the display it widens to the composed aspect (more world
                 * on a wider window, never a stretch). XYOFFSET 0 is an
                 * off-screen pass (the shadow target's light camera), which
                 * keeps its own shape. */
                Camera c = s_cameras[r.index];
                if (s_state.offx != 0 || s_state.offy != 0)
                    RosePan_RendererWidenProjection(c.projection);
                RosePan_RendererSet3DViewProjection(c.view, c.projection);
            }
            break;
        }
    }
    if (drew)
        RestoreState();
}

void RosePan_I3dRecordVu1Strip(const void *at, unsigned int prim, const RosePanI3dVu1Vertex *v,
                               int count)
{
    if (count < 3)
        return;
    Record r = {};
    r.kind = REC_STRIP;
    r.prim = prim & 0x7ff;
    r.index = s_strips.size();
    s_strips.emplace_back(v, v + count);
    s_records[(uintptr_t)at] = r;
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

/* ---- the forced mesh test ---- */

namespace
{
/* `size` bytes of `path` from `offset` into `out` (8-byte aligned). */
bool ReadSpan(FILE *f, long offset, size_t size, std::vector<uint64_t> &out)
{
    out.assign((size + 7) / 8, 0);
    return fseek(f, offset, SEEK_SET) == 0 && fread(out.data(), 1, size, f) == size;
}

/* A turning camera at `dist` from `c`, RoR's world being y-down: row-major
 * view (x right, y up, z forward) and a reverse-Z perspective (depth w at
 * the near plane, 0 at the far one), the forms the renderer takes. */
void OrbitCamera(const float *c, float dist, float angle, float *view, float *proj)
{
    const float eye[3] = {c[0] + sinf(angle) * dist, c[1] - dist * 0.25f, c[2] + cosf(angle) * dist};
    float f[3] = {c[0] - eye[0], c[1] - eye[1], c[2] - eye[2]};
    float len = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    for (float &v : f)
        v /= len;
    const float up[3] = {0.0f, -1.0f, 0.0f};
    float r[3] = {up[1] * f[2] - up[2] * f[1], up[2] * f[0] - up[0] * f[2], up[0] * f[1] - up[1] * f[0]};
    len = sqrtf(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
    for (float &v : r)
        v /= len;
    const float u[3] = {f[1] * r[2] - f[2] * r[1], f[2] * r[0] - f[0] * r[2], f[0] * r[1] - f[1] * r[0]};
    const float v16[16] = {
        r[0], u[0], f[0], 0.0f,
        r[1], u[1], f[1], 0.0f,
        r[2], u[2], f[2], 0.0f,
        -(r[0] * eye[0] + r[1] * eye[1] + r[2] * eye[2]),
        -(u[0] * eye[0] + u[1] * eye[1] + u[2] * eye[2]),
        -(f[0] * eye[0] + f[1] * eye[1] + f[2] * eye[2]), 1.0f};
    memcpy(view, v16, sizeof(v16));
    const float n = dist * 0.05f, fa = dist * 4.0f;
    const float sy = 1.0f / tanf(0.5f * 0.75f);                /* 43 degrees vertical */
    const float sx = sy * 3.0f / 4.0f;      /* the frame shows at 4:3; the replay widens it */
    const float p16[16] = {
        sx, 0.0f, 0.0f, 0.0f,
        0.0f, sy, 0.0f, 0.0f,
        0.0f, 0.0f, -n / (fa - n), 1.0f,
        0.0f, 0.0f, fa * n / (fa - n), 0.0f};
    memcpy(proj, p16, sizeof(p16));
}
}

void RosePan_I3dMeshForcedTest(i3dContext *ctx)
{
    static int s_enabled = -1;
    static std::vector<uint64_t> s_pack;
    static std::vector<I3dTexture> s_tex;
    static std::vector<I3dClut> s_clut;
    static std::vector<I3dTexEntry> s_table;
    static I3rRoot *s_root;
    static unsigned s_frame;

    if (s_enabled < 0)
    {
        const char *env = getenv("ROSEPAN_I3D_MESH_TEST");
        s_enabled = env != NULL && env[0] == '1';
        if (!s_enabled)
            return;
        s_enabled = 0;
        char path[1024];
        FILE *f = NULL;
        if (RosePan_PathResolveData("DATA/CH/CHARA.RPK", path, sizeof(path)))
            f = fopen(path, "rb");
        if (f == NULL)
        {
            RosePan_LogPrintf("rosepan_i3d mesh test: DATA/CH/CHARA.RPK not found\n");
            return;
        }
        /* CHARA.RPK -> its first chapter pack -> that pack's first model:
         * the headers' first 2048 bytes hold the offset tables. */
        std::vector<uint64_t> head;
        bool ok = ReadSpan(f, 0, 2048, head);
        const long chap = ok ? BedPfh(head.data()).getFileOffset(0) : -1;
        ok = ok && chap > 0 && ReadSpan(f, chap, 2048, head);
        BedPfh chap_pfh(head.data());
        const long model = ok ? chap + chap_pfh.getFileOffset(0) : -1;
        const int model_size = ok ? chap_pfh.getFileSize(0) : -1;
        ok = ok && model_size > 0 && ReadSpan(f, model, (size_t)model_size, s_pack);
        fclose(f);
        if (!ok)
        {
            RosePan_LogPrintf("rosepan_i3d mesh test: CHARA.RPK's first model not read\n");
            return;
        }
        /* The model's pack: its .mdl (the TIM2 pack, the i3r, an optional
         * collision file) and its .MOT. */
        BedPfh entry(s_pack.data());
        if (!entry.checkID() || entry.getNumFile() < 1)
            return;
        BedPfh pack(entry.getFile(0));
        if (!pack.checkID() || pack.getNumFile() < 2)
            return;
        s_root = (I3rRoot *)pack.getFile(1);
        if (I3rRoot__CheckIdString(s_root) != I3D_OK || i3rRelocate(s_root) != I3D_OK)
        {
            RosePan_LogPrintf("rosepan_i3d mesh test: the model's i3r did not relocate\n");
            s_root = NULL;
            return;
        }
        BedPfh tims(pack.getFile(0));
        const int n = i3rGetNumTex(s_root);
        s_tex.resize((size_t)n);
        s_clut.resize((size_t)n);
        s_table.resize((size_t)n);
        int found = 0;
        for (int i = 0; i < n; i++)
        {
            const char *name = i3rGetTexName(s_root, i);
            const int k = tims.checkID() && name != NULL ? tims.getFileIndex(name) : -1;
            I3dTexEntry &e = s_table[(size_t)i];
            e.pszTexName = name;
            e.pPicture = NULL;
            e.m_clut = NULL;
            e.m_0c = 0;
            if (k < 0)
            {
                RosePan_LogPrintf("rosepan_i3d mesh test: texture %s not in the model's TIM2 pack\n",
                                  name != NULL ? name : "(null)");
                continue;
            }
            i3dTexInit(&s_tex[(size_t)i]);
            i3dClutInit(&s_clut[(size_t)i]);
            i3dTexSetTim2(&s_tex[(size_t)i], tims.getFile(k), 0, 0);
            i3dClutSetTim2(&s_clut[(size_t)i], tims.getFile(k), 0, 0);
            e.pPicture = &s_tex[(size_t)i].m_handle;
            e.m_clut = &s_clut[(size_t)i].m_handle;
            found++;
        }
        i3rAssignTex(s_root, s_table.data(), n);
        RosePan_LogPrintf("rosepan_i3d mesh test: %s, %d nodes, %d of %d textures\n",
                          chap_pfh.getFileName(0), i3rGetNumNode(s_root), found, n);
        s_enabled = 1;
    }
    if (!s_enabled || s_root == NULL)
        return;

    const I3rRootData *rd = (const I3rRootData *)s_root->m_item.getData();
    float view[16], proj[16];
    OrbitCamera(rd->m_clip_center, rd->m_clip_radius * 3.2f, (float)s_frame++ * 0.02f, view, proj);
    static const float s_world[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    I3dLayer *layer = i3dCtxCreateLayer(ctx, 2000.0f);
    u_long *pkt = i3dLayerBeginPacket(layer);
    u_long *p = i3dPktDmaTag(pkt, 2, 0x10, NULL, 0, 0);
    RosePan_I3dRecordCamera(p, view, proj);
    RosePan_I3dRecordModel(p + 2, s_root, s_world, NULL, s_table.data(), (int)s_table.size(), NULL);
    p[0] = p[1] = p[2] = p[3] = 0;
    i3dLayerEndPacket(layer, p + 4);
}
