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

/* Type 6 (vtable from rtti.py; EE 0x240). layout: host -- polymorphic. */
class HfmMain : public EmyMain
{
public:
    HfmMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,  // ROR 0x0028589c
            const _tagEnemyTransition *trans, float *pos)
        : EmyMain(oid, name, param, prm, trans, pos)
    {
        initOut();
    }
    ~HfmMain();                                                         // EE slot 1 (+0x08), ROR 0x0014529c
    void selectOut();                                                    // EE slot 2 (+0x10), ROR 0x00145680
    int  GetType() { return 6; }                                       // EE slot 3 (+0x18), ROR 0x002858d4
    void decide();                                                       // EE slot 4 (+0x20), ROR 0x001453c0
    void exec();                                                         // EE slot 5 (+0x28), ROR 0x00145560
    void after();                                                        // EE slot 6 (+0x30), ROR 0x00145604
    void initOut();                                                      // ROR 0x00145020
    /* createEmy, after loadParam. */
    void initAfterParam();                                               // ROR 0x001452fc
    /* m_free_cnt - n, at least 0 (EmyManager::subEmyFreeCnt). */
    void subFreeCnt(int n);                                              // ROR 0x0014582c

    int m_free_cnt;             // ROR 0x234: SetFreeCnt
    int m_23c;                  // ROR 0x23c: EmyManager::getEmyHoldWord23c
};

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

/* The enemy move task (vtable 0x2e5040; unassigned@001fd424). layout: host --
 * polymorphic; nothing placed yet. */
struct TskEnemyMove : CmnITask
{
    /* Stops it: +4 = 0, its SE stopped, EmyManager's work word of its
     * oid cleared (EmyMain::InactiveMove). */
    void stop();                                                         // ROR 0x00202f10
};

/* ChrMotionRef, the Tsk* script tasks, the chr* helpers: task/tsk_chr.h. */
#include "task/tsk_chr.h"

/* TskSndParam, TskSnd, TskCamera, scnChrRequest, scenePlayStart: task/tsk_scene.h. */
#include "task/tsk_scene.h"

/* 1 when the two strings are equal, a..z compared as A..Z, else 0
 * (unassigned@00106010, after utl/utlfile.cpp). */
int   utlStrEqualNoCase(const char *a, const char *b);                   // ROR 0x00106010
/* Swaps a sound id 0x0a00xxxx..0x0a02xxxx for its twin in the column the
 * game's language / version picks (table data 0x295f08). Other ids pass. */
u_int utlConvSndId(u_int id);                                            // ROR 0x00106658
/* v[3] = 0, then v normalised in place (sceVu0Normalize). */
void  utlNormalize3(float *v);                                           // ROR 0x00106db0
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

void utlRandReset(void);                                                 // ROR 0x0010421c
/* A random integer in lo..hi (lo + 0x1041e4(hi - lo + 1)). */
int  utlRandRange(int lo, int hi);                                       // ROR 0x0010423c
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

/* out[0] = (m00, m11, m22, 0), out[1] = m[3]. */
void  i3dMatGetScaleTrans(sceVu0FVECTOR *out, sceVu0FVECTOR *m);         // ROR 0x00228c38

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
/* unassigned@0021fb50: writes the VIF / DMA packets of one mesh item (a
 * type 0x4c entry's child 0 child) into buf. i3rBuildPackets passes the
 * same buffer start for every mesh. */
void  i3rBuildMeshPacket(I3rItem *mesh, void *buf);                     // ROR 0x0021fb50

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

/* Draws an i3r model into layer with its node matrices (NULL: none) and
 * world matrix, the thread switch held off around it (0x21fe68). opt is
 * NULL or a record of a mask and five floats (DogActor::drawModel's
 * {0x1f, v x5} for a model with an animated value; role unverified). */
int   utlDrawI3r(I3dLayer *layer, I3rRoot *root, sceVu0FMATRIX *nodes, sceVu0FVECTOR *world,
                 const void *opt);                                       // ROR 0x00106760
/* out.xy = in.xy turned by angle about z, out.z = in.z (unassigned@00106658). */
void  utlVecRotZ(float angle, float *out, const float *in);              // ROR 0x00106a40
/* v (4 floats) in place by a y rotation matrix of angle (wrapped once to
 * -pi..pi; sceVu0RotMatrixY / ApplyMatrix). DogActor::getRootMove. */
void  utlVecRotY(float angle, float *v);                                 // ROR 0x00106bc8
/* sqrt(x * x + y * y + z * z) of v. */
float utlVecLength3(const float *v);                                     // ROR 0x00106ad0
/* Points the first word of each of the root's materials at disp. */
int   utlSetI3rDispControl(I3rRoot *root, void *disp);                   // ROR 0x001067d8
/* Node n's material's display record (its first word), NULL none; the
 * set returns 0, 1 when the node has no material. */
void *utlGetI3rNodeDisp(I3rRoot *root, int n);                           // ROR 0x001068d4
int   utlSetI3rNodeDisp(I3rRoot *root, void *disp, int n);               // ROR 0x0010686c

/* Strips of count vertices with an optional matrix, STQ, colours and
 * positions (EE vectors); primitives unverified. */
u_long   *i3dPktDrawStrip(u_long *pkt, sceVu0FVECTOR *matrix, int count, const void *stq,
                          const void *colors, const void *xyz, int flags); // ROR 0x0021db18
u_long   *i3dPktDrawStripCtx(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix, int count,
                             const void *stq, const void *colors, const void *xyz,
                             int flags);                                 // ROR 0x0021dd10
u_long   *i3dPktDrawLineStrip(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix, int count,
                              const void *color, const void *xyz, int flags); // ROR 0x0021e1d8
/* The VU1 sprite packets (mnuBase's menu draw): the begin calls the
 * layer's view (and fog) and writes the sprite GIF template (mode 1 / 0:
 * 0x3026b0 / 0x3026c0); then per sprite a colour, a matrix and the
 * sprite's uv and position, each UNPACKed to VU1 (the sprite MSCALs its
 * microprogram); the end returns pkt. Not drawn on the host yet (the
 * native i3d module has no VU1 path). */
u_long   *i3dPktBeginVu1Sprite(u_long *pkt, I3dLayer *layer, int mode);   // ROR 0x0021e5a0
u_long   *i3dPktVu1Matrix(u_long *pkt, sceVu0FVECTOR *matrix);           // ROR 0x0021e428
u_long   *i3dPktVu1Color(u_long *pkt, const float *color);               // ROR 0x0021e488
u_long   *i3dPktVu1Sprite(u_long *pkt, const float *pos, const float *uv); // ROR 0x0021e690
u_long   *i3dPktEndVu1Sprite(u_long *pkt);                               // ROR 0x0021e688
/* The i3m motion player (unassigned@0022e840). */
struct I3mRoot;
/* The motion's length in frames (the root's m_length). */
float     i3mGetLength(const I3mRoot *root);                             // ROR 0x0022e958
/* Samples every node's keys at time t into 0x30-byte pose records. */
void      i3mGetPose(I3mRoot *root, void *pose, int flags, float t);     // ROR 0x0022e960
/* Two motions' poses (root1 at t1, root2 at t2) blended at rate r into pose. */
void      i3mGetBlendPose(I3mRoot *root1, void *pose, I3mRoot *root2, int flags, float t1, float t2, float r); // ROR 0x0022eab0
/* Pose records to 0x40-byte node matrices through map (u16 count, then a
 * model node per motion node, 0xffff = none). */
void      i3mPoseToMatrices(const void *pose, sceVu0FMATRIX *mats, const u_short *map); // ROR 0x0022f7a0
/* One 0x30-byte pose record: out = a * t + b * (1 - t) (the rotation
 * quaternion at +0x10 slerped). */
void      i3mPoseLerp(float t, void *out, const void *a, const void *b); // ROR 0x0022f610
/* Quaternions (x, y, z, w): from the euler angles rot (x, y, z), the
 * product a * b, normalized. */
void      i3mQuatFromEuler(float *out, const float *rot);                // ROR 0x00231198
void      i3mQuatMul(float *out, const float *a, const float *b);        // ROR 0x00230470
void      i3mQuatNormalize(float *out, const float *in);                 // ROR 0x00230660

/* Wire shapes as line strips (MagScene::drawStrips' debug records through
 * i3dLayerDrawWireSphere / Box / Shape). */
u_long   *i3dPktDrawWireSphere(float radius, u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix,
                               const float *center);                    // ROR 0x0021e6f8
u_long   *i3dPktDrawWireBox(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix,
                            const float *center, const void *extent, const void *color); // ROR 0x0021e8f8
u_long   *i3dPktDrawWireShape(u_long *pkt, I3dLayer *layer, void *a, void *b, void *c); // ROR 0x0021e9f8

/* The distance of two points (sqrt of 0x106e14). */
float utlVecDist(const float *a, const float *b);                        // ROR 0x00106e70
/* A direction to angles: rot x (pitch, from -y), y (yaw, atan2 x / z), z 0. */
void  utlVecToRot(const float *dir, float *rot);                         // ROR 0x00107248

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
/* The base of the 43 Eff* effect classes (vtable 0x2e3250, vptr at +0x20).
 * layout: host -- only the flag word is placed. */
struct Eff0
{
    /* Slot 10 of every Eff class (never overridden): flag bit 4 = hidden,
     * bit 8 = changed. */
    void setVisible(bool on);                                            // ROR 0x0016aa04

    u_short m_flags;            // ROR 0x00
};
/* Obj176a3c::createEff's parameter block (EE 0x80; natives 281 fill it on
 * the stack). m_kind picks the class (1..0x34). layout: host -- word 0x10
 * holds a DogActor for kind 0x15, so the data that follows moves. */
struct EffParam
{
    int m_kind;                 // ROR 0x00
    int _04;
    int m_08;                   // ROR 0x08: 0 from every caller seen
    int _0c;
    union
    {
        u_char m_data[0x70];    // ROR 0x10: the script's 0x70 bytes (native 281 ops 0..49)
        struct
        {
            DogActor *m_actor;  // ROR 0x10
            int       m_args[12]; // ROR 0x14
        } m_achr;               // kind 0x15 (native 281 op 38)
        const void *m_file;     // ROR 0x10: kind 0xd, an event pack's effect file (scenePlayStart)
        struct
        {
            float m_pos[3];     // ROR 0x10
            int   m_1c;         // ROR 0x1c: 0
            float m_20;         // ROR 0x20: -1
            int   m_24;         // ROR 0x24: 0
            float m_scale;      // ROR 0x28: 1, 1.75 for a kind-2 damage
        } m_hit;                // kind 1 (scnDamageStart)
    };
};
struct Obj176a3c                /* scene +0x837e0, constructor 0x176a3c */
{
    /* News the Eff class of param->m_kind into a free table entry; its
     * number (scenePlayStart, native 281). */
    int  createEff(const EffParam *param);                               // ROR 0x00176cc8
    /* Deletes entry no of its table (GamMan::ctlObj1cbb80). */
    void release(int no);                                                // ROR 0x0017756c
    /* Deletes every entry of the table and resets its state words
     * (GamMan::clearPool20b00). */
    void releaseAll();                                                   // ROR 0x0017762c

    int   _00;
    Eff0 *m_entry[0x110];       // ROR 0x04: the constructor clears 0x440 bytes
};
/* layout: host -- placeholder. */
/* A floor sector of a sky pack's sector.sec (EE 0x80, file data). */
struct MagSectorRec
{
    u_char _00[0x48];
    int    m_snd;               // ROR 0x48: the footstep sound slot (1..11), < 0 = the map's default
    u_char _4c[0x80 - 0x4c];
};
static_assert(offsetof(MagSectorRec, m_snd) == 0x48, "m_snd");
static_assert(sizeof(MagSectorRec) == 0x80, "MagSectorRec is 0x80 bytes on the EE");
/* What FUN_0017c340 finds under a point (its first word out). */
struct MagSectorHit
{
    int m_00;                   // ROR 0x00
    int m_sector;               // ROR 0x04: the sector's index
};
static_assert(offsetof(MagSectorHit, m_sector) == 0x04, "m_sector");
/* layout: host -- placeholder. */
struct Obj17c0f0                /* scene +0x8542c, constructor 0x17c0f0: a flag word, a buffer, four 0x18-byte records */
{
    int           _00[3];
    int           m_0c;         // ROR 0x0c: non-zero when the sectors are loaded; role unverified
    MagSectorRec *m_sectors;    // ROR 0x10: the sector records
};
static_assert(sizeof(Obj176a3c) >= 4 && sizeof(Obj17c0f0) >= 4, "placeholders");

int   FUN_00176afc(Obj176a3c *obj, i3dContext *ctx, MagScene *scene);    // ROR 0x00176afc
void  FUN_00177830(Obj176a3c *obj, i3dContext *ctx);  /* MagScene::draw's world pass */ // ROR 0x00177830
/* MagScene::update's step: runs a check slot of each of its objects and
 * deletes the finished ones. */
void  FUN_0017772c(Obj176a3c *obj);                                      // ROR 0x0017772c
void  FUN_0017c2c0(Obj17c0f0 *obj);  /* frees the buffer and the records */ // ROR 0x0017c2c0
/* Bits into (set) / out of (clear) the entries of the group key finds,
 * in record rec or all four when rec < 0 (native 281 ops 200 / 201). */
void  sectorSetFlags(Obj17c0f0 *obj, int key, u_int bits, int rec);      // ROR 0x0017c45c
void  sectorClearFlags(Obj17c0f0 *obj, int key, u_int bits, int rec);    // ROR 0x0017c4f4
/* Takes a sky pack's sector.sec (copied to the game heap when copy is
 * set); non-zero on success. */
int   FUN_0017c1f4(Obj17c0f0 *obj, void *data, int copy);               // ROR 0x0017c1f4
/* The sector under pos (hit[0], NULL when none) within r / h (the ROM
 * passes 40 and 0 in f12 / f13). */
void  FUN_0017c340(Obj17c0f0 *obj, const float *pos, MagSectorHit **hit, int arg,
                   float r, float h);                                    // ROR 0x0017c340
/* mnuInitPool 0x1944d0 / mnuTermPool 0x194548 (the mnu pool): mnu/mnubase.h. */
/* m_837e0's teardown (frees its buffer, closes its FileReader). */
void  FUN_00176c50(Obj176a3c *obj);                                      // ROR 0x00176c50
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
