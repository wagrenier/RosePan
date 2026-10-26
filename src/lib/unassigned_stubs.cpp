// lib/unassigned_stubs.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.
#include <math.h>
#include <string.h>
#include "game/gamsnd.h"
#include "rx/rxsnd.h"
#include "mnu/mnuSelectLanguage.h"
#include "msg/msgman.h"

#include "dog/dogentity.h"
#include "mag/maglifebwn.h"
#include "mag/magscene.h"
#include "dog/dogcamera.h"
#include "dog/doglightpool.h"

#include "common/rosepan_stub.h"
#include "game/gamman.h"
#include "lib/unassigned.h"
#include "mag/magsystem.h"
#include "rx/rxfile.h"
#include "rx/rxsys.h"
#include "rx/rxthread.h"
#include "sequence/sequence.h"
#include "snd/sndman.h"
#include "utl/utltm2.h"

// ---- globals (hand-written) ----
// (The global managers moved to system/system.cpp.)

WrdTable   g_wrdTable;          /* data 0x298ee0: data/wrd/WRD.BIN's table */

int g_emyManagerTaskId;         /* bss 0x7b45c0 */
int g_bss_7a3d48;               /* bss 0x7a3d48 */
int g_bss_7a3d4c;               /* bss 0x7a3d4c */
int g_bss_7a3210;               /* bss 0x7a3210 */
int g_bss_7812ac;               /* bss 0x7812ac */
int g_bss_780d04;               /* bss 0x780d04: GamMan option 0 (brightness) */

const char *g_versionNumber = "1.01";   /* data 0x2d9e18 -> 0x301720 */

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x002155a0
void debugNop(void)
{
    ROSEPAN_STUB("debugNop", 0x002155a0);
}

// ---- bridges (hand-written) ----

// stub of ROR 0x001f47f0 (a constructor: stubgen leaves it to hand)
TskSeqEvent::TskSeqEvent(u_int, int, const void *)
{
    ROSEPAN_STUB("TskSeqEvent::TskSeqEvent", 0x001f47f0);
    m_radius = 0.0f;
}

// stub of ROR 0x001f4a24 (a constructor: stubgen leaves it to hand)
TskMsgMan::TskMsgMan(const char *, int, const int *, int, int, int)
{
    ROSEPAN_STUB("TskMsgMan::TskMsgMan", 0x001f4a24);
}

// stub of ROR 0x001f8fc0 (a constructor: stubgen leaves it to hand)
TskSnd::TskSnd(int kind, const TskSndParam *param)
{
    ROSEPAN_STUB("TskSnd::TskSnd", 0x001f8fc0);
}

// stub of ROR 0x001f184c (a constructor: stubgen leaves it to hand)
TskFrameFunc::TskFrameFunc(int kind, intptr_t arg, int frames)
{
    ROSEPAN_STUB("TskFrameFunc::TskFrameFunc", 0x001f184c);
}

// stub of ROR 0x001f438c (a constructor: stubgen leaves it to hand)
TskMovie::TskMovie(const char *)
{
    ROSEPAN_STUB("TskMovie::TskMovie", 0x001f438c);
}

// stub of ROR 0x001e9a3c (a constructor: stubgen leaves it to hand)
TskMoveTypeGen::TskMoveTypeGen(u_int, int, intptr_t, int, float, float)
{
    ROSEPAN_STUB("TskMoveTypeGen::TskMoveTypeGen", 0x001e9a3c);
}

// stub of ROR 0x001eb63c (a constructor: stubgen leaves it to hand)
TskMoveTypeRound::TskMoveTypeRound(u_int, int, intptr_t, int, float, float, float)
{
    ROSEPAN_STUB("TskMoveTypeRound::TskMoveTypeRound", 0x001eb63c);
}

// stub of ROR 0x001ec344 (a constructor: stubgen leaves it to hand)
TskMovePointPath::TskMovePointPath(u_int, int, intptr_t, int, int, float, float)
{
    ROSEPAN_STUB("TskMovePointPath::TskMovePointPath", 0x001ec344);
}

// stub of ROR 0x001ec940 (a constructor: stubgen leaves it to hand)
TskMoveSectaFindPath::TskMoveSectaFindPath(u_int, intptr_t, int, int, int, int, float, float)
{
    ROSEPAN_STUB("TskMoveSectaFindPath::TskMoveSectaFindPath", 0x001ec940);
}

// stub of ROR 0x001ed998 (a constructor: stubgen leaves it to hand)
TskMoveSecta::TskMoveSecta(u_int, int, intptr_t, int, int, float, float)
{
    ROSEPAN_STUB("TskMoveSecta::TskMoveSecta", 0x001ed998);
}

// stub of ROR 0x001fbc0c (a constructor: stubgen leaves it to hand)
TskCamera::TskCamera(intptr_t, intptr_t)
{
    ROSEPAN_STUB("TskCamera::TskCamera", 0x001fbc0c);
}

// stub of ROR 0x001f5244 (a constructor: stubgen leaves it to hand)
TskAttach::TskAttach(u_int oid, const int *value, int arg)
{
    ROSEPAN_STUB("TskAttach::TskAttach", 0x001f5244);
}

// stub of ROR 0x001f4184 (a constructor: stubgen leaves it to hand)
TskSysFunc::TskSysFunc(int kind, int a, intptr_t arg)
{
    ROSEPAN_STUB("TskSysFunc::TskSysFunc", 0x001f4184);
}

// stub of ROR 0x0010421c
void utlRandReset(void)
{
    ROSEPAN_STUB("utlRandReset", 0x0010421c);
}

// stub of ROR 0x0010ff74
void DogEntity::normalizeRotate()
{
    ROSEPAN_STUB("DogEntity::normalizeRotate", 0x0010ff74);
}

// stub of ROR 0x0010fc7c
void DogEntity::setRotate(float x, float y, float z)
{
    ROSEPAN_STUB("DogEntity::setRotate", 0x0010fc7c);
}

// stub of ROR 0x00215b5c
void dbgTraceVector3(float x, float y, float z)
{
    ROSEPAN_STUB("dbgTraceVector3", 0x00215b5c);
}

// stub of ROR 0x00176afc
int FUN_00176afc(Obj176a3c *obj, i3dContext *ctx, MagScene *scene)
{
    ROSEPAN_STUB("FUN_00176afc", 0x00176afc);
    return {};
}

// stub of ROR 0x0017c2c0
void FUN_0017c2c0(Obj17c0f0 *obj)
{
    ROSEPAN_STUB("FUN_0017c2c0", 0x0017c2c0);
}

// stub of ROR 0x001944d0
void FUN_001944d0(void)
{
    ROSEPAN_STUB("FUN_001944d0", 0x001944d0);
}

// stub of ROR 0x001259e8
void MagLifeBwn::renderPostProcess()
{
    ROSEPAN_STUB("MagLifeBwn::renderPostProcess", 0x001259e8);
}

// stub of ROR 0x0017c1f4
int FUN_0017c1f4(Obj17c0f0 *obj, void *data, int copy)
{
    ROSEPAN_STUB("FUN_0017c1f4", 0x0017c1f4);
    return {};
}

// stub of ROR 0x00140030
void MagScene::loadTenkyuTex(int no, const BedPfh *pfh, i3dContext *ctx)
{
    ROSEPAN_STUB("MagScene::loadTenkyuTex", 0x00140030);
}

// stub of ROR 0x0013f8e4
void MagScene::setDefaultLight()
{
    ROSEPAN_STUB("MagScene::setDefaultLight", 0x0013f8e4);
}

// stub of ROR 0x00143d7c
void MagScene::setFog(const I3dFog *fog, int frames)
{
    ROSEPAN_STUB("MagScene::setFog", 0x00143d7c);
}

// stub of ROR 0x00215bb4
void dbgTraceVector4(const float *v)
{
    ROSEPAN_STUB("dbgTraceVector4", 0x00215bb4);
}

// stub of ROR 0x0021a828
int i3rCopyClipSphere(I3rRoot *root, float *center, float *radius)
{
    ROSEPAN_STUB("i3rCopyClipSphere", 0x0021a828);
    return {};
}

// stub of ROR 0x0021a938
int i3rReplaceStqMapEntity(I3rRoot *root, I3rStqMapEntity *entity)
{
    ROSEPAN_STUB("i3rReplaceStqMapEntity", 0x0021a938);
    return {};
}

// stub of ROR 0x0010fa6c
sceVu0FVECTOR * DogEntity::getMatrix()
{
    ROSEPAN_STUB("DogEntity::getMatrix", 0x0010fa6c);
    // PC: the unit matrix until the camera class that holds the +0xf0
    // matrix is written; i3dCtxCreateView copies what this returns.
    return i3dMatGetUnit();
}

// stub of ROR 0x0017ea44
WrdEntry * WrdTable::get(int n)
{
    ROSEPAN_STUB("WrdTable::get", 0x0017ea44);
    return {};
}

// stub of ROR 0x0010fac0
float DogCamera::getLensFov()
{
    ROSEPAN_STUB("DogCamera::getLensFov", 0x0010fac0);
    return {};
}

// Written ahead for native 016 (leaves of unassigned@0010f944): the lens
// angle as a scale of m_144.
// ROR 0x0010fc08
void DogCamera::setLensScale(float scale)
{
    m_angle = scale * m_144 * 0x1.cd94dep-1f;  // 0x3f66ca6f, 0.901526
}

// ROR 0x0010fbd4
float DogCamera::getLensScale()
{
    return m_angle / m_144 / 0x1.cd94dep-1f;
}

// stub of ROR 0x00177830
void FUN_00177830(Obj176a3c *obj, i3dContext *ctx)
{
    ROSEPAN_STUB("FUN_00177830", 0x00177830);
}

// stub of ROR 0x00140e88
void MagScene::drawFocus(i3dContext *ctx, int frame)
{
    ROSEPAN_STUB("MagScene::drawFocus", 0x00140e88);
}

// stub of ROR 0x001406b8
void MagScene::drawSkyDome(i3dContext *ctx)
{
    ROSEPAN_STUB("MagScene::drawSkyDome", 0x001406b8);
}

// stub of ROR 0x0021e1d8
u_long   * i3dPktDrawLineStrip(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix, int count,
                              const void *color, const void *xyz, int flags)
{
    ROSEPAN_STUB("i3dPktDrawLineStrip", 0x0021e1d8);
    // PC: the packet cursor stays valid for the i3d groups (i3d/i3dlayer.cpp).
    return pkt;
}

// stub of ROR 0x0021db18
u_long   * i3dPktDrawStrip(u_long *pkt, sceVu0FVECTOR *matrix, int count, const void *stq,
                          const void *colors, const void *xyz, int flags)
{
    ROSEPAN_STUB("i3dPktDrawStrip", 0x0021db18);
    // PC: the packet cursor stays valid for the i3d groups (i3d/i3dlayer.cpp).
    return pkt;
}

// stub of ROR 0x0021dd10
u_long   * i3dPktDrawStripCtx(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix, int count,
                             const void *stq, const void *colors, const void *xyz,
                             int flags)
{
    ROSEPAN_STUB("i3dPktDrawStripCtx", 0x0021dd10);
    // PC: the packet cursor stays valid for the i3d groups (i3d/i3dlayer.cpp).
    return pkt;
}

// stub of ROR 0x0021a8e0
int i3rUpdateTexAnim(I3rRoot *root)
{
    ROSEPAN_STUB("i3rUpdateTexAnim", 0x0021a8e0);
    return {};
}

// stub of ROR 0x00106e70
float utlVecDist(const float *a, const float *b)
{
    ROSEPAN_STUB("utlVecDist", 0x00106e70);
    return {};
}

// bridge of ROR 0x00228c38
void i3dMatGetScaleTrans(sceVu0FVECTOR *out, sceVu0FVECTOR *m)
{
    ROSEPAN_BRIDGE("i3dMatGetScaleTrans", 0x00228c38);
    out[0][3] = 0.0f;
    out[0][0] = m[0][0];
    out[0][1] = m[1][1];
    out[0][2] = m[2][2];
    i3dVecCopyW(out[1], m[3]);
}

// stub of ROR 0x0021e8f8
u_long   * i3dPktDrawWireBox(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix,
                            const float *center, const void *extent, const void *color)
{
    ROSEPAN_STUB("i3dPktDrawWireBox", 0x0021e8f8);
    return {};
}

// stub of ROR 0x0021e9f8
u_long   * i3dPktDrawWireShape(u_long *pkt, I3dLayer *layer, void *a, void *b, void *c)
{
    ROSEPAN_STUB("i3dPktDrawWireShape", 0x0021e9f8);
    return {};
}

// stub of ROR 0x0021e6f8
u_long   * i3dPktDrawWireSphere(float radius, u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix,
                               const float *center)
{
    ROSEPAN_STUB("i3dPktDrawWireSphere", 0x0021e6f8);
    return {};
}

// ---- bridge: utlTm2GetSize (2026-10-22, the dogtexinfo session) ----
// The ROM's code, until utl/utltm2.cpp (unassigned@00104970) is written:
// DogTexInfo::addData sizes its TIM2 copy with it, and a stub's 0 would
// copy nothing.

// bridge of ROR 0x00104e1c
int utlTm2GetSize(const void *tm2)
{
    ROSEPAN_BRIDGE("utlTm2GetSize", 0x00104e1c);
    const u_char *p = (const u_char *)tm2;
    u_int n = *(const u_short *)(p + 6);
    int size = p[5] != 0 ? 0x80 : 0x10;
    for (; n != 0; n--)
        size += *(const int *)(p + size);
    return size;
}

// stub of ROR 0x001bf024
void GamFont::drawChar(int x, int y, int code)
{
    ROSEPAN_STUB("GamFont::drawChar", 0x001bf024);
}

// stub of ROR 0x001bef24
int GamFont::fillRect(int x, int y, int w, int h)
{
    ROSEPAN_STUB("GamFont::fillRect", 0x001bef24);
    return {};
}

// stub of ROR 0x001bffcc
int GamFont::getGlyph(int code)
{
    ROSEPAN_STUB("GamFont::getGlyph", 0x001bffcc);
    return {};
}

// stub of ROR 0x001c020c
int GamFont::getGlyphLeft(int glyph)
{
    ROSEPAN_STUB("GamFont::getGlyphLeft", 0x001c020c);
    return {};
}

// stub of ROR 0x001c0254
int GamFont::getGlyphWidth(int glyph)
{
    ROSEPAN_STUB("GamFont::getGlyphWidth", 0x001c0254);
    return {};
}

// stub of ROR 0x001bff64
void GamFont::setColor(u_int color)
{
    ROSEPAN_STUB("GamFont::setColor", 0x001bff64);
}

// stub of ROR 0x001bebec
void GamFont::setFont(const void *font)
{
    ROSEPAN_STUB("GamFont::setFont", 0x001bebec);
}

// stub of ROR 0x001bedc0
int GamFont::setTarget(void *buf, u_int w, u_int h, int bits)
{
    ROSEPAN_STUB("GamFont::setTarget", 0x001bedc0);
    return {};
}

// ──────────────────────────────────────────────────────────────────────
// Equality without case: a..z fold to A..Z (unassigned@00106010, written
// ahead for MagLoaderIndex::find).
// ROR 0x00106010

int utlStrEqualNoCase(const char *a, const char *b)
{
    char ca = *a;
    for (;;) {
        a++;
        char cb = *b++;
        if ((unsigned)(cb - 'a') < 26)
            cb -= 0x20;
        if ((unsigned)(ca - 'a') < 26)
            ca -= 0x20;
        if (ca != cb)
            return 0;
        if (ca == 0)
            return 1;
        ca = *a;
    }
}

// stub of ROR 0x00106658
u_int utlConvSndId(u_int id)
{
    ROSEPAN_STUB("utlConvSndId", 0x00106658);
    return {};
}

// stub of ROR 0x00110114
float * DogEntity::getMatrixZ(float *out)
{
    ROSEPAN_STUB("DogEntity::getMatrixZ", 0x00110114);
    return {};
}

// stub of ROR 0x0017772c
void FUN_0017772c(Obj176a3c *obj)
{
    ROSEPAN_STUB("FUN_0017772c", 0x0017772c);
}

// stub of ROR 0x0017c340
void FUN_0017c340(Obj17c0f0 *obj, const float *pos, MagSectorHit **hit, int arg,
                   float r, float h)
{
    ROSEPAN_STUB("FUN_0017c340", 0x0017c340);
}

// stub of ROR 0x0013f0c0
int MagScene::canOpenItemMenu()
{
    ROSEPAN_STUB("MagScene::canOpenItemMenu", 0x0013f0c0);
    return {};
}

// stub of ROR 0x0013f114
int MagScene::canOpenMenu10001()
{
    ROSEPAN_STUB("MagScene::canOpenMenu10001", 0x0013f114);
    return {};
}

// stub of ROR 0x0013f1e4
void MagScene::clearGamFlag25()
{
    ROSEPAN_STUB("MagScene::clearGamFlag25", 0x0013f1e4);
}

// stub of ROR 0x0013f458
void MagScene::closeSysMsgWindow(int win)
{
    ROSEPAN_STUB("MagScene::closeSysMsgWindow", 0x0013f458);
}

// stub of ROR 0x0013f07c
void MagScene::gamEnterMenu()
{
    ROSEPAN_STUB("MagScene::gamEnterMenu", 0x0013f07c);
}

// stub of ROR 0x0013f09c
void MagScene::gamLeaveMenu()
{
    ROSEPAN_STUB("MagScene::gamLeaveMenu", 0x0013f09c);
}

// stub of ROR 0x0013f194
int MagScene::getMenuFadeInTime()
{
    ROSEPAN_STUB("MagScene::getMenuFadeInTime", 0x0013f194);
    return {};
}

// stub of ROR 0x0013f144
int MagScene::getMenuFadeOutTime()
{
    ROSEPAN_STUB("MagScene::getMenuFadeOutTime", 0x0013f144);
    return {};
}

// stub of ROR 0x0014062c
void MagScene::rotateTenkyu()
{
    ROSEPAN_STUB("MagScene::rotateTenkyu", 0x0014062c);
}

// stub of ROR 0x0013f130
void MagScene::setGamWord54()
{
    ROSEPAN_STUB("MagScene::setGamWord54", 0x0013f130);
}

// stub of ROR 0x0013e548
void MagScene::updateCamera()
{
    ROSEPAN_STUB("MagScene::updateCamera", 0x0013e548);
}

// stub of ROR 0x00143fd0
void MagScene::updateFog()
{
    ROSEPAN_STUB("MagScene::updateFog", 0x00143fd0);
}

// stub of ROR 0x00140c68
void MagScene::updateRecords853fc()
{
    ROSEPAN_STUB("MagScene::updateRecords853fc", 0x00140c68);
}

// stub of ROR 0x00107248
void utlVecToRot(const float *dir, float *rot)
{
    ROSEPAN_STUB("utlVecToRot", 0x00107248);
}

// stub of ROR 0x001beeb4
int GamFont::clear()
{
    ROSEPAN_STUB("GamFont::clear", 0x001beeb4);
    return {};
}

// stub of ROR 0x001c047c
int GamFont::getTextWidth(const char *text)
{
    ROSEPAN_STUB("GamFont::getTextWidth", 0x001c047c);
    return {};
}

// stub of ROR 0x00140a84
void MagScene::initRecords853bc()
{
    ROSEPAN_STUB("MagScene::initRecords853bc", 0x00140a84);
}

// stub of ROR 0x00176c50
void FUN_00176c50(Obj176a3c *obj)
{
    ROSEPAN_STUB("FUN_00176c50", 0x00176c50);
}

// stub of ROR 0x00194548
void FUN_00194548(void)
{
    ROSEPAN_STUB("FUN_00194548", 0x00194548);
}

// stub of ROR 0x00234050
float i3dSegSegDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                     float *s, float *t)
{
    ROSEPAN_STUB("i3dSegSegDist2", 0x00234050);
    return {};
}

// stub of ROR 0x00106db0
void utlNormalize3(float *v)
{
    ROSEPAN_STUB("utlNormalize3", 0x00106db0);
}

// stub of ROR 0x001bfe30
void GamFont::scroll(int rows)
{
    ROSEPAN_STUB("GamFont::scroll", 0x001bfe30);
}

// stub of ROR 0x001bff28
void GamFont::setFill(u_int fill)
{
    ROSEPAN_STUB("GamFont::setFill", 0x001bff28);
}

// stub of ROR 0x001bff5c
void GamFont::setFillMsb(int on)
{
    ROSEPAN_STUB("GamFont::setFillMsb", 0x001bff5c);
}

// stub of ROR 0x002337e0
float i3dPointSegDist2(const float *p, const float *a, const float *d, float *t)
{
    ROSEPAN_STUB("i3dPointSegDist2", 0x002337e0);
    return {};
}

// stub of ROR 0x0021a3e8
int i3rBuildPackets(I3rRoot *root)
{
    ROSEPAN_STUB("i3rBuildPackets", 0x0021a3e8);
    return {};
}

// stub of ROR 0x0021a558
int i3rClearPackets(I3rRoot *root)
{
    ROSEPAN_STUB("i3rClearPackets", 0x0021a558);
    return {};
}

// stub of ROR 0x0022c478
int i3rItemRelocate(I3rItem *item, I3rItem *base)
{
    ROSEPAN_STUB("i3rItemRelocate", 0x0022c478);
    return {};
}

// stub of ROR 0x0022c328
int i3rItemUnrelocate(I3rItem *item, I3rItem *base)
{
    ROSEPAN_STUB("i3rItemUnrelocate", 0x0022c328);
    return {};
}

// stub of ROR 0x001deaac
void GamArea::checkPc()
{
    ROSEPAN_STUB("GamArea::checkPc", 0x001deaac);
}

// stub of ROR 0x001dea40
void GamArea::startPcAction()
{
    ROSEPAN_STUB("GamArea::startPcAction", 0x001dea40);
}

// ──────────────────────────────────────────────────────────────────────
// A text window slot is in use (unassigned@0013e2ec, written ahead for
// GamMan::isMsgBusy and native 004).
// ROR 0x0013f444

int MagScene::isWindowOpen(int win)
{
    return m_window[win] != NULL;
}

// ──────────────────────────────────────────────────────────────────────
// A step along angle on the ground plane (unassigned@001e6cac, written
// ahead for native 152).
// ROR 0x001f6b28

void utlAddPolar(float angle, float dist, float *v)
{
    v[0] = v[0] + sinf(angle) * dist;
    v[2] = v[2] + cosf(angle) * dist;
}

// stub of ROR 0x0013f280
int MagScene::openSysMsgWindow(int msg, const int *args, int arg)
{
    ROSEPAN_STUB("MagScene::openSysMsgWindow", 0x0013f280);
    return {};
}

// stub of ROR 0x001f83f4
int scenePlayStart(const char *name, intptr_t arg, int wait)
{
    ROSEPAN_STUB("scenePlayStart", 0x001f83f4);
    return {};
}

// stub of ROR 0x001f8cec
void scenePlayStartSimple(const char *name)
{
    ROSEPAN_STUB("scenePlayStartSimple", 0x001f8cec);
}

// stub of ROR 0x001e0c2c
int GamArea::ctl_trg_flg(int op, int id, u_int bits)
{
    ROSEPAN_STUB("GamArea::ctl_trg_flg", 0x001e0c2c);
    return {};
}


// stub of ROR 0x001dd414
void GamArea::init()
{
    ROSEPAN_STUB("GamArea::init", 0x001dd414);
}

// stub of ROR 0x001dee78
void GamArea::update(const float *pos, const float *rot)
{
    ROSEPAN_STUB("GamArea::update", 0x001dee78);
}

// stub of ROR 0x00130de0
void MagLife130da4::setVolume(int volume)
{
    ROSEPAN_STUB("MagLife130da4::setVolume", 0x00130de0);
}

// stub of ROR 0x0017e5c0
void WrdTable::load(int stage)
{
    ROSEPAN_STUB("WrdTable::load", 0x0017e5c0);
}

// stub of ROR 0x001f688c
void gamClearFrameReq(void)
{
    ROSEPAN_STUB("gamClearFrameReq", 0x001f688c);
}

// stub of ROR 0x001f8fb8
void gamSystemDataLoaded(void)
{
    ROSEPAN_STUB("gamSystemDataLoaded", 0x001f8fb8);
}

// stub of ROR 0x00163ab8
void EmyManager::getOids(int *a, int *b)
{
    ROSEPAN_STUB("EmyManager::getOids", 0x00163ab8);
}

// stub of ROR 0x001dddbc
void GamArea::clearKind(int kind)
{
    ROSEPAN_STUB("GamArea::clearKind", 0x001dddbc);
}

// stub of ROR 0x001dd7bc
void GamArea::stopKind(int kind)
{
    ROSEPAN_STUB("GamArea::stopKind", 0x001dd7bc);
}

// stub of ROR 0x001d544c
void GamMan::placeAchr(int oid, void *out, int arg)
{
    ROSEPAN_STUB("GamMan::placeAchr", 0x001d544c);
}

// stub of ROR 0x001d5da0
void GamMan::relinkObj(u_int oid)
{
    ROSEPAN_STUB("GamMan::relinkObj", 0x001d5da0);
}

// stub of ROR 0x001f5e48
void * chrChangeMotion(u_int oid, int motion, int a2, int a3, float blend,
                      float rotY)
{
    ROSEPAN_STUB("chrChangeMotion", 0x001f5e48);
    return {};
}

// stub of ROR 0x00144334
void gamSetBwnFollow(int on)
{
    ROSEPAN_STUB("gamSetBwnFollow", 0x00144334);
}

// stub of ROR 0x00144500
void gamSetBwnItem(int item)
{
    ROSEPAN_STUB("gamSetBwnItem", 0x00144500);
}

// stub of ROR 0x0014457c
void gamSetBwnWait(int on)
{
    ROSEPAN_STUB("gamSetBwnWait", 0x0014457c);
}

// stub of ROR 0x001fbbd0
void gamStage2Loaded(void)
{
    ROSEPAN_STUB("gamStage2Loaded", 0x001fbbd0);
}

// stub of ROR 0x001634d4
void EmyManager::releaseObj(u_int oid)
{
    ROSEPAN_STUB("EmyManager::releaseObj", 0x001634d4);
}

// stub of ROR 0x001e0d70
int GamArea::hasId(u_int id)
{
    ROSEPAN_STUB("GamArea::hasId", 0x001e0d70);
    return {};
}

// stub of ROR 0x001d83d0
int GamMan::lookupFlagTable(int keyCol, int key, int col)
{
    ROSEPAN_STUB("GamMan::lookupFlagTable", 0x001d83d0);
    return {};
}

// stub of ROR 0x0017756c
void Obj176a3c::release(int no)
{
    ROSEPAN_STUB("Obj176a3c::release", 0x0017756c);
}

// stub of ROR 0x001f6578
u_int chrCreateItemObj(short id, const float *pos, const float *rot)
{
    ROSEPAN_STUB("chrCreateItemObj", 0x001f6578);
    return {};
}

// ---- stubs (hand-written: the mnu/ menu objects' virtual slots, which stubgen cannot see behind the vtable) ----

// stub of ROR 0x00192d60
void mnuBase::term()
{
    ROSEPAN_STUB("mnuBase::term", 0x00192d60);
}

// stub of ROR 0x00194908
void MnuMap::init(DogTexInfo *tex)
{
    ROSEPAN_STUB("MnuMap::init", 0x00194908);
}

// stub of ROR 0x00194960
void MnuMap::term()
{
    ROSEPAN_STUB("MnuMap::term", 0x00194960);
}

// stub of ROR 0x00194fe4
int MnuMap::processPad(BedPad *pad)
{
    ROSEPAN_STUB("MnuMap::processPad", 0x00194fe4);
    return {};
}

// stub of ROR 0x00195e90
void MnuMap::draw(I3dLayer *layer)
{
    ROSEPAN_STUB("MnuMap::draw", 0x00195e90);
}

// stub of ROR 0x00196bdc
void MnuPBook::init(DogTexInfo *tex)
{
    ROSEPAN_STUB("MnuPBook::init", 0x00196bdc);
}

// stub of ROR 0x00196d98
void MnuPBook::term()
{
    ROSEPAN_STUB("MnuPBook::term", 0x00196d98);
}

// stub of ROR 0x001976b8
int MnuPBook::processPad(BedPad *pad)
{
    ROSEPAN_STUB("MnuPBook::processPad", 0x001976b8);
    return {};
}

// stub of ROR 0x00197f70
void MnuPBook::draw(I3dLayer *layer)
{
    ROSEPAN_STUB("MnuPBook::draw", 0x00197f70);
}

// stub of ROR 0x00198860
void MnuDiary::init(DogTexInfo *tex)
{
    ROSEPAN_STUB("MnuDiary::init", 0x00198860);
}

// stub of ROR 0x00198a38
void MnuDiary::term()
{
    ROSEPAN_STUB("MnuDiary::term", 0x00198a38);
}

// stub of ROR 0x001990b0
int MnuDiary::processPad(BedPad *pad)
{
    ROSEPAN_STUB("MnuDiary::processPad", 0x001990b0);
    return {};
}

// stub of ROR 0x00199c94
void MnuDiary::draw(I3dLayer *layer)
{
    ROSEPAN_STUB("MnuDiary::draw", 0x00199c94);
}

// stub of ROR 0x0013ca10
void MagScene::restoreCamera()
{
    ROSEPAN_STUB("MagScene::restoreCamera", 0x0013ca10);
}

// stub of ROR 0x0013c930
void MagScene::saveCamera()
{
    ROSEPAN_STUB("MagScene::saveCamera", 0x0013c930);
}

// stub of ROR 0x001ad8e0
void MnuBoard2D::draw(I3dLayer *layer)
{
    ROSEPAN_STUB("MnuBoard2D::draw", 0x001ad8e0);
}

// stub of ROR 0x001ad824
void MnuBoard2D::init(DogTexInfo *tex)
{
    ROSEPAN_STUB("MnuBoard2D::init", 0x001ad824);
}

// stub of ROR 0x001ad8bc
int MnuBoard2D::isInit()
{
    ROSEPAN_STUB("MnuBoard2D::isInit", 0x001ad8bc);
    return {};
}

// stub of ROR 0x001ad8cc
int MnuBoard2D::processPad(BedPad *pad)
{
    ROSEPAN_STUB("MnuBoard2D::processPad", 0x001ad8cc);
    return {};
}

// stub of ROR 0x001ad870
void MnuBoard2D::term()
{
    ROSEPAN_STUB("MnuBoard2D::term", 0x001ad870);
}

// stub of ROR 0x00198b04
int MnuDiary::setType(int type)
{
    ROSEPAN_STUB("MnuDiary::setType", 0x00198b04);
    return {};
}

// stub of ROR 0x001aed08
void MnuItem::draw(I3dLayer *layer)
{
    ROSEPAN_STUB("MnuItem::draw", 0x001aed08);
}

// stub of ROR 0x001aeacc
void MnuItem::init(DogTexInfo *tex)
{
    ROSEPAN_STUB("MnuItem::init", 0x001aeacc);
}

// stub of ROR 0x001aec48
int MnuItem::isInit()
{
    ROSEPAN_STUB("MnuItem::isInit", 0x001aec48);
    return {};
}

// stub of ROR 0x001b099c
int MnuItem::processPad(BedPad *pad)
{
    ROSEPAN_STUB("MnuItem::processPad", 0x001b099c);
    return {};
}

// stub of ROR 0x001aebc8
void MnuItem::term()
{
    ROSEPAN_STUB("MnuItem::term", 0x001aebc8);
}

// stub of ROR 0x001ae040
void MnuMovie::draw(I3dLayer *layer)
{
    ROSEPAN_STUB("MnuMovie::draw", 0x001ae040);
}

// stub of ROR 0x001adfb0
void MnuMovie::init(DogTexInfo *tex)
{
    ROSEPAN_STUB("MnuMovie::init", 0x001adfb0);
}

// stub of ROR 0x001ae01c
int MnuMovie::isInit()
{
    ROSEPAN_STUB("MnuMovie::isInit", 0x001ae01c);
    return {};
}

// stub of ROR 0x001ae02c
int MnuMovie::processPad(BedPad *pad)
{
    ROSEPAN_STUB("MnuMovie::processPad", 0x001ae02c);
    return {};
}

// stub of ROR 0x001adfe4
void MnuMovie::term()
{
    ROSEPAN_STUB("MnuMovie::term", 0x001adfe4);
}

// stub of ROR 0x001ae614
void MnuNow::draw(I3dLayer *layer)
{
    ROSEPAN_STUB("MnuNow::draw", 0x001ae614);
}

// stub of ROR 0x001ae520
void MnuNow::init(DogTexInfo *tex)
{
    ROSEPAN_STUB("MnuNow::init", 0x001ae520);
}

// stub of ROR 0x001ae5b4
int MnuNow::isInit()
{
    ROSEPAN_STUB("MnuNow::isInit", 0x001ae5b4);
    return {};
}

// stub of ROR 0x001ae600
int MnuNow::processPad(BedPad *pad)
{
    ROSEPAN_STUB("MnuNow::processPad", 0x001ae600);
    return {};
}

// stub of ROR 0x001ae574
void MnuNow::term()
{
    ROSEPAN_STUB("MnuNow::term", 0x001ae574);
}

// stub of ROR 0x001ab680
void MnuOpt::draw(I3dLayer *layer)
{
    ROSEPAN_STUB("MnuOpt::draw", 0x001ab680);
}

// stub of ROR 0x001ab598
void MnuOpt::init(DogTexInfo *tex)
{
    ROSEPAN_STUB("MnuOpt::init", 0x001ab598);
}

// stub of ROR 0x001ab65c
int MnuOpt::isInit()
{
    ROSEPAN_STUB("MnuOpt::isInit", 0x001ab65c);
    return {};
}

// stub of ROR 0x001ab66c
int MnuOpt::processPad(BedPad *pad)
{
    ROSEPAN_STUB("MnuOpt::processPad", 0x001ab66c);
    return {};
}

// stub of ROR 0x001ab5e4
void MnuOpt::term()
{
    ROSEPAN_STUB("MnuOpt::term", 0x001ab5e4);
}

// stub of ROR 0x00196e54
int MnuPBook::setType(int type)
{
    ROSEPAN_STUB("MnuPBook::setType", 0x00196e54);
    return {};
}

// stub of ROR 0x001bdfe0
void MnuSelectLanguage::draw(I3dLayer *layer)
{
    ROSEPAN_STUB("MnuSelectLanguage::draw", 0x001bdfe0);
}

// stub of ROR 0x001bdc00
void MnuSelectLanguage::init(DogTexInfo *tex)
{
    ROSEPAN_STUB("MnuSelectLanguage::init", 0x001bdc00);
}

// stub of ROR 0x001bdc94
int MnuSelectLanguage::isInit()
{
    ROSEPAN_STUB("MnuSelectLanguage::isInit", 0x001bdc94);
    return {};
}

// stub of ROR 0x001bde58
int MnuSelectLanguage::processPad(BedPad *pad)
{
    ROSEPAN_STUB("MnuSelectLanguage::processPad", 0x001bde58);
    return {};
}

// stub of ROR 0x001bdc54
void MnuSelectLanguage::term()
{
    ROSEPAN_STUB("MnuSelectLanguage::term", 0x001bdc54);
}

// stub of ROR 0x0019a754
void MnuStock::draw(I3dLayer *layer)
{
    ROSEPAN_STUB("MnuStock::draw", 0x0019a754);
}

// stub of ROR 0x0019a650
void MnuStock::init(DogTexInfo *tex)
{
    ROSEPAN_STUB("MnuStock::init", 0x0019a650);
}

// stub of ROR 0x0019a730
int MnuStock::isInit()
{
    ROSEPAN_STUB("MnuStock::isInit", 0x0019a730);
    return {};
}

// stub of ROR 0x0019a740
int MnuStock::processPad(BedPad *pad)
{
    ROSEPAN_STUB("MnuStock::processPad", 0x0019a740);
    return {};
}

// stub of ROR 0x0019a6b8
void MnuStock::term()
{
    ROSEPAN_STUB("MnuStock::term", 0x0019a6b8);
}


// stub of ROR 0x001a074c
int MnuTextList::close()
{
    ROSEPAN_STUB("MnuTextList::close", 0x001a074c);
    return {};
}

// stub of ROR 0x001a0bb8
void MnuTextList::endSelect()
{
    ROSEPAN_STUB("MnuTextList::endSelect", 0x001a0bb8);
}

// stub of ROR 0x001a0ba0
int MnuTextList::getSelect()
{
    ROSEPAN_STUB("MnuTextList::getSelect", 0x001a0ba0);
    return {};
}

// stub of ROR 0x001a06e8
int MnuTextList::open()
{
    ROSEPAN_STUB("MnuTextList::open", 0x001a06e8);
    return {};
}

// stub of ROR 0x001a06d4
void MnuTextList::processPad(BedPad *pad)
{
    ROSEPAN_STUB("MnuTextList::processPad", 0x001a06d4);
}

// stub of ROR 0x001a09b8
void MnuTextList::setItems(const char *items, u_int mask)
{
    ROSEPAN_STUB("MnuTextList::setItems", 0x001a09b8);
}

// stub of ROR 0x00187464
void MsgManSet::close()
{
    ROSEPAN_STUB("MsgManSet::close", 0x00187464);
}

// stub of ROR 0x001875c0
int MsgManSet::getState(int window)
{
    ROSEPAN_STUB("MsgManSet::getState", 0x001875c0);
    return {};
}

// stub of ROR 0x0018741c
void MsgManSet::open()
{
    ROSEPAN_STUB("MsgManSet::open", 0x0018741c);
}

// stub of ROR 0x001874f4
void MsgManSet::setMessage(int window, const char *text, int flag)
{
    ROSEPAN_STUB("MsgManSet::setMessage", 0x001874f4);
}

// stub of ROR 0x001874ac
void MsgManSet::update()
{
    ROSEPAN_STUB("MsgManSet::update", 0x001874ac);
}

// stub of ROR 0x001832e8
const char * msgGetSysText(int no)
{
    ROSEPAN_STUB("msgGetSysText", 0x001832e8);
    return {};
}

// stub of ROR 0x001d68a0
void GamMan::readSaveData(void *buf)
{
    ROSEPAN_STUB("GamMan::readSaveData", 0x001d68a0);
}

// stub of ROR 0x001d6078
void GamMan::writeSaveData(void *buf)
{
    ROSEPAN_STUB("GamMan::writeSaveData", 0x001d6078);
}

// stub of ROR 0x001d480c
void GamMan::fitObjBounds(u_int oid)
{
    ROSEPAN_STUB("GamMan::fitObjBounds", 0x001d480c);
}

// stub of ROR 0x001d4a3c
float GamMan::getObjRadius(u_int oid, int *out)
{
    ROSEPAN_STUB("GamMan::getObjRadius", 0x001d4a3c);
    return {};
}

// stub of ROR 0x001d5bc8
void GamMan::queueObjUpdate(GamSecta *cell, u_int oid)
{
    ROSEPAN_STUB("GamMan::queueObjUpdate", 0x001d5bc8);
}

// stub of ROR 0x00144ae0
intptr_t magBwnSeqCommand(const intptr_t *args)
{
    ROSEPAN_STUB("magBwnSeqCommand", 0x00144ae0);
    return {};
}

// stub of ROR 0x001f6b8c
int chrRotFrames(float angle, float speed)
{
    ROSEPAN_STUB("chrRotFrames", 0x001f6b8c);
    return {};
}

// stub of ROR 0x001f0e90
void chrRotStart(float angle, u_int oid, int frames)
{
    ROSEPAN_STUB("chrRotStart", 0x001f0e90);
}

// stub of ROR 0x001f0f9c
void chrRotTo(float dir, float speed, u_int oid)
{
    ROSEPAN_STUB("chrRotTo", 0x001f0f9c);
}

// stub of ROR 0x001f10a4
void chrRotToIn(float dir, u_int oid, int frames)
{
    ROSEPAN_STUB("chrRotToIn", 0x001f10a4);
}

// stub of ROR 0x0017c4f4
void sectorClearFlags(Obj17c0f0 *obj, int key, u_int bits, int rec)
{
    ROSEPAN_STUB("sectorClearFlags", 0x0017c4f4);
}

// stub of ROR 0x0017c45c
void sectorSetFlags(Obj17c0f0 *obj, int key, u_int bits, int rec)
{
    ROSEPAN_STUB("sectorSetFlags", 0x0017c45c);
}

// stub of ROR 0x001dd5e8
void GamArea::addTrg(GamAreaTrg *trg, int newId)
{
    ROSEPAN_STUB("GamArea::addTrg", 0x001dd5e8);
}

// stub of ROR 0x001de6fc
void GamArea::registFtb(int key, u_int value)
{
    ROSEPAN_STUB("GamArea::registFtb", 0x001de6fc);
}

// stub of ROR 0x001de4e4
void GamArea::startTrgs()
{
    ROSEPAN_STUB("GamArea::startTrgs", 0x001de4e4);
}

// stub of ROR 0x001ddc80
void GamArea::stopTrgs()
{
    ROSEPAN_STUB("GamArea::stopTrgs", 0x001ddc80);
}

// stub of ROR 0x0016aa04
void Eff0::setVisible(bool on)
{
    ROSEPAN_STUB("Eff0::setVisible", 0x0016aa04);
}

// stub of ROR 0x00176cc8
int Obj176a3c::createEff(const EffParam *param)
{
    ROSEPAN_STUB("Obj176a3c::createEff", 0x00176cc8);
    return {};
}

// stub of ROR 0x00163b20
int EmyManager::getEmyWord3c(u_int oid)
{
    ROSEPAN_STUB("EmyManager::getEmyWord3c", 0x00163b20);
    return {};
}

// stub of ROR 0x00164078
int EmyManager::isEmyFlag30(u_int oid)
{
    ROSEPAN_STUB("EmyManager::isEmyFlag30", 0x00164078);
    return {};
}

// stub of ROR 0x00163bb8
void EmyManager::releaseEmyHold(u_int oid)
{
    ROSEPAN_STUB("EmyManager::releaseEmyHold", 0x00163bb8);
}

// stub of ROR 0x0016366c
void EmyManager::setEmyActive(u_int oid, int on)
{
    ROSEPAN_STUB("EmyManager::setEmyActive", 0x0016366c);
}

// stub of ROR 0x00163b48
int EmyManager::setEmyWord238(u_int oid, int v)
{
    ROSEPAN_STUB("EmyManager::setEmyWord238", 0x00163b48);
    return {};
}

// stub of ROR 0x00162afc
void EmyManager::setSlotWord24(int no, int v)
{
    ROSEPAN_STUB("EmyManager::setSlotWord24", 0x00162afc);
}

// stub of ROR 0x00163b80
int EmyManager::testEmyView(u_int oid)
{
    ROSEPAN_STUB("EmyManager::testEmyView", 0x00163b80);
    return {};
}

// stub of ROR 0x001f5d90
int chrChangeMotionBySuffix(float blend, float rotY, u_int oid, const char *name, int a2,
                             int a3)
{
    ROSEPAN_STUB("chrChangeMotionBySuffix", 0x001f5d90);
    return {};
}

// stub of ROR 0x001f61ec
void chrKillMotionTask(u_int oid)
{
    ROSEPAN_STUB("chrKillMotionTask", 0x001f61ec);
}

// stub of ROR 0x001f59b4
CmnITask * chrMtnMove(float speed, float rotY, u_int oid, int motion, int p1d8, u_int flags,
                     const char *name)
{
    ROSEPAN_STUB("chrMtnMove", 0x001f59b4);
    return {};
}

// stub of ROR 0x001f5bdc
CmnITask * chrMtnMove2(float speed, u_int oid, int motion, int p1d8, u_int flags,
                      const char *name)
{
    ROSEPAN_STUB("chrMtnMove2", 0x001f5bdc);
    return {};
}

// stub of ROR 0x001f6308
void chrSetMotion(float speed, int no, int motion, int p1d8, u_int flags, u_int oid,
                  const char *name)
{
    ROSEPAN_STUB("chrSetMotion", 0x001f6308);
}

// stub of ROR 0x00162c2c
u_int EmyManager::addEmy(int chrNo, short kind, void *chr, intptr_t seqWord, void *image,
                const void *pos, u_int oid, int group)
{
    ROSEPAN_STUB("EmyManager::addEmy", 0x00162c2c);
    return {};
}

// stub of ROR 0x00217f10
float utlAtan2(float x, float z)
{
    ROSEPAN_STUB("utlAtan2", 0x00217f10);
    return {};
}

// stub of ROR 0x001d8064
GamSecta * GamMan::findSectaCell(const float *pos, GamSecta *cur, int type2)
{
    ROSEPAN_STUB("GamMan::findSectaCell", 0x001d8064);
    return {};
}

// stub of ROR 0x0017eb3c
void WrdTable::cnvPosL2W(float *out, int no, const float *in)
{
    ROSEPAN_STUB("WrdTable::cnvPosL2W", 0x0017eb3c);
}

// stub of ROR 0x0017ebac
void WrdTable::cnvPosW2L(float *out, const float *in, int no)
{
    ROSEPAN_STUB("WrdTable::cnvPosW2L", 0x0017ebac);
}

// stub of ROR 0x0017762c
void Obj176a3c::releaseAll()
{
    ROSEPAN_STUB("Obj176a3c::releaseAll", 0x0017762c);
}

// stub of ROR 0x00172678
void gamCreateEff1a(void)
{
    ROSEPAN_STUB("gamCreateEff1a", 0x00172678);
}

// stub of ROR 0x00144398
void gamSetBwnPos(const float *pos)
{
    ROSEPAN_STUB("gamSetBwnPos", 0x00144398);
}

// stub of ROR 0x001443d8
void gamSetBwnRot(const float *rot)
{
    ROSEPAN_STUB("gamSetBwnRot", 0x001443d8);
}

// stub of ROR 0x001635e0
void EmyManager::resetWork()
{
    ROSEPAN_STUB("EmyManager::resetWork", 0x001635e0);
}

// stub of ROR 0x00162b68
void EmyManager::setGroup(float f, int a, int b, int c, int count, int d,
                  const void *pos)
{
    ROSEPAN_STUB("EmyManager::setGroup", 0x00162b68);
}

// stub of ROR 0x001ddaa8
void GamArea::clear()
{
    ROSEPAN_STUB("GamArea::clear", 0x001ddaa8);
}

// stub of ROR 0x0013d4d0
float MagScene::getCameraPathPos()
{
    ROSEPAN_STUB("MagScene::getCameraPathPos", 0x0013d4d0);
    return {};
}

// stub of ROR 0x0013d52c
void MagScene::setCameraPathPos(float pos)
{
    ROSEPAN_STUB("MagScene::setCameraPathPos", 0x0013d52c);
}

// stub of ROR 0x001d811c
GamSecta * GamMan::getSectaCell(int no, float *pos, int list)
{
    ROSEPAN_STUB("GamMan::getSectaCell", 0x001d811c);
    return {};
}

// stub of ROR 0x001d3b44
void GamMan::checkMenuReq()
{
    ROSEPAN_STUB("GamMan::checkMenuReq", 0x001d3b44);
}

// stub of ROR 0x001d817c
float GamMan::getSectaCellY(GamSecta *cell, const float *pos, int list)
{
    ROSEPAN_STUB("GamMan::getSectaCellY", 0x001d817c);
    return {};
}

// stub of ROR 0x001d7f48
GamSecta * GamMan::pickSectaCell(const float *pos, int list)
{
    ROSEPAN_STUB("GamMan::pickSectaCell", 0x001d7f48);
    return {};
}

// stub of ROR 0x001d7eb8
void GamMan::selectSectaList(int list)
{
    ROSEPAN_STUB("GamMan::selectSectaList", 0x001d7eb8);
}

// stub of ROR 0x001e8f88
int chrMotionRefEnd(const ChrMotionRef *ref)
{
    ROSEPAN_STUB("chrMotionRefEnd", 0x001e8f88);
    return {};
}

// stub of ROR 0x00144418
void gamGetBwnPos(float *out)
{
    ROSEPAN_STUB("gamGetBwnPos", 0x00144418);
}

// stub of ROR 0x0014448c
void gamGetBwnRot(float *out)
{
    ROSEPAN_STUB("gamGetBwnRot", 0x0014448c);
}

// stub of ROR 0x001f9658
int TskSnd::getVolume(int *volL, int *volR)
{
    ROSEPAN_STUB("TskSnd::getVolume", 0x001f9658);
    return {};
}
