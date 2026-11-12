/* ==========================================================================
 *  debug/motnamecnv0.h
 *
 *  debug/motnamecnv0.cpp (RoR's source string at 0x301a08; pinned
 *  0x216154..0x2163ac): free functions over the scene's loader interfaces
 *  (mag/magsceneif.h; MagScene +0x854e0 the map one, +0x854e8 the
 *  character one, bss 0x781260 / 0x781268) -- character lookups for
 *  scenePlayStart, and motNameConvert, which turns a scene thread's motion
 *  keys into the actor's motion numbers. The four map / count wrappers have
 *  no callers in the retail ROM.
 * ======================================================================== */
#ifndef _DEBUG_MOTNAMECNV0_H
#define _DEBUG_MOTNAMECNV0_H

#include <stdint.h>

struct ScenePlayRecord;

/* MagSceneIFChara::getMotionNo: actor's motion named name. */
int  motNameGetMotionNo(int actor, const char *name);                    // ROR 0x00216154
/* A scene thread's records (0x48 bytes each, chained while the next
 * one's kind is set): each kind-2 record's motion key (+8) is looked up
 * in names ({name, key} pairs ending with a NULL name) and replaced by
 * actor's motion number of that name (0 when the actor has none). Returns
 * 1 when every kind-2 record converted, 0 when a key had no name or a
 * name no motion. */
int  motNameConvert(int actor, ScenePlayRecord *records,
                    const intptr_t *names);                              // ROR 0x00216184
/* The character index of name (MagSceneIFChara::find; asserts name). */
int  motNameFindChara(const char *name);                                 // ROR 0x0021628c
/* MagSceneIFChara::getCategory of character index chara: scenePlayStart
 * picks the object type and model arg from it (0..6 named kinds, 8 and
 * up the numbered ones). */
int  motNameGetCategory(int chara);                                      // ROR 0x002162e0
/* MagSceneIFMap::getMax. No callers. */
int  motNameGetMapMax();                                                 // ROR 0x0021630c
/* MagSceneIFMap::getName(theIndex). No callers; the ROM forwards its
 * second argument register, so the first is unused. */
const char *motNameGetMapName(int unused, int theIndex);                 // ROR 0x00216334
/* MagSceneIFMap::find(theName). No callers; as motNameGetMapName. */
int  motNameFindMap(int unused, const char *theName);                    // ROR 0x0021635c
/* MagSceneIFChara::getMax. No callers. */
int  motNameGetCharaMax();                                               // ROR 0x00216384

#endif /* _DEBUG_MOTNAMECNV0_H */
