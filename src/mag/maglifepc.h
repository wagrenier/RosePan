/* ==========================================================================
 *  mag/maglifepc.h
 *
 *  MagLifePC (mag/maglifepc*.cpp; vtable _vt$9MagLifePC, base
 *  MagLifeControlChara, vptr at +0x14, constructor 0x11bf0c): the player
 *  character's controller. The scene holds one at +0x2adc0 and binds the
 *  player's actor to it (MagScene::setPC). Minimal: only the words
 *  MagSystem reads; the class hierarchy comes with the maglifepc TUs.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEPC_H
#define _MAG_MAGLIFEPC_H

/* EE size unknown. layout: host -- polymorphic in the ROM (vptr +0x14);
 * only the words MagSystem reads are placed, EE offsets in the comments. */
struct MagLifePC
{
    int getSubStateClass();                                              // ROR 0x0011e080

    int m_state;                // ROR 0x00: MagSystem::getPCStateClass switches on it (1..0x17)
    int m_0c;                   // ROR 0x0c: state 9 asks it for 1
    int m_e4;                   // ROR 0xe4: MagSystem::getPCWordE4 returns it; role unknown
    int m_ec;                   // ROR 0xec: getSubStateClass maps 1 / 2 / other
};

#endif /* _MAG_MAGLIFEPC_H */
