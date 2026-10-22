/* ==========================================================================
 *  i3d/i3dtim2.h
 *
 *  The i3d library's TIM2 helpers (i3d/i3dtim2.cpp, ROR 0x224de0..0x225018,
 *  not written yet): picture lookup and the per-level size / image / CLUT
 *  of a picture, read by the texture objects (i3d/i3dtex.cpp).
 *
 *  A TIM2 file: a 0x10-byte file header ("TIM2", version, alignment byte
 *  +5 -- non-zero puts the first picture at +0x80 --, picture count +6),
 *  then the pictures, each a header (below) followed by its image data
 *  (the mip levels) and its CLUT.
 * ======================================================================== */
#ifndef _I3D_I3DTIM2_H
#define _I3D_I3DTIM2_H

#include <stddef.h>

/* A TIM2 picture header (Sony's TIM2 format, sceTim2PictureHeader). EE
 * 0x30, read from the file: plain EE layout. When MipMapTextures > 1 an
 * extension follows at +0x30 (MIPTBP1 / MIPTBP2) and the level sizes at
 * +0x40. */
struct I3dTim2Picture
{
    u_int  m_total_size;        // ROR 0x00: header + image + CLUT bytes (the next picture)
    u_int  m_clut_size;         // ROR 0x04
    u_int  m_image_size;        // ROR 0x08
    u_short m_header_size;      // ROR 0x0c
    u_short m_clut_colors;      // ROR 0x0e
    u_char m_pict_format;       // ROR 0x10
    u_char m_mipmap_textures;   // ROR 0x11: levels
    u_char m_clut_type;         // ROR 0x12: low 5 bits 0 none, 1 16-bit, 2 24-bit, 3 32-bit
    u_char m_image_type;        // ROR 0x13: 1 16-bit, 2 24-bit, 3 32-bit, 4 4-bit index, 5 8-bit index
    u_short m_width;            // ROR 0x14
    u_short m_height;           // ROR 0x16
    u_long m_gs_tex0;           // ROR 0x18
    u_long m_gs_tex1;           // ROR 0x20
    u_int  m_gs_regs;           // ROR 0x28
    u_int  m_gs_tex_clut;       // ROR 0x2c
};
static_assert(offsetof(I3dTim2Picture, m_header_size) == 0x0c, "I3dTim2Picture::m_header_size");
static_assert(offsetof(I3dTim2Picture, m_clut_colors) == 0x0e, "I3dTim2Picture::m_clut_colors");
static_assert(offsetof(I3dTim2Picture, m_mipmap_textures) == 0x11, "I3dTim2Picture::m_mipmap_textures");
static_assert(offsetof(I3dTim2Picture, m_clut_type) == 0x12, "I3dTim2Picture::m_clut_type");
static_assert(offsetof(I3dTim2Picture, m_image_type) == 0x13, "I3dTim2Picture::m_image_type");
static_assert(offsetof(I3dTim2Picture, m_width) == 0x14, "I3dTim2Picture::m_width");
static_assert(offsetof(I3dTim2Picture, m_height) == 0x16, "I3dTim2Picture::m_height");
static_assert(offsetof(I3dTim2Picture, m_gs_tex0) == 0x18, "I3dTim2Picture::m_gs_tex0");
static_assert(offsetof(I3dTim2Picture, m_gs_tex1) == 0x20, "I3dTim2Picture::m_gs_tex1");
static_assert(sizeof(I3dTim2Picture) == 0x30, "I3dTim2Picture is 0x30 bytes on the EE");

/* ROR 0x00224e70: picture n of a TIM2 file, NULL past the count. */
I3dTim2Picture *i3dTim2GetPicture(void *tim2, u_int n);                  // ROR 0x00224e70
/* ROR 0x00224ed0: level's width / height (either pointer may be NULL);
 * returns its image bytes rounded up to 16. */
u_int  i3dTim2GetMipSize(I3dTim2Picture *pict, u_int level, int *w, int *h); // ROR 0x00224ed0
/* ROR 0x00224f58: the level's image data, NULL past the levels. */
u_char *i3dTim2GetImage(I3dTim2Picture *pict, int level);                // ROR 0x00224f58
/* ROR 0x00224ff0: the CLUT data, NULL when the picture has no colours. */
u_char *i3dTim2GetClut(I3dTim2Picture *pict);                            // ROR 0x00224ff0

#endif /* _I3D_I3DTIM2_H */
