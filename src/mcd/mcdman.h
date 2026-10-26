/* ==========================================================================
 *  mcd/mcdman.h
 *
 *  McdMan (mcd/mcdman.cpp, 0x187724..): the memory card manager. MnuMcd
 *  (mnu/mnumcd.h) holds one at +0xc0 and drives it a command at a time:
 *  request(cmd) queues one (cmd indexes the command word table at +0),
 *  update runs it a step per frame (sceMcGetInfo / sceMcSync ...),
 *  getStatus says when it is done (2) and m_result how it went. Class name
 *  from the file (ror-inferred). Not written yet: only the members MnuMcd
 *  touches are declared, padded to the EE size.
 * ======================================================================== */
#ifndef _MCD_MCDMAN_H
#define _MCD_MCDMAN_H

#include <stddef.h>

#include "sdk/eetypes.h"

/* One sceMcGetDir entry (sdk/libmc.h sceMcTblGetDir) as McdMan keeps it;
 * declared here without the SDK type's 64-byte alignment, which would
 * spread into every class that holds an MnuMcd. EE 0x40. */
struct McdDirEntry
{
    u_char m_00[0x10];          // ROR 0x00: the create / modify dates
    u_int  m_size;              // ROR 0x10: FileSizeByte
    u_char m_14[0x20 - 0x14];
    char   m_name[0x20];        // ROR 0x20: EntryName
};
static_assert(sizeof(McdDirEntry) == 0x40, "EE 0x40");
static_assert(offsetof(McdDirEntry, m_size) == 0x10, "m_size");
static_assert(offsetof(McdDirEntry, m_name) == 0x20, "m_name");

/* EE 0xac0 (MnuMcd +0xc0..+0xb80). layout: host -- m_buf is a pointer;
 * the offsets before it are exact. */
struct McdMan
{
    void init();                                                         // ROR 0x00187788
    void term();                                                         // ROR 0x001879c4
    /* Reads the manager's pfh and takes file no out of it (likely the
     * save icon); MnuMcd passes 0. */
    void loadData(int no);                                               // ROR 0x00187b10
    void releaseData();                                                  // ROR 0x00187c30
    void update();                                                       // ROR 0x00187c4c
    /* 1 busy, 2 done. */
    int  getStatus();                                                    // ROR 0x00187e00
    /* Queues command cmd: 1 taken, 6 when one is still running. */
    int  request(int cmd);                                               // ROR 0x00187e08
    /* 1 when the directory holds an icon.sys of 0x3c4 bytes. */
    int  hasIconSys();                                                   // ROR 0x0018b874
    /* Whether the entry named name exists and is not the ________ one. */
    int  findSave(const char *name);                                     // ROR 0x0018b9b8
    /* Enough free space for the save next to what the directory holds. */
    int  hasSpace();                                                     // ROR 0x0018bb28

    int         m_cmd_word[0x16];   // ROR 0x00: request's table, one word per command
    int         m_result;           // ROR 0x58: the finished command's result (1 ok, 7, 8, 0xe, 0xf, 0x15 ...)
    int         m_cmd;              // ROR 0x5c: the running command word, 0 idle
    int         _60;
    int         m_status;           // ROR 0x64: getStatus
    int         m_68;               // ROR 0x68: request clears it (the command's step)
    int         _6c;
    int         m_port;             // ROR 0x70: the card port MnuMcd::execCmd sets before a request
    u_char      _74[0x84 - 0x74];
    int         m_free;             // ROR 0x84: free space from getInfo (unverified; hasSpace compares it)
    u_char      _88[0x90 - 0x88];
    int         m_file_num;         // ROR 0x90: entries in m_dir after getDir
    u_char      _94[0xb0 - 0x94];
    int         m_size;             // ROR 0xb0: bytes to read / write
    void       *m_buf;              // ROR 0xb4: the buffer to read into / write from
    u_char      _b8[0x114 - 0xb8];
    char        m_path[0x40];       // ROR 0x114: getDir's pattern / the file read or the save directory
    char        m_name[0x40];       // ROR 0x154: the save's file name (MnuMcd::execSave)
    u_char      _194[0x1c0 - 0x194];
    McdDirEntry m_dir[(0xac0 - 0x1c0) / 0x40]; // ROR 0x1c0: getDir's table (count unverified: up to the EE size)
};
static_assert(offsetof(McdMan, m_result) == 0x58, "m_result");
static_assert(offsetof(McdMan, m_cmd) == 0x5c, "m_cmd");
static_assert(offsetof(McdMan, m_status) == 0x64, "m_status");
static_assert(offsetof(McdMan, m_68) == 0x68, "m_68");
static_assert(offsetof(McdMan, m_port) == 0x70, "m_port");
static_assert(offsetof(McdMan, m_free) == 0x84, "m_free");
static_assert(offsetof(McdMan, m_file_num) == 0x90, "m_file_num");
static_assert(offsetof(McdMan, m_size) == 0xb0, "m_size");
static_assert(offsetof(McdMan, m_buf) >= 0xb4, "m_buf");
static_assert(offsetof(McdMan, m_path) >= 0x114, "m_path");
static_assert(offsetof(McdMan, m_name) >= 0x154, "m_name");
static_assert(offsetof(McdMan, m_dir) >= 0x1c0, "m_dir");
static_assert(sizeof(McdMan) >= 0xac0, "EE 0xac0");

#endif /* _MCD_MCDMAN_H */
