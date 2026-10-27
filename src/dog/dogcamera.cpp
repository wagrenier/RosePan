// dog/dogcamera.cpp -- the camera (0x10f860..0x10fc7c).
//
// DogCamera is the scene's eye (MagScene +0x2ab70): a DogEntityEx whose
// world matrix places the camera and whose inverse, kept at +0xf0, is the
// view matrix the scene draws with (getMatrix). Around it: a one-level
// save / restore of the pose and lens (the life menu's camera), a timed
// move that lerps translation and rotation from a start to an end pose,
// the lens angle mapped to the field of view, and the heading the pad
// steers by.

#include "dog/dogcamera.h"

#include <math.h>

#include "common/ror_assert.h"
#include "i3d/i3dmath.h"
#include "lib/unassigned.h"

static void copyVector(sceVu0FVECTOR dst, const sceVu0FVECTOR src)
{
    for (int i = 0; i < 4; i++)
        dst[i] = src[i];
}

// ROR 0x0010f860
// callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
void DogCamera::saveCamera()
{
    ROR_ASSERT(m_saved==0);  /* 22 */
    m_saved++;
    m_saved_angle = m_angle;
    copyVector(m_saved_trans, m_trans);
    copyVector(m_saved_rotate, m_rotate);
    copyVector(m_saved_scale, m_scale);
}

// ROR 0x0010f8cc
// callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
void DogCamera::restoreCamera()
{
    ROR_ASSERT(m_saved != 0);  /* 36 */
    m_saved--;
    m_angle = m_saved_angle;
    copyVector(m_trans, m_saved_trans);
    copyVector(m_rotate, m_saved_rotate);
    copyVector(m_scale, m_saved_scale);
    m_dirty = 1;
}

// ROR 0x0010f944
void DogCamera::updateMove()
{
    if (m_move_frames <= 0)
        return;

    if (m_move_frame < m_move_frames)
    {
        float t = (float)m_move_frame / (float)m_move_frames;
        sceVu0FVECTOR v = { 0.0f, 0.0f, 0.0f, 1.0f };

        utlLerp4(t, v, m_move_trans, m_move_trans_end);
        m_dirty = 1;
        copyVector(m_trans, v);
        utlLerp4(t, v, m_move_rot, m_move_rot_end);
        copyVector(m_rotate, v);
        normalizeRotate();
        m_dirty = 1;

        m_move_frame++;
        if (m_move_frame < m_move_frames)
            return;
    }
    m_move_frames = 0;
    m_move_frame = 0;
}

// ROR 0x0010fa6c
// callcheck: DogEntity::callTransform (EE slot 2 through the vptr)
sceVu0FVECTOR *DogEntity::getMatrix()
{
    // The ROM writes this + 0xf0, past DogEntityEx: every caller passes a
    // camera (MagScene's +0x2ab70, the movie layer's), so the host names
    // the class that holds the matrix.
    DogCamera *cam = static_cast<DogCamera *>(this);

    callTransform();
    i3dMatInverse(cam->m_view_matrix, m_matrix);
    return cam->m_view_matrix;
}

/* rodata (copied to the stack): the field of view in degrees for each
 * 15 degrees of lens angle. PC: three more 180s -- at exactly 180 degrees
 * the ROM reads the entry past the table (uninitialised stack) times a
 * zero fraction. */
static const float s_lens_fov[16] = {
    0.0f, 28.0f, 57.0f, 82.0f, 99.5f, 114.5f, 128.0f, 139.0f,
    148.0f, 156.5f, 164.75f, 172.75f, 180.0f, 180.0f, 180.0f, 180.0f,
};

// ROR 0x0010fac0
float DogCamera::getLensFov()
{
    const float pi = 0x1.921fb8p+1f;  // 0x40490fdc, the ROM's pi
    float step;
    float frac = modff(m_angle * 180.0f / pi / 15.0f, &step);
    int i = (int)step;

    return (s_lens_fov[i] + (s_lens_fov[i + 1] - s_lens_fov[i]) * frac) * pi / 180.0f;
}

// ROR 0x0010fbd4
float DogCamera::getLensScale()
{
    return m_angle / m_144 / 0x1.cd94dep-1f;  // 0x3f66ca6f, 0.901526
}

// ROR 0x0010fc08
void DogCamera::setLensScale(float scale)
{
    m_angle = scale * m_144 * 0x1.cd94dep-1f;
}

// ROR 0x0010fc28
// callcheck: DogEntity::callTransform (EE slot 2 through the vptr)
float DogCamera::getRotY()
{
    sceVu0FVECTOR z;

    callTransform();
    copyVector(z, m_matrix[2]);
    z[1] = 0.0f;
    utlNormalize3(z);
    return utlVecToRotY(z);
}
