/* ==========================================================================
 *  utl/utlrand.cpp
 *
 *  The game's random numbers. See utlrand.h.
 *
 *  The ROM divides by n in the mean / peak draws with a trap on zero
 *  (break 7); every caller passes n >= 1, and the port returns lo instead
 *  of dividing by zero.
 * ======================================================================== */
#include "utl/utlrand.h"

/* ---- data ------------------------------------------------------------- */

uint32_t g_utl_rand_seed;                       /* data 0x308484 */

/* ---- text ------------------------------------------------------------- */

/* ROR 0x001041c4 */
void utlRandStep(void)
{
    g_utl_rand_seed = g_utl_rand_seed * 0x769 + 0x7cf9;
}

/* ROR 0x001041e4 */
int utlRand(int n)
{
    utlRandStep();
    /* lwu seed, __muldi3 by the sign-extended n, dsra32: the high word of
     * the 64-bit product. */
    return (int)(((int64_t)g_utl_rand_seed * (int64_t)n) >> 32);   // callcheck: __muldi3 (the 64-bit multiply)
}

/* ROR 0x0010421c */
void utlRandReset(void)
{
    g_utl_rand_seed = 0xc45330af;
}

/* ROR 0x00104230 */
void utlRandSeed(uint32_t theSeed)
{
    g_utl_rand_seed = theSeed;
}

/* ROR 0x0010423c */
int utlRandRange(int lo, int hi)
{
    return utlRand(hi - lo + 1) + lo;
}

/* ROR 0x0010426c */
int utlRandMean(int lo, int hi, int n)
{
    int sum = 0;
    for (int i = n; i > 0; i--) {
        sum += utlRand(hi - lo + 1);
    }
    if (n == 0) {
        return lo;      /* PC: the ROM traps */
    }
    return sum / n + lo;
}

/* ROR 0x001042ec */
int utlRandPeakLo(int lo, int hi, int n)
{
    int range = hi - lo;
    int sum   = 0;
    for (int i = n; i > 0; i--) {
        sum += utlRand(range * 2 + 1);
    }
    if (n == 0) {
        return lo;      /* PC: the ROM traps */
    }
    int mean = sum / n;
    int d    = mean - range;
    if (mean < range) {
        d = range - mean;
    }
    return lo + d;
}

/* ROR 0x00104394 */
int utlRandPeakHi(int lo, int hi, int n)
{
    int range = hi - lo;
    int sum   = 0;
    for (int i = n; i > 0; i--) {
        sum += utlRand(range * 2 + 1);
    }
    if (n == 0) {
        return lo;      /* PC: the ROM traps */
    }
    int mean = sum / n;
    if (range < mean) {
        mean = range * 2 - mean;
    }
    return mean + lo;
}

/* ROR 0x00104438 */
int utlRandPeak(int lo, int hi, int peak, int n)
{
    if (peak < utlRandRange(lo, hi)) {
        return utlRandPeakHi(lo, peak, n);
    }
    return utlRandPeakLo(peak, hi, n);
}

/* ROR 0x001044b8 */
float utlRandRangeF(float lo, float hi)
{
    float range = hi - lo;
    float steps = range * 65536.0f;
    int   r     = utlRandRange(0, (int)steps);      /* cvt.w.s truncates */
    if (r == 0) {
        return 0.0f;
    }
    return ((float)r / steps) * range + lo;
}
