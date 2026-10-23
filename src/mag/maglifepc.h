/* ==========================================================================
 *  mag/maglifepc.h
 *
 *  MagLifePC (mag/maglifepc*.cpp; vtable _vt$9MagLifePC 0x2e0d68, base
 *  MagLifeControlChara, vptr at +0x14, constructor 0x11bf0c): the player
 *  character's controller. The scene holds one at +0x2adc0 (EE size at
 *  most 0x220, the distance to MagLifeBwn) and binds the player's actor to
 *  it (MagScene::setPC). Minimal: the words MagSystem and the scene use.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEPC_H
#define _MAG_MAGLIFEPC_H

#include <stddef.h>

#include "mag/maglife.h"

struct BedPad;

/* layout: host -- polymorphic in the ROM (vptr +0x14); EE offsets in the
 * comments. */
struct MagLifePC : MagLifeControlChara
{
    /* EE slot 10 (+0x50): binds the actor (NULL unbinds), resets the
     * sub state and the PC mode. */
    void setActor(DogActor *actor, int no, int mode);                    // ROR 0x0011bd4c
    /* EE slot 13 (+0x68). */
    void termWorld();                                                    // ROR 0x0011c418
    void initMotion();                                                   // ROR 0x0011dd40
    /* EE slot 2 (+0x10) */
    void live() override;                                                // ROR 0x0011ade4
    /* EE slot 16 (+0x90) */
    void processPad(BedPad *pad);                                        // ROR 0x0011b30c
    /* Word 1 while the state word is 0x17, else -1. */
    int  getState17Count();                                              // ROR 0x0011af10
    int  getSubStateClass();                                             // ROR 0x0011e080

    u_char _60[0xe4 - 0x60];
    int    m_e4;                // ROR 0xe4: MagSystem::getPCWordE4 returns it; setActor stores 0x11ab88's result
    u_char _e8[0xec - 0xe8];
    int    m_ec;                // ROR 0xec: getSubStateClass maps 1 / 2 / other; setActor sets -1
};
static_assert(offsetof(MagLifePC, m_e4) >= 0xe4, "m_e4");
static_assert(offsetof(MagLifePC, m_ec) >= 0xec, "m_ec");

#endif /* _MAG_MAGLIFEPC_H */
