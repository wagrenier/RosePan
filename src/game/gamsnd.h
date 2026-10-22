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

/* layout: host -- no members reconstructed. */
struct GamSndMan
{
    /* 1 and *entry when id is in the table, else 0 and NULL. */
    int findSnd(int id, GamSndEntry **entry, int arg);                   // ROR 0x001e4908
    /* Non-zero when id's bank is loaded (its handle at +0x1728 is set). */
    int isSndBankLoaded(int id);                                         // ROR 0x001e435c
};

#endif /* _GAME_GAMSND_H */
