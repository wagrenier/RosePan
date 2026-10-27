/* ==========================================================================
 *  game/gamtrg.h
 *
 *  GamArea, the area triggers (game/gamtrg.cpp 0x1dd1e8..0x1e0df0; the file
 *  name is in cloneTrg's allocDebug tag, the class name is invented):
 *  GamMan's member at +0x40 (bss 0x781300). The scripts (native 023) and
 *  GamMan's create_trg add trigger records; the listed ones with flag
 *  0x1000000 are copied into the active list, ordered by priority, and
 *  update tests them against the PC every frame, starting their "trg"
 *  threads. registFtb keeps key -> values tables a trigger can point to.
 * ======================================================================== */
#ifndef _GAME_GAMTRG_H
#define _GAME_GAMTRG_H

#include <stddef.h>
#include <stdint.h>

#include "sdk/eetypes.h"
#include "sequence/sequence.h"

/* An area trigger record (EE 0xb0, plain EE layout: the scripts pass it
 * from their image, native 023; GamArea copies it whole into a node).
 * Field roles beyond the ones named are unverified. */
struct GamAreaTrg
{
    float  m_rect[4];           // ROR 0x00: x0, z0, x1, z1; native 023 orders each pair when m_flags & 0x103
    u_int  _10[4];
    u_int  m_flags;             // ROR 0x20: native 023 sets 0x3000000; 0x1000000 activates it, 0x4000001 checkPc's, 0x20 m_ftbKey, 4 m_delay
    int    m_ftbKey;            // ROR 0x24: flag 0x20: the registFtb key activateTrg resolves into m_ftb
    int    m_delay;             // ROR 0x28: flag 4: activateTrg sets m_time = frame count + m_delay
    u_int  m_id;                // ROR 0x2c: addTrg skips a listed id; newId replaces it with a fresh oid
    u_int  m_thread;            // ROR 0x30: the "trg" thread's id (its top bits the image)
    int    m_entry;             // ROR 0x34: the thread's entry, an address in that image
    u_short m_pri;              // ROR 0x38: the active list's order (ascending, a new one after its equals)
    u_short _3a;
    int    _3c;
    int    m_time;              // ROR 0x40: see m_delay
    u_int  m_oid;               // ROR 0x44: flags 0x82c00 / 0x5000: an object; the thread's arg
    u_int  m_oid2;              // ROR 0x48: flag 0x82c00: a second object
    int    m_actor;             // ROR 0x4c: activateTrg: getActorNo(m_oid), -1 when not an object (0x5000)
    int    m_actor2;            // ROR 0x50: activateTrg: getActorNo(m_oid2)
    u_int  m_ftb;               // ROR 0x54: the ROM keeps a pointer to the ftb entry's data, 0 none.
                                //          PORT: the host keeps the entry's node index + 1 (GamArea::ftbOf)
    u_int  _58[6];
    u_int  m_70;                // ROR 0x70: non-zero: addTrg sets m_flags 0x400000; bit 2 flagPcTrg's
    u_int  _74[15];
};
static_assert(sizeof(GamAreaTrg) == 0xb0, "GamAreaTrg is 0xb0 bytes on the EE");
static_assert(offsetof(GamAreaTrg, m_flags) == 0x20, "m_flags");
static_assert(offsetof(GamAreaTrg, m_ftbKey) == 0x24, "m_ftbKey");
static_assert(offsetof(GamAreaTrg, m_delay) == 0x28, "m_delay");
static_assert(offsetof(GamAreaTrg, m_id) == 0x2c, "m_id");
static_assert(offsetof(GamAreaTrg, m_thread) == 0x30, "m_thread");
static_assert(offsetof(GamAreaTrg, m_entry) == 0x34, "m_entry");
static_assert(offsetof(GamAreaTrg, m_pri) == 0x38, "m_pri");
static_assert(offsetof(GamAreaTrg, m_time) == 0x40, "m_time");
static_assert(offsetof(GamAreaTrg, m_oid) == 0x44, "m_oid");
static_assert(offsetof(GamAreaTrg, m_oid2) == 0x48, "m_oid2");
static_assert(offsetof(GamAreaTrg, m_actor) == 0x4c, "m_actor");
static_assert(offsetof(GamAreaTrg, m_actor2) == 0x50, "m_actor2");
static_assert(offsetof(GamAreaTrg, m_ftb) == 0x54, "m_ftb");
static_assert(offsetof(GamAreaTrg, m_70) == 0x70, "m_70");

/* A registFtb table entry (EE 0x30, the data of a 0x40-byte node): a key
 * and up to nine values. layout: host -- ints only, EE offsets. */
struct GamAreaFtb
{
    int   m_key;                // ROR 0x00
    u_int m_value[9];           // ROR 0x04: [0] set on creation; registFtb scans [1..8] for a free slot
    u_int _28[2];
};
static_assert(sizeof(GamAreaFtb) == 0x30, "GamAreaFtb is 0x30 bytes on the EE");
static_assert(offsetof(GamAreaFtb, m_value) == 0x04, "m_value");

/* EE 0x18b60. layout: host -- the pools hold pointers; EE offsets in the
 * comments (each pool: free list, used list +0xd0, max +0x1a0, nodes
 * +0x1c0, 0xc0-byte nodes). */
struct GamArea
{
    GamArea();                                                           // ROR 0x001dd1e8
    ~GamArea();                                                          // ROR 0x001dd3ec

    /* A new ftb pool (0x40 nodes) into m_ftbPool, emptied. GamMan::init;
     * the ROM never frees the previous one. */
    void init();                                                         // ROR 0x001dd414
    /* A heap copy of trg (allocDebug, game/gamtrg.cpp line 0x62); no
     * callers in the ROM. */
    GamAreaTrg *cloneTrg(const GamAreaTrg *trg);                         // ROR 0x001dd558
    /* Copies trg into a node of the trigger list unless a listed one has
     * its id; newId != 0 first gives trg a fresh oid (GamMan::makeOid(0,
     * 0xd, image): image goes through in a3 untouched (-1 the current
     * stat). Flag 0x1000000 also activates it. */
    void addTrg(GamAreaTrg *trg, int newId, int image);                  // ROR 0x001dd5e8
    /* afterSeqExec's stage change kind 3: the triggers of kind (2 every
     * one but image 1's, 3 image 3's, other kinds all) back to the free
     * lists, from the trigger list and the ftb pool. */
    void stopKind(int kind);                                             // ROR 0x001dd7bc
    /* Empties the active list, the ftb pool and the trigger list. */
    void clear();                                                        // ROR 0x001ddaa8
    /* Empties the active list. */
    void stopTrgs();                                                     // ROR 0x001ddc80
    void resetCursor();                                                  // ROR 0x001dddac
    /* stopKind's selection on the active list, then resetCursor. */
    void clearKind(int kind);                                            // ROR 0x001dddbc
    /* The oid of the first fchr within 50 of pos (collectFchrsNear type
     * 5, 0x8000 / 0x4000001) whose kind is kind, among the first nine
     * with a kind; 0 none. */
    u_int findFchrOid(const float *pos, short kind);                     // ROR 0x001ddf48
    /* A copy of trg into the active list (after the ones whose m_pri is
     * not above its own), its objects, actors and ftb entry resolved. */
    void activateTrg(GamAreaTrg trg);                                    // ROR 0x001de104
    /* stopTrgs, then activates the listed triggers with flag 0x1000000. */
    void startTrgs();                                                    // ROR 0x001de4e4
    /* Adds value to key's values in the ftb pool (a new entry when the
     * key has none). */
    void registFtb(int key, u_int value);                                // ROR 0x001de6fc
    /* pos (x, z) inside the polygon of the NULL-terminated (x, z) point
     * pointers pts (crossing parity). */
    int  isInPolygon(const float *pos, const float *const *pts);         // ROR 0x001de8ac
    /* The PC's present motion again (chrMtnMove2, speed 1 / 60). */
    void startPcAction();                                                // ROR 0x001dea40
    /* Starts the "trg" thread of the first active trigger with flags
     * 0x4000001 whose rect holds the PC. */
    void checkPc();                                                      // ROR 0x001deaac
    /* ctl_trg_flg(2, id, 0x10000000) on the first active trigger with
     * flag 1, m_70 bit 2 and a thread whose rect holds the PC. No callers
     * in the ROM. */
    void flagPcTrg();                                                    // ROR 0x001dec14
    /* With GamMan flags bit 4, the PC's state class 5..7, no item-use
     * task, CROSS pressed and SQUARE / TRIANGLE / CIRCLE / START not held:
     * *flags |= 0x10 and 1. update passes a stack word it never reads. */
    int  canCheckPc(void *unused, u_int *flags);                         // ROR 0x001ded70
    /* Every frame while m_flags bit 2 is set: the triggers against the
     * PC. */
    void update(const float *pos, const float *rot);                     // ROR 0x001dee78
    /* The trigger word of the active trigger whose id matches (0 = every
     * one): op 1 sets bits, 2 ors, 3 clears, 4 tests (the first match), 5
     * tests and clears 0x20000000 (with GamMan::pause(0x8000)). */
    int  ctl_trg_flg(int op, int id, u_int bits);                        // ROR 0x001e0c2c
    /* 1 when a listed trigger has id. */
    int  hasId(u_int id);                                                // ROR 0x001e0d70

    /* PORT: m_ftb's entry (NULL when none). */
    GamAreaFtb *ftbOf(const GamAreaTrg &trg);

    SeqFixedPool<GamAreaTrg, 0x100> m_trgs;     // ROR 0x00000: the listed triggers (addTrg)
    SeqFixedPool<GamAreaTrg, 0x100> m_active;   // ROR 0x0c1c0: the active ones (used +0xc290), by m_pri
    SeqFixedPool<GamAreaTrg, 8>     m_fired;    // ROR 0x18380: update's triggers to start this frame
    SeqPool<GamAreaFtb>            *m_ftbPool;  // ROR 0x18b40: init's, 0x40 nodes new[]
    SeqNode<GamAreaTrg>            *m_cursor;   // ROR 0x18b58: activateTrg: the active list's last node; update's current one
    int                             m_18b5c;    // ROR 0x18b5c: update's (a count it compares with 2)
};

#endif /* _GAME_GAMTRG_H */
