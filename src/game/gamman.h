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

/* EE size at least 0x22098. layout: host -- no members reconstructed. */
struct GamMan
{
    void init();                                                         // ROR 0x001c0d70
    void loadSystemData();                                               // ROR 0x001c1b48
    void loadStage(int stage);                                           // ROR 0x001c204c
    void update();                                                       // ROR 0x001c526c
    void postUpdate();                                                   // ROR 0x001c53d8
};

extern GamMan g_gamMan;         /* bss 0x7812c0 */

#endif /* _GAME_GAMMAN_H */
