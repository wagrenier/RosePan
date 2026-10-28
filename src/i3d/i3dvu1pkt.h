/* ==========================================================================
 *  i3d/i3dvu1pkt.h
 *
 *  The i3d library's VU1 packet writers (i3d/i3dvu1pkt.cpp, ROR
 *  0x21c960..0x21eb28): instead of GIF primitives they UNPACK vertices and
 *  state to VU1 memory and MSCAL a microprogram of the image i3dInitHardware
 *  loads (data 0x2912b0), which transforms by the view's world -> screen
 *  matrix (VU1 0x3e2, the CALLed view packet put it there) and kicks the
 *  primitives itself.
 *
 *  - strips: i3dPktDrawStrip / i3dPktDrawStripColor (one body, 0x21c960):
 *    a matrix (VU1 0x3d2), up to 0x50 positions per chunk (VU1 0x1dc),
 *    STQ / colours by REF, ADC flags, a triangle strip (PRIM 0x4c / 0x5c
 *    from the command qwords at data 0x302970 / 0x3029b0);
 *  - the clipped line strips (i3dPktDrawStripCtx / LineStrip,
 *    i3dClipSegment): not VU1 at all -- transformed, clipped and projected
 *    on the CPU, written as GIF lines -- and the wire shapes built on them;
 *  - the sprites (mnuBase's menu draw): i3dPktBeginVu1Sprite writes the
 *    GIF template (VU1 0x3f4: a fan, TME from the mode, FGE with fog) and
 *    the colour scale (0x3f5), then per sprite a texture rectangle
 *    (i3dPktVu1Rect, 0x3f1), a matrix's three rows (i3dPktVu1Matrix,
 *    0x3ee) and i3dPktVu1Sprite's colour and centre, which run
 *    microprogram 0x712: corners centre +- 0.5 row 0 +- 0.5 row 1.
 *
 *  PORT: the host has no VU1. Each writer still writes the ROM's packet
 *  (sizes and words), and records what the microprogram would draw in the
 *  native i3d module (rosepan_i3d.h RosePan_I3dRecordVu1Strip), computed
 *  on the CPU with the view the last i3dPktCallView named.
 * ======================================================================== */
#ifndef _I3D_I3DVU1PKT_H
#define _I3D_I3DVU1PKT_H

#include "i3d/i3dlayer.h"
#include "i3d/i3dview.h"
#include "sdk/vu0_intrin.h"

/* Strips of count vertices: positions xyz (w ignored) through matrix, optional
 * STQ (s, t used), per-vertex colours (0..1) or one colour, optional ADC bytes
 * (non-zero: the vertex does not draw a triangle). */
u_long   *i3dPktDrawStripImpl(u_long *pkt, sceVu0FVECTOR *matrix, int count, const void *stq,
                              const float *color, const void *colors, const void *xyz,
                              const u_char *adc);                        // ROR 0x0021c960
u_long   *i3dPktDrawStrip(u_long *pkt, sceVu0FVECTOR *matrix, int count, const void *stq,
                          const void *colors, const void *xyz, const u_char *adc); // ROR 0x0021db18
u_long   *i3dPktDrawStripColor(u_long *pkt, sceVu0FVECTOR *matrix, int count, const void *stq,
                               const float *color, const void *xyz, const u_char *adc); // ROR 0x0021db40

/* The clip helpers of the CPU-side strips: VU0 CLIP flags of two clip-space
 * points (b in bits 0..5, a in 6..11: +x -x +y -y +z -z against |w|), and a
 * segment clipped to the six planes in place, its STQ (+0x10) and colour
 * (+0x20) interpolated along when set; 0 when it lies outside. */
u_int     i3dVu0ClipPair(const float *a, const float *b);               // ROR 0x0021db60
int       i3dClipSegment(float *a, float *b, int stq, int color);       // ROR 0x0021db90

/* Line strips drawn on the CPU as GIF lines through the layer's view
 * (camera * projection, clipped, then the viewport): segment i is skipped
 * when adc[i + 1] is set or both ends are behind the eye.
 * - i3dPktDrawStripCtx: optional STQ and per-vertex colours (0..1, clamped;
 *   gouraud / textured lines, white without colours);
 * - i3dPktDrawLineStrip: one colour. */
u_long   *i3dPktDrawStripCtx(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix, int count,
                             const void *stq, const void *colors, const void *xyz,
                             const u_char *adc);                         // ROR 0x0021dd10
u_long   *i3dPktDrawLineStrip(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix, int count,
                              const void *color, const void *xyz, const u_char *adc); // ROR 0x0021e1d8

/* The VU1 sprite packets. */
u_long   *i3dPktBeginVu1Sprite(u_long *pkt, I3dLayer *layer, int mode);   // ROR 0x0021e5a0
u_long   *i3dPktVu1Matrix(u_long *pkt, sceVu0FVECTOR *matrix);           // ROR 0x0021e428
u_long   *i3dPktVu1Rect(u_long *pkt, const float *rect);                 // ROR 0x0021e488
u_long   *i3dPktVu1ViewMatrix(u_long *pkt, I3dLayer *layer);             // ROR 0x0021e4e8
u_long   *i3dPktVu1ViewMatrixScaled(float sx, float sy, u_long *pkt, I3dLayer *layer); // ROR 0x0021e520
u_long   *i3dPktVu1Sprite(u_long *pkt, const float *pos, const float *color); // ROR 0x0021e690
u_long   *i3dPktEndVu1Sprite(u_long *pkt);                               // ROR 0x0021e688

/* Wire shapes as line strips (MagScene::drawStrips' debug records through
 * i3dLayerDrawWireSphere / Box / Shape):
 * - sphere: three circles of radius round center (XZ red, XY green, YZ
 *   blue), each from the centre out;
 * - box: centre +- extent, the 12 edges;
 * - shape: the unit cube (+-1) through b (times a when a is set), in
 *   colour c. */
u_long   *i3dPktDrawWireSphere(float radius, u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix,
                               const float *center);                    // ROR 0x0021e6f8
u_long   *i3dPktDrawWireBox(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix,
                            const float *center, const void *extent, const void *color); // ROR 0x0021e8f8
u_long   *i3dPktDrawWireShape(u_long *pkt, I3dLayer *layer, void *a, void *b, void *c); // ROR 0x0021e9f8

/* PC: the view whose packet i3dPktCallView last CALLed (VU1 0x3e2 on the
 * EE); the host's VU1 writers transform with it. */
void      i3dVu1SetView(const I3dView *view);

#endif /* _I3D_I3DVU1PKT_H */
