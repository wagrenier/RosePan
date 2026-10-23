/* ==========================================================================
 *  mag/maglifebwnctrl.h
 *
 *  MagLifeBwnCtrl (mag/maglifebwnctrl.cpp; vtable _vt$14MagLifeBwnCtrl
 *  0x2e13e0, base MagLife, vptr at +0x14, constructor 0x12ba80): Brown's
 *  command layer over MagLifeBwn. The scene holds one at +0x2bd20 (EE
 *  0x130: the scene's menu words follow at +0x2be50); its state word is what
 *  MagSystem::getBwnState reads. Minimal: what the scene touches. EE 0x130:
 *  the words at +0x130 / +0x138 an earlier session gave it are the scene's
 *  menu id and menu result (+0x2be50 / +0x2be58, MagScene::closeMenu).
 * ======================================================================== */
#ifndef _MAG_MAGLIFEBWNCTRL_H
#define _MAG_MAGLIFEBWNCTRL_H

#include <stddef.h>

#include "mag/maglife.h"
#include "utl/utlfind.h"

struct BedPad;
struct MagLifeBwn;
struct MagLifePC;

/* layout: host -- polymorphic in the ROM (vptr +0x14); EE offsets in the
 * comments. */
struct MagLifeBwnCtrl : MagLife
{
    /* EE slot 2 (+0x10) */
    void live() override;                                                // ROR 0x0012a61c
    /* EE slot 9 (+0x48): empty. */
    void processPad(BedPad *pad) {}                                      // ROR 0x0012a6a0

    u_char      _28[0x48 - 0x28];
    UtlFind     m_find;         // ROR 0x48: cleared by MagScene::clearPC and termWorld
    u_char      _60[0x70 - 0x60];
    MagLifeBwn *m_bwn;          // ROR 0x70: Brown's controller while Brown is bound (MagScene::setBwn)
    MagLifePC  *m_pc;           // ROR 0x74: the PC's controller while Brown is bound
    int         m_78;           // ROR 0x78: MagScene::clearPC zeroes it; role unknown
    u_char      _7c[0x130 - 0x7c];
};
static_assert(offsetof(MagLifeBwnCtrl, m_find) >= 0x48, "m_find");
static_assert(offsetof(MagLifeBwnCtrl, m_bwn) >= 0x70, "m_bwn");
static_assert(sizeof(MagLifeBwnCtrl) >= 0x130, "MagLifeBwnCtrl is 0x130 bytes on the EE");

#endif /* _MAG_MAGLIFEBWNCTRL_H */
