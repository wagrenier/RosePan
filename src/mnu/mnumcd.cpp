/* ==========================================================================
 *  mnu/mnumcd.cpp
 *
 *  The memory card check (0x1a1244..0x1a4bd8; RoR names no file here: mnu/
 *  and the name from the class, between MnuTextList's mnu/mnuselect.cpp
 *  and mnu/mnudlg.cpp). See mnu/mnumcd.h.
 *
 *  PORT: McdMan (mcd/mcdman.cpp) and MsgMan (msg/msgman.cpp) are not
 *  written yet: until they are, McdMan::request takes no command and every
 *  check waits at its first card query with empty windows.
 * ======================================================================== */
#include "mnu/mnumcd.h"

#include <stdlib.h>
#include <string.h>

#include "bed/bedpad.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "snd/sndman.h"

/* ---- data ---- */

/* getDir's pattern: the game's save directories (rodata 0x2f8580). */
static const char mnuMcdDirPattern[] = "/BASLUS-21448/*";

/* checkBootDebug's texts (Shift-JIS, rodata 0x2f88e0..0x2f8af8). */
/* "Slots 1 and 2 have no memory card (PS2) inserted." (first half) */
static const char mnuMcdTextNoCard12a[] =
    "\x83X\x83\x8d\x83" "b\x83g1\x81" "A2\x82\xc9\x83\x81\x83\x82\x83\x8a\x81[\x83J\x81[\x83h(PS2)\x82\xaa\x8d\xb7\x82\xb3\x82\xea\x82\xc4"; // 0x002f88e0
/* "...To save [February]'s data" */
static const char mnuMcdTextNoCard12b[] =
    "\x82\xa2\x82\xdc\x82\xb9\x82\xf1\x81" "B\x81yFebruary\x81z\x82\xcc\x83" "f\x81[\x83^\x82\xf0\x83Z\x81[\x83u\x82\xb7\x82\xe9\x82\xc9\x82\xcd"; // 0x002f8910
/* "a memory card (PS2) with ??KB or more free" (full-width brackets) */
static const char mnuMcdTextNeedKbW[] =
    "??KB\x88\xc8\x8f\xe3\x82\xcc\x8b\xf3\x82\xab\x97" "e\x97\xca\x82\xcc\x82\xa0\x82\xe9\x83\x81\x83\x82\x83\x8a\x81[\x83J\x81[\x83h\x81iPS2\x81j\x82\xaa"; // 0x002f8940
/* "is needed. Start the game as it is?" */
static const char mnuMcdTextStartAnyway[] =
    "\x95K\x97v\x82\xc5\x82\xb7\x81" "B\x82\xb1\x82\xcc\x82\xdc\x82\xdc\x83Q\x81[\x83\x80\x82\xf0\x8aJ\x8en\x82\xb5\x82\xdc\x82\xb7\x82\xa9\x81H"; // 0x002f8970
/* "Slot 1 has no memory card (PS2) inserted." */
static const char mnuMcdTextNoCard1[] =
    "\x83X\x83\x8d\x83" "b\x83g1\x82\xc9\x83\x81\x83\x82\x83\x8a\x81[\x83J\x81[\x83h(PS2)\x82\xaa\x8d\xb7\x82\xb3\x82\xea\x82\xc4\x82\xa2\x82\xdc\x82\xb9\x82\xf1\x81" "B"; // 0x002f89a0
/* "Slot 2 has no memory card (PS2) inserted." */
static const char mnuMcdTextNoCard2[] =
    "\x83X\x83\x8d\x83" "b\x83g2\x82\xc9\x83\x81\x83\x82\x83\x8a\x81[\x83J\x81[\x83h(PS2)\x82\xaa\x8d\xb7\x82\xb3\x82\xea\x82\xc4\x82\xa2\x82\xdc\x82\xb9\x82\xf1\x81" "B"; // 0x002f89d8
/* "To save [February]'s data" */
static const char mnuMcdTextToSave[] =
    "\x81yFebruary\x81z\x82\xcc\x83" "f\x81[\x83^\x82\xf0\x83Z\x81[\x83u\x82\xb7\x82\xe9\x82\xc9\x82\xcd"; // 0x002f8a10
/* "a memory card (PS2) with ??KB or more free" */
static const char mnuMcdTextNeedKb[] =
    "??KB\x88\xc8\x8f\xe3\x82\xcc\x8b\xf3\x82\xab\x97" "e\x97\xca\x82\xcc\x82\xa0\x82\xe9\x83\x81\x83\x82\x83\x8a\x81[\x83J\x81[\x83h(PS2)\x82\xaa"; // 0x002f8a38
/* "Checking the memory card (PS2)." */
static const char mnuMcdTextChecking[] =
    "\x83\x81\x83\x82\x83\x8a\x81[\x83J\x81[\x83h(PS2)\x82\xcc\x83`\x83" "F\x83" "b\x83N\x92\x86\x82\xc5\x82\xb7\x81" "B"; // 0x002f8a68
/* "Do not remove the memory cards (PS2) in slots 1 and 2" */
static const char mnuMcdTextDontRemove[] =
    "\x83X\x83\x8d\x83" "b\x83g1\x81" "A2\x82\xcc\x83\x81\x83\x82\x83\x8a\x81[\x83J\x81[\x83h(PS2)\x82\xe2"; // 0x002f8a90
/* "or the controllers." */
static const char mnuMcdTextControllers[] =
    "\x83R\x83\x93\x83g\x83\x8d\x81[\x83\x89\x81[\x82\xc8\x82\xc7\x82\xf0\x94\xb2\x82\xab\x8d\xb7\x82\xb5\x82\xb5\x82\xc8\x82\xa2\x82\xc5\x89\xba\x82\xb3\x82\xa2\x81" "B"; // 0x002f8ab8
/* "Start as it is" */
static const char mnuMcdTextStart[] =
    "\x82\xb1\x82\xcc\x82\xdc\x82\xdc\x8en\x82\xdf\x82\xe9";               // 0x002f8ae8

/* The pad bits the prompts read (bed/bedpad.h). */
#define MCD_PAD_CROSS(pad)  (((pad)->m_7a >> 14) & 1)   /* confirm */
#define MCD_PAD_CIRCLE(pad) (((pad)->m_7c >> 13) & 1)   /* back (checkBootDebug) */
#define MCD_PAD_LEFT        0x80                        /* m_70 */
#define MCD_PAD_RIGHT       0x20

/* The string copy the ROM inlines everywhere here: NULL clears dest. */
static void mcdCopyStr(char *dest, const char *src)
{
    if (src == NULL)
        dest[0] = '\0';
    else
        strcpy(dest, src);
}

/* McdMan's result -> slot state for the directory / file commands that
 * failed or did not end in "ok" (execCmd's shared tail). */
static int mcdErrorState(int result)
{
    switch (result) {
    case 5:   return 6;
    case 7:   return 4;
    case 0xf: return 7;
    default:  return 8;
    }
}

/* ---- the menu ---- */

// ROR 0x001a1244; callcheck: McdMan, MsgMan (member constructors, not written: the members start zeroed in the global scene)
MnuMcd::MnuMcd()
{
    m_open = 0;
    resetCursors();
}

// ROR 0x001a12a8; callcheck: ~MsgMan, ~McdMan, operator_delete (member destructors and the deleting variant: implicit on the host)
MnuMcd::~MnuMcd()
{
    close();
}

// ROR 0x001a134c
void MnuMcd::init(DogTexInfo *tex)
{
    (void)tex;
    m_win[1] = -1;
    m_win[0] = -1;
    m_both = 0;
    open();
    m_init = 1;
    m_auto = 0;
    m_poll = 0;
    for (int i = 0; i < 2; i++)
        m_slot_state[i] = 0;
    m_cmd = 0;
    m_req = 0;
    m_system = MagSystem::m_current_system;
    setSlot(0);
    m_flags = 2;
    m_choice = 10;
    m_shown = 0;
    m_msg_no = 0;
    m_text_mode = 0;
}

// ROR 0x001a13f0
void MnuMcd::term()
{
    close();
}

// ROR 0x001a140c
int MnuMcd::isInit()
{
    return m_init;
}

// ROR 0x001a1414
int MnuMcd::processPad(BedPad *pad)
{
    return exec(pad);
}

// ROR 0x001a1428
void MnuMcd::draw(I3dLayer *layer)
{
    (void)layer;
}

// ROR 0x001a1430
int MnuMcd::open()
{
    if (m_open == 1)
        return -1;
    m_open = 1;
    m_step = 0;
    m_state = 0;
    m_next = 4;
    m_mode = 0;
    m_req_mode = 0;
    return 0;
}

// ROR 0x001a1468
int MnuMcd::close()
{
    if (m_open == 0)
        return -1;
    MagScene *scene = &m_system->m_scene;
    if (m_win[0] >= 0) {
        scene->closeWindow(m_win[0]);
        m_win[0] = -1;
    }
    if (m_win[1] >= 0) {
        scene->closeWindow(m_win[1]);
        m_win[1] = -1;
    }
    m_mcd.releaseData();
    m_mcd.term();
    m_msg.close();
    m_open = 0;
    m_init = 0;
    return 0;
}

// ROR 0x001a1514
void MnuMcd::makeChecksum(u_char *data)
{
    u_char *p = data;
    for (int row = 0; row < 0x2d1; row++) {
        for (int col = 0; col < 0x10; col++) {
            data[0x2d20 + row] += *p;
            data[0x2d10 + col] += *p;
            p++;
        }
    }
}

// ROR 0x001a1574
int MnuMcd::testChecksum(u_char *data)
{
    u_char colsum[0x10];
    bool nonzero = false;
    const u_char *p = data;

    for (int i = 0x0f; i >= 0; i--)
        colsum[i] = 0;
    for (int row = 0; row < 0x2d1; row++) {
        u_char sum = 0;
        for (int col = 0; col < 0x10; col++) {
            u_char c = *p++;
            sum += c;
            colsum[col] += c;
            if (c != 0)
                nonzero = true;
        }
        if (sum != data[0x2d20 + row])
            return 0;
    }
    if (!nonzero)
        return 0;
    for (int col = 0; col < 0x10; col++) {
        if (colsum[col] != data[0x2d10 + col])
            return 0;
    }
    return 1;
}

// ROR 0x001a1650
void MnuMcd::setTextMode(int mode)
{
    m_text_mode = mode;
}

// ROR 0x001a1658
int MnuMcd::startCheck(int mode)
{
    int idle = m_state == 4;
    if (idle) {
        m_req_mode = mode;
        m_result = 0;
        m_next = 2;
    }
    return idle;
}

// ROR 0x001a1684
int MnuMcd::getSlotState(int slot)
{
    if (slot < 2)
        return m_slot_state[slot];
    return 8;
}

// ROR 0x001a16b4
int MnuMcd::setSlotState(int slot, int state)
{
    if (slot < 2)
        m_slot_state[slot] = state;
    return slot < 2;
}

// ROR 0x001a1714
void MnuMcd::setSlot(int slot)
{
    m_slot = slot;
}

// ROR 0x001a171c
int MnuMcd::requestCmd(int slot, int cmd)
{
    m_auto = 0;
    int idle = m_req == 0;
    if (idle) {
        setSlot(slot);
        setSlotState(m_slot, 0);
        m_req = cmd;
    }
    return idle;
}

// ROR 0x001a177c
void MnuMcd::setMessage(int no)
{
    if (no == m_msg_no) {
        m_flags &= ~0x10u;
        return;
    }
    m_msg_no = no;
    m_flags |= 0x14;
    m_choice = 10;
}

// ROR 0x001a17b0
void MnuMcd::setChoice(int no)
{
    if (no == m_choice) {
        m_flags &= ~0x20u;
        return;
    }
    m_choice = no;
    m_flags |= 0x20;
}

// ROR 0x001a17dc
int MnuMcd::isCmdDone()
{
    return getSlotState(m_slot) != 0;
}

// ROR 0x001a17fc
int MnuMcd::exec(BedPad *pad)
{
    MagScene *scene;
    int ret = 0;

    if (m_open == 0)
        return -1;
    switch (m_state) {
    case 0:
        m_state = 1;
        m_mcd.init();
        m_3c = 0;
        scene = &m_system->m_scene;
        m_win[0] = scene->openWindow(0x200, 0x100, 0, -1);
        scene->updateWindowFont(m_win[0]);
        m_system->setViewParam(m_win[0], 0x3c, 0x82, 0x7fff, 0x200, 0x100);
        scene->setWindowShow(m_win[0], 1);
        m_win[1] = scene->openWindow(0x200, 0x100, 0, -1);
        scene->updateWindowFont(m_win[1]);
        m_system->setViewParam(m_win[1], 0x3c, 0x82, 0x7fff, 0x200, 0x100);
        scene->setWindowShow(m_win[1], 1);
        m_msg.open();
        ret = 0;
        break;
    case 1:
        m_mcd.loadData(0);
        m_state = 4;
        break;
    case 2: {
        int done;
        switch (m_mode) {
        case 0:
            if (checkBoot(pad) == 1)
                m_state = 5;
            updateWork();
            return 0;
        case 1: done = checkBootDebug(pad); break;
        case 2: done = checkSave(pad); break;
        case 3: done = checkLoad(pad); break;
        case 4: done = checkInfo(); break;
        case 5: done = execSave(pad); break;
        case 6: done = execLoad(pad); break;
        default: done = 0; break;
        }
        if (done == 1)
            m_state = 4;
        updateWork();
        ret = 0;
        break;
    }
    case 4: {
        int next = m_next;
        m_next = 4;
        m_state = next;
        m_mode = m_req_mode;
        break;
    }
    case 5:
        m_auto = 0;
        m_state = 6;
        break;
    case 6:
        updateWork();
        if (m_cmd == 0) {
            scene = &m_system->m_scene;
            scene->closeWindow(m_win[0]);
            m_win[0] = -1;
            scene->closeWindow(m_win[1]);
            m_win[1] = -1;
            m_mcd.releaseData();
            m_mcd.term();
            m_state = 7;
            m_msg.close();
        }
        ret = 0;
        break;
    case 3:
    case 7:
    default:
        m_req_mode = 0;
        m_next = 2;
        ret = 1;
        m_state = 0;
        break;
    }
    return ret;
}

// ROR 0x001a1b2c
int MnuMcd::resetCursors()
{
    for (int i = 0; i < 7; i++)
        m_cursor[i] = 0;
    return 0;
}

// ROR 0x001a1b54
int MnuMcd::updateWork()
{
    execCmd();
    m_mcd.update();
    updateMessage();
    return 0;
}

// ROR 0x001a1b8c; callcheck: strcpy (inside mcdCopyStr, the copy the ROM inlines)
void MnuMcd::setSaveFile(const char *path, const char *dir, int size, void *buf)
{
    mcdCopyStr(m_path, path);
    mcdCopyStr(m_dir, dir);
    m_buf = buf;
    m_size = size;
}

// ROR 0x001a1c08; callcheck: strcpy (inside mcdCopyStr, the copy the ROM inlines)
void MnuMcd::setLoadFile(const char *path, int size, void *buf)
{
    mcdCopyStr(m_path, path);
    m_buf = buf;
    m_size = size;
    m_dir[0] = '\0';
}

// ROR 0x001a1c64; callcheck: strcpy (inside mcdCopyStr, the copy the ROM inlines)
void MnuMcd::setDir(const char *dir)
{
    mcdCopyStr(m_dir, dir);
}

// ROR 0x001a1c8c
int MnuMcd::getFileNum()
{
    return m_mcd.m_file_num;
}

// ROR 0x001a1c94
const char *MnuMcd::getFileName(int i)
{
    if (i < m_mcd.m_file_num)
        return m_mcd.m_dir[i].m_name;
    return NULL;
}

// ROR 0x001a1cbc
u_int MnuMcd::getFileSize(int i)
{
    if (i < m_mcd.m_file_num)
        return m_mcd.m_dir[i].m_size;
    return 0;
}

// ROR 0x001a1ce4; callcheck: strcpy (inside mcdCopyStr, the copy the ROM inlines)
int MnuMcd::execCmd()
{
    int st;

    switch (m_cmd) {
    case 0:
        if (m_auto != 1) {
            if (m_req == 0)
                return 0;
            m_cmd = m_req;
            m_req = 0;
            setSlotState(m_slot, 0);
            return 0;
        }
        if (m_req != 0) {
            m_cmd = m_req;
            m_req = 0;
            setSlotState(m_slot, 0);
            return 0;
        }
        if ((m_poll & 7) == 4) {
            m_cmd = 1;
            setSlotState(m_slot, 0);
        }
        m_poll++;
        break;

    /* getInfo */
    case 1:
        m_mcd.m_port = m_slot;
        setSlotState(m_slot, 0);
        m_mcd_req = m_mcd.request(0);
        if (m_mcd_req != 1)
            break;
        m_cmd = 2;
        /* fall through */
    case 2:
        m_mcd_status = st = m_mcd.getStatus();
        if (st == 1)
            break;
        m_cmd = 0;
        if (st == 2) {
            switch (m_mcd.m_result) {
            case 1:
            case 0x15:
                setSlotState(m_slot, 1);
                if (m_mcd.m_free == 0)
                    setSlotState(m_slot, 3);
                return 0;
            case 7:
                setSlotState(m_slot, 4);
                return 0;
            case 8:
            case 0xe:
                setSlotState(m_slot, 2);
                return 0;
            }
        }
        setSlotState(m_slot, 8);
        return 0;

    case 3:
        break;

    /* getDir on the save directories */
    case 4:
        m_mcd.m_port = m_slot;
        setSlotState(m_slot, 0);
        mcdCopyStr(m_mcd.m_path, mnuMcdDirPattern);
        m_mcd_req = m_mcd.request(0x12);
        if (m_mcd_req != 1)
            break;
        m_cmd = 5;
        /* fall through */
    case 5:
        m_mcd_status = st = m_mcd.getStatus();
        if (st == 2) {
            m_cmd = 0;
            if (m_mcd.m_result == 1) {
                setSlotState(m_slot, m_mcd.hasIconSys() ? 5 : 7);
                return 0;
            }
            switch (m_mcd.m_result) {
            case 5:   setSlotState(m_slot, 6); break;
            case 7:   setSlotState(m_slot, 4); break;
            case 0xf: setSlotState(m_slot, 7); break;
            default:  setSlotState(m_slot, 8); break;
            }
            return 0;
        }
        if (st == 1)
            return 0;
        setSlotState(m_slot, 8);
        m_cmd = 0;
        setSlotState(m_slot, mcdErrorState(m_mcd.m_result));
        return 0;

    /* McdMan commands 0x11, 0x13 and 0x14 (execSave's 6 and 8, execLoad's
     * 10): 1 when they end ok. */
    case 6:
    case 8:
    case 10: {
        static const int mcdCmd[3] = { 0x11, 0x13, 0x14 };
        int cmd = m_cmd;
        m_mcd.m_port = m_slot;
        setSlotState(m_slot, 0);
        m_mcd_req = m_mcd.request(mcdCmd[(cmd - 6) / 2]);
        if (m_mcd_req != 1)
            break;
        m_cmd = cmd + 1;
    }
        /* fall through */
    case 7:
    case 9:
    case 11:
        m_mcd_status = st = m_mcd.getStatus();
        if (st == 2) {
            m_cmd = 0;
            if (m_mcd.m_result == 1)
                setSlotState(m_slot, 1);
            else
                setSlotState(m_slot, mcdErrorState(m_mcd.m_result));
            return 0;
        }
        if (st == 1)
            return 0;
        setSlotState(m_slot, 8);
        m_cmd = 0;
        setSlotState(m_slot, mcdErrorState(m_mcd.m_result));
        return 0;

    /* McdMan command 8 (execSave's 0xc, before it creates the directory) */
    case 0xc:
        m_mcd.m_port = m_slot;
        setSlotState(m_slot, 0);
        m_mcd_req = m_mcd.request(8);
        if (m_mcd_req == 1)
            m_cmd = 0xd;
        break;
    case 0xd:
        m_mcd_status = st = m_mcd.getStatus();
        if (st == 2) {
            m_cmd = 0;
            setSlotState(m_slot, m_mcd.m_result == 1 ? 1 : 8);
            return 0;
        }
        if (st == 1)
            return 0;
        m_cmd = 0;
        setSlotState(m_slot, 8);
        break;

    default:
        break;
    }
    return 0;
}

// ROR 0x001a239c
int MnuMcd::checkBoot(BedPad *pad)
{
    int ret = 0;
    int st;

/* A message step: shows the windows (flags 1 / 2 off). */
#define MCD_MSG_STEP(n) do { m_step = (n); m_flags &= ~3u; } while (0)

    switch (m_step) {
    case 0:
        m_step = 0x1c;
        m_flags |= 2;
        break;
    case 1:
        if (requestCmd(0, 1) == 1)
            m_step = 2;
        break;
    case 2:
        if (isCmdDone() == 1)
            m_step = m_both == 0 ? 0x12 : 3;
        break;
    case 3:
        if (requestCmd(1, 1) == 1)
            m_step = 4;
        break;
    case 4:
        if (isCmdDone() == 1)
            m_step = 9;
        break;
    case 9:
        if (getSlotState(0) == 4 || getSlotState(1) == 4)
            goto ok;
        if (getSlotState(0) == 3 || getSlotState(0) == 1)
            goto dir0;
        if (getSlotState(1) == 3 || getSlotState(1) == 1)
            goto dir1;
        if (getSlotState(0) == 8 && getSlotState(1) == 8)
            MCD_MSG_STEP(0x1d);
        else if (getSlotState(0) == 8)
            MCD_MSG_STEP(0x23);
        else if (getSlotState(1) == 8)
            MCD_MSG_STEP(0x23);
        break;
    case 10:
        if (requestCmd(0, 4) == 1)
            m_step = 0xe;
        break;
    case 0xe:
        if (isCmdDone() != 1)
            break;
        st = getSlotState(m_slot);
        if (st < 8) {
            if (st < 6) {
                if (st == 5)
                    goto ok;
                break;
            }
            if (m_mcd.hasSpace() == 1)
                goto ok;
            if (m_both != 0 && (getSlotState(1) == 3 || getSlotState(1) == 1))
                goto dir1;
            MCD_MSG_STEP(0x28);
        } else if (st == 8) {
            if (m_both != 0 && (getSlotState(1) == 3 || getSlotState(1) == 1))
                goto dir1;
            MCD_MSG_STEP(0x1d);
        }
        break;
    case 0xf:
        if (requestCmd(1, 4) == 1)
            m_step = 0x10;
        break;
    case 0x10:
        if (isCmdDone() == 1)
            m_step = 0x11;
        break;
    case 0x11:
        if (getSlotState(0) == 8 && getSlotState(1) == 8) {
            MCD_MSG_STEP(0x1d);
            break;
        }
        st = getSlotState(1);
        if (st < 8) {
            if (st > 5) {
                if (m_mcd.hasSpace() == 1) {
                    m_cursor[0] = 1;
                    m_step = 0x32;
                } else {
                    MCD_MSG_STEP(getSlotState(0) == 8 ? 0x2b : 0x2d);
                }
                break;
            }
            if (st == 5)
                goto ok;
            break;
        }
        if (st == 8)
            MCD_MSG_STEP(0x28);
        break;
    case 0x12:
        switch (getSlotState(0)) {
        case 1:
        case 3:
        dir0:
            m_step = 10;
            break;
        case 4:
            goto ok;
        default:
            MCD_MSG_STEP(0x23);
            break;
        }
        break;
    case 0x1c:
        m_step = 1;
        m_cursor[0] = 2;
        m_blink = 0;
        break;
    case 0x1d:
    case 0x23:
        setMessage(m_both != 0 ? 3 : 1);
        goto answer;
    case 0x28:
        setMessage(4);
        goto answer;
    case 0x2b:
        setMessage(5);
        goto answer;
    case 0x2d:
        setMessage(m_both != 0 ? 6 : 4);
    answer:
        if (m_cursor[0] == 2)
            m_cursor[0] = 0;
        break;
    case 0x32:
        m_step = 0x33;
        break;
    case 0x33:
        m_step = 0;
        ret = m_cursor[0] == 1;
        break;
    ok:
        m_cursor[0] = 1;
        m_step = 0x32;
        break;
    dir1:
        m_step = 0xf;
        break;
    default:
        break;
    }
#undef MCD_MSG_STEP

    if (m_step < 0x32 || m_step > 0x33) {
        int *cursor = &m_cursor[0];
        if (*cursor != 2) {
            if (MCD_PAD_CROSS(pad)) {
                g_sndMan.playSe(4);
                if (*cursor == 1) {
                    m_step = 0x32;
                } else {
                    MagScene *scene = &m_system->m_scene;
                    m_step = 0x1c;
                    scene->updateWindowFont(m_win[0]);
                    scene->updateWindowFont(m_win[1]);
                    m_flags &= ~3u;
                    setMessage(m_both == 0 ? 8 : 10);
                    *cursor = 2;
                }
            } else if (pad->m_70 & MCD_PAD_LEFT) {
                if (*cursor != 0) {
                    *cursor = 0;
                    g_sndMan.playSe(1);
                }
            } else if ((pad->m_70 & MCD_PAD_RIGHT) && *cursor != 1) {
                *cursor = 1;
                g_sndMan.playSe(1);
            }
        }
    }
    switch (m_cursor[0]) {
    case 0: setChoice(0); break;
    case 1: setChoice(1); break;
    case 2: setChoice(10); break;
    }
    return ret;
}

// ROR 0x001a2b14
int MnuMcd::checkBootDebug(BedPad *pad)
{
    u_int redraw = 0;
    int ret = 0;

    switch (m_step) {
    case 0:
        m_step = 1;
        redraw = 0x11;
        m_debug_next = 0x35;
        m_cursor[1] = 0;
        m_debug_state[0] = 0;
        m_debug_state[1] = 0;
        m_blink = 0;
        break;
    case 1:
        if (requestCmd(0, 1) == 1)
            m_step = 2;
        break;
    case 2:
        if (isCmdDone() == 1)
            m_step = 3;
        break;
    case 3:
        if (requestCmd(1, 1) == 1)
            m_step = 4;
        break;
    case 4:
        if (isCmdDone() == 1)
            m_step = 9;
        break;
    case 9:
        m_step = 0x1d;
        redraw = 0x11;
        m_debug_state[0] = getSlotState(0);
        m_debug_state[1] = getSlotState(1);
        break;
    case 0x32:
        m_step = 0x33;
        if (m_cursor[1] == 1)
            setSlot(1);
        else if (m_cursor[1] == 0 || m_cursor[1] == 2)
            setSlot(0);
        /* fall through */
    case 0x33: {
        MagScene *scene = &m_system->m_scene;
        scene->setWindowShow(m_win[0], 1);
        ret = 1;
        scene->setWindowShow(m_win[1], 1);
        m_step = 0;
        break;
    }
    }

    if (m_debug_next == 0x32 && m_step == 0x1d)
        m_step = 0x32;
    if (m_step < 0x32 || m_step > 0x33) {
        if (MCD_PAD_CROSS(pad)) {
            switch (m_debug_state[0]) {
            case 0:
            case 2:
            case 8:
                redraw = 2;
                g_sndMan.playSe(3);
                break;
            case 1:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
                redraw = 1;
                if (m_cursor[1] < 2 && m_cursor[1] >= 0) {
                    m_debug_next = 0x32;
                    g_sndMan.playSe(4);
                }
                break;
            }
        } else if (MCD_PAD_CIRCLE(pad)) {
            m_debug_next = 0x32;
            m_cursor[1] = 2;
            g_sndMan.playSe(2);
        } else if (pad->m_70 & MCD_PAD_LEFT) {
            redraw = 1;
            if (m_cursor[1] != 0) {
                m_cursor[1] = 0;
                g_sndMan.playSe(1);
            }
        } else if (pad->m_70 & MCD_PAD_RIGHT) {
            redraw = 1;
            if (m_cursor[1] != 1) {
                m_cursor[1] = 1;
                g_sndMan.playSe(1);
            }
        }
    }

    if (redraw != 0) {
        MagScene *scene = &m_system->m_scene;
        int win = m_win[0];
        scene->updateWindowFont(win);
        scene->setWindowShow(win, 0);
        if (redraw == 2) {
            if (getSlotState(0) == 8)
                m_system->drawView(win, 0, 0x14, mnuMcdTextNoCard1, 0);
            else if (getSlotState(1) == 8)
                m_system->drawView(win, 0, 0x14, mnuMcdTextNoCard2, 0);
            m_system->drawView(win, 0, 0x28, mnuMcdTextToSave, 0);
            m_system->drawView(win, 0, 0x3c, mnuMcdTextNeedKb, 0);
            m_system->drawView(win, 0, 0x50, mnuMcdTextStartAnyway, 0);
        } else if (redraw == 1) {
            m_system->drawView(win, 0, 0x14, mnuMcdTextNoCard12a, 0);
            m_system->drawView(win, 0, 0x28, mnuMcdTextNoCard12b, 0);
            m_system->drawView(win, 0, 0x3c, mnuMcdTextNeedKbW, 0);
            m_system->drawView(win, 0, 0x50, mnuMcdTextStartAnyway, 0);
        } else if (redraw == 99) {
            m_system->drawView(win, 0, 0x14, mnuMcdTextChecking, 0);
            m_system->drawView(win, 0, 0x28, mnuMcdTextDontRemove, 0);
            m_system->drawView(win, 0, 0x3c, mnuMcdTextControllers, 0);
        }
        if (redraw != 99) {
            if (m_cursor[1] == 0)
                m_system->drawView(win, 0x1e, 0x78, mnuMcdTextStart, 0);
            else if (m_cursor[1] == 1)
                m_system->drawView(win, 0x1e, 0x78, mnuMcdTextStart, 8);
        }
    }
    m_blink++;
    return ret;
}

// ROR 0x001a30cc
int MnuMcd::checkSave(BedPad *pad)
{
    u_int msg = 0;
    int ret = 0;

    switch (m_step) {
    case 0:
        m_step = 0x1c;
        break;
    case 1:
        if (requestCmd(0, 1) == 1)
            m_step = 2;
        break;
    case 2:
        if (isCmdDone() != 1)
            break;
        switch (getSlotState(m_slot)) {
        case 1:
        case 3:
            m_step = 3;
            break;
        case 4:
            goto ok;
        default:
            m_step = 0x1d;
            break;
        }
        break;
    case 3:
        if (requestCmd(0, 4) == 1)
            m_step = 4;
        break;
    case 4:
        if (isCmdDone() != 1)
            break;
        switch (getSlotState(m_slot)) {
        case 4:
        case 5:
            goto ok;
        case 6:
        case 7:
            if (m_mcd.hasSpace() != 1) {
                m_step = 0x23;
                break;
            }
            goto ok;
        case 8:
            m_step = 0x1d;
            break;
        }
        break;
    ok:
        m_result = 1;
        m_step = 0x32;
        m_cursor[2] = 1;
        break;
    case 0x1c:
        m_flags &= ~3u;
        m_step = 1;
        setMessage(8);
        m_cursor[2] = 2;
        m_blink = 0;
        break;
    case 0x1d:
        msg = 1;
        if (m_cursor[2] == 2)
            m_cursor[2] = 0;
        break;
    case 0x23:
        msg = 2;
        if (m_cursor[2] == 2)
            m_cursor[2] = 0;
        break;
    case 0x32:
        m_step = 0x33;
        break;
    case 0x33:
        m_flags = 2;
        m_step = 0;
        ret = m_cursor[2] == 1;
        break;
    }

    if (m_step != 0x1c && m_step != 0 && (m_step < 0x32 || m_step > 0x33)) {
        int *cursor = &m_cursor[2];
        if (*cursor != 2) {
            if (MCD_PAD_CROSS(pad)) {
                g_sndMan.playSe(4);
                if (*cursor == 1) {
                    m_result = 2;
                    m_step = 0x32;
                } else {
                    MagScene *scene = &m_system->m_scene;
                    m_step = 0x1c;
                    msg = 99;
                    scene->updateWindowFont(m_win[0]);
                    scene->updateWindowFont(m_win[1]);
                    *cursor = 2;
                }
            } else if (pad->m_70 & MCD_PAD_LEFT) {
                if (*cursor != 0) {
                    *cursor = 0;
                    g_sndMan.playSe(1);
                }
            } else if ((pad->m_70 & MCD_PAD_RIGHT) && *cursor != 1) {
                *cursor = 1;
                g_sndMan.playSe(1);
            }
        }
    }

    if (msg != 0) {
        m_flags &= ~3u;
        if (msg == 2)
            setMessage(7);
        else if (msg == 1)
            setMessage(1);
        else if (msg == 99)
            setMessage(8);
        if (msg != 99) {
            if (m_text_mode == 1) {
                if (m_cursor[2] == 0)
                    setChoice(2);
                else if (m_cursor[2] == 1)
                    setChoice(3);
            } else if (m_text_mode == 2) {
                if (m_cursor[2] == 0)
                    setChoice(4);
                else if (m_cursor[2] == 1)
                    setChoice(5);
            }
        }
    }
    m_blink++;
    return ret;
}

// ROR 0x001a3568
int MnuMcd::hasSaveData()
{
    int i = 0;

    if (getFileNum() > 0) {
        while (i < getFileNum()) {
            const char *name = getFileName(i);
            if (name[0] == 'd' && name[1] == 't') {
                int v[8];
                char num[3];
                bool bad = false;
                for (int k = 0; k < 5; k++) {
                    num[0] = name[2];
                    num[1] = name[3];
                    name += 2;
                    num[2] = '\0';
                    if (num[0] == '_' || num[1] == '_') {
                        bad = true;
                    } else {
                        int n = atoi(num);
                        if (n < 0) {
                            bad = true;
                            break;
                        }
                        v[k] = n;
                    }
                }
                if (!bad && v[0] >= 0 && v[1] / 10 == 0)
                    return 1;
            }
            i++;
        }
    }
    return 0;
}

// ROR 0x001a36c4
int MnuMcd::checkLoad(BedPad *pad)
{
    u_int msg = 0;
    int ret = 0;

    switch (m_step) {
    case 0:
        m_step = 0x1c;
        break;
    case 1:
        if (requestCmd(0, 1) == 1)
            m_step = 2;
        break;
    case 2:
        if (isCmdDone() != 1)
            break;
        switch (getSlotState(m_slot)) {
        case 1:
        case 3:
            m_step = 3;
            break;
        case 4:
            m_step = 0x28;
            break;
        default:
            m_step = 0x1d;
            break;
        }
        break;
    case 3:
        if (requestCmd(0, 4) == 1)
            m_step = 4;
        break;
    case 4:
        if (isCmdDone() != 1)
            break;
        switch (getSlotState(m_slot)) {
        case 5:
        case 7:
            if (hasSaveData() != 0) {
                m_result = 1;
                m_step = 0x32;
                m_cursor[3] = 1;
                break;
            }
            /* fall through */
        case 4:
        case 6:
            m_step = 0x28;
            break;
        case 8:
            m_step = 0x1d;
            break;
        }
        break;
    case 0x1c:
        m_flags &= ~3u;
        m_step = 1;
        setMessage(8);
        m_cursor[3] = 2;
        m_blink = 0;
        break;
    case 0x1d:
        msg = 1;
        if (m_cursor[3] == 2)
            m_cursor[3] = 0;
        break;
    case 0x23:
        msg = 2;
        if (m_cursor[3] == 2)
            m_cursor[3] = 0;
        break;
    case 0x28:
        msg = 3;
        if (m_cursor[3] == 2)
            m_cursor[3] = 0;
        break;
    case 0x32:
        m_step = 0x33;
        break;
    case 0x33:
        m_flags = 2;
        m_step = 0;
        ret = m_cursor[3] == 1;
        break;
    }

    if (m_step != 0x1c && m_step != 0 && (m_step < 0x32 || m_step > 0x33)) {
        int *cursor = &m_cursor[3];
        if (*cursor != 2) {
            if (MCD_PAD_CROSS(pad)) {
                g_sndMan.playSe(4);
                if (*cursor == 1) {
                    m_result = 2;
                    m_step = 0x32;
                } else {
                    MagScene *scene = &m_system->m_scene;
                    m_step = 0x1c;
                    msg = 99;
                    scene->updateWindowFont(m_win[0]);
                    scene->updateWindowFont(m_win[1]);
                    *cursor = 2;
                }
            } else if (pad->m_70 & MCD_PAD_LEFT) {
                if (*cursor != 0) {
                    *cursor = 0;
                    g_sndMan.playSe(1);
                }
            } else if ((pad->m_70 & MCD_PAD_RIGHT) && *cursor != 1) {
                *cursor = 1;
                g_sndMan.playSe(1);
            }
        }
    }

    if (msg != 0) {
        m_flags &= ~3u;
        if (msg == 2)
            setMessage(4);
        else if (msg == 1)
            setMessage(1);
        else if (msg == 3)
            setMessage(0x17);
        else if (msg == 99)
            setMessage(8);
        if (msg != 99) {
            if (m_text_mode == 1) {
                if (m_cursor[3] == 0)
                    setChoice(2);
                else if (m_cursor[3] == 1)
                    setChoice(3);
            } else if (m_text_mode == 2) {
                if (m_cursor[3] == 0)
                    setChoice(4);
                else if (m_cursor[3] == 1)
                    setChoice(5);
            }
        }
    }
    m_blink++;
    return ret;
}

// ROR 0x001a3b80
int MnuMcd::checkInfo()
{
    switch (m_step) {
    case 0:
        if (requestCmd(0, 1) == 1)
            m_step = 1;
        return 0;
    case 1:
        if (isCmdDone() == 1) {
            getSlotState(m_slot);
            m_result = 1;
            m_step = 0x32;
        }
        return 0;
    case 0x32:
        m_step = 0;
        return 1;
    }
    return 0;
}

// ROR 0x001a3c4c; callcheck: strcpy (inside mcdCopyStr, the copy the ROM inlines)
int MnuMcd::execSave(BedPad *pad)
{
    int ret = 0;
    int st;

    switch (m_step) {
    case 0:
        m_flags &= ~3u;
        m_step = 1;
        setMessage(8);
        m_cursor[5] = 2;
        /* fall through */
    case 1:
        if (requestCmd(0, 1) != 1)
            break;
        m_step = 2;
        break;
    case 2:
        if (isCmdDone() != 1)
            break;
        switch (getSlotState(m_slot)) {
        case 1:
            if (m_mcd.m_result != 1) {
                if (m_mcd.m_result == 0x15)
                    goto failed;
                break;
            }
            /* fall through */
        case 3:
            m_step = 0x12;
            break;
        case 4:
            m_step = 3;
            break;
        default:
            goto failed;
        }
        break;

    /* the card is unformatted: format it? */
    case 3:
        setMessage(0xd);
        m_cursor[5] = 0;
        m_step = 4;
        /* fall through */
    case 4:
        m_decided = 0;
        if (requestCmd(0, 1) != 1)
            break;
        m_step = 5;
        /* fall through */
    case 5:
        if (MCD_PAD_CROSS(pad)) {
            m_decided = 1;
            g_sndMan.playSe(4);
        } else if (m_decided == 0) {
            if (pad->m_70 & MCD_PAD_LEFT) {
                if (m_cursor[5] != 0) {
                    m_cursor[5] = 0;
                    g_sndMan.playSe(1);
                }
            } else if ((pad->m_70 & MCD_PAD_RIGHT) && m_cursor[5] != 1) {
                m_cursor[5] = 1;
                g_sndMan.playSe(1);
            }
        }
        if (m_cursor[5] == 0)
            setChoice(6);
        else if (m_cursor[5] == 1)
            setChoice(7);
        if (isCmdDone() != 1)
            break;
        st = getSlotState(m_slot);
        if (st == 4) {
            if (m_decided == 0)
                m_step = 4;
            else if (m_cursor[5] == 0)
                goto failed;
            else if (m_cursor[5] == 1)
                m_step = 9;
        } else if (st == 2) {
            m_result = st;
            m_step = 0x32;
        } else {
            m_step = 0x2d;
        }
        break;
    case 9:
        setMessage(0xe);
        setChoice(10);
        if (requestCmd(0, 0xc) != 1)
            break;
        m_step = 10;
        break;
    case 10:
        if (isCmdDone() != 1)
            break;
        if (getSlotState(m_slot) == 1) {
            m_step = 0xb;
            setMessage(0xf);
            setChoice(10);
            m_wait = 0x3c;
        } else {
            m_step = 0x2f;
        }
        break;
    case 0xb:
        if (requestCmd(0, 1) != 1)
            break;
        m_step = 0xc;
        /* fall through */
    case 0xc:
        if (isCmdDone() == 1) {
            if (getSlotState(m_slot) == 1) {
                if (m_wait < 1) {
                    m_step = 0xe;
                    setMessage(0xb);
                } else {
                    m_step = 0xb;
                }
            } else {
                m_step = 0x2d;
            }
        }
        m_wait--;
        break;

    /* create the directory */
    case 0xe:
        if (requestCmd(0, 6) != 1)
            break;
        m_step = 0xf;
        break;
    case 0xf:
        if (isCmdDone() != 1)
            break;
        st = getSlotState(m_slot);
        if (st == 2) {
            m_result = st;
            m_step = 0x32;
        } else if (st == 1) {
            m_step = 0x10;
        } else {
            m_step = 0x2d;
        }
        break;
    case 0x10:
        m_step = 0x11;
        /* fall through */
    case 0x11:
        m_step = 0x1d;
        break;

    /* formatted: look at the directory */
    case 0x12:
        if (requestCmd(0, 4) != 1)
            break;
        m_step = 0x13;
        break;
    case 0x13:
        if (isCmdDone() != 1)
            break;
        switch (getSlotState(m_slot)) {
        case 4:
        case 8:
            goto failed;
        case 5:
            m_step = 0x1d;
            break;
        case 6:
        case 7:
            if (m_mcd.hasSpace() == 1) {
                m_step = 0xe;
                setMessage(0xb);
                break;
            }
            m_step = 0x1d;
            break;
        }
        break;

    /* a save there already: overwrite it? */
    case 0x1d:
        if (m_mcd.findSave(m_dir[0] != '\0' ? m_dir + 0xe : NULL) != 0) {
            m_step = 0x20;
            break;
        }
        m_step = 0x1e;
        break;

    /* write */
    case 0x1e:
        mcdCopyStr(m_mcd.m_path, m_dir);
        mcdCopyStr(m_mcd.m_name, m_path);
        m_mcd.m_buf = m_buf;
        m_mcd.m_size = m_size;
        setMessage(0xb);
        setChoice(10);
        m_step = 0x1f;
        /* fall through */
    case 0x1f:
        if (requestCmd(0, 8) != 1)
            break;
        m_step = 0x23;
        break;
    case 0x20:
        setMessage(0x12);
        m_cursor[5] = 0;
        m_step = 0x21;
        /* fall through */
    case 0x21:
        m_decided = 0;
        if (requestCmd(0, 1) != 1)
            break;
        m_step = 0x22;
        /* fall through */
    case 0x22:
        if (MCD_PAD_CROSS(pad)) {
            m_decided = 1;
            g_sndMan.playSe(4);
        } else if (m_decided == 0) {
            if ((pad->m_70 & MCD_PAD_LEFT) && m_cursor[5] != 0) {
                m_cursor[5] = 0;
                g_sndMan.playSe(1);
            }
            if ((pad->m_70 & MCD_PAD_RIGHT) && m_cursor[5] != 1) {
                m_cursor[5] = 1;
                g_sndMan.playSe(1);
            }
        }
        if (m_cursor[5] == 0)
            setChoice(8);
        else if (m_cursor[5] == 1)
            setChoice(9);
        if (isCmdDone() != 1)
            break;
        st = getSlotState(m_slot);
        if (st == 3 || st == 1) {
            if (m_decided == 0)
                m_step = 0x21;
            else if (m_cursor[5] == 0)
                goto failed;
            else if (m_cursor[5] == 1)
                m_step = 0x1e;
            break;
        }
        goto failed;
    case 0x23:
        if (isCmdDone() != 1)
            break;
        m_step = getSlotState(m_slot) == 1 ? 0x28 : 0x2d;
        break;

    /* saved */
    case 0x28:
        setMessage(0x13);
        m_step = 0x29;
        m_wait = 0x3c;
        /* fall through */
    case 0x29:
        if (m_wait-- < 1)
            m_step = 0x2b;
        break;
    case 0x2b:
        setMessage(0);
        m_result = 1;
        m_step = 0x32;
        break;

    /* errors: wait for cross */
    case 0x2d:
        setMessage(0x15);
        m_cursor[5] = 0;
        m_decided = 0;
        m_step = 0x2e;
        goto error;
    case 0x2f:
        setMessage(0x16);
        m_cursor[5] = 0;
        m_decided = 0;
        m_step = 0x30;
        /* fall through */
    case 0x2e:
    case 0x30:
    error:
        if (MCD_PAD_CROSS(pad)) {
            m_decided = 1;
            g_sndMan.playSe(4);
        }
        if (m_decided == 0)
            break;
    failed:
        m_result = 2;
        m_step = 0x32;
        break;
    case 0x32:
        m_step = 0x33;
        /* fall through */
    case 0x33:
        m_step = 0;
        m_flags = 2;
        ret = 1;
        break;
    }
    m_blink++;
    return ret;
}

// ROR 0x001a44fc; callcheck: strcpy (inside mcdCopyStr, the copy the ROM inlines)
int MnuMcd::execLoad(BedPad *pad)
{
    int ret = 0;
    int st;

    switch (m_step) {
    case 0:
        m_flags &= ~3u;
        m_step = 1;
        setMessage(0xc);
        m_cursor[6] = 2;
        /* fall through */
    case 1:
        if (requestCmd(0, 1) != 1)
            break;
        m_step = 2;
        break;
    case 2:
        if (isCmdDone() != 1)
            break;
        st = getSlotState(m_slot);
        if (st == 1 || st == 3) {
            if (m_mcd.m_result == 1) {
                m_step = 0x1d;
                break;
            }
            if (m_mcd.m_result != 0x15)
                break;
        }
        m_result = 3;
        m_step = 0x2d;
        break;

    /* read */
    case 0x1d:
        mcdCopyStr(m_mcd.m_path, m_path);
        m_mcd.m_buf = m_buf;
        m_mcd.m_size = m_size;
        if (requestCmd(0, 10) != 1)
            break;
        m_step = 0x23;
        break;
    case 0x23:
        if (isCmdDone() != 1)
            break;
        if (getSlotState(m_slot) == 1) {
            if (testChecksum((u_char *)m_buf) == 1) {
                m_step = 0x28;
                break;
            }
            m_result = 4;
            m_step = 0x2b;
        } else {
            m_result = 3;
            m_step = 0x2d;
        }
        break;

    /* loaded */
    case 0x28:
        setMessage(0x14);
        m_step = 0x29;
        m_wait = 0x3c;
        /* fall through */
    case 0x29:
        if (m_wait-- < 1)
            m_step = 0x2a;
        break;
    case 0x2a:
        setMessage(0);
        m_result = 1;
        m_step = 0x32;
        break;

    /* errors: wait for cross */
    case 0x2b:
        setMessage(0x10);
        m_step = 0x2c;
        goto error;
    case 0x2d:
        setMessage(0x11);
        m_step = 0x2e;
        /* fall through */
    case 0x2c:
    case 0x2e:
    error:
        if (MCD_PAD_CROSS(pad)) {
            m_step = 0x32;
            g_sndMan.playSe(4);
        }
        break;
    case 0x32:
        m_step = 0x33;
        /* fall through */
    case 0x33:
        m_step = 0;
        m_flags = 2;
        ret = 1;
        break;
    }
    m_blink++;
    return ret;
}

// ROR 0x001a47f0
void MnuMcd::updateMessage()
{
    MagScene *scene;
    int no;

    if (m_win[0] < 0)
        return;
    if (m_win[1] < 0)
        return;
    if (m_flags & 1)
        return;
    scene = &m_system->m_scene;
    if ((m_flags & 2) == 0) {
        scene->setWindowShow(m_win[0], 0);
        scene->setWindowShow(m_win[1], 0);
        m_shown = 1;
    } else {
        scene->setWindowShow(m_win[0], 1);
        scene->setWindowShow(m_win[1], 1);
        m_msg.close();
        m_shown = 0;
    }
    if (m_flags & 4) {
        m_flags &= ~4u;
        scene->updateWindowFont(m_win[0]);
        scene->updateWindowFont(m_win[1]);
        m_msg.close();
    }
    if (m_flags & 8)
        m_blink = 0;

    /* the message window */
    if (m_flags & 0x10) {
        m_flags &= ~0x10u;
        if (m_msg.getState(m_win[0]) != 0)
            m_msg.close();
        m_msg.open();
        no = 0;
        switch (m_msg_no) {
        case 1:
        case 2:
        case 3:
            if (m_text_mode == 0)
                no = 0x1d;
            else if (m_text_mode == 1)
                no = 0x1e;
            else if (m_text_mode == 2)
                no = 0x1f;
            break;
        case 4:
        case 5:
        case 6:    no = 0x20; break;
        case 7:    no = 0x21; break;
        case 8:
        case 9:
        case 10:   no = 0x23; break;
        case 0xb:  no = 0x24; break;
        case 0xc:  no = 0x25; break;
        case 0xd:  no = 0x26; break;
        case 0xe:  no = 0x27; break;
        case 0xf:  no = 0x28; break;
        case 0x10: no = 0x29; break;
        case 0x11: no = 0x2a; break;
        case 0x12: no = 0x2d; break;
        case 0x13: no = 0x2e; break;
        case 0x14: no = 0x2f; break;
        case 0x15: no = 0x2b; break;
        case 0x16: no = 0x2c; break;
        case 0x17: no = 0x22; break;
        }
        if (no != 0)
            m_msg.setMessage(m_win[0], msgGetSysText(no), 0);
    }

    /* the choice window: texts 0x30..0x39 */
    if (m_flags & 0x20) {
        m_flags &= ~0x20u;
        if (m_msg.getState(m_win[1]) != 0)
            m_msg.close();
        m_msg.open();
        switch (m_msg_no) {
        case 1: case 2: case 3: case 4: case 5: case 6: case 7:
        case 0xd: case 0x12: case 0x17:
            if (m_choice >= 0 && m_choice <= 9)
                m_msg.setMessage(m_win[1], msgGetSysText(0x30 + m_choice), 0);
            break;
        }
    }

    if (++m_blink > 0x30)
        m_blink = 0;
    if (m_msg.getState(m_win[0]) != 0 || m_msg.getState(m_win[1]) != 0)
        m_msg.update();
}
