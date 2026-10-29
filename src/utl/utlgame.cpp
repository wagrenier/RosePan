/* ==========================================================================
 *  utl/utlgame.cpp
 *
 *  The game's one-call helpers (ROR 0x1024d0..0x1041c4). See utlgame.h.
 *
 *  The searches over GamMan's object lists walk the pools' used lists
 *  (m_fchrList 0x79ec50, m_vfchrList 0x79ef90, m_avfchrList 0x7a01d0,
 *  m_achrList 0x79e610). The dog's oid (GamMan::m_dogPtr->m_oid, 0 with
 *  no dog) is inline in the ROM at each use: dogOid() below.
 * ======================================================================== */
#include "utl/utlgame.h"

#include "common/ror_assert.h"
#include "dog/dogactor.h"
#include "emy/emy_manager.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "mag/magsystem.h"
#include "sequence/sequence.h"
#include "task/tsk_scene.h"
#include "utl/utlmath.h"
#include "utl/utlrand.h"

/* ---- data ------------------------------------------------------------- */

// 200 x 0x10 bytes at 0x002e6978 (datadump.py)
static const UtlSoundRec tbl_sound[UTL_SOUND_MAX] = {
    { 6, -1, 0, "SE_SAVE_FINISH" },  // 0x002e6978
    { 3, -1, 1, "SE_DAMAGE_BIG" },  // 0x002e6988
    { 3, -1, 2, "SE_FALL" },  // 0x002e6998
    { 17, -1, 3, "SE_GET_ITEM" },  // 0x002e69a8
    { 21, -1, 4, "SE_DOG_BREATH_BIG" },  // 0x002e69b8
    { 22, -1, 5, "SE_DOG_BREATH_SNEAK" },  // 0x002e69c8
    { 23, -1, 6, "SE_DOG_BREATH_SNEAK_2" },  // 0x002e69d8
    { 24, -1, 7, "SE_DOG_BREATH_DEPEND" },  // 0x002e69e8
    { 25, -1, 8, "SE_DOG_BARK_QUESTION" },  // 0x002e69f8
    { 26, -1, 9, "SE_DOG_BARK_DEPEND" },  // 0x002e6a08
    { 27, -1, 10, "SE_DOG_GROWL" },  // 0x002e6a18
    { 28, -1, 11, "SE_DOG_HOWL" },  // 0x002e6a28
    { 29, -1, 12, "SE_DOG_HOWL_L" },  // 0x002e6a38
    { 30, -1, 13, "SE_DOG_ROAR" },  // 0x002e6a48
    { 31, -1, 14, "SE_DOG_ROAR_L" },  // 0x002e6a58
    { 32, -1, 15, "SE_DOG_YELP_S" },  // 0x002e6a68
    { 33, -1, 16, "SE_DOG_YELP_L" },  // 0x002e6a78
    { 34, -1, 17, "SE_DOG_BITE" },  // 0x002e6a88
    { 35, -1, 18, "SE_DOG_BITE_WAIT" },  // 0x002e6a98
    { 36, -1, 19, "SE_DOG_GARIGARI" },  // 0x002e6aa8
    { 37, -1, 20, "SE_PC_BREATH_NORM" },  // 0x002e6ab8
    { 38, -1, 21, "SE_PC_BREATH_NEAR_DEAD" },  // 0x002e6ac8
    { 39, -1, 22, "SE_PC_BREATH_EMPTY" },  // 0x002e6ad8
    { 40, -1, 23, "SE_PC_PURR_EMPTY" },  // 0x002e6ae8
    { 41, -1, 24, "SE_PC_COUGH" },  // 0x002e6af8
    { 42, -1, 25, "SE_PC_ATTACK_1" },  // 0x002e6b08
    { 43, -1, 26, "SE_PC_ATTACK_2" },  // 0x002e6b18
    { 44, -1, 27, "SE_PC_ATTACK_3" },  // 0x002e6b28
    { 45, -1, 28, "SE_PC_ATTACK_4" },  // 0x002e6b38
    { 46, -1, 29, "SE_PC_ATTACK_5" },  // 0x002e6b48
    { 48, -1, 30, "SE_PC_DAMGE_1" },  // 0x002e6b58
    { 49, -1, 31, "SE_PC_DAMGE_2" },  // 0x002e6b68
    { 50, -1, 32, "SE_PC_DAMGE_3" },  // 0x002e6b78
    { 51, -1, 33, "SE_PC_LAST_1" },  // 0x002e6b88
    { 52, -1, 34, "SE_PC_LAST_2" },  // 0x002e6b98
    { 53, -1, 35, "SE_PC_CMD_COME_1" },  // 0x002e6ba8
    { 54, -1, 36, "SE_PC_CMD_COME_2" },  // 0x002e6bb8
    { 55, -1, 37, "SE_PC_CMD_COME_3" },  // 0x002e6bc8
    { 56, -1, 38, "SE_PC_CMD_GO_1" },  // 0x002e6bd8
    { 57, -1, 39, "SE_PC_CMD_GO_2" },  // 0x002e6be8
    { 58, -1, 40, "SE_PC_CMD_GO_3" },  // 0x002e6bf8
    { 61, -1, 41, "SE_PC_CMD_STAY_1" },  // 0x002e6c08
    { 62, -1, 42, "SE_PC_CMD_STAY_2" },  // 0x002e6c18
    { 59, -1, 43, "SE_PC_CMD_WAIT_1" },  // 0x002e6c28
    { 60, -1, 44, "SE_PC_CMD_WAIT_2" },  // 0x002e6c38
    { 63, -1, 45, "SE_PC_CMD_SHITDOWN" },  // 0x002e6c48
    { 64, -1, 46, "SE_PC_SHAKE_OFF" },  // 0x002e6c58
    { 65, -1, 47, "SE_PC_ASLEEP" },  // 0x002e6c68
    { 66, -1, 48, "SE_PC_TURN_IN_BED" },  // 0x002e6c78
    { 67, -1, 49, "SE_PC_SUP_LIGHT" },  // 0x002e6c88
    { 68, -1, 50, "SE_PC_POWER" },  // 0x002e6c98
    { 69, -1, 51, "SE_PC_SIGH" },  // 0x002e6ca8
    { 70, -1, 52, "SE_PC_YAWN" },  // 0x002e6cb8
    { 71, -1, 53, "SE_PC_GOOD_DOG" },  // 0x002e6cc8
    { 72, -1, 54, "SE_PC_CALL_BROWN_1" },  // 0x002e6cd8
    { 73, -1, 55, "SE_PC_CALL_BROWN_2" },  // 0x002e6ce8
    { 74, -1, 56, "SE_PC_CALL_BROWN_3" },  // 0x002e6cf8
    { 75, -1, 57, "SE_PC_SUP_1" },  // 0x002e6d08
    { 76, -1, 58, "SE_PC_SUP_3" },  // 0x002e6d18
    { 77, -1, 59, "SE_PC_GUARD_1" },  // 0x002e6d28
    { 78, -1, 60, "SE_PC_R_U_OK" },  // 0x002e6d38
    { 79, -1, 61, "SE_PC_SMILE" },  // 0x002e6d48
    { 80, -1, 62, "SE_PC_CRY_BROWN_DEAD" },  // 0x002e6d58
    { 47, -1, 63, "SE_PC_SHOCK" },  // 0x002e6d68
    { 1073744179, -1, 64, "SE_SLAP" },  // 0x002e6d78
    { 1073744180, -1, 65, "SE_HAND_PUSH" },  // 0x002e6d88
    { 1073744181, -1, 66, "SE_HAND_FLUSH_PUSH" },  // 0x002e6d98
    { 1073744182, -1, 67, "SE_KICK_LOW" },  // 0x002e6da8
    { 1073744183, -1, 68, "SE_MEAT_KICK_LOW" },  // 0x002e6db8
    { 1073744184, -1, 69, "SE_HAND_PC_ATTACK_1" },  // 0x002e6dc8
    { 1073744185, -1, 70, "SE_HAND_PC_ATTACK_4" },  // 0x002e6dd8
    { 1073809722, -1, 71, "SE_KNIFE_MEAT_HIT" },  // 0x002e6de8
    { 1073809723, -1, 72, "SE_KNIFE_SWING_1" },  // 0x002e6df8
    { 1073809724, -1, 73, "SE_KNIFE_SWING_2" },  // 0x002e6e08
    { 1073809725, -1, 74, "SE_KNIFE_SWING_3" },  // 0x002e6e18
    { 1073809726, -1, 75, "SE_KNIFE_PC_ATTACK_1" },  // 0x002e6e28
    { 1073809727, -1, 76, "SE_KNIFE_PC_ATTACK_2" },  // 0x002e6e38
    { 1073809728, -1, 77, "SE_KNIFE_PC_ATTACK_3" },  // 0x002e6e48
    { 1073809729, -1, 78, "SE_KNIFE_PC_ATTACK_4" },  // 0x002e6e58
    { 1073809730, -1, 79, "SE_KNIFE_PC_ATTACK_5" },  // 0x002e6e68
    { 1073875273, -1, 80, "SE_HOUCHOU_MEAT_HIT" },  // 0x002e6e78
    { 1073875274, -1, 81, "SE_HOUCHOU_SWING_1" },  // 0x002e6e88
    { 1073875275, -1, 82, "SE_HOUCHOU_SWING_2" },  // 0x002e6e98
    { 1073875276, -1, 83, "SE_HOUCHOU_SWING_3" },  // 0x002e6ea8
    { 1073875277, -1, 84, "SE_HOUCHOU_PC_ATTACK_1" },  // 0x002e6eb8
    { 1073875278, -1, 85, "SE_HOUCHOU_PC_ATTACK_2" },  // 0x002e6ec8
    { 1073875279, -1, 86, "SE_HOUCHOU_PC_ATTACK_3" },  // 0x002e6ed8
    { 1073875280, -1, 87, "SE_HOUCHOU_PC_ATTACK_4" },  // 0x002e6ee8
    { 1073875281, -1, 88, "SE_HOUCHOU_PC_ATTACK_5" },  // 0x002e6ef8
    { 1073940824, -1, 89, "SE_RAPIER_MEAT_HIT" },  // 0x002e6f08
    { 1073940825, -1, 90, "SE_RAPIER_SWING_1" },  // 0x002e6f18
    { 1073940826, -1, 91, "SE_RAPIER_SWING_2" },  // 0x002e6f28
    { 1073940827, -1, 92, "SE_RAPIER_SWING_3" },  // 0x002e6f38
    { 1073940828, -1, 93, "SE_RAPIER_PC_ATTACK_1" },  // 0x002e6f48
    { 1073940829, -1, 94, "SE_RAPIER_PC_ATTACK_2" },  // 0x002e6f58
    { 1073940830, -1, 95, "SE_RAPIER_PC_ATTACK_3" },  // 0x002e6f68
    { 1073940831, -1, 96, "SE_RAPIER_PC_ATTACK_4" },  // 0x002e6f78
    { 1073940832, -1, 97, "SE_RAPIER_PC_ATTACK_5" },  // 0x002e6f88
    { 1074006375, -1, 98, "SE_AXE_HIT" },  // 0x002e6f98
    { 1074006376, -1, 99, "SE_AXE_SWING_1" },  // 0x002e6fa8
    { 1074006377, -1, 100, "SE_AXE_SWING_2" },  // 0x002e6fb8
    { 1074006378, -1, 101, "SE_AXE_SWING_3" },  // 0x002e6fc8
    { 1074006379, -1, 102, "SE_AXE_PC_ATTACK_1" },  // 0x002e6fd8
    { 1074006380, -1, 103, "SE_AXE_PC_ATTACK_2" },  // 0x002e6fe8
    { 1074006381, -1, 104, "SE_AXE_PC_ATTACK_3" },  // 0x002e6ff8
    { 1074006382, -1, 105, "SE_AXE_PC_ATTACK_4" },  // 0x002e7008
    { 1074006383, -1, 106, "SE_AXE_PC_ATTACK_5" },  // 0x002e7018
    { 1074071926, -1, 107, "SE_PIPE_HIT" },  // 0x002e7028
    { 1074071927, -1, 108, "SE_PIPE_SWING_1" },  // 0x002e7038
    { 1074071928, -1, 109, "SE_PIPE_SWING_2" },  // 0x002e7048
    { 1074071929, -1, 110, "SE_PIPE_SWING_3" },  // 0x002e7058
    { 1074071930, -1, 111, "SE_PIPE_PC_ATTACK_1" },  // 0x002e7068
    { 1074071931, -1, 112, "SE_PIPE_PC_ATTACK_2" },  // 0x002e7078
    { 1074071932, -1, 113, "SE_PIPE_PC_ATTACK_3" },  // 0x002e7088
    { 1074071933, -1, 114, "SE_PIPE_PC_ATTACK_4" },  // 0x002e7098
    { 1074071934, -1, 115, "SE_PIPE_PC_ATTACK_5" },  // 0x002e70a8
    { 1074137477, -1, 116, "SE_SCOOP_HIT" },  // 0x002e70b8
    { 1074137478, -1, 117, "SE_SCOOP_SWING_1" },  // 0x002e70c8
    { 1074137479, -1, 118, "SE_SCOOP_SWING_2" },  // 0x002e70d8
    { 1074137480, -1, 119, "SE_SCOOP_SWING_3" },  // 0x002e70e8
    { 1074137481, -1, 120, "SE_SCOOP_PC_ATTACK_1" },  // 0x002e70f8
    { 1074137482, -1, 121, "SE_SCOOP_PC_ATTACK_2" },  // 0x002e7108
    { 1074137483, -1, 122, "SE_SCOOP_PC_ATTACK_3" },  // 0x002e7118
    { 1074137484, -1, 123, "SE_SCOOP_PC_ATTACK_4" },  // 0x002e7128
    { 1074137485, -1, 124, "SE_SCOOP_PC_ATTACK_5" },  // 0x002e7138
    { 1074203028, -1, 125, "SE_HOLD_GUN" },  // 0x002e7148
    { 1074203029, -1, 126, "SE_STANDBY_GUN" },  // 0x002e7158
    { 1074203030, -1, 127, "SE_HIT_GUN" },  // 0x002e7168
    { 1074203031, -1, 128, "SE_HIT_GUN_EMPTY" },  // 0x002e7178
    { 1074203032, -1, 129, "SE_RELOAD" },  // 0x002e7188
    { 1074203033, -1, 130, "SE_DROP_CARTRIDGE" },  // 0x002e7198
    { 1074203034, -1, 131, "SE_HIT_MEAT" },  // 0x002e71a8
    { 1074203035, -1, 132, "SE_HIT_WOOD" },  // 0x002e71b8
    { 1074203036, -1, 133, "SE_HIT_METAL" },  // 0x002e71c8
    { 1074203037, -1, 134, "SE_GUN_PC_ATTACK_1" },  // 0x002e71d8
    { 1074203038, -1, 135, "SE_GUN_PC_ATTACK_4" },  // 0x002e71e8
    { 1074270213, -1, 136, "SE_MEAT_HIT" },  // 0x002e71f8
    { 1074270214, -1, 137, "SE_MEAT_SWING_1" },  // 0x002e7208
    { 1074270215, -1, 138, "SE_MEAT_SWING_2" },  // 0x002e7218
    { 1074270216, -1, 139, "SE_MEAT_SWING_3" },  // 0x002e7228
    { 1074270217, -1, 140, "SE_MEAT_PC_ATTACK_1" },  // 0x002e7238
    { 1074270218, -1, 141, "SE_MEAT_PC_ATTACK_2" },  // 0x002e7248
    { 1074270219, -1, 142, "SE_MEAT_PC_ATTACK_3" },  // 0x002e7258
    { 1074270220, -1, 143, "SE_MEAT_PC_ATTACK_4" },  // 0x002e7268
    { 1074270221, -1, 144, "SE_MEAT_PC_ATTACK_5" },  // 0x002e7278
    { 167772306, -1, 145, "SE_WA_JFS_IN_METAL" },  // 0x002e7288
    { 167772307, -1, 146, "SE_WA_JFS_IN_METAL_2" },  // 0x002e7298
    { 167772308, -1, 147, "SE_WA_JFS_IN_METAL_N" },  // 0x002e72a8
    { 167772309, -1, 148, "SE_WA_JFS_IN_METAL_N_2" },  // 0x002e72b8
    { 167772316, -1, 149, "SE_WA_JFS_IN_METAL_S" },  // 0x002e72c8
    { 167772317, -1, 150, "SE_WA_JFS_IN_METAL_S_2" },  // 0x002e72d8
    { 167838098, -1, 151, "SE_WO_JFS_IN_WOOD_FLOOR" },  // 0x002e72e8
    { 167838099, -1, 152, "SE_WO_JFS_IN_WOOD_FLOOR_2" },  // 0x002e72f8
    { 167838100, -1, 153, "SE_WO_JFS_IN_WOOD_FLOOR_K" },  // 0x002e7308
    { 167838101, -1, 154, "SE_WO_JFS_IN_WOOD_FLOOR_K_2" },  // 0x002e7318
    { 167838102, -1, 155, "SE_WO_JFS_IN_CARPET" },  // 0x002e7328
    { 167838103, -1, 156, "SE_WO_JFS_IN_CARPET_2" },  // 0x002e7338
    { 167838104, -1, 157, "SE_WO_JFS_IN_UNDER_GROUND" },  // 0x002e7348
    { 167838105, -1, 158, "SE_WO_JFS_IN_UNDER_GROUND_2" },  // 0x002e7358
    { 167838106, -1, 159, "SE_WO_JFS_IN_LADDER" },  // 0x002e7368
    { 167838107, -1, 160, "SE_WO_JFS_IN_LADDER_2" },  // 0x002e7378
    { 167838108, -1, 161, "SE_WO_JFS_OUT_GRASS_GROUND" },  // 0x002e7388
    { 167838109, -1, 162, "SE_WO_JFS_OUT_GRASS_GROUND_2" },  // 0x002e7398
    { 167838110, -1, 163, "SE_WO_JFS_OUT_GRASS" },  // 0x002e73a8
    { 167838111, -1, 164, "SE_WO_JFS_OUT_GRASS_2" },  // 0x002e73b8
    { 167838112, -1, 165, "SE_WO_JFS_OUT_STONE" },  // 0x002e73c8
    { 167838113, -1, 166, "SE_WO_JFS_OUT_STONE_2" },  // 0x002e73d8
    { 167838114, -1, 167, "SE_WO_JFS_OUT_WATER" },  // 0x002e73e8
    { 167838115, -1, 168, "SE_WO_JFS_OUT_WATER_2" },  // 0x002e73f8
    { 167772326, -1, 169, "SE_WA_DOG_IN_METAL" },  // 0x002e7408
    { 167772327, -1, 170, "SE_WA_DOG_IN_METAL_2" },  // 0x002e7418
    { 167772328, -1, 171, "SE_WA_DOG_IN_METAL_N" },  // 0x002e7428
    { 167772329, -1, 172, "SE_WA_DOG_IN_METAL_N_2" },  // 0x002e7438
    { 167772334, -1, 173, "SE_WA_DOG_IN_METAL_S" },  // 0x002e7448
    { 167772335, -1, 174, "SE_WA_DOG_IN_METAL_S_2" },  // 0x002e7458
    { 167838116, -1, 175, "SE_WO_BFS_IN_WOOD_FLOOR" },  // 0x002e7468
    { 167838117, -1, 176, "SE_WO_BFS_IN_WOOD_FLOOR_2" },  // 0x002e7478
    { 167838118, -1, 177, "SE_WO_BFS_IN_WOOD_FLOOR_K" },  // 0x002e7488
    { 167838119, -1, 178, "SE_WO_BFS_IN_WOOD_FLOOR_K_2" },  // 0x002e7498
    { 167838120, -1, 179, "SE_WO_BFS_IN_CARPET" },  // 0x002e74a8
    { 167838121, -1, 180, "SE_WO_BFS_IN_CARPET_2" },  // 0x002e74b8
    { 167838122, -1, 181, "SE_WO_BFS_IN_UNDER_GROUND" },  // 0x002e74c8
    { 167838123, -1, 182, "SE_WO_BFS_IN_UNDER_GROUND_2" },  // 0x002e74d8
    { 167838124, -1, 183, "SE_WO_BFS_IN_LADDER" },  // 0x002e74e8
    { 167838125, -1, 184, "SE_WO_BFS_IN_LADDER_2" },  // 0x002e74f8
    { 167838126, -1, 185, "SE_WO_BFS_OUT_GRASS_GROUND" },  // 0x002e7508
    { 167838127, -1, 186, "SE_WO_BFS_OUT_GRASS_GROUND_2" },  // 0x002e7518
    { 167838128, -1, 187, "SE_WO_BFS_OUT_GRASS" },  // 0x002e7528
    { 167838129, -1, 188, "SE_WO_BFS_OUT_GRASS_2" },  // 0x002e7538
    { 167838130, -1, 189, "SE_WO_BFS_OUT_STONE" },  // 0x002e7548
    { 167838131, -1, 190, "SE_WO_BFS_OUT_STONE_2" },  // 0x002e7558
    { 167838132, -1, 191, "SE_WO_BFS_OUT_WATER" },  // 0x002e7568
    { 167838133, -1, 192, "SE_WO_BFS_OUT_WATER_2" },  // 0x002e7578
    { 537397386, -1, 193, "SE_DIAL" },  // 0x002e7588
    { 537397387, -1, 194, "SE_DIAL_OPEN" },  // 0x002e7598
    { 537397397, -1, 195, "SE_OPEN" },  // 0x002e75a8
    { 537397398, -1, 196, "SE_BOX_NOTOPEN" },  // 0x002e75b8
    { 536938158, -1, 197, "SE_EV2_CLOTH_S" },  // 0x002e75c8
    { 536938159, -1, 198, "SE_EV2_CLOTH_L" },  // 0x002e75d8
    { 537134982, -1, 199, "SE_EV5_ROPE_CREAK" },  // 0x002e75e8
};

/* ---- text ------------------------------------------------------------- */

/* The dog's oid, 0 without one (inline in the ROM). */
static inline u_int dogOid()
{
    return g_gamMan.m_dogPtr != NULL ? g_gamMan.m_dogPtr->m_oid : 0;
}

/* Wraps a heading once into -pi .. pi (inline in the ROM). */
static inline float wrapRotOnce(float r)
{
    if (r < -DOG_PI)
        return r + DOG_PI2;
    if (DOG_PI <= r)
        return r - DOG_PI2;
    return r;
}

/* ROR 0x001024d0 */
const char *utlGetSoundName(int no)
{
    int link = tbl_sound[no].m_link;
    if (link >= 0)
    {
        no = link;
        ROR_ASSERT(tbl_sound[no].m_tag == no); /* 325 */  // callcheck: dbgAssertFailed (the assert handler)
    }
    return tbl_sound[no].m_name;
}

/* ROR 0x0010254c */
void utlPlaySe3d(int id, const float *pos)
{
    /* The ROM has one call with pos and one with NULL. */
    g_gamMan.m_sndMan.playSE((u_int)id, true, (float *)pos, 0);
}

/* ROR 0x001025c0 */
u_int utlGetSoundSE(u_int theN)
{
    u_int oid = 0;
    if (theN < UTL_SOUND_MAX)
    {
        ROR_ASSERT(tbl_sound[theN].m_tag == (int)theN); /* 356 */  // callcheck: dbgAssertFailed (the assert handler)
        int no = tbl_sound[theN].m_link;
        if (no >= 0)
        {
            ROR_ASSERT(tbl_sound[no].m_tag == no); /* 379 */
            theN = (u_int)no;
        }
        oid = tbl_sound[theN].m_se;
        ROR_ASSERT(oid != 0); /* 388 */
    }
    return oid;
}

/* ROR 0x001026b0 */
void utlPlaySe(int id, int delay, const float *pos)
{
    u_int se = utlGetSoundSE((u_int)id);
    if (se == 0)
        return;
    if (pos != NULL)
        g_gamMan.m_sndMan.playSE(se, true, (float *)pos, 0);
    else if (delay > 0)
        scnPlaySEDelayed(se, delay);
    else
        g_gamMan.m_sndMan.playSE(se, true, NULL, 0);
}

/* ROR 0x00102758 */
void utlPlaySeIfIdle(int no)
{
    u_int se = utlGetSoundSE((u_int)no);
    if (se != 0 && g_gamMan.m_sndMan.funcSE(se, 0x16, NULL) == 0)
        g_gamMan.m_sndMan.playSE(se, true, NULL, 0);
}

/* ROR 0x001027d4 */
void utlStopSe(int no)
{
    u_int se = utlGetSoundSE((u_int)no);
    if (se != 0 && g_gamMan.m_sndMan.funcSE(se, 0x16, NULL) != 0)
        g_gamMan.m_sndMan.funcSE(se, 1, NULL);
}

/* ROR 0x0010284c */
int utlPlaySeChance(int no, int chance, int range)
{
    int r = utlRandRange(0, range);
    if (r < chance)
        utlPlaySe(no, 0, NULL);
    return r < chance;
}

/* ROR 0x001028a8 */
void utlPlaySeRand2(int a, int b)
{
    if (utlRandRange(0, 200) < 100)
        utlPlaySe(a, 0, NULL);
    else
        utlPlaySe(b, 0, NULL);
}

/* ROR 0x00102918 */
void utlPlaySeRand3(int a, int b, int c)
{
    int r = utlRandRange(0, 300);
    if (r < 100)
        utlPlaySe(a, 0, NULL);
    else if (r < 200)
        utlPlaySe(b, 0, NULL);
    else
        utlPlaySe(c, 0, NULL);
}

/* ROR 0x001029c4 */
void utlPlaySeRand4(int a, int b, int c, int d)
{
    int r = utlRandRange(0, 400);
    if (r < 100)
        utlPlaySe(a, 0, NULL);
    else if (r < 200)
        utlPlaySe(b, 0, NULL);
    else if (r < 300)
        utlPlaySe(c, 0, NULL);
    else
        utlPlaySe(d, 0, NULL);
}

/* ROR 0x00102ab8 */
void utlGetObjPos(float *out, u_int oid)
{
    g_gamMan.getObjPos(oid, out);
}

/* ROR 0x00102adc */
int utlIsEmyDamaging(u_int theOID)
{
    ROR_ASSERT(theOID != 0); /* 575 */  // callcheck: dbgAssertFailed (the assert handler)
    return g_emyManager.isEmyDamaging(theOID);
}

/* ROR 0x00102b24 */
int utlTestEmyStateFlag(u_int theOID)
{
    ROR_ASSERT(theOID != 0); /* 584 */  // callcheck: dbgAssertFailed (the assert handler)
    return g_emyManager.testEmyStateFlag(theOID);
}

/* ROR 0x00102b6c */
unsigned long long utlFindItemByUids(float *out, const int *uids, int n)
{
    for (int i = 0; i < n; i++)
    {
        unsigned long long found = utlFindFreeObjByUid(out, (u_int)uids[i]);
        if (found != 0)
            return found;
        found = utlFindItemByUid(out, (u_int)uids[i]);
        if (found != 0)
            return found;
    }
    return 0;
}

/* ROR 0x00102bf0 */
unsigned long long utlFindItemByUid(float *out, u_int uid)
{
    float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    unsigned long long found = 0;
    bool any = false;
    for (SeqNode<FCHRstat *> *node = g_gamMan.m_avfchrList.m_used.begin();
         node != g_gamMan.m_avfchrList.m_used.end(); node = node->m_next)
    {
        FCHRstat *obj = node->m_data;
        if (uid != (u_short)obj->m_kind)
            continue;
        if (!utlIsItemFlag2((short)uid))
        {
            obj->getPos(out);
            return (unsigned long long)uid << 32 | obj->m_oid;
        }
        /* Flagged: keep looking, the last one wins. */
        obj->getPos(pos);
        any = true;
        found = (unsigned long long)uid << 32 | obj->m_oid;
    }
    if (!any)
        return 0;
    out[0] = pos[0];
    out[1] = pos[1];
    out[2] = pos[2];
    out[3] = pos[3];
    return found;
}

/* ROR 0x00102d54 */
FCHRstat *utlFindVFchrByUid(u_int uid)
{
    for (SeqNode<FCHRstat *> *node = g_gamMan.m_vfchrList.m_used.begin();
         node != g_gamMan.m_vfchrList.m_used.end(); node = node->m_next)
    {
        if (uid == (u_short)node->m_data->m_kind)
            return node->m_data;
    }
    return NULL;
}

/* ROR 0x00102dc0 */
int utlFindDoorBetween(UtlDoorInfo *out, int a, int b, const float *pos)
{
    float best = 1e6f;
    for (SeqNode<FCHRstat *> *node = g_gamMan.m_fchrList.m_used.begin();
         node != g_gamMan.m_fchrList.m_used.end(); node = node->m_next)
    {
        FCHRstat *obj = node->m_data;
        if (gamOidType(obj->m_oid) != 9)
            continue;
        int param[4];
        obj->getParam(0x41, param);
        int room[2];
        g_gamMan.ctlObjParam((u_int)param[0], 0xc, room, 0);
        if (!((a == room[0] && b == room[1]) || (a == room[1] && b == room[0])))
            continue;
        float at[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        obj->getPos(at);
        float d = utlVecDist(at, pos);
        if (!(d < best))
            continue;
        out->m_pos[0] = at[0];
        out->m_pos[1] = at[1];
        out->m_pos[2] = at[2];
        out->m_pos[3] = at[3];
        out->m_rot_y = wrapRotOnce(obj->m_rot[1]);
        out->m_room[0] = room[0];
        out->m_room[1] = room[1];
        out->m_obj = obj;
        best = d;
        /* ctlObjParam 2 tests the object record's flags (data 0x2e5884..:
         * the words 8, 0x10, 0x20). */
        u_int bit = 8;
        if (g_gamMan.ctlObjParam((u_int)param[0], 2, &bit, 0) != 0)
        {
            out->m_kind = 1;
            continue;
        }
        bit = 0x10;
        if (g_gamMan.ctlObjParam((u_int)param[0], 2, &bit, 0) != 0)
        {
            out->m_kind = 2;
            continue;
        }
        bit = 0x20;
        if (g_gamMan.ctlObjParam((u_int)param[0], 2, &bit, 0) == 0)
        {
            out->m_kind = 0;
            continue;
        }
        out->m_kind = 3;
        out->m_rot_y += DOG_PI;
        while (out->m_rot_y < -DOG_PI)
            out->m_rot_y += DOG_PI2;
        while (DOG_PI <= out->m_rot_y)
            out->m_rot_y -= DOG_PI2;
    }
    return best < 1e6f;
}

/* ROR 0x0010315c */
unsigned long long utlFindFreeObjByUid(float *out, u_int uid)
{
    for (SeqNode<FCHRstat *> *node = g_gamMan.m_fchrList.m_used.begin();
         node != g_gamMan.m_fchrList.m_used.end(); node = node->m_next)
    {
        FCHRstat *obj = node->m_data;
        if (uid == (u_short)obj->m_kind && !utlIsItemFlag2((short)uid))
        {
            obj->getPos(out);
            return (unsigned long long)uid << 32 | obj->m_oid;
        }
    }
    return 0;
}

/* ROR 0x00103240 */
int utlGetItemUids(int *out, int theMax, u_int item)
{
    int id = (int)(item & 0xffff);
    short field[8];
    ROR_ASSERT(theMax >= 4); /* 797 */  // callcheck: dbgAssertFailed (the assert handler)
    g_gamMan.m_sysData.getField(id, 0x10, field);
    out[0] = field[0];
    g_gamMan.m_sysData.getField(id, 0x11, field);
    out[1] = field[0];
    g_gamMan.m_sysData.getField(id, 0x12, field);
    out[2] = field[0];
    if ((int)item >> 9 == 6)
    {
        out[3] = -1;
        return 3;
    }
    out[3] = (int)item;
    return 4;
}

/* ROR 0x0010332c */
void utlTakeObjItem(u_int oid)
{
    int param[4];
    param[0] = 0;
    g_gamMan.getObjParam(oid, 2, param);
    g_gamMan.addItem((short)param[0], 0);
    g_gamMan.releaseChr(oid);
}

/* ROR 0x00103390 */
int utlGetOidTypeKind(u_int oid)
{
    u_int type = gamOidType(oid);
    if (type == 5)
        return 1;
    return type == 6 ? 2 : 0;
}

/* ROR 0x001033b8 */
void utlSetItemFlag2(short id)
{
    g_gamMan.m_sysData.setFlags(id, 2);
}

/* ROR 0x001033f0 */
int utlIsItemFlag2(short id)
{
    return g_gamMan.m_sysData.testFlags(id, 2);
}

/* ROR 0x00103428 */
void utlMarkObjTaken(u_int oid)
{
    g_gamMan.markObjTaken(oid);
}

/* ROR 0x0010344c */
int utlIsObj(u_int oid)
{
    return g_gamMan.isObj(oid);
}

/* ROR 0x0010346c */
int utlTestSeqBit34(void)
{
    return (g_sequence.m_bits[1] & 4) != 0;
}

/* ROR 0x00103484 */
int utlIsVibrationOn(void)
{
    int status[4];
    status[0] = 0;
    g_gamMan.getStatus(0x12, status);
    return status[0] != 0;
}

/* ROR 0x001034b8 */
int utlGetWrdNo(void)
{
    int status[4];
    g_gamMan.getStatus(5, status);
    return status[0];
}

/* ROR 0x001034e4 */
int utlGetActorNo(u_int oid)
{
    return (short)g_gamMan.getActorNo(oid);
}

/* ROR 0x00103510 */
float utlGetEmySpinePos(float *out, u_int oid, int kind)
{
    int no = (short)g_gamMan.getActorNo(oid);
    if ((u_int)no >= 0x80)
        return 0.0f;
    DogActor &actor = g_magSystem.m_scene.m_actors[no];
    int n = actor.getNodeIndexByName("Bip01 Spine1");   // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)
    ROR_ASSERT(n >= 0); /* 931 */
    if (n >= 0)     /* PC: the ROM's assert never returns */
        actor.getNodeWorldPos(out, n);
    if (kind != 0)
        return g_emyManager.getEmyTypeDist(oid);
    return g_emyManager.getEmyTypeDist2(oid);
}

/* ROR 0x00103654 */
int utlFindNearestEmyInRange(float theR, const UtlEnemyInfoBuf &theInfo)
{
    ROR_ASSERT(-DOG_PI <= theR && theR < DOG_PI); /* 967 */  // callcheck: dbgAssertFailed (the assert handler)
    ROR_ASSERT(0 <= theInfo.m_count && theInfo.m_count <= UtlEnemyInfoBuf::MAX_INFO); /* 968 */
    int count = theInfo.m_count;
    if (count <= 0)
        return -1;
    if (count == 1)
        return 0;
    float best = 1e10f;
    int found = -1;
    for (int i = 0; i < count; i++)
    {
        const UtlEnemyInfo &e = theInfo.m_info[i];
        if (-theR <= e.m_rot && e.m_rot <= theR && e.m_dist < best)
        {
            best = e.m_dist;
            found = i;
        }
    }
    return found;
}

/* Fills e from enemy object obj: its flags, oid, uid and actor number,
 * and with an actor its m_trans and spine node position. Returns the
 * actor, NULL when it has none (the rest of e stays as it was). Inline in
 * utlCollectEmyInfo / utlCollectEmyInfoAt in the ROM. */
static DogActor *fillEmyInfo(UtlEnemyInfo &e, ObjStat *obj)
{
    e.m_flags = obj->m_flags;
    e.m_obj_id = obj->m_oid;
    e.m_24 = (u_short)obj->m_kind;
    e.m_actor_no = obj->m_actor;
    if ((u_int)e.m_actor_no >= 0x80)
        return NULL;
    DogActor &actor = g_magSystem.m_scene.m_actors[e.m_actor_no];
    for (int i = 0; i < 4; i++)
        e.m_trans[i] = actor.m_trans[i];
    int n = actor.getNodeIndexByName("Bip01 Spine1");
    if (n < 0)
    {
        for (int i = 0; i < 4; i++)
            e.m_pos[i] = actor.m_trans[i];
    }
    else
    {
        actor.getNodeWorldPos(e.m_pos, n);
    }
    return &actor;
}

/* ROR 0x0010376c */
int utlCollectEmyInfoAt(UtlEnemyInfoBuf *eib, const float *pos, int kind, int flag,
                        float rot, float radius)
{
    int count = 0;
    eib->m_count = 0;
    for (SeqNode<ACHRstat *> *node = g_gamMan.m_achrList.m_used.begin();
         node != g_gamMan.m_achrList.m_used.end(); node = node->m_next)
    {
        ACHRstat *obj = node->m_data;
        u_int oid = obj->m_oid;
        if (gamOidType(oid) != 2)
            continue;
        if ((obj->m_flags & (flag == 0 ? 0x400 : 0x2000)) != 0)
            continue;
        UtlEnemyInfo &e = eib->m_info[count];
        if (fillEmyInfo(e, obj) != NULL)    // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)
        {
            if (kind == 0)
                e.m_radius = 0.0f;
            else if (kind == 1)
                e.m_radius = g_emyManager.getEmyTypeDist(oid);
            else
                e.m_radius = g_emyManager.getEmyTypeDist2(oid);
        }
        e.m_dist = utlVecDistRotY(pos, &e.m_rot, e.m_pos);
        e.m_rot = wrapRotOnce(e.m_rot - rot);
        if (0.0f < radius && e.m_dist <= radius)
            count++;
        if (count >= UtlEnemyInfoBuf::MAX_INFO)
            break;
    }
    eib->m_count = count;
    return count;
}

/* ROR 0x00103a90 */
int utlCollectEmyInfo(UtlEnemyInfoBuf *eib, int flag)
{
    int count = 0;
    eib->m_count = 0;
    for (SeqNode<ACHRstat *> *node = g_gamMan.m_achrList.m_used.begin();
         node != g_gamMan.m_achrList.m_used.end(); node = node->m_next)
    {
        ACHRstat *obj = node->m_data;
        u_int oid = obj->m_oid;
        if (gamOidType(oid) != 2)
            continue;
        if ((obj->m_flags & (flag == 0 ? 0x400 : 0x2000)) != 0)
            continue;
        UtlEnemyInfo &e = eib->m_info[count];
        if (fillEmyInfo(e, obj) != NULL)    // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)
            e.m_radius = g_emyManager.getEmyTypeDist(oid);
        count++;
        if (count >= UtlEnemyInfoBuf::MAX_INFO)
            break;
    }
    eib->m_count = count;
    return count;
}

/* ROR 0x00103ccc */
int utlCollectItemInfo(UtlEnemyInfoBuf *eib, int flag)
{
    int count = 0;
    for (SeqNode<FCHRstat *> *node = g_gamMan.m_avfchrList.m_used.begin();
         node != g_gamMan.m_avfchrList.m_used.end(); node = node->m_next)
    {
        FCHRstat *obj = node->m_data;
        if (flag != 0 && (obj->m_flags & 0x20) != 0)
            continue;
        UtlEnemyInfo &e = eib->m_info[count];
        count++;
        e.m_24 = (u_short)obj->m_kind;
        e.m_obj_id = obj->m_oid;
        obj->getPos(e.m_pos);
        if (count >= UtlEnemyInfoBuf::MAX_INFO)
            return count;
    }
    return count;
}

/* ROR 0x00103de4 */
int utlPauseForMenu(void)
{
    int open = g_gamMan.isMenuOpen();
    if (open == 0)
    {
        g_gamMan.pause(0x100000);
        g_gamMan.m_flags |= 0x400;
    }
    return open == 0;
}

/* ROR 0x00103e4c */
int utlGetPcEquipItemChrID(void)
{
    int id = g_gamMan.m_pcEquipItemChrID;
    ROR_ASSERT(id < 128); /* 1241 */  // callcheck: dbgAssertFailed (the assert handler)
    return id;
}

/* ROR 0x00103e94 */
int utlGetPcChrNo(void)
{
    int id = g_gamMan.m_pcActor;
    ROR_ASSERT(id < 128); /* 1251 */  // callcheck: dbgAssertFailed (the assert handler)
    return id;
}

/* ROR 0x00103edc */
void utlSetDogParam1b(int v)
{
    int param[4];
    param[0] = v;
    g_gamMan.setDogParam(0x1b, param);
}

/* ROR 0x00103f08 */
int utlGetDogParam1b(void)
{
    int param[4];
    param[0] = 0;
    g_gamMan.getDogParam(0x1b, param);
    return param[0];
}

/* ROR 0x00103f38 */
int utlGetDogParam1c(void)
{
    int param[4];
    param[0] = 0;
    g_gamMan.getDogParam(0x1c, param);
    return param[0];
}

/* ROR 0x00103f68 */
int utlGetEmyHold(u_int oid)
{
    return g_emyManager.getEmyHoldWord248(oid);
}

/* ROR 0x00103f88 */
int utlGetEmyWord238(u_int oid)
{
    return g_emyManager.getEmyWord238(oid);
}

/* ROR 0x00103fac */
int utlIsEmyState7(u_int oid)
{
    return g_emyManager.getEmyWord3c(oid) == 7;
}

/* ROR 0x00103fd8 */
int utlIsEmyState4(u_int oid)
{
    return g_emyManager.getEmyWord3c(oid) == 4;
}

/* ROR 0x00104004 */
int utlTestDogHitAt(const float *pos, u_int *hit, float r)
{
    float at[4] = { pos[0], pos[1], pos[2], pos[3] };
    u_int oid = dogOid();
    u_int result[8];
    g_gamMan.setObjRadius(oid, r);
    int ret = g_gamMan.testHitAt(at, oid, 0, 1, result);
    if (hit != NULL)
        *hit = result[4];
    return ret;
}

/* ROR 0x001040a0 */
float utlGetDogRadius(void)
{
    return g_gamMan.getObjRadius(dogOid(), NULL);
}

/* ROR 0x001040dc */
void utlSetDogRadius(float r)
{
    g_gamMan.setObjRadius(dogOid(), r);
}

/* ROR 0x0010411c */
int utlGetObjKind(u_int oid)
{
    int param[4];
    param[0] = 0;
    g_gamMan.getObjParam(oid, 2, param);
    return param[0];
}

/* ROR 0x00104150 */
void utlSetBwnObjFlag(int on)
{
    /* Both branches of the ROM set the flag. */
    (void)on;
    g_gamMan.setObj(dogOid(), 0x40);
}
