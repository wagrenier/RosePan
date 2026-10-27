/* ==========================================================================
 *  i3d/i3rtype.cpp
 *
 *  The i3r per-type accessors (ROR 0x22a6c0..0x22b1b0): per item type, the
 *  ROM's one-line reads of the item's child slots. See i3rtype.h for the
 *  naming. The nine fixup pairs that sit between them (types 0x4d 0x29
 *  0x4a 0x26 0x59 0x25 0x2d 0x2a 0x52) are written in i3ritem.cpp with the
 *  fixup tables that hold them.
 *
 *  PORT: the ROM reads a child list's raw pointer word; here getChildren()
 *  resolves the host's relative pointer (i3rroot.h).
 * ======================================================================== */
#include "i3d/i3rtype.h"

#include "i3d/i3ritem.h"

// ──────────────────────────────────────────────────────────────────────
// Type 0x4d: after its fixup pair 0x22a6c0 / 0x22a730 (i3ritem.cpp).

/* ROR 0x0022a7d0 */
I3rItem *i3rGetChildBegin4D(const I3rItem *item)
{
    return item->getChildren()[0].getChildren();
}

/* ROR 0x0022a7e0 */
I3rItem *i3rGetChildEnd4D(const I3rItem *item)
{
    return item->getChildren()[0].getChildren() + item->getChildren()[0].getNumChild();
}

/* ROR 0x0022a800 */
int i3rGetNumChild4D_1(const I3rItem *item)
{
    return (int)item->getChildren()[1].getNumChild();
}

/* ROR 0x0022a818 */
I3rItem *i3rGetChild4D_1(const I3rItem *item, int n)
{
    return item->getChildren()[1].getChildren() + n;
}

/* ROR 0x0022a830 */
I3rItem *i3rGetChildBegin4D_1(const I3rItem *item)
{
    return item->getChildren()[1].getChildren();
}

/* ROR 0x0022a840 */
I3rItem *i3rGetChildEnd4D_1(const I3rItem *item)
{
    return item->getChildren()[1].getChildren() + item->getChildren()[1].getNumChild();
}

/* ROR 0x0022a860 */
I3rItem *i3rGetSlot4D_2(const I3rItem *item)
{
    return &item->getChildren()[2];
}

/* ROR 0x0022a870 */
int i3rHasData4D_2(const I3rItem *item)
{
    return item->getChildren()[2].m_data != 0;
}

// ──────────────────────────────────────────────────────────────────────
// Type 0x29: after its fixup pair 0x22a880 / 0x22a888 (i3ritem.cpp).

/* ROR 0x0022a890 */
I3rItem *i3rGetSlot29(const I3rItem *item)
{
    return &item->getChildren()[0];
}

/* ROR 0x0022a898 */
int i3rHasData29(const I3rItem *item)
{
    return item->getChildren()[0].m_data != 0;
}

/* ROR 0x0022a8a8 */
I3rItem *i3rGetSlot29_1(const I3rItem *item)
{
    return &item->getChildren()[1];
}

/* ROR 0x0022a8b8 */
int i3rHasData29_1(const I3rItem *item)
{
    return item->getChildren()[1].m_data != 0;
}

/* ROR 0x0022a8c8 */
I3rItem *i3rGetSlot29_2(const I3rItem *item)
{
    return &item->getChildren()[2];
}

/* ROR 0x0022a8d8 */
int i3rHasData29_2(const I3rItem *item)
{
    return item->getChildren()[2].m_data != 0;
}

// ──────────────────────────────────────────────────────────────────────
// Type 0x4a: after its fixup pair 0x22a8e8 / 0x22a8f0 (i3ritem.cpp).

/* ROR 0x0022a8f8 */
I3rItem *i3rGetSlot4A(const I3rItem *item)
{
    return &item->getChildren()[0];
}

/* ROR 0x0022a900 */
int i3rHasData4A(const I3rItem *item)
{
    return item->getChildren()[0].m_data != 0;
}

// ──────────────────────────────────────────────────────────────────────
// Type 0x26: after its fixup pair 0x22a910 / 0x22a938 (i3ritem.cpp).

/* ROR 0x0022a960 */
I3rItem *i3rGetSlot26(const I3rItem *item)
{
    return &item->getChildren()[0];
}

/* ROR 0x0022a968 */
int i3rHasData26(const I3rItem *item)
{
    return item->getChildren()[0].m_data != 0;
}

/* ROR 0x0022a978 */
I3rItem *i3rGetSlot26_1(const I3rItem *item)
{
    return &item->getChildren()[1];
}

/* ROR 0x0022a988 */
int i3rHasData26_1(const I3rItem *item)
{
    return item->getChildren()[1].m_data != 0;
}

/* ROR 0x0022a998 */
I3rItem *i3rGetSlot26_2(const I3rItem *item)
{
    return &item->getChildren()[2];
}

/* ROR 0x0022a9a8 */
int i3rHasData26_2(const I3rItem *item)
{
    return item->getChildren()[2].m_data != 0;
}

/* ROR 0x0022a9b8 */
int i3rGetNumChild26_3(const I3rItem *item)
{
    return (int)item->getChildren()[3].getNumChild();
}

/* ROR 0x0022a9d0 */
I3rItem *i3rGetChild26_3(const I3rItem *item, int n)
{
    return item->getChildren()[3].getChildren() + n;
}

/* ROR 0x0022a9e8 */
I3rItem *i3rGetChildBegin26_3(const I3rItem *item)
{
    return item->getChildren()[3].getChildren();
}

/* ROR 0x0022a9f8 */
I3rItem *i3rGetChildEnd26_3(const I3rItem *item)
{
    return item->getChildren()[3].getChildren() + item->getChildren()[3].getNumChild();
}

// ──────────────────────────────────────────────────────────────────────
// Type 0x59: after its fixup pair 0x22aa18 / 0x22aa30 (i3ritem.cpp).

/* ROR 0x0022aa48 */
int i3rGetNumChild59(const I3rItem *item)
{
    return (int)item->getChildren()[0].getNumChild();
}

/* ROR 0x0022aa60 */
I3rItem *i3rGetChild59(const I3rItem *item, int n)
{
    return item->getChildren()[0].getChildren() + n;
}

/* ROR 0x0022aa78 */
I3rItem *i3rGetChildBegin59(const I3rItem *item)
{
    return item->getChildren()[0].getChildren();
}

/* ROR 0x0022aa88 */
I3rItem *i3rGetChildEnd59(const I3rItem *item)
{
    return item->getChildren()[0].getChildren() + item->getChildren()[0].getNumChild();
}

// ──────────────────────────────────────────────────────────────────────
// Type 0x25: after its fixup pair 0x22aaa8 / 0x22aab0 (i3ritem.cpp).

/* ROR 0x0022aab8 */
int i3rGetNumChild25(const I3rItem *item)
{
    return (int)item->getChildren()[0].getNumChild();
}

/* ROR 0x0022aad0 */
I3rItem *i3rGetChild25(const I3rItem *item, int n)
{
    return item->getChildren()[0].getChildren() + n;
}

/* ROR 0x0022aae8 */
I3rItem *i3rGetChildBegin25(const I3rItem *item)
{
    return item->getChildren()[0].getChildren();
}

/* ROR 0x0022aaf8 */
I3rItem *i3rGetChildEnd25(const I3rItem *item)
{
    return item->getChildren()[0].getChildren() + item->getChildren()[0].getNumChild();
}

// ──────────────────────────────────────────────────────────────────────
// Type 0x2d: after its fixup pair 0x22ab18 / 0x22ab70 (i3ritem.cpp).

/* ROR 0x0022abc0 */
I3rItem *i3rGetSlot2D(const I3rItem *item)
{
    return &item->getChildren()[0];
}

/* ROR 0x0022abc8 */
int i3rHasData2D(const I3rItem *item)
{
    return item->getChildren()[0].m_data != 0;
}

/* ROR 0x0022abd8 */
I3rItem *i3rGetSlot2D_1(const I3rItem *item)
{
    return &item->getChildren()[1];
}

/* ROR 0x0022abe8 */
int i3rHasData2D_1(const I3rItem *item)
{
    return item->getChildren()[1].m_data != 0;
}

// ──────────────────────────────────────────────────────────────────────
// Type 0x2a: after its fixup pair 0x22abf8 / 0x22ac00 (i3ritem.cpp).

/* ROR 0x0022ac08 */
I3rItem *i3rGetSlot2A(const I3rItem *item)
{
    return &item->getChildren()[0];
}

/* ROR 0x0022ac10 */
int i3rHasData2A(const I3rItem *item)
{
    return item->getChildren()[0].m_data != 0;
}

/* ROR 0x0022ac20 */
int i3rGetNumChild2A_1(const I3rItem *item)
{
    return (int)item->getChildren()[1].getNumChild();
}

/* ROR 0x0022ac38 */
I3rItem *i3rGetChild2A_1(const I3rItem *item, int n)
{
    return item->getChildren()[1].getChildren() + n;
}

/* ROR 0x0022ac50 */
int i3rFindChild2A_1(const I3rItem *item, const char *name)
{
    return i3rItemFindChild(&item->getChildren()[1], name);
}

/* ROR 0x0022ac70 */
const char *i3rGetChildName2A_1(const I3rItem *item, int n)
{
    return item->getChildren()[1].getChildren()[n].getName();
}

/* ROR 0x0022ac88 */
/* -1 (absent) gives the item before the list, as in the ROM. */
I3rItem *i3rGetChildByName2A_1(const I3rItem *item, const char *name)
{
    const I3rItem *list = &item->getChildren()[1];

    return list->getChildren() + i3rItemFindChild(list, name);
}

/* ROR 0x0022acc8 */
int i3rHasChild2A_1(const I3rItem *item, const char *name)
{
    return i3rItemFindChild(&item->getChildren()[1], name) != -1;
}

/* ROR 0x0022acf0 */
I3rItem *i3rGetChildBegin2A_1(const I3rItem *item)
{
    return item->getChildren()[1].getChildren();
}

/* ROR 0x0022ad00 */
I3rItem *i3rGetChildEnd2A_1(const I3rItem *item)
{
    return item->getChildren()[1].getChildren() + item->getChildren()[1].getNumChild();
}

// ──────────────────────────────────────────────────────────────────────
// Type 0x52: after its fixup pair 0x22ad20 / 0x22adf8 (i3ritem.cpp).

/* ROR 0x0022aec0 */
int i3rGetNumChild52(const I3rItem *item)
{
    return (int)item->getChildren()[0].getNumChild();
}

/* ROR 0x0022aed8 */
I3rItem *i3rGetChild52(const I3rItem *item, int n)
{
    return item->getChildren()[0].getChildren() + n;
}

/* ROR 0x0022aef0 */
int i3rFindChild52(const I3rItem *item, const char *name)
{
    return i3rItemFindChild(&item->getChildren()[0], name);
}

/* ROR 0x0022af08 */
const char *i3rGetChildName52(const I3rItem *item, int n)
{
    return item->getChildren()[0].getChildren()[n].getName();
}

/* ROR 0x0022af20 */
/* -1 (absent) gives the item before the list, as in the ROM. */
I3rItem *i3rGetChildByName52(const I3rItem *item, const char *name)
{
    const I3rItem *list = &item->getChildren()[0];

    return list->getChildren() + i3rItemFindChild(list, name);
}

/* ROR 0x0022af58 */
int i3rHasChild52(const I3rItem *item, const char *name)
{
    return i3rItemFindChild(&item->getChildren()[0], name) != -1;
}

/* ROR 0x0022af80 */
I3rItem *i3rGetChildBegin52(const I3rItem *item)
{
    return item->getChildren()[0].getChildren();
}

/* ROR 0x0022af90 */
I3rItem *i3rGetChildEnd52(const I3rItem *item)
{
    return item->getChildren()[0].getChildren() + item->getChildren()[0].getNumChild();
}

/* ROR 0x0022afb0 */
I3rItem *i3rGetSlot52_1(const I3rItem *item)
{
    return &item->getChildren()[1];
}

/* ROR 0x0022afc0 */
int i3rHasData52_1(const I3rItem *item)
{
    return item->getChildren()[1].m_data != 0;
}

/* ROR 0x0022afd0 */
int i3rGetNumChild52_2(const I3rItem *item)
{
    return (int)item->getChildren()[2].getNumChild();
}

/* ROR 0x0022afe8 */
I3rItem *i3rGetChild52_2(const I3rItem *item, int n)
{
    return item->getChildren()[2].getChildren() + n;
}

/* ROR 0x0022b000 */
int i3rFindChild52_2(const I3rItem *item, const char *name)
{
    return i3rItemFindChild(&item->getChildren()[2], name);
}

/* ROR 0x0022b020 */
const char *i3rGetChildName52_2(const I3rItem *item, int n)
{
    return item->getChildren()[2].getChildren()[n].getName();
}

/* ROR 0x0022b038 */
/* -1 (absent) gives the item before the list, as in the ROM. */
I3rItem *i3rGetChildByName52_2(const I3rItem *item, const char *name)
{
    const I3rItem *list = &item->getChildren()[2];

    return list->getChildren() + i3rItemFindChild(list, name);
}

/* ROR 0x0022b078 */
int i3rHasChild52_2(const I3rItem *item, const char *name)
{
    return i3rItemFindChild(&item->getChildren()[2], name) != -1;
}

/* ROR 0x0022b0a0 */
I3rItem *i3rGetChildBegin52_2(const I3rItem *item)
{
    return item->getChildren()[2].getChildren();
}

/* ROR 0x0022b0b0 */
I3rItem *i3rGetChildEnd52_2(const I3rItem *item)
{
    return item->getChildren()[2].getChildren() + item->getChildren()[2].getNumChild();
}

/* ROR 0x0022b0d0 */
int i3rGetNumChild52_3(const I3rItem *item)
{
    return (int)item->getChildren()[3].getNumChild();
}

/* ROR 0x0022b0e8 */
I3rItem *i3rGetChild52_3(const I3rItem *item, int n)
{
    return item->getChildren()[3].getChildren() + n;
}

/* ROR 0x0022b100 */
I3rItem *i3rGetChildBegin52_3(const I3rItem *item)
{
    return item->getChildren()[3].getChildren();
}

/* ROR 0x0022b110 */
I3rItem *i3rGetChildEnd52_3(const I3rItem *item)
{
    return item->getChildren()[3].getChildren() + item->getChildren()[3].getNumChild();
}

/* ROR 0x0022b130 */
I3rItem *i3rGetSlot52_4(const I3rItem *item)
{
    return &item->getChildren()[4];
}

/* ROR 0x0022b140 */
int i3rHasData52_4(const I3rItem *item)
{
    return item->getChildren()[4].m_data != 0;
}

/* ROR 0x0022b150 */
I3rItem *i3rGetSlot52_5(const I3rItem *item)
{
    return &item->getChildren()[5];
}

/* ROR 0x0022b160 */
int i3rHasData52_5(const I3rItem *item)
{
    return item->getChildren()[5].m_data != 0;
}

/* ROR 0x0022b170 */
I3rItem *i3rGetSlot52_6(const I3rItem *item)
{
    return &item->getChildren()[6];
}

/* ROR 0x0022b180 */
int i3rHasData52_6(const I3rItem *item)
{
    return item->getChildren()[6].m_data != 0;
}

/* ROR 0x0022b190 */
I3rItem *i3rGetSlot52_7(const I3rItem *item)
{
    return &item->getChildren()[7];
}

/* ROR 0x0022b1a0 */
int i3rHasData52_7(const I3rItem *item)
{
    return item->getChildren()[7].m_data != 0;
}
