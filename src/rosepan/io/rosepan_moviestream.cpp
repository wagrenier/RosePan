/* ==========================================================================
 *  rosepan_moviestream.cpp  --  the movie's bytes, read straight off the host
 *
 *  See rosepan_moviestream.h for what this replaces and why.
 *
 *  One open handle and a byte cursor.  Resolution follows the same order the
 *  rest of the port uses for a data file: the named extracted file if it is
 *  there, otherwise the span inside IMG_BD.BIN at `start_sector`.
 * ======================================================================== */

#include "rosepan_moviestream.h"

#include "rosepan_file.h"
#include "rosepan_paths.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{
RosePan_File *g_handle;
long long    g_base;        /* byte offset of the file inside the handle   */
long long    g_size;        /* its length                                  */
long long    g_pos;         /* how far the movie has read                  */

/* The archive every unextracted data file lives in. */
const char *const kArchive = "\\IMG_BD.BIN;1";

/* Open a resolved host path, or nullptr. */
RosePan_File *OpenResolved(const char *ps2_name)
{
    char host[1024];

    if (ps2_name == nullptr || ps2_name[0] == '\0')
    {
        return nullptr;
    }
    if (RosePan_PathResolveData(ps2_name, host, sizeof(host)) == 0)
    {
        return nullptr;
    }

    return RosePan_FileOpen(host, "rb");
}
}

extern "C" {

int RosePan_MovieStreamOpen(const RosePan_MovieStreamReq *req)
{
    if (req == nullptr || req->size <= 0)
    {
        return 0;
    }

    RosePan_MovieStreamClose();

    /* The extracted file, if this install has one. */
    g_handle = OpenResolved(req->file_name);
    if (g_handle != nullptr)
    {
        const long long len = (long long)RosePan_FileStreamSize(g_handle);

        g_base = 0;
        g_size = (len > 0 && len < (long long)req->size) ? len
                                                         : (long long)req->size;
        g_pos  = 0;
        printf("[movie] stream: named file '%s', %lld bytes (table says %d)\n",
               req->file_name, (long long)len, req->size);
        return 1;
    }

    /* Otherwise the archive span.  A negative or absurd sector means the CD
     * table entry is not one we can stream. */
    if (req->start_sector <= 0)
    {
        return 0;
    }

    g_handle = OpenResolved(kArchive);
    if (g_handle == nullptr)
    {
        printf("movie: no host file for '%s' and no IMG_BD.BIN -- falling back"
               " to the IOP stream path\n",
               req->file_name != nullptr ? req->file_name : "(unnamed)");
        return 0;
    }

    g_base = (long long)req->start_sector * 2048ll;
    g_size = (long long)req->size;
    g_pos  = 0;

    printf("[movie] stream: IMG_BD.BIN sector %d (offset %lld), %d bytes\n",
           req->start_sector, (long long)g_base, req->size);

    {
        const long long archive = (long long)RosePan_FileStreamSize(g_handle);

        if (archive > 0 && g_base >= archive)
        {
            printf("movie: sector %d is past the end of IMG_BD.BIN\n",
                   req->start_sector);
            RosePan_MovieStreamClose();
            return 0;
        }
    }

    return 1;
}

int RosePan_MovieStreamRead(void *buf, int size)
{
    if (buf == nullptr || size <= 0)
    {
        return 0;
    }

    if (g_handle == nullptr)
    {
        std::memset(buf, 0, (size_t)size);
        return size;
    }

    long long want = (long long)size;

    /* Never read past the file's own end -- the archive is one big blob and a
     * movie that ran off its span would demux the next file's bytes. */
    if (g_pos + want > g_size)
    {
        want = g_size - g_pos;
    }
    if (want < 0)
    {
        want = 0;
    }

    size_t got = 0;
    if (want > 0 &&
        RosePan_FileSeek(g_handle, g_base + g_pos, ROSEPAN_SEEK_SET) == 0)
    {
        got = RosePan_FileRead(g_handle, buf, (size_t)want);
    }

    g_pos += (long long)got;

    /* The tail, and everything after the end, is zeroes.  See the header: the
     * ROM's reader never reports "nothing", and the demuxer stops on the
     * stream's own terminator rather than on a short read. */
    if ((int)got < size)
    {
        std::memset((char *)buf + got, 0, (size_t)(size - (int)got));
    }

    return size;
}

void RosePan_MovieStreamClose(void)
{
    if (g_handle != nullptr)
    {
        RosePan_FileCloseHandle(g_handle);
        g_handle = nullptr;
    }

    g_base = 0;
    g_size = 0;
    g_pos  = 0;
}

int RosePan_MovieStreamIsOpen(void)
{
    return g_handle != nullptr ? 1 : 0;
}

}  /* extern "C" */
