/* ==========================================================================
 *  spr/sprtexture.cpp
 *
 *  The sprite texture loader (see spr/sprtexture.h). Not reconstructed
 *  yet: stubs.
 * ======================================================================== */
#include "spr/sprtexture.h"

#include "common/rosepan_stub.h"

// ---- bridges (hand-written) ----

// stub of ROR 0x0018febc
// PC: S2dMes::loadTexture and S2dMovie::loadTexture spin on this until it is
// non-zero; a stub's 0 would hang MagScene::initRender. Reports "done" with
// nothing loaded until this TU is written.
int SprTexture::update()
{
    ROSEPAN_BRIDGE("SprTexture::update", 0x0018febc);
    return 1;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x0018f744
void SprTexture::setList(const SprTexFile *list, const char *rpk)
{
    ROSEPAN_STUB("SprTexture::setList", 0x0018f744);
}

// stub of ROR 0x0018f808
void SprTexture::setListMode(const SprTexFile *list, const char *rpk, int mode)
{
    ROSEPAN_STUB("SprTexture::setListMode", 0x0018f808);
}
