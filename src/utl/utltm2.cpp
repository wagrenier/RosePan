// utl/utltm2.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "utl/utltm2.h"

#include <stddef.h>

#include "common/ror_assert.h"
#include "i3d/i3dtim2.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x0010559c
void utlTm2ToSepia(void *tm2, int r, int g, int b)
{
    ROSEPAN_STUB("utlTm2ToSepia", 0x0010559c);
}

// ──────────────────────────────────────────────────────────────────────
// Picture theN's header: the pictures follow the file header (0x10
// bytes, 0x80 when m_format is set), each m_total_size long.
// callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
// ROR 0x00104e64

const I3dTim2Picture *utlTm2GetPicture(const void *tm2, int theN)
{
    const I3dTim2File *h = (const I3dTim2File *)tm2;
    int off = h->m_format != 0 ? 0x80 : 0x10;
    ROR_ASSERT(theN < h->m_pictures); /* 125: theN < h->Pictures */
    const I3dTim2Picture *pic = NULL;
    for (int i = 0; i <= theN; i++) {
        pic = (const I3dTim2Picture *)((const u_char *)tm2 + off);
        off += pic->m_total_size;
    }
    return pic;
}
