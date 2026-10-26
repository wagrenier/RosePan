/* ==========================================================================
 *  task/tsk_man.h
 *
 *  The task manager (task/tsk_man.cpp, pinned 0x1e7094..0x1e7f28; the file
 *  name is a guess after RoR's task/tsk_*.cpp, no source string names it).
 *  One CmnTaskMan, the global g_taskMan (bss 0x7a3d80), runs every task
 *  object (CmnITask) main's loop has added, once a frame from
 *  CmnTaskMan::exec:
 *  - addTask wraps the task in a 0x40-byte node (CmnTaskNode) with a new id
 *    (from 2 up), calls its init and inserts the node into the list at +0,
 *    sorted by the frame it runs next (m_nextFrame, then insertion order).
 *  - exec pops every node due by the timer's frame count, steps a running
 *    one (state 0) through its exec slot, reinserts it m_period frames later
 *    unless exec returned non-zero or its frame count ran out, and frees it
 *    then (term, delete when it owns the task). Paused nodes (state 1) only
 *    move on; dead ones (state 2) are freed.
 *  - killTask marks a live node dead at once, or queues the id when the node
 *    is not in the list (it is the one exec is stepping): exec applies the
 *    queue after the run. Then exec sweeps the dead nodes and rewrites the
 *    live id table (+0x288, -1 ends it), which getTask reads: "still runs".
 *  Also here: the test / logger tasks no code creates (TskTest, TskLogger).
 * ======================================================================== */
#ifndef _TASK_TSK_MAN_H
#define _TASK_TSK_MAN_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "system/system.h"

/* The task interface 32 classes derive from. The ROM has no vtable of its
 * own for it (it is abstract; only its pause slot has a body, 0x28a694,
 * empty); every task class's vtable has these six slots.
 * PORT: on the host the slots have defaults, so that task classes whose
 * virtuals are not reconstructed yet (only their constructors are, or
 * stubs) still run: kind 0, nothing to do, and exec ends the task on its
 * first step -- what every task did while CmnTaskMan was a stub.
 * layout: host -- no members. */
struct CmnITask
{
    /* The task's kind: the pause / kill lists and pauseOthers /
     * killStageTasks pick tasks by it. */
    virtual int  get_task_type() { return 0; }                          // EE slot 1 (+0x08)
    /* Called by CmnTaskMan::setNodePause for kinds 5, 0x10, 0x13 and 0x14
     * when the node's pause state changes. */
    virtual void pause(int on) { (void)on; }                            // EE slot 2 (+0x10), ROR 0x0028a694
    /* addTask, once the node is set up. */
    virtual void init() {}                                              // EE slot 3 (+0x18)
    /* One step: the task id, the timer's time of the last frame (ms), the
     * node's arg. Non-zero ends the task. */
    virtual int  exec(int id, int time, int arg)                        // EE slot 4 (+0x20)
    {
        (void)id; (void)time; (void)arg;
        return 1;
    }
    /* The node is freed (end, kill, the manager's destructor). */
    virtual void term() {}                                              // EE slot 5 (+0x28)
    virtual ~CmnITask() {}                                              // EE slot 6 (+0x30)
};

/* addTask's node, 0x40 bytes on the EE (operator new). layout: host --
 * pointers; EE offsets in the comments. */
struct CmnTaskNode
{
    CmnITask    *m_task;        // ROR 0x00: NULL once terminated
    CmnTaskNode *m_next;        // ROR 0x04
    int          m_state;       // ROR 0x08: 0 run, 1 paused, 2 dead (freed by exec)
    int          m_id;          // ROR 0x0c
    int         *m_idOut;       // ROR 0x10: the caller's id word (set only when given)
    u_int        m_flags;       // ROR 0x14: bit 0 the node owns the task (delete on free)
    int          m_arg;         // ROR 0x18: exec's third argument
    int          _1c;           // ROR 0x1c: not written
    int          m_kind;        // ROR 0x20: get_task_type at addTask
    u_int        m_start;       // ROR 0x24: the first frame
    u_int        m_period;      // ROR 0x28: frames between steps
    u_int        m_end;         // ROR 0x2c: the last frame to step, 0 none
    u_int        m_lastFrame;   // ROR 0x30: the frame of the last step
    u_int        m_nextFrame;   // ROR 0x34: the frame of the next step (the list's order)
};

#define TASK_KILL_QUEUE_MAX 128 /* words +0x84..+0x284 */
#define TASK_LIVE_MAX       256 /* words from +0x288 */

/* The task manager (EE 0x6c0 bytes: g_taskMan 0x7a3d80 .. g_sequence
 * 0x7a4440). layout: host -- pointers; EE offsets in the comments. */
struct CmnTaskMan
{
    CmnTaskMan();                                                        // ROR 0x001e7094
    ~CmnTaskMan();                                                       // ROR 0x001e70dc

    /* A new node for task: it first runs at frame start (-1: now), then
     * every period frames, for frames frames (0: until exec returns
     * non-zero or it is killed). The id goes to *id when given; flags bit
     * 0 hands the task to the manager (deleted when the node is freed).
     * group is not read. 1 = added. */
    int  addTask(int group, int start, int period, int frames, CmnITask *task, int arg,
                 int *id, int flags);                                    // ROR 0x001e71fc
    /* Steps every node due this frame (see the file comment), then applies
     * the kill queue, frees the dead nodes, rewrites the live id table and
     * runs GamMan::execPauseReq. Main's loop, once a frame. */
    int  exec();                                                         // ROR 0x001e7a20
    /* 1 while the task id is in the live id table (added, not freed), else
     * 0; -1 is never live. */
    int  getTask(int id);                                                // ROR 0x001e7d7c
    /* Kills the task id: a live node in the list dies now (state 2), else
     * the id is queued (once) for exec. -1 is ignored. */
    void killTask(int id);                                               // ROR 0x001e7724
    /* Pauses (on 1) / resumes (0) the task id (-1 ignored). */
    void pauseTask(int id, int on);                                      // ROR 0x001e7414
    /* Pauses / resumes every task whose kind is in kinds (-1 ends the
     * list; a list starting with -1 matches nothing); dead ones stay. */
    void pauseKinds(const int *kinds, int on);                           // ROR 0x001e747c
    /* Pauses / resumes every task but those of kinds 1, 2, 5, 7, 0xf,
     * 0x11, 0x12, 0x17. */
    void pauseOthers(int on);                                            // ROR 0x001e7544
    /* The task object of id (the list at +4, then +0), NULL when none. */
    CmnITask *findTask(int id);                                          // ROR 0x001e79c0
    /* Every task of the list at +0 is marked for removal (state 2) but
     * those whose kind is 1..3 or 0x11. */
    void killStageTasks();                                               // ROR 0x001e7618
    /* Every live task of the list at +0 whose kind is kind is terminated
     * now and marked dead. */
    void killKind(int kind);                                             // ROR 0x001e77b4
    /* killTask for every task of the list at +0 whose kind is in kinds (-1
     * ends it). The ROM's caller passes a third word it never reads. */
    void killKinds(const int *kinds);                                    // ROR 0x001e7910

    void insertNode(CmnTaskNode *node);                                  // ROR 0x001e7184
    void setNodePause(CmnTaskNode *node, int on);                        // ROR 0x001e7340
    void untrackNode(CmnTaskNode *node);                                 // ROR 0x001e7698
    int  killListed(int id);                                             // ROR 0x001e76b8
    void applyKillQueue();                                               // ROR 0x001e785c
    void freeNode(CmnTaskNode *node);                                    // ROR 0x001e7c7c
    void termNode(CmnTaskNode *node);                                    // ROR 0x001e7d08
    void removeLiveId(int id);                                           // ROR 0x001e7de8
    void addLiveId(int id);                                              // ROR 0x001e7e50

    CmnTaskNode *m_list;        // ROR 0x00: the nodes, by m_nextFrame
    CmnTaskNode *m_list2;       // ROR 0x04: searched first by findTask; nothing here adds to it
    int          m_nextId;      // ROR 0x08: from 2
    int          _0c[13];       // ROR 0x0c
    SysTimer     m_timer;       // ROR 0x40 (bss 0x7a3dc0): m_updates is the frame count
    int          _6c[6];        // ROR 0x6c
    int          m_killQueue[TASK_KILL_QUEUE_MAX]; // ROR 0x84: ids killTask could not find
    int         *m_killEnd;     // ROR 0x284: the queue's end
    int          m_live[TASK_LIVE_MAX];            // ROR 0x288: live ids, -1 ends, -2 removed
    int          _688[14];      // ROR 0x688
};

extern CmnTaskMan g_taskMan;    /* bss 0x7a3d80 (system/system.cpp) */

/* TskTest : CmnITask (vtable 0x2e4898, no constructor in the ROM: no code
 * creates one). Its slots are inline (kind 4, empty init / term).
 * layout: host -- no members. */
struct TskTest : CmnITask
{
    int  get_task_type() override { return 4; }                         // ROR 0x0028fed4
    void init() override {}                                             // ROR 0x0028fedc
    int  exec(int id, int time, int arg) override;                      // ROR 0x001e7ed0
    void term() override {}                                             // ROR 0x0028fee4
};

/* TskLogger : CmnITask (vtable 0x2e4858; no code creates one): an oid and
 * its actor no. layout: host -- EE +4 / +8. */
struct TskLogger : CmnITask
{
    explicit TskLogger(u_int oid);                                       // ROR 0x001e7ed8

    int  get_task_type() override { return 4; }                         // ROR 0x0028ff70
    void init() override {}                                             // ROR 0x0028ff78
    int  exec(int id, int time, int arg) override;                      // ROR 0x001e7f18
    void term() override {}                                             // ROR 0x0028ff80

    u_int m_oid;                // ROR 0x04
    int   m_actor;              // ROR 0x08: GamMan::getActorNo(m_oid)
};

#endif /* _TASK_TSK_MAN_H */
