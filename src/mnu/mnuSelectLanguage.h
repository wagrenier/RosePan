/* ==========================================================================
 *  mnu/mnuSelectLanguage.h
 *
 *  MnuSelectLanguage (mnu/mnuSelectLanguage.cpp, 0x1bdae4..0x1be2cc): the
 *  language select screen. MagLifeMenuSelLang (mag/maglifemenu.h, menu
 *  0x10010) holds one at +0x40; closing it chains to the title
 *  (MagScene::closeMenu). Class name from the file (ror-inferred).
 *
 *  It reads data/mnurpk/mnusllg.rpk: location.tm2 (sellang_bg_tcc), the
 *  512 x 448 background, sprite 1 of the display list. Over it a 512 x
 *  256 scene window at (220, 120) shows the five languages, the cursor's
 *  in colour 15, the others in 8. processPad saves the volume and sets the
 *  brightness option to its default (4) for the screen, then up / down
 *  move the cursor (0..4), cross takes it (MagSystem::setLanguage) and
 *  fades the scene out (time 7); once the fade has ended it restores the
 *  volume and the player's brightness and returns -1. The same open /
 *  exec / execMain shape as MnuNow, with exec inline in processPad.
 * ======================================================================== */
#ifndef _MNU_MNUSELECTLANGUAGE_H
#define _MNU_MNUSELECTLANGUAGE_H

#include "sdk/eetypes.h"
#include "msg/msgman.h"
#include "spr/sprman.h"

struct BedPad;
struct DogTexInfo;
struct I3dLayer;
struct MagScene;

/* EE 0x1090 (MagLifeMenuSelLang +0x40..+0x10d0). layout: host -- SprMan
 * and MsgManSet hold pointers; the EE offsets are in the comments. */
struct MnuSelectLanguage
{
    MnuSelectLanguage();                                                 // ROR 0x001bdae4
    ~MnuSelectLanguage();                                                // ROR 0x001bdb4c

    void init(DogTexInfo *tex);                                          // ROR 0x001bdc00
    void term();                                                         // ROR 0x001bdc54
    int  isInit();                                                       // ROR 0x001bdc94
    /* 0 while shown; -1 before open and once the language was taken and
     * the fade out has ended. */
    int  processPad(BedPad *pad);                                        // ROR 0x001bde58
    void draw(I3dLayer *layer);                                          // ROR 0x001bdfe0

    void openWindow();                                                   // ROR 0x001bdc9c
    void closeWindow();                                                  // ROR 0x001bdd48
    /* Shows the language list with the cursor's language highlighted. */
    void updateMessage();                                                // ROR 0x001bddbc
    void loadTexture();                                                  // ROR 0x001be004
    /* 0, or -1 when already open / not open. */
    int  open();                                                         // ROR 0x001be048
    int  close();                                                        // ROR 0x001be084
    /* 0 in the ROM. */
    int  initWork();                                                     // ROR 0x001be0c0
    void setDisp();                                                      // ROR 0x001be0cc
    /* 0, then 1 once a language was taken and the fade out has ended. */
    int  execMain(BedPad *pad);                                          // ROR 0x001be110

    int        m_open;          // ROR 0x0000: open / close
    int        _04[2];
    int        m_step;          // ROR 0x000c: processPad's step
    int        m_main_step;     // ROR 0x0010: execMain's step
    MsgManSet  m_msg;           // ROR 0x0014: EE 0x778, the language list
    SprMan     m_spr;           // ROR 0x078c: EE 0x8ec, the background
    MagScene  *m_pScene;        // ROR 0x1078: MagLifeMenuSelLang::init stores the scene (assert m_pScene)
    int        m_language;      // ROR 0x107c: the cursor, 0..4 (English, French, German, Spanish, Italian)
    int        m_init;          // ROR 0x1080: isInit
    int        m_window;        // ROR 0x1084: the scene window's slot, -1 when closed
};

#endif /* _MNU_MNUSELECTLANGUAGE_H */
