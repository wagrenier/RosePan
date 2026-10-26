/* ==========================================================================
 *  spr/s2d.cpp
 *
 *  The scene's 2D layers (0x190a78..0x192ac0; RoR names no file here: spr/
 *  from the link order after spr/sprtexture.cpp and the SprMan they are
 *  built on). S2dCineSco (the cinemascope bars), S2dBg (full-screen
 *  pictures), S2dMes (message icons, map-name caption) and S2dMovie
 *  (loading.tm2 and the movie layers); see spr/s2d.h.
 *
 *  PORT: the pattern tables are the ROM's .data words as they are (an EE
 *  image): the SprDispDef records and the tables themselves point into
 *  them by EE address (the _EE constants give each table's base, which
 *  goes to SprMan::m_pattern_ee next to m_pattern: SprMan::eePtr resolves
 *  them). Writable: SprMan writes the anim counts back.
 * ======================================================================== */
#include "spr/s2d.h"
#include "common/rosepan_stub.h"

#include <string.h>

#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "sdk/libgraph.h"

/* ---- data ---- */

/* S2dBg's picture slots: setTexName copies a file name in (data 0x29a180,
 * 7 x 0x40). */
static char s2dBgTexName[7][0x40];

/* S2dBg's texture list: seven pictures named by setTexName, ids -1 until
 * then (data 0x299c60). */
static SprTexFile s2dBgTexList[8] = {
    { -1, s2dBgTexName[0], "sgbg00_tcc", 0, 1, NULL },  // 0x00299c60
    { -1, s2dBgTexName[1], "sgbg01_tcc", 0, 1, NULL },  // 0x00299c78
    { -1, s2dBgTexName[2], "sgbg02_tcc", 0, 1, NULL },  // 0x00299c90
    { -1, s2dBgTexName[3], "sgbg03_tcc", 0, 1, NULL },  // 0x00299ca8
    { -1, s2dBgTexName[4], "sgbg04_tcc", 0, 1, NULL },  // 0x00299cc0
    { -1, s2dBgTexName[5], "sgbg05_tcc", 0, 1, NULL },  // 0x00299cd8
    { -1, s2dBgTexName[6], "sgbg06_tcc", 0, 1, NULL },  // 0x00299cf0
    { -1, "", "", 0, 0, NULL },                         // 0x00299d08
};

/* S2dMes's textures in data/mnurpk/s2dman.rpk (data 0x29a340). */
static SprTexFile s2dMesTexList[3] = {
    { 1, "wait.tm2", "smic01_tcc", 0, 1, NULL },        // 0x0029a340
    { 2, "wait2.tm2", "smic02_tcc", 0, 1, NULL },       // 0x0029a358
    { -1, "", "", 0, 0, NULL },                         // 0x0029a370
};

/* S2dMovie's texture in data/mnurpk/s2dman.rpk (data 0x29bcf8). */
static SprTexFile s2dMovieTexList[2] = {
    { 0, "loading.tm2", "sm00_tcc", 0, 1, NULL },       // 0x0029bcf8
    { -1, "", "", 0, 0, NULL },                         // 0x0029bd10
};

static const char s2dGenRpk[] = "data/mnurpk/s2dgen.rpk";   /* rodata 0x2f7b90 */
static const char s2dManRpk[] = "data/mnurpk/s2dman.rpk";   /* rodata 0x2f7c48 (0x2f7cd8 the same text) */

/* The S2dMes item -> sprite map (data 0x29bb70). */
static const int s2dMesSprNo[S2D_MES_ITEM_MAX] = { 3, 4, 5, 1, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

/* backupSprite's copies (data 0x29bba8 .. 0x29bcc0, 14 words each). */
static int s2dMesBackupProcess[S2D_MES_ITEM_MAX];   /* data 0x29bba8: flags & 2 */
static int s2dMesBackupStdX[S2D_MES_ITEM_MAX];      /* data 0x29bbe0 */
static int s2dMesBackupStdY[S2D_MES_ITEM_MAX];      /* data 0x29bc18 (the ROM saves std x here too) */
static int s2dMesBackupOffX[S2D_MES_ITEM_MAX];      /* data 0x29bc50 */
static int s2dMesBackupOffY[S2D_MES_ITEM_MAX];      /* data 0x29bc88 */
static int s2dMesBackupState[S2D_MES_ITEM_MAX];     /* data 0x29bcc0 */

/* S2dCineSco's pattern table (data 0x2999d8..0x299b70): EE image, see the file comment. */
static u_int s2dCineScoPattern[102] = {
    0xffffff9e, 0x00000000, 0x00000000, 0x00000100, 0x00000100, 0xffffff9e, 0x00000000, 0x00000000,  // 0x002999d8
    0x00002000, 0x00000400, 0xffffff9e, 0x00000000, 0x00000000, 0x00002000, 0x00000300, 0x00000000,  // 0x002999f8
    0x00000005, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01000000,  // 0x00299a18
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0x00000000,  // 0x00299a38
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0x00000000, 0x00000000,  // 0x00299a58
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x03000000, 0x00000000, 0x00000000, 0x00000000,  // 0x00299a78
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000006, 0x00000001, 0x00000000, 0x00000000,  // 0x00299a98
    0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x00299ab8
    0x00000000, 0x00000000, 0x00000006, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0,  // 0x00299ad8
    0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x00299af8
    0x00000000, 0xffffffff, 0x00299a18, 0x00000000, 0xffffffff, 0x00299a50, 0xffffffff, 0xffffffff,  // 0x00299b18
    0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00299aa8, 0xffffffff, 0xffffffff, 0x00000000,  // 0x00299b38
    0x00000000, 0xffffffff, 0x00299ae0, 0xffffffff, 0xffffffff, 0x00000000,  // 0x00299b58
};
#define S2DCINESCOPATTERN_EE 0x002999d8u

static const int s2dCineScoBlackClear[4] = { 0, 0, 0, 0 };  /* black, alpha 0, data 0x299b70 */
static const int s2dCineScoBlackOpaque[4] = { 0, 0, 0, 128 };  /* black, alpha 0x80, data 0x299b80 */

/* S2dCineSco's display list: sprite 0 and the two bars (data 0x299b90). */
static const SprDispDef s2dCineScoDisp[4] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 480000, 0x00299b18u },  // 0x00299b90
    { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 480000, 0x00299b40u },  // 0x00299bc4
    { 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 6400, 480000, 0x00299b58u },  // 0x00299bf8
    { -1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0x00299b18u },  // 0x00299c2c
};

/* S2dBg's pattern table (data 0x299d20..0x299fe0): EE image, see the file comment. */
static u_int s2dBgPattern[176] = {
    0x00000000, 0x00000000, 0x00000000, 0x00002000, 0x00001c00, 0x00000001, 0x00000000, 0x00000000,  // 0x00299d20
    0x00002000, 0x00001c00, 0x00000002, 0x00000000, 0x00000000, 0x00002000, 0x00001c00, 0x00000003,  // 0x00299d40
    0x00000000, 0x00000000, 0x00002000, 0x00001c00, 0x00000004, 0x00000000, 0x00000000, 0x00002000,  // 0x00299d60
    0x00001c00, 0x00000005, 0x00000000, 0x00000000, 0x00002000, 0x00001c00, 0x00000006, 0x00000000,  // 0x00299d80
    0x00000000, 0x00002000, 0x00001c00, 0x00000000, 0x00000006, 0x00000000, 0x00000000, 0x00000000,  // 0x00299da0
    0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x00299dc0
    0x00000000, 0x00000000, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0,  // 0x00299de0
    0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x00299e00
    0x00000006, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000,  // 0x00299e20
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000006, 0x00000003,  // 0x00299e40
    0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000,  // 0x00299e60
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000006, 0x00000004, 0x00000000, 0x00000000,  // 0x00299e80
    0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x00299ea0
    0x00000000, 0x00000000, 0x00000006, 0x00000005, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0,  // 0x00299ec0
    0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x00299ee0
    0x00000006, 0x00000006, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000,  // 0x00299f00
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff,  // 0x00299f20
    0x00299db0, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0x00299de8, 0xffffffff,  // 0x00299f40
    0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0x00299e20, 0xffffffff, 0xffffffff, 0x00000000,  // 0x00299f60
    0x00000000, 0xffffffff, 0x00299e58, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff,  // 0x00299f80
    0x00299e90, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0x00299ec8, 0xffffffff,  // 0x00299fa0
    0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0x00299f00, 0xffffffff, 0xffffffff, 0x00000000,  // 0x00299fc0
};
#define S2DBGPATTERN_EE 0x00299d20u

/* S2dBg's display list: the seven pictures (data 0x299fe0). */
static const SprDispDef s2dBgDisp[8] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32000, 0x00299f38u },  // 0x00299fe0
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32000, 0x00299f50u },  // 0x0029a014
    { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32000, 0x00299f68u },  // 0x0029a048
    { 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32000, 0x00299f80u },  // 0x0029a07c
    { 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32000, 0x00299f98u },  // 0x0029a0b0
    { 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32000, 0x00299fb0u },  // 0x0029a0e4
    { 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32000, 0x00299fc8u },  // 0x0029a118
    { -1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0x00000000u },  // 0x0029a14c
};

/* S2dMes's pattern table (data 0x29a388..0x29b7f8): EE image, see the file comment. */
static u_int s2dMesPattern[1308] = {
    0xffffffff, 0x00000000, 0x00000000, 0x00000100, 0x00000100, 0x00000001, 0x00000000, 0x00000000,  // 0x0029a388
    0x00000200, 0x00000200, 0x00000001, 0x00000200, 0x00000000, 0x00000200, 0x00000200, 0x00000001,  // 0x0029a3a8
    0x00000400, 0x00000000, 0x00000200, 0x00000200, 0x00000001, 0x00000600, 0x00000000, 0x00000200,  // 0x0029a3c8
    0x00000200, 0x00000001, 0x00000000, 0x00000200, 0x00000200, 0x00000200, 0x00000001, 0x00000200,  // 0x0029a3e8
    0x00000200, 0x00000200, 0x00000200, 0x00000001, 0x00000400, 0x00000200, 0x00000200, 0x00000200,  // 0x0029a408
    0x00000001, 0x00000600, 0x00000200, 0x00000200, 0x00000200, 0x00000001, 0x00000000, 0x00000000,  // 0x0029a428
    0x00000200, 0x00000200, 0x00000001, 0x00000200, 0x00000000, 0x00000200, 0x00000200, 0x00000001,  // 0x0029a448
    0x00000400, 0x00000000, 0x00000200, 0x00000200, 0x00000001, 0x00000600, 0x00000000, 0x00000200,  // 0x0029a468
    0x00000200, 0x00000001, 0x00000000, 0x00000200, 0x00000200, 0x00000200, 0x00000001, 0x00000200,  // 0x0029a488
    0x00000200, 0x00000200, 0x00000200, 0x00000001, 0x00000400, 0x00000200, 0x00000200, 0x00000200,  // 0x0029a4a8
    0x00000001, 0x00000600, 0x00000200, 0x00000200, 0x00000200, 0x00000001, 0x00000000, 0x00000000,  // 0x0029a4c8
    0x00000200, 0x00000200, 0x00000001, 0x00000200, 0x00000000, 0x00000200, 0x00000200, 0x00000001,  // 0x0029a4e8
    0x00000400, 0x00000000, 0x00000200, 0x00000200, 0x00000001, 0x00000600, 0x00000000, 0x00000200,  // 0x0029a508
    0x00000200, 0x00000001, 0x00000000, 0x00000200, 0x00000200, 0x00000200, 0x00000001, 0x00000200,  // 0x0029a528
    0x00000200, 0x00000200, 0x00000200, 0x00000001, 0x00000400, 0x00000200, 0x00000200, 0x00000200,  // 0x0029a548
    0x00000001, 0x00000600, 0x00000200, 0x00000200, 0x00000200, 0x00000001, 0x00000c80, 0x00000000,  // 0x0029a568
    0x00001380, 0x00001000, 0x00000001, 0x00000c80, 0x00001000, 0x00001380, 0x00001000, 0x00000001,  // 0x0029a588
    0x00000c80, 0x00000000, 0x00000c80, 0x00000a00, 0x00000001, 0x00000000, 0x00001e00, 0x00000c80,  // 0x0029a5a8
    0x00000200, 0x00000001, 0x00000000, 0x00000000, 0x00000200, 0x00000200, 0x00000001, 0x00000200,  // 0x0029a5c8
    0x00000000, 0x00000200, 0x00000200, 0x00000001, 0x00000400, 0x00000000, 0x00000200, 0x00000200,  // 0x0029a5e8
    0x00000001, 0x00000600, 0x00000000, 0x00000200, 0x00000200, 0x00000001, 0x00000000, 0x00000200,  // 0x0029a608
    0x00000200, 0x00000200, 0x00000001, 0x00000200, 0x00000200, 0x00000200, 0x00000200, 0x00000001,  // 0x0029a628
    0x00000400, 0x00000200, 0x00000200, 0x00000200, 0x00000001, 0x00000600, 0x00000200, 0x00000200,  // 0x0029a648
    0x00000200, 0x00000001, 0x00000000, 0x00000400, 0x00000200, 0x00000200, 0x00000001, 0x00000200,  // 0x0029a668
    0x00000400, 0x00000200, 0x00000200, 0x00000001, 0x00000400, 0x00000400, 0x00000200, 0x00000200,  // 0x0029a688
    0x00000001, 0x00000600, 0x00000400, 0x00000200, 0x00000200, 0x00000001, 0x00000000, 0x00000600,  // 0x0029a6a8
    0x00000200, 0x00000200, 0x00000001, 0x00000200, 0x00000600, 0x00000200, 0x00000200, 0x00000001,  // 0x0029a6c8
    0x00000400, 0x00000600, 0x00000200, 0x00000200, 0x00000001, 0x00000600, 0x00000600, 0x00000200,  // 0x0029a6e8
    0x00000200, 0x00000001, 0x00000000, 0x00000800, 0x00000200, 0x00000200, 0x00000001, 0x00000200,  // 0x0029a708
    0x00000800, 0x00000200, 0x00000200, 0x00000001, 0x00000400, 0x00000800, 0x00000200, 0x00000200,  // 0x0029a728
    0x00000001, 0x00000600, 0x00000800, 0x00000200, 0x00000200, 0x00000001, 0x00000000, 0x00000a00,  // 0x0029a748
    0x00000200, 0x00000200, 0x00000001, 0x00000200, 0x00000a00, 0x00000200, 0x00000200, 0x00000001,  // 0x0029a768
    0x00000400, 0x00000a00, 0x00000200, 0x00000200, 0x00000001, 0x00000600, 0x00000a00, 0x00000200,  // 0x0029a788
    0x00000200, 0x00000001, 0x00000000, 0x00000c00, 0x00000200, 0x00000200, 0x00000001, 0x00000200,  // 0x0029a7a8
    0x00000c00, 0x00000200, 0x00000200, 0x00000001, 0x00000400, 0x00000c00, 0x00000200, 0x00000200,  // 0x0029a7c8
    0x00000001, 0x00000600, 0x00000c00, 0x00000200, 0x00000200, 0x00000001, 0x00000000, 0x00000e00,  // 0x0029a7e8
    0x00000200, 0x00000200, 0x00000001, 0x00000200, 0x00000e00, 0x00000200, 0x00000200, 0x00000001,  // 0x0029a808
    0x00000400, 0x00000e00, 0x00000200, 0x00000200, 0x00000001, 0x00000600, 0x00000e00, 0x00000200,  // 0x0029a828
    0x00000200, 0x00000001, 0x00000000, 0x00001000, 0x00000200, 0x00000200, 0x00000001, 0x00000200,  // 0x0029a848
    0x00001000, 0x00000200, 0x00000200, 0x00000001, 0x00000400, 0x00001000, 0x00000200, 0x00000200,  // 0x0029a868
    0x00000001, 0x00000600, 0x00001000, 0x00000200, 0x00000200, 0x00000001, 0x00000000, 0x00001220,  // 0x0029a888
    0x00000200, 0x00000200, 0x00000001, 0x00000200, 0x00001220, 0x00000200, 0x00000200, 0x00000001,  // 0x0029a8a8
    0x00000400, 0x00001220, 0x00000200, 0x00000200, 0x00000001, 0x00000600, 0x00001220, 0x00000200,  // 0x0029a8c8
    0x00000200, 0x00000002, 0x00000180, 0x00000000, 0x00000180, 0x00000180, 0x00000002, 0x00000480,  // 0x0029a8e8
    0x00000000, 0x00000180, 0x00000180, 0x00000002, 0x00000300, 0x00000000, 0x00000180, 0x00000180,  // 0x0029a908
    0x00000002, 0x00000000, 0x00000000, 0x00000180, 0x00000180, 0x00000002, 0x000002a0, 0x00000180,  // 0x0029a928
    0x00000200, 0x00000180, 0x00000002, 0x00000000, 0x00000180, 0x00000200, 0x00000180, 0x00000002,  // 0x0029a948
    0x00000000, 0x00000310, 0x000001e0, 0x00000180, 0x00000002, 0x000003b0, 0x00000310, 0x000001e0,  // 0x0029a968
    0x00000180, 0x00000002, 0x000001d0, 0x00000310, 0x000001e0, 0x00000180, 0x00000002, 0x00000560,  // 0x0029a988
    0x00000310, 0x000001e0, 0x00000180, 0x00000000, 0x00000005, 0xffffffff, 0x00000000, 0x00000000,  // 0x0029a9a8
    0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029a9c8
    0x00000000, 0x00000000, 0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029a9e8
    0x00000000, 0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029aa08
    0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029aa28
    0x00000003, 0x00000001, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003,  // 0x0029aa48
    0x00000002, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000003,  // 0x0029aa68
    0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000004, 0xffffffd0,  // 0x0029aa88
    0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000005, 0xffffffd0, 0xffffff80,  // 0x0029aaa8
    0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000006, 0xffffffd0, 0xffffff80, 0x00000000,  // 0x0029aac8
    0xfffffff0, 0xfffffff0, 0x00000003, 0x00000007, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0,  // 0x0029aae8
    0xfffffff0, 0x00000003, 0x00000008, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0,  // 0x0029ab08
    0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029ab28
    0x00000003, 0x00000009, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003,  // 0x0029ab48
    0x0000000a, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x0000000b,  // 0x0029ab68
    0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x0000000c, 0xffffffd0,  // 0x0029ab88
    0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x0000000d, 0xffffffd0, 0xffffff80,  // 0x0029aba8
    0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x0000000e, 0xffffffd0, 0xffffff80, 0x00000000,  // 0x0029abc8
    0xfffffff0, 0xfffffff0, 0x00000003, 0x0000000f, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0,  // 0x0029abe8
    0xfffffff0, 0x00000003, 0x00000010, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0,  // 0x0029ac08
    0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029ac28
    0x00000003, 0x00000011, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003,  // 0x0029ac48
    0x00000012, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000013,  // 0x0029ac68
    0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000014, 0xffffffd0,  // 0x0029ac88
    0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000015, 0xffffffd0, 0xffffff80,  // 0x0029aca8
    0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000016, 0xffffffd0, 0xffffff80, 0x00000000,  // 0x0029acc8
    0xfffffff0, 0xfffffff0, 0x00000003, 0x00000017, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0,  // 0x0029ace8
    0xfffffff0, 0x00000003, 0x00000018, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0,  // 0x0029ad08
    0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029ad28
    0x00000006, 0x00000019, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x03000000,  // 0x0029ad48
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000006, 0x00000019,  // 0x0029ad68
    0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000,  // 0x0029ad88
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000006, 0x0000001a, 0x00000000, 0x00000000,  // 0x0029ada8
    0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029adc8
    0x00000000, 0x00000000, 0x00000006, 0x0000001b, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0,  // 0x0029ade8
    0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029ae08
    0x00000006, 0x0000001c, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000,  // 0x0029ae28
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0x0000001d,  // 0x0029ae48
    0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x0000001e, 0xffffffd0,  // 0x0029ae68
    0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x0000001f, 0xffffffd0, 0xffffff80,  // 0x0029ae88
    0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000020, 0xffffffd0, 0xffffff80, 0x00000000,  // 0x0029aea8
    0xfffffff0, 0xfffffff0, 0x00000003, 0x00000021, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0,  // 0x0029aec8
    0xfffffff0, 0x00000003, 0x00000022, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0,  // 0x0029aee8
    0x00000003, 0x00000023, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003,  // 0x0029af08
    0x00000024, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000,  // 0x0029af28
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0x00000025,  // 0x0029af48
    0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000026, 0xffffffd0,  // 0x0029af68
    0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000027, 0xffffffd0, 0xffffff80,  // 0x0029af88
    0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000028, 0xffffffd0, 0xffffff80, 0x00000000,  // 0x0029afa8
    0xfffffff0, 0xfffffff0, 0x00000003, 0x00000029, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0,  // 0x0029afc8
    0xfffffff0, 0x00000003, 0x0000002a, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0,  // 0x0029afe8
    0x00000003, 0x0000002b, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003,  // 0x0029b008
    0x0000002c, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000,  // 0x0029b028
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0x0000002d,  // 0x0029b048
    0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x0000002e, 0xffffffd0,  // 0x0029b068
    0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x0000002f, 0xffffffd0, 0xffffff80,  // 0x0029b088
    0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000030, 0xffffffd0, 0xffffff80, 0x00000000,  // 0x0029b0a8
    0xfffffff0, 0xfffffff0, 0x00000003, 0x00000031, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0,  // 0x0029b0c8
    0xfffffff0, 0x00000003, 0x00000032, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0,  // 0x0029b0e8
    0x00000003, 0x00000033, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003,  // 0x0029b108
    0x00000034, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000,  // 0x0029b128
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0x00000035,  // 0x0029b148
    0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000036, 0xffffffd0,  // 0x0029b168
    0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000037, 0xffffffd0, 0xffffff80,  // 0x0029b188
    0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000038, 0xffffffd0, 0xffffff80, 0x00000000,  // 0x0029b1a8
    0xfffffff0, 0xfffffff0, 0x00000003, 0x00000039, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0,  // 0x0029b1c8
    0xfffffff0, 0x00000003, 0x0000003a, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0,  // 0x0029b1e8
    0x00000003, 0x0000003b, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003,  // 0x0029b208
    0x0000003c, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000,  // 0x0029b228
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0x0000003d,  // 0x0029b248
    0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x0000003e, 0xffffffd0,  // 0x0029b268
    0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x0000003f, 0xffffffd0, 0xffffff80,  // 0x0029b288
    0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000040, 0xffffffd0, 0xffffff80, 0x00000000,  // 0x0029b2a8
    0xfffffff0, 0xfffffff0, 0x00000003, 0x00000041, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0,  // 0x0029b2c8
    0xfffffff0, 0x00000003, 0x00000042, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0,  // 0x0029b2e8
    0x00000003, 0x00000043, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003,  // 0x0029b308
    0x00000044, 0xffffffd0, 0xffffff80, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000,  // 0x0029b328
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0x00000045,  // 0x0029b348
    0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000,  // 0x0029b368
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0x00000046, 0x00000000, 0x00000000,  // 0x0029b388
    0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029b3a8
    0x00000000, 0x00000000, 0x00000005, 0x00000047, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0,  // 0x0029b3c8
    0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029b3e8
    0x00000005, 0x00000048, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000,  // 0x0029b408
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0x00000049,  // 0x0029b428
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x03000000, 0x00000000, 0x00000000,  // 0x0029b448
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0x0000004a, 0x00000000, 0x00000000,  // 0x0029b468
    0x00000000, 0x00000000, 0x00000000, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029b488
    0x00000000, 0x00000000, 0x00000005, 0x0000004b, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0,  // 0x0029b4a8
    0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029b4c8
    0x00000005, 0x0000004c, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000,  // 0x0029b4e8
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0x0000004d,  // 0x0029b508
    0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000,  // 0x0029b528
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0x0000004e, 0x00000000, 0x00000000,  // 0x0029b548
    0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029b568
    0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x0029a9b8, 0x00000000, 0xffffffff, 0x0029a9f0,  // 0x0029b588
    0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x0029ae60, 0x00000000,  // 0x0029b5a8
    0xffffffff, 0x0029af60, 0x00000000, 0xffffffff, 0x0029b060, 0x00000000, 0xffffffff, 0x0029b160,  // 0x0029b5c8
    0x00000000, 0xffffffff, 0x0029b260, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff,  // 0x0029b5e8
    0x0029ae60, 0x00000000, 0xffffffff, 0x0029af60, 0x00000000, 0xffffffff, 0x0029b060, 0x00000000,  // 0x0029b608
    0xffffffff, 0x0029b160, 0x00000000, 0xffffffff, 0x0029b260, 0xffffffff, 0xffffffff, 0x00000000,  // 0x0029b628
    0x00000000, 0xffffffff, 0x0029ae60, 0x00000000, 0xffffffff, 0x0029af60, 0x00000000, 0xffffffff,  // 0x0029b648
    0x0029b060, 0x00000000, 0xffffffff, 0x0029b160, 0x00000000, 0xffffffff, 0x0029b260, 0xffffffff,  // 0x0029b668
    0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0x0029ad48, 0x00000000, 0xffffffff, 0x0029ad80,  // 0x0029b688
    0x00000000, 0xffffffff, 0x0029adb8, 0x00000000, 0xffffffff, 0x0029adf0, 0xffffffff, 0xffffffff,  // 0x0029b6a8
    0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x0029ae28, 0xffffffff, 0xffffffff, 0x00000000,  // 0x0029b6c8
    0x00000000, 0xffffffff, 0x0029b360, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff,  // 0x0029b6e8
    0x0029b398, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0x0029b3d0, 0xffffffff,  // 0x0029b708
    0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0x0029b408, 0xffffffff, 0xffffffff, 0x00000000,  // 0x0029b728
    0x00000000, 0xffffffff, 0x0029b440, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff,  // 0x0029b748
    0x0029b478, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0x0029b4b0, 0xffffffff,  // 0x0029b768
    0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0x0029b4e8, 0xffffffff, 0xffffffff, 0x00000000,  // 0x0029b788
    0x00000000, 0xffffffff, 0x0029b520, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff,  // 0x0029b7a8
    0x0029b558, 0xffffffff, 0xffffffff, 0x00000000, 0x000000ff, 0x00000000, 0x00000000, 0x00000000,  // 0x0029b7c8
    0x000000ff, 0x00000000, 0x00000000, 0x00000080,  // 0x0029b7e8
};
#define S2DMESPATTERN_EE 0x0029a388u

/* S2dMes's display list: 16 sprites (data 0x29b7f8). */
static const SprDispDef s2dMesDisp[17] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 524288, 0x0029b590u },  // 0x0029b7f8
    { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b690u },  // 0x0029b82c
    { 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b6d0u },  // 0x0029b860
    { 3, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b5b8u },  // 0x0029b894
    { 4, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b600u },  // 0x0029b8c8
    { 5, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b648u },  // 0x0029b8fc
    { 6, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b6e8u },  // 0x0029b930
    { 7, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b700u },  // 0x0029b964
    { 8, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b718u },  // 0x0029b998
    { 9, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b730u },  // 0x0029b9cc
    { 10, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b748u },  // 0x0029ba00
    { 11, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b760u },  // 0x0029ba34
    { 12, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b778u },  // 0x0029ba68
    { 13, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b790u },  // 0x0029ba9c
    { 14, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b7a8u },  // 0x0029bad0
    { 15, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 524288, 0x0029b7c0u },  // 0x0029bb04
    { -1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0x0029b590u },  // 0x0029bb38
};

/* S2dMovie's pattern table (data 0x29bd28..0x29bf10): EE image, see the file comment. */
static u_int s2dMoviePattern[122] = {
    0xffffffff, 0x00000000, 0x00000000, 0x00000100, 0x00000100, 0x00000000, 0x00000000, 0x00000000,  // 0x0029bd28
    0x00001000, 0x00000400, 0x00000000, 0x00000000, 0x00000400, 0x00001000, 0x00000400, 0x00000000,  // 0x0029bd48
    0x00000000, 0x00000800, 0x00001000, 0x00000400, 0x00000000, 0x00000000, 0x00000c00, 0x00001000,  // 0x0029bd68
    0x00000400, 0x00000000, 0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029bd88
    0x00000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029bda8
    0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005,  // 0x0029bdc8
    0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x03000000, 0x00000000,  // 0x0029bde8
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000006, 0x00000001,  // 0x0029be08
    0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000006, 0x00000002, 0x00000000,  // 0x0029be28
    0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000006, 0x00000003, 0x00000000, 0x00000000,  // 0x0029be48
    0x00000000, 0xfffffff0, 0xfffffff0, 0x00000006, 0x00000004, 0x00000000, 0x00000000, 0x00000000,  // 0x0029be68
    0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x0029be88
    0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x0029bd90, 0x00000000, 0xffffffff, 0x0029bdc8,  // 0x0029bea8
    0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x0029be20, 0xffffffff,  // 0x0029bec8
    0xffffffff, 0x00000000, 0x000000ff, 0x00000000, 0x00000000, 0x00000000, 0x000000ff, 0x00000000,  // 0x0029bee8
    0x00000000, 0x00000080,  // 0x0029bf08
};
#define S2DMOVIEPATTERN_EE 0x0029bd28u

/* S2dMovie's display list (data 0x29bf10). */
static const SprDispDef s2dMovieDisp[3] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32752, 0x0029beb0u },  // 0x0029bf10
    { 1, 0, 0, 0, 0, 1, 0, 0, 0, 512, 5920, 524288, 0x0029bed8u },  // 0x0029bf44
    { -1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0x0029beb0u },  // 0x0029bf78
};

/* The S2dMovie item -> sprite map (data 0x29bfb0). */
static const int s2dMovieSprNo[S2D_MOVIE_ITEM_MAX] = { 1 };

/* The layers' TEST_1 (0x5000d): alpha test GREATER than 0, depth test
 * GEQUAL. */
#define S2D_LAYER_TEST SCE_GS_SET_TEST(1, 6, 0, 0, 0, 0, 1, 2)


/* ======================================================================
 *  S2dCineSco
 * ====================================================================== */

// ──────────────────────────────────────────────────────────────────────
// The MagLife0 / MagLife words (the ROM inlines their constructors and
// stores _vt$10S2dCineSco at +0x14), the sprite manager, then clear.
// ROR 0x00190a78

S2dCineSco::S2dCineSco()
{
    // callcheck: SprMan::SprMan (member constructor)
    m_state = 0;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_level = 1;
    m_04 = -1;
    m_state = 0;
    m_hide = 0;
    m_scene = NULL;
    clear();
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 1: closes, ends the sprites when still inited.
// ROR 0x00190ad4

S2dCineSco::~S2dCineSco()
{
    // callcheck: ~SprMan, operator_delete (member destructor, deleting variant)
    close();
    if (m_inited == 1)
        term();
}

// ──────────────────────────────────────────────────────────────────────
// Three sprites on the scene's texture table; opens the layer.
// ROR 0x00190b70

void S2dCineSco::init(DogTexInfo *tex)
{
    m_spr.init(3);
    m_spr.m_tex.m_tex_info = tex;
    open();
    m_inited = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00190b60

int S2dCineSco::clear()
{
    m_inited = 0;
    m_open = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00190bbc

void S2dCineSco::term()
{
    if (m_inited != 0) {
        m_spr.release();
        close();
        m_inited = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 2: the per-frame step.
// ROR 0x00190c04

void S2dCineSco::live()
{
    step();
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 6: draws the sprites into the layer it is given.
// ROR 0x00190c20

void S2dCineSco::render(I3dLayer *layer)
{
    if (layer != NULL) {
        m_spr.m_layer[0] = layer;
        m_spr.draw();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00190c44

int S2dCineSco::open()
{
    if (m_open == 1)
        return -1;
    m_open = 1;
    m_phase = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00190c68

int S2dCineSco::close()
{
    if (m_open == 0)
        return -1;
    m_open = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The phase machine: start once, then run the sprites every frame; 1 when
// the layer went back to phase 0.
// ROR 0x00190c84

int S2dCineSco::step()
{
    if (m_open == 0)
        return -1;
    switch (m_phase) {
    case 0:
        m_phase = 3;
        /* fall through */
    case 3:
        m_phase = 4;
        start();
        return 0;
    case 4:
        process();
        return 0;
    case 6:
        m_phase = 7;
        /* fall through */
    default:    /* 1, 2, 5, 7 */
        m_phase = 0;
        break;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00190d20

void S2dCineSco::start()
{
    m_system = MagSystem::m_current_system;
    m_spr.m_pattern = s2dCineScoPattern;
    m_spr.m_pattern_ee = S2DCINESCOPATTERN_EE;    // PC
    m_spr.setDispList(s2dCineScoDisp);
    m_spr.m_active = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00190d70

int S2dCineSco::process()
{
    m_spr.update();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Shows the two bars (sprites 1 and 2): black at once for time 0, else a
// fade from clear to opaque black over time frames.
// ROR 0x00190d90

int S2dCineSco::fadeIn(int time)
{
    if (time == 0) {
        SprSprite *bar = m_spr.m_sprite;
        bar[1].m_color[0] = 0;
        bar[1].m_color[1] = 0;
        bar[1].m_color[2] = 0;
        bar[1].m_color[3] = 0x80;
        bar[2].m_color[0] = 0;
        bar[2].m_color[1] = 0;
        bar[2].m_color[2] = 0;
        bar[2].m_color[3] = 0x80;
    } else {
        m_spr.setFade(1, 1, time, s2dCineScoBlackClear, s2dCineScoBlackOpaque);
        m_spr.setFade(2, 1, time, s2dCineScoBlackClear, s2dCineScoBlackOpaque);
    }
    m_spr.m_sprite[1].m_disp = 1;
    m_spr.m_sprite[2].m_disp = 1;
    m_spr.m_sprite[1].m_state = 1;
    m_spr.m_sprite[2].m_state = 1;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Hides the bars at once (time 0) or fades them out.
// ROR 0x00190e98

int S2dCineSco::fadeOut(int time)
{
    if (time == 0) {
        m_spr.m_sprite[1].m_disp = 0;
        m_spr.m_sprite[2].m_disp = 0;
        m_spr.m_sprite[1].m_state = 0;
        m_spr.m_sprite[2].m_state = 0;
    } else {
        m_spr.setFade(1, 2, time, s2dCineScoBlackClear, s2dCineScoBlackOpaque);
        m_spr.setFade(2, 2, time, s2dCineScoBlackClear, s2dCineScoBlackOpaque);
        m_spr.m_sprite[1].m_state = 4;
        m_spr.m_sprite[1].m_fade_wait = time;
        m_spr.m_sprite[2].m_state = 4;
        m_spr.m_sprite[2].m_fade_wait = time;
    }
    return 0;
}

/* ======================================================================
 *  S2dBg
 * ====================================================================== */

// ──────────────────────────────────────────────────────────────────────
// As S2dCineSco's (vtable _vt$5S2dBg), the sprite manager at +0x48.
// ROR 0x00190f6c

S2dBg::S2dBg()
{
    // callcheck: SprMan::SprMan (member constructor)
    m_state = 0;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_level = 1;
    m_04 = -1;
    m_state = 0;
    m_hide = 0;
    m_scene = NULL;
    clear();
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 1.
// ROR 0x00190fc8

S2dBg::~S2dBg()
{
    // callcheck: ~SprMan, operator_delete (member destructor, deleting variant)
    close();
    if (m_inited == 1)
        term();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191054

int S2dBg::clear()
{
    m_bg_step = 0;
    m_open = 0;
    m_inited = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Keeps the texture table for the loads; the sprites are allocated when a
// load starts (processBg step 3).
// ROR 0x00191068

void S2dBg::init(DogTexInfo *tex)
{
    m_tex = tex;
    open();
    m_inited = 1;
}

// ──────────────────────────────────────────────────────────────────────
// Unlike the other layers' term, no SprMan::release here (processBg's
// close request frees the sprites).
// ROR 0x00191098

void S2dBg::term()
{
    if (m_inited != 0) {
        close();
        m_inited = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 2: counts the frame (MagLife +4 starts at -1), then steps.
// ROR 0x001910d8

void S2dBg::live()
{
    m_04 = m_04 + 1;
    step();
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 6.
// ROR 0x00191100

void S2dBg::render(I3dLayer *layer)
{
    if (layer != NULL) {
        m_spr.m_layer[0] = layer;
        m_spr.draw();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191124

int S2dBg::open()
{
    if (m_open == 1)
        return -1;
    m_open = 1;
    m_phase = 0;
    m_sub = 0;
    m_44 = 2;
    m_sub_state = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191158

int S2dBg::close()
{
    if (m_open == 0)
        return -1;
    m_open = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// As S2dCineSco::step, with the sub step while m_sub is 0 (it never
// returns 1 here, so phase 4 runs until the layer is closed).
// ROR 0x00191174

int S2dBg::step()
{
    if (m_open == 0)
        return -1;
    switch (m_phase) {
    case 0:
        m_phase = 3;
        /* fall through */
    case 3:
        m_phase = 4;
        start();
        return 0;
    case 4:
        if (m_sub == 0 && subStep() == 1)
            m_phase = 6;
        process();
        return 0;
    case 6:
        m_phase = 7;
        /* fall through */
    default:    /* 1, 2, 5, 7 */
        m_sub_state = 0;
        break;
    }
    m_phase = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191248

void S2dBg::start()
{
    m_system = MagSystem::m_current_system;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191258

int S2dBg::process()
{
    processBg();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191278

int S2dBg::subStep()
{
    if (m_sub_state == 0)
        m_sub_state = 3;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The picture requests (the inline s2dBg* methods move m_bg_step): 1
// empties the slot list, 3 allocates seven sprites and starts loading
// the named pictures out of s2dgen.rpk, 4 waits for them and builds the
// display list, 7 / 8 free the sprites. The sprites run every frame.
// ROR 0x001912a0

int S2dBg::processBg()
{
    int i;

    switch (m_bg_step) {
    case 1:
        for (i = 0; i < 7; i++)
            s2dBgTexList[i].m_id = -1;
        m_bg_step = 2;
        break;
    case 3:
        m_spr.init(7);
        m_spr.m_tex.m_tex_info = m_tex;
        m_spr.m_tex.setListMode(s2dBgTexList, s2dGenRpk, 1);
        m_bg_step = 4;
        break;
    case 4:
        if (m_spr.m_tex.update() != 0) {
            m_spr.m_pattern = s2dBgPattern;
            m_spr.m_pattern_ee = S2DBGPATTERN_EE;    // PC
            m_spr.setDispList(s2dBgDisp);
            m_spr.m_active = 1;
            m_bg_step = 5;
        }
        break;
    case 7:
        m_bg_step = 8;
        /* fall through */
    case 8:
        m_spr.release();
        m_bg_step = 0;
        break;
    }
    m_spr.update();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Names picture no for the next load and ends the list after it.
// ROR 0x001913c4

int S2dBg::setTexName(int no, const char *name)
{
    if (no >= 7)
        return -1;
    s2dBgTexName[no][0] = '\0';
    strcpy(s2dBgTexName[no], name);
    s2dBgTexList[no].m_id = no;
    s2dBgTexList[no + 1].m_id = -1;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Fades picture no in over time frames.
// ROR 0x0019143c

void S2dBg::dispIn(int no, int time)
{
    m_spr.fadeInSprite(no, 1, time, 0, -1, g_sprColorClear, g_sprColorOpaque);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191478

void S2dBg::dispOut(int no, int time)
{
    m_spr.fadeOutDefault(no, time);
}

/* ======================================================================
 *  S2dMes
 * ====================================================================== */

// ──────────────────────────────────────────────────────────────────────
// As S2dCineSco's (vtable _vt$6S2dMes), the sprite manager at +0x48.
// ROR 0x00191494

S2dMes::S2dMes()
{
    // callcheck: SprMan::SprMan (member constructor)
    m_level = 1;
    m_state = 0;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_hide = 0;
    m_04 = -1;
    m_state = 0;
    m_scene = NULL;
    m_open = 0;
    clear();
    m_inited = 0;
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 1.
// ROR 0x001914fc

S2dMes::~S2dMes()
{
    // callcheck: ~SprMan, operator_delete (member destructor, deleting variant)
    close();
    if (m_inited == 1)
        term();
}

// ──────────────────────────────────────────────────────────────────────
// Sixteen sprites; loads the icon textures at once (it waits for them).
// ROR 0x00191588

int S2dMes::init(DogTexInfo *tex)
{
    m_spr.init(16);
    m_spr.m_tex.m_tex_info = tex;
    m_spr.m_00 = 1;
    open();
    loadTexture();
    m_inited = 1;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001915f8

void S2dMes::term()
{
    if (m_inited != 0) {
        m_spr.release();
        close();
        m_inited = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// wait.tm2 / wait2.tm2 from s2dman.rpk; spins until the loader is done.
// ROR 0x00191640

void S2dMes::loadTexture()
{
    m_spr.m_tex.setList(s2dMesTexList, s2dManRpk);
    while (m_spr.m_tex.update() == 0)
        ;
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 2.
// ROR 0x00191684

void S2dMes::live()
{
    step();
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 6.
// ROR 0x001916a0

void S2dMes::render(I3dLayer *layer)
{
    if (layer != NULL) {
        m_spr.m_layer[0] = layer;
        m_spr.draw();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001916c4

int S2dMes::open()
{
    if (m_open == 1)
        return -1;
    m_open = 1;
    m_phase = 0;
    m_sub = 0;
    m_44 = 2;
    m_sub_state = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001916f8

int S2dMes::close()
{
    if (m_open == 0)
        return -1;
    m_map_no = 0;
    m_open = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// As S2dCineSco::step; phase 4 alternates waiting for a map-name request
// and showing the caption.
// ROR 0x00191718

int S2dMes::step()
{
    int ret;

    if (m_open == 0)
        return -1;
    switch (m_phase) {
    case 0:
        m_phase = 3;
        /* fall through */
    case 3:
        m_phase = 4;
        start();
        return 0;
    case 4:
        if (m_sub == 0) {
            ret = waitMapName();
            if (ret == 1)
                m_sub = 1;
            else if (ret == 999)
                m_phase = 6;
        } else if (m_sub == 1 && drawMapName() == 1) {
            m_sub = 0;
        }
        process();
        return 0;
    case 6:
        m_phase = 7;
        /* fall through */
    default:    /* 1, 2, 5, 7 */
        m_sub_state = 0;
        break;
    }
    m_phase = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0019183c

int S2dMes::clear()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191844

void S2dMes::start()
{
    m_system = MagSystem::m_current_system;
    m_spr.m_pattern = s2dMesPattern;
    m_spr.m_pattern_ee = S2DMESPATTERN_EE;    // PC
    m_spr.setDispList(s2dMesDisp);
    m_spr.m_active = 1;
    m_fade_time = 30;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001918a8

int S2dMes::process()
{
    m_spr.update();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Waits for setMapNameFadeIn; 1 once it came (state 14).
// ROR 0x001918c8

int S2dMes::waitMapName()
{
    switch (m_sub_state) {
    case 0:
        m_sub_state = 6;
        m_map_name_on = 0;
        /* fall through */
    case 6:
        if (m_map_name_on == 1)
            m_sub_state = 14;
        return 0;
    case 14:
        m_sub_state = 0;
        return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The map-name caption: the text of word-table record m_map_no in a scene
// text window at y 0x168, centred on x 0x100, with sprite 2 (its frame)
// fading in with it over m_fade_time frames; out again once
// setMapNameFadeOut clears m_map_name_on, then 1.
// ROR 0x00191938

int S2dMes::drawMapName()
{
    MagScene *scene = &m_system->m_scene;

    switch (m_sub_state) {
    case 0: {
        const char *text = NULL;
        WrdEntry *entry;
        int width;

        m_sub_state = 6;
        m_map_name[0] = '\0';
        entry = g_wrdTable.get(m_map_no);
        if (entry != NULL)
            text = entry->m_text;
        if (text != NULL)
            strcpy(m_map_name, text);
        m_view = scene->openWindow(0x200, 0x100, 0, -1);
        width = scene->getWindowTextWidth(m_view, m_map_name);
        scene->updateWindowFont(m_view);
        m_system->setViewParam(m_view, 0x100 - width / 2, 0x168, 0xffff, 0x200, 0x100);
        scene->setWindowShow(m_view, 0);
        m_system->drawView(m_view, 0, 0, m_map_name, 0);
        scene->fadeInWindow(m_view, m_fade_time, 0, 0x80);
        m_spr.m_sprite[2].m_off[0] = 0x9c0;
        m_spr.m_sprite[2].m_off[1] = 0x17c0;
        m_spr.fadeIn(2, m_fade_time, g_sprColorClear, g_sprColorOpaque);
        return 0;
    }
    case 6:
        if (m_map_name_on != 1)
            m_sub_state = 15;
        return 0;
    case 14:
        if (m_fade_time < ++m_fade_count) {
            scene->closeWindow(m_view);
            m_sub_state = 0;
            return 1;
        }
        return 0;
    case 15:
        m_fade_count = 0;
        m_spr.fadeOut(2, m_fade_time, g_sprColorClear, g_sprColorOpaque);
        scene->fadeOutWindow(m_view, m_fade_time, 0, 0x80);
        m_sub_state = 14;
        return 0;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191ba8

bool S2dMes::setSpriteStdPoint(int no, int x, int y)
{
    if (no < S2D_MES_ITEM_MAX) {
        SprSprite *spr = &m_spr.m_sprite[s2dMesSprNo[no]];
        spr->m_std[0] = x;
        spr->m_std[1] = y;
    }
    return no < S2D_MES_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191c00

bool S2dMes::setSpriteStdPoint(int no, int x, int y, int z)
{
    if (no < S2D_MES_ITEM_MAX) {
        SprSprite *spr = &m_spr.m_sprite[s2dMesSprNo[no]];
        spr->m_std[0] = x;
        spr->m_std[1] = y;
        spr->m_std[2] = z;
    }
    return no < S2D_MES_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191c64

bool S2dMes::setSpriteOffPoint(int no, int x, int y)
{
    if (no < S2D_MES_ITEM_MAX) {
        SprSprite *spr = &m_spr.m_sprite[s2dMesSprNo[no]];
        spr->m_off[0] = x;
        spr->m_off[1] = y;
    }
    return no < S2D_MES_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191cbc

bool S2dMes::setSpriteOffPoint(int no, int x, int y, int z)
{
    if (no < S2D_MES_ITEM_MAX) {
        SprSprite *spr = &m_spr.m_sprite[s2dMesSprNo[no]];
        spr->m_off[0] = x;
        spr->m_off[1] = y;
        spr->m_off[2] = z;
    }
    return no < S2D_MES_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191d20

bool S2dMes::setSpriteState(int no, int on)
{
    if (no < S2D_MES_ITEM_MAX)
        m_spr.m_sprite[s2dMesSprNo[no]].m_state = (on == 1);
    return no < S2D_MES_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191d74

bool S2dMes::setSpriteDisp(int no, int on)
{
    if (no < S2D_MES_ITEM_MAX)
        m_spr.m_sprite[s2dMesSprNo[no]].m_disp = (on == 1);
    return no < S2D_MES_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191dc8

bool S2dMes::setSpriteAnim(int no, int anim)
{
    if (no < S2D_MES_ITEM_MAX)
        m_spr.setAnim(s2dMesSprNo[no], anim);
    return no < S2D_MES_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191e04

bool S2dMes::setSpriteFadeIn(int no, int time)
{
    if (no < S2D_MES_ITEM_MAX)
        m_spr.setFade(s2dMesSprNo[no], 1, time, g_sprColorClear, g_sprColorOpaque);
    return no < S2D_MES_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// Fades the item out when it is on; 1 for a valid item.
// ROR 0x00191e58

int S2dMes::setSpriteFadeOut(int no, int time)
{
    int ret = 0;

    if (no < S2D_MES_ITEM_MAX) {
        if (m_spr.m_sprite[s2dMesSprNo[no]].m_state != 0)
            m_spr.fadeOut(s2dMesSprNo[no], time, g_sprColorClear, g_sprColorOpaque);
        ret = 1;
    }
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191ed0

bool S2dMes::setSpriteColorStd(int no)
{
    if (no < S2D_MES_ITEM_MAX) {
        SprSprite *spr = &m_spr.m_sprite[s2dMesSprNo[no]];
        spr->m_color[0] = 0x80;
        spr->m_color[1] = 0x80;
        spr->m_color[2] = 0x80;
        spr->m_color[3] = 0x80;
    }
    return no < S2D_MES_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191f44

bool S2dMes::setSpriteProcessOff(int no)
{
    if (no < S2D_MES_ITEM_MAX)
        m_spr.m_sprite[s2dMesSprNo[no]].m_flags |= 2;
    return no < S2D_MES_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00191f98

bool S2dMes::setSpriteProcessOn(int no)
{
    if (no < S2D_MES_ITEM_MAX)
        m_spr.m_sprite[s2dMesSprNo[no]].m_flags &= ~2u;
    return no < S2D_MES_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// Saves the item's process bit, points and state (the ROM reads the std
// x word for both std backups: kept).
// ROR 0x00191ff0

bool S2dMes::backupSprite(int no)
{
    if (no < S2D_MES_ITEM_MAX) {
        SprSprite *spr = &m_spr.m_sprite[s2dMesSprNo[no]];
        s2dMesBackupProcess[no] = spr->m_flags & 2;
        s2dMesBackupStdX[no] = spr->m_std[0];
        s2dMesBackupStdY[no] = spr->m_std[0];
        s2dMesBackupOffX[no] = spr->m_off[0];
        s2dMesBackupOffY[no] = spr->m_off[1];
        s2dMesBackupState[no] = spr->m_state;
    }
    return no < S2D_MES_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// Puts backupSprite's words back (both std backups into std x, as the
// ROM) and shows the item again or hides it.
// ROR 0x001920c4

int S2dMes::restoreSprite(int no)
{
    int ret = 0;

    if (no < S2D_MES_ITEM_MAX) {
        SprSprite *spr = &m_spr.m_sprite[s2dMesSprNo[no]];
        if (s2dMesBackupProcess[no] == 2)
            spr->m_flags |= 2;
        else
            spr->m_flags &= ~2u;
        spr->m_std[0] = s2dMesBackupStdX[no];
        spr->m_std[0] = s2dMesBackupStdY[no];
        spr->m_off[0] = s2dMesBackupOffX[no];
        spr->m_off[1] = s2dMesBackupOffY[no];
        spr->m_state = s2dMesBackupState[no];
        if (s2dMesBackupState[no] == 0) {
            setSpriteState(no, 0);
            setSpriteDisp(no, 0);
            ret = 1;
        } else {
            setSpriteColorStd(no);
            setSpriteState(no, 1);
            setSpriteDisp(no, 1);
            setSpriteAnim(no, 0);
            ret = 1;
        }
    }
    return ret;
}

/* ======================================================================
 *  S2dMovie
 * ====================================================================== */

// ──────────────────────────────────────────────────────────────────────
// No vtable: the sprite manager at +0x18, closed and not inited.
// ROR 0x0019226c

S2dMovie::S2dMovie()
{
    // callcheck: SprMan::SprMan (member constructor)
    m_open = 0;
    clear();
    m_inited = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001922a4

S2dMovie::~S2dMovie()
{
    // callcheck: ~SprMan, operator_delete (member destructor, deleting variant)
    close();
    if (m_inited == 1)
        term();
}

// ──────────────────────────────────────────────────────────────────────
// Two sprites on the scene's texture table; loads loading.tm2 at once.
// ROR 0x00192318

void S2dMovie::init(i3dContext *ctx, MagScene *scene)
{
    (void)ctx;
    m_scene = scene;
    m_spr.init(2);
    m_spr.m_tex.m_tex_info = &scene->m_tex_info;
    open();
    loadTexture();
    m_inited = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0019237c

void S2dMovie::term()
{
    if (m_inited != 0) {
        m_spr.release();
        close();
        m_inited = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001923c4

void S2dMovie::loadTexture()
{
    m_spr.m_tex.setList(s2dMovieTexList, s2dManRpk);
    while (m_spr.m_tex.update() == 0)
        ;
}

// ──────────────────────────────────────────────────────────────────────
// MagScene::update's per-frame call.
// ROR 0x00192408

void S2dMovie::update()
{
    step();
}

// ──────────────────────────────────────────────────────────────────────
// The movie frame, while RxMpeg has one: a layer at key 890 with the
// scene's light set 0 and view, depth / alpha test on, drawn by RxMpeg.
// ROR 0x0019241c

void S2dMovie::drawMovie(i3dContext *ctx, I3dLight *light, I3dView *view)
{
    I3dLayer *layer;

    if (g_rxMpeg.m_playing == 1 && (layer = createLayer(ctx, 890.0f)) != NULL) {
        i3dLayerSetLight(layer, light);
        i3dLayerSetView(layer, view);
        i3dLayerEndPacket(layer, i3dPktSetReg(i3dLayerBeginPacket(layer), SCE_GS_TEST_1, S2D_LAYER_TEST));
        g_rxMpeg.draw(layer);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The sprites: two layers (keys 900 and 1000) set up as drawMovie's, the
// SprMan's layer pair, then SprMan::draw.
// ROR 0x001924e0

void S2dMovie::draw(i3dContext *ctx, I3dLight *light, I3dView *view)
{
    I3dLayer *layer0 = createLayer(ctx, 900.0f);
    I3dLayer *layer1 = createLayer(ctx, 1000.0f);

    if (layer0 == NULL || layer1 == NULL)
        return;
    i3dLayerSetLight(layer0, light);
    i3dLayerSetView(layer0, view);
    i3dLayerEndPacket(layer0, i3dPktSetReg(i3dLayerBeginPacket(layer0), SCE_GS_TEST_1, S2D_LAYER_TEST));
    i3dLayerSetLight(layer1, light);
    i3dLayerSetView(layer1, view);
    i3dLayerEndPacket(layer1, i3dPktSetReg(i3dLayerBeginPacket(layer1), SCE_GS_TEST_1, S2D_LAYER_TEST));
    m_spr.m_layer[0] = layer0;
    m_spr.m_layer[1] = layer1;
    m_spr.draw();
}

// ──────────────────────────────────────────────────────────────────────
// A layer at key with a view from the scene's view 0 and camera. Both
// callers replace the view at once with MagScene::draw's.
// ROR 0x001925f0

I3dLayer *S2dMovie::createLayer(i3dContext *ctx, float key)
{
    I3dLayer *layer = i3dCtxCreateLayer(ctx, key);
    MagScene *scene = m_scene;
    sceVu0FVECTOR *matrix = scene->m_camera_entity.getMatrix();

    i3dLayerSetView(layer, i3dCtxCreateView(ctx, &scene->m_view[0], matrix));
    return layer;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00192670

int S2dMovie::open()
{
    if (m_open == 1)
        return -1;
    m_open = 1;
    m_phase = 0;
    m_sub = 0;
    m_14 = 2;
    m_sub_state = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001926a4

int S2dMovie::close()
{
    if (m_open == 0)
        return -1;
    m_open = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// As S2dBg::step.
// ROR 0x001926c0

int S2dMovie::step()
{
    if (m_open == 0)
        return -1;
    switch (m_phase) {
    case 0:
        m_phase = 3;
        /* fall through */
    case 3:
        m_phase = 4;
        start();
        return 0;
    case 4:
        if (m_sub == 0 && subStep() == 1)
            m_phase = 6;
        process();
        return 0;
    case 6:
        m_phase = 7;
        /* fall through */
    default:    /* 1, 2, 5, 7 */
        m_sub_state = 0;
        break;
    }
    m_phase = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00192794

int S2dMovie::clear()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0019279c

void S2dMovie::start()
{
    m_system = MagSystem::m_current_system;
    m_spr.m_pattern = s2dMoviePattern;
    m_spr.m_pattern_ee = S2DMOVIEPATTERN_EE;    // PC
    m_spr.setDispList(s2dMovieDisp);
    m_spr.m_active = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001927ec

int S2dMovie::process()
{
    m_spr.update();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0019280c

int S2dMovie::subStep()
{
    if (m_sub_state == 0)
        m_sub_state = 3;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00192834

void S2dMovie::playMovie(const char *name)
{
    playMovie(name, 0x200, 0x180);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00192850

void S2dMovie::playMovie(const char *name, int w, int h)
{
    strcpy(g_rxMpeg.m_name, name);
    g_rxMpeg.open();
    g_rxMpeg.play(g_rxMpeg.m_name, w, h);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001928b8

int S2dMovie::isMovieEnd()
{
    return g_rxMpeg.isReady();
}

// ──────────────────────────────────────────────────────────────────────
// The skip: the scene goes to sub mode 8 until closeMovie.
// ROR 0x001928d4

int S2dMovie::stopMovie()
{
    m_system->m_scene.pushSubMode();
    return g_rxMpeg.run();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00192904

int S2dMovie::getMovieStatus()
{
    return g_rxMpeg.getStatus();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00192920

int S2dMovie::closeMovie()
{
    int ret = g_rxMpeg.close();

    if (ret >= 0)
        m_system->m_scene.popSubMode();
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// As S2dMes's setters; no caller found in the ROM.
// ROR 0x00192978

bool S2dMovie::setSpriteStdPoint(int no, int x, int y)
{
    if (no < S2D_MOVIE_ITEM_MAX) {
        SprSprite *spr = &m_spr.m_sprite[s2dMovieSprNo[no]];
        spr->m_std[0] = x;
        spr->m_std[1] = y;
    }
    return no < S2D_MOVIE_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001929cc

bool S2dMovie::setSpriteOffPoint(int no, int x, int y)
{
    if (no < S2D_MOVIE_ITEM_MAX) {
        SprSprite *spr = &m_spr.m_sprite[s2dMovieSprNo[no]];
        spr->m_off[0] = x;
        spr->m_off[1] = y;
    }
    return no < S2D_MOVIE_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00192a20

bool S2dMovie::setSpriteState(int no, int on)
{
    if (no < S2D_MOVIE_ITEM_MAX)
        m_spr.m_sprite[s2dMovieSprNo[no]].m_state = (on == 1);
    return no < S2D_MOVIE_ITEM_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00192a70

bool S2dMovie::setSpriteDisp(int no, int on)
{
    if (no < S2D_MOVIE_ITEM_MAX)
        m_spr.m_sprite[s2dMovieSprNo[no]].m_disp = (on == 1);
    return no < S2D_MOVIE_ITEM_MAX;
}
