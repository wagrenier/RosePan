/* ==========================================================================
 *  rosepan_draw2d.cpp -- the native 2D draw module (see rosepan_draw2d.h).
 * ======================================================================== */
#include "rosepan/rendering/rosepan_draw2d.h"

#include <vector>

#include "rosepan/rendering/rosepan_renderer.h"

namespace
{
constexpr float kRendererFrameWidth = 640.0f;
constexpr float kRendererFrameHeight = 448.0f;
}

const void *RosePan_Draw2dMakeT8Texture(const unsigned char *pixels,
                                        const uint32_t *clut,
                                        int width, int height,
                                        int csm1, int opaque)
{
    if (pixels == nullptr || clut == nullptr || width <= 0 || height <= 0)
    {
        return nullptr;
    }

    std::vector<unsigned char> rgba((size_t)width * (size_t)height * 4u);
    for (size_t i = 0; i < (size_t)width * (size_t)height; i++)
    {
        unsigned int index = pixels[i];
        if (csm1)
        {
            index = (index & 0xe7u) | ((index & 0x08u) << 1) | ((index & 0x10u) >> 1);
        }
        const uint32_t c = clut[index];
        const unsigned int a = c >> 24;
        rgba[i * 4 + 0] = (unsigned char)(c & 0xff);
        rgba[i * 4 + 1] = (unsigned char)((c >> 8) & 0xff);
        rgba[i * 4 + 2] = (unsigned char)((c >> 16) & 0xff);
        /* GS alpha: 0x80 is opaque. */
        rgba[i * 4 + 3] = opaque || a >= 0x80 ? 0xff : (unsigned char)(a << 1);
    }
    return RosePan_RendererCreateImageTexture(rgba.data(), width, height);
}

void RosePan_Draw2dSprite(const void *texture, const RosePanSprite2d *sprite,
                          int frame_width, int frame_height,
                          int decal, int linear)
{
    if (texture == nullptr || sprite == nullptr || frame_width <= 0 || frame_height <= 0)
    {
        return;
    }

    const float sx = kRendererFrameWidth / (float)frame_width;
    const float sy = kRendererFrameHeight / (float)frame_height;
    const float x0 = sprite->x0 * sx, x1 = sprite->x1 * sx;
    const float y0 = sprite->y0 * sy, y1 = sprite->y1 * sy;
    const float xy[8] = {x0, y0, x1, y0, x0, y1, x1, y1};
    const float uv[8] = {sprite->s0, sprite->t0, sprite->s1, sprite->t0,
                         sprite->s0, sprite->t1, sprite->s1, sprite->t1};

    if (decal)
    {
        /* DECAL: the texel as it is; 0x80 is the renderer's neutral colour. */
        RosePan_RendererDrawImageQuad(texture, xy, uv, 0x80, 0x80, 0x80, 0x80, linear);
    }
    else
    {
        RosePan_RendererDrawImageQuad(texture, xy, uv, sprite->rgba[0], sprite->rgba[1],
                                      sprite->rgba[2], sprite->rgba[3], linear);
    }
}
