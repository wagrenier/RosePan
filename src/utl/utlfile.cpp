// utl/utlfile.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "utl/utlfile.h"

#include <stddef.h>

#include "common/ror_assert.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// ──────────────────────────────────────────────────────────────────────
// Copies theSrc (NULL: nothing) into theDest, cut to theSize - 1 chars,
// always terminated.
// callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
// ROR 0x00105e50

void utlStrCopy(char *theDest, int theSize, const char *theSrc)
{
    ROR_ASSERT(theDest); /* 15 */
    ROR_ASSERT(2 <= theSize && theSize <= 1024); /* 16 */
    char *d = theDest;
    if (theSrc != NULL) {
        for (; d < theDest + theSize - 1 && *theSrc != '\0'; theSrc++)
            *d++ = *theSrc;
    }
    *d = '\0';
}

// ──────────────────────────────────────────────────────────────────────
// utlStrCopy of a then b (either may be NULL), together cut to theSize - 1.
// callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
// ROR 0x00105f0c

void utlStrCopy2(char *theDest, int theSize, const char *a, const char *b)
{
    ROR_ASSERT(theDest); /* 36 */
    ROR_ASSERT(2 <= theSize && theSize <= 1024); /* 37 */
    char *d = theDest;
    if (a != NULL) {
        for (; d < theDest + theSize - 1 && *a != '\0'; a++)
            *d++ = *a;
    }
    if (b != NULL) {
        for (; d < theDest + theSize - 1 && *b != '\0'; b++)
            *d++ = *b;
    }
    *d = '\0';
}
