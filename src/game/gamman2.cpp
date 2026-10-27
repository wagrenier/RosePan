// game/gamman2.cpp -- GamMan's methods past switchVolume (0x1d3b44..0x1d84c8).
//
// No source-path string here: the range follows game/gamman.cpp's last
// assert-bearing function and ends where game/gamstat.cpp's GamIdMap starts;
// every function is a GamMan method (the second file is a guess in RoR's
// magscene2.cpp / maglifepc2.cpp style). It holds the menu request
// (checkMenuReq), the object update list (+0x2246c), the hit tests over it,
// the save data writer / reader, the flag table 0x2fc1f8 and the secta
// queries over the scene's four secta lists (bss 0x7811ac, MagScene
// +0x8542c).
//
// Written so far: the update list, the flag table, the radius / bounds
// helpers and the secta queries. Still stubs (lib/unassigned_stubs.cpp):
// checkMenuReq with 0x1d4370, 0x1d4540; the hit walk 0x1d7c20 with its
// callbacks 0x1d4b9c / 0x1d50a8, testHitAt, placeAchr, 0x1d5610..0x1d5ec4;
// writeSaveData / readSaveData.

#include <string.h>

#include "game/gamman.h"

#include "lib/unassigned.h"
#include "mag/magsystem.h"

/* The scene's secta lists (bss 0x7811ac). */
static inline Obj17c0f0 &gamSectaWorld() { return g_magSystem.m_scene.m_8542c; }

/* The flag table (data 0x2fc1f8): one row per picture book / diary, the
 * columns as MnuPBook, MnuDiary, MnuItem, addItem and checkMenuReq read
 * them (0: 0x2000xx / 0x1000xx book id, 1: menu type, 2: item id, 3..7:
 * flag / message numbers); a row of -1 ends it. */
static const int s_flagTable[][8] = {
    {0x00200001, 0x1038, 0x0a17, 0x29, 0x3d, 0x3a, 0x41, 0x66}, // 0x002fc1f8
    {0x00200002, 0x1059, 0x0a18, 0x2a, 0x3e, 0x3b, 0x42, 0x67}, // 0x002fc218
    {0x00200003, 0x1063, 0x0a19, 0x2b, 0x3f, 0x3c, 0x43, 0x68}, // 0x002fc238
    {0x00200004, 0x106b, 0x0a1b, 0x2c, 0x40, 0x3d, 0x44, 0x69}, // 0x002fc258
    {0x00200009, 0x109d, 0x0a1c, 0x2d, 0x41, 0x3e, 0x45, 0x6a}, // 0x002fc278
    {0x0020000b, 0x10af, 0x0a1d, 0x2e, 0x42, 0x3f, 0x46, 0x6b}, // 0x002fc298
    {0x0020000c, 0x10ba, 0x0a1e, 0x2f, 0x43, 0x40, 0x47, 0x6c}, // 0x002fc2b8
    {0x00200005, 0x1074, 0x0a1f, 0x30, 0x44, 0x41, 0x48, 0x6d}, // 0x002fc2d8
    {0x00200006, 0x107e, 0x0a20, 0x31, 0x45, 0x42, 0x49, 0x6e}, // 0x002fc2f8
    {0x00200007, 0x1088, 0x0a21, 0x32, 0x46, 0x43, 0x4a, 0x6f}, // 0x002fc318
    {0x0020000a, 0x10a8, 0x0ab6, 0x33, 0x47, 0x44, 0x4b, 0x70}, // 0x002fc338
    {0x00100001, 0x1000, 0x0acc, 0x34, 0x48, 0x45, 0x4c, 0x71}, // 0x002fc358
    {0x00100002, 0x1024, 0x0acd, 0x35, 0x49, 0x46, 0x4d, 0x72}, // 0x002fc378
    {0x00100003, 0x102a, 0x0ace, 0x36, 0x4a, 0x47, 0x4e, 0x73}, // 0x002fc398
    {-1, -1, -1, -1, -1, -1, -1, -1},                           // 0x002fc3b8
};

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d44e4

int GamMan::isSeqPair0Pending()
{
    if (m_seqPair[0].m_b == 0 || m_seqPair[0].m_a == 0)
        return 0;
    if (g_sequence.m_global[100] != g_sequence.m_global[0x65])
        return 0;
    return (m_flags & 0x8000) == 0;
}

// ──────────────────────────────────────────────────────────────────────
// Shape 1..3 are fixed radii, 4 the record's own; *out (when given) the
// record's first word. An unknown shape returns 0 and leaves *out.
// ROR 0x001d4a3c

float GamMan::getObjRadius(u_int oid, int *out)
{
    GamObjColli colli;
    getObjParam(oid, 0x18, &colli);
    float r;
    switch (colli.m_shape)
    {
    case 0: r = 0.0f; break;
    case 1: r = 17.0f; break;
    case 2: r = 25.0f; break;
    case 3: r = 45.0f; break;
    case 4: r = colli.m_radius; break;
    default: return 0.0f;
    }
    if (out)
        *out = colli.m_kind;
    return r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d49d0

void GamMan::setObjRadius(u_int oid, float r)
{
    GamObjColli colli;
    g_gamMan.getObjParam(oid, 0x18, &colli);
    colli.m_shape = 4;
    colli.m_radius = r;
    g_gamMan.setObjParam(oid, 0x18, &colli);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d4aec

float GamMan::getObjKeepRadius(u_int oid, int *out)
{
    GamObjColli colli;
    getObjParam(oid, 0x1005, &colli);
    float r;
    switch (colli.m_shape)
    {
    case 0: r = 0.0f; break;
    case 1: r = 17.0f; break;
    case 2: r = 25.0f; break;
    case 3: r = 45.0f; break;
    case 4: r = colli.m_radius; break;
    default: return 0.0f;
    }
    if (out)
        *out = colli.m_kind;
    return r;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d4730

void GamMan::sortQuadCorners(float *quad)
{
    for (int i = 0; i < 3; i++)
    {
        for (int j = i + 1; j < 4; j++)
        {
            float *a = &quad[i * 2];
            float *b = &quad[j * 2];
            if (b[0] < a[0] && b[1] < a[1])
            {
                float x = b[0], z = b[1];
                b[0] = a[0];
                b[1] = a[1];
                a[0] = x;
                a[1] = z;
            }
        }
    }
    float x = quad[6], z = quad[7];
    quad[6] = quad[4];
    quad[7] = quad[5];
    quad[4] = x;
    quad[5] = z;
}

// ──────────────────────────────────────────────────────────────────────
// The corner quad (param 0x1003) sorted and stored back, the object moved
// to its centre (x / z), and param 0x18 made shape 4 with the quad's larger
// extent as its radius.
// ROR 0x001d480c

void GamMan::fitObjBounds(u_int oid)
{
    float pos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float quad[8];
    getObjPos(oid, pos);
    getObjParam(oid, 0x1003, quad);
    sortQuadCorners(quad);
    setObjParam(oid, 0x1003, quad);
    pos[0] = (quad[0] + quad[2] + quad[4] + quad[6]) * 0.25f;
    pos[2] = (quad[1] + quad[3] + quad[5] + quad[7]) * 0.25f;
    setObjPos(oid, pos);

    float minX = 10000.0f, minZ = 10000.0f;
    float maxX = -10000.0f, maxZ = -10000.0f;
    for (int i = 0; i < 4; i++)
    {
        float x = quad[i * 2], z = quad[i * 2 + 1];
        if (x < minX)
            minX = x;
        if (z < minZ)
            minZ = z;
        if (maxX < x)
            maxX = x;
        if (maxZ < z)
            maxZ = z;
    }
    float size = maxX - minX;
    if (size <= maxZ - minZ)
        size = maxZ - minZ;

    GamObjColli colli;
    g_gamMan.getObjParam(oid, 0x18, &colli);
    colli.m_shape = 4;
    colli.m_radius = size;
    g_gamMan.setObjParam(oid, 0x18, &colli);
}

// ──────────────────────────────────────────────────────────────────────
// A record for oid already on the list goes back to the free list first,
// so the oid ends up once, at the tail, with the new cell.
// ROR 0x001d5bc8

void GamMan::queueObjUpdate(GamSecta *cell, u_int oid)
{
    SeqPool<GamObjUpdate> *pool = m_pool2246c;
    for (SeqNode<GamObjUpdate> *node = pool->m_used.begin(); node != pool->m_used.end();
         node = node->m_next)
    {
        if (node->m_data.m_oid == oid)
        {
            pool->remove(node);
            break;
        }
    }
    GamObjUpdate rec;
    rec.m_oid = oid;
    rec.m_cell = cell;
    m_pool2246c->add(rec);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d5da0

void GamMan::relinkObj(u_int oid)
{
    SeqPool<GamObjUpdate> *pool = m_pool2246c;
    if (pool->m_used.m_count == 0)
        return;
    for (SeqNode<GamObjUpdate> *node = pool->m_used.begin(); node != pool->m_used.end();
         node = node->m_next)
    {
        if (node->m_data.m_oid == oid)
        {
            pool->remove(node);
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d7b08

int GamMan::getObjSectaNo(u_int oid, int list)
{
    /* PORT: zeroed; the ROM queries stack garbage for an oid that is
     * neither an achr nor an fchr. */
    float pos[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    switch (gamOidList(oid))
    {
    case 1: {
        ACHRstat *achr = getAchr(oid);
        if (gamOidType(oid) == 0)
            m_pcPtr->syncActorPos();
        achr->getPos(pos);
        break;
    }
    case 2:
        getFchr(oid)->getPos(pos);
        break;
    }
    GamSecta *hit[4] = {NULL, NULL, NULL, NULL};
    FUN_0017c340(&gamSectaWorld(), pos, hit, 0, 40.0f, 0.0f);
    if (hit[list] == NULL)
        return -1;
    setSectaList(0);
    return hit[list]->m_id;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d7de4

float GamMan::getFloorY(const float *pos)
{
    GamSecta *hit[4] = {NULL, NULL, NULL, NULL};
    FUN_0017c340(&gamSectaWorld(), pos, hit, 0, 40.0f, 0.0f);
    int list = m_sectaList;
    if (hit[list] != NULL)
        return getSectaCellY(hit[list], pos, list);
    int i = 0;
    while (i < 4 && hit[i] == NULL)
        i++;
    if (i == 4)
        return 0.0f;
    return getSectaCellY(hit[i], pos, i);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d7eb8

void GamMan::selectSectaList(int list)
{
    setSectaList(list);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d7ed4

GamSecta *GamMan::getSectaCellPtr(int no, int list)
{
    setSectaList(list);
    MagSectaList &l = gamSectaWorld().m_list[list];
    if (l.m_num == 0 || l.m_cells == NULL)
        return NULL;
    return &l.m_cells[no];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d7f48

GamSecta *GamMan::pickSectaCell(const float *pos, int list)
{
    setSectaList(list);
    /* PORT: cleared; the ROM leaves the answer array as stack garbage and
     * lets 0x17c340 fill it (its a3 0: no starting cells). */
    GamSecta *hit[4] = {NULL, NULL, NULL, NULL};
    FUN_0017c340(&gamSectaWorld(), pos, hit, 0, 40.0f, 0.0f);
    return hit[list];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d8064

GamSecta *GamMan::findSectaCell(const float *pos, GamSecta *cur, int list)
{
    setSectaList(list);
    GamSecta *hit[4];
    memset(hit, 0, sizeof(hit));
    hit[list] = cur;
    FUN_0017c340(&gamSectaWorld(), pos, hit, cur != NULL, 200.0f, 0.0f);
    GamSecta *cell = hit[list];
    if (cell != NULL && (cell->m_flags & 4) != 0)
        return NULL;
    return cell;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d811c

GamSecta *GamMan::getSectaCell(int no, float *pos, int list)
{
    MagSectaList &l = gamSectaWorld().m_list[list];
    GamSecta *cell = NULL;
    if (l.m_num != 0 && l.m_cells != NULL)
        cell = &l.m_cells[no];
    if (cell == NULL)
    {
        /* PORT: the ROM reads the position from address 0x10 (a NULL
         * cell); the host answers the origin. */
        pos[0] = pos[1] = pos[2] = 0.0f;
        return NULL;
    }
    pos[0] = cell->m_pos[0];
    pos[1] = cell->m_pos[1];
    pos[2] = cell->m_pos[2];
    return cell;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d817c

float GamMan::getSectaCellY(GamSecta *cell, const float *pos, int list)
{
    if (cell == NULL)
        return 100000.0f;
    return FUN_0017dd5c(&gamSectaWorld().m_list[list], cell->m_id, pos, 0.0f);
}

// ──────────────────────────────────────────────────────────────────────
// A cell number past an empty list's end gives a NULL start or goal;
// 0x17d298 decides what that means.
// ROR 0x001d81f0

int GamMan::findSectaPath(int list, int from, int to, GamSectaPath *path)
{
    GamSecta *fromCell[4];
    GamSecta *toCell[4];
    memset(fromCell, 0, sizeof(fromCell));
    memset(toCell, 0, sizeof(toCell));
    MagSectaList &l = gamSectaWorld().m_list[list];
    fromCell[list] = (l.m_num != 0 && l.m_cells != NULL) ? &l.m_cells[from] : NULL;
    toCell[list] = (l.m_num != 0 && l.m_cells != NULL) ? &l.m_cells[to] : NULL;
    MagSectaPathReq req;
    req.m_list = list;
    req.m_from = fromCell;
    req.m_to = toCell;
    req.m_path = path;
    if (FUN_0017d298(&l, &req) == 0)
        return 0;
    return path->m_num;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d83c0

void GamMan::setSectaList(int list)
{
    m_sectaList = list;
}

// ──────────────────────────────────────────────────────────────────────
// keyCol 0..4, 6, 7 search that column for key and answer column col of
// the row; 99 takes key as the row number (no bounds check, as in the
// ROM); 5 and the rest answer -1.
// ROR 0x001d83d0

int GamMan::lookupFlagTable(int keyCol, int key, int col)
{
    if (keyCol == 5)
        return -1;
    if (keyCol == 99)
        return s_flagTable[key][col];
    if (keyCol < 0 || keyCol > 7)
        return -1;
    for (int row = 0; s_flagTable[row][keyCol] != -1; row++)
    {
        if (s_flagTable[row][keyCol] == key)
            return s_flagTable[row][col];
    }
    return -1;
}
