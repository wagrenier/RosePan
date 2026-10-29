/* ==========================================================================
 *  debug/mimu_debug.cpp
 *
 *  The assert handler, the float / vector / matrix traces and the float
 *  formatter (pinned 0x215a30..0x216154; RoR's own source string): see
 *  mimu_debug.h. The traces format and print nothing in the retail build;
 *  the port keeps that, so a trace call costs its formatting and shows
 *  nothing, as on the PS2.
 * ======================================================================== */
#include "debug/mimu_debug.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common/ror_assert.h"

/* data 0x301958: the decimal digits n binary fraction digits need
 * (about n * log10(2)); dbgSplitFloat copies it to the stack. */
static const short s_dbgFracDigits[32] = {
    0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5,
    5, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10,
};

/* data 0x3019e0: dbgFloatToStr's powers of ten, copied to the stack. */
static const int s_dbgPow10[10] = {
    1000000000, 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1,
};

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00215a30; callcheck: scePrintf (printf on the host)

void dbgAssertFailed(const char *file, int line, const char *expr)
{
    printf("Assertion Failed: %s(%d)   %s\n", file, line, expr);
    // PORT: the ROM loops for ever here (common/ror_assert.h).
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00215a70

void dbgTraceMatrix(const float *m)
{
    char x[256], y[256], z[256], w[256];

    for (int row = 0; row < 4; row++, m += 4) {
        dbgFloatToStr(m[0], x, 3);
        dbgFloatToStr(m[1], y, 3);
        dbgFloatToStr(m[2], z, 3);
        dbgFloatToStr(m[3], w, 3);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00215ae8

void dbgTraceFloat4(float x, float y, float z, float w)
{
    char sx[256], sy[256], sz[256], sw[256];

    dbgFloatToStr(x, sx, 3);
    dbgFloatToStr(y, sy, 3);
    dbgFloatToStr(z, sz, 3);
    dbgFloatToStr(w, sw, 3);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00215b5c

void dbgTraceVector3(float x, float y, float z)
{
    char sx[256], sy[256], sz[256];

    dbgFloatToStr(x, sx, 3);
    dbgFloatToStr(y, sy, 3);
    dbgFloatToStr(z, sz, 3);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00215bb4

void dbgTraceVector4(const float *v)
{
    char x[256], y[256], z[256], w[256];

    dbgFloatToStr(v[0], x, 3);
    dbgFloatToStr(v[1], y, 3);
    dbgFloatToStr(v[2], z, 3);
    dbgFloatToStr(v[3], w, 3);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00215c14

void dbgTraceVector4Alt(const float *v)
{
    char x[256], y[256], z[256], w[256];

    dbgFloatToStr(v[0], x, 3);
    dbgFloatToStr(v[1], y, 3);
    dbgFloatToStr(v[2], z, 3);
    dbgFloatToStr(v[3], w, 3);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00215c74

void dbgSplitFloat(float value, DbgFloat *out)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));

    out->m_int = 0;
    out->m_frac = 0;
    int exp = (int)(bits >> 23) & 0xff;
    out->m_sign = (int)(bits >> 31);
    out->m_digits = 0;
    out->m_shift = 0;
    if (exp == 0)
        return;

    /* The mantissa with its hidden bit, trailing zero bits dropped; e is
     * the power of two it is scaled by. */
    int mant = (int)((bits & 0x7fffff) | 0x800000);
    int e = exp - 150;
    while ((mant & 1) == 0) {
        mant >>= 1;
        e++;
    }

    if (e == 0) {
        out->m_int = mant;
    } else if (e > 0) {
        out->m_int = mant;
        for (int i = 0; i < e; i++) {
            out->m_int = (int)((uint32_t)out->m_int << 1);
            if (out->m_int < 0) {
                out->m_sign = 2;
                break;
            }
        }
    } else {
        int n = -e;
        if (n >= 32) {
            out->m_sign = 3;
            return;
        }
        int whole = mant >> n;
        out->m_int = whole;
        out->m_frac = mant - (whole << n);
        out->m_shift = (short)n;
        out->m_digits = s_dbgFracDigits[n];
        return;
    }
    out->m_shift = 0;
    out->m_frac = 0;
    out->m_digits = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00215e1c; callcheck: dbgAssertFailed (inside ROR_ASSERT)

char *dbgFloatToStr(float value, char *theValStr, int prec)
{
    (void)prec;
    ROR_ASSERT(theValStr);  /* 262 */
    DbgFloat f;
    dbgSplitFloat(value, &f);

    if (f.m_sign == 3) {
        strcpy(theValStr, "--UNDERFLOW--");
        return theValStr;
    }
    if (f.m_sign == 2) {
        strcpy(theValStr, "# OVERFLOW #");
        return theValStr;
    }

    int n = 1;
    theValStr[0] = f.m_sign == 0 ? ' ' : '-';

    /* The integer part, leading zeros dropped; its last digit always. */
    int rem = f.m_int;
    int started = 0;
    for (int i = 0; s_dbgPow10[i] > 1; i++) {
        int d = rem / s_dbgPow10[i];
        rem %= s_dbgPow10[i];
        if (d > 0) {
            theValStr[n++] = (char)('0' + d);
            started++;
        } else if (started > 0) {
            theValStr[n++] = '0';
        }
    }
    theValStr[n++] = (char)('0' + rem);
    theValStr[n++] = '.';

    /* The binary fraction to m_digits decimal digits: times ten per digit,
     * the shift taken off four bits at a time between them. */
    int frac = f.m_frac;
    int digits = f.m_digits;
    int shift = f.m_shift;
    while (digits > 0 || shift > 0) {
        if (digits > 0) {
            digits--;
            frac *= 10;
        }
        if (shift > 0) {
            int k = shift < 5 ? shift : 4;
            shift -= k;
            frac >>= k;
        }
    }
    /* ... then scaled to three decimals. */
    if (f.m_digits > 3)
        frac /= s_dbgPow10[12 - f.m_digits];
    else if (f.m_digits < 3)
        frac *= s_dbgPow10[6 + f.m_digits];

    for (int i = 7; s_dbgPow10[i] > 1; i++) {
        theValStr[n++] = (char)('0' + frac / s_dbgPow10[i]);
        frac %= s_dbgPow10[i];
    }
    theValStr[n] = (char)('0' + frac);
    theValStr[n + 1] = '\0';
    return theValStr;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0021614c

void dbgUnused21614c(void)
{
}
