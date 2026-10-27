/* ==========================================================================
 *  i3d/i3mroot.cpp
 *
 *  The I3D_I3M motion file root (ROR 0x22b4b8..0x22ba20): the id and
 *  version checks, the relocators over its tables and nodes, and the node
 *  and key lookups. See i3mroot.h for the layout and why the host keeps
 *  the file's offsets (the relocators change nothing).
 * ======================================================================== */
#include "i3d/i3mroot.h"

#include <string.h>

/* ROR 0x0022b4b8 */
int I3mRoot__CheckIdString(const I3mRoot *root)
{
    if (memcmp(root->m_id, "I3D_I3M", 8) != 0)
        return 3;
    return I3D_OK;
}

/* ROR 0x0022b4e8 */
void i3mRelocField(u_int *field, int delta)
{
    if (*field != 0)
        *field += delta;
}

/* ROR 0x0022b500 */
int I3mRoot__CheckVersion(const I3mRoot *root)
{
    return root->m_version == I3M_VERSION;
}

// ──────────────────────────────────────────────────────────────────────
// The four root tables first, then each node's name and keys; the
// relocation u16 goes to 0.
// ROR 0x0022b518

int i3mRelocate(I3mRoot *root)
{
    I3mNode *node;
    u_int i;
    int ret;

    ret = I3mRoot__CheckIdString(root);
    if (ret != I3D_OK)
        return ret;
    if (!I3mRoot__CheckVersion(root))
        return 4;
    if (root->m_relocate == 0)
        return I3D_OK;

    /* ROM: the delta is the root's address. The host keeps offsets from
     * the root, so the delta is 0 and the relocation u16 stays 1. */
    i3mRelocField(&root->m_node, 0);
    i3mRelocField(&root->m_20, 0);
    i3mRelocField(&root->m_float, 0);
    i3mRelocField(&root->m_28, 0);
    node = (I3mNode *)i3mPtr(root, root->m_node);
    for (i = 0; i < root->m_num_node; i++, node++)
    {
        i3mRelocField(&node->m_name, 0);
        i3mRelocField(&node->m_key, 0);
    }
    return I3D_OK;
}

// ──────────────────────────────────────────────────────────────────────
// The reverse: each node's name and keys, then +0x20, +0x24, +0x28 and
// the node table last; the relocation u16 goes to 1.
// ROR 0x0022b5f8

int i3mUnrelocate(I3mRoot *root)
{
    I3mNode *node;
    u_int i;
    int ret;

    ret = I3mRoot__CheckIdString(root);
    if (ret != I3D_OK)
        return ret;
    if (!I3mRoot__CheckVersion(root))
        return 4;
    if (root->m_relocate == 1)
        return I3D_OK;

    /* ROM: the delta is minus the root's address; 0 on the host (and the
     * host never leaves state 1, so this is not reached). */
    node = (I3mNode *)i3mPtr(root, root->m_node);
    for (i = 0; i < root->m_num_node; i++, node++)
    {
        i3mRelocField(&node->m_name, 0);
        i3mRelocField(&node->m_key, 0);
    }
    i3mRelocField(&root->m_20, 0);
    i3mRelocField(&root->m_float, 0);
    i3mRelocField(&root->m_28, 0);
    i3mRelocField(&root->m_node, 0);
    root->m_relocate = 1;
    return I3D_OK;
}

/* ROR 0x0022b6f8 */
int i3mGetNumNode(const I3mRoot *root)
{
    if (I3mRoot__CheckIdString(root) != I3D_OK)
        return -1;
    return root->m_num_node;
}

/* ROR 0x0022b730 */
const char *i3mGetNodeName(const I3mRoot *root, int i)
{
    const I3mNode *node;

    if (I3mRoot__CheckIdString(root) != I3D_OK || (u_int)(i & 0xffff) >= root->m_num_node)
        return NULL;
    node = (const I3mNode *)i3mPtr(root, root->m_node);
    return (const char *)i3mPtr(root, node[i & 0xffff].m_name);
}

/* ROR 0x0022b790 */
int i3mGetNodeIndexByName(const I3mRoot *root, const char *name)
{
    const I3mNode *node = (const I3mNode *)i3mPtr(root, root->m_node);
    int i;

    for (i = 0; i < (int)root->m_num_node; i++)
    {
        if (strcmp(name, (const char *)i3mPtr(root, node[i].m_name)) == 0)
            return i;
    }
    return -1;
}

/* ROR 0x0022b820 */
const I3mNode *i3mGetNode(const I3mRoot *root, int i)
{
    if (I3mRoot__CheckIdString(root) != I3D_OK || (u_int)(i & 0xffff) >= root->m_num_node)
        return NULL;
    return (const I3mNode *)i3mPtr(root, root->m_node) + (i & 0xffff);
}

/* ROR 0x0022b878 */
int i3mGetNodeNumKey(const I3mRoot *root, int i)
{
    const I3mNode *node;

    if (I3mRoot__CheckIdString(root) != I3D_OK || (u_int)(i & 0xffff) >= root->m_num_node)
        return 0xffff;
    node = (const I3mNode *)i3mPtr(root, root->m_node);
    return node[i & 0xffff].m_num_key;
}

// ──────────────────────────────────────────────────────────────────────
// A key's time: its first u16 indexes the float table.
// ROR 0x0022b8d8

float i3mGetNodeKeyTime(const I3mRoot *root, int i, int k)
{
    const I3mNode *node;
    const u_short *key;
    const float *time;

    k &= 0xffff;
    if (I3mRoot__CheckIdString(root) != I3D_OK || (u_int)(i & 0xffff) >= root->m_num_node)
        return 0.0f;
    node = (const I3mNode *)i3mPtr(root, root->m_node) + (i & 0xffff);
    if ((u_int)k >= node->m_num_key)
        return 0.0f;
    key = (const u_short *)i3mPtr(root, node->m_key);
    time = (const float *)i3mPtr(root, root->m_float);
    return time[key[root->m_key_stride * k]];
}

/* ROR 0x0022b970 */
int i3mGetNodeKeyTimes(const I3mRoot *root, float *out, int i)
{
    const I3mNode *node;
    const u_short *key;
    const float *time;
    u_int n;

    if (I3mRoot__CheckIdString(root) != I3D_OK || (u_int)(i & 0xffff) >= root->m_num_node)
        return 0xffff;
    node = (const I3mNode *)i3mPtr(root, root->m_node) + (i & 0xffff);
    key = (const u_short *)i3mPtr(root, node->m_key);
    time = (const float *)i3mPtr(root, root->m_float);
    for (n = node->m_num_key; n != 0; n--)
    {
        *out++ = time[*key];
        key += root->m_key_stride;
    }
    return i & 0xffff;
}
