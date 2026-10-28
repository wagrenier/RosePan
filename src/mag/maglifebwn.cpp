/* ==========================================================================
 *  mag/maglifebwn.cpp
 *
 *  MagLifeBwn, Brown's controller: the state requests and the embedded
 *  modes (requestState / changeMode / applyState), the per-frame live (the
 *  health class, the footsteps, the neck's look and smell sub motion, the
 *  escape from floor without secta cells), the root-motion step with its
 *  body and object hit tests, and the path: searchPath through the secta
 *  cells into the search line, which followPath walks.
 *  0x1233d8..0x125bd4 (pinned 2026-10-27; the asserts name this file).
 * ======================================================================== */

#include "mag/maglifebwn.h"

#include <math.h>
#include <string.h>

#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "utl/utlmath.h"

/* Brown's footstep sounds by floor kind (playStepSe adds the frame
 * parity); kind 0 has none. */
// ROR 0x002ebb40
static const int s_bwnStepSe[12] = {
    -100, 175, 169, 171, 189, 185, 187, 183, 173, 181, 179, 191,
};

/* The neck's nodes (the sub motions of updateLook / setNeckMotion). */
// ROR 0x00297408
static const char *s_neckNodes[] = {
    "Bip01 Neck", "Bip01 Neck1", "Bip01 Head", NULL,
};

// ROR 0x0029812c
static const char *s_smellMotion = "smell";

/* The look octants' motions (updateLook's v; setNeckMotion's copy adds
 * "wait"). */
// ROR 0x00298108
static const char *s_neckMotions[9] = {
    "neckblend3", "neckblend4", "neckblend5", "neckblend6",
    "neckblend7", "neckblend8", "neckblend1", "neckblend2", "wait",
};

/* followPath's bend motions: the walking rows it turns from, then two
 * turn rows (by the bend's side) for each, the sharpness added. */
// ROR 0x002eb980
static const short s_pathWalkSeq[4] = { 87, 78, 96, 69 };
// ROR 0x002eb988
static const short s_pathTurnSeq[8] = { 88, 91, 79, 82, 97, 100, 70, 73 };

/* The motion sequence rows that move without the body test. */
static inline bool isNoBodyTestSeq(int seq)
{
    return seq == 0x43 || seq == 0x69 || seq == 0x6b;
}

/* The point at a 16.16 index of the search line (the ROM repeats this
 * inline): the point itself, or a lerp towards the next. */
static void getSearchLinePos(const MagLifeBwn *bwn, u_int idx, float *out)
{
    u_int frac = idx & 0xffff;
    const float *p = bwn->m_search_line[idx >> 16];
    if (frac == 0)
    {
        out[0] = p[0];
        out[1] = p[1];
        out[2] = p[2];
    }
    else
    {
        utlLerp4((float)frac * 0x1p-16f, out, p, p + 4);
    }
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 16: the state the next applyState enters.
// ROR 0x001233d8
void MagLifeBwn::requestState(int mode, int a, int b)
{
    m_next_state[0] = mode;
    m_next_state[1] = -1;
    m_next_state[3] = b;
    m_next_state[2] = a;
}

// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x001233f4
int MagLifeBwn::changeMode(int theMode, int a, int b)
{
    ROR_ASSERT(theMode >= 0); /* 270 */
    if (m_state == theMode && theMode != 0)
        return 0;
    requestState(theMode, a, b);
    applyState(0);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The way to target: the secta cells there and under Brown, the path
// between them (GamMan::findSectaPath's request), then the search line:
// Brown's position, the cells' points, and the target as the last point
// when the way ends in its cell -- else one more point, carried on from
// the last two (a single cell: the target, when a straight line to it
// stays inside). Each point's w is the length to the next; the widths are
// the cells' narrower sides. The index starts 150 along the line.
// callcheck: FUN_00215a30, dpcmp, dpsub, dptofp, fptodp (the assert handler; the double fabs)
// ROR 0x0012349c
int MagLifeBwn::searchPath(const float *target)
{
    dbgTraceVector4(target);
    if (m_state == 6)
        clearMode();
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    getPosition(pos);
    m_goal_dist = utlVecDist(pos, target);
    m_scene->m_8542c.findCells(target, m_goal_secta, 0, 100.0f, 0.0f);
    if (m_goal_secta[0] == NULL)
        return -3;
    GamSecta **nowsct = getSectaCells();
    if (nowsct == NULL)
        ROR_ASSERT(nowsct); /* 338 */
    else if (nowsct[0] == NULL)
        return -2;
    SctPathReq req;
    req.m_list = 0;
    req.m_from = nowsct;
    req.m_to = m_goal_secta;
    req.m_path = &m_search_path;
    if (m_scene->m_8542c.m_list[0].searchPath(&req) == 0)
        return -1;
    /* (The ROM counts up to m_search_path.m_num here and uses nothing.) */
    ROR_ASSERT(m_search_path.m_num > 0); /* 400 */
    m_reach_goal = (m_search_path.m_cell[m_search_path.m_num - 1] == m_goal_secta[0]);
    getPosition(m_search_line[0]);
    for (int i = 1; i < m_search_path.m_num; i++)
    {
        const GamSecta *cell = m_search_path.m_cell[i];
        m_search_line[i][0] = cell->m_pos[0];
        m_search_line[i][1] = cell->m_pos[1];
        m_search_line[i][2] = cell->m_pos[2];
    }
    int num = m_search_path.m_num;
    if (m_reach_goal == 0)
    {
        float *last = m_search_line[num];
        if (num < 2)
        {
            if (m_scene->m_8542c.m_list[0].canWalkStraight(pos, target, 40.0f) == 0)
                return -100;
            for (int k = 0; k < 4; k++)
                last[k] = target[k];
        }
        else
        {
            const float *p1 = m_search_line[num - 1];
            const float *p2 = m_search_line[num - 2];
            for (int k = 0; k < 3; k++)
                last[k] = (p1[k] + p1[k]) - p2[k];
        }
        last[3] = 0.0f;
        m_search_path.m_num++;
    }
    else
    {
        float *last = m_search_line[num - 1];
        for (int k = 0; k < 4; k++)
            last[k] = target[k];
        last[3] = 0.0f;
    }
    for (int i = 0; i < m_search_path.m_num - 1; i++)
    {
        m_search_line[i][3] = utlVecDist(m_search_line[i], m_search_line[i + 1]);
        const GamSecta *cell = m_search_path.m_cell[i];
        /* the ROM takes both through double (fabs) */
        float w = (float)fabs((double)(cell->m_x0 - cell->m_x1));
        float d = (float)fabs((double)(cell->m_z0 - cell->m_z1));
        m_search_width[i] = (w < d) ? w : d;
    }
    m_path_end = 0;
    m_num_search_line = m_search_path.m_num;
    m_pos_index = advancePathIndex(150.0f, 0);
    return m_reach_goal == 0;
}

// ──────────────────────────────────────────────────────────────────────
// A sphere of theSize round the b_ch1 node (moved by ofs) against the
// scene. On a hit, a wall-like normal gives a step back (0, 0, -1) turned
// by Brown's heading, anything else no step.
// callcheck: FUN_00215a30, i3rGetNodeIndexByName (the assert handler; inside the inline DogModel::getNodeIndexByName)
// ROR 0x0012391c
int MagLifeBwn::checkBodyHit(float theSize, float *out, const float *ofs)
{
    ROR_ASSERT(m_actor); /* 502 */
    int node = m_actor->getNodeIndexByName("b_ch1");
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_actor->getNodeWorldPos(pos, node); /* EE slot 18 */
    if (ofs)
    {
        pos[0] += ofs[0];
        pos[1] += ofs[1];
        pos[2] += ofs[2];
        pos[3] = 1.0f;
    }
    ROR_ASSERT(m_scene); /* 514 */
    ROR_ASSERT(theSize > 0.0f); /* 516 */
    MagSceneRayHit hit;
    memset(&hit, 0, sizeof(hit)); /* PC: the ROM clears the vectors only */
    for (int i = 0; i < 5; i++)
        hit.m_vec[i][3] = 1.0f;
    if (0.0f < m_scene->castSphere(theSize, &hit, pos))
    {
        float ny = hit.m_vec[1][1];
        if (ny < -0x1.999998p-4f || 0x1.999998p-4f <= ny) /* 0x3dcccccc */
        {
            out[2] = 0.0f;
            out[3] = 1.0f;
            out[0] = 0.0f;
            out[1] = 0.0f;
        }
        else
        {
            float back[4] = { 0.0f, 0.0f, -1.0f, 1.0f };
            utlVecRotMatY(m_actor->getRotY(), back);
            for (int k = 0; k < 4; k++)
                out[k] = back[k];
        }
        return 0;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// One frame of the motion's root step. Unless the row skips it, the body
// test first: on a hit Brown steps back when a cell and no object is
// there (-1 either way). Then the step: no cell there keeps Brown in place
// (only the root's offsets applied, -3), an object stops him (-2), else he
// moves, follows the floor and keeps the cells (1).
// ROR 0x00123bb8
int MagLifeBwn::stepMotion()
{
    GamSecta *cells[4];
    float pos[4];
    int seq = getMotionSeq();
    if (!isNoBodyTestSeq(seq))
    {
        float back[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        if (checkBodyHit(10.0f, back, NULL) == 0)
        {
            DogActor *a = m_actor;
            pos[0] = a->m_trans[0] + back[0];
            pos[1] = a->m_trans[1] + back[1];
            pos[2] = a->m_trans[2] + back[2];
            pos[3] = 1.0f;
            for (int i = 0; i < 4; i++)
                cells[i] = m_secta[i];
            m_scene->m_8542c.findCells(pos, cells, m_secta[0] != NULL, 100.0f, 0.0f);
            if (cells[0] != NULL && testObjAt(back) == 0)
            {
                a = m_actor;
                a->m_trans[0] += back[0];
                a->m_trans[1] += back[1];
                a->m_trans[3] = 1.0f;
                a->m_trans[2] += back[2];
                a->m_dirty = 1;
                followGround();
                m_secta[0] = NULL;
            }
            m_move_result = -1;
            return -1;
        }
    }

    float move[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float rot[4];
    float trans_ofs_y = 0.0f;
    rot[0] = 0.0f;
    if (m_actor->getModeArg() < 0)
    {
        float frame = m_actor->getMotionFrame();
        if (0.0f <= frame)
        {
            float at[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            /* the root track keys at 120 Hz */
            m_actor->getMotionMove(frame + frame, (frame + 1.0f) + (frame + 1.0f), move, rot, at);
            trans_ofs_y = at[1];
        }
    }
    move[3] = 1.0f;
    utlVecRotMatY(m_actor->m_rotate[1], move);
    DogActor *a = m_actor;
    pos[0] = a->m_trans[0] + move[0];
    pos[1] = a->m_trans[1] + move[1];
    pos[2] = a->m_trans[2] + move[2];
    pos[3] = 1.0f;
    for (int i = 0; i < 4; i++)
        cells[i] = m_secta[i];
    m_scene->m_8542c.findCells(pos, cells, m_secta[0] != NULL, 100.0f, 0.0f);
    int result;
    if (cells[0] == NULL)
    {
        float none[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_actor->addTrans(none, trans_ofs_y, rot[0]);
        dbgTraceVector4(move);
        result = -3;
    }
    else
    {
        if (testObjAt(move))
        {
            m_move_result = -2;
            return -2;
        }
        m_actor->addTrans(move, trans_ofs_y, rot[0]);
        followGround();
        for (int i = 0; i < 4; i++)
            m_secta[i] = cells[i];
        result = 1;
    }
    m_move_result = result;
    return m_move_result;
}

// ROR 0x00123f78
u_int MagLifeBwn::testObjAt(const float *ofs)
{
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    DogActor *a = m_actor;
    pos[0] = a->m_trans[0] + ofs[0];
    pos[1] = a->m_trans[1] + ofs[1];
    pos[2] = a->m_trans[2] + ofs[2];
    u_int hit[4];
    if (utlTestDogHitAt(pos, hit, utlGetDogRadius()) == 0)
        return 0;
    u_int word = hit[0];
    if (((word >> 23) & 0x1f) != 2)
        word = 0;
    return word;
}

// ──────────────────────────────────────────────────────────────────────
// The index moves over each segment's length (the points' w); it stops
// at the last point.
// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x00124008
int MagLifeBwn::advancePathIndex(float theLength, int thePosIndex)
{
    ROR_ASSERT(0 <= thePosIndex && thePosIndex <= ((m_num_search_line-1)<<16)); /* 739 */
    int posidx = thePosIndex;
    for (;;)
    {
        ROR_ASSERT(0 <= posidx && posidx <= ((m_num_search_line-1)<<16)); /* 744 */
        int end = (m_num_search_line - 1) << 16;
        if (end <= posidx)
            return end;
        float len = m_search_line[(u_int)posidx >> 16][3];
        float rest = len * (float)(0x10000 - (posidx & 0xffff)) * 0x1p-16f;
        if (rest <= theLength)
        {
            theLength -= rest;
            posidx = (((u_int)posidx >> 16) + 1) << 16;
        }
        else
        {
            float f = theLength * 65536.0f;
            theLength = 0.0f;
            posidx += (int)(f / len);
        }
        if (theLength <= 0.0f)
            return posidx;
    }
}

// ──────────────────────────────────────────────────────────────────────
// A frame along the search line: the root step's length taken towards
// the point that far on (height differences within 10 ignored), unless
// an object or (for rows that test it) the body is in the way. The index
// runs at half the step where Brown is off the line by more than 1.5 cell
// widths, at twice the step where he is within one. On walking rows a
// bend ahead starts a turn row by its side and sharpness (dot < 0.7).
// callcheck: utlLerp4 (inside getSearchLinePos)
// ROR 0x00124150
void MagLifeBwn::followPath()
{
    if (m_path_end)
        return;
    float move[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float rot[4];
    float trans_ofs_y = 0.0f;
    rot[0] = 0.0f;
    if (m_actor->getModeArg() >= 0)
        return;
    float frame = m_actor->getMotionFrame();
    if (0.0f <= frame)
    {
        float at[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_actor->getMotionMove(frame + frame, (frame + 1.0f) + (frame + 1.0f), move, rot, at);
        trans_ofs_y = at[1];
    }
    move[3] = 1.0f;
    float step = utlVecLength(move);
    u_int idx = (u_int)advancePathIndex(step, m_pos_index);

    float d[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    getSearchLinePos(this, idx, d);
    float apos[4];
    for (int k = 0; k < 4; k++)
        apos[k] = m_actor->m_trans[k];
    d[0] -= apos[0];
    d[3] = 1.0f;
    d[1] -= apos[1];
    d[2] -= apos[2];
    if (-10.0f <= d[1] && d[1] <= 10.0f)
        d[1] = 0.0f;
    float dist = utlVecLength(d);
    if (step < dist)
    {
        float s = step / dist;
        d[0] *= s;
        d[3] = 1.0f;
        d[1] *= s;
        d[2] *= s;
    }
    else
    {
        m_path_end = 1;
    }
    float roty = utlVecToRotY(d);
    if (testObjAt(d))
    {
        m_move_result = 0;
        return;
    }
    short seq = (short)getMotionSeq();
    if (!isNoBodyTestSeq(seq))
    {
        float back[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        if (checkBodyHit(10.0f, back, d) == 0)
        {
            m_move_result = 0;
            return;
        }
    }
    m_actor->addTransRotY(d, trans_ofs_y, roty);
    followGround();
    float now[4];
    for (int k = 0; k < 4; k++)
        now[k] = m_actor->m_trans[k];
    m_scene->m_8542c.findCells(now, m_secta, m_secta[0] != NULL, 100.0f, 0.0f);

    u_int cur = (u_int)m_pos_index;
    float width = m_search_width[(int)cur >> 16];
    if (width * 1.5f < dist)
        m_pos_index = advancePathIndex(step * 0.5f, cur);
    else if (width <= dist)
        m_pos_index = idx;
    else
        m_pos_index = advancePathIndex(step + step, cur);
    m_move_result = 1;

    if (m_actor->isBlendIn())
        return;
    int i;
    for (i = 0; i < 4; i++)
        if (s_pathWalkSeq[i] == seq)
            break;
    if (i == 4)
        return;
    dbgTraceVector4(apos);
    float a[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    getSearchLinePos(this, cur, a);
    dbgTraceVector4(a);
    a[0] -= apos[0];
    a[3] = 1.0f;
    a[1] -= apos[1];
    a[2] -= apos[2];
    float b[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    getSearchLinePos(this, (u_int)m_pos_index, b);
    dbgTraceVector4(b);
    b[0] -= apos[0];
    b[3] = 1.0f;
    b[1] -= apos[1];
    b[2] -= apos[2];
    float c[4];
    c[0] = b[0] - a[0];
    c[3] = 1.0f;
    c[1] = b[1] - a[1];
    c[2] = b[2] - a[2];
    utlNormalize3(a);
    utlNormalize3(b);
    utlNormalize3(c);
    utlVecDot(a, b); /* unused */
    float dot = utlVecDot(c, a);
    if (dot != 0.0f)
    {
        float cross[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        utlVecCross(cross, a, c);
        int sharp = 2;
        if (0x1.333332p-2f <= dot) /* 0x3e999999 */
        {
            sharp = 1;
            if (0.5f <= dot)
            {
                if (0x1.666666p-1f <= dot) /* 0x3f333333 */
                    return;
                sharp = 0;
            }
        }
        changeMotionSeq(s_pathTurnSeq[i * 2 + (0.0f < cross[1])] + sharp, -1);
    }
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 14: modes 0..9 through changeMode.
// ROR 0x001247c8
int MagLifeBwn::message(u_int mode, int a, int b)
{
    int ret = 0;
    if (m_actor && mode < 10)
    {
        if (0 <= m_next_state[0] && (u_int)m_next_state[0] == mode)
            return -2;
        ret = changeMode(mode, a, b) ? 0 : -1;
    }
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// The neck sub motion theMotion over s_neckNodes, fading to theRate over
// 20 frames; the one already playing only takes the new rate (rate 0
// fades it). theMotion < 0 clears it.
// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x0012481c
void MagLifeBwn::setNeckMotion(float theRate, int theMotion)
{
    const char *motions[] = {
        "neckblend3", "neckblend4", "neckblend5", "neckblend6",
        "neckblend7", "neckblend8", "neckblend1", "neckblend2", "wait",
    };
    DogActor *a = m_actor;
    if (a == NULL)
        return;
    if (theMotion < 0)
    {
        if (a->m_motion.m_blend[0].m_no < 0)
            return;
        a->setMotionSub(0, -1, NULL);
        return;
    }
    const char *name;
    if (theRate == 0.0f)
    {
        if (a->m_motion.m_blend[0].m_no < 0)
            return;
        int no = a->getMotionNoBySuffix(motions[theMotion]);
        if (no == m_actor->m_motion.m_blend[0].m_no)
        {
            m_actor->fadeMotionRateSub(theRate, 0, 20);
            return;
        }
        name = motions[theMotion];
    }
    else
    {
        ROR_ASSERT(0 <= theMotion && theMotion < (int)(sizeof(motions)/sizeof(motions[0]))); /* 1127 */
        int no = m_actor->getMotionNoBySuffix(motions[theMotion]);
        if (no == m_actor->m_motion.m_blend[0].m_no)
        {
            m_actor->setMotionTimeRatioSub(theRate, 0);
            return;
        }
        name = motions[theMotion];
    }
    m_actor->setMotionSubBySuffix(0, name, s_neckNodes);
    m_actor->fadeMotionRateSub(theRate, 0, 20);
}

// ROR 0x0012498c
void MagLifeBwn::clearNeckMotion()
{
    m_actor->setMotionSub(0, -1, NULL);
}

// ──────────────────────────────────────────────────────────────────────
// The neck. Control 1 plays "smell"; control 2, when the look position
// changed, picks the octant of the direction from the head (60 down, 80
// ahead of Brown) to m_neck_pos in his frame, and a rate by its angle
// (none beyond 90 degrees). The neck sub motion then follows: a new
// octant fades in over 15 frames, the same one takes the rate, none winds
// the motion's ratio down by 0.1 a frame and clears it at 0.
// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x001249b4
void MagLifeBwn::updateLook()
{
    int target = -1;
    float rate = 0.0f;
    if (0 < m_neck_control)
    {
        if (m_neck_control == 1)
        {
            int cur = m_actor->m_motion.m_blend[0].m_no;
            if (0 <= cur && cur == m_actor->findMotionBySuffix(s_smellMotion))
                return;
            m_actor->setMotionSubBySuffix(0, s_smellMotion, s_neckNodes);
            return;
        }
        if (m_neck_changed == 0)
        {
            target = -2;
        }
        else
        {
            m_neck_changed = 0;
            float roty = getRotY();
            float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            getPosition(pos);
            float head[4] = { 0.0f, -60.0f, 80.0f, 1.0f };
            utlVecRotMatY(roty, head);
            pos[0] += head[0];
            pos[1] += head[1];
            pos[2] += head[2];
            pos[3] = 1.0f;
            float d[4];
            d[0] = m_neck_pos[0] - pos[0];
            d[3] = 1.0f;
            d[1] = m_neck_pos[1] - pos[1];
            d[2] = m_neck_pos[2] - pos[2];
            float r[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            utlVecRotMatY(-roty, d);
            utlVecToRot(d, r);
            const float half_pi = 0x1.921fb4p+0f; /* 0x3fc90fda */
            if (-half_pi < r[1] && r[1] < half_pi && -half_pi < r[0] && r[0] < half_pi)
            {
                float t = atan2f(r[0], r[1]);
                int v = (int)((t + 0x1.c463aep+1f) / 0x1.921fb4p-1f) & 7; /* 0x406231d7 / 0x3f490fda */
                ROR_ASSERT(v >= 0 && v < 8); /* 1247 */
                rate = utlVecLength(r) / 0x1.2d97c6p+1f; /* 0x4016cbe3, 3 pi / 4 */
                if (1.0f < rate)
                    rate = 1.0f;
                target = v;
            }
        }
    }
    if (-2 < target)
    {
        m_neck_target = target;
        m_neck_rate = rate;
    }

    DogActor *a = m_actor;
    int cur = a->m_motion.m_blend[0].m_no;
    if (cur < 0)
    {
        if (0 <= m_neck_target)
        {
            a->setMotionSubBySuffix(0, s_neckMotions[m_neck_target], s_neckNodes);
            m_actor->fadeMotionRateSub(m_neck_rate, 0, 15);
        }
        return;
    }
    a->getMotionTimeRatioSub(0); /* unused */
    float ratio;
    if (m_neck_target < 0)
    {
        ratio = m_actor->getMotionTimeRatioSub(0);
        if (ratio == 0.0f)
        {
            clearNeckMotion();
            return;
        }
        ratio -= 0x1.999998p-4f; /* 0x3dcccccc */
        if (ratio < 0.0f)
            ratio = 0.0f;
    }
    else
    {
        int no = m_actor->findMotionBySuffix(s_neckMotions[m_neck_target]);
        if (cur != no)
        {
            m_actor->setMotionSubBySuffix(0, s_neckMotions[m_neck_target], s_neckNodes);
            m_actor->fadeMotionRateSub(m_neck_rate, 0, 15);
            return;
        }
        ratio = m_neck_rate;
    }
    m_actor->setMotionTimeRatioSub(ratio, 0);
}

// ROR 0x00124d7c
void MagLifeBwn::updateLimp()
{
    const char *nodes[] = {
        "Bip01 L Clavicle", "Bip01 L UpperArm", "Bip01 L Forearm",
        "Bip01 L Hand", "Bip01 L Finger0", NULL,
    };
    DogActor *a = m_actor;
    if (m_health_status == 3)
    {
        if (a->m_motion.m_blend[1].m_no < 0)
            a->setMotionSubBySuffix(1, "w_blend_wait", nodes);
    }
    else if (0 <= a->m_motion.m_blend[1].m_no)
    {
        a->setMotionSub(1, -1, nodes);
    }
}

// ROR 0x00124e18
void MagLifeBwn::clearLegMotion()
{
    m_actor->setMotionSub(1, -1, NULL);
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 2. Suspended: only the neck. Else the frame count, the health
// class, the footsteps and the neck; no mode yet enters state 2; a
// pending state is applied, or the mode runs (and a frame without a move
// counts as unmovable); then the escape from floor without cells.
// ROR 0x00124e40
void MagLifeBwn::live()
{
    if (m_actor == NULL)
        return;
    if (m_suspend)
    {
        updateLook();
        return;
    }
    m_health_status_prev = m_health_status;
    m_04++; /* MagLife0::live, inline */
    m_health_status = calcHealth();
    playStepSe(s_bwnStepSe);
    updateLook();
    if (m_mode_cur == NULL)
        changeMode(2, 0, 0);
    if (applyState(1) == 0)
    {
        if (m_mode_cur)
            m_mode_cur->live(*this);
        if (m_move_result < 1)
        {
            m_move_result = 1;
            m_unmovable_count++;
        }
        else
        {
            m_unmovable_count = 0;
        }
    }
    escapeNoFloor();
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 12 (MagScene::resetEntities): the saved state and the goal
// cleared, state 0, no motion sequence or neck motion; a dog below a
// fifth of its maximum health gets a quarter of that fifth back (at most
// up to it), plus one.
// ROR 0x00124f38
void MagLifeBwn::reset()
{
    m_prev_state[0] = 0;
    m_prev_state[1] = 0;
    m_prev_state[2] = 0;
    m_prev_state[3] = 0;
    clearGoal();
    message(0, 0, 0); /* EE slot 14 */
    clearMotionSeq();
    setNeckMotion(0.0f, -1);
    m_unmovable_count = 0;
    m_escape = -1;
    int fifth = utlGetDogParam1c() / 5;
    int hp = utlGetDogParam1b();
    if (hp < fifth)
    {
        hp += fifth / 4;
        if (fifth < hp)
            hp = fifth;
        utlSetDogParam1b(hp + 1);
    }
    m_health_status = calcHealth();
}

// callcheck: utlLerp4 (inside getSearchLinePos)
// ROR 0x00125014
void MagLifeBwn::aimAtPath()
{
    float d[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    getSearchLinePos(this, (u_int)m_pos_index, d);
    DogActor *a = m_actor;
    d[0] -= a->m_trans[0];
    d[2] -= a->m_trans[2];
    d[3] = 1.0f;
    d[1] -= a->m_trans[1];
    m_turn_rot_y = utlVecToRotY(d);
    m_turning = 0;
}

// ──────────────────────────────────────────────────────────────────────
// A step, then the turn: within pi/20 of m_turn_rot_y is done; else a
// turn row by the angle (0x2b..0x2d to the one side by 60-degree steps,
// 0x2e..0x30 to the other), waiting for its end before the next.
// ROR 0x00125104
int MagLifeBwn::turnToPath()
{
    if (stepMotion() == -1)
    {
        changeMotionSeq(2, -1);
        return -1;
    }
    float d = m_turn_rot_y - getRotY();
    if (d < -0x1.921fb8p+1f) /* 0x40490fdc */
        d += 0x1.921fb8p+2f; /* 0x40c90fdc */
    else if (0x1.921fb8p+1f <= d)
        d -= 0x1.921fb8p+2f;
    const float pi_20 = 0x1.41b2f8p-3f; /* 0x3e20d97c */
    if (-pi_20 < d && d < pi_20)
        return 1;
    if (m_turning == 0)
    {
        const float pi_3 = 0x1.0c1524p+0f;  /* 0x3f860a92 */
        const float pi2_3 = 0x1.0c1524p+1f; /* 0x40060a92 */
        int seq;
        if (d < 0.0f)
            seq = (-pi_3 < d) ? 0x2b : (-pi2_3 < d) ? 0x2c : 0x2d;
        else
            seq = (d < pi_3) ? 0x2e : (d < pi2_3) ? 0x2f : 0x30;
        changeMotionSeq(seq, -1);
        m_turning = 1;
    }
    else if (isMotionSeqEnd())
    {
        m_turning = 0;
    }
    return 0;
}

// ROR 0x001252c4
int MagLifeBwn::calcHealth()
{
    int hp = utlGetDogParam1b();
    int max = utlGetDogParam1c();
    float r = (float)hp / (float)max;
    if (r <= 0.0f)
        return 4;
    if (r < 0x1.999998p-3f) /* 0x3e4ccccc */
        return 3;
    if (r < 0x1.999998p-2f) /* 0x3ecccccc */
        return 2;
    if (r < 0x1.999998p-1f) /* 0x3f4ccccc */
        return 1;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Brown stands where no secta cell is (m_escape -1 queries first). The
// low half counts probes, each 1/4 further out, along +x, -x, +z, -z in
// turn; the first with a cell gives the direction (1..4) in the high
// half, which Brown then walks a unit a frame until a cell is under him,
// for at most 100 steps (then 5: given up).
// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x00125368
void MagLifeBwn::escapeNoFloor()
{
    DogActor *a = m_actor;
    float pos[4];
    for (int k = 0; k < 4; k++)
        pos[k] = a->m_trans[k];
    if (m_escape < 0)
    {
        m_scene->m_8542c.findCells(pos, m_secta, 0, 100.0f, 0.0f);
        m_escape = 0;
    }
    u_int v = (u_int)m_escape;
    int dir = (int)v >> 16;
    if (v == 0 && m_secta[0] != NULL)
        return;
    float dx = 0.0f;
    float dz = 0.0f;
    if (dir == 0)
    {
        u_int k = v & 3;
        float dist = (float)((int)(v & 0xffff) >> 2);
        switch (k)
        {
        case 0: dx = dist; break;
        case 1: dx = -dist; break;
        case 2: dz = dist; break;
        default: dz = -dist; break;
        }
        pos[0] += dx;
        pos[2] += dz;
        pos[1] += 0.0f;
        pos[3] = 1.0f;
        GamSecta *cells[4];
        m_scene->m_8542c.findCells(pos, cells, 0, 100.0f, 0.0f);
        if (cells[0] == NULL)
        {
            m_escape++;
            return;
        }
        m_escape = (k + 1) * 0x10000;
        return;
    }
    if (4 < dir)
    {
        if (dir == 5)
            return;
        ROR_ASSERT(0); /* 1895 */
        return;
    }
    m_scene->m_8542c.findCells(pos, m_secta, 0, 100.0f, 0.0f);
    if (m_secta[0] != NULL)
    {
        m_escape = 0;
        return;
    }
    switch (dir)
    {
    case 1: dx = 1.0f; break;
    case 2: dx = -1.0f; break;
    case 3: dz = 1.0f; break;
    case 4: dz = -1.0f; break;
    }
    a = m_actor;
    a->m_trans[0] += dx;
    a->m_trans[1] += 0.0f;
    a->m_trans[3] = 1.0f;
    a->m_trans[2] += dz;
    a->m_dirty = 1;
    m_escape++;
    if (99 < (v & 0xffff))
        m_escape = 0x50000;
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 10. The ROM stores the NULL actor before it clears the neck
// motion, so that clear runs on NULL.
// callcheck: clearNeckMotion (skipped on the NULL actor, see the PC: note)
// ROR 0x001256d4
void MagLifeBwn::setActor(DogActor *actor, int no, int mode)
{
    m_mode = mode;
    m_no = no;
    m_actor = actor;
    /* PC: clearNeckMotion on the NULL actor (the ROM's call when actor is
     * NULL) is skipped. */
}

// ──────────────────────────────────────────────────────────────────────
// The pending state enters once the current mode's term agrees (state 0
// enters regardless); the old state is saved as setState saves it.
// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x00125704
int MagLifeBwn::applyState(int force)
{
    int next = m_next_state[0];
    if (next < 0)
        return 0;
    int ok = 1;
    if (m_mode_cur)
        ok = m_mode_cur->term(*this, force);
    if (ok == 0 && next != 0)
        return 1;
    if (m_state != 0)
    {
        m_prev_state[0] = m_state;
        m_prev_state[1] = m_04;
        m_prev_state[2] = m_08;
        m_prev_state[3] = m_0c;
    }
    m_state = m_next_state[0];
    m_04 = m_next_state[1];
    m_08 = m_next_state[2];
    m_0c = m_next_state[3];
    m_next_state[0] = -1;
    m_next_state[1] = -1;
    switch (m_state)
    {
    case 0: m_mode_cur = &m_mode_null; break;
    case 1: m_mode_cur = &m_mode_one; break;
    case 2: m_mode_cur = &m_mode_wait; break;
    case 3: m_mode_cur = &m_mode_sit; break;
    case 4: m_mode_cur = &m_mode_fuse; break;
    case 5: m_mode_cur = &m_mode_move; break;
    case 6: m_mode_cur = &m_mode_path; break;
    case 7: m_mode_cur = &m_mode_turn; break;
    case 8: m_mode_cur = &m_mode_rotate; break;
    case 9: m_mode_cur = &m_mode_wall; break;
    default:
        m_mode_cur = NULL;
        ROR_ASSERT(0); /* 1987 */
        break;
    }
    m_move_result = 1;
    m_unmovable_count = 0;
    if (m_mode_cur)
        m_mode_cur->init(*this);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The motion row's class decides how Brown holds still: 0x10 plays
// "wait", 0x30 (and no row) stops the motion.
// ROR 0x001258a4
void MagLifeBwn::suspend()
{
    m_suspend = 1;
    if (m_actor)
    {
        m_suspend_motion = m_actor->m_motion.getNo();
        int seq = getMotionSeq();
        u_int cls;
        if (seq < 0)
            cls = 0x30;
        else if (m_motion_seq == NULL)
            cls = 0x30; /* PC: the row table is not ported yet (the constructor) */
        else
            cls = m_motion_seq[seq].m_flags & 0xf0;
        if (cls == 0x10)
        {
            int no = m_actor->getMotionNoBySuffix("wait");
            m_actor->changeMotion(0x1.11112ep-6f, no, 0, -1, -1); /* 0x3c888897 */
        }
        else if (cls == 0x30)
        {
            m_actor->m_motion.m_speed = 0.0f;
        }
    }
    setNeckControl(0);
}

// ROR 0x00125980
void MagLifeBwn::resume()
{
    m_suspend = 0;
    if (m_actor)
    {
        if (m_actor->m_motion.getNo() != m_suspend_motion)
        {
            clearMode();
            return;
        }
        m_actor->m_motion.m_speed = 0x1.11112ep-6f; /* 0x3c888897, 1/60 */
    }
}

// ROR 0x001259e8
void MagLifeBwn::renderPostProcess()
{
    if (m_actor && m_suspend == 0)
        updateMotionSeq();
}

// ROR 0x00125a20
void MagLifeBwn::setNeckControl(int mode)
{
    if (m_neck_control != mode)
    {
        m_neck_control = mode;
        m_neck_changed = 1;
    }
}

// ROR 0x00125a3c
void MagLifeBwn::setNeckPos(const float *pos)
{
    if (m_neck_pos[0] == pos[0] && m_neck_pos[1] == pos[1] && m_neck_pos[2] == pos[2])
        return;
    m_neck_changed = 1;
    m_neck_pos[0] = pos[0];
    m_neck_pos[1] = pos[1];
    m_neck_pos[2] = pos[2];
    m_neck_pos[3] = pos[3];
}

// ROR 0x00125a90
void MagLifeBwn::clearMode()
{
    m_mode_cur = NULL;
    m_state = -1;
    m_04 = -1;
}

// ──────────────────────────────────────────────────────────────────────
// The modes' vptrs are the host's (the ROM stores the ten EE vtables).
// callcheck: MagLifeControlChara (the base constructor, run implicitly)
// ROR 0x00125aa4
MagLifeBwn::MagLifeBwn()
{
    m_next_state[0] = 0;
    m_next_state[1] = 0;
    m_next_state[2] = 0;
    m_next_state[3] = 0;
    m_neck_pos[2] = 0.0f;
    m_neck_pos[3] = 1.0f;
    m_neck_pos[0] = 0.0f;
    m_neck_pos[1] = 0.0f;
    for (int i = 0; i < MAX_SEARCH_LINE; i++)
    {
        m_search_line[i][2] = 0.0f;
        m_search_line[i][3] = 1.0f;
        m_search_line[i][1] = 0.0f;
        m_search_line[i][0] = 0.0f;
    }
    m_neck_target = -1;
    m_neck_rate = 0.0f;
    m_health_status = 0;
    m_health_status_prev = 0;
    m_escape = 0;
    m_neck_changed = 0;
    m_suspend = 0;
    m_mode_cur = NULL;
    setMotionSeqTable(g_bwn_motion_seq); /* 0x297418 */
}

// ──────────────────────────────────────────────────────────────────────
// Brown's motion sequence rows (MagLifeMotionSeq: id, motion, blend,
// frames, speed, _0e, flags, next state, se); the constructor sets them.
// 138 x 0x18 bytes at 0x00297418 (datadump.py)
const MagLifeMotionSeq g_bwn_motion_seq[138] = {
    { 0, "base", -1, -1, 0, -1, 0, -1, -1 },  // 0x00297418
    { 1, "wait", -1, -1, 0, -1, 0, -1, -1 },  // 0x00297430
    { 2, "wait", -1, -1, 0, -1, 0, -1, -1 },  // 0x00297448
    { 3, "free1", -1, 1, 0, -1, 0, 2, -1 },  // 0x00297460
    { 4, "free2", -1, 1, 0, -1, 0, 2, -1 },  // 0x00297478
    { 5, "free3", -1, 1, 0, -1, 0, 2, -1 },  // 0x00297490
    { 6, "free4", -1, 1, 0, -1, 0, 2, -1 },  // 0x002974a8
    { 7, "sit_in", -1, 1, 0, -1, 0x30, 8, -1 },  // 0x002974c0
    { 8, "sit_wait", -1, -1, 0, -1, 0, -1, -1 },  // 0x002974d8
    { 9, "sit_out", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x002974f0
    { 10, "sit_nade", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297508
    { 11, "sit_happy", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297520
    { 12, "fuse_in", -1, 1, 0, -1, 0x30, 13, -1 },  // 0x00297538
    { 13, "fuse_wait", -1, -1, 0, -1, 0, -1, -1 },  // 0x00297550
    { 14, "fuse_out", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297568
    { 15, "fuse_sit", -1, 1, 0, -1, 0, 8, -1 },  // 0x00297580
    { 16, "sit_fuse", -1, 1, 0, -1, 0, 13, -1 },  // 0x00297598
    { 17, "tired", -1, -1, 0, -1, 0, -1, -1 },  // 0x002975b0
    { 18, "look", -1, 1, 0, -1, 0x30, 2, 8 },  // 0x002975c8
    { 19, "smell", -1, -1, 0, -1, 0, -1, -1 },  // 0x002975e0
    { 20, "tail", -1, -1, 0, -1, 0, -1, -1 },  // 0x002975f8
    { 21, "tail_down", -1, -1, 0, -1, 0, -1, -1 },  // 0x00297610
    { 22, "tail_fuse", -1, -1, 0, -1, 0, -1, -1 },  // 0x00297628
    { 23, "tail_sit", -1, -1, 0, -1, 0, -1, -1 },  // 0x00297640
    { 24, "shock", -1, 1, 0, -1, 0x30, -1, 5 },  // 0x00297658
    { 25, "alert", -1, 1, 0, -1, 0x30, -1, 10 },  // 0x00297670
    { 26, "alert_lo", -1, 1, 0, -1, 0x30, -1, 10 },  // 0x00297688
    { 27, "alert_hi", -1, 1, 0, -1, 0x30, -1, 10 },  // 0x002976a0
    { 28, "alert", -1, 3, 0, -1, 0, 2, 18 },  // 0x002976b8
    { 29, "wall_in", -1, 1, 0, -1, 0, 30, -1 },  // 0x002976d0
    { 30, "wall", -1, -1, 0, -1, 0, -1, 19 },  // 0x002976e8
    { 31, "wall_in", -1, 1, 0, -1, 0, 32, -1 },  // 0x00297700
    { 32, "wall_wait1", -1, -1, 0, -1, 0, -1, -1 },  // 0x00297718
    { 33, "wall_in", -1, 1, 0, -1, 0, 34, -1 },  // 0x00297730
    { 34, "wall_wait2", -1, -1, 0, -1, 0, -1, 11 },  // 0x00297748
    { 35, "wall_in", -1, 1, 0, -1, 0, 36, -1 },  // 0x00297760
    { 36, "wall_wait3", -1, -1, 0, -1, 0, -1, 5 },  // 0x00297778
    { 37, "wall_out", -1, 1, 0, -1, 0, -1, -1 },  // 0x00297790
    { 38, "bark", -1, -1, 0, -1, 0, -1, -1 },  // 0x002977a8
    { 39, "bark", -1, 1, 0, -1, 0, 2, 11 },  // 0x002977c0
    { 40, "bark_lo", -1, -1, 0, -1, 0, -1, -1 },  // 0x002977d8
    { 41, "bark_hi", -1, -1, 0, -1, 0, -1, -1 },  // 0x002977f0
    { 42, "bark", -1, 1, 0, -1, 0, -1, 11 },  // 0x00297808
    { 43, "turn_l", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297820
    { 44, "turn_l_90", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297838
    { 45, "turn_l180", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297850
    { 46, "turn_r", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297868
    { 47, "turn_r_90", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297880
    { 48, "turn_r180", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297898
    { 49, "turn_b", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x002978b0
    { 50, "s_fuse_in", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x002978c8
    { 51, "s_fuse_off", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x002978e0
    { 52, "s_sit_in", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x002978f8
    { 53, "s_sit_off", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297910
    { 54, "s_slide_l", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297928
    { 55, "s_slide_r", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297940
    { 56, "s_turn_b", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297958
    { 57, "s_turn_bl", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297970
    { 58, "s_turn_br", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297988
    { 59, "s_turn_l_90", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x002979a0
    { 60, "s_turn_l", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x002979b8
    { 61, "s_turn_r_90", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x002979d0
    { 62, "s_turn_r", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x002979e8
    { 63, "s_turn_r180", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297a00
    { 64, "s_turn_l180", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297a18
    { 65, "s_wait", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297a30
    { 66, "s_walk_b", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297a48
    { 67, "walk_b", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297a60
    { 68, "walk_b", -1, 1, 0, -1, 0x30, 2, -1 },  // 0x00297a78
    { 69, "s_walk", -1, -1, 0, -1, 0x31, -1, -1 },  // 0x00297a90
    { 70, "s_turn_fl30", -1, 1, 0, -1, 0x31, 69, -1 },  // 0x00297aa8
    { 71, "s_turn_fl60", -1, 1, 0, -1, 0x31, 69, -1 },  // 0x00297ac0
    { 72, "s_turn_fl90", -1, 1, 0, -1, 0x31, 69, -1 },  // 0x00297ad8
    { 73, "s_turn_fr30", -1, 1, 0, -1, 0x31, 69, -1 },  // 0x00297af0
    { 74, "s_turn_fr60", -1, 1, 0, -1, 0x31, 69, -1 },  // 0x00297b08
    { 75, "s_turn_fr90", -1, 1, 0, -1, 0x31, 69, -1 },  // 0x00297b20
    { 76, "s_turn_180", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297b38
    { 77, "s_walk_stop", -1, 1, 0, -1, 0x31, -1, -1 },  // 0x00297b50
    { 78, "walk", -1, -1, 0, -1, 0x31, -1, -1 },  // 0x00297b68
    { 79, "turn_fl_30", -1, 1, 0, -1, 0x31, 78, -1 },  // 0x00297b80
    { 80, "turn_fl_60", -1, 1, 0, -1, 0x31, 78, -1 },  // 0x00297b98
    { 81, "turn_fl_90", -1, 1, 0, -1, 0x31, 78, -1 },  // 0x00297bb0
    { 82, "turn_fr_30", -1, 1, 0, -1, 0x31, 78, -1 },  // 0x00297bc8
    { 83, "turn_fr_60", -1, 1, 0, -1, 0x31, 78, -1 },  // 0x00297be0
    { 84, "turn_fr_90", -1, 1, 0, -1, 0x31, 78, -1 },  // 0x00297bf8
    { 85, "walk_turn_180", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297c10
    { 86, "walk_stop", -1, 1, 0, -1, 0x31, -1, -1 },  // 0x00297c28
    { 87, "run", -1, -1, 0, -1, 0x31, -1, -1 },  // 0x00297c40
    { 88, "run_turn_fl30", -1, 1, 0, -1, 0x31, 87, -1 },  // 0x00297c58
    { 89, "run_turn_fl60", -1, 1, 0, -1, 0x31, 87, -1 },  // 0x00297c70
    { 90, "run_turn_fl90", -1, 1, 0, -1, 0x31, 87, -1 },  // 0x00297c88
    { 91, "run_turn_fr30", -1, 1, 0, -1, 0x31, 87, -1 },  // 0x00297ca0
    { 92, "run_turn_fr60", -1, 1, 0, -1, 0x31, 87, -1 },  // 0x00297cb8
    { 93, "run_turn_fr90", -1, 1, 0, -1, 0x31, 87, -1 },  // 0x00297cd0
    { 94, "run_turn_180", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297ce8
    { 95, "run_stop", -1, 1, 0, -1, 0x31, -1, -1 },  // 0x00297d00
    { 96, "dash", -1, -1, 0, -1, 0x31, -1, -1 },  // 0x00297d18
    { 97, "dash_turn_fl30", -1, 1, 0, -1, 0x31, 96, -1 },  // 0x00297d30
    { 98, "dash_turn_fl60", -1, 1, 0, -1, 0x31, 96, -1 },  // 0x00297d48
    { 99, "dash_turn_fl90", -1, 1, 0, -1, 0x31, 96, -1 },  // 0x00297d60
    { 100, "dash_turn_fr30", -1, 1, 0, -1, 0x31, 96, -1 },  // 0x00297d78
    { 101, "dash_turn_fr60", -1, 1, 0, -1, 0x31, 96, -1 },  // 0x00297d90
    { 102, "dash_turn_fr90", -1, 1, 0, -1, 0x31, 96, -1 },  // 0x00297da8
    { 103, "dash_turn_180", -1, 1, 0, -1, 0x30, -1, -1 },  // 0x00297dc0
    { 104, "dash_stop", -1, 1, 0, -1, 0x31, -1, -1 },  // 0x00297dd8
    { 105, "dan_up_lo", -1, -1, 0, -1, 0x30, -1, -1 },  // 0x00297df0
    { 106, "dan_up_wait_lo", -1, -1, 0, -1, 0x30, -1, -1 },  // 0x00297e08
    { 107, "dan_down_lo", -1, -1, 0, -1, 0x30, -1, -1 },  // 0x00297e20
    { 108, "dan_down_wait_lo", -1, -1, 0, -1, 0x30, -1, -1 },  // 0x00297e38
    { 109, "dmg_big", -1, 1, 0, -1, 0x30, -1, 15 },  // 0x00297e50
    { 110, "dmg_death", -1, 1, 0, -1, 0x30, 111, -1 },  // 0x00297e68
    { 111, "dmgl_wait", -1, 1, 0, -1, 0x30, 112, -1 },  // 0x00297e80
    { 112, "dmgl_out", -1, 1, 0, -1, 0x30, -1, 11 },  // 0x00297e98
    { 113, "dmgb_in", -1, 1, 0, -1, 0x30, 111, 16 },  // 0x00297eb0
    { 114, "alert_lo", -1, 1, 0, -1, 0x30, 115, 10 },  // 0x00297ec8
    { 115, "bite_in_lo", -1, 1, 0, -1, 0x30, 116, 17 },  // 0x00297ee0
    { 116, "bite_wait_lo", -1, -1, 0, -1, 0, -1, 18 },  // 0x00297ef8
    { 117, "bite_out_lo", -1, 1, 0, -1, 0x30, 126, -1 },  // 0x00297f10
    { 118, "alert", -1, 1, 0, -1, 0x30, 119, 10 },  // 0x00297f28
    { 119, "bite_in_mi", -1, 1, 0, -1, 0x30, 120, 17 },  // 0x00297f40
    { 120, "bite_wait_mi", -1, -1, 0, -1, 0, -1, 18 },  // 0x00297f58
    { 121, "bite_out_mi", -1, 1, 0, -1, 0x30, 126, -1 },  // 0x00297f70
    { 122, "alert_hi", -1, 1, 0, -1, 0x30, 123, 10 },  // 0x00297f88
    { 123, "bite_in_hi", -1, 1, 0, -1, 0x30, 124, 17 },  // 0x00297fa0
    { 124, "bite_wait_hi", -1, -1, 0, -1, 0, -1, 18 },  // 0x00297fb8
    { 125, "bite_out_hi", -1, 1, 0, -1, 0x30, 126, -1 },  // 0x00297fd0
    { 126, "alert", -1, 1, 0, -1, 0, -1, 10 },  // 0x00297fe8
    { 127, "naze_f", -1, 1, 0, -1, 0x30, -1, 7 },  // 0x00298000
    { 128, "naze_front", -1, 1, 0, -1, 0x30, -1, 7 },  // 0x00298018
    { 129, "naze_l", -1, 1, 0, -1, 0x30, -1, 7 },  // 0x00298030
    { 130, "dmg_death", -1, 1, 0, -1, 0, 131, -1 },  // 0x00298048
    { 131, "dmgl_wait", -1, -1, 0, -1, 0, -1, -1 },  // 0x00298060
    { 132, "find_lost", -1, 10, 0, -1, 0x10, -1, -1 },  // 0x00298078
    { 133, "find_lost", -1, 10, 0, -1, 0x10, -1, -1 },  // 0x00298090
    { 134, "find_lost", -1, 10, 0, -1, 0x10, -1, -1 },  // 0x002980a8
    { 135, "find_lost", -1, 10, 0, -1, 0x10, -1, -1 },  // 0x002980c0
    { 136, "find_lost", -1, 10, 0, -1, 0x10, -1, -1 },  // 0x002980d8
    { 137, "find_lost", -1, 10, 0, -1, 0x10, -1, -1 },  // 0x002980f0
};
