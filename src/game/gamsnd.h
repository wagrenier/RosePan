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

/* One sound of the table (EE 0x38 bytes, an array at *(GamSndMan +
 * 0x1540), ended by id 0 or 0x400 entries). The name is what
 * SndMan::loadBankById hands rxSndXvabLoad. */
struct GamSndEntry
{
    char     m_name[0x1c];      // ROR 0x00
    int      m_id;              // ROR 0x1c
    u_short  m_bank;            // ROR 0x20: SndMan bank number
    u_short  m_prog;            // ROR 0x22
    u_short  m_note;            // ROR 0x24
    u_short  _26[6];
    u_short  m_32;              // ROR 0x32: isSndBankLoaded's second argument
    int      _34;
};
static_assert(sizeof(GamSndEntry) == 0x38, "GamSndEntry");
static_assert(offsetof(GamSndEntry, m_bank) == 0x20, "m_bank");
static_assert(offsetof(GamSndEntry, m_32) == 0x32, "m_32");

/* A BGM state record GamMan keeps at +0x22a5c (EE 0x24, the words
 * GamSndMan::setBgmRec copies to its +0x15a0). */
struct GamBgmRec
{
    int m_word[9];              // ROR 0x00
};
static_assert(sizeof(GamBgmRec) == 0x24, "GamBgmRec");

/* layout: host -- only the words GamMan reads. GamMan's same-named inline
 * methods (registSnd, funcSnd, clear_snd_lst, loadSePack, the ELF's
 * symbols) forward to these. */
struct GamSndMan
{
    void init();                                                         // ROR 0x001e0f1c
    /* The volume words back to 0x80 and funcSnd 0xd / 0xe / 0xf at 0x80
     * (MnuTitle's first frame). */
    void resetVolume();                                                  // ROR 0x001e107c
    /* Every frame (GamMan::update; pos unused: mode 1 takes the camera's). */
    void update(const float *pos);                                       // ROR 0x001e100c
    /* Frees bank slot n (SndMan::freeBank) and marks it -1. */
    void freeSePack(int n);                                              // ROR 0x001e46b8
    void loadSePack(const char *name, void *data, int arg);              // ROR 0x001e4458
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
    /* Fades the current BGM slot out over 0x1e frames (mode 1 keeps it,
     * 2 works on the other slot +0x1708). */
    void stopBgm(int mode);                                              // ROR 0x001e17b4
    /* GamMan::resume's call (0, 0, 1): restarts the BGM slot (+0x1684,
     * reset to 0 when reset is set). */
    void resumeBgm(int a, int mode, int reset);                          // ROR 0x001e1920
    /* rec into +0x15a0, +0x1684 cleared (GamMan::onSubSeqEnd, before
     * resumeBgm(1, 0, 0)). */
    void setBgmRec(const GamBgmRec *rec);                                // ROR 0x001e15d0

    /* The script's sound API (native 252). Names and parameter types are
     * GamMan's inline forwarders' (ELF symbols 0x28e820..0x28ee48); the
     * trailing int some take is the 0 the forwarder adds. */
    /* Clears the current BGM slot's pushed SE (mode 1) / stream id (2, -1
     * any) words. */
    void unpush_snd(int mode, u_int se, int str);                        // ROR 0x001e1620
    void put_se_3d_pos(u_int se, float *pos, int wrd, u_int flags);      // ROR 0x001e239c
    void put_se_3d_pos_chr(u_int se, u_int oid, u_int flags);            // ROR 0x001e2704
    void del_se_3d_pos(u_int se, int arg);                               // ROR 0x001e2aa0
    void put_str_3d_pos(int str, float *pos, int wrd, u_int flags);      // ROR 0x001e2db4
    void put_str_3d_pos_chr(int str, u_int oid, u_int flags);            // ROR 0x001e3138
    void del_str_3d_pos(int str, int arg);                               // ROR 0x001e34ec
    void loadSePack(const char *name, int arg);                          // ROR 0x001e4540
    int  getStrNO(const char *name);                                     // ROR 0x001e4ab8
    void setReverb(int a, int b, int c);                                 // ROR 0x001e4cc4
    int  get_se_oid(const char *name);                                   // ROR 0x001e4d04
    int  playSE(u_int se, bool loop, float *pos, int arg);               // ROR 0x001e4ebc
    int  playSE(u_int se, int a, int b, float c, int d, int e, int arg); // ROR 0x001e50c8
    int  playStr(int str, int a, float *pos, bool b, u_int flags);       // ROR 0x001e5250
    int  playStr(int str, int a, int b, float c, int d, int e, int arg); // ROR 0x001e5428
    void set_str_3d_snd_prm(int str, int a, int b, int c, int d);        // ROR 0x001e5540
    /* Function no (SND_FUNC_NO) on the playing streams str: 1 / 8 stop,
     * 2 / 3 / 5 / 0xb set, 4 loop, 0x13 a TskSnd over the parameter block
     * arg (it writes words 3, 8 and 9 back), 0x16 non-zero while one plays. */
    int  funcStr(int str, int no, void *arg);                            // ROR 0x001e5660
    int  getStrNoByChan(int chan);                                       // ROR 0x001e5be0
    void set_se_3d_snd_prm(u_int se, int a, int b, int c, int d);        // ROR 0x001e5de4
    /* The same over the 0x30 SE records (+0xd40, 0x28 bytes each). */
    int  funcSE(u_int se, int no, void *arg);                            // ROR 0x001e5e58
    int  isPlayEnvStr();                                                 // ROR 0x001e6cac

    int m_mode;                 // ROR 0x0000: loadStage sets 1 (update then uses the camera position)
    int m_mstVolSe[2];          // ROR 0x1590: option 3's funcSnd(0xe) words
    int m_mstVolStr[2];         // ROR 0x1598: GamMan::getMstVolStr, option 2's funcSnd(0xf) words
};

#endif /* _GAME_GAMSND_H */
