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
extern int g_data_296198;       /* data 0x296198: main sets 2 */

/* bss 0x7812ac, just below GamMan: mainInitGame passes it to
 * seqSetLanguage (which ignores it). Role not known yet. */
extern int g_bss_7812ac;

/* ---- the debug range before debug/mimu_debug.cpp (unassigned@0020eda4) ---- */

void debugNop(void);                                                     // ROR 0x002155a0

#endif /* _LIB_UNASSIGNED_H */
