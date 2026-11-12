/* ==========================================================================
 *  snd/unitque.h
 *
 *  A byte ring ("que", RoR's spelling) with its header in front of the
 *  data (snd/unitque.cpp 0x1830fc..0x1832e8). The movie player keeps its
 *  video elementary stream (0x80000 bytes) and each audio channel's samples
 *  (0x20000) in one; nothing else uses it.
 * ======================================================================== */
#ifndef _SND_UNITQUE_H
#define _SND_UNITQUE_H

#include <stddef.h>

/* The header; the data follows at +0x40. The words are ints, so the EE
 * offsets hold on the host. */
struct UnitQue
{
    int m_write;                // ROR 0x00: write position
    int m_read;                 // ROR 0x04: read position
    int m_count;                // ROR 0x08: bytes held
    int m_user;                 // ROR 0x0c: the owner's word (rxMpegAudioCallback: header seen); reset clears it
    int m_size;                 // ROR 0x10: data bytes
    int _14[(0x40 - 0x14) / 4];
};
static_assert(offsetof(UnitQue, m_write) == 0x00, "m_write");
static_assert(offsetof(UnitQue, m_read) == 0x04, "m_read");
static_assert(offsetof(UnitQue, m_count) == 0x08, "m_count");
static_assert(offsetof(UnitQue, m_user) == 0x0c, "m_user");
static_assert(offsetof(UnitQue, m_size) == 0x10, "m_size");
static_assert(sizeof(UnitQue) == 0x40, "UnitQue");

/* A que of size data bytes from rx memory hook 0. */
UnitQue       *unitQueCreate(int size);                                  // ROR 0x001830fc
void           unitQueDelete(UnitQue *pQue);                             // ROR 0x0018316c
void           unitQueReset(UnitQue *pQue);                              // ROR 0x00183194
/* The bytes readable at the read position without wrapping. */
int            unitQueGetReadable(UnitQue *pQue);                        // ROR 0x001831a8
int            unitQueGetCount(UnitQue *pQue);                           // ROR 0x001831c4
int            unitQueGetFree(UnitQue *pQue);                            // ROR 0x001831cc
/* Copies len bytes in, wrapping at the end; the caller checks the room. */
void           unitQueWrite(UnitQue *pQue, const void *data, int len);   // ROR 0x001831f8
unsigned char *unitQueGetReadPtr(UnitQue *pQue);                         // ROR 0x001832a4
void           unitQueConsume(UnitQue *pQue, int len);                   // ROR 0x001832b8

#endif /* _SND_UNITQUE_H */
