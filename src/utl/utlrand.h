/* ==========================================================================
 *  utl/utlrand.h
 *
 *  The game's random numbers (ROR 0x1041c4..0x10452c; no source-path
 *  string, the file name is the port's): one 32-bit linear congruential
 *  seed (data 0x308484) and the distributions every enemy, the dog and the
 *  battle rolls draw from. MagSystem's constructor resets the seed, so a
 *  run's sequence depends only on how many draws came before.
 * ======================================================================== */
#ifndef _UTL_UTLRAND_H
#define _UTL_UTLRAND_H

#include <stdint.h>

/* The seed word (data 0x308484). */
extern uint32_t g_utl_rand_seed;

/* seed = seed * 0x769 + 0x7cf9. */
void  utlRandStep(void);                                                 // ROR 0x001041c4
/* Steps the seed, then (seed * n) >> 32: an integer in 0..n-1 for n > 0. */
int   utlRand(int n);                                                    // ROR 0x001041e4
/* seed = 0xc45330af (MagSystem::MagSystem). */
void  utlRandReset(void);                                                // ROR 0x0010421c
/* seed = theSeed; nothing in the ROM calls it. */
void  utlRandSeed(uint32_t theSeed);                                     // ROR 0x00104230
/* A random integer in lo..hi (lo + utlRand(hi - lo + 1)). */
int   utlRandRange(int lo, int hi);                                      // ROR 0x0010423c
/* lo + the mean of n draws over 0..hi - lo: values bunch in the middle. */
int   utlRandMean(int lo, int hi, int n);                                // ROR 0x0010426c
/* lo + |mean of n draws over 0..2 (hi - lo) - (hi - lo)|: lo..hi, peaking at lo. */
int   utlRandPeakLo(int lo, int hi, int n);                              // ROR 0x001042ec
/* The mean of n draws over 0..2 (hi - lo) folded at hi - lo, plus lo:
 * lo..hi, peaking at hi. */
int   utlRandPeakHi(int lo, int hi, int n);                              // ROR 0x00104394
/* lo..hi peaking at peak: one utlRandRange(lo, hi) picks the side
 * (above peak: utlRandPeakHi(lo, peak, n), else utlRandPeakLo(peak, hi, n)). */
int   utlRandPeak(int lo, int hi, int peak, int n);                      // ROR 0x00104438
/* A random float in lo..hi in steps of (hi - lo) / 65536 -- 0.0 (not lo)
 * when the integer draw is 0. */
float utlRandRangeF(float lo, float hi);                                 // ROR 0x001044b8

#endif /* _UTL_UTLRAND_H */
