/* ==========================================================================
 *  mag/magscenecmd.h
 *
 *  The scene's command glue (mag/magscenecmd.cpp, 0x143d3c..0x145020; the
 *  file name is the port's -- no source string names it): MagScene's fog
 *  fade, and the free functions the game, the tasks and the scripts drive
 *  Brown through -- each forwards to the scene's MagLifeBwnCtrl
 *  (g_magSystem.m_scene.m_bwn_ctrl, bss 0x727aa0). Brown's script native
 *  (039) lands in magBwnSeqCommand.
 * ======================================================================== */
#ifndef _MAG_MAGSCENECMD_H
#define _MAG_MAGSCENECMD_H

#include <stdint.h>

/* ---- Brown for GamMan (game/gamman.cpp) ---- */
/* Brown's controller level: 0 (auto) when on, 2 (manual) when off. */
void gamSetBwnFollow(int on);                                            // ROR 0x00144334
/* The find-target item to Brown's controller (0 none), no request. */
void gamSetBwnItem(int item);                                            // ROR 0x00144500
/* Brown's controller level: 1 when on, 0 (auto) when off. */
void gamSetBwnWait(int on);                                              // ROR 0x0014457c
/* Brown's controller gets the dog's position / rotation (a debug trace of
 * the vector first). */
void gamSetBwnPos(const float *pos);                                     // ROR 0x00144398
void gamSetBwnRot(const float *rot);                                     // ROR 0x001443d8
/* Their counterparts: Brown's position / rotation into out[0..2] when out
 * != NULL. */
void gamGetBwnPos(float *out);                                           // ROR 0x00144418
void gamGetBwnRot(float *out);                                           // ROR 0x0014448c

/* ---- Brown for the scene, the enemies and the effects ---- */
/* Brown's reaction to a hit: requestHurt(react) (state 10). damage is
 * passed and not read. */
void magBwnHurt(int react, int damage);                                  // ROR 0x0014426c
int  magBwnIsBarkPhase3(void);                                           // ROR 0x00144244
int  magBwnIsHurt(void);                                                 // ROR 0x001442a4
void magBwnNop(void);                                                    // ROR 0x001442cc
int  magBwnReturn0(void);                                                // ROR 0x001442d4
/* The bark sub object's query / set while Brown barks (state 11). */
int  magBwnBarkQuery(int v);                                             // ROR 0x001442dc
void magBwnBarkSet(int v);                                               // ROR 0x00144308
/* The controller level is 0 (auto). */
int  magBwnIsLevelAuto(void);                                            // ROR 0x00144388
void magBwnReset(void);                                                  // ROR 0x00144538
int  magBwnReturn0b(void);                                               // ROR 0x001445c8
int  magBwnReturn0c(void);                                               // ROR 0x001445d0
/* The find result's oid (0 while there is none). */
int  magBwnGetFoundOid(void);                                            // ROR 0x00144a40

/* ---- Brown's script commands (native 039) ---- */
/* args are the six script words, op first: an op number (3 the state's
 * code, 0x14 / 0x15 rotation / position, 0x1f walk, 0x21 / 0x22 angle,
 * 0x20 / 0x23 wait for the state, 0x24) or a four-char code; returns the
 * script's v0 (SEQ_RET_WAIT while a wait op's state runs). */
intptr_t magBwnSeqCommand(const intptr_t *args);                         // ROR 0x00144ae0

#endif /* _MAG_MAGSCENECMD_H */
