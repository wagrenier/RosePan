/* ==========================================================================
 *  mag/magscene.cpp
 *
 *  The scene's core (RoR's "mag/magscene.cpp", 0x1316b8..0x137300; its
 *  asserts run from 0x131a28 to 0x135f44, the functions before and after
 *  them are taken as the same file): binding the PC and Brown to their
 *  actors, init and initRender (fog, texture table, the 2D layers
 *  registered with the life manager, the view records), start, the
 *  per-frame update by sub mode (fades, menus, text windows, actors and
 *  their footsteps, sky billboards). The constructor, destructor, openMenu
 *  and most window functions are still stubs.
 * ======================================================================== */
#include "mag/magscene.h"

#include "common/ror_assert.h"
#include "common/rosepan_stub.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dtim2.h"
#include "sdk/libgraph.h"
#include "utl/utlfind.h"
#include "utl/utlgame.h"
#include "utl/utlmisc.h"
#include "utl/utltm2.h"

/* rodata 0x2ed500: the shadow01 texture DogTexInfo::add builds the scene's
 * first texture from: an I3dTexDesc (256 x 256, one level at block
 * 0x2300, PSMCT32, tcc 1, bilinear; 0x20 bytes with the padding). */
alignas(4) static const u_char s_shadow01_image[0x20] = {
    0x00, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// ──────────────────────────────────────────────────────────────────────
// Starts a fade (p->m_mode 0 stops the current one): kinds 0 / 4 run the
// fader (to black / to 0xff), 1 and 2 the keyhole, whose texture is loaded
// when the kind changes. A start while the same direction is already
// running is ignored.
// ROR 0x001311f4 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::setFade(const MagFadeParam *p)
{
    int cur = m_fade.m_kind;

    if (p->m_mode >= 2 && p->m_mode < 4)
    {
        if (getFadeState() == 3)
            return;
    }
    else if (p->m_mode >= 6 && p->m_mode < 8)
    {
        if (getFadeState() == 4)
            return;
    }

    if (p->m_mode == 0)
    {
        if (cur == 1 || cur == 2)
        {
            m_fade_key.releaseTexture(&m_tex_info);
            m_life_man.remove(&m_fade_key);
        }
        else if (cur == 0 || cur == 4)
            m_fader.stop();
        else
            ROR_ASSERT(0); /* 201 */
    }
    else
    {
        if (cur >= 0 && cur != p->m_kind)
        {
            m_fade_key.releaseTexture(&m_tex_info);
            m_life_man.remove(&m_fade_key);
            cur = -1;
        }
        if (cur < 0 && p->m_kind >= 0)
        {
            switch (p->m_kind)
            {
            case 0:
                m_fader.m_color = 0;
                m_fader.m_level = p->m_level;
                break;
            case 1:
            case 2:
            case 3:
                m_fade_key.loadTexture(&m_tex_info, 0);
                m_fade_key.m_level = p->m_level;
                break;
            case 4:
                m_fader.m_color = 0xff;
                m_fader.m_level = p->m_level;
                break;
            default:
                ROR_ASSERT(0); /* 247 */
            }
        }
        if (p->m_kind == 1 || p->m_kind == 2)
        {
            m_fade_key.start(p);
            if (m_life_man.find(&m_fade_key) < 0)
                m_life_man.add(&m_fade_key, 0x8003);
        }
        else if (p->m_kind == 0 || p->m_kind == 4)
            m_fader.start(p);
        else
            ROR_ASSERT(0); /* 269 */
    }

    m_fade = *p;
    if (p->m_mode == 0)
        m_fade.m_kind = -1;
}

// ──────────────────────────────────────────────────────────────────────
// The current fade object's state mapped to 0..6 (0 when no fade runs).
// ROR 0x00131550

int MagScene::getFadeState()
{
    int state;
    switch (m_fade.m_kind)
    {
    case 0:
    case 4:
        state = m_fader.m_state;
        break;
    case 1:
    case 2:
        state = m_fade_key.m_state;
        break;
    default:
        return 0;
    }
    switch (state)
    {
    case 1:
    case 5:
        return 2;
    case 2:
    case 3:
        return 3;
    case 4:
        return 5;
    case 6:
    case 7:
        return 4;
    case 8:
        return 6;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The far plane of both view records (the second also takes the first's
// near plane).
// ROR 0x00131618

void MagScene::setViewFar(float far)
{
    m_view[0].m_far = far;
    m_view[1].m_near = m_view[0].m_near;
    m_view[1].m_far = far;
}

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
        m_sub_mode.m_step = -1;
        m_sub_mode.m_mode = 0;
    }
    else
    {
        m_pc.setActor(&m_actors[no], no, mode);
        m_actor_flags[no] |= 0x60;
        m_pc.initMotion();
        m_sub_mode.m_mode = 1;
        m_sub_mode.m_step = -1;
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
    m_tex_info.add((const I3dTexDesc *)s_shadow01_image, "shadow01");
    m_2aad1 = 3;
    m_2aad0 = 1;
    for (int i = 0; i < 4; i++)
    {
        MagSceneTenkyu *t = &m_tenkyu[i];
        t->m_id = -1;
        t->m_path_data = NULL;
        t->m_c_data = NULL;
        t->m_c_num = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// After loadSystemData (MagSystem::start): each text window's font draws
// into its texture's image (bit8a..bit8d, 512 x 256, 8 or 4 bits by the
// TIM2's image type); the scene interfaces point at the scene.
// ROR 0x00131a28 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::start()
{
    loadSystemData();
    for (int i = 0; i < 4; i++)
    {
        int no = m_font_tex_no[i];
        if (no < 0)
            continue; /* PC: loadSystemData has asserted the font texture is missing (a failed read); the ROM would index the table with -1 */
        u_char *p = ((I3dTexture *)m_tex_info.m_tex_info[no].pPicture)->m_level[0].m_data;
        ROR_ASSERT(p); /* 554 */
        const I3dTim2Picture *pic = utlTm2GetPicture(m_tex_info.m_tex_data[no].m_tm2, 0);
        if (pic->m_image_type == 5)
            m_window_font[i].setTarget(p, 0x200, 0x100, 8);
        else
            m_window_font[i].setTarget(p, 0x200, 0x100, 4);
    }
    m_854e8 = this;
    m_854e0 = this;
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
    m_menu_id = -1;
    m_menu_result = -1;
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
    m_life_man.add(&m_fader, 0x8008);
    m_mes.init(&m_tex_info);
    m_mes.m_level = 7;
    m_life_man.add(&m_mes, 0x800a);

    FUN_00176afc(&m_837e0, ctx, this);
    m_movie.init(ctx, this);
    m_sub_mode.m_step = -1;
    m_sub_mode.m_mode = 0;
    m_fade.m_kind = -1;
    FUN_001944d0();
    m_229c = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Turns the sky models' billboard nodes to the camera: node flag 1 (b_a*
// names) fully, 2 (b_y*) about y only; each keeps its translation.
// ROR 0x00132048  // callcheck: FUN_00215a30, I3rRoot__CheckIdString, i3rGetNumNode (inside the inline DogModel::isAlive / get_nNode and ROR_ASSERT)

void MagScene::updateTenkyuBillboards()
{
    for (int i = 0; i < 4; i++)
    {
        DogModel *model = &m_tenkyu[i].m_model[0];
        if (!model->isAlive())
            continue;

        sceVu0FVECTOR dir = {0.0f, 0.0f, 0.0f, 1.0f};
        m_camera_entity.getMatrixZ(dir);
        sceVu0FVECTOR rot = {0.0f, 0.0f, 0.0f, 1.0f};
        utlVecToRot(dir, rot);
        rot[0] = -rot[0];
        if (rot[0] >= 3.1415927f)
            rot[0] -= 6.2831855f;
        if (rot[0] < -3.1415927f)
            rot[0] += 6.2831855f;
        rot[1] += 3.1415927f;
        if (rot[1] >= 3.1415927f)
            rot[1] -= 6.2831855f;
        if (rot[1] < -3.1415927f)
            rot[1] += 6.2831855f;

        int n = model->get_nNode();
        int *flag = model->getNodeFlag();
        sceVu0FMATRIX *pmat = model->getModelMatrix();
        ROR_ASSERT(pmat); /* 813 */
        for (; n > 0; n--, flag++, pmat++)
        {
            sceVu0FVECTOR pos;
            if (*flag == 1)
            {
                i3dVecCopy(pos, (*pmat)[3]);
                i3dMatUnit(*pmat);
                i3dMatRotZ(0x1.921fb8p+1f, *pmat, *pmat); /* 0x40490fdc, one ulp over pi */
                i3dMatRotX(rot[0], *pmat, *pmat);
                i3dMatRotY(rot[1], *pmat, *pmat);
                i3dMatTranslate(*pmat, *pmat, pos);
            }
            else if (*flag == 2)
            {
                i3dVecCopy(pos, (*pmat)[3]);
                i3dMatUnit(*pmat);
                i3dMatRotZ(0x1.921fb8p+1f, *pmat, *pmat);
                i3dMatRotY(rot[1], *pmat, *pmat);
                i3dMatTranslate(*pmat, *pmat, pos);
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// Brown's controller and command layer; Brown's actor keeps flag 0x60
// while he is within 250 units of the PC in height.
// ROR 0x0013233c

void MagScene::updateBwn()
{
    if (!m_bwn.m_actor)
        return;
    m_bwn.live();
    m_bwn_ctrl.live();
    if (!m_pc.m_actor)
        return;

    unsigned *flags = &m_actor_flags[m_bwn_no];
    float dy = m_bwn.m_actor->m_trans[1] - m_pc.m_actor->m_trans[1];
    if (dy >= -250.0f && dy <= 250.0f)
        *flags |= 0x60;
    else
        *flags &= ~0x60u;
}

// ──────────────────────────────────────────────────────────────────────
// A footstep: when the actor's motion has event bit 0 in this frame's
// step, the sound of the floor sector under it (or the map's default slot)
// from the actor's table, at its position.
// ROR 0x00132430 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::playActorFootstep(int theNo)
{
    ROR_ASSERT(m_actor_snd[theNo]); /* 887 */
    DogActor *actor = &m_actors[theNo];
    float t = actor->getMotionFrame();
    if (!(t >= 0.0f))
        return;
    float step = actor->getMotionStep();
    if (step > -0x1.999998p-4f && step < 0x1.999998p-4f) /* +-0x3dcccccc */
        return;
    if (!(actor->getMotionEvent(t, t + step, -1) & 1))
        return;

    sceVu0FVECTOR pos;
    pos[0] = actor->m_trans[0];
    pos[1] = actor->m_trans[1];
    pos[2] = actor->m_trans[2];
    pos[3] = actor->m_trans[3];
    MagSectorHit *hit[4] = {NULL, NULL, NULL, NULL};
    FUN_0017c340(&m_8542c, pos, hit, 0, 40.0f, 0.0f);

    int sndid = -1;
    if (hit[0])
    {
        MagSectorRec *sec = NULL;
        if (m_8542c.m_0c && m_8542c.m_sectors)
            sec = &m_8542c.m_sectors[hit[0]->m_sector];
        if (sec)
            sndid = sec->m_snd;
    }
    if (sndid < 0)
        sndid = m_854a4;
    if (sndid > 0)
    {
        ROR_ASSERT(sndid < 12); /* 933 */
        int id = m_actor_snd[theNo][sndid];
        if (id >= 0)
        {
            sceVu0FVECTOR at;
            at[0] = actor->m_trans[0];
            at[1] = actor->m_trans[1];
            at[2] = actor->m_trans[2];
            at[3] = actor->m_trans[3];
            utlPlaySe3d(id, at);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// The live actors' motion commands: in_menu 0 runs the actors without
// flag 8 (and their footsteps), 1 only those with it (they keep moving
// under a menu or a message).
// ROR 0x0013262c  // callcheck: FUN_00215a30, I3rRoot__CheckIdString (inside the inline DogModel::isAlive)

void MagScene::updateActors(int in_menu)
{
    for (int i = 0; i < 128; i++)
    {
        DogActor *actor = &m_actors[i];
        if (!actor->isAlive())
            continue;
        if (!in_menu)
        {
            if (!(m_actor_flags[i] & 8))
            {
                actor->runMotionCmd();
                if (m_actor_snd[i])
                    playActorFootstep(i);
            }
        }
        else if (m_actor_flags[i] & 8)
            actor->runMotionCmd();
    }
}

// ──────────────────────────────────────────────────────────────────────
// Saves the sub mode and pauses (sub mode 8).
// ROR 0x00132744

void MagScene::pushSubMode()
{
    m_sub_mode.push();
    m_sub_mode.m_mode = 8;
    m_sub_mode.m_step = -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00132784

void MagScene::popSubMode()
{
    m_sub_mode.pop();
}

// ──────────────────────────────────────────────────────────────────────
// Sub mode 8: only the lives of group 4 and the camera.
// ROR 0x001327a0

void MagScene::updatePaused()
{
    m_life_man.update(4);
    m_camera_entity.transform();
}

// ──────────────────────────────────────────────────────────────────────
// One frame of the scene (MagSystem::update), by sub mode:
//  0  the first frame: everything runs once, then 1 when the PC is bound;
//  1  walking: the pads open the menus (pad 0 bit 3 the item menu, bit 0
//     menu 0x10001) and turn the camera (bits 8 / 9), the PC takes pad 0
//     (its state 0x17 goes to 6), Brown's command layer pad 1;
//  2  text windows: closes those that are done (or wait for pad bit 14),
//     back to 1 when none waits;
//  3  fades out (getMenuFadeOutTime) and opens the requested menu -> 7;
//  4  after a menu: fades back in -> 1;
//  5  the system message, closed by pad bits 13 / 14;
//  6  the PC's state 0x17: the camera hangs from the PC and turns once
//     every 2400 counts;
//  7  the menu takes pad 0 until processPad returns non-zero;
//  8  paused (pushSubMode); 9 the debug windows.
// ROR 0x001327dc // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::update(BedPad *pad0, BedPad *pad1)
{
    m_movie.update();

    int next;
    switch (m_sub_mode.m_mode)
    {
    case 9:
        updateDebug(pad0, pad1);
        return;

    case 8:
        updatePaused();
        return;

    case 0:
        updateFog();
        FUN_0017772c(&m_837e0);
        m_life_man.update(2);
        m_pc.live();
        updateBwn();
        updateActors(0);
        updateCamera();
        m_camera_entity.transform();
        updateTenkyuBillboards();
        rotateTenkyu();
        updateRecords853fc();
        next = 1;
        if (!m_pc.m_actor)
            return;
        break;

    case 2:
    {
        for (int i = 0; i < 4; i++)
        {
            MagLifeWindow *w = m_window[i];
            if (!w)
                continue;
            if (w->m_wait_key == 0)
                closeWindow(i);
            else if (w->m_wait_key > 0 && (pad0->m_78 >> 14 & 1))
                closeWindow(i);
        }
        int waiting = 0;
        for (int i = 0; i < 4; i++)
        {
            if (m_window[i])
                waiting += m_window[i]->m_wait_key > 0;
        }
        if (waiting < 1)
        {
            m_sub_mode.m_mode = 1;
            m_sub_mode.m_step = -1;
        }
        m_life_man.update(1);
        return;
    }

    case 1:
    {
        updateFog();
        FUN_0017772c(&m_837e0);
        m_life_man.update(1);
        ROR_ASSERT(m_pMenuBase == NULL); /* 1102 */
        if (pad0->m_7c >> 3 & 1)
        {
            if (canOpenItemMenu())
            {
                clearGamFlag25();
                requestMenu(0x10000, 0);
                return;
            }
        }
        else if ((pad0->m_7c & 1) && canOpenMenu10001())
        {
            clearGamFlag25();
            requestMenu(0x10001, 0);
        }
        u_int buttons = pad0->m_7a;
        if ((buttons >> 8 & 1) || (buttons >> 9 & 1))
            changeCamera();
        if (m_pc.m_actor)
        {
            m_pc.processPad(pad0);
            if (m_pc.getState17Count() >= 0)
            {
                m_sub_mode.m_mode = 6;
                m_sub_mode.m_step = -1;
            }
        }
        m_pc.live();
        if (m_bwn.m_actor)
            m_bwn_ctrl.processPad(pad1);
        updateBwn();
        updateActors(0);
        updateCamera();
        m_camera_entity.transform();
        updateTenkyuBillboards();
        rotateTenkyu();
        updateRecords853fc();
        return;
    }

    case 3:
    {
        bool go = false;
        updateFog();
        m_life_man.update(1);
        rotateTenkyu();
        updateRecords853fc();
        if (m_sub_mode.m_arg == 0x10007)
            go = true;
        else if (m_sub_mode.m_step < 1)
        {
            int time = getMenuFadeOutTime();
            if (time < 1)
                go = true;
            else
            {
                MagFadeParam fade = {0, 6, time, 0, 0, 7};
                setFade(&fade);
                m_sub_mode.m_step = 1;
            }
        }
        if (!go && getFadeState() < 5)
            return;
        openMenu(m_sub_mode.m_arg);
        next = 7;
        break;
    }

    case 7:
        m_life_man.update(1);
        updateActors(1);
        if (!m_pMenuBase)
        {
            ROR_ASSERT(m_pMenuBase); /* 1350 */
            if (!m_pMenuBase)
                return;
        }
        m_menu_result = m_pMenuBase->processPad(pad0);
        if (isMenuRunning())
            return;
        closeMenu();
        /* closeMenu has already set m_menu_id to -1, so the ROM always takes
         * the first branch and never the second. */
        if (m_menu_id != 0x10007)
        {
            MagFadeParam fade = {0, 1, 0, 0, 0, 7}; /* the ROM leaves m_time .. m_10 unset */
            setFade(&fade);
        }
        if (m_menu_id != 0x1000b)
            return;
        setGamWord54();
        return;

    case 6:
    {
        updateFog();
        m_life_man.update(1);
        m_pc.live();
        int count = m_pc.getState17Count();
        if (count >= 0)
        {
            if (count == 0)
            {
                m_camera_entity.setParent(&m_actors[m_pc_no]);
                m_camera_2ad58 = m_pc_no;
            }
            m_camera_entity.m_dirty = 1;
            m_camera_entity.m_trans[0] = 0.0f;
            m_camera_entity.m_trans[2] = 0.0f;
            m_camera_entity.m_trans[1] = -150.0f - (float)count * 0.5f;
            m_camera_entity.setRotate(-1.5707964f, 0.0f,
                                      ((float)count * 6.2831855f) / 2400.0f - 3.1415927f);
        }
        updateCamera();
        m_camera_entity.transform();
        updateTenkyuBillboards();
        rotateTenkyu();
        updateRecords853fc();
        return;
    }

    case 5:
        updateFog();
        FUN_0017772c(&m_837e0);
        m_life_man.update(1);
        rotateTenkyu();
        updateRecords853fc();
        m_pc.live();
        updateActors(1);
        ROR_ASSERT(m_sys_msg_window >= 0); /* 1445 */
        if ((pad0->m_7c >> 13 & 1) || (pad0->m_7c >> 14 & 1))
            closeSysMsgWindow(m_sys_msg_window);
        return;

    case 4:
    {
        bool go = false;
        updateFog();
        m_life_man.update(1);
        m_pc.live();
        updateBwn();
        updateActors(0);
        rotateTenkyu();
        updateRecords853fc();
        if (m_sub_mode.m_arg == 0x10007)
            go = true;
        else if (m_sub_mode.m_step < 1)
        {
            int time = getMenuFadeInTime();
            if (time > 0)
            {
                MagFadeParam fade = {0, 2, time, 0, 0, 7};
                setFade(&fade);
                m_sub_mode.m_step = 1;
            }
            go = time <= 0;
            if (m_pc_no >= 0)
                m_pc.initMotion();
        }
        next = 1;
        if (!go)
        {
            if (getFadeState() > 4)
            {
                MagFadeParam fade = {0, 0, 0, 0, 0, 7};
                go = true;
                setFade(&fade);
            }
            if (!go)
                return;
        }
        break;
    }

    default:
        return;
    }

    m_sub_mode.m_mode = next;
    m_sub_mode.m_step = -1;
}

// ──────────────────────────────────────────────────────────────────────
// Closes text window winNo (removed from the life manager and deleted).
// ROR 0x001348b0 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::closeWindow(int winNo)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2137 */
    MagLifeWindow **pw = &m_window[winNo];
    if (*pw)
    {
        m_life_man.remove(*pw);
        delete *pw; /* the ROM runs slot 1 with flags 3 */
        *pw = NULL;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Asks for menu id (with arg): sub mode 3 fades out and opens it. The PC's
// mode and number are kept beside the id.
// ROR 0x001357b0

void MagScene::requestMenu(int id, int arg)
{
    m_sub_mode.m_mode = 3;
    m_sub_mode.m_step = -1;
    m_menu_arg = arg;
    m_sub_mode.m_arg = id;
    m_sub_mode.m_arg2 = (int)(((u_int)m_pc_mode << 16) | ((u_int)m_pc_no & 0xffff));
    gamEnterMenu();
}

// ──────────────────────────────────────────────────────────────────────
// True while the open menu's last processPad returned 0.
// ROR 0x00135b64

bool MagScene::isMenuRunning()
{
    return m_menu_result == 0;
}

// ──────────────────────────────────────────────────────────────────────
// Closes the open menu: terms it (not the item menu, 0x10000), removes it
// from the life manager, deletes the ones openMenu allocated (0x1000b ..
// 0x1000f), then sub mode 4 fades back in; menu 0x10010 chains to 0x10005.
// ROR 0x00135b78

void MagScene::closeMenu()
{
    if (!m_pMenuBase)
        return;
    int id = m_menu_id;
    if (id != 0x10000)
        m_pMenuBase->term();
    m_life_man.remove(m_pMenuBase);
    if ((u_int)(m_menu_id - 0x1000b) < 5 && m_pMenuBase)
        delete m_pMenuBase; /* the ROM runs slot 1 with flags 3 */
    m_pMenuBase = NULL;
    m_menu_id = -1;
    gamLeaveMenu();
    m_sub_mode.m_mode = 4;
    m_sub_mode.m_step = -1;
    if (id == 0x10010)
    {
        clearGamFlag25();
        requestMenu(0x10005, 0);
    }
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
    /* PC: until the constructor is written, its loop at 0x136ab4 that frees
     * the strip records (MagScene::draw walks them every frame, and zeroed
     * records read as type 0 with no vertices). */
    for (int i = 0; i < 0x2000; i++)
    {
        m_strip[i].m_type = -1;
        m_strip[i].m_data = NULL;
        m_strip[i].m_life = -1;
        m_strip[i].m_count = 0;
        m_strip[i].m_colors = NULL;
    }
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

// stub of ROR 0x0013580c
void MagScene::openMenu(int id)
{
    ROSEPAN_STUB("MagScene::openMenu", 0x0013580c);
}
