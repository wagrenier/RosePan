/* ==========================================================================
 *  task/naga_chunk.h
 *
 *  task/naga_chunk.cpp (0x1fc4f0..; RoR's source string anchors 0x1fc4f0 /
 *  0x1fc89c / 0x1fcb14): the chunk table event packs are made of. Minimal:
 *  the reader scenePlayStart and DisplayMessage use. The rest of the file
 *  (the writers 0x1fc4f0..0x1fc89c, host: file save / load) is not
 *  declared yet.
 * ======================================================================== */
#ifndef _TASK_NAGA_CHUNK_H
#define _TASK_NAGA_CHUNK_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "utl/utlmisc.h"

/* One 16-byte table entry; a zero first word ends the table. EE layout
 * (the table comes from the event pack). */
struct NagaChunkEntry
{
    char m_name[8];             // ROR 0x00: zero-padded; compared as two words
    int  m_offset;              // ROR 0x08: the data, from the table's start
    int  m_0c;                  // ROR 0x0c: role unknown (the size?)
};
static_assert(sizeof(NagaChunkEntry) == 0x10, "NagaChunkEntry is 16 bytes");
static_assert(offsetof(NagaChunkEntry, m_offset) == 0x08, "m_offset");

/* The reader (EE 0x20 on the callers' stacks): a cursor over a table.
 * find continues from the cursor, so callers rewind first. layout: host --
 * a pointer. */
struct NagaChunk
{
    /* Written here ahead of the file (stubgen cannot stub a constructor). */
    // ROR 0x001fca84
    NagaChunk(const void *top)
    {
        m_top = (NagaChunkEntry *)top;
        m_owned = 0;
        m_cur = 0;
        m_end = 0;
        rewind();
    }
    // ROR 0x001fcaac
    ~NagaChunk()
    {
        if (m_top != NULL && m_owned != 0)
            utlMemFree(m_top);
    }

    void rewind();                                                       // ROR 0x001fcbf8
    /* The next entry; 0 at the end. */
    int  next();                                                         // ROR 0x001fcc20
    /* From the next entry on, the first whose name is tag's; 0 when none. */
    int  findTag(const NagaChunkEntry *tag);                             // ROR 0x001fcc68
    int  find(const char *name);                                         // ROR 0x001fccf4
    NagaChunkEntry *getEntry();                                          // ROR 0x001fcda4
    void *getData();                                                     // ROR 0x001fcdb8
    int  getInt();                                                       // ROR 0x001fcdd4

    NagaChunkEntry *m_top;      // ROR 0x00
    int             m_owned;    // ROR 0x04: frees m_top when set
    int             m_end;      // ROR 0x08
    int             m_cur;      // ROR 0x0c: -1 before the first
    int             _10[4];
};

#endif /* _TASK_NAGA_CHUNK_H */
