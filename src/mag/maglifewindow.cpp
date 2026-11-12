/* ==========================================================================
 *  mag/maglifewindow.cpp
 *
 *  The scene's text windows (RoR's file, anchored by its assert strings;
 *  0x12fca8..0x1308bc): MagLifeWindow draws Shift-JIS text into a GamFont
 *  buffer and shows that buffer as a sprite with an alpha fade. See
 *  maglifewindow.h. The linkonce accessors are inline in the header.
 * ======================================================================== */
#include "mag/maglifewindow.h"

#include "common/ror_assert.h"
#include "i3d/i3dlayer.h"
#include "i3d/i3dpacket.h"
#include "lib/unassigned.h"
#include "sdk/libgraph.h"

/* A Shift-JIS lead byte (0x81..0x9f, 0xe0..0xfc). */
static inline int isSjisLead(u_int c)
{
    return c - 0x81 < 0x1f || c - 0xe0 < 0x1d;
}

// ──────────────────────────────────────────────────────────────────────
// GamFont::getCharMetrics on the window's font.
// ROR 0x0012fca8 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeWindow::getCharMetrics(int code, int *left, int *width, int *extra)
{
    ROR_ASSERT(m_fntman); /* 43 */
    m_fntman->getCharMetrics(code, left, width, extra);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0012fd30 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeWindow::setProportional(int on)
{
    ROR_ASSERT(m_fntman); /* 49 */
    m_fntman->m_prop_on = on != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0012fd88 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeWindow::setFontWord30(int v)
{
    ROR_ASSERT(m_fntman); /* 55 */
    m_fntman->m_spacing = v;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0012fddc // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeWindow::setFontWord34(int v)
{
    ROR_ASSERT(m_fntman); /* 61 */
    m_fntman->m_spacing_prop = v;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0012fe30

void MagLifeWindow::setup(int w, int h)
{
    m_buf_h = 0x200;
    m_fade_alpha = 128.0f;
    m_w = w;
    m_h = h;
    m_fade_time = -1;
    m_buf_w = 0x200;
    m_fade_max = 128.0f;
    m_fade_min = 0.0f;
    m_fade_step = 0.0f;
}

// ──────────────────────────────────────────────────────────────────────
// Ramps the alpha up from `from` to `to` over `time` frames (render steps
// it).
// ROR 0x0012fe68

void MagLifeWindow::fadeIn(int time, int from, int to)
{
    if (time < 1)
        time = 1;
    if (to < 1)
        to = 1;
    if (from < 0)
        from = m_alpha;
    if (from < 0)
        from = 0;
    if (to > 0x80)
        to = 0x80;
    if (from > 0x7f)
        from = 0x7f;
    m_fade_time = time;
    if (to <= from)
        to = from + 1;
    m_fade_alpha = (float)from;
    m_fade_max = (float)to;
    m_fade_min = (float)from;
    m_fade_step = ((float)to - (float)from) / (float)time;
}

// ──────────────────────────────────────────────────────────────────────
// Ramps the alpha down from `to` to `from` over `time` frames.
// ROR 0x0012fef8

void MagLifeWindow::fadeOut(int time, int from, int to)
{
    if (time < 1)
        time = 1;
    if (from < 0)
        from = 0;
    if (to < 0)
        to = m_alpha;
    if (to < 1)
        to = 1;
    if (from > 0x7f)
        from = 0x7f;
    if (to > 0x80)
        to = 0x80;
    if (to <= from)
        to = from + 1;
    m_fade_time = time;
    m_fade_min = (float)from;
    m_fade_alpha = (float)to;
    m_fade_max = (float)to;
    m_fade_step = ((float)from - (float)to) / (float)time;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0012ff84

void MagLifeWindow::live()
{
    if (m_timer > 0)
        m_timer--;
}

// ──────────────────────────────────────────────────────────────────────
// Steps the fade (its last frame lands on the end alpha), then draws the
// font texture as one sprite: GS (0x700 + x, 0x720 + y) .. + (m_w, m_h),
// z m_pos[2] << 4, uv m_uv, colour m_color / m_alpha, alpha blended.
// Nothing without a texture.
// ROR 0x0012ff9c

void MagLifeWindow::render(I3dLayer *layer)
{
    if (m_tex_no < 0)
        return;

    float x = m_pos[0];
    float y = m_pos[1];
    int   t = m_fade_time;
    if (t >= 0) {
        if (t == 0) {
            float a = m_fade_step <= 0.0f ? m_fade_min : m_fade_max;
            m_alpha = (int)a;
            m_fade_alpha = a;
            m_fade_step = 0.0f;
        } else {
            m_fade_alpha += m_fade_step;
            if (m_fade_alpha < m_fade_min)
                m_fade_alpha = m_fade_min;
            if (m_fade_max < m_fade_alpha)
                m_fade_alpha = m_fade_max;
            m_alpha = (int)m_fade_alpha;
        }
        m_fade_time = t - 1;
    }

    I3dTexSetDesc set_desc;
    set_desc.m_depth = 60000.0f;
    set_desc.m_tex_no = &m_tex_no;
    set_desc.m_tex_count = 1;
    set_desc.m_sort_func = 0;
    set_desc.m_sort_order = 0;
    set_desc.m_sort_param = 0;
    int z = (int)m_pos[2] << 4;
    I3dDrawSet *set = i3dLayerFindDrawSet(layer, &set_desc);
    i3dLayerSetDrawSet(layer, set);

    u_long *pkt = i3dLayerBeginSet(layer, 950.0f);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEXA, 0x8000);
    pkt = i3dPktSetReg(pkt, I3D_GS_TEST_1, 0x31003);
    pkt = i3dPktSetReg(pkt, I3D_GS_ALPHA_1, 0x44);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(set, 0));
    u_long xyz0 = i3dPackXYZ(((int)x + 0x700) * 0x10, ((int)y + 0x720) * 0x10, z);
    u_long xyz1 = i3dPackXYZ(((int)x + m_w + 0x700) * 0x10, ((int)y + m_h + 0x720) * 0x10, z);
    u_long uv0 = i3dPackUV(m_uv[0] << 4, m_uv[1] << 4);
    u_long uv1 = i3dPackUV(m_uv[2] << 4, m_uv[3] << 4);
    u_long rgbaq = i3dPackRGBAQ(1.0f, m_color[0], m_color[1], m_color[2], m_alpha);
    pkt = i3dPktTexSpriteUV(pkt, xyz0, xyz1, uv0, uv1, rgbaq);
    i3dLayerEndSet(layer, pkt);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00130214

void MagLifeWindow::getPos(int *x, int *y)
{
    *x = (int)m_pos[0];
    *y = (int)m_pos[1];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00130230

void MagLifeWindow::getPos3(int *x, int *y, int *z)
{
    *x = (int)m_pos[0];
    *y = (int)m_pos[1];
    *z = (int)m_pos[2];
}

// ──────────────────────────────────────────────────────────────────────
// Draws str into the font buffer from (x, y) in color. Shift-JIS lead
// bytes take the next byte; 10 is a new line, 0x11 a marker that (as the
// first byte) centres every line in m_w, other codes below 0x20 draw
// nothing. A glyph that would pass m_buf_w starts a new line, one that
// would pass m_buf_h scrolls the buffer. The cursor is left after the text.
// ROR 0x00130258 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeWindow::drawText(int x, int y, const char *str, int color)
{
    const u_char *p = (const u_char *)str;
    int line_x[10];             /* each line's start: 0, or its centring offset */
    int left, width, extra;

    ROR_ASSERT(m_fntman); /* 319 */
    ROR_ASSERT(m_ch_w > 0); /* 320 */
    ROR_ASSERT(m_ch_h > 0); /* 321 */
    ROR_ASSERT(m_buf_w > 0); /* 322 */
    ROR_ASSERT(m_buf_h > 0); /* 323 */
    m_fntman->setColor(color << 2);

    for (int i = 9; i >= 0; i--)
        line_x[i] = 0;

    /* PC: the ROM indexes line_x past its 10 lines (other locals there). */
#define LINE_X(n) (*((n) < 10 ? &line_x[n] : &line_x[9]))

    if (*p == 0x11) {
        int line = 0;
        for (const u_char *q = p; *q != 0;) {
            u_int code = *q++;
            if (isSjisLead(code))
                code = code << 8 | *q++;
            m_fntman->getCharMetrics(code, &left, &width, &extra);
            if (code == 10)
                line++;
            else if (code != 0x11)
                LINE_X(line) += left + width;
        }
        for (int i = 0; i < 10; i++) {
            int cx = (m_w - line_x[i]) / 2;
            if (cx < 0)
                cx = 0;
            line_x[i] = cx;
        }
    }

    int line = 0;
    int cx = x + line_x[0];
    while (*p != 0) {
        u_int code = *p++;
        if (isSjisLead(code))
            code = code << 8 | *p++;
        m_fntman->getCharMetrics(code, &left, &width, &extra);
        if (code == 10) {
            line++;
            y += m_ch_h;
            cx = x + LINE_X(line);
        } else if (code != 0x11 && code > 0x1f) {
            if (m_buf_w < cx + width + extra) {
                line++;
                cx = LINE_X(line);
                y += m_ch_h;
            }
            ROR_ASSERT(m_buf_h > 0); /* 462 */
            if (m_buf_h < y + m_ch_h) {
                m_fntman->scroll(y + m_ch_h - m_buf_h);
                y -= m_ch_h;
                if (y < 0)
                    y = 0;
            }
            m_fntman->drawChar(cx - left, y, code);
            cx += width + extra;
        }
    }
#undef LINE_X
    m_cursor_y = y;
    m_cursor_x = cx;
}

// ──────────────────────────────────────────────────────────────────────
// Draws str in color from the cursor: a Shift-JIS character advances a
// cell, others half a cell (13 none), 10 starts a new line, as does a
// character that would pass m_buf_w; past m_buf_h the buffer scrolls up.
// The bytes are signed here (lb), so the lead-byte test never matches a
// byte >= 0x80: such bytes draw nothing (the ROM's; ASCII only in effect).
// ROR 0x001305ec // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeWindow::print(const char *str, int color)
{
    ROR_ASSERT(m_fntman); /* 503 */
    m_fntman->setColor(color << 2);

    int cy = m_cursor_y;
    int cx = m_cursor_x;
    while (*str != 0) {
        int adv = 0;
        int code = (s_char)*str++;
        if (isSjisLead((u_int)code)) {
            adv = m_ch_w;
            code = (code & 0xff) << 8 | (u_char)*str++;
        } else if (code == 10) {
            cx = 0;
            cy += m_ch_h;
        } else if (code != 0xd) {
            adv = m_ch_w / 2;
        }
        if (m_buf_w < cx + adv) {
            cx = 0;
            cy += m_ch_h;
        }
        if (m_buf_h < cy + m_ch_h) {
            int d = m_buf_h - (cy + m_ch_h);
            cy += d;
            m_fntman->scroll(d);
        }
        if (code >= 0x20) {
            m_fntman->drawChar(cx, cy, code);
            cx += adv;
        }
    }
    m_cursor_y = cy;
    m_cursor_x = cx;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00130778 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeWindow::setFontManager(GamFont *thefontManager)
{
    ROR_ASSERT(thefontManager); /* 578 */
    m_fntman = thefontManager;
    m_ch_w = thefontManager->m_char_w;
    ROR_ASSERT(m_ch_w); /* 582 */
    m_ch_h = thefontManager->m_char_h;
    ROR_ASSERT(m_ch_h); /* 585 */
    m_buf_w = thefontManager->m_dst_w;
    ROR_ASSERT(m_buf_w); /* 588 */
    m_buf_h = thefontManager->m_dst_h;
    ROR_ASSERT(m_buf_h); /* 591 */
    m_uv[2] = m_buf_w;
    m_uv[3] = m_buf_h;
    m_uv[0] = 0;
    m_uv[1] = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013086c // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

int MagLifeWindow::getTextWidth(const char *theStr)
{
    ROR_ASSERT(theStr); /* 608 */
    return m_fntman->getTextWidth(theStr);
}

// ──────────────────────────────────────────────────────────────────────
// MagLife0 / MagLife / MagLifePos's inline constructors, then the
// window's words (the ROM's order; MagLifePos's vectors are set twice).
// ROR 0x001308bc

MagLifeWindow::MagLifeWindow()
{
    m_level = 1;
    m_state = 0;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_hide = 0;
    m_04 = -1;
    m_state = 0;
    m_scene = NULL;
    sceVu0FVECTOR *v[5] = { &m_pos, &m_speed, &m_accel, &m_60, &m_70 };
    for (int i = 0; i < 5; i++) {
        (*v[i])[0] = 0.0f;
        (*v[i])[1] = 0.0f;
        (*v[i])[2] = 0.0f;
        (*v[i])[3] = 1.0f;
    }
    m_color[2] = 0xff;
    m_alpha = 0x80;
    m_fade_min = 0.0f;
    m_fade_alpha = 128.0f;
    m_tex_no = -1;
    m_timer = 0;
    m_uv[0] = 0;
    m_uv[1] = 0;
    m_uv[2] = 0;
    m_uv[3] = 0;
    m_w = 0;
    m_h = 0;
    m_color[0] = 0xff;
    m_color[1] = 0xff;
    m_fade_time = -1;
    m_fade_step = 128.0f;
    m_fade_max = 0.0f;
    m_fntman = NULL;
    m_ch_w = 0;
    m_ch_h = 0;
    m_cursor_y = 0;
    m_buf_w = 0;
    m_buf_h = 0;
    m_cursor_x = 0;
}
