/* ==========================================================================
 *  mnu/mnunow.h
 *
 *  MnuNow (mnu/mnunow.cpp, 0x1ae474..0x1ae954): the "now loading" screen.
 *  MagLifeMenuNow (mag/maglifemenu.h, menu 0x10009) holds one at +0x40;
 *  MagScene points its menu at it (MagScene::m_menu_now). Class name from
 *  the wrapper (ror-inferred); no source string names the file.
 *
 *  It reads data/mnurpk/mnunow.rpk: loading.tm2 (nowloading_tcc, texture
 *  0), four 256 x 64 rows of one picture. The display list has two
 *  sprites: 0 off, 1 at (32, 370) cycling the four rows six frames each.
 *  processPad fades the scene in (time 4), runs the sprites until circle
 *  is pressed, then fades out (3), resets the double buffer and returns
 *  1. The same open / exec / execMain shape as MnuMovie and MnuOpt.
 * ======================================================================== */
#ifndef _MNU_MNUNOW_H
#define _MNU_MNUNOW_H

#include "sdk/eetypes.h"
#include "spr/sprman.h"

struct BedPad;
struct DogTexInfo;
struct I3dLayer;
struct MagScene;
struct MagSystem;

/* EE 0x920 (MagLifeMenuNow +0x40..+0x960). layout: host -- SprMan
 * holds pointers; the EE offsets are in the comments. */
struct MnuNow
{
    MnuNow();                                                            // ROR 0x001ae474
    ~MnuNow();                                                           // ROR 0x001ae4ac

    void init(DogTexInfo *tex);                                          // ROR 0x001ae520
    void term();                                                         // ROR 0x001ae574
    int  isInit();                                                       // ROR 0x001ae5b4
    /* 0 while shown, 1 when the fade out has ended, -1 before open. */
    int  processPad(BedPad *pad);                                        // ROR 0x001ae600
    void draw(I3dLayer *layer);                                          // ROR 0x001ae614

    void loadTexture();                                                  // ROR 0x001ae5bc
    /* 0, or -1 when already open / not open. */
    int  open();                                                         // ROR 0x001ae640
    int  close();                                                        // ROR 0x001ae674
    int  exec(BedPad *pad);                                              // ROR 0x001ae6a0
    /* 0 in the ROM. */
    int  initWork();                                                     // ROR 0x001ae7f0
    void setDisp();                                                      // ROR 0x001ae7f8
    int  update(BedPad *pad);                                            // ROR 0x001ae848
    /* 0, then m_result (1) once circle's fade out has ended. */
    int  execMain(BedPad *pad);                                          // ROR 0x001ae868

    int        m_open;          // ROR 0x000: open / close
    int        m_step;          // ROR 0x004: exec's step
    int        m_pause;         // ROR 0x008: execMain is skipped while set (open clears it; nothing sets it)
    int        m_main_step;     // ROR 0x00c: execMain's step
    int        m_10;            // ROR 0x010: open sets 2; not read
    SprMan     m_spr;           // ROR 0x014: EE 0x8ec
    MagSystem *m_system;        // ROR 0x900: MagSystem::m_current_system (setDisp)
    I3dLayer  *m_layer;         // ROR 0x904: draw's layer
    int        m_result;        // ROR 0x908: 1 once circle was pressed
    MagScene  *m_scene;         // ROR 0x90c: MagLifeMenuNow::init stores the scene
    int        m_init;          // ROR 0x910: isInit
};

#endif /* _MNU_MNUNOW_H */
