// mag/maglifemenu.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "mag/maglifemenu.h"

// ---- stubs (hand-written: the base's slots, MagLifeMenuItem's bodies) ----

// stub of ROR 0x00284040
MagLifeMenuBase::~MagLifeMenuBase()
{
    ROSEPAN_STUB("MagLifeMenuItem::~MagLifeMenuItem", 0x00284040);
}

// stub of ROR 0x0012d040
void MagLifeMenuBase::init(DogTexInfo *tex)
{
    ROSEPAN_STUB("MagLifeMenuItem::init", 0x0012d040);
}

// stub of ROR 0x002840b0
void MagLifeMenuBase::term()
{
    ROSEPAN_STUB("MagLifeMenuItem::term", 0x002840b0);
}

// stub of ROR 0x002840cc
int MagLifeMenuBase::isInit()
{
    ROSEPAN_STUB("MagLifeMenuItem::isInit", 0x002840cc);
    return {};
}

// stub of ROR 0x0012d06c
int MagLifeMenuBase::processPad(BedPad *pad)
{
    ROSEPAN_STUB("MagLifeMenuItem::processPad", 0x0012d06c);
    return {};
}
