/* ==========================================================================
 *  mag/magloader.h
 *
 *  The scene's two pack loaders (mag/magloader.cpp, 0x1420f8..0x143130):
 *  the character list over data/ch/CHARA.RPK (scene +0x215c) and the map
 *  units over data/bg/mapunit.bin (scene +0x224c). Each is a FileReader
 *  with its request, then the pack's index read from the first 2048 bytes
 *  at init; load / loadByName read one entry into the game heap. Class
 *  names are descriptive (no vtable or symbol names them). Minimal: what
 *  mag/magscene2.cpp calls.
 * ======================================================================== */
#ifndef _MAG_MAGLOADER_H
#define _MAG_MAGLOADER_H

#include <stddef.h>

#include "lib/unassigned.h"

/* A pack's name index (cleared by 0x14257c). EE 0x1c. layout: host --
 * pointers into the loaded header. */
struct MagLoaderIndex
{
    /* The entry named theName (case-insensitive), -1 when none. */
    int find(const char *theName) const;                                 // ROR 0x00142500

    int          m_num;         // ROR 0x00
    int          _04;
    int          _08;
    int         *m_size;        // ROR 0x0c: per-entry sizes ("size > 0")
    int         *m_offset;      // ROR 0x10: per-entry offsets ("offset > 0")
    const char **m_name;        // ROR 0x14: per-entry names
    int          _18;
};
static_assert(offsetof(MagLoaderIndex, m_num) == 0, "m_num");

/* The character list: ids are (category << 10) + n, 0x5800 of them in 22
 * categories. EE 0xf0. layout: host. */
struct MagCharaLoader
{
    void  init();                                                        // ROR 0x001425dc
    /* Reads pack entry theIndex into a new game-heap buffer. */
    void *load(int theIndex);                                            // ROR 0x00142b18
    /* The pack entry of character id theID (asserts it exists). */
    int   getIndex(int theID) const;                                     // ROR 0x00142c88
    /* The character id of pack entry theIndex. */
    int   getId(int theIndex) const;                                     // ROR 0x00142dac

    FileReader     m_reader;    // ROR 0x00
    FileReadReq    m_req;       // ROR 0x08
    u_char         _2c[0xcc - 0x2c];
    MagLoaderIndex m_index;     // ROR 0xcc
    u_char         _e8[0xf0 - 0xe8];
};
static_assert(offsetof(MagCharaLoader, m_index) >= 0xcc, "m_index");

/* The map units (data/bg/mapunit.bin), by name. EE 0x50. layout: host. */
struct MagMapLoader
{
    void  init();                                                        // ROR 0x00142ed8
    /* Reads the unit named theName into a new game-heap buffer. */
    void *loadByName(const char *theName);                               // ROR 0x00142fd0

    FileReader     m_reader;    // ROR 0x00
    FileReadReq    m_req;       // ROR 0x08
    MagLoaderIndex m_index;     // ROR 0x2c
    u_char         _48[0x50 - 0x48];
};
static_assert(offsetof(MagMapLoader, m_index) >= 0x2c, "m_index");

#endif /* _MAG_MAGLOADER_H */
