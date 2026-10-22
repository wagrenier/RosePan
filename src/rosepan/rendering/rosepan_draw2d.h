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

/* Draws `sprite` from `texture` into the current host frame.  `frame_width`
 * x `frame_height` is the game's frame buffer (RoR's is 512x448), stretched
 * to the renderer's 640x448 frame the way the PS2 display stretches it.
 * `decal`: TEX0 TFX DECAL (the texture's colour as it is, the vertex colour
 * ignored); `linear`: TEX1 bilinear filtering. */
void RosePan_Draw2dSprite(const void *texture, const RosePanSprite2d *sprite,
                          int frame_width, int frame_height,
                          int decal, int linear);

#endif /* ROSEPAN_DRAW2D_H */
