// lib/unassigned_stubs.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.
#include "dog/dogentity.h"
#include "mag/maglifebwn.h"

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

// stub of ROR 0x00192318
void FUN_00192318(Obj19226c *obj, i3dContext *ctx, MagScene *scene)
{
    ROSEPAN_STUB("FUN_00192318", 0x00192318);
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

// stub of ROR 0x00191068
void S2dBg::init(DogTexInfo *tex)
{
    ROSEPAN_STUB("S2dBg::init", 0x00191068);
}

// stub of ROR 0x00190b70
void S2dCineSco::init(DogTexInfo *tex)
{
    ROSEPAN_STUB("S2dCineSco::init", 0x00190b70);
}

// stub of ROR 0x00191588
int S2dMes::init(DogTexInfo *tex)
{
    ROSEPAN_STUB("S2dMes::init", 0x00191588);
    return {};
}

// stub of ROR 0x0021a120
void i3dFogInit(I3dFog *fog)
{
    ROSEPAN_STUB("i3dFogInit", 0x0021a120);
}
