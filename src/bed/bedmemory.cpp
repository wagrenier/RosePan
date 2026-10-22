// bed/bedmemory.cpp -- Rule of Rose module (file from the inference in ror_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/rosepan_stub.h"
#include "bed/bedmemory.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00100a3c
void * BedMemory::alloc(int size, int align)
{
    ROSEPAN_STUB("BedMemory::alloc", 0x00100a3c);
    return {};
}

// stub of ROR 0x00100c84
void * BedMemory::allocDebug(int size, const char *filename, int line, int id, int align)
{
    ROSEPAN_STUB("BedMemory::allocDebug", 0x00100c84);
    return {};
}

// stub of ROR 0x00101518
void BedMemory::check(int mode)
{
    ROSEPAN_STUB("BedMemory::check", 0x00101518);
}

// stub of ROR 0x00100354
void BedMemory::create(int size, int id)
{
    ROSEPAN_STUB("BedMemory::create", 0x00100354);
}

// stub of ROR 0x00100568
void BedMemory::destroy()
{
    ROSEPAN_STUB("BedMemory::destroy", 0x00100568);
}

// stub of ROR 0x00101328
void BedMemory::free(void *ptr)
{
    ROSEPAN_STUB("BedMemory::free", 0x00101328);
}

// stub of ROR 0x001010a4
void * BedMemory::realloc(void *ptr, int size)
{
    ROSEPAN_STUB("BedMemory::realloc", 0x001010a4);
    return {};
}
