// lib/unassigned_stubs.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "rx/rxfile.h"
#include "rx/rxthread.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x002458b0
int rxFileCallDevice(int device, const char *arg)
{
    ROSEPAN_STUB("rxFileCallDevice", 0x002458b0);
    return {};
}

// stub of ROR 0x00248870
int rxFileGetCurrent(void)
{
    ROSEPAN_STUB("rxFileGetCurrent", 0x00248870);
    return {};
}

// stub of ROR 0x00246278
void rxFileInit(void)
{
    ROSEPAN_STUB("rxFileInit", 0x00246278);
}

// stub of ROR 0x00248660
void rxFileInitSlots(void)
{
    ROSEPAN_STUB("rxFileInitSlots", 0x00248660);
}

// stub of ROR 0x00245ed8
int rxFileLoad(const char *name, void *buf)
{
    ROSEPAN_STUB("rxFileLoad", 0x00245ed8);
    return {};
}

// stub of ROR 0x00246030
int rxFileLoadModule(const char *path, int arglen, const char *args)
{
    ROSEPAN_STUB("rxFileLoadModule", 0x00246030);
    return {};
}

// stub of ROR 0x002461f0
int rxFileRebootIop(const char *img)
{
    ROSEPAN_STUB("rxFileRebootIop", 0x002461f0);
    return {};
}

// stub of ROR 0x00248890
int rxFileSetDevice(int slot, int device)
{
    ROSEPAN_STUB("rxFileSetDevice", 0x00248890);
    return {};
}

// stub of ROR 0x00248998
void rxFileSetDir(int ctx, const char *dir)
{
    ROSEPAN_STUB("rxFileSetDir", 0x00248998);
}

// stub of ROR 0x00248a80
void rxFileSetSearchDir(int ctx, const char *dir)
{
    ROSEPAN_STUB("rxFileSetSearchDir", 0x00248a80);
}

// stub of ROR 0x00248fd8
void rxFileStartThread(void)
{
    ROSEPAN_STUB("rxFileStartThread", 0x00248fd8);
}

// stub of ROR 0x0024a258
int rxThAddCallback(void (*func)(void))
{
    ROSEPAN_STUB("rxThAddCallback", 0x0024a258);
    return {};
}

// stub of ROR 0x0024a8c8
void rxThDisableDispatch(void)
{
    ROSEPAN_STUB("rxThDisableDispatch", 0x0024a8c8);
}

// stub of ROR 0x0024a958
void rxThEnableDispatch(void)
{
    ROSEPAN_STUB("rxThEnableDispatch", 0x0024a958);
}

// stub of ROR 0x0024b0b0
void rxThInitOnce(void)
{
    ROSEPAN_STUB("rxThInitOnce", 0x0024b0b0);
}

// stub of ROR 0x0024ae60
void rxThRequest(RxThRequestFunc func, int a, int b, int c)
{
    ROSEPAN_STUB("rxThRequest", 0x0024ae60);
}

// stub of ROR 0x0024a508
void rxThYield(void)
{
    ROSEPAN_STUB("rxThYield", 0x0024a508);
}

// stub of ROR 0x00248a60
char * rxFileGetDir(int ctx)
{
    ROSEPAN_STUB("rxFileGetDir", 0x00248a60);
    return {};
}

// stub of ROR 0x00248bb0
char * rxFileGetSearchDir(int ctx)
{
    ROSEPAN_STUB("rxFileGetSearchDir", 0x00248bb0);
    return {};
}
