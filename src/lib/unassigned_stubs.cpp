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

// bridge of ROR 0x0021b288 (the ROM's code; unassigned@00216ad0 replaces it)
int i3dBatchClearTextures(I3dDrawSet *set)
{
    ROSEPAN_BRIDGE("i3dBatchClearTextures", 0x0021b288);
    for (u_int i = 0; i < set->m_tex_count; i++)
        if (set->m_tex_used[i] != 0)
            i3dTexRegsSetNone(set->m_tex_regs[i]);
    return 0;
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

// stub of ROR 0x00237cc8
int i3dFlushCache(void)
{
    ROSEPAN_STUB("i3dFlushCache", 0x00237cc8);
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

// bridge of ROR 0x0021b550 (the ROM's code; unassigned@00216ad0 replaces it):
// a CNT tag with VIF DIRECT 2 (0x21b4d0), an A+D GIF tag, value / reg.
u_long * i3dPktSetReg(u_long *pkt, u_long reg, u_long value)
{
    ROSEPAN_BRIDGE("i3dPktSetReg", 0x0021b550);
    pkt[5] = reg;
    pkt[2] = 0x1000000000008001ull;
    pkt[4] = value;
    pkt[3] = 0xe;
    return i3dPktDmaTag(pkt, 2, 0x10, NULL, 0x11000000, 2 | 0x50000000) + 4;
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

// bridge of ROR 0x0021b260 (the ROM's code; unassigned@00216ad0 replaces it)
u_long   * i3dDrawSetUseTex(I3dDrawSet *set, int n)
{
    ROSEPAN_BRIDGE("i3dDrawSetUseTex", 0x0021b260);
    set->m_tex_used[n]++;
    return set->m_tex_regs[n];
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

// bridge of ROR 0x0021b490 (the ROM's code; unassigned@00216ad0 replaces it)
u_long   * i3dPktDmaTag(u_long *pkt, u_int qwc, u_int id, const void *addr,
                       u_int vif0, u_int vif1)
{
    ROSEPAN_BRIDGE("i3dPktDmaTag", 0x0021b490);
    pkt[0] = ((u_long)i3dDmaAddr(addr) << 32) | (u_long)(u_int)(id << 24) | (qwc & 0xffff);
    pkt[1] = ((u_long)vif1 << 32) | vif0;
    return pkt + 2;
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

// bridge of ROR 0x0021b4f8 (the ROM's code; unassigned@00216ad0 replaces it):
// a REF of the 6-qword register packet with VIF DIRECT 6.
u_long   * i3dPktRefTexRegs(u_long *pkt, u_long *regs)
{
    ROSEPAN_BRIDGE("i3dPktRefTexRegs", 0x0021b4f8);
    return i3dPktDmaTag(pkt, 6, 0x30, regs, 0x11000000, 6 | 0x50000000);
}

// stub of ROR 0x0021c140
u_long   * i3dPktSprite(u_long *pkt, u_long xyz0, u_long xyz1, u_long rgbaq)
{
    ROSEPAN_STUB("i3dPktSprite", 0x0021c140);
    // PC: the packet cursor stays valid for the i3d groups (i3d/i3dlayer.cpp).
    return pkt;
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
    // PC: the packet cursor stays valid for the i3d groups (i3d/i3dlayer.cpp).
    return pkt;
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

// ---- i3d bridges the layer TU needs (i3d/i3dlayer.cpp, 2026-10-22) ----
// The ROM's code for small helpers of the packet module (unassigned@00216ad0),
// so the layers' packet cursor and draw sets stay valid until that TU is
// written; it replaces them.

// bridge of ROR 0x0021b600
u_long *i3dPktSetAlphaMode(u_long *pkt, int mode)
{
    ROSEPAN_BRIDGE("i3dPktSetAlphaMode", 0x0021b600);
    u_long alpha = 0;
    if (mode == 0)
        return pkt;
    switch (mode)
    {
    case 1: alpha = 0x40; break;
    case 2: alpha = 0; break;
    case 0x10: alpha = 0x44; break;
    case 0x11: alpha = 0x48; break;
    case 0x12: alpha = 0x42; break;
    case 0x20: alpha = 0x54; break;
    case 0x21: alpha = 0x58; break;
    case 0x22: alpha = 0x52; break;
    }
    return i3dPktSetReg(pkt, 0x42, alpha);
}

// bridge of ROR 0x0021b698: blend mode 0x10, then TEST_1 0x5000d.
u_long *i3dPktOpenChain(u_char *pkt)
{
    ROSEPAN_BRIDGE("i3dPktOpenChain", 0x0021b698);
    return i3dPktSetReg(i3dPktSetAlphaMode((u_long *)pkt, 0x10), 0x47, 0x5000d);
}

// stub of ROR 0x0021b5a0
u_long *i3dPktCallView(u_long *pkt, I3dLayer *layer)
{
    ROSEPAN_STUB("i3dPktCallView", 0x0021b5a0);
    // PC: a CALL of view +0x3f8 when the layer has a view; i3dCtxCreateView
    // is a stub (NULL), so there is none yet.
    return pkt;
}

// stub of ROR 0x0021b5d0
u_long *i3dPktCallFog(u_long *pkt, I3dLayer *layer)
{
    ROSEPAN_STUB("i3dPktCallFog", 0x0021b5d0);
    // PC: a CALL of fog +0x18 when the layer has fog; i3dCtxSetFog is a stub.
    return pkt;
}

// bridge of ROR 0x0021b220
void i3dDrawSetInsertAfter(I3dDrawSet *set, I3dDrawSet *at)
{
    ROSEPAN_BRIDGE("i3dDrawSetInsertAfter", 0x0021b220);
    I3dDrawSet *next = at->m_next;
    set->m_next = next;
    next->m_prev = set;
    at->m_next = set;
    set->m_prev = at;
}

// bridge of ROR 0x0021b238
void i3dDrawSetMoveAfter(I3dDrawSet *set, I3dDrawSet *at)
{
    ROSEPAN_BRIDGE("i3dDrawSetMoveAfter", 0x0021b238);
    set->m_prev->m_next = set->m_next;
    set->m_next->m_prev = set->m_prev;
    I3dDrawSet *next = at->m_next;
    set->m_next = next;
    next->m_prev = set;
    at->m_next = set;
    set->m_prev = at;
}

// bridge of ROR 0x0021b300
void i3dTexSetDescInit(I3dTexSetDesc *desc)
{
    ROSEPAN_BRIDGE("i3dTexSetDescInit", 0x0021b300);
    desc->m_sort_param = 0;
    desc->m_depth = 0.0f;
    desc->m_tex_no = NULL;
    desc->m_tex_count = 0;
    desc->m_sort_func = 0;
    desc->m_sort_order = 0;
}

// bridge of ROR 0x0021b320: a draw set from a description, its group and
// its per-slot texture numbers, register packets (6 qwords) and use counts.
int i3dDrawSetInit(I3dDrawSet *set, I3dLayer *layer, const I3dTexSetDesc *desc, u_int hash)
{
    ROSEPAN_BRIDGE("i3dDrawSetInit", 0x0021b320);
    i3dContext *ctx = layer->m_ctx;
    set->m_hash = hash;
    set->m_next = set;
    set->m_prev = set;
    set->m_group = i3dCtxCreateGroup(ctx, desc->m_sort_func, desc->m_sort_order, desc->m_sort_param);
    set->m_layer = layer;
    set->m_tex_regs = NULL;
    set->m_reserved_next = NULL;
    set->m_tex_index = NULL;
    set->m_tex_count = desc->m_tex_count;
    set->m_depth = desc->m_depth;
    set->m_sort_func = desc->m_sort_func;
    set->m_sort_order = desc->m_sort_order;
    set->m_sort_param = desc->m_sort_param;
    set->m_tex_used = NULL;
    if (desc->m_tex_count != 0)
    {
        // PC: host-sized arrays (the ROM: count words each, 16-byte rounded).
        u_int n = desc->m_tex_count;
        u_int *index = (u_int *)i3dCtxAllocTail(ctx, (int)((n * sizeof(u_int) + 15) >> 4));
        set->m_tex_regs = (u_long **)i3dCtxAllocTail(ctx, (int)((n * sizeof(u_long *) + 15) >> 4));
        set->m_tex_used = (int *)i3dCtxAllocTail(ctx, (int)((n * sizeof(int) + 15) >> 4));
        set->m_tex_index = index;
        for (u_int i = 0; i < n; i++)
        {
            index[i] = (u_int)desc->m_tex_no[i];
            set->m_tex_regs[i] = (u_long *)i3dCtxAllocTail(ctx, 6);
            set->m_tex_used[i] = 0;
        }
    }
    return 0;
}

// bridge of ROR 0x00218db8: bits per pixel by GS pixel format.
int i3dPsmBits(int psm)
{
    ROSEPAN_BRIDGE("i3dPsmBits", 0x00218db8);
    switch (psm)
    {
    case 0x00: case 0x01: case 0x1b: case 0x24: case 0x2c: return 0x20;
    case 0x02: case 0x0a: return 0x10;
    case 0x13: return 8;
    case 0x14: return 4;
    }
    return 0;
}

// bridge of ROR 0x00236980
void i3dVecCopyW(float *out, const float *in)
{
    ROSEPAN_BRIDGE("i3dVecCopyW", 0x00236980);
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
    out[3] = in[3];
}

// bridge of ROR 0x00237168: data 0x303c10.
sceVu0FVECTOR *i3dMatGetUnit(void)
{
    ROSEPAN_BRIDGE("i3dMatGetUnit", 0x00237168);
    static sceVu0FMATRIX s_unit = {
        { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f },
    };
    return s_unit;
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
