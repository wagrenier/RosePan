// mag/magscene2.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "mag/magscene.h"

// ---- stubs (hand-written: constructors and destructors are not stubbed by stubgen) ----

// stub of ROR 0x001360a0
// PC: runs before main (g_magSystem's member); the stub only logs.
MagScene::MagScene()
{
    ROSEPAN_STUB("MagScene::MagScene", 0x001360a0);
}

// stub of ROR 0x00136d14
MagScene::~MagScene()
{
    ROSEPAN_STUB("MagScene::~MagScene", 0x00136d14);
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x0013fe64
int MagScene::addLight(int type, unsigned int mask)
{
    ROSEPAN_STUB("MagScene::addLight", 0x0013fe64);
    return {};
}

// stub of ROR 0x0013604c
void MagScene::clearPC()
{
    ROSEPAN_STUB("MagScene::clearPC", 0x0013604c);
}

// stub of ROR 0x0013d5ac
int MagScene::getCameraMode(int *out)
{
    ROSEPAN_STUB("MagScene::getCameraMode", 0x0013d5ac);
    return {};
}

// stub of ROR 0x00131840
void MagScene::init()
{
    ROSEPAN_STUB("MagScene::init", 0x00131840);
}

// stub of ROR 0x00131b90
int MagScene::initRender(i3dContext *ctx)
{
    ROSEPAN_STUB("MagScene::initRender", 0x00131b90);
    return {};
}

// stub of ROR 0x001377f0
int MagScene::load(i3dContext *ctx, int a, int b, int c)
{
    ROSEPAN_STUB("MagScene::load", 0x001377f0);
    return {};
}

// stub of ROR 0x0013910c
int MagScene::loadActorAnim(int no, int arg, const char *name)
{
    ROSEPAN_STUB("MagScene::loadActorAnim", 0x0013910c);
    return {};
}

// stub of ROR 0x00138ed0
int MagScene::loadModel(i3dContext *ctx, int id, const char *rpk)
{
    ROSEPAN_STUB("MagScene::loadModel", 0x00138ed0);
    return {};
}

// stub of ROR 0x00138bc4
void MagScene::loadModelByName(i3dContext *ctx, const char *name, int arg,
                         const char *rpk)
{
    ROSEPAN_STUB("MagScene::loadModelByName", 0x00138bc4);
}

// stub of ROR 0x0013c29c
void MagScene::postDraw()
{
    ROSEPAN_STUB("MagScene::postDraw", 0x0013c29c);
}

// stub of ROR 0x001390d4
void MagScene::setActorAnim(int no, int a, int b)
{
    ROSEPAN_STUB("MagScene::setActorAnim", 0x001390d4);
}

// stub of ROR 0x00131790
void MagScene::setBwn(int no, int arg)
{
    ROSEPAN_STUB("MagScene::setBwn", 0x00131790);
}

// stub of ROR 0x0013c2ec
void MagScene::setCameraTarget(const float *pos, const float *at)
{
    ROSEPAN_STUB("MagScene::setCameraTarget", 0x0013c2ec);
}

// stub of ROR 0x001316b8
void MagScene::setPC(int no, int mode)
{
    ROSEPAN_STUB("MagScene::setPC", 0x001316b8);
}
