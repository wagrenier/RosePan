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
// helpers, the secta queries, the hit tests (the walk over the update
// list through the member pointer m_hitFunc: hitAtPoint / hitAtSweep) and
// checkMenuReq with restoreAfterMenu / endPcAction, the save image's
// writer / reader, and findSectaCellFrom (no callers).

#include <math.h>
#include <string.h>

#include "game/gamman.h"

#include "lib/unassigned.h"
#include "mag/magsystem.h"
#include "task/tsk_man.h"

/* The scene's secta lists (bss 0x7811ac). */
static inline SctWorld &gamSectaWorld() { return g_magSystem.m_scene.m_8542c; }

/* The hit walk's record count (data 0x2d8f58). */
static int s_hitWalkCount;

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
// The menus global 0x11 can ask for: items 0xa17..0xa19, 0xa1b..0xa21,
// 0xab6, 0xacc..0xace open their book (lookupFlagTable's column 2 ->
// column 0), 0xad0..0xad2 the map when they match the map area (0, 2, 1).
// Any other request collects the achrs and fchrs within 150 of the PC into
// scratch lists and drops them (nothing reads them). The ROM's switch on
// global 0x15 (jump table 0x2fc100, 11 cases) has only case 3.
// ROR 0x001d3b44

void GamMan::checkMenuReq()
{
    u_short req = (u_short)g_sequence.m_global[0x11];
    int bgm = (g_sequence.m_bits[1] & 0x4000000) == 0;

    if (g_sequence.m_global[0x15] == 3 && m_22a84 == -1)
    {
        m_flags |= 0x2000;
        if ((g_sequence.m_bits[1] & 0x20000000) == 0)
            m_flags2 |= 0x80000;

        int book = req == 0xab6 || (req >= 0xa17 && req < 0xa1a) || (req >= 0xa1b && req < 0xa22)
                || (req >= 0xacc && req < 0xacf);
        if (book)
        {
            if ((g_sequence.m_bits[1] & 0x20000000) == 0)
            {
                g_gamMan.m_flags |= 0x400;
                g_magSystem.m_scene.requestMenu(lookupFlagTable(2, req, 0), 0);
                enterMenuPause(0);
                m_22a84 = 2;
                m_flags2 &= ~0x80000u;
                return;
            }
        }
        else if (req >= 0xad0 && req < 0xad3)
        {
            if ((getMapArea() == 0 && req == 0xad0) || (getMapArea() == 1 && req == 0xad2)
                || (getMapArea() == 2 && req == 0xad1))
            {
                if (canOpenMapMenu() != 0)
                {
                    g_gamMan.m_flags |= 0x400;
                    g_magSystem.m_scene.requestMenu(0x10001, 0);
                    enterMenuPause(0);
                    m_22a84 = 1;
                    m_flags2 &= ~0x80000u;
                    return;
                }
            }
            else
            {
                m_flags2 |= 0x40000;
            }
        }
        else
        {
            // callcheck: operator_new[] (the node arrays)
            SeqPool<ACHRstat *> achrs;
            achrs.build(new SeqNode<ACHRstat *>[0x10], 0x10);
            SeqPool<FCHRstat *> fchrs;
            fchrs.build(new SeqNode<FCHRstat *>[0x20], 0x20);
            /* The ROM walks both lists to their ends and does nothing. */
            collectAchrsNear(m_pcPos, 150.0f, &achrs, -1, 0, 0);
            collectFchrsNear(m_pcPos, 150.0f, &fchrs, -1, 0, 0);
            // callcheck: operator_delete[] (the node arrays)
            delete[] fchrs.m_node;
            delete[] achrs.m_node;
        }
    }

    if (m_seqPair[0].m_b != 0 && m_seqPair[0].m_a != 0
        && g_sequence.m_global[100] == g_sequence.m_global[0x65] && (m_flags & 0x8000) == 0)
    {
        g_sequence.queueSingle(m_seqPair[0].m_b, m_seqPair[0].m_a, 0, 0x40);
        g_gamMan.resume(0x8000);
    }
    else if (m_seqPair[1].m_b != 0 && m_seqPair[1].m_a != 0 && (m_flags & 0x8000) == 0)
    {
        g_sequence.queueSingle(m_seqPair[1].m_b, m_seqPair[1].m_a, 0, 0x40);
        g_gamMan.resume(0x8000);
    }
    m_flags |= 2;
    if (g_sequence.m_global[0x28] == 0x13)
    {
        m_flags2 |= 0x80;
        g_sequence.m_global[0x28] = -1;
    }
    g_sequence.m_bits[1] &= ~0x10000000u;
    g_sequence.m_bits[1] &= ~0x20000000u;
    int start = g_taskMan.m_timer.m_updates + 1;
    // callcheck: operator_new (the new expression)
    TskFrameFunc *task = new TskFrameFunc(9, 0, 0);
    g_taskMan.addTask(1, start, 1, 0, task, 0, NULL, 1);
    restoreAfterMenu(bgm && (m_flags2 & 0x80000) == 0);
    if ((m_flags & 4) != 0)
        resume(0x100);
    m_22a84 = -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d4370

void GamMan::restoreAfterMenu(int bgm)
{
    if (bgm)
        m_sndMan.resumeBgm(0, 0, 1);
    if ((g_gamMan.m_flags2 & 0x100000) != 0)
    {
        int kinds[2] = {5, -1};
        g_sequence.m_flags &= ~1u;
        g_taskMan.pauseKinds(kinds, 0);
        g_gamMan.m_flags2 &= ~0x100000u;
    }
    if ((m_flags2 & 0x40000000) != 0)
    {
        m_pcActorPtr->m_motion.m_speed = m_pcMotionSpeed;
        /* PORT: no dog (-1) skipped; the ROM writes before the actors. */
        if (m_dogChrNo >= 0)
            g_magSystem.m_scene.m_actors[m_dogChrNo].m_motion.m_speed = m_dogMotionSpeed;
        m_flags2 &= ~0x40000000u;
    }
}

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
// ROR 0x001d4540

void GamMan::endPcAction()
{
    int b = m_seqPair[2].m_b;
    int a = m_seqPair[2].m_a;
    if (g_sequence.m_global[0x28] == 0x13)
    {
        u_int oid = (u_int)g_sequence.m_global[0x2a];
        FCHRstat *fchr = getVFchr(oid);
        if (fchr != NULL)
        {
            u_int flag[4];
            fchr->getParam(0x40, flag);
            /* PORT: the scripts give their items -1 here (seqFunc155 3 / 4's
             * flag word: no flag). On the EE m_bits + 0x1ffffffc wraps into the
             * uncached RAM mirror and sets bit 31 of m_bits[-1], its heap
             * header size (BedMemory::getHeader would assert on a free); the
             * host faults, so a bit outside m_bits' 0xa00 bytes is skipped. */
            if (flag[0] != 0 && flag[0] < 0xa00 * 8)
                g_sequence.m_bits[flag[0] >> 5] |= 1u << (flag[0] & 0x1f);
        }
        if (m_pcActor != -1)
            m_pcMotion = g_magSystem.m_scene.getActorMotion(m_pcActor);
        listAndReleaseChr(oid);
        m_flags2 |= 0x100;
    }
    int start = g_taskMan.m_timer.m_updates + 1;
    // callcheck: operator_new (the new expression)
    TskFrameFunc *task = new TskFrameFunc(0x11, 0, 0);
    g_taskMan.addTask(1, start, 1, 0, task, 0, NULL, 1);
    if (b != 0 && a != 0 && (m_flags & 0x8000) == 0)
    {
        g_sequence.queueSingle(b, a, 0, 0x40);
        g_gamMan.resume(0x8000);
    }
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
// m_hitFunc's point test: oid's box (its radius plus m_hitRadius, three
// radii for kind 3) round m_hitPos within 150 of height, then by kind: 1
// m_hitPos inside its corner quad (depth 10), 2 / 3 within the radii of
// its Bip01 Spine1 (kind 2) or its position, 3 also of its Bip01 Neck
// (m_far 1). Kind 4's test asks for a point outside the radius in x / z
// and inside it in 3D, which never holds (as in the ROM); 5 / 6 are
// never hit.
// ROR 0x001d4b9c

int GamMan::hitAtPoint(u_int oid, ChrHitRec *out)
{
    int kind;
    float r = getObjRadius(oid, &kind);
    if (out)
        memset(out, 0, sizeof(*out));

    float pos[4];
    float e;
    if (kind == 3)
    {
        /* The node's position is replaced by the object's either way. */
        chrGetNodeWorldPos(oid, pos, "Bip01 Spine1");
        if (getObjPos(oid, pos) == 0)
            return 0;
        e = r * 3.0f + m_hitRadius;
    }
    else
    {
        if (kind != 2 || chrGetNodeWorldPos(oid, pos, "Bip01 Spine1") == 0)
        {
            if (getObjPos(oid, pos) == 0)
                return 0;
        }
        e = r + m_hitRadius;
    }

    float dy = m_hitPos[1] - pos[1];
    if (dy < 0.0f ? dy <= -150.0f : 150.0f <= dy)
        return 0;
    float x = m_hitPos[0];
    float z = m_hitPos[2];
    if (!(pos[0] - e <= x) || !(x <= pos[0] + e) || !(pos[2] - e <= z) || !(z <= pos[2] + e))
        return 0;

    float d;
    switch (kind)
    {
    case 1: {
        float quad[8];
        getObjParam(oid, 0x1003, quad);
        const float *pts[6] = {&quad[0], &quad[2], &quad[4], &quad[6], &quad[0], NULL};
        if (m_area.isInPolygon(m_hitPos, pts) == 0)
            return 0;
        if (out == NULL)
            return 1;
        out->m_oid = oid;
        out->m_depth = 10.0f;
        out->m_far = 0;
        return 1;
    }
    case 2:
        d = sqrtf((pos[0] - x) * (pos[0] - x) + (pos[2] - z) * (pos[2] - z));
        if (!(d < r + m_hitRadius))
            return 0;
        break;
    case 3:
        d = sqrtf((pos[0] - x) * (pos[0] - x) + (pos[2] - z) * (pos[2] - z));
        if (d < r + m_hitRadius)
            break;
        if (chrGetNodeWorldPos(oid, pos, "Bip01 Neck") == 0)
            return 0;
        pos[0] -= m_hitPos[0];
        pos[2] -= m_hitPos[2];
        d = sqrtf(pos[0] * pos[0] + pos[2] * pos[2]);
        if (!(d < r + m_hitRadius))
            return 0;
        if (out == NULL)
            return 1;
        out->m_far = 1;
        out->m_oid = oid;
        out->m_depth = r - d;
        return 1;
    case 4: {
        d = sqrtf((pos[0] - x) * (pos[0] - x) + (pos[2] - z) * (pos[2] - z));
        if (!(r < d))
            return 0;
        float h = pos[1] - m_hitPos[1];
        d = sqrtf(d * d + h * h);
        if (!(d < r))
            return 0;
        break;
    }
    default:
        return 0;
    }
    if (out == NULL)
        return 1;
    out->m_oid = oid;
    out->m_far = 0;
    out->m_depth = r - d;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// m_hitFunc's sweep test: as hitAtPoint, but an object round the sweep
// end m_hitPos2 (its radius plus m_hitRadius2) is tested there (m_far 1).
// Kind 1 hits on the box alone (depth 0); kind 4 measures from the other
// end than kinds 2 / 3 and, as in hitAtPoint, can never hit.
// ROR 0x001d50a8

int GamMan::hitAtSweep(u_int oid, ChrHitRec *out)
{
    int kind;
    float r = getObjRadius(oid, &kind);
    float pos[4];
    getObjPos(oid, pos);

    float e = r + m_hitRadius;
    float e2 = r + m_hitRadius2;
    int atEnd = pos[0] - e2 <= m_hitPos2[0] && m_hitPos2[0] <= pos[0] + e2
             && pos[2] - e2 <= m_hitPos2[2] && m_hitPos2[2] <= pos[2] + e2;
    if (!atEnd)
    {
        if (m_hitPos[0] < pos[0] - e || pos[0] + e < m_hitPos[0]
            || m_hitPos[2] < pos[2] - e || pos[2] + e < m_hitPos[2])
            return 0;
    }

    float d;
    switch (kind)
    {
    case 1:
        if (out == NULL)
            return 1;
        out->m_oid = oid;
        out->m_depth = 0.0f;
        out->m_far = atEnd;
        return 1;
    case 2:
    case 3: {
        float cx, cz;
        if (atEnd)
        {
            cx = m_hitPos2[0];
            cz = m_hitPos2[2];
            r += m_hitRadius2;
        }
        else
        {
            cx = m_hitPos[0];
            cz = m_hitPos[2];
            r += m_hitRadius;
        }
        d = sqrtf((pos[0] - cx) * (pos[0] - cx) + (pos[2] - cz) * (pos[2] - cz));
        break;
    }
    case 4: {
        const float *c = atEnd ? m_hitPos : m_hitPos2;
        float dx = pos[0] - c[0];
        float dz = pos[2] - c[2];
        d = sqrtf(dx * dx + dz * dz);
        if (!(r < d))
            return 0;
        float dy = pos[1] - c[1];
        d = sqrtf(d * d + dy * dy);
        break;
    }
    default:
        return 0;
    }
    if (!(d < r))
        return 0;
    if (out == NULL)
        return 1;
    out->m_oid = oid;
    out->m_depth = r - d;
    out->m_far = atEnd;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The achr's hit test from its Bip01 Spine1 (or its position). Without
// arg, an achr of type 2 it hits is pushed away by a TskAvoidMove on its
// own task id (out's oid becomes this one's).
// ROR 0x001d544c

int GamMan::placeAchr(int oid, void *out, int arg)
{
    ACHRstat *achr = getAchr(oid);
    GamSecta *cell = achr->m_secta;
    if ((achr->m_flags & 0x80) != 0)
        return 0;
    if (chrGetNodeWorldPos(oid, m_hitPos, "Bip01 Spine1") == 0)
        achr->getPos(m_hitPos);
    m_hitRadius = getObjRadius(oid, NULL);
    m_hitFunc = &GamMan::hitAtPoint;
    ChrHitRec *hit = (ChrHitRec *)out;
    int ret = hitTestObjs(cell, oid, hit, 0);
    if (ret != 0 && hit != NULL)
    {
        u_int other = hit->m_oid;
        if (arg == 0 && gamOidType(other) == 2)
        {
            ACHRstat *chr = getAchr(other);
            hit->m_oid = oid;
            if (g_taskMan.getTask(chr->m_taskId) == 0)
            {
                // callcheck: operator_new (the new expression)
                TskAvoidMove *task = new TskAvoidMove(other, hit, 8, 0x1e, NULL, 0);
                g_taskMan.addTask(1, -1, 1, 0, task, 0, &chr->m_taskId, 0x11);
            }
        }
    }
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d5610

int GamMan::testAchrHit(u_int oid)
{
    if (isObj(oid) == 0)
        return 0;
    ACHRstat *achr = getAchr(oid);
    GamSecta *cell = achr->m_secta;
    if ((achr->m_flags & 0x80) != 0)
        return 0;
    float pos[4];
    achr->getPos(pos);
    memcpy(m_hitPos, pos, sizeof(m_hitPos));
    m_hitRadius = getObjRadius(oid, NULL);
    m_hitFunc = &GamMan::hitAtPoint;
    return hitTestObjs(cell, oid, NULL, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d56f4

int GamMan::testPcHitAt(const float *pos, ChrHitRec *out)
{
    if ((m_pcPtr->m_flags & 0x80) != 0)
        return 0;
    GamSecta *cell = findSectaCell(pos, m_pcPtr->m_secta, 0);
    memcpy(m_hitPos, pos, sizeof(m_hitPos));
    m_hitRadius = getObjRadius(m_21fcc, NULL);
    m_hitFunc = &GamMan::hitAtPoint;
    int ret = hitTestObjs(cell, m_21fcc, out, m_21fd0);
    if (ret == 0 || out == NULL)
        return ret;
    u_int other = out->m_oid;
    switch (gamOidType(other))
    {
    case 1:
        return 0;
    case 2: {
        ACHRstat *chr = getAchr(other);
        out->m_oid = m_21fcc;
        if (chr != NULL && g_taskMan.getTask(chr->m_taskId) == 0)
        {
            // callcheck: operator_new (the new expression)
            TskAvoidMove *task = new TskAvoidMove(other, out, 8, 0x1e, NULL, 0);
            g_taskMan.addTask(1, -1, 1, 0, task, 0, &chr->m_taskId, 0x11);
        }
        break;
    }
    }
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d58d8

int GamMan::setupDogSweep(float *pos, float angle)
{
    ObjStat *dog = m_dogPtr;
    if (dog == NULL || (dog->m_flags & 0x80) != 0)
        return 0;
    findSectaCell(pos, dog->m_secta, 0);
    memcpy(m_hitPos, pos, sizeof(m_hitPos));
    pos[0] += sinf(angle) * m_21fbc[2];
    pos[2] += cosf(angle) * m_21fbc[2];
    memcpy(m_hitPos2, pos, sizeof(m_hitPos2));
    m_hitRadius = m_21fbc[0];
    m_hitRadius2 = m_21fbc[1];
    m_hitFunc = &GamMan::hitAtSweep;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The hit test at pos for oid (radius 25 without one); a3 is a second oid
// not tested, a4 is not read. A hit copies the record to out; a type-2
// achr hit is pushed away (out's oid becomes oid).
// ROR 0x001d5a0c

int GamMan::testHitAt(const float *pos, u_int oid, int a3, int a4, void *out)
{
    (void)a4;
    if (testObjFlags(oid, 0x80) != 0)
        return 0;
    memcpy(m_hitPos, pos, sizeof(m_hitPos));
    m_hitRadius = oid != 0 ? getObjRadius(oid, NULL) : 25.0f;
    m_hitFunc = &GamMan::hitAtPoint;
    /* PORT: cleared; hitAtPoint fills it only when it is reached. */
    ChrHitRec hit;
    memset(&hit, 0, sizeof(hit));
    int ret = hitTestObjs(NULL, oid, &hit, (u_int)a3);
    if (out != NULL && ret != 0)
    {
        ChrHitRec *rec = (ChrHitRec *)out;
        *rec = hit;
        u_int other = rec->m_oid;
        if (gamOidType(other) == 2)
        {
            ACHRstat *chr = getAchr(other);
            rec->m_oid = oid;
            if (g_taskMan.getTask(chr->m_taskId) == 0)
            {
                // callcheck: operator_new (the new expression)
                TskAvoidMove *task = new TskAvoidMove(other, rec, 8, 0x1e, NULL, 0);
                g_taskMan.addTask(1, -1, 1, 0, task, 0, &chr->m_taskId, 0x11);
            }
        }
    }
    return ret;
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
// The ROM pushes the dropped records at the free list's head; the host's
// remove appends them (only which free node is reused next differs).
// ROR 0x001d5ec4

void GamMan::clearObjUpdates(int mode)
{
    SeqPool<GamObjUpdate> *pool = m_pool2246c;
    if (pool->m_used.m_count == 0 || mode < 0)
        return;
    if (mode < 3)
    {
        pool->clear();
        return;
    }
    if (mode != 3)
        return;
    SeqNode<GamObjUpdate> *node = pool->m_used.begin();
    while (node != pool->m_used.end())
    {
        SeqNode<GamObjUpdate> *next = node->m_next;
        if (node->m_data.m_oid >> 0x1e == 3)
            m_pool2246c->remove(node);
        node = next;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The save image (MnuSvld's 0x3000 bytes, EE layout): a 0x40-byte header,
// the sequence globals and bits (variable length), 8 bytes, the PC's and
// the dog's ObjStat blocks (0xb0 each), then the sections below at fixed
// offsets from there (version 0x10). Nothing past the header is aligned
// for the host's types: every word goes through memcpy.

enum
{
    SAVE_VERSION      = 0x10,
    SAVE_HDR_CHAPTER  = 0x10,   /* m_chapter */
    SAVE_HDR_INU      = 0x14,   /* m_inuMode */
    SAVE_HDR_18       = 0x18,   /* 0x2d10 (nothing reads it back) */
    SAVE_HDR_MINUTES  = 0x1c,   /* getPlayMinutes */
    SAVE_HDR_EQUIP    = 0x20,   /* m_equipItem */
    SAVE_HDR_FIND     = 0x24,   /* m_findTargetItem */
    SAVE_HDR_VERSION  = 0x28,
    SAVE_HDR_SIZE     = 0x40,
    /* From the item block (after the two ObjStat blocks): */
    SAVE_ITEM         = 0x000,  /* 12 words: id | num << 16 */
    SAVE_STOCK_NUM    = 0x030,
    SAVE_STOCK        = 0x038,  /* up to 0x14e words */
    SAVE_MARK         = 0x56c,  /* the byte 'N' */
    SAVE_OIDS         = 0x57c,  /* m_1f240's 0x169 words */
    SAVE_OBJRESET     = 0xb20,  /* 6-byte GamObjReset records (0x3c0 + 8) */
    SAVE_VFCHR        = 0xee8,  /* 0x26 SaveVFchr records */
    SAVE_OBJSET       = 0x13a8, /* 8-byte GamObjSet records (0xc00) */
    SAVE_OPTIONS      = 0x1fa8, /* 12 words from m_optOutput */
    SAVE_BGM          = 0x1fd8, /* the GamBgmRec */
    SAVE_ITEMFLAGS    = 0x1ffc, /* the item records' flag bytes */
    SAVE_ITEMFLAG_NUM = 0x1c7,
};

/* A vfchr object of the save image (EE 0x20). */
struct SaveVFchr
{
    u_int   m_oid;              // ROR 0x00: 0 ends the list
    short   m_kind;             // ROR 0x04: FCHRstat m_kind
    u_short m_flags;            // ROR 0x06: FCHRstat m_flags' low half
    int     m_wrd;              // ROR 0x08: param 8 (addVFchr's arg)
    int     m_model;            // ROR 0x0c: param 0x1000
    float   m_pos[4];           // ROR 0x10: param 0x43
};
static_assert(sizeof(SaveVFchr) == 0x20, "SaveVFchr");
static_assert(offsetof(SaveVFchr, m_kind) == 0x04, "m_kind");
static_assert(offsetof(SaveVFchr, m_flags) == 0x06, "m_flags");
static_assert(offsetof(SaveVFchr, m_wrd) == 0x08, "m_wrd");
static_assert(offsetof(SaveVFchr, m_model) == 0x0c, "m_model");
static_assert(offsetof(SaveVFchr, m_pos) == 0x10, "m_pos");

static inline void saveWord(u_char *p, u_int v) { memcpy(p, &v, 4); }
static inline u_int loadWord(const u_char *p) { u_int v; memcpy(&v, p, 4); return v; }

/* The options block (+0x21f8c..+0x21fbc, 12 words). */
static int *gamOptionWord(GamMan *gam, int i)
{
    int *word[12] = {
        &gam->m_optOutput, &gam->m_optVibration, &gam->m_21f94, &gam->m_optMasterLevel,
        &gam->m_optBrightLevel, &gam->m_masterVolume, &gam->m_optSeLevel, &gam->m_optStrLevel,
        &gam->m_21fac[0], &gam->m_21fac[1], &gam->m_21fac[2], &gam->m_21fac[3],
    };
    return word[i];
}

/* The item word id | num << 16 (the slot's EE +0x8 word). */
static inline u_int gamItemWord(const GamItemSlot &slot)
{
    return (u_short)slot.m_id | (u_int)slot.m_num << 16;
}

static inline void gamSetItemWord(GamSysData &sys, GamItemSlot &slot, u_int word)
{
    slot.m_rec = sys.getRec((short)word);
    slot.m_id = (short)word;
    slot.m_num = (u_short)(word >> 16);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d6078

int GamMan::writeSaveData(void *buf)
{
    u_char *out = (u_char *)buf;
    memcpy(out, "FEB_SAVE", 8);
    saveWord(out + SAVE_HDR_CHAPTER, (u_int)m_chapter);
    saveWord(out + SAVE_HDR_INU, (u_int)m_inuMode);
    int minutes = getPlayMinutes();
    saveWord(out + SAVE_HDR_18, 0x2d10);
    saveWord(out + SAVE_HDR_MINUTES, (u_int)minutes);
    saveWord(out + SAVE_HDR_EQUIP, (u_short)m_equipItem);
    saveWord(out + SAVE_HDR_FIND, (u_short)m_findTargetItem);
    utlStrNCopy((char *)out + 8, g_versionNumber, 8);
    saveWord(out + SAVE_HDR_VERSION, SAVE_VERSION);
    if ((m_flags2 & 0x20000000) != 0)
        g_sequence.m_bits[0] |= 0x8000;

    u_char *p = out + SAVE_HDR_SIZE;
    g_sequence.m_global[0x26] = m_chapter;
    g_sequence.m_global[0x31] = m_wrdNo;
    g_sequence.m_global[0x32] = m_21f80;
    p += g_sequence.saveGlobals(p);
    p += g_sequence.saveBits(p);
    p += 8;
    p += m_pcPtr->save(p);
    p += m_dogPtr->save(p);

    /* The 12 items and the stock. */
    for (int i = 0; i < 12; i++)
        saveWord(p + SAVE_ITEM + i * 4, gamItemWord(m_itemHave.m_begin[i]));
    saveWord(p + SAVE_STOCK_NUM, (u_int)m_itemStock.m_used.m_count);
    u_char *q = p + SAVE_STOCK;
    int n = 1;
    for (SeqNode<GamItemSlot> *node = m_itemStock.m_used.begin(); node != m_itemStock.m_used.end();
         node = node->m_next)
    {
        saveWord(q, gamItemWord(node->m_data));
        q += 4;
        if (n++ >= 0x14e)
            break;
    }
    p[SAVE_MARK] = 'N';
    memcpy(p + SAVE_OIDS, m_1f240, 0x5a4);

    /* The object table's reset words, ended by oid 0 (the ROM leaves the
     * last record's m_0c in the end marker). */
    q = p + SAVE_OBJRESET;
    u_short last0c = 0;
    for (int i = 0; i < m_objTable->m_num; i++)
    {
        GamObjReset rec;
        rec.m_oid = (short)m_objRec[i].m_oid;
        rec.m_flags = (u_short)m_objRec[i].m_flags;
        rec.m_0c = (u_short)m_objRec[i].m_0c;
        last0c = rec.m_0c;
        memcpy(q, &rec, sizeof(rec));
        q += sizeof(rec);
    }
    GamObjReset resetEnd = {0, 0, last0c};
    memcpy(q, &resetEnd, sizeof(resetEnd));

    /* The vfchrs of image 1 (oid >> 30), at most 0x26, ended by oid 0
     * (the ROM's end record keeps the last one's other words). */
    SaveVFchr rec;
    memset(&rec, 0, sizeof(rec));
    q = p + SAVE_VFCHR;
    int count = 0;
    for (SeqNode<FCHRstat *> *node = m_vfchrList.m_used.begin();
         node != m_vfchrList.m_used.end() && count < 0x26; node = node->m_next)
    {
        FCHRstat *chr = node->m_data;
        if (chr->m_oid >> 30 != 1)
            continue;
        count++;
        rec.m_oid = chr->m_oid;
        rec.m_kind = chr->m_kind;
        rec.m_flags = (u_short)chr->m_flags;
        chr->getParam(8, &rec.m_wrd);
        chr->getParam(0x1000, &rec.m_model);
        chr->getParam(0x43, rec.m_pos);
        memcpy(q, &rec, sizeof(rec));
        q += sizeof(rec);
    }
    rec.m_oid = 0;
    rec.m_flags = 0;
    memcpy(q, &rec, sizeof(rec));

    /* Every vfchr's oid and flags, ended by a zero record. */
    q = p + SAVE_OBJSET;
    for (SeqNode<FCHRstat *> *node = m_vfchrList.m_used.begin(); node != m_vfchrList.m_used.end();
         node = node->m_next)
    {
        GamObjSet set = {node->m_data->m_oid, node->m_data->m_flags};
        memcpy(q, &set, sizeof(set));
        q += sizeof(set);
    }
    memset(q, 0, sizeof(GamObjSet));

    for (int i = 0; i < 12; i++)
        saveWord(p + SAVE_OPTIONS + i * 4, (u_int)*gamOptionWord(this, i));
    memcpy(p + SAVE_BGM, m_sndMan.get_snd_prm(), sizeof(GamBgmRec));

    /* The item records' flag bytes (bits 0x01, 0x04, 0x10, 0x40, 0x80). */
    const GamSysRec5 *item = (const GamSysRec5 *)m_sysData.getRec(0xa00);
    for (int i = 0; i < SAVE_ITEMFLAG_NUM; i++)
        p[SAVE_ITEMFLAGS + i] = (u_char)(item[i].m_flags & 0xd5);
    return (int)(p + SAVE_ITEMFLAGS + SAVE_ITEMFLAG_NUM - 1 - out) + 0x3a;
}

// ──────────────────────────────────────────────────────────────────────
// The reader follows the image's version (header +0x28): below 7 nothing
// is read; older images have shorter sections (m_1f240 0x80 / 0x140 /
// 0x14e words, the object resets 0x600 / 0x280 bytes, 0x40 / 0x30 / 0x2a
// vfchrs). Bit 0x1000 of sequence bits word 8 makes it a New Game from
// the save: globals, items, stock, the oid list and the item flags
// cleared instead of loaded.
// ROR 0x001d68a0; callcheck: fptodp, dpcmp (the soft-float double compare of the rotation with pi)

int GamMan::readSaveData(const void *buf)
{
    const u_char *in = (const u_char *)buf;
    int version = (int)loadWord(in + SAVE_HDR_VERSION);
    if (version <= 6)
        return 0;

    m_chapter = (int)loadWord(in + SAVE_HDR_CHAPTER);
    m_inuMode = (int)loadWord(in + SAVE_HDR_INU);
    m_playMinutes = (int)loadWord(in + SAVE_HDR_MINUTES);
    m_equipItem = (short)loadWord(in + SAVE_HDR_EQUIP);
    m_flags2 |= 0x200000;
    m_findTargetItem = (short)loadWord(in + SAVE_HDR_FIND);

    const u_char *p = in + SAVE_HDR_SIZE;
    p += g_sequence.loadGlobals(p);
    p += g_sequence.loadBits(p);
    p += 8;
    m_21f80 = -1;
    m_chapter = -1;
    m_wrdNo = -1;
    memcpy(m_pcSave, p, sizeof(m_pcSave));
    p += m_pcPtr->load(p);
    memcpy(m_dogSave, p, sizeof(m_dogSave));
    p += m_dogPtr->load(p);
    m_flags2 |= 0x40;
    memcpy(m_pcStart, m_pcPtr->m_pos, sizeof(m_pcStart));
    memcpy(m_dogStart, m_dogPtr->m_pos, sizeof(m_dogStart));
    float rotY = m_pcPtr->m_rot[1];
    if ((double)rotY < -3.141592653589793 || (double)rotY > 3.141592653589793)
        rotY = 0.0f;
    m_pcStart[3] = rotY;
    m_dogStart[3] = m_dogPtr->m_rot[1];

    /* The items: a slot's m_oid and +0xc keep what they held (stack
     * garbage in the ROM). */
    clearItemHave();
    for (int i = 0; i < 12; i++)
        gamSetItemWord(m_sysData, m_itemHave.m_begin[i], loadWord(p + SAVE_ITEM + i * 4));
    const u_char *stockBase = version < 0xd ? NULL : p + SAVE_STOCK_NUM;
    int stockNum = (int)loadWord(p + SAVE_STOCK_NUM);
    const u_char *q = p + SAVE_STOCK;
    m_itemStock.clear();
    for (int i = 0; i < stockNum; i++, q += 4)
    {
        GamItemSlot item;
        memset(&item, 0, sizeof(item));
        gamSetItemWord(m_sysData, item, loadWord(q));
        m_itemStock.add(item);
    }
    if (version > 0xc)
        q = stockBase + 300 * 4;
    if (version > 0xe)
        q = stockBase + 0x14e * 4;

    /* The oid list m_1f240 and its count. */
    int oids;
    if (version >= 0x10)
    {
        oids = 0x169;
        q += 5 * 4;
    }
    else if (version == 0xf)
    {
        oids = 0x14e;
        q += 5 * 4;
    }
    else
    {
        oids = (signed char)*q;
        q += 4 * 4;
    }
    m_1f244 = oids;
    int words = version >= 0x10 ? 0x169 : version == 0xf ? 0x14e : version >= 0xb ? 0x140 : 0x80;
    memcpy(m_1f240, q, words * 4);
    q += words * 4;

    int resetBytes = version >= 0xd ? 0x3c0 : version == 0xc ? 0x280 : 0x600;
    memcpy(m_18bc0, q, resetBytes);
    q += resetBytes;

    releaseVFchrs(0);
    q += 8;
    if (version > 9)
    {
        int max = version >= 0x10 ? 0x26 : version == 0xf ? 0x2a : version >= 0xc ? 0x30 : 0x40;
        for (int i = 0; i < max; i++)
        {
            SaveVFchr rec;
            memcpy(&rec, q + i * sizeof(rec), sizeof(rec));
            if (rec.m_oid == 0)
                break;
            addVFchr(-1, rec.m_oid, rec.m_wrd, rec.m_kind, (intptr_t)rec.m_pos);
            int actor = -1;
            FCHRstat *chr = getVFchr(rec.m_oid);
            if (chr == NULL)
                continue;   // PORT: the ROM goes on with NULL (no free vfchr slot)
            static const u_int kept[] = {4, 1, 0x40, 0x80};
            for (u_int bit : kept)
            {
                if ((rec.m_flags & bit) != 0)
                    chr->m_flags |= bit;
                else
                    chr->m_flags &= ~bit;
            }
            chr->setParam(3, &actor);
            chr->setParam(0x1000, &rec.m_model);
            chr->setParam(0x43, rec.m_pos);
            if (m_sysData.testFlags(rec.m_kind, 0x200000))
                chr->m_flags |= 1;
        }
        q += max * sizeof(SaveVFchr);
    }

    memcpy(m_18bc4, q, 0xc00);
    if ((m_flags2 & 0x400) == 0)
        for (int i = 0; i < 12; i++)
            *gamOptionWord(this, i) = (int)loadWord(q + 0xc00 + i * 4);
    memcpy(&m_bgmRec, q + 0xc30, sizeof(m_bgmRec));
    q += 0xc54;

    GamSysRec5 *item = (GamSysRec5 *)m_sysData.getRec(0xa00);
    if ((g_sequence.m_bits[8] & 0x1000) == 0)
    {
        /* The ROM loads the byte signed (lb): a saved 0x80 sets every bit
         * above it too. */
        for (int i = 0; i < SAVE_ITEMFLAG_NUM; i++)
            item[i].m_flags = (item[i].m_flags & 0xffffff2a) | (u_int)(int)(signed char)q[i];
        g_sequence.m_bits[7] |= 0x80000000;
    }
    else
    {
        g_sequence.clearGlobals();
        clearItemHave();
        m_itemStock.clear();
        g_sequence.m_bits[0] |= 0x80000000;
        g_sequence.m_global[0x26] = 1;
        g_sequence.m_bits[0] &= ~0x8000u;
        memset(m_1f240, 0, 0x5a4);
        m_playMinutes = 0;
        m_flags2 &= ~0x40u;
        for (int i = 0; i < SAVE_ITEMFLAG_NUM; i++)
            item[i].m_flags &= 0xffffff2a;
    }
    q += SAVE_ITEMFLAG_NUM - 1;

    int mode = g_sequence.m_global[0x1f];
    if ((g_sequence.m_bits[0] & 0x8000) != 0 && mode != 1 && mode != 5 && mode != 7)
    {
        m_flags2 |= 0x10000000;
        g_sequence.m_bits[0] &= ~0x8000u;
        m_21ff4 = g_sequence.m_global[0x51];
        m_21ff8 = g_sequence.m_global[0x52];
    }
    return (int)(q - in) + 0x3a;
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
    gamSectaWorld().findCells(pos, hit, 0, 40.0f, 0.0f);
    if (hit[list] == NULL)
        return -1;
    setSectaList(0);
    return hit[list]->m_id;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d7c20

int GamMan::hitTestObjs(GamSecta *cell, u_int oid, ChrHitRec *out, u_int skip)
{
    (void)cell;
    SeqPool<GamObjUpdate> *pool = m_pool2246c;
    if (pool->m_used.m_count == 0)
        return 0;
    s_hitWalkCount = 0;
    for (SeqNode<GamObjUpdate> *node = pool->m_used.begin(); node != pool->m_used.end();
         node = node->m_next)
    {
        if (s_hitWalkCount++ > 0x400)
            return 0;
        u_int other = node->m_data.m_oid;
        if (other == oid || (skip != 0 && other == skip))
            continue;
        if (gamOidType(other) == 1 && g_magSystem.getBwnState() == 0xd)
            continue;
        if ((this->*m_hitFunc)(other, out) != 0)
            return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d7de4

float GamMan::getFloorY(const float *pos)
{
    GamSecta *hit[4] = {NULL, NULL, NULL, NULL};
    gamSectaWorld().findCells(pos, hit, 0, 40.0f, 0.0f);
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
    SctLayer &l = gamSectaWorld().m_list[list];
    if (l.m_head == NULL || l.m_cells == NULL)
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
    gamSectaWorld().findCells(pos, hit, 0, 40.0f, 0.0f);
    return hit[list];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d7fb8

GamSecta *GamMan::findSectaCellFrom(const float *pos, const int *cur, int list)
{
    setSectaList(list);
    SctLayer &l = gamSectaWorld().m_list[list];
    int no = l.findNextCell(*cur, pos, 40.0f);
    if (no == -1 || l.m_head == NULL || l.m_cells == NULL)
        return NULL;
    return &l.m_cells[no];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d8064

GamSecta *GamMan::findSectaCell(const float *pos, GamSecta *cur, int list)
{
    setSectaList(list);
    GamSecta *hit[4];
    memset(hit, 0, sizeof(hit));
    hit[list] = cur;
    gamSectaWorld().findCells(pos, hit, cur != NULL, 200.0f, 0.0f);
    GamSecta *cell = hit[list];
    if (cell != NULL && (cell->m_flags & 4) != 0)
        return NULL;
    return cell;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d811c

GamSecta *GamMan::getSectaCell(int no, float *pos, int list)
{
    SctLayer &l = gamSectaWorld().m_list[list];
    GamSecta *cell = NULL;
    if (l.m_head != NULL && l.m_cells != NULL)
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
    return gamSectaWorld().m_list[list].getCellY(cell->m_id, pos, 0.0f);
}

// ──────────────────────────────────────────────────────────────────────
// The ROM takes list as its seventh argument (t2).
// ROR 0x001d81c4

int GamMan::countSectaPath(int from, int to, int list)
{
    GamSectaPath path;
    return findSectaPath(list, from, to, &path);
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
    SctLayer &l = gamSectaWorld().m_list[list];
    fromCell[list] = (l.m_head != NULL && l.m_cells != NULL) ? &l.m_cells[from] : NULL;
    toCell[list] = (l.m_head != NULL && l.m_cells != NULL) ? &l.m_cells[to] : NULL;
    SctPathReq req;
    req.m_list = list;
    req.m_from = fromCell;
    req.m_to = toCell;
    req.m_path = path;
    if (l.searchPath(&req) == 0)
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
