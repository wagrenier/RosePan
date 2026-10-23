/* ==========================================================================
 *  mag/maglifebwn.h
 *
 *  MagLifeBwn (mag/maglifebwn.cpp; vtable _vt$10MagLifeBwn 0x2e1140, base
 *  MagLifeControlChara, vptr at +0x14, constructor 0x125aa4): the
 *  controller of Brown, the dog. The scene holds one at +0x2afe0 (EE
 *  0xd40: MagLifeBwnCtrl follows at +0x2bd20) and binds Brown's actor to
 *  it (MagScene::setBwn). Minimal: what the scene calls.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEBWN_H
#define _MAG_MAGLIFEBWN_H

#include <stddef.h>

#include "mag/maglife.h"

/* layout: host -- polymorphic in the ROM (vptr +0x14); EE offsets in the
 * comments. */
struct MagLifeBwn : MagLifeControlChara
{
    /* EE slot 10 (+0x50): as MagLifePC::setActor; a NULL actor runs
     * 0x12498c. */
    void setActor(DogActor *actor, int no, int mode);                    // ROR 0x001256d4
    /* EE slot 15 (+0x78): MagLifeControlChara's, unless +0xce4 is set. */
    void renderPostProcess();                                            // ROR 0x001259e8
    /* EE slot 2 (+0x10) */
    void live() override;                                                // ROR 0x00124e40

    u_char _60[0xce4 - 0x60];
    int    m_ce4;               // ROR 0xce4: non-zero skips renderPostProcess; role unknown
    u_char _ce8[0xd40 - 0xce8];
};
static_assert(offsetof(MagLifeBwn, m_ce4) >= 0xce4, "m_ce4");
static_assert(sizeof(MagLifeBwn) >= 0xd40, "MagLifeBwn is 0xd40 bytes on the EE");

#endif /* _MAG_MAGLIFEBWN_H */
