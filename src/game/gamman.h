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
#include "game/gamstat.h"
#include "i3d/i3denv.h"
#include "sequence/sequence.h"

/* An area trigger record (EE 0xb0, plain EE layout: the scripts pass it
 * from their image, native 023; GamArea copies it whole into a node).
 * Field roles beyond the ones named are unverified. */
struct GamAreaTrg
{
    float m_rect[4];            // ROR 0x00: x0, z0, x1, z1; native 023 orders each pair when m_flags & 0x103
    u_int _10[4];
    u_int m_flags;              // ROR 0x20: native 023 sets 0x3000000; 0x1000000 registers it (0x1de104), 0x400000 set by addTrg when m_70
    u_int _24[2];
    u_int m_id;                 // ROR 0x2c: addTrg skips a listed id; newId replaces it with a fresh oid
    u_int _30[16];
    u_int m_70;                 // ROR 0x70: non-zero: addTrg sets m_flags 0x400000
    u_int _74[15];
};
static_assert(sizeof(GamAreaTrg) == 0xb0, "GamAreaTrg is 0xb0 bytes on the EE");
static_assert(offsetof(GamAreaTrg, m_flags) == 0x20, "m_flags");
static_assert(offsetof(GamAreaTrg, m_id) == 0x2c, "m_id");
static_assert(offsetof(GamAreaTrg, m_70) == 0x70, "m_70");

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
    /* afterSeqExec's stage change kind 3: the rects / triggers of kind. */
    void stopKind(int kind);                                             // ROR 0x001dd7bc
    void clearKind(int kind);                                            // ROR 0x001dddbc
    /* 1 when one of its rects (the list +0xd0) has id. */
    int  hasId(u_int id);                                                // ROR 0x001e0d70
    /* Copies trg into a node of its list +0xd0 unless a listed one has
     * its id; newId != 0 first gives trg a fresh oid (GamMan::makeOid(0,
     * 0xd)). The ROM's callers pass a fourth word it never reads. */
    void addTrg(GamAreaTrg *trg, int newId);                             // ROR 0x001dd5e8
    /* stopTrgs, then the listed triggers with flag 0x1000000 again. */
    void startTrgs();                                                    // ROR 0x001de4e4
    /* Empties the active list +0xc290 into its free list. */
    void stopTrgs();                                                     // ROR 0x001ddc80
    /* Empties its trigger lists (+0xc1c0 / +0x18b40's / +0xd0) into their
     * free lists. */
    void clear();                                                        // ROR 0x001ddaa8
    /* Adds value to key's up-to-8 values in the pool *(+0x18b40). */
    void registFtb(int key, u_int value);                                // ROR 0x001de6fc
};

struct DogActor;

/* An object id (OID_STATE in the ELF's symbols): bits 28..29 the list
 * (0 vfchr, 1 achr, 2 fchr), bits 23..27 the object type, bits 0..8 the
 * slot. */
static inline u_int gamOidList(u_int oid) { return oid >> 0x1c & 3; }
static inline u_int gamOidType(u_int oid) { return oid >> 0x17 & 0x1f; }
static inline u_int gamOidSlot(u_int oid) { return oid & 0x1ff; }

/* GamMan::getObjParam(0x18)'s answer: the object's shape and private
 * collisions (kind 5 one in m_colli[0], kind 6 four). EE 0x28; the ROM's
 * callers clear only m_kind. */
struct GamObjColli
{
    int   m_kind;               // ROR 0x00: 5 a segment, 6 four walls; -1 before the call
    int   m_shape;              // ROR 0x04: getObjRadius: 0 none, 1 17, 2 25, 3 45, 4 m_radius
    float m_08;                 // ROR 0x08: the colli adders: -height
    float m_0c;                 // ROR 0x0c: the colli adders: the base y
    float m_radius;             // ROR 0x10: shape 4's radius
    int   _14;
    int   m_colli[4];           // ROR 0x18: MagScene private collision ids
};
static_assert(offsetof(GamObjColli, m_shape) == 0x04, "m_shape");
static_assert(offsetof(GamObjColli, m_08) == 0x08, "m_08");
static_assert(offsetof(GamObjColli, m_0c) == 0x0c, "m_0c");
static_assert(offsetof(GamObjColli, m_radius) == 0x10, "m_radius");
static_assert(offsetof(GamObjColli, m_colli) == 0x18, "m_colli");
static_assert(sizeof(GamObjColli) == 0x28, "GamObjColli is 0x28 bytes on the EE");

/* One item (EE 0x10, 16-aligned): the 12 slots at GamMan +0x1a700
 * (getItemHavePtr's vector) and the stock pool's records. addItem /
 * addStock fill all of it but +0xc (stack garbage in the ROM, zero on the
 * host); clearItemHave sets only the id. layout: host -- m_rec is a
 * pointer. */
struct alignas(16) GamItemSlot
{
    const void *m_rec;          // ROR 0x0: GamSysData::getRec(id), the item's system data record
    u_int       m_oid;          // ROR 0x4: GamMan::makeOid(2, 5)
    short       m_id;           // ROR 0x8: 0xa00 = none
    u_short     m_num;          // ROR 0xa: the count of a stacking item (1..99)
    int         _c;             // ROR 0xc
};
static_assert(offsetof(GamItemSlot, m_oid) >= 0x4, "m_oid");
static_assert(offsetof(GamItemSlot, m_id) >= 0x8, "m_id");

/* The vector at +0x1a6c0: begin, end, capacity, the 12 slots at +0x40.
 * layout: host -- pointers. */
struct GamItemHave
{
    GamItemSlot *m_begin;       // ROR 0x0
    GamItemSlot *m_end;         // ROR 0x4
    int          m_capacity;    // ROR 0x8: 0xc
    GamItemSlot  m_slot[12];    // ROR 0x40
};

/* One record of the object table (EE 0x50; file data). ctlObjParam's ops
 * name the words they touch. */
struct GamObjRec
{
    u_int    m_oid;             // ROR 0x00
    u_int    m_flags;           // ROR 0x04: ops 0 set / 1 clear / 2 test / 0x11 store; 0x8000 cleared on a stage reset
    int      m_08;              // ROR 0x08: ops 9 / 10 / 15
    int      m_0c;              // ROR 0x0c: ops 4 / 5
    float    m_10[4];           // ROR 0x10: ops 8 / 14
    float    m_20;              // ROR 0x20: op 13
    int      _24;
    int      m_28;              // ROR 0x28: ops 6 / 12; op 16 compares it with GamMan +0x21f7c
    int      m_2c;              // ROR 0x2c: ops 7 / 12
    float    m_30[4];           // ROR 0x30: op 11 (a4 == 0), op 16
    float    m_40[4];           // ROR 0x40: op 11 (a4 != 0), op 16
};
static_assert(sizeof(GamObjRec) == 0x50, "GamObjRec");
static_assert(offsetof(GamObjRec, m_flags) == 0x04, "m_flags");
static_assert(offsetof(GamObjRec, m_08) == 0x08, "m_08");
static_assert(offsetof(GamObjRec, m_0c) == 0x0c, "m_0c");
static_assert(offsetof(GamObjRec, m_10) == 0x10, "m_10");
static_assert(offsetof(GamObjRec, m_20) == 0x20, "m_20");
static_assert(offsetof(GamObjRec, m_28) == 0x28, "m_28");
static_assert(offsetof(GamObjRec, m_2c) == 0x2c, "m_2c");
static_assert(offsetof(GamObjRec, m_30) == 0x30, "m_30");
static_assert(offsetof(GamObjRec, m_40) == 0x40, "m_40");

/* One record of the list at +0x18bc0 (0x600 bytes, ended by oid 0; EE
 * 6): onStageSeqEnd puts each word back into the oid's table record
 * (ctlObjParam ops 0x11 / 4). */
struct GamObjReset
{
    short    m_oid;             // ROR 0x0
    u_short  m_flags;           // ROR 0x2: to GamObjRec::m_flags
    u_short  m_0c;              // ROR 0x4: to GamObjRec::m_0c
};
static_assert(sizeof(GamObjReset) == 6, "GamObjReset");
static_assert(offsetof(GamObjReset, m_0c) == 4, "m_0c");

/* One record of the list at +0x18bc4 (0xc00 bytes, 0x180 records, ended
 * by oid 0; EE 8): onSubSeqEnd sets / resets the object's bits 4, 1,
 * 0x40, 0x80 and 0x10 by these flags. */
struct GamObjSet
{
    u_int m_oid;                // ROR 0x0
    u_int m_flags;              // ROR 0x4
};
static_assert(sizeof(GamObjSet) == 8, "GamObjSet");

/* ctlObjParam op 16's answer (EE 0x20): the record's field character. */
struct GamObjInfo
{
    u_int    m_oid;             // ROR 0x00: the FCHRstat's oid
    float    m_rotY;            // ROR 0x04: its rotation's y
    float    m_pos[3];          // ROR 0x08: the record's m_30 or m_40 (x, y, z)
    int      m_alt;             // ROR 0x14: 1 when m_28 != GamMan +0x21f7c (m_40 picked)
    u_int    m_flags;           // ROR 0x18: the record's m_flags & 0xf8
    int      _1c;               // ROR 0x1c: stack garbage in the ROM
};
static_assert(sizeof(GamObjInfo) == 0x20, "GamObjInfo");
static_assert(offsetof(GamObjInfo, m_pos) == 0x08, "m_pos");
static_assert(offsetof(GamObjInfo, m_alt) == 0x14, "m_alt");
static_assert(offsetof(GamObjInfo, m_flags) == 0x18, "m_flags");

/* A script image GamMan keeps (system call 0x14; file data in the stage
 * image): width and height bytes, the pixels before it (16-aligned), a
 * word at +0xc that setImage points at them. */
struct GamImage
{
    u_char   m_w;               // ROR 0x0
    u_char   m_h;               // ROR 0x1
    u_char   _2[10];
    u_int    m_pixels;          // ROR 0xc: the pixels' address (PC: through the VM's word store)
};
static_assert(sizeof(GamImage) == 0x10, "GamImage");
static_assert(offsetof(GamImage, m_pixels) == 0xc, "m_pixels");

/* The stage's object table (W000.WRD file 1, +0x18bcc; EE 0x5020): a
 * 0x20-byte header with the count at +8, then 0x50-byte records. */
struct GamObjTable
{
    int      _0[2];
    u_short  m_num;             // ROR 0x08
    u_short  _a;
    int      _c[4];
    int      m_version;         // ROR 0x1c: 1 = the current record size (0x50), else 0x30
    GamObjRec m_rec[0x100];     // ROR 0x20
};
static_assert(offsetof(GamObjTable, m_num) == 0x08, "m_num");
static_assert(offsetof(GamObjTable, m_version) == 0x1c, "m_version");
static_assert(offsetof(GamObjTable, m_rec) == 0x20, "m_rec");
static_assert(sizeof(GamObjTable) == 0x5020, "GamObjTable is 0x5020 bytes (init's allocDebug)");

/* Opaque pool records (EE data sizes; their classes come with the code
 * that fills them). */
struct GamRec4  { u_int _0; };
struct GamRec8  { u_int _0[2]; };
struct GamRec16 { u_int _0[4]; };
static_assert(sizeof(GamRec4) == 4, "GamRec4");
static_assert(sizeof(GamRec8) == 8, "GamRec8");
static_assert(sizeof(GamRec16) == 0x10, "GamRec16");

/* A sky dome record of the pool +0x1d240 (EE 0x10): releaseTenkyus
 * hands m_no to MagScene::releaseTenkyu. */
struct GamTenkyuRec
{
    u_int m_flags;              // ROR 0x0: the load mode << 30 (2 / 3: which of a pair shows, showMapUnits); bit 0 cleared by resetTenkyuFlags
    int m_unit;                 // ROR 0x4: the unit's index in data/bg/mapunit.bin (loadMapUnit's id)
    int m_no;                   // ROR 0x8: the scene's tenkyu number
    int _c;                     // ROR 0xc: stack garbage in the ROM
};
static_assert(sizeof(GamTenkyuRec) == 0x10, "GamTenkyuRec");
static_assert(offsetof(GamTenkyuRec, m_no) == 0x8, "m_no");
static_assert(offsetof(GamTenkyuRec, m_flags) == 0x0, "m_flags");
static_assert(offsetof(GamTenkyuRec, m_unit) == 0x4, "m_unit");

/* A record of the pool +0x20b00 (EE 8): an object's entry in the scene's
 * +0x837e0 table (ctlObj1cbb80 releases it). */
struct GamObjFxRec
{
    u_int m_oid;                // ROR 0x0
    int   m_no;                 // ROR 0x4: the Obj176a3c entry, -1 none
};
static_assert(sizeof(GamObjFxRec) == 8, "GamObjFxRec");
static_assert(offsetof(GamObjFxRec, m_no) == 4, "m_no");

/* A record of the pool +0x1f880 (EE 0xc): a stage-2 actor initStage2
 * loaded from its A%03d.ARA pack. */
struct GamActorRec
{
    u_short m_flags;            // ROR 0x0: 1
    short   m_model;            // ROR 0x2: the pack's model id
    int     m_actor;            // ROR 0x4: MagScene::loadActor's number
    int     m_8;                // ROR 0x8: stack garbage in the ROM, 0 here
};
static_assert(sizeof(GamActorRec) == 0xc, "GamActorRec");
static_assert(offsetof(GamActorRec, m_model) == 2, "m_model");
static_assert(offsetof(GamActorRec, m_actor) == 4, "m_actor");

/* A pushCamera record (GamMan +0x22470, EE 0x30; init sets four modes
 * -1, pushCamera fills three): the camera mode and its words, and the
 * scene camera entity's translation, rotation and lens angle. */
struct GamCamRec
{
    int   m_mode;               // ROR 0x00: MagSystem::getCameraMode's word 0 & 0xf (0..5), -1 after init
    int   m_arg;                // ROR 0x04: its word 1 (the camera number)
    u_int m_21f78;              // ROR 0x08: GamMan +0x21f78 (setCameraMode's arg for modes 3..5)
    float m_trans[3];           // ROR 0x0c: DogCamera m_trans x, y, z
    float m_rotate[3];          // ROR 0x18: DogCamera m_rotate x, y, z
    float m_angle;              // ROR 0x24: DogCamera m_angle
    float m_pathPos;            // ROR 0x28: modes 3..5: MagScene::getCameraPathPos
    int   _2c;
};
static_assert(sizeof(GamCamRec) == 0x30, "GamCamRec stride");
static_assert(offsetof(GamCamRec, m_21f78) == 0x08, "m_21f78");
static_assert(offsetof(GamCamRec, m_trans) == 0x0c, "m_trans");
static_assert(offsetof(GamCamRec, m_rotate) == 0x18, "m_rotate");
static_assert(offsetof(GamCamRec, m_angle) == 0x24, "m_angle");
static_assert(offsetof(GamCamRec, m_pathPos) == 0x28, "m_pathPos");

/* GamMan +0x22540: three records, -1.0 = unset (EE 0x20). */
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
    /* The chapter's own 3xxx.se pack, and with withArea its 1xxx area pack
     * (the second of loadStageSnd's pair), read and registered again. */
    void loadChapterSe(int chapter, int withArea);                            // ROR 0x001c2848
    void loadStageSnd(int chapter);                                      // ROR 0x001c2f50
    /* The PC's actor plays anim (MagSystem::setActorAnim(.., 1)). */
    void setPcAnim(int anim);                                            // ROR 0x001c4f38
    /* m_pcJnfc = on, then MagSystem::setPC(m_pcActor). */
    void setPcJnfc(int on);                                              // ROR 0x001c53e0
    /* which 0x11..0x1c: *v into an option / camera word (0x1b m_21f54). */
    void setSysParam(int which, const void *v);                          // ROR 0x001c6f08
    /* The two ACHRstat pointers (+0x22090 / +0x22094): param 0x1c copied
     * into 0x1b; then 0x1d84b0 and four words cleared. MnuTitle calls it
     * when the title closes for a new game. */
    void resetPcStat();                                                  // ROR 0x001c1378
    /* The PC (and dog) placed for a stage start by m_1a684: 0xcccccccc
     * leaves them; 0xcccccccb the kept start transforms (m_pcStart /
     * m_dogStart); -n the room's n-th start of W000.WRD file 2; an oid
     * 50 units in front of that fchr (type 9), turned round when the
     * object's param 12 room is this one. */
    void setPcStartPos();                                                // ROR 0x001c3d38
    /* Every avfchr's actor gets motion 1 at 1/60 (after a map unit
     * load). */
    void resetAVFchrMotions();                                           // ROR 0x001c4610
    void setStagePack(void *buf);                                        // ROR 0x001c46f4
    /* Sequence::reapThreads' calls when thread 0x44000000 (the stage
     * image's main thread) / 0x84000000 (image 2's) ends. */
    void onStageSeqEnd();                                                // ROR 0x001c4168
    void onSubSeqEnd();                                                  // ROR 0x001c43f0
    /* Stage change kind 2's second half (stage 2 data). */
    void initStage2();                                                   // ROR 0x001c47c4
    /* The vfchrs of this room get their models again (loadVFchrModel at
     * their position), hidden ones hidden; the avfchr list emptied first. */
    void reloadVFchrModels();                                            // ROR 0x001c387c
    void empty1c4f30();                                                  // ROR 0x001c4f30
    /* The scene's S2dMes sprite no (the menus' button guides 4 / 5): show
     * (process on, standard colour, anim - 1) or hide; its position in
     * pixels; fade in / out over time frames. */
    void showMesSprite(int no, int show, int anim);                      // ROR 0x001c5094
    void setMesSpritePos(int no, int x, int y);                          // ROR 0x001c5168
    void fadeInMesSprite(int no, int time);                              // ROR 0x001c51e0
    void fadeOutMesSprite(int no, int time);                             // ROR 0x001c5240
    /* The game into / out of the save screen's 0x3000-byte image
     * (unassigned@001d3b44). */
    void writeSaveData(void *buf);                                       // ROR 0x001d6078
    void readSaveData(void *buf);                                        // ROR 0x001d68a0
    void update();                                                       // ROR 0x001c526c
    void postUpdate();                                                   // ROR 0x001c53d8
    /* The dog's mode (getInuMode): 1 follows the PC (shown, Brown's
     * controller on), 2 hidden and away (bitset word 1 bits 2 / 4 clear),
     * 3 hidden, 4 shown but held. */
    void setInuMode(int mode);                                           // ROR 0x001c541c
    /* Returns 0; the pause / resume pair calls it with 1 / 0. */
    int  empty1c58b8(int on);                                            // ROR 0x001c58b8
    /* Pauses (1) / resumes (0) the type-2 actor characters' tasks and
     * the tasks of kinds 0x13 / 0x14 and EmyManager's; flags2 bit 0x10000
     * holds the state. Returns 0 when nothing changes. */
    int  pauseAchr(int on);                                              // ROR 0x001c58c0
    /* Every type-2 achr: on calls its actor's virtual +0x44 with 1 and
     * sets +0x3f4 bit 0; off undoes it where the bit is set (resetting
     * the object of its param 0x67 first). Native 212 case 50. */
    void switchEnemies(int on);                                          // ROR 0x001c5a5c
    /* Stops the stage sound and the tasks before a menu opens; kind 1 / 2
     * also sets Sequence flag 1. */
    void enterMenu(int kind);                                            // ROR 0x001c5bf4
    /* pause(0x100000)'s part: flags2 bit 4, pause(2), enterMenu for the
     * menu ids (0x10000.. item / map, 0x1000a the ehon). */
    void pauseForMenu(int menu);                                         // ROR 0x001c5e1c
    /* resume(0x100000)'s part: hides S2dMes sprite 2 after the ehon,
     * flags2 bit 8. */
    void resumeFromMenu(int arg);                                        // ROR 0x001c5f70
    /* Halts / restarts parts of the game by mask (0x100000 before the
     * menus; resume(4) once the PC's motion is over). */
    void resume(u_int mask);                                             // ROR 0x001c65e8
    /* Shows the actor of the object linked to the achr oid (+0x254) and,
     * for type 0, EmyManager's two objects. */
    void showLinked(u_int oid);                                          // ROR 0x001c687c
    /* EmyManager's two object ids into +0x22004 / +0x22008. */
    void getEmyOids();                                                   // ROR 0x001c6a04
    void pause(u_int mask);                                              // ROR 0x001c6a40
    /* m_playMinutes plus the minutes in m_vcount (1800 frames each). */
    int  getPlayMinutes();                                               // ROR 0x001c6c98
    /* Copies one status word to *out: 1 m_flags, 2 m_vcount, 3 the play
     * minutes, 4 m_chapter (a short), 5 / 6 m_wrdNo / m_21f80, 7
     * m_21f68, 8 m_cameraMode, 9 m_pcActor, 10 m_dogChrNo, 11
     * m_murderChrNo, 12..14 the PC / dog / murderer ObjStat, 15 m_volume
     * (a short), 16 m_optBrightLevel, 17..20 m_optOutput ..
     * m_optMasterLevel, 21 &m_optOutput, 22 / 23 m_21fbc[0] / [1] (their
     * bits), 25 m_findTargetItem, 26 m_equipItem, 28 m_04. */
    void getStatus(int which, void *out);                                // ROR 0x001c6ccc
    /* Camera mode 1..5 through MagSystem::setCameraMode; 4 / 5 take map
     * camera no (below getMapCamNum). */
    void setCameraMode(int mode, u_int no);                              // ROR 0x001c7210
    /* The record count of the current sky slot's _c.bin (the cameras). */
    /* The current room (WRD) number (an inline in the ELF). */
    int  getWrdNo() const { return m_wrdNo; }                            // ROR 0x001c6c28
    int  getMapCamNum();                                                 // ROR 0x001c7494
    /* The script's SE pack swaps (native 252 cases 48 / 100): every SE
     * stopped, banks 0xb (the first only), 0x20 and 0x40 freed, name
     * loaded, the empty item's pack reloaded, m_flags2 0x1000000 /
     * 0x2000000 set. */
    void loadScriptSePack(const char *name);                             // ROR 0x001c7500
    void loadScriptSePack2(const char *name);                            // ROR 0x001c75bc
    /* The chapter's pack again: bank 0x1f with flag 1 / bank 0xc with 0
     * (initStage2, native 252 cases 101 / 102). */
    void reloadChapterSe1f();                                            // ROR 0x001c766c
    void reloadChapterSe0c();                                            // ROR 0x001c76cc
    /* A new object id: image (-1 = m_cur_stat) << 30, type << 23, list
     * << 28 for the types without their own counter, the stage word, and
     * a free number counted down from init's table +0x225f4... */
    u_int makeOid(u_int list, u_int type, int image);                    // ROR 0x001c6fe0
    /* Loads the SE pack of an equipped item (data/snd/xv/640N.se; cur:
     * the equipped item +0x2220c instead of id). */
    void loadItemSe(u_short id, int cur);                                // ROR 0x001c772c
    /* The PC holds item id (0xa00 none): the PC's param 0x32, the item's
     * actor in the PC's right hand (node p_hn_r), its SE pack. */
    void equipItem(short id);                                            // ROR 0x001c7a4c
    /* Puts item id in its slot (a stacking one counts up to 99; num sets
     * the count of a new one) and sets its flag bit. 1 when every slot is
     * taken. */
    int  addItem(short id, int num);                                     // ROR 0x001c7e40
    /* Takes item id out (a stacking one counts down first); one: only the
     * first slot holding it. */
    void delItem(short id, int one);                                     // ROR 0x001c82f8
    void delItemAll(short id);                                           // ROR 0x001c8414
    /* The 12 item slots, all 0xa00. */
    void clearItemHave();                                                // ROR 0x001c845c
    /* m_selItem / m_selItemNum: the slot holding id (the last of them). */
    void selectItem(short id);                                           // ROR 0x001c8598
    /* The count of item id's slot, 0 when none holds it. */
    int  getItemNum(short id);                                           // ROR 0x001c8508
    /* m_selItemNum (selectItem's count). */
    int  getSelItemNum();                                                // ROR 0x001c898c
    int  hasItem(short id, int withStock);                               // ROR 0x001c8a28
    int  getItemCount(short id, int withStock);                          // ROR 0x001c8ae8
    /* 1 when an item slot is free. */
    int  hasFreeItemSlot();                                              // ROR 0x001c8c50
    /* 1 when item id (0: oid's param 2) fits: a stacking one under 99 in
     * its slot, or a free slot. */
    int  canAddItem(u_int oid, short id);                                // ROR 0x001c8ca4
    /* Item id into the stock (0x14e records; a stacking one counts up).
     * 1 when the stock is full or a count hit 99. */
    int  addStock(short id, int count);                                  // ROR 0x001c8f7c
    /* Item id out of the stock (one: a stacking one counts down, only the
     * first record). 1 when a record was freed. */
    int  delStock(short id, int one);                                    // ROR 0x001c94d8
    /* 1 when the fchr of oid (types 4 / 5) can be talked to: shown, not
     * held (0x200000), and 0x10 or both 8 and 2 in its flags. */
    int  isFchrActive(u_int oid);                                        // ROR 0x001c96c8
    /* The item the PC's find cursor targets (0xa00 none) to the scene. */
    void setFindTargetItem(short id);                                    // ROR 0x001c9784
    /* Empty in the US build (before the map menu). */
    void beforeMapMenu();                                                // ROR 0x001c98b8
    /* The object table record of oid: op 0..0x13 sets / reads one of its
     * words through data (see GamObjRec); -1 when oid has no record or
     * for the read ops that return nothing, 0 or a test result else. */
    int  ctlObjParam(u_int oid, u_int op, void *data, int alt);          // ROR 0x001c9920
    /* The options: 0 brightness, 1 master volume, 2 stream volume, 3 SE
     * volume (levels 0..8), 4 / 5 read 2 / 3 into *(int *)arg, 6 the
     * sound output mode, 7 vibration. Returns -1. */
    int  setOption(u_int no, intptr_t arg);                              // ROR 0x001c9c1c
    void empty1ca0d4();                                                  // ROR 0x001ca0d4
    /* Loads map unit name from data/bg/mapunit.bin (mode 2 / 3) unless a
     * loaded unit already has id. */
    void loadMapUnit(const char *name, int id, int mode);                // ROR 0x001ca0dc
    /* Shows / hides the loaded map units by their load mode: mode 1 shows
     * all; 2 shows mode-2 units (m_mapModelNo = the last) and hides mode-3
     * ones; 3 the other way round. */
    void showMapUnits(int mode);                                         // ROR 0x001c9e20
    /* The RTPK pack data of unit into a scene sky slot (MagSystem::
     * loadScene) and a +0x1d240 record, unless the unit is loaded. */
    void loadMapPack(u_short unit, void *data, int mode);                // ROR 0x001ca38c
    /* A new object from model file name (oid 0: one is made from kind
     * >> 10), added with addChr(.., kind2, pos, place); returns the oid,
     * 0 when oid already exists. */
    u_int createObjByName(u_int kind, const char *name, int group, u_int oid,
                          int kind2, const void *pos, int place);        // ROR 0x001ca72c
    /* MagSystem::loadModel(model) bound to oid, addChr when add != 0;
     * returns the scene model number, -1 when oid exists. */
    int  createObjByModel(int model, u_int oid, int kind, const void *pos,
                          int place, int add);                           // ROR 0x001ca8e8
    /* The archive of model group no (kind >> 10): data/ch/<group>/....RPK,
     * NULL for 7 and past 21. */
    const char *getChrRpk(u_int no);                                     // ROR 0x001ca604
    /* The actor's light id by the object type: 1 for 0..3 / 5, 0 for 4 / 9. */
    void setObjLight(int actor, u_int oid);                              // ROR 0x001ca598
    /* Swaps a live oid's model for model (or file name), keepPos keeps its
     * position; returns oid, 0 for no object or no change. */
    u_int changeObjModel(u_int model, const char *name, u_int oid, int keepPos); // ROR 0x001cad88
    /* Releases the stage's sky domes (the +0x1d240 records) of kind. */
    void releaseTenkyus(int kind);                                       // ROR 0x001cb01c
    /* The PC becomes the achr oid (-1: none to the scene): m_pcPtr, its
     * actor, the camera. */
    void setPc(int oid);                                                 // ROR 0x001cb24c
    /* The dog becomes the achr oid (-1: hides the dog). */
    void setDog(int oid);                                                // ROR 0x001cb384
    /* The murderer becomes the achr oid (m_murderOid, m_murderPtr). */
    void setMurder(int oid);                                             // ROR 0x001cb424
    /* The PC's position / rotation (when there is a PC) and its copy in
     * m_pcPos / m_pcRot. */
    void setPcPos(const float *pos);                                     // ROR 0x001cb520
    void setPcRot(const float *rot);                                     // ROR 0x001cb57c
    /* The PC's ACHRstat::setParam(which, data), when there is a PC. */
    void setPcParam(int which, const void *data);                        // ROR 0x001cb4f0
    void ctlObj1cb8e4(u_int oid, int a, int b);                          // ROR 0x001cb8e4
    /* Kind 0x32: an Obj176a3c effect 0x32 at pos (radius 50) bound to oid. */
    void addObjMarker(u_int oid, int kind, const float *pos);            // ROR 0x001cb970
    /* A type-2 achr placed (placeAchr) and, in move state 2 without one, a
     * TskSeqEvent {60, radius} on its m_placeTask. */
    void startAchrPlaceTask(float radius, ACHRstat *chr, int keep);      // ROR 0x001d0efc
    /* A +0x20b00 record {oid, no}: the scene +0x837e0 entry no belongs to oid. */
    void addObjFx(u_int oid, int no);                                    // ROR 0x001cba0c
    /* The scene +0x837e0 entry of oid's first +0x20b00 record, -1 none. */
    int  findObjFx(u_int oid);                                           // ROR 0x001cbafc
    /* Frees oid's +0x20b00 records, releasing their scene +0x837e0 entry. */
    void ctlObj1cbb80(u_int oid);                                        // ROR 0x001cbb80
    /* Clears the scene's +0x837e0 effect table and frees every +0x20b00
     * record (kind is not read). */
    void clearPool20b00(int kind);                                       // ROR 0x001cbd0c
    /* The scene actor of oid, NULL without one. */
    DogActor *getActor(u_int oid);                                       // ROR 0x001cbdb8
    /* Keeps a script image {w, h, ...} in slot. */
    void setImage(int slot, u_char *image);                              // ROR 0x001cbdfc
    /* The scene actor number of oid, -1 without one. */
    int  getActorNo(u_int oid);                                          // ROR 0x001cbe38
    /* Drops object oid: its private collisions, then an achr (not the
     * dog; the PC's words cleared) or its fchr / vfchr records, and its
     * object sounds. */
    void releaseChr(u_int oid);                                          // ROR 0x001ccc30
    /* An achr / fchr / vfchr object's teardown pieces. */
    void releaseFchr(u_int oid);                                         // ROR 0x001cc7c8
    void releaseVFchr(u_int oid);                                        // ROR 0x001cc970
    void clearVFchrSlot(u_int oid);                                      // ROR 0x001cc004
    /* Adds a vfchr object (an FCHRstat in the vfchr list) unless oid is one. */
    void addVFchr(int model, u_int oid, int arg, int kind, intptr_t pos); // ROR 0x001cc044
    /* The first free of the 0x180 vfchr slots takes oid; its FCHRstat
     * storage in m_1f248, NULL when all are taken. */
    FCHRstat *allocVFchr(u_int oid);                                     // ROR 0x001cbfb0
    /* Loads model (MagSystem::loadModel) for vfchr oid: its params 3 (the
     * actor) and 0x1000, position pos, motion 1, the avfchr list; model
     * 0x800 gets a 1e-7 scale. */
    void loadVFchrModel(int model, u_int oid, intptr_t pos);             // ROR 0x001caa14
    /* loadVFchrModel by name (MagScene::loadModelByName, the pack of model
     * >> 10); params 0x1000, 0x1004 (name) and 3. */
    void loadVFchrModelByName(int model, const char *name, u_int oid, intptr_t pos); // ROR 0x001cabd4
    /* Releases every vfchr's actor; keep 0 also clears their slots and
     * frees them from the list. */
    void releaseVFchrs(int keep);                                        // ROR 0x001cea44
    /* 1 when id is one of the 0x169 words of m_1f240. */
    int  isInOidList(int id);                                            // ROR 0x001c7da8
    /* 1 when an object record (m_objTable) has id in m_28 or m_2c. */
    int  isObjRecRef(int id);                                            // ROR 0x001c98c0
    /* The object's m_flags & bits != 0; 0 for no object. */
    int  testObjFlags(u_int oid, u_int bits);                            // ROR 0x001d1720
    /* Achr objects only: word bits >> 24 of +0x3ec & the low 24 bits. */
    int  testAchrFlag(u_int oid, u_int bits);                            // ROR 0x001d17c8
    /* setObjWork's getter (ObjStat::getWork); -1 for no object. */
    int  getObjWork(u_int oid, int no);                                  // ROR 0x001d217c
    /* Parents oid's actor to node of parent's model (MagSystem::
     * setActorParent), its position cleared; an achr parent links oid
     * (+0x254). detachObj undoes it. */
    void attachObj(u_int oid, u_int parent, const char *node);           // ROR 0x001d1a5c
    void detachObj(u_int oid, u_int parent);                             // ROR 0x001d1bec
    /* 1 when a vfchr's getParam(which) word 0 is value. */
    int  hasVFchrParam(int which, int value);                            // ROR 0x001d2d7c
    // ROR 0x0028f300
    void cnvPosW2L(float *a, float *b) { gamWrdConvPos(0, a, b); }
    // ROR 0x0028f334
    void cnvPosL2W(float *a, float *b) { gamWrdConvPos(1, a, b); }
    // ROR 0x0028f368
    void getWrdOffset(float *out) { gamWrdConvPos(2, out, NULL); }
    /* Adds the object oid of scene model (-1 none) by its list: an achr
     * (ACHRstat) or an fchr (FCHRstat); kind is its system data id. */
    void addChr(int model, u_int oid, int kind, const void *pos, int place); // ROR 0x001cc250
    /* EmyManager group a set up (b, c, count, d, pos, f), then count
     * type-2 achrs of kind's model (field 0xf) at pos, oid 0: EmyManager
     * gives each its oid. model, oid and e are never read. */
    void addChrGroup(int model, u_int oid, int a, int b, int c, int count, int d,
                     int e, int kind, const void *pos, int place, float f); // ROR 0x001cc560
    /* Stage change kind 2's first half. */
    void releaseStage2();                                                // ROR 0x001cd224
    /* Drops the stage's objects by kind (2: those not of image 1 and the
     * vfchr actors of room m_21f84; 3: image 3; else all): tasks, enemy
     * work, script callbacks, update list, area triggers, object sounds,
     * camera stack, achrs, and unless keepFchrs the fchrs and vfchrs. */
    void releaseObjs(int kind, int keepFchrs);                           // ROR 0x001cdf5c
    /* Every achr (the PC and dog kept with script bit [2] 0x8000) and
     * fchr deleted, the avfchr actors released, and with [7] >= 0 the
     * vfchrs (releaseVFchrs(0)). */
    void releaseAllChrs();                                               // ROR 0x001ced18
    void releaseAll() { releaseAllChrs(); }                              // ROR 0x001c4fa4
    /* sysTermManagers: releaseAll, then init again. */
    void term();                                                         // ROR 0x001c0d44
    /* Every achr's and fchr's secta pointers cleared. */
    void clearChrSectaPtrs();                                            // ROR 0x001cebf8
    /* The words of m_1f240 with an image (bits 30..31) cleared. */
    void clearOidListImages();                                           // ROR 0x001c7dec
    /* Sequence::exec's last call: the stage-change requests in flags2. */
    /* Records a stage change for afterSeqExec: kind 1 chapter (a full
     * reset, m_stageReq 5), kind 2 room into +0x21f7c, kind 3 a3 into
     * +0x21f80 (m_stageReq 3); word goes to +0x1a684. Kind 2 to the room
     * already loaded only resumes the menu pause. */
    void requestStage(int kind, int room, int a3, int chapter, u_int word); // ROR 0x001cd044
    void afterSeqExec();                                                 // ROR 0x001cd25c
    /* Reads the PC's position / rotation into m_pcPos / m_pcRot, puts the
     * dog 35 units to the PC's side with its heading, and tells Brown's
     * controller. */
    void storePcTransform();                                             // ROR 0x001cd404
    /* Stage 2 with the PC and dog kept (flags2 0x10000000): their models
     * m_21ff4 / m_21ff8 (0x2005..0x2009 / 0x2401..0x2404 kept, else
     * 0x2000 / 0x2400), setPc /
     * setDog, the dog shown by script bit [1] 4, the PC's chapter anim;
     * sequence globals 0x51 / 0x52 get the models. */
    void setupPcDogStage2();                                             // ROR 0x001cd954
    /* Achr oid's actor released and model loaded again (params 3 and
     * 0x1000); returns the actor. */
    int  reloadAchrModel(u_short model, u_int oid);                      // ROR 0x001caf78
    /* With m_flags 0x20: the PC at m_sysWord moved by the room offset
     * change from globals [5] / [7] plus 50 units along global [10]'s
     * heading, the dog beside it; then globals [5] / [7] = this room's
     * offset x / z. */
    void startStage2();                                                  // ROR 0x001cd60c
    void empty1cd7c0();                                                  // ROR 0x001cd7c0
    /* The requested stage change by m_cur_stat (1 loadStage, 2 the stage-2
     * data, 3 nothing yet), then the sequence globals [1..4] and resume(2). */
    void execStageReq();                                                 // ROR 0x001cd7c8
    /* Stage change kind 5: the menus, the PC, the objects, the sounds
     * and the sequence images of the stage dropped. */
    void resetStage();                                                   // ROR 0x001cdbf0
    void setSysMsgHandle(int handle);                                    // ROR 0x001cfb38
    /* 1 when oid names a live object of its list. */
    int  isObj(u_int oid);                                               // ROR 0x001d046c
    /* *out = the Y heading of oid's actor (an achr or fchr); 0 when oid
     * is no object. */
    int  getObjRotY(u_int oid, float *out);                              // ROR 0x001d0758
    /* oid's rotation from an angle (normalised first). */
    void setObjDir(u_int oid, float dir);                                // ROR 0x001d024c
    /* By the oid's list, the object's position / rotation (each angle
     * wrapped into -pi..pi in place) setters; setObjPos returns 1, 0 for
     * no object. */
    int  setObjPos(u_int oid, const float *pos);                         // ROR 0x001cfbe4
    void setObjRot(u_int oid, float *rot);                               // ROR 0x001cfe78
    /* An achr / fchr's scene actor gets one scale for x, y and z. */
    void setObjScale(u_int oid, float scale);                            // ROR 0x001d1e88
    /* getObjParam's counterpart; returns 1, 0 for no object. */
    int  setObjParam(u_int oid, int which, const void *data);            // ROR 0x001d1d5c
    /* Achr only: adds data to param which. */
    void addObjParam(u_int oid, int which, const void *data);            // ROR 0x001d1f74
    /* resetObj's counterpart: sets bits of the object's flags. */
    int  setObj(u_int oid, u_int bits);                                  // ROR 0x001d10d0
    /* Achr: word bits >> 24 of +0x3ec gets / loses bits & 0xffffff; fchr:
     * +0x120 gets / loses bits. */
    void setObjAttr(u_int oid, u_int bits);                              // ROR 0x001d1838
    void clearObjAttr(u_int oid, u_int bits);                            // ROR 0x001d18c4
    /* Achr / fchr: placed at the point where (0x1d811c) resolves to. */
    void placeObjAt(u_int oid, int where);                               // ROR 0x001cfda4
    /* GamSysData::setFlags(id, bits), then every live achr / fchr of
     * system id gets bits set (on) or cleared. */
    void setKindFlags(int id, u_int bits, int on);                       // ROR 0x001d1fd8
    /* Word no of the object's work array (+0x1b0) = / += value. */
    void setObjWork(u_int oid, int no, int value);                       // ROR 0x001d2270
    void addObjWork(u_int oid, int no, int value);                       // ROR 0x001d2334
    /* Bits 5..6 of the scene's m_actor_flags for oid's actor = mode & 3. */
    void setActorBindMode(u_int oid, int mode);                          // ROR 0x001d2428
    /* Param 0x18 becomes shape 4 sized to the corner quad (0x1003). */
    void fitObjBounds(u_int oid);                                        // ROR 0x001d480c
    /* {cell, oid} to the tail of the +0x2246c update list (an oid listed
     * already moves there). */
    void queueObjUpdate(GamSecta *cell, u_int oid);                      // ROR 0x001d5bc8
    /* The radius param 0x18's shape word picks (0, 17, 25, 45, or its
     * own); word 0 to *out when out. */
    float getObjRadius(u_int oid, int *out);                             // ROR 0x001d4a3c
    /* Private collisions for the object: one segment {p0, p1}; four
     * walls round a rotated box {x, y, z, sx, sy, sz, rot}; four walls
     * round a quad of four x / z corners with heights [8] / [9]. */
    void addObjColliSegment(u_int oid, const float *seg);                // ROR 0x001d080c
    void addObjColliBox(u_int oid, const float *box);                    // ROR 0x001d091c
    void addObjColliQuad(u_int oid, const float *quad);                  // ROR 0x001d0c64
    /* Clears bits of the object's flags; bits 0x40 / 0x10000000 / 1 / 0x10
     * also re-link, re-place, show and ... it. Returns 0 for no object. */
    int  resetObj(u_int oid, u_int bits);                                // ROR 0x001d1410
    /* MagScene's open menu id != -1. */
    int  isMenuOpen();                                                   // ROR 0x001d1a48
    /* Clears bit 0 of every sky dome record's flags; returns 0. */
    int  resetTenkyuFlags();                                             // ROR 0x001cb1c0
    /* 1 while the message task runs or the system message window is open. */
    int  isMsgBusy();                                                    // ROR 0x001cf9d4
    /* Copies eight words over m_msgParam (native 007). */
    void setMsgParam(const int *param);                                  // ROR 0x001cf7d0
    /* Kills the message task, clears the message mode (native 009). */
    void killMsgTask();                                                  // ROR 0x001cf95c
    /* Restarts the message task (m_msgTaskId) on text with arg, m_msgParam,
     * m_msgMode and time (native 006). */
    void openMsg(const char *text, int arg, int time);                   // ROR 0x001cf854
    /* MagScene::openSysMsgWindow(msg, 0, 0) into m_sysMsgHandle; the
     * four ints are never read (native 006 passes -1). */
    void openSysMsg(int msg, int a, int b, int c, int d);                // ROR 0x001cfa5c
    /* S2dMes sprite 3: on with param {anim, x, y, fade in, fade out,
     * off x, off y} (points * 16), or fades it out by param[4]. */
    void showMsgSprite(int on, const int *param);                        // ROR 0x001cf6bc
    /* The heading from oid's position to target's (0 unless both exist). */
    float getObjDirTo(u_int oid, u_int target);                          // ROR 0x001cfb48
    /* The object's position / rotation (x, y, z, w) into out; 1 when it
     * exists. getObjRot leaves out alone for a vfchr. */
    int  getObjPos(u_int oid, float *out);                               // ROR 0x001d05f4
    int  getObjRot(u_int oid, float *out);                               // ROR 0x001d06b8
    /* An achr's position += d (x, y, z); other objects are left alone. */
    void moveAchr(u_int oid, const float *d);                            // ROR 0x001d2a14
    /* Camera mode, path position and the scene camera's trans / rotate /
     * angle onto a three-deep stack (+0x22470, count +0x22530), and back. */
    void pushCamera();                                                   // ROR 0x001c5ffc
    void popCamera();                                                    // ROR 0x001c621c
    /* Frees motion id of the PC's actor (native 028). */
    void releasePcMotion(int id);                                        // ROR 0x001c4f70
    /* Saves the scene's fog in the first free of three slots (native 012). */
    void pushFog();                                                      // ROR 0x001c6470
    /* Puts the last saved fog back and frees its slot (native 012). */
    void popFog();                                                       // ROR 0x001c651c
    /* Light sets 0 / 1 into 7 / 6 by mode (0 both, 1 set 1, 2 set 0). */
    void saveLightSets(int mode);                                        // ROR 0x001d2494
    void restoreLightSets(int mode);                                     // ROR 0x001d25e4
    int  isAchr(u_int oid);                                              // ROR 0x001d2aac
    int  isFchr(u_int oid);                                              // ROR 0x001d2adc
    int  isVFchr(u_int oid);                                             // ROR 0x001d2b10
    ACHRstat *getAchr(u_int oid);                                        // ROR 0x001d2b8c
    /* Type 5 looks in the vfchr list first. */
    FCHRstat *getFchr(u_int oid);                                        // ROR 0x001d2ba8
    /* The type-9 fchr whose param 0x41 is no, NULL none. */
    FCHRstat *findFchrType9(int no);                                     // ROR 0x001d2c0c
    FCHRstat *getVFchr(u_int oid);                                       // ROR 0x001d2d00
    /* The ELF's inline: the object of oid in its list. */
    // ROR 0x0028f7e4
    ObjStat *find(u_int oid)
    {
        switch (gamOidList(oid))
        {
        case 1:  return getAchr(oid);
        case 0:  return getVFchr(oid);
        case 2:  return getFchr(oid);
        default: return NULL;
        }
    }
    /* An object's param by which (0x41 the type-9 number ...). */
    void getObjParam(u_int oid, int which, void *out);                   // ROR 0x001d1c80
    /* The PC actor's motion speed (DogActor +0x1e8) set to 1e-7: its
     * motion stands still (GamArea::update). */
    void freezePcMotion();                                               // ROR 0x001c46c8
    /* 1 while the scene's fade is in states 1, 3 or 4. */
    int  isFading();                                                     // ROR 0x001c574c
    /* The room's WRD entry word 0x1c (the map area the map items 0xad0 ..
     * 0xad2 open: 0 / 2 / 1); -1 without a room, with flags2 0x2000 or
     * without an entry. */
    int  getMapArea();                                                   // ROR 0x001c6c38
    /* oid into the 0x169-word list m_1f240 (unless listed), then
     * releaseChr(oid). */
    void listAndReleaseChr(u_int oid);                                   // ROR 0x001c7d04
    /* Item slot slot = item; a stacking item (record flag 0x20000) held in
     * another slot is counted into that one first (99 at most) and slot
     * gets id 0xa00 with item's other words. */
    void setItemSlot(int slot, GamItemSlot item);                        // ROR 0x001c812c
    /* Slot slot emptied (the empty item's record, id 0xa00), unequipping /
     * untargeting what it held. */
    void clearItemSlot(int slot);                                        // ROR 0x001c8214
    /* The slot holding the equipped item, -1 none. */
    int  getEquipSlot();                                                 // ROR 0x001c854c
    /* The equipped weapon (0xa3e / 0xa43) not full (m_selItemNum != 6):
     * 1 + the slot of its ammo (0xa42 / 0xa3f) with the lowest count, the
     * number of such slots into *count; 0 none. */
    int  findReloadSlot(int *count);                                     // ROR 0x001c8604
    /* Moves ammo into the equipped weapon's count (6 at most): slot -1
     * takes findReloadSlot's and then others, else slot (0xa3f only).
     * Emptied slots cleared, m_selItem's count updated. 1 when reloaded. */
    int  reloadEquip(int slot);                                          // ROR 0x001c870c
    /* m_selItemNum (and m_selItem's count) down by one when above 0. */
    void decSelItemNum();                                                // ROR 0x001c899c
    /* The item slots holding something (id != 0xa00). */
    int  countItems();                                                   // ROR 0x001c89e4
    /* A type-5 fchr's flags & 0x100000 (0 for other types). */
    int  testFchr100000(u_int oid);                                      // ROR 0x001c8dc8
    /* Its kind's system flag 4, object bit 2 set and bit 1 cleared (shown),
     * the 0x32 marker when it can be talked to. */
    void markObjTaken(u_int oid);                                        // ROR 0x001c8e1c
    /* The object's system data id (ObjStat::m_kind), -1 for none. */
    short getObjKind(u_int oid);                                         // ROR 0x001cbedc
    /* Item id's model (field 0xf) loaded as a new type-5 fchr at pos. */
    void createItemObj(short id, const float *pos);                      // ROR 0x001c8eb4
    /* A copy of item into the stock; a stacking one (record flag 0x20000)
     * counts into its record. 1 when the stock is full. */
    int  addStockItem(GamItemSlot item);                                 // ROR 0x001c9198
    /* The stock record with oid back to the free list; 1 when found. */
    int  delStockByOid(u_int oid);                                       // ROR 0x001c9334
    /* m_flags 4 set and m_flags2 0x800 clear (MagScene::canOpenItemMenu). */
    int  canOpenItemMenu();                                              // ROR 0x001c97c8
    /* Map item 0xad0 / 0xad1 / 0xad2 held for map area 0 / 2 / 1, and
     * canOpenItemMenu (MagScene::canOpenMenu10001). */
    int  canOpenMapMenu();                                               // ROR 0x001c97f0
    /* ACHRstat::getParam / setParam of the PC / dog / murderer when set. */
    void getPcParam(int which, void *out);                               // ROR 0x001cb460
    void getDogParam(int which, void *out);                              // ROR 0x001cb490
    void getMurderParam(int which, void *out);                           // ROR 0x001cb4c0
    void setDogParam(int which, const void *data);                       // ROR 0x001cb5d8
    void setMurderParam(int which, const void *data);                    // ROR 0x001cb608
    /* The system message window closed (m_sysMsgHandle -1), the message
     * mode 0. */
    void closeSysMsg();                                                  // ROR 0x001cfaa8
    /* setObj(oid, 1) when hide, else resetObj(oid, 1). */
    void hideObj(u_int oid, int hide);                                   // ROR 0x001d23f8
    /* In unassigned@001d3b44 (GamMan methods past switchVolume). */
    /* Cell no of secta list list (bss 0x7811ac, 0x18 per list, 0x90 per
     * cell) and its position (+0x10) into pos. */
    GamSecta *getSectaCell(int no, float *pos, int list);                // ROR 0x001d811c
    void relinkObj(u_int oid);                                           // ROR 0x001d5da0
    /* The secta cell of pos (0x17c340 over the list bss 0x7811ac, starting
     * at cur), NULL when none or its flags bit 2 is set; type2: the oid's
     * type is 2 (ObjStat's position setters). */
    GamSecta *findSectaCell(const float *pos, GamSecta *cur, int type2); // ROR 0x001d8064
    void placeAchr(int oid, void *out, int arg);                         // ROR 0x001d544c
    /* The table data 0x2fc1f8 (8-word rows, -1 ends): column col of the
     * row whose column keyCol is key (keyCol 99: row key). -1 none. */
    int  lookupFlagTable(int keyCol, int key, int col);                  // ROR 0x001d83d0
    void markReset();                                                    // ROR 0x001d3588
    /* Marks light set theD in m_flags, copies set theS over it. */
    void setLightFlag(int theD, int theS);                               // ROR 0x001d2724
    /* 0: saves the volume in m_saved_volume and silences; 1: restores
     * the saved volume (when one is saved) and marks none saved. */
    void switchVolume(int on);                                           // ROR 0x001d3ac8
    void setVolume(int volume);                                          // ROR 0x001d3a38
    /* Returns 1; nothing else in the US build. */
    int  resetExtra();                                                   // ROR 0x001d84a8
    /* Returns 0 (resetPcStat's call). */
    int  empty1d84b0();                                                  // ROR 0x001d84b0
    /* The ELF's inlines; nothing calls them. */
    int  getPlayTime_HH() const { return m_playTimeHH; }                 // ROR 0x0028e254
    void *getSectaObjPtr() const { return NULL; }                        // ROR 0x0028e3c4
    /* Empty (an inline in the ELF; nothing calls it). */
    void rmSnd() {}                                                      // ROR 0x0028eb60

    /* An {a, b} pair the system calls 0x32..0x35 set (in the ROM b, the
     * script's a2, comes first): the ELF's regist_call_back_ehon / _menu /
     * _seq_evt and regist_jump_area_act. layout: host (two ints, EE 8). */
    struct SeqPair
    {
        int m_b;                // ROR 0x0: the script's a2
        int m_a;                // ROR 0x4: the script's a1
    };

    int          m_rate;            // ROR 0x00: getRate; the constructor sets 1
    int          m_04;              // ROR 0x04: getStatus 28
    GamArea      m_area;            // ROR 0x40 (bss 0x781300)
    void        *m_18bc0;           // ROR 0x18bc0: 0x600 bytes (init)
    GamObjSet   *m_18bc4;           // ROR 0x18bc4: 0xc00 bytes (init), the object bits onSubSeqEnd applies (writer not found yet)
    GamObjTable *m_objTable;        // ROR 0x18bcc: W000.WRD file 1
    GamObjRec   *m_objRec;          // ROR 0x18bd0: m_objTable + 0x20, its records
    int          m_18dc0;           // ROR 0x18dc0: the room's WRD table word 0x1c (gamSetWrdRoom)
    int          m_18dc4;           // ROR 0x18dc4
    int          m_18dc8;           // ROR 0x18dc8
    int          m_18dd0;           // ROR 0x18dd0: ... word 0x18, 0 when the room has no entry
    GamSndMan    m_sndMan;          // ROR 0x18e00 (bss 0x79a0c0): the sound table
    u_char      *m_wrdFile2;        // ROR 0x1a680: 0x4000 bytes, W000.WRD file 2
    u_int        m_1a684;           // ROR 0x1a684: 0xcccccccc after init
    GamItemHave  m_itemHave;        // ROR 0x1a6c0: getItemHavePtr
    SeqFixedPool<GamItemSlot, 0x14e> m_itemStock; // ROR 0x1a7c0: getItemStockPtr (used list +0x1a7f0, count +0x1a810)
    int          m_selItemNum;      // ROR 0x1d200: the selected item's count (selectItem)
    GamItemSlot *m_selItem;         // ROR 0x1d204: the selected item's slot, NULL none
    SeqFixedPool<GamTenkyuRec, 4>  m_pool1d240; // ROR 0x1d240: the stage's sky domes (releaseTenkyus)
    int          m_mapModelNo;      // ROR 0x1d300: the scene sky slot (no < MAX_MAP_MODEL)
    SeqFixedPool<ACHRstat *, 0x80> m_achrList;  // ROR 0x1d340: getAchrListPtr
    SeqFixedPool<FCHRstat *, 0x40> m_fchrList;  // ROR 0x1d980: getFchrListPtr
    SeqFixedPool<FCHRstat *, 0x180> m_vfchrList;// ROR 0x1dcc0: getVFchrListPtr (getVFchr's objects)
    SeqFixedPool<FCHRstat *, 0x40> m_avfchrList;// ROR 0x1ef00: getAVFchrListPtr (object pointers: releaseVFchr reads their oid)
    u_char      *m_1f240;           // ROR 0x1f240: 0x5a4 bytes, zeroed by init
    int          m_1f244;           // ROR 0x1f244
    u_char      *m_1f248;           // ROR 0x1f248: 0x42000 bytes
    int          m_vfchrSlot[0x180];// ROR 0x1f24c: vfchr oids (clearVFchrSlot); initGameState clears only 0x180 bytes
    SeqFixedPool<GamActorRec, 0x80> m_pool1f880; // ROR 0x1f880: the stage-2 actors (initStage2)
    SeqFixedPool<GamRec8, 0x80>    m_pool202c0; // ROR 0x202c0
    SeqFixedPool<GamObjFxRec, 0x80> m_pool20b00; // ROR 0x20b00
    ACHRstat    *m_achr[0x80];      // ROR 0x21340: by oid slot (set_achr)
    FCHRstat    *m_fchr[0x280];     // ROR 0x21540: by oid slot (slots 1..0x27f)
    int          m_cur_stat;        // ROR 0x21f40: getCurStat; 0 system data, 1 a stage loading
    int          m_prev_stat;       // ROR 0x21f44: m_cur_stat before the last requestStage
    u_int        m_flags;           // ROR 0x21f48: getSysFlg; 1 one second passed (isOneSec), 0x400 setExistMenu, 0x40000 the reset frame
    u_int        m_flags2;          // ROR 0x21f4c: chkSysFlg2; 0x2000 / 0x4000 read by system call 0x22
    int          m_chapter;         // ROR 0x21f50: getChapter, the stage loadStage loaded
    int          m_21f54;           // ROR 0x21f54
    int          m_vcount;          // ROR 0x21f60: getVCount, frames since init
    int          m_playMinutes;     // ROR 0x21f64: getPlayMinutes adds m_vcount's minutes
    int          m_21f68;           // ROR 0x21f68: getStatus 7
    int          m_cameraMode;      // ROR 0x21f6c: getCameraMode
    int          m_21f70;           // ROR 0x21f70: setCameraMode's third word
    u_int        m_21f74;           // ROR 0x21f74: the map camera mode 4 set last, -1 none
    u_int        m_21f78;           // ROR 0x21f78
    int          m_wrdNo;           // ROR 0x21f7c: getWrdNo, the room (WRD) number; to sequence global [1] on a stage change; -1 after resetStage
    int          m_21f80;           // ROR 0x21f80: ... global [2] (-1 for change kind 2)
    int          m_21f84;           // ROR 0x21f84: ... global [3]
    int          m_21f88;           // ROR 0x21f88: ... global [4]
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
    int          m_murderOid;       // ROR 0x21fd8: setMurder's oid
    int          m_pcActor;         // ROR 0x21fdc: getPcChrNo, the PC's actor number in the scene
    DogActor    *m_pcActorPtr;      // ROR 0x21fe0: the PC's scene actor (setPc)
    int          m_pcMotion;        // ROR 0x21fe4: get_pc_last_mot, the motion system call 0x2a waits on
    int          m_dogChrNo;        // ROR 0x21fe8: getDogChrNo
    int          m_murderChrNo;     // ROR 0x21fec: getMurderChrNo
    int          m_pcJnfc;          // ROR 0x21ff0: is_pc_jnfc
    int          m_21ff4;           // ROR 0x21ff4
    int          m_21ff8;           // ROR 0x21ff8
    int          m_22004;           // ROR 0x22004: EmyManager's object id (getEmyOids)
    int          m_22008;           // ROR 0x22008: ... its second
    int          m_pcEquipItemChrID;// ROR 0x2200c: getPcEquipItemChrID
    int          m_inuTask;         // ROR 0x22010: get_inu_task
    float        m_pcPos[4];        // ROR 0x22020: getPcPos
    float        m_pcRot[4];        // ROR 0x22030: getPcRot
    float        m_dogPos[4];       // ROR 0x22040: getDogPos
    int          m_sysWord[4];      // ROR 0x22050: setPCPosAMJ; system call 0x67 copies four words here
    float        m_pcStart[4];      // ROR 0x22060: the PC's kept start position, [3] its rotation y (setPcStartPos 0xcccccccb; writer not found yet)
    float        m_dogStart[4];     // ROR 0x22070: the dog's
    ObjStat     *m_pcPtr;           // ROR 0x22090: getPCPtr
    ObjStat     *m_dogPtr;          // ROR 0x22094: getDogPtr
    ObjStat     *m_murderPtr;       // ROR 0x22098: getMurderPtr
    u_char       m_pcSave[0xb0];    // ROR 0x220a0: ObjStat::save of the PC across a stage change
    u_char       m_dogSave[0xb0];   // ROR 0x22150: ... of the dog (ObjStat::save writes 0xb0)
    u_char       _22200[8];         // ROR 0x22200
    int          m_pcEquipItemOID;  // ROR 0x22208: getPcEquipItemOID
    short        m_equipItem;       // ROR 0x2220c: getPcEquipItem, 0xa00 = none
    short        m_findTargetItem;  // ROR 0x2220e: getFindTargetItem
    int          m_saved_volume;    // ROR 0x22214: chkNoise; -1 = none saved (SprMan::volumeOn / Off read it)
    int          m_volume;          // ROR 0x22218: setVolume's last volume (0x80 after init)
    int          m_stageReq;        // ROR 0x2221c: afterSeqExec's change kind (3 the stage-2 images, 5 a full reset)
    GamSysData   m_sysData;         // ROR 0x22220: the system pack's file 0
    int          m_sysMsgHandle;    // ROR 0x2227c: getSysMsgHND
    int          m_msgParam[8];      // ROR 0x22280: the message window words, 10, 10, 0xffff, 0x100, 0x100, 0, 0 (initGameState), all eight from setMsgParam
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
    GamImage    *m_image[0x20];     // ROR 0x222e4: setImage's slots (system call 0x14)
    intptr_t     m_seqWord;         // ROR 0x22364: setEmyParam; system call 0x15 stores its a1
    char         m_2236c[4][0x40];  // ROR 0x2236c: four strings, emptied by initGameState
    SeqFixedPool<GamRec16, 0x100> *m_pool2246c; // ROR 0x2246c: init's new (0x1840 bytes)
    GamCamRec    m_camStack[4];     // ROR 0x22470: pushCamera / popCamera (three used)
    int          m_camDepth;        // ROR 0x22530: the records pushed
    I3dFog       m_fogStack[3];     // ROR 0x22540: pushFog / popFog; m_color[0] -1 = a free slot
    int          m_msgMode;         // ROR 0x225d8: chgMsgMode
    int          m_225dc;           // ROR 0x225dc: initStage2 clears it
    int          m_225e4;           // ROR 0x225e4
    void        *m_keepBuf;         // ROR 0x225e8: dropMap's m_p_keep_buf (releaseTenkyus frees it)
    int          m_225f0;           // ROR 0x225f0
    int          m_225f4[0x1a];     // ROR 0x225f4: init's table (0x7f, 0x6f, 0x27f, 0x2ff, 3, 8, 4, 5, ...)
    GamBgmRec    m_bgmRec;          // ROR 0x22a5c: onSubSeqEnd hands it to GamSndMan::setBgmRec (writer not found yet)
    u_char      *m_evtPack;         // ROR 0x22a80: getEvtPack, 0xa0000 bytes, W000.WRD file 4
    int          m_22a84;           // ROR 0x22a84
    int          m_playTimeHH;      // ROR 0x22aa0: getPlayTime_HH
    int          m_22ab0;           // ROR 0x22ab0
};

extern GamMan g_gamMan;         /* bss 0x7812c0 */

/* Outside GamMan (files not yet defined). */
void gamClearFrameReq(void);                                             // ROR 0x001f688c
void gamSystemDataLoaded(void);                                          // ROR 0x001f8fb8
/* execStageReq's last call for change kind 2. */
void gamStage2Loaded(void);                                              // ROR 0x001fbbd0
/* Effect kind 0x1a created in the scene's +0x837e0 table (initStage2's
 * first call). */
void gamCreateEff1a(void);                                               // ROR 0x00172678
extern int    g_bss_780d04;     /* bss 0x780d04: option 0's brightness word (32..176) */
/* The words GamMan, MagScene and the menus read through bss 0x7b2060 /
 * 0x7b205c are g_sequence.m_bits / m_global (Sequence +0xdc20 / +0xdc1c,
 * stored by Sequence::init): the script's flag bitset and globals. */

/* ---- outside this file (unassigned@00143d3c: MagScene's Brown glue;
 * unassigned@001e6cac: the chr task helpers) ---- */
/* Brown's controller mode: 0 when on, 2 when off. */
void gamSetBwnFollow(int on);                                            // ROR 0x00144334
/* The find-target item to Brown's controller (0 none). */
void gamSetBwnItem(int item);                                            // ROR 0x00144500
/* Brown's controller mode: 1 when on, 0 when off. */
void gamSetBwnWait(int on);                                              // ROR 0x0014457c
/* Brown's controller (MagScene +0x2bd20) gets the dog's position / rotation
 * (0x12a808 / 0x12a89c; a debug trace of the vector first). */
void gamSetBwnPos(const float *pos);                                     // ROR 0x00144398
void gamSetBwnRot(const float *rot);                                     // ROR 0x001443d8
/* Changes the motion of oid's actor (an achr through a task in its slot
 * +0x3dc); rotY >= 32769 keeps the current rotation. */
void *chrChangeMotion(u_int oid, int motion, int a2, int a3, float blend,
                      float rotY);                                        // ROR 0x001f5e48
/* Creates item id's object (its chr no and model from the system data),
 * placed at pos / rot when given; returns its oid. */
u_int chrCreateItemObj(short id, const float *pos, const float *rot);    // ROR 0x001f6578
struct CmnITask;
/* The motion helpers of native 100. name != NULL picks the motion by
 * name. An achr's motion task lives in its slot +0x3dc (m_task[2]):
 * chrKillMotionTask kills it; chrMtnMove / chrMtnMove2 replace it with a
 * new TskMtnMove / TskMtnMove2 (rotY >= 32769 keeps the heading). */
void chrKillMotionTask(u_int oid);                                       // ROR 0x001f61ec
CmnITask *chrMtnMove(float speed, float rotY, u_int oid, int motion, int p1d8, u_int flags,
                     const char *name);                                  // ROR 0x001f59b4
CmnITask *chrMtnMove2(float speed, u_int oid, int motion, int p1d8, u_int flags,
                      const char *name);                                 // ROR 0x001f5bdc
/* chrChangeMotion to the motion DogActor::getMotionNoBySuffix(name) gives. */
int  chrChangeMotionBySuffix(float blend, float rotY, u_int oid, const char *name, int a2,
                             int a3);                                    // ROR 0x001f5d90
/* MagScene::changeActorMotion on actor no keeping its heading; a speed
 * outside 0.0001..100 becomes 1/60. */
void chrSetMotion(float speed, int no, int motion, int p1d8, u_int flags, u_int oid,
                  const char *name);                                     // ROR 0x001f6308

#endif /* _GAME_GAMMAN_H */
