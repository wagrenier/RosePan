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

/* ---- the MagScene range after setFog (unassigned@00143d3c) ---- */
/* Brown's script commands (native 039): args are the six script words, op
 * first; drives the scene's MagLifeBwnCtrl; returns the script's v0 (or a
 * SEQ_RET_* code). */
intptr_t magBwnSeqCommand(const intptr_t *args);                         // ROR 0x00144ae0

/* ---- the task framework (unassigned@001e46b8) ---- */

/* CmnITask, CmnTaskMan, g_taskMan: task/tsk_man.h. */
#include "task/tsk_man.h"

/* One of EmyManager's 8 slots (EE 0x3c, at +0x148; init clears them).
 * Native 212 cases 24 / 30 store its two words; roles unknown. */
struct EmySlot
{
    int    _00[3];
    int    m_0c;                // ROR 0x0c: native 212 case 24
    u_char _10[0x24 - 0x10];
    int    m_24;                // ROR 0x24: setSlotWord24 (native 212 case 30)
    u_char _28[0x3c - 0x28];
};
static_assert(sizeof(EmySlot) == 0x3c, "EmySlot EE size");
static_assert(offsetof(EmySlot, m_0c) == 0x0c, "EmySlot m_0c");
static_assert(offsetof(EmySlot, m_24) == 0x24, "EmySlot m_24");

/* EmyManager : CmnITask, one global at bss 0x7b2140 (constructor
 * 0x16265c, vtable 0x2e31a0). layout: host -- only m_slot placed; it
 * holds 64 enemy pointers at +4, each enemy's first word its oid. The
 * per-enemy wrappers do nothing (0 / false; testEmyView 1) when findEmy
 * finds none. */
struct EmyManager : CmnITask
{
    /* A new enemy for the type-2 achr chr (forwards to 0x162c88): chrNo
     * from the kind's system data field 0x72, GamMan's script word
     * +0x22364, the image slot the caller picked, the position. */
    /* oid 0 (addChrGroup's achrs): one is made, 0x9100006f + a count
     * (when a further stack word the callers leave unset is non-zero),
     * set in GamMan's achr slot table and the achr's param 1. group is
     * EmyManager's group (addChr passes 0). Returns the oid, 0 when no
     * enemy was added. */
    u_int addEmy(int chrNo, short kind, void *chr, intptr_t seqWord, void *image,
                 const void *pos, u_int oid, int group = 0);             // ROR 0x00162c2c
    /* Group a (< 9) set up: b (< 0x11), c (at least 60), count (< 0x11),
     * d, the position and f; on. */
    void setGroup(float f, int a, int b, int c, int count, int d,
                  const void *pos);                                      // ROR 0x00162b68
    /* Clears its stage work: bytes +0x134 / +0x135, the two object ids,
     * +0x144 and the 0x1e0 bytes at +0x148. */
    void resetWork();                                                    // ROR 0x001635e0
    /* Its two object ids (+0x138 / +0x13c). */
    void getOids(int *a, int *b);                                        // ROR 0x00163ab8
    /* Deletes its object whose first word is oid (a type-2 achr's). */
    void releaseObj(u_int oid);                                          // ROR 0x001634d4
    /* The enemy whose oid this is, NULL when none. */
    void *findEmy(u_int oid);                                            // ROR 0x001636a4
    /* Enemy +8 = on; off pauses its achr's tasks and sets achr flags
     * 0x400 | 0x2000, on clears them. */
    void setEmyActive(u_int oid, int on);                                // ROR 0x0016366c
    int  getEmyWord3c(u_int oid);                                        // ROR 0x00163b20
    int  setEmyWord238(u_int oid, int v);                                // ROR 0x00163b48
    /* Two cone tests (angle pi) from the enemy against the objects of
     * bss 0x7a3350 / 0x7a3354. */
    int  testEmyView(u_int oid);                                         // ROR 0x00163b80
    /* Achr flags lose 0x10000000 and 0x80, enemy +0x240 = 1. */
    void releaseEmyHold(u_int oid);                                      // ROR 0x00163bb8
    void setSlotWord24(int no, int v);                                   // ROR 0x00162afc
    /* Enemy word +0x30 bit 0. */
    int  isEmyFlag30(u_int oid);                                         // ROR 0x00164078

    EmySlot m_slot[8];          // ROR 0x148
};

/* What ACHRstat +0x3d0 points at: only its actor no is read. EE layout
 * (the rest unknown). */
struct ChrMotionRef
{
    int _00[3];
    int m_actor;                // ROR 0x0c: the DogActor no
};
static_assert(offsetof(ChrMotionRef, m_actor) == 0x0c, "ChrMotionRef m_actor");
/* !DogActor::isModeSet of ref's actor (unassigned@001e6d54). */
int chrMotionRefEnd(const ChrMotionRef *ref);                            // ROR 0x001e8f88

/* The parameter block TskSnd's constructor copies (0x40 bytes on the EE;
 * only the words SndMan::playStream sets are named). layout: host. */
struct TskSndParam
{
    int         _00[3];
    int         m_id;           // ROR 0x0c: the stream id
    int         _10[3];
    const char *m_name;         // ROR 0x1c: kind 7 copies the name (0x1b chars)
    int         _20[2];
    int         m_arg;          // ROR 0x28: playStream's arg
    int         m_mode;         // ROR 0x2c: playStream's mode
    int         m_id2;          // ROR 0x30: the stream id again
    int         _34[3];
};

/* TskSnd : CmnITask (vtable 0x2e4e70, EE 0xc0 bytes, unassigned@001f82e4):
 * a sound task; kind picks its step function (7: play the stream m_name
 * once the old one has stopped). layout: host -- no members. */
struct TskSnd : CmnITask
{
    TskSnd(int kind, const TskSndParam *param);                          // ROR 0x001f8fc0
};

/* TskFrameFunc : CmnITask (vtable 0x2e4d50, EE 0x80 bytes): kind < 0x17
 * picks its step function (table 0x2fe420); arg is a value or a pointer
 * to the caller's words, frames its length. layout: host -- no members. */
struct TskFrameFunc : CmnITask
{
    TskFrameFunc(int kind, intptr_t arg, int frames);                    // ROR 0x001f184c
};

/* TskMovie : CmnITask (vtable 0x2e4c50, EE 0x80 bytes): plays the movie
 * name (S2dMovie::playMovie) after stopping the sound; native 253's movie
 * task (GamMan::m_movieTaskId). layout: host. */
struct TskMovie : CmnITask
{
    TskMovie(const char *name);                                          // ROR 0x001f438c

    int  _04;
    int  m_state;               // ROR 0x08: 100 from the constructor; native 253 case 4 waits while <= 100
    int  _0c;
    char m_name[0x40];          // ROR 0x10
};

/* The achr movement tasks (unassigned@001e6cac; class names from their
 * vtables): oid the achr, mode the way it moves, rate the step per frame
 * (1/60 by default). Natives 202 / 211 start them on the achr's
 * m_task[0]. layout: host -- no members. */
struct TskMoveTypeGen : CmnITask           // EE 0xc0, vtable 0x2e4ad0
{
    TskMoveTypeGen(u_int oid, int mode, intptr_t arg, int kind, float frames, float rate); // ROR 0x001e9a3c
};
/* layout: host */
struct TskMoveTypeRound : CmnITask         // EE 0x80
{
    TskMoveTypeRound(u_int oid, int mode, intptr_t arg, int kind, float a, float b, float rate); // ROR 0x001eb63c
};
/* layout: host */
struct TskMovePointPath : CmnITask         // EE 0xc0
{
    TskMovePointPath(u_int oid, int mode, intptr_t path, int arg, int kind, float a, float rate); // ROR 0x001ec344
};
/* layout: host */
struct TskMoveSectaFindPath : CmnITask     // EE 0x2c0
{
    TskMoveSectaFindPath(u_int oid, intptr_t arg, int a3, int kind, int shift, int t2, float a,
                         float rate);                                    // ROR 0x001ec940
};
/* layout: host */
struct TskMoveSecta : CmnITask             // EE 0x80
{
    TskMoveSecta(u_int oid, int mode, intptr_t arg, int a3, int kind, float a, float rate); // ROR 0x001ed998
};

/* TskCamera : CmnITask (EE 0xc0, unassigned@001f82e4): a scripted camera
 * move (native 016 case 50, on GamMan's m_camTaskId). layout: host. */
struct TskCamera : CmnITask
{
    TskCamera(intptr_t a, intptr_t b);                                   // ROR 0x001fbc0c
};

/* TskAttach : CmnITask (vtable 0x2e4bd0, EE 0x40 bytes): +4 the oid, +8
 * its actor, +0xc arg, +0x10 *value. layout: host -- no members. */
struct TskAttach : CmnITask
{
    TskAttach(u_int oid, const int *value, int arg);                     // ROR 0x001f5244
};

/* TskSeqEvent : CmnITask (vtable 0x2e4c90, EE 0x80 bytes): kind 0 reads
 * {frames, radius} from param and the achr of oid; GamMan::
 * startAchrPlaceTask's task (ACHRstat::m_placeTask). layout: host. */
struct TskSeqEvent : CmnITask
{
    TskSeqEvent(u_int oid, int kind, const void *param);                 // ROR 0x001f47f0

    float m_radius;             // ROR 0x48: param word 1
};

/* TskMsgMan : CmnITask (vtable 0x2e4c10, EE 0x9c0 bytes): a message
 * window on text (two MsgMan at +0x224) with the eight window words param;
 * GamMan::openMsg's task (m_msgTaskId). layout: host -- no members. */
struct TskMsgMan : CmnITask
{
    TskMsgMan(const char *text, int arg, const int *param, int a4, int mode, int time); // ROR 0x001f4a24
};

/* TskSysFunc : CmnITask (vtable 0x2e4cd0, EE 0x80 bytes): kinds 0..1 set
 * the defaults 100, 0x1e, 0xf, 0x1b and keep arg. layout: host. */
struct TskSysFunc : CmnITask
{
    TskSysFunc(int kind, int a, intptr_t arg);                           // ROR 0x001f4184
};

/* Event scenes (unassigned@001f82e4): data/evt/%02d/scene/<name> from
 * the stage pfh, played by a ScenePlay task (vtable 0x2e4dd0, 0x580
 * bytes). scenePlayStart also reads the map / CHARA / THREAD sections and
 * halts the game; wait is its third argument. */
int  scenePlayStart(const char *name, intptr_t arg, int wait);           // ROR 0x001f83f4
void scenePlayStartSimple(const char *name);                             // ROR 0x001f8cec

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
/* v.x += sinf(angle) * dist, v.z += cosf(angle) * dist (unassigned@001e6cac). */
void  utlAddPolar(float angle, float dist, float *v);                    // ROR 0x001f6b28
/* An achr's turn task (TskChrRot, ctor 0x1f0434) on ACHRstat::m_task[1]:
 * chrRotStart kills the running one and, for a non-zero angle, turns by
 * angle over frames; chrRotTo / chrRotToIn turn to the heading dir (the
 * delta wrapped into -pi..pi) at speed / over frames. chrRotFrames is
 * |angle / (speed * 0.05233)|. */
void  chrRotStart(float angle, u_int oid, int frames);                   // ROR 0x001f0e90
void  chrRotTo(float dir, float speed, u_int oid);                       // ROR 0x001f0f9c
void  chrRotToIn(float dir, u_int oid, int frames);                      // ROR 0x001f10a4
int   chrRotFrames(float angle, float speed);                            // ROR 0x001f6b8c
extern EmyManager g_emyManager; /* bss 0x7b2140 */
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

/* i3d's matrix struct (tag_I3dMatrixF4x4_s in DogModel's symbols): the
 * same 16 floats as an sceVu0FMATRIX, but the ROM copies it with
 * unaligned ldl / sdl, so it is only 4-aligned. EE 0x40. */
struct I3dMatrixF4x4
{
    float m[4][4];
};
static_assert(sizeof(I3dMatrixF4x4) == 0x40, "I3dMatrixF4x4 is 0x40 bytes on the EE");

/* The item tree's relocators (unassigned@0022b4b8): recursive over the
 * children, base = the root item; per-type data fixups from the tables
 * 0x2db4c8 / 0x2db360. I3D_OK, 3 type > 0x59, 4 an invalid type. */
int   i3rItemRelocate(I3rItem *item, I3rItem *base);                     // ROR 0x0022c478
int   i3rItemUnrelocate(I3rItem *item, I3rItem *base);                   // ROR 0x0022c328
/* i3rSetRelocate's hooks: build the root's model packets (0x21fb50 per
 * mesh) into the slots of its child 6, and clear those slots. */
int   i3rBuildPackets(I3rRoot *root);                                    // ROR 0x0021a3e8
int   i3rClearPackets(I3rRoot *root);                                    // ROR 0x0021a558

/* An i3d STQ map entity (tag_I3rStqMapEntity_s in DogModel's symbols).
 * The scene keeps one at +0x2aa90 (less than 0x40 bytes before +0x2aad0).
 * layout: host -- placeholder, fields not known. */
struct I3rStqMapEntity
{
    u_char _00[0x40];
};
static_assert(sizeof(I3rStqMapEntity) >= 0x40, "placeholder");

/* The model's clip sphere: centre (w = 1) and radius. */
int   i3rCopyClipSphere(I3rRoot *root, float *center, float *radius);    // ROR 0x0021a828
int   i3rReplaceStqMapEntity(I3rRoot *root, I3rStqMapEntity *entity);    // ROR 0x0021a938
/* Steps the frame counter of every animated texture of the root; I3D_OK. */
int   i3rUpdateTexAnim(I3rRoot *root);                                   // ROR 0x0021a8e0

/* Strips of count vertices with an optional matrix, STQ, colours and
 * positions (EE vectors); primitives unverified. */
u_long   *i3dPktDrawStrip(u_long *pkt, sceVu0FVECTOR *matrix, int count, const void *stq,
                          const void *colors, const void *xyz, int flags); // ROR 0x0021db18
u_long   *i3dPktDrawStripCtx(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix, int count,
                             const void *stq, const void *colors, const void *xyz,
                             int flags);                                 // ROR 0x0021dd10
u_long   *i3dPktDrawLineStrip(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix, int count,
                              const void *color, const void *xyz, int flags); // ROR 0x0021e1d8
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
/* Allocates the 1 MB 2D packet buffer (data 0x29bfb4); mnuBase::initSystem
 * calls it too. */
void  FUN_001944d0(void);                                                // ROR 0x001944d0
/* Frees the buffer FUN_001944d0 allocates (sdata 0x29bfb4); MagScene::term
 * and mnuBase::termSystem. */
void  FUN_00194548(void);                                                // ROR 0x00194548
/* m_837e0's teardown (frees its buffer, closes its FileReader). */
void  FUN_00176c50(Obj176a3c *obj);                                      // ROR 0x00176c50
/* The closest points of segments p0 + s d0 and p1 + t d1 (s, t in 0..1,
 * either out pointer may be NULL); returns the squared distance. */
float i3dSegSegDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                     float *s, float *t);                                // ROR 0x00234050
/* The squared distance from p to the segment a + t d (t in 0..1, written
 * to *t when t is not NULL). */
float i3dPointSegDist2(const float *p, const float *a, const float *d, float *t); // ROR 0x002337e0

/* ---- the word table (unassigned@00164fc0: 0x17e0a8..0x17eff4) ---- */

/* A word-table record: the strings +0x08..+0x14. layout: host -- the
 * strings are pointers; the file format is not read yet. */
struct WrdEntry
{
    int         _00[2];
    const char *m_str0;         // ROR 0x08: gamGetWrdString 0
    const char *m_mapName;      // ROR 0x0c: gamGetWrdString 1 / 4 (GamMan::getMapName)
    const char *m_text;         // ROR 0x10: gamGetWrdString 2, S2dMes::drawMapName's caption
    const char *m_str3;         // ROR 0x14: gamGetWrdString 3
    int         m_18;           // ROR 0x18: gamSetWrdRoom keeps it in GamMan +0x18dd0
    int         m_mapArea;      // ROR 0x1c: GamMan::getMapArea (gamSetWrdRoom keeps it in GamMan +0x18dc0)
    float       m_ofs[3];       // ROR 0x20: the room's offset (cnvPosW2L / cnvPosL2W, gamWrdConvPos op 2)
};

/* data/wrd/WRD.BIN once loaded (0x17e5c0, from GamMan::loadStage): +4 the
 * file (count at its +8), +8 the record pointers. Class name from the
 * file, invented. layout: host -- no members reconstructed. */
struct WrdTable
{
    /* Record n, NULL when none is loaded or n is out of range. */
    WrdEntry *get(int n);                                                // ROR 0x0017ea44
    /* Reads data/wrd/WRD.BIN and keeps file "%03d" of stage. */
    void load(int stage);                                                // ROR 0x0017e5c0
    /* out = in - record no's offset, when a table is loaded. */
    void cnvPosW2L(float *out, const float *in, int no);                 // ROR 0x0017ebac
    /* out = record no's offset + in, when a table is loaded. */
    void cnvPosL2W(float *out, int no, const float *in);                 // ROR 0x0017eb3c
};

extern WrdTable g_wrdTable;     /* data 0x298ee0 */

/* ---- the mnu/ menu objects (unassigned@00192ac0, @001a0950, @001a6830):
 * each MagLifeMenu* wrapper (mag/maglifemenu.h) holds one at its +0x40 and
 * forwards init / term / isInit / processPad / render to it. Class names
 * from the wrappers (ror-inferred) except mnuBase, MnuMap, MnuPBook and
 * MnuDiary (the ELF's vtables). Their constructors are not written: the
 * objects live in the global scene and start zeroed (a stub constructor
 * would log before main). EE sizes from the scene's member spacing.
 * MnuSvld (mnu/mnusvld.h) and MnuSelectLanguage (mnu/mnuSelectLanguage.h)
 * have their files' headers. ---- */

struct BedPad;
struct I3dLayer;

/* The polymorphic menu base (vtable 0x2e4640, vptr +0x10c; constructor
 * 0x192ac0 also builds a FileReader at +0x68). layout: host -- polymorphic.
 * EE slots: 1 ~dtor 0x192b3c, 2 init (pure), 3 term 0x192d60, 4 isInit,
 * 5 processPad (pure), 6 draw (pure), 7 0x193140, 8 getType, 9 getFlags,
 * 10 0x1938a8, 11 0x193920; 7.. are declared when a TU needs them. */
struct mnuBase
{
    /* EE slot 1 (+0x08); 0x192b3c is not written (nothing to release in a
     * zeroed object). */
    virtual ~mnuBase() {}
    /* EE slot 2 (+0x10) */
    virtual void init(DogTexInfo *tex) = 0;
    /* EE slot 3 (+0x18) */
    virtual void term();                                                 // ROR 0x00192d60
    /* EE slot 4 (+0x20): bit 0 of the flags. */
    // ROR 0x0028dec0
    virtual int  isInit() { return m_flags & 1; }
    /* EE slot 5 (+0x28) */
    virtual int  processPad(BedPad *pad) = 0;
    /* EE slot 6 (+0x30) */
    virtual void draw(I3dLayer *layer) = 0;

    int       m_type;           // ROR 0x00: mnuBase::getType 0x28dee0
    u_int     m_flags;          // ROR 0x04: bit 0 set once init has run
    MagScene *m_scene;          // ROR 0x08: MagLifeMenuPBook::init stores the wrapper's scene
    u_char    _0c[0x10c - 0x0c];
    /* ROR 0x10c: the EE vptr (the host's is at 0) */
};
static_assert(offsetof(mnuBase, m_flags) >= 0x04, "m_flags");
static_assert(offsetof(mnuBase, m_scene) >= 0x08, "m_scene");

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

/* The two books MagLifeMenuPBook switches between (vtables 0x2e4710 /
 * 0x2e4778). setType keeps the row of its page table (0x29c0f8 /
 * 0x29c368, 12-byte rows, -1 ends) whose first word is type. layout: host
 * -- polymorphic. EE 0x8c0 (MagLifeMenuPBook +0x50..+0x910). */
struct MnuPBook : mnuBase
{
    void init(DogTexInfo *tex) override;                                 // ROR 0x00196bdc
    void term() override;                                                // ROR 0x00196d98
    int  processPad(BedPad *pad) override;                               // ROR 0x001976b8
    void draw(I3dLayer *layer) override;                                 // ROR 0x00197f70
    /* 1 when type is one of its pages (kept at +0x114), else 0. */
    int  setType(int type);                                              // ROR 0x00196e54

    u_char    _110[0x8c0 - 0x110];
};
static_assert(sizeof(MnuPBook) >= 0x8c0, "EE 0x8c0");

/* EE 0x8f0 (MagLifeMenuPBook +0x910..+0x1200). layout: host -- polymorphic. */
struct MnuDiary : mnuBase
{
    void init(DogTexInfo *tex) override;                                 // ROR 0x00198860
    void term() override;                                                // ROR 0x00198a38
    int  processPad(BedPad *pad) override;                               // ROR 0x001990b0
    void draw(I3dLayer *layer) override;                                 // ROR 0x00199c94
    /* 1 when type is one of its pages (kept at +0x114), else 0. */
    int  setType(int type);                                              // ROR 0x00198b04

    u_char    _110[0x8f0 - 0x110];
};
static_assert(sizeof(MnuDiary) >= 0x8f0, "EE 0x8f0");

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
 * 0x1a066c..0x1a0bb8 in unassigned@00192ac0). Class name invented. EE
 * 0x104 (the next MnuTitle word is +0x4d6c). Its constructor 0x1a066c
 * (clears m_open, calls 0x1a0860) is not written: the object lives in the
 * global scene and starts zeroed. */
struct MnuTextList
{
    /* m_open 1 and the defaults; -1 when already open. */
    int  open();                                                         // ROR 0x001a06e8
    /* 0x1a0950 and m_open 0; -1 when not open. */
    int  close();                                                        // ROR 0x001a074c
    void processPad(BedPad *pad);                                        // ROR 0x001a06d4
    /* items: the labels, each followed by "#0#0"; mask: the enabled rows. */
    void setItems(const char *items, u_int mask);                        // ROR 0x001a09b8
    /* The chosen row (< 0 none yet); -2 when not open. */
    int  getSelect();                                                    // ROR 0x001a0ba0
    void endSelect();                                                    // ROR 0x001a0bb8

    int    m_open;              // ROR 0x00
    int    m_cur;               // ROR 0x04: MnuTitle::execMovie sets the row to start on (unverified)
    u_char _08[0x54 - 0x08];
    int    m_x;                 // ROR 0x54: open sets 100, MnuTitle 0x40 (unverified: position)
    int    m_y;                 // ROR 0x58: open sets 100, MnuTitle 0x40
    int    m_z;                 // ROR 0x5c: open sets 0x7fff, MnuTitle 2000
    u_char _60[0x104 - 0x60];
};
static_assert(offsetof(MnuTextList, m_cur) == 0x04, "m_cur");
static_assert(offsetof(MnuTextList, m_x) == 0x54, "m_x");
static_assert(offsetof(MnuTextList, m_z) == 0x5c, "m_z");
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
