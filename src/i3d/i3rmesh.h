/* ==========================================================================
 *  i3d/i3rmesh.h
 *
 *  The i3r material packets (i3d/i3rmesh.cpp, ROR 0x228c08..0x22a6c0,
 *  between i3dvram.cpp and the i3r type accessors; no source string names
 *  the run, the file name is the port's). The model's VU1 writers
 *  (i3rDrawGroupSkinned / Rigid, 0x220838; i3d/i3rdraw.cpp, not ported)
 *  ask i3rGetMaterialPacket for a material's packet: per layer of the
 *  material its texture REF, its GS state (TEST, ALPHA, register
 *  overrides), the light set, the STQ map matrices and the uv transform,
 *  all unpacked to VU1 at the context head after the view and fog. The
 *  packets are cached per layer in 256 buckets keyed by material, texture
 *  frame, light, fog and view.
 *
 *  The host's mesh draw (rosepan/rendering/rosepan_i3dmesh.cpp) reads the
 *  model items itself and takes the layer's TEST / ALPHA from
 *  i3rPktLayerTest / i3rPktLayerAlpha; nothing else here runs on the host
 *  until the VU1 writers do.
 * ======================================================================== */
#ifndef _I3D_I3RMESH_H
#define _I3D_I3RMESH_H

#include <stddef.h>

#include "i3d/i3dlayer.h"
#include "i3d/i3rroot.h"

struct I3dLight;
struct I3dLightDesc;
struct I3dFogState;
struct I3dView;
struct I3rLayerData;

/* A mesh draw's option record (the third argument of i3rGetMaterialPacket,
 * passed down from the VU1 writers' callers): a uv offset and scale over
 * the layer's own, and a draw-set depth offset. Roles from its two readers
 * (0x228f50, 0x22a050); i3dLayerFindMaterialDrawSet reads m_depth. Plain
 * EE layout. */
struct I3rMeshOpt
{
    sceVu0FVECTOR m_uv_offset;  // ROR 0x00
    sceVu0FVECTOR m_uv_scale;   // ROR 0x10
    float         m_depth;      // ROR 0x20: added to the material's depth; non-zero = not cached
    u_char        m_uv_set;     // ROR 0x24: non-zero = m_uv_offset / m_uv_scale apply (not cached)
    u_char        _25[0x0b];
};
static_assert(offsetof(I3rMeshOpt, m_uv_scale) == 0x10, "I3rMeshOpt::m_uv_scale");
static_assert(offsetof(I3rMeshOpt, m_depth) == 0x20, "I3rMeshOpt::m_depth");
static_assert(offsetof(I3rMeshOpt, m_uv_set) == 0x24, "I3rMeshOpt::m_uv_set");
static_assert(sizeof(I3rMeshOpt) == 0x30, "I3rMeshOpt: the 0x30 bytes known");

/* A material's cached packet (EE 0x38 in 4 qwords of the context tail):
 * one circular list per bucket of I3dLayer::m_mesh_cache, the bucket's
 * front the last one used. layout: host -- pointers. */
struct I3rMeshPacket
{
    u_int           m_hash;         // ROR 0x00: the bucket hash; 0 = not cached
    I3rMeshPacket  *m_prev;         // ROR 0x04: the bucket's list (searched this way)
    I3rMeshPacket  *m_next;         // ROR 0x08
    I3dDrawSet     *m_set;          // ROR 0x0c: i3dLayerFindMaterialDrawSet's
    I3dGroup       *m_group;        // ROR 0x10: m_set's group
    u_int           m_sort;         // ROR 0x14: the material's sort function byte is non-zero
    u_int           m_count;        // ROR 0x18: layers
    u_int          *m_layer_pkt;    // ROR 0x1c: per layer, the DMA address of its packet
    u_int           m_pkt;          // ROR 0x20: the DMA address of the view / fog packet before them
    I3dLight       *m_light;        // ROR 0x24: the layer's at build time (the key)
    I3dView        *m_view;         // ROR 0x28
    I3dFogState    *m_fog;          // ROR 0x2c
    const I3rItem  *m_material;     // ROR 0x30: the material item (type 0x25)
    u_int           m_frame;        // ROR 0x34: its texture frame (I3rMaterialData::m_tex_frame)
};

/* The GS register values a layer's bytes give (i3rPktLayerTest /
 * i3rPktLayerAlpha); each writes a {value, register} pair only when the
 * layer sets something. */
u_long        *i3rPktLayerTest(u_long *pkt, const I3rLayerData *layer);          // ROR 0x0022a448
u_long        *i3rPktLayerAlpha(u_long *pkt, const I3rLayerData *layer);         // ROR 0x0022a638

/* out[0] = (m00, m11, m22, 0), out[1] = m[3] (i3dCtxCreateView's screen
 * scale and offset from its viewport). */
void           i3dMatGetScaleTrans(sceVu0FVECTOR *out, sceVu0FVECTOR *m);        // ROR 0x00228c38
/* Clears an option record's uv offset and scale. No callers found. */
void           i3rMeshOptClear(I3rMeshOpt *opt);                                 // ROR 0x00228c08
/* AFAIL from a layer's fail byte: 0x10 FB_ONLY, 0x0f ZB_ONLY, 0x18
 * RGB_ONLY, else KEEP. */
u_int          i3rAlphaFailMode(u_int mode);                                     // ROR 0x00228c78
/* The packet of material mat (a type 0x25 item) for layer's current
 * light, fog and view: from the cache, or built at the context head.
 * opt may be NULL. */
I3rMeshPacket *i3rGetMaterialPacket(I3dLayer *layer, const I3rItem *mat,
                                    const I3rMeshOpt *opt);                      // ROR 0x00228f50

#endif /* _I3D_I3RMESH_H */
