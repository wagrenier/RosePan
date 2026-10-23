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
#include "mag/maglifeman.h"
#include "mag/maglifemenu.h"
#include "mag/maglifepc.h"
#include "mag/maglifewindow.h"
#include "sdk/sce_gs.h"
#include "spr/s2d.h"

/* A record of a sky pack's <name>_c.bin (MagScene::load initialises the
 * words below; the rest is file data). EE size at least 0x30; roles
 * unverified. */
struct MagSceneCRec
{
    u_char m_00;                // ROR 0x00: load copies m_00 >> 1 into m_20
    u_char m_01;                // ROR 0x01
    u_char m_02;                // ROR 0x02: load stores the file's head word >> 16
    u_char m_03;                // ROR 0x03: load clears it
    u_char _04[0x20 - 0x04];
    int    m_20;                // ROR 0x20
    int    m_24;                // ROR 0x24: load clears it
    int    m_28;                // ROR 0x28: load clears it
    int    m_2c;                // ROR 0x2c: load clears it
};
static_assert(offsetof(MagSceneCRec, m_20) == 0x20, "m_20");
static_assert(offsetof(MagSceneCRec, m_2c) == 0x2c, "m_2c");

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
    void    *m_tex_list;        // ROR 0x504: i3r texture list (releaseTenkyuTex)
    void    *m_data;            // ROR 0x508: freed by releaseTenkyuTex
    u_char   _50c[0x520 - 0x50c];
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
    void commandPC(int cmd, const float *arg);                           // ROR 0x0013166c
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
    int  getActorMotionLength(int no, int motion);                       // ROR 0x001333e4
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
    void setCameraMode(int mode);                                        // ROR 0x0013caf0
    int  getCameraMode(int *out);                                        // ROR 0x0013d5ac
    void setLightNo(int theNo);                                          // ROR 0x0013f4d0
    /* Only asserts theNo < 8 (the destructor passes -1). */
    void checkLightNo(int theNo);                                        // ROR 0x0013f49c
    void loadLight(const void *theData);                                 // ROR 0x0013f658
    void setDefaultLight();                                              // ROR 0x0013f8e4
    void setLightFlags(const int *flags, int no);                        // ROR 0x0013f528
    void setLightParam(int type, int idx, int which, const float *v, int no); // ROR 0x0013f980
    void getLightParam(int type, int idx, int which, float *v, int no);  // ROR 0x0013fa50
    int  addLight(int type, unsigned int mask);                          // ROR 0x0013fe64
    void releaseTenkyuTex(int no);                                       // ROR 0x0013ff00
    void loadTenkyuTex(int no, const BedPfh *pfh, i3dContext *ctx);      // ROR 0x00140030
    /* Fades to fog over frames (0: at once). */
    void setFog(const I3dFog *fog, int frames);                          // ROR 0x00143d7c
    void updateFog();                                                    // ROR 0x00143fd0
    void updateDebug(BedPad *pad0, BedPad *pad1);                        // ROR 0x0014313c
    void updateCamera();                                                 // ROR 0x0013e548
    void changeCamera();                                                 // ROR 0x0013d5e4
    void rotateTenkyu();                                                 // ROR 0x0014062c
    void updateRecords853fc();                                           // ROR 0x00140c68
    /* The constructor's: clears the records at +0x853bc and sets their
     * defaults. */
    void initRecords853bc();                                             // ROR 0x00140a84
    /* GamMan glue (g_gamMan and the words at *(0x7b2060) / *(0x7b205c)). */
    void gamEnterMenu();                                                 // ROR 0x0013f07c
    void gamLeaveMenu();                                                 // ROR 0x0013f09c
    int  canOpenItemMenu();                                              // ROR 0x0013f0c0
    int  canOpenMenu10001();                                             // ROR 0x0013f114
    void setGamWord54();                                                 // ROR 0x0013f130
    int  getMenuFadeOutTime();                                           // ROR 0x0013f144
    int  getMenuFadeInTime();                                            // ROR 0x0013f194
    void clearGamFlag25();                                               // ROR 0x0013f1e4
    void closeSysMsgWindow(int win);                                     // ROR 0x0013f458

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
    void clearStrip(int no);                                             // ROR 0x00134698
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
    int            m_22cc;              // ROR 0x22cc: MagSystem::clearScene22cc zeroes it; role unknown
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
    MagSceneTenkyu m_tenkyu[4];         // ROR 0x29540: stride 0x520
    int            m_2a9c0;             // ROR 0x2a9c0: the constructor clears it; role unknown
    int            m_focus_actor;       // ROR 0x2a9c4: the actor drawFocusActor shows close up, -1 = none (then drawShadows)
    u_int          m_2a9c8;             // ROR 0x2a9c8: bit 1 = draw the blob shadows; other bits unknown
    float          m_2a9cc;             // ROR 0x2a9cc: mapparam.bbb word 11 / 100, kept in 0..1 (1 when 0); role unknown
    I3dLightDesc   m_shadow_light;      // ROR 0x2a9d0: the light of the shadow layers
    MagViewParam   m_view[2];           // ROR 0x2aa10
    float          m_2aa80;             // ROR 0x2aa80: the camera angle the frustum was built for (-1 after initRender: rebuild)
    int            m_2aa84;             // ROR 0x2aa84: load sets 1 with a map's fog; role unknown
    I3rStqMapEntity m_stq_map;          // ROR 0x2aa90: the sky's second model's STQ map
    u_char         m_2aad0;             // ROR 0x2aad0: init sets 1; role unknown
    u_char         m_2aad1;             // ROR 0x2aad1: init sets 3; role unknown
    I3dFog         m_fog[4];            // ROR 0x2aae0: init clears [0] and copies it to the others
    int            m_fog_2ab60;         // ROR 0x2ab60: init zeroes it; role unknown
    int            m_fog_2ab64;         // ROR 0x2ab64: init zeroes it; role unknown
    DogCamera      m_camera_entity;     // ROR 0x2ab70: the camera (m_trans +0x2abb0 and m_rotate +0x2abc0 are the eye actors are drawn for)
    int            m_camera_mode;       // ROR 0x2ad50: getCameraMode returns it
    int            m_camera_2ad58;      // ROR 0x2ad58: getCameraMode's second word out
    float          m_camera_2ad5c;      // ROR 0x2ad5c: 600 from the constructor; role unknown
    float          m_camera_2ad60;      // ROR 0x2ad60: 500 from the constructor; role unknown
    int            m_camera_2ad64;      // ROR 0x2ad64: the constructor clears it; role unknown
    int            m_camera_2ad68;      // ROR 0x2ad68: idem
    int            _2ad6c[5];
    int            m_camera_2ad80;      // ROR 0x2ad80: the constructor clears it; role unknown
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
    MagLifeFader   m_fader;             // ROR 0x41850: fade kinds 0 / 4
    MagLifeFadeKey m_fade_key;          // ROR 0x41890: fade kinds 1..3
    MagLife130da4  m_life_418f0;        // ROR 0x418f0
    S2dCineSco     m_cine_sco;          // ROR 0x41930
    S2dBg          m_bg;                // ROR 0x42260
    S2dMes         m_mes;               // ROR 0x42bb0
    MagFadeParam   m_fade;              // ROR 0x43700: the current fade (m_kind -1 = none; setFade)
    int            m_sys_msg_window;    // ROR 0x43718: the system message's window (asserted >= 0 in sub mode 5)
    MagSceneStrip  m_strip[0x2000];     // ROR 0x437e0: drawStrips' records
    Obj176a3c      m_837e0;             // ROR 0x837e0
    S2dMovie       m_movie;             // ROR 0x84aa8
    int            m_bwn_no;            // ROR 0x853b4: setBwn's actor number, -1 = none
    int            m_pc_no;             // ROR 0x853b8: setPC's actor number, -1 = none
    Obj17c0f0      m_8542c;             // ROR 0x8542c: reset by releaseTenkyu, sector.sec by load
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
static_assert(offsetof(MagScene, m_fade) > offsetof(MagScene, m_mes), "EE order");
static_assert(offsetof(MagScene, m_shadow_lightrec) >= 0x2000, "m_vec_pair comes first");
static_assert(offsetof(MagScene, m_854d5) > offsetof(MagScene, m_854a4), "EE order");

#endif /* _MAG_MAGSCENE_H */
