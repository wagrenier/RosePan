/* ==========================================================================
 *  dog/dogmodel.h
 *
 *  DogModel (dog/dogmodel.cpp; vtable _vt$8DogModel 0x2e0b90, base
 *  DogEntityEx, vptr at +0x94): an entity drawn from an i3d model -- the
 *  model root (m_i3r_root, asserted by every accessor in this header), the
 *  per-node matrices, the node it hangs from when attached, hide / light /
 *  attention words. Minimal: what MagSystem's actor methods touch.
 *
 *  EE virtual slots added or overridden (DogEntityEx's are in dogentity.h):
 *    2 transform 0x1150c0, 3 ~DogModel 0x2827c8, 5 0x114d04, 6 0x115cec,
 *    7 getNodeMatrixPtr 0x281da8, 8 setHide 0x281db0, 9 isHide 0x281db8,
 *    10 setAttachNode 0x281dc0, 11 0x114e38, 12 0x1151f4, 13 0x1152a8,
 *    14 0x1158ec, 15 0x115790, 16 0x115c60
 *  PORT: not polymorphic on the host yet (dogentity.h); setAttachNode,
 *  which MagSystem calls through the vtable, is a plain member until then.
 * ======================================================================== */
#ifndef _DOG_DOGMODEL_H
#define _DOG_DOGMODEL_H

#include <stddef.h>

#include "common/ror_assert.h"
#include "dog/dogentity.h"
#include "lib/unassigned.h"

/* EE size at least 0x148. layout: host -- holds the i3d pointers. */
struct DogModel : DogEntityEx
{
    /* EE slot 10 (+0x50) */
    void setAttachNode(int node) { m_attach_node = node; }               // ROR 0x00281dc0
    void setLightID(int id) { m_light_id = id; }                         // ROR 0x00281d3c
    int  getLightID() const { return m_light_id; }                       // ROR 0x00281d44

    // ROR 0x0028215c (the line is the header's 305 in the ROM)
    int getNodeIndexByName(const char *p)
    {
        ROR_ASSERT(m_i3r_root);
        ROR_ASSERT(p);
        return i3rGetNodeIndexByName(m_i3r_root, p);
    }

    u_char         _e4[0x11c - 0xe4];
    I3rRoot       *m_i3r_root;      // ROR 0x11c: the model (DogModel(tag_I3rRoot_s *))
    u_char         _120[0x130 - 0x120];
    sceVu0FMATRIX *m_model_matrix;  // ROR 0x130: one matrix per node (getModelMatrix, getNodeMatrixPtr)
    int            m_node_flag;     // ROR 0x134: getNodeFlag
    int            m_attach_node;   // ROR 0x138: the parent's node this model hangs from
    int            m_attention;     // ROR 0x13c: setAttention / getAttention
    int            m_hide;          // ROR 0x140: setHide / isHide
    int            m_light_id;      // ROR 0x144: setLightID / getLightID
};
static_assert(offsetof(DogModel, m_i3r_root) >= 0x11c, "m_i3r_root");
static_assert(offsetof(DogModel, m_model_matrix) >= 0x130, "m_model_matrix");
static_assert(offsetof(DogModel, m_attach_node) >= 0x138, "m_attach_node");
static_assert(offsetof(DogModel, m_light_id) >= 0x144, "m_light_id");

#endif /* _DOG_DOGMODEL_H */
