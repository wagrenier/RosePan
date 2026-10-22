/* ==========================================================================
 *  dog/dogactor.h
 *
 *  DogActor (dog/dogactor.cpp; vtable _vt$8DogActor 0x2e0a80, base
 *  DogModel, vptr at +0x94, EE 0x4d0: the scene's actor stride, entry 18
 *  of the i3d size table): a model with motions -- up to four motion banks
 *  (m_motion_data), a base rotation, the Y offsets the motion root moves
 *  by. The scene holds 128 of them (MagScene::m_actors). Minimal: what
 *  MagSystem's actor methods touch.
 *
 *  EE virtual slots added or overridden (DogModel's are in dogmodel.h):
 *    3 ~DogActor 0x10f16c, 7 getNodeMatrixPtr 0x2812b4, 12 0x10e5c0,
 *    14 0x10f244, 15 0x10f1f0, 17 getNodeWorldMatrix 0x10eab8,
 *    18 0x10eb28, 19 0x10e3ac, 20 0x10e534, 21 0x10cffc
 *  PORT: not polymorphic on the host yet (dogentity.h).
 * ======================================================================== */
#ifndef _DOG_DOGACTOR_H
#define _DOG_DOGACTOR_H

#include <stddef.h>

#include "dog/dogmodel.h"

/* EE 0x4d0. layout: host -- a DogModel (pointers); EE offsets in the
 * comments. */
struct DogActor : DogModel
{
    int  getMotionMove(float t0, float t1, float *move, float *rot, float *pos); // ROR 0x00109348
    void releaseMotion(int theId);                                       // ROR 0x00109b88
    /* Loads a motion pack (asserts theFileName, m_pack_motion[theLevel] == NULL);
     * the order of the two ints is the ROM's (MagScene::setActorAnim swaps them). */
    u_int loadPackMotion(const char *theFileName, int a, int b);         // ROR 0x0010c17c
    void addTrans(const float *d, float trans_ofs_y, float rot_y_aar);   // ROR 0x0010a038
    /* EE slot 17 (+0x88): out = m_matrix x the model matrix of node n. */
    void getNodeWorldMatrix(sceVu0FMATRIX out, int n);                   // ROR 0x0010eab8

    // ROR 0x00281244
    void setBaseRotate(const float *v)
    {
        m_base_rotate[0] = v[0];
        m_base_rotate[1] = v[1];
        m_base_rotate[2] = v[2];
        m_base_rotate[3] = v[3];
    }
    // ROR 0x0028126c
    void setBaseRotate(float x, float y, float z)
    {
        m_base_rotate[0] = x;
        m_base_rotate[3] = 1.0f;
        m_base_rotate[1] = y;
        m_base_rotate[2] = z;
    }
    void clearRotYAAR() { m_rot_y_aar = 0.0f; }                          // ROR 0x0028128c
    void clearRotOfsY() { m_rot_ofs_y = 0.0f; }                          // ROR 0x00281294

    u_char        _148[0x3c0 - 0x148];
    sceVu0FVECTOR m_base_rotate;        // ROR 0x3c0: getBaseRotateY reads y
    float         m_rot_y_aar;          // ROR 0x3d0: clearRotYAAR; addTrans's second float
    int           m_control_node;       // ROR 0x3d4: getControlNode
    int           m_control_node_flg;   // ROR 0x3d8: setControlNodeFlg / getControlNodeFlg
    u_char        _3dc[0x420 - 0x3dc];
    float         m_trans_ofs_y_save;   // ROR 0x420: saveTransOfsY copies m_trans_ofs[1]
    float         m_rot_ofs_y;          // ROR 0x424: clearRotOfsY / getRotOfsYSave
    u_char        _428[0x4d0 - 0x428];
};
static_assert(offsetof(DogActor, m_base_rotate) >= 0x3c0, "m_base_rotate");
static_assert(offsetof(DogActor, m_rot_y_aar) >= 0x3d0, "m_rot_y_aar");
static_assert(offsetof(DogActor, m_rot_ofs_y) >= 0x424, "m_rot_ofs_y");
static_assert(sizeof(DogActor) >= 0x4d0, "DogActor is 0x4d0 bytes on the EE");

#endif /* _DOG_DOGACTOR_H */
