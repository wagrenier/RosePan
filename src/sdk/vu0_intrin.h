/* ==========================================================================
 *  vu0_intrin.h  (EE VU0 fixed<->float helpers — PC-port shim)
 *
 *  On the EE the VU0 "macro-mode" names (`_lqc2`, `_vmulabc`, `_vmaddbc`, ...)
 *  are inline COP2 instructions emitted by the GCC EE toolchain, not a library.
 *  There is no PS2 toolchain here, so those inline-asm call sites are rewritten
 *  to equivalent scalar C directly in the reconstructed sources (matching the
 *  Fatal Frame 1 PC port's approach).
 *
 *  The only VU0 names kept here are the ones the reconstruction uses in plain
 *  FUNCTION form — the fixed-point conversions `_ftoiN(int *out, float *in)` —
 *  which have a well-defined scalar meaning: out[k] = (int)(in[k] * 2^N), the
 *  4.N fixed formats the GS/VIF expect.
 *
 *  The fundamental quadword vector/matrix types also live here so they are
 *  ambient wherever the sceVu0* library types are referenced.
 *
 *  Reconstructed from the Feb 6 2004 prototype (SLES_523.84).
 * ======================================================================== */

#ifndef _VU0_INTRIN_H
#define _VU0_INTRIN_H

/* ---- fundamental vector/matrix types ------------------------------------ */
/* 16-byte aligned as in Sony's libvu0.h (the EE's lqc2/sqc2 need it), so a
   struct member of these types lands at the same offset, and the struct gets
   the same size and tail padding, as on the EE.  Sizes are unchanged (16 / 64);
   only alignment differs from a plain float[4].  Both host compilers honour
   the alignment on an array typedef. */
#if defined(_MSC_VER) && !defined(__clang__)
typedef __declspec(align(16)) float sceVu0FVECTOR[4];    /* xyzw */
typedef __declspec(align(16)) int   sceVu0IVECTOR[4];
typedef __declspec(align(16)) float sceVu0FMATRIX[4][4];
#else
typedef float sceVu0FVECTOR[4]    __attribute__((aligned(16)));    /* xyzw */
typedef int   sceVu0IVECTOR[4]    __attribute__((aligned(16)));
typedef float sceVu0FMATRIX[4][4] __attribute__((aligned(16)));
#endif

#ifdef __cplusplus
static_assert(sizeof(sceVu0FVECTOR) == 16 && alignof(sceVu0FVECTOR) == 16, "sceVu0FVECTOR: 16 bytes, 16-aligned");
static_assert(sizeof(sceVu0IVECTOR) == 16 && alignof(sceVu0IVECTOR) == 16, "sceVu0IVECTOR: 16 bytes, 16-aligned");
static_assert(sizeof(sceVu0FMATRIX) == 64 && alignof(sceVu0FMATRIX) == 16, "sceVu0FMATRIX: 64 bytes, 16-aligned");
#endif

/* ---- VU0 pipeline/status ops (no math; no host equivalent) -------------- */
/* The host has no VU0 coprocessor pipeline, so a NOP is literally nothing and
   the clip/status registers read back "nothing pending".  These are control
   ops, not the arithmetic kernels (those are rewritten to scalar C in place). */
#ifndef _vnop
#define _vnop()      ((void)0)
#endif
#ifndef _vclip
#define _vclip(a, b) ((void)0)          /* clip-flag pipeline: nothing to do  */
#endif
#ifndef _cfc2
#define _cfc2(...)   (0u)               /* clip flags / VPU-STAT: idle/clear   */
#endif

#ifdef __cplusplus

inline int getCopCondition(int, int) { return 0; }

/* ---- fixed<->float conversions (function form: out, in as pointers) ----- */
/* out[k] = (int)(in[k] * 2^N) — the 4.N fixed formats the GS/VIF expect.     */
inline void _ftoi0 (int *o, const float *i){ for (int k=0;k<4;k++) o[k]=(int)(i[k]);          }
inline void _ftoi4 (int *o, const float *i){ for (int k=0;k<4;k++) o[k]=(int)(i[k]*16.0f);    }
inline void _ftoi12(int *o, const float *i){ for (int k=0;k<4;k++) o[k]=(int)(i[k]*4096.0f);  }
inline void _ftoi15(int *o, const float *i){ for (int k=0;k<4;k++) o[k]=(int)(i[k]*32768.0f); }

#endif /* __cplusplus */

#endif /* _VU0_INTRIN_H */
