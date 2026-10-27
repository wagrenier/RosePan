// dog/dogmodel.cpp -- DogModel, an entity drawn from an i3d model (0x11440c..0x1160a8).
//
// A DogModel owns (or borrows) an i3r model root: loadPack takes a model
// pack (file 0 its TIM2s, file 1 the model, file 2 an optional second
// root), loadData a model already in memory with an optional i3c
// collision file, share another model's data. Each load assigns the
// scene's textures to the model, allocates one block of per-node matrices
// (local, model) and flags, and resets the entity transform; release
// undoes what m_load_flag says once the last reference is gone.
//
// Each frame draw places the model -- from its own transform, its
// parent's matrix or a node of its parent (m_attach_node), or turned to
// the camera when it is a billboard -- and hands the root, the node
// matrices and the world matrix to the i3d renderer. The casts run the
// collision module over the i3c's groups for MagScene's ray / sphere /
// cylinder tests.

#include "dog/dogmodel.h"

#include <string.h>

#include "bed/bedpfh.h"
#include "common/ror_assert.h"
#include "i3d/i3dmath.h"
#include "mag/magscene.h"
#include "utl/utlmisc.h"

/* The angle wrap's pi and 2 pi: 0x40490fdc / 0x40c90fdc, one ulp above
 * the floats nearest them. */
static const float s_pi  = 0x1.921fb8p+1f;
static const float s_2pi = 0x1.921fb8p+2f;

/* The casts' node matrix buffer: the ROM's 4 KB stack array. */
#define DOG_CAST_MAX_NODE 64

/* PORT: the ROM calls the parent's EE slot 2 (transform) through its
 * vtable. The classes are not polymorphic on the host yet; every parent
 * that has a node matrix is a DogModel or DogActor (one transform,
 * 0x1150c0), so the parent is taken as a DogModel (DogEntityEx::setParent
 * is not written yet: nothing has a parent today). */
static DogModel *parentModel(DogEntityEx *p)
{
    return (DogModel *)p;
}

/* The cast record castRayGroups and castCylinder start from when they use
 * their own: every vector (0, 0, 0, 1). */
static void initHit(MagSceneRayHit *hit)
{
    for (int i = 0; i < 5; i++)
    {
        hit->m_vec[i][0] = 0.0f;
        hit->m_vec[i][1] = 0.0f;
        hit->m_vec[i][2] = 0.0f;
        hit->m_vec[i][3] = 1.0f;
    }
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x0011440c

float DogModel::castRay(MagSceneRayHit *hit, const float *pos, const float *dir)
{
    sceVu0FVECTOR ray[2];
    ray[0][0] = pos[0];
    ray[0][1] = pos[1];
    ray[0][2] = pos[2];
    ray[0][3] = 0.0f;
    ray[1][0] = dir[0];
    ray[1][1] = dir[1];
    ray[1][2] = dir[2];
    ray[1][3] = 0.0f;

    I3cQuery q;
    ROR_ASSERT(m_i3c_root); /* 272 */
    q.m_root = m_i3c_root;
    int nNode = i3cGetNumNode(q.m_root);
    ROR_ASSERT(nNode >= 0); /* 275 */
    ROR_ASSERT(m_mat_model); /* 277 */
    q.m_mat = m_mat_model;
    ROR_ASSERT(m_colli_tbl); /* 280 */
    q.m_colli_tbl = m_colli_tbl;
    q.m_group = -1;     /* PC: the ROM leaves the word unset */
    q.m_func = dogModelHitNearest;
    q.m_14 = 0;
    q.m_hit = hit;
    q._1c = 0;
    q.m_20 = 1;
    hit->m_50 = -1;
    hit->m_vec[0][3] = -1.0f;

    sceVu0FMATRIX mats[DOG_CAST_MAX_NODE];
    ROR_ASSERT(nNode <= 64); /* 295 */
    /* PC: the ROM writes past its 64 matrices after the assert. */
    if (nNode > DOG_CAST_MAX_NODE)
        nNode = DOG_CAST_MAX_NODE;
    for (int i = 0; i < nNode; i++)
        i3dMatUnit(mats[i]);
    i3cCastRay(&q, ray[0], mats);

    if (hit->m_50 > -2)
        return hit->m_vec[0][3];
    return -1.0f;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x001145c0

float DogModel::castSphere(float r, MagSceneRayHit *hit, const float *pos)
{
    sceVu0FVECTOR sphere;
    sphere[0] = pos[0];
    sphere[1] = pos[1];
    sphere[2] = pos[2];
    sphere[3] = r;

    I3cQuery q;
    ROR_ASSERT(m_i3c_root); /* 361 */
    q.m_root = m_i3c_root;
    int nNode = i3cGetNumNode(q.m_root);
    ROR_ASSERT(nNode >= 0); /* 364 */
    ROR_ASSERT(m_mat_model); /* 366 */
    q.m_mat = m_mat_model;
    ROR_ASSERT(m_colli_tbl); /* 369 */
    q.m_colli_tbl = m_colli_tbl;
    q.m_group = -1;     /* PC: the ROM leaves the word unset */
    q.m_func = dogModelHitNearest;
    q.m_14 = 0;
    q.m_hit = hit;
    q._1c = 0;
    q.m_20 = 1;
    hit->m_50 = -1;
    hit->m_vec[0][3] = -1.0f;

    sceVu0FMATRIX mats[DOG_CAST_MAX_NODE];
    ROR_ASSERT(nNode <= 64); /* 381 */
    /* PC: the ROM writes past its 64 matrices after the assert. */
    if (nNode > DOG_CAST_MAX_NODE)
        nNode = DOG_CAST_MAX_NODE;
    for (int i = 0; i < nNode; i++)
        i3dMatUnit(mats[i]);
    i3cCastSphere(&q, sphere, mats);

    if (hit->m_50 > -2)
        return hit->m_vec[0][3];
    return -1.0f;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30, i3cGetNumGroup (the assert handler; get_nGroup inline)
// ROR 0x00114758

float DogModel::castRayGroups(const float *thePos, const float *theDir)
{
    MagSceneRayHit hit;
    initHit(&hit);

    sceVu0FVECTOR ray[2];
    ray[0][0] = thePos[0];
    ray[0][1] = thePos[1];
    ray[0][2] = thePos[2];
    ray[0][3] = 0.0f;
    ray[1][0] = theDir[0];
    ray[1][1] = theDir[1];
    ray[1][2] = theDir[2];
    ray[1][3] = 0.0f;

    I3cQuery q;
    ROR_ASSERT(m_i3c_root); /* 446 */
    q.m_root = m_i3c_root;
    int nNode = i3cGetNumNode(q.m_root);
    ROR_ASSERT(nNode >= 0); /* 449 */
    ROR_ASSERT(m_mat_model); /* 451 */
    q.m_mat = m_mat_model;
    ROR_ASSERT(m_colli_tbl); /* 454 */
    q.m_colli_tbl = m_colli_tbl;
    q.m_func = dogModelHitNearest;
    q.m_14 = 0;
    q.m_hit = &hit;
    q._1c = 0;
    q.m_20 = 0;         /* PC: the ROM leaves the word unset */
    int ng = get_nGroup();
    ROR_ASSERT(ng >= 0); /* 462 */
    hit.m_50 = -1;
    hit.m_vec[0][3] = -1.0f;

    sceVu0FMATRIX mats[DOG_CAST_MAX_NODE];
    ROR_ASSERT(nNode <= 64); /* 468 */
    /* PC: the ROM writes past its 64 matrices after the assert. */
    if (nNode > DOG_CAST_MAX_NODE)
        nNode = DOG_CAST_MAX_NODE;
    for (int i = 0; i < nNode; i++)
        i3dMatUnit(mats[i]);

    /* Group -1 first, then each group. */
    if (ng >= 0)
    {
        q.m_group = -1;
        int g;
        do
        {
            g = q.m_group + 1;
            i3cCastRay(&q, ray[0], mats);
            q.m_group = g;
        } while (g < ng);
    }

    if (hit.m_50 < -1)
        return thePos[1];
    return hit.m_vec[0][1];
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30, i3cGetNumGroup (the assert handler; get_nGroup inline)
// ROR 0x001149d4

float DogModel::castCylinder(float r, const float *pos, const float *dir, MagSceneRayHit *hit)
{
    if (m_i3c_root == NULL)
        return -1.0f;

    MagSceneRayHit local;
    initHit(&local);

    sceVu0FVECTOR cyl[2];
    cyl[0][0] = pos[0];
    cyl[0][1] = pos[1];
    cyl[0][2] = pos[2];
    cyl[0][3] = r;
    cyl[1][0] = dir[0];
    cyl[1][1] = dir[1];
    cyl[1][2] = dir[2];
    cyl[1][3] = 0.0f;

    I3cQuery q;
    ROR_ASSERT(m_i3c_root); /* 536 */
    q.m_root = m_i3c_root;
    int nNode = i3cGetNumNode(q.m_root);
    ROR_ASSERT(nNode >= 0); /* 539 */
    ROR_ASSERT(m_mat_model); /* 541 */
    q.m_mat = m_mat_model;
    ROR_ASSERT(m_colli_tbl); /* 544 */
    q.m_colli_tbl = m_colli_tbl;
    if (hit == NULL)
        hit = &local;
    q.m_group = -1;
    q.m_func = dogModelHitCylinder;
    q.m_14 = 0;
    q.m_hit = hit;
    q._1c = 0;
    q.m_20 = 1;
    int ng = get_nGroup();
    ROR_ASSERT(ng >= 0); /* 556 */
    (void)ng;
    hit->m_50 = -100;
    hit->m_vec[0][3] = -1.0f;

    sceVu0FMATRIX mats[DOG_CAST_MAX_NODE];
    ROR_ASSERT(nNode <= 64); /* 562 */
    /* PC: the ROM writes past its 64 matrices after the assert. */
    if (nNode > DOG_CAST_MAX_NODE)
        nNode = DOG_CAST_MAX_NODE;
    for (int i = 0; i < nNode; i++)
        i3dMatUnit(mats[i]);
    i3cCastCylinder(&q, cyl[0], mats);

    if (hit->m_50 > -2)
        return hit->m_vec[0][3];
    return -1.0f;
}

// ROR 0x00114c54

void DogModel::clear()
{
    m_mode = -1;
    m_f4 = -1;
    m_clip_radius = -1.0f;
    m_shadow_light_prev = -1;
    m_disp_control.m_20 = 0;
    m_flag100 = 0;
    m_load_flag = LOAD_NONE;
    m_ref_count = 0;
    m_tex_info = NULL;
    m_pack_file = NULL;
    m_original = NULL;
    m_i3r_root = NULL;
    m_i3r_root2 = NULL;
    m_i3c_root = NULL;
    m_colli_tbl = NULL;
    m_hide = 0;
    m_attach_node = -1;
    m_attention = -1;
    m_mat_model = NULL;
    m_mat_local = NULL;
    m_mat_flag = NULL;
    m_light_id = 0;
    m_shadow_light = -1;
    m_shadow_fade = 0;
    m_16c = NULL;
    for (int i = 0; i < 4; i++)
    {
        m_disp_control.m_color[i] = 0.0f;
        m_disp_control.m_10[i] = 1.0f;
    }
    m_disp_control.m_24 = 1;
    m_disp_control.m_25 = 1;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x00114d04

void DogModel::setRoot(I3rRoot *p)
{
    m_mode = -1;
    m_f4 = -1;
    /* PC: CheckIdString reads the id; the ROM reads address 0 for NULL. */
    if (p != NULL)
        ROR_ASSERT(I3D_OK == I3rRoot__CheckIdString( p )); /* 677 */
    m_mat_flag = NULL;
    m_i3r_root = p;
    m_hide = 0;
    m_i3c_root = NULL;
    m_mat_model = NULL;
    m_mat_local = NULL;
    if (p != NULL)
        i3rRelocate(p);
}

// ROR 0x00114d8c

void DogModel::getWorldPos(float *out)
{
    DogModel *parent = parentModel(m_parent);
    const float *pos;
    sceVu0FMATRIX m;
    if (parent == NULL)
    {
        pos = m_trans;
    }
    else if (m_attach_node < 0)
    {
        parent->transform();
        pos = parent->m_matrix[3];
    }
    else
    {
        sceVu0FMATRIX *mat = parent->m_mat_model;
        parent->transform();
        i3dMatMul(m, parent->m_matrix, mat[m_attach_node]);
        pos = m[3];
    }
    out[0] = pos[0];
    out[1] = pos[1];
    out[2] = pos[2];
    out[3] = pos[3];
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x00114e38

int DogModel::transformBillboard(const float *theDir, const float *thePos)
{
    u_int mode = (int)m_flag100 >> 1;
    if (!(mode & 1))
        return 0;

    float ry;
    if (!(mode & 4))
    {
        /* Turned to the camera: its y rotation plus a half turn. */
        ROR_ASSERT(theDir); /* 779 */
        ry = theDir[1] + s_pi;
        if (ry >= s_pi)
            ry -= s_2pi;
        if (ry < -s_pi)
            ry += s_2pi;
        if (!(mode & 2))
        {
            i3dMatUnit(m_matrix);
            i3dMatRotY(ry, m_matrix, m_matrix);
            i3dMatTranslate(m_matrix, m_matrix, m_trans);
            i3dMatTranslate(m_matrix, m_matrix, m_trans_ofs);
            return 1;
        }
        i3dMatUnit(m_matrix);
        i3dMatRotX(theDir[0], m_matrix, m_matrix);
    }
    else
    {
        /* Turned towards thePos. */
        ROR_ASSERT(thePos); /* 748 */
        sceVu0FVECTOR d, rot;
        d[0] = thePos[0] - m_trans[0];
        d[1] = thePos[1] - m_trans[1];
        d[2] = thePos[2] - m_trans[2];
        d[3] = 1.0f;
        rot[0] = 0.0f;
        rot[1] = 0.0f;
        rot[2] = 0.0f;
        rot[3] = 1.0f;
        utlVecToRot(d, rot);
        i3dMatUnit(m_matrix);
        if (mode & 2)
            i3dMatRotX(rot[0], m_matrix, m_matrix);
        ry = rot[1];
    }
    i3dMatRotY(ry, m_matrix, m_matrix);
    i3dMatTranslate(m_matrix, m_matrix, m_trans);
    i3dMatTranslate(m_matrix, m_matrix, m_trans_ofs);
    return 1;
}

// ROR 0x001150c0

void DogModel::transform()
{
    int rebuilt = makeLocalMatrix(m_local_matrix);
    DogModel *parent = parentModel(m_parent);
    if (parent == NULL)
    {
        i3dMatCopy2(m_matrix, m_local_matrix);
    }
    else if (m_attach_node < 0)
    {
        parent->transform();
        i3dMatMul(m_matrix, parent->m_matrix, m_local_matrix);
    }
    else
    {
        /* Hung from a node: a freshly built local matrix is turned into
         * the node's axes first (x -> -x, y <-> -z). */
        sceVu0FMATRIX axes;
        memset(axes, 0, sizeof(axes));
        axes[0][0] = -1.0f;
        axes[1][2] = -1.0f;
        axes[2][1] = -1.0f;
        axes[3][3] = 1.0f;
        if (rebuilt)
            i3dMatMul(m_local_matrix, m_local_matrix, axes);
        i3dMatMul(m_matrix, parent->m_mat_model[m_attach_node], m_local_matrix);
        parent->transform();
        i3dMatMul(m_matrix, parent->m_matrix, m_matrix);
    }
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x001151f4

void DogModel::draw(I3dLayer *layer, const float *cam_rotate, const float *cam_trans)
{
    if (!transformBillboard(cam_rotate, cam_trans))
        transform();
    if (m_hide == 0)
    {
        ROR_ASSERT(I3D_OK == I3rRoot__CheckIdString( m_i3r_root )); /* 899 */
        utlDrawI3r(layer, m_i3r_root, m_mat_model, m_matrix, 0);
    }
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x001152a8

void DogModel::drawUnitMatrix(I3dLayer *layer)
{
    sceVu0FMATRIX m;
    i3dMatUnit(m);
    ROR_ASSERT(I3D_OK == I3rRoot__CheckIdString( m_i3r_root )); /* 924 */
    utlDrawI3r(layer, m_i3r_root, NULL, m, 0);
}

// ROR 0x0011531c

void DogModel::setFlag100Bits(int v)
{
    m_flag100 = (m_flag100 & ~0xeu) | ((v & 7) << 1);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30, I3rRoot__CheckIdString, i3rGetNumNode, i3rCopyLocalBasicTransform, i3rConvertLocalToModel (the assert handler; isAlive, get_nNode, copyLocalBasicTransform and convertLocalToModel inline)
// ROR 0x0011533c

void DogModel::allocMatrix()
{
    if (isAlive() && m_mat_local == NULL)
    {
        int n = get_nNode();
        /* One block: n local matrices, n model matrices, n flag words. */
        m_mat_local = (sceVu0FMATRIX *)g_utlMemory.alloc(n * 0x84, -1);
        ROR_ASSERT(NULL != m_mat_local); /* 1053 */
        /* PC: the ROM hangs in the assert; no block, no pointers into it. */
        if (m_mat_local == NULL)
            return;
        m_mat_model = m_mat_local + n;
        m_mat_flag = (int *)(m_mat_local + 2 * n);
        memset(m_mat_flag, 0, n * 4);
        copyLocalBasicTransform(m_mat_local);
        convertLocalToModel(m_mat_model, m_mat_local);
    }
}

// ROR 0x001154cc

void DogModel::freeMatrix()
{
    if (m_mat_local != NULL)
    {
        g_utlMemory.free(m_mat_local);
        m_mat_local = NULL;
        m_mat_model = NULL;
    }
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30, I3rRoot__CheckIdString, i3rGetNumNode, i3rGetNodeNameByIndex (the assert handler; isAlive, get_nNode and getNodeName inline)
// ROR 0x0011550c

void DogModel::setupNodeFlags()
{
    if (!isAlive())
        return;
    ROR_ASSERT(m_mat_flag); /* 1093 */
    /* PC: no flag words to write (allocMatrix failed). */
    if (m_mat_flag == NULL)
        return;
    int n = get_nNode();
    for (int i = 0; i < n; i++)
    {
        const char *name = getNodeName(i);
        /* PC: a node without a name (the ROM reads address 0). */
        if (name == NULL)
            continue;
        if (name[0] == 'b' && name[2] == '_')
        {
            if (name[1] == 'a')
                m_mat_flag[i] = 1;
            else if (name[1] == 'y')
                m_mat_flag[i] = 2;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x00115694

u_short *DogModel::setupColliTbl()
{
    if (m_i3c_root == NULL)
    {
        m_colli_tbl = NULL;
        return NULL;
    }
    ROR_ASSERT(m_colli_tbl == NULL); /* 1131 */
    int n = i3cGetNumNode(m_i3c_root);
    m_colli_tbl = (u_short *)g_utlMemory.alloc(n * 2, -1);
    ROR_ASSERT(m_colli_tbl); /* 1139 */
    for (int i = 0; i < n; i++)
    {
        int node = i3rGetNodeIndexByName(m_i3r_root, i3cGetNodeName(m_i3c_root, i & 0xffff));
        m_colli_tbl[i] = node < 0 ? 0xffff : (u_short)node;
    }
    return m_colli_tbl;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30, i3rAssignTex (the assert handler; assignTex inline)
// ROR 0x00115790

int DogModel::loadData(DogTexInfo *tex, void *data, void *i3c)
{
    setRoot((I3rRoot *)data);
    if (assignTex(tex) != 0)
        ROR_ASSERT(0); /* 1339 */

    m_i3c_root = (I3cRoot *)i3c;
    if (i3c != NULL)
    {
        if (i3cRelocate(m_i3c_root) != 0)
        {
            ROR_ASSERT(0); /* 1353 */
            return -1;
        }
        setupColliTbl();
        i3cSetBothFaceEnable(m_i3c_root, 1);
    }
    /* The ROM's own double call (the second returns at once). */
    if (m_mat_model == NULL)
        allocMatrix();
    allocMatrix();

    m_load_flag = LOAD_REF;
    m_ref_count = 1;
    m_tex_info = tex;
    m_original = NULL;
    m_pack_file = NULL;
    reset();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30, i3rAssignTex, BedPfh (the assert handler, also BedPfh's getFile / getNumFile inline; assignTex inline; BedPfh is the textures constructor)
// ROR 0x001158ec

int DogModel::loadPack(DogTexInfo *tex, const BedPfh &pfh, int sepia, int sepia_disp)
{
    ROR_ASSERT(m_pack_file == NULL); /* 1391 */
    ROR_ASSERT(m_load_flag == LOAD_NONE); /* 1392 */
    ROR_ASSERT(m_ref_count == 0); /* 1393 */
    m_pack_file = pfh.m_top;
    if (m_pack_file == NULL)
        return -1;

    /* File 0: the textures. */
    BedPfh textures(pfh.getFile(0));
    tex->addPfh(&textures, NULL, 0, sepia);
    /* File 1: the model. */
    setRoot((I3rRoot *)pfh.getFile(1));
    if (sepia && sepia_disp)
    {
        m_disp_control.m_color[3] = 0.0f;
        m_disp_control.m_color[0] = 1.0f;
        m_disp_control.m_10[2] = 0.0f;
        m_disp_control.m_10[1] = 0.0f;
        m_disp_control.m_10[0] = 0.0f;
        m_disp_control.m_10[3] = 1.0f;
        m_disp_control.m_color[2] = 1.0f;
        m_disp_control.m_color[1] = 1.0f;
        utlSetI3rDispControl(m_i3r_root, &m_disp_control);
    }
    if (assignTex(tex) != 0)
    {
        tex->dump();
        ROR_ASSERT(0); /* 1438 */
    }
    /* File 2, when the pack has a non-empty one: a second root. */
    if (pfh.getNumFile() > 2 && pfh.getFileSize(2) > 0)
    {
        m_i3r_root2 = (I3rRoot *)pfh.getFile(2);
        if (m_i3r_root2 != NULL)
            i3rRelocate(m_i3r_root2);
    }
    allocMatrix();

    m_load_flag = LOAD_REF;
    m_ref_count = 1;
    m_tex_info = tex;
    m_original = NULL;
    reset();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30 (the assert handler)
// ROR 0x00115c60

int DogModel::share(DogModel *theOriginal)
{
    ROR_ASSERT(theOriginal); /* 1524 */
    theOriginal->m_ref_count++;
    m_ref_count = 1;
    m_load_flag = LOAD_SHARE;
    m_original = NULL;
    m_tex_info = NULL;
    reset();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30, BedPfh (the assert handler, also BedPfh's getFile inline; BedPfh is the pfh constructor)
// ROR 0x00115cec

void DogModel::release(int force)
{
    if (!force)
    {
        int n = m_ref_count > 0 ? --m_ref_count : 0;
        if (n > 0)
            return;
    }
    else
    {
        m_ref_count = 0;
    }
    ROR_ASSERT(m_ref_count <= 0); /* 1565 */
    ROR_ASSERT(m_load_flag != LOAD_NONE); /* 1570 */

    switch (m_load_flag)
    {
    case LOAD_NONE:
        break;

    case LOAD_PACK:
    {
        if (m_colli_tbl != NULL)
        {
            g_utlMemory.free(m_colli_tbl);
            m_colli_tbl = NULL;
        }
        ROR_ASSERT(m_pack_file); /* 1591 */
        ROR_ASSERT(m_tex_info); /* 1594 */
        /* The pack's file 0 by the first word of the table after the
         * header (no BedPfh here in the ROM). */
        const int *table = (const int *)(m_pack_file + 1);
        m_tex_info->releasePfh((const BedPfhHead *)((const u_char *)m_pack_file + table[0]));
        g_utlMemory.free(m_pack_file);
        m_pack_file = NULL;
        break;
    }

    case LOAD_DATA:
        if (m_colli_tbl != NULL)
        {
            g_utlMemory.free(m_colli_tbl);
            m_colli_tbl = NULL;
        }
        if (m_i3c_root != NULL)
        {
            g_utlMemory.free(m_i3c_root);
            m_i3c_root = NULL;
        }
        ROR_ASSERT(m_tex_info); /* 1620 */
        m_tex_info->releaseModel(this);
        ROR_ASSERT(m_i3r_root); /* 1625 */
        g_utlMemory.free(m_i3r_root);
        break;

    case LOAD_SHARE:
        ROR_ASSERT(m_original); /* 1632 */
        m_original->release(0);
        break;

    case LOAD_REF:
        if (m_colli_tbl != NULL)
        {
            g_utlMemory.free(m_colli_tbl);
            m_colli_tbl = NULL;
        }
        if (m_pack_file != NULL)
        {
            ROR_ASSERT(m_tex_info); /* 1652 */
            BedPfh pfh(m_pack_file);
            m_tex_info->releasePfh((const BedPfhHead *)pfh.getFile(0));
            m_pack_file = NULL;
        }
        break;

    default:
        ROR_ASSERT(0); /* 1664 */
        break;
    }

    if (m_mat_local != NULL)
        g_utlMemory.free(m_mat_local);
    if (m_16c != NULL)
        g_utlMemory.free(m_16c);
    clear();
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FUN_00215a30, i3rCopyClipSphere (the assert handler; copyClipSphere inline)
// ROR 0x00116038

float DogModel::getClipRadius()
{
    if (m_clip_radius < 0.0f)
    {
        sceVu0FVECTOR center;
        copyClipSphere(center, &m_clip_radius);
    }
    return m_clip_radius;
}

// ROR 0x00282620

DogModel::DogModel()
{
    m_mode = 0;
    m_f4 = 0;
    m_f8 = 0;
    m_fc = 0;
    clear();
}

// ROR 0x002826f4

DogModel::DogModel(I3rRoot *p)
{
    m_mode = 0;
    m_f4 = 0;
    m_f8 = 0;
    m_fc = 0;
    setRoot(p);
}
