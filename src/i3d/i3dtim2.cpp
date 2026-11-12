/* ==========================================================================
 *  i3d/i3dtim2.cpp
 *
 *  The i3d library's TIM2 helpers (ROR 0x224de0..0x225018): the file
 *  header check, picture lookup, and a picture's level sizes, image data
 *  and CLUT. See i3dtim2.h. The texture objects (i3dtex.cpp) call the
 *  lookups; the check, i3dTim2IsClutOnly and i3dTim2GetClutCount have no
 *  callers in the ROM.
 * ======================================================================== */
#include "i3d/i3dtim2.h"

/* ROR 0x00224de0 */
int i3dTim2CheckHeader(void *tim2)
{
    I3dTim2File *file = (I3dTim2File *)tim2;
    const char *id = file->m_id;
    int kind;

    /* ROM: the TIM2 test is or-ed, so any one matching byte passes. */
    if (id[0] == 'T' || id[1] == 'I' || id[2] == 'M' || id[3] == '2')
        kind = 1;
    else if (id[0] == 'C' || id[1] == 'L' || id[2] == 'T')
        kind = 2;
    else
        return 0;

    switch (file->m_version)
    {
    case 3:
    case 0x82:
        break;
    case 4:
        if (file->m_format >= 2)
            kind = 0;
        break;
    default:
        kind = 0;
        break;
    }
    return kind;
}

/* ROR 0x00224e70 */
I3dTim2Picture *i3dTim2GetPicture(void *tim2, u_int n)
{
    I3dTim2File *file = (I3dTim2File *)tim2;
    u_char *pict;
    u_int i;

    if (n >= file->m_pictures)
        return NULL;

    pict = (u_char *)file + (file->m_format ? 0x80 : 0x10);
    for (i = 0; i < n; i++)
        pict += ((I3dTim2Picture *)pict)->m_total_size;
    return (I3dTim2Picture *)pict;
}

/* ROR 0x00224ec0 */
int i3dTim2IsClutOnly(I3dTim2Picture *pict)
{
    return pict->m_mipmap_textures == 0;
}

/* ROR 0x00224ed0 */
u_int i3dTim2GetMipSize(I3dTim2Picture *pict, u_int level, int *w, int *h)
{
    int width = (int)pict->m_width >> level;
    int height = (int)pict->m_height >> level;
    int size;

    if (w != NULL)
        *w = width;
    if (h != NULL)
        *h = height;

    size = width * height;
    switch (pict->m_image_type)
    {
    case 1: size *= 2; break;   /* 16-bit */
    case 2: size *= 3; break;   /* 24-bit */
    case 3: size *= 4; break;   /* 32-bit */
    case 4: size /= 2; break;   /* 4-bit index */
    }                           /* 5, 8-bit index: one byte a texel */
    return (u_int)(size + 0xf) & ~0xfu;
}

/* ROR 0x00224f58 */
u_char *i3dTim2GetImage(I3dTim2Picture *pict, int level)
{
    u_char *image;
    int i;

    if (level >= pict->m_mipmap_textures)
        return NULL;

    image = (u_char *)pict + pict->m_header_size;
    if (pict->m_mipmap_textures != 1)
    {
        I3dTim2MipMap *mip = (I3dTim2MipMap *)(pict + 1);

        for (i = 0; i < level; i++)
            image += mip->m_mm_image_size[i];
    }
    return image;
}

/* ROR 0x00224fb0 */
int i3dTim2GetClutCount(I3dTim2Picture *pict)
{
    if (pict->m_image_type == 4)
        return (pict->m_clut_colors + 0xf) >> 4;
    if (pict->m_image_type == 5)
        return (pict->m_clut_colors + 0xff) >> 8;
    return 0;
}

/* ROR 0x00224ff0 */
u_char *i3dTim2GetClut(I3dTim2Picture *pict)
{
    if (pict->m_clut_colors == 0)
        return NULL;
    return (u_char *)pict + pict->m_header_size + pict->m_image_size;
}
