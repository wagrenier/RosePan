/* ==========================================================================
 *  sct/sctlayer.cpp
 *
 *  The secta world (pinned 0x17c0f0..0x17e540; the file name is inferred
 *  from the ELF's class SctLayer): see sctlayer.h. On the host the file
 *  data keeps its offsets (the ROM relocates them in place) and the
 *  accessors in SctLayer resolve them.
 * ======================================================================== */
#include "sct/sctlayer.h"

#include <string.h>

#include "utl/utlmisc.h"

/* The cell walk's direction lists: SCT_DIRS_ALL (the start cell; the ROM
 * copies data 0x2f30a0 to the stack) and, for a cell reached by direction
 * d, data 0x298e20 + d * 0x18 -- d first, then its neighbours. */
static const int s_sctDirsAll[9] = { 0, 1, 2, 3, 4, 5, 6, 7, -1 };
static const int s_sctDirOrder[8][6] = {
    { 0, 4, 5, 1, 3, -1 },  // 0x00298e20
    { 1, 6, 4, 2, 0, -1 },  // 0x00298e38
    { 2, 7, 6, 3, 1, -1 },  // 0x00298e50
    { 3, 5, 7, 0, 2, -1 },  // 0x00298e68
    { 4, 1, 0, 6, 5, -1 },  // 0x00298e80
    { 5, 0, 3, 4, 7, -1 },  // 0x00298e98
    { 6, 2, 1, 7, 4, -1 },  // 0x00298eb0
    { 7, 3, 2, 5, 6, -1 },  // 0x00298ec8
};

static inline const int *sctDirList(int dirs)
{
    return dirs == SCT_DIRS_ALL ? s_sctDirsAll : s_sctDirOrder[dirs];
}

/* canWalkStraight's edges by direction 0..3 (data 0x2f30c8, copied to the
 * stack): the axis the segment crosses the edge on, the other axis (x 0,
 * z 2), the edge's bound and the bounds the crossing must lie between
 * (0..3 = the cell's x0, z0, x1, z1). */
struct SctEdge
{
    int m_axis;
    int m_other;
    int m_bound;
    int m_lo;
    int m_hi;
};
static_assert(sizeof(SctEdge) == 0x14, "SctEdge is 0x14 bytes on the EE");
static const SctEdge s_sctEdges[4] = {
    { 2, 0, 3, 0, 2 },      // 0x002f30c8: +z, crosses z1 between x0 and x1
    { 0, 2, 2, 1, 3 },      // 0x002f30dc: +x, crosses x1 between z0 and z1
    { 2, 0, 1, 0, 2 },      // 0x002f30f0: -z, crosses z0
    { 0, 2, 0, 1, 3 },      // 0x002f3104: -x, crosses x0
};

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c0f0 // callcheck: SctLayer (the member constructors, run by the host)

SctWorld::SctWorld()
{
    m_data = NULL;
    m_flags = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c14c // callcheck: operator_delete (the deleting variant; the layers' destructors run by the host)

SctWorld::~SctWorld()
{
    release();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c1f4

int SctWorld::setup(void *data, int copy)
{
    if (copy == 0)
    {
        m_data = (SctFileHead *)data;
    }
    else
    {
        int size = calcSize((const SctFileHead *)data);
        void *p = g_utlMemory.alloc(size, -1);
        memcpy(p, data, size);
        m_data = (SctFileHead *)p;
        m_flags |= 2;
    }

    for (int i = 0; i < 4; i++)
    {
        u_int ofs = m_data->m_layer[i];
        if (ofs != 0)
            m_list[i].setup((u_char *)m_data + ofs, 0);
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c2c0

void SctWorld::release()
{
    for (int i = 0; i < 4; i++)
        m_list[i].release();

    if ((m_flags & 2) && m_data != NULL)
        g_utlMemory.free(m_data);
    m_flags = 0;
    m_data = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c340

void SctWorld::findCells(const float *pos, GamSecta **hit, int keep, float yTol, float r)
{
    if (keep == 0)
    {
        for (int i = 3; i >= 0; i--)
            hit[i] = NULL;
    }
    for (int i = 0; i < 4; i++)
        hit[i] = m_list[i].findCellNear(pos, hit[i], yTol, r);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c3f0

int SctWorld::calcSize(const SctFileHead *head)
{
    int size = sizeof(SctFileHead);
    for (int i = 0; i < 4; i++)
    {
        u_int ofs = head->m_layer[i];
        if (ofs != 0)
            size += SctLayer::calcSize((const SctLayerHead *)((const u_char *)head + ofs));
    }
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c45c

void SctWorld::setFlags(const char *key, u_int bits, int rec)
{
    if (rec < 0)
    {
        for (int i = 0; i < 4; i++)
            m_list[i].setGroupFlags(key, bits);
    }
    else
    {
        m_list[rec].setGroupFlags(key, bits);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c4f4

void SctWorld::clearFlags(const char *key, u_int bits, int rec)
{
    if (rec < 0)
    {
        for (int i = 0; i < 4; i++)
            m_list[i].clearGroupFlags(key, bits);
    }
    else
    {
        m_list[rec].clearGroupFlags(key, bits);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c58c

SctLayer::SctLayer()
{
    m_groups = NULL;
    m_flags = 0;
    m_head = NULL;
    m_sectors = NULL;
    m_cells = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c5b0 // callcheck: operator_delete (the deleting variant, compiler generated)

SctLayer::~SctLayer()
{
    release();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c60c

int SctLayer::setup(void *data, int copy)
{
    release();
    if (copy == 0)
    {
        m_head = (SctLayerHead *)data;
    }
    else
    {
        int size = calcSize((const SctLayerHead *)data);
        void *p = g_utlMemory.alloc(size, -1);
        memcpy(p, data, size);
        m_head = (SctLayerHead *)p;
        m_flags |= 2;
    }

    m_sectors = (SctSector *)(m_head + 1);
    for (int i = 0; i < m_head->m_sector_num; i++)
        relocSector(&m_sectors[i]);

    // The cells start at the first sector's.
    // PC: the ROM reads sector 0 even when the layer has none; guarded.
    m_cells = m_head->m_sector_num > 0 ? getSectorCells(&m_sectors[0]) : NULL;
    if (m_head->m_group_num < 1)
        m_groups = NULL;
    else
        m_groups = (SctGroup *)(m_cells + m_head->m_cell_num);
    for (int i = 0; i < m_head->m_group_num; i++)
        relocGroup(&m_groups[i], m_cells, m_head->m_cell_num);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c754

void SctLayer::release()
{
    if ((m_flags & 2) && m_head != NULL)
        g_utlMemory.free(m_head);
    m_flags = 0;
    m_head = NULL;
    m_sectors = NULL;
    m_cells = NULL;
    m_groups = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c7b0

int SctLayer::calcSize(const SctLayerHead *head)
{
    int size = sizeof(SctLayerHead) + head->m_sector_num * (int)sizeof(SctSector)
             + head->m_cell_num * (int)sizeof(GamSecta) + head->m_group_num * (int)sizeof(SctGroup);
    const SctGroup *group = (const SctGroup *)((const u_char *)(head + 1)
                          + head->m_sector_num * sizeof(SctSector) + head->m_cell_num * sizeof(GamSecta));
    for (int i = 0; i < head->m_group_num; i++)
        size += group[i].m_cell_num * 4;
    return size;
}

// ──────────────────────────────────────────────────────────────────────
// The ROM relocates m_cells when bit 0 is set (and leaves the bit set).
// PC: the offset stays; getSectorCells resolves it.
// ROR 0x0017c814

void SctLayer::relocSector(SctSector *sector)
{
    (void)sector;
}

// ──────────────────────────────────────────────────────────────────────
// The ROM relocates m_cells when bit 0 is set and turns each member word
// (a cell index) into &cells[index] in place. PC: the offset and the
// indices stay; getGroupCell resolves them.
// ROR 0x0017c844

void SctLayer::relocGroup(SctGroup *group, GamSecta *cells, int num)
{
    (void)group;
    (void)cells;
    (void)num;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c8b8

GamSecta *SctLayer::findCell(const float *pos, GamSecta *start, float yTol)
{
    if (m_head == NULL)
        return NULL;

    int from = 0;
    if (start != NULL)
    {
        if (testCell(yTol, 0.0f, start, pos))
            return start;
        GamSecta *cell = findCellInSector(start->m_sector, pos, yTol);
        if (cell != NULL)
            return cell;
    }

    int no;
    while ((no = findSector(pos, from, 0.0f)) != -1)
    {
        GamSecta *cell = findCellInSector(no, pos, yTol);
        if (cell != NULL)
            return cell;
        from = no + 1;
    }
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017c994

GamSecta *SctLayer::findCellNear(const float *pos, GamSecta *start, float yTol, float r)
{
    if (m_head == NULL)
        return NULL;

    GamSecta *cell = findCell(pos, start, yTol);
    if (cell != NULL)
        return cell;
    if (r <= 0.0f)
        return NULL;

    if (start != NULL)
    {
        if (testCell(yTol, r, start, pos))
            return start;
        cell = findNearCellInSector(start->m_sector, pos, yTol, r);
        if (cell != NULL)
            return cell;
    }

    int from = 0;
    int no;
    while ((no = findSector(pos, from, r)) != -1)
    {
        cell = findNearCellInSector(no, pos, yTol, r);
        if (cell != NULL)
            return cell;
        from = no + 1;
    }
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017caa8

GamSecta *SctLayer::findCellInSector(int no, const float *pos, float yTol)
{
    SctSector *sector = &m_sectors[no];
    GamSecta *cell = getSectorCells(sector);
    for (int i = 0; i < sector->m_cell_num; i++, cell++)
    {
        if (testCell(yTol, 0.0f, cell, pos))
            return cell;
    }
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017cb50

GamSecta *SctLayer::findNearCellInSector(int no, const float *pos, float yTol, float r)
{
    SctSector *sector = &m_sectors[no];
    GamSecta *cell = getSectorCells(sector);
    GamSecta *best = NULL;
    float bestDist = 0.0f;
    for (int i = 0; i < sector->m_cell_num; i++, cell++)
    {
        if (!testCell(yTol, r, cell, pos))
            continue;
        float dx = pos[0] - cell->m_pos[0];
        float dy = pos[1] - cell->m_pos[1];
        float dz = pos[2] - cell->m_pos[2];
        float d = dx * dx + dy * dy + dz * dz;
        if (best == NULL || d < bestDist)
        {
            best = cell;
            bestDist = d;
        }
    }
    return best;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017cc68

int SctLayer::testCell(float yTol, float r, const GamSecta *cell, const float *pos)
{
    float y = getCellY(cell->m_id, pos, r);
    if (y == 100000.0f)
        return 0;
    float d = y - pos[1];
    return -yTol <= d && d <= yTol;
}

// ──────────────────────────────────────────────────────────────────────
// Inside the bounds grown by r, and for a triangle cell on its side of the
// diagonal: with the half's corner at (ax, az) and the far edges at bx /
// bz, 1 - (x - ax) / (bx - ax) <= (z - az) / (bz - az).
// ROR 0x0017cce8

int SctLayer::isInCell(int id, const float *pos, float r)
{
    const GamSecta *cell = &m_cells[id];
    float x = pos[0];
    float x0 = cell->m_x0 - r;
    float z0 = cell->m_z0 - r;
    float x1 = cell->m_x1 + r;
    float z1 = cell->m_z1 + r;
    if (!(x0 <= x) || !(x <= x1))
        return 0;
    float z = pos[2];
    if (!(z0 <= z) || !(z <= z1))
        return 0;

    float ax = 0.0f, bx = 0.0f, az = 0.0f, bz = 0.0f;
    switch (cell->m_flags & SCT_CELL_HALF_MASK)
    {
    case 0:
        return 1;
    case 0x10:
        ax = x0; bx = x1; az = z0; bz = z1;
        break;
    case 0x20:
        ax = x1; bx = x0; az = z0; bz = z1;
        break;
    case 0x40:
        ax = x0; bx = x1; az = z1; bz = z0;
        break;
    case 0x80:
        ax = x1; bx = x0; az = z1; bz = z0;
        break;
    default:                    // the ROM divides by 0 here
        break;
    }
    return 1.0f - (x - ax) / (bx - ax) <= (z - az) / (bz - az);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017ce74

int SctLayer::findSector(const float *pos, int from, float r)
{
    for (int i = from; i < m_head->m_sector_num; i++)
    {
        const SctSector *s = &m_sectors[i];
        if (s->m_x0 - r <= pos[0] && pos[0] <= s->m_x1 + r
         && s->m_z0 - r <= pos[2] && pos[2] <= s->m_z1 + r)
            return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// Breadth first from the goal over the sector links (two alternating
// 64-entry queues) until the start is reached; the route then follows
// m_prev from the start to the goal.
// ROR 0x0017cf0c // callcheck: memset (the queue counts' clear)

int SctLayer::searchSectorRoute(int from, int to, SctRoute *route)
{
    if (from == to)
    {
        route->m_num = 1;
        route->m_sector[0] = &m_sectors[to];
        return 1;
    }

    int queue[2][64];
    int count[2] = { 0, 0 };
    int limit = m_head->m_sector_num + 1;
    clearSectorMarks();
    m_sectors[to].m_dist = 0;
    m_sectors[to].m_flags |= 0x80000000;
    queue[0][0] = to;
    count[0] = 1;
    int cur = 0;

    do
    {
        for (int i = 0; i < count[cur]; i++)
        {
            SctSector *s = &m_sectors[queue[cur][i]];
            if (s->m_id == from || !(s->m_dist < limit))
                continue;
            for (int k = 0; s->m_link[k] != -1; k++)
            {
                SctSector *n = &m_sectors[s->m_link[k]];
                if ((int)n->m_flags < 0 && !(s->m_dist + 1 < n->m_dist))
                    continue;
                int j;
                for (j = 0; j < count[cur]; j++)
                    if (queue[cur][j] == n->m_id)
                        break;
                if (j < count[cur])
                    continue;

                n->m_flags |= 0x80000000;
                n->m_dist = s->m_dist + 1;
                n->m_prev = s->m_id;
                if (n->m_id == from)
                {
                    limit = n->m_dist;
                    break;
                }
                int other = cur ^ 1;
                for (j = 0; j < count[other]; j++)
                    if (queue[other][j] == n->m_id)
                        break;
                if (j == count[other])
                {
                    queue[other][j] = n->m_id;
                    count[other] = j + 1;
                }
            }
        }
        count[cur] = 0;
        cur ^= 1;
    } while (count[cur] > 0);

    if (m_head->m_sector_num < limit)
        return 0;

    route->m_num = 0;
    SctSector *s = &m_sectors[from];
    while (s->m_id != to)
    {
        route->m_sector[route->m_num++] = s;
        s = &m_sectors[s->m_prev];
    }
    route->m_sector[route->m_num++] = s;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017d298

int SctLayer::searchPath(SctPathReq *req)
{
    if (m_head == NULL)
        return 0;
    GamSecta *from = req->m_from[req->m_list];
    GamSecta *to = req->m_to[req->m_list];
    if (from == NULL || to == NULL)
        return 0;

    SctRoute route;
    if (!searchSectorRoute(from->m_sector, to->m_sector, &route))
        return 0;

    SctCellWalk walk;
    walk.m_from = from->m_id;
    walk.m_to = to->m_id;
    walk.m_route = &route;
    walk.m_path = req->m_path;
    searchCellPath(&walk);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Spreads walking costs from the start cell sector by sector along the
// route (expandCellPath), then through the last sector to the goal
// (finishCellPath). A dead end or an unreached goal ends the path at the
// cell those leave in the front instead. The path is the m_prev chain
// from that cell back to the start, written start first.
// ROR 0x0017d35c

void SctLayer::searchCellPath(SctCellWalk *walk)
{
    if (walk->m_from == walk->m_to)
    {
        walk->m_path->m_cell[0] = &m_cells[walk->m_from];
        walk->m_path->m_num = 1;
        return;
    }

    int ok = 1;
    clearCellMarks(walk->m_route);
    GamSecta *start = &m_cells[walk->m_from];
    start->m_mark |= 0x80000000;
    start->m_dist = 0.0f;
    start->m_dirs = SCT_DIRS_ALL;

    SctCellFront front;
    front.m_cur_num = 1;
    front.m_cur[0] = walk->m_from;
    SctSector **next = &walk->m_route->m_sector[1];
    for (int i = 1; i < walk->m_route->m_num; i++)
    {
        front.m_sector = (*next++)->m_id;
        if (!expandCellPath(&front))
        {
            ok = 0;
            break;
        }
        memcpy(front.m_cur, front.m_next, sizeof(front.m_cur));
        front.m_cur_num = front.m_next_num;
    }
    if (ok)
    {
        front.m_cell = walk->m_to;
        finishCellPath(&front);
    }

    GamSectaPath *path = walk->m_path;
    path->m_num = 1;
    for (const GamSecta *c = &m_cells[front.m_cell]; c->m_id != walk->m_from; c = &m_cells[c->m_prev])
        path->m_num++;
    GamSecta *c = &m_cells[front.m_cell];
    for (int i = path->m_num - 1; i >= 0; i--)
    {
        path->m_cell[i] = c;
        if (i > 0)              // PC: the ROM also forms the start cell's m_prev, unread
            c = &m_cells[c->m_prev];
    }
}

// ──────────────────────────────────────────────────────────────────────
// Breadth first over the cells of the sector the front leaves (two
// alternating 32-entry queues), from its border cells: a neighbour in the
// same sector is queued, one in the sector being entered joins the next
// front; each is taken when unreached or reached at a higher cost. False
// when no cell of the next sector was reached: the front's cell is then
// the cheapest cell next to a blocked one.
// ROR 0x0017d62c // callcheck: memset, memcpy (the queues' set up)

int SctLayer::expandCellPath(SctCellFront *front)
{
    int queue[2][32];
    int count[2] = { 0, 0 };
    GamSecta *best = NULL;
    memcpy(queue[0], front->m_cur, front->m_cur_num * 4);
    front->m_next_num = 0;
    count[0] = front->m_cur_num;
    int cur = 0;

    do
    {
        for (int i = 0; i < count[cur]; i++)
        {
            GamSecta *c = &m_cells[queue[cur][i]];
            for (const int *dir = sctDirList(c->m_dirs); *dir != -1; dir++)
            {
                int d = *dir;
                if (c->m_next[d] == -1)
                    continue;
                GamSecta *n = &m_cells[c->m_next[d]];
                if (n->m_sector != c->m_sector && n->m_sector != front->m_sector)
                    continue;
                u_int reached = n->m_mark & 0x80000000;
                if (reached && n->m_dist <= c->m_dist + c->m_cost[d])
                    continue;

                if (n->m_flags & SCT_CELL_BLOCKED)
                {
                    n->m_mark |= 0x80000000;
                    if (best == NULL || c->m_dist < best->m_dist)
                        best = c;
                    continue;
                }

                if (n->m_sector == front->m_sector)
                {
                    if (!reached)
                        front->m_next[front->m_next_num++] = n->m_id;
                }
                else
                {
                    int j;
                    for (j = 0; j < count[cur]; j++)
                        if (queue[cur][j] == n->m_id)
                            break;
                    if (j < count[cur])
                        continue;
                    int other = cur ^ 1;
                    for (j = 0; j < count[other]; j++)
                        if (queue[other][j] == n->m_id)
                            break;
                    if (j == count[other])
                    {
                        queue[other][j] = n->m_id;
                        count[other] = j + 1;
                    }
                }
                n->m_mark |= 0x80000000;
                n->m_dist = c->m_dist + c->m_cost[d];
                n->m_prev = c->m_id;
                n->m_dirs = d;
            }
        }
        count[cur] = 0;
        cur ^= 1;
    } while (count[cur] > 0);

    if (front->m_next_num > 0)
        return 1;
    front->m_cell = best->m_id;     // the ROM reads through NULL when nothing was blocked
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Breadth first over the last sector's cells from the front toward the
// goal cell. False when it was not reached: the front's cell is then the
// cheapest cell next to a blocked one, else the cheapest front cell.
// ROR 0x0017d9ac // callcheck: memset, memcpy (the queues' set up)

int SctLayer::finishCellPath(SctCellFront *front)
{
    int queue[2][32];
    int count[2] = { 0, 0 };
    memcpy(queue[0], front->m_cur, front->m_cur_num * 4);
    front->m_next_num = 0;
    count[0] = front->m_cur_num;
    GamSecta *best = NULL;
    int found = 0;
    int cur = 0;

    do
    {
        for (int i = 0; i < count[cur]; i++)
        {
            GamSecta *c = &m_cells[queue[cur][i]];
            if (c->m_id == front->m_cell)
            {
                found = 1;
                continue;
            }
            for (const int *dir = sctDirList(c->m_dirs); *dir != -1; dir++)
            {
                int d = *dir;
                if (c->m_next[d] == -1)
                    continue;
                GamSecta *n = &m_cells[c->m_next[d]];
                if (n->m_sector != c->m_sector)
                    continue;
                if ((int)n->m_mark < 0 && n->m_dist <= c->m_dist + c->m_cost[d])
                    continue;

                n->m_mark |= 0x80000000;
                n->m_dist = c->m_dist + c->m_cost[d];
                n->m_prev = c->m_id;
                n->m_dirs = d;
                if (n->m_flags & SCT_CELL_BLOCKED)
                {
                    if (best == NULL || c->m_dist < best->m_dist)
                        best = c;
                    if (n->m_id == front->m_cell)
                        break;
                    continue;
                }
                if (n->m_id == front->m_cell)
                {
                    found = 1;
                    break;
                }

                int j;
                for (j = 0; j < count[cur]; j++)
                    if (queue[cur][j] == n->m_id)
                        break;
                if (j < count[cur])
                    continue;
                int other = cur ^ 1;
                for (j = 0; j < count[other]; j++)
                    if (queue[other][j] == n->m_id)
                        break;
                if (j == count[other])
                {
                    queue[other][j] = n->m_id;
                    count[other] = j + 1;
                }
            }
        }
        count[cur] = 0;
        cur ^= 1;
    } while (count[cur] > 0);

    if (found)
        return 1;

    if (best == NULL)
    {
        best = &m_cells[front->m_cur[0]];
        for (int i = 1; i < front->m_cur_num; i++)
        {
            GamSecta *c = &m_cells[front->m_cur[i]];
            if (c->m_dist < best->m_dist)
                best = c;
        }
    }
    front->m_cell = best->m_id;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Flat (m_y), sloping along z (m_y_z0 at z0 to m_y at z1), along x
// (m_y_x0 at x0 to m_y at x1) or both (the z slope plus the x rise).
// ROR 0x0017dd5c

float SctLayer::getCellY(int id, const float *pos, float r)
{
    if (!isInCell(id, pos, r))
        return 100000.0f;

    const GamSecta *c = &m_cells[id];
    switch (c->m_flags & (SCT_CELL_SLOPE_Z | SCT_CELL_SLOPE_X))
    {
    case 0:
        return c->m_y;
    case SCT_CELL_SLOPE_Z:
        return c->m_y_z0 + ((c->m_y - c->m_y_z0) * (pos[2] - c->m_z0)) / (c->m_z1 - c->m_z0);
    case SCT_CELL_SLOPE_X:
        return c->m_y_x0 + ((c->m_y - c->m_y_x0) * (pos[0] - c->m_x0)) / (c->m_x1 - c->m_x0);
    case SCT_CELL_SLOPE_Z | SCT_CELL_SLOPE_X:
        return c->m_y_z0 + ((c->m_y - c->m_y_z0) * (pos[2] - c->m_z0)) / (c->m_z1 - c->m_z0)
                         + ((c->m_y - c->m_y_x0) * (pos[0] - c->m_x0)) / (c->m_x1 - c->m_x0);
    }
    return 100000.0f;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017ded0

int SctLayer::findNextCell(int id, const float *pos, float yTol)
{
    GamSecta *cells = m_cells;
    for (int i = 0; i < 8; i++)
    {
        int n = cells[id].m_next[i];
        if (n != -1 && testCell(yTol, 0.0f, &m_cells[n], pos))
            return n;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017dfa8

void SctLayer::clearSectorMarks()
{
    for (int i = 0; i < m_head->m_sector_num; i++)
        m_sectors[i].m_flags &= 0x7fffffff;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017dff0

void SctLayer::clearCellMarks(const SctRoute *route)
{
    if (route == NULL)
    {
        for (int i = 0; i < m_head->m_cell_num; i++)
            m_cells[i].m_mark &= 0x7fffffff;
        return;
    }
    for (int i = 0; i < route->m_num; i++)
    {
        const SctSector *s = route->m_sector[i];
        GamSecta *cell = getSectorCells(s);
        for (int k = 0; k < s->m_cell_num; k++, cell++)
            cell->m_mark &= 0x7fffffff;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Steps from from's cell across the edge the segment leaves it by (the
// major axis's edge tried first) until the segment ends inside the cell:
// true when that is to's cell.
// ROR 0x0017e0a8

int SctLayer::canWalkStraight(const float *from, const float *to, float yTol)
{
    GamSecta *cell = findCell(from, NULL, yTol);
    GamSecta *goal = findCell(to, NULL, yTol);
    if (cell == NULL || goal == NULL)
        return 0;

    float d[3];
    int sel[2];
    d[0] = to[0] - from[0];
    d[2] = to[2] - from[2];
    d[1] = 0.0f;                // PC: never read (the ROM leaves the stack word)
    sel[0] = from[0] <= to[0] ? 1 : 3;
    sel[1] = from[2] <= to[2] ? 0 : 2;
    float adz = d[2] < 0.0f ? -d[2] : d[2];
    float adx = d[0] < 0.0f ? -d[0] : d[0];
    if (adx < adz)
    {
        int t = sel[0];
        sel[0] = sel[1];
        sel[1] = t;
    }

    for (;;)
    {
        if (cell->m_flags & SCT_CELL_BLOCKED)
            return 0;
        const float *bound = &cell->m_x0;
        int k;
        for (k = 0; k < 2; k++)
        {
            const SctEdge *e = &s_sctEdges[sel[k]];
            float t = (bound[e->m_bound] - from[e->m_axis]) / d[e->m_axis];
            if (1.0f <= t)
            {
                k = 2;
                break;
            }
            float v = from[e->m_other] + d[e->m_other] * t;
            if (bound[e->m_lo] <= v && v <= bound[e->m_hi])
                break;
        }
        if (k == 2)
            return cell->m_id == goal->m_id;
        int n = cell->m_next[sel[k]];
        if (n == -1)
            return 0;
        cell = &m_cells[n];
    }
}

// ──────────────────────────────────────────────────────────────────────
// PC: guards a layer with no data (the ROM reads the header through NULL;
// the scripts flag every layer, loaded or not).
// ROR 0x0017e3d8 // callcheck: strcmp

SctGroup *SctLayer::findGroup(const char *key)
{
    if (m_head == NULL)
        return NULL;
    for (int i = 0; i < m_head->m_group_num; i++)
    {
        if (strcmp(m_groups[i].m_name, key) == 0)
            return &m_groups[i];
    }
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017e464

void SctLayer::setGroupFlags(const char *key, u_int bits)
{
    SctGroup *group = findGroup(key);
    if (group == NULL)
        return;
    for (int i = 0; i < group->m_cell_num; i++)
        getGroupCell(group, i)->m_flags |= bits;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0017e4d0

void SctLayer::clearGroupFlags(const char *key, u_int bits)
{
    SctGroup *group = findGroup(key);
    if (group == NULL)
        return;
    for (int i = 0; i < group->m_cell_num; i++)
        getGroupCell(group, i)->m_flags &= ~bits;
}
