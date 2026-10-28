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
 *  - models: i3rDrawModel files one mesh record (RosePan_I3dRecordMesh)
 *    per 0x4d mesh in its material's draw-set group, so meshes replay in
 *    the ROM's draw-set and depth order; the forced tests record a whole
 *    model (RosePan_I3dRecordModel). Either is recorded at its
 *    packet address like a primitive and drawn at replay through the
 *    renderer's mesh stream (rosepan_i3dmesh.*: the mesh items read the way
 *    VU1 would read their VIF blocks), with the camera the last
 *    RosePan_I3dRecordCamera before it set.
 *
 *  - VU1 primitives: the VU1 writers (i3d/i3dvu1pkt.cpp: strips, sprites)
 *    compute on the CPU what their microprogram would kick and record it
 *    as screen-space vertices (RosePan_I3dRecordVu1Strip).
 *
 *  Not here yet: CALLed channel state packets (fog / light, VU1 state only).
 * ======================================================================== */
#ifndef ROSEPAN_I3D_H
#define ROSEPAN_I3D_H

#include <stdint.h>

struct i3dContext;
struct I3rRoot;
struct I3dTexEntry;
struct I3rBlendOpt;
struct RosePanI3dMeshRef;

/* A GIF-tagged primitive written at `pkt` by an i3d writer: pkt[2] the GIF
 * tag (REGLIST or PACKED), pkt[3] its register descriptors, the data from
 * pkt[4].  Decoded into host vertices now (the data may be reused later). */
void RosePan_I3dRecordGif(const uint64_t *pkt);

/* An A+D register write at `at` (i3dPktSetReg). */
void RosePan_I3dRecordReg(const void *at, uint64_t reg, uint64_t value);

/* A REF of a draw set's texture register packet at `at`; `regs` is read
 * at replay, when i3dCtxFlushTextures has filled it. */
void RosePan_I3dRecordTexRegs(const void *at, const uint64_t *regs);

/* RxMpeg::draw's picture (snd/rxmpeg.cpp), at `at` in its layer packet:
 * the live movie frame (rosepan_video.h, `pic_w` x `pic_h`) over x0..x1 x
 * y0..y1 of a `frame_w` x `frame_h` frame buffer, where the ROM's image
 * transfer would have written it.  Replayed in the layer's order like the
 * primitives, so what draws after it (the fader) still covers it. */
void RosePan_I3dRecordMovie(const void *at, int x0, int y0, int x1, int y1,
                            int frame_w, int frame_h, int pic_w, int pic_h);
/* An i3r model draw at `at` (rosepan_i3dmesh.h RosePan_I3dMeshDraw's
 * arguments): `world` and `opt` (i3rDrawModel's blend words, NULL for
 * none) are copied, `root`, `nodes` and `tex` must stay valid until the
 * frame's replay. */
void RosePan_I3dRecordModel(const void *at, const I3rRoot *root, const float *world,
                            const float *nodes, const I3dTexEntry *tex, int tex_count,
                            const I3rBlendOpt *opt);

/* One mesh of an i3r model draw at `at` (rosepan_i3dmesh.h
 * RosePan_I3dMeshDrawRef): i3rDrawModel's part writers file one per 0x4d
 * mesh (one per batch for a depth-keyed material) in the material's
 * draw-set group, so the replay draws them in the ROM's draw-set and
 * depth order. Copies and lifetimes as RosePan_I3dRecordModel. */
void RosePan_I3dRecordMesh(const void *at, const I3rRoot *root, const float *world,
                           const float *nodes, const I3dTexEntry *tex, int tex_count,
                           const I3rBlendOpt *opt, const RosePanI3dMeshRef *ref);

/* The camera the model records after `at` draw with: row-major 4x4s, p' =
 * p * M (RosePan_RendererSet3DViewProjection's form; copied). */
void RosePan_I3dRecordCamera(const void *at, const float *view, const float *projection);

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

/* A vertex a VU1 microprogram would send: GS screen x / y in pixels (the
 * 2048-centred space XYZ2 holds, before ftoi4), its clip-space depth z / w
 * (near -1, far +1: the view's camera * projection), ST and Q (s / q is the
 * texture coordinate), RGBAQ's colour on the PS2 scale, and whether its XYZ2
 * kicks (ADC clear). */
struct RosePanI3dVu1Vertex
{
    float   x, y, z;
    float   s, t, q;
    uint8_t rgba[4];
    int     kick;
};

/* A triangle strip of `count` such vertices at `at` (copied), drawn with the
 * GS PRIM bits `prim` (TME, ABE...) and the replay's register state; textured
 * ones honour TEST_1's Z test (GEQUAL) against the meshes. */
void RosePan_I3dRecordVu1Strip(const void *at, unsigned int prim, const RosePanI3dVu1Vertex *v,
                               int count);

/* PC test (no ROM counterpart): with ROSEPAN_I3D_TEST=1, draws two TIM2
 * pictures of DATA/MNURPK/S2DMAN.RPK each frame through the i3d library
 * (i3dTexSetTim2 / i3dClutSetTim2, a texture table, a layer, a draw set,
 * i3dPktTexSpriteUV), proving the module end to end before SprMan reaches
 * the writers.  Called from MagSystem::update after the scene draw. */
void RosePan_I3dForcedTest(i3dContext *ctx);

/* PC test (no ROM counterpart): with ROSEPAN_I3D_MESH_TEST=1, loads the
 * first model of DATA/CH/CHARA.RPK (CHAP01, "elnb" -> amd.mdl: its TIM2
 * pack and its i3r), relocates it and assigns its textures with the i3r library, and
 * draws it each frame in a layer of its own through RosePan_I3dRecordModel
 * with a fixed camera turning around it.  Called beside
 * RosePan_I3dForcedTest. */
void RosePan_I3dMeshForcedTest(i3dContext *ctx);

#endif /* ROSEPAN_I3D_H */
