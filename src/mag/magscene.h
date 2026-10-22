/* ==========================================================================
 *  mag/magscene.h
 *
 *  The scene (mag/magscene*.cpp), EE 0x85500: MagSystem holds it as its
 *  member at +0x12540 (bss 0x6fbd80) up to its own words at +0x97a40. It
 *  owns 128 actors of 0x4d0 bytes at +0x2920 (DogActor-like: their vptr is
 *  at +0x94, as DogEntity's), one flag word per actor at +0x29120, the
 *  player's MagLifePC at +0x2adc0 and Brown's MagLifeBwn at +0x2afe0, the
 *  light set at +0x2050, the camera (+0x2ad50..) and four view objects
 *  (+0x2734). Minimal: what MagSystem calls and reads; the scene TUs fill
 *  in the class.
 * ======================================================================== */
#ifndef _MAG_MAGSCENE_H
#define _MAG_MAGSCENE_H

#include "bed/bedpad.h"
#include "lib/unassigned.h"
#include "mag/maglifebwn.h"
#include "mag/maglifepc.h"
#include "sdk/sce_gs.h"

/* EE 0x85500. layout: host -- holds the actors and controllers (pointers,
 * vptrs); only what MagSystem reads is placed, EE offsets in the comments. */
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
    void setLightFlags(const int *flags, int no);                        // ROR 0x0013f528
    void setLightParam(int type, int idx, int which, const float *v, int no); // ROR 0x0013f980
    void getLightParam(int type, int idx, int which, float *v, int no);  // ROR 0x0013fa50
    int  addLight(int type, unsigned int mask);                          // ROR 0x0013fe64

    int        m_22cc;               // ROR 0x22cc: MagSystem::clearScene22cc zeroes it; role unknown
    unsigned   m_actor_flags[128];   // ROR 0x29120: one word per actor (+0x2920 + no * 0x4d0); 0x60 = bound / chapter model
    MagLifePC  m_pc;                 // ROR 0x2adc0
    MagLifeBwn m_bwn;                // ROR 0x2afe0
};

#endif /* _MAG_MAGSCENE_H */
