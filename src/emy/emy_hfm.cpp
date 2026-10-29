/* ==========================================================================
 *  emy/emy_hfm.cpp
 *
 *  HfmMain (enemy type 6) and its outs (0x145020..0x149d18). HfmMain is the
 *  usual enemy loop: decide picks an input of the transition table (only
 *  inputs the current state's row has), exec moves the state graph by it,
 *  after switches outs on a change and runs the current one. The outs do
 *  the state's work and hand the next input back from Exec.
 * ======================================================================== */
#include "emy/emy_hfm.h"

#include "common/rosepan_stub.h"

#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magscenecmd.h"
#include "mag/magsystem.h"
#include "snd/sndman.h"
#include "task/tsk_chr.h"
#include "task/tsk_scene.h"

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145020 // callcheck: operator_new, EmyOut (new HfmOutXxx: the inline constructors call EmyOut::EmyOut)

void HfmMain::initOut()
{
    m_motion_list[0]  = 1;
    m_motion_list[1]  = 1;
    m_motion_list[2]  = 3;
    m_motion_list[3]  = 7;
    m_motion_list[4]  = 3;
    m_motion_list[5]  = 3;
    m_motion_list[7]  = 0x14;
    m_motion_list[8]  = 1;
    m_motion_list[9]  = 0xd;
    m_motion_list[10] = 0x32;
    m_state_flg[1]  = 1;
    m_state_flg[2]  = 1;
    m_state_flg[3]  = 1;
    m_state_flg[4]  = 0;
    m_state_flg[5]  = 1;
    m_state_flg[6]  = 1;
    m_state_flg[7]  = 0;
    m_state_flg[8]  = 0;
    m_state_flg[9]  = 0;
    m_pc_seen  = 0;
    m_229      = 0;
    m_slap_try = 0;

    m_out[1] = new HfmOutWalk(m_oid, m_param, this);
    m_out[2] = new HfmOutSlap(m_oid, m_param, this);
    m_out[3] = new HfmOutBitten(m_oid, m_param, this);
    m_out[5] = new HfmOutCounter(m_oid, m_param, this);
    m_out[4] = new HfmOutBreak(m_oid, m_param, this);
    m_out[6] = new HfmOutRide(m_oid, m_param, this);
    m_out[7] = new HfmOutDead(m_oid, m_param, this);
    m_out[8] = new HfmOutWin(m_oid, m_param, this);
    m_out[9] = new HfmOutTest(m_oid, m_param, this);

    int col[12];
    m_param->getParam(GAM_PRM_COL_PRM, col);
    col[1] = 2;
    m_param->setParam(GAM_PRM_COL_PRM, col);

    m_free_cnt = 0;
    m_238      = 0;
    m_free_max = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014529c // callcheck: ~EmyMain, operator_delete (the base destructor and the deleting variant are the compiler's)

HfmMain::~HfmMain()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001452fc

void HfmMain::initAfterParam()
{
    u_int item;
    m_param->getParam(0x67, &item);
    if (item == 0)
        return;

    u_int kind = 0;
    m_param->getParam(0x66, &kind);
    short model;
    g_gamMan.m_sysData.getField((u_short)kind, 0xf, &model);
    if ((u_short)model == 0xffff)
        g_gamMan.addChr(-1, item, (u_short)kind, NULL, 0);
    else
        g_gamMan.createObjByModel((u_short)model, item, (u_short)kind, NULL, -1, 1);
    emyAttachItem(m_oid, item);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001453c0

void HfmMain::decide()
{
    /* The slap chance (%) by try. */
    static const int s_slapChance[6] = { 0, 10, 30, 50, 70, 100 };

    if (m_decide_sw == 0)
        return;
    if (m_req != -1)
        return;

    if (m_input[1] && magBwnBarkQuery(m_oid) != -1) {
        m_req = 1;
        m_free_left = utlRandRange(60, 120);
        return;
    }
    if (m_input[2] && (m_input_flg_motion & 0x11) == 1) {
        m_slap_try++;
        if (utlRandRange(0, 99) < s_slapChance[m_slap_try]) {
            m_slap_try = 0;
            m_req = 2;
            return;
        }
    }
    if (m_input_flg_motion == 0)
        return;

    int seen = isPCInSight();
    if (!m_pc_seen) {
        if (seen) {
            m_pc_seen = 1;
            if (m_input[3]) {
                m_req = 3;
                return;
            }
        }
    } else if (!seen) {
        m_pc_seen = 0;
        if (m_input[4]) {
            m_req = 4;
            return;
        }
    }
    if (m_input[5] && isLowLife()) {
        m_req = 5;
        return;
    }
    if (m_input[6] && isPCStateClass4())
        m_req = 6;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145560

void HfmMain::exec()
{
    m_changed = 0;
    if (m_notify & 2) {
        m_state = 7;
        m_map->m_cur = 7;
        m_changed = 1;
    } else if (m_req != -1) {
        int state = m_map->next(m_req);
        m_pc_seen = 0;
        if (state == 0) {
            m_map->m_cur = m_state;
        } else {
            m_state = state;
            m_changed = 1;
        }
    } else if (m_notify & 1) {
        m_map->m_cur = m_state;
        m_changed = 1;
    }
    m_notify = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145604

void HfmMain::after()
{
    EmyOut *out = m_emy_out;
    if (m_changed) {
        out->Term();
        selectOut();
        out = m_emy_out;
    }
    m_req = out->Exec();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145680
// The ROM returns 0 (the slot's value is never read).

void HfmMain::selectOut()
{
    if (m_state >= 1 && m_state < 10) {
        m_emy_out = m_out[m_state];
        m_emy_out->Init();
    }
    int n = m_trans->m_inputs;
    const char *row = m_trans->m_table + m_state * n;
    // PORT: the ROM writes as many words as the table has inputs; the
    // host array holds 8.
    if (n > 8)
        n = 8;
    for (int i = 0; i < n; i++)
        m_input[i] = row[i] != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145744

bool HfmMain::isPCSameArea()
{
    return EmyMain::isPCSameArea();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145760

bool HfmMain::isPCStateClass4()
{
    return g_magSystem.getPCStateClass() == 4;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145788

int HfmMain::isPCInSight()
{
    float range;
    m_param->getParam(0x5f, &range);
    ObjStat *pcObj = g_gamMan.getPCPtr();
    u_int pc = 0;
    if (pcObj != NULL)
        pc = pcObj->m_oid;
    return emyChkInSight(range, 0x1.921fb4p+1f /* 0x40490fda, pi */, 1, pc, m_param);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001457e8

int HfmMain::virtual7()
{
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001457f0

bool HfmMain::countFree()
{
    return m_free_max < ++m_free_cnt;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145808

void HfmMain::startFree(int max)
{
    m_free_max = max;
    m_free_cnt = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145814

bool HfmMain::tickFreeLeft()
{
    return --m_free_left >= 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014582c

void HfmMain::subFreeCnt(int n)
{
    n = m_free_left - n;
    if (n < 0)
        n = 0;
    m_free_left = n;
}

// ──── HfmOutWalk ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145844

void HfmOutWalk::Init()
{
    selectTarget(100);
    setMotion(0xc, 0);
    m_step = 0;
    setStep(3);
    m_emy->m_state2 = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145894
// Steps: 0 choose (roam to a point the PC cannot see, or idle), 1 moving,
// 2 walking in place for 30 frames, 3 until the motion ends.

int HfmOutWalk::Exec()
{
    int cnt = m_cnt++;
    switch (m_step) {
    case 0:
    {
        int r = utlRandRange(1, 95);
        if (r <= 80) {
            float pos[4];
            getRoamPos(pos);
            ObjStat *pcObj = g_gamMan.getPCPtr();
            u_int pc = 0;
            if (pcObj != NULL)
                pc = pcObj->m_oid;
            if (!emyChkInSight(0x1.cbfffep+6f /* 0x42e5ffff, 115 */, 0x1.921fb4p+2f /* 0x40c90fda, 2 pi */,
                               0, pc, m_achr)) {
                emyStartMove(0.0f, this, pos);
                setStep(1);
                return -1;
            }
        }
        startIdle(r);
        return -1;
    }
    case 1:
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        break;
    case 2:
        if (cnt + 1 < 30)
            return -1;
        break;
    case 3:
        if (!g_gamMan.isAchrMotionEnd(m_oid))
            return -1;
        m_emy->m_input_flg_motion = 1;
        break;
    default:
        return -1;
    }
    setStep(0);
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145a10
// Tries the random offset at four scales, each in the four quadrants
// (+x +z, -x +z, -x -z, +x -z).

void HfmOutWalk::getRoamPos(float *pos)
{
    static const float s_scale[4] = {                                    // data 0x002ef328
        1.0f, 0x1.666666p-1f /* 0.7 */, 0x1.999998p-2f /* 0.4 */, 0x1.999998p-4f /* 0.1 */,
    };
    float from[4];
    float home[4];
    float p[4];

    getBasePos(from);
    m_achr->getParam(0x59, home);
    float dx = utlRandRangeF(-0x1.1f7ffep+9f, 0x1.1f7ffep+9f);           // +-575 (0x440fbfff)
    float dz = utlRandRangeF(-0x1.1f7ffep+9f, 0x1.1f7ffep+9f);
    for (int i = 0; i < 4; i++) {
        float ox = dx * s_scale[i];
        float oz = dz * s_scale[i];
        for (int j = 0; j < 4; j++)
            p[j] = home[j];
        p[0] = home[0] + ox;
        p[2] = home[2] + oz;
        if (emyCanReach(from, p))
            goto found;
        p[0] = home[0] - ox;
        if (emyCanReach(from, p))
            goto found;
        p[2] = home[2] - oz;
        if (emyCanReach(from, p))
            goto found;
        p[0] = home[0] + ox;
        if (emyCanReach(from, p))
            goto found;
    }
    for (int j = 0; j < 4; j++)
        pos[j] = home[j];
    return;

found:
    for (int j = 0; j < 4; j++)
        pos[j] = p[j];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145cc8

void HfmOutWalk::startIdle(int r)
{
    if (r < 91) {
        setWalkMotion();
        setStep(2);
        return;
    }
    setMotion(0xc, 0);
    setStep(3);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145d28

void HfmOutWalk::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145d44

void HfmOutWalk::setStep(int step)
{
    if (m_step != step) {
        m_step = step;
        m_cnt = 0;
    }
}

// ──── HfmOutSlap ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145d60

void HfmOutSlap::Init()
{
    m_chase_cnt = 45;
    m_combo     = 0;
    m_close     = 0;
    m_motion    = 0;
    m_cnt       = 0;
    selectTarget(100);
    emyStartChase(10.0f, this, m_target);
    m_emy->m_state2 = 0;
    m_step = 0;
}

// ──────────────────────────────────────────────────────────────────────
// The scene request word of a slap that hit: 0x28 the close one, 0x27 a
// chained one, 0x26 the first (inline in the ROM's Exec).

static int hfmSlapWord(const HfmOutSlap *out)
{
    if (out->m_close)
        return 0x28;
    return out->m_combo ? 0x27 : 0x26;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00145dc0
// Steps: 0 chase the target (8 when the move gives up), 1 the slap, 2 / 3
// until its motion ends (2 may chain another slap), then 7 or a new chase.

int HfmOutSlap::Exec()
{
    switch (m_step) {
    case 0:
        m_chase_cnt++;
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        if (m_move_done != 1)
            return 8;
        m_se_done = 0;
        if (m_chase_cnt < 46) {
            m_close    = 1;
            m_motion   = 0x16;
            m_hit_from = 0x4b;
            m_hit_to   = 0x4e;
            m_se_frame = 0x46;
            m_reach    = 105.0f;
        } else {
            m_close = 0;
            if ((m_combo == 0 || m_motion != 0x15) && utlRandRange(0, 99) > 49) {
                m_motion   = 0x15;
                m_hit_from = 8;
                m_hit_to   = 0xb;
                m_reach    = 85.0f;
            } else {
                m_motion   = 0x14;
                m_hit_from = 0xe;
                m_hit_to   = 0x11;
                m_reach    = 90.0f;
            }
            m_se_frame = 0;
        }
        playSE(0x13);
        setMotion(m_motion, 0);
        m_chase_cnt = 0;
        m_hit_dog   = 0;
        m_step      = 1;
        m_hit_pc    = 0;
        return -1;

    case 1:
    {
        int end = 0;            // PORT: the ROM leaves it unset for other motions (none are used)
        m_cnt++;
        if (m_motion == 0x14)
            end = 0xd;
        else if (m_motion == 0x16)
            end = 0x3a;
        if (end < m_cnt) {
            m_emy->setDecideSw(0);
            g_gamMan.setObj(m_oid, 0x400);
        }
        if (m_se_frame < m_cnt && !m_se_done) {
            playSE(0x38);
            m_se_done = 1;
        }
        if (m_hit_from <= m_cnt && m_cnt <= m_hit_to) {
            if (!m_hit_pc && g_magSystem.getPCStateClass() != 9) {
                m_hit_pc = 1;
                ObjStat *pcObj = g_gamMan.m_pcPtr;
                u_int pc = 0;
                if (pcObj != NULL)
                    pc = pcObj->m_oid;
                if (emyChkHitReach(m_reach, m_oid, 1, pc)) {
                    int word = hfmSlapWord(this);
                    playSE(0x10);
                    scnChrRequest(0xb, m_oid, 0, 0, word, pc, 0);
                }
            }
            if (!m_hit_dog && g_magSystem.getBwnState() != 10 && g_magSystem.getBwnState() != 0xd) {
                m_hit_dog = 1;
                ObjStat *dogObj = g_gamMan.m_dogPtr;
                u_int dog = 0;
                if (dogObj != NULL)
                    dog = dogObj->m_oid;
                if (emyChkHitReach(m_reach, m_oid, 1, dog)) {
                    int word = hfmSlapWord(this);
                    playSE(0x10);
                    scnChrRequest(0xb, m_oid, 0, 0, word, dog, 0);
                }
            }
        }
        if (m_cnt == m_hit_to) {
            m_emy->setDecideSw(1);
            g_gamMan.resetObj(m_oid, 0x400);
            m_cnt  = 0;
            m_step = m_close ? 3 : 2;
        }
        return -1;
    }

    case 2:
        if (!g_gamMan.isAchrMotionEnd(m_oid))
            return -1;
        m_emy->m_input_flg_motion = 1;
        if (emyChkInSight(0x1.6ffffep+6f /* 0x42b7ffff, 92 */, 0x1.0c1522p+0f /* 0x3f860a91, pi / 3 */,
                          1, m_target, m_achr)
            && utlRandRange(0, 99) < 50) {
            /* Chain the other slap. The ROM keeps m_hit_pc / m_hit_dog /
             * m_se_done from the last one, so a chained slap neither
             * tests a hit nor plays SE 0x38 again. */
            m_combo++;
            if (m_motion == 0x15) {
                m_motion   = 0x14;
                m_hit_from = 0xe;
                m_hit_to   = 0x11;
                m_reach    = 90.0f;
            } else {
                m_motion   = 0x15;
                m_hit_from = 8;
                m_hit_to   = 0xb;
                m_reach    = 85.0f;
            }
            playSE(0x13);
            playSE(0x38);
            setMotion(m_motion, 0);
            m_chase_cnt = 0;
            m_step      = 1;
            return -1;
        }
        m_combo = 0;
        break;

    case 3:
        if (!g_gamMan.isAchrMotionEnd(m_oid))
            return -1;
        m_emy->m_input_flg_motion = 1;
        break;

    default:
        return -1;
    }

    if (utlRandRange(0, 99) < 50)
        return 7;
    emyStartChase(10.0f, this, m_target);
    m_step = 0;
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014635c

void HfmOutSlap::Term()
{
    termCommon();
}

// ──── HfmOutBreak ─────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00146378

void HfmOutBreak::Init()
{
    selectTarget(100);
    m_step = 0;
    m_emy->setDecideSw(0);
    m_emy->m_state2 = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001463b8

int HfmOutBreak::Exec()
{
    if (m_step == 0) {
        setMotion(0x30, 0);
        playSE(0x2a);
        m_step = 1;
    } else if (m_step == 1) {
        if (!g_gamMan.isAchrMotionEnd(m_oid))
            return -1;
        m_emy->m_input_flg_motion = 1;
        m_emy->setDecideSw(1);
        return 7;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00146460

void HfmOutBreak::Term()
{
    termCommon();
}

// ──── HfmOutWalk2 (vtable only: initOut never builds it) ──────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014647c

void HfmOutWalk2::Init()
{
    selectTarget(100);
    setMotion(0xc, 0);
    m_step = 0;
    setStep(3);
    m_emy->m_state2 = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001464cc
// HfmOutWalk::Exec, but step 0 roams when the point lies 115 or more from
// the world origin (the ROM measures the point itself, not a distance).

int HfmOutWalk2::Exec()
{
    int cnt = m_cnt++;
    switch (m_step) {
    case 0:
    {
        int r = utlRandRange(1, 95);
        if (r <= 80) {
            float pos[4];
            getRoamPos(pos);
            if (emyVecLength(pos) >= 0x1.cbfffep+6f /* 0x42e5ffff, 115 */) {
                emyStartMove(0.0f, this, pos);
                setStep(1);
                return -1;
            }
        }
        startIdle(r);
        return -1;
    }
    case 1:
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        break;
    case 2:
        if (cnt + 1 < 30)
            return -1;
        break;
    case 3:
        if (!g_gamMan.isAchrMotionEnd(m_oid))
            return -1;
        m_emy->m_input_flg_motion = 1;
        break;
    default:
        return -1;
    }
    setStep(0);
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00146628
// HfmOutWalk::getRoamPos's code (its own copy in the ROM).

void HfmOutWalk2::getRoamPos(float *pos)
{
    static const float s_scale[4] = {
        1.0f, 0x1.666666p-1f /* 0.7 */, 0x1.999998p-2f /* 0.4 */, 0x1.999998p-4f /* 0.1 */,
    };
    float from[4];
    float home[4];
    float p[4];

    getBasePos(from);
    m_achr->getParam(0x59, home);
    float dx = utlRandRangeF(-0x1.1f7ffep+9f, 0x1.1f7ffep+9f);           // +-575 (0x440fbfff)
    float dz = utlRandRangeF(-0x1.1f7ffep+9f, 0x1.1f7ffep+9f);
    for (int i = 0; i < 4; i++) {
        float ox = dx * s_scale[i];
        float oz = dz * s_scale[i];
        for (int j = 0; j < 4; j++)
            p[j] = home[j];
        p[0] = home[0] + ox;
        p[2] = home[2] + oz;
        if (emyCanReach(from, p))
            goto found;
        p[0] = home[0] - ox;
        if (emyCanReach(from, p))
            goto found;
        p[2] = home[2] - oz;
        if (emyCanReach(from, p))
            goto found;
        p[0] = home[0] + ox;
        if (emyCanReach(from, p))
            goto found;
    }
    for (int j = 0; j < 4; j++)
        pos[j] = home[j];
    return;

found:
    for (int j = 0; j < 4; j++)
        pos[j] = p[j];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001468e0

void HfmOutWalk2::startIdle(int r)
{
    if (r < 91) {
        setWalkMotion(1);
        setStep(2);
        return;
    }
    setMotion(0xc, 0);
    setStep(3);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00146944
// The ROM also passes 32768.5 (0x47000100) in f13 and 1 in t0, which
// chrMtnMove2Achr does not read.

void HfmOutWalk2::setWalkMotion(int motion)
{
    float agl = m_emy->m_agl;
    float speed = agl == 0.0f ? 0x1.111110p-6f /* 0x3c888888, 1 / 60 */ : agl * 0x1.111110p-6f;
    chrMtnMove2Achr(speed, m_oid, motion, -1, 0x100);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001469b0

void HfmOutWalk2::Term()
{
    termCommon();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001469cc

void HfmOutWalk2::setStep(int step)
{
    if (m_step != step) {
        m_step = step;
        m_cnt = 0;
    }
}

// ──── HfmOutRide ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001469e8

void HfmOutRide::Init()
{
    selectTarget(100);
    m_emy->m_state2 = 0;
    start();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00146a20

int HfmOutRide::Exec()
{
    switch (m_phase) {
    case 0:
        return execPhase0();
    case 1:
        return execPhase1();
    case 2:
        return execPhase2();
    case 3:
        return execPhase3();
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00146ab8

void HfmOutRide::Term()
{
    termCommon();
    m_emy->m_238 = 0;           // SetMissing(false)
}

// ──── HfmOutDead ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148d30
// Effect kind 3 at the enemy, hidden and out of the hit tests, the death
// motion 0x31, its sounds stopped.

void HfmOutDead::Init()
{
    float pos[3];
    m_achr->getParam(5, pos);
    EffParam eff = {};          // PORT: the ROM leaves the words it does not set as they were
    eff.m_kind = 3;
    eff.m_08   = 0;
    eff.m_hit.m_pos[0] = pos[0];
    eff.m_hit.m_pos[1] = pos[1];
    eff.m_hit.m_pos[2] = pos[2];
    eff.m_hit.m_1c    = 0;
    eff.m_hit.m_20    = -1.0f;
    eff.m_hit.m_24    = 0;
    eff.m_hit.m_scale = 100.0f;
    eff.m_hit.m_2c    = 0x5a;
    g_magSystem.m_scene.m_eff_man.createEff(&eff);
    m_emy->m_visible = 0;
    g_gamMan.resetObj(m_oid, 0x40);
    g_gamMan.setObj(m_oid, 0x10000000);
    g_gamMan.setObj(m_oid, 0x400);
    setMotion(0x31, 0x100);
    m_emy->stopAllSE();
    m_emy->m_state2 = 0;
    m_achr->m_flags |= 0x400;
    m_achr->m_flags |= 0x2000;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148e48

int HfmOutDead::Exec()
{
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148e50

void HfmOutDead::Term()
{
    termCommon();
}

// ──── HfmOutWin ───────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148e6c

void HfmOutWin::Init()
{
    m_range = utlRandRangeF(0x1.cbfffep+6f /* 0x42e5ffff, 115 */, 0x1.58fffep+7f /* 0x432c7fff, 172.5 */);
    selectTarget(100);
    emyStartChase(m_range, this, m_target);
    m_step = 0;
    m_emy->m_state2 = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148ed4

int HfmOutWin::Exec()
{
    if (m_step == 0 && m_move_done != 0) {
        m_emy->m_move->stop();
        setMotion(0x16, 0x100);
        m_step = 1;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148f34

void HfmOutWin::Term()
{
    termCommon();
}

// ──── HfmOutBitten ────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148f50

void HfmOutBitten::Init()
{
    m_step = 0;
    setMotion(0xd, 0x100);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148f74
// Steps: 0 bitten (voice; the dog letting go ends it with 7, the free
// count running out shakes it off), 1 25 frames to the throw, 2 until the
// motion ends.

int HfmOutBitten::Exec()
{
    if (magBwnBarkQuery(m_oid) == -1 && m_step == 0) {
        magBwnBarkSet(1);
        return 7;
    }
    switch (m_step) {
    case 0:
        tickVoice(10, 25, 0x32);
        if (!static_cast<HfmMain *>(m_emy)->tickFreeLeft()) {
            stopVoice();
            m_emy->setDecideSw(0);
            g_gamMan.setObj(m_oid, 0x400);
            setMotion(0xe, 0);
            m_cnt = 25;
            playSE(0x12);
            m_step = 1;
        }
        break;
    case 1:
        m_emy->setDecideSw(0);
        g_gamMan.setObj(m_oid, 0x400);
        if (--m_cnt == 0) {
            /* The ROM reads the dog object without a NULL test. */
            scnChrRequest(0xb, m_oid, 0, 0, 0x2d, g_gamMan.m_dogPtr->m_oid, 0);
            magBwnBarkSet(0);
            m_step = 2;
        }
        break;
    case 2:
        m_emy->setDecideSw(0);
        g_gamMan.setObj(m_oid, 0x400);
        if (!g_gamMan.isAchrMotionEnd(m_oid))
            return -1;
        m_emy->m_input_flg_motion = 1;
        m_emy->setDecideSw(1);
        g_gamMan.resetObj(m_oid, 0x400);
        return 7;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00149168

void HfmOutBitten::Term()
{
    termCommon();
}

// ──── HfmOutRide's phases ─────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// The hit test of a slap frame (inline in the ROM's phases and in
// HfmOutCounter::Exec): the PC unless it is down (state class 9), the dog
// unless the bwn is in state 10 / 0xd, each once; a hit within reach plays
// SE 0x10 and sends scene request 0xb with word to it.

static void hfmHitTest(EmyOut *out, int *hitPc, int *hitDog, float reach, int word)
{
    if (!*hitPc && g_magSystem.getPCStateClass() != 9) {
        *hitPc = 1;
        ObjStat *pcObj = g_gamMan.m_pcPtr;
        u_int pc = 0;
        if (pcObj != NULL)
            pc = pcObj->m_oid;
        if (emyChkHitReach(reach, out->m_oid, 1, pc)) {
            out->playSE(0x10);
            scnChrRequest(0xb, out->m_oid, 0, 0, word, pc, 0);
        }
    }
    if (!*hitDog && g_magSystem.getBwnState() != 10 && g_magSystem.getBwnState() != 0xd) {
        *hitDog = 1;
        ObjStat *dogObj = g_gamMan.m_dogPtr;
        u_int dog = 0;
        if (dogObj != NULL)
            dog = dogObj->m_oid;
        if (emyChkHitReach(reach, out->m_oid, 1, dog)) {
            out->playSE(0x10);
            scnChrRequest(0xb, out->m_oid, 0, 0, word, dog, 0);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// rot - pi, wrapped once into -pi..pi in double precision (inline in
// execGrab and execHug).

static float hfmTurnBack(float rot)
{
    float r = (float)((double)rot - 3.141592653589793);
    if ((double)r < -3.141592653589793)
        r = (float)((double)r + 6.283185307179586);
    if ((double)r > 3.141592653589793)
        r = (float)((double)r - 6.283185307179586);
    return r;
}

// ──────────────────────────────────────────────────────────────────────
// The decide switch and the object's 0x400 bit together (inline in the
// ROM's steps): on lets the enemy be hit / interrupted again.

static void hfmSetDecide(EmyOut *out, int on)
{
    out->m_emy->setDecideSw(on);
    if (on)
        g_gamMan.resetObj(out->m_oid, 0x400);
    else
        g_gamMan.setObj(out->m_oid, 0x400);
}

// ──────────────────────────────────────────────────────────────────────
// The motion ended: the emy's motion input is set (inline everywhere).

static bool hfmMotionEnd(EmyOut *out)
{
    if (!g_gamMan.isAchrMotionEnd(out->m_oid))
        return false;
    out->m_emy->m_input_flg_motion = 1;
    return true;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00146ae4
// Phases by r = 0..99: with the PC riding or hurt (commandPC 0x16) 0 for
// r < 10, 1 below 75, else 2 / 3 (r < 85); otherwise 0 below 20, 1 below
// 35, else 2 / 3. Phases 0 / 1 chase with a time limit (peak 150 / 80).

void HfmOutRide::start()
{
    m_chase_cnt = 0;
    m_cnt = 0;
    m_58 = 0;
    m_5c = utlRandRange(0x3c, 0x5a);
    int r  = utlRandRange(0, 99);
    int lo = 10;
    int hi;
    if (g_magSystem.m_scene.commandPC(0x16, NULL) == 0) {
        lo = 0x14;
        hi = 0x23;
    } else {
        hi = 0x4b;
    }
    int peak;
    if (r < lo) {
        m_phase = 0;
        peak = 0x96;
    } else if (r < hi) {
        m_phase = 1;
        peak = 0x50;
    } else {
        m_step  = 0;
        m_phase = r < 0x55 ? 2 : 3;
        goto chase;
    }
    m_step = 0;
    m_chase_max = utlRandPeakHi(10, peak, 3);
chase:
    utlRandRange(0, 99);        // the ROM draws a number it does not use
    emyStartChase(0x1.cbfffep+6f /* 0x42e5ffff, 115 */, this, m_target);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00146bf0

void HfmOutRide::restart()
{
    m_chase_cnt = 0;
    m_cnt = 0;
    m_58 = 0;
    m_5c = utlRandRange(0x3c, 0x5a);
    int peak = 0x96;
    if (utlRandRange(0, 99) < 0x4b) {
        m_phase = 0;
    } else {
        m_phase = 1;
        peak = 0x50;
    }
    m_step = 0;
    m_chase_max = utlRandPeakHi(10, peak, 3);
    emyStartChase(0x1.cbfffep+6f /* 0x42e5ffff, 115 */, this, m_target);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00146c8c

void HfmOutRide::placePC(float dist)
{
    float pos[4];
    getBasePos(pos);
    pos[3] = m_pc_pos[3];
    getFrontPos(dist, pos);
    if (g_magSystem.m_scene.commandPC(0x15, pos) != 0)
        return;
    getFrontPos(30.0f, pos);
    if (g_magSystem.m_scene.commandPC(0x15, pos) != 0)
        return;
    getTargetNodePos(pos);
    GamSecta *hit[4];
    g_magSystem.m_scene.m_8542c.findCells(pos, hit, 0, 40.0f, 0.0f);
    if (hit[1] != NULL)
        m_achr->ObjStat::setPos(pos);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00146d88 // callcheck: emyChkHitReach, getBwnState, getPCStateClass, isAchrMotionEnd, resetObj, scnChrRequest, setDecideSw, setObj (hfmHitTest / hfmSetDecide / hfmMotionEnd (inline in the ROM))
// Two slaps: chase (until it arrives or m_chase_max frames), slap 0x14 with
// its hit frames 14..17, slap 0x15 (8..11), motion 2, then start again.

int HfmOutRide::execPhase0()
{
    int end = 0;                // PORT: the ROM leaves it unset unless m_44 names a slap
    switch (m_step) {
    case 0:
        m_chase_cnt++;
        if (m_move_done == 2) {
            m_emy->m_move->stop();
            return 8;
        }
        if (m_move_done == 1) {
            m_emy->m_move->stop();
            m_reached = 1;
        } else {
            if (m_chase_cnt < m_chase_max)
                return -1;
            m_emy->m_move->stop();
            m_reached = 0;
        }
        setMotion(0x14, 0);
        m_cnt     = 0;
        m_step    = 1;
        m_hit_dog = 0;
        m_hit_pc  = 0;
        playSE(0x13);
        playSE(0x38);
        break;

    case 1:
    case 3:
        m_cnt++;
        if (m_44 == 0x14)
            end = 0xd;
        else if (m_44 == 0x16)
            end = 0x3a;
        if (end < m_cnt)
            hfmSetDecide(this, 0);
        if (m_step == 1) {
            if (0xd < m_cnt && m_cnt < 0x12)
                hfmHitTest(this, &m_hit_pc, &m_hit_dog, 90.0f, 0x26);
            if (m_cnt != 0x11)
                return -1;
        } else {
            if (7 < m_cnt && m_cnt < 0xc)
                hfmHitTest(this, &m_hit_pc, &m_hit_dog, 85.0f, 0x27);
            if (m_cnt != 0xb)
                return -1;
        }
        hfmSetDecide(this, 1);
        m_cnt = 0;
        m_step++;
        break;

    case 2:
        if (hfmMotionEnd(this)) {
            setMotion(0x15, 0);
            playSE(0x13);
            playSE(0x38);
            m_cnt     = 0;
            m_hit_pc  = 0;
            m_step    = 3;
            m_hit_dog = 0;
        }
        break;

    case 4:
        if (hfmMotionEnd(this)) {
            setMotion(2, 0);
            m_step = 5;
        }
        break;

    case 5:
        if (hfmMotionEnd(this))
            start();
        break;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001473e4 // callcheck: emyChkHitReach, getBwnState, getPCStateClass, isAchrMotionEnd, resetObj, scnChrRequest, setDecideSw, setObj (hfmHitTest / hfmSetDecide / hfmMotionEnd (inline in the ROM))
// The long slap 0x16: SE 0x38 after frame 70, hit frames 75..78 at 105.

int HfmOutRide::execPhase1()
{
    switch (m_step) {
    case 0:
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        if (m_move_done == 2)
            return 8;
        setMotion(0x16, 0);
        m_se_done  = 0;
        m_se_frame = 0x46;
        playSE(0x13);
        m_cnt     = 0;
        m_step    = 1;
        m_hit_pc  = 0;
        m_hit_dog = 0;
        break;

    case 1:
        m_cnt++;
        if (m_se_frame < m_cnt && !m_se_done) {
            playSE(0x38);
            m_se_done = 1;
        }
        hfmSetDecide(this, m_cnt < 0x42);
        if (0x4a < m_cnt && m_cnt < 0x4f)
            hfmHitTest(this, &m_hit_pc, &m_hit_dog, 105.0f, 0x28);
        if (m_cnt == 0x4e)
            m_step = 2;
        break;

    case 2:
        m_cnt++;
        hfmSetDecide(this, m_cnt < 0x59);
        if (!hfmMotionEnd(this))
            return -1;
        hfmSetDecide(this, 1);
        m_cnt = 0;
        start();
        break;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00147840
// The ride. Step 0 chases and jumps (motion 0x1c) when the point 150 ahead
// is on the cells, 1 grabs the PC's back, 2 / 3 mount, 4 rides (waits,
// hits in steps 5 / 6), 7 thrown off, 9 / 10 the PC struggling. While
// riding (4, 9, 10) the dog's bark makes it let go.

int HfmOutRide::execPhase2()
{
    if ((m_step == 4 || m_step == 9 || m_step == 10) && magBwnBarkQuery(m_oid) != -1) {
        magBwnBarkSet(1);
        release();
        return -1;
    }
    switch (m_step) {
    case 0:
    {
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        if (m_move_done == 2)
            return 8;
        float pos[4];
        getFrontPos(150.0f, pos);
        GamSecta *hit[4];
        g_magSystem.m_scene.m_8542c.findCells(pos, hit, 0, 40.0f, 0.0f);
        if (hit[1] == NULL) {
            restart();
            return -1;
        }
        playSE(0x13);
        playSE(0x38);
        setMotion(0x1c, 0);
        m_cnt  = 0;
        m_step = 1;
        break;
    }
    case 1:  execGrab();     break;
    case 2:  execMount();    break;
    case 3:  execRideEnd();  break;
    case 4:  execRide();     break;
    case 5:  execHit();      break;
    case 6:  execHitNext();  break;
    case 7:  execThrown();   break;
    case 8:  execStep8();    break;
    case 9:  execStruggle(); break;
    case 10: execGrip();     break;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00147760 // callcheck: resetObj (in hfmSetDecide)

void HfmOutRide::release()
{
    stopVoice();
    m_emy->m_state2 = 0;
    g_magSystem.m_scene.commandPC(m_side ? 9 : 6, NULL);
    setMotion(0x2f, 0);
    m_emy->m_238 = 1;           // SetMissing(true)
    playSE(0x39);
    hfmSetDecide(this, 1);
    m_cnt     = 0;
    m_step    = 7;
    m_se_done = 0;
    m_emy->setDecideSw(1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00147a28 // callcheck: dpadd, dpcmp, dpsub, dptofp, fptodp, isAchrMotionEnd (host doubles in hfmTurnBack and the side test; hfmMotionEnd)
// The jump: frames 27..32 with the PC in sight (60, pi / 4), free (not
// riding or hurt) and the game flags 4 set / 0x2000 clear, it lands on the
// PC's back: from behind (m_side) PC command 7 and motion 0x1e, else 2 and
// motion 0x1d with the PC turned to face it. Missing ends in start.

void HfmOutRide::execGrab()
{
    m_cnt++;
    if (m_cnt < 7) {
        hfmSetDecide(this, 1);
    } else {
        hfmSetDecide(this, 0);
        m_emy->m_238 = 1;
    }
    if (hfmMotionEnd(this)) {
        hfmSetDecide(this, 1);
        m_emy->m_238 = 0;
        m_cnt     = 0;
        m_se_done = 0;
        start();
        playSE(0x39);
        return;
    }
    if (!(0x1a < m_cnt && m_cnt < 0x21))
        return;
    if (!emyChkInSight(60.0f, 0x1.921fb4p-1f /* 0x3f490fda, pi / 4 */, 1, m_target, m_achr))
        return;
    if (g_magSystem.m_scene.commandPC(0x16, NULL) != 0)
        return;
    if ((g_gamMan.m_flags & 0x2000) != 0 || (g_gamMan.m_flags & 4) == 0)
        return;

    HfmMain *hfm = static_cast<HfmMain *>(m_emy);
    m_emy->m_238 = 0;
    m_emy->setDecideSw(0);
    hfm->startFree(0x28);
    g_gamMan.resetObj(m_oid, 0x40);
    g_gamMan.setObj(m_oid, 0x10000000);
    g_gamMan.setObj(m_oid, 0x4000000);
    m_pc_pos[3] = g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(m_oid)].getRotY();
    /* The ROM also looks up the target's actor here and drops it. */
    m_side = (double)getTargetAngle() >= 1.5707963267948966;
    hfm->m_23c = m_side;
    int motion;
    if (!m_side) {
        m_emy->m_state2 = 3;
        g_magSystem.m_scene.commandPC(2, NULL);
        motion = 0x1d;
        getFrontPos(45.0f, m_pc_pos);
        m_pc_pos[3] = hfmTurnBack(m_pc_pos[3]);
    } else {
        m_emy->m_state2 = 4;
        g_magSystem.m_scene.commandPC(7, NULL);
        motion = 0x1e;
        getFrontPos(60.0f, m_pc_pos);
    }
    placePC(-3.0f);
    setMotion(motion, 0);
    m_cnt  = 0;
    m_step = 2;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00147e04 // callcheck: isAchrMotionEnd (in hfmMotionEnd)

void HfmOutRide::execMount()
{
    if (hfmMotionEnd(this)) {
        placePC(-3.0f);
        setMotion(m_side ? 0x25 : 0x24, 0x100);
        nextHold();
        return;
    }
    placePC(-3.0f);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00147e94 // callcheck: isAchrMotionEnd (in hfmMotionEnd)

void HfmOutRide::execRideEnd()
{
    if (hfmMotionEnd(this)) {
        setMotion(m_side ? 0x25 : 0x24, 0x100);
        nextHold();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00147f00
// Riding: when the wait runs out a series of 1..3 hits (motion 0x26 or
// 0x28, a voice SE from the table 0x2ef418); the PC in state class 3
// struggles (motion 0x21 / 0x22, step 9).

void HfmOutRide::execRide()
{
    static const int s_hit_se[5] = { 0x18, 0x19, 0x19, 0x1b, 0x1b };  // ROR 0x002ef418

    tickVoice(5, 0xf, 0x25);
    int cls = g_magSystem.getPCStateClass();
    if (cls == 9)
        return;
    placePC(-3.0f);
    m_wait--;
    m_64--;
    if (m_wait < 1) {
        m_hits = utlRandRange(1, 3);
        int motion = 0x28;
        if (utlRandRange(0, 99) < 0x32) {
            m_hit_alt = 1;
            m_hit_at  = 0xc;
        } else {
            m_hit_alt = 0;
            m_hit_at  = 0xc;
            motion = 0x26;
        }
        setMotion(motion, 0);
        stopVoice();
        playSE(s_hit_se[utlRandRange(0, 4)]);
        playSE(0x38);
        m_step = 5;
    } else if (cls == 3) {
        stopVoice();
        setMotion(m_side ? 0x22 : 0x21, 0x100);
        m_step = 9;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148060

void HfmOutRide::execHit()
{
    m_cnt++;
    if (m_cnt == m_hit_at) {
        playSE(0x10);
        scnChrRequest(0xb, m_oid, 0, 0, 0x2b, m_target, 0);
        m_cnt = 0;
        m_hits--;
        m_step = m_hits == 0 ? 3 : 6;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001480e0 // callcheck: isAchrMotionEnd (in hfmMotionEnd)

void HfmOutRide::execHitNext()
{
    if (!hfmMotionEnd(this))
        return;
    if (g_magSystem.getPCStateClass() == 3) {
        setMotion(m_side ? 0x22 : 0x21, 0x100);
        m_step = 9;
        return;
    }
    int motion;
    if (m_hit_alt == 0) {
        motion = 0x28;
        m_hit_alt = 1;
        m_hit_at  = 0xc;
    } else {
        m_hit_alt = 0;
        m_hit_at  = 0xc;
        motion = 0x26;
    }
    setMotion(motion, 0);
    stopVoice();
    playSE(0x38);
    m_step = 5;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001481b8 // callcheck: isAchrMotionEnd (in hfmMotionEnd)

void HfmOutRide::execThrown()
{
    m_cnt++;
    if (0x28 < m_cnt && !m_se_done) {
        playSE(0x37);
        m_se_done = 1;
    }
    if (hfmMotionEnd(this)) {
        m_emy->m_238 = 0;
        g_gamMan.setObj(m_oid, 0x40);
        g_gamMan.resetObj(m_oid, 0x10000000);
        g_gamMan.resetObj(m_oid, 0x4000000);
        start();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014827c // callcheck: isAchrMotionEnd (in hfmMotionEnd)

void HfmOutRide::execStep8()
{
    m_cnt++;
    if (0x28 < m_cnt && !m_se_done) {
        playSE(0x37);
        m_se_done = 1;
    }
    if (hfmMotionEnd(this)) {
        m_emy->m_238 = 0;
        start();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148304

void HfmOutRide::execStruggle()
{
    if (g_magSystem.getPCStateClass() == 3) {
        if (static_cast<HfmMain *>(m_emy)->countFree())
            release();
        return;
    }
    setMotion(m_side ? 0x25 : 0x24, 0x100);
    nextHold();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0014837c

void HfmOutRide::execGrip()
{
    m_grip--;
    tickVoice(1, 10, 0x13);
    if (g_magSystem.getPCStateClass() == 3) {
        if (static_cast<HfmMain *>(m_emy)->countFree()) {
            stopVoice();
            release();
        }
    } else if (m_grip < 1) {
        stopVoice();
        scnChrRequest(0xb, m_oid, 0, 0, 0x2b, m_target, 0);
        setMotion(m_side ? 0x25 : 0x24, 0x100);
        nextHold();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148450

void HfmOutRide::nextHold()
{
    if (utlRandRange(0, 99) < 0x1e) {
        m_step = 10;
        m_grip = utlRandRange(0x14, 0x28);
        setMotion(0x23, 0x100);
        return;
    }
    m_step = 4;
    int r = utlRandRange(0, 99);
    int wait = 0x14;
    if (0x26 < r)
        wait = r < 0x4f ? 0x2d : 0x4b;
    m_wait = wait;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001484ec
// The hug: chase, motion 0x18 when the point 98 ahead is on the cells, 1
// grab, 2 hold, 3 squeeze, 4 let go.

int HfmOutRide::execPhase3()
{
    switch (m_step) {
    case 0:
    {
        if (m_move_done == 0)
            return -1;
        m_emy->m_move->stop();
        if (m_move_done == 2)
            return 8;
        float pos[4];
        getFrontPos(98.0f, pos);
        GamSecta *hit[4];
        g_magSystem.m_scene.m_8542c.findCells(pos, hit, 0, 40.0f, 0.0f);
        if (hit[1] == NULL) {
            restart();
            return -1;
        }
        m_cnt = 0;
        setMotion(0x18, 0);
        playSE(0x13);
        m_step = 1;
        break;
    }
    case 1: execHug();     break;
    case 2: execHugHold(); break;
    case 3: execHugHit();  break;
    case 4: execHugEnd();  break;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148620 // callcheck: dpadd, dpcmp, dpsub, dptofp, fptodp, isAchrMotionEnd (host doubles in hfmTurnBack and the side test; hfmMotionEnd)
// Frames 15..19 with the PC in sight (~90, pi / 4) and free: the target's
// actor is put 26.5 ahead of the enemy, PC command 12 from behind (m_side)
// or 10 turned to face it, the hold motion 0x1a with SE 0x2a in m_se.

void HfmOutRide::execHug()
{
    m_cnt++;
    hfmSetDecide(this, m_cnt < 7);
    if (hfmMotionEnd(this)) {
        hfmSetDecide(this, 1);
        m_cnt     = 0;
        m_se_done = 0;
        start();
        return;
    }
    if (!(0xe < m_cnt && m_cnt < 0x14))
        return;
    if (!emyChkInSight(0x1.69fffep+6f /* 0x42b4ffff, 90 */, 0x1.921fb4p-1f /* 0x3f490fda, pi / 4 */,
                       1, m_target, m_achr))
        return;
    if (g_magSystem.m_scene.commandPC(0x16, NULL) != 0)
        return;
    if ((g_gamMan.m_flags & 0x2000) != 0 || (g_gamMan.m_flags & 4) == 0)
        return;

    HfmMain *hfm = static_cast<HfmMain *>(m_emy);
    m_emy->setDecideSw(0);
    hfm->startFree(0x28);
    g_gamMan.resetObj(m_oid, 0x40);
    g_gamMan.setObj(m_oid, 0x10000000);
    g_gamMan.setObj(m_oid, 0x4000000);
    m_wait = utlRandRange(0x14, 0x28);
    m_emy->m_state2 = 2;
    DogActor *target = &g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(m_target)];
    float pos[4];
    getFrontPos(26.5f, pos);
    target->m_trans[0] = pos[0];
    target->m_trans[1] = pos[1];
    target->m_trans[2] = pos[2];
    target->m_dirty = 1;
    m_pc_pos[3] = g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(m_oid)].getRotY();
    m_side = (double)getTargetAngle() >= 1.5707963267948966;
    hfm->m_23c = m_side;
    if (!m_side) {
        m_pc_pos[3] = hfmTurnBack(m_pc_pos[3]);
        g_magSystem.m_scene.commandPC(10, NULL);
    } else {
        g_magSystem.m_scene.commandPC(0xc, NULL);
    }
    placePC(15.0f);
    setMotion(0x1a, 0x100);
    m_cnt = 0;
    m_se  = playSE(0x2a);
    m_step = 2;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148a00

void HfmOutRide::releaseHug()
{
    m_emy->m_state2 = 0;
    g_magSystem.m_scene.commandPC(m_side ? 0xd : 0xb, NULL);
    setMotion(0x19, 0);
    playSE(0x37);
    g_gamMan.setObj(m_oid, 0x40);
    g_gamMan.resetObj(m_oid, 0x10000000);
    g_gamMan.resetObj(m_oid, 0x4000000);
    m_emy->setDecideSw(1);
    m_cnt     = 0;
    m_se_done = 0;
    m_step    = 4;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148af0

void HfmOutRide::execHugHold()
{
    if (magBwnBarkQuery(m_oid) != -1) {
        magBwnBarkSet(1);
        releaseHug();
        return;
    }
    placePC(15.0f);
    int cls = g_magSystem.getPCStateClass();
    if (cls == 9)
        return;
    m_wait--;
    if (m_wait < 1) {
        m_cnt = 0;
        setMotion(0x35, 0);
        playSE(0x13);
        m_step = 3;
    } else if (cls == 3) {
        m_58 = 0;
        if (static_cast<HfmMain *>(m_emy)->countFree())
            releaseHug();
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148bd0 // callcheck: isAchrMotionEnd (in hfmMotionEnd)

void HfmOutRide::execHugHit()
{
    m_cnt++;
    if (m_cnt == 0x12) {
        g_sndMan.stopSe(m_se);
        playSE(0x10);
        scnChrRequest(0xb, m_oid, 0, 0, m_side ? 0x2a : 0x29, m_target, 0);
        m_wait = utlRandRange(0x14, 0x28);
    }
    if (hfmMotionEnd(this)) {
        setMotion(0x1a, 0x100);
        m_cnt  = 0;
        m_step = 2;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00148cb4 // callcheck: isAchrMotionEnd (in hfmMotionEnd)

void HfmOutRide::execHugEnd()
{
    m_cnt++;
    if (0 < m_cnt && !m_se_done) {
        playSE(0x37);
        m_se_done = 1;
    }
    if (hfmMotionEnd(this))
        start();
}

// ──── HfmOutCounter ───────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00149184

void HfmOutCounter::Init()
{
    m_emy->m_state2 = 0;
    setMotion(0x34, 0);
    playSE(0x13);
    m_cnt  = 0;
    m_step = 0;
    m_emy->setDecideSw(0);
    m_hit_dog = 0;
    m_hit_pc  = 0;
    selectTarget(100);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001491ec // callcheck: emyChkHitReach, getBwnState, getPCStateClass, isAchrMotionEnd, playSE, resetObj, scnChrRequest, setObj (hfmHitTest / hfmSetDecide / hfmMotionEnd (inline in the ROM))
// Step 0: the blow's hit frames 8..23 at 126.5 (word 0x2c), 2 at frame 23;
// step 2 waits for the motion end and returns 7. m_cnt stays 0 in step 2,
// so its frame-28 switch never turns the decide on early (ROM).

int HfmOutCounter::Exec()
{
    if (m_step == 0) {
        m_cnt++;
        hfmSetDecide(this, 0);
        if (7 < m_cnt && m_cnt < 0x18)
            hfmHitTest(this, &m_hit_pc, &m_hit_dog, 0x1.f9fffep+6f /* 0x42fcffff, 126.5 */, 0x2c);
        if (m_cnt == 0x17) {
            m_cnt  = 0;
            m_step = 2;
        }
    } else if (m_step == 2) {
        hfmSetDecide(this, (u_int)m_cnt >= 0x1c);
        if (hfmMotionEnd(this)) {
            hfmSetDecide(this, 1);
            m_emy->setDecideSw(1);
            return 7;
        }
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001494ac

void HfmOutCounter::Term()
{
    termCommon();
}

// ──── HfmOutTest ──────────────────────────────────────────────────────

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001494c8

void HfmOutTest::Init()
{
    m_step = 0;
    setMotion(0x10, 0);
    m_emy->m_state2 = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00149500 // callcheck: isAchrMotionEnd (in hfmMotionEnd)
// A motion walk-through: each time a motion ends, the next step's motion
// starts. The ROM is a 0x2d-case switch (jump table 0x2ef4b0) with the
// same body per case; the motions are this table, -1 where a step only
// advances. Step 4 starts 0x10 again and goes back to step 0, so steps
// 5..0x2c never run; step 0x2c (motion 0x31) also returns to 0.

static const int s_hfmTestMotion[0x2d] = {
    0x10, 0x10, 0x10, 0x10, 0x10, 0x07, 0x0c, 0x14, 0x15, 0x00, 0x00, 0x16,
    0x17, 0x18, 0x19, 0x1a, 0x1c, 0x1c,   -1, 0x24, 0x21, 0x1f, 0x23, 0x26,
    0x27, 0x28, 0x29, 0x2e, 0x2f,   -1,   -1,   -1, 0x25, 0x20, 0x30, 0x13,
    0x32, 0x33, 0x0d, 0x0e, 0x0f, 0x12, 0x0b, 0x0a, 0x31,
};

int HfmOutTest::Exec()
{
    if ((u_int)m_step > 0x2c)
        return -1;
    if (!hfmMotionEnd(this))
        return -1;
    int motion = s_hfmTestMotion[m_step];
    if (motion >= 0)
        setMotion(motion, 0);
    if (m_step == 4 || m_step == 0x2c)
        m_step = 0;
    else
        m_step++;
    return -1;
}
