/* ==========================================================================
 *  eff/effcameraanim1.h
 *
 *  EffCameraAnim1 (kind 0xd): an event pack's camera animation. It takes
 *  the scene camera off its mode, plays one key a frame from the effect
 *  file -- translation, rotation, lens angle -- and puts the camera and the
 *  mode back when the keys run out. Its code is eff/effcameraanim1.cpp
 *  (0x16f71c..0x16fb70), between EffBreath2Ground and the shared helpers.
 * ======================================================================== */
#ifndef _EFF_EFFCAMERAANIM1_H
#define _EFF_EFFCAMERAANIM1_H

#include <stddef.h>

#include "eff/eff0.h"

/* One key of the file: the camera at one frame. EE 0x1c. */
struct EffCameraKey
{
    float m_trans[3];           // ROR 0x00: the scene camera's m_trans
    float m_rotate[3];          // ROR 0x0c: its rotation in radians (any range: setKey wraps it)
    float m_angle;              // ROR 0x18: the lens angle in degrees
};

/* The effect file EffParam::m_file points at (scenePlayStart). EE 0x10 +
 * m_num keys. */
struct EffCameraFile
{
    int          m_type;        // ROR 0x00: init plays only type 1
    int          m_num;         // ROR 0x04: frames (keys) to play
    int          _08[2];
    EffCameraKey m_key[1];      // ROR 0x10: m_num of them
};

static_assert(sizeof(EffCameraKey) == 0x1c, "EffCameraKey");
static_assert(offsetof(EffCameraKey, m_angle) == 0x18, "m_angle");
static_assert(offsetof(EffCameraFile, m_num) == 0x04, "m_num");
static_assert(offsetof(EffCameraFile, m_key) == 0x10, "m_key");

/* kind 0xd; EE 0x50, vtable 0x2e3800. layout: host. */
class EffCameraAnim1 : public Eff0
{
public:
    EffCameraAnim1() {}                                                     // ROR 0x0028b858 (linkonce; inlined in createEff)
    virtual ~EffCameraAnim1() {}                                            // ROR 0x0028b7a8 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016f71c
    virtual int  liveMain();                                                // ROR 0x0016faa0
    /* Nothing to draw. */
    virtual void render(RENDERPARAM *) {}                                   // ROR 0x0028b890 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *, const I3dDrawSet *, u_long *) { return NULL; } // ROR 0x0028b898 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *) { return NULL; }      // ROR 0x0028b888 (linkonce)

    /* Puts key on the scene camera (frees its mode first). */
    void setKey(const EffCameraKey *key);                                   // ROR 0x0016f80c

    int                 m_frame;        // ROR 0x24: keys played after the first
    int                 m_num;          // ROR 0x28: EffCameraFile::m_num
    const EffCameraKey *m_key;          // ROR 0x2c: the key on the camera
    int                 m_saved_mode;   // ROR 0x30: the scene's camera mode before
    float               m_saved_trans[3];  // ROR 0x34: the camera's m_trans before
    float               m_saved_rotate[3]; // ROR 0x40: its m_rotate before
    float               m_saved_angle;  // ROR 0x4c: its m_angle before
};

#endif /* _EFF_EFFCAMERAANIM1_H */
