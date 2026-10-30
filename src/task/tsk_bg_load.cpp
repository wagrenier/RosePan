/* ==========================================================================
 *  task/tsk_bg_load.cpp
 *
 *  The background pack loaders TskBGLoadArea / TskBGLoadMap (see the
 *  header). Unreferenced in the ROM.
 * ======================================================================== */
#include "task/tsk_bg_load.h"

#include "bed/bedpfh.h"
#include "game/gamman.h"
#include "utl/utlmisc.h"
#include "utl/utlmisc2.h"

static const char s_file[] = "task/tsk_bg_load.cpp";

// The pack TskBGLoadMap reads (the pointer at data 0x2d8ff4).
static const char *const s_mapUnitName = "data/bg/mapunit.bin";

// ROR 0x001fce1c

int TskBGLoad::exec(int id, int time, int arg)
{
    (void)id; (void)time; (void)arg;
    return m_reader.update(1) == 0;
}

// ROR 0x001fce40

int TskBGLoad::startRead(const char *name, void *buf, int size, int offset, int open)
{
    m_30 = 0;
    m_req = FileReadReq();
    if (open == 0)
    {
        m_req.m_08 = 0;
    }
    else
    {
        m_req.m_name = name;
        m_req.m_08 = 1;
    }
    m_req.m_offset = offset;
    m_req.m_size = size;
    m_req.m_buf = buf;
    m_req.m_close = 1;
    m_req.m_whence = 0;
    m_reader.read(&m_req, 1);
    return 0;
}

// ROR 0x001fcef8 // callcheck: FileReader (the member m_reader's constructor)

TskBGLoadArea::TskBGLoadArea(const char *name, void **out)
{
    TskBGLoadArea::start(name, out);
}

// ROR 0x001fcf6c

void TskBGLoadArea::start(const char *name, void **out)
{
    utlStrNCopy(m_name, name, 0x100);
    m_out = out;
    m_step = 0;
    startRead(m_name, getHead(), 0x800, 0, 1);
    m_step = 1;
}

// ROR 0x001fd008 // callcheck: BedPfh, dbgAssertFailed (the local pfh's constructor and getTotalSize's m_top assert, bedpfh.h line 72)

int TskBGLoadArea::exec(int id, int time, int arg)
{
    (void)id; (void)time; (void)arg;
    if (m_step == 1)
    {
        if (m_reader.resume(&m_req, 1) != 0)
            return 0;
        BedPfh pfh(m_req.m_buf);
        int total = pfh.getTotalSize();
        if (total < 1)
            return 1;
        int size = (total + 0x7ff) / 2048 * 2048;
        void *buf = g_utlMemory.allocDebug(size, s_file, 183, 0, 0x101);
        if (buf == NULL)
        {
            g_gamMan.releaseObjs(2, 1);
            buf = g_utlMemory.allocDebug(size, s_file, 216, 0, 0x101);
            if (buf == NULL)
            {
                *m_out = NULL;
                return 1;
            }
        }
        *m_out = buf;
        startRead(m_name, buf, total, 0, 1);
        m_step = 2;
        return 0;
    }
    if (m_step < 1)
        return m_step != 0;
    if (m_step == 2)
        return m_reader.resume(&m_req, 1) == 0;
    return 1;
}

// ROR 0x001fd200 // callcheck: FileReader (the member m_reader's constructor)

TskBGLoadMap::TskBGLoadMap(const char *name, void **out)
{
    TskBGLoadMap::start(name, out);
}

// ROR 0x001fd264

void TskBGLoadMap::start(const char *name, void **out)
{
    (void)name;
    m_out = out;
    m_step = 0;
    startRead(s_mapUnitName, getHead(), 0x800, 0, 1);
    m_step = 1;
}

// ROR 0x001fd2dc // callcheck: BedPfh, dbgAssertFailed (the local pfh's constructor and getTotalSize's m_top assert, bedpfh.h line 72)

int TskBGLoadMap::exec(int id, int time, int arg)
{
    (void)id; (void)time; (void)arg;
    if (m_step == 1)
    {
        if (m_reader.resume(&m_req, 1) != 0)
            return 0;
        BedPfh pfh(m_req.m_buf);
        int total = pfh.getTotalSize();
        if (total < 1)
            return 1;
        void *buf = g_utlMemory.allocDebug((total + 0x7ff) / 2048 * 2048, s_file, 341, 0, 1);
        if (buf == NULL)
            return 1;
        *m_out = buf;
        startRead(s_mapUnitName, buf, total, 0, 1);
        m_step = 2;
        return 0;
    }
    if (m_step < 1)
        return m_step != 0;
    if (m_step == 2)
        return m_reader.resume(&m_req, 1) == 0;
    return 1;
}
