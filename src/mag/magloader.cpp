// mag/magloader.cpp -- the scene's character and map-unit pack loaders
// (0x141e44..0x14313c): MagLoaderIndex, MagCharaLoader, MagMapLoader.

#include <stdint.h>
#include <string.h>

#include "bed/bedpfh.h"
#include "common/ror_assert.h"
#include "lib/unassigned.h"
#include "mag/magloader.h"
#include "utl/utlmisc.h"

/* The 64-byte aligned start of a read buffer, as the ROM computes it from
 * a stack array 0x40 bytes longer than the read ((p >> 6 << 6) + 0x40). */
static inline u_char *alignReadBuf(u_char *work)
{
    return (u_char *)(((uintptr_t)work >> 6 << 6) + 0x40);
}

// ──────────────────────────────────────────────────────────────────────
// A debug listing of the entries; only the loop over m_num is left.
// ROR 0x00141e44

void MagLoaderIndex::dump() const
{
    if (m_num > 0) {
        for (int i = 1; i < m_num; i++) {
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// The block for theNum entries: on the EE 13 bytes each (size, offset and
// name-pointer words, a length byte), then the names.
// ROR 0x00141e78

int MagLoaderIndex::calcSize(int theNum, int theNameSize) const
{
    /* PC: the name pointers are host pointers (the ROM's theNum * 0xd). */
    return theNum * (int)(sizeof(int) * 2 + sizeof(char *) + 1) + theNameSize;
}

// ──────────────────────────────────────────────────────────────────────
// Each of theNum NUL-separated names: its length and its start offset in
// theNameBuf. Returns the bytes the names take, NULs included.
// callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
// ROR 0x00141e90

int MagLoaderIndex::scanNames(int *theLens, int *theOffsets, int theNum,
                              const char *theNameBuf) const
{
    ROR_ASSERT(theNameBuf); /* 62 */
    int pos = 0;
    for (int i = 0; i < theNum; i++) {
        theOffsets[i] = pos;
        int len = (int)strlen(theNameBuf);
        theLens[i] = len;
        pos += len + 1;
        theNameBuf += len + 1;
    }
    return pos;
}

// ──────────────────────────────────────────────────────────────────────
// A new index over a pack header's tables: one game-heap block with the
// sizes, the offsets (plus theBase, the pack's own offset in its file),
// the name pointers, the name lengths, then a copy of the names.
// callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
// ROR 0x00141f54

int MagLoaderIndex::create(int theNum, const char *theNames, const int *theOffsets,
                           const int *theSizes, int theBase)
{
    int lens[1024];
    int offs[1024];

    ROR_ASSERT(0 < theNum && theNum <= 1024); /* 87 */
    m_name_size = scanNames(lens, offs, theNum, theNames);
    m_data_size = calcSize(theNum, m_name_size);
    m_data = g_utlMemory.alloc(m_data_size, -1);
    ROR_ASSERT(m_data); /* 100 */

    m_size = (int *)m_data;
    m_offset = m_size + theNum;
    m_name = (const char **)(m_offset + theNum);
    m_name_len = (u_char *)(m_name + theNum);
    char *buf = (char *)(m_name_len + theNum);
    memcpy(buf, theNames, m_name_size);
    for (int i = 0; i < theNum; i++) {
        m_size[i] = theSizes[i];
        m_offset[i] = theOffsets[i] + theBase;
        m_name_len[i] = (u_char)lens[i];
        m_name[i] = buf + offs[i];
    }
    m_num = theNum;
    m_name_buf = buf;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Appends theNum entries: a new block with the old entries and then the
// new ones, the old block released. theSort orders the new entries by
// their first 32 name characters (an exchange sort over the tail only).
// callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
// ROR 0x001420f8

void MagLoaderIndex::add(int theNum, const char *theNames, const int *theOffsets,
                         const int *theSizes, int theBase, int theSort)
{
    int lens[1024];
    int offs[1024];

    int oldNum = m_num;
    if (oldNum == 0) {
        create(theNum, theNames, theOffsets, theSizes, theBase);
        return;
    }

    int num = oldNum + theNum;
    int addSize = scanNames(lens, offs, theNum, theNames);
    int nameSize = m_name_size + addSize;
    int dataSize = calcSize(num, nameSize);
    void *data = g_utlMemory.alloc(dataSize, -1);
    ROR_ASSERT(data); /* 228 */

    int *sizes = (int *)data;
    int *offsets = sizes + num;
    const char **names = (const char **)(offsets + num);
    u_char *nameLen = (u_char *)(names + num);
    char *buf = (char *)(nameLen + num);
    memcpy(buf, m_name_buf, m_name_size);
    memcpy(buf + m_name_size, theNames, addSize);

    /* The ROM stores the names' offsets first, then rebases them. */
    for (int i = 0; i < m_num; i++)
        names[i] = buf + (m_name[i] - m_name_buf);
    for (int i = m_num; i < num; i++)
        names[i] = buf + offs[i - m_num] + m_name_size;

    for (int i = 0; i < m_num; i++) {
        sizes[i] = m_size[i];
        offsets[i] = m_offset[i];
        nameLen[i] = m_name_len[i];
    }
    for (int i = 0; i < theNum; i++) {
        sizes[m_num + i] = theSizes[i];
        offsets[m_num + i] = theOffsets[i] + theBase;
        nameLen[m_num + i] = (u_char)lens[i];
    }

    release();
    m_num = num;
    m_size = sizes;
    m_data_size = dataSize;
    m_offset = offsets;
    m_name = names;
    m_name_len = nameLen;
    m_name_buf = buf;
    m_name_size = nameSize;
    m_data = data;

    if (theSort) {
        int last = num - 1;
        for (int i = oldNum; i < last; i++) {
            for (int j = last; j > i; j--) {
                if (strncmp(m_name[i], m_name[j], 0x20) > 0) {
                    const char *n = m_name[i];
                    m_name[i] = m_name[j];
                    m_name[j] = n;
                    int s = m_size[i];
                    m_size[i] = m_size[j];
                    m_size[j] = s;
                    int o = m_offset[i];
                    m_offset[i] = m_offset[j];
                    m_offset[j] = o;
                    u_char l = m_name_len[i];
                    m_name_len[i] = m_name_len[j];
                    m_name_len[j] = l;
                }
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// The entry named theName, compared without case; -1 when none.
// ROR 0x00142500

int MagLoaderIndex::find(const char *theName) const
{
    for (int i = 0; i < m_num; i++) {
        if (utlStrEqualNoCase(theName, m_name[i]))
            return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// Zeroes the entry count, the block and the tables; the name size and
// buffer (words 7 / 8) are left as they are.
// ROR 0x0014257c

void MagLoaderIndex::clear()
{
    m_offset = NULL;
    m_num = 0;
    m_data = NULL;
    m_data_size = 0;
    m_name = NULL;
    m_name_len = NULL;
    m_size = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Frees the block to the game heap, then clears.
// ROR 0x0014259c

void MagLoaderIndex::release()
{
    if (m_data != NULL)
        g_utlMemory.free(m_data);
    clear();
}

// ──────────────────────────────────────────────────────────────────────
// MagScene's constructor builds it: an empty index, no entries per pack.
// callcheck: FileReader (the member's constructor, run by the host)
// ROR 0x00142e84

MagCharaLoader::MagCharaLoader()
{
    m_index.clear();
    for (int i = 0; i < MAG_CHARA_PACK_MAX; i++) {
        m_num_chara[i] = 0;
        m_chara_start[i] = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Reads CHARA.RPK's 2048-byte header (the file stays open), then for each
// of the 20 sub-packs it names the sub-pack's first 0x2000 bytes, and
// appends that header's entries to the index with the sub-pack's offset
// added: entry numbers run pack after pack, m_chara_start[i] being pack
// i's first. Returns 1.
// callcheck: dbgAssertFailed, FileReadReq, BedPfh (the assert handler inside ROR_ASSERT; the request's reset, the local pack views)
// ROR 0x001425dc

int MagCharaLoader::init()
{
    m_req = FileReadReq();
    m_index.clear();

    u_char head[0x800 + 0x40];
    memset(head, 0, sizeof(head));
    m_req.m_name = "data/ch/CHARA.RPK";
    m_req.m_08 = 1;
    m_req.m_size = 0x800;
    m_req.m_buf = alignReadBuf(head);
    m_reader.read(&m_req, 0);
    BedPfh pfh(m_req.m_buf);

    const char *names[MAG_CHARA_PACK_MAX] = {
        "PC",     "BWN",    "ACTION", "ITEM",   "FIX",    "ENEMY",  "DOOR",
        "INST",   "CHAP01", "CHAP02", "CHAP03", "CHAP04", "CHAP05", "CHAP06",
        "CHAP07", "CHAP08", "CHAP09", "CHAP10", "CHAP11", "CHAP12",
    }; /* data 0x2eed80, copied to the stack */

    int start = 0;
    for (int i = 0; i < MAG_CHARA_PACK_MAX; i++) {
        m_num_chara[i] = 0;
        m_chara_start[i] = start;
        int n = pfh.getFileIndex(names[i]);
        if (n < 0)
            continue;
        ROR_ASSERT(n >= 0 && n < 32); /* 456 */
        if (pfh.getFileSize(n) <= 0)
            continue;
        int offset = pfh.getFileOffset(n);

        u_char work[0x2000 + 0x40];
        memset(work, 0, sizeof(work));
        m_req.m_size = 0x2000;
        m_req.m_08 = 0;
        m_req.m_offset = offset;
        m_req.m_buf = alignReadBuf(work);
        m_req.m_whence = 0;
        m_req.m_name = NULL;
        m_reader.read(&m_req, 0);
        BedPfh sub(m_req.m_buf);

        int num = sub.getNumFile();
        m_num_chara[i] = num;
        m_chara_start[i] = start;
        start += num;
        if (num > 0)
            m_index.add(num, sub.m_name, sub.m_offset, sub.m_size, offset, 0);
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// init's twin over the loose packs (data/ch/pc/PC.RPK .. data/ch/12/
// CHAP12.RPK, each read whole up to 0x2000 bytes with its own request);
// packs that are not on the disc are skipped. Nothing calls it.
// callcheck: dbgAssertFailed, FileReadReq, FileReader, ~FileReader, BedPfh (the assert handler inside ROR_ASSERT; locals' constructors and destructors)
// ROR 0x00142908

int MagCharaLoader::initFromPacks()
{
    static const char *const s_packFile[MAG_CHARA_PACK_MAX] = {
        "data/ch/pc/PC.RPK",       "data/ch/bwn/BWN.RPK",   "data/ch/action/ACTION.RPK",
        "data/ch/item/ITEM.RPK",   "data/ch/fix/FIX.RPK",   "data/ch/enemy/ENEMY.RPK",
        "data/ch/door/DOOR.RPK",   "data/ch/inst/INST.RPK", "data/ch/01/CHAP01.RPK",
        "data/ch/02/CHAP02.RPK",   "data/ch/03/CHAP03.RPK", "data/ch/04/CHAP04.RPK",
        "data/ch/05/CHAP05.RPK",   "data/ch/06/CHAP06.RPK", "data/ch/07/CHAP07.RPK",
        "data/ch/08/CHAP08.RPK",   "data/ch/09/CHAP09.RPK", "data/ch/10/CHAP10.RPK",
        "data/ch/11/CHAP11.RPK",   "data/ch/12/CHAP12.RPK",
    }; /* data 0x298d80 */

    int start = 0;
    m_index.clear();
    u_char work[0x2000 + 0x40];
    memset(work, 0, sizeof(work));
    u_char *buf = alignReadBuf(work);
    FileReader reader;

    for (int i = 0; i < MAG_CHARA_PACK_MAX; i++) {
        m_num_chara[i] = 0;
        m_chara_start[i] = start;
        FileReadReq req;
        if (!fileExists(s_packFile[i]))
            continue;
        req = FileReadReq();
        req.m_name = s_packFile[i];
        req.m_size = 0x2000;
        req.m_close = 1;
        req.m_08 = 1;
        req.m_buf = buf;
        if (reader.read(&req, 0) < 0) {
            ROR_ASSERT(0); /* 606 */
            continue;
        }
        BedPfh pfh(req.m_buf);
        int num = pfh.getNumFile();
        m_num_chara[i] = num;
        m_chara_start[i] = start;
        start += num;
        if (num < 1) {
            ROR_ASSERT(0); /* 599 */
        } else {
            m_index.add(num, pfh.m_name, pfh.m_offset, pfh.m_size, 0, 0);
            m_index.dump();
        }
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Reads pack entry theIndex from CHARA.RPK (still open from init) into a
// new game-heap buffer (64-byte aligned) and returns it.
// callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
// ROR 0x00142b18

void *MagCharaLoader::load(int theIndex)
{
    ROR_ASSERT(theIndex >= 0); /* 631 */
    if (theIndex < 0)
        return NULL; /* PC: the ROM reads entry -1 after the assert */
    int size = m_index.m_size[theIndex];
    ROR_ASSERT(size > 0); /* 634 */
    int offset = m_index.m_offset[theIndex];
    ROR_ASSERT(offset > 0); /* 637 */
    void *pbuf = g_utlMemory.alloc(size, 1);
    ROR_ASSERT(pbuf); /* 640 */
    m_req.m_offset = offset;
    m_req.m_size = size;
    m_req.m_buf = pbuf;
    m_req.m_whence = 0;
    m_reader.read(&m_req, 0);
    return pbuf;
}

// ──────────────────────────────────────────────────────────────────────
// The pack holding pack entry theIndex: the last one with entries whose
// start is at or below it (0 when none).
// ROR 0x00142c2c

int MagCharaLoader::getCategory(int theIndex) const
{
    int cat = MAG_CHARA_PACK_MAX - 1;
    if (m_num_chara[cat] == 0 || theIndex < m_chara_start[cat]) {
        for (cat = MAG_CHARA_PACK_MAX - 2; cat > 0; cat--) {
            if (m_num_chara[cat] != 0 && m_chara_start[cat] <= theIndex)
                return cat;
        }
    }
    return cat;
}

// ──────────────────────────────────────────────────────────────────────
// The pack entry of character id theID: its id category (theID >> 10)
// names a pack, its low 10 bits the number in that pack.
// callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
// ROR 0x00142c88

int MagCharaLoader::getIndex(int theID) const
{
    if (!(theID >= 0 && theID < 0x5800)) {
        ROR_ASSERT(theID >= 0 && theID < (0x5800)); /* 670 */
        return -1;
    }
    /* id category -> pack: 8 PC, 9 BWN, 0 ACTION, 3 ITEM, 2 FIX, 5 ENEMY,
     * 6 DOOR, 4 INST, 10..21 CHAP01..12; 1 and 7 have none. */
    const signed char packOf[22] = {
        2, -1, 4, 3, 7, 5, 6, -1, 0, 1, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    };
    int no = theID % 0x400;
    int cat = packOf[theID / 0x400];
    ROR_ASSERT(cat >= 0); /* 704 */
    if (cat < 0)
        return -1; /* PC: the ROM reads m_num_chara[-1] after the assert */
    ROR_ASSERT(no < m_num_chara[cat]); /* 706 */
    return m_chara_start[cat] + no;
}

// ──────────────────────────────────────────────────────────────────────
// The character id of pack entry theIndex: its pack's id category << 10
// plus its number in the pack.
// callcheck: getCategory (inline in the ROM)
// ROR 0x00142dac

int MagCharaLoader::getId(int theIndex) const
{
    int cat = getCategory(theIndex);
    const short idBase[MAG_CHARA_PACK_MAX] = {
        0x2000, 0x2400, 0x0000, 0x0c00, 0x0800, 0x1400, 0x1800, 0x1000, 0x2800, 0x2c00,
        0x3000, 0x3400, 0x3800, 0x3c00, 0x4000, 0x4400, 0x4800, 0x4c00, 0x5000, 0x5400,
    };
    return idBase[cat] + (theIndex - m_chara_start[cat]);
}

// ──────────────────────────────────────────────────────────────────────
// Reads mapunit.bin's first 0x2000 bytes (the file stays open) and builds
// the index from its header. Returns 1.
// callcheck: dbgAssertFailed, FileReadReq, BedPfh (the assert handler inside ROR_ASSERT; the request's reset, the local pack view)
// ROR 0x00142ed8

int MagMapLoader::init()
{
    m_req = FileReadReq();
    m_index.clear();

    u_char work[0x2000 + 0x40];
    memset(work, 0, sizeof(work));
    m_req.m_08 = 1;
    m_req.m_name = "data/bg/mapunit.bin";
    m_req.m_size = 0x2000;
    m_req.m_buf = alignReadBuf(work);
    m_reader.read(&m_req, 0);
    BedPfh pfh(m_req.m_buf);
    m_index.create(pfh.getNumFile(), pfh.m_name, pfh.m_offset, pfh.m_size, 0);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Reads the map unit named theName into a new game-heap buffer (64-byte
// aligned) and returns it.
// callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
// ROR 0x00142fd0

void *MagMapLoader::loadByName(const char *theName)
{
    ROR_ASSERT(theName); /* 818 */
    int idx = m_index.find(theName);
    ROR_ASSERT(idx >= 0); /* 822 */
    if (idx < 0)
        return NULL; /* PC: the ROM reads entry -1 after the assert */
    int size = m_index.m_size[idx];
    ROR_ASSERT(size > 0); /* 825 */
    int offset = m_index.m_offset[idx];
    ROR_ASSERT(offset > 0); /* 828 */
    void *pbuf = g_utlMemory.alloc(size, 1);
    ROR_ASSERT(pbuf); /* 831 */
    m_req.m_offset = offset;
    m_req.m_size = size;
    m_req.m_buf = pbuf;
    m_req.m_whence = 0;
    m_reader.read(&m_req, 0);
    return pbuf;
}
