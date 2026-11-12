/* ==========================================================================
 *  spr/s2d.h
 *
 *  The scene's 2D layers (spr/s2d.cpp, 0x190a78..0x192ac0), each a
 *  SprMan with a phase machine (open -> start -> process each frame):
 *  - S2dCineSco (scene +0x41930): the cinemascope bars, two black sprites
 *    faded in / out together;
 *  - S2dBg (+0x42260): up to seven full-screen pictures (sgbg0N_tcc) from
 *    data/mnurpk/s2dgen.rpk, driven by the inline s2dBg* requests;
 *  - S2dMes (+0x42bb0): the message icons and buttons (14 sprites, key
 *    wait / write / select cursor icons) from data/mnurpk/s2dman.rpk and
 *    the map-name caption;
 *  - S2dMovie (+0x84aa8): no vtable; loading.tm2 and the RxMpeg movie
 *    layers.
 *  S2dCineSco / S2dBg / S2dMes are MagLife (vtables _vt$10S2dCineSco
 *  0x2e4550, _vt$5S2dBg 0x2e45a0, _vt$6S2dMes 0x2e45f0, vptr at +0x14):
 *  slot 1 the destructor, 2 live, 6 render(layer): they override MagLife's
 *  virtuals (mag/maglife.h), MagLifeMan runs them.
 * ======================================================================== */
#ifndef _SPR_S2D_H
#define _SPR_S2D_H

#include <stddef.h>

#include "mag/maglife.h"
#include "sdk/eetypes.h"
#include "spr/sprman.h"

struct DogTexInfo;
struct I3dLayer;
struct I3dLight;
struct I3dView;
struct i3dContext;
struct MagScene;
struct MagSystem;

/* EE 0x92c (the scene's S2dBg follows at +0x930). layout: host. */
struct S2dCineSco : MagLife
{
    S2dCineSco();                                                        // ROR 0x00190a78
    ~S2dCineSco();                                                       // ROR 0x00190ad4

    void init(DogTexInfo *tex);                                          // ROR 0x00190b70
    int  clear();                                                        // ROR 0x00190b60
    void term();                                                         // ROR 0x00190bbc
    /* EE slot 2. */
    void live() override;                                                // ROR 0x00190c04
    /* EE slot 6. */
    void render(I3dLayer *layer) override;                               // ROR 0x00190c20
    int  open();                                                         // ROR 0x00190c44
    int  close();                                                        // ROR 0x00190c68
    int  step();                                                         // ROR 0x00190c84
    void start();                                                        // ROR 0x00190d20
    int  process();                                                      // ROR 0x00190d70
    /* Fades the two bars in over time frames (0: at once). */
    int  fadeIn(int time);                                               // ROR 0x00190d90
    int  fadeOut(int time);                                              // ROR 0x00190e98

    int        m_open;          // ROR 0x030
    int        m_inited;        // ROR 0x034
    int        m_phase;         // ROR 0x038: 0, 3 start, 4 run, 6 / 7 end
    SprMan     m_spr;           // ROR 0x03c
    MagSystem *m_system;        // ROR 0x928
};

/* PC: frame line `line` stays in the window while the bars are up, for a
 * prompt the ROM draws past the band a wide window zooms to. Asked for every
 * frame the prompt shows. */
void s2dCineScoKeepLine(int line);

/* EE 0x940. layout: host. */
struct S2dBg : MagLife
{
    S2dBg();                                                             // ROR 0x00190f6c
    ~S2dBg();                                                            // ROR 0x00190fc8

    void init(DogTexInfo *tex);                                          // ROR 0x00191068
    int  clear();                                                        // ROR 0x00191054
    void term();                                                         // ROR 0x00191098
    /* EE slot 2. */
    void live() override;                                                // ROR 0x001910d8
    /* EE slot 6. */
    void render(I3dLayer *layer) override;                               // ROR 0x00191100
    int  open();                                                         // ROR 0x00191124
    int  close();                                                        // ROR 0x00191158
    int  step();                                                         // ROR 0x00191174
    void start();                                                        // ROR 0x00191248
    int  process();                                                      // ROR 0x00191258
    int  subStep();                                                      // ROR 0x00191278
    int  processBg();                                                    // ROR 0x001912a0
    /* Names picture no (< 7) for the next load. */
    int  setTexName(int no, const char *name);                           // ROR 0x001913c4
    void dispIn(int no, int time);                                       // ROR 0x0019143c
    void dispOut(int no, int time);                                      // ROR 0x00191478

    /* The requests: each moves m_bg_step one state on, 1 when taken. */
    // ROR 0x0028d158
    int s2dBgInit()
    {
        if (m_bg_step == 0) {
            m_bg_step = 1;
            return 1;
        }
        return 0;
    }
    // ROR 0x0028d178
    bool s2dBgInitStat() { return m_bg_step == 2; }
    // ROR 0x0028d188
    int s2dBgLoadStart()
    {
        if (m_bg_step == 2) {
            m_bg_step = 3;
            return 1;
        }
        return 0;
    }
    // ROR 0x0028d1ac
    bool s2dBgLoadStat() { return m_bg_step == 5; }
    // ROR 0x0028d1bc
    int s2dBgDispStart()
    {
        if (m_bg_step == 5) {
            m_bg_step = 6;
            return 1;
        }
        return 0;
    }
    // ROR 0x0028d1e0
    int s2dBgDispStat() { return 0; }
    // ROR 0x0028d1e8
    int s2dBgClose()
    {
        if (m_bg_step == 6) {
            m_bg_step = 7;
            return 1;
        }
        return 0;
    }
    // ROR 0x0028d20c
    void s2dBgDispOn_(int no)
    {
        m_spr.m_sprite[no].m_disp = 1;
        m_spr.m_sprite[no].m_state = 1;
    }
    // ROR 0x0028d244
    void s2dBgDispOff(int no) { m_spr.m_sprite[no].m_state = 0; }

    int         m_open;         // ROR 0x030
    int         m_inited;       // ROR 0x034
    int         m_phase;        // ROR 0x038
    int         m_sub;          // ROR 0x03c: 0 runs subStep in phase 4
    int         m_sub_state;    // ROR 0x040
    int         m_44;           // ROR 0x044: open sets 2
    SprMan      m_spr;          // ROR 0x048
    MagSystem  *m_system;       // ROR 0x934
    DogTexInfo *m_tex;          // ROR 0x938
    int         m_bg_step;      // ROR 0x93c: 0 idle, 1 init, 2 ready, 3 load, 4 loading, 5 loaded, 6 shown, 7 / 8 close
};

/* The sprites of the 14 S2dMes items (data 0x29bb70). */
#define S2D_MES_ITEM_MAX 14

/* EE at least 0x950 + the map-name buffer (the scene's next known word is
 * +0x43700, 0xb50 after it). layout: host. */
struct S2dMes : MagLife
{
    S2dMes();                                                            // ROR 0x00191494
    ~S2dMes();                                                           // ROR 0x001914fc

    int  init(DogTexInfo *tex);                                          // ROR 0x00191588
    int  clear();                                                        // ROR 0x0019183c
    void term();                                                         // ROR 0x001915f8
    void loadTexture();                                                  // ROR 0x00191640
    /* EE slot 2. */
    void live() override;                                                // ROR 0x00191684
    /* EE slot 6. */
    void render(I3dLayer *layer) override;                               // ROR 0x001916a0
    int  open();                                                         // ROR 0x001916c4
    int  close();                                                        // ROR 0x001916f8
    int  step();                                                         // ROR 0x00191718
    void start();                                                        // ROR 0x00191844
    int  process();                                                      // ROR 0x001918a8
    int  waitMapName();                                                  // ROR 0x001918c8
    int  drawMapName();                                                  // ROR 0x00191938

    /* The item setters: no < S2D_MES_ITEM_MAX, true when it is. */
    bool setSpriteStdPoint(int no, int x, int y);                        // ROR 0x00191ba8
    bool setSpriteStdPoint(int no, int x, int y, int z);                 // ROR 0x00191c00
    bool setSpriteOffPoint(int no, int x, int y);                        // ROR 0x00191c64
    bool setSpriteOffPoint(int no, int x, int y, int z);                 // ROR 0x00191cbc
    bool setSpriteState(int no, int on);                                 // ROR 0x00191d20
    bool setSpriteDisp(int no, int on);                                  // ROR 0x00191d74
    bool setSpriteAnim(int no, int anim);                                // ROR 0x00191dc8
    bool setSpriteFadeIn(int no, int time);                              // ROR 0x00191e04
    int  setSpriteFadeOut(int no, int time);                             // ROR 0x00191e58
    bool setSpriteColorStd(int no);                                      // ROR 0x00191ed0
    bool setSpriteProcessOff(int no);                                    // ROR 0x00191f44
    bool setSpriteProcessOn(int no);                                     // ROR 0x00191f98
    bool backupSprite(int no);                                           // ROR 0x00191ff0
    int  restoreSprite(int no);                                          // ROR 0x001920c4

    // ROR 0x0028de3c
    void setMapNumber(int no) { m_map_no = no; }
    // ROR 0x0028de44
    void setMapNameFadeIn(int time)
    {
        if (m_map_name_on == 0) {
            m_fade_time = time;
            m_map_name_on = 1;
        }
    }
    // ROR 0x0028de60
    void setMapNameFadeOut(int time)
    {
        if (m_map_name_on == 1) {
            m_fade_time = time;
            m_map_name_on = 0;
        }
    }

    int        m_open;          // ROR 0x030
    int        m_inited;        // ROR 0x034
    int        m_phase;         // ROR 0x038
    int        m_sub;           // ROR 0x03c: 0 waits for a map name, 1 shows it
    int        m_sub_state;     // ROR 0x040: 0, 6, 14, 15
    int        m_44;            // ROR 0x044: open sets 2
    SprMan     m_spr;           // ROR 0x048
    MagSystem *m_system;        // ROR 0x934
    int        _938;
    int        m_map_no;        // ROR 0x93c: setMapNumber
    int        m_fade_count;    // ROR 0x940
    int        m_map_name_on;   // ROR 0x944: setMapNameFadeIn / Out
    int        m_view;          // ROR 0x948: the scene text view of the caption
    int        m_fade_time;     // ROR 0x94c: frames; start sets 30
    char       m_map_name[0x100]; // ROR 0x950: EE length not known (at most 0x200)
};

/* The sprites of S2dMovie's one item (data 0x29bfb0). */
#define S2D_MOVIE_ITEM_MAX 1

/* EE 0x90c (the scene's +0x853b4 follows). No vtable. layout: host. */
struct S2dMovie
{
    S2dMovie();                                                          // ROR 0x0019226c
    ~S2dMovie();                                                         // ROR 0x001922a4

    void init(i3dContext *ctx, MagScene *scene);                  // ROR 0x00192318
    void term();                                                         // ROR 0x0019237c
    void loadTexture();                                                  // ROR 0x001923c4
    void update();                                                       // ROR 0x00192408
    /* MagScene::draw's calls: the movie frame (key 890, while RxMpeg has
     * one) and the two sprite layers (keys 900 / 1000). */
    void drawMovie(i3dContext *ctx, I3dLight *light, I3dView *view);     // ROR 0x0019241c
    void draw(i3dContext *ctx, I3dLight *light, I3dView *view);          // ROR 0x001924e0
    I3dLayer *createLayer(i3dContext *ctx, float key);                   // ROR 0x001925f0
    int  open();                                                         // ROR 0x00192670
    int  close();                                                        // ROR 0x001926a4
    int  step();                                                         // ROR 0x001926c0
    int  clear();                                                        // ROR 0x00192794
    void start();                                                        // ROR 0x0019279c
    int  process();                                                      // ROR 0x001927ec
    int  subStep();                                                      // ROR 0x0019280c

    /* The movie (g_rxMpeg): status -1 idle, 0 playing, 1 finished. */
    void playMovie(const char *name);                                    // ROR 0x00192834
    void playMovie(const char *name, int w, int h);                      // ROR 0x00192850
    int  isMovieEnd();                                                   // ROR 0x001928b8
    int  stopMovie();                                                    // ROR 0x001928d4
    int  getMovieStatus();                                               // ROR 0x00192904
    int  closeMovie();                                                   // ROR 0x00192920

    /* S2dMes's item setters over one item (no < S2D_MOVIE_ITEM_MAX). */
    bool setSpriteStdPoint(int no, int x, int y);                        // ROR 0x00192978
    bool setSpriteOffPoint(int no, int x, int y);                        // ROR 0x001929cc
    bool setSpriteState(int no, int on);                                 // ROR 0x00192a20
    bool setSpriteDisp(int no, int on);                                  // ROR 0x00192a70

    int        m_open;          // ROR 0x000
    int        m_inited;        // ROR 0x004
    int        m_phase;         // ROR 0x008
    int        m_sub;           // ROR 0x00c
    int        m_sub_state;     // ROR 0x010
    int        m_14;            // ROR 0x014: open sets 2
    SprMan     m_spr;           // ROR 0x018
    MagSystem *m_system;        // ROR 0x904
    MagScene  *m_scene;         // ROR 0x908
};

#endif /* _SPR_S2D_H */
