// mag/maglife.cpp -- the life objects' bound-actor controller
// (MagLifeControlChara: states, the motion sequence rows, the floor and
// footstep queries) and MagLifeBillboard.

#include "common/ror_assert.h"
#include "mag/maglife.h"

#include <string.h>

#include "dog/dogactor.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "sct/sctlayer.h"
#include "sdk/libvu0.h"
#include "utl/utlgame.h"
#include "utl/utlmath.h"

// ──────────────────────────────────────────────────────────────────────
// The bound-actor controller's constructor (MagLifePC and MagLifeBwn run
// it first). MagLife0's part is inlined: level 1, the state quad cleared,
// m_04 -1.
// ROR 0x0011a888
MagLifeControlChara::MagLifeControlChara()
{
    m_level = 1;
    m_state = 0;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    for (int i = 0; i < 4; i++)
        m_prev_state[i] = 0;
    m_04 = -1;
    m_state = 0;
    m_hide = 0;
    m_scene = NULL;
    m_seq_no = 0;
    m_seq_count = 0;
    m_48 = 0;
    m_4c = 0;
    m_no = -1;
    m_secta[0] = NULL;
    m_mode = 0;
    m_actor = NULL;
    m_74 = 0;
    m_78 = 0;
    clearMotionSeq();
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 9: enters state with arg. A running state (m_state != 0) is
// saved first, the whole quad m_state..m_0c (MagLifePC::restoreState
// brings it back).
// ROR 0x00119fe0
void MagLifeControlChara::setState(int state, int arg)
{
    if (m_state != 0)
    {
        m_prev_state[0] = m_state;
        m_prev_state[1] = m_04;
        m_prev_state[2] = m_08;
        m_prev_state[3] = m_0c;
    }
    m_08 = arg;
    m_state = state;
    m_04 = -1;
}

// ROR 0x00119b34
void MagLifeControlChara::clearGoal()
{
    m_70 = -1;
    memset(m_secta, 0, sizeof(m_secta));
}

// ROR 0x0011a130
void MagLifeControlChara::clearMotionSeq()
{
    m_seq_no = -1;
    m_seq_count = -1;
}

// ROR 0x0011a144
void MagLifeControlChara::setMotionSeqTable(const MagLifeMotionSeq *table)
{
    m_motion_seq = table;
}

// ROR 0x00119f88
void MagLifeControlChara::getPosition(float *out)
{
    DogActor *actor = m_actor;
    if (actor)
    {
        out[0] = actor->m_trans[0];
        out[1] = actor->m_trans[1];
        out[2] = actor->m_trans[2];
        out[3] = actor->m_trans[3];
    }
}

// Written by the mag/maglifebwnctrl.cpp session (MagLifeBwnCtrlModeFind::init).
// ROR 0x00119fa4
float *MagLifeControlChara::getTransPtr()
{
    return m_actor->m_trans;
}

// ROR 0x00119fb0
float MagLifeControlChara::getRotY()
{
    if (m_actor)
        return m_actor->getRotY();
    return 0.0f;
}

// ROR 0x0011a010
void MagLifeControlChara::setMotionSeqNo(int no)
{
    m_seq_no = no;
    m_seq_count = 0;
    m_48 = 0;
    m_4c = 0;
}

// ROR 0x0011a028
int MagLifeControlChara::getMotionSeq()
{
    return m_seq_no;
}

// ROR 0x0011a054
int MagLifeControlChara::isMotionSeqEnd()
{
    return 1 < m_48;
}

// PC: the two secta helpers below were written by the mag/maglifebwn.cpp
// session (2026-10-27): Brown's path search and the PC's moveBy need them.

// ──────────────────────────────────────────────────────────────────────
// The secta cells under the actor (radius 40), searched from the cells
// held when the first one is set.
// ROR 0x0011a6b0
void MagLifeControlChara::updateSectaCells()
{
    float pos[4];
    for (int i = 0; i < 4; i++)
        pos[i] = m_actor->m_trans[i];
    m_scene->m_8542c.findCells(pos, m_secta, m_secta[0] != NULL, 40.0f, 0.0f);
}

// ──────────────────────────────────────────────────────────────────────
// m_secta, queried first when it holds no cell.
// ROR 0x0011a64c
GamSecta **MagLifeControlChara::getSectaCells()
{
    if (m_secta[0] == NULL)
    {
        float pos[4];
        for (int i = 0; i < 4; i++)
            pos[i] = m_actor->m_trans[i];
        m_scene->m_8542c.findCells(pos, m_secta, 0, 40.0f, 0.0f);
    }
    return m_secta;
}

// ──────────────────────────────────────────────────────────────────────
// The floor ahead with its height: the cell under the point (radius 180)
// and, on a 0x20000 cell, the floor's height there against the point's.
// ROR 0x00119b60
int MagLifeControlChara::testFloorStep(float ahead, float *out)
{
    DogActor *actor = m_actor;
    if (!actor)
        return -1;
    sceVu0FVECTOR pos;
    pos[0] = actor->m_trans[0];
    pos[1] = actor->m_trans[1];
    pos[2] = actor->m_trans[2];
    pos[3] = actor->m_trans[3];
    if (ahead != 0.0f)
    {
        sceVu0FVECTOR v = {0.0f, 0.0f, ahead, 1.0f};
        utlVecRotMatY(actor->m_rotate[1], v);
        pos[0] += v[0];
        pos[2] += v[2];
        pos[3] = 1.0f;
        pos[1] += v[1];
        if (out)
        {
            out[0] = pos[0];
            out[1] = pos[1];
            out[2] = pos[2];
            out[3] = 1.0f;
        }
    }
    GamSecta *hit[4];
    memset(hit, 0, sizeof(hit));
    hit[0] = m_secta[0];
    m_scene->m_8542c.findCells(pos, hit, hit[0] != NULL, 180.0f, 0.0f);
    if (hit[0] == NULL)
        return -1;
    if (!(hit[0]->m_flags & 0x20000))
        return 0;
    float y = m_scene->m_8542c.m_list[0].getCellY(hit[0]->m_id, pos, 0.0f);
    if (y - pos[1] > 10.0f)
        return 3;
    if (y - pos[1] < -10.0f)
        return 2;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The floor ahead: the cell under the point (radius 40), started from the
// cell held in m_secta[0].
// ROR 0x00119cec
int MagLifeControlChara::testFloor(float ahead, float *out)
{
    DogActor *actor = m_actor;
    if (!actor)
        return -1;
    sceVu0FVECTOR pos;
    pos[0] = actor->m_trans[0];
    pos[1] = actor->m_trans[1];
    pos[2] = actor->m_trans[2];
    pos[3] = actor->m_trans[3];
    if (ahead != 0.0f)
    {
        sceVu0FVECTOR v = {0.0f, 0.0f, ahead, 1.0f};
        utlVecRotMatY(actor->m_rotate[1], v);
        pos[0] += v[0];
        pos[2] += v[2];
        pos[3] = 1.0f;
        pos[1] += v[1];
        if (out)
        {
            out[0] = pos[0];
            out[1] = pos[1];
            out[2] = pos[2];
            out[3] = 1.0f;
        }
    }
    GamSecta *hit[4];
    memset(hit, 0, sizeof(hit));
    hit[0] = m_secta[0];
    m_scene->m_8542c.findCells(pos, hit, hit[0] != NULL, 40.0f, 0.0f);
    if (hit[0] == NULL)
        return -1;
    if (hit[0]->m_flags & 0x20000)
        return 2;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Keeps the actor on the ground: a ray from 60 above it, 120 long; the hit
// at t moves it by (t - 0.5) * 60, at most 7 a call. Returns the step.
// ROR 0x00119e1c
float MagLifeControlChara::followGround()
{
    DogActor *actor = m_actor;
    sceVu0FVECTOR pos;
    pos[0] = actor->m_trans[0];
    pos[1] = actor->m_trans[1] - 60.0f;
    pos[2] = actor->m_trans[2];
    pos[3] = actor->m_trans[3];
    sceVu0FVECTOR dir = {0.0f, 120.0f, 0.0f, 1.0f};
    MagSceneRayHit hit;
    for (int i = 0; i < 5; i++)
    {
        hit.m_vec[i][0] = 0.0f;
        hit.m_vec[i][1] = 0.0f;
        hit.m_vec[i][2] = 0.0f;
        hit.m_vec[i][3] = 1.0f;
    }
    hit.m_50 = -1;
    float t = m_scene->castRay(&hit, pos, dir, -1);
    float d = 0.0f;
    if (0.0f <= t && t <= 1.0f)
        d = (t - 0.5f) * 60.0f;
    float step;
    if (d < -7.0f)
        step = -7.0f;
    else if (7.0f < d)
        step = 7.0f;
    else
        step = d;
    if (step != 0.0f)
    {
        actor = m_actor;
        actor->m_dirty = 1;
        actor->m_trans[1] += step;
    }
    return step;
}

// ROR 0x0011a030
void MagLifeControlChara::endMotionSeqPhase()
{
    m_48 = 1;
}

// ROR 0x0011a03c
void MagLifeControlChara::setMotionSeqEnd()
{
    m_48 = 2;
}

// ROR 0x0011a048
int MagLifeControlChara::isMotionSeqStarted()
{
    return 0 < m_48;
}

// ──────────────────────────────────────────────────────────────────────
// Follows the rows from m_seq_no along m_next_state: 1 at an endless row,
// 0 where the chain ends.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011a064
int MagLifeControlChara::isMotionStateEndless()
{
    const MagLifeMotionSeq *m_tbl_motion_state = m_motion_seq;
    int cnt = 0;
    int st = getMotionSeq();
    /* PC: the ROM reads row -1 (the words before the table) for a cleared
     * sequence; the host stops there. */
    if (st < 0)
        return 0;
    for (;;)
    {
        if (m_tbl_motion_state[st].m_frames < 0)
            return 1;
        ROR_ASSERT(m_tbl_motion_state[st].m_next_state != st); /* 336 */
        st = m_tbl_motion_state[st].m_next_state;
        if (st < 0)
            return 0;
        cnt++;
        ROR_ASSERT(cnt < 10); /* 343 */
    }
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 15's per-frame step (renderPostProcess): once the row's motion
// runs, counts its frames (m_frames > 0, while the actor is in no mode)
// or the motion's loops (with the row's sound each loop), and moves to
// m_next_state after m_frames, or ends the chain.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011a14c
void MagLifeControlChara::updateMotionSeq()
{
    ROR_ASSERT(m_actor); /* 367 */
    if (isMotionSeqEnd() || !isMotionSeqStarted())
        return;
    int st = getMotionSeq();
    const MagLifeMotionSeq *row = &m_motion_seq[st];
    if (row->m_frames < 1)
    {
        if (m_actor->m_motion_result != 0)
        {
            if (0 < row->m_se)
                playSe(row->m_se);
            m_seq_count++;
        }
        return;
    }
    if (m_actor->isModeSet())
        return;
    m_seq_count++;
    row = &m_motion_seq[st];
    if (row->m_frames <= m_seq_count)
    {
        int next = row->m_next_state;
        if (next < 0)
        {
            setMotionSeqEnd();
            return;
        }
        changeMotionSeq(next, -1);
    }
}

// ──────────────────────────────────────────────────────────────────────
// changeMotionSeq of theState's next state, or of theState itself when it
// has none.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011a27c
void MagLifeControlChara::changeMotionSeqAlt(int theState, int blend)
{
    ROR_ASSERT(theState >= 0); /* 419 */
    int next = m_motion_seq[theState].m_next_state;
    if (next >= 0)
    {
        changeMotionSeq(next, blend);
        return;
    }
    changeMotionSeq(theState, blend);
}

// ──────────────────────────────────────────────────────────────────────
// Starts row theState: its motion at m_speed percent of 1/60 a frame
// (flag 1 for a counted-loop row, 0x40 for flag bit 0), blend < 0 taking
// the row's blend (0 becomes 5 while the actor still blends in), and the
// row's sound. Nothing while the row already runs; row 0 only resets.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011a320
void MagLifeControlChara::changeMotionSeq(int theState, int blend)
{
    const MagLifeMotionSeq *m_tbl_motion_state = m_motion_seq;
    ROR_ASSERT(theState >= 0); /* 436 */
    if (!isMotionSeqEnd() && getMotionSeq() == theState)
        return;
    setMotionSeqNo(theState);
    if (theState < 1)
        return;
    ROR_ASSERT(m_tbl_motion_state[theState].m_state_id == theState); /* 443 */
    const MagLifeMotionSeq *row = &m_tbl_motion_state[theState];
    float speed = 0x1.11112ep-6f; /* 0x3c888897, 1/60 */
    u_int flags = row->m_frames < 1;
    if (row->m_flags & 1)
        flags |= 0x40;
    if (row->m_speed != 0)
        speed = ((float)row->m_speed * 0x1.11112ep-6f) / 100.0f;
    if (blend < 0)
    {
        blend = row->m_blend;
        ROR_ASSERT(m_actor); /* 467 */
        if (blend == 0 && m_actor->isBlendIn())
            blend = 5;
    }
    changeMotion(speed, m_tbl_motion_state[theState].m_motion, flags, NULL, blend);
    if (0 < m_tbl_motion_state[theState].m_se)
        playSe(m_tbl_motion_state[theState].m_se);
    endMotionSeqPhase();
}

// ──────────────────────────────────────────────────────────────────────
// theMotion (and sub) on the actor; a zero link (m_78) blends in 0 frames
// once.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011a4ec
int MagLifeControlChara::changeMotion(float speed, const char *theMotion, u_int flags,
                                      const char *sub, int blend)
{
    ROR_ASSERT(theMotion); /* 504 */
    ROR_ASSERT(m_actor); /* 505 */
    if (m_actor == NULL || theMotion == NULL)
        return -1;
    int no = m_actor->getMotionNoBySuffix(theMotion);
    if (no < 0)
    {
        no = m_actor->getMotionNoByName(theMotion);
        if (no < 0)
            return no;
    }
    int sub_no = -1;
    if (sub)
    {
        sub_no = m_actor->getMotionNoBySuffix(sub);
        if (sub_no < 0)
            sub_no = m_actor->getMotionNoByName(sub);
    }
    if (m_78 != 0)
    {
        m_78 = 0;
        blend = 0;
    }
    return m_actor->changeMotion(speed, no, flags, sub_no, blend);
}

// ──────────────────────────────────────────────────────────────────────
// Sound se at the actor's position (nothing without an actor).
// ROR 0x0011a704
void MagLifeControlChara::playSe(int se)
{
    DogActor *actor = m_actor;
    if (actor)
    {
        sceVu0FVECTOR pos;
        pos[0] = actor->m_trans[0];
        pos[1] = actor->m_trans[1];
        pos[2] = actor->m_trans[2];
        pos[3] = actor->m_trans[3];
        utlPlaySe(se, 0, pos);
    }
}

// ──────────────────────────────────────────────────────────────────────
// A footstep on the motion's event bit 0 this frame: table[] by the floor
// sector's sound slot (the map's default when it has none), the second of
// a pair while m_04 / 13 is odd.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011a738
void MagLifeControlChara::playStepSe(const int *table)
{
    if (!m_actor)
        return;
    float t = m_actor->getMotionFrame();
    if (!(0.0f <= t))
        return;
    if (!(m_actor->getMotionEvent(t, t + 1.0f, -1) & 1))
        return;
    int sndid = -1;
    GamSecta **cells = getSectaCells();
    if (cells && cells[0])
    {
        SctSector *sec = NULL;
        SctLayer *floor = &m_scene->m_8542c.m_list[0];
        if (floor->m_head && floor->m_sectors)
            sec = &floor->m_sectors[cells[0]->m_sector];
        if (sec)
            sndid = sec->m_snd;
    }
    if (sndid < 0)
    {
        sndid = m_scene->m_854a4;
        if (sndid < 0)
            return;
    }
    ROR_ASSERT(sndid < 12); /* 618 */
    int se = table[sndid] + ((m_04 / 13) & 1);
    if (se >= 0)
        playSe(se);
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 2: the actor faces the camera (its z axis turned into
// rotations: x negated, y half a turn) and sits m_distance along the
// direction from its world position to the camera's.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0011a8f4
void MagLifeBillboard::live()
{
    m_04++;
    ROR_ASSERT(m_scene); /* 657 */
    MagScene *scene = m_scene;
    sceVu0FVECTOR z = {0.0f, 0.0f, 0.0f, 1.0f};
    scene->m_camera_entity.getMatrixZ(z);
    sceVu0FVECTOR rot = {0.0f, 0.0f, 0.0f, 1.0f};
    utlVecToRot(z, rot);
    rot[0] = -rot[0];
    if (0x1.921fb6p+1f <= rot[0]) /* 0x40490fdb, pi */
        rot[0] -= 0x1.921fb6p+2f; /* 0x40c90fdb */
    if (rot[0] < -0x1.921fb6p+1f)
        rot[0] += 0x1.921fb6p+2f;
    rot[1] += 0x1.921fb6p+1f;
    if (0x1.921fb6p+1f <= rot[1])
        rot[1] -= 0x1.921fb6p+2f;
    if (rot[1] < -0x1.921fb6p+1f)
        rot[1] += 0x1.921fb6p+2f;
    DogActor *actor = m_actor;
    actor->m_rotate[0] = rot[0];
    actor->m_rotate[1] = rot[1];
    actor->m_rotate[2] = rot[2];
    actor->m_rotate[3] = rot[3];
    actor->normalizeRotate();
    actor->m_dirty = 1;
    sceVu0FVECTOR at = {0.0f, 0.0f, 0.0f, 1.0f};
    m_actor->getWorldPos(at);
    sceVu0FVECTOR eye;
    eye[0] = scene->m_camera_entity.m_trans[0];
    eye[1] = scene->m_camera_entity.m_trans[1];
    eye[2] = scene->m_camera_entity.m_trans[2];
    eye[3] = scene->m_camera_entity.m_trans[3];
    sceVu0FVECTOR dir = {0.0f, 0.0f, 0.0f, 1.0f};
    sceVu0SubVector(dir, eye, at);
    utlNormalize3(dir);
    float dist = m_distance;
    actor = m_actor;
    actor->m_dirty = 1;
    actor->m_trans[0] = dir[0] * dist;
    actor->m_trans[1] = dir[1] * dist;
    actor->m_trans[2] = dir[2] * dist;
    actor->m_trans[3] = 1.0f;
}
