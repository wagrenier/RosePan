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
#include "sdk/libvu0.h"

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
            float m_1c;         // ROR 0x1c: 0  } the direction: EffBloodSpray1 sprays along it,
            float m_20;         // ROR 0x20: -1 } EffBloodStay1 lays its stains across it
            float m_24;         // ROR 0x24: 0  } (every caller: straight down)
            float m_scale;      // ROR 0x28: 1, 1.75 for a kind-2 damage; 100 for kind 3
            int   m_2c;         // ROR 0x2c: kind 3 (HfmOutDead::Init): 90, the frames it stays (< 0 forever)
        } m_hit;                // kind 1 (scnDamageStart); kind 3 the same words
        struct
        {
            const float *m_pos; // ROR 0x10: the caller's position, read at every drop
            const float *m_dir; // ROR 0x14: the caller's direction (down when shorter than 0.9)
            float m_scale;      // ROR 0x18
            u_int m_frames;     // ROR 0x1c: frames it sprays (a drop every second one)
        } m_spray2;             // kind 2 (EffBloodSpray2; script op 15)
        struct
        {
            const float *m_pos; // ROR 0x10: the caller's position, followed while it lives
            const float *m_up;  // ROR 0x14: the trail's normal (it lies across up x direction)
            float m_width;      // ROR 0x18
            float m_length;     // ROR 0x1c: the trail's length (31 segments)
        } m_trail;              // kind 4 (EffBloodStay2; script op 17)
        struct
        {
            int   m_type;       // ROR 0x10: 0 / 1 the texture half (other types draw nothing)
            int   m_flip;       // ROR 0x14: type 0: non-zero mirrored; type 1: bit 2 mirrored
            const float *m_pos; // ROR 0x18: copied at init
            float m_rot;        // ROR 0x1c: the y rotation
            float m_alpha;      // ROR 0x20
        } m_prim;               // kind 6 (EffBloodStay3Prim)
        char m_suffix[0x70];    // ROR 0x10: kinds 0x33 / 0x34, the pack name's suffix (createEff)
        struct
        {
            float m_pos[3];     // ROR 0x10
            float m_near;       // ROR 0x1c: EffFindItem1::m_near
        } m_find;               // kind 0x32 (EffFindItem1::init)
        struct
        {
            float m_pos[3];     // ROR 0x10: the gun's gun_mazzle node, world space
        } m_muzzle;             // kind 0x26 (MagLifePC::createMuzzleEff)
        struct
        {
            float m_pos[3];     // ROR 0x10: the PC actor's m_trans
            float m_rot[3];     // ROR 0x1c: its m_rotate
            int   m_shots;      // ROR 0x28: MagLifePC::m_shots
        } m_reload;             // kind 0x27 (MagLifePC::reload)
        struct
        {
            const float *m_pos; // ROR 0x10: the caller's position (read while it lives)
            const float *m_dir; // ROR 0x14: the caller's direction
            float m_18;         // ROR 0x18: 0.5 (ClrOut*)
            float m_1c[6];      // ROR 0x1c: 1 (ClrOut*)
            int   m_34;         // ROR 0x34: 8 (ClrOut*)
        } m_breath;             // kind 0xa (EffBreath2; ClrOutMove / ClrOutAtk)
        struct
        {
            float m_pos[3];     // ROR 0x10
            float m_1c;         // ROR 0x1c: 100 (ClrOut*)
            float m_20[3];      // ROR 0x20: 1 (ClrOut*)
            float m_2c;         // ROR 0x2c: 0.5 (ClrOut*)
            int   m_30;         // ROR 0x30: frames: 100 / 200 (ClrOut*, its hit point's life)
        } m_ground;             // kind 0xc (EffBreath2Ground; ClrOutMove / ClrOutAtk)
        struct
        {
            float m_pos[3];     // ROR 0x10
            float m_scale[2];   // ROR 0x1c: the sprite's width and height
            float m_color[4];   // ROR 0x24: 0..1, alpha last
            float m_near;       // ROR 0x34: the pull toward the camera
            int   m_fog;        // ROR 0x38: non-zero = fogged (EFF_LIGHT_FOG)
        } m_light1;             // kind 0x21 (EffLight1::init)
        struct
        {
            float m_pos[3];     // ROR 0x10
            float m_scale[2];   // ROR 0x1c: the sprite's width and height
            float m_color[4];   // ROR 0x24: 0..1, alpha last
            int   m_tex;        // ROR 0x34: the pack texture
            float m_fade_near;  // ROR 0x38: full alpha up to this distance from the eye
            float m_fade_far;   // ROR 0x3c: gone (not drawn) from this distance
            int   m_fog;        // ROR 0x40: non-zero = fogged (EFF_LIGHT_FOG)
        } m_light2;             // kind 0x22 (EffLight2::init)
        struct
        {
            int   m_mode;       // ROR 0x10: 0 = flash light set 1's lights, 1 = only the ambient
        } m_light3;             // kind 0x23 (EffLight3::init)
        struct
        {
            float m_pos[3];     // ROR 0x10: the vent
            float m_rot[2];     // ROR 0x1c: its x / y angles (the puffs go along its z axis)
            float m_size;       // ROR 0x24: the puffs' size scale
            float m_speed;      // ROR 0x28: their speed scale
            float m_color[4];   // ROR 0x2c: 0..1, alpha last
            int   m_frames;     // ROR 0x3c: frames of each period it puffs (one every third)
            int   m_pause;      // ROR 0x40: frames of each period it rests
        } m_steam1;             // kinds 0x29 / 0x2d (EffSteam1::init)
        struct
        {
            const float *m_pos; // ROR 0x10: four floats copied at init (EffSteam1's matrix row 3)
            float m_dir[3];     // ROR 0x14: the direction (EffSteam1's z axis, turned)
            float m_size;       // ROR 0x20: the size scale
            float m_speed;      // ROR 0x24: the start speed, times 15
            float m_fade;       // ROR 0x28: how fast it fades once at full alpha
            float m_color[4];   // ROR 0x2c: 0..1, alpha last
        } m_steam1prim;         // kinds 0x2a / 0x2e (EffSteam1Prim::init; EffSteam1 creates 0x2a)
        struct
        {
            float m_pos[3];     // ROR 0x10: the centre (wisps start within m_radius / 2 of it)
            float m_radius;     // ROR 0x1c: also the wisps' size; they start m_radius / 2 toward -y
            float m_alpha;      // ROR 0x20
        } m_steam2;             // kind 0x2b (EffSteam2::init)
        struct
        {
            float m_pos[3];     // ROR 0x10
            float m_size;       // ROR 0x1c
            float m_alpha;      // ROR 0x20
        } m_steam2prim;         // kind 0x2c (EffSteam2Prim::init; EffSteam2 creates it)
        struct
        {
            float m_pos[3];     // ROR 0x10: the origin (it starts within m_size / 2 of it)
            float m_rot;        // ROR 0x1c: the y angle it drifts along
            float m_size;       // ROR 0x20: the sprite's size
            float m_speed;      // ROR 0x24: its drift per frame
            float m_dist;       // ROR 0x28: how far it drifts before it starts again
            float m_color[4];   // ROR 0x2c: 0..1, alpha last
        } m_cloud;              // kind 0x11 (EffCloud1::init)
        struct
        {
            float m_pos[3];     // ROR 0x10
            float m_rot;        // ROR 0x1c: the y angle
            float m_scale;      // ROR 0x20
            float m_uspeed;     // ROR 0x24: texture u scroll per second
            float m_vspeed;     // ROR 0x28: v scroll per second
            float m_color[4];   // ROR 0x2c: 0..1, alpha last
        } m_water1;             // kind 0x2f (EffWater1::init)
        struct
        {
            float m_box[2][3];  // ROR 0x10: two opposite corners (x, y, z), any order
            int   m_mode;       // ROR 0x28: 0, or non-zero: bigger, faster drops with ground ripples, a denser grid, lighter fog
        } m_water3;             // kind 0x31 (EffWater3::init)
    };
};

/* Kind 0x32 (pack finditem1): a blinking sprite at m_pos, pulled m_near
 * toward the camera -- shown 15 frames out of every 45, its texture
 * stepping 0, 1, 2, 3, 2, 1, 0 while shown. Its code opens
 * eff/effman.cpp (0x176694..0x176a3c). EE 0x50. layout: host. */
class EffFindItem1 : public Eff0
{
public:
    EffFindItem1() {}                                   // ROR 0x0028c928 (linkonce; inlined in createEff)
    virtual ~EffFindItem1() {}                          // ROR 0x0028c878 (linkonce)
    virtual int  init(const EffParam *param);                                // ROR 0x00176694
    virtual int  liveMain();                                                 // ROR 0x00176700
    virtual void render(RENDERPARAM *param);                                 // ROR 0x00176728
    virtual u_long *renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt); // ROR 0x00176754
    virtual void updateZDepth();                                             // ROR 0x001769ec

    sceVu0FVECTOR m_pos;        // ROR 0x30: w 1
    float         m_near;       // ROR 0x40: how far the sprite is pulled toward the camera
    u_char        m_frame;      // ROR 0x44: 0..44, rendered while < 15
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
