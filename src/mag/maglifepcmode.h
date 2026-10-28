/* ==========================================================================
 *  mag/maglifepcmode.h
 *
 *  The PC's modes (mag/maglifepcmode.cpp, 0x1208b8..): one small object
 *  per MagLifePC state, embedded in MagLifePC (+0x138..+0x1a8, vptrs set
 *  by its constructor 0x11bf0c). MagLifePC::selectMode points m_mode_cur
 *  at the one for m_state and runs its init; live runs its live each
 *  frame, processPad its pad.
 *
 *  Three EE virtual slots (MagLifePCMode has no data, so the vptr is at
 *  +0 of each object): 1 init(pc), 2 live(pc), 3 pad(pc, pad). The slot
 *  names come from the ror-symbols MagLifePCMode::init / ::pad (0x282c18 /
 *  0x282c20) and MagLifePCModeHold::live (0x282f44). No virtual
 *  destructor: the modes die with the PC.
 *
 *  State -> mode (MagLifePC::selectMode): 0 Null, 1 Wait, 2 Walk2D,
 *  3 Run2D, 4 Turn, 5 AtkWait2D, 6 AtkMove2D, 7 Roped, 8 Hold, 9 Ride,
 *  10 Attacking, 11 none (MagLifePC::beginAttack), 12 HoldAttack,
 *  13 Unhold, 14 Damage, 15 DogGo, 16 DogCome, 17 DogStay, 18 Hug,
 *  19 Item, 20 Reload.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEPCMODE_H
#define _MAG_MAGLIFEPCMODE_H

#include <stddef.h>

struct BedPad;
struct MagLifePC;

/* EE 4 bytes (the vptr). layout: host -- polymorphic. */
struct MagLifePCMode
{
    /* EE slot 1 (+0x08): MagLifePC::selectMode(1) runs it on entry. */
    virtual void init(MagLifePC *pc) {}                                  // ROR 0x00282c18
    /* EE slot 2 (+0x10): MagLifePC::live runs it each frame. */
    virtual void live(MagLifePC *pc);                                    // ROR 0x001208b8
    /* EE slot 3 (+0x18): MagLifePC::padMode; MagLifePC::processPad
     * repeats it while it returns more than 1. */
    virtual int  pad(MagLifePC *pc, BedPad *thePad) { return 0; }        // ROR 0x00282c20
};

/* The moving modes' base (no vtable of its own in the ROM). layout: host. */
struct MagLifePCModeMove : MagLifePCMode
{
};

/* layout: host (EE 4 bytes, at MagLifePC +0x138). */
struct MagLifePCModeNull : MagLifePCMode
{
    void init(MagLifePC *pc) override;                                   // ROR 0x00120900
    void live(MagLifePC *pc) override;                                   // ROR 0x001209b0
};

/* layout: host (EE 4 bytes, +0x13c). */
struct MagLifePCModeWait : MagLifePCMode
{
    void init(MagLifePC *pc) override;                                   // ROR 0x001209e0
    void live(MagLifePC *pc) override;                                   // ROR 0x00120b9c
    int  pad(MagLifePC *pc, BedPad *thePad) override;                    // ROR 0x00120bdc
};

/* layout: host (EE 4 bytes, +0x140). */
struct MagLifePCModeWalk2D : MagLifePCModeMove
{
    void init(MagLifePC *pc) override;                                   // ROR 0x00120f60
    void live(MagLifePC *pc) override;                                   // ROR 0x00121070
    int  pad(MagLifePC *pc, BedPad *thePad) override;                    // ROR 0x00121078
};

/* layout: host (EE 4 bytes, +0x144). */
struct MagLifePCModeRun2D : MagLifePCModeMove
{
    void init(MagLifePC *pc) override;                                   // ROR 0x00121168
    void live(MagLifePC *pc) override;                                   // ROR 0x001211c8
    int  pad(MagLifePC *pc, BedPad *thePad) override;                    // ROR 0x001211e4
};

/* layout: host (EE 8 bytes, +0x148). */
struct MagLifePCModeTurn : MagLifePCModeMove
{
    void init(MagLifePC *pc) override;                                   // ROR 0x001212c8
    void live(MagLifePC *pc) override;                                   // ROR 0x00121364
    int  pad(MagLifePC *pc, BedPad *thePad) override;                    // ROR 0x0012136c

    int m_04;                   // ROR 0x04 (PC +0x14c): the constructor leaves it
};

/* layout: host (EE 4 bytes, +0x150). */
struct MagLifePCModeAtkWait2D : MagLifePCMode
{
    void init(MagLifePC *pc) override;                                   // ROR 0x001214b4
    void live(MagLifePC *pc) override;                                   // ROR 0x001216a4
    int  pad(MagLifePC *pc, BedPad *thePad) override;                    // ROR 0x001217c0
};

/* layout: host (EE 4 bytes, +0x154). */
struct MagLifePCModeAtkMove2D : MagLifePCModeMove
{
    void init(MagLifePC *pc) override;                                   // ROR 0x00121858
    void live(MagLifePC *pc) override;                                   // ROR 0x00121890
    int  pad(MagLifePC *pc, BedPad *thePad) override;                    // ROR 0x00121898
};

/* layout: host (EE 0x10 bytes, +0x158). */
struct MagLifePCModeAttacking : MagLifePCModeMove
{
    // ROR 0x00282ea8
    MagLifePCModeAttacking() : m_04(0), m_08(0), m_0c(0) {}

    void init(MagLifePC *pc) override;                                   // ROR 0x001218fc
    void live(MagLifePC *pc) override;                                   // ROR 0x00121934
    int  pad(MagLifePC *pc, BedPad *thePad) override;                    // ROR 0x00121c64

    int m_04;                   // ROR 0x04 (PC +0x15c)
    int m_08;                   // ROR 0x08 (PC +0x160)
    int m_0c;                   // ROR 0x0c (PC +0x164)
};

/* layout: host (EE 0xc bytes, +0x168). */
struct MagLifePCModeDamage : MagLifePCMode
{
    void init(MagLifePC *pc) override;                                   // ROR 0x00121cd8
    void live(MagLifePC *pc) override;                                   // ROR 0x00121fc8

    int m_04;                   // ROR 0x04 (PC +0x16c)
    int m_08;                   // ROR 0x08 (PC +0x170)
};

/* layout: host (EE 4 bytes, +0x174). */
struct MagLifePCModeItem : MagLifePCMode
{
    void init(MagLifePC *pc) override;                                   // ROR 0x001220f0
    void live(MagLifePC *pc) override;                                   // ROR 0x001221ec
};

/* layout: host (EE 4 bytes, +0x178). */
struct MagLifePCModeDogGo : MagLifePCMode
{
    void init(MagLifePC *pc) override;                                   // ROR 0x00122338
    void live(MagLifePC *pc) override;                                   // ROR 0x001223c0
};

/* layout: host (EE 4 bytes, +0x17c). */
struct MagLifePCModeDogCome : MagLifePCMode
{
    void init(MagLifePC *pc) override;                                   // ROR 0x0012245c
    void live(MagLifePC *pc) override;                                   // ROR 0x00122668
};

/* layout: host (EE 4 bytes, +0x180). */
struct MagLifePCModeDogStay : MagLifePCMode
{
    void init(MagLifePC *pc) override;                                   // ROR 0x001226d0
    void live(MagLifePC *pc) override;                                   // ROR 0x00122758
};

/* layout: host (EE 8 bytes, +0x184). */
struct MagLifePCModeHold : MagLifePCMode
{
    // ROR 0x00282f4c
    MagLifePCModeHold() : m_04(0) {}

    void init(MagLifePC *pc) override;                                   // ROR 0x001227e4
    /* Empty (linkonce). */
    // ROR 0x00282f44
    void live(MagLifePC *pc) override {}
    int  pad(MagLifePC *pc, BedPad *thePad) override;                    // ROR 0x00122828

    int m_04;                   // ROR 0x04 (PC +0x188): MagLifePC::reset4MapJump clears it
};

/* layout: host (EE 4 bytes, +0x18c). */
struct MagLifePCModeHoldAttack : MagLifePCMode
{
    void init(MagLifePC *pc) override;                                   // ROR 0x001228fc
};

/* layout: host (EE 4 bytes, +0x190). */
struct MagLifePCModeUnhold : MagLifePCMode
{
    void init(MagLifePC *pc) override;                                   // ROR 0x00122930
};

/* layout: host (EE 0xc bytes, +0x194). */
struct MagLifePCModeRide : MagLifePCMode
{
    void init(MagLifePC *pc) override;                                   // ROR 0x00122954
    void live(MagLifePC *pc) override;                                   // ROR 0x00122ad4
    int  pad(MagLifePC *pc, BedPad *thePad) override;                    // ROR 0x00122bd4
    /* MagLifePC::message 6 / 9 / 0xb / 0xd. */
    void message(MagLifePC *pc, int msg);                                // ROR 0x00122c9c

    int m_04;                   // ROR 0x04 (PC +0x198): the PC's constructor and reset4MapJump clear it
    int m_08;                   // ROR 0x08 (PC +0x19c)
};

/* layout: host (EE 4 bytes, +0x1a0). */
struct MagLifePCModeRoped : MagLifePCMode
{
    void init(MagLifePC *pc) override;                                   // ROR 0x00122d70
    void live(MagLifePC *pc) override;                                   // ROR 0x00122dec
    int  pad(MagLifePC *pc, BedPad *thePad) override;                    // ROR 0x00122e2c
};

/* layout: host (EE 4 bytes, +0x1a4). */
struct MagLifePCModeReload : MagLifePCMode
{
    void init(MagLifePC *pc) override;                                   // ROR 0x00122fc4
    void live(MagLifePC *pc) override;                                   // ROR 0x00123004
};

/* layout: host (EE 4 bytes, +0x1a8). */
struct MagLifePCModeHug : MagLifePCMode
{
    void init(MagLifePC *pc) override;                                   // ROR 0x001230a8
    void live(MagLifePC *pc) override;                                   // ROR 0x001230fc
};

#endif /* _MAG_MAGLIFEPCMODE_H */
