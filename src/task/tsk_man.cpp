/* ==========================================================================
 *  task/tsk_man.cpp
 *
 *  CmnTaskMan, the per-frame task runner, and the TskTest / TskLogger
 *  tasks (pinned 0x1e7094..0x1e7f28); see tsk_man.h.
 * ======================================================================== */
#include "task/tsk_man.h"

#include <string.h>

#include "game/gamman.h"

// ──────────────────────────────────────────────────────────────────────
// callcheck: SysTimer (m_timer's constructor, run by the host before the body)
// ROR 0x001e7094

CmnTaskMan::CmnTaskMan()
{
    m_list2 = NULL;
    m_live[0] = -1;
    m_nextId = 2;
    m_killEnd = m_killQueue;
    m_list = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: operator_delete (GCC 2's delete flag; the host's delete frees)
// callcheck: ~SysTimer (m_timer's destructor, run by the host after the body)
// ROR 0x001e70dc

CmnTaskMan::~CmnTaskMan()
{
    CmnTaskNode *node;
    CmnTaskNode *next;

    for (node = m_list2; node != NULL; node = next)
    {
        next = node->m_next;
        freeNode(node);
    }
    for (node = m_list; node != NULL; node = next)
    {
        next = node->m_next;
        freeNode(node);
    }
}

// ──────────────────────────────────────────────────────────────────────
// After every node due no later than this one.
// ROR 0x001e7184

void CmnTaskMan::insertNode(CmnTaskNode *node)
{
    CmnTaskNode *prev;
    CmnTaskNode *next;

    prev = m_list;
    if (prev == NULL)
    {
        node->m_next = NULL;
        m_list = node;
        return;
    }
    if (node->m_nextFrame < prev->m_nextFrame)
    {
        node->m_next = prev;
        m_list = node;
        return;
    }
    for (;;)
    {
        next = prev->m_next;
        if (next == NULL)
        {
            node->m_next = NULL;
            prev->m_next = node;
            return;
        }
        if (node->m_nextFrame < next->m_nextFrame)
            break;
        prev = next;
    }
    node->m_next = next;
    prev->m_next = node;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: operator_new (new CmnTaskNode)
// ROR 0x001e71fc

int CmnTaskMan::addTask(int group, int start, int period, int frames, CmnITask *task, int arg,
                        int *id, int flags)
{
    CmnTaskNode *node;

    (void)group;
    if (start == -1)
        start = m_timer.m_updates;
    node = new CmnTaskNode;
    if (node == NULL)
        return 0;

    node->m_task = task;
    node->m_state = 0;
    node->m_next = m_list2;
    node->m_flags = flags;
    node->m_id = m_nextId++;
    if (id != NULL)
    {
        node->m_idOut = id;
        *id = node->m_id;
    }
    addLiveId(node->m_id);
    node->m_arg = arg;
    node->m_kind = task->get_task_type();
    node->m_period = period;
    node->m_start = start;
    if (frames == 0)
        node->m_end = 0;
    else
        node->m_end = start + frames - 1;
    node->m_nextFrame = start;
    task->init();
    insertNode(node);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Only kinds 0x10, 5, 0x13 and 0x14 hear about it, and only on a change;
// the caller stores the state.
// ROR 0x001e7340

void CmnTaskMan::setNodePause(CmnTaskNode *node, int on)
{
    int kind;

    kind = node->m_task->get_task_type();
    if (kind != 0x10 && kind != 5 && kind != 0x13 && kind != 0x14)
        return;
    if (node->m_state == on)
        return;
    if (on == 1)
        node->m_task->pause(1);
    else if (on == 0)
        node->m_task->pause(0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e7414

void CmnTaskMan::pauseTask(int id, int on)
{
    CmnTaskNode *node;

    if (id == -1)
        return;
    for (node = m_list; node != NULL; node = node->m_next)
    {
        if (node->m_id == id)
        {
            setNodePause(node, on);
            node->m_state = on;
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e747c

void CmnTaskMan::pauseKinds(const int *kinds, int on)
{
    CmnTaskNode *node;
    const int   *k;

    for (node = m_list; node != NULL; node = node->m_next)
    {
        if (kinds[0] == -1)
            continue;
        for (k = kinds; *k != -1; k++)
        {
            if (node->m_task->get_task_type() == *k && node->m_state != 2)
            {
                setNodePause(node, on);
                node->m_state = on;
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// The ROM asks every node's task for its kind before it tests the task
// pointer; a terminated node (task NULL) is always dead, so the kind
// does not matter there.
// ROR 0x001e7544

void CmnTaskMan::pauseOthers(int on)
{
    CmnTaskNode *node;
    int          kind;

    for (node = m_list; node != NULL; node = node->m_next)
    {
        if (node->m_task != NULL)
        {
            kind = node->m_task->get_task_type();
            if (kind == 1 || kind == 0x12 || kind == 2 || kind == 7 || kind == 0xf
                || kind == 0x17 || kind == 0x11 || kind == 5)
                continue;
        }
        if (node->m_state != 2)
        {
            setNodePause(node, on);
            node->m_state = on;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// No term here: exec's sweep frees (and terminates) the dead nodes.
// ROR 0x001e7618

void CmnTaskMan::killStageTasks()
{
    CmnTaskNode *node;
    int          kind;

    for (node = m_list; node != NULL; node = node->m_next)
    {
        if (node->m_task != NULL)
        {
            kind = node->m_task->get_task_type();
            if ((u_int)(kind - 1) < 3 || kind == 0x11)
                continue;
        }
        node->m_state = 2;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e7698

void CmnTaskMan::untrackNode(CmnTaskNode *node)
{
    if (node != NULL)
        removeLiveId(node->m_id);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e76b8

int CmnTaskMan::killListed(int id)
{
    CmnTaskNode *node;

    for (node = m_list; node != NULL; node = node->m_next)
    {
        if (node->m_id == id && node->m_state != 2)
        {
            node->m_state = 2;
            untrackNode(node);
            return 1;
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// No bound on the queue (128 words).
// ROR 0x001e7724

void CmnTaskMan::killTask(int id)
{
    int *p;

    if (id == -1 || killListed(id))
        return;
    for (p = m_killQueue; p != m_killEnd; p++)
    {
        if (*p == id)
            return;
    }
    *m_killEnd = id;
    m_killEnd++;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e77b4

void CmnTaskMan::killKind(int kind)
{
    CmnTaskNode *node;

    for (node = m_list; node != NULL; node = node->m_next)
    {
        if (node->m_state == 2 || node->m_task == NULL)
            continue;
        if (node->m_task->get_task_type() == kind)
        {
            termNode(node);
            node->m_state = 2;
            untrackNode(node);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// A match shortens the queue by its last word, not by the matched one
// (as the ROM does): with several ids queued, one can be dropped.
// ROR 0x001e785c

void CmnTaskMan::applyKillQueue()
{
    CmnTaskNode *node;
    int         *p;
    int          id;

    for (node = m_list; node != NULL && m_killEnd != m_killQueue; node = node->m_next)
    {
        if (node->m_state == 2)
            continue;
        p = m_killQueue;
        id = *p;
        for (;;)
        {
            p++;
            if (node->m_id == id)
            {
                termNode(node);
                node->m_state = 2;
                m_killEnd--;
                if (m_killEnd == m_killQueue)
                    return;
            }
            if (p >= m_killEnd)
                break;
            id = *p;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e7910

void CmnTaskMan::killKinds(const int *kinds)
{
    CmnTaskNode *node;
    CmnTaskNode *next;
    const int   *k;

    for (node = m_list; node != NULL; node = next)
    {
        next = node->m_next;
        if (kinds[0] == -1)
            continue;
        for (k = kinds; *k != -1; k++)
        {
            if (node->m_task->get_task_type() == *k)
                killTask(node->m_id);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e79c0

CmnITask *CmnTaskMan::findTask(int id)
{
    CmnTaskNode *node;

    for (node = m_list2; node != NULL; node = node->m_next)
    {
        if (node->m_id == id)
            return node->m_task;
    }
    for (node = m_list; node != NULL; node = node->m_next)
    {
        if (node->m_id == id)
            return node->m_task;
    }
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// The head of the list when it is due by the frame count (inlined twice
// in exec).

static CmnTaskNode *taskPopDue(CmnTaskMan *man)
{
    CmnTaskNode *node;

    node = man->m_list;
    if (node == NULL || (u_int)man->m_timer.m_updates < node->m_nextFrame)
        return NULL;
    man->m_list = node->m_next;
    node->m_next = NULL;
    return node;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: GamMan::execPauseReq (g_gamMan.execPauseReq)
// ROR 0x001e7a20

int CmnTaskMan::exec()
{
    CmnTaskNode *node;
    CmnTaskNode *done;
    CmnTaskNode *prev;
    CmnTaskNode *next;
    int          count;

    m_timer.hold();
    m_killEnd = m_killQueue;

    // Step the due nodes; a node that ends is freed once the next one is
    // out of the list.
    node = taskPopDue(this);
    while (node != NULL)
    {
        done = NULL;
        if (node->m_state == 0)
        {
            int ret = node->m_task->exec(node->m_id, m_timer.m_lastTime, node->m_arg);

            node->m_lastFrame = node->m_nextFrame;
            node->m_nextFrame += node->m_period;
            if (ret != 0 || (node->m_end != 0 && node->m_end < node->m_nextFrame))
                done = node;
            else
                insertNode(node);
        }
        else if (node->m_state != 2)
        {
            node->m_lastFrame = node->m_nextFrame;
            node->m_nextFrame += node->m_period;
            insertNode(node);
        }
        else
        {
            done = node;
        }
        next = taskPopDue(this);
        if (done != NULL)
            freeNode(done);
        node = next;
    }

    if (m_killEnd != m_killQueue)
        applyKillQueue();

    // Free the dead nodes and rewrite the live id table.
    count = 0;
    prev = NULL;
    for (node = m_list; node != NULL; node = next)
    {
        next = node->m_next;
        if (node->m_state == 2)
        {
            if (prev == NULL)
                m_list = next;
            else
                prev->m_next = next;
            freeNode(node);
        }
        else
        {
            m_live[count++] = node->m_id;
            prev = node;
        }
    }
    m_live[count] = -1;

    g_gamMan.execPauseReq();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// termNode's body, inlined.
// callcheck: memset (the node is cleared before it is freed)
// callcheck: operator_delete (delete task, delete node)
// ROR 0x001e7c7c

void CmnTaskMan::freeNode(CmnTaskNode *node)
{
    CmnITask *task;

    untrackNode(node);
    task = node->m_task;
    if (task != NULL)
    {
        task->term();
        if ((node->m_flags & 1) != 0)
        {
            delete task;
            node->m_flags &= ~1u;
        }
        node->m_task = NULL;
    }
    memset(node, 0, sizeof(*node));
    delete node;
}

// ──────────────────────────────────────────────────────────────────────
// The ROM frees an owned task with a plain operator delete (no
// destructor call); the host deletes it through its virtual destructor.
// callcheck: operator_delete (delete task)
// ROR 0x001e7d08

void CmnTaskMan::termNode(CmnTaskNode *node)
{
    CmnITask *task;

    untrackNode(node);
    task = node->m_task;
    if (task == NULL)
        return;
    task->term();
    if ((node->m_flags & 1) != 0)
    {
        delete task;
        node->m_flags &= ~1u;
    }
    node->m_task = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e7d7c

int CmnTaskMan::getTask(int id)
{
    int i;

    if (id == -1)
        return 0;
    for (i = 0; i < TASK_LIVE_MAX && m_live[i] != -1; i++)
    {
        if (m_live[i] == id)
            return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e7de8

void CmnTaskMan::removeLiveId(int id)
{
    int i;

    if (id == -1)
        return;
    for (i = 0; i < TASK_LIVE_MAX && m_live[i] != -1; i++)
    {
        if (m_live[i] == id)
            m_live[i] = -2;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e7e50

void CmnTaskMan::addLiveId(int id)
{
    int i;

    if (id == -1)
        return;
    for (i = 0; i < TASK_LIVE_MAX && m_live[i] != -1; i++)
    {
        if (m_live[i] == id)
            return;
    }
    if (i < 0xfe)
    {
        m_live[i] = id;
        m_live[i + 1] = -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e7ed0

int TskTest::exec(int id, int time, int arg)
{
    (void)id; (void)time; (void)arg;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: GamMan::getActorNo (g_gamMan.getActorNo)
// ROR 0x001e7ed8

TskLogger::TskLogger(u_int oid)
{
    m_oid = oid;
    m_actor = g_gamMan.getActorNo(oid);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e7f18

int TskLogger::exec(int id, int time, int arg)
{
    (void)id; (void)time; (void)arg;
    return 0;
}
