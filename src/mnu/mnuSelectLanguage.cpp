/* ==========================================================================
 *  mnu/mnuSelectLanguage.cpp
 *
 *  The language select screen (0x1bdae4..0x1be2cc). See
 *  mnu/mnuSelectLanguage.h.
 *
 *  PORT: the pattern table is the ROM's .data words as they are (an EE
 *  image, as in mnu/mnunow.cpp): the display list and the anim tables
 *  point into it by EE address, SprMan::eePtr resolves them against
 *  MNUSLLGPATTERN_EE. Writable: SprMan writes the anim counts back.
 * ======================================================================== */
#include "mnu/mnuSelectLanguage.h"

#include <stddef.h>

#include "bed/bedpad.h"
#include "common/ror_assert.h"
#include "game/gamman.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "snd/sndman.h"

/* ---- data ---- */

/* The screen's texture in data/mnurpk/mnusllg.rpk (data 0x2b89e0). */
static SprTexFile mnuSlLgTexList[2] = {
    { 0, "location.tm2", "sellang_bg_tcc", 0x8800, 1, NULL },  // 0x002b89e0
    { -1, "", "", 0, 0, NULL },                                // 0x002b89f8
};

static const char mnuSlLgRpk[] = "data/mnurpk/mnusllg.rpk";    /* rodata 0x2fb380 */

/* The screen's pattern table (data 0x2b8a10..0x2b8bf0): EE image, see the
 * file comment. Pattern 1 is the whole 512 x 448 background, frames from
 * 0x2b8a38, anim tables 0x2b8b70 (sprite 0) and 0x2b8b98 (sprite 1:
 * pattern 1, 30 frames); four RGBA colours from 0x2b8bb0 that no code of
 * the file reads. */
static u_int mnuSlLgPattern[120] = {
    0xffffffff, 0x00000000, 0x00000000, 0x00000100, 0x00000100, 0x00000000, 0x00000000, 0x00000000,  // 0x002b8a10
    0x00002000, 0x00001c00, 0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002b8a30
    0x00000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002b8a50
    0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005,  // 0x002b8a70
    0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0xffffffff,  // 0x002b8a90
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0xffffffff, 0x00000000,  // 0x002b8ab0
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005, 0xffffffff, 0x00000000, 0x00000000,  // 0x002b8ad0
    0x00000000, 0x00000000, 0x00000000, 0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000,  // 0x002b8af0
    0x00000000, 0x00000000, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002b8b10
    0x00000000, 0x00000000, 0x0000001e, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0xfffffff0,  // 0x002b8b30
    0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002b8b50
    0x00000000, 0xffffffff, 0x002b8a38, 0x00000000, 0xffffffff, 0x002b8a70, 0xffffffff, 0xffffffff,  // 0x002b8b70
    0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x002b8b38, 0xffffffff, 0xffffffff, 0x00000000,  // 0x002b8b90
    0x00000080, 0x00000080, 0x00000080, 0x00000080, 0x00000080, 0x00000080, 0x00000080, 0x00000000,  // 0x002b8bb0
    0x00000080, 0x00000080, 0x00000080, 0x00000028, 0x00000080, 0x00000080, 0x00000080, 0x00000080,  // 0x002b8bd0
};
#define MNUSLLGPATTERN_EE 0x002b8a10u

/* The display list (data 0x2b8bf0): sprite 0 off, sprite 1 the background
 * at (0, 0). */
static const SprDispDef mnuSlLgDisp[3] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 16000 }, 0x002b8b70u },        // 0x002b8bf0
    { 1, 0, 0, 0, 0, 1, 0, 1, 0, { 0, 0, 16016 }, 0x002b8b98u },        // 0x002b8c24
    { -1, 0, 0, 0, 0, 1, 0, 1, 0, { 0, 0, 0 }, 0x002b8b70u },           // 0x002b8c58
};

/* The language list, one text per cursor position: the cursor's language
 * in colour 15, the others in 8 (data 0x2b8c90). */
static const char *const mnuSlLgMessage[5] = {
    "#L#c15English#n#n#c8French#n#n#c8German#n#n#c8Spanish#n#n#c8Italian##",  // 0x002fb1c8
    "#L#c8English#n#n#c15French#n#n#c8German#n#n#c8Spanish#n#n#c8Italian##",  // 0x002fb210
    "#L#c8English#n#n#c8French#n#n#c15German#n#n#c8Spanish#n#n#c8Italian##",  // 0x002fb258
    "#L#c8English#n#n#c8French#n#n#c8German#n#n#c15Spanish#n#n#c8Italian##",  // 0x002fb2a0
    "#L#c8English#n#n#c8French#n#n#c8German#n#n#c8Spanish#n#n#c15Italian##",  // 0x002fb2e8
};

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001bdae4; callcheck: MsgMan, SprMan (member constructors: implicit on the host)

MnuSelectLanguage::MnuSelectLanguage()
{
    m_open = 0;
    m_init = 0;
    initWork();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001bdb4c; callcheck: operator_delete, ~SprMan, ~MsgMan (member destructors and the deleting variant: implicit on the host)

MnuSelectLanguage::~MnuSelectLanguage()
{
    close();
    if (m_init != 0)
        term();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001bdc00

void MnuSelectLanguage::init(DogTexInfo *tex)
{
    g_magSystem.m_scene.m_sepia = 0;
    m_spr.init(2);
    m_spr.m_tex.m_tex_info = tex;
    open();
    m_init = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001bdc54

void MnuSelectLanguage::term()
{
    if (m_init != 0) {
        m_spr.release();
        close();
        m_init = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001bdc94

int MnuSelectLanguage::isInit()
{
    return m_init;
}

// ──────────────────────────────────────────────────────────────────────
// A 512 x 256 window at (220, 120), hidden until the list is shown.
// ROR 0x001bdc9c

void MnuSelectLanguage::openWindow()
{
    ROR_ASSERT(m_pScene);       /* 160 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    m_msg.open();
    m_window = m_pScene->openWindow(0x200, 0x100, 0, -1);
    m_pScene->updateWindowFont(m_window);
    m_pScene->setWindowPos(m_window, 220, 120, 0x7fff);
    m_pScene->setWindowSize(m_window, 0x200, 0x100);
    m_pScene->setWindowShow(m_window, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001bdd48

void MnuSelectLanguage::closeWindow()
{
    ROR_ASSERT(m_pScene);       /* 181 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    if (m_window >= 0) {
        m_pScene->updateWindowFont(m_window);
        m_pScene->closeWindow(m_window);
        m_window = -1;
    }
    m_msg.close();
}

// ──────────────────────────────────────────────────────────────────────
// Every frame: the list text again (the running one closed first).
// ROR 0x001bddbc

void MnuSelectLanguage::updateMessage()
{
    ROR_ASSERT(m_pScene);       /* 204 */  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
    if (m_msg.getState(m_window) != 0)
        m_msg.close();
    m_msg.open();
    m_msg.setMessage(m_window, mnuSlLgMessage[m_language], 0);
    m_msg.update();
}

// ──────────────────────────────────────────────────────────────────────
// The first frame saves the volume and sets the brightness option to its
// default, the next loads the texture, then the sprites show and execMain
// runs until it returns 1; then the volume and the player's brightness
// come back and -1 says the screen is done.
// ROR 0x001bde58

int MnuSelectLanguage::processPad(BedPad *pad)
{
    if (m_open == 0)
        return -1;
    switch (m_step) {
    case 0:
        m_spr.saveVolume();
        SprMan::volumeOff();
        g_gamMan.setOption(0, 4);
        g_gamMan.m_sndMan.resetVolume();
        m_step = 1;
        return 0;
    case 1:
        loadTexture();
        m_step = 2;
        return 0;
    case 2:
        setDisp();
        m_step = 3;
        /* fall through */
    case 3:
        g_gamMan.setOption(0, 4);
        m_step = 4;
        /* fall through */
    case 4:
        if (execMain(pad) == 1)
            m_step = 5;
        m_spr.update();
        return 0;
    case 5:
        m_step = 6;
        /* fall through */
    case 6:
    default:
        break;
    }
    m_spr.m_active = 0;
    m_main_step = 0;
    m_step = 0;
    m_spr.restoreVolume();
    g_bss_7a3d4c = 1;
    g_bss_7a3d48 = 1;
    u_int bright;
    g_gamMan.getStatus(0x10, &bright);
    g_gamMan.setOption(0, (int)bright);
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001bdfe0

void MnuSelectLanguage::draw(I3dLayer *layer)
{
    if (layer != NULL) {
        m_spr.m_layer[0] = layer;
        m_spr.draw();
    }
}

// ──────────────────────────────────────────────────────────────────────
// Loads mnusllg.rpk's texture at once.
// ROR 0x001be004

void MnuSelectLanguage::loadTexture()
{
    m_spr.m_tex.setList(mnuSlLgTexList, mnuSlLgRpk);
    while (m_spr.m_tex.update() == 0)
        ;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001be048

int MnuSelectLanguage::open()
{
    if (m_open == 1)
        return -1;
    m_open = 1;
    m_main_step = 0;
    m_step = 0;
    openWindow();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001be084

int MnuSelectLanguage::close()
{
    if (m_open == 0)
        return -1;
    closeWindow();
    m_open = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001be0c0

int MnuSelectLanguage::initWork()
{
    m_language = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001be0cc

void MnuSelectLanguage::setDisp()
{
    m_spr.m_pattern = mnuSlLgPattern;
    m_spr.m_pattern_ee = MNUSLLGPATTERN_EE;     // PC
    m_spr.setDispList(mnuSlLgDisp);
    m_spr.m_active = 1;
}

// ──────────────────────────────────────────────────────────────────────
// Steps 0 / 1: up / down move the cursor (sound 1), cross takes the
// language (sound 4) and asks for the close; the list is shown again
// every frame. 2 fades the scene out, 3 waits for it, then sets the
// background's state to 0 and returns 1.
// ROR 0x001be110

int MnuSelectLanguage::execMain(BedPad *pad)
{
    switch (m_main_step) {
    case 0:
        m_main_step = 1;
        /* fall through */
    case 1:
        break;
    case 2:
        g_magSystem.m_scene.fadeOut(7, 7, 0);
        m_pScene->updateWindowFont(m_window);
        m_main_step = 3;
        return 0;
    case 3:
        if (g_magSystem.m_scene.getFadeState() < 5)
            return 0;
        m_spr.m_sprite[1].m_state = 0;
        m_main_step = 0;
        return 1;
    default:
        return 0;
    }
    if (pad->m_dir_repeat & 0x10) {             /* up */
        if (m_language > 0)
            m_language--;
        g_sndMan.playSe(1);
        m_pScene->updateWindowFont(m_window);
    } else if (pad->m_dir_repeat & 0x40) {      /* down */
        if (m_language < 4)
            m_language++;
        g_sndMan.playSe(1);
        m_pScene->updateWindowFont(m_window);
    }
    if (pad->m_trig & 0x4000) {                 /* cross */
        g_magSystem.setLanguage(m_language);
        g_sndMan.playSe(4);
        m_main_step = 2;
    }
    updateMessage();
    return 0;
}
