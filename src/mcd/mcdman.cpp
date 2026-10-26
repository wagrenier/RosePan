/* ==========================================================================
 *  mcd/mcdman.cpp
 *
 *  The memory card manager (0x187724..0x18bb8c; RoR's own file string at
 *  0x187a28). See mcd/mcdman.h for the command / step machine. The libmc
 *  calls go to src/sdk/libmc.cpp, a directory per card on the host.
 *
 *  PORT: every libmc command repeats the same issue / sync bookkeeping in
 *  the ROM; here the shared tails are the helpers at the top (issued,
 *  synced, retryNoFormat, setDone, next), each command keeping its own
 *  result-code map in the ROM's order.
 * ======================================================================== */
#include "mcd/mcdman.h"

#include <string.h>

#include "bed/bedpfh.h"
#include "common/ror_assert.h"
#include "game/gamfile.h"
#include "utl/utlmisc.h"

/* ---- data ---- */

static const char s_file[] = "mcd/mcdman.cpp";

/* init's result names (rodata 0x2f7608..), by McdResult. */
static const char *const s_err_name[MCD_ERR_NUM] = {
    NULL,
    "ERR_NoErr", "ERR_CmdErr", "ERR_Capacity", "ERR_NoInit", "ERR_NotPathName",
    "ERR_Semapho", "ERR_NoFormat", "ERR_DeniedPermit", "ERR_NoMemoryCard",
    "ERR_NoFunction", "ERR_NotFd", "ERR_NotFileName", "ERR_NotOpen",
    "ERR_NotMemCardAccess", "ERR_NotEmpty", "ERR_FullDevice", "ERR_ExistEntry",
    "ERR_UpLimitHandle", "ERR_NotRead", "ERR_FailReplace", "ERR_ChangedCard",
};

/* data 0x2998c0..0x29993c: the save's paths and the names checkFile looks
 * for in the directory ('?' matches writeFile's marked names). */
static const char *const s_icon_path[] = { // 0x002998c0
    "/BASLUS-21448/slime1.ico", "/BASLUS-21448/slime2.ico", "/BASLUS-21448/slime3.ico", NULL,
};
static const char *const s_icon_name[] = { // 0x002998d0
    "slime1.ico", "slime2.ico", "slime3.ico", NULL,
};
static const int s_icon_file_size[] = { 0x11350, 0x11350, 0x11350, -1 }; // 0x002998e0
static const char *const s_data_name[] = { // 0x002998f0
    "dt00????????", "dt01????????", "dt02????????", "dt03????????",
    "dt04????????", "dt05????????", "dt06????????", "dt07????????",
    "dt08????????", "dt09????????", "dt10????????", "dt11????????", NULL, NULL,
};
static const char *const s_dummy_icon_path[] = { // 0x00299928
    "/FEBDUMMY/slime1.ico", "/FEBDUMMY/slime2.ico", "/FEBDUMMY/slime3.ico", NULL,
};
static const char *s_dummy_path = "/FEBDUMMY/FEBDUMMY"; // 0x00299938
static const char *s_icon_pack = "data/mnurpk/mcdicon.rpk"; // 0x0029993c

static const char s_save_dir[] = "/BASLUS-21448";
static const char s_save_icon_sys[] = "/BASLUS-21448/icon.sys";
static const char s_save_main[] = "/BASLUS-21448/BASLUS-21448";
static const char s_save_main_name[] = "BASLUS-21448";
static const char s_data_dir[] = "/dt";
static const char s_data_tail[] = "________";
static const char s_icon_sys_name[] = "icon.sys";
static const char s_dummy_dir[] = "/FEBDUMMY";
static const char s_dummy_icon_sys[] = "/FEBDUMMY/icon.sys";

/* icon.sys's title: "Rule of Rose" in full-width Shift-JIS. */
static const char s_icon_title[] =
    "\x82\x71\x82\x95\x82\x8c\x82\x85\x81\x40\x82\x8f\x82\x86\x81\x40"
    "\x82\x71\x82\x8f\x82\x93\x82\x85";

/* The size of the save the card needs room for (KB), with what the save
 * directory already holds counted in. */
#define MCD_SAVE_KB 0xed

/* ---- PC helpers: the libmc commands' shared steps ---- */

/* The issue step's tail: the call's own return value says whether there is
 * a request to sync (step 4) or the command failed. named: the call takes a
 * path (-210, a bad name, is a result of its own). */
void McdMan::issued(int ret, int named)
{
    m_status = MCD_STATUS_ERROR;
    m_ret = ret;
    if (named && ret == sceMcErrEntryName)
        m_result = MCD_ERR_NOTPATHNAME;
    else if (ret == sceMcErrNotInit)
        m_result = MCD_ERR_NOINIT;
    else if (ret != 0)
        m_result = MCD_ERR_CMDERR;
    else {
        m_step = MCD_STEP_SYNC;
        m_result = MCD_ERR_NOERR;
        m_status = MCD_STATUS_BUSY;
    }
    m_retry++;
}

/* The sync step: 1 when the request finished (m_sync_result holds its
 * result, m_status is ERROR until the command's map says otherwise), 0 while
 * it runs or when the library had nothing running (the command ends). */
int McdMan::synced()
{
    int r = sceMcSync(m_sync_mode, &m_sync_cmd, &m_sync_result);
    if (r == sceMcExecIdle) {
        m_status = MCD_STATUS_ERROR;
        m_result = MCD_ERR_NOFUNCTION;
        m_step = MCD_STEP_END;
        return 0;
    }
    if (r != sceMcExecFinish)
        return 0;
    m_status = MCD_STATUS_ERROR;
    return 1;
}

/* sceMcResNoFormat: issue again, up to 8 times. */
void McdMan::retryNoFormat()
{
    m_result = MCD_ERR_NOFORMAT;
    if (m_retry > 7)
        return;
    m_step = MCD_STEP_ISSUE;
    m_result = MCD_ERR_NOERR;
    m_status = MCD_STATUS_BUSY;
}

void McdMan::setDone()
{
    m_status = MCD_STATUS_DONE;
    m_step = MCD_STEP_END;
}

/* A composite command's next sub step, with a fresh libmc command. */
void McdMan::next(int sub)
{
    m_status = MCD_STATUS_BUSY;
    m_sub = sub;
    m_step = MCD_STEP_START;
}

/* The open mode for a file checkFile found (2: there, other size) or not (0). */
void McdMan::setOpenMode(int found)
{
    if (found < 2) {
        if (found == 0)
            m_open_mode = sceMcFileCreateFile | sceMcFileAttrReadable | sceMcFileAttrWriteable;
    } else if (found == 2) {
        m_open_mode = sceMcFileAttrReadable | sceMcFileAttrWriteable;
    }
}

/* ---- McdMan ---- */

// ROR 0x00187724
McdMan::McdMan()
{
    init();
}

// ROR 0x00187738 // callcheck: operator_delete (the deleting variant: implicit on the host)
McdMan::~McdMan()
{
    term();
}

// ROR 0x00187788
void McdMan::init()
{
    int i;

    for (i = 0; i < MCD_CMD_NUM; i++)
        m_cmd_word[i] = i + 1;
    m_result = MCD_ERR_NOERR;
    m_cmd = MCD_CMD_NONE;
    m_status = MCD_STATUS_IDLE;
    m_sync_mode = 1;
    m_port = 0;
    m_slot = 0;
    m_type = 0;
    m_free = 0;
    m_format = 0;
    m_dir_mode = 0;
    m_path[0] = 0;
    m_name[0] = 0;
    for (i = MCD_ERR_NOERR; i < MCD_ERR_NUM; i++)
        m_err_name[i] = s_err_name[i];
    m_icon_size[0] = 0;
    m_icon[0] = NULL;
    m_pack = NULL;
}

// ROR 0x001879c4
void McdMan::term()
{
    releaseData();
}

// ROR 0x001879e0
int McdMan::freeIconPack()
{
    if (m_pack) {
        utlMemFree(m_pack);
        m_icon_size[0] = 0;
        m_pack = NULL;
        m_pack_size = 0;
        m_icon[0] = NULL;
    }
    return 0;
}

// ROR 0x00187a28 // callcheck: FileReadReq, FileReader, ~FileReader (the locals' constructors and destructor)
int McdMan::loadIconPack()
{
    FileReader reader;

    if (m_pack) {
        utlMemFree(m_pack);
        m_pack = NULL;
        m_pack_size = 0;
    }
    m_pack_size = fileGetSize(s_icon_pack);
    m_pack = g_utlMemory.allocDebug(m_pack_size, s_file, 281, 0, 2);

    FileReadReq req;
    req.m_buf = m_pack;
    req.m_size = m_pack_size;
    req.m_name = s_icon_pack;
    req.m_close = 1;
    req.m_08 = 1;
    req.m_whence = 0;
    req.m_offset = 0;
    reader.read(&req, 0);
    return 0;
}

// ROR 0x00187b10 // callcheck: FUN_00215a30, BedPfh (the assert handler inside getFile; the local's constructor)
int McdMan::loadData(int no)
{
    BedPfh pfh;

    pfh.clear();
    if (!m_pack)
        loadIconPack();
    pfh = BedPfh(m_pack);
    int n = pfh.getFileIndex(s_icon_name[no]);
    if (n >= 0) {
        m_icon[no] = pfh.getFile(n);
        m_icon_size[no] = pfh.getFileSize(n);
    }
    return 0;
}

// ROR 0x00187c30
void McdMan::releaseData()
{
    freeIconPack();
}

// ROR 0x00187c4c
void McdMan::update()
{
    int r;

    switch (m_cmd) {
    case MCD_CMD_NONE:        return;
    case MCD_CMD_GETINFO:     r = cmdGetInfo(); break;
    case MCD_CMD_GETSLOTMAX:  r = cmdGetSlotMax(); break;
    case MCD_CMD_GETENTSPACE: r = cmdGetEntSpace(); break;
    case MCD_CMD_CHDIR:       r = cmdChdir(); break;
    case MCD_CMD_CLOSE:       r = cmdClose(); break;
    case MCD_CMD_DELETE:      r = cmdDelete(); break;
    case MCD_CMD_DELETENOW:   r = cmdDeleteNow(); break;
    case MCD_CMD_FLUSH:       r = cmdFlush(); break;
    case MCD_CMD_FORMAT:      r = cmdFormat(); break;
    case MCD_CMD_GETDIR:      r = cmdGetDir(); break;
    case MCD_CMD_MKDIR:       r = cmdMkdir(); break;
    case MCD_CMD_OPEN:        r = cmdOpen(); break;
    case MCD_CMD_READ:        r = cmdRead(); break;
    case MCD_CMD_RENAME:      r = cmdRename(); break;
    case MCD_CMD_SEEK:        r = cmdSeek(); break;
    case MCD_CMD_UNFORMAT:    r = cmdUnformat(); break;
    case MCD_CMD_WRITE:       r = cmdWrite(); break;
    case MCD_CMD_CREATESAVE:  r = cmdCreateSave(); break;
    case MCD_CMD_READDIR:     r = cmdReadDir(); break;
    case MCD_CMD_WRITEFILE:   r = cmdWriteFile(); break;
    case MCD_CMD_READFILE:    r = cmdReadFile(); break;
    case MCD_CMD_FILLDUMMY:   r = cmdFillDummy(); break;
    default:                  return;
    }
    if (r == 1) {
        m_step = MCD_STEP_START;
        m_cmd = MCD_CMD_NONE;
    }
}

// ROR 0x00187e00
int McdMan::getStatus()
{
    return m_status;
}

// ROR 0x00187e08
int McdMan::request(int n)
{
    if (m_cmd != MCD_CMD_NONE)
        return 6;
    m_sub = 0;
    m_cmd = m_cmd_word[n];
    m_status = MCD_STATUS_BUSY;
    return 1;
}

/* ---- the libmc commands ---- */

/* Unlike the others its sync answer leaves m_status busy until decided,
 * reports errors as done, and a PS1 or PDA card as not accessible. */
// ROR 0x00187e44
int McdMan::cmdGetInfo()
{
    int end = 0;
    int r;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        issued(sceMcGetInfo(m_port, m_slot, &m_type, &m_free, &m_format), 1);
        break;
    case MCD_STEP_SYNC:
        r = sceMcSync(m_sync_mode, &m_sync_cmd, &m_sync_result);
        if (r == sceMcExecIdle) {
            m_status = MCD_STATUS_ERROR;
            m_step = MCD_STEP_END;
            m_result = MCD_ERR_NOFUNCTION;
            m_file_num = 0;
            break;
        }
        if (r != sceMcExecFinish)
            break;
        switch (m_sync_result) {
        case sceMcResSucceed:
            if (m_format) {
                m_status = MCD_STATUS_DONE;
                m_step = MCD_STEP_END;
                m_result = MCD_ERR_NOERR;
            } else if (m_retry > 7) {
                goto fail_nofmt;
            } else {
                m_step = MCD_STEP_ISSUE;
            }
            break;
        case sceMcResNoFormat:
            if (m_retry < 8) {
                m_step = MCD_STEP_ISSUE;
                break;
            }
        fail_nofmt:
            m_result = MCD_ERR_NOFORMAT;
            goto fail;
        case sceMcResChangedCard:
            m_result = MCD_ERR_CHANGEDCARD;
            goto fail;
        case sceMcResDeniedPermit:
            m_result = MCD_ERR_DENIEDPERMIT;
            goto fail;
        default:
            m_result = MCD_ERR_NOTMEMCARDACCESS;
        fail:
            m_status = MCD_STATUS_DONE;
            m_step = MCD_STEP_END;
            m_file_num = 0;
            break;
        }
        if (m_type == sceMcTypePS1 || m_type == sceMcTypePDA)
            m_result = MCD_ERR_NOTMEMCARDACCESS;
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

/* sceMcGetSlotMax answers at once: no sync step. */
// ROR 0x001880cc
int McdMan::cmdGetSlotMax()
{
    int end = 0;
    int r;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        r = sceMcGetSlotMax(m_port);
        m_status = MCD_STATUS_ERROR;
        m_ret = r;
        if (r < 0) {
            if (r == sceMcErrNotInit)
                m_result = MCD_ERR_NOINIT;
            else if (r == sceMcErrBusy)
                m_result = MCD_ERR_SEMAPHO;
            else
                m_result = MCD_ERR_CMDERR;
            m_step = MCD_STEP_END;
        } else {
            m_result = MCD_ERR_NOERR;
            m_status = MCD_STATUS_DONE;
            m_slot_max = r;
            m_step = MCD_STEP_END;
        }
        m_retry++;
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

// ROR 0x001881c0 // callcheck: sceMcSync (inside synced)
int McdMan::cmdGetEntSpace()
{
    int end = 0;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        issued(sceMcGetEntSpace(m_port, m_slot, m_path), 1);
        break;
    case MCD_STEP_SYNC:
        if (!synced())
            break;
        if (m_sync_result >= 0) {
            m_ent_space = m_sync_result;
            setDone();
        } else if (m_sync_result == sceMcResNoFormat) {
            retryNoFormat();
        } else if (m_sync_result == sceMcResDeniedPermit) {
            m_result = MCD_ERR_DENIEDPERMIT;
        } else if (m_sync_result <= -10) {
            m_result = MCD_ERR_NOMEMORYCARD;
        }
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

// ROR 0x001883c0 // callcheck: sceMcSync (inside synced)
int McdMan::cmdChdir()
{
    int end = 0;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        issued(sceMcChdir(m_port, m_slot, m_path, m_name), 1);
        break;
    case MCD_STEP_SYNC:
        if (!synced())
            break;
        switch (m_sync_result) {
        case sceMcResSucceed:      setDone(); break;
        case sceMcResNoFormat:     retryNoFormat(); break;
        case sceMcResNoEntry:      m_result = MCD_ERR_NOTPATHNAME; break;
        case sceMcResDeniedPermit: m_result = MCD_ERR_DENIEDPERMIT; break;
        default:
            if (m_sync_result <= -10)
                m_result = MCD_ERR_NOMEMORYCARD;
            break;
        }
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

// ROR 0x001885d4 // callcheck: sceMcSync (inside synced)
int McdMan::cmdClose()
{
    int end = 0;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        issued(sceMcClose(m_fd), 0);
        break;
    case MCD_STEP_SYNC:
        if (!synced())
            break;
        switch (m_sync_result) {
        case sceMcResSucceed:      setDone(); break;
        case sceMcResNoFormat:     retryNoFormat(); break;
        case sceMcResNoEntry:      m_result = MCD_ERR_NOTFD; break;
        case sceMcResDeniedPermit: m_result = MCD_ERR_DENIEDPERMIT; break;
        default:
            if (m_sync_result <= -10)
                m_result = MCD_ERR_NOMEMORYCARD;
            break;
        }
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

// ROR 0x001887c0
int McdMan::findNextMatch(int i, const char *path, char *out)
{
    char pat[0x40];
    const char *slash;
    char *star;
    int j;

    if (i >= m_file_num)
        return -1;
    slash = strrchr(path, '/');
    if (slash)
        strcpy(pat, slash + 1);
    else
        strcpy(pat, path);
    star = strchr(pat, '*');
    if (!star)
        return -1;
    *star = 0;
    for (j = i; j < m_file_num; j++) {
        if (strstr(getDirName(j), pat)) {
            if (slash) {
                strcpy(pat, path);
                strcpy(strrchr(pat, '/') + 1, getDirName(j));
                strcpy(out, pat);
            } else {
                strcpy(out, getDirName(j));
            }
            return j + 1;
        }
    }
    /* No match: the ROM answers the index after the first one asked. */
    return i + 1;
}

/* m_path, or with a '*' each m_dir entry it matches, one sceMcDelete per
 * match (step 1 picks the next). */
// ROR 0x0018894c // callcheck: sceMcSync (inside synced)
int McdMan::cmdDelete()
{
    int end = 0;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_NEXT;
        m_del_index = -1;
        if (strchr(m_path, '*') && m_file_num > 0)
            m_del_index = 0;
        /* fall through */
    case MCD_STEP_NEXT:
        if (m_del_index == -1) {
            strcpy(m_work, m_path);
        } else {
            m_del_index = findNextMatch(m_del_index, m_path, m_work);
            if (m_del_index < 0) {
                m_status = MCD_STATUS_DONE;
                m_step = MCD_STEP_END;
                m_sync_result = 0;
                break;
            }
        }
        m_retry = 0;
        m_step = MCD_STEP_ISSUE;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        issued(sceMcDelete(m_port, m_slot, m_work), 1);
        break;
    case MCD_STEP_SYNC:
        if (!synced())
            break;
        switch (m_sync_result) {
        case sceMcResSucceed:
            m_step = MCD_STEP_END;
            m_status = m_del_index == -1 ? MCD_STATUS_DONE : MCD_STATUS_BUSY;
            break;
        case sceMcResNoFormat:     retryNoFormat(); break;
        case sceMcResDeniedPermit: m_result = MCD_ERR_DENIEDPERMIT; break;
        case sceMcResNoEntry:      m_result = MCD_ERR_NOTFILENAME; break;
        default:
            if (m_sync_result <= -10)
                m_result = MCD_ERR_NOMEMORYCARD;
            break;
        }
        break;
    case MCD_STEP_END:
        if (m_del_index == -1)
            end = 1;
        else
            m_step = MCD_STEP_NEXT;
        break;
    }
    return stepEnd(end);
}

/* The ROM takes sceMcDelete's return value 1 as success and never syncs:
 * a real call answers 0 when accepted, so this command always reports
 * MCD_ERR_CMDERR (nothing calls it). */
// ROR 0x00188bfc
int McdMan::cmdDeleteNow()
{
    int end = 0;
    int r;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        r = sceMcDelete(m_port, m_slot, m_path);
        m_status = MCD_STATUS_ERROR;
        m_ret = r;
        if (r == 1) {
            m_result = MCD_ERR_NOERR;
            setDone();
        } else {
            m_result = MCD_ERR_CMDERR;
        }
        m_retry++;
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

// ROR 0x00188cd4 // callcheck: sceMcSync (inside synced)
int McdMan::cmdFlush()
{
    int end = 0;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        issued(sceMcFlush(m_fd), 0);
        break;
    case MCD_STEP_SYNC:
        if (!synced())
            break;
        switch (m_sync_result) {
        case sceMcResSucceed:      setDone(); break;
        case sceMcResNoFormat:     retryNoFormat(); break;
        case sceMcResNoEntry:      m_result = MCD_ERR_NOTOPEN; break;
        case sceMcResDeniedPermit: m_result = MCD_ERR_NOTMEMCARDACCESS; break;
        default:
            if (m_sync_result <= -10)
                m_result = MCD_ERR_NOMEMORYCARD;
            break;
        }
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

// ROR 0x00188ec0 // callcheck: sceMcSync (inside synced)
int McdMan::cmdFormat()
{
    int end = 0;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        issued(sceMcFormat(m_port, m_slot), 0);
        break;
    case MCD_STEP_SYNC:
        if (!synced())
            break;
        if (m_sync_result == sceMcResSucceed)
            setDone();
        else if (m_sync_result == sceMcResDeniedPermit)
            m_result = MCD_ERR_NOTMEMCARDACCESS;
        else if (m_sync_result <= -10)
            m_result = MCD_ERR_NOMEMORYCARD;
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

// ROR 0x00189068 // callcheck: sceMcSync (inside synced)
int McdMan::cmdGetDir()
{
    int end = 0;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_file_num = 0;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_file_num = 0;
        m_step = MCD_STEP_END;
        issued(sceMcGetDir(m_port, m_slot, m_path, m_dir_mode, MCD_DIR_MAX,
                           (sceMcTblGetDir *)m_dir), 1);
        break;
    case MCD_STEP_SYNC:
        if (!synced())
            break;
        if (m_sync_result > 0) {
            setDone();
            m_file_num = m_sync_result;
            break;
        }
        switch (m_sync_result) {
        case 0:
        case sceMcResNoEntry:      m_result = MCD_ERR_NOTPATHNAME; break;
        case sceMcResNoFormat:     retryNoFormat(); break;
        case sceMcResDeniedPermit: m_result = MCD_ERR_DENIEDPERMIT; break;
        case sceMcResNotEmpty:     m_result = MCD_ERR_NOTEMPTY; break;
        default:
            if (m_sync_result <= -10)
                m_result = MCD_ERR_NOMEMORYCARD;
            break;
        }
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

// ROR 0x001892b0 // callcheck: sceMcSync (inside synced)
int McdMan::cmdMkdir()
{
    int end = 0;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        issued(sceMcMkdir(m_port, m_slot, m_path), 1);
        break;
    case MCD_STEP_SYNC:
        if (!synced())
            break;
        switch (m_sync_result) {
        case sceMcResSucceed:      setDone(); break;
        case sceMcResNoFormat:     retryNoFormat(); break;
        case sceMcResFullDevice:   m_result = MCD_ERR_FULLDEVICE; break;
        case sceMcResNoEntry:      m_result = MCD_ERR_EXISTENTRY; break;
        case sceMcResDeniedPermit: m_result = MCD_ERR_DENIEDPERMIT; break;
        default:
            if (m_sync_result <= -10)
                m_result = MCD_ERR_NOMEMORYCARD;
            break;
        }
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

// ROR 0x001894d4 // callcheck: sceMcSync (inside synced)
int McdMan::cmdOpen()
{
    int end = 0;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        issued(sceMcOpen(m_port, m_slot, m_path, m_open_mode), 1);
        break;
    case MCD_STEP_SYNC:
        if (!synced())
            break;
        if (m_sync_result >= 0) {
            setDone();
            m_fd = m_sync_result;
            break;
        }
        switch (m_sync_result) {
        case sceMcResNoFormat:      retryNoFormat(); break;
        case sceMcResFullDevice:    m_result = MCD_ERR_FULLDEVICE; break;
        case sceMcResNoEntry:       m_result = MCD_ERR_NOTPATHNAME; break;
        case sceMcResDeniedPermit:  m_result = MCD_ERR_DENIEDPERMIT; break;
        case sceMcResUpLimitHandle: m_result = MCD_ERR_UPLIMITHANDLE; break;
        default:
            if (m_sync_result <= -10)
                m_result = MCD_ERR_NOMEMORYCARD;
            break;
        }
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

// ROR 0x00189718 // callcheck: sceMcSync (inside synced)
int McdMan::cmdRead()
{
    int end = 0;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        issued(sceMcRead(m_fd, m_buf, m_size), 0);
        break;
    case MCD_STEP_SYNC:
        if (!synced())
            break;
        if (m_sync_result >= 0) {
            setDone();
            m_rw_size = m_sync_result;
            break;
        }
        switch (m_sync_result) {
        case sceMcResNoFormat:      retryNoFormat(); break;
        case sceMcResFullDevice:    m_result = MCD_ERR_NOTREAD; break;
        case sceMcResNoEntry:       m_result = MCD_ERR_NOTOPEN; break;
        case sceMcResDeniedPermit:  m_result = MCD_ERR_DENIEDPERMIT; break;
        case sceMcResUpLimitHandle: m_result = MCD_ERR_UPLIMITHANDLE; break;
        default:
            if (m_sync_result <= -10)
                m_result = MCD_ERR_NOMEMORYCARD;
            break;
        }
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

/* m_path to the file name of m_name (sceMcRename takes a bare name). */
// ROR 0x0018993c // callcheck: sceMcSync (inside synced)
int McdMan::cmdRename()
{
    int end = 0;
    const char *slash;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        slash = strrchr(m_name, '/');
        if (slash)
            strcpy(m_work, slash + 1);
        else
            strcpy(m_work, m_name);
        issued(sceMcRename(m_port, m_slot, m_path, m_work), 1);
        break;
    case MCD_STEP_SYNC:
        if (!synced())
            break;
        switch (m_sync_result) {
        case sceMcResSucceed:      setDone(); break;
        case sceMcResNoFormat:     retryNoFormat(); break;
        case sceMcResNoEntry:      m_result = MCD_ERR_NOTFILENAME; break;
        case sceMcResDeniedPermit: m_result = MCD_ERR_NOTMEMCARDACCESS; break;
        default:
            if (m_sync_result <= -10)
                m_result = MCD_ERR_NOMEMORYCARD;
            break;
        }
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

// ROR 0x00189b7c // callcheck: sceMcSync (inside synced)
int McdMan::cmdSeek()
{
    int end = 0;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        issued(sceMcSeek(m_fd, m_seek_offset, m_seek_mode), 1);
        break;
    case MCD_STEP_SYNC:
        if (!synced())
            break;
        if (m_sync_result >= 0) {
            setDone();
            m_seek_pos = m_sync_result;
            break;
        }
        switch (m_sync_result) {
        case sceMcResNoFormat:     retryNoFormat(); break;
        case sceMcResNoEntry:      m_result = MCD_ERR_NOTOPEN; break;
        case sceMcResDeniedPermit: m_result = MCD_ERR_NOTMEMCARDACCESS; break;
        default:
            if (m_sync_result <= -10)
                m_result = MCD_ERR_NOMEMORYCARD;
            break;
        }
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

// ROR 0x00189d94 // callcheck: sceMcSync (inside synced)
int McdMan::cmdUnformat()
{
    int end = 0;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        issued(sceMcUnformat(m_port, m_slot), 1);
        break;
    case MCD_STEP_SYNC:
        if (!synced())
            break;
        if (m_sync_result == sceMcResSucceed)
            setDone();
        else if (m_sync_result == sceMcResDeniedPermit)
            m_result = MCD_ERR_NOTMEMCARDACCESS;
        else if (m_sync_result <= -10)
            m_result = MCD_ERR_NOMEMORYCARD;
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

// ROR 0x00189f58 // callcheck: sceMcSync (inside synced)
int McdMan::cmdWrite()
{
    int end = 0;

    switch (m_step) {
    case MCD_STEP_START:
        m_step = MCD_STEP_ISSUE;
        m_retry = 0;
        /* fall through */
    case MCD_STEP_ISSUE:
        m_step = MCD_STEP_END;
        issued(sceMcWrite(m_fd, m_buf, m_size), 0);
        break;
    case MCD_STEP_SYNC:
        if (!synced())
            break;
        if (m_sync_result >= 0) {
            setDone();
            m_rw_size = m_sync_result;
            break;
        }
        switch (m_sync_result) {
        case sceMcResNoFormat:     retryNoFormat(); break;
        case sceMcResFullDevice:   m_result = MCD_ERR_NOTREAD; break;
        case sceMcResNoEntry:      m_result = MCD_ERR_NOTOPEN; break;
        case sceMcResDeniedPermit: m_result = MCD_ERR_DENIEDPERMIT; break;
        case sceMcResFailReplace:  m_result = MCD_ERR_FAILREPLACE; break;
        default:
            if (m_sync_result <= -10)
                m_result = MCD_ERR_NOMEMORYCARD;
            break;
        }
        break;
    case MCD_STEP_END:
        end = 1;
        break;
    }
    return stepEnd(end);
}

/* ---- the save ---- */

// ROR 0x0018a17c
void McdMan::makeIconSys()
{
    _iconVu0IVECTOR bg_color[4];
    /* The light constants' bits from the ROM (not all round-to-nearest). */
    static const _iconVu0FVECTOR light_dir[3] = {
        { 0.5f, 0.5f, 0.5f, 0.0f },
        { 0.0f, -0x1.999998p-2f, -0x1.999998p-4f, 0.0f },
        { -0.5f, -0.5f, 0.5f, 0.0f },
    };
    static const _iconVu0FVECTOR light_color[3] = {
        { 0.48f, 0.48f, 0.03f, 0.0f },
        { 0.5f, 0x1.51eb84p-2f, 0x1.999998p-3f, 0.0f },
        { 0x1.1eb85p-3f, 0x1.1eb85p-3f, 0.38f, 0.0f },
    };
    static const _iconVu0FVECTOR ambient = { 0.5f, 0.5f, 0.5f, 0.0f }; // 0x00299940

    memset(bg_color, 0, sizeof(bg_color));
    bg_color[0][0] = 0x80;
    bg_color[1][1] = 0x80;
    bg_color[2][2] = 0x80;
    bg_color[3][0] = 0x80;
    bg_color[3][1] = 0x80;
    bg_color[3][2] = 0x80;

    memset(&m_icon_sys, 0, sizeof(m_icon_sys));
    /* strcpy in the ROM: its NUL lands in Reserv1. */
    memcpy(m_icon_sys.Head, "PS2D", 4);
    m_icon_sys.Reserv1 = 0;
    strcpy((char *)m_icon_sys.TitleName, s_icon_title);
    m_icon_sys.OffsLF = 0x20;
    m_icon_sys.TransRate = 0x60;
    memcpy(m_icon_sys.BgColor, bg_color, sizeof(bg_color));
    memcpy(m_icon_sys.LightDir, light_dir, sizeof(light_dir));
    memcpy(m_icon_sys.LightColor, light_color, sizeof(light_color));
    memcpy(m_icon_sys.Ambient, ambient, sizeof(ambient));
    strcpy((char *)m_icon_sys.FnameView, s_icon_name[0]);
    strcpy((char *)m_icon_sys.FnameCopy, s_icon_name[0]);
    strcpy((char *)m_icon_sys.FnameDel, s_icon_name[0]);
}

/* "/BASLUS-21448/dtNN________". */
// ROR 0x0018a3f4 // callcheck: strlen (strcat is strlen + strcpy in the ROM)
void McdMan::makeDataPath(int no, char *path)
{
    char num[3];

    num[0] = no / 10 + '0';
    num[1] = no % 10 + '0';
    num[2] = 0;
    strcpy(path, s_save_dir);
    strcat(path, s_data_dir);
    strcat(path, num);
    strcat(path, s_data_tail);
}

// ROR 0x0018a4d4
void McdMan::freeWork()
{
    if (m_data) {
        utlMemFree(m_data);
        m_data = NULL;
    }
}

/* The save directory: mkdir, the icon(s), icon.sys, the main file and the
 * 12 data files of 0x3000 zero bytes; a file checkFile finds at its size is
 * kept. */
// ROR 0x0018a50c // callcheck: strcpy (inside setPath)
int McdMan::cmdCreateSave()
{
    switch (m_sub) {
    case 0:
        m_sub = 1;
        m_step = MCD_STEP_START;
        m_status = MCD_STATUS_BUSY;
        setPath(s_save_dir);
        /* fall through */
    case 1:
        if (cmdMkdir() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(4);
            return 0;
        }
        if (m_status != MCD_STATUS_ERROR)
            return 0;
        if (m_result != MCD_ERR_EXISTENTRY)
            return 1;
        m_result = MCD_ERR_NOERR;
        m_sub = 4;
        m_step = MCD_STEP_START;
        m_status = MCD_STATUS_BUSY;
        return 0;
    case 4:
        m_index = 0;
        /* fall through */
    case 5:
        m_status = MCD_STATUS_BUSY;
        m_sub = 10;
        m_step = MCD_STEP_START;
        setPath(s_icon_path[m_index]);
        {
            int found = checkFile(s_icon_name[m_index], s_icon_file_size[m_index]);
            if (found == 1) {
                m_sub = 0x32;
                return 0;
            }
            setOpenMode(found);
        }
        return 0;
    case 10:
        if (cmdOpen() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x14);
            return 0;
        }
        return subEnd();
    case 0x14:
        m_sub = 0x1e;
        m_size = m_icon_size[m_index];
        m_buf = m_icon[m_index];
        return 0;
    case 0x1e:
        if (cmdWrite() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x28);
            return 0;
        }
        return subEnd();
    case 0x28:
        if (cmdClose() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x32);
            return 0;
        }
        return subEnd();
    case 0x32:
        if (++m_index >= MCD_ICON_NUM)
            m_sub = 0x6e;
        else
            m_sub = 5;
        return 0;
    case 0x3c:
        m_sub = 0x46;
        m_status = MCD_STATUS_BUSY;
        m_step = MCD_STEP_START;
        makeIconSys();
        setPath(s_save_icon_sys);
        {
            int found = checkFile(s_icon_sys_name, sizeof(sceMcIconSys));
            if (found == 1) {
                m_sub = 999;
                return 0;
            }
            setOpenMode(found);
        }
        return 0;
    case 0x46:
        if (cmdOpen() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x50);
            return 0;
        }
        return subEnd();
    case 0x50:
        m_sub = 0x5a;
        m_buf = &m_icon_sys;
        m_size = sizeof(sceMcIconSys);
        return 0;
    case 0x5a:
        if (cmdWrite() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(100);
            return 0;
        }
        return subEnd();
    case 100:
        if (cmdClose() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(999);
            return 0;
        }
        return subEnd();
    case 0x6e:
        m_sub = 0x78;
        m_status = MCD_STATUS_BUSY;
        m_step = MCD_STEP_START;
        setPath(s_save_main);
        {
            int found = checkFile(s_save_main_name, 0x3000);
            if (found == 1) {
                m_sub = 200;
                return 0;
            }
            setOpenMode(found);
        }
        return 0;
    case 0x78:
        if (cmdOpen() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x82);
            return 0;
        }
        return subEnd();
    case 0x82:
        m_sub = 0x8c;
        m_data = g_utlMemory.allocDebug(0x3000, s_file, 2780, 0, 2);
        if (!m_data) {
            m_sub = 999;
            return 1;
        }
        memset(m_data, 0, 0x3000);
        m_size = 0x3000;
        m_buf = m_data;
        return 0;
    case 0x8c:
        if (cmdWrite() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x96);
            return 0;
        }
        break;
    case 0x96:
        if (cmdClose() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(200);
            return 0;
        }
        break;
    case 200:
        m_sub = 0xd2;
        m_index = 0;
        /* fall through */
    case 0xd2:
        m_sub = 0xdc;
        m_step = MCD_STEP_START;
        m_status = MCD_STATUS_BUSY;
        makeDataPath(m_index, m_path);
        {
            /* PORT: when the main file was already there (sub 200) m_data
             * was never allocated and a missing data file is written from
             * NULL, as in the ROM. */
            int found = checkFile(s_data_name[m_index], 0x3000);
            if (found == 1) {
                m_sub = 0x104;
                return 0;
            }
            setOpenMode(found);
        }
        return 0;
    case 0xdc:
        if (cmdOpen() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0xe6);
            return 0;
        }
        break;
    case 0xe6:
        m_sub = 0xf0;
        m_size = 0x3000;
        m_buf = m_data;
        /* fall through */
    case 0xf0:
        if (cmdWrite() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0xfa);
            return 0;
        }
        break;
    case 0xfa:
        if (cmdClose() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x104);
            return 0;
        }
        break;
    case 0x104:
        if (++m_index < MCD_DATA_NUM) {
            m_sub = 0xd2;
            return 0;
        }
        freeWork();
        m_sub = 0x3c;
        return 0;
    case 999:
        m_status = MCD_STATUS_DONE;
        return 1;
    default:
        return 0;
    }
    /* The steps that hold m_data: a failure frees it. */
    if (m_status == MCD_STATUS_ERROR) {
        freeWork();
        return 1;
    }
    return 0;
}

// ROR 0x0018ad0c
int McdMan::cmdReadDir()
{
    switch (m_sub) {
    case 0:
        m_sub = 1;
        m_step = MCD_STEP_START;
        m_status = MCD_STATUS_BUSY;
        /* fall through */
    case 1:
        if (cmdGetDir() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x6e);
            return 0;
        }
        return subEnd();
    case 0x6e:
        m_status = MCD_STATUS_DONE;
        return 1;
    default:
        return 0;
    }
}

/* m_name from m_buf / m_size. A data file ("/dtNN________") is written as
 * "/dtNN1_______" -- the old one (m_path, when given) renamed to that
 * first -- and renamed back once closed, so a broken write leaves the mark. */
// ROR 0x0018ade0
int McdMan::cmdWriteFile()
{
    int ret;

    switch (m_sub) {
    case 0:
        m_status = MCD_STATUS_BUSY;
        m_open_tries = 0;
        m_step = MCD_STEP_START;
        m_mark = NULL;
        if (m_name[0]) {
            m_mark = strstr(m_name, s_data_dir);
            if (m_mark) {
                m_mark += 5;
                *m_mark = '1';
            }
        }
        m_sub = m_path[0] ? 10 : 0x3c;
        return 0;
    case 10:
        if (cmdRename() != 1)
            return 0;
        if (m_status != MCD_STATUS_DONE) {
            if (m_status != MCD_STATUS_ERROR)
                return 0;
            if (m_result != MCD_ERR_NOTFILENAME)
                return 1;
        }
        next(0x3c);
        return 0;
    case 0x3c:
        m_sub = 0x46;
        m_status = MCD_STATUS_BUSY;
        m_step = MCD_STEP_START;
        setPath(m_name);
        m_open_mode = sceMcFileAttrReadable | sceMcFileAttrWriteable;
        return 0;
    case 0x46:
        if (cmdOpen() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x50);
            return 0;
        }
        if (m_status != MCD_STATUS_ERROR)
            return 0;
        /* Not there: once more, creating it. */
        ret = 1;
        if (m_result == MCD_ERR_NOTPATHNAME && m_open_tries == 0) {
            m_step = MCD_STEP_START;
            m_open_mode = sceMcFileCreateFile | sceMcFileAttrReadable | sceMcFileAttrWriteable;
            m_status = MCD_STATUS_BUSY;
            ret = 0;
        }
        m_open_tries++;
        return ret;
    case 0x50:
        m_sub = 0x5a;
        /* fall through */
    case 0x5a:
        if (cmdWrite() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(100);
            return 0;
        }
        return subEnd();
    case 100:
        if (cmdClose() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x65);
            return 0;
        }
        return subEnd();
    case 0x65:
        if (!m_mark) {
            m_sub = 0x6e;
            return 0;
        }
        m_sub = 0x66;
        strcpy(m_path, m_name);
        *m_mark = '0';
        return 0;
    case 0x66:
        if (cmdRename() != 1)
            return 0;
        if (m_status != MCD_STATUS_DONE) {
            if (m_status != MCD_STATUS_ERROR)
                return 0;
            if (m_result != MCD_ERR_NOTFILENAME)
                return 1;
        }
        next(0x6e);
        return 0;
    case 0x6e:
        m_status = MCD_STATUS_DONE;
        return 1;
    default:
        return 0;
    }
}

/* m_path into m_buf / m_size. */
// ROR 0x0018b12c
int McdMan::cmdReadFile()
{
    switch (m_sub) {
    case 0:
        m_status = MCD_STATUS_BUSY;
        m_sub = 0x3c;
        m_step = MCD_STEP_START;
        /* fall through */
    case 0x3c:
        m_open_mode = sceMcFileAttrReadable;
        m_sub = 0x46;
        m_step = MCD_STEP_START;
        m_status = MCD_STATUS_BUSY;
        return 0;
    case 0x46:
        if (cmdOpen() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x50);
            return 0;
        }
        return subEnd();
    case 0x50:
        m_sub = 0x5a;
        /* fall through */
    case 0x5a:
        if (cmdRead() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(100);
            return 0;
        }
        return subEnd();
    case 100:
        if (cmdClose() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x6e);
            return 0;
        }
        return subEnd();
    case 0x6e:
        m_status = MCD_STATUS_DONE;
        return 1;
    default:
        return 0;
    }
}

/* Debug: a /FEBDUMMY save (icon, icon.sys) and a /FEBDUMMY/FEBDUMMY file
 * written in pieces of up to 100 KB until the card is full. */
// ROR 0x0018b2bc // callcheck: strcpy (inside setPath)
int McdMan::cmdFillDummy()
{
    /* PORT: the ROM writes the fill from m_icon_sys, past the end of McdMan
     * (the content does not matter); the host writes zeros. */
    static u_char s_fill[100 << 10];

    switch (m_sub) {
    case 0:
        m_sub = 1;
        m_step = MCD_STEP_START;
        m_status = MCD_STATUS_BUSY;
        setPath(s_dummy_dir);
        /* fall through */
    case 1:
        if (cmdMkdir() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(4);
            return 0;
        }
        if (m_status != MCD_STATUS_ERROR)
            return 0;
        if (m_result != MCD_ERR_EXISTENTRY)
            return 1;
        m_result = MCD_ERR_NOERR;
        m_sub = 4;
        m_step = MCD_STEP_START;
        m_status = MCD_STATUS_BUSY;
        return 0;
    case 4:
        m_index = 0;
        /* fall through */
    case 5:
        m_status = MCD_STATUS_BUSY;
        m_sub = 10;
        m_step = MCD_STEP_START;
        setPath(s_dummy_icon_path[m_index]);
        m_open_mode = sceMcFileCreateFile | sceMcFileAttrReadable | sceMcFileAttrWriteable;
        return 0;
    case 10:
        if (cmdOpen() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x14);
            return 0;
        }
        return subEnd();
    case 0x14:
        m_sub = 0x1e;
        m_size = m_icon_size[m_index];
        m_buf = m_icon[m_index];
        return 0;
    case 0x1e:
        if (cmdWrite() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x28);
            return 0;
        }
        return subEnd();
    case 0x28:
        if (cmdClose() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x32);
            return 0;
        }
        return subEnd();
    case 0x32:
        if (++m_index < MCD_ICON_NUM)
            m_sub = 5;
        else
            m_sub = 0x3c;
        return 0;
    case 0x3c:
        m_sub = 0x46;
        m_status = MCD_STATUS_BUSY;
        m_step = MCD_STEP_START;
        makeIconSys();
        setPath(s_dummy_icon_sys);
        m_open_mode = sceMcFileCreateFile | sceMcFileAttrReadable | sceMcFileAttrWriteable;
        return 0;
    case 0x46:
        if (cmdOpen() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x50);
            return 0;
        }
        return subEnd();
    case 0x50:
        m_buf = &m_icon_sys;
        m_sub = 0x5a;
        m_size = sizeof(sceMcIconSys);
        return 0;
    case 0x5a:
        if (cmdWrite() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(100);
            return 0;
        }
        return subEnd();
    case 100:
        if (cmdClose() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x6e);
            return 0;
        }
        return subEnd();
    case 0x6e:
        m_sub = 0x78;
        return 0;
    case 0x78:
        if (cmdGetInfo() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x82);
            return 0;
        }
        return subEnd();
    case 0x82:
        /* The ROM's dead store: the next one overwrites it. */
        if (m_free < 2)
            m_sub = 0xa1;
        setPath(s_dummy_path);
        m_sub = 0x8c;
        m_open_mode = sceMcFileCreateFile | sceMcFileAttrReadable | sceMcFileAttrWriteable;
        return 0;
    case 0x8c:
        if (cmdOpen() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x96);
            return 0;
        }
        return subEnd();
    case 0x96:
        m_index = 1;
        m_sub = 0x97;
        return 0;
    case 0x97:
        m_sub = 0x98;
        m_fill_kb = m_free - m_index < 100 ? m_free - m_index : 100;
        m_buf = s_fill;
        m_size = m_fill_kb << 10;
        /* fall through */
    case 0x98:
        if (cmdWrite() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0x99);
            return 0;
        }
        return subEnd();
    case 0x99:
        m_index += m_fill_kb;
        m_sub = m_index >= m_free ? 0xa0 : 0x97;
        return 0;
    case 0xa0:
        if (cmdClose() != 1)
            return 0;
        if (m_status == MCD_STATUS_DONE) {
            next(0xa1);
            return 0;
        }
        return subEnd();
    case 0xa1:
        m_sub = 999;
        return 0;
    case 999:
        m_status = MCD_STATUS_DONE;
        return 1;
    default:
        return 0;
    }
}

// ROR 0x0018b874
int McdMan::hasIconSys()
{
    int i;

    for (i = 0; i < m_file_num; i++) {
        if (strcmp(getDirName(i), s_icon_sys_name) == 0 &&
            getDirSize(i) == (int)sizeof(sceMcIconSys))
            return 1;
    }
    return 0;
}

// ROR 0x0018b918
int McdMan::matchName(const char *name, const char *pat)
{
    int i;

    if (!name || !pat)
        return -1;
    for (i = 0; name[i] && pat[i]; i++) {
        if (name[i] != '?' && pat[i] != '?' && name[i] != pat[i])
            return -1;
    }
    return name[i] || pat[i] ? -1 : 0;
}

// ROR 0x0018b9b8
int McdMan::findSave(const char *name)
{
    int i;

    for (i = 0; i < m_file_num; i++) {
        if (matchName(getDirName(i), name) == 0)
            return strstr(getDirName(i), s_data_tail) == NULL;
    }
    return 0;
}

// ROR 0x0018ba68
int McdMan::checkFile(const char *name, int size)
{
    int i;

    for (i = 0; i < m_file_num; i++) {
        if (matchName(getDirName(i), name) == 0)
            return getDirSize(i) == size ? 1 : 2;
    }
    return 0;
}

// ROR 0x0018bb28
int McdMan::hasSpace()
{
    int total = 0;
    int i;

    for (i = 0; i < m_file_num; i++)
        total += getDirSize(i);
    return m_free >= MCD_SAVE_KB - total || m_free > MCD_SAVE_KB - 1;
}
