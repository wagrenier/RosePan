/* ==========================================================================
 *  emy/emy_main.cpp
 *
 *  EmyMain, the base of the eleven enemy classes (0x164118..0x166a1c):
 *  construction and teardown, the state graph from the transition table
 *  (setStartState), the script parameters onto the achr and the move /
 *  turn tasks (loadParam), the group appear / respawn steps, the four
 *  owned sound records, the SE table by type (getSE) and the small tests
 *  the classes' decide calls (the PC's state class and facing, the dog's
 *  bark, the life thresholds). RoR's source string "emy/emy_main.cpp"
 *  (setStartState's and loadParam's allocDebug) names the file.
 * ======================================================================== */
#include "emy/emy_main.h"

#include <math.h>
#include <string.h>

#include "dog/dogactor.h"
#include "emy/emy_manager.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "lib/unassigned.h"
#include "mag/magscenecmd.h"
#include "mag/magsystem.h"
#include "task/tsk_man.h"
#include "utl/utlmisc.h"
#include "utl/utlrand.h"

/* The source name of the allocDebug tags (0x2f14b8). */
static const char s_file[] = "emy/emy_main.cpp";

/* The kind move table (0x2f18b8, a row per system data field 0x20;
 * getMoveParam): words 0..3 go to the move record, word 4 (float) too,
 * word 5 (float) is loadParam's speed base. */
static const EmyKindMoveParam s_kindMove[12] = {
    { { 300,    14, 0, 0, 0x437a0000 /* 250 */, 0x3f800000 /* 1.0 */ } },  // 0x002f18b8
    { { 500,    12, 0, 0, 0x43fa0000 /* 500 */, 0x3f800000 /* 1.0 */ } },  // 0x002f18d0
    { { 300,     5, 0, 0, 0x43960000 /* 300 */, 0x3fc00000 /* 1.5 */ } },  // 0x002f18e8
    { { 350,     7, 0, 0, 0x43960000 /* 300 */, 0x3fa66666 /* 1.3 */ } },  // 0x002f1900
    { { 150,    15, 0, 0, 0x00000000 /* 0 */,   0x3f800000 /* 1.0 */ } },  // 0x002f1918
    { { 200,     5, 0, 0, 0x00000000 /* 0 */,   0x3fa66666 /* 1.3 */ } },  // 0x002f1930
    { { 500,     2, 0, 0, 0x43c80000 /* 400 */, 0x3fa66666 /* 1.3 */ } },  // 0x002f1948
    { { 400,     4, 0, 0, 0x43c80000 /* 400 */, 0x3fcccccc /* 1.6 */ } },  // 0x002f1960
    { { 400,     1, 0, 0, 0x43c80000 /* 400 */, 0x3f8ccccc /* 1.1 */ } },  // 0x002f1978
    { { 880,     1, 0, 0, 0x43c80000 /* 400 */, 0x3f800000 /* 1.0 */ } },  // 0x002f1990
    { { 999999,  1, 0, 0, 0x43fa0000 /* 500 */, 0x3f800000 /* 1.0 */ } },  // 0x002f19a8
    { { 1000,    2, 0, 0, 0x437a0000 /* 250 */, 0x40000000 /* 2.0 */ } },  // 0x002f19c0
};

/* getSE's data, the ROM image 0x2f16b8..0x2f18b8: per-parsonality rows
 * and SE pairs (pickSE) for nos 0..14, then from 0x2f1758 the table nos
 * 15.. index straight (the ROM reads 0x2f171c + no * 4). */
static const u_int s_seData[128] = {
    0x0c00038e, 0x0c00038f, 0x0c000390, 0x00000000, 0x0c000399, 0x0c0003a3, 0x0c010464, 0x0c01045d,  // 0x002f16b8
    0x0c01045e, 0x00000000, 0x0c01046b, 0x0c010460, 0x0c010461, 0x00000000, 0x0c00037e, 0x0c000386,  // 0x002f16d8
    0x0c00037f, 0x0c000387, 0x0c00037d, 0x0c00038b, 0x0c00039d, 0x0c0003a7, 0x0c000381, 0x0c000389,  // 0x002f16f8
    0x0c000382, 0x0c00038a, 0x0c0003a1, 0x0c0003ab, 0x0c000384, 0x0c00038c, 0x0c00039e, 0x0c0003a8,  // 0x002f1718
    0x0c000380, 0x0c000388, 0x0c00039c, 0x0c0003a6, 0x0c000385, 0x0c00038d, 0x0c00039f, 0x0c0003a9,  // 0x002f1738
    0x1f00054c, 0x1f00054b, 0x1f00054d, 0x1f00054e, 0x1f00054f, 0x1f000550, 0x1f000551, 0x1f000552,  // 0x002f1758
    0x1f000553, 0x1f000554, 0x1f000555, 0x1f000556, 0x1f000557, 0x1f000558, 0x1f000559, 0x1f00055a,  // 0x002f1778
    0x1f00055b, 0x1f00055c, 0x1f00055d, 0x1f00055e, 0x1f00055f, 0x1f000560, 0x1f000561, 0x1f000562,  // 0x002f1798
    0x1f000563, 0x1f000564, 0x1f000565, 0x1f000566, 0x1f000567, 0x1f000568, 0x1f000569, 0x1f00056a,  // 0x002f17b8
    0x1f00056b, 0x1f00056c, 0x1f00056d, 0x1f00056e, 0x1f00056f, 0x1f000570, 0x1f000571, 0x1f000572,  // 0x002f17d8
    0x1f000573, 0x1f000574, 0x1f000575, 0x1f010594, 0x1f010595, 0x1f010596, 0x1f010597, 0x1f010598,  // 0x002f17f8
    0x1f010599, 0x1f01059a, 0x1f01059b, 0x1f01059c, 0x1f01059d, 0x1f01059e, 0x1f0105a0, 0x1f0105a2,  // 0x002f1818
    0x1f0105a3, 0x1f0105a6, 0x1f0105a7, 0x1f0105ab, 0x1f0105ac, 0x1f0105ad, 0x1f0105b0, 0x1f0105b1,  // 0x002f1838
    0x1f0105b2, 0x1f020625, 0x1f020626, 0x1f02062c, 0x1f02062d, 0x1f02062e, 0x1f020633, 0x1f020634,  // 0x002f1858
    0x1f020635, 0x1f020636, 0x1f020638, 0x1f020639, 0x1f02063a, 0x1f02063b, 0x1f02063c, 0x1f02063d,  // 0x002f1878
    0x1f02063f, 0x1f020640, 0x1f020641, 0x1f020642, 0x1f020643, 0x1f020644, 0x1f020645, 0x00000000,  // 0x002f1898
};
/* An address of the image as a pointer into s_seData. */
#define EMY_SE(addr) (&s_seData[((addr) - 0x2f16b8) / 4])

static inline ACHRstat *emyPC()
{
    return static_cast<ACHRstat *>(g_gamMan.m_pcPtr);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00164118 // callcheck: memset (the host clears by sizeof)

EmyMain::EmyMain(u_int oid, u_short name, ACHRstat *param, const _tagEnemyParam *prm,
                 const _tagEnemyTransition *trans, float *pos)
{
    (void)name;     /* not stored: loadParam takes the name from prm */
    (void)prm;
    m_oid = oid;
    m_req = -1;
    m_trans = trans;
    m_input_flg_motion = 1;
    m_manage_sw = 1;
    m_visible = 1;
    m_down_flg = 0;
    m_damage = 0;
    m_map = NULL;
    m_param = NULL;
    m_emy_out = NULL;
    m_decide_sw = 1;
    m_notify = 0;
    m_state2 = 0;
    m_func = &EmyMain::default_func;
    m_agl = 1.0f;
    m_parsonality = 0;
    m_1ec = 0;
    m_move = NULL;
    m_turn = NULL;
    memset(m_se, 0, sizeof(m_se));
    m_param = param;
    if (pos != NULL)
        param->setParam(0x59, pos);
    memset(m_out, 0, sizeof(m_out));
    // PORT: the ROM leaves these to the heap or to loadParam / setGroup;
    // the host starts them cleared.
    m_name = 0;
    m_voice = 0;
    m_down_supine = 0;
    m_bark_wait = 0;
    m_bark_cool = 0;
    m_state = 0;
    m_changed = 0;
    memset(m_motion_list, 0, sizeof(m_motion_list));
    memset(m_state_flg, 0, sizeof(m_state_flg));
    m_grp_no = 0;
    m_grp_state = 0;
    m_grp_wait = 0;
    m_grp_1d2 = 0;
    m_grp_respawn = 0;
    m_lp = 0;
    m_damage_end = 0;
    m_func_count = 0;
    m_move_id = -1;
    m_turn_id = -1;
    m_238 = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00164208 // callcheck: ~EmyStateMap, operator_delete (delete m_map; the deleting variant is the compiler's)

EmyMain::~EmyMain()
{
    int item[4];
    m_param->getParam(0x67, item);
    if (item[0] != 0) {
        g_magSystem.setActorParent(g_gamMan.getActorNo(item[0]), -1, 0);
        g_gamMan.releaseChr(item[0]);
        item[0] = 0;
        m_param->setParam(0x67, item);
    }
    if (m_map != NULL) {
        delete m_map;
        m_map = NULL;
    }
    for (int i = 0; i < 40; i++) {
        if (m_out[i] != NULL)
            delete m_out[i];
    }
    if (g_taskMan.getTask(m_move_id))
        g_taskMan.killTask(m_move_id);
    if (g_taskMan.getTask(m_turn_id))
        g_taskMan.killTask(m_turn_id);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00164364 // callcheck: operator_new (new)
// A GamIdMap per row of the table: each non-zero byte maps its column
// (the decide input) to the state it leads to.

void EmyMain::setStartState(int state, const _tagEnemyTransition *trans)
{
    m_map = new EmyStateMap(state);
    int inputs = trans->m_inputs;
    int states = trans->m_states;
    const char *table = trans->m_table;
    u_char *pairs = (u_char *)g_utlMemory.allocDebug(inputs * 2, s_file, 0x91, 0, 2);
    for (int i = 0; i < states; i++) {
        int n = 0;
        const char *row = table + i * inputs;
        for (int j = 0; j < inputs; j++) {
            if (row[j] != 0) {
                pairs[n * 2] = (u_char)j;
                pairs[n * 2 + 1] = (u_char)row[j];
                n++;
            }
        }
        if (n != 0) {
            GamIdMap *map = new GamIdMap;
            map->init(i, n);
            for (int k = 0; k < n; k++)
                map->add(pairs[k * 2], pairs[k * 2 + 1]);
            m_map->addMap(i, map);
        }
    }
    utlMemFree(pairs);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00164508 // callcheck: operator_new, TskEnemyMove, TskEnemyTurn (new T calls the constructors)
// The script block onto the achr: flags, rotation, position, a random
// life around the record's, one attr bit per flag byte, the item / move
// params; then the enemy's name, start state and speed, and its move and
// turn tasks.

void EmyMain::loadParam(const _tagEnemyParam *prm)
{
    int stat = 0;
    m_param->m_flags |= 0x200000;
    m_param->m_flags |= 0x400000;
    m_param->m_flags |= 0x20000000;
    g_gamMan.setObj(m_oid, 0x40);
    g_gamMan.resetObj(m_oid, 0x10000000);
    g_gamMan.resetObj(m_oid, 0x400);
    float range = 0x1.921fb4p+1f;   /* 0x40490fda, pi */
    m_param->setParam(0x1105, &range);

    _tagEnemyParam *p = (_tagEnemyParam *)g_utlMemory.allocDebug(sizeof(_tagEnemyParam), s_file, 0xd3, 0, 2);
    memcpy(p, prm, sizeof(_tagEnemyParam));
    m_param->setParam(7, &stat);
    m_param->setParam(6, p->m_rot);
    float pos[4];
    m_param->getParam(0x59, pos);
    m_param->setParam(5, pos);

    int kind = g_gamMan.getObjKind(m_oid);
    u_short lp, lpRange;
    g_gamMan.m_sysData.getField(kind, 0x1c, &lp);
    g_gamMan.m_sysData.getField(kind, 0x1f, &lpRange);
    lp = (u_short)utlRandRange(lp - lpRange, lp + lpRange);
    m_param->setParam(0x1b, &lp);
    m_lp = lp;

    if (p->m_flag800 != 0)
        m_param->m_flags |= 0x800;
    else
        m_param->m_flags &= ~0x800u;
    for (int i = 0; i < 23; i++) {
        u_int bit = i < 7 ? 1u << i : 1u << (i + 1);
        if (p->m_attr0[i] != 0)
            m_param->m_attr[0] |= bit;
        else
            m_param->m_attr[0] &= ~bit;
    }
    for (int i = 0; i < 8; i++) {
        if (p->m_attr1[i] != 0)
            m_param->m_attr[1] |= 1u << i;
        else
            m_param->m_attr[1] &= ~(1u << i);
    }

    m_param->setParam(0x5b, &p->m_p5b);
    m_param->setParam(0x5c, &p->m_p5c);
    m_param->setParam(0x5f, &p->m_p5f);
    m_param->setParam(0x68, &p->m_p68);
    m_param->setParam(0x69, &p->m_p69);
    m_param->setParam(0x5a, &p->m_p5a);
    m_param->setParam(0x5e, &p->m_p5e);
    m_param->setParam(0x63, &p->m_p63);
    m_param->setParam(0x66, &p->m_item_kind);
    m_param->setParam(0x6a, &p->m_p6a);
    m_param->setParam(0x67, &p->m_item);
    m_name = p->m_name;
    m_bark_wait = utlRandRange(0, 20);
    m_bark_cool = 0;
    m_state = p->m_start_state;
    float speed;
    memcpy(&speed, &getMoveParam()->m_w[5], sizeof(speed));
    m_agl = utlRandRangeF(speed * 0x1.ccccccp-1f /* 0x3f666666, 0.9 */,
                          speed * 0x1.199998p+0f /* 0x3f8ccccc, 1.1 */);
    utlMemFree(p);

    m_move = new TskEnemyMove;
    g_taskMan.addTask(1, -1, 1, 0, m_move, 0, &m_move_id, 0x11);
    m_turn = new TskEnemyTurn;
    g_taskMan.addTask(1, -1, 1, 0, m_turn, 0, &m_turn_id, 0x11);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00164fc0
// Applies the pending decide input: the state it leads to becomes
// m_state and m_changed is set.

void EmyMain::exec()
{
    m_changed = 0;
    if (m_req != -1) {
        int next = m_map->next(m_req);
        if (next != 0) {
            m_state = next;
            m_changed = 1;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165010

void EmyMain::startBitten()
{
    if (m_emy_out != NULL)
        m_emy_out->startBitten();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165038

void EmyMain::updateOut()
{
    if (m_emy_out != NULL)
        m_emy_out->tickBitten();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165060

void EmyMain::stopOut()
{
    if (m_emy_out != NULL)
        m_emy_out->m_bitten = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165074
// 0 hidden; 1 waits m_grp_wait frames; 2 appears at pos facing f (1
// that frame); 4 sinks for 26 frames, then hides with its limbs back and
// its life restored and waits again (1). motion is not read.

int EmyMain::updateGroup(float f, int motion, const float *pos)
{
    (void)motion;
    switch (m_grp_state) {
    case 0:
        g_gamMan.hideObj(m_oid, 1);
        break;
    case 1:
        if (m_grp_wait == 0)
            m_grp_state = 2;
        else
            m_grp_wait--;
        break;
    case 2: {
        m_param->setParam(5, pos);
        float rot[4] = { 0.0f, f, 0.0f, 0.0f };
        m_param->setParam(6, rot);
        g_gamMan.setObj(m_oid, 0x40);
        g_gamMan.resetObj(m_oid, 0x10000000);
        g_gamMan.resetObj(m_oid, 0x400);
        m_param->setParam(0x1b, &m_lp);
        m_grp_state = 3;
        m_req = 0;
        g_emyManager.clearGrpBusy(m_grp_no);
        m_move->resume();
        m_turn->resume();
        return 1;
    }
    case 4: {
        float p[4];
        m_param->getPos(p);
        p[1] += 0.5f;
        m_param->setPos(p);
        m_grp_1d2++;
        if ((u_short)m_grp_1d2 > 25) {
            g_gamMan.hideObj(m_oid, 1);
            for (int i = 0; i < 5; i++)
                chrRestoreLimb(m_oid, i);
            m_param->m_flags |= 0x2000;
            m_param->m_flags |= 0x400;
            m_param->setParam(0x1b, &m_lp);
            m_grp_state = 1;
        }
        break;
    }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165280 // callcheck: dbgAssertFailed, i3rGetNodeIndexByName (DogModel::getNodeIndexByName inline)

void EmyMain::getBasePos(float *pos)
{
    ACHRstat *achr = g_gamMan.getAchr(m_oid);
    emyGetNodePos(g_gamMan.getActorNo(m_oid), "Bip01 Spine1", pos);
    float obj[4];
    achr->getPos(obj);
    pos[1] = obj[1];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165398

void EmyMain::setSE(int handle, int type, int n)
{
    stopAllSE();
    m_se[0].m_handle = handle;
    m_se[0].m_type = type;
    m_se[0].m_10 = 0;
    m_se[0].m_14 = 0;
    m_se[0].m_wait = utlRandPeakHi(1, n, 4);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165404

void EmyMain::addSE(int handle, int type, int n)
{
    for (int i = 0; i < 4; i++) {
        EmySERec &rec = m_se[i];
        if (rec.m_handle == 0) {
            rec.m_handle = handle;
            rec.m_10 = 0;
            rec.m_type = type;
            rec.m_14 = 0;
            rec.m_wait = utlRandPeakHi(1, n, 4);
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165498

void EmyMain::stopSE(int handle)
{
    for (int i = 0; i < 4; i++) {
        EmySERec &rec = m_se[i];
        if (rec.m_handle == handle) {
            if (rec.m_type < 3)
                g_gamMan.m_sndMan.funcSE(handle, 8, NULL);
            else
                g_gamMan.m_sndMan.funcStr(handle, 8, NULL);
            memset(&rec, 0, sizeof(rec));
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165570

void EmyMain::stopAllSE()
{
    for (int i = 0; i < 4; i++) {
        EmySERec &rec = m_se[i];
        int handle = rec.m_handle;
        if (handle != 0) {
            if (rec.m_type < 3)
                g_gamMan.m_sndMan.funcSE(handle, 8, NULL);
            else
                g_gamMan.m_sndMan.funcStr(handle, 8, NULL);
            memset(&rec, 0, sizeof(rec));
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165638

void EmyMain::clearSEType(int type)
{
    for (int i = 0; i < 4; i++) {
        if (m_se[i].m_type == type)
            memset(&m_se[i], 0, sizeof(m_se[i]));
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001656b4

bool EmyMain::testHittable()
{
    if (GetType() == 3 && m_state == 3)
        return false;
    return m_down_flg == 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165710

bool EmyMain::testStateFlag()
{
    if (m_down_flg != 0)
        return false;
    return m_state_flg[m_state] != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165734

bool EmyMain::isPCSameArea()
{
    if (g_gamMan.m_pcPtr == NULL)
        return false;
    int pc, own[3];
    emyPC()->getParam(8, &pc);
    m_param->getParam(8, own);
    return pc == own[0];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165794 // callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (the double angle wrap, emyAbsAngle)

int EmyMain::testBarkReact()
{
    if (m_bark_cool >= 0) {
        m_bark_cool--;
        return 0;
    }
    if (!magBwnIsBarkPhase3())
        return 0;
    float dog[4], base[4];
    emyGetDogPos(dog);
    getBasePos(base);
    if (--m_bark_wait >= 0 || !emyChkNearXZ(300.0f, base, dog))
        return 0;
    float a = atan2f(base[0] - dog[0], base[2] - dog[2]);
    float rot = g_magSystem.m_scene.m_actors[g_gamMan.m_dogPtr->m_actor].getRotY();
    if ((double)emyAbsAngle(a - rot) < 0.7853981633974483 /* pi / 4 */) {
        m_bark_wait = utlRandRange(0, 20);
        m_bark_cool = utlRandRange(60, 120);
        return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165a68

int EmyMain::isPCStateClass1()
{
    if (g_magSystem.getPCStateClass() == 1)
        return 1;
    return magBwnReturn0() == 0 ? 1 : 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165aa8 // callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (the double angle wrap, emyAbsAngle)

int EmyMain::isPCAhead()
{
    if ((m_param->m_attr[0] & 1) == 1)
        return 0;
    if ((u_int)(g_magSystem.getPCStateClass() - 6) >= 3)
        return 0;
    float pc[4], base[4];
    emyGetPCPos(pc);
    getBasePos(base);
    float a = atan2f(pc[0] - base[0], pc[2] - base[2]);
    float rot = g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(m_oid)].getRotY();
    return (double)emyAbsAngle(a - rot) < 0.39269908169872414 /* pi / 8 */ ? 1 : 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165d38

int EmyMain::isPCStateClass2()
{
    if (g_magSystem.getPCStateClass() == 2)
        return 1;
    return magBwnReturn0() == 1 ? 1 : 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00165d7c // callcheck: fptodp, dpcmp, dpadd, dpsub, dptofp (the double angle wrap, emyAbsAngle)

int EmyMain::isPCClass7OffAxis()
{
    if ((m_param->m_attr[0] & 1) == 1)
        return 0;
    if (g_magSystem.getPCStateClass() != 7)
        return 0;
    float base[4], pc[4];
    getBasePos(base);
    emyGetPCPos(pc);
    float a = atan2f(pc[0] - base[0], pc[2] - base[2]);
    float rot = g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(m_oid)].getRotY();
    return (double)emyAbsAngle(a - rot) > 0.0 ? 1 : 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166004

int EmyMain::isPCNear()
{
    float base[4], pc[4];
    getBasePos(base);
    emyGetPCPos(pc);
    return emyChkNearXZ(60.0f, base, pc);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166048

int EmyMain::isPCOrDogInSight()
{
    u_int pc = g_gamMan.m_pcPtr != NULL ? g_gamMan.m_pcPtr->m_oid : 0;
    int a = emyChkInSight(60.0f, 0x1.921fb4p+2f /* 0x40c90fda, 2 pi */, 0, pc, m_param);
    int b = emyChkInSight(60.0f, 0x1.921fb4p+2f, 0, g_gamMan.m_dogPtr->m_oid, m_param);
    return a != 0 || b != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166108

bool EmyMain::isLowLife()
{
    return m_param->m_lp < ((u_short)m_lp >> 2);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166128

int EmyMain::hasOid0Twice()
{
    if (g_emyManager.hasOid(0) && g_emyManager.hasOid(0))
        return 1;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166178

bool EmyMain::isHalfLife()
{
    return m_param->m_lp < ((u_short)m_lp >> 1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166198

int EmyMain::isPCStateClass4()
{
    if (g_magSystem.getPCStateClass() == 4)
        return 1;
    return magBwnReturn0() == 2 ? 1 : 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001661dc

void EmyMain::notify(int bits)
{
    if ((u_int)(bits - 1) < 2 || bits == 4 || bits == 8)
        m_damage = 1;
    m_notify |= bits;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016621c
// Param 0x66 is the item's kind: 0xa00 means none (both params cleared);
// otherwise a type-5 oid gets the kind's model (field 0xf, or addChr
// without one) and is attached to the enemy.

void EmyMain::createHeldItem()
{
    short kind;
    short model;
    int item[3];
    m_param->getParam(0x66, &kind);
    if (kind == 0xa00) {
        kind = 0;
        item[0] = 0;
        m_param->setParam(0x66, &kind);
        m_param->setParam(0x67, item);
        return;
    }
    item[0] = (int)g_gamMan.makeOid(2, 5, -1);
    m_param->setParam(0x67, item);
    if (item[0] == 0) {
        kind = 0;
        m_param->setParam(0x66, &kind);
        return;
    }
    m_param->getParam(0x66, &kind);
    g_gamMan.m_sysData.getField((u_short)kind, 0xf, &model);
    if (model == -1)
        g_gamMan.addChr(-1, item[0], (u_short)kind, NULL, 0);
    else
        g_gamMan.createObjByModel((u_short)model, item[0], (u_short)kind, NULL, -1, 1);
    emyAttachItem(m_oid, item[0]);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166364

void EmyMain::setGroup(int no)
{
    m_grp_no = no;
    m_grp_state = 0;
    m_grp_respawn = 0;
    m_grp_wait = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166378

void EmyMain::setManageSw(int on)
{
    m_manage_sw = on;
    if (on == 0) {
        m_param->pauseTasks(0, 1);
        pauseMove(1);
        m_param->m_flags |= 0x400;
        m_param->m_flags |= 0x2000;
    } else {
        pauseMove(0);
        m_param->m_flags &= ~0x400u;
        m_param->m_flags &= ~0x2000u;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016640c
// PC: the ROM passes an unset stack word (getParam was probably meant);
// the host's is 0. No callers.

int EmyMain::setLPFromStack()
{
    int lp[4] = { 0, 0, 0, 0 };
    m_param->setParam(0x1b, lp);
    return lp[0];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166434

void EmyMain::callFunc()
{
    (this->*m_func)();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x0016649c

void EmyMain::setDecideSw(int on)
{
    m_decide_sw = on;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001664a4

void EmyMain::pauseMove(int on)
{
    m_param->pauseTasks(on, 0);
    if (on == 1) {
        m_move->suspend();
        m_turn->suspend();
        return;
    }
    m_move->resume();
    m_turn->resume();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166518

void EmyMain::resetOut()
{
    m_emy_out->dropItem();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166534

void EmyMain::countDamageEnd()
{
    if (--m_damage_end < 1) {
        m_damage_end = 0;
        m_param->m_flags &= ~0x400u;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166568

u_int EmyMain::pickSE(const u_int *pair)
{
    u_int i = m_parsonality;
    if (i == 2)
        i = utlRandRange(0, 99) > 49 ? 1 : 0;
    return pair[i];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001665b8
// The SE id of no for this enemy's type: fixed ids, a per-parsonality
// pick (pickSE) or a row by parsonality; nos past 14 come from one table.
// 0 when the type has none.

u_int EmyMain::getSE(int no)
{
    if (no > 14)
        return s_seData[(0x2f171c - 0x2f16b8) / 4 + no];
    int type = GetType();
    switch (no) {
    case 0:  return 0x0c000368;
    case 1:  return 0x0c000369;
    case 2:  return 0x0c000376;
    case 3:
    case 4:  return 0;
    case 5:  return 0x0c00037c;
    case 6:  return pickSE(EMY_SE(0x2f16f0));
    case 7:  return pickSE(EMY_SE(0x2f16f8));
    case 8:
        switch (type) {
        case 0:  return pickSE(EMY_SE(0x2f1738));
        case 1:  return 0x0c000393;
        case 2:  return pickSE(EMY_SE(0x2f1740));
        case 3:  return 0x0c0204c2;
        default: return 0;
        }
    case 9:
        switch (type) {
        case 0:  return pickSE(EMY_SE(0x2f1700));
        case 1:  return 0x0c000394;
        case 2:  return pickSE(EMY_SE(0x2f1708));
        case 3:  return 0x0c0204c4;
        case 4:  return 0x0c010467;
        case 5:  return 0x0c01046e;
        default: return 0;
        }
    case 10: return pickSE(EMY_SE(0x2f1710));
    case 11:
        switch (type) {
        case 0:  return pickSE(EMY_SE(0x2f1718));
        case 1:  return 0x0c000398;
        case 2:  return pickSE(EMY_SE(0x2f1720));
        case 3:  return 0x0c0204c8;
        case 4:  return 0x0c01046a;
        case 5:  return 0x0c010471;
        default: return 0;
        }
    case 12:
        switch (type) {
        case 0:  return 0x0c00037b;
        case 1:  return EMY_SE(0x2f16b8)[m_parsonality];
        case 2:  return pickSE(EMY_SE(0x2f16c8));
        case 3:  return EMY_SE(0x2f16d0)[m_parsonality];
        case 4:  return EMY_SE(0x2f16e0)[m_parsonality];
        case 5:  return 0x0c0204bf;
        default: return 0;
        }
    case 13:
        switch (type) {
        case 0:  return pickSE(EMY_SE(0x2f1728));
        case 1:  return 0x0c000395;
        case 2:  return pickSE(EMY_SE(0x2f1730));
        case 3:  return 0x0c0204c5;
        case 4:  return 0x0c010468;
        case 5:  return 0x0c01046f;
        default: return 0;
        }
    case 14:
        switch (type) {
        case 0:  return pickSE(EMY_SE(0x2f1748));
        case 1:  return 0x0c000396;
        case 2:  return pickSE(EMY_SE(0x2f1750));
        case 3:  return 0x0c0204c6;
        default: return 0;
        }
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001669a4

const EmyKindMoveParam *EmyMain::getMoveParam()
{
    u_short row;
    g_gamMan.m_sysData.getField(g_gamMan.getObjKind(m_oid), 0x20, &row);
    return &s_kindMove[row];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00166a0c

bool EmyMain::testNotifyHit()
{
    return (m_notify & 1) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// The ELF's linkonce task wrappers (inline in the class body).
// ROR 0x0028a910

void EmyMain::StartMove(const EmyMoveParam *prm)
{
    m_move->start(prm);
}

// ROR 0x0028a92c

void EmyMain::ActiveMove()
{
    m_move->m_active = 1;
}

// ROR 0x0028a958

void EmyMain::DamagePauseMove()
{
    m_move->suspend();
}

// ROR 0x0028a974

void EmyMain::DamageResumeMove()
{
    m_move->resume();
}

// ROR 0x0028a990

void EmyMain::StartTurn(const EmyTurnParam *prm)
{
    m_turn->start(prm);
}

// ROR 0x0028a9ac

void EmyMain::ActiveTurn()
{
    m_turn->m_active = 1;
}

// ROR 0x0028a9bc

void EmyMain::InactiveTurn()
{
    m_turn->m_active = 0;
}

// ROR 0x0028a9c8

void EmyMain::DamagePauseTurn()
{
    m_turn->suspend();
}

// ROR 0x0028a9e4

void EmyMain::DamageResumeTurn()
{
    m_turn->resume();
}
