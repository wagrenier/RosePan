/* ==========================================================================
 *  mag/magscene.cpp
 *
 *  The scene's core (RoR's "mag/magscene.cpp", 0x1316b8..0x137300; its
 *  asserts run from 0x131a28 to 0x135f44, the functions before and after
 *  them are taken as the same file): binding the PC and Brown to their
 *  actors, init and initRender (fog, texture table, the 2D layers
 *  registered with the life manager, the view records), start, the
 *  per-frame update by sub mode (fades, menus, text windows, actors and
 *  their footsteps, sky billboards), the text windows, the strip records
 *  and the constructor. The destructor and openMenu are still stubs.
 * ======================================================================== */
#include "mag/magscene.h"

#include <math.h>
#include <string.h>

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
// Fades the screen in (mode 2) over time frames at level: from black, or
// from the 0xff colour when white is set (fade kind 4).
// ROR 0x00131184

void MagScene::fadeIn(int time, int level, int white)
{
    MagFadeParam fade = {white ? 4 : 0, 2, time, 0, 0, level}; /* the ROM leaves m_0c / m_10 unset */
    setFade(&fade);
}

// ──────────────────────────────────────────────────────────────────────
// Fades the screen out (mode 6) the same way.
// ROR 0x001311bc

void MagScene::fadeOut(int time, int level, int white)
{
    MagFadeParam fade = {white ? 4 : 0, 6, time, 0, 0, level}; /* idem */
    setFade(&fade);
}

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
// The fader's frame counter (+0x30: frames since start, up to its time).
// ROR 0x00131608

int MagScene::getFaderWord30()
{
    return m_fader.m_frame;
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
// ROR 0x0013164c

bool MagScene::isPCWordDcSet()
{
    return m_pc.isWordDcSet();
}

// ──────────────────────────────────────────────────────────────────────
// Passes an event command to the player's controller.
// ROR 0x0013166c

void MagScene::commandPC(int cmd, const float *arg)
{
    m_pc.command(cmd, arg, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00131690

void MagScene::setPCState14(int arg, int word)
{
    m_pc.setState14(arg, word);
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
// Tears the scene down: the 0x837e0 object and the movie, the font file,
// every actor's model, the sky slots, the texture table, and every life
// object (the fader, a member, is taken off first).
// ROR 0x00131f74

void MagScene::term()
{
    FUN_00176c50(&m_837e0);
    m_movie.term();
    if (m_font_data)
    {
        g_utlMemory.free(m_font_data);
        m_font_data = NULL;
    }
    for (int i = 0; i < 128; i++)
        m_actors[i].releaseModel(NULL);
    releaseTenkyuAll();
    m_tex_info.term();
    m_life_man.remove(&m_fader);
    // PC: the scene's other member lives (initRender adds them) come off
    // the manager too: deleteAll deletes what is left, and the host's
    // delete on a member of the scene corrupts the heap.
    m_life_man.remove(&m_fade_key);
    m_life_man.remove(&m_life_418f0);
    m_life_man.remove(&m_cine_sco);
    m_life_man.remove(&m_bg);
    m_life_man.remove(&m_mes);
    m_life_man.deleteAll();
    FUN_00194548();
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
            else if (w->m_wait_key > 0 && (pad0->m_hold >> 14 & 1))
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
        if (pad0->m_repeat >> 3 & 1)
        {
            if (canOpenItemMenu())
            {
                clearGamFlag25();
                requestMenu(0x10000, 0);
                return;
            }
        }
        else if ((pad0->m_repeat & 1) && canOpenMenu10001())
        {
            clearGamFlag25();
            requestMenu(0x10001, 0);
        }
        u_int buttons = pad0->m_trig;
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
        if ((pad0->m_repeat >> 13 & 1) || (pad0->m_repeat >> 14 & 1))
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
// The number of actor thePtr (its index in m_actors).
// ROR 0x00133090 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

int MagScene::getActorNo(DogActor *thePtr)
{
    ROR_ASSERT(thePtr); /* 1495 */
    return (int)(thePtr - m_actors);
}

// ──────────────────────────────────────────────────────────────────────
// Actor no's motion number, bit-inverted while its motion is stopped
// (speed 0).
// ROR 0x001330f4

int MagScene::getActorMotion(int no)
{
    DogActor *actor = &m_actors[no];
    if (actor->m_motion.m_speed == 0.0f)
        return ~actor->m_motion.getNo();
    return actor->m_motion.getNo();
}

// ──────────────────────────────────────────────────────────────────────
// Starts motion on actor no at speed (p1d8 >= 0 first goes to
// setParam1d8); flags >> 8 and flags & 0xff become the actor's two motion
// flag words.
// ROR 0x00133144

void MagScene::setActorMotion(float speed, int no, int motion, int p1d8, u_int flags)
{
    if (p1d8 >= 0)
        m_actors[no].setParam1d8(p1d8);
    DogActor *actor = &m_actors[no];
    actor->setMotion(speed, motion, 0, -1, -1);
    actor->m_motion_flags = (int)flags >> 8;
    actor->m_motion_flags2 = flags & 0xff;
}

// ──────────────────────────────────────────────────────────────────────
// Blends actor no to motion (the flags as in setActorMotion).
// ROR 0x001331f4

void MagScene::changeActorMotion(float speed, int no, int motion, int p1d8, u_int flags)
{
    if (p1d8 >= 0)
        m_actors[no].setParam1d8(p1d8);
    m_actors[no].changeMotion(speed, motion, (int)flags >> 8, flags & 0xff, -1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00133288

void MagScene::setActorMotionByName(float speed, int no, const char *name, int p1d8, u_int flags)
{
    int motion = m_actors[no].findMotionByName(name);
    if (motion >= 0)
        setActorMotion(speed, no, motion, p1d8, flags);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00133324

void MagScene::changeActorMotionByName(float speed, int no, const char *name, int p1d8,
                                       u_int flags)
{
    int motion = m_actors[no].findMotionByName(name);
    if (motion >= 0)
        changeActorMotion(speed, no, motion, p1d8, flags);
}

// ──────────────────────────────────────────────────────────────────────
// fps motion frames per second -> frames per 60 Hz frame.
// ROR 0x001333c0

void MagScene::setActorMotionSpeed(float fps, int no)
{
    m_actors[no].m_motion.m_speed = fps * 0x1.11112Ep-6f; /* 0x3c888897, the ROM's 1/60 */
}

// ──────────────────────────────────────────────────────────────────────
// The length of motion (< 0: the current one) of actor no.
// ROR 0x001333e4

int MagScene::getActorMotionLength(int no, int motion)
{
    if (motion < 0)
        motion = m_actors[no].m_motion.getNo();
    return m_actors[no].getMotionLength(motion);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00133440

float MagScene::getActorMotionSpeed(int no)
{
    return m_actors[no].m_motion.m_speed;
}

// ──────────────────────────────────────────────────────────────────────
// The distance (in dir lengths) along dir from pos to the first thing it
// hits: a wall segment when dir is level, else (or when no wall is hit)
// the collision model of sky slot tenkyu (< 0: the first of the four that
// is hit). -1 when nothing is.
// ROR 0x00133454  // callcheck: FUN_00215a30, I3rRoot__CheckIdString (inside the inline DogModel::isAlive)

float MagScene::castRay(MagSceneRayHit *hit, const float *pos, const float *dir, int tenkyu)
{
    float t;

    if (dir[1] == 0.0f)
    {
        t = castWalls(hit, pos, dir);
        if (t >= 0.0f)
            return t;
    }
    if (tenkyu < 0)
    {
        for (int i = 0; i < 4; i++)
        {
            DogModel *col = &m_tenkyu[i].m_model[2];
            if (m_tenkyu[i].m_id < 0)
                continue;
            if (col->isAlive())
            {
                t = col->castRay(hit, pos, dir);
                if (t >= 0.0f)
                    return t;
            }
        }
    }
    else if (m_tenkyu[tenkyu].m_id >= 0)
    {
        DogModel *col = &m_tenkyu[tenkyu].m_model[2];
        if (!col->isAlive())
            return -1.0f;
        return col->castRay(hit, pos, dir);
    }
    return -1.0f;
}

// ──────────────────────────────────────────────────────────────────────
// A sphere of radius r at pos: the walls first (1 on a hit), then sky
// slot 0's collision model; -1 when nothing is hit or r <= 0.
// ROR 0x00133658 // callcheck: FUN_00215a30, I3rRoot__CheckIdString (inside the inline DogModel::isAlive)

float MagScene::castSphere(float r, MagSceneRayHit *hit, const float *pos)
{
    if (r <= 0.0f)
        return -1.0f;
    float t = hitWallsSphere(r, hit, pos);
    if (t >= 0.0f)
        return t;
    if (m_tenkyu[0].m_id < 0)
        return -1.0f;
    DogModel *col = &m_tenkyu[0].m_model[2];
    if (!col->isAlive())
        return -1.0f;
    return col->castSphere(r, hit, pos);
}

// ──────────────────────────────────────────────────────────────────────
// The height where pos + t dir (t in 0..1) meets sky slot 0, 100000 when
// it does not. Nothing in the ROM calls it.
// ROR 0x0013377c

float MagScene::getGroundHeight(const float *pos, const float *dir)
{
    MagSceneRayHit hit;

    for (int i = 0; i < 5; i++)
    {
        hit.m_vec[i][0] = 0.0f;
        hit.m_vec[i][1] = 0.0f;
        hit.m_vec[i][2] = 0.0f;
        hit.m_vec[i][3] = 1.0f;
    }
    hit.m_50 = -1;
    float t = castRay(&hit, pos, dir, 0);
    if (t >= 0.0f && t <= 1.0f)
        return pos[1] + dir[1] * t;
    return 100000.0f;
}

// ──────────────────────────────────────────────────────────────────────
// A vertical cylinder of radius theR standing on pos and reaching theH
// (< 0) along y, against the wall segments whose height range overlaps
// it: within theR of a segment (taken at pos's height) is a hit, with the
// wall normal in hit->m_vec[1]. 1 on a hit, else -1.
// ROR 0x0013386c // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

float MagScene::hitWallsCylinder(float theH, float theR, MagSceneRayHit *hit, const float *pos)
{
    ROR_ASSERT(theH < 0.0f); /* 1683 */
    float r2 = theR * theR;
    float h = fabsf(theH);
    float mid = pos[1] + theH * 0.5f;

    for (int i = 0; i < MAX_PRIVATE_COLLI; i++)
    {
        const MagSceneVecPair *w = &m_vec_pair[i];
        float y0 = w->m_a[1];
        float y1 = w->m_b[1];

        if (y0 == y1)
            continue;
        if (fabsf((y0 + y1) * 0.5f - mid) > (fabsf(y0 - y1) + h) * 0.5f)
            continue;

        sceVu0FVECTOR p1, d1;
        p1[0] = w->m_a[0];
        p1[1] = pos[1];
        p1[2] = w->m_a[2];
        p1[3] = w->m_a[3];
        d1[0] = w->m_b[0] - w->m_a[0];
        d1[1] = 0.0f;
        d1[2] = w->m_b[2] - w->m_a[2];
        d1[3] = 1.0f;

        float t;
        if (i3dPointSegDist2(pos, p1, d1, &t) <= r2)
        {
            if (hit)
            {
                hit->m_vec[1][1] = 0.0f;
                hit->m_vec[1][2] = d1[0];
                hit->m_vec[1][0] = -d1[2];
                utlNormalize3(hit->m_vec[1]);
            }
            return 1.0f;
        }
    }
    return -1.0f;
}

// ──────────────────────────────────────────────────────────────────────
// The level ray pos + t dir (t in 0..1) against the wall segments in
// m_vec_pair whose height range covers pos's y: each is tested in the
// plane at that height (i3dSegSegDist2 0 = they cross). Returns t of the
// first hit with its wall normal in hit->m_vec[1], else -1.
// ROR 0x00133a0c // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

float MagScene::castWalls(MagSceneRayHit *hit, const float *pos, const float *dir)
{
    ROR_ASSERT(dir[1] == 0.0f); /* 1740: "theD.m_y == 0.0f" */
    for (int i = 0; i < 256; i++)
    {
        const MagSceneVecPair *w = &m_vec_pair[i];
        float y0 = w->m_a[1];
        float y1 = w->m_b[1];

        if (y0 == y1)
            continue;
        if (fabsf((y0 + y1) * 0.5f - pos[1]) > fabsf(y0 - y1) * 0.5f)
            continue;

        sceVu0FVECTOR p1, d1;
        p1[0] = w->m_a[0];
        p1[1] = pos[1];
        p1[2] = w->m_a[2];
        p1[3] = w->m_a[3];
        d1[0] = w->m_b[0] - w->m_a[0];
        d1[1] = 0.0f;
        d1[2] = w->m_b[2] - w->m_a[2];
        d1[3] = 1.0f;

        float s, u;
        if (i3dSegSegDist2(pos, dir, p1, d1, &s, &u) == 0.0f)
        {
            if (hit)
            {
                hit->m_vec[1][1] = 0.0f;
                hit->m_vec[1][2] = d1[0];
                hit->m_vec[1][0] = -d1[2];
                utlNormalize3(hit->m_vec[1]);
            }
            return s;
        }
    }
    return -1.0f;
}

// ──────────────────────────────────────────────────────────────────────
// A sphere of radius theR at pos against the wall segments whose height
// range covers pos's y; as hitWallsCylinder.
// ROR 0x00133b80 // callcheck: FUN_00215a30, FUN_0026cb48 (the assert handler; sqrtf's NaN path)

float MagScene::hitWallsSphere(float theR, MagSceneRayHit *hit, const float *pos)
{
    ROR_ASSERT(theR > 0.0f); /* 1794 */
    for (int i = 0; i < MAX_PRIVATE_COLLI; i++)
    {
        const MagSceneVecPair *w = &m_vec_pair[i];
        float y0 = w->m_a[1];
        float y1 = w->m_b[1];

        if (y0 == y1)
            continue;
        if (fabsf((y0 + y1) * 0.5f - pos[1]) > fabsf(y0 - y1) * 0.5f)
            continue;

        sceVu0FVECTOR p1, d1;
        p1[0] = w->m_a[0];
        p1[1] = pos[1];
        p1[2] = w->m_a[2];
        p1[3] = w->m_a[3];
        d1[0] = w->m_b[0] - w->m_a[0];
        d1[1] = 0.0f;
        d1[2] = w->m_b[2] - w->m_a[2];
        d1[3] = 1.0f;

        float t;
        if (sqrtf(i3dPointSegDist2(pos, p1, d1, &t)) <= theR)
        {
            if (hit)
            {
                hit->m_vec[1][1] = 0.0f;
                hit->m_vec[1][2] = d1[0];
                hit->m_vec[1][0] = -d1[2];
                utlNormalize3(hit->m_vec[1]);
            }
            return 1.0f;
        }
    }
    return -1.0f;
}

// ──────────────────────────────────────────────────────────────────────
// A cylinder of radius r at pos, dir[1] high, against the walls, then
// the collision model of sky slot tenkyu (< 0: the first of the four that
// is hit). >= 0 on a hit.
// ROR 0x00133d10 // callcheck: FUN_00215a30, I3rRoot__CheckIdString (inside the inline DogModel::isAlive)

float MagScene::castCylinder(float r, const float *pos, const float *dir, int tenkyu,
                             MagSceneRayHit *hit)
{
    float t = hitWallsCylinder(dir[1], r, hit, pos);
    if (t >= 0.0f)
        return t;
    if (tenkyu >= 0)
        return m_tenkyu[tenkyu].m_model[2].castCylinder(r, pos, dir, hit);
    for (int i = 0; i < 4; i++)
    {
        if (m_tenkyu[i].m_id < 0)
            continue;
        DogModel *col = &m_tenkyu[i].m_model[2];
        if (col->isAlive())
        {
            t = col->castCylinder(r, pos, dir, hit);
            if (t >= 0.0f)
                return t;
        }
    }
    return -1.0f;
}

// ──────────────────────────────────────────────────────────────────────
// A strip record's colour from RGBA bytes: r, g, b / 255, a / 128 (the
// PS2's alpha 0x80 = 1). The ROM inlines it in each adder.

static void setStripRGBA(float *c, u_int rgba)
{
    c[0] = (float)(rgba & 0xff) / 255.0f;
    c[1] = (float)((rgba >> 8) & 0xff) / 255.0f;
    c[2] = (float)((rgba >> 16) & 0xff) / 255.0f;
    c[3] = (float)(rgba >> 24) * 0.0078125f;
}

// ──────────────────────────────────────────────────────────────────────
// A strip of count vertices at pos with a colour per vertex (type 0) for
// life frames (-1: until cleared). Returns the record, -1 when all are
// used.
// ROR 0x00133e94

int MagScene::addStrip(int count, void *pos, const void *colors, void *data2, int life)
{
    for (int i = 0; i < 0x2000; i++)
    {
        MagSceneStrip *st = &m_strip[i];
        if (st->m_type != -1)
            continue;
        st->m_data2 = data2;
        st->m_life = life;
        st->m_count = (short)count;
        st->m_data = pos;
        st->m_type = 0;
        st->m_colors = colors;
        return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// The same drawn through the layer's context (type 1); the life is left
// as the record has it.
// ROR 0x00133efc

int MagScene::addStripCtx(int count, void *pos, const void *colors, void *data2)
{
    for (int i = 0; i < 0x2000; i++)
    {
        MagSceneStrip *st = &m_strip[i];
        if (st->m_type != -1)
            continue;
        st->m_data2 = data2;
        st->m_type = 1;
        st->m_count = (short)count;
        st->m_data = pos;
        st->m_colors = colors;
        return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// A line strip of count vertices in one colour (type 2).
// ROR 0x00133f68

int MagScene::addLineStrip(int count, void *pos, u_int rgba, void *data2)
{
    for (int i = 0; i < 0x2000; i++)
    {
        MagSceneStrip *st = &m_strip[i];
        if (st->m_type != -1)
            continue;
        st->m_data2 = data2;
        st->m_type = 2;
        st->m_count = (short)count;
        st->m_data = pos;
        setStripRGBA(st->m_color, rgba);
        return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// A wire box (type 3: drawStrips passes data2 and data to
// i3dLayerDrawWireBox). Nothing in the ROM calls it.
// ROR 0x00134114

int MagScene::addWireBox(void *data, void *data2, u_int rgba)
{
    for (int i = 0; i < 0x2000; i++)
    {
        MagSceneStrip *st = &m_strip[i];
        if (st->m_type != -1)
            continue;
        st->m_data2 = data2;
        st->m_type = 3;
        st->m_data = data;
        setStripRGBA(st->m_color, rgba);
        return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// Type 4 (drawStrips draws data's vectors). Nothing in the ROM calls it.
// ROR 0x001342bc

int MagScene::addStripType4(void *data, u_int rgba)
{
    for (int i = 0; i < 0x2000; i++)
    {
        MagSceneStrip *st = &m_strip[i];
        if (st->m_type != -1)
            continue;
        st->m_data = data;
        st->m_type = 4;
        setStripRGBA(st->m_color, rgba);
        return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// A wire sphere of radius r at pos (type 5). Nothing in the ROM calls it.
// ROR 0x00134460

int MagScene::addWireSphere(float r, void *pos)
{
    for (int i = 0; i < 0x2000; i++)
    {
        MagSceneStrip *st = &m_strip[i];
        if (st->m_type != -1)
            continue;
        st->m_data2 = pos;
        st->m_type = 5;
        st->m_color[0] = r;
        return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// Record no's colour (no check of no). Nothing in the ROM calls it.
// ROR 0x001344c4

int MagScene::setStripColor(int no, u_int rgba)
{
    setStripRGBA(m_strip[no].m_color, rgba);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// A copy of rec in the first free record. Nothing in the ROM calls it.
// ROR 0x00134624

int MagScene::addStripRecord(const MagSceneStrip *rec)
{
    for (int i = 0; i < 0x2000; i++)
    {
        if (m_strip[i].m_type != -1)
            continue;
        m_strip[i] = *rec;
        return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// Frees strip record no (no < 0: all 0x2000): no type, no data, no life.
// ROR 0x00134698

void MagScene::clearStrip(int no)
{
    if (no < 0)
    {
        for (int i = 0; i < 0x2000; i++)
        {
            m_strip[i].m_type = -1;
            m_strip[i].m_data = NULL;
            m_strip[i].m_life = -1;
            m_strip[i].m_count = 0;
            m_strip[i].m_colors = NULL;
        }
        return;
    }
    m_strip[no].m_type = -1;
    m_strip[no].m_data = NULL;
    m_strip[no].m_life = -1;
    m_strip[no].m_count = 0;
    m_strip[no].m_colors = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Opens a text window w x h in the first free slot, drawing with that
// slot's font (its texture from start) and closing by life (see
// MagLifeWindow::m_wait_key; > 0 also waits in sub mode 2). Returns the
// slot, -1 when all four are open. a3 is not read.
// ROR 0x00134730 // callcheck: operator_new, FUN_00215a30 (new MagLifeWindow; the assert handler, inside ROR_ASSERT)

int MagScene::openWindow(int w, int h, int a3, int life)
{
    int n;

    for (n = 0; n < MAX_WINDOW; n++)
        if (m_window[n] == NULL)
            break;
    if (n >= MAX_WINDOW)
        return -1;

    m_window[n] = new MagLifeWindow();
    ROR_ASSERT(m_window[ n ]); /* 2110 */
    m_window[n]->setup(w, h);
    GamFont *font = &m_window_font[n];
    font->setFont(m_font_data);
    m_window[n]->setFontManager(font);
    m_window[n]->m_tex_no = m_font_tex_no[n];
    m_window[n]->m_wait_key = life;
    if (life > 0)
    {
        m_sub_mode.m_mode = 2;
        m_sub_mode.m_step = -1;
    }
    m_life_man.add(m_window[n], 0x800c);
    return n;
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
// Places window theN: v = x, y, z (the position, w 1), width, height.
// ROR 0x00134944 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::setViewParam(int theN, const int *v)
{
    ROR_ASSERT(theN >= 0 && theN < MAX_WINDOW); /* 2156 */
    MagLifeWindow *w = m_window[theN];
    if (w)
    {
        w->m_pos[0] = (float)v[0];
        w->m_pos[1] = (float)v[1];
        w->m_pos[2] = (float)v[2];
        w->m_pos[3] = 1.0f;
        w->m_w = v[3];
        w->m_h = v[4];
    }
}

// ──────────────────────────────────────────────────────────────────────
// Moves window winNo to (x, y, z).
// ROR 0x00134a00 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::setWindowPos(int winNo, int x, int y, int z)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2179 */
    MagLifeWindow *w = m_window[winNo];
    if (w)
    {
        w->m_pos[0] = (float)x;
        w->m_pos[1] = (float)y;
        w->m_pos[2] = (float)z;
        w->m_pos[3] = 1.0f;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00134ab8 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::setWindowSize(int winNo, int w, int h)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2194 */
    MagLifeWindow *pw = m_window[winNo];
    if (pw)
    {
        pw->m_h = h;
        pw->m_w = w;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00134b34 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::getWindowPos(int winNo, int *x, int *y)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2205 */
    if (m_window[winNo])
        m_window[winNo]->getPos(x, y);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00134bb8 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::getWindowPos3(int winNo, int *x, int *y, int *z)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2215 */
    if (m_window[winNo])
        m_window[winNo]->getPos3(x, y, z);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00134c4c // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::getWindowSize(int theN, int *w, int *h)
{
    ROR_ASSERT(theN >= 0 && theN < MAX_WINDOW); /* 2225 */
    MagLifeWindow *pw = m_window[theN];
    if (pw)
    {
        *w = pw->m_w;
        *h = pw->m_h;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The window's four words +0x84..+0x90 (roles unknown).
// ROR 0x00134cd0 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::setWindowWords84(int theN, int a, int b, int c, int d)
{
    ROR_ASSERT(theN >= 0 && theN < MAX_WINDOW); /* 2242 */
    MagLifeWindow *pw = m_window[theN];
    if (pw)
    {
        pw->m_84[3] = d;
        pw->m_84[0] = a;
        pw->m_84[1] = b;
        pw->m_84[2] = c;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00134d6c // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::getWindowWords84(int theN, int *a, int *b, int *c, int *d)
{
    ROR_ASSERT(theN >= 0 && theN < MAX_WINDOW); /* 2252 */
    MagLifeWindow *pw = m_window[theN];
    if (pw)
    {
        *a = pw->m_84[0];
        *b = pw->m_84[1];
        *c = pw->m_84[2];
        *d = pw->m_84[3];
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00134e18 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::getWindowWordCc(int theN, int *out)
{
    ROR_ASSERT(theN >= 0 && theN < MAX_WINDOW); /* 2262 */
    if (m_window[theN])
        *out = m_window[theN]->m_cc;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00134e88 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::getWindowCharMetrics(int winNo, int code, int *left, int *width, int *extra)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2272 */
    if (m_window[winNo])
        m_window[winNo]->getCharMetrics(code, left, width, extra);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00134f2c // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::setWindowProportional(int winNo, int on)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2282 */
    if (m_window[winNo])
        m_window[winNo]->setProportional(on);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00134fa0 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::setWindowFontWord30(int winNo, int v)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2292 */
    if (m_window[winNo])
        m_window[winNo]->setFontWord30(v);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00135014 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::setWindowFontWord34(int winNo, int v)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2302 */
    if (m_window[winNo])
        m_window[winNo]->setFontWord34(v);
}

// ──────────────────────────────────────────────────────────────────────
// Fades window theN in (MagLifeWindow::fadeIn).
// ROR 0x00135088 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::fadeInWindow(int theN, int time, int from, int to)
{
    ROR_ASSERT(theN >= 0 && theN < MAX_WINDOW); /* 2318 */
    if (m_window[theN])
        m_window[theN]->fadeIn(time, from, to);
}

// ──────────────────────────────────────────────────────────────────────
// Fades window theN out (MagLifeWindow::fadeOut).
// ROR 0x0013511c // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::fadeOutWindow(int theN, int time, int from, int to)
{
    ROR_ASSERT(theN >= 0 && theN < MAX_WINDOW); /* 2333 */
    if (m_window[theN])
        m_window[theN]->fadeOut(time, from, to);
}

// ──────────────────────────────────────────────────────────────────────
// Hides window winNo (hide != 0) or shows it: the window's slot 7,
// setHide. (The Ghidra name says "show"; the ROM passes the flag to
// setHide, and S2d's map name passes 0 to show its window.)
// ROR 0x001351b0 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::setWindowShow(int winNo, int hide)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2349 */
    if (m_window[winNo])
        m_window[winNo]->setHide(hide != 0);
}

// ──────────────────────────────────────────────────────────────────────
// Draws str into window winNo's font buffer at (x, y), then releases the
// window font's texture so the next frame uploads the new pixels.
// ROR 0x00135234 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::drawView(int winNo, int x, int y, const void *p, int arg)
{
    const char *str = (const char *)p;

    ROR_ASSERT(str != NULL); /* 2369: "str != __null" (GCC's NULL) */
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2371 */
    if (m_window[winNo])
    {
        m_window[winNo]->drawText(x, y, str, arg);
        I3dTexEntry *pti = m_tex_info.getTexInfo(m_font_tex_no[winNo]);
        ROR_ASSERT(pti); /* 2381 */
        if (pti == NULL)
            return; /* PC: the ROM reads address 4 here */
        ROR_ASSERT(pti->pPicture); /* 2382 */
        i3dTexRelease((I3dTexture *)pti->pPicture);
    }
}

// ──────────────────────────────────────────────────────────────────────
// Draws text in color at window winNo's cursor (when it is open) and
// re-uploads its font texture.
// ROR 0x00135364 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::printWindow(int winNo, const char *text, int color)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2395 */
    if (m_window[winNo])
    {
        m_window[winNo]->print(text, color);
        i3dTexRelease((I3dTexture *)m_tex_info.m_tex_info[m_font_tex_no[winNo]].pPicture);
    }
}

// ──────────────────────────────────────────────────────────────────────
// Window winNo's fill colour (its font's setFill).
// ROR 0x00135418 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::setWindowFill(int winNo, u_int fill)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2409 */
    m_window_font[winNo].setFill(fill);
}

// ──────────────────────────────────────────────────────────────────────
// The font's fill flag, then the fill colour 0 again (so it takes the flag).
// ROR 0x00135494 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::setWindowFillMsb(int winNo, int on)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2417 */
    m_window_font[winNo].setFillMsb(on);
    setWindowFill(winNo, 0);
}

// ──────────────────────────────────────────────────────────────────────
// Window winNo's text colour (no check of winNo). Nothing in the ROM
// calls it.
// ROR 0x00135520

void MagScene::setWindowColor(int winNo, u_int color)
{
    m_window_font[winNo].setColor(color);
}

// ──────────────────────────────────────────────────────────────────────
// The width of text in window no's font.
// ROR 0x0013555c

int MagScene::getWindowTextWidth(int no, const char *text)
{
    return m_window_font[no].getTextWidth(text);
}

// ──────────────────────────────────────────────────────────────────────
// Clears window winNo's font buffer and releases its texture.
// ROR 0x00135594 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::updateWindowFont(int winNo)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2452 */
    m_window_font[winNo].clear();
    i3dTexRelease((I3dTexture *)m_tex_info.m_tex_info[m_font_tex_no[winNo]].pPicture);
}

// ──────────────────────────────────────────────────────────────────────
// Re-uploads window winNo's font texture.
// ROR 0x00135628 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::releaseWindowTex(int winNo)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2469 */
    i3dTexRelease((I3dTexture *)m_tex_info.m_tex_info[m_font_tex_no[winNo]].pPicture);
}

// ──────────────────────────────────────────────────────────────────────
// Scrolls window theN's text buffer by rows and re-uploads it.
// ROR 0x0013569c // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::scrollWindow(int theN, int rows)
{
    ROR_ASSERT(theN >= 0 && theN < MAX_WINDOW); /* 2478 */
    m_window_font[theN].scroll(rows);
    i3dTexRelease((I3dTexture *)m_tex_info.m_tex_info[m_font_tex_no[theN]].pPicture);
}

// ──────────────────────────────────────────────────────────────────────
// Gives window winNo's font the language's font file again.
// ROR 0x00135740 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::resetWindowFont(int winNo)
{
    ROR_ASSERT(winNo >= 0 && winNo < MAX_WINDOW); /* 2489 */
    m_window_font[winNo].setFont(m_font_data);
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
// Makes menu id the open one: ids 0x10000..0x10009 and 0x10010 are the
// embedded menus, 0x1000b and 0x1000c..0x1000f are allocated (closeMenu
// deletes them), the families 0x20001..0x7fffff pick the board / pbook
// menu by bit and keep the id in it. A menu not yet initialised gets the
// scene, the requested argument and the texture table. Then it goes on
// the life manager and its result is cleared.
// ROR 0x0013580c // callcheck: operator_new, FUN_00215a30, MagLifeMenuSpecial, MagLifeMenuSpecial2 (new MagLifeMenuSpecial / Special2; the assert handler, inside ROR_ASSERT)

void MagScene::openMenu(int id)
{
    switch (id)
    {
    case 0x10000:
        m_pMenuBase = &m_menu_item;
        break;
    case 0x10001:
        m_pMenuBase = &m_menu_map;
        break;
    case 0x10002:
        m_pMenuBase = &m_menu_stock;
        break;
    case 0x10003:
        m_pMenuBase = &m_menu_save;
        break;
    case 0x10004:
        m_pMenuBase = &m_menu_load;
        break;
    case 0x10005:
        m_pMenuBase = &m_menu_title;
        break;
    case 0x10006:
        m_pMenuBase = &m_menu_opt;
        break;
    case 0x10007:
        m_pMenuBase = &m_menu_movie;
        break;
    case 0x10008:
        m_pMenuBase = &m_menu_mcd;
        break;
    case 0x10009:
        m_pMenuBase = &m_menu_now;
        break;
    case 0x1000b:
        m_pMenuBase = new MagLifeMenuSpecial;
        break;
    case 0x1000c:
    case 0x1000d:
    case 0x1000e:
    case 0x1000f:
    {
        MagLifeMenuSpecial2 *menu = new MagLifeMenuSpecial2;
        m_pMenuBase = menu;
        menu->m_type = id - 0x1000c;
        break;
    }
    case 0x10010:
        m_pMenuBase = &m_menu_sellang;
        break;
    default:
        if ((u_int)(id - 0x20001) < 0x7dffff)
        {
            MagLifeMenuBase *menu;
            if (id & 0x40000)
                menu = &m_menu_board2d;
            else if (id & 0x80000)
                menu = &m_menu_board3d;
            else if ((id & 0x100000) || (id & 0x200000))
                menu = &m_menu_pbook;
            else
            {
                ROR_ASSERT(0); /* 2602 */
                break;
            }
            m_pMenuBase = menu;
            menu->m_type = id;
        }
        else
        {
            ROR_ASSERT(0); /* 2606 */
        }
        break;
    }
    /* PC: an unknown id leaves no menu (the ROM goes on with what
     * m_pMenuBase held, NULL after closeMenu). */
    if (!m_pMenuBase)
        return;
    if (!m_pMenuBase->isInit())
    {
        m_pMenuBase->m_menu_scene = this;
        m_pMenuBase->m_menu_arg = m_menu_arg;
        m_pMenuBase->init(&m_tex_info);
    }
    m_life_man.add(m_pMenuBase, 0x8002);
    m_menu_id = id;
    m_menu_result = 0;
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
// The keyhole fade with theTexName from EVNTEX.RPK: loads the texture,
// resets the fade key to state 5 and puts it on the life manager (0x8003)
// when it is not there yet.
// ROR 0x00135cac // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::openFadeKey(const char *theTexName)
{
    ROR_ASSERT(theTexName); /* 2711 */
    m_fade_key.loadTextureByName(&m_tex_info, theTexName);
    m_fade_key.reset(5, 0);
    if (m_life_man.find(&m_fade_key) < 0)
        m_life_man.add(&m_fade_key, 0x8003);
}

// ──────────────────────────────────────────────────────────────────────
// The keyhole's magnification, over time frames (<= 0: at once).
// ROR 0x00135d58 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::setFadeKeyMagnify(float theMagnify, int time)
{
    ROR_ASSERT(theMagnify > 0.0f); /* 2725 */
    m_fade_key.setMagnify(theMagnify, time);
}

// ──────────────────────────────────────────────────────────────────────
// Takes the keyhole fade off the life manager and frees its texture.
// ROR 0x00135dcc

void MagScene::removeFadeKey()
{
    m_life_man.remove(&m_fade_key);
    m_fade_key.releaseTexture(&m_tex_info);
}

// ──────────────────────────────────────────────────────────────────────
// Adds the wall segment theP1 -> theP2 (their heights must differ: equal
// heights mark a free segment) to the first free one of the
// MAX_PRIVATE_COLLI that castWalls and the hitWalls tests read. Returns
// its id, -1 when all are used.
// ROR 0x00135e20 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

int MagScene::addPrivateColli(const float *theP1, const float *theP2)
{
    ROR_ASSERT(theP1); /* 2740 */
    ROR_ASSERT(theP2); /* 2741 */
    ROR_ASSERT(theP1[1] != theP2[1]); /* 2742 */
    for (int i = 0; i < MAX_PRIVATE_COLLI; i++)
    {
        MagSceneVecPair *w = &m_vec_pair[i];
        if (w->m_a[1] != w->m_b[1])
            continue;
        w->m_a[0] = theP1[0];
        w->m_a[1] = theP1[1];
        w->m_a[3] = 0.0f;
        w->m_a[2] = theP1[2];
        w->m_b[0] = theP2[0];
        w->m_b[1] = theP2[1];
        w->m_b[3] = 0.0f;
        w->m_b[2] = theP2[2];
        return i;
    }
    ROR_ASSERT(0); /* 2764 */
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// Frees wall segment theID (both end heights 0).
// ROR 0x00135f44 // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)

void MagScene::removePrivateColli(int theID)
{
    ROR_ASSERT(theID >= 0 && theID < MAX_PRIVATE_COLLI); /* 2773 */
    m_vec_pair[theID].m_a[1] = 0.0f;
    m_vec_pair[theID].m_b[1] = 0.0f;
}

// ──────────────────────────────────────────────────────────────────────
// After a map load (the last call of load): clears the y of both vectors
// of the 256 records at the scene's start, resets every actor's shadow
// light, and resets the PC, Brown and Brown's controller.
// ROR 0x00135fa4

void MagScene::resetEntities()
{
    for (int i = 0; i < 256; i++)
    {
        m_vec_pair[i].m_a[1] = 0.0f;
        m_vec_pair[i].m_b[1] = 0.0f;
    }
    for (int i = 0; i < 128; i++)
        m_actors[i].resetShadow();
    m_pc.reset();
    m_bwn.reset();
    m_bwn_ctrl.reset();
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
// The scene's constructor. g_magSystem holds the scene, so on the host it
// runs before main. The ROM also runs its members' constructors here (the
// light pool, the loaders, the sub mode, 128 DogActors, the sky models,
// the camera, the PC / Brown controllers, the fades, the 13 menus at
// +0x2be60.., the 2D layers, the scene interfaces); the host runs those
// that exist as C++ constructors (GamFont, DogEntity / DogCamera, S2d*),
// the rest start zeroed as part of g_magSystem until their TUs write them.
// ROR 0x001360a0
// callcheck: FUN_00100320, FUN_0010eebc, FUN_00110a94, FUN_00114c54, FUN_0011bf0c, FUN_00125aa4, FUN_0012ba80, FUN_0012d398, FUN_0012d58c, MagLifeMenuPBook, MagLifeMenuBoard3D, FUN_0012f008, FUN_0012f298, FUN_00130da4, FUN_0014257c, FUN_00142e84, FUN_00176a3c, FUN_0017c0f0, FUN_001947a4, FUN_0019a52c, FUN_001a1244, FUN_001a59b8, FUN_001a90ec, FUN_001ab474, FUN_001ad6b4, FUN_001adf04, FUN_001ae474, FUN_001ae954, FUN_001bdae4, FileReader, GamFont, S2dBg, S2dCineSco, S2dMes, S2dMovie, memset (member constructors: the host runs the C++ ones itself, the others are not written yet; the memsets are the field clears below)

MagScene::MagScene()
{
    for (int i = 0; i < 256; i++)
    {
        for (int k = 0; k < 4; k++)
        {
            float v = (k == 3) ? 1.0f : 0.0f;
            m_vec_pair[i].m_a[k] = v;
            m_vec_pair[i].m_b[k] = v;
        }
    }

    m_22a0 = 0x1.999998p-1f;    /* 0x3f4ccccc, 0.8 */
    m_22a4 = 1.03f;
    m_22a8 = 0x1.999998p-2f;    /* 0x3ecccccc, 0.4 */
    m_22b0 = 1.0f;
    m_22b4 = 1;
    m_22b8 = 0;
    m_22cc = 0;
    m_sub_mode.m_mode = 0;
    m_sub_mode.m_step = 0;
    m_sub_mode.m_arg = 0;
    m_sub_mode.m_arg2 = 0;
    memset(m_sub_mode.m_stack, 0, sizeof(m_sub_mode.m_stack));
    m_life_man.init();
    m_tex_info.clear();

    m_2a9c0 = 0;
    for (int k = 0; k < 4; k++)
    {
        float v = (k == 3) ? 1.0f : 0.0f;
        m_camera_pos[k] = v;
        m_camera_at[k] = v;
    }
    m_854e0 = NULL;
    m_854e8 = NULL;

    memset(&m_shadow_lightrec, 0, sizeof(m_shadow_lightrec));
    m_shadow_mode = 0;
    m_shadow_src = NULL;
    m_2048 = 0;
    m_204c = 0;
    m_shadow_mode = -1;
    m_sub_mode.m_step = -1;
    m_sub_mode.m_mode = 0;
    m_pc_no = 0;
    clearStrip(-1); /* inlined in the ROM */
    m_camera_2ad60 = 500.0f;
    m_camera_2ad5c = 600.0f;
    m_font_data = NULL;
    m_pMenuBase = NULL;
    m_camera_2ad64 = 0;
    m_camera_2ad68 = 0;
    m_camera_2ad80 = 0;
    for (int i = 0; i < 4; i++)
    {
        MagSceneTenkyu *t = &m_tenkyu[i];
        t->m_c_data = NULL;
        t->m_id = -1;
        t->m_c_num = 0;
        t->m_path_data = NULL;
        t->m_light_data = NULL;
        /* the ROM clears 0x10 bytes from m_tex_list */
        t->m_tex_list = NULL;
        t->m_data = NULL;
        memset(t->_50c, 0, 8);
    }
    initRecords853bc();
    m_854a4 = 0;
    m_focus_actor = -1;
    m_2a9c8 = 3;
    m_2aa84 = 1;
    m_mirror_axis = 0;
    for (int i = 0; i < 4; i++)
        m_22bc[i] = -1;
    m_854d5 = 1;
    m_854d4 = 1;
    m_854b0[0] = 0x1.999998p-4f; /* 0x3dcccccc, 0.1 */
    m_854b0[3] = 1.0f;
    for (int i = 0; i < 5; i++)
        m_854c0[i] = 0;
    m_854b0[2] = 0x1.999998p-4f;
    m_854b0[1] = 0x1.999998p-4f;
}

// ---- stubs (hand-written: constructors and destructors are not stubbed by stubgen) ----

// ──────────────────────────────────────────────────────────────────────
// The scene's destructor. The ROM's body: checkLightNo(-1),
// releaseTenkyuAll(), then its members' destructors in reverse order (the
// scene interfaces, the 2D layers, the menus, the sky models, the texture
// table, the 128 actors, the window fonts, MagLifeMan::deleteAll, the two
// loaders, the light pool). The host runs the member destructors itself.
// PORT: the explicit part is left out. The ROM never destroys the scene
// (g_magSystem lives for the whole run); on the host only std::exit's
// static teardown reaches it, with the game threads still running, and
// releasing the sky models there races their drawing (open-items.md
// Decisions).
// ROR 0x00136d14 // callcheck: checkLightNo, releaseTenkyuAll, FUN_00110b78, FUN_0012d5e0, FUN_0014259c, FUN_00176aa0, FUN_0017c14c, FUN_00194858, FUN_00196b2c, FUN_001987b0, FUN_0019a598, FUN_001a12a8, FUN_001a5a34, FUN_001a9150, FUN_001ab4e0, FUN_001ad72c, FUN_001adf3c, FUN_001ae4ac, FUN_001ae9f4, FUN_001bdb4c, deleteAll, operator_delete, release, term, ~FileReader, ~GamFont, ~S2dBg, ~S2dCineSco, ~S2dMes, ~S2dMovie (the explicit part left out on the host, see above; the member destructors run by the host)
MagScene::~MagScene()
{
}

// ---- stubs (tools/reverse/stubgen.py) ----

