/* ==========================================================================
 *  eff/effclasses.h
 *
 *  The Eff classes EffMan::createEff (eff/effman.cpp) news, one per effect
 *  kind, declared only as far as createEff needs them: the constructor and
 *  destructor and the virtual slots each overrides (rtti.py; Eff0's slot
 *  names), with the EE object size createEff passes to operator new. Their
 *  code (0x16afcc..0x17b6d4 outside eff/effman.cpp, in alphabetical class
 *  link order) is not written yet: the overrides are stubs, and the data
 *  members past Eff0 are an undecoded block (m_body) of the EE size. When a
 *  class's own file is written, move its declaration into that file's
 *  header and include it here.
 *
 *  Constructors marked linkonce are the ELF's inline ones (createEff inlines
 *  them: Eff0::Eff0, then the class's vtable); the others are out of line.
 *  Kinds 7, 0xe..0x10, 0x24, 0x25, 0x28 and 0x30 have no class (createEff
 *  news nothing for them). EffFindItem1 (kind 0x32) lives in eff/effman.h.
 * ======================================================================== */
#ifndef _EFF_EFFCLASSES_H
#define _EFF_EFFCLASSES_H

#include "eff/eff0.h"
#include "eff/effbg.h"
#include "eff/effblood.h"
#include "eff/effbreath.h"
#include "eff/effcameraanim1.h"
#include "eff/effdust2.h"
#include "eff/efffire1.h"
#include "eff/efffire2.h"
#include "eff/efffog1.h"
#include "eff/efffog2.h"
#include "eff/efffog3.h"
#include "eff/efffoot.h"
#include "eff/efflight.h"
#include "eff/effmuzzleflash.h"
#include "eff/effreload.h"
#include "eff/effsteam.h"
#include "eff/effwater.h"

/* kinds 0x1..0x6 (EffBloodSpray1 .. EffBloodStay3Prim): eff/effblood.h.
 * kinds 0x8..0xc (EffBreath1 .. EffBreath2Ground): eff/effbreath.h. */

/* kinds 0x12, 0x13 (EffDust2, EffDust2Prim): eff/effdust2.h.
 * kinds 0x14, 0x15 (EffFire1, EffFire1Ex): eff/efffire1.h.
 * kind 0x16 (EffFire2): eff/efffire2.h.
 * kind 0x17 (EffFog1): eff/efffog1.h.
 * kinds 0x18, 0x19 (EffFog2, EffFog2Prim): eff/efffog2.h. */

/* kinds 0x1a..0x1c (EffFog3, EffFog3Dark, EffFog3WeakUp): eff/efffog3.h. */

/* kinds 0x1d..0x20 (EffFoot1 .. EffFoot2Prim): eff/efffoot.h. */

/* kinds 0x33, 0x34 (EffGeneric1, EffBg1): eff/effgeneric.h, eff/effbg.h. */

/* kinds 0x21..0x23 (EffLight1, EffLight2, EffLight3): eff/efflight.h. */

/* kind 0x26 (EffMuzzleFlash1): eff/effmuzzleflash.h. */

/* kind 0x27 (EffReload1): eff/effreload.h. */

/* kinds 0x11, 0x29..0x2e (EffCloud1, EffSteam1 .. EffSteam2Prim): eff/effsteam.h. */

/* kinds 0x2f, 0x31 (EffWater1, EffWater3): eff/effwater.h. */

#endif /* _EFF_EFFCLASSES_H */
