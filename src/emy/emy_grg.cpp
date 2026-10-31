/* ==========================================================================
 *  emy/emy_grg.cpp
 *
 *  GrgMain (enemy type 9) and its outs (0x153e40..0x155d54). The usual
 *  enemy loop: decide picks an input of the transition table (only inputs
 *  the current state's row has), exec moves the state graph by it, after
 *  switches outs on a change and runs the current one. Unlike the fighting
 *  enemies it reads no notify bits: it roams rooms 0x84..0x8b by the room
 *  tables below, hides in their fchrs and reappears elsewhere.
 * ======================================================================== */
#include "emy/emy_grg.h"

#include <math.h>
#include <string.h>

#include "dog/dogactor.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "sct/sctlayer.h"
#include "sequence/sequence.h"
#include "task/tsk_chr.h"
#include "task/tsk_man.h"
#include "utl/utlrand.h"

static const float GRG_PI   = 0x1.921fb4p+1f;   /* 0x40490fda */
static const float GRG_PI_2 = 0x1.921fb4p+0f;   /* 0x3fc90fda */
static const float GRG_PI_4 = 0x1.921fb4p-1f;   /* 0x3f490fda */

/* The script globals it keeps (see the header). */
enum
{
    GRG_GLOBAL_ACTIVE = 0x53,
    GRG_GLOBAL_AREA   = 0x54,
    GRG_GLOBAL_55     = 0x55,
};

/* GrgOutDisappear: the reappearance points by room, x y z and the rot y (0x2efc98). */
static const float s_grg_appear[9][8][4] = {
    {
        { -350.0f, 0.0f, 400.0f, -GRG_PI_2 },  // 0x002efc98
        { -350.0f, 0.0f, 100.0f, -GRG_PI_2 },  // 0x002efca8
        { -350.0f, 0.0f, -350.0f, -GRG_PI_2 },  // 0x002efcb8
        { 250.0f, 0.0f, -200.0f, 0.0f },  // 0x002efcc8
        { 200.0f, 0.0f, -725.0f, -GRG_PI_2 },  // 0x002efcd8
        { 250.0f, 0.0f, 300.0f, GRG_PI },  // 0x002efce8
        { -160.0f, 0.0f, -500.0f, GRG_PI },  // 0x002efcf8
        { 600.0f, 0.0f, -500.0f, GRG_PI },  // 0x002efd08
    },
    {
        { 600.0f, 0.0f, -400.0f, 0.0f },  // 0x002efd18
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efd28
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efd38
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efd48
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efd58
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efd68
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efd78
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efd88
    },
    {
        { -160.0f, 0.0f, -400.0f, 0.0f },  // 0x002efd98
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efda8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efdb8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efdc8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efdd8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efde8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efdf8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efe08
    },
    {
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efe18
        { -250.0f, 0.0f, -350.0f, GRG_PI_2 },  // 0x002efe28
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efe38
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efe48
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efe58
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efe68
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efe78
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efe88
    },
    {
        { 250.0f, 0.0f, -300.0f, GRG_PI },  // 0x002efe98
        { 250.0f, 0.0f, 200.0f, 0.0f },  // 0x002efea8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efeb8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efec8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efed8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efee8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efef8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002eff08
    },
    {
        { -250.0f, 0.0f, 100.0f, GRG_PI_2 },  // 0x002eff18
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002eff28
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002eff38
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002eff48
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002eff58
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002eff68
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002eff78
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002eff88
    },
    {
        { -250.0f, 0.0f, 400.0f, GRG_PI_2 },  // 0x002eff98
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002effa8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002effb8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002effc8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002effd8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002effe8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002efff8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0008
    },
    {
        { 300.0f, 0.0f, -725.0f, GRG_PI_2 },  // 0x002f0018
        { 550.0f, 0.0f, -400.0f, -GRG_PI_2 },  // 0x002f0028
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0038
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0048
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0058
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0068
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0078
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0088
    },
    {
        { 450.0f, 0.0f, -400.0f, GRG_PI_2 },  // 0x002f0098
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f00a8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f00b8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f00c8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f00d8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f00e8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f00f8
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0108
    },
};

/* GrgOutWalk::walkToPoint: the four points of each room (0x2f01d0). */
static const float s_grg_walk[9][4][4] = {
    {
        { 700.0f, 0.0f, -380.0f, 0.0f },  // 0x002f01d0
        { -200.0f, 0.0f, 50.0f, 0.0f },  // 0x002f01e0
        { 150.0f, 0.0f, 500.0f, 0.0f },  // 0x002f01f0
        { -180.0f, 0.0f, 450.0f, 0.0f },  // 0x002f0200
    },
    {
        { 710.0f, 0.0f, -730.0f, 0.0f },  // 0x002f0210
        { 520.0f, 0.0f, -650.0f, 0.0f },  // 0x002f0220
        { 660.0f, 0.0f, -640.0f, 0.0f },  // 0x002f0230
        { 690.0f, 0.0f, -520.0f, 0.0f },  // 0x002f0240
    },
    {
        { 0.0f, 0.0f, -720.0f, 0.0f },  // 0x002f0250
        { -40.0f, 0.0f, -640.0f, 0.0f },  // 0x002f0260
        { -160.0f, 0.0f, -610.0f, 0.0f },  // 0x002f0270
        { -45.0f, 0.0f, -520.0f, 0.0f },  // 0x002f0280
    },
    {
        { -700.0f, 0.0f, -250.0f, 0.0f },  // 0x002f0290
        { -430.0f, 0.0f, -200.0f, 0.0f },  // 0x002f02a0
        { -550.0f, 0.0f, -600.0f, 0.0f },  // 0x002f02b0
        { -440.0f, 0.0f, -700.0f, 0.0f },  // 0x002f02c0
    },
    {
        { 530.0f, 0.0f, -25.0f, 0.0f },  // 0x002f02d0
        { 160.0f, 0.0f, 60.0f, 0.0f },  // 0x002f02e0
        { 660.0f, 0.0f, 140.0f, 0.0f },  // 0x002f02f0
        { 430.0f, 0.0f, 260.0f, 0.0f },  // 0x002f0300
    },
    {
        { -410.0f, 0.0f, -30.0f, 0.0f },  // 0x002f0310
        { -570.0f, 0.0f, 40.0f, 0.0f },  // 0x002f0320
        { -540.0f, 0.0f, 200.0f, 0.0f },  // 0x002f0330
        { -710.0f, 0.0f, 160.0f, 0.0f },  // 0x002f0340
    },
    {
        { -460.0f, 0.0f, 510.0f, 0.0f },  // 0x002f0350
        { -590.0f, 0.0f, 410.0f, 0.0f },  // 0x002f0360
        { -650.0f, 0.0f, 350.0f, 0.0f },  // 0x002f0370
        { -430.0f, 0.0f, 330.0f, 0.0f },  // 0x002f0380
    },
    {
        { 290.0f, 0.0f, 190.0f, 0.0f },  // 0x002f0390
        { 390.0f, 0.0f, -550.0f, 0.0f },  // 0x002f03a0
        { 550.0f, 0.0f, -550.0f, 0.0f },  // 0x002f03b0
        { 630.0f, 0.0f, -400.0f, 0.0f },  // 0x002f03c0
    },
    {
        { 680.0f, 0.0f, -550.0f, 0.0f },  // 0x002f03d0
        { 550.0f, 0.0f, -560.0f, 0.0f },  // 0x002f03e0
        { 660.0f, 0.0f, -390.0f, 0.0f },  // 0x002f03f0
        { 590.0f, 0.0f, -350.0f, 0.0f },  // 0x002f0400
    },
};

/* GrgOutWalk::walkToFchr: the point by the fchr picked (0x2f0430). */
static const float s_grg_fchr[9][8][4] = {
    {
        { -250.0f, 0.0f, 400.0f, 0.0f },  // 0x002f0430
        { -250.0f, 0.0f, 100.0f, 0.0f },  // 0x002f0440
        { -250.0f, 0.0f, -350.0f, 0.0f },  // 0x002f0450
        { 250.0f, 0.0f, -300.0f, 0.0f },  // 0x002f0460
        { 300.0f, 0.0f, -725.0f, 0.0f },  // 0x002f0470
        { 250.0f, 0.0f, 400.0f, 0.0f },  // 0x002f0480
        { -160.0f, 0.0f, -400.0f, 0.0f },  // 0x002f0490
        { 600.0f, 0.0f, -400.0f, 0.0f },  // 0x002f04a0
    },
    {
        { 600.0f, 0.0f, -500.0f, 0.0f },  // 0x002f04b0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f04c0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f04d0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f04e0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f04f0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0500
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0510
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0520
    },
    {
        { -160.0f, 0.0f, -500.0f, 0.0f },  // 0x002f0530
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0540
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0550
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0560
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0570
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0580
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0590
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f05a0
    },
    {
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f05b0
        { -350.0f, 0.0f, -350.0f, 0.0f },  // 0x002f05c0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f05d0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f05e0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f05f0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0600
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0610
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0620
    },
    {
        { 250.0f, 0.0f, -200.0f, 0.0f },  // 0x002f0630
        { 250.0f, 0.0f, 300.0f, 0.0f },  // 0x002f0640
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0650
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0660
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0670
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0680
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0690
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f06a0
    },
    {
        { -350.0f, 0.0f, 100.0f, 0.0f },  // 0x002f06b0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f06c0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f06d0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f06e0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f06f0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0700
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0710
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0720
    },
    {
        { -350.0f, 0.0f, 400.0f, 0.0f },  // 0x002f0730
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0740
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0750
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0760
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0770
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0780
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0790
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f07a0
    },
    {
        { 200.0f, 0.0f, -725.0f, 0.0f },  // 0x002f07b0
        { 450.0f, 0.0f, -400.0f, 0.0f },  // 0x002f07c0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f07d0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f07e0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f07f0
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0800
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0810
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0820
    },
    {
        { 550.0f, 0.0f, -400.0f, 0.0f },  // 0x002f0830
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0840
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0850
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0860
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0870
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0880
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f0890
        { 0.0f, 0.0f, 0.0f, 0.0f },  // 0x002f08a0
    },
};

/* GrgOutWalk::walkToFchr: the fchr to pick by room, by utlRandRange(0, 64)
 * (0x2f08b0). The draw reaches 64, one past the row: the ROM reads the
 * next row's first byte, for the last room the byte after the table
 * (0x31, of the string at 0x2f0af0), kept as the last entry. */
static const char s_grg_pick[9 * 64 + 1] = {
    /* 0x002f08b0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                       2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7,
    /* 0x002f08f0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x002f0930 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x002f0970 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 0x002f09b0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
                       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 0x002f09f0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x002f0a30 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x002f0a70 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 0x002f0ab0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0x31,
};

/* GrgOutWalk::selectNext: by room, a draw 0..max under walk walks to a
 * point, under fchr goes to an fchr, else idles (0x2f0410). */
struct GrgRate
{
    char walk;
    char fchr;
    char max;
};
static_assert(sizeof(GrgRate) == 3, "GrgRate");
static const GrgRate s_grg_rate[9] = {
    { 60, 80, 100 },  // 0x002f0410
    { 30, 70, 100 },  // 0x002f0413
    { 20, 90, 100 },  // 0x002f0416
    { 50, 80, 100 },  // 0x002f0419
    { 50, 80, 100 },  // 0x002f041c
    { 30, 60, 100 },  // 0x002f041f
    { 30, 60, 100 },  // 0x002f0422
    { 30, 80, 100 },  // 0x002f0425
    { 30, 70, 100 },  // 0x002f0428
};

static u_int grgPCOid()
{
    ObjStat *pc = g_gamMan.m_pcPtr;
    return pc != NULL ? pc->m_oid : 0;
}

static u_int grgDogOid()
{
    ObjStat *dog = g_gamMan.m_dogPtr;
    return dog != NULL ? dog->m_oid : 0;
}

static int grgRoom()
{
    int room;
    g_gamMan.getStatus(5, &room);
    return room;
}

static int grgArea(ACHRstat *achr)
{
    int area;
    achr->getParam(GAM_PRM_O_AREA_NO, &area);
    return area;
}

/* chk_motion_end inline: the achr's motion ended sets the emy's motion
 * input. */
static bool grgMotionEnd(EmyOut *out)
{
    if (!g_gamMan.isAchrMotionEnd(out->m_oid))
        return false;
    out->m_emy->m_input_flg_motion = 1;
    return true;
}

/* The achr's move task is gone (it arrived) sets the emy's motion input. */
static bool grgMoveDone(EmyOut *out)
{
    if (g_taskMan.getTask(out->m_achr->m_task[0]) != 0)
        return false;
    out->m_emy->m_input_flg_motion = 1;
    return true;
}

/* The walk to pos (walkToPoint / walkToFchr inline it): the walk motions
 * (params 0x1100..0x1104), the old move task killed, a TskMoveTypeGen mode
 * 0xd to the point (its constructor aims at once, so pos may be a local). */
static void grgStartMove(EmyOut *out, const float *pos)
{
    ACHRstat *achr = out->m_achr;
    int walk = 2, left = 3, right = 4;
    float speed = 1.0f, turn = 1.0f;
    int *move_id = &achr->m_task[0];
    achr->setParam(0x1100, &walk);
    achr->setParam(0x1102, &left);
    achr->setParam(0x1101, &right);
    achr->setParam(0x1103, &speed);
    achr->setParam(0x1104, &turn);
    if (g_taskMan.getTask(*move_id) != 0) {
        g_taskMan.killTask(*move_id);
        *move_id = -1;
    }
    TskMoveTypeGen *task = new TskMoveTypeGen(out->m_oid, 0xd, (intptr_t)pos, 0, 1.0f,
                                              0x1.111110p-6f /* 0x3c888888, 1 / 60 */);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, move_id, 0x11);
}

static DogActor &grgActor(u_int oid)
{
    return g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(oid)];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00153e40 // callcheck: operator_new, EmyOut (new GrgOutXxx: the inline constructors call EmyOut::EmyOut)
// The ROM clears m_arm .. m_appear_count and sets m_arm twice.

void GrgMain::initOut()
{
    m_arm          = 1;
    m_appear_count = 0;
    m_in_area      = 0;
    m_seen         = 0;
    int area = -1;
    m_param->setParam(GAM_PRM_O_AREA_NO, &area);

    m_motion_list[2]  = 2;
    m_motion_list[3]  = 5;
    m_motion_list[7]  = 0xd;
    m_motion_list[9]  = 0x15;
    m_motion_list[10] = 0x13;
    m_238             = 1;
    m_motion_list[0]  = 1;
    m_motion_list[1]  = 1;
    m_motion_list[4]  = 1;
    m_motion_list[5]  = 1;
    m_motion_list[6]  = 1;
    m_motion_list[8]  = 1;
    m_state_flg[0]    = 1;
    m_state_flg[1]    = 1;
    m_state_flg[2]    = 0;
    m_state_flg[3]    = 1;
    m_state_flg[4]    = 0;
    m_state_flg[5]    = 0;
    m_state_flg[6]    = 0;
    m_released        = 0;
    m_arm             = 1;
    m_in_area         = 0;
    m_seen            = 0;
    m_appear_count    = 0;
    m_23c             = 0;

    m_out[1] = new GrgOutCatch(m_oid, m_param, this);
    m_out[2] = new GrgOutWalk(m_oid, m_param, this);
    m_out[3] = new GrgOutBitten(m_oid, m_param, this);
    m_out[4] = new GrgOutDisappear(m_oid, m_param, this);
    m_out[5] = new GrgOutEvent(m_oid, m_param, this);
    m_out[6] = new GrgOutTest(m_oid, m_param, this);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154030 // callcheck: ~EmyMain, operator_delete (the base destructor and the deleting variant are the compiler's)

GrgMain::~GrgMain()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154090 // callcheck: testInput7, isTargetInSight, isAreaCurrent, testInput1, testInput0
// Only while the motion input word is set. Every test runs in the order
// 8, 7, the sight edge (5 into sight, 6 out of it), 4, 1, 0 and the last
// true one wins.

void GrgMain::decide()
{
    if (m_decide_sw == 0 || m_input_flg_motion == 0 || m_req != -1)
        return;

    if (m_input[8] && m_released) {
        m_released = 0;
        m_req = 8;
    }
    if (m_input[7] && testInput7())
        m_req = 7;

    u_char seen = (u_char)isTargetInSight();
    if (m_seen == 0) {
        m_seen = seen;
        if (seen != 0 && m_input[5])
            m_req = 5;
    } else {
        m_seen = seen;
        if (seen == 0 && m_input[6])
            m_req = 6;
    }

    if (!isAreaCurrent()) {
        if (m_in_area && m_input[4]) {
            m_in_area = 0;
            m_req = 4;
        }
    } else if (m_in_area == 0) {
        m_in_area = 1;
    }

    if (m_input[1] && testInput1())
        m_req = 1;
    if (m_input[0] && testInput0()) {
        m_param->m_flags &= ~0x10000000u;
        m_param->m_flags &= ~0x80u;
        m_req = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154208
// No notify handling: an input the graph has no move for keeps the state.

void GrgMain::exec()
{
    m_changed = 0;
    if (m_req == -1)
        return;
    m_notify = 0;
    int state = m_map->next(m_req);
    m_seen = 0;
    if (state == 0) {
        m_map->m_cur = m_state;
    } else {
        m_state   = state;
        m_changed = 1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154270 // callcheck: getParam, getPos (its position is read and dropped)

void GrgMain::after()
{
    g_sequence.m_global[GRG_GLOBAL_AREA] = grgArea(m_param);
    float pos[4];
    m_param->getPos(pos);
    g_sequence.m_global[GRG_GLOBAL_55] = -1;

    EmyOut *out = m_emy_out;
    if (m_changed) {
        out->Term();
        selectOut();
        out = m_emy_out;
    }
    m_req = out->Exec();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154340
// The out starts before the inputs are read; the ROM returns 0 (the
// slot's value is never read).

void GrgMain::selectOut()
{
    if (m_state > 0 && m_state <= 6) {
        m_emy_out = m_out[m_state];
        m_emy_out->Init();
    }
    int n = m_trans->m_inputs;
    const char *row = m_trans->table() + m_state * n;
    // PORT: the ROM writes as many words as the table has inputs; the
    // host array holds 12 (EE 0x204..0x234).
    if (n > 12)
        n = 12;
    for (int i = 0; i < n; i++)
        m_input[i] = row[i] != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154408 // callcheck: getParam, getStatus (grgRoom / grgArea, inline in the ROM)

bool GrgMain::isPCInArea()
{
    if (g_gamMan.m_pcPtr == NULL)
        return false;
    int room = grgRoom();
    return room == grgArea(m_param);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154474 // callcheck: getParam, getStatus (grgRoom / grgArea, inline in the ROM)

bool GrgMain::isAreaCurrent()
{
    int room = grgRoom();
    return room == grgArea(m_param);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001544c8

int GrgMain::testInput7()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001544d0 // callcheck: getParam, getStatus (grgRoom / grgArea, inline in the ROM)

int GrgMain::testInput1()
{
    if (m_arm != 0)
        return 0;
    int room = grgRoom();
    if (room == 0x81) {
        m_arm = 1;
        return 1;
    }
    if (m_appear_count < 0x10)
        return 0;
    if (grgArea(m_param) == room || g_magSystem.getPCStateClass() == 10)
        return 0;
    m_arm = 1;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154584 // callcheck: getParam, getStatus (grgRoom / grgArea, inline in the ROM)

int GrgMain::testInput0()
{
    if (m_arm == 0 || (u_int)(m_238 - 2) <= 1)
        return 0;
    int room = grgRoom();
    if (room != grgArea(m_param))
        return 0;
    if (m_238 == 1) {
        float range;
        m_param->getParam(0x5f, &range);
        if (!emyChkInSight(range, GRG_PI, 1, grgPCOid(), m_param))
            return 0;
    }
    m_arm = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154668
// Both tests run (no short cut in the ROM).

bool GrgMain::isTargetInSight()
{
    if (!isPCInArea())
        return false;
    float range;
    m_param->getParam(0x5f, &range);
    int pc  = emyChkInSight(range, GRG_PI, 1, grgPCOid(), m_param);
    int dog = emyChkInSight(range, GRG_PI, 1, grgDogOid(), m_param);
    return pc != 0 || dog != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154734

int GrgMain::testView()
{
    return isTargetInSight();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154748

void GrgMain::releaseHold()
{
    m_param->m_flags &= ~0x10000000u;
    m_param->m_flags &= ~0x80u;
    m_released = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154780

void GrgOutCatch::Init()
{
    g_sequence.m_global[GRG_GLOBAL_ACTIVE] = 1;
    g_gamMan.hideObj(m_oid, 0);
    selectTarget(100);
    emyStartChase(10.0f, this, m_target);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001547e8 // callcheck: getPos, isAchrMotionEnd (the PC's and its own position; grgMotionEnd, inline in the ROM)
// Steps: 0 the chase (80, pi): on sight the catch motion by the secta
// cell under it (none: input 10) when the target is within pi / 4 ahead
// within 50 (input 9), else motion 0xe; a lost chase is input 10. 1 the
// catch, then motion 0xf held (2). 3 motion 0xe, then the chase again.

int GrgOutCatch::Exec()
{
    switch (m_step) {
    case 0:
    {
        int r = emyChkChase(80.0f, GRG_PI, this, m_target, 0);
        if (r == 2)
            return 10;
        if (r != 1)
            return -1;
        float pc[4], own[4];
        // The ROM reads the PC without a NULL test.
        g_gamMan.m_pcPtr->getPos(pc);
        m_achr->getPos(own);
        GamSecta *cell = g_gamMan.pickSectaCell(own, 0);
        if (cell == NULL)
            return 10;
        int motion;
        if ((cell->m_flags & 0x20000) == 0)
            motion = 0xd;
        else
            motion = pc[1] < own[1] ? 0xb : 0xc;
        if (!emyChkInSight(50.0f, GRG_PI_4, 0, m_target, m_achr)) {
            setMotion(0xe, 0);
            m_step = 3;
            return -1;
        }
        setMotion(motion, 0);
        m_step = 1;
        return 9;
    }
    case 1:
        if (grgMotionEnd(this)) {
            setMotion(0xf, 0x100);
            m_step = 2;
        }
        return -1;
    case 3:
        if (grgMotionEnd(this)) {
            emyStartChase(10.0f, this, m_target);
            m_step = 0;
        }
        return -1;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154a18

void GrgOutCatch::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154a34

void GrgOutBitten::Init()
{
    g_sequence.m_global[GRG_GLOBAL_ACTIVE] = 1;
    m_wait     = utlRandRange(0x3c, 0xb4);
    m_hit_back = 1;
    setMotion(0x13, 0x100);
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154a94 // callcheck: isAchrMotionEnd (grgMotionEnd, inline in the ROM)

int GrgOutBitten::Exec()
{
    if (m_step == 0) {
        if (--m_wait == 0) {
            setMotion(m_hit_back ? 0x14 : 0x16, 0);
            m_step = 1;
        }
    } else if (m_step == 1) {
        if (grgMotionEnd(this))
            return 9;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154b44

void GrgOutBitten::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154b60

void GrgOutDisappear::Init()
{
    g_sequence.m_global[GRG_GLOBAL_ACTIVE] = 1;
    g_gamMan.hideObj(m_oid, 1);
    m_count = 0;
    g_gamMan.getStatus(5, &m_room);
    m_point = 0;
    int wait = calcWait();
    m_step = 0;
    m_wait = wait;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154bd8 // callcheck: getStatus, isAchrMotionEnd (grgRoom / grgMotionEnd, inline in the ROM)
// m_room is never updated: once the room differs, the wait is drawn again
// every frame. The rot y is always the row's first point's.

int GrgOutDisappear::Exec()
{
    if (m_step == 0) {
        m_count++;
        int room = grgRoom();
        if (m_point != 0 || room != m_room) {
            m_point = 0;
            m_wait  = calcWait();
        }
        if (m_wait < m_count) {
            int idx = getRoomIndex();
            const float *p = s_grg_appear[idx][m_point];
            float pos[4] = { p[0], p[1], p[2], 0.0f };
            m_achr->setParam(GAM_PRM_O_POS, pos);
            float rot[4] = { 0.0f, s_grg_appear[idx][0][3], 0.0f, 0.0f };
            m_achr->setParam(GAM_PRM_O_ROT, rot);
            g_gamMan.hideObj(m_oid, 0);
            m_step = 1;
        }
        return -1;
    }
    if (m_step == 1 && grgMotionEnd(this)) {
        static_cast<GrgMain *>(m_emy)->m_appear_count++;
        return 9;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154d78 // callcheck: getStatus (grgRoom, inline in the ROM; the port reads the achr's position for room 0x89)
// Rooms 0x84..0x8b; any other is row 0.

int GrgOutDisappear::getRoomIndex()
{
    switch (grgRoom()) {
    case 0x84: return 0;
    case 0x85: return 2;
    case 0x86: return 1;
    case 0x87: return 4;
    case 0x88: return 3;
    case 0x89:
    {
        // PORT: the ROM compares a stack word nothing wrote (sp + 0x18)
        // with 255 -- the copy GrgOutWalk::getRoomIndex has reads the achr's
        // z there (against 260); the port reads the achr's z too.
        float pos[4];
        m_achr->getPos(pos);
        return pos[2] < 255.0f ? 5 : 6;
    }
    case 0x8a: return 7;
    case 0x8b: return 8;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154e20 // callcheck: sqrtf (the ROM's sqrt.s, its NaN fallback)

int GrgOutDisappear::calcWait()
{
    int idx = getRoomIndex();
    const float *p = s_grg_appear[idx][m_point];
    float pos[4];
    m_achr->getPos(pos);
    float dx = p[0] - pos[0];
    float dz = p[2] - pos[2];
    float d  = sqrtf(dx * dx + dz * dz) * 0.5f;
    return utlRandRange((int)(d * 0.7f), (int)(d * 1.3f));
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154f1c

void GrgOutDisappear::Term()
{
    termCommon();
    g_gamMan.hideObj(m_oid, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154f54

void GrgOutEvent::Init()
{
    g_sequence.m_global[GRG_GLOBAL_ACTIVE] = 0;
    g_gamMan.hideObj(m_oid, 1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154f88

int GrgOutEvent::Exec()
{
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154f90

void GrgOutWalk::Init()
{
    g_sequence.m_global[GRG_GLOBAL_ACTIVE] = 1;
    u_short room = (u_short)getRoomIndex();
    m_point = 0;
    m_room  = room;
    walkToPoint();
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00154fd8 // callcheck: getTask, isAchrMotionEnd, fptodp, dpcmp, dpadd, dpsub, dptofp, atan2f, getRotY, getActorNo, setActorMotion (grgMoveDone / grgMotionEnd inline, the double compares and emyWrapAngle)
// Steps: 0 walking to a point, 1 walking to the fchr, then the turn to it
// (motion 4 left / 3 right), 2 the motion by the fchr's facing against
// its own (0x23 more than pi / 2 apart, else 0x21) with the fchr's motion
// 1, 3 its end: hidden, input 3; 4 idling, 5 the idle's last motion.
// Steps 0, 5 (and 1, 2 by the move task) end with selectNext.

int GrgOutWalk::Exec()
{
    int ret = -1;
    int next;
    switch (m_step) {
    case 0:
        if (!grgMoveDone(this))
            return -1;
        next = selectNext();
        break;
    case 1:
    {
        if (!grgMoveDone(this))
            return -1;
        float to[4], own[4];
        g_gamMan.getFchr(m_fchr)->getPos(to);
        m_achr->getPos(own);
        float a = atan2f(to[0] - own[0], to[2] - own[2]);
        a = emyWrapAngle(a - grgActor(m_oid).getRotY());
        int motion = a >= 0.0f ? 4 : 3;
        // The ROM passes 1 in a3 too, which chrRotMotion does not read.
        chrRotMotion(fabsf(a), 1.0f, 0x1.000200p+15f /* 0x47000100, the object's heading */,
                     m_oid, motion, 0);
        next = 2;
        break;
    }
    case 2:
    {
        // The turn runs on m_task[1]: this waits for the move task only.
        if (!grgMoveDone(this))
            return -1;
        float a = grgActor(m_fchr).getRotY() - grgActor(m_oid).getRotY();
        a = emyWrapAngle(a);
        int motion;
        if ((double)a <= -1.5707963267948966 || (double)a >= 1.5707963267948966)
            motion = 0x23;
        else
            motion = 0x21;
        setMotion(motion, 0);
        // The ROM also passes the fchr's oid in t1, which setActorMotion does not read.
        g_magSystem.m_scene.setActorMotion(0x1.11112ep-6f /* 0x3c888897 */,
                                           g_gamMan.getActorNo(m_fchr), 1, 0, 0);
        next = 3;
        break;
    }
    case 3:
        if (!grgMotionEnd(this))
            return -1;
        ret = 3;
        g_gamMan.hideObj(m_oid, 1);
        next = 6;
        break;
    case 4:
    {
        if (!grgMotionEnd(this))
            return -1;
        u_char left = m_idle--;
        if (left != 1) {
            setMotion(utlRandRange(0x2a, 0x2b), 0);
            return -1;
        }
        setMotion(0x29, 0);
        next = 5;
        break;
    }
    case 5:
        if (!grgMotionEnd(this))
            return -1;
        next = selectNext();
        break;
    default:
        return -1;
    }
    m_step = next;
    return ret;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00155498

int GrgOutWalk::selectNext()
{
    const GrgRate &rate = s_grg_rate[m_room];
    char draw = (char)utlRandRange(0, rate.max);
    if (draw < rate.walk) {
        if (walkToPoint())
            return 0;
        return startIdle();
    }
    if (draw >= rate.fchr)
        return startIdle();
    if (walkToFchr())
        return 1;
    if (walkToPoint())
        return 0;
    return startIdle();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015558c // callcheck: getParam (grgArea, inline in the ROM)

int GrgOutWalk::getRoomIndex()
{
    int area = grgArea(m_achr);
    float pos[4];
    m_achr->getPos(pos);
    switch (area) {
    case 0x84: return 0;
    case 0x85: return 2;
    case 0x86: return 1;
    case 0x87: return 4;
    case 0x88: return 3;
    case 0x89: return pos[2] < 260.0f ? 5 : 6;
    case 0x8a: return 7;
    case 0x8b: return 8;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00155648 // callcheck: operator_new, TskMoveTypeGen, addTask, getTask, killTask, setParam (grgStartMove, inline in the ROM)

int GrgOutWalk::walkToPoint()
{
    int i = utlRandRange(0, 2);
    if ((signed char)m_point <= (signed char)i)
        i++;
    m_point = (u_char)i;
    const float *p = s_grg_walk[m_room][i];
    float to[4] = { p[0], p[1], p[2], p[3] };
    float own[4];
    m_achr->getPos(own);
    if (!emyCanReach(to, own))
        return 0;
    grgStartMove(this, to);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0015582c // callcheck: operator_new[], operator_delete[], operator_new, TskMoveTypeGen, addTask, getTask, killTask, setParam (the 0x40 list nodes; grgStartMove, inline in the ROM)
// The fchrs of type 9 within 1000; the pick counts only those with an
// oid. Fewer than the pick: the ROM reads the list head's unset data (the
// port: none).

int GrgOutWalk::walkToFchr()
{
    SeqPool<FCHRstat *> fchrs;
    fchrs.build(new SeqNode<FCHRstat *>[0x40], 0x40);
    // PORT: the ROM passes a stack position nothing wrote; the port
    // searches around the enemy.
    float pos[4];
    m_achr->getPos(pos);
    g_gamMan.collectFchrsNear(pos, 1000.0f, &fchrs, 9, 0, 0);

    int pick = s_grg_pick[m_room * 64 + utlRandRange(0, 0x40)];
    FCHRstat *found = NULL;
    int n = 0;
    for (SeqNode<FCHRstat *> *node = fchrs.m_used.m_head.m_next; node != &fchrs.m_used.m_head;
         node = node->m_next) {
        if (node->m_data->m_oid == 0)
            continue;
        if (n == pick) {
            found = node->m_data;
            break;
        }
        n++;
    }
    m_fchr = found != NULL ? found->m_oid : 0;
    if (m_fchr == 0) {
        delete[] fchrs.m_node;
        return 0;
    }

    const float *p = s_grg_fchr[m_room][pick];
    float to[4] = { p[0], p[1], p[2], p[3] };
    grgStartMove(this, to);
    delete[] fchrs.m_node;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00155bb4

int GrgOutWalk::startIdle()
{
    m_idle = (u_char)utlRandRange(2, 6);
    setMotion(0x28, 0);
    return 4;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00155bf8

void GrgOutWalk::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00155c14

void GrgOutTest::Init()
{
    g_sequence.m_global[GRG_GLOBAL_ACTIVE] = 1;
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00155c2c // callcheck: isAchrMotionEnd (grgMotionEnd, inline in the ROM)

int GrgOutTest::Exec()
{
    switch (m_step) {
    case 0:
        setMotion(0x15, 0);
        m_step = 1;
        break;
    case 1:
        if (grgMotionEnd(this)) {
            setMotion(0x16, 0);
            m_step = 2;
        }
        break;
    case 2:
        if (grgMotionEnd(this)) {
            setMotion(0x15, 0);
            m_step = 1;
        }
        break;
    case 3:
        if (grgMotionEnd(this))
            m_step = 1;
        break;
    }
    return -1;
}
