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

/* ---- the globals file before main.cpp (unassigned@001fd424): its static
 * initializer 0x2041f8 constructs the heap, MagSystem, GamMan, the task
 * manager, the sequence, EmyManager, SndMan and 0x7b40c0 ---- */

void sysInitManagers(void);                                              // ROR 0x00204148

/* A frame timer (the task manager's member at +0x40). EE size at least
 * 0x2c. layout: host -- no members reconstructed. */
struct SysTimer
{
    void start();                                                        // ROR 0x00203ea8
    void read();                                                         // ROR 0x00203f38
    void update();                                                       // ROR 0x00204014
};

/* ---- the task framework (unassigned@001e46b8) ---- */

/* The task interface 32 classes derive from (no vtable of its own in the
 * ELF). layout: host -- no members reconstructed. */
struct CmnITask
{
};

/* EmyManager : CmnITask, one global at bss 0x7b2140 (constructor
 * 0x16265c). layout: host -- no members reconstructed. */
struct EmyManager : CmnITask
{
    /* Its two object ids (+0x138 / +0x13c). */
    void getOids(int *a, int *b);                                        // ROR 0x00163ab8
    /* Deletes its object whose first word is oid (a type-2 achr's). */
    void releaseObj(u_int oid);                                          // ROR 0x001634d4
};

/* The task manager, one global at bss 0x7a3d80. EE size at least 0x288.
 * layout: host -- only the timer member is placed (EE +0x40). */
struct CmnTaskMan
{
    int  addTask(int group, int start, int prio, int frames, CmnITask *task, int arg,
                 int *id, int user);                                     // ROR 0x001e71fc
    void exec();                                                         // ROR 0x001e7a20
    /* Non-zero while the task id still runs. */
    void *getTask(int id);                                               // ROR 0x001e7d7c
    /* Queues the task id for removal (once; -1 is ignored). */
    void killTask(int id);                                               // ROR 0x001e7724
    /* Pauses (on) / resumes the task id (-1 ignored). */
    void pauseTask(int id, int on);                                      // ROR 0x001e7414
    /* Pauses / resumes every task whose kind is in kinds (-1 ends the
     * list; a list starting with -1 matches nothing). */
    void pauseKinds(const int *kinds, int on);                           // ROR 0x001e747c
    /* Pauses / resumes every task but those of kinds 1, 2, 5, 7, 0xf,
     * 0x11, 0x12, 0x17. */
    void pauseOthers(int on);                                            // ROR 0x001e7544

    SysTimer m_timer;           // ROR 0x40 (bss 0x7a3dc0)
};

extern CmnTaskMan g_taskMan;    /* bss 0x7a3d80 */

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

/* Swaps a sound id 0x0a00xxxx..0x0a02xxxx for its twin in the column the
 * game's language / version picks (table data 0x295f08). Other ids pass. */
u_int utlConvSndId(u_int id);                                            // ROR 0x00106658
/* v[3] = 0, then v normalised in place (sceVu0Normalize). */
void  utlNormalize3(float *v);                                           // ROR 0x00106db0
extern EmyManager g_emyManager; /* bss 0x7b2140 */
extern int g_emyManagerTaskId;  /* bss 0x7b45c0: addTask's id for EmyManager (read by 0x1c58c0) */

/* Words main sets once the boot load is over; the game modes (0x19a824,
 * 0x1a92a0, 0x1b099c, 0x1bde58, 0x1a98fc) read and write them. Roles not
 * known yet. */
extern int g_bss_7a3d48;        /* bss 0x7a3d48: main sets 1 */
extern int g_bss_7a3d4c;        /* bss 0x7a3d4c: main sets 1 */

/* The object at bss 0x7b40c0 (constructor 0x1810f4: sizes 0x200 / 0x180 /
 * 0xe00, between snd/sndman.cpp and snd/rxmpeg.cpp): a movie player, class
 * name unverified. The words are ints, so the EE offsets hold on the host.
 * layout: host -- only the words MagSystem and S2dMovie use. */
struct I3dLayer;

struct RxMpeg
{
    void open();                                                         // ROR 0x00181150
    void setFrameParity(int parity);                                     // ROR 0x00181204
    /* Draws the decoded frame into layer's packet. */
    void draw(I3dLayer *layer);                                          // ROR 0x001812a8
    /* Starts the movie name at w x h. */
    void play(const char *name, int w, int h);                           // ROR 0x0018154c
    /* Asks the library to stop; -1 when nothing plays. */
    int  stop();                                                         // ROR 0x00181644
    int  close();                                                        // ROR 0x001816b0
    /* The library's status word, -1 when not open (callers: 0 playing, 1 finished). */
    int  getStatus();                                                    // ROR 0x0018173c
    int  isEnd();                                                        // ROR 0x0018176c
    /* Once a frame from SndMan::update (frame parity and the decoder). */
    void update();                                                       // ROR 0x0018120c
    /* Empty in the ROM; SndMan::update calls it after rxSndUpdate. */
    void postUpdate();                                                   // ROR 0x00181494

    int  m_active;              // ROR 0x00: open sets it, close / the constructor clear it; MagSystem::waitVSync runs rxThVSync while set
    int  m_04;                  // ROR 0x04: 1 = S2dMovie::drawMovie draws a frame; RxMpeg::stop tests it
    int  _08[(0x44 - 0x08) / 4];
    char m_name[0x80];          // ROR 0x44: S2dMovie::playMovie copies the name here (0xc4 is the next word close reads)
};
static_assert(offsetof(RxMpeg, m_04) == 0x04, "m_04");
static_assert(offsetof(RxMpeg, m_name) == 0x44, "m_name");

extern RxMpeg g_rxMpeg;         /* bss 0x7b40c0 */

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

/* The screen fader (scene +0x41850, constructor 0x12f008, vtable 0x2e1ca8;
 * class name from rtti.py). layout: host -- a MagLife. */
struct MagLifeFader : MagLife
{
    /* State m_mode, time m_time. */
    void start(const MagFadeParam *p);                                   // ROR 0x0012f060
    /* State 4, word 1 -1. */
    void stop();                                                         // ROR 0x0012f080

    u_char _28[0x30 - 0x28];
    int    m_30;                // ROR 0x30: MagScene::getFaderWord30; role unknown
    int    _34;
    int    m_color;             // ROR 0x38: 0 for fade kind 0, 0xff for kind 4
};
static_assert(offsetof(MagLifeFader, m_30) >= 0x30, "m_30");
static_assert(offsetof(MagLifeFader, m_color) >= 0x38, "m_color");
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
struct Obj176a3c                /* scene +0x837e0, constructor 0x176a3c */
{
    /* Deletes entry no of its table (GamMan::ctlObj1cbb80). */
    void release(int no);                                                // ROR 0x0017756c

    int _00;
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

/* A word-table record: the text at +0x10. layout: host -- the text is a
 * pointer; the file format is not read yet. */
struct WrdEntry
{
    int         _00[4];
    const char *m_text;         // ROR 0x10: S2dMes::drawMapName's caption
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
};

extern WrdTable g_wrdTable;     /* data 0x298ee0 */

/* ---- the debug range before debug/mimu_debug.cpp (unassigned@0020eda4) ---- */

void debugNop(void);                                                     // ROR 0x002155a0

/* ---- after the assert handler 0x215a30 (unassigned@0020ff34) ---- */

/* Formats the three floats (3 digits) and prints nothing: a trace whose
 * output the retail build compiled out. */
void dbgTraceVector3(float x, float y, float z);                         // ROR 0x00215b5c
/* The same for four floats. */
void dbgTraceVector4(const float *v);                                    // ROR 0x00215bb4

#endif /* _LIB_UNASSIGNED_H */
