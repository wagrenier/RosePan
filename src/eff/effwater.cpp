/* ==========================================================================
 *  eff/effwater.cpp
 *
 *  EffWater1 (0x17a918..0x17afd8) and EffWater3 (0x17afd8..0x17c0f0): the
 *  scrolling water model and the rain. See eff/effwater.h.
 * ======================================================================== */
#include "eff/effwater.h"

#include <stdio.h>
#include <string.h>

#include "bed/bedpfh.h"
#include "dog/dogcamera.h"
#include "dog/dogmodel.h"
#include "dog/dogtexinfo.h"
#include "eff/effman.h"
#include "game/gamfile.h"
#include "i3d/i3dlayer.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dview.h"
#include "i3d/i3dvu1pkt.h"
#include "i3d/i3rmodel.h"
#include "i3d/i3rroot.h"
#include "i3d/i3rtype.h"
#include "mag/magscene.h"
#include "sdk/libgraph.h"
#include "utl/utlmath.h"
#include "utl/utlmisc.h"

static const float EFF_HALF_PI = 0x1.921fb2p+0f;    /* 0x3fc90fd9, the ROM's pi / 2 */

/* EffWater3s alive (0x298e1c): the first one's init greys the fog. The
 * destructor counts down even after a failed init (the ROM's). */
static int s_count;

/* The drop's texture rectangle before drawCell's first sprite moves its
 * u range to 0.5..1 (data 0x2f3070). */
static const float s_dropRect[4] = { 0.0f, 0.25f, 0.5f, 1.0f };

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017a918

EffWater1::EffWater1()
{
    // callcheck: Eff0 (the base constructor, compiler-made)
    m_root = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017a94c

EffWater1::~EffWater1()
{
    // callcheck: ~Eff0, operator_delete (the base destructor and the deleting variant, compiler-made)
    if (m_root != NULL)
        g_utlMemory.free(m_root);
}

// ──────────────────────────────────────────────────────────────────────
// Reads data/eff/EFFWATER1.RPK whole, copies its file 0 (the model) into
// a block of its own, relocates it, adds the pack's textures to the
// scene's and assigns them; then the param's placement, colour and
// scroll speeds.
// callcheck: FileReader, ~FileReader, FileReadReq, BedPfh, dbgAssertFailed (the locals' constructors and destructors; BedPfh::getFile's inline asserts, lines 85 / 86)
// ROR 0x0017a9c8

int EffWater1::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;

    FileReader reader;
    BedPfh pfh;
    pfh.clear();
    MagScene *scene = g_effScene;
    DogTexInfo *tex = &scene->m_tex_info;
    char name[64];
    sprintf(name, "data/eff/EFFWATER1.RPK");
    int size = fileGetSize(name);
    void *file = g_utlMemory.alloc(size, -1);
    memset(file, 0, size);

    FileReadReq req;
    req.m_size = size;
    req.m_close = 1;
    req.m_08 = 1;
    req.m_whence = 0;
    req.m_offset = 0;
    req.m_buf = file;
    req.m_step = 0;
    req.m_name = name;
    reader.read(&req, 0);

    pfh = BedPfh(file);
    int model_size = pfh.getFileSize(0);
    m_root = (I3rRoot *)g_utlMemory.alloc(model_size, -1);
    memcpy(m_root, pfh.getFile(0), model_size);
    i3rRelocate(m_root);
    tex->addPfh(&pfh, m_root, 1, 0);
    tex->setTexTable(g_effCtx);
    i3rAssignTex(m_root, tex->m_tex_info, tex->m_num_tex);
    g_utlMemory.free(file);

    m_pos[0] = param->m_water1.m_pos[0];
    m_pos[1] = param->m_water1.m_pos[1];
    m_pos[2] = param->m_water1.m_pos[2];
    m_pos[3] = 1.0f;
    m_rot = param->m_water1.m_rot;
    m_scale = param->m_water1.m_scale;
    for (int i = 0; i < 4; i++)
        m_color[i] = param->m_water1.m_color[i];
    m_uspeed = param->m_water1.m_uspeed;
    m_vspeed = param->m_water1.m_vspeed;
    m_uv[0] = 0.0f;
    m_uv[2] = 1.0f;
    m_fade = 0;
    m_uv[1] = 0.0f;
    m_uv[3] = 1.0f;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Scrolls the texture (u / v by speed / 30, kept within -1..1) and hands
// the offsets to every node's first material, bumping the node's change
// count. Never over.
// ROR 0x0017ac5c

int EffWater1::liveMain()
{
    I3rItem *item = &m_root->m_item;
    int n = i3rGetNumChild52(item);

    float u = m_uv[0] + m_uspeed / 30.0f;
    m_uv[0] = u;
    if (u >= 1.0f || u <= -1.0f)
        m_uv[0] = u - (float)(int)u;
    float v = m_uv[1] + m_vspeed / 30.0f;
    m_uv[1] = v;
    if (v >= 1.0f || v <= -1.0f)
        m_uv[1] = v - (float)(int)v;

    for (int i = 0; i < n; i++)
    {
        I3rItem *node = i3rGetChild52(item, i);
        i3rSetMatEntity0(i3rGetChild25(node, 0), m_uv);
        u_short *changes = (u_short *)((u_char *)node->getData() + 8);
        (*changes)++;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The visibility change's 5 frames: hidden counts m_fade up to 5, shown
// down to 0; then the activation is over.
// ROR 0x0017adc0

void EffWater1::liveActivateExec()
{
    if (m_flags & EFF_FLAG_HIDDEN)
    {
        if (++m_fade < 5)
            return;
    }
    else
    {
        if (--m_fade > 0)
            return;
    }
    m_flags &= ~EFF_FLAG_ACTIVATE;
}

// ──────────────────────────────────────────────────────────────────────
// Unless hidden for good: every node's display control pointed at a
// record with the param's colour, then the model drawn turned by m_rot
// about y, scaled by m_scale (x / z times m_fade / 5 during an
// activation), at m_pos.
// ROR 0x0017ae04

void EffWater1::renderPreMain(RENDERPARAM *param)
{
    if ((m_flags & (EFF_FLAG_HIDDEN | EFF_FLAG_ACTIVATE)) == EFF_FLAG_HIDDEN)
        return;

    /* PC: zeroed whole (the ROM leaves 0x26..0x2f as the stack had them).
     * The nodes keep pointing at it after the draw, as in the ROM. */
    DogDispControl disp;
    memset(&disp, 0, sizeof(disp));
    for (int i = 0; i < 4; i++)
        disp.m_10[i] = m_color[i];
    disp.m_20 = 0x3f800000;     /* the float 1.0's bits */
    disp.m_25 = 1;
    disp.m_24 = 1;

    I3rItem *item = &m_root->m_item;
    int n = i3rGetNumChild52_3(item);
    for (int i = 0; i < n; i++)
    {
        I3rItem *slot = i3rGetSlot2A(i3rGetChild52_3(item, i));
        /* PORT: the ROM stores the pointer; the host field holds a far
         * pointer (the record is not in the file). */
        i3rFarPtrSet(&slot->m_data, &disp);
    }

    float scale[4] = { m_scale, m_scale, m_scale, 1.0f };
    if (m_flags & EFF_FLAG_ACTIVATE)
    {
        scale[0] = (scale[0] * (float)m_fade) / 5.0f;
        scale[2] = scale[0];
    }
    sceVu0FMATRIX m;
    i3dMatUnit(m);
    i3dMatRotY(m_rot, m, m);
    i3dMatScale(m, m, scale);
    m[3][0] = m_pos[0];
    m[3][1] = m_pos[1];
    m[3][2] = m_pos[2];
    m[3][3] = m_pos[3];
    utlDrawI3r(param->m_cur, m_root, NULL, m, NULL);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017afd8

EffWater3::EffWater3()
{
    // callcheck: Eff0 (the base constructor, compiler-made)
    m_num_cell = 0;
    m_cell = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017b010

EffWater3::~EffWater3()
{
    // callcheck: ~Eff0, operator_delete (the base destructor and the deleting variant, compiler-made)
    if (m_cell != NULL)
    {
        g_utlMemory.free(m_cell);
        m_cell = NULL;
    }
    s_count--;
}

// ──────────────────────────────────────────────────────────────────────
// The mode's sizes, the box sorted (x / z extent, m_top / m_bottom), the
// grid of cells (at least two cell sizes along an axis to split it), each
// cell's drop layers spread over the box's height and ripple layers over
// 0..-3; the first EffWater3 alive moves the fog toward grey 0.15 and the
// mode's distances (relative to MagScene::m_fog[3], at once).
// ROR 0x0017b0a4

int EffWater3::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;

    float cell_size;
    if (param->m_water3.m_mode == 0)
    {
        m_alpha = 0x1.999998p-3f;   /* 0x3e4ccccc */
        m_size = 100.0f;
        m_speed = 30.0f;
        m_height = 60.0f;
        m_bright = 0.5f;
        m_ripple_size = 15.0f;
        m_ripple_spread = 1.5f;
        m_splash_alpha = 1.0f;
        m_splash_scale = 1.75f;
        m_width = 1.0f;
        cell_size = 500.0f;
        m_ripple_alpha = 0.0f;
    }
    else
    {
        m_ripple_size = 15.0f;
        m_speed = 60.0f;
        m_width = 2.0f;
        m_height = 120.0f;
        m_alpha = 0x1.999998p-3f;
        m_ripple_spread = 1.5f;
        m_splash_alpha = 1.0f;
        m_splash_scale = 1.75f;
        m_size = 120.0f;
        m_bright = 1.0f;
        m_ripple_alpha = 1.0f;
        cell_size = 200.0f;
    }

    const float (*box)[3] = param->m_water3.m_box;
    float x0 = box[0][0];
    float x1 = box[1][0];
    if (x1 < x0)
    {
        x0 = box[1][0];
        x1 = box[0][0];
    }
    float z0 = box[0][2];
    float z1 = box[1][2];
    if (z1 < z0)
    {
        z0 = box[1][2];
        z1 = box[0][2];
    }
    if (box[0][1] <= box[1][1])
    {
        m_top = box[0][1];
        m_bottom = box[1][1];
    }
    else
    {
        m_top = box[1][1];
        m_bottom = box[0][1];
    }
    const float w = x1 - x0;
    const float d = z1 - z0;
    int nx = 1;
    if (cell_size + cell_size <= w)
        nx = (int)(w / cell_size);
    int nz = 1;
    if (cell_size + cell_size <= d)
        nz = (int)(d / cell_size);

    m_num_cell = nx * nz;
    EffWater3Cell *c = (EffWater3Cell *)g_utlMemory.alloc(nx * nz * (int)sizeof(EffWater3Cell), -1);
    m_cell = c;
    for (int iz = 0; iz < nz; iz++)
    {
        for (int ix = 0; ix < nx; ix++, c++)
        {
            c->m_rect[0] = x0 + (w * (float)ix) / (float)nx;
            c->m_rect[1] = z0 + (d * (float)iz) / (float)nz;
            c->m_rect[2] = x0 + (w * (float)(ix + 1)) / (float)nx;
            c->m_rect[3] = z0 + (d * (float)(iz + 1)) / (float)nz;
            for (int j = 0; j < 8; j++)
            {
                resetDrop(c->m_drop[j], c);
                c->m_drop[j][1] += (float)j * (m_bottom - m_top) * 0.125f;
            }
            for (int j = 0; j < 4; j++)
            {
                resetRipple(c->m_ripple[j], c);
                c->m_ripple[j][1] += (float)j * -3.0f * 0.25f;
            }
        }
    }

    if (s_count == 0)
    {
        const I3dFog &to = g_effScene->m_fog[3];
        float near_dist, far_dist, far_amount;
        if (param->m_water3.m_mode == 0)
        {
            far_amount = 0x1.999998p-2f;    /* 0x3ecccccc */
            near_dist = 200.0f;
            far_dist = 400.0f;
        }
        else
        {
            far_amount = 0.5f;
            near_dist = 600.0f;
            far_dist = 1500.0f;
        }
        const float grey = 0x1.333332p-3f;  /* 0x3e199999 */
        I3dFog delta;
        delta.m_color[0] = grey - to.m_color[0];
        delta.m_color[1] = grey - to.m_color[1];
        delta.m_color[2] = grey - to.m_color[2];
        delta.m_color[3] = 0.0f;
        delta.m_param[0] = near_dist - to.m_param[0];
        delta.m_param[1] = 0.0f - to.m_param[1];
        delta.m_param[2] = far_dist - to.m_param[2];
        delta.m_param[3] = far_amount - to.m_param[3];
        g_effScene->addFog(&delta, 0);
    }
    s_count++;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Drops fall m_speed a frame and start again above the box once past
// m_bottom; ripples run 0.5 a frame toward -3 and start again past it.
// Never over.
// ROR 0x0017b5e0

int EffWater3::liveMain()
{
    EffWater3Cell *c = m_cell;
    for (int i = 0; i < m_num_cell; i++, c++)
    {
        for (int j = 0; j < 8; j++)
        {
            float *drop = c->m_drop[j];
            drop[1] += m_speed;
            if (drop[1] > m_bottom)
                resetDrop(drop, c);
        }
        for (int j = 0; j < 4; j++)
        {
            float *ripple = c->m_ripple[j];
            ripple[1] += -0.5f;
            if (ripple[1] < -3.0f)
                resetRipple(ripple, c);
        }
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Texture 0 with ALPHA_1 0x44 (Cs blended by As over Cd), one VU1 sprite
// batch for the cells in view.
// ROR 0x0017b6d4

u_long *EffWater3::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    u_long *p = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x44);
    p = i3dPktCallView(p, param->m_cur);
    p = i3dPktRefTexRegs(p, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), 0));
    p = i3dPktBeginVu1Sprite(p, param->m_cur, 1);
    I3dView *view = i3dLayerGetView(param->m_cur);
    const EffWater3Cell *c = m_cell;
    for (int i = 0; i < m_num_cell; i++, c++)
    {
        if (isCellVisible(c, view))
            p = drawCell(p, c);
    }
    return i3dPktEndVu1Sprite(p);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017b7cc

void EffWater3::resetDrop(float *drop, const EffWater3Cell *cell)
{
    drop[0] = m_size * ((float)effRand() / 255.0f);
    drop[1] = m_top - m_height;
    drop[2] = m_size * ((float)effRand() / 255.0f);
    drop[3] = m_size + m_size * 0.5f * ((float)effRand() / 255.0f);
    const float w = cell->m_rect[2] - cell->m_rect[0];
    while (w <= drop[0])
        drop[0] -= w;
    const float d = cell->m_rect[3] - cell->m_rect[1];
    while (d <= drop[2])
        drop[2] -= d;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017b918

void EffWater3::resetRipple(float *ripple, const EffWater3Cell *cell)
{
    const float size = m_size * m_ripple_spread;
    const u_char r = effRand();
    ripple[1] = 0.0f;
    ripple[0] = size * ((float)r / 255.0f);
    ripple[2] = size * ((float)effRand() / 255.0f);
    ripple[3] = size + size * 0.5f * ((float)effRand() / 255.0f);
    const float w = cell->m_rect[2] - cell->m_rect[0];
    while (w <= ripple[0])
        ripple[0] -= w;
    const float d = cell->m_rect[3] - cell->m_rect[1];
    while (d <= ripple[2])
        ripple[2] -= d;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017ba54

bool EffWater3::isCellVisible(const EffWater3Cell *cell, I3dView *view)
{
    const float scale[4] = {
        (cell->m_rect[2] - cell->m_rect[0]) * 0.5f,
        (m_bottom - m_top) * 0.5f,
        (cell->m_rect[3] - cell->m_rect[1]) * 0.5f,
        1.0f,
    };
    sceVu0FMATRIX m;
    i3dMatUnit(m);
    i3dMatScale(m, m, scale);
    m[3][0] = cell->m_rect[0] + scale[0];
    m[3][1] = m_top + scale[1];
    m[3][2] = cell->m_rect[1] + scale[2];
    return (u_int)(i3dViewTestBox(view, m) - 1) < 2;
}

// ──────────────────────────────────────────────────────────────────────
// Each layer is one sprite tiled over the cell from its offset by its
// spacing (a row that runs past x1 starts the next one shifted back by
// the cell's width). The sprite's position is the matrix's row 3 (the
// VU1 matrix takes three rows), as in the ROM.
//  - drops: the camera matrix made upright, scaled m_width x m_height,
//    m_height / 2 below the drop's y; the rectangle (u 0.5..1, v 0.25..1)
//    is sent again every 32 sprites;
//  - ripples: flat (turned pi / 2 about x) on m_bottom, m_ripple_size x
//    y / -3, alpha m_ripple_alpha, v 0..0.125;
//  - splashes: facing the camera 1 above m_bottom, m_splash_scale times a
//    ripple's size, alpha m_splash_alpha, v 0.125..0.25.
// The colour is m_bright grey with alpha m_alpha, times the ambient light.
// ROR 0x0017bb50

u_long *EffWater3::drawCell(u_long *pkt, const EffWater3Cell *cell)
{
    float color[4] = { m_bright, m_bright, m_bright, m_alpha };
    effAmbientColor(color);

    DogCamera *cam = &g_effScene->m_camera_entity;
    cam->callTransform();
    sceVu0FMATRIX m;
    memcpy(m, cam->m_matrix, sizeof(m));
    effMatMakeUpright(m);
    float scale[4] = { m_width, m_height, 1.0f, 1.0f };
    i3dMatScale(m, m, scale);
    float rect[4];
    memcpy(rect, s_dropRect, sizeof(rect));

    float *pos = m[3];
    u_int count = (u_int)-1;
    for (int j = 0; j < 8; j++)
    {
        const float *drop = cell->m_drop[j];
        pos[0] = drop[0] + cell->m_rect[0];
        pos[1] = drop[1] + m_height * 0.5f;
        pos[2] = drop[2] + cell->m_rect[1];
        while (pos[2] < cell->m_rect[3])
        {
            while (pos[0] < cell->m_rect[2])
            {
                if ((++count & 0x1f) == 0)
                {
                    rect[0] = 0.5f;
                    rect[2] = 1.0f;
                    pkt = i3dPktVu1Rect(pkt, rect);
                }
                pkt = i3dPktVu1Matrix(pkt, m);
                pkt = i3dPktVu1Sprite(pkt, pos, color);
                pos[0] += drop[3];
            }
            pos[0] -= cell->m_rect[2] - cell->m_rect[0];
            pos[2] += drop[3];
        }
    }

    if (m_ripple_alpha > 0.0f)
    {
        color[3] = m_ripple_alpha;
        rect[0] = 0.0f;
        rect[1] = 0.0f;
        rect[2] = 1.0f;
        rect[3] = 0.125f;
        pkt = i3dPktVu1Rect(pkt, rect);
        for (int j = 0; j < 4; j++)
        {
            const float *ripple = cell->m_ripple[j];
            i3dMatUnit(m);
            i3dMatRotX(EFF_HALF_PI, m, m);
            const float s = (m_ripple_size * ripple[1]) / -3.0f;
            scale[0] = s;
            scale[1] = s;
            i3dMatScale(m, m, scale);
            pos[0] = ripple[0] + cell->m_rect[0];
            pos[1] = ripple[1] + m_bottom;
            pos[2] = ripple[2] + cell->m_rect[1];
            while (pos[2] < cell->m_rect[3])
            {
                while (pos[0] < cell->m_rect[2])
                {
                    pkt = i3dPktVu1Matrix(pkt, m);
                    pkt = i3dPktVu1Sprite(pkt, pos, color);
                    pos[0] += ripple[3];
                }
                pos[0] -= cell->m_rect[2] - cell->m_rect[0];
                pos[2] += ripple[3];
            }
        }
    }

    if (m_splash_alpha > 0.0f)
    {
        color[3] = m_splash_alpha;
        rect[0] = 0.0f;
        rect[1] = 0.125f;
        rect[2] = 1.0f;
        rect[3] = 0.25f;
        pkt = i3dPktVu1Rect(pkt, rect);
        for (int j = 0; j < 4; j++)
        {
            const float *ripple = cell->m_ripple[j];
            const float s = (m_splash_scale * m_ripple_size * ripple[1]) / -3.0f;
            scale[0] = s;
            scale[1] = s;
            cam->callTransform();
            i3dMatScale(m, cam->m_matrix, scale);
            pos[0] = ripple[0] + cell->m_rect[0];
            pos[1] = (ripple[1] + m_bottom) - 1.0f;
            pos[2] = ripple[2] + cell->m_rect[1];
            while (pos[2] < cell->m_rect[3])
            {
                while (pos[0] < cell->m_rect[2])
                {
                    pkt = i3dPktVu1Matrix(pkt, m);
                    pkt = i3dPktVu1Sprite(pkt, pos, color);
                    pos[0] += ripple[3];
                }
                pos[0] -= cell->m_rect[2] - cell->m_rect[0];
                pos[2] += ripple[3];
            }
        }
    }
    return pkt;
}
