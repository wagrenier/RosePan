/* ==========================================================================
 *  mag/maglifepcmove.cpp
 *
 *  MagLifePC's movement: the collision sweeps (an object test, then a
 *  cylinder against the walls and the sky slots' models, sliding along the
 *  wall it hits, and two side rays for a head-on wall), moveBy (every moving
 *  mode's step: the sweeps, the floor, the secta cells), the root-motion
 *  step of the current motion, and the stick-driven motions: the stance
 *  walk (MagLifePCModeAtkMove2D) and an older pad set nothing references
 *  (movePad -> moveFree / moveDowned, moveTank), plus sweepRay, an older
 *  single-ray sweep nothing calls.
 *  0x11e0d8..0x120788 (pinned; the asserts name this file from 0x11e4c4).
 * ======================================================================== */

#include "mag/maglifepc.h"

#include <math.h>

#include "bed/bedpad.h"
#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "utl/utlmath.h"

/* The motion speed's frame unit (the ROM's 1/60 constant, one ulp off). */
#define PC_MOTION_FPS_INV   0x1.11112ep-6f      /* 0x3c888897 */
/* utlVecRotMatY's quarter turns (0x3fc90fda, not DOG_PI / 2). */
#define PC_HALF_PI          0x1.921fb4p+0f
/* moveToward: the heading step limit and the "about face" bound. */
#define PC_TURN_STEP        0x1.999998p-3f      /* 0.2 */
#define PC_TURN_BACK        0x1.91b8c4p+1f      /* 3.138451 */
/* moveBy: the slide and the move this parallel call the wall head-on. */
#define PC_HEAD_ON_DOT      0x1.999998p-1f      /* 0.8 */
/* The sweeps' smallest cylinder radius / side distance. */
#define PC_SWEEP_MIN        0x1.999998p-4f      /* 0.1 */

/* The ROM's angle-to-sector macro (its asserts name yyy): ry + pi in steps
 * of pi / n, 2n sectors; the caller asserts the range. */
#define PC_ROT_SECTOR(out, ry, n)                                   \
    do {                                                            \
        float yyy = (ry) + DOG_PI;                                  \
        ROR_ASSERT(yyy >= 0.0f && yyy < DOG_PI2);                   \
        (out) = (int)(yyy * (n) / DOG_PI);                          \
    } while (0)

/* The walk / run motions by health (rows 0..4) and the run (row 5). EE
 * 0x20 bytes; layout: host -- pointers. */
struct MagLifePCMoveMotion
{
    const char *m_wait;
    const char *m_in;
    const char *m_loop;
    const char *m_back_in;
    const char *m_back;
    const char *m_turn_l;
    const char *m_turn_r;
    const char *m_1c;           // "" in every row; nothing reads it
};
/* PC: moveFree and moveTank index row 6 (the run from health 1 up), past
 * the table into the words at 0x297300 (0 for m_in / m_loop, garbage for
 * m_back / m_turn_r); the host row is all NULL, so changeMotion asserts. */
// ROR 0x00297240
static const MagLifePCMoveMotion s_moveMotions[7] = {
    { "wait",       "walk_in", "walk",     "walk_b_in", "walk_b",    "turn_l",   "turn_r",   "" },
    { "tired",      "walk_in", "walk",     "walk_b_in", "walk_b",    "turn_l",   "turn_r",   "" },
    { "w_wait",     "",        "w_walk",   "",          "w_walk_b",  "w_turn_l", "w_turn_r", "" },
    { "ev201_wait", "",        "ev201_up", "",          "ev201_dwn", "ev201_l",  "ev201_r",  "" },
    { "ev501_wait", "",        "ev201_up", "",          "ev201_dwn", "ev201_l",  "ev201_r",  "" },
    { "wait",       "run_in",  "run",      "walk_b_in", "walk_b",    "turn_l",   "turn_r",   "" },
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
};

/* sweepRay's ray record, its debug line (vectors 0..1 of four) and the
 * hit triangle's strip colours, each set up once. */
// ROR 0x00297300
static int s_rayHitInit;
// ROR 0x00297304
static int s_rayLineInit;
// ROR 0x00297308
static int s_rayStripAdded;
// ROR 0x00297310
static const float s_rayStripColors[3][4] = {
    { 1.0f, 0.0f, 1.0f, 0.5f }, { 1.0f, 0.0f, 1.0f, 0.5f }, { 1.0f, 0.0f, 1.0f, 0.5f },
};
// ROR 0x00664590
static MagSceneRayHit s_rayHit;
// ROR 0x00664600
static sceVu0FVECTOR s_rayLine[4];

/* sweepMove's cylinder record (its vectors set to (0, 0, 0, 1) on the
 * first call) and the debug strip m_218 adds once from its vectors 2..4. */
// ROR 0x00297340
static int s_sweepHitInit;
// ROR 0x00297344
static int s_sweepStripAdded;
// ROR 0x00297350
static const float s_sweepStripColors[3][4] = {
    { 1.0f, 1.0f, 0.0f, 0.5f }, { 1.0f, 1.0f, 0.0f, 0.5f }, { 1.0f, 1.0f, 0.0f, 0.5f },
};
// ROR 0x00664640
static MagSceneRayHit s_sweepHit;

/* moveFree's stick heading in the world (kept while the stick holds). */
// ROR 0x00297380
static float s_freeRotY;
/* The stance steps by the stick's direction against the heading. */
// ROR 0x00297388
static const char *s_freeStepMotions[8] = {
    "atk_walk", "atk_step_fr", "atk_step_r", "atk_step_br",
    "atk_walk_b", "atk_step_bl", "atk_step_l", "atk_step_fl",
};
// ROR 0x002eac38
static const float s_freeStepRotY[8] = {
    0.0f, 0.7853981256484985f, 1.570796251296997f, 2.356194257736206f,
    3.1415929794311523f, -2.356194257736206f, -1.570796251296997f, -0.7853981256484985f,
};

/* moveAttackStance's: the same pair of tables, its own copy. */
// ROR 0x002973a8
static float s_stanceRotY;
// ROR 0x002973b0
static const char *s_stanceStepMotions[8] = {
    "atk_walk", "atk_step_fr", "atk_step_r", "atk_step_br",
    "atk_walk_b", "atk_step_bl", "atk_step_l", "atk_step_fl",
};
// ROR 0x002eacc0
static const float s_stanceStepRotY[8] = {
    0.0f, 0.7853981256484985f, 1.570796251296997f, 2.356194257736206f,
    3.1415929794311523f, -2.356194257736206f, -1.570796251296997f, -0.7853981256484985f,
};
/* Its last step (written, never read). */
// ROR 0x006646b0
static float s_stanceStep;
// ROR 0x006646b4
static float s_stanceStepDir;

/* moveDowned's crawl by direction (back, left, forward, right). */
// ROR 0x002973d0
static const char *s_downedMotions[4] = { "ev201_dwn", "ev201_l", "ev201_up", "ev201_r" };

/* A ray record's vectors to (0, 0, 0, 1). */
static void clearRayHit(MagSceneRayHit *hit)
{
    for (int i = 0; i < 5; i++)
    {
        hit->m_vec[i][0] = 0.0f;
        hit->m_vec[i][1] = 0.0f;
        hit->m_vec[i][2] = 0.0f;
        hit->m_vec[i][3] = 1.0f;
    }
}

/* Inline in the ROM (moveFree, moveAttackStance): the stick (x, -y) as a
 * heading, turned by the camera's (its z axis, level). */
static float stickWorldRotY(MagScene *scene, float x, float y)
{
    sceVu0FVECTOR dir = { x, 0.0f, -y, 1.0f };
    sceVu0FVECTOR rot = { 0.0f, 0.0f, 0.0f, 1.0f };
    utlVecToRot(dir, rot);
    float ry = rot[1];
    DogCamera *cam = &scene->m_camera_entity;
    cam->transform();
    sceVu0FVECTOR fwd = { cam->m_matrix[2][0], 0.0f, cam->m_matrix[2][2], cam->m_matrix[2][3] };
    utlNormalize3(fwd);
    sceVu0FVECTOR crot = { 0.0f, 0.0f, 0.0f, 1.0f };
    utlVecToRot(fwd, crot);
    ry += crot[1];
    if (ry < -DOG_PI)
        ry += DOG_PI2;
    if (ry >= DOG_PI)
        ry -= DOG_PI2;
    return ry;
}

/* The current motion's root step from frame f to f + speed (the frames
 * doubled, as every caller here does) into move / rot, the root's y into
 * trans_y; nothing while the actor's mode has an argument. Inline in the
 * ROM (moveToward, moveStanceStep). */
static void getRootStep(DogActor *actor, float speed, float *move, float *rot, float *trans_y)
{
    sceVu0FVECTOR pos = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (actor->getModeArg() < 0)
    {
        float f = actor->getMotionFrame();
        if (f >= 0.0f)
        {
            float t1 = f + speed;
            actor->getMotionMove(f + f, t1 + t1, move, rot, pos);
            *trans_y = pos[1];
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011e0d8
float MagLifePC::sweepRay(float range, float *out, const float *delta)
{
    sceVu0FVECTOR pos = { m_actor->m_trans[0], m_actor->m_trans[1] - 30.0f,
                          m_actor->m_trans[2], m_actor->m_trans[3] };
    float len = sqrtf(delta[0] * delta[0] + delta[2] * delta[2]);
    if (len <= 0.0f)
        return -1.0f;
    sceVu0FVECTOR ray = { delta[0] * range / len, 0.0f, delta[2] * range / len, 1.0f };
    if (!s_rayHitInit)
    {
        clearRayHit(&s_rayHit);
        s_rayHitInit = 1;
    }
    s_rayHit.m_50 = -1;
    float t = m_scene->castRay(&s_rayHit, pos, ray, -1);
    if (t >= 0.0f)
    {
        /* As sweepMove: the wall's normal facing the ray, turned a quarter
         * to the side the ray leans to, the level move's length long. */
        sceVu0FVECTOR n = { s_rayHit.m_vec[1][0], s_rayHit.m_vec[1][1],
                            s_rayHit.m_vec[1][2], s_rayHit.m_vec[1][3] };
        if (utlVecDot(n, ray) < 0.0f)
        {
            n[0] = -n[0];
            n[3] = 1.0f;
            n[1] = -n[1];
            n[2] = -n[2];
        }
        sceVu0FVECTOR c = { 0.0f, 0.0f, 0.0f, 1.0f };
        utlVecCross(c, ray, n);
        if (c[1] > 0.0f)
            utlVecRotMatY(-PC_HALF_PI, n);
        else
            utlVecRotMatY(PC_HALF_PI, n);
        out[0] = n[0] * len;
        out[1] = n[1] * len;
        out[2] = n[2] * len;
        out[3] = 1.0f;
    }
    if (m_218 != 0.0f)
    {
        if (!s_rayLineInit)
        {
            for (int i = 0; i < 4; i++)
            {
                s_rayLine[i][0] = 0.0f;
                s_rayLine[i][1] = 0.0f;
                s_rayLine[i][2] = 0.0f;
                s_rayLine[i][3] = 1.0f;
            }
            s_rayLineInit = 1;
        }
        /* From the ray's start, 50 times out (stale when the ray missed). */
        for (int i = 0; i < 4; i++)
            s_rayLine[0][i] = pos[i];
        s_rayLine[1][0] = pos[0] + out[0] * 50.0f;
        s_rayLine[1][1] = pos[1] + out[1] * 50.0f;
        s_rayLine[1][2] = pos[2] + out[2] * 50.0f;
        s_rayLine[1][3] = 1.0f;
        if (!s_rayStripAdded)
        {
            s_rayStripAdded = 1;
            m_scene->addLineStrip(2, s_rayLine, 0x80ff8030, NULL);
            m_scene->addStrip(3, s_rayHit.m_vec[2], s_rayStripColors, NULL, -1);
        }
    }
    return t;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011e4c4
float MagLifePC::sweepSides(float range, float radius, const float *from, const float *delta,
                            float *out)
{
    sceVu0FVECTOR to = { from[0] + delta[0], from[1] + delta[1], from[2] + delta[2], 1.0f };
    if (testObjHit(to, NULL))
        return 1.0f;
    float h = range - radius;
    if (h <= PC_SWEEP_MIN)
        h = 10.0f;
    sceVu0FVECTOR n = { delta[0], delta[1], delta[2], delta[3] };
    float len = utlVecLength(n);
    n[1] = 0.0f;
    n[0] = n[0] / len;
    n[2] = n[2] / len;
    sceVu0FVECTOR ray = { n[0] * range, n[1], n[2] * range, 1.0f };
    sceVu0FVECTOR side_a = { n[0], n[1], n[2], n[3] };
    utlVecRotMatY(PC_HALF_PI, side_a);
    side_a[0] *= h;
    side_a[1] = n[1];
    side_a[2] *= h;
    side_a[3] = 1.0f;
    sceVu0FVECTOR side_b = { n[0], n[1], n[2], n[3] };
    utlVecRotMatY(-PC_HALF_PI, side_b);
    side_b[0] *= h;
    side_b[1] = n[1];
    side_b[2] *= h;
    side_b[3] = 1.0f;
    sceVu0FVECTOR pos_a = { from[0] + side_a[0], from[1] + side_a[1] - 50.0f, from[2] + side_a[2], 1.0f };
    sceVu0FVECTOR pos_b = { from[0] + side_b[0], from[1] + side_b[1] - 50.0f, from[2] + side_b[2], 1.0f };
    /* PC: the ROM leaves m_50 as stack garbage. */
    MagSceneRayHit hit_a = {};
    MagSceneRayHit hit_b = {};
    clearRayHit(&hit_a);
    clearRayHit(&hit_b);
    ROR_ASSERT(m_scene); /* 291 */
    float ta = m_scene->castRay(&hit_a, pos_a, ray, -1);
    float tb = m_scene->castRay(&hit_b, pos_b, ray, -1);
    int hit = 0;
    if (0.0f <= ta && ta <= 1.0f)
        hit = 1;
    if (0.0f <= tb && tb <= 1.0f)
        hit |= 2;
    if (hit == 3)
        hit = (tb < ta) ? 2 : 1;
    const float *side;
    switch (hit)
    {
    case 0:
        return -1.0f;
    case 1:
        side = side_b;          /* side a is blocked: away from it */
        break;
    case 2:
        side = side_a;
        break;
    default:
        ROR_ASSERT(0); /* 349 */
        return 1.0f;
    }
    if (out != NULL)
    {
        out[0] = side[0] / h * len;
        out[1] = side[1] / h * len;
        out[2] = side[2] / h * len;
        out[3] = 1.0f;
    }
    return 0.0f;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011ea04
float MagLifePC::sweepMove(float range, float radius, const float *from, const float *delta,
                           float *out)
{
    sceVu0FVECTOR to = { from[0] + delta[0], from[1] + delta[1], from[2] + delta[2], 1.0f };
    u_int oid;
    if (testObjHit(to, &oid))
    {
        if (out != NULL)
            slideOffObj(out, delta, oid);
        return 1.0f;
    }
    float h = range - radius;
    sceVu0FVECTOR dir = { delta[0], delta[1], delta[2], delta[3] };
    float len = utlVecLength(dir);
    dir[1] = 0.0f;
    dir[0] = dir[0] * radius / len;
    dir[2] = dir[2] * radius / len;
    sceVu0FVECTOR pos = { from[0] + dir[0], from[1] + dir[1] - (range + 50.0f), from[2] + dir[2], 1.0f };
    sceVu0FVECTOR down = { 0.0f, -50.0f, 0.0f, 1.0f };
    if (h <= PC_SWEEP_MIN)
        h = 10.0f;
    if (!s_sweepHitInit)
    {
        clearRayHit(&s_sweepHit);
        s_sweepHitInit = 1;
    }
    ROR_ASSERT(m_scene); /* 428 */
    float t = m_scene->castCylinder(h, pos, down, -1, &s_sweepHit);
    if (out != NULL && t >= 0.0f)
    {
        /* The wall's normal facing the move, turned a quarter along the
         * wall to the side the move leans to, the move's length long. */
        sceVu0FVECTOR n = { s_sweepHit.m_vec[1][0], s_sweepHit.m_vec[1][1],
                            s_sweepHit.m_vec[1][2], s_sweepHit.m_vec[1][3] };
        if (utlVecDot(n, dir) < 0.0f)
        {
            n[0] = -n[0];
            n[3] = 1.0f;
            n[1] = -n[1];
            n[2] = -n[2];
        }
        sceVu0FVECTOR c = { 0.0f, 0.0f, 0.0f, 1.0f };
        utlVecCross(c, dir, n);
        if (c[1] > 0.0f)
            utlVecRotMatY(-PC_HALF_PI, n);
        else
            utlVecRotMatY(PC_HALF_PI, n);
        n[0] *= len;
        n[3] = 1.0f;
        n[1] *= len;
        n[2] *= len;
        out[0] = n[0];
        out[1] = n[1];
        out[2] = n[2];
        out[3] = n[3];
    }
    if (m_218 != 0.0f && !s_sweepStripAdded)
    {
        s_sweepStripAdded = 1;
        m_scene->addStrip(3, s_sweepHit.m_vec[2], s_sweepStripColors, NULL, -1);
    }
    return t;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011ee00
void MagLifePC::moveBy(const float *move, float trans_ofs_y, float rot)
{
    DogActor *actor = m_actor;
    if (m_health >= 3)
    {
        sceVu0FVECTOR none = { 0.0f, 0.0f, 0.0f, 0.0f };
        actor->addTrans(none, trans_ofs_y, rot);
        updateSectaCells();
        return;
    }
    if (m_scene->m_22b8 != 0)
    {
        actor->addTrans(move, trans_ofs_y, rot);
        updateSectaCells();
        return;
    }
    sceVu0FVECTOR pos = { actor->m_trans[0], actor->m_trans[1], actor->m_trans[2], actor->m_trans[3] };
    sceVu0FVECTOR slide = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (sweepMove(m_sweep_range, m_sweep_radius, pos, move, slide) < 0.0f)
        m_actor->addTrans(move, trans_ofs_y, rot);
    else
    {
        /* Blocked: try the slide along the wall. */
        dbgTraceVector4(slide);
        actor = m_actor;
        sceVu0FVECTOR pos2 = { actor->m_trans[0], actor->m_trans[1], actor->m_trans[2], actor->m_trans[3] };
        if (sweepMove(m_sweep_range, m_sweep_radius, pos2, slide, NULL) < 0.0f)
            m_actor->addTrans(slide, trans_ofs_y, rot);
        else
        {
            /* The slide is blocked too: on a head-on wall the side rays
             * pick another slide, else stand. */
            sceVu0FVECTOR nm = { move[0], move[1], move[2], move[3] };
            utlNormalize3(nm);
            sceVu0FVECTOR ns = { slide[0], slide[1], slide[2], slide[3] };
            utlNormalize3(ns);
            float d = utlVecDot(ns, nm);
            int retry = 1;
            if (d >= PC_HEAD_ON_DOT || d <= -PC_HEAD_ON_DOT)
                retry = sweepSides(m_sweep_range, m_sweep_radius, pos, move, slide) == 0.0f;
            sceVu0FVECTOR none = { 0.0f, 0.0f, 0.0f, 1.0f };
            if (retry && sweepMove(m_sweep_range, m_sweep_radius, pos2, slide, NULL) < 0.0f)
                m_actor->addTrans(slide, trans_ofs_y, rot);
            else
                m_actor->addTrans(none, trans_ofs_y, rot);
        }
    }
    followGround();
    updateSectaCells();
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011f0b4
int MagLifePC::moveTank(BedPad *pad)
{
    float x, y;
    float mag = pad->getDir(&x, &y);
    if (mag <= m_dead_zone)
        return 0;
    ROR_ASSERT(m_move_level != 0); /* 684: m_move_stat != MOV_STAT_WAIT */
    float step = getHealthMotionStep() * mag;
    float ax = x;
    if (x < 0.0f)
        ax = -x;
    const char *in = NULL;
    const char *motion;
    int walking = 0;
    float turn_x = 0.0f;
    if (ax / mag < m_cos_1fc)
    {
        int row, in_row;
        if (m_move_level == 1)
            row = in_row = m_health;
        else
        {
            ROR_ASSERT(m_move_level == 2); /* 709: m_move_stat == MOV_STAT_RUN */
            row = 6;
            in_row = m_move_level_prev != 0 ? -1 : 6;
        }
        if (y < 0.0f)
        {
            if (in_row >= 0)
                in = s_moveMotions[in_row].m_in;
            motion = s_moveMotions[row].m_loop;
        }
        else
        {
            if (in_row >= 0)
                in = s_moveMotions[in_row].m_back_in;
            motion = s_moveMotions[row].m_back;
        }
        walking = 1;
        turn_x = x;
    }
    else if (m_stance == 0)
        motion = x > 0.0f ? s_moveMotions[m_health].m_turn_r : s_moveMotions[m_health].m_turn_l;
    else
        motion = x > 0.0f ? "atk_step_r" : "atk_step_l";
    if (in == NULL)
        changeMotion(step * PC_MOTION_FPS_INV, motion, 5, NULL, -1);
    else
        changeMotion(step * PC_MOTION_FPS_INV, in, 6, motion, -1);

    sceVu0FVECTOR move = { 0.0f, 0.0f, 0.0f, 1.0f };
    sceVu0FVECTOR pos = { 0.0f, 0.0f, 0.0f, 1.0f };
    float rot;
    float trans_y = 0.0f;
    if (m_actor->getModeArg() < 0)
    {
        float f = m_actor->getMotionFrame120();
        if (f >= 0.0f)
        {
            m_actor->getMotionMove(f, f + (step + step), move, &rot, pos);
            trans_y = pos[1];
        }
    }
    move[1] = 0.0f;
    move[3] = 1.0f;
    float turn = 0.0f;
    float vry;
    if (walking)
    {
        turn = turn_x * m_204;
        vry = m_actor->getRotY();
    }
    else
        vry = m_actor->m_base_rotate[1];
    ROR_ASSERT((vry >= -DOG_PI) && (vry < DOG_PI)); /* 830 */
    utlVecRotMatY(vry, move);
    moveBy(move, trans_y, 0.0f);
    DogActor *actor = m_actor;
    float ry = actor->m_rotate[1] + turn;
    actor->m_rotate[1] = ry;
    if (ry < -DOG_PI)
        actor->m_rotate[1] = ry + DOG_PI2;
    else if (ry >= DOG_PI)
        actor->m_rotate[1] = ry - DOG_PI2;
    actor->m_dirty = 1;
    m_actor->addBaseRotY(turn);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011f4a0
int MagLifePC::isKeepMotion(int no)
{
    (void)no;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// callcheck: getModeArg, getMotionFrame, getMotionMove (in getRootStep)
// ROR 0x0011f4a8
void MagLifePC::moveToward(float theRotY, float thePlyY, float speed, const char *theIn,
                           const char *theMotion, const char *theTurn)
{
    ROR_ASSERT((-DOG_PI <= theRotY) && (theRotY < DOG_PI)); /* 1048 */
    ROR_ASSERT((-DOG_PI <= thePlyY) && (thePlyY < DOG_PI)); /* 1049 */
    float ry = theRotY - thePlyY;
    int about_face = 0;
    if (ry < -DOG_PI)
        ry += DOG_PI2;
    if (ry >= DOG_PI)
        ry -= DOG_PI2;
    ROR_ASSERT((-DOG_PI <= ry) && (ry < DOG_PI)); /* 1058 */
    const char *first;
    if (-PC_TURN_BACK < ry && ry < PC_TURN_BACK)
    {
        if (ry > PC_TURN_STEP)
            ry = PC_TURN_STEP;
        if (ry < -PC_TURN_STEP)
            ry = -PC_TURN_STEP;
        DogActor *actor = m_actor;
        float r = actor->m_rotate[1] + ry;
        actor->m_rotate[1] = r;
        if (r < -DOG_PI)
            actor->m_rotate[1] = r + DOG_PI2;
        else if (r >= DOG_PI)
            actor->m_rotate[1] = r - DOG_PI2;
        actor->m_dirty = 1;
        m_actor->addBaseRotY(ry);
        first = theIn;
    }
    else
    {
        about_face = 1;
        first = theTurn;
    }
    if (!isKeepMotion(m_actor->m_motion.getNo()))
    {
        int r;
        if (first == NULL)
            r = changeMotion(speed * PC_MOTION_FPS_INV, theMotion, 5, NULL, -1);
        else
            r = changeMotion(speed * PC_MOTION_FPS_INV, first, about_face ? 2 : 6, theMotion, -1);
        if (r >= 0)
            setStanceSubMotion();
    }
    sceVu0FVECTOR move = { 0.0f, 0.0f, 0.0f, 1.0f };
    float rot = 0.0f;
    float trans_y = 0.0f;
    getRootStep(m_actor, speed, move, &rot, &trans_y);
    move[1] = 0.0f;
    move[3] = 1.0f;
    utlVecRotMatY(about_face ? thePlyY : theRotY, move);
    moveBy(move, trans_y, rot);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// callcheck: getModeArg, getMotionFrame, getMotionMove (in getRootStep)
// ROR 0x0011f884
void MagLifePC::moveStanceStep(float theRotY, float speed, const char *theMotion)
{
    ROR_ASSERT((theRotY >= -DOG_PI) && (theRotY < DOG_PI)); /* 1164 */
    if (changeMotion(speed * PC_MOTION_FPS_INV, theMotion, 0x45, NULL, 5) >= 0)
        setStanceSubMotion();
    sceVu0FVECTOR move = { 0.0f, 0.0f, 0.0f, 1.0f };
    float rot = 0.0f;
    float trans_y = 0.0f;
    getRootStep(m_actor, speed, move, &rot, &trans_y);
    move[1] = 0.0f;
    move[3] = 1.0f;
    utlVecRotMatY(theRotY, move);
    moveBy(move, trans_y, rot);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// callcheck: utlNormalize3, utlVecToRot (in stickWorldRotY)
// ROR 0x0011f9f8
int MagLifePC::moveFree(BedPad *pad)
{
    ROR_ASSERT(m_stance == 0); /* 1237: m_attack_stat == ATK_STAT_NONE */
    float x, y;
    pad->getDir(&x, &y);
    if (m_stick_threshold < m_stick_change)
        s_freeRotY = stickWorldRotY(m_scene, x, y);
    float step = getHealthMotionStep();
    ROR_ASSERT(m_move_level != 0); /* 1317: m_move_stat != MOV_STAT_WAIT */
    if (m_stance == 0)
    {
        int row, in_row;
        const char *turn;
        if (m_move_level == 1)
        {
            row = in_row = m_health;
            turn = m_move_level_prev == 0 ? "turn_r180" : "walk_turn_r180";
        }
        else
        {
            ROR_ASSERT(m_move_level == 2); /* 1340: m_move_stat == MOV_STAT_RUN */
            row = m_health + 5;
            if (row > 6)
                row = 6;
            in_row = m_move_level_prev == 0 ? row : -1;
            turn = "run_turn_r180";
        }
        const char *in = in_row >= 0 ? s_moveMotions[in_row].m_in : NULL;
        moveToward(s_freeRotY, m_actor->m_rotate[1], step, in, s_moveMotions[row].m_loop, turn);
    }
    else
    {
        int py, ay;
        PC_ROT_SECTOR(py, s_freeRotY, 8.0f); /* 1373 */
        ROR_ASSERT(py >= 0 && py < (8)*2); /* 1376 */
        PC_ROT_SECTOR(ay, m_actor->m_rotate[1], 8.0f); /* 1381 */
        ROR_ASSERT(ay >= 0 && ay < (8)*2); /* 1384 */
        int dir = (((py + 1) / 2 & 7) - ((ay + 1) / 2 & 7)) & 7;
        float ppp = s_freeRotY - s_freeStepRotY[dir];
        if (ppp < -DOG_PI)
            ppp += DOG_PI2;
        if (ppp >= DOG_PI)
            ppp -= DOG_PI2;
        ROR_ASSERT(ppp >= -DOG_PI && ppp < DOG_PI); /* 1404 */
        moveToward(ppp, ppp, step, NULL, s_freeStepMotions[dir], NULL);
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// callcheck: utlNormalize3, utlVecToRot (in stickWorldRotY)
// ROR 0x0011ff10
int MagLifePC::moveAttackStance(BedPad *pad)
{
    ROR_ASSERT(m_stance != 0); /* 1665: m_attack_stat != ATK_STAT_NONE */
    float x, y;
    pad->getDir(&x, &y);
    if (m_stick_threshold < m_stick_change)
        s_stanceRotY = stickWorldRotY(m_scene, x, y);
    float step = getHealthMotionStep();
    if (m_move_level == 0)
        return 0;
    int py, ay;
    PC_ROT_SECTOR(py, s_stanceRotY, 8.0f); /* 1799 */
    ROR_ASSERT(py >= 0 && py < (8)*2); /* 1802 */
    float tmpry = m_actor->m_rotate[1];
    ROR_ASSERT(-DOG_PI <= tmpry && tmpry < DOG_PI); /* 1807 */
    PC_ROT_SECTOR(ay, m_actor->m_rotate[1], 8.0f); /* 1809 */
    ROR_ASSERT(ay >= 0 && ay < (8)*2); /* 1812 */
    int dir = (((py + 1) / 2 & 7) - ((ay + 1) / 2 & 7)) & 7;
    float ppp = s_stanceRotY - s_stanceStepRotY[dir];
    if (ppp < -DOG_PI)
        ppp += DOG_PI2;
    if (ppp >= DOG_PI)
        ppp -= DOG_PI2;
    s_stanceStepDir = ppp;
    ROR_ASSERT(ppp >= -DOG_PI && ppp < DOG_PI); /* 1833 */
    s_stanceStep = step;
    moveStanceStep(s_stanceStepDir, step, s_stanceStepMotions[dir]);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x001203a0
int MagLifePC::moveDowned(BedPad *pad)
{
    float x, y;
    pad->getDir(&x, &y);
    sceVu0FVECTOR dir = { x, 0.0f, -y, 1.0f };
    sceVu0FVECTOR rot = { 0.0f, 0.0f, 0.0f, 1.0f };
    utlVecToRot(dir, rot);
    int py, ay;
    PC_ROT_SECTOR(py, rot[1], 4.0f); /* 1878 */
    ROR_ASSERT(py >= 0 && py < (4)*2); /* 1881 */
    PC_ROT_SECTOR(ay, m_actor->m_rotate[1], 8.0f); /* 1886 */
    ROR_ASSERT(ay >= 0 && ay < (8)*2); /* 1889 */
    /* The stick in four directions against the heading's eight, as the ROM
     * has it (only the low two bits of the heading's half). */
    changeMotion(PC_MOTION_FPS_INV, s_downedMotions[(((py + 1) / 2 & 3) - (ay + 1) / 2) & 3], 5,
                 NULL, -1);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001205dc
int MagLifePC::movePad(BedPad *pad)
{
    if (m_action_motion < 0)
    {
        if (m_move_level == 0)
            return 0;
        if ((u_int)(m_health - 3) < 2)
            return moveDowned(pad);
        if (m_stance != 0)
            return moveAttackStance(pad);
    }
    else if (m_stance != 0)
        return moveAttackStance(pad);
    return moveFree(pad);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00120668
void MagLifePC::applyMotionMove()
{
    sceVu0FVECTOR move = { 0.0f, 0.0f, 0.0f, 1.0f };
    float rot = 0.0f;
    float trans_y = 0.0f;
    if (m_actor->getModeArg() < 0)
    {
        float f = m_actor->getMotionFrame();
        if (f >= 0.0f)
        {
            sceVu0FVECTOR pos = { 0.0f, 0.0f, 0.0f, 1.0f };
            float t1 = f + 1.0f;
            m_actor->getMotionMove(f + f, t1 + t1, move, &rot, pos);
            trans_y = pos[1];
        }
        move[3] = 1.0f;
        float vry = m_actor->m_base_rotate[1];
        ROR_ASSERT((vry >= -DOG_PI) && (vry < DOG_PI)); /* 2027 */
        utlVecRotMatY(vry, move);
        m_actor->addTrans(move, trans_y, rot);
    }
}
