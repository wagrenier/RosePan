/* ==========================================================================
 *  i3d/i3ritem.cpp
 *
 *  The i3r item tree's core (ROR 0x22c1f8..0x22e840): the child lookup by
 *  name, the item type check, the tree relocators and the per-type data
 *  fixups of tables 0x2db4c8 / 0x2db360. See i3ritem.h / i3rroot.h.
 *
 *  Each type's fixups are the ROM's own small functions, one pair per
 *  type, written here through four helpers: a field holding an offset
 *  from the data start becomes a pointer (and back), and an array of such
 *  fields of a count read from the data. Nine of the pairs (types 0x25,
 *  0x26, 0x29, 0x2a, 0x2d, 0x4a, 0x4d, 0x52, 0x59) sit at 0x22a6c0..
 *  0x22aec0, in the run unassigned@00228c08 before i3rroot.cpp; they are
 *  written here with the tables. Still to write in this file: the
 *  per-type accessors between the fixups (0x22c840.., most call
 *  i3rItemFindChild), unreferenced by the code ported so far.
 *
 *  PORT: a relocated field holds the distance to its target (i3rroot.h):
 *  i3rSetPtr / i3rPtr in place of the ROM's add / subtract of the base.
 * ======================================================================== */
#include "i3d/i3ritem.h"

#include <string.h>

/* ROR 0x0022c1f8 */
int i3rItemFindChild(const I3rItem *item, const char *name)
{
    const I3rItem *child = item->getChildren();
    int lo = 0;
    int hi = (int)item->getNumChild() - 1;

    if (item->getNumChild() == 0)
        return -1;
    do
    {
        int mid = lo + ((hi - lo) >> 1);
        int c = strcmp(name, child[mid].getName());

        if (c < 0)
            hi = mid - 1;
        else
        {
            lo = mid + 1;
            if (c == 0)
                return mid;
        }
    } while (lo <= hi);
    return -1;
}

/* ROR 0x0022c2a0 */
int i3rIsItemType(u_int type)
{
    switch (type)
    {
    case 0x04: case 0x16: case 0x24: case 0x2c: case 0x2e: case 0x2f: case 0x36:
    case 0x3f: case 0x40: case 0x41: case 0x48: case 0x4e: case 0x51:
        return 0;
    }
    return 1;
}

/* The fixups' helpers (inlined in the ROM): field off of the data holds an
 * offset from data; 0 stays NULL. */
static inline void i3rRelocField(u_char *data, u_int off)
{
    u_int *field = (u_int *)(data + off);

    if (*field != 0)
        i3rSetPtr(field, data + *field);
}

static inline void i3rUnrelocField(u_char *data, u_int off)
{
    u_int *field = (u_int *)(data + off);

    if (*field != 0)
        *field = (u_int)((u_char *)i3rPtr(field) - data);
}

/* The n entries of the (relocated) array at field off, each an offset
 * from data. PORT: the ROM indexes a NULL array regardless; skipped. */
static inline void i3rRelocArray(u_char *data, u_int off, u_int n)
{
    u_int *entry = (u_int *)i3rPtr((u_int *)(data + off));
    u_int i;

    if (entry == NULL)
        return;
    for (i = 0; i < n; i++)
    {
        if (entry[i] != 0)
            i3rSetPtr(&entry[i], data + entry[i]);
    }
}

static inline void i3rUnrelocArray(u_char *data, u_int off, u_int n)
{
    u_int *entry = (u_int *)i3rPtr((u_int *)(data + off));
    u_int i;

    if (entry == NULL)
        return;
    for (i = 0; i < n; i++)
    {
        if (entry[i] != 0)
            entry[i] = (u_int)((u_char *)i3rPtr(&entry[i]) - data);
    }
}

typedef void (*I3rDataFixup)(u_char *data);
/* The tables (defined at the end of the file, after the fixups). */
extern const I3rDataFixup i3rUnrelocDataTbl[0x5a];
extern const I3rDataFixup i3rRelocDataTbl[0x5a];

// ──────────────────────────────────────────────────────────────────────
// The fixups at 0x22a6c0..0x22aec0 (see the file header). Type 0x52 is
// the root's data (I3rRootData: the matrix tables, the texture and node
// name tables and their entries).

/* ROR 0x0022a6c0 */
static void i3rUnrelocData4D(u_char *data)
{
    i3rUnrelocArray(data, 0x04, *(u_char *)(data + 0x0e));
    i3rUnrelocField(data, 0x08);
    i3rUnrelocField(data, 0x04);
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022a730 */
static void i3rRelocData4D(u_char *data)
{
    i3rRelocField(data, 0x00);
    i3rRelocField(data, 0x04);
    i3rRelocArray(data, 0x04, *(u_char *)(data + 0x0e));
    i3rRelocField(data, 0x08);
}

/* ROR 0x0022a880 */
static void i3rUnrelocData29(u_char *) {}

/* ROR 0x0022a888 */
static void i3rRelocData29(u_char *) {}

/* ROR 0x0022a8e8 */
static void i3rUnrelocData4A(u_char *) {}

/* ROR 0x0022a8f0 */
static void i3rRelocData4A(u_char *) {}

/* ROR 0x0022a910 */
static void i3rUnrelocData26(u_char *data)
{
    i3rUnrelocField(data, 0x18);
    i3rUnrelocField(data, 0x14);
}

/* ROR 0x0022a938 */
static void i3rRelocData26(u_char *data)
{
    i3rRelocField(data, 0x14);
    i3rRelocField(data, 0x18);
}

/* ROR 0x0022aa18 */
static void i3rUnrelocData59(u_char *data)
{
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022aa30 */
static void i3rRelocData59(u_char *data)
{
    i3rRelocField(data, 0x00);
}

/* ROR 0x0022aaa8 */
static void i3rUnrelocData25(u_char *) {}

/* ROR 0x0022aab0 */
static void i3rRelocData25(u_char *) {}

/* ROR 0x0022aad0 */
I3rItem *i3rGetChild25(const I3rItem *item, int n)
{
    return item->getChildren()[0].getChildren() + n;
}

/* ROR 0x0022ab18 */
static void i3rUnrelocData2D(u_char *data)
{
    i3rUnrelocArray(data, 0x00, *(u_short *)(data + 0x04));
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022ab70 */
static void i3rRelocData2D(u_char *data)
{
    i3rRelocField(data, 0x00);
    i3rRelocArray(data, 0x00, *(u_short *)(data + 0x04));
}

/* ROR 0x0022abf8 */
static void i3rUnrelocData2A(u_char *) {}

/* ROR 0x0022ac00 */
static void i3rRelocData2A(u_char *) {}

/* ROR 0x0022ad20 */
static void i3rUnrelocData52(u_char *data)
{
    i3rUnrelocArray(data, 0x20, *(u_short *)(data + 0x2e));
    i3rUnrelocArray(data, 0x18, *(u_short *)(data + 0x2c));
    i3rUnrelocField(data, 0x20);
    i3rUnrelocField(data, 0x1c);
    i3rUnrelocField(data, 0x18);
    i3rUnrelocField(data, 0x14);
    i3rUnrelocField(data, 0x10);
}

/* ROR 0x0022adf8 */
static void i3rRelocData52(u_char *data)
{
    i3rRelocField(data, 0x10);
    i3rRelocField(data, 0x14);
    i3rRelocField(data, 0x18);
    i3rRelocArray(data, 0x18, *(u_short *)(data + 0x2c));
    i3rRelocField(data, 0x1c);
    i3rRelocField(data, 0x20);
    i3rRelocArray(data, 0x20, *(u_short *)(data + 0x2e));
}

/* ROR 0x0022aec0 (written with dog/dogactor.cpp, whose setupEye calls it) */
int i3rGetNumChild52(const I3rItem *item)
{
    return item->getChildren()[0].getNumChild();
}

/* ROR 0x0022aed8 */
I3rItem *i3rGetChild52(const I3rItem *item, int n)
{
    return item->getChildren()[0].getChildren() + n;
}

/* ROR 0x0022af08 (written with dog/dogactor.cpp) */
const char *i3rGetChildName52(const I3rItem *item, int n)
{
    return item->getChildren()[0].getChildren()[n].getName();
}

// ──────────────────────────────────────────────────────────────────────
// The no-op pair: types 0x00 / 0x01 and the invalid types.

/* ROR 0x0022c318 */
static void i3rUnrelocDataNone(u_char *) {}

/* ROR 0x0022c320 */
static void i3rRelocDataNone(u_char *) {}

// ──────────────────────────────────────────────────────────────────────
// Unrelocates item: its data by type, its children, then its own offsets
// from base. Bit 31 of the info word (or no data) skips the data.
// ROR 0x0022c328

int i3rItemUnrelocate(I3rItem *item, I3rItem *base)
{
    u_int info = item->m_info;
    u_int type = (info >> 24) & 0x7f;
    u_int n = info & 0xffffff;
    u_char *data = (u_char *)item->getData();
    I3rItem *children = item->getChildren();
    const u_char *name = (const u_char *)item->getName();
    u_int i;
    int result;

    if (!i3rIsItemType(type))
        return 4;
    if (data != NULL && (int)info >= 0)
    {
        if (type >= 0x5a)
            return 3;
        i3rUnrelocDataTbl[type](data);
    }
    for (i = 0; i < n; i++)
    {
        result = i3rItemUnrelocate(&children[i], base);
        if (result != I3D_OK)
            return result;
    }
    item->m_children = children != NULL ? (u_int)((u_char *)children - (u_char *)base) : 0;
    item->m_data = data != NULL ? (u_int)(data - (u_char *)base) : 0;
    item->m_name = name != NULL ? (u_int)(name - (u_char *)base) : 0;
    return I3D_OK;
}

// ──────────────────────────────────────────────────────────────────────
// Relocates item: its offsets from base into pointers, its data by type,
// then its children. Bit 31 of the info word (or no data) skips the data.
// ROR 0x0022c478

int i3rItemRelocate(I3rItem *item, I3rItem *base)
{
    u_int info = item->m_info;
    u_int type = (info >> 24) & 0x7f;
    u_int n = info & 0xffffff;
    u_int data = item->m_data;
    u_int children = item->m_children;
    u_int name = item->m_name;
    I3rItem *child;
    u_int i;
    int result;

    if (!i3rIsItemType(type))
        return 4;
    i3rSetPtr(&item->m_data, data != 0 ? (u_char *)base + data : NULL);
    i3rSetPtr(&item->m_children, children != 0 ? (u_char *)base + children : NULL);
    i3rSetPtr(&item->m_name, name != 0 ? (u_char *)base + name : NULL);
    if (data != 0 && (int)info >= 0)
    {
        if (type >= 0x5a)
            return 3;
        i3rRelocDataTbl[type]((u_char *)item->getData());
    }
    child = item->getChildren();
    for (i = 0; i < n; i++)
    {
        result = i3rItemRelocate(&child[i], base);
        if (result != I3D_OK)
            return result;
    }
    return I3D_OK;
}

// ──────────────────────────────────────────────────────────────────────
// The fixups in this file's extent (0x22c5a0..), in address order; the
// accessors between them are still to write.

/* ROR 0x0022c5a0 */
static void i3rUnrelocData02(u_char *) {}

/* ROR 0x0022c5a8 */
static void i3rRelocData02(u_char *) {}

/* ROR 0x0022c5b0 */
static void i3rUnrelocData03(u_char *) {}

/* ROR 0x0022c5b8 */
static void i3rRelocData03(u_char *) {}

/* ROR 0x0022c5c0 */
static void i3rUnrelocData35(u_char *) {}

/* ROR 0x0022c5c8 */
static void i3rRelocData35(u_char *) {}

/* ROR 0x0022c5d0 */
static void i3rUnrelocData34(u_char *data)
{
    i3rUnrelocField(data, 0x28);
    i3rUnrelocField(data, 0x24);
    i3rUnrelocField(data, 0x20);
}

/* ROR 0x0022c608 */
static void i3rRelocData34(u_char *data)
{
    i3rRelocField(data, 0x20);
    i3rRelocField(data, 0x24);
    i3rRelocField(data, 0x28);
}

/* ROR 0x0022c658 */
static void i3rUnrelocData33(u_char *data)
{
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022c670 */
static void i3rRelocData33(u_char *data)
{
    i3rRelocField(data, 0x00);
}

/* ROR 0x0022c6e8 */
static void i3rUnrelocData37(u_char *) {}

/* ROR 0x0022c6f0 */
static void i3rRelocData37(u_char *) {}

/* ROR 0x0022c6f8 */
static void i3rUnrelocData50(u_char *data)
{
    i3rUnrelocField(data, 0x18);
    i3rUnrelocField(data, 0x10);
    i3rUnrelocField(data, 0x08);
}

/* ROR 0x0022c730 */
static void i3rRelocData50(u_char *data)
{
    i3rRelocField(data, 0x08);
    i3rRelocField(data, 0x10);
    i3rRelocField(data, 0x18);
}

/* ROR 0x0022c7e0 */
static void i3rUnrelocData47(u_char *data)
{
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022c7f8 */
static void i3rRelocData47(u_char *data)
{
    i3rRelocField(data, 0x00);
}

/* ROR 0x0022ca00 */
static void i3rUnrelocData30(u_char *) {}

/* ROR 0x0022ca08 */
static void i3rRelocData30(u_char *) {}

/* ROR 0x0022ca70 */
static void i3rUnrelocData4F(u_char *) {}

/* ROR 0x0022ca78 */
static void i3rRelocData4F(u_char *) {}

/* ROR 0x0022cae0 */
static void i3rUnrelocData31(u_char *data)
{
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022caf8 */
static void i3rRelocData31(u_char *data)
{
    i3rRelocField(data, 0x00);
}

/* ROR 0x0022cb10 */
static void i3rUnrelocData56(u_char *data)
{
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022cb28 */
static void i3rRelocData56(u_char *data)
{
    i3rRelocField(data, 0x00);
}

/* ROR 0x0022cf20 */
static void i3rUnrelocData55(u_char *data)
{
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022cf38 */
static void i3rRelocData55(u_char *data)
{
    i3rRelocField(data, 0x00);
}

/* ROR 0x0022cfd0 */
static void i3rUnrelocData32(u_char *) {}

/* ROR 0x0022cfd8 */
static void i3rRelocData32(u_char *) {}

/* ROR 0x0022cff8 */
static void i3rUnrelocData58(u_char *data)
{
    i3rUnrelocField(data, 0x10);
    i3rUnrelocField(data, 0x0c);
    i3rUnrelocField(data, 0x08);
}

/* ROR 0x0022d030 */
static void i3rRelocData58(u_char *data)
{
    i3rRelocField(data, 0x08);
    i3rRelocField(data, 0x0c);
    i3rRelocField(data, 0x10);
}

/* ROR 0x0022d080 */
static void i3rUnrelocData49(u_char *data)
{
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022d098 */
static void i3rRelocData49(u_char *data)
{
    i3rRelocField(data, 0x00);
}

/* ROR 0x0022d0b0 */
static void i3rUnrelocData28(u_char *) {}

/* ROR 0x0022d0b8 */
static void i3rRelocData28(u_char *) {}

/* ROR 0x0022d0f8 */
static void i3rUnrelocData57(u_char *) {}

/* ROR 0x0022d100 */
static void i3rRelocData57(u_char *) {}

/* ROR 0x0022d120 */
static void i3rUnrelocData4C(u_char *data)
{
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022d138 */
static void i3rRelocData4C(u_char *data)
{
    i3rRelocField(data, 0x00);
}

/* ROR 0x0022d1b0 */
static void i3rUnrelocData4B(u_char *) {}

/* ROR 0x0022d1b8 */
static void i3rRelocData4B(u_char *) {}

/* ROR 0x0022d220 */
static void i3rUnrelocData27(u_char *) {}

/* ROR 0x0022d228 */
static void i3rRelocData27(u_char *) {}

/* ROR 0x0022d230 */
static void i3rUnrelocData2B(u_char *) {}

/* ROR 0x0022d238 */
static void i3rRelocData2B(u_char *) {}

/* ROR 0x0022d240 */
static void i3rUnrelocData46(u_char *data)
{
    i3rUnrelocArray(data, 0x08, *(u_short *)(data + 0x10));
    i3rUnrelocField(data, 0x08);
    i3rUnrelocField(data, 0x04);
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022d2b8 */
static void i3rRelocData46(u_char *data)
{
    i3rRelocField(data, 0x00);
    i3rRelocField(data, 0x04);
    i3rRelocField(data, 0x08);
    i3rRelocArray(data, 0x08, *(u_short *)(data + 0x10));
}

/* ROR 0x0022d388 */
static void i3rUnrelocData45(u_char *) {}

/* ROR 0x0022d390 */
static void i3rRelocData45(u_char *) {}

/* ROR 0x0022d410 */
static void i3rUnrelocData53(u_char *data)
{
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022d428 */
static void i3rRelocData53(u_char *data)
{
    i3rRelocField(data, 0x00);
}

/* ROR 0x0022d440 */
static void i3rUnrelocData54(u_char *data)
{
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022d458 */
static void i3rRelocData54(u_char *data)
{
    i3rRelocField(data, 0x00);
}

/* ROR 0x0022d470 */
static void i3rUnrelocData23(u_char *) {}

/* ROR 0x0022d478 */
static void i3rRelocData23(u_char *) {}

/* ROR 0x0022d480 */
static void i3rUnrelocData0A(u_char *) {}

/* ROR 0x0022d488 */
static void i3rRelocData0A(u_char *) {}

/* ROR 0x0022d490 */
static void i3rUnrelocData09(u_char *) {}

/* ROR 0x0022d498 */
static void i3rRelocData09(u_char *) {}

/* ROR 0x0022d520 */
static void i3rUnrelocData0E(u_char *data)
{
    i3rUnrelocField(data, 0x04);
}

/* ROR 0x0022d538 */
static void i3rRelocData0E(u_char *data)
{
    i3rRelocField(data, 0x04);
}

/* ROR 0x0022d550 */
static void i3rUnrelocData0D(u_char *data)
{
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022d568 */
static void i3rRelocData0D(u_char *data)
{
    i3rRelocField(data, 0x00);
}

/* ROR 0x0022d600 */
static void i3rUnrelocData0C(u_char *) {}

/* ROR 0x0022d608 */
static void i3rRelocData0C(u_char *) {}

/* ROR 0x0022d610 */
static void i3rUnrelocData0B(u_char *) {}

/* ROR 0x0022d618 */
static void i3rRelocData0B(u_char *) {}

/* ROR 0x0022d6a0 */
static void i3rUnrelocData15(u_char *) {}

/* ROR 0x0022d6a8 */
static void i3rRelocData15(u_char *) {}

/* ROR 0x0022d6b0 */
static void i3rUnrelocData08(u_char *data)
{
    i3rUnrelocArray(data, 0x04, *(u_int *)(data + 0x08));
    i3rUnrelocField(data, 0x04);
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022d710 */
static void i3rRelocData08(u_char *data)
{
    i3rRelocField(data, 0x00);
    i3rRelocField(data, 0x04);
    i3rRelocArray(data, 0x04, *(u_int *)(data + 0x08));
}

/* ROR 0x0022d808 */
static void i3rUnrelocData10(u_char *) {}

/* ROR 0x0022d810 */
static void i3rRelocData10(u_char *) {}

/* ROR 0x0022d818 */
static void i3rUnrelocData1B(u_char *) {}

/* ROR 0x0022d820 */
static void i3rRelocData1B(u_char *) {}

/* ROR 0x0022d888 */
static void i3rUnrelocData44(u_char *) {}

/* ROR 0x0022d890 */
static void i3rRelocData44(u_char *) {}

/* ROR 0x0022d898 */
static void i3rUnrelocData39(u_char *) {}

/* ROR 0x0022d8a0 */
static void i3rRelocData39(u_char *) {}

/* ROR 0x0022d8a8 */
static void i3rUnrelocData18(u_char *) {}

/* ROR 0x0022d8b0 */
static void i3rRelocData18(u_char *) {}

/* ROR 0x0022d8f0 */
static void i3rUnrelocData19(u_char *) {}

/* ROR 0x0022d8f8 */
static void i3rRelocData19(u_char *) {}

/* ROR 0x0022d958 */
static void i3rUnrelocData05(u_char *) {}

/* ROR 0x0022d960 */
static void i3rRelocData05(u_char *) {}

/* ROR 0x0022d980 */
static void i3rUnrelocData0F(u_char *) {}

/* ROR 0x0022d988 */
static void i3rRelocData0F(u_char *) {}

/* ROR 0x0022da10 */
static void i3rUnrelocData20(u_char *) {}

/* ROR 0x0022da18 */
static void i3rRelocData20(u_char *) {}

/* ROR 0x0022da80 */
static void i3rUnrelocData12(u_char *) {}

/* ROR 0x0022da88 */
static void i3rRelocData12(u_char *) {}

/* ROR 0x0022daa8 */
static void i3rUnrelocData14(u_char *) {}

/* ROR 0x0022dab0 */
static void i3rRelocData14(u_char *) {}

/* ROR 0x0022dad0 */
static void i3rUnrelocData11(u_char *) {}

/* ROR 0x0022dad8 */
static void i3rRelocData11(u_char *) {}

/* ROR 0x0022dae0 */
static void i3rUnrelocData42(u_char *) {}

/* ROR 0x0022dae8 */
static void i3rRelocData42(u_char *) {}

/* ROR 0x0022daf0 */
static void i3rUnrelocData1A(u_char *) {}

/* ROR 0x0022daf8 */
static void i3rRelocData1A(u_char *) {}

/* ROR 0x0022db18 */
static void i3rUnrelocData06(u_char *) {}

/* ROR 0x0022db20 */
static void i3rRelocData06(u_char *) {}

/* ROR 0x0022db28 */
static void i3rUnrelocData43(u_char *) {}

/* ROR 0x0022db30 */
static void i3rRelocData43(u_char *) {}

/* ROR 0x0022db38 */
static void i3rUnrelocData3E(u_char *) {}

/* ROR 0x0022db40 */
static void i3rRelocData3E(u_char *) {}

/* ROR 0x0022dba0 */
static void i3rUnrelocData13(u_char *) {}

/* ROR 0x0022dba8 */
static void i3rRelocData13(u_char *) {}

/* ROR 0x0022dbc8 */
static void i3rUnrelocData07(u_char *) {}

/* ROR 0x0022dbd0 */
static void i3rRelocData07(u_char *) {}

/* ROR 0x0022dc10 */
static void i3rUnrelocData22(u_char *) {}

/* ROR 0x0022dc18 */
static void i3rRelocData22(u_char *) {}

/* ROR 0x0022dc58 */
static void i3rUnrelocData17(u_char *) {}

/* ROR 0x0022dc60 */
static void i3rRelocData17(u_char *) {}

/* ROR 0x0022dec0 */
static void i3rUnrelocData38(u_char *data)
{
    i3rUnrelocArray(data, 0x04, *(u_int *)(data + 0x08));
    i3rUnrelocArray(data, 0x00, *(u_int *)(data + 0x08));
    i3rUnrelocField(data, 0x04);
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022df38 */
static void i3rRelocData38(u_char *data)
{
    i3rRelocField(data, 0x00);
    i3rRelocField(data, 0x04);
    i3rRelocArray(data, 0x04, *(u_int *)(data + 0x08));
    i3rRelocArray(data, 0x00, *(u_int *)(data + 0x08));
}

/* ROR 0x0022dfc8 */
static void i3rUnrelocData1F(u_char *) {}

/* ROR 0x0022dfd0 */
static void i3rRelocData1F(u_char *) {}

/* ROR 0x0022e0f0 */
static void i3rUnrelocData3D(u_char *) {}

/* ROR 0x0022e0f8 */
static void i3rRelocData3D(u_char *) {}

/* ROR 0x0022e100 */
static void i3rUnrelocData3B(u_char *) {}

/* ROR 0x0022e108 */
static void i3rRelocData3B(u_char *) {}

/* ROR 0x0022e110 */
static void i3rUnrelocData1C(u_char *) {}

/* ROR 0x0022e118 */
static void i3rRelocData1C(u_char *) {}

/* ROR 0x0022e180 */
static void i3rUnrelocData1D(u_char *data)
{
    i3rUnrelocArray(data, 0x04, *(u_int *)(data + 0x08));
    i3rUnrelocField(data, 0x04);
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022e1e0 */
static void i3rRelocData1D(u_char *data)
{
    i3rRelocField(data, 0x00);
    i3rRelocField(data, 0x04);
    i3rRelocArray(data, 0x04, *(u_int *)(data + 0x08));
}

/* ROR 0x0022e2a0 */
static void i3rUnrelocData1E(u_char *data)
{
    i3rUnrelocField(data, 0x00);
}

/* ROR 0x0022e2b8 */
static void i3rRelocData1E(u_char *data)
{
    i3rRelocField(data, 0x00);
}

/* ROR 0x0022e3e0 */
static void i3rUnrelocData3C(u_char *) {}

/* ROR 0x0022e3e8 */
static void i3rRelocData3C(u_char *) {}

/* ROR 0x0022e470 */
static void i3rUnrelocData3A(u_char *) {}

/* ROR 0x0022e478 */
static void i3rRelocData3A(u_char *) {}

/* ROR 0x0022e500 */
static void i3rUnrelocData21(u_char *) {}

/* ROR 0x0022e508 */
static void i3rRelocData21(u_char *) {}

/* data 0x2db360: i3rItemUnrelocate's fixups by item type. */
const I3rDataFixup i3rUnrelocDataTbl[0x5a] = {
    i3rUnrelocDataNone, i3rUnrelocDataNone, i3rUnrelocData02, i3rUnrelocData03,  /* 0x00 */
    i3rUnrelocDataNone, i3rUnrelocData05, i3rUnrelocData06, i3rUnrelocData07,  /* 0x04 */
    i3rUnrelocData08, i3rUnrelocData09, i3rUnrelocData0A, i3rUnrelocData0B,  /* 0x08 */
    i3rUnrelocData0C, i3rUnrelocData0D, i3rUnrelocData0E, i3rUnrelocData0F,  /* 0x0c */
    i3rUnrelocData10, i3rUnrelocData11, i3rUnrelocData12, i3rUnrelocData13,  /* 0x10 */
    i3rUnrelocData14, i3rUnrelocData15, i3rUnrelocDataNone, i3rUnrelocData17,  /* 0x14 */
    i3rUnrelocData18, i3rUnrelocData19, i3rUnrelocData1A, i3rUnrelocData1B,  /* 0x18 */
    i3rUnrelocData1C, i3rUnrelocData1D, i3rUnrelocData1E, i3rUnrelocData1F,  /* 0x1c */
    i3rUnrelocData20, i3rUnrelocData21, i3rUnrelocData22, i3rUnrelocData23,  /* 0x20 */
    i3rUnrelocDataNone, i3rUnrelocData25, i3rUnrelocData26, i3rUnrelocData27,  /* 0x24 */
    i3rUnrelocData28, i3rUnrelocData29, i3rUnrelocData2A, i3rUnrelocData2B,  /* 0x28 */
    i3rUnrelocDataNone, i3rUnrelocData2D, i3rUnrelocDataNone, i3rUnrelocDataNone,  /* 0x2c */
    i3rUnrelocData30, i3rUnrelocData31, i3rUnrelocData32, i3rUnrelocData33,  /* 0x30 */
    i3rUnrelocData34, i3rUnrelocData35, i3rUnrelocDataNone, i3rUnrelocData37,  /* 0x34 */
    i3rUnrelocData38, i3rUnrelocData39, i3rUnrelocData3A, i3rUnrelocData3B,  /* 0x38 */
    i3rUnrelocData3C, i3rUnrelocData3D, i3rUnrelocData3E, i3rUnrelocDataNone,  /* 0x3c */
    i3rUnrelocDataNone, i3rUnrelocDataNone, i3rUnrelocData42, i3rUnrelocData43,  /* 0x40 */
    i3rUnrelocData44, i3rUnrelocData45, i3rUnrelocData46, i3rUnrelocData47,  /* 0x44 */
    i3rUnrelocDataNone, i3rUnrelocData49, i3rUnrelocData4A, i3rUnrelocData4B,  /* 0x48 */
    i3rUnrelocData4C, i3rUnrelocData4D, i3rUnrelocDataNone, i3rUnrelocData4F,  /* 0x4c */
    i3rUnrelocData50, i3rUnrelocDataNone, i3rUnrelocData52, i3rUnrelocData53,  /* 0x50 */
    i3rUnrelocData54, i3rUnrelocData55, i3rUnrelocData56, i3rUnrelocData57,  /* 0x54 */
    i3rUnrelocData58, i3rUnrelocData59,  /* 0x58 */
};

/* data 0x2db4c8: i3rItemRelocate's fixups by item type. */
const I3rDataFixup i3rRelocDataTbl[0x5a] = {
    i3rRelocDataNone, i3rRelocDataNone, i3rRelocData02, i3rRelocData03,  /* 0x00 */
    i3rRelocDataNone, i3rRelocData05, i3rRelocData06, i3rRelocData07,  /* 0x04 */
    i3rRelocData08, i3rRelocData09, i3rRelocData0A, i3rRelocData0B,  /* 0x08 */
    i3rRelocData0C, i3rRelocData0D, i3rRelocData0E, i3rRelocData0F,  /* 0x0c */
    i3rRelocData10, i3rRelocData11, i3rRelocData12, i3rRelocData13,  /* 0x10 */
    i3rRelocData14, i3rRelocData15, i3rRelocDataNone, i3rRelocData17,  /* 0x14 */
    i3rRelocData18, i3rRelocData19, i3rRelocData1A, i3rRelocData1B,  /* 0x18 */
    i3rRelocData1C, i3rRelocData1D, i3rRelocData1E, i3rRelocData1F,  /* 0x1c */
    i3rRelocData20, i3rRelocData21, i3rRelocData22, i3rRelocData23,  /* 0x20 */
    i3rRelocDataNone, i3rRelocData25, i3rRelocData26, i3rRelocData27,  /* 0x24 */
    i3rRelocData28, i3rRelocData29, i3rRelocData2A, i3rRelocData2B,  /* 0x28 */
    i3rRelocDataNone, i3rRelocData2D, i3rRelocDataNone, i3rRelocDataNone,  /* 0x2c */
    i3rRelocData30, i3rRelocData31, i3rRelocData32, i3rRelocData33,  /* 0x30 */
    i3rRelocData34, i3rRelocData35, i3rRelocDataNone, i3rRelocData37,  /* 0x34 */
    i3rRelocData38, i3rRelocData39, i3rRelocData3A, i3rRelocData3B,  /* 0x38 */
    i3rRelocData3C, i3rRelocData3D, i3rRelocData3E, i3rRelocDataNone,  /* 0x3c */
    i3rRelocDataNone, i3rRelocDataNone, i3rRelocData42, i3rRelocData43,  /* 0x40 */
    i3rRelocData44, i3rRelocData45, i3rRelocData46, i3rRelocData47,  /* 0x44 */
    i3rRelocDataNone, i3rRelocData49, i3rRelocData4A, i3rRelocData4B,  /* 0x48 */
    i3rRelocData4C, i3rRelocData4D, i3rRelocDataNone, i3rRelocData4F,  /* 0x4c */
    i3rRelocData50, i3rRelocDataNone, i3rRelocData52, i3rRelocData53,  /* 0x50 */
    i3rRelocData54, i3rRelocData55, i3rRelocData56, i3rRelocData57,  /* 0x54 */
    i3rRelocData58, i3rRelocData59,  /* 0x58 */
};
