/* ==========================================================================
 *  rxsnddrv.h  (RXSNDDRV.IRX, Rule of Rose's IOP sound driver)
 *
 *  "Sound driver version 1.00": the server of SIF RPC 0xd000 that the EE's
 *  rxSnd* functions (rx/rxsnd.cpp) call.  The module is the only game code on
 *  the IOP; it is reconstructed from MODULES/RXSNDDRV.IRX (linked at 0, gp
 *  0x164e0, no symbols), one file per subsystem in link order:
 *
 *    rxsnddrv.cpp  0x0000..0x0b08  entry, double-buffer thread, dispatcher
 *    rpc.cpp       0x0b08..0x0c60  the RPC server thread
 *    dma.cpp       0x0c60..0x1680  SPU DMA queues and their thread
 *    sys.cpp       0x1680..0x19e0  system parameters (debug, rate, output)
 *    mix.cpp       0x19e0..0x1ca0  per-core master mix and volume
 *    effect.cpp    0x1ca0..0x2330  per-core SPU2 reverb
 *    xvab.cpp      0x2330..0x2e00  XVAB sound banks and the chunk file format
 *    xseq.cpp      0x2e00..0x35c0  XSEQ sequences (BGM's MIDI data)
 *    voice.cpp     0x35c0..0x45b0  the 48 SPU2 voices: allocation, registers
 *    bgm.cpp       0x45b0..0x6ab0  the BGM MIDI sequencer
 *    (not yet)     0x6ab0..0x9eb0  SE, NOTE: stubs.cpp until reconstructed
 *    adpcm.cpp     0x9eb0..0xb4a0  ADPCM streams (title music, voices, movies)
 *    (not yet)     0xb4a0..        PCM: stubs.cpp until reconstructed
 *
 *  Functions carry `IOP 0x<addr>` (never `ROR 0x`: these are IOP addresses,
 *  and the TU tools count ROR annotations as EE functions).
 *
 *  The double buffer.  Commands arrive on the EE's RPC (any time) and are
 *  written into bank RxSndDrvCmdBank(); the double-buffer thread flips the
 *  bank once per V-blank and then applies RxSndDrvTickBank(), the bank the
 *  commands of the last frame went into.  Every subsystem keeps two pending
 *  records {mask, values} indexed by bank for that.
 *
 *  Host: IOP addresses the EE hands over (heap blocks, SPU addresses) are
 *  u_int words, as on the IOP; IOP heap blocks come from the host IOP arena
 *  (RosePan_IopAllocSysMemory), which lies below 4 GB, so they are host
 *  pointers.  EE addresses are not (see RxSndDrvEePointer).
 * ======================================================================== */

#ifndef ROSEPAN_IOP_RXSNDDRV_H
#define ROSEPAN_IOP_RXSNDDRV_H

#include "scetypes.h"

#include <stddef.h>

/* ---- debug flags (system parameter 0; RxSndDrvGetDebugFlags) ----------- */
#define RXSNDDRV_DBG_ERROR   0x1     /* "snddrv : invalide RPC command" and the other errors */
#define RXSNDDRV_DBG_WARN    0x2     /* unsupported MIDI controllers / meta events, slot shortages */
#define RXSNDDRV_DBG_INFO    0x4     /* start-up / open / close messages */

/* ---- rxsnddrv.cpp ------------------------------------------------------ */

int   RxSndDrvStart(void);                          // IOP 0x00000000 (_start)
int   RxSndDrvStartTick(void);                      // IOP 0x00000190
int   RxSndDrvStopTick(void);                       // IOP 0x00000238
int   RxSndDrvTickBank(void);                       // IOP 0x00000254
int   RxSndDrvCmdBank(void);                        // IOP 0x00000268
int   RxSndDrvGetVersion(void);                     // IOP 0x0000027c
u_char *RxSndDrvZeroBuffer(void);                   // IOP 0x0000028c
void  RxSndDrvTickThread(void);                     // IOP 0x0000029c
u_int RxSndDrvCommand(u_int cmd, u_char *reply, const u_int *data); // IOP 0x00000360

/* ---- rpc.cpp ----------------------------------------------------------- */

int   RxSndDrvRpcStart(void);                       // IOP 0x00000b08
void  RxSndDrvRpcThread(void);                      // IOP 0x00000bc4
void *RxSndDrvRpcServe(u_int fno, void *data, int size); // IOP 0x00000c24

/* ---- dma.cpp ----------------------------------------------------------- */

/* Transfer types (command 0x40 {type, a, b, size}). */
enum
{
    RXSNDDRV_DMA_IOP_TO_SPU = 1,    /* a SPU, b IOP */
    RXSNDDRV_DMA_SPU_TO_IOP = 2,    /* a IOP, b SPU */
    RXSNDDRV_DMA_EE_TO_SPU  = 3,    /* a SPU, b EE: 0x4000 at a time through the staging buffer */
    RXSNDDRV_DMA_SPU_TO_EE  = 4,    /* a EE, b SPU: the same, the other way */
    RXSNDDRV_DMA_ZERO_SPU   = 5,    /* a SPU: zeros from RxSndDrvZeroBuffer, 0x1000 at a time */
};

typedef void (*RxSndDrvDmaDone)(void *arg, int size);

void  RxSndDrvDmaProc(void);                        // IOP 0x00000c60
int   RxSndDrvDmaIntr(int core, void *arg);         // IOP 0x00000ed4
void  RxSndDrvDmaInit(void);                        // IOP 0x00001048
int   RxSndDrvDmaPush(int queue, int type, u_int a, u_int b, int size,
                      RxSndDrvDmaDone done, void *arg); // IOP 0x00001124
void  RxSndDrvDmaKick(void);                        // IOP 0x00001250
int   RxSndDrvDmaBusy(int id);                      // IOP 0x0000127c
void  RxSndDrvDmaCancel(int id);                    // IOP 0x0000132c
void  RxSndDrvDmaCancelArg(void *arg);              // IOP 0x00001414
void  RxSndDrvDmaThread(void);                      // IOP 0x0000151c
void  RxSndDrvDmaInit2(void);                       // IOP 0x00001570
void  RxSndDrvSendToEe(u_int ee, void *iop, int size);   // IOP 0x00001580
void  RxSndDrvGetFromEe(void *iop, u_int ee, int size);  // IOP 0x00001618

/* PC: an EE address the EE sent as a 32-bit word, widened back to the host
 * pointer rx/rxsnd.cpp registered for it (RosePan_RxSndNoteEeAddress), or the
 * word itself when none was registered. */
void *RxSndDrvEePointer(u_int ee);
void  RosePan_RxSndNoteEeAddress(const void *ee, size_t size);

/* ---- sys.cpp ----------------------------------------------------------- */

void  RxSndDrvSysTick(void);                        // IOP 0x00001680
void  RxSndDrvSysInit(void);                        // IOP 0x00001814
int   RxSndDrvGetFrameRate(void);                   // IOP 0x00001880
u_int RxSndDrvGetDebugFlags(void);                  // IOP 0x00001890
int   RxSndDrvGetOutputMode(void);                  // IOP 0x000018a0
void  RxSndDrvSysSet(const u_int *data);            // IOP 0x000018b0

/* ---- mix.cpp ----------------------------------------------------------- */

void  RxSndDrvMixTick(void);                        // IOP 0x000019e0
void  RxSndDrvMixInit(void);                        // IOP 0x00001b5c
void  RxSndDrvMixSet(int core, const u_int *data);  // IOP 0x00001bdc

/* ---- effect.cpp -------------------------------------------------------- */

void  RxSndDrvEffectTick(void);                     // IOP 0x00001ca0
void  RxSndDrvEffectInit(void);                     // IOP 0x000020c4
void  RxSndDrvEffectSet(int core, const u_int *data); // IOP 0x000021e0

/* ---- xvab.cpp ---------------------------------------------------------- */

/* A bank's tables (IOP addresses, 0 none) and its SPU sample base. */
struct RxSndDrvXvabTables
{
    u_int m_tgif;                       // IOP +0x00: programs
    u_int m_seif;                       // IOP +0x04: SE groups
    u_int m_velt;                       // IOP +0x08: velocity curves
    u_int m_pant;                       // IOP +0x0c: pan curves
    u_int m_lfot;                       // IOP +0x10: LFO curves
    u_int m_spu;                        // IOP +0x14
};
static_assert(sizeof(RxSndDrvXvabTables) == 0x18, "RxSndDrvXvabTables");

/* A slot (0x80 at IOP 0x19680). */
struct RxSndDrvXvab
{
    u_int              m_file;          // IOP +0x00: the file in IOP memory, 0 free
    RxSndDrvXvabTables m_tables;        // IOP +0x04
    int                m_users;         // IOP +0x1c: notes / SEs using it
};
static_assert(sizeof(RxSndDrvXvab) == 0x20, "RxSndDrvXvab");
static_assert(offsetof(RxSndDrvXvab, m_tables) == 0x4, "RxSndDrvXvab::m_tables");
static_assert(offsetof(RxSndDrvXvab, m_users) == 0x1c, "RxSndDrvXvab::m_users");

int   RxSndDrvXvabOpen(u_int id, u_int iop, u_int spu);    // IOP 0x00002330
int   RxSndDrvXvabRelocate(u_char *file, int dir, int open); // IOP 0x00002578
void  RxSndDrvXvabInit(void);                       // IOP 0x00002854
void  RxSndDrvXvabCloseAll(void);                   // IOP 0x00002880
int   RxSndDrvXvabClose(u_int id);                  // IOP 0x000028b8
RxSndDrvXvabTables *RxSndDrvXvabGetTables(u_int id); // IOP 0x000029b0
u_int RxSndDrvXvabGetSpu(int id);                   // IOP 0x000029e8
int   RxSndDrvXvabLock(int id);                     // IOP 0x00002a14
void  RxSndDrvXvabUnlock(int id);                   // IOP 0x00002a88
u_int RxSndDrvXvabFindSe(int id, int group, int index, int *ticks, int *a, int *b,
                         int *mode);                // IOP 0x00002ae8
int   RxSndDrvXvabAttach(u_char *file);             // IOP 0x00002c7c
int   RxSndDrvXvabDetach(u_char *file);             // IOP 0x00002ca0
u_int RxSndDrvChunkGetVersion(const void *file);    // IOP 0x00002cc4
u_char *RxSndDrvChunkFind(const void *file, u_int tag);   // IOP 0x00002cd0
int   RxSndDrvXvabGetUsers(int id);                 // IOP 0x00002d14
u_char *RxSndDrvChunkFindV1(const void *file, u_int tag); // IOP 0x00002d40
u_char *RxSndDrvChunkFindV2(const void *file, u_int tag); // IOP 0x00002da0

/* ---- xseq.cpp ---------------------------------------------------------- */

/* A slot (0x20 at IOP 0x1a680). */
struct RxSndDrvXseq
{
    u_int m_file;                       // IOP +0x00: the file in IOP memory, 0 free
    u_int m_head;                       // IOP +0x04: its HEAD (or MIDI) chunk
    int   m_state;                      // IOP +0x08: -1 after a close, read nowhere
    int   m_users;                      // IOP +0x0c: BGMs playing it
    short m_bank[8];                    // IOP +0x10: XVAB per bank index, -1 unset
};
static_assert(sizeof(RxSndDrvXseq) == 0x20, "RxSndDrvXseq");
static_assert(offsetof(RxSndDrvXseq, m_head) == 0x4, "RxSndDrvXseq::m_head");
static_assert(offsetof(RxSndDrvXseq, m_state) == 0x8, "RxSndDrvXseq::m_state");
static_assert(offsetof(RxSndDrvXseq, m_users) == 0xc, "RxSndDrvXseq::m_users");
static_assert(offsetof(RxSndDrvXseq, m_bank) == 0x10, "RxSndDrvXseq::m_bank");

int   RxSndDrvXseqOpen(u_int id, u_int iop);        // IOP 0x00002e00
int   RxSndDrvXseqRelocate(u_char *file, int dir, int open); // IOP 0x00002fa0
void  RxSndDrvXseqInit(void);                       // IOP 0x00003170
void  RxSndDrvXseqCloseAll(void);                   // IOP 0x0000319c
int   RxSndDrvXseqClose(u_int id);                  // IOP 0x000031d4
u_int *RxSndDrvXseqGetHead(u_int id);               // IOP 0x000032d8
void  RxSndDrvXseqSetBank(u_int id, u_int index, u_int xvab); // IOP 0x00003310
int   RxSndDrvXseqGetBank(int id, u_int index);     // IOP 0x00003348
int   RxSndDrvXseqLock(int id);                     // IOP 0x0000338c
void  RxSndDrvXseqUnlock(int id);                   // IOP 0x00003400
int   RxSndDrvXseqAttach(u_char *file);             // IOP 0x00003460
int   RxSndDrvXseqDetach(u_char *file);             // IOP 0x00003484
u_char *RxSndDrvXseqChunkFind(const void *file, u_int tag);   // IOP 0x000034a8
u_char *RxSndDrvXseqChunkFindV1(const void *file, u_int tag); // IOP 0x00003518
u_char *RxSndDrvXseqChunkFindV2(const void *file, u_int tag); // IOP 0x00003568

/* ---- voice.cpp --------------------------------------------------------- */

/* Told when an owner loses its voice: reason 0 it ended or was released,
 * 1 another allocation took it. */
typedef void (*RxSndDrvVoiceDone)(int voice, int reason, void *arg);

void  RxSndDrvVoiceTick(void);                      // IOP 0x000035c0
int   RxSndDrvVoiceAlloc(int voice, int priority, int autoFree, int group, int owner,
                         RxSndDrvVoiceDone done, void *arg); // IOP 0x00003b80
void  RxSndDrvVoiceInit(void);                      // IOP 0x00003eb8
void  RxSndDrvVoiceSetOutputMode(int mode);         // IOP 0x00003f6c
void  RxSndDrvVoiceRelease(int voice, int reason, int noFade); // IOP 0x00003fcc
void  RxSndDrvVoiceSetSample(int voice, u_int ssa, u_short adsr1, u_short adsr2); // IOP 0x0000409c
void  RxSndDrvVoiceSetLoop(int voice, u_int lsax);  // IOP 0x000040d0
void  RxSndDrvVoiceSetSwitch(int voice, u_char bits); // IOP 0x000040fc
void  RxSndDrvVoiceSetVolume(int voice, int l, int r);    // IOP 0x00004128
void  RxSndDrvVoiceSetVolumeRaw(int voice, int l, int r); // IOP 0x00004160
void  RxSndDrvVoiceSetPitch(int voice, u_short center, u_short centerFine, u_short note,
                            short fine, int scale); // IOP 0x00004190
void  RxSndDrvVoiceKeyOn(int voice);                // IOP 0x00004208
void  RxSndDrvVoiceKeyOff(int voice);               // IOP 0x0000429c
u_short RxSndDrvVoiceGetEnvx(int voice);            // IOP 0x00004340
void  RxSndDrvVoiceGetEnvxAll(u_char *reply);       // IOP 0x00004384
void  RxSndDrvVoiceGetTable(u_int kind, u_char *reply); // IOP 0x000043b4
u_int RxSndDrvVoiceGetNax(int voice);               // IOP 0x00004468
void  RxSndDrvVoiceSetEnable(u_int voice, u_int enable); // IOP 0x0000451c
void  RxSndDrvVoiceGetFlags(u_char *reply);         // IOP 0x00004540
void  RxSndDrvVoiceFree(int voice);                 // IOP 0x00004570

/* ---- bgm.cpp ----------------------------------------------------------- */

void  RxSndDrvBgmInit(void);                        // IOP 0x000045b0
int   RxSndDrvBgmOpen(u_int slot, u_int xseq);      // IOP 0x00004680
void  RxSndDrvBgmSet(u_int slot, const u_int *data);    // IOP 0x000059ec
int   RxSndDrvBgmStatus(u_char *reply, u_char *values); // IOP 0x00005b68
void  RxSndDrvBgmChannelSet(int slot, int channel, const u_int *data); // IOP 0x00005c10
void  RxSndDrvBgmTrackSet(int slot, u_int track, const u_int *data);   // IOP 0x00005d20
void  RxSndDrvBgmTick(void);                        // IOP 0x00005e60

/* ---- adpcm.cpp --------------------------------------------------------- */

int   RxSndDrvAdpcmOpen(int id, int pitch, const u_int *chans, int count, u_int iopSize,
                        int spuSize, int priority); // IOP 0x00009eb0
void  RxSndDrvAdpcmInit(void);                      // IOP 0x0000acac
void  RxSndDrvAdpcmCloseAll(void);                  // IOP 0x0000ad54
int   RxSndDrvAdpcmData(int id, int size, int end); // IOP 0x0000ad94
void  RxSndDrvAdpcmSet(u_int id, const u_int *data);    // IOP 0x0000aea0
void  RxSndDrvAdpcmChannelSet(int id, u_int no, const u_int *data); // IOP 0x0000b01c
int   RxSndDrvAdpcmStatus(u_char *reply);           // IOP 0x0000b160
void  RxSndDrvAdpcmTick(void);                      // IOP 0x0000b1dc

/* ---- not reconstructed yet (stubs.cpp) --------------------------------- */

void  RxSndDrvSeInit(void);                         // IOP 0x00006ab0
void  RxSndDrvSeSet(u_int voice, const u_int *data);    // IOP 0x00006bcc
int   RxSndDrvSePlay(int voice, int xvab, int prog, int note);  // IOP 0x000084b0
int   RxSndDrvSePlay2(int voice, int xvab, int prog, int note); // IOP 0x00008658
int   RxSndDrvSeStatus(u_char *reply);              // IOP 0x00008800
void  RxSndDrvSeTick(void);                         // IOP 0x00008878
void  RxSndDrvNoteTick(void);                       // IOP 0x000093c0
void  RxSndDrvNoteInit(void);                       // IOP 0x000098e4
/* The NOTE layer as the BGM sequencer calls it: a NOTE id from Play (-1
 * none), then key off, free and its parameters; GetFlags bit 1 = sounding. */
int   RxSndDrvNotePlay(int xvab, int program, int key, int a, int b); // IOP 0x00008f10
void  RxSndDrvNoteKeyOff(int note);                 // IOP 0x00009974
void  RxSndDrvNoteFree(int note);                   // IOP 0x000099c8
void  RxSndDrvNoteSetSwitch(int note, int on);      // IOP 0x00009ab0
void  RxSndDrvNoteSetVolume(int note, short l, short r); // IOP 0x00009ae4
void  RxSndDrvNoteSetVelocity(int note, int velocity);   // IOP 0x00009b1c
void  RxSndDrvNoteSetPan(int note, int pan);        // IOP 0x00009bb0
void  RxSndDrvNoteSetPitch(int note, int scale);    // IOP 0x00009c0c
void  RxSndDrvNoteSetBend(int note, int bend);      // IOP 0x00009c40
u_int RxSndDrvNoteGetFlags(int note);               // IOP 0x00009db4
void  RxSndDrvPcmInit(void);                        // IOP 0x0000b4a0
void  RxSndDrvPcmTick(void);                        // IOP 0x0000b5ac
int   RxSndDrvPcmOpen(int id, int a, u_int b, u_int c);  // IOP 0x0000c944
void  RxSndDrvPcmData(int id, const u_int *data, int a); // IOP 0x0000cabc
void  RxSndDrvPcmSet(u_int id, const u_int *data);  // IOP 0x0000cc28
int   RxSndDrvPcmStatus(u_char *reply);             // IOP 0x0000cd58

#endif /* ROSEPAN_IOP_RXSNDDRV_H */
