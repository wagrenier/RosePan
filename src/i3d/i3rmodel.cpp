/* ==========================================================================
 *  i3d/i3rmodel.cpp
 *
 *  The i3r model helpers (ROR 0x21a3e8..0x21ac80): the root's packets,
 *  texture assignment and animation, STQ map entities, the clip sphere and
 *  the node transforms. See i3rmodel.h for the item tree they walk.
 *
 *  The ROM reaches the children through tiny per-type accessors
 *  (0x22a890.. / 0x22abc0.. / 0x22b170: an item's child k, or whether its
 *  data is set); here they are I3rItem's getChildren() / m_data reads.
 *
 *  PORT: the ROM walks most lists with do-while loops that assume at least
 *  one entry; the host uses counted loops (an empty list does nothing).
 * ======================================================================== */
#include "i3d/i3rmodel.h"

#include <stdio.h>
#include <string.h>

#include "lib/unassigned.h"

/* The root item's data. */
static inline I3rRootData *i3rModelData(const I3rRoot *root)
{
    return (I3rRootData *)root->m_item.getData();
}

/* PORT: an STQ slot may point outside the file (an entity the game owns);
 * the host field still holds the distance, so it must fit in 32 bits.
 * One that does not (DogActor::setEyeTex's m_eye_uv in the global actor
 * array against a heap model) is left NULL -- a truncated distance would
 * be a wild pointer for the reader -- and logged once; a side table is
 * the real fix (open-items.md). */
static void i3rSetFarPtr(u_int *field, const void *p)
{
    static int s_logged;
    ptrdiff_t d = p != NULL ? (const u_char *)p - (const u_char *)field : 0;

    if (d != (ptrdiff_t)(int)d)
    {
        if (!s_logged)
            printf("[i3r] %p is too far from the model slot %p for a 32-bit field (left NULL)\n", p, (void *)field);
        s_logged = 1;
        p = NULL;
    }
    i3rSetPtr(field, p);
}

/* out = a times b as VU0 micro 0 multiplies them (vf4..7 = a, vf8..11 =
 * b, the result in vf8..11): row i = sum over j of b[i][j] * a[j]. */
static void i3rMatMul(I3dMatrixF4x4 *out, const I3dMatrixF4x4 *a, const I3dMatrixF4x4 *b)
{
    I3dMatrixF4x4 r;
    int i, k;

    for (i = 0; i < 4; i++)
        for (k = 0; k < 4; k++)
            r.m[i][k] = a->m[0][k] * b->m[i][0] + a->m[1][k] * b->m[i][1] +
                        a->m[2][k] * b->m[i][2] + a->m[3][k] * b->m[i][3];
    *out = r;
}

/* A mesh list's packets: every entry's child 0's children are meshes. */
static void i3rBuildListPackets(const I3rItem *list, void *buf)
{
    I3rItem *entry = list->getChildren();
    u_int i, j;

    for (i = 0; i < list->getNumChild(); i++)
    {
        I3rItem *meshes = &entry[i].getChildren()[0];
        I3rItem *mesh = meshes->getChildren();

        for (j = 0; j < meshes->getNumChild(); j++)
            i3rBuildMeshPacket(&mesh[j], buf);
    }
}

// ROR 0x0021a3e8
// callcheck: FUN_0022b170, FUN_0022abc8, FUN_0022abc0, FUN_0022abe8, FUN_0022abd8, i3rBuildMeshPacket (the item accessors: getChildren / m_data reads; the mesh builder is called in i3rBuildListPackets)
int i3rBuildPackets(I3rRoot *root)
{
    I3rItem *child = root->m_item.getChildren();
    I3rPacketBuf *packets = (I3rPacketBuf *)child[6].getData();
    /* ROM: every mesh gets the same buffer start (s2 is never advanced). */
    void *buf = i3rPtr(&packets->m_buf);
    I3rItem *mesh = child[2].getChildren();
    u_int i;

    for (i = 0; i < child[2].getNumChild(); i++)
    {
        I3rItem *part = mesh[i].getChildren();

        if (part[0].m_data != 0)
            i3rBuildListPackets(&part[0].getChildren()[0], buf);
        if (part[1].m_data != 0)
            i3rBuildListPackets(&part[1].getChildren()[1], buf);
    }
    return I3D_OK;
}

// ROR 0x0021a558
// callcheck: FUN_0022b170 (the item accessor: child 6)
int i3rClearPackets(I3rRoot *root)
{
    I3rPacketBuf *packets = (I3rPacketBuf *)root->m_item.getChildren()[6].getData();
    u_int *q = (u_int *)i3rPtr(&packets->m_buf);
    u_int i;

    for (i = 0; i < packets->m_qwc; i++)
    {
        q[0] = 0;
        q[1] = 0;
        q[2] = 0;
        q[3] = 0;
        q += 4;
    }
    return I3D_OK;
}

// ROR 0x0021a5b0
int i3rAssignTex(I3rRoot *root, const I3dTexEntry *tex, int n)
{
    I3rItem *mats = &root->m_item.getChildren()[0];
    u_int i, j, k;

    for (i = 0; i < mats->getNumChild(); i++)
    {
        I3rItem *layers = &mats->getChildren()[i].getChildren()[0];

        for (j = 0; j < layers->getNumChild(); j++)
        {
            I3rLayerData *layer = (I3rLayerData *)layers->getChildren()[j].getData();
            const char *name = (const char *)i3rPtr(&layer->m_tex_name);
            int index = -1;

            if (*name != '\0' && n != 0)
            {
                for (k = 0; k < (u_int)n; k++)
                {
                    if (tex[k].pszTexName != NULL && strcmp(name, tex[k].pszTexName) == 0)
                    {
                        index = (int)k;
                        break;
                    }
                }
            }
            layer->m_tex_index = index;
        }
    }
    return I3D_OK;
}

// ROR 0x0021a700
int i3rCopyLocalBasicTransform(I3rRoot *root, I3dMatrixF4x4 *mat)
{
    u_int n = root->m_item.getChildren()[3].getNumChild();
    const I3dMatrixF4x4 *basic = (const I3dMatrixF4x4 *)i3rPtr(&i3rModelData(root)->m_local_basic);
    u_int i;

    for (i = 0; i < n; i++)
        mat[i] = basic[i];
    return I3D_OK;
}

// ROR 0x0021a760
int i3rConvertLocalToModel(I3rRoot *root, I3dMatrixF4x4 *model, const I3dMatrixF4x4 *local)
{
    I3rItem *nodes = &root->m_item.getChildren()[3];
    u_int i;

    for (i = 0; i < nodes->getNumChild(); i++)
    {
        const I3rNodeData *node = (const I3rNodeData *)nodes->getChildren()[i].getData();

        if (node->m_parent < 0)
            model[i] = local[i];
        else
            i3rMatMul(&model[i], &model[node->m_parent], &local[i]);
    }
    return I3D_OK;
}

// ROR 0x0021a828
int i3rCopyClipSphere(I3rRoot *root, float *center, float *radius)
{
    const I3rRootData *data = i3rModelData(root);

    *radius = data->m_clip_radius;
    center[0] = data->m_clip_center[0];
    center[1] = data->m_clip_center[1];
    center[2] = data->m_clip_center[2];
    center[3] = 1.0f;
    return I3D_OK;
}

// ROR 0x0021a8e0
int i3rUpdateTexAnim(I3rRoot *root)
{
    I3rItem *mats = &root->m_item.getChildren()[0];
    u_int i;

    for (i = 0; i < mats->getNumChild(); i++)
    {
        I3rMaterialData *mat = (I3rMaterialData *)mats->getChildren()[i].getData();

        if (mat->m_tex_anim != 0)
            mat->m_tex_frame++;
    }
    return I3D_OK;
}

// ROR 0x0021a938
int i3rReplaceStqMapEntity(I3rRoot *root, I3rStqMapEntity *entity)
{
    u_char id = entity->m_id;
    I3rItem *mats = &root->m_item.getChildren()[0];
    u_int i, j, k;

    for (i = 0; i < mats->getNumChild(); i++)
    {
        I3rItem *mat = &mats->getChildren()[i];
        I3rItem *layers = &mat->getChildren()[0];

        for (j = 0; j < layers->getNumChild(); j++)
        {
            I3rItem *slots = &layers->getChildren()[j].getChildren()[3];

            for (k = 0; k < slots->getNumChild(); k++)
            {
                I3rItem *slot = &slots->getChildren()[k];
                const I3rStqMapEntity *cur = (const I3rStqMapEntity *)slot->getData();

                /* PORT: the ROM reads an empty slot's id at address 0x41. */
                if (cur != NULL && cur->m_id == id)
                {
                    i3rSetFarPtr(&slot->m_data, entity);
                    ((I3rMaterialData *)mat->getData())->m_tex_anim = 1;
                }
            }
        }
    }
    return I3D_OK;
}

/* A material's entity slot n (its child 1's child n), NULL when the slot
 * list or the slot is empty. */
static I3rItem *i3rMatEntitySlot(const I3rItem *mat, int n)
{
    I3rItem *slots = &mat->getChildren()[1];

    if (slots->m_data == 0)
        return NULL;
    if (slots->getChildren()[n].m_data == 0)
        return NULL;
    return &slots->getChildren()[n];
}

// ROR 0x0021aa10
// callcheck: FUN_0022a988, FUN_0022a978, FUN_0022a898, FUN_0022a890 (the item accessors, in i3rMatEntitySlot)
void *i3rGetMatEntity0(const I3rItem *mat)
{
    I3rItem *slot = i3rMatEntitySlot(mat, 0);

    return slot != NULL ? slot->getData() : NULL;
}

// ROR 0x0021aa70
// callcheck: FUN_0022a988, FUN_0022a978, FUN_0022a8b8, FUN_0022a8a8 (the item accessors, in i3rMatEntitySlot)
void *i3rGetMatEntity1(const I3rItem *mat)
{
    I3rItem *slot = i3rMatEntitySlot(mat, 1);

    return slot != NULL ? slot->getData() : NULL;
}

// ROR 0x0021aad0
// callcheck: FUN_0022a988, FUN_0022a978, FUN_0022a8d8, FUN_0022a8c8 (the item accessors, in i3rMatEntitySlot)
void *i3rGetMatEntity2(const I3rItem *mat)
{
    I3rItem *slot = i3rMatEntitySlot(mat, 2);

    return slot != NULL ? slot->getData() : NULL;
}

// ROR 0x0021ab30
// callcheck: FUN_0022a988, FUN_0022a978, FUN_0022a898, FUN_0022a890 (the item accessors, in i3rMatEntitySlot)
int i3rSetMatEntity0(I3rItem *mat, void *entity)
{
    I3rItem *slot = i3rMatEntitySlot(mat, 0);

    if (slot == NULL)
        return 1;
    i3rSetFarPtr(&slot->m_data, entity);
    return 0;
}

// ROR 0x0021aba0
// callcheck: FUN_0022a988, FUN_0022a978, FUN_0022a8b8, FUN_0022a8a8 (the item accessors, in i3rMatEntitySlot)
int i3rSetMatEntity1(I3rItem *mat, void *entity)
{
    I3rItem *slot = i3rMatEntitySlot(mat, 1);

    if (slot == NULL)
        return 1;
    i3rSetFarPtr(&slot->m_data, entity);
    return 0;
}

// ROR 0x0021ac10
// callcheck: FUN_0022a988, FUN_0022a978, FUN_0022a8d8, FUN_0022a8c8 (the item accessors, in i3rMatEntitySlot)
int i3rSetMatEntity2(I3rItem *mat, void *entity)
{
    I3rItem *slot = i3rMatEntitySlot(mat, 2);

    if (slot == NULL)
        return 1;
    i3rSetFarPtr(&slot->m_data, entity);
    return 0;
}
