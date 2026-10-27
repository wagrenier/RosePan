// lib/unassigned_stubs.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.
#include <math.h>
#include <string.h>
#include "game/gamsnd.h"
#include "rx/rxsnd.h"
#include "mnu/mnuSelectLanguage.h"
#include "msg/msgman.h"
#include "debug/motnamecnv0.h"
#include "task/naga_chunk.h"
#include "task/tsk_scene.h"
#include "dog/dogtexinfo.h"

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


int g_emyManagerTaskId;         /* bss 0x7b45c0 */
int g_bss_7a3d48;               /* bss 0x7a3d48 */
int g_bss_7a3d4c;               /* bss 0x7a3d4c */
int g_bss_7a3210;               /* bss 0x7a3210 */
int g_bss_7812ac;               /* bss 0x7812ac */
int g_bss_780d04;               /* bss 0x780d04: GamMan option 0 (brightness) */

const char *g_versionNumber = "1.01";   /* data 0x2d9e18 -> 0x301720 */

// ---- MnuTextList (unassigned@00192ac0), written ahead for TskFrameFunc kind 8 ----

// ROR 0x001a066c

MnuTextList::MnuTextList()
{
    m_open = 0;
    clear();
}

// ROR 0x001a0684

MnuTextList::~MnuTextList()
{
    close();
}

// ROR 0x001a0860

int MnuTextList::clear()
{
    m_cur = 0;
    for (int i = 0; i < 6; i++)
        m_08[i] = 0;
    m_28 = 2;
    m_6c = 0;
    m_24 = 2;
    m_70 = 0;
    m_20 = 1;
    m_60 = -1;
    for (int i = 0; i < 0x20; i++)
        m_84[i] = 0;
    return 0;
}

// ---- utlStrNCopy (unassigned@00216ad0), written ahead for TskFrameFunc kind 15 ----

// ROR 0x0021838c

void utlStrNCopy(char *dst, const char *src, int n)
{
    int i = 0;

    if (*src != '\0')
    {
        i = 1;
        if (n > 0)
        {
            for (;;)
            {
                *dst++ = *src++;
                if (*src == '\0')
                    break;
                int more = i < n;
                i++;
                if (!more)
                    break;
            }
        }
    }
    if (i < n)
        *dst++ = '\0';
    if (i + 1 < n)
        *dst = '\0';
}

// ---- utlDirDist (unassigned@00216ad0), written ahead for TskMovePointPath ----
// The heading pi/2 - acos(x / len) (negated for z < 0), each wrapped once
// into -pi..pi in double as the ROM does.
// callcheck: sqrtf, acosf (FUN_0026cad0), fptodp, dpcmp, dpadd, dpsub, dptofp
// ROR 0x002180e4

float utlDirDist(float x, float z, float *dist)
{
    const double pi = 3.141592653589793, twoPi = 6.283185307179586;
    float len = sqrtf(x * x + z * z);

    *dist = len;
    if (len == 0.0f)
    {
        *dist = 0.0f;
        return 0.0f;
    }
    float a = acosf(x / len);
    if (z < 0.0f)
        a = -a;
    if ((double)a < -pi)
        a = (float)((double)a + twoPi);
    if ((double)a > pi)
        a = (float)((double)a - twoPi);
    float r = (float)((double)-a + 1.5707963267948966);
    if ((double)r < -pi)
        r = (float)((double)r + twoPi);
    if ((double)r > pi)
        r = (float)((double)r - twoPi);
    return r;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x002155a0
void debugNop(void)
{
    ROSEPAN_STUB("debugNop", 0x002155a0);
}

// ---- bridges (hand-written) ----

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

// stub of ROR 0x0010fa6c
sceVu0FVECTOR * DogEntity::getMatrix()
{
    ROSEPAN_STUB("DogEntity::getMatrix", 0x0010fa6c);
    // PC: the unit matrix until the camera class that holds the +0xf0
    // matrix is written; i3dCtxCreateView copies what this returns.
    return i3dMatGetUnit();
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
int GamMan::placeAchr(int oid, void *out, int arg)
{
    ROSEPAN_STUB("GamMan::placeAchr", 0x001d544c);
    return 0;
}

// stub of ROR 0x001d5da0
void GamMan::relinkObj(u_int oid)
{
    ROSEPAN_STUB("GamMan::relinkObj", 0x001d5da0);
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

// ---- stubs (hand-written: the mnu/ menu objects' virtual slots, which stubgen cannot see behind the vtable) ----

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
int MnuTextList::processPad(BedPad *pad)
{
    ROSEPAN_STUB("MnuTextList::processPad", 0x001a06d4);
    return {};
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

// stub of ROR 0x001d5a0c
int GamMan::testHitAt(const float *pos, u_int oid, int a3, int a4, void *out)
{
    ROSEPAN_STUB("GamMan::testHitAt", 0x001d5a0c);
    return {};
}

// stub of ROR 0x0018761c
void MsgManSet::release(int window)
{
    ROSEPAN_STUB("MsgManSet::release", 0x0018761c);
}


// stub of ROR 0x001d81f0
int GamMan::findSectaPath(int list, int from, int to, GamSectaPath *path)
{
    ROSEPAN_STUB("GamMan::findSectaPath", 0x001d81f0);
    return {};
}

// stub of ROR 0x001d7b08
int GamMan::getObjSectaNo(u_int oid, int list)
{
    ROSEPAN_STUB("GamMan::getObjSectaNo", 0x001d7b08);
    return {};
}

// stub of ROR 0x001417e4
void MagScene::setFocusData(const void *data, int size)
{
    ROSEPAN_STUB("MagScene::setFocusData", 0x001417e4);
}

// stub of ROR 0x001fccf4
int NagaChunk::find(const char *name)
{
    ROSEPAN_STUB("NagaChunk::find", 0x001fccf4);
    return {};
}

// stub of ROR 0x001fcdb8
void * NagaChunk::getData()
{
    ROSEPAN_STUB("NagaChunk::getData", 0x001fcdb8);
    return {};
}

// stub of ROR 0x001fcda4
NagaChunkEntry * NagaChunk::getEntry()
{
    ROSEPAN_STUB("NagaChunk::getEntry", 0x001fcda4);
    return {};
}

// stub of ROR 0x001fcdd4
int NagaChunk::getInt()
{
    ROSEPAN_STUB("NagaChunk::getInt", 0x001fcdd4);
    return {};
}

// stub of ROR 0x001fcc20
int NagaChunk::next()
{
    ROSEPAN_STUB("NagaChunk::next", 0x001fcc20);
    return {};
}

// stub of ROR 0x001fcbf8
void NagaChunk::rewind()
{
    ROSEPAN_STUB("NagaChunk::rewind", 0x001fcbf8);
}

// stub of ROR 0x002162e0
int motNameGetCategory(int chara)
{
    ROSEPAN_STUB("motNameGetCategory", 0x002162e0);
    return {};
}

// stub of ROR 0x00218338
void utlStrToLower(char *dst, const char *src)
{
    ROSEPAN_STUB("utlStrToLower", 0x00218338);
}

// stub of ROR 0x00163e10
void EmyManager::pauseEmy(u_int oid, int on)
{
    ROSEPAN_STUB("EmyManager::pauseEmy", 0x00163e10);
}

// stub of ROR 0x001fd6bc
int TskAttack::stepKind12()
{
    ROSEPAN_STUB("TskAttack::stepKind12", 0x001fd6bc);
    return {};
}

// stub of ROR 0x001fd9bc
int TskAttack::stepKind15()
{
    ROSEPAN_STUB("TskAttack::stepKind15", 0x001fd9bc);
    return {};
}

// stub of ROR 0x001ffce0
int TskAttack::stepKind26()
{
    ROSEPAN_STUB("TskAttack::stepKind26", 0x001ffce0);
    return {};
}

// stub of ROR 0x001ff494
int TskAttack::stepKind2e()
{
    ROSEPAN_STUB("TskAttack::stepKind2e", 0x001ff494);
    return {};
}

// stub of ROR 0x001ff664
int TskAttack::stepKind2f()
{
    ROSEPAN_STUB("TskAttack::stepKind2f", 0x001ff664);
    return {};
}

// stub of ROR 0x001ff830
int TskAttack::stepKind30()
{
    ROSEPAN_STUB("TskAttack::stepKind30", 0x001ff830);
    return {};
}

// stub of ROR 0x0010423c
int utlRandRange(int lo, int hi)
{
    ROSEPAN_STUB("utlRandRange", 0x0010423c);
    return {};
}

// stub of ROR 0x00163c50
void EmyManager::clearEmyWord18(u_int oid)
{
    ROSEPAN_STUB("EmyManager::clearEmyWord18", 0x00163c50);
}

// stub of ROR 0x00163d90
int EmyManager::getEmyWord1d8(u_int oid)
{
    ROSEPAN_STUB("EmyManager::getEmyWord1d8", 0x00163d90);
    return {};
}

// stub of ROR 0x00163f1c
int EmyManager::getEmyWord238(u_int oid)
{
    ROSEPAN_STUB("EmyManager::getEmyWord238", 0x00163f1c);
    return {};
}

// stub of ROR 0x00163624
void EmyManager::notifyEmy(u_int oid, int bits)
{
    ROSEPAN_STUB("EmyManager::notifyEmy", 0x00163624);
}

// stub of ROR 0x00163c24
void EmyManager::setEmyWord18(u_int oid)
{
    ROSEPAN_STUB("EmyManager::setEmyWord18", 0x00163c24);
}

// stub of ROR 0x001fe160
int TskDamage::stepKind0()
{
    ROSEPAN_STUB("TskDamage::stepKind0", 0x001fe160);
    return {};
}

// stub of ROR 0x001ff2e0
int TskDamage::stepKind1()
{
    ROSEPAN_STUB("TskDamage::stepKind1", 0x001ff2e0);
    return {};
}

// stub of ROR 0x001ffbc0
int TskDamage::stepKind2()
{
    ROSEPAN_STUB("TskDamage::stepKind2", 0x001ffbc0);
    return {};
}

// stub of ROR 0x00200444
int TskDamage::stepKind5()
{
    ROSEPAN_STUB("TskDamage::stepKind5", 0x00200444);
    return {};
}

// stub of ROR 0x002005b8
int TskDamage::stepKind6()
{
    ROSEPAN_STUB("TskDamage::stepKind6", 0x002005b8);
    return {};
}

// stub of ROR 0x001fffc0
int TskDamage::stepKind7()
{
    ROSEPAN_STUB("TskDamage::stepKind7", 0x001fffc0);
    return {};
}

// stub of ROR 0x0020014c
int TskDamage::stepKind8()
{
    ROSEPAN_STUB("TskDamage::stepKind8", 0x0020014c);
    return {};
}

// stub of ROR 0x001ff9fc
int TskDamage::stepKind9()
{
    ROSEPAN_STUB("TskDamage::stepKind9", 0x001ff9fc);
    return {};
}

// stub of ROR 0x002002cc
int TskDamage::stepKindA()
{
    ROSEPAN_STUB("TskDamage::stepKindA", 0x002002cc);
    return {};
}

// stub of ROR 0x0016a8a4
void emyPlaySE(u_int oid, int no)
{
    ROSEPAN_STUB("emyPlaySE", 0x0016a8a4);
}

// stub of ROR 0x00163f9c
int EmyManager::getEmyHoldWord23c(u_int oid)
{
    ROSEPAN_STUB("EmyManager::getEmyHoldWord23c", 0x00163f9c);
    return {};
}

// stub of ROR 0x00163f44
int EmyManager::getEmyHoldWord24c(u_int oid)
{
    ROSEPAN_STUB("EmyManager::getEmyHoldWord24c", 0x00163f44);
    return {};
}

// stub of ROR 0x00163e74
void EmyManager::setEmyMode6(u_int oid)
{
    ROSEPAN_STUB("EmyManager::setEmyMode6", 0x00163e74);
}

// stub of ROR 0x00163e48
void EmyManager::stopEmyMove(u_int oid)
{
    ROSEPAN_STUB("EmyManager::stopEmyMove", 0x00163e48);
}

// stub of ROR 0x0014426c
void magBwnHurt(int react, int damage)
{
    ROSEPAN_STUB("magBwnHurt", 0x0014426c);
}

// stub of ROR 0x0010426c
int utlRandMean(int lo, int hi, int n)
{
    ROSEPAN_STUB("utlRandMean", 0x0010426c);
    return {};
}

// stub of ROR 0x00104438
int utlRandPeak(int lo, int hi, int peak, int n)
{
    ROSEPAN_STUB("utlRandPeak", 0x00104438);
    return {};
}

// stub of ROR 0x0010fd18
int DogEntity::makeLocalMatrix(sceVu0FVECTOR *out)
{
    ROSEPAN_STUB("DogEntity::makeLocalMatrix", 0x0010fd18);
    return {};
}

// stub of ROR 0x0011764c
void DogTexInfo::dump()
{
    ROSEPAN_STUB("DogTexInfo::dump", 0x0011764c);
}

// stub of ROR 0x001141c8
int dogModelHitCylinder(I3cQuery *q, void *a1, const u_short *a2, int a3, float *a4, float *a5)
{
    ROSEPAN_STUB("dogModelHitCylinder", 0x001141c8);
    return {};
}

// stub of ROR 0x00114034
int dogModelHitNearest(I3cQuery *q, void *a1, const u_short *a2, int a3, float *a4, float *a5)
{
    ROSEPAN_STUB("dogModelHitNearest", 0x00114034);
    return {};
}

// stub of ROR 0x002325a8
int i3cCastCylinder(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats)
{
    ROSEPAN_STUB("i3cCastCylinder", 0x002325a8);
    return {};
}

// stub of ROR 0x00232508
int i3cCastRay(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats)
{
    ROSEPAN_STUB("i3cCastRay", 0x00232508);
    return {};
}

// stub of ROR 0x002324b8
int i3cCastSphere(I3cQuery *q, const float *shape, sceVu0FMATRIX *mats)
{
    ROSEPAN_STUB("i3cCastSphere", 0x002324b8);
    return {};
}

// stub of ROR 0x00106760
int utlDrawI3r(I3dLayer *layer, I3rRoot *root, sceVu0FMATRIX *nodes, sceVu0FVECTOR *world,
                 int flags)
{
    ROSEPAN_STUB("utlDrawI3r", 0x00106760);
    return {};
}

// stub of ROR 0x001067d8
int utlSetI3rDispControl(I3rRoot *root, void *disp)
{
    ROSEPAN_STUB("utlSetI3rDispControl", 0x001067d8);
    return {};
}

// stub of ROR 0x0021fb50
void i3rBuildMeshPacket(I3rItem *mesh, void *buf)
{
    ROSEPAN_STUB("i3rBuildMeshPacket", 0x0021fb50);
}

// stub of ROR 0x0021e5a0
u_long   * i3dPktBeginVu1Sprite(u_long *pkt, I3dLayer *layer, int mode)
{
    ROSEPAN_STUB("i3dPktBeginVu1Sprite", 0x0021e5a0);
    return {};
}

// stub of ROR 0x0021e688
u_long   * i3dPktEndVu1Sprite(u_long *pkt)
{
    ROSEPAN_STUB("i3dPktEndVu1Sprite", 0x0021e688);
    return {};
}

// stub of ROR 0x0021e488
u_long   * i3dPktVu1Color(u_long *pkt, const float *color)
{
    ROSEPAN_STUB("i3dPktVu1Color", 0x0021e488);
    return {};
}

// stub of ROR 0x0021e428
u_long   * i3dPktVu1Matrix(u_long *pkt, sceVu0FVECTOR *matrix)
{
    ROSEPAN_STUB("i3dPktVu1Matrix", 0x0021e428);
    return {};
}

// stub of ROR 0x0021e690
u_long   * i3dPktVu1Sprite(u_long *pkt, const float *pos, const float *uv)
{
    ROSEPAN_STUB("i3dPktVu1Sprite", 0x0021e690);
    return {};
}

// stub of ROR 0x0022e958
float i3mGetLength(const I3mRoot *root)
{
    ROSEPAN_STUB("i3mGetLength", 0x0022e958);
    return {};
}

// stub of ROR 0x0022e960
void i3mGetPose(I3mRoot *root, void *pose, int flags, float t)
{
    ROSEPAN_STUB("i3mGetPose", 0x0022e960);
}

// stub of ROR 0x0022f7a0
void i3mPoseToMatrices(const void *pose, sceVu0FMATRIX *mats, const u_short *map)
{
    ROSEPAN_STUB("i3mPoseToMatrices", 0x0022f7a0);
}

// stub of ROR 0x00106ad0
float utlVecLength3(const float *v)
{
    ROSEPAN_STUB("utlVecLength3", 0x00106ad0);
    return {};
}

// stub of ROR 0x00106a40
void utlVecRotZ(float angle, float *out, const float *in)
{
    ROSEPAN_STUB("utlVecRotZ", 0x00106a40);
}
