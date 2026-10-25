// sequence/seqcall.cpp -- the script native functions (0x2053f4..0x20e3d8).
//
// The sequence scripts are MIPS code the VM (0x214c24) interprets. A jal
// below the image calls into the game: target 0x144 + n * 0x40 runs entry
// n of g_seqFuncTable with the script's a0..a3, t0, t1; v0 gets the result
// unless it is one of the SEQ_RET_* codes, which steer the thread. Entry 18
// is the system call, a switch on a0: menus (8 and 0x16 the title menu),
// items and the stock, event scenes, tasks, the fade key.
//
// RoR does not name this file (no assert in it): the table and its 512
// functions sit between system/sysnowloading.cpp and sequence/sequence.cpp
// -- sequence/ by what calls them, seqcall a guess. 471 entries are the
// same two instructions (return 0); one function stands for all of them.
//
// Written so far: the table, the empty and the return-code entries, 000,
// 005 and the system call 018. The others are stubs (seqcall.h lists them).

#include "sequence/seqcall.h"

#include <stdlib.h>
#include <string.h>

#include "common/rosepan_stub.h"
#include "game/gamman.h"
#include "lib/unassigned.h"
#include "mag/magsystem.h"
#include "sequence/sequence.h"

// ──────────────────────────────────────────────────────────────────────
// The 471 empty entries (8-byte "return 0" functions in the ROM, from
// 0x205b70 to 0x20de2c).

intptr_t seqFuncNone(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Jump: the script continues at a1, an address in its image.
// ROR 0x002053f4

intptr_t seqFunc000(intptr_t, intptr_t a1, intptr_t, intptr_t, intptr_t, intptr_t)
{
    g_sequence.jumpTo(a1);
    return SEQ_RET_JUMP;
}

// ──────────────────────────────────────────────────────────────────────
// Yield: the script continues after the call next frame.
// ROR 0x00205564

intptr_t seqFunc003(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    return SEQ_RET_YIELD;
}

// ──────────────────────────────────────────────────────────────────────
// A random number 0..a1-1 (a1 = 0 counts as 1).
// ROR 0x002058f0

intptr_t seqFunc005(intptr_t, intptr_t a1, intptr_t, intptr_t, intptr_t, intptr_t)
{
    int range = 1;
    if (a1 != 0)
        range = (int)a1;
    return rand() % range;
}

// ──────────────────────────────────────────────────────────────────────
// End: the thread stops.
// ROR 0x00209344

intptr_t seqFunc040(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    return SEQ_RET_END;
}

// ──────────────────────────────────────────────────────────────────────
// What every menu call does first (the ROM repeats it in each case): no
// menu while one is open, else halt the game for it.

static int seqBeginMenu(void)
{
    if (g_gamMan.isMenuOpen())
        return 0;
    g_gamMan.pause(0x100000);
    g_gamMan.m_flags |= 0x400;
    return 1;
}

// Starts a TskFrameFunc on GamMan's frame-task id.
static void seqStartFrameTask(int kind, intptr_t arg, int frames)
{
    TskFrameFunc *task = new TskFrameFunc(kind, arg, frames);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, &g_gamMan.m_frameTaskId, 0x11);
}

// ──────────────────────────────────────────────────────────────────────
// The system call: a0 picks the service, a1..a3 are its arguments. Item
// ids are the low half of a1.
// callcheck: pause, operator_new, TskFrameFunc (inline in the ROM: seqBeginMenu, seqStartFrameTask)
// ROR 0x0020694c

intptr_t seqFunc018(intptr_t id, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t, intptr_t)
{
    GamMan   &gam = g_gamMan;
    MagScene &scene = g_magSystem.m_scene;

    switch ((u_int)id)
    {
    // ---- the menus ----
    case 0x01:  // the item menu
        if (!seqBeginMenu())
            return 0;
        gam.enterMenu(1);
        scene.requestMenu(0x10000, 0);
        return 0;
    case 0x02:  // the map
        if (!seqBeginMenu())
            return 0;
        gam.beforeMapMenu();
        gam.enterMenu(2);
        scene.requestMenu(0x10001, 0);
        return 0;
    case 0x03:  // save
        if (!seqBeginMenu())
            return 0;
        gam.enterMenu(1);
        scene.requestMenu(0x10003, 0);
        return 0;
    case 0x04:  // load
        if (!seqBeginMenu())
            return 0;
        scene.requestMenu(0x10004, 0);
        return 0;
    case 0x05:  // the stock
        if (!seqBeginMenu())
            return 0;
        gam.enterMenu(1);
        scene.requestMenu(0x10002, 0);
        return 0;
    case 0x06:  // options
        if (!seqBeginMenu())
            return 0;
        scene.requestMenu(0x10006, 0);
        return 0;
    case 0x07:  // any menu id 0x20001..0x7fffff, the BGM faded out first
        if ((u_int)(a1 - 0x20001) > 0x7dfffe)
            return 0;
        if (!seqBeginMenu())
            return 0;
        gam.m_sndMan.stopBgm(0);
        scene.requestMenu((int)a1, 0);
        return 0;
    case 0x08:
    case 0x16:  // the title menu
        scene.fadeIn(0, 7, 0);
        scene.requestMenu(0x10005, 0);
        return 0;
    case 0x09:  // any menu id
        if (!seqBeginMenu())
            return 0;
        scene.requestMenu((int)a1, 0);
        return 0;
    case 0x0e:
        return gam.isMenuOpen() != 0;
    case 0x0f:
        if ((u_int)g_sequence.m_global[0x17] != 0)
            return (u_int)g_sequence.m_global[0x17];
        return scene.m_menu_id;

    // ---- items and the stock ----
    case 0x0a:
        gam.addItem((short)a1, 0);
        return 0;
    case 0x0b:
        gam.delItem((short)a1, 1);
        return 0;
    case 0x0c:
        return gam.hasItem((short)a1, 0);
    case 0x0d:
        gam.delItem((short)a1, 0);
        return 0;
    case 0x10:  // add the item and say so in a message window (message 0x11)
    {
        gam.addItem((short)a1, 0);
        // The ROM sets only the first of the 16 words the window copies.
        int args[16] = {0};
        args[0] = (u_short)a1;
        gam.setSysMsgHandle(scene.openSysMsgWindow(0x11, args, 1));
        return 0;
    }
    case 0x11:
        gam.addItem((short)a1, (int)a2);
        return 0;
    case 0x13:
        gam.delItemAll((short)a1);
        return 0;
    case 0x19:
        return gam.hasItem((short)a1, 1);
    case 0x3c:
        return gam.canAddItem(0, (short)a1);
    case 0x3d:
        return gam.addStock((short)a1, 0);
    case 0x3e:
        return gam.addStock((short)a1, (int)a2);
    case 0x3f:
        return gam.delStock((short)a1, 1);
    case 0x40:
        return gam.delStock((short)a1, 0);
    case 0x41:
        gam.selectItem((short)a1);
        return 0;
    case 0x42:
        return gam.getItemCount((short)a1, 1);
    case 0x43:
        gam.equipItem(0xa00);
        return 0;
    case 0x44:
        gam.equipItem(gam.m_equipItem);
        return 0;

    // ---- tasks ----
    case 0x12:  // TskFrameFunc kind 0x13 over the words a1..a3
    {
        intptr_t words[3] = {a1, a2, a3};
        seqStartFrameTask(0x13, (intptr_t)words, 0);
        return 0;
    }
    case 0x22:  // a TskSysFunc for record a1 of the word table, replacing the running one
    {
        if (g_wrdTable.get((int)a1) && !(gam.m_flags2 & 0x2000))
        {
            if (!(gam.m_flags2 & 0x4000))
            {
                if (g_taskMan.getTask(gam.m_sysTaskId))
                {
                    g_taskMan.killTask(gam.m_sysTaskId);
                    gam.m_sysTaskId = -1;
                }
            }
            else
            {
                gam.m_flags2 &= ~0x4000u;
            }
        }
        TskSysFunc *task = new TskSysFunc(1, 0, a1);
        g_taskMan.addTask(1, -1, 1, 0, task, 0, &gam.m_sysTaskId, 0x11);
        return 0;
    }
    case 0x65:
        seqStartFrameTask(7, a1, 0);
        return 0;
    case 300:
        seqStartFrameTask(0xf, a1, 0x1e);
        return 0;
    case 0x12d:
        seqStartFrameTask(0x10, 0, 0x1e);
        return 0;
    case 999:
        if (g_taskMan.getTask(gam.m_frameTaskId))
        {
            g_taskMan.killTask(gam.m_frameTaskId);
            gam.m_frameTaskId = -1;
        }
        return 0;

    // ---- the stage, the PC and the sequence ----
    case 0x14:
        gam.setImage((int)a1, (u_char *)a2);
        return 0;
    case 0x15:
        gam.m_seqWord = a1;
        return 0;
    case 0x1e:
        g_sequence.m_base[a1] = a2;
        return 0;
    case 0x1f:
        gam.setPcStartPos();
        return 0;
    case 0x21:
        gam.m_area.checkPc();
        return 0;
    case 0x29:
        gam.m_area.startPcAction();
        return 0;
    case 0x2a:  // wait until the PC's motion is no longer m_pcMotion
        if (scene.getActorMotion(gam.m_pcActor) != gam.m_pcMotion)
        {
            gam.resume(4);
            return 0;
        }
        return SEQ_RET_WAIT;
    case 0x32:
        gam.m_seqPair[1].m_b = (int)a2;
        gam.m_seqPair[1].m_a = (int)a1;
        return 0;
    case 0x33:
        gam.m_seqPair[2].m_b = (int)a2;
        gam.m_seqPair[2].m_a = (int)a1;
        return 0;
    case 0x34:
        gam.m_seqPair[3].m_b = (int)a2;
        gam.m_seqPair[3].m_a = (int)a1;
        return 0;
    case 0x35:
        gam.m_seqPair[0].m_b = (int)a2;
        gam.m_seqPair[0].m_a = (int)a1;
        g_sequence.m_global[0x64] = (int)a3;
        return 0;
    case 0x50:
        gam.getStatus((int)a1, (void *)a2);
        return 0;
    case 0x66:
        gam.storePcTransform();
        return 0;
    case 0x67:
        memcpy(gam.m_sysWord, (const void *)a1, sizeof(gam.m_sysWord));
        return 0;

    // ---- event scenes, vibration, the fade key ----
    case 0x46:
        return scenePlayStart((const char *)a1, a2, 1);
    case 0x47:
        scenePlayStart((const char *)a1, a2, 0);
        return 0;
    case 0x48:
        scenePlayStartSimple((const char *)a1);
        return 0;
    case 0x5a:
        if ((u_int)a1 < 0x3d)
            g_magSystem.startVibration((int)a1);
        else
            g_magSystem.startVibration(0xf);
        return 0;
    case 0xc8:
        scene.openFadeKey((const char *)a1);
        return 0;
    case 0xc9:
        scene.setFadeKeyMagnify(*(const float *)a1, (int)a2);
        return 0;
    case 0xca:
        scene.removeFadeKey();
        return 0;

    default:    // 0, 0x28, 0x12e and every other id
        return 0;
    }
}

// The table (data 0x2d90f8).
const SeqFunc g_seqFuncTable[SEQ_FUNC_COUNT] = {
    /*   0 */ seqFunc000, seqFunc001, seqFunc002, seqFunc003,
    /*   4 */ seqFunc004, seqFunc005, seqFunc006, seqFunc007,
    /*   8 */ seqFuncNone, seqFunc009, seqFuncNone, seqFuncNone,
    /*  12 */ seqFunc012, seqFuncNone, seqFunc014, seqFuncNone,
    /*  16 */ seqFunc016, seqFuncNone, seqFunc018, seqFunc019,
    /*  20 */ seqFunc020, seqFunc021, seqFuncNone, seqFunc023,
    /*  24 */ seqFuncNone, seqFuncNone, seqFunc026, seqFunc027,
    /*  28 */ seqFunc028, seqFunc029, seqFuncNone, seqFuncNone,
    /*  32 */ seqFunc032, seqFunc033, seqFuncNone, seqFuncNone,
    /*  36 */ seqFunc036, seqFuncNone, seqFunc038, seqFunc039,
    /*  40 */ seqFunc040, seqFuncNone, seqFuncNone, seqFuncNone,
    /*  44 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /*  48 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /*  52 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /*  56 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /*  60 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /*  64 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /*  68 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /*  72 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /*  76 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /*  80 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /*  84 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /*  88 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /*  92 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /*  96 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 100 */ seqFunc100, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 104 */ seqFuncNone, seqFuncNone, seqFunc106, seqFuncNone,
    /* 108 */ seqFuncNone, seqFuncNone, seqFunc110, seqFuncNone,
    /* 112 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 116 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 120 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 124 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 128 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 132 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 136 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 140 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 144 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 148 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFunc151,
    /* 152 */ seqFunc152, seqFuncNone, seqFuncNone, seqFunc155,
    /* 156 */ seqFunc156, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 160 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 164 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 168 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 172 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 176 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 180 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 184 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 188 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 192 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 196 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 200 */ seqFunc200, seqFuncNone, seqFunc202, seqFuncNone,
    /* 204 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 208 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFunc211,
    /* 212 */ seqFunc212, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 216 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 220 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 224 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 228 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 232 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 236 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 240 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 244 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 248 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 252 */ seqFunc252, seqFunc253, seqFuncNone, seqFuncNone,
    /* 256 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 260 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 264 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 268 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 272 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 276 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 280 */ seqFuncNone, seqFunc281, seqFuncNone, seqFuncNone,
    /* 284 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 288 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 292 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 296 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 300 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 304 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 308 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 312 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 316 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 320 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 324 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 328 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 332 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 336 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 340 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 344 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 348 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 352 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 356 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 360 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 364 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 368 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 372 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 376 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 380 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 384 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 388 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 392 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 396 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 400 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 404 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 408 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 412 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 416 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 420 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 424 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 428 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 432 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 436 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 440 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 444 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 448 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 452 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 456 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 460 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 464 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 468 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 472 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 476 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 480 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 484 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 488 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 492 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 496 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 500 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 504 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
    /* 508 */ seqFuncNone, seqFuncNone, seqFuncNone, seqFuncNone,
};

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x0020541c
intptr_t seqFunc001(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc001", 0x0020541c);
    return {};
}

// stub of ROR 0x0020544c
intptr_t seqFunc002(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc002", 0x0020544c);
    return {};
}

// stub of ROR 0x00205570
intptr_t seqFunc004(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc004", 0x00205570);
    return {};
}

// stub of ROR 0x00205928
intptr_t seqFunc006(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc006", 0x00205928);
    return {};
}

// stub of ROR 0x00205b18
intptr_t seqFunc007(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc007", 0x00205b18);
    return {};
}

// stub of ROR 0x00205b44
intptr_t seqFunc009(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc009", 0x00205b44);
    return {};
}

// stub of ROR 0x00205b88
intptr_t seqFunc012(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc012", 0x00205b88);
    return {};
}

// stub of ROR 0x00206614
intptr_t seqFunc014(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc014", 0x00206614);
    return {};
}

// stub of ROR 0x00206640
intptr_t seqFunc016(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc016", 0x00206640);
    return {};
}

// stub of ROR 0x002076cc
intptr_t seqFunc019(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc019", 0x002076cc);
    return {};
}

// stub of ROR 0x00207ae8
intptr_t seqFunc020(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc020", 0x00207ae8);
    return {};
}

// stub of ROR 0x0020810c
intptr_t seqFunc021(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc021", 0x0020810c);
    return {};
}

// stub of ROR 0x0020839c
intptr_t seqFunc023(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc023", 0x0020839c);
    return {};
}

// stub of ROR 0x00208518
intptr_t seqFunc026(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc026", 0x00208518);
    return {};
}

// stub of ROR 0x00208640
intptr_t seqFunc027(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc027", 0x00208640);
    return {};
}

// stub of ROR 0x00208b10
intptr_t seqFunc028(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc028", 0x00208b10);
    return {};
}

// stub of ROR 0x00208b98
intptr_t seqFunc029(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc029", 0x00208b98);
    return {};
}

// stub of ROR 0x00208dc4
intptr_t seqFunc032(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc032", 0x00208dc4);
    return {};
}

// stub of ROR 0x00208e24
intptr_t seqFunc033(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc033", 0x00208e24);
    return {};
}

// stub of ROR 0x002090a0
intptr_t seqFunc036(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc036", 0x002090a0);
    return {};
}

// stub of ROR 0x002090e0
intptr_t seqFunc038(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc038", 0x002090e0);
    return {};
}

// stub of ROR 0x00209270
intptr_t seqFunc039(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc039", 0x00209270);
    return {};
}

// stub of ROR 0x00209528
intptr_t seqFunc100(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc100", 0x00209528);
    return {};
}

// stub of ROR 0x002098e0
intptr_t seqFunc106(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc106", 0x002098e0);
    return {};
}

// stub of ROR 0x002099bc
intptr_t seqFunc110(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc110", 0x002099bc);
    return {};
}

// stub of ROR 0x00209d4c
intptr_t seqFunc151(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc151", 0x00209d4c);
    return {};
}

// stub of ROR 0x0020a368
intptr_t seqFunc152(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc152", 0x0020a368);
    return {};
}

// stub of ROR 0x0020a7d0
intptr_t seqFunc155(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc155", 0x0020a7d0);
    return {};
}

// stub of ROR 0x0020abfc
intptr_t seqFunc156(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc156", 0x0020abfc);
    return {};
}

// stub of ROR 0x0020b3c0
intptr_t seqFunc200(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc200", 0x0020b3c0);
    return {};
}

// stub of ROR 0x0020b528
intptr_t seqFunc202(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc202", 0x0020b528);
    return {};
}

// stub of ROR 0x0020bd08
intptr_t seqFunc211(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc211", 0x0020bd08);
    return {};
}

// stub of ROR 0x0020c4c8
intptr_t seqFunc212(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc212", 0x0020c4c8);
    return {};
}

// stub of ROR 0x0020c6fc
intptr_t seqFunc252(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc252", 0x0020c6fc);
    return {};
}

// stub of ROR 0x0020d428
intptr_t seqFunc253(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc253", 0x0020d428);
    return {};
}

// stub of ROR 0x0020df98
intptr_t seqFunc281(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    ROSEPAN_STUB("seqFunc281", 0x0020df98);
    return {};
}
