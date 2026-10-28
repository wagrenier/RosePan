/* ==========================================================================
 *  mag/maglifebwnmode.h
 *
 *  Brown's modes (mag/maglifebwnmode.cpp, 0x125bd4..): one small object per
 *  MagLifeBwn state, embedded in MagLifeBwn (+0xcf0..+0xd30, vptrs set by
 *  its constructor 0x125aa4). MagLifeBwn::applyState points m_mode_cur at
 *  the one for m_state and runs its init; MagLifeBwn::live runs its live
 *  each frame; term decides whether a requested state may replace it.
 *
 *  Four EE virtual slots (MagLifeBwnMode has no data, vptr at +0 of each
 *  object; the names and parameters are the ELF's linkonce symbols
 *  init__14MagLifeBwnModeR10MagLifeBwn, setParam__..R10MagLifeBwniPv,
 *  term__..R10MagLifeBwni, live__18MagLifeBwnModeNullR10MagLifeBwn):
 *  1 init, 2 live, 3 setParam, 4 term. No virtual destructor.
 *
 *  State -> mode (MagLifeBwn::applyState): 0 Null, 1 One, 2 Wait, 3 Sit,
 *  4 Fuse, 5 Move, 6 Path, 7 Turn, 8 Rotate, 9 Wall.
 *  Minimal: the classes MagLifeBwn embeds; their methods are stubs until
 *  mag/maglifebwnmode.cpp is written.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEBWNMODE_H
#define _MAG_MAGLIFEBWNMODE_H

#include <stddef.h>

struct MagLifeBwn;

/* EE 4 bytes (the vptr). layout: host -- polymorphic. */
struct MagLifeBwnMode
{
    /* EE slot 1 (+0x08): MagLifeBwn::applyState runs it on entry. */
    virtual void init(MagLifeBwn &bwn) {}                                // ROR 0x002834bc
    /* EE slot 2 (+0x10): MagLifeBwn::live runs it each frame. The shared
     * one: stepMotion, then state 0 once the motion sequence ends. */
    virtual int  live(MagLifeBwn &bwn);                                  // ROR 0x00125bd4
    /* EE slot 3 (+0x18) */
    virtual void setParam(MagLifeBwn &bwn, int id, void *param) {}       // ROR 0x002834c4
    /* EE slot 4 (+0x20): 0 refuses a requested state (unless it is 0);
     * force is 1 from MagLifeBwn::live, 0 from changeMode. */
    virtual int  term(MagLifeBwn &bwn, int force) { return 1; }          // ROR 0x002834cc
};

/* layout: host (EE 4 bytes, MagLifeBwn +0xcf0). */
struct MagLifeBwnModeNull : MagLifeBwnMode
{
    void init(MagLifeBwn &bwn) override;                                 // ROR 0x00125c24
    // ROR 0x00283524
    int  live(MagLifeBwn &bwn) override { return 0; }
};

/* layout: host (EE 4 bytes, +0xcf4). */
struct MagLifeBwnModeOne : MagLifeBwnMode
{
    void init(MagLifeBwn &bwn) override;                                 // ROR 0x001263a0
    int  live(MagLifeBwn &bwn) override;                                 // ROR 0x001263ec
};

/* layout: host (EE 0xc bytes, +0xcf8). */
struct MagLifeBwnModeWait : MagLifeBwnMode
{
    void init(MagLifeBwn &bwn) override;                                 // ROR 0x00125c2c

    int _04[2];                 // ROR 0x04: roles unknown
};

/* layout: host (EE 4 bytes, +0xd04). */
struct MagLifeBwnModeSit : MagLifeBwnMode
{
    void init(MagLifeBwn &bwn) override;                                 // ROR 0x00125cb8
    int  term(MagLifeBwn &bwn, int force) override;                      // ROR 0x00125d38
};

/* layout: host (EE 4 bytes, +0xd08). */
struct MagLifeBwnModeFuse : MagLifeBwnMode
{
    void init(MagLifeBwn &bwn) override;                                 // ROR 0x00125db4
    int  term(MagLifeBwn &bwn, int force) override;                      // ROR 0x00125e34
};

/* layout: host (EE 0xc bytes, +0xd0c). */
struct MagLifeBwnModeMove : MagLifeBwnMode
{
    void init(MagLifeBwn &bwn) override;                                 // ROR 0x00125eb0
    int  live(MagLifeBwn &bwn) override;                                 // ROR 0x00125f48
    // ROR 0x0028366c
    void setParam(MagLifeBwn &bwn, int id, void *param) override {}

    int _04[2];                 // ROR 0x04: roles unknown
};

/* layout: host (EE 0x10 bytes, +0xd18). */
struct MagLifeBwnModePath : MagLifeBwnMode
{
    void init(MagLifeBwn &bwn) override;                                 // ROR 0x00126008
    int  live(MagLifeBwn &bwn) override;                                 // ROR 0x00126044
    int  term(MagLifeBwn &bwn, int force) override;                      // ROR 0x001262a4

    int _04[3];                 // ROR 0x04: roles unknown
};

/* layout: host (EE 4 bytes, +0xd28). */
struct MagLifeBwnModeTurn : MagLifeBwnMode
{
    void init(MagLifeBwn &bwn) override;                                 // ROR 0x00126364
};

/* layout: host (EE 4 bytes, +0xd2c). */
struct MagLifeBwnModeWall : MagLifeBwnMode
{
    void init(MagLifeBwn &bwn) override;                                 // ROR 0x00126450
};

/* layout: host (EE 0x10 bytes, +0xd30). */
struct MagLifeBwnModeRotate : MagLifeBwnMode
{
    void init(MagLifeBwn &bwn) override;                                 // ROR 0x001264d8
    int  live(MagLifeBwn &bwn) override;                                 // ROR 0x0012668c

    int _04[3];                 // ROR 0x04: roles unknown
};

#endif /* _MAG_MAGLIFEBWNMODE_H */
