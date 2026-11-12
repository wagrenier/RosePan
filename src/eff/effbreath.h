/* ==========================================================================
 *  eff/effbreath.h
 *
 *  The breath effects, code 0x16dcc8..0x16f71c (after eff/effblood.cpp,
 *  before eff/effcameraanim1.cpp; no source string names the file --
 *  named after the classes, in the alphabetical Eff* link order):
 *  - EffBreath1Space (kind 9): a box. Each frame one group of 8 of the
 *    scene's 128 actors (16 groups in turn) is checked: an actor loaded,
 *    with a "Bip01 Head" node and its m_trans inside the box, gets one
 *    EffBreath1 (its number kept per actor); one whose model is gone has
 *    its EffBreath1 released. It never ends by itself.
 *  - EffBreath1 (kind 8): the actor's breath. Every frame a point at an
 *    offset from its head node (characters 0x24xx: (8, 25, 0), the rest
 *    (0, 14, 0)) goes on a ring of 8 (a small step keeps the last y). Every
 *    70 frames, while the actor stays in the box, a puff starts: alpha up
 *    0.06 a frame to 1, then down 0.015; its size from 8 grows faster each
 *    frame. Drawn as up to 8 camera-facing sprites 10 apart back along the
 *    ring, each risen more, bigger and fainter, the farthest first.
 *  - EffBreath2 (kind 0xa; ClrOut*): a stream along the caller's direction,
 *    spread up to 15 degrees / m_scale on x and z: an EffBreath2Prim on 5
 *    of every 8 frames, alternating texture halves, for m_frames frames.
 *  - EffBreath2Prim (kind 0xb): a camera-facing puff moving 7.5 * m_scale
 *    along its direction, growing 2.5 a frame from 5, slowed and pulled
 *    down by m_drag; over once its y passes 0 (the floor; y grows down).
 *  - EffBreath2Ground (kind 0xc; ClrOut*): four puffs lying flat on the
 *    floor around a point, starting at frames 0, 6, 8, 12, growing in over
 *    2 frames and fading out over the last 15; at least 17 frames.
 * ======================================================================== */
#ifndef _EFF_EFFBREATH_H
#define _EFF_EFFBREATH_H

#include "eff/eff0.h"
#include "sdk/libvu0.h"

struct DogActor;

/* EffBreath1's ring of mouth positions. */
#define EFF_BREATH1_POINTS  8

/* MagScene::m_actors' length (EffBreath1Space keeps an effect per actor). */
#define EFF_BREATH1_ACTORS  128

/* One ring entry: the position and its distance from the entry before. */
struct EffBreath1Point
{
    sceVu0FVECTOR m_pos;        // ROR 0x00
    float         m_len;        // ROR 0x10
    u_char        _14[0x20 - 0x14];
};
static_assert(sizeof(EffBreath1Point) == 0x20, "EffBreath1Point");

/* kind 0x8; EE 0x1a0, vtable 0x2e3720. layout: host. */
class EffBreath1 : public Eff0
{
public:
    EffBreath1() {}                                                         // ROR 0x0028b4c8 (linkonce; inlined in createEff)
    virtual ~EffBreath1() {}                                                // ROR 0x0028b418 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016dcc8
    virtual int  liveMain();                                                // ROR 0x0016de40
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016df38
    virtual void updateZDepth();                                            // ROR 0x0016e3ec

    /* The mouth's world position onto the ring. */
    void addPoint();                                                        // ROR 0x0016e438
    /* The actor's m_trans inside [m_min, m_max]. */
    int  isActorInBox();                                                    // ROR 0x0016e65c

    float         m_min[3];     // ROR 0x24: the box (EffBreath1Space's)
    float         m_max[3];     // ROR 0x30
    float         m_scale;      // ROR 0x3c: the sprites' size scale
    float         m_color[4];   // ROR 0x40: 0..1, alpha last
    DogActor     *m_actor;      // ROR 0x50
    int           m_node;       // ROR 0x54: its "Bip01 Head"
    u_char        _58[0x60 - 0x58];
    sceVu0FVECTOR m_mouth;      // ROR 0x60: the mouth in the head node's space, w 1
    int           m_frame;      // ROR 0x70: 0..69, a puff starts at 0 (init 62)
    u_char        _74[0x80 - 0x74];
    EffBreath1Point m_point[EFF_BREATH1_POINTS]; // ROR 0x80
    int           m_head;       // ROR 0x180: the newest m_point
    float         m_alpha;      // ROR 0x184: drawn while above 0
    float         m_alpha_step; // ROR 0x188: 0.06 in, -0.015 out
    float         m_size;       // ROR 0x18c
    float         m_grow;       // ROR 0x190: m_size's step, 0.015 more each frame
    u_char        _194[0x1a0 - 0x194];
};

/* kind 0x9; EE 0x254, vtable 0x2e3790. layout: host. */
class EffBreath1Space : public Eff0
{
public:
    EffBreath1Space() {}                                                    // ROR 0x0028b3d0 (linkonce; inlined in createEff)
    virtual ~EffBreath1Space() {}                                           // ROR 0x0028b320 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016e6e4
    virtual int  liveMain();                                                // ROR 0x0016e7cc
    virtual void render(RENDERPARAM *) {}                                   // ROR 0x0028b408 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *, const I3dDrawSet *, u_long *) { return NULL; } // ROR 0x0028b410 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *) { return NULL; }      // ROR 0x0028b400 (linkonce)

    /* A kind-8 EffBreath1 for actor no. */
    void createBreath(int no);                                              // ROR 0x0016e92c
    /* The actor has a "Bip01 Head" and its m_trans is inside the box. */
    int  isActorInBox(DogActor *actor);                                     // ROR 0x0016e9f4

    float m_min[3];             // ROR 0x24: the box
    float m_max[3];             // ROR 0x30
    float m_scale;              // ROR 0x3c: } EffBreath1's m_scale, m_color
    float m_color[4];           // ROR 0x40: }
    int   m_eff[EFF_BREATH1_ACTORS]; // ROR 0x50: each actor's EffBreath1 number, -1 none
    u_int m_frame;              // ROR 0x250: its low 4 bits pick the group of 8 actors
};

/* kind 0xa; EE 0x54, vtable 0x2e36b0. layout: host. */
class EffBreath2 : public Eff0
{
public:
    EffBreath2() {}                                                         // ROR 0x0028b5a8 (linkonce; inlined in createEff)
    virtual ~EffBreath2() {}                                                // ROR 0x0028b4f8 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016eb00
    virtual int  liveMain();                                                // ROR 0x0016eb98
    virtual void render(RENDERPARAM *) {}                                   // ROR 0x0028b5d8 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *, const I3dDrawSet *, u_long *) { return NULL; } // ROR 0x0028b5e0 (linkonce)

    const float *m_pos;         // ROR 0x24: the caller's position
    const float *m_dir;         // ROR 0x28: the caller's direction
    float m_scale;              // ROR 0x2c
    float m_drag;               // ROR 0x30
    float m_34;                 // ROR 0x34: handed on to EffBreath2Prim's m_50
    float m_color[4];           // ROR 0x38
    u_int m_frame;              // ROR 0x48
    u_int m_frames;             // ROR 0x4c
    int   m_rand;               // ROR 0x50: effRand() at init; unread
};

/* kind 0xb; EE 0x80, vtable 0x2e3640. layout: host. */
class EffBreath2Prim : public Eff0
{
public:
    EffBreath2Prim() {}                                                     // ROR 0x0028b698 (linkonce; inlined in createEff)
    virtual ~EffBreath2Prim() {}                                            // ROR 0x0028b5e8 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016edc0
    virtual int  liveMain();                                                // ROR 0x0016eeb4
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016ef74
    virtual void updateZDepth();                                            // ROR 0x0016f160

    u_char        _24[0x30 - 0x24];
    sceVu0FVECTOR m_pos;        // ROR 0x30: w 1
    sceVu0FVECTOR m_vel;        // ROR 0x40: w 0
    float         m_50;         // ROR 0x50: EffBreath2's m_34; unread
    float         m_size;       // ROR 0x54: 5 more at init (PC: from 0, see init)
    float         m_drag;       // ROR 0x58
    u_char        _5c[0x60 - 0x5c];
    float         m_color[4];   // ROR 0x60
    int           m_half;       // ROR 0x70: the texture half
    u_char        _74[0x80 - 0x74];
};

/* One of EffBreath2Ground's puffs: its offset on the floor and y angle. */
struct EffBreath2Puff
{
    float  m_x;                 // ROR 0x00
    float  m_z;                 // ROR 0x04
    float  m_rot;               // ROR 0x08
    u_char _0c[0x10 - 0x0c];
};
static_assert(sizeof(EffBreath2Puff) == 0x10, "EffBreath2Puff");

/* renderMain's per-puff block for renderPuff (an EE stack frame; it holds
 * pointers). layout: host. */
struct EffBreath2GroundDraw
{
    const EffBreath2Puff *m_puff; // ROR 0x00
    float        m_depth;       // ROR 0x04: the centre's view depth, for the fog
    float        m_scale;       // ROR 0x08: the puff's size factor (0x2f2278)
    float        m_alpha;       // ROR 0x0c: its alpha factor (0x2f2288)
    int          m_frame;       // ROR 0x10: the frame for the fades
    Eff0::RENDERPARAM *m_param; // ROR 0x14: unread
    u_long      *m_pkt;         // ROR 0x18
};

/* kind 0xc; EE 0xb0, vtable 0x2e35d0. layout: host. */
class EffBreath2Ground : public Eff0
{
public:
    EffBreath2Ground() {}                                                   // ROR 0x0028b778 (linkonce; inlined in createEff)
    virtual ~EffBreath2Ground() {}                                          // ROR 0x0028b6c8 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016f1a0
    virtual int  liveMain();                                                // ROR 0x0016f380
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016f3c8

    /* One puff lying flat; returns the packet pointer (also left in d). */
    u_long *renderPuff(EffBreath2GroundDraw *d);                            // ROR 0x0016f55c

    EffBreath2Puff m_puff[4];   // ROR 0x24: the first at the centre
    u_char         _64[0x70 - 0x64];
    sceVu0FVECTOR  m_pos;       // ROR 0x70: y 2 less than given, w 1
    float          m_size;      // ROR 0x80
    u_char         _84[0x90 - 0x84];
    float          m_color[4];  // ROR 0x90
    int            m_frame;     // ROR 0xa0
    int            m_frames;    // ROR 0xa4
    u_char         _a8[0xb0 - 0xa8];
};

#endif /* _EFF_EFFBREATH_H */
