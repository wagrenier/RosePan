// lib/unassigned_stubs.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.
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
#include "rx/rxthread.h"
#include "sequence/sequence.h"
#include "snd/sndman.h"

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


// stub of ROR 0x00237c68
int i3dPrintf(const char *fmt)
{
    ROSEPAN_STUB("i3dPrintf", 0x00237c68);
    return {};
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

// stub of ROR 0x00223d48
int i3dBatchAppend(I3dGroup *dst, I3dGroup *src)
{
    ROSEPAN_STUB("i3dBatchAppend", 0x00223d48);
    return {};
}

// stub of ROR 0x0021b288
int i3dBatchClearTextures(I3dDrawSet *set)
{
    ROSEPAN_STUB("i3dBatchClearTextures", 0x0021b288);
    return {};
}

// stub of ROR 0x00219d68
u_long * i3dClutMakeMove(void *clut, u_long *pkt, u_int src, u_int dst)
{
    ROSEPAN_STUB("i3dClutMakeMove", 0x00219d68);
    return {};
}

// stub of ROR 0x00219d10
u_long * i3dClutMakeUpload(void *clut, u_long *pkt, u_int dst)
{
    ROSEPAN_STUB("i3dClutMakeUpload", 0x00219d10);
    return {};
}

// stub of ROR 0x00224798
int i3dCtxCloseFrame(i3dContext *ctx)
{
    ROSEPAN_STUB("i3dCtxCloseFrame", 0x00224798);
    return {};
}

// stub of ROR 0x002246c8
int i3dCtxRunGroup(i3dContext *ctx, I3dGroup *group)
{
    ROSEPAN_STUB("i3dCtxRunGroup", 0x002246c8);
    return {};
}

// stub of ROR 0x00237cc8
int i3dFlushCache(void)
{
    ROSEPAN_STUB("i3dFlushCache", 0x00237cc8);
    return {};
}

// stub of ROR 0x00224878
void i3dGroupAddLayer(I3dGroup *group, u_int key, I3dGroup *layer)
{
    ROSEPAN_STUB("i3dGroupAddLayer", 0x00224878);
}

// stub of ROR 0x002243a0
int i3dGroupInit(I3dGroup *group, i3dContext *ctx, int a, int b, int c)
{
    ROSEPAN_STUB("i3dGroupInit", 0x002243a0);
    return {};
}

// stub of ROR 0x00223878
int i3dLayerInit(I3dLayer *layer, i3dContext *ctx)
{
    ROSEPAN_STUB("i3dLayerInit", 0x00223878);
    return {};
}

// stub of ROR 0x00224368
I3dNode * i3dSortList(I3dNode *first, int order, int b, u_int count)
{
    ROSEPAN_STUB("i3dSortList", 0x00224368);
    return {};
}

// stub of ROR 0x00237178
void i3dSysInit(void)
{
    ROSEPAN_STUB("i3dSysInit", 0x00237178);
}

// stub of ROR 0x00219d40
u_long * i3dTexMakeMove(void *tex, u_long *pkt, u_int src, u_int dst)
{
    ROSEPAN_STUB("i3dTexMakeMove", 0x00219d40);
    return {};
}

// stub of ROR 0x00219c88
u_long * i3dTexMakeUpload(void *tex, u_long *pkt, u_int dst)
{
    ROSEPAN_STUB("i3dTexMakeUpload", 0x00219c88);
    return {};
}

// stub of ROR 0x00224b28
int i3dTexRegsSet(u_long *regs, I3dVramHandle *tex, I3dVramHandle *clut,
                    u_int arg)
{
    ROSEPAN_STUB("i3dTexRegsSet", 0x00224b28);
    return {};
}

// stub of ROR 0x00224d88
void i3dTexRegsSetNone(u_long *regs)
{
    ROSEPAN_STUB("i3dTexRegsSetNone", 0x00224d88);
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

// stub of ROR 0x0021a120
void i3dFogInit(I3dFog *fog)
{
    ROSEPAN_STUB("i3dFogInit", 0x0021a120);
}

// stub of ROR 0x0017c1f4
int FUN_0017c1f4(Obj17c0f0 *obj, void *data, int copy)
{
    ROSEPAN_STUB("FUN_0017c1f4", 0x0017c1f4);
    return {};
}

// stub of ROR 0x001be414
int FileReader::close()
{
    ROSEPAN_STUB("FileReader::close", 0x001be414);
    return {};
}

// stub of ROR 0x001be524
int FileReader::read(FileReadReq *req, int async)
{
    ROSEPAN_STUB("FileReader::read", 0x001be524);
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

// stub of ROR 0x001be84c
int fileExists(const char *theName)
{
    ROSEPAN_STUB("fileExists", 0x001be84c);
    return {};
}

// stub of ROR 0x001be768
int fileGetSize(const char *theName)
{
    ROSEPAN_STUB("fileGetSize", 0x001be768);
    return {};
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
    return {};
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

// stub of ROR 0x00226c58
I3dView * i3dCtxCreateView(i3dContext *ctx, MagViewParam *param, sceVu0FVECTOR *matrix)
{
    ROSEPAN_STUB("i3dCtxCreateView", 0x00226c58);
    return {};
}

// stub of ROR 0x00223378
u_long * i3dLayerBeginPacket(I3dLayer *layer)
{
    ROSEPAN_STUB("i3dLayerBeginPacket", 0x00223378);
    return {};
}

// stub of ROR 0x00223398
int i3dLayerEndPacket(I3dLayer *layer, u_long *end)
{
    ROSEPAN_STUB("i3dLayerEndPacket", 0x00223398);
    return {};
}

// stub of ROR 0x00223240
int i3dLayerSetLight(I3dLayer *layer, I3dLight *light)
{
    ROSEPAN_STUB("i3dLayerSetLight", 0x00223240);
    return {};
}

// stub of ROR 0x00223260
int i3dLayerSetView(I3dLayer *layer, I3dView *view)
{
    ROSEPAN_STUB("i3dLayerSetView", 0x00223260);
    return {};
}

// stub of ROR 0x0021b550
u_long * i3dPktSetReg(u_long *pkt, u_long reg, u_long value)
{
    ROSEPAN_STUB("i3dPktSetReg", 0x0021b550);
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

// stub of ROR 0x00222f88
int FUN_00222f88(I3dLayer *layer, void *a, void *b, void *rec)
{
    ROSEPAN_STUB("FUN_00222f88", 0x00222f88);
    return {};
}

// stub of ROR 0x00223050
int FUN_00223050(void *a, I3dLayer *layer, void *b, const float *c)
{
    ROSEPAN_STUB("FUN_00223050", 0x00223050);
    return {};
}

// stub of ROR 0x002230e8
int FUN_002230e8(I3dLayer *layer, void *a, const float *b, void *c, void *rec)
{
    ROSEPAN_STUB("FUN_002230e8", 0x002230e8);
    return {};
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

// stub of ROR 0x0021b148
I3dLight * i3dCtxCreateLight(i3dContext *ctx, const I3dLightDesc *desc)
{
    ROSEPAN_STUB("i3dCtxCreateLight", 0x0021b148);
    return {};
}

// stub of ROR 0x0021a158
void     * i3dCtxSetFog(i3dContext *ctx, const I3dFog *fog)
{
    ROSEPAN_STUB("i3dCtxSetFog", 0x0021a158);
    return {};
}

// stub of ROR 0x0021b260
u_long   * i3dDrawSetUseTex(I3dDrawSet *set, int n)
{
    ROSEPAN_STUB("i3dDrawSetUseTex", 0x0021b260);
    return {};
}

// stub of ROR 0x00223298
u_long   * i3dLayerBeginSet(I3dLayer *layer, float key)
{
    ROSEPAN_STUB("i3dLayerBeginSet", 0x00223298);
    return {};
}

// stub of ROR 0x002232f0
void i3dLayerEndSet(I3dLayer *layer, u_long *end)
{
    ROSEPAN_STUB("i3dLayerEndSet", 0x002232f0);
}

// stub of ROR 0x00223558
I3dDrawSet * i3dLayerFindDrawSet(I3dLayer *layer, const I3dTexSetDesc *desc)
{
    ROSEPAN_STUB("i3dLayerFindDrawSet", 0x00223558);
    return {};
}

// stub of ROR 0x00223270
I3dDrawSet * i3dLayerGetDrawSet(I3dLayer *layer)
{
    ROSEPAN_STUB("i3dLayerGetDrawSet", 0x00223270);
    return {};
}

// stub of ROR 0x00223228
int i3dLayerSetDrawSet(I3dLayer *layer, I3dDrawSet *set)
{
    ROSEPAN_STUB("i3dLayerSetDrawSet", 0x00223228);
    return {};
}

// stub of ROR 0x00223250
int i3dLayerSetFog(I3dLayer *layer, void *fog)
{
    ROSEPAN_STUB("i3dLayerSetFog", 0x00223250);
    return {};
}

// stub of ROR 0x0021ac80
void i3dLightDescInit(I3dLightDesc *desc)
{
    ROSEPAN_STUB("i3dLightDescInit", 0x0021ac80);
}

// stub of ROR 0x00237678
void i3dMatInverse(sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    ROSEPAN_STUB("i3dMatInverse", 0x00237678);
}

// stub of ROR 0x00237ba0
void i3dMatLookDir(sceVu0FVECTOR *out, const float *eye, const float *dir,
                    const float *up)
{
    ROSEPAN_STUB("i3dMatLookDir", 0x00237ba0);
}

// stub of ROR 0x00237818
void i3dMatRotX(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    ROSEPAN_STUB("i3dMatRotX", 0x00237818);
}

// stub of ROR 0x00237830
void i3dMatRotY(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    ROSEPAN_STUB("i3dMatRotY", 0x00237830);
}

// stub of ROR 0x00237848
void i3dMatRotZ(float angle, sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    ROSEPAN_STUB("i3dMatRotZ", 0x00237848);
}

// stub of ROR 0x00237640
void i3dMatTranslate(sceVu0FVECTOR *out, sceVu0FVECTOR *in, const float *v)
{
    ROSEPAN_STUB("i3dMatTranslate", 0x00237640);
}

// stub of ROR 0x00237340
void i3dMatUnit(sceVu0FVECTOR *m)
{
    ROSEPAN_STUB("i3dMatUnit", 0x00237340);
}

// stub of ROR 0x0021c8c8
u_long i3dPackRGBAQ(float q, u_int r, u_int g, u_int b, u_int a)
{
    ROSEPAN_STUB("i3dPackRGBAQ", 0x0021c8c8);
    return {};
}

// stub of ROR 0x0021c940
u_long i3dPackXYZ(u_int x, u_int y, u_int z)
{
    ROSEPAN_STUB("i3dPackXYZ", 0x0021c940);
    return {};
}

// stub of ROR 0x0021b490
u_long   * i3dPktDmaTag(u_long *pkt, u_int qwc, u_int id, const void *addr,
                       u_int vif0, u_int vif1)
{
    ROSEPAN_STUB("i3dPktDmaTag", 0x0021b490);
    return {};
}

// stub of ROR 0x0021e1d8
u_long   * i3dPktDrawLineStrip(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix, int count,
                              const void *color, const void *xyz, int flags)
{
    ROSEPAN_STUB("i3dPktDrawLineStrip", 0x0021e1d8);
    return {};
}

// stub of ROR 0x0021db18
u_long   * i3dPktDrawStrip(u_long *pkt, sceVu0FVECTOR *matrix, int count, const void *stq,
                          const void *colors, const void *xyz, int flags)
{
    ROSEPAN_STUB("i3dPktDrawStrip", 0x0021db18);
    return {};
}

// stub of ROR 0x0021dd10
u_long   * i3dPktDrawStripCtx(u_long *pkt, I3dLayer *layer, sceVu0FVECTOR *matrix, int count,
                             const void *stq, const void *colors, const void *xyz,
                             int flags)
{
    ROSEPAN_STUB("i3dPktDrawStripCtx", 0x0021dd10);
    return {};
}

// stub of ROR 0x0021b4f8
u_long   * i3dPktRefTexRegs(u_long *pkt, u_long *regs)
{
    ROSEPAN_STUB("i3dPktRefTexRegs", 0x0021b4f8);
    return {};
}

// stub of ROR 0x0021c140
u_long   * i3dPktSprite(u_long *pkt, u_long xyz0, u_long xyz1, u_long rgbaq)
{
    ROSEPAN_STUB("i3dPktSprite", 0x0021c140);
    return {};
}

// stub of ROR 0x002372d0
void i3dVecCopy(float *out, const float *in)
{
    ROSEPAN_STUB("i3dVecCopy", 0x002372d0);
}

// stub of ROR 0x00237408
void i3dVecSub(float *out, const float *a, const float *b)
{
    ROSEPAN_STUB("i3dVecSub", 0x00237408);
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

// stub of ROR 0x002233e8
u_long * i3dLayerBeginPostPacket(I3dLayer *layer)
{
    ROSEPAN_STUB("i3dLayerBeginPostPacket", 0x002233e8);
    return {};
}

// stub of ROR 0x00223408
int i3dLayerEndPostPacket(I3dLayer *layer, u_long *end)
{
    ROSEPAN_STUB("i3dLayerEndPostPacket", 0x00223408);
    return {};
}

// stub of ROR 0x002373a8
void i3dMatApply(float *out, sceVu0FVECTOR *m, const float *v)
{
    ROSEPAN_STUB("i3dMatApply", 0x002373a8);
}

// stub of ROR 0x002377d0
void i3dMatMul(sceVu0FVECTOR *out, sceVu0FVECTOR *a, sceVu0FVECTOR *b)
{
    ROSEPAN_STUB("i3dMatMul", 0x002377d0);
}

// stub of ROR 0x0021c538
u_long   * i3dPktLine(u_long *pkt, u_long xyz0, u_long xyz1, u_long rgbaq)
{
    ROSEPAN_STUB("i3dPktLine", 0x0021c538);
    return {};
}

// stub of ROR 0x002372e0
void i3dVecCopy2(float *out, const float *in)
{
    ROSEPAN_STUB("i3dVecCopy2", 0x002372e0);
}

// stub of ROR 0x00237580
float i3dVecDot(const float *a, const float *b)
{
    ROSEPAN_STUB("i3dVecDot", 0x00237580);
    return {};
}

// stub of ROR 0x00237468
void i3dVecNeg(float *out, const float *in)
{
    ROSEPAN_STUB("i3dVecNeg", 0x00237468);
}

// stub of ROR 0x00237360
void i3dVecScale(float s, float *out, const float *in)
{
    ROSEPAN_STUB("i3dVecScale", 0x00237360);
}
