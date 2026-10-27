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
/* With no menu open: GamMan::pause(0x100000) and bss 0x7a3208 |= 0x400;
 * returns whether it paused. */
int utlPauseForMenu(void);                                               // ROR 0x00103de4
/* GamMan's PC equip item / PC actor numbers (assert id < 128). */
int utlGetPcEquipItemChrID(void);                                        // ROR 0x00103e4c
int utlGetPcChrNo(void);                                                 // ROR 0x00103e94

#endif /* _UTL_UTLGAME_H */
