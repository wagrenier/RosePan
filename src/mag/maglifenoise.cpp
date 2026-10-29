/* ==========================================================================
 *  mag/maglifenoise.cpp
 *
 *  The screen noise (RoR's file name inferred from the class, between
 *  mag/maglifewindow.cpp and mag/maglifetest.cpp; pinned 0x130a2c..
 *  0x130ee8): a MagLife that, while GamMan::setVolume's strength is set,
 *  blends four randomly offset full-screen sprites of a noise texture over
 *  the frame; see maglifenoise.h.
 * ======================================================================== */
#include "mag/maglifenoise.h"

#include "i3d/i3dlayer.h"
#include "i3d/i3dpacket.h"
#include "rosepan/os/rosepan_rand.h"
#include "rosepan/rendering/rosepan_i3d.h"

/* The sprite's Z (0x7ffe0, in XYZ2's upper word). */
#define MAG_NOISE_Z 0x7ffe000000000ull

// ──────────────────────────────────────────────────────────────────────
// One noise layer: the blend, then a REGLIST GIF tag (PRIM, NOP, UV,
// XYZ2, UV, XYZ2) whose sprite covers rect and samples from a random
// corner (rand % w, rand % h in 10.4, under one texel step) a window half
// to all of w x h wide, shifted by 8 (the far corner in texels is half
// the sum). The colour is writeNoise's RGBAQ register.
// ROR 0x00130a2c

u_long *MagLifeNoise::writeNoiseLayer(u_long *pkt, const MagLifeNoiseRect *rect, int w, int h,
                                      int fix, int mode)
{
    if (mode != 0)
        pkt = i3dPktSetReg(pkt, I3D_GS_ALPHA_1, (u_long)fix << 32 | 0x61);
    else
        pkt = i3dPktSetReg(pkt, I3D_GS_ALPHA_1, (u_long)fix << 32 | 0x64);
    u_long *data = i3dPktDirectTag(pkt, 4, 0x10, NULL);

    int u = RosePan_Rand() % w;  // callcheck: rand (RosePan_Rand keeps the ROM 31-bit range)
    int v = RosePan_Rand() % h;
    float f = (float)RosePan_Rand() / ROSEPAN_RAND_MAXF;
    int half_h = h / 2;
    int half_w = w / 2;

    data[0] = 0x6400000000008001ull;                /* REGLIST, NREG 6, EOP */
    data[1] = (I3D_GS_XYZ2 << 20) | (I3D_GS_UV << 16) | (I3D_GS_XYZ2 << 12) |
              (I3D_GS_UV << 8) | (I3D_GS_NOP << 4) | I3D_GS_PRIM;
    data[2] = 0x156;                                /* sprite, TME, ABE, FST */
    data[4] = (u_long)u | (u_long)v << 16;
    data[6] = (u_long)((u + half_w + (int)(f * (float)half_w)) * 8) |
              (u_long)((v + half_h + (int)(f * (float)half_h)) * 8) << 16;
    data[5] = (u_long)(rect->m_x << 4) | (u_long)(rect->m_y << 4) << 16 | MAG_NOISE_Z;
    data[7] = (u_long)((rect->m_x + rect->m_w) * 16) |
              (u_long)((rect->m_y + rect->m_h) * 16) << 16 | MAG_NOISE_Z;
    RosePan_I3dRecordGif((const uint64_t *)(data - 2));     // PC: the native i3d module
    return data + 8;
}

// ──────────────────────────────────────────────────────────────────────
// CLAMP_1 repeat, RGBAQ (0x20, 0x20, 0x20, 0x3f), TEST_1 colour only
// (alpha test never, fail = frame; Z always), then four layers whose
// FIX is a quarter of the volume, rounded up for the first.
// ROR 0x00130c34

u_long *MagLifeNoise::writeNoise(u_long *pkt, const MagLifeNoiseRect *rect)
{
    if (rect->m_volume == 0)
        return pkt;     /* the ROM returns nothing here; render never gets here at 0 */
    pkt = i3dPktSetReg(pkt, 0x08, 0);                       /* CLAMP_1 */
    pkt = i3dPktSetReg(pkt, I3D_GS_RGBAQ, 0x3f202020);
    pkt = i3dPktSetReg(pkt, I3D_GS_TEST_1, 0x31001);

    int fix = (rect->m_volume + 3) / 4;
    if (fix < 0)
        fix = 0;
    pkt = writeNoiseLayer(pkt, rect, 400, 400, fix, 1);
    fix = (rect->m_volume + 2) / 4;
    if (fix < 0)
        fix = 0;
    pkt = writeNoiseLayer(pkt, rect, 600, 600, fix, 0);
    fix = (rect->m_volume + 1) / 4;
    if (fix < 0)
        fix = 0;
    pkt = writeNoiseLayer(pkt, rect, 888, 888, fix, 1);
    fix = rect->m_volume / 4;
    if (fix < 0)
        fix = 0;
    return writeNoiseLayer(pkt, rect, 1000, 1000, fix, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00130da4

MagLifeNoise::MagLifeNoise()
{
    /* MagLife0's words, then MagLife's and the noise's (the ROM's order). */
    m_state = 0;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_level = 1;
    m_tex_no = 0;
    m_04 = -1;
    m_state = 0;
    m_hide = 0;
    m_scene = NULL;
    m_volume = 0;
}

// ──────────────────────────────────────────────────────────────────────
// linkonce
// ROR 0x00285378

MagLifeNoise::MagLifeNoise(int tex_no)
{
    m_level = 1;
    m_state = 0;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_tex_no = tex_no;
    m_04 = -1;
    m_state = 0;
    m_hide = 0;
    m_scene = NULL;
    m_volume = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00130de0

void MagLifeNoise::setVolume(int volume)
{
    m_volume = volume;
}

// ──────────────────────────────────────────────────────────────────────
// The draw set of m_tex_no (depth 1), one packet at sort key 895 with its
// texture registers and the four layers over the 512 x 448 frame.
// ROR 0x00130de8

void MagLifeNoise::render(I3dLayer *layer)
{
    if (m_volume == 0)
        return;
    MagLifeNoiseRect rect;
    rect.m_w = 0x200;
    rect.m_x = 0x700;
    rect.m_y = 0x720;
    rect.m_h = 0x1c0;
    rect._08 = 0;
    rect._0c = 0;
    rect.m_volume = m_volume;

    I3dTexSetDesc desc;
    desc.m_depth = 1.0f;
    desc.m_tex_no = &m_tex_no;
    desc.m_tex_count = 1;
    desc.m_sort_func = 0;
    desc.m_sort_order = 0;
    desc.m_sort_param = 0;
    I3dDrawSet *set = i3dLayerFindDrawSet(layer, &desc);
    i3dLayerSetDrawSet(layer, set);
    u_long *pkt = i3dLayerBeginSet(layer, 895.0f);
    u_long *regs = i3dDrawSetUseTex(set, 0);
    pkt = i3dPktRefTexRegs(pkt, regs);
    pkt = writeNoise(pkt, &rect);
    i3dLayerEndSet(layer, pkt);
}
