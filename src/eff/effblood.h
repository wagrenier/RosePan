/* ==========================================================================
 *  eff/effblood.h
 *
 *  The blood effects, kinds 1..6, code 0x16b62c..0x16dcc8 (between EffBg1
 *  and EffBreath1; no source string names the file).
 *
 *  - EffBloodSpray1 (kind 1, scnDamageStart): one burst of 9 drops from a
 *    point, each along the param's direction turned by a table of small
 *    angles, slowing, falling, growing, fading; camera-facing sprites
 *    (texture 0 for drops 0..4, 1 for the rest) drawn far to near.
 *  - EffBloodSpray2 (kind 2, script op 15): a spray that follows a caller's
 *    position: a drop every second frame for m_frames frames, 16 at most
 *    alive, then it waits for the last one to fade.
 *  - EffBloodStay1 (kind 3, the enemies' deaths): 2..4 stains on the floor,
 *    each growing in (sin of 45..90 degrees over 20 frames) after a delay,
 *    for m_life frames, then fading out.
 *  - EffBloodStay2 (kind 4, script op 17): a trail of 31 segments behind a
 *    caller's position, a textured strip lying across up x direction.
 *  - EffBloodStay3 (kind 5): no picture; it lives while one of its two
 *    counters is above 0 (effSetBloodStay3 creates the one instance).
 *  - EffBloodStay3Prim (kind 6): one flat sprite on the floor, faded in
 *    over 15 frames and out over 30 after 135, gone after 165.
 *
 *  Eff0::m_0c is the classes' state and m_08 last frame's (copied at the
 *  top of liveMain): 0 starting, 10 running, 20 fading (EffBloodStay1),
 *  -1 finished (EffBloodStay2).
 * ======================================================================== */
#ifndef _EFF_EFFBLOOD_H
#define _EFF_EFFBLOOD_H

#include "eff/eff0.h"
#include "sdk/libvu0.h"

struct DogActor;

/* EffBloodSpray1's drop. EE 0x40. */
struct EffBloodSpray1Drop
{
    sceVu0FVECTOR m_pos;        // ROR 0x00: w 1
    sceVu0FVECTOR m_vel;        // ROR 0x10: the direction, w 0
    short m_alpha;              // ROR 0x20: 0x7f00 at the start, alive while above 0xff
    short m_fade;               // ROR 0x22: taken off m_alpha each frame after the 8th
    short m_fade_add;           // ROR 0x24: added to m_fade each frame
    short _26;
    float m_speed;              // ROR 0x28: m_vel's scale, times 0.975 a frame
    float m_fall;               // ROR 0x2c: added to m_vel's y each frame
    float m_size;               // ROR 0x30
    float m_grow;               // ROR 0x34: added to m_size each frame
    u_char _38[0x40 - 0x38];
};
static_assert(sizeof(EffBloodSpray1Drop) == 0x40, "EffBloodSpray1Drop is 0x40 bytes on the EE");
static_assert(offsetof(EffBloodSpray1Drop, m_alpha) == 0x20, "EffBloodSpray1Drop::m_alpha");
static_assert(offsetof(EffBloodSpray1Drop, m_speed) == 0x28, "EffBloodSpray1Drop::m_speed");
static_assert(offsetof(EffBloodSpray1Drop, m_grow) == 0x34, "EffBloodSpray1Drop::m_grow");

/* kind 0x1; EE 0x280, vtable 0x2e33a0. layout: host. */
class EffBloodSpray1 : public Eff0
{
public:
    EffBloodSpray1();                                                       // ROR 0x0016b62c
    virtual ~EffBloodSpray1() {}                                            // ROR 0x0028ae78 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016b668
    virtual int  liveMain();                                                // ROR 0x0016b978
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016ba7c
    virtual void updateZDepth();                                            // ROR 0x0016bdd4

    int                m_frame;     // ROR 0x24: frames lived
    u_char             _28[0x30 - 0x28];
    sceVu0FVECTOR      m_pos;       // ROR 0x30: w 1
    EffBloodSpray1Drop m_drop[9];   // ROR 0x40
};

/* EffBloodSpray2's drop. EE 0x50. */
struct EffBloodSpray2Drop
{
    sceVu0FVECTOR m_pos;        // ROR 0x00: w 1
    sceVu0FVECTOR m_vel;        // ROR 0x10: the direction, w 0
    float m_speed;              // ROR 0x20: m_vel's scale, times 0.95 a frame
    float m_fall;               // ROR 0x24: added to m_vel's y each frame
    float m_size;               // ROR 0x28
    float m_grow;               // ROR 0x2c: added to m_size each frame
    short m_tex;                // ROR 0x30: the pack texture, 0 / 1
    short m_age;                // ROR 0x32: frames lived; it fades from the 17th
    short m_alpha;              // ROR 0x34: 0x7f00 at the start, alive while above 0xff
    short m_fade;               // ROR 0x36: taken off m_alpha each frame
    short m_fade_add;           // ROR 0x38: added to m_fade each frame
    u_char _3a[0x50 - 0x3a];
};
static_assert(sizeof(EffBloodSpray2Drop) == 0x50, "EffBloodSpray2Drop is 0x50 bytes on the EE");
static_assert(offsetof(EffBloodSpray2Drop, m_speed) == 0x20, "EffBloodSpray2Drop::m_speed");
static_assert(offsetof(EffBloodSpray2Drop, m_tex) == 0x30, "EffBloodSpray2Drop::m_tex");
static_assert(offsetof(EffBloodSpray2Drop, m_fade_add) == 0x38, "EffBloodSpray2Drop::m_fade_add");

/* kind 0x2; EE 0x590, vtable 0x2e3330. layout: host -- the caller's pointers. */
class EffBloodSpray2 : public Eff0
{
public:
    EffBloodSpray2();                                                       // ROR 0x0016be14
    virtual ~EffBloodSpray2() {}                                            // ROR 0x0028af28 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016be70
    virtual int  liveMain();                                                // ROR 0x0016c258
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016c400
    virtual void updateZDepth();                                            // ROR 0x0016c748

    /* Drop m_next % 16 from *m_pos along *m_dir (down when it is shorter
     * than 0.9) turned by table 0x2f2020[m_next % 9]; the later in each
     * run of 7, the slower, smaller and quicker to fade. m_next++. */
    void emitDrop();                                                        // ROR 0x0016bedc

    u_int              m_count;     // ROR 0x24: frames sprayed
    u_int              m_frames;    // ROR 0x28: EffParam m_spray2.m_frames
    int                m_next;      // ROR 0x2c: the next drop's number (starts at effRand())
    const float       *m_pos;       // ROR 0x30: the caller's position
    const float       *m_dir;       // ROR 0x34: the caller's direction
    float              m_scale;     // ROR 0x38
    u_char             _3c[0x40 - 0x3c];
    EffBloodSpray2Drop m_drop[17];  // ROR 0x40: emitDrop fills only 0..15
};

/* EffBloodStay1's stain: its offset from the centre (world x / z) and an
 * unused angle. EE 0x10. */
struct EffBloodStay1Stain
{
    float m_x;                  // ROR 0x00
    float m_z;                  // ROR 0x04
    float m_rot;                // ROR 0x08: 0..2 pi; nothing draws with it (isActorOnStain reads it as z)
    float _0c;
};
static_assert(sizeof(EffBloodStay1Stain) == 0x10, "EffBloodStay1Stain is 0x10 bytes on the EE");

/* kind 0x3; EE 0xc0, vtable 0x2e3560. layout: host. */
class EffBloodStay1 : public Eff0
{
public:
    /* renderMain's block for drawStain (its stack). EE 0x28. layout: host
     * -- pointers. */
    struct DRAW
    {
        const EffBloodStay1Stain *m_stain;  // ROR 0x00
        float        m_depth;   // ROR 0x04: the centre's view depth (the fog)
        float        m_size;    // ROR 0x08: m_size times the stain's scale
        int          m_age;     // ROR 0x0c: frames since the stain's delay
        RENDERPARAM *m_param;   // ROR 0x10
        u_long      *m_pkt;     // ROR 0x14
    };

    EffBloodStay1() {}                                                      // ROR 0x0028b088 (linkonce; inlined in createEff)
    virtual ~EffBloodStay1() {}                                             // ROR 0x0028afd8 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016c7c0
    virtual int  liveMain();                                                // ROR 0x0016ca00
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016cac0

    /* One stain: the matrix scaled by sin(45..90 degrees as the age goes
     * 0..20) times the size, at the stain's offset 2 above the centre, alpha
     * twice m_alpha's share of the growth (at most 1); returns the packet. */
    u_long *drawStain(DRAW *draw);                                          // ROR 0x0016ccb8
    /* 1 when a toe or finger of actor lands in a stain this frame (the
     * actor near the centre first). Unreferenced. */
    int isActorOnStain(DogActor *actor);                                    // ROR 0x0016cebc

    EffBloodStay1Stain m_stain[4];  // ROR 0x24: stain 0 at the centre
    int                m_count;     // ROR 0x64: 2..4
    u_char             _68[0x70 - 0x68];
    sceVu0FMATRIX      m_matrix;    // ROR 0x70: z the param's direction, row 3 the centre (pos + z / 2)
    float              m_size;      // ROR 0xb0: EffParam m_hit.m_scale
    int                m_frame;     // ROR 0xb4: frames lived
    float              m_alpha;     // ROR 0xb8: 1, falls by 0.01 a frame in state 20
    int                m_life;      // ROR 0xbc: EffParam m_hit.m_2c (< 0 forever)
};

/* EffBloodStay2's segment end: the trail's two edges. EE 0x20. */
struct EffBloodStay2Edge
{
    sceVu0FVECTOR m_a;          // ROR 0x00: w 1
    sceVu0FVECTOR m_b;          // ROR 0x10: w 1
};
static_assert(sizeof(EffBloodStay2Edge) == 0x20, "EffBloodStay2Edge is 0x20 bytes on the EE");

/* kind 0x4; EE 0x4f0, vtable 0x2e34f0. layout: host -- the caller's pointers. */
class EffBloodStay2 : public Eff0
{
public:
    EffBloodStay2() {}                                                      // ROR 0x0028b168 (linkonce; inlined in createEff)
    virtual ~EffBloodStay2() {}                                             // ROR 0x0028b0b8 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016d01c
    virtual int  liveMain();                                                // ROR 0x0016d0d0
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016d4bc

    /* m_edge[m_count] = at +- normalize(up x dir) * m_width / 2. */
    void setEdge(const float *at, const float *up, const float *dir);       // ROR 0x0016d3e4

    u_char            _24[0x30 - 0x24];
    EffBloodStay2Edge m_edge[32];   // ROR 0x030: [m_count] follows the caller
    float             m_v[32];      // ROR 0x430: the texture's t at each edge (length / (3 m_width))
    const float      *m_pos;        // ROR 0x4b0: the caller's position
    const float      *m_up;         // ROR 0x4b4: the caller's up
    float             m_width;      // ROR 0x4b8
    float             m_step;       // ROR 0x4bc: the trail's length / 31, a segment at most
    float             m_len;        // ROR 0x4c0: the length to the caller
    float             m_len_done;   // ROR 0x4c4: the length to the last edge
    int               m_count;      // ROR 0x4c8: finished segments (0..31)
    u_char            _4cc[0x4d0 - 0x4cc];
    sceVu0FVECTOR     m_last;       // ROR 0x4d0: the last edge's centre, w 1
    sceVu0FVECTOR     m_dir;        // ROR 0x4e0: the direction there
};

/* kind 0x5; EE 0x2c, vtable 0x2e3480. layout: host. */
class EffBloodStay3 : public Eff0
{
public:
    EffBloodStay3() {}                                                      // ROR 0x0028b1e8 (linkonce; inlined in createEff)
    virtual ~EffBloodStay3();                                               // ROR 0x0016d804
    virtual int  init(const EffParam *param);                               // ROR 0x0016d870
    virtual int  liveMain();                                                // ROR 0x0016d8ac
    virtual void render(RENDERPARAM *param) {}                              // ROR 0x0028b230 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt) { return NULL; } // ROR 0x0028b238 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *layer) { return NULL; } // ROR 0x0028b218 (linkonce)

    void set(int n, int count) { m_count[n] = count; }                      // ROR 0x0028b220 (linkonce)

    int m_count[2];             // ROR 0x24: it lives while one is above 0
};

/* kind 0x6; EE 0x50, vtable 0x2e3410. layout: host. */
class EffBloodStay3Prim : public Eff0
{
public:
    EffBloodStay3Prim() {}                                                  // ROR 0x0028b2f0 (linkonce; inlined in createEff)
    virtual ~EffBloodStay3Prim() {}                                         // ROR 0x0028b240 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x0016d9a8
    virtual int  liveMain();                                                // ROR 0x0016da30
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x0016da44

    int           m_type;       // ROR 0x24: EffParam m_prim.m_type
    int           m_flip;       // ROR 0x28
    u_char        _2c[0x30 - 0x2c];
    sceVu0FVECTOR m_pos;        // ROR 0x30: w 1
    float         m_rot;        // ROR 0x40
    float         m_alpha;      // ROR 0x44
    int           m_frame;      // ROR 0x48: frames lived (165 at most)
};

/* The live EffBloodStay3's EffMan number, -1 none (its destructor clears it). */
extern int g_effBloodStay3;                                                 // ROR 0x00298e10
/* Sets the EffBloodStay3's counter n to count, creating it first when there
 * is none; 0 when it cannot be created. Unreferenced. */
int effSetBloodStay3(int n, int count);                                     // ROR 0x0016d8e8

#endif /* _EFF_EFFBLOOD_H */
