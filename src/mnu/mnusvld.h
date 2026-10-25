/* ==========================================================================
 *  mnu/mnusvld.h
 *
 *  MnuSvld (mnu/mnusvld.cpp): the memory card save / load screen, one
 *  class for both. MagLifeMenuSave and MagLifeMenuLoad (mag/maglifemenu.h)
 *  hold one at +0x40 and set m_save (1 save, 0 load) in their
 *  constructors; MnuTitle has one at +0x1400. Class name from the file
 *  (ror-inferred). Its constructor 0x1a59b8 and destructor 0x1a5a34 are
 *  not written: the objects live in the global scene and start zeroed.
 * ======================================================================== */
#ifndef _MNU_MNUSVLD_H
#define _MNU_MNUSVLD_H

#include <stddef.h>

#include "sdk/eetypes.h"

struct BedPad;
struct DogTexInfo;
struct I3dLayer;
struct MagScene;

/* EE 0x2780 (MagLifeMenuSave +0x40..+0x27c0). layout: host -- not
 * written, padded to its EE size. */
struct MnuSvld
{
    void init(DogTexInfo *tex);                                          // ROR 0x001a5af4
    void term();                                                         // ROR 0x001a5bf4
    int  isInit();                                                       // ROR 0x001a5c78
    int  processPad(BedPad *pad);                                        // ROR 0x001a5cc4
    void draw(I3dLayer *layer);                                          // ROR 0x001a5cd8

    u_char    _00[0x24e8];
    int       m_save;           // ROR 0x24e8: 1 the save screen, 0 the load screen (the wrappers' constructors)
    u_char    _24ec[0x2730 - 0x24ec];
    MagScene *m_scene;          // ROR 0x2730: MagLifeMenuSave / Load::init store the scene
    u_char    _2734[0x2780 - 0x2734];
};
static_assert(offsetof(MnuSvld, m_save) == 0x24e8, "m_save");
static_assert(offsetof(MnuSvld, m_scene) >= 0x2730, "m_scene");

#endif /* _MNU_MNUSVLD_H */
