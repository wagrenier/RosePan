/* ==========================================================================
 *  mnu/mnumap.cpp
 *
 *  The map screen (0x1947a4..0x196a9c; RoR names no file here: mnu/ and
 *  the name from MnuMap and its pack mnumap.rpk, between mnuBase and
 *  MnuPBook). See mnu/mnumap.h.
 *
 *  PORT: all of MnuMap.
 * ======================================================================== */
#include "mnu/mnumap.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bed/bedpad.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "i3d/i3dpacket.h"
#include "mag/magscene.h"
#include "sequence/sequence.h"
#include "snd/sndman.h"
#include "wrd/wrdsrv.h"

/* ---- data ---- */

/* The floors' maps (data 0x29bfb8). */
static const MnuMapRow mnuMapRows[] = {
    { 0, 1, "map_al", 24, 0 },          // 0x0029bfb8
    { 0, 2, "map_au", 24, 1 },          // 0x0029bfd8
    { 0, 3, "map_am", 24, 3 },          // 0x0029bff8
    { 2, 0, "map_gu",  4, 4 },          // 0x0029c018
    { 2, 1, "map_g1",  4, 5 },          // 0x0029c038
    { 1, 4, "map_ou",  4, 4 },          // 0x0029c058
    { 1, 1, "map_o1",  4, 5 },          // 0x0029c078
    { 1, 2, "map_o2",  4, 8 },          // 0x0029c098
    { 1, 3, "map_or",  4, 6 },          // 0x0029c0b8
    { -1, -1, "", 0, -1 },              // 0x0029c0d8
};

/* toMapPos' scales (rodata 0x2f7dc0; the function copies them to its
 * stack first). */
static const MnuMapScale mnuMapScales[13] = {
    { 38400.0f, 6400.0f, 3072.0f, 512.0f,  24.0f, 514.0f, { 0.0f, 0.0f } },  // 0x002f7dc0
    {  6400.0f, 6400.0f,  512.0f, 512.0f, 128.0f, 354.0f, { 0.0f, 0.0f } },  // 0x002f7de0
    {  6400.0f, 6400.0f,  512.0f, 512.0f, 155.0f, 185.0f, { 0.0f, 0.0f } },  // 0x002f7e00
    { 38400.0f, 8550.0f, 3072.0f, 512.0f,  24.0f, 448.0f, { 0.0f, 0.0f } },  // 0x002f7e20
    { 38400.0f, 8550.0f, 3072.0f, 512.0f,  24.0f, 448.0f, { 0.0f, 0.0f } },  // 0x002f7e40
    { 41160.0f, 7850.0f, 3072.0f, 512.0f,  44.0f, 465.0f, { 0.0f, 0.0f } },  // 0x002f7e60
    { 39600.0f, 8550.0f, 3072.0f, 512.0f,  48.0f, 448.0f, { 0.0f, 0.0f } },  // 0x002f7e80
    { 39000.0f, 8550.0f, 3072.0f, 512.0f,  45.0f, 448.0f, { 0.0f, 0.0f } },  // 0x002f7ea0
    { 39600.0f, 8550.0f, 3072.0f, 512.0f,  60.0f, 448.0f, { 0.0f, 0.0f } },  // 0x002f7ec0
    { 39300.0f, 8550.0f, 3072.0f, 512.0f,  72.0f, 448.0f, { 0.0f, 0.0f } },  // 0x002f7ee0
    { 38400.0f, 8550.0f, 3072.0f, 512.0f,  24.0f, 448.0f, { 0.0f, 0.0f } },  // 0x002f7f00
    { 32880.0f, 5510.0f, 3072.0f, 512.0f, -54.0f, 576.0f, { 0.0f, 0.0f } },  // 0x002f7f20
    { 32880.0f, 5510.0f, 3072.0f, 512.0f, -54.0f, 534.0f, { 0.0f, 0.0f } },  // 0x002f7f40
};

/* The pack (the constructor builds it in place) and the files (rodata
 * 0x2f7d58 / 0x2f7d68 / 0x2f7db0). */
static const char mnuMapPack[] = "data/mnurpk/mnumap.rpk";
static const char mnuMapTex[] = "map01_tcc.tm2";
static const char mnuMapBgTex[] = "map_bg_tcc.tm2";
static const char mnuMapStripFmt[] = "%s_%02d.tm2";
/* The room-name window's text without a map (built inline by open). */
static const char mnuMapNoMap[] = "N O     M A P";

/* The view's place on the screen (512 x 448). */
static const float MNU_MAP_VIEW_X = 64.0f;
static const float MNU_MAP_VIEW_Y = 112.0f;

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001947a4; callcheck: mnuBase, MsgMan (the inline base constructor; the MsgManSet member: implicit on the host)

MnuMap::MnuMap()
{
    m_type = 4;
    strcpy(m_packName, mnuMapPack);
    m_helpWindow = -1;
    m_nameWindow = -1;
}

// ROR 0x00194858; callcheck: ~MsgMan, ~mnuBase, operator_delete (member / base destructors and the deleting variant: implicit on the host)

MnuMap::~MnuMap()
{
    MnuMap::term();
}

// ──────────────────────────────────────────────────────────────────────
// Lays out the work (0x1b files, 0x1a textures: the two base ones and
// up to 24 strips) and opens the pack; open picks the floor and queues
// the files, processPad loads them.
// ROR 0x00194908 // callcheck: openPack (the ROM calls slot 10 through the vtable)

void MnuMap::init(DogTexInfo *tex)
{
    setupWork(tex, 0x1b, 0, 0, 0x1a);
    m_step = 0;
    openPack();
    open();
}

// ROR 0x00194960

void MnuMap::term()
{
    if (m_nameWindow != -1) {
        m_scene->closeWindow(m_nameWindow);
        m_nameWindow = -1;
    }
    restoreVolume();
    m_msg.close();
    mnuBase::term();
}

// ──────────────────────────────────────────────────────────────────────
// The row of the PC's area and floor (or the terminator: N O MAP), the
// PC's and Brown's map positions and the PC's heading, the view centred
// on the PC, the room-name window, the base textures and the strips.
// ROR 0x001949bc

void MnuMap::open()
{
    const WrdEntry *room = g_wrdSrv.get(g_gamMan.getWrdNo());
    m_scrollFrom[0] = 0.0f;
    m_animTime = 1;
    m_animLen = 1;
    m_scrollFrom[1] = 0.0f;
    m_flags |= MNU_MAP_BUSY;
    m_openTime = -1;
    m_alpha = 0;
    m_alphaStep = 0;
    m_moveCount = 0;

    int area = g_gamMan.getMapArea();
    int floor = gamGetMapFloor(&g_gamMan);
    /* PORT: the ROM reads the room without a NULL test. */
    if (room == NULL || room->m_18 == 0)
        floor = 0;
    m_pcRow = mnuMapRows;
    while (m_pcRow->m_area != -1 && !(m_pcRow->m_area == area && m_pcRow->m_floor == floor))
        m_pcRow++;
    m_row = m_pcRow;
    if (m_row->m_area == -1)
        m_flags |= MNU_MAP_NONE;
    m_mapH = 0x200;
    m_mapW = m_row->m_numStrips << 7;

    static GamMapMarker markers[0x80];      /* the ROM's 0x1000-byte stack buffer */
    int n = gamGetMapMarkers(&g_gamMan, m_row->m_floor, markers);
    m_dogPos[0] = -100000.0f;
    m_pcPos[1] = -100000.0f;
    m_pcPos[0] = -100000.0f;
    m_dogPos[1] = -100000.0f;
    if (!(m_flags & MNU_MAP_NONE)) {
        /* the markers say who is there; the positions are GamMan's */
        for (int i = 0; i < n; i++) {
            float pos[4];
            if (markers[i].m_kind == 1) {
                memcpy(pos, g_gamMan.m_pcPos, sizeof(pos));
                toMapPos(pos);
                m_pcPos[0] = pos[0];
                m_pcPos[1] = pos[2];
            } else if (markers[i].m_kind == 2) {
                memcpy(pos, g_gamMan.m_dogPos, sizeof(pos));
                toMapPos(pos);
                m_dogPos[0] = pos[0];
                m_dogPos[1] = pos[2];
            }
        }
        m_pcAngle = g_gamMan.m_pcRot[1] + 3.1415923f;
        while (m_pcAngle < -3.1415923f)
            m_pcAngle += 6.2831845f;
        while (m_pcAngle >= 3.1415923f)
            m_pcAngle -= 6.2831845f;
    }
    getViewSize(m_scroll);
    m_scroll[0] = m_pcPos[0] - m_scroll[0] * 0.5f;
    m_scroll[1] = m_pcPos[1] - m_scroll[1] * 0.5f;
    clampScroll(m_scroll);

    int cc = 0;
    if (!(m_flags & MNU_MAP_NONE)) {
        // the room's name under the view
        const char *text = room ? room->m_text : "";
        m_nameWindow = m_scene->openWindow(0x100, 0x20, 0, -1);
        int w = m_scene->getWindowTextWidth(m_nameWindow, text);
        m_scene->getWindowWordCc(m_nameWindow, &cc);
        m_scene->setWindowPos(m_nameWindow, (0x200 - w) / 2, 0x188 - cc, 0x7fff);
        m_scene->setWindowSize(m_nameWindow, 0x100, 0x20);
        m_scene->setWindowWords84(m_nameWindow, 0, 0, 0x100, 0x20);
        m_scene->setWindowFillMsb(m_nameWindow, 1);
        m_scene->setWindowFill(m_nameWindow, 0);
        m_scene->updateWindowFont(m_nameWindow);
        m_scene->setWindowShow(m_nameWindow, 1);
        m_scene->drawView(m_nameWindow, 0, 0, text, 0);
    } else {
        // no map: the notice in the middle of the screen
        m_nameWindow = m_scene->openWindow(0x100, 0x20, 0, -1);
        int w = m_scene->getWindowTextWidth(m_nameWindow, mnuMapNoMap);
        m_scene->getWindowWordCc(m_nameWindow, &cc);
        m_scene->setWindowPos(m_nameWindow, (0x200 - w) / 2, 0xf0 - cc / 2, 0x7fff);
        m_scene->setWindowSize(m_nameWindow, 0x100, 0x20);
        m_scene->setWindowWords84(m_nameWindow, 0, 0, 0x100, 0x20);
        m_scene->setWindowFillMsb(m_nameWindow, 1);
        m_scene->setWindowFill(m_nameWindow, 0);
        m_scene->updateWindowFont(m_nameWindow);
        m_scene->setWindowShow(m_nameWindow, 1);
        m_scene->drawView(m_nameWindow, 0, 0, mnuMapNoMap, 0);
    }
    addFile(mnuMapTex, MNU_FILE_TEX, 0);
    addFile(mnuMapBgTex, MNU_FILE_TEX, 1);
    queueStrips();
}

// ROR 0x00194fa4

void MnuMap::start()
{
    pushCamera(256.0f, 1.5707963f);
    lowerVolume();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00194fe4 // callcheck: term (the ROM calls slot 3 through the vtable)

int MnuMap::processPad(BedPad *pad)
{
    if (m_animTime < m_animLen && ++m_animTime == m_animLen)
        m_flags &= ~(MNU_MAP_ZOOMING | MNU_MAP_SCROLLING | MNU_MAP_BUSY);
    if ((u_int)m_openTime < 18)
        m_openTime++;
    if (m_alphaStep != 0) {
        m_alpha += m_alphaStep;
        if (m_alpha > 15)
            m_alpha = 15;
        else if (m_alpha < 0)
            m_alpha = 0;
        if ((u_int)(m_alpha - 1) > 13)
            m_alphaStep = 0;
    }
    updateLoad();
    m_10 = m_step;
    switch (m_step) {
    case 0:
        if (m_scene->getFadeState() > 4)
            m_step = 1;
        break;
    case 1:
        if (isTexLoaded(1) && isTexLoaded(0)) {
            start();
            m_scene->fadeIn(15, 7, 0);
            m_openTime = 0;
            g_sndMan.playSe(5);
            m_step = 2;
        }
        break;
    case 2:
        if (m_scene->getFadeState() < 5 || m_openTime < 18 || m_numFiles != 0)
            return 0;
        m_step = 3;
        m_alphaStep = 1;
        break;
    case 3:
        if (m_alphaStep == 0) {
            m_scene->setWindowShow(m_nameWindow, 0);
            m_msg.open();
            showHelp(1);
            m_step = 4;
            m_flags &= ~MNU_MAP_BUSY;
        }
        break;
    case 4:
        if (updatePad(pad) == 0) {
            m_scene->fadeOut(15, 7, 0);
            g_sndMan.playSe(2);
            m_scene->setWindowShow(m_nameWindow, 1);
            showHelp(0);
            m_step = 5;
        }
        break;
    case 5:
        if (m_scene->getFadeState() > 4)
            m_step = 10;
        break;
    case 6:
        // a floor change: the strips fade out
        cancelLoad();
        m_scene->setWindowShow(m_nameWindow, 1);
        m_alphaStep = -1;
        m_step = 7;
        break;
    case 7:
        if (m_alpha != 0 || m_numFiles != 0)
            return 0;
        if (m_flags & MNU_MAP_CENTRE) {
            float w, h;
            if (m_flags & MNU_MAP_ZOOM) {
                w = 192.0f;
                h = 128.0f;
            } else {
                w = 384.0f;
                h = 256.0f;
            }
            m_scroll[0] = m_pcPos[0] - w;
            m_scroll[1] = m_pcPos[1] - h;
            clampScroll(m_scroll);
        } else {
            for (int i = 0; i < m_row->m_numStrips; i++)
                freeTex(i + 2);
            if (m_flags & MNU_MAP_NEXT)
                m_row++;
            else if (m_flags & MNU_MAP_PREV)
                m_row--;
        }
        queueStrips();
        m_step = 8;
        break;
    case 8:
        if (m_numFiles != 0)
            return 0;
        m_step = 9;
        m_alphaStep = 1;
        break;
    case 9:
        if (m_alpha == 15) {
            m_flags &= ~(MNU_MAP_NEXT | MNU_MAP_PREV | MNU_MAP_CENTRE | MNU_MAP_BUSY);
            if (isCurrentFloor())
                m_scene->setWindowShow(m_nameWindow, 0);
            m_step = 4;
        }
        break;
    case 10:
        popCamera();
        term();
        return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Step 4's input: Circle or Select closes (0); R1 / L1 go to the next /
// previous floor of the area, Square zooms, Triangle centres on the PC
// (its own floor only), anything else of those buzzes; the left stick or
// the d-pad scrolls 16 pixels a frame. The strips follow the scroll.
// ROR 0x00195430

int MnuMap::updatePad(BedPad *pad)
{
    m_msg.update();
    if (!(m_flags & MNU_MAP_BUSY)) {
        u_short trig = pad->m_trig;
        u_int change = 0;
        if ((trig & 0x2000) || (trig & 0x0001)) {      // Circle, Select
            m_flags |= MNU_MAP_BUSY;
            return 0;
        }
        /* PORT: the ROM reads the neighbouring row before it tests
         * MNU_MAP_NONE, past the table when m_row is its terminator. */
        if (trig & 0x0800) {                            // R1
            if (!(m_flags & MNU_MAP_NONE) && m_row->m_area == m_row[1].m_area)
                change = MNU_MAP_NEXT;
        } else if (trig & 0x0400) {                     // L1
            if (!(m_flags & MNU_MAP_NONE) && m_row > mnuMapRows &&
                m_row->m_area == m_row[-1].m_area)
                change = MNU_MAP_PREV;
        } else if (trig & 0x8000) {                     // Square: zoom
            if (!(m_flags & MNU_MAP_NONE)) {
                m_flags = (m_flags | MNU_MAP_ZOOMING | MNU_MAP_BUSY) ^ MNU_MAP_ZOOM;
                m_animLen = 18;
                m_animTime = 0;
                m_scrollFrom[0] = m_scroll[0];
                m_scrollFrom[1] = m_scroll[1];
                float dx, dy;
                if (m_flags & MNU_MAP_ZOOM) {
                    dx = 192.0f;
                    dy = 128.0f;
                } else {
                    dx = -192.0f;
                    dy = -128.0f;
                }
                m_scroll[0] += dx;
                m_scroll[1] += dy;
                clampScroll(m_scroll);
                g_sndMan.playSe(4);
                queueStrips();
                return 1;
            }
        } else if (trig & 0x1000) {                     // Triangle: centre
            if (isCurrentFloor() && !(m_flags & MNU_MAP_NONE))
                change = MNU_MAP_CENTRE;
        } else {
            float to[2];
            to[0] = pad->getLeftX() * 16.0f;
            to[1] = pad->getLeftY() * 16.0f;
            if (to[0] == 0.0f && to[1] == 0.0f) {
                u_short hold = pad->m_hold;
                int x = (hold >> 5) & 1;                // right
                if ((hold >> 7) & 1)                    // left
                    x--;
                int y = (hold >> 6) & 1;                // down
                if ((hold >> 4) & 1)                    // up
                    y--;
                to[0] = (float)x * 16.0f;
                to[1] = (float)y * 16.0f;
            }
            to[0] += m_scroll[0];
            to[1] += m_scroll[1];
            clampScroll(to);
            int still = 1;
            float d = to[0] - m_scroll[0];
            if (d >= 0.5f) {
                m_scroll[0] = to[0];
                m_flags |= MNU_MAP_MOVED_X;
                still = 0;
            } else if (d <= -0.5f) {
                m_scroll[0] = to[0];
                m_flags &= ~MNU_MAP_MOVED_X;
                still = 0;
            }
            d = to[1] - m_scroll[1];
            if (d >= 0.5f) {
                m_scroll[1] = to[1];
                still = 0;
                m_flags |= MNU_MAP_MOVED_Y;
            } else if (d <= -0.5f) {
                m_scroll[1] = to[1];
                still = 0;
                m_flags &= ~MNU_MAP_MOVED_Y;
            }
            if (still)
                m_moveCount = 0;
            else
                m_moveCount = (m_moveCount + 1) % 12;
            queueStrips();
            return 1;
        }
        if (change != 0) {
            m_step = 6;
            m_flags |= change | MNU_MAP_BUSY;
            g_sndMan.playSe(4);
        } else {
            g_sndMan.playSe(3);
        }
    }
    queueStrips();
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0019586c

void MnuMap::clampScroll(float *pos)
{
    float view[2];
    getViewSize(view);
    float size = (float)m_mapW;
    if (size < view[0])
        pos[0] = size * 0.5f - view[0] * 0.5f;
    else if (pos[0] < 0.0f)
        pos[0] = 0.0f;
    else if (size - view[0] < pos[0])
        pos[0] = size - view[0];
    size = (float)m_mapH;
    if (size < view[1])
        pos[1] = size * 0.5f - view[1] * 0.5f;
    else if (pos[1] < 0.0f)
        pos[1] = 0.0f;
    else if (size - view[1] < pos[1])
        pos[1] = size - view[1];
}

// ROR 0x00195958

int MnuMap::isScrollInRange(float x, float y)
{
    float view[2];
    getViewSize(view);
    if (0.0f <= x && x <= (float)m_mapW - view[0] &&
        0.0f <= y && y <= (float)m_mapH - view[1])
        return 1;
    return 0;
}

// ROR 0x001959ec

void MnuMap::getViewSize(float *out)
{
    float h;
    if (m_flags & MNU_MAP_ZOOM) {
        h = 256.0f;
        out[0] = 384.0f;
    } else {
        h = 512.0f;
        out[0] = 768.0f;
    }
    out[1] = h;
}

// ──────────────────────────────────────────────────────────────────────
// The strip column under the view's left edge (mid-zoom too): strips
// more than 3 left or 9 right of it freed, those from 2 left to 7 right
// queued (texture slot strip + 2, file number strip + 1).
// ROR 0x00195a34

int MnuMap::queueStrips()
{
    float t = getScrollRate();
    int col = (int)(m_scrollFrom[0] + (m_scroll[0] - m_scrollFrom[0]) * t);
    if (col < 0)
        col += 0x7f;
    col >>= 7;
    for (int i = 0; i < col - 3; i++)
        freeTex(i + 2);
    for (int i = col + 9; i < m_row->m_numStrips; i++)
        freeTex(i + 2);
    /* the ROM's start for column 1 is strip -1: texture slot 1, file 00 */
    int i = col > 0 ? col - 2 : 0;
    while (i < col + 8 && i < m_row->m_numStrips) {
        int slot = i + 2;
        i++;
        if (!isTexLoaded(slot)) {
            char name[0x20];
            sprintf(name, mnuMapStripFmt, m_row->m_name, i);
            addFile(name, MNU_FILE_TEX, slot);
        }
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// The help line: six button sprites and the help window, which shows
// system text 0x18; show 0 hides them and closes the window.
// ROR 0x00195b78

void MnuMap::showHelp(int show)
{
    static const int sprites[6][2] = {
        { 7, 0x10 }, { 6, 0x88 }, { 0xc, 0xfc }, { 10, 0x118 }, { 9, 0x184 }, { 4, 0x1a8 },
    };
    if (!show) {
        for (int i = 0; i < 6; i++)
            g_gamMan.showMesSprite(sprites[i][0], 0, 0);
        m_scene->setWindowShow(m_helpWindow, 1);
        if (m_helpWindow != -1) {
            m_scene->closeWindow(m_helpWindow);
            m_helpWindow = -1;
        }
        return;
    }
    for (int i = 0; i < 6; i++) {
        g_gamMan.setMesSpritePos(sprites[i][0], sprites[i][1], 0x198);
        g_gamMan.fadeInMesSprite(sprites[i][0], 0);
        g_gamMan.showMesSprite(sprites[i][0], 1, 0);
    }
    if (m_helpWindow == -1)
        m_helpWindow = m_scene->openWindow(0x200, 0x20, 0, -1);
    m_scene->setWindowPos(m_helpWindow, 0, 0x19a, 0x7fff);
    m_scene->setWindowSize(m_helpWindow, 0x200, 0x20);
    m_scene->setWindowWords84(m_helpWindow, 0, 0, 0x200, 0x20);
    m_scene->setWindowFillMsb(m_helpWindow, 1);
    m_scene->setWindowFill(m_helpWindow, 0);
    m_scene->updateWindowFont(m_helpWindow);
    m_scene->setWindowShow(m_helpWindow, 0);
    m_msg.setMessage(m_helpWindow, msgGetSysText(0x18), 0);
}

// ──────────────────────────────────────────────────────────────────────
// The view (the strips at m_alpha, the PC's and, once the script's bit
// 0x22 is set, Brown's marker on the PC's floor), the frame, the area and
// floor labels, the scroll arrows and the frame lines, which grow over
// the first 18 frames.
// ROR 0x00195e90

void MnuMap::draw(I3dLayer *layer)
{
    if ((u_int)(m_10 - 2) >= 8)
        return;

    float scale = (m_flags & MNU_MAP_ZOOM) ? 1.0f : 0.5f;
    float sx, sy;
    if (m_flags & MNU_MAP_ZOOMING) {
        float t = getZoomRate();
        sx = m_scrollFrom[0] + (m_scroll[0] - m_scrollFrom[0]) * t;
        sy = m_scrollFrom[1] + (m_scroll[1] - m_scrollFrom[1]) * t;
        if (m_flags & MNU_MAP_ZOOM)
            scale = t * 0.5f + 0.5f;
        else
            scale = t * -0.5f + 1.0f;
    } else if (m_flags & MNU_MAP_SCROLLING) {
        float t = getScrollRate();
        sx = m_scrollFrom[0] + (m_scroll[0] - m_scrollFrom[0]) * t;
        sy = m_scrollFrom[1] + (m_scroll[1] - m_scrollFrom[1]) * t;
    } else {
        sx = m_scroll[0];
        sy = m_scroll[1];
    }

    beginDraw(layer);
    m_pkt = i3dPktSetReg(m_pkt, 0x42 /* ALPHA_1 */, 0x44);
    MnuSprite spr;
    if (m_alpha > 0) {
        // the strips, 128 x 512 each
        spr.m_x = 0.0f;
        spr.m_y = 0.0f;
        spr.m_w = scale * 128.0f;
        spr.m_h = scale * 512.0f;
        spr.m_uv[0] = 0.0078125f;
        spr.m_uv[1] = 0.001953125f;
        spr.m_uv[2] = 0.9921875f;
        spr.m_uv[3] = 0.99804688f;
        spr.m_color[0] = 1.0f;
        spr.m_color[1] = 1.0f;
        spr.m_color[2] = 1.0f;
        spr.m_color[3] = (float)m_alpha / 15.0f;
        spr.m_angle = 0.0f;
        for (int i = 0; i < m_row->m_numStrips; i++) {
            if (isTexLoaded(i + 2)) {
                useTex(i + 2);
                drawSprite(((float)i * 128.0f - sx) * scale + MNU_MAP_VIEW_X,
                           -sy * scale + MNU_MAP_VIEW_Y, &spr);
            }
        }
    }
    if (m_alpha == 15 && isCurrentFloor()) {
        // the PC, turned to its heading
        useTex(0);
        float half = scale * 16.0f;
        spr.m_x = half;
        spr.m_y = scale * 12.0f;
        spr.m_w = scale * 32.0f;
        spr.m_h = scale * 32.0f;
        spr.m_uv[0] = 0.375f;
        spr.m_uv[1] = 0.46875f;
        spr.m_uv[2] = 0.4375f;
        spr.m_uv[3] = 0.59375f;
        spr.m_color[0] = 1.0f;
        spr.m_color[1] = 1.0f;
        spr.m_color[2] = 1.0f;
        spr.m_color[3] = 1.0f;
        spr.m_angle = m_pcAngle;
        drawSprite((m_pcPos[0] - sx) * scale + MNU_MAP_VIEW_X,
                   (m_pcPos[1] - sy) * scale + MNU_MAP_VIEW_Y, &spr);
        if (g_sequence.m_bits[1] & 4) {
            // Brown
            spr.m_x = half;
            spr.m_y = half;
            spr.m_uv[0] = 0.25f;
            spr.m_uv[1] = 0.46875f;
            spr.m_uv[2] = 0.3125f;
            spr.m_uv[3] = 0.59375f;
            spr.m_angle = 0.0f;
            drawSprite((m_dogPos[0] - sx) * scale + MNU_MAP_VIEW_X,
                       (m_dogPos[1] - sy) * scale + MNU_MAP_VIEW_Y, &spr);
        }
    }

    // the frame
    useTex(1);
    spr.m_x = 256.5f;
    spr.m_y = 224.5f;
    spr.m_w = 513.0f;
    spr.m_h = 449.0f;
    spr.m_uv[0] = 0.0f;
    spr.m_uv[1] = 0.0f;
    spr.m_uv[2] = 1.0019531f;
    spr.m_uv[3] = 0.87695312f;
    spr.m_color[0] = 1.0f;
    spr.m_color[1] = 1.0f;
    spr.m_color[2] = 1.0f;
    spr.m_color[3] = 1.0f;
    spr.m_angle = 0.0f;
    drawSprite(256.5f, 224.5f, &spr);

    useTex(0);
    spr.m_x = 0.0f;
    spr.m_y = 0.0f;
    if (!(m_flags & MNU_MAP_NONE)) {
        // the area's name, a 40-pixel row of map01_tcc per area
        spr.m_w = 320.0f;
        spr.m_h = 40.0f;
        spr.m_uv[0] = 0.25f;
        spr.m_uv[1] = (float)m_row->m_area * 40.0f * (1.0f / 256.0f);
        spr.m_uv[2] = 0.875f;
        spr.m_uv[3] = (float)(m_row->m_area + 1) * 40.0f * (1.0f / 256.0f);
        drawSprite(24.0f, 24.0f, &spr);
        // the floor's, a 20-pixel row
        spr.m_w = 128.0f;
        spr.m_h = 20.0f;
        spr.m_uv[0] = 0.0f;
        spr.m_uv[1] = (float)m_row->m_label * 20.0f * (1.0f / 256.0f);
        spr.m_uv[2] = 0.25f;
        spr.m_uv[3] = (float)(m_row->m_label + 1) * 20.0f * (1.0f / 256.0f);
        drawSprite(80.0f, 78.0f, &spr);
    }
    // the two marks on the label line
    spr.m_w = 8.0f;
    spr.m_h = 16.0f;
    spr.m_uv[0] = 0.25f;
    spr.m_uv[1] = 0.59375f;
    spr.m_uv[2] = 0.265625f;
    spr.m_uv[3] = 0.65625f;
    drawSprite(72.0f, 80.0f, &spr);
    spr.m_uv[0] = 0.265625f;
    spr.m_uv[2] = (spr.m_w + 136.0f) * (1.0f / 512.0f);
    drawSprite(208.0f, 80.0f, &spr);

    if (m_openTime >= 0) {
        // the frame lines, 12 pixels thick, growing with m_openTime / 18
        spr.m_uv[0] = 0.0f;
        spr.m_h = 12.0f;
        spr.m_uv[1] = 0.765625f;
        spr.m_uv[3] = 0.8125f;
        spr.m_w = (float)m_openTime * 144.0f / 18.0f;
        spr.m_uv[2] = (spr.m_w + 0.0f) * (1.0f / 512.0f);
        drawSprite(190.0f, 392.0f, &spr);
        if (m_openTime >= 0) {
            // top and bottom
            spr.m_h = 12.0f;
            spr.m_uv[1] = 0.90625f;
            spr.m_uv[0] = 0.0f;
            spr.m_uv[3] = 0.953125f;
            spr.m_w = (float)m_openTime * 448.0f / 18.0f;
            spr.m_uv[2] = (spr.m_w + 0.0f) * (1.0f / 512.0f);
            drawSprite(32.0f, 106.0f, &spr);
            spr.m_uv[1] = 0.953125f;
            spr.m_uv[3] = (spr.m_h + 244.0f) * (1.0f / 256.0f);
            drawSprite(32.0f, 362.0f, &spr);
            // right and left, turned a quarter
            spr.m_h = 12.0f;
            spr.m_angle = 1.5707963f;
            spr.m_uv[1] = 0.8125f;
            spr.m_uv[0] = 0.0f;
            spr.m_uv[3] = 0.859375f;
            spr.m_w = (float)m_openTime * 304.0f / 18.0f;
            spr.m_uv[2] = (spr.m_w + 0.0f) * (1.0f / 512.0f);
            drawSprite(454.0f, 88.0f, &spr);
            spr.m_uv[1] = 0.859375f;
            spr.m_uv[3] = (spr.m_h + 220.0f) * (1.0f / 256.0f);
            drawSprite(70.0f, 88.0f, &spr);
        }
    }
    endDraw(layer);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00196760

int MnuMap::isCurrentFloor()
{
    return m_pcRow->m_area == m_row->m_area && m_pcRow->m_floor == m_row->m_floor;
}

// ROR 0x00196790

float MnuMap::getScrollRate()
{
    float c = cosf((((float)m_animTime * 180.0f) / (float)m_animLen) * 3.1415925f / 180.0f);
    return -c * 0.5f + 0.5f;
}

// ROR 0x00196804

float MnuMap::getZoomRate()
{
    float c = cosf((((float)m_animTime * 180.0f) / (float)m_animLen) * 3.1415925f / 180.0f);
    return -c * 0.5f + 0.5f;
}

// ──────────────────────────────────────────────────────────────────────
// The area's row of mnuMapScales, or in the attic (area 0, floor 3) the
// room's own; pos[0] / pos[2] plus the room's offset to map pixels.
// ROR 0x00196878

void MnuMap::toMapPos(float *pos)
{
    int area = g_gamMan.getMapArea();
    int floor = gamGetMapFloor(&g_gamMan);
    int wrd = g_gamMan.getWrdNo();
    const WrdEntry *room = g_wrdSrv.get(wrd);
    if (area == -1)
        return;
    /* PORT: the ROM indexes its table with any area and reads the room
     * without a NULL test. */
    if ((u_int)area >= 3 || room == NULL)
        return;
    const MnuMapScale *row = &mnuMapScales[area];
    if (area == 0 && floor == 3) {
        switch (wrd) {
        case 0x32: row = &mnuMapScales[12]; break;
        case 0x33: row = &mnuMapScales[11]; break;
        case 0x63: row = &mnuMapScales[3]; break;
        case 0x64: row = &mnuMapScales[4]; break;
        case 0x65: row = &mnuMapScales[5]; break;
        case 0x66: row = &mnuMapScales[6]; break;
        case 0x67: row = &mnuMapScales[7]; break;
        case 0x68: row = &mnuMapScales[8]; break;
        case 0x69: row = &mnuMapScales[9]; break;
        case 0x6a: row = &mnuMapScales[10]; break;
        }
    }
    pos[0] = (row->m_mapW * -(pos[0] + room->m_ofs[0])) / row->m_worldW + row->m_x;
    pos[2] = (row->m_mapH * (pos[2] + room->m_ofs[2])) / row->m_worldH + row->m_y;
}
