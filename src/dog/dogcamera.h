/* ==========================================================================
 *  dog/dogcamera.h
 *
 *  DogCamera (vtable _vt$9DogCamera 0x2e1fb8, base DogEntityEx, vptr at
 *  +0x94; its constructor is inlined into MagScene's, 0x1360a0): the
 *  scene's camera at MagScene +0x2ab70 -- a transform node whose view
 *  matrix (+0xf0) DogEntity::getMatrix returns, and a lens angle the scene
 *  turns into its view frustum. EE size at most 0x1e0 (the scene's next
 *  member is at +0x2ad50). dog/dogcamera.cpp.
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
        : m_130(0), m_move_frame(-1), m_138(0), m_13c(0), m_angle(0x1.0c1524p+0f /* pi / 3 */),
          m_144(115.0f), m_saved(0)
    {
        _ee_vptr = DOG_VT_CAMERA;
        for (int i = 0; i < 4; i++)
        {
            float v = (i == 3) ? 1.0f : 0.0f;
            m_saved_trans[i] = v;
            m_saved_rotate[i] = v;
            m_saved_scale[i] = v;
            m_move_trans_end[i] = v;
            m_move_rot_end[i] = v;
            m_move_trans[i] = v;
            m_move_rot[i] = v;
        }
    }

    /* Pushes the lens angle and the translation, rotation and scale to
     * the m_saved_* copies (one level: asserts m_saved==0). */
    void  saveCamera();                                                  // ROR 0x0010f860
    /* Puts back what saveCamera kept (asserts m_saved != 0); dirty. */
    void  restoreCamera();                                               // ROR 0x0010f8cc
    /* One frame of a timed move: while m_move_frame < m_move_frames, the
     * translation and rotation are the lerp of the m_move_* start and end
     * by m_move_frame / m_move_frames; the move ends (both 0) after the
     * last frame. Nothing when m_move_frames <= 0. */
    void  updateMove();                                                  // ROR 0x0010f944
    /* The lens angle m_angle (radians) mapped through a 13-point curve in
     * 15-degree steps (0, 28, 57, 82 .. 172.75, 180 degrees); radians. */
    float getLensFov();                                                  // ROR 0x0010fac0
    /* m_angle / m_144 / 0.901526: the inverse of setLensScale. */
    float getLensScale();                                                // ROR 0x0010fbd4
    /* m_angle as scale * m_144 * 0.901526 (native 016 cases 5 / 6). */
    void  setLensScale(float scale);                                     // ROR 0x0010fc08
    /* Transforms, then the y rotation of the world matrix's z axis laid
     * flat (y = 0): the heading the camera looks along. */
    float getRotY();                                                     // ROR 0x0010fc28

    u_char        _e8[0xf0 - 0xe8];
    sceVu0FMATRIX m_view_matrix;    // ROR 0xf0: what getMatrix returns (the inverse of m_matrix)
    int           m_130;            // ROR 0x130: 0 from the constructor; role unknown
    int           m_move_frame;     // ROR 0x134: the frame of the move updateMove is on (-1 from the constructor)
    int           m_138;            // ROR 0x138: role unknown
    int           m_13c;            // ROR 0x13c: role unknown
    float         m_angle;          // ROR 0x140: the lens angle; MagScene::draw rebuilds the frustum when it changes
    float         m_144;            // ROR 0x144: 115 from the constructor; the lens scale's unit
    float         m_saved_angle;    // ROR 0x148: saveCamera's m_angle
    u_char        _14c[0x150 - 0x14c];
    sceVu0FVECTOR m_saved_trans;    // ROR 0x150: saveCamera's m_trans ((0, 0, 0, 1) from the constructor)
    sceVu0FVECTOR m_saved_rotate;   // ROR 0x160: saveCamera's m_rotate
    sceVu0FVECTOR m_saved_scale;    // ROR 0x170: saveCamera's m_scale
    int           m_saved;          // ROR 0x180: 1 while a saveCamera is pending (asserts m_saved==0 / != 0)
    u_char        _184[0x190 - 0x184];
    sceVu0FVECTOR m_move_trans_end; // ROR 0x190: updateMove's translation at the last frame
    sceVu0FVECTOR m_move_rot_end;   // ROR 0x1a0: its rotation at the last frame
    sceVu0FVECTOR m_move_trans;     // ROR 0x1b0: its translation at frame 0
    sceVu0FVECTOR m_move_rot;       // ROR 0x1c0: its rotation at frame 0
    int           m_move_frames;    // ROR 0x1d0: the move's length in frames; 0 = no move (not set by the constructor)
    u_char        _1d4[0x1e0 - 0x1d4];
};
static_assert(offsetof(DogCamera, m_view_matrix) >= 0xf0, "m_view_matrix");
static_assert(offsetof(DogCamera, m_move_frame) >= 0x134, "m_move_frame");
static_assert(offsetof(DogCamera, m_angle) >= 0x140, "m_angle");
static_assert(offsetof(DogCamera, m_144) >= 0x144, "m_144");
static_assert(offsetof(DogCamera, m_saved_angle) >= 0x148, "m_saved_angle");
static_assert(offsetof(DogCamera, m_saved_trans) >= 0x150, "m_saved_trans");
static_assert(offsetof(DogCamera, m_saved_rotate) >= 0x160, "m_saved_rotate");
static_assert(offsetof(DogCamera, m_saved_scale) >= 0x170, "m_saved_scale");
static_assert(offsetof(DogCamera, m_saved) >= 0x180, "m_saved");
static_assert(offsetof(DogCamera, m_move_trans_end) >= 0x190, "m_move_trans_end");
static_assert(offsetof(DogCamera, m_move_rot_end) >= 0x1a0, "m_move_rot_end");
static_assert(offsetof(DogCamera, m_move_trans) >= 0x1b0, "m_move_trans");
static_assert(offsetof(DogCamera, m_move_rot) >= 0x1c0, "m_move_rot");
static_assert(offsetof(DogCamera, m_move_frames) >= 0x1d0, "m_move_frames");

#endif /* _DOG_DOGCAMERA_H */
