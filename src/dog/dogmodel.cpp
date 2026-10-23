// dog/dogmodel.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "dog/dogmodel.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00115cec
void DogModel::release(int arg)
{
    ROSEPAN_STUB("DogModel::release", 0x00115cec);
}

// stub of ROR 0x00115790
int DogModel::loadData(DogTexInfo *tex, void *data, void *data2)
{
    ROSEPAN_STUB("DogModel::loadData", 0x00115790);
    return {};
}

// stub of ROR 0x001158ec
int DogModel::loadPack(DogTexInfo *tex, const BedPfh &pfh, int light, int arg)
{
    ROSEPAN_STUB("DogModel::loadPack", 0x001158ec);
    return {};
}

// stub of ROR 0x0011550c
void DogModel::setupNodeFlags()
{
    ROSEPAN_STUB("DogModel::setupNodeFlags", 0x0011550c);
}

// stub of ROR 0x001151f4
void DogModel::draw(I3dLayer *layer, const float *cam_rotate, const float *cam_trans)
{
    ROSEPAN_STUB("DogModel::draw", 0x001151f4);
}

// stub of ROR 0x001152a8
void DogModel::drawUnitMatrix(I3dLayer *layer)
{
    ROSEPAN_STUB("DogModel::drawUnitMatrix", 0x001152a8);
}

// stub of ROR 0x00116038
float DogModel::getClipRadius()
{
    ROSEPAN_STUB("DogModel::getClipRadius", 0x00116038);
    return {};
}

// stub of ROR 0x001150c0
void DogModel::transform()
{
    ROSEPAN_STUB("DogModel::transform", 0x001150c0);
}

// stub of ROR 0x0011440c
float DogModel::castRay(void *hit, const float *pos, const float *dir)
{
    ROSEPAN_STUB("DogModel::castRay", 0x0011440c);
    return {};
}

// stub of ROR 0x001149d4
float DogModel::castCylinder(float r, const float *pos, const float *dir, void *hit)
{
    ROSEPAN_STUB("DogModel::castCylinder", 0x001149d4);
    return {};
}

// stub of ROR 0x001145c0
float DogModel::castSphere(float r, void *hit, const float *pos)
{
    ROSEPAN_STUB("DogModel::castSphere", 0x001145c0);
    return {};
}
