/* ==========================================================================
 *  i3d/i3rdraw.h
 *
 *  The i3r model draw (i3d/i3rdraw.cpp, ROR 0x21fb50..0x222f50, between
 *  i3dctx.cpp and i3dlayer.cpp; no source string names the run): the mesh
 *  packet builder i3rBuildPackets runs at relocation, and i3rDrawModel,
 *  utlDrawI3r's body, which culls a model against its layer's view and
 *  writes the VU1 packets of its visible nodes' meshes.
 *
 *  The ROM's VIF / VU1 packet writers (0x220098 .. 0x222ce0) are not on the
 *  host: the native i3d module (rosepan/rendering/rosepan_i3dmesh.h) reads
 *  the mesh items the way VU1 reads their VIF blocks, so i3rDrawModel
 *  records the model at its layer's packet instead (architecture.md "PC
 *  platform layer"). See i3rmodel.h / rosepan_i3dmesh.h for the item tree.
 * ======================================================================== */
#ifndef _I3D_I3RDRAW_H
#define _I3D_I3RDRAW_H

#include <stddef.h>

#include "i3d/i3rroot.h"
#include "i3d/i3dlayer.h"

/* i3rDrawModel's results. */
#define I3R_DRAW_OK     0       /* something was drawn */
#define I3R_DRAW_CULLED 0xd     /* off screen, or nothing visible */

/* A node's mesh entry (the data of a node's child 1's children, read by
 * i3rDrawModel). From the file: plain EE layout. */
struct I3rNodeEntryData
{
    u_int   m_bones;            // ROR 0x00: relocated: the u16 node index per bone slot
    short   m_mesh;             // ROR 0x04: the combined mesh, an index into root child 2
    u_short m_num_bone;         // ROR 0x06
    u_char  m_enable;           // ROR 0x08: 0 = not drawn
};
static_assert(offsetof(I3rNodeEntryData, m_mesh) == 0x04, "I3rNodeEntryData::m_mesh");
static_assert(offsetof(I3rNodeEntryData, m_num_bone) == 0x06, "I3rNodeEntryData::m_num_bone");
static_assert(offsetof(I3rNodeEntryData, m_enable) == 0x08, "I3rNodeEntryData::m_enable");

/* A node's display record (child 0's data; utlSetI3rNodeDisp points it at
 * a DogModel / DogActor record): only the byte i3rDrawModel reads. */
#define I3R_NODE_DISP_SHOWN 0x25    /* u_char: 0 = the node is hidden */

/* ROR 0x0021fb50: writes the VIF / DMA packets of one mesh item (a type
 * 0x4c group's child 0 child) into buf and stores their addresses in the
 * item's data. i3rBuildPackets passes the same buffer start for every
 * mesh. PC: nothing (see i3rdraw.cpp). */
void  i3rBuildMeshPacket(I3rItem *mesh, void *buf);                     // ROR 0x0021fb50

/* ROR 0x0021fe68: draws root into layer with world (its local -> world
 * matrix) and nodes (one matrix per node, model space). nodes NULL: the
 * model's box (root child 4) is tested against the layer's view first, and
 * the root data's own node table is used. opt: the blend weight words
 * i3rDrawSetupBlend reads (NULL: the model's own). Returns I3R_DRAW_OK, or
 * I3R_DRAW_CULLED when nothing was drawn. */
int   i3rDrawModel(I3dLayer *layer, I3rRoot *root, sceVu0FMATRIX *nodes, sceVu0FVECTOR *world,
                   const void *opt);                                     // ROR 0x0021fe68

#endif /* _I3D_I3RDRAW_H */
