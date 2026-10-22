/* ==========================================================================
 *  mag/magscene.h
 *
 *  The scene (mag/magscene*.cpp). MagSystem holds one as its member at
 *  +0x12540 (bss 0x6fbd80); MagSystem::init / setSceneFlags / start pass it
 *  to 0x131840, 0x13f528, 0x131a28, 0x131b90. Minimal: only what main
 *  reaches is declared; the scene TUs fill in the class.
 * ======================================================================== */
#ifndef _MAG_MAGSCENE_H
#define _MAG_MAGSCENE_H

/* EE size unknown yet. layout: host -- no members reconstructed. */
struct MagScene
{
    void initCamera();                                                   // ROR 0x0013c928
};

#endif /* _MAG_MAGSCENE_H */
