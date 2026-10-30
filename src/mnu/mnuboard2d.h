/* ==========================================================================
 *  mnu/mnuboard2d.h
 *
 *  mnu/mnuboard2d.cpp (inferred by link order, before mnu/mnunow.cpp):
 *  the 2D board pictures. Only what MnuItem calls is declared yet.
 * ======================================================================== */
#ifndef _MNU_MNUBOARD2D_H
#define _MNU_MNUBOARD2D_H

#include "sdk/eetypes.h"

/* The texture name (no extension) of board key in the {key, name} table
 * 0x2abdb8 (ended by key -1); NULL when the key is not there. */
const char *mnuBoardGetTexName(int key);                                 // ROR 0x001ad7d4

#endif /* _MNU_MNUBOARD2D_H */
