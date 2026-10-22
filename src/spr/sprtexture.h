/* ==========================================================================
 *  spr/sprtexture.h
 *
 *  The sprite texture loader (spr/sprtexture.cpp, ..0x190a78): takes a list
 *  of texture files (or one .rpk pack), reads them through the rx file
 *  layer and adds them to a DogTexInfo; SprTexture::update steps it and
 *  turns non-zero when done. Each SprMan owns one at +8. Minimal: what
 *  spr/s2d.cpp touches.
 * ======================================================================== */
#ifndef _SPR_SPRTEXTURE_H
#define _SPR_SPRTEXTURE_H

#include <stddef.h>

#include "sdk/eetypes.h"

struct DogTexInfo;

/* An entry of a texture file list, EE 0x18; m_id < 0 ends the list.
 * layout: host -- pointers. */
struct SprTexFile
{
    int         m_id;           // ROR 0x00
    const char *m_name;         // ROR 0x04: the file (in the pack)
    const char *m_tex_name;     // ROR 0x08: the texture's name in the DogTexInfo
    int         m_size;         // ROR 0x0c: set by the loader
    u_int       m_flags;        // ROR 0x10: 1 = load; the loader clears bit 2
    void       *m_buf;          // ROR 0x14: the loaded file, game heap
};

/* EE 0x438. layout: host -- pointers. */
struct SprTexture
{
    /* Takes the list and the pack (NULL = loose files) and picks the
     * first load step. */
    void setList(const SprTexFile *list, const char *rpk);              // ROR 0x0018f744
    void setListMode(const SprTexFile *list, const char *rpk, int mode); // ROR 0x0018f808
    /* Steps the load; non-zero when it is done. */
    int  update();                                                       // ROR 0x0018febc

    const SprTexFile *m_list;   // ROR 0x000
    const char  *m_rpk;         // ROR 0x004
    u_char       _008[0x414 - 0x008];
    DogTexInfo  *m_tex_info;    // ROR 0x414: where the textures go
    u_char       _418[0x434 - 0x418];
    int          m_mode;        // ROR 0x434: setListMode
};

#endif /* _SPR_SPRTEXTURE_H */
