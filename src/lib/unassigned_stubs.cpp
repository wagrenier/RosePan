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
#include "emy/emy_dwf.h"
#include "mag/maglifepc.h"
#include "eff/eff0.h"
#include "eff/effclasses.h"
#include "eff/effgeneric.h"

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

// ---- stubs (tools/reverse/stubgen.py) ----

// ---- bridges (hand-written) ----

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

// ──────────────────────────────────────────────────────────────────────
// A step along angle on the ground plane (unassigned@001e6cac, written
// ahead for native 152).
// ROR 0x001f6b28

void utlAddPolar(float angle, float dist, float *v)
{
    v[0] = v[0] + sinf(angle) * dist;
    v[2] = v[2] + cosf(angle) * dist;
}

// ---- stubs (hand-written: EmyMain / EmyOut and the enemy classes' constructors and destructors, which stubgen does not write) ----

// ---- stubs (hand-written: the mnu/ menu objects' virtual slots, which stubgen cannot see behind the vtable) ----

// stub of ROR 0x00172678

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

// ---- Eff constructors / destructors (hand stubs: stubgen leaves them out; eff/effclasses.h) ----

// stub of ROR 0x0028c050
// stub of ROR 0x00172b30
// stub of ROR 0x00172cb8
// stub of ROR 0x0028c058
// stub of ROR 0x0028c060
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
// stub of ROR 0x0028c148
// stub of ROR 0x0028c150
// stub of ROR 0x0028c158
// stub of ROR 0x00172de8
// stub of ROR 0x00172e08
// stub of ROR 0x0028c240
// stub of ROR 0x0028c248
// stub of ROR 0x00174dd8

// stub of ROR 0x00174e9c

// stub of ROR 0x00174eb0

// stub of ROR 0x0017512c

// stub of ROR 0x00175170

// stub of ROR 0x00175254

// stub of ROR 0x00175268

// stub of ROR 0x00175378

// stub of ROR 0x001755e8

