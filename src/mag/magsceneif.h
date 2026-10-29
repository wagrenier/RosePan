/* ==========================================================================
 *  mag/magsceneif.h
 *
 *  The scene's two loader interfaces (mag/magsceneif.cpp, 0x143430..
 *  0x143d3c, source string 0x2ef168): one-word objects at the end of the
 *  scene (+0x854e0 the map one, +0x854e8 the character one) holding
 *  m_scene_ptr, which MagScene's start sets and its constructor clears.
 *  Each method asserts m_scene_ptr and reads the scene's map or character
 *  loader (mag/magloader.h). debug/motnamecnv*.cpp call them. Class names
 *  are descriptive: only the member and parameter names come from the
 *  asserts (m_scene_ptr, theIndex, theName, getMax()).
 * ======================================================================== */
#ifndef _MAG_MAGSCENEIF_H
#define _MAG_MAGSCENEIF_H

struct MagScene;

/* The map units, by name (scene +0x854e0, EE 4 bytes). layout: host. */
struct MagSceneIFMap
{
    /* The map index's entry count (MagMapLoader m_index.m_num). */
    int         getMax() const;                                          // ROR 0x00143498
    /* Entry theIndex's name (asserts 0 <= theIndex < getMax()). */
    const char *getName(int theIndex) const;                             // ROR 0x001434e4
    /* MagLoaderIndex::find of theName in the map index, -1 when none. */
    int         find(const char *theName) const;                         // ROR 0x00143578

    MagScene *m_scene_ptr;      // ROR 0x00
};

/* The characters and their actors' motions (scene +0x854e8, EE 4 bytes).
 * layout: host. */
struct MagSceneIFChara
{
    /* The character index's entry count (MagCharaLoader m_index.m_num). */
    int         getMax() const;                                          // ROR 0x001435f4
    /* Entry theIndex's name (asserts 0 <= theIndex < getMax()). */
    const char *getName(int theIndex) const;                             // ROR 0x00143640
    /* MagLoaderIndex::find of theName in the character index, -1 when none. */
    int         find(const char *theName) const;                         // ROR 0x001436d4
    /* MagCharaLoader::getCategory of entry theIndex: its pack, 0..19. */
    int         getCategory(int theIndex) const;                         // ROR 0x00143750
    /* Scene actor theActor's motion named theName (getMotionNoByName); when
     * none, getMotionNoBySuffix of the name cut at its first '*' or '+'. */
    int         getMotionNo(int theActor, const char *theName) const;    // ROR 0x001437c0

    MagScene *m_scene_ptr;      // ROR 0x00
};

#endif /* _MAG_MAGSCENEIF_H */
