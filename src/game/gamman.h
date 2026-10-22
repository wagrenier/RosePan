/* ==========================================================================
 *  game/gamman.h
 *
 *  GamMan, the game manager (game/gamman.cpp; 181 inline methods in the
 *  ELF), one global at bss 0x7812c0. In the ROM it is polymorphic (vptr at
 *  +0x2ab4, slot 1 the destructor 0x1c0c8c); the virtuals arrive with the
 *  gamman TU. Minimal: only the methods main reaches are declared.
 * ======================================================================== */
#ifndef _GAME_GAMMAN_H
#define _GAME_GAMMAN_H

#include "game/gamsnd.h"

/* EE size at least 0x2221c. layout: host -- only the members other TUs
 * read are declared. */
struct GamMan
{
    void init();                                                         // ROR 0x001c0d70
    void loadSystemData();                                               // ROR 0x001c1b48
    void loadStage(int stage);                                           // ROR 0x001c204c
    void update();                                                       // ROR 0x001c526c
    void postUpdate();                                                   // ROR 0x001c53d8
    /* 0: saves the volume in m_saved_volume and silences; 1: restores
     * the saved volume (when one is saved) and marks none saved. */
    void switchVolume(int on);                                           // ROR 0x001d3ac8
    void setVolume(int volume);                                          // ROR 0x001d3a38

    GamSndMan m_sndMan;         // ROR 0x18e00 (bss 0x79a0c0): the sound table
    int m_saved_volume;         // ROR 0x22214: -1 = none saved (SprMan::volumeOn / Off read it)
};

extern GamMan g_gamMan;         /* bss 0x7812c0 */

#endif /* _GAME_GAMMAN_H */
