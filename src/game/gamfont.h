/* ==========================================================================
 *  game/gamfont.h
 *
 *  The font rasterizer (game/gamfont.cpp, 0x1beb70..0x1c05c0; RoR does not
 *  name the file -- game/ by link order, after the class): a GamFont draws
 *  a font file's 1- or 2-bit glyphs into a 4- or 8-bit pixel buffer that
 *  the caller then uploads as a texture (a CLUT image). SprMan's text
 *  sprites have one, MagScene's four text windows one each (+0x2744), and
 *  MagLifeWindow draws through its window's.
 *
 *  Codes are Shift-JIS: single bytes (ASCII, half-width kana) and the
 *  two-byte rows 0x81..0x9f / 0xe0..0xea. A font lists its codes in a
 *  sorted u16 table (binary search); a font with kanji flags appends the
 *  JIS level-1 and / or level-2 kanji in code order, found by walking the
 *  row table at data 0x2b8df0 instead.
 * ======================================================================== */
#ifndef _GAME_GAMFONT_H
#define _GAME_GAMFONT_H

#include <stddef.h>

#include "sdk/scetypes.h"

/* The font file's header (MagScene's m_font_data, SprMan's font files).
 * The table offsets count from +4. EE layout (file bytes). */
struct GamFontHeader
{
    u_int   m_00;               // ROR 0x00
    u_int   m_04;               // ROR 0x04
    u_int   m_flags;            // ROR 0x08: GAMFONT_KANJI1 / GAMFONT_KANJI2 (byte 0xb bits 1, 2)
    u_short m_count;            // ROR 0x0c: glyphs in the file
    u_char  m_char_w;           // ROR 0x0e: the glyph cell
    u_char  m_char_h;           // ROR 0x0f
    u_char  m_bpp;              // ROR 0x10: 1 or 2 bits per pixel
    u_char  m_11[3];            // ROR 0x11
    u_int   m_code_ofs;         // ROR 0x14: the sorted u16 code table, 0 = none
    u_int   m_18;               // ROR 0x18
    u_int   m_width_ofs;        // ROR 0x1c: (left, width) bytes per glyph, 0 = fixed pitch
    u_int   m_glyph_ofs;        // ROR 0x20: the glyph bitmaps, rows MSB first
};
static_assert(offsetof(GamFontHeader, m_flags) == 0x08, "m_flags");
static_assert(offsetof(GamFontHeader, m_count) == 0x0c, "m_count");
static_assert(offsetof(GamFontHeader, m_char_w) == 0x0e, "m_char_w");
static_assert(offsetof(GamFontHeader, m_char_h) == 0x0f, "m_char_h");
static_assert(offsetof(GamFontHeader, m_bpp) == 0x10, "m_bpp");
static_assert(offsetof(GamFontHeader, m_code_ofs) == 0x14, "m_code_ofs");
static_assert(offsetof(GamFontHeader, m_width_ofs) == 0x1c, "m_width_ofs");
static_assert(offsetof(GamFontHeader, m_glyph_ofs) == 0x20, "m_glyph_ofs");
static_assert(sizeof(GamFontHeader) == 0x24, "GamFontHeader");

/* The font holds the JIS level-1 kanji (0x889f..0x9872, 0x1130 glyphs) /
 * the level-2 kanji (0x989f..0xeaa4, 0xde0 glyphs) after its code table. */
#define GAMFONT_KANJI1  0x02000000
#define GAMFONT_KANJI2  0x04000000

/* EE 0x6c. layout: host -- the buffer and font pointers (offsets asserted
 * up to the first one; the EE offsets in comments). */
struct GamFont
{
    GamFont();                                                           // ROR 0x001beb70
    ~GamFont();                                                          // ROR 0x001bebc4

    /* Reads a font file's header: cell, depth, the code / width / glyph
     * tables. NULL keeps the current font. */
    void setFont(const void *font);                                      // ROR 0x001bebec
    /* The pixel buffer the glyphs go to: w & 0xff80, h & 0xff80 (4 bits)
     * or 0xffc0 (8 bits). 1 when set. */
    int  setTarget(void *buf, u_int w, u_int h, int bits);               // ROR 0x001bedc0
    /* Forgets the buffer; 1 when there was one. */
    int  releaseTarget();                                                // ROR 0x001bee8c
    /* Fills the whole buffer with m_fill; 1 when there is one. */
    int  clear();                                                        // ROR 0x001beeb4
    /* Half the cell for a single-byte code, else the cell. No callers. */
    int  getCellWidth(int code);                                         // ROR 0x001beeec
    /* Fills the rect with m_fill; 1 when there is a buffer. */
    int  fillRect(int x, int y, int w, int h);                           // ROR 0x001bef24
    /* ORs a code's glyph into the buffer at (x, y) in m_color. */
    void drawChar(int x, int y, int code);                               // ROR 0x001bf024
    /* drawChar storing instead of ORing: the glyph's clear pixels become
     * 0, a 4-bit x rounds down to even. No callers. */
    void putChar(int x, int y, int code);                                // ROR 0x001bf8c4
    /* Scrolls the buffer by rows (< 0: up), filling the freed rows with
     * m_fill; nothing beyond +-m_dst_h. */
    void scroll(int rows);                                               // ROR 0x001bfe30
    /* m_fill = fill (| 0x80 for 8-bit targets while m_fill_msb is set). */
    void setFill(u_int fill);                                            // ROR 0x001bff28
    void setFillMsb(int on);                                             // ROR 0x001bff5c
    /* m_color = color & 0xf (4 bits) or & 0xff (8 bits, | 0x80 while
     * m_fill_msb is set). */
    void setColor(u_int color);                                          // ROR 0x001bff64
    /* The glyph of a code, -1 when the font has none. */
    int  getGlyph(int code);                                             // ROR 0x001bffcc
    /* A character's left offset, width and spacing (m_spacing_prop for
     * proportional fonts); all 0 below 0x20. */
    void getCharMetrics(int code, int *left, int *width, int *extra);    // ROR 0x001c0078
    /* A code's advance without the spacing. */
    int  getCharWidth(int code);                                         // ROR 0x001c0148
    int  getGlyphLeft(int glyph);                                        // ROR 0x001c020c
    /* The glyph's width (proportional fonts), else m_char_w. */
    int  getGlyphWidth(int glyph);                                       // ROR 0x001c0254
    /* 0x20..0x7f, 0xa0..0xdf: ASCII and half-width kana. */
    int  isHalfWidth(int code);                                          // ROR 0x001c0298
    /* A two-byte code with lead byte 0x80..0x9f or 0xe0..0xea. */
    int  isSjisCode(u_int code);                                         // ROR 0x001c02b8
    /* 0x20..0xff. */
    int  isSingleByte(int code);                                         // ROR 0x001c02e4
    /* The width of a Shift-JIS string, spacing included. */
    int  calcTextWidth(const char *text);                                // ROR 0x001c02f8
    /* calcTextWidth over single-byte codes only. No callers. */
    int  calcTextWidth8(const char *text);                               // ROR 0x001c03d0
    /* The same as calcTextWidth (a tail jump). */
    int  getTextWidth(const char *text);                                 // ROR 0x001c047c
    /* calcTextWidth without the spacing. No callers. */
    int  calcTextWidthNoSpace(const char *text);                         // ROR 0x001c0490
    /* A Shift-JIS code's index among the font's kanji rows, -1 outside. */
    int  getKanjiIndex(int code);                                        // ROR 0x001c0534

    int            m_00;            // ROR 0x00
    int            m_04;            // ROR 0x04: 0 whenever the buffer changes
    int            m_08;            // ROR 0x08
    u_char        *m_dst;           // ROR 0x0c: setTarget's buffer
    int            m_dst_w;         // ROR 0x10: pixels
    int            m_dst_h;         // ROR 0x14
    int            m_dst_bits;      // ROR 0x18: 4 or 8
    int            m_dst_size;      // ROR 0x1c: bytes
    int            m_20;            // ROR 0x20
    int            m_prop;          // ROR 0x24: the font has a width table
    int            m_prop_on;       // ROR 0x28: use it (MagLifeWindow::setProportional)
    int            m_color;         // ROR 0x2c: setColor
    int            m_spacing;       // ROR 0x30: added per character, fixed-pitch fonts
    int            m_spacing_prop;  // ROR 0x34: added per character, proportional fonts
    int            m_char_w;        // ROR 0x38: the glyph cell (font header +0xe)
    int            m_char_h;        // ROR 0x3c: (+0xf), the line height
    int            m_bpp;           // ROR 0x40: (+0x10) 1 or 2
    int            m_code_count;    // ROR 0x44: codes in the code table (= the first kanji glyph)
    int            m_font_bytes;    // ROR 0x48: every glyph's bitmap
    int            m_row_bytes;     // ROR 0x4c: a glyph row
    int            m_glyph_bytes;   // ROR 0x50: a glyph
    const GamFontHeader *m_font;    // ROR 0x54: setFont's file
    const u_short *m_codes;         // ROR 0x58: sorted codes, glyph = index
    const u_char  *m_widths;        // ROR 0x5c: (left, width) per glyph
    const u_char  *m_glyphs;        // ROR 0x60: the bitmaps
    int            m_fill;          // ROR 0x64: clear / fillRect / scroll's colour
    int            m_fill_msb;      // ROR 0x68: 8-bit fill and colour get bit 7
};
static_assert(offsetof(GamFont, m_00) == 0x00, "m_00");
static_assert(offsetof(GamFont, m_04) == 0x04, "m_04");
static_assert(offsetof(GamFont, m_08) == 0x08, "m_08");

#endif /* _GAME_GAMFONT_H */
