/* ==========================================================================
 *  debug/motnamecnv0.h
 *
 *  debug/motnamecnv0.cpp (RoR's source string at 0x301a08): looks
 *  character and motion names up in the loader's indexes (MagLoader at
 *  bss 0x781268). Minimal: what scenePlayStart calls.
 * ======================================================================== */
#ifndef _DEBUG_MOTNAMECNV0_H
#define _DEBUG_MOTNAMECNV0_H

#include <stdint.h>

/* The character index of name (MagLoaderIndex::find on the loader's
 * character index; asserts name). */
int  motNameFindChara(const char *name);                                 // ROR 0x0021628c
/* MagCharaLoader::getCategory of character index chara: scenePlayStart
 * picks the object type and model arg from it (0..6 named kinds, 8 and
 * up the numbered ones). */
int  motNameGetCategory(int chara);                                      // ROR 0x002162e0
/* A scene thread's records (0x48 bytes each, chained while the next
 * one's kind is set): each kind-2 record's motion key (+8) is looked up
 * in names ({name, key} pairs ending with a NULL name) and replaced by
 * actor's motion number of that name. 0 when a key had no name. */
int  motNameConvert(int actor, void *records, const intptr_t *names);    // ROR 0x00216184

#endif /* _DEBUG_MOTNAMECNV0_H */
