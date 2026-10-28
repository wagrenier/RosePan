/* ==========================================================================
 *  lib/unassigned.h
 *
 *  Prototypes of RoR code whose source file is not decided yet (unassigned
 *  runs in ror_functions.txt), declared so reconstructed callers compile and
 *  stubgen can stub them into lib/unassigned_stubs.cpp. Move each block to
 *  its real header when a session defines its TU.
 * ======================================================================== */
#ifndef _LIB_UNASSIGNED_H
#define _LIB_UNASSIGNED_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "sdk/vu0_intrin.h"

/* SysTimer and sysInitManagers: system/system.cpp (the globals file). */
#include "system/system.h"

/* magBwnSeqCommand, magBwnHurt and Brown's other glue: mag/magscenecmd.h. */
#include "mag/magscenecmd.h"

/* ---- the task framework (unassigned@001e46b8) ---- */

/* CmnITask, CmnTaskMan, g_taskMan: task/tsk_man.h. */
#include "task/tsk_man.h"

/* EmyManager, EmyGroup, g_emyManager: emy/emy_manager.h (EmyMain, EmyOut:
 * emy/emy_main.h). */
#include "emy/emy_manager.h"

/* ---- the enemy classes (unassigned@00145020, one file each, not split yet) ----
 * EmyMain's eleven derived classes, newed by EmyManager::createEmy by type.
 * Their constructors are the ELF's linkonce XxxMain::XxxMain: EmyMain's
 * constructor, the vtable store, then the class's own init (initOut: it
 * builds the per-state EmyOut objects). layout: host -- polymorphic; the
 * EE sizes (createEmy's operator new) and offsets are in the comments. */
/* Type 0 (vtable from rtti.py; EE 0x274). layout: host -- polymorphic. */
class DwfMain : public EmyMain
{
public:
    DwfMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x00289810
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~DwfMain();                                                         // EE slot 1 (+0x08), ROR 0x0015c2a4
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x0015cdac
    int  GetType() { return 0; }                                       // EE slot 3 (+0x18), ROR 0x00289848
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x0015c710
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x0015cc84
    void after();                                                        // EE slot 6 (+0x30), ROR 0x0015cd30
    void initOut();                                                      // ROR 0x0015c304
};

/* Type 1 (vtable from rtti.py; EE 0x270). layout: host -- polymorphic. */
class PigMain : public EmyMain
{
public:
    PigMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x00289470
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~PigMain();                                                         // EE slot 1 (+0x08), ROR 0x0015a580
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x0015abc4
    int  GetType() { return 1; }                                       // EE slot 3 (+0x18), ROR 0x002894a8
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x0015a5e0
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x0015aa8c
    void after();                                                        // EE slot 6 (+0x30), ROR 0x0015ab48
    void initOut();                                                      // ROR 0x0015a17c
};

/* Type 2 (vtable from rtti.py; EE 0x270). layout: host -- polymorphic. */
class GotMain : public EmyMain
{
public:
    GotMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x002891b0
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~GotMain();                                                         // EE slot 1 (+0x08), ROR 0x00158798
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x00158dd8
    int  GetType() { return 2; }                                       // EE slot 3 (+0x18), ROR 0x002891e8
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x001587f8
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x00158ca4
    void after();                                                        // EE slot 6 (+0x30), ROR 0x00158d5c
    void initOut();                                                      // ROR 0x001583d4
};

/* Type 3 (vtable from rtti.py; EE 0x25c). layout: host -- polymorphic. */
class FshMain : public EmyMain
{
public:
    FshMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x00288668
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~FshMain();                                                         // EE slot 1 (+0x08), ROR 0x00155d54
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x001561bc
    int  GetType() { return 3; }                                       // EE slot 3 (+0x18), ROR 0x002886a0
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x00155db4
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x00156048
    void after();                                                        // EE slot 6 (+0x30), ROR 0x00156140
    void initOut();                                                      // ROR 0x00156620
};

/* Type 4 (vtable from rtti.py; EE 0x228). layout: host -- polymorphic. */
class BdsMain : public EmyMain
{
public:
    BdsMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x0028621c
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~BdsMain();                                                         // EE slot 1 (+0x08), ROR 0x00149eb0
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x0014a158
    int  GetType() { return 4; }                                       // EE slot 3 (+0x18), ROR 0x00286254
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x00149f10
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x0014a034
    void after();                                                        // EE slot 6 (+0x30), ROR 0x0014a0dc
    void initOut();                                                      // ROR 0x00149d18
};

/* Type 5 (vtable from rtti.py; EE 0x228). layout: host -- polymorphic. */
class BdlMain : public EmyMain
{
public:
    BdlMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x002867ec
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~BdlMain();                                                         // EE slot 1 (+0x08), ROR 0x0014b554
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x0014b7fc
    int  GetType() { return 5; }                                       // EE slot 3 (+0x18), ROR 0x00286824
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x0014b5b4
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x0014b6d8
    void after();                                                        // EE slot 6 (+0x30), ROR 0x0014b780
    void initOut();                                                      // ROR 0x0014b3bc
};

/* Type 6, HfmMain: emy/emy_hfm.h. */
#include "emy/emy_hfm.h"

/* Type 7 (vtable from rtti.py; EE 0x23c). layout: host -- polymorphic. */
class MrtMain : public EmyMain
{
public:
    MrtMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x00286e34
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~MrtMain();                                                         // EE slot 1 (+0x08), ROR 0x0014cef8
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x0014d240
    int  GetType() { return 7; }                                       // EE slot 3 (+0x18), ROR 0x00286e6c
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x0014cf58
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x0014d13c
    void after();                                                        // EE slot 6 (+0x30), ROR 0x0014d1c4
    void initOut();                                                      // ROR 0x0014cde4
};

/* Type 8 (vtable from rtti.py; EE 0xa80). layout: host -- polymorphic. */
class ClrMain : public EmyMain
{
public:
    ClrMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x00287244
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~ClrMain();                                                         // EE slot 1 (+0x08), ROR 0x0014de54
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x0014e224
    int  GetType() { return 8; }                                       // EE slot 3 (+0x18), ROR 0x0028727c
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x0014deb4
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x0014e0b4
    void after();                                                        // EE slot 6 (+0x30), ROR 0x0014e16c
    virtual void initOut();                                              // EE slot 7 (+0x38), ROR 0x0014dc7c
    int m_248;                  // ROR 0x248: EmyManager::getEmyHoldWord248
};

/* Type 9 (vtable from rtti.py; EE 0x244). layout: host -- polymorphic. */
class GrgMain : public EmyMain
{
public:
    GrgMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x00288058
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~GrgMain();                                                         // EE slot 1 (+0x08), ROR 0x00154030
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x00154340
    int  GetType() { return 9; }                                       // EE slot 3 (+0x18), ROR 0x00288090
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x00154090
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x00154208
    void after();                                                        // EE slot 6 (+0x30), ROR 0x00154270
    void initOut();                                                      // ROR 0x00153e40
    /* Forwards to 0x154668 (EmyManager::testEmyView). */
    int  testView();                                                     // ROR 0x00154734
    /* The achr loses flags 0x10000000 and 0x80; m_240 = 1. */
    void releaseHold();                                                  // ROR 0x00154748

    int m_240;                  // ROR 0x240
};

/* Type 10 (vtable from rtti.py; EE 0x258). layout: host -- polymorphic. */
class NorMain : public EmyMain
{
public:
    NorMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x00287868
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut(prm);
    }
    ~NorMain();                                                         // EE slot 1 (+0x08), ROR 0x00150df8
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x00151158
    int  GetType() { return 10; }                                       // EE slot 3 (+0x18), ROR 0x002878b0
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x00150e58
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x0015102c
    void after();                                                        // EE slot 6 (+0x30), ROR 0x001510dc
    void initOut(const _tagEnemyParam *prm);                             // ROR 0x00150b24
    int m_24c;                  // ROR 0x24c: EmyManager::getEmyHoldWord24c
};

/* TskEnemyMove::start's record (EE 0x60 on emyStartMove's stack; the word
 * index the task reads in the comments). layout: host -- the pointer. */
struct EmyMoveParam
{
    u_int  m_oid;               // [0]: the enemy
    int   *m_done;              // [1]: cleared, then the result (EmyOut::m_move_done)
    int    m_mode;              // [2]: the type's (0x2f1d50: 1 for types 6..10)
    float  m_agl;               // [3]: EmyMain::m_agl
    float  m_walk;              // [4]: the type's speed (0x2f1c90), times m_agl
    float  m_run;               // [5]: the chase's second speed (0x2f1c94; 0 for a move)
    float  m_dist;              // [6]: the type's pair (0x2f1d80: 45 / 60)
    float  m_angle;             // [7]: ... and 0.3 pi
    int    m_parsonality;       // [8]: EmyMain::m_parsonality (the task keeps it + 3)
    int    m_carry;             // [9]: a Got (type 2) carrying kind 0xa2e (chase only)
    u_int  m_target;            // [10]: the oid followed (chase), 0
    float  m_pos[3];            // [11]: the goal (move)
    int    _0e;                 // [14]: not set
    float  m_turn;              // [15]: the type's turn rate (0x2f1c98)
    int    _10;                 // [16]: not set
    u_int  m_kind[4];           // [17]: EmyMain::getMoveParam's words 0..3
    int    m_type;              // [21]: EmyMain::GetType
    u_int  m_kind4;             // [22]: ... and word 4
    float  m_range;             // [23]: added to the two radii (the stop distance)
};

/* TskEnemyTurn::start's record (emyStartTurn). layout: host -- the pointer. */
struct EmyTurnParam
{
    u_int  m_oid;               // [0]: the enemy
    u_int  m_target;            // [1]: the PC's or the dog's oid, 0 none
    int   *m_done;              // [2]: cleared, then the result (EmyOut::m_turn_done)
    int    m_type;              // [3]: EmyMain::GetType
    float  m_agl;               // [4]: EmyMain::m_agl
    float  m_arg;               // [5]: emyStartTurn's float (task +0x28)
};

/* The enemy move task (vtable 0x2e5040; unassigned@001fd424). layout: host --
 * polymorphic; nothing placed yet. */
struct TskEnemyMove : CmnITask
{
    /* Active (+4 = 1) toward the record's goal or target. */
    void start(const EmyMoveParam *prm);                                 // ROR 0x00200700
    /* Stops it: +4 = 0, its SE stopped, EmyManager's work word of its
     * oid cleared (EmyMain::InactiveMove). */
    void stop();                                                         // ROR 0x00202f10
};

/* The enemy turn task (unassigned@001fd424). layout: host -- polymorphic. */
struct TskEnemyTurn : CmnITask
{
    /* Active (+4 = 1) toward the record's target. */
    void start(const EmyTurnParam *prm);                                 // ROR 0x0020350c

    int m_active;               // ROR 0x04: start sets 1, EmyOut::termCommon 0
};

/* ChrMotionRef, the Tsk* script tasks, the chr* helpers: task/tsk_chr.h. */
#include "task/tsk_chr.h"

/* TskSndParam, TskSnd, TskCamera, scnChrRequest, scenePlayStart: task/tsk_scene.h. */
#include "task/tsk_scene.h"

/* 1 when the two strings are equal, a..z compared as A..Z, else 0
 * (unassigned@00106010, after utl/utlfile.cpp). */
int   utlStrEqualNoCase(const char *a, const char *b);                   // ROR 0x00106010
/* The vector helpers, utlConvSndId and the i3r wrappers: utl/utlmath.h. */
#include "utl/utlmath.h"
/* The heading of (x, z) in -pi..pi, 0 for a zero vector (unassigned@00216ad0). */
float utlAtan2(float x, float z);                                        // ROR 0x00217f10
/* Copies src to dst with upper case letters (the ctype table's bit 1)
 * lowered. */
void  utlStrToLower(char *dst, const char *src);                         // ROR 0x00218338
/* The heading of (x, z) (pi/2 - acos(x / len), negative for z < 0) in
 * -pi..pi and its length into *dist; 0 / 0 for a zero vector. */
float utlDirDist(float x, float z, float *dist);                          // ROR 0x002180e4
/* Copies src into dst, at most n chars; zero-terminates while room is
 * left (n = strlen: no terminator). */
void  utlStrNCopy(char *dst, const char *src, int n);                    // ROR 0x0021838c
/* The SE no of the enemy oid (0x163ea0) played at its position (0x16a594)
 * (unassigned@00164fc0). */
void emyPlaySE(u_int oid, int no);                                       // ROR 0x0016a8a4
extern int g_emyManagerTaskId;  /* bss 0x7b45c0: addTask's id for EmyManager (read by 0x1c58c0) */

/* Words main sets once the boot load is over; the game modes (0x19a824,
 * 0x1a92a0, 0x1b099c, 0x1bde58, 0x1a98fc) read and write them. Roles not
 * known yet. */
extern int g_bss_7a3d48;        /* bss 0x7a3d48: main sets 1 */
extern int g_bss_7a3d4c;        /* bss 0x7a3d4c: main sets 1 */
/* An index 1..13 (other values count as 9) that picks the save slot of the
 * sequence globals and bits (Sequence::saveGlobals); 0x172b30, 0x1af130,
 * 0x1b4488, 0x216ad0, 0x217260 read it too. Role not known yet. */
extern int g_bss_7a3210;        /* bss 0x7a3210 */

/* ---- the movie player: moved to snd/rxmpeg.h ---- */
#include "snd/rxmpeg.h"

/* ---- the file reader: moved to game/gamfile.h ---- */
#include "game/gamfile.h"

/* A font rasterizer (0x1beb70..0x1c0534, named in the sprman session; game/
 * by link order, before GamMan::init): draws a font file's 1- or 2-bit
 * glyphs into a 4- or 8-bit pixel buffer (a texture image). SprMan's text
 * sprites have one, MagScene's text windows one each (+0x2744). EE 0x6c.
 * layout: host -- the buffer and font pointers; EE offsets in comments. */
struct GamFont
{
    // ROR 0x001beb70
    GamFont()
        : m_00(0), m_04(0), m_dst(NULL), m_dst_w(0), m_dst_h(0), m_dst_bits(4),
          m_dst_size(0), m_color(0), m_30(0), m_34(2), m_char_w(0), m_char_h(0),
          m_first_code(0), m_font(NULL), m_fill(0x80), m_68(1)
    {
    }
    // ROR 0x001bebc4 (GCC 2 destructor: only the deleting variant)
    ~GamFont() {}

    /* Reads a font file's header (glyph cell, first code, tables). */
    void setFont(const void *font);                                      // ROR 0x001bebec
    /* The pixel buffer the glyphs go to; w & 0xff80, bits 4 or 8. 1 when
     * set. */
    int  setTarget(void *buf, u_int w, u_int h, int bits);               // ROR 0x001bedc0
    /* Fills the whole buffer with m_fill; 1 when there is one. */
    int  clear();                                                        // ROR 0x001beeb4
    /* Fills the rect with m_fill. */
    int  fillRect(int x, int y, int w, int h);                           // ROR 0x001bef24
    void drawChar(int x, int y, int code);                               // ROR 0x001bf024
    /* Scrolls the buffer by rows (< 0: up), filling the freed rows with
     * m_fill; nothing beyond +-m_dst_h. */
    void scroll(int rows);                                               // ROR 0x001bfe30
    /* m_fill = fill (| 0x80 for 8-bit targets while m_68 is set). */
    void setFill(u_int fill);                                            // ROR 0x001bff28
    void setFillMsb(int on);                                             // ROR 0x001bff5c
    void setColor(u_int color);                                          // ROR 0x001bff64
    /* The glyph of a (Shift-JIS) code, -1 when the font has none. */
    int  getGlyph(int code);                                             // ROR 0x001bffcc
    int  getGlyphLeft(int glyph);                                        // ROR 0x001c020c
    /* The glyph's width (proportional fonts), else m_char_w. */
    int  getGlyphWidth(int glyph);                                       // ROR 0x001c0254
    /* The width of a Shift-JIS string (a tail jump to 0x1c02f8). */
    int  getTextWidth(const char *text);                                 // ROR 0x001c047c
    /* A character's left offset, width and spacing (m_30 fixed, m_34
     * proportional); all 0 below 0x20. */
    void getCharMetrics(int code, int *left, int *width, int *extra);    // ROR 0x001c0078

    int            m_00;            // ROR 0x00
    int            m_04;            // ROR 0x04
    u_char        *m_dst;           // ROR 0x0c: setTarget's buffer
    int            m_dst_w;         // ROR 0x10
    int            m_dst_h;         // ROR 0x14
    int            m_dst_bits;      // ROR 0x18: 4 or 8
    int            m_dst_size;      // ROR 0x1c
    int            m_prop;          // ROR 0x24: the font has a width table
    int            m_prop_on;       // ROR 0x28
    int            m_color;         // ROR 0x2c: setColor
    int            m_30;            // ROR 0x30
    int            m_34;            // ROR 0x34
    int            m_char_w;        // ROR 0x38: the glyph cell (font header +0xe)
    int            m_char_h;        // ROR 0x3c: (+0xf), the line height
    int            m_first_code;    // ROR 0x44
    const u_char  *m_font;          // ROR 0x54: setFont's file
    int            m_fill;          // ROR 0x64: fillRect's colour
    int            m_68;            // ROR 0x68
};
static_assert(offsetof(GamFont, m_00) == 0x00, "m_00");

/* ---- the utl range after utl/utlgame.cpp (unassigned@00103edc) ---- */

/* The dog's params 0x1b / 0x1c through GamMan::get/setDogParam
 * (MagLifeBwn::calcHealth's ratio; reset tops 0x1b up). */
void utlSetDogParam1b(int v);                                            // ROR 0x00103edc
int  utlGetDogParam1b(void);                                             // ROR 0x00103f08
int  utlGetDogParam1c(void);                                             // ROR 0x00103f38
/* GamMan::setObjRadius(the dog's oid, r), then testHitAt(pos, the dog's
 * oid, 0, 1): non-zero on a hit, the record's word +0x10 into *hit. */
/* EmyManager::getEmyHoldWord248 / getEmyWord238 of the enemy oid. */
int  utlGetEmyHold(u_int oid);                                           // ROR 0x00103f68
int  utlGetEmyWord238(u_int oid);                                        // ROR 0x00103f88
/* EmyManager::getEmyWord3c(oid) == 7 / == 4. */
int  utlIsEmyState7(u_int oid);                                          // ROR 0x00103fac
int  utlIsEmyState4(u_int oid);                                          // ROR 0x00103fd8
int  utlTestDogHitAt(const float *pos, u_int *hit, float r);             // ROR 0x00104004
/* GamMan::getObjRadius of the dog (GamMan::m_dogPtr's oid, 0 without). */
float utlGetDogRadius(void);                                             // ROR 0x001040a0

/* GamMan::getObjParam(oid, 2): the object's kind (0x401 / 0x421 ...). */
int  utlGetObjKind(u_int oid);                                           // ROR 0x0010411c
void utlRandReset(void);                                                 // ROR 0x0010421c
/* A random integer in lo..hi (lo + 0x1041e4(hi - lo + 1)). */
int  utlRandRange(int lo, int hi);                                       // ROR 0x0010423c
/* (lo, hi, n): the mean of n draws over 0..2 (hi - lo) folded at hi - lo,
 * plus lo: values peak at hi. */
int  utlRandPeakHi(int lo, int hi, int n);                               // ROR 0x00104394
/* GamMan::setObj(the object at GamMan +0x22094, 0x40); both values of on
 * do the same in the ROM. */
void utlSetBwnObjFlag(int on);                                           // ROR 0x00104150
/* A random float in lo..hi in steps of (hi - lo) / 65536 -- 0.0 (not lo)
 * when the integer draw is 0. */
float utlRandRangeF(float lo, float hi);                                 // ROR 0x001044b8
/* lo + the mean of n draws over 0..hi - lo. */
int  utlRandMean(int lo, int hi, int n);                                 // ROR 0x0010426c
/* lo..hi, peaking at peak (the mean of n draws, folded). */
int  utlRandPeak(int lo, int hi, int peak, int n);                       // ROR 0x00104438

/* ---- the i3d library (unassigned@00216ad0 .. 0x237d80; its version string
 * "i3d_package_version 050121-stable-050826") ---- */

/* The context and the VRAM cache have their own files (i3d/i3dctx.h,
 * i3d/i3dvram.h); what they call in the rest of the library is below. */
#include "i3d/i3dctx.h"
#include "i3d/i3dlayer.h"

/* The vector / matrix module (and i3dSysInit / i3dPrintf / i3dFlushCache)
 * has its own file. */
#include "i3d/i3dmath.h"

/* The texture / CLUT objects and their GS transfers (i3dTexMakeUpload ...,
 * i3dPsmBits) have their own file. */
#include "i3d/i3dtex.h"

/* The draw sets, tags, register writes and primitive writers have their
 * own file. */
#include "i3d/i3dpacket.h"

/* An i3r item's vector at time t (unassigned@00225018): its data when its
 * child 0 has no data, else child 0's keys interpolated (0x225148). */
struct I3rItem;
void  i3rTrackGetValue(const I3rItem *track, float *out, float t);       // ROR 0x002251f0

/* A blend target's weight at time t (unassigned@00225018): its child 0's
 * curve (a type 0x58 item: keys, then slope / intercept pairs) at t when
 * it has one, else the target's own weight (its data). */
float i3rBlendGetWeight(float t, const I3rItem *target);                // ROR 0x00225108

/* The block VU1 gets for one blended slot (0x225498's first argument;
 * EE: five words). layout: host -- m_data is a pointer. */
struct I3rVertexRef
{
    const void *m_data;         // ROR 0x00: the qwords (or the VIF stream, m_kind 1)
    u_int   m_qwc;              // ROR 0x04
    u_int   m_count;            // ROR 0x08: the vertices
    u_int   m_kind;             // ROR 0x0c: the 0x33 block's kind
    u_int   m_blended;          // ROR 0x10: 1 = computed into buf (V4-32 qwords)
};

/* One blended slot (unassigned@00225018): blocks[0] the mesh's own, [k]
 * blend target k. Group flags & 2: buf = the sum of list's weights x their
 * targets (flags & 1: each target's per-vertex curves at t); flags & 1
 * alone: buf = the own block's curves at t; else ref = the own block.
 * Returns the end of what it wrote into buf. PC: the list in place of the
 * ROM's entries and count. */
struct I3rBlendList;
float *i3rBlendVertices(float t, I3rVertexRef *ref, float *buf, const I3rItem *blocks, u_int num_blocks,
                        const I3rBlendList *list, u_int flags);         // ROR 0x00225498

/* A layer's light set, fog and view (EE layer +0x82c / +0x830 / +0x834).
 * Opaque: built by 0x11262c over the scene light pool, i3dCtxSetFog and
 * i3dCtxCreateView. */
struct I3dLight;
struct I3dView;
struct MagViewParam;

/* The views (i3dCtxCreateView) have their own file. */
#include "i3d/i3dview.h"


/* The fog and light records have their own file. */
#include "i3d/i3denv.h"

/* The i3d model root (I3D_BIN, tag_I3rRoot_s in DogModel's symbols) and
 * its item tree have their own file. */
#include "i3d/i3rroot.h"

#include "i3d/i3ritem.h"
/* The model helpers (packets, textures, STQ entities, node transforms)
 * and I3dMatrixF4x4 have their own file. */
#include "i3d/i3rmodel.h"
/* The model draw (i3rBuildMeshPacket, i3rDrawModel) has its own file. */
#include "i3d/i3rdraw.h"
/* The material packets (i3rGetMaterialPacket, the layer TEST / ALPHA,
 * i3dMatGetScaleTrans) have their own file. */
#include "i3d/i3rmesh.h"

#include "i3d/i3croot.h"

struct I3cQuery;
struct MagSceneRayHit;
/* A cast's hit callback (dogModelHitNearest / dogModelHitCylinder);
 * arguments past the query are the collision module's (roles unverified). */
typedef int (*I3cHitFunc)(I3cQuery *q, void *a1, const u_short *a2, int a3, float *a4, float *a5);
/* A collision cast's request (built on the stack by DogModel's casts).
 * layout: host -- pointers. */
struct I3cQuery
{
    I3cRoot        *m_root;       // ROR 0x00: m_i3c_root
    sceVu0FMATRIX  *m_mat;        // ROR 0x04: the model's node matrices (m_mat_model)
    u_short        *m_colli_tbl;  // ROR 0x08: the collision node -> model node table
    int             m_group;      // ROR 0x0c: the group cast (-1 all); the callbacks copy it to the hit's m_50
    I3cHitFunc      m_func;       // ROR 0x10
    int             m_14;         // ROR 0x14: 0
    MagSceneRayHit *m_hit;        // ROR 0x18: the record the callback fills
    int             _1c;          // ROR 0x1c: not set
    int             m_20;         // ROR 0x20: 1 (castRayGroups leaves it unset)
};
static_assert(offsetof(I3cQuery, m_hit) >= 0x18, "m_hit");

/* The casts: a sphere (centre, w = radius), a ray (pos, dir) or a
 * cylinder (centre, w = radius; dir) under the query, with up to 64 node
 * matrices (DogModel passes unit ones). */
int   i3cCastSphere(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats); // ROR 0x002324b8
int   i3cCastRay(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats);    // ROR 0x00232508
int   i3cCastCylinder(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats); // ROR 0x002325a8
/* The hit callbacks (unassigned@00113ea8): keep the nearest hit. */
int   dogModelHitNearest(I3cQuery *q, void *a1, const u_short *a2, int a3, float *a4, float *a5); // ROR 0x00114034
int   dogModelHitCylinder(I3cQuery *q, void *a1, const u_short *a2, int a3, float *a4, float *a5); // ROR 0x001141c8


/* The i3d VU1 packet writers (strips, sprites, wire shapes). */
#include "i3d/i3dvu1pkt.h"
/* The i3m pose player. */
#include "i3d/i3mpose.h"



/* bss 0x7812ac, just below GamMan: mainInitGame passes it to
 * seqSetLanguage (which ignores it). Role not known yet. */
extern int g_bss_7812ac;

#include "mag/maglife.h"

struct DogTexInfo;
struct MagScene;

/* Scene members whose class has no name yet, named by their constructor's
 * address. layout: host -- only the words the scene touches. */
/* MagScene::setFade's record (six words; the scene keeps the current one
 * at +0x43700, kind -1 = none). */
struct MagFadeParam
{
    int m_kind;                 // ROR 0x00: 0 MagLifeFader to black, 4 to colour 0xff, 1..3 MagLifeFadeKey (the keyhole)
    int m_mode;                 // ROR 0x04: the fader's state to start in; 0 stops the fade
    int m_time;                 // ROR 0x08: frames
    int m_0c;                   // ROR 0x0c: role unknown (MagScene::update leaves it unset)
    int m_10;                   // ROR 0x10: role unknown (idem)
    int m_level;                // ROR 0x14: the fade object's level (MagScene::update passes 7)
};
static_assert(offsetof(MagFadeParam, m_level) == 0x14, "m_level");
static_assert(sizeof(MagFadeParam) == 0x18, "MagFadeParam is 0x18 bytes on the EE");

/* The keyhole fade (scene +0x41890, mag/maglifefadekey.cpp: the
 * kagiana_tcc texture of data/evntex/EVNTEX.RPK). layout: host -- a
 * MagLife. */
struct MagLifeFadeKey : MagLife
{
    /* Adds kagiana_tcc to tex (the int is 0 from MagScene::setFade). */
    void loadTexture(DogTexInfo *tex, int arg);                          // ROR 0x0012f2f0
    /* Releases the texture m_tex_no from theTexInfo. */
    void releaseTexture(DogTexInfo *theTexInfo);                         // ROR 0x0012f790
    void start(const MagFadeParam *p);                                   // ROR 0x0012f824
    /* Adds theName from data/evntex/EVNTEX.RPK to tex. */
    void loadTextureByName(DogTexInfo *tex, const char *theName);       // ROR 0x0012f518
    /* m_state = state, the time words = time, the magnify words 1. */
    void reset(int state, int time);                                     // ROR 0x0012f7f4
    /* The magnify word to magnify over time frames (<= 0: at once). */
    void setMagnify(float magnify, int time);                            // ROR 0x0012fc80

    u_char _28[0x40 - 0x28];
    int    m_tex_no;            // ROR 0x40: kagiana_tcc in the scene's table, -1 = not loaded
};
static_assert(offsetof(MagLifeFadeKey, m_tex_no) >= 0x40, "m_tex_no");
struct MagLife130da4 : MagLife  /* scene +0x418f0, constructor 0x130da4 */
{
    /* m_volume = volume (GamMan::setVolume's scaled master volume). */
    void setVolume(int volume);                                          // ROR 0x00130de0

    u_char _28[0x30 - 0x28];
    int    m_volume;            // ROR 0x30
    int    m_34;                // ROR 0x34: initRender copies the scene's +0x2910 into it
};
static_assert(offsetof(MagLife130da4, m_34) >= 0x34, "m_34");
struct DogActor;
/* The effect manager at scene +0x837e0, its EffParam, and Eff0: eff/effman.h. */
#include "eff/effman.h"
/* The secta world (SctWorld at scene +0x8542c, its SctLayers, GamSecta):
 * sct/sctlayer.h. */
#include "sct/sctlayer.h"

/* mnuInitPool 0x1944d0 / mnuTermPool 0x194548 (the mnu pool): mnu/mnubase.h. */
/* The closest points of segments p0 + s d0 and p1 + t d1 (s, t in 0..1,
 * either out pointer may be NULL); returns the squared distance. */
float i3dSegSegDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                     float *s, float *t);                                // ROR 0x00234050
/* The squared distance from p to the segment a + t d (t in 0..1, written
 * to *t when t is not NULL). */
float i3dPointSegDist2(const float *p, const float *a, const float *d, float *t); // ROR 0x002337e0

/* ---- the room table: wrd/wrdsrv.h (WrdSrv, g_wrdSrv) ---- */
#include "wrd/wrdsrv.h"

/* ---- the mnu/ menu objects (unassigned@00192ac0, @001a0950, @001a6830):
 * each MagLifeMenu* wrapper (mag/maglifemenu.h) holds one at its +0x40 and
 * forwards init / term / isInit / processPad / render to it. Class names
 * from the wrappers (ror-inferred) except mnuBase, MnuMap, MnuPBook and
 * MnuDiary (the ELF's vtables). Constructors not written (MnuMap's and
 * the plain objects'): the objects live in the global scene and start
 * zeroed (a stub constructor would log before main). EE sizes from the scene's member spacing.
 * MnuSvld (mnu/mnusvld.h) and MnuSelectLanguage (mnu/mnuSelectLanguage.h)
 * have their files' headers. ---- */

struct BedPad;
struct I3dLayer;

/* mnuBase and the two books: their files' headers. */
#include "mnu/mnubase.h"
#include "mnu/mnupbook.h"

/* The map menu (vtable 0x2e46a8). EE 0x8e0 (MagLifeMenuMap +0x40..+0x920).
 * layout: host -- polymorphic. */
struct MnuMap : mnuBase
{
    void init(DogTexInfo *tex) override;                                 // ROR 0x00194908
    void term() override;                                                // ROR 0x00194960
    int  processPad(BedPad *pad) override;                               // ROR 0x00194fe4
    void draw(I3dLayer *layer) override;                                 // ROR 0x00195e90

    u_char    _110[0x8e0 - 0x110];
};
static_assert(sizeof(MnuMap) >= 0x8e0, "EE 0x8e0");

/* The plain menu objects: the same five calls, not polymorphic. Each
 * keeps the wrapper's scene at the offset its init wrapper writes.
 * layout: host -- unwritten classes padded to their EE size. */

/* The item menu. EE 0x2710 (MagLifeMenuItem +0x40..+0x2750). */
struct MnuItem
{
    void init(DogTexInfo *tex);                                          // ROR 0x001aeacc
    void term();                                                         // ROR 0x001aebc8
    int  isInit();                                                       // ROR 0x001aec48
    int  processPad(BedPad *pad);                                        // ROR 0x001b099c
    void draw(I3dLayer *layer);                                          // ROR 0x001aed08

    u_char    _00[0x1b60];
    MagScene *m_scene;          // ROR 0x1b60: MagLifeMenuItem::init stores the scene
    u_char    _1b64[0x2710 - 0x1b64];
};
static_assert(offsetof(MnuItem, m_scene) == 0x1b60, "m_scene");

/* The stock (item box) menu. EE 0x15b0 (MagLifeMenuStock +0x40..+0x15f0). */
struct MnuStock
{
    void init(DogTexInfo *tex);                                          // ROR 0x0019a650
    void term();                                                         // ROR 0x0019a6b8
    int  isInit();                                                       // ROR 0x0019a730
    int  processPad(BedPad *pad);                                        // ROR 0x0019a740
    void draw(I3dLayer *layer);                                          // ROR 0x0019a754

    u_char    _00[0x110c];
    MagScene *m_scene;          // ROR 0x110c: MagLifeMenuStock::init stores the scene
    u_char    _1110[0x15b0 - 0x1110];
};
static_assert(offsetof(MnuStock, m_scene) >= 0x110c, "m_scene");

/* The options menu (also inside MnuTitle at +0x3b80). EE 0x10f0
 * (MagLifeMenuOpt +0x40..+0x1130). */
struct MnuOpt
{
    void init(DogTexInfo *tex);                                          // ROR 0x001ab598
    void term();                                                         // ROR 0x001ab5e4
    int  isInit();                                                       // ROR 0x001ab65c
    int  processPad(BedPad *pad);                                        // ROR 0x001ab66c
    void draw(I3dLayer *layer);                                          // ROR 0x001ab680

    u_char    _00[0x10c8];
    MagScene *m_scene;          // ROR 0x10c8: MagLifeMenuOpt::init stores the scene
    u_char    _10cc[0x10f0 - 0x10cc];
};
static_assert(offsetof(MnuOpt, m_scene) == 0x10c8, "m_scene");

/* The movie (theatre) menu. EE 0x990 (MagLifeMenuMovie +0x40..+0x9d0). */
struct MnuMovie
{
    void init(DogTexInfo *tex);                                          // ROR 0x001adfb0
    void term();                                                         // ROR 0x001adfe4
    int  isInit();                                                       // ROR 0x001ae01c
    int  processPad(BedPad *pad);                                        // ROR 0x001ae02c
    void draw(I3dLayer *layer);                                          // ROR 0x001ae040

    u_char    _00[0x968];
    MagScene *m_scene;          // ROR 0x968: MagLifeMenuMovie::init stores the scene
    u_char    _96c[0x990 - 0x96c];
};
static_assert(offsetof(MnuMovie, m_scene) == 0x968, "m_scene");

/* The memory card menu MnuMcd: mnu/mnumcd.h. */
#include "mnu/mnumcd.h"

/* The title screen MnuTitle: mnu/mnutitle.h. */

/* The text list MnuTitle's movie viewer runs (MnuTitle +0x4c68; code
 * 0x1a066c..0x1a0bb8 in unassigned@00192ac0) and TskFrameFunc kind 8
 * (operator new). Class name invented. EE 0x104 (the next MnuTitle word
 * is +0x4d6c). The constructor, destructor and clear are written ahead
 * (lib/unassigned_stubs.cpp). */
struct MnuTextList
{
    MnuTextList();                                                       // ROR 0x001a066c
    ~MnuTextList();                                                      // ROR 0x001a0684
    /* The words cleared and the defaults; returns 0. */
    int  clear();                                                        // ROR 0x001a0860
    /* m_open 1 and the defaults; -1 when already open. */
    int  open();                                                         // ROR 0x001a06e8
    /* 0x1a0950 and m_open 0; -1 when not open. */
    int  close();                                                        // ROR 0x001a074c
    /* Non-zero once a row is chosen (0x1a0788's result). */
    int  processPad(BedPad *pad);                                        // ROR 0x001a06d4
    /* items: the labels, each followed by "#0#0"; mask: the enabled rows. */
    void setItems(const char *items, u_int mask);                        // ROR 0x001a09b8
    /* The chosen row (< 0 none yet); -2 when not open. */
    int  getSelect();                                                    // ROR 0x001a0ba0
    void endSelect();                                                    // ROR 0x001a0bb8

    int    m_open;              // ROR 0x00
    int    m_cur;               // ROR 0x04: MnuTitle::execMovie sets the row to start on (unverified)
    int    m_08[6];             // ROR 0x08: cleared by clear (roles unknown)
    int    m_20;                // ROR 0x20: clear sets 1
    int    m_24;                // ROR 0x24: clear sets 2
    int    m_28;                // ROR 0x28: clear sets 2
    u_char _2c[0x54 - 0x2c];
    int    m_x;                 // ROR 0x54: open sets 100, MnuTitle 0x40 (unverified: position)
    int    m_y;                 // ROR 0x58: open sets 100, MnuTitle 0x40
    int    m_z;                 // ROR 0x5c: open sets 0x7fff, MnuTitle 2000
    int    m_60;                // ROR 0x60: clear sets -1
    u_char _64[0x6c - 0x64];
    int    m_6c;                // ROR 0x6c: clear sets 0
    int    m_70;                // ROR 0x70: clear sets 0
    u_char _74[0x84 - 0x74];
    int    m_84[0x20];          // ROR 0x84: cleared by clear
};
static_assert(offsetof(MnuTextList, m_cur) == 0x04, "m_cur");
static_assert(offsetof(MnuTextList, m_20) == 0x20, "m_20");
static_assert(offsetof(MnuTextList, m_28) == 0x28, "m_28");
static_assert(offsetof(MnuTextList, m_x) == 0x54, "m_x");
static_assert(offsetof(MnuTextList, m_z) == 0x5c, "m_z");
static_assert(offsetof(MnuTextList, m_60) == 0x60, "m_60");
static_assert(offsetof(MnuTextList, m_6c) == 0x6c, "m_6c");
static_assert(offsetof(MnuTextList, m_70) == 0x70, "m_70");
static_assert(offsetof(MnuTextList, m_84) == 0x84, "m_84");
static_assert(sizeof(MnuTextList) == 0x104, "EE 0x104");

/* The "now" (status) menu. EE 0x920 (MagLifeMenuNow +0x40..+0x960). */
struct MnuNow
{
    void init(DogTexInfo *tex);                                          // ROR 0x001ae520
    void term();                                                         // ROR 0x001ae574
    int  isInit();                                                       // ROR 0x001ae5b4
    int  processPad(BedPad *pad);                                        // ROR 0x001ae600
    void draw(I3dLayer *layer);                                          // ROR 0x001ae614

    u_char    _00[0x90c];
    MagScene *m_scene;          // ROR 0x90c: MagLifeMenuNow::init stores the scene
    u_char    _910[0x920 - 0x910];
};
static_assert(offsetof(MnuNow, m_scene) >= 0x90c, "m_scene");

/* The 2D board. EE 0x10c0 (MagLifeMenuBoard2D +0x40..+0x1100). */
struct MnuBoard2D
{
    void init(DogTexInfo *tex);                                          // ROR 0x001ad824
    void term();                                                         // ROR 0x001ad870
    int  isInit();                                                       // ROR 0x001ad8bc
    int  processPad(BedPad *pad);                                        // ROR 0x001ad8cc
    void draw(I3dLayer *layer);                                          // ROR 0x001ad8e0

    u_char    _00[0x10b8];
    int       m_board;          // ROR 0x10b8: MagLifeMenuBoard2D::init stores getTypeValue's board id
    u_char    _10bc[0x10c0 - 0x10bc];
};
static_assert(offsetof(MnuBoard2D, m_board) == 0x10b8, "m_board");

/* ---- the debug range before debug/mimu_debug.cpp (unassigned@0020eda4) ---- */

void debugNop(void);                                                     // ROR 0x002155a0

/* ---- after the assert handler 0x215a30 (unassigned@0020ff34) ---- */

/* Formats the three floats (3 digits) and prints nothing: a trace whose
 * output the retail build compiled out. */
void dbgTraceVector3(float x, float y, float z);                         // ROR 0x00215b5c
/* The same for four floats. */
void dbgTraceVector4(const float *v);                                    // ROR 0x00215bb4

#endif /* _LIB_UNASSIGNED_H */
