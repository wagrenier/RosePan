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

/* Type 4, BdsMain: emy/emy_bds.h. */
#include "emy/emy_bds.h"

/* Type 5, BdlMain: emy/emy_bdl.h. */
#include "emy/emy_bdl.h"

/* Type 6, HfmMain: emy/emy_hfm.h. */
#include "emy/emy_hfm.h"

/* Type 7, MrtMain: emy/emy_mrt.h. */
#include "emy/emy_mrt.h"

/* Type 8, ClrMain: emy/emy_clr.h. */
#include "emy/emy_clr.h"

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

/* Type 10, NorMain: emy/emy_nor.h. */
#include "emy/emy_nor.h"

/* EmyMoveParam, EmyTurnParam, TskEnemyMove, TskEnemyTurn: task/tsk_enemy.h. */
#include "task/tsk_enemy.h"

/* ChrMotionRef, the Tsk* script tasks, the chr* helpers: task/tsk_chr.h. */
#include "task/tsk_chr.h"

/* TskSndParam, TskSnd, TskCamera, scnChrRequest, scenePlayStart: task/tsk_scene.h. */
#include "task/tsk_scene.h"

/* 1 when the two strings are equal, a..z compared as A..Z, else 0
 * (unassigned@00106010, after utl/utlfile.cpp). */
int   utlStrEqualNoCase(const char *a, const char *b);                   // ROR 0x00106010
/* The vector helpers, utlConvSndId and the i3r wrappers: utl/utlmath.h. */
#include "utl/utlmath.h"
/* utlAtan2, utlDirDist, utlStrToLower, utlStrNCopy ...: utl/utlmisc2.h. */
#include "utl/utlmisc2.h"
/* The SE no of the enemy oid (0x163ea0) played at its position (0x16a594);
 * returns the SE's SndMan id
 * (unassigned@00164fc0). */
int  emyPlaySE(u_int oid, int no);                                        // ROR 0x0016a8a4
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

/* ---- the font rasterizer: moved to game/gamfont.h ---- */
#include "game/gamfont.h"

/* The tail of utl/utlgame.cpp (0x103edc..0x1041c4: the dog / enemy
 * wrappers) is in its header; the random numbers after it are
 * utl/utlrand.cpp. */
#include "utl/utlgame.h"
#include "utl/utlrand.h"

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

/* The curves, tracks, blend targets and vertex refs (i3rTrackGetValue,
 * i3rBlendVertices ...) have their own file. */
#include "i3d/i3rblend.h"

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

/* The i3c casts and their query: i3d/i3cquery.h. */
#include "i3d/i3cquery.h"
/* The hit callbacks (unassigned@00113ea8): keep the nearest hit. */
int   dogModelHitNearest(I3cQuery *q, int code, const u_short *prim, const float *verts,
                         float *plane, float *point);                               // ROR 0x00114034
int   dogModelHitCylinder(I3cQuery *q, int code, const u_short *prim, const float *verts,
                          float *plane, float *point);                              // ROR 0x001141c8


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
    int m_kind;                 // ROR 0x00: 0 MagLifeFader to black, 4 to colour 0xff, 1..3 MagLifeFadeKeyHole (the keyhole)
    int m_mode;                 // ROR 0x04: the fader's state to start in; 0 stops the fade
    int m_time;                 // ROR 0x08: frames
    int m_0c;                   // ROR 0x0c: role unknown (MagScene::update leaves it unset)
    int m_10;                   // ROR 0x10: role unknown (idem)
    int m_level;                // ROR 0x14: the fade object's level (MagScene::update passes 7)
};
static_assert(offsetof(MagFadeParam, m_level) == 0x14, "m_level");
static_assert(sizeof(MagFadeParam) == 0x18, "MagFadeParam is 0x18 bytes on the EE");

/* The keyhole fade at scene +0x41890 (MagLifeFadeKeyHole): mag/maglifefadekey.h. */
#include "mag/maglifefadekey.h"
/* The screen noise at scene +0x418f0 (MagLifeNoise): mag/maglifenoise.h. */
#include "mag/maglifenoise.h"
struct DogActor;
/* The effect manager at scene +0x837e0, its EffParam, and Eff0: eff/effman.h. */
#include "eff/effman.h"
/* The secta world (SctWorld at scene +0x8542c, its SctLayers, GamSecta):
 * sct/sctlayer.h. */
#include "sct/sctlayer.h"

/* mnuInitPool 0x1944d0 / mnuTermPool 0x194548 (the mnu pool): mnu/mnubase.h. */
/* i3dRaySegDist2 0x234050 / i3dPointSegDist2 0x2337e0 (the distance tests):
 * i3d/i3dgeom.h. */
#include "i3d/i3dgeom.h"

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

/* The options menu MnuOpt: mnu/mnuopt.h (not included here: it holds a
 * SprMan, and spr/sprman.h includes this file). */

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

/* The text list MnuTextList: mnu/mnuselect.h. */
#include "mnu/mnuselect.h"

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

/* debugNop 0x2155a0: sequence/sequence.h. */
#include "debug/mimu_debug.h"

#endif /* _LIB_UNASSIGNED_H */
