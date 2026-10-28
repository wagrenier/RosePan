/* ==========================================================================
 *  utl/utlgame.h
 *
 *  Game-state queries (utl/utlgame.cpp). Minimal: only what reconstructed
 *  code calls so far.
 * ======================================================================== */
#ifndef _UTL_UTLGAME_H
#define _UTL_UTLGAME_H

/* GamMan's option 0x12 (asked through 0x1c6ccc) != 0: the controller
 * vibration switch MagSystem gates BedPad::setVibration on. */
int utlIsVibrationOn(void);                                              // ROR 0x00103484
/* Plays sound id at pos (NULL: no position) on the sound object 0x79a0c0. */
void utlPlaySe3d(int id, const float *pos);                              // ROR 0x0010254c
/* GamMan::getActorNo(oid). */
int utlGetActorNo(unsigned int oid);                                     // ROR 0x001034e4
/* GamMan::getStatus(5): m_wrdNo, the current room. */
int utlGetWrdNo(void);                                                   // ROR 0x001034b8
/* With no menu open: GamMan::pause(0x100000) and bss 0x7a3208 |= 0x400;
 * returns whether it paused. */
int utlPauseForMenu(void);                                               // ROR 0x00103de4
/* GamMan's PC equip item / PC actor numbers (assert id < 128). */
int utlGetPcEquipItemChrID(void);                                        // ROR 0x00103e4c
int utlGetPcChrNo(void);                                                 // ROR 0x00103e94
/* One of three / four sound ids at random (0x10423c), played without a
 * position (0x1026b0). */
void utlPlaySeRand3(int a, int b, int c);                                // ROR 0x00102918
void utlPlaySeRand4(int a, int b, int c, int d);                         // ROR 0x001029c4
/* g_sequence.m_bits[1] & 4: script flag 34 (MagLifePC::processPad's dog
 * commands need it). */
int utlTestSeqBit34(void);                                               // ROR 0x0010346c

#endif /* _UTL_UTLGAME_H */
