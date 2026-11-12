/* ==========================================================================
 *  system/sysnowloading.h
 *
 *  The now-loading screen (system/sysnowloading.cpp, 0x204d38..0x2053f4): an
 *  object of 0x2a0 bytes main creates around the boot loads. init allocates
 *  a 1 MB image and a 16 KB packet from the game heap, sets up a 512x448
 *  double buffer, clears GS memory and loads LOADING.BIN from DATA/CMNTEX;
 *  while a file loads, the pfh loader 0x1be564 calls draw every V-blank
 *  (one of the picture's four 256x64 "Now Loading" frames in the bottom-right
 *  corner, the next every 8 frames); term clears GS memory and frees both.
 * ======================================================================== */
#ifndef _SYSTEM_SYSNOWLOADING_H
#define _SYSTEM_SYSNOWLOADING_H

#include <stddef.h>

#include "rosepan/rendering/rosepan_draw2d.h"
#include "sdk/libdma.h"
#include "sdk/libgraph.h"

/* EE 0x2a0. layout: host -- three pointers from +0x290 and the host-only
 * members after m_frame; the GS packets before them keep their EE offsets. */
struct SysNowLoading
{
    // The inline constructor clears the four words main sees cleared after
    // operator new; the destructor's term call is the second one in
    // mainInitGame.
    SysNowLoading() : m_image(NULL), m_packet(NULL), m_dmaChan(NULL), m_frame(0), m_texture(NULL) {}
    ~SysNowLoading() { term(); }

    void loadImage();                                                    // ROR 0x00204d38
    void setST(float s, float t, float *st);                             // ROR 0x00204e68
    void makePacket(int frame);                                          // ROR 0x00204e98
    void uploadImage();                                                  // ROR 0x00205078
    void clearVram();                                                    // ROR 0x002050e8
    void draw();                                                         // ROR 0x002051cc
    void init();                                                         // ROR 0x002052ac
    void term();                                                         // ROR 0x00205394

    sceGsDBuff     m_dbuff;     // ROR 0x000: sceGsSetDefDBuff(this, 0, 512, 448, ...)
    sceGsLoadImage m_loadImage; // ROR 0x230: clearVram's and uploadImage's transfers
    u_char        *m_image;     // ROR 0x290: 1 MB, LOADING.BIN (pixels, CLUT at +0x10000)
    u_int         *m_packet;    // ROR 0x294: 16 KB DMA packet (makePacket)
    sceDmaChan    *m_dmaChan;   // ROR 0x298: channel 2 (GIF)
    int            m_frame;     // ROR 0x29c: frames drawn

    // PC: the native draw path's state (rosepan_draw2d): the picture as a GPU
    // texture (made by loadImage) and the sprite makePacket decides.
    const void      *m_texture;
    RosePanSprite2d  m_sprite;
};
static_assert(offsetof(SysNowLoading, m_dbuff.draw0) == 0x60, "m_dbuff.draw0");
static_assert(offsetof(SysNowLoading, m_dbuff.clear0.rgbaq) == 0x100, "m_dbuff.clear0.rgbaq");
static_assert(offsetof(SysNowLoading, m_dbuff.draw1) == 0x150, "m_dbuff.draw1");
static_assert(offsetof(SysNowLoading, m_dbuff.clear1.rgbaq) == 0x1f0, "m_dbuff.clear1.rgbaq");
static_assert(offsetof(SysNowLoading, m_loadImage) == 0x230, "m_loadImage");
static_assert(offsetof(SysNowLoading, m_image) == 0x290, "m_image");

#endif /* _SYSTEM_SYSNOWLOADING_H */
