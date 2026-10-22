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

// ---- the vector module's second copy (unassigned@00228c08): bridges of
// the ROM's VU0 macro code for i3d/i3dpacket.cpp's float writers ----

// bridge of ROR 0x002372d0: lq / sq.
void i3dVecCopy(float *out, const float *in)
{
    ROSEPAN_BRIDGE("i3dVecCopy", 0x002372d0);
    memmove(out, in, 4 * sizeof(float));
}

// bridge of ROR 0x002373d8: vadd.xyzw.
void i3dVecAdd(float *out, const float *a, const float *b)
{
    ROSEPAN_BRIDGE("i3dVecAdd", 0x002373d8);
    for (int i = 0; i < 4; i++)
        out[i] = a[i] + b[i];
}

// bridge of ROR 0x00237438: vmul.xyzw.
void i3dVecMul(float *out, const float *a, const float *b)
{
    ROSEPAN_BRIDGE("i3dVecMul", 0x00237438);
    for (int i = 0; i < 4; i++)
        out[i] = a[i] * b[i];
}

// bridge of ROR 0x00237450: a byte copy of i3dVecMul.
void i3dVecMul2(float *out, const float *a, const float *b)
{
    ROSEPAN_BRIDGE("i3dVecMul2", 0x00237450);
    for (int i = 0; i < 4; i++)
        out[i] = a[i] * b[i];
}

// bridge of ROR 0x00237600: vftoi0.xyzw (truncates).
void i3dVecFtoi0(int *out, const float *in)
{
    ROSEPAN_BRIDGE("i3dVecFtoi0", 0x00237600);
    for (int i = 0; i < 4; i++)
        out[i] = (int)in[i];
}

// bridge of ROR 0x00237610: vftoi4.xyzw (12.4 fixed point, truncates).
void i3dVecFtoi4(int *out, const float *in)
{
    ROSEPAN_BRIDGE("i3dVecFtoi4", 0x00237610);
    for (int i = 0; i < 4; i++)
        out[i] = (int)(in[i] * 16.0f);
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
// The ROM's code for small helpers of the texture objects (unassigned@00216ad0)
// and the vector module, until their TUs replace them.

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

// bridge of ROR 0x00236960
void i3dVecZero(float *v)
{
    ROSEPAN_BRIDGE("i3dVecZero", 0x00236960);
    v[0] = v[1] = v[2] = v[3] = 0.0f;
}

// bridge of ROR 0x00236990
void i3dMatCopy(sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    ROSEPAN_BRIDGE("i3dMatCopy", 0x00236990);
    memmove(out, in, sizeof(sceVu0FMATRIX));
}

// bridge of ROR 0x00236a68: vf0 - vf0 - v.
void i3dVu0VecNeg(float *out, const float *in)
{
    ROSEPAN_BRIDGE("i3dVu0VecNeg", 0x00236a68);
    for (int k = 0; k < 4; k++)
        out[k] = 0.0f - in[k];
}

// bridge of ROR 0x00236af0: vopmula / vopmsub, w 0.
void i3dVecCross(float *out, const float *a, const float *b)
{
    ROSEPAN_BRIDGE("i3dVecCross", 0x00236af0);
    float x = a[1] * b[2] - b[1] * a[2];
    float y = a[2] * b[0] - b[2] * a[0];
    float z = a[0] * b[1] - b[0] * a[1];
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = 0.0f;
}

// bridge of ROR 0x00236b38: xyz times vrsqrt of the squared length, w 0.
void i3dVecNormalize(float *out, const float *in)
{
    ROSEPAN_BRIDGE("i3dVecNormalize", 0x00236b38);
    float q = 1.0f / sqrtf(in[0] * in[0] + in[1] * in[1] + in[2] * in[2]);
    out[0] = in[0] * q;
    out[1] = in[1] * q;
    out[2] = in[2] * q;
    out[3] = 0.0f;
}

// bridge of ROR 0x00236bd8: the adjugate rows (cross products) over the
// determinant, transposed; translation -t * R^-1; column w 0, 0, 0, 1.
void i3dVu0MatInverse(sceVu0FVECTOR *out, sceVu0FVECTOR *in)
{
    ROSEPAN_BRIDGE("i3dVu0MatInverse", 0x00236bd8);
    sceVu0FVECTOR c5, c6, c7;
    float t[3] = { -in[3][0], -in[3][1], -in[3][2] };
    i3dVecCross(c5, in[1], in[2]);
    i3dVecCross(c6, in[2], in[0]);
    i3dVecCross(c7, in[0], in[1]);
    float q = 1.0f / (c7[0] * in[2][0] + c7[1] * in[2][1] + c7[2] * in[2][2]);
    for (int i = 0; i < 3; i++)
    {
        out[i][0] = q * c5[i];
        out[i][1] = q * c6[i];
        out[i][2] = q * c7[i];
        out[i][3] = 0.0f;
    }
    for (int k = 0; k < 3; k++)
        out[3][k] = out[0][k] * t[0] + out[1][k] * t[1] + out[2][k] * t[2];
    out[3][3] = 1.0f;
}

// bridge of ROR 0x00236d30: VU0 micro 0 (vf4..7 = a, vf8..11 = b).
void i3dVu0MatMul(sceVu0FVECTOR *out, sceVu0FVECTOR *a, sceVu0FVECTOR *b)
{
    ROSEPAN_BRIDGE("i3dVu0MatMul", 0x00236d30);
    sceVu0FMATRIX r;
    for (int i = 0; i < 4; i++)
        for (int k = 0; k < 4; k++)
            r[i][k] = a[0][k] * b[i][0] + a[1][k] * b[i][1] + a[2][k] * b[i][2] + a[3][k] * b[i][3];
    memcpy(out, r, sizeof(r));
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
