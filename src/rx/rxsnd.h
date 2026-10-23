/* ==========================================================================
 *  rx/rxsnd.h
 *
 *  The rx sound library (0x237ce8..0x23ff58, pinned; no file string;
 *  "rxsnd :" messages): the EE client of the IOP sound module RXSNDDRV.IRX
 *  (RPC 0xd000). Commands are either sent at once (rxSndRpcCommand) or
 *  queued and flushed once a frame by rxSndUpdate (rxSndQueue*). Families:
 *  0x1x system, 0x2x IOP heap, 0x4x XVAB banks, 0x7x BGM, 0x8x SE voices,
 *  0xax ADPCM streams, 0xbx PCM streams. Each family keeps its slots on an
 *  active / stopping / free list; the per-frame status replies move them.
 *
 *  Names from its own strings (rxSndBgmStop, rxSndSeStop, rxSndAdpcmClose
 *  are passed to the BG-call hook) and from how SndMan uses them.
 *  On the host the IOP side is src/sdk/iop_rxsnd.cpp (a silent stand-in).
 * ======================================================================== */
#ifndef _RX_RXSND_H
#define _RX_RXSND_H

#include "sdk/eetypes.h"
#include "sdk/sifrpc.h"

#include <stddef.h>

/* ---- IOP command data (EE layout: it is the RPC payload) --------------- */

/* The queue's per-command header; the command's data follows. EE 0xc. */
struct RxSndQueueHeader
{
    u_int m_cmd;                        // ROR +0x0
    int   m_replySize;                  // ROR +0x4: bytes reserved in the reply area
    int   m_dataSize;                   // ROR +0x8
};
static_assert(sizeof(RxSndQueueHeader) == 0xc, "RxSndQueueHeader");
static_assert(offsetof(RxSndQueueHeader, m_replySize) == 0x4, "RxSndQueueHeader::m_replySize");
static_assert(offsetof(RxSndQueueHeader, m_dataSize) == 0x8, "RxSndQueueHeader::m_dataSize");

/* {slot, parameter, value} of the set commands 0x71 / 0x82 / 0xa2 / 0xb2.
 * Parameters seen: 0 stop / close, 1 pause, 2 attribute, 3 (0xb2: 4)
 * volume (value = two s16 L / R), 4 pitch (value = pitch * 4096). EE 0xc. */
struct RxSndCmdSet
{
    int m_id;                           // ROR +0x0
    int m_param;                        // ROR +0x4
    union
    {
        int   m_value;                  // ROR +0x8
        short m_vol[2];                 // ROR +0x8: L, R
    };
};
static_assert(sizeof(RxSndCmdSet) == 0xc, "RxSndCmdSet");
static_assert(offsetof(RxSndCmdSet, m_param) == 0x4, "RxSndCmdSet::m_param");
static_assert(offsetof(RxSndCmdSet, m_value) == 0x8, "RxSndCmdSet::m_value");

/* Command 0xa4: one channel's parameter of an ADPCM stream (the volume of
 * a mono / stereo stream goes per channel). EE 0xc. */
struct RxSndCmdChannelSet
{
    u_short m_id;                       // ROR +0x0
    u_short m_channel;                  // ROR +0x2
    u_short m_param;                    // ROR +0x4: 3 volume
    u_short m_6;                        // ROR +0x6: always 0
    short   m_vol[2];                   // ROR +0x8: L, R (the other side 0)
};
static_assert(sizeof(RxSndCmdChannelSet) == 0xc, "RxSndCmdChannelSet");
static_assert(offsetof(RxSndCmdChannelSet, m_channel) == 0x2, "RxSndCmdChannelSet::m_channel");
static_assert(offsetof(RxSndCmdChannelSet, m_param) == 0x4, "RxSndCmdChannelSet::m_param");
static_assert(offsetof(RxSndCmdChannelSet, m_vol) == 0x8, "RxSndCmdChannelSet::m_vol");

/* ---- status replies (queued commands 0x72 / 0x83 / 0xb3 / 0xa3) -------- */
/* Each lists the slots the IOP still plays: the slot id and its status bits
 * (2 playing, 4 ended, 8 paused), then the count. */

struct RxSndStatusEntry                 /* EE 0x4 */
{
    u_char m_id;                        // ROR +0x0
    u_char m_status;                    // ROR +0x1
    u_char _2[2];
};
static_assert(sizeof(RxSndStatusEntry) == 0x4, "RxSndStatusEntry");
static_assert(offsetof(RxSndStatusEntry, m_status) == 0x1, "RxSndStatusEntry::m_status");

struct RxSndBgmStatus                   /* EE 0x104 */
{
    RxSndStatusEntry m_entry[32];       // ROR +0x0
    u_int            m_value[32];       // ROR +0x80: per entry (stored in RxSndBgm::m_value)
    int              m_count;           // ROR +0x100
};
static_assert(sizeof(RxSndBgmStatus) == 0x104, "RxSndBgmStatus");
static_assert(offsetof(RxSndBgmStatus, m_value) == 0x80, "RxSndBgmStatus::m_value");
static_assert(offsetof(RxSndBgmStatus, m_count) == 0x100, "RxSndBgmStatus::m_count");

struct RxSndSeStatus                    /* EE 0x104 */
{
    RxSndStatusEntry m_entry[64];       // ROR +0x0
    int              m_count;           // ROR +0x100
};
static_assert(sizeof(RxSndSeStatus) == 0x104, "RxSndSeStatus");
static_assert(offsetof(RxSndSeStatus, m_count) == 0x100, "RxSndSeStatus::m_count");

struct RxSndPcmStatusEntry              /* EE 0xc */
{
    u_char m_id;                        // ROR +0x0
    u_char m_status;                    // ROR +0x1
    short  m_2;                         // ROR +0x2: stored in RxSndPcm::m_20 (unverified)
    int    m_posHi;                     // ROR +0x4: play position, compared signed
    int    m_posLo;                     // ROR +0x8
};
static_assert(sizeof(RxSndPcmStatusEntry) == 0xc, "RxSndPcmStatusEntry");
static_assert(offsetof(RxSndPcmStatusEntry, m_2) == 0x2, "RxSndPcmStatusEntry::m_2");
static_assert(offsetof(RxSndPcmStatusEntry, m_posHi) == 0x4, "RxSndPcmStatusEntry::m_posHi");
static_assert(offsetof(RxSndPcmStatusEntry, m_posLo) == 0x8, "RxSndPcmStatusEntry::m_posLo");

struct RxSndPcmStatus                   /* EE 0x184 */
{
    RxSndPcmStatusEntry m_entry[32];    // ROR +0x0
    int                 m_count;        // ROR +0x180
};
static_assert(sizeof(RxSndPcmStatus) == 0x184, "RxSndPcmStatus");
static_assert(offsetof(RxSndPcmStatus, m_count) == 0x180, "RxSndPcmStatus::m_count");

struct RxSndAdpcmStatusEntry            /* EE 0x10 */
{
    u_char m_id;                        // ROR +0x0
    u_char m_status;                    // ROR +0x1
    u_char _2[2];
    int    m_free;                      // ROR +0x4: bytes the IOP buffer can take
    u_int  m_posHi;                     // ROR +0x8: play position, compared unsigned
    u_int  m_posLo;                     // ROR +0xc
};
static_assert(sizeof(RxSndAdpcmStatusEntry) == 0x10, "RxSndAdpcmStatusEntry");
static_assert(offsetof(RxSndAdpcmStatusEntry, m_status) == 0x1, "RxSndAdpcmStatusEntry::m_status");
static_assert(offsetof(RxSndAdpcmStatusEntry, m_free) == 0x4, "RxSndAdpcmStatusEntry::m_free");
static_assert(offsetof(RxSndAdpcmStatusEntry, m_posHi) == 0x8, "RxSndAdpcmStatusEntry::m_posHi");
static_assert(offsetof(RxSndAdpcmStatusEntry, m_posLo) == 0xc, "RxSndAdpcmStatusEntry::m_posLo");

struct RxSndAdpcmStatus                 /* EE 0x304 */
{
    RxSndAdpcmStatusEntry m_entry[48];  // ROR +0x0
    int                   m_count;      // ROR +0x300
};
static_assert(sizeof(RxSndAdpcmStatus) == 0x304, "RxSndAdpcmStatus");
static_assert(offsetof(RxSndAdpcmStatus, m_count) == 0x300, "RxSndAdpcmStatus::m_count");

/* ---- EE-side state ----------------------------------------------------- */

/* bss 0x7b5ac0. EE 0x8. */
struct RxSndConfig
{
    u_char m_debugFlags;                // ROR +0x0: 1 errors, 2 BG-call warnings, 4 bind messages
    u_char m_1;                         // ROR +0x1: set 1, not read here (unverified)
    u_char m_2;                         // ROR +0x2: set 0x3c, not read here (unverified)
    u_char _3;
    short  m_masterVolL;                // ROR +0x4
    short  m_masterVolR;                // ROR +0x6
};
static_assert(sizeof(RxSndConfig) == 0x8, "RxSndConfig");
static_assert(offsetof(RxSndConfig, m_masterVolL) == 0x4, "RxSndConfig::m_masterVolL");
static_assert(offsetof(RxSndConfig, m_masterVolR) == 0x6, "RxSndConfig::m_masterVolR");

/* bss 0x7b6b00: two IOP buffers taken at init. EE 0x80. */
struct RxSndIopBuf
{
    u_int _0[0x10];
    u_int m_buf0;                       // ROR +0x40: IOP address
    u_int m_buf1;                       // ROR +0x44: IOP address
    u_int _48[0xe];
};
static_assert(sizeof(RxSndIopBuf) == 0x80, "RxSndIopBuf");
static_assert(offsetof(RxSndIopBuf, m_buf0) == 0x40, "RxSndIopBuf::m_buf0");
static_assert(offsetof(RxSndIopBuf, m_buf1) == 0x44, "RxSndIopBuf::m_buf1");

/* A block of the SPU heap (bss 0x7b6b80, 256 of them), on a circular list
 * through the sentinel 0x7b8380. layout: host -- it holds pointers. EE 0x18. */
struct RxSndSpuNode
{
    RxSndSpuNode *m_next;               // ROR +0x0
    RxSndSpuNode *m_prev;               // ROR +0x4
    int           m_8;                  // ROR +0x8 (unverified: owner / state)
    int           m_c;                  // ROR +0xc
    u_int         m_addr;               // ROR +0x10: SPU address
    u_int         m_size;               // ROR +0x14
};

/* bss 0x7b83a0, 256 of them: the handles rxSndSpuHeapAlloc gives out
 * (unverified). EE 0x14. */
struct RxSndSpuSlot
{
    int m_index;                        // ROR +0x0
    int m_used;                         // ROR +0x4
    int _8[3];
};
static_assert(sizeof(RxSndSpuSlot) == 0x14, "RxSndSpuSlot");
static_assert(offsetof(RxSndSpuSlot, m_used) == 0x4, "RxSndSpuSlot::m_used");

/* bss 0x7b97a0: the SPU region the heap covers (unverified names). EE 0x20. */
struct RxSndSpuRegion
{
    int   m_0;                          // ROR +0x0: -1
    int   m_4;                          // ROR +0x4: 1
    u_int m_addr;                       // ROR +0x8: 0x5010
    u_int m_size;                       // ROR +0xc: 0x1baff0
    int   m_10;                         // ROR +0x10: 0
    int   m_14[2];                      // ROR +0x14: -1, -1
    int   _1c;
};
static_assert(sizeof(RxSndSpuRegion) == 0x20, "RxSndSpuRegion");
static_assert(offsetof(RxSndSpuRegion, m_addr) == 0x8, "RxSndSpuRegion::m_addr");
static_assert(offsetof(RxSndSpuRegion, m_10) == 0x10, "RxSndSpuRegion::m_10");
static_assert(offsetof(RxSndSpuRegion, m_14) == 0x14, "RxSndSpuRegion::m_14");

/* An XVAB bank (bss 0x7b97c0, 256 of them; filled by rxSndXvabLoad). EE 0x18. */
struct RxSndXvab
{
    u_int  m_flags;                     // ROR +0x0: bit 0 used
    u_char _4[0x10];
    int    m_transfer;                  // ROR +0x14: IOP transfer handle, -1 when done
};
static_assert(sizeof(RxSndXvab) == 0x18, "RxSndXvab");
static_assert(offsetof(RxSndXvab, m_transfer) == 0x14, "RxSndXvab::m_transfer");

/* A BGM slot (bss 0x7bbbc0, 32). layout: host -- next pointer. EE 0x18. */
struct RxSndBgm
{
    RxSndBgm *m_next;                   // ROR +0x0
    int       m_id;                     // ROR +0x4: index, also the IOP's id
    u_int     m_flags;                  // ROR +0x8: 1 open, 2 playing, 4 ended, 8 paused
    int       m_keep;                   // ROR +0xc: 0 = stop by itself when ended (unverified)
    u_int     m_value;                  // ROR +0x10: from RxSndBgmStatus::m_value
    short     m_volL;                   // ROR +0x14
    short     m_volR;                   // ROR +0x16
};

/* An SE voice (bss 0x7bbed0, 64). layout: host -- next pointer. EE 0x14. */
struct RxSndSeVoice
{
    RxSndSeVoice *m_next;               // ROR +0x0
    int           m_id;                 // ROR +0x4
    u_int         m_flags;              // ROR +0x8: 1 open, 2 playing, 4 ended, 8 paused
    int           m_keep;               // ROR +0xc: 0 = stop by itself when ended (unverified)
    short         m_volL;               // ROR +0x10
    short         m_volR;               // ROR +0x12
};

/* A play-position callback of a PCM (pool 0x7bc400) or ADPCM (pool
 * 0x7befc0) stream, 256 each, sorted by position. layout: host. EE 0x14. */
typedef void (*RxSndPosFunc)(int id, int arg);
struct RxSndPosCallback
{
    RxSndPosCallback *m_next;           // ROR +0x0
    u_int             m_posHi;          // ROR +0x4
    u_int             m_posLo;          // ROR +0x8
    RxSndPosFunc      m_func;           // ROR +0xc
    int               m_arg;            // ROR +0x10
};

/* A PCM stream (bss 0x7bd804, 32). layout: host -- pointers. EE 0x3c. */
struct RxSndPcm
{
    RxSndPcm         *m_next;           // ROR +0x0
    int               m_id;             // ROR +0x4
    int               m_8;              // ROR +0x8
    u_int             m_iopBuf;         // ROR +0xc: IOP address
    int               _10[2];
    u_char            m_status;         // ROR +0x18: 1 open, 2 playing, 4 ended, 8 paused
    u_char            _19;
    u_char            m_alloc;          // ROR +0x1a: bit 0 m_iopBuf is ours
    u_char            _1b;
    short             m_volL;           // ROR +0x1c
    short             m_volR;           // ROR +0x1e
    int               m_20;             // ROR +0x20: RxSndPcmStatusEntry::m_2
    int               _24[5];
    RxSndPosCallback *m_callbacks;      // ROR +0x38
};

/* An ADPCM stream (bss 0x7c03c8, 48). layout: host -- pointers. EE 0x50. */
struct RxSndAdpcm
{
    RxSndAdpcm       *m_next;           // ROR +0x0
    int               m_mode;           // ROR +0x4: 1..2 mono / stereo (unverified), 0x1000000 interleaved
    u_char            m_id;             // ROR +0x8
    u_char            m_status;         // ROR +0x9: 1 open, 2 playing, 4 ended, 8 paused
    u_char            m_alloc;          // ROR +0xa: bit 0 m_iopBuf, bit 1 m_spuBuf are ours
    u_char            _b;
    u_char            m_channels;       // ROR +0xc
    u_char            _d[7];
    short             m_volL;           // ROR +0x14
    short             m_volR;           // ROR +0x16
    u_int             m_iopBuf;         // ROR +0x18: IOP address
    int               m_bufSize;        // ROR +0x1c
    int               m_bufFill;        // ROR +0x20: m_bufSize - the IOP's free bytes
    int               _24[2];
    u_int             m_spuBuf;         // ROR +0x2c: SPU address
    int               _30[4];
    u_long            m_position;       // ROR +0x40
    RxSndPosCallback *m_callbacks;      // ROR +0x48
};

typedef u_int (*RxSndAllocFunc)(int size);
typedef void  (*RxSndFreeFunc)(u_int addr);
typedef void  (*RxSndBgCallHook)(const char *func);

/* ---- functions --------------------------------------------------------- */

/* rxTh yield callback: selects the RPC buffer bank (1 while yielding). */
void  rxSndThreadCallback(int yielding);                                 // ROR 0x00237ce8
/* BG-call hook: warns when a library call comes from a yielded BG thread. */
void  rxSndCheckBgCall(const char *func);                                // ROR 0x00237d00
void  rxSndInit(void);                                                   // ROR 0x00237d80
void  rxSndTerm(void);                                                   // ROR 0x00237db0
void  rxSndInitModules(void);                                            // ROR 0x00237dd0
void  rxSndTermModules(void);                                            // ROR 0x00237e30
/* Once a frame: flushes the queue, asks every family's status, applies it. */
void  rxSndUpdate(void);                                                 // ROR 0x00237eb0
u_char rxSndGetDebugFlags(void);                                         // ROR 0x00238038
void  rxSndSetMasterVolume(int volL, int volR);                          // ROR 0x00238188
int   rxSndSetBufferBank(int bank);                                      // ROR 0x00238268
void  rxSndInitConfig(void);                                             // ROR 0x00238478
void  rxSndTermConfig(void);                                             // ROR 0x002384a8
int   rxSndGetBufferBank(void);                                          // ROR 0x002384b8
short rxSndGetMasterVolumeL(void);                                       // ROR 0x002384c8
short rxSndGetMasterVolumeR(void);                                       // ROR 0x002384d8
void *rxSndGetWorkBuffer(void);                                          // ROR 0x002384e8
void  rxSndWait(void);                                                   // ROR 0x002384f8
void  rxSndSetBgCallHook(RxSndBgCallHook hook);                          // ROR 0x00238500
void  rxSndCallBgCallHook(const char *func);                             // ROR 0x00238510
u_int rxSndDefIopAlloc(int size);                                        // ROR 0x002385b8
u_int rxSndDefIopAllocMode1(int size);                                   // ROR 0x00238630
void  rxSndDefIopFree(u_int addr);                                       // ROR 0x002386b0
u_int rxSndIopAlloc(int size);                                           // ROR 0x00238740
u_int rxSndIopAllocMode1(int size);                                      // ROR 0x00238768
void  rxSndIopFree(u_int addr);                                          // ROR 0x00238790
void  rxSndIopBufInit(void);                                             // ROR 0x00238d28
void  rxSndIopBufTerm(void);                                             // ROR 0x00238d78
u_int rxSndDefSpuAlloc(int size);                                        // ROR 0x00238ff0
u_int rxSndDefSpuAllocMode1(int size);                                   // ROR 0x00239010
void  rxSndDefSpuFree(u_int addr);                                       // ROR 0x00239030
void  rxSndSetSpuAllocHook(RxSndAllocFunc func);                         // ROR 0x00239048
void  rxSndSetSpuAllocMode1Hook(RxSndAllocFunc func);                    // ROR 0x00239068
void  rxSndSetSpuFreeHook(RxSndFreeFunc func);                           // ROR 0x00239088
u_int rxSndSpuAlloc(int size);                                           // ROR 0x002390a8
u_int rxSndSpuAllocMode1(int size);                                      // ROR 0x002390d0
void  rxSndSpuFree(u_int addr);                                          // ROR 0x002390f8
/* Asks the IOP whether XVAB transfer `handle` is still running (command
 * 0x41); wait != 0 polls until it is done. */
int   rxSndXvabPoll(int handle, int wait);                               // ROR 0x00239288
void  rxSndSpuHeapInit(void);                                            // ROR 0x00239380
void  rxSndSpuHeapTerm(void);                                            // ROR 0x002394a0
u_int rxSndSpuHeapAlloc(int mode, int arg, int size);                    // ROR 0x002394e0
void  rxSndSpuHeapFree(u_int addr);                                      // ROR 0x002398f8
RxSndSpuNode *rxSndSpuHeapNewNode(void);                                 // ROR 0x00239be8
void  rxSndXvabInit(void);                                               // ROR 0x0023a170
void  rxSndXvabTerm(void);                                               // ROR 0x0023a178
void  rxSndXseqInit(void);                                               // ROR 0x0023b018
void  rxSndXseqTerm(void);                                               // ROR 0x0023b020
u_int rxSndReadU32(const u_int *p);                                      // ROR 0x0023b028
void  rxSndBgmSetMasterVolume(int volL, int volR);                       // ROR 0x0023b238
void  rxSndBgmStop(int bgm, int arg);                                    // ROR 0x0023b3b8
void  rxSndBgmSetVolume(int bgm, int volL, int volR);                    // ROR 0x0023b840
void  rxSndBgmInit(void);                                                // ROR 0x0023bb78
void  rxSndBgmTerm(void);                                                // ROR 0x0023bbd0
void  rxSndBgmUpdate(const RxSndBgmStatus *status);                      // ROR 0x0023bbd8
short rxSndBgmGetMasterVolumeL(void);                                    // ROR 0x0023bd50
short rxSndBgmGetMasterVolumeR(void);                                    // ROR 0x0023bd60
void  rxSndSeSetMasterVolume(int volL, int volR);                        // ROR 0x0023bd70
void  rxSndSeInit(void);                                                 // ROR 0x0023c910
void  rxSndSeTerm(void);                                                 // ROR 0x0023c970
void  rxSndSeUpdate(const RxSndSeStatus *status);                        // ROR 0x0023c978
short rxSndSeGetMasterVolumeL(void);                                     // ROR 0x0023cae8
short rxSndSeGetMasterVolumeR(void);                                     // ROR 0x0023caf8
void  rxSndPcmSetMasterVolume(int volL, int volR);                       // ROR 0x0023cb08
void  rxSndPcmSetVolume(int pcm, int volL, int volR);                    // ROR 0x0023d7e0
void  rxSndPcmInit(void);                                                // ROR 0x0023d908
void  rxSndPcmTerm(void);                                                // ROR 0x0023d998
void  rxSndPcmUpdate(const RxSndPcmStatus *status);                      // ROR 0x0023d9a0
short rxSndPcmGetMasterVolumeL(void);                                    // ROR 0x0023dbb0
short rxSndPcmGetMasterVolumeR(void);                                    // ROR 0x0023dbc0
void  rxSndAdpcmSetMasterVolume(int volL, int volR);                     // ROR 0x0023dbd0
/* The bytes of the stream's IOP buffer that are free; raw == 0 keeps 0x80
 * per channel in reserve. */
int   rxSndAdpcmGetFreeSize(int stream, int raw);                        // ROR 0x0023e1a0
void  rxSndAdpcmInit(void);                                              // ROR 0x0023f648
void  rxSndAdpcmTerm(void);                                              // ROR 0x0023f6e0
void  rxSndAdpcmUpdate(const RxSndAdpcmStatus *status);                  // ROR 0x0023f6e8
short rxSndAdpcmGetMasterVolumeL(void);                                  // ROR 0x0023f940
short rxSndAdpcmGetMasterVolumeR(void);                                  // ROR 0x0023f950

/* ---- transport --------------------------------------------------------- */
int   rxSndCreateSema(int maxCount, int initCount);                      // ROR 0x0023f960
/* Binds RPC `rpcNo`, waiting until the IOP serves it; 1 bound, 0 error. */
int   rxSndBindRpc(sceSifClientData *cd, u_int rpcNo);                   // ROR 0x0023f9b0
void  rxSndUnbindRpc(sceSifClientData *cd);                              // ROR 0x0023faa8
void  rxSndRpcEndFunc(void);                                             // ROR 0x0023fac8
void  rxSndCallRpc(sceSifClientData *cd, u_int fno, void *send, int sendSize,
                   void *reply, int replySize);                          // ROR 0x0023faf8
void  rxSndRpcInit(void);                                                // ROR 0x0023fbc0
void  rxSndRpcTerm(void);                                                // ROR 0x0023fbf8
/* Sends command `cmd` at once: `send` goes out, the reply's words 1.. come
 * back in `reply`; returns the reply's word 0. */
u_int rxSndRpcCommand(u_int cmd, void *reply, int replySize,
                      const void *send, int sendSize);                   // ROR 0x0023fc18
/* Queues command `cmd` with `dataSize` bytes of data (write them through
 * rxSndQueueGetData) and `replySize` bytes of reply (rxSndQueueGetReply,
 * valid after the next flush). */
void  rxSndQueueBegin(u_int cmd, int replySize, int dataSize);           // ROR 0x0023fda8
void *rxSndQueueGetReply(void);                                          // ROR 0x0023fe40
void *rxSndQueueGetData(void);                                           // ROR 0x0023fe58
void  rxSndQueueEnd(void);                                               // ROR 0x0023fe70
void  rxSndQueueFlush(void);                                             // ROR 0x0023feb8

/* ---- what SndMan calls (later slices write the rest) ------------------- */

/* XVAB banks: Load returns the bank's id (id -1 takes a free one), -1 on
 * failure; Check is 1 while the transfer runs (wait 1 waits for it). */
int   rxSndXvabLoad(int id, const char *name, int arg2, int arg3);       // ROR 0x0023a1b8
int   rxSndXvabFree(int id, int arg);                                    // ROR 0x0023a5a8
int   rxSndXvabCheck(int id, int wait);                                  // ROR 0x0023a720

/* SE voices of an XVAB. Status: bit 0 open, bit 2 ended; 0 when closed. */
int   rxSndSePlay(int xvab, int prog, int note, int volL, int volR, int arg); // ROR 0x0023be68
void  rxSndSeStop(int voice, int arg);                                   // ROR 0x0023c0f8
u_int rxSndSeGetStatus(int voice);                                       // ROR 0x0023c228
void  rxSndSePause(int voice, int pause, int arg);                       // ROR 0x0023c260
/* IOP command 0x82, parameter 2 (pan or an effect level: not known). */
void  rxSndSeSetAttr(int voice, int value);                              // ROR 0x0023c328
void  rxSndSeSetVolume(int voice, int volL, int volR);                   // ROR 0x0023c3c0
void  rxSndSeSetPitch(int voice, float pitch);                           // ROR 0x0023c510

/* ADPCM streams fed from memory. Status: bit 0 open, bit 2 ended, bit 3
 * paused. GetFree: the bytes Write can take now. */
int   rxSndAdpcmOpen(int mode, int rate, int arg2, int arg3, int arg4, int arg5); // ROR 0x0023dcc8
void  rxSndAdpcmClose(int stream);                                       // ROR 0x0023e020
u_int rxSndAdpcmGetStatus(int stream);                                   // ROR 0x0023e158
int   rxSndAdpcmGetFree(int stream);                                     // ROR 0x0023e240
void  rxSndAdpcmWrite(int stream, const void *data, int offset, int size); // ROR 0x0023eb10
/* No more data: the stream plays out what it has. */
void  rxSndAdpcmEnd(int stream);                                         // ROR 0x0023eb28
void  rxSndAdpcmPause(int stream, int pause);                            // ROR 0x0023eef8
/* IOP command 0xa2, parameter 2 (the stream twin of rxSndSeSetAttr). */
void  rxSndAdpcmSetAttr(int stream, int value);                          // ROR 0x0023ef98
void  rxSndAdpcmSetVolume(int stream, int volL, int volR);               // ROR 0x0023f038
void  rxSndAdpcmSetPitch(int stream, float pitch);                       // ROR 0x0023f310

#endif /* _RX_RXSND_H */
