// mag/magscenelight.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "mag/magscene.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x0013fa50
void MagScene::getLightParam(int type, int idx, int which, float *v, int no)
{
    ROSEPAN_STUB("MagScene::getLightParam", 0x0013fa50);
}

// stub of ROR 0x0013f528
void MagScene::setLightFlags(const int *flags, int no)
{
    ROSEPAN_STUB("MagScene::setLightFlags", 0x0013f528);
}

// stub of ROR 0x0013f980
void MagScene::setLightParam(int type, int idx, int which, const float *v, int no)
{
    ROSEPAN_STUB("MagScene::setLightParam", 0x0013f980);
}

// stub of ROR 0x0013f4d0
void MagScene::setLightNo(int theNo)
{
    ROSEPAN_STUB("MagScene::setLightNo", 0x0013f4d0);
}

// stub of ROR 0x0013fe64
int MagScene::addLight(int type, unsigned int mask)
{
    ROSEPAN_STUB("MagScene::addLight", 0x0013fe64);
    return {};
}

// stub of ROR 0x0013f658
void MagScene::loadLight(const void *theData)
{
    ROSEPAN_STUB("MagScene::loadLight", 0x0013f658);
}
