/* ==========================================================================
 *  mnu/mnunow.cpp
 *
 *  The "now loading" screen (0x1ae474..0x1ae954; no source string names
 *  the file). See mnu/mnunow.h.
 *
 *  PORT: the pattern table is the ROM's .data words as they are (an EE
 *  image, as in mnu/mnusvld.cpp): the display list and the anim tables
 *  point into it by EE address, SprMan::eePtr resolves them against
 *  MNUNOWPATTERN_EE. Writable: SprMan writes the anim counts back.
 * ======================================================================== */
#include "mnu/mnunow.h"

#include <stddef.h>

#include "bed/bedpad.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"

/* ---- data ---- */

/* The screen's texture in data/mnurpk/mnunow.rpk (data 0x2ac788). */
static SprTexFile mnuNowTexList[2] = {
    { 0, "loading.tm2", "nowloading_tcc", 0x10378, 1, NULL },  // 0x002ac788
    { -1, "", "", 0, 0, NULL },                                // 0x002ac7a0
};

static const char mnuNowRpk[] = "data/mnurpk/mnunow.rpk";      /* rodata 0x2facc8 */

/* The screen's pattern table (data 0x2ac7b8..0x2ac980): EE image, see
 * the file comment. Patterns 0 (none) and 1..4 (the four 256 x 64 rows of
 * texture 0), frames from 0x2ac820, anim tables 0x2ac940 (sprite 0) and
 * 0x2ac968 (sprite 1: patterns 1..4, six frames each, looping). */
static u_int mnuNowPattern[114] = {
    0xffffffff, 0x00000000, 0x00000000, 0x00000100, 0x00000100, 0x00000000, 0x00000000, 0x00000000,  // 0x002ac7b8
    0x00001000, 0x00000400, 0x00000000, 0x00000000, 0x00000400, 0x00001000, 0x00000400, 0x00000000,  // 0x002ac7d8
    0x00000000, 0x00000800, 0x00001000, 0x00000400, 0x00000000, 0x00000000, 0x00000c00, 0x00001000,  // 0x002ac7f8
    0x00000400, 0x00000000, 0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002ac818
    0x00000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002ac838
    0x00000005, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000005,  // 0x002ac858
    0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x03000000, 0x00000000,  // 0x002ac878
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000006, 0x00000001,  // 0x002ac898
    0x00000000, 0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000006, 0x00000002, 0x00000000,  // 0x002ac8b8
    0x00000000, 0x00000000, 0xfffffff0, 0xfffffff0, 0x00000006, 0x00000003, 0x00000000, 0x00000000,  // 0x002ac8d8
    0x00000000, 0xfffffff0, 0xfffffff0, 0x00000006, 0x00000004, 0x00000000, 0x00000000, 0x00000000,  // 0x002ac8f8
    0xfffffff0, 0xfffffff0, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,  // 0x002ac918
    0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x002ac820, 0x00000000, 0xffffffff, 0x002ac858,  // 0x002ac938
    0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x002ac8b0, 0xffffffff,  // 0x002ac958
    0xffffffff, 0x00000000,  // 0x002ac978
};
#define MNUNOWPATTERN_EE 0x002ac7b8u

/* The display list (data 0x2ac980): sprite 0 off, sprite 1 the "now
 * loading" animation at (32, 370). */
static const SprDispDef mnuNowDisp[3] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 16000 }, 0x002ac940u },        // 0x002ac980
    { 1, 0, 0, 0, 0, 1, 0, 1, 0, { 512, 5920, 16160 }, 0x002ac968u },   // 0x002ac9b4
    { -1, 0, 0, 0, 0, 1, 0, 1, 0, { 0, 0, 0 }, 0x002ac940u },           // 0x002ac9e8
};

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae474; callcheck: SprMan (member constructor: implicit on the host)

MnuNow::MnuNow()
{
    m_open = 0;
    initWork();
    m_init = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae4ac; callcheck: operator_delete, ~SprMan (member destructor and the deleting variant: implicit on the host)

MnuNow::~MnuNow()
{
    close();
    if (m_init == 1)
        term();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae520

void MnuNow::init(DogTexInfo *tex)
{
    m_spr.init(2);
    m_spr.m_tex.m_tex_info = tex;
    open();
    loadTexture();
    m_init = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae574

void MnuNow::term()
{
    if (m_init != 0) {
        m_spr.release();
        close();
        m_init = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae5b4

int MnuNow::isInit()
{
    return m_init;
}

// ──────────────────────────────────────────────────────────────────────
// Loads mnunow.rpk's texture at once.
// ROR 0x001ae5bc

void MnuNow::loadTexture()
{
    m_spr.m_tex.setList(mnuNowTexList, mnuNowRpk);
    while (m_spr.m_tex.update() == 0)
        ;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae600

int MnuNow::processPad(BedPad *pad)
{
    return exec(pad);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae614

void MnuNow::draw(I3dLayer *layer)
{
    m_layer = layer;
    if (layer != NULL) {
        m_spr.m_layer[0] = layer;
        m_spr.draw();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae640

int MnuNow::open()
{
    if (m_open == 1)
        return -1;
    m_open = 1;
    m_step = 0;
    m_pause = 0;
    m_10 = 2;
    m_main_step = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae674

int MnuNow::close()
{
    if (m_open == 0)
        return -1;
    m_open = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// One frame (processPad): the first frame sets the sprites up and fades
// the scene in, 4 runs execMain (unless paused) and the sprites until
// execMain returns 1, 6 fades out; then 1 once the fade has ended (the
// steps back at 0), 0 before. -1 before open.
// ROR 0x001ae6a0

int MnuNow::exec(BedPad *pad)
{
    if (m_open == 0)
        return -1;
    switch (m_step) {
    case 0:
        m_step = 3;
        /* fall through */
    case 3:
        m_step = 4;
        setDisp();
        m_system->m_scene.fadeIn(4, 7, 0);
        return 0;
    case 4:
        if (m_pause == 0 && execMain(pad) == 1)
            m_step = 6;
        update(pad);
        return 0;
    case 6:
        m_system->m_scene.fadeOut(4, 7, 0);
        m_step = 7;
        /* fall through */
    case 1:
    case 2:
    case 5:
    case 7:
    default:
        break;
    }
    int done = m_system->m_scene.getFadeState() > 4;
    if (done) {
        m_main_step = 0;
        m_step = 0;
    }
    return done;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae7f0

int MnuNow::initWork()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae7f8

void MnuNow::setDisp()
{
    m_system = MagSystem::m_current_system;
    m_spr.m_pattern = mnuNowPattern;
    m_spr.m_pattern_ee = MNUNOWPATTERN_EE;     // PC
    m_spr.setDispList(mnuNowDisp);
    m_spr.m_active = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae848

int MnuNow::update(BedPad *pad)
{
    (void)pad;
    m_spr.update();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Step 0 clears the result, 1 waits; circle (in any step) asks for the
// close: 4 fades the scene out, 5 waits for it, then resets the double
// buffer and returns m_result (1).
// ROR 0x001ae868

int MnuNow::execMain(BedPad *pad)
{
    int ret = 0;
    switch (m_main_step) {
    case 0:
        m_result = 0;
        m_main_step = 1;
        break;
    case 4:
        m_system->m_scene.fadeOut(3, 7, 0);
        m_main_step = 5;
        /* fall through */
    case 5:
        if (m_system->m_scene.getFadeState() < 5)
            break;
        m_system->resetDBuff();
        ret = m_result;
        break;
    default:
        break;
    }
    if (pad->m_trig & 0x2000) {                 /* circle */
        m_main_step = 4;
        m_result = 1;
    }
    return ret;
}
