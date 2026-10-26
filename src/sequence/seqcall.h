/* ==========================================================================
 *  sequence/seqcall.h
 *
 *  The script native functions (sequence/seqcall.cpp, 0x2053f4..0x20e3d8):
 *  a table of 512 functions (data 0x2d90f8) the sequence VM calls when a
 *  script does a jal below the image (target 0x144 + n * 0x40 calls entry
 *  n, VM 0x214c24), with the script's a0..a3, t0, t1. 471 entries are an
 *  empty "return 0"; entry 18 is the system call, a switch on a0.
 *
 *  Script registers are EE words that hold EE addresses in the ROM; on the
 *  host the VM keeps them as intptr_t (open-items "Decisions"), so pointer
 *  arguments are host addresses.
 * ======================================================================== */
#ifndef _SEQUENCE_SEQCALL_H
#define _SEQUENCE_SEQCALL_H

#include <stdint.h>

typedef intptr_t (*SeqFunc)(intptr_t a0, intptr_t a1, intptr_t a2, intptr_t a3,
                            intptr_t t0, intptr_t t1);

/* What a native function returns besides a value for v0 (the VM 0x214c24
 * tests the low word). */
#define SEQ_RET_WAIT   ((intptr_t)(int)0xcccccccc)  /* run the call again next frame */
#define SEQ_RET_YIELD  ((intptr_t)(int)0xcccccccd)  /* continue after the call next frame */
#define SEQ_RET_JUMP   ((intptr_t)(int)0xccccccce)  /* continue at Sequence::jumpTo's target */
#define SEQ_RET_RETURN ((intptr_t)(int)0xccccccc9)  /* continue at the saved return address */
#define SEQ_RET_END    ((intptr_t)(int)0xccccccca)  /* end the thread */

#define SEQ_FUNC_COUNT 512

extern const SeqFunc g_seqFuncTable[SEQ_FUNC_COUNT];  /* data 0x2d90f8 */

/* The entries with code (the others are seqFuncNone). */
intptr_t seqFuncNone(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
intptr_t seqFunc000(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x002053f4
intptr_t seqFunc001(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020541c
intptr_t seqFunc002(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020544c
intptr_t seqFunc003(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00205564
intptr_t seqFunc004(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00205570
intptr_t seqFunc005(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x002058f0
intptr_t seqFunc006(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00205928
intptr_t seqFunc007(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00205b18
intptr_t seqFunc009(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00205b44
intptr_t seqFunc012(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00205b88
intptr_t seqFunc014(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00206614
intptr_t seqFunc016(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00206640
intptr_t seqFunc018(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020694c
intptr_t seqFunc019(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x002076cc
intptr_t seqFunc020(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00207ae8
intptr_t seqFunc021(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020810c
intptr_t seqFunc023(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020839c
intptr_t seqFunc026(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00208518
intptr_t seqFunc027(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00208640
intptr_t seqFunc028(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00208b10
intptr_t seqFunc029(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00208b98
intptr_t seqFunc032(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00208dc4
intptr_t seqFunc033(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00208e24
intptr_t seqFunc036(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x002090a0
intptr_t seqFunc038(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x002090e0
intptr_t seqFunc039(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00209270
intptr_t seqFunc040(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00209344
intptr_t seqFunc100(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00209528
intptr_t seqFunc106(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x002098e0
intptr_t seqFunc110(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x002099bc
intptr_t seqFunc151(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x00209d4c
intptr_t seqFunc152(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020a368
intptr_t seqFunc155(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020a7d0
intptr_t seqFunc156(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020abfc
intptr_t seqFunc200(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020b3c0
intptr_t seqFunc202(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020b528
intptr_t seqFunc211(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020bd08
intptr_t seqFunc212(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020c4c8
intptr_t seqFunc252(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020c6fc
intptr_t seqFunc253(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020d428
intptr_t seqFunc281(intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t); // ROR 0x0020df98

/* Native 281's helpers (281 itself is still a stub). */
intptr_t seqSectorFlags(int op, int key, unsigned int bits);             // ROR 0x0020de34
int seqGetIdPair(unsigned int no, int *id, int *flag);                   // ROR 0x0020dea8

#endif /* _SEQUENCE_SEQCALL_H */
