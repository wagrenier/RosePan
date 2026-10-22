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
#include "dog/dogactor.h"
#include "dog/dogtexinfo.h"
#include "lib/unassigned.h"
#include "mag/magloader.h"
#include "mag/maglifebwn.h"
#include "mag/maglifebwnctrl.h"
#include "mag/maglifeman.h"
#include "mag/maglifepc.h"
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
    MagSceneCRec **m_c_rec;     // ROR 0x4fc: its records (the ROM turns the file's offsets into pointers in place)
    void    *m_light_data;      // ROR 0x500: <name>_l.bin, NULL when the pack has none
    void    *m_tex_list;        // ROR 0x504: i3r texture list (releaseTenkyuTex)
    void    *m_data;            // ROR 0x508: freed by releaseTenkyuTex
    u_char   _50c[0x520 - 0x50c];
};
static_assert(offsetof(MagSceneTenkyu, m_model) >= 0x10, "m_model");
/* No bound on m_id: the host DogModel is smaller than the EE's 0x1a0. */
static_assert(offsetof(MagSceneTenkyu, m_id) > offsetof(MagSceneTenkyu, m_model), "m_id");

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

/* EE 0x85500. layout: host -- holds the actors and controllers (pointers,
 * vptrs); EE offsets in the comments. */
struct MagScene
{
    MagScene();                                                          // ROR 0x001360a0
    ~MagScene();                                                         // ROR 0x00136d14

    void init();                                                         // ROR 0x00131840
    void setPC(int no, int mode);                                        // ROR 0x001316b8
    void setBwn(int no, int arg);                                        // ROR 0x00131790
    void start();                                                        // ROR 0x00131a28
    int  initRender(i3dContext *ctx);                                    // ROR 0x00131b90
    void update(BedPad *pad0, BedPad *pad1);                             // ROR 0x001327dc
    void pushSubMode();         /* BedSubMode::push on +0x22d0, then mode 8 */ // ROR 0x00132744
    void popSubMode();                                                   // ROR 0x00132784
    /* The text windows (MAX_WINDOW 4, slots at +0x2734; the "view" methods
     * take the same numbers). openWindow returns the slot. */
    int  openWindow(int w, int h, int a3, int life);                     // ROR 0x00134730
    void closeWindow(int no);                                            // ROR 0x001348b0
    void setViewParam(int no, const int *v);                             // ROR 0x00134944
    void fadeInWindow(int no, int time, int from, int to);               // ROR 0x00135088
    void fadeOutWindow(int no, int time, int from, int to);              // ROR 0x0013511c
    void setWindowShow(int no, int on);                                  // ROR 0x001351b0
    int  getWindowTextWidth(int no, const char *text);                   // ROR 0x0013555c
    void updateWindowFont(int no);                                       // ROR 0x00135594
    void drawView(int no, int a, int b, const void *p, int c);           // ROR 0x00135234
    void setViewFar(float far);                                          // ROR 0x00131618
    void resetEntities();                                                // ROR 0x00135fa4
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

    MagCharaLoader m_chara_loader;      // ROR 0x215c: data/ch/CHARA.RPK
    MagMapLoader   m_map_loader;        // ROR 0x224c: data/bg/mapunit.bin
    int            m_229c;              // ROR 0x229c: initRender zeroes it; role unknown
    float          m_22a4;              // ROR 0x22a4: scales the second view's depth range
    int            m_pc_mode;           // ROR 0x22ac: setPC's mode
    int            m_22cc;              // ROR 0x22cc: MagSystem::clearScene22cc zeroes it; role unknown
    int            m_22d0;              // ROR 0x22d0: 1 while a PC is bound (setPC), 0 after initRender
    int            m_22d4;              // ROR 0x22d4: -1 after setPC / initRender; role unknown
    MagLifeMan     m_life_man;          // ROR 0x2330
    int            m_font_tex_no[4];    // ROR 0x28f4: bit8a..bit8d in m_tex_info
    int            _2904;
    void          *m_font_data;         // ROR 0x2908: the language's font file from PREMIERE.RPK
    int            m_common_tex_no[2];  // ROR 0x290c: r_shadow, noise in m_tex_info (initRender hands noise to +0x418f0)
    void          *m_premiere;          // ROR 0x2914: PREMIERE.RPK's file 0 (the common textures)
    DogActor       m_actors[128];       // ROR 0x2920: stride 0x4d0
    unsigned       m_actor_flags[128];  // ROR 0x29120: one word per actor; 0x60 = bound / chapter model
    unsigned       m_actor_flags2[128]; // ROR 0x29320: a second word per actor, cleared with the first
    DogTexInfo     m_tex_info;          // ROR 0x29520: 256 textures, shadow01 first
    MagSceneTenkyu m_tenkyu[4];         // ROR 0x29540: stride 0x520
    float          m_2a9cc;             // ROR 0x2a9cc: mapparam.bbb word 11 / 100, kept in 0..1 (1 when 0); role unknown
    MagViewParam   m_view[2];           // ROR 0x2aa10
    float          m_2aa80;             // ROR 0x2aa80: -1 after initRender; role unknown
    int            m_2aa84;             // ROR 0x2aa84: load sets 1 with a map's fog; role unknown
    I3rStqMapEntity m_stq_map;          // ROR 0x2aa90: the sky's second model's STQ map
    u_char         m_2aad0;             // ROR 0x2aad0: init sets 1; role unknown
    u_char         m_2aad1;             // ROR 0x2aad1: init sets 3; role unknown
    I3dFog         m_fog[4];            // ROR 0x2aae0: init clears [0] and copies it to the others
    int            m_fog_2ab60;         // ROR 0x2ab60: init zeroes it; role unknown
    int            m_fog_2ab64;         // ROR 0x2ab64: init zeroes it; role unknown
    DogEntity      m_camera_entity;     // ROR 0x2ab70: the camera's transform node (its class, likely DogCamera, is bigger: getMatrix reads +0xf0)
    int            m_camera_mode;       // ROR 0x2ad50: getCameraMode returns it
    int            m_camera_2ad58;      // ROR 0x2ad58: getCameraMode's second word out
    int            m_camera_target_set; // ROR 0x2ad9c: setCameraTarget sets 1
    sceVu0FVECTOR  m_camera_pos;        // ROR 0x2ada0: setCameraTarget's position
    sceVu0FVECTOR  m_camera_at;         // ROR 0x2adb0: setCameraTarget's look-at point
    MagLifePC      m_pc;                // ROR 0x2adc0
    MagLifeBwn     m_bwn;               // ROR 0x2afe0
    MagLifeBwnCtrl m_bwn_ctrl;          // ROR 0x2bd20
    MagLife12f008  m_life_41850;        // ROR 0x41850
    MagLife130da4  m_life_418f0;        // ROR 0x418f0
    S2dCineSco     m_cine_sco;          // ROR 0x41930
    S2dBg          m_bg;                // ROR 0x42260
    S2dMes         m_mes;               // ROR 0x42bb0
    int            m_43700;             // ROR 0x43700: -1 after initRender; role unknown
    Obj176a3c      m_837e0;             // ROR 0x837e0
    S2dMovie       m_movie;             // ROR 0x84aa8
    int            m_bwn_no;            // ROR 0x853b4: setBwn's actor number, -1 = none
    int            m_pc_no;             // ROR 0x853b8: setPC's actor number, -1 = none
    Obj17c0f0      m_8542c;             // ROR 0x8542c: reset by releaseTenkyu, sector.sec by load
    int            m_85494;             // ROR 0x85494: mapparam.bbb word 9 (files over 0x20 bytes); role unknown
    int            m_85498;             // ROR 0x85498: mapparam.bbb word 10; role unknown
    int            m_854a4;             // ROR 0x854a4: mapparam.bbb word 8; role unknown
};
static_assert(offsetof(MagScene, m_actors) > offsetof(MagScene, m_life_man), "EE order");
static_assert(offsetof(MagScene, m_pc) > offsetof(MagScene, m_camera_at), "EE order");
static_assert(offsetof(MagScene, m_actors) > offsetof(MagScene, m_premiere), "EE order");
static_assert(offsetof(MagScene, m_view) > offsetof(MagScene, m_2a9cc), "EE order");

#endif /* _MAG_MAGSCENE_H */
