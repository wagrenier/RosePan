/* ==========================================================================
 *  debug/mimu_debug.h
 *
 *  debug/mimu_debug.cpp (RoR's source string at 0x301998; pinned
 *  0x215a30..0x216154): the assert handler most of the game calls, the
 *  debug traces of floats / vectors / matrices, and the float -> text
 *  formatter they use. The retail build keeps the formatting and drops the
 *  printing, so every trace formats into stack buffers and shows nothing.
 * ======================================================================== */
#ifndef _DEBUG_MIMU_DEBUG_H
#define _DEBUG_MIMU_DEBUG_H

#include <stddef.h>

/* An IEEE single split into the parts dbgFloatToStr prints. EE layout
 * (stack data, 0x10 bytes). */
struct DbgFloat
{
    int   m_sign;               // ROR 0x00: 0 positive, 1 negative; 2 overflow, 3 underflow
    int   m_int;                // ROR 0x04: integer part
    int   m_frac;               // ROR 0x08: fraction, m_shift binary digits
    short m_shift;              // ROR 0x0c: binary digits of m_frac
    short m_digits;             // ROR 0x0e: decimal digits m_shift binary digits hold
};
static_assert(sizeof(DbgFloat) == 0x10, "DbgFloat");
static_assert(offsetof(DbgFloat, m_int) == 0x04, "m_int");
static_assert(offsetof(DbgFloat, m_frac) == 0x08, "m_frac");
static_assert(offsetof(DbgFloat, m_shift) == 0x0c, "m_shift");
static_assert(offsetof(DbgFloat, m_digits) == 0x0e, "m_digits");

/* Prints "Assertion Failed: file(line)   expr". The ROM then loops for
 * ever; the host returns (common/ror_assert.h). ROR_ASSERT expands to
 * the same print inline, so the port's code does not call it. */
void  dbgAssertFailed(const char *file, int line, const char *expr);     // ROR 0x00215a30
/* Format a 4x4 matrix's rows (m[0..15]) and print nothing. */
void  dbgTraceMatrix(const float *m);                                    // ROR 0x00215a70
/* Format four floats and print nothing. */
void  dbgTraceFloat4(float x, float y, float z, float w);                // ROR 0x00215ae8
/* Format three floats and print nothing. */
void  dbgTraceVector3(float x, float y, float z);                        // ROR 0x00215b5c
/* Format v[0..3] and print nothing. */
void  dbgTraceVector4(const float *v);                                   // ROR 0x00215bb4
/* The same code as dbgTraceVector4; no callers. */
void  dbgTraceVector4Alt(const float *v);                                // ROR 0x00215c14
/* Split value's bits into out (see DbgFloat). */
void  dbgSplitFloat(float value, DbgFloat *out);                         // ROR 0x00215c74
/* Write value to theValStr as sign (' ' or '-'), the integer digits, '.'
 * and three decimals ("--UNDERFLOW--" / "# OVERFLOW #" out of range);
 * returns theValStr. prec is passed (always 3) but not read. */
char *dbgFloatToStr(float value, char *theValStr, int prec);             // ROR 0x00215e1c
/* Empty; no references. */
void  dbgUnused21614c(void);                                             // ROR 0x0021614c

#endif /* _DEBUG_MIMU_DEBUG_H */
