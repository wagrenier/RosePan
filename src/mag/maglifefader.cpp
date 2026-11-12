/* ==========================================================================
 *  mag/maglifefader.cpp
 *
 *  The screen fader (RoR's file name inferred from the class, like
 *  mag/maglifefadekey.cpp after it; pinned 0x12f008..0x12f298): a MagLife
 *  that draws one full-screen sprite of m_color, its alpha ramped by the
 *  fade state. MagScene::setFade starts and stops it; see maglifefader.h.
 * ======================================================================== */
#include "mag/maglifefader.h"

#include "i3d/i3dlayer.h"
#include "i3d/i3dpacket.h"
#include "lib/unassigned.h"

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0012f008

MagLifeFader::MagLifeFader()
{
    /* MagLife0's words, then MagLife's and the fader's (the ROM's order). */
    m_state = 0;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_level = 1;
    m_color = 0;
    m_04 = -1;
    m_state = 0;
    m_hide = 0;
    m_scene = NULL;
    m_frame = 0;
    m_time = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0012f060

void MagLifeFader::start(const MagFadeParam *p)
{
    m_04 = -1;
    m_state = p->m_mode;
    m_frame = 0;
    m_time = p->m_time;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0012f080

void MagLifeFader::stop()
{
    m_state = 4;
    m_04 = -1;
}

// ──────────────────────────────────────────────────────────────────────
// Counts the frames of a running fade (states 2, 3 in, 6, 7 out); at
// m_time a fade in ends in state 4 (clear), a fade out in 8 (covered).
// ROR 0x0012f094

void MagLifeFader::live()
{
    int  state = m_state;
    bool end = false;

    m_04++;
    if ((state >= 2 && state <= 3) || (state >= 6 && state <= 7)) {
        m_frame++;
        end = m_frame >= m_time;
    }
    if (end) {
        if (state <= 3) {
            m_state = 4;
            m_04 = -1;
        } else {
            m_state = 8;
            m_04 = -1;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// One sprite over the whole screen (GS 0x7000,0x7200 .. 0x9000,0x8e00, z
// 0x80000) in grey m_color, blended (Cs - Cd) * As + Cd.
// ROR 0x0012f128

void MagLifeFader::render(I3dLayer *layer)
{
    int alpha = 0;

    switch (m_state) {
    case 1:
    case 8:
        alpha = 0x80;
        break;
    case 2:
    case 3:
        if (m_time > 0 && m_frame < m_time)
            alpha = ((m_time - m_frame) * 0x80) / m_time;
        break;
    case 6:
    case 7: {
        int time = m_time - 2;
        if (time < 1)
            time = 1;
        if (m_frame < time)
            alpha = (m_frame << 7) / time;
        else
            alpha = 0x80;
        break;
    }
    default:
        return;
    }

    u_long *pkt = i3dLayerBeginSet(layer, 0.0f);
    pkt = i3dPktSetReg(pkt, I3D_GS_ALPHA_1, 0x44);
    u_long xyz0 = i3dPackXYZ(0x7000, 0x7200, 0x80000);
    u_long xyz1 = i3dPackXYZ(0x9000, 0x8e00, 0x80000);
    u_long rgbaq = i3dPackRGBAQ(1.0f, m_color, m_color, m_color, alpha);
    pkt = i3dPktSprite(pkt, xyz0, xyz1, rgbaq);
    i3dLayerEndSet(layer, pkt);
}
