/* ==========================================================================
 *  task/tsk_chr.cpp
 *
 *  The script's tasks: the achr motion / movement tasks with their chr*
 *  starters and the generic script tasks (pinned 0x1e7f28..0x1f6c64); see
 *  tsk_chr.h. Written so far: TskMtnMove / TskMtnMove2 and their starters
 *  (chrMtnMove, chrMtnMove2, chrChangeMotion, chrKillMotionTask,
 *  chrSetMotion, chrGetNodeWorldPos), TskMoveTypeRound (circling),
 *  TskMoveTypeGen (straight moves), TskMovePointPath (path walks) with
 *  chrMoveFrames, chrChangeMotionBySuffix, TskMovie (the film task),
 *  TskMsgMan (the message window task), TskChrRot and chrRotStart / chrRotTo /
 *  chrRotToIn / chrRotFrames (turns), TskFrameFunc (the multi-frame
 *  jobs: fades, the door opening, game over, fog, items), the frame
 *  request list.
 * ======================================================================== */
#include "task/tsk_chr.h"

#include <math.h>
#include <string.h>

#include "dog/dogactor.h"
#include "dog/dogcamera.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "rx/rxsys.h"
#include "sdk/libvu0.h"
#include "sequence/sequence.h"
#include "snd/rxmpeg.h"
#include "snd/sndman.h"
#include "spr/s2d.h"

/* The angle limits the ROM compares in double (data 0x2fd958..). */
#define CHR_PI     3.141592653589793
#define CHR_TWO_PI 6.283185307179586

/* An angle wrapped once into -pi..pi (the ROM's inline: one add or
 * subtract of 2 pi, compared in double). */
static inline float chrWrapPi(float r)
{
    if ((double)r < -CHR_PI)
        r = (float)((double)r + CHR_TWO_PI);
    if ((double)r > CHR_PI)
        r = (float)((double)r - CHR_TWO_PI);
    return r;
}

/* chrWrapPi, then 0 when still outside -pi..pi. */
static inline float chrNormPi(float r)
{
    r = chrWrapPi(r);
    if ((double)r < -CHR_PI || (double)r > CHR_PI)
        r = 0.0f;
    return r;
}

/* The heading "keep the current one" (rotY >= 32769). */
#define CHR_ROT_KEEP 32769.0f

int   g_frameReqNum;                    /* data 0x2d8f68 */
u_int g_frameReq[GAM_FRAME_REQ_MAX];    /* data 0x2d8f70 */

// ──────────────────────────────────────────────────────────────────────
// The task: the motion (unless it runs already), its speed and length,
// the heading, the achr, and the step function.
// callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (libgcc soft double: the double compares of chrWrapPi / chrNormPi)
// ROR 0x001e7f28

TskMtnMove::TskMtnMove(float speed, float rotY, int no, int motion, int p1d8, u_int flags,
                       u_int oid)
{
    m_actor = no;
    m_motion = motion;
    m_flagsF = (float)(int)flags;
    m_oid = oid;
    m_speed = speed;
    m_flags = 0;
    if ((short)m_actor == -1)
    {
        m_stepFunc = &TskMtnMove::stepNone;
        return;
    }

    MagScene &scene = g_magSystem.m_scene;
    if (m_motion != scene.getActorMotion(m_actor))
    {
        DogActor &dog = scene.m_actors[m_actor];
        if (gamOidType(oid) == 2)
            dog.changeMotion(speed, m_motion, (int)flags >> 8, flags & 0xff, p1d8);
        else
            dog.changeMotion(speed, m_motion, (int)flags >> 8, flags & 0xff,
                             p1d8 != -1 ? p1d8 : 0);
    }
    m_step = speed / 0x1.11111p-6f; /* 0x3c888888 (1/60) */
    scene.setActorMotionSpeed(m_step, m_actor);
    m_end = scene.getActorMotionLength(m_actor, m_motion) * 120.0f;

    DogActor &dog = scene.m_actors[m_actor];
    m_rotY = dog.m_base_rotate[1];
    if (p1d8 < 0)
    {
        if (rotY >= CHR_ROT_KEEP)
        {
            m_rotY = dog.m_rotate[1];
        }
        else
        {
            sceVu0FVECTOR rot;
            rotY = chrNormPi(rotY);
            rot[0] = 0.0f;
            rot[1] = rotY;
            rot[2] = 0.0f;
            rot[3] = 0.0f;
            g_magSystem.setActorRotate(m_actor, rot);
            m_rotY = rot[1];
        }
    }
    else
    {
        // PORT: at the motion's first frame the ROM also wraps rotY into a
        // vector it never uses; the heading stays the actor's either way.
        if (dog.getMotionFrame120() < 0x1.0624dcp-10f /* 0x3a83126e */)
            (void)chrNormPi(rotY);
        m_rotY = dog.m_rotate[1];
    }

    if (gamOidList(m_oid) == 1)
    {
        m_achr = g_gamMan.getAchr(m_oid);
        m_dogActor = g_gamMan.getActor(m_oid);
    }
    else
    {
        m_achr = NULL;
        m_dogActor = NULL;
    }
    if (m_achr != NULL
        && (((m_achr->m_flags & 0x40) != 0 && (m_achr->m_flags & 0x80) == 0)
            || (m_achr->m_flags & 0x4000000) != 0))
        m_stepFunc = &TskMtnMove::stepMoveHit;
    else
        m_stepFunc = &TskMtnMove::stepMove;
}

// ──────────────────────────────────────────────────────────────────────
// Empty.
// ROR 0x001e8408

void TskMtnMove::term()
{
}

// ──────────────────────────────────────────────────────────────────────
// The step function the constructor picked.
// ROR 0x001e8410

int TskMtnMove::exec(int id, int time, int arg)
{
    (void)id;
    (void)time;
    (void)arg;
    return (this->*m_stepFunc)();
}

// ──────────────────────────────────────────────────────────────────────
// Pausing freezes the actor's motion (speed 1e-7) and keeps the speed in
// the achr's param 0x1201; resuming restores it (1/60 when it is about 0).
// The PC's task is left alone.
// ROR 0x001e8478

void TskMtnMove::pause(int on)
{
    float speed;
    u_int pc = 0;

    if (g_gamMan.m_pcPtr != NULL)
        pc = g_gamMan.m_pcPtr->m_oid;
    if (m_oid == pc)
        return;

    if (on != 0)
    {
        speed = g_magSystem.m_scene.getActorMotionSpeed(m_actor);
        if ((m_flags & 2) == 0)
        {
            if (m_dogActor != NULL)
                m_dogActor->m_motion.m_speed = 0x1.ad7f28p-24f; /* 0x33d6bf94 */
            if (m_achr != NULL)
                m_achr->setParam(0x1201, &speed);
        }
        m_flags |= 2;
        return;
    }

    speed = 0x1.11111p-6f; /* 0x3c888888 */
    if ((m_flags & 2) != 0)
    {
        if (m_achr != NULL)
            m_achr->getParam(0x1201, &speed);
        if (speed < 0.0f)
        {
            if (-0x1.0624dcp-10f /* 0xba83126e */ < speed)
                speed = 0x1.11111p-6f;
        }
        else if (speed < 0x1.0624dcp-10f /* 0x3a83126e */)
        {
            speed = 0x1.11111p-6f;
        }
        if (m_dogActor != NULL)
            m_dogActor->m_motion.m_speed = speed;
    }
    m_flags &= ~2u;
}

// ──────────────────────────────────────────────────────────────────────
// One frame: while the motion runs (speed > 1e-7, before its end) the
// actor moves by the root move of the next two steps, turned by the
// heading. Ends (1) when the object lost its actor.
// callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (libgcc soft double: the double compares of chrWrapPi / chrNormPi)
// ROR 0x001e85d0

int TskMtnMove::stepMove()
{
    MagScene &scene = g_magSystem.m_scene;
    float speed = scene.getActorMotionSpeed(m_actor);
    float frame = scene.m_actors[m_actor].getMotionFrame120();
    float rot[4];

    if (frame >= 0.0f && (m_flags & 2) == 0 && speed > 0x1.ad7f28p-24f /* 0x33d6bf94 */
        && frame < m_end)
    {
        m_actor = g_gamMan.getActorNo(m_oid);
        if (m_actor == -1)
            return 1;
        if (g_magSystem.getActorMotionMove(m_actor, frame, frame + (m_step + m_step), m_move,
                                           rot, m_mpos) != 0)
        {
            if ((m_flags & 1) != 0)
                return 0;
            float r = chrWrapPi(-m_rotY);
            m_move[1] = 0.0f;
            m_move[3] = 1.0f;
            m_trans[0] = m_move[0] * cosf(r) - m_move[2] * sinf(r);
            m_trans[3] = 1.0f;
            m_trans[1] = 0.0f;
            m_trans[2] = m_move[0] * sinf(r) + m_move[2] * cosf(r);
            g_magSystem.addActorTrans(m_actor, m_trans, m_mpos[1], rot[0]);
        }
    }
    m_rotYOut = m_rotY;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// stepMove for achrs off the secta lists: the heading follows the actor
// while the motion stands; a move is refused (the actor only turns) when
// the hit test finds something at the target or no cell is there.
// callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (libgcc soft double: the double compares of chrWrapPi / chrNormPi)
// ROR 0x001e89c0

int TskMtnMove::stepMoveHit()
{
    sceVu0FVECTOR zero;
    float rot[4];

    memset(zero, 0, sizeof(zero));
    MagScene &scene = g_magSystem.m_scene;
    float frame = scene.m_actors[m_actor].getMotionFrame120();
    float speed = scene.getActorMotionSpeed(m_actor);

    if (frame < 0x1.0624dcp-10f /* 0x3a83126e */ || (m_flags & 2) != 0
        || (speed < 0.0f ? speed >= -0x1.ad7f28p-24f /* 0xb3d6bf94 */
                         : speed <= 0x1.ad7f28p-24f /* 0x33d6bf94 */))
    {
        m_rotY = m_dogActor->getRotY();
    }
    else if (frame < m_end && (m_flags & 1) == 0)
    {
        m_actor = g_gamMan.getActorNo(m_oid);
        if (m_actor != -1
            && g_magSystem.getActorMotionMove(m_actor, frame, frame + (m_step + m_step), m_move,
                                              rot, m_mpos) != 0)
        {
            m_move[1] = 0.0f;
            m_move[3] = 1.0f;
            m_rotY = chrNormPi(m_rotY);
            float r = chrWrapPi(-m_rotY);
            m_trans[0] = m_move[0] * cosf(r) - m_move[2] * sinf(r);
            m_trans[1] = 0.0f;
            m_trans[3] = 1.0f;
            m_trans[2] = m_move[0] * sinf(r) + m_move[2] * cosf(r);
            m_achr->getPos(m_pos);
            m_pos[0] += m_trans[0];
            m_pos[2] += m_trans[2];
            if (g_gamMan.testHitAt(m_pos, m_oid, 0, 0, NULL) != 0)
                g_magSystem.addActorTrans(m_actor, zero, m_mpos[1], rot[0]);
            else if (m_achr->hasSectaAt(m_pos) == 0)
                g_magSystem.addActorTrans(m_actor, zero, m_mpos[1], rot[0]);
            else
                g_magSystem.addActorTrans(m_actor, m_trans, m_mpos[1], rot[0]);
        }
    }
    m_rotYOut = m_rotY;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// No actor: the task ends at once (hidden after chrMotionRefEnd; created
// in Ghidra 2026-10-26).
// ROR 0x001e8fbc

int TskMtnMove::stepNone()
{
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The motion of ref's actor is over (no mode set).
// ROR 0x001e8f88

int chrMotionRefEnd(const ChrMotionRef *ref)
{
    return !g_magSystem.m_scene.m_actors[ref->m_actor].isModeSet();
}

// ──────────────────────────────────────────────────────────────────────
// The task: the motion (unless it runs already: flag 4), its speed and
// length, the base heading, the achr, and the step function (the same
// choice as TskMtnMove's).
// ROR 0x001e8fc4

TskMtnMove2::TskMtnMove2(float speed, int no, int motion, int p1d8, u_int flags, u_int oid)
{
    m_mpos[0] = 0.0f;
    m_mpos[1] = 0.0f;
    m_mpos[2] = 0.0f;
    m_mpos[3] = 1.0f;
    m_move[0] = 0.0f;
    m_move[1] = 0.0f;
    m_move[2] = 0.0f;
    m_move[3] = 1.0f;
    m_actor = no;
    m_motion = motion;
    m_flagsF = (float)(int)flags;
    m_oid = oid;
    m_speed = speed;
    m_flags = 0;
    m_c4 = 0;
    if ((short)m_actor == -1)
    {
        m_stepFunc = &TskMtnMove2::stepNone;
        return;
    }

    if (gamOidList(oid) == 1)
    {
        m_achr = g_gamMan.getAchr(oid);
        m_dogActor = g_gamMan.getActor(m_oid);
    }
    else
    {
        m_achr = NULL;
        m_dogActor = NULL;
    }
    MagScene &scene = g_magSystem.m_scene;
    if (m_motion != scene.getActorMotion(m_actor))
    {
        m_dogActor->changeMotion(speed, m_motion, (int)flags >> 8, flags & 0xff, p1d8);
        m_flags |= 4;
    }
    m_step = speed / 0x1.11111p-6f; /* 0x3c888888 (1/60) */
    scene.setActorMotionSpeed(m_step, m_actor);
    m_end = scene.getActorMotionLength(m_actor, m_motion) * 120.0f;
    m_rotY = scene.m_actors[m_actor].m_base_rotate[1];

    if (m_achr != NULL
        && (((m_achr->m_flags & 0x40) != 0 && (m_achr->m_flags & 0x80) == 0)
            || (m_achr->m_flags & 0x4000000) != 0))
    {
        m_radius = g_gamMan.getObjRadius(m_oid, NULL);
        m_stepFunc = &TskMtnMove2::stepMoveHit;
    }
    else
    {
        m_stepFunc = &TskMtnMove2::stepMove;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Empty.
// ROR 0x001e9224

void TskMtnMove2::term()
{
}

// ──────────────────────────────────────────────────────────────────────
// The step function the constructor picked.
// ROR 0x001e922c

int TskMtnMove2::exec(int id, int time, int arg)
{
    (void)id;
    (void)time;
    (void)arg;
    return (this->*m_stepFunc)();
}

// ──────────────────────────────────────────────────────────────────────
// TskMtnMove::pause's rule: pausing freezes the motion (speed 1e-7) and
// keeps the speed in the achr's param 0x1201; resuming restores it (1/60
// when it is about 0). The PC's task is left alone.
// ROR 0x001e9294

void TskMtnMove2::pause(int on)
{
    float speed;
    u_int pc = 0;

    if (g_gamMan.m_pcPtr != NULL)
        pc = g_gamMan.m_pcPtr->m_oid;
    if (m_oid == pc)
        return;

    if (on != 0)
    {
        speed = g_magSystem.m_scene.getActorMotionSpeed(m_actor);
        if ((m_flags & 2) == 0)
        {
            if (m_dogActor != NULL)
                m_dogActor->m_motion.m_speed = 0x1.ad7f28p-24f; /* 0x33d6bf94 */
            if (m_achr != NULL)
                m_achr->setParam(0x1201, &speed);
        }
        m_flags |= 2;
        return;
    }

    speed = 0x1.11111p-6f; /* 0x3c888888 */
    if ((m_flags & 2) != 0)
    {
        if (m_achr != NULL)
            m_achr->getParam(0x1201, &speed);
        if (speed < 0.0f)
        {
            if (-0x1.0624dcp-10f /* 0xba83126e */ < speed)
                speed = 0x1.11111p-6f;
        }
        else if (speed < 0x1.0624dcp-10f /* 0x3a83126e */)
        {
            speed = 0x1.11111p-6f;
        }
        if (m_dogActor != NULL)
            m_dogActor->m_motion.m_speed = speed;
    }
    m_flags &= ~2u;
}

// ──────────────────────────────────────────────────────────────────────
// One frame: while the motion runs (|speed| > 1e-7, before its end) the
// actor moves by the root move of the next two steps turned by its base
// heading; paused or standing it only takes the motion's turn. Ends (1)
// when the object lost its actor.
// ROR 0x001e93ec

int TskMtnMove2::stepMove()
{
    MagScene &scene = g_magSystem.m_scene;
    sceVu0FVECTOR zero = {0.0f, 0.0f, 0.0f, 1.0f};
    sceVu0FMATRIX mat;
    float rot[4];
    float speed = scene.getActorMotionSpeed(m_actor);
    float frame = scene.m_actors[m_actor].getMotionFrame120();

    if (frame >= 0.0f && (m_flags & 2) == 0
        && (speed < 0.0f ? speed < -0x1.ad7f28p-24f /* 0xb3d6bf94 */
                         : speed > 0x1.ad7f28p-24f /* 0x33d6bf94 */))
    {
        if (frame < m_end)
        {
            m_actor = g_gamMan.getActorNo(m_oid);
            if (m_actor == -1)
                return 1;
            if (m_dogActor->getMotionMove(frame, frame + (m_step + m_step), m_move, rot, m_mpos)
                != 0)
            {
                if ((m_flags & 1) != 0)
                    return 0;
                m_move[3] = 1.0f;
                m_move[1] = 0.0f;
                sceVu0UnitMatrix(mat);
                sceVu0RotMatrixY(mat, mat, m_dogActor->m_base_rotate[1]);
                sceVu0ApplyMatrix(m_move, mat, m_move);
                m_dogActor->addTrans(m_move, m_mpos[1], rot[0]);
            }
        }
    }
    else if (m_dogActor->getMotionMove(frame, frame + (m_step + m_step), m_move, rot, m_mpos)
             != 0)
    {
        m_dogActor->addTrans(zero, m_mpos[1], rot[0]);
    }
    m_rotYOut = m_rotY;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// stepMove for achrs off the secta lists: standing, the heading follows
// the actor. A move is refused (the actor only turns) when the hit test
// finds something at the target -- for achrs on the secta lists, only
// when also no cell lies 1.25 radii ahead -- and then, when the actor
// stands in something, a TskAvoidMove (unless one runs) pushes it out.
// The ROM adds the unturned move to the position before the test (kept).
// ROR 0x001e9614

int TskMtnMove2::stepMoveHit()
{
    MagScene &scene = g_magSystem.m_scene;
    sceVu0FVECTOR zero = {0.0f, 0.0f, 0.0f, 1.0f};
    sceVu0FMATRIX mat;
    u_int hit[8];
    float rot[4];
    float to[4];
    float ahead[4];
    int   refused = 0;
    float speed = scene.getActorMotionSpeed(m_actor);
    float frame = scene.m_actors[m_actor].getMotionFrame120();

    rot[0] = 0.0f;
    if (frame < 0.0f || (m_flags & 2) != 0
        || (speed < 0.0f ? speed >= -0x1.ad7f28p-24f /* 0xb3d6bf94 */
                         : speed <= 0x1.ad7f28p-24f /* 0x33d6bf94 */))
    {
        m_rotY = m_dogActor->getRotY();
    }
    else if (frame < m_end && (m_flags & 1) == 0)
    {
        m_actor = g_gamMan.getActorNo(m_oid);
        if (m_actor != -1
            && m_dogActor->getMotionMove(frame, frame + (m_step + m_step), m_move, rot, m_mpos)
                   != 0)
        {
            m_achr->getPos(m_achrPos);
            m_move[1] = 0.0f;
            m_move[3] = 1.0f;
            if (chrGetNodeWorldPos(m_oid, m_pos, "Bip01 Spine1") == 0)
                m_achr->getPos(m_pos);
            m_pos[0] += m_move[0];
            m_pos[2] += m_move[2];
            sceVu0UnitMatrix(mat);
            sceVu0RotMatrixY(mat, mat, m_dogActor->m_base_rotate[1]);
            sceVu0ApplyMatrix(m_move, mat, m_move);

            to[0] = m_pos[0] + m_move[0];
            to[1] = 0.0f;
            to[2] = m_pos[2] + m_move[2];
            to[3] = 0.0f;
            float d = m_radius * 1.25f;
            ahead[0] = m_pos[0] + (m_move[0] < 0.0f ? -d : d);
            ahead[1] = 0.0f;
            ahead[2] = m_pos[2] + (m_move[2] < 0.0f ? -d : d);
            ahead[3] = 0.0f;
            if (g_gamMan.testHitAt(to, m_oid, 0, 1, hit) != 0 && (m_achr->m_flags & 0x40) != 0)
                refused = 1;
            else if (m_achr->hasSectaAt(ahead) == 0)
                refused = 1;
            else
                m_dogActor->addTrans(m_move, m_mpos[1], rot[0]);

            if (refused)
            {
                sceVu0ApplyMatrix(zero, mat, zero);
                m_dogActor->addTrans(zero, m_mpos[1], rot[0]);
                if (chrGetNodeWorldPos(m_oid, m_pos, "Bip01 Spine1") == 0)
                    m_achr->getPos(m_pos);
                if (g_gamMan.testHitAt(m_pos, m_oid, 0, 1, hit) != 0
                    && g_taskMan.getTask(m_achr->m_taskId) == 0)
                {
                    // callcheck: operator_new (the new expression)
                    TskAvoidMove *task = new TskAvoidMove(m_oid, hit, 8, 0x10, 0, 0);
                    g_taskMan.addTask(1, -1, 1, 0, task, 0, &m_achr->m_taskId, 0x11);
                }
            }
        }
    }
    m_rotYOut = m_rotY;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// No actor: the task ends at once (hidden after stepMoveHit; created in
// Ghidra 2026-10-26).
// ROR 0x001e9a34

int TskMtnMove2::stepNone()
{
    return 1;
}

/* The heading a quarter turn from dir (way < 0 adds pi/2, else subtracts;
 * in double, as the ROM does), wrapped once into -pi..pi. */
static inline float chrQuarterTurn(float dir, float way)
{
    float r;

    if (way < 0.0f)
        r = (float)((double)dir + 1.5707963267948966);
    else
        r = (float)((double)dir - 1.5707963267948966);
    return chrWrapPi(r);
}

/* A float frame count as the ROM converts it to unsigned (cvt.w.s, with
 * 2^31 taken off and put back as the top bit above it). */
static inline int chrFloatToU(float f)
{
    if (f >= 2147483648.0f)
        return (int)((u_int)(int)(f - 2147483648.0f) | 0x80000000u);
    return (int)f;
}

/* The slide of speed a frame along dir (the y step 0). */
static inline void chrSlideVel(float *vel, float dir, float speed)
{
    vel[1] = 0.0f;
    vel[0] = sinf(dir) * speed;
    vel[2] = cosf(dir) * speed;
}

/* TskMoveTypeGen's step after an aim: modes 0..7 slide (stepSlideHit for
 * an achr off the secta lists: flag 0x40 without 0x80), 8.. walk by the
 * motion (the member pointers data 0x2fdac0..). */
static inline TskMoveTypeGen::StepFunc chrMoveGenStep(int mode, const ACHRstat *achr)
{
    if (mode >= 8)
        return &TskMoveTypeGen::stepMotion;
    if ((achr->m_flags & 0x40) == 0 || (achr->m_flags & 0x80) != 0)
        return &TskMoveTypeGen::stepSlide;
    return &TskMoveTypeGen::stepSlideHit;
}

// ──────────────────────────────────────────────────────────────────────
// The straight move: the achr's walk and turn params (the walk speed per
// 60th, 1/60 for modes above 5 outside 0.0001..1), its actor's position
// and heading, then the aim and the turn to it.
// ROR 0x001e9a3c

TskMoveTypeGen::TskMoveTypeGen(u_int oid, int mode, intptr_t arg, int kind, float frames,
                               float rate)
{
    (void)rate;
    m_oid = oid;
    m_achr = g_gamMan.getAchr(oid);
    m_actor = m_achr->m_actor;
    m_mode = mode;
    m_arg = arg;
    m_value = frames;
    m_kind = kind;
    m_mtnTask = NULL;
    m_stepFunc = NULL;
    m_flags = 0;
    m_every = 0;
    // PORT: the ROM leaves these unset; the modes that do not write them
    // read garbage (mode 15 slides by m_vel for m_frames frames).
    m_targetOid = 0;
    m_speed = m_dist = 0.0f;
    m_frames = m_half = 0;
    memset(m_target, 0, sizeof(m_target));
    memset(m_vel, 0, sizeof(m_vel));
    m_pos[3] = m_rot[3] = 0.0f;
    m_achr->getParam(0x1100, &m_motion);
    m_achr->getParam(0x1103, &m_rate);
    m_achr->getParam(0x1101, &m_turnRight);
    m_achr->getParam(0x1102, &m_turnLeft);
    m_achr->getParam(0x1105, &m_turnBack);
    m_achr->getParam(0x1104, &m_rotSpeed);
    m_rate = m_rate * 0x1.11111p-6f;
    if (m_mode > 5 && !(0.0001f < m_rate && m_rate < 1.0f))
        m_rate = 0x1.11111p-6f;
    if ((m_achr->m_flags & 0x400000) != 0)
        m_flags |= 1;
    // PORT: the ROM reads actor -1's slot too; an achr here has an actor.
    if (m_actor >= 0)
    {
        DogActor &actor = g_magSystem.m_scene.m_actors[m_actor];
        m_pos[0] = actor.m_trans[0];
        m_pos[1] = actor.m_trans[1];
        m_pos[2] = actor.m_trans[2];
        m_rot[0] = 0.0f;
        m_rot[1] = actor.getRotY();
        m_rot[2] = 0.0f;
    }
    turnTo(aim());
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e9cac

void TskMoveTypeGen::term()
{
}

// ──────────────────────────────────────────────────────────────────────
// The step while frames are left; then next (the end, or a new aim).
// ROR 0x001eae7c

int TskMoveTypeGen::exec(int id, int time, int arg)
{
    (void)id;
    (void)time;
    (void)arg;
    if (m_frames > 0)
        return (this->*m_stepFunc)();
    return next();
}

// ──────────────────────────────────────────────────────────────────────
// Sets off by mode (jump table data 0x2ff6e0, 15 cases): the velocity and
// frames of a slide (with the walk motion at m_rate), or for modes 8.. a
// chrMtnMove of the walk motion. Returns the heading to turn to.
// callcheck: sinf, cosf, fptodp, dpadd, dpcmp, dpsub, dptofp (chrSlideVel; libgcc soft double: chrWrapPi and the dir + pi)
// ROR 0x001e9cb4

float TskMoveTypeGen::aim()
{
    float dir = 0.0f;
    float dist;

    memcpy(m_lastPos, m_pos, sizeof(m_lastPos));
    const float *p = (const float *)m_arg;
    switch ((u_int)m_mode)
    {
    case 0:
    case 8:
        // By the vector p each frame.
        m_vel[0] = p[0];
        m_vel[1] = p[1];
        m_vel[2] = p[2];
        dir = utlAtan2(m_vel[0], m_vel[2]);
        m_frames = chrFloatToU(m_value);
        if (m_mode >= 8)
        {
            m_mtnTask = chrMtnMove(m_rate, dir, m_oid, m_motion, -1, 0x100, NULL);
            m_half = 0;
        }
        else
        {
            chrSetMotion(m_rate, m_actor, m_motion, 0, 0x100, m_oid, NULL);
        }
        break;
    case 1:
    case 9:
    case 2:
    case 10:
        // Along the heading: 1 at the speed *p, 2 the distance *p.
        m_frames = chrFloatToU(m_value);
        dir = m_rot[1];
        if ((m_mode & 7) == 1)
        {
            m_speed = p[0];
        }
        else
        {
            m_dist = p[0];
            m_speed = m_dist / (float)m_frames;
        }
        chrSlideVel(m_vel, dir, m_speed);
        if (m_mode >= 8)
        {
            m_mtnTask = chrMtnMove(m_rate, CHR_ROT_KEEP, m_oid, m_motion, -1, 0x100, NULL);
            m_half = 0;
        }
        else
        {
            chrSetMotion(m_rate, m_actor, m_motion, 0, 0x100, m_oid, NULL);
        }
        break;
    case 3:
    case 4:
    case 11:
    case 12:
    {
        // Toward the object p at the speed m_value (negative: backwards).
        m_targetOid = (u_int)m_arg;
        m_speed = m_value;
        g_gamMan.getObjPos(m_targetOid, m_target);
        dir = utlDirDist(m_target[0] - m_pos[0], m_target[2] - m_pos[2], &dist);
        float speed = m_speed;
        if (speed < 0.0f)
            dir = (float)((double)dir + CHR_PI);
        dir = chrWrapPi(dir);
        m_mtnTask = NULL;
        if (m_mode < 8)
        {
            if (speed == 0.0f)
                m_speed = 1.0f;
            m_frames = (int)(dist / m_speed);
            dir = chrWrapPi(dir);
            chrSlideVel(m_vel, dir, m_speed);
            chrSetMotion(m_rate, m_actor, m_motion, 0, 0x100, m_oid, NULL);
        }
        else
        {
            m_frames = chrMoveFrames(dir, dist, m_rate, m_oid, (u_short)m_actor, &m_mtnTask,
                                     m_motion);
            m_half = m_frames / 2;
        }
        if (m_mode == 4 || m_mode == 12)
        {
            m_frames = m_kind;
            m_every = m_kind;
        }
        break;
    }
    case 5:
    case 13:
    case 7:
    case 6:
    case 14:
        // To the point p: 5 / 7 at the speed m_value, 6 in m_value frames.
        if (m_mode != 6 && m_mode != 14)
            m_speed = m_value;
        m_target[0] = p[0];
        m_target[1] = p[1];
        m_target[2] = p[2];
        m_target[3] = p[3];
        dir = utlDirDist(m_target[0] - m_pos[0], m_target[2] - m_pos[2], &dist);
        if (m_mode >= 8)
        {
            if (m_mode == 13)
                m_mtnTask = NULL;
            m_frames = chrMoveFrames(dir, dist, m_rate, m_oid, (u_short)m_actor, &m_mtnTask,
                                     m_motion);
            m_half = m_frames / 2;
            break;
        }
        if (m_mode == 6)
        {
            m_frames = (int)m_value;
            // PORT: frames 0 divides by 0 (the EE FPU gives the largest
            // float, cvt.w.s then 0x7fffffff).
            m_speed = m_frames != 0 ? (float)(int)(dist / (float)m_frames) : 2147483647.0f;
        }
        else
        {
            if (m_speed == 0.0f)
                m_speed = 1.0f;
            m_frames = (int)(dist / m_speed);
        }
        dir = chrWrapPi(dir);
        chrSlideVel(m_vel, dir, m_speed);
        chrSetMotion(m_rate, m_actor, m_motion, 0, 0x100, m_oid, NULL);
        break;
    default:
        break;
    }
    m_stepFunc = chrMoveGenStep(m_mode, m_achr);
    return dir;
}

// ──────────────────────────────────────────────────────────────────────
// Modes 3 / 4 / 11 / 12 aim again at the object from the actor's
// position (the heading the motion task holds while one runs); modes 8..
// start a new chrMtnMove with the heading kept.
// callcheck: sinf, cosf, fptodp, dpadd, dpcmp, dpsub, dptofp (chrSlideVel; libgcc soft double: chrWrapPi and the dir + pi)
// ROR 0x001ea5ec

float TskMoveTypeGen::retarget()
{
    float dir = 0.0f;
    float dist;

    DogActor &actor = g_magSystem.m_scene.m_actors[m_actor];
    m_pos[0] = actor.m_trans[0];
    m_pos[1] = actor.m_trans[1];
    m_pos[2] = actor.m_trans[2];
    int id = m_achr->m_task[2];
    if (g_taskMan.getTask(id) == 0)
    {
        m_rot[0] = actor.m_rotate[0];
        m_rot[1] = actor.m_rotate[1];
        m_rot[2] = actor.m_rotate[2];
    }
    else
    {
        m_rot[1] = ((TskMtnMove *)g_taskMan.findTask(id))->m_rotY;
    }
    memcpy(m_lastPos, m_pos, sizeof(m_lastPos));
    switch (m_mode)
    {
    case 3:
    case 4:
    case 11:
    case 12:
    {
        m_targetOid = (u_int)m_arg;
        m_speed = m_value;
        g_gamMan.getObjPos(m_targetOid, m_target);
        dir = utlDirDist(m_target[0] - m_pos[0], m_target[2] - m_pos[2], &dist);
        float speed = m_speed;
        if (speed < 0.0f)
            dir = (float)((double)dir + CHR_PI);
        dir = chrWrapPi(dir);
        m_mtnTask = NULL;
        if (m_mode < 8)
        {
            if (speed == 0.0f)
                m_speed = 1.0f;
            m_frames = (int)(dist / m_speed);
            dir = chrWrapPi(dir);
            chrSlideVel(m_vel, dir, m_speed);
            chrSetMotion(m_rate, m_actor, m_motion, 0, 0x100, m_oid, NULL);
        }
        else
        {
            m_frames = chrMoveFrames(CHR_ROT_KEEP, dist, m_rate, m_oid, (u_short)m_actor,
                                     &m_mtnTask, m_motion);
            m_half = m_frames / 2;
        }
        if (m_mode == 4 || m_mode == 12)
            m_frames = m_every;
        break;
    }
    default:
        break;
    }
    m_stepFunc = chrMoveGenStep(m_mode, m_achr);
    return dir;
}

// ──────────────────────────────────────────────────────────────────────
// The turn to dir: with a turn motion and dir within m_turnBack of
// straight behind, the turn motion by the side (dir - heading < 0 left)
// and stepTurn waits for it; else a plain turn (chrRotTo; modes 8.. from
// the task's heading).
// callcheck: fptodp, dpadd, dpcmp, dpsub, dptofp (libgcc soft double: chrWrapPi and the heading + pi)
// ROR 0x001eafb0

void TskMoveTypeGen::turnTo(float dir)
{
    float rotY = m_rot[1];

    if (m_turnLeft != -1 || m_turnRight != -1)
    {
        float back = chrWrapPi((float)((double)rotY + CHR_PI));
        float off = chrWrapPi(dir - back);
        if ((off < 0.0f ? -off : off) < m_turnBack)
        {
            int motion = dir - rotY < 0.0f ? m_turnLeft : m_turnRight;
            if (m_mode < 8)
                chrRotToMotion(dir, m_rotSpeed, CHR_ROT_KEEP, m_oid, motion, 1);
            else
                chrRotToMotion(dir, m_rotSpeed, rotY, m_oid, motion, 0);
            m_frames = 1;
            m_stepFunc = &TskMoveTypeGen::stepTurn;
            return;
        }
    }
    if (m_mode < 8)
        chrRotTo(dir, m_rotSpeed, m_oid);
    else
        chrRotFromTo(rotY, dir, m_rotSpeed, m_oid);
}

// ──────────────────────────────────────────────────────────────────────
// Frames out: modes other than 3 / 4 / 11 / 12 kill the motion task and
// end; those aim again (the actor re-read when the no is out of range) and
// turn, every m_every frames for 4 / 12; no frames left stops the slide.
// ROR 0x001eaf00

int TskMoveTypeGen::next()
{
    switch (m_mode)
    {
    case 3:
    case 4:
    case 11:
    case 12:
        break;
    default:
        chrKillMotionTask(m_oid);
        return 1;
    }
    if ((u_int)m_actor > 0x200)
        m_actor = m_achr->m_actor;
    turnTo(retarget());
    if (m_every > 0)
        m_frames = m_every;
    if (m_frames == 0)
    {
        m_vel[2] = 0.0f;
        m_vel[0] = 0.0f;
        m_vel[1] = 0.0f;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Waits for the turn motion (the heading followed meanwhile); once the
// turn task is gone (and not on the first frame), aims again from the
// actor and turns the rest of the way.
// ROR 0x001eaa38

int TskMoveTypeGen::stepTurn()
{
    if (g_taskMan.getTask(m_achr->m_task[1]) == 0 && m_frames != 1)
    {
        m_mtnTask = NULL;
        DogActor &actor = g_magSystem.m_scene.m_actors[m_actor];
        m_pos[0] = actor.m_trans[0];
        m_pos[1] = actor.m_trans[1];
        m_pos[2] = actor.m_trans[2];
        m_rot[0] = actor.m_rotate[0];
        m_rot[1] = actor.m_rotate[1];
        m_rot[2] = actor.m_rotate[2];
        m_rot[1] = actor.getRotY();
        float dir = aim();
        if (m_mode < 8)
            chrRotTo(dir, m_rotSpeed, m_oid);
        else
            chrRotFromTo(m_rot[1], dir, m_rotSpeed, m_oid);
    }
    else
    {
        m_achr->ObjStat::getRot(m_rot);
        m_frames = 2;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The slide of an achr off the secta lists: (held while turning when
// flag 1) a hit keeps the last position and prints "HIT in moving task";
// a position with no cell ends the move (next); else a frame is done.
// callcheck: scePrintf (the hit message)
// ROR 0x001eab54

int TskMoveTypeGen::stepSlideHit()
{
    u_int hit[0x10];

    if ((m_flags & 1) != 0 && g_taskMan.getTask(m_achr->m_task[1]) != 0)
        return 0;
    memset(hit, 0, sizeof(hit));  // PORT: placeAchr writes it only on a hit
    if (g_gamMan.placeAchr((int)m_oid, hit, 0) != 0)
    {
        memcpy(m_pos, m_lastPos, sizeof(m_pos));
        scePrintf("HIT in moving task ! oid 0x%x \n", hit[4]);
        return 0;
    }
    sceVu0AddVector(m_pos, m_pos, m_vel);
    if (!m_achr->trySetPos(m_pos))
        return next();
    m_frames--;
    memcpy(m_lastPos, m_pos, sizeof(m_lastPos));
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The slide (held while turning when flag 1), set on the floor.
// ROR 0x001eac38

int TskMoveTypeGen::stepSlide()
{
    if ((m_flags & 1) == 0 || g_taskMan.getTask(m_achr->m_task[1]) == 0)
    {
        sceVu0AddVector(m_pos, m_pos, m_vel);
        m_achr->setPosOnFloor(m_pos);
        m_frames--;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Modes 8..: the motion walks the achr (with flag 1 its moving held while
// a turn task runs); below half the frames the walk aims again at
// m_target from where the actor stands (frames from chrMoveFrames, the
// turn at speed 1), half of them till the next aim (none under 6).
// ROR 0x001eacb8

int TskMoveTypeGen::stepMotion()
{
    TskMtnMove *mtn = (TskMtnMove *)m_mtnTask;

    // PORT: the ROM writes through the task pointer unchecked.
    if ((m_flags & 1) != 0 && mtn != NULL)
    {
        int turning = g_taskMan.getTask(m_achr->m_task[1]) != 0;
        if ((m_flags & 2) == 0)
        {
            if (turning)
            {
                m_flags |= 2;
                mtn->m_flags |= 1;
            }
        }
        else if (!turning)
        {
            m_flags &= ~2u;
            mtn->m_flags &= ~1u;
        }
    }
    if (m_frames < m_half)
    {
        float dist;
        DogActor &actor = g_magSystem.m_scene.m_actors[m_actor];
        m_pos[0] = actor.m_trans[0];
        m_pos[1] = actor.m_trans[1];
        m_pos[2] = actor.m_trans[2];
        float dir = utlDirDist(m_target[0] - m_pos[0], m_target[2] - m_pos[2], &dist);
        m_frames = chrMoveFrames(dir, dist, m_rate, m_oid, (u_short)m_actor, &m_mtnTask,
                                 m_motion);
        m_rot[0] = actor.m_rotate[0];
        m_rot[1] = actor.m_rotate[1];
        m_rot[2] = actor.m_rotate[2];
        chrRotFromTo(m_rot[1], dir, 1.0f, m_oid);
        m_half = m_frames < 6 ? 0 : m_frames / 2;
    }
    m_frames--;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The circling: the achr, its actor's position and heading, the centre
// by mode (0 / 2 an object's actor, 1 / 3 the point arg) and its step;
// modes 0 / 1 play motion at rate here, 2 / 3 walk by a chrMtnMove.
// ROR 0x001eb63c

TskMoveTypeRound::TskMoveTypeRound(float radius, float speed, float rate, u_int oid, int mode,
                                   intptr_t arg, int motion)
{
    m_oid = oid;
    m_achr = g_gamMan.getAchr(oid);
    m_actor = m_achr->m_actor;
    m_radius = radius;
    m_speed = speed;
    m_dir = 1.0f;
    m_mode = mode;
    // PORT: the ROM leaves m_flags, m_lastPos and the w words unset (a
    // hit on the first frame would restore garbage).
    m_flags = 0;
    m_mtnTask = NULL;
    m_target = 0;
    m_targetActor = -1;
    m_pos[3] = m_rot[3] = m_center[3] = 0.0f;
    // PORT: the ROM reads actor -1's slot too; an achr here has an actor.
    if (m_actor >= 0)
    {
        DogActor &actor = g_magSystem.m_scene.m_actors[m_actor];
        m_pos[0] = actor.m_trans[0];
        m_pos[1] = actor.m_trans[1];
        m_pos[2] = actor.m_trans[2];
        m_rot[0] = actor.m_rotate[0];
        m_rot[1] = actor.m_rotate[1];
        m_rot[2] = actor.m_rotate[2];
    }
    memcpy(m_lastPos, m_pos, sizeof(m_lastPos));

    // PORT: other modes set no step in the ROM (exec then calls garbage);
    // they circle the point here.
    m_stepFunc = &TskMoveTypeRound::stepPoint;
    switch (m_mode)
    {
    case 0:
    case 2:
        m_target = (u_int)arg;
        m_targetActor = g_gamMan.getActorNo(m_target);
        // PORT: the ROM reads actor -1's slot too.
        if (m_targetActor >= 0)
        {
            const DogActor &actor = g_magSystem.m_scene.m_actors[m_targetActor];
            m_center[0] = actor.m_trans[0];
            m_center[1] = actor.m_trans[1];
            m_center[2] = actor.m_trans[2];
        }
        break;
    case 1:
    case 3:
    {
        const float *p = (const float *)arg;
        m_center[0] = p[0];
        m_center[1] = p[1];
        m_center[2] = p[2];
        break;
    }
    default:
        return;
    }
    switch (m_mode)
    {
    case 0:
        m_stepFunc = &TskMoveTypeRound::stepObj;
        chrSetMotion(rate, m_actor, motion, 0, 0x100, oid, NULL);
        break;
    case 1:
        m_stepFunc = &TskMoveTypeRound::stepPoint;
        chrSetMotion(rate, m_actor, motion, 0, 0x100, oid, NULL);
        break;
    case 2:
        m_stepFunc = &TskMoveTypeRound::stepObjMotion;
        m_mtnTask = (TskMtnMove *)chrMtnMove(rate, CHR_ROT_KEEP, oid, motion, -1, 0x100, NULL);
        break;
    case 3:
        m_stepFunc = &TskMoveTypeRound::stepPoint;
        m_mtnTask = (TskMtnMove *)chrMtnMove(rate, CHR_ROT_KEEP, oid, motion, -1, 0x100, NULL);
        break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001eb978

void TskMoveTypeRound::term()
{
}

// ──────────────────────────────────────────────────────────────────────
// The step, then the placing: a hit (for an achr off the secta lists) or
// no cell keeps the last position, flags the achr 0x1000 and the first
// time turns the way round (again once a 60th frame cleared bit 2); a
// free move becomes the last position. The heading goes to the object.
// callcheck: scePrintf (the hit message)
// ROR 0x001eb980

int TskMoveTypeRound::exec(int id, int time, int arg)
{
    u_int hit[0x10];

    (void)id;
    (void)time;
    (void)arg;
    (this->*m_stepFunc)();
    memset(hit, 0, sizeof(hit));  // PORT: placeAchr writes it only on a hit
    int blocked;
    if (g_gamMan.placeAchr((int)m_oid, hit, 0) != 0 && (m_achr->m_flags & 0x40) != 0)
        blocked = 1;
    else
        blocked = !m_achr->trySetPos(m_pos);

    if (blocked)
    {
        scePrintf("HIT in moving task ! oid 0x%x \n", hit[4]);
        if ((m_flags & 1) == 0)
            m_achr->m_flags |= 0x1000;
        m_flags |= 1;
        memcpy(m_pos, m_lastPos, sizeof(m_pos));
        if ((m_flags & 2) == 0)
        {
            m_flags |= 2;
            m_dir = -m_dir;
            m_achr->m_flags &= ~0x1000u;
        }
        else if (g_gamMan.m_vcount % 60 == 0)
        {
            m_flags &= ~2u;
        }
    }
    else
    {
        memcpy(m_lastPos, m_pos, sizeof(m_lastPos));
        m_achr->m_flags &= ~0x1000u;
        if ((m_flags & 1) != 0)
            m_achr->m_flags &= ~0x1000u;
        m_flags &= ~1u;
    }
    m_achr->ObjStat::setRot(m_rot);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Modes 1 / 3, around the point: the heading a quarter turn from the
// centre, plus a push of speed when off the ring by more than 10 (this
// step pushes inward when inside, the object steps outward: a ROM
// quirk). Mode 3 gives the motion task the heading and takes the actor's
// position less the push; mode 1 slides along the heading plus the push.
// callcheck: fptodp, dpadd, dpsub, dpcmp, dptofp (chrQuarterTurn)
// ROR 0x001ebb80

void TskMoveTypeRound::stepPoint()
{
    float dist, px, pz;

    float dir = utlDirDist(m_center[0] - m_pos[0], m_center[2] - m_pos[2], &dist);
    if (dist < m_radius - 10.0f)
    {
        px = sinf(dir) * m_speed;
        pz = cosf(dir) * m_speed;
    }
    else if (m_radius + 10.0f < dist)
    {
        px = -sinf(dir) * m_speed;
        pz = -cosf(dir) * m_speed;
    }
    else
    {
        px = pz = 0.0f;
    }
    float rot = chrQuarterTurn(dir, m_dir);
    m_rot[1] = rot;
    if (m_mode == 3)
    {
        m_mtnTask->m_rotY = rot;
        const DogActor &actor = g_magSystem.m_scene.m_actors[m_actor];
        m_pos[0] = actor.m_trans[0] - px;
        m_pos[1] = actor.m_trans[1];
        m_pos[2] = actor.m_trans[2] - pz;
    }
    else
    {
        m_pos[0] = m_pos[0] + sinf(rot) * m_speed;
        m_pos[0] = m_pos[0] + px;
        m_pos[2] = m_pos[2] + cosf(rot) * m_speed + pz;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Mode 2, around an object by the motion: the centre follows the object;
// off the ring the heading points from the pushed position to the centre
// once (bit 4, cleared each 180th frame), else a quarter turn from it.
// The motion task takes the heading; the position is the actor's.
// callcheck: fptodp, dpadd, dpsub, dpcmp, dptofp (chrQuarterTurn)
// ROR 0x001ebde8

void TskMoveTypeRound::stepObjMotion()
{
    float dist, px, pz;

    if (m_targetActor >= 0)  // PORT: the ROM reads actor -1's slot too
    {
        const DogActor &target = g_magSystem.m_scene.m_actors[m_targetActor];
        m_center[0] = target.m_trans[0];
        m_center[1] = target.m_trans[1];
        m_center[2] = target.m_trans[2];
    }
    float dir = utlDirDist(m_center[0] - m_pos[0], m_center[2] - m_pos[2], &dist);
    if (dist < m_radius - 10.0f)
    {
        px = -sinf(dir) * m_speed;
        pz = -cosf(dir) * m_speed;
    }
    else if (m_radius + 10.0f < dist)
    {
        px = sinf(dir) * m_speed;
        pz = cosf(dir) * m_speed;
    }
    else
    {
        px = pz = 0.0f;
    }
    float rot = chrQuarterTurn(dir, m_dir);
    const DogActor &actor = g_magSystem.m_scene.m_actors[m_actor];
    float x = actor.m_trans[0] + px;
    float z = actor.m_trans[2] + pz;
    if ((px != 0.0f || pz != 0.0f) && (m_flags & 4) == 0)
    {
        rot = utlAtan2(m_center[0] - x, m_center[2] - z);
        m_flags |= 4;
    }
    else if (g_gamMan.m_vcount % 180 == 0)
    {
        m_flags &= ~4u;
    }
    m_rot[1] = rot;
    m_mtnTask->m_rotY = rot;
    m_pos[0] = actor.m_trans[0];
    m_pos[1] = actor.m_trans[1];
    m_pos[2] = actor.m_trans[2];
}

// ──────────────────────────────────────────────────────────────────────
// Mode 0, sliding around an object: the centre follows it; the achr
// slides a quarter turn from the centre plus the push off the ring, and
// faces the centre while pushed both ways (x and z non-zero).
// callcheck: fptodp, dpadd, dpsub, dpcmp, dptofp (chrQuarterTurn)
// ROR 0x001ec0e4

void TskMoveTypeRound::stepObj()
{
    float dist, px, pz;

    if (m_targetActor >= 0)  // PORT: the ROM reads actor -1's slot too
    {
        const DogActor &target = g_magSystem.m_scene.m_actors[m_targetActor];
        m_center[0] = target.m_trans[0];
        m_center[1] = target.m_trans[1];
        m_center[2] = target.m_trans[2];
    }
    float dir = utlDirDist(m_center[0] - m_pos[0], m_center[2] - m_pos[2], &dist);
    if (dist < m_radius - 10.0f)
    {
        px = -sinf(dir) * m_speed;
        pz = -cosf(dir) * m_speed;
    }
    else if (m_radius + 10.0f < dist)
    {
        px = sinf(dir) * m_speed;
        pz = cosf(dir) * m_speed;
    }
    else
    {
        px = pz = 0.0f;
    }
    float rot = chrQuarterTurn(dir, m_dir);
    m_rot[1] = rot;
    m_pos[0] = m_pos[0] + (sinf(rot) * m_speed + px);
    m_pos[2] = m_pos[2] + (cosf(rot) * m_speed + pz);
    if (px != 0.0f && pz != 0.0f)
        m_rot[1] = utlAtan2(m_center[0] - m_pos[0], m_center[2] - m_pos[2]);
}

// ──────────────────────────────────────────────────────────────────────
// The path walk: the achr, its actor's position and heading, the first
// point by mode & 3 (2 / 3 a random one) and its step; modes 0..3 play
// motion at rate here (4..7 start it in aim, through chrMoveFrames).
// callcheck: rand (the random first point), sinf, cosf (aim, tail-called)
// ROR 0x001ec344

TskMovePointPath::TskMovePointPath(float speed, float rate, u_int oid, int mode, int count,
                                   const ChrPathPoint *path, int motion)
{
    m_oid = oid;
    m_achr = g_gamMan.getAchr(oid);
    m_actor = m_achr->m_actor;
    m_mode = mode;
    m_path = path;
    m_count = count;
    m_speed = speed;
    m_motion = motion;
    m_rate = rate;
    m_index = 0;
    m_mtnTask = NULL;
    // PORT: the ROM leaves the w words (and m_vel) unset; sceVu0AddVector
    // adds w too.
    m_pos[3] = m_rot[3] = 0.0f;
    m_vel[0] = m_vel[1] = m_vel[2] = m_vel[3] = 0.0f;
    // PORT: the ROM reads actor -1's slot too; an achr here has an actor.
    if (m_actor >= 0)
    {
        DogActor &actor = g_magSystem.m_scene.m_actors[m_actor];
        m_pos[0] = actor.m_trans[0];
        m_pos[1] = actor.m_trans[1];
        m_pos[2] = actor.m_trans[2];
        m_rot[0] = actor.m_rotate[0];
        m_rot[1] = actor.m_rotate[1];
        m_rot[2] = actor.m_rotate[2];
    }

    // PORT: modes >= 8 (a script global shift > 1) set no step in the ROM,
    // which then calls garbage; they walk the path once here.
    m_stepFunc = &TskMovePointPath::stepOnce;
    switch ((u_int)m_mode)
    {
    case 0: case 4:
        m_point = m_path;
        m_stepFunc = &TskMovePointPath::stepOnce;
        break;
    case 1: case 5:
        m_point = m_path;
        m_stepFunc = &TskMovePointPath::stepLoop;
        break;
    case 2: case 6:
        m_point = m_path + rand() % m_count;
        m_stepFunc = &TskMovePointPath::stepRandom;
        break;
    case 3: case 7:
        m_point = m_path + rand() % m_count;
        m_stepFunc = &TskMovePointPath::stepRandomLoop;
        break;
    default:
        m_point = m_path;  // PORT: unset in the ROM
        break;
    }
    if (m_mode < 4)
        chrSetMotion(m_rate, m_actor, m_motion, 0, 0x100, oid, NULL);
    aim();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ec5d0

void TskMovePointPath::term()
{
}

// ──────────────────────────────────────────────────────────────────────
// Toward the point: modes 0..3 slide by m_vel, 4..7 follow where the
// motion took the achr; a frame counts only when the position has a cell.
// At the point the step picks the next one (re-aimed) or ends the walk
// with motion 1 (heading kept).
// ROR 0x001ec5d8

int TskMovePointPath::exec(int id, int time, int arg)
{
    (void)id;
    (void)time;
    (void)arg;
    if (m_frames > 0)
    {
        if (m_mode < 4)
            sceVu0AddVector(m_pos, m_pos, m_vel);
        else
            m_achr->ObjStat::getPos(m_pos);
        if (m_achr->trySetPos(m_pos))
            m_frames--;
        return 0;
    }
    if ((this->*m_stepFunc)())
    {
        chrMtnMove(m_rate, CHR_ROT_KEEP, m_oid, 1, -1, 0x100, NULL);
        return 1;
    }
    aim();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Mode 0: the next point; after the last one the walk ends.
// ROR 0x001ec6e8

int TskMovePointPath::stepOnce()
{
    if (m_index++ < m_count - 1)
    {
        m_point++;
        return 0;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Mode 1: the next point, after the last one the first again.
// ROR 0x001ec72c

int TskMovePointPath::stepLoop()
{
    if (m_index++ < m_count - 1)
    {
        m_point++;
    }
    else
    {
        m_index = 0;
        m_point = m_path;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Mode 2: count random points (the first included), then the end.
// callcheck: rand
// ROR 0x001ec774

int TskMovePointPath::stepRandom()
{
    if (m_index++ < m_count)
    {
        m_point = m_path + rand() % m_count;
        return 0;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Mode 3: random points for ever.
// callcheck: rand
// ROR 0x001ec7e8

int TskMovePointPath::stepRandomLoop()
{
    m_point = m_path + rand() % m_count;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The direction and distance from the achr to m_point: modes 0..3 face
// it (setObjRot) and slide m_speed a frame; 4..7 hand it to the motion.
// callcheck: sinf, cosf
// ROR 0x001ec834

void TskMovePointPath::aim()
{
    float dist;

    g_gamMan.getObjPos(m_oid, m_pos);
    float dir = utlDirDist(m_point->x - m_pos[0], m_point->z - m_pos[2], &dist);
    if (m_mode < 4)
    {
        m_vel[1] = 0.0f;
        m_vel[0] = sinf(dir) * m_speed;
        m_rot[1] = dir;
        m_vel[2] = cosf(dir) * m_speed;
        m_frames = (int)(dist / m_speed);
        g_gamMan.setObjRot(m_oid, m_rot);
    }
    else
    {
        m_frames = chrMoveFrames(dir, dist, m_rate, m_oid, (u_short)m_actor, &m_mtnTask,
                                 m_motion);
    }
}

// ──────────────────────────────────────────────────────────────────────
// A motion-driven move of dist toward dir: the motion task (started once,
// into *task), then the frames from the motion's root move over 50 x its
// length at rate (x 25; 0 when the motion does not move).
// callcheck: sqrtf (the NaN fallback of sqrt.S)
// ROR 0x001ee7f0

int chrMoveFrames(float dir, float dist, float rate, u_int oid, int no, CmnITask **task,
                  int motion)
{
    float rot[4] = {dir, 0.0f, 0.0f, 0.0f};  // PORT: the ROM stores only rot[0]
    float move[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // PORT: read even if the actor has no motion

    no &= 0xffff;
    if (*task == NULL)
        *task = chrMtnMove(rate, dir, oid, motion, -1, 0x100, NULL);
    float len = g_magSystem.m_scene.getActorMotionLength(no, motion);
    g_magSystem.getActorMotionMove(no, 0.0f, len * 50.0f, move, rot, NULL);
    float speed = sqrtf(move[0] * move[0] + move[2] * move[2]) * (rate / 0x1.11111p-6f);
    float t;
    if (speed <= 0.0f)
        t = 0.0f * len;
    else
        t = dist / speed * len;
    return (int)(t * 25.0f);
}

// ──────────────────────────────────────────────────────────────────────
// The turn: angle wrapped (still outside -pi..pi: no turn, at once), the
// heading (rotY >= 32769 the actor's), the achr's turn motion when it has
// flag 0x80000 (param 0x1102 turning left, 0x1101 right). No motion: the
// turn over frames (frames < 1: set now); a motion: it plays at speed /
// 60 toward the target heading.
// callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (libgcc soft double: chrWrapPi and the -pi..pi checks)
// ROR 0x001f0434

TskChrRot::TskChrRot(float angle, float rotY, float speed, u_int oid, int frames, int motion,
                     int restore, int unused)
{
    (void)unused;
    m_oid = oid;
    m_actor = g_gamMan.getActorNo(oid);
    m_angle = chrWrapPi(angle);
    m_restore = restore;
    m_achr = g_gamMan.getAchr(m_oid);
    m_motion = motion;
    m_motionTask = NULL;
    // PORT: the ROM forms the address for actor -1 too; it is only read
    // below for an achr, which has an actor.
    m_dogActor = m_actor >= 0 ? &g_magSystem.m_scene.m_actors[m_actor] : NULL;

    u_int flags = m_achr->m_flags;
    if ((double)m_angle < -CHR_PI || (double)m_angle > CHR_PI)
    {
        m_angle = 0.0f;
        frames = 0;
    }
    if (rotY >= CHR_ROT_KEEP)
        rotY = m_dogActor->getRotY();
    m_rot[0] = 0.0f;
    m_rot[1] = rotY;
    m_rot[2] = 0.0f;
    m_rot[3] = 0.0f;
    if ((flags & 0x80000) != 0)
        m_achr->getParam(angle < 0.0f ? 0x1102 : 0x1101, &m_motion);

    if (m_motion == -1)
    {
        if (frames < 1)
        {
            m_rot[1] = chrWrapPi(m_rot[1] + m_angle);
            g_magSystem.setActorRotate(m_actor, m_rot);
            m_step = 0.0f;
            m_frames = 0;
            m_count = 0;
        }
        else
        {
            m_frames = frames - 1;
            m_count = frames - 1;
            m_step = m_angle / (float)frames;
        }
        m_stepFunc = &TskChrRot::stepFrames;
    }
    else
    {
        float from = m_rot[1];
        m_speed = speed * 0x1.11111p-6f; /* 0x3c888888 */
        m_angle = chrWrapPi(from + m_angle);
        m_step = 0.0f;
        // The ROM passes from in f13 as well; chrMtnMove2 does not read it.
        m_motionTask = chrMtnMove2(m_speed, oid, m_motion, 0, 0x100, NULL);
        m_stepFunc = &TskChrRot::stepMotion;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The step function the constructor picked.
// ROR 0x001f08a4

int TskChrRot::exec(int id, int time, int arg)
{
    (void)id;
    (void)time;
    (void)arg;
    return (this->*m_stepFunc)();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f089c

void TskChrRot::term()
{
}

// ──────────────────────────────────────────────────────────────────────
// One frame of the turn: the heading moves by m_step and goes to the
// object; a running motion task (m_task[2]) gets it as its heading. Ends
// after m_frames + 1 frames.
// callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (libgcc soft double: chrWrapPi / chrNormPi)
// ROR 0x001f090c

int TskChrRot::stepFrames()
{
    if (m_count-- < 0)
        return 1;
    m_rot[1] = chrWrapPi(m_rot[1] + m_step);
    g_gamMan.setObjRot(m_oid, m_rot);

    int *id = &m_achr->m_task[2];
    if (g_taskMan.getTask(*id) == 0)
        return 0;
    m_motionTask = g_taskMan.findTask(*id);
    m_rot[1] = chrNormPi(m_rot[1]);
    // The ROM stores the word at +0x6c of whatever task it is: TskMtnMove's
    // m_rotY; a TskMtnMove2 (native 100's) has its m_step there.
    if (TskMtnMove *move = dynamic_cast<TskMtnMove *>(m_motionTask))
        move->m_rotY = m_rot[1];
    else if (TskMtnMove2 *move2 = dynamic_cast<TskMtnMove2 *>(m_motionTask))
        move2->m_step = m_rot[1];
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The turn motion runs until the actor's heading is within 0.1 of the
// target; then motion 1 again (m_restore), the motion task killed (the
// ROM kills the one chrMtnMove2 just started: the motion change stays,
// the moving stops) and the heading set.
// callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (libgcc soft double: chrWrapPi)
// ROR 0x001f0b50

int TskChrRot::stepMotion()
{
    const float near = 0x1.999998p-4f; /* 0x3dcccccc */
    float rotY = m_dogActor->getRotY();
    float d = chrWrapPi(rotY - m_angle);

    if (d < 0.0f)
    {
        if (d < -near)
            return 0;
    }
    else if (near < d)
    {
        return 0;
    }
    if (m_restore != 0)
        chrMtnMove2(0x1.11111p-6f, m_oid, 1, 0, 0, NULL);

    int *id = &m_achr->m_task[2];
    if (g_taskMan.getTask(*id) != 0)
    {
        g_taskMan.killTask(*id);
        *id = -1;
    }
    m_rot[1] = rotY;
    m_rot[3] = 1.0f;
    m_rot[0] = 0.0f;
    m_rot[2] = 0.0f;
    g_gamMan.setObjRot(m_oid, m_rot);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Kills the achr's turn task (m_task[1]) and starts a new one turning by
// angle over frames (none for 0).
// ROR 0x001f0e90

void chrRotStart(float angle, u_int oid, int frames)
{
    ACHRstat *chr = g_gamMan.getAchr(oid);
    int      *id = &chr->m_task[1];

    if (g_taskMan.getTask(*id) != 0)
    {
        g_taskMan.killTask(*id);
        *id = -1;
    }
    if (angle == 0.0f)
        return;
    // callcheck: operator_new (the new expression)
    TskChrRot *task = new TskChrRot(angle, CHR_ROT_KEEP, 1.0f, oid, frames, -1, 1, 1);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, id, 0x11);
}

// ──────────────────────────────────────────────────────────────────────
// A turn to the heading dir at speed (chrRotFrames frames).
// callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (libgcc soft double: chrWrapPi)
// ROR 0x001f0f9c

void chrRotTo(float dir, float speed, u_int oid)
{
    sceVu0FVECTOR rot;

    g_gamMan.getObjRot(oid, rot);
    if (rot[1] == dir)
        return;
    float angle = chrWrapPi(dir - rot[1]);
    chrRotStart(angle, oid, chrRotFrames(angle, speed));
}

// ──────────────────────────────────────────────────────────────────────
// A turn to the heading dir over frames.
// callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (libgcc soft double: chrWrapPi)
// ROR 0x001f10a4

void chrRotToIn(float dir, u_int oid, int frames)
{
    sceVu0FVECTOR rot;

    g_gamMan.getObjRot(oid, rot);
    if (rot[1] == dir)
        return;
    chrRotStart(chrWrapPi(dir - rot[1]), oid, frames);
}

// ──────────────────────────────────────────────────────────────────────
// A turn from the heading from to the heading to at speed (chrRotFrames
// frames).
// callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (libgcc soft double: chrWrapPi)
// ROR 0x001f11a0

void chrRotFromTo(float from, float to, float speed, u_int oid)
{
    if (from == to)
        return;
    float angle = chrWrapPi(to - from);
    chrRotStartFrom(angle, from, oid, chrRotFrames(angle, speed));
}

// ──────────────────────────────────────────────────────────────────────
// chrRotStart from the heading rotY instead of the actor's.
// ROR 0x001f1298

void chrRotStartFrom(float angle, float rotY, u_int oid, int frames)
{
    ACHRstat *chr = g_gamMan.getAchr(oid);
    int      *id = &chr->m_task[1];

    if (g_taskMan.getTask(*id) != 0)
    {
        g_taskMan.killTask(*id);
        *id = -1;
    }
    if (angle == 0.0f)
        return;
    // callcheck: operator_new (the new expression)
    TskChrRot *task = new TskChrRot(angle, rotY, 1.0f, oid, frames, -1, 1, 1);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, id, 0x11);
}

// ──────────────────────────────────────────────────────────────────────
// A turn by angle from the heading rotY played by the turn motion.
// ROR 0x001f13a8

void chrRotMotion(float angle, float speed, float rotY, u_int oid, int motion, int restore)
{
    ACHRstat *chr = g_gamMan.getAchr(oid);
    int      *id = &chr->m_task[1];

    if (g_taskMan.getTask(*id) != 0)
    {
        g_taskMan.killTask(*id);
        *id = -1;
    }
    if (angle == 0.0f)
        return;
    // callcheck: operator_new (the new expression)
    TskChrRot *task = new TskChrRot(angle, rotY, speed, oid, 0, motion, restore, 1);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, id, 0x11);
}

// ──────────────────────────────────────────────────────────────────────
// chrRotMotion with motionNeg for a negative angle, motionPos else. No
// callers in the ROM.
// ROR 0x001f14cc

void chrRotMotionBySign(float angle, float speed, float rotY, u_int oid, int motionNeg,
                        int motionPos, int restore)
{
    ACHRstat *chr = g_gamMan.getAchr(oid);
    int      *id = &chr->m_task[1];

    if (g_taskMan.getTask(*id) != 0)
    {
        g_taskMan.killTask(*id);
        *id = -1;
    }
    if (angle == 0.0f)
        return;
    int motion = angle < 0.0f ? motionNeg : motionPos;
    // callcheck: operator_new (the new expressions of both branches)
    TskChrRot *task = new TskChrRot(angle, rotY, speed, oid, 0, motion, restore, 1);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, id, 0x11);
}

// ──────────────────────────────────────────────────────────────────────
// A turn to the heading dir from rotY (>= 32769: the object's) played by
// the turn motion.
// callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (libgcc soft double: chrWrapPi)
// ROR 0x001f1670

void chrRotToMotion(float dir, float speed, float rotY, u_int oid, int motion, int restore)
{
    ACHRstat *chr = g_gamMan.getAchr(oid);
    int      *id = &chr->m_task[1];

    if (g_taskMan.getTask(*id) != 0)
    {
        g_taskMan.killTask(*id);
        *id = -1;
    }
    if (rotY >= CHR_ROT_KEEP)
    {
        sceVu0FVECTOR rot;
        g_gamMan.getObjRot(oid, rot);
        rotY = rot[1];
    }
    float angle = chrWrapPi(dir - rotY);
    if (angle == 0.0f)
        return;
    // callcheck: operator_new (the new expression)
    TskChrRot *task = new TskChrRot(angle, rotY, speed, oid, 0, motion, restore, 1);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, id, 0x11);
}

// ──────────────────────────────────────────────────────────────────────
// TskFrameFunc: the constructor's switch on kind (jump table data
// 0x2fe420) copies the job's words and picks its step (member pointers
// data 0x2fe2c0..). Kind 7 (a door opening) does its work here: the PC's
// door motion by the side it stands on, the camera set out along the
// door's heading and looking back, the PC placed in front of the door.
// callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (libgcc soft double: chrWrapPi and kind 7's angle - pi / 2, angle - pi)
// callcheck: operator_new (new MnuTextList)
// ROR 0x001f184c

TskFrameFunc::TskFrameFunc(int kind, intptr_t arg, int frames)
{
    GamMan   &gam = g_gamMan;
    MagScene &scene = g_magSystem.m_scene;

    m_kind = kind;
    m_state = 0;
    m_count = 0;        /* PORT: the ROM leaves it to the kinds that use it */
    m_stepFunc = NULL;  /* PORT: the ROM leaves it unset for the kinds without a step */
    m_list = NULL;      /* PORT: only kind 8 sets it; term reads it for kind 8 only */
    switch (kind)
    {
    case 2:
        m_word[0] = *(const int *)arg;
        m_stepFunc = &TskFrameFunc::stepKeyIn1;
        break;
    case 3:
        m_word[0] = *(const int *)arg;
        m_stepFunc = &TskFrameFunc::stepKeyOut1;
        break;
    case 4:
        m_word[0] = *(const int *)arg;
        m_stepFunc = &TskFrameFunc::stepKeyIn2;
        break;
    case 5:
        m_word[0] = *(const int *)arg;
        m_stepFunc = &TskFrameFunc::stepKeyOut2;
        break;
    case 6:
        m_stepFunc = &TskFrameFunc::stepSysMsg;
        gam.pause(0x100000);
        m_state = arg != 0 ? 200 : 100;
        g_sequence.m_global[0x15] = 0;
        break;
    case 7:
    {
        const int   *w = (const int *)arg;
        const float *f = (const float *)arg;
        DogActor    &pc = scene.m_actors[gam.m_pcActor];
        int          mtnR, mtnO, mtnL, mtnT;

        if (gam.m_pcJnfc == 2)
        {
            mtnR = pc.findMotionByName("jnfc_doorr_open__x");
            mtnO = pc.findMotionByName("jnfc_dooro_open__y");
            mtnL = pc.findMotionByName("jnfc_doorl_open__x");
            mtnT = pc.findMotionByName("jnfc_doort_open__y");
        }
        else
        {
            mtnR = pc.findMotionByName("pc_doorr_open__x");
            mtnO = pc.findMotionByName("pc_dooro_open__y");
            mtnL = pc.findMotionByName("pc_doorl_open__x");
            mtnT = pc.findMotionByName("pc_doort_open__y");
        }

        u_int oid = (u_int)w[0];
        float angle = f[1];
        float dist = f[2];
        float height = f[3];
        float rotX = f[4];
        short objKind = gam.getObjKind(oid);
        sceVu0FVECTOR pos;
        gam.getObjPos(oid, pos);
        float side = chrWrapPi((float)((double)angle - CHR_PI / 2));
        float back = chrWrapPi((float)((double)angle - CHR_PI));
        gam.setCameraMode(1, 0);

        // The camera: dist out along angle, height up, turned to back.
        float eyeX = pos[0] + sinf(angle) * dist;
        float eyeZ = pos[2] + cosf(angle) * dist;
        float eyeY = pos[1] + height;
        DogCamera &cam = scene.m_camera_entity;
        cam.m_dirty = 1;
        cam.m_trans[1] = eyeY;
        cam.m_trans[0] = eyeX;
        cam.m_trans[2] = eyeZ;
        cam.m_rotate[0] = rotX;
        cam.m_rotate[1] = back;
        cam.m_rotate[2] = 0.0f;
        cam.normalizeRotate();
        cam.m_dirty = 1;

        sceVu0FVECTOR rot = {rotX, back, 0.0f, 0.0f};
        gam.getObjRot(oid, rot);

        // The side: the door's heading against back. (For kind 0x1201 the
        // ROM first picks o / t by |back - rot| - 0.1 > pi and overwrites
        // the choice at once; left out.)
        sceVu0FVECTOR to;
        float d = chrWrapPi(back - rot[1]);
        int   near = d < 0.0f ? -0.1f < d : d < 0.1f;
        if (objKind == 0x1201)
        {
            memcpy(to, pos, sizeof(to));
            utlAddPolar(angle, -50.0f, to);
            memcpy(gam.m_sysWord, to, sizeof(to));
            if (near)
            {
                m_word[1] = mtnO;
                rxSysPrintf("<<1>>\n");
                to[0] = pos[0] + sinf(angle) * 50.0f;
                to[0] -= sinf(side) * -25.0f;
                to[2] = pos[2] + cosf(angle) * 50.0f;
                to[2] -= cosf(side) * -25.0f;
            }
            else
            {
                m_word[1] = mtnT;
                rxSysPrintf("<<2>>\n");
                to[0] = pos[0] + sinf(angle) * 50.0f;
                to[0] += sinf(side) * -25.0f;
                to[2] = pos[2] + cosf(angle) * 50.0f;
                to[2] += cosf(side) * -25.0f;
            }
        }
        else
        {
            to[0] = pos[0];
            to[1] = pos[1];
            to[2] = pos[2];
            to[3] = 0.0f;
            memcpy(gam.m_sysWord, to, sizeof(to));
            float off;
            if (near)
            {
                m_word[1] = mtnR;
                off = 25.0f;
            }
            else
            {
                m_word[1] = mtnL;
                off = -25.0f;
            }
            to[0] = pos[0] + sinf(angle) * 50.0f;
            to[0] += sinf(side) * off;
            to[2] = pos[2] + cosf(angle) * 50.0f;
            to[2] += cosf(side) * off;
        }

        // The PC turns to back, plays the door motion and stands at to;
        // the door's actor plays its motion 2.
        rot[0] = 0.0f;
        rot[1] = back;
        rot[2] = 0.0f;
        rot[3] = 0.0f;
        m_word[0] = gam.m_pcPtr != NULL ? gam.m_pcPtr->getOID() : 0;
        gam.setPcRot(rot);
        gam.setObj(gam.m_pcPtr != NULL ? (u_int)gam.m_pcPtr->getOID() : 0, 0x80);
        chrSetMotion(0x1.11111p-6f /* 0x3c888888 */, gam.getActorNo(oid), 2, (int)oid, 0, 0, NULL);
        // (The ROM also passes back in f13; chrMtnMove2 does not read it.)
        chrMtnMove2(0x1.11111p-6f, (u_int)m_word[0], m_word[1], 0, 0, NULL);
        to[1] = pos[1];
        gam.setPcPos(to);
        m_count = 0;
        m_state = 100;
        m_stepFunc = &TskFrameFunc::stepWaitAchrMotion;
        break;
    }
    case 8:
    {
        const intptr_t *w = (const intptr_t *)arg;
        m_list = new MnuTextList();
        m_list->open();
        m_list->m_x = (int)w[2];
        m_list->m_y = (int)w[3];
        m_list->m_z = 0x8001;
        m_list->setItems((const char *)w[0], (u_int)w[1]);
        m_stepFunc = &TskFrameFunc::stepTextList;
        break;
    }
    case 9:
        m_stepFunc = &TskFrameFunc::stepMenuPause;
        break;
    case 10:
        m_stepFunc = &TskFrameFunc::stepClearPcRot;
        break;
    case 11:
    {
        const int *w = (const int *)arg;
        m_word[0] = w[0];
        m_word[1] = w[1];
        m_word[2] = w[2];
        m_word[3] = w[3];
        m_stepFunc = &TskFrameFunc::stepStageThread;
        break;
    }
    case 12:
        m_fog[0] = scene.m_fog[0];
        m_fog[1] = *(const I3dFog *)arg;
        m_frames = frames;
        m_count = 0;
        m_stepFunc = &TskFrameFunc::stepFog;
        break;
    case 14:
        m_word[2] = (int)arg;
        m_state = 100;
        m_stepFunc = &TskFrameFunc::stepGameOver;
        break;
    case 15:
        m_word[9] = frames;
        m_state = 100;
        m_word[8] = 0;
        // PORT: the ROM copies up to 0x100 chars over the task's words;
        // the names are short, kept to the 0x20 bytes before m_word[8].
        utlStrNCopy(m_name, (const char *)arg, (int)sizeof(m_name));
        m_stepFunc = &TskFrameFunc::stepBg;
        break;
    case 16:
        m_word[9] = frames;
        m_state = 200;
        m_stepFunc = &TskFrameFunc::stepBg;
        break;
    case 0x11:
        m_stepFunc = &TskFrameFunc::stepClearFlag100;
        break;
    case 0x12:
        m_word[0] = frames;
        m_state = 100;
        m_stepFunc = &TskFrameFunc::stepNone;
        break;
    case 0x13:
    {
        const int *w = (const int *)arg;
        m_word[0] = w[0];
        m_word[1] = w[1];
        m_word[3] = w[2];
        if (w[0] == 0xa64)
            m_state = 300;
        else if (gam.countItems() == 12)
            m_state = m_word[3] == 0 ? 300 : 200;
        else
            m_state = m_word[3] != 0 ? 100 : 0x96;
        m_stepFunc = &TskFrameFunc::stepGetItem;
        break;
    }
    case 0x15:
        m_stepFunc = &TskFrameFunc::stepPcMsg;
        break;
    case 0x16:
        m_stepFunc = &TskFrameFunc::stepClearFlag400000;
        break;
    default:    // 0, 1, 13, 20, >= 23: no step
        break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// One step of the job; non-zero ends the task.
// ROR 0x001f2778

int TskFrameFunc::exec(int, int, int)
{
    if (m_stepFunc == NULL)
        return 1;   /* PORT: the ROM calls an unset member pointer */
    return (this->*m_stepFunc)();
}

// ──────────────────────────────────────────────────────────────────────
// Kind 8 closes and frees its list.
// callcheck: ~MnuTextList (delete m_list)
// ROR 0x001f2718

void TskFrameFunc::term()
{
    if (m_kind == 8 && m_list != NULL)
    {
        m_list->close();
        delete m_list;
        m_list = NULL;
    }
}

/* MagScene::setFade with kind, mode, time (the ROM's stack record sets
 * only these three words). */
static void tskSetFade(int kind, int mode, int time)
{
    MagFadeParam p;
    p.m_kind = kind;
    p.m_mode = mode;
    p.m_time = time;
    p.m_0c = 0;
    p.m_10 = 0;
    p.m_level = 7;  /* PORT: an unset stack word in the ROM; 7 as MagScene::update passes */
    g_magSystem.m_scene.setFade(&p);
}

// ──────────────────────────────────────────────────────────────────────
// Kind 2: fades out over half the frames, then the keyhole (fade kind 1)
// opens over the other half.
// callcheck: setFade (in tskSetFade)
// ROR 0x001f27e0

int TskFrameFunc::stepKeyIn1()
{
    MagScene &scene = g_magSystem.m_scene;

    if (m_state == 1)
    {
        if (--m_count != -1)
            return 0;
        scene.fadeIn(0, 7, 0);
        m_count = m_word[0] / 2;
        tskSetFade(1, 2, m_count);
        m_state = 99;
    }
    else if (m_state == 99)
    {
        return --m_count == -1;
    }
    else if (m_state == 0)
    {
        m_count = m_word[0] / 2;
        scene.fadeOut(m_count, 7, 0);
        m_state++;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 3: the keyhole (kind 1) closes over a quarter of the frames; after
// half of them it is taken off and the scene fades in.
// callcheck: setFade (in tskSetFade)
// ROR 0x001f2920

int TskFrameFunc::stepKeyOut1()
{
    MagScene &scene = g_magSystem.m_scene;

    if (m_state == 1)
    {
        if (m_count-- > 0)
            return 0;
        m_count = m_word[0] / 2;
        tskSetFade(1, 0, 0);
        scene.fadeIn(m_count, 7, 0);
        m_state = 99;
    }
    else if (m_state == 99)
    {
        return m_count-- < 1;
    }
    else if (m_state == 0)
    {
        tskSetFade(1, 6, m_word[0] / 4);
        m_state++;
        m_count = m_word[0] / 2;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 4: kind 2 with the keyhole kind 2.
// callcheck: setFade (in tskSetFade)
// ROR 0x001f2a84

int TskFrameFunc::stepKeyIn2()
{
    MagScene &scene = g_magSystem.m_scene;

    if (m_state == 1)
    {
        if (m_count-- > 0)
            return 0;
        scene.fadeIn(0, 7, 0);
        m_count = m_word[0] / 2;
        tskSetFade(2, 2, m_count);
        m_state = 99;
    }
    else if (m_state == 99)
    {
        return m_count-- < 1;
    }
    else if (m_state == 0)
    {
        m_count = m_word[0] / 2;
        scene.fadeOut(m_count, 7, 0);
        m_state++;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 5: kind 3 with the keyhole kind 2.
// callcheck: setFade (in tskSetFade)
// ROR 0x001f2bcc

int TskFrameFunc::stepKeyOut2()
{
    MagScene &scene = g_magSystem.m_scene;

    if (m_state == 1)
    {
        if (m_count-- > 0)
            return 0;
        m_count = m_word[0] / 2;
        tskSetFade(2, 0, 0);
        scene.fadeIn(m_count, 7, 0);
        m_state = 99;
    }
    else if (m_state == 99)
    {
        return m_count-- < 1;
    }
    else if (m_state == 0)
    {
        tskSetFade(2, 6, m_word[0] / 4);
        m_state++;
        m_count = m_word[0] / 2;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 6: system message 0xd (state 100) or 0xe (200) over a fade in,
// held until it is read; then the game resumes (0xd also pauses 0x8000).
// ROR 0x001f2d38

int TskFrameFunc::stepSysMsg()
{
    GamMan &gam = g_gamMan;

    switch (m_state)
    {
    case 100:
    case 200:
        g_magSystem.m_scene.fadeIn(10, 7, 0);
        gam.openSysMsg(m_state == 100 ? 0xd : 0xe, -1, -1, -1, -1);
        if (gam.isMsgBusy())
            m_state++;
        return 0;
    case 0x65:
    case 0xc9:
        if (gam.isMsgBusy() == 0)
            m_state++;
        return 0;
    case 0x66:
        gam.resume(0x100000);
        gam.pause(0x8000);
        return 1;
    case 0xca:
        gam.resume(0x100000);
        return 1;
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Kind 7: until the PC's door motion is over.
// ROR 0x001f2ec4

int TskFrameFunc::stepWaitAchrMotion()
{
    if (m_state == 100)
        return g_gamMan.isAchrMotionEnd((u_int)m_word[0]) != 0;
    m_count++;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 14, the game over: halt and fade out (100), turn the PC towards
// open floor unless the cause m_word[2] is 0xb (0x65), after 0x77 frames
// reset the game (0x66), fade in on the DEATH_J.PSS film (0x67), fade
// out once it ends (0x68), then stage 99 -- the title (0x69).
// callcheck: operator_new (new TskMovie), memset
// callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (libgcc soft double: chrWrapPi)
// ROR 0x001f2f0c

int TskFrameFunc::stepGameOver()
{
    GamMan   &gam = g_gamMan;
    MagScene &scene = g_magSystem.m_scene;
    char      film[0x40];
    char      other[0x40];

    // Both names are built each step; only the first is used.
    memset(film, 0, sizeof(film));
    strcpy(film, "DEATH_J.PSS");
    memset(other, 0, sizeof(other));
    strcpy(other, "DEATH_B.PSS");
    (void)other;

    switch (m_state)
    {
    case 100:
        gam.pause(0x100000);
        m_count = 1;
        scene.fadeOut(0x78, 7, 0);
        m_state = 0x65;
        return 0;
    case 0x65:
    {
        m_count = 0x77;
        m_state = 0x66;
        ObjStat *pc = gam.m_pcPtr;
        float    z = gam.m_pcPos[2];
        if (pc == NULL || m_word[2] == 0xb)
            return 0;
        float x = gam.m_pcPos[0];
        float dir = gam.m_pcRot[1];
        chrMtnMove2(0x1.11111p-6f, (u_int)pc->getOID(), 0x2c, 0x1e, 0, NULL);
        // No floor 50 and 140 ahead: turn to the first heading (steps of
        // pi / 8) that has both.
        sceVu0FVECTOR at;
        at[1] = gam.m_pcPos[1];
        at[3] = 0.0f;   /* PORT: not set by the ROM (hasSectaAt reads x, z) */
        at[0] = x + sinf(dir) * 50.0f;
        at[2] = z + cosf(dir) * 50.0f;
        if (pc->hasSectaAt(at) != 0)
            return 0;
        at[0] = x + sinf(dir) * 140.0f;
        at[2] = z + cosf(dir) * 140.0f;
        if (pc->hasSectaAt(at) != 0)
            return 0;
        for (int i = 0; i < 15; i++)
        {
            dir = chrWrapPi(dir + 0.39269906f);
            at[0] = x + sinf(dir) * 50.0f;
            at[2] = z + cosf(dir) * 50.0f;
            if (pc->hasSectaAt(at) == 0)
                continue;
            at[0] = x + sinf(dir) * 140.0f;
            at[2] = z + cosf(dir) * 140.0f;
            if (pc->hasSectaAt(at) != 0)
            {
                chrRotTo(dir, 1.0f, gam.m_pcPtr != NULL ? (u_int)gam.m_pcPtr->getOID() : 0);
                return 0;
            }
        }
        return 0;
    }
    case 0x66:
        if (m_count-- > 0)
            return 0;
        g_taskMan.pauseOthers(1);
        scene.m_837e0.releaseAll();
        gam.resetGame();
        m_count = 0;
        m_state = 0x67;
        return 0;
    case 0x67:
    {
        if (m_count-- > 0)
            return 0;
        m_count = 0x28;
        scene.setFocus(0);
        scene.fadeIn(m_count, 7, 0);
        TskMovie *task = new TskMovie(film);
        g_taskMan.addTask(1, -1, 1, 0, task, 0, &m_word[0], 0x11);
        m_state = 0x68;
        return 0;
    }
    case 0x68:
        if (g_taskMan.getTask(m_word[0]) != 0)
            return 0;
        m_count = 0x3c;
        scene.fadeOut(0x3c, 7, 0);
        m_state = 0x69;
        return 0;
    case 0x69:
        if (m_count-- < 1)
            return 0;
        gam.requestStage(1, -1, -1, 99, 0xccccccccu);
        m_state = 999;
        return 0;
    case 999:
        return 1;
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Kinds 15 / 16: S2dBg picture 0. 15 walks its requests (init, the
// texture name, load, display) and shows it over m_word[9] frames; 16
// takes it off over m_word[9] frames and closes.
// ROR 0x001f34c8

int TskFrameFunc::stepBg()
{
    S2dBg &bg = g_magSystem.m_scene.m_bg;

    switch (m_state)
    {
    case 100:
        if (bg.s2dBgInit() == 0)
            return 0;
        m_state = 0x65;
        return 0;
    case 0x65:
        if (!bg.s2dBgInitStat())
            return 0;
        bg.setTexName(0, m_name);
        m_state = 0x66;
        return 0;
    case 0x66:
        if (bg.s2dBgLoadStart() == 0)
            return 0;
        m_state = 0x67;
        return 0;
    case 0x67:
        if (!bg.s2dBgLoadStat())
            return 0;
        m_state = 0x68;
        return 0;
    case 0x68:
        bg.s2dBgDispStart();
        bg.dispIn(0, m_word[9]);
        return 1;
    case 200:
        bg.dispOut(0, m_word[9]);
        m_state = 0xc9;
        m_count = m_word[9];
        return 0;
    case 0xc9:
        if (m_count-- >= 1)
            return 0;
        bg.s2dBgClose();
        return 1;
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Kind 8: the list until a row is chosen; the row into script global
// 0x2d.
// ROR 0x001f370c

int TskFrameFunc::stepTextList()
{
    int done = m_list->processPad(&MagSystem::m_current_system->m_pad[0]);
    if (done != 0)
        g_sequence.m_global[0x2d] = m_list->getSelect();
    return done != 0;
}

// ROR 0x001f3760

int TskFrameFunc::stepMenuPause()
{
    g_gamMan.enterMenuPause(1);
    return 1;
}

// ROR 0x001f3788

int TskFrameFunc::stepClearFlag100()
{
    g_gamMan.m_flags2 &= ~0x100u;
    return 1;
}

// ROR 0x001f37b8

int TskFrameFunc::stepClearFlag400000()
{
    GamMan &gam = g_gamMan;

    if ((gam.m_flags & 4) != 0)
        gam.m_flags2 &= ~0x800u;
    gam.m_flags2 &= ~0x400000u;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 0x15 (GamMan::enterMenuPause, system call 0x14): with m_flags2
// 0x80000 and the PC in state class 5..7, halt, PC motion 1 and system
// window 0xb (0x16 with 0x40000) until it closes; the flags 0xc0000 are
// cleared either way.
// ROR 0x001f3824

int TskFrameFunc::stepPcMsg()
{
    GamMan   &gam = g_gamMan;
    MagScene &scene = g_magSystem.m_scene;
    int       args[16];
    u_int     cls;

    switch (m_state)
    {
    case 0:
        if ((gam.m_flags2 & 0x80000) == 0)
            return 1;
        cls = (u_int)g_magSystem.getPCStateClass();
        if (cls - 5 > 2)
        {
            gam.m_flags2 &= 0xfff3ffffu;
            return 1;
        }
        m_state = 1;
        return 0;
    case 1:
        if (gam.isFading())
            return 0;
        cls = (u_int)g_magSystem.getPCStateClass();
        if (cls - 5 > 2)
        {
            gam.m_flags2 &= 0xfff3ffffu;
            return 1;
        }
        gam.pause(0x100000);
        // (The ROM passes rotY 32769 -- keep -- in f13; chrMtnMove2 has none.)
        chrMtnMove2(0x1.11111p-6f, gam.m_pcPtr != NULL ? (u_int)gam.m_pcPtr->getOID() : 0, 1,
                    0x1e, 0, NULL);
        memset(args, 0, sizeof(args));
        m_word[2] = scene.openSysMsgWindow((gam.m_flags2 & 0x40000) != 0 ? 0x16 : 0xb, args, 1);
        gam.m_flags2 &= 0xfff3ffffu;
        m_state = 3;
        m_count = 1;
        return 0;
    case 2:     // not reached: state 1 goes to 3
        if (scene.isWindowOpen(m_word[2]))
            m_state = 3;
        return 0;
    case 3:
        if (m_count-- > 0)
        {
            scene.m_mes.setSpriteProcessOn(2);
            scene.m_mes.setSpriteColorStd(2);
            scene.m_mes.setSpriteState(2, 1);
            scene.m_mes.setSpriteDisp(2, 1);
            scene.m_mes.setSpriteAnim(2, 2);
        }
        if (scene.isWindowOpen(m_word[2]))
            return 0;
        m_state = 4;
        return 0;
    case 4:
        if ((gam.m_flags2 & 8) != 0)
            return 0;
        gam.killMsgTask();
        gam.resume(0x100000);
        return 1;
    default:
        return 0;
    }
}

// ROR 0x001f3b3c

int TskFrameFunc::stepClearPcRot()
{
    sceVu0FVECTOR rot;
    memset(rot, 0, sizeof(rot));
    g_gamMan.setPcRot(rot);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 11 (native 29 with a system call 0x34 pair): runs the pair's
// thread and, once it ends, changes to room m_word[2]; the pair is used
// up.
// ROR 0x001f3b74

int TskFrameFunc::stepStageThread()
{
    GamMan &gam = g_gamMan;

    if (m_state == 0)
    {
        g_sequence.queueSingle((u_int)m_word[0], m_word[1], 0, 0);
        m_state++;
    }
    else if (m_state == 1)
    {
        if (g_sequence.hasThread((u_int)m_word[0], 0) == 0)
        {
            gam.requestStage(2, m_word[2], 0, -1, (u_int)m_word[3]);
            gam.m_seqPair[3].m_b = 0;
            gam.m_seqPair[3].m_a = 0;
            return 1;
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 12: the fog moves from m_fog[0] to m_fog[1] over m_frames frames.
// ROR 0x001f3c44

int TskFrameFunc::stepFog()
{
    int n = m_count;
    int frames = m_frames;

    if (n < frames)
    {
        float  t = (float)n / (float)frames;
        float  u = 1.0f - t;
        I3dFog fog;
        for (int i = 0; i < 4; i++)
            fog.m_color[i] = m_fog[1].m_color[i] * t + m_fog[0].m_color[i] * u;
        for (int i = 0; i < 4; i++)
            fog.m_param[i] = m_fog[1].m_param[i] * t + m_fog[0].m_param[i] * u;
        g_magSystem.m_scene.setFog(&fog, 0);
        m_count++;
    }
    return n >= frames;
}

// ROR 0x001f3d90

int TskFrameFunc::stepNone()
{
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 0x13, an item found: window 0x11, then the item is taken (100;
// 0x96 takes it at once), or -- twelve items held -- window 0x13 and the
// item menu before it is taken (200); 300 ends at once.
// ROR 0x001f3d98

int TskFrameFunc::stepGetItem()
{
    GamMan   &gam = g_gamMan;
    MagScene &scene = g_magSystem.m_scene;
    int       args[16];

    switch (m_state)
    {
    case 100:
        memset(args, 0, sizeof(args));
        gam.m_sndMan.playSE(0x11, false, NULL, 0);
        args[0] = (u_short)m_word[0];
        args[1] = 1;
        m_word[2] = scene.openSysMsgWindow(0x11, args, 1);
        m_state = 0x65;
        return 0;
    case 0x65:
        if (scene.isWindowOpen(m_word[2]))
            return 0;
        gam.addItem((short)m_word[0], 0);
        return 1;
    case 0x96:
        gam.addItem((short)m_word[0], 0);
        return 1;
    case 200:
        memset(args, 0, sizeof(args));
        gam.m_sndMan.playSE(0x11, false, NULL, 0);
        args[0] = (u_short)m_word[0];
        m_word[2] = scene.openSysMsgWindow(0x13, args, 1);
        m_state = 0xc9;
        return 0;
    case 0xc9:
        if (scene.isWindowOpen(m_word[2]))
            return 0;
        scene.m_cine_sco.fadeOut(0);
        g_sequence.m_bits[1] |= 0x10000000;
        scene.requestMenu(0x10000, 0);
        g_sequence.m_flags |= 1;
        m_state = 0xca;
        return 0;
    case 0xca:
        if (gam.isMenuOpen() == 0)
            return 0;
        m_state = 0xcb;
        return 0;
    case 0xcb:
        if (gam.isMenuOpen() != 0)
            return 0;
        g_sequence.m_flags &= ~1u;
        gam.addItem((short)m_word[0], 0);
        return 1;
    case 300:
        return 1;
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Kinds 0..1: map arg's caption, 30 frames in, 27 held, 15 out.
// ROR 0x001f4184

TskSysFunc::TskSysFunc(int kind, int, intptr_t arg)
{
    m_state = 0;
    m_kind = kind;
    m_stepFunc = NULL;
    if (kind < 2 && kind >= 0)
    {
        m_map = (int)arg;
        m_state = 100;
        m_inTime = 0x1e;
        m_outTime = 0xf;
        m_holdTime = 0x1b;
        m_stepFunc = &TskSysFunc::stepMapName;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Kind 1 hides a caption still shown.
// ROR 0x001f4220

void TskSysFunc::term()
{
    S2dMes &mes = g_magSystem.m_scene.m_mes;
    if (m_kind == 1 && mes.m_map_name_on == 1)
    {
        mes.m_map_name_on = 0;
        mes.m_fade_time = 0;
    }
}

// ROR 0x001f425c

int TskSysFunc::exec(int, int, int)
{
    if (m_stepFunc == NULL)
        return 1;   /* PORT: the ROM calls an unset member pointer */
    return (this->*m_stepFunc)();
}

// ──────────────────────────────────────────────────────────────────────
// 100 shows the caption (setMapNumber, setMapNameFadeIn inlined), 0x65
// holds it m_holdTime frames, then fades it out and ends.
// ROR 0x001f42c4 (folded into the body of 0x1f425c by Ghidra)

int TskSysFunc::stepMapName()
{
    S2dMes &mes = g_magSystem.m_scene.m_mes;
    switch (m_state)
    {
    case 100:
        mes.setMapNumber(m_map);
        mes.setMapNameFadeIn(m_inTime);
        m_state = 0x65;
        m_count = m_holdTime;
        return 0;
    case 0x65:
        if (m_count-- > 0)
            return 0;
        mes.setMapNameFadeOut(m_outTime);
        return 1;
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Stops the music, fades sprite 3 out and starts the film at 60 fps.
// ROR 0x001f438c

TskMovie::TskMovie(const char *name)
{
    g_gamMan.m_sndMan.funcSnd(0x1d, 1, 0x40, 0, -1);
    g_sndMan.stopAll(8);
    m_04 = 0;
    strncpy(m_name, name, sizeof(m_name));
    g_magSystem.m_scene.m_mes.setSpriteFadeOut(3, 0x1e);
    m_state = 100;
    g_sequence.m_bits[1] |= 0x8000000;
    g_magSystem.m_scene.m_movie.playMovie(m_name);
    MagSystem::m_v_interval = 1;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: operator_delete (GCC 2's delete flag; the host's delete frees)
// ROR 0x001f448c

TskMovie::~TskMovie()
{
    TskMovie::term();
}

// ──────────────────────────────────────────────────────────────────────
// The film's states (see tsk_chr.h); 1 ends the task.
// ROR 0x001f454c

int TskMovie::exec(int id, int time, int arg)
{
    MagScene &scene = g_magSystem.m_scene;
    int n;

    (void)id;
    (void)time;
    (void)arg;
    switch (m_state)
    {
    case 100:
        if (scene.m_movie.isMovieEnd() != 1)
            return 0;
        m_state = 0x65;
        break;
    case 0x65:
        if (scene.m_movie.stopMovie() >= 0)
        {
            m_state = 0x66;
            m_count = 2;
        }
        break;
    case 0x66:
        n = m_count--;
        if (n < 1)
        {
            scene.fadeIn(10, 7, 0);
            m_state = 0x67;
        }
        break;
    case 0x67:
        /* START held: the scene fades out at once. */
        if ((~g_magSystem.m_pad_ref[0].m_pad->m_hold & 8) == 0)
            scene.fadeOut(0, 7, 0);
        else if (g_rxMpeg.checkNearEnd() != 1)
            return 0;
        m_state = 0x68;
        break;
    case 0x68:
        m_vol[0] = g_gamMan.m_sndMan.m_mstVol[0];
        m_vol[1] = g_gamMan.m_sndMan.m_mstVol[1];
        g_gamMan.m_sndMan.funcSnd(0xd, 0, 0, 0x14, -1);
        m_count = 0;
        m_state = 0x69;
        break;
    case 0x69:
        n = m_count++;
        if (n > 0x1e)
        {
            if ((int)g_sequence.m_bits[1] < 0)
                g_sequence.m_bits[1] &= 0x7fffffff;
            else
                scene.fadeIn(0x1e, 7, 0);
            scene.m_movie.closeMovie();
            MagSystem::m_v_interval = 2;
            m_state = 0x6a;
            g_gamMan.m_sndMan.funcSnd(0xd, m_vol[0], m_vol[1], 10, -1);
            g_gamMan.m_sndMan.funcSnd(0x1e, 1, 0x40, 0, -1);
            return 0;
        }
        break;
    case 0x6a:
    default:
        return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Undoes the constructor: the script bit, the message task, the film,
// 30 fps.
// ROR 0x001f44e8

void TskMovie::term()
{
    g_sequence.m_bits[1] &= ~0x8000000u;
    g_gamMan.killMsgTask();
    g_magSystem.m_scene.m_movie.closeMovie();
    MagSystem::m_v_interval = 2;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 0 with a radius above 1 grows the achr's collision; else the
// task ends at once.
// ROR 0x001f47f0

TskSeqEvent::TskSeqEvent(u_int oid, int kind, const void *param)
{
    m_kind = kind;
    m_achr = NULL;
    m_frames = 0;
    m_radius = 0.0f;
    m_t = 0.0f;
    if (kind == 0)
    {
        const int *p = (const int *)param;
        m_count = 0;
        m_frames = p[0];
        memcpy(&m_radius, &p[1], sizeof(float));
        m_achr = g_gamMan.getAchr(oid);
        if (m_radius > 1.0f)
        {
            m_stepFunc = &TskSeqEvent::stepGrow;
            return;
        }
    }
    m_stepFunc = &TskSeqEvent::stepNone;
}

// ROR 0x001f48a8

int TskSeqEvent::exec(int, int, int)
{
    return (this->*m_stepFunc)();
}

// ROR 0x001f4910 (folded into the body of 0x1f48a8 by Ghidra)

int TskSeqEvent::stepNone()
{
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Collision kind 2, shape 4, radius m_radius * count / frames while the
// achr's object lives; 1 at the end or when it is gone.
// ROR 0x001f4918

int TskSeqEvent::stepGrow()
{
    int n = m_count++;
    if (n >= m_frames)
        return 1;
    if (!g_gamMan.isObj(m_achr->m_oid))
        return 1;
    m_t = (float)m_count / (float)m_frames;
    float radius = m_radius * m_t;
    /* PORT: the ROM stores a stack record with only these three words set,
     * the other nine are stack garbage; the host keeps them as they are. */
    m_achr->m_colPrm[0] = 2;
    m_achr->m_colPrm[1] = 4;
    memcpy(&m_achr->m_colPrm[4], &radius, sizeof(float));
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Opens the window from the words param (writing 0x8001 into the caller's
// p[2], the view's flag), fills it and shows "#C" + text. arg is not read.
// ROR 0x001f4a24 // callcheck: MsgMan::MsgMan (MsgManSet's members construct zeroed on the host)

TskMsgMan::TskMsgMan(const char *text, int arg, int *param, int fill, int mode, int time)
{
    MagScene &scene = g_magSystem.m_scene;

    (void)arg;
    m_text[0] = '#';
    m_text[1] = 'C';
    m_text[2] = '\0';
    // PORT: the ROM's strncpy may write 0x200 bytes after the 2-byte
    // prefix, two past m_text into m_mode (set right after); bounded here.
    size_t len = strlen(m_text);
    strncpy(m_text + len, text, sizeof(m_text) - len - 1);
    m_text[sizeof(m_text) - 1] = '\0';
    memcpy(m_param, param, sizeof(m_param));
    m_textPtr = m_text;
    m_mode = mode;
    m_count = 0;
    m_window = scene.openWindow(param[3], param[4], param[6], -1);
    m_time = time;
    m_count = 0;
    param[2] = 0x8001;
    g_magSystem.setViewParam(m_window, param[0], param[1], 0x8001, param[3], param[4]);
    scene.setWindowWords84(m_window, 0, 0, param[3], param[4]);
    if (fill == 0)
    {
        scene.setWindowFillMsb(m_window, 0);
        scene.setWindowFill(m_window, 1);
    }
    else
    {
        scene.setWindowFillMsb(m_window, 1);
        scene.setWindowFill(m_window, 0);
    }
    scene.updateWindowFont(m_window);
    m_msg.open();
    m_msg.setMessage(m_window, m_textPtr, param[5]);
    scePrintf("TskMsgMan::TskMsgMan \n");
    scePrintf("m_window_no : %d %s \n", m_window, m_textPtr);
    scePrintf("======= ======= ======= ======= ======= ======= ======= ======= ======= \n");
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: ~MsgMan, operator_delete (the implicit member destructors, GCC 2's delete flag; the host's delete frees)
// ROR 0x001f4c74

TskMsgMan::~TskMsgMan()
{
    TskMsgMan::term();
}

// ──────────────────────────────────────────────────────────────────────
// Closes the text and the window.
// ROR 0x001f4d14

void TskMsgMan::term()
{
    MagScene &scene = g_magSystem.m_scene;

    m_msg.close();
    scene.setWindowShow(m_window, 1);
    scene.updateWindowFont(m_window);
    scene.closeWindow(m_window);
    m_mode = 0;
}

// ──────────────────────────────────────────────────────────────────────
// The message sprites in the ROM's order.
// ROR 0x001f4d84

void TskMsgMan::pause(int on)
{
    static const int sprites[] = {0, 2, 1, 3, 5, 4, 6, 7, 8, 9, 10, 11, 12, 13};
    MagScene &scene = g_magSystem.m_scene;

    scene.setWindowShow(m_window, on != 0);
    for (size_t i = 0; i < sizeof(sprites) / sizeof(sprites[0]); i++)
    {
        if (on != 0)
            scene.m_mes.setSpriteProcessOff(sprites[i]);
        else
            scene.m_mes.setSpriteProcessOn(sprites[i]);
    }
}

// ──────────────────────────────────────────────────────────────────────
// One frame of the text; at the end the window's MsgMan is released
// (term closes the window).
// ROR 0x001f4f5c

int TskMsgMan::exec(int id, int time, int arg)
{
    const BedPad *pad = g_magSystem.m_pad_ref[0].m_pad;
    int done = 0;

    (void)id;
    (void)time;
    (void)arg;
    switch (m_mode)
    {
    case 1:
        done = m_time < m_count++;
        break;
    case 2:
        if (m_msg.getState(m_window) == 0 && ((pad->m_trig >> 14) & 1) != 0)
            done = 10 < m_count;
        m_count++;
        break;
    case 3:
        if (m_msg.getState(m_window) == 0 && 10 < m_count)
            done = ((pad->m_trig >> 14) & 1) != 0;
        if (!done)
            done = m_time < m_count;
        m_count++;
        break;
    case 4:
        done = m_msg.getState(m_window) == 0;
        m_count++;
        break;
    }
    if (!done)
        m_msg.update();
    else
        m_msg.release(m_window);
    return done;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f5244

TskAttach::TskAttach(u_int oid, const int *value, const char *name)
{
    m_oid = oid;
    m_actor = g_gamMan.getActorNo(oid);
    m_name = name;
    m_light = *value;
}

// ──────────────────────────────────────────────────────────────────────
// Light m_light (type 2) to the node's world position (the matrix's
// translation row).
// ROR 0x001f52e4

int TskAttach::exec(int, int, int)
{
    /* PORT: zeroed -- the ROM passes the stack matrix untouched when the
     * node is missing. */
    I3dMatrixF4x4 m = {};
    g_magSystem.getActorNodeMatrixByName(m_actor, &m, m_name);
    g_magSystem.m_scene.setLightParam(2, m_light, 0, m.m[3], 0);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// A new TskMtnMove for oid: an achr's replaces its motion task m_task[2]
// (rotY >= 32769 first sets the actor's rotation to its heading); other
// objects get one with no actor.
// ROR 0x001f59b4

CmnITask *chrMtnMove(float speed, float rotY, u_int oid, int motion, int p1d8, u_int flags,
                     const char *name)
{
    TskMtnMove *task;
    int        *id;

    if (gamOidList(oid) == 1)
    {
        ACHRstat *chr = g_gamMan.getAchr(oid);
        int       no = chr->m_actor;

        id = &chr->m_task[2];
        if (name != NULL)
            motion = g_magSystem.m_scene.m_actors[no].findMotionByName(name);
        if (g_taskMan.getTask(*id) != 0)
        {
            if (rotY >= CHR_ROT_KEEP)
            {
                sceVu0FVECTOR rot;
                rotY = g_magSystem.m_scene.m_actors[no].getRotY();
                rot[0] = 0.0f;
                rot[1] = rotY;
                rot[2] = 0.0f;
                rot[3] = 0.0f;
                g_magSystem.setActorRotate(no, rot);
            }
            g_taskMan.killTask(*id);
            *id = -1;
        }
        chr->m_13c = motion;
        // callcheck: operator_new (the new expression)
        task = new TskMtnMove(speed, rotY, no, motion, p1d8, flags, oid);
        g_taskMan.addTask(1, -1, 1, 0, task, 0, id, 0x11);
    }
    else
    {
        task = new TskMtnMove(speed, CHR_ROT_KEEP, -1, motion, p1d8, flags, oid);
        id = NULL;
        g_taskMan.addTask(1, -1, 1, 0, task, 0, id, 1);
    }
    return task;
}

// ──────────────────────────────────────────────────────────────────────
// oid's actor's node by name, as DogActor::getNodeWorldPos (vtable slot
// 18) gives it; the ROM inlines DogModel::getNodeIndexByName with its
// asserts.
// ROR 0x001f5558 // callcheck: FUN_00215a30, i3rGetNodeIndexByName (the inlined getNodeIndexByName: its assert handler and lookup)

int chrGetNodeWorldPos(u_int oid, float *out, const char *name)
{
    DogActor *actor = g_gamMan.getActor(oid);
    sceVu0FVECTOR pos;

    if (actor == NULL || (g_gamMan.m_flags2 & 0x3000) != 0)
        return 0;
    int node = actor->getNodeIndexByName(name);
    if (node < 0)
        return 0;
    pos[0] = 0.0f;
    pos[1] = 0.0f;
    pos[2] = 0.0f;
    pos[3] = 1.0f;
    actor->getNodeWorldPos(pos, node);
    out[0] = pos[0];
    out[3] = pos[3];
    out[1] = pos[1];
    out[2] = pos[2];
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// A new TskMtnMove2 for oid: an achr's replaces its motion task m_task[2];
// other objects get one with no actor.
// ROR 0x001f5bdc

CmnITask *chrMtnMove2(float speed, u_int oid, int motion, int p1d8, u_int flags,
                      const char *name)
{
    TskMtnMove2 *task;
    int         *id;

    if (gamOidList(oid) == 1)
    {
        ACHRstat *chr = g_gamMan.getAchr(oid);
        int       no = chr->m_actor;

        id = &chr->m_task[2];
        if (name != NULL)
            motion = g_magSystem.m_scene.m_actors[no].findMotionByName(name);
        if (g_taskMan.getTask(*id) != 0)
        {
            g_taskMan.killTask(*id);
            *id = -1;
        }
        chr->m_13c = motion;
        // callcheck: operator_new (the new expression)
        task = new TskMtnMove2(speed, no, motion, p1d8, flags, oid);
        g_taskMan.addTask(1, -1, 1, 0, task, 0, id, 0x11);
    }
    else
    {
        task = new TskMtnMove2(speed, -1, motion, p1d8, flags, oid);
        id = NULL;
        g_taskMan.addTask(1, -1, 1, 0, task, 0, id, 1);
    }
    return task;
}

// ──────────────────────────────────────────────────────────────────────
// The motion named by its suffix (0 when the actor has none of that
// name); nothing without an actor. Always 0.
// ROR 0x001f5d90

int chrChangeMotionBySuffix(float blend, float rotY, u_int oid, const char *name, int a2, int a3)
{
    int no = g_gamMan.getActorNo(oid);
    if (no != -1)
    {
        int motion = g_magSystem.m_scene.m_actors[no].getMotionNoBySuffix(name);
        chrChangeMotion(oid, motion < 0 ? 0 : motion, a2, a3, blend, rotY);
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// An achr gets a new TskMtnMove in m_task[2] (as chrMtnMove, the heading
// kept with rotY >= 32769); other objects change the motion through the
// scene.
// ROR 0x001f5e48

void *chrChangeMotion(u_int oid, int motion, int a2, int a3, float blend, float rotY)
{
    int no = g_gamMan.getActorNo(oid);

    if (gamOidList(oid) != 1)
    {
        // PORT: the ROM leaves oid in t1, which changeActorMotion does not read.
        g_magSystem.m_scene.changeActorMotion(blend, no, motion, a2, (u_int)a3);
        return NULL;
    }

    ACHRstat *chr = g_gamMan.getAchr(oid);
    int      *id = &chr->m_task[2];
    if (g_taskMan.getTask(*id) != 0)
    {
        if (rotY >= CHR_ROT_KEEP)
        {
            sceVu0FVECTOR rot;
            rotY = g_magSystem.m_scene.m_actors[no].getRotY();
            memset(rot, 0, sizeof(rot));
            rot[1] = rotY;
            g_magSystem.setActorRotate(no, rot);
        }
        g_taskMan.killTask(*id);
        *id = -1;
    }
    chr->m_13c = motion;
    // callcheck: operator_new (the new expression)
    TskMtnMove *task = new TskMtnMove(blend, rotY, no, motion, a2, (u_int)a3, oid);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, id, 0x11);
    return task;
}

// ──────────────────────────────────────────────────────────────────────
// Kills an achr's motion task m_task[2].
// ROR 0x001f61ec

void chrKillMotionTask(u_int oid)
{
    if (g_gamMan.isObj(oid) == 0 || gamOidList(oid) != 1)
        return;
    ACHRstat *chr = g_gamMan.getAchr(oid);
    int      *id = &chr->m_task[2];
    if (g_taskMan.getTask(*id) != 0)
    {
        g_taskMan.killTask(*id);
        *id = -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// A speed outside 0.0001..100 becomes 1/60 with p1d8 1, a p1d8 below -1
// or above 300 becomes 1. An achr keeps the heading of its running motion
// task (TskMtnMove's m_rotY), which is killed; other objects keep the
// actor's heading.
// ROR 0x001f6308 // callcheck: findTask (only when the task runs; inlined below as the TskMtnMove cast)

void chrSetMotion(float speed, int no, int motion, int p1d8, u_int flags, u_int oid,
                  const char *name)
{
    MagScene &scene = g_magSystem.m_scene;
    sceVu0FVECTOR rot;

    if (speed < 0.0001f || speed > 100.0f)
    {
        speed = 0x1.11111p-6f; /* 0x3c888888 */
        p1d8 = 1;
    }
    else if (p1d8 < -1)
    {
        p1d8 = 1;
    }
    if (300.0f < (float)p1d8)
        p1d8 = 1;
    if (name != NULL)
        motion = scene.m_actors[no].findMotionByName(name);

    DogActor &actor = scene.m_actors[no];
    if (gamOidList(oid) == 1)
    {
        ACHRstat *chr = g_gamMan.getAchr(oid);
        int      *id = &chr->m_task[2];

        rot[0] = actor.m_rotate[0];
        rot[1] = actor.m_rotate[1];
        rot[2] = actor.m_rotate[2];
        rot[3] = 0.0f; /* PORT: not set by the ROM (not read by setActorRotate's x, y, z) */
        scene.changeActorMotion(speed, no, motion, p1d8, flags);
        if (g_taskMan.getTask(*id) != 0)
        {
            TskMtnMove *task = static_cast<TskMtnMove *>(g_taskMan.findTask(*id));
            rot[1] = task->m_rotY;
            rot[0] = 0.0f;
            rot[2] = 0.0f;
            rot[3] = 0.0f;
        }
        g_magSystem.setActorRotate(no, rot);
        if (g_taskMan.getTask(*id) != 0)
        {
            g_taskMan.killTask(*id);
            *id = -1;
        }
        chr->m_13c = motion;
    }
    else
    {
        rot[1] = actor.m_rotate[1];
        scene.changeActorMotion(speed, no, motion, p1d8 != 0 ? p1d8 : 1, flags);
        rot[0] = 0.0f;
        rot[2] = 0.0f;
        rot[3] = 0.0f;
        g_magSystem.setActorRotate(no, rot);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f6878

int chrIsOidType2(u_int oid)
{
    return gamOidType(oid) == 2;
}

// ──────────────────────────────────────────────────────────────────────
// GamMan::update's per-frame reset of the frame request list.
// ROR 0x001f688c

void gamClearFrameReq(void)
{
    // PC: the ROM clears 0x20 bytes, the first 8 of the 32 words.
    memset(g_frameReq, 0, 0x20);
    g_frameReqNum = 0;
}

// ──────────────────────────────────────────────────────────────────────
// The frames a turn by angle takes at speed: |angle / (speed * 0.05233)|.
// ROR 0x001f6b8c

int chrRotFrames(float angle, float speed)
{
    int frames = (int)(angle / (speed * 0x1.acaff6p-5f)); /* 0x3d5657fb */
    if ((float)frames < 0.0f)
        frames = -frames;
    return frames;
}
