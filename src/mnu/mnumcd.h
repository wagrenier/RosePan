/* ==========================================================================
 *  mnu/mnumcd.h
 *
 *  MnuMcd (mnu/mnumcd.cpp, 0x1a1244..0x1a4bd8): the memory card check and
 *  the save / load card work. MnuTitle holds one at +0 (execLogo runs check
 *  mode 0 before the logos), MagLifeMenuMcd one at +0x40, and the save
 *  screen code drives modes 2..6. Class name from the code's own users
 *  (ror-inferred).
 *
 *  processPad (exec) steps m_state: open two scene windows and the McdMan,
 *  load its data, then idle at 4 until startCheck(mode) asks for a check;
 *  the check mode's function (m_mode: 0 checkBoot, 1 checkBootDebug, 2
 *  checkSave, 3 checkLoad, 4 checkInfo, 5 execSave, 6 execLoad) steps
 *  m_step and returns 1 when finished (m_result: 1 ok, 2 / 3 / 4 failed or
 *  cancelled), processPad then returns 1. The checks queue card commands
 *  with requestCmd(slot, cmd); execCmd runs them through McdMan and leaves
 *  a state per slot (getSlotState): 0 pending, 1 card ok, 2, 3 ok but no
 *  free space, 4 unformatted, 5 our directory has icon.sys, 6 / 7 no save
 *  directory yet, 8 no card / error (the meanings from how the checks use
 *  them; unverified). updateMessage puts the system texts of m_msg_no /
 *  m_choice into the two windows.
 * ======================================================================== */
#ifndef _MNU_MNUMCD_H
#define _MNU_MNUMCD_H

#include <stddef.h>

#include "sdk/eetypes.h"
#include "mcd/mcdman.h"
#include "msg/msgman.h"

struct BedPad;
struct DogTexInfo;
struct I3dLayer;
struct MagSystem;

/* EE 0x1400 (MagLifeMenuMcd +0x40..+0x1440; MnuTitle's next member is at
 * +0x1400). layout: host -- m_system (+0x58) is the first pointer; the
 * offsets before it are exact. */
struct MnuMcd
{
    /* callcheck: McdMan::McdMan (the member's constructor, implicit on the
     * host), MsgMan::MsgMan (not written: the member starts zeroed in the
     * global scene). */
    MnuMcd();                                                            // ROR 0x001a1244
    /* callcheck: MsgMan::~MsgMan, McdMan::~McdMan (implicit). */
    ~MnuMcd();                                                           // ROR 0x001a12a8

    void init(DogTexInfo *tex);                                          // ROR 0x001a134c
    void term();                                                         // ROR 0x001a13f0
    int  isInit();                                                       // ROR 0x001a140c
    int  processPad(BedPad *pad);                                        // ROR 0x001a1414
    void draw(I3dLayer *layer);                                          // ROR 0x001a1428
    /* When idle (m_state 4): run check mode on the next processPad and
     * return 1, else 0. */
    int  startCheck(int mode);                                           // ROR 0x001a1658
    void setTextMode(int mode);                                          // ROR 0x001a1650
    int  getSlotState(int slot);                                         // ROR 0x001a1684
    /* The save's file path and directory, its size and data. */
    void setSaveFile(const char *path, const char *dir, int size, void *buf); // ROR 0x001a1b8c
    void setLoadFile(const char *path, int size, void *buf);             // ROR 0x001a1c08
    void setDir(const char *dir);                                        // ROR 0x001a1c64
    int  getFileNum();                                                   // ROR 0x001a1c8c
    const char *getFileName(int i);                                      // ROR 0x001a1c94
    u_int getFileSize(int i);                                            // ROR 0x001a1cbc
    /* Row and column sums of a 0x2d10-byte save image, stored after it. */
    void makeChecksum(u_char *data);                                     // ROR 0x001a1514
    int  testChecksum(u_char *data);                                     // ROR 0x001a1574

    int  open();                                                         // ROR 0x001a1430
    int  close();                                                        // ROR 0x001a1468
    int  setSlotState(int slot, int state);                              // ROR 0x001a16b4
    void setSlot(int slot);                                              // ROR 0x001a1714
    int  requestCmd(int slot, int cmd);                                  // ROR 0x001a171c
    void setMessage(int no);                                             // ROR 0x001a177c
    void setChoice(int no);                                              // ROR 0x001a17b0
    int  isCmdDone();                                                    // ROR 0x001a17dc
    int  exec(BedPad *pad);                                              // ROR 0x001a17fc
    int  resetCursors();                                                 // ROR 0x001a1b2c
    int  updateWork();                                                   // ROR 0x001a1b54
    int  execCmd();                                                      // ROR 0x001a1ce4
    int  checkBoot(BedPad *pad);                                         // ROR 0x001a239c
    int  checkBootDebug(BedPad *pad);                                    // ROR 0x001a2b14
    int  checkSave(BedPad *pad);                                         // ROR 0x001a30cc
    int  hasSaveData();                                                  // ROR 0x001a3568
    int  checkLoad(BedPad *pad);                                         // ROR 0x001a36c4
    int  checkInfo();                                                    // ROR 0x001a3b80
    int  execSave(BedPad *pad);                                          // ROR 0x001a3c4c
    int  execLoad(BedPad *pad);                                          // ROR 0x001a44fc
    void updateMessage();                                                // ROR 0x001a47f0

    int        m_open;          // ROR 0x00: open / close
    int        m_init;          // ROR 0x04: isInit
    /* Each check mode's yes / no cursor ([mode]; 0 / 1, 2 no prompt). */
    int        m_cursor[7];     // ROR 0x08
    int        m_state;         // ROR 0x24: exec's step
    int        m_next;          // ROR 0x28: the step after the idle one (4 idle, 2 run a check)
    int        m_mode;          // ROR 0x2c: the running check mode
    int        m_req_mode;      // ROR 0x30: startCheck's mode
    int        m_step;          // ROR 0x34: the check mode's step
    int        m_debug_next;    // ROR 0x38: checkBootDebug's pending step (0x35, 0x32 to finish)
    int        m_3c;            // ROR 0x3c: exec clears it when it opens; role unknown
    u_char     _40[0x58 - 0x40];
    MagSystem *m_system;        // ROR 0x58: MagSystem::m_current_system (init)
    u_char     _5c[0x64 - 0x5c];
    int        m_blink;         // ROR 0x64: frame counter (0..0x30)
    int        m_wait;          // ROR 0x68: countdown of a message that stays (0x3c frames)
    int        m_poll;          // ROR 0x6c: execCmd's frame counter while m_auto
    int        m_auto;          // ROR 0x70: 1: execCmd repeats getInfo every 8 frames (nothing sets 1 here)
    int        m_slot;          // ROR 0x74: the card slot of the current command
    int        m_slot_state[2]; // ROR 0x78: getSlotState
    int        m_debug_state[2]; // ROR 0x80: checkBootDebug's copy of the slot states
    int        m_both;          // ROR 0x88: check slot 2 as well (init clears it; nothing in this file sets it)
    int        m_cmd;           // ROR 0x8c: the command execCmd runs, 0 none
    int        m_req;           // ROR 0x90: the queued command
    u_char     _94[0xc0 - 0x94];
    McdMan     m_mcd;           // ROR 0xc0: EE 0xac0
    MsgManSet  m_msg;           // ROR 0xb80: the two windows' texts
    int        m_mcd_status;    // ROR 0x12f8: the last McdMan::getStatus
    int        m_mcd_req;       // ROR 0x12fc: the last McdMan::request
    int        m_win[2];        // ROR 0x1300: the scene windows (message, choice), -1 none
    int        m_size;          // ROR 0x1308: setSaveFile / setLoadFile
    void      *m_buf;           // ROR 0x130c
    char       m_path[0x40];    // ROR 0x1310: the file path
    char       m_dir[0x40];     // ROR 0x1350: the save directory (findSave skips its first 14 chars)
    int        m_result;        // ROR 0x1390: the finished check (1 ok, 2 / 3 / 4 failed or cancelled)
    u_char     _1394[0x13ac - 0x1394];
    /* 1 freeze the windows, 2 hide them, 4 refresh the font, 8 restart
     * the blink, 0x10 new message, 0x20 new choice. */
    u_int      m_flags;         // ROR 0x13ac
    int        m_msg_no;        // ROR 0x13b0: setMessage
    int        m_choice;        // ROR 0x13b4: setChoice (10 none)
    int        m_text_mode;     // ROR 0x13b8: setTextMode
    int        m_shown;         // ROR 0x13bc: 1 while the windows show
    int        m_decided;       // ROR 0x13c0: execSave's prompts: cross pressed
    u_char     _13c4[0x1400 - 0x13c4];
};
static_assert(offsetof(MnuMcd, m_init) == 0x04, "m_init");
static_assert(offsetof(MnuMcd, m_cursor) == 0x08, "m_cursor");
static_assert(offsetof(MnuMcd, m_state) == 0x24, "m_state");
static_assert(offsetof(MnuMcd, m_next) == 0x28, "m_next");
static_assert(offsetof(MnuMcd, m_mode) == 0x2c, "m_mode");
static_assert(offsetof(MnuMcd, m_req_mode) == 0x30, "m_req_mode");
static_assert(offsetof(MnuMcd, m_step) == 0x34, "m_step");
static_assert(offsetof(MnuMcd, m_debug_next) == 0x38, "m_debug_next");
static_assert(offsetof(MnuMcd, m_3c) == 0x3c, "m_3c");
static_assert(offsetof(MnuMcd, m_system) == 0x58, "m_system");
static_assert(offsetof(MnuMcd, m_blink) >= 0x64, "m_blink");
static_assert(offsetof(MnuMcd, m_slot) >= 0x74, "m_slot");
static_assert(offsetof(MnuMcd, m_both) >= 0x88, "m_both");
static_assert(offsetof(MnuMcd, m_req) >= 0x90, "m_req");
static_assert(offsetof(MnuMcd, m_mcd) >= 0xc0, "m_mcd");
static_assert(offsetof(MnuMcd, m_msg) >= 0xb80, "m_msg");
static_assert(offsetof(MnuMcd, m_win) >= 0x1300, "m_win");
static_assert(offsetof(MnuMcd, m_result) >= 0x1390, "m_result");
static_assert(offsetof(MnuMcd, m_flags) >= 0x13ac, "m_flags");
static_assert(offsetof(MnuMcd, m_decided) >= 0x13c0, "m_decided");
static_assert(sizeof(MnuMcd) >= 0x1400, "EE 0x1400");

#endif /* _MNU_MNUMCD_H */
