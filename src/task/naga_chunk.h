/* ==========================================================================
 *  task/naga_chunk.h
 *
 *  task/naga_chunk.cpp (pinned 0x1fc47c..0x1fce0c; RoR's source string
 *  anchors 0x1fc4f0 / 0x1fc89c / 0x1fcb14): the chunk tables event packs are
 *  made of. A table is 16-byte entries {name[8], offset, size} ended by a
 *  zero entry; each offset counts from the table's start, and a chunk's data
 *  may itself be a table (nested chunks: "CHARA" -> "THREAD" -> "name").
 *  NagaChunk reads one (scenePlayStart, DisplayMessage); NagaChunkWriter
 *  builds one and saves it to the host (a debug tool: nothing in the retail
 *  game calls the writer).
 * ======================================================================== */
#ifndef _TASK_NAGA_CHUNK_H
#define _TASK_NAGA_CHUNK_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "utl/utlmisc.h"

/* An entry's name: 8 bytes, zero-padded, compared as two words. */
struct NagaChunkTag
{
    char m_name[8];             // ROR 0x00
};
static_assert(sizeof(NagaChunkTag) == 0x08, "NagaChunkTag is 8 bytes");

/* One 16-byte table entry; a zero first word ends the table. EE layout
 * (the table comes from the event pack). */
struct NagaChunkEntry
{
    char m_name[8];             // ROR 0x00: zero-padded; compared as two words
    int  m_offset;              // ROR 0x08: the data, from the table's start
    int  m_size;                // ROR 0x0c: the data's size in bytes
};
static_assert(sizeof(NagaChunkEntry) == 0x10, "NagaChunkEntry is 16 bytes");
static_assert(offsetof(NagaChunkEntry, m_offset) == 0x08, "m_offset");
static_assert(offsetof(NagaChunkEntry, m_size) == 0x0c, "m_size");

/* The reader (EE 0x20 on the callers' stacks): a cursor over a table.
 * find continues from the cursor, so callers rewind first. layout: host --
 * a pointer. */
struct NagaChunk
{
    NagaChunk();                                                         // ROR 0x001fca70
    NagaChunk(const void *top);                                          // ROR 0x001fca84
    ~NagaChunk();                                                        // ROR 0x001fcaac

    /* Reads host:<path> whole when no table is set; 1 on success. */
    int  load(const char *path);                                         // ROR 0x001fcb14
    void rewind();                                                       // ROR 0x001fcbf8
    /* The next entry; 0 at the end. */
    int  next();                                                         // ROR 0x001fcc20
    /* From the next entry on, the first whose name is tag's; 0 when none. */
    int  findTag(const NagaChunkTag *tag);                               // ROR 0x001fcc68
    int  find(const char *name);                                         // ROR 0x001fccf4
    void setIndex(int index);                                            // ROR 0x001fcd4c
    /* The current entry's name, zero-terminated (in m_name). */
    char *getName();                                                     // ROR 0x001fcd54
    NagaChunkEntry *getEntry();                                          // ROR 0x001fcda4
    void *getData();                                                     // ROR 0x001fcdb8
    int  getInt();                                                       // ROR 0x001fcdd4
    int  getSize();                                                      // ROR 0x001fcdf4

    NagaChunkEntry *m_top;      // ROR 0x00
    int             m_owned;    // ROR 0x04: frees m_top when set (load)
    int             m_end;      // ROR 0x08: set when the cursor ran off the table
    int             m_cur;      // ROR 0x0c: -1 before the first
    char            m_name[12]; // ROR 0x10: getName's copy
    int             _1c;        // ROR 0x1c
};

/* The writer (EE 0x510): up to 64 entries, each with its data in a block of
 * its own until build lays table and data out in one buffer. layout: host --
 * pointers; the EE offsets are in the comments. */
struct NagaChunkWriter
{
    enum { kMaxEntries = 64 };

    NagaChunkWriter();                                                   // ROR 0x001fc47c
    ~NagaChunkWriter();                                                  // ROR 0x001fc490

    void addEntry(const NagaChunkTag *tag, const void *data, int size);  // ROR 0x001fc4f0
    void add(const NagaChunkTag *tag, const void *data, int size);       // ROR 0x001fc5e4
    void add(const char *name, const void *data, int size);              // ROR 0x001fc600
    void add(const char *name, int value);                               // ROR 0x001fc678
    void add(const char *name, float value);                             // ROR 0x001fc6b4
    void add(const char *name, const char *str);                         // ROR 0x001fc6f0
    void add(const char *name, const NagaChunkWriter *chunk);            // ROR 0x001fc744
    void add(const NagaChunkTag *tag, int value);                        // ROR 0x001fc77c
    void add(const NagaChunkTag *tag, float value);                      // ROR 0x001fc7b8
    void add(const NagaChunkTag *tag, const char *str);                  // ROR 0x001fc7f4
    void add(const NagaChunkTag *tag, const NagaChunkWriter *chunk);     // ROR 0x001fc848
    void align(int alignment);                                           // ROR 0x001fc880
    /* Lays the table and the data out in m_buffer (once). */
    void build();                                                        // ROR 0x001fc89c
    /* Writes the built buffer to host:<path>; 1 on success. */
    int  save(const char *path);                                         // ROR 0x001fc9e8

    NagaChunkEntry m_entry[kMaxEntries]; // ROR 0x000
    int            m_count;              // ROR 0x400
    void          *m_data[kMaxEntries];  // ROR 0x404: each entry's data until build
    void          *m_buffer;             // ROR 0x504: the built table
    int            m_size;               // ROR 0x508: m_buffer's size
    int            m_offset;             // ROR 0x50c: the next entry's data offset
};

#endif /* _TASK_NAGA_CHUNK_H */
