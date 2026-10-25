// lib/unassigned_stubs.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.
#include <math.h>
#include <string.h>
#include "game/gamsnd.h"
#include "rx/rxsnd.h"

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
RxMpeg     g_rxMpeg;            /* bss 0x7b40c0 */
WrdTable   g_wrdTable;          /* data 0x298ee0: data/wrd/WRD.BIN's table */

int g_emyManagerTaskId;         /* bss 0x7b45c0 */
int g_bss_7a3d48;               /* bss 0x7a3d48 */
int g_bss_7a3d4c;               /* bss 0x7a3d4c */
int g_bss_7812ac;               /* bss 0x7812ac */

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

// stub of ROR 0x00204148
void sysInitManagers(void)
{
    ROSEPAN_STUB("sysInitManagers", 0x00204148);
}

// ---- bridges (hand-written) ----

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

// stub of ROR 0x001f4184 (a constructor: stubgen leaves it to hand)
TskSysFunc::TskSysFunc(int kind, int a, intptr_t arg)
{
    ROSEPAN_STUB("TskSysFunc::TskSysFunc", 0x001f4184);
}

// stub of ROR 0x00181204
void RxMpeg::setFrameParity(int parity)
{
    ROSEPAN_STUB("RxMpeg::setFrameParity", 0x00181204);
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

// stub of ROR 0x001816b0
int RxMpeg::close()
{
    ROSEPAN_STUB("RxMpeg::close", 0x001816b0);
    return {};
}

// stub of ROR 0x001812a8
void RxMpeg::draw(I3dLayer *layer)
{
    ROSEPAN_STUB("RxMpeg::draw", 0x001812a8);
}

// stub of ROR 0x0018173c
int RxMpeg::getStatus()
{
    ROSEPAN_STUB("RxMpeg::getStatus", 0x0018173c);
    return {};
}

// stub of ROR 0x0018176c
int RxMpeg::isEnd()
{
    ROSEPAN_STUB("RxMpeg::isEnd", 0x0018176c);
    return {};
}

// stub of ROR 0x00181150
void RxMpeg::open()
{
    ROSEPAN_STUB("RxMpeg::open", 0x00181150);
}

// stub of ROR 0x0018154c
void RxMpeg::play(const char *name, int w, int h)
{
    ROSEPAN_STUB("RxMpeg::play", 0x0018154c);
}

// stub of ROR 0x00181644
int RxMpeg::stop()
{
    ROSEPAN_STUB("RxMpeg::stop", 0x00181644);
    return {};
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

// stub of ROR 0x00181494
void RxMpeg::postUpdate()
{
    ROSEPAN_STUB("RxMpeg::postUpdate", 0x00181494);
}

// stub of ROR 0x0018120c
void RxMpeg::update()
{
    ROSEPAN_STUB("RxMpeg::update", 0x0018120c);
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

// stub of ROR 0x0012f060
void MagLifeFader::start(const MagFadeParam *p)
{
    ROSEPAN_STUB("MagLifeFader::start", 0x0012f060);
}

// stub of ROR 0x0012f080
void MagLifeFader::stop()
{
    ROSEPAN_STUB("MagLifeFader::stop", 0x0012f080);
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

// stub of ROR 0x0013f280
int MagScene::openSysMsgWindow(int msg, const int *args, int arg)
{
    ROSEPAN_STUB("MagScene::openSysMsgWindow", 0x0013f280);
    return {};
}

// stub of ROR 0x002113a4
void Sequence::jumpTo(intptr_t target)
{
    ROSEPAN_STUB("Sequence::jumpTo", 0x002113a4);
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
