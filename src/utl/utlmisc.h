/* ==========================================================================
 *  utl/utlmisc.h
 *
 *  Small utilities (utl/utlmisc.cpp, ROR 0x105650..0x105e50; the assert at
 *  0x105650 names the file, the rest of the run up to utlfile.cpp is taken
 *  as the same file): a texture-name test, float to fixed point, a
 *  "name value" text parser, the frame counter, and the game heap -- a
 *  BedMemory (bss 0x6e9800) behind the game's operator new / delete and
 *  the memory hooks main hands to rxSys.
 * ======================================================================== */
#ifndef _UTL_UTLMISC_H
#define _UTL_UTLMISC_H

#include <stddef.h>

/* utlParseParam's value kinds (the record's type word). */
enum UTL_PARAM_TYPE
{
    UTL_PARAM_NONE   = 0,       /* a name with nothing after it */
    UTL_PARAM_INT    = 1,       /* decimal, or hex after '$' */
    UTL_PARAM_FLOAT  = 2,       /* digits with a '.' */
    UTL_PARAM_STRING = 3,       /* in double quotes */
};

/* One parsed "name value" line (12 bytes on the EE: name +0x0, type +0x4,
 * value +0x8). Names and strings point into the parsed text, which the
 * parser cuts with NULs. layout: host -- holds pointers. */
struct UtlParam
{
    char *m_name;               // ROR 0x00
    int   m_type;               // ROR 0x04: UTL_PARAM_TYPE
    union
    {
        int   i;
        float f;
        char *s;
    } m_value;                  // ROR 0x08
};

int   utlIsTccName(const char *theName);                                 // ROR 0x00105650
int   utlFtoi15(float value);                                            // ROR 0x001056f8
char *utlSkipSpace(char *p);                                             // ROR 0x00105710
char *utlSkipCtrl(char *p);                                              // ROR 0x00105750
char *utlSkipSymbol(char *p);                                            // ROR 0x001057a8
char *utlNextLine(char *p);                                              // ROR 0x00105838
int   utlParseParam(UtlParam *out, int max, char *text);                 // ROR 0x001058a0
void  utlIncFrameCount(void);                                            // ROR 0x00105b3c

/* The ROM's global operator new / delete (GCC 2 __nw__FUi, __vn__FUi,
 * __dl__FPv, __vd__FPv and two debug overloads nothing calls). PORT: they
 * are not installed as the host's global operators -- a new-expression in
 * reconstructed code uses the host heap (sizes are host sizes, and the
 * platform layer allocates before main creates the game heap). Code that
 * needs the game heap itself calls these or utlMemAlloc. */
void *utlOperatorNew(size_t size);                                       // ROR 0x00105b54
void *utlOperatorNewDebug(size_t size, const char *filename, int line, int id); // ROR 0x00105b68
void *utlOperatorNewArray(size_t size);                                  // ROR 0x00105b7c
void *utlOperatorNewArrayDebug(size_t size, const char *filename, int line, int id); // ROR 0x00105b90
void  utlOperatorDelete(void *ptr);                                      // ROR 0x00105ba4
void  utlOperatorDeleteArray(void *ptr);                                 // ROR 0x00105bc4

void  utlMemInit(void);                                                  // ROR 0x00105be4
void  utlMemTerm(void);                                                  // ROR 0x00105c80
void *utlMemAlloc(int size);                                             // ROR 0x00105ca0
int   utlMemAlignIndex(int align);                                       // ROR 0x00105cc4
void *utlMemAllocAlign(int size, int align);                             // ROR 0x00105d08
void *utlMemAllocDebug(int size, const char *filename, int line, int id); // ROR 0x00105d40
void *utlMemAllocAlignDebug(int size, int align, const char *filename, int line, int id); // ROR 0x00105d78
void *utlMemRealloc(void *ptr, int size);                                // ROR 0x00105de0
void  utlMemFree(void *ptr);                                             // ROR 0x00105e08
void  utlMemCheck(void);                                                 // ROR 0x00105e2c

#endif /* _UTL_UTLMISC_H */
