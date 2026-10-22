/* ==========================================================================
 *  mag/magscene2.cpp
 *
 *  The scene's set-up half (RoR's "mag/magscene2.cpp", 0x1316b8..): init
 *  and initRender (fog, texture table, the 2D layers registered with the
 *  life manager, the view records), binding the PC and Brown to their
 *  actors, the sky slots' release, the camera target. The constructor,
 *  destructor and loaders are still stubs below.
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
        t->m_4fc = 0;
        t->m_4f4 = 0;
        t->m_4f8 = 0;
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
    m_life_418f0.m_34 = m_2910;
    m_life_man.add(&m_life_418f0, 0x8004);
    m_cine_sco.init(&m_tex_info);
    m_cine_sco.m_level = 7;
    m_life_man.add(&m_cine_sco, 0x8006);
    m_life_man.add(&m_life_41850, 0x8008);
    m_mes.init(&m_tex_info);
    m_mes.m_level = 7;
    m_life_man.add(&m_mes, 0x800a);

    FUN_00176afc(&m_837e0, ctx, this);
    FUN_00192318(&m_84aa8, ctx, this);
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

// ──────────────────────────────────────────────────────────────────────
// Frees sky slot no when it is in use: waits for the GS path, releases
// its three models and its buffer, frees its textures.
// ROR 0x001386d4

void MagScene::releaseTenkyu(int no)
{
    MagSceneTenkyu *t = &m_tenkyu[no];

    if (t->m_id < 0)
        return;
    sceGsSyncPath(0, 0);
    t->m_model[0].release(0);
    t->m_model[1].release(0);
    t->m_model[2].release(0);
    t->m_4f4 = 0;
    t->m_4f8 = 0;
    t->m_4fc = 0;
    t->m_500 = 0;
    if (t->m_buf)
    {
        g_utlMemory.free(t->m_buf);
        t->m_buf = NULL;
    }
    t->m_id = -1;
    releaseTenkyuTex(no);
    FUN_0017c2c0(&m_8542c);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00138804

void MagScene::releaseTenkyuAll()
{
    for (int i = 0; i < 4; i++)
        releaseTenkyu(i);
}

// ──────────────────────────────────────────────────────────────────────
// Loads actor no's motion pack from the PC's pack file (the ROM passes
// b before a).
// ROR 0x001390d4

void MagScene::setActorAnim(int no, int a, int b)
{
    m_actors[no].loadPackMotion("data/ch/pc/pc/MOTIONPK.RPK", b, a);
}

// ──────────────────────────────────────────────────────────────────────
// After the frame's draw: the PC's and Brown's motion sequences.
// callcheck: updateMotionSeq (inlined MagLifeControlChara::renderPostProcess calls it)
// ROR 0x0013c29c

void MagScene::postDraw()
{
    m_pc.renderPostProcess();
    m_bwn.renderPostProcess();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013c2ec

void MagScene::setCameraTarget(const float *pos, const float *at)
{
    m_camera_pos[0] = pos[0];
    m_camera_pos[1] = pos[1];
    m_camera_pos[2] = pos[2];
    m_camera_pos[3] = pos[3];
    m_camera_at[0] = at[0];
    m_camera_at[1] = at[1];
    m_camera_at[2] = at[2];
    m_camera_at[3] = at[3];
    m_camera_target_set = 1;
}

// ──────────────────────────────────────────────────────────────────────
// Empty in the ROM (mainInitGame calls it).
// ROR 0x0013c928

void MagScene::initCamera()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0013d5ac

int MagScene::getCameraMode(int *out)
{
    if (out)
    {
        out[0] = m_camera_mode;
        out[1] = m_camera_2ad58;
    }
    return m_camera_mode;
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

// stub of ROR 0x0013fe64
int MagScene::addLight(int type, unsigned int mask)
{
    ROSEPAN_STUB("MagScene::addLight", 0x0013fe64);
    return {};
}

// stub of ROR 0x001377f0
int MagScene::load(i3dContext *ctx, int a, int b, int c)
{
    ROSEPAN_STUB("MagScene::load", 0x001377f0);
    return {};
}

// stub of ROR 0x0013910c
int MagScene::loadActorAnim(int no, int arg, const char *name)
{
    ROSEPAN_STUB("MagScene::loadActorAnim", 0x0013910c);
    return {};
}

// stub of ROR 0x00138ed0
int MagScene::loadModel(i3dContext *ctx, int id, const char *rpk)
{
    ROSEPAN_STUB("MagScene::loadModel", 0x00138ed0);
    return {};
}

// stub of ROR 0x00138bc4
void MagScene::loadModelByName(i3dContext *ctx, const char *name, int arg,
                         const char *rpk)
{
    ROSEPAN_STUB("MagScene::loadModelByName", 0x00138bc4);
}
