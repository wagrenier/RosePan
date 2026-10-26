/* ==========================================================================
 *  wrd/wrdsrv.h
 *
 *  WrdSrv (wrd/wrdsrv.cpp, 0x17e540..0x17f0d0; the class is the ELF's,
 *  vtable _vt$6WrdSrv 0x2e4538, the file name is inferred from it and from
 *  the data directory): the room table. One global, g_wrdSrv (data
 *  0x298ee0), built by the file's static initializer. GamMan::loadStage
 *  calls load(stage): it reads file "%03d" of data/wrd/WRD.BIN (an RTPK of
 *  26 stages) and keeps a copy. A stage file is a WrdFileHead, an index of
 *  room numbers -> room offsets (0 = no room), then the 0x40-byte rooms:
 *  number, five strings (the room's id name "o1_cway", the floor name in
 *  Japanese and English, the room's caption, a second caption), the map
 *  words gamSetWrdRoom keeps, the room's offset in the world, and the rooms
 *  it links to. get(n) is room n (GamMan::m_wrdNo is the current one);
 *  cnvPosL2W / W2L move positions between the room and the world;
 *  findRoute walks the links breadth first.
 * ======================================================================== */
#ifndef _WRD_WRDSRV_H
#define _WRD_WRDSRV_H

#include <stddef.h>

#include "sdk/eetypes.h"

/* A stage file's header, file data. EE 0x10; the index (m_index_num
 * u_int offsets, padded to 16 bytes) follows it, the rooms follow that. */
struct WrdFileHead
{
    u_short m_version;          // ROR 0x00: 0x101 in every stage of the disc
    u_short m_flags;            // ROR 0x02: bit 0 = the offsets are not relocated yet
    int     m_size;             // ROR 0x04: the file's size (setup copies this much)
    int     m_index_num;        // ROR 0x08: index entries (the highest room number + 1)
    int     m_room_num;         // ROR 0x0c: rooms
};
static_assert(offsetof(WrdFileHead, m_flags) == 0x02, "m_flags");
static_assert(offsetof(WrdFileHead, m_size) == 0x04, "m_size");
static_assert(offsetof(WrdFileHead, m_index_num) == 0x08, "m_index_num");
static_assert(offsetof(WrdFileHead, m_room_num) == 0x0c, "m_room_num");
static_assert(sizeof(WrdFileHead) == 0x10, "WrdFileHead is 0x10 bytes on the EE");

/* A room as the file stores it. EE 0x40; offsets from the file's start
 * (0 = none), which the ROM turns into pointers in place. */
struct WrdFileRoom
{
    int   m_id;                 // ROR 0x00: the room number (its index slot)
    u_int m_name;               // ROR 0x04: "o1_cway" (findByName)
    u_int m_str0;               // ROR 0x08: the floor name, Shift-JIS
    u_int m_mapName;            // ROR 0x0c: "First Floor"
    u_int m_text;               // ROR 0x10: "Hallway"
    u_int m_str3;               // ROR 0x14: "Rickety Shed"
    int   m_18;                 // ROR 0x18: the floor (0 outside, 1 first, 2 second, 3 attic)
    int   m_mapArea;            // ROR 0x1c
    float m_ofs[3];             // ROR 0x20: the room's offset in the world
    int   m_link_num;           // ROR 0x2c
    u_int m_link;               // ROR 0x30: m_link_num room numbers
    u_int m_search;             // ROR 0x34: 0 in the file; setup points it at the room's WrdSearch
    u_char _38[0x40 - 0x38];
};
static_assert(offsetof(WrdFileRoom, m_name) == 0x04, "m_name");
static_assert(offsetof(WrdFileRoom, m_str0) == 0x08, "m_str0");
static_assert(offsetof(WrdFileRoom, m_mapName) == 0x0c, "m_mapName");
static_assert(offsetof(WrdFileRoom, m_text) == 0x10, "m_text");
static_assert(offsetof(WrdFileRoom, m_str3) == 0x14, "m_str3");
static_assert(offsetof(WrdFileRoom, m_18) == 0x18, "m_18");
static_assert(offsetof(WrdFileRoom, m_mapArea) == 0x1c, "m_mapArea");
static_assert(offsetof(WrdFileRoom, m_ofs) == 0x20, "m_ofs");
static_assert(offsetof(WrdFileRoom, m_link_num) == 0x2c, "m_link_num");
static_assert(offsetof(WrdFileRoom, m_link) == 0x30, "m_link");
static_assert(offsetof(WrdFileRoom, m_search) == 0x34, "m_search");
static_assert(sizeof(WrdFileRoom) == 0x40, "WrdFileRoom is 0x40 bytes on the EE");

/* findRoute's mark per room (setup allocates one per room). EE 0x10. */
struct WrdSearch
{
    u_int m_flags;              // ROR 0x00: bit 0 = reached
    int   m_dist;               // ROR 0x04: links from the search's start (findRoute's "to")
    int   m_prev;               // ROR 0x08: the room it was reached from
    int   _0c;
};
static_assert(offsetof(WrdSearch, m_dist) == 0x04, "m_dist");
static_assert(offsetof(WrdSearch, m_prev) == 0x08, "m_prev");
static_assert(sizeof(WrdSearch) == 0x10, "WrdSearch is 0x10 bytes on the EE");

/* A room as get() returns it: WrdFileRoom with its offsets made pointers.
 * layout: host -- the ROM relocates the file's words in place, which x64
 * pointers do not fit; setup builds these beside the file (EE offsets in
 * the comments are WrdFileRoom's). */
struct WrdEntry
{
    int          m_id;          // ROR 0x00
    const char  *m_name;        // ROR 0x04: "o1_cway"
    const char  *m_str0;        // ROR 0x08: gamGetWrdString 0 (the floor, Shift-JIS)
    const char  *m_mapName;     // ROR 0x0c: gamGetWrdString 1 / 4 (GamMan::getMapName; "First Floor")
    const char  *m_text;        // ROR 0x10: gamGetWrdString 2, S2dMes::drawMapName's caption
    const char  *m_str3;        // ROR 0x14: gamGetWrdString 3
    int          m_18;          // ROR 0x18: the floor (0 outside, 1 first, 2 second, 3 attic); gamSetWrdRoom keeps it in GamMan +0x18dd0
    int          m_mapArea;     // ROR 0x1c: GamMan::getMapArea (gamSetWrdRoom keeps it in GamMan +0x18dc0)
    float        m_ofs[3];      // ROR 0x20: the room's offset (cnvPosW2L / cnvPosL2W, gamWrdConvPos op 2)
    int          m_link_num;    // ROR 0x2c
    const int   *m_link;        // ROR 0x30: the linked room numbers
    WrdSearch   *m_search;      // ROR 0x34
};
static_assert(offsetof(WrdEntry, m_id) == 0x00, "m_id");
static_assert(offsetof(WrdEntry, m_name) >= 0x04, "m_name");

/* findRoute's answer. EE 0x100. layout: host -- room pointers. */
struct WrdRoute
{
    WrdEntry *m_room[63];       // ROR 0x00: from .. to
    int       m_num;            // ROR 0xfc
};
static_assert(offsetof(WrdRoute, m_room) == 0x00, "m_room");
static_assert(offsetof(WrdRoute, m_num) >= 0xfc, "m_num");

/* EE 0x18, vptr at +0x14. layout: host -- polymorphic (the host vptr is at
 * 0) and pointers; EE offsets in the comments. */
class WrdSrv
{
public:
    WrdSrv();                                                            // ROR 0x0017e540
    virtual ~WrdSrv();                                                   // ROR 0x0017e564 (EE slot 1)

    /* Reads stage's file of data/wrd/WRD.BIN and sets it up; stage 99
     * (the boot stage) keeps what is loaded. 1 on success. */
    int  load(int stage);                                                // ROR 0x0017e5c0
    /* Takes a stage file (copy: into the game heap) and makes its rooms. */
    int  setup(void *file, int copy);                                    // ROR 0x0017e798
    void release();                                                      // ROR 0x0017e9d0
    /* Room n, NULL when none is loaded, n is out of range or has no room. */
    WrdEntry *get(int n);                                                // ROR 0x0017ea44
    WrdEntry *findByName(const char *name);                              // ROR 0x0017ea84
    /* out = room no's offset + in, when a table is loaded. */
    void cnvPosL2W(float *out, int no, const float *in);                 // ROR 0x0017eb3c
    /* out = in - room no's offset, when a table is loaded. */
    void cnvPosW2L(float *out, const float *in, int no);                 // ROR 0x0017ebac
    /* The rooms from `from` to `to` along the links (the fewest links);
     * 1 found, 0 none (or a room missing). */
    int  findRoute(int from, int to, WrdRoute *out);                     // ROR 0x0017ec1c
    void clearSearch();                                                  // ROR 0x0017eff4

    u_int        m_flags;       // ROR 0x00: bit 1 = m_file is setup's copy
    WrdFileHead *m_file;        // ROR 0x04
    WrdEntry   **m_index;       // ROR 0x08: room number -> room (EE: the file's index)
    WrdEntry    *m_room;        // ROR 0x0c: the rooms (EE: the file's records)
    WrdSearch   *m_search;      // ROR 0x10
};

extern WrdSrv g_wrdSrv;         /* data 0x298ee0 */

#endif /* _WRD_WRDSRV_H */
