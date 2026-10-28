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
 *  Polymorphic on the host since mag/maglifeman.cpp (MagLifeMan runs slots
 *  1 / 2 / 6 / 8 on lives of any class): slots 1, 2, 6, 7, 8 are real
 *  virtuals; 3 attached, 4 getLevel, 5 getName (pure in MagLife0) and 9..
 *  are declared when a TU needs them (the classes' other slots are still
 *  plain members, called on the known class). The host vptr sits at offset
 *  0, so the EE offsets below are comments with >= asserts.
 * ======================================================================== */
#ifndef _MAG_MAGLIFE_H
#define _MAG_MAGLIFE_H

#include <stddef.h>

#include "sdk/eetypes.h"

struct DogActor;
struct GamSecta;
struct I3dLayer;
struct MagScene;

/* EE at least 0x28 (MagLife0 0x24 + MagLife's hide word). layout: host --
 * polymorphic (EE vptr at +0x14). MagLife0 and MagLife are one host class. */
struct MagLife
{
    /* EE slot 1 (+0x08): MagLife0's restores _vt$8MagLife0; flags & 1
     * deletes (MagLifeMan::deleteAll passes 3). */
    virtual ~MagLife() {}                                                // ROR 0x00282aa0
    /* EE slot 2 (+0x10): counts the frames in m_04 (MagLife0::live; MagLife's
     * own 0x283e44 is the same). MagLifeMan::update runs it. */
    virtual void live() { m_04++; }                                      // ROR 0x00282ad4
    /* EE slot 6 (+0x30): draws into layer; nothing by default.
     * MagLifeMan::draw runs it while getHide() is 0. */
    virtual void render(I3dLayer *layer) {}                              // ROR 0x00282844
    /* EE slot 7 (+0x38) */
    virtual void setHide(int hide) { m_hide = hide; }                    // ROR 0x0028284c
    /* EE slot 8 (+0x40) */
    virtual int  getHide() { return m_hide; }                            // ROR 0x00282854

    int       m_state;          // ROR 0x00: MagSystem::getPCStateClass / getBwnState switch on it
    int       m_04;             // ROR 0x04: the constructors set -1; role unknown
    int       m_08;             // ROR 0x08: the constructors clear it
    int       m_0c;             // ROR 0x0c: MagSystem asks the PC's for 1 in state 9; role unknown
    int       m_level;          // ROR 0x10: MagLife0::getLevel; the scene's 2D layers get 7
    /* ROR 0x14: the EE vptr (the host's is at 0) */
    u_char    _18[0x20 - 0x18];
    MagScene *m_scene;          // ROR 0x20: the owning scene, set by MagScene::initRender
    int       m_hide;           // ROR 0x24: MagLife::setHide / getHide; the constructors clear it
};
static_assert(offsetof(MagLife, m_0c) >= 0x0c, "m_0c");
static_assert(offsetof(MagLife, m_level) >= 0x10, "m_level");
static_assert(offsetof(MagLife, m_scene) >= 0x20, "m_scene");
static_assert(offsetof(MagLife, m_hide) >= 0x24, "m_hide");

/* One row of a controller's motion sequence table (0x18 bytes; the PC's
 * is at 0x2961e0: "base", "run_out", "wait" ..., Brown's at 0x297418).
 * Read by updateMotionSeq; only the words other files read are named
 * yet (the session that writes updateMotionSeq lays out the rest). */
struct MagLifeMotionSeq
{
    u_char  _00[0x10];
    u_short m_flags;            // ROR 0x10: & 0xf0 is MagLifeBwn::suspend's class (0x10 wait, 0x30 stop)
    short   m_alt;              // ROR 0x12: changeMotionSeqAlt's alternative row
    u_char  _14[0x18 - 0x14];
};
static_assert(offsetof(MagLifeMotionSeq, m_flags) == 0x10, "m_flags");
static_assert(sizeof(MagLifeMotionSeq) == 0x18, "MagLifeMotionSeq is 0x18 bytes on the EE");

/* EE at least 0x7c. layout: host -- holds the actor pointer. */
struct MagLifeControlChara : MagLife
{
    /* m_level 1, m_04 / m_no -1, the rest cleared; the derived
     * constructors set m_motion_seq. */
    MagLifeControlChara();                                               // ROR 0x0011a888

    /* EE slot 9 (+0x48): the state quad m_state..m_0c into m_prev_state
     * (when m_state != 0), then m_state = state, m_08 = arg, m_04 = -1.
     * Called on the known class (MagLifePC::changeState). */
    void setState(int state, int arg);                                   // ROR 0x00119fe0
    /* m_seq_no / m_seq_count = -1. */
    void clearMotionSeq();                                               // ROR 0x0011a130
    void setMotionSeqTable(const MagLifeMotionSeq *table);               // ROR 0x0011a144
    /* m_70 = -1, m_secta cleared. */
    void clearGoal();                                                    // ROR 0x00119b34
    /* On the motion's event bit 1: the sound table[floor kind] (+ the
     * frame count's parity / 13) at the actor (playSe). */
    void playStepSe(const int *table);                                   // ROR 0x0011a738
    /* EE slot 15 (+0x78), inline in the class (linkonce). */
    // ROR 0x00282938
    void renderPostProcess()
    {
        if (m_actor)
            updateMotionSeq();
    }
    void updateMotionSeq();                                              // ROR 0x0011a14c
    /* m_seq_no = no, the phase words cleared. */
    void setMotionSeqNo(int no);                                         // ROR 0x0011a010
    /* The secta query (radius 40) at the actor, ahead along its heading
     * when ahead != 0 (that point into out when given): -1 without an
     * actor or a cell, 2 on a cell flagged 0x20000, else 0. */
    int  testFloor(float ahead, float *out);                             // ROR 0x00119cec
    /* 1 when a row along m_seq_no's alternatives (+0x12) loops (+0x0a <
     * 0), 0 when the chain ends. */
    int  isMotionStateEndless();                                         // ROR 0x0011a064
    /* utlPlaySe(se) at the actor (nothing without one). */
    void playSe(int se);                                                 // ROR 0x0011a704
    /* m_seq_no. */
    int  getMotionSeq();                                                 // ROR 0x0011a028
    /* m_48 > 1: the row's motion has ended. */
    int  isMotionSeqEnd();                                               // ROR 0x0011a054
    /* Row theState of m_tbl_motion_state (m_motion_seq) and its motion;
     * blend < 0: the row's own blend frames. Nothing while it already
     * plays; row 0 only clears. */
    void changeMotionSeq(int theState, int blend);                       // ROR 0x0011a320
    /* changeMotionSeq of the row's alternative (+0x12) when it has one. */
    void changeMotionSeqAlt(int theState, int blend);                    // ROR 0x0011a27c
    /* The actor's m_trans into out (untouched without an actor). */
    void getPosition(float *out);                                        // ROR 0x00119f88
    /* DogActor::getRotY, 0 without an actor. */
    float getRotY();                                                     // ROR 0x00119fb0
    /* The actor's m_trans itself (no actor check). */
    float *getTransPtr();                                                // ROR 0x00119fa4
    /* A ray from 60 above the actor over 120: m_trans.y follows the floor
     * by (t - 0.5) * 60, at most 7 a call; returns the step. */
    float followGround();                                                // ROR 0x00119e1c
    /* The secta query (radius 40) at the actor into m_secta, searched
     * from the cells it holds when the first is set. */
    void updateSectaCells();                                             // ROR 0x0011a6b0
    /* m_secta, queried at the actor first when its first cell is NULL
     * (MagLifeBwn::searchPath's nowsct). */
    GamSecta **getSectaCells();                                          // ROR 0x0011a64c

    u_char    _28[0x30 - 0x28];
    int       m_prev_state[4];  // ROR 0x30: setState saves m_state, m_04, m_08, m_0c here
    int       m_seq_no;         // ROR 0x40: the motion sequence row (clearMotionSeq: -1)
    int       m_seq_count;      // ROR 0x44: its frame count (clearMotionSeq: -1)
    int       m_48;             // ROR 0x48: the row's phase (isMotionSeqEnd: > 1); the constructor clears it
    int       m_4c;             // ROR 0x4c: the constructor clears it
    int       m_no;             // ROR 0x50: the actor's number (setActor)
    int       m_mode;           // ROR 0x54: setActor's third argument
    DogActor *m_actor;          // ROR 0x58: the bound actor, NULL = none (asserted in mag/maglife.cpp)
    const MagLifeMotionSeq *m_motion_seq; // ROR 0x5c: setMotionSeqTable (the asserts' m_tbl_motion_state)
    GamSecta *m_secta[4];       // ROR 0x60: the secta cells under the actor, one per list (SctWorld::findCells); clearGoal clears them, the constructor the first
    int       m_70;             // ROR 0x70: clearGoal sets -1
    int       m_74;             // ROR 0x74: the constructor clears it
    int       m_78;             // ROR 0x78: MagLifePC::setActor: 0 while the fade state is 5, else 1
};
static_assert(offsetof(MagLifeControlChara, m_prev_state) >= 0x30, "m_prev_state");
static_assert(offsetof(MagLifeControlChara, m_seq_no) >= 0x40, "m_seq_no");
static_assert(offsetof(MagLifeControlChara, m_no) >= 0x50, "m_no");
static_assert(offsetof(MagLifeControlChara, m_actor) >= 0x58, "m_actor");
static_assert(offsetof(MagLifeControlChara, m_motion_seq) >= 0x5c, "m_motion_seq");
static_assert(offsetof(MagLifeControlChara, m_78) >= 0x78, "m_78");

#endif /* _MAG_MAGLIFE_H */
