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
#include "mag/maglifepc.h"
#include "eff/eff0.h"
#include "eff/effclasses.h"

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

// stub of ROR 0x00215b5c
void dbgTraceVector3(float x, float y, float z)
{
    ROSEPAN_STUB("dbgTraceVector3", 0x00215b5c);
}


// stub of ROR 0x00215bb4
void dbgTraceVector4(const float *v)
{
    ROSEPAN_STUB("dbgTraceVector4", 0x00215bb4);
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


// stub of ROR 0x00234050
float i3dSegSegDist2(const float *p0, const float *d0, const float *p1, const float *d1,
                     float *s, float *t)
{
    ROSEPAN_STUB("i3dSegSegDist2", 0x00234050);
    return {};
}

// stub of ROR 0x002337e0
float i3dPointSegDist2(const float *p, const float *a, const float *d, float *t)
{
    ROSEPAN_STUB("i3dPointSegDist2", 0x002337e0);
    return {};
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


// stub of ROR 0x00130de0
void MagLife130da4::setVolume(int volume)
{
    ROSEPAN_STUB("MagLife130da4::setVolume", 0x00130de0);
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

// stub of ROR 0x001832e8
const char * msgGetSysText(int no)
{
    ROSEPAN_STUB("msgGetSysText", 0x001832e8);
    /* PC: "" not NULL -- MnuPBook::setPageText strlen()s it. The ROM reads
     * 0x1d4 string pointers at data 0x298fe8 (datadump.py 298fe8 468 S:text). */
    return "";
}

// stub of ROR 0x00217f10
float utlAtan2(float x, float z)
{
    ROSEPAN_STUB("utlAtan2", 0x00217f10);
    return {};
}

// stub of ROR 0x00172678
void gamCreateEff1a(void)
{
    ROSEPAN_STUB("gamCreateEff1a", 0x00172678);
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



// stub of ROR 0x002251f0
void i3rTrackGetValue(const I3rItem *track, float *out, float t)
{
    ROSEPAN_STUB("i3rTrackGetValue", 0x002251f0);
}

// stub of ROR 0x0011d044
int MagLifePC::canReload()
{
    ROSEPAN_STUB("MagLifePC::canReload", 0x0011d044);
    return {};
}

// stub of ROR 0x0011c520
int MagLifePC::getEquipItem()
{
    ROSEPAN_STUB("MagLifePC::getEquipItem", 0x0011c520);
    return {};
}

// stub of ROR 0x0011ce50
int MagLifePC::getEquipItemChrNo()
{
    ROSEPAN_STUB("MagLifePC::getEquipItemChrNo", 0x0011ce50);
    return {};
}

// stub of ROR 0x0011c464
int MagLifePC::getPcParam1b()
{
    ROSEPAN_STUB("MagLifePC::getPcParam1b", 0x0011c464);
    return {};
}

// stub of ROR 0x0011c4c0
int MagLifePC::getPcParam1c()
{
    ROSEPAN_STUB("MagLifePC::getPcParam1c", 0x0011c4c0);
    return {};
}

// stub of ROR 0x0011cb70
int MagLifePC::getWeaponType(int item)
{
    ROSEPAN_STUB("MagLifePC::getWeaponType", 0x0011cb70);
    return {};
}

// stub of ROR 0x0012ce34
void MagLifePC::message17()
{
    ROSEPAN_STUB("MagLifePC::message17", 0x0012ce34);
}

// stub of ROR 0x0012c9c0
void MagLifePC::setState14(int arg, int word)
{
    ROSEPAN_STUB("MagLifePC::setState14", 0x0012c9c0);
}

// stub of ROR 0x0012bedc
float MagLifePC::calcTargetAngle(float *roty, float *rotx, const float *pos)
{
    ROSEPAN_STUB("MagLifePC::calcTargetAngle", 0x0012bedc);
    return {};
}

// stub of ROR 0x0011cbf4
int MagLifePC::canTakeItem(u_int oid)
{
    ROSEPAN_STUB("MagLifePC::canTakeItem", 0x0011cbf4);
    return {};
}

// stub of ROR 0x0012ca28
int MagLifePC::getAttackHeight()
{
    ROSEPAN_STUB("MagLifePC::getAttackHeight", 0x0012ca28);
    return {};
}

// stub of ROR 0x0012c014
void MagLifePC::getHitFrameRange(float *from, float *to)
{
    ROSEPAN_STUB("MagLifePC::getHitFrameRange", 0x0012c014);
}

// stub of ROR 0x0011c974
void MagLifePC::hitTarget(u_int oid, int combo, int kind)
{
    ROSEPAN_STUB("MagLifePC::hitTarget", 0x0011c974);
}

// stub of ROR 0x0011d06c
void MagLifePC::reload()
{
    ROSEPAN_STUB("MagLifePC::reload", 0x0011d06c);
}

// stub of ROR 0x0011cc14
int MagLifePC::takeItem(u_int oid)
{
    ROSEPAN_STUB("MagLifePC::takeItem", 0x0011cc14);
    return {};
}

// stub of ROR 0x0011cd20
void MagLifePC::slideOffObj(float *out, const float *delta, u_int oid)
{
    ROSEPAN_STUB("MagLifePC::slideOffObj", 0x0011cd20);
}

// stub of ROR 0x0011ccd4
int MagLifePC::testObjHit(const float *pos, u_int *oid)
{
    ROSEPAN_STUB("MagLifePC::testObjHit", 0x0011ccd4);
    return {};
}

// ---- Eff constructors / destructors (hand stubs: stubgen leaves them out; eff/effclasses.h) ----

// stub of ROR 0x0016b62c
EffBloodSpray1::EffBloodSpray1()
{
    ROSEPAN_STUB("EffBloodSpray1::EffBloodSpray1", 0x0016b62c);
}

// stub of ROR 0x0016be14
EffBloodSpray2::EffBloodSpray2()
{
    ROSEPAN_STUB("EffBloodSpray2::EffBloodSpray2", 0x0016be14);
}

// stub of ROR 0x0016d804
EffBloodStay3::~EffBloodStay3()
{
    ROSEPAN_STUB("EffBloodStay3::~EffBloodStay3", 0x0016d804);
}

// stub of ROR 0x00172ac8
EffFog3::~EffFog3()
{
    ROSEPAN_STUB("EffFog3::~EffFog3", 0x00172ac8);
}

// stub of ROR 0x00173df8
EffGeneric1::EffGeneric1()
{
    ROSEPAN_STUB("EffGeneric1::EffGeneric1", 0x00173df8);
}

// stub of ROR 0x00173e48
EffGeneric1::~EffGeneric1()
{
    ROSEPAN_STUB("EffGeneric1::~EffGeneric1", 0x00173e48);
}

// stub of ROR 0x0016afcc
EffBg1::EffBg1()
{
    ROSEPAN_STUB("EffBg1::EffBg1", 0x0016afcc);
}

// stub of ROR 0x0016affc
EffBg1::~EffBg1()
{
    ROSEPAN_STUB("EffBg1::~EffBg1", 0x0016affc);
}

// stub of ROR 0x0017562c
EffLight3::EffLight3()
{
    ROSEPAN_STUB("EffLight3::EffLight3", 0x0017562c);
}

// stub of ROR 0x00175664
EffLight3::~EffLight3()
{
    ROSEPAN_STUB("EffLight3::~EffLight3", 0x00175664);
}

// stub of ROR 0x001790e4
EffReload1::EffReload1()
{
    ROSEPAN_STUB("EffReload1::EffReload1", 0x001790e4);
}

// stub of ROR 0x0017911c
EffReload1::~EffReload1()
{
    ROSEPAN_STUB("EffReload1::~EffReload1", 0x0017911c);
}

// stub of ROR 0x0017a918
EffWater1::EffWater1()
{
    ROSEPAN_STUB("EffWater1::EffWater1", 0x0017a918);
}

// stub of ROR 0x0017a94c
EffWater1::~EffWater1()
{
    ROSEPAN_STUB("EffWater1::~EffWater1", 0x0017a94c);
}

// stub of ROR 0x0017afd8
EffWater3::EffWater3()
{
    ROSEPAN_STUB("EffWater3::EffWater3", 0x0017afd8);
}

// stub of ROR 0x0017b010
EffWater3::~EffWater3()
{
    ROSEPAN_STUB("EffWater3::~EffWater3", 0x0017b010);
}

// stub of ROR 0x0016b060
int EffBg1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffBg1::init", 0x0016b060);
    return {};
}

// stub of ROR 0x0016b348
int EffBg1::liveMain()
{
    ROSEPAN_STUB("EffBg1::liveMain", 0x0016b348);
    return {};
}

// stub of ROR 0x0028ae70
u_long * EffBg1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffBg1::renderMain", 0x0028ae70);
    return {};
}

// stub of ROR 0x0016b444
void EffBg1::renderPreMain(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffBg1::renderPreMain", 0x0016b444);
}

// stub of ROR 0x0028b218
I3dDrawSet * EffBloodStay3::createPacketEntry(I3dLayer *layer)
{
    ROSEPAN_STUB("EffBloodStay3::createPacketEntry", 0x0028b218);
    return {};
}

// stub of ROR 0x0016d870
int EffBloodStay3::init(const EffParam *param)
{
    ROSEPAN_STUB("EffBloodStay3::init", 0x0016d870);
    return {};
}

// stub of ROR 0x0016d8ac
int EffBloodStay3::liveMain()
{
    ROSEPAN_STUB("EffBloodStay3::liveMain", 0x0016d8ac);
    return {};
}

// stub of ROR 0x0028b230
void EffBloodStay3::render(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffBloodStay3::render", 0x0028b230);
}

// stub of ROR 0x0028b238
u_long * EffBloodStay3::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffBloodStay3::renderMain", 0x0028b238);
    return {};
}

// stub of ROR 0x0028c050
I3dDrawSet * EffFog3::createPacketEntry(I3dLayer *layer)
{
    ROSEPAN_STUB("EffFog3::createPacketEntry", 0x0028c050);
    return {};
}

// stub of ROR 0x00172b30
int EffFog3::init(const EffParam *param)
{
    ROSEPAN_STUB("EffFog3::init", 0x00172b30);
    return {};
}

// stub of ROR 0x00172cb8
int EffFog3::liveMain()
{
    ROSEPAN_STUB("EffFog3::liveMain", 0x00172cb8);
    return {};
}

// stub of ROR 0x0028c058
void EffFog3::render(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffFog3::render", 0x0028c058);
}

// stub of ROR 0x0028c060
u_long * EffFog3::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffFog3::renderMain", 0x0028c060);
    return {};
}

// stub of ROR 0x00173f3c
int EffGeneric1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffGeneric1::init", 0x00173f3c);
    return {};
}

// stub of ROR 0x00174438
int EffGeneric1::liveMain()
{
    ROSEPAN_STUB("EffGeneric1::liveMain", 0x00174438);
    return {};
}

// stub of ROR 0x0028c648
u_long * EffGeneric1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffGeneric1::renderMain", 0x0028c648);
    return {};
}

// stub of ROR 0x00174534
void EffGeneric1::renderPreMain(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffGeneric1::renderPreMain", 0x00174534);
}

// stub of ROR 0x0028c860
I3dDrawSet * EffLight3::createPacketEntry(I3dLayer *layer)
{
    ROSEPAN_STUB("EffLight3::createPacketEntry", 0x0028c860);
    return {};
}

// stub of ROR 0x00175804
int EffLight3::init(const EffParam *param)
{
    ROSEPAN_STUB("EffLight3::init", 0x00175804);
    return {};
}

// stub of ROR 0x00175dc4
int EffLight3::liveMain()
{
    ROSEPAN_STUB("EffLight3::liveMain", 0x00175dc4);
    return {};
}

// stub of ROR 0x0028c870
u_long * EffLight3::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffLight3::renderMain", 0x0028c870);
    return {};
}

// stub of ROR 0x0028c868
void EffLight3::renderPreMain(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffLight3::renderPreMain", 0x0028c868);
}

// stub of ROR 0x001791ac
int EffReload1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffReload1::init", 0x001791ac);
    return {};
}

// stub of ROR 0x00179324
int EffReload1::liveMain()
{
    ROSEPAN_STUB("EffReload1::liveMain", 0x00179324);
    return {};
}

// stub of ROR 0x0028ca88
u_long * EffReload1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffReload1::renderMain", 0x0028ca88);
    return {};
}

// stub of ROR 0x00179390
void EffReload1::renderPreMain(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffReload1::renderPreMain", 0x00179390);
}

// stub of ROR 0x0017a9c8
int EffWater1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffWater1::init", 0x0017a9c8);
    return {};
}

// stub of ROR 0x0017adc0
void EffWater1::liveActivateExec()
{
    ROSEPAN_STUB("EffWater1::liveActivateExec", 0x0017adc0);
}

// stub of ROR 0x0017ac5c
int EffWater1::liveMain()
{
    ROSEPAN_STUB("EffWater1::liveMain", 0x0017ac5c);
    return {};
}

// stub of ROR 0x0028cf70
u_long * EffWater1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffWater1::renderMain", 0x0028cf70);
    return {};
}

// stub of ROR 0x0017ae04
void EffWater1::renderPreMain(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffWater1::renderPreMain", 0x0017ae04);
}

// stub of ROR 0x0017b0a4
int EffWater3::init(const EffParam *param)
{
    ROSEPAN_STUB("EffWater3::init", 0x0017b0a4);
    return {};
}

// stub of ROR 0x0017b5e0
int EffWater3::liveMain()
{
    ROSEPAN_STUB("EffWater3::liveMain", 0x0017b5e0);
    return {};
}

// stub of ROR 0x0017b6d4
u_long * EffWater3::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffWater3::renderMain", 0x0017b6d4);
    return {};
}

// stub of ROR 0x0016ff18
float effCalcViewDepth(DogCamera *cam, const float *pos)
{
    ROSEPAN_STUB("effCalcViewDepth", 0x0016ff18);
    return {};
}

// stub of ROR 0x0016ff78
void effMoveToward(float d, float *out, const float *from, const float *to)
{
    ROSEPAN_STUB("effMoveToward", 0x0016ff78);
}

// stub of ROR 0x0016b668
int EffBloodSpray1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffBloodSpray1::init", 0x0016b668);
    return {};
}

// stub of ROR 0x0016b978
int EffBloodSpray1::liveMain()
{
    ROSEPAN_STUB("EffBloodSpray1::liveMain", 0x0016b978);
    return {};
}

// stub of ROR 0x0016ba7c
u_long * EffBloodSpray1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffBloodSpray1::renderMain", 0x0016ba7c);
    return {};
}

// stub of ROR 0x0016bdd4
void EffBloodSpray1::updateZDepth()
{
    ROSEPAN_STUB("EffBloodSpray1::updateZDepth", 0x0016bdd4);
}

// stub of ROR 0x0016be70
int EffBloodSpray2::init(const EffParam *param)
{
    ROSEPAN_STUB("EffBloodSpray2::init", 0x0016be70);
    return {};
}

// stub of ROR 0x0016c258
int EffBloodSpray2::liveMain()
{
    ROSEPAN_STUB("EffBloodSpray2::liveMain", 0x0016c258);
    return {};
}

// stub of ROR 0x0016c400
u_long * EffBloodSpray2::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffBloodSpray2::renderMain", 0x0016c400);
    return {};
}

// stub of ROR 0x0016c748
void EffBloodSpray2::updateZDepth()
{
    ROSEPAN_STUB("EffBloodSpray2::updateZDepth", 0x0016c748);
}

// stub of ROR 0x0016c7c0
int EffBloodStay1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffBloodStay1::init", 0x0016c7c0);
    return {};
}

// stub of ROR 0x0016ca00
int EffBloodStay1::liveMain()
{
    ROSEPAN_STUB("EffBloodStay1::liveMain", 0x0016ca00);
    return {};
}

// stub of ROR 0x0016cac0
u_long * EffBloodStay1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffBloodStay1::renderMain", 0x0016cac0);
    return {};
}

// stub of ROR 0x0016d01c
int EffBloodStay2::init(const EffParam *param)
{
    ROSEPAN_STUB("EffBloodStay2::init", 0x0016d01c);
    return {};
}

// stub of ROR 0x0016d0d0
int EffBloodStay2::liveMain()
{
    ROSEPAN_STUB("EffBloodStay2::liveMain", 0x0016d0d0);
    return {};
}

// stub of ROR 0x0016d4bc
u_long * EffBloodStay2::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffBloodStay2::renderMain", 0x0016d4bc);
    return {};
}

// stub of ROR 0x0016d9a8
int EffBloodStay3Prim::init(const EffParam *param)
{
    ROSEPAN_STUB("EffBloodStay3Prim::init", 0x0016d9a8);
    return {};
}

// stub of ROR 0x0016da30
int EffBloodStay3Prim::liveMain()
{
    ROSEPAN_STUB("EffBloodStay3Prim::liveMain", 0x0016da30);
    return {};
}

// stub of ROR 0x0016da44
u_long * EffBloodStay3Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffBloodStay3Prim::renderMain", 0x0016da44);
    return {};
}

// stub of ROR 0x0016dcc8
int EffBreath1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffBreath1::init", 0x0016dcc8);
    return {};
}

// stub of ROR 0x0016de40
int EffBreath1::liveMain()
{
    ROSEPAN_STUB("EffBreath1::liveMain", 0x0016de40);
    return {};
}

// stub of ROR 0x0016df38
u_long * EffBreath1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffBreath1::renderMain", 0x0016df38);
    return {};
}

// stub of ROR 0x0016e3ec
void EffBreath1::updateZDepth()
{
    ROSEPAN_STUB("EffBreath1::updateZDepth", 0x0016e3ec);
}

// stub of ROR 0x0016e6e4
int EffBreath1Space::init(const EffParam *param)
{
    ROSEPAN_STUB("EffBreath1Space::init", 0x0016e6e4);
    return {};
}

// stub of ROR 0x0016e7cc
int EffBreath1Space::liveMain()
{
    ROSEPAN_STUB("EffBreath1Space::liveMain", 0x0016e7cc);
    return {};
}

// stub of ROR 0x0028b408
void EffBreath1Space::render(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffBreath1Space::render", 0x0028b408);
}

// stub of ROR 0x0028b410
u_long * EffBreath1Space::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffBreath1Space::renderMain", 0x0028b410);
    return {};
}

// stub of ROR 0x0028b400
I3dDrawSet * EffBreath1Space::createPacketEntry(I3dLayer *layer)
{
    ROSEPAN_STUB("EffBreath1Space::createPacketEntry", 0x0028b400);
    return {};
}

// stub of ROR 0x0016eb00
int EffBreath2::init(const EffParam *param)
{
    ROSEPAN_STUB("EffBreath2::init", 0x0016eb00);
    return {};
}

// stub of ROR 0x0016eb98
int EffBreath2::liveMain()
{
    ROSEPAN_STUB("EffBreath2::liveMain", 0x0016eb98);
    return {};
}

// stub of ROR 0x0028b5d8
void EffBreath2::render(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffBreath2::render", 0x0028b5d8);
}

// stub of ROR 0x0028b5e0
u_long * EffBreath2::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffBreath2::renderMain", 0x0028b5e0);
    return {};
}

// stub of ROR 0x0016edc0
int EffBreath2Prim::init(const EffParam *param)
{
    ROSEPAN_STUB("EffBreath2Prim::init", 0x0016edc0);
    return {};
}

// stub of ROR 0x0016eeb4
int EffBreath2Prim::liveMain()
{
    ROSEPAN_STUB("EffBreath2Prim::liveMain", 0x0016eeb4);
    return {};
}

// stub of ROR 0x0016ef74
u_long * EffBreath2Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffBreath2Prim::renderMain", 0x0016ef74);
    return {};
}

// stub of ROR 0x0016f160
void EffBreath2Prim::updateZDepth()
{
    ROSEPAN_STUB("EffBreath2Prim::updateZDepth", 0x0016f160);
}

// stub of ROR 0x0016f1a0
int EffBreath2Ground::init(const EffParam *param)
{
    ROSEPAN_STUB("EffBreath2Ground::init", 0x0016f1a0);
    return {};
}

// stub of ROR 0x0016f380
int EffBreath2Ground::liveMain()
{
    ROSEPAN_STUB("EffBreath2Ground::liveMain", 0x0016f380);
    return {};
}

// stub of ROR 0x0016f3c8
u_long * EffBreath2Ground::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffBreath2Ground::renderMain", 0x0016f3c8);
    return {};
}

// stub of ROR 0x0016f71c
int EffCameraAnim1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffCameraAnim1::init", 0x0016f71c);
    return {};
}

// stub of ROR 0x0016faa0
int EffCameraAnim1::liveMain()
{
    ROSEPAN_STUB("EffCameraAnim1::liveMain", 0x0016faa0);
    return {};
}

// stub of ROR 0x0028b890
void EffCameraAnim1::render(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffCameraAnim1::render", 0x0028b890);
}

// stub of ROR 0x0028b898
u_long * EffCameraAnim1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffCameraAnim1::renderMain", 0x0028b898);
    return {};
}

// stub of ROR 0x0028b888
I3dDrawSet * EffCameraAnim1::createPacketEntry(I3dLayer *layer)
{
    ROSEPAN_STUB("EffCameraAnim1::createPacketEntry", 0x0028b888);
    return {};
}

// stub of ROR 0x001704a4
int EffDust2::init(const EffParam *param)
{
    ROSEPAN_STUB("EffDust2::init", 0x001704a4);
    return {};
}

// stub of ROR 0x00170594
int EffDust2::liveMain()
{
    ROSEPAN_STUB("EffDust2::liveMain", 0x00170594);
    return {};
}

// stub of ROR 0x0028b988
void EffDust2::render(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffDust2::render", 0x0028b988);
}

// stub of ROR 0x0028b990
u_long * EffDust2::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffDust2::renderMain", 0x0028b990);
    return {};
}

// stub of ROR 0x0028b980
I3dDrawSet * EffDust2::createPacketEntry(I3dLayer *layer)
{
    ROSEPAN_STUB("EffDust2::createPacketEntry", 0x0028b980);
    return {};
}

// stub of ROR 0x0017093c
int EffDust2Prim::init(const EffParam *param)
{
    ROSEPAN_STUB("EffDust2Prim::init", 0x0017093c);
    return {};
}

// stub of ROR 0x001709c8
int EffDust2Prim::liveMain()
{
    ROSEPAN_STUB("EffDust2Prim::liveMain", 0x001709c8);
    return {};
}

// stub of ROR 0x001709f4
u_long * EffDust2Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffDust2Prim::renderMain", 0x001709f4);
    return {};
}

// stub of ROR 0x00170c64
void EffDust2Prim::updateZDepth()
{
    ROSEPAN_STUB("EffDust2Prim::updateZDepth", 0x00170c64);
}

// stub of ROR 0x00170ca4
int EffFire1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffFire1::init", 0x00170ca4);
    return {};
}

// stub of ROR 0x00170d44
int EffFire1::liveMain()
{
    ROSEPAN_STUB("EffFire1::liveMain", 0x00170d44);
    return {};
}

// stub of ROR 0x00170e14
u_long * EffFire1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffFire1::renderMain", 0x00170e14);
    return {};
}

// stub of ROR 0x00171278
void EffFire1::liveActivateExec()
{
    ROSEPAN_STUB("EffFire1::liveActivateExec", 0x00171278);
}

// stub of ROR 0x001711fc
void EffFire1::updateZDepth()
{
    ROSEPAN_STUB("EffFire1::updateZDepth", 0x001711fc);
}

// stub of ROR 0x001712bc
int EffFire1Ex::init(const EffParam *param)
{
    ROSEPAN_STUB("EffFire1Ex::init", 0x001712bc);
    return {};
}

// stub of ROR 0x00171378
void EffFire1Ex::updateZDepth()
{
    ROSEPAN_STUB("EffFire1Ex::updateZDepth", 0x00171378);
}

// stub of ROR 0x0017145c
int EffFire2::init(const EffParam *param)
{
    ROSEPAN_STUB("EffFire2::init", 0x0017145c);
    return {};
}

// stub of ROR 0x00171534
int EffFire2::liveMain()
{
    ROSEPAN_STUB("EffFire2::liveMain", 0x00171534);
    return {};
}

// stub of ROR 0x00171548
u_long * EffFire2::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffFire2::renderMain", 0x00171548);
    return {};
}

// stub of ROR 0x0017167c
void EffFire2::updateZDepth()
{
    ROSEPAN_STUB("EffFire2::updateZDepth", 0x0017167c);
}

// stub of ROR 0x001716bc
int EffFog1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffFog1::init", 0x001716bc);
    return {};
}

// stub of ROR 0x001717b8
int EffFog1::liveMain()
{
    ROSEPAN_STUB("EffFog1::liveMain", 0x001717b8);
    return {};
}

// stub of ROR 0x001717f4
u_long * EffFog1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffFog1::renderMain", 0x001717f4);
    return {};
}

// stub of ROR 0x00171bbc
void EffFog1::updateZDepth()
{
    ROSEPAN_STUB("EffFog1::updateZDepth", 0x00171bbc);
}

// stub of ROR 0x00171bfc
int EffFog2::init(const EffParam *param)
{
    ROSEPAN_STUB("EffFog2::init", 0x00171bfc);
    return {};
}

// stub of ROR 0x00171c94
int EffFog2::liveMain()
{
    ROSEPAN_STUB("EffFog2::liveMain", 0x00171c94);
    return {};
}

// stub of ROR 0x0028bee0
void EffFog2::render(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffFog2::render", 0x0028bee0);
}

// stub of ROR 0x0028bee8
u_long * EffFog2::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffFog2::renderMain", 0x0028bee8);
    return {};
}

// stub of ROR 0x0028bed8
I3dDrawSet * EffFog2::createPacketEntry(I3dLayer *layer)
{
    ROSEPAN_STUB("EffFog2::createPacketEntry", 0x0028bed8);
    return {};
}

// stub of ROR 0x00171e28
int EffFog2Prim::init(const EffParam *param)
{
    ROSEPAN_STUB("EffFog2Prim::init", 0x00171e28);
    return {};
}

// stub of ROR 0x00172058
int EffFog2Prim::liveMain()
{
    ROSEPAN_STUB("EffFog2Prim::liveMain", 0x00172058);
    return {};
}

// stub of ROR 0x00172284
u_long * EffFog2Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffFog2Prim::renderMain", 0x00172284);
    return {};
}

// stub of ROR 0x00172638
void EffFog2Prim::updateZDepth()
{
    ROSEPAN_STUB("EffFog2Prim::updateZDepth", 0x00172638);
}

// stub of ROR 0x00172d98
int EffFog3Dark::init(const EffParam *param)
{
    ROSEPAN_STUB("EffFog3Dark::init", 0x00172d98);
    return {};
}

// stub of ROR 0x0028c148
int EffFog3Dark::liveMain()
{
    ROSEPAN_STUB("EffFog3Dark::liveMain", 0x0028c148);
    return {};
}

// stub of ROR 0x0028c150
void EffFog3Dark::render(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffFog3Dark::render", 0x0028c150);
}

// stub of ROR 0x0028c158
u_long * EffFog3Dark::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffFog3Dark::renderMain", 0x0028c158);
    return {};
}

// stub of ROR 0x00172de8
int EffFog3WeakUp::init(const EffParam *param)
{
    ROSEPAN_STUB("EffFog3WeakUp::init", 0x00172de8);
    return {};
}

// stub of ROR 0x00172e08
int EffFog3WeakUp::liveMain()
{
    ROSEPAN_STUB("EffFog3WeakUp::liveMain", 0x00172e08);
    return {};
}

// stub of ROR 0x0028c240
void EffFog3WeakUp::render(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffFog3WeakUp::render", 0x0028c240);
}

// stub of ROR 0x0028c248
u_long * EffFog3WeakUp::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffFog3WeakUp::renderMain", 0x0028c248);
    return {};
}

// stub of ROR 0x00172e4c
int EffFoot1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffFoot1::init", 0x00172e4c);
    return {};
}

// stub of ROR 0x00172f4c
int EffFoot1::liveMain()
{
    ROSEPAN_STUB("EffFoot1::liveMain", 0x00172f4c);
    return {};
}

// stub of ROR 0x0028c330
void EffFoot1::render(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffFoot1::render", 0x0028c330);
}

// stub of ROR 0x0028c338
u_long * EffFoot1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffFoot1::renderMain", 0x0028c338);
    return {};
}

// stub of ROR 0x001731f0
int EffFoot1Prim::init(const EffParam *param)
{
    ROSEPAN_STUB("EffFoot1Prim::init", 0x001731f0);
    return {};
}

// stub of ROR 0x00173274
int EffFoot1Prim::liveMain()
{
    ROSEPAN_STUB("EffFoot1Prim::liveMain", 0x00173274);
    return {};
}

// stub of ROR 0x001732d8
u_long * EffFoot1Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffFoot1Prim::renderMain", 0x001732d8);
    return {};
}

// stub of ROR 0x00173340
void EffFoot1Prim::updateZDepth()
{
    ROSEPAN_STUB("EffFoot1Prim::updateZDepth", 0x00173340);
}

// stub of ROR 0x00173630
int EffFoot2::init(const EffParam *param)
{
    ROSEPAN_STUB("EffFoot2::init", 0x00173630);
    return {};
}

// stub of ROR 0x00173688
int EffFoot2::liveMain()
{
    ROSEPAN_STUB("EffFoot2::liveMain", 0x00173688);
    return {};
}

// stub of ROR 0x0028c508
void EffFoot2::render(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffFoot2::render", 0x0028c508);
}

// stub of ROR 0x0028c510
u_long * EffFoot2::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffFoot2::renderMain", 0x0028c510);
    return {};
}

// stub of ROR 0x0028c500
I3dDrawSet * EffFoot2::createPacketEntry(I3dLayer *layer)
{
    ROSEPAN_STUB("EffFoot2::createPacketEntry", 0x0028c500);
    return {};
}

// stub of ROR 0x00173a90
int EffFoot2Prim::init(const EffParam *param)
{
    ROSEPAN_STUB("EffFoot2Prim::init", 0x00173a90);
    return {};
}

// stub of ROR 0x00173b2c
int EffFoot2Prim::liveMain()
{
    ROSEPAN_STUB("EffFoot2Prim::liveMain", 0x00173b2c);
    return {};
}

// stub of ROR 0x00173b68
u_long * EffFoot2Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffFoot2Prim::renderMain", 0x00173b68);
    return {};
}

// stub of ROR 0x00174dd8
int EffLight1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffLight1::init", 0x00174dd8);
    return {};
}

// stub of ROR 0x00174e9c
int EffLight1::liveMain()
{
    ROSEPAN_STUB("EffLight1::liveMain", 0x00174e9c);
    return {};
}

// stub of ROR 0x00174eb0
u_long * EffLight1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffLight1::renderMain", 0x00174eb0);
    return {};
}

// stub of ROR 0x0017512c
void EffLight1::liveActivateExec()
{
    ROSEPAN_STUB("EffLight1::liveActivateExec", 0x0017512c);
}

// stub of ROR 0x00175170
int EffLight2::init(const EffParam *param)
{
    ROSEPAN_STUB("EffLight2::init", 0x00175170);
    return {};
}

// stub of ROR 0x00175254
int EffLight2::liveMain()
{
    ROSEPAN_STUB("EffLight2::liveMain", 0x00175254);
    return {};
}

// stub of ROR 0x00175268
void EffLight2::render(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffLight2::render", 0x00175268);
}

// stub of ROR 0x00175378
u_long * EffLight2::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffLight2::renderMain", 0x00175378);
    return {};
}

// stub of ROR 0x001755e8
void EffLight2::liveActivateExec()
{
    ROSEPAN_STUB("EffLight2::liveActivateExec", 0x001755e8);
}

// stub of ROR 0x001788ec
int EffMuzzleFlash1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffMuzzleFlash1::init", 0x001788ec);
    return {};
}

// stub of ROR 0x00178ac0
int EffMuzzleFlash1::liveMain()
{
    ROSEPAN_STUB("EffMuzzleFlash1::liveMain", 0x00178ac0);
    return {};
}

// stub of ROR 0x00178c84
u_long * EffMuzzleFlash1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffMuzzleFlash1::renderMain", 0x00178c84);
    return {};
}

// stub of ROR 0x001790a4
void EffMuzzleFlash1::updateZDepth()
{
    ROSEPAN_STUB("EffMuzzleFlash1::updateZDepth", 0x001790a4);
}

// stub of ROR 0x00179428
int EffSteam1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffSteam1::init", 0x00179428);
    return {};
}

// stub of ROR 0x0017955c
int EffSteam1::liveMain()
{
    ROSEPAN_STUB("EffSteam1::liveMain", 0x0017955c);
    return {};
}

// stub of ROR 0x0028cb78
void EffSteam1::render(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffSteam1::render", 0x0028cb78);
}

// stub of ROR 0x0028cb80
u_long * EffSteam1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffSteam1::renderMain", 0x0028cb80);
    return {};
}

// stub of ROR 0x0028cb70
I3dDrawSet * EffSteam1::createPacketEntry(I3dLayer *layer)
{
    ROSEPAN_STUB("EffSteam1::createPacketEntry", 0x0028cb70);
    return {};
}

// stub of ROR 0x001797d8
int EffSteam1Prim::init(const EffParam *param)
{
    ROSEPAN_STUB("EffSteam1Prim::init", 0x001797d8);
    return {};
}

// stub of ROR 0x001798d8
int EffSteam1Prim::liveMain()
{
    ROSEPAN_STUB("EffSteam1Prim::liveMain", 0x001798d8);
    return {};
}

// stub of ROR 0x00179998
u_long * EffSteam1Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffSteam1Prim::renderMain", 0x00179998);
    return {};
}

// stub of ROR 0x00179b5c
void EffSteam1Prim::updateZDepth()
{
    ROSEPAN_STUB("EffSteam1Prim::updateZDepth", 0x00179b5c);
}

// stub of ROR 0x00179b9c
int EffSteam2::init(const EffParam *param)
{
    ROSEPAN_STUB("EffSteam2::init", 0x00179b9c);
    return {};
}

// stub of ROR 0x00179c30
int EffSteam2::liveMain()
{
    ROSEPAN_STUB("EffSteam2::liveMain", 0x00179c30);
    return {};
}

// stub of ROR 0x0028cd50
void EffSteam2::render(RENDERPARAM *param)
{
    ROSEPAN_STUB("EffSteam2::render", 0x0028cd50);
}

// stub of ROR 0x0028cd58
u_long * EffSteam2::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffSteam2::renderMain", 0x0028cd58);
    return {};
}

// stub of ROR 0x0028cd48
I3dDrawSet * EffSteam2::createPacketEntry(I3dLayer *layer)
{
    ROSEPAN_STUB("EffSteam2::createPacketEntry", 0x0028cd48);
    return {};
}

// stub of ROR 0x00179dfc
int EffSteam2Prim::init(const EffParam *param)
{
    ROSEPAN_STUB("EffSteam2Prim::init", 0x00179dfc);
    return {};
}

// stub of ROR 0x00179e70
int EffSteam2Prim::liveMain()
{
    ROSEPAN_STUB("EffSteam2Prim::liveMain", 0x00179e70);
    return {};
}

// stub of ROR 0x00179ed0
u_long * EffSteam2Prim::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffSteam2Prim::renderMain", 0x00179ed0);
    return {};
}

// stub of ROR 0x0017a2bc
void EffSteam2Prim::updateZDepth()
{
    ROSEPAN_STUB("EffSteam2Prim::updateZDepth", 0x0017a2bc);
}

// stub of ROR 0x0017a2fc
int EffCloud1::init(const EffParam *param)
{
    ROSEPAN_STUB("EffCloud1::init", 0x0017a2fc);
    return {};
}

// stub of ROR 0x0017a534
int EffCloud1::liveMain()
{
    ROSEPAN_STUB("EffCloud1::liveMain", 0x0017a534);
    return {};
}

// stub of ROR 0x0017a5ec
u_long * EffCloud1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    ROSEPAN_STUB("EffCloud1::renderMain", 0x0017a5ec);
    return {};
}

// stub of ROR 0x0017a8d8
void EffCloud1::updateZDepth()
{
    ROSEPAN_STUB("EffCloud1::updateZDepth", 0x0017a8d8);
}

// stub of ROR 0x00225108
float i3rBlendGetWeight(float t, const I3rItem *target)
{
    ROSEPAN_STUB("i3rBlendGetWeight", 0x00225108);
    return {};
}

// stub of ROR 0x00225498
float * i3rBlendVertices(float t, I3rVertexRef *ref, float *buf, const I3rItem *blocks, u_int num_blocks,
                        const I3rBlendList *list, u_int flags)
{
    ROSEPAN_STUB("i3rBlendVertices", 0x00225498);
    return {};
}

// stub of ROR 0x00200700
void TskEnemyMove::start(const EmyMoveParam *prm)
{
    ROSEPAN_STUB("TskEnemyMove::start", 0x00200700);
}

// stub of ROR 0x0020350c
void TskEnemyTurn::start(const EmyTurnParam *prm)
{
    ROSEPAN_STUB("TskEnemyTurn::start", 0x0020350c);
}

