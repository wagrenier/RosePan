// task/naga_chunk.cpp -- chunk tables: the event packs' format (0x1fc47c..0x1fce0c).
//
// A table is 16-byte entries {name[8], offset, size} ended by a zero entry;
// an entry's data sits at the table's start + offset and may be a table
// itself. NagaChunk walks one with a cursor (scenePlayStart, DisplayMessage);
// NagaChunkWriter collects named ints, floats, strings, blobs and nested
// writers, lays them out behind their table (build) and saves the result to
// the host. Nothing in the retail game calls the writer or NagaChunk::load
// (the "host:" device is the development PC's): they are written for
// completeness.

#include "task/naga_chunk.h"

#include <stdio.h>
#include <string.h>

#include "bed/bedmemory.h"
#include "sdk/fileio.h"
#include "utl/utlmisc.h"

static const char s_file[] = "task/naga_chunk.cpp";

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc47c

NagaChunkWriter::NagaChunkWriter()
{
    m_size = 0;
    m_count = 0;
    m_offset = 0;
    m_buffer = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Frees the built buffer (the per-entry blocks are freed by build).
// callcheck: operator_delete (deleting variant)
// ROR 0x001fc490

NagaChunkWriter::~NagaChunkWriter()
{
    if (m_buffer != NULL)
    {
        utlMemFree(m_buffer);
    }
}

// ──────────────────────────────────────────────────────────────────────
// Appends an entry: its tag, its data at the running offset, a copy of the
// data in a block of its own.
// ROR 0x001fc4f0

void NagaChunkWriter::addEntry(const NagaChunkTag *tag, const void *data, int size)
{
    NagaChunkEntry *entry = &m_entry[m_count];
    memcpy(entry->m_name, tag->m_name, sizeof(entry->m_name));
    entry->m_offset = m_offset;
    entry->m_size = size;
    m_data[m_count] = g_utlMemory.allocDebug(size, s_file, 74, 0, 2);
    memcpy(m_data[m_count], data, size);
    m_count++;
    m_offset += size;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc5e4

void NagaChunkWriter::add(const NagaChunkTag *tag, const void *data, int size)
{
    addEntry(tag, data, size);
}

// ──────────────────────────────────────────────────────────────────────
// By name: the name zero-padded to 8 bytes (cut at 8).
// ROR 0x001fc600

void NagaChunkWriter::add(const char *name, const void *data, int size)
{
    NagaChunkTag tag;
    memset(&tag, 0, sizeof(tag));
    strncpy(tag.m_name, name, sizeof(tag.m_name));
    addEntry(&tag, data, size);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc678

void NagaChunkWriter::add(const char *name, int value)
{
    m_offset = (m_offset + 3) & ~3;
    int word = value;
    add(name, &word, 4);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc6b4

void NagaChunkWriter::add(const char *name, float value)
{
    m_offset = (m_offset + 3) & ~3;
    float word = value;
    add(name, &word, 4);
}

// ──────────────────────────────────────────────────────────────────────
// A string with its terminator (not aligned).
// ROR 0x001fc6f0

void NagaChunkWriter::add(const char *name, const char *str)
{
    add(name, (const void *)str, (int)strlen(str) + 1);
}

// ──────────────────────────────────────────────────────────────────────
// A nested table: another writer's built buffer.
// ROR 0x001fc744

void NagaChunkWriter::add(const char *name, const NagaChunkWriter *chunk)
{
    m_offset = (m_offset + 3) & ~3;
    add(name, chunk->m_buffer, chunk->m_size);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc77c

void NagaChunkWriter::add(const NagaChunkTag *tag, int value)
{
    m_offset = (m_offset + 3) & ~3;
    int word = value;
    add(tag, &word, 4);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc7b8

void NagaChunkWriter::add(const NagaChunkTag *tag, float value)
{
    m_offset = (m_offset + 3) & ~3;
    float word = value;
    add(tag, &word, 4);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc7f4

void NagaChunkWriter::add(const NagaChunkTag *tag, const char *str)
{
    add(tag, (const void *)str, (int)strlen(str) + 1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc848

void NagaChunkWriter::add(const NagaChunkTag *tag, const NagaChunkWriter *chunk)
{
    m_offset = (m_offset + 3) & ~3;
    add(tag, chunk->m_buffer, chunk->m_size);
}

// ──────────────────────────────────────────────────────────────────────
// Rounds the running data offset up to a power of two.
// ROR 0x001fc880

void NagaChunkWriter::align(int alignment)
{
    alignment -= 1;
    m_offset = (m_offset + alignment) & ~alignment;
}

// ──────────────────────────────────────────────────────────────────────
// One buffer: the table (the entries and a zero terminator; m_count then
// counts the terminator), the data behind it with every offset moved past
// the table, the size rounded to 16. Each entry's own block is freed.
// ROR 0x001fc89c

void NagaChunkWriter::build()
{
    if (m_buffer != NULL)
    {
        return;
    }
    m_count++;
    m_size = (m_count * (int)sizeof(NagaChunkEntry) + m_offset + 0xf) & ~0xf;
    m_buffer = g_utlMemory.allocDebug(m_size, s_file, 170, 0, 2);

    int tableSize = m_count * (int)sizeof(NagaChunkEntry);
    NagaChunkEntry *out = (NagaChunkEntry *)m_buffer;
    for (int i = 0; i < m_count - 1; i++, out++)
    {
        memcpy(out->m_name, m_entry[i].m_name, sizeof(out->m_name));
        out->m_offset = m_entry[i].m_offset + tableSize;
        out->m_size = m_entry[i].m_size;
        memcpy((u_char *)m_buffer + out->m_offset, m_data[i], m_entry[i].m_size);
        utlMemFree(m_data[i]);
    }
    memset(out, 0, sizeof(*out));
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc9e8

int NagaChunkWriter::save(const char *path)
{
    if (m_buffer == NULL)
    {
        return 0;
    }
    char name[64];
    sprintf(name, "host:%s", path);
    int fd = sceOpen(name, SCE_WRONLY | SCE_CREAT | SCE_TRUNC);
    if (fd < 0)
    {
        return 0;
    }
    sceWrite(fd, m_buffer, (m_size + 0xf) & ~0xf);
    sceClose(fd);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// An empty reader, for load.
// ROR 0x001fca70

NagaChunk::NagaChunk()
{
    m_owned = 0;
    m_top = NULL;
    m_cur = 0;
    m_end = 0;
}

// ──────────────────────────────────────────────────────────────────────
// A reader over a table in memory (not owned).
// ROR 0x001fca84

NagaChunk::NagaChunk(const void *top)
{
    m_top = (NagaChunkEntry *)top;
    m_owned = 0;
    m_cur = 0;
    m_end = 0;
    rewind();
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: operator_delete (deleting variant)
// ROR 0x001fcaac

NagaChunk::~NagaChunk()
{
    if (m_top != NULL && m_owned != 0)
    {
        utlMemFree(m_top);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fcb14

int NagaChunk::load(const char *path)
{
    if (m_top != NULL)
    {
        return 0;
    }
    char name[64];
    sprintf(name, "host:%s", path);
    int fd = sceOpen(name, SCE_RDONLY);
    if (fd < 0)
    {
        return 0;
    }
    int size = sceLseek(fd, 0, SCE_SEEK_END);
    m_top = (NagaChunkEntry *)g_utlMemory.allocDebug(size, s_file, 266, 0, 2);
    sceLseek(fd, 0, SCE_SEEK_SET);
    sceRead(fd, m_top, size);
    sceClose(fd);
    m_owned = 1;
    rewind();
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Back before the first entry; an empty table is at its end at once.
// ROR 0x001fcbf8

void NagaChunk::rewind()
{
    m_end = 0;
    m_cur = -1;
    if (*(const u_int *)m_top[0].m_name == 0)
    {
        m_end = 1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fcc20

int NagaChunk::next()
{
    if (m_end != 0)
    {
        return 0;
    }
    m_cur++;
    if (*(const u_int *)m_top[m_cur].m_name == 0)
    {
        m_end = 1;
        return 0;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Steps on until an entry's name matches; at the terminator the reader is at
// its end (callers rewind before the next search).
// ROR 0x001fcc68

int NagaChunk::findTag(const NagaChunkTag *tag)
{
    if (m_end != 0)
    {
        return 0;
    }
    const u_int *want = (const u_int *)tag->m_name;
    m_cur++;
    for (const NagaChunkEntry *entry = &m_top[m_cur];
         *(const u_int *)entry->m_name != 0; entry++)
    {
        const u_int *name = (const u_int *)entry->m_name;
        if (name[0] == want[0] && name[1] == want[1])
        {
            return 1;
        }
        m_cur++;
    }
    m_end = 1;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fccf4

int NagaChunk::find(const char *name)
{
    NagaChunkTag tag;
    memset(&tag, 0, sizeof(tag));
    strncpy(tag.m_name, name, sizeof(tag.m_name));
    return findTag(&tag);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fcd4c

void NagaChunk::setIndex(int index)
{
    m_cur = index;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fcd54

char *NagaChunk::getName()
{
    memset(m_name, 0, sizeof(m_name));
    strncpy(m_name, m_top[m_cur].m_name, sizeof(m_top[m_cur].m_name));
    return m_name;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fcda4

NagaChunkEntry *NagaChunk::getEntry()
{
    return &m_top[m_cur];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fcdb8

void *NagaChunk::getData()
{
    return (u_char *)m_top + m_top[m_cur].m_offset;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fcdd4

int NagaChunk::getInt()
{
    return *(const int *)getData();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fcdf4

int NagaChunk::getSize()
{
    return m_top[m_cur].m_size;
}
