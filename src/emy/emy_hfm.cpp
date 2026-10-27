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
    m_motion_flg[1]  = 1;
    m_motion_flg[2]  = 1;
    m_motion_flg[3]  = 1;
    m_motion_flg[4]  = 0;
    m_motion_flg[5]  = 1;
    m_motion_flg[6]  = 1;
    m_motion_flg[7]  = 0;
    m_motion_flg[8]  = 0;
    m_motion_flg[9]  = 0;
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
                emyStartMove(0, this, pos);
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

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x00145d60
void HfmOutSlap::Init()
{
    ROSEPAN_STUB("HfmOutSlap::Init", 0x00145d60);
}

// stub of ROR 0x00145dc0
int HfmOutSlap::Exec()
{
    ROSEPAN_STUB("HfmOutSlap::Exec", 0x00145dc0);
    return -1;
}

// stub of ROR 0x0014635c
void HfmOutSlap::Term()
{
    ROSEPAN_STUB("HfmOutSlap::Term", 0x0014635c);
}

// stub of ROR 0x00146378
void HfmOutBreak::Init()
{
    ROSEPAN_STUB("HfmOutBreak::Init", 0x00146378);
}

// stub of ROR 0x001463b8
int HfmOutBreak::Exec()
{
    ROSEPAN_STUB("HfmOutBreak::Exec", 0x001463b8);
    return -1;
}

// stub of ROR 0x00146460
void HfmOutBreak::Term()
{
    ROSEPAN_STUB("HfmOutBreak::Term", 0x00146460);
}

// stub of ROR 0x0014647c
void HfmOutWalk2::Init()
{
    ROSEPAN_STUB("HfmOutWalk2::Init", 0x0014647c);
}

// stub of ROR 0x001464cc
int HfmOutWalk2::Exec()
{
    ROSEPAN_STUB("HfmOutWalk2::Exec", 0x001464cc);
    return -1;
}

// stub of ROR 0x001469b0
void HfmOutWalk2::Term()
{
    ROSEPAN_STUB("HfmOutWalk2::Term", 0x001469b0);
}

// stub of ROR 0x001469e8
void HfmOutRide::Init()
{
    ROSEPAN_STUB("HfmOutRide::Init", 0x001469e8);
}

// stub of ROR 0x00146a20
int HfmOutRide::Exec()
{
    ROSEPAN_STUB("HfmOutRide::Exec", 0x00146a20);
    return -1;
}

// stub of ROR 0x00146ab8
void HfmOutRide::Term()
{
    ROSEPAN_STUB("HfmOutRide::Term", 0x00146ab8);
}

// stub of ROR 0x00148d30
void HfmOutDead::Init()
{
    ROSEPAN_STUB("HfmOutDead::Init", 0x00148d30);
}

// stub of ROR 0x00148e48
int HfmOutDead::Exec()
{
    ROSEPAN_STUB("HfmOutDead::Exec", 0x00148e48);
    return -1;
}

// stub of ROR 0x00148e50
void HfmOutDead::Term()
{
    ROSEPAN_STUB("HfmOutDead::Term", 0x00148e50);
}

// stub of ROR 0x00148e6c
void HfmOutWin::Init()
{
    ROSEPAN_STUB("HfmOutWin::Init", 0x00148e6c);
}

// stub of ROR 0x00148ed4
int HfmOutWin::Exec()
{
    ROSEPAN_STUB("HfmOutWin::Exec", 0x00148ed4);
    return -1;
}

// stub of ROR 0x00148f34
void HfmOutWin::Term()
{
    ROSEPAN_STUB("HfmOutWin::Term", 0x00148f34);
}

// stub of ROR 0x00148f50
void HfmOutBitten::Init()
{
    ROSEPAN_STUB("HfmOutBitten::Init", 0x00148f50);
}

// stub of ROR 0x00148f74
int HfmOutBitten::Exec()
{
    ROSEPAN_STUB("HfmOutBitten::Exec", 0x00148f74);
    return -1;
}

// stub of ROR 0x00149168
void HfmOutBitten::Term()
{
    ROSEPAN_STUB("HfmOutBitten::Term", 0x00149168);
}

// stub of ROR 0x00149184
void HfmOutCounter::Init()
{
    ROSEPAN_STUB("HfmOutCounter::Init", 0x00149184);
}

// stub of ROR 0x001491ec
int HfmOutCounter::Exec()
{
    ROSEPAN_STUB("HfmOutCounter::Exec", 0x001491ec);
    return -1;
}

// stub of ROR 0x001494ac
void HfmOutCounter::Term()
{
    ROSEPAN_STUB("HfmOutCounter::Term", 0x001494ac);
}

// stub of ROR 0x001494c8
void HfmOutTest::Init()
{
    ROSEPAN_STUB("HfmOutTest::Init", 0x001494c8);
}

// stub of ROR 0x00149500
int HfmOutTest::Exec()
{
    ROSEPAN_STUB("HfmOutTest::Exec", 0x00149500);
    return -1;
}
