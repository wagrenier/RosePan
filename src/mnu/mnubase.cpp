// mnu/mnubase.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "mnu/mnubase.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00194588
void * mnuAlloc(int size)
{
    ROSEPAN_STUB("mnuAlloc", 0x00194588);
    return {};
}

// stub of ROR 0x0019397c
void mnuBase::addFile(const char *name, u_int kind, int arg)
{
    ROSEPAN_STUB("mnuBase::addFile", 0x0019397c);
}

// stub of ROR 0x00193920
void mnuBase::closePack()
{
    ROSEPAN_STUB("mnuBase::closePack", 0x00193920);
}

// stub of ROR 0x001938a8
void mnuBase::openPack()
{
    ROSEPAN_STUB("mnuBase::openPack", 0x001938a8);
}

// stub of ROR 0x00193868
void mnuBase::restoreVolume()
{
    ROSEPAN_STUB("mnuBase::restoreVolume", 0x00193868);
}

// stub of ROR 0x00192ba4
void mnuBase::setupWork(DogTexInfo *tex, int nFiles, int nModels, int nDatas,
                   int nTexs)
{
    ROSEPAN_STUB("mnuBase::setupWork", 0x00192ba4);
}

// stub of ROR 0x00194688
void mnuFree(void *p)
{
    ROSEPAN_STUB("mnuFree", 0x00194688);
}
