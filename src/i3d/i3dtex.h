/* ==========================================================================
 *  i3d/i3dtex.h
 *
 *  The i3d library's texture and CLUT objects (i3d/i3dtex.cpp, ROR
 *  0x2184c0..0x21a120): what DogTexInfo keeps per texture, made from a TIM2
 *  picture (i3dTexSetTim2 / i3dClutSetTim2) or from a description of an
 *  image already placed in VRAM (i3dTexSetDesc). Each object starts with
 *  its VRAM handle (i3d/i3dvram.h); the context's VRAM cache reserves a
 *  block for it when a layer draws with it, and turns new / moved blocks
 *  into the GS transfers below (i3dCtxMakeVramTransfer). The layers read
 *  the GS registers the setters prepared (i3dTexRegsSet adds the VRAM
 *  addresses).
 *
 *  Also here: the GS pixel format tables (bits per texel, storage bits,
 *  blocks an image takes) and a PSMT8 / PSMT4 -> PSMCT32 swizzle that
 *  nothing in the ROM calls.
 *
 *  Names are inferred from the code (no symbols, no asserts).
 * ======================================================================== */
#ifndef _I3D_I3DTEX_H
#define _I3D_I3DTEX_H

#include <stddef.h>

#include "i3d/i3dvram.h"
#include "i3d/i3dtim2.h"

/* GS pixel formats the library handles. */
enum
{
    I3D_PSMCT32  = 0x00,
    I3D_PSMCT24  = 0x01,
    I3D_PSMCT16  = 0x02,
    I3D_PSMCT16S = 0x0a,
    I3D_PSMT8    = 0x13,
    I3D_PSMT4    = 0x14,
    I3D_PSMT8H   = 0x1b,
    I3D_PSMT4HL  = 0x24,
    I3D_PSMT4HH  = 0x2c
};

/* I3dVramHandle::m_kind. */
enum
{
    I3D_VRAM_KIND_TEX  = 1,
    I3D_VRAM_KIND_CLUT = 2
};

#define I3D_TEX_LEVELS 7

/* One mip level's upload. EE 0x20. layout: host -- a pointer. */
struct I3dTexLevel
{
    u_long         m_bitbltbuf; // ROR 0x00: BITBLTBUF with SBP = DBP = the block offset (the upload adds the VRAM block)
    u_long         m_trxreg;    // ROR 0x08: TRXREG (width, height)
    u_char        *m_data;      // ROR 0x10: the image (NULL for i3dTexSetDesc; i3dTexSwizzle rewrites it)
    u_int          m_qwc;       // ROR 0x14: image qwords (0 = no upload)
    u_int          m_block;     // ROR 0x18: GS block offset from the texture's base
    u_int          m_bw;        // ROR 0x1c: buffer width in 64-pixel units
};

/* A texture. EE 0x130 (i3dTexInit's memset). layout: host -- the handle
 * and the level pointers; EE offsets in the comments. */
struct I3dTexture
{
    I3dVramHandle  m_handle;    // ROR 0x00: m_image = this, m_size in pages
    u_long         m_tex0;      // ROR 0x18: TEX0 without TBP0's VRAM base (no PSM: TEX2 has it)
    u_long         m_tex1;      // ROR 0x20: TEX1
    u_long         m_tex2;      // ROR 0x28: TEX2 (PSM bits 20..25)
    u_long         m_miptbp1;   // ROR 0x30
    u_long         m_miptbp2;   // ROR 0x38
    I3dTexLevel    m_level[I3D_TEX_LEVELS]; // ROR 0x40
    u_int          m_levels;    // ROR 0x120
    u_int          m_width;     // ROR 0x124
    u_int          m_height;    // ROR 0x128
};
static_assert(offsetof(I3dTexture, m_handle) == 0, "the handle starts the texture");

/* A CLUT. EE 0x38 (i3dClutInit's memset). layout: host -- the handle and
 * the data pointer. */
struct I3dClut
{
    I3dVramHandle  m_handle;    // ROR 0x00: m_image = this, m_size in 4-block units
    u_long         m_tex0;      // ROR 0x18: or-ed into TEX0 (CPSM, CLD 4)
    u_long         m_tex2;      // ROR 0x20: or-ed into TEX2 (CPSM)
    u_int          m_palettes;  // ROR 0x28: 16- or 256-colour palettes (index images only)
    u_int          m_colors;    // ROR 0x2c
    u_int          m_psm;       // ROR 0x30: the upload's format (24-bit colours: CT24 into CT32)
    u_char        *m_data;      // ROR 0x34
};
static_assert(offsetof(I3dClut, m_handle) == 0, "the handle starts the CLUT");

/* A texture already in VRAM, as DogTexInfo::add passes it. EE 0x1e, plain
 * EE layout. */
struct I3dTexDesc
{
    u_short m_width;            // ROR 0x00
    u_short m_height;           // ROR 0x02
    u_short m_levels;           // ROR 0x04
    u_short m_block[I3D_TEX_LEVELS]; // ROR 0x06: each level's block
    u_char  m_psm;              // ROR 0x14
    u_char  m_tcc;              // ROR 0x15
    u_char  m_tfx;              // ROR 0x16
    u_char  m_lcm;              // ROR 0x17
    u_char  m_mmag;             // ROR 0x18
    u_char  m_mmin;             // ROR 0x19
    u_char  m_mtba;             // ROR 0x1a
    u_char  m_l;                // ROR 0x1b
    u_short m_k;                // ROR 0x1c
};
static_assert(offsetof(I3dTexDesc, m_block) == 0x06, "I3dTexDesc::m_block");
static_assert(offsetof(I3dTexDesc, m_psm) == 0x14, "I3dTexDesc::m_psm");
static_assert(offsetof(I3dTexDesc, m_tfx) == 0x16, "I3dTexDesc::m_tfx");
static_assert(offsetof(I3dTexDesc, m_mmag) == 0x18, "I3dTexDesc::m_mmag");
static_assert(offsetof(I3dTexDesc, m_k) == 0x1c, "I3dTexDesc::m_k");

/* ---- pixel formats ---- */

int     i3dLog2(int n);                                                  // ROR 0x00218d40
/* Bits per texel as data (CT24 24, T8H 8 ...); 0 for other formats. */
int     i3dPsmTexelBits(int psm);                                        // ROR 0x00218d58
/* Bits per pixel as stored in VRAM (CT24 / T8H / T4HL / T4HH 32). */
int     i3dPsmBits(int psm);                                             // ROR 0x00218db8
/* block + the GS blocks a w x h image of psm takes. */
u_int   i3dPsmBlockEnd(u_int block, int w, int h, int psm);              // ROR 0x00218e10

/* ---- GS transfers ---- */

u_long *i3dPktImageUpload(u_long *pkt, u_long bitbltbuf, u_long trxpos, u_long trxreg,
                          const u_char *data, int qwc);                  // ROR 0x00218fa8
u_long *i3dPktClutUpload(u_long *pkt, u_int dbp, int colors, int psm,
                         const u_char *data);                            // ROR 0x002190c8
u_long *i3dPktVramMove(u_long *pkt, u_int src, u_int dst, u_int pages);  // ROR 0x002192c8

/* The VRAM cache's transfers (i3dCtxMakeVramTransfer; block addresses,
 * tex / clut = the handle's m_image). */
u_long *i3dTexMakeUpload(void *tex, u_long *pkt, u_int dst);             // ROR 0x00219c88
u_long *i3dClutMakeUpload(void *clut, u_long *pkt, u_int dst);           // ROR 0x00219d10
u_long *i3dTexMakeMove(void *tex, u_long *pkt, u_int src, u_int dst);    // ROR 0x00219d40
u_long *i3dClutMakeMove(void *clut, u_long *pkt, u_int src, u_int dst);  // ROR 0x00219d68

/* ---- objects ---- */

int     i3dTexSetTim2(I3dTexture *tex, void *tim2, u_int n, char ct16s); // ROR 0x002193f0
int     i3dTexSetDesc(I3dTexture *tex, const I3dTexDesc *desc);          // ROR 0x002197c8
int     i3dTexRelease(I3dTexture *tex);                                  // ROR 0x00219a98
/* 0, or 0xb when the picture has no CLUT. */
int     i3dClutSetTim2(I3dClut *clut, void *tim2, u_int n, char ct16s);  // ROR 0x00219ab0
int     i3dClutRelease(I3dClut *clut);                                   // ROR 0x00219c70
void    i3dTexSetTcc(I3dTexture *tex, u_long tcc);                       // ROR 0x00219d80
void    i3dTexSetTfx(I3dTexture *tex, u_long tfx);                       // ROR 0x00219da8
void    i3dTexSetMmag(I3dTexture *tex, u_long mmag);                     // ROR 0x00219dd0
void    i3dTexSetMmin(I3dTexture *tex, u_long mmin);                     // ROR 0x00219df0
void    i3dTexSetL(I3dTexture *tex, u_long l);                           // ROR 0x00219e10
void    i3dTexSetK(I3dTexture *tex, u_long k);                           // ROR 0x00219e30
int     i3dTexInit(I3dTexture *tex);                                     // ROR 0x00219e58
int     i3dTexTerm(I3dTexture *tex);                                     // ROR 0x00219e98
int     i3dClutInit(I3dClut *clut);                                      // ROR 0x00219eb8
int     i3dClutTerm(I3dClut *clut);                                      // ROR 0x00219ef8

/* ---- PSMT8 / PSMT4 -> PSMCT32 (no caller in the ROM) ---- */

void    i3dNibbleSet(u_char *p, u_int i, u_char v);                      // ROR 0x002184c0
u_char  i3dNibbleGet(const u_char *p, u_int i);                          // ROR 0x00218500
void    i3dSwizzleT4Block(const u_char *src, u_char *dst);               // ROR 0x00218528
void    i3dSwizzleT8Page(const u_char *src, u_int *dst);                 // ROR 0x00218638
int     i3dTexSwizzle(I3dTexture *tex);                                  // ROR 0x00218778
void    i3dSwizzleT8Block(const u_char *src, u_char *dst);               // ROR 0x00219f18
void    i3dSwizzleT4Page(const u_char *src, u_int *dst);                 // ROR 0x00219fb8

#endif /* _I3D_I3DTEX_H */
