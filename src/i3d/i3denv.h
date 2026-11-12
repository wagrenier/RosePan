/* ==========================================================================
 *  i3d/i3denv.h
 *
 *  The i3d frame environment (i3d/i3denv.cpp, ROR 0x21a120..0x21a3e8 and
 *  0x21ac80..0x21b220): the fog and light records a layer hangs on itself
 *  (I3dLayer +0x830 / +0x82c, i3dpacket.cpp's i3dPktCallFog and the strip
 *  uploaders read them). Both live in the context's packet tail for one
 *  frame.
 *
 *  Fog: i3dCtxSetFog turns a fog record (colour, near / far distances and
 *  fog amounts) into FOGCOL and a ramp F(z) = base + scale / z with
 *  F = 255 (no fog) .. 0 (all fog colour), and writes a small VU1 packet at
 *  the context head that the layer CALLs.
 *
 *  Lights: a description (an ambient colour and five lists of lights, one
 *  per kind) becomes a light: i3dCtxCreateLight copies every list into the
 *  context tail, normalizing directions and turning spot cone angles into
 *  the cosines VU1 uses.
 * ======================================================================== */
#ifndef _I3D_I3DENV_H
#define _I3D_I3DENV_H

#include <stddef.h>

#include "i3d/i3dctx.h"

/* ---- fog ---- */

/* The fog record i3dCtxSetFog reads (MagScene keeps four at +0x2aae0;
 * magscene2.cpp's load fills [1] from mapparam.bbb). EE 0x20, plain floats. */
struct I3dFog
{
    sceVu0FVECTOR m_color;      // ROR 0x00: rgb 0..1 (clamped), w unused
    float         m_param[4];   // ROR 0x10: near distance, fog amount there (0..1),
                                //           far distance, fog amount there
};
static_assert(offsetof(I3dFog, m_param) == 0x10, "m_param");
static_assert(sizeof(I3dFog) == 0x20, "I3dFog is 0x20 bytes on the EE");

/* A frame's fog state (2 qwords from the context tail). Only words, so the
 * EE layout holds on the host. The first qword is copied into the fog
 * packet as is (VU1 address 0x3f8). */
struct I3dFogState
{
    u_int  m_00;                // ROR 0x00: 0
    u_int  m_04;                // ROR 0x04: 0
    float  m_ramp_base;         // ROR 0x08: F(z) = base + scale / z, 0..255
    float  m_ramp_scale;        // ROR 0x0c
    u_long m_fogcol;            // ROR 0x10: FOGCOL (r | g << 8 | b << 16)
    u_int  m_packet;            // ROR 0x18: DMA address of the fog packet (i3dPktCallFog)
    u_int  _1c;
};
static_assert(offsetof(I3dFogState, m_ramp_base) == 0x08, "m_ramp_base");
static_assert(offsetof(I3dFogState, m_fogcol) == 0x10, "m_fogcol");
static_assert(offsetof(I3dFogState, m_packet) == 0x18, "m_packet");
static_assert(sizeof(I3dFogState) == 0x20, "I3dFogState is 2 qwords on the EE");

/* ---- lights ---- */

/* Kind 0: a parallel light. EE 0x20. */
struct I3dParallelLight
{
    sceVu0FVECTOR m_dir;        // ROR 0x00: normalized by i3dCtxCreateLight (w 0)
    sceVu0FVECTOR m_color;      // ROR 0x10
};
static_assert(sizeof(I3dParallelLight) == 0x20, "I3dParallelLight is 0x20 bytes on the EE");

/* Kind 1: copied as is; position / colour roles unverified. EE 0x20. */
struct I3dPointLight
{
    sceVu0FVECTOR m_pos;        // ROR 0x00
    sceVu0FVECTOR m_color;      // ROR 0x10
};
static_assert(sizeof(I3dPointLight) == 0x20, "I3dPointLight is 0x20 bytes on the EE");

/* Kind 2: a spot light. EE 0x40. In a description m_cone holds the inner
 * and outer half angles (radians); in a created light it holds cos outer,
 * 1 / (cos inner - cos outer), cos^2 outer, 1 / sin outer. */
struct I3dSpotLight
{
    sceVu0FVECTOR m_pos;        // ROR 0x00
    sceVu0FVECTOR m_dir;        // ROR 0x10: normalized by i3dCtxCreateLight (w 0)
    sceVu0FVECTOR m_color;      // ROR 0x20
    float         m_cone[4];    // ROR 0x30
};
static_assert(offsetof(I3dSpotLight, m_cone) == 0x30, "m_cone");
static_assert(sizeof(I3dSpotLight) == 0x40, "I3dSpotLight is 0x40 bytes on the EE");

/* Kind 3: a direction (normalized) and two vectors; role unverified
 * (two colours of a hemisphere light?). EE 0x30. */
struct I3dDir2Light
{
    sceVu0FVECTOR m_dir;        // ROR 0x00
    sceVu0FVECTOR m_10;         // ROR 0x10
    sceVu0FVECTOR m_20;         // ROR 0x20
};
static_assert(sizeof(I3dDir2Light) == 0x30, "I3dDir2Light is 0x30 bytes on the EE");

/* Kind 4: three vectors copied as is; role unverified. EE 0x30. */
struct I3dVec3Light
{
    sceVu0FVECTOR m_v[3];       // ROR 0x00
};
static_assert(sizeof(I3dVec3Light) == 0x30, "I3dVec3Light is 0x30 bytes on the EE");

/* A light description (i3dCtxCreateLight's input; MagScene keeps the
 * shadow light's at +0x2a9d0) and a created light (4 qwords of the context
 * tail on the EE) share this layout. A count of 0 or a NULL list leaves the
 * kind out. EE 0x40. layout: host -- pointers. */
struct I3dLightDesc
{
    sceVu0FVECTOR     m_ambient;          // ROR 0x00
    int               m_parallel_count;   // ROR 0x10
    I3dParallelLight *m_parallel;         // ROR 0x14
    int               m_point_count;      // ROR 0x18
    I3dPointLight    *m_point;            // ROR 0x1c
    int               m_spot_count;       // ROR 0x20
    I3dSpotLight     *m_spot;             // ROR 0x24
    int               m_dir2_count;       // ROR 0x28
    I3dDir2Light     *m_dir2;             // ROR 0x2c
    int               m_vec3_count;       // ROR 0x30
    I3dVec3Light     *m_vec3;             // ROR 0x34
    i3dContext       *m_ctx;              // ROR 0x38: a created light's context (lists come from its tail)
};
static_assert(offsetof(I3dLightDesc, m_ambient) == 0, "m_ambient");

/* A created light: a description whose lists live in the context tail.
 * layout: host -- I3dLightDesc's. */
struct I3dLight : I3dLightDesc
{
};

/* ---- functions ---- */

/* All zero. */
void      i3dFogInit(I3dFog *fog);                                       // ROR 0x0021a120
/* The fog state of a frame from a fog record, its packet written at the
 * context head. */
void     *i3dCtxSetFog(i3dContext *ctx, const I3dFog *fog);              // ROR 0x0021a158
/* The fog packet: a RET-tagged chain unpacking FOGCOL and the ramp to
 * VU1 and starting micro 1; the new head. */
u_char   *i3dPktMakeFog(u_char *pkt, const I3dFogState *fog);           // ROR 0x0021a270

void      i3dLightDescInit(I3dLightDesc *desc);                          // ROR 0x0021ac80
int       i3dLightSetParallel(I3dLight *light, int count, const I3dParallelLight *list); // ROR 0x0021acd0
int       i3dLightSetDir2(I3dLight *light, int count, const I3dDir2Light *list);         // ROR 0x0021ad88
int       i3dLightSetVec3(I3dLight *light, int count, const I3dVec3Light *list);         // ROR 0x0021ae58
int       i3dLightSetPoint(I3dLight *light, int count, const I3dPointLight *list);       // ROR 0x0021aed8
int       i3dLightSetSpot(I3dLight *light, int count, const I3dSpotLight *list);         // ROR 0x0021af50
int       i3dLightSetAmbient(I3dLight *light, const float *ambient);    // ROR 0x0021b100
void      i3dLightInit(I3dLight *light);                                 // ROR 0x0021b110
I3dLight *i3dCtxCreateLight(i3dContext *ctx, const I3dLightDesc *desc);  // ROR 0x0021b148

#endif
