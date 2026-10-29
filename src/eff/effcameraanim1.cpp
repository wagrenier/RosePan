/* ==========================================================================
 *  eff/effcameraanim1.cpp
 *
 *  EffCameraAnim1 (0x16f71c..0x16fb70): the camera animation of an event
 *  pack. See eff/effcameraanim1.h.
 * ======================================================================== */
#include "eff/effcameraanim1.h"

#include "dog/dogcamera.h"
#include "eff/effman.h"
#include "mag/magscene.h"

#define EFF_PI      3.1415925f  /* 0x40490fda: the wrap bounds, pi rounded down */
#define EFF_TWO_PI  6.283185f   /* 0x40c90fda */

// ──────────────────────────────────────────────────────────────────────
// A type-1 file only: saves the scene's camera mode, the camera's
// translation, rotation and lens angle, frees the camera and puts the
// first key on it. 0 for any other file (createEff deletes the object).
// ROR 0x0016f71c

int EffCameraAnim1::init(const EffParam *param)
{
    MagScene *scene = g_effScene;
    if (!Eff0::init(param))
        return 0;

    const EffCameraFile *file = (const EffCameraFile *)param->m_file;
    if (file->m_type != 1)
        return 0;

    DogCamera &cam = scene->m_camera_entity;
    m_saved_mode = g_effScene->getCameraMode(NULL);
    m_saved_trans[0] = cam.m_trans[0];
    m_saved_trans[1] = cam.m_trans[1];
    m_saved_trans[2] = cam.m_trans[2];
    m_saved_rotate[0] = cam.m_rotate[0];
    m_saved_rotate[1] = cam.m_rotate[1];
    m_saved_rotate[2] = cam.m_rotate[2];
    m_frame = 0;
    m_saved_angle = cam.m_angle;
    m_key = file->m_key;
    m_num = file->m_num;
    g_effScene->setCameraMode(0, 0, NULL);
    setKey(m_key);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Frees the camera if a mode holds it, then the key's translation, its
// rotation wrapped into -pi..pi and its angle in radians.
// ROR 0x0016f80c

void EffCameraAnim1::setKey(const EffCameraKey *key)
{
    if (g_effScene->getCameraMode(NULL) != 0)
        g_effScene->setCameraMode(0, 0, NULL);

    DogCamera &cam = g_effScene->m_camera_entity;
    float x = key->m_rotate[0];
    float y = key->m_rotate[1];
    float z = key->m_rotate[2];
    while (x < -EFF_PI) x += EFF_TWO_PI;
    while (x >= EFF_PI) x -= EFF_TWO_PI;
    while (y < -EFF_PI) y += EFF_TWO_PI;
    while (y >= EFF_PI) y -= EFF_TWO_PI;
    while (z < -EFF_PI) z += EFF_TWO_PI;
    while (z >= EFF_PI) z -= EFF_TWO_PI;

    cam.m_trans[0] = key->m_trans[0];
    cam.m_trans[1] = key->m_trans[1];
    cam.m_trans[2] = key->m_trans[2];
    cam.m_dirty = 1;
    cam.setRotate(x, y, z);
    cam.m_angle = (key->m_angle * 3.141593f) / 180.0f;
}

// ──────────────────────────────────────────────────────────────────────
// The next key each frame; after m_num frames the saved camera and mode
// come back (the mode's actor argument is not saved: 0) and it ends.
// ROR 0x0016faa0

int EffCameraAnim1::liveMain()
{
    m_frame++;
    if (m_frame >= m_num)
    {
        DogCamera &cam = g_effScene->m_camera_entity;
        cam.m_dirty = 1;
        cam.m_trans[0] = m_saved_trans[0];
        cam.m_trans[1] = m_saved_trans[1];
        cam.m_trans[2] = m_saved_trans[2];
        cam.setRotate(m_saved_rotate[0], m_saved_rotate[1], m_saved_rotate[2]);
        cam.m_angle = m_saved_angle;
        g_effScene->setCameraMode(m_saved_mode, 0, NULL);
        return 0;
    }
    m_key++;
    setKey(m_key);
    return 1;
}
