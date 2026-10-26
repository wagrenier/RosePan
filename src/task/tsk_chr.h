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
#include "task/tsk_man.h"

struct ACHRstat;
struct DogActor;

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

    u_int     m_flags;          // ROR 0x04: bit 0 stops the moving (writer elsewhere), bit 1 paused
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

/* TskFrameFunc : CmnITask (vtable 0x2e4d50, EE 0x80 bytes): kind < 0x17
 * picks its step function (table 0x2fe420); arg is a value or a pointer
 * to the caller's words, frames its length. layout: host -- no members. */
struct TskFrameFunc : CmnITask
{
    TskFrameFunc(int kind, intptr_t arg, int frames);                    // ROR 0x001f184c
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

/* The achr movement tasks (class names from their vtables): oid the achr,
 * mode the way it moves, rate the step per frame (1/60 by default).
 * Natives 202 / 211 start them on the achr's m_task[0]. layout: host --
 * no members. */
struct TskMoveTypeGen : CmnITask           // EE 0xc0, vtable 0x2e4ad0
{
    TskMoveTypeGen(u_int oid, int mode, intptr_t arg, int kind, float frames, float rate); // ROR 0x001e9a3c
};
/* layout: host */
struct TskMoveTypeRound : CmnITask         // EE 0x80
{
    TskMoveTypeRound(u_int oid, int mode, intptr_t arg, int kind, float a, float b, float rate); // ROR 0x001eb63c
};
/* layout: host */
struct TskMovePointPath : CmnITask         // EE 0xc0
{
    TskMovePointPath(u_int oid, int mode, intptr_t path, int arg, int kind, float a, float rate); // ROR 0x001ec344
};
/* layout: host */
struct TskMoveSectaFindPath : CmnITask     // EE 0x2c0
{
    TskMoveSectaFindPath(u_int oid, intptr_t arg, int a3, int kind, int shift, int t2, float a,
                         float rate);                                    // ROR 0x001ec940
};
/* layout: host */
struct TskMoveSecta : CmnITask             // EE 0x80
{
    TskMoveSecta(u_int oid, int mode, intptr_t arg, int a3, int kind, float a, float rate); // ROR 0x001ed998
};

/* TskAttach : CmnITask (vtable 0x2e4bd0, EE 0x40 bytes): +4 the oid, +8
 * its actor, +0xc arg, +0x10 *value. layout: host -- no members. */
struct TskAttach : CmnITask
{
    TskAttach(u_int oid, const int *value, int arg);                     // ROR 0x001f5244
};

/* TskSeqEvent : CmnITask (vtable 0x2e4c90, EE 0x80 bytes): kind 0 reads
 * {frames, radius} from param and the achr of oid; GamMan::
 * startAchrPlaceTask's task (ACHRstat::m_placeTask). layout: host. */
struct TskSeqEvent : CmnITask
{
    TskSeqEvent(u_int oid, int kind, const void *param);                 // ROR 0x001f47f0

    float m_radius;             // ROR 0x48: param word 1
};

/* TskMsgMan : CmnITask (vtable 0x2e4c10, EE 0x9c0 bytes): a message
 * window on text (two MsgMan at +0x224) with the eight window words param;
 * GamMan::openMsg's task (m_msgTaskId). layout: host -- no members. */
struct TskMsgMan : CmnITask
{
    TskMsgMan(const char *text, int arg, const int *param, int a4, int mode, int time); // ROR 0x001f4a24
};

/* TskSysFunc : CmnITask (vtable 0x2e4cd0, EE 0x80 bytes): kinds 0..1 set
 * the defaults 100, 0x1e, 0xf, 0x1b and keep arg. layout: host. */
struct TskSysFunc : CmnITask
{
    TskSysFunc(int kind, int a, intptr_t arg);                           // ROR 0x001f4184
};

/* ---- the chr* starters (natives 100 / 106 / 202.., GamMan) ---- */

/* An achr's turn task (TskChrRot, ctor 0x1f0434) on ACHRstat::m_task[1]:
 * chrRotStart kills the running one and, for a non-zero angle, turns by
 * angle over frames; chrRotTo / chrRotToIn turn to the heading dir (the
 * delta wrapped into -pi..pi) at speed / over frames. chrRotFrames is
 * |angle / (speed * 0.05233)|. */
void  chrRotStart(float angle, u_int oid, int frames);                   // ROR 0x001f0e90
void  chrRotTo(float dir, float speed, u_int oid);                       // ROR 0x001f0f9c
void  chrRotToIn(float dir, u_int oid, int frames);                      // ROR 0x001f10a4
int   chrRotFrames(float angle, float speed);                            // ROR 0x001f6b8c
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
/* gamOidType(oid) == 2 (TskMtnMove's test, inlined there). */
int  chrIsOidType2(u_int oid);                                           // ROR 0x001f6878

/* The objects 0x1f68bc pushed aside this frame (at most 32; GamMan::update
 * clears them each frame). */
#define GAM_FRAME_REQ_MAX 32
extern int   g_frameReqNum;                     /* sdata 0x2d8f68 */
extern u_int g_frameReq[GAM_FRAME_REQ_MAX];     /* data 0x2d8f70 */
/* Empties the list (the ROM clears only its first 8 words). */
void gamClearFrameReq(void);                                             // ROR 0x001f688c

#endif /* _TASK_TSK_CHR_H */
