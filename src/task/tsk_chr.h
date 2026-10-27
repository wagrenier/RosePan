/* ==========================================================================
 *  task/tsk_chr.h
 *
 *  The script's tasks (task/tsk_chr.cpp, pinned 0x1e7f28..0x1f6c64; no
 *  source string names the file -- it sits between task/tsk_man.cpp and
 *  task/tsk_scene.cpp, the name is descriptive):
 *  - the achr motion / movement tasks the natives start on an achr's task
 *    slots (ACHRstat::m_task[]): TskMtnMove / TskMtnMove2 (a motion, the
 *    actor moved by its root motion), TskMoveType*, TskMovePointPath,
 *    TskMoveSecta*, TskMoveAdj, TskRotAdj, TskMovePhysic, TskAvoidMove,
 *    TskChrRot (turns), TskAttach, and their chr* starters;
 *  - the generic script tasks: TskMovie (a film), TskMsgMan (a message
 *    window), TskSeqEvent, TskSysFunc, TskFrameFunc / TskGenFrameFunc
 *    (per-frame value changes).
 *  Every class derives from CmnITask (task/tsk_man.h) and has the six-slot
 *  vtable: get_task_type, pause, init, exec, term, the destructor.
 * ======================================================================== */
#ifndef _TASK_TSK_CHR_H
#define _TASK_TSK_CHR_H

#include <stddef.h>
#include <stdint.h>

#include "sdk/eetypes.h"
#include "i3d/i3denv.h"
#include "game/gamstat.h"
#include "msg/msgman.h"
#include "task/tsk_man.h"

struct ACHRstat;
struct DogActor;
struct MnuTextList;

/* What ACHRstat +0x3d0 points at: only its actor no is read (it matches
 * TskMtnMove's m_actor; the writer is not found yet). EE layout (the rest
 * unknown). */
struct ChrMotionRef
{
    int _00[3];
    int m_actor;                // ROR 0x0c: the DogActor no
};
static_assert(offsetof(ChrMotionRef, m_actor) == 0x0c, "ChrMotionRef m_actor");
/* !DogActor::isModeSet of ref's actor. */
int chrMotionRefEnd(const ChrMotionRef *ref);                            // ROR 0x001e8f88

/* TskMtnMove : CmnITask (vtable 0x2e4910, EE 0xc0 bytes; kind 5): plays
 * motion on actor no (DogActor::changeMotion unless it already runs) and
 * moves the actor by the motion's root move each frame (MagSystem::
 * getActorMotionMove / addActorTrans, turned by the heading m_rotY) until
 * the motion's length. The step function is picked once: no actor -> the
 * task ends; an achr off the secta lists (flag 0x40 without 0x80) or with
 * 0x4000000 -> stepMoveHit (the move refused when something is in the
 * way or no cell is there); else stepMove. chrMtnMove / chrChangeMotion
 * start it on the achr's m_task[2]. layout: host -- pointers and a
 * member-function pointer; EE offsets in the comments. */
struct TskMtnMove : CmnITask
{
    typedef int (TskMtnMove::*StepFunc)();

    /* speed in frames per frame (f12), rotY the heading (>= 32769: keep
     * the actor's), no the scene actor (-1: none), motion its number, p1d8
     * changeMotion's last word (-1: 0 unless oid's type is 2), flags
     * (bits 8.. and 0..7 changeMotion's two flag words), oid the object. */
    TskMtnMove(float speed, float rotY, int no, int motion, int p1d8, u_int flags,
               u_int oid);                                               // ROR 0x001e7f28
    ~TskMtnMove() override {}                                            // ROR 0x0028ffd8

    int  get_task_type() override { return 5; }                          // ROR 0x0029000c
    void pause(int on) override;                                         // ROR 0x001e8478
    void init() override {}                                              // ROR 0x00290060
    int  exec(int id, int time, int arg) override;                       // ROR 0x001e8410
    void term() override;                                                // ROR 0x001e8408

    int  stepMove();                                                     // ROR 0x001e85d0
    int  stepMoveHit();                                                  // ROR 0x001e89c0
    int  stepNone();                                                     // ROR 0x001e8fbc

    u_int     m_flags;          // ROR 0x04: bit 0 stops the moving (TskMoveTypeGen::stepMotion), bit 1 paused
    u_int     m_oid;            // ROR 0x08
    int       m_actor;          // ROR 0x0c: the scene actor no, re-read from m_oid each step
    int       m_motion;         // ROR 0x10
    float     m_pos[4];         // ROR 0x20: stepMoveHit's target position
    float     m_mpos[4];        // ROR 0x30: getActorMotionMove's position ([1] goes to addActorTrans)
    float     m_move[4];        // ROR 0x40: the root move (x, -, z)
    float     m_trans[4];       // ROR 0x50: the move turned by -m_rotY
    ACHRstat *m_achr;           // ROR 0x60: NULL unless oid is an achr
    DogActor *m_dogActor;       // ROR 0x64: GamMan::getActor(oid)
    float     m_flagsF;         // ROR 0x68: flags as a float (not read here)
    float     m_rotY;           // ROR 0x6c: the heading
    float     m_rotYOut;        // ROR 0x70: m_rotY after each step
    float     m_end;            // ROR 0x74: the motion's length * 120 (getMotionFrame120 units)
    float     m_speed;          // ROR 0x78
    float     m_step;           // ROR 0x7c: speed / (1/60)
    StepFunc  m_stepFunc;       // ROR 0x80: GCC 2 {delta, index, pfn}
};

/* TskMtnMove2 : CmnITask (vtable 0x2e48d0, EE 0x100 bytes; kind 5): the
 * variant of TskMtnMove native 100 starts through chrMtnMove2. The
 * motion (changeMotion on the achr's actor unless it runs; flag 4 then)
 * moves the actor by its root move turned by the actor's base heading
 * (m_base_rotate[1]) through DogActor::getMotionMove / addTrans; while
 * the motion stands it only takes the motion's turn. The achrs
 * TskMtnMove sends to stepMoveHit get stepMoveHit here too: a move into
 * something (testHitAt from the Bip01 Spine1 node, or no cell 1.25 radii
 * ahead) is refused, and when the actor stands in something a
 * TskAvoidMove starts on the achr's m_taskId. layout: host -- pointers
 * and a member-function pointer; EE offsets in the comments. */
struct TskMtnMove2 : CmnITask
{
    typedef int (TskMtnMove2::*StepFunc)();

    TskMtnMove2(float speed, int no, int motion, int p1d8, u_int flags, u_int oid); // ROR 0x001e8fc4
    ~TskMtnMove2() override {}                                           // ROR 0x002900b8

    int  get_task_type() override { return 5; }                          // ROR 0x002900ec
    void pause(int on) override;                                         // ROR 0x001e9294
    void init() override {}                                              // ROR 0x00290140
    int  exec(int id, int time, int arg) override;                       // ROR 0x001e922c
    void term() override;                                                // ROR 0x001e9224

    int  stepMove();                                                     // ROR 0x001e93ec
    int  stepMoveHit();                                                  // ROR 0x001e9614
    int  stepNone();                                                     // ROR 0x001e9a34

    u_int     m_flags;          // ROR 0x04: bit 0 stops the moving, bit 1 paused, bit 2 the motion was changed
    u_int     m_oid;            // ROR 0x08
    int       m_actor;          // ROR 0x0c: the scene actor no, re-read from m_oid each step
    int       m_motion;         // ROR 0x10
    float     m_pos[4];         // ROR 0x20: stepMoveHit's position (the Bip01 Spine1 node or the achr's)
    float     m_mpos[4];        // ROR 0x30: getMotionMove's position ([1] goes to addTrans)
    float     m_move[4];        // ROR 0x40: the root move
    ACHRstat *m_achr;           // ROR 0x50: NULL unless oid is an achr
    DogActor *m_dogActor;       // ROR 0x54: GamMan::getActor(oid)
    float     m_flagsF;         // ROR 0x58: flags as a float (not read here)
    float     m_rotY;           // ROR 0x5c: the actor's base heading
    float     m_rotYOut;        // ROR 0x60: m_rotY after each step
    float     m_end;            // ROR 0x64: the motion's length * 120
    float     m_speed;          // ROR 0x68
    float     m_step;           // ROR 0x6c: speed / (1/60)
    float     m_achrPos[4];     // ROR 0x70: the achr's position (stepMoveHit; not read)
    int       m_c4;             // ROR 0xc4: 0 (not read here)
    float     m_radius;         // ROR 0xc8: GamMan::getObjRadius (stepMoveHit's look-ahead)
    StepFunc  m_stepFunc;       // ROR 0xcc: GCC 2 {delta, index, pfn}
};

/* The hit tests' result record (GamMan::hitAtPoint / hitAtSweep clear
 * it and fill +0x10..+0x18; 8 words on the EE). */
struct ChrHitRec
{
    int   _00[4];
    u_int m_oid;                // ROR 0x10: the object hit, 0 none
    float m_depth;              // ROR 0x14: how far inside its radius (10 for a quad)
    int   m_far;                // ROR 0x18: 1 hit round the sweep end / the Bip01 Neck
    int   _1c;
};
static_assert(offsetof(ChrHitRec, m_oid) == 0x10, "ChrHitRec m_oid");
static_assert(offsetof(ChrHitRec, m_depth) == 0x14, "ChrHitRec m_depth");
static_assert(offsetof(ChrHitRec, m_far) == 0x18, "ChrHitRec m_far");
static_assert(sizeof(ChrHitRec) == 0x20, "ChrHitRec size");

/* TskAvoidMove : CmnITask (vtable 0x2e4b50, EE 0xc0 bytes; kind 0): an
 * achr pushed off what it stands in or walks into, on its m_taskId
 * (GamMan::placeAchr / testHitAt and TskMtnMove2::stepMoveHit start it
 * with frames 8, limit 30 or 16). From the hit record's object (hit !=
 * NULL): byHit 0 -> away from it at half its distance / frames a frame
 * (stepAway: round the cells, 8 directions, when no cell lies ahead;
 * ends after frames frames once nothing is hit), byHit != 0 -> away at
 * its distance / frames (stepAwayHit: ends when nothing is hit, refused
 * for objects of kind 1 / 2); toward the point to (hit NULL) at most
 * 1/30 a frame (stepToward: 9 frames on cells, faster each limit frames
 * stuck). The PC, a GamMan m_flags2 0x3000 state, or an object flagged
 * 0x4000 end it at once. While it runs the achr carries flag 0x4000.
 * Each step also ends after limit frames. layout: host -- pointers and
 * a member-function pointer; EE offsets in the comments. */
struct TskAvoidMove : CmnITask
{
    typedef int (TskAvoidMove::*StepFunc)();

    TskAvoidMove(u_int oid, const void *hit, int frames, int limit, const float *to,
                 int byHit);                                             // ROR 0x001ef184
    ~TskAvoidMove() override {}                                          // ROR 0x001ef414

    int  get_task_type() override { return 0; }                          // ROR 0x00290498
    void init() override {}                                              // ROR 0x002904a0
    int  exec(int id, int time, int arg) override;                       // ROR 0x001f0398
    void term() override;                                                // ROR 0x001ef448

    /* The set-ups: away from object target (by half its distance; byHit
     * 0), away by its distance (byHit != 0), toward the point to. */
    void aimAway(u_int target, const float *pos, int frames);            // ROR 0x001efab8
    void aimAwayHit(u_int target, const float *pos, int frames);         // ROR 0x001efeb8
    void aimToward(u_int oid, const float *pos, const float *to, int frames); // ROR 0x001f0288
    int  stepAway();                                                     // ROR 0x001ef498
    int  stepAwayHit();                                                  // ROR 0x001ef888
    int  stepToward();                                                   // ROR 0x001ef9cc
    int  stepNone();                                                     // ROR 0x001efab0

    ACHRstat     *m_achr;          // ROR 0x04
    u_int         m_oid;           // ROR 0x0c
    u_int         m_target;        // ROR 0x10: the object pushed from
    int           m_actor;         // ROR 0x14: the achr's actor
    sceVu0FVECTOR m_pos;           // ROR 0x20: the achr's position (the Bip01 Spine1 node at the start)
    sceVu0FVECTOR m_vel;           // ROR 0x30: the push a frame
    sceVu0FVECTOR m_ahead;         // ROR 0x40: stepAway's look-ahead (m_vel x 1.25)
    int           m_frames;        // ROR 0x50
    int           m_58;            // ROR 0x58: 0 (not read)
    int           m_count;         // ROR 0x60: frames pushed
    int           m_still;         // ROR 0x64: stepToward's frames on cells
    int           m_limit;         // ROR 0x68
    int           m_time;          // ROR 0x6c: the frames the set-ups get
    int           m_tries;         // ROR 0x70: frames run, against m_limit
    float         m_dir;           // ROR 0x74: the push heading
    float         m_speed;         // ROR 0x78: the push length a frame
    StepFunc      m_stepFunc;      // ROR 0x7c: GCC 2 {delta, index, pfn}
};

/* TskMoveAdj : CmnITask (vtable 0x2e4990; kind 0xd): scene actor actor
 * eased to pos -- count frames at height ref[2] x (count / the actor's
 * +0x1d8) - 7.5 above it, then at pos, and ends. No caller in the ROM.
 * layout: host -- a pointer; EE offsets in the comments. */
struct TskMoveAdj : CmnITask
{
    /* base: the object whose +0x10 vector m_ref is. a / b are not read. */
    TskMoveAdj(float a, int actor, const float *base, int count, const float *pos,
               int b);                                                   // ROR 0x001ee2fc
    ~TskMoveAdj() override {}                                            // ROR 0x001ee330

    int  get_task_type() override { return 0xd; }                        // ROR 0x00290378
    void init() override {}                                              // ROR 0x00290380
    int  exec(int id, int time, int arg) override;                       // ROR 0x001ee36c
    void term() override {}                                              // ROR 0x001ee364

    sceVu0FVECTOR m_pos;           // ROR 0x10
    int           m_actor;         // ROR 0x20
    float         m_a;             // ROR 0x24: not read
    const float  *m_ref;           // ROR 0x28: base + 4 floats
    int           m_len;           // ROR 0x2c: the actor's +0x1d8 each frame
    int           m_count;         // ROR 0x30: frames left
    int           m_b;             // ROR 0x34: not read
};

/* TskRotAdj : CmnITask (vtable 0x2e4950; kind 0xe): scene actor actor
 * turned by angle about y over frames frames (frames < 1: at once). No
 * caller in the ROM. layout: host; EE offsets in the comments. */
struct TskRotAdj : CmnITask
{
    TskRotAdj(float angle, int actor, int frames);                       // ROR 0x001ee4a8
    ~TskRotAdj() override {}                                             // ROR 0x001ee6b0

    int  get_task_type() override { return 0xe; }                        // ROR 0x002903d8
    void init() override {}                                              // ROR 0x002903e0
    int  exec(int id, int time, int arg) override;                       // ROR 0x001ee6ec
    void term() override {}                                              // ROR 0x001ee6e4

    int           m_actor;         // ROR 0x04
    float         m_angle;         // ROR 0x08: angle wrapped into -pi..pi (not read)
    float         m_step;          // ROR 0x0c: angle / frames
    int           m_count;         // ROR 0x10: frames left
    int           m_frames;        // ROR 0x14
    sceVu0FVECTOR m_rot;           // ROR 0x20: the actor's rotation (w not set)
};

/* TskMovePhysic : CmnITask (vtable 0x2e4b10, EE 0x80 bytes; kind 0): an
 * fchr thrown along its heading (0x1fd9bc and two more item users): each
 * frame t = frames / 30 moves it h(t) = 16t - 29.4t^2 up (y down) and
 * 24 - 16t along (both as the change over the last half second after
 * t 0.5; the along speed keeps its last value at 16 and below). A wall
 * (MagScene::castRay) stops it there and turns the heading back at a
 * quarter (stopped when the bounce hits too). Within 100 of the PC (or
 * else the dog) it calls the character request 0x1fac60 (4, or 3) with
 * 0x15, once. Ends below its cell's floor. layout: host -- pointers and
 * a member-function pointer; EE offsets in the comments. */
struct TskMovePhysic : CmnITask
{
    typedef int (TskMovePhysic::*StepFunc)();

    TskMovePhysic(u_int oid);                                            // ROR 0x001eeaec
    ~TskMovePhysic() override {}                                         // ROR 0x001eec08

    int  get_task_type() override { return 0; }                          // ROR 0x00290438
    void init() override {}                                              // ROR 0x00290440
    int  exec(int id, int time, int arg) override;                       // ROR 0x001ef11c
    void term() override {}                                              // ROR 0x001eec3c

    int  stepThrow();                                                    // ROR 0x001eec4c

    FCHRstat     *m_fchr;          // ROR 0x08
    u_int         m_oid;           // ROR 0x0c
    int           m_actor;         // ROR 0x10
    sceVu0FVECTOR m_pos;           // ROR 0x20
    sceVu0FVECTOR m_rot;           // ROR 0x30: the actor's rotation
    sceVu0FVECTOR m_dir;           // ROR 0x40: (sin, 1, cos) of the heading
    int           m_58;            // ROR 0x58: 0 (not read)
    u_int         m_flags;         // ROR 0x5c: 1 the character request is still to come
    float         m_speed;         // ROR 0x60: the last along speed
    int           m_count;         // ROR 0x64: frames run
    StepFunc      m_stepFunc;      // ROR 0x70: GCC 2 {delta, index, pfn}
};
/* The throw's height 16t - 29.4t^2 and along speed 24 - 16t at t. */
float chrThrowHeight(float t);                                           // ROR 0x001ef0d8
float chrThrowSpeed(float t);                                            // ROR 0x001ef100

/* TskGenFrameFunc : CmnITask (vtable 0x2e4d10; kind 0): a one-frame job,
 * kind 1 plays sound effect value (0x1f97c8). Other kinds set no step
 * (the ROM then calls a garbage pointer). layout: host -- a member-
 * function pointer; EE offsets in the comments. */
struct TskGenFrameFunc : CmnITask
{
    typedef int (TskGenFrameFunc::*StepFunc)();

    TskGenFrameFunc(int kind, int value, int a3);                       // ROR 0x001f4088
    ~TskGenFrameFunc() override {}                                       // ROR 0x001f40c0

    void init() override {}                                              // ROR 0x002905c0
    int  exec(int id, int time, int arg) override;                       // ROR 0x001f40f4
    void term() override {}                                              // ROR 0x002905c8

    int  stepPlaySe();                                                   // ROR 0x001f415c

    StepFunc m_stepFunc;        // ROR 0x08: GCC 2 {delta, index, pfn}
    int      m_value;           // ROR 0x10
};

/* TskFrameFunc : CmnITask (vtable 0x2e4d50, EE 0x80 bytes; kind 0x11):
 * the script's and GamMan's multi-frame jobs. The constructor's switch
 * on kind (jump table data 0x2fe420) copies what the job needs and picks
 * the step exec calls each frame until it returns non-zero (member
 * pointers data 0x2fe2c0..0x2fe418). arg, by kind: 2..5 -> int frames;
 * 6, 14 a value; 7 -> {oid, angle, distance, height, camera rotX} words;
 * 8 -> intptr_t {items, mask, x, y}; 11 -> int {thread, entry, room,
 * word}; 12 -> an I3dFog; 15 -> a texture name; 19 -> int {item, word,
 * mode}. frames: kinds 12, 15, 16, 18. Kinds 0, 1, 13, 20 and >= 23 set
 * no step (the ROM then calls a garbage pointer). layout: host -- a
 * member-function pointer; EE offsets in the comments. */
struct TskFrameFunc : CmnITask
{
    typedef int (TskFrameFunc::*StepFunc)();

    TskFrameFunc(int kind, intptr_t arg, int frames);                    // ROR 0x001f184c
    ~TskFrameFunc() override {}                                          // ROR 0x001f26e4

    int  get_task_type() override { return 0x11; }                      // ROR 0x00290560
    void init() override {}                                              // ROR 0x00290568
    int  exec(int id, int time, int arg) override;                       // ROR 0x001f2778
    void term() override;                                                // ROR 0x001f2718

    int  stepKeyIn1();                                                   // ROR 0x001f27e0
    int  stepKeyOut1();                                                  // ROR 0x001f2920
    int  stepKeyIn2();                                                   // ROR 0x001f2a84
    int  stepKeyOut2();                                                  // ROR 0x001f2bcc
    int  stepSysMsg();                                                   // ROR 0x001f2d38
    int  stepWaitAchrMotion();                                           // ROR 0x001f2ec4
    int  stepGameOver();                                                 // ROR 0x001f2f0c
    int  stepBg();                                                       // ROR 0x001f34c8
    int  stepTextList();                                                 // ROR 0x001f370c
    int  stepMenuPause();                                                // ROR 0x001f3760
    int  stepClearFlag100();                                             // ROR 0x001f3788
    int  stepClearFlag400000();                                          // ROR 0x001f37b8
    int  stepPcMsg();                                                    // ROR 0x001f3824
    int  stepClearPcRot();                                               // ROR 0x001f3b3c
    int  stepStageThread();                                              // ROR 0x001f3b74
    int  stepFog();                                                      // ROR 0x001f3c44
    int  stepNone();                                                     // ROR 0x001f3d90
    int  stepGetItem();                                                  // ROR 0x001f3d98

    int          m_kind;        // ROR 0x04
    StepFunc     m_stepFunc;    // ROR 0x08: GCC 2 {delta, index, pfn}
    int          m_state;       // ROR 0x10: the step's state (0 unless the kind sets one)
    int          m_count;       // ROR 0x14: frames counted by the step
    MnuTextList *m_list;        // ROR 0x18: kind 8's list
    union
    {
        int    m_word[16];      // ROR 0x20: the kind's words (see the steps)
        float  m_float[16];     // ROR 0x20
        I3dFog m_fog[2];        // ROR 0x20: kind 12, the fog from / to
        char   m_name[0x20];    // ROR 0x20: kind 15, S2dBg's texture name (m_word[8] ends it)
    };
    int          m_frames;      // ROR 0x60: kind 12's length
};

/* TskMovie : CmnITask (vtable 0x2e4c50, EE 0x80 bytes; kind 0): native
 * 253's film task (GamMan::m_movieTaskId). The constructor fades the
 * sound and the message sprite 3 out, sets the script bit 0x8000000 of
 * word 1, plays the film name (S2dMovie::playMovie) and runs the game at
 * 60 fps. exec then: 100 waits for the film's end, 0x65 stops it, 0x66
 * waits 2 frames and fades the scene in, 0x67 fades out at once when
 * START is held, else waits for RxMpeg::checkNearEnd, 0x68 saves the BGM
 * volume and fades it to 0, 0x69 after 31 frames closes the film, 30 fps
 * again, fades in (or clears the script bit 0x80000000 of word 1 that
 * asks not to) and restores the volume; 0x6a ends the task. term undoes
 * the constructor. layout: host. */
struct TskMovie : CmnITask
{
    TskMovie(const char *name);                                          // ROR 0x001f438c
    ~TskMovie() override;                                                // ROR 0x001f448c

    void init() override {}                                              // ROR 0x00290714
    int  exec(int id, int time, int arg) override;                       // ROR 0x001f454c
    void term() override;                                                // ROR 0x001f44e8

    int  m_04;                  // ROR 0x04: 0 (not read)
    int  m_state;               // ROR 0x08: 100..0x6a; native 253 case 4 waits while <= 100
    int  m_count;               // ROR 0x0c: frames in states 0x66 / 0x69
    char m_name[0x40];          // ROR 0x10
    int  m_vol[2];              // ROR 0x50: GamSndMan::m_mstVol saved by state 0x68
};

/* TskMoveTypeGen : CmnITask (vtable 0x2e4ad0, EE 0xc0 bytes; kind 8):
 * native 202's straight moves of an achr (on its m_task[0]). By mode & 7:
 * 0 by the vector arg each frame for frames frames, 1 along the heading at
 * the speed *arg for frames frames, 2 the distance *arg along the heading
 * over frames frames, 3 / 4 toward the object arg at speed frames (4 aims
 * again every kind frames, 3 when it gets there; a negative speed walks
 * backwards), 5 / 7 to the point arg at speed frames, 6 to the point arg in
 * frames frames. Modes 0..7 slide the achr (stepSlide / stepSlideHit)
 * while the achr's walk motion (param 0x1100 at param 0x1103 / 60) plays;
 * modes 8.. (an achr flagged 0x200000) let a chrMtnMove of it walk there
 * (stepMotion). Before setting off the achr turns to the heading (param
 * 0x1104 its speed): a heading within param 0x1105 of straight behind plays
 * the turn motion (0x1101 right, 0x1102 left) and waits for it (stepTurn).
 * An achr flagged 0x400000 holds still while a turn task runs. Modes 0..2
 * and 5.. end after their frames; 3 / 4 follow the object until killed.
 * rate is not read. layout: host -- pointers and a member-function pointer;
 * EE offsets in the comments. */
struct TskMoveTypeGen : CmnITask
{
    typedef int (TskMoveTypeGen::*StepFunc)();

    TskMoveTypeGen(u_int oid, int mode, intptr_t arg, int kind, float frames, float rate); // ROR 0x001e9a3c
    ~TskMoveTypeGen() override {}                                        // ROR 0x001e9c78

    int  get_task_type() override { return 8; }                          // ROR 0x00290198
    void init() override {}                                              // ROR 0x002901a0
    int  exec(int id, int time, int arg) override;                       // ROR 0x001eae7c
    void term() override;                                                // ROR 0x001e9cac

    /* Sets off by mode (the velocity, frames, motion and step); returns
     * the heading to take. */
    float aim();                                                         // ROR 0x001e9cb4
    /* Modes 3 / 4 / 11 / 12: aim again at the object from where the actor
     * stands; returns the heading. */
    float retarget();                                                    // ROR 0x001ea5ec
    /* The turn to dir before (or while) moving. */
    void  turnTo(float dir);                                             // ROR 0x001eafb0
    /* The frames ran out: 1 ends the task, the following modes aim again. */
    int   next();                                                        // ROR 0x001eaf00
    int   stepTurn();                                                    // ROR 0x001eaa38
    int   stepSlideHit();                                                // ROR 0x001eab54
    int   stepSlide();                                                   // ROR 0x001eac38
    int   stepMotion();                                                  // ROR 0x001eacb8

    ACHRstat     *m_achr;          // ROR 0x04
    u_int         m_oid;           // ROR 0x08
    int           m_actor;         // ROR 0x0c: the achr's actor (ObjStat::m_actor)
    u_int         m_targetOid;     // ROR 0x10: modes 3 / 4, the object followed
    float         m_speed;         // ROR 0x14: the slide per frame
    float         m_dist;          // ROR 0x18: mode 2's distance
    int           m_mode;          // ROR 0x1c
    sceVu0FVECTOR m_target;        // ROR 0x20: modes 3.., the position walked to
    sceVu0FVECTOR m_pos;           // ROR 0x30
    sceVu0FVECTOR m_rot;           // ROR 0x40: [1] the heading
    sceVu0FVECTOR m_vel;           // ROR 0x50: the slide (modes 0..7)
    int           m_frames;        // ROR 0x60: frames left
    int           m_half;          // ROR 0x64: stepMotion aims again below it
    int           m_every;         // ROR 0x68: modes 4 / 12, kind: the frames between aims
    intptr_t      m_arg;           // ROR 0x6c
    int           m_motion;        // ROR 0x70: param 0x1100, the walk motion
    float         m_rate;          // ROR 0x74: param 0x1103 / 60, its speed
    float         m_rotSpeed;      // ROR 0x78: param 0x1104, the turn speed
    StepFunc      m_stepFunc;      // ROR 0x7c: GCC 2 {delta, index, pfn}
    CmnITask     *m_mtnTask;       // ROR 0x84: modes 8.., chrMtnMove's (a TskMtnMove)
    sceVu0FVECTOR m_lastPos;       // ROR 0x90: the last position placed
    float         m_value;         // ROR 0xa0: frames (modes 0..2, 6) or the speed (3..5, 7)
    int           m_kind;          // ROR 0xa4
    u_int         m_flags;         // ROR 0xa8: 1 hold while a turn task runs, 2 the motion held
    int           m_turnRight;     // ROR 0xac: param 0x1101, -1 none
    int           m_turnLeft;      // ROR 0xb0: param 0x1102, -1 none
    float         m_turnBack;      // ROR 0xb4: param 0x1105, the angle from behind that plays them
};
/* TskMoveTypeRound : CmnITask (vtable 0x2e4a90, EE 0x80 bytes; kind 9):
 * native 211 cases 10 / 11 -- an achr circles a centre at radius (kept
 * within 10 of it), walking at speed along the circle (dir -1 one way,
 * +1 the other: a blocked move turns it round). By mode: 0 slides around
 * the actor of object arg (chrSetMotion of motion at rate, facing the
 * centre when off the ring), 1 slides around the point arg, 2 / 3 let a
 * chrMtnMove of motion walk around the object / the point (its heading
 * set each frame). exec places the achr (GamMan::placeAchr, then
 * ObjStat::trySetPos); a refused move prints "HIT in moving task", keeps
 * the last position and flags the achr 0x1000. Never ends by itself.
 * layout: host -- pointers and a member-function pointer; EE offsets in
 * the comments. */
struct TskMoveTypeRound : CmnITask
{
    typedef void (TskMoveTypeRound::*StepFunc)();

    TskMoveTypeRound(float radius, float speed, float rate, u_int oid, int mode, intptr_t arg,
                     int motion);                                        // ROR 0x001eb63c
    ~TskMoveTypeRound() override {}                                      // ROR 0x001eb944

    int  get_task_type() override { return 9; }                         // ROR 0x002901f8
    void init() override {}                                              // ROR 0x00290200
    int  exec(int id, int time, int arg) override;                       // ROR 0x001eb980
    void term() override;                                                // ROR 0x001eb978

    void stepPoint();                                                    // ROR 0x001ebb80
    void stepObjMotion();                                                // ROR 0x001ebde8
    void stepObj();                                                      // ROR 0x001ec0e4

    ACHRstat     *m_achr;          // ROR 0x04
    u_int         m_oid;           // ROR 0x08
    int           m_actor;         // ROR 0x0c: the achr's actor
    u_int         m_target;        // ROR 0x10: modes 0 / 2, the centre object
    int           m_targetActor;   // ROR 0x14: its actor
    u_int         m_flags;         // ROR 0x18: 1 blocked, 2 turned round (cleared each 60th frame), 4 aimed (each 180th)
    float         m_radius;        // ROR 0x1c
    float         m_dir;           // ROR 0x24: 1 or -1, the way round
    float         m_speed;         // ROR 0x28
    int           m_mode;          // ROR 0x2c
    sceVu0FVECTOR m_center;        // ROR 0x30
    sceVu0FVECTOR m_pos;           // ROR 0x40
    sceVu0FVECTOR m_rot;           // ROR 0x50: [1] the heading
    StepFunc      m_stepFunc;      // ROR 0x60: GCC 2 {delta, index, pfn}
    TskMtnMove   *m_mtnTask;       // ROR 0x68: modes 2 / 3, chrMtnMove's
    sceVu0FVECTOR m_lastPos;       // ROR 0x70: the last position placed
};
/* One point of a TskMovePointPath path (the script's x, z pairs). */
struct ChrPathPoint
{
    float x;                    // ROR 0x0
    float z;                    // ROR 0x4
};
static_assert(sizeof(ChrPathPoint) == 8, "ChrPathPoint size");

/* TskMovePointPath : CmnITask (vtable 0x2e4a50, EE 0xc0 bytes; kind 10):
 * native 211 cases 20..23 -- an achr walks the count points of path
 * from point to point. mode & 3 picks the order (member pointers data
 * 0x2fdd60..): 0 each point once, 1 round and round, 2 count random
 * points, 3 random points for ever. Modes 0..3 slide the achr at speed
 * per frame (sceVu0AddVector + ObjStat::trySetPos, facing the point)
 * while motion plays (chrSetMotion at rate); modes 4..7 let a
 * chrMtnMove motion walk it there (chrMoveFrames gives the frames). At
 * the end motion 1 plays. layout: host -- pointers and a member-function
 * pointer; EE offsets in the comments. */
struct TskMovePointPath : CmnITask
{
    typedef int (TskMovePointPath::*StepFunc)();

    TskMovePointPath(float speed, float rate, u_int oid, int mode, int count,
                     const ChrPathPoint *path, int motion);              // ROR 0x001ec344
    ~TskMovePointPath() override {}                                      // ROR 0x001ec59c

    int  get_task_type() override { return 0xa; }                        // ROR 0x00290258
    void init() override {}                                              // ROR 0x00290260
    int  exec(int id, int time, int arg) override;                       // ROR 0x001ec5d8
    void term() override;                                                // ROR 0x001ec5d0

    /* Aims at m_point: the heading, the velocity and the frames to it. */
    void aim();                                                          // ROR 0x001ec834
    int  stepOnce();                                                     // ROR 0x001ec6e8
    int  stepLoop();                                                     // ROR 0x001ec72c
    int  stepRandom();                                                   // ROR 0x001ec774
    int  stepRandomLoop();                                               // ROR 0x001ec7e8

    ACHRstat           *m_achr;        // ROR 0x04
    u_int               m_oid;         // ROR 0x08
    const ChrPathPoint *m_path;        // ROR 0x0c
    const ChrPathPoint *m_point;       // ROR 0x10: the point walked to
    int                 m_actor;       // ROR 0x14: the achr's actor (ObjStat::m_actor)
    int                 m_count;       // ROR 0x18
    int                 m_index;       // ROR 0x1c: points reached
    int                 m_frames;      // ROR 0x20: frames left to the point
    float               m_speed;       // ROR 0x2c: the slide per frame (modes 0..3)
    int                 m_mode;        // ROR 0x30
    sceVu0FVECTOR       m_pos;         // ROR 0x40
    sceVu0FVECTOR       m_rot;         // ROR 0x50: [1] the heading
    sceVu0FVECTOR       m_vel;         // ROR 0x60: the slide (modes 0..3)
    StepFunc            m_stepFunc;    // ROR 0x70: GCC 2 {delta, index, pfn}
    CmnITask           *m_mtnTask;     // ROR 0x78: chrMoveFrames' chrMtnMove (modes 4..7)
    int                 m_motion;      // ROR 0x7c
    float               m_rate;        // ROR 0x80: the motion's speed
};
/* TskMoveSectaFindPath : CmnITask (vtable 0x2e4a10, EE 0x2c0 bytes; kind
 * 0xb): an achr walks the secta cells from the one it stands in to cell
 * no to (GamMan::findSectaPath over list chrIsOidType2(oid)), cell point
 * by cell point. mode < 1 slides it at speed a frame (stepSlide, or
 * stepSlideHit for an achr off the secta lists) while its walk motion
 * (param 0x1100 at param 0x1103 / 60) plays; mode > 0 lets a chrMtnMove
 * of the walk motion carry it (stepMotion). Each aim turns the achr like
 * TskMoveTypeGen::turnTo (params 0x1101 / 0x1102 within 0x1105 of
 * behind -> stepTurn). The status word *status (may be NULL) gets 4 when
 * there is no path, bit 2 when the last cell is reached. kind and maxCells
 * are not read. layout: host -- pointers and a member-function pointer;
 * EE offsets in the comments. */
struct TskMoveSectaFindPath : CmnITask
{
    typedef int (TskMoveSectaFindPath::*StepFunc)();

    TskMoveSectaFindPath(u_int oid, int to, int *status, int kind, int mode, int maxCells,
                         float speed, float rate);                       // ROR 0x001ec940
    ~TskMoveSectaFindPath() override {}                                  // ROR 0x001ecba4

    int  get_task_type() override { return 0xb; }                        // ROR 0x002902b8
    void init() override {}                                              // ROR 0x002902c0
    int  exec(int id, int time, int arg) override;                       // ROR 0x001ed100
    void term() override;                                                // ROR 0x001ecbd8

    /* Sets off toward cell m_index: the velocity and frames (or the
     * motion's frames), then the turn. */
    void aim();                                                          // ROR 0x001ed1d0
    /* aim's first half (the ROM's inline, also in stepTurn): the actor's
     * position and heading, the slide or chrMoveFrames; returns the
     * heading to the cell. */
    float setOff();
    int  stepTurn();                                                     // ROR 0x001ecbe0
    int  stepSlideHit();                                                 // ROR 0x001ecdd4
    int  stepSlide();                                                    // ROR 0x001ecebc
    int  stepMotion();                                                   // ROR 0x001ecf3c

    ACHRstat     *m_achr;          // ROR 0x004
    u_int         m_oid;           // ROR 0x008
    int           m_actor;         // ROR 0x00c: the achr's actor (ObjStat::m_actor)
    GamSectaPath  m_path;          // ROR 0x010: the cells (count at 0x20c)
    int           m_index;         // ROR 0x218: the cell walked to
    int           m_frames;        // ROR 0x21c: frames left to it
    int           m_half;          // ROR 0x220: stepMotion aims again below it
    int           m_from;          // ROR 0x224: the start cell no
    int           m_to;            // ROR 0x228: the goal cell no
    float         m_speed;         // ROR 0x230: the slide per frame (0 -> 1)
    sceVu0FVECTOR m_target;        // ROR 0x240: stepMotion's aim (never written by the ROM)
    sceVu0FVECTOR m_pos;           // ROR 0x250
    sceVu0FVECTOR m_rot;           // ROR 0x260: [1] the heading
    sceVu0FVECTOR m_vel;           // ROR 0x270: the slide (mode < 1)
    sceVu0FVECTOR m_lastPos;       // ROR 0x280: the last position placed
    int          *m_status;        // ROR 0x290: the owner's status word, NULL none
    CmnITask     *m_mtnTask;       // ROR 0x294: mode > 0, chrMoveFrames' chrMtnMove (a TskMtnMove)
    int           m_mode;          // ROR 0x298: > 0 walk by the motion
    int           m_motion;        // ROR 0x29c: param 0x1100, the walk motion
    float         m_rate;          // ROR 0x2a0: param 0x1103 / 60, its speed
    float         m_rotSpeed;      // ROR 0x2a4: param 0x1104, the turn speed
    u_int         m_flags;         // ROR 0x2a8: 1 hold while a turn task runs, 2 the motion held (never set by the ROM)
    int           m_turnRight;     // ROR 0x2ac: param 0x1101, -1 none
    int           m_turnLeft;      // ROR 0x2b0: param 0x1102, -1 none
    float         m_turnBack;      // ROR 0x2b4: param 0x1105, the angle from behind that plays them
    StepFunc      m_stepFunc;      // ROR 0x2b8: GCC 2 {delta, index, pfn}
};

/* TskMoveSecta : CmnITask (vtable 0x2e49d0, EE 0x80 bytes; kind 0xc):
 * native 211 cases 40..43 / 50 / 51 -- an achr walks to secta cells, each
 * leg a TskMoveSectaFindPath on the achr's m_task[3] that reports into
 * m_status (bit 1 the leg is to start, 2 it ended, 4 no way, 8 wait
 * m_left frames, then go again). By mode % 6 (6.. walk by the motion):
 * 0 the count cells of the list arg in turn, 1 round and round, 2 count
 * random ones, 3 random ones for ever, 4 to the cell of object arg (a
 * TskMoveTypeGen mode 3 / 11 when already in it), 5 the same, again every
 * max(count, 10) frames. Without a way it stands (motion 1) and tries
 * again in 60 frames. Modes 0 / 2 end with motion 1. layout: host --
 * pointers; EE offsets in the comments. */
struct TskMoveSecta : CmnITask
{
    TskMoveSecta(u_int oid, int mode, int count, intptr_t arg, int kind, float speed,
                 float rate);                                            // ROR 0x001ed998
    ~TskMoveSecta() override {}                                          // ROR 0x001edd08

    int  get_task_type() override { return 0xc; }                        // ROR 0x00290318
    void init() override {}                                              // ROR 0x00290320
    int  exec(int id, int time, int arg) override;                       // ROR 0x001edd90
    void term() override;                                                // ROR 0x001edd3c

    /* A new leg to cell no to on *m_taskId. */
    void startLeg(int to);

    ACHRstat  *m_achr;          // ROR 0x04
    u_int      m_oid;           // ROR 0x08
    int        m_status;        // ROR 0x0c: written by the legs (see above)
    int       *m_taskId;        // ROR 0x10: &m_achr->m_task[3], the leg's task id
    int        m_actor;         // ROR 0x14
    intptr_t   m_arg;           // ROR 0x18: modes 0..3 the cell list (const int *), 4 / 5 the object
    const int *m_cur;           // ROR 0x1c: modes 0 / 1, the list entry walked to
    float      m_speed;         // ROR 0x2c
    int        m_mode;          // ROR 0x30
    int        m_count;         // ROR 0x38: the list's length (mode 5: the frames between goes)
    int        m_left;          // ROR 0x3c: entries (frames) left
    int        m_kind;          // ROR 0x40: handed to the legs
    float      m_rate;          // ROR 0x44
};

/* TskAttach : CmnITask (vtable 0x2e4bd0, EE 0x40 bytes; kind 0): seqcall
 * light case 0x3c -- each frame light *value of type 2 (MagScene::
 * setLightParam, which 0) goes to the position of node name of oid's
 * actor. Never ends by itself. layout: host -- a pointer; EE offsets in
 * the comments. */
struct TskAttach : CmnITask
{
    TskAttach(u_int oid, const int *value, const char *name);            // ROR 0x001f5244
    ~TskAttach() override {}                                             // ROR 0x001f52a8

    void init() override {}                                              // ROR 0x002907d4
    int  exec(int id, int time, int arg) override;                       // ROR 0x001f52e4
    void term() override {}                                              // ROR 0x001f52dc

    u_int       m_oid;          // ROR 0x04
    int         m_actor;        // ROR 0x08: GamMan::getActorNo(m_oid)
    const char *m_name;         // ROR 0x0c: the node
    int         m_light;        // ROR 0x10: *value, the light index
};

/* TskSeqEvent : CmnITask (vtable 0x2e4c90, EE 0x80 bytes; kind 0):
 * GamMan::startAchrPlaceTask's task (ACHRstat::m_placeTask). Kind 0 reads
 * {frames, radius} from param: with a radius above 1 the achr of oid
 * grows its collision (m_colPrm kind 2, shape 4) to radius over frames
 * frames, then the task ends; any other kind or radius ends at once.
 * layout: host -- a pointer and a member-function pointer; EE offsets in
 * the comments. */
struct TskSeqEvent : CmnITask
{
    typedef int (TskSeqEvent::*StepFunc)();

    TskSeqEvent(u_int oid, int kind, const void *param);                 // ROR 0x001f47f0
    ~TskSeqEvent() override {}                                           // ROR 0x00290680

    void init() override {}                                              // ROR 0x002906b4
    int  exec(int id, int time, int arg) override;                       // ROR 0x001f48a8
    void term() override {}                                              // ROR 0x001f48a0

    int  stepNone();                                                     // ROR 0x001f4910
    int  stepGrow();                                                     // ROR 0x001f4918

    StepFunc  m_stepFunc;       // ROR 0x04: GCC 2 {delta, index, pfn}
    int       m_kind;           // ROR 0x0c
    ACHRstat *m_achr;           // ROR 0x1c
    int       m_frames;         // ROR 0x20: param word 0
    int       m_count;          // ROR 0x3c: frames run
    float     m_radius;         // ROR 0x48: param word 1
    float     m_t;              // ROR 0x4c: m_count / m_frames
};

/* TskMsgMan : CmnITask (vtable 0x2e4c10, EE 0x9c0 bytes; kind 0x10): a
 * message window -- GamMan::openMsg's task (m_msgTaskId). The constructor
 * opens a scene window sized by the eight window words param (p[0..1] the
 * position, p[3..4] the size, p[5] MsgManSet::setMessage's flag, p[6] the
 * window's life; it writes 0x8001 into the caller's p[2]) and shows "#C"
 * + text in it; fill picks the window fill (0: fill 1, else the msb fill).
 * exec, by mode: 1 ends after time frames, 2 when the text is out and
 * CROSS is pressed (after 10 frames), 3 the same or after time frames, 4
 * when the text is out. pause hides the window and stops the message
 * sprites 0..13. layout: host -- MsgManSet holds host pointers later; EE
 * offsets in the comments. */
struct TskMsgMan : CmnITask
{
    TskMsgMan(const char *text, int arg, int *param, int fill, int mode, int time); // ROR 0x001f4a24
    ~TskMsgMan() override;                                               // ROR 0x001f4c74

    int  get_task_type() override { return 0x10; }                      // ROR 0x00290774
    void pause(int on) override;                                         // ROR 0x001f4d84
    void init() override {}                                              // ROR 0x0029077c
    int  exec(int id, int time, int arg) override;                       // ROR 0x001f4f5c
    void term() override;                                                // ROR 0x001f4d14

    /* The window reopened from m_param showing text (no "#C"; m_textPtr
     * becomes text itself). a3 is not read. No caller in the ROM. */
    void setText(const char *text, int word, int a3, int fill);          // ROR 0x001f50b8

    char      m_text[0x200];    // ROR 0x004: "#C" + the text (strncpy 0x200 after the prefix)
    int       m_mode;           // ROR 0x204: exec's end rule 1..4; 0 after term
    int       m_word;           // ROR 0x208: setText's word (not read)
    int       m_time;           // ROR 0x210: frames (modes 1 and 3)
    int       m_window;         // ROR 0x214: the scene window (MagScene::openWindow)
    int       m_count;          // ROR 0x21c: frames run
    char     *m_textPtr;        // ROR 0x220: m_text
    MsgManSet m_msg;            // ROR 0x224
    int       m_param[8];       // ROR 0x99c: the window words as given (not read again)
};

/* TskSysFunc : CmnITask (vtable 0x2e4cd0, EE 0x80 bytes; kind 0x17):
 * seqcall case 0x22's map-name caption (GamMan::m_sysTaskId). Kinds 0..1
 * show the caption of map arg (S2dMes::setMapNumber, setMapNameFadeIn
 * over 30 frames), hold it 27 frames and fade it out over 15; term of
 * kind 1 hides a caption still shown. Other kinds set no step (the ROM
 * then calls a garbage pointer). a is not read. layout: host -- a
 * member-function pointer; EE offsets in the comments. */
struct TskSysFunc : CmnITask
{
    typedef int (TskSysFunc::*StepFunc)();

    TskSysFunc(int kind, int a, intptr_t arg);                           // ROR 0x001f4184
    ~TskSysFunc() override {}                                            // ROR 0x001f41ec

    int  get_task_type() override { return 0x17; }                      // ROR 0x00290620
    void init() override {}                                              // ROR 0x00290628
    int  exec(int id, int time, int arg) override;                       // ROR 0x001f425c
    void term() override;                                                // ROR 0x001f4220

    int  stepMapName();                                                  // ROR 0x001f42c4

    int      m_kind;            // ROR 0x04
    StepFunc m_stepFunc;        // ROR 0x08: GCC 2 {delta, index, pfn}
    int      m_state;           // ROR 0x10: 100 fade in, 0x65 hold then fade out
    int      m_count;           // ROR 0x14: frames left to hold
    int      m_map;             // ROR 0x18: arg, S2dMes::setMapNumber's
    int      m_inTime;          // ROR 0x1c: 30
    int      m_outTime;         // ROR 0x20: 15
    int      m_holdTime;        // ROR 0x24: 27
};

/* TskChrRot : CmnITask (vtable 0x2e4b90, EE 0x80 bytes; kind 0xe): an
 * achr's turn by angle (wrapped into -pi..pi) from the heading rotY
 * (>= 32769: the actor's). Without a turn motion (motion -1; an achr with
 * flag 0x80000 takes its param 0x1102 for a left turn, 0x1101 for a right
 * one) stepFrames adds angle / frames to the heading for frames frames
 * (frames < 1: at once) and hands it to the running motion task;
 * with one, the motion plays through chrMtnMove2 (speed / 60) and
 * stepMotion waits until the actor's heading is within 0.1 of the
 * target, then (restore set) changes back to motion 1, kills the motion
 * task and sets the heading. Started by chrRotStart on ACHRstat::m_task[1]
 * and by 0x14d65c and the chrRot* starters below. layout: host --
 * pointers and a member-function pointer; EE offsets in the comments. */
struct TskChrRot : CmnITask
{
    typedef int (TskChrRot::*StepFunc)();

    /* unused: the ROM's callers pass a sixth word (t1) it never reads. */
    TskChrRot(float angle, float rotY, float speed, u_int oid, int frames, int motion,
              int restore, int unused);                                  // ROR 0x001f0434
    ~TskChrRot() override {}                                             // ROR 0x001f0868

    int  get_task_type() override { return 0xe; }                        // ROR 0x002904f8
    void init() override {}                                              // ROR 0x00290500
    int  exec(int id, int time, int arg) override;                       // ROR 0x001f08a4
    void term() override;                                                // ROR 0x001f089c

    int  stepFrames();                                                   // ROR 0x001f090c
    int  stepMotion();                                                   // ROR 0x001f0b50
    /* 1 when the actor's heading is within 0.1 of m_angle (unwrapped;
     * no caller in the ROM). */
    int  isTurned();                                                     // ROR 0x001f0e14

    float     m_rot[4];         // ROR 0x10: (0, heading, 0, -) for setObjRot / setActorRotate
    u_int     m_oid;            // ROR 0x20
    int       m_actor;          // ROR 0x24: GamMan::getActorNo(m_oid)
    float     m_angle;          // ROR 0x30: the turn; the target heading with a motion
    float     m_step;           // ROR 0x34: angle / frames
    int       m_count;          // ROR 0x38: frames left - 1
    int       m_frames;         // ROR 0x3c
    int       m_restore;        // ROR 0x40: stepMotion ends with motion 1
    int       m_motion;         // ROR 0x44: the turn motion, -1 none
    float     m_speed;          // ROR 0x48: speed / 60
    CmnITask *m_motionTask;     // ROR 0x4c: chrMtnMove2's, then findTask(m_task[2])
    DogActor *m_dogActor;       // ROR 0x50: the scene actor m_actor
    ACHRstat *m_achr;           // ROR 0x54
    StepFunc  m_stepFunc;       // ROR 0x58: GCC 2 {delta, index, pfn}
};

/* ---- the chr* starters (natives 100 / 106 / 202.., GamMan) ---- */

/* An achr's turn task (TskChrRot) on ACHRstat::m_task[1]: chrRotStart
 * kills the running one and, for a non-zero angle, turns by angle over
 * frames; chrRotTo / chrRotToIn turn to the heading dir (the delta
 * wrapped into -pi..pi) at speed / over frames. chrRotFrames is
 * |angle / (speed * 0.05233)|. */
void  chrRotStart(float angle, u_int oid, int frames);                   // ROR 0x001f0e90
void  chrRotTo(float dir, float speed, u_int oid);                       // ROR 0x001f0f9c
void  chrRotToIn(float dir, u_int oid, int frames);                      // ROR 0x001f10a4
int   chrRotFrames(float angle, float speed);                            // ROR 0x001f6b8c
/* The TskChrRot starters the move tasks use, each replacing m_task[1]:
 * chrRotFromTo turns from the heading from to to at speed; chrRotStartFrom
 * is chrRotStart from the heading rotY; chrRotMotion / chrRotMotionBySign
 * (motionNeg for a negative angle) turn by angle with a turn motion played
 * at speed (restore: back to motion 1 at the end); chrRotToMotion turns to
 * the heading dir that way (rotY >= 32769: the object's heading). */
void  chrRotFromTo(float from, float to, float speed, u_int oid);        // ROR 0x001f11a0
void  chrRotStartFrom(float angle, float rotY, u_int oid, int frames);   // ROR 0x001f1298
void  chrRotMotion(float angle, float speed, float rotY, u_int oid, int motion,
                   int restore);                                         // ROR 0x001f13a8
void  chrRotMotionBySign(float angle, float speed, float rotY, u_int oid, int motionNeg,
                         int motionPos, int restore);                    // ROR 0x001f14cc
void  chrRotToMotion(float dir, float speed, float rotY, u_int oid, int motion,
                     int restore);                                       // ROR 0x001f1670
/* Starts (when *task is NULL) a chrMtnMove of motion at rate heading
 * dir and returns the frames its root move needs for dist: dist over the
 * move across 50 x the motion's length, times the length, x 25. no is
 * oid's actor. */
int   chrMoveFrames(float dir, float dist, float rate, u_int oid, int no, CmnITask **task,
                    int motion);                                         // ROR 0x001ee7f0
/* chrMoveFrames' other way round (no caller in the ROM): motion played
 * on actor no at 1/60, then (when *task is NULL) a chrMtnMove of it at
 * the rate that covers dist in frames frames (chrRateForFrames over the
 * root move across 50 x the motion's length; 0 when it does not move). */
void  chrMoveInFrames(float dir, float dist, u_int oid, int no, CmnITask **task, int motion,
                      int frames);                                       // ROR 0x001ee954
/* (ratio / frames) / 0.02778 (= x 36), 0 for frames 0. */
float chrRateForFrames(float ratio, int frames);                         // ROR 0x001f6c14
/* v.x += sinf(angle) * dist, v.z += cosf(angle) * dist. */
void  utlAddPolar(float angle, float dist, float *v);                    // ROR 0x001f6b28

/* The motion helpers of native 100. name != NULL picks the motion by
 * name. An achr's motion task lives in its slot m_task[2]:
 * chrKillMotionTask kills it; chrMtnMove / chrMtnMove2 replace it with a
 * new TskMtnMove / TskMtnMove2 (rotY >= 32769 keeps the heading). Other
 * objects get a task with no actor, which ends at once. */
void chrKillMotionTask(u_int oid);                                       // ROR 0x001f61ec
CmnITask *chrMtnMove(float speed, float rotY, u_int oid, int motion, int p1d8, u_int flags,
                     const char *name);                                  // ROR 0x001f59b4
CmnITask *chrMtnMove2(float speed, u_int oid, int motion, int p1d8, u_int flags,
                      const char *name);                                 // ROR 0x001f5bdc
/* chrChangeMotion to the motion DogActor::getMotionNoBySuffix(name) gives. */
int  chrChangeMotionBySuffix(float blend, float rotY, u_int oid, const char *name, int a2,
                             int a3);                                    // ROR 0x001f5d90
/* Changes the motion of oid's actor: an achr through a new TskMtnMove in
 * its m_task[2] (returned), other objects by MagScene::changeActorMotion
 * (NULL); rotY >= 32769 keeps the current heading. */
void *chrChangeMotion(u_int oid, int motion, int a2, int a3, float blend,
                      float rotY);                                        // ROR 0x001f5e48
/* MagScene::changeActorMotion on actor no keeping its heading; a speed
 * outside 0.0001..100 becomes 1/60. */
void chrSetMotion(float speed, int no, int motion, int p1d8, u_int flags, u_int oid,
                  const char *name);                                     // ROR 0x001f6308
/* Creates item id's object (its chr no and model from the system data),
 * placed at pos / rot when given; returns its oid. */
u_int chrCreateItemObj(short id, const float *pos, const float *rot);    // ROR 0x001f6578
/* The world position of oid's actor's node name into out (x, y, z, 1):
 * 0 without an actor or node, or while GamMan::m_flags2 has 0x3000. */
int  chrGetNodeWorldPos(u_int oid, float *out, const char *name);       // ROR 0x001f5558
/* 1 when object target's sphere (its radius) meets range around oid's
 * position (utlHitCheck). */
int  chrIsNear(float range, u_int oid, u_int target);                    // ROR 0x001f5350
/* 1 when object target (radius 50) is within range of node name of
 * oid's actor. */
int  chrIsNearNode(float range, u_int oid, u_int target, const char *name); // ROR 0x001f5410
/* An achr's limb part (0 neck, 1 / 2 right / left upper arm, 3 / 4
 * right / left thigh) back: clears its bit in m_attr[0] and sets the node's
 * DogActor::setNodeScale to 1.0. 0 when the bit was clear; a part above 4
 * only calls setNodeScale(1.0, -1). */
int  chrRestoreLimb(u_int oid, u_int part);                              // ROR 0x001f5680
/* chrMtnMove2 for an achr only (NULL otherwise, and no name): its m_task[2]
 * is killed (the slot not reset) and replaced by a new TskMtnMove2. */
CmnITask *chrMtnMove2Achr(float speed, u_int oid, int motion, int p1d8, u_int flags); // ROR 0x001f603c
/* An achr's running TskMtnMove2 gets step (no caller in the ROM). */
void chrSetMtnMove2Step(float step, u_int oid);                          // ROR 0x001f616c
/* chrSetMotion on oid's actor. */
void chrSetMotionByOid(float speed, u_int oid, int motion, int p1d8, u_int flags,
                       const char *name);                                // ROR 0x001f627c
/* A new object no (an achr below 4, an fchr below 10) of scene model
 * (0xffff: none, loaded by MagSystem::loadModel), placed at pos / rot when
 * given; returns its oid. No caller in the ROM. */
u_int chrCreateChr(short model, int no, const float *pos, const float *rot); // ROR 0x001f6694
/* A model-less fchr of type 6 with params 0x32 (word) and 0x33 (value),
 * placed at pos / rot when given; returns its oid. No caller in the ROM. */
u_int chrCreateFchr6(float value, const float *pos, const float *rot, short word); // ROR 0x001f67a0
/* Achr oid steps one unit away from object obj (a random diagonal when
 * their directions cancel) unless testHitAt finds something ten ahead or
 * the cell refuses; each obj once per frame (g_frameReq). No caller found. */
void chrStepAwayFrom(u_int obj, u_int oid);                              // ROR 0x001f68bc
/* gamOidType(oid) == 2 (TskMtnMove's test, inlined there). */
int  chrIsOidType2(u_int oid);                                           // ROR 0x001f6878

/* The objects chrStepAwayFrom stepped away from this frame (at most 32; GamMan::update
 * clears them each frame). */
#define GAM_FRAME_REQ_MAX 32
extern int   g_frameReqNum;                     /* sdata 0x2d8f68 */
extern u_int g_frameReq[GAM_FRAME_REQ_MAX];     /* data 0x2d8f70 */
/* Empties the list (the ROM clears only its first 8 words). */
void gamClearFrameReq(void);                                             // ROR 0x001f688c

#endif /* _TASK_TSK_CHR_H */
