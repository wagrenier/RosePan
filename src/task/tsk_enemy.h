/* ==========================================================================
 *  task/tsk_enemy.h
 *
 *  task/tsk_enemy.cpp (pinned 0x20067c..0x203d94, between the TskAttack /
 *  TskDamage steps and system/system.cpp; RoR has no source string for
 *  it, the name is the port's): the enemy behaviour tasks EmyMain owns.
 *  - TskEnemyMove (vtable 0x2e5040, EE 0xad0): walks the enemy's actor to
 *    a goal or after a target oid through the secta cells (emyStartMove /
 *    emyStartChase), with idle / look-around breaks and a voice now and
 *    then;
 *  - TskEnemyTurn (0x2e5080, EE 0x60): turns the actor toward the PC or
 *    the dog with the turn motions 8 / 9 (emyStartTurn).
 *  Both are kind 0x13 (CmnTaskMan's pause list) and manager-owned
 *  (EmyMain::loadParam adds them once; start / stop switch them on and
 *  off). The task never ends itself: exec returns 0, and the result goes
 *  to the starter's word (EmyOut::m_move_done / m_turn_done).
 * ======================================================================== */
#ifndef _TASK_TSK_ENEMY_H
#define _TASK_TSK_ENEMY_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "task/tsk_man.h"

struct DogActor;

/* TskEnemyMove::start's record (EE 0x60 on emyStartMove's stack; the word
 * index the task reads in the comments). layout: host -- the pointer. */
struct EmyMoveParam
{
    u_int  m_oid;               // [0]: the enemy
    int   *m_done;              // [1]: cleared, then the result (EmyOut::m_move_done)
    int    m_mode;              // [2]: the type's (0x2f1d50: 1 for types 6..10)
    float  m_agl;               // [3]: EmyMain::m_agl
    float  m_walk;              // [4]: the type's speed (0x2f1c90), times m_agl
    float  m_run;               // [5]: the chase's second speed (0x2f1c94; 0 for a move)
    float  m_dist;              // [6]: the type's pair (0x2f1d80: 45 / 60)
    float  m_angle;             // [7]: ... and 0.3 pi
    int    m_parsonality;       // [8]: EmyMain::m_parsonality (the task keeps it + 3)
    int    m_carry;             // [9]: a Got (type 2) carrying kind 0xa2e (chase only)
    u_int  m_target;            // [10]: the oid followed (chase), 0
    float  m_pos[3];            // [11]: the goal (move)
    int    _0e;                 // [14]: not set
    float  m_turn;              // [15]: the type's turn rate (0x2f1c98)
    int    _10;                 // [16]: not set
    u_int  m_kind[4];           // [17]: EmyMain::getMoveParam's words 0..3
    int    m_type;              // [21]: EmyMain::GetType
    u_int  m_kind4;             // [22]: ... and word 4
    float  m_range;             // [23]: added to the two radii (the stop distance)
};

/* TskEnemyTurn::start's record (emyStartTurn). layout: host -- the pointer. */
struct EmyTurnParam
{
    u_int  m_oid;               // [0]: the enemy
    u_int  m_target;            // [1]: the PC's or the dog's oid, 0 none
    int   *m_done;              // [2]: cleared, then the result (EmyOut::m_turn_done)
    int    m_type;              // [3]: EmyMain::GetType
    float  m_agl;               // [4]: EmyMain::m_agl
    float  m_arg;               // [5]: emyStartTurn's float (task +0x28)
};

/* TskEnemyMove : CmnITask (vtable 0x2e5040, EE 0xad0; kind 0x13). Active
 * between start and stop; exec plans a route to m_goal (the target's
 * position, led by the PC's speed when it is the PC) through the secta
 * cells, walks it with the walk / turn motions, and breaks off now and
 * then (pickEvent: an idle motion, a look around). The result goes to
 * *m_done: 0 under way, 1 arrived (within m_stopRange), 2 the target is
 * gone. layout: host -- polymorphic; EE offsets in the comments.
 * Written: the lifecycle, exec's frame, pickEvent, tickVoice and
 * getTargetPos. Not yet: plan 0x200988 and step 0x200d18 with their
 * route helpers 0x201164..0x202e20 (the route data +0x88..+0xa80). */
struct TskEnemyMove : CmnITask
{
    /* m_active, m_dog and m_pause cleared (EmyMain::loadParam news it).
     * PORT: also m_se (-1) and the three event countdowns, which the ROM
     * never sets before their first use (heap garbage there). */
    TskEnemyMove();                                                      // ROR 0x002006b0
    ~TskEnemyMove() override {}                                          // ROR 0x002006cc

    int  get_task_type() override { return 0x13; }                       // ROR 0x00290ed8
    /* CmnTaskMan's pause: the actor's motion speed tiny while paused. */
    void pause(int on) override;                                         // ROR 0x00202ec8
    void init() override {}                                              // ROR 0x0020067c
    /* While active and not suspended: one frame of the walk; returns 0. */
    int  exec(int id, int time, int arg) override;                       // ROR 0x002008f0
    /* Stops the voice still playing. */
    void term() override;                                                // ROR 0x00200684

    /* Active (+4 = 1) toward the record's goal or target. */
    void start(const EmyMoveParam *prm);                                 // ROR 0x00200700
    /* Stops it: +4 = 0, its SE stopped, EmyManager's work word of its
     * oid cleared (EmyMain::InactiveMove). */
    void stop();                                                         // ROR 0x00202f10
    /* Paused (+0xac4 = 1): its SE stopped, the achr's motion speed made
     * tiny while active; resume undoes it (EmyMain::pauseMove). */
    void suspend();                                                      // ROR 0x00202f60
    void resume();                                                       // ROR 0x00202fbc

    /* The step's event this frame (only in step 0 without a restart):
     * 1 m_restart was set (cleared), else the last of the three
     * countdowns that ran out this frame -- 4 m_kind[0] (only while
     * m_74 is 0), 3 m_kind[3] (reloaded from m_kind[2]), 2 m_kind[1];
     * each reloads with utlRandRange(k * 80, k * 120) / 100; 0 none. */
    int  pickEvent();                                                    // ROR 0x00200bc4
    /* Events 1 / 2 re-plan the route to the goal (the target's position
     * now), 3 / 4 the stand / idle motion; 0 when the target is gone
     * (*m_done = 2), else 1: step runs. */
    int  plan(const float *pos);                                         // ROR 0x00200988
    /* One frame of m_step (0 walk the route, 1 turn on the spot, 2 turn
     * to the next point, 3 wait for the blend, 4 the idle motion). */
    void step(const float *pos);                                         // ROR 0x00200d18
    /* A voice every 30..80 frames (first 15..30) while farther than 60
     * from the goal and fewer than four enemies talk: table 0x2ff5c0 by
     * type 6 (one of 16) / 10 (one of 4), else voice 12 for types 0..5;
     * EmyManager's work word marks the speaker until the SE ends. */
    void tickVoice(const float *pos);                                    // ROR 0x00202fe4
    /* target's position into pos: emyGetSEPos, or for the PC its spine
     * node led along the actor's facing by the PC's speed (by its state
     * class) x the frames to get there at m_walk (at most 250), when the
     * led point is on a cell of m_layer. */
    void getTargetPos(u_int target, float *pos);                         // ROR 0x0020313c

    int       m_active;         // ROR 0x04: start / EmyMain::ActiveMove set 1, stop 0
    u_int     m_oid;            // ROR 0x08
    int       m_actor;          // ROR 0x0c: the achr's scene actor (ACHRstat m_actor)
    int       m_mode;           // ROR 0x10: prm m_mode
    int       m_layer;          // ROR 0x14: EmyManager::getEmyTypeFlag: the secta list walked
    /* ROR 0x18..0x27: not read yet (0x18 is the base the route points
     * are addressed from: +0x288 = +0x18 + 0x270). */
    DogActor *m_dog;            // ROR 0x28: scene actor m_actor
    int       m_step;           // ROR 0x2c: step's state, 0 at start
    int      *m_done;           // ROR 0x30: prm m_done
    int       m_restart;        // ROR 0x34: 1 at start: pickEvent's event 1
    u_int     m_target;         // ROR 0x38: followed oid, 0 to walk to m_pos
    float     m_pos[3];         // ROR 0x3c: prm m_pos
    int       m_48;             // ROR 0x48: prm word 14 (not set by the starters)
    float     m_speed;          // ROR 0x4c: m_agl / 60, the motion speed
    float     m_walk;           // ROR 0x50: prm m_walk (getTargetPos's speed)
    float     m_walkSpeed;      // ROR 0x54: m_walk x m_agl
    float     m_runSpeed;       // ROR 0x58: m_run x m_agl
    int       m_motion;         // ROR 0x5c: the motion changeMotion was last given, 0
    int       m_motionFlags;    // ROR 0x60: 0x40 at start, 0x41 for the idle motion
    int       m_idleMotion;     // ROR 0x64: m_parsonality + 3
    int       m_68;             // ROR 0x68: step 3 -> 4 when set
    int       m_6c;             // ROR 0x6c
    int       m_carry;          // ROR 0x70: walk motion 10 instead of 2
    int       m_74;             // ROR 0x74: blocks event 4; idle motion 7 when set
    float     m_goal[4];        // ROR 0x78: where the route ends (m_pos or getTargetPos); w unused
    /* ROR 0x88..0xa77: SctLayer::searchPath's route (out at +0x88, its
     * point count +0x284, points of 0x10 from +0x288). Not placed yet. */
    int       m_nodes;          // ROR 0xa78: route points
    int       m_node;           // ROR 0xa7c: the point walked to
    int       m_turnLeft;       // ROR 0xa80
    float     m_turn;           // ROR 0xa84: prm m_turn (the angle step 2 walks within)
    float     m_keepRadius;     // ROR 0xa88: GamMan::getObjKeepRadius(m_oid)
    float     m_stopRange;      // ROR 0xa8c: keep radius + target's radius + prm m_range
    float     m_dist;           // ROR 0xa90: prm m_dist
    float     m_angle;          // ROR 0xa94: prm m_angle
    int       m_kind0;          // ROR 0xa98: prm m_kind[0]: event 4's period
    int       m_wait0;          // ROR 0xa9c
    int       m_kind1;          // ROR 0xaa0: prm m_kind[1]: event 2's period
    int       m_wait1;          // ROR 0xaa4
    int       m_kind2;          // ROR 0xaa8: prm m_kind[2]: event 3's period
    int       m_wait3;          // ROR 0xaac: event 3's countdown
    int       m_kind3;          // ROR 0xab0: prm m_kind[3]: event 3 on / off
    u_int     m_kind4;          // ROR 0xab4: prm m_kind4
    int       m_se;             // ROR 0xab8: the voice's SndMan id, -1 none
    int       m_voiceWait;      // ROR 0xabc: frames to the next voice
    int       m_type;           // ROR 0xac0: prm m_type (EmyMain::GetType)
    int       m_pause;          // ROR 0xac4: suspend / resume
};

/* TskEnemyTurn : CmnITask (vtable 0x2e5080, EE 0x60; kind 0x13). Active
 * between start and EmyOut::termCommon; exec turns the actor toward the
 * target's position with motion 9 (left of it) / 8, and sets *m_done to 1
 * once within m_range of facing it, 2 when the target is gone.
 * layout: host -- polymorphic; EE offsets in the comments. */
struct TskEnemyTurn : CmnITask
{
    /* m_active and m_dog cleared (EmyMain::loadParam news it). PORT:
     * also m_se (-1) and m_pause, which the ROM leaves unset. */
    TskEnemyTurn();                                                      // ROR 0x002034c0
    ~TskEnemyTurn() override {}                                          // ROR 0x002034d8

    int  get_task_type() override { return 0x13; }                       // ROR 0x00291080
    void pause(int on) override;                                         // ROR 0x0020395c
    /* Both stop the voice still playing. */
    void init() override;                                                // ROR 0x00203494
    int  exec(int id, int time, int arg) override;                       // ROR 0x002035d8
    void term() override;                                                // ROR 0x00203468

    /* Active (+4 = 1) toward the record's target. */
    void start(const EmyTurnParam *prm);                                 // ROR 0x0020350c
    /* TskEnemyMove::suspend / resume's twins (+0x48). */
    void suspend();                                                      // ROR 0x002039a4
    void resume();                                                       // ROR 0x00203a00

    /* Keeps the turn motion on the side the target is (and restarts it
     * once it ends); *m_done = 1 within m_range. */
    void turn(const float *pos);                                         // ROR 0x00203684
    /* Motion 9 when the target is left of the facing (m_right 0), else 8
     * (m_right 1); changed only when it differs. */
    void setTurnMotion(const float *pos);                                // ROR 0x00203828
    /* TskEnemyMove::tickVoice without the speaker count: within 180 of
     * the target it stays quiet; table 0x2ff6a8. */
    void tickVoice(const float *pos);                                    // ROR 0x00203a28
    /* TskEnemyMove::getTargetPos with the PC led by its speed x 3 and no
     * cell test. */
    void getTargetPos(u_int target, float *pos);                         // ROR 0x00203b44

    int       m_active;         // ROR 0x04: start sets 1, EmyOut::termCommon 0
    u_int     m_oid;            // ROR 0x08
    int       m_actor;          // ROR 0x0c: the achr's scene actor
    DogActor *m_dog;            // ROR 0x10: scene actor m_actor
    int      *m_done;           // ROR 0x14: prm m_done
    u_int     m_target;         // ROR 0x18: prm m_target
    float     m_speed;          // ROR 0x1c: m_agl / 60
    int       m_motion;         // ROR 0x20: 8 / 9, 0 at start
    int       m_right;          // ROR 0x24: 1 turning with motion 8, 1 at start
    float     m_range;          // ROR 0x28: prm m_arg: the angle that counts as facing
    float     m_targetPos[4];   // ROR 0x2c: getTargetPos each frame; w (+0x38) unused
    int       m_se;             // ROR 0x3c: the voice's SndMan id, -1 none
    int       m_voiceWait;      // ROR 0x40: frames to the next voice
    int       m_type;           // ROR 0x44: prm m_type
    int       m_pause;          // ROR 0x48: suspend / resume
};

#endif /* _TASK_TSK_ENEMY_H */
