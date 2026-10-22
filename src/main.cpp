/* ==========================================================================
 *  main.cpp
 *
 *  Rule of Rose's main and its file (ROR 0x2043dc..0x204d38; see main.h).
 *  main brings the console up through the rx middleware -- the IOP image
 *  and boot path, rxSysInit, the game heap and the memory hooks -- then
 *  mainInitGame starts the devices, the GS and the managers and loads the
 *  common data and the first stage behind the now-loading screen, and
 *  mainLoop runs the game for ever.
 *
 *  PC: the process entry at the bottom opens the log and runs the game's
 *  main on the game thread (RosePan_HostMain), with the window on this one.
 *  The ROM's main is mainGame here, since the host owns the name main.
 * ======================================================================== */

#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/gamman.h"
#include "lib/unassigned.h"
#include "mag/magsystem.h"
#include "rx/rxsys.h"
#include "rx/rxthread.h"
#include "sdk/eekernel.h"
#include "sdk/eeregs.h"
#include "sdk/libdbc.h"
#include "sdk/libdma.h"
#include "sdk/libgraph.h"
#include "sdk/libmc.h"
#include "sdk/libpad2.h"
#include "sequence/sequence.h"
#include "snd/sndman.h"
#include "system/sysnowloading.h"
#include "utl/utlmisc.h"

#include "rosepan/io/rosepan_log.h"
#include "rosepan/os/rosepan_host.h"
#include "rosepan/rendering/rosepan_renderer.h"
#include "rosepan/rosepan_memory.h"

/* ---- data ----------------------------------------------------------------- */

static char           s_version[0x40];      /* bss 0x7b45d0: "0.1.<number>n.<date>.<time>" */
static int            s_mainThreadId;       /* sdata 0x2d8ff8 */
SysNowLoading        *g_nowLoading;         /* sdata 0x2d8ffc */

/* mainExceptionHandler's register rows (data 0x2d9000). */
static const MainRegRow s_regRows[] = {
    { "\nat/v0-1    ",  1, 3, 0 },
    { "\na0-3       ",  4, 4, 0 },
    { "\nt0-3       ",  8, 4, 0 },
    { "\nt4-9       ", 12, 4, 0 },
    { "",              24, 2, 0 },
    { "\ns0-8       ", 16, 4, 0 },
    { "\ns4-7/k0-1  ", 20, 4, 0 },
    { "",              26, 2, 0 },
    { "\ngp/sp/fp/ra", 28, 4, 0 },
    { NULL,             0, 0, 0 },
};

static int                 s_inException;   /* sdata 0x2d90a0 */
static int                 s_oldHandlers[14]; /* data 0x2d90a8: SetDebugHandler's results by cause, 1..7 and 10..13 */
static int                 s_handlersSet;   /* sdata 0x2d90e8 */

// ──────────────────────────────────────────────────────────────────────
// The command line: "-l fre|ger|spa|ita" picks the language, "-m N" or
// "-mN" the stage to load at boot; a bare number is read and dropped.
// Returns the stage, 99 when none is given.
// ROR 0x002043dc

int mainParseArgs(int argc, char **argv)
{
    int stage = 99;
    int opt   = 0;

    for (int n = argc - 1; n > 0; n--)
    {
        char *arg = *++argv;

        if (opt == 'l')
        {
            if (strcmp(arg, "fre") == 0)
                g_magSystem.setLanguage(1);
            else if (strcmp(arg, "ger") == 0)
                g_magSystem.setLanguage(2);
            else if (strcmp(arg, "spa") == 0)
                g_magSystem.setLanguage(3);
            else if (strcmp(arg, "ita") == 0)
                g_magSystem.setLanguage(4);
            opt = 0;
        }
        else if (opt == 0)
        {
            if (arg[0] == '-')
            {
                if (arg[1] == 'l')
                    opt = 'l';
                else if (arg[1] == 'm')
                {
                    if (arg[2] != '\0')
                        stage = atoi(arg + 2);
                    else
                        opt = 'm';
                }
            }
            else if ((u_char)(arg[0] - '0') < 10)
            {
                atoi(arg);
            }
        }
        else if (opt == 'm' && (u_int)((u_char)arg[0] - '0') < 10)
        {
            opt   = 0;
            stage = atoi(arg);
        }
    }
    return stage;
}

// ──────────────────────────────────────────────────────────────────────
// The game's main: the version line, the IOP image and boot path, the
// rx system and the game heap behind its memory hooks, then the game.
// PC: named mainGame (the host's process entry is main, below).
// callcheck: __main (GCC 2's static constructors: the host runs them before main)
// ROR 0x002045a8

int mainGame(int argc, char **argv)
{
    int stage = mainParseArgs(argc, argv);

    // The build's __DATE__ and __TIME__.
    snprintf(s_version, sizeof(s_version), "0.1.%sn.%s.%s", g_versionNumber, "Jul 27 2006",
             "22:25:45");
    rxSysSetIopImage("IOPRP300.IMG");
    // PC: the ROM boots from "cdrom0:"; the port reads the extracted disc's
    // loose files through HOST (open-items.md "Decisions", boot from host0).
    static char bootPath[] = "host0:";
    rxSysSetBootPath(bootPath);
    s_mainThreadId = GetThreadId();
    ChangeThreadPriority(s_mainThreadId, 5);
    rxSysInit();
    utlMemInit();
    rxSysSetMemFunc0((RxSysMemFunc)utlMemAlloc);
    rxSysSetMemFunc1((RxSysMemFunc)utlMemAlloc);
    rxSysSetMemFunc2((RxSysMemFunc)utlMemRealloc);
    rxSysSetMemFunc3((RxSysMemFunc)utlMemFree);
    strcpy(g_sndMan.m_bootName, argv[0]);
    rxSysSetDebugHandlers();
    // PC: the ROM passes 1 (CDROM); the boot device (-1) keeps the files on
    // HOST, where the disc's files are.
    rxSysInitFileDevice(-1);
    mainInitGame(stage);
    mainLoop();
    mainExit();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Devices, GS, managers; then, behind the now-loading screen, the common
// data and the boot stage (none for -1). The now-loading object is freed
// before the game starts; the two double-buffer resets restore the game's
// 512x448 buffers it replaced.
// callcheck: operator_new, operator_delete (the host's new / delete of SysNowLoading)
// ROR 0x002046d4

void mainInitGame(int stage)
{
    mainInitDevices();
    mainInitGraph();
    sysInitManagers();
    g_magSystem.init();
    g_nowLoading = new SysNowLoading;
    if (g_nowLoading != NULL)
    {
        g_nowLoading->init();
        g_magSystem.setSceneFlags(1, 1, 1, 1, 1, 1, 0);
        g_magSystem.m_scene.initCamera();
        g_magSystem.start();
        seqSetLanguage(g_bss_7812ac, 1);
        g_gamMan.loadSystemData();
        if (stage != -1)
            g_gamMan.loadStage(stage);
        g_nowLoading->term();
        delete g_nowLoading;    // ~SysNowLoading calls term again
        g_bss_7a3d4c = 1;
        g_bss_7a3d48 = 1;
        g_nowLoading = NULL;
        g_magSystem.resetDBuff();
        MagSystem::m_v_interval = 2;
        g_magSystem.resetDBuff();
    }
}

// ──────────────────────────────────────────────────────────────────────
// After the loop (never reached): the system object and the devices.
// ROR 0x0020483c

void mainExit(void)
{
    g_magSystem.term();
    mainExitDevices();
}

// ──────────────────────────────────────────────────────────────────────
// The frame loop: EmyManager runs as a task from the first frame; each
// frame restarts timer 0 (H-blanks, MagSystem::update measures with it),
// then sequence, game, tasks, system update and the game's post step.
// ROR 0x00204864

void mainLoop(void)
{
    g_taskMan.m_timer.start();
    g_taskMan.addTask(1, -1, 1, 0, &g_emyManager, 0, &g_emyManagerTaskId, 0);
    REG_RCNT0_MODE = 0x83;      // H-blank clock, counting
    for (;;)
    {
        // PC: the renderer's frame (ReiPan's pre_super / SendDMAMain hooks).
        RosePan_RendererBeginFrame();
        REG_RCNT0_COUNT = 0;
        g_taskMan.m_timer.update();
        g_sequence.exec();
        g_gamMan.update();
        g_taskMan.exec();
        g_magSystem.update();
        g_gamMan.postUpdate();
        RosePan_RendererEndFrame();
    }
}

// ──────────────────────────────────────────────────────────────────────
// NTSC interlaced frame mode and a 640x224 double buffer whose two draw
// environments are put once.
// ROR 0x0020491c

void mainInitGraph(void)
{
    sceGsDBuff db;

    // inter 1 is interlaced in Sony's SDK (sdk/libgraph.h has
    // SCE_GS_INTERLACE / SCE_GS_NOINTERLACE swapped: hazards.md).
    sceGsResetGraph(0, 1, SCE_GS_NTSC, SCE_GS_FRAME);
    sceGsSetDefDBuff(&db, SCE_GS_PSMCT32, 640, 224, 2 /* ZTST GEQUAL */, SCE_GS_PSMZ24, 1);
    FlushCache(0);
    rxThSync(0);
    sceGsPutDrawEnv(&db.giftag0);
    sceGsSyncPath(0, 0);
    sceGsPutDrawEnv(&db.giftag1);
    sceGsSyncPath(0, 0);
}

// ──────────────────────────────────────────────────────────────────────
// Sound, controller ports, pads, memory card.
// ROR 0x0020499c

void mainInitDevices(void)
{
    g_sndMan.init();
    sceDbcInit();
    scePad2Init(0);
    sceMcInit();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002049d4

void mainExitDevices(void)
{
    sceMcEnd();
    scePad2End();
    sceDbcEnd();
}

// ──────────────────────────────────────────────────────────────────────
// The debug exception handler: once, reset the VIFs, VUs, GS path and DMA,
// switch to the rx exception screen, print epc / status / cause, the bad
// addresses, the registers by row and 14 lines of the stack, then hang.
// ROR 0x00204a00

void mainExceptionHandler(u_int stat, u_int cause, u_int epc, u_int bva, u_int bpa,
                          u_long128 *gpr)
{
    if (s_inException != 0)
        return;
    s_inException = 1;
    debugNop();
    sceDevVif0Reset();
    sceDevVu0Reset();
    sceDevVif1Reset();
    sceDevVu1Reset();
    sceGsResetPath();
    sceDmaReset(1);
    rxSysInitExceptionScreen();
    rxSysPrintf("epc     = %08x  status  = %08x  cause   = %08x\n", epc, stat, cause);
    rxSysPrintf("badvadr = %08x  badpadr = %08x\n", bva, bpa);
    for (const MainRegRow *row = s_regRows; row->m_name != NULL; row++)
    {
        rxSysPrintf("%s", row->m_name);
        for (int i = 0; i < row->m_count; i++)
            rxSysPrintf(" %08x", *(u_int *)&gpr[row->m_first + i]);
    }
    rxSysPrintf("\n\n");
    // PC: the stack pointer is an EE address; read it through emulated RAM.
    u_int sp = *(u_int *)&gpr[29];
    for (int line = 13; line >= 0; line--, sp += 0x10)
    {
        if (sp > 0xfffff && (int)sp <= rxSysGetMemorySize() - 0x11)
        {
            u_int *p = (u_int *)RosePan_GetHostPointer(sp);
            rxSysPrintf("%08x: %08x %08x %08x %08x\n", sp, p[0], p[1], p[2], p[3]);
        }
    }
    for (;;)
    {
    }
}

// ──────────────────────────────────────────────────────────────────────
// Once: mainExceptionHandler for causes 1-7 and 10-13, keeping the old
// handlers. Nothing in the retail ELF calls it.
// ROR 0x00204bf8

void mainSetExceptionHandlers(void)
{
    if (s_handlersSet != 0)
        return;
    s_handlersSet = 1;
    for (int cause = 1; cause < 8; cause++)
        s_oldHandlers[cause] = SetDebugHandler(cause, mainExceptionHandler);
    for (int cause = 10; cause < 14; cause++)
        s_oldHandlers[cause] = SetDebugHandler(cause, mainExceptionHandler);
}

// ──────────────────────────────────────────────────────────────────────
// Puts the saved handlers back. Nothing calls it.
// ROR 0x00204ca4

void mainResetExceptionHandlers(void)
{
    if (s_handlersSet == 0)
        return;
    s_handlersSet = 0;
    // PC: the EE kernel returns the old handler's address; the host returns
    // 0, so this restores none.
    for (int cause = 1; cause < 8; cause++)
        SetDebugHandler(cause, (RosePanDebugHandler)(intptr_t)s_oldHandlers[cause]);
    for (int cause = 10; cause < 14; cause++)
        SetDebugHandler(cause, (RosePanDebugHandler)(intptr_t)s_oldHandlers[cause]);
}

/* ---- PC: the process entry ------------------------------------------------- */

static int    s_argc;
static char **s_argv;

// PC: the game thread's body. argv[0] is the ROM's boot file, not the host
// executable's path (main copies it into a 0x20-byte field); the host's
// other arguments pass through, so "-m N" picks the boot stage.
static int GameMain(void)
{
    static char  bootName[] = "cdrom0:\\SLUS_214.48;1";
    static char *argv[16];
    int          argc = 0;

    argv[argc++] = bootName;
    for (int i = 1; i < s_argc && argc < 15; i++)
        argv[argc++] = s_argv[i];
    argv[argc] = NULL;
    return mainGame(argc, argv);
}

int main(int argc, char **argv)
{
    s_argc = argc;
    s_argv = argv;
    RosePan_LogInit();
    return RosePan_HostMain(GameMain);
}
