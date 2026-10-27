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

#include <string.h>

#include "common/rosepan_stub.h"
#include "game/gamtrg.h"

#include "bed/bedmemory.h"
#include "bed/bedpad.h"
#include "game/gamman.h"
#include "game/gamstat.h"
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

// stub of ROR 0x001dee78
void GamArea::update(const float *pos, const float *rot)
{
    ROSEPAN_STUB("GamArea::update", 0x001dee78);
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
