/* ==========================================================================
 *  sequence/sequence.h
 *
 *  The event sequence system (sequence/sequence.cpp 0x20e3d8..0x20ff34 and
 *  the VM after it, sequence/seqvm.cpp 0x20ff34..0x2155a0): one global
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
 * m_wait = -1 and m_24 = 1; the VM keeps the rest (the bytes it does not
 * set are stack garbage in the ROM, zero on the host). layout: host -- pc,
 * the registers and the stack hold host addresses (open-items "Decisions":
 * script registers are intptr_t). */
struct SeqThread
{
    u_int    m_flags;           // ROR 0x00: 0x40 holds pause 0x8000, 0x80 resume a call, 0x200 VM state saved
    u_int    m_id;              // ROR 0x04: image << 30 | ...
    intptr_t m_pc;              // ROR 0x08: where the VM continues
    int      m_resume;          // ROR 0x0c: the call save slot seqVmResume loads (flag 0x80)
    char     m_name[8];         // ROR 0x10: "mst", "trg", "imdy", "single" (startThread copies it into Sequence::m_single)
    int      _18;               // ROR 0x18
    int      m_wait;            // ROR 0x1c: Sequence::waitFrames' count, -1 idle
    int      m_area;            // ROR 0x20: the GamArea rect id whose trigger bits the thread holds, 0 none
    int      m_24;              // ROR 0x24: 1 at the start; the VM sets 1 after a native call
    int      m_arg;             // ROR 0x28: matched by findThread, returned by getThreadArg
    intptr_t m_reg[32];         // ROR 0x2c: the VM registers seqVmSaveThread keeps (flag 0x200)
    intptr_t m_stack;           // ROR 0xac: a 2 KB slot holding the thread's 0x780-byte gp area
};

/* An entry of the third pool (EE 0x10 bytes; freeStageEntries and
 * clearStageVars free the image-1 ones). layout: host. */
struct SeqEntry
{
    u_int m_id;                 // ROR 0x0: image << 30 | ...
    int   m_value;              // ROR 0x4: addEntry's value
    int   _8[2];                // ROR 0x8: stack garbage in the ROM
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

    /* ---- the VM's thread functions (sequence/seqvm.cpp) ---- */
    /* Makes the VM continue at target, an address in the current image
     * (the native function then returns SEQ_RET_JUMP). */
    void jumpTo(intptr_t target);                                        // ROR 0x002113a4
    /* The running thread waits n frames: 1 while it waits, 0 when the
     * count ends (flag 0x400 starts it again). */
    int  waitFrames(int n);                                              // ROR 0x00211318
    /* Runs a thread at once in m_single (flag 0x100); one that waits goes
     * on the start list. Nothing when (id, arg) already runs. */
    void execThread(u_int id, intptr_t entry, int arg, u_int flags, int resume,
                    int area, const char *name);                         // ROR 0x002113dc
    /* Queues a thread: id, starting at the first "addiu sp" among the
     * words entry + 0x94..0xb0 (on the start list; exec moves it into
     * its image pool). */
    void startThread(u_int id, intptr_t entry);                          // ROR 0x00211e84
    /* Queues a thread start in m_start: entry (an address in image id >>
     * 30, moved to its addiu sp), arg, flags; callSave != 0 adds a call
     * save slot (flag 0x80). */
    void queueThread(u_int id, intptr_t entry, int arg, u_int flags, int callSave,
                     const char *name);                                  // ROR 0x002120ac
    void freeStageEntries();                                             // ROR 0x002127f4
    void addEntry(u_int id, int value);                                  // ROR 0x00212920
    /* addEntry's id: queueThread(id, its value, arg, 2, callSave, "none"). */
    void callEntry(u_int id, int arg, int callSave);                     // ROR 0x002129b8
    /* The same started at once (0x211830) with the VM state saved around it. */
    void callEntryNow(u_int id, int arg, int callSave);                  // ROR 0x00212a5c
    /* Copies the thread id / arg (findThread) into m_ended. */
    void endThread(u_int id, int arg);                                   // ROR 0x00212d08
    /* The same for the thread 0x213d90 finds by key. */
    void endThreadOf(intptr_t key);                                      // ROR 0x00212e64
    /* Moves image idx's threads to the ended list, reaps them, and clears
     * the image and its registers. */
    void killThreads(int idx);                                           // ROR 0x00213450
    int  hasThreadIn(u_int id, SeqNode<SeqThread> *it, SeqNode<SeqThread> *end,
                     int arg);                                           // ROR 0x00213adc
    int  findThreadIn(u_int id, SeqNode<SeqThread> *it, SeqNode<SeqThread> *end,
                      SeqNode<SeqThread> **node, int arg);               // ROR 0x00213b28
    /* Non-zero when the id's image pool has it; *node gets its node. */
    int  findThread(u_int id, SeqNode<SeqThread> **node, int arg);       // ROR 0x00213c6c
    int  hasThread(u_int id, int arg);                                   // ROR 0x00213d00
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
    /* The thread execThread runs at once (flag 0x100: the VM saves into
     * it, getThreadArg returns its arg). */
    SeqThread m_single;         // ROR 0xdc30
    /* A record seqVmLoadRecord 0x2144cc (unreferenced) loads and clears;
     * NULL after init. */
    SeqThread *m_record;        // ROR 0xdce0
};

extern Sequence g_sequence;     /* bss 0x7a4440 */

/* ---- the VM's state (bss 0x7b4640..0x7b4dd0, data 0x2d9950..0x2d9e18;
 * sequence/seqvm.cpp) ---- */

/* An image's VM addresses, kept by setupImage and restored by
 * seqVmSelectImage (bss 0x7b4850, 0x30 bytes each). layout: host. */
struct SeqVmImage
{
    intptr_t m_code;            // ROR 0x00: the image + 0x30
    int      m_link;            // ROR 0x04: header +0x10, the gp area's link address
    intptr_t m_data;            // ROR 0x08: the image + header +0x14
    intptr_t m_data2;           // ROR 0x0c: m_data + header +0x1c + 8
    int      m_hdr20;           // ROR 0x10: header +0x20
    int      m_hdr24;           // ROR 0x14: header +0x24
    intptr_t m_reloc;           // ROR 0x18: what lui 0x41 / 0x45 loads
    int      _1c;               // ROR 0x1c
    intptr_t m_gp;              // ROR 0x20: the 0x780-byte gp area (stack at its top)
    intptr_t m_sp;              // ROR 0x24
};

extern SeqNode<SeqThread> *g_seqThread;     /* bss 0x7b4640: the running thread */
extern SeqNode<SeqThread> *g_seqThreadEnd;  /* bss 0x7b4650: its list's end */
extern u_int    g_seqFpr[32];               /* bss 0x7b4680: the FPU registers (float bits) */
extern intptr_t g_seqGpr[32];               /* bss 0x7b4700: the registers (gp 28, sp 29, ra 31) */
extern intptr_t g_seqPc;                    /* bss 0x7b4780: the word being run */

/* PC: a 32-bit word of script memory that may hold a host pointer (the
 * wide-word map of seqvm.cpp; open-items "Decisions"). Game code that
 * stores an address into script data goes through these. */
intptr_t seqVmLoadWord(intptr_t addr);
void     seqVmStoreWord(intptr_t addr, intptr_t value);
extern int      g_seqHi;                    /* bss 0x7b4790 */
extern int      g_seqLo;                    /* bss 0x7b47a0 */
/* A taken jump: an image offset + 0x8160, a native call below 0x8160, or
 * the host pc Sequence::jumpTo sets. */
extern intptr_t g_seqJump;                  /* bss 0x7b47b0 */
extern u_int    g_seqBranch;                /* bss 0x7b47c0: a branch after the delay slot, | 0x80000000 */
extern intptr_t g_seqImageCode;             /* bss 0x7b47d0: the current image + 0x30 */
extern int      g_seqDataLink;              /* bss 0x7b47e0 */
extern intptr_t g_seqData2;                 /* bss 0x7b47f0 */
extern intptr_t g_seqData;                  /* bss 0x7b4800 */
extern intptr_t g_seqDataReloc;             /* bss 0x7b4810 */
extern int      g_seqHdr20;                 /* bss 0x7b4820 */
extern int      g_seqHdr24;                 /* bss 0x7b4830 */
extern int      g_seqFpuCond;               /* bss 0x7b4840: the compare result bc1t / bc1f test */
extern SeqVmImage g_seqImageState[3];       /* bss 0x7b4850 */
extern int     *g_seqGlobal;                /* bss 0x7b4910: Sequence::m_global */
extern u_char  *g_seqStackMid;              /* bss 0x7b4914: g_seqStack + 0x4000 */
/* Save slots of a waiting call's registers (8 x 32 registers;
 * seqVmResume reads a0..a3 back; word 0 0xcccccccc = free). PC: intptr_t
 * registers, compared on the low 32 bits. */
extern intptr_t g_seqCallSave[0x100];       /* bss 0x7b4920 */
extern u_char  *g_seqStack;                 /* bss 0x7b4d20: 0x10000 bytes, 32 stacks of 2 KB */
extern int      g_seqStackUsed[32];         /* bss 0x7b4d30 */
/* Where SEQ_RET_RETURN continues (+ 4); nothing in the VM file sets it. */
extern intptr_t g_seqReturnPc;              /* bss 0x7b4db0 */
extern int      g_seqImage;                 /* bss 0x7b4dd0: the image exec is running */
extern int      g_seqStackNext;             /* data 0x2d9950: the next stack slot to try */
extern intptr_t g_seqImageRegs[3][32];      /* data 0x2d9c08: each image's registers */
extern u_char  *g_seqImageSave[3];          /* data 0x2d9e08: 0x780 bytes per image */

void     seqVmSelectImage(int idx);                                      // ROR 0x00214310
/* Empty in the ROM. */
void     seqVmSaveImage(int idx);                                        // ROR 0x002144c4
void     seqVmLoadThread(void);                                          // ROR 0x00214664
void     seqVmSaveThread(Sequence *seq);                                 // ROR 0x002147e0
u_char  *seqVmAllocStack(void);                                          // ROR 0x002149a8
void     seqVmFreeStack(intptr_t stack);                                 // ROR 0x00214a94
int      seqVmAllocCallSave(void);                                       // ROR 0x00214ac4
void     seqVmResume(int slot);                                          // ROR 0x00214bcc
/* Runs a thread from pc until it waits; returns where it continues, 0
 * when it ended. */
intptr_t seqVmRun(intptr_t pc);                                          // ROR 0x00214c24

/* The VM's word-0 handler and its unreferenced functions. */
void     seqOpNop(u_int a, u_int b, u_int c, u_int d);                   // ROR 0x0020ff34
void     seqOpUnused211298(void);                                        // ROR 0x00211298
void     seqUnused213448(void);                                          // ROR 0x00213448
void     seqVmIdleLoop(void);                                            // ROR 0x002142c0
void     seqVmLoadRecord(Sequence *seq);                                 // ROR 0x002144cc

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
