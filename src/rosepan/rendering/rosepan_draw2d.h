/* ==========================================================================
 *  rosepan_draw2d.h
 *
 *  The native 2D draw module (architecture.md "PC platform layer"): RoR's 2D
 *  code keeps its decisions -- which picture, which part of it, where -- and
 *  hands them here instead of writing GIF packets.  Pictures become GPU
 *  textures when RoR loads them, decoded on the host, never through GS
 *  memory.  First user: the now-loading sprite (system/sysnowloading.cpp).
 * ======================================================================== */
#ifndef ROSEPAN_DRAW2D_H
#define ROSEPAN_DRAW2D_H

#include <stdint.h>

/* One GS SPRITE as RoR decided it: corners in pixels of the game's frame
 * buffer (GS coordinates minus the draw offset), texture coordinates
 * normalised 0..1 (the ST registers), colour on the PS2 scale. */
struct RosePanSprite2d
{
    float         x0, y0, x1, y1;
    float         s0, t0, s1, t1;
    unsigned char rgba[4];
};

/* An 8-bit indexed picture (PSMT8 pixels, row-major, `width` x `height`)
 * with its 256-entry CT32 CLUT as RoR loads it into GS memory, as a GPU
 * texture.  `csm1`: the CLUT is a 16x16 CT32 image read in CSM1 order
 * (index bits 3 and 4 swapped).  `opaque`: alpha forced to full, for a
 * sprite drawn with alpha blending off (ABE 0).  NULL on failure. */
const void *RosePan_Draw2dMakeT8Texture(const unsigned char *pixels,
                                        const uint32_t *clut,
                                        int width, int height,
                                        int csm1, int opaque);

/* A picture as RoR keeps it in main memory (the i3d texture and CLUT
 * objects, i3d/i3dtex.h), decoded the way the GS would read it after the
 * upload: `psm` and `clut_psm` are GS pixel formats (PSMCT32 0, PSMCT24 1,
 * PSMCT16 2, PSMCT16S 0xa, PSMT8 0x13, PSMT4 0x14); PSMT8 / PSMT4 pixels
 * index `clut` (`clut_colors` colours), from entry `clut_offset` (a 4-bit
 * image's CSA * 16).  The CLUT is the uploaded data, 16 colours a row: 4
 * bytes per colour for PSMCT32, 3 for PSMCT24 (uploaded into a PSMCT32
 * CLUT), 2 for PSMCT16 / 16S.  `clut_csm2`: TEX0 CSM2, the CLUT read in
 * order; otherwise CSM1, where the GS reads the 16-wide CLUT image in 8x2
 * tiles -- entry n (offset included) is at index bits 3 and 4 swapped, so
 * a 4-bit palette's colours 8..15 are the second row's 0..7.  Entries past
 * the data read as transparent black.  `tcc`: TEX0 TCC; 0 takes
 * the texture as RGB only (alpha full, the vertex alpha decides). */
struct RosePanImage2d
{
    const unsigned char *pixels;
    int                  psm;
    int                  width, height;
    const unsigned char *clut;
    int                  clut_colors;
    int                  clut_offset;
    int                  clut_psm;
    int                  clut_csm2;
    int                  tcc;
};

/* `image` as a GPU texture (RGBA8), NULL for a format it cannot read. */
const void *RosePan_Draw2dMakeTexture(const RosePanImage2d *image);

/* Draws `sprite` from `texture` into the current host frame.  `frame_width`
 * x `frame_height` is the game's frame buffer (RoR's is 512x448), stretched
 * to the renderer's frame (RosePan_RendererGetFrameSize) the way the PS2
 * display stretches it; the renderer keeps that frame's proportions at any
 * window aspect.
 * `decal`: TEX0 TFX DECAL (the texture's colour as it is, the vertex colour
 * ignored); `linear`: TEX1 bilinear filtering. */
void RosePan_Draw2dSprite(const void *texture, const RosePanSprite2d *sprite,
                          int frame_width, int frame_height,
                          int decal, int linear);

#endif /* ROSEPAN_DRAW2D_H */
