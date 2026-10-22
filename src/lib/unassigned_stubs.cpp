// lib/unassigned_stubs.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "rx/rxfile.h"
#include "rx/rxthread.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x0024a258
int rxThAddCallback(void (*func)(void))
{
    ROSEPAN_STUB("rxThAddCallback", 0x0024a258);
    return {};
}

// stub of ROR 0x0024a8c8
void rxThDisableDispatch(void)
{
    ROSEPAN_STUB("rxThDisableDispatch", 0x0024a8c8);
}

// stub of ROR 0x0024a958
void rxThEnableDispatch(void)
{
    ROSEPAN_STUB("rxThEnableDispatch", 0x0024a958);
}

// stub of ROR 0x0024b0b0
void rxThInitOnce(void)
{
    ROSEPAN_STUB("rxThInitOnce", 0x0024b0b0);
}

// stub of ROR 0x0024ae60
void rxThRequest(RxThRequestFunc func, int a, int b, int c)
{
    ROSEPAN_STUB("rxThRequest", 0x0024ae60);
}

// stub of ROR 0x0024a508
void rxThYield(void)
{
    ROSEPAN_STUB("rxThYield", 0x0024a508);
}

