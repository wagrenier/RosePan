// bed/bedpad.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "bed/bedpad.h"

// ---- stubs (hand-written: destructors are not stubbed by stubgen) ----

// stub of ROR 0x00101aec
BedPad::~BedPad()
{
    ROSEPAN_STUB("BedPad::~BedPad", 0x00101aec);
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00101bcc
void BedPad::init(int thePortNo)
{
    ROSEPAN_STUB("BedPad::init", 0x00101bcc);
}

// stub of ROR 0x00101b50
void BedPad::setVibration(int small, int big)
{
    ROSEPAN_STUB("BedPad::setVibration", 0x00101b50);
}

// stub of ROR 0x00101c7c
void BedPad::term()
{
    ROSEPAN_STUB("BedPad::term", 0x00101c7c);
}

// stub of ROR 0x00101ccc
void BedPad::update()
{
    ROSEPAN_STUB("BedPad::update", 0x00101ccc);
}
