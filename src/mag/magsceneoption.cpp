/* ==========================================================================
 *  mag/magsceneoption.cpp
 *
 *  The scene's tuning options (RoR's "mag/magsceneoption.cpp",
 *  0x13c328..0x13c664): MagScene::loadSystemData reads data/options.txt
 *  when the disc has one (the retail disc does not) and hands it here.
 *  Each "NAME value" line found in s_option_def sets a render scale, the
 *  camera's zoom unit, or one of the PC's movement tunings
 *  (MagLifePC::setParam). Unknown names are skipped.
 * ======================================================================== */
#include <string.h>

#include "common/ror_assert.h"
#include "mag/magscene.h"
#include "utl/utlmisc.h"

/* The option names and their ids (0x2ee548, 15 rows and a { NULL, -1 } end
 * that the search falls onto for an unknown name). Ids 6..14 are
 * MagLifePC::setParam's. EE 8 bytes; layout: host -- the name pointer. */
struct MagSceneOptionDef
{
    const char *m_name;
    int         m_id;
};

// 16 x 0x8 bytes at 0x002ee548 (datadump.py)
static const MagSceneOptionDef s_option_def[16] = {
    { "MAGTEST", 0 },             // 0x002ee548: a string, not used
    { "CAMERA_MOVE_SPEED", 1 },   // 0x002ee550: a float, not used
    { "ZOOM_RATIO", 2 },          // 0x002ee558
    { "SHADOW_BLACKNESS", 3 },    // 0x002ee560
    { "SHADOW_OFFSET", 4 },       // 0x002ee568
    { "AMBIENT_EXTRA", 5 },       // 0x002ee570
    { "PC_MOVE_LIMIT", 6 },       // 0x002ee578
    { "PC_PAD_ROTATE", 7 },       // 0x002ee580
    { "PC_PAD_FORWARD", 8 },      // 0x002ee588
    { "PC_ROTATE_SPEED", 9 },     // 0x002ee590
    { "PC_R180_LIMIT", 10 },      // 0x002ee598
    { "PC_PADDIFF_LIMIT", 11 },   // 0x002ee5a0
    { "PC_HIT_SIZE", 12 },        // 0x002ee5a8
    { "PC_HIT_OFFSET", 13 },      // 0x002ee5b0
    { "PC_HIT_CHECK_DISP", 14 },  // 0x002ee5b8
    { nullptr, -1 },              // 0x002ee5c0
};

// ──────────────────────────────────────────────────────────────────────
// Parses text (NUL-cut in place by utlParseParam) into m_option_bucket
// and applies each known option. An integer value counts as its float.
// Failed asserts fall through to the store, as on the ROM.
// callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
// ROR 0x0013c328
void MagScene::parseOption(char *text)
{
    int n = utlParseParam(m_option_bucket, MAX_OPTION, text);

    for (int i = 0; i < n; i++)
    {
        int def = 0;
        while (def < 15 && strcmp(m_option_bucket[i].m_name, s_option_def[def].m_name) != 0)
            def++;

        float value = m_option_bucket[i].m_type == UTLVB_TYPE_FLOAT
                          ? m_option_bucket[i].m_value.f
                          : (float)m_option_bucket[i].m_value.i;

        switch (s_option_def[def].m_id)
        {
        case 0:
            ROR_ASSERT(m_option_bucket[i].m_type == UTLVB_TYPE_STR);    /* 114 */
            break;
        case 1:
            ROR_ASSERT(m_option_bucket[i].m_type == UTLVB_TYPE_FLOAT);  /* 119 */
            break;
        case 2:
            m_camera_entity.m_144 = value;
            break;
        case 3:
            ROR_ASSERT(m_option_bucket[i].m_type == UTLVB_TYPE_FLOAT);  /* 128 */
            m_22a0 = value;
            break;
        case 4:
            ROR_ASSERT(m_option_bucket[i].m_type == UTLVB_TYPE_FLOAT);  /* 134 */
            m_opt_shadow_offset = value;
            ROR_ASSERT(m_opt_shadow_offset >= 0.9f && m_opt_shadow_offset <= 1.9f);  /* 137 */
            break;
        case 5:
            ROR_ASSERT(m_option_bucket[i].m_type == UTLVB_TYPE_FLOAT);  /* 141 */
            m_opt_ambient_extra = value;
            ROR_ASSERT(m_opt_ambient_extra >= 0.0f && m_opt_ambient_extra <= 2.0f);  /* 144 */
            break;
        case 6: case 7: case 8: case 9: case 10:
        case 11: case 12: case 13: case 14:
            m_pc.setParam(value, s_option_def[def].m_id);
            break;
        }
    }
}
