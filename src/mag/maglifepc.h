/* ==========================================================================
 *  mag/maglifepc.h
 *
 *  MagLifePC (mag/maglifepc*.cpp; vtable _vt$9MagLifePC 0x2e0d68, base
 *  MagLifeControlChara, vptr at +0x14, constructor 0x11bf0c): the player
 *  character's controller, EE 0x220 bytes. The scene holds one at +0x2adc0
 *  and binds the player's actor to it (MagScene::setPC).
 *
 *  m_state (MagLife0 +0x00) is the PC's state; each has a mode object
 *  embedded here (mag/maglifepcmode.h lists them) that selectMode makes
 *  current. changeState enters a state: the base's slot 9 setState saves
 *  the old one and selectMode(1) runs the new mode's init. live runs the
 *  mode each frame, processPad reads the pad into the status words
 *  (health, move level, stance) and hands it to the mode. message is the
 *  event / script interface (MagScene::commandPC).
 *
 *  mag/maglifepc.cpp is 0x11ab40..0x11c464 (pinned 2026-10-27); the class's
 *  other members live in maglifepc2.cpp (0x11c64c..: items, attacks),
 *  maglifepc3.cpp (0x11d720..: motions, initMotion), maglifepcmove.cpp,
 *  maglifepcbtl.cpp and maglifepceff.cpp.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEPC_H
#define _MAG_MAGLIFEPC_H

#include <stddef.h>

#include "mag/maglife.h"
#include "mag/maglifepcmode.h"

struct BedPad;

/* One record of collectNearItems (EE 0x20 bytes; pickUpNearItem fills
 * m_pos with (0, 0, 0, 1) first). */
struct MagLifePCItemData
{
    float m_pos[4];             // ROR 0x00
    int   m_obj_id;             // ROR 0x10: asserted != 0 for the nearest
    int   m_item_id;            // ROR 0x14: the item (MagLifePCModeItem::live's message argument)
    int   _18;
    short m_actor;              // ROR 0x1c: the object's scene actor (ObjStat::m_actor)
    short _1e;
};
static_assert(sizeof(MagLifePCItemData) == 0x20, "MagLifePCItemData is 0x20 bytes on the EE");
static_assert(offsetof(MagLifePCItemData, m_obj_id) == 0x10, "m_obj_id");
static_assert(offsetof(MagLifePCItemData, m_item_id) == 0x14, "m_item_id");
static_assert(offsetof(MagLifePCItemData, m_actor) == 0x1c, "m_actor");

/* The attack heights (getAttackSeNo's assert names the ends; 4 is the
 * kick / stomp, startAttack's low half). */
enum { ATK_HEIGHT_NORMAL = 0, ATK_HEIGHT_STOMP = 4 };

/* MAX_SUBMODE_STACK (the pushSubMode / popSubMode asserts). */
enum { MAX_SUBMODE_STACK = 4 };

/* layout: host -- polymorphic in the ROM (vptr +0x14); EE offsets in the
 * comments. */
struct MagLifePC : MagLifeControlChara
{
    MagLifePC();                                                         // ROR 0x0011bf0c
    /* EE slot 1 (+0x08): restores _vt$8MagLife0 (linkonce). */
    // ROR 0x00282b34
    ~MagLifePC() override {}

    /* ---- EE virtual slots (called on the known class) ---- */
    /* EE slot 2 (+0x10) */
    void live() override;                                                // ROR 0x0011ade4
    /* EE slot 5 (+0x28): no name (linkonce). */
    // ROR 0x00282bc8
    const char *getName() { return NULL; }
    /* EE slot 10 (+0x50): binds the actor (NULL unbinds), resets the
     * status words and enters state 0. */
    void setActor(DogActor *actor, int no, int mode);                    // ROR 0x0011bd4c
    /* EE slot 12 (+0x60): back to state 0 and the 'wait2' motion of the
     * equipped item's actor (MagScene::resetEntities). */
    void reset4MapJump();                                                // ROR 0x0011be60
    /* EE slot 13 (+0x68): state -1, no mode, full stamina. */
    void termWorld();                                                    // ROR 0x0011c418
    /* EE slot 14 (+0x70): an event message 0..0x18 (MagScene::commandPC).
     * arg is a vec4 for 0x15, an int for 0x13 / 0x17 / 0x18. Returns 0, or
     * 0xfffffe93 without an actor; 0x12 / 0x15 / 0x16 return a value. */
    int  message(int msg, const float *arg);                             // ROR 0x0011b5e0
    /* EE slot 16 (+0x80) */
    void processPad(BedPad *pad);                                        // ROR 0x0011b30c

    /* ---- mag/maglifepc.cpp ---- */
    /* Unreferenced: the state saved by setState back into m_state..m_0c. */
    void restoreState();                                                 // ROR 0x0011ab40
    /* 0..4: m_health_override, m_mode 1 -> 3, 3 -> 4, else 2 when the PC
     * param ratio is at most 0.3, else stamina <= 300. */
    int  calcHealth();                                                   // ROR 0x0011ab88
    /* The stick into m_stick_*: 0 inside the dead zone, 1 walk, 2 run. */
    int  readStick(BedPad *pad);                                         // ROR 0x0011ac24
    /* The stance: m_weapon_type + 1 while 0x800 is held, else 0; -1 in
     * state 10 (keep). */
    int  readStance(BedPad *pad);                                        // ROR 0x0011acf4
    void updatePadStatus(BedPad *pad);                                   // ROR 0x0011ad6c
    /* m_stance != 0 (MagScene::isPCWordDcSet, the Dog* modes). */
    bool isWordDcSet();                                                  // ROR 0x0011af04
    /* m_04 while the state is 0x17, else -1. */
    int  getState17Count();                                              // ROR 0x0011af10
    /* Brown orders (states 15 go / 16 come / 17 stay): flags 1 the voice,
     * 2 MagLifeBwnCtrl. */
    void orderDog(int state, u_int flags);                               // ROR 0x0011af30
    void pickUpNearItem();                                               // ROR 0x0011b07c
    float getStickRotY();                                                // ROR 0x0011b128
    int  canTurnDirect(float from, float to);                            // ROR 0x0011b204
    int  padMode(BedPad *pad);                                           // ROR 0x0011b2c4
    /* MagScene::parseOption's PC options 6..14. */
    void setParam(float theValue, int id);                               // ROR 0x0011b964
    void pushSubMode();                                                  // ROR 0x0011bc64
    void popSubMode();                                                   // ROR 0x0011bcd8
    void drainStamina();                                                 // ROR 0x0011bdc0
    void recoverStamina();                                               // ROR 0x0011be00
    int  isBlendIn();                                                    // ROR 0x0011be30
    /* Enters theMode with arg (force: even when already in it). */
    void changeState(int theMode, int arg, int force);                   // ROR 0x0011c1e8
    /* m_mode_cur = the mode object of m_state; with doInit its init. */
    void selectMode(int doInit);                                         // ROR 0x0011c2ac

    /* linkonce accessors */
    // ROR 0x00282bb8
    int  getStamina() { return m_stamina; }
    // ROR 0x00282bc0
    int  getHealth() { return m_health; }
    // ROR 0x00282bd0
    void setDebugDashOnOff(int on) { m_debug_dash = on; }

    /* ---- mag/maglifepc2.cpp: items, attacks (0x11c464..0x11d720) ---- */
    /* GamMan::getPcParam(0x1b) / (0x1c): calcHealth's ratio; setPcParam1b
     * is unreferenced. */
    static int getPcParam1b();                                           // ROR 0x0011c464
    static void setPcParam1b(int value);                                 // ROR 0x0011c494
    static int getPcParam1c();                                           // ROR 0x0011c4c0
    static int getPcParam0();                                            // ROR 0x0011c4f0
    /* g_gamMan's equipped item (+0x2220c, 0xa00 none) / its actor number
     * (+0x2200c), both read as shorts. */
    static int getEquipItem();                                           // ROR 0x0011c520
    static int getEquipItemChrNo();                                      // ROR 0x0011ce50
    /* Unreferenced: the positions of the achrs (not the PC / dog) whose
     * param 0 is the PC's, max at most; returns the count. */
    int  collectPcParam0AchrPos(float *out, int max);                    // ROR 0x0011c52c
    /* Up to max items in a radius range circle centred range - 5 ahead of
     * the actor and -200..30 of its height, into records of 0x20 bytes;
     * returns the count. */
    int  collectNearItems(float theRange, MagLifePCItemData *theData, int theMax); // ROR 0x0011c64c
    /* Damages oid by the combo step through scnChrRequest(0xb): kind 0 the
     * equipped item's attack type (bare hands: combo 0), else word 7. */
    void hitTarget(u_int oid, int combo, int kind);                      // ROR 0x0011c974
    /* System data field 0x27 of item (2 for no item, <= 0). */
    int  getAttackType(int item);                                        // ROR 0x0011caf8
    /* 0..3: item's weapon class (attack type 5 -> 3, 4 -> 2, 6 -> 1),
     * m_weapon_override first. */
    int  getWeaponType(int item);                                        // ROR 0x0011cb70
    static int countItems();                                             // ROR 0x0011cbd0
    /* GamMan::canAddItem of oid. */
    int  canTakeItem(u_int oid);                                         // ROR 0x0011cbf4
    /* Picks oid up (addItem, the script's pick-up words, releaseChr);
     * -1 when it cannot. */
    int  takeItem(u_int oid);                                            // ROR 0x0011cc14
    /* GamMan::testPcHitAt at pos; the object id into *oid when given. */
    int  testObjHit(const float *pos, u_int *oid);                       // ROR 0x0011ccd4
    /* out = delta less its part towards object oid (level), normalised
     * when shorter than 1. */
    void slideOffObj(float *out, const float *delta, u_int oid);         // ROR 0x0011cd20
    /* oid's 'p_fuckme' node ('Bip01 Spine1' without one) into out; returns
     * its EmyManager distance (flag 0: getEmyTypeDist2), 0 without an actor. */
    float getTargetSpinePos(float *out, u_int oid, int flag);            // ROR 0x0011ce5c
    float getEmyTypeRate(u_int oid);                                     // ROR 0x0011d028
    int  canReload();                                                    // ROR 0x0011d044
    /* GamMan::reloadEquip, the shells effect (0x27, m_shots) and the
     * equipped item's 'reload' motion. */
    void reload();                                                       // ROR 0x0011d06c
    static int getSelItemNum();                                          // ROR 0x0011d15c
    /* One shot: GamMan::decSelItemNum and m_shots + 1. */
    void decSelItemNum();                                                // ROR 0x0011d178
    bool testEmyHittable(u_int oid);                                     // ROR 0x0011d1b0
    /* The hit sound id for theHeight 0..4 and theCombo 0..2 (kind != 0:
     * the hit, else the swing). */
    int  getAttackSeNo(int theHeight, int theCombo, int kind);           // ROR 0x0011d1cc
    /* The attack height (0..3) of enemy oid at XZ distance dist by its
     * type's distance bands; -1 outside them. */
    int  getAttackHeightByDist(float dist, u_int oid);                   // ROR 0x0011d3d0

    /* ---- mag/maglifepc3.cpp: motions ---- */
    /* DogActor::getMotionStep scaled by m_health (1, 1, 1, 0, 0). */
    float getHealthMotionStep();                                         // ROR 0x0011d720
    /* Unimplemented in the ROM: asserts and returns -1. */
    int  changeMotion(float speed, int theNo, u_int flags, int theNo2, int arg); // ROR 0x0011d790
    /* DogActor::changeMotion to theMotion (blended with theMotion2 when
     * set): in m_mode 2 the "jnfc_" variant of the name first, else by
     * suffix. < 0 when the motion is missing. */
    int  changeMotion(float speed, const char *theMotion, u_int flags,
                      const char *theMotion2, int arg);                  // ROR 0x0011d7c0
    /* Unreferenced: file theIndex of the pack theFileName into bank theLevel. */
    int  loadMotion(int theLevel, const char *theFileName, int theIndex); // ROR 0x0011d990
    /* MOTIONPK.RPK file theIndex (< 0: by m_stance; stance 0 loads nothing)
     * into bank theLevel (< 0: 3). */
    void loadWeaponMotion(int theIndex, int theLevel);                   // ROR 0x0011dc64
    /* The weapon type's motions (loaded when its "_in" is missing). */
    void initMotion();                                                   // ROR 0x0011dd40
    /* Unreferenced: a one-shot motion unless one plays (m_action_motion);
     * isActionMotionEnd ends it with the actor's mode. */
    int  playActionMotion(float speed, int theNo);                       // ROR 0x0011ddf4
    int  playActionMotion(float speed, const char *theMotion);           // ROR 0x0011de78
    int  isActionMotionEnd();                                            // ROR 0x0011df14
    /* Sub motion 0 by m_stance over the upper body (both arms for 4). */
    void setStanceSubMotion();                                           // ROR 0x0011df7c
    void clearSubMotion();                                               // ROR 0x0011dfd0
    /* Sub motion 0 for the wait mode by m_weapon_type; noGrip: the right
     * fingers without the grip motion. */
    void setWaitSubMotion(int noGrip);                                   // ROR 0x0011dff8
    /* 6 / 7 / 5 by m_move_level_prev 1 / 2 / other. */
    int  getSubStateClass();                                             // ROR 0x0011e080
    /* States 1..10 and 14. */
    int  isMoveState();                                                  // ROR 0x0011e0ac
    /* ---- mag/maglifepcmove.cpp ---- */
    /* The collision sweep of a move from by delta (range / radius are
     * m_sweep_range / m_sweep_radius): an object at from + delta (out: the
     * delta slid off it) gives 1, else a cylinder of radius range - radius
     * pushed radius ahead, against the scene; < 0 = free, else out (when
     * given) is the move's length along the wall it hit. */
    float sweepMove(float range, float radius, const float *from, const float *delta,
                    float *out);                                         // ROR 0x0011ea04
    /* Two rays along delta from both sides of from (range - radius out):
     * 0 when one hits and out is the move's length towards the other side,
     * -1 when neither hits, 1 on an object at from + delta. */
    float sweepSides(float range, float radius, const float *from, const float *delta,
                     float *out);                                        // ROR 0x0011e4c4
    /* The PC moved by a root-motion step: no move from health 3 up, no
     * collision while the scene's m_22b8 is set, else the sweeps (sliding
     * along walls), the floor and the secta cells. */
    void moveBy(const float *move, float trans_ofs_y, float rot);        // ROR 0x0011ee00
    /* Unreferenced: tank control (stick y walks, x turns or turns in place). */
    int  moveTank(BedPad *pad);                                          // ROR 0x0011f0b4
    /* Always 0: the current motion is restarted by moveToward. */
    int  isKeepMotion(int no);                                           // ROR 0x0011f4a0
    /* Turns towards theRotY (at most 0.2 a frame; near 180 degrees the turn
     * motion theTurn) from thePlyY, plays theMotion (theIn first when set)
     * and moves by its root motion. */
    void moveToward(float theRotY, float thePlyY, float speed, const char *theIn,
                    const char *theMotion, const char *theTurn);         // ROR 0x0011f4a8
    /* A stance step motion and its root motion turned by theRotY. */
    void moveStanceStep(float theRotY, float speed, const char *theMotion); // ROR 0x0011f884
    /* The walk / run by the stick relative to the camera. */
    int  moveFree(BedPad *pad);                                          // ROR 0x0011f9f8
    /* The stance walk by the stick (8 directions against the heading). */
    int  moveAttackStance(BedPad *pad);                                  // ROR 0x0011ff10
    /* Health 3 / 4: the ev201 crawl motion of the stick's direction. */
    int  moveDowned(BedPad *pad);                                        // ROR 0x001203a0
    /* Unreferenced: moveDowned, moveAttackStance or moveFree by the status. */
    int  movePad(BedPad *pad);                                           // ROR 0x001205dc
    /* The current motion's root step (frames doubled) turned by the base
     * heading, unless the actor's mode has an argument. */
    void applyMotionMove();                                              // ROR 0x00120668
    /* ---- mag/maglifepcbtl.cpp ---- */
    /* out = the actor's m_trans 120 lower (the attack's base); returns its
     * heading. */
    float getAttackBasePos(float *out);                                  // ROR 0x0012bc20
    /* The weapon's hit node hitNo for theHeight into out; returns its
     * radius, 0 without one. */
    float getWeaponHitPos(float *out, int theHeight, int hitNo);         // ROR 0x0012bc98
    /* The yaw / pitch from the actor to pos; returns the yaw less the
     * actor's heading in [-pi, pi]. */
    float calcTargetAngle(float *roty, float *rotx, const float *pos);   // ROR 0x0012bedc
    /* The current attack's hit window in motion frames (default 0..1000). */
    void getHitFrameRange(float *from, float *to);                       // ROR 0x0012c014
    /* The motion of theWeapon (0 bare hands, 1 gun, 2 knife, 3 axe),
     * theHeight (0 mid, 1 high, 2 low, 3 mid-low, 4 stomp) and theCombo
     * 0..2; "gun_empty" for the gun without ammunition. */
    const char *getAttackMotionName(int theWeapon, int theHeight, int theCombo); // ROR 0x0012c104
    /* Remembers the attack (m_atk_*), plays its motion and swing sound. */
    void playAttackMotion(int theWeapon, int theHeight, int theCombo);   // ROR 0x0012c1fc
    /* Starts the attack motion for combo step combo; returns the height
     * (0..4, 4 the kick) in the low half and 1 in the high half when the
     * attack has no hit test. */
    u_int startAttack(int combo);                                        // ROR 0x0012c31c
    /* 0..4: the target's height class (m_target_no), else message 0x17's
     * low half (>= 0). */
    int  getAttackHeight();                                              // ROR 0x0012ca28
    /* The height to attack enemy oid at pos with: 4 (stomp) when it is
     * not hittable standing and within 80, else getAttackHeightByDist. */
    int  calcAttackHeight(const float *pos, u_int oid);                  // ROR 0x0012caa4
    /* State 11: picks the attack target, then state 5. */
    void beginAttack();                                                  // ROR 0x0012cb50
    /* The equipped gun's muzzle flash (EffMan kind 0x26 at its gun_mazzle
     * node; mag/maglifepceff.cpp). */
    void createMuzzleEff();                                              // ROR 0x0012ccac
    /* State 0xe with arg (mag/maglifepcbtl.cpp), word +0x0c = word. */
    void setState14(int arg, int word);                                  // ROR 0x0012c9c0
    /* message 0xe..0x11 (mag/maglifepceff.cpp): the first three are
     * assert(0) -- retired messages -- and 0x11 does nothing. */
    void message14();                                                    // ROR 0x0012cdb0
    void message15();                                                    // ROR 0x0012cddc
    void message16();                                                    // ROR 0x0012ce08
    void message17();                                                    // ROR 0x0012ce34

    int    m_7c;                // ROR 0x7c
    int    m_80;                // ROR 0x80
    int    m_84;                // ROR 0x84: the constructor and readStick clear it
    int    m_88;                // ROR 0x88
    int    m_8c;                // ROR 0x8c
    int    m_submode_stack[MAX_SUBMODE_STACK][4]; // ROR 0x90: pushSubMode's state quads
    int    m_stamina;           // ROR 0xd0: 0..1000 (getStamina)
    int    m_stamina_tick;      // ROR 0xd4: drainStamina's counter
    int    m_submode_level;     // ROR 0xd8
    int    m_stance;            // ROR 0xdc: readStance (0 = none, 1.. weapon type + 1); the asserts' m_attack_stat (ATK_STAT_NONE 0)
    int    m_move_level;        // ROR 0xe0: readStick (0 idle, 1 walk, 2 run); the asserts' m_move_stat (MOV_STAT_WAIT / _RUN)
    int    m_health;            // ROR 0xe4: calcHealth (getHealth; MagSystem::getPCWordE4)
    int    m_stance_prev;       // ROR 0xe8
    int    m_move_level_prev;   // ROR 0xec: getSubStateClass maps 1 / 2 / other
    int    m_health_prev;       // ROR 0xf0
    int    m_action_motion;     // ROR 0xf4: playActionMotion's motion while it plays, -1 none (constructor, a stance change)
    int    m_pad_seen;          // ROR 0xf8: processPad sets 1, live clears it
    int    m_fc;                // ROR 0xfc: the constructor sets -1
    int    m_100;               // ROR 0x100: the constructor sets -1
    float  m_stick_mag;         // ROR 0x104: readStick, 0 inside the dead zone
    float  m_stick_x;           // ROR 0x108: BedPad::getDir
    float  m_stick_y;           // ROR 0x10c
    float  m_stick_roty;        // ROR 0x110: getStickRotY
    float  m_stick_change;      // ROR 0x114: BedPad::getDirChange; selectMode sets 9999
    int    m_shots;             // ROR 0x118: shots since the last reload (decSelItemNum; reload's effect, then 0); the constructor clears it
    u_char _11c[0x124 - 0x11c];
    int    m_atk_weapon;        // ROR 0x124: playAttackMotion's weapon (getHitFrameRange)
    int    m_atk_height;        // ROR 0x128: its height
    int    m_atk_combo;         // ROR 0x12c: its combo step
    int    m_mode_frame;        // ROR 0x130: frames in the mode (selectMode 0, live +1)
    MagLifePCMode *m_mode_cur;  // ROR 0x134: selectMode's pick, NULL in state 11
    MagLifePCModeNull       m_mode_null;        // ROR 0x138
    MagLifePCModeWait       m_mode_wait;        // ROR 0x13c
    MagLifePCModeWalk2D     m_mode_walk;        // ROR 0x140
    MagLifePCModeRun2D      m_mode_run;         // ROR 0x144
    MagLifePCModeTurn       m_mode_turn;        // ROR 0x148
    MagLifePCModeAtkWait2D  m_mode_atk_wait;    // ROR 0x150
    MagLifePCModeAtkMove2D  m_mode_atk_move;    // ROR 0x154
    MagLifePCModeAttacking  m_mode_attacking;   // ROR 0x158
    MagLifePCModeDamage     m_mode_damage;      // ROR 0x168
    MagLifePCModeItem       m_mode_item;        // ROR 0x174
    MagLifePCModeDogGo      m_mode_dog_go;      // ROR 0x178
    MagLifePCModeDogCome    m_mode_dog_come;    // ROR 0x17c
    MagLifePCModeDogStay    m_mode_dog_stay;    // ROR 0x180
    MagLifePCModeHold       m_mode_hold;        // ROR 0x184
    MagLifePCModeHoldAttack m_mode_hold_attack; // ROR 0x18c
    MagLifePCModeUnhold     m_mode_unhold;      // ROR 0x190
    MagLifePCModeRide       m_mode_ride;        // ROR 0x194
    MagLifePCModeRoped      m_mode_roped;       // ROR 0x1a0
    MagLifePCModeReload     m_mode_reload;      // ROR 0x1a4
    MagLifePCModeHug        m_mode_hug;         // ROR 0x1a8
    int    m_weapon_override;   // ROR 0x1ac: getWeaponType uses it when >= 0 (message 0x17's high half)
    int    m_1b0;               // ROR 0x1b0: message 0x17's low half
    int    m_health_override;   // ROR 0x1b4: calcHealth uses it when >= 0 (message 0x18)
    int    m_debug_dash;        // ROR 0x1b8: setDebugDashOnOff; the constructor sets 1
    u_char _1bc[0x1c0 - 0x1bc];
    float  m_target_pos[4];     // ROR 0x1c0: beginAttack's target
    float  m_1d0[4];            // ROR 0x1d0: the constructor sets (0, 0, 0, 1)
    float  m_target_dist;       // ROR 0x1e0: startAttack: getTargetSpinePos's type distance
    u_char _1e4[0x1f0 - 0x1e4];
    int    m_target_no;         // ROR 0x1f0: beginAttack
    int    m_target_node;       // ROR 0x1f4: beginAttack: the target's UtlEnemyInfo::m_24 (its uid), -1 none
    float  m_dead_zone;         // ROR 0x1f8: option 6 (stick magnitude), 0.1
    float  m_cos_1fc;           // ROR 0x1fc: option 7, cos(5 degrees)
    float  m_cos_200;           // ROR 0x200: option 8, cos(89 degrees)
    float  m_204;               // ROR 0x204: option 9, 0.07
    float  m_208;               // ROR 0x208: option 10 (degrees / 180), 0.9
    float  m_stick_threshold;   // ROR 0x20c: option 11, getStickRotY's, 0.001
    float  m_sweep_range;       // ROR 0x210: option 12, 45
    float  m_sweep_radius;      // ROR 0x214: option 13, 20
    float  m_218;               // ROR 0x218: option 14, 0
    int    m_weapon_type;       // ROR 0x21c: getWeaponType (processPad, initMotion)
};
static_assert(offsetof(MagLifePC, m_submode_stack) >= 0x90, "m_submode_stack");
static_assert(offsetof(MagLifePC, m_stamina) >= 0xd0, "m_stamina");
static_assert(offsetof(MagLifePC, m_health) >= 0xe4, "m_health");
static_assert(offsetof(MagLifePC, m_move_level_prev) >= 0xec, "m_move_level_prev");
static_assert(offsetof(MagLifePC, m_stick_change) >= 0x114, "m_stick_change");
static_assert(offsetof(MagLifePC, m_mode_cur) >= 0x134, "m_mode_cur");
static_assert(offsetof(MagLifePC, m_mode_hug) >= 0x1a8, "m_mode_hug");
static_assert(offsetof(MagLifePC, m_debug_dash) >= 0x1b8, "m_debug_dash");
static_assert(offsetof(MagLifePC, m_target_no) >= 0x1f0, "m_target_no");
static_assert(offsetof(MagLifePC, m_atk_combo) >= 0x12c, "m_atk_combo");
static_assert(offsetof(MagLifePC, m_target_dist) >= 0x1e0, "m_target_dist");
static_assert(offsetof(MagLifePC, m_weapon_type) >= 0x21c, "m_weapon_type");

#endif /* _MAG_MAGLIFEPC_H */
