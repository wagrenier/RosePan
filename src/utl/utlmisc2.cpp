/* ==========================================================================
 *  utl/utlmisc2.cpp
 *
 *  Angle and string helpers. See utlmisc2.h.
 *
 *  The two headings keep the ROM's arithmetic: acos in float, then each
 *  wrap into -pi..pi compared and corrected in double (the soft-float
 *  dpcmp / dpadd / dpsub calls) with the result rounded back to float.
 * ======================================================================== */
#include "utl/utlmisc2.h"

#include <math.h>
#include <string.h>

static const double UTL_PI      = 3.141592653589793;    /* 0x400921fb54442d18 */
static const double UTL_TWO_PI  = 6.283185307179586;    /* 0x401921fb54442d18 */
static const double UTL_HALF_PI = 1.5707963267948966;   /* 0x3ff921fb54442d18 */

/* One wrap of a into -pi..pi, in double. */
static float utlWrapPi(float a)
{
    if ((double)a < -UTL_PI)
        a = (float)((double)a + UTL_TWO_PI);
    if ((double)a > UTL_PI)
        a = (float)((double)a - UTL_TWO_PI);
    return a;
}

// ROR 0x00217f10 // callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (the soft-float wraps, in utlWrapPi)

float utlAtan2(float x, float z)
{
    float len = sqrtf(x * x + z * z);

    if (len == 0.0f)
        return 0.0f;
    float a = acosf(x / len);
    if (z < 0.0f)
        a = -a;
    a = utlWrapPi(a);
    return utlWrapPi((float)((double)-a + UTL_HALF_PI));
}

// ROR 0x002180e4 // callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (the soft-float wraps, in utlWrapPi)

float utlDirDist(float x, float z, float *dist)
{
    float len = sqrtf(x * x + z * z);

    *dist = len;
    if (len == 0.0f)
    {
        *dist = 0.0f;
        return 0.0f;
    }
    float a = acosf(x / len);
    if (z < 0.0f)
        a = -a;
    a = utlWrapPi(a);
    return utlWrapPi((float)((double)-a + UTL_HALF_PI));
}

// ROR 0x002182c4

char *utlStrToUpper(char *s)
{
    char buf[256];
    const char *p = s;
    char *d = buf;

    while (*p != '\0')
    {
        unsigned char c = (unsigned char)*p++;
        *d++ = (unsigned char)(c - 'a') < 26 ? (char)(c - 0x20) : (char)c;
    }
    *d = '\0';
    strncpy(s, buf, sizeof(buf));
    return s;
}

/* The ROM tests newlib's _ctype_ (0x3079a8 + 1) bit _U with the signed char
 * as index; that bit is set for A..Z only, so the host compares the range
 * (Shift-JIS bytes, negative there, are never upper). */
// ROR 0x00218338

void utlStrToLower(char *dst, const char *src)
{
    char c;

    while ((c = *src++) != '\0')
    {
        if (c >= 'A' && c <= 'Z')
            c += ' ';
        *dst++ = c;
    }
    *dst = '\0';
}

// ROR 0x0021838c

void utlStrNCopy(char *dst, const char *src, int n)
{
    int i = 0;

    if (*src != '\0')
    {
        i = 1;
        if (n > 0)
        {
            for (;;)
            {
                *dst++ = *src++;
                if (*src == '\0')
                    break;
                int more = i < n;
                i++;
                if (!more)
                    break;
            }
        }
    }
    if (i < n)
        *dst++ = '\0';
    if (i + 1 < n)
        *dst = '\0';
}

// ROR 0x002183f8

void utlMemCopyBytes(char *dst, const char *src, int n)
{
    if (n <= 0)
        return;
    do
    {
        *dst++ = *src++;
    } while (--n != 0);
}

// ROR 0x00218424

float utlFmodf(float x, float m)
{
    return x - m * (float)(int)(x / m);
}

// ROR 0x00218450

float utlFmodPi(float x)
{
    return utlFmodf(x, 3.1415925f);
}

// ROR 0x00218470

void rorDebugPrintf(const char *fmt, ...)
{
    (void)fmt;
}

// ROR 0x002184b8

void utlNullFunc(void)
{
}
