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
// checkMenuReq with restoreAfterMenu / endPcAction. Still stubs
// (lib/unassigned_stubs.cpp): writeSaveData / readSaveData.

#include <math.h>
#include <string.h>

#include "game/gamman.h"

#include "lib/unassigned.h"
#include "mag/magsystem.h"
#include "task/tsk_man.h"

/* The scene's secta lists (bss 0x7811ac). */
static inline Obj17c0f0 &gamSectaWorld() { return g_magSystem.m_scene.m_8542c; }

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
            if (flag[0] != 0)
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
