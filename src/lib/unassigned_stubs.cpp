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
#include "emy/emy_main.h"

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


// stub of ROR 0x00215bb4
void dbgTraceVector4(const float *v)
{
    ROSEPAN_STUB("dbgTraceVector4", 0x00215bb4);
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

// stub of ROR 0x0017772c
void FUN_0017772c(Obj176a3c *obj)
{
    ROSEPAN_STUB("FUN_0017772c", 0x0017772c);
}

// stub of ROR 0x0017c340
void FUN_0017c340(Obj17c0f0 *obj, const float *pos, GamSecta **hit, int arg,
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

// stub of ROR 0x00140c68
void MagScene::updateRecords853fc()
{
    ROSEPAN_STUB("MagScene::updateRecords853fc", 0x00140c68);
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


// stub of ROR 0x00130de0
void MagLife130da4::setVolume(int volume)
{
    ROSEPAN_STUB("MagLife130da4::setVolume", 0x00130de0);
}

// stub of ROR 0x0017756c
void Obj176a3c::release(int no)
{
    ROSEPAN_STUB("Obj176a3c::release", 0x0017756c);
}

// ---- stubs (hand-written: EmyMain / EmyOut and the enemy classes' constructors and destructors, which stubgen does not write) ----

// stub of ROR 0x00164118
EmyMain::EmyMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,
                 const _tagEnemyTransition *trans, float *pos)
{
    ROSEPAN_STUB("EmyMain::EmyMain", 0x00164118);
    (void)prm;
    (void)trans;
    (void)pos;
    // PORT: the fields EmyManager reads, until the constructor is written.
    m_oid = oid;
    m_name = name;
    m_parsonality = 0;
    m_voice = 0;
    m_manage_sw = 0;
    m_visible = 0;
    m_down_flg = 0;
    m_damage = 0;
    m_down_supine = 0;
    m_param = param;
    m_notify = 0;
    m_state = 0;
    m_emy_out = NULL;
    m_state2 = 0;
    m_grp_no = 0;
    m_grp_state = 0;
    m_grp_wait = 0;
    m_grp_1d2 = 0;
    m_grp_respawn = 0;
    m_lp = 0;
    m_damage_end = 0;
    m_move = NULL;
    m_turn = NULL;
    m_238 = 0;
}

// stub of ROR 0x00164208
EmyMain::~EmyMain()
{
    ROSEPAN_STUB("EmyMain::~EmyMain", 0x00164208);
}

// stub of ROR 0x00166a1c
EmyOut::EmyOut(u_int oid, ACHRstat *achr, EmyMain *emy)
{
    ROSEPAN_STUB("EmyOut::EmyOut", 0x00166a1c);
}

// stub of ROR 0x00166a74
EmyOut::~EmyOut()
{
    ROSEPAN_STUB("EmyOut::~EmyOut", 0x00166a74);
}

// stub of ROR 0x0015c2a4
DwfMain::~DwfMain()
{
    ROSEPAN_STUB("DwfMain::~DwfMain", 0x0015c2a4);
}

// stub of ROR 0x0015a580
PigMain::~PigMain()
{
    ROSEPAN_STUB("PigMain::~PigMain", 0x0015a580);
}

// stub of ROR 0x00158798
GotMain::~GotMain()
{
    ROSEPAN_STUB("GotMain::~GotMain", 0x00158798);
}

// stub of ROR 0x00155d54
FshMain::~FshMain()
{
    ROSEPAN_STUB("FshMain::~FshMain", 0x00155d54);
}

// stub of ROR 0x00149eb0
BdsMain::~BdsMain()
{
    ROSEPAN_STUB("BdsMain::~BdsMain", 0x00149eb0);
}

// stub of ROR 0x0014b554
BdlMain::~BdlMain()
{
    ROSEPAN_STUB("BdlMain::~BdlMain", 0x0014b554);
}

// stub of ROR 0x0014cef8
MrtMain::~MrtMain()
{
    ROSEPAN_STUB("MrtMain::~MrtMain", 0x0014cef8);
}

// stub of ROR 0x0014de54
ClrMain::~ClrMain()
{
    ROSEPAN_STUB("ClrMain::~ClrMain", 0x0014de54);
}

// stub of ROR 0x00154030
GrgMain::~GrgMain()
{
    ROSEPAN_STUB("GrgMain::~GrgMain", 0x00154030);
}

// stub of ROR 0x00150df8
NorMain::~NorMain()
{
    ROSEPAN_STUB("NorMain::~NorMain", 0x00150df8);
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
    /* PC: "" not NULL -- MnuPBook::setPageText strlen()s it. The ROM reads
     * 0x1d4 string pointers at data 0x298fe8 (datadump.py 298fe8 468 S:text). */
    return "";
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

// stub of ROR 0x00217f10
float utlAtan2(float x, float z)
{
    ROSEPAN_STUB("utlAtan2", 0x00217f10);
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

// stub of ROR 0x0018761c
void MsgManSet::release(int window)
{
    ROSEPAN_STUB("MsgManSet::release", 0x0018761c);
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

// stub of ROR 0x0021fb50
void i3rBuildMeshPacket(I3rItem *mesh, void *buf)
{
    ROSEPAN_STUB("i3rBuildMeshPacket", 0x0021fb50);
}

// stub of ROR 0x0021e5a0
u_long   * i3dPktBeginVu1Sprite(u_long *pkt, I3dLayer *layer, int mode)
{
    ROSEPAN_STUB("i3dPktBeginVu1Sprite", 0x0021e5a0);
    /* PC: the packet back unchanged, not NULL -- the menus' next real
     * packet writer (i3dPktRefTexRegs, i3dPktSetReg) writes through it. */
    return pkt;
}

// stub of ROR 0x0021e688
u_long   * i3dPktEndVu1Sprite(u_long *pkt)
{
    ROSEPAN_STUB("i3dPktEndVu1Sprite", 0x0021e688);
    /* PC: the packet back unchanged, not NULL -- the menus' next real
     * packet writer (i3dPktRefTexRegs, i3dPktSetReg) writes through it. */
    return pkt;
}

// stub of ROR 0x0021e488
u_long   * i3dPktVu1Color(u_long *pkt, const float *color)
{
    ROSEPAN_STUB("i3dPktVu1Color", 0x0021e488);
    /* PC: the packet back unchanged, not NULL -- the menus' next real
     * packet writer (i3dPktRefTexRegs, i3dPktSetReg) writes through it. */
    return pkt;
}

// stub of ROR 0x0021e428
u_long   * i3dPktVu1Matrix(u_long *pkt, sceVu0FVECTOR *matrix)
{
    ROSEPAN_STUB("i3dPktVu1Matrix", 0x0021e428);
    /* PC: the packet back unchanged, not NULL -- the menus' next real
     * packet writer (i3dPktRefTexRegs, i3dPktSetReg) writes through it. */
    return pkt;
}

// stub of ROR 0x0021e690
u_long   * i3dPktVu1Sprite(u_long *pkt, const float *pos, const float *uv)
{
    ROSEPAN_STUB("i3dPktVu1Sprite", 0x0021e690);
    /* PC: the packet back unchanged, not NULL -- the menus' next real
     * packet writer (i3dPktRefTexRegs, i3dPktSetReg) writes through it. */
    return pkt;
}

// stub of ROR 0x0014b780
void BdlMain::after()
{
    ROSEPAN_STUB("BdlMain::after", 0x0014b780);
}

// stub of ROR 0x0014b5b4
void BdlMain::decide()
{
    ROSEPAN_STUB("BdlMain::decide", 0x0014b5b4);
}

// stub of ROR 0x0014b6d8
void BdlMain::exec()
{
    ROSEPAN_STUB("BdlMain::exec", 0x0014b6d8);
}

// stub of ROR 0x0014b3bc
void BdlMain::initOut()
{
    ROSEPAN_STUB("BdlMain::initOut", 0x0014b3bc);
}

// stub of ROR 0x0014b7fc
void BdlMain::selectOut()
{
    ROSEPAN_STUB("BdlMain::selectOut", 0x0014b7fc);
}

// stub of ROR 0x0014a0dc
void BdsMain::after()
{
    ROSEPAN_STUB("BdsMain::after", 0x0014a0dc);
}

// stub of ROR 0x00149f10
void BdsMain::decide()
{
    ROSEPAN_STUB("BdsMain::decide", 0x00149f10);
}

// stub of ROR 0x0014a034
void BdsMain::exec()
{
    ROSEPAN_STUB("BdsMain::exec", 0x0014a034);
}

// stub of ROR 0x00149d18
void BdsMain::initOut()
{
    ROSEPAN_STUB("BdsMain::initOut", 0x00149d18);
}

// stub of ROR 0x0014a158
void BdsMain::selectOut()
{
    ROSEPAN_STUB("BdsMain::selectOut", 0x0014a158);
}

// stub of ROR 0x0014e16c
void ClrMain::after()
{
    ROSEPAN_STUB("ClrMain::after", 0x0014e16c);
}

// stub of ROR 0x0014deb4
void ClrMain::decide()
{
    ROSEPAN_STUB("ClrMain::decide", 0x0014deb4);
}

// stub of ROR 0x0014e0b4
void ClrMain::exec()
{
    ROSEPAN_STUB("ClrMain::exec", 0x0014e0b4);
}

// stub of ROR 0x0014dc7c
void ClrMain::initOut()
{
    ROSEPAN_STUB("ClrMain::initOut", 0x0014dc7c);
}

// stub of ROR 0x0014e224
void ClrMain::selectOut()
{
    ROSEPAN_STUB("ClrMain::selectOut", 0x0014e224);
}

// stub of ROR 0x0015cd30
void DwfMain::after()
{
    ROSEPAN_STUB("DwfMain::after", 0x0015cd30);
}

// stub of ROR 0x0015c710
void DwfMain::decide()
{
    ROSEPAN_STUB("DwfMain::decide", 0x0015c710);
}

// stub of ROR 0x0015cc84
void DwfMain::exec()
{
    ROSEPAN_STUB("DwfMain::exec", 0x0015cc84);
}

// stub of ROR 0x0015c304
void DwfMain::initOut()
{
    ROSEPAN_STUB("DwfMain::initOut", 0x0015c304);
}

// stub of ROR 0x0015cdac
void DwfMain::selectOut()
{
    ROSEPAN_STUB("DwfMain::selectOut", 0x0015cdac);
}

// stub of ROR 0x00156140
void FshMain::after()
{
    ROSEPAN_STUB("FshMain::after", 0x00156140);
}

// stub of ROR 0x00155db4
void FshMain::decide()
{
    ROSEPAN_STUB("FshMain::decide", 0x00155db4);
}

// stub of ROR 0x00156048
void FshMain::exec()
{
    ROSEPAN_STUB("FshMain::exec", 0x00156048);
}

// stub of ROR 0x00156620
void FshMain::initOut()
{
    ROSEPAN_STUB("FshMain::initOut", 0x00156620);
}

// stub of ROR 0x001561bc
void FshMain::selectOut()
{
    ROSEPAN_STUB("FshMain::selectOut", 0x001561bc);
}

// stub of ROR 0x00158d5c
void GotMain::after()
{
    ROSEPAN_STUB("GotMain::after", 0x00158d5c);
}

// stub of ROR 0x001587f8
void GotMain::decide()
{
    ROSEPAN_STUB("GotMain::decide", 0x001587f8);
}

// stub of ROR 0x00158ca4
void GotMain::exec()
{
    ROSEPAN_STUB("GotMain::exec", 0x00158ca4);
}

// stub of ROR 0x001583d4
void GotMain::initOut()
{
    ROSEPAN_STUB("GotMain::initOut", 0x001583d4);
}

// stub of ROR 0x00158dd8
void GotMain::selectOut()
{
    ROSEPAN_STUB("GotMain::selectOut", 0x00158dd8);
}

// stub of ROR 0x00154270
void GrgMain::after()
{
    ROSEPAN_STUB("GrgMain::after", 0x00154270);
}

// stub of ROR 0x00154090
void GrgMain::decide()
{
    ROSEPAN_STUB("GrgMain::decide", 0x00154090);
}

// stub of ROR 0x00154208
void GrgMain::exec()
{
    ROSEPAN_STUB("GrgMain::exec", 0x00154208);
}

// stub of ROR 0x00153e40
void GrgMain::initOut()
{
    ROSEPAN_STUB("GrgMain::initOut", 0x00153e40);
}

// stub of ROR 0x00154748
void GrgMain::releaseHold()
{
    ROSEPAN_STUB("GrgMain::releaseHold", 0x00154748);
}

// stub of ROR 0x00154340
void GrgMain::selectOut()
{
    ROSEPAN_STUB("GrgMain::selectOut", 0x00154340);
}

// stub of ROR 0x00154734
int GrgMain::testView()
{
    ROSEPAN_STUB("GrgMain::testView", 0x00154734);
    return {};
}

// stub of ROR 0x0014d1c4
void MrtMain::after()
{
    ROSEPAN_STUB("MrtMain::after", 0x0014d1c4);
}

// stub of ROR 0x0014cf58
void MrtMain::decide()
{
    ROSEPAN_STUB("MrtMain::decide", 0x0014cf58);
}

// stub of ROR 0x0014d13c
void MrtMain::exec()
{
    ROSEPAN_STUB("MrtMain::exec", 0x0014d13c);
}

// stub of ROR 0x0014cde4
void MrtMain::initOut()
{
    ROSEPAN_STUB("MrtMain::initOut", 0x0014cde4);
}

// stub of ROR 0x0014d240
void MrtMain::selectOut()
{
    ROSEPAN_STUB("MrtMain::selectOut", 0x0014d240);
}

// stub of ROR 0x001510dc
void NorMain::after()
{
    ROSEPAN_STUB("NorMain::after", 0x001510dc);
}

// stub of ROR 0x00150e58
void NorMain::decide()
{
    ROSEPAN_STUB("NorMain::decide", 0x00150e58);
}

// stub of ROR 0x0015102c
void NorMain::exec()
{
    ROSEPAN_STUB("NorMain::exec", 0x0015102c);
}

// stub of ROR 0x00150b24
void NorMain::initOut(const _tagEnemyParam *prm)
{
    ROSEPAN_STUB("NorMain::initOut", 0x00150b24);
}

// stub of ROR 0x00151158
void NorMain::selectOut()
{
    ROSEPAN_STUB("NorMain::selectOut", 0x00151158);
}

// stub of ROR 0x0015ab48
void PigMain::after()
{
    ROSEPAN_STUB("PigMain::after", 0x0015ab48);
}

// stub of ROR 0x0015a5e0
void PigMain::decide()
{
    ROSEPAN_STUB("PigMain::decide", 0x0015a5e0);
}

// stub of ROR 0x0015aa8c
void PigMain::exec()
{
    ROSEPAN_STUB("PigMain::exec", 0x0015aa8c);
}

// stub of ROR 0x0015a17c
void PigMain::initOut()
{
    ROSEPAN_STUB("PigMain::initOut", 0x0015a17c);
}

// stub of ROR 0x0015abc4
void PigMain::selectOut()
{
    ROSEPAN_STUB("PigMain::selectOut", 0x0015abc4);
}

// stub of ROR 0x00202f10
void TskEnemyMove::stop()
{
    ROSEPAN_STUB("TskEnemyMove::stop", 0x00202f10);
}

// stub of ROR 0x00166b74
void EmyOut::getBasePos(float *pos)
{
    ROSEPAN_STUB("EmyOut::getBasePos", 0x00166b74);
}

// stub of ROR 0x00166da4
void EmyOut::selectTarget(int rate)
{
    ROSEPAN_STUB("EmyOut::selectTarget", 0x00166da4);
}

// stub of ROR 0x001680a0
void EmyOut::setMotion(int motion, int flags)
{
    ROSEPAN_STUB("EmyOut::setMotion", 0x001680a0);
}

// stub of ROR 0x001682b0
void EmyOut::setWalkMotion()
{
    ROSEPAN_STUB("EmyOut::setWalkMotion", 0x001682b0);
}

// stub of ROR 0x00166ad0
void EmyOut::termCommon()
{
    ROSEPAN_STUB("EmyOut::termCommon", 0x00166ad0);
}

// stub of ROR 0x0016968c
int EmyStateMap::next(int input)
{
    ROSEPAN_STUB("EmyStateMap::next", 0x0016968c);
    return {};
}

// stub of ROR 0x00169810
int emyAttachItem(u_int oid, u_int item)
{
    ROSEPAN_STUB("emyAttachItem", 0x00169810);
    return {};
}

// stub of ROR 0x0016976c
int emyCanReach(const float *from, const float *to)
{
    ROSEPAN_STUB("emyCanReach", 0x0016976c);
    return {};
}

// stub of ROR 0x00169bac
int emyChkInSight(float range, float angle, int useAngle, u_int target,
                   ACHRstat *achr)
{
    ROSEPAN_STUB("emyChkInSight", 0x00169bac);
    return {};
}

// stub of ROR 0x001670dc
void emyStartMove(int mode, EmyOut *out, const float *pos)
{
    ROSEPAN_STUB("emyStartMove", 0x001670dc);
}

// stub of ROR 0x001044b8
float utlRandRangeF(float lo, float hi)
{
    ROSEPAN_STUB("utlRandRangeF", 0x001044b8);
    return {};
}

// stub of ROR 0x0017d298
int FUN_0017d298(MagSectaList *list, MagSectaPathReq *req)
{
    ROSEPAN_STUB("FUN_0017d298", 0x0017d298);
    return {};
}

// stub of ROR 0x0017dd5c
float FUN_0017dd5c(MagSectaList *list, int id, const float *pos, float h)
{
    ROSEPAN_STUB("FUN_0017dd5c", 0x0017dd5c);
    return {};
}

// stub of ROR 0x0017ded0
int FUN_0017ded0(MagSectaList *list, int id, const float *pos, float r)
{
    ROSEPAN_STUB("FUN_0017ded0", 0x0017ded0);
    return {};
}

// stub of ROR 0x001694cc
int EmyOut::playSE(int no)
{
    ROSEPAN_STUB("EmyOut::playSE", 0x001694cc);
    return {};
}

// stub of ROR 0x0016947c
void EmyOut::stopVoice()
{
    ROSEPAN_STUB("EmyOut::stopVoice", 0x0016947c);
}

// stub of ROR 0x001693ec
int EmyOut::tickVoice(int lo, int hi, int se)
{
    ROSEPAN_STUB("EmyOut::tickVoice", 0x001693ec);
    return {};
}

// stub of ROR 0x00169f94
int emyChkHitReach(float range, u_int oid, int nodes, u_int target)
{
    ROSEPAN_STUB("emyChkHitReach", 0x00169f94);
    return {};
}

// stub of ROR 0x00166f50
void emyStartChase(float range, EmyOut *out, u_int target)
{
    ROSEPAN_STUB("emyStartChase", 0x00166f50);
}

// stub of ROR 0x00169a14
float emyVecLength(const float *v)
{
    ROSEPAN_STUB("emyVecLength", 0x00169a14);
    return {};
}


// stub of ROR 0x0021fe68
int i3rDrawModel(I3dLayer *layer, I3rRoot *root, sceVu0FMATRIX *nodes, sceVu0FVECTOR *world,
                   const void *opt)
{
    ROSEPAN_STUB("i3rDrawModel", 0x0021fe68);
    return {};
}
