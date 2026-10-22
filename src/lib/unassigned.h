/* ==========================================================================
 *  lib/unassigned.h
 *
 *  Prototypes of RoR code whose source file is not decided yet (unassigned
 *  runs in ror_functions.txt), declared so reconstructed callers compile and
 *  stubgen can stub them into lib/unassigned_stubs.cpp. Move each block to
 *  its real header when a session defines its TU.
 * ======================================================================== */
#ifndef _LIB_UNASSIGNED_H
#define _LIB_UNASSIGNED_H

/* ---- the globals file before main.cpp (unassigned@001fd424): its static
 * initializer 0x2041f8 constructs the heap, MagSystem, GamMan, the task
 * manager, the sequence, EmyManager, SndMan and 0x7b40c0 ---- */

void sysInitManagers(void);                                              // ROR 0x00204148

/* A frame timer (the task manager's member at +0x40). EE size at least
 * 0x2c. layout: host -- no members reconstructed. */
struct SysTimer
{
    void start();                                                        // ROR 0x00203ea8
    void read();                                                         // ROR 0x00203f38
    void update();                                                       // ROR 0x00204014
};

/* ---- the task framework (unassigned@001e46b8) ---- */

/* The task interface 32 classes derive from (no vtable of its own in the
 * ELF). layout: host -- no members reconstructed. */
struct CmnITask
{
};

/* EmyManager : CmnITask, one global at bss 0x7b2140 (constructor
 * 0x16265c). layout: host -- no members reconstructed. */
struct EmyManager : CmnITask
{
};

/* The task manager, one global at bss 0x7a3d80. EE size at least 0x288.
 * layout: host -- only the timer member is placed (EE +0x40). */
struct CmnTaskMan
{
    int  addTask(int group, int start, int prio, int frames, CmnITask *task, int arg,
                 int *id, int user);                                     // ROR 0x001e71fc
    void exec();                                                         // ROR 0x001e7a20

    SysTimer m_timer;           // ROR 0x40 (bss 0x7a3dc0)
};

extern CmnTaskMan g_taskMan;    /* bss 0x7a3d80 */
extern EmyManager g_emyManager; /* bss 0x7b2140 */
extern int g_emyManagerTaskId;  /* bss 0x7b45c0: addTask's id for EmyManager (read by 0x1c58c0) */

/* Words main sets once the boot load is over; the game modes (0x19a824,
 * 0x1a92a0, 0x1b099c, 0x1bde58, 0x1a98fc) read and write them. Roles not
 * known yet. */
extern int g_bss_7a3d48;        /* bss 0x7a3d48: main sets 1 */
extern int g_bss_7a3d4c;        /* bss 0x7a3d4c: main sets 1 */

/* The object at bss 0x7b40c0 (constructor 0x1810f4: sizes 0x200 / 0x180 /
 * 0xe00, between snd/sndman.cpp and snd/rxmpeg.cpp): a movie player, class
 * name unverified. layout: host -- only the words MagSystem uses. */
struct RxMpeg
{
    void setFrameParity(int parity);                                     // ROR 0x00181204

    int m_active;               // ROR 0x00: cleared by the constructor; MagSystem::waitVSync runs rxThVSync while set
};

extern RxMpeg g_rxMpeg;         /* bss 0x7b40c0 */

/* ---- the utl range after utl/utlgame.cpp (unassigned@00103edc) ---- */

void utlRandReset(void);                                                 // ROR 0x0010421c

/* ---- the i3d library (unassigned@00216ad0 .. 0x237d80; its version string
 * "i3d_package_version 050121-stable-050826") ---- */

/* The context and the VRAM cache have their own files (i3d/i3dctx.h,
 * i3d/i3dvram.h); what they call in the rest of the library is below. */
#include "i3d/i3dctx.h"

int   i3dPrintf(const char *fmt);  /* printf-like in the ROM */          // ROR 0x00237c68
void  i3dSysInit(void);            /* empty in the ROM */                // ROR 0x00237178
int   i3dFlushCache(void);         /* FlushCache(0) */                   // ROR 0x00237cc8

/* GS transfer packets for the VRAM cache (block addresses: 64-word units). */
u_long *i3dTexMakeUpload(void *tex, u_long *pkt, u_int dst);             // ROR 0x00219c88
u_long *i3dClutMakeUpload(void *clut, u_long *pkt, u_int dst);           // ROR 0x00219d10
u_long *i3dTexMakeMove(void *tex, u_long *pkt, u_int src, u_int dst);    // ROR 0x00219d40
u_long *i3dClutMakeMove(void *clut, u_long *pkt, u_int src, u_int dst);  // ROR 0x00219d68

/* The packet's first tags (0x21b600 / 0x21b550); returns the chain cursor. */
u_long *i3dPktOpenChain(u_char *pkt);                                    // ROR 0x0021b698
/* Writes the no-texture registers into each used slot of a draw set. */
int   i3dBatchClearTextures(I3dDrawSet *set);                            // ROR 0x0021b288

/* The draw groups and layers (0x2239b8..0x224d88). */
int   i3dLayerInit(I3dLayer *layer, i3dContext *ctx);                    // ROR 0x00223878
int   i3dBatchAppend(I3dGroup *dst, I3dGroup *src);                      // ROR 0x00223d48
I3dNode *i3dSortList(I3dNode *first, int order, int b, u_int count);     // ROR 0x00224368
int   i3dGroupInit(I3dGroup *group, i3dContext *ctx, int a, int b, int c); // ROR 0x002243a0
int   i3dCtxRunGroup(i3dContext *ctx, I3dGroup *group);                  // ROR 0x002246c8
int   i3dCtxCloseFrame(i3dContext *ctx);                                 // ROR 0x00224798
void  i3dGroupAddLayer(I3dGroup *group, u_int key, I3dGroup *layer);     // ROR 0x00224878
int   i3dTexRegsSet(u_long *regs, I3dVramHandle *tex, I3dVramHandle *clut,
                    u_int arg);                                          // ROR 0x00224b28
void  i3dTexRegsSetNone(u_long *regs);                                   // ROR 0x00224d88

/* An i3d model root (tag_I3rRoot_s in DogModel's symbols). Opaque. */
struct I3rRoot;

/* i3d's matrix struct (tag_I3dMatrixF4x4_s in DogModel's symbols): the
 * same 16 floats as an sceVu0FMATRIX, but the ROM copies it with
 * unaligned ldl / sdl, so it is only 4-aligned. EE 0x40. */
struct I3dMatrixF4x4
{
    float m[4][4];
};
static_assert(sizeof(I3dMatrixF4x4) == 0x40, "I3dMatrixF4x4 is 0x40 bytes on the EE");

/* The node index of the named node, -1 when the model has none. */
int   i3rGetNodeIndexByName(I3rRoot *root, const char *name);            // ROR 0x0022b3d8

/* bss 0x7812ac, just below GamMan: mainInitGame passes it to
 * seqSetLanguage (which ignores it). Role not known yet. */
extern int g_bss_7812ac;

/* ---- the debug range before debug/mimu_debug.cpp (unassigned@0020eda4) ---- */

void debugNop(void);                                                     // ROR 0x002155a0

/* ---- after the assert handler 0x215a30 (unassigned@0020ff34) ---- */

/* Formats the three floats (3 digits) and prints nothing: a trace whose
 * output the retail build compiled out. */
void dbgTraceVector3(float x, float y, float z);                         // ROR 0x00215b5c

#endif /* _LIB_UNASSIGNED_H */
