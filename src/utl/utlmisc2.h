/* ==========================================================================
 *  utl/utlmisc2.h
 *
 *  Angle and string helpers (utl/utlmisc2.cpp, ROR 0x217f10..0x2184c0, the
 *  port's file name: no source string names it). The run sits between the
 *  debug menu (DBGFLAG.BIN, 0x216ad0..0x217f10) and the i3d texture library
 *  0x2184c0: two headings in -pi..pi from an (x, z) vector, upper / lower
 *  case copies, a padding strncpy, a float modulo, and the empty varargs
 *  debug print game code still calls.
 * ======================================================================== */
#ifndef _UTL_UTLMISC2_H
#define _UTL_UTLMISC2_H

/* The heading of (x, z): pi/2 - acos(x / len), negated acos for z < 0, each
 * step wrapped once into -pi..pi in double as the ROM does; 0 for a zero
 * vector. */
float utlAtan2(float x, float z);                                        // ROR 0x00217f10
/* utlAtan2 that also stores the length into *dist (0 for a zero vector). */
float utlDirDist(float x, float z, float *dist);                         // ROR 0x002180e4
/* Upper-cases a..z of s in place (through a 256-byte copy); returns s. */
char *utlStrToUpper(char *s);                                            // ROR 0x002182c4
/* Copies src to dst with upper case letters (the ctype table's bit 1)
 * lowered. */
void  utlStrToLower(char *dst, const char *src);                         // ROR 0x00218338
/* Copies src into dst, at most n chars; zero-terminates while room is
 * left (n = strlen: no terminator). */
void  utlStrNCopy(char *dst, const char *src, int n);                    // ROR 0x0021838c
/* Copies n bytes src -> dst, one at a time. */
void  utlMemCopyBytes(char *dst, const char *src, int n);                // ROR 0x002183f8
/* x - (int)(x / m) * m: the remainder with the quotient truncated. */
float utlFmodf(float x, float m);                                        // ROR 0x00218424
/* utlFmodf(x, pi) with the ROM's pi 0x40490fda. */
float utlFmodPi(float x);                                                // ROR 0x00218450
/* The retail build's debug print: spills its arguments and returns. */
void  rorDebugPrintf(const char *fmt, ...);                              // ROR 0x00218470
/* An empty function (no callers). */
void  utlNullFunc(void);                                                 // ROR 0x002184b8

#endif /* _UTL_UTLMISC2_H */
