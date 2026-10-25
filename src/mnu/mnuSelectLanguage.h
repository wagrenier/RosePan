/* ==========================================================================
 *  mnu/mnuSelectLanguage.h
 *
 *  MnuSelectLanguage (mnu/mnuSelectLanguage.cpp): the language select
 *  screen MagLifeMenuSelLang (mag/maglifemenu.h, menu 0x10010) holds at
 *  +0x40; closing it chains to the title (MagScene::closeMenu). Class name
 *  from the file (ror-inferred). Its constructor 0x1bdae4 and destructor
 *  0x1bdb4c are not written: the object lives in the global scene and
 *  starts zeroed.
 * ======================================================================== */
#ifndef _MNU_MNUSELECTLANGUAGE_H
#define _MNU_MNUSELECTLANGUAGE_H

#include <stddef.h>

#include "sdk/eetypes.h"

struct BedPad;
struct DogTexInfo;
struct I3dLayer;
struct MagScene;

/* EE 0x1090 (MagLifeMenuSelLang +0x40..+0x10d0). layout: host -- not
 * written, padded to its EE size. */
struct MnuSelectLanguage
{
    void init(DogTexInfo *tex);                                          // ROR 0x001bdc00
    void term();                                                         // ROR 0x001bdc54
    int  isInit();                                                       // ROR 0x001bdc94
    int  processPad(BedPad *pad);                                        // ROR 0x001bde58
    void draw(I3dLayer *layer);                                          // ROR 0x001bdfe0

    u_char    _00[0x1078];
    MagScene *m_scene;          // ROR 0x1078: MagLifeMenuSelLang::init stores the scene
    u_char    _107c[0x1090 - 0x107c];
};
static_assert(offsetof(MnuSelectLanguage, m_scene) == 0x1078, "m_scene");

#endif /* _MNU_MNUSELECTLANGUAGE_H */
