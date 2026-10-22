// lib/unassigned_stubs.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

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

// The i3d library's own struct sizes (data 0x302ba0); MagSystem::initGraph
// compares them with the game's copy and hangs on a mismatch, so a {}
// stub would stop the boot. Delete with the i3d library TU.
int i3dGetStructSize(unsigned int i)
{
    static const int s_size[0x24] = {
        0x20,  0x130, 0x130, 0x1e,  0x38,    0x38,    0x6,  0x18, 0x8,
        0x20,  0x20,  0x20,  0x20,  0x40,    0x40,    0x40, 0x40, 0x20,
        0x4d0, 0x3b0, 0x80,  0xa0,  0x8,     0x44,    0x18, 0x20, 0x12168,
        0x12168, 0x10, 0x18, 0x60,  0x400,   0x400,   0x18, 0x20, 0x38,
    };
    ROSEPAN_BRIDGE("i3dGetStructSize", 0x0021f1a8);
    return i < 0x24 ? s_size[i] : -1;
}

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

// stub of ROR 0x0021f6a0
int i3dCtxBeginFrame(i3dContext *ctx)
{
    ROSEPAN_STUB("i3dCtxBeginFrame", 0x0021f6a0);
    return {};
}

// stub of ROR 0x0021f760
int i3dCtxEndFrame(i3dContext *ctx)
{
    ROSEPAN_STUB("i3dCtxEndFrame", 0x0021f760);
    return {};
}

// stub of ROR 0x0021f250
int i3dCtxGetOverrun(i3dContext *ctx)
{
    ROSEPAN_STUB("i3dCtxGetOverrun", 0x0021f250);
    return {};
}

// stub of ROR 0x0021f1f8
void * i3dCtxGetPacket(i3dContext *ctx)
{
    ROSEPAN_STUB("i3dCtxGetPacket", 0x0021f1f8);
    return {};
}


// stub of ROR 0x0021f900
int i3dCtxInit(i3dContext *ctx, void *buf, int size, int count)
{
    ROSEPAN_STUB("i3dCtxInit", 0x0021f900);
    return {};
}

// stub of ROR 0x0021ed10
int i3dCtxSetup(i3dContext *ctx, int a, int b)
{
    ROSEPAN_STUB("i3dCtxSetup", 0x0021ed10);
    return {};
}

// stub of ROR 0x0021f8c8
int i3dCtxTerm(i3dContext *ctx)
{
    ROSEPAN_STUB("i3dCtxTerm", 0x0021f8c8);
    return {};
}

// stub of ROR 0x0021eb28
int i3dInitHardware(void)
{
    ROSEPAN_STUB("i3dInitHardware", 0x0021eb28);
    return {};
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

// A frame with nothing drawn leaves its whole share of the packet buffer
// (0x1ae000); MagSystem::update asserts the rest is not 0 every frame, so a
// {} stub would flood the log. Delete with the i3d library TU.
int i3dCtxGetRest(i3dContext *ctx)
{
    (void)ctx;
    ROSEPAN_BRIDGE("i3dCtxGetRest", 0x0021f200);
    return 0x1ae000;
}
