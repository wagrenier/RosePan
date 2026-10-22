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
 *  PORT: not polymorphic on the host yet (dogentity.h); the slots that
 *  MagSystem and MagScene's draw call through the vtable (setAttachNode,
 *  transform, draw, drawUnitMatrix, setHide, isHide) are plain members until
 *  then, called on the object's known class.
 * ======================================================================== */
#ifndef _DOG_DOGMODEL_H
#define _DOG_DOGMODEL_H

#include <stddef.h>

#include "common/ror_assert.h"
#include "dog/dogentity.h"
#include "lib/unassigned.h"

struct BedPfh;
struct DogTexInfo;
struct I3dLayer;

/* EE size at least 0x148. layout: host -- holds the i3d pointers. */
struct DogModel : DogEntityEx
{
    /* EE slot 14 (+0x70): loads the model from a pack (asserts
     * m_pack_file == __null) with the scene's textures and a light id. */
    int  loadPack(DogTexInfo *tex, const BedPfh &pfh, int light, int arg); // ROR 0x001158ec
    /* EE slot 15 (+0x78): loads an i3r model (and an optional second file). */
    int  loadData(DogTexInfo *tex, void *data, void *data2);             // ROR 0x00115790
    /* Node flags from the node names (b_a* = 1, b_y* = 2). */
    void setupNodeFlags();                                               // ROR 0x0011550c
    /* EE slot 6 (+0x30): frees what load brought in (asserts m_ref_count <= 0). */
    void release(int arg);                                               // ROR 0x00115cec
    /* EE slot 10 (+0x50) */
    void setAttachNode(int node) { m_attach_node = node; }               // ROR 0x00281dc0
    /* EE slot 2 (+0x10): the node matrices and the world matrix. */
    void transform();                                                    // ROR 0x001150c0
    /* EE slot 12 (+0x60): unless slot 11 culls it from the camera's rotate
     * / trans, transforms and (not hidden) draws the model into layer. */
    void draw(I3dLayer *layer, const float *cam_rotate, const float *cam_trans); // ROR 0x001151f4
    /* EE slot 13 (+0x68): the model with a unit matrix, no node matrices. */
    void drawUnitMatrix(I3dLayer *layer);                                // ROR 0x001152a8
    /* The clip sphere's radius, read from the root on first use. */
    float getClipRadius();                                               // ROR 0x00116038
    /* EE slot 8 (+0x40) */
    void setHide(int hide) { m_hide = hide; }                            // ROR 0x00281db0
    /* EE slot 9 (+0x48) */
    int  isHide() const { return m_hide; }                               // ROR 0x00281db8
    void setLightID(int id) { m_light_id = id; }                         // ROR 0x00281d3c
    int  getLightID() const { return m_light_id; }                       // ROR 0x00281d44

    // ROR 0x00281df0
    bool isAlive() const
    {
        if (m_i3r_root)
        {
            ROR_ASSERT(I3D_OK == I3rRoot__CheckIdString( m_i3r_root )); /* 248 */
        }
        return m_i3r_root != NULL;
    }

    // ROR 0x0028238c
    void copyClipSphere(float *p, float *r)
    {
        ROR_ASSERT(p); /* 311 */
        ROR_ASSERT(r); /* 311 */
        i3rCopyClipSphere(m_i3r_root, p, r);
    }

    // ROR 0x00282494 (MagScene::load asserts the result is I3D_OK)
    int replaceStqMapEntity(I3rStqMapEntity *p)
    {
        ROR_ASSERT(m_i3r_root); /* 313 */
        ROR_ASSERT(p); /* 313 */
        return i3rReplaceStqMapEntity(m_i3r_root, p);
    }

    // ROR 0x0028215c (the line is the header's 305 in the ROM)
    int getNodeIndexByName(const char *p)
    {
        ROR_ASSERT(m_i3r_root);
        ROR_ASSERT(p);
        return i3rGetNodeIndexByName(m_i3r_root, p);
    }

    u_char         _e4[0xf0 - 0xe4];
    int            m_mode;          // ROR 0xf0: DogActor::getMode; ready() clears it
    int            m_f4;            // ROR 0xf4: ready() sets -1; role unknown
    u_char         _f8[0x108 - 0xf8];
    float          m_clip_radius;   // ROR 0x108: < 0 until getClipRadius reads the clip sphere
    u_char         _10c[0x11c - 0x10c];
    I3rRoot       *m_i3r_root;      // ROR 0x11c: the model (DogModel(tag_I3rRoot_s *))
    u_char         _120[0x130 - 0x120];
    sceVu0FMATRIX *m_model_matrix;  // ROR 0x130: one matrix per node (getModelMatrix, getNodeMatrixPtr)
    int            m_node_flag;     // ROR 0x134: getNodeFlag
    int            m_attach_node;   // ROR 0x138: the parent's node this model hangs from
    int            m_attention;     // ROR 0x13c: setAttention / getAttention
    int            m_hide;          // ROR 0x140: setHide / isHide
    int            m_light_id;      // ROR 0x144: setLightID / getLightID
};
static_assert(offsetof(DogModel, m_mode) >= 0xf0, "m_mode");
static_assert(offsetof(DogModel, m_clip_radius) >= 0x108, "m_clip_radius");
static_assert(offsetof(DogModel, m_i3r_root) >= 0x11c, "m_i3r_root");
static_assert(offsetof(DogModel, m_model_matrix) >= 0x130, "m_model_matrix");
static_assert(offsetof(DogModel, m_attach_node) >= 0x138, "m_attach_node");
static_assert(offsetof(DogModel, m_light_id) >= 0x144, "m_light_id");

#endif /* _DOG_DOGMODEL_H */
