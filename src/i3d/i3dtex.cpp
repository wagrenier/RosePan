/* ==========================================================================
 *  i3d/i3dtex.cpp
 *
 *  The i3d library's texture and CLUT objects (ROR 0x2184c0..0x21a120):
 *  the PSMT8 / PSMT4 swizzle helpers, the GS pixel format tables, the GS
 *  transfer packets the VRAM cache emits, and the objects' setters. See
 *  i3dtex.h.
 *
 *  PORT: the transfer packets carry the ROM's VIF / GIF words, their REF
 *  tags the low 28 bits of host pointers (i3dDmaAddr); nothing consumes
 *  them yet (no GS emulation: a host draw module will read the objects --
 *  their levels' data, sizes and formats -- instead of the packets).
 * ======================================================================== */
#include "i3d/i3dtex.h"
#include "i3d/i3dctx.h"
#include "i3d/i3dpacket.h"

#include <string.h>

#include "rosepan/rendering/rosepan_i3d.h"

/* GIF tags: A+D with NLOOP 4 (the transfer registers), IMAGE (FLG 2). */
#define I3D_GIF_AD4_EOP   0x1000000000008004ull
#define I3D_GIF_IMAGE_EOP 0x0800000000008000ull

/* GS registers of a transfer. */
#define I3D_GS_BITBLTBUF 0x50ull
#define I3D_GS_TRXPOS    0x51ull
#define I3D_GS_TRXREG    0x52ull
#define I3D_GS_TRXDIR    0x53ull

/* ---- PSMT8 / PSMT4 -> PSMCT32 swizzle tables ---- */

/* Where each 8x8-word PSMCT32 block of a page goes: (x, y) in words. */
static const u_char s_ct32BlockPos[64] = {      /* data 0x301e78 */
    0, 0, 8, 0, 0, 8, 8, 8, 16, 0, 24, 0, 16, 8, 24, 8,
    0, 16, 8, 16, 0, 24, 8, 24, 16, 16, 24, 16, 16, 24, 24, 24,
    32, 0, 40, 0, 32, 8, 40, 8, 48, 0, 56, 0, 48, 8, 56, 8,
    32, 16, 40, 16, 32, 24, 40, 24, 48, 16, 56, 16, 48, 24, 56, 24
};

/* The nibble order of a PSMT4 block's even / odd 128-nibble column. */
static const u_char s_t4ColumnEven[128] = {     /* data 0x301eb8 */
    0, 68, 8, 76, 16, 84, 24, 92, 1, 69, 9, 77, 17, 85, 25, 93,
    2, 70, 10, 78, 18, 86, 26, 94, 3, 71, 11, 79, 19, 87, 27, 95,
    4, 64, 12, 72, 20, 80, 28, 88, 5, 65, 13, 73, 21, 81, 29, 89,
    6, 66, 14, 74, 22, 82, 30, 90, 7, 67, 15, 75, 23, 83, 31, 91,
    32, 100, 40, 108, 48, 116, 56, 124, 33, 101, 41, 109, 49, 117, 57, 125,
    34, 102, 42, 110, 50, 118, 58, 126, 35, 103, 43, 111, 51, 119, 59, 127,
    36, 96, 44, 104, 52, 112, 60, 120, 37, 97, 45, 105, 53, 113, 61, 121,
    38, 98, 46, 106, 54, 114, 62, 122, 39, 99, 47, 107, 55, 115, 63, 123
};
static const u_char s_t4ColumnOdd[128] = {      /* data 0x301f38 */
    4, 64, 12, 72, 20, 80, 28, 88, 5, 65, 13, 73, 21, 81, 29, 89,
    6, 66, 14, 74, 22, 82, 30, 90, 7, 67, 15, 75, 23, 83, 31, 91,
    0, 68, 8, 76, 16, 84, 24, 92, 1, 69, 9, 77, 17, 85, 25, 93,
    2, 70, 10, 78, 18, 86, 26, 94, 3, 71, 11, 79, 19, 87, 27, 95,
    36, 96, 44, 104, 52, 112, 60, 120, 37, 97, 45, 105, 53, 113, 61, 121,
    38, 98, 46, 106, 54, 114, 62, 122, 39, 99, 47, 107, 55, 115, 63, 123,
    32, 100, 40, 108, 48, 116, 56, 124, 33, 101, 41, 109, 49, 117, 57, 125,
    34, 102, 42, 110, 50, 118, 58, 126, 35, 103, 43, 111, 51, 119, 59, 127
};

/* The byte order of a PSMT8 block's even / odd 64-byte column. */
static const u_char s_t8ColumnEven[64] = {      /* data 0x301fb8 */
    0, 36, 8, 44, 1, 37, 9, 45, 2, 38, 10, 46, 3, 39, 11, 47,
    4, 32, 12, 40, 5, 33, 13, 41, 6, 34, 14, 42, 7, 35, 15, 43,
    16, 52, 24, 60, 17, 53, 25, 61, 18, 54, 26, 62, 19, 55, 27, 63,
    20, 48, 28, 56, 21, 49, 29, 57, 22, 50, 30, 58, 23, 51, 31, 59
};
static const u_char s_t8ColumnOdd[64] = {       /* data 0x301ff8 */
    4, 32, 12, 40, 5, 33, 13, 41, 6, 34, 14, 42, 7, 35, 15, 43,
    0, 36, 8, 44, 1, 37, 9, 45, 2, 38, 10, 46, 3, 39, 11, 47,
    20, 48, 28, 56, 21, 49, 29, 57, 22, 50, 30, 58, 23, 51, 31, 59,
    16, 52, 24, 60, 17, 53, 25, 61, 18, 54, 26, 62, 19, 55, 27, 63
};

/* Where each 32x16 PSMT4 block of a page is read: (x, y) in nibbles. */
static const u_char s_t4BlockPos[64] = {        /* data 0x302038 */
    0, 0, 0, 16, 32, 0, 32, 16, 0, 32, 0, 48, 32, 32, 32, 48,
    64, 0, 64, 16, 96, 0, 96, 16, 64, 32, 64, 48, 96, 32, 96, 48,
    0, 64, 0, 80, 32, 64, 32, 80, 0, 96, 0, 112, 32, 96, 32, 112,
    64, 64, 64, 80, 96, 64, 96, 80, 64, 96, 64, 112, 96, 96, 96, 112
};

/* Where each 16x16 PSMT8 block of a page is read: (x, y) in bytes. */
static const u_char s_t8BlockPos[64] = {        /* data 0x302078 */
    0, 0, 16, 0, 0, 16, 16, 16, 32, 0, 48, 0, 32, 16, 48, 16,
    0, 32, 16, 32, 0, 48, 16, 48, 32, 32, 48, 32, 32, 48, 48, 48,
    64, 0, 80, 0, 64, 16, 80, 16, 96, 0, 112, 0, 96, 16, 112, 16,
    64, 32, 80, 32, 64, 48, 80, 48, 96, 32, 112, 32, 96, 48, 112, 48
};

/* ---- the GS block layout of a page, per format (i3dPsmBlockEnd) ---- */

static const u_char s_blockCT32[32] = {         /* data 0x302220: 8 x 4 blocks of 8x8 */
    0, 1, 4, 5, 16, 17, 20, 21, 2, 3, 6, 7, 18, 19, 22, 23,
    8, 9, 12, 13, 24, 25, 28, 29, 10, 11, 14, 15, 26, 27, 30, 31
};
static const u_char s_blockCT16[32] = {         /* data 0x302240: 4 x 8 blocks of 16x8 */
    0, 2, 8, 10, 1, 3, 9, 11, 4, 6, 12, 14, 5, 7, 13, 15,
    16, 18, 24, 26, 17, 19, 25, 27, 20, 22, 28, 30, 21, 23, 29, 31
};
static const u_char s_blockCT16S[32] = {        /* data 0x302260 */
    0, 2, 16, 18, 1, 3, 17, 19, 8, 10, 24, 26, 9, 11, 25, 27,
    9, 11, 25, 27, 9, 11, 25, 27, 12, 14, 28, 30, 13, 15, 29, 31
};
static const u_char s_blockT8[32] = {           /* data 0x302280: 8 x 4 blocks of 16x16 */
    0, 1, 4, 5, 16, 17, 20, 21, 2, 3, 6, 7, 18, 19, 22, 23,
    8, 9, 12, 13, 24, 25, 28, 29, 10, 11, 14, 15, 26, 27, 30, 31
};
static const u_char s_blockT4[32] = {           /* data 0x3022a0: 4 x 8 blocks of 32x16 */
    0, 2, 8, 10, 1, 3, 9, 11, 4, 6, 12, 14, 5, 7, 13, 15,
    16, 18, 24, 26, 17, 19, 25, 27, 20, 22, 28, 30, 21, 23, 29, 31
};

/* ---- PSMT8 / PSMT4 -> PSMCT32 ---- */

// ROR 0x002184c0
void i3dNibbleSet(u_char *p, u_int i, u_char v)
{
    u_char *b = p + (i >> 1);
    if ((i & 1) == 0)
        *b = (u_char)((*b & 0xf0) | (v & 0xf));
    else
        *b = (u_char)((*b & 0xf) | (v << 4));
}

// ROR 0x00218500
u_char i3dNibbleGet(const u_char *p, u_int i)
{
    const u_char *b = p + (i >> 1);
    if ((i & 1) == 0)
        return *b & 0xf;
    return *b >> 4;
}

// A 32x16 PSMT4 block (0x100 bytes) in PSMCT32 column order: each 64-byte
// column through its nibble table.
// ROR 0x00218528
void i3dSwizzleT4Block(const u_char *src, u_char *dst)
{
    u_int col;
    u_int i;

    for (col = 0; col < 4; col++)
    {
        const u_char *table = (col & 1) == 0 ? s_t4ColumnEven : s_t4ColumnOdd;
        u_int off = col * 0x40;
        for (i = 0; i < 0x80; i++)
            i3dNibbleSet(dst + off, i, i3dNibbleGet(src + off, table[i]));
    }
}

// A 128x64 PSMT8 page (src, 0x80 bytes a row) as a 64x32-word PSMCT32 page
// (dst, 0x40 words a row): block by block.
// ROR 0x00218638
void i3dSwizzleT8Page(const u_char *src, u_int *dst)
{
    u_int buf[0x80];    /* the block, then its swizzled copy at +0x100 bytes */
    u_char *blk = (u_char *)buf;
    u_int b;
    u_int y;
    u_int x;

    for (b = 0; b < 0x20; b++)
    {
        const u_char *s = src + s_t8BlockPos[b * 2 + 1] * 0x80 + s_t8BlockPos[b * 2];
        for (y = 0; y < 0x10; y++)
            for (x = 0; x < 0x10; x++)
                blk[y * 0x10 + x] = s[y * 0x80 + x];
        i3dSwizzleT8Block(blk, blk + 0x100);

        u_int *d = dst + s_ct32BlockPos[b * 2 + 1] * 0x40 + s_ct32BlockPos[b * 2];
        const u_int *w = buf + 0x40;
        for (y = 0; y < 8; y++)
            for (x = 0; x < 8; x++)
                d[y * 0x40 + x] = w[y * 8 + x];
    }
}

// Interleaves each group of rows of a PSMT8 (2 rows of w / 4 words, 32-word
// runs) or PSMT4 (4 rows of w / 8 words, 16-word runs) image into one
// PSMCT32 row of lw = w / 2 words. Inlined twice in i3dTexSwizzle.
static void i3dTexSwizzleRows(u_int *data, u_int lw, u_int lh, u_int rowlen, u_int run,
                              u_int *line, u_int *tmp)
{
    u_int r;
    u_int j;
    u_int base = 0;

    for (r = 0; r < lh; r++, base += lw)
    {
        for (j = 0; j < lw; j++)
            line[j] = data[base + j];
        for (j = 0; j < lw; j++)
            tmp[((j & (run - 1)) + (j / run) * 0x40) % lw | (j / rowlen) * run] = line[j];
        for (j = 0; j < lw; j++)
            data[base + j] = tmp[j];
    }
}

// Copies a 64x32-word page out of (back = 0) or into (back = 1) an image lw
// words wide. Inlined in i3dTexSwizzle.
static void i3dTexSwizzleCopyPage(u_int *data, u_int lw, u_int px, u_int py, u_int *page, int back)
{
    u_int k = 0;
    u_int y;
    u_int x;

    for (y = py * 0x20; y < py * 0x20 + 0x20; y++)
        for (x = px * 0x40; x < px * 0x40 + 0x40; x++, k++)
        {
            if (back)
                data[y * lw + x] = page[k];
            else
                page[k] = data[y * lw + x];
        }
}

// Rewrites the texture's PSMT8 levels (w a multiple of 128, h of 64) and
// PSMT4 levels (w and h multiples of 128) in place as PSMCT32 images of
// w / 2 x h / 2 (h / 4) and points their BITBLTBUF / TRXREG at that. No
// caller in the ROM.
// ROR 0x00218778
int i3dTexSwizzle(I3dTexture *tex)
{
    static u_int s_line[0x400];
    static u_int s_tmp[0x400];
    static u_int s_page[0x800];
    static u_int s_out[0x800];
    u_int i;

    for (i = 0; i < tex->m_levels; i++)
    {
        I3dTexLevel *lv = &tex->m_level[i];
        u_int psm = (u_int)(lv->m_bitbltbuf >> 24) & 0x3f;
        u_int w = (u_int)lv->m_trxreg & 0xfff;
        u_int h = (u_int)(lv->m_trxreg >> 32) & 0xfff;
        u_int *data = (u_int *)lv->m_data;
        u_int lw = 0;
        u_int lh = 0;
        int done = 0;
        u_int py;
        u_int px;

        if (psm == I3D_PSMT8)
        {
            if ((w & 0x7f) == 0 && (h & 0x3f) == 0)
            {
                lh = h >> 1;
                lw = w >> 1;
                i3dTexSwizzleRows(data, lw, lh, w >> 2, 0x20, s_line, s_tmp);
                for (py = 0; py < h >> 6; py++)
                    for (px = 0; px < w >> 7; px++)
                    {
                        i3dTexSwizzleCopyPage(data, lw, px, py, s_page, 0);
                        i3dSwizzleT8Page((const u_char *)s_page, s_out);
                        i3dTexSwizzleCopyPage(data, lw, px, py, s_out, 1);
                    }
                done = 1;
            }
        }
        else if (psm == I3D_PSMT4 && (w & 0x7f) == 0 && (h & 0x7f) == 0)
        {
            lh = h >> 2;
            lw = w >> 1;
            i3dTexSwizzleRows(data, lw, lh, w >> 3, 0x10, s_line, s_tmp);
            for (py = 0; py < h >> 7; py++)
                for (px = 0; px < w >> 7; px++)
                {
                    i3dTexSwizzleCopyPage(data, lw, px, py, s_page, 0);
                    i3dSwizzleT4Page((const u_char *)s_page, s_out);
                    i3dTexSwizzleCopyPage(data, lw, px, py, s_out, 1);
                }
            done = 1;
        }

        if (done)
        {
            u_long bw = (lw + 0x3f) >> 6;
            u_long bb = lv->m_bitbltbuf;
            /* SBP / DBP kept, the widths new, both formats PSMCT32 (0) */
            lv->m_bitbltbuf = (bb & 0x3fff) | (bw << 48) | (bb & 0x3fff00000000ull) | (bw << 16);
            lv->m_trxreg = (u_long)(lw & 0x7ff) | ((u_long)lh << 32);
        }
    }
    return 0;
}

/* ---- pixel formats ---- */

// ceil(log2 n): the ROM's 31 - plzcw(n - 1) (1 and 0 give 0).
// ROR 0x00218d40
int i3dLog2(int n)
{
    int x = n - 1;
    u_int v = x < 0 ? ~(u_int)x : (u_int)x;
    int lz = 0;

    while (lz < 32 && (v & (0x80000000u >> lz)) == 0)
        lz++;
    return 0x1f - (lz - 1);
}

// ROR 0x00218d58
int i3dPsmTexelBits(int psm)
{
    switch (psm)
    {
    case I3D_PSMCT32:  return 0x20;
    case I3D_PSMCT24:  return 0x18;
    case I3D_PSMCT16:
    case I3D_PSMCT16S: return 0x10;
    case I3D_PSMT8:
    case I3D_PSMT8H:   return 8;
    case I3D_PSMT4:
    case I3D_PSMT4HL:
    case I3D_PSMT4HH:  return 4;
    }
    return 0;
}

// ROR 0x00218db8
int i3dPsmBits(int psm)
{
    switch (psm)
    {
    case I3D_PSMCT32:
    case I3D_PSMCT24:
    case I3D_PSMT8H:
    case I3D_PSMT4HL:
    case I3D_PSMT4HH:  return 0x20;
    case I3D_PSMCT16:
    case I3D_PSMCT16S: return 0x10;
    case I3D_PSMT8:    return 8;
    case I3D_PSMT4:    return 4;
    }
    return 0;
}

// Within one page: the highest block the image touches + 1 (the page's
// block layout); larger: whole pages of 32 blocks.
// ROR 0x00218e10
u_int i3dPsmBlockEnd(u_int block, int w, int h, int psm)
{
    switch (psm)
    {
    case I3D_PSMCT32:
    case I3D_PSMCT24:
    case I3D_PSMT8H:
    case I3D_PSMT4HL:
    case I3D_PSMT4HH:
        if (w <= 0x40 && h <= 0x20)
            return block + s_blockCT32[((h - 1) >> 3) * 8 + ((w - 1) >> 3)] + 1;
        return block + (((w + 0x3f) >> 6) * ((h + 0x1f) >> 5) << 5);
    case I3D_PSMCT16:
        if (w <= 0x40 && h <= 0x40)
            return block + s_blockCT16[((h - 1) >> 3) * 4 + ((w - 1) >> 4)] + 1;
        return block + (((w + 0x3f) >> 6) * ((h + 0x3f) >> 6) << 5);
    case I3D_PSMCT16S:
        if (w <= 0x40 && h <= 0x40)
            return block + s_blockCT16S[((h - 1) >> 3) * 4 + ((w - 1) >> 4)] + 1;
        return block + (((w + 0x3f) >> 6) * ((h + 0x3f) >> 6) << 5);
    case I3D_PSMT8:
        if (w <= 0x80 && h <= 0x40)
            return block + s_blockT8[((h - 1) >> 4) * 8 + ((w - 1) >> 4)] + 1;
        return block + (((w + 0x7f) >> 7) * ((h + 0x3f) >> 6) << 5);
    case I3D_PSMT4:
        if (w <= 0x80 && h <= 0x80)
            return block + s_blockT4[((h - 1) >> 4) * 4 + ((w - 1) >> 5)] + 1;
        return block + (((w + 0x7f) >> 7) * ((h + 0x7f) >> 7) << 5);
    }
    return block;
}

/* ---- GS transfers ---- */

// Host -> VRAM: the transfer registers (TRXDIR 0), then the image in
// IMAGE GIF packets of at most 0x7fff qwords, each a REF of the data.
// ROR 0x00218fa8
u_long *i3dPktImageUpload(u_long *pkt, u_long bitbltbuf, u_long trxpos, u_long trxreg,
                          const u_char *data, int qwc)
{
    if (qwc != 0)
    {
        pkt[0] = I3D_DMA_CNT | 5;
        pkt[1] = (u_long)(I3D_VIF_DIRECT | 5) << 32;
        pkt[2] = I3D_GIF_AD4_EOP;
        pkt[3] = I3D_GS_A_D;
        pkt[4] = bitbltbuf;
        pkt[5] = I3D_GS_BITBLTBUF;
        pkt[7] = I3D_GS_TRXPOS;
        pkt[6] = trxpos;
        pkt[9] = I3D_GS_TRXREG;
        pkt[8] = trxreg;
        pkt[11] = I3D_GS_TRXDIR;
        pkt[10] = 0;
        pkt += 12;
        do
        {
            u_long n = qwc < 0x8000 ? (u_long)qwc : 0x7fff;
            pkt[0] = I3D_DMA_CNT | 1;
            pkt[1] = (u_long)(I3D_VIF_DIRECT | 1) << 32;
            pkt[3] = 0;
            pkt[2] = n | I3D_GIF_IMAGE_EOP;
            pkt[4] = n | ((u_long)i3dDmaAddr(data) << 32) | I3D_DMA_REF;
            pkt[5] = (n | I3D_VIF_DIRECT) << 32;
            data += n * 0x10;
            qwc -= (int)n;
            pkt += 6;
        } while (qwc > 0);
    }
    return pkt;
}

// A CLUT as 16-wide images of up to 256 colours (16 rows) each, one per
// chunk of VRAM.
// ROR 0x002190c8
u_long *i3dPktClutUpload(u_long *pkt, u_int dbp, int colors, int psm, const u_char *data)
{
    int bits = i3dPsmTexelBits(psm);
    int store = i3dPsmBits(psm);
    int left = (colors * bits + 0x7f) >> 7;
    int max = (bits << 8) >> 7;

    while (left > 0)
    {
        int rows = 0x10;
        int n = max;
        if (left < max)
        {
            rows = (left << 7) / (bits << 4);
            n = left;
        }
        const u_char *src = data;

        pkt[0] = I3D_DMA_CNT | 5;
        pkt[1] = (u_long)(I3D_VIF_DIRECT | 5) << 32;
        pkt[2] = I3D_GIF_AD4_EOP;
        pkt[3] = I3D_GS_A_D;
        data += n * 0x10;
        pkt[4] = ((u_long)dbp << 32) | ((u_long)psm << 56) | (1ull << 48);    /* DBW 1 */
        left -= n;
        pkt[5] = I3D_GS_BITBLTBUF;
        dbp += (store * 0x100 + 0x7ff) >> 11;
        pkt[6] = 0;
        pkt[7] = I3D_GS_TRXPOS;
        pkt[8] = ((u_long)rows << 32) | 0x10;
        pkt[9] = I3D_GS_TRXREG;
        pkt[10] = 0;
        pkt[11] = I3D_GS_TRXDIR;
        pkt[12] = I3D_DMA_CNT | 1;
        pkt[13] = (u_long)(I3D_VIF_DIRECT | 1) << 32;
        pkt[14] = (u_long)n | I3D_GIF_IMAGE_EOP;
        pkt[15] = 0;
        pkt[16] = (u_long)n | ((u_long)i3dDmaAddr(src) << 32) | I3D_DMA_REF;
        pkt[17] = ((u_long)n | I3D_VIF_DIRECT) << 32;
        pkt += 18;
    }
    return pkt;
}

// Copies pages inside VRAM (TRXDIR 2, each a 64x32 PSMCT32 rectangle),
// first to last when moving down, last to first when moving up, so that
// overlapping ranges survive.
// ROR 0x002192c8
u_long *i3dPktVramMove(u_long *pkt, u_int src, u_int dst, u_int pages)
{
    if (pages != 0 && src != dst)
    {
        u_int qwc = pages * 4 + 1;
        int off;
        int step;

        pkt[0] = qwc | I3D_DMA_CNT;
        pkt[1] = (u_long)(qwc | I3D_VIF_DIRECT) << 32;
        pkt[2] = (u_long)pages | 0x4000000000008000ull;    /* NREG 4, EOP */
        pkt[3] = 0xeeee;                                    /* A+D x 4 */
        pkt += 4;
        if (dst < src)
        {
            off = 0;
            step = 0x20;
        }
        else
        {
            step = -0x20;
            off = (int)pages * 0x20 - 0x20;
        }
        do
        {
            u_int s = src + off;
            int d = (int)dst + off;
            pkt[1] = I3D_GS_BITBLTBUF;
            off += step;
            pages--;
            pkt[0] = (u_long)s | 0x10000 | ((u_long)(u_int)d << 32) | (1ull << 48);  /* SBW / DBW 1 */
            pkt[2] = 0;
            pkt[3] = I3D_GS_TRXPOS;
            pkt[4] = 0x2000000040ull;                                           /* 64 x 32 */
            pkt[5] = I3D_GS_TRXREG;
            pkt[6] = 2;
            pkt[7] = I3D_GS_TRXDIR;
            pkt += 8;
        } while (pages != 0);
    }
    return pkt;
}

/* ---- objects ---- */

// MIPTBP1 / MIPTBP2 from three levels' block and width (inlined in the
// ROM's setters).
static u_long i3dTexMipTbp(const I3dTexLevel *lv)
{
    return ((u_long)lv[0].m_block & 0x3fff)
         | (((u_long)lv[2].m_bw & 0x3f) << 54)
         | (((u_long)lv[1].m_bw & 0x3f) << 34)
         | (((u_long)lv[1].m_block & 0x3fff) << 20)
         | (((u_long)lv[0].m_bw & 0x3f) << 14)
         | (((u_long)lv[2].m_block & 0x3fff) << 40);
}

// Zeroes the levels from first on (the setters' tail loop).
static void i3dTexClearLevels(I3dTexture *tex, u_int first)
{
    for (; first < I3D_TEX_LEVELS; first++)
    {
        I3dTexLevel *lv = &tex->m_level[first];
        lv->m_data = NULL;
        lv->m_qwc = 0;
        lv->m_block = 0;
        lv->m_bw = 0;
        lv->m_bitbltbuf = 0;
        lv->m_trxreg = 0;
    }
}

// Picture n of a TIM2 file: its levels laid out block after block from 0
// (the cache adds the VRAM base), each with its own upload. 16-bit images
// become PSMCT16S when ct16s is set.
// ROR 0x002193f0
int i3dTexSetTim2(I3dTexture *tex, void *tim2, u_int n, char ct16s)
{
    u_int end = 0;
    int width = 0;
    int height = 0;
    int psm;
    u_int i;

    i3dTexRelease(tex);
    I3dTim2Picture *pict = i3dTim2GetPicture(tim2, n);
    i3dTim2GetMipSize(pict, 0, &width, &height);
    tex->m_height = height;
    tex->m_width = width;
    tex->m_levels = pict->m_mipmap_textures;
    switch (pict->m_image_type)
    {
    case 1:  psm = ct16s ? I3D_PSMCT16S : I3D_PSMCT16; break;
    case 2:  psm = I3D_PSMCT24; break;
    case 3:  psm = I3D_PSMCT32; break;
    case 4:  psm = I3D_PSMT4; break;
    case 5:  psm = I3D_PSMT8; break;
    default: psm = I3D_PSMCT32; break;
    }

    for (i = 0; i < tex->m_levels; i++)
    {
        I3dTexLevel *lv = &tex->m_level[i];
        int w = 0;
        int h = 0;

        i3dTim2GetMipSize(pict, i, &w, &h);
        lv->m_data = i3dTim2GetImage(pict, (int)i);
        lv->m_block = end;
        lv->m_qwc = (u_int)(w * h * i3dPsmTexelBits(psm) + 0x7f) >> 7;
        end = i3dPsmBlockEnd(end, w, h, psm);
        if ((u_int)(psm - I3D_PSMT8) < 2)
            lv->m_bw = ((w + 0x7f) >> 7) << 1;
        else
            lv->m_bw = (w + 0x3f) >> 6;

        u_long blk = lv->m_block & 0x3fff;
        u_long bw = lv->m_bw & 0x3f;
        lv->m_bitbltbuf = blk | ((u_long)psm << 56) | (blk << 32) | ((u_long)psm << 24)
                        | (bw << 16) | (bw << 48);
        lv->m_trxreg = ((u_long)w & 0xfff) | (((u_long)h & 0xfff) << 32);
    }
    i3dTexClearLevels(tex, tex->m_levels);

    tex->m_handle.m_image = tex;
    tex->m_handle.m_size = (end + 0x1f) >> 5;
    tex->m_tex0 = ((u_long)tex->m_level[0].m_block & 0x3fff)
                | (pict->m_gs_tex0 & 0x1800000000ull)                      /* TFX */
                | (((u_long)i3dLog2(height) & 0xf) << 30)
                | (((u_long)i3dLog2(width) & 0xf) << 26)
                | (((u_long)tex->m_level[0].m_bw & 0x3f) << 14)
                | (pict->m_gs_tex0 & 0x400000000ull);                      /* TCC */
    tex->m_tex2 = (u_long)psm << 20;
    tex->m_tex1 = pict->m_gs_tex1;
    tex->m_handle.m_static = 0;
    tex->m_miptbp1 = i3dTexMipTbp(&tex->m_level[1]);
    tex->m_miptbp2 = i3dTexMipTbp(&tex->m_level[4]);
    tex->m_handle.m_kind = I3D_VRAM_KIND_TEX;
    return 0;
}

// A texture the caller placed in VRAM itself: levels at the description's
// blocks, no uploads, never cached (m_static).
// ROR 0x002197c8
int i3dTexSetDesc(I3dTexture *tex, const I3dTexDesc *desc)
{
    u_int i;

    i3dTexRelease(tex);
    tex->m_width = desc->m_width;
    tex->m_height = desc->m_height;
    tex->m_levels = desc->m_levels;
    for (i = 0; i < tex->m_levels; i++)
    {
        I3dTexLevel *lv = &tex->m_level[i];
        int w = (int)desc->m_width >> i;

        lv->m_block = desc->m_block[i];
        if ((u_int)(desc->m_psm - I3D_PSMT8) < 2)
            lv->m_bw = ((w + 0x7f) >> 7) << 1;
        else
            lv->m_bw = (w + 0x3f) >> 6;
        lv->m_trxreg = 0;
        lv->m_data = NULL;
        lv->m_qwc = 0;
        lv->m_bitbltbuf = 0;
    }
    i3dTexClearLevels(tex, tex->m_levels);

    tex->m_handle.m_size = 0;
    tex->m_handle.m_image = tex;
    tex->m_tex0 = ((u_long)tex->m_level[0].m_block & 0x3fff)
                | (((u_long)desc->m_tfx & 3) << 35)
                | (((u_long)i3dLog2(desc->m_height) & 0xf) << 30)
                | (((u_long)i3dLog2(desc->m_width) & 0xf) << 26)
                | (((u_long)tex->m_level[0].m_bw & 0x3f) << 14)
                | (((u_long)desc->m_tcc & 1) << 34);
    tex->m_tex1 = ((u_long)desc->m_lcm & 1)
                | (((u_long)desc->m_k & 0xfff) << 32)
                | (((u_long)desc->m_levels & 7) << 2)                     /* MXL: the level count */
                | (((u_long)desc->m_mtba & 1) << 9)
                | (((u_long)desc->m_mmin & 7) << 6)
                | (((u_long)desc->m_mmag & 1) << 5)
                | (((u_long)desc->m_l & 3) << 19);
    tex->m_miptbp2 = i3dTexMipTbp(&tex->m_level[4]);
    tex->m_handle.m_static = 1;
    tex->m_tex2 = ((u_long)desc->m_psm & 0x3f) << 20;
    tex->m_miptbp1 = i3dTexMipTbp(&tex->m_level[1]);
    tex->m_handle.m_kind = I3D_VRAM_KIND_TEX;
    return 0;
}

// ROR 0x00219a98
int i3dTexRelease(I3dTexture *tex)
{
    RosePan_I3dReleaseObject(tex);                 // PC: its GPU textures
    return i3dVramHandleRelease(&tex->m_handle);
}

// The CLUT of TIM2 picture n (CSM1). 24-bit colours are uploaded as
// PSMCT24 into a PSMCT32 CLUT.
// ROR 0x00219ab0
int i3dClutSetTim2(I3dClut *clut, void *tim2, u_int n, char ct16s)
{
    int psm;
    int cpsm;
    int upsm;

    i3dClutRelease(clut);
    I3dTim2Picture *pict = i3dTim2GetPicture(tim2, n);
    u_int type = pict->m_clut_type & 0x1f;
    u_char image = pict->m_image_type;
    if (type == 0)
        return 0xb;
    if (type == 1)
        psm = ct16s ? I3D_PSMCT16S : I3D_PSMCT16;
    else
        psm = type == 2 ? I3D_PSMCT24 : I3D_PSMCT32;
    upsm = I3D_PSMCT24;
    cpsm = I3D_PSMCT32;
    if (psm != I3D_PSMCT24)
    {
        upsm = psm;
        cpsm = psm;
    }
    clut->m_psm = upsm;

    if (image == 4)
        clut->m_palettes = (pict->m_clut_colors + 0xf) >> 4;
    else if (image == 5)
        clut->m_palettes = (pict->m_clut_colors + 0xff) >> 8;
    u_int colors = pict->m_clut_colors;
    clut->m_colors = colors;
    clut->m_data = i3dTim2GetClut(pict);

    int bits = i3dPsmBits(cpsm);
    clut->m_handle.m_static = 0;
    clut->m_handle.m_image = clut;
    clut->m_tex2 = (u_long)cpsm << 51;
    clut->m_tex0 = ((u_long)cpsm << 51) | 0x8000000001300000ull;  /* CLD 4, PSM bits 0x13 */
    clut->m_handle.m_kind = I3D_VRAM_KIND_CLUT;
    clut->m_handle.m_size = (colors * (u_int)bits + 0x1fff) >> 13;
    return 0;
}

// ROR 0x00219c70
int i3dClutRelease(I3dClut *clut)
{
    RosePan_I3dReleaseObject(clut);                // PC: its GPU textures
    return i3dVramHandleRelease(&clut->m_handle);
}

// Writes each level's upload at pkt, the levels' blocks offset by dst.
// ROR 0x00219c88
u_long *i3dTexMakeUpload(void *tex, u_long *pkt, u_int dst)
{
    I3dTexture *t = (I3dTexture *)tex;
    u_int i;

    for (i = 0; i < t->m_levels; i++)
    {
        I3dTexLevel *lv = &t->m_level[i];
        pkt = i3dPktImageUpload(pkt, ((u_long)dst << 32) + lv->m_bitbltbuf, 0,
                                lv->m_trxreg, lv->m_data, (int)lv->m_qwc);
    }
    return pkt;
}

// ROR 0x00219d10
u_long *i3dClutMakeUpload(void *clut, u_long *pkt, u_int dst)
{
    I3dClut *c = (I3dClut *)clut;
    return i3dPktClutUpload(pkt, dst, (int)c->m_colors, (int)c->m_psm, c->m_data);
}

// ROR 0x00219d40
u_long *i3dTexMakeMove(void *tex, u_long *pkt, u_int src, u_int dst)
{
    I3dTexture *t = (I3dTexture *)tex;
    return i3dPktVramMove(pkt, src, dst, t->m_handle.m_size);
}

// A moved CLUT is uploaded again rather than copied.
// ROR 0x00219d68
u_long *i3dClutMakeMove(void *clut, u_long *pkt, u_int src, u_int dst)
{
    (void)src;
    return i3dClutMakeUpload(clut, pkt, dst);
}

// ROR 0x00219d80
void i3dTexSetTcc(I3dTexture *tex, u_long tcc)
{
    tex->m_tex0 = (tex->m_tex0 & ~(1ull << 34)) | (tcc << 34);
}

// ROR 0x00219da8
void i3dTexSetTfx(I3dTexture *tex, u_long tfx)
{
    tex->m_tex0 = (tex->m_tex0 & ~(3ull << 35)) | (tfx << 35);
}

// ROR 0x00219dd0
void i3dTexSetMmag(I3dTexture *tex, u_long mmag)
{
    tex->m_tex1 = (tex->m_tex1 & ~0x20ull) | (mmag << 5);
}

// ROR 0x00219df0
void i3dTexSetMmin(I3dTexture *tex, u_long mmin)
{
    tex->m_tex1 = (tex->m_tex1 & ~0x1c0ull) | (mmin << 6);
}

// ROR 0x00219e10
void i3dTexSetL(I3dTexture *tex, u_long l)
{
    tex->m_tex1 = (tex->m_tex1 & 0xffffffffffe7ffffull) | (l << 19);
}

// ROR 0x00219e30
void i3dTexSetK(I3dTexture *tex, u_long k)
{
    tex->m_tex1 = (tex->m_tex1 & ~(0xfffull << 32)) | (k << 32);
}

// ROR 0x00219e58
int i3dTexInit(I3dTexture *tex)
{
    memset(tex, 0, sizeof(*tex));
    i3dVramHandleInit(&tex->m_handle);
    tex->m_handle.m_static = 0;
    tex->m_handle.m_kind = 0;
    return 0;
}

// ROR 0x00219e98
int i3dTexTerm(I3dTexture *tex)
{
    i3dTexRelease(tex);
    return 0;
}

// ROR 0x00219eb8
int i3dClutInit(I3dClut *clut)
{
    memset(clut, 0, sizeof(*clut));
    i3dVramHandleInit(&clut->m_handle);
    clut->m_handle.m_static = 0;
    clut->m_handle.m_kind = 0;
    return 0;
}

// ROR 0x00219ef8
int i3dClutTerm(I3dClut *clut)
{
    i3dClutRelease(clut);
    return 0;
}

// A 16x16 PSMT8 block (0x100 bytes) in PSMCT32 column order: each 64-byte
// column through its byte table.
// ROR 0x00219f18
void i3dSwizzleT8Block(const u_char *src, u_char *dst)
{
    u_int col;
    u_int i;

    for (col = 0; col < 4; col++)
    {
        const u_char *table = (col & 1) == 0 ? s_t8ColumnEven : s_t8ColumnOdd;
        u_int off = col * 0x40;
        for (i = 0; i < 0x40; i++)
            dst[off + i] = src[off + table[i]];
    }
}

// A 128x128 PSMT4 page (src, 0x80 nibbles a row) as a 64x32-word PSMCT32
// page (dst, 0x40 words a row): block by block.
// ROR 0x00219fb8
void i3dSwizzleT4Page(const u_char *src, u_int *dst)
{
    u_int buf[0x80];    /* the block, then its swizzled copy at +0x100 bytes */
    u_char *blk = (u_char *)buf;
    u_int b;
    u_int y;
    u_int x;

    for (b = 0; b < 0x20; b++)
    {
        u_int bx = s_t4BlockPos[b * 2];
        u_int by = s_t4BlockPos[b * 2 + 1];
        for (y = 0; y < 0x10; y++)
        {
            u_int idx = (by + y) * 0x80 + bx;
            for (x = 0; x < 0x20; x++)
                i3dNibbleSet(blk, y * 0x20 + x, i3dNibbleGet(src, idx++));
        }
        i3dSwizzleT4Block(blk, blk + 0x100);

        u_int *d = dst + s_ct32BlockPos[b * 2 + 1] * 0x40 + s_ct32BlockPos[b * 2];
        const u_int *w = buf + 0x40;
        for (y = 0; y < 8; y++)
            for (x = 0; x < 8; x++)
                d[y * 0x40 + x] = w[y * 8 + x];
    }
}
