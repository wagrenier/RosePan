/* ==========================================================================
 *  mnu/mnubase.h
 *
 *  mnuBase (mnu/mnubase.cpp, 0x192ac0..0x1947a4): the polymorphic base of
 *  the 3D menus (MnuMap, MnuPBook, MnuDiary; vtable 0x2e4640, vptr +0x10c).
 *  A menu's init calls setupWork with its slot counts, names its pack at
 *  m_packName and opens it (openPack, slot 10), queues the pack's files
 *  with addFile, and its processPad loads them into the model / data /
 *  texture slots step by step; term frees the slots, closes the pack
 *  (closePack, slot 11) and resets the state. The slot memory comes from
 *  the mnu pool: the 1 MB 2D buffer FUN_001944d0 allocates (data
 *  0x29bfb4: a table of 128 {address, size, flags} blocks at its start,
 *  flags bit 0 free, bit 1 in use), carved by mnuAlloc / mnuFree.
 *  Names ror-inferred except the class (the ELF's vtable). Only the
 *  constructor and destructor are written (inline); the rest is declared
 *  for the menus that call it.
 * ======================================================================== */
#ifndef _MNU_MNUBASE_H
#define _MNU_MNUBASE_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "game/gamfile.h"

struct BedPad;
struct DogTexInfo;
struct I3dLayer;
struct MagScene;

/* A block of the mnu pool (16-byte granules); NULL when none fits. */
void *mnuAlloc(int size);                                                // ROR 0x00194588
/* Releases the pool block at p (nothing when p is not one). */
void  mnuFree(void *p);                                                  // ROR 0x00194688

/* A pack file to load: the file list's 0x30-byte rows (EE layout). */
struct MnuFile
{
    int    m_00;                // ROR 0x00
    int    m_04;                // ROR 0x04
    u_int  m_kind;              // ROR 0x08: low 4 bits = the file kind (4: a texture set)
    int    m_arg;               // ROR 0x0c: -1 in an empty row
    char   m_name[0x20];        // ROR 0x10
};
static_assert(sizeof(MnuFile) == 0x30, "EE 0x30");
static_assert(offsetof(MnuFile, m_name) == 0x10, "m_name");

/* layout: host -- polymorphic (the EE vptr is at +0x10c, the host's at 0;
 * pointers double): EE offsets in the comments. EE size 0x110.
 * EE slots: 1 ~dtor 0x192b3c, 2 init (pure), 3 term 0x192d60, 4 isInit,
 * 5 processPad (pure), 6 draw (pure), 7 0x193140, 8 getType, 9 getFlags,
 * 10 openPack 0x1938a8, 11 closePack 0x193920; 7 is declared when a TU
 * needs it. */
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
        m_datas = NULL;
        m_numDatas = 0;
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
    virtual int  isInit() { return m_flags & 1; }
    /* EE slot 5 (+0x28) */
    virtual int  processPad(BedPad *pad) = 0;
    /* EE slot 6 (+0x30) */
    virtual void draw(I3dLayer *layer) = 0;
    /* EE slot 10 (+0x50): reads the first 0x1000 bytes of m_packName (the
     * pack's directory) into m_packBuf, keeping the file open. */
    virtual void openPack();                                             // ROR 0x001938a8
    /* EE slot 11 (+0x58): closes the pack and frees m_packBuf. */
    virtual void closePack();                                            // ROR 0x00193920

    /* Carves one pool block into nFiles file rows and nModels / nDatas /
     * nTexs slots (0x20 / 0x10 / 0x10 bytes) and keeps tex. */
    void setupWork(DogTexInfo *tex, int nFiles, int nModels, int nDatas,
                   int nTexs);                                           // ROR 0x00192ba4
    /* Queues name (kind, arg) unless a row of that kind & 0xf and arg is
     * there already or the list is full. */
    void addFile(const char *name, u_int kind, int arg);                 // ROR 0x0019397c
    /* GamMan::switchVolume(0) and flag 0x10, when 0x7a34d4 is -1. */
    void lowerVolume();                                                  // ROR 0x0019381c
    /* Undoes lowerVolume (flag 0x10 set: switchVolume(1)). */
    void restoreVolume();                                                // ROR 0x00193868

    int          m_type;        // ROR 0x00: getType 0x28dee0; the books store 7
    u_int        m_flags;       // ROR 0x04: 1 init done, 2 / 0x10 / 0x40000 (see the menus)
    MagScene    *m_scene;       // ROR 0x08: the wrapper's scene (its windows)
    DogTexInfo  *m_tex;         // ROR 0x0c: setupWork's tex
    int          m_10;          // ROR 0x10: -1
    int          m_step;        // ROR 0x14: the load / run step, -1 idle; init sets 0
    MnuFile     *m_files;       // ROR 0x18: the file list (setupWork)
    int          m_maxFiles;    // ROR 0x1c
    int          m_numFiles;    // ROR 0x20
    char         m_packName[0x40]; // ROR 0x24: the pack openPack reads
    void        *m_packBuf;     // ROR 0x64: its first 0x1000 bytes
    FileReader   m_reader;      // ROR 0x68
    FileReadReq  m_req;         // ROR 0x70..0x94
    void        *m_models;      // ROR 0x94: 0x20-byte slots (i3r models, from processPad's loader)
    int          m_numModels;   // ROR 0x98
    void        *m_datas;       // ROR 0x9c: 0x10-byte slots
    int          m_numDatas;    // ROR 0xa0
    void        *m_texs;        // ROR 0xa4: 0x10-byte slots (DogTexInfo entries)
    int          m_numTexs;     // ROR 0xa8
    u_char       _ac[0x10c - 0xac];
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

#endif /* _MNU_MNUBASE_H */
