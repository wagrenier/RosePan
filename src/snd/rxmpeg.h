/* ==========================================================================
 *  snd/rxmpeg.h
 *
 *  The movie player (snd/rxmpeg.cpp, 0x1810f4..0x1830fc): the RxMpeg
 *  object (bss 0x7b40c0: the title's logo and attract movies, S2dMovie's
 *  scene movies, the movie gallery) over a C library that plays a PSS file
 *  from movie/ through an rx file ring: sceMpegDemuxPss splits it, the
 *  video stream goes into a que the IPU decodes from (two RGBA32 pictures,
 *  one shown while the other decodes, flipped every other frame), the
 *  audio stream (SShd header: PCM or ADPCM) into a que an rxSnd stream
 *  plays.
 *
 *  A movie's life: open, play (the ring fills; isReady says it is primed),
 *  run (demux and decode start), getStatus 1 once it has ended, close.
 *  playNow starts without the prebuffer wait.
 * ======================================================================== */
#ifndef _SND_RXMPEG_H
#define _SND_RXMPEG_H

#include <stddef.h>
#include <stdint.h>

#include "sdk/libmpeg.h"

struct I3dLayer;
struct UnitQue;

/* The object at bss 0x7b40c0 (g_rxMpeg; constructed by the static
 * initializer 0x2041f8). Class name from the file; no vtable. The words
 * are ints and the name chars, so the EE offsets hold on the host. */
struct RxMpeg
{
    RxMpeg();                                                            // ROR 0x001810f4
    ~RxMpeg();                                                           // ROR 0x00181128
    /* Marks the player open and resets the library's words. */
    void open();                                                         // ROR 0x00181150
    /* No callers in the ROM. */
    void setUserWord(int word);                                          // ROR 0x001811d0
    void setFrameParity(int parity);                                     // ROR 0x00181204
    /* Once a frame from SndMan::update: the frame hold, the library's frame. */
    void update();                                                       // ROR 0x0018120c
    /* The shown picture into layer's packet (a GS image transfer into the
     * frame buffer). */
    void draw(I3dLayer *layer);                                          // ROR 0x001812a8
    /* Empty in the ROM; SndMan::update calls it after rxSndUpdate. */
    void postUpdate();                                                   // ROR 0x00181494
    /* Starts the movie name at w x h at once (no prebuffer wait). */
    int  playNow(const char *name, int w, int h);                        // ROR 0x001814ac
    int  startNow(const char *name);                                     // ROR 0x001814cc
    /* Starts the movie name at w x h, held until run. */
    int  play(const char *name, int w, int h);                           // ROR 0x0018154c
    int  startPaused(const char *name);                                  // ROR 0x0018156c
    /* Lets a held movie go (once isReady); -1 when nothing plays. */
    int  run();                                                          // ROR 0x00181644
    int  close();                                                        // ROR 0x001816b0
    /* -1 when not open, else 0 playing / 1 idle or finished. */
    int  getStatus();                                                    // ROR 0x0018173c
    /* 1 once the ring is primed (or playback started). */
    int  isReady();                                                      // ROR 0x0018176c
    /* 1 when fewer than 0x2800 bytes of the file are left; then, once
     * (+0x34), fades the scene out when it is not fading and bit 0x8000000
     * of g_sequence.m_bits[1] is set. */
    int  checkNearEnd();                                                 // ROR 0x00181780

    int  m_active;              // ROR 0x00: open sets it, close / the constructor clear it; MagSystem::waitVSync runs rxThVSync while set
    int  m_playing;             // ROR 0x04: 1 once a start opened the file; S2dMovie::drawMovie draws while it is 1
    int  m_08;                  // ROR 0x08: open clears it
    int  m_0c;                  // ROR 0x0c
    int  m_hold;                // ROR 0x10: update holds the picture at m_holdAt while set
    int  m_holdAt;              // ROR 0x14: the frame count to hold at, -1 none (update clears it on a mismatch)
    int  m_18;                  // ROR 0x18: 0x80
    int  m_1c;                  // ROR 0x1c: 0x80
    int  m_volume;              // ROR 0x20: 0x80; update passes it to rxMpegSetVolume
    int  m_24;                  // ROR 0x24: open clears it
    int  m_28;                  // ROR 0x28: 0xe00 (open, setUserWord)
    int  m_width;               // ROR 0x2c: play's w (0x200), the frame buffer width draw writes
    int  m_height;              // ROR 0x30: play's h (0x180)
    int  m_fadedOut;            // ROR 0x34: checkNearEnd faded the scene out
    int  m_38;                  // ROR 0x38
    int  m_parity;              // ROR 0x3c: setFrameParity; draw writes to frame buffer 0 when odd
    int  m_picture;             // ROR 0x40: draw stores the shown picture's address, the starts clear it; nothing reads it (PC: its low word)
    char m_name[0x80];          // ROR 0x44: the callers copy the movie name here
    int  m_savedLevel;          // ROR 0xc4: GamMan status 0x10 (option 0's level) startPaused saves, close restores
};
static_assert(offsetof(RxMpeg, m_playing) == 0x04, "m_playing");
static_assert(offsetof(RxMpeg, m_hold) == 0x10, "m_hold");
static_assert(offsetof(RxMpeg, m_holdAt) == 0x14, "m_holdAt");
static_assert(offsetof(RxMpeg, m_volume) == 0x20, "m_volume");
static_assert(offsetof(RxMpeg, m_28) == 0x28, "m_28");
static_assert(offsetof(RxMpeg, m_width) == 0x2c, "m_width");
static_assert(offsetof(RxMpeg, m_height) == 0x30, "m_height");
static_assert(offsetof(RxMpeg, m_fadedOut) == 0x34, "m_fadedOut");
static_assert(offsetof(RxMpeg, m_parity) == 0x3c, "m_parity");
static_assert(offsetof(RxMpeg, m_picture) == 0x40, "m_picture");
static_assert(offsetof(RxMpeg, m_name) == 0x44, "m_name");
static_assert(offsetof(RxMpeg, m_savedLevel) == 0xc4, "m_savedLevel");
static_assert(sizeof(RxMpeg) == 0xc8, "RxMpeg");

extern RxMpeg g_rxMpeg;         /* bss 0x7b40c0 */

/* ---- the movie library ---- */

/* One audio channel (data 0x6e9288, 0x40 bytes each; only channel 0 is
 * used). The header is the stream's SShd chunk as it arrives, from its
 * tag: SShd, size, type (0x10 and up ADPCM, else PCM), rate, channels,
 * interleave, loop start / end, SSbd, size.
 * layout: host -- m_que is a pointer (EE +0x14, 4 bytes). */
struct RxMpegAudio
{
    int      m_no;              // ROR 0x00: the channel number
    int      m_type;            // ROR 0x04: 0 PCM, 1 ADPCM, -1 not known yet (the stream tables' index)
    int      m_stream;          // ROR 0x08: the rxSnd stream, -1 none
    int      m_volL;            // ROR 0x0c: 0x80
    int      m_volR;            // ROR 0x10: 0x80
    UnitQue *m_que;             // ROR 0x14: the samples
    int      m_hdr[10];         // ROR 0x18: the SShd chunk ([2] type, [3] rate, [4] channels, [5] interleave)
};

int  rxMpegPcmOpen(int stereo, int rate, int iopAddr, int iopKB, int spuAddr, int spuKB); // ROR 0x0018182c
int  rxMpegAdpcmOpen(int stereo, int rate, int iopAddr, int iopKB, int spuAddr, int spuKB); // ROR 0x00181848
void rxMpegAudioInit(int no);                                            // ROR 0x0018185c
void rxMpegAudioReset(int no);                                           // ROR 0x001818b4
void rxMpegAudioOpen(int no);                                            // ROR 0x00181920
void rxMpegAudioClose(int no);                                           // ROR 0x00181998
int  rxMpegAudioIsOpen(int no);                                          // ROR 0x001819f4
void rxMpegAudioCreate(int no);                                          // ROR 0x00181a24
void rxMpegAudioDelete(int no);                                          // ROR 0x00181a70
void rxMpegAudioUpdate(int no);                                          // ROR 0x00181ab4
void rxMpegResetFrames(void);                                            // ROR 0x00181bf4
void rxMpegCreatePictures(void);                                         // ROR 0x00181c14
void rxMpegDeletePictures(void);                                         // ROR 0x00181ce4
/* The sceMpeg callbacks rxMpegCreate registers. */
int  rxMpegVideoCallback(sceMpeg *mp, sceMpegCbData *data, void *que);   // ROR 0x00181d54
int  rxMpegAudioCallback(sceMpeg *mp, sceMpegCbData *data, void *no);    // ROR 0x00181dd4
int  rxMpegErrorCallback(sceMpeg *mp, sceMpegCbData *data, void *arg);   // ROR 0x00181eec
void rxMpegSetIpuDma(int chcr);                                          // ROR 0x00181ef4
int  rxMpegNodataCallback(sceMpeg *mp, sceMpegCbData *data, void *arg);  // ROR 0x00181f5c
int  rxMpegStopDmaCallback(sceMpeg *mp, sceMpegCbData *data, void *arg); // ROR 0x00182094
int  rxMpegRestartDmaCallback(sceMpeg *mp, sceMpegCbData *data, void *arg); // ROR 0x001820d0
void rxMpegSetUserWord(int word);                                        // ROR 0x0018210c
/* Stores the V-blank count (data 0x298fd0); MagSystem's V-blank callback
 * passes it on every V-blank. */
void rxMpegSetVCount(int count);                                         // ROR 0x00182118
void rxMpegSetDemuxed(int demuxed);                                      // ROR 0x00182148
void rxMpegDecodeThread(intptr_t *args);                                 // ROR 0x00182154
void rxMpegDecodePoll(void);                                             // ROR 0x001822c4
int  rxMpegRequestTrue(intptr_t *args);                                  // ROR 0x0018240c
void rxMpegStopDecode(void);                                             // ROR 0x00182414
void rxMpegStartDecode(void);                                            // ROR 0x0018247c
void rxMpegCloseStream(void);                                            // ROR 0x001824c8
void rxMpegReset(void);                                                  // ROR 0x0018252c
void rxMpegCreate(void);                                                 // ROR 0x001825e4
void rxMpegDelete(void);                                                 // ROR 0x00182818
void rxMpegInit(void);                                                   // ROR 0x001828c4
void rxMpegPrepare(void);                                                // ROR 0x00182928
/* Opens movie/<name> running (Now) or held until rxMpegRun (Paused);
 * the ring's fd or -1. */
int  rxMpegOpenNow(const char *name);                                    // ROR 0x0018294c
int  rxMpegIsNearEnd(void);                                              // ROR 0x00182a60
int  rxMpegOpenPaused(const char *name);                                 // ROR 0x00182a6c
void rxMpegRun(void);                                                    // ROR 0x00182b7c
void rxMpegClose(void);                                                  // ROR 0x00182bb4
int  rxMpegGetFrameCount(void);                                          // ROR 0x00182be8
int  rxMpegIsReady(void);                                                // ROR 0x00182bfc
void rxMpegUpdate(void);                                                 // ROR 0x00182c08
void rxMpegSetFlag4c(int on);                                            // ROR 0x00182f3c
int  rxMpegGetHeight(void);                                              // ROR 0x00182f58
unsigned char *rxMpegGetPicture(void);                                   // ROR 0x00182f64
void rxMpegDummy(int a);                                                 // ROR 0x00182fc8
void rxMpegSetFlag34(int on);                                            // ROR 0x00182fdc
void rxMpegSetVolume(int volume);                                        // ROR 0x00182fec
int  rxMpegSetPause(int pause);                                          // ROR 0x001830dc
int  rxMpegGetStatus(void);                                              // ROR 0x001830f0

#endif /* _SND_RXMPEG_H */
