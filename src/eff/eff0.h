/* ==========================================================================
 *  eff/eff0.h
 *
 *  Eff0, the base of the 43 Eff* effect classes (vtable 0x2e3250, vptr at
 *  +0x20; code 0x16a914..0x16ac84, not written yet: this header declares
 *  what EffMan (eff/effman.cpp) calls through the table). Every class is
 *  `: Eff0`; EffMan::createEff news one per effect kind. The virtuals are
 *  declared in vtable slot order (GCC 2: 8-byte slots, slot 0 the type
 *  info); the names with a ROR symbol come from the overrides the ELF keeps
 *  (EffLight3::renderPreMain, EffBloodStay3::render, ...).
 *
 *  Flags (m_flags): 2 finished (the finish countdown ran out), 4 hidden
 *  (isActivate is its inverse), 8 activation pending (live runs
 *  liveActivateExec), 0x10 weak (isBeWeak: EffMan may evict it when its
 *  table is full).
 * ======================================================================== */
#ifndef _EFF_EFF0_H
#define _EFF_EFF0_H

#include "sdk/eetypes.h"

struct EffParam;
struct EffPack;
struct I3dLayer;
struct I3dDrawSet;

/* Eff0's flag bits. */
enum
{
    EFF_FLAG_FINISHED = 0x02,
    EFF_FLAG_HIDDEN   = 0x04,
    EFF_FLAG_ACTIVATE = 0x08,
    EFF_FLAG_WEAK     = 0x10,
};

/* EE 0x24. layout: host -- the vptr (EE +0x20) and the pack pointer. */
class Eff0
{
public:
    /* The layers EffMan::draw renders into (its stack; the ROM's symbols
     * name the type Eff0::RENDERPARAM). m_cur is the one a render draws
     * into: Eff0::render sets it to m_layer[0]. EE 0xc. layout: host --
     * pointers. */
    struct RENDERPARAM
    {
        I3dLayer *m_layer[2];   // ROR 0x00: depth-tested (key 850), always-pass (key 855)
        I3dLayer *m_cur;        // ROR 0x08
    };

    Eff0();                                                              // ROR 0x0016a914
    virtual ~Eff0();                                                     // ROR 0x0016a950 (slot 1)
    /* Slot 2: takes the param; 0 makes createEff delete the object. */
    virtual int  init(const EffParam *param);                            // ROR 0x0016a9ac
    virtual void term();                                                 // ROR 0x0016a9e0 (slot 3)
    /* Slot 4, once a frame (EffMan::live): 0 when the effect is over. */
    virtual int  live();                                                 // ROR 0x0016aa2c
    /* Slot 5 (pure in Eff0): the class's own step, live's result. */
    virtual int  liveMain() = 0;
    /* Slot 6 (EffMan::draw). */
    virtual void render(RENDERPARAM *param);                             // ROR 0x0016aacc
    virtual void renderPreMain(RENDERPARAM *param);                      // ROR 0x0016ab10 (slot 7)
    /* Slot 8 (pure in Eff0): the packet between BeginSet and EndSet. */
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt) = 0;
    virtual I3dDrawSet *createPacketEntry(I3dLayer *layer);              // ROR 0x0016abe4 (slot 9)
    /* Slot 10 of every Eff class (never overridden): clears / sets
     * EFF_FLAG_HIDDEN and sets EFF_FLAG_ACTIVATE. */
    virtual void setVisible(bool on);                                    // ROR 0x0016aa04
    virtual void liveActivateExec();                                     // ROR 0x0028adf8 (slot 11)
    /* Slot 12: m_z for EffMan::draw's sort (Eff0: nothing). */
    virtual void updateZDepth();                                         // ROR 0x0028ae18

    u_short  m_flags;           // ROR 0x00: EFF_FLAG_*
    int      m_life;            // ROR 0x04: live's finish countdown, -1 none (the constructor); EffMan::retireEff sets 0
    int      m_08;              // ROR 0x08
    int      m_0c;              // ROR 0x0c
    float    m_z;               // ROR 0x10: depth from the camera, EffMan::draw draws the largest first
    EffPack *m_pack;            // ROR 0x14: the param's pack (its m_ref counts the users)
    int      m_kind;            // ROR 0x18: EffParam::m_kind
    int      m_delay;           // ROR 0x1c: frames before it renders (EffParam +8)
};

struct DogCamera;

/* The depth of pos along cam's view axis: the camera's transform, then
 * dot(matrix row 2, pos - row 3). */
float effCalcViewDepth(DogCamera *cam, const float *pos);                // ROR 0x0016ff18
/* out = from + normalize(to - from) * d. */
void  effMoveToward(float d, float *out, const float *from, const float *to); // ROR 0x0016ff78

/* Clears the per-kind draw-set cache 0x8282a0[54] that renderPreMain fills
 * (EffMan::draw, once a frame). */
void effClearDrawSets();                                                 // ROR 0x0016add8
/* Fills the 256-byte table 0x828378 with a random permutation of 0..255
 * and clears its cursor 0x308050 (EffMan::init). */
void effInitRandTable();                                                 // ROR 0x0016ac84

#endif /* _EFF_EFF0_H */
