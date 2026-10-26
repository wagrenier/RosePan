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
// 001, 002, 004 (wait until), 005, 006 (messages), 007, 009, 012 (lights,
// fog), 014, the system call 018, 019 (pad), 020 (math), 021, 027, 028, 029 (stage
// changes), 032, 033, 036, 038, 039, 106 (motion speed), 110, 151 (object
// control), 152 (object queries), 155 (vfchr objects), 016 (camera), 023
// (area triggers), 026, 156 (ray tests), 200 (turns), 202 / 211 (achr
// moves), 100 (actor motions), 212 (enemies), 252 (sound), 253 (movies)
// and 281 (scene effects) with its helpers: every entry of the table.

#include "sequence/seqcall.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "common/rosepan_stub.h"
#include "game/gamman.h"
#include "lib/unassigned.h"
#include "mag/magsystem.h"
#include "sdk/libvu0.h"
#include "sequence/sequence.h"
#include "snd/rxmpeg.h"
#include "snd/sndman.h"
#include "utl/utlmisc.h"

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
// Wait until: a0 picks the condition (19 cases, jump table data 0x2ff930);
// while it holds the call runs again next frame, else the script goes on
// with 0. Out-of-range a0 and a missing object go on at once.

static intptr_t seqWaitWhile(int busy)
{
    return busy != 0 ? SEQ_RET_WAIT : 0;
}

// ROR 0x00205570

intptr_t seqFunc004(intptr_t id, intptr_t a1, intptr_t a2, intptr_t, intptr_t, intptr_t)
{
    GamMan   &gam = g_gamMan;
    MagScene &scene = g_magSystem.m_scene;
    u_int     oid = (u_int)a1;

    switch ((u_int)id)
    {
    case 0:  // a1 frames
        if (a1 == 0)
            return 0;
        return seqWaitWhile(g_sequence.waitFrames((int)a1));
    case 1:
        return seqWaitWhile(gam.resetTenkyuFlags());
    case 2:  // while the object's actor plays motion a2
        if (!gam.isObj(oid))
            return 0;
        return scene.getActorMotion(gam.getActorNo(oid)) == (int)a2 ? SEQ_RET_WAIT : 0;
    case 3:  // while the object's task (param 0x19) runs
    {
        int task = 0;   // PC: the ROM leaves it unset when getObjParam has nothing
        gam.getObjParam(oid, 0x19, &task);
        return seqWaitWhile(g_taskMan.getTask(task) != NULL);
    }
    case 4:  // while thread a1 lives
        return seqWaitWhile(g_sequence.hasThread((u_int)a1, 0));
    case 5:  // while the actor's fourth task runs
        if (!gam.isObj(oid))
            return 0;
        return gam.getAchr(oid)->m_task[3] != -1 ? SEQ_RET_WAIT : 0;
    case 6:  // 17 frames, then while a menu is open
        if (g_sequence.waitFrames(0x11) != 0)
            return SEQ_RET_WAIT;
        return seqWaitWhile(gam.isMenuOpen());
    case 7:  // while a message shows
        return seqWaitWhile(gam.isMsgBusy());
    case 8:
        return seqWaitWhile(g_taskMan.getTask(gam.m_frameTaskId) != NULL);
    case 9:  // until pad 0 bit a1 is set (0..15 held, m_hold +0x78; 16.. pressed this frame, m_trig +0x7a)
    {
        // a1 == 16 tests bit 16 of the 16-bit m_hold and waits for ever (the
        // ROM bounds a1 < 0x11; kept).
        const BedPad *pad = g_magSystem.m_pad_ref[0].m_pad;
        if ((u_int)a1 < 0x11)
        {
            u_int bit = 1u << (u_int)a1;
            return (pad->m_hold & bit) == bit ? 0 : SEQ_RET_WAIT;
        }
        u_int bit = 1u << (u_int)(a1 - 0x10);
        return (pad->m_trig & bit) == bit ? 0 : SEQ_RET_WAIT;
    }
    case 10:
        return seqWaitWhile(g_taskMan.getTask(gam.m_selectTaskId) != NULL);
    case 11:
        return seqWaitWhile(g_taskMan.getTask(gam.m_movieTaskId) != NULL);
    case 12:  // while the actor's second task runs
        if (!gam.isObj(oid))
            return 0;
        return gam.getAchr(oid)->m_task[1] != -1 ? SEQ_RET_WAIT : 0;
    case 13:
        return seqWaitWhile(g_taskMan.getTask(gam.m_camTaskId) != NULL);
    case 14:
        return seqWaitWhile(g_taskMan.getTask(gam.m_bgLoadTaskId) != NULL);
    case 15:  // while the system message window is open
        return seqWaitWhile(scene.isWindowOpen(gam.m_sysMsgHandle));
    case 16:  // while a thread named a1 lives
        return seqWaitWhile(g_sequence.hasThreadNamed((const char *)a1));
    case 17:  // while the object's actor is in a mode
        if (!gam.isObj(oid))
            return 0;
        return seqWaitWhile(scene.m_actors[gam.getActorNo(oid)].isModeSet());
    case 18:  // until script flag 62 is set
        return (g_sequence.m_bits[1] & 0x40000000) != 0 ? 0 : SEQ_RET_WAIT;
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// A random number 0..a1-1 (a1 = 0 counts as 1).
// ROR 0x002058f0

intptr_t seqFunc005(intptr_t, intptr_t a1, intptr_t, intptr_t, intptr_t, intptr_t)
{
    int range = 1;
    if (a1 != 0)
        range = (int)a1;
    return RosePan_Rand() % range;  // callcheck: rand (RosePan_Rand keeps the ROM 31-bit range)
}

// ──────────────────────────────────────────────────────────────────────
// Stage changes: a0 1 / 2 a room a1 of the stage (2 with a2 for the
// fade's word, 1 with SEQ_RET_WAIT), 10 chapter a1; the thread ends either
// way. Rooms wait while the background load runs (flags 0x800). With a
// TskFrameFunc pair set by system call 0x34 the change goes through a kind
// 0xb frame task (the pair, the room, the word) instead of at once.
// callcheck: operator_new (new TskFrameFunc)
// ROR 0x00208b98

intptr_t seqFunc029(intptr_t id, intptr_t a1, intptr_t a2, intptr_t, intptr_t, intptr_t)
{
    GamMan &gam = g_gamMan;
    u_int   word;

    if (id == 1 || id == 2)
    {
        if ((gam.m_flags & 0x800) != 0 && g_taskMan.getTask(gam.m_bgLoadTaskId) != NULL)
            return SEQ_RET_WAIT;
        word = id == 1 ? (u_int)SEQ_RET_WAIT : (u_int)a2;
        if (gam.m_seqPair[3].m_a == 0)
        {
            gam.requestStage(2, (int)a1, 0, -1, word);
            return SEQ_RET_END;
        }
        // The task reads the four words (the ROM's stack block).
        int words[4] = {gam.m_seqPair[3].m_b, gam.m_seqPair[3].m_a, (int)a1, (int)word};
        TskFrameFunc *task = new TskFrameFunc(0xb, (intptr_t)words, 0);
        g_taskMan.addTask(1, -1, 1, 0, task, 0, NULL, 1);
        return SEQ_RET_END;
    }
    if (id == 10)
        gam.requestStage(1, -1, -1, (int)a1, (u_int)SEQ_RET_WAIT);
    return SEQ_RET_END;
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

// ──────────────────────────────────────────────────────────────────────
// A new game's state: the script globals and GamMan's game state cleared.
// ROR 0x0020541c

intptr_t seqFunc001(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    g_sequence.clearGlobals();
    g_gamMan.initGameState();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The default scene setup: every scene flag but the last on, the ambient
// and the two lights of set 1 (grey 0.3, the direction {pi, 0, 0}), the
// sky path display off and GamMan's light flag 1.
// ROR 0x0020544c

intptr_t seqFunc002(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    static const float grey[4] = {0.3f, 0.3f, 0.3f, 1.0f};
    static const float dir[4] = {3.1415925f, 0.0f, 0.0f, 1.0f};
    MagScene &scene = g_magSystem.m_scene;

    scene.m_22b4 = 0;
    g_magSystem.setSceneFlags(1, 1, 1, 1, 1, 1, 0);
    scene.setLightParam(0, 0, 2, grey, 0);
    scene.setLightParam(1, 0, 1, dir, 0);
    scene.setLightParam(1, 0, 2, grey, 0);
    g_gamMan.setLightFlag(1, 0);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// GamMan's state: a0 picks the service (a jump table, data 0x3005c0), a1
// its argument. The script flag bits are g_sequence.m_bits.
// ROR 0x0020810c

intptr_t seqFunc021(intptr_t op, intptr_t a1, intptr_t a2, intptr_t, intptr_t, intptr_t)
{
    GamMan &gam = g_gamMan;
    int     arg = (int)a1;

    switch ((u_int)op)
    {
    case 1:
        gam.resume((u_int)a1);
        return 0;
    case 2:     // the menu pause continues next frame
        gam.pause((u_int)a1);
        if ((u_int)a1 != 0x100000)
            return 0;
        return SEQ_RET_YIELD;
    case 3:
        gam.setDog(arg);
        return 0;
    case 4:
        gam.setMurder(arg);
        return 0;
    case 5:
    case 6:
        gam.setPc(arg);
        return 0;
    case 7:
        g_sequence.m_bits[(u_int)a1 >> 5] |= 1u << ((u_int)a1 & 0x1f);
        return 0;
    case 8:
        g_sequence.m_bits[(u_int)a1 >> 5] &= ~(1u << ((u_int)a1 & 0x1f));
        return 0;
    case 9:
        g_sequence.m_global[0x5d] = arg;
        gam.setSysParam(0x1b, &arg);
        return 0;
    case 10:
        return (gam.m_flags & (u_int)a1) != 0;
    case 11:
        return gam.m_chapter;
    case 12:
        gam.m_flags2 |= (u_int)a1;
        return 0;
    case 13:
        gam.m_flags2 &= ~(u_int)a1;
        return 0;
    case 14:
        return (gam.m_flags2 & (u_int)a1) != 0;
    case 20:
        gam.setPcJnfc(arg);
        return 0;
    case 21:
        gam.m_21ff4 = arg;
        gam.m_21ff8 = (int)a2;
        gam.m_flags2 |= 0x10000000;
        return 0;
    case 30:
        return gam.m_pcPtr ? gam.m_pcPtr->m_oid : 0;
    case 31:
        return gam.m_dogPtr ? gam.m_dogPtr->m_oid : 0;
    default:    // 0, 15..19, 22..29
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Objects of the stage: a0 picks the service (data 0x300700, 205
// entries, 14 with code), a1..t1 its arguments. Models come from the
// system data record's field 0xf when it has one, else the object is
// added without a model (-1).

// Field 0xf of system data record id (inline in the ROM).
static int seqModelOf(int id)
{
    u_short model[8];
    g_gamMan.m_sysData.getField(id, 0xf, model);
    return model[0];
}

// callcheck: getField (inline in the ROM: seqModelOf)
// ROR 0x00208640
intptr_t seqFunc027(intptr_t op, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t t0, intptr_t t1)
{
    GamMan &gam = g_gamMan;
    int     id = (int)(a1 & 0xffff);
    int     model;

    gamStage2Loaded();
    switch ((u_int)op)
    {
    case 2:     // the ROM passes (a1, 2) to the empty function
        gam.empty1ca0d4();
        return 0;
    case 3:
        gam.createObjByModel(id, (u_int)a2, 0xffff, NULL, -1, 1);
        return 0;
    case 4:
        model = seqModelOf(id);
        if (model != 0xffff)
            gam.createObjByModel(model, (u_int)a2, id, NULL, -1, 1);
        else
            gam.addChr(-1, (u_int)a2, id, NULL, 0);
        return 0;
    case 5:     // at position a3
        model = seqModelOf(id);
        if (model != 0xffff)
            gam.createObjByModel(model, (u_int)a2, id, (const void *)a3, -1, 1);
        else
            gam.addChr(-1, (u_int)a2, id, (const void *)a3, 0);
        return 0;
    case 6:     // at position t0, place a3
        model = seqModelOf(id);
        if (model != 0xffff)
            gam.createObjByModel(model, (u_int)a2, id, (const void *)t0, (int)a3, 1);
        else
            gam.addChr(-1, (u_int)a2, id, (const void *)t0, (int)a3);
        return 0;
    case 7:     // at position t1, place t0 (the ROM tests a3, not the model)
        model = seqModelOf(id);
        if ((int)a3 != 0xffff)
            gam.createObjByModel(model, (u_int)a2, id, (const void *)t1, (int)t0, 1);
        else
            gam.addChr(-1, (u_int)a2, id, (const void *)t1, (int)t0);
        return 0;
    case 8:     // a group: t1 points to 8 words, word 7 an int made float
    {
        const int *w = (const int *)t1;
        model = seqModelOf(id);
        gam.addChrGroup(model, (u_int)a2, w[1], w[2], w[3], w[4], w[5], w[6], id,
                        (const void *)t0, w[0], (float)w[7]);
        return 0;
    }
    case 9:
        gam.createObjByName((u_int)id, (const char *)a2, -1, (u_int)a3, 0xffff,
                            (const void *)t0, -1);
        return 0;
    case 10:    // the ROM passes (a1, 3) to the empty function
        gam.empty1ca0d4();
        return 0;
    case 12:
        gam.loadMapUnit((const char *)a1, (int)a2, 2);
        return 0;
    case 13:
        gam.loadMapUnit((const char *)a1, (int)a2, 3);
        return 0;
    case 14:
        gam.changeObjModel((u_int)id, (const char *)a2, (u_int)a3, 1);
        return 0;
    case 15:
        gam.changeObjModel((u_int)id, NULL, (u_int)a2, 1);
        return 0;
    case 20:
        gam.setPcAnim((int)a1);
        return 0;
    case 35:
        gam.createObjByName((u_int)(t1 & 0xffff), (const char *)a1, 9, (u_int)a3,
                            (int)(a2 & 0xffff), NULL, -1);
        return 0;
    case 36:    // model of record a2, oid a3, at t0, place = script global 0x2e
    {
        int kind = (int)(a2 & 0xffff);
        gam.createObjByModel(seqModelOf(kind), (u_int)a3, kind, (const void *)t0,
                             g_sequence.m_global[0x2e], 1);
        return 0;
    }
    case 50:
        gam.createObjByModel(id, (u_int)a2, (int)(a3 & 0xffff), NULL, -1, 1);
        return 0;
    case 100:   // only while no BG load task runs
        if (!g_taskMan.getTask(gam.m_bgLoadTaskId))
            gam.empty1cd7c0();
        return 0;
    case 200:   // a new object; its oid to script global 0x62, its angle *t0
    {
        u_int oid = gam.createObjByName((u_int)id, (const char *)a2, (int)((u_int)a1 >> 10), 0,
                                        0xffff, (const void *)a3, -1);
        g_sequence.m_global[0x62] = (int)oid;
        if (t0 != 0)
            gam.setObjDir(oid, *(const float *)t0);
        return 0;
    }
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Terminates the script string a1 (op 10) that runs past 0x1fd chars.
// ROR 0x00208dc4

intptr_t seqFunc032(intptr_t op, intptr_t a1, intptr_t, intptr_t, intptr_t, intptr_t)
{
    // The ROM's a1 < 0x8000000 (an EE RAM address) is the host's non-null.
    if (op != 10 || a1 == 0)
        return 0;
    char *p = (char *)a1;
    char *c;
    int   n = 0;
    for (;;)
    {
        c = p;
        if (*c == '\0')
            break;
        int more = n < 0x200;
        n++;
        p = c + 1;
        if (!more)
            break;
    }
    if (n > 0x1fd)
    {
        c[2] = '\0';
        c[1] = '\0';
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Fades, the volume, the cinemascope bars and the fade tasks: a0 picks
// (data 0x300ac0, 14 entries), a1 is the time or value.
// callcheck: operator_new, TskFrameFunc, addTask (inline in the ROM: seqStartFrameTask)
// ROR 0x00208e24

intptr_t seqFunc033(intptr_t op, intptr_t a1, intptr_t, intptr_t, intptr_t, intptr_t)
{
    MagScene &scene = g_magSystem.m_scene;
    int       state;
    intptr_t  word;

    switch ((u_int)op)
    {
    case 1:     // fade in unless one is running or done (states 0, 3, 5)
        state = scene.getFadeState();
        if (state == 3 || state == 5 || state == 0)
            return 0;
        scene.fadeIn((int)a1, 7, 0);
        return 1;
    case 2:     // fade out unless one is running or done (states 4, 6)
        state = scene.getFadeState();
        if (state == 4 || state == 6)
            return 0;
        scene.fadeOut((int)a1, 7, 0);
        return 1;
    case 3:
        return scene.getFaderWord30() - 1;
    case 4:
        return (u_int)(scene.getFadeState() - 3) < 2;
    case 5:     // a1 is an unsigned volume, halved
        g_gamMan.setVolume((int)((float)(u_int)a1 * 0.5f));
        return 0;
    case 6:
        scene.m_cine_sco.fadeIn((int)a1);
        return 0;
    case 7:
        scene.m_cine_sco.fadeOut((int)a1);
        return 0;
    case 10:    // TskFrameFunc kinds 2..5 over the word a1
    case 11:
    case 12:
    case 13:
        word = a1;
        seqStartFrameTask((int)op - 8, (intptr_t)&word, 0);
        return 0;
    default:    // 0, 8, 9
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Threads, entries and objects of the init scripts: a0 picks (data
// 0x300d20, 100 entries, 13 with code). G000's main calls it ~80 times.
// ROR 0x002099bc

static float s_seqChrPos[3];    // data 0x2d9900: case 21's position

intptr_t seqFunc110(intptr_t op, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t, intptr_t)
{
    GamMan   &gam = g_gamMan;
    Sequence &seq = g_sequence;

    switch ((u_int)op)
    {
    case 1:     // a thread at a1 with id a2
        seq.queueThread((u_int)a2, a1, 0, 0, 0, "none");
        return 0;
    case 2:
        gam.addChr((int)a1, (u_int)a2, 0xffff, NULL, 0);
        return 0;
    case 3:
        gam.addChr(-1, (u_int)a1, 0xc00, NULL, 0);
        return 0;
    case 4:     // a vfchr of record a1's model (a1 0xffff: none)
    {
        u_short model[8];
        if ((int)a1 != 0xffff)
            gam.m_sysData.getField((int)(a1 & 0xffff), 0xf, model);
        else
            model[0] = 0xffff;
        gam.addVFchr(model[0], (u_int)a2, (int)a3, (int)(a1 & 0xffff), 0);
        return 0;
    }
    case 11:
        seq.addEntry((u_int)a2, (int)a1);
        return 0;
    case 13:
        seq.callEntry((u_int)a1, 0, 0);
        return SEQ_RET_YIELD;
    case 14:
        seq.callEntry((u_int)a1, 0, 1);
        return SEQ_RET_YIELD;
    case 15:
        seq.callEntryNow((u_int)a1, 0, 1);
        return SEQ_RET_RETURN;
    case 20:
        gam.addChr((int)a1, (u_int)a2, (int)(a3 & 0xffff), NULL, 0);
        return 0;
    case 21:    // an achr of kind 0x1200 at a copy of the position a2
    {
        const float *pos = (const float *)a2;
        s_seqChrPos[0] = pos[0];
        s_seqChrPos[1] = pos[1];
        s_seqChrPos[2] = pos[2];
        gam.addChr(-1, (u_int)a1, 0x1200, s_seqChrPos, 0);
        return 0;
    }
    case 30:    // a named thread at a1 with id a2
        seq.queueThread((u_int)a2, a1, 0, 0, 0, (const char *)a3);
        return 0;
    case 31:
        seq.endThreadOf((const char *)a1);
        return 0;
    case 99:
        seq.endThread((u_int)a2, 0);
        return 0;
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// A script word that holds a float's bits (the ROM loads it with lwc1).

static float seqWordFloat(intptr_t word)
{
    int   bits = (int)word;
    float f;
    memcpy(&f, &bits, sizeof(f));
    return f;
}

// ──────────────────────────────────────────────────────────────────────
// Script math: a0 picks the operation (62 cases, jump table data 0x3004a0;
// 0, 6..9, 14..19, 22..29, 31..49 do nothing). Cases 1..4 take the angle
// as float bits in a1 and store to a2; the others store to the float (or
// vector) at a1 from the floats or vectors at a2, a3 and t0. The vector
// cases copy xyz into a quadword with w = 0 for the VU0 library and store
// all four words of its result.

// A script vector as the ROM loads it for VU0: xyz, w = 0.
static void seqLoadVec(sceVu0FVECTOR v, const float *p)
{
    v[0] = p[0];
    v[1] = p[1];
    v[2] = p[2];
    v[3] = 0.0f;
}

// ROR 0x00207ae8
// callcheck: fptodp, dpmul, dpdiv, dptofp (the soft-float double ops of cases 1 and 2)
// callcheck: rand (RosePan_Rand keeps the ROM 31-bit range)

intptr_t seqFunc020(intptr_t op, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t t0, intptr_t)
{
    float        *out = (float *)a1;
    const float  *p   = (const float *)a2;
    const float  *q   = (const float *)a3;
    sceVu0FVECTOR r, u, v;
    float         ang, x, y, z;

    switch ((u_int)op)
    {
    case 1:  // sin of degrees (in double, pi / 180 from data 0x300598)
        *(float *)a2 = sinf((float)((double)seqWordFloat(a1) * 3.141592653589793 / 180.0));
        break;
    case 2:  // cos of degrees
        *(float *)a2 = cosf((float)((double)seqWordFloat(a1) * 3.141592653589793 / 180.0));
        break;
    case 3:
        *(float *)a2 = sinf(seqWordFloat(a1));
        break;
    case 4:
        *(float *)a2 = cosf(seqWordFloat(a1));
        break;
    case 5:
        *out = atan2f(p[0], q[0]);
        break;
    case 10:
        *out = p[0] + q[0];
        break;
    case 11:
        *out = p[0] - q[0];
        break;
    case 12:
        *out = p[0] / q[0];
        break;
    case 13:
        *out = p[0] * q[0];
        break;
    case 20:  // cvt.w.s: truncates
        *(int *)a1 = (int)p[0];
        break;
    case 21:
        *out = (float)*(const int *)a2;
        break;
    case 30:
        *out = sqrtf(p[0] + q[0]);
        break;
    case 50:  // a2 + the offset a3 turned by the Y angle at t0
        ang = *(const float *)t0;
        x = cosf(ang) * q[0] + sinf(ang) * q[2] + p[0];
        y = q[1] + p[1];
        z = -sinf(ang) * q[0] + cosf(ang) * q[2] + p[2];
        out[0] = x;
        out[1] = y;
        out[2] = z;
        break;
    case 51:
        seqLoadVec(u, p);
        seqLoadVec(v, q);
        sceVu0AddVector(r, u, v);
        memcpy(out, r, sizeof(r));
        break;
    case 52:
        seqLoadVec(u, p);
        seqLoadVec(v, q);
        sceVu0SubVector(r, u, v);
        memcpy(out, r, sizeof(r));
        break;
    case 53:
        seqLoadVec(u, p);
        sceVu0Normalize(r, u);
        memcpy(out, r, sizeof(r));
        break;
    case 54:  // the dot product of a1 and a2, stored over a1's x
        seqLoadVec(u, out);
        seqLoadVec(v, p);
        *out = sceVu0InnerProduct(u, v);
        break;
    case 55:
        seqLoadVec(u, p);
        seqLoadVec(v, q);
        sceVu0OuterProduct(r, u, v);
        memcpy(out, r, sizeof(r));
        break;
    case 56:
        *out = sqrtf(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
        break;
    case 57:
        seqLoadVec(u, p);
        r[3] = 0.0f;  // the ROM's vmulx.xyz keeps u's w; the host's leaves r's
        sceVu0ScaleVectorXYZ(r, u, q[0]);
        memcpy(out, r, sizeof(r));
        break;
    case 58:
        memcpy(out, p, 4 * sizeof(float));
        break;
    case 59:
        out[0] = p[0];
        out[1] = q[0];
        out[2] = *(const float *)t0;
        out[3] = 0.0f;
        break;
    case 60:  // wrap by 2 pi as the ROM spells it (0x40c90fda)
        *out = fmodf(p[0], 6.2831845f);
        break;
    case 61:  // a random unit vector (0x4e7fffff = 2^30 - 64 centres rand)
        r[0] = (float)RosePan_Rand() - 1073741760.0f;
        r[1] = (float)RosePan_Rand() - 1073741760.0f;
        r[2] = (float)RosePan_Rand() - 1073741760.0f;
        r[3] = 0.0f;
        sceVu0Normalize(r, r);
        memcpy(out, r, sizeof(r));
        break;
    default:
        break;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Message window: a0 1 copies eight words from a1 over GamMan's message
// parameters.
// ROR 0x00205b18

intptr_t seqFunc007(intptr_t op, intptr_t a1, intptr_t, intptr_t, intptr_t, intptr_t)
{
    if ((int)op == 1)
        g_gamMan.setMsgParam((const int *)a1);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Message task: a0 1 kills it.
// ROR 0x00205b44

intptr_t seqFunc009(intptr_t op, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    if ((int)op == 1)
        g_gamMan.killMsgTask();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Sky: a0 1 shows the first sky slot's model when a1 != 0, hides it when 0.
// ROR 0x00206614

intptr_t seqFunc014(intptr_t op, intptr_t a1, intptr_t, intptr_t, intptr_t, intptr_t)
{
    if ((int)op == 1)
        g_magSystem.m_scene.m_tenkyu[0].m_model[0].m_hide = a1 == 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Pad 0 queries: a0 0..0x1f one button bit (0..15 held m_hold, 16..31 the
// auto-repeated m_repeat; 0x14..0x17 the repeated direction bits), 4..7 the
// direction past half way (left, down, right, up), 0x40 / 0x41 all of a1's
// bits held / pressed, 0x42 any button held, 0x80 the stick into a2's two
// words (a1 0: the direction * 128, else the right stick, truncated, << 7).
// Bits 13 and 14 swap places in the table (0xd reads bit 14, 0xe bit 13).
// ROR 0x002076cc

intptr_t seqFunc019(intptr_t op, intptr_t a1, intptr_t a2, intptr_t, intptr_t, intptr_t)
{
    const BedPad *pad = g_magSystem.m_pad_ref[0].m_pad;
    float x, y;
    pad->getDir(&x, &y);
    u_int hold = pad->m_hold;
    u_int repeat = pad->m_repeat;
    u_int dir = pad->m_dir_repeat;

    switch ((u_int)op)
    {
    case 0x00: case 0x01: case 0x02: case 0x03:
        return (hold >> (u_int)op) & 1;
    case 0x04: return x < -0.5f;
    case 0x05: return 0.5f < y;
    case 0x06: return 0.5f < x;
    case 0x07: return y < -0.5f;
    case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c:
        return (hold >> (u_int)op) & 1;
    case 0x0d: return (hold >> 14) & 1;
    case 0x0e: return (hold >> 13) & 1;
    case 0x0f: return hold >> 15;
    case 0x10: case 0x11: case 0x12: case 0x13:
        return (repeat >> (u_int)(op - 0x10)) & 1;
    case 0x14: case 0x15: case 0x16: case 0x17:
        return (dir >> (u_int)(op - 0x10)) & 1;
    case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c:
        return (repeat >> (u_int)(op - 0x10)) & 1;
    case 0x1d: return (repeat >> 14) & 1;
    case 0x1e: return (repeat >> 13) & 1;
    case 0x1f: return repeat >> 15;
    case 0x40: return ((u_int)pad->m_hold & (u_int)a1) == (u_int)a1;
    case 0x41: return ((u_int)pad->m_trig & (u_int)a1) == (u_int)a1;
    case 0x42:  // the ROM tests the sixteen bits one by one
        return hold != 0;
    case 0x80:
    {
        int *out = (int *)a2;
        if (a1 != 0)
        {
            out[0] = (int)pad->getRightX() << 7;
            out[1] = (int)pad->getRightY() << 7;
        }
        else
        {
            if (out == NULL)    // PC: the ROM reads word 0 of address 0
                return 0;
            out[0] = (int)(x * 128.0f);
            out[1] = (int)(y * 128.0f);
        }
        return out[0] != 0 || out[1] != 0;
    }
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Release: a0 1 / 2 / 10 the sky domes of kind 2 / 3 / 0, 3 the object a1,
// 0x14 the PC's motion a1.
// ROR 0x00208b10

intptr_t seqFunc028(intptr_t op, intptr_t a1, intptr_t, intptr_t, intptr_t, intptr_t)
{
    GamMan &gam = g_gamMan;
    switch ((u_int)op)
    {
    case 0x01: gam.releaseTenkyus(2);              break;
    case 0x02: gam.releaseTenkyus(3);              break;
    case 0x03: gam.releaseChr((u_int)a1);          break;
    case 0x0a: gam.releaseTenkyus(0);              break;
    case 0x14: gam.releasePcMotion((int)a1);       break;
    default:                                       break;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// a0 1: the running thread's argument.
// ROR 0x002090a0

intptr_t seqFunc036(intptr_t op, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t)
{
    if ((int)op == 1)
        return g_sequence.getThreadArg();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Object table records (GamMan::ctlObjParam on the record of oid a1): a0 1 /
// 2 set / clear flag bits a2, 3 tests them, 4 / 5 set / get word +0x0c, 6
// sets +0x08; a2 points at the answer for 10 (+0x30), 12 (+0x20), 13 (+0x10),
// 14 (+0x08); 11 writes the record's two words +0x28 / +0x2c to *a2 / *a3.
// 99 runs op 5 with a3 as the alternative and drops the answer.
// ROR 0x002090e0

intptr_t seqFunc038(intptr_t op, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t, intptr_t)
{
    GamMan &gam = g_gamMan;
    u_int   oid = (u_int)a1;
    int     value = (int)a2;    // the ROM's stack copy of a2

    switch ((u_int)op)
    {
    case 0x01: gam.ctlObjParam(oid, 0x0, &value, 0);           break;
    case 0x02: gam.ctlObjParam(oid, 0x1, &value, 0);           break;
    case 0x03: return gam.ctlObjParam(oid, 0x2, &value, 0);
    case 0x04: gam.ctlObjParam(oid, 0x4, &value, 0);           break;
    case 0x05:
    {
        int word = 0;   // PC: unset in the ROM when no record matches
        gam.ctlObjParam(oid, 0x5, &word, 0);
        return word;
    }
    case 0x06: gam.ctlObjParam(oid, 0x9, &value, 0);           break;
    case 0x0a: gam.ctlObjParam(oid, 0xb, (void *)a2, 0);       break;
    case 0x0b:
    {
        int pair[2] = {0, 0};   // PC: unset in the ROM when no record matches
        gam.ctlObjParam(oid, 0xc, pair, 0);
        *(int *)a2 = pair[0];
        *(int *)a3 = pair[1];
        break;
    }
    case 0x0c: gam.ctlObjParam(oid, 0xd, (void *)a2, 0);       break;
    case 0x0d: gam.ctlObjParam(oid, 0xe, (void *)a2, 0);       break;
    case 0x0e: gam.ctlObjParam(oid, 0xf, (void *)a2, 0);       break;
    case 0x0f: gam.ctlObjParam(oid, 0x10, (void *)a2, 0);      break;
    case 0x63: gam.ctlObjParam(oid, 0x5, &value, (int)a3);     break;
    default:                                                   break;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Brown: a0 'eqds' sets / clears GamMan flags2 bit 0x4000000 by a1 (and then
// goes on as any other op), 100 sets the dog's mode a1, 0x65 reads it; the
// others go to magBwnSeqCommand with the six words, whose answer the script
// gets (a wait code too).
// ROR 0x00209270

intptr_t seqFunc039(intptr_t op, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t t0, intptr_t t1)
{
    GamMan &gam = g_gamMan;
    if ((int)op == 0x65716473)
    {
        if (a1 != 0)
            gam.m_flags2 |= 0x4000000;
        else
            gam.m_flags2 &= ~0x4000000u;
    }
    if ((int)op == 100)
    {
        gam.setInuMode((int)a1);
        return 0;
    }
    if ((int)op == 0x65)
        return gam.m_inuMode;
    intptr_t args[6] = {op, a1, a2, a3, t0, t1};
    return magBwnSeqCommand(args);
}

// ──────────────────────────────────────────────────────────────────────
// Lights and fog (jump table data 0x2ff9e0, 501 entries). The light cases
// come in three runs over the same (type, which) pairs: set from a1 (no 0),
// set from a1 with no a2, get into a1; a light index is the script's t1
// (type 0 has one light). Then 0x12c the actor light id, 0x190..0x195 the
// fog (push, pop, get, set or fade over a2 frames, near / far pair), 0x3c a
// TskAttach task, 0xc8 the scene's fog switch, 0xc9 GamMan::setLightFlag,
// 0xca..0xcf save / restore light sets, 0x1f4 the scene flags from a1.

/* layout: host -- a table of this file, not ROM data. */
struct SeqLightOp
{
    u_char type, which;
};

static const SeqLightOp s_seqLightOps[15] = {
    {0, 2},                                 // 1 / 0x46 / 0x64 (index 0)
    {1, 1}, {1, 2},                         // 0x0a.. / 0x47.. / 0x6e..
    {2, 0}, {2, 2},                         // 0x14..
    {3, 1}, {3, 0}, {3, 2}, {3, 5},         // 0x1e..
    {4, 1}, {4, 2}, {4, 3},                 // 0x28..
    {5, 0}, {5, 2}, {5, 4},                 // 0x32..
};

// The light op of a case of the first run (1, 0xa, 0xb, 0x14, ... 0x34), -1
// when it is none.
static int seqLightOpOf(u_int op)
{
    static const u_char s_cases[15] = {
        0x01, 0x0a, 0x0b, 0x14, 0x15, 0x1e, 0x1f, 0x20, 0x21, 0x28, 0x29, 0x2a, 0x32, 0x33, 0x34,
    };
    for (int i = 0; i < 15; i++)
    {
        if (s_cases[i] == op)
            return i;
    }
    return -1;
}

static void seqAddTask(CmnITask *task)
{
    g_taskMan.addTask(1, -1, 1, 0, task, 0, NULL, 1);
}

// ROR 0x00205b88

intptr_t seqFunc012(intptr_t id, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t, intptr_t t1)
{
    MagScene &scene = g_magSystem.m_scene;
    GamMan   &gam = g_gamMan;
    u_int     op = (u_int)id;

    if (op >= 0x1f5)
        return 0;

    // The three light runs: 1.. set, 0x46.. set with a frame count, 0x64.. get.
    int run = -1, light = -1;
    if (op < 0x46)
        run = 0, light = seqLightOpOf(op);
    else if (op < 0x64)
        run = 1, light = seqLightOpOf(op - 0x45);
    else if (op < 0xa0)
        run = 2, light = seqLightOpOf(op - 0x63);
    if (light >= 0)
    {
        const SeqLightOp &l = s_seqLightOps[light];
        int idx = light == 0 ? 0 : (int)t1;
        if (run == 0)
            scene.setLightParam(l.type, idx, l.which, (const float *)a1, 0);
        else if (run == 1)
            scene.setLightParam(l.type, idx, l.which, (const float *)a1, (int)a2);
        else
            scene.getLightParam(l.type, idx, l.which, (float *)a1, 0);
        return 0;
    }

    switch (op)
    {
    case 0xa0:
        scene.getLightParam(0, 0, 2, (float *)a1, (int)a2);
        break;
    case 0x12c:
        g_magSystem.setActorLightID(gam.getActorNo((u_int)a1), (int)a2);
        break;
    case 0x190:
        gam.pushFog();
        break;
    case 0x191:
        gam.popFog();
        break;
    case 0x192:
        *(I3dFog *)a1 = scene.m_fog[0];
        break;
    case 0x193:
    {
        I3dFog fog = *(const I3dFog *)a1;
        if (a2 == 0)
        {
            scene.setFog(&fog, 0);
            break;
        }
        // The task gets the address of this stack copy, as in the ROM
        // (TskFrameFunc kind 0xc reads it at once).
        seqAddTask(new TskFrameFunc(0xc, (intptr_t)&fog, (int)a2));
        break;
    }
    case 0x194:  // near distance and amount
    {
        I3dFog fog = scene.m_fog[0];
        fog.m_param[0] = *(const float *)a1;
        fog.m_param[1] = *(const float *)a2;
        scene.setFog(&fog, 0);
        break;
    }
    case 0x195:  // far distance and amount
    {
        I3dFog fog = scene.m_fog[0];
        fog.m_param[2] = *(const float *)a1;
        fog.m_param[3] = *(const float *)a2;
        scene.setFog(&fog, 0);
        break;
    }
    case 0x3c:
        seqAddTask(new TskAttach((u_int)a1, (const int *)a2, (int)a3));  // callcheck: operator_new, addTask (new and seqAddTask)
        break;
    case 0xc8:
        scene.m_2aa84 = a1 != 0 ? 1 : 0;
        break;
    case 0xc9:
        gam.setLightFlag((int)a1, (int)a2);
        break;
    case 0xca: gam.saveLightSets(2);    break;
    case 0xcb: gam.restoreLightSets(2); break;
    case 0xcc: gam.saveLightSets(1);    break;
    case 0xcd: gam.restoreLightSets(1); break;
    case 0xce: gam.saveLightSets(0);    break;
    case 0xcf: gam.restoreLightSets(0); break;
    case 0x1f4:
    {
        const int *f = (const int *)a1;
        g_magSystem.setSceneFlags(f[0], f[1], f[2], f[3], f[4], f[5], 0);
        break;
    }
    default:
        break;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Object control: a0 picks the operation (58 cases, jump table data
// 0x300eb0) on object t1 -- nothing unless t1 is live. a1 / a2 are words,
// float bits or pointers by case; cases 15..17 and 25 / 26 work on the
// system data record t1 / a1 instead. Always returns 0.
// ROR 0x00209d4c

intptr_t seqFunc151(intptr_t op, intptr_t a1, intptr_t a2, intptr_t, intptr_t, intptr_t t1)
{
    GamMan     &gam = g_gamMan;
    u_int       oid = (u_int)t1;
    short       id = (short)(t1 & 0xffff);
    GamObjColli colli;

    if (!gam.isObj(oid))
        return 0;
    switch ((u_int)op)
    {
    case 1:
    case 6:
        gam.setObjPos(oid, (const float *)a1);
        break;
    case 2:
        gam.setObjRot(oid, (float *)a1);
        break;
    case 3:
        gam.setObjScale(oid, seqWordFloat(a1));
        break;
    case 5:
        gam.setObjDir(oid, *(const float *)a1);
        break;
    case 10:
        gam.setObjParam(oid, (int)a1, (const void *)a2);
        break;
    case 11:
        gam.addObjParam(oid, (int)a1, (const void *)a2);
        break;
    case 12:
        gam.setObj(oid, (u_int)a1);
        break;
    case 13:
        gam.resetObj(oid, (u_int)a1);
        break;
    case 15:
        gam.m_sysData.setField(id, (int)a1, (const void *)a2);
        break;
    case 16:
        if ((int)a1 == 0)
            gam.m_sysData.setFlags(id, (u_int)a2);
        else
            gam.m_sysData.setFlags2(id, (u_short)a1, (u_int)a2);
        break;
    case 17:
        if ((int)a1 == 0)
            gam.m_sysData.clearFlags(id, (u_int)a2);
        else
            gam.m_sysData.clearFlags2(id, (u_short)a1, (u_int)a2);
        break;
    case 18:
        gam.setObjAttr(oid, (u_int)a1);
        break;
    case 19:
        gam.clearObjAttr(oid, (u_int)a1);
        break;
    case 20:
        gam.placeObjAt(oid, (int)a1);
        break;
    case 25:
        gam.setKindFlags((u_short)a1, (u_int)a2, 1);
        break;
    case 26:
        gam.setKindFlags((u_short)a1, (u_int)a2, 0);
        break;
    case 30:
        gam.setObjWork(oid, (int)a1, (int)a2);
        break;
    case 31:
        gam.addObjWork(oid, (int)a1, (int)a2);
        break;
    case 40:
        if ((int)a1 != 0)
            gam.setObj(oid, 1);
        else
            gam.resetObj(oid, 1);
        break;
    case 41:
        gam.setActorBindMode(oid, (int)a1);
        break;
    case 42:
        g_magSystem.m_scene.m_actors[gam.getActorNo(oid)].setFlag100Bits((int)a1);
        break;
    case 50:    // the shape word
        gam.getObjParam(oid, 0x18, &colli);
        colli.m_shape = (int)a1;
        gam.setObjParam(oid, 0x18, &colli);
        break;
    case 51:    // the shape word, shape 4 with its radius
        gam.getObjParam(oid, 0x18, &colli);
        colli.m_shape = (int)a1;
        if ((int)a1 == 4)
            colli.m_radius = seqWordFloat(a2);
        gam.setObjParam(oid, 0x18, &colli);
        break;
    case 52:    // a whole shape; kind 1 is refitted to the corners
    {
        GamObjColli *shape = (GamObjColli *)a1;
        gam.setObjParam(oid, 0x18, shape);
        if (shape->m_kind == 1)
            gam.fitObjBounds(oid);
        break;
    }
    case 53:    // a box {x, y, z, sx, sy, sz, rot, -}: the shape a1 (4, the
                // mean size), the corners of the rotated x / z rectangle,
                // the position
    {
        GamObjColli *shape = (GamObjColli *)a1;
        float        box[8];
        float        quad[8];

        memcpy(box, (const void *)a2, sizeof(box));
        shape->m_shape = 4;
        shape->m_radius = (box[3] + box[4] + box[5]) / 3.0f;
        gam.setObjParam(oid, 0x18, shape);

        float c = cosf(box[6]);
        float s = sinf(box[6]);
        float hx = box[3] * 0.5f;
        float hz = box[5] * 0.5f;
        quad[0] = c * -hx + s * -hz + box[0];
        quad[1] = s * hx + c * -hz + box[2];
        quad[2] = c * hx + s * -hz + box[0];
        quad[3] = -s * hx + c * -hz + box[2];
        quad[4] = c * hx + s * hz + box[0];
        quad[5] = -s * hx + c * hz + box[2];
        quad[6] = c * -hx + s * hz + box[0];
        quad[7] = s * hx + c * hz + box[2];
        gam.setObjParam(oid, 0x1003, quad);
        gam.setObjPos(oid, box);
        gam.queueObjUpdate(0, oid);
        break;
    }
    case 54:
        *(float *)a1 = gam.getObjRadius(oid, NULL);
        break;
    case 55:
        gam.addObjColliSegment(oid, (const float *)a1);
        break;
    case 56:
        gam.addObjColliBox(oid, (const float *)a1);
        break;
    case 57:
        gam.addObjColliQuad(oid, (const float *)a1);
        break;
    default:    // 0, 4, 7..9, 14, 21..24, 27..29, 32..39, 43..49
        break;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Object queries: a0 picks the question (51 cases, jump table data
// 0x300fa0) about object t1 -- nothing unless t1 is live, except 0x19 /
// 0x1a, which read the system data record t1. Answers go to a1 (vectors)
// or v0. Cases 0x28..0x2a step the actor's position along an angle.
// ROR 0x0020a368

intptr_t seqFunc152(intptr_t op, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t, intptr_t t1)
{
    GamMan   &gam = g_gamMan;
    DogActor *actors = g_magSystem.m_scene.m_actors;
    u_int     oid = (u_int)t1;
    float    *out = (float *)a1;
    int       no = 0;

    if ((u_int)op - 0x19 >= 2)
    {
        if (!gam.isObj(oid))
            return 0;
        no = gam.getActorNo(oid);
    }

    float v[3];
    float angle, dist;
    switch ((u_int)op)
    {
    case 1:     // the actor's translation
        out[0] = actors[no].m_trans[0];
        out[1] = actors[no].m_trans[1];
        out[2] = actors[no].m_trans[2];
        return 0;
    case 2:     // its rotation
        out[0] = actors[no].m_rotate[0];
        out[1] = actors[no].m_rotate[1];
        out[2] = actors[no].m_rotate[2];
        return 0;
    case 3:     // its scale -- the ROM indexes the actors by t1 itself, not the actor number
        if (oid < 128)  // PC: an oid past the 128 actors reads beyond the scene in the ROM; skipped
        {
            out[0] = actors[oid].m_scale[0];
            out[1] = actors[oid].m_scale[1];
            out[2] = actors[oid].m_scale[2];
        }
        return 0;
    case 0x14:
        gam.getObjParam(oid, (int)a1, (void *)a2);
        return 0;
    case 0x15:
        return gam.testObjFlags(oid, (u_int)a1);
    case 0x16:
        return gam.testAchrFlag(oid, (u_int)a2);
    case 0x19:  // field a1 of system record t1 into a2
        gam.m_sysData.getField((int)(oid & 0xffff), (int)a1, (void *)a2);
        return 0;
    case 0x1a:  // system record t1's flags: a1 != 0 the second word
        if (a1 != 0)
            return gam.m_sysData.testFlags94((short)oid, (int)(a1 & 0xffff), (u_int)a2);
        return gam.m_sysData.testFlags((short)oid, (u_int)a2);
    case 0x1e:
        return gam.getObjWork(oid, (int)a1);
    case 0x28:  // a2 ahead of the translation, the angle its y + pi wrapped to -pi..pi
    {
        // The ROM reads the translation's y (+0x44), not the rotation's;
        // kept. The wrap runs in double, as the ROM's dpadd / dpcmp do.
        // callcheck: fptodp, dpadd, dpcmp, dpsub, dptofp (the double arithmetic below)
        const double pi = 3.141592653589793;
        v[0] = actors[no].m_trans[0];
        v[1] = actors[no].m_trans[1];
        v[2] = actors[no].m_trans[2];
        angle = (float)((double)v[1] + pi);
        if ((double)angle < -pi)
            angle = (float)((double)angle + 2.0 * pi);
        if ((double)angle > pi)
            angle = (float)((double)angle - 2.0 * pi);
        dist = seqWordFloat(a2);
        break;
    }
    case 0x29:  // the same without the half turn
        v[0] = actors[no].m_trans[0];
        v[1] = actors[no].m_trans[1];
        v[2] = actors[no].m_trans[2];
        angle = v[1];
        dist = seqWordFloat(a2);
        break;
    case 0x2a:  // angle a2, distance a3
        v[0] = actors[no].m_trans[0];
        v[1] = actors[no].m_trans[1];
        v[2] = actors[no].m_trans[2];
        angle = seqWordFloat(a2);
        dist = seqWordFloat(a3);
        break;
    case 0x2b:  // t1 onto node a2 of object a1
        gam.attachObj(oid, (u_int)a1, (const char *)a2);
        return 0;
    case 0x2c:
        gam.detachObj(oid, (u_int)a1);
        return 0;
    case 0x32:
        return g_sequence.hasThreadNamed((const char *)a1);
    default:
        return 0;
    }
    utlAddPolar(angle, dist, v);
    out[0] = v[0];
    out[1] = v[1];
    out[2] = v[2];
    out[3] = 1.0f;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Vfchr objects (24 cases, jump table data 0x301090) -- the scripts'
// placed characters: cases 2..5 add vfchr a2 unless it exists (4 / 5 load
// its model when it belongs to the current WRD room, getStatus 5), 10 / 11
// release, 20..22 are GamMan's WRD room inlines. Returns 0 but case 23.

// FCHRstat::setParam on vfchr oid. PC: skipped when there is none (the
// ROM writes through what getVFchr returns; addVFchr always makes one,
// a stub does not).
static void seqSetVFchrParam(u_int oid, int which, const void *data)
{
    FCHRstat *vf = g_gamMan.getVFchr(oid);
    if (vf != NULL)
        vf->setParam(which, data);
}

// ROR 0x0020a7d0

intptr_t seqFunc155(intptr_t op, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t t0, intptr_t t1)
{
    GamMan &gam = g_gamMan;
    u_int   oid = (u_int)a2;
    int     model = (int)a1;    // the ROM hands setParam the address of its a1 / t0 words
    int     word = (int)t0;

    switch ((u_int)op)
    {
    case 1:     // param a1 of vfchr t1 from a2; callcheck: setParam (inside seqSetVFchrParam)
        seqSetVFchrParam((u_int)t1, (int)a1, (const void *)a2);
        return 0;
    case 2:     // vfchr a2 of model a1: arg t0, kind a3
        if (gam.getVFchr(oid) != NULL)
            return 0;
        gam.addVFchr(-1, oid, (int)t0, (int)(a3 & 0xffff), t1);
        seqSetVFchrParam(oid, 0x1000, &model);
        return 0;
    case 3:     // the same with t0 pointing at {arg, param 0x40}
    {
        const int *words = (const int *)t0;
        if (gam.getVFchr(oid) != NULL)
            return 0;
        gam.addVFchr(-1, oid, words[0], (int)(a3 & 0xffff), t1);
        seqSetVFchrParam(oid, 0x1000, &model);
        seqSetVFchrParam(oid, 0x40, &words[1]);
        return 0;
    }
    case 4:     // vfchr a2 of system record a1's model, room a3, unless listed
    {
        if (gam.getVFchr(oid) != NULL || gam.isInOidList((int)oid) != 0 ||
            gam.isObjRecRef((int)a3) == 0)
            return 0;
        u_short rec[8];
        gam.m_sysData.getField((int)(a1 & 0xffff), 0xf, rec);
        int recModel = rec[0] == 0xffff ? 0x800 : rec[0];
        gam.addVFchr(-1, oid, (int)a3, (int)(a1 & 0xffff), t1);
        int room[4];
        gam.getStatus(5, room);
        if ((int)a3 == room[0])
        {
            gam.loadVFchrModel(recModel & 0xffff, oid, t1);
            int param[4];
            gam.getObjParam(oid, 3, param);  // the answer is not used
        }
        seqSetVFchrParam(oid, 0x1000, &recModel);
        seqSetVFchrParam(oid, 0x40, &word);
        return 0;
    }
    case 5:     // a new vfchr (image a3) of system record a1's model by name, room a2
    {
        int         id = (int)(a1 & 0xffff);
        u_short     rec[8];
        const char *name[2] = {NULL, NULL};
        gam.m_sysData.getField(id, 0xf, rec);
        gam.m_sysData.getField(id, 0xb, name);
        u_int vfOid = gam.makeOid(0, 5, (int)a3);
        gam.addVFchr(-1, vfOid, (int)a2, id, t0);
        int room[4];
        gam.getStatus(5, room);
        if ((int)a2 == room[0])
            gam.loadVFchrModelByName(rec[0], name[0], vfOid, t0);
        int recModel = rec[0];
        seqSetVFchrParam(vfOid, 0x1000, &recModel);
        return 0;
    }
    case 10:
        gam.releaseVFchrs(0);
        return 0;
    case 11:
        gam.releaseVFchr(oid);
        return 0;
    case 20:    // callcheck: gamWrdConvPos (inside GamMan::cnvPosL2W / cnvPosW2L / getWrdOffset)
        gam.cnvPosL2W((float *)a2, (float *)a1);
        return 0;
    case 21:
        gam.cnvPosW2L((float *)a2, (float *)a1);
        return 0;
    case 22:
        gam.getWrdOffset((float *)a1);
        return 0;
    case 23:    // is a vfchr's param 2 a1
        return gam.hasVFchrParam(2, (int)a1);
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Messages: a0 picks the call (21 cases, jump table data 0x2ff980).
// 1 / 2 / 5 open the message window on text a1 (1 cuts it at 0x1fe
// characters first), 3 the system message window, 4 sets the message mode,
// 10 / 11 show / hide message sprite 3 with the words at a1, 0x14 starts
// the item-use frame task (kind 0x15), 0x15 waits for it.
// ROR 0x00205928

intptr_t seqFunc006(intptr_t op, intptr_t a1, intptr_t a2, intptr_t, intptr_t, intptr_t)
{
    GamMan &gam = g_gamMan;

    switch ((u_int)op)
    {
    case 1:
    {
        // The ROM's count stops at 0x201 and leaves p one past the byte
        // that ended it; a long text gets two NULs there.
        char *text = (char *)a1;
        char *p = text;
        int   n = 0;
        for (;;)
        {
            char c = *p++;
            if (c == '\0')
                break;
            int more = n < 0x200;
            n++;
            if (!more)
                break;
        }
        if (n >= 0x1fe)
        {
            p[1] = '\0';
            p[0] = '\0';
        }
        gam.openMsg(text, 0, 0xb4);
        return 0;
    }
    case 2:
        gam.openMsg((const char *)a1, (int)a2, 0xb4);
        return 0;
    case 3:
        gam.openSysMsg((const char *)a1, -1, -1, -1, -1);
        return 0;
    case 4:
        gam.m_msgMode = (int)a1;
        return 0;
    case 5:
        gam.openMsg((const char *)a1, 0, (int)a2);
        return 0;
    case 10:
        gam.showMsgSprite(1, (const int *)a1);
        return 0;
    case 11:
        gam.showMsgSprite(0, (const int *)a1);
        return 0;
    case 0x14:  // callcheck: operator_new (new TskFrameFunc)
    {
        gam.m_flags2 |= 0x80000;
        TskFrameFunc *task = new TskFrameFunc(0x15, 0, 0);
        g_taskMan.addTask(1, -1, 1, 0, task, 0, &gam.m_itemUseTaskId, 0x11);
        return 0;
    }
    case 0x15:
        return g_taskMan.getTask(gam.m_itemUseTaskId) != NULL ? SEQ_RET_WAIT : 0;
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Actor motions of object a1 (jump table data 0x300c90, a0 0..32). a2 is
// the motion (a number, or a name where noted), a3 the p1d8 word, t0
// points at the speed (frames per frame), t1 bit 1 gives motion flag
// 0x100. 1 / 10 / 12 and 2 start the motion at 1/60 (2 with a3), killing
// the achr's motion task first; 3 changes to it keeping the heading
// (chrSetMotion); 4 / 9 run it as a TskMtnMove / TskMtnMove2 task on an
// achr (else as 3); 5 is 3 by name; 6 is 4 by name (else a blend by
// name); 7 blends to the motion named by suffix; 8 returns that motion's
// number; 30 the current motion (~no while stopped); 31 / 32 store the
// motion time / length at a2. The others return 0.
// ROR 0x00209528

static const float SEQ_MOTION_STEP = 0x1.11112Ep-6f;  // 0x3c888897, MagScene's 1/60

intptr_t seqFunc100(intptr_t op, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t t0, intptr_t t1)
{
    GamMan   &gam = g_gamMan;
    MagScene &scene = g_magSystem.m_scene;
    u_int     oid = (u_int)a1;
    /* The ROM leaves its stack word unset when getObjRotY finds no actor
     * (which cannot happen past isObj for the achrs it is called for). */
    float     rotY = 0.0f;

    if (!gam.isObj(oid))
        return 0;
    int   no = gam.getActorNo(oid);
    u_int flags = (u_int)(t1 << 7) & 0x100;
    bool  isAchr = (oid >> 28 & 3) == 1;
    const float *speed = (const float *)t0;
    switch ((u_int)op)
    {
    case 1: case 10: case 12:
        chrKillMotionTask(oid);
        scene.setActorMotion(SEQ_MOTION_STEP, no, (int)a2, 0, flags);
        return 0;
    case 2:
        chrKillMotionTask(oid);
        scene.setActorMotion(SEQ_MOTION_STEP, no, (int)a2, (int)a3, flags);
        return 0;
    case 4:
        if (isAchr)
        {
            gam.getObjRotY(oid, &rotY);
            chrMtnMove(*speed, rotY, oid, (int)a2, (int)a3, flags, NULL);
            return 0;
        }
        break;
    case 5:
        chrSetMotion(*speed, no, 0, (int)a3, flags, oid, (const char *)a2);
        return 0;
    case 6:
        if (isAchr)
        {
            gam.getObjRotY(oid, &rotY);
            chrMtnMove(*speed, rotY, oid, 0, (int)a3, flags, (const char *)a2);
        }
        else
        {
            chrKillMotionTask(oid);
            scene.changeActorMotionByName(*speed, no, (const char *)a2, (int)a3, flags);
        }
        return 0;
    case 7:
        gam.getObjRotY(oid, &rotY);
        chrChangeMotionBySuffix(*speed, rotY, oid, (const char *)a2, (int)a3, (int)flags);
        return 0;
    case 8:
    {
        int n = gam.getActorNo(oid);
        if (n == -1)
            return 0;
        return scene.m_actors[n].getMotionNoBySuffix((const char *)a2);
    }
    case 9:
        if (isAchr)
        {
            /* The ROM passes the heading in f13 too; chrMtnMove2 has no
             * use for it. */
            gam.getObjRotY(oid, &rotY);
            chrMtnMove2(*speed, oid, (int)a2, (int)a3, flags, NULL);
            return 0;
        }
        break;
    case 3:
        break;
    case 30:
        return scene.getActorMotion(no);
    case 31:
        *(float *)a2 = scene.m_actors[no].getMotionTime();
        return 0;
    case 32:
        *(float *)a2 = scene.getActorMotionLength(no, -1);
        return 0;
    default:
        return 0;
    }
    /* 3, and 4 / 9 for an object that is no achr. */
    chrSetMotion(*speed, no, (int)a2, (int)a3, flags, oid, NULL);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Motion speed of object a1's actor to the float at a2: a0 1 through the
// scene only, 2 through the achr (which keeps it, +0x418).
// ROR 0x002098e0

intptr_t seqFunc106(intptr_t op, intptr_t a1, intptr_t a2, intptr_t, intptr_t, intptr_t)
{
    GamMan &gam = g_gamMan;
    u_int   oid = (u_int)a1;

    if (!gam.isObj(oid))
        return 0;
    ACHRstat *achr = gam.getAchr(oid);
    if (op == 1)
        g_magSystem.m_scene.setActorMotionSpeed(*(const float *)a2, achr->m_actor);
    else if (op == 2)
        achr->setMotionSpeed(*(const float *)a2);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Turns object a1 (an achr): a0 1 stops, 2 by the angle at a2 at the
// speed at a3, 3 by the angle at a2 over a3 frames, 4 / 5 to the heading
// at a2 at the speed at a3 / over a3 frames, 6 / 7 the same toward object
// a2. Cases 0 and 8.. do nothing (jump table data 0x3010f0).
// ROR 0x0020b3c0

intptr_t seqFunc200(intptr_t op, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t, intptr_t)
{
    GamMan &gam = g_gamMan;
    u_int   oid = (u_int)a1;

    if (!gam.isObj(oid))
        return 0;
    switch ((u_int)op)
    {
    case 1:
        chrRotStart(*(const float *)a2, oid, 0);
        break;
    case 2:
    {
        float angle = *(const float *)a2;
        chrRotStart(angle, oid, chrRotFrames(angle, *(const float *)a3));
        break;
    }
    case 3:
        chrRotStart(*(const float *)a2, oid, (int)a3);
        break;
    case 4:
        chrRotTo(*(const float *)a2, *(const float *)a3, oid);
        break;
    case 5:
        chrRotToIn(*(const float *)a2, oid, (int)a3);
        break;
    case 6:
        chrRotTo(gam.getObjDirTo(oid, (u_int)a2), *(const float *)a3, oid);
        break;
    case 7:
        chrRotToIn(gam.getObjDirTo(oid, (u_int)a2), oid, (int)a3);
        break;
    default:
        break;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The camera: a0 1 / 2 set the scene camera's position / rotation from
// the vector at a1, 3 / 4 read them back into it, 5 / 6 its lens scale
// (the float at a1), 7 the path position a1 (unless flags 0x20000), 8
// the actor of object a1 the camera follows, 10 / 11 camera mode a1
// (11 with map camera a2), 20 / 21 push / pop the camera, 50 / 51 start
// / stop a TskCamera (a2, a1). Jump table data 0x3001c0 (52 cases).
// callcheck: operator_new (new TskCamera)
// ROR 0x00206640

intptr_t seqFunc016(intptr_t op, intptr_t a1, intptr_t a2, intptr_t, intptr_t, intptr_t)
{
    GamMan    &gam = g_gamMan;
    DogCamera &cam = g_magSystem.m_scene.m_camera_entity;
    float     *v = (float *)a1;

    switch ((u_int)op)
    {
    case 1:
        cam.m_trans[0] = v[0];
        cam.m_trans[1] = v[1];
        cam.m_trans[2] = v[2];
        cam.m_dirty = 1;
        break;
    case 2:
        cam.m_rotate[0] = v[0];
        cam.m_rotate[1] = v[1];
        cam.m_rotate[2] = v[2];
        cam.normalizeRotate();
        cam.m_dirty = 1;
        break;
    case 3:
        v[0] = cam.m_trans[0];
        v[1] = cam.m_trans[1];
        v[2] = cam.m_trans[2];
        break;
    case 4:
        v[0] = cam.m_rotate[0];
        v[1] = cam.m_rotate[1];
        v[2] = cam.m_rotate[2];
        break;
    case 5:
        cam.setLensScale(v[0]);
        break;
    case 6:
        v[0] = cam.getLensScale();
        break;
    case 7:
        if ((gam.m_flags & 0x20000) == 0)
            g_magSystem.m_scene.seekCameraPath((float)(u_int)a1);
        break;
    case 8:
        gam.m_21f70 = gam.getActorNo((u_int)a1);
        break;
    case 10:
        gam.setCameraMode((int)a1, 0);
        break;
    case 11:
        gam.setCameraMode((int)a1, (u_int)a2 & 0xffff);
        break;
    case 20:
        gam.pushCamera();
        break;
    case 21:
        gam.popCamera();
        break;
    case 50:
    {
        TskCamera *task = new TskCamera(a2, a1);
        g_taskMan.addTask(1, -1, 1, 0, task, 0, &gam.m_camTaskId, 0x11);
        break;
    }
    case 51:
        if (g_taskMan.getTask(gam.m_camTaskId) != NULL)
        {
            g_taskMan.killTask(gam.m_camTaskId);
            gam.m_camTaskId = -1;
        }
        break;
    default:
        break;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Area triggers (GamMan's GamArea): a0 1..5 add the trigger record at a1
// (1 keeps its id, 2..5 give it a new one; the ROM passes 2..5 a fourth
// word -1, 0, 1, 2 addTrg never reads), 10 / 11 start / stop them, 30
// registers a2 under key a1. The record is flagged 0x3000000 first, and
// with flags 0x103 its rect's corners are ordered. Jump tables data
// 0x300640 (31 cases) and 0x3006c0.
// ROR 0x0020839c

intptr_t seqFunc023(intptr_t op, intptr_t a1, intptr_t a2, intptr_t, intptr_t, intptr_t)
{
    GamArea &area = g_gamMan.m_area;

    switch ((u_int)op)
    {
    case 1: case 2: case 3: case 4: case 5:
    {
        GamAreaTrg *trg = (GamAreaTrg *)a1;
        trg->m_flags |= 0x3000000;
        if ((trg->m_flags & 0x103) != 0)
        {
            for (int i = 0; i < 2; i++)
            {
                float lo = trg->m_rect[i];
                float hi = trg->m_rect[i + 2];
                if (hi < lo)
                {
                    trg->m_rect[i] = hi;
                    trg->m_rect[i + 2] = lo;
                }
            }
        }
        area.addTrg(trg, op != 1);
        break;
    }
    case 10:
        area.startTrgs();
        break;
    case 11:
        area.stopTrgs();
        break;
    case 30:
        area.registFtb((int)a1, (u_int)a2);
        break;
    default:
        break;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// A kind 8 frame task over the words a1..t0: a0 1 on GamMan's frame-task
// id, 2 on its select-task id. Jump table data 0x3006e0 (6 cases).
// callcheck: operator_new (new TskFrameFunc)
// ROR 0x00208518

intptr_t seqFunc026(intptr_t op, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t t0, intptr_t)
{
    GamMan &gam = g_gamMan;
    int    *id;

    if (op == 1)
        id = &gam.m_frameTaskId;
    else if (op == 2)
        id = &gam.m_selectTaskId;
    else
        return 0;
    // The task reads the four words (the ROM's stack block).
    intptr_t words[4] = {a1, a2, a3, t0};
    TskFrameFunc *task = new TskFrameFunc(8, (intptr_t)words, 0);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, id, 0x11);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Ray tests against the sky slots' collision (MagScene::castRay, all
// four): a0 1 from the point a1 along a2, 2 from a1 lowered by 10 toward
// the heading at a2 over the length at a3, 3 from object a1 raised by 10
// along its heading over the length at a2. The hit record goes to a3
// (case 2 always: the length's word is overwritten). 1 when the hit lies
// within the ray (0 <= t <= 1).

struct SeqRayHit
{
    MagSceneRayHit hit;
    u_int          m_60[4];     // the ROM copies 0x70 bytes: 0x10 past MagSceneRayHit (stale stack)
};
static_assert(sizeof(SeqRayHit) == 0x70, "the 0x70 bytes native 156 copies out");

static void seqClearHit(SeqRayHit *h)
{
    for (int k = 0; k < 5; k++)
    {
        h->hit.m_vec[k][0] = 0.0f;
        h->hit.m_vec[k][1] = 0.0f;
        h->hit.m_vec[k][2] = 0.0f;
        h->hit.m_vec[k][3] = 1.0f;
    }
    h->hit.m_50 = -1;
    h->hit._54[0] = h->hit._54[1] = h->hit._54[2] = 0;  // PC: unset in the ROM
    h->m_60[0] = h->m_60[1] = h->m_60[2] = h->m_60[3] = 0;
}

// ROR 0x0020abfc

intptr_t seqFunc156(intptr_t op, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t, intptr_t)
{
    MagScene &scene = g_magSystem.m_scene;
    SeqRayHit h;
    float     p[4], q[4], t;

    seqClearHit(&h);
    switch ((u_int)op)
    {
    case 1:
        memcpy(p, (const float *)a1, sizeof(p));
        memcpy(q, (const float *)a2, sizeof(q));
        t = scene.castRay(&h.hit, p, q, 0);
        if (a3 != 0)
            memcpy((void *)a3, &h, sizeof(h));
        break;
    case 2:
    {
        const float *dir = (const float *)a2;
        float       *len = (float *)a3;
        memcpy(p, (const float *)a1, sizeof(p));
        p[1] -= 10.0f;
        memcpy(q, p, sizeof(q));
        q[0] += sinf(dir[0]) * len[0];
        q[1] += cosf(dir[0]) * len[0];
        t = scene.castRay(&h.hit, p, q, 0);
        memcpy(len, &h, sizeof(h));
        break;
    }
    case 3:
    {
        float r[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // PC: the ROM leaves it unset for a vfchr
        const float *len = (const float *)a2;
        p[0] = p[1] = p[2] = p[3] = 0.0f;       // PC: idem for no object
        g_gamMan.getObjPos((u_int)a1, p);
        g_gamMan.getObjRot((u_int)a1, r);
        p[1] += 10.0f;
        memcpy(q, p, sizeof(q));
        q[0] += sinf(r[1]) * len[0];
        q[1] += cosf(r[1]) * len[0];
        t = scene.castRay(&h.hit, p, q, 0);
        if (a3 != 0)
            memcpy((void *)a3, &h, sizeof(h));
        break;
    }
    default:
        return 0;
    }
    return 0.0f <= t && t <= 1.0f ? 1 : 0;
}

// ──────────────────────────────────────────────────────────────────────
// Achr movement: a0 1 moves object a1 by the vector at a2, 2 by the float
// a2 along its heading; the others start a TskMoveTypeGen on the achr's
// m_task[0] (killing the one there) with a mode (+8 for an achr flagged
// 0x200000), a2 and the frames a3 (10 / 11 / 20 / 21 / 22 a count, 12 /
// 13 / 14 / 23 / 24 / 25 the float at a3). Jump table data 0x301110 (26
// cases).

// The heading wrapped into -pi..pi in double precision, as the ROM does.
static float seqWrapPi(float a)
{
    if ((double)a < -3.141592653589793)
        a = (float)((double)a + 6.283185307179586);
    if ((double)a > 3.141592653589793)
        a = (float)((double)a - 6.283185307179586);
    return a;
}

// Kills the task on id (when it runs) and sets id -1.
static void seqKillTaskOn(int *id)
{
    if (g_taskMan.getTask(*id) != NULL)
    {
        g_taskMan.killTask(*id);
        *id = -1;
    }
}

static const float SEQ_RATE_60 = 0x1.11111p-6f;  // 0x3c888888: one frame of 60

// callcheck: operator_new, getTask, killTask, fptodp, dpcmp, dpadd, dpsub, dptofp (new
// TskMoveTypeGen, seqKillTaskOn, the double compares in seqWrapPi)
// ROR 0x0020b528

intptr_t seqFunc202(intptr_t op, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t t0, intptr_t)
{
    GamMan &gam = g_gamMan;
    u_int   oid = (u_int)a1;
    int     mode, kind;
    float   frames;

    if (!gam.isObj(oid))
        return 0;
    ACHRstat *achr = gam.getAchr(oid);
    int       base = (achr->m_flags & 0x200000) != 0 ? 8 : 0;

    switch ((u_int)op)
    {
    case 1:
        gam.moveAchr(oid, (const float *)a2);
        return 0;
    case 2:
    {
        float r[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // PC: the ROM leaves it unset for a vfchr
        float dist = seqWordFloat(a2);
        float d[4];
        gam.getObjRot(oid, r);
        d[0] = sinf(seqWrapPi(r[1])) * dist;
        d[1] = 0.0f;
        d[2] = cosf(seqWrapPi(r[1])) * dist;
        d[3] = 0.0f;                            // PC: unset in the ROM; moveAchr reads x, y, z
        gam.moveAchr(oid, d);
        return 0;
    }
    case 10: mode = 0; kind = 2; frames = (float)(u_int)a3; break;
    case 11: mode = 1; kind = 2; frames = (float)(u_int)a3; break;
    case 12: mode = 5; kind = 2; frames = *(const float *)a3; break;
    case 13: mode = 3; kind = 2; frames = *(const float *)a3; break;
    case 14: mode = 4; kind = 2; frames = *(const float *)a3; break;
    case 20: mode = 2; kind = 0; frames = (float)(u_int)a3; break;
    case 21: mode = 1; kind = 0; frames = (float)(u_int)a3; break;
    case 22: mode = 6; kind = 0; frames = (float)(u_int)a3; break;
    case 23: mode = 5; kind = 0; frames = *(const float *)a3; break;
    case 24: mode = 3; kind = 0; frames = *(const float *)a3; break;
    case 25: mode = 4; kind = (int)t0; frames = *(const float *)a3; break;
    default:
        return 0;
    }
    seqKillTaskOn(&achr->m_task[0]);
    TskMoveTypeGen *task = new TskMoveTypeGen(oid, base + mode, a2, kind, frames, SEQ_RATE_60);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, &achr->m_task[0], 0x11);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Achr path moves on the achr's m_task[0] (the running one killed): a0
// 10 / 11 a TskMoveTypeRound, 20..23 a TskMovePointPath, 30 a
// TskMoveSectaFindPath, 40..43 / 50 / 51 a TskMoveSecta, 800 kills all
// its tasks. a3 and t0 are float words. Script globals 13 / 14 / 15 give
// the kind, the rate (x 10000) and a mode shift; a zero rate means one
// frame of 60, kind 2, shift 0.
// callcheck: operator_new, getTask, killTask (new TskMove*, seqKillTaskOn)
// ROR 0x0020bd08

intptr_t seqFunc211(intptr_t op, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t t0, intptr_t t1)
{
    GamMan &gam = g_gamMan;
    u_int   oid = (u_int)a1;

    if (!gam.isObj(oid))
        return 0;
    ACHRstat  *achr = gam.getAchr(oid);
    int       *id = &achr->m_task[0];
    const int *glob = g_sequence.m_global;
    int        kind = glob[0xd];
    float      rate = (float)(u_int)glob[0xe] / 10000.0f;
    int        shift = glob[0xf];
    if (rate == 0.0f)
    {
        rate = SEQ_RATE_60;
        kind = 2;
        shift = 0;
    }

    CmnITask *task;
    switch ((u_int)op)
    {
    case 10: case 11:
        seqKillTaskOn(id);
        task = new TskMoveTypeRound(oid, shift * 2 + (int)(op - 10), a2, kind, seqWordFloat(a3),
                                    seqWordFloat(t0), rate);
        break;
    case 20: case 21: case 22: case 23:
        seqKillTaskOn(id);
        task = new TskMovePointPath(oid, shift * 4 + (int)(op - 20), a2, (int)a3, kind,
                                    seqWordFloat(t0), rate);
        break;
    case 30:
        seqKillTaskOn(id);
        task = new TskMoveSectaFindPath(oid, a2, 0, kind, shift, 0x7f, seqWordFloat(t0), rate);
        break;
    case 40: case 41: case 42: case 43: case 50: case 51:
    {
        int      no = op < 50 ? (int)(op - 40) : (int)(op - 46);
        intptr_t arg = op == 50 ? 0 : op == 51 ? t1 : a2;
        seqKillTaskOn(id);
        task = new TskMoveSecta(oid, shift * 6 + no, arg, (int)a3, kind, seqWordFloat(t0), rate);
        break;
    }
    case 800:
        achr->killTasks();
        return 0;
    default:
        return 0;
    }
    g_taskMan.addTask(1, -1, 1, 0, task, 0, id, 0x11);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Enemies over g_emyManager (jump table data 0x3011b0, a0 0..50), a1 the
// enemy's oid: 10 (de)activates it by a2; 20 / 21 read its word +0x3c /
// set +0x238 to a2; 22 its view test (1 when there is no such enemy);
// 23 releases its hold flags; 24 / 30 set the words +0xc / +0x24 of
// manager slot a1 (30 to a2 != 0); 40 its flag bit +0x30; 50 switches
// every type-2 achr by a1 (GamMan::switchEnemies). The others return 0.
// ROR 0x0020c4c8

intptr_t seqFunc212(intptr_t op, intptr_t a1, intptr_t a2, intptr_t, intptr_t, intptr_t)
{
    EmyManager &emy = g_emyManager;
    u_int       oid = (u_int)a1;

    switch ((u_int)op)
    {
    case 10:
        emy.setEmyActive(oid, a2 != 0);
        break;
    case 20:
        return emy.getEmyWord3c(oid);
    case 21:
        emy.setEmyWord238(oid, (int)a2);
        break;
    case 22:
        return emy.testEmyView(oid);
    case 23:
        emy.releaseEmyHold(oid);
        break;
    case 24:
        emy.m_slot[a1].m_0c = (int)a2;
        break;
    case 30:
        emy.setSlotWord24((int)a1, a2 != 0);
        break;
    case 40:
        return emy.isEmyFlag30(oid) != 0;
    case 50:
        g_gamMan.switchEnemies(a1 != 0);
        break;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Sound (jump table data 0x301280, a0 0..200; 131 of the 201 entries do
// nothing): the script's calls into GamSndMan, GamMan's member at
// +0x18e00. 1..8 and 42..47 the sound functions (funcSnd), master
// volumes and reverb; 10..32 SEs; 52..92 streams; 48 and 100..102 SE
// pack swaps; 200 a stream started looping. a1 is the SE / stream id
// (or a name), a2..t1 its arguments; script vectors and names are
// addresses in the script image. Most cases return 0; the queries return
// their result, 81 / 82 wait on one.

// The movie task runs: native 252 plays no SE.
static int seqMoviePlays(void)
{
    return g_taskMan.getTask(g_gamMan.m_movieTaskId) != NULL;
}

// callcheck: operator_new, TskSnd, addTask, getTask (case 30 the new expression; inline in the ROM: seqAddTask, seqMoviePlays)
// ROR 0x0020c6fc

intptr_t seqFunc252(intptr_t id, intptr_t a1, intptr_t a2, intptr_t a3, intptr_t t0, intptr_t t1)
{
    GamMan    &gam = g_gamMan;
    GamSndMan &snd = gam.m_sndMan;
    u_int      se = (u_int)a1;
    int        str = (int)a1;
    // The ROM builds these on its stack and leaves the words it does not
    // set unset; PC: zeroed.
    int         prm[8] = {};    // funcSE / funcStr 5 and 0xb: words 0, 1, 2, 4
    TskSndParam task = {};      // 0x13 (the block TskSnd copies) and case 30

    switch ((u_int)id)
    {
    case 1:
        snd.funcSnd(0xd, (int)a1, (int)a2, 0, -1);
        return 0;
    case 2:
        snd.funcSnd(0xd, (int)a1, (int)a2, (int)a3, -1);
        return 0;
    case 3:  // the 3D centre: 1 the camera
        snd.m_mode = a1 != 0;
        return 0;
    case 4:
        snd.unpush_snd(1, 0, -1);
        snd.funcSnd(0x11, 0x40, 0x40, 0, -1);
        return 0;
    case 5:
        snd.funcSnd(0x12, 0x40, 0x40, 0, -1);
        return 0;
    case 6:  // the SE master volume
        snd.funcSnd(0xe, (int)a1, (int)a2, (int)a3, -1);
        return 0;
    case 7:  // the stream master volume
        snd.funcSnd(0xf, (int)a1, (int)a2, (int)a3, -1);
        return 0;
    case 8:
        snd.setReverb((int)a1, (int)a2, (int)a3);
        return 0;

    case 10:
    case 13:
        if (seqMoviePlays())
            return 0;
        snd.playSE(se, true, NULL, 0);
        return 0;
    case 11:  // stop
        snd.unpush_snd(1, se, -1);
        snd.funcSE(se, 1, NULL);
        return 0;
    case 12:  // at the position a2 (NULL none)
        if (seqMoviePlays())
            return 0;
        snd.playSE(se, false, (float *)a2, 0);
        return 0;
    case 14:
        if (seqMoviePlays())
            return 0;
        snd.playSE(se, (int)a2, (int)a3, seqWordFloat(t0), (int)t1, 0, 0);
        return 0;
    case 15:
        snd.unpush_snd(1, se, -1);
        snd.funcSE(se, 8, NULL);
        return 0;
    case 16:
    case 17:
        prm[0] = (int)a2;
        prm[1] = (int)a3;
        memcpy(&prm[2], (const float *)t0, sizeof(float));
        prm[4] = (int)t1;
        snd.funcSE(se, id == 16 ? 5 : 0xb, prm);
        return 0;
    case 18:
        task._00[0] = (int)a1;
        task._00[1] = (int)a2;
        task._00[2] = (int)a3;
        task._10[1] = (int)t0;
        snd.funcSE(se, 0x13, &task);
        return 0;
    case 19:
        snd.put_se_3d_pos(se, (float *)a2, (int)a3, 0);
        return 0;
    case 20:
        snd.put_se_3d_pos_chr(se, (u_int)a2, 0);
        return 0;
    case 21:
        snd.del_se_3d_pos(se, (int)a2);
        return 0;
    case 22:
        snd.del_se_3d_pos_chr(se);
        return 0;
    case 23:  // in the current room
        snd.put_se_3d_pos(se, (float *)a2, gam.getWrdNo(), 4);
        return 0;
    case 25:  // non-zero while it plays
        return snd.funcSE(se, 0x16, NULL);
    case 26:
    case 27:
    {
        // The ROM passes the address of its spilled a2, t0 next to it.
        int words[2] = { (int)a2, (int)t0 };
        snd.funcSE(se, id == 26 ? 6 : 0xc, words);
        return 0;
    }
    case 28:
        return snd.get_se_oid((const char *)a1);
    case 29:  // loop
        snd.funcSE(se, 4, (void *)a2);
        return 0;
    case 30:
        task._00[0] = (int)a1;
        task._10[1] = (int)a2;
        seqAddTask(new TskSnd(6, &task));
        return 0;
    case 31:
        snd.put_se_3d_pos(se, (float *)a2, (int)a3, 0x20);
        return 0;
    case 32:
        snd.put_se_3d_pos_chr(se, (u_int)a2, 0x20);
        return 0;

    case 40:
        snd.set_se_3d_snd_prm(se, (int)a2, (int)a3, (int)t0, (int)t1);
        return 0;
    case 41:
        snd.set_str_3d_snd_prm(str, (int)a2, (int)a3, (int)t0, (int)t1);
        return 0;
    case 42:
    case 43:
    case 44:
        snd.funcSnd(0x1c + ((int)id - 42), (int)a1, 0x40, 0, -1);
        return 0;
    case 45:
    case 46:
        snd.loadSePack((const char *)a1, id == 46);
        return 0;
    case 47:  // the number in the first two characters of a1
    {
        const char *name = (const char *)a1;
        char        num[3] = { name[0], name[1], 0 };
        snd.funcSnd(0x19, 0, 0, 0, atoi(num));
        return 0;
    }
    case 48:
        gam.loadScriptSePack((const char *)a1);
        return 0;
    case 100:
        gam.loadScriptSePack2((const char *)a1);
        return 0;
    case 101:
        gam.reloadChapterSe1f();
        return 0;
    case 102:
        gam.reloadChapterSe0c();
        return 0;

    case 52:
        return snd.playStr(str, 1, NULL, false, 0);
    case 53:  // stop
        snd.unpush_snd(2, 0, str);
        snd.funcStr(str, 1, NULL);
        return 0;
    case 54:
    case 69:
        snd.playStr(str, (int)a2, (int)a3, seqWordFloat(t0), (int)t1, -1, 0);
        return 0;
    case 55:
        snd.unpush_snd(2, 0, str);
        snd.funcStr(str, 8, NULL);
        return 0;
    case 56:
    {
        int words[2] = { (int)a2, (int)a3 };
        snd.funcStr(str, 2, words);
        return 0;
    }
    case 57:
    {
        int word = (int)a2;     // the ROM stores it through f0
        snd.funcStr(str, 3, &word);
        return 0;
    }
    case 58:  // loop
        snd.funcStr(str, 4, (void *)a2);
        return 0;
    case 59:
    case 60:
        // Unlike 16 / 17, word 2 is t0 itself.
        prm[0] = (int)a2;
        prm[1] = (int)a3;
        prm[2] = (int)t0;
        snd.funcStr(str, id == 59 ? 5 : 0xb, prm);
        return 0;
    case 61:
        task._00[0] = (int)a1;
        task._00[1] = (int)a2;
        task._00[2] = (int)a3;
        task._10[1] = (int)t0;
        snd.funcStr(str, 0x13, &task);
        return 0;
    case 63:
        snd.put_str_3d_pos(str, (float *)a2, (int)a3, 0);
        return 0;
    case 64:
        snd.put_str_3d_pos_chr(str, (u_int)a2, 0);
        return 0;
    case 65:
        snd.del_str_3d_pos(str, (int)a2);
        return 0;
    case 66:
        snd.del_str_3d_pos_chr((u_int)a1);
        return 0;
    case 67:  // in the current room
        snd.put_str_3d_pos(str, (float *)a2, gam.getWrdNo(), 4);
        return 0;
    case 68:
        return snd.playStr(str, (int)a2, NULL, false, 0);
    case 70:
        return snd.getStrNO((const char *)a1);
    case 71:
        snd.playStr(str, -1, (float *)a2, false, 0);
        return 0;
    case 72:
        snd.put_str_3d_pos(str, (float *)a2, (int)a3, 0x20);
        return 0;
    case 73:
        snd.put_str_3d_pos_chr(str, (u_int)a2, 0x20);
        return 0;
    case 80:  // non-zero while it plays
        return snd.funcStr(str, 0x16, NULL);
    case 81:  // until it plays
        return snd.funcStr(str, 0x16, NULL) != 0 ? 0 : SEQ_RET_WAIT;
    case 82:  // while it plays
        return snd.funcStr(str, 0x16, NULL) != 0 ? SEQ_RET_WAIT : 0;
    case 90:
        return snd.playStr(str, 0, NULL, true, 0);
    case 91:
        return snd.isPlayEnvStr();
    case 92:  // 61 on the stream of channel a1
    {
        int no = snd.getStrNoByChan((int)a1);
        if (no == -1)
            return 0;
        task._00[0] = no;
        task._00[1] = (int)a2;
        task._00[2] = (int)a3;
        task._10[1] = (int)t0;
        snd.funcStr(no, 0x13, &task);
        return 0;
    }
    case 200:  // started with flag 0x100, then looped
        snd.playStr(str, 0, NULL, false, 0x100);
        snd.funcStr(str, 4, (void *)1);
        return 0;
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Movies (jump table data 0x3015b0, a0 0..10) over the TskMovie task
// GamMan::m_movieTaskId: 1 starts the movie a1 (a name) once no movie
// task is listed, stopping the sound first, and yields; 2 kills it and
// waits 5 frames; 3 is the task listed; 4 waits while the task is gone
// but its state is still <= 100; 5 the movie's frame count. 0 and 6..10
// return 0.
// ROR 0x0020d428

intptr_t seqFunc253(intptr_t id, intptr_t a1, intptr_t, intptr_t, intptr_t, intptr_t)
{
    GamMan    &gam = g_gamMan;
    BEDMEMINFO info;
    g_utlMemory.getInfo(&info);   // the ROM passes a third word 0; the result is not used
    int task = gam.m_movieTaskId;

    switch ((u_int)id)
    {
    case 1:
    {
        if (g_taskMan.getTask(gam.m_movieTaskId) != NULL)
            return 0;
        g_sndMan.stopAll(1);
        if (g_taskMan.getTask(gam.m_movieTaskId) != NULL)
            return 0;
        // callcheck: operator_new (the new expression)
        TskMovie *movie = new TskMovie((const char *)a1);
        g_taskMan.addTask(1, -1, 1, 0, movie, 0, &gam.m_movieTaskId, 0x11);
        return SEQ_RET_YIELD;
    }
    case 2:
        if (g_taskMan.getTask(gam.m_movieTaskId) != NULL)
            g_taskMan.killTask(gam.m_movieTaskId);
        return g_sequence.waitFrames(5) != 0 ? SEQ_RET_WAIT : 0;
    case 3:
        // PC: the ROM returns getTask's word itself (0 or 1).
        return g_taskMan.getTask(task) != NULL;
    case 4:
    {
        if (g_taskMan.getTask(task) != NULL)
            return 0;
        TskMovie *movie = (TskMovie *)g_taskMan.findTask(task);
        if (movie == NULL)
            return 0;   // PC: the ROM reads the state of a NULL task
        return movie->m_state <= 100 ? SEQ_RET_WAIT : 0;
    }
    case 5:
        return rxMpegGetFrameCount();
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Native 281's helpers. Ops 200 / 201 set / clear bits in sector group
// key of the scene's sector.sec object, all four records.
// ROR 0x0020de34

intptr_t seqSectorFlags(int op, int key, u_int bits)
{
    Obj17c0f0 *sector = &g_magSystem.m_scene.m_8542c;
    if (op == 200)
        sectorSetFlags(sector, key, bits, -1);
    else if (op == 201)
        sectorClearFlags(sector, key, bits, -1);
    return 0;
}

// The pair no (< 50) of the table data 0x3015e0 into *id and *flag;
// returns *id (-1 rows are none), -1 for no out of range. The ROM copies
// the 0x64-byte table to the stack first.

static const signed char s_seqIdPairs[50][2] = {
    { -1, 0 }, { 18, 1 }, { 45, 1 }, { 14, 1 }, { 15, 1 },
    { 20, 1 }, { 23, 1 }, { 29, 0 }, { 49, 0 }, {  5, 1 },
    { 40, 1 }, { 38, 0 }, {  9, 0 }, { -1, 0 }, {  1, 0 },
    {  2, 0 }, {  3, 1 }, {  4, 1 }, {  7, 1 }, { 13, 0 },
    { 17, 0 }, { -1, 0 }, { 22, 1 }, { 24, 1 }, { -1, 0 },
    { 41, 1 }, { 43, 1 }, { 47, 1 }, { 48, 1 }, { 51, 1 },
    { 33, 1 }, { 34, 1 }, { 35, 1 }, { 36, 1 }, { 37, 1 },
    { 50, 1 }, {  8, 0 }, { 52, 1 }, { 21, 1 }, { 26, 0 },
    { 27, 0 }, { 28, 0 }, { -1, 0 }, { -1, 0 }, { -1, 0 },
    { -1, 0 }, { -1, 0 }, { -1, 0 }, { -1, 0 }, { -1, 0 },
};

// ROR 0x0020dea8

int seqGetIdPair(u_int no, int *id, int *flag)
{
    if (no >= 50)
        return -1;
    *id   = s_seqIdPairs[no][0];
    *flag = s_seqIdPairs[no][1];
    return *id;
}

// ──────────────────────────────────────────────────────────────────────
// Scene effects, by a0: 200..299 sector flags (seqSectorFlags); 13 the
// focus (a1 0: setFocus(word 0), 1: focus value 0 lerps from float 0 (< 0
// where it is) to float 1 over word 2 frames, 2: setFocusLerp(word 0,
// float 1, float 2, word 3)); 21 the eye ops of object a1's actor (a2
// 0..3); 38 an effect of kind 0x15 on the actor of object word 0 with
// words 1..12, entered for object a1; 98 shows / hides object a1's effect
// (word 0); 100 ctlObj1cb8e4(a1, a2, 0); 101 releases a1's effects. Other
// a0 below 50 take an effect kind and a flag from seqGetIdPair: the
// effect gets the script's 0x70 bytes at a2 and, with the flag, is
// entered for object a1. a2 points at the words unless noted.
// ROR 0x0020df98

intptr_t seqFunc281(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t, intptr_t, intptr_t)
{
    GamMan      &gam = g_gamMan;
    MagScene    &scene = g_magSystem.m_scene;
    u_int        op = (u_int)a0;
    const int   *word = (const int *)a2;
    const float *fword = (const float *)a2;
    EffParam     param;

    if (op / 100 == 2)
        return seqSectorFlags((int)op, (int)a1, (u_int)a2);

    memset(&param, 0, sizeof(param));   // PC: the ROM's stack words it does not set are garbage
    switch (op)
    {
    case 13:
        if (a1 == 1)
        {
            float from = fword[0];
            if (from < 0.0f)
                from = scene.m_focus_val[0];
            scene.m_focus_val[0] = scene.startLerp(from, fword[1], &scene.m_focus_lerp[0], word[2]);
        }
        else if (a1 == 0)
        {
            scene.setFocus(word[0]);
        }
        else if (a1 == 2)
        {
            scene.setFocusLerp(word[0], fword[1], fword[2], word[3]);
        }
        return 0;
    case 21:
    {
        DogActor *actor = gam.getActor((u_int)a1);
        if (actor == NULL)
            return 0;   // PC: the ROM reads the eye words of a NULL actor
        switch (a2)
        {
        case 0: actor->eyeHold();  break;
        case 1: actor->eyeStart(); break;
        case 2: actor->eyeStep();  break;
        case 3: actor->eyeReset(); break;
        }
        return 0;
    }
    case 38:
        param.m_kind = 0x15;
        param.m_08 = 0;
        param.m_achr.m_actor = gam.getActor((u_int)word[0]);
        memcpy(param.m_achr.m_args, word + 1, sizeof(param.m_achr.m_args));
        gam.addObjFx((u_int)a1, scene.m_837e0.createEff(&param));
        return 0;
    case 98:
    {
        int no = gam.findObjFx((u_int)a1);
        // PC: the ROM indexes the table with -1 when a1 has no effect.
        Eff0 *eff = no >= 0 && no < 0x110 ? scene.m_837e0.m_entry[no] : NULL;
        if (eff != NULL)
            eff->setVisible(word[0] != 0);
        return 0;
    }
    case 99:
        return 0;
    case 100:
        gam.ctlObj1cb8e4((u_int)a1, (int)a2, 0);
        return 0;
    case 101:
        gam.ctlObj1cbb80((u_int)a1);
        return 0;
    }

    int flag;
    if (seqGetIdPair(op, &param.m_kind, &flag) < 0)
        return 0;
    param.m_08 = 0;
    memcpy(param.m_data, (const void *)a2, sizeof(param.m_data));
    int no = scene.m_837e0.createEff(&param);
    if (flag != 0)
        gam.addObjFx((u_int)a1, no);
    return 0;
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
