// dog/dogactor.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "dog/dogactor.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x0010a038
void DogActor::addTrans(const float *d, float trans_ofs_y, float rot_y_aar)
{
    ROSEPAN_STUB("DogActor::addTrans", 0x0010a038);
}

// stub of ROR 0x00109348
int DogActor::getMotionMove(float t0, float t1, float *move, float *rot, float *pos)
{
    ROSEPAN_STUB("DogActor::getMotionMove", 0x00109348);
    return {};
}

// stub of ROR 0x0010eab8
void DogActor::getNodeWorldMatrix(sceVu0FMATRIX out, int n)
{
    ROSEPAN_STUB("DogActor::getNodeWorldMatrix", 0x0010eab8);
}

// stub of ROR 0x00109b88
void DogActor::releaseMotion(int theId)
{
    ROSEPAN_STUB("DogActor::releaseMotion", 0x00109b88);
}

// stub of ROR 0x0010c17c
u_int DogActor::loadPackMotion(const char *theFileName, int a, int b)
{
    ROSEPAN_STUB("DogActor::loadPackMotion", 0x0010c17c);
    return {};
}
