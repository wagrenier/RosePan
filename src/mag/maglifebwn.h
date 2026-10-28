/* ==========================================================================
 *  mag/maglifebwn.h
 *
 *  MagLifeBwn (mag/maglifebwn.cpp; vtable _vt$10MagLifeBwn 0x2e1140, base
 *  MagLifeControlChara, vptr at +0x14, constructor 0x125aa4): the
 *  controller of Brown, the dog, EE 0xd40 bytes. The scene holds one at
 *  +0x2afe0 (MagLifeBwnCtrl follows at +0x2bd20, the orders' side) and
 *  binds Brown's actor to it (MagScene::setBwn).
 *
 *  m_state is Brown's state; each has a mode object embedded here
 *  (mag/maglifebwnmode.h lists them). changeMode requests a state (vtable
 *  slot 16 writes it to m_next_state) and applyState enters it once the
 *  current mode's term agrees: the old state quad is saved as setState
 *  does, the new mode's init runs. live runs the mode each frame, with the
 *  neck (look / smell) sub motion, the limp, the footsteps and the escape
 *  from floor without secta cells around it.
 *
 *  Moving: stepMotion moves by the motion's root step after the body and
 *  object hit tests; searchPath finds a way through the secta cells to a
 *  point and lays it out as the search line (points + segment lengths,
 *  widths), which followPath walks by a 16.16 index (advancePathIndex).
 *
 *  mag/maglifebwn.cpp is 0x1233d8..0x125bd4 (pinned 2026-10-27; the
 *  asserts name the file) plus the linkonce accessors below.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEBWN_H
#define _MAG_MAGLIFEBWN_H

#include <stddef.h>

#include "common/ror_assert.h"
#include "game/gamstat.h"
#include "mag/maglife.h"
#include "mag/maglifebwnmode.h"

/* The search line's capacity (the assert's (127)). */
enum { MAX_SEARCH_LINE = 127 };

/* layout: host -- polymorphic in the ROM (vptr +0x14); EE offsets in the
 * comments. */
struct MagLifeBwn : MagLifeControlChara
{
    MagLifeBwn();                                                        // ROR 0x00125aa4
    /* EE slot 1 (+0x08) (linkonce). */
    // ROR 0x00283344
    ~MagLifeBwn() override {}

    /* ---- EE virtual slots (called on the known class) ---- */
    /* EE slot 2 (+0x10) */
    void live() override;                                                // ROR 0x00124e40
    /* EE slot 5 (+0x28) (linkonce). */
    // ROR 0x002833dc
    const char *getName() { return NULL; }
    /* EE slot 10 (+0x50): binds the actor; NULL unbinds (the neck sub
     * motion cleared first). */
    void setActor(DogActor *actor, int no, int mode);                    // ROR 0x001256d4
    /* EE slot 12 (+0x60): no state, the neck motion off, the health
     * topped up by a quarter of a fifth of the maximum while below that
     * fifth (MagScene::resetEntities). */
    void reset();                                                        // ROR 0x00124f38
    /* EE slot 14 (+0x70): the slot of MagLifePC::message. mode 0..9
     * through changeMode: 0, -2 when it is the one pending already, -1
     * when refused; 0 without an actor or for another mode. */
    int  message(u_int mode, int a, int b);                              // ROR 0x001247c8
    /* EE slot 15 (+0x78): updateMotionSeq unless suspended. */
    void renderPostProcess();                                            // ROR 0x001259e8
    /* EE slot 16 (+0x80): m_next_state = (mode, -1, a, b). */
    void requestState(int mode, int a, int b);                           // ROR 0x001233d8

    /* ---- mag/maglifebwn.cpp ---- */
    /* Requests theMode (not when already in it, unless 0) and applies it:
     * 1, else 0. */
    int  changeMode(int theMode, int a, int b);                          // ROR 0x001233f4
    /* The way to target through the secta cells into the search line:
     * 0 reaching its cell, 1 ending short of it, -1 no way, -2 / -3 no
     * cell under Brown / the target, -100 no straight line from a single
     * cell. */
    int  searchPath(const float *target);                                // ROR 0x0012349c
    /* A sphere of theSize at the b_ch1 node (+ ofs) against the scene: 0
     * on a hit (out = a step back along the heading, 0 against floor-like
     * normals), else 1. */
    int  checkBodyHit(float theSize, float *out, const float *ofs);      // ROR 0x0012391c
    /* One frame of root motion (when the body is clear and a cell stays
     * under it); 1 moved, -1 pushed back off a wall, -2 an object, -3 no
     * cell (m_move_result too). */
    int  stepMotion();                                                   // ROR 0x00123bb8
    /* The object hit at the actor + ofs: its hit word when the kind
     * (bits 23..27) is 2, else 0. */
    u_int testObjAt(const float *ofs);                                   // ROR 0x00123f78
    /* thePosIndex (16.16 along the search line) moved theLength on. */
    int  advancePathIndex(float theLength, int thePosIndex);             // ROR 0x00124008
    /* One frame along the search line (sets m_path_end at its end), then
     * a turn motion at a bend. */
    void followPath();                                                   // ROR 0x00124150
    /* The neck sub motion theMotion (neckblend3..2, wait) at theRate;
     * theMotion < 0 clears it. */
    void setNeckMotion(float theRate, int theMotion);                    // ROR 0x0012481c
    void clearNeckMotion();                                              // ROR 0x0012498c
    /* The neck control: 1 smell, 2 the neckblend towards m_neck_pos. */
    void updateLook();                                                   // ROR 0x001249b4
    /* Health 3: the w_blend_wait sub motion over the left foreleg. */
    void updateLimp();                                                   // ROR 0x00124d7c
    void clearLegMotion();                                               // ROR 0x00124e18
    /* m_turn_rot_y = towards the search line at m_pos_index. */
    void aimAtPath();                                                    // ROR 0x00125014
    /* stepMotion and a turn motion towards m_turn_rot_y: 1 facing it,
     * 0 turning, -1 blocked. */
    int  turnToPath();                                                   // ROR 0x00125104
    /* 0..4 by the dog's param 0x1b / 0x1c ratio (4 at 0). */
    int  calcHealth();                                                   // ROR 0x001252c4
    /* No cell under Brown: probes around in growing steps and walks out. */
    void escapeNoFloor();                                                // ROR 0x00125368
    /* m_next_state becomes the state when the mode's term agrees (force
     * from live); 1 when it changed or was refused for a non-zero state. */
    int  applyState(int force);                                          // ROR 0x00125704
    /* Stops Brown for a scene: live and renderPostProcess skip, the motion
     * frozen or wait; resume restarts it (or clears the mode when the
     * motion changed meanwhile). */
    void suspend();                                                      // ROR 0x001258a4
    void resume();                                                       // ROR 0x00125980
    void setNeckControl(int mode);                                       // ROR 0x00125a20
    void setNeckPos(const float *pos);                                   // ROR 0x00125a3c
    /* No mode, state -1. */
    void clearMode();                                                    // ROR 0x00125a90

    /* ---- linkonce accessors (the ELF's names) ---- */
    // ROR 0x002833c8
    int  getNextMode() { return m_next_state[0]; }
    // ROR 0x002833d0
    void setNeckMotionTarget(int target, float rate)
    {
        m_neck_rate = rate;
        m_neck_target = target;
    }
    // ROR 0x002833e4 (mag/maglifebwn.h line 213 in the ROM)
    const float *getLastSearchPos() const
    {
        ROR_ASSERT(0<m_num_search_line && m_num_search_line<(127)); /* 213 */
        return m_search_line[m_num_search_line - 1];
    }
    // ROR 0x00283440
    int  isActionEnd() const { return m_path_end; }
    // ROR 0x00283448
    int  isPathMoveEnd() const { return m_path_end; }
    // ROR 0x00283450
    int  getMoveResult() const { return m_move_result != 0; }
    // ROR 0x0028345c
    void resetMove() { m_move_result = 0; }
    // ROR 0x00283464
    int  getNeckControl() const { return m_neck_control; }
    // ROR 0x0028346c
    int  getUnmovableCount() const { return m_unmovable_count; }
    // ROR 0x00283474
    int  getHealthStatus() const { return m_health_status; }

    int       m_7c;                 // ROR 0x7c: role unknown
    int       m_next_state[4];      // ROR 0x80: requestState's quad (state, -1, a, b); [0] / [1] -1 once applied
    float     m_neck_pos[4];        // ROR 0x90: setNeckPos, what the neck looks at
    float     m_search_line[MAX_SEARCH_LINE][4]; // ROR 0xa0: the points; w the length to the next
    float     m_search_width[MAX_SEARCH_LINE];   // ROR 0x890: the cell's narrower side per segment
    GamSecta *m_goal_secta[4];      // ROR 0xa8c: the cells at searchPath's target
    GamSectaPath m_search_path;     // ROR 0xa9c: the cells of the way (m_num at 0xc98)
    float     m_goal_dist;          // ROR 0xc9c: searchPath: from Brown to the target
    int       m_num_search_line;    // ROR 0xca0
    int       m_reach_goal;         // ROR 0xca4: the way ends in the target's cell
    int       m_pos_index;          // ROR 0xca8: 16.16 along the search line
    int       m_unmovable_count;    // ROR 0xcac: frames live found no move
    int       m_turning;            // ROR 0xcb0: turnToPath's motion running
    float     m_turn_rot_y;         // ROR 0xcb4: aimAtPath's heading
    int       m_health_status;      // ROR 0xcb8: calcHealth, 0..4
    int       m_health_status_prev; // ROR 0xcbc
    int       m_path_end;           // ROR 0xcc0: followPath reached the line's end
    int       m_move_result;        // ROR 0xcc4: stepMotion / followPath, live sets 1
    int       _cc8[2];
    int       m_escape;             // ROR 0xcd0: escapeNoFloor: direction << 16 | probe step; -1 to start
    int       m_neck_control;       // ROR 0xcd4: 0 none, 1 smell, 2 look at m_neck_pos
    int       m_neck_changed;       // ROR 0xcd8: setNeckControl / setNeckPos, updateLook clears it
    int       m_neck_target;        // ROR 0xcdc: the neckblend 0..7, -1 none
    float     m_neck_rate;          // ROR 0xce0
    int       m_suspend;            // ROR 0xce4: suspend / resume
    int       m_suspend_motion;     // ROR 0xce8: the motion suspend saw
    MagLifeBwnMode *m_mode_cur;     // ROR 0xcec: applyState's pick
    MagLifeBwnModeNull   m_mode_null;   // ROR 0xcf0
    MagLifeBwnModeOne    m_mode_one;    // ROR 0xcf4
    MagLifeBwnModeWait   m_mode_wait;   // ROR 0xcf8
    MagLifeBwnModeSit    m_mode_sit;    // ROR 0xd04
    MagLifeBwnModeFuse   m_mode_fuse;   // ROR 0xd08
    MagLifeBwnModeMove   m_mode_move;   // ROR 0xd0c
    MagLifeBwnModePath   m_mode_path;   // ROR 0xd18
    MagLifeBwnModeTurn   m_mode_turn;   // ROR 0xd28
    MagLifeBwnModeWall   m_mode_wall;   // ROR 0xd2c
    MagLifeBwnModeRotate m_mode_rotate; // ROR 0xd30
};
static_assert(offsetof(MagLifeBwn, m_next_state) >= 0x80, "m_next_state");
static_assert(offsetof(MagLifeBwn, m_neck_pos) >= 0x90, "m_neck_pos");
static_assert(offsetof(MagLifeBwn, m_search_line) >= 0xa0, "m_search_line");
static_assert(offsetof(MagLifeBwn, m_search_width) >= 0x890, "m_search_width");
static_assert(offsetof(MagLifeBwn, m_goal_secta) >= 0xa8c, "m_goal_secta");
static_assert(offsetof(MagLifeBwn, m_search_path) >= 0xa9c, "m_search_path");
static_assert(offsetof(MagLifeBwn, m_num_search_line) >= 0xca0, "m_num_search_line");
static_assert(offsetof(MagLifeBwn, m_pos_index) >= 0xca8, "m_pos_index");
static_assert(offsetof(MagLifeBwn, m_health_status) >= 0xcb8, "m_health_status");
static_assert(offsetof(MagLifeBwn, m_move_result) >= 0xcc4, "m_move_result");
static_assert(offsetof(MagLifeBwn, m_escape) >= 0xcd0, "m_escape");
static_assert(offsetof(MagLifeBwn, m_neck_rate) >= 0xce0, "m_neck_rate");
static_assert(offsetof(MagLifeBwn, m_suspend) >= 0xce4, "m_suspend");
static_assert(offsetof(MagLifeBwn, m_mode_cur) >= 0xcec, "m_mode_cur");
static_assert(offsetof(MagLifeBwn, m_mode_rotate) >= 0xd30, "m_mode_rotate");
static_assert(sizeof(MagLifeBwn) >= 0xd40, "MagLifeBwn is 0xd40 bytes on the EE");

#endif /* _MAG_MAGLIFEBWN_H */
