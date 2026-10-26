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
// PC: the ROM defines the global objects in the file before main.cpp, whose
// static initializer 0x2041f8 constructs them (unassigned@001fd424). They
// live here until that file is defined; move them with it.

MagSystem  g_magSystem;         /* bss 0x6e9840 */
GamMan     g_gamMan;            /* bss 0x7812c0 */
CmnTaskMan g_taskMan;           /* bss 0x7a3d80 */
Sequence   g_sequence;          /* bss 0x7a4440 */
EmyManager g_emyManager;        /* bss 0x7b2140 */
SndMan     g_sndMan;            /* bss 0x7b2580 */
WrdTable   g_wrdTable;          /* data 0x298ee0: data/wrd/WRD.BIN's table */

int g_emyManagerTaskId;         /* bss 0x7b45c0 */
int g_bss_7a3d48;               /* bss 0x7a3d48 */
int g_bss_7a3d4c;               /* bss 0x7a3d4c */
int g_bss_7a3210;               /* bss 0x7a3210 */
int g_bss_7812ac;               /* bss 0x7812ac */
int g_bss_780d04;               /* bss 0x780d04: GamMan option 0 (brightness) */

const char *g_versionNumber = "1.01";   /* data 0x2d9e18 -> 0x301720 */

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x001e71fc
int CmnTaskMan::addTask(int group, int start, int prio, int frames, CmnITask *task, int arg,
                 int *id, int user)
{
    ROSEPAN_STUB("CmnTaskMan::addTask", 0x001e71fc);
    return {};
}

// stub of ROR 0x001e7a20
void CmnTaskMan::exec()
{
    ROSEPAN_STUB("CmnTaskMan::exec", 0x001e7a20);
}

// stub of ROR 0x00203ea8
void SysTimer::start()
{
    ROSEPAN_STUB("SysTimer::start", 0x00203ea8);
}

// stub of ROR 0x00204014
void SysTimer::update()
{
    ROSEPAN_STUB("SysTimer::update", 0x00204014);
}

// stub of ROR 0x002155a0
void debugNop(void)
{
    ROSEPAN_STUB("debugNop", 0x002155a0);
}

// ---- bridges (hand-written) ----

// Written ahead of its file (unassigned@001fd424) by the sequence.cpp
// session: Sequence::exec needs the image pools init builds.
// ROR 0x00204148
void sysInitManagers(void)
{
    g_sequence.init();
    g_gamMan.init();
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

// stub of ROR 0x001e7d7c
void * CmnTaskMan::getTask(int id)
{
    ROSEPAN_STUB("CmnTaskMan::getTask", 0x001e7d7c);
    return {};
}

// stub of ROR 0x001e4908
int GamSndMan::findSnd(int id, GamSndEntry **entry, int arg)
{
    ROSEPAN_STUB("GamSndMan::findSnd", 0x001e4908);
    return {};
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

// stub of ROR 0x001e7724
void CmnTaskMan::killTask(int id)
{
    ROSEPAN_STUB("CmnTaskMan::killTask", 0x001e7724);
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

// stub of ROR 0x001dbef8
void GamSysData::set(const u_char *data)
{
    ROSEPAN_STUB("GamSysData::set", 0x001dbef8);
}

// stub of ROR 0x00130de0
void MagLife130da4::setVolume(int volume)
{
    ROSEPAN_STUB("MagLife130da4::setVolume", 0x00130de0);
}

// stub of ROR 0x001d8964
void ObjStat::getRot(float *out)
{
    ROSEPAN_STUB("ObjStat::getRot", 0x001d8964);
}

// stub of ROR 0x001d96ec
int ObjStat::save(void *out)
{
    ROSEPAN_STUB("ObjStat::save", 0x001d96ec);
    return {};
}

// stub of ROR 0x001d8f04
void ObjStat::updatePos(float *out)
{
    ROSEPAN_STUB("ObjStat::updatePos", 0x001d8f04);
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

// stub of ROR 0x001da130
void ACHRstat::pauseTasks(int on, int all)
{
    ROSEPAN_STUB("ACHRstat::pauseTasks", 0x001da130);
}

// stub of ROR 0x001e747c
void CmnTaskMan::pauseKinds(const int *kinds, int on)
{
    ROSEPAN_STUB("CmnTaskMan::pauseKinds", 0x001e747c);
}

// stub of ROR 0x001e7544
void CmnTaskMan::pauseOthers(int on)
{
    ROSEPAN_STUB("CmnTaskMan::pauseOthers", 0x001e7544);
}

// stub of ROR 0x001e7414
void CmnTaskMan::pauseTask(int id, int on)
{
    ROSEPAN_STUB("CmnTaskMan::pauseTask", 0x001e7414);
}

// stub of ROR 0x00163ab8
void EmyManager::getOids(int *a, int *b)
{
    ROSEPAN_STUB("EmyManager::getOids", 0x00163ab8);
}

// stub of ROR 0x001db774
void FCHRstat::getRot(float *out)
{
    ROSEPAN_STUB("FCHRstat::getRot", 0x001db774);
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

// stub of ROR 0x001d9f10
void ObjStat::clearSecta(int all)
{
    ROSEPAN_STUB("ObjStat::clearSecta", 0x001d9f10);
}

// stub of ROR 0x001d8ea8
void ObjStat::findSecta()
{
    ROSEPAN_STUB("ObjStat::findSecta", 0x001d8ea8);
}

// stub of ROR 0x001d9830
int ObjStat::load(const void *in)
{
    ROSEPAN_STUB("ObjStat::load", 0x001d9830);
    return {};
}

// stub of ROR 0x001d95c4
void ObjStat::setHide(int hide)
{
    ROSEPAN_STUB("ObjStat::setHide", 0x001d95c4);
}

// stub of ROR 0x001d8dc4
void ObjStat::syncActorPos()
{
    ROSEPAN_STUB("ObjStat::syncActorPos", 0x001d8dc4);
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

// stub of ROR 0x001da228
void ACHRstat::getParam(int which, void *out)
{
    ROSEPAN_STUB("ACHRstat::getParam", 0x001da228);
}

// stub of ROR 0x001da740
void ACHRstat::setParam(int which, const void *data)
{
    ROSEPAN_STUB("ACHRstat::setParam", 0x001da740);
}

// stub of ROR 0x001634d4
void EmyManager::releaseObj(u_int oid)
{
    ROSEPAN_STUB("EmyManager::releaseObj", 0x001634d4);
}

// stub of ROR 0x001db31c
void FCHRstat::getParam(int which, void *out)
{
    ROSEPAN_STUB("FCHRstat::getParam", 0x001db31c);
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

// stub of ROR 0x001dbfa0
const void * GamSysData::getRec(short id)
{
    ROSEPAN_STUB("GamSysData::getRec", 0x001dbfa0);
    return {};
}

// stub of ROR 0x001dcd38
void GamSysData::setFlags(short id, u_int bits)
{
    ROSEPAN_STUB("GamSysData::setFlags", 0x001dcd38);
}

// stub of ROR 0x001dd00c
int GamSysData::testFlags(short id, u_int bits)
{
    ROSEPAN_STUB("GamSysData::testFlags", 0x001dd00c);
    return {};
}

// stub of ROR 0x0017756c
void Obj176a3c::release(int no)
{
    ROSEPAN_STUB("Obj176a3c::release", 0x0017756c);
}

// stub of ROR 0x001d966c
void ObjStat::releaseActor()
{
    ROSEPAN_STUB("ObjStat::releaseActor", 0x001d966c);
}

// stub of ROR 0x001f6578
u_int chrCreateItemObj(short id, const float *pos, const float *rot)
{
    ROSEPAN_STUB("chrCreateItemObj", 0x001f6578);
    return {};
}

// stub of ROR 0x001dc48c
void GamSysData::getField(int id, int field, void *out)
{
    ROSEPAN_STUB("GamSysData::getField", 0x001dc48c);
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
void GamMan::queueObjUpdate(int kind, u_int oid)
{
    ROSEPAN_STUB("GamMan::queueObjUpdate", 0x001d5bc8);
}

// stub of ROR 0x001dce30
void GamSysData::clearFlags(short id, u_int bits)
{
    ROSEPAN_STUB("GamSysData::clearFlags", 0x001dce30);
}

// stub of ROR 0x001dd154
void GamSysData::clearFlags2(short id, int unused, u_int bits)
{
    ROSEPAN_STUB("GamSysData::clearFlags2", 0x001dd154);
}

// stub of ROR 0x001dc070
void GamSysData::setField(int id, int field, const void *data)
{
    ROSEPAN_STUB("GamSysData::setField", 0x001dc070);
}

// stub of ROR 0x001dd118
void GamSysData::setFlags2(short id, int unused, u_int bits)
{
    ROSEPAN_STUB("GamSysData::setFlags2", 0x001dd118);
}

// stub of ROR 0x00144ae0
intptr_t magBwnSeqCommand(const intptr_t *args)
{
    ROSEPAN_STUB("magBwnSeqCommand", 0x00144ae0);
    return {};
}
