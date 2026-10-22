/* ==========================================================================
 *  utl/utlmisc.cpp
 *
 *  Small utilities and the game heap. See utlmisc.h.
 *
 *  The game heap is a BedMemory over one big malloc: utlMemInit probes the
 *  largest block newlib's malloc still gives (from what is left of the
 *  32 MB below the stack) and hands that size to BedMemory::create. Every
 *  wrapper below goes to that one heap; main installs utlMemAlloc,
 *  utlMemRealloc and utlMemFree as rxSys's memory hooks.
 * ======================================================================== */
#include "utl/utlmisc.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>

#include "bed/bedmemory.h"
#include "common/ror_assert.h"

/* ---- data ------------------------------------------------------------- */

static int s_frameCount;                        /* data 0x295f00 */
BedMemory g_utlMemory;                          /* bss 0x6e9800: the game heap */

/* utlMemInit's first probe: (0x01ffeff0 - (0x828e94 + 0x81000)) & ~0xff,
 * the RAM between the end of the ELF's bss (0x828e94) plus 0x81000 and
 * the top of the stack. The host's malloc meets it at once. */
#define UTL_MEM_PROBE_SIZE 0x1755100

/* ---- text ------------------------------------------------------------- */

/* ROR 0x00105650 */
int utlIsTccName(const char *theName)
{
    ROR_ASSERT(theName);    // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    if (theName == NULL) {
        return 0;   /* PC: the ROM's handler hangs here; do not walk NULL */
    }

    for (const char *p = theName; *p != '\0'; p++) {
        if (p[0] == '_' && p[1] == 't' && p[2] == 'c' && p[3] == 'c') {
            return 1;
        }
    }
    return 0;
}

/* ROR 0x001056f8 */
int utlFtoi15(float value)
{
    /* vftoi15: value * 2^15, truncated toward zero; the VU saturates where
     * a host cast is undefined. */
    float fixed = value * 32768.0f;
    if (fixed >= 2147483647.0f) {
        return INT_MAX;
    }
    if (fixed <= -2147483648.0f) {
        return INT_MIN;
    }
    if (fixed != fixed) {
        return 0;   /* PC: NaN; the VU has none */
    }
    return (int)fixed;
}

/* ROR 0x00105710 */
char *utlSkipSpace(char *p)
{
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    return p;
}

/* ROR 0x00105750 */
char *utlSkipCtrl(char *p)
{
    /* control characters 0x01..0x1f and DEL: the line ends */
    while ((unsigned char)(*p - 1) < 0x1f || *p == 0x7f) {
        p++;
    }
    return p;
}

/* ROR 0x001057a8 */
char *utlSkipSymbol(char *p)
{
    for (;;) {
        unsigned char c = (unsigned char)*p;
        if ((unsigned char)(c - '0') < 10 || (unsigned char)(c - 'A') < 26 ||
            (unsigned char)(c - 'a') < 26 || c == '_') {
            p++;
            continue;
        }
        return p;
    }
}

/* ROR 0x00105838 */
char *utlNextLine(char *p)
{
    /* the rest of the line: printable characters and tabs */
    while ((unsigned char)*p > 0x1f || *p == '\t') {
        p++;
    }
    if (*p == '\0') {
        return NULL;
    }
    p = utlSkipCtrl(p);
    if (*p == '\0') {
        return NULL;
    }
    return p;
}

/* ROR 0x001058a0
 * Parses up to max lines of "name value" into out and returns how many
 * records it wrote. The text is cut in place: a NUL after each name and
 * after a quoted string. A value that is none of the four forms (a name
 * running straight into the line end, a letter) leaves the record's type
 * as it was. */
int utlParseParam(UtlParam *out, int max, char *text)
{
    int count = 0;

    while (text != NULL) {
        char *next = utlNextLine(text);
        char *name = utlSkipSpace(text);
        out->m_name = name;

        char *end = utlSkipSymbol(name);
        if (*end == '\0') {
            out->m_type = UTL_PARAM_NONE;
        } else {
            char *value = utlSkipSpace(end);
            *end = '\0';

            if (*value == '"') {
                char *str = value + 1;
                char *q = str;
                while (*q != '"' && *q != '\0') {
                    q++;
                }
                *q = '\0';
                out->m_type = UTL_PARAM_STRING;
                out->m_value.s = str;
            } else if (*value == '$') {
                int hex = 0;
                for (value++; isxdigit((unsigned char)*value); value++) {
                    int c = *value;
                    if (c >= 'a') {
                        hex = hex * 16 + c - 'a' + 10;
                    } else if (c >= 'A') {
                        hex = hex * 16 + c - 'A' + 10;
                    } else {
                        hex = hex * 16 + c - '0';
                    }
                }
                out->m_type = UTL_PARAM_INT;
                out->m_value.i = hex;
            } else if ((unsigned char)(*value - '0') <= 9) {
                int   whole = 0;
                float real  = 0.0f;
                float scale = 0.0f;     /* 0 until a '.' is seen */

                for (; isdigit((unsigned char)*value) || *value == '.'; value++) {
                    int c = *value;
                    if (c == '.') {
                        /* a second '.' starts the fraction again */
                        scale = 10.0f;
                        real  = (float)whole;
                    } else if (scale == 0.0f) {
                        whole = whole * 10 + c - '0';
                    } else {
                        real  += (float)(c - '0') / scale;
                        scale *= 10.0f;
                    }
                }

                if (scale == 0.0f) {
                    out->m_type = UTL_PARAM_INT;
                    out->m_value.i = whole;
                } else {
                    out->m_type = UTL_PARAM_FLOAT;
                    out->m_value.f = real;
                }
            }
        }

        count++;
        out++;
        text = next;
        if (count >= max) {
            break;
        }
    }
    return count;
}

/* ---- frame counter ---------------------------------------------------- */

/* ROR 0x00105b3c */
void utlIncFrameCount(void)
{
    s_frameCount++;
}

/* ---- operator new / delete -------------------------------------------- */

/* ROR 0x00105b54 */
void *utlOperatorNew(size_t size)
{
    return utlMemAlloc((int)size);
}

/* ROR 0x00105b68 */
void *utlOperatorNewDebug(size_t size, const char *filename, int line, int id)
{
    return utlMemAllocDebug((int)size, filename, line, id);
}

/* ROR 0x00105b7c */
void *utlOperatorNewArray(size_t size)
{
    return utlMemAlloc((int)size);
}

/* ROR 0x00105b90 */
void *utlOperatorNewArrayDebug(size_t size, const char *filename, int line, int id)
{
    return utlMemAllocDebug((int)size, filename, line, id);
}

/* ROR 0x00105ba4 */
void utlOperatorDelete(void *ptr)
{
    if (ptr != NULL) {
        utlMemFree(ptr);
    }
}

/* ROR 0x00105bc4 */
void utlOperatorDeleteArray(void *ptr)
{
    if (ptr != NULL) {
        utlMemFree(ptr);
    }
}

/* ---- the game heap ---------------------------------------------------- */

/* ROR 0x00105be4 */
void utlMemInit(void)
{
    int size = UTL_MEM_PROBE_SIZE;
    void *block;

    for (;;) {
        /* the largest block malloc gives, in 0x100 steps... */
        if (size > 0) {
            block = malloc(size);
            if (block == NULL) {
                size -= 0x100;
                continue;
            }
            free(block);
        }
        /* ...and again, in case the free did not give it back whole */
        block = malloc(size);
        if (block != NULL) {
            break;
        }
        size = UTL_MEM_PROBE_SIZE;
    }
    free(block);

    g_utlMemory.create(size, 0);
}

/* ROR 0x00105c80 */
void utlMemTerm(void)
{
    g_utlMemory.destroy();
}

/* ROR 0x00105ca0 */
void *utlMemAlloc(int size)
{
    return g_utlMemory.alloc(size, -1);
}

/* ROR 0x00105cc4
 * BedMemory::alloc's alignment index: 0x40 -> 1, 0x80 -> 2, anything else
 * (0x20 and the rest) 0. */
int utlMemAlignIndex(int align)
{
    if (align == 0x40) {
        return 1;
    }
    if (align == 0x80) {
        return 2;
    }
    return 0;
}

/* ROR 0x00105d08 */
void *utlMemAllocAlign(int size, int align)
{
    return g_utlMemory.alloc(size, utlMemAlignIndex(align));
}

/* ROR 0x00105d40 */
void *utlMemAllocDebug(int size, const char *filename, int line, int id)
{
    return g_utlMemory.allocDebug(size, filename, line, id, -1);
}

/* ROR 0x00105d78 */
void *utlMemAllocAlignDebug(int size, int align, const char *filename, int line, int id)
{
    return g_utlMemory.allocDebug(size, filename, line, id, utlMemAlignIndex(align));
}

/* ROR 0x00105de0 */
void *utlMemRealloc(void *ptr, int size)
{
    return g_utlMemory.realloc(ptr, size);
}

/* ROR 0x00105e08 */
void utlMemFree(void *ptr)
{
    g_utlMemory.free(ptr);
}

/* ROR 0x00105e2c */
void utlMemCheck(void)
{
    g_utlMemory.check(0);
}
