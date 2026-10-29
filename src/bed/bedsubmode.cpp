// bed/bedsubmode.cpp -- BedSubMode, a mode record with a stack of saved copies (bed/bedsubmode.h).

#include "bed/bedsubmode.h"

#include "common/ror_assert.h"

// ROR 0x00100258  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
void BedSubMode::push()
{
    ROR_ASSERT(m_depth < MAX_STACK); /* 25 */
    int *slot = m_stack[m_depth];
    slot[0] = m_mode;
    slot[1] = m_step;
    slot[2] = m_arg;
    slot[3] = m_arg2;
    m_depth++;
}

// ROR 0x001002c0  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
void BedSubMode::pop()
{
    ROR_ASSERT(m_depth > 0); /* 35 */
    m_depth--;
    const int *slot = m_stack[m_depth];
    m_mode = slot[0];
    m_step = slot[1];
    m_arg = slot[2];
    m_arg2 = slot[3];
}

// ROR 0x00100320
void BedSubMode::reset()
{
    m_depth = 0;
    for (int i = MAX_STACK - 1; i >= 0; i--) {
        m_stack[i][0] = 0;
        m_stack[i][1] = 0;
        m_stack[i][2] = 0;
        m_stack[i][3] = 0;
    }
}
