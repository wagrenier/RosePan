// dog/dogactor.cpp -- DogActor: a DogModel with motions.
//
// The motions live in records (m_motion_data, m_max_motion of them), filled
// from motion packs in up to four banks (m_pack_motion; bank 0 is the model
// pack's file 1, banks 1..3 loadPackMotion's heap blocks). A record points
// at its i3m, an optional root move track (x, y, z and the turn per frame;
// getRootMove) and a tag track. The posture block holds the pose records of
// the current and the previous motion and the map from motion nodes to model
// nodes. Written 2026-10-27: the load / release paths, the record lookups,
// the root track and the motion starts; then (same day) the per-frame
// player: runMotionCmd, the posture update, draw / drawModel / drawShadow,
// checkName and the eye; then (a third session) the root track readers
// (getMotionMove, getMotionEvent, the root base), the blend entry starts,
// the node scales and matrices, shareModel and the second constructor.

#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "common/rosepan_stub.h"
#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "bed/bedpfh.h"
#include "game/gamfile.h"
#include "i3d/i3ritem.h"
#include "i3d/i3rmodel.h"
#include "i3d/i3rroot.h"
#include "i3d/i3dmath.h"
#include "lib/unassigned.h"
#include "mag/magsystem.h"
#include "utl/utlmisc.h"

// ──────────────────────────────────────────────────────────────────────
// The posture block and the record table.
// callcheck: dbgAssertFailed, i3mGetNumNode (the assert handler; getMotionNumNode inline)
// ROR 0x00108674

int DogActor::allocPosture(int n)
{
    if (m_max_motion <= 0)
        return -1;
    if (m_posture != NULL)
        return 0;
    m_num_posture_node = getMotionNumNode(n);
    int size = m_num_posture_node * 0x62 + 2;
    m_posture = (u_char *)g_utlMemory.alloc(size, -1);
    ROR_ASSERT(m_posture != NULL); /* 90 */
    if (m_posture == NULL)
        return -1;
    memset(m_posture, 0, size);
    m_posture_prev = m_posture + m_num_posture_node * 0x30;
    m_assign_tbl = (u_short *)(m_posture + m_num_posture_node * 0x60);
    setupAssignTbl(n);
    m_motion.m_no = n;
    m_motion.m_speed = 0x1.11112Ep-6f; /* 0x3c888897 */
    m_motion.m_frame = 0.0f;
    return 0;
}

// ROR 0x00108814

void DogActor::freePosture()
{
    if (m_posture != NULL)
    {
        g_utlMemory.free(m_posture);
        m_posture = NULL;
    }
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00108850

void DogActor::trimMotion()
{
    ROR_ASSERT(m_max_motion > 0); /* 132 */
    int i = m_max_motion;
    if (m_motion_data[i - 1].m_i3m_root == NULL)
    {
        for (i -= 2; i >= 0 && m_motion_data[i].m_i3m_root == NULL; i--)
            ;
        /* Every record free: the ROM passes -1 (setMaxMotion asserts). */
        setMaxMotion(i < 0 ? i : i + 1);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The root track's header words: ints, or floats in some files (a word with
// bits above the small int range is read as a float).
// ROR 0x001088fc

int DogActor::getTrackNumFrame(const DogMotionTrack *trk)
{
    if (trk == NULL)
        return 0;
    u_int v = trk->m_numframe;
    if ((v & ~0x7ffu) != 0)
    {
        float f;
        memcpy(&f, &v, sizeof(f));
        return (int)f;
    }
    return (int)v;
}

// ROR 0x0010893c

int DogActor::getTrackType(const DogMotionTrack *trk)
{
    if (trk == NULL)
        return 0;
    u_int v = trk->m_type;
    if ((v & ~0xffu) != 0)
    {
        float f;
        memcpy(&f, &v, sizeof(f));
        return (int)f;
    }
    return (int)v;
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0010897c

int DogActor::getTrackStride(const DogMotionTrack *trk)
{
    if (trk == NULL)
        return 0;
    switch (getTrackType(trk))
    {
    case 0:
    case 4:
    case 0x14:
        return 4;
    case 1:
        return 6;
    case 2:
    case 0x12:
        return 2;
    case 3:
    case 0x13:
        return 3;
    case 5:
    case 6:
    case 0x15:
        return 1;
    default:
        ROR_ASSERT(0); /* 223 */
        return 0;
    }
}

// Frame n as (x, y, z, turn): type 1 holds six floats (the move in 4, 2, 5
// and the turn in 3), 2 y and the turn, 3 x y z, 4 x y z and the turn, 5 y,
// 6 the turn.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00108a14

void DogActor::getTrackKey(const DogMotionTrack *trk, float *out, int n)
{
    if (trk == NULL)
    {
        out[0] = 0.0f;
        out[3] = 1.0f;
        out[2] = 0.0f;
        out[1] = 0.0f;
        return;
    }
    const float *k = trk->m_data + n * getTrackStride(trk);
    switch (getTrackType(trk))
    {
    case 0:
        ROR_ASSERT(0); /* 246 */
        break;
    case 1:
        out[3] = k[3];
        out[0] = k[4];
        out[1] = k[2];
        out[2] = k[5];
        break;
    case 2:
    case 0x12:
        out[3] = k[1];
        out[1] = k[0];
        out[2] = 0.0f;
        out[0] = 0.0f;
        break;
    case 3:
    case 0x13:
        out[0] = k[0];
        out[1] = k[1];
        out[2] = k[2];
        out[3] = 0.0f;
        break;
    case 4:
    case 0x14:
        out[3] = k[3];
        out[0] = k[0];
        out[1] = k[1];
        out[2] = k[2];
        break;
    case 5:
    case 0x15:
        out[1] = k[0];
        out[3] = 0.0f;
        out[0] = 0.0f;
        out[2] = 0.0f;
        break;
    case 6:
        out[3] = k[0];
        out[2] = 0.0f;
        out[0] = 0.0f;
        out[1] = 0.0f;
        break;
    default:
        ROR_ASSERT(0); /* 271 */
        break;
    }
}

// t in frames (15-bit fixed point between keys). Before 0 or from the last
// key on, t wraps once and the track's whole move (last key - first) is
// added (subtracted before 0); type 4 turns the sampled move by the
// track's whole turn first.
// callcheck: dbgAssertFailed, DogMotionBlend::getNo (the assert handler; getRootTrack inline, getNo called)
// ROR 0x00108c90

int DogActor::getRootMove(float t, float *out)
{
    int no = m_motion.getNo();
    DogMotionTrack *trk = NULL;
    if (no >= 0)
        trk = getRootTrack(no);
    if (trk == NULL)
    {
        out[3] = 0.0f;
        out[2] = 0.0f;
        out[1] = 0.0f;
        out[0] = 0.0f;
        return 0;
    }
    int stride = getTrackStride(trk);
    int numframe = getTrackNumFrame(trk);
    int type = getTrackType(trk);
    int loopflag = 0;
    if (t == 0.0f)
    {
        getTrackKey(trk, out, 0);
        return 1;
    }
    if (t < 0.0f)
    {
        loopflag = -1;
        t += (float)(numframe - 1);
    }
    else if (t >= (float)(numframe - 1))
    {
        t -= (float)(numframe - 1);
        loopflag = 1;
    }
    int fr = utlFtoi15(t);
    int frno = fr >> 15;
    if (frno == numframe - 1)
    {
        int frfr = fr & 0x7fff;
        ROR_ASSERT(frfr == 0); /* 372 */
        getTrackKey(trk, out, frno);
    }
    else
    {
        float b = (float)(fr & 0x7fff) * 0x1p-15f;
        const float *k = trk->m_data + frno * stride;
        float a = 1.0f - b;
        ROR_ASSERT(frno <= numframe-2); /* 382 */
        float v[6];
        for (int i = 0; i < stride; i++)
            v[i] = k[i] * a + k[stride + i] * b;
        const float *first = trk->m_data;
        const float *last = trk->m_data + (numframe - 1) * stride;
        float f = (float)loopflag;
        switch (type)
        {
        case 0:
            ROR_ASSERT(0); /* 492 */
            break;
        case 1:
            out[0] = v[4];
            out[1] = v[2];
            out[2] = v[5];
            out[3] = v[3];
            if (loopflag != 0)
            {
                out[0] = v[4] + (last[4] - first[4]) * f;
                out[1] = v[2] + (last[2] - first[2]) * f;
                out[2] = v[5] + (last[5] - first[5]) * f;
                out[3] = v[3] + (last[3] - first[3]) * f;
            }
            break;
        case 2:
        case 0x12:
            out[0] = 0.0f;
            out[1] = v[0];
            out[2] = 0.0f;
            out[3] = v[1];
            if (loopflag != 0)
            {
                out[1] = v[0] + (last[0] - first[0]) * f;
                out[3] = v[1] + (last[1] - first[1]) * f;
            }
            break;
        case 3:
        case 0x13:
            out[0] = v[0];
            out[1] = v[1];
            out[2] = v[2];
            out[3] = 0.0f;
            if (loopflag != 0)
            {
                out[0] = v[0] + (last[0] - first[0]) * f;
                out[1] = v[1] + (last[1] - first[1]) * f;
                out[2] = v[2] + (last[2] - first[2]) * f;
            }
            break;
        case 4:
        case 0x14:
            out[0] = v[0];
            out[1] = v[1];
            out[2] = v[2];
            out[3] = v[3];
            if (loopflag != 0)
            {
                ROR_ASSERT(loopflag == 1); /* 451 */
                float turn = last[3] - first[3];
                utlVecRotMatY(turn, out);
                out[0] += last[0] - first[0];
                out[1] += last[1] - first[1];
                out[3] += turn;
                out[2] += last[2] - first[2];
            }
            break;
        case 5:
        case 0x15:
            out[0] = 0.0f;
            out[1] = v[0];
            out[2] = 0.0f;
            out[3] = 0.0f;
            if (loopflag != 0)
                out[1] = v[0] + (last[0] - first[0]) * f;
            break;
        case 6:
            out[0] = 0.0f;
            out[1] = 0.0f;
            out[2] = 0.0f;
            out[3] = v[0];
            if (loopflag != 0)
                out[3] = v[0] + (last[0] - first[0]) * f;
            break;
        default:
            ROR_ASSERT(0); /* 495 */
            break;
        }
    }
    if (out[3] < -DOG_PI)
        out[3] += DOG_PI2;
    if (out[3] >= DOG_PI)
        out[3] -= DOG_PI2;
    /* (theResult.m_w >= -DOG_PI) && (theResult.m_w < DOG_PI) */
    ROR_ASSERT((out[3] >= -DOG_PI) && (out[3] < DOG_PI)); /* 514 */
    return loopflag == 0 ? 1 : 2;
}

// ──────────────────────────────────────────────────────────────────────
// The actor's words.
// callcheck: DogModel::clear, DogMotionBlend::reset (called; clearNodeScale is called)
// ROR 0x00109714

void DogActor::clear()
{
    DogModel::clear();
    m_mode = -1;
    m_f4 = -1;
    m_id = -1;
    m_model_copy = NULL;
    m_sub_motion = 0;
    m_max_motion = 0;
    m_motion_data = NULL;
    m_flag_motion_data = FLAG_MOTION_NONE;
    m_posture = NULL;
    m_assign_tbl = NULL;
    m_interpolation = 0;
    m_motion.m_motion = NULL;
    m_motion.m_max_motion = 0;
    m_motion.reset();
    m_motion_prev.m_motion = NULL;
    m_motion_prev.m_max_motion = 0;
    m_motion_prev.reset();
    m_control_node = -1;
    m_control_node_flg = 0;
    clearNodeScale();
    m_motion_flags2 = -1;
    m_trans_ofs_y_save = 0.0f;
    m_rot_ofs_y = 0.0f;
    for (int i = 0; i < DOG_MOTION_LEVEL_MAX; i++)
        m_pack_motion[i] = NULL;
    m_motion_cmd_idx = -1;
    for (int i = 0; i < 8; i++)
        m_motion_cmd[i] = -1;
    m_rot_y_aar = 0.0f;
    m_4c5 = 1;
    m_4b0[0] = 1.0f;
    m_4a0[0] = m_rot_y_aar;
    m_4c0 = 0x1.999998p-4f; /* 0x3dcccccc */
    m_454 = 0;
    m_4b0[3] = 1.0f;
    m_4b0[2] = 1.0f;
    m_4b0[1] = 1.0f;
    m_4a0[3] = m_rot_y_aar;
    m_4a0[2] = m_rot_y_aar;
    m_4a0[1] = m_rot_y_aar;
    m_4c4 = 1;
}

// callcheck: dbgAssertFailed, DogMotionBlend::reset (the assert handler; reset called)
// ROR 0x00109860

void DogActor::setMaxMotion(int theN)
{
    ROR_ASSERT(theN >= 0); /* 763 */
    if (theN > m_max_motion)
    {
        DogMotionData *p = (DogMotionData *)g_utlMemory.alloc(theN * (int)sizeof(DogMotionData), -1);
        ROR_ASSERT(p); /* 772 */
        if (m_motion_data != NULL)
        {
            ROR_ASSERT(m_max_motion != 0); /* 777 */
            memcpy(p, m_motion_data, m_max_motion * sizeof(DogMotionData));
            g_utlMemory.free(m_motion_data);
            m_motion_data = NULL;
        }
        /* PC: from 0 after trimMotion's -1 (the ROM writes the word before
         * the block, the heap header). */
        for (int i = m_max_motion < 0 ? 0 : m_max_motion; i < theN; i++)
        {
            p[i].m_no = -1;
            p[i].m_i3m_root = NULL;
            p[i].m_root = NULL;
            p[i].m_level = -1;
            p[i].m_name = NULL;
        }
        m_motion_data = p;
    }
    else
    {
        if (theN >= m_max_motion)
            return;
        ROR_ASSERT(m_motion_data); /* 803 */
        /* PC: from 0 for trimMotion's -1 (see above). */
        for (int i = theN < 0 ? 0 : theN; i < m_max_motion; i++)
        {
            DogMotionData &rec = m_motion_data[i];
            if (m_flag_motion_data == FLAG_MOTION_EACH && rec.m_i3m_root != NULL)
                g_utlMemory.free(rec.m_i3m_root);
            rec.m_i3m_root = NULL;
            rec.m_root = NULL;
            rec.m_level = -1;
            rec.m_no = -1;
            rec.m_name = NULL;
        }
        if (theN < 1)
        {
            m_flag_motion_data = FLAG_MOTION_NONE;
            g_utlMemory.free(m_motion_data);
            m_motion_data = NULL;
        }
        else
        {
            DogMotionData *p = (DogMotionData *)g_utlMemory.alloc(theN * (int)sizeof(DogMotionData), -1);
            ROR_ASSERT(p); /* 824 */
            memcpy(p, m_motion_data, theN * sizeof(DogMotionData));
            g_utlMemory.free(m_motion_data);
            m_motion_data = p;
        }
    }
    m_max_motion = theN;
    m_motion.m_motion = m_motion_data;
    m_motion.m_max_motion = m_max_motion;
    m_motion.reset();
    m_motion_prev.m_motion = m_motion_data;
    m_motion_prev.m_max_motion = m_max_motion;
    m_motion_prev.reset();
    ROR_ASSERT(m_max_motion > 0); /* 850 */
}

// ROR 0x00109b48

void DogActor::freeMotionData()
{
    if (m_motion_data != NULL)
    {
        g_utlMemory.free(m_motion_data);
        m_motion_data = NULL;
    }
    m_max_motion = 0;
}

// Bank 0 (the model pack's own motions) is never released here: its
// records go with the model. Bank theId's records are freed (each its own
// block) or dropped with the bank's pack; motion states that played them
// lose them.
// callcheck: dbgAssertFailed, DogMotionBlend::getNo (the assert handler; getBlendNo inline, getNo called)
// ROR 0x00109b88

void DogActor::releaseMotion(int theId)
{
    if (theId <= 0)
        return;
    ROR_ASSERT(theId >= 0 && theId < (4)); /* 901 */
    if (m_flag_motion_data == FLAG_MOTION_EACH)
    {
        for (int i = m_max_motion - 1; i >= 0; i--)
        {
            DogMotionData *pm = &m_motion_data[i];
            if (pm->m_level == theId)
            {
                ROR_ASSERT(pm->m_i3m_root); /* 911 */
                g_utlMemory.free(pm->m_i3m_root);
                pm->m_i3m_root = NULL;
                pm->m_name = NULL;
                pm->m_level = -1;
                pm->m_no = -1;
            }
        }
    }
    else if (m_flag_motion_data == FLAG_MOTION_PACK && m_pack_motion[theId] != NULL)
    {
        for (int i = m_max_motion - 1; i >= 0; i--)
        {
            DogMotionData *pm = &m_motion_data[i];
            if (pm->m_level == theId)
            {
                ROR_ASSERT(pm->m_i3m_root); /* 932 */
                pm->m_name = NULL;
                pm->m_i3m_root = NULL;
                pm->m_level = -1;
                pm->m_no = -1;
            }
        }
        g_utlMemory.free(m_pack_motion[theId]);
        m_pack_motion[theId] = NULL;
        /* PC: the ROM reads record -1 without a motion (the heap header,
         * never NULL; m_no stays -1 either way). */
        int no = m_motion.getNo();
        if (no >= 0 && m_motion_data[no].m_i3m_root == NULL)
            m_motion.m_no = -1;
        for (int n = 0; n < DOG_MOTION_BLEND_MAX; n++)
        {
            int bno = m_motion.getBlendNo(n);
            if (bno > 0 && m_motion_data[bno].m_i3m_root == NULL)
                m_motion.m_blend[n].m_no = -1;
        }
    }
    if (m_max_motion > 0 && m_motion_data[m_max_motion - 1].m_i3m_root == NULL)
        trimMotion();
}

// ROR 0x00109dec

void DogActor::releaseSubMotion()
{
    if (m_sub_motion > 0)
    {
        releaseMotion(m_sub_motion);
        m_sub_motion = -1;
    }
}

// callcheck: DogModel::release (called)
// ROR 0x00109e28

void DogActor::releaseAll()
{
    for (int i = 0; i < DOG_MOTION_LEVEL_MAX; i++)
        releaseMotion(i);
    if (m_motion_data != NULL)
    {
        g_utlMemory.free(m_motion_data);
        m_motion_data = NULL;
    }
    if (m_posture != NULL)
    {
        g_utlMemory.free(m_posture);
        m_posture = NULL;
    }
    release(1);
    if (m_model_copy != NULL)
    {
        g_utlMemory.free(m_model_copy);
        m_model_copy = NULL;
    }
}

// tex is passed on in the ROM and never read.
// callcheck: dbgAssertFailed, DogModel::freeMatrix (the assert handler; decRefCount inline)
// ROR 0x00109ed0

void DogActor::releaseModel(DogTexInfo *tex)
{
    (void)tex;
    if (m_original == NULL)
    {
        if (decRefCount() > 0)
        {
            /* Shared and still referenced: only hidden. */
            m_hide = 1;
            return;
        }
        releaseAll();
    }
    else
    {
        for (int i = 1; i < DOG_MOTION_LEVEL_MAX; i++)
            ROR_ASSERT(m_pack_motion[i] == NULL); /* 1101 */
        static_cast<DogActor *>(m_original)->releaseShared();
        freeMotionData();
        freePosture();
        freeMatrix();
    }
    clear();
}

// callcheck: dbgAssertFailed (the assert handler; decRefCount inline)
// ROR 0x00109fb4

void DogActor::releaseShared()
{
    ROR_ASSERT(m_original == NULL); /* 1151 */
    if (decRefCount() < 1)
    {
        releaseAll();
        clear();
    }
}

// ──────────────────────────────────────────────────────────────────────
// Entity moves.
// ROR 0x0010a038

void DogActor::addTrans(const float *d, float trans_ofs_y, float rot_y_aar)
{
    m_trans[0] += d[0];
    m_trans[1] += d[1];
    m_trans[3] = 1.0f;
    m_trans[2] += d[2];
    m_rot_y_aar = rot_y_aar;
    m_trans_ofs[1] = trans_ofs_y;
    m_trans_ofs[3] = 1.0f;
    m_dirty = 1;
}

// The root's turn at twice the motion frame (the track's frames are 120ths?
// unverified: getMotionFrame counts 60ths).
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0010a584

float DogActor::getRootRotY()
{
    if (isBlendIn())
        return -m_80[1];
    float t = getMotionFrame();
    float turn = 0.0f;
    if (t + t >= 0.0f)
    {
        float v[4];
        v[0] = 0.0f;
        v[3] = 1.0f;
        v[2] = 0.0f;
        v[1] = 0.0f;
        getRootMove(t + t, v);
        turn = v[3];
        /* (v.m_w >= -DOG_PI) && (v.m_w < DOG_PI) */
        ROR_ASSERT((turn >= -DOG_PI) && (turn < DOG_PI)); /* 1421 */
    }
    return turn;
}

// callcheck: dbgAssertFailed, DogMotionBlend::getNo (the assert handler; getRootTrack inline, getNo called)
// ROR 0x0010a650

float DogActor::getRotY()
{
    if (isBlendIn())
        return m_rotate[1];
    int no = m_motion.getNo();
    DogMotionTrack *trk = NULL;
    if (no >= 0)
        trk = getRootTrack(no);
    float r;
    if (trk == NULL)
    {
        r = m_rotate[1];
    }
    else
    {
        /* m_80 is the ROM's m_rot_offset, m_base_rotate its m_base_rot. */
        r = m_base_rotate[1] + m_80[1];
        ROR_ASSERT((m_base_rotate[1] >= -DOG_PI) && (m_base_rotate[1] < DOG_PI)); /* 1446 */
        ROR_ASSERT((m_80[1] >= -DOG_PI) && (m_80[1] < DOG_PI)); /* 1447 */
        if (r < -DOG_PI)
            r += DOG_PI2;
        else if (r >= DOG_PI)
            r -= DOG_PI2;
        r += getRootRotY();
    }
    if (r < -DOG_PI)
        r += DOG_PI2;
    else if (r >= DOG_PI)
        r -= DOG_PI2;
    return r;
}

// ──────────────────────────────────────────────────────────────────────
// Record lookups.
// ROR 0x0010a874

int DogActor::findMotionById(int theID)
{
    for (int i = 0; i < m_max_motion; i++)
    {
        if (m_motion_data[i].m_no == theID)
            return i;
    }
    return -1;
}

// A record name is <prefix>_<name>; the name matches theName in any case
// and may go on with '+', '*' or "__". A record name without '_' ends the
// search.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0010a8b8

int DogActor::findMotionBySuffix(const char *theName)
{
    ROR_ASSERT(theName); /* 1539 */
    for (int i = 0; i < m_max_motion; i++)
    {
        const char *s = m_motion_data[i].m_name;
        if (s == NULL)
            continue;
        for (; *s != '_'; s++)
        {
            if (*s == '\0')
                return -1;
        }
        s++;
        const char *t = theName;
        while (*t != '\0' && toupper((u_char)*t) == toupper((u_char)*s))
        {
            t++;
            s++;
        }
        if (*t == '\0')
        {
            char c = *s;
            if (c == '\0' || c == '+' || c == '*' || (c == '_' && s[1] == '_'))
                return i;
        }
    }
    return -1;
}

// ROR 0x0010aa5c

int DogActor::getMotionNoBySuffix(const char *theName)
{
    int n = findMotionBySuffix(theName);
    if (n < 0)
        return -1;
    return m_motion_data[n].m_no;
}

// callcheck: dbgAssertFailed, strcmp (the assert handler; strcmp called)
// ROR 0x0010aa9c

int DogActor::findMotionByName(const char *theName)
{
    ROR_ASSERT(theName); /* 1611 */
    for (int i = 0; i < m_max_motion; i++)
    {
        const char *s = m_motion_data[i].m_name;
        if (s != NULL && strcmp(s, theName) == 0)
            return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// Motion starts.
// ROR 0x0010ae58

void DogActor::startMotionMode()
{
    u_int mode = (u_int)m_mode & 8;
    if (m_interpolation < 1)
    {
        mode |= 1;
    }
    else
    {
        mode |= 2;
        if ((m_motion_flags & 0x40) != 0)
            mode |= 1;
    }
    m_f4 = 0;
    m_mode = (int)(mode | 4);
}

// Starts the record with id theID (the record index from here on).
// callcheck: dbgAssertFailed (the assert handler; DogMotionBlend::getData inline)
// ROR 0x0010af18

int DogActor::setMotion(float speed, int theID, u_int flags, int flags2, int p1d8)
{
    ROR_ASSERT(m_max_motion > 0); /* 1793 */
    u_int mode = (u_int)m_mode;
    int n = findMotionById(theID);
    if (n < 0 || n >= m_max_motion)
        return -1;
    mode &= 4;
    if (m_motion_data[n].m_i3m_root == NULL)
        return -1;
    if (mode == 0)
        savePosture();
    m_motion.m_no = n;
    m_motion.m_frame = 0.0f;
    m_motion.m_speed = speed;
    /* The ROM passes the record index where getMotionLength wants an id. */
    if ((flags & 0x80) != 0 || speed < 0.0f)
        m_motion.m_frame = getMotionLength(n);
    if (p1d8 < 0)
    {
        int *param = m_motion.getData(n).m_param; /* the header's line 123 */
        if (param != NULL)
            setParam1d8(*param);
    }
    else
    {
        setParam1d8(p1d8);
    }
    if (mode == 0 && m_interpolation <= 0)
    {
        /* No blend-in: the root's height at frame 0 becomes the offset. */
        float v[4];
        v[2] = 0.0f;
        v[1] = 0.0f;
        v[0] = 0.0f;
        v[3] = 1.0f;
        getRootMove(0.0f, v);
        m_trans_ofs[3] = 1.0f;
        m_trans_ofs[1] = v[1];
        m_dirty = 1;
    }
    if ((char)m_motion.getData(n).m_10 != 0 && m_interpolation > 0) /* the header's line 122 */
        flags |= 0x40;
    m_motion_flags = (int)flags;
    m_motion_flags2 = flags2;
    startMotionMode();
    return n;
}

// callcheck: dbgAssertFailed, DogMotionBlend::getNo, DogEntity::normalizeRotate (the assert handler; getNo, normalizeRotate called)
// ROR 0x0010b1b0

int DogActor::changeMotion(float speed, int theNo, u_int flags, int flags2, int arg)
{
    ROR_ASSERT(theNo >= 0); /* 1891 */
    if ((flags & 4) != 0)
    {
        /* The same motion (or flags2's): only the speed. */
        if (findMotionById(theNo) == m_motion.getNo())
        {
            m_motion.m_speed = speed;
            return -1;
        }
        if (flags2 >= 0 && findMotionById(flags2) == m_motion.getNo())
        {
            m_motion.m_speed = speed;
            return -1;
        }
    }
    if (((u_int)m_mode & 4) == 0)
    {
        /* The root's turn so far goes into the entity and the base. */
        saveRootOffset();
        float r = getRotY();
        m_rotate[1] = r;
        normalizeRotate();
        m_dirty = 1;
        setBaseRotateY(r);
        m_80[0] = 0.0f;
        m_80[3] = 1.0f;
        m_80[2] = 0.0f;
        m_80[1] = 0.0f;
        m_dirty = 1;
    }
    setMotion(speed, theNo, flags, -1, arg);
    m_motion_flags2 = flags2;
    return theNo;
}

// ROR 0x0010bf00

void DogActor::setParam1d8(int v)
{
    m_interpolation = v;
}

// callcheck: dbgAssertFailed, i3mGetNumNode, i3mGetNodeName, i3rGetNodeIndexByName (the assert handler; getMotionNumNode, getMotionNodeName and DogModel::getNodeIndexByName inline)
// ROR 0x0010bf08

int DogActor::setupAssignTbl(int n)
{
    int num = getMotionNumNode(n);
    ROR_ASSERT(m_assign_tbl); /* 2261 */
    u_short *p = m_assign_tbl;
    *p = (u_short)num;
    int missing = 0;
    for (int i = 0; i < num; i++)
    {
        p++;
        int node = getNodeIndexByName(getMotionNodeName(n, i));
        if (node < 0)
        {
            missing++;
            *p = 0xffff;
        }
        else
        {
            *p = (u_short)node;
        }
    }
    return missing;
}

// ──────────────────────────────────────────────────────────────────────
// Loading.
// callcheck: dbgAssertFailed, BedPfh, FileReadReq, FileReader, ~FileReader (the assert handler; the pfh constructors; inline in game/gamfile.h: the two constructors and the destructor)
// ROR 0x0010c17c

u_int DogActor::loadPackMotion(const char *theFileName, int theIndex, int theLevel)
{
    if (m_original != NULL)
    {
        /* Shared: the original loads, the records are copied. */
        DogActor *orig = static_cast<DogActor *>(m_original);
        u_int r = orig->loadPackMotion(theFileName, theIndex, theLevel);
        int n = orig->m_max_motion;
        ROR_ASSERT(n>=0); /* 2310 */
        setMaxMotion(n);
        if (orig->m_motion_data != NULL && n > 0)
            memcpy(m_motion_data, orig->m_motion_data, n * sizeof(DogMotionData));
        return r;
    }
    ROR_ASSERT(theFileName); /* 2325 */
    if (theLevel < 0)
    {
        theLevel = 0;
        if (m_pack_motion[0] != NULL)
            for (theLevel = 1; theLevel <= 3 && m_pack_motion[theLevel] != NULL; theLevel++)
                ;
    }
    else if (m_pack_motion[theLevel] != NULL)
    {
        releaseMotion(theLevel);
        ROR_ASSERT(m_pack_motion[theLevel] == NULL); /* 2335 */
    }
    if ((u_int)theLevel > 3)
        return (u_int)-1;
    int lev = theLevel;
    ROR_ASSERT(m_pack_motion[lev] == NULL); /* 2358 */

    FileReader reader;
    FileReadReq req;
    /* The pack's header: its first 2048 bytes, into a 64-aligned stack buffer. */
    u_char buf[2048 + 64];
    void *head = (void *)((((uintptr_t)buf) >> 6 << 6) + 64);
    req.m_08 = 1;
    req.m_name = theFileName;
    req.m_buf = head;
    req.m_size = 0x800;
    reader.read(&req, 0);
    BedPfh pfh1(head);
    ROR_ASSERT(pfh1.getFileOffset(0) == (2048)); /* 2377 */
    ROR_ASSERT(theIndex >= 0 && theIndex < pfh1.getNumFile()); /* 2379 */
    int size = pfh1.getFileSize(theIndex);
    if (size < 1)
    {
        ROR_ASSERT(0); /* 2385 */
        return (u_int)-1;
    }
    size = (size + 0x7ff) / 0x800 * 0x800;
    void *p = g_utlMemory.allocDebug(size, "dog/dogactor.cpp", 2390, 0, 1);
    ROR_ASSERT(p != NULL); /* 2391 */
    ROR_ASSERT((((intptr_t)p)%64)==0); /* 2392: ((int)p) on the EE */
    /* PC: the ROM hangs in the assert; no block, no read. */
    if (p == NULL)
        return (u_int)-1;
    req.m_offset = pfh1.getFileOffset(theIndex);
    req.m_close = 1;
    req.m_whence = 0;
    req.m_buf = p;
    req.m_size = size;
    reader.read(&req, 0);
    BedPfh pfh(p);
    if (loadMotion(pfh, lev) >= 0)
        return (u_int)lev;
    g_utlMemory.free(p);
    return (u_int)-1;
}

// A pack's files are either i3m files (a root track and a tag track may
// follow the i3m inside the file) or packs of the motion's parts (file 0
// the i3m, 1 the parameters, 2 the root track, 3 the tag track).
// callcheck: dbgAssertFailed, BedPfh, i3mRelocate (the assert handler; the part pack constructor; relocateMotion inline)
// ROR 0x0010c568

int DogActor::loadMotion(const BedPfh &pfh, int theIndex)
{
    if (m_original != NULL)
        return -100;
    ROR_ASSERT(theIndex < (4)); /* 2433 */
    int index = theIndex;
    if ((u_int)theIndex > 3)
    {
        /* Bank 0 while it is free, else the first free one from 1. */
        index = 0;
        if (m_pack_motion[0] != NULL)
            for (index = 1; index <= 3 && m_pack_motion[index] != NULL; index++)
                ;
    }
    if ((u_int)index > 3)
        return -1;
    ROR_ASSERT(m_pack_motion[index] == NULL); /* 2464 */
    int num = pfh.getNumFile();
    if (num == 0)
        return 0;
    ROR_ASSERT(num > 0); /* 2472 */
    int base = m_max_motion;
    setMaxMotion(base + num);
    ROR_ASSERT(m_flag_motion_data != FLAG_MOTION_EACH); /* 2479 */
    m_flag_motion_data = FLAG_MOTION_PACK;
    /* The ROM clears bank 0's pack pointer here whatever the bank (bank 0
     * is never released by pointer: it lies inside the model pack). */
    m_pack_motion[0] = NULL;
    m_pack_motion[index] = pfh.m_top;
    if ((pfh.getFlags() & 0x10) == 0)
    {
        for (int i = 0; i < num; i++)
        {
            m_motion_data[base + i].m_no = base + i;
            m_motion_data[base + i].m_level = index;
        }
    }
    else
    {
        /* The files' user data carry the ids. */
        for (int i = 0; i < num; i++)
        {
            int id = pfh.getUserData(i) & 0xfff;
            m_motion_data[base + i].m_no = id;
            m_motion_data[base + i].m_10 = id;
            m_motion_data[base + i].m_level = index;
        }
    }
    const char *name = pfh.m_name;
    for (int i = 0; i < num; i++)
    {
        int m = base + i;
        DogMotionData &rec = m_motion_data[m];
        rec.m_root = NULL;
        rec.m_tag_data = NULL;
        rec.m_param = NULL;
        rec.m_10 = 0;
        rec.m_name = name;
        if (name != NULL)
            name += strlen(name) + 1;
        u_char *file = (u_char *)pfh.getFile(i);
        if (*(u_int *)file == 0x4b505452) /* "RTPK" */
        {
            BedPfh part(file);
            rec.m_i3m_root = (I3mRoot *)part.getFile(0);
            int *param = (int *)part.getFile(1);
            rec.m_10 = param[0];
            rec.m_param = param + 1;
            if (part.getNumFile() > 2)
            {
                if (part.getFileSize(2) > 0)
                    rec.m_root = (DogMotionTrack *)part.getFile(2);
                if (part.getNumFile() > 3 && part.getFileSize(3) > 0)
                    rec.m_tag_data = (DogMotionTrack *)part.getFile(3);
            }
        }
        else
        {
            rec.m_i3m_root = (I3mRoot *)file;
            int off = rec.m_i3m_root->m_size;
            if (off != pfh.getFileSize(i))
            {
                rec.m_root = (DogMotionTrack *)(file + off);
                int size = getTrackNumFrame(rec.m_root) * getTrackStride(rec.m_root) + 0x10;
                if (pfh.getFileSize(i) < size + off)
                {
                    rec.m_tag_data = (DogMotionTrack *)((u_char *)rec.m_root + size);
                    ROR_ASSERT(m_motion_data[m].m_tag_data->m_numframe & 0x80000000); /* 2594 */
                }
            }
        }
        if (relocateMotion(m) != 0)
            return -1;
        if (allocPosture(m) < 0)
            return -3;
    }
    return index;
}

// callcheck: dbgAssertFailed, BedPfh::clear, BedPfh::BedPfh, BedPfh::getFileName, BedMemory::allocDebug, memcpy, DogEntity::reset, DogModel::allocMatrix (the assert handler; BedPfh's inlines; slot 1 reset and slot 14 loadPack called directly)
// ROR 0x0010cd5c

int DogActor::load(const BedPfh &pfh, DogTexInfo *tex, int mode, int sepia)
{
    reset(); /* EE slot 1 */
    const void *top = pfh.m_top;
    if (mode == 2)
    {
        void *copy = g_utlMemory.allocDebug(pfh.getTotalSize(), "dog/dogactor.cpp", 2662, 0, 1);
        memcpy(copy, pfh.m_top, pfh.getTotalSize());
        top = copy;
    }
    /* The ROM clears the view, then constructs it on top. */
    BedPfh pack;
    pack.clear();
    pack = BedPfh(top);
    if (mode > 0)
        m_model_copy = (void *)top;
    BedPfh model(pack.getFile(0));
    (void)pack.getFileName(0);
    int rrr = loadPack(tex, model, sepia, 1); /* EE slot 14 */
    ROR_ASSERT(rrr>=0); /* 2692 */
    allocMatrix();
    if (pack.getNumFile() > 1)
    {
        BedPfh motion(pack.getFile(1));
        loadMotion(motion, -1);
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Motion time.
// callcheck: dbgAssertFailed, i3mGetLength (the assert handler; i3mGetLength called)
// ROR 0x0010d0e8

float DogActor::getMotionLength(int theID)
{
    ROR_ASSERT(theID >= 0); /* 2847 */
    ROR_ASSERT(m_motion_data); /* 2848 */
    int n = findMotionById(theID);
    ROR_ASSERT(n>=0 && n < m_max_motion); /* 2852 */
    /* PC: the ROM hangs in the asserts; no record, no length. */
    if (m_motion_data == NULL || n < 0)
        return 0.0f;
    ROR_ASSERT(m_motion_data[n].m_i3m_root); /* 2853 */
    return i3mGetLength(m_motion_data[n].m_i3m_root);
}

// ROR 0x0010d9ec

float DogActor::getMotionTime()
{
    if (m_mode == 2)
        return (float)~m_f4;
    return m_motion.m_frame;
}

// ROR 0x0010da4c

float DogActor::getMotionFrame()
{
    if (m_mode != 2)
        return m_motion.m_frame * 60.0f;
    return (float)~m_f4;
}

// ──────────────────────────────────────────────────────────────────────
// The motion frame in 120ths (written ahead with task/tsk_chr.cpp, whose
// TskMtnMove compares it with the motion's length * 120).
// ROR 0x0010da80

float DogActor::getMotionFrame120()
{
    float f = m_motion.m_frame * 60.0f;
    return f + f;
}

// ROR 0x0010dad8

float DogActor::getMotionStep()
{
    if (m_45c < 1 || m_460 == m_45c - 1)
        return m_motion.m_speed / 0x1.11112Ep-6f; /* 0x3c888897 */
    return 0.0f;
}

// ROR 0x0010db38

int DogActor::isBlendIn()
{
    return (m_mode >> 1) & 1;
}

// ROR 0x0010e7b4

void DogActor::saveRotOfsY()
{
    m_rot_ofs_y = getRootRotY();
}

// ROR 0x0010e930

void DogActor::saveRootOffset()
{
    if (!isBlendIn())
    {
        m_trans_ofs_y_save = m_trans_ofs[1];
        saveRotOfsY();
        return;
    }
    float v[4];
    v[2] = 0.0f;
    v[3] = 1.0f;
    v[1] = 0.0f;
    v[0] = 0.0f;
    getRootMove(0.0f, v);
    m_trans_ofs_y_save = v[1];
    saveRotOfsY();
}

// ROR 0x0010ec50

void DogActor::clearNodeScale()
{
    for (int i = 0; i < DOG_SCALE_NODE_MAX; i++)
    {
        m_scale_node_no[i] = -1;
        m_scale_node_val[i] = 1.0f;
    }
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0010ed2c

void DogActor::setBaseRotateY(float r)
{
    if (r < -DOG_PI)
        r += DOG_PI2;
    if (r >= DOG_PI)
        r -= DOG_PI2;
    m_base_rotate[1] = r;
    /* (m_base_rot.m_y >= -DOG_PI) && (m_base_rot.m_y < DOG_PI) */
    ROR_ASSERT((m_base_rotate[1] >= -DOG_PI) && (m_base_rotate[1] < DOG_PI)); /* 4190 */
}

// callcheck: memcpy (copied with memcpy)
// ROR 0x0010edbc

void DogActor::copyPosture()
{
    memcpy(m_posture_prev, m_posture, m_num_posture_node * 0x30);
}

// ROR 0x0010edec

void DogActor::savePosture()
{
    copyPosture();
    m_motion_prev = m_motion;
}

// ──────────────────────────────────────────────────────────────────────
// The constructor: DogModel's (inline), both motion states, three vectors.
// callcheck: DogMotionBlend (the members' constructors, run before the body)
// ROR 0x0010eebc

DogActor::DogActor()
{
    _ee_vptr = DOG_VT_ACTOR;
    m_base_trans[3] = 1.0f;
    m_base_trans[2] = 0.0f;
    m_base_trans[1] = 0.0f;
    m_base_trans[0] = 0.0f;
    m_base_rotate[3] = 1.0f;
    m_base_rotate[2] = 0.0f;
    m_base_rotate[1] = 0.0f;
    m_base_rotate[0] = 0.0f;
    m_3e0[3] = 1.0f;
    m_3e0[0] = 0.0f;
    m_3e0[2] = 0.0f;
    m_3e0[1] = 0.0f;
    clear();
}

// The two loads clear the 0x30 bytes from 0x470: the eye words.
static void dogActorClearEye(DogActor *a)
{
    for (int i = 0; i < 4; i++)
        a->m_eye_uv[i] = 0.0f;
    a->m_eye_mat = NULL;
    a->m_eye_node = NULL;
    a->m_eye_flags = 0;
    a->m_eye_state = 0;
    a->m_eye_count = 0;
    a->m_eye_period = 0;
    memset(a->_498, 0, sizeof(a->_498));
}

// callcheck: memset (dogActorClearEye clears the eye words field by field)
// ROR 0x0010f1f0

int DogActor::loadData(DogTexInfo *tex, void *data, void *i3c)
{
    int r = DogModel::loadData(tex, data, i3c);
    dogActorClearEye(this);
    if (r >= 0)
        setupEye();
    return r;
}

// callcheck: memset (dogActorClearEye clears the eye words field by field)
// ROR 0x0010f244

int DogActor::loadPack(DogTexInfo *tex, const BedPfh &pfh, int sepia, int sepia_disp)
{
    int r = DogModel::loadPack(tex, pfh, sepia, sepia_disp);
    dogActorClearEye(this);
    if (r >= 0)
        setupEye();
    return r;
}

// ROR 0x0010f298

void DogActor::resetShadow()
{
    m_shadow_fade = -1;
    m_shadow_light_prev = m_shadow_light;
    m_shadow_light = -1;
    m_mode |= 8;
    m_motion.m_speed = 0x1.11112Ep-6f; /* 0x3c888897 */
    m_motion.m_frame = 0.0f;
    m_motion.m_no = 0;
}

// The first model node whose name holds "eye" and its first material.
// callcheck: i3rGetNumChild52, i3rGetChildName52, i3rGetChild52, i3rGetChild25, strstr, DogActor::eyeHold (called)
// ROR 0x0010f3a4

void DogActor::setupEye()
{
    I3rItem *root = &m_i3r_root->m_item;
    int n = i3rGetNumChild52(root);
    for (int i = 0; i < n; i++)
    {
        if (strstr(i3rGetChildName52(root, i), "eye") != NULL)
        {
            m_eye_node = i3rGetChild52(root, i);
            if (m_eye_node != NULL)
                m_eye_mat = i3rGetChild25(m_eye_node, 0);
            break;
        }
    }
    m_eye_uv[0] = 0.0f;
    m_eye_uv[2] = 1.0f;
    m_eye_uv[1] = 0.0f;
    m_eye_uv[3] = 1.0f;
    m_eye_flags = 0;
    m_eye_count = 0;
    m_eye_period = 0;
    m_eye_state = 0;
    eyeHold();
}

// ──────────────────────────────────────────────────────────────────────
// The actor is in a mode (native 004 case 17 waits while it is).
// ROR 0x0010a834

bool DogActor::isModeSet() const
{
    return m_mode != 0;
}

// ──────────────────────────────────────────────────────────────────────
// The eye flag ops of native 281 op 21: each does nothing unless the
// model has an eye node with a material (setEyeTex found them).
// ROR 0x0010f70c

void DogActor::eyeHold()
{
    if (m_eye_node != NULL && m_eye_mat != NULL)
        m_eye_flags = (m_eye_flags & ~2u) | 1;
}

// ROR 0x0010f73c

void DogActor::eyeStart()
{
    if (m_eye_node == NULL || m_eye_mat == NULL)
        return;
    if ((m_eye_flags & 1) == 0)
    {
        m_eye_flags |= 3;
        if (m_eye_state != 0)
            return;
    }
    else if (m_eye_count >= m_eye_period)
    {
        return;
    }
    m_eye_count = 0;
    m_eye_period = 1;
}

// ROR 0x0010f7a0

void DogActor::eyeStep()
{
    if (m_eye_node == NULL || m_eye_mat == NULL)
        return;
    u_int flags = m_eye_flags;
    if ((flags & 1) != 0)
    {
        if (m_eye_count < m_eye_period)
            m_eye_flags = flags & ~3u;
        else
            m_eye_flags = flags | 2;
        return;
    }
    if (m_eye_count >= m_eye_period)
        m_eye_flags = flags | 3;
}

// ROR 0x0010f808

void DogActor::eyeReset()
{
    if (m_eye_node == NULL || m_eye_mat == NULL)
        return;
    setEyeTex(2);
    m_eye_period = 0;
    m_eye_count = 0;
    m_eye_flags &= ~3u;
}

// ──────────────────────────────────────────────────────────────────────
// The per-frame player: the motion command list, the motion step, the
// blend-in, the posture and the draw.

// One more blend-in frame (m_f4 counts the frames since a motion start).
// ROR 0x00108660

void DogActor::countBlendFrame()
{
    m_f4++;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler inside getRootTrack)
// ROR 0x00109284

void DogActor::getRootEndKey(float *out)
{
    DogMotionTrack *trk = NULL;
    int no = m_motion.getNo();
    if (no >= 0)
        trk = getRootTrack(no);
    if (trk == NULL)
    {
        out[3] = 0.0f;
        out[2] = 0.0f;
        out[1] = 0.0f;
        out[0] = 0.0f;
        return;
    }
    getTrackKey(trk, out, getTrackNumFrame(trk) - 1);
}

// ROR 0x0010be80

int DogActor::stepMotionLoop()
{
    if (m_max_motion > 0 && m_motion.getNo() >= 0)
        return m_motion.stepLoop();
    return 0;
}

// ROR 0x0010bed0

int DogActor::stepMotion()
{
    if (m_max_motion > 0)
        return m_motion.step();
    return 0;
}

// The heading turned by r: m_rotate y (wrapped once), the base rotation
// too, the root turn consumed. Inline in the ROM (draw and runMotionCmd
// repeat it).
static void dogActorTurn(DogActor *a, float r)
{
    float y = a->m_rotate[1] + r;
    a->m_rotate[1] = y;
    if (y < -DOG_PI)
        a->m_rotate[1] = y + DOG_PI2;
    else if (y >= DOG_PI)
        a->m_rotate[1] = y - DOG_PI2;
    a->m_dirty = 1;
    a->addBaseRotY(r);
    a->m_rot_y_aar = 0.0f;
}

// A looping step's wrap: the root track's whole turn once more (forward)
// or back (backward).
static void dogActorTurnWrap(DogActor *a, int result)
{
    float key[4];
    key[2] = 0.0f;
    key[3] = 1.0f;
    key[1] = 0.0f;
    key[0] = 0.0f;
    a->getRootEndKey(key);
    if (key[3] == 0.0f)
        return;
    dogActorTurn(a, result == 1 ? key[3] : -key[3]);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed, i3mGetLength (the assert handler; getMotionRoot inline)
// ROR 0x0010d1d0

int DogActor::startMotionCmd(int theID, u_int flags)
{
    ROR_ASSERT(theID >= 0); /* 2862 */
    int n = findMotionById(theID);
    if ((flags & 1) && m_motion.getNo() == n)
        return -1;

    float t = 0.0f;
    if (flags & 4)
    {
        /* At the current motion's time: clamped to the new length, or
         * (flag 8) scaled to it. */
        t = getMotionTime();
        if (t < 0.0f)
            t = 0.0f;
        I3mRoot *root = getMotionRoot(n);
        /* PC: no record (the ROM's assert) starts the motion from 0. */
        float len = root != NULL ? i3mGetLength(root) : 0.0f;
        float newlen = getMotionLength(theID);
        if (!(flags & 8))
        {
            if (newlen < t)
                t = newlen;
        }
        else if (len != 0.0f)
        {
            t = (t * newlen) / len;
        }
        else
        {
            t = 0.0f;
        }
    }
    changeMotion(0x1.11112Ep-6f, theID, 0, -1, -1); /* 0x3c888897 */
    if (t != 0.0f)
        m_motion.m_frame = t;
    m_motion_cmd_run = 1;
    return theID;
}

// ──────────────────────────────────────────────────────────────────────
// The motion command list (MagScene::updateActors, once a frame). A word:
// bits 28..31 the op, 22..27 flags (1: the id's record looked up first;
// 0x20: the list holds at this command and the motion stops at its end),
// 12..21 a chance out of 1000 (op 4), 0..9 a motion id or a command index.
// Ops: 1 jump to command op1; 2 play motion op1 once, then the next
// command; 3 loop motion op1 (the command holds); 4 as 2 with the chance.
// Without a list the motion steps alone by m_motion_flags (1 loop; else at
// its end 2 changes to m_motion_flags2, none of 1, 2, 4 stops).
// callcheck: dbgAssertFailed, FUN_0010423c, getRootEndKey, addBaseRotY (the assert handler; utlRandRange; getRootEndKey and addBaseRotY inside dogActorTurnWrap / dogActorTurn)
// ROR 0x0010d388

void DogActor::runMotionCmd()
{
    if (m_motion_cmd_idx >= 0)
    {
        ROR_ASSERT(m_motion_cmd_idx < DOG_MAX_MOTION_CMD); /* 2920 */
        int next = 1;
        /* PC: the ROM spins on an op that does nothing (0, 5..15, a cleared
         * word) and on a jump to itself; the host gives up after a pass
         * over the list. */
        int guard = 0;
        while (next && m_motion_cmd_idx < DOG_MAX_MOTION_CMD && guard++ <= DOG_MAX_MOTION_CMD)
        {
            int r = 0;
            u_int cmd = (u_int)m_motion_cmd[m_motion_cmd_idx];
            int fl = (int)cmd >> 22;
            int op1 = cmd & 0x3ff;
            u_int flags = fl & 0x3f;
            if (fl & 1)
            {
                /* The lookups' results are not used (an inline's leftovers). */
                findMotionById(op1);
                m_motion.getNo();
            }

            int advance = 0;
            switch (cmd >> 28)
            {
            case 1:
                ROR_ASSERT(op1 < DOG_MAX_MOTION_CMD); /* 2953 */
                m_motion_cmd_idx = op1;
                m_motion_cmd_run = 0;
                break;
            case 2:
                r = 1;
                if (m_motion_cmd_run == 0 && startMotionCmd(op1, flags) < 0)
                    advance = 1;
                break;
            case 3:
                r = 2;
                if (m_motion_cmd_run == 0 && startMotionCmd(op1, flags) < 0)
                    advance = 1;
                break;
            case 4:
                if (m_motion_cmd_run == 0)
                {
                    if ((int)((int)cmd >> 12 & 0x3ff) <= utlRandRange(0, 1000)
                        || startMotionCmd(op1, flags) < 0)
                    {
                        advance = 1;
                        break;
                    }
                }
                if (m_motion_cmd_run == 1)
                    r = 1;
                break;
            default:
                break;
            }

            if (!advance)
            {
                if (r == 1)
                {
                    next = 0;
                    m_motion_result = stepMotion();
                    if (m_motion_result != 0)
                        advance = 1;
                }
                else if (r == 2)
                {
                    next = 0;
                    m_motion_result = stepMotionLoop();
                    if (m_motion_result == 1 || m_motion_result == 2)
                        dogActorTurnWrap(this, m_motion_result);
                }
            }
            if (advance)
            {
                if ((fl & 0x20) == 0)
                {
                    m_motion_cmd_run = 0;
                    m_motion_cmd_idx++;
                    ROR_ASSERT(m_motion_cmd_idx < DOG_MAX_MOTION_CMD); /* 3012 */
                    next = 1;
                }
                else
                {
                    m_f4 = -1;
                    m_mode = 0;
                    m_motion.stop();
                }
            }
        }
    }
    else if (m_mode & 4)
    {
        m_f4 = 0;
        m_mode &= ~4;
    }
    else if ((m_mode & 1) == 0)
    {
        m_motion_result = 0;
    }
    else
    {
        u_int mf = m_motion_flags;
        if ((mf & 1) == 0)
        {
            m_motion_result = stepMotion();
            if (m_motion_result != 0)
            {
                if ((mf & 7) == 0)
                {
                    m_mode = 0;
                    m_f4 = -1;
                    m_motion.stop();
                }
                else if (mf & 2)
                {
                    changeMotion(0x1.11112Ep-6f, m_motion_flags2, 1, -1, -1); /* 0x3c888897 */
                    m_mode = 1;
                    m_f4 = 0;
                    m_motion_flags2 = -1;
                }
            }
        }
        else
        {
            m_motion_result = stepMotionLoop();
            if (m_motion_result == 1 || m_motion_result == 2)
                dogActorTurnWrap(this, m_motion_result);
        }
    }

    float trans_y, rot_y;
    if (getBlendInOffset(&trans_y, &rot_y))
    {
        m_trans_ofs[1] = trans_y;
        m_trans_ofs[3] = 1.0f;
        m_dirty = 1;
        setAddRotY(rot_y);
    }
    updateEye();
}

// The blend-in rate both rate getters share (inline in the ROM; asserts
// at lines 3275..3298 and 3325..3349): the motion's own curve when its
// m_param starts with m_interpolation (then a percentage per frame), else
// (m_f4 + 1) / m_interpolation clamped to 0..1.
// callcheck: dbgAssertFailed (the assert handler; getData inline)
static float dogActorBlendInRate(DogActor *a)
{
    /* m_interpolation > 0 */
    ROR_ASSERT(a->m_interpolation > 0); /* 3275 */
    int len;
    int no = a->m_motion.getNo();
    if (no < 0)
    {
        len = a->m_interpolation;
    }
    else
    {
        const int *curve = a->m_motion.getData(no).m_param;
        len = a->m_interpolation;
        if (curve != NULL && *curve == len && a->m_f4 >= 0)
        {
            /* PC: past the curve's last frame the ROM reads on into the
             * pack; the host ends the blend there. */
            if (a->m_f4 >= len)
                return 1.0f;
            return (float)curve[a->m_f4 + 1] / 100.0f;
        }
    }
    ROR_ASSERT(len > 0); /* 3294: m_interpolation > 0 */
    /* PC: the ROM divides by zero after the assert. */
    if (len < 1)
        return 1.0f;
    float r = (float)(a->m_f4 + 1) / (float)len;
    if (r < 0.0f)
        r = 0.0f;
    else if (r > 1.0f)
        r = 1.0f;
    ROR_ASSERT(0.0f <= r && r <= 1.0f); /* 3298 */
    return r;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed, getNo (the assert handler and the motion number inside dogActorBlendInRate)
// ROR 0x0010db48

float DogActor::getBlendInRate()
{
    if (m_mode >= 0 && (m_mode & 2))
        return dogActorBlendInRate(this);
    return 1.0f;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed, FUN_001078dc, getNo (the assert handler; getNodeRate; the motion number inside dogActorBlendInRate)
// ROR 0x0010dd30

float DogActor::getNodeBlendRate(int theNode)
{
    ROR_ASSERT(theNode >= 0 && theNode <= (int)(((int)(sizeof(int)*8))*4)); /* 3313 */
    float r = m_motion.getNodeRate(theNode);
    if (r >= 1.0f)
    {
        r = 1.0f;
        if (m_mode >= 0 && (m_mode & 2))
            r = dogActorBlendInRate(this);
    }
    return r;
}

// ──────────────────────────────────────────────────────────────────────
// The pose of this frame: the current motion sampled, each node blended
// with the previous pose by its rate, the control node's rotation set (or
// added), then the local matrices (with the node scales) and the model
// matrices.
// callcheck: dbgAssertFailed, FUN_001081b0, FUN_0022f610, FUN_00231198, FUN_00230470, FUN_00230660, i3rConvertLocalToModel, FUN_00116130, FUN_00108660 (the assert handler; getPose, the pose lerp and quaternion ops, convertLocalToModel inline, stepAnim, countBlendFrame)
// ROR 0x0010df54

int DogActor::updatePosture(int count)
{
    if (m_motion.getNo() < 0)
        return 0;
    if (m_max_motion < 1)
        return 0;
    /* (m_motion_now.getMainMotionNo() >= 0) && (m_motion_now.getMainMotionNo() < m_max_motion) */
    ROR_ASSERT((m_motion.getNo() >= 0) && (m_motion.getNo() < m_max_motion)); /* 3367 */
    ROR_ASSERT(NULL != m_posture); /* 3368 */
    /* PC: the ROM hangs in the asserts; no pose block, nothing to pose. */
    if (m_motion.getNo() >= m_max_motion || m_posture == NULL || m_mat_local == NULL)
        return 0;

    m_motion.getPose(m_posture);
    if (m_mode & 8)
        copyPosture();
    for (int i = 0; i < m_num_posture_node; i++)
    {
        float rrr = getNodeBlendRate(i);
        ROR_ASSERT(0.0f <= rrr && rrr <= 1.0f); /* 3391 */
        if (rrr != 1.0f)
        {
            u_char *p = m_posture + i * 0x30;
            i3mPoseLerp(rrr, p, p, m_posture_prev + i * 0x30);
        }
    }
    if ((m_mode & 2) && getBlendInRate() >= 1.0f)
        m_mode = (m_mode & ~2) | 1;

    if (m_control_node >= 0)
    {
        /* The node's rotation quaternion (+0x10 of its pose record). */
        float *q = (float *)(m_posture + m_control_node * 0x30 + 0x10);
        if (m_control_node_flg == 0)
        {
            float r[4];
            r[0] = 0.0f;
            r[3] = 1.0f;
            r[2] = 0.0f;
            r[1] = 0.0f;
            i3mQuatFromEuler(r, m_3e0);
            i3mQuatMul(q, r, q);
            i3mQuatNormalize(q, q);
        }
        else
        {
            i3mQuatFromEuler(q, m_3e0);
            i3mQuatNormalize(q, q);
        }
    }

    ROR_ASSERT(NULL != m_mat_local); /* 3494 */
    i3mPoseToMatrices(m_posture, m_mat_local, m_assign_tbl);
    for (int i = 0; i < DOG_SCALE_NODE_MAX; i++)
    {
        /* Node 0 is never scaled (the ROM tests > 0). */
        int node = m_scale_node_no[i];
        if (node > 0)
        {
            for (int row = 0; row < 4; row++)
                i3dVecScaleXYZ(m_scale_node_val[i], m_mat_local[node][row], m_mat_local[node][row]);
        }
    }
    convertLocalToModel((I3dMatrixF4x4 *)m_mat_model, (const I3dMatrixF4x4 *)m_mat_local);
    if (m_16c != NULL)
        stepAnim();
    if (count)
        countBlendFrame();
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 12 (+0x60). The posture is updated every m_45c-th frame (every
// frame when 0); a frame without it draws with the model matrices as
// they are. The root's turn (m_rot_y_aar) goes into the heading after the
// draw.
// callcheck: dbgAssertFailed, FUN_0010df54, addBaseRotY, FUN_00108660 (the assert handler; updatePosture, addBaseRotY inside dogActorTurn, countBlendFrame); transformBillboard / transform / drawModel are vtable slots 11 / 2 / 19
// ROR 0x0010e5c0

void DogActor::draw(I3dLayer *theRenderStage, const float *cam_rotate, const float *cam_trans)
{
    ROR_ASSERT(theRenderStage); /* 3863 */
    ROR_ASSERT(m_i3r_root); /* 3864 */
    int update = 1;
    if (m_45c > 0)
    {
        if (++m_460 < m_45c)
            update = 0;
        else
            m_460 = 0;
    }
    int nodes = update ? updatePosture(0) : 1;
    if (!transformBillboard(cam_rotate, cam_trans))
        transform();
    if (m_hide == 0)
        drawModel(theRenderStage, nodes, NULL);
    if (m_rot_y_aar != 0.0f)
        dogActorTurn(this, m_rot_y_aar);
    if (update)
        countBlendFrame();
    m_mode &= ~8;
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 19 (+0x98). The attention node's material shows the actor's
// pulsing display record for the draw; a model with an m_16c animation
// draws with its value.
// callcheck: FUN_001068d4, FUN_0010686c (utlGetI3rNodeDisp / utlSetI3rNodeDisp)
// ROR 0x0010e3ac

void DogActor::drawModel(I3dLayer *layer, int nodes, DogEntity *src)
{
    void *saved = NULL;
    if (m_attention >= 0)
    {
        u_int t = g_magSystem.m_frame & 0x1f;
        if (t > 15)
            t = 0x1f - t;
        saved = utlGetI3rNodeDisp(m_i3r_root, m_attention);
        float s = (float)(int)t * 0.25f + 1.0f;
        m_4b0[0] = s;
        m_4b0[2] = s;
        m_4b0[1] = s;
        utlSetI3rNodeDisp(m_i3r_root, m_4a0, m_attention);
    }
    DogEntity *ent = src != NULL ? src : this;
    const DogModelAnim *anim = (const DogModelAnim *)m_16c;
    if (anim == NULL)
    {
        utlDrawI3r(layer, m_i3r_root, nodes ? m_mat_model : NULL, ent->m_matrix, NULL);
    }
    else
    {
        struct { u_int m_mask; float m_value[5]; } opt;
        opt.m_mask = 0x1f;
        for (int i = 0; i < 5; i++)
            opt.m_value[i] = anim->m_value;
        utlDrawI3r(layer, m_i3r_root, NULL, ent->m_matrix, &opt);
    }
    if (m_attention >= 0)
        utlSetI3rNodeDisp(m_i3r_root, saved, m_attention);
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 20 (+0xa0): the shadow model (the pack's file 2) when there is
// one.
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0010e534

void DogActor::drawShadow(I3dLayer *theRenderStage)
{
    ROR_ASSERT(theRenderStage); /* 3829 */
    ROR_ASSERT(m_i3r_root); /* 3830 */
    I3rRoot *root = m_i3r_root2 != NULL ? m_i3r_root2 : m_i3r_root;
    utlDrawI3r(theRenderStage, root, m_mat_model, m_matrix, NULL);
}

// ROR 0x0010e7dc

void DogActor::addBaseRotY(float r)
{
    float y = m_base_rotate[1] + r;
    m_base_rotate[1] = y;
    if (y < -DOG_PI)
        m_base_rotate[1] = y + DOG_PI2;
    else if (y >= DOG_PI)
        m_base_rotate[1] = y - DOG_PI2;
}

// ──────────────────────────────────────────────────────────────────────
// While blending in, the Y offset and turn the last motion left fade out
// into the new motion's root.
// callcheck: DogActor::isBlendIn, FUN_0010db48, DogActor::getRootMove (called)
// ROR 0x0010e848

int DogActor::getBlendInOffset(float *trans_y, float *rot_y)
{
    *rot_y = 0.0f;
    *trans_y = 0.0f;
    if (!isBlendIn())
        return 0;
    float move[4];
    move[2] = 0.0f;
    move[3] = 1.0f;
    move[1] = 0.0f;
    move[0] = 0.0f;
    float r = getBlendInRate();
    float k = 1.0f - r;
    getRootMove(0.0f, move);
    *trans_y = m_trans_ofs_y_save * k + move[1] * r;
    float a = m_rot_ofs_y;
    if (a < -0x1.921faep+1f) /* 0xc0490fd7 */
        a = DOG_PI;
    *rot_y = -a * k;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The model pack's file 0 name is theName up to its extension.
// callcheck: dbgAssertFailed, BedPfh::BedPfh, BedPfh::getFileName (the assert handler; called)
// ROR 0x0010f2dc

int DogActor::checkName(const char *theName)
{
    ROR_ASSERT(theName); /* 4614 */
    if (theName == NULL)
        return 0;
    if (m_model_copy == NULL)
        return 0;
    BedPfh pfh(m_model_copy);
    const char *name = pfh.getFileName(0);
    if (name == NULL)
        return 0;
    for (;;)
    {
        char c = *theName++;
        char d = *name++;
        if (c == '\0')
            return d == '\0' || d == '.';
        if (d == '\0' || d == '.' || d != c)
            return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The blink: once m_eye_count reaches m_eye_period, the eye texture runs
// 2 for 4 frames, 1, then 0, and a new period of 60..90 frames starts (or
// none, half the time, after a long one). Flag 2 (eyeStart) ends at the
// open eye instead.
// callcheck: FUN_0010423c, DogActor::setEyeTex (utlRandRange; called)
// ROR 0x0010f480

void DogActor::updateEye()
{
    /* Frames, texture state; -1 ends. PC: the ROM's stack table has no
     * eighth word (it reads garbage after the end); -1 here. */
    static const int s_seq[8] = { 3, 2, 1, 1, 1, 0, -1, -1 };

    if ((m_eye_flags & 1) == 0)
        return;
    if (++m_eye_count < m_eye_period)
        return;
    int t = m_eye_count - m_eye_period;
    int i = 0;
    for (; s_seq[i] != -1; i += 2)
    {
        if (t <= s_seq[i])
        {
            setEyeTex(s_seq[i + 1]);
            break;
        }
        t -= s_seq[i];
    }
    if ((m_eye_flags & 2) && s_seq[i + 1] == 0)
    {
        m_eye_flags &= ~3u;
        return;
    }
    if (s_seq[i] == -1)
    {
        if (m_eye_period < 60 || utlRandRange(0, 1) != 0)
            m_eye_period = utlRandRange(60, 90);
        else
            m_eye_period = 0;
        m_eye_count = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The eye texture: the eye node is looked up again, its material's entity
// 0 pointed at m_eye_uv with v = {0, 0.32, 0.65}[state] (the texture's
// rows: open, half, closed -- unverified), and the node's change count
// bumped.
// callcheck: i3rGetNumChild52, i3rGetChildName52, i3rGetChild52, i3rGetChild25, strstr, i3rSetMatEntity0 (called)
// ROR 0x0010f5e4

void DogActor::setEyeTex(int state)
{
    static const float s_eye_v[3] = { 0.0f, 0x1.48p-2f, 0x1.4cp-1f }; /* 0x3ea40000, 0x3f260000 */

    if (state == m_eye_state)
        return;
    m_eye_node = NULL;
    m_eye_mat = NULL;
    I3rItem *root = &m_i3r_root->m_item;
    int n = i3rGetNumChild52(root);
    for (int i = 0; i < n; i++)
    {
        if (strstr(i3rGetChildName52(root, i), "eye") != NULL)
        {
            m_eye_node = i3rGetChild52(root, i);
            if (m_eye_node == NULL)
            {
                m_eye_state = state;
                return;
            }
            m_eye_mat = i3rGetChild25(m_eye_node, 0);
            break;
        }
    }
    if (m_eye_node != NULL && m_eye_mat != NULL)
    {
        m_eye_uv[1] = s_eye_v[state];
        i3rSetMatEntity0(m_eye_mat, m_eye_uv);
        u_short *changes = (u_short *)((u_char *)m_eye_node->getData() + 8);
        (*changes)++;
    }
    m_eye_state = state;
}

// ──────────────────────────────────────────────────────────────────────
// The root track, read for the callers that move the actor themselves.

static void dogCopy4(float *dst, const float *src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00108bb0

float DogActor::getTrackKeyRotY(const DogMotionTrack *trk, int n)
{
    if (trk == NULL)
        return 0.0f;
    const float *k = trk->m_data + n * getTrackStride(trk);
    switch (getTrackType(trk))
    {
    case 0:
        ROR_ASSERT(0); /* 288 */
        break;
    case 1:
    case 4:
    case 0x14:
        return k[3];
    case 2:
    case 0x12:
        return k[1];
    case 3:
    case 5:
    case 0x13:
    case 0x15:
        break;
    case 6:
        return k[0];
    default:
        ROR_ASSERT(0); /* 308 */
        break;
    }
    return 0.0f;
}

// callcheck: dbgAssertFailed, DogMotionBlend::getNo (the assert handler; getMotionAuxData inline, getNo called)
// ROR 0x00109634

int DogActor::getRootTrackEnd(float *end)
{
    DogMotionTrack *trk = getMotionAuxData(m_motion.getNo());
    if (trk == NULL)
    {
        if (end != NULL)
        {
            end[0] = 0.0f;
            end[3] = 1.0f;
            end[2] = 0.0f;
            end[1] = 0.0f;
        }
        return 0;
    }
    int n = getTrackNumFrame(trk);
    if (end != NULL)
        getTrackKey(trk, end, n - 1);
    return n;
}

// A non-looping motion clamps both frames to the last key. A looping one
// reports the track's whole turn in rot when the step crossed the end
// (forward) or the start (backward), so the caller can carry it over.
// callcheck: dbgAssertFailed, DogMotionBlend::getNo (the assert handler; getMotionAuxData inline, getNo called)
// ROR 0x00109348

int DogActor::getMotionMove(float t0, float t1, float *move, float *rot, float *pos)
{
    DogMotionTrack *trk = getMotionAuxData(m_motion.getNo());
    if (trk == NULL)
    {
        *rot = 0.0f;
        move[3] = 1.0f;
        move[0] = 0.0f;
        move[2] = 0.0f;
        move[1] = 0.0f;
        return 0;
    }
    u_int flags = m_motion_flags;
    if ((flags & 1) == 0)
    {
        float last = (float)(getTrackNumFrame(trk) - 1);
        if (last < t0)
            t0 = last;
        if (last < t1)
            t1 = last;
    }
    float start[4];
    start[2] = 0.0f;
    start[1] = 0.0f;
    start[0] = 0.0f;
    start[3] = 1.0f;
    getRootMove(t0, start);
    getRootMove(t1, move);
    if (pos != NULL)
        dogCopy4(pos, move);
    move[0] -= start[0];
    move[1] -= start[1];
    move[2] -= start[2];
    move[3] -= start[3];
    if ((getTrackType(trk) & 0x10) == 0)
    {
        move[1] = 0.0f;
    }
    else
    {
        float first[4];
        first[2] = 0.0f;
        first[1] = 0.0f;
        first[0] = 0.0f;
        first[3] = 1.0f;
        getRootMove(0.0f, first);
        if (pos != NULL)
            pos[1] = first[1];
    }
    *rot = 0.0f;
    if ((flags & 1) == 0)
        return 1;
    float speed = m_motion.m_speed;
    int n = getTrackNumFrame(trk);
    float turn = getTrackKeyRotY(trk, n - 1);
    if (speed > 0.0f)
    {
        if ((float)n - 1.0f <= t0 || t1 < (float)n - 1.0f)
        {
            /* Not across the end: wrapped only when t1 came round below t0. */
            if (t0 < 0.0f || t1 < 0.0f || t0 <= t1)
                return 1;
        }
        *rot = turn;
        return 2;
    }
    if (speed < 0.0f && t0 >= 0.0f && t1 < 0.0f)
    {
        *rot = -turn;
        return 2;
    }
    return 1;
}

// The tag track: word 0 the frame count (bit 31 set), then one word of
// event bits per frame.
// callcheck: dbgAssertFailed, DogMotionBlend::getNo, DogActor::findMotionById, utlFtoi15 (the assert handler; getMotionTagData inline)
// ROR 0x0010bcc8

u_int DogActor::getMotionEvent(float t0, float t1, int motion)
{
    if (motion < 0)
        motion = m_motion.getNo();
    else if (motion < 0x10000)
        motion = findMotionById(motion);
    const u_int *tag = getMotionTagData(motion);
    if (tag == NULL)
        return 0;
    int i0 = utlFtoi15(t0) >> 15;
    int n = (int)(tag[0] & 0x7fffffff);
    if (i0 < 0 || i0 >= n)
        return 0;
    int i1 = utlFtoi15(t1) >> 15;
    if (i1 < 0 || i1 >= n)
        return 0;
    if (i0 == i1)
        return 0;
    u_int bits = 0;
    do
    {
        i0++;
        if (i0 >= n)
            i0 -= n;
        bits |= tag[i0 + 1];
    } while (i0 != i1);
    return bits;
}

// ROR 0x0010be1c

u_int DogActor::getMotionEventNow()
{
    float t = getMotionFrame();
    if (t < 0.0f)
        return 0;
    return getMotionEvent(t, t + 1.0f, -1);
}

// ──────────────────────────────────────────────────────────────────────
// Placing the actor against its root track.
// The heading both take: rot_y less the root's turn at the current frame.

static void dogActorSetHeading(DogActor *a, float rot_y)
{
    float v[4];
    v[3] = 1.0f;
    v[0] = 0.0f;
    v[2] = 0.0f;
    v[1] = 0.0f;
    float t = a->getMotionFrame();
    a->getRootMove(t + t, v);
    float r = rot_y - v[3];
    if (r < -DOG_PI)
        r += DOG_PI2;
    else if (r >= DOG_PI)
        r -= DOG_PI2;
    a->m_rotate[1] = r;
    a->normalizeRotate();
    a->m_dirty = 1;
    a->m_base_rotate[1] = r;
    a->setAddRotY(0.0f);
}

// callcheck: getMotionFrame, getRootMove, normalizeRotate, setAddRotY (in dogActorSetHeading)
// ROR 0x0010a090

void DogActor::addTransRotY(const float *d, float trans_ofs_y, float rot_y)
{
    m_trans[0] += d[0];
    m_trans[1] += d[1];
    m_trans[3] = 1.0f;
    m_trans[2] += d[2];
    m_trans_ofs[1] = trans_ofs_y;
    m_dirty = 1;
    m_trans_ofs[3] = 1.0f;
    dogActorSetHeading(this, rot_y);
}

// callcheck: getMotionFrame, getRootMove, normalizeRotate, setAddRotY (in dogActorSetHeading)
// ROR 0x0010a1c8

void DogActor::setTransRotY(const float *d, float trans_ofs_y, float rot_y)
{
    m_trans_ofs[3] = 1.0f;
    m_trans_ofs[1] = trans_ofs_y;
    dogCopy4(m_trans, d);
    m_dirty = 1;
    dogActorSetHeading(this, rot_y);
}

// trans is the root move's scratch before it takes m_trans (resetRootBase
// passes m_base_trans / m_base_rotate themselves: rot reads the base
// before it writes it). The wrap here is <= -pi / > pi, unlike the others.
// callcheck: dbgAssertFailed, DogMotionBlend::getNo (the assert handler; getMotionAuxData inline, getNo called)
// ROR 0x0010a2d0

void DogActor::getRootBase(float *trans, float *rot)
{
    DogMotionTrack *trk = getMotionAuxData(m_motion.getNo());
    if (trk == NULL)
    {
        dogCopy4(trans, m_trans);
        dogCopy4(rot, m_rotate);
        return;
    }
    float t = getMotionFrame();
    if (t + t < 0.0f)
    {
        dogCopy4(trans, m_base_trans);
        dogCopy4(rot, m_base_rotate);
        return;
    }
    getRootMove(t + t, trans);
    float base[4];
    dogCopy4(base, m_base_rotate);
    float y = trans[3] + base[1];
    rot[3] = 1.0f;
    rot[1] = y;
    rot[0] = base[0] + 0.0f;
    rot[2] = base[2] + 0.0f;
    if (y <= -DOG_PI)
        rot[1] = y + DOG_PI2;
    else if (y > DOG_PI)
        rot[1] = y - DOG_PI2;
    dogCopy4(trans, m_trans);
}

// callcheck: dbgAssertFailed, DogMotionBlend::getNo (the assert handler; getMotionAuxData inline, getNo called)
// ROR 0x0010a448

void DogActor::getRootRotate(float *rot)
{
    DogMotionTrack *trk = getMotionAuxData(m_motion.getNo());
    if (trk == NULL)
    {
        dogCopy4(rot, m_rotate);
        return;
    }
    dogCopy4(rot, m_base_rotate);
    float t = getMotionFrame();
    if (t + t >= 0.0f)
    {
        float v[4];
        v[0] = 0.0f;
        v[3] = 1.0f;
        v[2] = 0.0f;
        v[1] = 0.0f;
        getRootMove(t + t, v);
        rot[1] += v[3];
        if (rot[1] < -DOG_PI)
            rot[1] += DOG_PI2;
        if (rot[1] >= DOG_PI)
            rot[1] -= DOG_PI2;
    }
}

// ROR 0x0010ac98

void DogActor::resetRootBase()
{
    saveRootOffset();
    m_rotate[1] = getRotY();
    normalizeRotate();
    m_dirty = 1;
    m_80[3] = 1.0f;
    m_80[0] = 0.0f;
    m_80[2] = 0.0f;
    m_80[1] = 0.0f;
    m_dirty = 1;
    getRootBase(m_base_trans, m_base_rotate);
}

// ──────────────────────────────────────────────────────────────────────
// Motion records and starts.
// callcheck: dbgAssertFailed, strcmp (the assert handler)
// ROR 0x0010ab40

int DogActor::getMotionNoByName(const char *theName)
{
    ROR_ASSERT(theName); /* 1635 */
    for (int i = 0; i < m_max_motion; i++)
    {
        const char *name = m_motion_data[i].m_name;
        if (name != NULL && strcmp(name, theName) == 0)
            return m_motion_data[i].m_no;
    }
    return -1;
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0010abf4

void DogActor::setMotionCmd(const int *theCmd, int theLength)
{
    ROR_ASSERT(theCmd); /* 1659 */
    ROR_ASSERT(theLength > 0 && theLength <= DOG_MAX_MOTION_CMD); /* 1660 */
    for (int i = 0; i < theLength; i++)
        m_motion_cmd[i] = theCmd[i];
    m_motion_cmd_idx = 0;
}

// callcheck: dbgAssertFailed (the assert handler; m_motion.getData inline)
// ROR 0x0010ad0c

void DogActor::setMotionInterpolation(int theN, int theTime)
{
    ROR_ASSERT(m_max_motion > 0); /* 1700 */
    ROR_ASSERT(theN >= 0 && theN < m_max_motion); /* 1701 */
    if (theTime < 0)
    {
        int *param = m_motion.getData(theN).m_param;
        theTime = 0;
        if (param != NULL)
            theTime = *param;
    }
    setParam1d8(theTime);
}

// a1 (theID) passes straight through to getMotionLength.
// ROR 0x0010ae10

void DogActor::resetMotionTime(int theID)
{
    if (m_speed0 >= 0.0f)
        m_motion.m_frame = 0.0f;
    else
        m_motion.m_frame = getMotionLength(theID);
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0010ae90

void DogActor::setMotionNo(int theN)
{
    ROR_ASSERT(m_max_motion > 0); /* 1780 */
    ROR_ASSERT(theN >= 0 && theN < m_max_motion); /* 1781 */
    m_motion.m_no = theN;
    startMotionMode();
}

// The two blend entry starts share this body (inline in the ROM): the
// entry's node mask from the names, each named node's pose kept as the
// previous one, then the entry restarts on record no.
static int dogActorStartSub(DogActor *a, int theS, int no, const char **theNodes)
{
    if (no < 0 || no >= a->m_max_motion)
        return -2;
    if (a->m_motion_data[no].m_i3m_root == NULL)
        return -3;
    DogMotionBlendEntry &e = a->m_motion.m_blend[theS];
    for (int i = 0; i < 4; i++)
        e.m_node_mask[i] = 0;
    for (; *theNodes != NULL; theNodes++)
    {
        int i = a->i3mGetNodeIndexByName(no, *theNodes);
        ROR_ASSERT(i >= 0); /* 1993 */
        if (i < 0)
            continue;   // PC: the ROM sets a bit before the mask and copies pose -1
        e.m_node_mask[i / 32] |= 1 << (i & 31);
        memcpy(a->m_posture_prev + i * 0x30, a->m_posture + i * 0x30, 0x30);
    }
    e.m_no = no;
    e.m_frame = 0.0f;
    e.m_speed = 0x1.11112Ep-6f; /* 0x3c888897 */
    e.m_1c = -1.0f;
    return no;
}

static int dogActorClearSub(DogActor *a, int theS)
{
    DogMotionBlendEntry &e = a->m_motion.m_blend[theS];
    e.m_no = -1;
    e.m_frame = 0.0f;
    e.m_speed = 0x1.11112Ep-6f; /* 0x3c888897 */
    e.m_1c = -1.0f;
    return -1;
}

// callcheck: dbgAssertFailed, i3mGetNodeIndexByName (the assert handler; i3mGetNodeIndexByName inline)
// ROR 0x0010b328

int DogActor::setMotionSub(int theS, int theID, const char **theNodes)
{
    if (theID < 0)
        return dogActorClearSub(this, theS);
    return dogActorStartSub(this, theS, findMotionById(theID), theNodes);
}

// callcheck: dbgAssertFailed, i3mGetNodeIndexByName (the assert handler; i3mGetNodeIndexByName inline)
// ROR 0x0010b620

int DogActor::setMotionSubBySuffix(int theS, const char *theName, const char **theNodes)
{
    if (theName == NULL)
        return dogActorClearSub(this, theS);
    return dogActorStartSub(this, theS, findMotionBySuffix(theName), theNodes);
}

// callcheck: dbgAssertFailed, i3mGetLength (the assert handler; getBlendNo / getMotionRoot inline)
// ROR 0x0010b918

void DogActor::setMotionTimeRatioSub(float r, int n)
{
    I3mRoot *root = getMotionRoot(m_motion.getBlendNo(n));
    float len = root != NULL ? i3mGetLength(root) : 0.0f;  // PC: NULL only where the ROM hangs in the assert
    DogMotionBlendEntry &e = m_motion.m_blend[n];
    e.m_1c = r;
    e.m_frame = len * r;
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0010ba20

void DogActor::setMotionRateSub(float theR, int theS)
{
    ROR_ASSERT(theS >= 0); /* 2083 */
    ROR_ASSERT(theR >= 0.0f && theR <= 1.0f); /* 2084 */
    m_motion.m_blend[theS].m_1c = theR;
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0010bac4

void DogActor::fadeMotionRateSub(float theR, int theS, int theTime)
{
    ROR_ASSERT(theS >= 0); /* 2093 */
    ROR_ASSERT(theR >= 0.0f && theR <= 1.0f); /* 2094 */
    ROR_ASSERT(theTime >= 0.0f && theTime <= 1000); /* 2095 */
    m_motion.setBlendRate(theR, theS, theTime);
}

// callcheck: dbgAssertFailed, i3mGetLength (the assert handler; getBlendNo / getMotionRoot inline)
// ROR 0x0010bbac

float DogActor::getMotionTimeRatioSub(int n)
{
    I3mRoot *root = getMotionRoot(m_motion.getBlendNo(n));
    float len = root != NULL ? i3mGetLength(root) : 0.0f;
    if (len > 0.0f)
    {
        float frame = m_motion.m_blend[n].m_frame;
        if (frame > 0.0f)
            return frame / len;
    }
    return 0.0f;
}

// ROR 0x0010da18

float DogActor::getMotionTimeRatio()
{
    if (m_mode != 2)
        return m_motion.getTimeRatio();
    return 0.0f;
}

// ROR 0x0010da98

const char *DogActor::getMotionName(int theID)
{
    int i = findMotionById(theID);
    if (i < 0)
        return NULL;
    return m_motion_data[i].m_name;
}

// ROR 0x0010db18

int DogActor::getModeArg()
{
    if (m_mode == 2)
        return m_f4;
    return -1;
}

// ROR 0x0010e328

void DogActor::resetMotion(int theN)
{
    m_rot_y_aar = 0.0f;
    m_80[3] = 1.0f;
    m_80[2] = 0.0f;
    m_80[1] = 0.0f;
    m_80[0] = 0.0f;
    m_trans_ofs[3] = 1.0f;
    m_dirty = 1;
    m_trans_ofs[1] = 0.0f;
    setParam1d8(0);
    setMotionNo(theN);  /* the ROM passes a third word 0; setMotionNo takes one */
    m_motion.m_frame = 0.0f;
}

// callcheck: dbgAssertFailed, i3mGetNodeIndexByName, DogMotionBlend::getNo (the assert handler; i3mGetNodeIndexByName inline)
// ROR 0x0010e9ac

int DogActor::setControlNode(const char *theName)
{
    int no;
    if (theName == NULL || (no = m_motion.getNo()) < 0)
    {
        m_control_node = -1;
        return -1;
    }
    m_control_node = i3mGetNodeIndexByName(no, theName);
    return m_control_node;
}

// ──────────────────────────────────────────────────────────────────────
// Nodes.
// callcheck: dbgAssertFailed, i3dMatMul (the assert handler)
// ROR 0x0010eab8

void DogActor::getNodeWorldMatrix(sceVu0FMATRIX out, int n)
{
    ROR_ASSERT(m_mat_model); /* 4088 */
    i3dMatMul(out, m_matrix, m_mat_model[n]);
}

// callcheck: dbgAssertFailed, i3dMatMul (the assert handler)
// ROR 0x0010eb28

void DogActor::getNodeWorldPos(float *out, int n)
{
    ROR_ASSERT(m_mat_model); /* 4112 */
    sceVu0FMATRIX m;
    i3dMatMul(m, m_matrix, m_mat_model[n]);
    dogCopy4(out, m[3]);
}

// ROR 0x0010eba0

void DogActor::setNodeScale(float scale, int node)
{
    for (int i = 0; i < DOG_SCALE_NODE_MAX; i++)
    {
        if (m_scale_node_no[i] == node)
        {
            if (scale == 1.0f)
                m_scale_node_no[i] = -1;
            else
                m_scale_node_val[i] = scale;
            return;
        }
    }
    for (int i = 0; i < DOG_SCALE_NODE_MAX; i++)
    {
        if (m_scale_node_no[i] < 0 || m_scale_node_val[i] == 1.0f)
        {
            m_scale_node_no[i] = node;
            m_scale_node_val[i] = scale;
            return;
        }
    }
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0010ec8c

void DogActor::checkNodeScale()
{
    for (int i = 0; i < DOG_SCALE_NODE_MAX; i++)
    {
        ROR_ASSERT(m_scale_node_no[i] == -1); /* 4174 */
        ROR_ASSERT(m_scale_node_val[i] == 1.0f); /* 4175 */
    }
}

// ──────────────────────────────────────────────────────────────────────
// Sharing, construction.
// The master's model and records, reference counted (releaseModel /
// releaseShared undo it).
// callcheck: dbgAssertFailed, memcpy (the assert handler; the copy is sized for the host records)
// ROR 0x0010cffc

int DogActor::shareModel(DogActor *src)
{
    clear();
    int n = src->m_max_motion;
    ROR_ASSERT(n>=0); /* 2782 */
    setMaxMotion(n);
    DogActor *orig = static_cast<DogActor *>(src->m_original);
    if (orig == NULL)
        orig = src;
    m_load_flag = LOAD_SHARE;
    m_original = orig;
    m_id = src->m_id;
    m_i3r_root = orig->m_i3r_root;
    allocMatrix();
    if (orig->m_motion_data != NULL && n > 0)
    {
        memcpy(m_motion_data, orig->m_motion_data, n * sizeof(DogMotionData));
        allocPosture(0);
    }
    orig->m_ref_count++;
    m_f4 = -1;
    m_mode = 0;
    return 0;
}

// The second constructor: the first one's body (the entity and model
// words inline, both motion states, the three vectors, clear), then the
// records. Delegating gives the same order on the host.
// callcheck: DogMotionBlend, clear (run by the delegated constructor)
// ROR 0x0010f008

DogActor::DogActor(int theN) : DogActor()
{
    setMaxMotion(theN);
}

// ~DogActor (ROR 0x0010f16c: releaseModel(NULL), DogModel::release(1),
// operator delete when flagged) is not a host destructor, as ~DogModel is
// not: the scene's actors are global and the scene releases them itself.

// ──────────────────────────────────────────────────────────────────────
// The ELF's inline accessors (linkonce copies of the class body's
// inlines), defined out of line.
// DogActor::__tf (ROR 0x00280f68) is the GCC 2 type_info function: the
// host has its own RTTI.
// ROR 0x00280fb8

void DogActor::saveTransOfsY() { m_trans_ofs_y_save = m_trans_ofs[1]; }

// ROR 0x00280fc4

float DogActor::getTransOfsYSave() const { return m_trans_ofs_y_save; }

// ROR 0x00280fcc

float DogActor::getRotOfsYSave() const { return m_rot_ofs_y; }

// ROR 0x00280fd4

void DogActor::setId(int id) { m_id = id; }

// ROR 0x00280fdc

int DogActor::getId() const { return m_id; }

// ROR 0x00280fe4

int DogActor::getMode() const { return m_mode; }

// ROR 0x00280fec

void DogActor::ready()
{
    m_f4 = -1;
    m_mode = 0;
}

// ROR 0x00281000

int DogActor::getMotionLoopTrig() const { return m_motion_result; }

// ROR 0x00281008

int DogActor::getMotionNo() { return m_motion.getNo(); }

// ROR 0x00281020

void DogActor::setMotionSpeed(float s) { m_motion.m_speed = s; }

// ROR 0x00281028

float DogActor::getMotionSpeed() const { return m_motion.m_speed; }

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x00281030

void DogActor::setMotionSpeedSub(float s, int n)
{
    ROR_ASSERT(n>=0 && n<=4); /* 233 */
    m_motion.m_blend[n].m_speed = s;
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0028109c

float DogActor::getMotionSpeedSub(int n)
{
    ROR_ASSERT(n>=0 && n<=4); /* 234 */
    return m_motion.m_blend[n].m_speed;
}

// ROR 0x002810fc

void DogActor::setMotionSpeed0(float s) { m_speed0 = s; }

// ROR 0x00281104

void DogActor::setMotionTime0(float t) { m_time0 = t; }

// ROR 0x0028110c

int DogActor::getInterpolation() const { return m_interpolation; }

// callcheck: dbgAssertFailed (the assert handler; m_motion.getData inline)
// ROR 0x00281114

int DogActor::getMotionLinkTime(int n) { return *m_motion.getData(n).m_param; }

// callcheck: dbgAssertFailed (the assert handler; getBlendNo inline)
// ROR 0x002811a4

int DogActor::getMotionNoSub(int n) { return m_motion.getBlendNo(n); }

// ROR 0x00281204

int DogActor::getNumMotion() const { return m_max_motion; }

// ROR 0x0028120c

void DogActor::setMotionTime(float t) { m_motion.m_frame = t; }

// ROR 0x00281214

void DogActor::setMotionTimeRatio(float r) { m_motion.setTimeRatio(r); }

// ROR 0x00281230

void DogActor::setMotionFlag(int flags, int flags2)
{
    m_motion_flags2 = flags2;
    m_motion_flags = flags;
}

// ROR 0x0028123c

float DogActor::getBaseRotateY() const { return m_base_rotate[1]; }

// ROR 0x00281244

void DogActor::setBaseRotate(const float *v)
{
    m_base_rotate[0] = v[0];
    m_base_rotate[1] = v[1];
    m_base_rotate[2] = v[2];
    m_base_rotate[3] = v[3];
}

// ROR 0x0028126c

void DogActor::setBaseRotate(float x, float y, float z)
{
    m_base_rotate[0] = x;
    m_base_rotate[3] = 1.0f;
    m_base_rotate[1] = y;
    m_base_rotate[2] = z;
}

// ROR 0x0028128c

void DogActor::clearRotYAAR() { m_rot_y_aar = 0.0f; }

// ROR 0x00281294

void DogActor::clearRotOfsY() { m_rot_ofs_y = 0.0f; }

// ROR 0x0028129c

void DogActor::setControlNodeFlg(int f) { m_control_node_flg = f; }

// ROR 0x002812a4

int DogActor::getControlNode() const { return m_control_node; }

// ROR 0x002812ac

int DogActor::getControlNodeFlg() const { return m_control_node_flg; }

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x002812b4

sceVu0FMATRIX *DogActor::getNodeMatrixPtr(int n)
{
    ROR_ASSERT(m_mat_model); /* 322 */
    return m_mat_model + n;
}

// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0028130c

I3mRoot *DogActor::GetI3mRootPtr(int n)
{
    ROR_ASSERT(n>=0); /* 342 */
    ROR_ASSERT(m_motion_data); /* 342 */
    ROR_ASSERT(m_motion_data[n].m_i3m_root); /* 342 */
    return m_motion_data[n].m_i3m_root;
}

// callcheck: dbgAssertFailed (the assert handler; getRootTrack inline)
// ROR 0x002813b4

DogMotionTrack *DogActor::getMotionAuxData(int n)
{
    if (n < 0)
        return NULL;
    return getRootTrack(n);
}

// The tag track's layout is not DogMotionTrack's (see getMotionEvent).
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x0028141c

const u_int *DogActor::getMotionTagData(int n)
{
    if (n < 0)
        return NULL;
    ROR_ASSERT(m_motion_data); /* 346 */
    if (m_motion_data == NULL)  // PC: the ROM hangs in the assert
        return NULL;
    return (const u_int *)m_motion_data[n].m_tag_data;
}

// callcheck: dbgAssertFailed (the assert handler; getMotionRoot inline)
// ROR 0x00281484

float DogActor::getMaxTime0(int n)
{
    I3mRoot *root = getMotionRoot(n);
    return root != NULL ? i3mGetLength(root) : 0.0f;  // PC: NULL only where the ROM hangs in the assert
}

// callcheck: dbgAssertFailed, i3mRelocate (the assert handler; relocateMotion inline)
// ROR 0x00281518

int DogActor::unpack(int n) { return relocateMotion(n); }

// callcheck: dbgAssertFailed, i3mGetNumNode (the assert handler; getMotionNumNode inline)
// ROR 0x002815d4

int DogActor::get_nNode(int n) { return getMotionNumNode(n); }

// callcheck: dbgAssertFailed, i3mGetNodeName (the assert handler; getMotionNodeName inline)
// ROR 0x00281690

const char *DogActor::getNodeName(int n, int i) { return getMotionNodeName(n, i); }

// ROR 0x00281b54

void DogActor::setMotionSpeedRatio(float r)
{
    m_speed_ratio = r;
    m_motion.m_0c = r;
}

// ROR 0x00281b60

void DogActor::setMotionTickTime(int t)
{
    m_motion.m_14 = 0;
    m_motion.m_10 = t;
    m_45c = t;
    m_460 = 0;
}

// The pose readers (the class body's inlines at lines 372..383).
// callcheck: dbgAssertFailed, i3mGetNodeIndexByName (the assert handler)
// ROR 0x00281778

int DogActor::i3mGetNodeIndexByName(int n, const char *p)
{
    ROR_ASSERT(n>=0 & n<m_max_motion); /* 372 */
    ROR_ASSERT(m_motion_data); /* 372 */
    ROR_ASSERT(m_motion_data[n].m_i3m_root); /* 372 */
    ROR_ASSERT(p); /* 372 */
    return ::i3mGetNodeIndexByName(m_motion_data[n].m_i3m_root, p);
}

// The ELF's signature: getMotion(int, tag_I3mPosture_s *, float, I3M_INTER).
// callcheck: dbgAssertFailed, i3mGetPose (the assert handler)
// ROR 0x00281860

void DogActor::getMotion(int n, void *p, float t, int inter)
{
    ROR_ASSERT(n>=0 & n<m_max_motion); /* 374 */
    ROR_ASSERT(m_motion_data); /* 374 */
    ROR_ASSERT(m_motion_data[n].m_i3m_root); /* 374 */
    ROR_ASSERT(p); /* 374 */
    ROR_ASSERT(t>=0); /* 374 */
    i3mGetPose(m_motion_data[n].m_i3m_root, p, inter, t);
}

// The ELF's signature: getBlendMotion(int, tag_I3mPosture_s *, float, int,
// float, float, I3M_INTER).
// callcheck: dbgAssertFailed, FUN_0022eab0 (the assert handler; i3mGetBlendPose)
// ROR 0x00281990

void DogActor::getBlendMotion(int m1, void *p, float t1, int m2, float t2, float r, int inter)
{
    ROR_ASSERT(m_motion_data); /* 377 */
    ROR_ASSERT(m_motion_data[m1].m_i3m_root); /* 378 */
    ROR_ASSERT(m_motion_data[m2].m_i3m_root); /* 379 */
    ROR_ASSERT(p); /* 380 */
    ROR_ASSERT(t1>=0.0f); /* 381 */
    ROR_ASSERT(t2>=0.0f); /* 382 */
    ROR_ASSERT(r >= 0.0f && r <= 1.0f); /* 383 */
    i3mGetBlendPose(m_motion_data[m1].m_i3m_root, p, m_motion_data[m2].m_i3m_root, inter, t1, t2, r);
}
