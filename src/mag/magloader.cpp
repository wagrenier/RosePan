// mag/magloader.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "mag/magloader.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00142dac
int MagCharaLoader::getId(int theIndex) const
{
    ROSEPAN_STUB("MagCharaLoader::getId", 0x00142dac);
    return {};
}

// stub of ROR 0x00142c88
int MagCharaLoader::getIndex(int theID) const
{
    ROSEPAN_STUB("MagCharaLoader::getIndex", 0x00142c88);
    return {};
}

// stub of ROR 0x001425dc
void MagCharaLoader::init()
{
    ROSEPAN_STUB("MagCharaLoader::init", 0x001425dc);
}

// stub of ROR 0x00142b18
void * MagCharaLoader::load(int theIndex)
{
    ROSEPAN_STUB("MagCharaLoader::load", 0x00142b18);
    return {};
}

// stub of ROR 0x00142500
int MagLoaderIndex::find(const char *theName) const
{
    ROSEPAN_STUB("MagLoaderIndex::find", 0x00142500);
    return {};
}

// stub of ROR 0x00142ed8
void MagMapLoader::init()
{
    ROSEPAN_STUB("MagMapLoader::init", 0x00142ed8);
}

// stub of ROR 0x00142fd0
void * MagMapLoader::loadByName(const char *theName)
{
    ROSEPAN_STUB("MagMapLoader::loadByName", 0x00142fd0);
    return {};
}
