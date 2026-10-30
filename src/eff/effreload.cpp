/* ==========================================================================
 *  eff/effreload.cpp
 *
 *  EffReload1 (0x1790e4..0x179428): the gun's reload model played once.
 *  See eff/effreload.h.
 * ======================================================================== */
#include "eff/effreload.h"

#include "eff/effcommon.h"
#include "eff/effman.h"
#include "i3d/i3dmath.h"
#include "i3d/i3mpose.h"
#include "i3d/i3rmodel.h"
#include "mag/magscene.h"
#include "utl/utlmath.h"
#include "utl/utlmisc.h"

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001790e4

EffReload1::EffReload1()
{
    // callcheck: Eff0 (the base constructor, compiler-made)
    m_node_map = NULL;
    m_nodes = NULL;
    m_root = NULL;              // PC: the ROM leaves it (init sets it before anything reads it)
}

// ──────────────────────────────────────────────────────────────────────
// Frees the node block and the node map (the model is the pack's).
// ROR 0x0017911c

EffReload1::~EffReload1()
{
    // callcheck: ~Eff0, operator_delete (the base destructor and the deleting variant, compiler-made)
    if (m_nodes != NULL)
        g_utlMemory.free(m_nodes);
    if (m_node_map != NULL)
        g_utlMemory.free(m_node_map);
}

// ──────────────────────────────────────────────────────────────────────
// The pack's model 0 with the scene's textures and its motion m_shots - 1
// (m_shots clamped to 6; below 1 no model, and liveMain ends it at once);
// the node block holds the pose, the local transforms (copied from the
// model) and the model matrices. The placement: rotation m_rot (z, x, y)
// at m_pos.
// ROR 0x001791ac

int EffReload1::init(const EffParam *param)
{
    if (!Eff0::init(param))
        return 0;

    MagScene *scene = g_effScene;
    int shots = param->m_reload.m_shots;
    if (shots > EFF_RELOAD_MAX_SHOTS)
        shots = EFF_RELOAD_MAX_SHOTS;
    if (shots < 1)
    {
        m_root = NULL;
        return 1;
    }

    const EffPack *pack = param->m_pack;
    m_root = (I3rRoot *)pack->m_file[0].m_ptr;
    m_motion = (I3mRoot *)pack->m_file[pack->m_num_i3r + shots - 1].m_ptr;
    i3rAssignTex(m_root, scene->m_tex_info.m_tex_info, scene->m_tex_info.m_num_tex);
    m_node_map = effMakeNodeMap(m_root, m_motion);

    int num = i3rGetNumNode(m_root);
    u_char *block = (u_char *)g_utlMemory.alloc(num * 0xb0, -1);
    m_nodes = block;
    m_local = (I3dMatrixF4x4 *)(block + num * 0x30);
    m_world = m_local + num;
    i3rCopyLocalBasicTransform(m_root, m_local);

    i3dMatUnit(m_mat);
    float rot[4];
    rot[0] = param->m_reload.m_rot[0];
    rot[1] = param->m_reload.m_rot[1];
    rot[2] = param->m_reload.m_rot[2];
    rot[3] = 0.0f;
    i3dMatRotZXY(m_mat, m_mat, rot);
    m_mat[3][0] = param->m_reload.m_pos[0];
    m_mat[3][1] = param->m_reload.m_pos[1];
    m_frame = 0;
    m_mat[3][2] = param->m_reload.m_pos[2];
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The next frame; over (0) once frames / 60 reach the motion's length,
// or at once without a model.
// ROR 0x00179324

int EffReload1::liveMain()
{
    if (m_root == NULL)
        return 0;
    m_frame++;
    return (float)m_frame / 60.0f < i3mGetLength(m_motion);
}

// ──────────────────────────────────────────────────────────────────────
// The motion's pose at m_frame / 60 s into the local transforms, the model
// matrices from them, and the model drawn at m_mat into the param's layer.
// ROR 0x00179390

void EffReload1::renderPreMain(RENDERPARAM *param)
{
    if (m_root == NULL)
        return;
    i3mGetPose(m_motion, m_nodes, 0x111, (float)m_frame / 60.0f);
    i3mPoseToMatrices(m_nodes, (sceVu0FMATRIX *)m_local, m_node_map);
    i3rConvertLocalToModel(m_root, m_world, m_local);
    utlDrawI3r(param->m_cur, m_root, (sceVu0FMATRIX *)m_world, m_mat, NULL);
}
