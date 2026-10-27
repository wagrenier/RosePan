/* ==========================================================================
 *  i3d/i3rtype.h
 *
 *  The i3r per-type accessors (i3d/i3rtype.cpp, ROR 0x22a6c0..0x22b1b0,
 *  just before i3rroot.cpp). For each item type the ROM has its fixup
 *  pair (written in i3ritem.cpp with the fixup tables) followed by one-line
 *  accessors over the item's child slots: child k is item->getChildren()
 *  [k], a fixed slot of the type's layout. Named i3r<Op><type>[_<k>] (no
 *  suffix for slot 0):
 *    GetSlot      the slot item (children + k)
 *    HasData      whether the slot's data is set (m_data != 0)
 *    GetNumChild  the slot's child count;  GetChild  its n-th child
 *    GetChildBegin / GetChildEnd   its child list and one past its end
 *    FindChild    the index of its child named name (i3rItemFindChild, -1)
 *    GetChildName the n-th child's name
 *    GetChildByName / HasChild     the named child / whether it exists
 *  Each type has only the accessors its layout needs; most have no callers
 *  in the ROM.
 * ======================================================================== */
#ifndef _I3D_I3RTYPE_H
#define _I3D_I3RTYPE_H

#include "i3d/i3rroot.h"

/* Type 0x4d (read by the model packet builders 0x220838.., unassigned@0021fb50);
 * fixups 0x22a6c0 / 0x22a730 in i3ritem.cpp. */
I3rItem    *i3rGetChildBegin4D(const I3rItem *item);                      // ROR 0x0022a7d0
I3rItem    *i3rGetChildEnd4D(const I3rItem *item);                        // ROR 0x0022a7e0
int         i3rGetNumChild4D_1(const I3rItem *item);                      // ROR 0x0022a800
I3rItem    *i3rGetChild4D_1(const I3rItem *item, int n);                  // ROR 0x0022a818
I3rItem    *i3rGetChildBegin4D_1(const I3rItem *item);                    // ROR 0x0022a830
I3rItem    *i3rGetChildEnd4D_1(const I3rItem *item);                      // ROR 0x0022a840
I3rItem    *i3rGetSlot4D_2(const I3rItem *item);                          // ROR 0x0022a860
int         i3rHasData4D_2(const I3rItem *item);                          // ROR 0x0022a870

/* Type 0x29 (a layer's three entity slots: i3rGetMatEntity0..2 / i3rSetMatEntity0..2);
 * fixups 0x22a880 / 0x22a888 in i3ritem.cpp. */
I3rItem    *i3rGetSlot29(const I3rItem *item);                            // ROR 0x0022a890
int         i3rHasData29(const I3rItem *item);                            // ROR 0x0022a898
I3rItem    *i3rGetSlot29_1(const I3rItem *item);                          // ROR 0x0022a8a8
int         i3rHasData29_1(const I3rItem *item);                          // ROR 0x0022a8b8
I3rItem    *i3rGetSlot29_2(const I3rItem *item);                          // ROR 0x0022a8c8
int         i3rHasData29_2(const I3rItem *item);                          // ROR 0x0022a8d8

/* Type 0x4a (read by the mesh packet builder 0x229380, i3d/i3rmesh.cpp);
 * fixups 0x22a8e8 / 0x22a8f0 in i3ritem.cpp. */
I3rItem    *i3rGetSlot4A(const I3rItem *item);                            // ROR 0x0022a8f8
int         i3rHasData4A(const I3rItem *item);                            // ROR 0x0022a900

/* Type 0x26 (a material layer: child 1 its entity slots (type 0x29), child 3 its STQ entity slots);
 * fixups 0x22a910 / 0x22a938 in i3ritem.cpp. */
I3rItem    *i3rGetSlot26(const I3rItem *item);                            // ROR 0x0022a960
int         i3rHasData26(const I3rItem *item);                            // ROR 0x0022a968
I3rItem    *i3rGetSlot26_1(const I3rItem *item);                          // ROR 0x0022a978
int         i3rHasData26_1(const I3rItem *item);                          // ROR 0x0022a988
I3rItem    *i3rGetSlot26_2(const I3rItem *item);                          // ROR 0x0022a998
int         i3rHasData26_2(const I3rItem *item);                          // ROR 0x0022a9a8
int         i3rGetNumChild26_3(const I3rItem *item);                      // ROR 0x0022a9b8
I3rItem    *i3rGetChild26_3(const I3rItem *item, int n);                  // ROR 0x0022a9d0
I3rItem    *i3rGetChildBegin26_3(const I3rItem *item);                    // ROR 0x0022a9e8
I3rItem    *i3rGetChildEnd26_3(const I3rItem *item);                      // ROR 0x0022a9f8

/* Type 0x59 (read by 0x220670, unassigned@0021fb50);
 * fixups 0x22aa18 / 0x22aa30 in i3ritem.cpp. */
int         i3rGetNumChild59(const I3rItem *item);                        // ROR 0x0022aa48
I3rItem    *i3rGetChild59(const I3rItem *item, int n);                    // ROR 0x0022aa60
I3rItem    *i3rGetChildBegin59(const I3rItem *item);                      // ROR 0x0022aa78
I3rItem    *i3rGetChildEnd59(const I3rItem *item);                        // ROR 0x0022aa88

/* Type 0x25 (a material: child 0 its layers);
 * fixups 0x22aaa8 / 0x22aab0 in i3ritem.cpp. */
int         i3rGetNumChild25(const I3rItem *item);                        // ROR 0x0022aab8
I3rItem    *i3rGetChild25(const I3rItem *item, int n);                    // ROR 0x0022aad0
I3rItem    *i3rGetChildBegin25(const I3rItem *item);                      // ROR 0x0022aae8
I3rItem    *i3rGetChildEnd25(const I3rItem *item);                        // ROR 0x0022aaf8

/* Type 0x2d (a combined mesh: children 0 / 1 the mesh lists i3rBuildPackets walks);
 * fixups 0x22ab18 / 0x22ab70 in i3ritem.cpp. */
I3rItem    *i3rGetSlot2D(const I3rItem *item);                            // ROR 0x0022abc0
int         i3rHasData2D(const I3rItem *item);                            // ROR 0x0022abc8
I3rItem    *i3rGetSlot2D_1(const I3rItem *item);                          // ROR 0x0022abd8
int         i3rHasData2D_1(const I3rItem *item);                          // ROR 0x0022abe8

/* Type 0x2a (a node; utlGetI3rNodeDisp and the effect code read child 0);
 * fixups 0x22abf8 / 0x22ac00 in i3ritem.cpp. */
I3rItem    *i3rGetSlot2A(const I3rItem *item);                            // ROR 0x0022ac08
int         i3rHasData2A(const I3rItem *item);                            // ROR 0x0022ac10
int         i3rGetNumChild2A_1(const I3rItem *item);                      // ROR 0x0022ac20
I3rItem    *i3rGetChild2A_1(const I3rItem *item, int n);                  // ROR 0x0022ac38
int         i3rFindChild2A_1(const I3rItem *item, const char *name);      // ROR 0x0022ac50
const char *i3rGetChildName2A_1(const I3rItem *item, int n);              // ROR 0x0022ac70
I3rItem    *i3rGetChildByName2A_1(const I3rItem *item, const char *name); // ROR 0x0022ac88
int         i3rHasChild2A_1(const I3rItem *item, const char *name);       // ROR 0x0022acc8
I3rItem    *i3rGetChildBegin2A_1(const I3rItem *item);                    // ROR 0x0022acf0
I3rItem    *i3rGetChildEnd2A_1(const I3rItem *item);                      // ROR 0x0022ad00

/* Type 0x52 (the root item: child 0 the materials, 2 the meshes, 3 the nodes, 6 the packet slot);
 * fixups 0x22ad20 / 0x22adf8 in i3ritem.cpp. */
int         i3rGetNumChild52(const I3rItem *item);                        // ROR 0x0022aec0
I3rItem    *i3rGetChild52(const I3rItem *item, int n);                    // ROR 0x0022aed8
int         i3rFindChild52(const I3rItem *item, const char *name);        // ROR 0x0022aef0
const char *i3rGetChildName52(const I3rItem *item, int n);                // ROR 0x0022af08
I3rItem    *i3rGetChildByName52(const I3rItem *item, const char *name);   // ROR 0x0022af20
int         i3rHasChild52(const I3rItem *item, const char *name);         // ROR 0x0022af58
I3rItem    *i3rGetChildBegin52(const I3rItem *item);                      // ROR 0x0022af80
I3rItem    *i3rGetChildEnd52(const I3rItem *item);                        // ROR 0x0022af90
I3rItem    *i3rGetSlot52_1(const I3rItem *item);                          // ROR 0x0022afb0
int         i3rHasData52_1(const I3rItem *item);                          // ROR 0x0022afc0
int         i3rGetNumChild52_2(const I3rItem *item);                      // ROR 0x0022afd0
I3rItem    *i3rGetChild52_2(const I3rItem *item, int n);                  // ROR 0x0022afe8
int         i3rFindChild52_2(const I3rItem *item, const char *name);      // ROR 0x0022b000
const char *i3rGetChildName52_2(const I3rItem *item, int n);              // ROR 0x0022b020
I3rItem    *i3rGetChildByName52_2(const I3rItem *item, const char *name); // ROR 0x0022b038
int         i3rHasChild52_2(const I3rItem *item, const char *name);       // ROR 0x0022b078
I3rItem    *i3rGetChildBegin52_2(const I3rItem *item);                    // ROR 0x0022b0a0
I3rItem    *i3rGetChildEnd52_2(const I3rItem *item);                      // ROR 0x0022b0b0
int         i3rGetNumChild52_3(const I3rItem *item);                      // ROR 0x0022b0d0
I3rItem    *i3rGetChild52_3(const I3rItem *item, int n);                  // ROR 0x0022b0e8
I3rItem    *i3rGetChildBegin52_3(const I3rItem *item);                    // ROR 0x0022b100
I3rItem    *i3rGetChildEnd52_3(const I3rItem *item);                      // ROR 0x0022b110
I3rItem    *i3rGetSlot52_4(const I3rItem *item);                          // ROR 0x0022b130
int         i3rHasData52_4(const I3rItem *item);                          // ROR 0x0022b140
I3rItem    *i3rGetSlot52_5(const I3rItem *item);                          // ROR 0x0022b150
int         i3rHasData52_5(const I3rItem *item);                          // ROR 0x0022b160
I3rItem    *i3rGetSlot52_6(const I3rItem *item);                          // ROR 0x0022b170
int         i3rHasData52_6(const I3rItem *item);                          // ROR 0x0022b180
I3rItem    *i3rGetSlot52_7(const I3rItem *item);                          // ROR 0x0022b190
int         i3rHasData52_7(const I3rItem *item);                          // ROR 0x0022b1a0

#endif /* _I3D_I3RTYPE_H */
