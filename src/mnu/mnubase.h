/* ==========================================================================
 *  mnu/mnubase.h
 *
 *  mnuBase (mnu/mnubase.cpp, 0x192ac0..0x1947a4): the polymorphic base of
 *  the 3D menus (MnuMap, MnuPBook, MnuDiary; vtable 0x2e4640, vptr +0x10c).
 *  A menu's init calls setupWork with its slot counts, names its pack at
 *  m_packName and opens it (openPack, slot 10), queues the pack's files
 *  with addFile, and its processPad loads them into the model / motion /
 *  texture slots step by step (updateLoad); term frees the slots, closes
 *  the pack (closePack, slot 11) and resets the state. The slot memory
 *  comes from the mnu pool: the 1 MB block mnuInitPool allocates (sdata
 *  0x29bfb4: a table of 128 {address, size, flags} blocks at its start),
 *  carved by mnuAlloc / mnuFree.
 *  The file rows are kinds 1 (an i3r model -> a model slot), 2 (an i3m
 *  motion -> a motion slot), 4 (a TIM2 -> a texture slot and a DogTexInfo
 *  record); bit 0x10 reads it waiting. The draw helpers (beginDraw ..
 *  endDraw) write VU1 sprite packets and draw the models with utlDrawI3r.
 *  Names ror-inferred except the class (the ELF's vtable).
 * ======================================================================== */
#ifndef _MNU_MNUBASE_H
#define _MNU_MNUBASE_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "sdk/vu0_intrin.h"
#include "game/gamfile.h"

struct BedPad;
struct DogTexInfo;
struct I3dDrawSet;
struct I3dLayer;
struct I3dMatrixF4x4;
struct I3mRoot;
struct I3rRoot;
struct MagScene;

/* A block of the mnu pool's table (EE 0x10: address, size, flags). The
 * pool (0x100800 bytes from g_utlMemory) starts with MNU_POOL_BLOCKS of
 * them; the first holds the 1 MB after the table, free. layout: host --
 * the address is a pointer (still 0x10 bytes). */
struct MnuPoolBlock
{
    u_char *m_addr;             // ROR 0x00
    u_int   m_size;             // ROR 0x04: bytes, a multiple of 16
    u_int   m_flags;            // ROR 0x08: MNU_BLOCK_*; 0 = an unused table entry
};
enum
{
    MNU_POOL_BLOCKS = 128,
    MNU_POOL_SIZE   = 0x100000,
    MNU_BLOCK_VALID = 1,        /* the entry describes a block */
    MNU_BLOCK_USED  = 2,        /* ... and it is allocated */
};
/* The pool (sdata 0x29bfb4), NULL outside mnuInitPool .. mnuTermPool. */
extern MnuPoolBlock *g_mnuPool;

/* Allocates the pool (MagScene::initRender, mnuBase::initSystem). */
void  mnuInitPool(void);                                                 // ROR 0x001944d0
/* Frees it (MagScene::term, mnuBase::termSystem). */
void  mnuTermPool(void);                                                 // ROR 0x00194548
/* A block of the mnu pool (16-byte granules, the best fit); NULL when
 * none fits or the table is full. */
void *mnuAlloc(u_int size);                                              // ROR 0x00194588
/* Releases the pool block at p (nothing when p is not one). */
void  mnuFree(void *p);                                                  // ROR 0x00194688
/* Merges neighbouring free blocks (setupWork runs it first). */
void  mnuMergeFree(void);                                                // ROR 0x001946e4

/* MnuFile::m_kind & MNU_FILE_KIND_MASK, and its flag. */
enum
{
    MNU_FILE_MODEL     = 1,     /* an i3r model -> model slot m_arg */
    MNU_FILE_MOTION    = 2,     /* an i3m motion -> motion slot m_arg */
    MNU_FILE_TEX       = 4,     /* a TIM2 -> texture slot m_arg */
    MNU_FILE_KIND_MASK = 0xf,
    MNU_FILE_WAIT      = 0x10,  /* read it waiting, not one poll per frame */
};

/* mnuBase::m_flags bits this file uses (the menus use more). */
enum
{
    MNU_INIT        = 0x01,     /* init done (isInit) */
    MNU_READING     = 0x04,     /* the head file row is being read */
    MNU_DROP_HEAD   = 0x08,     /* ... and is dropped when read */
    MNU_VOLUME_LOW  = 0x10,     /* lowerVolume silenced the game */
};

/* A pack file to load: the file list's rows (EE 0x30). layout: host --
 * the buffer is a pointer. */
struct MnuFile
{
    void  *m_buf;               // ROR 0x00: the read buffer (a pool block)
    int    m_size;              // ROR 0x04: the file's size in the pack, -1 unknown
    u_int  m_kind;              // ROR 0x08: MNU_FILE_*
    int    m_arg;               // ROR 0x0c: the slot; -1 in an empty row
    char   m_name[0x20];        // ROR 0x10
};
static_assert(offsetof(MnuFile, m_kind) >= 0x08, "m_kind");
static_assert(offsetof(MnuFile, m_name) >= 0x10, "m_name");

/* A model slot (EE 0x20): the i3r data and one work block of
 * nodes * 0xb2 + 2 bytes carved into the pose records, the local and
 * model matrices and the motion node map. layout: host -- pointers. */
struct MnuModel
{
    I3rRoot       *m_root;      // ROR 0x00: the file's data, relocated
    void          *m_pose;      // ROR 0x04: nodes * 0x30 (i3mGetPose); also the work block
    I3dMatrixF4x4 *m_local;     // ROR 0x08: nodes * 0x40
    I3dMatrixF4x4 *m_model;     // ROR 0x0c: nodes * 0x40
    u_short       *m_map;       // ROR 0x10: count, then a model node per motion node (bindMotion)
    int            m_size;      // ROR 0x14: the file's size (copyModel)
};
static_assert(offsetof(MnuModel, m_size) >= 0x14, "m_size");

/* A motion slot (EE 0x10). layout: host -- a pointer. */
struct MnuMotion
{
    I3mRoot *m_root;            // ROR 0x00: the file's data, relocated
    int      m_frames;          // ROR 0x04: its length * 30
};
static_assert(offsetof(MnuMotion, m_frames) >= 0x04, "m_frames");

/* A texture slot (EE 0x10). layout: host -- a pointer. */
struct MnuTex
{
    void *m_tm2;                // ROR 0x00: the TIM2 (a pool block)
    int   m_size;               // ROR 0x04
    int   m_tex;                // ROR 0x08: its DogTexInfo record, -1 none
    int   m_set;                // ROR 0x0c: its texture in the draw set (setDrawSet), -1 none
};
static_assert(offsetof(MnuTex, m_set) >= 0x0c, "m_set");

/* A 2D picture drawSprite draws (EE 0x34 read). EE layout: the menus'
 * own tables. */
struct MnuSprite
{
    float m_x, m_y;             // ROR 0x00: the pivot within the picture
    float m_w, m_h;             // ROR 0x08: its size in screen pixels
    /* The books' tables settle the order: +0x10 holds (0, 0, 1, 0.875) for
     * a 512 x 448 page on a 512 x 512 texture, +0x20 (1, 1, 1, 1). The ROM
     * hands +0x10 to 0x21e488 and +0x20 to 0x21e690, so those two names
     * (i3dPktVu1Color / i3dPktVu1Sprite's uv) are likely swapped
     * (open-items.md). */
    float m_uv[4];              // ROR 0x10: its texture rectangle u0, v0, u1, v1
    float m_color[4];           // ROR 0x20: the sprite colour
    float m_angle;              // ROR 0x30: about z, radians
};
static_assert(offsetof(MnuSprite, m_w) == 0x08, "m_w");
static_assert(offsetof(MnuSprite, m_uv) == 0x10, "m_uv");
static_assert(offsetof(MnuSprite, m_color) == 0x20, "m_color");
static_assert(offsetof(MnuSprite, m_angle) == 0x30, "m_angle");

/* layout: host -- polymorphic (the EE vptr is at +0x10c, the host's at 0;
 * pointers double): EE offsets in the comments. EE size 0x110.
 * EE slots: 1 ~dtor 0x192b3c, 2 init (pure), 3 term 0x192d60, 4 isInit,
 * 5 processPad (pure), 6 draw (pure), 7 setDrawSet 0x193140, 8 getType,
 * 9 getFlags, 10 openPack 0x1938a8, 11 closePack 0x193920. */
struct mnuBase
{
    // ROR 0x00192ac0
    mnuBase()
    {
        m_type = 0;
        m_numTexs = 0;
        m_10 = -1;
        m_step = -1;
        m_flags = 0;
        m_scene = NULL;
        m_tex = NULL;
        m_files = NULL;
        m_maxFiles = 0;
        m_numFiles = 0;
        m_packBuf = NULL;
        m_packName[0] = '\0';
        m_models = NULL;
        m_numModels = 0;
        m_motions = NULL;
        m_numMotions = 0;
        m_texs = NULL;
    }
    /* EE slot 1 (+0x08). */
    // ROR 0x00192b3c (the member FileReader's destructor runs after it)
    virtual ~mnuBase() { mnuBase::term(); }
    /* EE slot 2 (+0x10) */
    virtual void init(DogTexInfo *tex) = 0;
    /* EE slot 3 (+0x18): frees the slots and the file list, closes the
     * pack, clears the flags; m_10 / m_step back to -1. */
    virtual void term();                                                 // ROR 0x00192d60
    /* EE slot 4 (+0x20): bit 0 of the flags. */
    // ROR 0x0028dec0
    virtual int  isInit() { return m_flags & MNU_INIT; }
    /* EE slot 5 (+0x28) */
    virtual int  processPad(BedPad *pad) = 0;
    /* EE slot 6 (+0x30) */
    virtual void draw(I3dLayer *layer) = 0;
    /* EE slot 7 (+0x38): numbers the loaded textures and makes the layer
     * draw set of them (m_drawSet); beginDraw runs it. */
    virtual void setDrawSet(I3dLayer *layer);                            // ROR 0x00193140
    /* EE slot 10 (+0x50): reads the first 0x1000 bytes of m_packName (the
     * pack's directory) into m_packBuf, keeping the file open. */
    virtual void openPack();                                             // ROR 0x001938a8
    /* EE slot 11 (+0x58): closes the pack and frees m_packBuf. */
    virtual void closePack();                                            // ROR 0x00193920

    /* Carves one pool block into nFiles file rows and nModels / nMotions
     * / nTexs slots and keeps tex. */
    void setupWork(DogTexInfo *tex, int nFiles, int nModels, int nMotions,
                   int nTexs);                                           // ROR 0x00192ba4
    /* Queues name (kind, arg) unless a row of that kind & 0xf and arg is
     * there already or the list is full. */
    void addFile(const char *name, u_int kind, int arg);                 // ROR 0x0019397c
    /* GamMan::switchVolume(0) and flag 0x10, unless a volume is saved. */
    void lowerVolume();                                                  // ROR 0x0019381c
    /* Undoes lowerVolume (flag 0x10 set: switchVolume(1)). */
    void restoreVolume();                                                // ROR 0x00193868

    /* The menu's camera: GamMan::pushCamera, the scene camera reset with
     * lens angle, the view far set (the old one kept); popCamera undoes. */
    void pushCamera(float viewFar, float angle);                         // ROR 0x00192e94
    void popCamera();                                                    // ROR 0x00192f3c
    /* A menu draw: beginDraw takes the camera and opens the layer set and
     * the VU1 sprite packet; useTex / drawSprite / drawModel write into
     * it; endDraw closes it. */
    void beginDraw(I3dLayer *layer);                                     // ROR 0x00192f68
    void endDraw(I3dLayer *layer);                                       // ROR 0x001930f8
    void useTex(int n);                                                  // ROR 0x00193210
    /* spr at screen (x, y) (512 x 448). */
    void drawSprite(float x, float y, const MnuSprite *spr);             // ROR 0x00193258
    /* Model slot n with matrix world; motion >= 0 poses it at frame. */
    void drawModel(I3dLayer *layer, sceVu0FVECTOR *world, int n, int motion,
                   int frame);                                           // ROR 0x0019368c
    /* drawModel with a matrix from rot (ZXY), scale and pos. */
    void drawModelRot(I3dLayer *layer, const float *pos, const float *rot,
                      const float *scale, int n, int motion, int frame); // ROR 0x001935c4
    /* A screen position (512 x 448) to the view plane. */
    void toView(float *out, const float *in);                            // ROR 0x001937c0

    /* The loader processPad polls: finishes the head row's read into its
     * slot, then starts the next row's (returns while one runs). */
    void updateLoad();                                                   // ROR 0x00193a1c
    /* Unqueues the row (kind, arg); the head one is dropped when read. */
    void cancelFile(u_int kind, int arg);                                // ROR 0x00193d1c
    /* Unqueues every row but the head, which is dropped when read. */
    void cancelLoad();                                                   // ROR 0x00193dfc
    void clearFile(int n);                                               // ROR 0x00193e20
    /* A pack file read whole into a pool block (waiting); NULL when the
     * pack is not open or a row is being read. */
    void *readFile(const char *name, int *size);                         // ROR 0x00193e7c
    void freeData(void *p);                                              // ROR 0x00193f74

    void freeModel(int n);                                               // ROR 0x00193f98
    /* Model n's node map for motion slot motion. */
    void bindMotion(int n, int motion);                                  // ROR 0x00193fec
    /* The first empty model slot in lo..hi not queued, -1 none. */
    int  findFreeModel(int lo, int hi);                                  // ROR 0x001940a4
    /* Model src copied into slot dst when dst is free; 1 done. */
    int  copyModel(int dst, int src);                                    // ROR 0x00194148
    void freeMotion(int n);                                              // ROR 0x00194280
    void freeTex(int n);                                                 // ROR 0x00194370
    int  isTexLoaded(int n);                                             // ROR 0x001943d8
    /* The first texture slot in lo..hi neither loaded nor queued, -1. */
    int  findFreeTex(int lo, int hi);                                    // ROR 0x00194404

    int          m_type;        // ROR 0x00: getType 0x28dee0; the books store 7
    u_int        m_flags;       // ROR 0x04: MNU_* and the menus' own bits (2, 0x40000)
    MagScene    *m_scene;       // ROR 0x08: the wrapper's scene (its windows)
    DogTexInfo  *m_tex;         // ROR 0x0c: setupWork's tex
    int          m_10;          // ROR 0x10: -1
    int          m_step;        // ROR 0x14: the load / run step, -1 idle; init sets 0
    MnuFile     *m_files;       // ROR 0x18: the file list (setupWork); also the work block
    int          m_maxFiles;    // ROR 0x1c
    int          m_numFiles;    // ROR 0x20
    char         m_packName[0x40]; // ROR 0x24: the pack openPack reads
    void        *m_packBuf;     // ROR 0x64: its first 0x1000 bytes
    FileReader   m_reader;      // ROR 0x68
    FileReadReq  m_req;         // ROR 0x70..0x94
    MnuModel    *m_models;      // ROR 0x94
    int          m_numModels;   // ROR 0x98
    MnuMotion   *m_motions;     // ROR 0x9c
    int          m_numMotions;  // ROR 0xa0
    MnuTex      *m_texs;        // ROR 0xa4
    int          m_numTexs;     // ROR 0xa8
    I3dDrawSet  *m_drawSet;     // ROR 0xac: setDrawSet's
    u_long      *m_pkt;         // ROR 0xb0: the packet beginDraw opened
    sceVu0FMATRIX m_view;       // ROR 0xc0: the camera matrix, pushed back by the view far
    float        m_viewW;       // ROR 0x100: the view's width at the far plane
    float        m_viewH;       // ROR 0x104: its height
    float        m_saveFar;     // ROR 0x108: the view far pushCamera replaced
    /* ROR 0x10c: the EE vptr (the host's is at 0) */
};
static_assert(offsetof(mnuBase, m_flags) >= 0x04, "m_flags");
static_assert(offsetof(mnuBase, m_scene) >= 0x08, "m_scene");
static_assert(offsetof(mnuBase, m_tex) >= 0x0c, "m_tex");
static_assert(offsetof(mnuBase, m_step) >= 0x14, "m_step");
static_assert(offsetof(mnuBase, m_files) >= 0x18, "m_files");
static_assert(offsetof(mnuBase, m_packName) >= 0x24, "m_packName");
static_assert(offsetof(mnuBase, m_packBuf) >= 0x64, "m_packBuf");
static_assert(offsetof(mnuBase, m_reader) >= 0x68, "m_reader");
static_assert(offsetof(mnuBase, m_req) >= 0x70, "m_req");
static_assert(offsetof(mnuBase, m_models) >= 0x94, "m_models");
static_assert(offsetof(mnuBase, m_numTexs) >= 0xa8, "m_numTexs");
static_assert(offsetof(mnuBase, m_drawSet) >= 0xac, "m_drawSet");
static_assert(offsetof(mnuBase, m_pkt) >= 0xb0, "m_pkt");
static_assert(offsetof(mnuBase, m_view) >= 0xc0, "m_view");
static_assert(offsetof(mnuBase, m_saveFar) >= 0x108, "m_saveFar");

#endif /* _MNU_MNUBASE_H */
