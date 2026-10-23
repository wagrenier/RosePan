/* ==========================================================================
 *  mag/maglifemenusp.h
 *
 *  The "special" menus (mag/maglifemenusp.cpp): MagLifeMenuSpecial (EE
 *  0x80) and MagLifeMenuSpecial2 (EE 0x70), the two menus MagScene::openMenu
 *  allocates (0x1000b; 0x1000c..0x1000f with m_menu_id = id - 0x1000c) and
 *  closeMenu deletes. Minimal: the constructors the scene calls.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEMENUSP_H
#define _MAG_MAGLIFEMENUSP_H

#include "mag/maglifemenu.h"

/* layout: host -- polymorphic in the ROM (vptr +0x14). */
struct MagLifeMenuSpecial : MagLifeMenuBase
{
    MagLifeMenuSpecial();                                                // ROR 0x0012e23c
};

/* layout: host -- polymorphic in the ROM (vptr +0x14). */
struct MagLifeMenuSpecial2 : MagLifeMenuBase
{
    MagLifeMenuSpecial2();                                               // ROR 0x0012eef0
};

#endif /* _MAG_MAGLIFEMENUSP_H */
