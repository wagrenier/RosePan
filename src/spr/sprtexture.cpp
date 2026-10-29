// spr/sprtexture.cpp -- the sprite texture loader (0x18f5b0..0x190a78).
//
// A SprTexture loads a list of texture files (SprTexFile) into a DogTexInfo,
// one step per update call so the caller can keep the frame going:
//   - setList picks the start by the pack and m_mode: no pack -> the loose
//     files (5); mode 0 -> read the whole pack (3); mode 1 -> read its first
//     4 KB, the header and tables (1), and read each file from the disc.
//   - 5 checks the next entry, 6 finds it (in the pack or as a loose file)
//     and starts its read, 7 waits for the read, 8 decodes RTCM data and
//     adds the entries flagged SPR_TEX_LOAD (loadTex), 9 moves on, 10 hands
//     the table to the i3d context and reports done.
// The reads go through updateRead, a second state machine over a FileReader
// (open, seek, read, close, each polled with isReady).

#include "spr/sprtexture.h"

#include <new>
#include <string.h>

#include "dog/dogtexinfo.h"
#include "lib/unassigned.h"
#include "rx/rxthread.h"
#include "snd/sndman.h"
#include "utl/utlmisc.h"

static const char s_file[] = "spr/sprtexture.cpp";

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0018f5b0

SprTexture::SprTexture()
{
    m_pfh_body.clear();
    clear();
    m_ctx = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// Empty; the deleting variant calls operator delete.
// callcheck: operator_delete (the deleting destructor)
// ROR 0x0018f5e4

SprTexture::~SprTexture()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0018f60c

void SprTexture::clear()
{
    setStep(0);
    setReadStep(0);
    m_reader = NULL;
    m_list = NULL;
    m_400 = 0x40;
    m_count = 0;
    m_cur = 0;
    m_tex_info = NULL;
    m_rpk = NULL;
    m_buf = NULL;
    m_mode = 0;
    for (int i = 0; i < SPR_TEX_FILE_MAX; i++)
        m_tex_no[i] = 0;
    for (int i = 0; i < SPR_TEX_FILE_MAX; i++)
        m_slot[i] = -1;
    for (int i = 0; i < SPR_TEX_FILE_MAX; i++)
        m_tex_rec[i] = NULL;
    m_pfh = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0018f6dc

int SprTexture::getSlot(int id) const
{
    int slot = m_slot[id];
    if (slot < 0)
        slot = -1;
    return slot;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0018f6f8

int SprTexture::getTexNo(int id) const
{
    int slot = getSlot(id);
    if (slot < 0)
        return -1;
    return m_tex_no[slot];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0018f734

void *SprTexture::getTexRecord(int id) const
{
    return m_tex_rec[id];
}

// ──────────────────────────────────────────────────────────────────────
// Forgets the entries' buffers (m_buf = NULL, the end entry's too) and
// picks the first step.
// ROR 0x0018f744

void SprTexture::setList(SprTexFile *list, const char *rpk)
{
    m_rpk = rpk;
    m_list = list;
    if (list) {
        int i = 0;
        for (;;) {
            m_list[i].m_buf = NULL;
            if (m_list[i].m_id < 0)
                break;
            i++;
        }
    }
    if (m_rpk == NULL) {
        m_pfh = NULL;
        setStep(5);
    } else if (m_mode == 0) {
        setStep(3);
    } else if (m_mode == 1) {
        setStep(1);
    }
    m_cur = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0018f808

void SprTexture::setListMode(SprTexFile *list, const char *rpk, int mode)
{
    m_mode = mode;
    setList(list, rpk);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0018f824

void SprTexture::release()
{
    for (int i = 0; i < m_count; i++) {
        if (m_tex_no[i] > 0) {
            m_tex_info->release(m_tex_no[i]);
            m_tex_no[i] = 0;
        }
    }
    if (m_list) {
        for (int i = 0;; i++) {
            SprTexFile *f = &m_list[i];
            if (f->m_buf) {
                if (f->m_flags & SPR_TEX_DECODED) {
                    utlMemFree(f->m_buf);
                } else {
                    if (!(f->m_flags & SPR_TEX_IN_PACK))
                        utlMemFree(f->m_buf);
                    else
                        f->m_flags &= ~SPR_TEX_IN_PACK;
                    f->m_flags &= ~SPR_TEX_DECODED;
                }
                f->m_buf = NULL;
            }
            if (f->m_id < 0)
                break;
        }
    }
    for (int i = 0; i < SPR_TEX_FILE_MAX; i++)
        m_slot[i] = -1;
    for (int i = 0; i < SPR_TEX_FILE_MAX; i++)
        m_tex_rec[i] = NULL;
    if (m_buf) {
        utlMemFree(m_buf);
        m_buf = NULL;
    }
    m_cur = 0;
    m_count = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0018f9cc

int SprTexture::freeFile(int id)
{
    int i = 0;
    if (m_list[0].m_id < 0)
        return 0;
    if (id != m_list[0].m_id) {
        do {
            i++;
            if (m_list[i].m_id < 0)
                return 0;
        } while (id != m_list[i].m_id);
    }
    SprTexFile *f = &m_list[i];
    if (f->m_id < 0)
        return 0;
    if (f->m_buf) {
        if ((f->m_flags & SPR_TEX_DECODED) || !(f->m_flags & SPR_TEX_IN_PACK))
            utlMemFree(f->m_buf);
        m_list[i].m_buf = NULL;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0018fab4

int SprTexture::releaseTex(int id)
{
    if (m_slot[id] < 0 || m_tex_no[m_slot[id]] <= 0)
        return 0;
    int i = 0;
    if (m_list[0].m_id < 0)
        return 0;
    if (id != m_list[0].m_id) {
        do {
            i++;
            if (m_list[i].m_id < 0)
                return 0;
        } while (id != m_list[i].m_id);
    }
    if (m_list[i].m_id < 0)
        return 0;
    m_tex_info->release(m_tex_no[m_slot[id]]);
    m_tex_no[m_slot[id]] = 0;
    m_tex_rec[id] = NULL;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// A file the list skipped (no SPR_TEX_LOAD) is taken from the pack here,
// decoded when it is RTCM data, then added by its texture name.
// callcheck: dbgAssertFailed (the assert handler, inside BedPfh::getFile's ROR_ASSERT)
// ROR 0x0018fbb0

int SprTexture::loadTex(int id)
{
    if (m_slot[id] < 0 || m_tex_no[m_slot[id]] != 0)
        return 0;
    int i = 0;
    if (m_list[0].m_id < 0)
        return 0;
    if (id != m_list[0].m_id) {
        do {
            i++;
            if (m_list[i].m_id < 0)
                return 0;
        } while (id != m_list[i].m_id);
    }
    SprTexFile *f = &m_list[i];
    if (f->m_id < 0)
        return 0;
    if (f->m_buf == NULL) {
        if (m_pfh == NULL)
            return 0;
        int n = m_pfh->getFileIndex(f->m_name);
        if (n < 0)
            return 0;
        f->m_size = m_pfh->getFileSize(n);
        if (f->m_size < 0)
            return 0;
        f->m_buf = m_pfh->getFile(n);
        if (f->m_buf == NULL)
            return 0;
        f->m_flags &= ~SPR_TEX_DECODED;
        if (isRtcm(f->m_buf) == 1) {
            f->m_buf = decodeRtcm(f->m_buf);
            if (f->m_buf == NULL)
                return 0;
            f->m_flags |= SPR_TEX_DECODED;
        }
    }
    m_tex_no[m_slot[id]] = m_tex_info->addData(f->m_tex_name, f->m_buf, 0, 1);
    if (m_tex_no[m_slot[id]] < 1) {
        m_tex_no[m_slot[id]] = 0;
        return 0;
    }
    m_tex_rec[id] = m_tex_info->getTexInfo(m_tex_no[m_slot[id]]);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0018fe8c

int SprTexture::getPackIndex(const char *theName) const
{
    if (m_pfh)
        return m_pfh->getFileIndex(theName);
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// One step of the load (see the top of the file). Every failure sets
// step 0, which returns -1 from then on.
// callcheck: dbgAssertFailed (the assert handler, inside BedPfh::getFile's ROR_ASSERT)
// ROR 0x0018febc

int SprTexture::update()
{
    int r;
    SprTexFile *f;

    switch (m_step) {
    case 0:
        return -1;

    case 1:     // mode 1: the pack's first 4 KB
        if (m_buf) {
            setStep(5);
            return 0;
        }
        m_pfh = NULL;
        m_buf_size = 0x1000;
        m_buf = g_utlMemory.allocDebug(0x1000, s_file, 425, 0, 2);
        if (m_buf) {
            startRead(m_rpk, m_buf, m_buf_size, 0);
            setStep(4);
            return 0;
        }
        break;

    case 2:
    case 4:     // the pack's read
        r = updateRead();
        if (r > 0) {
            setStep(5);
            m_pfh = &m_pfh_body;
            new (&m_pfh_body) BedPfh(m_buf);
            return 0;
        }
        if (r >= 0)
            return 0;
        break;

    case 3:     // mode 0: the whole pack
        if (m_buf) {
            setStep(5);
            return 0;
        }
        m_pfh = NULL;
        m_buf_size = fileGetSize(m_rpk);
        m_buf = g_utlMemory.allocDebug(m_buf_size, s_file, 466, 0, 2);
        if (m_buf) {
            startRead(m_rpk, m_buf, m_buf_size, 0);
            setStep(4);
            return 0;
        }
        break;

    case 5:     // the next entry
        if (m_count >= SPR_TEX_FILE_MAX)
            setStep(0);
        f = &m_list[m_cur];
        if (f->m_id < 0 || f->m_name[0] == '\0') {
            setStep(10);
            return 0;
        }
        if (f->m_id < SPR_TEX_FILE_MAX) {
            setStep(6);
            return 0;
        }
        break;

    case 6: {   // find the file and start its read
        f = &m_list[m_cur];
        if (f->m_buf) {
            utlMemFree(f->m_buf);
            m_list[m_cur].m_buf = NULL;
        }
        int n = getPackIndex(m_list[m_cur].m_name);
        if (n < 0) {
            // a loose file
            f->m_size = fileGetSize(f->m_name);
            if (f->m_size >= 0) {
                f->m_buf = g_utlMemory.allocDebug(f->m_size, s_file, 542, 0, 2);
                if (f->m_buf) {
                    startRead(f->m_name, f->m_buf, f->m_size, 0);
                    f->m_flags &= ~SPR_TEX_IN_PACK;
                    setStep(7);
                    goto read;
                }
            }
        } else {
            f->m_size = m_pfh->getFileSize(n);
            if (f->m_size >= 0) {
                if (m_mode != 0) {
                    // mode 1: read it from the pack on the disc
                    f->m_buf = g_utlMemory.allocDebug(f->m_size, s_file, 578, 0, 2);
                    startRead(m_rpk, f->m_buf, f->m_size, m_pfh->getFileOffset(n));
                    f->m_flags &= ~SPR_TEX_IN_PACK;
                    setStep(7);
                    return 0;
                }
                // mode 0: it is in the pack in memory
                f->m_buf = m_pfh->getFile(n);
                if (f->m_buf) {
                    f->m_flags |= SPR_TEX_IN_PACK;
                    setStep(8);
                    return 0;
                }
            }
        }
        break;
    }

    case 7:     // the file's read
    read:
        r = updateRead();
        if (r > 0) {
            setStep(8);
            return 0;
        }
        if (r >= 0)
            return 0;
        break;

    case 8:     // decode and add
        f = &m_list[m_cur];
        if (f->m_flags & SPR_TEX_LOAD) {
            if (isRtcm(f->m_buf) == 1) {
                void *p = decodeRtcm(f->m_buf);
                if (!(m_list[m_cur].m_flags & SPR_TEX_IN_PACK))
                    utlMemFree(m_list[m_cur].m_buf);
                m_list[m_cur].m_buf = p;
            }
        }
        m_slot[m_list[m_cur].m_id] = m_count;
        f = &m_list[m_cur];
        if (f->m_flags & SPR_TEX_LOAD)
            loadTex(f->m_id);
        else if (m_mode == 0)
            f->m_buf = NULL;    // the pack keeps it; loadTex finds it again
        setStep(9);
        // fall through

    case 9:
        m_cur++;
        m_count++;
        setStep(5);
        return 0;

    case 10:    // done
        if (m_ctx)
            m_tex_info->setTexTable(m_ctx);
        return 1;

    default:
        return 0;
    }

    setStep(0);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001905a0

void SprTexture::setStep(int step)
{
    m_step = step;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001905a8

void SprTexture::setReadStep(int step)
{
    m_read_step = step;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001905b0

int SprTexture::startRead(const char *theName, void *buf, int size, int offset)
{
    strcpy(m_read_name, theName);
    m_read_buf = buf;
    m_read_size = size;
    m_read_offset = offset;
    setReadStep(1);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// One step of the read: new reader, open, seek (when m_read_offset is set),
// read, close, delete. While the rx file thread works it waits a V-blank
// and keeps the sound going.
// callcheck: operator_new, FileReader, ~FileReader (new / delete of the reader; the destructor is inline in lib/unassigned.h)
// ROR 0x00190618

int SprTexture::updateRead()
{
    int r = 0;

    switch (m_read_step) {
    case 0:
        r = -1;
        break;

    case 1:
        m_reader = new FileReader;
        setReadStep(2);
        r = 0;
        break;

    case 2:
        if (m_reader->isReady(&m_ready_404, &m_fd)) {
            setReadStep(3);
            r = 0;
        }
        break;

    case 3:
        if (m_read_size < 0 || m_reader->openFile(m_read_name) != 0) {
            setReadStep(0);
            r = -1;
        } else {
            setReadStep(4);
            r = 0;
        }
        break;

    case 4:
        if (!m_reader->isReady(&m_ready_404, &m_fd))
            return 0;
        if (m_fd < 0)
            r = m_fd;   // the open failed; the steps go on and fail too
        if (m_read_offset != 0) {
            setReadStep(5);
            return r;
        }
        setReadStep(7);
        break;

    case 5:
        m_reader->seekFile(m_fd, m_read_offset, 0);
        setReadStep(6);
        r = 0;
        break;

    case 6:
        rxThSync(1);
        g_sndMan.update();
        if (!m_reader->isReady(&m_ready_404, &m_ready_fd))
            return 0;
        setReadStep(7);
        break;

    case 7:
        m_reader->readFile(m_fd, m_read_buf, m_read_size);
        setReadStep(8);
        r = 0;
        break;

    case 8:
        rxThSync(1);
        g_sndMan.update();
        if (m_reader->isReady(&m_ready_404, &m_ready_fd)) {
            setReadStep(9);
            r = 0;
        }
        break;

    case 9:
        m_reader->closeFile(m_fd);
        setReadStep(10);
        r = 0;
        break;

    case 10:
        if (m_reader->isReady(&m_ready_404, &m_ready_fd)) {
            setReadStep(11);
            r = 0;
        }
        break;

    case 11:
        r = 1;
        if (m_reader)
            delete m_reader;    // m_reader is left set, as in the ROM
        break;
    }
    return r;
}

// ──────────────────────────────────────────────────────────────────────
// -1 when p does not start with id; otherwise 0 when the byte after it is
// NUL, 1 when not.
// ROR 0x00190884

int SprTexture::compareId(const char *id, const char *p)
{
    while (*id != '\0') {
        if (*id != *p)
            return -1;
        id++;
        p++;
    }
    return *id != *p;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001908d0

int SprTexture::isRtcm(const void *p)
{
    if (p == NULL)
        return 0;
    return compareId("RTCM", (const char *)p) >= 0;
}

// ──────────────────────────────────────────────────────────────────────
// The decode gets the decoded size plus one unit. A file of only the
// header returns the buffer as allocated.
// ROR 0x00190904

void *SprTexture::decodeRtcm(const void *p)
{
    const SprRtcmHead *head = (const SprRtcmHead *)p;
    const u_char *src = (const u_char *)p + sizeof(SprRtcmHead);
    u_int done = 0;

    if (!isRtcm(p))
        return NULL;
    int unit = head->m_unit;
    int total = head->m_total;
    u_char *out = (u_char *)g_utlMemory.allocDebug(head->m_size + unit, s_file, 1067, 0, 2);
    if (out == NULL || total == (int)sizeof(SprRtcmHead))
        return out;

    u_char *dst = out;
    for (;;) {
        int kind = src[0];
        u_int count = (u_int)src[1] << 16 | (u_int)src[2] << 8 | (u_int)src[3];
        src += 4;
        done += 4;
        if (kind == 0) {
            // one unit, count times
            for (u_int n = 0; n < count; n++) {
                memcpy(dst, src, unit);
                dst += unit;
            }
            src += unit;
            done += unit;
        } else if (kind == 1) {
            // count units
            for (u_int n = 0; n < count; n++) {
                memcpy(dst, src, unit);
                dst += unit;
                src += unit;
                done += unit;
            }
        } else {
            return NULL;    // the buffer is not freed, as in the ROM
        }
        if ((u_int)(total - (int)sizeof(SprRtcmHead)) <= done)
            break;
    }
    return out;
}
