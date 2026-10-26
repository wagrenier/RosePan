/* ==========================================================================
 *  mnu/mnutitle.cpp
 *
 *  The title screen (0x1a90ec..0x1ab474; RoR names no file here: mnu/ and
 *  the name from the class and data/mnurpk/mnutitle.rpk, between the save
 *  screen mnu/mnusvld.cpp and the options' MnuOpt). See mnu/mnutitle.h.
 *
 *  PORT: the pattern table is the ROM's .data words as they are (an EE
 *  image, as in spr/s2d.cpp): the display list and the anim / move tables
 *  point into it by EE address, SprMan::eePtr resolves them against
 *  MNUTITLEPATTERN_EE. Writable: SprMan writes the anim counts back.
 * ======================================================================== */
#include "mnu/mnutitle.h"

#include <string.h>

#include "bed/bedpad.h"
#include "game/gamman.h"
#include "game/gamsnd.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "sequence/sequence.h"
#include "snd/sndman.h"
#include "spr/s2d.h"

/* ---- data ---- */

/* The title's textures in data/mnurpk/mnutitle.rpk (data 0x2a7798). */
static SprTexFile mnuTitleTexList[8] = {
    { 0, "titlebg.tm2", "ti_bg_tcc", 0x8800, 1, NULL },      // 0x002a7798
    { 1, "titlebg2.tm2", "ti_bg2_tcc", 0x100040, 1, NULL },  // 0x002a77b0
    { 2, "ti_06.tm2", "ti_06_tcc", 0x8800, 1, NULL },        // 0x002a77c8
    { 3, "sce.tm2", "ti_07_tcc", 0x8800, 1, NULL },          // 0x002a77e0
    { 4, "punch.tm2", "ti_08_tcc", 0x8800, 1, NULL },        // 0x002a77f8
    { 5, "siro.tm2", "ti_09_tcc", 0x8800, 1, NULL },         // 0x002a7810
    { 6, "keikoku.tm2", "ti_11_tcc", 0x8800, 1, NULL },      // 0x002a7828
    { -1, "", "", 0, 0, NULL },                              // 0x002a7840
};

static const char mnuTitleRpk[] = "data/mnurpk/mnutitle.rpk";   /* rodata 0x2f9978 */
static const char mnuTitleStream[] = "s100000.str";            /* rodata 0x2f9900 */

/* The title's pattern table (data 0x2a7858..0x2a80c0): EE image, see the file comment. */
static u_int mnuTitlePattern[538] = {
    0xffffffff, 0x00000000, 0x00000000, 0x00000100, 0x00000100, 0x00000003, 0x00000000, 0x00000000,  // 0x002a7858
    0x00002000, 0x00001c00, 0x00000004, 0x00000000, 0x00000000, 0x00002000, 0x00001c00, 0x00000005,  // 0x002a7878
    0x00000000, 0x00000000, 0x00002000, 0x00001c00, 0x00000006, 0x00000000, 0x00000000, 0x00002000,  // 0x002a7898
    0x00001c00, 0x00000000, 0x00000000, 0x00000000, 0x00002000, 0x00001c00, 0x00000001, 0x00000000,  // 0x002a78b8
    0x00000000, 0x00002000, 0x00001c00, 0x00000002, 0x00000800, 0x00000000, 0x00000a80, 0x00000180,  // 0x002a78d8
    0x00000002, 0x00000800, 0x00000180, 0x00000a80, 0x00000180, 0x00000002, 0x00000800, 0x00000300,  // 0x002a78f8
    0x00000a80, 0x00000180, 0x00000002, 0x00000000, 0x00000000, 0x00000480, 0x000001a0, 0x00000002,  // 0x002a7918
    0x00000000, 0x000004e0, 0x00000480, 0x000001a0, 0x00000002, 0x00000000, 0x000009c0, 0x00000480,  // 0x002a7938
    0x000001a0, 0x00000002, 0x00000000, 0x000001a0, 0x00000480, 0x000001a0, 0x00000002, 0x00000000,  // 0x002a7958
    0x00000680, 0x00000480, 0x000001a0, 0x00000002, 0x00000000, 0x00000b60, 0x00000480, 0x000001a0,  // 0x002a7978
    0x00000002, 0x00000000, 0x00000340, 0x00000480, 0x000001a0, 0x00000002, 0x00000000, 0x00000820,  // 0x002a7998
    0x00000480, 0x000001a0, 0x00000002, 0x00000000, 0x00000d00, 0x00000480, 0x000001a0, 0x00000002,  // 0x002a79b8
    0x00000480, 0x00000000, 0x00000380, 0x00000190, 0x00000002, 0x00000480, 0x000004b0, 0x00000380,  // 0x002a79d8
    0x00000190, 0x00000002, 0x00000480, 0x00000960, 0x00000380, 0x00000190, 0x00000002, 0x00000480,  // 0x002a79f8
    0x00000190, 0x00000380, 0x00000190, 0x00000002, 0x00000480, 0x00000640, 0x00000380, 0x00000190,  // 0x002a7a18
    0x00000002, 0x00000480, 0x00000af0, 0x00000380, 0x00000190, 0x00000002, 0x00000480, 0x00000320,  // 0x002a7a38
    0x00000380, 0x00000190, 0x00000002, 0x00000480, 0x000007d0, 0x00000380, 0x00000190, 0x00000002,  // 0x002a7a58
    0x00000480, 0x00000c80, 0x00000380, 0x00000190, 0x00000005, 0xffffffff, 0x00000000, 0x00000000,  // 0x002a7a78
    0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002a7a98
    0x00000000, 0x00000000, 0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002a7ab8
    0x00000000, 0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002a7ad8
    0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005,  // 0x002a7af8
    0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0xffffffff,  // 0x002a7b18
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0xffffffff, 0x00000000,  // 0x002a7b38
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x03000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002a7b58
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000001e, 0x00000001, 0x00000000, 0x00000000,  // 0x002a7b78
    0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002a7b98
    0x00000000, 0x00000000, 0x0000001e, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0,  // 0x002a7bb8
    0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002a7bd8
    0x0000001e, 0x00000003, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000,  // 0x002a7bf8
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000001e, 0x00000004,  // 0x002a7c18
    0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000,  // 0x002a7c38
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000001e, 0x00000005, 0x00000000, 0x00000000,  // 0x002a7c58
    0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002a7c78
    0x00000000, 0x00000000, 0x0000001e, 0x00000006, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0,  // 0x002a7c98
    0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002a7cb8
    0x0000000a, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000,  // 0x002a7cd8
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0x00000001,  // 0x002a7cf8
    0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000002, 0x00000000,  // 0x002a7d18
    0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000003, 0x00000000, 0x00000000, 0x00000000,  // 0x002a7d38
    0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002a7d58
    0x00000000, 0x00000000, 0x0000000a, 0x00000007, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0,  // 0x002a7d78
    0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002a7d98
    0x00000005, 0x00000007, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000005,  // 0x002a7db8
    0x00000008, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000005, 0x00000009,  // 0x002a7dd8
    0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000,  // 0x002a7df8
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000007, 0x00000000, 0x00000000,  // 0x002a7e18
    0x00000000, 0xfffffff0, 0xfffffff0, 0x00000001, 0x00000008, 0x00000000, 0x00002000, 0x00000000,  // 0x002a7e38
    0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002a7e58
    0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x002a7a88, 0x00000000, 0xffffffff, 0x002a7ac0,  // 0x002a7e78
    0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x002a7b88, 0xffffffff,  // 0x002a7e98
    0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0x002a7bc0, 0xffffffff, 0xffffffff, 0x00000000,  // 0x002a7eb8
    0x00000000, 0xffffffff, 0x002a7bf8, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff,  // 0x002a7ed8
    0x002a7c30, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0x002a7c68, 0xffffffff,  // 0x002a7ef8
    0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0x002a7ca0, 0xffffffff, 0xffffffff, 0x00000000,  // 0x002a7f18
    0x00000000, 0xffffffff, 0x002a7cd8, 0x00000000, 0xffffffff, 0x002a7d10, 0xffffffff, 0xffffffff,  // 0x002a7f38
    0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x002a7cd8, 0x00000000, 0xffffffff, 0x002a7d10,  // 0x002a7f58
    0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x002a7cd8, 0x00000000,  // 0x002a7f78
    0xffffffff, 0x002a7d10, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0xffffffff,  // 0x002a7f98
    0x002a7cd8, 0x00000000, 0xffffffff, 0x002a7d10, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000,  // 0x002a7fb8
    0x00000000, 0xffffffff, 0x002a7cd8, 0x00000000, 0xffffffff, 0x002a7d10, 0xffffffff, 0xffffffff,  // 0x002a7fd8
    0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x002a7cd8, 0x00000000, 0xffffffff, 0x002a7d10,  // 0x002a7ff8
    0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x002a7cd8, 0x00000000,  // 0x002a8018
    0xffffffff, 0x002a7d10, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0xffffffff,  // 0x002a8038
    0x002a7d80, 0x00000000, 0xffffffff, 0x002a7db8, 0x00000000, 0xffffffff, 0x002a7e28, 0xffffffff,  // 0x002a8058
    0xffffffff, 0x00000000, 0x00000080, 0x00000080, 0x00000080, 0x00000080, 0x00000080, 0x00000080,  // 0x002a8078
    0x00000080, 0x00000000, 0x00000080, 0x00000080, 0x00000080, 0x00000028, 0x00000080, 0x00000080,  // 0x002a8098
    0x00000080, 0x00000080,  // 0x002a80b8
};
#define MNUTITLEPATTERN_EE 0x002a7858u

/* The title's display list: sprites 0..14 (data 0x2a80c0). 8 "press
 * start", 9..11 the three items, 12..14 execSelect's. */
static const SprDispDef mnuTitleDisp[16] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 16000 }, 0x002a7e80u },        // 0x002a80c0
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 16016 }, 0x002a7e80u },        // 0x002a80f4
    { 2, 0, 0, 0, 0, 1, 0, 0, 0, { 0, 0, 16016 }, 0x002a7ea8u },        // 0x002a8128
    { 3, 0, 0, 0, 0, 1, 0, 0, 0, { 0, 0, 16016 }, 0x002a7ec0u },        // 0x002a815c
    { 4, 0, 0, 0, 0, 1, 0, 0, 0, { 0, 0, 16016 }, 0x002a7ed8u },        // 0x002a8190
    { 5, 0, 0, 0, 0, 1, 0, 0, 0, { 0, 0, 16016 }, 0x002a7ef0u },        // 0x002a81c4
    { 6, 0, 0, 0, 0, 1, 0, 0, 0, { 0, 0, 16016 }, 0x002a7f08u },        // 0x002a81f8
    { 7, 0, 0, 0, 0, 1, 0, 0, 0, { 0, 0, 16016 }, 0x002a7f20u },        // 0x002a822c
    { 8, 0, 0, 0, 0, 1, 0, 0, 0, { 2752, 4736, 16320 }, 0x002a8050u },  // 0x002a8260
    { 9, 10, 0, 0, 0, 1, 0, 0, 0, { 3504, 4384, 16320 }, 0x002a7f38u }, // 0x002a8294
    { 10, 13, 0, 0, 0, 1, 0, 0, 0, { 3504, 4800, 16480 }, 0x002a7f38u }, // 0x002a82c8
    { 11, 16, 0, 0, 0, 1, 0, 0, 0, { 3504, 5216, 16640 }, 0x002a7f38u }, // 0x002a82fc
    { 12, 19, 0, 0, 0, 1, 0, 0, 0, { 3632, 4432, 16800 }, 0x002a7fd8u }, // 0x002a8330
    { 13, 22, 0, 0, 0, 1, 0, 0, 0, { 3632, 4832, 16800 }, 0x002a8000u }, // 0x002a8364
    { 14, 25, 0, 0, 0, 1, 0, 0, 0, { 3632, 5232, 16800 }, 0x002a8028u }, // 0x002a8398
    { -1, 0, 0, 0, 0, 1, 0, 1, 0, { 0, 0, 0 }, 0x002a7e80u },           // 0x002a83cc
};

/* 1 once the logos have run (execLogo then starts at "press start");
 * data 0x2a8400. */
static int mnuTitleLogoDone = 0;
/* The logo pictures execLogo shows, -1 ends (data 0x2a8408), and their
 * frame counts (data 0x2a8418). */
static const int mnuTitleLogoSpr[4] = { 2, 5, -1, 0 };
static const int mnuTitleLogoTime[4] = { 90, 150, -1, 0 };

/* "press start"'s fade colour: grey 0x80, alpha 0x28 (data 0x2999c0). */
static const int mnuTitleColorDim[4] = { 0x80, 0x80, 0x80, 0x28 };

/* The movie viewer's pages (data 0x2a8428). */
static const MnuTitleMoviePage mnuTitleMoviePage[8] = {
    { 7, "MV00_OPE#0#0MV01_TIT#0#0MV02_TIT#0#0MV03_TIT#0#0MV04_TIT#0#0MV05_TIT#0#0next    #0#0",
      { "mov/MV00_OPE.PSS", "mov/MV01_TIT.PSS", "mov/MV02_TIT.PSS", "mov/MV03_TIT.PSS",
        "mov/MV04_TIT.PSS", "mov/MV05_TIT.PSS", "" },
      { 0, 0, 0, 0, 0, 0, 2 },
      { { 640, 480 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 } } },  // 0x002a8428
    { 7, "front   #0#0MV06_TIT#0#0MV07_TIT#0#0MV08_TIT#0#0MV10_TIT#0#0MV11_TIT#0#0next    #0#0",
      { "", "mov/MV06_TIT.PSS", "mov/MV07_TIT.PSS", "mov/MV08_TIT.PSS", "mov/MV10_TIT.PSS",
        "mov/MV11_TIT.PSS", "" },
      { 1, 0, 0, 0, 0, 0, 2 },
      { { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 } } },  // 0x002a84a0
    { 7, "front   #0#0MV12_TIT#0#0MV01_OPE#0#0MV01_ANA#0#0MV02_MEZ#0#0MV02_HEY#0#0next    #0#0",
      { "", "mov/MV12_TIT.PSS", "mov/MV01_OPE.PSS", "mov/MV01_ANA.PSS", "mov/MV02_MEZ.PSS",
        "mov/MV02_HEY.PSS", "" },
      { 1, 0, 0, 0, 0, 0, 2 },
      { { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 } } },  // 0x002a8518
    { 7, "front   #0#0MV02_CHO#0#0MV02_LAS#0#0MV02_DEA#0#0MV03_ZAN#0#0MV03_TOU#0#0next    #0#0",
      { "", "mov/MV02_CHO.PSS", "mov/MV02_LAS.PSS", "mov/MV02_DEA.PSS", "mov/MV03_ZAN.PSS",
        "mov/MV03_TOU.PSS", "" },
      { 1, 0, 0, 0, 0, 0, 2 },
      { { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 } } },  // 0x002a8590
    { 7, "front   #0#0MV03_KAT#0#0MV03_SYU#0#0MV04_NIN#0#0MV04_KOB#0#0MV05_MEZ#0#0next    #0#0",
      { "", "mov/MV03_KAT.PSS", "mov/MV03_SYU.PSS", "mov/MV04_NIN.PSS", "mov/MV04_KOB.PSS",
        "mov/MV05_MEZ.PSS", "" },
      { 1, 0, 0, 0, 0, 0, 2 },
      { { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 } } },  // 0x002a8608
    { 7, "front   #0#0MV05_SOU#0#0MV05_NAG#0#0MV06_KIZ#0#0MV06_KI2#0#0MV07_MEZ#0#0next    #0#0",
      { "", "mov/MV05_SOU.PSS", "mov/MV05_NAG.PSS", "mov/MV06_KIZ.PSS", "mov/MV06_KI2.PSS",
        "mov/MV07_MEZ.PSS", "" },
      { 1, 0, 0, 0, 0, 0, 2 },
      { { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 } } },  // 0x002a8680
    { 7, "front   #0#0MV07_KOI#0#0MV08_HAT#0#0MV10_KOT#0#0MV11_TOK#0#0MV12_TAT#0#0next    #0#0",
      { "", "mov/MV07_KOI.PSS", "mov/MV08_HAT.PSS", "mov/MV10_KOT.PSS", "mov/MV11_TOK.PSS",
        "mov/MV12_TAT.PSS", "" },
      { 1, 0, 0, 0, 0, 0, 2 },
      { { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 } } },  // 0x002a86f8
    { 3, "front   #0#0MV12_NIN#0#0test#0#0",
      { "", "mov/MV12_NIN.PSS", "mov/test000.pss", NULL, NULL, NULL, NULL },
      { 1, 0, 0, 0, 0, 0, 0 },
      { { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 }, { 512, 448 } } },  // 0x002a8770
};

// ──────────────────────────────────────────────────────────────────────
// The members' constructors, the cursor counts; everything else starts
// zeroed (the object lives in the global scene).
// ROR 0x001a90ec; callcheck: MnuMcd, MnuOpt, MnuSvld, MnuTextList, SprMan (member constructors: SprMan's is implicit, the others are not written and their objects start zeroed)

MnuTitle::MnuTitle()
{
    m_svld.m_save = 0;
    m_open = 0;
    initCursors();
    m_init = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a9150; callcheck: operator_delete, ~MnuMcd, ~MnuOpt, ~MnuSvld, ~MnuTextList, ~SprMan (member destructors and the deleting variant: implicit on the host, or not written)

MnuTitle::~MnuTitle()
{
    close();
    if (m_init == 1)
        term();
}

// ──────────────────────────────────────────────────────────────────────
// Sets up the sprite manager (15 sprites, textures into tex) and opens.
// ROR 0x001a91f4

void MnuTitle::init(DogTexInfo *tex)
{
    g_magSystem.m_scene.m_22cc = 0;
    m_spr.init(15);
    m_spr.m_tex.m_tex_info = tex;
    m_tex = tex;
    m_system = MagSystem::m_current_system;
    open();
    m_init = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a9258

void MnuTitle::term()
{
    if (m_init != 0) {
        m_spr.release();
        close();
        m_init = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a9298

int MnuTitle::isInit()
{
    return m_init;
}

// ──────────────────────────────────────────────────────────────────────
// One frame: load and set up (modes 0..3), run the sub menu (4), close
// (6 / 8: m_result goes back to MagLifeMenuTitle).
// ROR 0x001a92a0

int MnuTitle::processPad(BedPad *pad)
{
    int stat[4];
    int r;

    if (m_open == 0)
        return -1;

    switch (m_mode) {
    case 0:
        m_spr.saveVolume();
        SprMan::volumeOff();
        g_gamMan.setOption(0, 4);
        g_gamMan.m_sndMan.resetVolume();
        m_cursor[0] = 0;
        m_cursor[1] = 0;
        m_mode = 1;
        m_cursor[2] = 0;
        return 0;
    case 1:
        m_mode = 2;
        loadTexture();
        return 0;
    case 2:
        m_mode = 3;
        setDisp();
        m_stream = -1;
        /* fall through */
    case 3:
        m_mode = 4;
        if (mnuTitleLogoDone == 1) {
            m_sub = 1;
            if (m_stream < 0) {
                m_stream = g_sndMan.playStreamStereo(mnuTitleStream);
                if (m_stream >= 0)
                    g_sndMan.setStreamLoop(m_stream, 1);
            }
        }
        g_gamMan.setOption(0, 4);
        break;
    case 4:
        break;
    case 7:
        return 0;
    case 6:
        m_mode = 8;
        g_gamMan.resetPcStat();
        /* fall through */
    default:        /* 5, 8 */
        r = m_result;
        m_spr.m_active = 0;
        m_mode = 0;
        m_step = 0;
        if (m_stream >= 0) {
            g_sndMan.stopAll(1);
            m_stream = -1;
        }
        m_spr.restoreVolume();
        g_bss_7a3d4c = 1;
        g_bss_7a3d48 = 1;
        g_gamMan.getStatus(0x10, stat);
        g_gamMan.setOption(0, stat[0]);
        return r;
    }

    switch (m_sub) {
    case 0:
        if (execLogo(pad) == 1)
            m_sub = 1;
        break;
    case 1:
        if (execMain(pad) == 1) {
            switch (m_result) {
            case 1:
            case 2:
                m_mode = 6;
                break;
            case 3:
                g_sequence.m_global[0x17] = 0x10004;    /* the load menu */
                m_sub = 3;
                g_gamMan.getStatus(0x10, stat);
                g_gamMan.setOption(0, stat[0]);
                break;
            case 4:
                g_sequence.m_global[0x17] = 0x10006;    /* the options */
                m_sub = 4;
                g_gamMan.getStatus(0x10, stat);
                g_gamMan.setOption(0, stat[0]);
                break;
            }
        }
        break;
    case 2:
    case 4:
        r = m_sub == 2 ? execMovieTest(pad) : execOption(pad);
        if (r == 1) {
            g_sequence.m_global[0x17] = 0;
            m_sub = 1;
            g_gamMan.setOption(0, 4);
        }
        break;
    case 3:
        r = execLoad(pad);
        if (r == 1) {
            m_sub = 1;
            if (m_stream < 0) {
                m_stream = g_sndMan.playStreamStereo(mnuTitleStream);
                if (m_stream >= 0)
                    g_sndMan.setStreamLoop(m_stream, 1);
            }
            g_gamMan.setOption(0, 4);
        } else if (r == 2) {
            g_sequence.m_global[0x17] = 0;
            m_mode = 6;
        }
        break;
    case 5:
        r = execMovie(pad);
        if (r == 1)
            m_sub = 1;
        else if (r == 2)
            m_mode = 6;
        break;
    case 6:
        r = execSelect(pad);
        if (r == 1) {
            m_mode = 6;
        } else if (r == 2) {
            m_sub = 1;
            g_gamMan.setOption(0, 4);
        }
        break;
    }
    update();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The sprites, and the load or option screen over them.
// ROR 0x001a972c

void MnuTitle::draw(I3dLayer *layer)
{
    m_layer = layer;
    if (layer != NULL) {
        m_spr.m_layer[0] = layer;
        m_spr.draw();
        if (m_sub == 3)
            m_svld.draw(layer);
        else if (m_sub == 4)
            m_opt.draw(layer);
    }
}

// ──────────────────────────────────────────────────────────────────────
// Loads data/mnurpk/mnutitle.rpk's textures (waits for the whole list).
// ROR 0x001a97b0

void MnuTitle::loadTexture()
{
    m_spr.m_tex.setList(mnuTitleTexList, mnuTitleRpk);
    while (m_spr.m_tex.update() == 0)
        ;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a97f4

int MnuTitle::open()
{
    if (m_open == 1)
        return -1;
    m_open = 1;
    m_mode = 0;
    m_sub = 0;
    m_4dec = 7;
    m_step = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a9828

int MnuTitle::close()
{
    if (m_open == 0)
        return -1;
    m_open = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a9844

int MnuTitle::initCursors()
{
    for (int i = 8; i >= 0; i--)
        m_cursor[i] = 0;
    m_cursor_num[1] = 5;
    m_cursor_num[2] = 2;
    m_cursor_num[0] = 3;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The sprites on the pattern table and their display list.
// ROR 0x001a988c

void MnuTitle::setDisp()
{
    m_system = MagSystem::m_current_system;
    m_spr.m_pattern = mnuTitlePattern;
    m_spr.m_pattern_ee = MNUTITLEPATTERN_EE;    // PC
    m_spr.setDispList(mnuTitleDisp);
    m_spr.m_active = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001a98dc

int MnuTitle::update()
{
    m_spr.update();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Sub menu 0, before the items: the memory card check and the logos (the
// first time only), CP_LOGO.PSS, the MV00_AD.PSS attract movie, then
// "press start" over the title stream for 0x708 frames (then the movie
// again). 1 when start is pressed.
// ROR 0x001a98fc

int MnuTitle::execLogo(BedPad *pad)
{
    MagScene &scene = m_system->m_scene;
    int t;

    switch (m_step) {
    case 0:
        m_cursor[1] = 0;
        m_timer = 0;
        sprite(6).m_state = 0;
        sprite(7).m_state = 0;
        sprite(8).m_state = 0;
        sprite(9).m_state = 0;
        sprite(10).m_state = 0;
        sprite(11).m_state = 0;
        m_step = 1;
        scene.fadeIn(7, 7, 0);
        if (mnuTitleLogoDone == 1) {
            m_step = 0x16;
            sprite(6).m_state = 1;
            sprite(8).m_state = 1;
            m_spr.setFade(8, 5, 10, mnuTitleColorDim, g_sprColorOpaque);
            return 0;
        }
        m_mcd.init(NULL);
        /* fall through */
    case 1:
        mnuTitleLogoDone = 1;
        m_mcd.processPad(pad);
        if (m_mcd.startCheck(0) != 1)
            return 0;
        m_step = 2;
        break;
    case 2:
        if (m_mcd.processPad(pad) != 1)
            return 0;
        m_step = 3;
        break;
    case 3:
        m_mcd.term();
        m_logo = 0;
        m_timer = 0;
        m_step = 4;
        break;
    case 4:
        if (++m_timer < 0x3d)
            return 0;
        scene.fadeIn(7, 7, 0);
        sprite(mnuTitleLogoSpr[m_logo]).m_state = 1;
        m_step = 5;
        m_timer = 0;
        break;
    case 5:
        if (++m_timer < mnuTitleLogoTime[m_logo])
            return 0;
        scene.fadeOut(7, 7, 0);
        m_timer = 0;
        m_step = 6;
        break;
    case 6:
        if (scene.getFadeState() < 5)
            return 0;
        sprite(mnuTitleLogoSpr[m_logo]).m_state = 0;
        m_timer = 0;
        m_logo++;
        if (mnuTitleLogoSpr[m_logo] == -1) {
            m_logo = 0;
            m_step = 7;
        } else {
            m_step = 4;
        }
        break;
    case 7:
        if (scene.getFadeState() < 5)
            return 0;
        sprite(6).m_state = 0;
        sprite(8).m_state = 0;
        m_step = 8;
        m_timer = 0;
        break;
    case 8:
        m_timer = 0;
        g_sequence.m_bits[1] |= 0x8000000;
        strcpy(g_rxMpeg.m_name, "CP_LOGO.PSS");
        g_rxMpeg.open();
        g_rxMpeg.play(g_rxMpeg.m_name, 0x200, 0x150);
        MagSystem::m_v_interval = 1;
        m_step = 9;
        /* fall through */
    case 9:
        if (g_rxMpeg.isReady() != 1)
            return 0;
        g_rxMpeg.run();
        m_timer = 0;
        m_step = 10;
        break;
    case 10:
        g_rxMpeg.checkNearEnd();
        if (g_rxMpeg.getStatus() != 1)
            return 0;
        g_rxMpeg.close();
        MagSystem::m_v_interval = 2;
        m_step = 0xf;
        m_timer = 0;
        g_sequence.m_bits[1] &= ~0x8000000u;
        break;
    case 0xf:
        m_step = 0x10;
        g_sequence.m_bits[1] |= 0x8000000;
        break;
    case 0x10:
        m_step = 0x11;
        break;
    case 0x11:
        strcpy(g_rxMpeg.m_name, "MV00_AD.PSS");
        g_rxMpeg.open();
        g_rxMpeg.play(g_rxMpeg.m_name, 0x200, 0x150);
        MagSystem::m_v_interval = 1;
        m_timer = 0;
        sprite(6).m_state = 0;
        sprite(8).m_state = 0;
        m_step = 0x12;
        /* fall through */
    case 0x12:
        if (g_rxMpeg.isReady() != 1)
            return 0;
        m_timer = 0;
        g_rxMpeg.run();
        g_sequence.m_bits[1] &= ~0x8000000u;
        m_step = 0x13;
        break;
    case 0x13:
        m_timer++;
        if (g_rxMpeg.getStatus() == 1) {
            /* the movie ended: "press start" */
            sprite(6).m_state = 1;
            m_spr.setFade(6, 1, 10, g_sprColorClear, g_sprColorOpaque);
            sprite(8).m_state = 1;
            m_spr.setFade(8, 5, 10, mnuTitleColorDim, g_sprColorOpaque);
            g_rxMpeg.close();
            MagSystem::m_v_interval = 2;
            m_step = 0x16;
            m_timer = 0;
            break;
        }
        if (g_rxMpeg.checkNearEnd() == 1) {
            g_sndMan.fadeOut(0x3c);
            m_step = 0x14;
            sprite(6).m_state = 1;
            m_spr.setFade(6, 1, 0x28, g_sprColorClear, g_sprColorOpaque);
            sprite(8).m_state = 1;
            t = 0x28;
        } else {
            if ((pad->m_trig >> 3 & 1) == 0)
                return 0;
            g_sndMan.fadeOut(0x14);
            m_step = 0x15;
            sprite(6).m_state = 1;
            m_spr.setFade(6, 1, 0x14, g_sprColorClear, g_sprColorOpaque);
            sprite(8).m_state = 1;
            t = 0x14;
        }
        m_spr.setFade(8, 5, t, mnuTitleColorDim, g_sprColorOpaque);
        m_timer = 0;
        break;
    case 0x14:
    case 0x15:
        t = ++m_timer;
        if (t < (m_step == 0x14 ? 0x3d : 0x15) && g_rxMpeg.getStatus() != 1)
            return 0;
        g_rxMpeg.close();
        MagSystem::m_v_interval = 2;
        g_sndMan.fadeIn(7);
        m_timer = 0;
        m_step = 0x16;
        break;
    case 0x16:
        m_step = 0x17;
        if (m_stream < 0) {
            m_stream = g_sndMan.playStreamStereo(mnuTitleStream);
            if (m_stream >= 0)
                g_sndMan.setStreamLoop(m_stream, 1);
        }
        /* fall through */
    case 0x17:
        if (++m_timer > 0x708) {
            m_timer = 0;
            m_step = 0x18;
            g_sndMan.fadeOut(7);
            scene.fadeOut(7, 7, 0);
            return 0;
        }
        if ((pad->m_trig >> 3 & 1) != 0) {
            m_step = 0x1c;
            sprite(8).m_color[0] = 0x80;
            sprite(8).m_color[1] = 0x80;
            sprite(8).m_color[2] = 0x80;
            sprite(8).m_color[3] = 0x80;
            m_spr.setFade(8, 0x100, 1, mnuTitleColorDim, g_sprColorOpaque);
            g_sndMan.playSe(4);
            m_timer = 0;
        }
        break;
    case 0x18:
        if (++m_timer > 7) {
            m_step = 0xf;
            if (m_stream >= 0) {
                g_sndMan.stopAll(1);
                m_stream = -1;
                g_sndMan.fadeIn(2);
                return 0;
            }
        }
        break;
    case 0x1c:
        if (m_timer++ > 0x14) {
            m_spr.fadeOutDefault(8, 6);
            m_spr.fadeOutDefault(6, 0xf);
            m_spr.fadeOutDefault(8, 6);
            m_timer = 0;
            m_step = 1;
            return 1;
        }
        return 0;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Sub menu 1: the three items (sprites 9..11; up / down, cross). m_result
// 2 / 3 / 4 for items 0 / 1 / 2, 1 when the script's bit 0x80 of
// g_sequence.m_bits[1] is clear; after 0x708 idle frames back to the
// movie (sub menu 0, step 0xf). 1 when the choice has faded out.
// ROR 0x001aa248

int MnuTitle::execMain(BedPad *pad)
{
    MagScene &scene = m_system->m_scene;
    u_int flags = 0;
    int   old = 0;
    int   r = 0;
    int   next;

    switch (m_step) {
    case 0:
        scene.fadeIn(7, 7, 0);
        m_step = 1;
        /* fall through */
    case 1:
        m_spr.fadeInSprite(7, 1, 0xf, 0, -1, g_sprColorClear, g_sprColorOpaque);
        m_step = 2;
        /* fall through */
    case 2:
        flags = 1;
        m_spr.fadeInSprite(9, 1, 0xf, 0, -1, g_sprColorClear, g_sprColorOpaque);
        m_spr.fadeInSprite(10, 1, 0xf, 0, -1, g_sprColorClear, g_sprColorOpaque);
        m_spr.fadeInSprite(11, 1, 0xf, 0, -1, g_sprColorClear, g_sprColorOpaque);
        m_idle = 0;
        m_step = 0xf;
        break;
    case 0xf:
        if (m_idle++ > 0x707) {
            m_timer = 0;
            g_sndMan.fadeOut(7);
            scene.fadeOut(7, 7, 0);
            m_step = 0x12;
            break;
        }
        old = m_cursor[0];
        if ((pad->m_dir_repeat & 0x10) != 0 && old > 0) {
            m_cursor[0] = old - 1;
            flags = 1;
            g_sndMan.playSe(1);
        }
        if ((pad->m_dir_repeat & 0x40) != 0 && m_cursor[0] < m_cursor_num[0] - 1) {
            m_cursor[0]++;
            flags = 1;
            g_sndMan.playSe(1);
        }
        if ((g_sequence.m_bits[1] & 0x80) == 0) {
            m_result = 1;
            m_step = 0x1c;
        }
        if ((pad->m_trig >> 14 & 1) == 0)
            break;
        flags |= 2;
        switch (m_cursor[0]) {
        case 0:  m_result = 2; break;
        case 1:  m_result = 3; break;
        case 2:  m_result = 4; break;
        default: m_result = 1; break;
        }
        next = 0x1f;
        if (m_cursor[0] != 0) {
            next = 0x1c;
            if (m_cursor[0] == 1)
                g_sndMan.fadeOut(7);
        }
        m_step = next;
        g_sndMan.playSe(4);
        break;
    case 0x12:
        if (++m_timer > 7) {
            m_step = 0xf;
            m_sub = 0;
            if (m_stream >= 0) {
                g_sndMan.stopAll(1);
                m_stream = -1;
                g_sndMan.fadeIn(2);
            }
            m_cursor[0] = 0;
            m_timer = 0;
            sprite(6).m_state = 0;
            sprite(7).m_state = 0;
            sprite(8).m_state = 0;
            sprite(9).m_state = 0;
            sprite(10).m_state = 0;
            sprite(11).m_state = 0;
            flags = 1;
            scene.fadeIn(7, 7, 0);
        }
        break;
    case 0x1c:
        m_step = 0x1d;
        scene.fadeOut(7, 7, 0);
        break;
    case 0x1d:
        if (scene.getFadeState() > 4) {
            if (m_cursor[0] == 1) {
                if (m_stream >= 0) {
                    g_sndMan.stopAll(1);
                    m_stream = -1;
                }
                g_sndMan.fadeIn(2);
            }
            r = 1;
            sprite(7).m_state = 0;
            sprite(9).m_state = 0;
            sprite(10).m_state = 0;
            sprite(11).m_state = 0;
            m_step = 0;
        }
        break;
    case 0x1f:
        m_spr.fadeOutDefault(9, 0x1e);
        m_spr.fadeOutDefault(10, 5);
        m_spr.fadeOutDefault(11, 5);
        m_timer = 0;
        m_step = 0x20;
        break;
    case 0x20:
        if (++m_timer > 0x1e)
            m_step = 0x21;
        break;
    case 0x21:
        m_step = 0;
        r = 1;
        break;
    }

    if (flags & 1) {
        /* the old item back to anim 0, the new one to anim 1 */
        m_spr.setAnim(old + 9, 0);
        sprite(old + 9).m_disp = 1;
        m_spr.setAnim(m_cursor[0] + 9, 1);
        sprite(m_cursor[0] + 9).m_disp = 1;
    }
    return r;
}

// ──────────────────────────────────────────────────────────────────────
// Sub menu 6: a three-way choice (sprites 12..14) stored as system
// parameter 0x1c (0 / 1 / 2). 1 after a choice, 2 after cancel.
// ROR 0x001aa7d0

int MnuTitle::execSelect(BedPad *pad)
{
    MagScene &scene = m_system->m_scene;
    u_int flags = 0;
    int   old = 0;
    int   r = 0;
    int   v[4];

    switch (m_step) {
    case 0:
        m_cursor_num[8] = 3;
        m_step = 0xf;
        m_cursor[8] = 0;
        m_spr.fadeInSprite(12, 1, 0xf, 0, -1, g_sprColorClear, g_sprColorOpaque);
        m_spr.fadeInSprite(13, 1, 0xf, 0, -1, g_sprColorClear, g_sprColorOpaque);
        flags = 1;
        m_spr.fadeInSprite(14, 1, 0xf, 0, -1, g_sprColorClear, g_sprColorOpaque);
        break;
    case 0xf:
        old = m_cursor[8];
        if ((pad->m_dir_repeat & 0x10) != 0 && old > 0) {
            m_cursor[8] = old - 1;
            flags = 1;
            g_sndMan.playSe(1);
        }
        if ((pad->m_dir_repeat & 0x40) != 0 && m_cursor[8] < m_cursor_num[8] - 1) {
            m_cursor[8]++;
            flags = 1;
            g_sndMan.playSe(1);
        }
        if ((g_sequence.m_bits[1] & 0x80) == 0) {
            m_result = 1;
            m_step = 0x1c;
        }
        if ((pad->m_trig >> 14 & 1) != 0) {
            flags |= 2;
            v[0] = m_cursor[8] == 0 ? 0 : m_cursor[8] == 2 ? 2 : 1;
            g_gamMan.setSysParam(0x1c, v);
            m_step = 0x1c;
            g_sndMan.playSe(4);
        }
        if ((pad->m_trig >> 13 & 1) != 0) {
            m_step = 0x1e;
            flags |= 2;
            g_sndMan.playSe(2);
        }
        break;
    case 0x1c:
        m_step = 0x1d;
        scene.fadeOut(7, 7, 0);
        break;
    case 0x1d:
        if (scene.getFadeState() > 4) {
            r = 1;
            sprite(7).m_state = 0;
            m_step = 0;
        }
        break;
    case 0x1e:
        m_step = 0x1f;
        m_spr.fadeOutDefault(12, 0xf);
        m_spr.fadeOutDefault(13, 0xf);
        m_spr.fadeOutDefault(14, 0xf);
        m_timer = 0;
        break;
    case 0x1f:
        if (++m_timer > 0xf)
            m_step = 0x20;
        break;
    case 0x20:
        r = 2;
        m_step = 2;
        break;
    }

    if (flags & 1) {
        m_spr.setAnim(old + 12, 0);
        sprite(old + 12).m_disp = 1;
        m_spr.setAnim(m_cursor[8] + 12, 1);
        sprite(m_cursor[8] + 12).m_disp = 1;
    }
    return r;
}

// ──────────────────────────────────────────────────────────────────────
// Sub menu 2 (a test mode): the memory card check, then R1 plays
// test000.pss on the scene's movie layer, R2 stops it, start leaves (1).
// ROR 0x001aabb4

int MnuTitle::execMovieTest(BedPad *pad)
{
    MagScene &scene = m_system->m_scene;
    S2dMovie &movie = m_system->m_scene.m_movie;

    switch (m_step) {
    case 0:
        m_cursor[2] = 0;
        sprite(6).m_state = 0;
        sprite(8).m_state = 0;
        scene.fadeIn(7, 7, 0);
        m_mcd.init(NULL);
        m_timer = 0;
        m_step = 1;
        /* fall through */
    case 1:
        m_mcd.processPad(pad);
        if (m_mcd.startCheck(0) != 1)
            return 0;
        m_step = 2;
        break;
    case 2:
        if (m_mcd.processPad(pad) != 1)
            return 0;
        m_step = 3;
        break;
    case 3:
        m_mcd.term();
        m_step = 4;
        m_logo = 0;
        scene.fadeOut(7, 7, 0);
        return 0;
    case 4:
        if (scene.getFadeState() < 5)
            return 0;
        sprite(6).m_state = 1;
        sprite(8).m_state = 1;
        scene.fadeIn(7, 7, 0);
        m_step = 0xf;
        break;
    case 0xf:
        if ((pad->m_repeat >> 11 & 1) != 0 && movie.getMovieStatus() == -1) {
            movie.playMovie("test000.pss");
            MagSystem::m_v_interval = 1;
        }
        if ((pad->m_repeat >> 9 & 1) != 0 && movie.getMovieStatus() == 0)
            movie.stopMovie();
        if (movie.getMovieStatus() == 1) {
            movie.closeMovie();
            MagSystem::m_v_interval = 2;
        }
        if ((pad->m_trig >> 3 & 1) != 0) {
            m_step = 0x1c;
            m_spr.setAnim(8, 2);
            g_sndMan.playSe(4);
            scene.fadeOut(7, 7, 0);
            m_timer = 0;
        }
        break;
    case 0x1c:
        if (scene.getFadeState() < 5)
            return 0;
        m_result = 1;
        m_step = 0;
        return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Sub menu 3: the load screen. 2 when it loaded a game, 1 back.
// ROR 0x001aaed0

int MnuTitle::execLoad(BedPad *pad)
{
    int r;

    switch (m_step) {
    case 0:
        m_cursor[3] = 0;
        m_timer = 0;
        m_svld.init(m_tex);
        m_step = 1;
        /* fall through */
    case 1:
        m_step = 0xf;
        return 0;
    case 0xf:
        if (m_svld.processPad(pad) == 0)
            return 0;
        r = 1;
        if (m_svld.m_loaded != 0)
            r = 2;
        m_svld.term();
        m_step = 0;
        return r;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Sub menu 4: the options. 1 when they close.
// ROR 0x001aaf98

int MnuTitle::execOption(BedPad *pad)
{
    switch (m_step) {
    case 0:
        m_cursor[4] = 0;
        m_timer = 0;
        m_opt.init(m_tex);
        m_step = 1;
        /* fall through */
    case 1:
        m_step = 0xf;
        return 0;
    case 0xf:
        if (m_opt.processPad(pad) == 0)
            return 0;
        m_opt.term();
        m_step = 0;
        return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Sub menu 5: the movie viewer, a text list per page (m_cursor[5] the
// page, [6] the row); circle leaves (1).
// ROR 0x001ab04c

int MnuTitle::execMovie(BedPad *pad)
{
    MagScene &scene = m_system->m_scene;
    S2dMovie &movie = m_system->m_scene.m_movie;
    int &page = m_cursor[5];
    int &sel = m_cursor[6];
    int r = 0;

    switch (m_step) {
    case 0:
        page = 0;
        sel = 0;
        m_timer = 0;
        scene.fadeIn(7, 7, 0);
        m_step = 1;
        /* fall through */
    case 1: {
        m_list.open();
        u_int mask = 0;
        for (int i = 0; i < mnuTitleMoviePage[page].m_num; i++)
            mask |= 1u << (i & 0x1f);
        m_list.m_y = 0x40;
        m_list.m_z = 2000;
        m_list.m_x = 0x40;
        m_list.setItems(mnuTitleMoviePage[page].m_items, mask);
        m_list.m_cur = sel;
        m_step = 0xf;
        break;
    }
    case 0xf:
        m_list.processPad(pad);
        sel = m_list.getSelect();
        if (sel >= 0) {
            m_list.endSelect();
            m_list.processPad(pad);
            m_list.close();
            switch (mnuTitleMoviePage[page].m_action[sel]) {
            case 0:
                strcpy(g_rxMpeg.m_name, mnuTitleMoviePage[page].m_movie[sel]);
                m_step = 0x12;
                break;
            case 1:
                if (page >= 0) {
                    page = page - 1;
                    sel = 0;
                    m_step = 1;
                }
                break;
            case 2:
                page = page + 1;
                sel = 0;
                m_step = 1;
                break;
            }
        }
        if ((pad->m_trig >> 13 & 1) == 0)
            return 0;
        m_list.endSelect();
        m_list.processPad(pad);
        m_list.close();
        m_step = 0x1c;
        break;
    case 0x12:
        g_rxMpeg.open();
        movie.playMovie(mnuTitleMoviePage[page].m_movie[sel]);
        if (mnuTitleMoviePage[page].m_size[sel][0] > 0x27f)
            m_system->resetDBuff384();
        MagSystem::m_v_interval = 1;
        movie.stopMovie();
        m_step = 0x16;
        break;
    case 0x16:
        if (movie.getMovieStatus() == 1) {
            movie.closeMovie();
            if (mnuTitleMoviePage[page].m_size[sel][0] > 0x27f)
                m_system->resetDBuff();
            MagSystem::m_v_interval = 2;
            m_step = 1;
        }
        if ((pad->m_trig >> 13 & 1) == 0)
            return 0;
        movie.closeMovie();
        if (mnuTitleMoviePage[page].m_size[sel][0] > 0x27f)
            m_system->resetDBuff();
        MagSystem::m_v_interval = 2;
        m_step = 1;
        break;
    case 0x1c:
        r = 1;
        break;
    }
    return r;
}
