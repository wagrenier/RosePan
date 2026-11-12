// game/gamtrg.cpp -- the area triggers GamArea (0x1dd1e8..0x1e0df0; the file
// name is in cloneTrg's allocDebug tag).
//
// GamMan's member m_area (bss 0x781300). Three node pools of 0xb0-byte
// trigger records: the listed triggers (the scripts' native 023 and
// GamMan::create_trg add them), the active ones (activateTrg copies the
// listed ones with flag 0x1000000 in, ordered by m_pri) and the up to eight
// update starts in a frame; and a heap pool of registFtb's key -> values
// tables. The pools are sequence.h's node pool, inline in the ROM wherever
// they are used.

#include <math.h>
#include <string.h>

#include "game/gamtrg.h"

#include "bed/bedmemory.h"
#include "bed/bedpad.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magscenecmd.h"
#include "mag/magsystem.h"
#include "sequence/sequence.h"
#include "system/system.h"
#include "task/tsk_chr.h"
#include "task/tsk_man.h"
#include "utl/utlmisc.h"

static const char s_file[] = "game/gamtrg.cpp";

/* stopKind / clearKind's selection by a record's id (its top two bits the
 * image): kind 2 every image but 1, kind 3 image 3 only, other kinds all. */
static bool gamTrgOfKind(int kind, u_int id)
{
    if (kind == 2)
        return id >> 30 != 1;
    if (kind == 3)
        return id >> 30 == 3;
    return true;
}

/* The PC's oid (GamMan +0x22090's), 0 without one (inline in the ROM). */
static u_int gamTrgPcOid()
{
    return g_gamMan.m_pcPtr != NULL ? g_gamMan.m_pcPtr->m_oid : 0;
}

/* The trigger's rect holds (pos x, pos z). */
static bool gamTrgHolds(const GamAreaTrg &trg, const float *pos)
{
    return trg.m_rect[0] <= pos[0] && pos[0] <= trg.m_rect[2] &&
           trg.m_rect[1] <= pos[2] && pos[2] <= trg.m_rect[3];
}

// ──────────────────────────────────────────────────────────────────────
// The three inline pools, every node free. The ROM leaves m_ftbPool and
// m_18b5c to init / update.
// ROR 0x001dd1e8

GamArea::GamArea()
{
    m_trgs.build(m_trgs.m_storage, 0x100);
    m_active.build(m_active.m_storage, 0x100);
    m_fired.build(m_fired.m_storage, 8);
    m_cursor = NULL;
    // PORT: zero instead of what the bss held.
    m_ftbPool = NULL;
    m_18b5c = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Nothing of its own (the ftb pool is never freed).
// ROR 0x001dd3ec // callcheck: operator_delete (the deleting destructor's flag; the host's delete does it)

GamArea::~GamArea()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dd414 // callcheck: operator_new, operator_new[] (new SeqPool, new SeqNode[])

void GamArea::init()
{
    SeqPool<GamAreaFtb> *pool = new SeqPool<GamAreaFtb>;
    pool->build(new SeqNode<GamAreaFtb>[0x40], 0x40);
    m_ftbPool = pool;
    pool->clear();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dd558

GamAreaTrg *GamArea::cloneTrg(const GamAreaTrg *trg)
{
    GamAreaTrg *copy = (GamAreaTrg *)g_utlMemory.allocDebug(sizeof(GamAreaTrg), s_file, 0x62, 0, 1);
    *copy = *trg;
    return copy;
}

// ──────────────────────────────────────────────────────────────────────
// The free list has no empty check (the ROM pops a 257th trigger off the
// sentinel).
// ROR 0x001dd5e8

void GamArea::addTrg(GamAreaTrg *trg, int newId, int image)
{
    if (trg->m_70 != 0)
        trg->m_flags |= 0x400000;
    if (newId != 0)
        trg->m_id = g_gamMan.makeOid(0, 0xd, image);

    for (SeqNode<GamAreaTrg> *it = m_trgs.m_used.begin(); it != m_trgs.m_used.end(); it = it->m_next) {
        if (it->m_data.m_id == trg->m_id)
            return;
    }
    m_trgs.add(*trg);
    if ((trg->m_flags & 0x1000000) != 0)
        activateTrg(*trg);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dd7bc

void GamArea::stopKind(int kind)
{
    SeqNode<GamAreaTrg> *it = m_trgs.m_used.begin();
    while (it != m_trgs.m_used.end()) {
        SeqNode<GamAreaTrg> *next = it->m_next;
        if (gamTrgOfKind(kind, it->m_data.m_id))
            m_trgs.remove(it);
        it = next;
    }

    SeqPool<GamAreaFtb> *pool = m_ftbPool;
    SeqNode<GamAreaFtb> *ftb = pool->m_used.begin();
    while (ftb != pool->m_used.end()) {
        SeqNode<GamAreaFtb> *next = ftb->m_next;
        if (gamTrgOfKind(kind, (u_int)ftb->m_data.m_key))
            pool->remove(ftb);
        ftb = next;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The ROM moves the trigger list's nodes one by one; the free list ends
// in the same order as the splice.
// ROR 0x001ddaa8

void GamArea::clear()
{
    m_active.clear();
    m_ftbPool->clear();
    m_trgs.clear();
}

// ──────────────────────────────────────────────────────────────────────
// Node by node in the ROM, the same order as the splice.
// ROR 0x001ddc80

void GamArea::stopTrgs()
{
    m_active.clear();
    resetCursor();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dddac

void GamArea::resetCursor()
{
    m_18b5c = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dddbc

void GamArea::clearKind(int kind)
{
    SeqNode<GamAreaTrg> *it = m_active.m_used.begin();
    while (it != m_active.m_used.end()) {
        SeqNode<GamAreaTrg> *next = it->m_next;
        if (gamTrgOfKind(kind, it->m_data.m_id))
            m_active.remove(it);
        it = next;
    }
    resetCursor();
}

// ──────────────────────────────────────────────────────────────────────
// The ROM compares the kind as a u_short with the sign-extended argument.
// ROR 0x001ddf48 // callcheck: operator_new[], operator_delete[] (new SeqNode[8])

u_int GamArea::findFchrOid(const float *pos, short kind)
{
    SeqPool<FCHRstat *> found;
    SeqNode<FCHRstat *> *nodes = new SeqNode<FCHRstat *>[8];
    found.build(nodes, 8);
    g_gamMan.collectFchrsNear(pos, 50.0f, &found, 5, 0x8000, 0x4000001);

    u_int oid = 0;
    if (found.m_used.m_count != 0) {
        int n = 0;
        for (SeqNode<FCHRstat *> *it = found.m_used.begin(); it != found.m_used.end(); it = it->m_next) {
            const FCHRstat *chr = it->m_data;
            u_int chrKind = (u_short)chr->m_kind;
            if (chrKind == 0xffff)
                continue;
            if (chrKind == (u_int)(int)kind) {
                oid = chr->m_oid;
                break;
            }
            if (n++ > 7)
                break;
        }
    }
    delete[] nodes;
    return oid;
}

// ──────────────────────────────────────────────────────────────────────
// trg comes by value (the callers' stack copy). An object trigger whose
// object is gone is dropped.
// ROR 0x001de104

void GamArea::activateTrg(GamAreaTrg trg)
{
    if ((trg.m_flags & 4) != 0)
        trg.m_time = g_taskMan.m_timer.m_updates + trg.m_delay;
    if ((trg.m_flags & 0x82c00) != 0) {
        if (g_gamMan.isObj(trg.m_oid) == 0)
            return;
        trg.m_actor = g_gamMan.getActorNo(trg.m_oid);
        trg.m_actor2 = g_gamMan.getActorNo(trg.m_oid2);
    }
    if ((trg.m_flags & 0x5000) != 0) {
        if (g_gamMan.isObj(trg.m_oid) == 0)
            trg.m_actor = -1;
        else
            trg.m_actor = g_gamMan.getActorNo(trg.m_oid);
    }
    if ((trg.m_flags & 0x80002) != 0)
        trg.m_flags |= 0x800000;
    if ((trg.m_flags & 0x20) != 0) {
        SeqPool<GamAreaFtb> *pool = m_ftbPool;
        SeqNode<GamAreaFtb> *ftb = pool->m_used.begin();
        while (ftb != pool->m_used.end() && ftb->m_data.m_key != trg.m_ftbKey)
            ftb = ftb->m_next;
        // PORT: the node's index + 1 for the ROM's pointer to its data.
        trg.m_ftb = ftb == pool->m_used.end() ? 0 : (u_int)(ftb - pool->m_node) + 1;
    }

    SeqList<GamAreaTrg> &used = m_active.m_used;
    SeqNode<GamAreaTrg> *at = used.begin();
    while (at != used.end() && at->m_data.m_pri <= trg.m_pri)
        at = at->m_next;

    SeqNode<GamAreaTrg> *node = m_active.m_free.popBack();
    node->m_data = trg;
    if (at == used.end()) {
        used.pushBack(node);
    } else {
        node->m_prev = at->m_prev;
        at->m_prev->m_next = node;
        node->m_next = at;
        at->m_prev = node;
        used.m_count++;
    }
    m_cursor = used.m_head.m_prev;
}

GamAreaFtb *GamArea::ftbOf(const GamAreaTrg &trg)
{
    if (trg.m_ftb == 0 || m_ftbPool == NULL)
        return NULL;
    return &m_ftbPool->m_node[trg.m_ftb - 1].m_data;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001de4e4

void GamArea::startTrgs()
{
    stopTrgs();
    if (m_trgs.m_used.m_count == 0)
        return;
    for (SeqNode<GamAreaTrg> *it = m_trgs.m_used.begin(); it != m_trgs.m_used.end(); it = it->m_next) {
        if ((it->m_data.m_flags & 0x1000000) != 0)
            activateTrg(it->m_data);
    }
}

// ──────────────────────────────────────────────────────────────────────
// A key already holding value, or a free slot of [1..8] taken, ends it; a
// key whose slots are full goes on to the next entry with that key, and
// past the last a new entry is made ([0] is never compared, so value
// lands there again). The ROM first sets bit 31 when bit 31 is set (no
// change).
// ROR 0x001de6fc // callcheck: memset (the 0x24-byte record, written field by field)

void GamArea::registFtb(int key, u_int value)
{
    SeqPool<GamAreaFtb> *pool = m_ftbPool;
    for (SeqNode<GamAreaFtb> *it = pool->m_used.begin(); it != pool->m_used.end(); it = it->m_next) {
        GamAreaFtb &ftb = it->m_data;
        if (ftb.m_key != key)
            continue;
        for (int i = 1; i <= 8; i++) {
            if (ftb.m_value[i] == value)
                return;
            if (ftb.m_value[i] == 0) {
                ftb.m_value[i] = value;
                return;
            }
        }
    }

    /* The ROM copies a 0x24-byte record: m_value[8] keeps what the node
     * held. */
    SeqNode<GamAreaFtb> *node = pool->m_free.popBack();
    node->m_data.m_key = key;
    node->m_data.m_value[0] = value;
    for (int i = 1; i < 8; i++)
        node->m_data.m_value[i] = 0;
    pool->m_used.pushBack(node);
}

// ──────────────────────────────────────────────────────────────────────
// Each edge pts[i] -> pts[i + 1] (the list is not closed: the caller
// repeats the first point). An edge on the line z = pos z counts when one
// of its ends lies left of pos.
// ROR 0x001de8ac

int GamArea::isInPolygon(const float *pos, const float *const *pts)
{
    int in = 0;
    if (pts[1] == NULL)
        return 0;

    const float *a = pts[0];
    for (;;) {
        const float *b = pts[1];
        float az = a[1];
        float bz = b[1];
        if (az == bz && az == pos[2]) {
            if (a[0] < b[0] && a[0] < pos[0])
                in ^= 1;
            else if (b[0] < a[0] && b[0] < pos[0])
                in ^= 1;
        } else if (az < bz && az <= pos[2] && pos[2] < bz) {
            if ((b[0] - a[0]) * (pos[2] - a[1]) - (b[1] - a[1]) * (pos[0] - a[0]) > 0.0f)
                in ^= 1;
        } else if (bz < az && bz < pos[2] && pos[2] <= az) {
            if ((a[0] - b[0]) * (pos[2] - b[1]) - (a[1] - b[1]) * (pos[0] - b[0]) > 0.0f)
                in ^= 1;
        }
        if (pts[2] == NULL)
            return in;
        a = pts[1];
        pts++;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The ROM also passes 32769.0 in f13 (chrMtnMove2 does not read it).
// ROR 0x001dea40

void GamArea::startPcAction()
{
    chrMtnMove2(1.0f / 60.0f, gamTrgPcOid(), g_gamMan.m_pcMotion, -1, 0, NULL);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001deaac

void GamArea::checkPc()
{
    float pos[4];
    g_gamMan.getObjPos(gamTrgPcOid(), pos);
    if (m_active.m_used.m_count == 0)
        return;
    for (SeqNode<GamAreaTrg> *it = m_active.m_used.begin(); it != m_active.m_used.end(); it = it->m_next) {
        const GamAreaTrg &trg = it->m_data;
        if ((trg.m_flags & 0x4000001) != 0x4000001 || !gamTrgHolds(trg, pos))
            continue;
        if (trg.m_thread != 0 && trg.m_entry != 0) {
            g_sequence.execThread(trg.m_thread, trg.m_entry, (int)trg.m_oid, 2, 0, (int)trg.m_id, "trg");
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001dec14

void GamArea::flagPcTrg()
{
    float pos[4];
    g_gamMan.getObjPos(gamTrgPcOid(), pos);
    if (m_active.m_used.m_count == 0)
        return;
    for (SeqNode<GamAreaTrg> *it = m_active.m_used.begin(); it != m_active.m_used.end(); it = it->m_next) {
        const GamAreaTrg &trg = it->m_data;
        if ((trg.m_flags & 1) == 0 || (trg.m_70 & 2) == 0 || !gamTrgHolds(trg, pos))
            continue;
        if (trg.m_thread != 0 && trg.m_entry != 0) {
            ctl_trg_flg(2, (int)trg.m_id, 0x10000000);
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// The pad is MagSystem's first (+0x97a40, bss 0x781280): CROSS (0x4000)
// the only button pressed this frame, and none of SQUARE 0x8000, TRIANGLE
// 0x1000, CIRCLE 0x2000, START 8 held.
// ROR 0x001ded70

int GamArea::canCheckPc(void *unused, u_int *flags)
{
    (void)unused;
    if ((g_gamMan.m_flags & 4) == 0)
        return 0;
    if ((u_int)(g_magSystem.getPCStateClass() - 5) >= 3)
        return 0;
    if (g_taskMan.getTask(g_gamMan.m_itemUseTaskId) != 0)
        return 0;
    const BedPad *pad = g_magSystem.m_pad_ref[0].m_pad;
    if (pad->m_trig != 0x4000)
        return 0;
    u_int hold = pad->m_hold;
    if ((hold & 0x8000) != 0 || (hold & 0x1000) != 0 || (hold & 0x2000) != 0 || (hold & 8) != 0)
        return 0;
    *flags |= 0x10;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// update's helpers (inline in the ROM).

/* The angle of the last flag 8 / 0x2000 / 0x4000 trigger that held; a flag
 * 1 trigger that fires leaves it in m_global[8]. */
static float s_trgAngle;        // ROR 0x002d8f64 (sdata, 0.0)

/* GamMan m_flags2 0x8000000 holds most tests off. */
static bool gamTrgFrozen()
{
    return (g_gamMan.m_flags2 & 0x8000000) != 0;
}

/* |d| after one wrap into -pi..pi (in doubles, as the ROM does). */
static float gamTrgAngleOff(float d)
{
    const double pi = 3.141592653589793;
    if ((double)d < -pi)
        d = (float)((double)d + 2.0 * pi);
    if ((double)d > pi)
        d = (float)((double)d - 2.0 * pi);
    return d < 0.0f ? -d : d;
}

/* The trigger's actor number, looked up (and kept) while it is -1; false
 * when oid is no object or has no actor. */
static bool gamTrgActor(int &actor, u_int oid)
{
    if (actor != -1)
        return true;
    if (g_gamMan.isObj(oid) == 0)
        return false;
    actor = g_gamMan.getActorNo(oid);
    return actor != -1;
}

static const float *gamTrgActorPos(int actor)
{
    return g_magSystem.m_scene.m_actors[actor].m_trans;
}

/* q within r of p on xz, their heights less than 150 apart. */
static bool gamTrgNear(const float *p, const float *q, float r)
{
    float dx = p[0] < q[0] ? q[0] - p[0] : p[0] - q[0];
    float dy = q[1] - p[1];
    if (dy < 0.0f ? dy <= -150.0f : 150.0f <= dy)
        return false;
    if (dx > r)
        return false;
    float dz = p[2] < q[2] ? q[2] - p[2] : p[2] - q[2];
    return dz <= r && sqrtf(dx * dx + dz * dz) < r;
}

/* Bit n of the script flags (g_sequence.m_bits). */
static bool gamTrgBit(u_int n)
{
    return (g_sequence.m_bits[n >> 5] & (1u << (n & 0x1f))) != 0;
}

/* Flag 0x20: every value of the ftb entry (up to eight, to the first 0)
 * holds: a positive one names a script flag that must be set, a negative
 * one (its low 16 bits) one that must be clear. */
static bool gamTrgFtbHolds(const GamAreaFtb *ftb)
{
    for (int i = 0; i < 8 && ftb->m_value[i] != 0; i++) {
        u_int v = ftb->m_value[i];
        if ((int)v < 0) {
            if (gamTrgBit(v & 0xffff))
                return false;
        } else if (!gamTrgBit(v)) {
            return false;
        }
    }
    return true;
}

/* Flag 0x8000: the script global m_cmpGlobal against m_cmpValue (ops 2..5
 * unsigned; 6 / 7 the signed short range lo = the low half, hi = the
 * high half). Other ops hold. */
static bool gamTrgCompare(const GamAreaTrg &trg)
{
    int g = g_sequence.m_global[trg.m_cmpGlobal];
    u_int ug = (u_int)g;
    u_int v = (u_int)trg.m_cmpValue;
    int lo = (short)(v & 0xffff);
    int hi = (short)(v >> 16);
    switch (trg.m_cmpOp) {
    case 0:
        return v == ug;
    case 1:
        return v != ug;
    case 2:
        return ug > v;
    case 3:
        return v > ug;
    case 4:
        return ug >= v;
    case 5:
        return v >= ug;
    case 6:
        return lo <= g && g <= hi;
    case 7:
        return g < lo || hi < g;
    }
    return true;
}

// ──────────────────────────────────────────────────────────────────────
// Each active trigger collects the bits of its conditions that hold in pass
// (from 0x3000000); one whose low 0xc31fffff flags equal pass (and, with
// flag 0x400000, whose Brown conditions all held) is copied to m_fired.
// Flags 1 / 2 (enter / leave the rect) and 0x100 / 0x40000 / 0x800 /
// 0x80000 add 0x80000000 while bit 0x10000000 (and 0x800000 for leaving)
// is clear: the edge bits ctl_trg_flg sets when the trigger fires, cleared
// again on the opposite side. A failed test ends the trigger's turn.
// The fired ones then start their "trg" threads (m_fired in order, not
// while a menu is open), the first that does not keep the game running
// (no 0x8000000) setting 0x20000000, resuming 0x8000 and ending the walk.
// The PC's rect entries counted in inside keep a second 0x4000000 trigger
// from starting while m_18b5c (the last start's count) is set.
// ROR 0x001dee78 // callcheck: dpadd, dpcmp, dpsub, dptofp, fptodp, sqrtf, isObj, getActorNo, operator_new (the soft-float doubles, C doubles here; new TskFrameFunc; the rest in the helpers above)

void GamArea::update(const float *pos, const float *rot)
{
    int inside = 0;
    /* The ROM keeps the Brown bits across the triggers (never reset). */
    u_int bwnPass = 0;
    u_int bwn = 0;

    if (m_fired.m_used.m_count != 0)
        m_fired.clear();

    SeqList<GamAreaTrg> &used = m_active.m_used;
    if (used.m_count != 0) {
        SeqNode<GamAreaTrg> *next;
        for (SeqNode<GamAreaTrg> *it = used.begin(); it != used.end(); it = next) {
            /* PORT: the ROM goes on from the node's m_next after a 0x40000000
             * trigger left the list (the free list's sentinel: it would walk
             * the free list for ever); the host from the saved next, which
             * the ROM also keeps (sp+0xa0) but never reads. */
            next = it->m_next;
            GamAreaTrg &trg = it->m_data;
            u_int f = trg.m_flags;
            u_int pass = 0x3000000;

            if ((f & 3) != 0) {
                if (gamTrgFrozen())
                    continue;
                if (!gamTrgHolds(trg, pos)) {
                    if ((f & 2) != 0) {
                        pass = 0x3000002;
                        if ((f & 0x80800000) == 0x80000000)
                            pass = 0x83000002;
                    }
                    trg.m_flags &= ~0x10000000u;
                } else {
                    if ((f & 1) != 0) {
                        if ((f & 0x4010000) == 0x4000000)
                            inside++;
                        pass = 0x3000001;
                        if ((f & 0x90000000) == 0x80000000)
                            pass = 0x83000001;
                    }
                    trg.m_flags &= ~0x800000u;
                }
                if (pass == 0x3000000)
                    continue;
            }

            if ((f & 0x100) != 0) {
                if (gamTrgFrozen())
                    continue;
                if (gamTrgHolds(trg, pos) && trg.m_y - 200.0f <= pos[1] && pos[1] <= trg.m_y + 200.0f) {
                    pass |= 0x100;
                    if ((f & 0x90000000) == 0x80000000)
                        pass |= 0x80000000;
                } else if ((f & 0x90000000) == 0x90000000) {
                    trg.m_flags &= ~0x10000000u;
                }
            }

            if ((f & 0x1000) != 0) {
                if (gamTrgFrozen() || !gamTrgActor(trg.m_actor, trg.m_oid))
                    continue;
                if (!gamTrgNear(pos, gamTrgActorPos(trg.m_actor), trg.m_radius))
                    continue;
                pass |= 0x1000;
            }

            if ((f & 0x400) != 0) {
                if (gamTrgFrozen() || !gamTrgActor(trg.m_actor, trg.m_oid) ||
                    !gamTrgActor(trg.m_actor2, trg.m_oid2))
                    continue;
                if (!gamTrgNear(gamTrgActorPos(trg.m_actor), gamTrgActorPos(trg.m_actor2), trg.m_radius))
                    continue;
                pass |= 0x400;
            }

            if ((f & 0x80800) != 0) {
                if (!gamTrgActor(trg.m_actor, trg.m_oid) || !gamTrgActor(trg.m_actor2, trg.m_oid2))
                    continue;
                if (gamTrgNear(gamTrgActorPos(trg.m_actor), gamTrgActorPos(trg.m_actor2), trg.m_radius)) {
                    if ((f & 0x800) != 0) {
                        pass |= 0x800;
                        if ((f & 0x90000000) == 0x80000000)
                            pass |= 0x80000000;
                    }
                    trg.m_flags &= ~0x800000u;
                } else {
                    if ((f & 0x80000) != 0) {
                        pass |= 0x80000;
                        if ((f & 0x80800000) == 0x80000000)
                            pass |= 0x80000000;
                    }
                    trg.m_flags &= ~0x10000000u;
                }
            }

            if ((f & 8) != 0) {
                if (!(gamTrgAngleOff(trg.m_angle - rot[1]) < trg.m_angleTol))
                    continue;
                pass |= 8;
                s_trgAngle = trg.m_angle;
            }

            if ((f & 0x2000) != 0) {
                if (gamTrgFrozen() || !gamTrgActor(trg.m_actor, trg.m_oid) ||
                    !gamTrgActor(trg.m_actor2, trg.m_oid2))
                    continue;
                const float *a = gamTrgActorPos(trg.m_actor);
                const float *b = gamTrgActorPos(trg.m_actor2);
                float ang = utlAtan2(b[0] - a[0], b[2] - a[2]);
                if (!(gamTrgAngleOff(ang - rot[1]) < trg.m_angleTol))
                    continue;
                pass |= 0x2000;
                s_trgAngle = ang;
            }

            if ((f & 0x4000) != 0) {
                if (gamTrgFrozen() || !gamTrgActor(trg.m_actor, trg.m_oid))
                    continue;
                const float *a = gamTrgActorPos(trg.m_actor);
                float ang = utlAtan2(a[0] - pos[0], a[2] - pos[2]);
                if (!(gamTrgAngleOff(ang - rot[1]) < trg.m_angleTol))
                    continue;
                pass |= 0x4000;
                s_trgAngle = ang;
            }

            if ((f & 0x10) != 0)
                canCheckPc(it, &pass);

            if ((f & 4) != 0) {
                u_int now = (u_int)g_taskMan.m_timer.m_updates;
                if (now < (u_int)trg.m_time)
                    continue;
                trg.m_time = trg.m_delay + (int)now;
                pass |= 4;
            }

            if ((f & 0x20) != 0) {
                const GamAreaFtb *ftb = ftbOf(trg);
                if (ftb == NULL || !gamTrgFtbHolds(ftb))
                    continue;
                pass |= 0x20;
            }

            if ((f & 0x8000) != 0) {
                if (!gamTrgCompare(trg))
                    continue;
                pass |= 0x8000;
            }

            if ((f & 0x10000) != 0) {
                if (!gamTrgBit(trg.m_ftb))
                    continue;
                pass |= 0x10000;
            }

            if ((f & 0x40) != 0) {
                if ((g_gamMan.m_flags & 0x2000) == 0 ||
                    (trg.m_kind != 0xffff && (u_int)g_sequence.m_global[0x11] != trg.m_kind))
                    continue;
                pass |= 0x40;
            }

            if ((f & 0x80) != 0) {
                if ((g_gamMan.m_flags2 & 0x80) == 0 ||
                    (trg.m_kind != 0xffff && (u_int)g_sequence.m_global[0x29] != trg.m_kind))
                    continue;
                pass |= 0x80;
            }

            if ((f & 0x200) != 0) {
                u_int oid = findFchrOid(pos, (short)trg.m_kind);
                if (oid == 0)
                    continue;
                g_gamMan.releaseChr(oid);
                pass |= 0x200;
            }

            if ((f & 0x40000) != 0) {
                if (gamTrgFrozen())
                    continue;
                /* The rect's corners (x0, z0), (x1, z1), then m_pt2, m_pt3,
                 * closed on (x0, z0). */
                const float *pts[6] = { &trg.m_rect[0], &trg.m_rect[2], trg.m_pt2, trg.m_pt3, &trg.m_rect[0], NULL };
                if (isInPolygon(pos, pts) == 0) {
                    if ((f & 0x90000000) == 0x90000000)
                        trg.m_flags &= ~0x10000000u;
                    continue;
                }
                if ((f & 0x90000000) == 0x80000000)
                    pass |= 0x80000000;
                pass |= 0x40000;
            }

            if ((f & 0x400000) != 0) {
                bwn = trg.m_70;
                if ((bwn & 0x40000) != 0) {
                    u_int found = (u_int)magBwnGetFoundOid() & 0xffff;
                    if (found == 0 || (u_int)trg.m_bwnA != found)
                        continue;
                    bwnPass |= 0x40000;
                }
                if ((bwn & 0x80000) != 0)
                    continue;
                if ((bwn & 0x100000) != 0) {
                    if (trg.m_bwnA != magBwnReturn0b())
                        continue;
                    bwnPass |= 0x100000;
                }
                if ((bwn & 0x200000) != 0) {
                    int b = magBwnReturn0b();
                    int c = magBwnReturn0c();
                    if (trg.m_bwnA != b || trg.m_bwnB != c)
                        continue;
                    bwnPass |= 0x200000;
                }
                if ((bwn & 4) != 0) {
                    if (gamTrgFrozen() || !(trg.m_y - 200.0f <= pos[1] && pos[1] <= trg.m_y + 200.0f))
                        continue;
                    bwnPass |= 4;
                }
            }

            if ((f & 0xc31fffff) != pass)
                continue;
            if ((f & 0x400000) != 0 && (bwn & ~2u) != bwnPass)
                continue;

            if ((f & 0x40000000) != 0) {
                m_active.remove(it);
                trg.m_flags &= ~0x1000000u;
            }
            if ((f & 0x4000000) != 0 && (g_gamMan.m_flags & 0x10000) != 0)
                continue;
            if ((f & 1) != 0)
                memcpy(&g_sequence.m_global[8], &s_trgAngle, sizeof(float));
            m_cursor = it;
            m_fired.add(trg);
        }
    }

    if (m_fired.m_used.m_count == 0)
        return;
    bool paused = false;
    for (SeqNode<GamAreaTrg> *it = m_fired.m_used.begin(); it != m_fired.m_used.end() && !paused; it = it->m_next) {
        const GamAreaTrg &trg = it->m_data;
        u_int f = trg.m_flags;
        if ((f & 0x4000000) != 0 && inside >= 2 && m_18b5c != 0)
            continue;
        if ((f & 0x8000000) == 0 && (f & 0x4000000) == 0 && (g_gamMan.m_flags & 0x8000) != 0)
            continue;
        if (g_gamMan.isMenuOpen() != 0 || trg.m_entry == 0)
            continue;

        if ((f & 0x40901) != 0)
            ctl_trg_flg(2, (int)trg.m_id, 0x10000000);
        if ((f & 0x80002) != 0)
            ctl_trg_flg(2, (int)trg.m_id, 0x800000);
        if ((f & 0x8000000) == 0) {
            ctl_trg_flg(2, (int)trg.m_id, 0x20000000);
            paused = true;
            g_gamMan.resume(0x8000);
        }
        if ((f & 0x40) != 0)
            g_gamMan.m_flags2 &= ~0x80000u;
        if ((f & 0x280) != 0)
            g_gamMan.freezePcMotion();
        if ((f & 0x4000000) == 0) {
            g_gamMan.m_flags2 |= 0x400800;
            int start = g_taskMan.m_timer.m_updates + 0x10;
            TskFrameFunc *task = new TskFrameFunc(0x16, 0, 0);
            g_taskMan.addTask(1, start, 1, 0, task, 0, NULL, 1);
        }
        g_sequence.execThread(trg.m_thread, trg.m_entry, (int)trg.m_oid, 2, 0, (int)trg.m_id, "trg");
        m_18b5c = inside;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Walks m_count nodes of the active list. id 0 applies ops 1..3 to every
// trigger and answers 1; op 4 / 5 answer for the first match.
// ROR 0x001e0c2c

int GamArea::ctl_trg_flg(int op, int id, u_int bits)
{
    int n = m_active.m_used.m_count;
    if (n == 0)
        return 0;

    SeqNode<GamAreaTrg> *it = m_active.m_used.begin();
    do {
        GamAreaTrg &trg = it->m_data;
        if ((int)trg.m_id == id || id == 0) {
            switch (op) {
            case 1:
                trg.m_flags = bits;
                break;
            case 2:
                trg.m_flags |= bits;
                break;
            case 3:
                trg.m_flags &= ~bits;
                break;
            case 4:
                return (trg.m_flags & bits) != 0;
            case 5:
                if ((trg.m_flags & 0x20000000) == 0)
                    return 0;
                g_gamMan.pause(0x8000);
                trg.m_flags &= ~0x20000000u;
                return 1;
            }
            if (id != 0)
                return 1;
        }
        n--;
        it = it->m_next;
    } while (n != 0);
    return id == 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e0d70

int GamArea::hasId(u_int id)
{
    if (m_trgs.m_used.m_count == 0)
        return 0;
    for (SeqNode<GamAreaTrg> *it = m_trgs.m_used.begin(); it != m_trgs.m_used.end(); it = it->m_next) {
        if (it->m_data.m_id == id)
            return 1;
    }
    return 0;
}
