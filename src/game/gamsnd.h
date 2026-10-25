/* ==========================================================================
 *  game/gamsnd.h
 *
 *  GamSndMan (game/gamsnd.cpp; vtable 0x2e4840, vptr at +0x1874,
 *  constructor 0x1e0df0): the game's sound table, GamMan's member at
 *  +0x18e00 (bss 0x79a0c0). SndMan looks sound ids up in it. Minimal:
 *  only what SndMan calls.
 * ======================================================================== */
#ifndef _GAME_GAMSND_H
#define _GAME_GAMSND_H

#include <stddef.h>

#include "sdk/eetypes.h"

/* One sound of the table (EE 0x38 bytes, an array at *(GamSndMan +
 * 0x1540), ended by id 0 or 0x400 entries). The name is what
 * SndMan::loadBankById hands rxSndXvabLoad. */
struct GamSndEntry
{
    char     m_name[0x1c];      // ROR 0x00
    int      m_id;              // ROR 0x1c
    u_short  m_bank;            // ROR 0x20: SndMan bank number
    u_short  m_prog;            // ROR 0x22
    u_short  m_note;            // ROR 0x24
    u_short  _26[6];
    u_short  m_32;              // ROR 0x32: isSndBankLoaded's second argument
    int      _34;
};
static_assert(sizeof(GamSndEntry) == 0x38, "GamSndEntry");
static_assert(offsetof(GamSndEntry, m_bank) == 0x20, "m_bank");
static_assert(offsetof(GamSndEntry, m_32) == 0x32, "m_32");

/* layout: host -- only the words GamMan reads. GamMan's same-named inline
 * methods (registSnd, funcSnd, clear_snd_lst, loadSePack, the ELF's
 * symbols) forward to these. */
struct GamSndMan
{
    void init();                                                         // ROR 0x001e0f1c
    /* Every frame (GamMan::update; pos unused: mode 1 takes the camera's). */
    void update(const float *pos);                                       // ROR 0x001e100c
    /* Frees bank slot n (SndMan::freeBank) and marks it -1. */
    void freeSePack(int n);                                              // ROR 0x001e46b8
    void loadSePack(const char *name, void *data, int arg);              // ROR 0x001e4458
    void registSnd(int state, void *data);                               // ROR 0x001e4790
    void registStrSnd(int state, void *data);                            // ROR 0x001e484c
    void clear_snd_lst(int arg);                                         // ROR 0x001e6570
    /* Sound function no (SND_FUNC_NO: 0xe / 0xf the SE / stream master
     * volumes, 0x11 at loadSystemData). */
    void funcSnd(int no, int a, int b, int c, int d);                    // ROR 0x001e6734
    /* 1 and *entry when id is in the table, else 0 and NULL. */
    int findSnd(int id, GamSndEntry **entry, int arg);                   // ROR 0x001e4908
    /* Non-zero when id's bank is loaded (its handle at +0x1728 is set). */
    int isSndBankLoaded(int id);                                         // ROR 0x001e435c
    /* Fades the current BGM slot out over 0x1e frames (mode 1 keeps it,
     * 2 works on the other slot +0x1708). */
    void stopBgm(int mode);                                              // ROR 0x001e17b4
    /* GamMan::resume's call (0, 0, 1): restarts the BGM slot (+0x1684,
     * reset to 0 when reset is set). */
    void resumeBgm(int a, int mode, int reset);                          // ROR 0x001e1920

    int m_mode;                 // ROR 0x0000: loadStage sets 1 (update then uses the camera position)
    int m_mstVolSe[2];          // ROR 0x1590: option 3's funcSnd(0xe) words
    int m_mstVolStr[2];         // ROR 0x1598: GamMan::getMstVolStr, option 2's funcSnd(0xf) words
};

#endif /* _GAME_GAMSND_H */
