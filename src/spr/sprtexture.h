/* ==========================================================================
 *  spr/sprtexture.h
 *
 *  SprTexture (spr/sprtexture.cpp, 0x18f5b0..0x190a78): the sprite texture
 *  loader. setList takes a list of texture files and a pack (.rpk, or NULL
 *  for loose files); update then steps through the list, one state per
 *  call, reading each file -- from the whole pack in memory (mode 0), from
 *  the pack on disc one file at a time after its first 4 KB (mode 1), or as
 *  a loose file -- decoding RTCM run-length data, and adding the entries
 *  flagged 1 to a DogTexInfo under their texture names. update turns 1
 *  when the list is done, -1 after an error. Each SprMan owns one at +8.
 * ======================================================================== */
#ifndef _SPR_SPRTEXTURE_H
#define _SPR_SPRTEXTURE_H

#include <stddef.h>

#include "bed/bedpfh.h"
#include "sdk/eetypes.h"

struct DogTexInfo;
struct FileReader;
struct i3dContext;

/* The most files a list may hold (the id and slot tables' size). */
#define SPR_TEX_FILE_MAX 60

/* SprTexFile::m_flags */
#define SPR_TEX_LOAD     0x1    /* add it to the DogTexInfo while the list loads */
#define SPR_TEX_IN_PACK  0x4    /* m_buf points into the pack: not freed */
#define SPR_TEX_DECODED  0x8    /* m_buf is loadTex's RTCM decode: freed */

/* An entry of a texture file list, EE 0x18; m_id < 0 ends the list. The
 * loader writes m_size, m_flags and m_buf back. layout: host -- pointers. */
struct SprTexFile
{
    int         m_id;           // ROR 0x00: 0..59, the index the getters take
    const char *m_name;         // ROR 0x04: the file (in the pack); "" ends the list too
    const char *m_tex_name;     // ROR 0x08: the texture's name in the DogTexInfo
    int         m_size;         // ROR 0x0c: set by the loader
    u_int       m_flags;        // ROR 0x10: SPR_TEX_*
    void       *m_buf;          // ROR 0x14: the loaded file
};
static_assert(offsetof(SprTexFile, m_id) == 0x00, "m_id");

/* The header of an RTCM run-length file (file data). EE 0x20; the records
 * follow: a kind byte, a 24-bit big-endian count, then one unit (kind 0,
 * repeated count times) or count units (kind 1). */
struct SprRtcmHead
{
    char  m_id[4];              // ROR 0x00: "RTCM"
    int   _04;
    int   m_size;               // ROR 0x08: the decoded size
    int   m_total;              // ROR 0x0c: the file size, header included
    short m_unit;               // ROR 0x10: the bytes of one unit
    u_char _12[0x20 - 0x12];
};
static_assert(offsetof(SprRtcmHead, m_size) == 0x08, "m_size");
static_assert(offsetof(SprRtcmHead, m_total) == 0x0c, "m_total");
static_assert(offsetof(SprRtcmHead, m_unit) == 0x10, "m_unit");
static_assert(sizeof(SprRtcmHead) == 0x20, "SprRtcmHead is 0x20 bytes on the EE");

/* EE 0x438. layout: host -- pointers. */
struct SprTexture
{
    SprTexture();                                                        // ROR 0x0018f5b0
    ~SprTexture();                                                       // ROR 0x0018f5e4

    void clear();                                                        // ROR 0x0018f60c
    /* The load order index of file id, -1 when it is not loaded. */
    int  getSlot(int id) const;                                          // ROR 0x0018f6dc
    /* The DogTexInfo texture of file id, -1 when none. */
    int  getTexNo(int id) const;                                         // ROR 0x0018f6f8
    /* The DogTexInfo record of file id (loadTex), NULL when none. */
    void *getTexRecord(int id) const;                                    // ROR 0x0018f734
    /* Takes the list and the pack (NULL = loose files) and picks the
     * first load step by m_mode. */
    void setList(SprTexFile *list, const char *rpk);                     // ROR 0x0018f744
    void setListMode(SprTexFile *list, const char *rpk, int mode);      // ROR 0x0018f808
    /* Releases the textures and frees the files and the pack. */
    void release();                                                      // ROR 0x0018f824
    /* Frees file id's buffer; 1 when id is in the list. */
    int  freeFile(int id);                                               // ROR 0x0018f9cc
    /* Releases file id's texture; 1 when it had one. */
    int  releaseTex(int id);                                             // ROR 0x0018fab4
    /* Adds file id's picture to the DogTexInfo; 1 when added. */
    int  loadTex(int id);                                                // ROR 0x0018fbb0
    /* The index of a file in the pack, -1 without a pack. */
    int  getPackIndex(const char *theName) const;                        // ROR 0x0018fe8c
    /* Steps the load: 0 busy, 1 done, -1 after an error. */
    int  update();                                                       // ROR 0x0018febc
    void setStep(int step);                                              // ROR 0x001905a0
    void setReadStep(int step);                                          // ROR 0x001905a8
    /* Starts reading size bytes of theName at offset into buf. */
    int  startRead(const char *theName, void *buf, int size, int offset); // ROR 0x001905b0
    /* Steps the read: 0 busy, 1 done, < 0 error. */
    int  updateRead();                                                   // ROR 0x00190618
    int  compareId(const char *id, const char *p);                      // ROR 0x00190884
    int  isRtcm(const void *p);                                          // ROR 0x001908d0
    /* Decodes an RTCM file into a new buffer (game heap); NULL on a bad
     * record. */
    void *decodeRtcm(const void *p);                                     // ROR 0x00190904

    SprTexFile  *m_list;        // ROR 0x000
    const char  *m_rpk;         // ROR 0x004: the pack, NULL = loose files
    void        *m_buf;         // ROR 0x008: the pack (mode 0) or its first 4 KB (mode 1)
    int          m_buf_size;    // ROR 0x00c
    char         m_read_name[0x100]; // ROR 0x010: startRead's file
    void        *m_read_buf;    // ROR 0x110
    int          m_read_size;   // ROR 0x114: < 0 fails the read
    int          m_read_offset; // ROR 0x118: 0 = no seek
    int          m_step;        // ROR 0x11c: update's state
    int          m_read_step;   // ROR 0x120: updateRead's state
    int          m_cur;         // ROR 0x124: the list entry being loaded
    int          m_count;       // ROR 0x128: entries done (the next slot)
    int          m_tex_no[SPR_TEX_FILE_MAX];    // ROR 0x12c: by slot, 0 = none
    int          m_slot[SPR_TEX_FILE_MAX];      // ROR 0x21c: by id, -1 = none
    void        *m_tex_rec[SPR_TEX_FILE_MAX];   // ROR 0x30c: by id, DogTexInfo::getTexInfo
    FileReader  *m_reader;      // ROR 0x3fc: updateRead's, new / delete
    int          m_400;         // ROR 0x400: clear sets 0x40; unverified, nothing here reads it
    int          m_ready_404;   // ROR 0x404: FileReader::isReady's unused argument
    int          m_ready_fd;    // ROR 0x408: isReady's fd after a seek / read / close
    int          m_fd;          // ROR 0x40c: the open file
    int          _410;
    DogTexInfo  *m_tex_info;    // ROR 0x414: where the textures go (the owner sets it)
    i3dContext  *m_ctx;         // ROR 0x418: setTexTable's context when the list is done, NULL = none
    BedPfh       m_pfh_body;    // ROR 0x41c: the view on m_buf
    BedPfh      *m_pfh;         // ROR 0x430: &m_pfh_body once the pack is read, NULL = loose files
    int          m_mode;        // ROR 0x434: setListMode; 0 = whole pack, 1 = file by file
};
static_assert(offsetof(SprTexture, m_list) == 0x000, "m_list");

#endif /* _SPR_SPRTEXTURE_H */
