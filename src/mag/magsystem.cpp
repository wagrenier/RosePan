/* ==========================================================================
 *  mag/magsystem.cpp
 *
 *  MagSystem, the system object (one global, bss 0x6e9840): it brings up the
 *  graphics (VIF1 channel, GS reset, a 512x448 double buffer, the i3d draw
 *  context over a static 3.4 MB packet buffer split in two frames), installs
 *  a V-blank callback that posts a semaphore every m_v_interval V-blanks,
 *  and runs the frame main's loop ends with: pads, scene update and draw
 *  into the i3d packet, wait for the semaphore and the last DMA, swap the
 *  buffers, kick the new packet on VIF1. The rest forwards to the scene.
 *
 *  PORT: the V-blank callback runs on the platform's V-blank thread; the
 *  ROM's handler epilogue (sync.l / ei) has no host counterpart. Asserts
 *  report and carry on (common/ror_assert.h).
 * ======================================================================== */
#include "mag/magsystem.h"

#include <string.h>

#include "bed/bedmemory.h"
#include "common/ror_assert.h"
#include "rx/rxthread.h"
#include "sdk/eekernel.h"
#include "sdk/eeregs.h"
#include "sdk/fileio.h"
#include "sdk/libdma.h"
#include "sdk/libgraph.h"
#include "sequence/sequence.h"
#include "snd/rxmpeg.h"
#include "snd/sndman.h"
#include "utl/utlgame.h"
#include "utl/utlmisc.h"

/* One frame's share of the packet buffer (two frames). */
#define MAGSYS_TRASH_SIZE 0x1ae000

/* The i3d packet buffer (bss 0x308580, 0x35c000 bytes). */
static u_long128 s_trash_buf[2 * MAGSYS_TRASH_SIZE / sizeof(u_long128)];

/* The chapters' archives (sdata 0x2961a8), for loadModel's chapter number. */
static const char *s_chapter_rpk[8] = {
    "data/ch/00/CHAP00.RPK",
    "data/ch/01/CHAP01.RPK",
    "data/ch/02/CHAP02.RPK",
    "data/ch/03/CHAP03.RPK",
    "data/ch/04/CHAP04.RPK",
    "data/ch/05/CHAP05.RPK",
    "data/ch/06/CHAP06.RPK",
    "data/ch/07/CHAP07.RPK",
};

/* The sizes of the i3d structures this game was compiled against (rodata
 * 0x2e90a8); initGraph compares them with the library's own table. */
static const int s_i3d_struct_size[0x24] = {
    0x20,  0x130, 0x130, 0x1e,  0x38,    0x38,    0x6,  0x18, 0x8,
    0x20,  0x20,  0x20,  0x20,  0x40,    0x40,    0x40, 0x40, 0x20,
    0x4d0, 0x3b0, 0x80,  0xa0,  0x8,     0x44,    0x18, 0x20, 0x12168,
    0x12168, 0x10, 0x18, 0x60,  0x400,   0x400,   0x18, 0x20, 0x38,
};

/* rodata 0x2e9138, EUC-JP: "Package version i3d_package_version
 * 050121-stable-050826 : fatal error / A debug build or a different build
 * of the library is linked into the release application. / Fix the
 * libraries you link." */
static const char s_i3d_version_error[] =
    "\xa5\xd1\xa5\xc3\xa5\xb1\xa1\xbc\xa5\xb8\xa5\xd0\xa1\xbc\xa5\xb8\xa5\xe7"
    "\xa5\xf3 i3d_package_version 050121-stable-050826 : \xc3\xd7\xcc\xbf\xc5"
    "\xaa\xa5\xa8\xa5\xe9\xa1\xbc\n"
    "\xa5\xea\xa5\xea\xa1\xbc\xa5\xb9\xc8\xc7\xa4\xce\xa5\xa2\xa5\xd7\xa5\xea"
    "\xa5\xb1\xa1\xbc\xa5\xb7\xa5\xe7\xa5\xf3\xa4\xcb\xa1\xa2\xa5\xc7\xa5\xd0"
    "\xa5\xc3\xa5\xb0\xc8\xc7\xa4\xde\xa4\xbf\xa4\xcf\n"
    "\xb0\xdb\xa4\xca\xa4\xeb\xa5\xd3\xa5\xeb\xa5\xc9\xa4\xce\xa5\xe9\xa5\xa4"
    "\xa5\xd6\xa5\xe9\xa5\xea\xa4\xac\xa5\xea\xa5\xf3\xa5\xaf\xa4\xb5\xa4\xec"
    "\xa4\xc6\xa4\xa4\xa4\xde\xa4\xb9\xa1\xa3\n"
    "\xa5\xea\xa5\xf3\xa5\xaf\xa4\xb9\xa4\xeb\xa5\xe9\xa5\xa4\xa5\xd6\xa5\xe9"
    "\xa5\xea\xa4\xf2\xbd\xa4\xc0\xb5\xa4\xb7\xa4\xc6\xa4\xaf\xa4\xc0\xa4\xb5"
    "\xa4\xa4\xa1\xa3\n";

/* storeImage's BMP header (0x36 bytes, built on the stack). */
#pragma pack(push, 1)
struct MagSystemBmpHeader
{
    unsigned char  m_magic[2];      // 0x00: 'B', 'M'
    unsigned int   m_file_size;     // 0x02
    unsigned int   m_reserved;      // 0x06
    unsigned int   m_data_offset;   // 0x0a
    unsigned int   m_info_size;     // 0x0e
    int            m_width;         // 0x12
    int            m_height;        // 0x16: negative = top-down
    unsigned short m_planes;        // 0x1a
    unsigned short m_bits;          // 0x1c
    unsigned int   m_compression;   // 0x1e
    unsigned char  _22[0x14];
};
#pragma pack(pop)
static_assert(sizeof(MagSystemBmpHeader) == 0x36, "the BMP header is 0x36 bytes");
static_assert(offsetof(MagSystemBmpHeader, m_width) == 0x12, "m_width");
static_assert(offsetof(MagSystemBmpHeader, m_compression) == 0x1e, "m_compression");

MagSystem *MagSystem::m_current_system = NULL;  /* sdata 0x296190 */
int        MagSystem::m_v_count = 0;            /* sdata 0x296194 */
int        MagSystem::m_v_interval = 2;         /* data 0x296198 */
int        MagSystem::m_v_sema = -1;            /* data 0x29619c */

// ──────────────────────────────────────────────────────────────────────
// The V-blank callback MagSystem::start installs: counts V-blanks, hands
// the count to the movie player, and posts the frame semaphore every
// m_v_interval V-blanks.
// ROR 0x00117654

int MagSystem::vSyncCallback(int cause)
{
    (void)cause;
    m_v_count++;
    rxMpegSetVCount(m_v_count);
    if (m_v_interval <= m_v_count)
    {
        m_v_count = 0;
        iSignalSema(m_v_sema);
    }
    // PC: the ROM ends with sync.l / ei (the interrupt handler epilogue).
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The same callback posting on every V-blank; nothing installs it.
// ROR 0x001176bc

int MagSystem::vSyncCallbackEach(int cause)
{
    (void)cause;
    m_v_count++;
    if (0 < m_v_count)
    {
        m_v_count = 0;
        iSignalSema(m_v_sema);
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The one system object: counters cleared, the pads and the scene built,
// registered as the current system, the random seed reset.
// callcheck: MagScene (the member's constructor)
// ROR 0x001176fc

MagSystem::MagSystem()
    : m_frame(0), m_vib_time(0), m_vib_on(0),
      m_store_image_filename(NULL), m_store_request(0)
{
    for (int i = 0; i < 2; i++)
    {
        m_pad_ref[i].m_pad = NULL;
    }
    ROR_ASSERT(m_current_system == NULL);  /* 184 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    m_current_system = this;
    m_trash_buf = NULL;
    utlRandReset();
    memset(&m_info, 0, sizeof(m_info));
    m_language = 0;
    m_language_free = 1;
}

// ──────────────────────────────────────────────────────────────────────
// No current system any more; the scene and the pads go with the object.
// callcheck: ~MagScene, ~BedPad, operator_delete (member destructors and the deleting variant)
// ROR 0x00117808

MagSystem::~MagSystem()
{
    m_current_system = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Takes a language once (main's -l option, or the language select): the
// sequence engine switches its texts; later calls are refused.
// ROR 0x001178b8

bool MagSystem::setLanguage(int lang)
{
    bool free = m_language_free != 0;
    if (free)
    {
        seqSetLanguage(lang, 1);
        m_language = lang;
        m_language_free = 0;
    }
    return free;
}

// ──────────────────────────────────────────────────────────────────────
// Runs pad 0's big motor at full strength for a number of frames (update
// stops it), when the vibration option is on.
// ROR 0x00117924

void MagSystem::startVibration(int frames)
{
    if (m_vib_on != 0)
    {
        m_pad_ref[0].m_pad->setVibration(0, 0xff);
        m_vib_time = frames;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The light set actor no is lit by (DogModel::setLightID, inlined).
// ROR 0x00117974

void MagSystem::setActorLightID(int no, int id)
{
    m_scene.m_actors[no].setLightID(id);
}

// ──────────────────────────────────────────────────────────────────────
// Graphics bring-up: both pads, the VIF1 channel, DMA / VIF0 / GS resets,
// NTSC interlaced frame mode, the 512x448 double buffer (frame pages 0x70
// and 0, the Z buffer after the second), the i3d version check (a
// mismatch prints and hangs), the i3d hardware set-up and the draw
// context over the static packet buffer.
// ROR 0x00117990

void MagSystem::initGraph()
{
    for (int i = 0; i < 2; i++)
    {
        m_pad[i].init(i);
        m_pad[i].m_repeat_wait = 15;
        m_pad[i].m_repeat_interval = 10;
        m_pad_ref[i].m_pad = &m_pad[i];
    }
    m_dma = sceDmaGetChan(1);
    sceDmaReset(1);
    vif0ResetVu0();
    sceGsResetPath();
    // 0, 1, 2, 0: reset, interlace, NTSC, frame in the SDK's numbering (the
    // platform's libgraph.h swaps the INTERLACE / FRAME macros' values).
    sceGsResetGraph(0, 1, SCE_GS_NTSC, 0);
    sceGsSetDefDBuff(&m_dbuff, SCE_GS_PSMCT32, 512, 448, 2 /* ZGEQUAL */, SCE_GS_PSMZ16S, 1 /* clear */);
    m_dbuff.clear1.rgbaq = sceGsRgbaq();
    m_dbuff.disp[1].dispfb.FBP = 0x70;
    m_dbuff.disp[0].dispfb.FBP = 0;
    m_dbuff.draw0.frame1.FBP = 0x70;
    m_dbuff.draw0.zbuf1.ZBP = m_dbuff.disp[1].dispfb.FBP * 2;
    m_dbuff.draw1.frame1.FBP = 0;
    m_dbuff.draw1.zbuf1.ZBP = m_dbuff.disp[1].dispfb.FBP * 2;
    m_dbuff.clear0.rgbaq = sceGsRgbaq();

    int size[0x24];
    memcpy(size, s_i3d_struct_size, sizeof(size));
    for (unsigned i = 0; i < 0x24; i++)
    {
        if (size[i] != i3dGetStructSize(i))
        {
            i3dPrintf(s_i3d_version_error);
            for (;;)
            {
            }
        }
    }

    if (i3dInitHardware() == 0)
    {
        m_trash_buf = s_trash_buf;
        i3dCtxInit(&m_i3d, s_trash_buf, sizeof(s_trash_buf), 2);
        m_info.m_trash_size = MAGSYS_TRASH_SIZE;
        i3dCtxSetup(&m_i3d, 0x158, 0xa8);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The game's 512x448 double buffer again (after the now-loading screen or
// a mode that replaced it), clear colours black.
// ROR 0x00117c58

void MagSystem::resetDBuff()
{
    sceGsSetDefDBuff(&m_dbuff, SCE_GS_PSMCT32, 512, 448, 2 /* ZGEQUAL */, SCE_GS_PSMZ16S, 1 /* clear */);
    m_dbuff.disp[1].dispfb.FBP = 0x70;
    m_dbuff.clear1.rgbaq = sceGsRgbaq();
    m_dbuff.draw0.zbuf1.ZBP = m_dbuff.disp[1].dispfb.FBP * 2;
    m_dbuff.disp[0].dispfb.FBP = 0;
    m_dbuff.draw0.frame1.FBP = 0x70;
    m_dbuff.draw1.frame1.FBP = 0;
    m_dbuff.draw1.zbuf1.ZBP = m_dbuff.disp[1].dispfb.FBP * 2;
    m_dbuff.clear0.rgbaq = sceGsRgbaq();
}

// ──────────────────────────────────────────────────────────────────────
// A 512x384 double buffer (frame pages 0x60 and 0); the clear colours are
// left as they are. The game mode 0x1ab04c uses it.
// ROR 0x00117d14

void MagSystem::resetDBuff384()
{
    sceGsSetDefDBuff(&m_dbuff, SCE_GS_PSMCT32, 512, 384, 2 /* ZGEQUAL */, SCE_GS_PSMZ16S, 1 /* clear */);
    m_dbuff.disp[1].dispfb.FBP = 0x60;
    m_dbuff.disp[0].dispfb.FBP = 0;
    m_dbuff.draw0.zbuf1.ZBP = m_dbuff.disp[1].dispfb.FBP * 2;
    m_dbuff.draw0.frame1.FBP = 0x60;
    m_dbuff.draw1.frame1.FBP = 0;
    m_dbuff.draw1.zbuf1.ZBP = m_dbuff.disp[1].dispfb.FBP * 2;
}

// ──────────────────────────────────────────────────────────────────────
// Graphics, then the scene.
// ROR 0x00117dc8

void MagSystem::init()
{
    initGraph();
    m_scene.init();
}

// ──────────────────────────────────────────────────────────────────────
// init again; nothing calls it.
// ROR 0x00117dfc

void MagSystem::reset()
{
    init();
}

// ──────────────────────────────────────────────────────────────────────
// Starts the frame: the scene, the V-blank semaphore (empty, no limit to
// speak of) and its callback, a heap check, the scene's render set-up on
// the i3d context.
// ROR 0x00117e18

void MagSystem::start()
{
    m_scene.start();
    SemaParam sp;
    sp.maxCount = 0x7fffffff;
    sp.initCount = 0;
    sp.option = 0;
    m_v_sema = CreateSema(&sp);
    ROR_ASSERT(m_v_sema >= 0);  /* 583 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    sceGsSyncVCallback(vSyncCallback);
    g_utlMemory.check(0);
    m_scene.initRender(&m_i3d);
}

// ──────────────────────────────────────────────────────────────────────
// Clears the scene word +0x22cc (GamMan::loadStage).
// ROR 0x00117ec0

void MagSystem::clearScene22cc()
{
    m_scene.m_22cc = 0;
}

// ──────────────────────────────────────────────────────────────────────
// Undoes start and initGraph: packet buffer, i3d context, pads, callback,
// semaphore.
// ROR 0x00117ed0

void MagSystem::term()
{
    if (m_trash_buf != NULL)
    {
        g_utlMemory.free(m_trash_buf);
        m_trash_buf = NULL;
    }
    i3dCtxTerm(&m_i3d);
    for (int i = 0; i < 2; i++)
    {
        m_pad[i].term();
    }
    sceGsSyncVCallback(NULL);
    ROR_ASSERT(m_v_sema);  /* 667 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    DeleteSema(m_v_sema);
}

// ──────────────────────────────────────────────────────────────────────
// The scene's light flags: the first three values, three zeros, and the
// set number; d, e and f are not used.
// ROR 0x00117f80

void MagSystem::setSceneFlags(int a, int b, int c, int d, int e, int f, int g)
{
    (void)d;
    (void)e;
    (void)f;
    int flags[6];
    flags[0] = a;
    flags[1] = b;
    flags[2] = c;
    flags[3] = 0;
    flags[4] = 0;
    flags[5] = 0;
    m_scene.setLightFlags(flags, g);
}

// ──────────────────────────────────────────────────────────────────────
// Adds a light of a type with a mask, then sets its parameter 2 to v0 and
// a second parameter to v1 (1 for type 1, 0 for type 2; type 0 has none).
// ROR 0x00117fc4

void MagSystem::addLight(int type, unsigned int mask, const float *v0, const float *v1)
{
    int idx = m_scene.addLight(type, mask);
    if (idx >= 0)
    {
        m_scene.setLightParam(type, idx, 2, v0, -1);
        if (type == 1)
        {
            m_scene.setLightParam(1, idx, 1, v1, -1);
        }
        else if (type != 0)
        {
            if (type == 2)
            {
                m_scene.setLightParam(2, idx, 0, v1, -1);
            }
            else
            {
                ROR_ASSERT(0);  /* 718 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// Binds actor no to the player's MagLifePC (no < 0 unbinds it and stops
// the motor). The mode code is 'NORM' 0, 'PC02' 1, 'JNFC' 2, 'PC05' 3, or
// 0..3 as is; anything else is 0. A new player clears the pads' +0x40
// words. The vibration option is read again.
// ROR 0x001180ec

void MagSystem::setPC(int no, int code)
{
    if (no < 0)
    {
        if (m_pc_no >= 0)
        {
            m_pc_no_last = m_pc_no;
        }
        m_pc_no = no;
        if (m_vib_on != 0)
        {
            m_vib_time = 0;
            m_pad_ref[0].m_pad->setVibration(0, 0);
        }
    }
    else
    {
        m_pad[0].m_44 = 0;
        m_pad[0].m_48 = 0;
        m_pad[0].m_40 = 0;
        m_pad[1].m_44 = 0;
        m_pad[1].m_48 = 0;
        m_pad[1].m_40 = 0;
        m_pc_no = no;
        m_pc_no_last = no;
    }

    int mode;
    if (code == 0x4e4f524d)         /* 'NORM' */
    {
        mode = 0;
    }
    else if (code == 0x4a4e4643)    /* 'JNFC' */
    {
        mode = 2;
    }
    else if (code == 0x50433032)    /* 'PC02' */
    {
        mode = 1;
    }
    else if (code == 0x50433035)    /* 'PC05' */
    {
        mode = 3;
    }
    else if (code >= 0 && code <= 3)
    {
        mode = code;
    }
    else
    {
        mode = 0;
    }
    m_scene.setPC(no, mode);
    m_vib_on = utlIsVibrationOn();
}

// ──────────────────────────────────────────────────────────────────────
// Binds actor no to Brown's MagLifeBwn.
// ROR 0x0011825c

void MagSystem::setBwn(int no, int arg)
{
    m_bwn_no = no;
    m_scene.setBwn(no, arg);
}

// ──────────────────────────────────────────────────────────────────────
// A frame without the scene: pads, an empty packet, the V-blank wait and
// the kick. Nothing calls it.
// ROR 0x0011828c

void MagSystem::flushFrame()
{
    m_pad[0].update();
    m_pad[1].update();
    beginPacket();
    endPacket();
    waitVSync();
    kickPacket();
}

// ──────────────────────────────────────────────────────────────────────
// The frame (the end of main's loop): the motor's countdown, the pads,
// the movie's buffer parity, scene update, sound, the scene drawn into a
// new packet (its time measured on RCNT0), then the V-blank wait, the
// buffer swap and the kick of this frame's packet.
// ROR 0x001182dc

void MagSystem::update()
{
    int t0 = REG_RCNT0_COUNT;
    if (0 < m_vib_time && --m_vib_time < 1)
    {
        m_pad_ref[0].m_pad->setVibration(0, 0);
    }
    m_pad[0].update();
    m_pad[1].update();
    g_rxMpeg.setFrameParity(m_frame & 1);
    m_scene.update(&m_pad[0], &m_pad[1]);
    g_sndMan.update();
    beginPacket();
    m_scene.draw(&m_i3d, &m_dbuff, m_frame);
    m_scene.postDraw();
    m_info.m_t_cpu = (int)REG_RCNT0_COUNT - t0;
    int r = i3dCtxGetRest(&m_i3d);
    ROR_ASSERT(r != 0);  /* 930 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    endPacket();
    waitVSync();
    m_info.m_t_vwait -= t0;
    kickPacket();
    m_frame++;
    utlIncFrameCount();
}

// ──────────────────────────────────────────────────────────────────────
// Opens this frame's packet in the i3d context.
// ROR 0x00118440

void MagSystem::beginPacket()
{
    i3dCtxBeginFrame(&m_i3d);
}

// ──────────────────────────────────────────────────────────────────────
// Writes the frame buffer (page 0, 512x448) to m_store_image_filename as
// a top-down 24-bit BMP, one line at a time through the store-image
// transfer, RGBA turned into BGR; then re-enables VIF1's tag transfer.
// ROR 0x0011845c

void MagSystem::storeImage()
{
    ROR_ASSERT(m_store_image_filename);  /* 1033 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    int fd = sceOpen(m_store_image_filename, SCE_WRONLY | SCE_CREAT | SCE_TRUNC);
    if (fd < 0)
    {
        return;
    }

    MagSystemBmpHeader header;
    memset(&header, 0, sizeof(header));
    header.m_magic[0] = 'B';
    header.m_magic[1] = 'M';
    header.m_file_size = sizeof(header) + 512 * 448 * 3;
    header.m_data_offset = sizeof(header);
    header.m_info_size = 0x28;
    header.m_width = 512;
    header.m_height = -448;
    header.m_planes = 1;
    header.m_bits = 24;
    header.m_compression = 0;
    sceWrite(fd, &header, sizeof(header));

    sceDmaSync(sceDmaGetChan(1), 0, 0);
    sceGsSyncPath(0, 0);
    sceGsStoreImage si;
    u_long128 rgba[512 * 4 / sizeof(u_long128)];
    unsigned char bgr[512 * 3];
    for (int y = 0; y < 448; y++)
    {
        sceGsSetDefStoreImage(&si, 0, 8, SCE_GS_PSMCT32, 0, (short)y, 512, 1);
        FlushCache(0);
        sceGsExecStoreImage(&si, rgba);
        sceGsSyncPath(0, 0);
        const unsigned char *src = (const unsigned char *)rgba;
        unsigned char *dst = bgr;
        for (int x = 0; x < 512; x++)
        {
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            src += 4;
            dst += 3;
        }
        sceWrite(fd, bgr, sizeof(bgr));
    }
    sceClose(fd);
    m_dma->chcr.TTE = 1;
    sceGsSyncPath(0, 0);
}

// ──────────────────────────────────────────────────────────────────────
// Closes the frame's packet: records what was left and any overrun (which
// must not happen), the heap's free space, then a requested screenshot.
// ROR 0x00118640

void MagSystem::endPacket()
{
    m_info.m_trash_rest = i3dCtxGetRest(&m_i3d);
    m_info.m_trash_overrun = i3dCtxGetOverrun(&m_i3d);
    ROR_ASSERT(m_info.m_trash_overrun == 0);  /* 1159 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    BEDMEMINFO info;
    g_utlMemory.getInfo(&info);
    m_info.m_heap_max_free = info.m_max_free;
    m_info.m_heap_total_free = info.m_total_free;
    i3dCtxEndFrame(&m_i3d);
    if (m_store_request != 0)
    {
        m_store_request = 0;
        sceGsSyncPath(0, 0);
        storeImage();
    }
}

// ──────────────────────────────────────────────────────────────────────
// Reads the packet rest against one frame's share; the result is unused
// (only the division's zero check survives in the ROM). Nothing calls it.
// ROR 0x001186f0

void MagSystem::checkPacketRest()
{
    int rest = i3dCtxGetRest(&m_i3d);
    (void)(rest / MAGSYS_TRASH_SIZE);
}

// ──────────────────────────────────────────────────────────────────────
// Turns read circuit 2 on or off in both display environments and writes
// the first one's PMODE to the GS. Nothing calls it.
// ROR 0x0011871c

void MagSystem::setDispCircuit2(int on)
{
    m_dbuff.disp[0].pmode.EN2 = on != 0;
    m_dbuff.disp[1].pmode.EN2 = on != 0;
    REG_GS_PMODE = *(u_long *)&m_dbuff.disp[0].pmode;
}

// ──────────────────────────────────────────────────────────────────────
// The end of a frame: the movie player's V-blank step, the rx threads'
// turn when the V-blank is due, the semaphore (drained, so a late frame
// does not run the next one early), the VIF1 DMA, then the buffer swap.
// ROR 0x00118754

void MagSystem::waitVSync()
{
    if (g_rxMpeg.m_active != 0)
    {
        rxThVSync(0);
    }
    m_info.m_t_vwait = REG_RCNT0_COUNT;
    if (m_v_interval <= m_v_count)
    {
        rxThSync(0);
    }
    if (m_v_sema < 0)
    {
        sceGsSyncPath(0, 0);
    }
    else
    {
        WaitSema(m_v_sema);
        while (PollSema(m_v_sema) >= 0)
        {
        }
    }
    while (sceDmaSync(m_dma, 1, 0) != 0)
    {
        rxThSync(0);
    }
    m_info.m_t_dma_end = REG_RCNT0_COUNT;
    FlushCache(0);
    sceGsSwapDBuff(&m_dbuff, m_frame & 1);
}

// ──────────────────────────────────────────────────────────────────────
// Sends this frame's i3d packet on VIF1.
// ROR 0x00118840

void MagSystem::kickPacket()
{
    sceGsSyncPath(0, 0);
    sceDmaSend(m_dma, i3dCtxGetPacket(&m_i3d));
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00118880

void MagSystem::setCameraMode(int mode)
{
    m_scene.setCameraMode(mode);
}

// ──────────────────────────────────────────────────────────────────────
// A light parameter from four floats (all sets).
// ROR 0x001188a8

void MagSystem::setLightColor(int type, int idx, int which, float r, float g, float b, float a)
{
    float v[4];
    v[0] = r;
    v[1] = g;
    v[2] = b;
    v[3] = a;
    m_scene.setLightParam(type, idx, which, v, -1);
}

// ──────────────────────────────────────────────────────────────────────
// A light parameter into four floats. Nothing calls it.
// ROR 0x001188e4

void MagSystem::getLightColor(int type, int idx, int which, float *r, float *g, float *b, float *a)
{
    float v[4];
    m_scene.getLightParam(type, idx, which, v, -1);
    *r = v[0];
    *g = v[1];
    *b = v[2];
    *a = v[3];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00118960

int MagSystem::getCameraMode(int *out)
{
    return m_scene.getCameraMode(out);
}

// ──────────────────────────────────────────────────────────────────────
// Five values for the scene's view object no.
// ROR 0x00118980

void MagSystem::setViewParam(int no, int a, int b, int c, int d, int e)
{
    int v[5];
    v[0] = a;
    v[1] = b;
    v[2] = c;
    v[3] = d;
    v[4] = e;
    m_scene.setViewParam(no, v);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001189bc

void MagSystem::drawView(int no, int a, int b, const void *p, int c)
{
    m_scene.drawView(no, a, b, p, c);
}

// ──────────────────────────────────────────────────────────────────────
// The scene's load on this system's i3d context.
// ROR 0x001189e4

int MagSystem::loadScene(int a, int b, int c)
{
    return m_scene.load(&m_i3d, a, b, c);
}

// ──────────────────────────────────────────────────────────────────────
// Hangs actor theNo from actor parentNo's node (parentNo < 0: detaches
// it, and the attach node is left as it was).
// ROR 0x00118a18

void MagSystem::setActorParent(int theNo, int parentNo, int node)
{
    ROR_ASSERT(theNo >= 0);  /* 1486 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    DogActor *actor = &m_scene.m_actors[theNo];
    if (parentNo < 0)
    {
        actor->setParent(NULL);
    }
    else
    {
        actor->setParent(&m_scene.m_actors[parentNo]);
        actor->setAttachNode(node);  /* EE: through vtable slot 10 */
    }
}

// ──────────────────────────────────────────────────────────────────────
// The world matrix of actor no's node n, into an i3d matrix (the ROM
// builds it in an aligned temporary and copies it out unaligned).
// ROR 0x00118adc

void MagSystem::getActorNodeMatrix(int no, I3dMatrixF4x4 *out, int n)
{
    ROR_ASSERT(no >= 0);  /* 1511 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(n >= 0);   /* 1512 */
    sceVu0FMATRIX m;
    m_scene.m_actors[no].getNodeWorldMatrix(m, n);  /* EE: through vtable slot 17 */
    memcpy(out, m, sizeof(*out));
}

// ──────────────────────────────────────────────────────────────────────
// The same into an aligned matrix.
// ROR 0x00118c14

void MagSystem::getActorNodeMatrix(int no, sceVu0FMATRIX out, int n)
{
    ROR_ASSERT(no >= 0);  /* 1530 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    ROR_ASSERT(n >= 0);   /* 1531 */
    m_scene.m_actors[no].getNodeWorldMatrix(out, n);  /* EE: through vtable slot 17 */
}

// ──────────────────────────────────────────────────────────────────────
// The world matrix of actor no's named node; returns the node index, -1
// (and out untouched) when the model has no such node.
// ROR 0x00118ccc

int MagSystem::getActorNodeMatrixByName(int no, I3dMatrixF4x4 *out, const char *name)
{
    /* DogModel::getNodeIndexByName inlined (its asserts: dog/dogmodel.h 305) */
    int n = m_scene.m_actors[no].getNodeIndexByName(name);  // callcheck: FUN_00215a30, i3rGetNodeIndexByName (inside the inline DogModel::getNodeIndexByName)
    if (n >= 0)
    {
        getActorNodeMatrix(no, out, n);
    }
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00118d9c

int MagSystem::getActorNodeMatrixByName(int no, sceVu0FMATRIX out, const char *name)
{
    int n = m_scene.m_actors[no].getNodeIndexByName(name);  // callcheck: FUN_00215a30, i3rGetNodeIndexByName (inside the inline DogModel::getNodeIndexByName)
    if (n >= 0)
    {
        getActorNodeMatrix(no, out, n);
    }
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// How far actor no's motion root moves from time t0 to t1.
// ROR 0x00118e6c

int MagSystem::getActorMotionMove(int no, float t0, float t1, float *move, float *rot, float *pos)
{
    return m_scene.m_actors[no].getMotionMove(t0, t1, move, rot, pos);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00118ea4

void MagSystem::addActorTrans(int no, const float *d, float trans_ofs_y, float rot_y_aar)
{
    m_scene.m_actors[no].addTrans(d, trans_ofs_y, rot_y_aar);
}

// ──────────────────────────────────────────────────────────────────────
// The scene takes the last two arguments the other way round.
// ROR 0x00118ed8

void MagSystem::setActorAnim(int no, int a, int b)
{
    m_scene.setActorAnim(no, b, a);
}

// ──────────────────────────────────────────────────────────────────────
// Likewise (the name last). Nothing calls it.
// ROR 0x00118f04

int MagSystem::loadActorAnim(int no, const char *name, int arg)
{
    return m_scene.loadActorAnim(no, arg, name);
}

// ──────────────────────────────────────────────────────────────────────
// Drops actor no's motion bank theId (0..3).
// ROR 0x00118f30

void MagSystem::releaseActorMotion(int no, int theId)
{
    m_scene.m_actors[no].releaseMotion(theId);
}

// ──────────────────────────────────────────────────────────────────────
// The camera's two target vectors, copied. Nothing calls it.
// ROR 0x00118f64

void MagSystem::setCameraTarget(const float *pos, const float *at)
{
    float p[4];
    float t[4];
    p[0] = pos[0];
    p[1] = pos[1];
    p[2] = pos[2];
    p[3] = pos[3];
    t[0] = at[0];
    t[1] = at[1];
    t[2] = at[2];
    t[3] = at[3];
    m_scene.setCameraTarget(p, t);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00118fdc

int MagSystem::getPCWordE4()
{
    return m_scene.m_pc.m_e4;
}

// ──────────────────────────────────────────────────────────────────────
// The player's state folded into a class (44 callers decide by it).
// ROR 0x00118fec

int MagSystem::getPCStateClass()
{
    int cls = 0xc;
    switch (m_scene.m_pc.m_state)
    {
    case 1:
    case 2:
    case 3:
    case 4:
        cls = m_scene.m_pc.getSubStateClass();
        break;
    case 5:
    case 6:
        cls = 1;
        break;
    case 7:
    case 8:
    case 0xb:
    case 0xd:
    case 0xf:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
        cls = 0xc;
        break;
    case 9:
        cls = 3;
        if (m_scene.m_pc.m_0c != 1)
        {
            cls = 0xb;
        }
        break;
    case 10:
        cls = 2;
        break;
    case 0xc:
        cls = 3;
        break;
    case 0xe:
        cls = 9;
        break;
    case 0x16:
    case 0x17:
        cls = 4;
        break;
    }
    return cls;
}

// ──────────────────────────────────────────────────────────────────────
// Brown's state word when it is 0..13, else -1.
// ROR 0x00119090

int MagSystem::getBwnState()
{
    switch (m_scene.m_bwn.m_d40)
    {
    case 0:  return 0;
    case 1:  return 1;
    case 2:  return 2;
    case 3:  return 3;
    case 4:  return 4;
    case 5:  return 5;
    case 6:  return 6;
    case 7:  return 7;
    case 8:  return 8;
    case 9:  return 9;
    case 10: return 10;
    case 11: return 11;
    case 12: return 12;
    case 13: return 13;
    default: return -1;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Loads model id from a chapter's archive (chapter < 0: none); models
// 0x20xx and 0x24xx get actor flags 0x60.
// ROR 0x00119138

void MagSystem::loadModel(int id, int chapter)
{
    const char *rpk = NULL;
    if (chapter >= 0)
    {
        rpk = s_chapter_rpk[chapter];
    }
    int no = m_scene.loadModel(&m_i3d, id, rpk);
    if (no >= 0 && ((id & 0xff00) == 0x2000 || (id & 0xff00) == 0x2400))
    {
        m_scene.m_actor_flags[no] |= 0x60;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Loads a model by name from a chapter's archive. Nothing calls it.
// ROR 0x001191e0

void MagSystem::loadModelByName(const char *name, int arg, int chapter)
{
    const char *rpk = NULL;
    if (chapter >= 0)
    {
        rpk = s_chapter_rpk[chapter];
    }
    m_scene.loadModelByName(&m_i3d, name, arg, rpk);
}

// ──────────────────────────────────────────────────────────────────────
// Places actor no (w is left as it was); the player's last actor gets a
// trace that prints nothing in the retail build.
// ROR 0x00119234

void MagSystem::setActorTrans(int no, float x, float y, float z)
{
    if (no == m_pc_no_last)
    {
        dbgTraceVector3(x, y, z);
    }
    DogActor *actor = &m_scene.m_actors[no];
    actor->m_trans[0] = x;
    actor->m_trans[1] = y;
    actor->m_trans[2] = z;
    actor->m_dirty = 1;
}

// ──────────────────────────────────────────────────────────────────────
// Turns actor no to (x, y, z) and makes that its base rotation; the
// offset vector and the Y correction are reset.
// ROR 0x001192c8

void MagSystem::setActorRotate(int no, float x, float y, float z)
{
    DogActor *p = &m_scene.m_actors[no];
    ROR_ASSERT(p);  /* 1877 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    p->setRotate(x, y, z);
    p->setBaseRotate(x, y, z);
    p->m_80[3] = 1.0f;
    p->m_80[2] = 0.0f;
    p->m_80[1] = 0.0f;
    p->m_80[0] = 0.0f;
    p->clearRotYAAR();
    p->m_dirty = 1;
}

// ──────────────────────────────────────────────────────────────────────
// The same from a vector (all four words), and the Y offset is cleared.
// ROR 0x0011939c

void MagSystem::setActorRotate(int no, const float *v)
{
    DogActor *p = &m_scene.m_actors[no];
    ROR_ASSERT(p);  /* 1895 */  // callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
    p->m_rotate[0] = v[0];
    p->m_rotate[1] = v[1];
    p->m_rotate[2] = v[2];
    p->m_rotate[3] = v[3];
    p->normalizeRotate();
    p->m_dirty = 1;
    p->setBaseRotate(v);
    p->m_80[3] = 1.0f;
    p->m_80[2] = 0.0f;
    p->m_80[1] = 0.0f;
    p->m_80[0] = 0.0f;
    p->clearRotYAAR();
    p->m_dirty = 1;
    p->clearRotOfsY();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0011948c

void MagSystem::clearPC()
{
    m_scene.clearPC();
}
