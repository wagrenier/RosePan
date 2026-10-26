/* ==========================================================================
 *  task/tsk_chr.cpp
 *
 *  The script's tasks: the achr motion / movement tasks with their chr*
 *  starters and the generic script tasks (pinned 0x1e7f28..0x1f6c64); see
 *  tsk_chr.h. Written so far: TskMtnMove / TskMtnMove2 and their starters
 *  (chrMtnMove, chrMtnMove2, chrChangeMotion, chrKillMotionTask,
 *  chrSetMotion, chrGetNodeWorldPos), TskMovie (the film task),
 *  TskMsgMan (the message window task), the frame request list.
 * ======================================================================== */
#include "task/tsk_chr.h"

#include <math.h>
#include <string.h>

#include "dog/dogactor.h"
#include "game/gamman.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
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
