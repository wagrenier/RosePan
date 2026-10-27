// dog/dogentity.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "dog/dogentity.h"

#include "dog/dogactor.h"

// ──────────────────────────────────────────────────────────────────────
// Written with dog/dogactor.cpp (2026-10-27; 0x10fcb4 is in
// unassigned@0010f944, DogEntity's methods): DogActor::runMotionCmd's
// blend-in turn.
// ROR 0x0010fcb4

void DogEntity::setAddRotY(float r)
{
    m_80[1] = r;
    if (r < -DOG_PI)
        m_80[1] = r + DOG_PI2;
    if (m_80[1] >= DOG_PI)
        m_80[1] -= DOG_PI2;
    m_dirty = 1;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x001102b4
void DogEntityEx::setParent(DogEntityEx *p)
{
    ROSEPAN_STUB("DogEntityEx::setParent", 0x001102b4);
}

// stub of ROR 0x0011023c
void DogEntity::reset()
{
    ROSEPAN_STUB("DogEntity::reset", 0x0011023c);
}

// stub of ROR 0x0011034c
void DogEntityEx::transform()
{
    ROSEPAN_STUB("DogEntityEx::transform", 0x0011034c);
}
