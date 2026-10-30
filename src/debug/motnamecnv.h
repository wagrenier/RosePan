/* ==========================================================================
 *  debug/motnamecnv.h
 *
 *  debug/motnamecnv.cpp (RoR's source string at 0x301a40; pinned
 *  0x2163ac..0x216768, after motnamecnv0.cpp and before naga_debug.cpp):
 *  more wrappers over the scene's character interface (mag/magsceneif.h;
 *  MagScene +0x854e8, bss 0x781268) -- character names, motion labels and
 *  names into a caller's buffer, motion and node counts -- and two copies
 *  of the assert printer. Nothing in the retail ROM calls any of them.
 *  Every wrapper leaves a0 alone and forwards a1.., so each takes an unused
 *  first argument (as motNameGetMapName does).
 * ======================================================================== */
#ifndef _DEBUG_MOTNAMECNV_H
#define _DEBUG_MOTNAMECNV_H

/* MagSceneIFChara::getName(theIndex), "----" for a negative index. */
const char *motNameGetCharaName(int unused, int theIndex);               // ROR 0x002163ac
/* MagSceneIFChara::find(str); a name it does not know is 0 when it starts
 * with "PC" (the player), else -1. */
int  motNameFindCharaOrPC(int unused, const char *str);                  // ROR 0x0021641c
/* Clears theBuf (theSize > 16 bytes) and fills it with
 * MagSceneIFChara::getMotionLabel of scene actor chara_num's motion
 * motion_num, the whole label (theAll 1); returns theBuf. */
char *motNameGetMotionLabel(int unused, char *theBuf, int theSize,
                            int chara_num, int motion_num);              // ROR 0x0021649c
/* MagSceneIFChara::getMaxMotion(theCharaID). */
int  motNameGetMaxMotion(int unused, int theCharaID);                    // ROR 0x00216598
/* Clears theBuf (theSize > 16 bytes) and copies scene actor theCharaID's
 * motion theMotionID's name into it upper-cased (utlStrNCopyUpper);
 * returns theBuf, left empty when the motion has no name. */
char *motNameGetMotionName(int unused, char *theBuf, int theSize,
                           int theCharaID, int theMotionID);             // ROR 0x002165c0
/* MagSceneIFChara::getNumNode(theCharaID). */
int  motNameGetNumNode(int unused, int theCharaID);                      // ROR 0x00216690
/* MagSceneIFChara::getNodeName(theCharaID, theNodeNo). */
const char *motNameGetNodeName(int unused, int theCharaID, int theNodeNo); // ROR 0x002166b8
/* Two unreferenced copies of dbgAssertFailed (0x215a30): print
 * "Assertion Failed: file(line)   expr" and loop for ever on the ROM.
 * Ghidra had folded both into the end of 0x2166b8. */
void motNameAssertFailed(const char *file, int line, const char *expr);  // ROR 0x002166e0
void motNameAssertFailed2(const char *file, int line, const char *expr); // ROR 0x00216728

#endif /* _DEBUG_MOTNAMECNV_H */
