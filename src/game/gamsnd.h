/* ==========================================================================
 *  game/gamsnd.h
 *
 *  GamSndMan (game/gamsnd.cpp; vtable 0x2e4840, vptr at +0x1874,
 *  constructor 0x1e0df0): the game's sound table, GamMan's member at
 *  +0x18e00 (bss 0x79a0c0). SndMan looks sound ids up in it; the
 *  scripts drive it through native 252.
 * ======================================================================== */
#ifndef _GAME_GAMSND_H
#define _GAME_GAMSND_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "sdk/libvu0.h"
#include "sequence/sequence.h"
#include "snd/sndman.h"

/* One sound of the table (EE 0x38 bytes, an array at *(GamSndMan +
 * 0x1540), ended by id 0 or 0x400 entries; the stream table +0x1544 has
 * 900, -1 marks a free one). The name is what SndMan::loadBankById hands
 * rxSndXvabLoad. */
struct GamSndEntry
{
    char     m_name[0x1c];      // ROR 0x00
    int      m_id;              // ROR 0x1c
    u_short  m_bank;            // ROR 0x20: SndMan bank number
    u_short  m_prog;            // ROR 0x22
    u_short  m_note;            // ROR 0x24
    u_short  m_vol;             // ROR 0x26: playSE's panned volume
    u_short  m_pan;             // ROR 0x28: 0..256, the left share (/ 256)
    u_short  m_far;             // ROR 0x2a: gamSnd3dVolume: m_farVol from this distance on
    u_short  m_near;            // ROR 0x2c: m_nearVol up to this distance
    u_short  m_nearVol;         // ROR 0x2e
    u_short  m_farVol;          // ROR 0x30
    u_short  m_32;              // ROR 0x32: isSndBankLoaded's second argument
    u_short  _34;
    u_short  m_resume;          // ROR 0x36: non-zero: pushSnd keeps the SE for resumeBgm
};
static_assert(sizeof(GamSndEntry) == 0x38, "GamSndEntry");
static_assert(offsetof(GamSndEntry, m_id) == 0x1c, "m_id");
static_assert(offsetof(GamSndEntry, m_bank) == 0x20, "m_bank");
static_assert(offsetof(GamSndEntry, m_vol) == 0x26, "m_vol");
static_assert(offsetof(GamSndEntry, m_far) == 0x2a, "m_far");
static_assert(offsetof(GamSndEntry, m_farVol) == 0x30, "m_farVol");
static_assert(offsetof(GamSndEntry, m_32) == 0x32, "m_32");
static_assert(offsetof(GamSndEntry, m_resume) == 0x36, "m_resume");

/* A playing sound GamSndMan keeps (EE 0x28; 0x30 of them, indexed by
 * the SndMan slot id). */
struct GamSndRec
{
    u_int m_flags;              // ROR 0x00: GAMSND_REC_*
    int   m_volL;               // ROR 0x04
    int   m_volR;               // ROR 0x08
    u_int m_se;                 // ROR 0x0c: the SE id, 0 none
    int   _10;
    int   m_str;                // ROR 0x14: the stream number, -1 none
    int   m_slot;               // ROR 0x18: SndMan's id, -1 none
    int   _1c[3];
};
static_assert(sizeof(GamSndRec) == 0x28, "GamSndRec");
static_assert(offsetof(GamSndRec, m_se) == 0x0c, "m_se");
static_assert(offsetof(GamSndRec, m_slot) == 0x18, "m_slot");

/* GamSndRec::m_flags (playSE stores its caller's flags, playStr its
 * caller's | 1). */
enum
{
    GAMSND_REC_STR   = 0x001,   /* a stream; else an SE */
    GAMSND_REC_8     = 0x008,   /* playStr's bool */
    GAMSND_REC_LOOP  = 0x010,   /* funcSE 4 set the loop */
    GAMSND_REC_KEEP  = 0x080,   /* funcSE 1 / 8 and pushSnd leave it */
    GAMSND_REC_BGM   = 0x100,   /* a stream pushSnd saves (playStr flag 0x100) */
};

/* One push level of the BGM state (EE 0x24, at GamSndMan +0x15a0; GamMan
 * keeps one at +0x22a5c that setBgmRec copies to level 0). */
struct GamBgmRec
{
    u_int m_se;                 // ROR 0x00: a BGM-class SE (id >> 24 == 0xb), 0 none
    int   m_seVolL;             // ROR 0x04
    int   m_seVolR;             // ROR 0x08
    int   m_str;                // ROR 0x0c: a GAMSND_REC_BGM stream, -1 none
    int   m_strVolL;            // ROR 0x10
    int   m_strVolR;            // ROR 0x14
    int   m_str2;               // ROR 0x18: a second stream, -1 none (only resumeBgm reads it)
    int   m_str2VolL;           // ROR 0x1c
    int   m_str2VolR;           // ROR 0x20
};
static_assert(sizeof(GamBgmRec) == 0x24, "GamBgmRec");
static_assert(offsetof(GamBgmRec, m_str) == 0x0c, "m_str");
static_assert(offsetof(GamBgmRec, m_str2) == 0x18, "m_str2");

/* A second BGM-class SE of a push level (EE 0xc, at GamSndMan +0x1654). */
struct GamSndPushSe
{
    u_int m_se;                 // ROR 0x00
    int   m_volL;               // ROR 0x04
    int   m_volR;               // ROR 0x08
};
static_assert(sizeof(GamSndPushSe) == 0xc, "GamSndPushSe");

/* A looping SE pushSnd saved (EE 0x10, at GamSndMan +0x1688). */
struct GamSndLoopSe
{
    u_int m_se;                 // ROR 0x00
    int   m_volL;               // ROR 0x04
    int   m_volR;               // ROR 0x08
    int   _0c;
};
static_assert(sizeof(GamSndLoopSe) == 0x10, "GamSndLoopSe");

/* GamSnd3d::m_flags (the put_* callers' flags, the low ones set by the
 * put_* functions themselves). */
enum
{
    GAMSND_3D_STR    = 0x001,   /* a stream; else an SE */
    GAMSND_3D_WRD    = 0x002,   /* a character's sound heard only in its room; stopObjSnds drops it */
    GAMSND_3D_CHR    = 0x004,   /* follows character m_oid (put_*_3d_pos_chr) */
    GAMSND_3D_START  = 0x020,   /* put_* starts it at once (unless it already plays) */
    GAMSND_3D_PLAYED = 0x040,   /* update3dVolume (re)started it */
    GAMSND_3D_ONCE   = 0x200,   /* started by put_*: dropped when it ends */
};

/* One 3D sound (EE 0x50: the put_* functions build it on the stack and the
 * list copies it into a free node). layout: host -- m_entry is a pointer;
 * the offsets up to it are the EE's. */
struct GamSnd3d
{
    /* m_dist = dist; m_pos = pos - origin (x, y, z), or pos (4 floats)
     * when there is no origin. */
    void setPos(float dist, const float *pos, const float *origin);      // ROR 0x001e2350

    sceVu0FVECTOR  m_pos;       // ROR 0x00: where it is heard from (room coordinates); update3d
    sceVu0FVECTOR  m_src;       // ROR 0x10: put_*_3d_pos: pos + the room's WRD offset; _chr: the offset
    u_int          m_snd;       // ROR 0x20: the SE / stream
    int            m_rec;       // ROR 0x24: its GamSndRec (SndMan id), -1 none
    u_int          m_flags;     // ROR 0x28: GAMSND_3D_*
    u_int          m_oid;       // ROR 0x2c: the character (GamMan::getObjPos), 0 a fixed position
    int            m_wrd;       // ROR 0x30: the room (WRD) it was put in, -1 none
    const GamSndEntry *m_entry; // ROR 0x34: its table entry, NULL: GamSndMan::m_defEntry
    float          m_dist;      // ROR 0x38: update3d's sort key (3000 until the first update)
    int            m_volL;      // ROR 0x3c: the volume update3dVolume set
    int            m_volR;      // ROR 0x40
    int            _44[3];      /* never written (the ROM copies stack garbage) */
};
static_assert(offsetof(GamSnd3d, m_src) == 0x10, "m_src");
static_assert(offsetof(GamSnd3d, m_snd) == 0x20, "m_snd");
static_assert(offsetof(GamSnd3d, m_rec) == 0x24, "m_rec");
static_assert(offsetof(GamSnd3d, m_flags) == 0x28, "m_flags");
static_assert(offsetof(GamSnd3d, m_oid) == 0x2c, "m_oid");
static_assert(offsetof(GamSnd3d, m_wrd) == 0x30, "m_wrd");

/* The 3D sound lists are RoR's node-pool template (sequence/sequence.h):
 * a node is EE 0x60 (prev, next, the sound at +0x10), a list 0x70 (the
 * sentinel, the count at +0x60). layout: host. */
typedef SeqNode<GamSnd3d> GamSnd3dNode;
typedef SeqList<GamSnd3d> GamSnd3dList;

#define GAMSND_NODE_MAX     0x20
#define GAMSND_REC_MAX      0x30
#define GAMSND_SE_MAX       0x400   /* m_table's entries (0xe000 bytes) */
#define GAMSND_STR_MAX      900     /* m_strTable's entries (0xc4e0 bytes) */

/* Listener volume of a sound at pos for sound entry: mode 0 hears from
 * the PC, 1 from the camera, else 0x40 each. scale (100.0) is not read. */
void gamSnd3dVolume(int mode, const float *pos, const GamSndEntry *entry, int *volL,
                    int *volR, float scale);                             // ROR 0x001e6e00
int  gamSndReturnZero();                                                 // ROR 0x001e4cbc

/* gamSndEarGain's argument block (EE 0x10, on gamSnd3dVolume's stack).
 * layout: host -- pointers. */
struct GamSndEar
{
    const float *m_eye;         // ROR 0x00: the listener
    const float *m_rot;         // ROR 0x04: its rotation
    float        m_side;        // ROR 0x08: the ear's angle off the facing (+-pi/3)
    const float *m_pos;         // ROR 0x0c: the sound
};
/* 0.3 (the sound behind the ear) .. 1.0 (ahead). */
float gamSndEarGain(const GamSndEar *ear);                               // ROR 0x001e6d54

/* The game's sound state (EE 0x1878: the vptr at +0x1874, one virtual,
 * the destructor). GamMan's same-named inline methods (registSnd,
 * funcSnd, clear_snd_lst, loadSePack, the ELF's symbols) forward to
 * these. The parameter blocks funcSE / funcStr take are TskSndParam words
 * (lib/unassigned.h): 2 / 9 / 0x21 volL, volR; 3 / 10 the pitch as a
 * float; 5 / 0xb volL, volR, pitch, -, attr; 0x13..0x15 / 0x1b a fade the
 * TskSnd runs (volL, volR, frames). layout: host -- the vptr, the list
 * links and the table pointers; EE offsets in the comments. */
struct GamSndMan
{
    /* The 3D lists linked (all nodes free), the records at m_recBuf. */
    GamSndMan();                                                         // ROR 0x001e0df0
    virtual ~GamSndMan();                                                // EE slot 1, ROR 0x001e0eb8

    /* resetVolume, reset, the two tables allocated, the BGM words reset. */
    void init();                                                         // ROR 0x001e0f1c
    /* The volume words back to 0x80 and funcSnd 0xd / 0xe / 0xf at 0x80
     * (MnuTitle's first frame). */
    void resetVolume();                                                  // ROR 0x001e107c
    /* Every frame (GamMan::update; pos unused: mode 1 takes the camera's). */
    void update(const float *pos);                                       // ROR 0x001e100c
    /* The used 3D sounds placed relative to the listener at pos; those
     * beyond 15000 on x or z stopped, the 3 nearest SEs or 4 nearest
     * streams (by m_dist) get update3dVolume. Not while m_flags bit 0
     * (stopBgm) or GamMan::m_flags2 bit 31 is set. */
    void update3d(const float *pos);                                     // ROR 0x001e3c24
    /* snd's volume from the listener (eased against its record's, scaled
     * by m_3dScale outside its room) into its record; an ended sound is
     * restarted, or dropped when it was a one-shot. */
    void update3dVolume(GamSnd3d *snd);                                  // ROR 0x001e386c
    /* The SE table entry named name (to id 0 or 0x400 entries). */
    int  findSndByName(const char *name, GamSndEntry **entry);           // ROR 0x001e4970
    /* Overwrites stream str's entry of the hashed table with entry (its
     * m_id set to str); 0 when str has none. */
    int  setStrSnd(int str, GamSndEntry *entry);                         // ROR 0x001e4b48
    /* playSE of the SE named name (unflagged); -1 when there is none. */
    int  playSEByName(const char *name, bool pan, float *pos);           // ROR 0x001e4db0
    /* playSE(se, volL, volR, pitch, attr) of the SE named name, now and
     * unflagged; -1 when there is none. */
    int  playSEByName(const char *name, int volL, int volR, float pitch, int attr); // ROR 0x001e4e20
    /* stopSeTrack's twin: stops stream str in SndMan id track while it
     * plays (its record cleared): 1, else 0. */
    int  stopStrTrack(int str, int track);                               // ROR 0x001e5d1c
    /* The ELF's inlines. */
    /* The push level m_pushDepth, m_bgmCur when nothing is pushed. */
    GamBgmRec *get_snd_prm();                                            // ROR 0x0028fdf8
    void setSnd3DCnt(int type);                                          // ROR 0x0028fe20
    void getMstVolSE(int &volL, int &volR);                              // ROR 0x0028fe28
    void getMstVolStr(int &volL, int &volR);                             // ROR 0x0028fe3c
    void getMstVol(int &volL, int &volR);                                // ROR 0x0028fe50
    void rmSnd(u_int id);                                                // ROR 0x0028fe64
    /* m_fileName[state] (OID_STATE). */
    const char *getSndFileName(int state) const;                         // ROR 0x0028fe6c
    GamSndEntry *get_snd_tbl_addr();                                     // ROR 0x0028fe7c
    /* Saves the playing BGM-class SEs (mode != 2), the GAMSND_REC_BGM
     * stream (mode != 1) and the looping SEs into a new push level --
     * only when none is pushed (m_pushDepth < 0). resumeBgm pops it. */
    void pushSnd(int mode);                                              // ROR 0x001e111c
    /* bank < 0x41 and, when check, its SE pack loaded. */
    bool isBankSet(int bank, int arg, int check);                        // ROR 0x001e43a4
    /* "BBII.se": the bank BB and the pack id II. */
    void parseSePackName(const char *name, int *bank, int *id);          // ROR 0x001e43e0
    /* SEs stopped, the XVAB data into bank, its pack id kept. */
    void loadSePackData(int bank, int id, void *data, int nowait);       // ROR 0x001e44b4
    /* Frees bank slot n (SndMan::freeBank) and marks it -1. */
    void freeSePack(int n);                                              // ROR 0x001e46b8
    /* The pack (name "BBII.se") from data into bank BB; nowait as
     * SndMan::loadBank. */
    void loadSePack(const char *name, void *data, int nowait);           // ROR 0x001e4458
    /* The 3D SE / stream entries of character oid dropped (GamMan::
     * releaseChr; GamMan's del_se_3d_pos_chr / del_str_3d_pos_chr forward
     * here). */
    void del_se_3d_pos_chr(u_int oid);                                   // ROR 0x001e2c40
    void del_str_3d_pos_chr(u_int oid);                                  // ROR 0x001e3698
    void registSnd(int state, void *data);                               // ROR 0x001e4790
    void registStrSnd(int state, void *data);                            // ROR 0x001e484c
    void clear_snd_lst(int arg);                                         // ROR 0x001e6570
    /* Sound function no (SND_FUNC_NO: 0xe / 0xf the SE / stream master
     * volumes, 0x11 at loadSystemData). */
    void funcSnd(int no, int a, int b, int c, int d);                    // ROR 0x001e6734
    /* 1 and *entry when id is in the table, else 0 and NULL. */
    int findSnd(int id, GamSndEntry **entry, int arg);                   // ROR 0x001e4908
    /* Non-zero when id's bank is loaded (its handle at +0x1728 is set). */
    int isSndBankLoaded(int id);                                         // ROR 0x001e435c
    /* GamMan::getSeID / getStrSnd / stopSE forward here. */
    int  getSeID(const char *name);                                      // ROR 0x001e4a0c
    int  getStrSnd(int str, GamSndEntry **entry);                        // ROR 0x001e4c1c
    /* Mode 1: every SE but the BGM-class ones stopped (stopSeTrack). */
    void stopSE(int mode);                                               // ROR 0x001e4d18
    /* Stops record track's SE when it is se and still plays: 1, else 0. */
    int  stopSeTrack(u_int se, int track);                               // ROR 0x001e649c
    /* Every used 3D sound still playing fades out untracked (SEs over 60
     * frames, streams over 30, the stream kept as the push level's
     * m_str2). */
    void fadeOut3dSnds();                                                // ROR 0x001e4120
    /* pushSnd(0), then what it saved fades out over 30 frames: the stream
     * (stopped at the end) unless mode 1, the BGM-class SEs unless mode 2,
     * the looping SEs; the 3D sounds fade too and m_flags bit 0 is set. */
    void stopBgm(int mode);                                              // ROR 0x001e17b4
    /* Pops pushSnd's level (m_pushDepth; reset pops from level 0): the
     * stream (mode != 1; restarted when replay or no longer playing, else
     * resumed) and the BGM-class SEs (mode != 2) fade back to their
     * volumes from 8 over 30 frames, the looping SEs restart. */
    void resumeBgm(int replay, int mode, int reset);                     // ROR 0x001e1920
    /* rec into +0x15a0, +0x1684 cleared (GamMan::onSubSeqEnd, before
     * resumeBgm(1, 0, 0)). */
    void setBgmRec(const GamBgmRec *rec);                                // ROR 0x001e15d0

    /* The script's sound API (native 252). Names and parameter types are
     * GamMan's inline forwarders' (ELF symbols 0x28e820..0x28ee48); the
     * trailing int some take is the 0 the forwarder adds. */
    /* Clears the current BGM slot's pushed SE (mode 1) / stream id (2, -1
     * any) words. */
    void unpush_snd(int mode, u_int se, int str);                        // ROR 0x001e1620
    /* A 3D SE at pos in room wrd (-1 GamMan's), sorted into the used list
     * by its distance from the room origin; flags GAMSND_3D_START starts it
     * now unless it already plays. Nothing without a WRD entry. */
    void put_se_3d_pos(u_int se, float *pos, int wrd, u_int flags);      // ROR 0x001e239c
    /* A 3D SE following character oid (once per oid and se). */
    void put_se_3d_pos_chr(u_int se, u_int oid, u_int flags);            // ROR 0x001e2704
    /* The 3D entries of se (in room wrd, -1 any) stopped and freed. */
    void del_se_3d_pos(u_int se, int wrd);                               // ROR 0x001e2aa0
    void put_str_3d_pos(int str, float *pos, int wrd, u_int flags);      // ROR 0x001e2db4
    void put_str_3d_pos_chr(int str, u_int oid, u_int flags);            // ROR 0x001e3138
    void del_str_3d_pos(int str, int wrd);                               // ROR 0x001e34ec
    /* Reads data/snd/xv/<name> (an "BBII" name, ".se" put on) and loads
     * it into bank BB. */
    void loadSePack(const char *name, int nowait);                       // ROR 0x001e4540
    int  getStrNO(const char *name);                                     // ROR 0x001e4ab8
    /* The SPU effect (rxSndSetEffectMode / Depth), kept in m_reverb*. */
    void setReverb(int mode, int depthL, int depthR);                    // ROR 0x001e4cc4
    int  get_se_oid(const char *name);                                   // ROR 0x001e4d04
    /* Plays se: at pos (3D volume), else panned by its entry (pan) or at
     * 100 / 100; flags into its record. SndMan's id, -1 when it cannot. */
    int  playSE(u_int se, bool pan, float *pos, int flags);              // ROR 0x001e4ebc
    /* Plays se at volL / volR, pitch, attr; flags into its record. With
     * delay, a TskSnd (kind 5) plays it that many frames later and -1 is
     * returned. */
    int  playSE(u_int se, int volL, int volR, float pitch, int attr, int delay,
                int flags);                                              // ROR 0x001e50c8
    /* Streams s<str>.str (ADPCM mode 1 above 99999) into SndMan id id (-1
     * any): at pos (3D volume, not looping), else at 0x80; flags (| 8 with
     * b) | 1 into its record. The id, -1 when no stream entry for pos. */
    int  playStr(int str, int id, float *pos, bool b, u_int flags);      // ROR 0x001e5250
    /* playStr into SndMan id id (no position), then volL / volR, pitch and
     * loop set; flags | 1 into its record. */
    int  playStr(int str, int volL, int volR, float pitch, int loop, int id,
                 int flags);                                             // ROR 0x001e5428
    /* Sets the 3D distance words (m_far, m_near, m_nearVol, m_farVol) of
     * str's entry; without one, setStrSnd of m_defEntry with them. */
    void set_str_3d_snd_prm(int str, int farDist, int nearDist, int nearVol, int farVol); // ROR 0x001e5540
    /* Function no (SND_FUNC_NO) on the playing streams str: 1 / 8 stop,
     * 2 / 3 / 5 / 0xb set, 4 loop, 0x13 a TskSnd over the parameter block
     * arg (it writes words 3, 8 and 9 back), 0x16 non-zero while one plays. */
    int  funcStr(int str, int no, void *arg);                            // ROR 0x001e5660
    /* The stream number playing in SndMan id chan, -1 none. */
    int  getStrNoByChan(int chan);                                       // ROR 0x001e5be0
    /* The same words of se's entry (nothing without one). */
    void set_se_3d_snd_prm(u_int se, int farDist, int nearDist, int nearVol, int farVol); // ROR 0x001e5de4
    /* The same over the 0x30 SE records (+0xd40, 0x28 bytes each). */
    int  funcSE(u_int se, int no, void *arg);                            // ROR 0x001e5e58
    int  isPlayEnvStr();                                                 // ROR 0x001e6cac
    /* SndMan::stopAll(8), then the SE packs 0, 10..12, 0x1f..0x21 and
     * 0x40 freed. */
    void free_all_se_bank();                                             // ROR 0x001e4700
    bool getStatSEtrack(u_int se, int track);                            // ROR 0x001e6368
    /* The volume of SE se in SndMan id track (record and SPU) while it
     * plays: 1, else 0. */
    int  setVolSEtrack(u_int se, int track, int volL, int volR);         // ROR 0x001e63c0
    bool getStatSTRtrack(u_int str, int track);                          // ROR 0x001e5b60
    int  setVolSTRtrack(u_int str, int track, int volL, int volR);       // ROR 0x001e5c40
    /* A stage reset: the BGM / stream records, the 0x30 sound records,
     * the three sound tasks killed, the 3D sound list emptied. */
    void reset();                                                        // ROR 0x001e1fa4
    /* The 3D sounds of objects dropped (GamMan::releaseObjs): the
     * GAMSND_3D_WRD ones (a stream stopped through funcStr 1) and the
     * GAMSND_3D_CHR ones (stopSeTrack). */
    void stopObjSnds();                                                  // ROR 0x001e2178

    int            m_mode;      // ROR 0x0000: the listener (the ELF's SND_3D_CENTER_TYPE, setSnd3DCnt), gamSnd3dVolume's mode (loadStage sets 1: the camera)
    int            _0004[15];
    GamSnd3dList   m_free;      // ROR 0x0040
    GamSnd3dList   m_used;      // ROR 0x00b0: update runs update3d while it has nodes
    int            m_nodeMax;   // ROR 0x0120: 0x20
    int            _0124[7];
    GamSnd3dNode   m_node[GAMSND_NODE_MAX]; // ROR 0x0140
    GamSndRec     *m_rec;       // ROR 0x0d40: m_recBuf
    GamSndRec     *m_recEnd;    // ROR 0x0d44: m_recBuf too (the destructor's empty loop)
    int            m_recMax;    // ROR 0x0d48: 0x30
    int            _0d4c[13];
    GamSndRec      m_recBuf[GAMSND_REC_MAX]; // ROR 0x0d80
    char           m_fileName[4][0x10]; // ROR 0x1500: GamMan::getSndFileName
    GamSndEntry   *m_table;     // ROR 0x1540: the SE table (GamMan::getSndParam)
    GamSndEntry   *m_strTable;  // ROR 0x1544: the stream table
    int            _1548[2];
    GamSndEntry    m_defEntry;  // ROR 0x1550: the 3D distances of a sound without an entry (init: vol, pan 0x80, far 0x6a4, near 100, nearVol 0x80, farVol 0)
    int            m_mstVol[2];    // ROR 0x1588: GamMan::getMstVol; funcSnd 0xd
    int            m_mstVolSe[2];  // ROR 0x1590: option 3's funcSnd(0xe) words
    int            m_mstVolStr[2]; // ROR 0x1598: GamMan::getMstVolStr, option 2's funcSnd(0xf) words
    GamBgmRec      m_bgm[4];       // ROR 0x15a0: the push levels
    GamBgmRec      m_bgmCur;       // ROR 0x1630: get_snd_prm's record while nothing is pushed (init: no SE, no stream)
    GamSndPushSe   m_pushSe[4];    // ROR 0x1654
    int            m_pushDepth;    // ROR 0x1684: -1 nothing pushed
    GamSndLoopSe   m_loopSe[8];    // ROR 0x1688
    int            m_loopSeCount;  // ROR 0x1708
    int            m_mstTaskId;    // ROR 0x170c: funcSnd 0xd's fade TskSnd
    int            m_seTaskId;     // ROR 0x1710: funcSnd 0xe's / funcSE 0x13's
    int            m_seTaskId2;    // ROR 0x1714: funcSE 0x14's
    int            m_strTaskId;    // ROR 0x1718: funcSnd 0xf's / funcStr 0x13's
    int            m_reverbMode;   // ROR 0x171c: setReverb; 4 (resetVolume)
    int            m_reverbDepthL; // ROR 0x1720: 0x80
    int            m_reverbDepthR; // ROR 0x1724: 0x80
    int            m_bankId[SND_BANK_MAX]; // ROR 0x1728: the SE pack id loaded in each bank, -1 none
    char           m_sePackName[0x40]; // ROR 0x182c: loadSePack(name, nowait)'s "BBII.se"
    float          m_3dScale;      // ROR 0x186c: funcSnd 0x1c (percent / 100), 0.5 at init
    u_int          m_flags;        // ROR 0x1870: bit 0 cleared by resumeBgm, 0x400 between funcSnd 0x1f and 0x20
};

#endif /* _GAME_GAMSND_H */
