/* ==========================================================================
 *  i3d/i3dpacket.h
 *
 *  The i3d library's GS packet writers (i3d/i3dpacket.cpp, ROR
 *  0x21b220..0x21c960): the draw-set list helpers, DMA tags, one-register
 *  writes, the blend and Z-clear packets, and one writer per GS primitive
 *  (point / line / triangle / sprite; flat, gouraud, UV- or ST-textured).
 *
 *  Two families of primitive writers:
 *  - the *F writers (0x21b7c8..0x21c0fc) take float vectors and write a
 *    PACKED GIF tag: positions go through the screen transform
 *    (i3dPktSetScreenScale / Offset, then ftoi4), colours are 0..1 scaled
 *    by 255 (alpha 128) or, textured, by 128 (ftoi0), UVs ftoi4;
 *  - the others take values already packed (i3dPackXYZ / RGBAQ / UV / ST)
 *    and write a REGLIST GIF tag with PRIM first.
 *  Every writer prefixes a DMA CNT tag with VIF FLUSHA / DIRECT
 *  (i3dPktDirectTag) and returns the position after its data.
 *
 *  Names are inferred (the library has no symbols); the GS words decide them.
 * ======================================================================== */
#ifndef _I3D_I3DPACKET_H
#define _I3D_I3DPACKET_H

#include "i3d/i3dctx.h"
#include "i3d/i3dlayer.h"
#include "sdk/vu0_intrin.h"

/* ---- GS registers and primitives the writers use ---- */

#define I3D_GS_PRIM    0x00ull
#define I3D_GS_RGBAQ   0x01ull
#define I3D_GS_ST      0x02ull
#define I3D_GS_UV      0x03ull
#define I3D_GS_XYZ2    0x05ull
#define I3D_GS_A_D     0x0eull
#define I3D_GS_NOP     0x0full
#define I3D_GS_ALPHA_1 0x42ull
#define I3D_GS_TEST_1  0x47ull

/* VIF codes of i3dPktDirectTag: FLUSHA, then DIRECT of the qwords. */
#define I3D_VIF_FLUSHA 0x11000000u
#define I3D_VIF_DIRECT 0x50000000u

/* ---- draw sets ---- */

void      i3dDrawSetInsertAfter(I3dDrawSet *set, I3dDrawSet *at);       // ROR 0x0021b220
void      i3dDrawSetMoveAfter(I3dDrawSet *set, I3dDrawSet *at);         // ROR 0x0021b238
/* Counts a use of texture slot n; its GS register packet. */
u_long   *i3dDrawSetUseTex(I3dDrawSet *set, int n);                     // ROR 0x0021b260
/* Writes the no-texture registers into each used slot of a draw set. */
int       i3dBatchClearTextures(I3dDrawSet *set);                       // ROR 0x0021b288
void      i3dTexSetDescInit(I3dTexSetDesc *desc);                       // ROR 0x0021b300
int       i3dDrawSetInit(I3dDrawSet *set, I3dLayer *layer, const I3dTexSetDesc *desc,
                         u_int hash);                                   // ROR 0x0021b320

/* ---- tags and register writes ---- */

/* A DMA tag (qwc, id 0..7, address) and its two VIF words. */
u_long   *i3dPktDmaTag(u_long *pkt, u_int qwc, u_int id, const void *addr,
                       u_int vif0, u_int vif1);                         // ROR 0x0021b490
/* A DMA tag whose VIF words send the qwc qwords to the GIF (FLUSHA, DIRECT). */
u_long   *i3dPktDirectTag(u_long *pkt, u_int qwc, u_int id, const void *addr); // ROR 0x0021b4d0
/* A REF of a 6-qword texture register packet. */
u_long   *i3dPktRefTexRegs(u_long *pkt, u_long *regs);                  // ROR 0x0021b4f8
u_long   *i3dPktUseTexRegs(u_long *pkt, I3dDrawSet *set, int n);        // ROR 0x0021b518
/* One GS register write (a CNT tag, an A+D GIF tag, value / reg);
 * returns the position after it (pkt + 3 qwords). */
u_long   *i3dPktSetReg(u_long *pkt, u_long reg, u_long value);          // ROR 0x0021b550
/* CALL tags to the layer view's / fog's packet, when the layer has one. */
u_long   *i3dPktCallView(u_long *pkt, I3dLayer *layer);                 // ROR 0x0021b5a0
u_long   *i3dPktCallFog(u_long *pkt, I3dLayer *layer);                  // ROR 0x0021b5d0
/* ALPHA_1 from a blend mode (0 writes nothing; see i3dpacket.cpp). */
u_long   *i3dPktSetAlphaMode(u_long *pkt, int mode);                    // ROR 0x0021b600
/* The packet's first tags: blend mode 0x10, TEST_1 0x5000d. */
u_long   *i3dPktOpenChain(u_char *pkt);                                 // ROR 0x0021b698
/* A full-screen sprite that writes only Z (alpha test never, fail = Z). */
u_long   *i3dPktClearZ(u_long *pkt);                                    // ROR 0x0021b6c8

/* ---- primitives from float vectors (PACKED) ---- */

/* out = ftoi4(xyz * screen scale + screen offset). */
void      i3dPktConvXYZ(int *out, const float *xyz);                    // ROR 0x0021b740
/* out = ftoi0(rgba * scale), scale one of the two colour scales below. */
void      i3dPktConvRGBA(int *out, const float *rgba, const float *scale); // ROR 0x0021b790

u_long   *i3dPktFlatSpriteF(u_long *pkt, const float *xyz0, const float *xyz1,
                            const float *rgba);                         // ROR 0x0021b7c8
u_long   *i3dPktTexSpriteF(u_long *pkt, const float *xyz0, const float *xyz1,
                           const float *uv0, const float *uv1,
                           const float *rgba);                          // ROR 0x0021b860
u_long   *i3dPktFlatTriangleF(u_long *pkt, const float *xyz0, const float *xyz1,
                              const float *xyz2, const float *rgba);    // ROR 0x0021b928
u_long   *i3dPktTexTriangleF(u_long *pkt, const float *xyz0, const float *xyz1,
                             const float *xyz2, const float *uv0, const float *uv1,
                             const float *uv2, const float *rgba);      // ROR 0x0021b9d0
u_long   *i3dPktGouraudTriangleF(u_long *pkt, const float *xyz0, const float *xyz1,
                                 const float *xyz2, const float *rgba0,
                                 const float *rgba1, const float *rgba2); // ROR 0x0021bac8
u_long   *i3dPktGouraudTexTriangleF(u_long *pkt, const float *xyz0, const float *xyz1,
                                    const float *xyz2, const float *uv0, const float *uv1,
                                    const float *uv2, const float *rgba0,
                                    const float *rgba1, const float *rgba2); // ROR 0x0021bbb8
u_long   *i3dPktFlatLineF(u_long *pkt, const float *xyz0, const float *xyz1,
                          const float *rgba);                           // ROR 0x0021bce0
u_long   *i3dPktTexLineF(u_long *pkt, const float *xyz0, const float *xyz1,
                         const float *uv0, const float *uv1, const float *rgba); // ROR 0x0021bd78
u_long   *i3dPktGouraudLineF(u_long *pkt, const float *xyz0, const float *xyz1,
                             const float *rgba0, const float *rgba1);   // ROR 0x0021be40
u_long   *i3dPktGouraudTexLineF(u_long *pkt, const float *xyz0, const float *xyz1,
                                const float *uv0, const float *uv1,
                                const float *rgba0, const float *rgba1); // ROR 0x0021bef8
u_long   *i3dPktFlatPointF(u_long *pkt, const float *xyz, const float *rgba); // ROR 0x0021bfe8
/* uv is four ints, copied as they are. */
u_long   *i3dPktTexPointF(u_long *pkt, const float *xyz, const int *uv,
                          const float *rgba);                           // ROR 0x0021c068
/* The screen transform of i3dPktConvXYZ (data 0x2db330 / 0x2db340). */
void      i3dPktSetScreenOffset(const float *offset);                   // ROR 0x0021c100
void      i3dPktSetScreenScale(const float *scale);                     // ROR 0x0021c120

/* ---- primitives from packed values (REGLIST) ---- */

/* A blended sprite (PRIM 0x46) from two XYZ2 corners and an RGBAQ. */
u_long   *i3dPktSprite(u_long *pkt, u_long xyz0, u_long xyz1, u_long rgbaq); // ROR 0x0021c140
u_long   *i3dPktTexSpriteUV(u_long *pkt, u_long xyz0, u_long xyz1, u_long uv0, u_long uv1,
                            u_long rgbaq);                              // ROR 0x0021c1a0
u_long   *i3dPktTexSpriteST(u_long *pkt, u_long xyz0, u_long xyz1, u_long st0, u_long st1,
                            u_long rgbaq);                              // ROR 0x0021c208
u_long   *i3dPktTriangle(u_long *pkt, u_long xyz0, u_long xyz1, u_long xyz2,
                         u_long rgbaq);                                 // ROR 0x0021c270
u_long   *i3dPktTexTriangleUV(u_long *pkt, u_long xyz0, u_long xyz1, u_long xyz2,
                              u_long uv0, u_long uv1, u_long uv2, u_long rgbaq); // ROR 0x0021c2d8
u_long   *i3dPktTexTriangleST(u_long *pkt, u_long xyz0, u_long xyz1, u_long xyz2,
                              u_long st0, u_long st1, u_long st2, u_long rgbaq); // ROR 0x0021c348
u_long   *i3dPktGouraudTriangle(u_long *pkt, u_long xyz0, u_long xyz1, u_long xyz2,
                                u_long rgbaq0, u_long rgbaq1, u_long rgbaq2); // ROR 0x0021c3b8
u_long   *i3dPktGouraudTexTriangleUV(u_long *pkt, u_long xyz0, u_long xyz1, u_long xyz2,
                                     u_long uv0, u_long uv1, u_long uv2, u_long rgbaq0,
                                     u_long rgbaq1, u_long rgbaq2);     // ROR 0x0021c428
u_long   *i3dPktGouraudTexTriangleST(u_long *pkt, u_long xyz0, u_long xyz1, u_long xyz2,
                                     u_long st0, u_long st1, u_long st2, u_long rgbaq0,
                                     u_long rgbaq1, u_long rgbaq2);     // ROR 0x0021c4b0
/* A blended line (PRIM 0x41) from two XYZ2 ends and an RGBAQ. */
u_long   *i3dPktLine(u_long *pkt, u_long xyz0, u_long xyz1, u_long rgbaq); // ROR 0x0021c538
u_long   *i3dPktTexLineUV(u_long *pkt, u_long xyz0, u_long xyz1, u_long uv0, u_long uv1,
                          u_long rgbaq);                                // ROR 0x0021c598
u_long   *i3dPktTexLineST(u_long *pkt, u_long xyz0, u_long xyz1, u_long st0, u_long st1,
                          u_long rgbaq);                                // ROR 0x0021c600
u_long   *i3dPktGouraudLine(u_long *pkt, u_long xyz0, u_long xyz1, u_long rgbaq0,
                            u_long rgbaq1);                             // ROR 0x0021c668
u_long   *i3dPktGouraudTexLineUV(u_long *pkt, u_long xyz0, u_long xyz1, u_long uv0,
                                 u_long uv1, u_long rgbaq0, u_long rgbaq1); // ROR 0x0021c6d0
u_long   *i3dPktGouraudTexLineST(u_long *pkt, u_long xyz0, u_long xyz1, u_long st0,
                                 u_long st1, u_long rgbaq0, u_long rgbaq1); // ROR 0x0021c740
u_long   *i3dPktPoint(u_long *pkt, u_long xyz, u_long rgbaq);           // ROR 0x0021c7b0
u_long   *i3dPktTexPointUV(u_long *pkt, u_long xyz, u_long uv, u_long rgbaq); // ROR 0x0021c808
u_long   *i3dPktTexPointST(u_long *pkt, u_long xyz, u_long st, u_long rgbaq); // ROR 0x0021c868

/* ---- packers ---- */

/* r | g << 8 | b << 16 | a << 24 | the bits of q << 32. */
u_long    i3dPackRGBAQ(float q, u_int r, u_int g, u_int b, u_int a);    // ROR 0x0021c8c8
/* The bits of s | the bits of t << 32. */
u_long    i3dPackST(float s, float t);                                  // ROR 0x0021c900
u_long    i3dPackUV(u_int u, u_int v);                                  // ROR 0x0021c928
u_long    i3dPackXYZ(u_int x, u_int y, u_int z);                        // ROR 0x0021c940

#endif /* _I3D_I3DPACKET_H */
