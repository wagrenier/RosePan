/* ==========================================================================
 *  rosepan_i3d.h
 *
 *  The native i3d draw module (architecture.md "PC platform layer"): the
 *  host consumer of RoR's i3d library.  The library keeps writing its GS
 *  packets as the ROM does (i3d/i3dpacket.cpp); the host never sends them
 *  to a GS.  Instead:
 *
 *  - recording: every 2D primitive writer (i3dPkt*Sprite / Triangle / Line
 *    / Point, both the REGLIST and the PACKED *F families) hands its GIF
 *    tag and data here as it writes them, and the register writes that
 *    steer them (i3dPktSetReg: ALPHA_1, TEST_1, SCISSOR_1, XYOFFSET_1;
 *    i3dPktRefTexRegs: a draw set's TEX0 / TEX1 packet; i3dPktCallView:
 *    the view's SCISSOR_1) are recorded too.  Each record is keyed by the
 *    packet address it was written at.
 *  - replay: the frame's DMA chain is assembled in i3dCtxCloseFrame by
 *    walking the sorted groups (i3dCtxRunGroup and the state / channel
 *    packet nodes).  Wherever the walk links a packet range into the chain,
 *    it calls RosePan_I3dReplay(start, end), which draws that range's
 *    records through the renderer -- so layers, draw sets and sort keys
 *    order the host draws exactly as they order the PS2's DMA.
 *  - textures: i3dTexRegsSet tells the module which I3dTexture / I3dClut
 *    (and palette) a TEX register packet stands for; the first draw that
 *    uses it decodes the object's main-memory image on the host
 *    (rosepan_draw2d's RosePan_Draw2dMakeTexture) into a GPU texture
 *    cached per object, dropped when i3dTexRelease / i3dClutRelease runs.
 *
 *  Not here yet: VU1 strips (the model / i3dPktDrawStrip path) -- see
 *  RosePan_I3dRecordVu1 -- and CALLed channel state packets (fog / light,
 *  VU1 state only).
 * ======================================================================== */
#ifndef ROSEPAN_I3D_H
#define ROSEPAN_I3D_H

#include <stdint.h>

struct i3dContext;

/* A GIF-tagged primitive written at `pkt` by an i3d writer: pkt[2] the GIF
 * tag (REGLIST or PACKED), pkt[3] its register descriptors, the data from
 * pkt[4].  Decoded into host vertices now (the data may be reused later). */
void RosePan_I3dRecordGif(const uint64_t *pkt);

/* An A+D register write at `at` (i3dPktSetReg). */
void RosePan_I3dRecordReg(const void *at, uint64_t reg, uint64_t value);

/* A REF of a draw set's texture register packet at `at`; `regs` is read
 * at replay, when i3dCtxFlushTextures has filled it. */
void RosePan_I3dRecordTexRegs(const void *at, const uint64_t *regs);

/* i3dTexRegsSet / i3dTexRegsSetNone: the objects `regs` describes (NULL
 * texture = no texture). */
void RosePan_I3dBindTexRegs(const uint64_t *regs, const void *tex, const void *clut,
                            unsigned int clut_index);

/* i3dTexRelease / i3dClutRelease: forget the GPU textures made from `obj`. */
void RosePan_I3dReleaseObject(const void *obj);

/* i3dCtxBeginFrame: a new frame's packets; the previous frame's records
 * are gone. */
void RosePan_I3dBeginFrame(void);

/* Draws the records written in [start, end), in address order. */
void RosePan_I3dReplay(const void *start, const void *end);

/* The named hook for VU1 strips (i3dPktDrawStrip and the model packets):
 * not drawn yet; counts and logs once so a smoke run shows when the path
 * is reached. */
void RosePan_I3dRecordVu1(const void *at, const void *data, unsigned int qwc);

/* PC test (no ROM counterpart): with ROSEPAN_I3D_TEST=1, draws two TIM2
 * pictures of DATA/MNURPK/S2DMAN.RPK each frame through the i3d library
 * (i3dTexSetTim2 / i3dClutSetTim2, a texture table, a layer, a draw set,
 * i3dPktTexSpriteUV), proving the module end to end before SprMan reaches
 * the writers.  Called from MagSystem::update after the scene draw. */
void RosePan_I3dForcedTest(i3dContext *ctx);

#endif /* ROSEPAN_I3D_H */
