/* ==========================================================================
 *  eff/effman.h
 *
 *  EffMan (eff/effman.cpp, 0x176694..0x1788ec; the class name is inferred,
 *  the binary has no symbol or source string for it): the scene's effect
 *  manager at MagScene +0x837e0. It owns
 *  - a table of 0x110 Eff0 objects: entries 0..0xff are live effects (the
 *    numbers createEff returns and natives 281 / GamMan's obj effects keep),
 *    0x100..0x10f hold effects retired to free their number (the table was
 *    full and they were weak) until their live returns 0;
 *  - a draw order of the 0x110 entry numbers, sorted far to near each frame;
 *  - a FIFO of the weak live entries, oldest first (the eviction order);
 *  - 16 packs read from data/eff/eff.bin (an RTPK whose 0x800-byte index is
 *    kept at m_index): per effect kind a pack of models (I3D_BIN), motions
 *    (I3D_I3M) and textures (TIM2, added to the scene's DogTexInfo).
 *  EE size 0x12c8 (MagScene's next member, m_movie, is at +0x84aa8).
 * ======================================================================== */
#ifndef _EFF_EFFMAN_H
#define _EFF_EFFMAN_H

#include "eff/eff0.h"
#include "game/gamfile.h"

struct DogActor;
struct MagScene;
struct i3dContext;

/* EffMan::createEff's parameter block (EE 0x80; natives 281 fill it on
 * the stack). m_kind picks the class (1..0x34) and its pack. layout: host --
 * word 0x10 holds a DogActor for kind 0x15, so the data that follows moves. */
struct EffParam
{
    int      m_kind;            // ROR 0x00
    int      _04;
    int      m_08;              // ROR 0x08: Eff0::m_delay; 0 from every caller seen
    EffPack *m_pack;            // ROR 0x0c: createEff writes the kind's pack (NULL none)
    union
    {
        u_char m_data[0x70];    // ROR 0x10: the script's 0x70 bytes (native 281 ops 0..49)
        struct
        {
            DogActor *m_actor;  // ROR 0x10
            int       m_args[12]; // ROR 0x14
        } m_achr;               // kind 0x15 (native 281 op 38)
        const void *m_file;     // ROR 0x10: kind 0xd, an event pack's effect file (scenePlayStart)
        struct
        {
            float m_pos[3];     // ROR 0x10
            int   m_1c;         // ROR 0x1c: 0
            float m_20;         // ROR 0x20: -1
            int   m_24;         // ROR 0x24: 0
            float m_scale;      // ROR 0x28: 1, 1.75 for a kind-2 damage; 100 for kind 3
            int   m_2c;         // ROR 0x2c: kind 3 (HfmOutDead::Init): 90
        } m_hit;                // kind 1 (scnDamageStart); kind 3 the same words
        char m_suffix[0x70];    // ROR 0x10: kinds 0x33 / 0x34, the pack name's suffix (createEff)
    };
};

/* One entry of a pack: a model or motion (relocated in place) or a
 * texture's number in the scene's DogTexInfo. EE one word. layout: host --
 * a pointer. */
union EffPackFile
{
    void *m_ptr;
    int   m_tex;
};

/* One of EffMan's 16 packs (EE 0x60). layout: host -- pointers. */
struct EffPack
{
    int         m_kind;         // ROR 0x00: the effect kind it was loaded for, 0 = free
    char        m_name[0x20];   // ROR 0x04: its file name in eff.bin (makePackName)
    int         m_keep;         // ROR 0x24: bit 0 = releasePacks(0) leaves it (the init preloads)
    void       *m_data;         // ROR 0x28: the pack, read from eff.bin into the game heap
    int         m_ref;          // ROR 0x2c: Eff objects using it (Eff0::init / term)
    EffPackFile m_file[8];      // ROR 0x30: the models, then the motions, then the textures
    int         m_num_i3r;      // ROR 0x50
    int         m_num_i3m;      // ROR 0x54
    int         m_num_tex;      // ROR 0x58
    int         _5c;
};
static_assert(offsetof(EffPack, m_keep) == 0x24, "m_keep");
static_assert(offsetof(EffPack, m_data) >= 0x28, "m_data");

/* What loadPack hands readPack (its stack, EE 0x2c). layout: host -- the
 * pack pointer. */
struct EffPackReq
{
    EffPack *m_pack;            // ROR 0x00: the slot to fill
    int      m_kind;            // ROR 0x04
    char     m_name[0x20];      // ROR 0x08
    int      m_keep;            // ROR 0x28
};

#define EFF_MAX       0x110     /* table entries */
#define EFF_MAX_LIVE  0x100     /* entries createEff hands out; the rest hold retired effects */
#define EFF_MAX_PACK  0x10

/* EE 0x12c8. layout: host -- pointers. */
struct EffMan
{
    EffMan();                                                            // ROR 0x00176a3c
    ~EffMan();                                                           // ROR 0x00176aa0

    /* MagScene::init: the scene and context, the index of eff.bin, the
     * nine packs every map keeps. Returns 1. */
    int  init(i3dContext *ctx, MagScene *scene);                         // ROR 0x00176afc
    /* MagScene::term: every effect and pack released, eff.bin closed. */
    void term();                                                         // ROR 0x00176c50
    /* News the Eff class of param->m_kind into a free table entry; its
     * number (scenePlayStart, native 281). */
    int  createEff(EffParam *param);                                     // ROR 0x00176cc8
    /* Puts eff in a free entry, evicting the oldest weak one when full;
     * its number, -1. */
    int  addEff(Eff0 *eff);                                              // ROR 0x001774e0
    /* Deletes entry no (GamMan::ctlObj1cbb80). */
    void release(int no);                                                // ROR 0x0017756c
    /* Deletes every entry and every pack not kept (GamMan::clearPool20b00). */
    void releaseAll();                                                   // ROR 0x0017762c
    int  findFreeNo();                                                   // ROR 0x001776c4
    /* MagScene::update's step: every effect's live, the finished deleted. */
    void live();                                                         // ROR 0x0017772c
    /* MagScene::draw's world pass. */
    void draw(i3dContext *ctx);                                          // ROR 0x00177830
    int  sortDrawOrder();                                                // ROR 0x001779b8
    int  packDrawOrder();                                                // ROR 0x00177a8c
    void insertDrawOrder(int n);                                         // ROR 0x00177c00
    I3dLayer *createLayer(i3dContext *ctx, float key);                   // ROR 0x00177d20
    void pushWeak(int no);                                               // ROR 0x00177e04
    void removeWeak(int no);                                             // ROR 0x00177e24
    int  popWeak();                                                      // ROR 0x00177e9c
    void retireEff(int no);                                              // ROR 0x00177ef8
    /* out = the pack name of kind, then suffix (NULL: none). */
    static void makePackName(int kind, char *out, const char *suffix);   // ROR 0x00177f54
    void loadIndex();                                                    // ROR 0x001780c0
    int  findPack(int kind, const char *name);                           // ROR 0x00178148
    int  findFreePack();                                                 // ROR 0x001781d0
    /* The slot of kind's pack (name + suffix), read into a free slot when
     * not loaded; -1. */
    int  loadPack(int kind, const char *suffix, int keep);               // ROR 0x0017820c
    int  readPack(EffPackReq *req);                                      // ROR 0x001782cc
    void releasePack(int n, int force);                                  // ROR 0x001787b0
    void releasePacks(int force);                                        // ROR 0x00178894

    MagScene   *m_scene;                // ROR 0x0000
    Eff0       *m_eff[EFF_MAX];         // ROR 0x0004
    int         m_next_no;              // ROR 0x0444: findFreeNo's round-robin start
    int         m_num_eff;              // ROR 0x0448: entries 0..0xff in use
    int         m_order[EFF_MAX];       // ROR 0x044c: entry numbers, the draw order
    int         m_weak[EFF_MAX];        // ROR 0x088c: weak entry numbers, oldest first
    int         m_num_weak;             // ROR 0x0c8c
    void       *m_index;                // ROR 0x0c90: eff.bin's first 0x800 bytes (its RTPK index)
    EffPack     m_pack[EFF_MAX_PACK];   // ROR 0x0c94
    int         m_no_1a;                // ROR 0x1294: the kind-0x1a effect (one at a time), -1 none
    int         m_no_1f;                // ROR 0x1298: the kind-0x1f effect, -1 none
    FileReader  m_reader;               // ROR 0x129c
    FileReadReq m_req;                  // ROR 0x12a4: eff.bin, kept open between reads
};

/* The scene and context EffMan::init stored (the Eff classes read them). */
extern MagScene   *g_effScene;                  /* 0x298e08 */
extern i3dContext *g_effCtx;                    /* 0x298e0c */

#endif /* _EFF_EFFMAN_H */
