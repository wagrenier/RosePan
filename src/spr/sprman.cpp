/* ==========================================================================
 *  spr/sprman.cpp
 *
 *  The sprite manager (see spr/sprman.h). Not reconstructed yet: the data
 *  the S2d layers use and stubs.
 * ======================================================================== */
#include "spr/sprman.h"

#include "common/rosepan_stub.h"

/* ---- data ---- */

const int g_sprColorOpaque[4] = { 0x80, 0x80, 0x80, 0x80 };    /* data 0x299990 */
const int g_sprColorClear[4]  = { 0x80, 0x80, 0x80, 0 };       /* data 0x2999b0 */

// ---- stubs (hand-written: constructors and destructors are not stubbed by stubgen) ----

// stub of ROR 0x0018bb8c
// PC: runs before main (the S2d layers are members of g_magSystem's scene);
// the stub only logs. The ROM constructs the SprTexture and the +0x440
// object and clears the words; the host's zeroed global does the latter.
SprMan::SprMan()
{
    ROSEPAN_STUB("SprMan::SprMan", 0x0018bb8c);
}

// stub of ROR 0x0018bc18
SprMan::~SprMan()
{
    ROSEPAN_STUB("SprMan::~SprMan", 0x0018bc18);
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x0018bd94
void SprMan::draw()
{
    ROSEPAN_STUB("SprMan::draw", 0x0018bd94);
}

// stub of ROR 0x0018f1f4
void SprMan::fadeInSprite(int no, int a, int time, int b, int c,
                      const int *from, const int *to)
{
    ROSEPAN_STUB("SprMan::fadeInSprite", 0x0018f1f4);
}

// stub of ROR 0x0018f520
void SprMan::fadeOut(int no, int time, const int *from, const int *to)
{
    ROSEPAN_STUB("SprMan::fadeOut", 0x0018f520);
}

// stub of ROR 0x0018f41c
void SprMan::fadeOutDefault(int no, int time)
{
    ROSEPAN_STUB("SprMan::fadeOutDefault", 0x0018f41c);
}

// stub of ROR 0x0018bcc0
int SprMan::init(int num)
{
    ROSEPAN_STUB("SprMan::init", 0x0018bcc0);
    return {};
}

// stub of ROR 0x0018c130
int SprMan::release()
{
    ROSEPAN_STUB("SprMan::release", 0x0018c130);
    return {};
}

// stub of ROR 0x0018c4d8
int SprMan::setAnim(int no, int anim)
{
    ROSEPAN_STUB("SprMan::setAnim", 0x0018c4d8);
    return {};
}

// stub of ROR 0x0018c190
void SprMan::setDispList(const SprDispDef *def)
{
    ROSEPAN_STUB("SprMan::setDispList", 0x0018c190);
}

// stub of ROR 0x0018d7ec
void SprMan::setFade(int no, int mode, int time, const int *from, const int *to)
{
    ROSEPAN_STUB("SprMan::setFade", 0x0018d7ec);
}

// stub of ROR 0x0018c068
int SprMan::update()
{
    ROSEPAN_STUB("SprMan::update", 0x0018c068);
    return {};
}
