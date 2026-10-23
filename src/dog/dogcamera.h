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
    /* Inline in the ROM (written into MagScene's constructor, 0x1360a0):
     * a 60-degree lens, 115 at +0x144, the vectors (0, 0, 0, 1). */
    DogCamera()
        : m_130(0), m_134(-1), m_138(0), m_13c(0), m_angle(0x1.0c1524p+0f /* pi / 3 */),
          m_144(115.0f), m_180(0)
    {
        for (int i = 0; i < 4; i++)
        {
            float v = (i == 3) ? 1.0f : 0.0f;
            m_150[i] = v;
            m_160[i] = v;
            m_170[i] = v;
            m_190[i] = v;
            m_1a0[i] = v;
            m_1b0[i] = v;
            m_1c0[i] = v;
        }
    }

    /* The lens angle m_angle (radians) mapped through a 13-point curve in
     * 15-degree steps (0, 28, 57, 82 .. 172.75, 180 degrees); radians. */
    float getLensFov();                                                  // ROR 0x0010fac0

    u_char        _e8[0xf0 - 0xe8];
    sceVu0FMATRIX m_view_matrix;    // ROR 0xf0: what getMatrix returns
    int           m_130;            // ROR 0x130: 0 from the constructor; role unknown
    int           m_134;            // ROR 0x134: -1 from the constructor; role unknown
    int           m_138;            // ROR 0x138: role unknown
    int           m_13c;            // ROR 0x13c: role unknown
    float         m_angle;          // ROR 0x140: the lens angle; MagScene::draw rebuilds the frustum when it changes
    float         m_144;            // ROR 0x144: 115 from the constructor; role unknown
    u_char        _148[0x150 - 0x148];
    sceVu0FVECTOR m_150;            // ROR 0x150: (0, 0, 0, 1) from the constructor; role unknown
    sceVu0FVECTOR m_160;            // ROR 0x160: idem
    sceVu0FVECTOR m_170;            // ROR 0x170: idem
    int           m_180;            // ROR 0x180: 0 from the constructor; role unknown
    u_char        _184[0x190 - 0x184];
    sceVu0FVECTOR m_190;            // ROR 0x190: (0, 0, 0, 1) from the constructor; role unknown
    sceVu0FVECTOR m_1a0;            // ROR 0x1a0: idem
    sceVu0FVECTOR m_1b0;            // ROR 0x1b0: idem
    sceVu0FVECTOR m_1c0;            // ROR 0x1c0: idem
    u_char        _1d0[0x1e0 - 0x1d0];
};
static_assert(offsetof(DogCamera, m_134) >= 0x134, "m_134");
static_assert(offsetof(DogCamera, m_144) >= 0x144, "m_144");
static_assert(offsetof(DogCamera, m_180) >= 0x180, "m_180");
static_assert(offsetof(DogCamera, m_1c0) >= 0x1c0, "m_1c0");
static_assert(offsetof(DogCamera, m_view_matrix) >= 0xf0, "m_view_matrix");
static_assert(offsetof(DogCamera, m_angle) >= 0x140, "m_angle");

#endif /* _DOG_DOGCAMERA_H */
