/* ==========================================================================
 *  i3d/i3rroot.cpp
 *
 *  The I3D_BIN model root (ROR 0x22b1b0..0x22b4b8): the id check, the
 *  relocation driver, and the root data's texture and node name lookups.
 *  See i3rroot.h for the file layout and the host's relative pointers.
 *  The item relocators and the per-type accessors / fixups around this
 *  file are still unassigned (unassigned@00228c08; the I3D_I3M root is i3d/i3mroot.cpp).
 * ======================================================================== */
#include "i3d/i3rroot.h"

#include <string.h>

#include <unordered_map>

#include "lib/unassigned.h"

/* The root item's data. */
static inline I3rRootData *i3rRootData(const I3rRoot *root)
{
    return (I3rRootData *)root->m_item.getData();
}

// PC: the far pointer side table (i3rroot.h I3R_FAR_PTR). Entries are
// only read while their field holds the marker, so a stale one is harmless.
static std::unordered_map<const u_int *, void *> s_far_ptrs;

void *i3rFarPtrGet(const u_int *field)
{
    auto it = s_far_ptrs.find(field);
    return it != s_far_ptrs.end() ? it->second : NULL;
}

void i3rFarPtrSet(u_int *field, const void *p)
{
    s_far_ptrs[field] = (void *)p;
    *field = I3R_FAR_PTR;
}

/* ROR 0x0022b1b0 */
int i3rFindName(const u_int *names, u_int count, const char *name)
{
    u_int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp((const char *)i3rPtr(&names[i]), name) == 0)
            return (int)i;
    }
    return -1;
}

/* ROR 0x0022b228 */
int I3rRoot__CheckIdString(const I3rRoot *root)
{
    /* The type byte is the top of the root item's info word (+0x17). */
    if (memcmp(root->m_id, "I3D_BIN", 8) != 0 || root->m_item.getType() != I3R_TYPE_ROOT)
        return 3;
    return I3D_OK;
}

/* ROR 0x0022b280 */
int i3rSetRelocate(I3rRoot *root, int on)
{
    int result;

    if (I3rRoot__CheckIdString(root) != I3D_OK)
        return 3;
    if (root->m_version != I3R_VERSION)
        return 4;
    if ((int)root->m_relocated == on)
        return I3D_OK;

    if (on == 0)
    {
        i3rClearPackets(root);
        i3rItemUnrelocate(&root->m_item, &root->m_item);
        root->m_relocated = 0;
    }
    else if (on == 1)
    {
        result = i3rItemRelocate(&root->m_item, &root->m_item);
        if (result != I3D_OK)
            return result;
        root->m_relocated = 1;
        i3rBuildPackets(root);
    }
    return I3D_OK;
}

/* ROR 0x0022b330 */
int i3rRelocate(I3rRoot *root)
{
    i3rSetRelocate(root, 1);
    return I3D_OK;
}

/* ROR 0x0022b350 */
int i3rUnrelocate(I3rRoot *root)
{
    i3rSetRelocate(root, 0);
    return I3D_OK;
}

/* ROR 0x0022b370 */
int i3rGetNumNode(const I3rRoot *root)
{
    return (int)root->m_item.getChildren()[3].getNumChild();
}

/* ROR 0x0022b388 */
const char *i3rGetNodeNameByIndex(const I3rRoot *root, int index)
{
    I3rRootData *data = i3rRootData(root);
    const u_short *indices = (const u_short *)i3rPtr(&data->m_node_index);
    u_int *names = (u_int *)i3rPtr(&data->m_node_name);
    u_int i;

    /* ROM: index arrives as a 64-bit register and is compared whole. */
    for (i = 0; i < data->m_num_node_name; i++)
    {
        if (indices[i] == (u_int)index)
            return (const char *)i3rPtr(&names[i]);
    }
    return NULL;
}

/* ROR 0x0022b3d8 */
int i3rGetNodeIndexByName(I3rRoot *root, const char *name)
{
    I3rRootData *data = i3rRootData(root);
    int n = i3rFindName((const u_int *)i3rPtr(&data->m_node_name), data->m_num_node_name, name);

    if (n < 0)
        return -1;
    return ((const u_short *)i3rPtr(&data->m_node_index))[n];
}

/* ROR 0x0022b428 */
int i3rGetNumTex(const I3rRoot *root)
{
    return i3rRootData(root)->m_num_tex;
}

/* ROR 0x0022b438 */
const char *i3rGetTexName(const I3rRoot *root, int n)
{
    u_int *names = (u_int *)i3rPtr(&i3rRootData(root)->m_tex_name);

    return (const char *)i3rPtr(&names[n]);
}

/* ROR 0x0022b450 */
int i3rGetTexIndexByName(const I3rRoot *root, const char *name)
{
    I3rRootData *data = i3rRootData(root);

    return i3rFindName((const u_int *)i3rPtr(&data->m_tex_name), data->m_num_tex, name);
}

/* ROR 0x0022b478 */
u_int i3rGetRootParam24(const I3rRoot *root)
{
    return i3rRootData(root)->m_param24;
}

/* ROR 0x0022b488 */
void *i3rGetRootMatrix10(const I3rRoot *root, int i)
{
    return (u_char *)i3rPtr(&i3rRootData(root)->m_local_basic) + i * 0x40;
}

/* ROR 0x0022b4a0 */
void *i3rGetRootMatrix14(const I3rRoot *root, int i)
{
    return (u_char *)i3rPtr(&i3rRootData(root)->m_matrix14) + i * 0x40;
}
