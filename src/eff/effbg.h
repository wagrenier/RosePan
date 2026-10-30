/* ==========================================================================
 *  eff/effbg.h
 *
 *  EffBg1 (kind 0x34): EffGeneric1's record animation run over the map's
 *  own model (the scene's first sky slot) instead of a model of its own --
 *  scrolling uv, keyed uv and keyed texture swaps on the material the
 *  param names. Its code is eff/effbg.cpp (0x16afcc..0x16b62c), the first
 *  Eff class in link order, between eff/eff0.cpp and eff/effblood.cpp (the
 *  file name is inferred from the class: no source-path string names it).
 * ======================================================================== */
#ifndef _EFF_EFFBG_H
#define _EFF_EFFBG_H

#include "eff/effgeneric.h"

/* kind 0x34; EE 0x90 (EffGeneric1's), vtable 0x2e32c0. layout: host. */
class EffBg1 : public EffGeneric1
{
public:
    EffBg1();                                                               // ROR 0x0016afcc
    virtual ~EffBg1();                                                      // ROR 0x0016affc
    virtual int  init(const EffParam *param);                               // ROR 0x0016b060
    virtual int  liveMain();                                                // ROR 0x0016b348
    virtual void renderPreMain(RENDERPARAM *param);                         // ROR 0x0016b444
    /* Nothing of its own to draw: the map's model carries the changes. */
    virtual u_long *renderMain(RENDERPARAM *, const I3dDrawSet *, u_long *) { return NULL; } // ROR 0x0028ae70 (linkonce)

    /* Type 6: the texture of the last key at or before m_frame copied onto
     * each layer of the record's material that does not show it yet. */
    void applyTexKeys(const EffGenTexKeys *rec);                            // ROR 0x0016b50c
};

#endif /* _EFF_EFFBG_H */
