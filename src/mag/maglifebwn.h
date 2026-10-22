/* ==========================================================================
 *  mag/maglifebwn.h
 *
 *  MagLifeBwn (mag/maglifebwn.cpp; vtable _vt$10MagLifeBwn, base
 *  MagLifeControlChara, vptr at +0x14, constructor 0x125aa4): the
 *  controller of Brown, the dog. The scene holds one at +0x2afe0
 *  (MagScene::setBwn). Minimal: only the word MagSystem reads.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEBWN_H
#define _MAG_MAGLIFEBWN_H

/* EE size unknown. layout: host -- polymorphic in the ROM (vptr +0x14);
 * only the word MagSystem reads is placed, its EE offset in the comment. */
struct MagLifeBwn
{
    int m_d40;                  // ROR 0xd40: MagSystem::getBwnState returns it when 0..13 (scene +0x2bd20)
};

#endif /* _MAG_MAGLIFEBWN_H */
