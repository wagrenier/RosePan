/* ==========================================================================
 *  mag/magsystem.h
 *
 *  MagSystem (mag/magsystem.cpp; asserts name m_v_sema): the game's system
 *  object, one global at bss 0x6e9840 right after the game heap. It owns
 *  the frame: double buffer, V-blank semaphore and callback, the scene
 *  member at +0x12540, the per-frame update main's loop calls. Minimal:
 *  only the methods main calls are declared so far.
 * ======================================================================== */
#ifndef _MAG_MAGSYSTEM_H
#define _MAG_MAGSYSTEM_H

#include "mag/magscene.h"

/* EE size at least 0x97a74 (setLanguage writes +0x97a6c / +0x97a70).
 * layout: host -- only the scene member is placed; the rest comes with the
 * magsystem TU. */
struct MagSystem
{
    bool setLanguage(int lang);                                          // ROR 0x001178b8
    void resetDBuff();                                                   // ROR 0x00117c58
    void init();                                                         // ROR 0x00117dc8
    void start();                                                        // ROR 0x00117e18
    void term();                                                         // ROR 0x00117ed0
    void setSceneFlags(int a, int b, int c, int d, int e, int f, int g); // ROR 0x00117f80
    void update();                                                       // ROR 0x001182dc

    MagScene m_scene;           // ROR 0x12540 (bss 0x6fbd80)
};

extern MagSystem g_magSystem;   /* bss 0x6e9840 */

#endif /* _MAG_MAGSYSTEM_H */
