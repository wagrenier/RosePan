/* ==========================================================================
 *  snd/sndman.h
 *
 *  The game's sound manager (snd/sndman.cpp, 0x17f0d0..0x1810f4), one
 *  global at bss 0x7b2580 (constructed by 0x17f0d0 in the static
 *  initializer 0x2041f8, started by main through SndMan::init). A thin
 *  layer over the rx sound library (rx/rxsnd.h): 65 XVAB banks of sound
 *  effects, 48 slots of playing sounds -- ADPCM streams read from
 *  data/snd/xws/ (ids 0..1) and SE voices (ids 2..47) -- and a master
 *  volume fade. Requests only change a slot's state; update() runs the
 *  state machines once a frame.
 * ======================================================================== */
#ifndef _SND_SNDMAN_H
#define _SND_SNDMAN_H

#include <stddef.h>

#include "sdk/eetypes.h"

#define SND_BANK_MAX    0x41    /* 65 bank handles; 65 is "no bank" */
#define SND_SLOT_MAX    0x30    /* 48 slots, ids 0..47 */
#define SND_RING_SIZE   0x40000 /* each stream's file ring */
#define SND_KEEP        (-0x400) /* setStream / setSe: keep this value */

/* A slot's m_type: the bits stopAll's mask selects. */
enum
{
    SND_TYPE_STREAM = 1,
    SND_TYPE_SE     = 8,
};

/* A slot's m_state: requests from the game, acted on by the per-frame
 * update (updateStream / updateSe). */
enum
{
    SND_STATE_START   = 0,      /* open / start the sound */
    SND_STATE_RESTART = 1,      /* stream: open again (a looping stream's end) */
    SND_STATE_PLAY    = 2,
    SND_STATE_SET     = 3,      /* volume / pitch / attribute changed */
    SND_STATE_PAUSE   = 4,      /* pause requested */
    SND_STATE_PAUSED  = 5,
    SND_STATE_RESUME  = 6,      /* resume requested */
    SND_STATE_STOP    = 7,      /* stop requested */
    SND_STATE_CLOSE   = 10,     /* stream: close, then open again */
};

/* What the ROM's asserts call strSndInfo->uniPtr.sndFStream: a stream
 * slot's data (EE offsets from the slot). layout: host (the ring
 * pointer). */
struct SndFStream
{
    int    m_14;                // ROR 0x14: -1, not read
    int    fd;                  // ROR 0x18: rxSysStreamOpenRing's handle
    int    m_1c;                // ROR 0x1c: -1, not read
    int    m_read;              // ROR 0x20: bytes handed to the ADPCM stream
    int    fStream;             // ROR 0x24: rxSndAdpcmOpen's stream, -1 closed
    int    m_mode;              // ROR 0x28: rxSndAdpcmOpen's mode (0, or 2 for playStream mode 1)
    int    m_volL;              // ROR 0x2c
    int    m_volR;              // ROR 0x30
    int    m_attr;              // ROR 0x34: rxSndAdpcmSetAttr's value
    int    m_size;              // ROR 0x38: the file's size (rxFileCallSizeHook)
    float  m_pitch;             // ROR 0x3c
    void  *m_ring;              // ROR 0x40: SND_RING_SIZE bytes
    char   m_name[0x40];        // ROR 0x44: the file name under data/snd/xws/
};

/* An SE slot's data (EE offsets from the slot). layout: host -- the
 * other half of SndInfo's union. */
struct SndSe
{
    int    m_voice;             // ROR 0x14: rxSndSePlay's voice, -1 before
    int    m_bank;              // ROR 0x18: SndMan bank number
    int    m_prog;              // ROR 0x1c
    int    m_note;              // ROR 0x20
    int    m_volL;              // ROR 0x24
    int    m_volR;              // ROR 0x28
    int    m_attr;              // ROR 0x2c: rxSndSeSetAttr's value
    float  m_pitch;             // ROR 0x30
};

/* One playing sound. EE 0x84 bytes. layout: host (the stream's ring
 * pointer). */
struct SndInfo
{
    int    m_type;              // ROR 0x00: SND_TYPE_*, -1 free
    int    m_id;                // ROR 0x04: the id the game holds, -1 free
    int    m_08;                // ROR 0x08: -1, not read
    int    m_loop;              // ROR 0x0c: 1 plays again at the end
    int    m_state;             // ROR 0x10: SND_STATE_*
    union
    {
        SndFStream sndFStream;
        SndSe      sndSe;
    } uniPtr;                   // ROR 0x14
};

/* EE size 0x1b18 (the last word, +0x1b14, is the TskSnd id).
 * layout: host -- the bank buffers and the slots hold pointers; EE
 * offsets in the comments. */
struct SndMan
{
    SndMan();                                                            // ROR 0x0017f0d0
    ~SndMan();                                                           // ROR 0x0017f13c

    void init();                                                         // ROR 0x0017f164
    /* Shuts the rx sound and system libraries down. */
    void term();                                                         // ROR 0x0017f188
    void update();                                                       // ROR 0x0017f200
    /* Master volume fades over frames frames. */
    void fadeIn(int frames);                                             // ROR 0x0017f264
    void fadeOut(int frames);                                            // ROR 0x0017f308
    void updateFade();                                                   // ROR 0x0017f3b8

    void     clearSlot(SndInfo *info);                                   // ROR 0x0017f514
    void     clearSlots();                                               // ROR 0x0017f530
    SndInfo *getSlot(int id);                                            // ROR 0x0017f584
    /* Stops every slot whose type is in mask (SND_TYPE_*). */
    void     stopAll(u_int mask);                                        // ROR 0x0017f5e8
    void     updateSlots();                                              // ROR 0x0017f6b8

    /* Banks by GamSndMan sound id (its entry's name and bank) / by file
     * name ("xxxNN...": NN is the bank). */
    int  loadBankById(int sndId, int nowait);                            // ROR 0x0017f7ac
    int  finishBankById(int sndId);                                      // ROR 0x0017f81c
    void loadBankByName(const char *name, int nowait);                   // ROR 0x0017f878
    void finishCurBank();                                                // ROR 0x0017f8e4
    /* Plays the sound effect sndId; its slot id, -1 when it cannot. */
    int  playSe(int sndId);                                              // ROR 0x0017f8fc

    /* Ids: kind 0 a stream (0..1), kind 1 an SE (2..47); -1 when none is
     * free. */
    int      allocId(int kind);                                          // ROR 0x0017f9ac
    int      checkId(int kind, int id);                                  // ROR 0x0017fa5c
    void     setIdUsed(int id);                                          // ROR 0x0017fa90
    void     setIdFree(int id);                                          // ROR 0x0017fac4
    void     initStreamSlot(SndInfo *info);                              // ROR 0x0017fb00
    SndInfo *findFreeSlot();                                             // ROR 0x0017fb5c
    void     freeSlot(SndInfo *info);                                    // ROR 0x0017fba4
    SndInfo *allocStreamSlot(int id);                                    // ROR 0x0017fbc4
    void     freeStreamSlot(SndInfo *info);                              // ROR 0x0017fc74
    int      countStreams();                                             // ROR 0x0017fc90

    /* Streams data/snd/xws/<name>; mode 1 opens it in ADPCM mode 2. id -1
     * takes a free stream id. Returns the id, -1 on failure. */
    int  playStreamStereo(const char *name);                             // ROR 0x0017fd1c
    int  playStreamAuto(const char *name, int mode);                     // ROR 0x0017fd3c
    int  playStream(const char *name, int mode, int id, int arg);        // ROR 0x0017fd58
    /* SND_KEEP (-1024.0f for the pitch) keeps a value; 0 when set, -1 for
     * no such stream. */
    int  setStream(int id, int volL, int volR, float pitch, int attr);   // ROR 0x0017ff58
    int  setStreamAttr(int id, int attr);                                // ROR 0x0018000c
    int  setStreamLoop(int id, int loop);                                // ROR 0x00180078
    int  getStreamState(int id);                                         // ROR 0x001800d0
    int  pauseStream(int id);                                            // ROR 0x00180108
    int  resumeStream(int id);                                           // ROR 0x00180168
    int  stopStream(int id);                                             // ROR 0x001801c8
    /* One frame of a stream; -1 when the slot is done. */
    int  updateStream(SndInfo *strSndInfo);                              // ROR 0x00180230

    /* Loads the XVAB xvab (rxSndXvabLoad's data; the by-id / by-name
     * callers pass a name there) as bank; waits for the transfer unless
     * nowait. The XVAB id, -1 on failure. */
    int  loadBank(int bank, const void *xvab, int nowait);               // ROR 0x001809d8
    int  finishBank(int bank);                                           // ROR 0x00180a90
    int  freeBank(int bank);                                             // ROR 0x00180b30
    void initSeSlot(SndInfo *info);                                      // ROR 0x00180b9c
    int  playSeBank(int bank, int prog, int note);                       // ROR 0x00180be4
    int  getSeState(int id);                                             // ROR 0x00180ca8
    int  setSe(int id, int volL, int volR, float pitch, int attr);       // ROR 0x00180ce0
    int  setSeAttr(int id, int attr);                                    // ROR 0x00180d94
    int  pauseSe(int id);                                                // ROR 0x00180e00
    int  resumeSe(int id);                                               // ROR 0x00180e54
    int  stopSe(int id);                                                 // ROR 0x00180ea8
    int  setSeLoop(int id, int loop);                                    // ROR 0x00180eec
    /* One frame of an SE; -1 when the slot is done. */
    int  updateSe(SndInfo *info);                                        // ROR 0x00180f44

    int     m_state;                    // ROR 0x00: 0 after construction, 1 after init
    char    m_bootName[0x20];           // ROR 0x04: main copies argv[0] here
    u_int   m_idUsed[2];                // ROR 0x24: id bits, 0..31 and 32..47
    int     m_bank[SND_BANK_MAX];       // ROR 0x2c: rxSndXvabLoad's ids, -1 none
    void   *m_bankData[SND_BANK_MAX];   // ROR 0x130: a bank's source buffer, freed by finishBank
    SndInfo m_info[SND_SLOT_MAX];       // ROR 0x234
    int     m_curBank;                  // ROR 0x1af4: loadBankByName's bank
    int     m_fadeState;                // ROR 0x1af8: 0 none, 1 out, 2 in
    int     m_fadeCount;                // ROR 0x1afc
    int     m_fadeFrames;               // ROR 0x1b00
    int     m_volL;                     // ROR 0x1b04: the master volume a fade reaches
    int     m_volR;                     // ROR 0x1b08
    int     m_curVolL;                  // ROR 0x1b0c
    int     m_curVolR;                  // ROR 0x1b10
    int     m_taskId;                   // ROR 0x1b14: the TskSnd playStream queued
};
static_assert(offsetof(SndMan, m_bootName) == 0x04, "m_bootName");

extern SndMan g_sndMan;         /* bss 0x7b2580 */

#endif /* _SND_SNDMAN_H */
