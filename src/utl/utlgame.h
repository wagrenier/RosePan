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

#endif /* _UTL_UTLGAME_H */
