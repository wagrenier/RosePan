/* ==========================================================================
 *  mag/maglifewindow.h
 *
 *  MagLifeWindow (mag/maglifewindow.cpp; vtable _vt$13MagLifeWindow
 *  0x2e1d48, base MagLifePos <- MagLife, vptr at +0x14, constructor
 *  0x1308bc): a text window. The scene keeps up to MAX_WINDOW (4) of them
 *  at +0x2734 (MagScene::openWindow), each drawing into one of the scene's
 *  GamFont buffers. Minimal: what the scene touches.
 *  PORT: not polymorphic on the host yet (see mag/maglife.h); the ROM
 *  deletes windows through slot 1, the host with delete on this class.
 * ======================================================================== */
#ifndef _MAG_MAGLIFEWINDOW_H
#define _MAG_MAGLIFEWINDOW_H

#include <stddef.h>

#include "mag/maglife.h"

/* layout: host -- polymorphic in the ROM (vptr +0x14); EE offsets in the
 * comments. */
struct MagLifeWindow : MagLife
{
    /* EE slot 1 (+0x08), linkonce in the ROM. */
    ~MagLifeWindow();                                                    // ROR 0x00285234

    u_char _28[0x80 - 0x28];
    int    m_wait_key;          // ROR 0x80: 0 = close at the next scene update, > 0 = close on pad bit 14, < 0 = stay (MagScene::update sub mode 2)
};
static_assert(offsetof(MagLifeWindow, m_wait_key) >= 0x80, "m_wait_key");

#endif /* _MAG_MAGLIFEWINDOW_H */
