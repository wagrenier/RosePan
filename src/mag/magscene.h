/* ==========================================================================
 *  mag/magscene.h
 *
 *  The scene (mag/magscene*.cpp), EE 0x85500: MagSystem holds it as its
 *  member at +0x12540 (bss 0x6fbd80) up to its own words at +0x97a40. It
 *  owns the light pool (+0x2050), the life manager (+0x2330), 128 actors
 *  of 0x4d0 bytes at +0x2920 (DogActor, dog/dogactor.h), one flag word
 *  per actor at +0x29120, the texture table (+0x29520), four sky slots
 *  (+0x29540), two view records (+0x2aa10), four fog records (+0x2aae0),
 *  the camera (+0x2ad50..), the player's MagLifePC at +0x2adc0, Brown's
 *  MagLifeBwn at +0x2afe0 and MagLifeBwnCtrl at +0x2bd20, the menus
 *  (MagLifeMenu*, +0x2be60..), the 2D layers (+0x41930..) and the scene
 *  interfaces at +0x854e0. Members are the ones reconstructed code
 *  touches; the constructor (0x1360a0) fills in the rest.
 * ======================================================================== */
#ifndef _MAG_MAGSCENE_H
#define _MAG_MAGSCENE_H

#include <stddef.h>

#include "bed/bedpad.h"
#include "bed/bedsubmode.h"
#include "dog/dogactor.h"
#include "dog/dogcamera.h"
#include "dog/doglightpool.h"
#include "dog/dogtexinfo.h"
#include "lib/unassigned.h"
#include "mag/magloader.h"
#include "mag/maglifebwn.h"
#include "mag/maglifebwnctrl.h"
#include "mag/maglifefader.h"
#include "mag/maglifeman.h"
#include "mag/maglifemenu.h"
#include "mag/maglifemenusp.h"
#include "mag/maglifepc.h"
#include "mag/maglifewindow.h"
#include "sdk/sce_gs.h"
#include "spr/s2d.h"

/* A record of a sky pack's <name>_c.bin: a map camera path (the asserts
 * call it ppath / thePath) -- m_pos_num points from +0x30, each (x, y, z,
 * w = the length of the segment that ends there). The camera stands on
 * the path at point m_seg plus the fraction m_seg_t and looks at the
 * target actor (mag/magscenecamera.cpp). MagScene::load initialises the
 * words after the file data. EE 0x30 plus the points. */
struct MagSceneCRec
{
    u_char m_pos_num;           // ROR 0x00: the point count (asserted 1..255); load starts m_seg at the middle
    u_char m_loop;              // ROR 0x01: non-zero: the path closes (the last point leads to the first)
    u_char m_alt;               // ROR 0x02: load stores the file's head word >> 16; 1 with one point: changeCamera flips m_alt_side
    u_char m_alt_side;          // ROR 0x03: a one-point path's view (0 the first point, 1 the point 4 on); load clears it
    float  m_fov;               // ROR 0x04: the lens angle in degrees (setCameraMode)
    u_char _08[0x10 - 0x08];
    float  m_look_rot[2];       // ROR 0x10: the x / y rotation the camera looks around when m_look_tag is set (updateCamera modes 4 / 5)
    u_char _18[0x1c - 0x18];
    char   m_tag[3];            // ROR 0x1c: letters 'a'.. tuning scoreCameraPath: [0] the step speed, [1] the distance, [2] the band
    char   m_look_tag;          // ROR 0x1f: a letter 'a'.. limiting the camera's look around m_look_rot (getCameraLookLimit); 0 = lookAt the actor
    int    m_seg;               // ROR 0x20: the point the camera's segment starts at (pa)
    int    m_goal_seg;          // ROR 0x24: changeCamera's goal for m_seg; load clears it
    float  m_seg_t;             // ROR 0x28: the fraction along the segment, 0 <= pb < 1; load clears it
    float  m_goal_t;            // ROR 0x2c: changeCamera's goal for m_seg_t; load clears it

    float       *points()       { return (float *)(this + 1); }
    const float *points() const { return (const float *)(this + 1); }
};
static_assert(offsetof(MagSceneCRec, m_fov) == 0x04, "m_fov");
static_assert(offsetof(MagSceneCRec, m_look_rot) == 0x10, "m_look_rot");
static_assert(offsetof(MagSceneCRec, m_tag) == 0x1c, "m_tag");
static_assert(offsetof(MagSceneCRec, m_look_tag) == 0x1f, "m_look_tag");
static_assert(offsetof(MagSceneCRec, m_seg) == 0x20, "m_seg");
static_assert(offsetof(MagSceneCRec, m_goal_t) == 0x2c, "m_goal_t");

/* getCameraPathWork's result (the assert names it theWork): the camera's
 * point on a path, its segment's unit direction and length. EE 0x24. */
struct MagCameraPathWork
{
    sceVu0FVECTOR m_pos;        // ROR 0x00: (0, 0, 0, 1) until a path of two or more points fills it
    sceVu0FVECTOR m_dir;        // ROR 0x10: w 0
    float         m_distance;   // ROR 0x20: the segment's length (its end point's w), asserted > 0
};
static_assert(offsetof(MagCameraPathWork, m_distance) == 0x20, "m_distance");

/* A sky ("tenkyu", celestial sphere) slot: the map unit MagScene::load
 * took -- a model, a second model with the STQ map, a collision model,
 * the _c.bin records and the _l.bin light data. EE 0x520. layout: host --
 * holds DogModels and pointers. */
struct MagSceneTenkyu
{
    void    *m_buf;             // ROR 0x000: game-heap buffer, freed by releaseTenkyu
    u_char   _004[0x010 - 0x004];
    DogModel m_model[3];        // ROR 0x010: stride 0x1a0 (the pack's model, its file 2, <name>_col.i3s.i3r)
    int      m_id;              // ROR 0x4f0: -1 = a free slot
    u_int   *m_c_data;          // ROR 0x4f4: <name>_c.bin (head word: count in the low half)
    int      m_c_num;           // ROR 0x4f8: its record count
    MagSceneCRec **m_path_data; // ROR 0x4fc: its records (the ROM turns the file's offsets into pointers in place; drawStrips' assert names it pmd->m_path_data)
    void    *m_light_data;      // ROR 0x500: <name>_l.bin, NULL when the pack has none
    I3rRoot *m_sky;             // ROR 0x504: the pack's tenkyu.rpk sky model (loadTenkyuTex), NULL = none
    sceVu0FMATRIX *m_sky_mats;  // ROR 0x508: its node matrices (drawSkyDome draws by them), one game-heap block with m_sky_spin
    float   *m_sky_spin;        // ROR 0x50c: per node the y turn per frame (initTenkyuSpin; rotateTenkyu applies it)
    int      m_sky_num_node;    // ROR 0x510: the node count, 0 when the pack has no motion
    u_char   _514[0x520 - 0x514];
};
static_assert(offsetof(MagSceneTenkyu, m_model) >= 0x10, "m_model");
/* No bound on m_id: the host DogModel is smaller than the EE's 0x1a0. */
static_assert(offsetof(MagSceneTenkyu, m_id) > offsetof(MagSceneTenkyu, m_model), "m_id");

/* A sky pack's _c.bin record as drawStrips reads it when m_22b4 is set: a
 * path of m_00 points from +0x30 (x, y, z). */
static_assert(sizeof(MagSceneCRec) == 0x30, "the path points follow the header");

/* A strip record (0x2000 of them at scene +0x437e0, reset by clearStrip):
 * what drawStrips draws by type -- 0..2 strips of m_count vertices, 3..5
 * other i3d primitives -- for m_life frames (-1: until cleared). EE 0x20.
 * layout: host -- pointers. */
struct MagSceneStrip
{
    union
    {
        const void *m_colors;   // ROR 0x00: types 0, 1 and 5: a pointer (colours)
        float       m_color[4]; // ROR 0x00: type 2: the colour itself
    };
    void  *m_data;              // ROR 0x10: the positions (types 0..3), type 4's object
    void  *m_data2;             // ROR 0x14: types 3 and 5
    int    m_life;              // ROR 0x18: frames left; -1 = forever
    short  m_count;             // ROR 0x1c: vertices (asserted > 0 for 0..2)
    short  m_type;              // ROR 0x1e: -1 = free
};
static_assert(offsetof(MagSceneStrip, m_data) >= 0x10, "m_data");
static_assert(offsetof(MagSceneStrip, m_type) >= 0x1e, "m_type");

/* castRay's hit record (EE 0x60: drawBlobShadows clears it to five
 * (0, 0, 0, 1) vectors and -1). Field roles unverified. */
struct MagSceneRayHit
{
    sceVu0FVECTOR m_vec[5];     // ROR 0x00: [1] = the hit normal (castWalls: the wall's, unit, w 0)
    int           m_50;         // ROR 0x50: -1 before the cast
    int           _54[3];
};
static_assert(sizeof(MagSceneRayHit) == 0x60, "MagSceneRayHit is 0x60 bytes on the EE");

/* A value running from m_from to m_to over m_frames (EE 0x10; startLerp
 * fills it, updateRecords853fc steps it). */
struct MagLerp
{
    float m_from;               // ROR 0x0
    float m_to;                 // ROR 0x4
    int   m_frame;              // ROR 0x8: frames run, 0 at the start
    int   m_frames;             // ROR 0xc: the length, 0 = done
};
static_assert(sizeof(MagLerp) == 0x10, "MagLerp is 0x10 bytes on the EE");
static_assert(offsetof(MagLerp, m_frames) == 0xc, "m_frames");

/* The scene's focus effect (mag/magscenefocus.cpp; the assert text's
 * m_focus), EE 0x70 at MagScene +0x853bc: the frame shrunk into the
 * efffocus work buffer and laid back over everything beyond a distance.
 * layout: host -- the track pointer. */
struct MagSceneFocus
{
    int         m_mode;         // ROR 0x00: bit 0 draws the far1 plane, bit 1 the far2 plane (twice); 0 = off
    int         m_texindex;     // ROR 0x04: efffocus in m_tex_info, -1 = not added
    float       m_bias;         // ROR 0x08: the next layer's key above 860 (+0.01 per layer, 0 at each drawFocus)
    float       m_val[3];       // ROR 0x0c: [0] the planes' alpha (1), [1] far1 (300), [2] far2 (600); native 281 op 13 drives them too
    const char *m_track;        // ROR 0x18: the focus track's text (setFocusData), NULL = none
    int         m_track_size;   // ROR 0x1c: its size in bytes
    int         m_track_pos;    // ROR 0x20: the next line's offset
    int         m_frame;        // ROR 0x24: frames since the track started (-1 before the first)
    int         m_wait;         // ROR 0x28: the frame the pending line runs at (-1 = none)
    int         _2c[5];         // ROR 0x2c: cleared by initFocus only
    MagLerp     m_lerp[3];      // ROR 0x40: m_val's lerps (startLerp, stepLerp)
};

/* An entry of the actor draw list collectActors fills (EE 0x10). */
struct MagSceneDrawEntry
{
    DogActor *m_actor;          // ROR 0x00
    int       m_key;            // ROR 0x04: 0x10000 when parented, + 65535 - distance to the camera; sortActors orders them ascending (far first, parented last)
    int       m_no;             // ROR 0x08: the actor number
    int       m_parent_no;      // ROR 0x0c: the parent actor's number, -1 = none
};
static_assert(offsetof(MagSceneDrawEntry, m_parent_no) >= 0x0c, "m_parent_no");

/* A view record (the two the scene keeps at +0x2aa10 / +0x2aa48): a
 * frustum, a depth range and a GS screen rectangle. EE 0x38; field roles
 * from initRender's values, unverified. */
struct MagViewParam
{
    float m_near;               // ROR 0x00: 5
    float m_far;                // ROR 0x04: 2000
    float m_left;               // ROR 0x08: -5.12
    float m_right;              // ROR 0x0c: 5.12
    float m_bottom;             // ROR 0x10: -4.48
    float m_top;                // ROR 0x14: 4.48
    float m_zmax;               // ROR 0x18: 65530 (the second view: m_22a4 * 65530)
    float m_zmin;               // ROR 0x1c: 1 (the second view: m_22a4)
    float m_x0;                 // ROR 0x20: 0
    float m_x1;                 // ROR 0x24: 511
    float m_y0;                 // ROR 0x28: 0
    float m_y1;                 // ROR 0x2c: 447
    float m_offx;               // ROR 0x30: 1792 (2048 - 256)
    float m_offy;               // ROR 0x34: 1824 (2048 - 224)
};
static_assert(offsetof(MagViewParam, m_zmax) == 0x18, "m_zmax");
static_assert(offsetof(MagViewParam, m_offy) == 0x34, "m_offy");
static_assert(sizeof(MagViewParam) == 0x38, "MagViewParam is 0x38 bytes on the EE");

/* A light of a map's _l.bin (MagLightFile; EE 0x20, plain floats). */
struct MagLightFileLight
{
    float m_color[3];           // ROR 0x00: point lights: scaled by m_vec[3]
    float m_vec[4];             // ROR 0x0c: directional: the direction (setLightParam reads w too); point: the position, w the intensity
    float _1c;
};
static_assert(offsetof(MagLightFileLight, m_vec) == 0x0c, "m_vec");
static_assert(sizeof(MagLightFileLight) == 0x20, "MagLightFileLight is 0x20 bytes on the EE");

/* A map's _l.bin (MagSceneTenkyu::m_light_data), loadLight's input: the
 * counts, the ambient colour, then one directional light when
 * m_direct_num > 0 and m_point_num point lights. EE layout, plain words. */
struct MagLightFile
{
    int   m_direct_num;         // ROR 0x00: > 0: one directional light follows (only one is read)
    int   m_point_num;          // ROR 0x04
    int   m_shadow_no;          // ROR 0x08: the light (directional first, then the points) that casts the projected shadows
    int   _0c;
    float m_ambient[3];         // ROR 0x10
    float _1c;
    MagLightFileLight m_light[1]; // ROR 0x20: m_direct_num ? 1 : 0 directional, then the point lights
};
static_assert(offsetof(MagLightFile, m_point_num) == 0x04, "m_point_num");
static_assert(offsetof(MagLightFile, m_shadow_no) == 0x08, "m_shadow_no");
static_assert(offsetof(MagLightFile, m_ambient) == 0x10, "m_ambient");
static_assert(offsetof(MagLightFile, m_light) == 0x20, "m_light");

/* The i3d light record the projected shadows are lit by (scene +0x2000;
 * setShadowLight 0x13fbe4 hangs it in m_shadow_light as its one
 * directional light for mode 1, its one spot light for modes 2 and 3).
 * makeActorShadow fills it each shadow. EE 0x40, plain floats. */
struct MagShadowLight
{
    sceVu0FVECTOR m_00;         // ROR 0x00: directional: the direction; spot: the position
    sceVu0FVECTOR m_10;         // ROR 0x10: directional: the colour (w = intensity); spot: the direction
    sceVu0FVECTOR m_color;      // ROR 0x20: spot: the colour (w = intensity)
    float         m_cone_inner; // ROR 0x30: spot: tan of the half angle - 0.1, at least 0
    float         m_cone_outer; // ROR 0x34: spot: tan of the half angle, at most 0.7
    float         _38[2];
};
static_assert(offsetof(MagShadowLight, m_cone_outer) == 0x34, "m_cone_outer");
static_assert(sizeof(MagShadowLight) == 0x40, "MagShadowLight is 0x40 bytes on the EE");

/* The 256 records at the scene's start (EE 0x20 each): wall segments
 * m_a -> m_b that castWalls tests a level ray against (only where m_a.y !=
 * m_b.y, the segment's height range covering the ray's y). The
 * constructor sets both ends to (0, 0, 0, 1), resetEntities clears their
 * y (so no wall is live until something fills them). */
struct MagSceneVecPair
{
    sceVu0FVECTOR m_a;          // ROR 0x00
    sceVu0FVECTOR m_b;          // ROR 0x10
};
static_assert(offsetof(MagSceneVecPair, m_b) == 0x10, "m_b");
static_assert(sizeof(MagSceneVecPair) == 0x20, "MagSceneVecPair is 0x20 bytes on the EE");

#define MAX_WINDOW 4            /* the scene's text windows (asserts winNo < MAX_WINDOW) */
#define MAX_PRIVATE_COLLI 256    /* the wall segments at the scene's start (addPrivateColli) */

/* EE 0x85500. layout: host -- holds the actors and controllers (pointers,
 * vptrs); EE offsets in the comments. */
struct MagScene
{
    MagScene();                                                          // ROR 0x001360a0
    ~MagScene();                                                         // ROR 0x00136d14

    void init();                                                         // ROR 0x00131840
    void setPC(int no, int mode);                                        // ROR 0x001316b8
    void setBwn(int no, int arg);                                        // ROR 0x00131790
    /* After loadSystemData: points the four window fonts at their
     * textures (bit8a..bit8d) and links the scene interfaces. */
    void start();                                                        // ROR 0x00131a28
    int  initRender(i3dContext *ctx);                                    // ROR 0x00131b90
    /* One frame of the scene by sub mode: 0 start, 1 walking, 2 waiting
     * for text windows, 3 fading out to a menu, 4 fading back in, 5 the
     * system message, 6 the PC's state 0x17 camera spin, 7 a menu, 8
     * paused, 9 debug. */
    void update(BedPad *pad0, BedPad *pad1);                             // ROR 0x001327dc
    void updatePaused();                                                 // ROR 0x001327a0
    void updateBwn();                                                    // ROR 0x0013233c
    /* paused 0: the actors without flag 8 (and their footsteps); 1: those
     * with it. */
    void updateActors(int paused);                                       // ROR 0x0013262c
    void playActorFootstep(int theNo);                                   // ROR 0x00132430
    void updateTenkyuBillboards();                                       // ROR 0x00132048
    /* Starts (or with m_mode 0 stops) a fade; see MagFadeParam. */
    void setFade(const MagFadeParam *p);                                 // ROR 0x001311f4
    /* The current fade object's state mapped to 0..6 (0 none; update waits
     * for more than 4). */
    int  getFadeState();                                                 // ROR 0x00131550
    /* setFade with fade kind 0 (white: 4), mode 2 (in) / 6 (out). */
    void fadeIn(int time, int level, int white);                         // ROR 0x00131184
    void fadeOut(int time, int level, int white);                        // ROR 0x001311bc
    int  getFaderWord30();                                               // ROR 0x00131608
    /* Takes the keyhole fade off the life manager and frees its texture. */
    void removeFadeKey();                                                // ROR 0x00135dcc
    /* The scene's PC (m_pc). */
    bool isPCWordDcSet();                                                // ROR 0x0013164c
    /* m_pc.message (0x15: 1 when the PC moved; 0x16: 1 riding or hurt). */
    int  commandPC(int cmd, const float *arg);                           // ROR 0x0013166c
    void setPCState14(int arg, int word);                                // ROR 0x00131690
    /* The actors' motions (no = the actor number). setActorMotion packs
     * the second flag word into flags' low byte (flags >> 8 the first). */
    int  getActorNo(DogActor *thePtr);                                   // ROR 0x00133090
    int  getActorMotion(int no);                                         // ROR 0x001330f4
    void setActorMotion(float speed, int no, int motion, int p1d8, u_int flags); // ROR 0x00133144
    void changeActorMotion(float speed, int no, int motion, int p1d8, u_int flags); // ROR 0x001331f4
    void setActorMotionByName(float speed, int no, const char *name, int p1d8,
                              u_int flags);                              // ROR 0x00133288
    void changeActorMotionByName(float speed, int no, const char *name, int p1d8,
                                 u_int flags);                           // ROR 0x00133324
    /* fps: motion frames per second (the actor keeps fps / 60). */
    void setActorMotionSpeed(float fps, int no);                         // ROR 0x001333c0
    float getActorMotionLength(int no, int motion);                      // ROR 0x001333e4
    float getActorMotionSpeed(int no);                                   // ROR 0x00133440
    /* Sub mode 3: fade out, then openMenu(id) with arg. */
    void requestMenu(int id, int arg);                                   // ROR 0x001357b0
    void openMenu(int id);                                               // ROR 0x0013580c
    bool isMenuRunning();                                                // ROR 0x00135b64
    void closeMenu();                                                    // ROR 0x00135b78
    void pushSubMode();         /* BedSubMode::push on +0x22d0, then mode 8 */ // ROR 0x00132744
    void popSubMode();                                                   // ROR 0x00132784
    /* The text windows (MAX_WINDOW 4, slots at +0x2734; the "view" methods
     * take the same numbers). openWindow returns the slot. */
    int  openWindow(int w, int h, int a3, int life);                     // ROR 0x00134730
    void closeWindow(int no);                                            // ROR 0x001348b0
    void setViewParam(int no, const int *v);                             // ROR 0x00134944
    void fadeInWindow(int no, int time, int from, int to);               // ROR 0x00135088
    void fadeOutWindow(int no, int time, int from, int to);              // ROR 0x0013511c
    /* The window's setHide(hide != 0). */
    void setWindowShow(int no, int hide);                                // ROR 0x001351b0
    int  getWindowTextWidth(int no, const char *text);                   // ROR 0x0013555c
    void setWindowPos(int winNo, int x, int y, int z);                   // ROR 0x00134a00
    void setWindowSize(int winNo, int w, int h);                         // ROR 0x00134ab8
    void getWindowPos(int winNo, int *x, int *y);                        // ROR 0x00134b34
    void getWindowPos3(int winNo, int *x, int *y, int *z);               // ROR 0x00134bb8
    void getWindowSize(int theN, int *w, int *h);                        // ROR 0x00134c4c
    void setWindowWords84(int theN, int a, int b, int c, int d);         // ROR 0x00134cd0
    void getWindowWords84(int theN, int *a, int *b, int *c, int *d);     // ROR 0x00134d6c
    void getWindowWordCc(int theN, int *out);                            // ROR 0x00134e18
    void getWindowCharMetrics(int winNo, int code, int *left, int *width,
                              int *extra);                               // ROR 0x00134e88
    void setWindowProportional(int winNo, int on);                       // ROR 0x00134f2c
    void setWindowFontWord30(int winNo, int v);                          // ROR 0x00134fa0
    void setWindowFontWord34(int winNo, int v);                          // ROR 0x00135014
    void updateWindowFont(int no);                                       // ROR 0x00135594
    void drawView(int no, int a, int b, const void *p, int c);           // ROR 0x00135234
    void setViewFar(float far);                                          // ROR 0x00131618
    void resetEntities();                                                // ROR 0x00135fa4
    /* Frees the font data, the actors' models, the sky slots and the
     * textures, and every life but the fader. */
    void term();                                                         // ROR 0x00131f74
    void clearPC();                                                      // ROR 0x0013604c
    void initCharaLoader();                                              // ROR 0x00137300
    void initMapLoader();                                                // ROR 0x00137318
    int  loadSystemData();                                               // ROR 0x00137330
    /* Takes a map unit pack into a free sky slot (copy: keep a copy in the
     * game heap); returns the slot, -5 / -7 when a model does not load. */
    int  load(i3dContext *ctx, int id, void *data, int copy);            // ROR 0x001377f0
    int  loadMapUnit(i3dContext *ctx, int theNo);                        // ROR 0x0013836c
    void loadMapByName(i3dContext *ctx, const char *theName);            // ROR 0x00138610
    void releaseTenkyu(int no);                                          // ROR 0x001386d4
    void releaseTenkyuAll();                                             // ROR 0x00138804
    int  findFreeActor();                                                // ROR 0x0013884c
    int  findActorById(int theID);                                       // ROR 0x001388e8
    int  findActorByName(const char *theName);                           // ROR 0x001389c4
    int  shareActor(u_int theActorNo);                                   // ROR 0x00138ac8
    /* arg and rpk are passed by MagSystem and not read (the ROM's a3 / t0). */
    int  loadModelByName(i3dContext *ctx, const char *name, int arg,
                         const char *rpk);                               // ROR 0x00138bc4
    int  loadActor(i3dContext *ctx, int theID, void *thePtr, int keep);  // ROR 0x00138ce8
    /* rpk is passed by MagSystem and not read. */
    int  loadModel(i3dContext *ctx, int id, const char *rpk);            // ROR 0x00138ed0
    void releaseActor(u_int theN);                                       // ROR 0x00139010
    void setActorAnim(int no, int a, int b);                             // ROR 0x001390d4
    int  loadActorAnim(int no, int arg, const char *name);               // ROR 0x0013910c
    int  loadTexture(const char *theFile, const char *theName);          // ROR 0x00139308
    void draw(i3dContext *ctx, sceGsDBuff *dbuff, int frame);            // ROR 0x0013bcc0
    void postDraw();                                                     // ROR 0x0013c29c
    void parseOption(char *text);                                        // ROR 0x0013c328
    void setCameraTarget(const float *pos, const float *at);             // ROR 0x0013c2ec
    void initCamera();                                                   // ROR 0x0013c928
    /* Saves the camera entity and the six camera words +0x2ad50.. (a
     * menu's own camera goes over them), counting in m_cam_save_count;
     * restoreCamera puts them back. */
    void saveCamera();                                                   // ROR 0x0013c930
    void restoreCamera();                                                // ROR 0x0013ca10
    void setCameraMode(int mode, int arg, const int *param);             // ROR 0x0013caf0
    /* The ROM's line-816 inline (mag/magscene.h; the name from
     * setCameraMode's assert; GamMan::getMapCamNum): sky slot no's camera
     * path count. */
    int  getCameraMaxPath(int no)
    {
        ROR_ASSERT(no >= 0 && no < MAX_MAP_MODEL); /* 816 */
        return m_tenkyu[no].m_c_num;
    }
    int  getCameraMode(int *out);                                        // ROR 0x0013d5ac
    /* Camera modes 3 / 4 (a map camera's path): the camera placed on the
     * path -- pos 0..100 starts pos percent along it, other values at the
     * point nearest the actor beyond 500 -- then stepped until the
     * actor's distance settles, looking at the actor (native 016 case 7). */
    void seekCameraPath(float pos);                                      // ROR 0x0013d240
    /* theWork for path's m_seg / m_seg_t (paths of one point leave it). */
    void getCameraPathWork(MagCameraPathWork *theWork, MagSceneCRec *path); // ROR 0x0013c664
    /* One step of path's m_seg / m_seg_t toward the distance band to
     * target; 1e6 less the squared distance error (+1e6 when flags bit 1
     * and a ray from the camera point hits something). */
    float scoreCameraPath(MagSceneCRec *thePath, const float *target, u_int flags); // ROR 0x0013d8f8
    int  findCameraPathStart(float pos, const MagSceneCRec *path, const float *target); // ROR 0x0013dfe0
    /* The camera's position thePos[theI] lerped by theF toward the next
     * point (modulo theMax). */
    void setCameraOnPath(float theF, const float *thePos, int theI, int theMax); // ROR 0x0013e190
    /* The current map camera path's point (sky slot m_cam_map_no, path
     * m_cam_path_no): its m_seg plus its fraction m_seg_t, and back
     * (GamMan::pushCamera / popCamera). */
    float getCameraPathPos();                                            // ROR 0x0013d4d0
    void setCameraPathPos(float pos);                                    // ROR 0x0013d52c
    /* Empties light set theNo of the pool (resetSet; < 0 starts over). */
    void setLightNo(int theNo);                                          // ROR 0x0013f4d0
    /* Only asserts theNo < 8 (the destructor passes -1). */
    void checkLightNo(int theNo);                                        // ROR 0x0013f49c
    /* A new pool from a map's _l.bin (MagLightFile): its ambient light in
     * sets 0 and 1, its first directional light and its point lights in
     * set 1; the light m_shadow_no counts to casts the projected shadows. */
    void loadLight(const void *theData);                                 // ROR 0x0013f658
    /* A grey ambient light (0.75) in light set 6. */
    void setDefaultLight();                                              // ROR 0x0013f8e4
    /* Light set no gets theNum[kind] free lights of each kind; set 0 also
     * picks the shadow mode by the first kind it has (point, spot, 5,
     * directional), light 0. */
    void setLightFlags(const int *theNum, int no);                       // ROR 0x0013f528
    /* DogLightPool::setLightParam / getLightParam, or the set-relative
     * twins when no >= 0. */
    void setLightParam(int type, int idx, int which, const float *v, int no); // ROR 0x0013f980
    void getLightParam(int type, int idx, int which, float *v, int no);  // ROR 0x0013fa50
    /* Light set theS of the pool copied over set theD (0..7, different). */
    void copyLightSet(int theD, int theS);                               // ROR 0x0013fb3c
    /* m_shadow_light from light theNo of kind theLight (1 directional, 2
     * point, 3 spot; other kinds: no source light). */
    void setShadowLight(int theLight, int theNo);                        // ROR 0x0013fbe4
    /* A free light of kind type, added to every light set whose bit is set
     * in mask (bits 0..8); its id, < 0 when none is free. */
    int  addLight(int type, unsigned int mask);                          // ROR 0x0013fe64
    void releaseTenkyuTex(int no);                                       // ROR 0x0013ff00
    /* The pack's sky (tenkyu.rpk: an optional I3D_I3M motion, then the
     * i3r) into slot no: its textures into m_tex_info, and with a motion
     * the node spin initTenkyuSpin derives from it. */
    void loadTenkyuTex(int no, const BedPfh *pfh, i3dContext *ctx);      // ROR 0x00140030
    void initTenkyuSpin(MagSceneTenkyu *t, I3mRoot *motion);             // ROR 0x00140390
    /* Fades to fog over frames (0: at once). */
    void setFog(const I3dFog *fog, int frames);                          // ROR 0x00143d7c
    /* The same from a colour and the four params (I3dFog::m_param). */
    void setFog(const float *color, float p0, float p1, float p2, float p3, int frames); // ROR 0x00143d3c
    /* Fades to the target fog plus delta's eight floats. */
    void addFog(const I3dFog *delta, int frames);                        // ROR 0x00143eec
    /* One step of setFog's fade. */
    void updateFog();                                                    // ROR 0x00143fd0
    void updateDebug(BedPad *pad0, BedPad *pad1);                        // ROR 0x0014313c
    /* Once a frame: plays out a pending path move (bit 0x10, 30 frames),
     * else runs the mode -- 1 looks at the actor, 3 takes the best path of
     * every sky slot, 4 / 5 stand on the current path (4 sliding it), 6 the
     * camera's own move. */
    void updateCamera();                                                 // ROR 0x0013e548
    /* The camera's x / y rotation toward target's normalised vector, kept
     * within limX / limY of base (a limit <= 0 keeps base). */
    void clampCameraLook(float limX, float limY, const float *target, const float *base); // ROR 0x0013e2ec
    /* The look limits of letter number n (0: none). */
    void getCameraLookLimit(int n, float *limX, float *limY);            // ROR 0x0013e4c0
    void changeCamera();                                                 // ROR 0x0013d5e4
    /* Turns every sky node about y by its spin (once a frame). */
    void rotateTenkyu();                                                 // ROR 0x0014062c
    /* ---- mag/magscenefocus.cpp ---- */
    /* Once a frame: steps the focus lerps and runs the focus track. */
    void updateFocus();                                                  // ROR 0x00140c68
    /* The constructor's: clears m_focus and sets its defaults. */
    void initFocus();                                                    // ROR 0x00140a84
    /* One frame of rec; returns the value to show. this is not read. */
    float stepLerp(MagLerp *rec);                                        // ROR 0x00140d40
    /* A layer that shrinks the frame buffer at block fbp (0 or 0x70 pages)
     * into the 256 x 256 work buffer (FRAME 0x138) efffocus reads. */
    void shrinkFocusFrame(i3dContext *ctx, u_int fbp);                   // ROR 0x00140fdc
    /* A layer that lays the work buffer over the frame at fbp as a sprite
     * filling the view at distance dist (z-tested: only what lies
     * further is covered), alpha m_focus.m_val[0]. */
    void drawFocusPlane(float dist, i3dContext *ctx, u_int fbp);         // ROR 0x00141184
    /* Runs the track's lines once their frame comes. */
    void runFocusTrack();                                                // ROR 0x0014186c
    /* One track line; 0 = stop for now (a later frame or term), 1 = go on. */
    int  execFocusCommand(char *line);                                   // ROR 0x001419d8
    /* mag/magscenefocus.cpp: mode 0 releases the efffocus texture, any
     * other adds it (once) and becomes the focus mode (+0x853bc). */
    void setFocus(int mode);                                             // ROR 0x00140b20
    /* With data: setFocus(3) and the scene's focus track (an event pack's
     * file 2) of size bytes, from its start. */
    void setFocusData(const void *data, int size);                       // ROR 0x001417e4
    /* rec runs from -> to over frames; returns the value to show now
     * (from, or to at once when frames is 0). this is not read. */
    float startLerp(float from, float to, MagLerp *rec, int frames);     // ROR 0x00140e60
    /* Focus value which (1, 2) runs to to over frames (from < 0: from
     * where it is). */
    void setFocusLerp(int which, float from, float to, int frames);      // ROR 0x00140d94
    /* ---- mag/magscenemenu.cpp: GamMan glue (g_gamMan and g_sequence.m_bits / m_global, read via bss 0x7b2060 / 0x7b205c) ---- */
    void gamEnterMenu(int menu);                                         // ROR 0x0013f07c
    void gamLeaveMenu();                                                 // ROR 0x0013f09c
    int  canOpenItemMenu();                                              // ROR 0x0013f0c0
    int  canOpenMenu10001();                                             // ROR 0x0013f114
    void setGamWord54();                                                 // ROR 0x0013f130
    int  getMenuFadeOutTime();                                           // ROR 0x0013f144
    int  getMenuFadeInTime();                                            // ROR 0x0013f194
    void clearGamFlag25();                                               // ROR 0x0013f1e4
    /* Opens message msg with 16 argument words (copied; args may be
     * NULL); returns the window handle. */
    int  openSysMsgWindow(int msg, const int *args, int fillMsb);        // ROR 0x0013f280
    void closeSysMsgWindow(int win);                                     // ROR 0x0013f458
    /* Sub mode 5 and the 0x1000a menu entered and left at once, the sub
     * mode popped again; returns 0. */
    int  pulseMenu1000a();                                               // ROR 0x0013f210
    /* m_window[win] is in use. */
    int  isWindowOpen(int win);                                          // ROR 0x0013f444

    /* ---- mag/magscenerender.cpp ---- */
    void releaseTexture(int no);                                         // ROR 0x00139540
    void copyTexture(const char *dst, const char *src);                  // ROR 0x00139568
    /* Re-sends the double buffer's draw environment (frame & 1 picks it). */
    static void putDrawEnv(I3dLayer *layer, sceGsDBuff *dbuff, int frame); // ROR 0x00139590
    u_int getDrawFlags();                                                // ROR 0x0013963c
    void drawCloseUp(i3dContext *ctx, DogActor *actor, DogEntity *src);  // ROR 0x00139664
    void drawTenkyuRotated(i3dContext *ctx, I3dLight *light, const float *trans,
                           const float *rot);                            // ROR 0x00139920
    void makeActorShadow(int theCount, i3dContext *ctx, DogActor *actor,
                         I3rStqMapEntity *stq);                          // ROR 0x00139b4c
    void drawFocusActor(i3dContext *ctx, sceGsDBuff *dbuff, int frame);  // ROR 0x0013a464
    void drawBlobShadows(I3dLayer *layer);                               // ROR 0x0013a57c
    void drawShadows(i3dContext *ctx, sceGsDBuff *dbuff, int frame);     // ROR 0x0013a924
    void drawActorShadow(int theChara, int theCount, i3dContext *ctx, sceGsDBuff *dbuff,
                         int frame, I3dView *view);                      // ROR 0x0013ab30
    void drawStrips(I3dLayer *layer);                                    // ROR 0x0013ad24
    void drawTenkyu(I3dLayer *layer);                                    // ROR 0x0013b20c
    int  collectActors(int *parented, MagSceneDrawEntry *theArray, int theMax,
                       int flag3);                                       // ROR 0x0013b2ac
    void sortActors(MagSceneDrawEntry *theArray, int theMax);            // ROR 0x0013b580
    void drawActors(MagSceneDrawEntry *theArray, int theMaxArray, I3dLayer **theRenderStage,
                    int theMaxStage, int flag3);                         // ROR 0x0013b650
    void drawActorMirror(DogActor *actor, I3dLayer *stage);              // ROR 0x0013b870
    /* ---- elsewhere ---- */
    /* The distance along dir from pos to the sky slots' collision models
     * (tenkyu < 0: all four), < 0 when nothing is hit. */
    float castRay(MagSceneRayHit *hit, const float *pos, const float *dir, int tenkyu); // ROR 0x00133454
    /* The ray pos + t dir (dir level: asserted y == 0) against the wall
     * segments in m_vec_pair: t of the first hit (hit->m_vec[1] its
     * normal), -1 when none. */
    float castWalls(MagSceneRayHit *hit, const float *pos, const float *dir); // ROR 0x00133a0c
    /* A sphere (r > 0) at pos against the walls, then sky slot 0's
     * collision model: 1 (walls) / the model's result, -1 when nothing. */
    float castSphere(float r, MagSceneRayHit *hit, const float *pos);    // ROR 0x00133658
    /* The height under pos along dir (castRay on sky slot 0), 100000 when
     * the ray does not hit within its length. Nothing calls it. */
    float getGroundHeight(const float *pos, const float *dir);          // ROR 0x0013377c
    /* A vertical cylinder at pos (radius r, from pos.y down to pos.y + h,
     * h < 0) against the walls: 1 on a hit (hit->m_vec[1] the wall
     * normal), else -1. */
    float hitWallsCylinder(float h, float r, MagSceneRayHit *hit, const float *pos); // ROR 0x0013386c
    /* A sphere against the walls whose height range covers pos.y. */
    float hitWallsSphere(float r, MagSceneRayHit *hit, const float *pos); // ROR 0x00133b80
    /* The cylinder pos .. pos + dir.y (radius r) against the walls, then the
     * sky slots' collision models (tenkyu < 0: all four). */
    float castCylinder(float r, const float *pos, const float *dir, int tenkyu,
                       MagSceneRayHit *hit);                             // ROR 0x00133d10
    /* The wall segments (MAX_PRIVATE_COLLI of them, m_vec_pair): a free one
     * (equal end heights) takes p1 -> p2; returns its id, -1 when full. */
    int  addPrivateColli(const float *theP1, const float *theP2);        // ROR 0x00135e20
    void removePrivateColli(int theID);                                  // ROR 0x00135f44
    /* drawStrips' records: each adder takes the first free one and returns
     * its index (-1 when all 0x2000 are used); colours as RGBA bytes
     * (r, g, b / 255, a / 128). Only the type 0..2 adders have callers. */
    int  addStrip(int count, void *pos, const void *colors, void *data2, int life); // ROR 0x00133e94
    int  addStripCtx(int count, void *pos, const void *colors, void *data2); // ROR 0x00133efc
    int  addLineStrip(int count, void *pos, u_int rgba, void *data2);    // ROR 0x00133f68
    int  addWireBox(void *data, void *data2, u_int rgba);                // ROR 0x00134114
    int  addStripType4(void *data, u_int rgba);                          // ROR 0x001342bc
    int  addWireSphere(float r, void *pos);                              // ROR 0x00134460
    int  setStripColor(int no, u_int rgba);                              // ROR 0x001344c4
    int  addStripRecord(const MagSceneStrip *rec);                       // ROR 0x00134624
    /* Window text: printWindow draws text in colour at the window's cursor;
     * the font-buffer helpers re-upload the window's texture
     * (i3dTexRelease) where the ROM does. */
    void printWindow(int winNo, const char *text, int color);            // ROR 0x00135364
    void setWindowFill(int winNo, u_int fill);                           // ROR 0x00135418
    void setWindowFillMsb(int winNo, int on);                            // ROR 0x00135494
    void setWindowColor(int winNo, u_int color);                         // ROR 0x00135520
    void releaseWindowTex(int winNo);                                    // ROR 0x00135628
    void scrollWindow(int theN, int rows);                               // ROR 0x0013569c
    void resetWindowFont(int winNo);                                     // ROR 0x00135740
    /* The keyhole fade with an EVNTEX.RPK texture: loads it, resets the
     * fade key to state 5 and puts it on the life manager. */
    void openFadeKey(const char *theTexName);                            // ROR 0x00135cac
    void setFadeKeyMagnify(float theMagnify, int time);                  // ROR 0x00135d58
    void clearStrip(int no);                                             // ROR 0x00134698
    /* The skies at layer 490: ambient-only white light, the first view
     * pushed out to 100000 at depth 0, centred under the camera in x / z. */
    void drawSkyDome(i3dContext *ctx);                                   // ROR 0x001406b8
    void drawFocus(i3dContext *ctx, int frame);                          // ROR 0x00140e88

    MagSceneVecPair m_vec_pair[256];    // ROR 0x0000: see MagSceneVecPair
    MagShadowLight m_shadow_lightrec;   // ROR 0x2000: the shadow layers' light record
    int            m_shadow_mode;       // ROR 0x2040: setShadowLight's mode: < 0 no projected shadows (the constructor's value), 1 directional, 2 point, 3 spot (2 and 3: the actor's nearest point light)
    const float   *m_shadow_src;        // ROR 0x2044: mode 1: the pool's directional light (its direction first); else that kind's light or NULL
    int            m_2048;              // ROR 0x2048: cleared with the shadow light by the constructor; role unknown
    int            m_204c;              // ROR 0x204c: idem
    DogLightPool   m_light_pool;        // ROR 0x2050: the render stages' light sets
    MagCharaLoader m_chara_loader;      // ROR 0x215c: data/ch/CHARA.RPK
    MagMapLoader   m_map_loader;        // ROR 0x224c: data/bg/mapunit.bin
    int            m_229c;              // ROR 0x229c: initRender zeroes it; role unknown
    float          m_22a0;              // ROR 0x22a0: scales the shadow light's intensity (with m_2a9cc); role unverified
    float          m_22a4;              // ROR 0x22a4: scales the second view's depth range
    float          m_22a8;              // ROR 0x22a8: the ambient scale of render stages 1 and 2 (others 1.0); role unverified
    int            m_pc_mode;           // ROR 0x22ac: setPC's mode
    float          m_22b0;              // ROR 0x22b0: blends m_22a4 toward 1 for the shadow view (1 = m_22a4 as is)
    int            m_22b4;              // ROR 0x22b4: sky path display: 0 off, 1 copy the points then 2, 2 draw the copy (the constructor's value: 1)
    int            m_22b8;              // ROR 0x22b8: the constructor clears it; role unknown
    int            m_22bc[4];           // ROR 0x22bc: the constructor sets -1; role unknown
    int            m_sepia;             // ROR 0x22cc: sepia on (Brown op sepo); textures and lights load sepia by it; MagSystem::clearScene22cc zeroes it
    BedSubMode     m_sub_mode;          // ROR 0x22d0: update's mode (pushSubMode sets 8; getDrawFlags tests 7 and 8); m_step -1 on entry; m_arg the requested menu id, m_arg2 the PC's mode / number then
    MagLifeMan     m_life_man;          // ROR 0x2330
    MagLifeWindow *m_window[4];         // ROR 0x2734: MAX_WINDOW text windows (openWindow), NULL = free
    GamFont        m_window_font[4];    // ROR 0x2744: stride 0x6c, each window's text buffer (start points it at bit8a..bit8d)
    int            m_font_tex_no[4];    // ROR 0x28f4: bit8a..bit8d in m_tex_info
    int            _2904;
    void          *m_font_data;         // ROR 0x2908: the language's font file from PREMIERE.RPK
    int            m_common_tex_no[2];  // ROR 0x290c: r_shadow, noise in m_tex_info (initRender hands noise to +0x418f0)
    void          *m_premiere;          // ROR 0x2914: PREMIERE.RPK's file 0 (the common textures)
    DogActor       m_actors[128];       // ROR 0x2920: stride 0x4d0
    unsigned       m_actor_flags[128];  // ROR 0x29120: one word per actor; 0x60 = bound / chapter model
    int           *m_actor_snd[128];    // ROR 0x29320: per actor, its 12 footstep sound ids by sector sound slot (asserted m_actor_snd[theNo]); NULL = none
    DogTexInfo     m_tex_info;          // ROR 0x29520: 256 textures, shadow01 first
    enum { MAX_MAP_MODEL = 4 };         /* the assert text's name for the slot count */
    MagSceneTenkyu m_tenkyu[MAX_MAP_MODEL]; // ROR 0x29540: stride 0x520
    int            m_2a9c0;             // ROR 0x2a9c0: the constructor clears it; role unknown
    int            m_focus_actor;       // ROR 0x2a9c4: the actor drawFocusActor shows close up, -1 = none (then drawShadows)
    u_int          m_2a9c8;             // ROR 0x2a9c8: bit 1 = draw the blob shadows; other bits unknown
    float          m_2a9cc;             // ROR 0x2a9cc: mapparam.bbb word 11 / 100, kept in 0..1 (1 when 0); role unknown
    I3dLightDesc   m_shadow_light;      // ROR 0x2a9d0: the light of the shadow layers
    MagViewParam   m_view[2];           // ROR 0x2aa10
    float          m_2aa80;             // ROR 0x2aa80: the camera angle the frustum was built for (-1 after initRender: rebuild)
    int            m_2aa84;             // ROR 0x2aa84: load sets 1 with a map's fog; role unknown
    I3rStqMapEntity m_stq_map;          // ROR 0x2aa90: the sky's second model's STQ map (0x50; init sets +0x40 = 1, id 3)
    I3dFog         m_fog[4];            // ROR 0x2aae0: init clears [0] and copies it to the others;
                                        //   [0] the fog drawn, [2] / [3] setFog's fade from / to
    int            m_fog_frame;         // ROR 0x2ab60: setFog's fade, frames done
    int            m_fog_frames;        // ROR 0x2ab64: setFog's fade length (0: none)
    DogCamera      m_camera_entity;     // ROR 0x2ab70: the camera (m_trans +0x2abb0 and m_rotate +0x2abc0 are the eye actors are drawn for)
    /* setCameraMode's modes: 0 free, 1 free with a target actor, 2 behind
     * the actor (its child), 3..5 a map camera path looking at the actor
     * (3 sky slot 0's first path, 4 a path by number, 5 the same placed
     * by param); changeCamera adds 0x10 while a move to the path's goal is
     * pending (updateCamera). */
    int            m_camera_mode;       // ROR 0x2ad50: getCameraMode returns it
    int            m_cam_move_frame;    // ROR 0x2ad54: updateCamera's frame of the 30-frame move to a path's goal; changeCamera clears it with each new goal
    int            m_cam_actor_no;      // ROR 0x2ad58: the actor the camera follows or looks at (-1 none); getCameraMode's second word out
    float          m_cam_dist_max;      // ROR 0x2ad5c: a path camera's farthest distance to the actor (600 from the constructor; setCameraMode param[1])
    float          m_cam_dist_min;      // ROR 0x2ad60: its nearest (500; param[0])
    int            m_cam_path_no;       // ROR 0x2ad64: the path in m_tenkyu[m_cam_map_no].m_path_data (modes 3..5)
    int            m_cam_map_no;        // ROR 0x2ad68: the sky slot of that path
    int            _2ad6c[5];
    int            m_cam_save_count;    // ROR 0x2ad80: saveCamera counts up, restoreCamera down
    int            m_saved_mode;        // ROR 0x2ad84: saveCamera's copies of +0x2ad50, +0x2ad58..+0x2ad68
    int            m_saved_actor_no;    // ROR 0x2ad88
    float          m_saved_dist_max;    // ROR 0x2ad8c
    float          m_saved_dist_min;    // ROR 0x2ad90
    int            m_saved_path_no;     // ROR 0x2ad94
    int            m_saved_map_no;      // ROR 0x2ad98
    int            m_camera_target_set; // ROR 0x2ad9c: setCameraTarget sets 1
    sceVu0FVECTOR  m_camera_pos;        // ROR 0x2ada0: setCameraTarget's position
    sceVu0FVECTOR  m_camera_at;         // ROR 0x2adb0: setCameraTarget's look-at point
    MagLifePC      m_pc;                // ROR 0x2adc0
    MagLifeBwn     m_bwn;               // ROR 0x2afe0
    MagLifeBwnCtrl m_bwn_ctrl;          // ROR 0x2bd20
    int            m_menu_id;           // ROR 0x2be50: the open menu's id (0x10000..), -1 = none
    int            m_menu_arg;          // ROR 0x2be54: requestMenu's argument
    int            m_menu_result;       // ROR 0x2be58: the menu's last processPad result; 0 = still running
    MagLifeMenuBase *m_pMenuBase;       // ROR 0x2be5c: the open menu (the asserts name it), NULL = none
    /* The embedded menus (openMenu's ids; mag/maglifemenu.h). */
    MagLifeMenuItem    m_menu_item;     // ROR 0x2be60: id 0x10000
    MagLifeMenuMap     m_menu_map;      // ROR 0x2e5b0: 0x10001
    MagLifeMenuStock   m_menu_stock;    // ROR 0x2eed0: 0x10002
    MagLifeMenuSave    m_menu_save;     // ROR 0x304c0: 0x10003
    MagLifeMenuLoad    m_menu_load;     // ROR 0x32c80: 0x10004
    MagLifeMenuPBook   m_menu_pbook;    // ROR 0x35440: ids with bit 0x100000 / 0x200000
    MagLifeMenuTitle   m_menu_title;    // ROR 0x36640: 0x10005
    MagLifeMenuOpt     m_menu_opt;      // ROR 0x3bd80: 0x10006
    MagLifeMenuMovie   m_menu_movie;    // ROR 0x3ceb0: 0x10007
    MagLifeMenuMcd     m_menu_mcd;      // ROR 0x3d880: 0x10008
    MagLifeMenuNow     m_menu_now;      // ROR 0x3ecc0: 0x10009
    MagLifeMenuBoard2D m_menu_board2d;  // ROR 0x3f620: ids with bit 0x40000
    MagLifeMenuBoard3D m_menu_board3d;  // ROR 0x40720: ids with bit 0x80000
    MagLifeMenuSelLang m_menu_sellang;  // ROR 0x40780: 0x10010
    MagLifeFader   m_fader;             // ROR 0x41850: fade kinds 0 / 4
    MagLifeFadeKey m_fade_key;          // ROR 0x41890: fade kinds 1..3
    MagLife130da4  m_life_418f0;        // ROR 0x418f0
    S2dCineSco     m_cine_sco;          // ROR 0x41930
    S2dBg          m_bg;                // ROR 0x42260
    S2dMes         m_mes;               // ROR 0x42bb0
    MagFadeParam   m_fade;              // ROR 0x43700: the current fade (m_kind -1 = none; setFade)
    int            m_sys_msg_window;    // ROR 0x43718: the system message's window (asserted >= 0 in sub mode 5)
    MagSceneStrip  m_strip[0x2000];     // ROR 0x437e0: drawStrips' records
    EffMan         m_eff_man;           // ROR 0x837e0: the effects (eff/effman.h)
    S2dMovie       m_movie;             // ROR 0x84aa8
    int            m_bwn_no;            // ROR 0x853b4: setBwn's actor number, -1 = none
    int            m_pc_no;             // ROR 0x853b8: setPC's actor number, -1 = none
    MagSceneFocus  m_focus;             // ROR 0x853bc: the focus effect (mag/magscenefocus.cpp)
    SctWorld      m_8542c;             // ROR 0x8542c: reset by releaseTenkyu, sector.sec by load
    int            m_mirror_axis;       // ROR 0x85494: mapparam.bbb word 9 (files over 0x20 bytes): 0 none, 1 z, 2 y (floor), 3 x
    float          m_mirror_plane;      // ROR 0x85498: mapparam.bbb word 10 (a float): the mirror plane's coordinate
    int            m_854a4;             // ROR 0x854a4: mapparam.bbb word 8: the map's default footstep sound slot
    sceVu0FVECTOR  m_854b0;             // ROR 0x854b0: (0.1, 0.1, 0.1, 1) from the constructor (a colour?); role unknown
    int            m_854c0[5];          // ROR 0x854c0: the constructor clears them; roles unknown
    u_char         m_854d4;             // ROR 0x854d4: the constructor sets 1; role unknown
    u_char         m_854d5;             // ROR 0x854d5: idem
    MagScene      *m_854e0;             // ROR 0x854e0: start points it at the scene (an interface pointer; class unknown)
    MagScene      *m_854e8;             // ROR 0x854e8: idem
};
static_assert(offsetof(MagScene, m_actors) > offsetof(MagScene, m_life_man), "EE order");
static_assert(offsetof(MagScene, m_light_pool) > offsetof(MagScene, m_shadow_src), "EE order");
static_assert(offsetof(MagScene, m_pc) > offsetof(MagScene, m_camera_at), "EE order");
static_assert(offsetof(MagScene, m_actors) > offsetof(MagScene, m_premiere), "EE order");
static_assert(offsetof(MagScene, m_view) > offsetof(MagScene, m_2a9cc), "EE order");
static_assert(offsetof(MagScene, m_window) > offsetof(MagScene, m_life_man), "EE order");
static_assert(offsetof(MagScene, m_pMenuBase) > offsetof(MagScene, m_bwn_ctrl), "EE order");
static_assert(offsetof(MagScene, m_fader) > offsetof(MagScene, m_menu_sellang), "EE order");
static_assert(offsetof(MagScene, m_fade) > offsetof(MagScene, m_mes), "EE order");
static_assert(offsetof(MagScene, m_shadow_lightrec) >= 0x2000, "m_vec_pair comes first");
static_assert(offsetof(MagScene, m_854d5) > offsetof(MagScene, m_854a4), "EE order");

#endif /* _MAG_MAGSCENE_H */
