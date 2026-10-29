/* ==========================================================================
 *  mag/magsceneif.h
 *
 *  The scene's two loader interfaces (mag/magsceneif.cpp, pinned
 *  0x143498..0x143d3c plus the linkonce dtors 0x2854d4..0x285650, source
 *  string 0x2ef168): 8-byte objects at the end of the scene (+0x854e0 the
 *  map one, +0x854e8 the character one) holding m_scene_ptr, which
 *  MagScene's start sets and its destructor clears, and the vptr (GCC 2:
 *  after the data, +4; the only virtual is the destructor). Each method
 *  asserts m_scene_ptr and reads the scene's map or character loader
 *  (mag/magloader.h) or one of its actors (m_actors[theCharaID]).
 *  debug/motnamecnv*.cpp call them through bss 0x781260 / 0x781268.
 *  Class names come from the ELF's vtables; member and parameter names
 *  from the asserts (m_scene_ptr, theIndex, theName, theSize, theCharaID,
 *  theMotionID, theNodeNo, getMax()).
 * ======================================================================== */
#ifndef _MAG_MAGSCENEIF_H
#define _MAG_MAGSCENEIF_H

struct MagScene;

/* EE 8 bytes: m_scene_ptr, vptr at +4 (vtable 0x2e1fa0). layout: host. */
struct MagSceneIF
{
    virtual ~MagSceneIF() {}                                             // ROR 0x00285514

    MagScene *m_scene_ptr;      // ROR 0x00
};

/* The map units, by name (scene +0x854e0; vtable 0x2e1f88). layout: host. */
struct MagSceneIFMap : MagSceneIF
{
    virtual ~MagSceneIFMap() {}                                          // ROR 0x00285548

    /* The map index's entry count (MagMapLoader m_index.m_num). */
    int         getMax() const;                                          // ROR 0x00143498
    /* Entry theIndex's name (asserts 0 <= theIndex < getMax()). */
    const char *getName(int theIndex) const;                             // ROR 0x001434e4
    /* MagLoaderIndex::find of theName in the map index, -1 when none. */
    int         find(const char *theName) const;                         // ROR 0x00143578
};

/* The characters and their actors' motions and nodes (scene +0x854e8;
 * vtable 0x2e1f70). layout: host. */
struct MagSceneIFChara : MagSceneIF
{
    virtual ~MagSceneIFChara() {}                                        // ROR 0x002855cc

    /* The character index's entry count (MagCharaLoader m_index.m_num). */
    int         getMax() const;                                          // ROR 0x001435f4
    /* Entry theIndex's name (asserts 0 <= theIndex < getMax()). */
    const char *getName(int theIndex) const;                             // ROR 0x00143640
    /* MagLoaderIndex::find of theName in the character index, -1 when none. */
    int         find(const char *theName) const;                         // ROR 0x001436d4
    /* MagCharaLoader::getCategory of entry theIndex: its pack, 0..19. */
    int         getCategory(int theIndex) const;                         // ROR 0x00143750
    /* Scene actor theCharaID's motion record count (no asserts). */
    int         getMaxMotion(int theCharaID) const;                      // ROR 0x001437a8
    /* Scene actor theActor's motion named theName (getMotionNoByName); when
     * none, getMotionNoBySuffix of the name cut at its first '*' or '+'. */
    int         getMotionNo(int theActor, const char *theName) const;    // ROR 0x001437c0
    /* DogActor::getMotionName of scene actor theCharaID's motion
     * theMotionID. */
    const char *getMotionName(int theCharaID, int theMotionID) const;    // ROR 0x001438b0
    /* That motion's name into theName (theSize > 16 bytes, cleared first):
     * the text after its first '_' -- up to a "__" unless theAll -- or the
     * whole name when it has no '_'. Nothing when the motion has no name. */
    void        getMotionLabel(char *theName, int theSize, int theCharaID,
                               int theMotionID, int theAll) const;       // ROR 0x0014395c
    /* Scene actor theCharaID's model node count (DogModel::get_nNode). */
    int         getNumNode(int theCharaID) const;                        // ROR 0x00143ba0
    /* Its node theNodeNo's name (DogModel::getNodeName). */
    const char *getNodeName(int theCharaID, int theNodeNo) const;        // ROR 0x00143c48
};

#endif /* _MAG_MAGSCENEIF_H */
