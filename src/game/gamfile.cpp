// game/gamfile.cpp -- the game's file reader (0x1be2cc..0x1beb70).
//
// A FileReader runs one FileReadReq at a time: open the request's name, seek,
// read, close -- each step only when the request's field asks for it, the
// field cleared once the step is issued. The steps go to the rx file layer
// asynchronously; between them the reader polls the file (isReady). A
// waiting reader draws the now-loading screen when main has one up, else
// keeps the frame and the sound going (rxThSync, SndMan::update); an async
// reader returns 1 instead and the caller polls again next frame.
//
// RoR does not name this file (no assert in it): game/ by link order (GamFont,
// then GamMan), gamfile after RoR's gam* names -- a guess.

#include "game/gamfile.h"

#include "main.h"
#include "rx/rxfile.h"
#include "rx/rxsys.h"
#include "rx/rxthread.h"
#include "snd/sndman.h"
#include "system/sysnowloading.h"

// The month lengths fileClockToSeconds sums (data 0x2b8da8). The ROM's table
// has 0 for December (the twelfth word is 0): kept, the year comes out 334
// days long (335 in a leap year).
static const int s_monthDays[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 0,
};

// ──────────────────────────────────────────────────────────────────────
// Opens the file for the request's open step (read-only, 0x8001); the fd
// lands in m_fd and the sync poll tells when the open is done.
// ROR 0x001be338

int FileReader::openFile(const char *theName)
{
    m_fd = rxSysStreamOpen(theName, 0x8001);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001be36c

int FileReader::readFile(int fd, void *buf, int size)
{
    return rxFileRead(fd, buf, size);
}

// ──────────────────────────────────────────────────────────────────────
// readFile on the reader's own file; -1 when none is open. No callers.
// ROR 0x001be38c

int FileReader::readCurrent(void *buf, int size)
{
    if (m_fd >= 0)
        return readFile(m_fd, buf, size);
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// Closes fd: 1 (and no file open any more) when it is the reader's file,
// 2 when it is another one.
// ROR 0x001be3c4

int FileReader::closeFile(int fd)
{
    if (fd == m_fd) {
        rxFileClose(fd);
        m_fd = -1;
        return 1;
    }
    rxFileClose(fd);
    return 2;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001be414

int FileReader::close()
{
    if (m_fd >= 0)
        return closeFile(m_fd);
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// Seeks fd: whence 1 / 2 pass as current / end, anything else as set.
// ROR 0x001be444

int FileReader::seekFile(int fd, int offset, int whence)
{
    int mode = 0;
    if (whence == 1)
        mode = 1;
    else if (whence > 1 && whence == 2)
        mode = 2;
    return rxFileSeek(fd, offset, mode);
}

// ──────────────────────────────────────────────────────────────────────
// seekFile on the reader's own file; -1 when none is open. No callers.
// ROR 0x001be490

int FileReader::seekCurrent(int offset, int whence)
{
    if (m_fd >= 0)
        return seekFile(m_fd, offset, whence);
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// True when the reader's file has no command running (rxFileSync without
// waiting); also when no file is open. Stores the fd to *fd when asked.
// ROR 0x001be4c8

int FileReader::isReady(int *unused, int *fd)
{
    int busy = 0;
    if (m_fd >= 0)
        busy = rxFileSync(m_fd, 1);
    if (fd != NULL)
        *fd = m_fd;
    return busy == 0;
}

// ──────────────────────────────────────────────────────────────────────
// Starts req from its first step and runs it (update).
// ROR 0x001be524

int FileReader::read(FileReadReq *req, int async)
{
    m_req = req;
    req->m_step = 0;
    return update(async);
}

// ──────────────────────────────────────────────────────────────────────
// Takes req where its step is and runs it on.
// ROR 0x001be548

int FileReader::resume(FileReadReq *req, int async)
{
    m_req = req;
    return update(async);
}

// ──────────────────────────────────────────────────────────────────────
// The request's state machine. A step that has work issues it, clears its
// field and sets the waiting bit; a step with nothing to do moves on. The
// waiting bit is cleared (the step advanced) once isReady reports the file
// idle: an async reader returns 1 while it is busy, a waiting one loops
// drawing the now-loading screen (or rxThSync + SndMan::update without one).
// Returns 0 once the close step is past.
// ROR 0x001be564

int FileReader::update(int async)
{
    for (;;) {
        FileReadReq *req = m_req;
        if (req->m_step & 1) {
            if (async) {
                int ready = isReady(NULL, NULL);
                if (ready < 0)
                    return -100;
                if (ready == 0)
                    return 1;
            } else {
                do {
                    if (g_nowLoading != NULL) {
                        g_nowLoading->draw();
                    } else {
                        rxThSync(1);
                        g_sndMan.update();
                    }
                } while (isReady(NULL, NULL) != 1);
            }
            m_req->m_step++;
            continue;
        }

        int step = req->m_step;
        switch (step >> 1) {
        case FILE_STEP_OPEN:
            if (req->m_name == NULL) {
                req->m_step = step + 2;
                continue;
            }
            openFile(req->m_name);
            m_req->m_fd = m_fd;
            m_req->m_name = NULL;
            break;
        case FILE_STEP_SEEK:
            if (req->m_whence < 0) {
                req->m_step = step + 2;
                continue;
            }
            seekFile(req->m_fd, req->m_offset, req->m_whence);
            m_req->m_whence = -1;
            break;
        case FILE_STEP_READ:
            if (req->m_size <= 0) {
                req->m_step = step + 2;
                continue;
            }
            readFile(req->m_fd, req->m_buf, req->m_size);
            m_req->m_size = 0;
            break;
        case FILE_STEP_CLOSE:
            if (req->m_close == 0) {
                req->m_step = step + 2;
                return 0;
            }
            closeFile(req->m_fd);
            m_req->m_close = 0;
            break;
        default:
            return 0;
        }
        m_req->m_step++;    // waiting on the step just issued
    }
}

// ──────────────────────────────────────────────────────────────────────
// The file's size: open it, seek to the end (the seek returns the position),
// close it, waiting out each command with rxThSync + SndMan::update. The
// open's error (< 0) when it does not open.
// ROR 0x001be768

int fileGetSize(const char *theName)
{
    int fd = rxSysStreamOpen(theName, 0x8001);
    int size = fd;
    if (fd >= 0) {
        while (rxFileSync(fd, 1) != 0) {
            rxThSync(1);
            g_sndMan.update();
        }
        size = rxFileSeek(fd, 0, 2);
        while (rxFileSync(fd, 1) != 0) {
            rxThSync(1);
            g_sndMan.update();
        }
        rxFileClose(fd);
        while (rxFileSync(fd, 1) != 0) {
            rxThSync(1);
            g_sndMan.update();
        }
    }
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// Whether the file opens (flags 1; closed again at once).
// ROR 0x001be84c

int fileExists(const char *theName)
{
    int fd = rxSysStreamOpen(theName, 1);
    if (fd >= 0)
        rxFileClose(fd);
    return fd >= 0;
}

// ──────────────────────────────────────────────────────────────────────
// No callers.
// ROR 0x001be880

int fileCdSetClockRead(int enable)
{
    return rxFileCdSetClockRead(enable);
}

// ──────────────────────────────────────────────────────────────────────
// No callers.
// ROR 0x001be894

int fileCdGetClockRead(void)
{
    return rxFileCdGetClockRead();
}

// ──────────────────────────────────────────────────────────────────────
// No callers.
// ROR 0x001be8a8

int fileCdGetClock(void *clock)
{
    return rxFileCdGetClock(clock);
}

// ──────────────────────────────────────────────────────────────────────
// The clock's bytes as seconds (they are read as binary, not the drive's
// BCD: a caller converts first). The year is leap when year % 4 == 0 and,
// for year % 100 == 0, year % 400 == 0. No caller but fileClockDiff.
// ROR 0x001be8bc

int fileClockToSeconds(const sceCdCLOCK *clock, int *sinceYear, int *toYearEnd,
                       int *yearLength)
{
    *sinceYear = 0;
    *toYearEnd = 0;
    if ((u_char)(clock->month - 1) >= 12)
        return 0;

    int leap = 0;
    if ((clock->year & 3) == 0) {
        leap = 1;
        if (clock->year % 100 == 0)
            leap = clock->year % 400 == 0;
    }

    int days = 0;
    for (int i = 0; i < 12; i++) {
        days += s_monthDays[i];
        if (i == 1)
            days += leap;
    }
    int length = days * 86400;
    *yearLength = length;

    int before = 0;
    for (int i = 0; i < clock->month - 1; i++) {
        before += s_monthDays[i];
        if (i == 1)
            before += leap;
    }
    int seconds = (((before + clock->day) * 24 + clock->hour) * 60 + clock->minute) * 60
                  + clock->second;
    *sinceYear = seconds;
    *toYearEnd = length - seconds;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The time from `from` to `to` in hours and minutes: within one year, or
// across one year end (to's year 0 follows 99); any other pair counts as
// 99:59, as does anything of 100 hours or more. No callers.
// ROR 0x001bea18

int fileClockDiff(const sceCdCLOCK *from, const sceCdCLOCK *to, int *hours,
                  int *minutes)
{
    *hours = 0;
    *minutes = 0;
    int s[6] = { 0, 0, 0, 0, 0, 0 };    // from: since, to end, length; to: the same
    if (fileClockToSeconds(from, &s[0], &s[1], &s[2]) != 1)
        return 0;
    if (fileClockToSeconds(to, &s[3], &s[4], &s[5]) != 1)
        return 0;

    int prevYear = to->year != 0 ? to->year - 1 : 99;
    int diff;
    if (from->year == to->year)
        diff = s[3] - s[0];
    else if (from->year == prevYear)
        diff = s[1] + s[3];
    else
        diff = 0x57e04;     // 99:59:00

    *hours = diff / 3600;
    if (diff / 3600 >= 100) {
        *hours = 99;
        *minutes = 59;
        return 1;
    }
    *minutes = (diff % 3600) / 60;
    return 1;
}
