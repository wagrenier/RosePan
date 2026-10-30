/* ==========================================================================
 *  mnu/mnuboard2d.h
 *
 *  MnuBoard2D (mnu/mnuboard2d.cpp, 0x1ad6b4..0x1adf04): the 2D board, one
 *  full-screen picture of data/mnurpk/mnuboard.rpk. MagLifeMenuBoard2D
 *  (mag/maglifemenu.h, the menu ids with bit 0x40000) holds one at +0x40
 *  and stores its type value as m_board before init. Class name from the
 *  wrapper's symbols (ror-symbol); no source string names the file.
 *
 *  processPad (exec) loads the texture mnuBoardGetTexName(m_board) +
 *  ".tm2" as "board1_tcc", shows the three sprites of the pattern image
 *  and fades the scene in over the script's global 0x59 frames, runs
 *  execMain until it returns 1, fades out over global 0x5a frames and
 *  closes. Two execMains, by m_main_mode (open sets 0; nothing in the ROM
 *  found sets 1): 0 holds the board while the script keeps bit 0x27 of
 *  g_sequence.m_bits (bit 7 of word 1) set, 1 waits for triangle (pad bit
 *  13). While the fade is complete, update keeps m_bits[1] bit 30 set
 *  (the shown bit MnuItem sets too); the close clears it.
 *  Its MsgManSet is constructed but never opened.
 * ======================================================================== */
#ifndef _MNU_MNUBOARD2D_H
#define _MNU_MNUBOARD2D_H

#include "sdk/eetypes.h"
#include "msg/msgman.h"
#include "spr/sprman.h"

struct BedPad;
struct DogTexInfo;
struct I3dLayer;
struct MagSystem;

/* The texture name (no extension) of board key in the {key, name} table
 * 0x2abdb8 (keys 0x40000..0x400e3, ended by key -1); NULL when the key is
 * not there. */
const char *mnuBoardGetTexName(int key);                                 // ROR 0x001ad7d4

/* EE 0x10c0 (MagLifeMenuBoard2D +0x40..+0x1100). layout: host -- SprMan
 * and MsgManSet hold pointers; the EE offsets are in the comments. */
struct MnuBoard2D
{
    MnuBoard2D();                                                        // ROR 0x001ad6b4
    ~MnuBoard2D();                                                       // ROR 0x001ad72c

    void init(DogTexInfo *tex);                                          // ROR 0x001ad824
    void term();                                                         // ROR 0x001ad870
    int  isInit();                                                       // ROR 0x001ad8bc
    /* 0 while shown, 1 when the board has closed, -1 before open. */
    int  processPad(BedPad *pad);                                        // ROR 0x001ad8cc
    void draw(I3dLayer *layer);                                          // ROR 0x001ad8e0

    /* 0, or -1 when already open / not open. */
    int  open();                                                         // ROR 0x001ad90c
    int  close();                                                        // ROR 0x001ad940
    int  exec(BedPad *pad);                                              // ROR 0x001ad96c
    int  initWork();                                                     // ROR 0x001adbd8
    void setDisp();                                                      // ROR 0x001adc0c
    int  update(BedPad *pad);                                            // ROR 0x001adc6c
    /* Mode 0: 1 once the script's bit 0x27 is clear and the fade done. */
    int  execMainFlag(BedPad *pad);                                      // ROR 0x001adcd8
    /* Mode 1: 1 once triangle was pressed and the fade done. */
    int  execMainPad(BedPad *pad);                                       // ROR 0x001addd0

    MsgManSet  m_msg;           // ROR 0x0000: never opened
    int        m_open;          // ROR 0x0778: open / close
    int        m_77c;           // ROR 0x077c: initWork / setDisp clear it; not read
    int        m_780;           // ROR 0x0780: initWork clears it; not read
    int        m_784;           // ROR 0x0784: initWork sets 1; not read
    int        _788;
    int        m_step;          // ROR 0x078c: exec's step
    int        m_main_mode;     // ROR 0x0790: 0 execMainFlag, 1 execMainPad (open sets 0)
    int        m_main_step;     // ROR 0x0794: execMain's step
    int        _798[2];
    int        m_7a0;           // ROR 0x07a0: open sets 2; not read
    SprMan     m_spr;           // ROR 0x07a4: EE 0x8ec, three sprites
    MagSystem *m_system;        // ROR 0x1090: MagSystem::m_current_system (its scene fades)
    I3dLayer  *m_layer;         // ROR 0x1094: draw's layer
    int        m_result;        // ROR 0x1098: execMainFlag's 1 once the bit cleared
    int        m_keep_tex;      // ROR 0x109c: term skips m_spr.release while set (exec clears it; nothing sets it)
    int        _10a0[5];
    int        m_init;          // ROR 0x10b4: isInit
    int        m_board;         // ROR 0x10b8: the board key (MagLifeMenuBoard2D::init: getTypeValue); 0x40001 at construction
};

#endif /* _MNU_MNUBOARD2D_H */
