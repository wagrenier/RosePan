/* ==========================================================================
 *  sequence/sequence.h
 *
 *  The event sequence system (sequence/sequence.cpp 0x20e3d8..0x20ff34 and
 *  the VM run after it, 0x20ff34..0x215e1c, still unassigned): one global
 *  at bss 0x7a4440 that loads the sequence images GamMan hands it and steps
 *  their threads every frame ("mst", "trg", "imdy", "single" thread kinds).
 *  The scripts are MIPS code the VM 0x214c24 interprets; their calls into
 *  the game go through the native table of sequence/seqcall.cpp.
 *
 *  Three images: 0 the system script (data/SEQ/G000.GRB, 0x20000 bytes),
 *  1 the stage script (W.WRD, 0x18000), 2 a second script (0x30000). A
 *  thread id's top two bits are its image; an image's main thread is
 *  image << 30 | 0x04000000.
 * ======================================================================== */
#ifndef _SEQUENCE_SEQUENCE_H
#define _SEQUENCE_SEQUENCE_H

#include <stdint.h>

#include "sdk/eetypes.h"

/* ---- the node pools (RoR inlines one list template wherever it uses
 * them; the names are invented) ---- */

/* A list node: EE prev +0x0, next +0x4, the data after them (+0x10 for a
 * thread record, 16-aligned; +0x8 for an entry). layout: host. */
template <class T>
struct SeqNode
{
    SeqNode *m_prev;            // ROR 0x0
    SeqNode *m_next;            // ROR 0x4
    T        m_data;            // ROR 0x10 / 0x8
};

/* A circular list with a sentinel node (its data unused) and the count
 * after it (EE +0xc0 for thread nodes, +0x18 for entries). layout: host. */
template <class T>
struct SeqList
{
    SeqNode<T> m_head;          // ROR 0x0
    int        m_count;         // ROR sizeof(SeqNode<T>)

    void reset()
    {
        m_head.m_prev = &m_head;
        m_head.m_next = &m_head;
        m_count = 0;
    }
    SeqNode<T> *begin() { return m_head.m_next; }
    SeqNode<T> *end() { return &m_head; }
    /* No check for an empty list, as in the ROM. */
    SeqNode<T> *popBack()
    {
        SeqNode<T> *node = m_head.m_prev;
        m_head.m_prev = node->m_prev;
        node->m_prev->m_next = &m_head;
        m_count--;
        return node;
    }
    void pushBack(SeqNode<T> *node)
    {
        node->m_prev = m_head.m_prev;
        m_head.m_prev->m_next = node;
        node->m_next = &m_head;
        m_head.m_prev = node;
        m_count++;
    }
    void erase(SeqNode<T> *node)
    {
        node->m_prev->m_next = node->m_next;
        node->m_next->m_prev = node->m_prev;
        m_count--;
    }
    /* Moves every node of from to the end of this list. */
    void spliceAll(SeqList &from)
    {
        if (from.m_count == 0)
            return;
        m_head.m_prev->m_next = from.m_head.m_next;
        from.m_head.m_next->m_prev = m_head.m_prev;
        m_head.m_prev = from.m_head.m_prev;
        from.m_head.m_prev->m_next = &m_head;
        m_count += from.m_count;
        from.reset();
    }
};

/* A free list and a used list over max nodes (EE for thread records:
 * free +0x0, used +0xd0, max +0x1a0, the nodes +0x1c0; for entries +0x0 /
 * +0x1c / +0x38 / +0x40). layout: host. */
template <class T>
struct SeqPool
{
    SeqList<T>  m_free;
    SeqList<T>  m_used;
    int         m_max;
    /* Inline in the Sequence's own pools, new[] in the image pools. */
    SeqNode<T> *m_node;

    /* Links nodes[0..max) into the free list in array order. */
    void build(SeqNode<T> *nodes, int max)
    {
        m_free.reset();
        m_used.reset();
        m_max = max;
        m_node = nodes;
        for (int i = 1; i < max; i++) {
            nodes[i].m_prev = &nodes[i - 1];
            nodes[i - 1].m_next = &nodes[i];
        }
        m_free.m_count = max;
        m_free.m_head.m_next = &nodes[0];
        nodes[0].m_prev = &m_free.m_head;
        m_free.m_head.m_prev = &nodes[max - 1];
        nodes[max - 1].m_next = &m_free.m_head;
    }
    /* The last free node gets a copy of data and goes to the end of the
     * used list. */
    SeqNode<T> *add(const T &data)
    {
        SeqNode<T> *node = m_free.popBack();
        node->m_data = data;
        m_used.pushBack(node);
        return node;
    }
    void remove(SeqNode<T> *node)
    {
        m_used.erase(node);
        m_free.pushBack(node);
    }
    /* Every used node back to the free list. */
    void clear() { m_free.spliceAll(m_used); }
};

/* A pool with its nodes inline (EE +0x1c0 / +0x40). layout: host. */
template <class T, int N>
struct SeqFixedPool : SeqPool<T>
{
    SeqNode<T> m_storage[N];
};

/* A sequence thread (EE 0xb0 bytes). startThread 0x211e84 fills id, pc,
 * m_1c = -1 and m_24 = 1; the VM keeps the rest (the bytes it does not
 * set are stack garbage in the ROM). layout: host -- pc and stack hold
 * host addresses (open-items "Decisions": script registers are intptr_t). */
struct SeqThread
{
    u_int    m_flags;           // ROR 0x00: 0x40 holds pause 0x8000, 0x80 resume a call, 0x200 VM state saved
    u_int    m_id;              // ROR 0x04: image << 30 | ...
    intptr_t m_pc;              // ROR 0x08: where the VM continues
    int      m_resume;          // ROR 0x0c: the save slot seqVmResume loads (flag 0x80)
    int      _10[3];            // ROR 0x10
    int      m_1c;              // ROR 0x1c: -1 at the start
    int      m_area;            // ROR 0x20: the GamArea rect id whose trigger bits the thread holds, 0 none
    int      m_24;              // ROR 0x24: 1 at the start; the VM sets 1 after a native call
    int      m_arg;             // ROR 0x28: matched by findThread, returned by getThreadArg
    int      _2c[32];           // ROR 0x2c
    intptr_t m_stack;           // ROR 0xac: the VM stack slot, freed by releaseThread
};

/* An entry of the third pool (EE 0x10 bytes; freeStageEntries and
 * clearStageVars free the image-1 ones). layout: host. */
struct SeqEntry
{
    u_int m_id;                 // ROR 0x0: image << 30 | ...
    int   _4[3];                // ROR 0x4
};

/* EE size at least 0xdce4. layout: host -- EE offsets in the comments. */
struct Sequence
{
    Sequence();                                                          // ROR 0x0020e454
    ~Sequence();                                                         // ROR 0x0020e63c
    void term();                                                         // ROR 0x0020e664
    void init();                                                         // ROR 0x0020e690
    void loadStageImage(const u_char *image);                            // ROR 0x0020eda4
    void loadSystemImage(const u_char *image);                           // ROR 0x0020eea8
    void loadSubImage(int kind, const u_char *image);                    // ROR 0x0020efac
    void stopImages(int kind);                                           // ROR 0x0020f0c8
    int  getThreadArg();                                                 // ROR 0x0020f140
    void clearGlobals();                                                 // ROR 0x0020f1a8
    void clearStageVars();                                               // ROR 0x0020f1fc
    void freeThreadPools();                                              // ROR 0x0020f37c
    void releaseThread(SeqThread *thread);                               // ROR 0x0020f428
    void reapThreads();                                                  // ROR 0x0020f4fc
    void runImage();                                                     // ROR 0x0020f788
    void exec();                                                         // ROR 0x0020f98c

    /* ---- the VM run (unassigned@0020ff34) ---- */
    /* Makes the VM continue at target, an address in the current image
     * (the native function then returns SEQ_RET_JUMP). */
    void jumpTo(intptr_t target);                                        // ROR 0x002113a4
    /* Queues a thread: id, starting at the first "addiu sp" among the
     * words entry + 0x94..0xb0 (on the start list; exec moves it into
     * its image pool). */
    void startThread(u_int id, intptr_t entry);                          // ROR 0x00211e84
    void freeStageEntries();                                             // ROR 0x002127f4
    /* Moves image idx's threads to the ended list and reaps them. */
    void killThreads(int idx, int arg);                                  // ROR 0x00213450
    /* Non-zero when the id's image pool has it; *node gets its node. */
    int  findThread(u_int id, SeqNode<SeqThread> **node, int arg);       // ROR 0x00213c6c
    /* Points the VM at image (registers 0x7b47d0..) and saves its state
     * for image idx. */
    void setupImage(u_char *image, int idx);                             // ROR 0x00213f48

    u_char                *m_image[4];      // ROR 0x00: images 0..2 (the game heap), [3] = 0
    SeqPool<SeqThread>    *m_threads[4];    // ROR 0x10: each image's 0x40 threads, [3] = 0
    /* Threads startThread queued (exec moves them into the image pools). */
    SeqFixedPool<SeqThread, 0x80> m_start;  // ROR 0x40
    /* Threads whose run ended (reaped at the next exec). */
    SeqFixedPool<SeqThread, 0x80> m_ended;  // ROR 0x6200
    SeqFixedPool<SeqEntry, 0x100> m_entries; // ROR 0xc3c0
    int      m_dc00;            // ROR 0xdc00: 0 after init
    int      m_dc04;            // ROR 0xdc04: 0 after init
    /* The base address of each loaded image (index = the VM's current
     * image, g_seqImage); system call 0x1e sets one. */
    intptr_t m_base[5];         // ROR 0xdc08
    /* The global variables, 0x800 bytes (cleared with the bitset by
     * clearGlobals). Words read here: [0x17] (system call 0xf), [0x64]
     * (set by system call 0x35); init sets the floats [9] = 100 and
     * [0x30] = 1.56. */
    int     *m_global;          // ROR 0xdc1c
    /* A bitset of 0x800 bytes (word 0 kept by the clear): GamMan::addItem
     * sets the bit of the item's record. */
    u_int   *m_bits;            // ROR 0xdc20
    /* Bit 0 set by GamMan::enterMenu for the item / map menus (exec does
     * nothing while set); 0x100 picks the word getThreadArg returns; 0x400
     * set by scenePlayStart. */
    u_int    m_flags;           // ROR 0xdc24
    char     m_name[8];         // ROR 0xdc40: startThread copies 8 chars of data 0x301708
    int      m_arg;             // ROR 0xdc58: getThreadArg's word under flag 0x100
    int      m_dce0;            // ROR 0xdce0: 0 after init
};

extern Sequence g_sequence;     /* bss 0x7a4440 */

/* ---- the VM's state (bss 0x7b4640..0x7b4dd0; the VM file defines it in
 * the ROM, sequence.cpp holds it until that file is written) ---- */

extern SeqNode<SeqThread> *g_seqThread;     /* bss 0x7b4640: the running thread */
extern SeqNode<SeqThread> *g_seqThreadEnd;  /* bss 0x7b4650: its list's end */
extern intptr_t g_seqImageCode;             /* bss 0x7b47d0: the current image + 0x30 */
extern int     *g_seqGlobal;                /* bss 0x7b4910: Sequence::m_global */
extern u_char  *g_seqStackMid;              /* bss 0x7b4914: g_seqStack + 0x4000 */
/* Save slots of a waiting call's registers (0x400 bytes, 8 x 0x80 in
 * the ROM; seqVmResume reads a0..a3 back). PC: intptr_t words. */
extern intptr_t g_seqCallSave[0x100];       /* bss 0x7b4920 */
extern u_char  *g_seqStack;                 /* bss 0x7b4d20: 0x10000 bytes, 32 stacks of 2 KB */
extern int      g_seqStackUsed[32];         /* bss 0x7b4d30 */
extern int      g_seqImage;                 /* bss 0x7b4dd0: the image exec is running */
extern u_char  *g_seqImageSave[3];          /* data 0x2d9e08: 0x780 bytes per image */

void     seqVmSelectImage(int idx);                                      // ROR 0x00214310
void     seqVmSaveImage(int idx);                                        // ROR 0x002144c4
void     seqVmLoadThread(Sequence *seq);                                 // ROR 0x00214664
void     seqVmFreeStack(intptr_t stack);                                 // ROR 0x00214a94
void     seqVmResume(int slot);                                          // ROR 0x00214bcc
/* Runs a thread from pc until it waits; returns where it continues, 0
 * when it ended. */
intptr_t seqVmRun(intptr_t pc);                                          // ROR 0x00214c24

/* Writes "SEQ" into the sequence data folder name (data 0x2d9910, 0x40
 * bytes); both arguments are unused in the US build. */
void seqSetLanguage(int lang, int arg);                                  // ROR 0x0020e3d8
/* The folder name seqSetLanguage writes. */
char *seqGetDataDir(void);                                               // ROR 0x0020e400
/* The global sequence's path: compiled (data/SEQ/G000.GRB) when kind is
 * non-zero, else its source (data/seq/seq_global/g000.seq). */
char *seqMakeGlobalPath(char *buf, int size, int kind);                  // ROR 0x0020e40c

/* data 0x2d9e18 -> "1.01": the number in main's version line (also read
 * by 0x1d6078). */
extern const char *g_versionNumber;

#endif /* _SEQUENCE_SEQUENCE_H */
