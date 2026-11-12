// system/sysnowloading.cpp -- the now-loading screen (0x204d38..0x2053f4).
//
// main creates one SysNowLoading around the boot loads. init gives it its own
// 512x448 double buffer, clears GS memory and reads LOADING.BIN: a 256x256
// PSMT8 picture holding four 256x64 "Now Loading" frames, then its CLUT.
// While a file loads, the pfh loader (0x1be564) calls draw every V-blank: the
// current frame of the picture as one sprite in the bottom-right corner, the
// next frame every 8 V-blanks.
//
// PC: the picture becomes a GPU texture when loadImage reads it and draw hands
// the sprite to the native 2D module (rosepan_draw2d) in a host frame of its
// own; the GS uploads, the double-buffer swap and the DMA packet do not reach
// a GS (architecture.md "PC platform layer").

#include "system/sysnowloading.h"

#include <string.h>

#include "rosepan/rendering/rosepan_renderer.h"
#include "sdk/eekernel.h"
#include "sdk/eeregs.h"
#include "sdk/fileio.h"
#include "utl/utlmisc.h"

static const char s_file[] = "system/sysnowloading.cpp";

// The frame buffer init sets up, and the GS primitive coordinates' origin:
// sceGsSetHalfOffset centres the 512x448 buffer on (2048, 2048).
static const int kFrameWidth = 512;
static const int kFrameHeight = 448;

static float gsToFrame(int gs, int size)
{
    return (float)gs / 16.0f - (float)(2048 - size / 2);
}

// ──────────────────────────────────────────────────────────────────────
// Reads the picture into the 1 MB image and uploads its CLUT.
// callcheck: sceGsSetDefLoadImage, FlushCache, sceGsExecLoadImage, sceGsSyncPath (PC: the CLUT upload is the GPU texture)
// ROR 0x00204d38

void SysNowLoading::loadImage()
{
    char path[32] = "cdrom0:\\DATA\\CMNTEX\\LOADING.BIN";
    int fd = sceOpen(path, SCE_RDONLY);
    sceRead(fd, m_image, 0x20000);
    sceClose(fd);
    // The ROM then copies the first 64 KB (the pixels) onto themselves, source
    // and destination both m_image: no effect, not written.
    // PC: the ROM LoadImages the CLUT (m_image + 0x10000, 16x16 CT32) to dbp
    // 8000 here and the pixels to dbp 0x1a40 every frame (uploadImage); the
    // host makes both one texture. The sprite is drawn with ABE 0: opaque.
    m_texture = RosePan_Draw2dMakeT8Texture(m_image, (const uint32_t *)(m_image + 0x10000),
                                            256, 256, 1, 1);
}

// ──────────────────────────────────────────────────────────────────────
// One ST register of the sprite: S, T (Q is 1.0).
// ROR 0x00204e68

void SysNowLoading::setST(float s, float t, float *st)
{
    st[0] = s;
    st[1] = t;
}

// ──────────────────────────────────────────────────────────────────────
// The sprite showing picture frame `frame` (0..3): rows frame*64..+64 of the
// 256x256 picture, stretched to 192x48 at (288, 376) of the 512x448 buffer.
// PC: the ROM writes it as a DMA packet into m_packet -- a cnt tag; A+D
// TEXFLUSH, TEX0 (dbp 0x1a40, TBW 4, PSMT8, 256x256, TCC, DECAL, cbp 8000 CT32
// CSM1, CLD 1), TEX1 (bilinear), CLAMP (clamp, clamp); a SPRITE (PRIM 0x16,
// textured, STQ) with RGBAQ (0xff, 0x40, 0x20, 0x80); an end tag. The host
// keeps the sprite's values; draw passes them to rosepan_draw2d.
// ROR 0x00204e98

void SysNowLoading::makePacket(int frame)
{
    setST(0.0f, (float)frame * 0.25f, &m_sprite.s0);
    m_sprite.rgba[0] = 0xff;
    m_sprite.rgba[1] = 0x40;
    m_sprite.rgba[2] = 0x20;
    m_sprite.rgba[3] = 0x80;
    m_sprite.x0 = gsToFrame(0x8200, kFrameWidth);
    m_sprite.y0 = gsToFrame(0x8980, kFrameHeight);
    setST(1.0f, (float)(frame + 1) * 0.25f, &m_sprite.s1);
    m_sprite.x1 = gsToFrame(0x8e00, kFrameWidth);
    m_sprite.y1 = gsToFrame(0x8c80, kFrameHeight);
}

// ──────────────────────────────────────────────────────────────────────
// The picture's pixels to dbp 0x1a40 (256x256 PSMT8), before every frame.
// callcheck: sceGsSetDefLoadImage, FlushCache, sceGsExecLoadImage, sceGsSyncPath (PC: the texture is on the GPU since loadImage)
// ROR 0x00205078

void SysNowLoading::uploadImage()
{
    // PC: nothing to upload -- loadImage made the GPU texture.
}

// ──────────────────────────────────────────────────────────────────────
// Zeroes all 4 MB of GS memory, 256 KB at a time from a zeroed 1 MB buffer.
// callcheck: sceGsSetDefLoadImage, FlushCache, sceGsExecLoadImage, sceGsSyncPath (PC: no GS memory on the native draw path)
// ROR 0x002050e8

void SysNowLoading::clearVram()
{
    void *zero = g_utlMemory.allocDebug(0x100000, s_file, 246, 0, 2);
    memset(zero, 0, 0x100000);
    // PC: the ROM LoadImages it 16 times, dbp n * 0x400 (256x256 CT32) for n
    // 0..15; the allocation is kept for the heap's sake.
    g_utlMemory.free(zero);
}

// ──────────────────────────────────────────────────────────────────────
// One frame of the loading screen: wait for the V-blank, swap the double
// buffer (which clears the new back buffer), send the sprite; every 8 frames
// the next picture frame.
// callcheck: sceDmaSend (PC: the sprite goes to rosepan_draw2d)
// ROR 0x002051cc

void SysNowLoading::draw()
{
    uploadImage();
    int field = sceGsSyncV(0);
    sceGsSetHalfOffset((m_frame & 1) ? &m_dbuff.draw1 : &m_dbuff.draw0, 0x800, 0x800, field == 0);
    FlushCache(0);
    sceGsSwapDBuff(&m_dbuff, m_frame & 1);
    sceGsSyncPath(0, 0);
    FlushCache(0);
    // PC: sceDmaSend(m_dmaChan, m_packet) -- the sprite in a host frame of
    // its own (draw runs inside the loads, outside mainLoop's frames).
    RosePan_RendererBeginFrame();
    RosePan_Draw2dSprite(m_texture, &m_sprite, kFrameWidth, kFrameHeight, 1, 1);
    RosePan_RendererEndFrame();
    m_frame++;
    sceGsSyncPath(0, 0);
    if ((m_frame & 7) == 0)
        makePacket(m_frame / 8 % 4);
}

// ──────────────────────────────────────────────────────────────────────
// The image and packet buffers, the GIF DMA channel, the 512x448 double buffer
// (clear colour black, Q 1.0), the first sprite, GS memory cleared, the picture;
// then timer 0 counts H-blanks.
// ROR 0x002052ac

void SysNowLoading::init()
{
    m_image = (u_char *)g_utlMemory.allocDebug(0x100000, s_file, 304, 0, 2);
    m_packet = (u_int *)g_utlMemory.allocDebug(0x4000, s_file, 305, 0, 2);
    m_dmaChan = sceDmaGetChan(2);  // GIF
    sceGsSetDefDBuff(&m_dbuff, SCE_GS_PSMCT32, kFrameWidth, kFrameHeight, 2, SCE_GS_PSMZ16S, 1);
    m_dbuff.clear1.rgbaq.R = 0;
    m_dbuff.clear1.rgbaq.G = 0;
    m_dbuff.clear1.rgbaq.B = 0;
    m_dbuff.clear1.rgbaq.A = 0;
    m_dbuff.clear1.rgbaq.Q = 1.0f;
    m_dbuff.clear0.rgbaq = m_dbuff.clear1.rgbaq;
    makePacket(0);
    clearVram();
    loadImage();
    REG_RCNT0_MODE = 0x83;
}

// ──────────────────────────────────────────────────────────────────────
// Clears GS memory and frees the two buffers (twice in mainInitGame: the
// explicit call, then the destructor's, which finds them freed).
// ROR 0x00205394

void SysNowLoading::term()
{
    clearVram();
    if (m_image != NULL)
    {
        g_utlMemory.free(m_image);
        m_image = NULL;
    }
    if (m_packet != NULL)
    {
        g_utlMemory.free(m_packet);
        m_packet = NULL;
    }
}
