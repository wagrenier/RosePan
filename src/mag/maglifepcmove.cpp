// mag/maglifepcmove.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "mag/maglifepc.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00120668
void MagLifePC::applyMotionMove()
{
    ROSEPAN_STUB("MagLifePC::applyMotionMove", 0x00120668);
}

// stub of ROR 0x0011ea04
float MagLifePC::sweepMove(float range, float radius, const float *from, const float *delta,
                    float *out)
{
    ROSEPAN_STUB("MagLifePC::sweepMove", 0x0011ea04);
    return {};
}

// stub of ROR 0x0011ff10
int MagLifePC::moveAttackStance(BedPad *pad)
{
    ROSEPAN_STUB("MagLifePC::moveAttackStance", 0x0011ff10);
    return {};
}
