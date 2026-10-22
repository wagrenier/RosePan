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

namespace
{
/* GS alpha: 0x80 is opaque. */
unsigned char GsAlpha(unsigned int a)
{
    return a >= 0x80 ? 0xff : (unsigned char)(a << 1);
}

/* One colour of `psm` (PSMCT32 / 24 / 16 / 16S) at `p`, as RGBA8. */
void ReadColour(const unsigned char *p, int psm, unsigned char *out)
{
    switch (psm)
    {
    case 0x00:
        out[0] = p[0];
        out[1] = p[1];
        out[2] = p[2];
        out[3] = GsAlpha(p[3]);
        break;
    case 0x01:
        /* TEXA's default TA0 (0x80) for every texel. */
        out[0] = p[0];
        out[1] = p[1];
        out[2] = p[2];
        out[3] = 0xff;
        break;
    default:
    {
        /* PSMCT16 / 16S: 5:5:5:1, the alpha bit picks TA1 (0x80) or TA0
         * (0) as RoR leaves TEXA. */
        const unsigned int v = (unsigned int)p[0] | ((unsigned int)p[1] << 8);
        out[0] = (unsigned char)((v & 0x1f) * 255 / 31);
        out[1] = (unsigned char)(((v >> 5) & 0x1f) * 255 / 31);
        out[2] = (unsigned char)(((v >> 10) & 0x1f) * 255 / 31);
        out[3] = (v & 0x8000) ? 0xff : 0;
        break;
    }
    }
}

int ColourBytes(int psm)
{
    return psm == 0x00 ? 4 : psm == 0x01 ? 3 : 2;
}
}

const void *RosePan_Draw2dMakeTexture(const RosePanImage2d *image)
{
    if (image == nullptr || image->pixels == nullptr || image->width <= 0 || image->height <= 0)
    {
        return nullptr;
    }

    const int psm = image->psm;
    const bool indexed = psm == 0x13 || psm == 0x14;
    if (indexed && image->clut == nullptr)
    {
        return nullptr;
    }
    if (!indexed && psm != 0x00 && psm != 0x01 && psm != 0x02 && psm != 0x0a)
    {
        return nullptr;
    }

    const size_t count = (size_t)image->width * (size_t)image->height;
    std::vector<unsigned char> rgba(count * 4u);
    for (size_t i = 0; i < count; i++)
    {
        unsigned char *out = &rgba[i * 4];
        if (indexed)
        {
            unsigned int index;
            if (psm == 0x13)
            {
                index = image->pixels[i];
            }
            else
            {
                /* The first texel in the low nibble. */
                const unsigned char byte = image->pixels[i >> 1];
                index = (i & 1) ? (byte >> 4) : (byte & 0x0fu);
            }
            index += (unsigned int)image->clut_offset;
            if (!image->clut_csm2)
            {
                index = (index & ~0x18u) | ((index & 0x08u) << 1) | ((index & 0x10u) >> 1);
            }
            if (index < (unsigned int)image->clut_colors)
            {
                ReadColour(image->clut + index * (unsigned int)ColourBytes(image->clut_psm),
                           image->clut_psm, out);
            }
            else
            {
                out[0] = out[1] = out[2] = out[3] = 0;
            }
        }
        else
        {
            ReadColour(image->pixels + i * (size_t)ColourBytes(psm), psm, out);
        }
        if (!image->tcc)
        {
            out[3] = 0xff;
        }
    }
    return RosePan_RendererCreateImageTexture(rgba.data(), image->width, image->height);
}

const void *RosePan_Draw2dMakeT8Texture(const unsigned char *pixels,
                                        const uint32_t *clut,
                                        int width, int height,
                                        int csm1, int opaque)
{
    if (clut == nullptr)
    {
        return nullptr;
    }

    RosePanImage2d image = {};
    image.pixels = pixels;
    image.psm = 0x13;
    image.width = width;
    image.height = height;
    image.clut = (const unsigned char *)clut;
    image.clut_colors = 256;
    image.clut_psm = 0x00;
    image.clut_csm2 = !csm1;
    image.tcc = !opaque;
    return RosePan_Draw2dMakeTexture(&image);
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
