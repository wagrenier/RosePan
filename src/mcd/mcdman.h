/* ==========================================================================
 *  mcd/mcdman.h
 *
 *  McdMan (mcd/mcdman.cpp, 0x187724..0x18bb8c): the memory card manager.
 *  MnuMcd (mnu/mnumcd.h) holds one at +0xc0 and drives it a command at a
 *  time: request(n) queues command word n of the table at +0 (init fills it
 *  with n + 1, the McdCmd values), update runs it a step per frame, and
 *  getStatus says busy (1), done (2) or failed (3), m_result why (McdResult,
 *  named by the ERR_ strings init stores). Each libmc command is the same
 *  step machine (m_step): 0 start, 3 issue the call, 4 sceMcSync until it
 *  finishes, 5 end; a "no format" answer retries up to 8 times. Commands
 *  18..22 chain those through m_sub: create the save directory with its
 *  icon, icon.sys, the main file and the 12 data files; read the directory;
 *  write a file through a marked temporary name; read a file; fill a card
 *  with a /FEBDUMMY file (debug). Class name from the file (ror-inferred).
 * ======================================================================== */
#ifndef _MCD_MCDMAN_H
#define _MCD_MCDMAN_H

#include <stddef.h>
#include <string.h>

#include "sdk/eetypes.h"
#include "sdk/libmc.h"

/* The command words (request's table; ror-inferred names from the libmc
 * call each one makes). */
enum McdCmd
{
    MCD_CMD_NONE         = 0,
    MCD_CMD_GETINFO      = 1,
    MCD_CMD_GETSLOTMAX   = 2,
    MCD_CMD_GETENTSPACE  = 3,
    MCD_CMD_CHDIR        = 4,
    MCD_CMD_CLOSE        = 5,
    MCD_CMD_DELETE       = 6,   /* m_path; with a '*' every match in m_dir */
    MCD_CMD_DELETENOW    = 7,
    MCD_CMD_FLUSH        = 8,
    MCD_CMD_FORMAT       = 9,
    MCD_CMD_GETDIR       = 10,
    MCD_CMD_MKDIR        = 11,
    MCD_CMD_OPEN         = 12,
    MCD_CMD_READ         = 13,
    MCD_CMD_RENAME       = 14,
    MCD_CMD_SEEK         = 15,
    MCD_CMD_UNFORMAT     = 16,
    MCD_CMD_WRITE        = 17,
    MCD_CMD_CREATESAVE   = 18,
    MCD_CMD_READDIR      = 19,
    MCD_CMD_WRITEFILE    = 20,
    MCD_CMD_READFILE     = 21,
    MCD_CMD_FILLDUMMY    = 22,
    MCD_CMD_NUM          = 22
};

/* m_result: the index of its name in m_err_name (init's ERR_ strings). */
enum McdResult
{
    MCD_ERR_NOERR            = 1,
    MCD_ERR_CMDERR           = 2,
    MCD_ERR_CAPACITY         = 3,
    MCD_ERR_NOINIT           = 4,
    MCD_ERR_NOTPATHNAME      = 5,
    MCD_ERR_SEMAPHO          = 6,
    MCD_ERR_NOFORMAT         = 7,
    MCD_ERR_DENIEDPERMIT     = 8,
    MCD_ERR_NOMEMORYCARD     = 9,
    MCD_ERR_NOFUNCTION       = 10,
    MCD_ERR_NOTFD            = 11,
    MCD_ERR_NOTFILENAME      = 12,
    MCD_ERR_NOTOPEN          = 13,
    MCD_ERR_NOTMEMCARDACCESS = 14,
    MCD_ERR_NOTEMPTY         = 15,
    MCD_ERR_FULLDEVICE       = 16,
    MCD_ERR_EXISTENTRY       = 17,
    MCD_ERR_UPLIMITHANDLE    = 18,
    MCD_ERR_NOTREAD          = 19,
    MCD_ERR_FAILREPLACE      = 20,
    MCD_ERR_CHANGEDCARD      = 21,
    MCD_ERR_NUM              = 22
};

/* getStatus. */
enum McdStatus
{
    MCD_STATUS_IDLE  = 0,
    MCD_STATUS_BUSY  = 1,
    MCD_STATUS_DONE  = 2,
    MCD_STATUS_ERROR = 3
};

/* m_step of a libmc command. */
enum McdStep
{
    MCD_STEP_START = 0,
    MCD_STEP_NEXT  = 1,         /* delete: the next match */
    MCD_STEP_ISSUE = 3,
    MCD_STEP_SYNC  = 4,
    MCD_STEP_END   = 5
};

/* One sceMcGetDir entry (sdk/libmc.h sceMcTblGetDir) as McdMan keeps it;
 * declared here without the SDK type's 64-byte alignment, which would
 * spread into every class that holds an MnuMcd. EE 0x40. */
struct McdDirEntry
{
    sceMcStDateTime m_create;   // ROR 0x00
    sceMcStDateTime m_modify;   // ROR 0x08: MnuSvld::readSlots shows it
    u_int  m_size;              // ROR 0x10: FileSizeByte
    u_char m_14[0x20 - 0x14];
    char   m_name[0x20];        // ROR 0x20: EntryName
};
static_assert(sizeof(McdDirEntry) == 0x40, "EE 0x40");
static_assert(offsetof(McdDirEntry, m_modify) == 0x08, "m_modify");
static_assert(offsetof(McdDirEntry, m_size) == 0x10, "m_size");
static_assert(offsetof(McdDirEntry, m_name) == 0x20, "m_name");

/* McdMan writes it as the file (0x3c4 bytes, hasIconSys checks the size). */
static_assert(sizeof(sceMcIconSys) == 0x3c4, "icon.sys");

#define MCD_DIR_MAX   19       /* getDir's maxent */
#define MCD_ICON_NUM  1         /* icons written (the ROM's name tables hold 3) */
#define MCD_DATA_NUM  12        /* dt00 .. dt11 */

/* EE 0xac0 (MnuMcd +0xc0..+0xb80). layout: host -- m_data (+0x80) is the
 * first pointer; the offsets before it are exact. */
struct McdMan
{
    McdMan();                                                            // ROR 0x00187724
    ~McdMan();                                                           // ROR 0x00187738
    void init();                                                         // ROR 0x00187788
    void term();                                                         // ROR 0x001879c4
    /* Takes icon no out of data/mnurpk/mcdicon.rpk (read on first use). */
    int  loadData(int no);                                               // ROR 0x00187b10
    void releaseData();                                                  // ROR 0x00187c30
    void update();                                                       // ROR 0x00187c4c
    int  getStatus();                                                    // ROR 0x00187e00
    /* Queues command word n: 1 taken, 6 when one is still running. */
    int  request(int n);                                                 // ROR 0x00187e08
    /* 1 when the directory holds an icon.sys of 0x3c4 bytes. */
    int  hasIconSys();                                                   // ROR 0x0018b874
    /* Whether the entry matching name exists and is not the ________ one. */
    int  findSave(const char *name);                                     // ROR 0x0018b9b8
    /* Enough free space for the save next to what the directory holds. */
    int  hasSpace();                                                     // ROR 0x0018bb28

    /* The ROM's inlines: m_dir reads checked against m_file_num, and the
     * m_path copy that takes NULL. */
    const char *getDirName(int i) const { return i < m_file_num ? m_dir[i].m_name : NULL; }
    int  getDirSize(int i) const { return i < m_file_num ? (int)m_dir[i].m_size : 0; }
    void setPath(const char *path)
    {
        if (path)
            strcpy(m_path, path);
        else
            m_path[0] = 0;
    }

    int  freeIconPack();                                                 // ROR 0x001879e0
    int  loadIconPack();                                                 // ROR 0x00187a28
    int  cmdGetInfo();                                                   // ROR 0x00187e44
    int  cmdGetSlotMax();                                                // ROR 0x001880cc
    int  cmdGetEntSpace();                                               // ROR 0x001881c0
    int  cmdChdir();                                                     // ROR 0x001883c0
    int  cmdClose();                                                     // ROR 0x001885d4
    /* The next m_dir entry from i matching path's '*' pattern, its full
     * name to out; returns the index after it (-1 past the end). */
    int  findNextMatch(int i, const char *path, char *out);              // ROR 0x001887c0
    int  cmdDelete();                                                    // ROR 0x0018894c
    int  cmdDeleteNow();                                                 // ROR 0x00188bfc
    int  cmdFlush();                                                     // ROR 0x00188cd4
    int  cmdFormat();                                                    // ROR 0x00188ec0
    int  cmdGetDir();                                                    // ROR 0x00189068
    int  cmdMkdir();                                                     // ROR 0x001892b0
    int  cmdOpen();                                                      // ROR 0x001894d4
    int  cmdRead();                                                      // ROR 0x00189718
    int  cmdRename();                                                    // ROR 0x0018993c
    int  cmdSeek();                                                      // ROR 0x00189b7c
    int  cmdUnformat();                                                  // ROR 0x00189d94
    int  cmdWrite();                                                     // ROR 0x00189f58
    void makeIconSys();                                                  // ROR 0x0018a17c
    void makeDataPath(int no, char *path);                               // ROR 0x0018a3f4
    void freeWork();                                                     // ROR 0x0018a4d4
    int  cmdCreateSave();                                                // ROR 0x0018a50c
    int  cmdReadDir();                                                   // ROR 0x0018ad0c
    int  cmdWriteFile();                                                 // ROR 0x0018ade0
    int  cmdReadFile();                                                  // ROR 0x0018b12c
    int  cmdFillDummy();                                                 // ROR 0x0018b2bc
    /* 0 when name matches pat ('?' in either matches any char), else -1. */
    int  matchName(const char *name, const char *pat);                   // ROR 0x0018b918
    /* 0 no entry matches name, 1 one does with this size, 2 another size. */
    int  checkFile(const char *name, int size);                          // ROR 0x0018ba68

    /* PC helpers (not in the ROM, where each command spells them out): the
     * shared steps of the libmc commands and of the composite ones. */
    void issued(int ret, int named);
    int  synced();
    void retryNoFormat();
    void setDone();
    void next(int sub);
    void setOpenMode(int found);
    int  stepEnd(int end) const { return m_status == MCD_STATUS_BUSY ? end : 1; }
    int  subEnd() const { return m_status == MCD_STATUS_ERROR; }

    int          m_cmd_word[MCD_CMD_NUM]; // ROR 0x00: request's table
    int          m_result;          // ROR 0x58: McdResult
    int          m_cmd;             // ROR 0x5c: the running McdCmd, 0 idle
    int          m_step;            // ROR 0x60: McdStep
    int          m_status;          // ROR 0x64: McdStatus
    int          m_sub;             // ROR 0x68: the composite command's step
    int          m_ret;             // ROR 0x6c: the last call's own return value
    int          m_port;            // ROR 0x70: MnuMcd::execCmd sets it
    int          m_slot;            // ROR 0x74
    int          m_slot_max;        // ROR 0x78: getSlotMax
    int          m_type;            // ROR 0x7c: getInfo's card type
    void        *m_data;            // ROR 0x80: createSave's 0x3000-byte zero buffer
    int          m_free;            // ROR 0x84: getInfo's free clusters (KB)
    int          m_format;          // ROR 0x88: getInfo's format flag
    int          m_ent_space;       // ROR 0x8c: getEntSpace
    int          m_file_num;        // ROR 0x90: entries in m_dir after getDir
    int          m_fd;              // ROR 0x94: open's file
    int          m_dir_mode;        // ROR 0x98: getDir's mode
    int          m_open_mode;       // ROR 0x9c: open's mode
    int          m_rw_size;         // ROR 0xa0: bytes read / written
    int          m_seek_pos;        // ROR 0xa4: seek's answer
    int          m_seek_offset;     // ROR 0xa8
    int          m_seek_mode;       // ROR 0xac
    int          m_size;            // ROR 0xb0: bytes to read / write
    void        *m_buf;             // ROR 0xb4: the buffer to read into / write from
    int          m_del_index;       // ROR 0xb8: delete's next m_dir index, -1 none
    int          m_retry;           // ROR 0xbc: issues of the running command
    int          m_open_tries;      // ROR 0xc0: writeFile's failed opens
    int          m_index;           // ROR 0xc4: the composite command's icon / data file
    int          m_sync_mode;       // ROR 0xc8: sceMcSync's mode (1 poll)
    int          m_sync_cmd;        // ROR 0xcc
    int          m_sync_result;     // ROR 0xd0
    char         m_work[0x40];      // ROR 0xd4: delete's / rename's name
    char         m_path[0x40];      // ROR 0x114: the file or directory a command works on
    char         m_name[0x40];      // ROR 0x154: chdir's answer; writeFile's file
    char        *m_mark;            // ROR 0x194: writeFile's temporary mark in m_name
    u_char       _198[0x1c0 - 0x198];
    McdDirEntry  m_dir[MCD_DIR_MAX]; // ROR 0x1c0: getDir's table
    const char  *m_err_name[MCD_ERR_NUM]; // ROR 0x680: by McdResult ([0] unset)
    int          m_icon_size[MCD_ICON_NUM]; // ROR 0x6d8: loadData's icon sizes
    void        *m_icon[MCD_ICON_NUM]; // ROR 0x6dc: loadData's icons (in m_pack)
    void        *m_pack;            // ROR 0x6e0: data/mnurpk/mcdicon.rpk
    int          m_pack_size;       // ROR 0x6e4
    sceMcIconSys m_icon_sys;        // ROR 0x6e8: makeIconSys
    int          m_fill_kb;         // ROR 0xaac: fillDummy's write in KB
    u_char       _ab0[0x10];
};
static_assert(offsetof(McdMan, m_result) == 0x58, "m_result");
static_assert(offsetof(McdMan, m_cmd) == 0x5c, "m_cmd");
static_assert(offsetof(McdMan, m_step) == 0x60, "m_step");
static_assert(offsetof(McdMan, m_status) == 0x64, "m_status");
static_assert(offsetof(McdMan, m_sub) == 0x68, "m_sub");
static_assert(offsetof(McdMan, m_ret) == 0x6c, "m_ret");
static_assert(offsetof(McdMan, m_port) == 0x70, "m_port");
static_assert(offsetof(McdMan, m_slot) == 0x74, "m_slot");
static_assert(offsetof(McdMan, m_slot_max) == 0x78, "m_slot_max");
static_assert(offsetof(McdMan, m_type) == 0x7c, "m_type");
static_assert(offsetof(McdMan, m_data) == 0x80, "m_data");
static_assert(offsetof(McdMan, m_free) >= 0x84, "m_free");
static_assert(offsetof(McdMan, m_file_num) >= 0x90, "m_file_num");
static_assert(offsetof(McdMan, m_size) >= 0xb0, "m_size");
static_assert(offsetof(McdMan, m_buf) >= 0xb4, "m_buf");
static_assert(offsetof(McdMan, m_sync_result) >= 0xd0, "m_sync_result");
static_assert(offsetof(McdMan, m_work) >= 0xd4, "m_work");
static_assert(offsetof(McdMan, m_path) >= 0x114, "m_path");
static_assert(offsetof(McdMan, m_name) >= 0x154, "m_name");
static_assert(offsetof(McdMan, m_dir) >= 0x1c0, "m_dir");
static_assert(offsetof(McdMan, m_err_name) >= 0x680, "m_err_name");
static_assert(offsetof(McdMan, m_icon_sys) >= 0x6e8, "m_icon_sys");
static_assert(offsetof(McdMan, m_fill_kb) >= 0xaac, "m_fill_kb");
static_assert(sizeof(McdMan) >= 0xac0, "EE 0xac0");

#endif /* _MCD_MCDMAN_H */
