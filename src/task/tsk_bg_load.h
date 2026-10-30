/* ==========================================================================
 *  task/tsk_bg_load.h
 *
 *  task/tsk_bg_load.cpp (pinned 0x1fce0c..0x1fd424 and the linkonce
 *  symbols 0x290c1c..0x290e88; RoR's source string anchors the allocDebug
 *  calls in 0x1fd008 / 0x1fd2dc): background pack loaders, tasks that read
 *  a whole pack file into a new block a few steps at a time while the game
 *  runs. Each reads the pack's first 0x800 bytes, takes the total size from
 *  its RTPK header (BedPfh), allocates that much (rounded to 2048) and reads
 *  the whole file into it, storing the block in the caller's word.
 *  - TskBGLoadArea: a pack by name; when the heap is full it releases the
 *    stage's objects (GamMan::releaseObjs(2, 1)) and tries once more, and
 *    stores NULL when that fails too.
 *  - TskBGLoadMap: data/bg/mapunit.bin; on a failed allocation it ends
 *    without storing anything.
 *  Nothing in the ROM constructs either (no reference to their
 *  constructors or vtables outside them): a leftover of streamed areas.
 * ======================================================================== */
#ifndef _TASK_TSK_BG_LOAD_H
#define _TASK_TSK_BG_LOAD_H

#include <stddef.h>
#include <stdint.h>

#include "game/gamfile.h"
#include "sdk/eetypes.h"
#include "task/tsk_man.h"

/* TskBGLoad : CmnITask (vtable 0x2e5000; kind 0x11): a FileReader and
 * the request it runs, plus a buffer for the pack's first 0x800 bytes.
 * EE 0x880 bytes. layout: host -- vptr, FileReader holds a pointer; EE
 * offsets in the comments. */
struct TskBGLoad : CmnITask
{
    TskBGLoad() {}                                                       // ROR 0x00290c6c
    ~TskBGLoad() override {}                                             // ROR 0x00290c94

    int  get_task_type() override { return 0x11; }                      // ROR 0x00290cf8
    void init() override {}                                              // ROR 0x001fce0c
    /* Runs the request: non-zero (end) once the read is done. */
    int  exec(int id, int time, int arg) override;                       // ROR 0x001fce1c
    void term() override {}                                              // ROR 0x001fce14

    /* Starts an asynchronous read of size bytes at offset into buf, from
     * name when open is set (the request opens and closes the file).
     * Returns 0. */
    int  startRead(const char *name, void *buf, int size, int offset, int open); // ROR 0x001fce40

    /* The first 64-byte boundary in m_head after its first byte: the ROM's
     * (this & ~63) + 0x80, 0x800 bytes for the pack's head. */
    void *getHead()
    {
        return (void *)(((uintptr_t)m_head + 64) & ~(uintptr_t)63);
    }

    FileReader  m_reader;       // ROR 0x04
    FileReadReq m_req;          // ROR 0x0c (to 0x30)
    int         m_30;           // ROR 0x30: cleared by startRead, never read
    u_char      m_head[0x840];  // ROR 0x40: the 64-aligned 0x800-byte head inside it
};

/* TskBGLoadArea : TskBGLoad (vtable 0x2e4fb8): loads the pack name into a
 * new block, stored to *out. EE 0x988 bytes. layout: host. */
struct TskBGLoadArea : TskBGLoad
{
    TskBGLoadArea(const char *name, void **out);                         // ROR 0x001fcef8
    ~TskBGLoadArea() override {}                                         // ROR 0x00290d50

    int  get_task_type() override { return 0x11; }                      // ROR 0x00290db4
    int  exec(int id, int time, int arg) override;                       // ROR 0x001fd008
    void term() override {}                                              // ROR 0x00290dbc
    /* Keeps name and out and starts the head read (step 1). */
    virtual void start(const char *name, void **out);                   // EE slot 7 (+0x38), ROR 0x001fcf6c

    char   m_name[0x100];       // ROR 0x880
    int    m_step;              // ROR 0x980: 0 idle, 1 reading the head, 2 reading the pack
    void **m_out;               // ROR 0x984: gets the block (NULL when no memory)
};

/* TskBGLoadMap : TskBGLoad (vtable 0x2e4f70): loads data/bg/mapunit.bin
 * into a new block, stored to *out. layout: host. */
struct TskBGLoadMap : TskBGLoad
{
    TskBGLoadMap(const char *name, void **out);                          // ROR 0x001fd200
    ~TskBGLoadMap() override {}                                          // ROR 0x00290e14

    int  get_task_type() override { return 0x11; }                      // ROR 0x00290e78
    int  exec(int id, int time, int arg) override;                       // ROR 0x001fd2dc
    void term() override {}                                              // ROR 0x00290e80
    /* Keeps out and starts the head read (step 1); name is not read. */
    virtual void start(const char *name, void **out);                   // EE slot 7 (+0x38), ROR 0x001fd264

    int    m_step;              // ROR 0x880: 0 idle, 1 reading the head, 2 reading the pack
    int    m_884;               // ROR 0x884: not written
    void **m_out;               // ROR 0x888: gets the block
};

#endif /* _TASK_TSK_BG_LOAD_H */
