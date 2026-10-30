/* ==========================================================================
 *  mag/maglifemenusp.h
 *
 *  The "special" menus (mag/maglifemenusp.cpp): two dial locks shown as
 *  scene actors under the scene's camera, opened by MagScene::openMenu
 *  (0x1000b MagLifeMenuSpecial; 0x1000c..0x1000f MagLifeMenuSpecial2 with
 *  m_type = id - 0x1000c) and deleted by closeMenu.
 *
 *  Both load their models as scene actors with actor flag 8 (they keep
 *  their motions running under the menu), save the scene's camera, place
 *  it and fade in, then let the pad turn three dials: left / right pick a
 *  dial (its actor's attention 0 / 1, the others -1), up / down turn the
 *  picked one a step and render eases it there 0x100 a frame. The frame
 *  counter is MagLife's m_04 (MagLife::live counts it; -1 on each step
 *  change). processPad returns m_result once the menu is done: 1 cancelled
 *  (pad bit 13), 2 opened.
 *
 *  MagLifeMenuSpecial (EE 0x80, vtable 0x2e1c38): the dials have eight
 *  positions (0x1000 of a 0x8000 turn) and open when all three stand at
 *  -0x4000 (pad bit 14); the box (actor 0) then plays motion 0.
 *  MagLifeMenuSpecial2 (EE 0x70, vtable 0x2e1bc8): ten digits (0x1000 of
 *  0xa000); pad bit 14 tries the code m_menu_arg (three decimal digits,
 *  hundreds on the first dial), the camera having zoomed onto the body
 *  (actor 0, one model per m_type) first; a wrong code shakes the lock
 *  and lets the dials turn again, the right one plays the body's motion 1.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEMENUSP_H
#define _MAG_MAGLIFEMENUSP_H

#include <stddef.h>

#include "mag/maglifemenu.h"

/* layout: host -- polymorphic in the ROM (vptr +0x14). */
struct MagLifeMenuSpecial : MagLifeMenuBase
{
    MagLifeMenuSpecial();                                                // ROR 0x0012e23c
    ~MagLifeMenuSpecial();                                               // ROR 0x0012e2e4
    const char *getName() { return NULL; }                               // ROR 0x00284f08
    void render(I3dLayer *theRenderStage) override;                      // ROR 0x0012dda4
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012d9c8
    void term() override;                                                // ROR 0x0012dac4
    int  isInit() override;                                              // ROR 0x0012db54
    int  processPad(BedPad *pad) override;                               // ROR 0x0012db64

    /* m_state (MagLife +0x00) is the menu's step: 0 set up, 1 fading in,
     * 2 dials idle, 3 dials turning, 4 opening, 5 cancelled (fading out),
     * 6 done. */
    int       m_actor_no[5];    // ROR 0x40: the scene actors: 0 box, 1..3 dials, 4 frame; -1 = none
    int       m_sel;            // ROR 0x54: the picked dial, 0..2
    int       m_cur[3];         // ROR 0x58: the dials' angles (0x8000 a turn), eased towards m_target
    int       m_target[3];      // ROR 0x64: where the pad turned them, kept in -0x4000..0x3fff
    int       m_result;         // ROR 0x70: processPad's value once done: 1 cancelled, 2 opened
    u_char    _74[0x80 - 0x74];
};
static_assert(offsetof(MagLifeMenuSpecial, m_actor_no) >= 0x40, "m_actor_no");
static_assert(offsetof(MagLifeMenuSpecial, m_sel) >= 0x54, "m_sel");
static_assert(offsetof(MagLifeMenuSpecial, m_cur) >= 0x58, "m_cur");
static_assert(offsetof(MagLifeMenuSpecial, m_target) >= 0x64, "m_target");
static_assert(offsetof(MagLifeMenuSpecial, m_result) >= 0x70, "m_result");

/* layout: host -- polymorphic in the ROM (vptr +0x14). */
struct MagLifeMenuSpecial2 : MagLifeMenuBase
{
    MagLifeMenuSpecial2();                                               // ROR 0x0012eef0
    ~MagLifeMenuSpecial2();                                              // ROR 0x0012efa0
    const char *getName() { return NULL; }                               // ROR 0x00284f60
    void render(I3dLayer *theRenderStage) override;                      // ROR 0x0012e854
    void init(DogTexInfo *tex) override;                                 // ROR 0x0012e34c
    void term() override;                                                // ROR 0x0012e498
    int  isInit() override;                                              // ROR 0x0012e528
    int  processPad(BedPad *pad) override;                               // ROR 0x0012e538

    /* m_state is the menu's step: 0 set up, 1 fading in, 2 the camera
     * closing in, 3 dials idle, 4 dials turning, 5 trying the code,
     * 6 opening, 7 cancelled (fading out), 8 done. */
    int       m_actor_no[4];    // ROR 0x40: the scene actors: 0 the body (by m_type), 1..3 dials; -1 = none
    int       m_sel;            // ROR 0x50: the picked dial, 0..2
    int       m_cur[3];         // ROR 0x54: the dials' angles (0xa000 a turn, 0x1000 a digit), eased towards m_target
    int       m_target[3];      // ROR 0x60: where the pad turned them, kept in -0x5000..0x4fff
    int       m_result;         // ROR 0x6c: processPad's value once done: 1 cancelled, 2 opened; 0 after a wrong code
};
static_assert(offsetof(MagLifeMenuSpecial2, m_actor_no) >= 0x40, "m_actor_no");
static_assert(offsetof(MagLifeMenuSpecial2, m_sel) >= 0x50, "m_sel");
static_assert(offsetof(MagLifeMenuSpecial2, m_cur) >= 0x54, "m_cur");
static_assert(offsetof(MagLifeMenuSpecial2, m_target) >= 0x60, "m_target");
static_assert(offsetof(MagLifeMenuSpecial2, m_result) >= 0x6c, "m_result");

#endif /* _MAG_MAGLIFEMENUSP_H */
