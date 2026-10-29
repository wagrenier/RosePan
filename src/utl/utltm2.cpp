// utl/utltm2.cpp -- Rule of Rose module: TIM2 helpers (ROR 0x104e1c..0x105650).
// The size of a TIM2 file, picture lookup, and the sepia tint DogTexInfo
// applies to a picture's image or CLUT.

#include "utl/utltm2.h"

#include <stddef.h>

#include "common/ror_assert.h"
#include "i3d/i3dtim2.h"

/* The tint utlTm2ToSepia sets for the colour loops (data 0x308488..). */
static int s_sepiaR;   // ROR 0x00308488
static int s_sepiaG;   // ROR 0x0030848c
static int s_sepiaB;   // ROR 0x00308490

// ──────────────────────────────────────────────────────────────────────
// The whole file's size: the file header (0x10 bytes, 0x80 when m_format
// is set) and every picture's m_total_size.
// ROR 0x00104e1c

int utlTm2GetSize(const void *tm2)
{
    const I3dTim2File *h = (const I3dTim2File *)tm2;
    u_int n = h->m_pictures;
    int size = h->m_format != 0 ? 0x80 : 0x10;
    for (; n != 0; n--)
        size += ((const I3dTim2Picture *)((const u_char *)tm2 + size))->m_total_size;
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// Picture theN's header: the pictures follow the file header (0x10
// bytes, 0x80 when m_format is set), each m_total_size long.
// callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
// ROR 0x00104e64

const I3dTim2Picture *utlTm2GetPicture(const void *tm2, int theN)
{
    const I3dTim2File *h = (const I3dTim2File *)tm2;
    int off = h->m_format != 0 ? 0x80 : 0x10;
    ROR_ASSERT(theN < h->m_pictures); /* 125: theN < h->Pictures */
    const I3dTim2Picture *pic = NULL;
    for (int i = 0; i <= theN; i++) {
        pic = (const I3dTim2Picture *)((const u_char *)tm2 + off);
        off += pic->m_total_size;
    }
    return pic;
}

/* The tint of one colour: its luminance (0x4d / 0x96 / 0x1d, sum 0x100)
 * times the sepia r, g, b, >> 16. The ROM inlines this into each colour
 * loop (the asserts at lines 147-149 in all three). */
static inline void utlTm2SepiaRgb(int lum, int *r, int *g, int *b)
{
    int theR = (s_sepiaR * lum) >> 16;
    int theG = (s_sepiaG * lum) >> 16;
    int theB = (s_sepiaB * lum) >> 16;
    ROR_ASSERT(theR >= 0 && theR < 256); /* 147 */
    ROR_ASSERT(theG >= 0 && theG < 256); /* 148 */
    ROR_ASSERT(theB >= 0 && theB < 256); /* 149 */
    *r = theR;
    *g = theG;
    *b = theB;
}

// ──────────────────────────────────────────────────────────────────────
// Tints n 16-bit colours (5:5:5, each channel widened to 8 bits as
// c * 8 + c / 4). The result keeps no STP bit 15: the ROM writes only the
// three channels.
// callcheck: dbgAssertFailed (inside ROR_ASSERT)
// ROR 0x00104ef8

static void utlTm2SepiaColors16(u_short *c, int n)
{
    for (; n > 0; n--, c++) {
        u_int v = *c;
        int r5 = v & 0x1f, g5 = (v >> 5) & 0x1f, b5 = (v >> 10) & 0x1f;
        int lum = (r5 * 8 + (r5 >> 2)) * 0x4d + (g5 * 8 + (g5 >> 2)) * 0x96 +
                  (b5 * 8 + (b5 >> 2)) * 0x1d;
        int r, g, b;
        utlTm2SepiaRgb(lum, &r, &g, &b);
        *c = (u_short)((r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10));
    }
}

// ──────────────────────────────────────────────────────────────────────
// Image type 1 (16-bit): the image follows the picture header.
// ROR 0x001050b0

static void utlTm2SepiaImage16(I3dTim2Picture *pic)
{
    utlTm2SepiaColors16((u_short *)((u_char *)pic + pic->m_header_size), (int)pic->m_image_size / 2);
}

// ──────────────────────────────────────────────────────────────────────
// Tints n 24-bit RGB colours.
// callcheck: dbgAssertFailed (inside ROR_ASSERT)
// ROR 0x001050e0

static void utlTm2SepiaColors24(u_char *c, int n)
{
    for (; n > 0; n--, c += 3) {
        int lum = c[0] * 0x4d + c[1] * 0x96 + c[2] * 0x1d;
        int r, g, b;
        utlTm2SepiaRgb(lum, &r, &g, &b);
        c[0] = (u_char)r;
        c[1] = (u_char)g;
        c[2] = (u_char)b;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Image type 2 (24-bit).
// ROR 0x00105254

static void utlTm2SepiaImage24(I3dTim2Picture *pic)
{
    utlTm2SepiaColors24((u_char *)pic + pic->m_header_size, (int)pic->m_image_size / 3);
}

// ──────────────────────────────────────────────────────────────────────
// Tints n 32-bit RGBA colours; alpha stays.
// callcheck: dbgAssertFailed (inside ROR_ASSERT)
// ROR 0x0010528c

static void utlTm2SepiaColors32(u_char *c, int n)
{
    for (; n > 0; n--, c += 4) {
        int lum = c[0] * 0x4d + c[1] * 0x96 + c[2] * 0x1d;
        int r, g, b;
        utlTm2SepiaRgb(lum, &r, &g, &b);
        c[0] = (u_char)r;
        c[1] = (u_char)g;
        c[2] = (u_char)b;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Image type 3 (32-bit).
// ROR 0x00105400

static void utlTm2SepiaImage32(I3dTim2Picture *pic)
{
    utlTm2SepiaColors32((u_char *)pic + pic->m_header_size, (int)pic->m_image_size / 4);
}

// ──────────────────────────────────────────────────────────────────────
// Image type 4 (4-bit index): tints the CLUT after the image, by its
// colour format (m_clut_type & 0x3f).
// callcheck: dbgAssertFailed (inside ROR_ASSERT)
// ROR 0x00105434

static void utlTm2SepiaClut4(I3dTim2Picture *pic)
{
    u_char *clut = (u_char *)pic + pic->m_header_size + pic->m_image_size;
    switch (pic->m_clut_type & 0x3f) {
    case 1: utlTm2SepiaColors16((u_short *)clut, pic->m_clut_colors); return;
    case 2: utlTm2SepiaColors24(clut, pic->m_clut_colors); return;
    case 3: utlTm2SepiaColors32(clut, pic->m_clut_colors); return;
    }
    ROR_ASSERT(0); /* 270 */
}

// ──────────────────────────────────────────────────────────────────────
// Image type 5 (8-bit index): the same CLUT tint (its own function in
// the ROM, assert line 292).
// callcheck: dbgAssertFailed (inside ROR_ASSERT)
// ROR 0x001054e8

static void utlTm2SepiaClut8(I3dTim2Picture *pic)
{
    u_char *clut = (u_char *)pic + pic->m_header_size + pic->m_image_size;
    switch (pic->m_clut_type & 0x3f) {
    case 1: utlTm2SepiaColors16((u_short *)clut, pic->m_clut_colors); return;
    case 2: utlTm2SepiaColors24(clut, pic->m_clut_colors); return;
    case 3: utlTm2SepiaColors32(clut, pic->m_clut_colors); return;
    }
    ROR_ASSERT(0); /* 292 */
}

// ──────────────────────────────────────────────────────────────────────
// Tints the first picture: direct colours in the image, indexed pictures
// through their CLUT.
// callcheck: dbgAssertFailed (inside ROR_ASSERT)
// ROR 0x0010559c

void utlTm2ToSepia(void *tm2, int r, int g, int b)
{
    s_sepiaR = r;
    s_sepiaG = g;
    s_sepiaB = b;
    I3dTim2Picture *pic = (I3dTim2Picture *)utlTm2GetPicture(tm2, 0);
    switch (pic->m_image_type) {
    case 1: utlTm2SepiaImage16(pic); return;
    case 2: utlTm2SepiaImage24(pic); return;
    case 3: utlTm2SepiaImage32(pic); return;
    case 4: utlTm2SepiaClut4(pic); return;
    case 5: utlTm2SepiaClut8(pic); return;
    }
    ROR_ASSERT(0); /* 336 */
}
