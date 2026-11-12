/* ==========================================================================
 *  snd/rxmpeg.cpp
 *
 *  The movie player (see snd/rxmpeg.h): 0x1810f4..0x1830fc, pinned in its
 *  first session (rxMpegCreate's asserts name the file; snd/sndman.cpp
 *  ends and snd/unitque.cpp starts at the edges). The RxMpeg methods come
 *  first, then the library under them.
 *
 *  PORT: the host has no IPU. The library runs as written over the host's
 *  libmpeg (src/sdk/libmpeg.cpp: a real PSS demuxer, FFmpeg's MPEG-2
 *  decoder behind sceMpegGetPicture), with three differences, each marked
 *  PC: where it is made --
 *  - the decoder is fed by the demuxer itself, so the video que is only a
 *    gauge: the Nodata callback retires what the decoder has parsed
 *    (RosePanMpegVideoPending) instead of starting an IPU DMA, which keeps
 *    the demux, and so the movie, at the film's own pace;
 *  - the decode loop rxMpegDecodeThread is not posted to an rx thread
 *    (host rx threads run alongside the main thread, and the decoder is not
 *    thread safe against the demuxer): rxMpegUpdate runs its one-pass twin
 *    rxMpegDecodePoll each frame;
 *  - the picture reaches the screen through the native i3d module
 *    (RosePan_I3dRecordMovie: the renderer's movie texture, rosepan_video.h)
 *    instead of RxMpeg::draw's GS image transfer, which is still written.
 * ======================================================================== */
#include "snd/rxmpeg.h"

#include <string.h>

#include "common/ror_assert.h"
#include "game/gamfile.h"
#include "game/gamman.h"
#include "i3d/i3dlayer.h"
#include "i3d/i3dpacket.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "rosepan/rendering/rosepan_i3d.h"
#include "rosepan/rendering/rosepan_video.h"
#include "rx/rxfile.h"
#include "rx/rxsnd.h"
#include "rx/rxsys.h"
#include "rx/rxthread.h"
#include "sdk/eekernel.h"
#include "sdk/libmpeg_video.h"
#include "sequence/sequence.h"
#include "snd/unitque.h"
#include "utl/utlfile.h"
#include "utl/utlmisc.h"

RxMpeg g_rxMpeg;                /* bss 0x7b40c0 */

/* ---- the library's data ---- */

static void    *gBuffer;                /* data 0x298f00: the file ring (0x60040, 64-aligned in use) */
static int      s_fd = -1;              /* data 0x298f04: the ring's fd, -1 none */
static int      s_left;                 /* data 0x298f08: file bytes not demuxed yet */
static int      s_size;                 /* data 0x298f0c: the file's size */
static int      s_volume = 0x80;        /* data 0x298f10: rxMpegSetVolume; nothing reads it */
static int      s_pause;                /* data 0x298f14: holds the shown picture and the audio */
static int      s_status = 1;           /* data 0x298f18: 1 idle / finished, 0 playing */
static int      s_frames;               /* data 0x298f1c: pictures shown */
static void    *gMpegWork;              /* data 0x298f20: the decoder's work area */
static int      s_dmaRun = 1;           /* data 0x298f24: the IPU DMA is not stopped */
static int      s_dmaSize;              /* data 0x298f28: the bytes the last IPU DMA took */
static UnitQue *s_videoQue;             /* data 0x298f2c: the video elementary stream */
static int      s_created;              /* data 0x298f30: rxMpegCreate ran */
static int      s_flag34;               /* data 0x298f34: rxMpegSetFlag34; nothing reads it */
static int      s_run;                  /* data 0x298f38: 1 demuxes, 0 holds the primed ring */
static int      s_ready;                /* data 0x298f3c: the ring is primed */
static void    *voData;                 /* data 0x298f40: the two pictures */
static int      s_quit;                 /* data 0x298f44: ends rxMpegDecodeThread */
static int      s_started;              /* data 0x298f48: the first picture came; the audio is open */
static int      s_flag4c;               /* data 0x298f4c: rxMpegSetFlag4c; nothing reads it */
static int      s_50;                   /* data 0x298f50: rxMpegInit clears it */
static int      s_nearEnd;              /* data 0x298f54: fewer than 0x2800 bytes left */
static int      s_width = 0x200;        /* data 0x298f58: the largest picture */
static int      s_height = 0x180;       /* data 0x298f5c */
static int      s_getCount;             /* data 0x298f60: sceMpegGetPicture calls (the thread's) */
static int      s_decoding;             /* data 0x298f64: inside the decode loop */
static u_int    s_vcount;               /* data 0x298fd0: rxMpegSetVCount */
static int      s_demuxed;              /* data 0x298fd4: rxMpegSetDemuxed */
static int      s_threadPic;            /* data 0x298fd8: the thread's last sceMpegGetPicture result */
static int      s_pollPic;              /* data 0x298fdc: rxMpegDecodePoll's */
static int      s_userWord;             /* data 0x298fe0: rxMpegSetUserWord */

static sceMpeg  s_mpeg;                 /* bss 0x6e9200 (the IPU DMA environment follows at 0x6e9248) */
static int      s_field;                /* bss 0x6e926c: toggles every frame; a picture is shown when 0 */
static int      s_field2;               /* bss 0x6e9270 */
static int      s_flipped;              /* bss 0x6e9274: set by the first flip */
static int      s_decodeIdx;            /* bss 0x6e9278: the picture decoded into */
static int      s_showIdx;              /* bss 0x6e927c: (s_showIdx + 1) % 2 is shown */
static int      s_free = 2;             /* bss 0x6e9280: pictures free to decode into */
static RxMpegAudio s_audio[1];          /* bss 0x6e9288 */

/* The end code the IPU is sent when the file is spent (data 0x298fc0). */
static u_int s_endCode[4] = {0xb7010000, 0xb7010000, 0xb7010000, 0xb7010000};

/* The audio stream tables, PCM then ADPCM (data 0x298f68..0x298f94). */
typedef int  (*RxMpegOpenFunc)(int, int, int, int, int, int);
typedef void (*RxMpegCloseFunc)(int);
typedef int  (*RxMpegGetFreeFunc)(int);
typedef int  (*RxMpegWriteFunc)(int, const void *, const void *, int);
typedef void (*RxMpegPauseFunc)(int, int);
typedef void (*RxMpegVolumeFunc)(int, int, int);
static const RxMpegOpenFunc    s_audioOpen[2]    = {rxMpegPcmOpen, rxMpegAdpcmOpen};
static const RxMpegCloseFunc   s_audioClose[2]   = {rxSndPcmClose, rxSndAdpcmClose};
static const RxMpegGetFreeFunc s_audioGetFree[2] = {rxSndPcmGetFree, rxSndAdpcmGetFree};
static const RxMpegWriteFunc   s_audioWrite[2]   = {rxSndPcmWrite, rxSndAdpcmWrite};
static const RxMpegPauseFunc   s_audioPause[2]   = {rxSndPcmPause, rxSndAdpcmPause};
static const RxMpegVolumeFunc  s_audioVolume[2]  = {rxSndPcmSetVolume, rxSndAdpcmSetVolume};

static const char s_movieDir[] = "movie/";                       /* rodata 0x2f3258 */

/* PC: the key the movie picture is published under (rosepan_video.h): the
 * ROM's frame buffer base for a 512-wide movie. */
static const int kMovieVideoAddr = 0xe00;

static unsigned char *align64(void *p)
{
    return (unsigned char *)(((uintptr_t)p + 0x3f) & ~(uintptr_t)0x3f);
}

/* ---- RxMpeg ---- */

// ROR 0x001810f4
RxMpeg::RxMpeg()
{
    m_1c = 0x80;
    m_height = 0x180;
    m_28 = 0xe00;
    m_width = 0x200;
    m_active = 0;
    m_playing = 0;
    m_volume = 0x80;
    m_18 = 0x80;
}

// ROR 0x00181128 (GCC 2 destructor: only the deleting variant)
// callcheck: operator_delete (the deleting variant; the host compiler emits it)
RxMpeg::~RxMpeg()
{
}

// ROR 0x00181150
void RxMpeg::open()
{
    m_active = 1;
    m_28 = 0xe00;
    m_playing = 0;
    m_hold = 0;
    m_08 = 0;
    rxMpegInit();
    rxMpegSetFlag34(0);
    rxMpegDummy(0);
    rxMpegSetFlag4c(0);
    m_24 = 0;
    m_fadedOut = 0;
}

// ROR 0x001811d0
void RxMpeg::setUserWord(int word)
{
    m_28 = 0xe00;
    rxMpegSetUserWord(word);
}

// ROR 0x00181204
void RxMpeg::setFrameParity(int parity)
{
    m_parity = parity;
}

// ROR 0x0018120c
void RxMpeg::update()
{
    if (m_active != 1)
        return;
    if (m_hold != 0) {
        if (m_holdAt < 0) {
            rxMpegSetPause(1);
        } else if (rxMpegGetFrameCount() == m_holdAt) {
            rxMpegSetPause(0);
        } else {
            rxMpegSetPause(1);
            m_holdAt = -1;
        }
    }
    rxMpegSetVolume(m_volume);
    rxMpegUpdate();
}

// ROR 0x001812a8
// The shown picture into the frame buffer: BITBLTBUF (frame buffer 0xe00,
// 0x12c0 past 512 wide, 0 on an odd frame; 64 lines down for a picture
// under 448 lines), TRXREG 16 x height, then per 16-pixel column TRXPOS,
// TRXDIR and an IMAGE REF of the column's macroblocks.
void RxMpeg::draw(I3dLayer *layer)
{
    int width = m_width;
    int height = rxMpegGetHeight();
    if (height == 0)
        return;
    int base = width <= 0x200 ? 0xe00 : 0x12c0;
    u_long dbp = (u_long)base;
    if (m_parity != 0)
        dbp = 0;
    unsigned char *src = rxMpegGetPicture();
    getStatus();
    m_picture = (int)(intptr_t)src;
    int blocks = height / 16;
    u_long qwc = (u_long)(blocks << 6);
    u_long *pkt = i3dLayerBeginPacket(layer);
    pkt = i3dPktSetReg(pkt, 0x50, (u_long)(dbp + (height < 0x1c0 ? 0x200 : 0)) << 32 |
                                  (u_long)(width / 64) << 48);
    pkt = i3dPktSetReg(pkt, 0x52, (u_long)height << 32 | 0x10);
    for (int i = 0; i < width / 16; i++) {
        u_long *p = i3dPktDirectTag(pkt, 4, 0x10, NULL);
        if (i == 0) {
            // PC: the picture through the native module, keyed at this
            // column's tag: the published movie frame over the rectangle
            // the transfer writes.
            RosePan_VideoBegin(kMovieVideoAddr);
            int y0 = height < 0x1c0 ? 64 : 0;
            RosePan_I3dRecordMovie(p, 0, y0, width / 16 * 16, y0 + height, width, 448,
                                   s_mpeg.width, height);
        }
        p[0] = 0x1000000000008002;
        p[2] = (u_long)(i << 4) << 32;
        p[3] = 0x51;
        p[5] = 0x53;
        p[6] = qwc | 0x800000000008000;
        p[1] = 0xe;
        p[4] = 0;
        pkt = i3dPktDmaTag(p + 8, (u_int)(qwc & 0xffff), 0x30, src, 0, (u_int)(qwc | 0x50000000));
        src += blocks * 0x400;
    }
    i3dLayerEndPacket(layer, pkt);
}

// ROR 0x00181494
void RxMpeg::postUpdate()
{
}

// ROR 0x001814ac
int RxMpeg::playNow(const char *name, int w, int h)
{
    m_height = h;
    m_width = w;
    return startNow(name);
}

// ROR 0x001814cc
int RxMpeg::startNow(const char *name)
{
    int ret = -1;
    if (m_active == 1) {
        m_picture = 0;
        ret = -1;
        if (rxMpegGetStatus() != 0) {
            rxMpegPrepare();
            ret = rxMpegOpenNow(name);
            if (ret >= 0)
                m_playing = 1;
        }
    }
    return ret;
}

// ROR 0x0018154c
int RxMpeg::play(const char *name, int w, int h)
{
    m_height = h;
    m_width = w;
    return startPaused(name);
}

// ROR 0x0018156c
int RxMpeg::startPaused(const char *name)
{
    int ret = -1;
    if (m_active == 1) {
        g_gamMan.getStatus(0x10, &m_savedLevel);
        int level = m_savedLevel;
        g_gamMan.setOption(0, (5 - level) / 2 + level);
        m_picture = 0;
        ret = -1;
        do {
            if (rxMpegGetStatus() != 0) {
                rxMpegPrepare();
                ret = rxMpegOpenPaused(name);
                if (ret < 0) {
                    // PC: the ROM retries for ever; a movie missing from the
                    // data folder would hang the host here.
                    printf("RxMpeg: cannot open movie %s\n", name);
                    break;
                }
                m_playing = 1;
            }
        } while (ret < 0);
    }
    return ret;
}

// ROR 0x00181644
int RxMpeg::run()
{
    int ret = -1;
    if (m_playing == 1) {
        if ((g_sequence.m_bits[1] & 0x8000000) != 0)
            MagSystem::m_current_system->m_scene.fadeIn(0x1e, 7, 0);
        rxMpegRun();
        ret = 0;
    }
    return ret;
}

// ROR 0x001816b0
int RxMpeg::close()
{
    if (getStatus() < 0)
        return -1;
    m_playing = 0;
    m_active = 0;
    rxMpegClose();
    g_gamMan.setOption(0, m_savedLevel);
    m_width = 0x200;
    m_height = 0x180;
    return 0;
}

// ROR 0x0018173c
int RxMpeg::getStatus()
{
    if (m_active == 1)
        return rxMpegGetStatus();
    return -1;
}

// ROR 0x0018176c
int RxMpeg::isReady()
{
    return rxMpegIsReady();
}

// ROR 0x00181780
int RxMpeg::checkNearEnd()
{
    if (rxMpegIsNearEnd() != 0 && m_fadedOut == 0 &&
        (g_sequence.m_bits[1] & 0x8000000) != 0) {
        int fade = g_magSystem.m_scene.getFadeState();
        if (fade != 4 && fade != 6) {
            m_fadedOut = 1;
            MagSystem::m_current_system->m_scene.fadeOut(0x1e, 7, 0);
        }
    }
    return rxMpegIsNearEnd();
}

/* ---- the audio channel ---- */

// ROR 0x0018182c
int rxMpegPcmOpen(int stereo, int rate, int iopAddr, int iopKB, int spuAddr, int spuKB)
{
    return rxSndPcmOpen(stereo, iopAddr, iopKB);
}

// ROR 0x00181848
int rxMpegAdpcmOpen(int stereo, int rate, int iopAddr, int iopKB, int spuAddr, int spuKB)
{
    return rxSndAdpcmOpen(stereo, rate, iopAddr, iopKB, spuAddr, spuKB);
}

// ROR 0x0018185c
void rxMpegAudioInit(int no)
{
    RxMpegAudio *ch = &s_audio[no];
    ch->m_no = no;
    ch->m_type = -1;
    ch->m_volR = 0x80;
    ch->m_stream = -1;
    ch->m_volL = 0x80;
    ch->m_que = NULL;
    memset(ch->m_hdr, 0, 0x20);
}

// ROR 0x001818b4
void rxMpegAudioReset(int no)
{
    RxMpegAudio *ch = &s_audio[no];
    unitQueReset(ch->m_que);
    ch->m_no = no;
    ch->m_volR = 0x80;
    ch->m_volL = 0x80;
    ch->m_type = -1;
    ch->m_stream = -1;
    memset(ch->m_hdr, 0, 0x20);
}

// ROR 0x00181920
void rxMpegAudioOpen(int no)
{
    RxMpegAudio *ch = &s_audio[no];
    if (ch->m_stream < 0 && ch->m_type != -1)
        ch->m_stream = s_audioOpen[ch->m_type](ch->m_hdr[4] > 1, ch->m_hdr[3], 0, 8, 0, 8);
}

// ROR 0x00181998
void rxMpegAudioClose(int no)
{
    RxMpegAudio *ch = &s_audio[no];
    if (ch->m_stream >= 0) {
        s_audioClose[ch->m_type](ch->m_stream);
        ch->m_stream = -1;
    }
}

// ROR 0x001819f4
int rxMpegAudioIsOpen(int no)
{
    RxMpegAudio *ch = &s_audio[no];
    int open = 0;
    if (ch->m_stream >= 0)
        open = ch->m_type != -1;
    return open;
}

// ROR 0x00181a24
void rxMpegAudioCreate(int no)
{
    s_audio[no].m_que = unitQueCreate(0x20000);
    rxMpegAudioReset(no);
}

// ROR 0x00181a70
void rxMpegAudioDelete(int no)
{
    if (s_audio[no].m_que != NULL) {
        unitQueDelete(s_audio[no].m_que);
        s_audio[no].m_que = NULL;
    }
}

// ROR 0x00181ab4
void rxMpegAudioUpdate(int no)
{
    if (rxMpegAudioIsOpen(no) == 0)
        return;
    RxMpegAudio *ch = &s_audio[no];
    int interleave = ch->m_hdr[5];
    UnitQue *que = ch->m_que;
    int type = ch->m_type;
    int stream = ch->m_stream;
    int channels = ch->m_hdr[4];
    for (;;) {
        int unit = interleave * channels;
        if (unitQueGetCount(que) < unit)
            break;
        if (s_audioGetFree[type](stream) < unit)
            break;
        unsigned char *data = unitQueGetReadPtr(que);
        unsigned char *right = data + interleave;
        if (channels < 2)
            right = NULL;
        s_audioWrite[type](stream, data, right, unit);
        unitQueConsume(que, unit);
    }
    s_audioPause[type](stream, s_pause);
    s_audioVolume[type](stream, ch->m_volL, ch->m_volR);
}

// ROR 0x00181bf4
void rxMpegResetFrames(void)
{
    s_free = 2;
    s_decodeIdx = 0;
    s_showIdx = 0;
}

/* ---- the pictures and the decoder's callbacks ---- */

// ROR 0x00181c14
// callcheck: dbgAssertFailed, mcdDbgAssertFailed (the two assert handlers; ROR_ASSERT)
void rxMpegCreatePictures(void)
{
    s_free = 2;
    s_decodeIdx = 0;
    s_showIdx = 0;
    voData = g_utlMemory.allocDebug(s_width * s_height * 8 + 0x40, "snd/rxmpeg.cpp", 0x176, 0, 2);
    if (voData == NULL) {
        ROR_ASSERT(voData);     /* 0x178, the printing handler */
        ROR_ASSERT(voData);     /* 0x17c */
    }
}

// ROR 0x00181ce4
void rxMpegDeletePictures(void)
{
    utlMemFree(voData);
    voData = NULL;
}

// ROR 0x00181d54
int rxMpegVideoCallback(sceMpeg *mp, sceMpegCbData *data, void *que)
{
    int len = (int)data->str.len;
    const u_char *src = data->str.data;
    rxThDisableDispatch();
    int ok = len <= unitQueGetFree((UnitQue *)que);
    if (ok)
        unitQueWrite((UnitQue *)que, src, len);
    rxThEnableDispatch();
    return ok;
}

// ROR 0x00181dd4
int rxMpegAudioCallback(sceMpeg *mp, sceMpegCbData *data, void *no)
{
    RxMpegAudio *ch = &s_audio[(intptr_t)no];
    UnitQue *que = ch->m_que;
    const u_char *src = data->str.data;
    int len = (int)data->str.len - 4;
    const u_char *p = src + 4;
    if (que->m_user == 0) {
        /* The first payload opens with the SShd chunk (after the 4-byte
         * sub-stream header) and the SSbd tag. */
        memcpy(ch->m_hdr, src + 4, sizeof(ch->m_hdr));
        ch->m_type = ch->m_hdr[2] >= 0x10;
        que->m_user = 1;
        p = src + 0x2c;
        len = (int)data->str.len - 0x2c;
    }
    rxThDisableDispatch();
    int ok = len <= unitQueGetFree(que);
    if (ok)
        unitQueWrite(que, p, len);
    rxThEnableDispatch();
    return ok;
}

// ROR 0x00181eec
int rxMpegErrorCallback(sceMpeg *mp, sceMpegCbData *data, void *arg)
{
    return 1;
}

// ROR 0x00181ef4
// PORT: D_ENABLEW |= 0x10000, D4_CHCR = chcr, D_ENABLEW restored (the
// toIPU channel) -- there is no DMAC or IPU on the host.
void rxMpegSetIpuDma(int chcr)
{
    DIntr();
    (void)chcr;
    EIntr();
}

// ROR 0x00181f5c
// Retires the bytes the last DMA took and starts the next: whole 2 KB units
// of the que, up to 0x80000, or the end code once the file is spent.
int rxMpegNodataCallback(sceMpeg *mp, sceMpegCbData *data, void *arg)
{
    rxThDisableDispatch();
    UnitQue *que = s_videoQue;
    int ret = 0;
    if (s_dmaRun != 0) {
        unitQueConsume(que, s_dmaSize);
        int n = unitQueGetReadable(que) / 0x800 * 0x800;
        if (n > 0x80000)
            n = 0x80000;
        // PC: the decoder took the stream from the demuxer; only what it
        // has parsed leaves the que, so the que's room paces the demux.
        // Still whole 2 KB units: the read position must stay on them, or
        // a sliver under 0x800 before the wrap is never taken.
        int parsed = (unitQueGetCount(que) - RosePanMpegVideoPending()) / 0x800 * 0x800;
        if (n > parsed)
            n = parsed < 0 ? 0 : parsed;
        if (n < 1) {
            if (s_left < 5) {
                FlushCache(0);
                // PORT: D4_QWC 1, D4_MADR s_endCode (no IPU on the host)
                (void)s_endCode;
                rxMpegSetIpuDma(0x101);
            }
        } else {
            FlushCache(0);
            // PORT: D4_QWC n / 16, D4_MADR the que's read position
            (void)unitQueGetReadPtr(que);
            rxMpegSetIpuDma(0x101);
        }
        s_dmaSize = n;
        rxThEnableDispatch();
        ret = 1;
    }
    /* The ROM leaves dispatch disabled when the DMA is stopped (the host
     * libmpeg never stops it). */
    return ret;
}

// ROR 0x00182094
// callcheck: sceIpuStopDMA (no IPU on the host)
int rxMpegStopDmaCallback(sceMpeg *mp, sceMpegCbData *data, void *arg)
{
    rxThDisableDispatch();
    s_dmaRun = 0;
    rxThEnableDispatch();
    return 1;
}

// ROR 0x001820d0
// callcheck: sceIpuRestartDMA (no IPU on the host)
int rxMpegRestartDmaCallback(sceMpeg *mp, sceMpegCbData *data, void *arg)
{
    rxThDisableDispatch();
    s_dmaRun = 1;
    rxThEnableDispatch();
    return 1;
}

// ROR 0x0018210c
void rxMpegSetUserWord(int word)
{
    s_userWord = word;
}

// ROR 0x00182118
void rxMpegSetVCount(int count)
{
    s_vcount = (u_int)count;
}

// ROR 0x00182148
void rxMpegSetDemuxed(int demuxed)
{
    s_demuxed = demuxed;
}

/* ---- decoding ---- */

// ROR 0x00182154
// Not posted on the host (see the banner): rxMpegDecodePoll runs instead.
void rxMpegDecodeThread(intptr_t *args)
{
    while (s_quit == 0) {
        if (s_run == 1) {
            while (s_free > 1 && unitQueGetCount(s_videoQue) > 0x800) {
                int mbs = s_width / 16 * s_height / 16;
                s_decoding = 1;
                int r = sceMpegGetPicture(&s_mpeg, (sceIpuRGB32 *)(align64(voData) +
                                          s_decodeIdx * s_height * s_width * 4), mbs);
                s_threadPic = r;
                s_getCount++;
                if (r >= 0) {
                    rxThDisableDispatch();
                    s_free--;
                    s_decodeIdx = (s_decodeIdx + 3) % 2;
                    rxThEnableDispatch();
                }
            }
            s_decoding = 0;
        }
    }
}

// ROR 0x001822c4
void rxMpegDecodePoll(void)
{
    if (s_quit != 0 || s_run != 1)
        return;
    while (s_free > 1 && unitQueGetCount(s_videoQue) > 0x800) {
        int mbs = s_width / 16 * s_height / 16;
        s_decoding = 1;
        int r = sceMpegGetPicture(&s_mpeg, (sceIpuRGB32 *)(align64(voData) +
                                  s_decodeIdx * s_height * s_width * 4), mbs);
        s_pollPic = r;
        if (r >= 0) {
            rxThDisableDispatch();
            s_free--;
            s_decodeIdx = (s_decodeIdx + 3) % 2;
            rxThEnableDispatch();
        }
    }
    s_decoding = 0;
}

// ROR 0x0018240c
int rxMpegRequestTrue(intptr_t *args)
{
    return 1;
}

// ROR 0x00182414
void rxMpegStopDecode(void)
{
    s_quit = 1;
    while (rxThGetCallCount() > 0) {
        rxThRequest(rxMpegRequestTrue, 0, 0, 0);
        rxThYield();
    }
    s_quit = 0;
}

// ROR 0x0018247c
// callcheck: rxThPostCall (PC: rxMpegDecodeThread is not posted; see the banner)
void rxMpegStartDecode(void)
{
    s_quit = 0;
}

/* ---- the library's life ---- */

// ROR 0x001824c8
void rxMpegCloseStream(void)
{
    s_left = 0;
    rxMpegStopDecode();
    if (s_fd >= 0) {
        rxFileClose(s_fd);
        s_left = 0;
        s_fd = -1;
    }
    rxMpegAudioClose(0);
}

// ROR 0x0018252c
void rxMpegReset(void)
{
    if (s_created == 0)
        return;
    rxMpegCloseStream();
    s_nearEnd = 0;
    s_frames = 0;
    s_started = 0;
    s_status = 1;
    sceMpegReset(&s_mpeg);
    rxMpegAudioReset(0);
    unitQueReset(s_videoQue);
    rxMpegSetIpuDma(1);
    s_dmaRun = 1;
    s_volume = 0x80;
    s_dmaSize = 0;
    s_field = 0;
    s_field2 = 0;
    s_flipped = 0;
    s_pause = 0;
    rxMpegResetFrames();
}

// ROR 0x001825e4
// callcheck: mcdDbgAssertFailed, FUN_0024f978 (the printing assert handler is ROR_ASSERT; 0x24f978 is a libmpeg setting -- mode 1 into its context +0x87c -- the host libmpeg does not have)
void rxMpegCreate(void)
{
    if (s_created != 0)
        return;
    s_created = 1;
    s_nearEnd = 0;
    gBuffer = ((void *(*)(int))g_rxSysMemFunc[0])(0x60040);
    ROR_ASSERT(gBuffer);
    gMpegWork = ((void *(*)(int))g_rxSysMemFunc[0])(s_width * s_height * 9 / 2 + 0x2840);
    ROR_ASSERT(gMpegWork);
    rxMpegCreatePictures();
    rxMpegAudioCreate(0);
    s_videoQue = unitQueCreate(0x80000);
    sceMpegCreate(&s_mpeg, align64(gMpegWork), s_width * s_height * 9 / 2 + 0x2800);
    sceMpegAddStrCallback(&s_mpeg, sceMpegStrM2V, 0, rxMpegVideoCallback, s_videoQue);
    sceMpegAddStrCallback(&s_mpeg, sceMpegStrPCM, 0, rxMpegAudioCallback, NULL);
    sceMpegAddStrCallback(&s_mpeg, sceMpegStrADPCM, 0, rxMpegAudioCallback, NULL);
    sceMpegAddCallback(&s_mpeg, sceMpegCbError, rxMpegErrorCallback, NULL);
    sceMpegAddCallback(&s_mpeg, sceMpegCbNodata, rxMpegNodataCallback, NULL);
    sceMpegAddCallback(&s_mpeg, sceMpegCbStopDMA, rxMpegStopDmaCallback, NULL);
    sceMpegAddCallback(&s_mpeg, sceMpegCbRestartDMA, rxMpegRestartDmaCallback, NULL);
    s_fd = -1;
}

// ROR 0x00182818
void rxMpegDelete(void)
{
    if (s_created == 0)
        return;
    s_created = 0;
    sceMpegReset(&s_mpeg);
    sceMpegDelete(&s_mpeg);
    unitQueDelete(s_videoQue);
    s_videoQue = NULL;
    rxMpegAudioDelete(0);
    rxMpegDeletePictures();
    ((void (*)(void *))g_rxSysMemFunc[3])(gMpegWork);
    gMpegWork = NULL;
    ((void (*)(void *))g_rxSysMemFunc[3])(gBuffer);
    gBuffer = NULL;
}

// ROR 0x001828c4
void rxMpegInit(void)
{
    s_volume = 0x80;
    s_ready = 0;
    s_created = 0;
    s_videoQue = NULL;
    s_flag34 = 0;
    s_50 = 0;
    s_nearEnd = 0;
    sceMpegInit();
    rxMpegAudioInit(0);
}

// ROR 0x00182928
void rxMpegPrepare(void)
{
    rxMpegCreate();
    rxMpegReset();
}

// ROR 0x0018294c
int rxMpegOpenNow(const char *name)
{
    char path[0x80];
    s_nearEnd = 0;
    s_getCount = 0;
    utlStrCopy2(path, sizeof(path), s_movieDir, name);
    int size = fileGetSize(path);
    if (size < 0)
        return -1;
    utlStrCopy2(path, sizeof(path), s_movieDir, name);
    s_fd = rxSysStreamOpenRing(path, align64(gBuffer), 0x60000, 0x30000);
    if (s_fd < 0)
        return -1;
    s_left = size;
    s_size = s_left;
    rxFilePause(s_fd, 0);
    s_status = 0;
    s_run = 1;
    s_frames = 0;
    s_started = 0;
    rxMpegStartDecode();
    return s_fd;
}

// ROR 0x00182a60
int rxMpegIsNearEnd(void)
{
    return s_nearEnd;
}

// ROR 0x00182a6c
int rxMpegOpenPaused(const char *name)
{
    char path[0x80];
    s_nearEnd = 0;
    s_getCount = 0;
    utlStrCopy2(path, sizeof(path), s_movieDir, name);
    int size = fileGetSize(path);
    if (size < 0)
        return -1;
    utlStrCopy2(path, sizeof(path), s_movieDir, name);
    s_fd = rxSysStreamOpenRing(path, align64(gBuffer), 0x60000, 0x30000);
    if (s_fd < 0)
        return -1;
    s_left = size;
    s_status = 0;
    s_frames = 0;
    s_started = 0;
    s_size = s_left;
    rxFilePause(s_fd, 0);
    s_run = 0;
    rxMpegStartDecode();
    return s_fd;
}

// ROR 0x00182b7c
void rxMpegRun(void)
{
    if (s_fd >= 0) {
        s_run = 1;
        rxFilePause(s_fd, 0);
    }
}

// ROR 0x00182bb4
void rxMpegClose(void)
{
    rxMpegReset();
    rxMpegDelete();
    s_ready = 0;
    s_run = 0;
}

// ROR 0x00182be8
int rxMpegGetFrameCount(void)
{
    int n = s_mpeg.frameCount;
    if (n < 0)
        n = 0;
    return n;
}

// ROR 0x00182bfc
int rxMpegIsReady(void)
{
    return s_ready;
}

// ROR 0x00182c08
void rxMpegUpdate(void)
{
    // PC: the decode thread's pass (see the banner).
    rxMpegDecodePoll();
    rxMpegSetDemuxed(0);
    if (s_fd < 0 || (s_started == 0 && rxFileGetRingFilled(s_fd) < 0x30000))
        return;
    s_ready = 1;
    if (s_run != 1) {
        rxFilePause(s_fd, 1);
        return;
    }
    int tries = 0;
    int got;
    do {
        int n = 0x4000;
        if (rxFileGetRingFilled(s_fd) < 0x4000)
            n = rxFileGetRingFilled(s_fd);
        if (rxFileGetRingSize(s_fd) - rxFileGetRingReadPos(s_fd) <= n)
            n = rxFileGetRingSize(s_fd) - rxFileGetRingReadPos(s_fd);
        got = sceMpegDemuxPss(&s_mpeg, rxFileGetRingData(s_fd), n);
        rxFileRingConsume(s_fd, got);
        if (got > 0)
            rxMpegSetDemuxed(1);
        tries++;
        s_left -= got;
    } while (got > 0 && tries < 2);
    if (s_started == 0 && s_free < 2) {
        rxMpegAudioOpen(0);
        s_started = 1;
    }
    if (s_run == 1 && s_started != 0) {
        if (s_left < 0x2800)
            s_nearEnd = 1;
        if (s_field == 0 && s_pause == 0) {
            s_frames++;
            if (s_free < 2) {
                s_free++;
                s_flipped = 1;
                s_field2 = 0;
                s_showIdx = (s_showIdx + 3) % 2;
            } else {
                // PC: and the decoder has had the stream (the que holds
                // what it has not parsed yet; rxMpegNodataCallback).
                s_status |= s_left < 5 && unitQueGetCount(s_videoQue) <= 0x800;
            }
        }
        s_field ^= 1;
        s_field2 ^= 1;
        rxMpegAudioUpdate(0);
    }
}

// ROR 0x00182f3c
void rxMpegSetFlag4c(int on)
{
    s_flag4c = on != 0;
}

// ROR 0x00182f58
int rxMpegGetHeight(void)
{
    return s_mpeg.height;
}

// ROR 0x00182f64
unsigned char *rxMpegGetPicture(void)
{
    return align64(voData) + (s_showIdx + 1) % 2 * s_height * s_width * 4;
}

// ROR 0x00182fc8
void rxMpegDummy(int a)
{
}

// ROR 0x00182fdc
void rxMpegSetFlag34(int on)
{
    s_flag34 = on != 0;
}

// ROR 0x00182fec
void rxMpegSetVolume(int volume)
{
    if (volume < 0)
        volume = 0;
    else if (volume > 0x80)
        volume = 0x80;
    s_volume = volume;
}

// ROR 0x001830dc
int rxMpegSetPause(int pause)
{
    int old = s_pause;
    s_pause = pause;
    return old;
}

// ROR 0x001830f0
int rxMpegGetStatus(void)
{
    return s_status;
}
