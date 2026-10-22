// i3d/i3dvram.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "i3d/i3dvram.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00228820
int i3dVramCompact(I3dVramHeap *heap)
{
    ROSEPAN_STUB("i3dVramCompact", 0x00228820);
    return {};
}

// stub of ROR 0x00228618
int i3dVramEndFrame(I3dVramHeap *heap)
{
    ROSEPAN_STUB("i3dVramEndFrame", 0x00228618);
    return {};
}

// stub of ROR 0x002289f8
int i3dVramInit(I3dVramHeap *heap, I3dVramBlock *nodes, u_int n,
                 I3dVramBlock **free_stack)
{
    ROSEPAN_STUB("i3dVramInit", 0x002289f8);
    return {};
}

// stub of ROR 0x00228650
int i3dVramReserve(I3dVramHeap *heap, I3dVramHandle **handles, u_int n)
{
    ROSEPAN_STUB("i3dVramReserve", 0x00228650);
    return {};
}

// stub of ROR 0x00228970
int i3dVramReset(I3dVramHeap *heap)
{
    ROSEPAN_STUB("i3dVramReset", 0x00228970);
    return {};
}

// stub of ROR 0x00228940
void i3dVramSetBase(I3dVramHeap *heap, u_int base)
{
    ROSEPAN_STUB("i3dVramSetBase", 0x00228940);
}

// stub of ROR 0x00228958
void i3dVramSetSize(I3dVramHeap *heap, u_int size)
{
    ROSEPAN_STUB("i3dVramSetSize", 0x00228958);
}

// stub of ROR 0x00228ae0
int i3dVramTerm(I3dVramHeap *heap)
{
    ROSEPAN_STUB("i3dVramTerm", 0x00228ae0);
    return {};
}
