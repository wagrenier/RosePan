/* ==========================================================================
 *  dog/dogentity.h
 *
 *  DogEntity and DogEntityEx (dog/dogentity.cpp; vtables _vt$9DogEntity
 *  0x2e0b38, _vt$11DogEntityEx 0x2e0b60, vptr at +0x94): the transform node
 *  under every Dog object -- a world matrix at +0x0, translation, rotation
 *  (radians, wrapped to [-pi, pi)), scale and an offset vector, a dirty
 *  flag the transform pass clears, and (DogEntityEx) a parent link.
 *  Minimal: what MagSystem's actor methods touch; the dog/ TUs fill in the
 *  rest.
 *
 *  EE virtual slots (GCC 2, 8-byte entries after an 8-byte header):
 *    DogEntity    1 reset 0x280ec8, 2 transform 0x280f18, 3 ~DogEntity 0x280f34
 *    DogEntityEx  1 reset 0x11023c, 2 transform 0x11034c, 3 ~DogEntityEx
 *                 0x281c80, 4 getParent 0x281cb4
 *  PORT: not polymorphic on the host. The EE vptr word (+0x94) is kept and
 *  each constructor stores its class's EE vtable address there, as the ROM's
 *  do (DOG_VT_*); the one virtual written code needs through a base pointer,
 *  slot 2 transform (a parent's, the camera's), dispatches on it in
 *  callTransform. Making the classes virtual would move every field off its
 *  EE offset.
 * ======================================================================== */
#ifndef _DOG_DOGENTITY_H
#define _DOG_DOGENTITY_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "sdk/vu0_intrin.h"

/* The EE vtable addresses the constructors store at +0x94: the host's
 * class tag (see the file comment). */
enum
{
    DOG_VT_ACTOR     = 0x002e0a80,  // _vt$8DogActor
    DOG_VT_ENTITY    = 0x002e0b38,  // _vt$9DogEntity
    DOG_VT_ENTITY_EX = 0x002e0b60,  // _vt$11DogEntityEx
    DOG_VT_MODEL     = 0x002e0b90,  // _vt$8DogModel
    DOG_VT_CAMERA    = 0x002e1fb8,  // _vt$9DogCamera
};

/* EE 0x98 + padding (data to +0x94, the vptr at +0x94). layout: host --
 * the EE vptr word is kept as padding so the data keeps its EE offsets
 * while the class is not polymorphic. */
struct DogEntity
{
    /* Inline in the ROM (no symbol; GCC 2 writes it into every derived
     * constructor -- the sky models' and the camera's in MagScene's,
     * 0x1360a0): translation, rotation, offset and m_80 (0, 0, 0, 1),
     * scale 1, dirty. The matrix is left as it is. */
    DogEntity()
    {
        for (int i = 0; i < 4; i++)
        {
            float v = (i == 3) ? 1.0f : 0.0f;
            m_trans[i] = v;
            m_rotate[i] = v;
            m_trans_ofs[i] = v;
            m_80[i] = v;
            m_scale[i] = 1.0f;
        }
        m_dirty = 1;
        _ee_vptr = DOG_VT_ENTITY;
    }

    /* EE slot 1 (+0x08): m_80, rotation, offset and translation
     * (0, 0, 0, 1), scale 1, dirty. */
    void reset();                                                        // ROR 0x00280ec8
    /* EE slot 2 called through the vptr word: the transform of the class
     * the object was built as (DogModel's for models and actors). */
    void callTransform();
    /* Runs slot 2 (transform) and returns the matrix 0x237678 makes at
     * +0xf0 -- past DogEntity: the derived camera class holds it (the
     * scene calls it on its camera, +0x2ab70). dog/dogcamera.cpp. */
    sceVu0FVECTOR *getMatrix();                                          // ROR 0x0010fa6c
    void setRotate(float x, float y, float z);                          // ROR 0x0010fc7c
    /* Copies m_matrix's third row (the z axis) to out; returns out. */
    float *getMatrixZ(float *out);                                       // ROR 0x00110114
    /* Copies m_matrix's fourth row (the position) to out. Unreferenced. */
    void  getMatrixPos(float *out);                                      // ROR 0x001101f0
    /* out = the rotation of m_matrix's z axis (utlVecToRot). Unreferenced. */
    void  getMatrixRot(float *out);                                      // ROR 0x001101fc
    /* Turns m_rotate towards pos (seen from m_trans): mode 0 x and y
     * (z 0), odd mode y only, other modes x only. Dirty; not normalized. */
    void  lookAt(const float *pos, u_int mode);                          // ROR 0x00110144
    void normalizeRotate();                                              // ROR 0x0010ff74
    /* m_80 y = r wrapped once to -pi..pi; dirty. */
    void setAddRotY(float r);                                            // ROR 0x0010fcb4
    /* When m_dirty: out = scale, rot z, x, y (m_rotate + m_80, each wrapped
     * into -pi..pi), translate m_trans and m_trans_ofs; clears m_dirty
     * and returns 1, else 0 with out untouched. */
    int  makeLocalMatrix(sceVu0FVECTOR *out);                            // ROR 0x0010fd18
    /* EE slot 2 (+0x10): rebuilds m_matrix when m_dirty. */
    void transform();                                                    // ROR 0x00280f18

    sceVu0FMATRIX m_matrix;     // ROR 0x00: the world matrix (DogActor::getNodeWorldMatrix's left operand)
    sceVu0FVECTOR m_trans;      // ROR 0x40: translation, w = 1
    sceVu0FVECTOR m_rotate;     // ROR 0x50: rotation x, y, z in radians, wrapped by normalizeRotate
    sceVu0FVECTOR m_scale;      // ROR 0x60: (1, 1, 1, 1) after reset
    sceVu0FVECTOR m_trans_ofs;  // ROR 0x70: an offset; y is DogActor's TransOfsY (saveTransOfsY)
    sceVu0FVECTOR m_80;         // ROR 0x80: a rotation added to m_rotate (makeLocalMatrix); (0, 0, 0, 1) after reset and setActorRotate
    int           m_dirty;      // ROR 0x90: set by every setter, 1 = the matrix needs a transform
    u_int         _ee_vptr;     // ROR 0x94: the EE vptr (see the file comment)
    /* PC: the 16-byte tail padding as a field. With a constructor the class
     * is no longer POD and MinGW (Itanium ABI) would put DogEntityEx's first
     * field in the tail padding, at 0x98 instead of 0xa0. */
    u_int         _98[2];
};
static_assert(offsetof(DogEntity, m_trans) == 0x40, "m_trans");
static_assert(offsetof(DogEntity, m_rotate) == 0x50, "m_rotate");
static_assert(offsetof(DogEntity, m_scale) == 0x60, "m_scale");
static_assert(offsetof(DogEntity, m_trans_ofs) == 0x70, "m_trans_ofs");
static_assert(offsetof(DogEntity, m_80) == 0x80, "m_80");
static_assert(offsetof(DogEntity, m_dirty) == 0x90, "m_dirty");
static_assert(sizeof(DogEntity) == 0xa0, "DogEntity is 0x98 bytes of data, 16-aligned");

/* EE size at least 0xe4. layout: host -- holds the parent pointer. */
struct DogEntityEx : DogEntity
{
    /* Inline in the ROM, as DogEntity's: no parent. */
    DogEntityEx() : m_parent(NULL) { _ee_vptr = DOG_VT_ENTITY_EX; }   // ROR 0x00281bc8

    /* EE slot 1 (DogModel, DogActor and DogCamera keep it): DogEntity's
     * reset, a unit local matrix and no parent. */
    void reset();                                                        // ROR 0x0011023c
    /* Hangs this under p (NULL: a root; asserts p's chain does not come
     * back to p) and clears the offset to (0, 0, 0, 1). */
    void setParent(DogEntityEx *p);                                      // ROR 0x001102b4
    DogEntityEx *getParent() { return m_parent; }                        // ROR 0x00281cb4
    /* EE slot 2 (+0x10): m_matrix = the parent's matrix (after its own
     * transform) * the local matrix, or the local matrix for a root. */
    void transform();                                                    // ROR 0x0011034c

    sceVu0FMATRIX m_local_matrix; // ROR 0xa0: DogModel::transform's local matrix (makeLocalMatrix)
    DogEntityEx *m_parent;      // ROR 0xe0: NULL = a root; setParent refuses cycles (assert p != m_parent)
};
static_assert(offsetof(DogEntityEx, m_parent) == 0xe0, "m_parent");

#endif /* _DOG_DOGENTITY_H */
