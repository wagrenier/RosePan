// sequence/sequence.cpp -- the event sequence system (0x20e3d8..0x20ff34).
//
// The Sequence keeps three images (system, stage, a second script), each
// with a pool of 0x40 threads. startThread (the VM file) queues a thread
// on the start list; exec moves the queued threads into their images'
// pools, runs every thread of images 0..2 through the VM, and reaps the
// threads whose run ended at the next frame (ended list -> free node).
//
// Written: the whole file. The VM it drives (seqVm*, startThread,
// findThread, killThreads, setupImage) is sequence/seqvm.cpp.

#include <stdio.h>
#include <string.h>

#include "game/gamman.h"
#include "rosepan/rosepan_eventtest.h"
#include "rosepan/rosepan_seqtest.h"
#include "sequence/sequence.h"
#include "utl/utlmisc.h"

static const char s_file[] = "sequence/sequence.cpp";

// The sequence data folder (data 0x2d9910, 0x40 bytes).
static char s_dataDir[0x40];

// ──────────────────────────────────────────────────────────────────────
// The US build has one folder whatever the language: both arguments are
// unused.
// ROR 0x0020e3d8

void seqSetLanguage(int lang, int arg)
{
    (void)lang;
    (void)arg;
    snprintf(s_dataDir, sizeof(s_dataDir), "SEQ");
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0020e400

char *seqGetDataDir(void)
{
    return s_dataDir;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0020e40c

char *seqMakeGlobalPath(char *buf, int size, int kind)
{
    if (kind == 0)
        snprintf(buf, size, "data/seq/seq_global/g000.seq");
    else
        snprintf(buf, size, "data/SEQ/G000.GRB");
    return buf;
}

// ──────────────────────────────────────────────────────────────────────
// The start and ended lists hold 0x80 threads, the entry pool 0x100.
// ROR 0x0020e454

Sequence::Sequence()
{
    m_start.build(m_start.m_storage, 0x80);
    m_ended.build(m_ended.m_storage, 0x80);
    m_entries.build(m_entries.m_storage, 0x100);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0020e63c

Sequence::~Sequence()
{
    // callcheck: operator_delete (the deleting destructor's flag 1; the host's global is never deleted)
}

// ──────────────────────────────────────────────────────────────────────
// sysTermManagers: the image pools go and init builds them again.
// ROR 0x0020e664

void Sequence::term()
{
    freeThreadPools();
    init();
}

// ──────────────────────────────────────────────────────────────────────
// An image's thread pool: 0x40 nodes from new[] (EE 0x200 + 0x3000 bytes).

static SeqPool<SeqThread> *newThreadPool()
{
    SeqPool<SeqThread> *pool = new SeqPool<SeqThread>;
    pool->build(new SeqNode<SeqThread>[0x40], 0x40);
    return pool;
}

static void setFloat(int *words, int i, float value)
{
    memcpy(&words[i], &value, sizeof(value));
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0020e690

void Sequence::init()
{
    // callcheck: operator_new, operator_new[] (newThreadPool, inlined four times in the ROM)
    m_threads[0] = newThreadPool();
    m_threads[1] = newThreadPool();
    m_threads[2] = newThreadPool();
    // The ROM builds a fourth pool and drops it.
    newThreadPool();
    m_threads[3] = NULL;

    m_threads[0]->clear();
    m_threads[1]->clear();
    m_threads[2]->clear();
    m_entries.clear();

    m_record = NULL;
    m_global = (int *)g_utlMemory.allocDebug(0xa00, s_file, 484, 0, 1);
    m_bits = (u_int *)g_utlMemory.allocDebug(0xa00, s_file, 485, 0, 1);
    m_image[0] = (u_char *)g_utlMemory.allocDebug(0x20000, s_file, 489, 0, 1);
    m_image[1] = (u_char *)g_utlMemory.allocDebug(0x18000, s_file, 490, 0, 1);
    m_image[2] = (u_char *)g_utlMemory.allocDebug(0x30000, s_file, 491, 0, 1);
    m_image[3] = NULL;

    g_seqStack = (u_char *)g_utlMemory.allocDebug(0x10000, s_file, 501, 0, 1);
    memset(g_seqStackUsed, 0, sizeof(g_seqStackUsed));
    g_seqImageSave[0] = (u_char *)g_utlMemory.allocDebug(0x780, s_file, 504, 0, 1);
    g_seqImageSave[1] = (u_char *)g_utlMemory.allocDebug(0x780, s_file, 505, 0, 1);
    g_seqImageSave[2] = (u_char *)g_utlMemory.allocDebug(0x780, s_file, 506, 0, 1);

    m_base[0] = 0;
    m_base[1] = 0;
    m_base[2] = 0;
    m_base[3] = 0;
    g_seqGlobal = m_global;
    g_seqStackMid = g_seqStack + 0x4000;
    memset(m_bits, 0, 0x800);
    clearGlobals();
    setFloat(m_global, 9, 100.0f);
    setFloat(m_global, 0x30, 100.0f);
    setFloat(m_global, 0x30, 1.56f);
    // 0xcccccccc in every word (PC: in the low 32 bits of each intptr_t).
    memset(g_seqCallSave, 0xcc, sizeof(g_seqCallSave));
    m_dc00 = 0;
    m_dc04 = 0;
}

// ──────────────────────────────────────────────────────────────────────
// GamMan::loadStage: the stage script (W.WRD) into image 1 and its main
// thread.
// ROR 0x0020eda4

void Sequence::loadStageImage(const u_char *image)
{
    memcpy(m_image[1], image, 0x18000);
    setupImage(m_image[1], 1);
    startThread(0x44000000, g_seqImageCode - 0xa4);
}

// ──────────────────────────────────────────────────────────────────────
// GamMan::loadSystemData: the system script (G000.GRB) into image 0.
// ROR 0x0020eea8

void Sequence::loadSystemImage(const u_char *image)
{
    memcpy(m_image[0], image, 0x20000);
    setupImage(m_image[0], 0);
    startThread(0x04000000, g_seqImageCode - 0xa4);
}

// ──────────────────────────────────────────────────────────────────────
// Kind 2 loads image 2; kind 1 does nothing.
// ROR 0x0020efac

void Sequence::loadSubImage(int kind, const u_char *image)
{
    if (kind != 2)
        return;
    memcpy(m_image[2], image, 0x30000);
    setupImage(m_image[2], 2);
    startThread(0x84000000, g_seqImageCode - 0xa4);
}

// ──────────────────────────────────────────────────────────────────────
// Kind 1 stops the stage image and image 2, kind 2 image 2 only.
// ROR 0x0020f0c8

void Sequence::stopImages(int kind)
{
    if (kind == 1) {
        freeStageEntries();
        killThreads(1);
        killThreads(2);
        return;
    }
    if (kind != 2)
        return;
    killThreads(2);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0020f140

int Sequence::getThreadArg()
{
    if ((m_flags & 0x100) == 0)
        return g_seqThread->m_data.m_arg;
    return m_single.m_arg;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0020f1a8

void Sequence::clearGlobals()
{
    memset(m_global, 0, 0x800);
    memset(m_bits + 1, 0, 0x7fc);
}

// ──────────────────────────────────────────────────────────────────────
// The stage's variables (globals 0x300.., bits 0x20..) and entries go.
// ROR 0x0020f1fc

void Sequence::clearStageVars()
{
    memset((u_char *)m_global + 0x300, 0, 0x200);
    memset((u_char *)m_bits + 0x20, 0, 0x200);

    SeqList<SeqEntry> &used = m_entries.m_used;
    for (SeqNode<SeqEntry> *it = used.begin(); it != used.end(); it = it->m_next) {
        if ((it->m_data.m_id >> 30) == 1) {
            // As in the ROM, the loop steps once more after a removal:
            // the node after a removed one is not tested.
            SeqNode<SeqEntry> *next = it->m_next;
            m_entries.remove(it);
            it = next;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0020f37c

void Sequence::freeThreadPools()
{
    for (int i = 0; i < 3; i++) {
        SeqPool<SeqThread> *pool = m_threads[i];
        if (pool != NULL) {
            if (pool->m_node != NULL)
                delete[] pool->m_node;
            delete pool;    // callcheck: operator_delete, operator_delete[] (C++ delete / delete[])
        }
    }
    m_threads[2] = NULL;
    m_threads[0] = NULL;
    m_threads[1] = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// A thread is over: what it held in the game goes with it.
// ROR 0x0020f428

void Sequence::releaseThread(SeqThread *thread)
{
    if (thread->m_area != 0) {
        g_gamMan.m_area.ctl_trg_flg(2, thread->m_area, 0x2000000);
        if (g_gamMan.m_area.ctl_trg_flg(4, thread->m_area, 0x10) != 0)
            g_gamMan.m_area.ctl_trg_flg(3, thread->m_area, 0x10000000);
        g_gamMan.m_area.ctl_trg_flg(5, thread->m_area, 0);
    }
    if (thread->m_flags & 0x40)
        g_gamMan.pause(0x8000);
    if (thread->m_stack != 0) {
        seqVmFreeStack(thread->m_stack);
        thread->m_stack = 0;
    }
    memset(thread, 0, sizeof(*thread));
}

// ──────────────────────────────────────────────────────────────────────
// Each ended record names a thread of an image pool: it leaves the pool
// and is released. The two main threads tell GamMan.
// ROR 0x0020f4fc

void Sequence::reapThreads()
{
    SeqList<SeqThread> &ended = m_ended.m_used;
    for (SeqNode<SeqThread> *it = ended.begin(); it != ended.end(); it = it->m_next) {
        u_int id = it->m_data.m_id;
        u_int image = id >> 30;
        SeqNode<SeqThread> *node = NULL;
        if (findThread(id, &node, it->m_data.m_arg) == 0)
            continue;
        if (id == 0x44000000)
            g_gamMan.onStageSeqEnd();
        if (id == 0x84000000)
            g_gamMan.onSubSeqEnd();
        if (image <= 2)
            m_threads[image]->remove(node);
        releaseThread(&node->m_data);
    }
}

// ──────────────────────────────────────────────────────────────────────
// Every thread of the image g_seqImage runs until it waits; one whose run
// returns 0 is copied to the ended list (it stays in the image pool until
// reapThreads).
// ROR 0x0020f788

void Sequence::runImage()
{
    seqVmSelectImage(g_seqImage);
    while (g_seqThread != g_seqThreadEnd) {
        seqVmLoadThread();
        SeqThread &thread = g_seqThread->m_data;
        if (thread.m_flags & 0x80) {
            thread.m_flags &= ~0x80u;
            seqVmResume(thread.m_resume);
        }
        intptr_t pc = seqVmRun(thread.m_pc);
        if (pc == 0)
            m_ended.add(g_seqThread->m_data);
        else
            g_seqThread->m_data.m_pc = pc;
        g_seqThread = g_seqThread->m_next;
    }
    seqVmSaveImage(g_seqImage);
}

// ──────────────────────────────────────────────────────────────────────
// Once a frame from main's loop (nothing while flag 1 is set): last
// frame's ended threads go, the queued ones start, images 0..2 run.
// ROR 0x0020f98c

void Sequence::exec()
{
    RosePan_SeqForcedTest();  // PC: ROSEPAN_SEQ_TEST=1 loads the G000 image (rosepan_seqtest.h)
    RosePanEventTest_Tick();  // PC: the event test's jumps (rosepan_eventtest.h)
    if (m_flags & 1)
        return;

    if (m_ended.m_used.m_count != 0) {
        reapThreads();
        m_ended.clear();
    }

    if (m_start.m_used.m_count != 0) {
        SeqList<SeqThread> &start = m_start.m_used;
        for (SeqNode<SeqThread> *it = start.begin(); it != start.end(); it = it->m_next) {
            u_int image = it->m_data.m_id >> 30;
            if (image <= 2)
                m_threads[image]->add(it->m_data);
            if (it->m_data.m_area != 0)
                g_gamMan.m_area.ctl_trg_flg(3, it->m_data.m_area, 0x2000000);
        }
        m_start.clear();
    }

    for (int image = 0; image < 3; image++) {
        g_seqImage = image;
        SeqPool<SeqThread> *pool = m_threads[image];
        if (pool->m_used.m_count != 0) {
            g_seqThread = pool->m_used.begin();
            g_seqThreadEnd = pool->m_used.end();
            runImage();
        }
    }
    g_seqImage = 0;
    g_gamMan.afterSeqExec();
}
