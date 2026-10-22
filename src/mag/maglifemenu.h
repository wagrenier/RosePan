/* ==========================================================================
 *  mag/maglifemenu.h
 *
 *  MagLifeMenuBase (mag/maglifemenu.cpp; base MagLife, vptr at +0x14): the
 *  base of the scene's menus (MagLifeMenuItem at scene +0x2be60,
 *  MagLifeMenuBoard2D / 3D, Map, Load, Mcd ...). The scene runs one at a
 *  time through m_pMenuBase (+0x2be5c): MagScene::openMenu inits it, the
 *  scene's update feeds it the pad until processPad returns non-zero,
 *  closeMenu terms it. Minimal: the slots the scene calls.
 *  PORT: not polymorphic on the host yet (open-items Decisions, "Dog vtable
 *  slots before the classes are polymorphic"): the slots are plain members
 *  of the base, so every menu runs the base's (stub) body until the menu
 *  TU makes the class polymorphic. The ROR addresses are MagLifeMenuItem's
 *  slots (the menu at +0x2be60, id 0x10000).
 * ======================================================================== */
#ifndef _MAG_MAGLIFEMENU_H
#define _MAG_MAGLIFEMENU_H

#include <stddef.h>

#include "mag/maglife.h"

struct BedPad;
struct DogTexInfo;

/* layout: host -- polymorphic in the ROM (vptr +0x14); EE offsets in the
 * comments. */
struct MagLifeMenuBase : MagLife
{
    /* EE slot 1 (+0x08): the deleting destructor closeMenu runs on the
     * menus openMenu allocated. */
    ~MagLifeMenuBase();                                                  // ROR 0x00284040 (MagLifeMenuItem's)
    /* EE slot 9 (+0x48): loads the menu's textures into tex. */
    void init(DogTexInfo *tex);                                          // ROR 0x0012d040 (MagLifeMenuItem's)
    /* EE slot 10 (+0x50) */
    void term();                                                         // ROR 0x002840b0 (MagLifeMenuItem's)
    /* EE slot 11 (+0x58): non-zero once init has run. */
    int  isInit();                                                       // ROR 0x002840cc (MagLifeMenuItem's)
    /* EE slot 12 (+0x60): one frame of input; non-zero closes the menu. */
    int  processPad(BedPad *pad);                                        // ROR 0x0012d06c (MagLifeMenuItem's)

    u_char    _28[0x30 - 0x28];
    MagScene *m_menu_scene;     // ROR 0x30: openMenu stores the scene before init
    int       m_menu_id;        // ROR 0x34: openMenu's id for the id families 0x20001.. and the 0x1000c.. menus
    int       m_menu_arg;       // ROR 0x38: requestMenu's argument
};
static_assert(offsetof(MagLifeMenuBase, m_menu_scene) >= 0x30, "m_menu_scene");
static_assert(offsetof(MagLifeMenuBase, m_menu_arg) >= 0x38, "m_menu_arg");

#endif /* _MAG_MAGLIFEMENU_H */
