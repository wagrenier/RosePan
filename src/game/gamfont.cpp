// game/gamfont.cpp -- the font rasterizer (0x1beb70..0x1c05c0).
//
// A GamFont draws a font file's 1- or 2-bit glyphs into a 4- or 8-bit pixel
// buffer (setTarget) that its owner uploads as a CLUT texture: SprMan's text
// sprites, MagScene's text windows (MagLifeWindow draws through them). A
// glyph pixel v (1..3; a 1-bit font's pixels count as 3) becomes v | m_color
// in the buffer; 4-bit buffers hold the even pixel in the low nibble.
//
// RoR does not name this file (no assert in it): game/ by link order (after
// game/gamfile.cpp, before GamMan), gamfont after the class -- a guess.

#include "game/gamfont.h"

#include <stdlib.h>
#include <string.h>

// The Shift-JIS rows a kanji font holds, in glyph order (data 0x2b8de8):
// { first code, codes }. Entry 0 is the single-byte range; getKanjiIndex
// walks the rest up to the zero entry. 0x8140..0x9872 is JIS level 1 (0x1130
// codes), 0x989f..0xeaa4 level 2 (0xde0) -- the counts setFont subtracts.
// layout: host -- the ROM's rows are 8 bytes (u16 start at +0, u16 count at
// +4); only the code reads them.
struct GamFontRow
{
    u_short m_start;
    u_short m_count;
};
static const GamFontRow s_sjisRows[] = {
    { 0x0020, 0xe0 },  // 0x002b8de8
    { 0x8140, 0xc0 }, { 0x8240, 0xc0 }, { 0x8340, 0xc0 }, { 0x8440, 0xc0 },  // 0x002b8df0
    { 0x8540, 0xc0 }, { 0x8640, 0xc0 }, { 0x8740, 0xc0 }, { 0x889f, 0x60 },  // 0x002b8e10
    { 0x8940, 0xc0 }, { 0x8a40, 0xc0 }, { 0x8b40, 0xc0 }, { 0x8c40, 0xc0 },  // 0x002b8e30
    { 0x8d40, 0xc0 }, { 0x8e40, 0xc0 }, { 0x8f40, 0xc0 }, { 0x9040, 0xc0 },  // 0x002b8e50
    { 0x9140, 0xc0 }, { 0x9240, 0xc0 }, { 0x9340, 0xc0 }, { 0x9440, 0xc0 },  // 0x002b8e70
    { 0x9540, 0xc0 }, { 0x9640, 0xc0 }, { 0x9740, 0xc0 }, { 0x9840, 0x50 },  // 0x002b8e90
    { 0x989f, 0x60 }, { 0x9940, 0xc0 }, { 0x9a40, 0xc0 }, { 0x9b40, 0xc0 },  // 0x002b8eb0
    { 0x9c40, 0xc0 }, { 0x9d40, 0xc0 }, { 0x9e40, 0xc0 }, { 0x9f40, 0xc0 },  // 0x002b8ed0
    { 0xe040, 0xc0 }, { 0xe140, 0xc0 }, { 0xe240, 0xc0 }, { 0xe340, 0xc0 },  // 0x002b8ef0
    { 0xe440, 0xc0 }, { 0xe540, 0xc0 }, { 0xe640, 0xc0 }, { 0xe740, 0xc0 },  // 0x002b8f10
    { 0xe840, 0xc0 }, { 0xe940, 0xc0 }, { 0xea40, 0xc0 },                    // 0x002b8f30
    { 0, 0 },                                                                // 0x002b8f48
};

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001beb70

GamFont::GamFont()
{
    m_dst_bits = 4;
    m_spacing_prop = 2;
    m_fill = 0x80;
    m_fill_msb = 1;
    m_00 = 0;
    m_04 = 0;
    m_dst = NULL;
    m_dst_w = 0;
    m_dst_h = 0;
    m_dst_size = 0;
    m_color = 0;
    m_char_w = 0;
    m_char_h = 0;
    m_code_count = 0;
    m_font = NULL;
    m_spacing = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001bebc4 // callcheck: operator_delete (the deleting variant, compiler generated)

GamFont::~GamFont()
{
}

// ──────────────────────────────────────────────────────────────────────
// The metrics of A, Z, a and z are read and dropped (left over from
// something the ROM no longer keeps).
// ROR 0x001bebec

void GamFont::setFont(const void *font)
{
    if (font == NULL)
        return;
    m_font = (const GamFontHeader *)font;
    m_char_w = m_font->m_char_w;
    m_char_h = m_font->m_char_h;
    m_bpp = m_font->m_bpp;
    u_short count = m_font->m_count;
    m_code_count = count;
    if (m_font->m_flags & GAMFONT_KANJI1)
        m_code_count = count - 0x1130;
    if (m_font->m_flags & GAMFONT_KANJI2)
        m_code_count -= 0xde0;
    m_codes = NULL;
    m_widths = NULL;
    m_glyphs = NULL;
    m_prop_on = 0;
    m_prop = 0;
    const u_char *base = (const u_char *)m_font + 4;
    if (m_font->m_code_ofs != 0)
        m_codes = (const u_short *)(base + m_font->m_code_ofs);
    if (m_font->m_width_ofs != 0) {
        m_prop = 1;
        m_prop_on = 1;
        m_widths = base + m_font->m_width_ofs;
    }
    if (m_font->m_glyph_ofs != 0)
        m_glyphs = base + m_font->m_glyph_ofs;

    int left = 0, width = 0, extra = 0;
    getCharMetrics('A', &left, &width, &extra);
    getCharMetrics('Z', &left, &width, &extra);
    getCharMetrics('a', &left, &width, &extra);
    getCharMetrics('z', &left, &width, &extra);

    u_int bits = m_char_w * m_bpp;
    m_row_bytes = ((int)bits >> 3) + ((bits & 7) != 0);
    m_glyph_bytes = m_row_bytes * m_char_h;
    m_font_bytes = m_glyph_bytes * count;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001bedc0

int GamFont::setTarget(void *buf, u_int w, u_int h, int bits)
{
    releaseTarget();
    m_dst_w = w & 0xff80;
    if (bits == 4)
        h &= 0xff80;
    else if (bits == 8)
        h &= 0xffc0;
    else
        return 0;
    m_dst_h = h;
    if (m_dst_w == 0 || m_dst_h == 0)
        return 0;
    int size = m_dst_w * m_dst_h;
    m_dst_bits = bits;
    if (bits == 4)
        size >>= 1;
    m_dst = (u_char *)buf;
    m_dst_size = size;
    m_04 = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001bee8c

int GamFont::releaseTarget()
{
    int had = m_dst != NULL;
    if (had) {
        m_dst = NULL;
        m_04 = 0;
    }
    return had;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001beeb4

int GamFont::clear()
{
    if (m_dst != NULL)
        memset(m_dst, m_fill, m_dst_size);
    return m_dst != NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001beeec

int GamFont::getCellWidth(int code)
{
    if (isHalfWidth(code) == 1)
        return m_char_w >> 1;
    return m_char_w;
}

// ──────────────────────────────────────────────────────────────────────
// A 4-bit rect starts at the even pixel at or left of x and covers w & ~1
// pixels.
// ROR 0x001bef24

int GamFont::fillRect(int x, int y, int w, int h)
{
    if (m_dst == NULL)
        return 0;
    if (m_dst_bits == 4) {
        u_int c = m_fill & 0xf;
        for (int row = 0; row < h; row++) {
            u_char *p = m_dst + (m_dst_w >> 1) * row + (x >> 1) + (m_dst_w >> 1) * y;
            for (int n = w >> 1; n > 0; n--)
                *p++ = (u_char)(c | (c << 4));
        }
    } else {
        for (int row = 0; row < h; row++) {
            u_char *p = m_dst + m_dst_w * row + x + m_dst_w * y;
            for (int n = w; n > 0; n--)
                *p++ = (u_char)m_fill;
        }
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The ROM unrolls one loop per (buffer depth, x parity, font depth); one
// loop over the glyph's pixels here, with the same stores: pixel p of the
// row goes to byte p >> 1, nibble p & 1 of a 4-bit buffer (p counts from
// x's parity), to byte p of an 8-bit one. Nothing for fonts that are not
// 1 or 2 bits deep.
// ROR 0x001bf024

void GamFont::drawChar(int x, int y, int code)
{
    if (m_dst == NULL)
        return;
    u_int color = m_color;
    int four = m_dst_bits == 4;
    int pitch = four ? m_dst_w >> 1 : m_dst_w;
    int glyph = getGlyph(code);
    if (glyph < 0)
        return;
    int bpp = m_bpp;
    if (bpp == 1)
        color |= 3;
    else if (bpp != 2)
        return;
    const u_char *src = m_glyphs + m_glyph_bytes * glyph;
    u_char *dst = m_dst + (x >> four) + y * (m_dst_w >> four);
    int mask = (1 << bpp) - 1;
    for (int row = 0; row < m_char_h; row++) {
        u_char *d = dst + pitch * row;
        int p = four ? (x & 1) : 0;
        for (int i = 0; i < m_row_bytes; i++) {
            u_int b = src[m_row_bytes * row + i];
            for (int k = 8 - bpp; k >= 0; k -= bpp, p++) {
                u_int v = (b >> k) & mask;
                if (v == 0)
                    continue;
                if (four)
                    d[p >> 1] |= (u_char)((v | color) << ((p & 1) * 4));
                else
                    d[p] |= (u_char)(v | color);
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// drawChar's blit with stores: every pixel of the glyph's rows is written,
// 0 where the glyph is clear; a 4-bit buffer is written from the even pixel
// at or left of x. No callers.
// ROR 0x001bf8c4

void GamFont::putChar(int x, int y, int code)
{
    if (m_dst == NULL)
        return;
    int four = m_dst_bits == 4;
    u_int color = m_color;
    int pitch = four ? m_dst_w >> 1 : m_dst_w;
    int glyph = getGlyph(code);
    if (glyph < 0)
        return;
    int bpp = m_bpp;
    if (bpp == 1)
        color |= 3;
    else if (bpp != 2)
        return;
    const u_char *src = m_glyphs + m_glyph_bytes * glyph;
    u_char *dst = m_dst + (x >> four) + y * (m_dst_w >> four);
    int mask = (1 << bpp) - 1;
    for (int row = 0; row < m_char_h; row++) {
        u_char *d = dst + pitch * row;
        int p = 0;
        for (int i = 0; i < m_row_bytes; i++) {
            u_int b = src[m_row_bytes * row + i];
            for (int k = 8 - bpp; k >= 0; k -= bpp, p++) {
                u_int v = (b >> k) & mask;
                u_char px = v != 0 ? (u_char)(v | color) : 0;
                if (!four)
                    d[p] = px;
                else if ((p & 1) == 0)
                    d[p >> 1] = px;
                else
                    d[p >> 1] |= v != 0 ? (u_char)((v | color) << 4) : 0;
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// A buffer that is neither 4 nor 8 bits moves w bytes per row and fills 0.
// ROR 0x001bfe30

void GamFont::scroll(int rows)
{
    if (rows >= m_dst_h)
        return;
    if (rows < -m_dst_h)
        return;
    int bytes = m_dst_w * (rows < 0 ? -rows : rows);
    u_int fill = 0;
    if (m_dst_bits == 4) {
        bytes >>= 1;
        fill = m_fill & 0xf;
        fill |= fill << 4;
    } else if (m_dst_bits == 8) {
        fill = m_fill;
    }
    if (rows < 0) {
        memmove(m_dst, m_dst + bytes, m_dst_size - bytes);
        memset(m_dst + (m_dst_size - bytes), fill, bytes);
    } else {
        memmove(m_dst + bytes, m_dst, m_dst_size - bytes);
        memset(m_dst, fill, bytes);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001bff28

void GamFont::setFill(u_int fill)
{
    m_fill = fill;
    if (m_dst_bits != 4 && m_dst_bits == 8 && m_fill_msb != 0)
        m_fill = fill | 0x80;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001bff5c

void GamFont::setFillMsb(int on)
{
    m_fill_msb = on;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001bff64

void GamFont::setColor(u_int color)
{
    if (m_dst_bits == 4) {
        m_color = color & 0xf;
        return;
    }
    if (m_dst_bits == 8) {
        m_color = color & 0xff;
        if (m_fill_msb != 0)
            m_color = (color & 0xff) | 0x80;
    }
}

// ──────────────────────────────────────────────────────────────────────
// getGlyph's bsearch order over u16 codes (folded into setColor's body in
// Ghidra until this session).
// ROR 0x001bffac

static int gamFontCompareCode(const void *a, const void *b)
{
    u_short ka = *(const u_short *)a;
    u_short kb = *(const u_short *)b;
    if (ka < kb)
        return -1;
    return kb < ka;
}

// ──────────────────────────────────────────────────────────────────────
// Below 0x8140, or in a font without kanji, the code table's index; a
// kanji's glyph comes after the table, in row order.
// ROR 0x001bffcc

int GamFont::getGlyph(int code)
{
    u_short key = (u_short)code;
    if ((m_font->m_flags & (GAMFONT_KANJI1 | GAMFONT_KANJI2)) == 0 || key < 0x8140) {
        const u_short *p = (const u_short *)bsearch(&key, m_codes, m_code_count, 2,
                                                    gamFontCompareCode);
        if (p == NULL)
            return -1;
        return (int)(p - m_codes);
    }
    int glyph = getKanjiIndex(key);
    if (glyph >= 0)
        glyph += m_code_count;
    return glyph;
}

// ──────────────────────────────────────────────────────────────────────
// Fixed-pitch fonts and codes the font lacks give no spacing.
// ROR 0x001c0078

void GamFont::getCharMetrics(int code, int *left, int *width, int *extra)
{
    if (code < 0x20) {
        *left = 0;
        *width = 0;
    } else if (m_prop == 0) {
        *left = 0;
        *width = getCharWidth(code);
    } else {
        int glyph = getGlyph(code);
        if (glyph >= 0) {
            *left = getGlyphLeft(glyph);
            *width = getGlyphWidth(glyph);
            *extra = m_prop == 0 ? m_spacing : m_spacing_prop;
            return;
        }
        *left = 0;
        *width = m_char_w;
    }
    *extra = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Proportional: the width table's (6 for the two spaces); fixed: half the
// cell for single-byte codes, the cell for the rest above 0x20.
// ROR 0x001c0148

int GamFont::getCharWidth(int code)
{
    if (m_prop == 1 && m_prop_on == 1) {
        if (code == 0x20 || code == 0x8140)
            return 6;
        return getGlyphWidth(getGlyph(code));
    }
    if (isHalfWidth(code) == 1)
        return m_char_w / 2;
    if (code > 0x20)
        return m_char_w;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c020c

int GamFont::getGlyphLeft(int glyph)
{
    if (m_prop == 1 && m_prop_on == 1) {
        if (glyph < 0)
            return m_char_w;
        return m_widths[glyph * 2];
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The ROM masks the index to a byte (andi 0xff): glyphs from 128 on read
// the width of glyph & 0x7f. Kept.
// ROR 0x001c0254

int GamFont::getGlyphWidth(int glyph)
{
    if (m_prop != 0 && m_prop_on != 0 && glyph >= 0)
        return m_widths[(glyph * 2 + 1) & 0xff];
    return m_char_w;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c0298

int GamFont::isHalfWidth(int code)
{
    return (u_int)(code - 0x20) < 0x60 || (u_int)(code - 0xa0) < 0x40;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c02b8

int GamFont::isSjisCode(u_int code)
{
    return (code & 0xff00) - 0x8000 < 0x1f01 || (code & 0xff00) - 0xe000 < 0xa01;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c02e4

int GamFont::isSingleByte(int code)
{
    return (u_int)(code - 0x20) < 0xe0;
}

// ──────────────────────────────────────────────────────────────────────
// A byte that is neither a half-width code nor a Shift-JIS lead is skipped
// (its follower is read as the next lead). 0x8164 and 0x8149 have fixed
// advances.
// ROR 0x001c02f8

int GamFont::calcTextWidth(const char *text)
{
    const u_char *s = (const u_char *)text;
    int w = 0;
    for (;;) {
        u_int code = *s++;
        if (code == 0)
            return w;
        int cw;
        if (isHalfWidth(code) == 1) {
            cw = getCharWidth(code);
        } else {
            code = (u_short)(code << 8 | *s);
            if (isSjisCode(code) != 1)
                continue;
            s++;
            if (code == 0x8164)
                cw = 0x12;
            else if (code == 0x8149)
                cw = 10;
            else
                cw = getCharWidth(code);
        }
        w += cw + (m_prop == 0 ? m_spacing : m_spacing_prop);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The two fixed advances cannot match a byte; kept as the ROM has them.
// ROR 0x001c03d0

int GamFont::calcTextWidth8(const char *text)
{
    const u_char *s = (const u_char *)text;
    int w = 0;
    for (;;) {
        u_int code = *s++;
        if (code == 0)
            return w;
        if (isSingleByte(code) != 1)
            continue;
        int cw;
        if (code == 0x8164)
            cw = 0x12;
        else if (code == 0x8149)
            cw = 10;
        else
            cw = getCharWidth(code);
        w += cw + (m_prop == 0 ? m_spacing : m_spacing_prop);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c047c

int GamFont::getTextWidth(const char *text)
{
    return calcTextWidth(text);
}

// ──────────────────────────────────────────────────────────────────────
// calcTextWidth's walk, every code through getCharWidth, no spacing.
// ROR 0x001c0490

int GamFont::calcTextWidthNoSpace(const char *text)
{
    const u_char *s = (const u_char *)text;
    int w = 0;
    for (;;) {
        u_int code = *s++;
        if (code == 0)
            return w;
        if (isHalfWidth(code) != 1) {
            code = (u_short)(code << 8 | *s);
            if (isSjisCode(code) != 1)
                continue;
            s++;
        }
        w += getCharWidth(code);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c0534

int GamFont::getKanjiIndex(int code)
{
    u_int c = code & 0xffff;
    u_int index = 0;
    for (const GamFontRow *r = &s_sjisRows[1]; r->m_start != 0; r++) {
        if (r->m_start <= c && c < (u_int)r->m_start + r->m_count)
            return index + (c - r->m_start);
        index = (index + r->m_count) & 0xffff;
    }
    return -1;
}
