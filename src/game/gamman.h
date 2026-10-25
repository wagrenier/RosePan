/* ==========================================================================
 *  game/gamman.h
 *
 *  GamMan, the game manager (game/gamman.cpp; 181 inline methods in the
 *  ELF), one global at bss 0x7812c0. In the ROM it is polymorphic (vptr at
 *  +0x2ab4, slot 1 the destructor 0x1c0c8c); the virtuals arrive with the
 *  gamman TU. Minimal: only the methods other TUs call are declared.
 * ======================================================================== */
#ifndef _GAME_GAMMAN_H
#define _GAME_GAMMAN_H

#include <stdint.h>

#include "game/gamsnd.h"

/* GamMan's member at +0x40 (bss 0x781300; its code is in
 * unassigned@001dd5e8, class name invented): a list of rects at +0xc290
 * tested against the PC's position. layout: host -- no members. */
struct GamArea
{
    void checkPc();                                                      // ROR 0x001deaac
    void startPcAction();                                                // ROR 0x001dea40
};

/* EE size at least 0x22368. layout: host -- only the members other TUs
 * read are declared. Items are short ids, 0xa00 = none. */
struct GamMan
{
    void init();                                                         // ROR 0x001c0d70
    void loadSystemData();                                               // ROR 0x001c1b48
    void loadStage(int stage);                                           // ROR 0x001c204c
    void setPcStartPos();                                                // ROR 0x001c3d38
    void update();                                                       // ROR 0x001c526c
    void postUpdate();                                                   // ROR 0x001c53d8
    /* Stops the stage sound and the tasks before a menu opens; kind 1 / 2
     * also sets Sequence flag 1. */
    void enterMenu(int kind);                                            // ROR 0x001c5bf4
    /* Halts / restarts parts of the game by mask (0x100000 before the
     * menus; resume(4) once the PC's motion is over). */
    void resume(u_int mask);                                             // ROR 0x001c65e8
    void pause(u_int mask);                                              // ROR 0x001c6a40
    /* Copies one status word (which = 1.., see the switch) to *out. */
    void getStatus(int which, void *out);                                // ROR 0x001c6ccc
    void equipItem(short id);                                            // ROR 0x001c7a4c
    void addItem(short id, int arg);                                     // ROR 0x001c7e40
    void delItem(short id, int arg);                                     // ROR 0x001c82f8
    void delItemAll(short id);                                           // ROR 0x001c8414
    void selectItem(short id);                                           // ROR 0x001c8598
    int  hasItem(short id, int withStock);                               // ROR 0x001c8a28
    int  getItemCount(short id, int withStock);                          // ROR 0x001c8ae8
    int  canAddItem(int arg, short id);                                  // ROR 0x001c8ca4
    int  addStock(short id, int count);                                  // ROR 0x001c8f7c
    int  delStock(short id, int one);                                    // ROR 0x001c94d8
    /* Empty in the US build (before the map menu). */
    void beforeMapMenu();                                                // ROR 0x001c98b8
    /* Keeps a script image {w, h, ...} in slot. */
    void setImage(int slot, u_char *image);                              // ROR 0x001cbdfc
    void storePcTransform();                                             // ROR 0x001cd404
    void setSysMsgHandle(int handle);                                    // ROR 0x001cfb38
    /* MagScene's open menu id != -1. */
    int  isMenuOpen();                                                   // ROR 0x001d1a48
    /* 0: saves the volume in m_saved_volume and silences; 1: restores
     * the saved volume (when one is saved) and marks none saved. */
    void switchVolume(int on);                                           // ROR 0x001d3ac8
    void setVolume(int volume);                                          // ROR 0x001d3a38

    /* An {a, b} pair the system calls 0x32..0x35 set (in the ROM b, the
     * script's a2, comes first). layout: host (two ints, EE 8 bytes). */
    struct SeqPair
    {
        int m_b;                // ROR 0x0: the script's a2
        int m_a;                // ROR 0x4: the script's a1
    };

    GamArea   m_area;           // ROR 0x40 (bss 0x781300)
    GamSndMan m_sndMan;         // ROR 0x18e00 (bss 0x79a0c0): the sound table
    u_int     m_flags;          // ROR 0x21f48: 0x400 set when a script opens a menu
    u_int     m_flags2;         // ROR 0x21f4c: 0x2000 / 0x4000 read by system call 0x22
    int       m_pcActor;        // ROR 0x21fdc: the PC's actor number in the scene
    int       m_pcMotion;       // ROR 0x21fe4: the motion system call 0x2a waits on
    int       m_sysWord[4];     // ROR 0x22050: system call 0x67 copies four words here
    short     m_equipItem;      // ROR 0x2220c: the equipped item, 0xa00 = none
    int       m_saved_volume;   // ROR 0x22214: -1 = none saved (SprMan::volumeOn / Off read it)
    int       m_frameTaskId;    // ROR 0x222ac: the TskFrameFunc the system call starts
    int       m_sysTaskId;      // ROR 0x222b0: the TskSysFunc of system call 0x22
    SeqPair   m_seqPair[4];     // ROR 0x222c4: system calls 0x35, 0x32, 0x33, 0x34
    intptr_t  m_seqWord;        // ROR 0x22364: system call 0x15 stores its a1
};

extern GamMan g_gamMan;         /* bss 0x7812c0 */

#endif /* _GAME_GAMMAN_H */
