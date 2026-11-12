/* ==========================================================================
 *  utl/utlfile.h
 *
 *  utl/utlfile.cpp (0x105e50..0x106010): bounded string copies for file
 *  names (both assert theDest and 2 <= theSize && theSize <= 1024, and
 *  always terminate the result).
 * ======================================================================== */
#ifndef _UTL_UTLFILE_H
#define _UTL_UTLFILE_H

/* Copies theSrc (NULL = nothing) into theDest, at most theSize - 1 chars. */
void utlStrCopy(char *theDest, int theSize, const char *theSrc);         // ROR 0x00105e50
/* Copies a then b (either NULL = nothing) into theDest, at most theSize - 1
 * chars in all. */
void utlStrCopy2(char *theDest, int theSize, const char *a, const char *b); // ROR 0x00105f0c

#endif /* _UTL_UTLFILE_H */
