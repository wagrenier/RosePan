/* ==========================================================================
 *  mag/magscene.cpp
 *
 *  The scene's core (RoR's "mag/magscene.cpp", 0x1316b8..0x137300; its
 *  asserts run from 0x131a28 to 0x135f44, the functions before and after
 *  them are taken as the same file): binding the PC and Brown to their
 *  actors, init and initRender (fog, texture table, the 2D layers
 *  registered with the life manager, the view records). The constructor,
 *  destructor, start, update and the window functions are still stubs.
 * ======================================================================== */
#include "mag/magscene.h"

#include "common/rosepan_stub.h"
#include "sdk/libgraph.h"
#include "utl/utlfind.h"
#include "utl/utlmisc.h"

/* rodata 0x2ed500: the shadow01 image DogTexInfo::add builds the scene's
 * first texture from (0x20 bytes; format not decoded yet). */
static const u_char s_shadow01_image[0x20] = {
    0x00, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// ──────────────────────────────────────────────────────────────────────
// Binds the player's controller to actor no (no < 0 unbinds it) and marks
// the actor as bound.
// ROR 0x001316b8

void MagScene::setPC(int no, int mode)
{
    m_pc_no = no;
    m_pc_mode = mode;
    if (no < 0)
    {
        m_pc.setActor(NULL, no, mode);
        m_22d4 = -1;
        m_22d0 = 0;
    }
    else
    {
        m_pc.setActor(&m_actors[no], no, mode);
        m_actor_flags[no] |= 0x60;
        m_pc.initMotion();
        m_22d0 = 1;
        m_22d4 = -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Binds Brown's controller to actor no (no < 0 unbinds it); Brown's
// command layer gets both controllers while he is bound.
// ROR 0x00131790

void MagScene::setBwn(int no, int arg)
{
    m_bwn_no = no;
    if (no < 0)
    {
        m_bwn.setActor(NULL, -1, 0);
        m_bwn_ctrl.m_pc = NULL;
        m_bwn_ctrl.m_bwn = NULL;
    }
    else
    {
        m_bwn.setActor(&m_actors[no], no, arg);
        m_bwn_ctrl.m_bwn = &m_bwn;
        m_bwn_ctrl.m_pc = &m_pc;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Once at boot (MagSystem::init): the fog records, the light number, the
// life manager, the texture table with shadow01, the sky slots.
// ROR 0x00131840

void MagScene::init()
{
    i3dFogInit(&m_fog[0]);
    m_fog[3] = m_fog[0];
    m_fog[2] = m_fog[3];
    m_fog[1] = m_fog[2];
    m_fog_2ab64 = 0;
    m_fog_2ab60 = 0;
    setLightNo(-1);
    m_life_man.init();
    m_tex_info.init(0x100);
    m_tex_info.add(s_shadow01_image, "shadow01");
    m_2aad1 = 3;
    m_2aad0 = 1;
    for (int i = 0; i < 4; i++)
    {
        MagSceneTenkyu *t = &m_tenkyu[i];
        t->m_id = -1;
        t->m_c_rec = NULL;
        t->m_c_data = NULL;
        t->m_c_num = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Before the first frame (MagSystem::start): the two view records, the
// controllers' scene links, the 2D layers on the life manager, the
// render objects, the 1 MB 2D packet buffer. Returns 1.
// ROR 0x00131b90

int MagScene::initRender(i3dContext *ctx)
{
    static const MagViewParam view = {
        5.0f, 2000.0f, -5.12f, 5.12f, -0x1.1eb85p+2f, 0x1.1eb85p+2f, /* +-4.48: 0x408f5c28 in the ROM */
        65530.0f, 1.0f, 0.0f, 511.0f, 0.0f, 447.0f, 1792.0f, 1824.0f,
    };

    m_view[0] = view;
    m_view[1] = view;
    m_2aa80 = -1.0f;
    m_view[1].m_zmin = m_22a4 * 1.0f;
    m_view[1].m_zmax = m_22a4 * 65530.0f;
    setLightNo(-1);
    m_pc.m_scene = this;
    m_bwn_ctrl.m_130 = -1;
    m_bwn_ctrl.m_138 = -1;
    m_bwn.m_scene = this;
    m_bwn_ctrl.m_scene = this;

    m_bg.init(&m_tex_info);
    m_bg.m_level = 7;
    m_life_man.add(&m_bg, 0x8000);
    m_life_418f0.m_34 = m_common_tex_no[1];
    m_life_man.add(&m_life_418f0, 0x8004);
    m_cine_sco.init(&m_tex_info);
    m_cine_sco.m_level = 7;
    m_life_man.add(&m_cine_sco, 0x8006);
    m_life_man.add(&m_life_41850, 0x8008);
    m_mes.init(&m_tex_info);
    m_mes.m_level = 7;
    m_life_man.add(&m_mes, 0x800a);

    FUN_00176afc(&m_837e0, ctx, this);
    m_movie.init(ctx, this);
    m_22d4 = -1;
    m_22d0 = 0;
    m_43700 = -1;
    FUN_001944d0();
    m_229c = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Unbinds the PC from Brown's command layer and ends the PC's world.
// ROR 0x0013604c

void MagScene::clearPC()
{
    m_bwn_ctrl.m_78 = 0;
    utlFindClear(&m_bwn_ctrl.m_find);
    m_pc.termWorld();
}

// ---- stubs (hand-written: constructors and destructors are not stubbed by stubgen) ----

// stub of ROR 0x001360a0
// PC: runs before main (g_magSystem's member); the stub only logs.
MagScene::MagScene()
{
    ROSEPAN_STUB("MagScene::MagScene", 0x001360a0);
}

// stub of ROR 0x00136d14
MagScene::~MagScene()
{
    ROSEPAN_STUB("MagScene::~MagScene", 0x00136d14);
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00135234
void MagScene::drawView(int no, int a, int b, const void *p, int c)
{
    ROSEPAN_STUB("MagScene::drawView", 0x00135234);
}

// stub of ROR 0x00134944
void MagScene::setViewParam(int no, const int *v)
{
    ROSEPAN_STUB("MagScene::setViewParam", 0x00134944);
}

// stub of ROR 0x00131a28
void MagScene::start()
{
    ROSEPAN_STUB("MagScene::start", 0x00131a28);
}

// stub of ROR 0x001327dc
void MagScene::update(BedPad *pad0, BedPad *pad1)
{
    ROSEPAN_STUB("MagScene::update", 0x001327dc);
}

// stub of ROR 0x001348b0
void MagScene::closeWindow(int no)
{
    ROSEPAN_STUB("MagScene::closeWindow", 0x001348b0);
}

// stub of ROR 0x00135088
void MagScene::fadeInWindow(int no, int time, int from, int to)
{
    ROSEPAN_STUB("MagScene::fadeInWindow", 0x00135088);
}

// stub of ROR 0x0013511c
void MagScene::fadeOutWindow(int no, int time, int from, int to)
{
    ROSEPAN_STUB("MagScene::fadeOutWindow", 0x0013511c);
}

// stub of ROR 0x00134730
int MagScene::openWindow(int w, int h, int a3, int life)
{
    ROSEPAN_STUB("MagScene::openWindow", 0x00134730);
    return {};
}

// stub of ROR 0x001351b0
void MagScene::setWindowShow(int no, int on)
{
    ROSEPAN_STUB("MagScene::setWindowShow", 0x001351b0);
}

// stub of ROR 0x00135594
void MagScene::updateWindowFont(int no)
{
    ROSEPAN_STUB("MagScene::updateWindowFont", 0x00135594);
}
