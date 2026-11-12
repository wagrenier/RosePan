/* ==========================================================================
 *  wrd/wrdsrv.cpp
 *
 *  The room table (pinned 0x17e540..0x17f0d0; the file name is inferred
 *  from the class WrdSrv and data/wrd/): see wrdsrv.h. On the host setup
 *  leaves the file's offsets alone and builds WrdEntry records with real
 *  pointers beside it (the ROM relocates the words in place).
 * ======================================================================== */
#include "wrd/wrdsrv.h"

#include <stdio.h>
#include <string.h>

#include "bed/bedpfh.h"
#include "game/gamfile.h"
#include "utl/utlmisc.h"

/* data 0x298ee0, constructed and destroyed by the file's static
 * initializer: __static_initialization_and_destruction_0$wrdsrv 0x17f038,
 * _GLOBAL_$I$wrdsrv 0x17f090, _GLOBAL_$D$wrdsrv 0x17f0b0. */
WrdSrv g_wrdSrv;

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017e540

WrdSrv::WrdSrv()
{
    m_search = NULL;
    m_flags = 0;
    m_file = NULL;
    m_index = NULL;
    m_room = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017e564 // callcheck: operator_delete (the deleting variant, compiler generated)

WrdSrv::~WrdSrv()
{
    release();
}

// ──────────────────────────────────────────────────────────────────────
// Reads the pack's first sector (the RTPK header and its tables), finds
// file "%03d" of stage, reads it into the game heap and sets up a copy of
// it. The ROM never frees the read buffer when setup succeeds (setup
// always does): kept, the heap use is the ROM's.
// ROR 0x0017e5c0 // callcheck: BedPfh, FileReadReq, FileReader, ~FileReader (the locals' constructors and destructor)

int WrdSrv::load(int stage)
{
    if (stage == 99)
        return 1;

    FileReader reader;
    alignas(64) u_char head[0x800];
    FileReadReq req;
    req.m_close = 1;
    req.m_08 = 1;
    req.m_name = "data/wrd/WRD.BIN";
    req.m_size = 0x800;
    req.m_whence = 0;
    req.m_offset = 0;
    req.m_step = 0;
    req.m_buf = head;
    reader.read(&req, 0);

    BedPfh pfh;
    pfh.clear();
    char name[0x20];
    sprintf(name, "%03d", stage);
    pfh = BedPfh(head);
    int n = pfh.getFileIndex(name);
    int size = pfh.getFileSize(n);
    void *buf = g_utlMemory.alloc(size, -1);

    FileReadReq req2;
    req2.m_name = "data/wrd/WRD.BIN";
    req2.m_08 = 1;
    req2.m_whence = 0;
    req2.m_offset = pfh.getFileOffset(n);
    req2.m_close = 1;
    req2.m_buf = buf;
    req2.m_step = 0;
    req2.m_size = size;
    reader.read(&req2, 0);

    if (!setup(buf, 1))
    {
        g_utlMemory.free(buf);
        return 0;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Drops the table, allocates a search record per room, takes the file (a
// game-heap copy when copy is set) and finds its index and rooms. A file
// whose +2 bit 0 is set still holds offsets: the ROM turns them into
// pointers in place (the index entries, the rooms' five strings and link
// list) and clears the bit. Every room's +0x34 then points at its search
// record.
// ROR 0x0017e798

int WrdSrv::setup(void *file, int copy)
{
    WrdFileHead *src = (WrdFileHead *)file;
    release();
    m_search = (WrdSearch *)g_utlMemory.alloc(src->m_room_num * 0x10, -1);
    memset(m_search, 0, src->m_room_num * 0x10);
    if (copy == 0)
    {
        m_file = src;
    }
    else
    {
        m_file = (WrdFileHead *)g_utlMemory.alloc(src->m_size, -1);
        memcpy(m_file, file, src->m_size);
        m_flags |= 2;
    }

    u_char *base = (u_char *)m_file;
    const u_int *index = (const u_int *)(base + 0x10);
    u_int roomOfs = 0x10 + (m_file->m_index_num + 3) / 4 * 0x10;
    const WrdFileRoom *rooms = (const WrdFileRoom *)(base + roomOfs);

    /* PORT: the host keeps the offsets and builds the records with
     * pointers in one game-heap block (the index, then the rooms), whatever
     * bit 0 says; the bit is cleared as the ROM does. */
    m_file->m_flags &= ~1;
    int indexNum = m_file->m_index_num;
    int roomNum = m_file->m_room_num;
    m_index = (WrdEntry **)g_utlMemory.alloc(
        indexNum * (int)sizeof(WrdEntry *) + roomNum * (int)sizeof(WrdEntry), -1);
    m_room = (WrdEntry *)(m_index + indexNum);
    for (int i = 0; i < indexNum; i++)
    {
        u_int ofs = index[i];
        u_int k = (ofs - roomOfs) / 0x40;
        m_index[i] = ofs != 0 && ofs >= roomOfs && k < (u_int)roomNum ? &m_room[k] : NULL;
    }
    for (int i = 0; i < roomNum; i++)
    {
        const WrdFileRoom *r = &rooms[i];
        WrdEntry *e = &m_room[i];
        e->m_id = r->m_id;
        e->m_name = r->m_name ? (const char *)(base + r->m_name) : NULL;
        e->m_str0 = r->m_str0 ? (const char *)(base + r->m_str0) : NULL;
        e->m_mapName = r->m_mapName ? (const char *)(base + r->m_mapName) : NULL;
        e->m_text = r->m_text ? (const char *)(base + r->m_text) : NULL;
        e->m_str3 = r->m_str3 ? (const char *)(base + r->m_str3) : NULL;
        e->m_18 = r->m_18;
        e->m_mapArea = r->m_mapArea;
        e->m_ofs[0] = r->m_ofs[0];
        e->m_ofs[1] = r->m_ofs[1];
        e->m_ofs[2] = r->m_ofs[2];
        e->m_link_num = r->m_link_num;
        e->m_link = r->m_link ? (const int *)(base + r->m_link) : NULL;
        e->m_search = &m_search[i];
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017e9d0

void WrdSrv::release()
{
    if (m_search != NULL)
    {
        g_utlMemory.free(m_search);
        m_search = NULL;
    }
    /* PORT: the host's records (setup). */
    if (m_index != NULL)
        g_utlMemory.free(m_index);
    if ((m_flags & 2) == 0)
    {
        m_flags = 0;
    }
    else if (m_file == NULL)
    {
        m_flags = 0;
    }
    else
    {
        g_utlMemory.free(m_file);
        m_flags = 0;
    }
    m_file = NULL;
    m_room = NULL;
    m_index = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017ea44

WrdEntry *WrdSrv::get(int n)
{
    if (m_file == NULL)
        return NULL;
    if (n < 0 || m_file->m_index_num <= n)
        return NULL;
    return m_index[n];
}

// ──────────────────────────────────────────────────────────────────────
// The room whose name is name (no callers in the ROM).
// ROR 0x0017ea84

WrdEntry *WrdSrv::findByName(const char *name)
{
    if (m_file == NULL)
        return NULL;
    WrdEntry *e = m_room;
    size_t len = strlen(name);
    for (int i = 0; i < m_file->m_room_num; i++, e++)
    {
        if (memcmp(e->m_name, name, len) == 0 && strlen(e->m_name) == len)
            return e;
    }
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017eb3c

void WrdSrv::cnvPosL2W(float *out, int no, const float *in)
{
    if (m_file == NULL)
        return;
    const WrdEntry *e = get(no);
    /* PORT: the ROM reads low memory for a missing room; the host takes a
     * zero offset. */
    static const float s_zero[3] = {0.0f, 0.0f, 0.0f};
    const float *ofs = e != NULL ? e->m_ofs : s_zero;
    out[0] = ofs[0] + in[0];
    out[1] = ofs[1] + in[1];
    out[2] = ofs[2] + in[2];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017ebac

void WrdSrv::cnvPosW2L(float *out, const float *in, int no)
{
    if (m_file == NULL)
        return;
    const WrdEntry *e = get(no);
    /* PORT: as cnvPosL2W. */
    static const float s_zero[3] = {0.0f, 0.0f, 0.0f};
    const float *ofs = e != NULL ? e->m_ofs : s_zero;
    out[0] = in[0] - ofs[0];
    out[1] = in[1] - ofs[1];
    out[2] = in[2] - ofs[2];
}

// ──────────────────────────────────────────────────────────────────────
// A breadth-first search from `to` over the rooms' links, two alternating
// lists of at most 64 rooms each, until it reaches `from` (or a ring past
// the best distance found); each reached room's search record keeps its
// distance and the room it came from. The route is then read from `from`
// back along m_prev to `to`. from == to is a route of one room.
// ROR 0x0017ec1c

int WrdSrv::findRoute(int from, int to, WrdRoute *out)
{
    if (get(from) == NULL)
        return 0;
    if (get(to) == NULL)
        return 0;
    if (from == to)
    {
        out->m_room[0] = get(to);
        out->m_num = 1;
        return 1;
    }

    int list[2][64];
    int num[2];
    memset(num, 0, sizeof(num));
    int best = m_file->m_room_num + 1;
    clearSearch();
    WrdEntry *start = get(to);
    start->m_search->m_flags |= 1;
    start->m_search->m_dist = 0;
    list[0][0] = to;
    num[0] = 1;

    u_int cur = 0;
    do
    {
        for (int j = 0; j < num[cur]; j++)
        {
            WrdEntry *room = get(list[cur][j]);
            if (room->m_id == from || room->m_search->m_dist >= best)
                continue;
            for (int i = 0; i < room->m_link_num; i++)
            {
                WrdEntry *next = get(room->m_link[i]);
                WrdSearch *s = next->m_search;
                if ((s->m_flags & 1) != 0 && room->m_search->m_dist + 1 >= s->m_dist)
                    continue;
                int k;
                for (k = 0; k < num[cur]; k++)
                {
                    if (list[cur][k] == next->m_id)
                        break;
                }
                if (k < num[cur])
                    continue;
                s->m_flags |= 1;
                next->m_search->m_dist = room->m_search->m_dist + 1;
                next->m_search->m_prev = room->m_id;
                if (next->m_id == from)
                {
                    best = next->m_search->m_dist;
                    break;
                }
                u_int other = cur ^ 1;
                for (k = 0; k < num[other]; k++)
                {
                    if (list[other][k] == next->m_id)
                        break;
                }
                if (k == num[other])
                {
                    list[other][k] = next->m_id;
                    num[other] = k + 1;
                }
            }
        }
        num[cur] = 0;
        cur ^= 1;
    } while (num[cur] > 0);

    if (m_file->m_room_num < best)
        return 0;
    out->m_num = 0;
    for (;;)
    {
        WrdEntry *room = get(from);
        if (room->m_id == to)
        {
            out->m_room[out->m_num++] = room;
            return 1;
        }
        out->m_room[out->m_num++] = room;
        from = room->m_search->m_prev;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017eff4

void WrdSrv::clearSearch()
{
    WrdSearch *s = m_search;
    for (int i = 0; i < m_file->m_room_num; i++, s++)
        s->m_flags &= ~1u;
}
