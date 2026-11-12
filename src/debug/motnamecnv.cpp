/* ==========================================================================
 *  debug/motnamecnv.cpp
 *
 *  Character, motion and node name wrappers over the scene's character
 *  interface, and two copies of the assert printer (pinned
 *  0x2163ac..0x216768): see motnamecnv.h. Every wrapper goes through
 *  g_magSystem.m_scene.m_chara_if, as the ROM reaches it at bss 0x781268.
 * ======================================================================== */
#include "debug/motnamecnv.h"

#include <stdio.h>
#include <string.h>

#include "common/ror_assert.h"
#include "lib/unassigned.h"
#include "mag/magsceneif.h"
#include "mag/magsystem.h"

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002163ac

const char *motNameGetCharaName(int unused, int theIndex)
{
    (void)unused;
    const char *p;
    if (theIndex < 0)
        p = "----";
    else
        p = g_magSystem.m_scene.m_chara_if.getName(theIndex);
    ROR_ASSERT(p); /* 191 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    return p;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0021641c

int motNameFindCharaOrPC(int unused, const char *str)
{
    (void)unused;
    ROR_ASSERT(str); /* 216 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

    int no = g_magSystem.m_scene.m_chara_if.find(str);
    if (no >= 0)
        return no;
    if (str[0] == 'P' && str[1] == 'C')
        return 0;
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0021649c

char *motNameGetMotionLabel(int unused, char *theBuf, int theSize,
                            int chara_num, int motion_num)
{
    (void)unused;
    ROR_ASSERT(theBuf);          /* 261 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(theSize > 16);    /* 262 */
    ROR_ASSERT(chara_num >= 0);  /* 263 */
    ROR_ASSERT(motion_num >= 0); /* 264 */

    memset(theBuf, 0, theSize);
    g_magSystem.m_scene.m_chara_if.getMotionLabel(theBuf, theSize, chara_num, motion_num, 1);
    return theBuf;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00216598

int motNameGetMaxMotion(int unused, int theCharaID)
{
    (void)unused;
    return g_magSystem.m_scene.m_chara_if.getMaxMotion(theCharaID);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002165c0

char *motNameGetMotionName(int unused, char *theBuf, int theSize,
                           int theCharaID, int theMotionID)
{
    (void)unused;
    ROR_ASSERT(theBuf);       /* 318 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(theSize > 16); /* 319 */

    memset(theBuf, 0, theSize);
    const char *name = g_magSystem.m_scene.m_chara_if.getMotionName(theCharaID, theMotionID);
    if (name != NULL)
        utlStrNCopyUpper(theBuf, theSize, name);
    return theBuf;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00216690

int motNameGetNumNode(int unused, int theCharaID)
{
    (void)unused;
    return g_magSystem.m_scene.m_chara_if.getNumNode(theCharaID);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002166b8

const char *motNameGetNodeName(int unused, int theCharaID, int theNodeNo)
{
    (void)unused;
    return g_magSystem.m_scene.m_chara_if.getNodeName(theCharaID, theNodeNo);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002166e0; callcheck: scePrintf (printf on the host)

void motNameAssertFailed(const char *file, int line, const char *expr)
{
    printf("Assertion Failed: %s(%d)   %s\n", file, line, expr);
    // PORT: the ROM loops for ever here (common/ror_assert.h).
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00216728; callcheck: scePrintf (printf on the host)

void motNameAssertFailed2(const char *file, int line, const char *expr)
{
    printf("Assertion Failed: %s(%d)   %s\n", file, line, expr);
    // PORT: the ROM loops for ever here (common/ror_assert.h).
}
