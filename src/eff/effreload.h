/* ==========================================================================
 *  eff/effreload.h
 *
 *  EffReload1 (kind 0x27), code 0x1790e4..0x179428 (after
 *  eff/effmuzzleflash.cpp, before eff/effsteam.cpp; no source string names
 *  the file -- named after the class, in the alphabetical Eff* link order).
 *
 *  The gun's reload (MagLifePC::reload): the pack's model 0 (the pack's own,
 *  not a copy; the scene's textures assigned) played once through motion
 *  m_shots - 1 of the pack (m_shots clamped to 6; none below 1, and the
 *  effect ends at once), placed at the PC actor's position turned by its
 *  rotation (z, x, y). renderPreMain poses it at m_frame / 60 s and draws
 *  it with utlDrawI3r; it is over once m_frame / 60 reaches the motion's
 *  length.
 * ======================================================================== */
#ifndef _EFF_EFFRELOAD_H
#define _EFF_EFFRELOAD_H

#include <stddef.h>

#include "eff/eff0.h"
#include "sdk/libvu0.h"

struct I3mRoot;
struct I3rRoot;
struct I3dMatrixF4x4;

/* The most motions the pack carries (one per bullet count). */
#define EFF_RELOAD_MAX_SHOTS 6

/* kind 0x27; EE 0x90, vtable 0x2e4130. layout: host. */
class EffReload1 : public Eff0
{
public:
    EffReload1();                                                           // ROR 0x001790e4
    virtual ~EffReload1();                                                  // ROR 0x0017911c
    virtual int  init(const EffParam *param);                               // ROR 0x001791ac
    virtual int  liveMain();                                                // ROR 0x00179324
    virtual void renderPreMain(RENDERPARAM *param);                         // ROR 0x00179390
    /* Drawn in renderPreMain through utlDrawI3r. */
    virtual u_long *renderMain(RENDERPARAM *, const I3dDrawSet *, u_long *) { return NULL; } // ROR 0x0028ca88 (linkonce)

    I3rRoot       *m_root;      // ROR 0x24: the pack's model 0, NULL when m_shots < 1
    I3mRoot       *m_motion;    // ROR 0x28: the pack's motion m_shots - 1
    void          *m_nodes;     // ROR 0x2c: the node block (one allocation, 0xb0 a node): the pose (0x30 a node)
    I3dMatrixF4x4 *m_local;     // ROR 0x30: in it, the local transforms
    I3dMatrixF4x4 *m_world;     // ROR 0x34: in it, one model matrix per node
    u_short       *m_node_map;  // ROR 0x38: effMakeNodeMap of the motion
    int            _3c;
    sceVu0FMATRIX  m_mat;       // ROR 0x40: the placement: rotation m_reload.m_rot (z, x, y), row 3 m_reload.m_pos
    int            m_frame;     // ROR 0x80: frames since init
    u_char         _84[0x90 - 0x84];
};

#endif /* _EFF_EFFRELOAD_H */
