/* ==========================================================================
 *  eff/efffog3.h
 *
 *  EffFog3 (kind 0x1a) and its two one-frame helpers EffFog3Dark (0x1b)
 *  and EffFog3WeakUp (0x1c): the scene fog that thickens while enemies are
 *  about. GamMan::initStage2 creates one EffFog3 per stage
 *  (gamCreateEff1a); init picks the chapter's record (g_bss_7a3210) from
 *  its pack's table, and every 30 frames liveMain checks the enemies:
 *  one awake while the fog is clear creates EffFog3Dark (the record's fog,
 *  over 30 frames); all of them down while it is dark creates
 *  EffFog3WeakUp (150 frames later the fog goes back over 90 frames; the
 *  next check is 180 frames on). The fog moves by the difference between the record's fog and
 *  the map's (MagScene::m_fog[1]), so it is relative to whatever the
 *  scene's target fog is then.
 *
 *  Code 0x172678..0x172e4c, between EffFog2Prim and EffFoot1.
 * ======================================================================== */
#ifndef _EFF_EFFFOG3_H
#define _EFF_EFFFOG3_H

#include <stddef.h>

#include "eff/eff0.h"

/* One record of the pack's fog table (the file after its models, motions
 * and textures; ends with m_chapter -1). EE 0x20. */
struct EffFog3Rec
{
    int   m_chapter;            // ROR 0x00: g_bss_7a3210 it is for, -1 the last record
    float m_dist_scale;         // ROR 0x04: the map fog's near and far distances times this
    float m_amount0_t;          // ROR 0x08: the near amount moved this far toward the far amount
    int   _0c;
    float m_amount1_scale;      // ROR 0x10: the far amount times this
    int   m_rgb[3];             // ROR 0x14: the fog colour, 0..255
};
static_assert(offsetof(EffFog3Rec, m_dist_scale) == 0x04, "m_dist_scale");
static_assert(offsetof(EffFog3Rec, m_amount0_t) == 0x08, "m_amount0_t");
static_assert(offsetof(EffFog3Rec, m_amount1_scale) == 0x10, "m_amount1_scale");
static_assert(offsetof(EffFog3Rec, m_rgb) == 0x14, "m_rgb");
static_assert(sizeof(EffFog3Rec) == 0x20, "EffFog3Rec is 0x20 bytes on the EE");

/* g_effFog3Flags bits. */
enum
{
    EFF_FOG3_DARK = 0x01,       /* the record's fog is on (effFog3Shift(1, ..)) */
};

/* kind 0x1a; EE 0x2c, vtable 0x2e3b80. layout: host. */
class EffFog3 : public Eff0
{
public:
    EffFog3() {}                                                            // ROR 0x0028c020 (linkonce; inlined in createEff)
    /* Forgets the record (g_effFog3Rec NULL). */
    virtual ~EffFog3();                                                     // ROR 0x00172ac8
    virtual int  init(const EffParam *param);                               // ROR 0x00172b30
    virtual int  liveMain();                                                // ROR 0x00172cb8
    /* Nothing to draw. */
    virtual void render(RENDERPARAM *) {}                                   // ROR 0x0028c058 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *, const I3dDrawSet *, u_long *) { return NULL; } // ROR 0x0028c060 (linkonce)
    virtual I3dDrawSet *createPacketEntry(I3dLayer *) { return NULL; }      // ROR 0x0028c050 (linkonce)

    int m_frame;                // ROR 0x24: frames since the last check
    int m_interval;             // ROR 0x28: frames between checks (30, 180 after a WeakUp)
};

/* kind 0x1b; EE 0x24, vtable 0x2e3b10. layout: host. Turns the record's
 * fog on over 30 frames and ends. */
class EffFog3Dark : public Eff0
{
public:
    EffFog3Dark() {}                                                        // ROR 0x0028c118 (linkonce; inlined in createEff)
    virtual ~EffFog3Dark() {}                                               // ROR 0x0028c068 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00172d98
    virtual int  liveMain() { return 0; }                                   // ROR 0x0028c148 (linkonce)
    virtual void render(RENDERPARAM *) {}                                   // ROR 0x0028c150 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *, const I3dDrawSet *, u_long *) { return NULL; } // ROR 0x0028c158 (linkonce)
};

/* kind 0x1c; EE 0x24, vtable 0x2e3aa0. layout: host. EffFog3 creates it
 * with a delay of 150 frames (EffParam m_08, Eff0::live); then it turns the
 * record's fog off over 90 frames and ends. */
class EffFog3WeakUp : public Eff0
{
public:
    EffFog3WeakUp() {}                                                      // ROR 0x0028c210 (linkonce; inlined in createEff)
    virtual ~EffFog3WeakUp() {}                                             // ROR 0x0028c160 (linkonce)
    virtual int  init(const EffParam *param);                               // ROR 0x00172de8
    virtual int  liveMain();                                                // ROR 0x00172e08
    virtual void render(RENDERPARAM *) {}                                   // ROR 0x0028c240 (linkonce)
    virtual u_long *renderMain(RENDERPARAM *, const I3dDrawSet *, u_long *) { return NULL; } // ROR 0x0028c248 (linkonce)
};

/* The chapter's record (EffFog3::init; NULL after its destructor). */
extern const EffFog3Rec *g_effFog3Rec;   /* 0x298e18 */
/* EFF_FOG3_* */
extern int g_effFog3Flags;               /* 0x298e14 */

/* EffFog3 into the current scene's effects (GamMan::initStage2). */
void gamCreateEff1a(void);                                                  // ROR 0x00172678
/* 1 when an enemy in the actor list is not down (types 0..3 state 4,
 * 4 / 5 state 6; 6..10 never count, unknown types always). */
int  effFog3AnyEmyAwake(void);                                              // ROR 0x001726b4
/* 1 when every enemy of types 0..5 in the actor list is down. */
int  effFog3AllEmyDown(void);                                               // ROR 0x001727ec
/* Moves the scene's target fog by (record's fog - map fog), or back with
 * on 0, over frames (MagScene::addFog). */
void effFog3Shift(int on, int frames);                                      // ROR 0x00172934

#endif /* _EFF_EFFFOG3_H */
