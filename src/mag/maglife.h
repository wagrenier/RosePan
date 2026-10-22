/* ==========================================================================
 *  mag/maglife.h
 *
 *  The life objects (mag/maglife.cpp): MagLife0 <- MagLife (vtables
 *  _vt$8MagLife0 0x2e0d30, vptr at +0x14) is the base of everything the
 *  scene's MagLifeMan runs each frame -- the controllers (MagLifePC,
 *  MagLifeBwn, MagLifeBwnCtrl), the menus (MagLifeMenu*), the 2D layers
 *  (S2dBg, S2dMes, S2dCineSco). MagLifeControlChara adds the bound actor.
 *  Minimal: what the scene and MagSystem touch; MagLife0 and MagLife are
 *  one host struct until a TU needs them apart.
 *
 *  EE virtual slots (MagLifeControlChara's, which MagLifePC / MagLifeBwn
 *  override): 1 ~dtor, 2 0x11ade4, 3 MagLife0::attached, 4 getLevel,
 *  5 getName, 6 MagLife::render, 7 setHide, 8 getHide, 9 0x119fe0,
 *  10 setActor, 11 getActor, 12 0x11be60, 13 termWorld, 14 0x11b5e0,
 *  15 renderPostProcess, 16 0x11b30c.
 *  PORT: not polymorphic on the host yet (as dog/dogentity.h): the EE vptr
 *  word at +0x14 is padding, and the slots written code calls are plain
 *  members of the class the scene holds.
 * ======================================================================== */
#ifndef _MAG_MAGLIFE_H
#define _MAG_MAGLIFE_H

#include <stddef.h>

#include "sdk/eetypes.h"

struct DogActor;
struct MagScene;

/* EE at least 0x24. layout: host -- holds the scene pointer. */
struct MagLife
{
    int       m_state;          // ROR 0x00: MagSystem::getPCStateClass / getBwnState switch on it
    u_char    _04[0x0c - 0x04];
    int       m_0c;             // ROR 0x0c: MagSystem asks the PC's for 1 in state 9; role unknown
    int       m_level;          // ROR 0x10: MagLife0::getLevel; the scene's 2D layers get 7
    u_int     _ee_vptr;         // ROR 0x14: the EE vptr (see the file comment)
    u_char    _18[0x20 - 0x18];
    MagScene *m_scene;          // ROR 0x20: the owning scene, set by MagScene::initRender
};
static_assert(offsetof(MagLife, m_0c) == 0x0c, "m_0c");
static_assert(offsetof(MagLife, m_level) == 0x10, "m_level");
static_assert(offsetof(MagLife, m_scene) == 0x20, "m_scene");

/* EE at least 0x60. layout: host -- holds the actor pointer. */
struct MagLifeControlChara : MagLife
{
    /* EE slot 15 (+0x78), inline in the class (linkonce). */
    // ROR 0x00282938
    void renderPostProcess()
    {
        if (m_actor)
            updateMotionSeq();
    }
    void updateMotionSeq();                                              // ROR 0x0011a14c

    u_char    _28[0x50 - 0x28];
    int       m_no;             // ROR 0x50: the actor's number (setActor)
    int       m_mode;           // ROR 0x54: setActor's third argument
    DogActor *m_actor;          // ROR 0x58: the bound actor, NULL = none (asserted in mag/maglife.cpp)
};
static_assert(offsetof(MagLifeControlChara, m_no) >= 0x50, "m_no");
static_assert(offsetof(MagLifeControlChara, m_actor) >= 0x58, "m_actor");

#endif /* _MAG_MAGLIFE_H */
