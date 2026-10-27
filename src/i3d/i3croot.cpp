/* ==========================================================================
 *  i3d/i3croot.cpp
 *
 *  The I3D_I3C collision file root (ROR 0x22ba20..0x22c1f8): the id check,
 *  the relocators over its nodes, parts and part trees, and the node /
 *  group lookups. See i3croot.h for the layout and why the host keeps the
 *  file's offsets (the relocators only clear the -1 "none" marks).
 * ======================================================================== */
#include "i3d/i3croot.h"

#include <string.h>

/* What the ROM's relocators do to a field the host keeps as an offset:
 * -1 (none) becomes 0. */
static inline void i3cClearNone(u_int *field)
{
    if (*field == 0xffffffffu)
        *field = 0;
}

/* ROR 0x0022ba20 */
int I3cRoot__CheckIdString(const I3cRoot *root)
{
    if (memcmp(root->m_id, "I3D_I3C", 8) != 0)
        return 3;
    return I3D_OK;
}

// ──────────────────────────────────────────────────────────────────────
// The tree's child offset first, then (not a leaf) each child.
// ROR 0x0022ba50

void i3cTreeRelocate(I3cTree *tree, I3cRoot *root)
{
    u_int n = tree->m_info;
    I3cTree *child;

    /* ROM: only a value below the root's address (still an offset) gets
     * the root added; the host always holds offsets. */
    i3cClearNone(&tree->m_child);
    if (n != 0 && (tree->m_info & I3C_TREE_LEAF) == 0)
    {
        child = (I3cTree *)i3cPtr(root, tree->m_child);
        for (; n != 0; n--)
        {
            i3cTreeRelocate(child, root);
            child++;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// The reverse: (not a leaf) the children first, then the child offset.
// ROR 0x0022bae8

void i3cTreeUnrelocate(I3cTree *tree, I3cRoot *root)
{
    u_int n = tree->m_info;
    I3cTree *child;
    int i;

    if (n != 0 && (tree->m_info & I3C_TREE_LEAF) == 0)
    {
        child = (I3cTree *)i3cPtr(root, tree->m_child);
        for (i = 0; i < (int)n; i++)
        {
            i3cTreeUnrelocate(child, root);
            child++;
        }
    }
    /* ROM: only a value at or above the root's address gets it subtracted. */
    i3cClearNone(&tree->m_child);
}

// ──────────────────────────────────────────────────────────────────────
// The node table, each node's name and parts (each part's tree, +0x24,
// +0x2c, then its tree's nodes), then the group name table; the root's
// relocation byte goes to 0.
// ROR 0x0022bba0

int i3cRelocate(I3cRoot *root)
{
    I3cNode *node;
    I3cPart *part;
    u_int i, j;
    u_int *name;

    if (root->m_relocate == 0)
        return 0;

    i3cClearNone(&root->m_node);
    node = (I3cNode *)i3cPtr(root, root->m_node);
    for (i = 0; i < root->m_num_node; i++, node++)
    {
        i3cClearNone(&node->m_name);
        i3cClearNone(&node->m_part);
        part = (I3cPart *)i3cPtr(root, node->m_part);
        for (j = 0; j < node->m_num_part; j++, part++)
        {
            i3cClearNone(&part->m_tree);
            i3cClearNone(&part->m_24);
            i3cClearNone(&part->m_2c);
            i3cTreeRelocate((I3cTree *)i3cPtr(root, part->m_tree), root);
        }
    }

    i3cClearNone(&root->m_group_name);
    name = (u_int *)i3cPtr(root, root->m_group_name);
    for (i = 0; i < root->m_num_group; i++)
        i3cClearNone(&name[i]);

    root->m_relocate = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The reverse of i3cRelocate: the group names, then the nodes (each part's
// tree first), then the node table; the relocation byte goes to 1.
// ROR 0x0022bdb0

int i3cUnrelocate(I3cRoot *root)
{
    I3cNode *node;
    I3cPart *part;
    u_int i, j;
    u_int *name;

    if (root->m_relocate == 1)
        return 0;

    name = (u_int *)i3cPtr(root, root->m_group_name);
    for (i = 0; i < root->m_num_group; i++)
        i3cClearNone(&name[i]);
    i3cClearNone(&root->m_group_name);

    node = (I3cNode *)i3cPtr(root, root->m_node);
    for (i = 0; i < root->m_num_node; i++, node++)
    {
        part = (I3cPart *)i3cPtr(root, node->m_part);
        for (j = 0; j < node->m_num_part; j++, part++)
        {
            i3cTreeUnrelocate((I3cTree *)i3cPtr(root, part->m_tree), root);
            i3cClearNone(&part->m_2c);
            i3cClearNone(&part->m_24);
            i3cClearNone(&part->m_tree);
        }
        i3cClearNone(&node->m_part);
        i3cClearNone(&node->m_name);
    }
    i3cClearNone(&root->m_node);

    root->m_relocate = 1;
    return 0;
}

/* ROR 0x0022bfd8 */
int i3cGetNumNode(const I3cRoot *root)
{
    return root->m_num_node;
}

/* ROR 0x0022bfe0 */
const char *i3cGetNodeName(const I3cRoot *root, int i)
{
    const I3cNode *node = (const I3cNode *)i3cPtr(root, root->m_node);

    if ((u_int)(i & 0xffff) >= root->m_num_node)
        return NULL;
    return (const char *)i3cPtr(root, node[i & 0xffff].m_name);
}

/* ROR 0x0022c010 */
int i3cGetNumGroup(const I3cRoot *root)
{
    return root->m_num_group;
}

/* ROR 0x0022c018 */
const char *i3cGetGroupName(const I3cRoot *root, int i)
{
    const u_int *name = (const u_int *)i3cPtr(root, root->m_group_name);

    if ((u_int)(i & 0xffff) >= root->m_num_group)
        return NULL;
    return (const char *)i3cPtr(root, name[i & 0xffff]);
}

/* ROR 0x0022c048 */
int i3cGetNodeIndexByName(const I3cRoot *root, const char *name)
{
    const I3cNode *node = (const I3cNode *)i3cPtr(root, root->m_node);
    int i;

    for (i = 0; i < (int)root->m_num_node; i++)
    {
        if (strcmp(name, (const char *)i3cPtr(root, node[i].m_name)) == 0)
            return i;
    }
    return -1;
}

/* ROR 0x0022c0d8 */
int i3cGetGroupIndexByName(const I3cRoot *root, const char *name)
{
    const u_int *group = (const u_int *)i3cPtr(root, root->m_group_name);
    int i;

    for (i = 0; i < (int)root->m_num_group; i++)
    {
        if (strcmp(name, (const char *)i3cPtr(root, group[i])) == 0)
            return i;
    }
    return -1;
}

/* ROR 0x0022c1d8 */
int i3cSetBothFaceEnable(I3cRoot *root, int flag)
{
    u_char old = root->m_both_face;

    root->m_both_face = (char)flag != 0;
    return old == 1;
}
