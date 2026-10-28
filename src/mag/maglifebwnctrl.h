/* ==========================================================================
 *  mag/maglifebwnctrl.h
 *
 *  MagLifeBwnCtrl (mag/maglifebwnctrl.cpp, 0x1267c0..0x12bc20 plus the
 *  linkonce accessors; vtable _vt$14MagLifeBwnCtrl 0x2e13e0, base MagLife,
 *  vptr at +0x14, constructor 0x12ba80, EE 0x130 bytes): Brown's command
 *  layer over MagLifeBwn -- what Brown decides to do. The scene holds one
 *  at +0x2bd20; its state word is what MagSystem::getBwnState reads.
 *
 *  States (m_state; magBwnGetStateCode's four-char codes), each with a mode
 *  object embedded here (+0x8c..+0x130, vptrs set by the constructor):
 *  0 NULL, 1 WAIT, 2 FOLW, 3 STAY, 4 FIND, 5 FDED, 6 MOVE, 7 MVED, 8 ROTA,
 *  9 RTED, 10 DAMG, 11 BARK, 12 HUG, 13 (Wound, no code). requestState
 *  enters one at once (applyMode: the mode's init); live runs the current
 *  mode's live each frame unless the level is MANUAL. The modes talk to
 *  Brown through MagLifeBwn::message (its modes 0..9) and request the next
 *  state themselves; m_08 is the state's phase word.
 *
 *  The mode classes (two EE virtual slots: 1 init, 2 live, both taking the
 *  controller -- the asserts' theBwnCtrl) have no constructors of their own.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEBWNCTRL_H
#define _MAG_MAGLIFEBWNCTRL_H

#include <stddef.h>

#include "mag/maglife.h"
#include "utl/utlfind.h"

struct BedPad;
struct MagLifeBwn;
struct MagLifeBwnCtrl;
struct MagLifePC;

/* setLevel's levels (the assert's names; 1 has no name in the ROM). */
enum
{
    MAG_BWNCTRL_LEVEL_AUTO = 0,
    MAG_BWNCTRL_LEVEL_1 = 1,
    MAG_BWNCTRL_LEVEL_MANUAL = 2,
};

/* setMission's missions (mag/maglifebwnctrl.h's assert names the ends;
 * what MagLifeBwnCtrlModeNull::live makes of them names the rest). */
enum
{
    MISSION_NONE = 0,
    MISSION_FIND = 1,           // -> state 4 FIND
    MISSION_STAY = 2,           // -> state 3 STAY
    MISSION_3 = 3,              // never read by the modes written so far
    MISSION_HUG = 4,            // -> state 12 HUG
};

/* EE 4 bytes (the vptr). layout: host -- polymorphic. */
struct MagLifeBwnCtrlMode
{
    /* EE slot 1 (+0x08): MagLifeBwnCtrl::applyMode runs it on entry. */
    virtual void init(MagLifeBwnCtrl &theBwnCtrl) = 0;
    /* EE slot 2 (+0x10): MagLifeBwnCtrl::live runs it each frame. */
    virtual void live(MagLifeBwnCtrl &theBwnCtrl) = 0;

    /* Enemies within radius of Brown (0 while his health is 3 or more). */
    int  countEmyAround(MagLifeBwnCtrl &theBwnCtrl, float radius);       // ROR 0x001267f4
    /* An enemy within 200 of a healthy Brown: state 11 (BARK), 1. */
    int  checkBark(MagLifeBwnCtrl &theBwnCtrl);                          // ROR 0x001268b4
    /* Turns Brown to the controller's m_angle (state 9 once there). */
    void turnToAngle(MagLifeBwnCtrl &theBwnCtrl);                        // ROR 0x00126928
};

/* layout: host (EE 4 bytes, +0x8c). */
struct MagLifeBwnCtrlModeNull : MagLifeBwnCtrlMode
{
    void init(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00126ba0
    void live(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00126bd8
};

/* layout: host (EE 4 bytes, +0x90). */
struct MagLifeBwnCtrlModeWait : MagLifeBwnCtrlMode
{
    void init(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00126ccc
    void live(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00126e68
};

/* layout: host (EE 8 bytes, +0x94). */
struct MagLifeBwnCtrlModeFollow : MagLifeBwnCtrlMode
{
    void init(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x0012712c
    void live(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00127150

    int m_04;                   // ROR 0x04: init clears it
};

/* layout: host (EE 4 bytes, +0x9c). */
struct MagLifeBwnCtrlModeStay : MagLifeBwnCtrlMode
{
    void init(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00127a74
    void live(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00127a90
};

/* layout: host (EE 0x30 bytes, +0xa0). */
struct MagLifeBwnCtrlModeFind : MagLifeBwnCtrlMode
{
    void init(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00127b28
    void live(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00127e68

    int   _04[3];
    float m_pos[4];             // ROR 0x10: the constructor sets (0, 0, 0, 1)
    int   _20[4];
};

/* layout: host (EE 4 bytes, +0xd0). */
struct MagLifeBwnCtrlModeFindEnd : MagLifeBwnCtrlMode
{
    void init(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x0012890c
    void live(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00128970
};

/* layout: host (EE 4 bytes, +0xd4). */
struct MagLifeBwnCtrlModeMove : MagLifeBwnCtrlMode
{
    void init(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00128b8c
    void live(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00128bf0
};

/* layout: host (EE 4 bytes, +0xd8). */
struct MagLifeBwnCtrlModeMoveEnd : MagLifeBwnCtrlMode
{
    void init(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00128c4c
    // ROR 0x00128c80
    void live(MagLifeBwnCtrl &theBwnCtrl) override {}
};

/* layout: host (EE 4 bytes, +0xdc). */
struct MagLifeBwnCtrlModeRot : MagLifeBwnCtrlMode
{
    void init(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00128c88
    // ROR 0x00128c94
    void live(MagLifeBwnCtrl &theBwnCtrl) override { turnToAngle(theBwnCtrl); }
};

/* layout: host (EE 4 bytes, +0xe0). */
struct MagLifeBwnCtrlModeRotEnd : MagLifeBwnCtrlMode
{
    void init(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00128cb0
    // ROR 0x00128ce4
    void live(MagLifeBwnCtrl &theBwnCtrl) override {}
};

/* layout: host (EE 0xc bytes, +0xe4). */
struct MagLifeBwnCtrlModeDamage : MagLifeBwnCtrlMode
{
    void init(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00128cec
    void live(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00128dbc

    int _04[2];
};

/* layout: host (EE 0x30 bytes, +0xf0). */
struct MagLifeBwnCtrlModeBark : MagLifeBwnCtrlMode
{
    void init(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00128e20
    void live(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00129028
    /* barkQuery: 1 in phase 11 when v is the bark's answer, else -1. */
    int  query(MagLifeBwnCtrl &theBwnCtrl, int v);                       // ROR 0x00129f50
    /* barkSet: the controller's m_0c = v, phase 12. */
    void set(MagLifeBwnCtrl &theBwnCtrl, int v);                         // ROR 0x00129f78

    int   _04[3];
    float m_pos[4];             // ROR 0x10: the constructor sets (0, 0, 0, 1)
    int   _20;
    int   m_answer;             // ROR 0x24: what query compares; init clears it
    int   m_28;                 // ROR 0x28: init clears it
    int   _2c;
};

/* Unused: no state picks it and nothing constructs it (vtable 0x2e1478).
 * layout: host (no data). */
struct MagLifeBwnCtrlModeSitOut : MagLifeBwnCtrlMode
{
    // ROR 0x00129f88
    void init(MagLifeBwnCtrl &theBwnCtrl) override {}
    // ROR 0x00129f90
    void live(MagLifeBwnCtrl &theBwnCtrl) override {}
};

/* layout: host (EE 8 bytes, +0x120). */
struct MagLifeBwnCtrlModeHug : MagLifeBwnCtrlMode
{
    void init(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x00129f98
    void live(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x0012a03c
    // ROR 0x00283cc8
    void setMode(int mode) { m_mode = mode; }

    int m_mode;                 // ROR 0x04: 0 walk up to the PC, 1 / 2 the hug (requestHug's arg)
};

/* layout: host (EE 8 bytes, +0x128). */
struct MagLifeBwnCtrlModeWound : MagLifeBwnCtrlMode
{
    void init(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x0012a21c
    void live(MagLifeBwnCtrl &theBwnCtrl) override;                      // ROR 0x0012a2e8

    int _04;
};

/* layout: host -- polymorphic in the ROM (vptr +0x14); EE offsets in the
 * comments. */
struct MagLifeBwnCtrl : MagLife
{
    MagLifeBwnCtrl();                                                    // ROR 0x0012ba80
    /* EE slot 1 (+0x08) (linkonce) */
    // ROR 0x00283cd0
    ~MagLifeBwnCtrl() override {}

    /* EE slot 2 (+0x10): the current mode's live (state 0 first when
     * there is none), unless the level is MANUAL. */
    void live() override;                                                // ROR 0x0012a61c
    /* EE slot 5 (+0x28) (linkonce) */
    // ROR 0x00283dbc
    const char *getName() { return NULL; }
    /* EE slot 9 (+0x48): empty. */
    void processPad(BedPad *pad) {}                                      // ROR 0x0012a6a0
    /* Empty (MagScene::resetEntities). */
    void reset() {}                                                      // ROR 0x0012ab18

    /* Enters theMode at once (arg into the phase word m_08) unless the
     * cell under Brown is flagged 0x20000 (state 13 always goes); the same
     * state again only with force. */
    void requestState(int theMode, int arg, int force);                  // ROR 0x0012a56c
    /* Points m_mode_cur at m_state's mode and runs its init. */
    void applyMode();                                                    // ROR 0x0012a394
    /* requestState(10, react). */
    void requestHurt(int react);                                         // ROR 0x0012a6a8
    int  isHurt();                                                       // ROR 0x0012a6cc
    /* State 11: the bark sub object's answer for v (1 / -1); -1 otherwise. */
    int  barkQuery(int v);                                               // ROR 0x0012a6dc
    void barkSet(int v);                                                 // ROR 0x0012a71c
    /* m_find_item = item, the finder set for item >= 0; request: state 4
     * (again: the finder's flag), else no mission. */
    void setFindItem(int item, int request, int again);                  // ROR 0x0012a758
    /* State 4 / 5 back to 1. */
    void cancelFind();                                                   // ROR 0x0012a7cc
    /* Brown's actor position / rotation (vec4), each then requestState(0). */
    void setPos(const float *v);                                         // ROR 0x0012a808
    void setRot(const float *v);                                         // ROR 0x0012a89c
    void getPos(float *out);                                             // ROR 0x0012a974
    void getRot(float *out);                                             // ROR 0x0012a9ec
    /* Walks to xyz (state 6). */
    void moveTo(const float *xyz);                                       // ROR 0x0012aa70
    /* m_angle, then state 0 / state 8 (turn to it). */
    void setAngle(float a);                                              // ROR 0x0012aac0
    void rotateTo(float a);                                              // ROR 0x0012aae8
    int  isStateDone();                                                  // ROR 0x0012ab10
    /* The nearest item ahead of the PC (within 0.7 of its heading and 100
     * in y): its position into m_target, its ids into m_found_obj /
     * m_found_oid; 1 found. */
    int  findItemAhead();                                                // ROR 0x0012ab20
    /* Floor 300, 600 .. 3000 ahead of the PC: the farthest point before the
     * first gap into m_target; 1 when there was one. */
    int  findFloorAhead();                                               // ROR 0x0012adec
    /* Enemies within 500 of the point 500 ahead of the PC / of the PC
     * (Brown's health below 3). */
    int  isEmyAheadOfPC();                                               // ROR 0x0012af70
    int  isEmyNearPC();                                                  // ROR 0x0012b0d0
    /* Brown off by more than 3 degrees from facing the PC: the heading
     * into m_angle, 1. */
    int  aimAtPC();                                                      // ROR 0x0012b19c
    /* 1 unless Brown is within 100 in y and 130 of the PC. */
    int  isFarFromPC();                                                  // ROR 0x0012b300
    /* 1 unless Brown is within 100 in y and at least 95 from the PC. */
    int  isTooNearPC();                                                 // ROR 0x0012b3d0
    /* The PC's dog orders (MagLifePC::orderDog passes 0). */
    void orderCome(int a1);                                              // ROR 0x0012b4a0
    void orderStay(int a1);                                              // ROR 0x0012b5f8
    int  orderGo(int a1);                                                // ROR 0x0012b65c
    /* theLevel MAG_BWNCTRL_LEVEL_AUTO (0) .. MAG_BWNCTRL_LEVEL_MANUAL (2). */
    void setLevel(int theLevel);                                         // ROR 0x0012b7cc
    /* requestState(12 HUG, arg, 1); the Hug mode's m_mode = arg when arg >= 0. */
    void requestHug(int arg);                                            // ROR 0x0012b788
    /* > 0 when the hug can go on: phase 0 (start) or 1 / 2 (Brown's
     * distance and heading to the PC); -3 without Brown, -2 in state 3. */
    int  checkHug(int phase);                                            // ROR 0x0012b870
    /* Brown's position into out (untouched while Brown is unbound). */
    void getBwnPosition(float *out);                                     // ROR 0x0012ba2c
    int  isBarkPhase3();                                                 // ROR 0x0012ba54

    /* ---- linkonce accessors (the ELF's names) ---- */
    // ROR 0x00283d54 (mag/maglifebwnctrl.h line 385 in the ROM)
    void setMission(int m);
    // ROR 0x00283da4
    int  getMission() const { return m_mission; }
    // ROR 0x00283dac
    void setPC(MagLifePC *pc) { m_pc = pc; }
    // ROR 0x00283db4
    void setBwn(MagLifeBwn *bwn) { m_bwn = bwn; }
    // ROR 0x00283dc4
    int  getControlLevel() const { return m_ctrl_level; }
    // ROR 0x00283dcc
    int  getFoundTargetUID() const { return m_found_oid; }
    // ROR 0x00283dd4
    int  getFindResult() const { return m_find_result; }
    // ROR 0x00283ddc
    int  getFindItem() const { return m_find_item; }
    // ROR 0x00283de4
    void termWorld()
    {
        m_mission = MISSION_NONE;
        utlFindClear(&m_find);
    }

    u_char      _28[0x30 - 0x28];
    float       m_target[4];    // ROR 0x30: moveTo / findItemAhead / findFloorAhead's point (w 1)
    float       m_angle;        // ROR 0x40: setAngle / rotateTo / aimAtPC's heading
    int         m_find_item;    // ROR 0x44: setFindItem's item (op gfit reads it)
    UtlFind     m_find;         // ROR 0x48: cleared by MagScene::clearPC and termWorld
    int         m_60;           // ROR 0x60: the Follow mode clears it
    unsigned int m_found_obj;   // ROR 0x64: findItemAhead's object id (FindEnd marks it taken)
    int         m_found_oid;    // ROR 0x68: getFoundTargetUID (the object's u16 +0x128); op ifsc
    int         m_ctrl_level;   // ROR 0x6c: setLevel's MAG_BWNCTRL_LEVEL
    MagLifeBwn *m_bwn;          // ROR 0x70: Brown's controller while Brown is bound (MagScene::setBwn)
    MagLifePC  *m_pc;           // ROR 0x74: the PC's controller while Brown is bound
    int         m_mission;      // ROR 0x78: setMission's MISSION_*
    int         m_find_result;  // ROR 0x7c: op gfrs; FindEnd: 1 found, -1 gone
    int         m_state_done;   // ROR 0x80: isStateDone
    int         m_mode_count;   // ROR 0x84: frames in the current mode
    MagLifeBwnCtrlMode *m_mode_cur; // ROR 0x88: applyMode's pick
    MagLifeBwnCtrlModeNull    m_mode_null;     // ROR 0x8c
    MagLifeBwnCtrlModeWait    m_mode_wait;     // ROR 0x90
    MagLifeBwnCtrlModeFollow  m_mode_follow;   // ROR 0x94
    MagLifeBwnCtrlModeStay    m_mode_stay;     // ROR 0x9c
    MagLifeBwnCtrlModeFind    m_mode_find;     // ROR 0xa0
    MagLifeBwnCtrlModeFindEnd m_mode_find_end; // ROR 0xd0
    MagLifeBwnCtrlModeMove    m_mode_move;     // ROR 0xd4
    MagLifeBwnCtrlModeMoveEnd m_mode_move_end; // ROR 0xd8
    MagLifeBwnCtrlModeRot     m_mode_rot;      // ROR 0xdc
    MagLifeBwnCtrlModeRotEnd  m_mode_rot_end;  // ROR 0xe0
    MagLifeBwnCtrlModeDamage  m_mode_damage;   // ROR 0xe4
    MagLifeBwnCtrlModeBark    m_mode_bark;     // ROR 0xf0
    MagLifeBwnCtrlModeHug     m_mode_hug;      // ROR 0x120
    MagLifeBwnCtrlModeWound   m_mode_wound;    // ROR 0x128
};
static_assert(offsetof(MagLifeBwnCtrl, m_target) >= 0x30, "m_target");
static_assert(offsetof(MagLifeBwnCtrl, m_angle) >= 0x40, "m_angle");
static_assert(offsetof(MagLifeBwnCtrl, m_find_item) >= 0x44, "m_find_item");
static_assert(offsetof(MagLifeBwnCtrl, m_find) >= 0x48, "m_find");
static_assert(offsetof(MagLifeBwnCtrl, m_60) >= 0x60, "m_60");
static_assert(offsetof(MagLifeBwnCtrl, m_found_obj) >= 0x64, "m_found_obj");
static_assert(offsetof(MagLifeBwnCtrl, m_found_oid) >= 0x68, "m_found_oid");
static_assert(offsetof(MagLifeBwnCtrl, m_ctrl_level) >= 0x6c, "m_ctrl_level");
static_assert(offsetof(MagLifeBwnCtrl, m_bwn) >= 0x70, "m_bwn");
static_assert(offsetof(MagLifeBwnCtrl, m_pc) >= 0x74, "m_pc");
static_assert(offsetof(MagLifeBwnCtrl, m_mission) >= 0x78, "m_mission");
static_assert(offsetof(MagLifeBwnCtrl, m_find_result) >= 0x7c, "m_find_result");
static_assert(offsetof(MagLifeBwnCtrl, m_state_done) >= 0x80, "m_state_done");
static_assert(offsetof(MagLifeBwnCtrl, m_mode_count) >= 0x84, "m_mode_count");
static_assert(offsetof(MagLifeBwnCtrl, m_mode_cur) >= 0x88, "m_mode_cur");
static_assert(offsetof(MagLifeBwnCtrl, m_mode_null) >= 0x8c, "m_mode_null");
static_assert(offsetof(MagLifeBwnCtrl, m_mode_find) >= 0xa0, "m_mode_find");
static_assert(offsetof(MagLifeBwnCtrl, m_mode_bark) >= 0xf0, "m_mode_bark");
static_assert(offsetof(MagLifeBwnCtrl, m_mode_hug) >= 0x120, "m_mode_hug");
static_assert(offsetof(MagLifeBwnCtrl, m_mode_wound) >= 0x128, "m_mode_wound");
static_assert(sizeof(MagLifeBwnCtrl) >= 0x130, "MagLifeBwnCtrl is 0x130 bytes on the EE");

#endif /* _MAG_MAGLIFEBWNCTRL_H */
