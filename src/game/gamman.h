/* ==========================================================================
 *  game/gamman.h
 *
 *  GamMan, the game manager (game/gamman.cpp; 181 inline methods in the
 *  ELF), one global at bss 0x7812c0. In the ROM it is polymorphic (vptr at
 *  +0x22ab4, slot 1 the destructor 0x1c0c8c); the virtuals arrive with the
 *  gamman TU. Members are named after the ELF's inline accessors where one
 *  reads them (getChapter +0x21f50, getVCount +0x21f60, getCurStat
 *  +0x21f40, ...); the rest by what the code does.
 * ======================================================================== */
#ifndef _GAME_GAMMAN_H
#define _GAME_GAMMAN_H

#include <stdint.h>

#include "game/gamsnd.h"
#include "sequence/sequence.h"

/* GamMan's member at +0x40 (bss 0x781300; its code is in
 * unassigned@001dd5e8, class name invented): a list of rects at +0xc290
 * tested against the PC's position. layout: host -- no members. */
struct GamArea
{
    void init();                                                         // ROR 0x001dd414
    /* Every frame while m_flags bit 2 is set: the rects against the PC. */
    void update(const float *pos, const float *rot);                     // ROR 0x001dee78
    void checkPc();                                                      // ROR 0x001deaac
    void startPcAction();                                                // ROR 0x001dea40
    /* The trigger word of the rect whose id matches (0 = every rect): op
     * 1 sets bits, 2 ors, 3 clears, 4 tests, 5 tests and clears
     * 0x20000000 (with GamMan::pause(0x8000)). */
    int  ctl_trg_flg(int op, int id, u_int bits);                        // ROR 0x001e0c2c
};

/* A game object's state (class ObjStat, vtable 0x2e4828, vptr at +0x258;
 * ACHRstat / FCHRstat derive from it; its code is in unassigned@001d3b44).
 * The ELF's inlines read +0x100 (oid), +0x108 (the scene actor, -1 none),
 * +0x190 position, +0x1a0 rotation. layout: host -- no members. */
struct ObjStat
{
    /* The actor's position into +0x190 and out (x, y, z, w); moves the
     * object to another secta cell when it left the current one. */
    void updatePos(float *out);                                          // ROR 0x001d8f04
    /* The actor's rotation (or +0x1a0 without one) into out[0..2]. */
    void getRot(float *out);                                             // ROR 0x001d8964
    /* Refreshes position and rotation, then copies +0x100..+0x1a0 to out. */
    int  save(void *out);                                                // ROR 0x001d96ec
    /* +0x100, the object id (the ELF's inline). */
    int  getOID() const;                                                 // ROR 0x0028f92c
};

/* The system pack's file 0 (GamMan +0x22220; class name invented): ten
 * pointers past the 0x40-byte header from its offsets +0x10..+0x34, the
 * data at +0x58. layout: host -- no members. */
struct GamSysData
{
    void set(const u_char *data);                                        // ROR 0x001dbef8
};

/* One of the 12 item slots (GamMan +0x1a700, getItemHavePtr's vector;
 * EE 0x10). clearItemHave sets only the id: the rest is stack garbage in
 * the ROM, zero on the host. */
struct GamItemSlot
{
    int   _0[2];
    short m_id;                 // ROR 0x8: 0xa00 = none
    short _a;
    int   _c;
};
static_assert(sizeof(GamItemSlot) == 0x10, "GamItemSlot");
static_assert(offsetof(GamItemSlot, m_id) == 0x8, "m_id");

/* The vector at +0x1a6c0: begin, end, capacity, the 12 slots at +0x40.
 * layout: host -- pointers. */
struct GamItemHave
{
    GamItemSlot *m_begin;       // ROR 0x0
    GamItemSlot *m_end;         // ROR 0x4
    int          m_capacity;    // ROR 0x8: 0xc
    GamItemSlot  m_slot[12];    // ROR 0x40
};

/* The stage's object table (W000.WRD file 1, +0x18bcc; EE 0x5020): a
 * 0x20-byte header with the count at +8, then 0x50-byte records. */
struct GamObjTable
{
    int      _0[2];
    u_short  m_num;             // ROR 0x08
    u_short  _a;
    int      _c[4];
    int      m_version;         // ROR 0x1c: 1 = the current record size (0x50), else 0x30
    u_char   m_rec[0x100][0x50];// ROR 0x20
};
static_assert(offsetof(GamObjTable, m_num) == 0x08, "m_num");
static_assert(offsetof(GamObjTable, m_version) == 0x1c, "m_version");
static_assert(offsetof(GamObjTable, m_rec) == 0x20, "m_rec");
static_assert(sizeof(GamObjTable) == 0x5020, "GamObjTable is 0x5020 bytes (init's allocDebug)");

/* Opaque pool records (EE data sizes; their classes come with the code
 * that fills them). */
struct GamRec4  { u_int _0; };
struct GamRec8  { u_int _0[2]; };
struct GamRec12 { u_int _0[3]; };
struct GamRec16 { u_int _0[4]; };
struct alignas(16) GamRec16A { u_int _0[4]; };
static_assert(sizeof(GamRec4) == 4, "GamRec4");
static_assert(sizeof(GamRec8) == 8, "GamRec8");
static_assert(sizeof(GamRec12) == 0xc, "GamRec12");
static_assert(sizeof(GamRec16) == 0x10, "GamRec16");
static_assert(sizeof(GamRec16A) == 0x10, "GamRec16A");

/* GamMan +0x22470: four records, id -1 = free (EE 0x30). */
struct GamRec22470
{
    int m_id;                   // ROR 0x00
    int _04[11];
};
static_assert(sizeof(GamRec22470) == 0x30, "GamRec22470 stride");

/* GamMan +0x22540: three records, -1.0 = unset (EE 0x20). */
struct GamRec22540
{
    float m_00;                 // ROR 0x00
    int   _04[7];
};
static_assert(sizeof(GamRec22540) == 0x20, "GamRec22540 stride");

/* EE size 0x22ab8. layout: host -- EE offsets in the comments. Items are
 * short ids, 0xa00 = none. */
struct GamMan
{
    GamMan();                                                            // ROR 0x001c05c0
    void init();                                                         // ROR 0x001c0d70
    /* Resets the game state: items, task ids, callbacks, the chr pools. */
    void initGameState();                                                // ROR 0x001c143c
    void loadSystemData();                                               // ROR 0x001c1b48
    void empty1c1a88();                                                  // ROR 0x001c1a88
    /* The old object table format: count 0x30-byte records into 0x50. */
    void convertObjTable(GamObjTable *dst, const GamObjTable *src, int count); // ROR 0x001c1f84
    void loadStage(int stage);                                           // ROR 0x001c204c
    /* The stage's sound packs (data/snd/xv/....se). */
    void loadStageSnd(int chapter);                                      // ROR 0x001c2f50
    void setPcStartPos();                                                // ROR 0x001c3d38
    void setStagePack(void *buf);                                        // ROR 0x001c46f4
    /* Sequence::reapThreads' calls when thread 0x44000000 (the stage
     * image's main thread) / 0x84000000 (image 2's) ends. */
    void onStageSeqEnd();                                                // ROR 0x001c4168
    void onSubSeqEnd();                                                  // ROR 0x001c43f0
    void empty1c4f30();                                                  // ROR 0x001c4f30
    void update();                                                       // ROR 0x001c526c
    void postUpdate();                                                   // ROR 0x001c53d8
    /* Stops the stage sound and the tasks before a menu opens; kind 1 / 2
     * also sets Sequence flag 1. */
    void enterMenu(int kind);                                            // ROR 0x001c5bf4
    /* Halts / restarts parts of the game by mask (0x100000 before the
     * menus; resume(4) once the PC's motion is over). */
    void resume(u_int mask);                                             // ROR 0x001c65e8
    void pause(u_int mask);                                              // ROR 0x001c6a40
    /* Copies one status word (which = 1.., see the switch) to *out. */
    void getStatus(int which, void *out);                                // ROR 0x001c6ccc
    /* Camera mode 1..5 through MagSystem::setCameraMode; 4 / 5 take map
     * camera no (below getMapCamNum). */
    void setCameraMode(int mode, u_int no);                              // ROR 0x001c7210
    /* The record count of the current sky slot's _c.bin (the cameras). */
    int  getMapCamNum();                                                 // ROR 0x001c7494
    void equipItem(short id);                                            // ROR 0x001c7a4c
    void addItem(short id, int arg);                                     // ROR 0x001c7e40
    void delItem(short id, int arg);                                     // ROR 0x001c82f8
    void delItemAll(short id);                                           // ROR 0x001c8414
    /* The 12 item slots, all 0xa00. */
    void clearItemHave();                                                // ROR 0x001c845c
    void selectItem(short id);                                           // ROR 0x001c8598
    int  hasItem(short id, int withStock);                               // ROR 0x001c8a28
    int  getItemCount(short id, int withStock);                          // ROR 0x001c8ae8
    int  canAddItem(int arg, short id);                                  // ROR 0x001c8ca4
    int  addStock(short id, int count);                                  // ROR 0x001c8f7c
    int  delStock(short id, int one);                                    // ROR 0x001c94d8
    /* Empty in the US build (before the map menu). */
    void beforeMapMenu();                                                // ROR 0x001c98b8
    /* The options: 0 brightness, 1 master volume, 2 stream volume, 3 SE
     * volume (levels 0..8), 4 / 5 read 2 / 3 into *(int *)arg, 6 the
     * sound output mode, 7 vibration. Returns -1. */
    int  setOption(u_int no, intptr_t arg);                              // ROR 0x001c9c1c
    /* Keeps a script image {w, h, ...} in slot. */
    void setImage(int slot, u_char *image);                              // ROR 0x001cbdfc
    void releaseChr(int arg);                                            // ROR 0x001ccc30
    /* Sequence::exec's last call: the stage-change requests in flags2. */
    void afterSeqExec();                                                 // ROR 0x001cd25c
    void storePcTransform();                                             // ROR 0x001cd404
    void setSysMsgHandle(int handle);                                    // ROR 0x001cfb38
    void resetObj(int oid, int arg);                                     // ROR 0x001d1410
    /* MagScene's open menu id != -1. */
    int  isMenuOpen();                                                   // ROR 0x001d1a48
    void markReset();                                                    // ROR 0x001d3588
    /* 0: saves the volume in m_saved_volume and silences; 1: restores
     * the saved volume (when one is saved) and marks none saved. */
    void switchVolume(int on);                                           // ROR 0x001d3ac8
    void setVolume(int volume);                                          // ROR 0x001d3a38
    /* Returns 1; nothing else in the US build. */
    int  resetExtra();                                                   // ROR 0x001d84a8

    /* An {a, b} pair the system calls 0x32..0x35 set (in the ROM b, the
     * script's a2, comes first): the ELF's regist_call_back_ehon / _menu /
     * _seq_evt and regist_jump_area_act. layout: host (two ints, EE 8). */
    struct SeqPair
    {
        int m_b;                // ROR 0x0: the script's a2
        int m_a;                // ROR 0x4: the script's a1
    };

    int          m_rate;            // ROR 0x00: getRate; the constructor sets 1
    GamArea      m_area;            // ROR 0x40 (bss 0x781300)
    void        *m_18bc0;           // ROR 0x18bc0: 0x600 bytes (init)
    void        *m_18bc4;           // ROR 0x18bc4: 0xc00 bytes (init)
    GamObjTable *m_objTable;        // ROR 0x18bcc: W000.WRD file 1
    u_char      *m_objRec;          // ROR 0x18bd0: m_objTable + 0x20, its records
    int          m_18dc4;           // ROR 0x18dc4
    int          m_18dc8;           // ROR 0x18dc8
    GamSndMan    m_sndMan;          // ROR 0x18e00 (bss 0x79a0c0): the sound table
    u_char      *m_wrdFile2;        // ROR 0x1a680: 0x4000 bytes, W000.WRD file 2
    u_int        m_1a684;           // ROR 0x1a684: 0xcccccccc after init
    GamItemHave  m_itemHave;        // ROR 0x1a6c0: getItemHavePtr
    SeqFixedPool<GamRec16A, 0x14e> m_itemStock; // ROR 0x1a7c0: getItemStockPtr
    int          m_1d200;           // ROR 0x1d200
    SeqFixedPool<GamRec16, 4>      m_pool1d240; // ROR 0x1d240
    int          m_mapModelNo;      // ROR 0x1d300: the scene sky slot (no < MAX_MAP_MODEL)
    SeqFixedPool<GamRec4, 0x80>    m_achrList;  // ROR 0x1d340: getAchrListPtr
    SeqFixedPool<GamRec4, 0x40>    m_fchrList;  // ROR 0x1d980: getFchrListPtr
    SeqFixedPool<GamRec4, 0x180>   m_vfchrList; // ROR 0x1dcc0: getVFchrListPtr
    SeqFixedPool<GamRec4, 0x40>    m_avfchrList;// ROR 0x1ef00: getAVFchrListPtr
    u_char      *m_1f240;           // ROR 0x1f240: 0x5a4 bytes, zeroed by init
    int          m_1f244;           // ROR 0x1f244
    u_char      *m_1f248;           // ROR 0x1f248: 0x42000 bytes
    u_char       m_1f24c[0x180];    // ROR 0x1f24c
    SeqFixedPool<GamRec12, 0x80>   m_pool1f880; // ROR 0x1f880
    SeqFixedPool<GamRec8, 0x80>    m_pool202c0; // ROR 0x202c0
    SeqFixedPool<GamRec8, 0x80>    m_pool20b00; // ROR 0x20b00
    int          m_cur_stat;        // ROR 0x21f40: getCurStat; 0 system data, 1 a stage loading
    u_int        m_flags;           // ROR 0x21f48: getSysFlg; 1 one second passed (isOneSec), 0x400 setExistMenu, 0x40000 the reset frame
    u_int        m_flags2;          // ROR 0x21f4c: chkSysFlg2; 0x2000 / 0x4000 read by system call 0x22
    int          m_chapter;         // ROR 0x21f50: getChapter, the stage loadStage loaded
    int          m_21f54;           // ROR 0x21f54
    int          m_vcount;          // ROR 0x21f60: getVCount, frames since init
    int          m_cameraMode;      // ROR 0x21f6c: getCameraMode
    int          m_21f70;           // ROR 0x21f70: setCameraMode's third word
    u_int        m_21f74;           // ROR 0x21f74: the map camera mode 4 set last, -1 none
    u_int        m_21f78;           // ROR 0x21f78
    int          m_optOutput;       // ROR 0x21f8c: option 6
    int          m_optVibration;    // ROR 0x21f90: option 7
    int          m_21f94;           // ROR 0x21f94
    int          m_optMasterLevel;  // ROR 0x21f98: option 1's level (0..8)
    int          m_optBrightLevel;  // ROR 0x21f9c: option 0's level
    int          m_masterVolume;    // ROR 0x21fa0: option 1's level scaled to 32..224
    int          m_optSeLevel;      // ROR 0x21fa4: option 3 / 5
    int          m_optStrLevel;     // ROR 0x21fa8: option 2 / 4
    float        m_21fbc[3];        // ROR 0x21fbc: 35, 40, 55
    int          m_21fcc;           // ROR 0x21fcc: releaseChr's argument for the PC
    int          m_21fd0;           // ROR 0x21fd0: ... for the dog
    int          m_inuMode;         // ROR 0x21fd4: getInuMode
    int          m_pcActor;         // ROR 0x21fdc: getPcChrNo, the PC's actor number in the scene
    int          m_21fe0;           // ROR 0x21fe0
    int          m_pcMotion;        // ROR 0x21fe4: get_pc_last_mot, the motion system call 0x2a waits on
    int          m_dogChrNo;        // ROR 0x21fe8: getDogChrNo
    int          m_murderChrNo;     // ROR 0x21fec: getMurderChrNo
    int          m_pcJnfc;          // ROR 0x21ff0: is_pc_jnfc
    int          m_21ff4;           // ROR 0x21ff4
    int          m_21ff8;           // ROR 0x21ff8
    int          m_22004;           // ROR 0x22004
    int          m_22008;           // ROR 0x22008
    int          m_pcEquipItemChrID;// ROR 0x2200c: getPcEquipItemChrID
    int          m_inuTask;         // ROR 0x22010: get_inu_task
    float        m_pcPos[4];        // ROR 0x22020: getPcPos
    float        m_pcRot[4];        // ROR 0x22030: getPcRot
    float        m_dogPos[4];       // ROR 0x22040: getDogPos
    int          m_sysWord[4];      // ROR 0x22050: setPCPosAMJ; system call 0x67 copies four words here
    ObjStat     *m_pcPtr;           // ROR 0x22090: getPCPtr
    ObjStat     *m_dogPtr;          // ROR 0x22094: getDogPtr
    ObjStat     *m_murderPtr;       // ROR 0x22098: getMurderPtr
    u_char       m_pcSave[0xb0];    // ROR 0x220a0: ObjStat::save of the PC across a stage change
    u_char       m_dogSave[0xa0];   // ROR 0x22150: ... of the dog
    int          m_pcEquipItemOID;  // ROR 0x22208: getPcEquipItemOID
    short        m_equipItem;       // ROR 0x2220c: getPcEquipItem, 0xa00 = none
    short        m_findTargetItem;  // ROR 0x2220e: getFindTargetItem
    int          m_saved_volume;    // ROR 0x22214: chkNoise; -1 = none saved (SprMan::volumeOn / Off read it)
    int          m_volume;          // ROR 0x22218: setVolume's last volume (0x80 after init)
    GamSysData   m_sysData;         // ROR 0x22220: the system pack's file 0
    int          m_sysMsgHandle;    // ROR 0x2227c: getSysMsgHND
    int          m_22280[7];        // ROR 0x22280: 10, 10, 0xffff, 0x100, 0x100, 0, 0 (initGameState)
    int          m_msgTaskId;       // ROR 0x222a0: getMsgTaskID
    int          m_selectTaskId;    // ROR 0x222a4: getSelectTaskID
    int          m_222a8;           // ROR 0x222a8: a task id
    int          m_frameTaskId;     // ROR 0x222ac: getSeqEvtTaskID, the TskFrameFunc the system call starts
    int          m_sysTaskId;       // ROR 0x222b0: getSysTaskID, the TskSysFunc of system call 0x22
    int          m_movieTaskId;     // ROR 0x222b4: getMovieTaskID
    int          m_camTaskId;       // ROR 0x222b8: getCamTaskID
    int          m_bgLoadTaskId;    // ROR 0x222bc: getBGLoadTaskID
    int          m_itemUseTaskId;   // ROR 0x222c0: getItemUseTaskID
    SeqPair      m_seqPair[4];      // ROR 0x222c4: system calls 0x35, 0x32, 0x33, 0x34
    u_char       m_222e4[0x80];     // ROR 0x222e4
    intptr_t     m_seqWord;         // ROR 0x22364: setEmyParam; system call 0x15 stores its a1
    char         m_2236c[4][0x40];  // ROR 0x2236c: four strings, emptied by initGameState
    SeqFixedPool<GamRec16, 0x100> *m_pool2246c; // ROR 0x2246c: init's new (0x1840 bytes)
    GamRec22470  m_22470[4];        // ROR 0x22470: stride 0x30
    int          m_22530;           // ROR 0x22530
    GamRec22540  m_22540[3];        // ROR 0x22540: stride 0x20
    int          m_msgMode;         // ROR 0x225d8: chgMsgMode
    int          m_225e4;           // ROR 0x225e4
    int          m_225e8;           // ROR 0x225e8
    int          m_225f0;           // ROR 0x225f0
    int          m_225f4[0x1a];     // ROR 0x225f4: init's table (0x7f, 0x6f, 0x27f, 0x2ff, 3, 8, 4, 5, ...)
    u_char      *m_evtPack;         // ROR 0x22a80: getEvtPack, 0xa0000 bytes, W000.WRD file 4
    int          m_22a84;           // ROR 0x22a84
    int          m_22ab0;           // ROR 0x22ab0
};

extern GamMan g_gamMan;         /* bss 0x7812c0 */

/* Outside GamMan (files not yet defined). */
void gamClearFrameReq(void);                                             // ROR 0x001f688c
void gamSystemDataLoaded(void);                                          // ROR 0x001f8fb8
extern int    g_bss_780d04;     /* bss 0x780d04: option 0's brightness word (32..176) */
/* bss 0x7b2060: a pointer to the debug / start flag words GamMan,
 * MagScene and the menus test (+0 0x8000, +4 bits 25 / 26, +8 0x8000). No
 * code stores it (scanned 2026-10-25): see open-items. */
extern u_int *g_bss_7b2060;

#endif /* _GAME_GAMMAN_H */
