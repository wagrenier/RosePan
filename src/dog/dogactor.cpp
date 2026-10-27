// dog/dogactor.cpp -- DogActor: a DogModel with motions.
//
// The motions live in records (m_motion_data, m_max_motion of them), filled
// from motion packs in up to four banks (m_pack_motion; bank 0 is the model
// pack's file 1, banks 1..3 loadPackMotion's heap blocks). A record points
// at its i3m, an optional root move track (x, y, z and the turn per frame;
// getRootMove) and a tag track. The posture block holds the pose records of
// the current and the previous motion and the map from motion nodes to model
// nodes. Written 2026-10-27: the load / release paths, the record lookups,
// the root track and the motion starts; the per-frame player (draw,
// getMotionMove, getMotionEvent, runMotionCmd) is still stubbed.

#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "common/rosepan_stub.h"
#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "bed/bedpfh.h"
#include "game/gamfile.h"
#include "i3d/i3ritem.h"
#include "i3d/i3rroot.h"
#include "utl/utlmisc.h"

// ──────────────────────────────────────────────────────────────────────
// The posture block and the record table.
// callcheck: FUN_00215a30, i3mGetNumNode (the assert handler; getMotionNumNode inline)
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

// callcheck: FUN_00215a30 (the assert handler)
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

// callcheck: FUN_00215a30 (the assert handler)
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
// callcheck: FUN_00215a30 (the assert handler)
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
// callcheck: FUN_00215a30, DogMotionBlend::getNo (the assert handler; getRootTrack inline, getNo called)
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
                utlVecRotY(turn, out);
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
    m_1d8 = 0;
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
    m_44c = -1;
    for (int i = 0; i < 8; i++)
        m_42c[i] = -1;
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

// callcheck: FUN_00215a30, DogMotionBlend::reset (the assert handler; reset called)
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
// callcheck: FUN_00215a30, DogMotionBlend::getNo (the assert handler; getBlendNo inline, getNo called)
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
// callcheck: FUN_00215a30, DogModel::freeMatrix (the assert handler; decRefCount inline)
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

// callcheck: FUN_00215a30 (the assert handler; decRefCount inline)
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
// callcheck: FUN_00215a30 (the assert handler)
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

// callcheck: FUN_00215a30, DogMotionBlend::getNo (the assert handler; getRootTrack inline, getNo called)
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
// callcheck: FUN_00215a30 (the assert handler)
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

// callcheck: FUN_00215a30, strcmp (the assert handler; strcmp called)
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
    if (m_1d8 < 1)
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
// callcheck: FUN_00215a30 (the assert handler; DogMotionBlend::getData inline)
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
    if (mode == 0 && m_1d8 <= 0)
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
    if ((char)m_motion.getData(n).m_10 != 0 && m_1d8 > 0) /* the header's line 122 */
        flags |= 0x40;
    m_motion_flags = (int)flags;
    m_motion_flags2 = flags2;
    startMotionMode();
    return n;
}

// callcheck: FUN_00215a30, DogMotionBlend::getNo, DogEntity::normalizeRotate (the assert handler; getNo, normalizeRotate called)
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
    m_1d8 = v;
}

// callcheck: FUN_00215a30, i3mGetNumNode, i3mGetNodeName, i3rGetNodeIndexByName (the assert handler; getMotionNumNode, getMotionNodeName and DogModel::getNodeIndexByName inline)
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
// callcheck: FUN_00215a30, BedPfh, FileReadReq, FileReader, ~FileReader (the assert handler; the pfh constructors; inline in game/gamfile.h: the two constructors and the destructor)
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
// callcheck: FUN_00215a30, BedPfh, i3mRelocate (the assert handler; the part pack constructor; relocateMotion inline)
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

// callcheck: FUN_00215a30, BedPfh::clear, BedPfh::BedPfh, BedPfh::getFileName, BedMemory::allocDebug, memcpy, DogEntity::reset, DogModel::allocMatrix (the assert handler; BedPfh's inlines; slot 1 reset and slot 14 loadPack called directly)
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
// callcheck: FUN_00215a30, i3mGetLength (the assert handler; i3mGetLength called)
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
        m_scale_node[i] = -1;
        m_scale[i] = 1.0f;
    }
}

// callcheck: FUN_00215a30 (the assert handler)
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
    m_3b0[3] = 1.0f;
    m_3b0[2] = 0.0f;
    m_3b0[1] = 0.0f;
    m_3b0[0] = 0.0f;
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

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00109348
int DogActor::getMotionMove(float t0, float t1, float *move, float *rot, float *pos)
{
    ROSEPAN_STUB("DogActor::getMotionMove", 0x00109348);
    return {};
}

// stub of ROR 0x0010eab8
void DogActor::getNodeWorldMatrix(sceVu0FMATRIX out, int n)
{
    ROSEPAN_STUB("DogActor::getNodeWorldMatrix", 0x0010eab8);
}

// stub of ROR 0x0010f2dc
int DogActor::checkName(const char *theName)
{
    ROSEPAN_STUB("DogActor::checkName", 0x0010f2dc);
    return {};
}

// stub of ROR 0x0010cffc
int DogActor::shareModel(DogActor *src)
{
    ROSEPAN_STUB("DogActor::shareModel", 0x0010cffc);
    return {};
}

// stub of ROR 0x0010e5c0
void DogActor::draw(I3dLayer *layer, const float *cam_rotate, const float *cam_trans)
{
    ROSEPAN_STUB("DogActor::draw", 0x0010e5c0);
}

// stub of ROR 0x0010e3ac
void DogActor::drawModel(I3dLayer *layer, int nodes, DogEntity *src)
{
    ROSEPAN_STUB("DogActor::drawModel", 0x0010e3ac);
}

// stub of ROR 0x0010eb28
void DogActor::getNodeWorldPos(float *out, int n)
{
    ROSEPAN_STUB("DogActor::getNodeWorldPos", 0x0010eb28);
}

// stub of ROR 0x0010e534
void DogActor::drawShadow(I3dLayer *layer)
{
    ROSEPAN_STUB("DogActor::drawShadow", 0x0010e534);
}

// stub of ROR 0x0010bcc8
u_int DogActor::getMotionEvent(float t0, float t1, int motion)
{
    ROSEPAN_STUB("DogActor::getMotionEvent", 0x0010bcc8);
    return {};
}

// stub of ROR 0x0010d388
void DogActor::runMotionCmd()
{
    ROSEPAN_STUB("DogActor::runMotionCmd", 0x0010d388);
}

// stub of ROR 0x0010f5e4
void DogActor::setEyeTex(int state)
{
    ROSEPAN_STUB("DogActor::setEyeTex", 0x0010f5e4);
}

// stub of ROR 0x0010eba0
void DogActor::setNodeScale(float scale, int node)
{
    ROSEPAN_STUB("DogActor::setNodeScale", 0x0010eba0);
}
