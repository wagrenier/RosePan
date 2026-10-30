/* ==========================================================================
 *  mnu/mnuboard2d.cpp
 *
 *  The 2D board (0x1ad6b4..0x1adf04; no source string names the file).
 *  See mnu/mnuboard2d.h.
 * ======================================================================== */
#include "mnu/mnuboard2d.h"

#include <stddef.h>
#include <string.h>

#include "bed/bedpad.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "sequence/sequence.h"
#include "snd/sndman.h"

/* The board pictures' texture names by key (data 0x2abdb8). EE 8 bytes;
 * layout: host -- m_name is a pointer. */
struct MnuBoardTexName
{
    int         m_key;          // ROR 0x00: a type value of the 0x40000 family, -1 ends the table
    const char *m_name;         // ROR 0x04: the file in mnuboard.rpk, without ".tm2"
};

static const MnuBoardTexName mnuBoardTexNames[229] = {
    { 0x40000, "k2_dummy" },  // 0x002abdb8
    { 0x40001, "k2_1_1" },  // 0x002abdc0
    { 0x40002, "k2_1_2" },  // 0x002abdc8
    { 0x40003, "k2_1_10e" },  // 0x002abdd0
    { 0x40004, "k2_1_13" },  // 0x002abdd8
    { 0x40005, "k2_1_15" },  // 0x002abde0
    { 0x40006, "k2_2_6e" },  // 0x002abde8
    { 0x40007, "k2_2_8" },  // 0x002abdf0
    { 0x40008, "k2_2_11d" },  // 0x002abdf8
    { 0x40009, "k2_2_11e" },  // 0x002abe00
    { 0x4000a, "k2_2_11f" },  // 0x002abe08
    { 0x4000b, "k2_2_13" },  // 0x002abe10
    { 0x4000c, "k2_2_28a" },  // 0x002abe18
    { 0x4000d, "k2_2_54" },  // 0x002abe20
    { 0x4000e, "k2_2_55" },  // 0x002abe28
    { 0x4000f, "k2_2_56" },  // 0x002abe30
    { 0x40010, "k2_2_57" },  // 0x002abe38
    { 0x40011, "k2_2_58" },  // 0x002abe40
    { 0x40012, "k2_3_4" },  // 0x002abe48
    { 0x40013, "k2_3_6" },  // 0x002abe50
    { 0x40014, "k2_3_7" },  // 0x002abe58
    { 0x40015, "k2_3_11" },  // 0x002abe60
    { 0x40016, "k2_3_11a" },  // 0x002abe68
    { 0x40017, "k2_3_11b" },  // 0x002abe70
    { 0x40018, "k2_3_11c" },  // 0x002abe78
    { 0x40019, "k2_3_11d" },  // 0x002abe80
    { 0x4001a, "k2_3_13" },  // 0x002abe88
    { 0x4001b, "k2_3_13a" },  // 0x002abe90
    { 0x4001c, "k2_3_13b" },  // 0x002abe98
    { 0x4001d, "k2_3_13c" },  // 0x002abea0
    { 0x4001e, "k2_3_13d" },  // 0x002abea8
    { 0x4001f, "k2_3_19" },  // 0x002abeb0
    { 0x40020, "k2_3_40" },  // 0x002abeb8
    { 0x40021, "k2_3_52" },  // 0x002abec0
    { 0x40022, "k2_4_35" },  // 0x002abec8
    { 0x40023, "k2_4_8b" },  // 0x002abed0
    { 0x40024, "k2_5_3a" },  // 0x002abed8
    { 0x40025, "k2_5_20a" },  // 0x002abee0
    { 0x40026, "k2_5_20b" },  // 0x002abee8
    { 0x40027, "k2_5_21a" },  // 0x002abef0
    { 0x40028, "k2_5_21c" },  // 0x002abef8
    { 0x40029, "k2_5_24a" },  // 0x002abf00
    { 0x4002a, "k2_7_10b" },  // 0x002abf08
    { 0x4002b, "k2_8_0d" },  // 0x002abf10
    { 0x4002c, "k2_8_19" },  // 0x002abf18
    { 0x4002d, "k2_10_3" },  // 0x002abf20
    { 0x4002e, "k2_10_6" },  // 0x002abf28
    { 0x4002f, "k2_10_7" },  // 0x002abf30
    { 0x40030, "k2_10_16" },  // 0x002abf38
    { 0x40031, "k2_10_17" },  // 0x002abf40
    { 0x40032, "k2_10_18" },  // 0x002abf48
    { 0x40033, "k2_10_19" },  // 0x002abf50
    { 0x40034, "k2_10_20" },  // 0x002abf58
    { 0x40035, "k2_10_21" },  // 0x002abf60
    { 0x40036, "k2_10_26" },  // 0x002abf68
    { 0x40037, "k2_10_42a" },  // 0x002abf70
    { 0x40038, "k2_10_a42a" },  // 0x002abf78
    { 0x40039, "k2_11_2" },  // 0x002abf80
    { 0x4003a, "k2_11_29" },  // 0x002abf88
    { 0x4003b, "k2_11_b42a" },  // 0x002abf90
    { 0x4003c, "k2_12_3" },  // 0x002abf98
    { 0x4003d, "k2_12_9" },  // 0x002abfa0
    { 0x4003e, "k2_12_18" },  // 0x002abfa8
    { 0x4003f, "k2_00" },  // 0x002abfb0
    { 0x40040, "k2_01" },  // 0x002abfb8
    { 0x40041, "k2_02" },  // 0x002abfc0
    { 0x40042, "k2_03" },  // 0x002abfc8
    { 0x40043, "k2_04" },  // 0x002abfd0
    { 0x40044, "k2_05" },  // 0x002abfd8
    { 0x40045, "k2_06" },  // 0x002abfe0
    { 0x40046, "k2_07" },  // 0x002abfe8
    { 0x40047, "k2_08" },  // 0x002abff0
    { 0x40048, "k2_09" },  // 0x002abff8
    { 0x40049, "k2_10" },  // 0x002ac000
    { 0x4004a, "k2_11" },  // 0x002ac008
    { 0x4004b, "k2_12" },  // 0x002ac010
    { 0x4004c, "k2_13" },  // 0x002ac018
    { 0x4004d, "k2_14" },  // 0x002ac020
    { 0x4004e, "k2_15" },  // 0x002ac028
    { 0x4004f, "k2_16" },  // 0x002ac030
    { 0x40050, "k2_17" },  // 0x002ac038
    { 0x40051, "k2_18" },  // 0x002ac040
    { 0x40052, "k2_19" },  // 0x002ac048
    { 0x40053, "k2_20" },  // 0x002ac050
    { 0x40054, "k2_21" },  // 0x002ac058
    { 0x40055, "k2_22" },  // 0x002ac060
    { 0x40056, "k2_23" },  // 0x002ac068
    { 0x40057, "k2_24" },  // 0x002ac070
    { 0x40058, "k2_25" },  // 0x002ac078
    { 0x40059, "k2_26" },  // 0x002ac080
    { 0x4005a, "k2_27" },  // 0x002ac088
    { 0x4005b, "k2_28" },  // 0x002ac090
    { 0x4005c, "k2_29" },  // 0x002ac098
    { 0x4005d, "k2_30" },  // 0x002ac0a0
    { 0x4005e, "k2_31" },  // 0x002ac0a8
    { 0x4005f, "k2_32" },  // 0x002ac0b0
    { 0x40060, "k2_33" },  // 0x002ac0b8
    { 0x40061, "k2_34" },  // 0x002ac0c0
    { 0x40062, "k2_35" },  // 0x002ac0c8
    { 0x40063, "k2_36" },  // 0x002ac0d0
    { 0x40064, "k2_37" },  // 0x002ac0d8
    { 0x40065, "k2_38" },  // 0x002ac0e0
    { 0x40066, "k2_39" },  // 0x002ac0e8
    { 0x40067, "k2_40" },  // 0x002ac0f0
    { 0x40068, "k2_41" },  // 0x002ac0f8
    { 0x40069, "k2_42" },  // 0x002ac100
    { 0x4006a, "k2_43" },  // 0x002ac108
    { 0x4006b, "k2_44" },  // 0x002ac110
    { 0x4006c, "k2_45" },  // 0x002ac118
    { 0x4006d, "k2_46" },  // 0x002ac120
    { 0x4006e, "k2_47" },  // 0x002ac128
    { 0x4006f, "k2_48" },  // 0x002ac130
    { 0x40070, "k2_49" },  // 0x002ac138
    { 0x40071, "k2_50" },  // 0x002ac140
    { 0x40072, "k2_51" },  // 0x002ac148
    { 0x40073, "k2_52" },  // 0x002ac150
    { 0x40074, "k2_53" },  // 0x002ac158
    { 0x40075, "k2_54" },  // 0x002ac160
    { 0x40076, "k2_55" },  // 0x002ac168
    { 0x40077, "k2_56" },  // 0x002ac170
    { 0x40078, "k2_57" },  // 0x002ac178
    { 0x40079, "k2_58" },  // 0x002ac180
    { 0x4007a, "k2_59" },  // 0x002ac188
    { 0x4007b, "k2_60" },  // 0x002ac190
    { 0x4007c, "k2_61" },  // 0x002ac198
    { 0x4007d, "k2_62" },  // 0x002ac1a0
    { 0x4007e, "k2_63" },  // 0x002ac1a8
    { 0x4007f, "k2_64" },  // 0x002ac1b0
    { 0x40080, "k2_65" },  // 0x002ac1b8
    { 0x40081, "k2_66" },  // 0x002ac1c0
    { 0x40082, "k2_67" },  // 0x002ac1c8
    { 0x40083, "k2_68" },  // 0x002ac1d0
    { 0x40084, "k2_69" },  // 0x002ac1d8
    { 0x40085, "k2_70" },  // 0x002ac1e0
    { 0x40086, "k2_71" },  // 0x002ac1e8
    { 0x40087, "k2_72" },  // 0x002ac1f0
    { 0x40088, "k2_73" },  // 0x002ac1f8
    { 0x40089, "k2_74" },  // 0x002ac200
    { 0x4008a, "k2_75" },  // 0x002ac208
    { 0x4008b, "k2_76" },  // 0x002ac210
    { 0x4008c, "k2_77" },  // 0x002ac218
    { 0x4008d, "k2_78" },  // 0x002ac220
    { 0x4008e, "k2_79" },  // 0x002ac228
    { 0x4008f, "k2_80" },  // 0x002ac230
    { 0x40090, "k2_81" },  // 0x002ac238
    { 0x40091, "k2_82" },  // 0x002ac240
    { 0x40092, "k2_83" },  // 0x002ac248
    { 0x40093, "k2_84" },  // 0x002ac250
    { 0x40094, "k2_85" },  // 0x002ac258
    { 0x40095, "k2_86" },  // 0x002ac260
    { 0x40096, "k2_87" },  // 0x002ac268
    { 0x40097, "k2_88" },  // 0x002ac270
    { 0x40098, "k2_89" },  // 0x002ac278
    { 0x40099, "k2_90" },  // 0x002ac280
    { 0x4009a, "k2_91" },  // 0x002ac288
    { 0x4009b, "k2_92" },  // 0x002ac290
    { 0x4009c, "k2_93" },  // 0x002ac298
    { 0x4009d, "k2_94" },  // 0x002ac2a0
    { 0x4009e, "k2_95" },  // 0x002ac2a8
    { 0x4009f, "k2_96" },  // 0x002ac2b0
    { 0x400a0, "k2_97" },  // 0x002ac2b8
    { 0x400a1, "k2_98" },  // 0x002ac2c0
    { 0x400a2, "k2_99" },  // 0x002ac2c8
    { 0x400a3, "k2_100" },  // 0x002ac2d0
    { 0x400a4, "k2_101" },  // 0x002ac2d8
    { 0x400a5, "k2_102" },  // 0x002ac2e0
    { 0x400a6, "k2_103" },  // 0x002ac2e8
    { 0x400a7, "k2_104" },  // 0x002ac2f0
    { 0x400a8, "k2_105" },  // 0x002ac2f8
    { 0x400a9, "k2_106" },  // 0x002ac300
    { 0x400aa, "k2_107" },  // 0x002ac308
    { 0x400ab, "k2_108" },  // 0x002ac310
    { 0x400ac, "k2_109" },  // 0x002ac318
    { 0x400ad, "k2_110" },  // 0x002ac320
    { 0x400ae, "k2_111" },  // 0x002ac328
    { 0x400af, "k2_112" },  // 0x002ac330
    { 0x400b0, "k2_113" },  // 0x002ac338
    { 0x400b1, "k2_114" },  // 0x002ac340
    { 0x400b2, "k2_115" },  // 0x002ac348
    { 0x400b3, "k2_116" },  // 0x002ac350
    { 0x400b4, "k2_117" },  // 0x002ac358
    { 0x400b5, "k2_118" },  // 0x002ac360
    { 0x400b6, "k2_119" },  // 0x002ac368
    { 0x400b7, "k2_120" },  // 0x002ac370
    { 0x400b8, "k2_121" },  // 0x002ac378
    { 0x400b9, "k2_122" },  // 0x002ac380
    { 0x400ba, "k2_123" },  // 0x002ac388
    { 0x400bb, "k2_124" },  // 0x002ac390
    { 0x400bc, "k2_125" },  // 0x002ac398
    { 0x400bd, "k2_126" },  // 0x002ac3a0
    { 0x400be, "k2_127" },  // 0x002ac3a8
    { 0x400bf, "k2_128" },  // 0x002ac3b0
    { 0x400c0, "k2_129" },  // 0x002ac3b8
    { 0x400c1, "k2_130" },  // 0x002ac3c0
    { 0x400c2, "k2_131" },  // 0x002ac3c8
    { 0x400c3, "k2_132" },  // 0x002ac3d0
    { 0x400c4, "k2_133" },  // 0x002ac3d8
    { 0x400c5, "k2_134" },  // 0x002ac3e0
    { 0x400c6, "k2_135" },  // 0x002ac3e8
    { 0x400c7, "k2_136" },  // 0x002ac3f0
    { 0x400c8, "k2_137" },  // 0x002ac3f8
    { 0x400c9, "k2_138" },  // 0x002ac400
    { 0x400ca, "k2_139" },  // 0x002ac408
    { 0x400cb, "k2_140" },  // 0x002ac410
    { 0x400cc, "k2_141" },  // 0x002ac418
    { 0x400cd, "k2_142" },  // 0x002ac420
    { 0x400ce, "k2_143" },  // 0x002ac428
    { 0x400cf, "k2_144" },  // 0x002ac430
    { 0x400d0, "k2_145" },  // 0x002ac438
    { 0x400d1, "k2_146" },  // 0x002ac440
    { 0x400d2, "k2_147" },  // 0x002ac448
    { 0x400d3, "k2_148" },  // 0x002ac450
    { 0x400d4, "k2_149" },  // 0x002ac458
    { 0x400d5, "k2_150" },  // 0x002ac460
    { 0x400d6, "k2_151" },  // 0x002ac468
    { 0x400d7, "k2_152" },  // 0x002ac470
    { 0x400d8, "k2_153" },  // 0x002ac478
    { 0x400d9, "k2_154" },  // 0x002ac480
    { 0x400da, "k2_155" },  // 0x002ac488
    { 0x400db, "k2_156" },  // 0x002ac490
    { 0x400dc, "k2_157" },  // 0x002ac498
    { 0x400dd, "k2_158" },  // 0x002ac4a0
    { 0x400de, "k2_159" },  // 0x002ac4a8
    { 0x400df, "k2_160" },  // 0x002ac4b0
    { 0x400e0, "k2_161" },  // 0x002ac4b8
    { 0x400e1, "k2_162" },  // 0x002ac4c0
    { 0x400e2, "k2_163" },  // 0x002ac4c8
    { 0x400e3, "k2_164" },  // 0x002ac4d0
    { -1, NULL },  // 0x002ac4d8
};

/* exec's texture file name: the key's name plus ".tm2" (data 0x2ac4e0;
 * PC: 0x80 bytes, the size MnuItem's copy of it has). */
static char mnuBoardFileName[0x80];

/* The one texture, loaded file by file into "board1_tcc" (data 0x2abad8). */
static SprTexFile mnuBoardTexList[2] = {
    { 0, mnuBoardFileName, "board1_tcc", 0x100040, 1, NULL },  // 0x002abad8
    { -1, "", "", 0, 0, NULL },                                // 0x002abaf0
};

static const char mnuBoardExt[] = ".tm2";                          /* rodata 0x2fabf8 */
static const char mnuBoardRpk[] = "data/mnurpk/mnuboard.rpk";      /* rodata 0x2fac00 */

/* The sprites' table image (data 0x2abb08..0x2abce8): the SprPattern
 * first (m_pattern points at it), then the anim tables the display list
 * names (0x2abc68, 0x2abc90) and the fade colours. Writable: SprMan
 * writes the anim counts back. */
static u_int mnuBoardPattern[120] = {
    0xffffffff, 0x00000000, 0x00000000, 0x00000100, 0x00000100, 0x00000000, 0x00000000, 0x00000000,  // 0x002abb08
    0x00002000, 0x00001c00, 0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002abb28
    0x00000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002abb48
    0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005,  // 0x002abb68
    0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0xffffffff,  // 0x002abb88
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0xffffffff, 0x00000000,  // 0x002abba8
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0xffffffff, 0x00000000, 0x00000000,  // 0x002abbc8
    0x00000000, 0x00000000, 0x00000000, 0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000,  // 0x002abbe8
    0x00000000, 0x00000000, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002abc08
    0x00000000, 0x00000000, 0x0000001e, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00002000,  // 0x002abc28
    0x00001c00, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002abc48
    0x00000000, 0xffffffff, 0x002abb30, 0x00000000, 0xffffffff, 0x002abb68, 0xffffffff, 0xffffffff,  // 0x002abc68
    0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x002abc30, 0xffffffff, 0xffffffff, 0x00000000,  // 0x002abc88
    0x00000080, 0x00000080, 0x00000080, 0x00000080, 0x00000080, 0x00000080, 0x00000080, 0x00000000,  // 0x002abca8
    0x00000080, 0x00000080, 0x00000080, 0x00000028, 0x00000080, 0x00000080, 0x00000080, 0x00000080,  // 0x002abcc8
};

#define MNUBOARDPATTERN_EE 0x002abb08u

/* The display list (data 0x2abce8): the picture's three sprites. */
static const SprDispDef mnuBoardDisp[4] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 32000 }, 0x002abc68u },   // 0x002abce8
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 32016 }, 0x002abc68u },   // 0x002abd1c
    { 2, 0, 0, 0, 0, 1, 0, 1, 0, { 0, 0, 32160 }, 0x002abc90u },   // 0x002abd50
    { -1, 0, 0, 0, 0, 1, 0, 1, 0, { 0, 0, 0 }, 0x002abc68u },      // 0x002abd84
};

/* g_sequence.m_global's fade times (in frames) for the board. */
#define MNUBOARD_GLOBAL_FADE_IN  0x59   /* +0x164 */
#define MNUBOARD_GLOBAL_FADE_OUT 0x5a   /* +0x168 */

/* g_sequence.m_bits[1]: bit 7 holds the board in mode 0 (the script
 * clears it to close), bit 30 is set while a menu is shown. */
#define MNUBOARD_BIT_HOLD   0x80u
#define MNUBOARD_BIT_SHOWN  0x40000000u

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ad6b4; callcheck: MsgMan, SprMan (member constructors: implicit on the host)

MnuBoard2D::MnuBoard2D()
{
    m_open = 0;
    m_board = 0x40001;
    initWork();
    m_init = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ad72c; callcheck: operator_delete, ~SprMan, ~MsgMan (member destructors and the deleting variant: implicit on the host)

MnuBoard2D::~MnuBoard2D()
{
    close();
    if (m_init == 1)
        term();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ad7d4

const char *mnuBoardGetTexName(int key)
{
    for (const MnuBoardTexName *e = mnuBoardTexNames; e->m_key != -1; e++) {
        if (e->m_key == key)
            return e->m_name;
    }
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Sets up the sprite manager (3 sprites, textures into tex) and opens;
// exec loads the texture.
// ROR 0x001ad824

void MnuBoard2D::init(DogTexInfo *tex)
{
    m_spr.init(3);
    m_spr.m_tex.m_tex_info = tex;
    open();
    m_init = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ad870

void MnuBoard2D::term()
{
    if (m_init != 0) {
        if (m_keep_tex == 0)
            m_spr.release();
        close();
        m_init = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ad8bc

int MnuBoard2D::isInit()
{
    return m_init;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ad8cc

int MnuBoard2D::processPad(BedPad *pad)
{
    return exec(pad);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ad8e0

void MnuBoard2D::draw(I3dLayer *layer)
{
    m_layer = layer;
    if (layer != NULL) {
        m_spr.m_layer[0] = layer;
        m_spr.draw();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ad90c

int MnuBoard2D::open()
{
    if (m_open == 1)
        return -1;
    m_open = 1;
    m_step = 0;
    m_main_mode = 0;
    m_7a0 = 2;
    m_main_step = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ad940

int MnuBoard2D::close()
{
    if (m_open == 0)
        return -1;
    m_open = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// One frame (processPad): load the board's texture (steps 0..2), set up
// the sprites and fade in (3), run the mode's execMain (4), fade out (6,
// 7) and close (8). 1 when closed, -1 before open.
// ROR 0x001ad96c

int MnuBoard2D::exec(BedPad *pad)
{
    if (m_open == 0)
        return -1;

    int ret = 0;
    switch (m_step) {
    case 0:
        m_keep_tex = 0;
        m_step = 1;
        /* fall through */
    case 1: {
        m_keep_tex = 0;
        m_step = 2;
        /* PORT: the ROM copies the name unchecked (NULL for a key the
         * table lacks). */
        const char *name = mnuBoardGetTexName(m_board);
        strcpy(mnuBoardFileName, name ? name : "");
        strcpy(mnuBoardFileName + strlen(mnuBoardFileName), mnuBoardExt);
        m_spr.m_tex.setListMode(mnuBoardTexList, mnuBoardRpk, 1);
        break;
    }

    case 2:
        if (m_spr.m_tex.update() != 0)
            m_step = 3;
        break;

    case 3:
        setDisp();
        m_step = 4;
        m_system->m_scene.fadeIn(g_sequence.m_global[MNUBOARD_GLOBAL_FADE_IN], 7, 0);
        m_spr.saveVolume();
        SprMan::volumeOff();
        break;

    case 4:
        if (m_main_mode == 0) {
            if (execMainFlag(pad) == 1)
                m_step = 6;
        } else if (m_main_mode == 1) {
            if (execMainPad(pad) == 1)
                m_step = 6;
        }
        update(pad);
        break;

    case 6:
        m_system->m_scene.fadeOut(g_sequence.m_global[MNUBOARD_GLOBAL_FADE_OUT], 7, 0);
        m_step = 7;
        break;

    case 7:
        if (m_system->m_scene.getFadeState() > 4) {
            m_spr.m_active = 0;
            m_step = 8;
        }
        break;

    case 5:
    case 8:
    default:
        g_sequence.m_bits[1] &= ~MNUBOARD_BIT_SHOWN;
        m_spr.restoreVolume();
        m_step = 0;
        m_main_step = 0;
        ret = 1;
        break;
    }
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001adbd8

int MnuBoard2D::initWork()
{
    m_77c = 0;
    m_780 = 0;
    m_784 = 1;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001adc0c

void MnuBoard2D::setDisp()
{
    m_system = MagSystem::m_current_system;
    m_spr.m_pattern = mnuBoardPattern;
    m_spr.m_pattern_ee = MNUBOARDPATTERN_EE;    // PC
    m_spr.setDispList(mnuBoardDisp);
    m_77c = 0;
    m_spr.m_active = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001adc6c

int MnuBoard2D::update(BedPad *pad)
{
    (void)pad;
    m_spr.update();
    if (m_system->m_scene.getFadeState() > 4)
        g_sequence.m_bits[1] |= MNUBOARD_BIT_SHOWN;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Mode 0: once the script clears the hold bit, the close sound and, when
// the fade is complete, 1 (m_result). Steps 0 / 1 go straight to 3.
// ROR 0x001adcd8

int MnuBoard2D::execMainFlag(BedPad *pad)
{
    (void)pad;
    switch (m_main_step) {
    case 0:
        m_main_step = 1;
        m_result = 0;
        /* fall through */
    case 1:
        m_main_step = 3;
        break;
    case 4:
        if (m_system->m_scene.getFadeState() < 5)
            return 0;
        m_main_step = 0;
        return m_result;
    default:
        break;
    }
    if ((g_sequence.m_bits[1] & MNUBOARD_BIT_HOLD) == 0) {
        m_main_step = 4;
        m_result = 1;
        g_sndMan.playSe(4);
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Mode 1: fade in over 15 frames (step 0), then triangle (pad bit 13)
// fades out; once that fade is complete, a fade in again and 1. ROM:
// exec then starts its own fade out.
// ROR 0x001addd0

int MnuBoard2D::execMainPad(BedPad *pad)
{
    switch (m_main_step) {
    case 0:
        m_main_step = 1;
        m_system->m_scene.fadeIn(0xf, 7, 0);
        /* fall through */
    case 1:
        m_main_step = 3;
        break;
    case 4:
        if (m_system->m_scene.getFadeState() < 5)
            return 0;
        m_system->m_scene.fadeIn(0xf, 7, 0);
        return 1;
    default:
        break;
    }
    if ((pad->m_trig >> 13 & 1) != 0) {
        m_system->m_scene.fadeOut(0xf, 7, 0);
        m_main_step = 4;
    }
    return 0;
}
