/* ==========================================================================
 *  debug/motnamecnv0.cpp
 *
 *  Character and motion name lookups over the scene's loader interfaces
 *  (pinned 0x216154..0x2163ac): see motnamecnv0.h. Every function goes
 *  through g_magSystem.m_scene's m_map_if / m_chara_if, as the ROM reaches
 *  them at bss 0x781260 / 0x781268.
 * ======================================================================== */
#include "debug/motnamecnv0.h"

#include <stddef.h>

#include "common/ror_assert.h"
#include "mag/magsceneif.h"
#include "mag/magsystem.h"
#include "task/tsk_scene.h"

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00216154

int motNameGetMotionNo(int actor, const char *name)
{
    return g_magSystem.m_scene.m_chara_if.getMotionNo(actor, name);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00216184

int motNameConvert(int actor, ScenePlayRecord *p, const intptr_t *tbl)
{
    ROR_ASSERT(p);   /* 119 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(tbl); /* 120 */

    int result = 1;
    for (; p->m_kind != 0; p++)
    {
        if (p->m_kind != 2)
            continue;

        const char *name = NULL;
        int no = -1;
        for (const intptr_t *e = tbl; e[0] != 0; e += 2)
        {
            if ((int)e[1] == p->m_08)
            {
                name = (const char *)e[0];
                no = motNameGetMotionNo(actor, name);
                p->m_08 = no;
                break;
            }
        }
        if (name != NULL)
        {
            if (no != -1)
                continue;
            p->m_08 = 0;
        }
        result = 0;
    }
    return result;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0021628c

int motNameFindChara(const char *str)
{
    ROR_ASSERT(str); /* 207 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    return g_magSystem.m_scene.m_chara_if.find(str);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x002162e0

int motNameGetCategory(int chara)
{
    return g_magSystem.m_scene.m_chara_if.getCategory(chara);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0021630c

int motNameGetMapMax()
{
    return g_magSystem.m_scene.m_map_if.getMax();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00216334

const char *motNameGetMapName(int unused, int theIndex)
{
    (void)unused;
    return g_magSystem.m_scene.m_map_if.getName(theIndex);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0021635c

int motNameFindMap(int unused, const char *theName)
{
    (void)unused;
    return g_magSystem.m_scene.m_map_if.find(theName);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00216384

int motNameGetCharaMax()
{
    return g_magSystem.m_scene.m_chara_if.getMax();
}
