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
};

/* The task manager, one global at bss 0x7a3d80. EE size at least 0x288.
 * layout: host -- only the timer member is placed (EE +0x40). */
struct CmnTaskMan
{
    int  addTask(int group, int start, int prio, int frames, CmnITask *task, int arg,
                 int *id, int user);                                     // ROR 0x001e71fc
    void exec();                                                         // ROR 0x001e7a20

    SysTimer m_timer;           // ROR 0x40 (bss 0x7a3dc0)
};

extern CmnTaskMan g_taskMan;    /* bss 0x7a3d80 */
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

    int  m_active;              // ROR 0x00: open sets it, close / the constructor clear it; MagSystem::waitVSync runs rxThVSync while set
    int  m_04;                  // ROR 0x04: 1 = S2dMovie::drawMovie draws a frame; RxMpeg::stop tests it
    int  _08[(0x44 - 0x08) / 4];
    char m_name[0x80];          // ROR 0x44: S2dMovie::playMovie copies the name here (0xc4 is the next word close reads)
};
static_assert(offsetof(RxMpeg, m_04) == 0x04, "m_04");
static_assert(offsetof(RxMpeg, m_name) == 0x44, "m_name");

extern RxMpeg g_rxMpeg;         /* bss 0x7b40c0 */

/* ---- the file reader (unassigned@001bde58: 0x1be2cc..0x1be880) ---- */

/* One read request: open the name, seek, read, close -- each step only
 * when its field asks for it (the reader clears the field once done).
 * EE 0x24. layout: host -- the name and buffer are pointers. */
struct FileReadReq
{
    // ROR 0x001be734 (memset 0x24 bytes, fd -1)
    FileReadReq()
    {
        memset(this, 0, sizeof(*this));
        m_fd = -1;
    }

    int         m_fd;           // ROR 0x00: the open file, -1 before the open step
    int         m_step;         // ROR 0x04: (step << 1) | waiting; FileReader::read resets it
    int         m_08;           // ROR 0x08: some callers set 1; the reader never reads it
    const char *m_name;         // ROR 0x0c: opened when set (rxSysStreamOpen, 0x8001)
    int         m_offset;       // ROR 0x10: the seek position
    int         m_whence;       // ROR 0x14: -1 = no seek, 0 / 1 / 2 = set / current / end
    void       *m_buf;          // ROR 0x18: read target
    int         m_size;         // ROR 0x1c: bytes to read, 0 = no read
    int         m_close;        // ROR 0x20: close the file after the read
};
static_assert(offsetof(FileReadReq, m_step) == 0x04, "m_step");
static_assert(offsetof(FileReadReq, m_name) >= 0x0c, "m_name");

/* Runs FileReadReqs (the pack loaders build one on the stack). EE 0x08.
 * layout: host -- holds a pointer. */
struct FileReader
{
    // ROR 0x001be2cc
    FileReader() : m_req(NULL), m_fd(-1) {}
    // ROR 0x001be2dc (GCC 2 destructor; the ROM's callers pass 2, no delete)
    ~FileReader()
    {
        if (m_fd >= 0)
            close();
    }

    /* Starts the request and, unless async, waits for it (drawing the
     * now-loading screen while it waits); returns the state machine's
     * result (1 = done). */
    int  read(FileReadReq *req, int async);                              // ROR 0x001be524
    int  close();                                                        // ROR 0x001be414

    FileReadReq *m_req;         // ROR 0x00: the request being run
    int          m_fd;          // ROR 0x04: the file this reader opened, -1 = none
};

/* The size of the file (rxSysStreamOpen, seek to the end), < 0 when it
 * does not open. */
int  fileGetSize(const char *theName);                                   // ROR 0x001be768
int  fileExists(const char *theName);                                    // ROR 0x001be84c

/* ---- the utl range after utl/utlgame.cpp (unassigned@00103edc) ---- */

void utlRandReset(void);                                                 // ROR 0x0010421c

/* ---- the i3d library (unassigned@00216ad0 .. 0x237d80; its version string
 * "i3d_package_version 050121-stable-050826") ---- */

/* The context and the VRAM cache have their own files (i3d/i3dctx.h,
 * i3d/i3dvram.h); what they call in the rest of the library is below. */
#include "i3d/i3dctx.h"
#include "i3d/i3dlayer.h"

int   i3dPrintf(const char *fmt);  /* printf-like in the ROM */          // ROR 0x00237c68
void  i3dSysInit(void);            /* empty in the ROM */                // ROR 0x00237178
int   i3dFlushCache(void);         /* FlushCache(0) */                   // ROR 0x00237cc8

/* The texture / CLUT objects and their GS transfers (i3dTexMakeUpload ...,
 * i3dPsmBits) have their own file. */
#include "i3d/i3dtex.h"

/* The draw sets, tags, register writes and primitive writers have their
 * own file. */
#include "i3d/i3dpacket.h"

/* Copies four words (no alignment needed). */
void  i3dVecCopyW(float *out, const float *in);                          // ROR 0x00236980
/* The i3d vector / matrix module (unassigned@00228c08; bridges of the
 * ROM's VU0 macro code until it is reconstructed). Matrices are row-vector:
 * p * i3dVu0MatMul(a, b) = (p * b) * a. Outputs may alias inputs. The ROM
 * has a second, byte-identical copy at 0x2372d0..0x237814 (i3dMatMul,
 * i3dMatInverse, i3dVecNeg ...), hence the Vu0 names here. */
void  i3dVecZero(float *v);                                              // ROR 0x00236960
void  i3dMatCopy(sceVu0FVECTOR *out, sceVu0FVECTOR *in);                 // ROR 0x00236990
void  i3dVu0VecNeg(float *out, const float *in);        /* all four */   // ROR 0x00236a68
void  i3dVecCross(float *out, const float *a, const float *b); /* w 0 */  // ROR 0x00236af0
void  i3dVecNormalize(float *out, const float *in);     /* xyz, w 0 */   // ROR 0x00236b38
/* The inverse of an affine matrix (3x3 part and translation). */
void  i3dVu0MatInverse(sceVu0FVECTOR *out, sceVu0FVECTOR *in);           // ROR 0x00236bd8
/* out row i = sum over j of b[i][j] * a[j] (VU0 micro 0). */
void  i3dVu0MatMul(sceVu0FVECTOR *out, sceVu0FVECTOR *a, sceVu0FVECTOR *b); // ROR 0x00236d30
/* out[0] = (m00, m11, m22, 0), out[1] = m[3]. */
void  i3dMatGetScaleTrans(sceVu0FVECTOR *out, sceVu0FVECTOR *m);         // ROR 0x00228c38
/* The unit matrix (data 0x303c10). */
sceVu0FVECTOR *i3dMatGetUnit(void);                                      // ROR 0x00237168

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

/* An i3d model root (tag_I3rRoot_s in DogModel's symbols). Opaque. */
struct I3rRoot;

/* i3d's matrix struct (tag_I3dMatrixF4x4_s in DogModel's symbols): the
 * same 16 floats as an sceVu0FMATRIX, but the ROM copies it with
 * unaligned ldl / sdl, so it is only 4-aligned. EE 0x40. */
struct I3dMatrixF4x4
{
    float m[4][4];
};
static_assert(sizeof(I3dMatrixF4x4) == 0x40, "I3dMatrixF4x4 is 0x40 bytes on the EE");

/* The node index of the named node, -1 when the model has none. */
int   i3rGetNodeIndexByName(I3rRoot *root, const char *name);            // ROR 0x0022b3d8

#define I3D_OK 0

/* An i3d STQ map entity (tag_I3rStqMapEntity_s in DogModel's symbols).
 * The scene keeps one at +0x2aa90 (less than 0x40 bytes before +0x2aad0).
 * layout: host -- placeholder, fields not known. */
struct I3rStqMapEntity
{
    u_char _00[0x40];
};
static_assert(sizeof(I3rStqMapEntity) >= 0x40, "placeholder");

/* I3D_OK when the root carries the i3r id string and version 0x52, 3
 * otherwise (the name is in dogmodel.h's assert text). */
int   I3rRoot__CheckIdString(const I3rRoot *root);                       // ROR 0x0022b228
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

/* i3d's vector / matrix helpers (VU0 macro code; the angle comes first in f12). */
void  i3dVecCopy(float *out, const float *in);                           // ROR 0x002372d0
void  i3dVecCopy2(float *out, const float *in);                          // ROR 0x002372e0
/* The second copy of i3dVecNormalize: xyz normalized, w 0. */
void  i3dVecNormalize2(float *out, const float *in);                     // ROR 0x002375d8
void  i3dVecAdd(float *out, const float *a, const float *b);             // ROR 0x002373d8
void  i3dVecMul(float *out, const float *a, const float *b);             // ROR 0x00237438
void  i3dVecMul2(float *out, const float *a, const float *b);  /* = Mul */ // ROR 0x00237450
void  i3dVecFtoi0(int *out, const float *in);                            // ROR 0x00237600
void  i3dVecFtoi4(int *out, const float *in);  /* 12.4 fixed */          // ROR 0x00237610
void  i3dVecSub(float *out, const float *a, const float *b);             // ROR 0x00237408
void  i3dVecScale(float s, float *out, const float *in);                // ROR 0x00237360
/* out = m x v (an sceVu0 row-vector product, w included). */
void  i3dMatApply(float *out, sceVu0FVECTOR *m, const float *v);        // ROR 0x002373a8
/* out = -in, all four components. */
void  i3dVecNeg(float *out, const float *in);                           // ROR 0x00237468
float i3dVecDot(const float *a, const float *b);                        // ROR 0x00237580
void  i3dMatMul(sceVu0FVECTOR *out, sceVu0FVECTOR *a, sceVu0FVECTOR *b); // ROR 0x002377d0
void  i3dMatUnit(sceVu0FVECTOR *m);                                      // ROR 0x00237340
void  i3dMatTranslate(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v); // ROR 0x00237640
void  i3dMatInverse(sceVu0FVECTOR *out, sceVu0FVECTOR *in);              // ROR 0x00237678
void  i3dMatRotX(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in);    // ROR 0x00237818
void  i3dMatRotY(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in);    // ROR 0x00237830
void  i3dMatRotZ(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in);    // ROR 0x00237848
/* A view matrix from an eye, a view direction and an up vector. */
void  i3dMatLookDir(sceVu0FVECTOR *out, const float *eye, const float *dir,
                    const float *up);                                    // ROR 0x00237ba0
/* The distance of two points (sqrt of 0x106e14). */
float utlVecDist(const float *a, const float *b);                        // ROR 0x00106e70

/* bss 0x7812ac, just below GamMan: mainInitGame passes it to
 * seqSetLanguage (which ignores it). Role not known yet. */
extern int g_bss_7812ac;

#include "mag/maglife.h"

struct DogTexInfo;
struct MagScene;

/* Scene members whose class has no name yet, named by their constructor's
 * address. layout: host -- only the words the scene touches. */
struct MagLife12f008 : MagLife  /* scene +0x41850, constructor 0x12f008 */
{
};
struct MagLife130da4 : MagLife  /* scene +0x418f0, constructor 0x130da4 */
{
    u_char _28[0x34 - 0x28];
    int    m_34;                // ROR 0x34: initRender copies the scene's +0x2910 into it
};
static_assert(offsetof(MagLife130da4, m_34) >= 0x34, "m_34");
struct Obj176a3c                /* scene +0x837e0, constructor 0x176a3c */
{
    int _00;
};
/* layout: host -- placeholder. */
struct Obj17c0f0                /* scene +0x8542c, constructor 0x17c0f0: a flag word, a buffer, four 0x18-byte records */
{
    int _00;
};
static_assert(sizeof(Obj176a3c) >= 4 && sizeof(Obj17c0f0) >= 4, "placeholders");

int   FUN_00176afc(Obj176a3c *obj, i3dContext *ctx, MagScene *scene);    // ROR 0x00176afc
void  FUN_00177830(Obj176a3c *obj, i3dContext *ctx);  /* MagScene::draw's world pass */ // ROR 0x00177830
void  FUN_0017c2c0(Obj17c0f0 *obj);  /* frees the buffer and the records */ // ROR 0x0017c2c0
/* Takes a sky pack's sector.sec (copied to the game heap when copy is
 * set); non-zero on success. */
int   FUN_0017c1f4(Obj17c0f0 *obj, void *data, int copy);               // ROR 0x0017c1f4
/* Allocates the 1 MB 2D packet buffer (data 0x29bfb4); mnuBase::initSystem
 * calls it too. */
void  FUN_001944d0(void);                                                // ROR 0x001944d0

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
