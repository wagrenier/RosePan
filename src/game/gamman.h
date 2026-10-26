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
    /* afterSeqExec's stage change kind 3: the rects / triggers of kind. */
    void stopKind(int kind);                                             // ROR 0x001dd7bc
    void clearKind(int kind);                                            // ROR 0x001dddbc
    /* 1 when one of its rects (the list +0xd0) has id. */
    int  hasId(u_int id);                                                // ROR 0x001e0d70
};

struct DogActor;

/* An object id (OID_STATE in the ELF's symbols): bits 28..29 the list
 * (0 vfchr, 1 achr, 2 fchr), bits 23..27 the object type, bits 0..8 the
 * slot. */
static inline u_int gamOidList(u_int oid) { return oid >> 0x1c & 3; }
static inline u_int gamOidType(u_int oid) { return oid >> 0x17 & 0x1f; }
static inline u_int gamOidSlot(u_int oid) { return oid & 0x1ff; }

/* A game object's state (class ObjStat, vtable 0x2e4828, vptr at +0x258;
 * ACHRstat / FCHRstat derive from it; its code is in unassigned@001d3b44).
 * The ELF's inlines read +0x100 (oid), +0x108 (the scene actor, -1 none),
 * +0x190 position, +0x1a0 rotation. save / load copy the 0xb0 bytes from
 * +0x100. layout: host -- EE offsets in the comments. */
struct ObjStat
{
    /* MagScene::releaseActor(m_actor) when it has one, m_actor = -1; a
     * type-5 object also drops its GamMan pool 0x20b00 records. */
    void releaseActor();                                                 // ROR 0x001d966c
    /* The actor's position into +0x190 and out (x, y, z, w); moves the
     * object to another secta cell when it left the current one. */
    void updatePos(float *out);                                          // ROR 0x001d8f04
    /* The actor's rotation (or +0x1a0 without one) into out[0..2]. */
    void getRot(float *out);                                             // ROR 0x001d8964
    /* The actor's position into +0x190; a new secta cell when it left
     * the current one's bounds. */
    void syncActorPos();                                                 // ROR 0x001d8dc4
    /* The secta cell of +0x190 from scratch. */
    void findSecta();                                                    // ROR 0x001d8ea8
    /* Hides (hide) / shows the actor through its setHide; with bit 0x40
     * of m_flags showing re-finds the secta cell, hiding re-links it. */
    void setHide(int hide);                                              // ROR 0x001d95c4
    /* Refreshes position and rotation, then copies +0x100..+0x1a0 to out. */
    int  save(void *out);                                                // ROR 0x001d96ec
    /* save's inverse: the 0xb0 bytes back to +0x100, the secta words
     * cleared. Returns 0xb0. */
    int  load(const void *in);                                           // ROR 0x001d9830
    /* Clears the secta words (+0x10c too when all), then 0x1da07c. */
    void clearSecta(int all);                                            // ROR 0x001d9f10
    /* +0x100, the object id (the ELF's inline). */
    // ROR 0x0028f92c
    int  getOID() const { return (int)m_oid; }

    u_int m_oid;                // ROR 0x100
    int   m_actor;              // ROR 0x108: the scene actor number, -1 none
    u_int m_flags;              // ROR 0x120: 1 hidden, 0x40 off the secta, 0x10000 the PC / dog, 0x200000, 0x10000000
    int   m_link;               // ROR 0x254: the oid of an object shown with this one, -1 none
};

/* An actor character (vtable 0x2e4810, EE 0x480 bytes, built by
 * 0x1cc250). layout: host. */
struct ACHRstat : ObjStat
{
    /* Pauses (on) / resumes its tasks: +0x3d4, +0x3e0, +0x3d8 and, with
     * all, +0x3dc. */
    void pauseTasks(int on, int all);                                    // ROR 0x001da130
    /* One of its params by which (0x18 the collision kind and ids ...). */
    void getParam(int which, void *out);                                 // ROR 0x001da228
    /* Stores *data into the param which (0x32 the equipped item ...). */
    void setParam(int which, const void *data);                          // ROR 0x001da740

    int   m_task[4];            // ROR 0x3d4: task ids, -1 none
    int   m_taskId;             // ROR 0x3e8: resetObj kills it
};

/* A field character (vtable 0x2e47f8, EE 0x2c0 bytes). layout: host. */
struct FCHRstat : ObjStat
{
    /* ObjStat::getRot's twin: the actor's rotation or +0x1a0. */
    void getRot(float *out);                                             // ROR 0x001db774
    /* One of its params by which (0x41 the type-9 number, 0x18 ...). */
    void getParam(int which, void *out);                                 // ROR 0x001db31c
};

/* GamMan::getObjParam(0x18)'s answer: the object's private collisions
 * (kind 5 one in m_colli[0], kind 6 four). EE 0x28 at least; the ROM's
 * caller clears only m_kind. */
struct GamObjColli
{
    int m_kind;                 // ROR 0x00: -1 before the call
    int _04[5];
    int m_colli[4];             // ROR 0x18: MagScene private collision ids
};
static_assert(offsetof(GamObjColli, m_colli) == 0x18, "m_colli");

/* The system pack's file 0 (GamMan +0x22220; class name invented): ten
 * pointers past the 0x40-byte header from its offsets +0x10..+0x34, the
 * data at +0x58. layout: host -- no members. */
struct GamSysData
{
    void set(const u_char *data);                                        // ROR 0x001dbef8
    /* The record of id: table id >> 9 of the ten, record id & 0x1ff. */
    const void *getRec(short id);                                        // ROR 0x001dbfa0
    /* Ors bits into the record's flag word (+0x90 / +0xb0). */
    void setFlags(short id, u_int bits);                                 // ROR 0x001dcd38
    /* The record's flag word & bits != 0 (0x20000: the item stacks). */
    int  testFlags(short id, u_int bits);                                // ROR 0x001dd00c
    /* Field field of record id into out (0xf: the record's model id, a
     * u_short, 0xffff none; others words or pointers). */
    void getField(int id, int field, void *out);                         // ROR 0x001dc48c
};

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
struct GamRec12 { u_int _0[3]; };
struct GamRec16 { u_int _0[4]; };
static_assert(sizeof(GamRec4) == 4, "GamRec4");
static_assert(sizeof(GamRec8) == 8, "GamRec8");
static_assert(sizeof(GamRec12) == 0xc, "GamRec12");
static_assert(sizeof(GamRec16) == 0x10, "GamRec16");

/* A sky dome record of the pool +0x1d240 (EE 0x10): releaseTenkyus
 * hands m_no to MagScene::releaseTenkyu. */
struct GamTenkyuRec
{
    u_int m_flags;              // ROR 0x0: bit 0 cleared by resetTenkyuFlags (role unverified)
    int _4;
    int m_no;                   // ROR 0x8: the scene's tenkyu number
    int _c;
};
static_assert(sizeof(GamTenkyuRec) == 0x10, "GamTenkyuRec");
static_assert(offsetof(GamTenkyuRec, m_no) == 0x8, "m_no");
static_assert(offsetof(GamTenkyuRec, m_flags) == 0x0, "m_flags");

/* A record of the pool +0x20b00 (EE 8): an object's entry in the scene's
 * +0x837e0 table (ctlObj1cbb80 releases it). */
struct GamObjFxRec
{
    u_int m_oid;                // ROR 0x0
    int   m_no;                 // ROR 0x4: the Obj176a3c entry, -1 none
};
static_assert(sizeof(GamObjFxRec) == 8, "GamObjFxRec");
static_assert(offsetof(GamObjFxRec, m_no) == 4, "m_no");

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
    void setPcStartPos();                                                // ROR 0x001c3d38
    void setStagePack(void *buf);                                        // ROR 0x001c46f4
    /* Sequence::reapThreads' calls when thread 0x44000000 (the stage
     * image's main thread) / 0x84000000 (image 2's) ends. */
    void onStageSeqEnd();                                                // ROR 0x001c4168
    void onSubSeqEnd();                                                  // ROR 0x001c43f0
    /* Stage change kind 2's second half (stage 2 data). */
    void initStage2();                                                   // ROR 0x001c47c4
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
     * minutes, 4 m_chapter (a short), 5 / 6 m_21f7c / m_21f80, 7
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
    int  getMapCamNum();                                                 // ROR 0x001c7494
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
    /* A new object from model file name (oid 0: one is made from kind
     * >> 10), added with addChr(.., kind2, pos, place); returns the oid,
     * 0 when oid already exists. */
    u_int createObjByName(u_int kind, const char *name, int group, u_int oid,
                          int kind2, const void *pos, int place);        // ROR 0x001ca72c
    /* MagSystem::loadModel(model) bound to oid, addChr when add != 0;
     * returns the scene model number, -1 when oid exists. */
    int  createObjByModel(int model, u_int oid, int kind, const void *pos,
                          int place, int add);                           // ROR 0x001ca8e8
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
    /* The PC's ACHRstat::setParam(which, data), when there is a PC. */
    void setPcParam(int which, const void *data);                        // ROR 0x001cb4f0
    void ctlObj1cb8e4(u_int oid, int a, int b);                          // ROR 0x001cb8e4
    /* Frees oid's +0x20b00 records, releasing their scene +0x837e0 entry. */
    void ctlObj1cbb80(u_int oid);                                        // ROR 0x001cbb80
    /* Frees the +0x20b00 records of kind (after 0x17762c). */
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
    /* Adds the object oid of scene model (-1 none) by its list: an achr
     * (ACHRstat) or an fchr (FCHRstat); kind is its system data id. */
    void addChr(int model, u_int oid, int kind, const void *pos, int place); // ROR 0x001cc250
    /* count achr objects of model registered as a group with 0x7b2140. */
    void addChrGroup(int model, u_int oid, int a, int b, int c, int count, int d,
                     int e, int kind, const void *pos, int place, float f); // ROR 0x001cc560
    /* Stage change kind 2's first half. */
    void releaseStage2();                                                // ROR 0x001cd224
    void releaseObjs(int kind, int arg);                                 // ROR 0x001cdf5c
    /* Sequence::exec's last call: the stage-change requests in flags2. */
    /* Records a stage change for afterSeqExec: kind 1 chapter (a full
     * reset, m_stageReq 5), kind 2 room into +0x21f7c, kind 3 a3 into
     * +0x21f80 (m_stageReq 3); word goes to +0x1a684. Kind 2 to the room
     * already loaded only resumes the menu pause. */
    void requestStage(int kind, int room, int a3, int chapter, u_int word); // ROR 0x001cd044
    void afterSeqExec();                                                 // ROR 0x001cd25c
    void storePcTransform();                                             // ROR 0x001cd404
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
    /* oid's rotation from an angle (normalised first). */
    void setObjDir(u_int oid, float dir);                                // ROR 0x001d024c
    /* Clears bits of the object's flags; bits 0x40 / 0x10000000 / 1 / 0x10
     * also re-link, re-place, show and ... it. Returns 0 for no object. */
    int  resetObj(u_int oid, u_int bits);                                // ROR 0x001d1410
    /* MagScene's open menu id != -1. */
    int  isMenuOpen();                                                   // ROR 0x001d1a48
    /* Clears bit 0 of every sky dome record's flags; returns 0. */
    int  resetTenkyuFlags();                                             // ROR 0x001cb1c0
    /* 1 while the message task runs or the system message window is open. */
    int  isMsgBusy();                                                    // ROR 0x001cf9d4
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
    /* In unassigned@001d3b44 (GamMan methods past switchVolume). */
    void relinkObj(u_int oid);                                           // ROR 0x001d5da0
    void placeAchr(int oid, void *out, int arg);                         // ROR 0x001d544c
    /* The table data 0x2fc1f8 (8-word rows, -1 ends): column col of the
     * row whose column keyCol is key (keyCol 99: row key). -1 none. */
    int  lookupFlagTable(int keyCol, int key, int col);                  // ROR 0x001d83d0
    void markReset();                                                    // ROR 0x001d3588
    /* which 1 / 0 / 6 / 7 sets m_flags 0x4000000 / 0x2000000 / 0x8000000 /
     * 0x10000000, then the scene's light update 0x13fb3c. */
    void setLightFlag(int which);                                        // ROR 0x001d2724
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
    int          m_04;              // ROR 0x04: getStatus 28
    GamArea      m_area;            // ROR 0x40 (bss 0x781300)
    void        *m_18bc0;           // ROR 0x18bc0: 0x600 bytes (init)
    void        *m_18bc4;           // ROR 0x18bc4: 0xc00 bytes (init)
    GamObjTable *m_objTable;        // ROR 0x18bcc: W000.WRD file 1
    GamObjRec   *m_objRec;          // ROR 0x18bd0: m_objTable + 0x20, its records
    int          m_18dc4;           // ROR 0x18dc4
    int          m_18dc8;           // ROR 0x18dc8
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
    SeqFixedPool<GamRec12, 0x80>   m_pool1f880; // ROR 0x1f880
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
    int          m_21f7c;           // ROR 0x21f7c: to sequence global [1] on a stage change; -1 after resetStage
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
    int          m_stageReq;        // ROR 0x2221c: afterSeqExec's change kind (3 the stage-2 images, 5 a full reset)
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
    GamImage    *m_image[0x20];     // ROR 0x222e4: setImage's slots (system call 0x14)
    intptr_t     m_seqWord;         // ROR 0x22364: setEmyParam; system call 0x15 stores its a1
    char         m_2236c[4][0x40];  // ROR 0x2236c: four strings, emptied by initGameState
    SeqFixedPool<GamRec16, 0x100> *m_pool2246c; // ROR 0x2246c: init's new (0x1840 bytes)
    GamRec22470  m_22470[4];        // ROR 0x22470: stride 0x30
    int          m_22530;           // ROR 0x22530
    GamRec22540  m_22540[3];        // ROR 0x22540: stride 0x20
    int          m_msgMode;         // ROR 0x225d8: chgMsgMode
    int          m_225e4;           // ROR 0x225e4
    void        *m_keepBuf;         // ROR 0x225e8: dropMap's m_p_keep_buf (releaseTenkyus frees it)
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
/* execStageReq's last call for change kind 2. */
void gamStage2Loaded(void);                                              // ROR 0x001fbbd0
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
/* Changes the motion of oid's actor (an achr through a task in its slot
 * +0x3dc); rotY >= 32769 keeps the current rotation. */
void *chrChangeMotion(u_int oid, int motion, int a2, int a3, float blend,
                      float rotY);                                        // ROR 0x001f5e48
/* Creates item id's object (its chr no and model from the system data),
 * placed at pos / rot when given; returns its oid. */
u_int chrCreateItemObj(short id, const float *pos, const float *rot);    // ROR 0x001f6578

#endif /* _GAME_GAMMAN_H */
