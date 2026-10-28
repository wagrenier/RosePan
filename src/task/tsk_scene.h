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

struct ACHRstat;

/* TskAttack : CmnITask (vtable 0x2e4eb0, EE 0x80; kind 0x15): an achr's
 * attack, started by scnAttackStart on the achr's task slot m_task[3].
 * setup picks the motion and the step from the parameter words' kind
 * (param[2]): 0x12..0x14, 0x15, 0x26..0x28, 0x2e, 0x2f, 0x30; other kinds
 * set no step and the task idles. The steps live in the file after
 * task/naga_chunk.cpp (unassigned@001fd424) and call scnChrRequest.
 * layout: host -- a member-function pointer; EE offsets in the comments. */
struct TskAttack : CmnITask
{
    typedef int (TskAttack::*StepFunc)();

    /* param: the caller's words ([0], [2] the kind, [3] / [4] kind 0x15,
     * [5] the target oid (0x13 / 0x14), [6] the motion and [10] a float
     * (0x2e..0x30)). */
    TskAttack(u_int oid, const int *param);                              // ROR 0x001f985c
    ~TskAttack() override {}                                             // ROR 0x00290ac0

    int  get_task_type() override { return 0x15; }                       // ROR 0x00290afc
    void init() override {}                                              // ROR 0x00290af4
    int  exec(int id, int time, int arg) override;                       // ROR 0x001f9cf0
    void term() override;                                                // ROR 0x001f9ce8

    void setup(u_int oid, const int *param);                             // ROR 0x001f98c0
    int  stepKind12();                                                   // ROR 0x001fd6bc
    int  stepKind15();                                                   // ROR 0x001fd9bc
    int  stepKind26();                                                   // ROR 0x001ffce0
    int  stepKind2e();                                                   // ROR 0x001ff494
    int  stepKind2f();                                                   // ROR 0x001ff664
    int  stepKind30();                                                   // ROR 0x001ff830

    StepFunc  m_step;           // ROR 0x04: GCC 2 {delta, index, pfn}; cleared by the constructor
    u_int     m_oid;            // ROR 0x10
    int       m_actor;          // ROR 0x14: the achr's scene actor
    ACHRstat *m_achr;           // ROR 0x18
    int       m_1c;             // ROR 0x1c: 0
    int       m_20;             // ROR 0x20: 0
    int       m_motion;         // ROR 0x24: the attack's motion number
    int       m_34;             // ROR 0x34: param[0]
    int       m_kind;           // ROR 0x3c: param[2]
    float     m_40;             // ROR 0x40: 0x15: 70 / 50 / 20 by system field 0x1b; 0x26..0x28: 110 / 110 / 120; 0x2e..0x30: param[10]
    int       m_44;             // ROR 0x44: a frame of the motion (role unverified)
    int       m_48;             // ROR 0x48: a later frame (role unverified)
    int       m_4c;             // ROR 0x4c: 0x15: object param 0x67
    u_int     m_target;         // ROR 0x50: 0x13 / 0x14: param[5], the object whose actor plays motion 9 / 10
    int       m_54;             // ROR 0x54: 0x15: param[3]
    int       m_58;             // ROR 0x58: 0x15: param[4]
};

/* scnDamageStart's parameter words (EE stack, scnChrRequest's a2 / word
 * / chr). */
struct ScnDamageParam
{
    int m_a2;                   // ROR 0x00: TskDamage::setup keeps it at +0x3c
    int m_word;                 // ROR 0x04
    int m_chr;                  // ROR 0x08: scnChrRequest's resolved chr
};
static_assert(sizeof(ScnDamageParam) == 0xc, "ScnDamageParam is 3 words");

/* TskDamage : CmnITask (vtable 0x2e4ef0, EE 0x80; kind 0x16): an achr
 * hit by attacker's attack, started by scnDamageStart on the achr's task
 * slot m_task[3]. setup takes the damage off the achr's LP (param 0x1b;
 * kill when it reaches them), tells the enemy (EmyManager::notifyEmy),
 * picks the reaction step by the chr word (param->m_chr; the steps live in
 * unassigned@001fd424 after task/naga_chunk.cpp) and plays a hurt SE.
 * layout: host -- a member-function pointer; EE offsets in the comments. */
struct TskDamage : CmnITask
{
    typedef int (TskDamage::*StepFunc)();

    TskDamage(u_int oid, u_int attacker, const ScnDamageParam *param, int kind) // ROR 0x001f9e38
    {
        setup(oid, attacker, param, kind);
    }
    ~TskDamage() override {}                                             // ROR 0x00290b54

    int  get_task_type() override { return 0x16; }                       // ROR 0x00290b90
    void init() override {}                                              // ROR 0x00290b88
    int  exec(int id, int time, int arg) override;                       // ROR 0x001fabf8
    void term() override;                                                // ROR 0x001fabd0

    void setup(u_int oid, u_int attacker, const ScnDamageParam *param, int kind); // ROR 0x001f9e58
    /* The reaction's frames for chr 0 / 1 / 8 by the motion the actor was
     * playing (m_motion), 0 for the rest; rewrites *front. */
    int  getReactFrames(int chr, int *front);                            // ROR 0x001fa768
    /* The hit took the last LP: the achr is stopped and LP 0 stored. */
    void kill();                                                         // ROR 0x001fa9c4
    /* Sets a random clear bit of the achr's m_attr[0] low five (into
     * m_attrBit); 1 when bits 7, 0x1e or 0x19 are then all set.
     * Unreferenced in the ROM. */
    int  pickAttrBit();                                                  // ROR 0x001faa70
    /* Every step's end test: with step flag 8 clears achr flag 0x200 and
     * resumes the enemy (setEmyMode6), or for a killed one (frames >=
     * 10000) stops it and kills the achr's tasks; 1 then, else 0. */
    int  endReact();                                                     // ROR 0x001fbb04
    int  stepKind0();                                                    // ROR 0x001fe160
    int  stepKind1();                                                    // ROR 0x001ff2e0
    int  stepKind2();                                                    // ROR 0x001ffbc0
    int  stepKind5();                                                    // ROR 0x00200444
    int  stepKind6();                                                    // ROR 0x002005b8
    int  stepKind7();                                                    // ROR 0x001fffc0
    int  stepKind8();                                                    // ROR 0x0020014c
    int  stepKind9();                                                    // ROR 0x001ff9fc
    int  stepKindA();                                                    // ROR 0x002002cc

    StepFunc  m_step = NULL;    // ROR 0x04: GCC 2 {delta, index, pfn}; PC: NULL for chr 3 / 4 (the ROM leaves it to the heap)
    u_int     m_flags;          // ROR 0x0c: 4 set by setup (7: 8 instead when the enemy word 0x238 is set), 0x100 killed
    u_int     m_oid;            // ROR 0x10
    int       m_actor;          // ROR 0x14: the achr's scene actor
    ACHRstat *m_achr = NULL;    // ROR 0x18
    int       m_1c;             // ROR 0x1c: 0
    int       m_frames;         // ROR 0x20: getReactFrames, else 0 / 10 / 20 / 30; +10000 by kill
    int       m_motion;         // ROR 0x24: the actor's motion when hit
    int       m_motion0;        // ROR 0x28: m_motion
    u_char    _2c[0x3c - 0x2c]; // ROR 0x2c
    int       m_damage;         // ROR 0x3c: param->m_a2
    int       m_40;             // ROR 0x40: 2 for kind 0 / 1 (or the global word 0x173 set), else 1
    u_int     m_attrBit;        // ROR 0x44: pickAttrBit's bit
    u_short   m_lp;             // ROR 0x48: the achr's param 0x1b
    u_short   m_emy1d8;         // ROR 0x4a: EmyManager::getEmyWord1d8 (never 0: the ROM traps)
    short     m_kind;           // ROR 0x4c: scnDamageStart's kind
    u_char    _4e[0x80 - 0x4e]; // ROR 0x4e
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

/* ──────────────────────────────────────────────────────────────────────
 *  The event scene player. An event pack file (data/evt/.. inside the
 *  stage's event pack, GamMan::m_evtPack) is a pfh: file 0 a NagaChunk
 *  table -- "map" (0xcccc once its motion names are converted), "CHARA"
 *  (a "THREAD" per actor: "name" the model, "data" the records, "STR" the
 *  motion names), "frame" (the length), "MESSAGE" ("strings") --, file 1
 *  an effect (kind 0xd), file 2 the focus track.
 * ────────────────────────────────────────────────────────────────────── */

struct ScenePlay;

/* One timed record of a thread (0x48 bytes, EE layout: it comes from the
 * pack). The thread runs on while the next record's kind is not 0. */
struct ScenePlayRecord
{
    int   m_frame;              // ROR 0x00: the thread frame it runs at
    int   m_kind;               // ROR 0x04: 1 place, 2 motion, 3 SE, 4 voice, 5 message, 6 fade in, 7 fade out, 8 show, 9 hide
    int   m_08;                 // ROR 0x08: 1 x (float), 2 the motion number (motNameConvert), 5 the line, 6 / 7 the fade time
    int   m_0c;                 // ROR 0x0c: 1 y (float), 5 the frames
    int   m_10;                 // ROR 0x10: 1 z (float), 2 the speed in frames a second (float), 3 / 4 zero with a volume: panned, else at the actor
    int   m_14;                 // ROR 0x14: 1 the Y rotation (float), 2 DogActor::setParam1d8, 3 / 4 the volume
    int   m_pan;                // ROR 0x18: 3 / 4 the pan (-63..63)
    int   m_frames;             // ROR 0x1c: 3 the SE's frames
    int   _20[2];
    char  m_name[0x20];         // ROR 0x28: 3 / 4 the SE / stream name
};
static_assert(sizeof(ScenePlayRecord) == 0x48, "ScenePlayRecord is 0x48 bytes");
static_assert(offsetof(ScenePlayRecord, m_frames) == 0x1c, "m_frames");
static_assert(offsetof(ScenePlayRecord, m_name) == 0x28, "m_name");

/* The scene's message lines (EE 0x90 at ScenePlay +0x1b0; vtable
 * 0x2e4e58, vptr at +0x8c). layout: host -- pointers and a vptr. */
struct DisplayMessage
{
    DisplayMessage()                                                     // ROR 0x001f6c64
        : m_count(0), m_cur(-1), m_timer(0) {}
    virtual ~DisplayMessage() {}                                         // ROR 0x001f6c84

    void tick();                                                         // ROR 0x001f6cb8
    void reset();                                                        // ROR 0x001f6cec
    void addLine(char *line);                                            // ROR 0x001f6d20
    /* Every "strings" chunk of the MESSAGE table becomes a line. */
    void load(void *data);                                               // ROR 0x001f6d8c
    /* Line no in the message window for frames frames; -1 closes it. */
    void show(int no, int frames);                                       // ROR 0x001f6dfc

    char *m_line[0x20];         // ROR 0x00
    int   m_count;              // ROR 0x80
    int   m_cur;                // ROR 0x84: the line shown, -1 none
    int   m_timer;              // ROR 0x88: frames left, 0 = until changed
};

/* A scene's SEs (EE 0x308 at ScenePlay +0x240; vtable 0x2e4e40, vptr at
 * +0x304): stopped when their frames run out or the scene ends.
 * layout: host -- a vptr. */
struct SeManager
{
    struct Entry
    {
        u_int m_se;             // ROR 0x00
        int   m_actor;          // ROR 0x04: play3d's actor, 0 for play
        int   m_frames;         // ROR 0x08: frames left, 0 = until the scene ends
    };

    SeManager() : m_count(0) {}                                          // ROR 0x001f6edc
    virtual ~SeManager() { stopAll(); }                                  // ROR 0x001f6ef0

    /* At actor's Bip01 node (its position without one). */
    void play3d(u_int se, int actor, int frames);                        // ROR 0x001f6f4c
    /* At vol, panned by pan (-63 left .. 63 right). */
    void play(u_int se, int vol, int pan, int frames);                   // ROR 0x001f70c8
    void tick();                                                         // ROR 0x001f71b8
    void stopAll();                                                      // ROR 0x001f723c
    void stop(int n);                                                    // ROR 0x001f728c
    void stopSE(u_int se, int actor);                                    // ROR 0x001f7338

    Entry m_entry[0x40];        // ROR 0x000
    int   m_count;              // ROR 0x300
};
static_assert(sizeof(SeManager::Entry) == 0xc, "SeManager::Entry is 12 bytes");
static_assert(offsetof(SeManager::Entry, m_frames) == 0x8, "m_frames");

/* A scene's voice stream (EE 0x10 at ScenePlay +0x548; vtable 0x2e4e28,
 * vptr at +0xc). layout: host -- a vptr. */
struct VoiceManager
{
    VoiceManager() : m_frames(0) {}                                      // ROR 0x001f73d4
    virtual ~VoiceManager() { stop(); }                                  // ROR 0x001f73e8

    void play3d(int str, int actor, int frames);                         // ROR 0x001f7444
    void play(int str, int vol, int pan, int frames);                    // ROR 0x001f7570
    void tick();                                                         // ROR 0x001f7610
    /* Empty in the ROM. */
    void stop() {}                                                       // ROR 0x001f7644

    int m_str;                  // ROR 0x0
    int m_actor;                // ROR 0x4
    int m_frames;               // ROR 0x8: frames left, 0 = no end
};

/* One actor's thread (EE 0xc0, operator new; vtable 0x2e4e10, vptr at
 * +0x9c). layout: host -- pointers and a vptr. */
struct ScenePlaySingle
{
    ScenePlaySingle(int actor, ScenePlayRecord *data);                   // ROR 0x001f764c
    virtual ~ScenePlaySingle() {}                                        // ROR 0x001f767c

    int  exec();                                                         // ROR 0x001f76b0
    /* The frame +1 and every record due; whether the thread still runs. */
    bool step();                                                         // ROR 0x001f76e0
    /* The current record (skip: SEs, voices and messages left out); true
     * while the next one is set. */
    bool run(int skip);                                                  // ROR 0x001f7788
    /* The motion's root move this frame. */
    void move();                                                         // ROR 0x001f7c0c

    ScenePlayRecord *m_data;    // ROR 0x00
    int              m_active;  // ROR 0x04
    int              m_actor;   // ROR 0x08
    int              m_frame;   // ROR 0x0c
    int              m_index;   // ROR 0x10: the current record
    u_int            m_oid;     // ROR 0x14: the scripted object, -1 for a model the scene loaded
    ScenePlay       *m_owner;   // ROR 0x18
};

/* ScenePlay : CmnITask (vtable 0x2e4dd0, EE 0x580; kind 0xf): plays an
 * event pack's threads for its "frame" count of frames. layout: host --
 * pointers and members with vptrs; EE offsets in the comments. */
struct ScenePlay : CmnITask
{
    ScenePlay(int arg);                                                  // ROR 0x001f7d54
    ~ScenePlay() override { term(); }                                    // ROR 0x001f7dc8

    int  get_task_type() override { return 0xf; }                        // ROR 0x0029093c
    void init() override {}                                              // ROR 0x001f7e48
    int  exec(int id, int time, int arg) override;                       // ROR 0x001f82e4
    void term() override;                                                // ROR 0x001f7e50

    void addSingle(int actor, ScenePlayRecord *data, u_int oid);         // ROR 0x001f7f68
    void setFrames(int frames);                                          // ROR 0x001f8028
    void setName(const char *name);                                      // ROR 0x001f836c
    /* Unreferenced: the event's camera track data/evt/<event>/camera/<name>
     * (0x20000 bytes at most) read into m_cameraData and created as an
     * effect of kind 0xd into m_eff; NULL or "" only frees the old one. */
    void loadCamera(const char *name);                                   // ROR 0x001f8034
    /* Unreferenced: the focus track data/evt/<event>/focus/<name> read
     * whole into m_focusData and handed to MagScene::setFocusData. */
    void loadFocus(const char *name);                                    // ROR 0x001f81a8
    void add_imd_load_chr_id(int id) { m_actor[m_numActor++] = id; }     // ROR 0x00290944
    void set_camera_eff_id(int id) { m_eff = id; }                       // ROR 0x00290964

    ScenePlaySingle *m_single[0xb];   // ROR 0x04
    int              m_numSingle;     // ROR 0x30
    int              m_frame;         // ROR 0x34: frames left
    int              m_frames;        // ROR 0x38
    int              m_3c;            // ROR 0x3c: the constructor's arg (0 from scenePlayStart)
    int              m_eff;           // ROR 0x40: the pack's effect (EffMan), -1 none
    void            *m_cameraData;    // ROR 0x44: loadCamera's buffer
    char             m_cameraPath[0x40]; // ROR 0x48: loadCamera's path
    void            *m_focusData;     // ROR 0x88: loadFocus's buffer
    char             m_focusPath[0x40];  // ROR 0x8c: loadFocus's path
    int              m_actor[0x10];   // ROR 0xcc: the models the scene loaded
    int              m_numActor;      // ROR 0x10c
    char             m_name[0x20];    // ROR 0x110
    u_char           _130[0x1b0 - 0x130];
    DisplayMessage   m_msg;           // ROR 0x1b0
    SeManager        m_se;            // ROR 0x240
    VoiceManager     m_voice;         // ROR 0x548
};

/* PLScenePlay : CmnITask (vtable 0x2e4d90, EE 8 bytes; kind 0xf): a task
 * that does nothing. Its constructor (0x1f8f70, hidden after
 * scenePlayStartSimple) has no callers. layout: host -- a vptr. */
struct PLScenePlay : CmnITask
{
    PLScenePlay() : m_04(0) {}                                           // ROR 0x001f8f70
    ~PLScenePlay() override {}                                           // ROR 0x001f8f84

    int  get_task_type() override { return 0xf; }                        // ROR 0x002909bc
    void init() override {}                                              // ROR 0x002909c4
    int  exec(int, int, int) override { return 0; }                      // ROR 0x002909cc
    void term() override {}                                              // ROR 0x002909d4

    int m_04;                   // ROR 0x04
};

/* The per-character request (jump tables data 0x2fece0 / 0x2fed10 /
 * 0x2fed40). chr 11 takes chr from object chrOid's kind (oid bits 23..27):
 * 4 for kind 0 (the PC), 3 for kind 1 (the dog), for kind 2 its system
 * field 0x72 (0..2 -> 0, 3 -> 1, 4..8 -> 5..9, 9 -> 2, 10 -> 10), other
 * kinds nothing. Then chr 3 -> scnChrRequestDog(oid, word), 4 ->
 * scnChrRequestPC(oid, word), 0..2 / 5..10 -> scnDamageStart(chrOid,
 * oid, {a2, word, chr}, a6). a3 is not read. TskMovePhysic passes chr 4
 * or 3 with word 0x15. */
void scnChrRequest(int chr, u_int oid, int a2, int a3, int word, u_int chrOid, int a6); // ROR 0x001fac60
/* Attacker oid hits the dog (only while the dog follows, inu mode 1, and
 * Brown is not in state 10): LP less the damage, Brown's reaction by the
 * damage's share of his max LP; words 0x25 / 0x2d react not. */
void scnChrRequestDog(u_int oid, int word);                              // ROR 0x001fae14
/* Attacker oid hits the PC (not in state class 9): LP less the damage and
 * the hurt state by word (data 0x2feea0; 2: front 5 / back 4 by the
 * attacker's side), or state 3 and a TskFrameFunc kind 0xe at 0 LP. */
void scnChrRequestPC(u_int oid, int word);                               // ROR 0x001fb084
/* The damage attacker deals target with attack word: the attacker's ST
 * (param 0x1d) times the word's percent (random ranges per word), times
 * its weapon's field 0x2b plus field 0x2a, less the target's DEF (field
 * 0x1e) times 90..110 percent; at least 0. Kind 5 attackers deal field
 * 0x2b times 70..100 percent times field 0x2a. */
int  scnCalcDamage(u_int attacker, u_int target, int word);              // ROR 0x001fb624
/* scnCalcDamage with the PC as the attacker (mag/maglifepc2.cpp). */
int  scnCalcPcDamage(u_int target, int word);                            // ROR 0x001fb5d4
/* The game heap's largest free chunk. Unreferenced. */
int  scnGetMaxFreeMem(void);                                             // ROR 0x001fbba4
/* Unless the achr of oid has flag 0x400: pauses its enemy, replaces its
 * task m_task[3] with a new TskDamage(oid, attacker, param, kind) and
 * creates effect kind 1 40 below it (scale 1.75 for kind 2, else 1). */
void scnDamageStart(u_int oid, u_int attacker, const ScnDamageParam *param, int kind); // ROR 0x001fa5bc
/* Kills the achr's tasks and starts TskAttack(oid, param) on its
 * m_task[3]. */
void scnAttackStart(u_int oid, const int *param);                        // ROR 0x001f9d90
/* SE se delay frames from now (a TskGenFrameFunc of kind 1). */
void scnPlaySEDelayed(u_int se, int delay);                              // ROR 0x001f97c8

/* "name.ext" -> "name/name" into out (the event pack's file name). */
char *scnMakeScenePath(char *out, const char *name);                     // ROR 0x001f839c
/* Starts the event scene name (natives 0x46 / 0x47): kills kind-5 tasks,
 * loads the THREAD models not given in oids (one oid a thread, -1 = load
 * "name"), queues a ScenePlay; withEff also creates the pack's effect and
 * focus track. Returns the "frame" count. */
int  scenePlayStart(const char *name, intptr_t oids, int withEff);       // ROR 0x001f83f4
/* Starts the event scene name's effect only (seqcall's native 0x18): a
 * ScenePlay without threads, frames or messages, its m_eff the pack's
 * file 1 (kind 0xd). */
void scenePlayStartSimple(const char *name);                             // ROR 0x001f8cec

/* GamMan::loadSystemData's last call: empty in the ROM. */
void gamSystemDataLoaded(void);                                          // ROR 0x001f8fb8
/* The end of a stage load (GamMan::execStageReq change kind 2, initStage2,
 * native 027, ScenePlay): the free-memory debug print. */
void gamStage2Loaded(void);                                              // ROR 0x001fbbd0

#endif /* _TASK_TSK_SCENE_H */
