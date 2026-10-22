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
#include "mag/maglifebwn.h"
#include "mag/maglifebwnctrl.h"
#include "mag/maglifeman.h"
#include "mag/maglifepc.h"
#include "sdk/sce_gs.h"

/* A sky ("tenkyu", celestial sphere) slot. EE 0x520. layout: host --
 * holds DogModels and pointers. */
struct MagSceneTenkyu
{
    void    *m_buf;             // ROR 0x000: game-heap buffer, freed by releaseTenkyu
    u_char   _004[0x010 - 0x004];
    DogModel m_model[3];        // ROR 0x010: stride 0x1a0
    int      m_id;              // ROR 0x4f0: -1 = a free slot
    int      m_4f4;             // ROR 0x4f4: 0 when free; role unknown
    int      m_4f8;             // ROR 0x4f8: 0 when free; role unknown
    int      m_4fc;             // ROR 0x4fc: 0 when free; role unknown
    int      m_500;             // ROR 0x500: 0 after release; role unknown
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
    void setViewParam(int no, const int *v);                             // ROR 0x00134944
    void drawView(int no, int a, int b, const void *p, int c);           // ROR 0x00135234
    void clearPC();                                                      // ROR 0x0013604c
    int  load(i3dContext *ctx, int a, int b, int c);                     // ROR 0x001377f0
    void releaseTenkyu(int no);                                          // ROR 0x001386d4
    void releaseTenkyuAll();                                             // ROR 0x00138804
    void loadModelByName(i3dContext *ctx, const char *name, int arg,
                         const char *rpk);                               // ROR 0x00138bc4
    int  loadModel(i3dContext *ctx, int id, const char *rpk);            // ROR 0x00138ed0
    void setActorAnim(int no, int a, int b);                             // ROR 0x001390d4
    int  loadActorAnim(int no, int arg, const char *name);               // ROR 0x0013910c
    void draw(i3dContext *ctx, sceGsDBuff *dbuff, int frame);            // ROR 0x0013bcc0
    void postDraw();                                                     // ROR 0x0013c29c
    void setCameraTarget(const float *pos, const float *at);             // ROR 0x0013c2ec
    void initCamera();                                                   // ROR 0x0013c928
    void setCameraMode(int mode);                                        // ROR 0x0013caf0
    int  getCameraMode(int *out);                                        // ROR 0x0013d5ac
    void setLightNo(int theNo);                                          // ROR 0x0013f4d0
    void setLightFlags(const int *flags, int no);                        // ROR 0x0013f528
    void setLightParam(int type, int idx, int which, const float *v, int no); // ROR 0x0013f980
    void getLightParam(int type, int idx, int which, float *v, int no);  // ROR 0x0013fa50
    int  addLight(int type, unsigned int mask);                          // ROR 0x0013fe64
    void releaseTenkyuTex(int no);                                       // ROR 0x0013ff00

    int            m_229c;              // ROR 0x229c: initRender zeroes it; role unknown
    float          m_22a4;              // ROR 0x22a4: scales the second view's depth range
    int            m_pc_mode;           // ROR 0x22ac: setPC's mode
    int            m_22cc;              // ROR 0x22cc: MagSystem::clearScene22cc zeroes it; role unknown
    int            m_22d0;              // ROR 0x22d0: 1 while a PC is bound (setPC), 0 after initRender
    int            m_22d4;              // ROR 0x22d4: -1 after setPC / initRender; role unknown
    MagLifeMan     m_life_man;          // ROR 0x2330
    int            m_2910;              // ROR 0x2910: copied into the +0x418f0 object by initRender
    DogActor       m_actors[128];       // ROR 0x2920: stride 0x4d0
    unsigned       m_actor_flags[128];  // ROR 0x29120: one word per actor; 0x60 = bound / chapter model
    DogTexInfo     m_tex_info;          // ROR 0x29520: 256 textures, shadow01 first
    MagSceneTenkyu m_tenkyu[4];         // ROR 0x29540: stride 0x520
    MagViewParam   m_view[2];           // ROR 0x2aa10
    float          m_2aa80;             // ROR 0x2aa80: -1 after initRender; role unknown
    u_char         m_2aad0;             // ROR 0x2aad0: init sets 1; role unknown
    u_char         m_2aad1;             // ROR 0x2aad1: init sets 3; role unknown
    I3dFog         m_fog[4];            // ROR 0x2aae0: init clears [0] and copies it to the others
    int            m_fog_2ab60;         // ROR 0x2ab60: init zeroes it; role unknown
    int            m_fog_2ab64;         // ROR 0x2ab64: init zeroes it; role unknown
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
    Obj19226c      m_84aa8;             // ROR 0x84aa8
    int            m_bwn_no;            // ROR 0x853b4: setBwn's actor number, -1 = none
    int            m_pc_no;             // ROR 0x853b8: setPC's actor number, -1 = none
    Obj17c0f0      m_8542c;             // ROR 0x8542c: reset by releaseTenkyu
};
static_assert(offsetof(MagScene, m_actors) > offsetof(MagScene, m_life_man), "EE order");
static_assert(offsetof(MagScene, m_pc) > offsetof(MagScene, m_camera_at), "EE order");

#endif /* _MAG_MAGSCENE_H */
