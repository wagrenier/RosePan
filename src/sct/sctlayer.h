/* ==========================================================================
 *  sct/sctlayer.h
 *
 *  The secta world (sct/sctlayer.cpp, 0x17c0f0..0x17e540; the class
 *  SctLayer is the ELF's, vtable _vt$8SctLayer 0x2e4520, the file name is
 *  inferred from it). A sky pack's sector.sec describes where the floor is:
 *  four layers (SctLayer, MagScene +0x8542c holds them in an SctWorld), each
 *  a list of sectors (0x80-byte rectangles linked to their neighbours), the
 *  cells inside them (GamSecta, 0x90: a rectangle or one triangle half of
 *  it, a height that may slope in x and / or z, eight neighbour cells with
 *  a walking cost each) and named groups of cells that scripts flag
 *  (native 281 ops 200 / 201). Layer 0 is the floor the PC and Brown walk
 *  (MagLifeControlChara, GamMan's secta queries); MagScene's footsteps read
 *  the sector of the cell under the PC.
 *
 *  The queries: findCells / findCellNear (the cell under a position within
 *  a height tolerance, then the nearest one within a radius), getCellY (the
 *  floor height), searchPath (sector route breadth first, then a cost
 *  spread over the cells along it: GamMan::findSectaPath, Brown's
 *  searchPath), canWalkStraight (a segment across the cell edges).
 *
 *  PC: the ROM relocates the file's offsets into pointers in place (sector
 *  +0x60, group +0x14, the group member words). Host pointers do not fit in
 *  the 4-byte words, so the file data keeps its offsets and indices and
 *  the accessors below resolve them; everything else (the path marks the
 *  searches write into the sectors and cells) works on the file data as on
 *  the EE. Cell +0x88 (the direction list the cell walk spreads through,
 *  an EE pointer) holds a host index instead: SCT_DIRS_ALL or the
 *  direction the cell was reached by.
 * ======================================================================== */
#ifndef _SCT_SCTLAYER_H
#define _SCT_SCTLAYER_H

#include <stddef.h>

#include "sdk/eetypes.h"

/* sector.sec's header, file data. EE 0x20: the four layers' offsets from
 * the file's start (0 = no layer), then padding. */
struct SctFileHead
{
    u_int m_00;                 // ROR 0x00: 0x100 in the disc's files
    u_int m_layer[4];           // ROR 0x04
    u_int _14[3];
};
static_assert(offsetof(SctFileHead, m_layer) == 0x04, "m_layer");
static_assert(sizeof(SctFileHead) == 0x20, "SctFileHead is 0x20 bytes on the EE");

/* A layer's header, file data (EE 0x10). The sectors follow it, then the
 * cells, the groups and the groups' member words. */
struct SctLayerHead
{
    int m_no;                   // ROR 0x00: the layer's number (0..3)
    int m_sector_num;           // ROR 0x04
    int m_cell_num;             // ROR 0x08
    int m_group_num;            // ROR 0x0c
};
static_assert(offsetof(SctLayerHead, m_sector_num) == 0x04, "m_sector_num");
static_assert(offsetof(SctLayerHead, m_cell_num) == 0x08, "m_cell_num");
static_assert(offsetof(SctLayerHead, m_group_num) == 0x0c, "m_group_num");
static_assert(sizeof(SctLayerHead) == 0x10, "SctLayerHead is 0x10 bytes on the EE");

#define SCT_SECTOR_LINK_MAX 16
/* A sector, file data (EE 0x80): a rectangle of cells. */
struct SctSector
{
    int   m_id;                 // ROR 0x00: its index
    int   m_link[SCT_SECTOR_LINK_MAX]; // ROR 0x04: the sectors it touches, -1 ends the list
    int   m_cell_num;           // ROR 0x44
    int   m_snd;                // ROR 0x48: the footstep sound slot (1..11), < 0 = the map's default
    float m_x0;                 // ROR 0x4c: the bounds (x0 <= x <= x1, z0 <= z <= z1)
    float m_z0;                 // ROR 0x50
    float m_x1;                 // ROR 0x54
    float m_z1;                 // ROR 0x58
    u_int m_flags;              // ROR 0x5c: bit 0 = m_cells is an offset (always, on the disc); bit 31 = searchSectorRoute reached it
    u_int m_cells;              // ROR 0x60: its first cell, an offset from the sector (PC: never relocated)
    int   m_dist;               // ROR 0x64: searchSectorRoute: links from the goal
    int   m_prev;               // ROR 0x68: searchSectorRoute: the sector it was reached from
    u_char _6c[0x80 - 0x6c];
};
static_assert(offsetof(SctSector, m_link) == 0x04, "m_link");
static_assert(offsetof(SctSector, m_cell_num) == 0x44, "m_cell_num");
static_assert(offsetof(SctSector, m_snd) == 0x48, "m_snd");
static_assert(offsetof(SctSector, m_x0) == 0x4c, "m_x0");
static_assert(offsetof(SctSector, m_z1) == 0x58, "m_z1");
static_assert(offsetof(SctSector, m_flags) == 0x5c, "m_flags");
static_assert(offsetof(SctSector, m_cells) == 0x60, "m_cells");
static_assert(offsetof(SctSector, m_dist) == 0x64, "m_dist");
static_assert(offsetof(SctSector, m_prev) == 0x68, "m_prev");
static_assert(sizeof(SctSector) == 0x80, "SctSector is 0x80 bytes on the EE");

/* Cell flags (GamSecta::m_flags). */
#define SCT_CELL_SLOPE_Z    0x02    /* the height runs from m_y_z0 at z0 to m_y at z1 */
#define SCT_CELL_BLOCKED    0x04    /* the path searches stop at it; findSectaCell refuses it */
#define SCT_CELL_SLOPE_X    0x08    /* the height runs from m_y_x0 at x0 to m_y at x1 */
#define SCT_CELL_HALF_MASK  0xf0    /* 0 the whole rectangle, 0x10 / 0x20 / 0x40 / 0x80 one triangle half */

/* The cell walk's direction lists (cell +0x88 on the host). */
#define SCT_DIRS_ALL        (-1)    /* all eight directions (the start cell) */

/* A secta cell (EE 0x90; GamMan::findSectaCell's answer, the cells of an
 * SctLayer), file data: plain EE layout. Directions 0..3 are the edges
 * +z, +x, -z, -x (canWalkStraight's table), 4..7 the corners. */
struct GamSecta
{
    int   m_id;                     // ROR 0x00: ObjStat::get_o_secta_id
    int   m_sector;                 // ROR 0x04: its sector (MagScene's footstep sound)
    u_int m_flags;                  // ROR 0x08: SCT_CELL_*
    int   _0c;
    float m_pos[3];                 // ROR 0x10: the cell's point (x, y, z): TskMoveSectaFindPath walks to it
    float m_x0;                     // ROR 0x1c: the bounds an object leaves (x0 <= x <= x1, z0 <= z <= z1)
    float m_z0;                     // ROR 0x20
    float m_x1;                     // ROR 0x24
    float m_z1;                     // ROR 0x28
    float m_y;                      // ROR 0x2c: the floor height (at x1 / z1 on a slope)
    float m_y_x0;                   // ROR 0x30: the height at x0 (SCT_CELL_SLOPE_X)
    float m_y_z0;                   // ROR 0x34: the height at z0 (SCT_CELL_SLOPE_Z)
    int   _38;
    int   m_next[8];                // ROR 0x3c: the neighbour cell by direction, -1 none
    float m_cost[8];                // ROR 0x5c: the cost of stepping to it
    u_int m_mark;                   // ROR 0x7c: bit 31 = the cell walk reached it
    float m_dist;                   // ROR 0x80: the cell walk's cost from the start
    int   m_prev;                   // ROR 0x84: the cell it was reached from
    int   m_dirs;                   // ROR 0x88: the directions to spread through (an EE pointer; PC: SCT_DIRS_ALL or the arrival direction)
    int   _8c;
};
static_assert(offsetof(GamSecta, m_flags) == 0x08, "m_flags");
static_assert(offsetof(GamSecta, m_x0) == 0x1c, "m_x0");
static_assert(offsetof(GamSecta, m_pos) == 0x10, "m_pos");
static_assert(offsetof(GamSecta, m_z1) == 0x28, "m_z1");
static_assert(offsetof(GamSecta, m_sector) == 0x04, "m_sector");
static_assert(offsetof(GamSecta, m_y) == 0x2c, "m_y");
static_assert(offsetof(GamSecta, m_y_x0) == 0x30, "m_y_x0");
static_assert(offsetof(GamSecta, m_y_z0) == 0x34, "m_y_z0");
static_assert(offsetof(GamSecta, m_next) == 0x3c, "m_next");
static_assert(offsetof(GamSecta, m_cost) == 0x5c, "m_cost");
static_assert(offsetof(GamSecta, m_mark) == 0x7c, "m_mark");
static_assert(offsetof(GamSecta, m_dist) == 0x80, "m_dist");
static_assert(offsetof(GamSecta, m_prev) == 0x84, "m_prev");
static_assert(offsetof(GamSecta, m_dirs) == 0x88, "m_dirs");
static_assert(sizeof(GamSecta) == 0x90, "GamSecta is 0x90 bytes on the EE (GamMan indexes the cells)");

/* A named group of cells, file data (EE 0x20). */
struct SctGroup
{
    char  m_name[0x14];         // ROR 0x00: findGroup's key (the script's string)
    u_int m_cells;              // ROR 0x14: its member words, an offset from the group (PC: never relocated)
    int   m_cell_num;           // ROR 0x18
    u_int m_flags;              // ROR 0x1c: bit 0 = m_cells is an offset and the members are cell indices
};
static_assert(offsetof(SctGroup, m_cells) == 0x14, "m_cells");
static_assert(offsetof(SctGroup, m_cell_num) == 0x18, "m_cell_num");
static_assert(offsetof(SctGroup, m_flags) == 0x1c, "m_flags");
static_assert(sizeof(SctGroup) == 0x20, "SctGroup is 0x20 bytes on the EE");

/* GamMan::findSectaPath's answer: the cells from the start to the goal (at
 * most 127). layout: host -- cell pointers; EE offsets in the comments. */
#define GAM_SECTA_PATH_MAX 127
struct GamSectaPath
{
    GamSecta *m_cell[GAM_SECTA_PATH_MAX]; // ROR 0x000
    int       m_num;                      // ROR 0x1fc: cells in m_cell
};

/* searchSectorRoute's answer (EE 0x100 on searchPath's stack): the sectors
 * from the start to the goal. layout: host -- pointers. */
#define SCT_ROUTE_MAX 63
struct SctRoute
{
    SctSector *m_sector[SCT_ROUTE_MAX]; // ROR 0x00
    int        m_num;                   // ROR 0xfc
};

/* searchPath's request: the layer, the start and goal cells (four-entry
 * arrays, the request's layer entry set) and the answer. layout: host. */
struct SctPathReq
{
    int           m_list;       // ROR 0x00
    GamSecta    **m_from;       // ROR 0x04
    GamSecta    **m_to;         // ROR 0x08
    GamSectaPath *m_path;       // ROR 0x0c
};

/* searchCellPath's walk (EE 0x10 on its stack). layout: host. */
struct SctCellWalk
{
    int           m_from;       // ROR 0x00: the start cell
    int           m_to;         // ROR 0x04: the goal cell
    SctRoute     *m_route;      // ROR 0x08
    GamSectaPath *m_path;       // ROR 0x0c
};

/* The cell walk's frontier (EE 0x90 on searchCellPath's stack): the border
 * cells reached so far and the next sector's border cells expandCellPath
 * collects. */
#define SCT_FRONT_MAX 16
struct SctCellFront
{
    int m_sector;               // ROR 0x00: the sector being entered
    int m_cell;                 // ROR 0x04: finishCellPath's goal; the cell the path ends in
    int m_cur[SCT_FRONT_MAX];   // ROR 0x08
    int m_cur_num;              // ROR 0x48
    int m_next[SCT_FRONT_MAX];  // ROR 0x4c
    int m_next_num;             // ROR 0x8c
};
static_assert(offsetof(SctCellFront, m_cur_num) == 0x48, "m_cur_num");
static_assert(offsetof(SctCellFront, m_next) == 0x4c, "m_next");
static_assert(offsetof(SctCellFront, m_next_num) == 0x8c, "m_next_num");
static_assert(sizeof(SctCellFront) == 0x90, "SctCellFront is 0x90 bytes on the EE");

/* One layer of sector.sec (EE 0x18, vptr at +0x14). layout: host --
 * pointers and the vptr; EE offsets in the comments. */
class SctLayer
{
public:
    SctLayer();                                                          // ROR 0x0017c58c
    virtual ~SctLayer();                                                 // ROR 0x0017c5b0

    /* Takes a layer of sector.sec (copied to the game heap when copy is
     * set); 1. */
    int  setup(void *data, int copy);                                    // ROR 0x0017c60c
    void release();                                                      // ROR 0x0017c754
    /* The layer's size in bytes (setup's copy). */
    static int calcSize(const SctLayerHead *head);                       // ROR 0x0017c7b0

    /* The cell under pos whose floor is within yTol of pos's height:
     * start, start's sector, then the sectors holding pos; NULL none. */
    GamSecta *findCell(const float *pos, GamSecta *start, float yTol);   // ROR 0x0017c8b8
    /* findCell, then (r > 0) the nearest cell within r. */
    GamSecta *findCellNear(const float *pos, GamSecta *start, float yTol, float r); // ROR 0x0017c994
    /* The height of cell id under pos (r widens its bounds); 100000 when
     * pos is outside it. */
    float getCellY(int id, const float *pos, float r);                   // ROR 0x0017dd5c
    /* The first of cell id's neighbours whose floor under pos is within
     * yTol; its index, -1 none. */
    int   findNextCell(int id, const float *pos, float yTol);            // ROR 0x0017ded0
    /* Non-zero when a way through the cells from req->m_from to req->m_to
     * exists; the cells go into req->m_path. */
    int   searchPath(SctPathReq *req);                                   // ROR 0x0017d298
    /* Non-zero when the segment from -> to crosses the cells' edges into
     * to's cell without a blocked or missing cell. */
    int   canWalkStraight(const float *from, const float *to, float yTol); // ROR 0x0017e0a8
    /* Bits into (set) / out of (clear) the cells of the group key. */
    void  setGroupFlags(const char *key, u_int bits);                    // ROR 0x0017e464
    void  clearGroupFlags(const char *key, u_int bits);                  // ROR 0x0017e4d0

    /* PC: the relocated words the ROM reads, resolved from the offsets. */
    GamSecta *getSectorCells(const SctSector *sector) const
    {
        return (GamSecta *)((u_char *)sector + sector->m_cells);
    }
    GamSecta *getGroupCell(const SctGroup *group, int i) const
    {
        return &m_cells[((const int *)((const u_char *)group + group->m_cells))[i]];
    }

    u_int         m_flags;      // ROR 0x00: bit 1 = m_head is a heap copy
    SctLayerHead *m_head;       // ROR 0x04: the layer's data, NULL = no layer
    SctSector    *m_sectors;    // ROR 0x08
    GamSecta     *m_cells;      // ROR 0x0c
    SctGroup     *m_groups;     // ROR 0x10: NULL when the layer has none

private:
    static void relocSector(SctSector *sector);                          // ROR 0x0017c814
    static void relocGroup(SctGroup *group, GamSecta *cells, int num);   // ROR 0x0017c844
    GamSecta *findCellInSector(int no, const float *pos, float yTol);    // ROR 0x0017caa8
    GamSecta *findNearCellInSector(int no, const float *pos, float yTol, float r); // ROR 0x0017cb50
    int   testCell(float yTol, float r, const GamSecta *cell, const float *pos); // ROR 0x0017cc68
    int   isInCell(int id, const float *pos, float r);                   // ROR 0x0017cce8
    int   findSector(const float *pos, int from, float r);               // ROR 0x0017ce74
    int   searchSectorRoute(int from, int to, SctRoute *route);          // ROR 0x0017cf0c
    void  searchCellPath(SctCellWalk *walk);                             // ROR 0x0017d35c
    int   expandCellPath(SctCellFront *front);                           // ROR 0x0017d62c
    int   finishCellPath(SctCellFront *front);                           // ROR 0x0017d9ac
    void  clearSectorMarks();                                            // ROR 0x0017dfa8
    void  clearCellMarks(const SctRoute *route);                         // ROR 0x0017dff0
    SctGroup *findGroup(const char *key);                                // ROR 0x0017e3d8
};

/* MagScene +0x8542c (bss 0x7811ac): sector.sec and its four layers.
 * layout: host -- pointers in the layers. */
class SctWorld
{
public:
    SctWorld();                                                          // ROR 0x0017c0f0
    ~SctWorld();                                                         // ROR 0x0017c14c

    /* Takes a sky pack's sector.sec (copied to the game heap when copy is
     * set); 1. */
    int  setup(void *data, int copy);                                    // ROR 0x0017c1f4
    /* Frees the copy and the layers. */
    void release();                                                      // ROR 0x0017c2c0
    /* The cell of each layer under pos (hit[0..3], NULL none) within yTol
     * vertically, else the nearest within r; keep starts each search from
     * the cell already in hit (else hit is cleared first). Callers pass
     * 40 / 100 and 0 in f12 / f13. */
    void findCells(const float *pos, GamSecta **hit, int keep, float yTol, float r); // ROR 0x0017c340
    static int calcSize(const SctFileHead *head);                        // ROR 0x0017c3f0
    /* Bits into (set) / out of (clear) the cells of the group key, in
     * layer rec or all four when rec < 0 (native 281 ops 200 / 201). */
    void setFlags(const char *key, u_int bits, int rec);                 // ROR 0x0017c45c
    void clearFlags(const char *key, u_int bits, int rec);               // ROR 0x0017c4f4

    u_int        m_flags;       // ROR 0x00: bit 1 = m_data is a heap copy
    SctFileHead *m_data;        // ROR 0x04
    SctLayer     m_list[4];     // ROR 0x08: list 0 is the floor (MagScene's footsteps)
};

#endif /* _SCT_SCTLAYER_H */
