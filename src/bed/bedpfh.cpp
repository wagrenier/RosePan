// bed/bedpfh.cpp -- BedPfh, the view on a pack file (bed/bedpfh.h).

#include "bed/bedpfh.h"

// ROR 0x00101738
void BedPfh::clear()
{
    m_name = NULL;
    m_top = NULL;
    m_size = NULL;
    m_offset = NULL;
    m_attr = NULL;
}

// ROR 0x00101750  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
// The tables follow the 0x20-byte header in flag order: sizes (1), offsets
// (2), one 16-bit attribute row per consecutive flag bit from 0x10, names
// (0x8000).
BedPfh::BedPfh(const void *p)
{
    clear();
    m_top = (BedPfhHead *)p;
    // PC: the ROM's assert hangs here; the host carries on, so leave the view
    // empty instead of reading tables out of a missing or foreign buffer.
    if (!checkID()) {
        ROR_ASSERT(checkID()); /* 53 */
        return;
    }

    u_char *q = (u_char *)(m_top + 1);
    int flags = getFlags();
    if (flags & 1) {
        m_size = (int *)q;
        q += getNumFile() * 4;
    }
    if (flags & 2) {
        m_offset = (int *)q;
        q += getNumFile() * 4;
    }
    if (flags & 0x10) {
        m_attr = (short *)q;
        int rows = 1;
        while (flags & (0x10 << rows))
            rows++;
        q += rows * getNumFile() * 2;
    }
    if (flags & 0x8000) {
        m_name = (const char *)q;
        getTop();
    }
}

// ROR 0x00101928  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
const char *BedPfh::getFileName(int theN) const
{
    ROR_ASSERT(theN >= 0 && theN <= getNumFile()); /* 90 */
    const char *s = m_name;
    // PC: guard the walk the ROM would not reach after a failed assert.
    if (s == NULL)
        return NULL;
    for (; theN > 0; theN--)
        while (*s++ != '\0')
            ;
    return s;
}

// ROR 0x001019f0  // callcheck: dbgAssertFailed (the assert handler, inside getNumFile)
int BedPfh::getFileIndex(const char *theName) const
{
    const char *s = m_name;
    if (s == NULL)
        return -1;
    int num = getNumFile();
    for (int i = 0; i < num; i++) {
        int k = 0;
        if (s[0] == theName[0]) {
            while (s[k] == theName[k]) {
                if (s[k] == '\0')
                    return i;
                k++;
            }
        }
        while (*s != '\0')
            s++;
        s++;
    }
    return -1;
}
