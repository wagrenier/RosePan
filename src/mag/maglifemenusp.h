/* ==========================================================================
 *  mag/maglifemenusp.h
 *
 *  The "special" menus (mag/maglifemenusp.cpp): MagLifeMenuSpecial (EE
 *  0x80) and MagLifeMenuSpecial2 (EE 0x70), the two menus MagScene::openMenu
 *  allocates (0x1000b; 0x1000c..0x1000f with m_type = id - 0x1000c) and
 *  closeMenu deletes. Minimal: the constructors the scene calls and the
 *  slots MagLifeMenuBase makes pure (EE slots 6, 9..12).
 * ======================================================================== */
#ifndef _MAG_MAGLIFEMENUSP_H
#define _MAG_MAGLIFEMENUSP_H

#include "mag/maglifemenu.h"

/* layout: host -- polymorphic in the ROM (vptr +0x14). */
struct MagLifeMenuSpecial : MagLifeMenuBase
{
    MagLifeMenuSpecial();                                                // ROR 0x0012e23c
    void render(I3dLayer *layer) override;                               // ROR 0x0012dda4
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012d9c8
    void term() override;                                                // ROR 0x0012dac4
    int  isInit() override;                                              // ROR 0x0012db54
    int  processPad(BedPad *pad) override;                               // ROR 0x0012db64
};

/* layout: host -- polymorphic in the ROM (vptr +0x14). */
struct MagLifeMenuSpecial2 : MagLifeMenuBase
{
    MagLifeMenuSpecial2();                                               // ROR 0x0012eef0
    void render(I3dLayer *layer) override;                               // ROR 0x0012e854
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012e34c
    void term() override;                                                // ROR 0x0012e498
    int  isInit() override;                                              // ROR 0x0012e528
    int  processPad(BedPad *pad) override;                               // ROR 0x0012e538
};

#endif /* _MAG_MAGLIFEMENUSP_H */
