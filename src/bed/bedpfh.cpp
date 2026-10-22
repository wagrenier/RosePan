// bed/bedpfh.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "bed/bedpfh.h"

#include "common/rosepan_stub.h"

// ---- stubs (hand-written: constructors and destructors are not stubbed by stubgen) ----

// stub of ROR 0x00101750
// PC: clears the view and keeps the header pointer so the inline getters
// see a pack; the tables stay NULL until the constructor is reconstructed.
BedPfh::BedPfh(const void *p)
    : m_top((BedPfhHead *)p), m_size(NULL), m_offset(NULL), m_attr(NULL), m_name(NULL)
{
    ROSEPAN_STUB("BedPfh::BedPfh", 0x00101750);
}

// ---- bridges (hand-written) ----

// bridge of ROR 0x00101738
// The ROM's code; SprTexture's constructor calls it (spr/sprtexture.cpp).
void BedPfh::clear()
{
    ROSEPAN_BRIDGE("BedPfh::clear", 0x00101738);
    m_name = NULL;
    m_top = NULL;
    m_size = NULL;
    m_offset = NULL;
    m_attr = NULL;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x001019f0
int BedPfh::getFileIndex(const char *theName) const
{
    ROSEPAN_STUB("BedPfh::getFileIndex", 0x001019f0);
    return {};
}

// stub of ROR 0x00101928
const char * BedPfh::getFileName(int theN) const
{
    ROSEPAN_STUB("BedPfh::getFileName", 0x00101928);
    return {};
}
