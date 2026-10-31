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
    u_short m_param;                    // ROR +0x4: 3 volume, 5 not known
    u_short m_6;                        // ROR +0x6: always 0
    union
    {
        short m_vol[2];                 // ROR +0x8: L, R (the other side 0)
        int   m_value;                  // ROR +0x8: parameter 5
    };
};
static_assert(sizeof(RxSndCmdChannelSet) == 0xc, "RxSndCmdChannelSet");
static_assert(offsetof(RxSndCmdChannelSet, m_channel) == 0x2, "RxSndCmdChannelSet::m_channel");
static_assert(offsetof(RxSndCmdChannelSet, m_param) == 0x4, "RxSndCmdChannelSet::m_param");
static_assert(offsetof(RxSndCmdChannelSet, m_vol) == 0x8, "RxSndCmdChannelSet::m_vol");

/* Queued command 0xb0: open a PCM stream. EE 0xc. */
struct RxSndCmdPcmOpen
{
    u_char m_id;                        // ROR +0x0
    u_char m_mode;                      // ROR +0x1
    u_char _2[2];
    u_int  m_iopBuf;                    // ROR +0x4
    int    m_blocks;                    // ROR +0x8
};
static_assert(sizeof(RxSndCmdPcmOpen) == 0xc, "RxSndCmdPcmOpen");
static_assert(offsetof(RxSndCmdPcmOpen, m_mode) == 0x1, "RxSndCmdPcmOpen::m_mode");
static_assert(offsetof(RxSndCmdPcmOpen, m_iopBuf) == 0x4, "RxSndCmdPcmOpen::m_iopBuf");
static_assert(offsetof(RxSndCmdPcmOpen, m_blocks) == 0x8, "RxSndCmdPcmOpen::m_blocks");

/* Queued command 0xb1: m_count more blocks of a PCM stream are filled; their
 * sequence numbers (int each) follow. EE 0x4 + 4 * m_count. */
struct RxSndCmdPcmData
{
    u_char m_id;                        // ROR +0x0
    u_char m_count;                     // ROR +0x1
    u_char _2[2];
};
static_assert(sizeof(RxSndCmdPcmData) == 0x4, "RxSndCmdPcmData");
static_assert(offsetof(RxSndCmdPcmData, m_count) == 0x1, "RxSndCmdPcmData::m_count");

/* Commands 0x73 / 0x74: one channel's parameter of a BGM slot. EE 0xc. */
struct RxSndCmdBgmChannelSet
{
    u_char m_id;                        // ROR +0x0
    u_char m_channel;                   // ROR +0x1
    u_char _2[2];
    int    m_param;                     // ROR +0x4: 3 volume
    short  m_vol[2];                    // ROR +0x8: L, R
};
static_assert(sizeof(RxSndCmdBgmChannelSet) == 0xc, "RxSndCmdBgmChannelSet");
static_assert(offsetof(RxSndCmdBgmChannelSet, m_channel) == 0x1, "RxSndCmdBgmChannelSet::m_channel");
static_assert(offsetof(RxSndCmdBgmChannelSet, m_param) == 0x4, "RxSndCmdBgmChannelSet::m_param");
static_assert(offsetof(RxSndCmdBgmChannelSet, m_vol) == 0x8, "RxSndCmdBgmChannelSet::m_vol");

/* Command 0x40: an EE -> SPU transfer. EE 0x10. */
struct RxSndCmdTransfer
{
    u_char m_type;                      // ROR +0x0: 3
    u_char m_dir;                       // ROR +0x1: 1
    u_char _2[2];
    u_int  m_spuAddr;                   // ROR +0x4
    u_int  m_eeAddr;                    // ROR +0x8
    int    m_size;                      // ROR +0xc
};
static_assert(sizeof(RxSndCmdTransfer) == 0x10, "RxSndCmdTransfer");
static_assert(offsetof(RxSndCmdTransfer, m_dir) == 0x1, "RxSndCmdTransfer::m_dir");
static_assert(offsetof(RxSndCmdTransfer, m_spuAddr) == 0x4, "RxSndCmdTransfer::m_spuAddr");
static_assert(offsetof(RxSndCmdTransfer, m_eeAddr) == 0x8, "RxSndCmdTransfer::m_eeAddr");
static_assert(offsetof(RxSndCmdTransfer, m_size) == 0xc, "RxSndCmdTransfer::m_size");

/* Queued commands 0x80 / 0x81: start an SE voice. EE 0x4. */
struct RxSndCmdSePlay
{
    u_char m_voice;                     // ROR +0x0
    u_char m_xvab;                      // ROR +0x1
    u_char m_prog;                      // ROR +0x2
    u_char m_note;                      // ROR +0x3
};
static_assert(sizeof(RxSndCmdSePlay) == 0x4, "RxSndCmdSePlay");
static_assert(offsetof(RxSndCmdSePlay, m_xvab) == 0x1, "RxSndCmdSePlay::m_xvab");
static_assert(offsetof(RxSndCmdSePlay, m_prog) == 0x2, "RxSndCmdSePlay::m_prog");
static_assert(offsetof(RxSndCmdSePlay, m_note) == 0x3, "RxSndCmdSePlay::m_note");

/* Queued command 0xa0: open an ADPCM stream; m_channels RxSndCmdAdpcmBuf
 * follow. EE 0x10. */
struct RxSndCmdAdpcmOpen
{
    u_char m_id;                        // ROR +0x0
    u_char _1;
    u_char m_channels;                  // ROR +0x2
    u_char _3;
    short  m_arg;                       // ROR +0x4: rxSndAdpcmOpenEx's argument, -1
    short  m_pitch;                     // ROR +0x6: rate * 4096 / 48000
    int    m_iopSize;                   // ROR +0x8: per channel
    int    m_spuSize;                   // ROR +0xc: per channel
};
static_assert(sizeof(RxSndCmdAdpcmOpen) == 0x10, "RxSndCmdAdpcmOpen");
static_assert(offsetof(RxSndCmdAdpcmOpen, m_channels) == 0x2, "RxSndCmdAdpcmOpen::m_channels");
static_assert(offsetof(RxSndCmdAdpcmOpen, m_arg) == 0x4, "RxSndCmdAdpcmOpen::m_arg");
static_assert(offsetof(RxSndCmdAdpcmOpen, m_pitch) == 0x6, "RxSndCmdAdpcmOpen::m_pitch");
static_assert(offsetof(RxSndCmdAdpcmOpen, m_iopSize) == 0x8, "RxSndCmdAdpcmOpen::m_iopSize");
static_assert(offsetof(RxSndCmdAdpcmOpen, m_spuSize) == 0xc, "RxSndCmdAdpcmOpen::m_spuSize");

struct RxSndCmdAdpcmBuf                 /* EE 0x8 */
{
    u_int m_iopBuf;                     // ROR +0x0
    u_int m_spuBuf;                     // ROR +0x4
};
static_assert(sizeof(RxSndCmdAdpcmBuf) == 0x8, "RxSndCmdAdpcmBuf");
static_assert(offsetof(RxSndCmdAdpcmBuf, m_spuBuf) == 0x4, "RxSndCmdAdpcmBuf::m_spuBuf");

/* Queued command 0xa1: `m_size` more bytes per channel are in the IOP ring
 * (whole 0x40 units); m_end 1: the last of them, the end block. EE 0x8. */
struct RxSndCmdAdpcmData
{
    u_char m_id;                        // ROR +0x0
    u_char m_end;                       // ROR +0x1
    u_char _2[2];
    int    m_size;                      // ROR +0x4
};
static_assert(sizeof(RxSndCmdAdpcmData) == 0x8, "RxSndCmdAdpcmData");
static_assert(offsetof(RxSndCmdAdpcmData, m_end) == 0x1, "RxSndCmdAdpcmData::m_end");
static_assert(offsetof(RxSndCmdAdpcmData, m_size) == 0x4, "RxSndCmdAdpcmData::m_size");

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
    short  m_2;                         // ROR +0x2: the IOP's filled blocks not yet mixed (pcm.cpp), stored in RxSndPcm::m_queued
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

/* bss 0x7b6b00: the bounce buffers of the unaligned EE <-> IOP copies, one
 * per buffer bank: EE ones (rxSndIopRead) and two IOP ones taken at init. EE 0x80. */
struct RxSndIopBuf
{
    u_char m_eeBuf[2][0x20];            // ROR +0x0: EE side, rxSndIopRead's bounce
    u_int m_buf[2];                     // ROR +0x40: IOP address, one per buffer bank (rxSndIopWrite's bounce)
    u_int _48[0xe];
};
static_assert(sizeof(RxSndIopBuf) == 0x80, "RxSndIopBuf");
static_assert(offsetof(RxSndIopBuf, m_buf) == 0x40, "RxSndIopBuf::m_buf");

/* A block of the SPU heap (bss 0x7b6b80, 256 of them), on a circular list
 * through the sentinel 0x7b8380: m_prev runs up the SPU addresses from
 * 0x5010, m_next down from 0x1c0000. A block's address is the sum of the
 * sizes before it (m_addr is only set at init). layout: host -- pointers.
 * EE 0x18. */
struct RxSndSpuSlot;
struct RxSndSpuNode
{
    RxSndSpuNode *m_next;               // ROR +0x0
    RxSndSpuNode *m_prev;               // ROR +0x4: also the free node list's link
    RxSndSpuSlot *m_owner;              // ROR +0x8: the slot it was allocated for
    int           m_used;               // ROR +0xc
    u_int         m_addr;               // ROR +0x10: SPU address (init only)
    u_int         m_size;               // ROR +0x14
};

/* A window of SPU memory the allocator may place blocks in (bss 0x7b83a0,
 * 256 of them; rxSndSpuHeapGetSlot). EE 0x14. */
struct RxSndSpuSlot
{
    int   m_index;                      // ROR +0x0: -1 for the whole region
    int   m_used;                       // ROR +0x4
    u_int m_addr;                       // ROR +0x8: SPU address
    u_int m_size;                       // ROR +0xc
    int   m_count;                      // ROR +0x10: blocks allocated in it
};
static_assert(sizeof(RxSndSpuSlot) == 0x14, "RxSndSpuSlot");
static_assert(offsetof(RxSndSpuSlot, m_used) == 0x4, "RxSndSpuSlot::m_used");
static_assert(offsetof(RxSndSpuSlot, m_addr) == 0x8, "RxSndSpuSlot::m_addr");
static_assert(offsetof(RxSndSpuSlot, m_size) == 0xc, "RxSndSpuSlot::m_size");
static_assert(offsetof(RxSndSpuSlot, m_count) == 0x10, "RxSndSpuSlot::m_count");

/* bss 0x7b97a0: the whole SPU heap as a slot, and the slot each buffer bank
 * allocates from (-1: the whole region). EE 0x20. */
struct RxSndSpuRegion
{
    RxSndSpuSlot m_slot;                // ROR +0x0: index -1, used, 0x5010, 0x1baff0
    int          m_bankSlot[2];         // ROR +0x14
    int          _1c;
};
static_assert(sizeof(RxSndSpuRegion) == 0x20, "RxSndSpuRegion");
static_assert(offsetof(RxSndSpuRegion, m_bankSlot) == 0x14, "RxSndSpuRegion::m_bankSlot");

/* An XVAB bank (bss 0x7b97c0, 256 of them; filled by rxSndXvabLoad). EE 0x18. */
struct RxSndXvab
{
    u_int  m_flags;                     // ROR +0x0: bit 0 used, 0x10000 / 0x20000 m_iopBuf / m_spuBuf are ours, 0x40000 samples sent
    u_int  m_iopBuf;                    // ROR +0x4: IOP address of the bank file
    int    m_iopSize;                   // ROR +0x8
    u_int  m_spuBuf;                    // ROR +0xc: SPU address of the samples
    int    m_spuSize;                   // ROR +0x10
    int    m_transfer;                  // ROR +0x14: IOP transfer handle, -1 when done
};
static_assert(sizeof(RxSndXvab) == 0x18, "RxSndXvab");
static_assert(offsetof(RxSndXvab, m_iopBuf) == 0x4, "RxSndXvab::m_iopBuf");
static_assert(offsetof(RxSndXvab, m_iopSize) == 0x8, "RxSndXvab::m_iopSize");
static_assert(offsetof(RxSndXvab, m_spuBuf) == 0xc, "RxSndXvab::m_spuBuf");
static_assert(offsetof(RxSndXvab, m_spuSize) == 0x10, "RxSndXvab::m_spuSize");
static_assert(offsetof(RxSndXvab, m_transfer) == 0x14, "RxSndXvab::m_transfer");

/* An XSEQ sequence (bss 0x7bafc0, 256 of them; filled by rxSndXseqLoad). EE 0xc. */
struct RxSndXseq
{
    u_int m_flags;                      // ROR +0x0: bit 0 used, 0x10000 m_iopBuf is ours
    u_int m_iopBuf;                     // ROR +0x4: IOP address of the file
    int   m_iopSize;                    // ROR +0x8
};
static_assert(sizeof(RxSndXseq) == 0xc, "RxSndXseq");
static_assert(offsetof(RxSndXseq, m_iopBuf) == 0x4, "RxSndXseq::m_iopBuf");
static_assert(offsetof(RxSndXseq, m_iopSize) == 0x8, "RxSndXseq::m_iopSize");

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

/* A PCM stream (bss 0x7bd804, 32): an IOP ring of m_blocks blocks of
 * m_blockSize bytes (mode 0: 0x800, modes 1..3: 0x1000; mode 1 keeps the
 * left half of each block in the first m_blocks * 0x800 bytes and the right
 * half after them). Whole blocks are announced to the IOP with a running
 * sequence number. layout: host -- pointers. EE 0x3c. */
struct RxSndPcm
{
    RxSndPcm         *m_next;           // ROR +0x0
    int               m_id;             // ROR +0x4
    int               m_user;           // ROR +0x8: rxSndPcmSetUserData, unread by the library
    u_int             m_iopBuf;         // ROR +0xc: IOP address
    int               m_blocks;         // ROR +0x10
    int               m_blockSize;      // ROR +0x14
    u_char            m_status;         // ROR +0x18: 1 open, 2 playing, 4 ended, 8 paused
    u_char            m_mode;           // ROR +0x19: 0..3
    u_char            m_alloc;          // ROR +0x1a: bit 0 m_iopBuf is ours
    u_char            _1b;
    short             m_volL;           // ROR +0x1c
    short             m_volR;           // ROR +0x1e
    int               m_queued;         // ROR +0x20: blocks the IOP holds (RxSndPcmStatusEntry::m_2)
    int               m_ringSize;       // ROR +0x24: m_blocks * m_blockSize
    int               m_writePos;       // ROR +0x28: next write offset in the ring
    int               m_pending;        // ROR +0x2c: bytes written past the last whole block
    int               m_seq;            // ROR +0x30: sequence number of the next block
    int               m_attr;           // ROR +0x34: rxSndPcmSetAttr; rxSndPcmEnd's mode 3 padding
    RxSndPosCallback *m_callbacks;      // ROR +0x38
};

/* An ADPCM stream (bss 0x7c03c8, 48). Each channel has m_bufSize bytes of
 * IOP ring at m_iopBuf + channel * m_bufSize and m_spuSize bytes of SPU
 * buffer. layout: host -- pointers. EE 0x50. */
struct RxSndAdpcm
{
    RxSndAdpcm       *m_next;           // ROR +0x0
    int               m_mode;           // ROR +0x4: 0 mono, 1 stereo from two buffers, 2 stereo interleaved, 0x1000000 n channels interleaved
    u_char            m_id;             // ROR +0x8
    u_char            m_status;         // ROR +0x9: 1 open, 2 playing, 4 ended, 8 paused
    u_char            m_alloc;          // ROR +0xa: bit 0 m_iopBuf, bit 1 m_spuBuf are ours
    u_char            m_end;            // ROR +0xb: rxSndAdpcmEnd sent the end block
    u_char            m_channels;       // ROR +0xc
    u_char            _d[3];
    int               m_user;           // ROR +0x10: rxSndAdpcmSetUserData, 0 at open
    short             m_volL;           // ROR +0x14
    short             m_volR;           // ROR +0x16
    u_int             m_iopBuf;         // ROR +0x18: IOP address
    int               m_bufSize;        // ROR +0x1c: per channel
    int               m_bufFill;        // ROR +0x20: m_bufSize - the IOP's free bytes
    int               m_writePos;       // ROR +0x24: next write offset in the ring
    int               m_pending;        // ROR +0x28: bytes written, not yet announced (< 0x40)
    u_int             m_spuBuf;         // ROR +0x2c: SPU address
    int               m_spuSize;        // ROR +0x30: per channel
    int               m_spuPos;         // ROR +0x34: write position modulo m_spuSize
    u_int             m_writtenHi;      // ROR +0x38: bytes written per channel, 64-bit
    u_int             m_writtenLo;      // ROR +0x3c
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
/* Command 0x30 {2, mode != 0} (stereo / mono output: not known). */
void  rxSndSetOutputMode(int mode);                                      // ROR 0x00237f90
/* Stores the flags (command 0x30 {0, flags} tells the IOP); the old ones back. */
u_char rxSndSetDebugFlags(int flags);                                   // ROR 0x00237fe0
u_char rxSndGetDebugFlags(void);                                         // ROR 0x00238038
/* Command 0x30 {1, rate}: 50 passes, anything else sends 60. */
void  rxSndSetFrameRate(int rate);                                       // ROR 0x00238048
/* Command 0x30 {3 + kind, value} for kind 0..2 (meaning not known). */
void  rxSndSetSysParam(int kind, int value);                             // ROR 0x002380a0
/* Queued command 0x31 {id, 1, L, R}. */
void  rxSndSetSysVolume(int id, int volL, int volR);                     // ROR 0x00238120
void  rxSndSetMasterVolume(int volL, int volR);                          // ROR 0x00238188
int   rxSndSetBufferBank(int bank);                                      // ROR 0x00238268
/* Commands 0x90 / 0x93 / 0x92: 48 words, one per SPU2 voice; 0x91 sets one. */
void  rxSndGetVoiceTable(u_int *out);                                    // ROR 0x002382a0
void  rxSndGetVoiceTableEx(int arg, u_int *out);                         // ROR 0x00238300
void  rxSndSetVoiceEntry(u_int voice, int value);                        // ROR 0x00238360
void  rxSndGetVoiceTable2(u_int *out);                                   // ROR 0x002383a8
/* The pitch ratio of `cents` (clamped to +-4800): 2^(cents / 1200) by table. */
float rxSndCentToPitch(int cents);                                       // ROR 0x00238408
void  rxSndInitConfig(void);                                             // ROR 0x00238478
void  rxSndTermConfig(void);                                             // ROR 0x002384a8
int   rxSndGetBufferBank(void);                                          // ROR 0x002384b8
short rxSndGetMasterVolumeL(void);                                       // ROR 0x002384c8
short rxSndGetMasterVolumeR(void);                                       // ROR 0x002384d8
void *rxSndGetWorkBuffer(void);                                          // ROR 0x002384e8
void  rxSndWait(void);                                                   // ROR 0x002384f8
void  rxSndSetBgCallHook(RxSndBgCallHook hook);                          // ROR 0x00238500
void  rxSndCallBgCallHook(const char *func);                             // ROR 0x00238510
void  rxSndMemMove(void *dst, const void *src, int size);              // ROR 0x00238538
u_int rxSndDefIopAlloc(int size);                                        // ROR 0x002385b8
u_int rxSndDefIopAllocMode1(int size);                                   // ROR 0x00238630
void  rxSndDefIopFree(u_int addr);                                       // ROR 0x002386b0
void  rxSndSetIopAllocHook(RxSndAllocFunc func);                         // ROR 0x002386e0
void  rxSndSetIopAllocMode1Hook(RxSndAllocFunc func);                    // ROR 0x00238700
void  rxSndSetIopFreeHook(RxSndFreeFunc func);                           // ROR 0x00238720
u_int rxSndIopAlloc(int size);                                           // ROR 0x00238740
u_int rxSndIopAllocMode1(int size);                                      // ROR 0x00238768
void  rxSndIopFree(u_int addr);                                          // ROR 0x00238790
/* Copies `size` bytes of EE memory to IOP address `iopAddr`: one SIF DMA
 * when everything is 16-byte aligned, else the unaligned head and tail go
 * through a bounce buffer and IOP command 0x23. Returns 1. */
int   rxSndIopWrite(u_int iopAddr, const void *src, int size);           // ROR 0x002387b8
/* IOP address `iopAddr` -> EE `dst`, the mirror of rxSndIopWrite. Returns 1. */
int   rxSndIopRead(void *dst, u_int iopAddr, int size);                  // ROR 0x002389c8
/* Writes the work buffer to `iopAddr` in 0x1000-byte chunks until `size`
 * bytes went (the ROM never advances the address). Returns 1. */
int   rxSndIopWriteWorkBuffer(u_int iopAddr, int size);                  // ROR 0x00238bb0
/* rxSndIopAlloc + rxSndIopWrite; the IOP address, 0 when the heap is full. */
u_int rxSndIopAllocWrite(const void *src, int size);                     // ROR 0x00238c28
void  rxSndIopHeapCmd1(void);                                            // ROR 0x00238c80
void  rxSndIopHeapCmd2(void);                                            // ROR 0x00238cb8
void  rxSndIopCopy(u_int dst, u_int src, int size);                      // ROR 0x00238cf0
void  rxSndIopBufInit(void);                                             // ROR 0x00238d28
void  rxSndIopBufTerm(void);                                             // ROR 0x00238d78
/* Defines SPU slot `id` (-1: the first unused) over [addr, addr + size)
 * inside the heap; the id, -1 when out of range. */
int   rxSndSpuHeapSetSlot(int id, u_int addr, u_int size);               // ROR 0x00238dc8
void  rxSndSpuHeapReleaseSlot(u_int id);                                 // ROR 0x00238f00
/* The slot the current buffer bank allocates from (-1: the whole heap). */
int   rxSndSpuHeapSetBankSlot(int slot);                                 // ROR 0x00238f58
int   rxSndSpuHeapGetBankSlot(void);                                     // ROR 0x00238fb8
u_int rxSndDefSpuAlloc(int size);                                        // ROR 0x00238ff0
u_int rxSndDefSpuAllocMode1(int size);                                   // ROR 0x00239010
void  rxSndDefSpuFree(u_int addr);                                       // ROR 0x00239030
void  rxSndSetSpuAllocHook(RxSndAllocFunc func);                         // ROR 0x00239048
void  rxSndSetSpuAllocMode1Hook(RxSndAllocFunc func);                    // ROR 0x00239068
void  rxSndSetSpuFreeHook(RxSndFreeFunc func);                           // ROR 0x00239088
u_int rxSndSpuAlloc(int size);                                           // ROR 0x002390a8
u_int rxSndSpuAllocMode1(int size);                                      // ROR 0x002390d0
void  rxSndSpuFree(u_int addr);                                          // ROR 0x002390f8
/* Command 0x40: starts sending `size` bytes of EE memory to SPU address
 * `spuAddr`; the handle for rxSndXvabPoll, negative while the IOP is busy. */
int   rxSndSpuTransfer(u_int spuAddr, const void *src, int size);        // ROR 0x00239120
/* Command 0x40 with the other transfer types (directions not known). */
void  rxSndSpuTransferType4(u_int a, u_int b, int size);                 // ROR 0x00239168
void  rxSndSpuTransferType1(u_int a, u_int b, int size);                 // ROR 0x002391b0
void  rxSndSpuTransferType2(u_int a, u_int b, int size);                 // ROR 0x002391f8
void  rxSndSpuTransferType5(u_int a, int size);                          // ROR 0x00239240
/* Asks the IOP whether XVAB transfer `handle` is still running (command
 * 0x41); wait != 0 polls until it is done. */
int   rxSndXvabPoll(int handle, int wait);                               // ROR 0x00239288
u_int rxSndSpuHeapGetFreeSize(void);                                     // ROR 0x00239320
u_int rxSndSpuHeapGetMaxFreeBlock(void);                                 // ROR 0x00239330
void  rxSndSpuHeapInit(void);                                            // ROR 0x00239380
void  rxSndSpuHeapTerm(void);                                            // ROR 0x002394a0
RxSndSpuSlot *rxSndSpuHeapGetSlot(u_int id);                             // ROR 0x002394a8
/* First fit of `size` (rounded up to 0x40) in the current bank's slot:
 * mode 0 lowest address, 1 highest, 2 exactly at `addr`. Returns the SPU
 * address, 0 when it does not fit. */
u_int rxSndSpuHeapAlloc(int mode, u_int addr, int size);                 // ROR 0x002394e0
void  rxSndSpuHeapFree(u_int addr);                                      // ROR 0x002398f8
/* Frees every block allocated in slot `id`. */
void  rxSndSpuHeapFreeSlot(u_int id);                                    // ROR 0x00239af8
RxSndSpuNode *rxSndSpuHeapNewNode(void);                                 // ROR 0x00239be8
void  rxSndSpuHeapFreeNode(RxSndSpuNode *node);                          // ROR 0x00239c10
void  rxSndSpuHeapPrintStatus(void);                                     // ROR 0x00239c30
/* Queued command 0x32 {core, attribute, value}: the SPU2 cores' effect
 * (mode 0..9 as libsd's reverb modes, depth L / R, delay, feedback). */
void  rxSndSetEffectModeCore(int core, int mode);                        // ROR 0x00239e58
void  rxSndSetEffectDepthCore(int core, int depthL, int depthR);         // ROR 0x00239ef8
void  rxSndSetEffectDelayCore(int core, int delay);                      // ROR 0x00239f80
void  rxSndSetEffectFeedbackCore(int core, int feedback);                // ROR 0x0023a000
void  rxSndSetEffectMode(int mode);                                      // ROR 0x0023a080
void  rxSndSetEffectDepth(int depthL, int depthR);                       // ROR 0x0023a0b8
void  rxSndSetEffectDelay(int delay);                                    // ROR 0x0023a100
void  rxSndSetEffectFeedback(int feedback);                              // ROR 0x0023a138
void  rxSndXvabInit(void);                                               // ROR 0x0023a170
void  rxSndXvabTerm(void);                                               // ROR 0x0023a178
/* A MIDI variable-length number at *p (7 bits a byte, 0x80 continues). */
u_int rxSndReadVarLen(const u_char **p);                                 // ROR 0x0023a180
/* The bank's SPU samples: 1 and their address / size, or 0 when it has none. */
int   rxSndXvabGetSampleData(const u_char **data, int *size, const void *xvab); // ROR 0x0023a930
u_int rxSndXvabGetIopAddr(int id);                                       // ROR 0x0023a7f0
u_int rxSndXvabGetSpuAddr(int id);                                       // ROR 0x0023a880
int   rxSndXvabGetFileSize(const void *xvab);                            // ROR 0x0023a918
const u_char *rxSndXvabGetSampleAddr(const void *xvab);                  // ROR 0x0023aa08
int   rxSndXvabGetSampleSize(const void *xvab);                          // ROR 0x0023aa38
/* XSEQ sequences: Load as rxSndXvabLoad (no SPU part; command 0x60), the id
 * or -1; Free as rxSndXvabFree (command 0x61). */
int   rxSndXseqLoad(int id, const void *xseq, u_int iopAddr);            // ROR 0x0023aa68
int   rxSndXseqFree(int id, int wait);                                   // ROR 0x0023ad60
/* Command 0x63 {id, a, b} (meaning not known). */
void  rxSndXseqSetParam(int id, int a, int b);                           // ROR 0x0023aeb0
u_int rxSndXseqGetIopAddr(int id);                                       // ROR 0x0023af70
int   rxSndXseqGetFileSize(const void *xseq);                            // ROR 0x0023b000
void  rxSndXseqInit(void);                                               // ROR 0x0023b018
void  rxSndXseqTerm(void);                                               // ROR 0x0023b020
u_int rxSndReadU32(const void *p);                                       // ROR 0x0023b028
/* XVAB / XSEQ files: words {magic, version, size}, then chunks {tag, size}
 * (version >= 0x200: a 16-byte file header and chunks {tag, ?, size, ?}). */
u_int rxSndChunkGetVersion(const void *file);                            // ROR 0x0023b030
int   rxSndChunkGetFileSize(const void *file);                           // ROR 0x0023b038
int   rxSndChunkFind(const u_char **data, int *size, const void *file, u_int tag); // ROR 0x0023b078
const u_char *rxSndChunkFindData(const void *file, u_int tag);           // ROR 0x0023b1c8
int   rxSndChunkGetSize(const void *file, u_int tag);                    // ROR 0x0023b200
void  rxSndBgmSetMasterVolume(int volL, int volR);                       // ROR 0x0023b238
/* Plays XSEQ `xseq` on a free BGM slot (command 0x70); the slot, -1 when
 * all 32 are taken. keep as rxSndSePlay. */
int   rxSndBgmPlay(int xseq, int volL, int volR, int keep);              // ROR 0x0023b2d0
void  rxSndBgmStop(int bgm, int arg);                                    // ROR 0x0023b3b8
void  rxSndBgmStopAll(int arg);                                          // ROR 0x0023b498
void  rxSndBgmPause(int bgm, int pause, int arg);                        // ROR 0x0023b4e8
u_int rxSndBgmGetStatus(int bgm);                                        // ROR 0x0023b5b0
u_int rxSndBgmGetValue(int bgm);                                         // ROR 0x0023b5e8
/* Queued commands 0x73 / 0x74 {slot, channel, 3, L, R}: raw volumes. */
void  rxSndBgmSetChannelVolume(int bgm, int channel, int volL, int volR);   // ROR 0x0023b628
void  rxSndBgmSetChannelVolume74(int bgm, int channel, int volL, int volR); // ROR 0x0023b6e8
void  rxSndBgmSetAttr(int bgm, int value);                               // ROR 0x0023b7a8
void  rxSndBgmSetVolume(int bgm, int volL, int volR);                    // ROR 0x0023b840
void  rxSndBgmSetPitch(int bgm, float pitch);                            // ROR 0x0023b990
void  rxSndBgmSetParam5(int bgm, float value);                           // ROR 0x0023ba38
void  rxSndBgmSetParam6(int bgm, int value);                             // ROR 0x0023bae0
void  rxSndBgmInit(void);                                                // ROR 0x0023bb78
void  rxSndBgmTerm(void);                                                // ROR 0x0023bbd0
void  rxSndBgmUpdate(const RxSndBgmStatus *status);                      // ROR 0x0023bbd8
short rxSndBgmGetMasterVolumeL(void);                                    // ROR 0x0023bd50
short rxSndBgmGetMasterVolumeR(void);                                    // ROR 0x0023bd60
void  rxSndSeSetMasterVolume(int volL, int volR);                        // ROR 0x0023bd70
/* rxSndSePlay's parameters 0xb / 0xc for every voice it starts (-1 none). */
void  rxSndSeSetDefaultParamB(int value);                                // ROR 0x0023be08
void  rxSndSeSetDefaultParamC(int value);                                // ROR 0x0023be38
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
int   rxSndXvabLoad(int id, const void *xvab, u_int iopAddr, u_int spuAddr); // ROR 0x0023a1b8
/* wait 0: 1 while the IOP still uses the bank; wait 1 waits for it. */
int   rxSndXvabFree(int id, int wait);                                   // ROR 0x0023a5a8
int   rxSndXvabCheck(int id, int wait);                                  // ROR 0x0023a720

/* SE voices of an XVAB. Status: bit 0 open, bit 2 ended; 0 when closed.
 * Play returns the voice, -1 when all 64 are taken; keep != 0 leaves an
 * ended voice open for its owner to stop. */
int   rxSndSePlay(int xvab, int prog, int note, int volL, int volR, int keep); // ROR 0x0023be68
/* rxSndSePlay with command 0x81. */
int   rxSndSePlayEx(int xvab, int prog, int note, int volL, int volR, int keep); // ROR 0x0023bfb0
void  rxSndSeStop(int voice, int arg);                                   // ROR 0x0023c0f8
void  rxSndSeStopAll(int arg);                                           // ROR 0x0023c1d8
u_int rxSndSeGetStatus(int voice);                                       // ROR 0x0023c228
void  rxSndSePause(int voice, int pause, int arg);                       // ROR 0x0023c260
/* Queued command 0x82 {voice, param, value}. */
void  rxSndSeSetParam(int voice, int param, int value);                  // ROR 0x0023c660
/* Parameters 6..9, 0..0x80. */
void  rxSndSeSetParam6(int voice, int value);                            // ROR 0x0023c700
void  rxSndSeSetParam7(int voice, int value);                            // ROR 0x0023c740
void  rxSndSeSetParam8(int voice, int value);                            // ROR 0x0023c780
void  rxSndSeSetParam9(int voice, int value);                            // ROR 0x0023c7c0
/* Parameter 10: pan -1..1 as 0..255. */
void  rxSndSeSetPan(int voice, float pan);                               // ROR 0x0023c800
void  rxSndSeSetParamB(int voice, int value);                            // ROR 0x0023c860
void  rxSndSeSetParamC(int voice, int value);                            // ROR 0x0023c8b8
/* IOP command 0x82, parameter 2 (pan or an effect level: not known). */
void  rxSndSeSetAttr(int voice, int value);                              // ROR 0x0023c328
void  rxSndSeSetVolume(int voice, int volL, int volR);                   // ROR 0x0023c3c0
void  rxSndSeSetPitch(int voice, float pitch);                           // ROR 0x0023c510
void  rxSndSeSetParam5(int voice, float value);                          // ROR 0x0023c5b8

/* ADPCM streams fed from memory. Status: bit 0 open, bit 2 ended, bit 3
 * paused. GetFree: the bytes Write can take now. */
/* Opens a stream: mode (RxSndAdpcm::m_mode), sample rate, IOP ring and SPU
 * buffer per channel (address 0: allocated here; sizes in KB). Returns the
 * stream, -1 on failure. */
int   rxSndAdpcmOpen(int mode, int rate, u_int iopAddr, int iopKB, u_int spuAddr, int spuKB); // ROR 0x0023dcc8
int   rxSndAdpcmOpenEx(int mode, int rate, u_int iopAddr, int iopKB, u_int spuAddr, int spuKB,
                       int arg);                                         // ROR 0x0023dce8
int   rxSndAdpcmOpenBytes(int mode, int rate, u_int iopAddr, int iopSize, u_int spuAddr,
                          int spuSize, int arg);                         // ROR 0x0023dd08
void  rxSndAdpcmClose(int stream);                                       // ROR 0x0023e020
u_int rxSndAdpcmGetStatus(int stream);                                   // ROR 0x0023e158
int   rxSndAdpcmGetFree(int stream);                                     // ROR 0x0023e240
/* Feeds `size` bytes of ADPCM (mode 1: the second channel's from dataR);
 * takes what fits in the IOP ring. Returns size. */
int   rxSndAdpcmWrite(int stream, const void *data, const void *dataR, int size); // ROR 0x0023eb10
int   rxSndAdpcmWriteEx(int stream, const void *data, const void *dataR, int size,
                        int raw);                                        // ROR 0x0023e258
/* No more data: the stream plays out what it has. */
void  rxSndAdpcmEnd(int stream);                                         // ROR 0x0023eb28
void  rxSndAdpcmPause(int stream, int pause);                            // ROR 0x0023eef8
/* IOP command 0xa2, parameter 2 (the stream twin of rxSndSeSetAttr). */
void  rxSndAdpcmSetAttr(int stream, int value);                          // ROR 0x0023ef98
void  rxSndAdpcmSetVolume(int stream, int volL, int volR);               // ROR 0x0023f038
void  rxSndAdpcmSetPitch(int stream, float pitch);                       // ROR 0x0023f310
/* Queued command 0xa2 parameters 6 (0..2) and 7 (0..0x7f). */
void  rxSndAdpcmSetParam6(int stream, int value);                        // ROR 0x0023f470
void  rxSndAdpcmSetParam7(int stream, int value);                        // ROR 0x0023f538
/* rxSndAdpcmOpenBytes's defaults for parameters 6 (0..2) and 7 (0..0x7f). */
void  rxSndAdpcmSetDefaultParam6(int value);                             // ROR 0x0023dc68
void  rxSndAdpcmSetDefaultParam7(int value);                             // ROR 0x0023dc98
void  rxSndAdpcmCloseAll(void);                                          // ROR 0x0023e118
/* Calls func(stream, arg) once the play position passes `offset` bytes
 * (all channels) after what has been written so far. 0 when no node is free. */
int   rxSndAdpcmAddCallback(int stream, int offset, RxSndPosFunc func, int arg); // ROR 0x0023ed10
int   rxSndAdpcmSetUserData(int stream, int data);                       // ROR 0x0023ee68
int   rxSndAdpcmGetUserData(int stream);                                 // ROR 0x0023eeb0
/* Command 0xa4 parameter 3 for one channel, without the master volumes. */
void  rxSndAdpcmSetChannelVolume(int stream, int channel, int volL, int volR); // ROR 0x0023f1d8
/* Command 0xa4 parameter 5 (not known) for one channel. */
void  rxSndAdpcmSetChannelParam5(int stream, int channel, int value);    // ROR 0x0023f3c0
u_long rxSndAdpcmGetPosition(int stream);                                // ROR 0x0023f600

/* PCM streams fed from memory, the block-ring twin of the ADPCM streams
 * (the sound driver's slot table at 0x298f68 pairs them). Status: bit 0
 * open, bit 2 ended, bit 3 paused. */
/* Queued command 0x31 {0, 0, 0xffc when on, else 0xfcc}: mix bits (not known). */
void  rxSndSetSysMix(int on);                                            // ROR 0x0023cba0
/* Queued command 0x31 {0, 2, L, R}: 0x7fff both, 0 both when muted. */
void  rxSndSetSysMute(int mute);                                         // ROR 0x0023cbf8
/* Queued command 0xb4 {value} (not known). */
void  rxSndPcmSetParamB4(int value);                                     // ROR 0x0023cc68
/* Opens a stream of `blocks` blocks (RxSndPcm); iopAddr 0: allocated here.
 * Returns the stream, -1 on failure. */
int   rxSndPcmOpen(int mode, u_int iopAddr, int blocks);                 // ROR 0x0023cca8
void  rxSndPcmClose(int stream);                                         // ROR 0x0023ce98
void  rxSndPcmCloseAll(void);                                            // ROR 0x0023cf90
u_int rxSndPcmGetStatus(int stream);                                     // ROR 0x0023cfd0
/* The bytes rxSndPcmWrite can take now. */
int   rxSndPcmGetFree(int stream);                                       // ROR 0x0023d018
/* Feeds up to `size` bytes (mode 1: half from data, half from dataR);
 * returns what was taken. */
int   rxSndPcmWrite(int stream, const void *data, const void *dataR, int size); // ROR 0x0023d078
/* Pads the last block and tells the IOP no more data comes. */
void  rxSndPcmEnd(int stream);                                           // ROR 0x0023d360
/* Calls func(stream, arg) once the play position passes `offset` bytes
 * after what has been written so far. 0 when no node is free. */
int   rxSndPcmAddCallback(int stream, int offset, RxSndPosFunc func, int arg); // ROR 0x0023d4b8
int   rxSndPcmSetUserData(int stream, int data);                         // ROR 0x0023d610
int   rxSndPcmGetUserData(int stream);                                   // ROR 0x0023d658
/* Command 0xb2 parameter 2 (the stream twin of rxSndSeSetAttr). */
void  rxSndPcmSetAttr(int stream, int value);                            // ROR 0x0023d6a0
void  rxSndPcmPause(int stream, int pause);                              // ROR 0x0023d740

#endif /* _RX_RXSND_H */
