/* ==========================================================================
 *  snd/sndman.cpp
 *
 *  The sound manager (see snd/sndman.h): 0x17f0d0..0x1810f4, pinned in the
 *  sndman session (the asserts in updateStream name the file; RxMpeg's
 *  constructor at 0x1810f4 opens snd/rxmpeg.cpp). Stream slots read their
 *  file through an rx file ring and feed it to an rxSnd ADPCM stream; SE
 *  slots start voices of the loaded XVAB banks.
 * ======================================================================== */
#include "snd/sndman.h"

#include <stdlib.h>
#include <string.h>

#include "common/ror_assert.h"
#include "game/gamman.h"
#include "lib/unassigned.h"
#include "rx/rxfile.h"
#include "rx/rxsnd.h"
#include "rx/rxsys.h"
#include "utl/utlfile.h"

/* ---- data ---- */

/* The two stream rings (stream ids 0 and 1). */
static u_char s_streamRing0[SND_RING_SIZE];                     /* bss 0x668980 */
static u_char s_streamRing1[SND_RING_SIZE];                     /* bss 0x6a8d80 */

static const char s_xwsDir[] = "data/snd/xws/";                 /* rodata 0x2f3150 */

/* ---- construction ---- */

// ROR 0x0017f0d0
SndMan::SndMan()
{
    m_volR = 0x80;
    m_state = 0;
    m_idUsed[0] = 0;
    m_idUsed[1] = 0;
    m_volL = 0x80;
    clearSlots();
    for (int i = 0; i < SND_BANK_MAX; i++) {
        m_bank[i] = -1;
        m_bankData[i] = NULL;
    }
}

// ROR 0x0017f13c (GCC 2 destructor: only the deleting variant)
// callcheck: operator_delete (the deleting variant; the host compiler emits it)
SndMan::~SndMan()
{
}

// ROR 0x0017f164
void SndMan::init()
{
    m_state = 1;
    m_fadeState = 0;
    rxSndInit();
}

// ROR 0x0017f188
void SndMan::term()
{
    if (m_state != 1)
        return;

    rxSndTerm();
    rxSysTerm();
    for (int i = 0; i < SND_BANK_MAX; i++) {
        m_bank[i] = -1;
        m_bankData[i] = NULL;
    }
    clearSlots();
}

// ROR 0x0017f200
void SndMan::update()
{
    if (m_state != 1)
        return;

    updateSlots();
    g_rxMpeg.update();
    rxSndUpdate();
    g_rxMpeg.postUpdate();
    updateFade();
}

/* ---- the master volume fade ---- */

// ROR 0x0017f264
void SndMan::fadeIn(int frames)
{
    if (m_fadeState == 0) {
        m_fadeFrames = frames;
        m_fadeState = 2;
        m_curVolR = 0;
        m_fadeCount = 0;
        m_curVolL = 0;
        return;
    }
    if (m_fadeState < 0 || m_fadeState > 2)
        return;

    /* Already fading: continue from the current volume. */
    if (m_curVolL < 1 || m_volL < 1)
        m_fadeCount = 0;
    else
        m_fadeCount = (int)((float)frames / ((float)m_volL / (float)m_curVolL));
    m_fadeFrames = frames;
    m_fadeState = 2;
}

// ROR 0x0017f308
void SndMan::fadeOut(int frames)
{
    if (m_fadeState == 0) {
        m_fadeCount = frames;
        m_curVolR = m_volR;
        m_fadeState = 1;
        m_curVolL = m_volL;
        m_fadeFrames = frames;
        return;
    }
    if (m_fadeState < 0 || m_fadeState > 2)
        return;

    if (m_curVolL < 1 || m_volL < 1)
        m_fadeCount = 0;
    else
        m_fadeCount = (int)((float)frames / ((float)m_volL / (float)m_curVolL));
    m_fadeFrames = frames;
    m_fadeState = 1;
}

// ROR 0x0017f3b8
void SndMan::updateFade()
{
    if (m_fadeState == 1) {
        int count = m_fadeCount;
        if (count < 1) {
            m_curVolL = 0;
            m_curVolR = 0;
            m_fadeState = 0;
        } else {
            m_fadeCount = count - 1;
            m_curVolL = (int)(((float)m_volL / (float)m_fadeFrames) * (float)count);
            m_curVolR = (int)(((float)m_volR / (float)m_fadeFrames) * (float)count);
        }
        rxSndSetMasterVolume(m_curVolL, m_curVolR);
    } else if (m_fadeState == 2) {
        int count = m_fadeCount;
        float frames = (float)m_fadeFrames;
        if (count < m_fadeFrames) {
            m_fadeCount = count + 1;
            m_curVolL = (int)(((float)m_volL / frames) * (float)count);
            m_curVolR = (int)(((float)m_volR / frames) * (float)count);
        } else {
            m_curVolL = m_volL;
            m_curVolR = m_volR;
            m_fadeState = 0;
        }
        rxSndSetMasterVolume(m_curVolL, m_curVolR);
    }
}

/* ---- slots ---- */

// ROR 0x0017f514
void SndMan::clearSlot(SndInfo *info)
{
    info->m_state = -1;
    info->m_type = -1;
    info->m_id = -1;
    info->m_08 = -1;
    info->m_loop = -1;
}

// ROR 0x0017f530
void SndMan::clearSlots()
{
    for (int i = 0; i < SND_SLOT_MAX; i++)
        clearSlot(&m_info[i]);
}

// ROR 0x0017f584
SndInfo *SndMan::getSlot(int id)
{
    if (id < 0 || id >= SND_SLOT_MAX)
        return NULL;
    for (int i = 0; i < SND_SLOT_MAX; i++) {
        if (m_info[i].m_id == id)
            return &m_info[i];
    }
    return NULL;
}

// ROR 0x0017f5e8
void SndMan::stopAll(u_int mask)
{
    for (int id = 0; id < SND_SLOT_MAX; id++) {
        SndInfo *info = getSlot(id);
        if (info == NULL)
            continue;

        if ((mask & info->m_type) == SND_TYPE_STREAM) {
            info->m_state = SND_STATE_STOP;
        } else if ((mask & info->m_type) == SND_TYPE_SE) {
            info->m_state = SND_STATE_STOP;
            if (info->uniPtr.sndSe.m_voice >= 0)
                rxSndSeStop(info->uniPtr.sndSe.m_voice, 1);
            info->uniPtr.sndSe.m_voice = -1;
            setIdFree(info->m_id);
            freeSlot(info);
        }
    }
}

// ROR 0x0017f6b8
void SndMan::updateSlots()
{
    for (int id = 0; id < SND_SLOT_MAX; id++) {
        SndInfo *info = getSlot(id);
        if (info == NULL)
            continue;

        if (info->m_type == SND_TYPE_STREAM) {
            if (updateStream(info) < 0) {
                setIdFree(info->m_id);
                freeStreamSlot(info);
            }
        } else if (info->m_type == SND_TYPE_SE) {
            if (updateSe(info) < 0) {
                setIdFree(info->m_id);
                freeSlot(info);
            }
        }
    }
}

/* ---- banks by sound id / name ---- */

// ROR 0x0017f7ac
int SndMan::loadBankById(int sndId, int nowait)
{
    GamSndEntry *entry;

    if (!g_gamMan.m_sndMan.findSnd(sndId, &entry, 1))
        return -1;
    return loadBank(entry->m_bank, entry->m_name, nowait);
}

// ROR 0x0017f81c
int SndMan::finishBankById(int sndId)
{
    GamSndEntry *entry;

    if (!g_gamMan.m_sndMan.findSnd(sndId, &entry, 1))
        return -1;
    return finishBank(entry->m_bank);
}

// ROR 0x0017f878
void SndMan::loadBankByName(const char *name, int nowait)
{
    char num[3];

    num[0] = name[3];
    num[1] = name[4];
    num[2] = 0;
    m_curBank = atoi(num);
    loadBank(m_curBank, name, nowait);
}

// ROR 0x0017f8e4
void SndMan::finishCurBank()
{
    finishBank(m_curBank);
}

// ROR 0x0017f8fc
int SndMan::playSe(int sndId)
{
    GamSndMan *table = &g_gamMan.m_sndMan;
    GamSndEntry *entry;
    int id = (int)utlConvSndId((u_int)sndId);

    if (table->isSndBankLoaded(id)) {
        table->findSnd(id, &entry, 1);
        if (table->findSnd(id, &entry, 1))
            return playSeBank(entry->m_bank, entry->m_prog, entry->m_note);
    }
    return -1;
}

/* ---- ids ---- */

// ROR 0x0017f9ac
int SndMan::allocId(int kind)
{
    int i;

    if (kind == 0) {
        for (i = 0; (m_idUsed[0] >> i) & 1; ) {
            if (++i > 1)
                return -1;
        }
        return i;
    }
    if (kind == 1) {
        for (i = 2; i < 32; i++) {
            if (!((m_idUsed[0] >> i) & 1))
                return i;
        }
    }
    for (i = 0; i < 16; i++) {
        if (!((m_idUsed[1] >> i) & 1))
            return i + 32;
    }
    return -1;
}

// ROR 0x0017fa5c
int SndMan::checkId(int kind, int id)
{
    if (kind != 0)
        return -1;
    if ((m_idUsed[0] >> (id & 0x1f)) & 1)
        return -1;
    return id;
}

// ROR 0x0017fa90
void SndMan::setIdUsed(int id)
{
    u_int bit = 1u << (id & 0x1f);

    if (id < 32)
        m_idUsed[0] |= bit;
    else
        m_idUsed[1] |= bit;
}

// ROR 0x0017fac4
void SndMan::setIdFree(int id)
{
    u_int mask = ~(1u << (id & 0x1f));

    if (id < 32)
        m_idUsed[0] &= mask;
    else
        m_idUsed[1] &= mask;
}

// ROR 0x0017fb00
void SndMan::initStreamSlot(SndInfo *info)
{
    SndFStream *s = &info->uniPtr.sndFStream;

    s->fStream = -1;
    info->m_state = SND_STATE_PAUSED;
    s->m_mode = 1;
    s->m_volR = 0x80;
    s->m_pitch = 1.0f;
    s->m_name[0] = 0;
    info->m_type = SND_TYPE_STREAM;
    info->m_loop = 0;
    s->m_14 = -1;
    s->fd = -1;
    s->m_1c = -1;
    s->m_read = 0;
    s->m_volL = 0x80;
    s->m_attr = 0;
    s->m_size = 0;
    s->m_ring = NULL;
}

// ROR 0x0017fb5c
SndInfo *SndMan::findFreeSlot()
{
    for (int i = 0; i < SND_SLOT_MAX; i++) {
        if (m_info[i].m_id == -1)
            return &m_info[i];
    }
    return NULL;
}

// ROR 0x0017fba4
void SndMan::freeSlot(SndInfo *info)
{
    if (info != NULL)
        clearSlot(info);
}

// ROR 0x0017fbc4
SndInfo *SndMan::allocStreamSlot(int id)
{
    SndInfo *info = findFreeSlot();

    if (info != NULL) {
        initStreamSlot(info);
        if (id == 0)
            info->uniPtr.sndFStream.m_ring = s_streamRing0;
        else
            info->uniPtr.sndFStream.m_ring = s_streamRing1;

        if (info->uniPtr.sndFStream.m_ring == NULL) {
            freeSlot(info);
            info = NULL;
        } else {
            memset(info->uniPtr.sndFStream.m_ring, 0, SND_RING_SIZE);
        }
    }
    return info;
}

// ROR 0x0017fc74
void SndMan::freeStreamSlot(SndInfo *info)
{
    freeSlot(info);
}

// ROR 0x0017fc90
int SndMan::countStreams()
{
    int count = 0;

    for (int id = 0; id < SND_SLOT_MAX; id++) {
        if ((m_idUsed[id >> 5] >> (id & 0x1f)) & 1) {
            SndInfo *info = getSlot(id);
            if (info != NULL && info->m_type == SND_TYPE_STREAM)
                count++;
        }
    }
    return count;
}

/* ---- streams ---- */

// ROR 0x0017fd1c
int SndMan::playStreamStereo(const char *name)
{
    return playStream(name, 1, -1, 0);
}

// ROR 0x0017fd3c
int SndMan::playStreamAuto(const char *name, int mode)
{
    return playStream(name, mode, -1, 0);
}

// ROR 0x0017fd58
// callcheck: operator_new (the new expression)
int SndMan::playStream(const char *name, int mode, int id, int arg)
{
    SndInfo *info;
    int reuse = 0;
    int sid;

    if (id == -1) {
        sid = allocId(0);
        if (sid < 0) {
            id = 1;
            reuse = 1;
            sid = id;
        }
    } else {
        sid = checkId(0, id);
        if (sid < 0) {
            reuse = 1;
            sid = id;
        }
    }

    if (reuse) {
        /* The id is taken: restart it, or replace what it plays. */
        info = getSlot(sid);
        if (info == NULL)
            return -1;

        SndFStream *s = &info->uniPtr.sndFStream;
        if (strcmp(s->m_name, name) == 0) {
            rxFileClose(s->fd);
            rxSndAdpcmClose(s->fStream);
            s->fStream = -1;
            s->m_read = 0;
        } else if (s->fStream != -1) {
            /* Stop the old stream; a TskSnd plays the new one after it. */
            TskSndParam param;
            memset(&param, 0, sizeof(param));

            stopStream(sid);
            param.m_name = name;
            param.m_mode = mode;
            param.m_id = sid;
            param.m_arg = arg;
            param.m_id2 = sid;
            if (g_taskMan.getTask(m_taskId) != 0)
                return sid;

            TskSnd *task = new TskSnd(7, &param);
            g_taskMan.addTask(1, -1, 1, 0, task, 0, &m_taskId, 0x11);
            return sid;
        }
    } else {
        info = allocStreamSlot(sid);
        if (info == NULL)
            return -1;
    }

    SndFStream *s = &info->uniPtr.sndFStream;
    if (mode == 0)
        s->m_mode = 0;
    else if (mode == 1)
        s->m_mode = 2;

    if (strlen(name) < sizeof(s->m_name))
        strcpy(s->m_name, name);
    else
        s->m_name[0] = 0;

    setIdUsed(sid);
    info->m_id = sid;
    info->m_state = SND_STATE_START;
    return sid;
}

// ROR 0x0017ff58
int SndMan::setStream(int id, int volL, int volR, float pitch, int attr)
{
    SndInfo *info = getSlot(id);

    if (info == NULL || info->m_type != SND_TYPE_STREAM)
        return -1;

    SndFStream *s = &info->uniPtr.sndFStream;
    if (volL != SND_KEEP)
        s->m_volL = volL;
    if (volR != SND_KEEP)
        s->m_volR = volR;
    if (pitch != -1024.0f)
        s->m_pitch = pitch;
    if (attr != SND_KEEP)
        s->m_attr = attr;

    if (info->m_state != SND_STATE_START) {
        if (info->m_state != SND_STATE_PLAY)
            return 0;
        info->m_state = SND_STATE_SET;
    }
    return 0;
}

// ROR 0x0018000c
int SndMan::setStreamAttr(int id, int attr)
{
    SndInfo *info = getSlot(id);

    if (info == NULL || info->m_type != SND_TYPE_STREAM)
        return -1;

    if (attr != SND_KEEP)
        info->uniPtr.sndFStream.m_attr = attr;

    if (info->m_state != SND_STATE_START) {
        if (info->m_state != SND_STATE_PLAY)
            return 0;
        info->m_state = SND_STATE_SET;
    }
    return 0;
}

// ROR 0x00180078
int SndMan::setStreamLoop(int id, int loop)
{
    SndInfo *info = getSlot(id);

    if (info == NULL || info->m_type != SND_TYPE_STREAM)
        return -1;

    if (loop == 0)
        info->m_loop = 0;
    else if (loop == 1)
        info->m_loop = 1;
    return 0;
}

// ROR 0x001800d0
int SndMan::getStreamState(int id)
{
    SndInfo *info = getSlot(id);

    if (info == NULL || info->m_type != SND_TYPE_STREAM)
        return -1;
    return info->m_state;
}

// ROR 0x00180108
int SndMan::pauseStream(int id)
{
    SndInfo *info = getSlot(id);

    if (info == NULL || info->m_type != SND_TYPE_STREAM)
        return -1;
    if (info->m_state != SND_STATE_PLAY || info->uniPtr.sndFStream.fd < 0)
        return -1;
    info->m_state = SND_STATE_PAUSE;
    return 0;
}

// ROR 0x00180168
int SndMan::resumeStream(int id)
{
    SndInfo *info = getSlot(id);

    if (info == NULL || info->m_type != SND_TYPE_STREAM)
        return -1;
    if (info->m_state != SND_STATE_PAUSED || info->uniPtr.sndFStream.fd < 0)
        return -1;
    info->m_state = SND_STATE_RESUME;
    return 0;
}

// ROR 0x001801c8
int SndMan::stopStream(int id)
{
    SndInfo *info = getSlot(id);

    if (info == NULL || info->m_type != SND_TYPE_STREAM)
        return -1;
    if (info->m_state != SND_STATE_PLAY && info->m_state != SND_STATE_PAUSED)
        return -1;
    if (info->uniPtr.sndFStream.fd < 0)
        return -1;
    info->m_state = SND_STATE_STOP;
    return 0;
}

/* How much of the ring the stream can take now: the filled bytes up to
 * the ring's end, no more than the stream has room for. */
static int sndStreamChunk(SndFStream *s)
{
    int size = rxFileGetRingFilled(s->fd);
    int contig = rxFileGetRingSize(s->fd) - rxFileGetRingReadPos(s->fd);

    if (contig < size)
        size = contig;
    if (size < 0)
        size = 0;

    int room = rxSndAdpcmGetFree(s->fStream);
    if (room <= size)
        size = room;
    if (size < 0)
        size = 0;
    return size;
}

// ROR 0x00180230
// callcheck: FUN_002155f4, rxFileGetRingFilled, rxFileGetRingReadPos, rxFileGetRingSize, rxSndAdpcmGetFree (the second assert handler is ROR_ASSERT; the ring and room reads are in sndStreamChunk, which the ROM inlines twice)
int SndMan::updateStream(SndInfo *strSndInfo)
{
    SndFStream *s = &strSndInfo->uniPtr.sndFStream;
    char path[0x60];
    int state = strSndInfo->m_state;

    if ((state > SND_STATE_RESTART || state < 0) && s->fStream < 0)
        return -1;

    switch (state) {
    case SND_STATE_RESTART:
        strSndInfo->m_state = SND_STATE_START;
        /* fall through */
    case SND_STATE_START:
        s->m_read = 0;
        utlStrCopy2(path, sizeof(path), s_xwsDir, s->m_name);
        s->m_size = rxFileCallSizeHook(path);
        if (s->m_size < 0)
            return -1;
        s->fd = rxSysStreamOpenRing(path, s->m_ring, SND_RING_SIZE, 0x20000);
        if (s->fd < 0)
            return -1;
        if (strSndInfo->m_loop == 1) {
            /* A looping stream: a marker at the end jumps back to 0. */
            RxFileMarkerDef marker;
            marker.pos = s->m_size;
            marker.active = 1;
            marker.id = 0;
            marker.count = 0;
            marker.max = 0;
            marker.jumpPos = 0;
            marker.callback = NULL;
            marker.user = 0;
            rxFileSetMarkers(s->fd, &marker, 1, NULL);
        }
        rxFilePause(s->fd, 0);
        s->fStream = rxSndAdpcmOpen(s->m_mode, 48000, 0, 4, 0, 4);
        if (s->fStream < 0) {
            rxFileClose(s->fd);
            return -1;
        }
        rxSndAdpcmSetVolume(s->fStream, s->m_volL, s->m_volR);
        rxSndAdpcmSetPitch(s->fStream, s->m_pitch);
        rxSndAdpcmSetAttr(s->fStream, s->m_attr);
        strSndInfo->m_state = SND_STATE_PLAY;
        break;

    case SND_STATE_PLAY:
    case SND_STATE_PAUSED:
    case 8:
    case 9:
        break;

    case SND_STATE_SET:
        ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fStream);
        strSndInfo->m_state = SND_STATE_PLAY;
        rxSndAdpcmSetVolume(s->fStream, s->m_volL, s->m_volR);
        rxSndAdpcmSetPitch(s->fStream, s->m_pitch);
        rxSndAdpcmSetAttr(s->fStream, s->m_attr);
        break;

    case SND_STATE_PAUSE:
        ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fStream);
        strSndInfo->m_state = SND_STATE_PAUSED;
        if (!(rxSndAdpcmGetStatus(s->fStream) & 8))
            rxSndAdpcmPause(s->fStream, 1);
        break;

    case SND_STATE_RESUME:
        ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fStream);
        strSndInfo->m_state = SND_STATE_PLAY;
        if (rxSndAdpcmGetStatus(s->fStream) & 8)
            rxSndAdpcmPause(s->fStream, 0);
        break;

    case SND_STATE_STOP:
        ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fStream);
        ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fd);
        rxFileClose(s->fd);
        rxSndAdpcmClose(s->fStream);
        s->fStream = -1;
        return -1;

    case SND_STATE_CLOSE:
        ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fStream);
        ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fd);
        rxFileClose(s->fd);
        rxSndAdpcmClose(s->fStream);
        strSndInfo->m_state = SND_STATE_RESTART;
        return -1;

    default:
        break;
    }

    ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fStream);
    ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fd);

    if (strSndInfo->m_loop == 0) {
        if (strSndInfo->m_state != SND_STATE_PLAY)
            return 0;

        if (!(rxSndAdpcmGetStatus(s->fStream) & 4)) {
            /* Two chunks a frame; the end of the file ends the stream. */
            for (int i = 1; i >= 0; i--) {
                ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fStream);
                ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fd);
                int size = sndStreamChunk(s);
                if (size > 0) {
                    rxSndAdpcmWrite(s->fStream, rxFileGetRingData(s->fd), 0, size);
                    rxFileRingConsume(s->fd, size);
                    s->m_read += size;
                    if (s->m_read >= s->m_size)
                        rxSndAdpcmEnd(s->fStream);
                }
            }
            return 0;
        }

        /* Played out. */
        ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fStream);
        ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fd);
        rxFileClose(s->fd);
        rxSndAdpcmClose(s->fStream);
        s->fStream = -1;
        s->m_read = 0;
        if (strSndInfo->m_loop == 0)
            return -1;
        if (strSndInfo->m_loop == 1)
            strSndInfo->m_state = SND_STATE_RESTART;
        return 0;
    }

    if (strSndInfo->m_loop == 1 && strSndInfo->m_state == SND_STATE_PLAY &&
        !(rxSndAdpcmGetStatus(s->fStream) & 4)) {
        /* Looping: the file's end marker wraps the ring, so just feed it. */
        for (int i = 0; i < 2; i++) {
            ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fStream);
            ROR_ASSERT(0<=strSndInfo->uniPtr.sndFStream.fd);
            int size = sndStreamChunk(s);
            if (size > 0) {
                rxSndAdpcmWrite(s->fStream, rxFileGetRingData(s->fd), 0, size);
                rxFileRingConsume(s->fd, size);
            }
        }
    }
    return 0;
}

/* ---- banks ---- */

// ROR 0x001809d8
int SndMan::loadBank(int bank, const void *xvab, int nowait)
{
    if (bank >= SND_BANK_MAX)
        return -1;

    if (m_bank[bank] >= 0)
        freeBank(bank);
    int id = rxSndXvabLoad(-1, xvab, 0, 0);
    m_bank[bank] = id;
    if (id < 0)
        return -1;
    if (nowait == 0)
        rxSndXvabCheck(id, 1);
    return id;
}

// ROR 0x00180a90
int SndMan::finishBank(int bank)
{
    if (bank >= SND_BANK_MAX)
        return -1;
    if (m_bank[bank] < 0 || m_bankData[bank] == NULL)
        return -1;

    if (rxSndXvabCheck(m_bank[bank], 0) == 1)
        return 0;       /* still transferring */

    /* rx memory hook 3 is the free function (main installs utlMemFree). */
    ((void (*)(void *))g_rxSysMemFunc[3])(m_bankData[bank]);
    m_bankData[bank] = NULL;
    return 1;
}

// ROR 0x00180b30
int SndMan::freeBank(int bank)
{
    if (bank >= SND_BANK_MAX || m_bank[bank] < 0)
        return -1;

    rxSndXvabFree(m_bank[bank], 1);
    m_bank[bank] = -1;
    return 0;
}

/* ---- sound effects ---- */

// ROR 0x00180b9c
void SndMan::initSeSlot(SndInfo *info)
{
    SndSe *se = &info->uniPtr.sndSe;

    se->m_pitch = 1.0f;
    info->m_type = SND_TYPE_SE;
    se->m_bank = SND_BANK_MAX;
    se->m_note = -1;
    se->m_volR = 0x80;
    info->m_loop = 0;
    info->m_state = SND_STATE_START;
    se->m_voice = -1;
    se->m_prog = -1;
    se->m_volL = 0x80;
    se->m_attr = 0;
}

// ROR 0x00180be4
int SndMan::playSeBank(int bank, int prog, int note)
{
    if (m_bank[bank] < 0)
        return -1;

    int id = allocId(1);
    if (id < 0)
        return -1;
    SndInfo *info = findFreeSlot();
    if (info == NULL)
        return -1;

    initSeSlot(info);
    info->uniPtr.sndSe.m_bank = bank;
    info->uniPtr.sndSe.m_prog = prog;
    info->uniPtr.sndSe.m_note = note;
    setIdUsed(id);
    info->m_id = id;
    return id;
}

// ROR 0x00180ca8
int SndMan::getSeState(int id)
{
    SndInfo *info = getSlot(id);

    if (info == NULL || info->m_type != SND_TYPE_SE)
        return -1;
    return info->m_state;
}

// ROR 0x00180ce0
int SndMan::setSe(int id, int volL, int volR, float pitch, int attr)
{
    SndInfo *info = getSlot(id);

    if (info == NULL || info->m_type != SND_TYPE_SE)
        return -1;

    SndSe *se = &info->uniPtr.sndSe;
    if (volL != SND_KEEP)
        se->m_volL = volL;
    if (volR != SND_KEEP)
        se->m_volR = volR;
    if (pitch != -1024.0f)
        se->m_pitch = pitch;
    if (attr != SND_KEEP)
        se->m_attr = attr;

    if (info->m_state != SND_STATE_START) {
        if (info->m_state != SND_STATE_PLAY)
            return 0;
        info->m_state = SND_STATE_SET;
    }
    return 0;
}

// ROR 0x00180d94
int SndMan::setSeAttr(int id, int attr)
{
    SndInfo *info = getSlot(id);

    if (info == NULL || info->m_type != SND_TYPE_SE)
        return -1;

    if (attr != SND_KEEP)
        info->uniPtr.sndSe.m_attr = attr;

    if (info->m_state != SND_STATE_START) {
        if (info->m_state != SND_STATE_PLAY)
            return 0;
        info->m_state = SND_STATE_SET;
    }
    return 0;
}

// ROR 0x00180e00
int SndMan::pauseSe(int id)
{
    SndInfo *info = getSlot(id);

    if (info == NULL || info->m_type != SND_TYPE_SE)
        return -1;
    if (info->m_state == SND_STATE_PLAY)
        info->m_state = SND_STATE_PAUSE;
    return 0;
}

// ROR 0x00180e54
int SndMan::resumeSe(int id)
{
    SndInfo *info = getSlot(id);

    if (info == NULL || info->m_type != SND_TYPE_SE)
        return -1;
    if (info->m_state == SND_STATE_PAUSED)
        info->m_state = SND_STATE_RESUME;
    return 0;
}

// ROR 0x00180ea8
int SndMan::stopSe(int id)
{
    SndInfo *info = getSlot(id);

    if (info == NULL || info->m_type != SND_TYPE_SE)
        return -1;
    info->m_state = SND_STATE_STOP;
    return 0;
}

// ROR 0x00180eec
int SndMan::setSeLoop(int id, int loop)
{
    SndInfo *info = getSlot(id);

    if (info == NULL || info->m_type != SND_TYPE_SE)
        return -1;
    info->m_loop = (loop != 0) ? 1 : 0;
    return 0;
}

// ROR 0x00180f44
int SndMan::updateSe(SndInfo *info)
{
    SndSe *se = &info->uniPtr.sndSe;
    int ret = 0;

    if (se->m_voice < 0) {
        if (info->m_state == SND_STATE_START) {
            se->m_voice = rxSndSePlay(m_bank[se->m_bank], se->m_prog, se->m_note,
                                      se->m_volL, se->m_volR, 1);
            rxSndSeSetPitch(se->m_voice, se->m_pitch);
            rxSndSeSetAttr(se->m_voice, se->m_attr);
            info->m_state = SND_STATE_PLAY;
        }
        if (se->m_voice < 0)
            return 0;
    }

    switch (info->m_state) {
    case SND_STATE_SET:
        rxSndSeSetVolume(se->m_voice, se->m_volL, se->m_volR);
        rxSndSeSetPitch(se->m_voice, se->m_pitch);
        rxSndSeSetAttr(se->m_voice, se->m_attr);
        info->m_state = SND_STATE_PLAY;
        break;

    case SND_STATE_PAUSE:
        info->m_state = SND_STATE_PAUSED;
        if (rxSndSeGetStatus(se->m_voice) != 8)
            rxSndSePause(se->m_voice, 1, 0);
        break;

    case SND_STATE_RESUME:
        info->m_state = SND_STATE_PLAY;
        if (rxSndSeGetStatus(se->m_voice) == 8)
            rxSndSePause(se->m_voice, 0, 0);
        break;

    case SND_STATE_STOP:
        if (se->m_voice >= 0)
            rxSndSeStop(se->m_voice, 1);
        ret = -1;
        se->m_voice = -1;
        break;
    }

    /* Played out: free the slot, or start again when looping. */
    if (se->m_voice >= 0 && (rxSndSeGetStatus(se->m_voice) & 4)) {
        rxSndSeStop(se->m_voice, 1);
        se->m_voice = -1;
        if (info->m_loop == 0)
            ret = -1;
        else if (info->m_loop == 1)
            info->m_state = SND_STATE_START;
    }
    return ret;
}
