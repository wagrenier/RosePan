// mag/maglifemenusp.cpp -- the scene's two dial locks (MagLifeMenuSpecial,
// MagLifeMenuSpecial2): models loaded as scene actors, a fixed camera, three
// dials the pad turns, and the box / body that opens on the right setting.

#include "mag/maglifemenusp.h"

#include "bed/bedpad.h"
#include "common/ror_assert.h"
#include "i3d/i3rmodel.h"
#include "mag/magscene.h"
#include "utl/utlgame.h"
#include "utl/utlrand.h"

/* The menus' motion speed (0x3c888897, a hair above 1/60). */
#define MENUSP_MOTION_SPEED 0x1.11112ep-6f

/* data 0x2ed090: MagLifeMenuSpecial's models in actor order: the box, the
 * three dials, the frame. */
static const int s_special_model[5] = { 0x10dd, 0x10d9, 0x10da, 0x10db, 0x10dc };

// ──────────────────────────────────────────────────────────────────────
// The MagLife0 / MagLife / MagLifeMenuBase words, no actors, dial 0
// picked, every dial at 0, then the reset the ROM inlines (step 0).
// ROR 0x0012e23c

MagLifeMenuSpecial::MagLifeMenuSpecial()
{
    m_menu_scene = NULL;
    m_sel = 0;
    m_result = 0;
    for (int i = 0; i < 5; i++)
        m_actor_no[i] = -1;
    m_type = -1;
    m_menu_scene = NULL;
    for (int i = 0; i < 3; i++)
    {
        m_cur[i] = 0;
        m_target[i] = 0;
    }
    m_04 = -1;
    m_state = 0;
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 1: releases the actors (term).
// ROR 0x0012e2e4 // callcheck: operator_delete (the deleting variant: delete does it on the host)

MagLifeMenuSpecial::~MagLifeMenuSpecial()
{
    term();
}

// ──────────────────────────────────────────────────────────────────────
// Loads the five models as scene actors that keep moving under the menu
// (actor flag 8) and saves the scene's camera; step 0.
// ROR 0x0012d9c8 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeMenuSpecial::init(DogTexInfo *tex)
{
    ROR_ASSERT(m_menu_scene); /* 47: the ROM's text is m_scene */
    for (int i = 0; i < 5; i++)
    {
        int no = m_menu_scene->loadModel(NULL, s_special_model[i], NULL);
        m_actor_no[i] = no;
        m_menu_scene->m_actor_flags[no] |= 8;
    }
    m_menu_scene->saveCamera();
    m_04 = -1;
    m_state = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Releases the actors init loaded and gives the scene its camera back.
// ROR 0x0012dac4 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeMenuSpecial::term()
{
    if (m_actor_no[0] < 0)
        return;
    ROR_ASSERT(m_menu_scene); /* 76: the ROM's text is m_scene */
    for (int i = 0; i < 5; i++)
    {
        m_menu_scene->releaseActor(m_actor_no[i]);
        m_actor_no[i] = -1;
    }
    m_menu_scene->restoreCamera();
}

// ROR 0x0012db54

int MagLifeMenuSpecial::isInit()
{
    return m_actor_no[4] >= 0;
}

// ──────────────────────────────────────────────────────────────────────
// Done: m_result. With the dials idle: pad bit 13 cancels (step 5), bit
// 14 opens when every dial stands at -0x4000 (step 4); then left / right
// pick a dial, up / down turn it an eighth (step 3); the picked dial's
// actor gets attention 0, the others -1.
// ROR 0x0012db64 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

int MagLifeMenuSpecial::processPad(BedPad *pad)
{
    if (m_state == 6)
        return m_result;
    if (m_state != 2)
        return 0;
    if (pad->m_repeat >> 13 & 1)
    {
        m_result = 1;
        m_04 = -1;
        m_state = 5;
    }
    else if (pad->m_repeat >> 14 & 1)
    {
        if (m_cur[0] == -0x4000 && m_cur[1] == -0x4000 && m_cur[2] == -0x4000)
        {
            m_04 = -1;
            m_state = 4;
        }
    }
    u_int dir = pad->m_dir_repeat;
    if (dir & 0x80)
    {
        if (--m_sel < 0)
            m_sel = 2;
    }
    else if (dir & 0x20)
    {
        if (++m_sel > 2)
            m_sel = 0;
    }
    else if (dir & (0x10 | 0x40))
    {
        if (dir & 0x10)
            m_target[m_sel] += 0x1000;
        else
            m_target[m_sel] -= 0x1000;
        if (m_target[m_sel] < -0x4000)
            m_target[m_sel] += 0x8000;
        if (m_target[m_sel] > 0x3fff)
            m_target[m_sel] -= 0x8000;
        m_04 = -1;
        m_state = 3;
    }
    for (int i = 0; i < 3; i++)
    {
        DogActor *actor = &m_menu_scene->m_actors[m_actor_no[i + 1]];
        ROR_ASSERT(actor); /* 171 */
        actor->setAttention(i == m_sel ? 0 : -1);
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Step 0 puts the camera at (0, -60, 200) turned (-0.1, pi, 0), the actors
// at the origin unrotated, hides the box and fades in over 30 frames;
// step 1 waits for the fade; step 3 eases each dial 0x100 a frame towards
// its target; step 4 shows only the box (and frame) and plays its motion
// 0 until it ends; step 5 fades out and back in. The actors draw with the
// scene.
// ROR 0x0012dda4 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeMenuSpecial::render(I3dLayer *theRenderStage)
{
    ROR_ASSERT(theRenderStage); /* 189 */
    if (!m_menu_scene)
        return;
    if (m_actor_no[4] < 0)
        return;
    DogActor *actor[5];
    for (int i = 0; i < 5; i++)
    {
        actor[i] = &m_menu_scene->m_actors[m_actor_no[i]];
        ROR_ASSERT(actor[i]); /* 202 */
    }
    switch (m_state)
    {
    case 0:
    {
        DogCamera *camera = &m_menu_scene->m_camera_entity;
        camera->m_trans[1] = -60.0f;
        camera->m_trans[2] = 200.0f;
        camera->m_dirty = 1;
        camera->m_trans[0] = 0.0f;
        camera->setRotate(-0x1.999998p-4f, DOG_PI, 0.0f); /* 0xbdcccccc, 0x40490fdc */
        for (int i = 0; i < 5; i++)
        {
            actor[i]->m_dirty = 1;
            actor[i]->m_trans[0] = 0.0f;
            actor[i]->m_trans[1] = 0.0f;
            actor[i]->m_trans[2] = 0.0f;
            actor[i]->setRotate(0.0f, 0.0f, 0.0f);
            actor[i]->transform(); /* EE slot 2 */
        }
        actor[0]->setHide(1); /* EE slot 8 */
        m_04 = -1;
        m_state = 1;
        m_menu_scene->fadeIn(30, 7, 0);
        return;
    }
    case 1:
        if (m_menu_scene->getFadeState() < 5)
            return;
        m_state = 2;
        m_04 = -1;
        break;
    case 2:
    case 6:
        break;
    case 3:
    {
        int moving = 0;
        for (int i = 0; i < 3; i++)
        {
            int d = m_target[i] - m_cur[i];
            if (d < -0x4000)
                d += 0x8000;
            if (d > 0x3fff)
                d -= 0x8000;
            if (d == 0)
            {
                if (m_cur[i] < -0x4000)
                    m_cur[i] += 0x8000;
                if (m_cur[i] > 0x3fff)
                    m_cur[i] -= 0x8000;
            }
            else
            {
                moving++;
                if (d > 0)
                    m_cur[i] += 0x100;
                if (d < 0)
                    m_cur[i] -= 0x100;
            }
            actor[i + 1]->setRotate((float)m_cur[i] * DOG_PI2 * 0x1p-15f, 0.0f, 0.0f);
        }
        if (moving)
            return;
        m_state = 2;
        m_04 = -1;
        break;
    }
    case 4:
        if (m_04 > 0)
        {
            if (actor[0]->isModeSet())
                return;
            m_state = 6;
            m_04 = -1;
            break;
        }
        m_result = 2;
        for (int i = 0; i < 4; i++)
            actor[i]->setHide(1); /* EE slot 8 */
        m_menu_scene->m_actor_flags[m_actor_no[0]] = 8;
        actor[0]->setHide(0); /* EE slot 8 */
        actor[0]->setMotion(MENUSP_MOTION_SPEED, 0, 0, -1, -1);
        m_04 = 1;
        break;
    case 5:
        if (m_04 < 1)
        {
            /* The ROM walks the three words from m_actor_no[4] (that, m_sel
             * and m_cur[0]) where MagLifeMenuSpecial2 walks the dials:
             * kept, but only for words that name a scene actor (the host
             * must not write outside m_actors). */
            const int *no = &m_actor_no[4];
            const int ror_words[3] = { no[0], m_sel, m_cur[0] };
            for (int i = 0; i < 3; i++)
            {
                if (ror_words[i] < 0 || ror_words[i] >= 128)
                    continue;
                DogActor *dial = &m_menu_scene->m_actors[ror_words[i]];
                ROR_ASSERT(dial); /* 303: the ROM's text is actor */
                dial->setAttention(-1);
            }
            m_menu_scene->fadeOut(30, 7, 0);
            m_04 = 1;
        }
        if (m_menu_scene->getFadeState() < 5)
            return;
        m_04 = -1;
        m_state = 6;
        m_menu_scene->fadeIn(30, 7, 0);
        break;
    default:
        break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The MagLife0 / MagLife / MagLifeMenuBase words, no actors, dial 0
// picked, every dial at 0, then the reset the ROM inlines (step 0).
// ROR 0x0012eef0

MagLifeMenuSpecial2::MagLifeMenuSpecial2()
{
    m_menu_scene = NULL;
    m_sel = 0;
    m_result = 0;
    for (int i = 0; i < 4; i++)
        m_actor_no[i] = -1;
    m_type = -1;
    m_menu_scene = NULL;
    for (int i = 0; i < 3; i++)
    {
        m_cur[i] = 0;
        m_target[i] = 0;
    }
    m_04 = -1;
    m_state = 0;
}

// ──────────────────────────────────────────────────────────────────────
// EE slot 1: releases the actors (term).
// ROR 0x0012efa0 // callcheck: operator_delete (the deleting variant: delete does it on the host)

MagLifeMenuSpecial2::~MagLifeMenuSpecial2()
{
    term();
}

// ──────────────────────────────────────────────────────────────────────
// Loads m_type's body and the three dials as scene actors that keep moving
// under the menu (actor flag 8) and saves the scene's camera; step 0.
// ROR 0x0012e34c // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeMenuSpecial2::init(DogTexInfo *tex)
{
    ROR_ASSERT(m_menu_scene); /* 392: the ROM's text is m_scene */
    /* The ROM's stack tables: the body by m_type (0..3), the dials. */
    const short body[4] = { 0x10e1, 0x10e2, 0x10e3, 0x10e4 };
    const short dial[3] = { 0x10de, 0x10df, 0x10e0 };
    int no = m_menu_scene->loadModel(NULL, body[m_type], NULL);
    m_actor_no[0] = no;
    m_menu_scene->m_actor_flags[no] |= 8;
    for (int i = 0; i < 3; i++)
    {
        no = m_menu_scene->loadModel(NULL, dial[i], NULL);
        m_actor_no[i + 1] = no;
        m_menu_scene->m_actor_flags[no] |= 8;
    }
    m_menu_scene->saveCamera();
    m_04 = -1;
    m_state = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Releases the actors init loaded, last first, and gives the scene its
// camera back.
// ROR 0x0012e498 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeMenuSpecial2::term()
{
    if (m_actor_no[0] < 0)
        return;
    ROR_ASSERT(m_menu_scene); /* 430: the ROM's text is m_scene */
    for (int i = 3; i >= 0; i--)
    {
        m_menu_scene->releaseActor(m_actor_no[i]);
        m_actor_no[i] = -1;
    }
    m_menu_scene->restoreCamera();
}

// ROR 0x0012e528

int MagLifeMenuSpecial2::isInit()
{
    return m_actor_no[3] >= 0;
}

// ──────────────────────────────────────────────────────────────────────
// Done: m_result. With the dials idle: pad bit 13 cancels (step 7), bit
// 14 tries the code m_menu_arg against the dials (step 5, m_result 2 when
// right, 0 when not; sound 0xc2); then left / right pick a dial, up /
// down turn it a digit (step 4, sound 0xc1); the picked dial's actor gets
// attention 1, the others -1.
// ROR 0x0012e538 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

int MagLifeMenuSpecial2::processPad(BedPad *pad)
{
    if (m_state == 8)
        return m_result;
    if (m_state != 3)
        return 0;
    if (pad->m_repeat >> 13 & 1)
    {
        m_result = 1;
        m_04 = -1;
        m_state = 7;
    }
    else if (pad->m_repeat >> 14 & 1)
    {
        int code = m_menu_arg;
        int d2 = (code % 10) * -0x1000;
        int d0 = (code / 100 % 10) * -0x1000;
        if (d2 < -0x5000)
            d2 += 0xa000;
        if (d0 < -0x5000)
            d0 += 0xa000;
        int d1 = (code / 10 % 10) * -0x1000;
        if (d1 < -0x5000)
            d1 += 0xa000;
        if (m_cur[0] == d0 && m_cur[1] == d1 && m_cur[2] == d2)
        {
            m_result = 2;
            m_04 = -1;
            m_state = 5;
        }
        else
        {
            m_04 = -1;
            m_state = 5;
            m_result = 0;
        }
        utlPlaySe(0xc2, 0, NULL);
    }
    u_int dir = pad->m_dir_repeat;
    int turned = 0;
    if (dir & 0x80)
    {
        if (--m_sel < 0)
            m_sel = 2;
    }
    else if (dir & 0x20)
    {
        if (++m_sel > 2)
            m_sel = 0;
    }
    else if (dir & (0x10 | 0x40))
    {
        if (dir & 0x10)
            m_target[m_sel] += 0x1000;
        else
            m_target[m_sel] -= 0x1000;
        if (m_target[m_sel] < -0x5000)
            m_target[m_sel] += 0xa000;
        if (m_target[m_sel] > 0x4fff)
            m_target[m_sel] -= 0xa000;
        m_04 = -1;
        turned = 1;
        m_state = 4;
    }
    for (int i = 0; i < 3; i++)
    {
        DogActor *actor = &m_menu_scene->m_actors[m_actor_no[i + 1]];
        ROR_ASSERT(actor); /* 543 */
        actor->setAttention(i == m_sel ? 1 : -1);
    }
    if (turned)
        utlPlaySe(0xc1, 0, NULL);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Step 0 puts the camera at (0, -20, 80) turned (-0.1, pi, 0), the actors
// at the origin unrotated, and fades in over 30 frames; step 1 waits for
// the fade; step 2 moves the camera in and down onto the body over 30
// frames by its clip radius; step 4 eases each dial 0x100 a frame towards
// its target; step 5 shakes the actors for 20 frames, holds them 20, then
// plays sound 0xc3 and opens (step 6) or 0xc4 and lets the dials turn
// again (step 3); step 6 plays the body's motion 1 until it ends; step 7
// fades out and back in. The actors draw with the scene.
// ROR 0x0012e854 // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)

void MagLifeMenuSpecial2::render(I3dLayer *theRenderStage)
{
    ROR_ASSERT(theRenderStage); /* 567 */
    if (!m_menu_scene)
        return;
    if (m_actor_no[3] < 0)
        return;
    DogActor *actor[4];
    for (int i = 0; i < 4; i++)
    {
        actor[i] = &m_menu_scene->m_actors[m_actor_no[i]];
        ROR_ASSERT(actor[i]); /* 580 */
    }
    switch (m_state)
    {
    case 0:
    {
        DogCamera *camera = &m_menu_scene->m_camera_entity;
        camera->m_trans[1] = -20.0f;
        camera->m_trans[2] = 80.0f;
        camera->m_dirty = 1;
        camera->m_trans[0] = 0.0f;
        camera->setRotate(-0x1.999998p-4f, DOG_PI, 0.0f); /* 0xbdcccccc, 0x40490fdc */
        for (int i = 0; i < 4; i++)
        {
            actor[i]->m_dirty = 1;
            actor[i]->m_trans[0] = 0.0f;
            actor[i]->m_trans[1] = 0.0f;
            actor[i]->m_trans[2] = 0.0f;
            actor[i]->setRotate(0.0f, 0.0f, 0.0f);
            actor[i]->transform(); /* EE slot 2 */
        }
        m_state = 1;
        m_04 = -1;
        m_menu_scene->fadeIn(30, 7, 0);
        return;
    }
    case 1:
        if (m_menu_scene->getFadeState() < 5)
            return;
        m_state = 2;
        m_04 = -1;
        return;
    case 2:
    {
        float center[4];
        float radius;
        i3rCopyClipSphere(m_menu_scene->m_actors[m_actor_no[0]].m_i3r_root, center, &radius);
        int t = m_04;
        if (t < 0)
            t = 0;
        float f = (float)t;
        DogCamera *camera = &m_menu_scene->m_camera_entity;
        camera->m_dirty = 1;
        camera->m_trans[0] = 0.0f;
        camera->m_trans[2] = 80.0f - f * (70.0f - radius) / 30.0f;
        camera->m_trans[1] = f * (60.0f - radius) * 0.25f / 30.0f + -20.0f;
        camera->setRotate(-0x1.999998p-4f - f * (60.0f - radius) / 300.0f / 30.0f, DOG_PI, 0.0f);
        if (t < 31)
            return;
        m_state = 3;
        m_04 = -1;
        return;
    }
    case 3:
    case 8:
        return;
    case 4:
    {
        int moving = 0;
        for (int i = 0; i < 3; i++)
        {
            int d = m_target[i] - m_cur[i];
            if (d < -0x5000)
                d += 0xa000;
            if (d > 0x4fff)
                d -= 0xa000;
            if (d == 0)
            {
                if (m_cur[i] < -0x5000)
                    m_cur[i] += 0xa000;
                if (m_cur[i] > 0x4fff)
                    m_cur[i] -= 0xa000;
            }
            else
            {
                moving++;
                if (d > 0)
                    m_cur[i] += 0x100;
                if (d < 0)
                    m_cur[i] -= 0x100;
            }
            actor[i + 1]->setRotate((float)m_cur[i] * DOG_PI2 / 40960.0f, 0.0f, 0.0f);
        }
        if (moving)
            return;
        m_state = 3;
        m_04 = -1;
        return;
    }
    case 5:
        if (m_04 < 20)
        {
            float x = (float)(utlRandRange(0, 10) - 5) * 0.08f;
            float y = (float)(utlRandRange(0, 10) - 5) * 0.08f;
            for (int i = 0; i < 4; i++)
            {
                DogActor *a = &m_menu_scene->m_actors[m_actor_no[i]];
                a->m_dirty = 1;
                a->m_trans[0] = x;
                a->m_trans[1] = y;
                a->m_trans[2] = 0.0f;
            }
            return;
        }
        if (m_04 < 40)
        {
            for (int i = 0; i < 4; i++)
            {
                DogActor *a = &m_menu_scene->m_actors[m_actor_no[i]];
                a->m_dirty = 1;
                a->m_trans[2] = 0.0f;
                a->m_trans[0] = 0.0f;
                a->m_trans[1] = 0.0f;
            }
            return;
        }
        if (m_result == 0)
        {
            utlPlaySe(0xc4, 0, NULL);
            m_state = 3;
        }
        else
        {
            utlPlaySe(0xc3, 0, NULL);
            m_state = 6;
        }
        m_04 = -1;
        return;
    case 6:
        if (m_04 > 0)
        {
            if (actor[0]->isModeSet())
                return;
            m_state = 8;
            m_04 = -1;
            return;
        }
        m_result = 2;
        actor[0]->setParam1d8(0);
        actor[0]->setMotion(MENUSP_MOTION_SPEED, 1, 0, -1, -1);
        m_04 = 1;
        return;
    case 7:
        if (m_04 < 1)
        {
            for (int i = 0; i < 3; i++)
            {
                DogActor *dial = &m_menu_scene->m_actors[m_actor_no[i + 1]];
                ROR_ASSERT(dial); /* 737: the ROM's text is actor */
                dial->setAttention(-1);
            }
            m_menu_scene->fadeOut(30, 7, 0);
            m_04 = 1;
        }
        if (m_menu_scene->getFadeState() < 5)
            return;
        m_04 = -1;
        m_state = 8;
        m_menu_scene->fadeIn(30, 7, 0);
        return;
    default:
        return;
    }
}
