// rx/rxthread.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "rx/rxthread.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x0024ace8
int rxThCreate(void (*entry)(void), void *stack, int stackSize,
                int priority, void *param)
{
    ROSEPAN_STUB("rxThCreate", 0x0024ace8);
    return {};
}

// stub of ROR 0x0024ab40
void rxThDelete(int index)
{
    ROSEPAN_STUB("rxThDelete", 0x0024ab40);
}

// stub of ROR 0x0024aae0
int rxThGetIndex(void)
{
    ROSEPAN_STUB("rxThGetIndex", 0x0024aae0);
    return {};
}

// stub of ROR 0x0024ac68
void * rxThGetParam(int index)
{
    ROSEPAN_STUB("rxThGetParam", 0x0024ac68);
    return {};
}

// stub of ROR 0x0024ac28
void rxThSetPriority(int index, int priority)
{
    ROSEPAN_STUB("rxThSetPriority", 0x0024ac28);
}

// stub of ROR 0x0024acc8
void rxThSetState(int index, short state)
{
    ROSEPAN_STUB("rxThSetState", 0x0024acc8);
}

// stub of ROR 0x0024b3f8
int rxThSync(int mode)
{
    ROSEPAN_STUB("rxThSync", 0x0024b3f8);
    return {};
}
