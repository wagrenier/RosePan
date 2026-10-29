/* ==========================================================================
 *  mag/maglifefadekey.cpp
 *
 *  The keyhole fade (RoR's "mag/maglifefadekey.cpp", pinned
 *  0x12f298..0x12fca8): MagLifeFadeKeyHole, a MagLife that draws one
 *  picture of data/evntex/EVNTEX.RPK over the screen, its colour and its
 *  hole's alpha ramped by the fade state and zoomed by m_magnify. See
 *  maglifefadekey.h.
 * ======================================================================== */
#include "mag/maglifefadekey.h"

#include <stdint.h>
#include <string.h>

#include "bed/bedpfh.h"
#include "common/ror_assert.h"
#include "dog/dogtexinfo.h"
#include "game/gamfile.h"
#include "i3d/i3dlayer.h"
#include "i3d/i3dpacket.h"
#include "lib/unassigned.h"
#include "sdk/libgraph.h"
#include "utl/utlmisc.h"

static const char s_file[] = "mag/maglifefadekey.cpp";
static const char s_pack[] = "data/evntex/EVNTEX.RPK";
/* The name every keyhole picture is added under (0x2ed278). */
static const char s_tex_name[] = "kagiana_tcc";

/* Rounds a file size up to whole 2048-byte sectors (the ROM's signed
 * (x + 0x7ff) >> 11 << 11). */
static inline int roundSector(int size)
{
    return (size + 0x7ff) / 0x800 * 0x800;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0012f298

MagLifeFadeKeyHole::MagLifeFadeKeyHole()
{
    /* MagLife0's words, then MagLife's and the hole's (the ROM's order). */
    m_level = 1;
    m_state = 0;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_50 = 0.0f;
    m_tex_no = -1;
    m_magnify = 1.0f;
    m_04 = -1;
    m_state = 0;
    m_hide = 0;
    m_scene = NULL;
    m_frame = 0;
    m_time = 0;
    m_54 = 0;
    /* PC: the ROM leaves the ramp words as the scene's memory had them
     * (reset and setMagnify set them); the host starts them at 1. */
    m_magnify_from = 1.0f;
    m_magnify_to = 1.0f;
}

// ──────────────────────────────────────────────────────────────────────
// Reads EVNTEX.RPK's 2048-byte header, then file fileno into a sector-
// rounded heap block, adds it to tex as kagiana_tcc (a copy: the block is
// freed after).
// callcheck: dbgAssertFailed, BedPfh, FileReadReq, FileReader, ~FileReader (the assert handler; the pfh constructor; inline in game/gamfile.h: the two constructors and the destructor)
// ROR 0x0012f2f0

int MagLifeFadeKeyHole::loadTexture(DogTexInfo *tex, int fileno)
{
    /* The ROM aligns a 0x840-byte stack buffer to 64 (up, +64). */
    u_char buf[0x800 + 0x40];
    memset(buf, 0, sizeof(buf));
    FileReader reader;
    FileReadReq req;
    void *head = (void *)((((uintptr_t)buf) >> 6 << 6) + 64);
    req.m_buf = head;
    req.m_size = 0x800;
    req.m_name = s_pack;
    reader.read(&req, 0);

    BedPfh pfh_head(head);
    ROR_ASSERT(pfh_head.getFileOffset(0) == (2048)); /* 77 */
    int filelen = pfh_head.getFileSize(fileno);
    ROR_ASSERT(filelen > 0); /* 81 */
    /* PC: the ROM goes on with the bad size; the host loads nothing. */
    if (filelen <= 0)
        return 0;
    int size = roundSector(filelen);
    void *p = g_utlMemory.allocDebug(size, s_file, 86, 0, 1);
    ROR_ASSERT(p != NULL); /* 87 */
    if (p == NULL)
        return 0;
    req.m_offset = pfh_head.getFileOffset(fileno);
    req.m_buf = p;
    req.m_close = 1;
    req.m_whence = 0;
    req.m_size = size;
    reader.read(&req, 0);

    m_tex_no = tex->addData(s_tex_name, p, DOG_TEX_COPY, -1);
    g_utlMemory.free(p);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// As loadTexture, with the file found by theName.
// callcheck: dbgAssertFailed, BedPfh, FileReadReq, FileReader, ~FileReader (the assert handler; the pfh constructor; inline in game/gamfile.h: the two constructors and the destructor)
// ROR 0x0012f518

int MagLifeFadeKeyHole::loadTextureByName(DogTexInfo *tex, const char *theName)
{
    ROR_ASSERT(theName); /* 110 */
    u_char buf[0x800 + 0x40];
    memset(buf, 0, sizeof(buf));
    FileReader reader;
    FileReadReq req;
    void *head = (void *)((((uintptr_t)buf) >> 6 << 6) + 64);
    req.m_buf = head;
    req.m_size = 0x800;
    req.m_name = s_pack;
    reader.read(&req, 0);

    BedPfh pfh_head(head);
    ROR_ASSERT(pfh_head.getFileOffset(0) == (2048)); /* 132 */
    int fileno = pfh_head.getFileIndex(theName);
    ROR_ASSERT(fileno >= 0); /* 135 */
    /* PC: the ROM reads the tables at -1; the host loads nothing. */
    if (fileno < 0)
        return 0;
    int filelen = pfh_head.getFileSize(fileno);
    ROR_ASSERT(filelen > 0); /* 139 */
    if (filelen <= 0)
        return 0;
    int size = roundSector(filelen);
    void *p = g_utlMemory.allocDebug(size, s_file, 144, 0, 1);
    ROR_ASSERT(p != NULL); /* 145 */
    if (p == NULL)
        return 0;
    req.m_offset = pfh_head.getFileOffset(fileno);
    req.m_buf = p;
    req.m_close = 1;
    req.m_whence = 0;
    req.m_size = size;
    reader.read(&req, 0);

    m_tex_no = tex->addData(s_tex_name, p, DOG_TEX_COPY, -1);
    g_utlMemory.free(p);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
// ROR 0x0012f790

void MagLifeFadeKeyHole::releaseTexture(DogTexInfo *theTexInfo)
{
    ROR_ASSERT(theTexInfo); /* 168 */
    if (m_tex_no >= 0) {
        theTexInfo->release(m_tex_no);
        m_tex_no = -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0012f7f4

void MagLifeFadeKeyHole::reset(int state, int time)
{
    m_state = state;
    m_magnify_from = 1.0f;
    m_time = time;
    m_04 = 0;
    m_08 = time;
    m_0c = 0;
    m_frame = 0;
    m_magnify = 1.0f;
    m_magnify_to = 1.0f;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0012f824

void MagLifeFadeKeyHole::start(const MagFadeParam *p)
{
    m_04 = 0;
    m_state = p->m_mode;
    m_0c = 0;
    m_08 = p->m_time;
    m_frame = 0;
    m_time = p->m_time;
}

// ──────────────────────────────────────────────────────────────────────
// Counts the frames: in states 2, 3, 6, 7 the fade's (a state ends when
// m_frame passes m_time: 2 -> 3 -> 4, 6 -> 7 -> 8), in 4, 5 the magnify
// ramp's (m_magnify from m_magnify_from to m_magnify_to; at the end the
// ramp's words are cleared).
// ROR 0x0012f84c

void MagLifeFadeKeyHole::live()
{
    int  state = m_state;
    bool end = false;

    m_04++;
    switch (state) {
    case 2:
    case 3:
    case 6:
    case 7:
        m_frame++;
        end = m_time < m_frame;
        break;
    case 4:
    case 5:
        m_frame++;
        if (m_frame < m_time) {
            m_magnify = m_magnify_from +
                        ((m_magnify_to - m_magnify_from) * (float)m_frame) / (float)m_time;
        } else {
            m_magnify = m_magnify_to;
            m_time = 0;
            m_frame = 0;
        }
        break;
    }
    if (!end)
        return;

    m_frame = 0;
    switch (state) {
    case 2:
        m_state = 3;
        break;
    case 3:
        m_state = 4;
        break;
    case 6:
        m_state = 7;
        break;
    case 7:
        m_state = 8;
        break;
    default:
        return;
    }
    m_04 = -1;
}

// ──────────────────────────────────────────────────────────────────────
// One textured sprite over the whole screen (GS 0x7000,0x7200 ..
// 0x9000,0x8e00, z 0x7fff0) of texture m_tex_no: the picture's centre
// (256, 256) +- 256 / m_magnify texels, shifted across the free width by
// m_50. The sprite's colour and its TEXA TA0 (TA1 0x80; TEXA acts on 16 /
// 24-bit texels only, not EVNTEX's 32-bit CLUTs) follow the state,
// blended (Cs - Cd) * As + Cd.
// ROR 0x0012f95c

void MagLifeFadeKeyHole::render(I3dLayer *layer)
{
    int    color = 0x80;
    u_long ta0 = 0;
    int    time = m_time;

    switch (m_state) {
    case 1:
    case 8:
        color = 0;
        ta0 = 0x80;
        break;
    case 2:
        if (time > 0 && m_frame <= time) {
            color = (m_frame << 7) / time;
            ta0 = 0x80;
        }
        break;
    case 3:
        if (time > 0 && m_frame <= time)
            ta0 = ((time - m_frame) * 0x80) / time;
        break;
    case 6:
        if (time > 0 && m_frame <= time)
            ta0 = (m_frame << 7) / time;
        break;
    case 7:
        if (time > 0 && m_frame <= time) {
            color = ((time - m_frame) * 0x80) / time;
            ta0 = 0x80;
        }
        break;
    }

    I3dTexSetDesc desc;
    desc.m_depth = 1.0f;
    desc.m_tex_no = &m_tex_no;
    desc.m_tex_count = 1;
    desc.m_sort_func = 0;
    desc.m_sort_order = 0;
    desc.m_sort_param = 0;
    I3dDrawSet *set = i3dLayerFindDrawSet(layer, &desc);
    i3dLayerSetDrawSet(layer, set);

    u_long *pkt = i3dLayerBeginSet(layer, 0.0f);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(set, 0));
    int half = (int)(256.0f / m_magnify);
    int shift = (int)((float)(0x100 - half) * m_50);
    pkt = i3dPktSetReg(pkt, I3D_GS_TEST_1, 0x51003);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEXA, ta0 | 0x8000000000UL);
    pkt = i3dPktSetReg(pkt, I3D_GS_ALPHA_1, 0x44);
    u_long xyz0 = i3dPackXYZ(0x7000, 0x7200, 0x7fff0);
    u_long xyz1 = i3dPackXYZ(0x9000, 0x8e00, 0x7fff0);
    u_long uv0 = i3dPackUV((shift - half + 0x100) * 0x10, (0x120 - half) * 0x10);
    u_long uv1 = i3dPackUV((half + shift + 0x100) * 0x10, (half + 0xe0) * 0x10);
    u_long rgbaq = i3dPackRGBAQ(1.0f, color, color, color, 0x80);
    pkt = i3dPktTexSpriteUV(pkt, xyz0, xyz1, uv0, uv1, rgbaq);
    i3dLayerEndSet(layer, pkt);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0012fc80

void MagLifeFadeKeyHole::setMagnify(float magnify, int time)
{
    if (time > 0) {
        m_time = time;
        m_magnify_from = m_magnify;
        m_magnify_to = magnify;
        m_frame = 0;
        return;
    }
    m_magnify = magnify;
    m_time = 0;
}
