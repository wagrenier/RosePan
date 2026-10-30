/* ==========================================================================
 *  mnu/mnumovie.h
 *
 *  MnuMovie (mnu/mnumovie.cpp, 0x1adf04..0x1ae474): the movie theatre's
 *  player. MagLifeMenuMovie (mag/maglifemenu.h, menu 0x10007) holds one
 *  at +0x40. Class name from the wrapper (ror-inferred); no source string
 *  names the file.
 *
 *  It plays the movie whose name the caller copied into g_rxMpeg.m_name
 *  (512 x 384, at once: RxMpeg::playNow) with MagSystem::m_v_interval at
 *  1 (60 fps), until the movie ends or circle / cross is pressed, then
 *  closes it and puts the interval back to 2. draw shows the picture and
 *  two black bars (screen rows 32..96 and 416..480) over the frame
 *  buffer's top and bottom. The same open / exec / execMain shape as
 *  MnuOpt; its SprMan is constructed but never set up.
 * ======================================================================== */
#ifndef _MNU_MNUMOVIE_H
#define _MNU_MNUMOVIE_H

#include "sdk/eetypes.h"
#include "spr/sprman.h"

struct BedPad;
struct DogTexInfo;
struct I3dLayer;
struct MagScene;
struct MagSystem;

/* EE 0x990 (MagLifeMenuMovie +0x40..+0x9d0). layout: host -- SprMan
 * holds pointers; the EE offsets are in the comments. */
struct MnuMovie
{
    MnuMovie();                                                          // ROR 0x001adf04
    ~MnuMovie();                                                         // ROR 0x001adf3c

    void init(DogTexInfo *tex);                                          // ROR 0x001adfb0
    void term();                                                         // ROR 0x001adfe4
    int  isInit();                                                       // ROR 0x001ae01c
    /* 0 while playing, 1 when the movie has closed, -1 before open. */
    int  processPad(BedPad *pad);                                        // ROR 0x001ae02c
    void draw(I3dLayer *layer);                                          // ROR 0x001ae040

    /* Empty in the ROM. */
    void prepare();                                                      // ROR 0x001ae024
    /* 0, or -1 when already open / not open. */
    int  open();                                                         // ROR 0x001ae188
    int  close();                                                        // ROR 0x001ae1bc
    int  exec(BedPad *pad);                                              // ROR 0x001ae1e8
    int  initWork();                                                     // ROR 0x001ae2d8
    void setDisp();                                                      // ROR 0x001ae318
    /* 0 in the ROM. */
    int  update(BedPad *pad);                                            // ROR 0x001ae328
    /* 0 while playing, 1 in the frame the movie closed. */
    int  execMain(BedPad *pad);                                          // ROR 0x001ae330

    int        m_open;          // ROR 0x000: open / close
    int        m_work[7];       // ROR 0x004: initWork clears them; nothing reads them
    int        m_20;            // ROR 0x020: initWork sets 5; not read
    int        m_24;            // ROR 0x024: initWork sets 2; not read
    int        m_28;            // ROR 0x028: initWork sets 2; not read
    int        _2c[4];
    int        m_step;          // ROR 0x03c: exec's step
    int        m_pause;         // ROR 0x040: execMain is skipped while set (open clears it; nothing sets it)
    int        m_main_step;     // ROR 0x044: execMain's step
    int        _48[2];
    int        m_50;            // ROR 0x050: open sets 2; not read
    SprMan     m_spr;           // ROR 0x054: EE 0x8ec; never set up
    MagSystem *m_system;        // ROR 0x940: MagSystem::m_current_system (setDisp)
    I3dLayer  *m_layer;         // ROR 0x944: draw's layer
    int        _948[8];
    MagScene  *m_scene;         // ROR 0x968: MagLifeMenuMovie::init stores the scene
    int        m_result;        // ROR 0x96c: 1 once the movie ended or was skipped
    int        m_init;          // ROR 0x970: isInit
};

#endif /* _MNU_MNUMOVIE_H */
