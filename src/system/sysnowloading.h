/* ==========================================================================
 *  system/sysnowloading.h
 *
 *  The now-loading screen (system/sysnowloading.cpp, 0x204d38..): an object
 *  of 0x2a0 bytes main creates around the boot loads. init allocates a 1 MB
 *  image and a 16 KB packet from the game heap, sets up a 512x448 double
 *  buffer, clears VRAM and loads LOADING.BIN from DATA/CMNTEX; term clears
 *  VRAM and frees both. Minimal: only what main reaches.
 * ======================================================================== */
#ifndef _SYSTEM_SYSNOWLOADING_H
#define _SYSTEM_SYSNOWLOADING_H

#include <stddef.h>

#include "sdk/libdma.h"
#include "sdk/libgraph.h"

/* EE 0x2a0. layout: host -- three pointers from +0x290; the GS packets
 * before them keep their EE offsets. */
struct SysNowLoading
{
    // The inline constructor clears the four words main sees cleared after
    // operator new; the destructor's term call is the second one in
    // mainInitGame.
    SysNowLoading() : m_image(NULL), m_packet(NULL), m_dmaChan(NULL), m_frame(0) {}
    ~SysNowLoading() { term(); }

    void init();                                                         // ROR 0x002052ac
    void term();                                                         // ROR 0x00205394

    sceGsDBuff     m_dbuff;     // ROR 0x000: sceGsSetDefDBuff(this, 0, 512, 448, ...)
    sceGsLoadImage m_loadImage; // ROR 0x230
    u_char        *m_image;     // ROR 0x290: 1 MB, LOADING.BIN
    u_int         *m_packet;    // ROR 0x294: 16 KB DMA packet
    sceDmaChan    *m_dmaChan;   // ROR 0x298: channel 2
    int            m_frame;     // ROR 0x29c: frames drawn
};
static_assert(offsetof(SysNowLoading, m_loadImage) == 0x230, "m_loadImage");
static_assert(offsetof(SysNowLoading, m_image) == 0x290, "m_image");

#endif /* _SYSTEM_SYSNOWLOADING_H */
