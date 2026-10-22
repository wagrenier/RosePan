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

int   i3dPrintf(const char *fmt);  /* printf-like in the ROM */          // ROR 0x00237c68
void  i3dSysInit(void);            /* empty in the ROM */                // ROR 0x00237178
int   i3dFlushCache(void);         /* FlushCache(0) */                   // ROR 0x00237cc8

/* GS transfer packets for the VRAM cache (block addresses: 64-word units). */
u_long *i3dTexMakeUpload(void *tex, u_long *pkt, u_int dst);             // ROR 0x00219c88
u_long *i3dClutMakeUpload(void *clut, u_long *pkt, u_int dst);           // ROR 0x00219d10
u_long *i3dTexMakeMove(void *tex, u_long *pkt, u_int src, u_int dst);    // ROR 0x00219d40
u_long *i3dClutMakeMove(void *clut, u_long *pkt, u_int src, u_int dst);  // ROR 0x00219d68

/* One GS register write (a DMA cnt tag, an A+D GIF tag, value / reg);
 * returns the position after it (pkt + 3 qwords). */
u_long *i3dPktSetReg(u_long *pkt, u_long reg, u_long value);             // ROR 0x0021b550
/* The packet's first tags (0x21b600 / 0x21b550); returns the chain cursor. */
u_long *i3dPktOpenChain(u_char *pkt);                                    // ROR 0x0021b698
/* Writes the no-texture registers into each used slot of a draw set. */
int   i3dBatchClearTextures(I3dDrawSet *set);                            // ROR 0x0021b288

/* A layer's light set, fog and view (EE layer +0x82c / +0x830 / +0x834).
 * Opaque: built by 0x11262c over the scene light pool, i3dCtxSetFog and
 * i3dCtxCreateView. */
struct I3dLight;
struct I3dView;
struct MagViewParam;

int   i3dLayerSetLight(I3dLayer *layer, I3dLight *light);                // ROR 0x00223240
int   i3dLayerSetFog(I3dLayer *layer, void *fog);                        // ROR 0x00223250
int   i3dLayerSetView(I3dLayer *layer, I3dView *view);                   // ROR 0x00223260
/* The layer group's packet cursor, and its end once the caller wrote it. */
u_long *i3dLayerBeginPacket(I3dLayer *layer);                            // ROR 0x00223378
int   i3dLayerEndPacket(I3dLayer *layer, u_long *end);                   // ROR 0x00223398
/* A view from a view record and a camera matrix, in the context's tail. */
I3dView *i3dCtxCreateView(i3dContext *ctx, MagViewParam *param, sceVu0FVECTOR *matrix); // ROR 0x00226c58

/* The draw groups and layers (0x2239b8..0x224d88). */
int   i3dLayerInit(I3dLayer *layer, i3dContext *ctx);                    // ROR 0x00223878
int   i3dBatchAppend(I3dGroup *dst, I3dGroup *src);                      // ROR 0x00223d48
I3dNode *i3dSortList(I3dNode *first, int order, int b, u_int count);     // ROR 0x00224368
int   i3dGroupInit(I3dGroup *group, i3dContext *ctx, int a, int b, int c); // ROR 0x002243a0
int   i3dCtxRunGroup(i3dContext *ctx, I3dGroup *group);                  // ROR 0x002246c8
int   i3dCtxCloseFrame(i3dContext *ctx);                                 // ROR 0x00224798
void  i3dGroupAddLayer(I3dGroup *group, u_int key, I3dGroup *layer);     // ROR 0x00224878
int   i3dTexRegsSet(u_long *regs, I3dVramHandle *tex, I3dVramHandle *clut,
                    u_int arg);                                          // ROR 0x00224b28
void  i3dTexRegsSetNone(u_long *regs);                                   // ROR 0x00224d88

/* The fog record i3dCtxSetFog turns into FOGCOL and the fog ramp: a
 * colour (0..1, clamped) and four floats. EE 0x20; float roles unverified. */
struct I3dFog
{
    sceVu0FVECTOR m_color;      // ROR 0x00
    float         m_param[4];   // ROR 0x10
};
static_assert(offsetof(I3dFog, m_param) == 0x10, "m_param");
static_assert(sizeof(I3dFog) == 0x20, "I3dFog is 0x20 bytes on the EE");

void  i3dFogInit(I3dFog *fog);     /* all zero */                        // ROR 0x0021a120

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

/* An i3d light description (i3dCtxCreateLight's input; MagScene keeps the
 * shadow light's at +0x2a9d0): an ambient colour, then five (list, count)
 * pairs of light kinds. EE 0x40. layout: host -- pointers. */
struct I3dLightDesc
{
    sceVu0FVECTOR m_ambient;    // ROR 0x00
    struct
    {
        void *m_list;           // ROR 0x10 + 8 k: the kind's lights
        int   m_count;          // ROR 0x14 + 8 k: 0 = the kind is not used
    } m_kind[5];
    int           _38[2];       // ROR 0x38
};
static_assert(offsetof(I3dLightDesc, m_ambient) == 0, "m_ambient");

void      i3dLightDescInit(I3dLightDesc *desc);                          // ROR 0x0021ac80
I3dLight *i3dCtxCreateLight(i3dContext *ctx, const I3dLightDesc *desc);  // ROR 0x0021b148
/* The fog state of a frame from a fog record; NULL on failure. */
void     *i3dCtxSetFog(i3dContext *ctx, const I3dFog *fog);              // ROR 0x0021a158

/* A layer's draw sets: the default one, the current one, one found (or
 * made) for a texture-set description (EE 0x18; i3dLayerFindDrawSet hashes
 * every word). layout: host -- a pointer. */
struct I3dTexSetDesc
{
    float  m_00;                // ROR 0x00: 1.0 (the run's depth key)
    int   *m_tex_no;            // ROR 0x04: the texture numbers
    u_int  m_tex_count;         // ROR 0x08
    u_int  m_0c;                // ROR 0x0c: unverified
    u_int  m_10;                // ROR 0x10: unverified
    u_int  m_14;                // ROR 0x14: unverified
};
static_assert(offsetof(I3dTexSetDesc, m_tex_count) >= 0x08, "m_tex_count");
I3dDrawSet *i3dLayerGetDrawSet(I3dLayer *layer);                         // ROR 0x00223270
int       i3dLayerSetDrawSet(I3dLayer *layer, I3dDrawSet *set);          // ROR 0x00223228
I3dDrawSet *i3dLayerFindDrawSet(I3dLayer *layer, const I3dTexSetDesc *desc); // ROR 0x00223558
/* Opens a packet node with a sort key in the current set; its packet. */
u_long   *i3dLayerBeginSet(I3dLayer *layer, float key);                  // ROR 0x00223298
void      i3dLayerEndSet(I3dLayer *layer, u_long *end);                  // ROR 0x002232f0
/* Counts a use of texture slot n; its GS register packet. */
u_long   *i3dDrawSetUseTex(I3dDrawSet *set, int n);                      // ROR 0x0021b260
u_long   *i3dPktRefTexRegs(u_long *pkt, u_long *regs);                   // ROR 0x0021b4f8
u_long   *i3dPktDmaTag(u_long *pkt, u_int qwc, u_int id, const void *addr,
                       u_int vif0, u_int vif1);                          // ROR 0x0021b490
u_long    i3dPackXYZ(u_int x, u_int y, u_int z);                         // ROR 0x0021c940
u_long    i3dPackRGBAQ(float q, u_int r, u_int g, u_int b, u_int a);     // ROR 0x0021c8c8
/* A blended sprite (PRIM 0x46) from two XYZ2 corners and an RGBAQ. */
u_long   *i3dPktSprite(u_long *pkt, u_long xyz0, u_long xyz1, u_long rgbaq); // ROR 0x0021c140
/* Strips of count vertices with an optional matrix, STQ, colours and
 * positions (EE vectors); primitives unverified. */
u_long   *i3dPktDrawStrip(u_long *pkt, sceVu0FVECTOR *matrix, int count, const void *stq,
                          const void *colors, const void *xyz, int flags); // ROR 0x0021db18
u_long   *i3dPktDrawStripCtx(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix, int count,
                             const void *stq, const void *colors, const void *xyz,
                             int flags);                                 // ROR 0x0021dd10
u_long   *i3dPktDrawLineStrip(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix, int count,
                              const void *color, const void *xyz, int flags); // ROR 0x0021e1d8
/* MagScene::drawStrips' record types 4, 3 and 5; roles unknown. */
int       FUN_00222f88(I3dLayer *layer, void *a, void *b, void *rec);    // ROR 0x00222f88
int       FUN_00223050(void *a, I3dLayer *layer, void *b, const float *c); // ROR 0x00223050
int       FUN_002230e8(I3dLayer *layer, void *a, const float *b, void *c, void *rec); // ROR 0x002230e8

/* i3d's vector / matrix helpers (VU0 macro code; the angle comes first in f12). */
void  i3dVecCopy(float *out, const float *in);                           // ROR 0x002372d0
void  i3dVecCopy2(float *out, const float *in);                          // ROR 0x002372e0
void  i3dVecSub(float *out, const float *a, const float *b);             // ROR 0x00237408
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
