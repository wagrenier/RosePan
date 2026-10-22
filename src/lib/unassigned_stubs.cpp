// lib/unassigned_stubs.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.
#include <math.h>
#include <string.h>

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

// stub of ROR 0x0017f164
void SndMan::init()
{
    ROSEPAN_STUB("SndMan::init", 0x0017f164);
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

// stub of ROR 0x0020e3d8
void seqSetLanguage(int lang, int arg)
{
    ROSEPAN_STUB("seqSetLanguage", 0x0020e3d8);
}

// stub of ROR 0x00204148
void sysInitManagers(void)
{
    ROSEPAN_STUB("sysInitManagers", 0x00204148);
}

// ---- bridges (hand-written) ----

// stub of ROR 0x00181204
void RxMpeg::setFrameParity(int parity)
{
    ROSEPAN_STUB("RxMpeg::setFrameParity", 0x00181204);
}

// stub of ROR 0x0017f200
void SndMan::update()
{
    ROSEPAN_STUB("SndMan::update", 0x0017f200);
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

// stub of ROR 0x0022b3d8
int i3rGetNodeIndexByName(I3rRoot *root, const char *name)
{
    ROSEPAN_STUB("i3rGetNodeIndexByName", 0x0022b3d8);
    return {};
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

// stub of ROR 0x0022b228
int I3rRoot__CheckIdString(const I3rRoot *root)
{
    ROSEPAN_STUB("I3rRoot__CheckIdString", 0x0022b228);
    return {};
}

// stub of ROR 0x00140030
void MagScene::loadTenkyuTex(int no, const BedPfh *pfh, i3dContext *ctx)
{
    ROSEPAN_STUB("MagScene::loadTenkyuTex", 0x00140030);
}

// stub of ROR 0x00135fa4
void MagScene::resetEntities()
{
    ROSEPAN_STUB("MagScene::resetEntities", 0x00135fa4);
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

// stub of ROR 0x00131618
void MagScene::setViewFar(float far)
{
    ROSEPAN_STUB("MagScene::setViewFar", 0x00131618);
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

// stub of ROR 0x0013555c
int MagScene::getWindowTextWidth(int no, const char *text)
{
    ROSEPAN_STUB("MagScene::getWindowTextWidth", 0x0013555c);
    return {};
}

// stub of ROR 0x00132784
void MagScene::popSubMode()
{
    ROSEPAN_STUB("MagScene::popSubMode", 0x00132784);
}

// stub of ROR 0x00132744
void MagScene::pushSubMode()
{
    ROSEPAN_STUB("MagScene::pushSubMode", 0x00132744);
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

// stub of ROR 0x0011262c
I3dLight * DogLightPool::createLight(i3dContext *ctx, int set, int arg, float scale)
{
    ROSEPAN_STUB("DogLightPool::createLight", 0x0011262c);
    return {};
}

// stub of ROR 0x00177830
void FUN_00177830(Obj176a3c *obj, i3dContext *ctx)
{
    ROSEPAN_STUB("FUN_00177830", 0x00177830);
}

// stub of ROR 0x00133454
float MagScene::castRay(MagSceneRayHit *hit, const float *pos, const float *dir, int tenkyu)
{
    ROSEPAN_STUB("MagScene::castRay", 0x00133454);
    return {};
}

// stub of ROR 0x00134698
void MagScene::clearStrip(int no)
{
    ROSEPAN_STUB("MagScene::clearStrip", 0x00134698);
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

// stub of ROR 0x001129cc
int DogLightPool::findNearestPointLight(int set, const float *pos)
{
    ROSEPAN_STUB("DogLightPool::findNearestPointLight", 0x001129cc);
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

// stub of ROR 0x0022b428
int i3rGetNumTex(const I3rRoot *root)
{
    ROSEPAN_STUB("i3rGetNumTex", 0x0022b428);
    return {};
}

// stub of ROR 0x0022b438
const char * i3rGetTexName(const I3rRoot *root, int n)
{
    ROSEPAN_STUB("i3rGetTexName", 0x0022b438);
    return {};
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
