/* ==========================================================================
 *  mnu/mnumovie.cpp
 *
 *  The movie theatre's player (0x1adf04..0x1ae474; no source string names
 *  the file). See mnu/mnumovie.h.
 * ======================================================================== */
#include "mnu/mnumovie.h"

#include <stddef.h>

#include "bed/bedpad.h"
#include "i3d/i3dlayer.h"
#include "i3d/i3dpacket.h"
#include "mag/magsystem.h"
#include "snd/rxmpeg.h"

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001adf04; callcheck: SprMan (member constructor: implicit on the host)

MnuMovie::MnuMovie()
{
    m_open = 0;
    initWork();
    m_init = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001adf3c; callcheck: operator_delete, ~SprMan (member destructor and the deleting variant: implicit on the host)

MnuMovie::~MnuMovie()
{
    close();
    if (m_init == 1)
        term();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001adfb0

void MnuMovie::init(DogTexInfo *tex)
{
    (void)tex;
    open();
    prepare();
    m_init = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001adfe4

void MnuMovie::term()
{
    if (m_init != 0) {
        close();
        m_init = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae01c

int MnuMovie::isInit()
{
    return m_init;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae024

void MnuMovie::prepare()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae02c

int MnuMovie::processPad(BedPad *pad)
{
    return exec(pad);
}

// ──────────────────────────────────────────────────────────────────────
// The movie's picture (while one plays), then two black bars over the
// top (rows 32..96) and bottom (416..480) of the 512-wide screen.
// ROR 0x001ae040

void MnuMovie::draw(I3dLayer *layer)
{
    m_layer = layer;
    if (layer == NULL)
        return;
    if (g_rxMpeg.m_playing == 1)
        g_rxMpeg.draw(layer);
    u_long *pkt = i3dLayerBeginPostPacket(layer);
    pkt = i3dPktSetReg(pkt, I3D_GS_ALPHA_1, 0x44);
    u_long xyz0 = i3dPackXYZ(0x7000, 0x7200, 0x80000);
    u_long xyz1 = i3dPackXYZ(0x9000, 0x7600, 0x80000);
    u_long rgbaq = i3dPackRGBAQ(1.0f, 0, 0, 0, 0x80);
    pkt = i3dPktSprite(pkt, xyz0, xyz1, rgbaq);
    xyz0 = i3dPackXYZ(0x7000, 0x8a00, 0x80000);
    xyz1 = i3dPackXYZ(0x9000, 0x8e00, 0x80000);
    rgbaq = i3dPackRGBAQ(1.0f, 0, 0, 0, 0x80);
    pkt = i3dPktSprite(pkt, xyz0, xyz1, rgbaq);
    i3dLayerEndPostPacket(layer, pkt);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae188

int MnuMovie::open()
{
    if (m_open == 1)
        return -1;
    m_open = 1;
    m_step = 0;
    m_pause = 0;
    m_50 = 2;
    m_main_step = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae1bc

int MnuMovie::close()
{
    if (m_open == 0)
        return -1;
    m_open = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// One frame (processPad): steps 0..3 fall through to setDisp in the first
// frame, 4 runs execMain (unless paused) until it returns 1, 6 / 7 end. 1
// when done (the step back at 0), -1 before open.
// ROR 0x001ae1e8

int MnuMovie::exec(BedPad *pad)
{
    if (m_open == 0)
        return -1;
    switch (m_step) {
    case 0:
        m_step = 1;
        /* fall through */
    case 1:
        m_step = 2;
        /* fall through */
    case 2:
        m_step = 3;
        setDisp();
        /* fall through */
    case 3:
        m_step = 4;
        /* fall through */
    case 4:
        if (m_pause == 0 && execMain(pad) == 1)
            m_step = 6;
        update(pad);
        return 0;
    case 6:
        m_step = 7;
        /* fall through */
    case 5:
    case 7:
    default:
        m_main_step = 0;
        break;
    }
    m_step = 0;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae2d8

int MnuMovie::initWork()
{
    for (int i = 0; i < 7; i++)
        m_work[i] = 0;
    m_28 = 2;
    m_20 = 5;
    m_24 = 2;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae318

void MnuMovie::setDisp()
{
    m_system = MagSystem::m_current_system;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ae328

int MnuMovie::update(BedPad *pad)
{
    (void)pad;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Step 0 starts g_rxMpeg's named movie (512 x 384, at once) at 60 fps; 3
// waits for its end or circle / cross; 9 / 10 close it, put the frame
// rate back to 30 fps and return m_result (1).
// ROR 0x001ae330

int MnuMovie::execMain(BedPad *pad)
{
    switch (m_main_step) {
    case 0:
        m_result = 0;
        m_main_step = 3;
        g_rxMpeg.open();
        g_rxMpeg.playNow(g_rxMpeg.m_name, 0x200, 0x180);
        m_main_step = 3;
        MagSystem::m_v_interval = 1;
        return 0;
    case 3:
        if (g_rxMpeg.getStatus() == 1) {
            m_result = 1;
            m_main_step = 9;
        }
        if (pad->m_trig & 0x4000) {            /* cross */
            m_result = 1;
            m_main_step = 9;
        }
        if (pad->m_trig & 0x2000) {            /* circle */
            m_result = 1;
            m_main_step = 9;
        }
        return 0;
    case 9:
        m_main_step = 10;
        /* fall through */
    case 10:
        g_rxMpeg.close();
        MagSystem::m_v_interval = 2;
        return m_result;
    default:
        return 0;
    }
}
