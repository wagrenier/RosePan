/* ==========================================================================
 *  mag/magloader.h
 *
 *  The scene's two pack loaders (mag/magloader.cpp, 0x141e44..0x14313c):
 *  the character list over data/ch/CHARA.RPK (scene +0x215c) and the map
 *  units over data/bg/mapunit.bin (scene +0x224c). Each is a FileReader
 *  with its request (the file stays open after init), and a MagLoaderIndex:
 *  the names, sizes and file offsets of every entry, built at init from the
 *  packs' headers; load / loadByName read one entry into the game heap.
 *  CHARA.RPK is a pack of 20 packs (PC, BWN, ACTION, ITEM, FIX, ENEMY,
 *  DOOR, INST, CHAP01..CHAP12): init reads the first 0x2000 bytes of each
 *  and appends its entries, so a character's pack entry is its pack's
 *  start plus its number. Character ids are (id category << 10) + number,
 *  22 id categories mapped onto the 20 packs (getIndex / getId).
 *  Class names are descriptive (no vtable or symbol names them).
 * ======================================================================== */
#ifndef _MAG_MAGLOADER_H
#define _MAG_MAGLOADER_H

#include <stddef.h>

#include "game/gamfile.h"
#include "lib/unassigned.h"
#include "sdk/eetypes.h"

/* A pack's entry index: one game-heap block (m_data) holding the per-entry
 * sizes, file offsets, name pointers and name lengths, then the names.
 * EE 0x24. layout: host -- pointers into m_data. */
struct MagLoaderIndex
{
    /* Zeroes words 0..6 (m_name_size / m_name_buf stay). */
    void clear();                                                        // ROR 0x0014257c
    /* Frees m_data, then clear. */
    void release();                                                      // ROR 0x0014259c
    /* The size of the block for theNum entries and theNameSize bytes of
     * names. */
    int  calcSize(int theNum, int theNameSize) const;                    // ROR 0x00141e78
    /* Walks theNum NUL-separated names: each one's length and start
     * offset; returns the bytes they take. */
    int  scanNames(int *theLens, int *theOffsets, int theNum,
                   const char *theNameBuf) const;                        // ROR 0x00141e90
    /* A new index over a pack header's tables (theBase added to every
     * offset). Returns 1. */
    int  create(int theNum, const char *theNames, const int *theOffsets,
                const int *theSizes, int theBase);                       // ROR 0x00141f54
    /* Appends theNum entries (create when empty); theSort sorts the new
     * ones by name. */
    void add(int theNum, const char *theNames, const int *theOffsets,
             const int *theSizes, int theBase, int theSort);             // ROR 0x001420f8
    /* A debug listing of the entries, compiled out (an empty loop). */
    void dump() const;                                                   // ROR 0x00141e44
    /* The entry named theName (case-insensitive), -1 when none. */
    int  find(const char *theName) const;                                // ROR 0x00142500

    int          m_num;         // ROR 0x00: entries
    void        *m_data;        // ROR 0x04: the block (game heap)
    int          m_data_size;   // ROR 0x08
    int         *m_size;        // ROR 0x0c: per-entry sizes ("size > 0")
    int         *m_offset;      // ROR 0x10: per-entry file offsets ("offset > 0")
    const char **m_name;        // ROR 0x14: per-entry names, into m_name_buf
    u_char      *m_name_len;    // ROR 0x18: per-entry name lengths
    int          m_name_size;   // ROR 0x1c: bytes of names
    char        *m_name_buf;    // ROR 0x20: the names, NUL-separated
};
static_assert(offsetof(MagLoaderIndex, m_num) == 0, "m_num");

/* The character packs inside CHARA.RPK. */
enum { MAG_CHARA_PACK_MAX = 20 };

/* The character list. EE 0xf0. layout: host. */
struct MagCharaLoader
{
    MagCharaLoader();                                                    // ROR 0x00142e84
    /* PC: the ROM's MagScene destructor releases the index (0x137274) and
     * destroys the reader inline. */
    ~MagCharaLoader() { m_index.release(); }

    /* Reads CHARA.RPK's header, then each sub-pack's, into the index.
     * Returns 1. */
    int   init();                                                        // ROR 0x001425dc
    /* init over the 20 loose packs (data/ch/pc/PC.RPK ..); no callers. */
    int   initFromPacks();                                               // ROR 0x00142908
    /* Reads pack entry theIndex into a new game-heap buffer. */
    void *load(int theIndex);                                            // ROR 0x00142b18
    /* The pack (0..19) that holds pack entry theIndex. */
    int   getCategory(int theIndex) const;                               // ROR 0x00142c2c
    /* The pack entry of character id theID (asserts it exists). */
    int   getIndex(int theID) const;                                     // ROR 0x00142c88
    /* The character id of pack entry theIndex. */
    int   getId(int theIndex) const;                                     // ROR 0x00142dac

    FileReader     m_reader;                            // ROR 0x00
    FileReadReq    m_req;                               // ROR 0x08
    int            m_num_chara[MAG_CHARA_PACK_MAX];     // ROR 0x2c: entries per pack
    int            m_chara_start[MAG_CHARA_PACK_MAX];   // ROR 0x7c: each pack's first entry
    MagLoaderIndex m_index;                             // ROR 0xcc
};
static_assert(offsetof(MagCharaLoader, m_index) >= 0xcc, "m_index");

/* The map units (data/bg/mapunit.bin), by name. EE 0x50. layout: host. */
struct MagMapLoader
{
    /* PC: inline in the ROM's MagScene constructor (0x136134: the reader,
     * then the index clear) and destructor (0x13725c). */
    MagMapLoader() { m_index.clear(); }
    ~MagMapLoader() { m_index.release(); }

    /* Reads mapunit.bin's header into the index. Returns 1. */
    int   init();                                                        // ROR 0x00142ed8
    /* Reads the unit named theName into a new game-heap buffer. */
    void *loadByName(const char *theName);                               // ROR 0x00142fd0

    FileReader     m_reader;    // ROR 0x00
    FileReadReq    m_req;       // ROR 0x08
    MagLoaderIndex m_index;     // ROR 0x2c
};
static_assert(offsetof(MagMapLoader, m_index) >= 0x2c, "m_index");

#endif /* _MAG_MAGLOADER_H */
