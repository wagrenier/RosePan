/* ==========================================================================
 *  mag/magsceneif.cpp
 *
 *  The scene's map and character interfaces: see magsceneif.h. The
 *  asserts carry this file's lines (0x1d .. 0x103) and, for the inline
 *  DogModel accessors, dog/dogmodel.h's (297, 310). The type_info
 *  functions MagSceneIF::__tf 0x2854d4 / MagSceneIFMap::__tf 0x28557c /
 *  MagSceneIFChara::__tf 0x285600 are not written (no RTTI is read on the
 *  host); the three destructors are the header's inline virtuals.
 * ======================================================================== */
#include "mag/magsceneif.h"

#include <stddef.h>
#include <string.h>

#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "mag/magloader.h"
#include "mag/magscene.h"
#include "utl/utlfile.h"

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00143498

int MagSceneIFMap::getMax() const
{
    ROR_ASSERT(m_scene_ptr); /* 29 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    return m_scene_ptr->m_map_loader.m_index.m_num;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001434e4

const char *MagSceneIFMap::getName(int theIndex) const
{
    ROR_ASSERT(m_scene_ptr); /* 38 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(0 <= theIndex && theIndex < getMax()); /* 39 */
    // PC: the ROM hangs in the assert; the host carries on, so no read
    // outside the table.
    if (theIndex < 0 || theIndex >= getMax())
        return NULL;
    return m_scene_ptr->m_map_loader.m_index.m_name[theIndex];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00143578

int MagSceneIFMap::find(const char *theName) const
{
    ROR_ASSERT(m_scene_ptr); /* 48 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(theName);     /* 49 */
    return m_scene_ptr->m_map_loader.m_index.find(theName);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001435f4

int MagSceneIFChara::getMax() const
{
    ROR_ASSERT(m_scene_ptr); /* 67 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    return m_scene_ptr->m_chara_loader.m_index.m_num;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00143640

const char *MagSceneIFChara::getName(int theIndex) const
{
    ROR_ASSERT(m_scene_ptr); /* 76 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(0 <= theIndex && theIndex < getMax()); /* 77 */
    // PC: as MagSceneIFMap::getName.
    if (theIndex < 0 || theIndex >= getMax())
        return NULL;
    return m_scene_ptr->m_chara_loader.m_index.m_name[theIndex];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001436d4

int MagSceneIFChara::find(const char *theName) const
{
    ROR_ASSERT(m_scene_ptr); /* 86 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(theName);     /* 87 */
    return m_scene_ptr->m_chara_loader.m_index.find(theName);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00143750

int MagSceneIFChara::getCategory(int theIndex) const
{
    ROR_ASSERT(m_scene_ptr); /* 96 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    return m_scene_ptr->m_chara_loader.getCategory(theIndex);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001437a8

int MagSceneIFChara::getMaxMotion(int theCharaID) const
{
    return m_scene_ptr->m_actors[theCharaID].m_max_motion;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001437c0

int MagSceneIFChara::getMotionNo(int theActor, const char *theName) const
{
    ROR_ASSERT(m_scene_ptr); /* 121 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    DogActor *actor = &m_scene_ptr->m_actors[theActor];
    int no = actor->getMotionNoByName(theName);
    if (no >= 0)
        return no;

    /* No motion of that name: cut it at its first '*' or '+' and match
     * the rest as a suffix. */
    char buf[0x100];
    utlStrCopy(buf, sizeof(buf), theName);
    buf[sizeof(buf) - 1] = '\0';
    if (buf[0] != '\0')
    {
        if (buf[0] == '*' || buf[0] == '+')
        {
            buf[0] = '\0';
        }
        else
        {
            for (char *p = buf + 1; *p != '\0'; p++)
            {
                if (*p == '*' || *p == '+')
                {
                    *p = '\0';
                    break;
                }
            }
        }
    }
    return actor->getMotionNoBySuffix(buf);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001438b0

const char *MagSceneIFChara::getMotionName(int theCharaID, int theMotionID) const
{
    ROR_ASSERT(m_scene_ptr);       /* 154 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(theCharaID >= 0);   /* 155 */
    ROR_ASSERT(theMotionID >= 0);  /* 156 */
    return m_scene_ptr->m_actors[theCharaID].getMotionName(theMotionID);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014395c

void MagSceneIFChara::getMotionLabel(char *theName, int theSize, int theCharaID,
                                     int theMotionID, int theAll) const
{
    ROR_ASSERT(m_scene_ptr);       /* 169 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(theName);           /* 170 */
    ROR_ASSERT(theSize > 16);      /* 171 */
    ROR_ASSERT(theCharaID >= 0);   /* 172 */
    ROR_ASSERT(theMotionID >= 0);  /* 173 */
    memset(theName, 0, theSize);

    const char *name = getMotionName(theCharaID, theMotionID);
    if (name == NULL)
        return;

    const char *p = name;
    while (*p != '_' && *p != '\0')
        p++;
    if (*p == '\0')
    {
        utlStrCopy(theName, theSize, name);
        return;
    }
    if (*p == '_')
        p++;
    else
        ROR_ASSERT(0); /* 205 */

    int i = 0;
    if (theAll)
    {
        while (*p != '\0' && i < theSize - 1)
            theName[i++] = *p++;
    }
    else
    {
        while (*p != '\0' && !(p[0] == '_' && p[1] == '_') && i < theSize - 1)
            theName[i++] = *p++;
    }
    theName[i] = '\0';
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00143ba0

int MagSceneIFChara::getNumNode(int theCharaID) const
{
    ROR_ASSERT(m_scene_ptr);       /* 245 */
    ROR_ASSERT(theCharaID >= 0);   /* 246 */
    /* DogModel's inline, hidden in DogActor by its get_nNode(int). */
    return m_scene_ptr->m_actors[theCharaID].DogModel::get_nNode();  // callcheck: dbgAssertFailed, i3rGetNumNode (the assert handler; inside the inline DogModel::get_nNode)
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00143c48

const char *MagSceneIFChara::getNodeName(int theCharaID, int theNodeNo) const
{
    ROR_ASSERT(m_scene_ptr);       /* 257 */
    ROR_ASSERT(theCharaID >= 0);   /* 258 */
    ROR_ASSERT(theNodeNo >= 0);    /* 259 */
    /* DogModel's inline, hidden in DogActor by its getNodeName(int, int). */
    return m_scene_ptr->m_actors[theCharaID].DogModel::getNodeName(theNodeNo);  // callcheck: dbgAssertFailed, i3rGetNodeNameByIndex (the assert handler; inside the inline DogModel::getNodeName)
}
