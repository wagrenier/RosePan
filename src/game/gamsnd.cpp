// game/gamsnd.cpp -- the game's sound state GamSndMan (0x1e0df0..0x1e7094;
// the file name is in init's allocDebug tags).
//
// GamMan's member m_sndMan (bss 0x79a0c0). Over SndMan (snd/sndman.cpp) it
// keeps the SE and stream tables of the system pack, the SE pack loaded in
// each bank, a record per playing sound (GamSndRec, by SndMan id), the
// master volumes (fades run as TskSnd tasks), the BGM push levels
// (pushSnd / resumeBgm around menus and sub-sequences) and the 3D sounds
// (update3d, the put_* / del_* calls). The script's native 252 and
// GamMan's forwarders call it.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/rosepan_stub.h"
#include "game/gamsnd.h"

#include "bed/bedmemory.h"
#include "game/gamman.h"
#include "i3d/i3dmath.h"
#include "lib/unassigned.h"
#include "mag/magsystem.h"
#include "rx/rxsnd.h"
#include "snd/sndman.h"
#include "task/tsk_man.h"
#include "utl/utlmisc.h"

static const char s_file[] = "game/gamsnd.cpp";

/* SndMan's "keep this pitch" (-1024.0f, SND_KEEP as a float). */
#define GAMSND_PITCH_KEEP   ((float)SND_KEEP)

// ──────────────────────────────────────────────────────────────────────
// Both 3D lists linked through the 32 nodes (all free), the record range
// at the inline records.
// ROR 0x001e0df0

GamSndMan::GamSndMan()
{
    m_free.m_head.m_prev = &m_free.m_head;
    m_free.m_head.m_next = &m_free.m_head;
    m_free.m_count = 0;
    m_used.m_head.m_next = &m_used.m_head;
    m_used.m_count = 0;
    m_used.m_head.m_prev = &m_used.m_head;
    m_nodeMax = GAMSND_NODE_MAX;
    for (int i = 0; i < m_nodeMax; i++)
    {
        m_node[i].m_prev = &m_node[i] - 1;
        m_node[i].m_next = &m_node[i] + 1;
    }
    m_used.m_head.m_next = &m_used.m_head;
    m_used.m_count = 0;
    m_used.m_head.m_prev = &m_used.m_head;
    m_free.m_head.m_next = &m_node[0];
    m_free.m_count = m_nodeMax;
    m_node[0].m_prev = &m_free.m_head;
    GamSnd3dNode *last = &m_node[m_free.m_count - 1];
    m_free.m_head.m_prev = last;
    last->m_next = &m_free.m_head;

    m_rec = m_recBuf;
    m_recMax = GAMSND_REC_MAX;
    m_recEnd = m_recBuf;
}

// ──────────────────────────────────────────────────────────────────────
// The ROM walks the (empty) record range and deletes for flags bit 0;
// nothing to free on the host.
// ROR 0x001e0eb8 // callcheck: operator_delete (the deleting variant: delete on the host)

GamSndMan::~GamSndMan()
{
}

// ──────────────────────────────────────────────────────────────────────
// Volumes back to 0x80, the records and lists reset, the SE table (0x400
// entries) and the stream table (900) allocated, nothing pushed, no fades,
// no SE pack in any bank.
// ROR 0x001e0f1c

void GamSndMan::init()
{
    resetVolume();
    reset();
    m_table = (GamSndEntry *)g_utlMemory.allocDebug(GAMSND_SE_MAX * sizeof(GamSndEntry),
                                                     s_file, 81, 0, 1);
    void *strTable = g_utlMemory.allocDebug(GAMSND_STR_MAX * sizeof(GamSndEntry), s_file, 82, 0, 1);
    m_157a = 0x6a4;
    m_157c = 100;
    m_157e = 0x80;
    m_bgm[0].m_str = -1;
    m_3dScale = 0.5f;
    m_1576 = 0x80;
    m_1578 = 0x80;
    m_163c = -1;
    m_mstTaskId = -1;
    m_seTaskId = -1;
    m_strTaskId = -1;
    m_pushDepth = -1;
    m_strTable = (GamSndEntry *)strTable;
    m_1580 = 0;
    m_1630 = 0;
    m_bgm[0].m_se = 0;
    memset(m_bankId, -1, sizeof(m_bankId));
}

// ──────────────────────────────────────────────────────────────────────
// Every frame: the 3D sounds, heard from pos or (m_mode 1) the camera.
// ROR 0x001e100c

void GamSndMan::update(const float *pos)
{
    if (m_used.m_count == 0)
        return;
    if (m_mode == 1)
    {
        const float *eye = g_magSystem.m_scene.m_camera_entity.m_trans;
        float cam[3] = { eye[0], eye[1], eye[2] };
        update3d(cam);
    }
    else
        update3d(pos);
}

// ──────────────────────────────────────────────────────────────────────
// The listener back to the PC and every master volume to 0x80 at once.
// ROR 0x001e107c

void GamSndMan::resetVolume()
{
    m_171c = 4;
    m_1720 = 0x80;
    m_1724 = 0x80;
    m_mode = 0;
    funcSnd(0xd, 0x80, 0x80, 0, -1);
    m_mstVolStr[0] = 0x80;
    m_mstVolSe[1] = 0x80;
    m_mstVolStr[1] = 0x80;
    m_mstVolSe[0] = 0x80;
    funcSnd(0xe, 0x80, 0x80, 0, -1);
    funcSnd(0xf, 0x80, 0x80, 0, -1);
}

// ──────────────────────────────────────────────────────────────────────
// One push level: the BGM-class SEs (id >> 24 == 0xb, but 0xb030352) and
// the GAMSND_REC_BGM stream with their volumes (a running fade's volume
// when its task lives), the looping SEs (GAMSND_REC_LOOP or the entry's
// m_resume) into m_loopSe. Mode 1 skips the streams, 2 the SEs. Only
// when nothing is pushed: the ROM never nests.
// ROR 0x001e111c

void GamSndMan::pushSnd(int mode)
{
    if (m_pushDepth >= 0)
        return;

    m_loopSeCount = 0;
    m_pushDepth++;
    m_bgm[m_pushDepth].m_se = 0;
    m_bgm[m_pushDepth].m_str = -1;
    m_bgm[m_pushDepth].m_str2 = -1;
    m_pushSe[m_pushDepth].m_se = 0;

    for (int i = 0; i < GAMSND_REC_MAX; i++)
    {
        GamSndRec *rec = &m_rec[i];
        u_int flags = rec->m_flags;
        if ((flags & GAMSND_REC_KEEP) != 0)
            continue;

        if ((flags & GAMSND_REC_STR) == 0)
        {
            if (mode == 2 || rec->m_se == 0 || rec->m_se == 0xb030352)
                continue;
            if (!getStatSEtrack(rec->m_se, rec->m_slot))
                continue;

            u_int se = rec->m_se;
            if ((se >> 24) == 0xb)
            {
                GamBgmRec *bgm = &m_bgm[m_pushDepth];
                if (bgm->m_se == 0)
                {
                    bgm->m_se = se;
                    if (g_taskMan.getTask(m_seTaskId) == 0)
                    {
                        bgm->m_seVolL = rec->m_volL;
                        bgm->m_seVolR = rec->m_volR;
                    }
                    else
                    {
                        TskSnd *task = static_cast<TskSnd *>(g_taskMan.findTask(m_seTaskId));
                        task->getVolume(&m_bgm[m_pushDepth].m_seVolL,
                                        &m_bgm[m_pushDepth].m_seVolR);
                    }
                }
                else
                {
                    m_pushSe[m_pushDepth].m_se = se;
                    m_pushSe[m_pushDepth].m_volL = rec->m_volL;
                    m_pushSe[m_pushDepth].m_volR = rec->m_volR;
                }
            }
            else
            {
                GamSndEntry *entry = NULL;
                findSnd(rec->m_se, &entry, 1);
                if ((rec->m_flags & GAMSND_REC_LOOP) == 0 && (entry == NULL || entry->m_resume == 0))
                    continue;
                m_loopSe[m_loopSeCount].m_se = rec->m_se;
                m_loopSe[m_loopSeCount].m_volL = rec->m_volL;
                m_loopSe[m_loopSeCount].m_volR = rec->m_volR;
                m_loopSeCount++;
            }
        }
        else if (mode != 1)
        {
            if (rec->m_str == -1 || (flags & GAMSND_REC_BGM) == 0)
                continue;

            /* The ROM tests getStatSTRtrack's 0 / 1 against 5 and -1: the
             * stream is always stored. */
            int stat = getStatSTRtrack(rec->m_str, rec->m_slot);
            GamBgmRec *bgm = &m_bgm[m_pushDepth];
            if (stat != 5 && stat != -1)
            {
                bgm->m_str = rec->m_str;
                if (g_taskMan.getTask(m_strTaskId) != 0)
                {
                    TskSnd *task = static_cast<TskSnd *>(g_taskMan.findTask(m_strTaskId));
                    task->getVolume(&bgm->m_strVolL, &bgm->m_strVolR);
                }
                else
                {
                    bgm->m_strVolL = rec->m_volL;
                    bgm->m_strVolR = rec->m_volR;
                }
            }
            if (bgm->m_strVolL == 0 && bgm->m_strVolR == 0 && m_pushDepth > 0)
            {
                bgm->m_str = bgm[-1].m_str;
                bgm->m_strVolL = bgm[-1].m_strVolL;
                bgm->m_strVolR = bgm[-1].m_strVolR;
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// pushSnd's level back: what still plays is resumed, what stopped plays
// again, each from volume 8 up to its pushed volume over 30 frames.
// ROR 0x001e1920

void GamSndMan::resumeBgm(int replay, int mode, int reset)
{
    m_flags &= ~1u;
    if (m_pushDepth < 0)
        return;
    if (reset != 0)
        m_pushDepth = 0;

    int low[2] = { 8, 8 };
    TskSndParam fade = {};     /* PC: zeroed (the ROM's stack words the fade does not set) */

    if (mode != 1)
    {
        GamBgmRec *bgm = &m_bgm[m_pushDepth];
        if (bgm->m_str != -1)
        {
            if (replay == 0 && funcStr(bgm->m_str, 0x16, NULL) != 0)
                funcStr(m_bgm[m_pushDepth].m_str, 0x18, NULL);
            else
            {
                playStr(m_bgm[m_pushDepth].m_str, 0, NULL, false, GAMSND_REC_BGM);
                funcStr(m_bgm[m_pushDepth].m_str, 4, (void *)1);
            }
            funcStr(m_bgm[m_pushDepth].m_str, 2, low);
            fade.m_frames = 0x1e;
            fade.m_volR = m_bgm[m_pushDepth].m_strVolR;
            fade.m_volL = m_bgm[m_pushDepth].m_strVolL;
            funcStr(m_bgm[m_pushDepth].m_str, 0x13, &fade);
            m_bgm[m_pushDepth].m_str = -1;
        }

        bgm = &m_bgm[m_pushDepth];
        if (bgm->m_str2 != -1)
        {
            funcStr(bgm->m_str2, 0x18, NULL);
            funcStr(m_bgm[m_pushDepth].m_str2, 2, low);
            fade.m_frames = 0x1e;
            fade.m_volR = m_bgm[m_pushDepth].m_str2VolR;
            fade.m_volL = m_bgm[m_pushDepth].m_str2VolL;
            /* funcStr has no case 0x14: this fade never starts. */
            funcStr(m_bgm[m_pushDepth].m_str2, 0x14, &fade);
            m_bgm[m_pushDepth].m_str2 = -1;
        }
    }

    if (mode != 2)
    {
        GamBgmRec *bgm = &m_bgm[m_pushDepth];
        if (bgm->m_se != 0)
        {
            if (funcSE(bgm->m_se, 0x16, NULL) == 0)
                playSE(m_bgm[m_pushDepth].m_se, false, NULL, 0);
            if (m_bgm[m_pushDepth].m_se == 0xb030352)
                funcSE(0xb030352, 4, NULL);
            int vol[2] = { m_bgm[m_pushDepth].m_seVolL, m_bgm[m_pushDepth].m_seVolR };
            if (funcSE(m_bgm[m_pushDepth].m_se, 0x21, vol) == 0)
            {
                funcSE(m_bgm[m_pushDepth].m_se, 2, low);
                fade.m_frames = 0x1e;
                fade.m_volR = m_bgm[m_pushDepth].m_seVolR;
                fade.m_volL = m_bgm[m_pushDepth].m_seVolL;
                funcSE(m_bgm[m_pushDepth].m_se, 0x13, &fade);
            }
            m_bgm[m_pushDepth].m_se = 0;
        }

        GamSndPushSe *push = &m_pushSe[m_pushDepth];
        if (push->m_se != 0)
        {
            if (funcSE(push->m_se, 0x16, NULL) == 0)
                playSE(m_pushSe[m_pushDepth].m_se, false, NULL, 0);
            if (m_pushSe[m_pushDepth].m_se == 0xb030352)
                funcSE(0xb030352, 4, NULL);
            int vol[2] = { m_pushSe[m_pushDepth].m_volL, m_pushSe[m_pushDepth].m_volR };
            if (funcSE(m_pushSe[m_pushDepth].m_se, 0x21, vol) == 0)
            {
                funcSE(m_pushSe[m_pushDepth].m_se, 2, low);
                fade.m_frames = 0x1e;
                fade.m_volR = m_pushSe[m_pushDepth].m_volR;
                fade.m_volL = m_pushSe[m_pushDepth].m_volL;
                funcSE(m_pushSe[m_pushDepth].m_se, 0x14, &fade);
            }
            m_pushSe[m_pushDepth].m_se = 0;
        }
    }

    if (m_loopSeCount > 0)
    {
        for (int i = 0; i < m_loopSeCount; i++)
        {
            GamSndLoopSe *loop = &m_loopSe[i];
            if (funcSE(loop->m_se, 0x16, NULL) == 0)
                playSE(m_loopSe[m_pushDepth].m_se, false, NULL, 0);    /* sic: by the push depth */
            funcSE(loop->m_se, 4, (void *)1);
            funcSE(loop->m_se, 2, low);
            fade.m_volL = loop->m_volL;
            fade.m_volR = loop->m_volR;
            fade.m_frames = 0x1e;
            funcSE(loop->m_se, 0x15, &fade);
        }
        m_loopSeCount = 0;
    }
    m_pushDepth--;
}

// ──────────────────────────────────────────────────────────────────────
// Non-zero when id is in the SE table and its bank holds an SE pack.
// ROR 0x001e435c

int GamSndMan::isSndBankLoaded(int id)
{
    GamSndEntry *entry;
    if (findSnd(id, &entry, 1) == 0)
        return 0;
    return isBankSet(entry->m_bank, entry->m_32, 1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e43a4

bool GamSndMan::isBankSet(int bank, int arg, int check)
{
    (void)arg;
    if ((u_int)bank > SND_BANK_MAX - 1)
        return false;
    if (check == 0)
        return true;
    return m_bankId[bank] != -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e43e0

void GamSndMan::parseSePackName(const char *name, int *bank, int *id)
{
    char num[3];
    num[0] = name[2];
    num[1] = name[3];
    num[2] = 0;
    *id = atoi(num);
    num[2] = 0;
    num[0] = name[0];
    num[1] = name[1];
    *bank = atoi(num);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e4458

void GamSndMan::loadSePack(const char *name, void *data, int nowait)
{
    int bank, id;
    parseSePackName(name, &bank, &id);
    loadSePackData(bank, id, data, nowait);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e44b4

void GamSndMan::loadSePackData(int bank, int id, void *data, int nowait)
{
    funcSnd(0x11, 0, 0, 0, -1);
    g_sndMan.loadBank(bank, data, nowait);
    m_bankId[bank] = id;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e46b8

void GamSndMan::freeSePack(int n)
{
    g_sndMan.freeBank(n);
    m_bankId[n] = -1;
}

// ──────────────────────────────────────────────────────────────────────
// The system pack's SE table into m_table (state is not read).
// ROR 0x001e4790

void GamSndMan::registSnd(int state, void *data)
{
    (void)state;
    memcpy(m_table, data, GAMSND_SE_MAX * sizeof(GamSndEntry));
}

// ──────────────────────────────────────────────────────────────────────
// The stream table into m_strTable.
// ROR 0x001e484c

void GamSndMan::registStrSnd(int state, void *data)
{
    (void)state;
    memcpy(m_strTable, data, GAMSND_STR_MAX * sizeof(GamSndEntry));
}

// ──────────────────────────────────────────────────────────────────────
// A linear search of the SE table (to id 0 or 0x400 entries); no table
// returns 0 with *entry untouched.
// ROR 0x001e4908

int GamSndMan::findSnd(int id, GamSndEntry **entry, int arg)
{
    (void)arg;
    GamSndEntry *e = m_table;
    if (e == NULL)
        return 0;
    for (int n = 0; n < GAMSND_SE_MAX && e[n].m_id != 0; n++)
    {
        if (e[n].m_id == id)
        {
            *entry = &e[n];
            return 1;
        }
    }
    *entry = NULL;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The stream table is hashed: str's entry is searched from (str % 100000)
// / 2 on, back to the start at a free entry (-1) or the end, 900 tries.
// ROR 0x001e4c1c

int GamSndMan::getStrSnd(int str, GamSndEntry **entry)
{
    int idx = (str % 100000) / 2;
    for (int n = 0; n < GAMSND_STR_MAX; n++, idx++)
    {
        /* PC: the end test first; the ROM reads entry idx (one past the
         * table at 900) before it. */
        if (idx >= GAMSND_STR_MAX || m_strTable[idx].m_id == -1)
            idx = 0;
        if (m_strTable[idx].m_id == str)
        {
            *entry = &m_strTable[idx];
            return 1;
        }
    }
    *entry = NULL;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e4cbc

int gamSndReturnZero()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e4d04

int GamSndMan::get_se_oid(const char *name)
{
    return getSeID(name);
}

// ──────────────────────────────────────────────────────────────────────
// The record of a sound SndMan could not start (id -1) is the one before
// the table: padding on the PS2.
// ROR 0x001e4ebc

int GamSndMan::playSE(u_int se, bool pan, float *pos, int flags)
{
    if ((g_gamMan.m_flags2 & 1) != 0)
        return -1;

    GamSndEntry *entry;
    if (findSnd(se, &entry, 1) == 0)
        return -1;

    int volL, volR, id;
    if (pos == NULL)
    {
        id = g_sndMan.playSe(se);
        if (!pan)
        {
            volR = 100;
            volL = 100;
        }
        else
        {
            float share = (float)entry->m_pan * 0.00390625f;
            volL = (int)((float)entry->m_vol * share);
            volR = (int)((float)entry->m_vol * (1.0f - share));
        }
        g_sndMan.setSe(id, volL, volR, GAMSND_PITCH_KEEP, 0);
    }
    else
    {
        float at[4] = { pos[0], pos[1], pos[2], pos[3] };
        gamSnd3dVolume(m_mode, at, entry, &volL, &volR, 100.0f);
        id = g_sndMan.playSe(se);
        g_sndMan.setSe(id, volL, volR, GAMSND_PITCH_KEEP, 0);
    }

    if (id < 0)
        return id;      /* PC: no record before m_recBuf */
    GamSndRec *rec = &m_rec[id];
    rec->m_flags = flags;
    rec->m_se = se;
    rec->m_slot = id;
    rec->m_volL = volL;
    rec->m_volR = volR;
    return id;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e5250

int GamSndMan::playStr(int str, int id, float *pos, bool b, u_int flags)
{
    char name[0x40];
    snprintf(name, sizeof(name), "s%06d.str", str);

    int sid, volL, volR;
    if (pos == NULL)
    {
        sid = g_sndMan.playStream(name, str > 99999, id, (flags >> 8) & 1);
        volR = 0x80;
        volL = 0x80;
    }
    else
    {
        GamSndEntry *entry;
        if (getStrSnd(str, &entry) == 0)
            return -1;
        float at[4] = { pos[0], pos[1], pos[2], pos[3] };
        gamSnd3dVolume(m_mode, at, entry, &volL, &volR, 100.0f);
        sid = g_sndMan.playStream(name, str > 99999, id, (flags >> 8) & 1);
        g_sndMan.setStreamLoop(sid, 0);
        g_sndMan.setStream(sid, volL, volR, GAMSND_PITCH_KEEP, 0);
    }

    if (sid < 0)
        return sid;     /* PC: no record before m_recBuf */
    GamSndRec *rec = &m_rec[sid];
    rec->m_flags = b ? (flags | GAMSND_REC_8 | GAMSND_REC_STR) : (flags | GAMSND_REC_STR);
    rec->m_str = str;
    rec->m_slot = sid;
    rec->m_volL = volL;
    rec->m_volR = volR;
    return sid;
}

// ──────────────────────────────────────────────────────────────────────
// Function no on the stream records of str: 1 stop the first / 8 every
// one, 2 / 9 volume (the record keeps them swapped, as the ROM does), 3 /
// 10 pitch, 5 / 0xb both, 4 the loop (decided by the first stream record,
// str or not), 0x13 / 0x1b a fade into m_strTaskId (0x1b stops at the end),
// 0x15 an untracked one, 0x16 1 while one plays, 0x17 / 0x18 pause /
// resume them all, 0x1a 1 when paused. A record whose stream ended is
// cleared on the way.
// ROR 0x001e5660 // callcheck: operator_new (new TskSnd)

int GamSndMan::funcStr(int str, int no, void *arg)
{
    int *prm = (int *)arg;
    TskSndParam *fade = (TskSndParam *)arg;

    for (int i = 0; i < GAMSND_REC_MAX; i++)
    {
        GamSndRec *rec = &m_rec[i];
        if ((rec->m_flags & GAMSND_REC_STR) == 0 || rec->m_str == -1)
            continue;

        if (no == 0x17)
        {
            if (rec->m_str == str && rec->m_slot != -1)
                g_sndMan.pauseStream(rec->m_slot);
            continue;
        }
        if (no == 0x18)
        {
            if (rec->m_str == str && rec->m_slot != -1)
                g_sndMan.resumeStream(rec->m_slot);
            continue;
        }
        if (no == 4)
        {
            if (rec->m_str != str || rec->m_slot == -1)
                return 0;
            g_sndMan.setStreamLoop(rec->m_slot, (int)(intptr_t)arg);
            return 0;
        }

        if (rec->m_str != str)
            continue;
        int slot = rec->m_slot;
        if (no == 0x1a)
            return g_sndMan.getStreamState(slot) == SND_STATE_PAUSED;
        if (g_sndMan.getStreamState(slot) == -1)
        {
            rec->m_str = -1;
            continue;
        }

        switch (no)
        {
        case 1:
        case 8:
            g_sndMan.stopStream(rec->m_slot);
            rec->m_str = -1;
            rec->m_slot = -1;
            if (no == 1)
                return 0;
            break;
        case 2:
        case 9:
            if (arg == NULL)
                return 0;
            rec->m_volR = prm[0];
            rec->m_volL = prm[1];
            g_sndMan.setStream(rec->m_slot, prm[0], prm[1], GAMSND_PITCH_KEEP, 0);
            if (no == 2)
                return 0;
            break;
        case 3:
        case 10:
            if (arg == NULL)
                return 0;
            g_sndMan.setStream(rec->m_slot, SND_KEEP, SND_KEEP, ((float *)arg)[0], 0);
            if (no == 3)
                return 0;
            break;
        case 5:
        case 0xb:
            if (arg == NULL)
                return 0;
            rec->m_volR = prm[0];
            rec->m_volL = prm[1];
            g_sndMan.setStream(rec->m_slot, prm[0], prm[1], ((float *)arg)[2], 0);
            if (no == 5)
                return 0;
            break;
        case 0x13:
        case 0x1b:
        {
            fade->m_snd = str;
            fade->m_id = rec->m_slot;
            fade->m_curL = rec->m_volL;
            fade->m_curR = rec->m_volR;
            fade->m_arg = (no == 0x13) ? 0 : 1;
            if (g_taskMan.getTask(m_strTaskId) != 0)
            {
                g_taskMan.killTask(m_strTaskId);
                m_strTaskId = -1;
            }
            TskSnd *task = new TskSnd(1, fade);
            g_taskMan.addTask(1, -1, 1, 0, task, 0, &m_strTaskId, 0x11);
            return m_strTaskId;
        }
        case 0x15:
        {
            fade->m_snd = str;
            fade->m_id = rec->m_slot;
            fade->m_curL = rec->m_volL;
            fade->m_arg = 1;
            fade->m_curR = rec->m_volR;
            TskSnd *task = new TskSnd(1, fade);
            g_taskMan.addTask(1, -1, 1, 0, task, 0, NULL, 0x11);
            return 0;
        }
        case 0x16:
            return 1;
        default:
            break;
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Whether stream str still plays in SndMan id track.
// ROR 0x001e5b60

bool GamSndMan::getStatSTRtrack(u_int str, int track)
{
    if (track < 0)
        return false;
    if (g_sndMan.getStreamState(track) == -1)
        return false;
    return (u_int)m_rec[track].m_str == str;
}

// ──────────────────────────────────────────────────────────────────────
// funcStr's twin over the SE records: 1 / 8 stop (not GAMSND_REC_KEEP
// ones), 2 / 9 volume, 3 / 10 pitch, 4 the loop (every one), 5 / 0xb
// volume, pitch and attribute, 6 / 0xc the attribute 5 set (the ROM reads
// it through a pointer only case 5 sets), 0x13 / 0x14 fades into
// m_seTaskId / m_seTaskId2, 0x15 an untracked one, 0x16 1 while one plays,
// 0x21 1 when the volume is the block's.
// ROR 0x001e5e58 // callcheck: operator_new (new TskSnd)

int GamSndMan::funcSE(u_int se, int no, void *arg)
{
    int *prm = (int *)arg;
    TskSndParam *fade = (TskSndParam *)arg;
    const int *set = NULL;

    for (int i = 0; i < GAMSND_REC_MAX; i++)
    {
        GamSndRec *rec = &m_rec[i];
        if ((rec->m_flags & GAMSND_REC_STR) != 0 || rec->m_se == 0 || rec->m_se != se)
            continue;
        if (g_sndMan.getSeState(rec->m_slot) == -1)
        {
            rec->m_se = 0;
            continue;
        }

        switch (no)
        {
        case 1:
        case 8:
            if ((rec->m_flags & GAMSND_REC_KEEP) != 0)
                break;
            g_sndMan.stopSe(rec->m_slot);
            rec->m_se = 0;
            rec->m_slot = -1;
            rec->m_volL = 0;
            rec->m_volR = 0;
            if (no == 1)
                return 0;
            break;
        case 2:
        case 9:
            rec->m_volL = prm[0];
            rec->m_volR = prm[1];
            g_sndMan.setSe(rec->m_slot, prm[0], prm[1], GAMSND_PITCH_KEEP, 0);
            if (no == 2)
                return 0;
            break;
        case 3:
        case 10:
            g_sndMan.setSe(rec->m_slot, SND_KEEP, SND_KEEP, ((float *)arg)[0], 0);
            if (no == 3)
                return 0;
            break;
        case 4:
            rec->m_flags |= GAMSND_REC_LOOP;
            g_sndMan.setSeLoop(rec->m_slot, (int)(intptr_t)arg);
            break;
        case 5:
        case 0xb:
            rec->m_volR = prm[1];
            rec->m_volL = prm[0];
            g_sndMan.setSe(rec->m_slot, prm[0], prm[1], ((float *)arg)[2], prm[4]);
            set = prm;
            if (no == 5)
                return 0;
            break;
        case 6:
        case 0xc:
            /* PC: set is always NULL here (the ROM reads address 0x10). */
            g_sndMan.setSe(rec->m_slot, SND_KEEP, SND_KEEP, GAMSND_PITCH_KEEP,
                           set != NULL ? set[4] : 0);
            return 0;
        case 0x13:
        case 0x14:
        {
            int *taskId = (no == 0x13) ? &m_seTaskId : &m_seTaskId2;
            fade->m_snd = se;
            fade->m_id = rec->m_slot;
            fade->m_curL = rec->m_volL;
            fade->m_curR = rec->m_volR;
            if (g_taskMan.getTask(*taskId) != 0)
            {
                g_taskMan.killTask(*taskId);
                *taskId = -1;
            }
            TskSnd *task = new TskSnd(0, fade);
            g_taskMan.addTask(1, -1, 1, 0, task, 0, taskId, 0x11);
            return *taskId;
        }
        case 0x15:
        {
            fade->m_snd = se;
            fade->m_id = rec->m_slot;
            fade->m_curL = rec->m_volL;
            fade->m_curR = rec->m_volR;
            TskSnd *task = new TskSnd(0, fade);
            g_taskMan.addTask(1, -1, 1, 0, task, 0, NULL, 0x11);
            return m_seTaskId;
        }
        case 0x16:
            return 1;
        case 0x21:
            return rec->m_volL == prm[0] && rec->m_volR == prm[1];
        default:
            break;
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Whether SE se still plays in SndMan id track.
// ROR 0x001e6368

bool GamSndMan::getStatSEtrack(u_int se, int track)
{
    if (track < 0 || m_rec[track].m_se != se)
        return false;
    return g_sndMan.getSeState(track) != -1;
}

// ──────────────────────────────────────────────────────────────────────
// Records back to free: 0 all, 1 the SEs, 2 the streams.
// ROR 0x001e6570

void GamSndMan::clear_snd_lst(int arg)
{
    /* PC: the ROM copies a local whose words but m_str / m_slot are
     * never set; zero here. */
    GamSndRec empty = {};
    empty.m_str = -1;
    empty.m_slot = -1;

    if (arg == 1)
    {
        for (int i = 0; i < GAMSND_REC_MAX; i++)
            if ((m_rec[i].m_flags & GAMSND_REC_STR) == 0)
                m_rec[i] = empty;
    }
    else if (arg == 0)
    {
        for (int i = 0; i < GAMSND_REC_MAX; i++)
            m_rec[i] = empty;
    }
    else if (arg == 2)
    {
        for (int i = 0; i < GAMSND_REC_MAX; i++)
            if ((m_rec[i].m_flags & GAMSND_REC_STR) != 0)
                m_rec[i] = empty;
    }
}

/* The option level (0..8) as a master volume scale: 10..100, then
 * 100..128 (inline twice in funcSnd). */
static int gamSndLevelVolume(int level)
{
    if (level < 7)
        return (int)(((float)level / 6.0f) * 90.0f) + 10;
    if (level < 9)
        return (int)((float)(level - 6) * 0.5f * 28.0f) + 100;
    return 0x80;
}

// ──────────────────────────────────────────────────────────────────────
// Sound function no (SND_FUNC_NO): 0xd / 0xe / 0xf the master / SE /
// stream volume (a, b; over c frames through a TskSnd, else at once; the
// SE and stream ones scaled by the option levels, at least 10), 0x10 /
// 0x11 / 0x12 stop everything / the SEs / the streams and free their
// records, 0x19 stop all and free bank d, 0x1c the 3D scale (a percent),
// 0x1d / 0x1f pushSnd(a), 0x1e / 0x20 resumeBgm(1, a, 0) (0x1f / 0x20 set
// and clear m_flags 0x400).
// ROR 0x001e6734 // callcheck: operator_new (new TskSnd)

void GamSndMan::funcSnd(int no, int a, int b, int c, int d)
{
    TskSndParam fade = {};     /* PC: zeroed (the ROM's stack words the fade does not set) */
    int level;
    int *taskId;
    int kind;

    fade.m_frames = c;
    switch (no)
    {
    case 0xd:
        if (c == 0)
        {
            g_sndMan.m_volL = a;
            g_sndMan.m_volR = b;
            rxSndSetMasterVolume(a, b);
            m_mstVol[1] = b;
            m_mstVol[0] = a;
            return;
        }
        fade.m_curL = m_mstVol[0];
        fade.m_curR = m_mstVol[1];
        fade.m_volL = a;
        fade.m_volR = b;
        taskId = &m_mstTaskId;
        kind = 2;
        break;
    case 0xe:
        m_mstVolSe[0] = a;
        m_mstVolSe[1] = b;
        g_gamMan.setOption(5, (intptr_t)&level);
        level = gamSndLevelVolume(level);
        fade.m_volR = (int)((float)b * (float)level * 0.0078125f);
        fade.m_volL = (int)((float)a * (float)level * 0.0078125f);
        if (fade.m_volR < 0xb)
            fade.m_volR = 10;
        if (fade.m_volL < 0xb)
            fade.m_volL = 10;
        if (c == 0)
        {
            rxSndSeSetMasterVolume(fade.m_volL, fade.m_volR);
            return;
        }
        fade.m_curL = m_mstVolSe[0];
        fade.m_curR = m_mstVolSe[1];
        taskId = &m_seTaskId;
        kind = 3;
        break;
    case 0xf:
        m_mstVolStr[0] = a;
        m_mstVolStr[1] = b;
        g_gamMan.setOption(4, (intptr_t)&level);
        level = gamSndLevelVolume(level);
        fade.m_volR = (int)((float)b * (float)level * 0.0078125f);
        fade.m_volL = (int)((float)a * (float)level * 0.0078125f);
        if (fade.m_volR < 0xb)
            fade.m_volR = 10;
        if (fade.m_volL < 0xb)
            fade.m_volL = 10;
        if (c == 0)
        {
            rxSndAdpcmSetMasterVolume(fade.m_volL, fade.m_volR);
            return;
        }
        fade.m_curL = m_mstVolStr[0];
        fade.m_curR = m_mstVolStr[1];
        taskId = &m_strTaskId;
        kind = 4;
        break;
    case 0x10:
        g_sndMan.stopAll(SND_TYPE_STREAM | SND_TYPE_SE);
        clear_snd_lst(0);
        return;
    case 0x11:
        g_sndMan.stopAll(SND_TYPE_SE);
        clear_snd_lst(1);
        return;
    case 0x12:
        g_sndMan.stopAll(SND_TYPE_STREAM);
        clear_snd_lst(2);
        return;
    case 0x19:
        g_sndMan.stopAll(SND_TYPE_STREAM | SND_TYPE_SE);
        g_sndMan.freeBank(d);
        return;
    case 0x1c:
        if (a < 0)
            a = 0;
        else if (a > 100)
            a = 100;
        m_3dScale = (float)a / 100.0f;
        return;
    case 0x1d:
        pushSnd(a);
        return;
    case 0x1e:
        resumeBgm(1, a, 0);
        return;
    case 0x1f:
        pushSnd(a);
        m_flags |= 0x400;
        return;
    case 0x20:
        resumeBgm(1, a, 0);
        m_flags &= ~0x400u;
        return;
    default:
        return;
    }

    if (g_taskMan.getTask(*taskId) != 0)
    {
        g_taskMan.killTask(*taskId);
        *taskId = -1;
    }
    TskSnd *task = new TskSnd(kind, &fade);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, taskId, 0x11);
}

// ──────────────────────────────────────────────────────────────────────
// How loud ear side's (an angle of +-60 degrees off the listener's facing,
// rotation rot) hears the sound at pos from eye: 0.3 behind .. 1.0 ahead.
// ROR 0x001e6d54

float gamSndEarGain(const GamSndEar *ear)
{
    sceVu0FVECTOR mat[4];
    sceVu0FVECTOR dir;
    i3dMatUnit(mat);
    i3dMatRotY(ear->m_side, mat, mat);
    i3dMatRotZXY(mat, mat, ear->m_rot);
    i3dVecSub(dir, ear->m_pos, ear->m_eye);
    i3dVecNormalize2(dir, dir);
    float dot = i3dVecDot(mat[2], dir);
    return (dot + 1.0f) * 0.5f * 0.7f + 0x1.333332p-2f;   /* 0x3e999999 */
}

// ──────────────────────────────────────────────────────────────────────
// The entry's distance fade (m_nearVol up to m_near, m_farVol from m_far,
// linear between) times each ear's gain.
// ROR 0x001e6e00

void gamSnd3dVolume(int mode, const float *pos, const GamSndEntry *entry, int *volL,
                    int *volR, float scale)
{
    (void)scale;
    sceVu0FVECTOR eye;
    sceVu0FVECTOR rot;
    memset(eye, 0, sizeof(eye));
    eye[3] = 1.0f;
    memset(rot, 0, sizeof(rot));

    float nearDist = (float)entry->m_near;
    float farVol = (float)entry->m_farVol;
    float nearVol = (float)entry->m_nearVol;
    float farDist = (float)entry->m_far;

    if (mode == 0)
    {
        eye[0] = g_gamMan.m_pcPos[0];
        eye[1] = g_gamMan.m_pcPos[1];
        eye[2] = g_gamMan.m_pcPos[2];
        rot[0] = g_gamMan.m_pcRot[0];
        rot[1] = g_gamMan.m_pcRot[1];
        rot[2] = g_gamMan.m_pcRot[2];
    }
    else if (mode == 1)
    {
        const DogCamera &cam = g_magSystem.m_scene.m_camera_entity;
        eye[0] = cam.m_trans[0];
        eye[1] = cam.m_trans[1];
        eye[2] = cam.m_trans[2];
        rot[0] = cam.m_rotate[0];
        rot[1] = cam.m_rotate[1];
        rot[2] = cam.m_rotate[2];
    }
    else
    {
        *volR = 0x40;
        *volL = 0x40;
        return;
    }

    sceVu0FVECTOR d;
    i3dVecSub(d, pos, eye);
    float dist = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);

    float vol = farVol;
    if (!(farDist <= dist))
    {
        vol = nearVol;
        if (!(dist <= nearDist))
            vol = farVol + ((nearVol - farVol) * (dist - farDist)) / (nearDist - farDist);
    }

    if (vol <= 0.0f)
    {
        *volR = 0;
        *volL = 0;
        return;
    }

    GamSndEar ear = { eye, rot, -1.0471975f, pos };
    *volL = (int)(vol * gamSndEarGain(&ear));
    ear.m_side = 1.0471975f;
    *volR = (int)(vol * gamSndEarGain(&ear));
}

// ──────────────────────────────────────────────────────────────────────
// Level 0 of the push levels from GamMan's record (GamMan::onSubSeqEnd,
// before resumeBgm(1, 0, 0)).
// ROR 0x001e15d0

void GamSndMan::setBgmRec(const GamBgmRec *rec)
{
    m_pushDepth = 0;
    m_bgm[0] = *rec;
}

// ──────────────────────────────────────────────────────────────────────
// Drops sounds from the pushed level (back to level 0) so resumeBgm does
// not bring them back: the stream str (-1 any) unless mode 1, the SE se (0
// any) unless mode 2.
// ROR 0x001e1620

void GamSndMan::unpush_snd(int mode, u_int se, int str)
{
    if (m_pushDepth < 0)
        return;
    m_pushDepth = 0;

    if (mode != 1 && (str == -1 || str == m_bgm[0].m_str))
    {
        m_bgm[0].m_str = -1;
        m_bgm[m_pushDepth].m_strVolL = 0;
        m_bgm[m_pushDepth].m_strVolR = 0;
    }
    if (mode == 2)
        return;

    if (se == 0)
    {
        m_bgm[m_pushDepth].m_se = 0;
        m_bgm[m_pushDepth].m_seVolL = 0;
        m_bgm[m_pushDepth].m_seVolR = 0;
        m_pushSe[m_pushDepth].m_se = 0;
    }
    else
    {
        if (se == m_bgm[m_pushDepth].m_se)
        {
            m_bgm[m_pushDepth].m_se = 0;
            m_bgm[m_pushDepth].m_seVolL = 0;
            m_bgm[m_pushDepth].m_seVolR = 0;
        }
        if (se != m_pushSe[m_pushDepth].m_se)
            return;
        m_pushSe[m_pushDepth].m_se = 0;
    }
    m_pushSe[m_pushDepth].m_volL = 0;
    m_pushSe[m_pushDepth].m_volR = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e17b4

void GamSndMan::stopBgm(int mode)
{
    TskSndParam fade = {};     /* PC: zeroed (the ROM's stack words the fade does not set) */

    pushSnd(0);
    int str = m_bgm[m_pushDepth].m_str;
    if (str != -1 && mode != 1)
    {
        fade.m_frames = 0x1e;
        fade.m_volL = 0;
        fade.m_volR = 0;
        funcStr(str, 0x1b, &fade);
    }
    if (mode != 2)
    {
        u_int se = m_bgm[m_pushDepth].m_se;
        if (se != 0)
        {
            fade.m_frames = 0x1e;
            fade.m_volL = 0;
            fade.m_volR = 0;
            funcSE(se, 0x13, &fade);
        }
        se = m_pushSe[m_pushDepth].m_se;
        if (se != 0)
        {
            fade.m_frames = 0x1e;
            fade.m_volL = 0;
            fade.m_volR = 0;
            funcSE(se, 0x14, &fade);
        }
    }
    for (int i = 0; i < m_loopSeCount; i++)
    {
        fade.m_frames = 0x1e;
        fade.m_volL = 0;
        fade.m_volR = 0;
        funcSE(m_loopSe[i].m_se, 0x15, &fade);
    }
    fadeOut3dSnds();
    m_flags |= 1;
}

// ──────────────────────────────────────────────────────────────────────
// A stage reset: no push levels, every record free, the three volume
// fades killed, the record range set again and the used 3D nodes handed
// back to the free list.
// ROR 0x001e1fa4

void GamSndMan::reset()
{
    m_pushDepth = -1;
    m_loopSeCount = 0;
    for (int i = 0; i < 4; i++)
    {
        m_bgm[i].m_str = -1;
        m_bgm[i].m_se = 0;
    }
    for (int i = 0; i < GAMSND_REC_MAX; i++)
    {
        m_rec[i].m_flags = 0;
        m_rec[i].m_se = 0;
        m_rec[i].m_slot = -1;
        m_rec[i].m_volL = 0;
        m_rec[i].m_volR = 0;
    }

    if (g_taskMan.getTask(m_mstTaskId) != 0)
        g_taskMan.killTask(m_mstTaskId);
    if (g_taskMan.getTask(m_seTaskId) != 0)
        g_taskMan.killTask(m_seTaskId);
    if (g_taskMan.getTask(m_strTaskId) != 0)
        g_taskMan.killTask(m_strTaskId);
    m_strTaskId = -1;
    m_mstTaskId = -1;
    m_seTaskId = -1;

    m_recEnd = m_recBuf;
    m_recMax = GAMSND_REC_MAX;
    m_rec = m_recBuf;

    if (m_used.m_count != 0)
    {
        GamSnd3dNode *freeLast = m_free.m_head.m_prev;
        freeLast->m_next = m_used.m_head.m_next;
        freeLast->m_next->m_prev = freeLast;
        m_free.m_head.m_prev = m_used.m_head.m_prev;
        m_used.m_head.m_prev->m_next = &m_free.m_head;
        m_free.m_count += m_used.m_count;
        m_used.m_head.m_next = &m_used.m_head;
        m_used.m_count = 0;
        m_used.m_head.m_prev = &m_used.m_head;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e4120 // callcheck: operator_new (new TskSnd)

void GamSndMan::fadeOut3dSnds()
{
    TskSndParam fade = {};     /* PC: zeroed (the ROM's stack words the fade does not set) */

    for (GamSnd3dNode *node = m_used.m_head.m_next; node != &m_used.m_head; node = node->m_next)
    {
        int track = node->m_rec;
        if ((node->m_flags & 1) == 0)
        {
            if (!getStatSEtrack(node->m_snd, track))
                continue;
            GamSndRec *rec = &m_rec[track];
            fade.m_snd = node->m_snd;
            fade.m_id = rec->m_slot;
            fade.m_curL = rec->m_volL;
            fade.m_curR = rec->m_volR;
            fade.m_frames = 0x3c;
            fade.m_volL = 0;
            fade.m_volR = 0;
            fade.m_arg = 0;
            TskSnd *task = new TskSnd(0, &fade);
            g_taskMan.addTask(1, -1, 1, 0, task, 0, NULL, 0x11);
        }
        else
        {
            if (!getStatSTRtrack(node->m_snd, track))
                continue;
            GamSndRec *rec = &m_rec[track];
            fade.m_snd = node->m_snd;
            fade.m_id = rec->m_slot;
            fade.m_curL = rec->m_volL;
            fade.m_curR = rec->m_volR;
            fade.m_frames = 0x1e;
            fade.m_volL = 0;
            fade.m_volR = 0;
            m_bgm[m_pushDepth].m_str2 = node->m_snd;
            m_bgm[m_pushDepth].m_str2VolL = rec->m_volL;
            m_bgm[m_pushDepth].m_str2VolR = rec->m_volR;
            funcStr(fade.m_snd, 0x15, &fade);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e4700

void GamSndMan::free_all_se_bank()
{
    g_sndMan.stopAll(SND_TYPE_SE);
    freeSePack(0);
    freeSePack(10);
    freeSePack(0xb);
    freeSePack(0xc);
    freeSePack(0x1f);
    freeSePack(0x20);
    freeSePack(0x21);
    freeSePack(0x40);
}

// ──────────────────────────────────────────────────────────────────────
// The SE id of name (a leading '.' or '*' skipped), 0 when none; the table
// ends at id -1 here.
// ROR 0x001e4a0c

int GamSndMan::getSeID(const char *name)
{
    GamSndEntry *e = m_table;
    if (*name == '.' || *name == '*')
        name++;
    if (e == NULL)
        return 0;
    for (int n = 0; n < GAMSND_SE_MAX && e[n].m_id != -1; n++)
        if (strcmp(name, e[n].m_name) == 0)
            return e[n].m_id;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// The stream number of name, 0 when none.
// ROR 0x001e4ab8

int GamSndMan::getStrNO(const char *name)
{
    GamSndEntry *e = m_strTable;
    if (e == NULL)
        return 0;
    for (int n = 0; n < GAMSND_STR_MAX && e[n].m_id != -1; n++)
        if (strcmp(name, e[n].m_name) == 0)
            return e[n].m_id;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e4d18

void GamSndMan::stopSE(int mode)
{
    if (mode != 1)
        return;
    for (int i = 0; i < GAMSND_REC_MAX; i++)
    {
        GamSndRec *rec = &m_rec[i];
        if ((rec->m_se >> 24) != 0xb)
            stopSeTrack(rec->m_se, rec->m_slot);
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001e649c

int GamSndMan::stopSeTrack(u_int se, int track)
{
    if (track < 0 || m_rec[track].m_se != se)
        return 0;
    if (g_sndMan.getSeState(track) == -1)
        return 0;
    g_sndMan.stopSe(track);
    m_rec[track].m_volL = 0;
    m_rec[track].m_volR = 0;
    m_rec[track].m_se = 0;
    m_rec[track].m_slot = -1;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// 1 while a GAMSND_REC_8 stream (playStr's bool) plays.
// ROR 0x001e6cac

int GamSndMan::isPlayEnvStr()
{
    for (int i = 0; i < GAMSND_REC_MAX; i++)
    {
        GamSndRec *rec = &m_rec[i];
        if ((rec->m_flags & GAMSND_REC_STR) != 0 && (rec->m_flags & GAMSND_REC_8) != 0
            && rec->m_str != -1 && g_sndMan.getStreamState(rec->m_slot) != -1)
            return 1;
    }
    return 0;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x001e3698
void GamSndMan::del_str_3d_pos_chr(u_int oid)
{
    ROSEPAN_STUB("GamSndMan::del_str_3d_pos_chr", 0x001e3698);
}

// stub of ROR 0x001e2c40
void GamSndMan::del_se_3d_pos_chr(u_int oid)
{
    ROSEPAN_STUB("GamSndMan::del_se_3d_pos_chr", 0x001e2c40);
}

// stub of ROR 0x001e2aa0
void GamSndMan::del_se_3d_pos(u_int se, int arg)
{
    ROSEPAN_STUB("GamSndMan::del_se_3d_pos", 0x001e2aa0);
}

// stub of ROR 0x001e34ec
void GamSndMan::del_str_3d_pos(int str, int arg)
{
    ROSEPAN_STUB("GamSndMan::del_str_3d_pos", 0x001e34ec);
}

// stub of ROR 0x001e5be0
int GamSndMan::getStrNoByChan(int chan)
{
    ROSEPAN_STUB("GamSndMan::getStrNoByChan", 0x001e5be0);
    return {};
}

// stub of ROR 0x001e50c8
int GamSndMan::playSE(u_int se, int a, int b, float c, int d, int e, int arg)
{
    ROSEPAN_STUB("GamSndMan::playSE", 0x001e50c8);
    return {};
}

// stub of ROR 0x001e5428
int GamSndMan::playStr(int str, int a, int b, float c, int d, int e, int arg)
{
    ROSEPAN_STUB("GamSndMan::playStr", 0x001e5428);
    return {};
}

// stub of ROR 0x001e239c
void GamSndMan::put_se_3d_pos(u_int se, float *pos, int wrd, u_int flags)
{
    ROSEPAN_STUB("GamSndMan::put_se_3d_pos", 0x001e239c);
}

// stub of ROR 0x001e2704
void GamSndMan::put_se_3d_pos_chr(u_int se, u_int oid, u_int flags)
{
    ROSEPAN_STUB("GamSndMan::put_se_3d_pos_chr", 0x001e2704);
}

// stub of ROR 0x001e2db4
void GamSndMan::put_str_3d_pos(int str, float *pos, int wrd, u_int flags)
{
    ROSEPAN_STUB("GamSndMan::put_str_3d_pos", 0x001e2db4);
}

// stub of ROR 0x001e3138
void GamSndMan::put_str_3d_pos_chr(int str, u_int oid, u_int flags)
{
    ROSEPAN_STUB("GamSndMan::put_str_3d_pos_chr", 0x001e3138);
}

// stub of ROR 0x001e4cc4
void GamSndMan::setReverb(int a, int b, int c)
{
    ROSEPAN_STUB("GamSndMan::setReverb", 0x001e4cc4);
}

// stub of ROR 0x001e5de4
void GamSndMan::set_se_3d_snd_prm(u_int se, int a, int b, int c, int d)
{
    ROSEPAN_STUB("GamSndMan::set_se_3d_snd_prm", 0x001e5de4);
}

// stub of ROR 0x001e5540
void GamSndMan::set_str_3d_snd_prm(int str, int a, int b, int c, int d)
{
    ROSEPAN_STUB("GamSndMan::set_str_3d_snd_prm", 0x001e5540);
}

// stub of ROR 0x001e4540
void GamSndMan::loadSePack(const char *name, int arg)
{
    ROSEPAN_STUB("GamSndMan::loadSePack", 0x001e4540);
}

// stub of ROR 0x001e2178
void GamSndMan::stopObjSnds()
{
    ROSEPAN_STUB("GamSndMan::stopObjSnds", 0x001e2178);
}

// stub of ROR 0x001e63c0
int GamSndMan::setVolSEtrack(u_int se, int track, int a, int b)
{
    ROSEPAN_STUB("GamSndMan::setVolSEtrack", 0x001e63c0);
    return {};
}

// stub of ROR 0x001e5c40
int GamSndMan::setVolSTRtrack(u_int str, int track, int a, int b)
{
    ROSEPAN_STUB("GamSndMan::setVolSTRtrack", 0x001e5c40);
    return {};
}

// stub of ROR 0x001e3c24
void GamSndMan::update3d(const float *pos)
{
    ROSEPAN_STUB("GamSndMan::update3d", 0x001e3c24);
}
