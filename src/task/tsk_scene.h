/* ==========================================================================
 *  task/tsk_scene.h
 *
 *  task/tsk_scene.cpp (pinned 0x1f6c64..0x1fc4f0; RoR's source string
 *  "task/tsk_scene.cpp" anchors 0x1f8034 / 0x1f81a8, ScenePlay's vtable
 *  slots span the run on both sides): the event scene player and the
 *  smaller script tasks next to it.
 *  - ScenePlay (vtable 0x2e4dd0) with ScenePlaySingle / DisplayMessage /
 *    SeManager / VoiceManager, PLScenePlay (0x2e4d90): event scenes,
 *    data/evt/%02d/scene/<name> (scenePlayStart, scenePlayStartSimple);
 *  - TskSnd (0x2e4e70): sound fades and delayed sounds;
 *  - TskAttack (0x2e4eb0), TskDamage (0x2e4ef0), scnChrRequest;
 *  - TskCamera (0x2e4f30): the scripted camera of native 016 case 50;
 *  - gamSystemDataLoaded / gamStage2Loaded, GamMan's load hooks.
 *  Every task class derives from CmnITask (task/tsk_man.h) with the
 *  six-slot vtable: get_task_type, pause, init, exec, term, the destructor.
 *  Written so far: TskSnd, TskCamera, the two hooks.
 * ======================================================================== */
#ifndef _TASK_TSK_SCENE_H
#define _TASK_TSK_SCENE_H

#include <stddef.h>
#include <stdint.h>

#include "sdk/eetypes.h"
#include "sdk/libvu0.h"
#include "task/tsk_man.h"

/* The parameter block TskSnd's constructor copies (0x40 bytes on the EE).
 * A fade (GamSndMan::funcSnd / funcSE / funcStr): m_snd the sound, m_id
 * its SndMan id, m_volL / m_volR the target, m_frames its length, m_curL /
 * m_curR the volume at the start. layout: host -- m_name is a pointer. */
struct TskSndParam
{
    int         m_snd;          // ROR 0x00: the SE id / stream number
    int         m_volL;         // ROR 0x04: the target volume
    int         m_volR;         // ROR 0x08
    int         m_id;           // ROR 0x0c: the SndMan id
    int         m_attr;         // ROR 0x10: kind 5 (GamSndMan::playSE's delayed SE): the attribute
    int         m_frames;       // ROR 0x14
    float       m_pitch;        // ROR 0x18: kind 5: the pitch
    const char *m_name;         // ROR 0x1c: kind 7 copies the name (0x1b chars)
    int         m_curL;         // ROR 0x20: the volume at the start
    int         m_curR;         // ROR 0x24
    int         m_arg;          // ROR 0x28: kind 7: playStream's arg (non-zero loops); kind 1: 0x17 instead of 1 at volume 0
    int         m_mode;         // ROR 0x2c: kind 7: playStream's mode
    int         m_id2;          // ROR 0x30: kind 7: the stream's new SndMan id
    int         _34[3];
};
static_assert(offsetof(TskSndParam, m_pitch) == 0x18, "TskSndParam m_pitch (the offsets before the pointer)");

/* TskSnd : CmnITask (vtable 0x2e4e70, EE 0xc0 bytes; kind 0x12): a sound
 * job the GamSndMan functions start. The constructor's kind picks the
 * step (+0x2c) the exec step (stepFade) calls once a frame with the
 * volume blended from m_curL / m_curR to m_volL / m_volR over m_frames:
 * 0 an SE's track volume, 1 a stream's, 2 / 3 / 4 the master / SE /
 * stream master volume (funcSnd 0xd / 0xe / 0xf), 5 play an SE (a
 * delayed playSE), 6 fade a playing SE out after m_frames, 7 play the
 * stream m_name once the old one has stopped. A step returns non-zero
 * to end the task. layout: host -- member-function pointers; EE offsets
 * in the comments. */
struct TskSnd : CmnITask
{
    typedef int (TskSnd::*ExecFunc)();
    typedef int (TskSnd::*StepFunc)(int volL, int volR);

    TskSnd(int kind, const TskSndParam *param);                          // ROR 0x001f8fc0
    ~TskSnd() override {}                                                // ROR 0x00290a34

    int  get_task_type() override { return 0x12; }                       // ROR 0x00290a2c
    void init() override {}                                              // ROR 0x00290a68
    int  exec(int id, int time, int arg) override;                       // ROR 0x001f91dc
    void term() override;                                                // ROR 0x001f9244

    /* m_frame / m_frames, 0 without frames. */
    float getRate();                                                     // ROR 0x001f9294
    int  stepFade();                                                     // ROR 0x001f92c8
    int  stepSEVol(int volL, int volR);                                  // ROR 0x001f93d8
    int  stepStrVol(int volL, int volR);                                 // ROR 0x001f9484
    int  stepMstVol(int volL, int volR);                                 // ROR 0x001f9550
    int  stepMstVolSe(int volL, int volR);                               // ROR 0x001f95a8
    int  stepMstVolStr(int volL, int volR);                              // ROR 0x001f9600
    /* The fade's target volume (m_volL / m_volR); returns 0. */
    int  getVolume(int *volL, int *volR);                                // ROR 0x001f9658
    int  stepPlaySE(int volL, int volR);                                 // ROR 0x001f9670
    int  stepPlayStream(int volL, int volR);                             // ROR 0x001f96c0
    int  stepSEFadeOut(int volL, int volR);                              // ROR 0x001f974c
    int  stepEnd(int volL, int volR);                                    // ROR 0x001f97c0

    char        m_name[0x20];   // ROR 0x04: kind 7: the stream's name (strncpy 0x1b)
    ExecFunc    m_exec;         // ROR 0x24: GCC 2 {delta, index, pfn}; always stepFade
    StepFunc    m_step;         // ROR 0x2c: the kind's step
    int         m_kind;         // ROR 0x34
    int         m_frames;       // ROR 0x38: m_param.m_frames
    int         m_frame;        // ROR 0x3c: frames run
    int         m_state;        // ROR 0x40: 100 runs the step, 101 ends, others wait (nothing here writes 101; unverified)
    TskSndParam m_param;        // ROR 0x80
};

/* TskCamera : CmnITask (vtable 0x2e4f30, EE 0xc0; kind 0): the scripted
 * camera native 016 case 50 starts on GamMan's m_camTaskId. Kind 0 looks
 * around with the left stick: the scene camera's rotation is m_center
 * plus an angle the stick moves within m_min..m_max (m_speed a frame) and
 * a small random drift, both smoothed. Kind 1 sets up once: object 0
 * (and object 1 too when the third word is set) is turned to face the
 * other and the camera placed over object 0's shoulder, pulled in when
 * the scene is in the way; it then holds. layout: host -- a
 * member-function pointer; EE offsets in the comments. */
struct TskCamera : CmnITask
{
    typedef int (TskCamera::*StepFunc)();

    /* param: the script's words (kind 0: [1..3] centre, [4..6] min,
     * [7..9] max, [10] speed; kind 1: [0] / [1] the objects, [2] turn
     * object 1 too). */
    TskCamera(intptr_t kind, intptr_t param);                            // ROR 0x001fbc0c
    ~TskCamera() override {}                                             // ROR 0x00290be8

    void init() override;                                                // ROR 0x001fc164
    int  exec(int id, int time, int arg) override;                       // ROR 0x001fc174
    void term() override;                                                // ROR 0x001fc16c

    void setupPad(const int *param);                                     // ROR 0x001fbc6c
    void setupFace(const int *param);                                    // ROR 0x001fbd2c
    int  execPad();                                                      // ROR 0x001fc210
    int  execFace();                                                     // ROR 0x001fc46c
    int  stepNone();                                                     // ROR 0x001fc474

    StepFunc      m_step;          // ROR 0x04: GCC 2 {delta, index, pfn}; exec's step for kinds above 1 (setupPad stores stepNone)
    int           m_0c;            // ROR 0x0c: 0 (not read)
    float         m_speed;         // ROR 0x10: the stick's angle a frame
    int           m_kind;          // ROR 0x14
    sceVu0FVECTOR m_center;        // ROR 0x20: the base rotation (w 0)
    sceVu0FVECTOR m_min;           // ROR 0x30: the angle's lower limits (x, y)
    sceVu0FVECTOR m_max;           // ROR 0x40: the upper limits
    sceVu0FVECTOR m_angle;         // ROR 0x50: the stick's angle
    sceVu0FVECTOR m_angleSmooth;   // ROR 0x60: m_angle smoothed (0.9 / 0.1)
    sceVu0FVECTOR m_drift;         // ROR 0x70: the random drift
    sceVu0FVECTOR m_driftSmooth;   // ROR 0x80: m_drift smoothed (0.85 / 0.1)
};

/* The per-character request (jump table data 0x2fed40 over chr 0..10;
 * chr 11 takes it from object chrOid's kind: 4 for kind 0, 3 for kind 1,
 * the system data field 0x72 for kind 2). Many callers; TskMovePhysic
 * passes chr 4 (the PC) or 3 (the dog) with word 0x15. Role of the words
 * unverified. */
void scnChrRequest(int chr, u_int oid, int a2, int a3, int word, u_int chrOid, int a6); // ROR 0x001fac60

/* Event scenes: data/evt/%02d/scene/<name> from the stage pfh, played by
 * a ScenePlay task (vtable 0x2e4dd0, 0x580 bytes). scenePlayStart also
 * reads the map / CHARA / THREAD sections and halts the game; wait is its
 * third argument. */
int  scenePlayStart(const char *name, intptr_t arg, int wait);           // ROR 0x001f83f4
void scenePlayStartSimple(const char *name);                             // ROR 0x001f8cec

/* GamMan::loadSystemData's last call: empty in the ROM. */
void gamSystemDataLoaded(void);                                          // ROR 0x001f8fb8
/* The end of a stage load (GamMan::execStageReq change kind 2, initStage2,
 * native 027, ScenePlay): the free-memory debug print. */
void gamStage2Loaded(void);                                              // ROR 0x001fbbd0

#endif /* _TASK_TSK_SCENE_H */
