/* ==========================================================================
 *  dog/dogentity.cpp
 *
 *  DogEntity and DogEntityEx (ROR 0x10fc7c..0x1103dc, linkonce
 *  0x280e88..0x280f68 and 0x281b78..0x281cbc): the transform node's
 *  setters, the local matrix (scale, rotate z x y, translate), the parent
 *  chain, and the slot-2 dispatch the host keeps on the EE vptr word.
 *
 *  Not written: the GCC 2 type_info functions DogEntity::__tf 0x280e88 /
 *  DogEntityEx::__tf 0x281b78 (no RTTI is read on the host), the
 *  destructors 0x280f34 / 0x281c80 (they only restore the vptr), and the
 *  constructor 0x281bc8 / getParent 0x281cb4 (inline in dogentity.h).
 * ======================================================================== */

#include "dog/dogentity.h"

#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "dog/dogmodel.h"
#include "i3d/i3dmath.h"
#include "lib/unassigned.h"

// ──────────────────────────────────────────────────────────────────────
// PC: EE slot 2 called through the vptr. The ROM loads the vtable at +0x94
// and calls entry +0x14 with this + the entry's offset; the host reads the
// class from the same word (the constructors store their EE vtable).

void DogEntity::callTransform()
{
    switch (_ee_vptr)
    {
    case DOG_VT_ENTITY:
        transform();
        break;
    case DOG_VT_MODEL:
    case DOG_VT_ACTOR:
        static_cast<DogModel *>(this)->transform();
        break;
    case DOG_VT_ENTITY_EX:
    case DOG_VT_CAMERA:
    default:
        static_cast<DogEntityEx *>(this)->transform();
        break;
    }
}

// ROR 0x0010fc7c

void DogEntity::setRotate(float x, float y, float z)
{
    m_rotate[0] = x;
    m_rotate[1] = y;
    m_rotate[2] = z;
    normalizeRotate();
    m_dirty = 1;
}

// ──────────────────────────────────────────────────────────────────────
// Written with dog/dogactor.cpp (2026-10-27): DogActor::runMotionCmd's
// blend-in turn.
// ROR 0x0010fcb4

void DogEntity::setAddRotY(float r)
{
    m_80[1] = r;
    if (r < -DOG_PI)
        m_80[1] = r + DOG_PI2;
    if (m_80[1] >= DOG_PI)
        m_80[1] -= DOG_PI2;
    m_dirty = 1;
}

/* An angle wrapped into [-pi, pi) (the loops makeLocalMatrix and
 * normalizeRotate repeat per axis). */
static float wrapAngle(float r)
{
    while (r >= DOG_PI)
        r -= DOG_PI2;
    while (r < -DOG_PI)
        r += DOG_PI2;
    return r;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: i3dMatRotZ, i3dMatRotX, i3dMatRotY (the angle goes in f12)
// ROR 0x0010fd18

int DogEntity::makeLocalMatrix(sceVu0FVECTOR *out)
{
    if (m_dirty == 0)
        return 0;
    i3dMatUnit(out);
    i3dMatScaleXYZ(out, out, m_scale);
    i3dMatRotZ(wrapAngle(m_rotate[2] + m_80[2]), out, out);
    i3dMatRotX(wrapAngle(m_rotate[0] + m_80[0]), out, out);
    i3dMatRotY(wrapAngle(m_rotate[1] + m_80[1]), out, out);
    i3dMatTranslate(out, out, m_trans);
    i3dMatTranslate(out, out, m_trans_ofs);
    m_dirty = 0;
    return 1;
}

// ROR 0x0010ff74

void DogEntity::normalizeRotate()
{
    m_rotate[0] = wrapAngle(m_rotate[0]);
    m_rotate[1] = wrapAngle(m_rotate[1]);
    m_rotate[2] = wrapAngle(m_rotate[2]);
}

// ROR 0x00110114

float *DogEntity::getMatrixZ(float *out)
{
    out[0] = m_matrix[2][0];
    out[1] = m_matrix[2][1];
    out[2] = m_matrix[2][2];
    out[3] = m_matrix[2][3];
    return out;
}

// ROR 0x00110144

void DogEntity::lookAt(const float *pos, u_int mode)
{
    sceVu0FVECTOR d;
    d[0] = pos[0] - m_trans[0];
    d[1] = pos[1] - m_trans[1];
    d[2] = pos[2] - m_trans[2];
    d[3] = 1.0f;
    if (mode == 0)
        utlVecToRot(d, m_rotate);
    else if (!(mode & 1))
        m_rotate[0] = utlVecToRotX(d);
    else
        m_rotate[1] = utlVecToRotY(d);
    m_dirty = 1;
}

// ROR 0x001101f0

void DogEntity::getMatrixPos(float *out)
{
    out[0] = m_matrix[3][0];
    out[1] = m_matrix[3][1];
    out[2] = m_matrix[3][2];
    out[3] = m_matrix[3][3];
}

// ROR 0x001101fc

void DogEntity::getMatrixRot(float *out)
{
    sceVu0FVECTOR z;
    z[0] = m_matrix[2][0];
    z[1] = m_matrix[2][1];
    z[2] = m_matrix[2][2];
    z[3] = m_matrix[2][3];
    utlVecToRot(z, out);
}

// ──────────────────────────────────────────────────────────────────────
// DogEntity::reset with the DogEntityEx members; slot 1 of every class
// below DogEntityEx.
// ROR 0x0011023c

void DogEntityEx::reset()
{
    DogEntity::reset();
    i3dMatUnit(m_local_matrix);
    m_parent = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: dbgAssertFailed (the assert handler)
// ROR 0x001102b4

void DogEntityEx::setParent(DogEntityEx *theParent)
{
    m_parent = theParent;
    if (theParent != NULL)
    {
        for (DogEntityEx *p = theParent->m_parent; p != NULL; p = p->m_parent)
            ROR_ASSERT(p != m_parent); /* 302 */
    }
    m_trans_ofs[0] = 0.0f;
    m_trans_ofs[1] = 0.0f;
    m_trans_ofs[2] = 0.0f;
    m_trans_ofs[3] = 1.0f;
    m_dirty = 1;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: DogEntity::callTransform (the parent's slot 2 through its vptr)
// ROR 0x0011034c

void DogEntityEx::transform()
{
    makeLocalMatrix(m_local_matrix);
    if (m_parent != NULL)
    {
        m_parent->callTransform();
        i3dMatMul(m_matrix, m_parent->m_matrix, m_local_matrix);
        return;
    }
    i3dMatCopy2(m_matrix, m_local_matrix);
}

// ──────────────────────────────────────────────────────────────────────
// Linkonce (inline in dogentity.h in the source; emitted at 0x280ec8).
// ROR 0x00280ec8

void DogEntity::reset()
{
    for (int i = 0; i < 4; i++)
    {
        float v = (i == 3) ? 1.0f : 0.0f;
        m_80[i] = v;
        m_rotate[i] = v;
        m_trans_ofs[i] = v;
        m_trans[i] = v;
        m_scale[i] = 1.0f;
    }
    m_dirty = 1;
}

// ROR 0x00280f18

void DogEntity::transform()
{
    makeLocalMatrix(m_matrix);
}
