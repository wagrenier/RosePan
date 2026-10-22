/* ==========================================================================
 *  game/gamfile.h
 *
 *  The game's file reader (game/gamfile.cpp, 0x1be2cc..0x1beb70; RoR does
 *  not name the file): FileReader runs a FileReadReq -- open, seek, read,
 *  close, each step only when the request asks for it -- over the rx file
 *  layer, either waiting (drawing the now-loading screen, or keeping the
 *  sound going) or one poll per call. Every pack loader builds a request on
 *  the stack. Also fileGetSize / fileExists, three wrappers of the rx CD
 *  clock calls and two clock helpers (no callers in the ROM).
 * ======================================================================== */
#ifndef _GAME_GAMFILE_H
#define _GAME_GAMFILE_H

#include <stddef.h>
#include <string.h>

#include "sdk/libcdvd.h"

/* One read request: open the name, seek, read, close -- each step only
 * when its field asks for it (the reader clears the field once done).
 * EE 0x24. layout: host -- the name and buffer are pointers. */
struct FileReadReq
{
    // ROR 0x001be734 (memset 0x24 bytes, fd -1)
    FileReadReq()
    {
        memset(this, 0, sizeof(*this));
        m_fd = -1;
    }

    int         m_fd;           // ROR 0x00: the open file, -1 before the open step
    int         m_step;         // ROR 0x04: (step << 1) | waiting; FileReader::read resets it
    int         m_08;           // ROR 0x08: some callers set 1; the reader never reads it
    const char *m_name;         // ROR 0x0c: opened when set (rxSysStreamOpen, 0x8001)
    int         m_offset;       // ROR 0x10: the seek position
    int         m_whence;       // ROR 0x14: -1 = no seek, 0 / 1 / 2 = set / current / end
    void       *m_buf;          // ROR 0x18: read target
    int         m_size;         // ROR 0x1c: bytes to read, 0 = no read
    int         m_close;        // ROR 0x20: close the file after the read
};
static_assert(offsetof(FileReadReq, m_fd) == 0x00, "m_fd");
static_assert(offsetof(FileReadReq, m_step) == 0x04, "m_step");
static_assert(offsetof(FileReadReq, m_08) == 0x08, "m_08");
static_assert(offsetof(FileReadReq, m_name) >= 0x0c, "m_name");

/* FileReadReq::m_step >> 1: the step the request is at. */
enum
{
    FILE_STEP_OPEN  = 0,
    FILE_STEP_SEEK  = 1,
    FILE_STEP_READ  = 2,
    FILE_STEP_CLOSE = 3,        // 4 and up: done
};

/* Runs FileReadReqs (the pack loaders build one on the stack). EE 0x08.
 * layout: host -- holds a pointer. */
struct FileReader
{
    // ROR 0x001be2cc
    FileReader() : m_req(NULL), m_fd(-1) {}
    // ROR 0x001be2dc (GCC 2 destructor; the ROM's callers pass 2, no delete)
    ~FileReader()
    {
        if (m_fd >= 0)
            close();
    }

    /* The asynchronous steps (SprTexture::updateRead runs them one by one;
     * isReady polls the last one). */
    int  openFile(const char *theName);                                  // ROR 0x001be338
    int  readFile(int fd, void *buf, int size);                          // ROR 0x001be36c
    int  readCurrent(void *buf, int size);                               // ROR 0x001be38c
    /* 1 (and m_fd -1) when fd is this reader's file, else 2. */
    int  closeFile(int fd);                                              // ROR 0x001be3c4
    int  close();                                                        // ROR 0x001be414
    int  seekFile(int fd, int offset, int whence);                       // ROR 0x001be444
    int  seekCurrent(int offset, int whence);                            // ROR 0x001be490
    /* True when m_fd has no request running; stores m_fd to *fd. */
    int  isReady(int *unused, int *fd);                                  // ROR 0x001be4c8

    /* Starts the request from its first step and, unless async, waits for
     * it; 0 = done, 1 = still running (async only). */
    int  read(FileReadReq *req, int async);                              // ROR 0x001be524
    /* The same without resetting the request's step. */
    int  resume(FileReadReq *req, int async);                            // ROR 0x001be548
    /* Runs m_req's steps: 0 = done, 1 = waiting on the file (async). */
    int  update(int async);                                              // ROR 0x001be564

    FileReadReq *m_req;         // ROR 0x00: the request being run
    int          m_fd;          // ROR 0x04: the file this reader opened, -1 = none
};
static_assert(offsetof(FileReader, m_req) == 0x00, "m_req");

/* The size of the file (rxSysStreamOpen, seek to the end), < 0 when it
 * does not open. */
int  fileGetSize(const char *theName);                                   // ROR 0x001be768
int  fileExists(const char *theName);                                    // ROR 0x001be84c

int  fileCdSetClockRead(int enable);                                     // ROR 0x001be880
int  fileCdGetClockRead(void);                                           // ROR 0x001be894
int  fileCdGetClock(void *clock);                                        // ROR 0x001be8a8
/* Seconds from the start of the clock's year (counting its day from 1),
 * seconds left to the year's end, and the year's length in seconds;
 * 0 when the month is not 1..12, else 1. */
int  fileClockToSeconds(const sceCdCLOCK *clock, int *sinceYear, int *toYearEnd,
                        int *yearLength);                                // ROR 0x001be8bc
/* Hours and minutes from one clock to the next (same or previous year),
 * at most 99:59; 1 when both clocks are valid. */
int  fileClockDiff(const sceCdCLOCK *from, const sceCdCLOCK *to, int *hours,
                   int *minutes);                                        // ROR 0x001bea18

#endif /* _GAME_GAMFILE_H */
