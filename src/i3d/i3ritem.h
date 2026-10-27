/* ==========================================================================
 *  i3d/i3ritem.h
 *
 *  The i3r item tree's core (i3d/i3ritem.cpp, ROR 0x22c1f8..0x22e840):
 *  the child lookup by name, the item type check, and the relocators
 *  i3rSetRelocate runs over an I3D_BIN file's item tree. See i3rroot.h
 *  for the item layout and the host's relative pointers.
 *
 *  Each item type has a pair of data fixups (tables 0x2db4c8 relocate /
 *  0x2db360 unrelocate, 0x5a types): the pointers inside an item's data
 *  are offsets from the start of that data, and some types also hold an
 *  array of such pointers with its count in the data (a u_int, u_short
 *  or u_char field). Types 0x00 / 0x01 and the invalid ones share a
 *  no-op.
 * ======================================================================== */
#ifndef _I3D_I3RITEM_H
#define _I3D_I3RITEM_H

#include "i3d/i3rroot.h"
#include "i3d/i3rtype.h"

/* ROR 0x0022c1f8: the index of the child of item named name (the children
 * are sorted by name: a bisection with strcmp), -1 when absent. */
int   i3rItemFindChild(const I3rItem *item, const char *name);           // ROR 0x0022c1f8
/* ROR 0x0022c2a0: 0 for the 13 types the tree may not hold (0x04, 0x16,
 * 0x24, 0x2c, 0x2e, 0x2f, 0x36, 0x3f, 0x40, 0x41, 0x48, 0x4e, 0x51), else
 * 1 -- types past 0x59 included (the relocators reject those with 3). */
int   i3rIsItemType(u_int type);                                         // ROR 0x0022c2a0
/* The item tree's relocators: recursive over the children, base = the
 * root item. Relocate turns the item's offsets (from base) into pointers
 * and then fixes its data by type (unless the info word's bit 31 says the
 * data needs none); unrelocate fixes the data and children first. I3D_OK,
 * 3 type > 0x59, 4 an invalid type; the first error ends the walk. */
int   i3rItemRelocate(I3rItem *item, I3rItem *base);                     // ROR 0x0022c478
int   i3rItemUnrelocate(I3rItem *item, I3rItem *base);                   // ROR 0x0022c328

#endif /* _I3D_I3RITEM_H */
