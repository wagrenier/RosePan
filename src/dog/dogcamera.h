/* ==========================================================================
 *  dog/dogcamera.h
 *
 *  DogCamera (vtable _vt$9DogCamera 0x2e1fb8, base DogEntityEx, vptr at
 *  +0x94; its constructor is inlined into MagScene's, 0x1360a0): the
 *  scene's camera at MagScene +0x2ab70 -- a transform node whose view
 *  matrix (+0xf0) DogEntity::getMatrix returns, and a lens angle the scene
 *  turns into its view frustum. EE size at most 0x1e0 (the scene's next
 *  member is at +0x2ad50). Minimal: what MagScene's draw touches.
 *
 *  EE virtual slots: 1 reset 0x11023c, 2 transform 0x11034c, 3 ~DogCamera
 *  0x2854a0, 4 getParent 0x281cb4 -- all DogEntityEx's but the destructor.
 *  PORT: not polymorphic on the host yet (dogentity.h).
 * ======================================================================== */
#ifndef _DOG_DOGCAMERA_H
#define _DOG_DOGCAMERA_H

#include <stddef.h>

#include "dog/dogentity.h"

/* layout: host -- a DogEntityEx (pointers); EE offsets in the comments. */
struct DogCamera : DogEntityEx
{
    /* The lens angle m_angle (radians) mapped through a 13-point curve in
     * 15-degree steps (0, 28, 57, 82 .. 172.75, 180 degrees); radians. */
    float getLensFov();                                                  // ROR 0x0010fac0

    u_char        _e8[0xf0 - 0xe8];
    sceVu0FMATRIX m_view_matrix;    // ROR 0xf0: what getMatrix returns
    u_char        _130[0x140 - 0x130];
    float         m_angle;          // ROR 0x140: the lens angle; MagScene::draw rebuilds the frustum when it changes
    u_char        _144[0x1e0 - 0x144];
};
static_assert(offsetof(DogCamera, m_view_matrix) >= 0xf0, "m_view_matrix");
static_assert(offsetof(DogCamera, m_angle) >= 0x140, "m_angle");

#endif /* _DOG_DOGCAMERA_H */
