/* ==========================================================================
 *  dog/dogmodel.h
 *
 *  DogModel (dog/dogmodel.cpp, ROR 0x11440c..0x1160a8 and the inline
 *  methods 0x281cbc..0x2827c8; vtable _vt$8DogModel 0x2e0b90, base
 *  DogEntityEx, vptr at +0x94): an entity drawn from an i3d model -- the
 *  model root (m_i3r_root, asserted by every accessor in this header), a
 *  second root from the pack's file 2 (m_i3r_root2), an optional collision
 *  root (m_i3c_root) with the model node of each collision node
 *  (m_colli_tbl), one block of per-node matrices (local, model) and flags,
 *  the node of the parent it hangs from, hide / light / attention / shadow
 *  words and a display control record handed to the model's materials.
 *
 *  Loading (m_load_flag, LOAD_NONE until a load): loadPack takes a model
 *  pack (file 0 textures, 1 the i3r model, 2 an optional second i3r),
 *  loadData an i3r already in memory with an optional i3c, share another
 *  model's data (reference counted on the original); release undoes each.
 *
 *  EE virtual slots added or overridden (DogEntityEx's are in dogentity.h):
 *    2 transform 0x1150c0, 3 ~DogModel 0x2827c8, 5 setRoot 0x114d04,
 *    6 release 0x115cec, 7 getNodeMatrixPtr 0x281da8, 8 setHide 0x281db0,
 *    9 isHide 0x281db8, 10 setAttachNode 0x281dc0, 11 transformBillboard
 *    0x114e38, 12 draw 0x1151f4, 13 drawUnitMatrix 0x1152a8, 14 loadPack
 *    0x1158ec, 15 loadData 0x115790, 16 share 0x115c60
 *  DogActor overrides 3, 7, 12, 14 and 15; slots 1, 2, 5, 6, 11 and 16 are
 *  the same in both, so DogModel's own code calls them directly.
 *  PORT: not polymorphic on the host yet (dogentity.h); the slots that
 *  MagSystem and MagScene's draw call through the vtable (setAttachNode,
 *  transform, draw, drawUnitMatrix, setHide, isHide) are plain members until
 *  then, called on the object's known class. ~DogModel (0x2827c8: release
 *  with force 1) is not written: the scene's models are global and the
 *  scene releases them itself.
 * ======================================================================== */
#ifndef _DOG_DOGMODEL_H
#define _DOG_DOGMODEL_H

#include <stddef.h>

#include "common/ror_assert.h"
#include "dog/dogentity.h"
#include "dog/dogtexinfo.h"
#include "lib/unassigned.h"

struct BedPfh;
struct BedPfhHead;
struct I3dLayer;
struct MagSceneRayHit;

/* m_load_flag: what release has to undo. Only LOAD_NONE is the ROM's name
 * (its asserts); the others are named after what release frees. */
enum
{
    LOAD_NONE  = 0,             // nothing loaded
    LOAD_PACK  = 1,             // m_pack_file owned: freed with its file 0's textures
    LOAD_DATA  = 2,             // the i3r root and the i3c owned: freed with the model's textures
    LOAD_SHARE = 3,             // m_original's data: its reference is released
    LOAD_REF   = 4,             // loadPack / loadData: the caller's data, only the textures are released
};

/* The record the model's materials point at (utlSetI3rDispControl); the
 * renderer reads it (fields' roles unverified). Plain EE layout. EE 0x30
 * (getDispControlCopy copies 0x30 bytes). */
struct DogDispControl
{
    float  m_color[4];          // ROR 0x00: clear (0, 0, 0, 0); loadPack's sepia (1, 1, 1, 0)
    float  m_10[4];             // ROR 0x10: clear (1, 1, 1, 1); loadPack's sepia (0, 0, 0, 1)
    int    m_20;                // ROR 0x20: clear 0
    u_char m_24;                // ROR 0x24: clear 1
    u_char m_25;                // ROR 0x25: clear 1
    u_char _26[0x30 - 0x26];
};
static_assert(offsetof(DogDispControl, m_10) == 0x10, "m_10");
static_assert(offsetof(DogDispControl, m_20) == 0x20, "m_20");
static_assert(offsetof(DogDispControl, m_24) == 0x24, "m_24");
static_assert(sizeof(DogDispControl) == 0x30, "DogDispControl is 0x30 bytes on the EE");

/* EE 0x1a0 (MagScene's model array stride). layout: host -- holds the i3d
 * pointers. */
struct DogModel : DogEntityEx
{
    /* Both inline in the ROM: DogEntity / DogEntityEx's constructors, the
     * sub mode words zeroed, then clear (and setRoot). */
    DogModel();                                                          // ROR 0x00282620
    DogModel(I3rRoot *p);                                                // ROR 0x002826f4

    /* Every field to its unloaded value (no model, flags 0, clip radius
     * -1, no parent node, no shadow light, the display control's
     * defaults). */
    void clear();                                                        // ROR 0x00114c54
    /* EE slot 5 (+0x28): takes p as the model root (relocated), no node
     * matrices or collision yet. */
    void setRoot(I3rRoot *p);                                            // ROR 0x00114d04
    /* EE slot 14 (+0x70): loads the model from a pack with the scene's
     * textures (file 0's TIM2s, turned sepia when sepia is set; with
     * sepia_disp the display control too). 0, -1 without a pack. */
    int  loadPack(DogTexInfo *tex, const BedPfh &pfh, int sepia, int sepia_disp); // ROR 0x001158ec
    /* EE slot 15 (+0x78): an i3r model already in memory (and an optional
     * i3c collision file); 0, -1 when the collision file is refused. */
    int  loadData(DogTexInfo *tex, void *data, void *i3c);               // ROR 0x00115790
    /* EE slot 16 (+0x80): uses theOriginal's data (a reference on it). */
    int  share(DogModel *theOriginal);                                   // ROR 0x00115c60
    /* EE slot 6 (+0x30): drops a reference (force: all of them) and frees
     * what the load brought in once none is left. */
    void release(int force);                                             // ROR 0x00115cec
    /* The per-node block: local matrices, model matrices, flags (once). */
    void allocMatrix();                                                  // ROR 0x0011533c
    /* Frees the per-node block (DogActor::releaseModel). */
    void freeMatrix();                                                   // ROR 0x001154cc
    /* The model node of every collision node (0xffff: none); NULL without
     * a collision file. */
    u_short *setupColliTbl();                                            // ROR 0x00115694
    /* Node flags from the node names (b_a* = 1, b_y* = 2). */
    void setupNodeFlags();                                               // ROR 0x0011550c
    /* EE slot 10 (+0x50) */
    void setAttachNode(int node) { m_attach_node = node; }               // ROR 0x00281dc0
    /* EE slot 2 (+0x10): the local matrix (when dirty) and the world
     * matrix from the parent's, or its node m_attach_node's. */
    void transform();                                                    // ROR 0x001150c0
    /* EE slot 11 (+0x58): with billboard bit 1 of m_flag100 builds the
     * world matrix turned to the camera (theDir: its rotation) or towards
     * thePos; 1 when it did. */
    int  transformBillboard(const float *theDir, const float *thePos);   // ROR 0x00114e38
    /* The world position of the model (its parent's when attached). */
    void getWorldPos(float *out);                                        // ROR 0x00114d8c
    /* EE slot 12 (+0x60): unless slot 11 places it from the camera's rotate
     * / trans, transforms and (not hidden) draws the model into layer. */
    void draw(I3dLayer *layer, const float *cam_rotate, const float *cam_trans); // ROR 0x001151f4
    /* EE slot 13 (+0x68): the model with a unit matrix, no node matrices. */
    void drawUnitMatrix(I3dLayer *layer);                                // ROR 0x001152a8
    /* The clip sphere's radius, read from the root on first use. */
    float getClipRadius();                                               // ROR 0x00116038
    /* Billboard bits 1..3 of m_flag100 = v & 7 (1 on, 2 with the x turn,
     * 4 towards a point). */
    void setFlag100Bits(int v);                                          // ROR 0x0011531c
    /* The ray pos + t dir against the collision groups (asserts
     * m_i3c_root, m_colli_tbl); hit->t (+0x0c) of the nearest, -1 none. */
    float castRay(MagSceneRayHit *hit, const float *pos, const float *dir); // ROR 0x0011440c
    /* A sphere of radius r at pos / a cylinder at pos of radius r along
     * dir against the collision groups; >= 0 on a hit. castCylinder takes
     * a NULL hit (then a local one) and is -1 without collision. */
    float castSphere(float r, MagSceneRayHit *hit, const float *pos);    // ROR 0x001145c0
    float castCylinder(float r, const float *pos, const float *dir, MagSceneRayHit *hit); // ROR 0x001149d4
    /* The ray against every collision group in turn; the hit record's word
     * +4 (thePos y when the hit's m_50 ends below -1). No callers. */
    float castRayGroups(const float *thePos, const float *theDir);       // ROR 0x00114758
    /* EE slot 8 (+0x40) */
    void setHide(int hide) { m_hide = hide; }                            // ROR 0x00281db0
    /* EE slot 9 (+0x48) */
    int  isHide() const { return m_hide; }                               // ROR 0x00281db8
    /* EE slot 7 (+0x38): DogActor's gives its node matrices. */
    sceVu0FMATRIX *getNodeMatrixPtr() { return NULL; }                   // ROR 0x00281da8
    void setLightID(int id) { m_light_id = id; }                         // ROR 0x00281d3c
    int  getLightID() const { return m_light_id; }                       // ROR 0x00281d44
    void setAttention(int a) { m_attention = a; }                        // ROR 0x00281d2c
    int  getAttention() const { return m_attention; }                    // ROR 0x00281d34
    int  getShadowLight() const { return m_shadow_light; }               // ROR 0x00281d4c
    int  getShadowLightOld() const { return m_shadow_light_prev; }       // ROR 0x00281d54
    int  getShadowCount() const { return m_shadow_fade; }                // ROR 0x00281d5c
    void setShadowCount(int n) { m_shadow_fade = n; }                    // ROR 0x00281d64
    // ROR 0x00281d6c
    void setShadowLight(int light)
    {
        m_shadow_fade = 0;
        m_shadow_light_prev = m_shadow_light;
        m_shadow_light = light;
    }
    void incShadowCount() { m_shadow_fade++; }                           // ROR 0x00281d80
    int *getSubMode() { return &m_mode; }                                // ROR 0x00281d90
    void incRefCount() { m_ref_count++; }                                // ROR 0x00281d98
    // ROR 0x00281d0c
    int decRefCount()
    {
        if (m_ref_count > 0)
            return --m_ref_count;
        return 0;
    }
    I3rRoot *getI3rRootPtr() { return m_i3r_root; }                      // ROR 0x00281dd0
    I3cRoot *getI3cRootPtr() { return m_i3c_root; }                      // ROR 0x00281dd8
    u_short *getColliAssignTbl() { return m_colli_tbl; }                 // ROR 0x00281de0

    // ROR 0x00281df0
    bool isAlive() const
    {
        if (m_i3r_root)
        {
            ROR_ASSERT(I3D_OK == I3rRoot__CheckIdString( m_i3r_root )); /* 248 */
        }
        return m_i3r_root != NULL;
    }

    /* The model's textures from the table; non-zero when one is missing
     * (the ROM's out-of-line copy drops the result, loadPack tests it). */
    // ROR 0x00281e48
    int assignTex(DogTexInfo *t)
    {
        ROR_ASSERT(m_i3r_root); /* 265 */
        ROR_ASSERT(t); /* 266 */
        return i3rAssignTex(m_i3r_root, t->m_tex_info, t->m_num_tex);
    }
    // ROR 0x0028200c
    int assignTex(const I3dTexEntry *p, int n)
    {
        ROR_ASSERT(m_i3r_root); /* 300 */
        ROR_ASSERT(p); /* 300 */
        ROR_ASSERT(n>0); /* 300 */
        return i3rAssignTex(m_i3r_root, p, n);
    }

    // ROR 0x00281ec0
    void unpack()
    {
        ROR_ASSERT(m_i3r_root); /* 296 */
        i3rRelocate(m_i3r_root);
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

    // ROR 0x00282508
    int updateMarkedMaterial()
    {
        ROR_ASSERT(m_i3r_root); /* 314 */
        return i3rUpdateTexAnim(m_i3r_root);
    }

    // ROR 0x0028215c (the line is the header's 305 in the ROM)
    int getNodeIndexByName(const char *p)
    {
        ROR_ASSERT(m_i3r_root);
        ROR_ASSERT(p);
        /* PORT: no node without a model (GamMan::attachObj asks actors
         * whose model is not loaded while DogActor::load is a stub). */
        if (m_i3r_root == NULL)
            return -1;
        return i3rGetNodeIndexByName(m_i3r_root, p);
    }

    // ROR 0x00282318
    const char *getNodeName(int i) const
    {
        ROR_ASSERT(m_i3r_root); /* 310 */
        ROR_ASSERT(i>=0); /* 310 */
        return i3rGetNodeNameByIndex(m_i3r_root, i);
    }

    /* The nodes' basic (bind) transforms, a matrix per node; the ROM has
     * the overload for both of its matrix types. */
    // ROR 0x002820ac
    void copyLocalBasicTransform(sceVu0FMATRIX *m)
    {
        ROR_ASSERT(m_i3r_root); /* 302 */
        i3rCopyLocalBasicTransform(m_i3r_root, (I3dMatrixF4x4 *)m);
    }
    // ROR 0x00282104
    void copyLocalBasicTransform(I3dMatrixF4x4 *m)
    {
        ROR_ASSERT(m_i3r_root); /* 303 */
        i3rCopyLocalBasicTransform(m_i3r_root, m);
    }

    /* Local node matrices c to model matrices a down the node tree. */
    // ROR 0x002821d0
    void convertLocalToModel(sceVu0FMATRIX *a, const sceVu0FMATRIX *c)
    {
        ROR_ASSERT(m_i3r_root); /* 307 */
        ROR_ASSERT(a); /* 307 */
        ROR_ASSERT(c); /* 307 */
        i3rConvertLocalToModel(m_i3r_root, (I3dMatrixF4x4 *)a, (const I3dMatrixF4x4 *)c);
    }
    // ROR 0x00282274
    void convertLocalToModel(I3dMatrixF4x4 *a, const I3dMatrixF4x4 *c)
    {
        ROR_ASSERT(m_i3r_root); /* 308 */
        ROR_ASSERT(a); /* 308 */
        ROR_ASSERT(c); /* 308 */
        i3rConvertLocalToModel(m_i3r_root, a, c);
    }

    // ROR 0x00281f08 (the line is the header's 297 in the ROM)
    int get_nNode() const
    {
        ROR_ASSERT(m_i3r_root);
        return i3rGetNumNode(m_i3r_root);
    }

    // ROR 0x00282550
    int get_nGroup() const
    {
        ROR_ASSERT(m_i3c_root); /* 317 */
        return i3cGetNumGroup(m_i3c_root);
    }

    // ROR 0x00282598
    int setBothFaceEnable(int flag)
    {
        ROR_ASSERT(m_i3c_root); /* 318 */
        return i3cSetBothFaceEnable(m_i3c_root, flag);
    }

    // ROR 0x002825ec
    void getDispControlCopy(DogDispControl *out) const { *out = m_disp_control; }

    int *getNodeFlag() { return m_mat_flag; }                            // ROR 0x00281dc8
    sceVu0FMATRIX *getModelMatrix() { return m_mat_model; }              // ROR 0x00281de8

    // ROR 0x00281f50
    int get_nTex() const
    {
        ROR_ASSERT(m_i3r_root); /* 298 */
        return i3rGetNumTex(m_i3r_root);
    }

    // ROR 0x00281f98
    const char *getTexName(int i) const
    {
        ROR_ASSERT(m_i3r_root); /* 299 */
        ROR_ASSERT(i>=0); /* 299 */
        return i3rGetTexName(m_i3r_root, i);
    }

    u_char          _e4[0xf0 - 0xe4];
    int             m_mode;         // ROR 0xf0: the sub mode words (getSubMode); DogActor::getMode; clear sets -1, ready() 0
    int             m_f4;           // ROR 0xf4: clear / ready() set -1; role unknown
    int             m_f8;           // ROR 0xf8: the constructors zero it
    int             m_fc;           // ROR 0xfc: the constructors zero it
    u_int           m_flag100;      // ROR 0x100: bits 1..3 billboard (setFlag100Bits, transformBillboard)
    int             m_ref_count;    // ROR 0x104: 1 after a load, + 1 per share
    float           m_clip_radius;  // ROR 0x108: < 0 until getClipRadius reads the clip sphere
    DogTexInfo     *m_tex_info;     // ROR 0x10c: the texture table the load used (release gives them back)
    int             m_load_flag;    // ROR 0x110: LOAD_*
    BedPfhHead     *m_pack_file;    // ROR 0x114: loadPack's pack
    DogModel       *m_original;     // ROR 0x118: LOAD_SHARE's model (set by the caller after share)
    I3rRoot        *m_i3r_root;     // ROR 0x11c: the model
    I3rRoot        *m_i3r_root2;    // ROR 0x120: the pack's file 2 when it has one (relocated); its reader is not known
    I3cRoot        *m_i3c_root;     // ROR 0x124: loadData's collision file
    u_short        *m_colli_tbl;    // ROR 0x128: the model node of each i3c node, 0xffff none (setupColliTbl)
    sceVu0FMATRIX  *m_mat_local;    // ROR 0x12c: nNode local matrices, then m_mat_model and m_mat_flag (one block)
    sceVu0FMATRIX  *m_mat_model;    // ROR 0x130: one model matrix per node (getModelMatrix)
    int            *m_mat_flag;     // ROR 0x134: one flag word per node (setupNodeFlags: b_a* names 1, b_y* 2)
    int             m_attach_node;  // ROR 0x138: the parent's node this model hangs from, -1 none
    int             m_attention;    // ROR 0x13c: setAttention / getAttention, clear -1
    int             m_hide;         // ROR 0x140: setHide / isHide
    int             m_light_id;     // ROR 0x144: setLightID / getLightID
    u_char          _148[0x160 - 0x148];
    int             m_shadow_light;       // ROR 0x160: the point light its projected shadow comes from (MagScene::makeActorShadow), clear -1
    int             m_shadow_light_prev;  // ROR 0x164: the one before (getShadowLightOld), faded out over the first 30 frames of a change
    int             m_shadow_fade;        // ROR 0x168: frames since the light changed (getShadowCount; 0..59 fade out then in; 60 and up, or negative, steady)
    void           *m_16c;          // ROR 0x16c: freed by release (its allocator is not known yet)
    DogDispControl  m_disp_control; // ROR 0x170: utlSetI3rDispControl's record
};
static_assert(offsetof(DogModel, m_mode) >= 0xf0, "m_mode");
static_assert(offsetof(DogModel, m_flag100) >= 0x100, "m_flag100");
static_assert(offsetof(DogModel, m_clip_radius) >= 0x108, "m_clip_radius");
static_assert(offsetof(DogModel, m_load_flag) >= 0x110, "m_load_flag");
static_assert(offsetof(DogModel, m_i3r_root) >= 0x11c, "m_i3r_root");
static_assert(offsetof(DogModel, m_i3c_root) >= 0x124, "m_i3c_root");
static_assert(offsetof(DogModel, m_mat_local) >= 0x12c, "m_mat_local");
static_assert(offsetof(DogModel, m_mat_model) >= 0x130, "m_mat_model");
static_assert(offsetof(DogModel, m_attach_node) >= 0x138, "m_attach_node");
static_assert(offsetof(DogModel, m_light_id) >= 0x144, "m_light_id");
static_assert(offsetof(DogModel, m_shadow_light) >= 0x160, "m_shadow_light");
static_assert(offsetof(DogModel, m_shadow_fade) >= 0x168, "m_shadow_fade");
static_assert(offsetof(DogModel, m_disp_control) >= 0x170, "m_disp_control");
static_assert(sizeof(DogModel) >= 0x1a0, "DogModel is 0x1a0 bytes on the EE");

#endif /* _DOG_DOGMODEL_H */
