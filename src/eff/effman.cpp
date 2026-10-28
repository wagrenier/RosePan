/* ==========================================================================
 *  eff/effman.cpp
 *
 *  The scene's effect manager EffMan (MagScene +0x837e0; 0x176a3c..0x1788ec,
 *  after EffFindItem1 0x176694..0x176a3c, the item-found sparkle):
 *  its table of Eff objects, their per-frame live and draw, and the effect
 *  packs it reads from data/eff/eff.bin. See eff/effman.h.
 * ======================================================================== */
#include "eff/effman.h"

#include <string.h>

#include "bed/bedpfh.h"
#include "common/ror_assert.h"
#include "common/rosepan_stub.h"
#include "dog/dogcamera.h"
#include "dog/doglightpool.h"
#include "eff/effclasses.h"
#include "dog/dogtexinfo.h"
#include "i3d/i3dctx.h"
#include "i3d/i3denv.h"
#include "i3d/i3dlayer.h"
#include "i3d/i3dmath.h"
#include "i3d/i3dpacket.h"
#include "i3d/i3dview.h"
#include "i3d/i3dvu1pkt.h"
#include "i3d/i3mroot.h"
#include "i3d/i3rmodel.h"
#include "i3d/i3rroot.h"
#include "mag/magscene.h"
#include "sdk/libgraph.h"
#include "utl/utlmisc.h"

MagScene   *g_effScene;                         /* 0x298e08 */
i3dContext *g_effCtx;                           /* 0x298e0c */

/* The pack name of each effect kind (0x2f2cd0; the ROM copies it to the
 * stack on every call). Kinds 0x33 / 0x34 take a suffix from their param. */
static const char *const s_pack_name[54] =
{
    "",             "bloodspray1",  "bloodspray2",  "bloodstay1",   /* 0x00 */
    "bloodstay2",   "bloodstay3",   "bloodstay3",   "bloodstay4",
    "breath1",      "breath1",      "breath2",      "breath2",      /* 0x08 */
    "breath2",      "",             "",             "",
    "",             "cloud1",       "dust2",        "dust2",        /* 0x10 */
    "fire1",        "fire1",        "fire2",        "fog1",
    "fog2",         "fog2",         "fog3",         "fog3",         /* 0x18 */
    "fog3",         "foot1",        "foot1",        "foot2",
    "foot2",        "light1",       "light2",       "",             /* 0x20 */
    "light4",       "light5",       "muzzleflash1", "reload1",
    "",             "steam1",       "steam1",       "steam2",       /* 0x28 */
    "steam2",       "steam3",       "steam3",       "water1",
    "",             "water3",       "finditem1",    "gr1",          /* 0x30 */
    "bg1",          "",
};

// ──────────────────────────────────────────────────────────────────────
// The param's position (w 1) and pull toward the camera, the blink from
// its first frame.
// ROR 0x00176694

int EffFindItem1::init(const EffParam *param)
{
    int ok = Eff0::init(param);
    if (ok)
    {
        m_pos[0] = param->m_find.m_pos[0];
        m_pos[1] = param->m_find.m_pos[1];
        m_pos[2] = param->m_find.m_pos[2];
        m_pos[3] = 1.0f;
        m_near = param->m_find.m_near;
        m_frame = 0;
    }
    return ok != 0;
}

// ──────────────────────────────────────────────────────────────────────
// A 45-frame cycle; it never ends by itself.
// ROR 0x00176700

int EffFindItem1::liveMain()
{
    m_frame = (u_char)((m_frame + 1) % 45);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Drawn during the first 15 frames of the cycle.
// ROR 0x00176728

void EffFindItem1::render(RENDERPARAM *param)
{
    if (m_frame < 15)
        Eff0::render(param);
}

// ──────────────────────────────────────────────────────────────────────
// One camera-facing sprite (40 x 40, half transparent, ALPHA_1 0x48) at
// m_pos pulled m_near toward the eye. The texture follows the frame: the
// first entry whose end the frame has not passed (the ROM builds both
// tables on the stack).
// ROR 0x00176754

u_long *EffFindItem1::renderMain(RENDERPARAM *param, const I3dDrawSet *set, u_long *pkt)
{
    static const u_int s_end[8] = { 2, 3, 4, 8, 10, 12, 14, 0 };
    static const int   s_tex[8] = { 0, 1, 2, 3, 2, 1, 0, -1 };
    static const float s_scale[4] = { 40.0f, 40.0f, 1.0f, 1.0f };   // 0x2f2a60
    static const float s_color[4] = { 1.0f, 1.0f, 1.0f, 0.5f };     // 0x2f2a70

    int i = 0;
    if (m_frame > 2)
    {
        for (i = 1; s_tex[i] != -1 && s_end[i] < m_frame; i++)
            ;
    }
    pkt = i3dPktSetReg(pkt, SCE_GS_ALPHA_1, 0x48);
    pkt = i3dPktCallView(pkt, param->m_cur);
    pkt = i3dPktRefTexRegs(pkt, i3dDrawSetUseTex(const_cast<I3dDrawSet *>(set), s_tex[i]));

    /* The rectangle, then the sprite's centre (the ROM reuses the buffer). */
    float pos[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    pkt = i3dPktBeginVu1Sprite(pkt, param->m_cur, 1);
    pkt = i3dPktVu1Rect(pkt, pos);

    DogCamera *cam = &g_effScene->m_camera_entity;
    cam->callTransform();
    sceVu0FMATRIX m;
    memcpy(m, cam->m_matrix, sizeof(m));
    effMoveToward(m_near, pos, m_pos, m[3]);
    i3dMatScale(m, m, s_scale);
    pkt = i3dPktVu1Matrix(pkt, m);
    pkt = i3dPktVu1Sprite(pkt, pos, s_color);
    return i3dPktEndVu1Sprite(pkt);
}

// ──────────────────────────────────────────────────────────────────────
// Sorted by its depth less the pull toward the camera.
// ROR 0x001769ec

void EffFindItem1::updateZDepth()
{
    m_z = effCalcViewDepth(&g_effScene->m_camera_entity, m_pos) - m_near;
}

// ──────────────────────────────────────────────────────────────────────
// The member constructor (MagScene::MagScene): an empty table and draw
// order, no index, no packs. The weak FIFO and the singleton numbers wait
// for init.
// callcheck: FileReader, memset (the member constructor the host runs; the clears below)
// ROR 0x00176a3c

EffMan::EffMan()
{
    m_scene = NULL;
    m_next_no = 0;
    m_num_eff = 0;
    memset(m_eff, 0, sizeof(m_eff));
    memset(m_order, 0, sizeof(m_order));
    m_index = NULL;
    memset(m_pack, 0, sizeof(m_pack));
}

// ──────────────────────────────────────────────────────────────────────
// PORT: the explicit part (term) is left out, as in ~MagScene: the ROM
// never destroys the scene, and on the host only the static teardown
// reaches it, with the game threads still running (open-items.md
// Decisions). The FileReader member closes eff.bin itself.
// ROR 0x00176aa0 // callcheck: term, ~FileReader, operator_delete (left out on the host, see above; the member destructor the host runs)

EffMan::~EffMan()
{
}

// ──────────────────────────────────────────────────────────────────────
// The scene and context (for the Eff classes too), the draw order 0..0x10f,
// a fresh random table, eff.bin's index, and the packs of the blood,
// item, foot, muzzle-flash, reload and fog effects, kept across maps.
// ROR 0x00176afc

int EffMan::init(i3dContext *ctx, MagScene *scene)
{
    m_scene = scene;
    m_next_no = 0;
    m_num_eff = 0;
    memset(m_eff, 0, sizeof(m_eff));
    for (int i = 0; i < EFF_MAX; i++)
        m_order[i] = i;
    g_effScene = m_scene;
    m_no_1f = -1;
    m_no_1a = -1;
    m_num_weak = 0;
    g_effCtx = ctx;
    effInitRandTable();
    loadIndex();
    loadPack(1, NULL, 1);
    loadPack(2, NULL, 1);
    loadPack(3, NULL, 1);
    loadPack(4, NULL, 1);
    loadPack(0x32, NULL, 1);
    loadPack(0x1f, NULL, 1);
    loadPack(0x26, NULL, 1);
    loadPack(0x27, NULL, 1);
    loadPack(0x1a, NULL, 1);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Every effect and every pack (kept ones too) released, the index freed,
// and a read with only the close step left closes eff.bin.
// ROR 0x00176c50

void EffMan::term()
{
    releaseAll();
    releasePacks(1);
    if (m_index != NULL)
    {
        g_utlMemory.free(m_index);
        m_index = NULL;
    }
    m_req.m_buf = NULL;
    m_req.m_close = 1;
    m_reader.read(&m_req, 0);   // PC: the file is closed already; this closes nothing
    m_scene = NULL;
    g_effScene = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// News the class of param->m_kind (eff/effclasses.h), hands it its pack
// (kinds 0x33 / 0x34 name theirs with the param's suffix), then init and
// a table entry; weak effects join the eviction FIFO. Kinds 0x1a and 0x1f
// are singletons: while one lives its number is returned instead.
// PC: the ROM goes on with NULL for a kind without a class (a crash), and
// when init or addEff fails it reads the deleted object's flags and
// returns the pack slot number left in the register (pushing it on the
// weak FIFO when the stale flags say weak); the host returns -1 for all
// three and touches nothing.
// callcheck: operator_new, Eff0, EffBloodSpray1, EffBloodSpray2, EffLight3, EffReload1, EffWater1, EffWater3, EffGeneric1, EffBg1 (new X runs its constructor; the inline ones run Eff0's)
// ROR 0x00176cc8

int EffMan::createEff(EffParam *param)
{
    Eff0 *eff = NULL;
    switch (param->m_kind)
    {
    case 0x01: eff = new EffBloodSpray1;    break;
    case 0x02: eff = new EffBloodSpray2;    break;
    case 0x03: eff = new EffBloodStay1;     break;
    case 0x04: eff = new EffBloodStay2;     break;
    case 0x05: eff = new EffBloodStay3;     break;
    case 0x06: eff = new EffBloodStay3Prim; break;
    case 0x08: eff = new EffBreath1;        break;
    case 0x09: eff = new EffBreath1Space;   break;
    case 0x0a: eff = new EffBreath2;        break;
    case 0x0b: eff = new EffBreath2Prim;    break;
    case 0x0c: eff = new EffBreath2Ground;  break;
    case 0x0d: eff = new EffCameraAnim1;    break;
    case 0x11: eff = new EffCloud1;         break;
    case 0x12: eff = new EffDust2;          break;
    case 0x13: eff = new EffDust2Prim;      break;
    case 0x14: eff = new EffFire1;          break;
    case 0x15: eff = new EffFire1Ex;        break;
    case 0x16: eff = new EffFire2;          break;
    case 0x17: eff = new EffFog1;           break;
    case 0x18: eff = new EffFog2;           break;
    case 0x19: eff = new EffFog2Prim;       break;
    case 0x1a:
        if (m_no_1a != -1)
            return m_no_1a;
        eff = new EffFog3;
        break;
    case 0x1b: eff = new EffFog3Dark;       break;
    case 0x1c: eff = new EffFog3WeakUp;     break;
    case 0x1d: eff = new EffFoot1;          break;
    case 0x1e: eff = new EffFoot1Prim;      break;
    case 0x1f:
        if (m_no_1f != -1)
            return m_no_1f;
        eff = new EffFoot2;
        break;
    case 0x20: eff = new EffFoot2Prim;      break;
    case 0x21: eff = new EffLight1;         break;
    case 0x22: eff = new EffLight2;         break;
    case 0x23: eff = new EffLight3;         break;
    case 0x26: eff = new EffMuzzleFlash1;   break;
    case 0x27: eff = new EffReload1;        break;
    case 0x29:
    case 0x2d: eff = new EffSteam1;         break;
    case 0x2a:
    case 0x2e: eff = new EffSteam1Prim;     break;
    case 0x2b: eff = new EffSteam2;         break;
    case 0x2c: eff = new EffSteam2Prim;     break;
    case 0x2f: eff = new EffWater1;         break;
    case 0x31: eff = new EffWater3;         break;
    case 0x32: eff = new EffFindItem1;      break;
    case 0x33: eff = new EffGeneric1;       break;
    case 0x34: eff = new EffBg1;            break;
    }
    if (eff == NULL)
        return -1;

    int pack;
    if (param->m_kind == 0x33 || param->m_kind == 0x34)
        pack = loadPack(param->m_kind, param->m_suffix, 0);
    else
        pack = loadPack(param->m_kind, NULL, 0);
    param->m_pack = pack == -1 ? NULL : &m_pack[pack];

    if (eff->init(param) == 0)
    {
        delete eff;
        return -1;
    }
    int no = addEff(eff);
    if (no == -1)
    {
        delete eff;
        return -1;
    }
    if (eff->m_flags & EFF_FLAG_WEAK)
        pushWeak(no);
    if (param->m_kind == 0x1a)
        m_no_1a = no;
    else if (param->m_kind == 0x1f)
        m_no_1f = no;
    return no;
}

// ──────────────────────────────────────────────────────────────────────
// eff into a free entry; when all 0x100 are in use, the oldest weak
// effect is retired to make room. Its number, -1 when none is free.
// ROR 0x001774e0

int EffMan::addEff(Eff0 *eff)
{
    int no = findFreeNo();
    if (no == -1)
    {
        no = popWeak();
        if (no == -1)
            return -1;
        retireEff(no);
    }
    m_eff[no] = eff;
    m_num_eff++;
    return no;
}

// ──────────────────────────────────────────────────────────────────────
// Deletes entry no: off the weak FIFO, and no longer the kind-0x1a /
// kind-0x1f singleton when it was one.
// ROR 0x0017756c

void EffMan::release(int no)
{
    if (m_eff[no] == NULL)
        return;
    if (no == m_no_1a)
        m_no_1a = -1;
    else if (no == m_no_1f)
        m_no_1f = -1;
    if (m_eff[no]->m_flags & EFF_FLAG_WEAK)
        removeWeak(no);
    if (m_eff[no] != NULL)
        delete m_eff[no];
    m_eff[no] = NULL;
    m_num_eff--;
}

// ──────────────────────────────────────────────────────────────────────
// Every entry (retired ones too) deleted, the counters reset, and the
// packs that are not kept released.
// ROR 0x0017762c

void EffMan::releaseAll()
{
    for (int i = 0; i < EFF_MAX; i++)
    {
        if (m_eff[i] != NULL)
        {
            delete m_eff[i];
            m_eff[i] = NULL;
        }
    }
    m_num_eff = 0;
    m_no_1f = -1;
    m_no_1a = -1;
    m_num_weak = 0;
    releasePacks(0);
}

// ──────────────────────────────────────────────────────────────────────
// The first empty entry of 0..0xff from m_next_no on (round robin; the
// one after it is where the next search starts); -1 when all are used.
// ROR 0x001776c4

int EffMan::findFreeNo()
{
    int no = m_next_no;
    for (int n = 0; ; )
    {
        int next = (no + 1) % EFF_MAX_LIVE;
        if (m_eff[no] == NULL)
        {
            m_next_no = next;
            return no;
        }
        if (++n > EFF_MAX_LIVE - 1)
            return -1;
        no = next;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Once a frame: the retired effects run until their live returns 0 and
// are deleted then; the live ones are released when theirs does.
// ROR 0x0017772c

void EffMan::live()
{
    for (int i = EFF_MAX_LIVE; i < EFF_MAX; i++)
    {
        Eff0 *eff = m_eff[i];
        if (eff != NULL && eff->live() == 0)
        {
            if (m_eff[i] != NULL)
                delete m_eff[i];
            m_eff[i] = NULL;
        }
    }
    if (m_num_eff != 0)
    {
        for (int i = 0; i < EFF_MAX_LIVE; i++)
        {
            Eff0 *eff = m_eff[i];
            if (eff != NULL && eff->live() == 0)
                release(i);
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// The world pass's effects: two layers at the camera (850 depth-tested
// GEQUAL, 855 always passing; both alpha-tested GEQUAL 0, CLAMP_1 0),
// every effect's depth, the draw order sorted far to near, the per-kind
// draw sets cleared, then each effect renders. Nothing while no live
// entry is in use.
// ROR 0x00177830

void EffMan::draw(i3dContext *ctx)
{
    if (m_num_eff == 0)
        return;

    Eff0::RENDERPARAM param;
    param.m_layer[0] = createLayer(ctx, 850.0f);
    u_long *pkt = i3dLayerBeginPacket(param.m_layer[0]);
    pkt = i3dPktSetReg(pkt, SCE_GS_CLAMP_1, 0);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x5000d);
    i3dLayerEndPacket(param.m_layer[0], pkt);
    param.m_layer[1] = createLayer(ctx, 855.0f);
    pkt = i3dLayerBeginPacket(param.m_layer[1]);
    pkt = i3dPktSetReg(pkt, SCE_GS_CLAMP_1, 0);
    pkt = i3dPktSetReg(pkt, SCE_GS_TEST_1, 0x3000d);
    i3dLayerEndPacket(param.m_layer[1], pkt);

    for (int i = 0; i < EFF_MAX; i++)
    {
        if (m_eff[i] != NULL)
            m_eff[i]->updateZDepth();
    }
    int n = sortDrawOrder();
    effClearDrawSets();
    for (int i = 0; i < n; i++)
        m_eff[m_order[i]]->render(&param);
}

// ──────────────────────────────────────────────────────────────────────
// The used entries to the front of the draw order, then an insertion
// sort by m_z, largest (farthest) first. The number of used entries.
// ROR 0x001779b8

int EffMan::sortDrawOrder()
{
    int n = packDrawOrder();
    Eff0 *prev = m_eff[m_order[0]];
    for (int i = 1; i < n; i++)
    {
        Eff0 *cur = m_eff[m_order[i]];
        if (prev->m_z < cur->m_z)
        {
            insertDrawOrder(i);
            cur = m_eff[m_order[i]];
        }
        prev = cur;
    }
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// Moves each run of empty entries in the draw order to its end (the
// order of the rest kept); returns where the first empty one now is
// (0x110 when none is empty).
// callcheck: memcpy (the ROM's memmove for a run of one is a plain copy here)
// ROR 0x00177a8c

int EffMan::packDrawOrder()
{
    int tmp[EFF_MAX];
    for (int i = 0; i < EFF_MAX; i++)
    {
        if (m_eff[m_order[i]] != NULL)
            continue;
        int j = i + 1;
        while (j < EFF_MAX && m_eff[m_order[j]] == NULL)
            j++;
        int run = j - i;
        if (j == EFF_MAX)
            return i;
        if (run == 1)
        {
            tmp[0] = m_order[i];
            memmove(&m_order[i], &m_order[j], (EFF_MAX - j) * sizeof(int));
            m_order[EFF_MAX - 1] = tmp[0];
        }
        else
        {
            memcpy(tmp, &m_order[i], run * sizeof(int));
            memmove(&m_order[i], &m_order[j], (EFF_MAX - j) * sizeof(int));
            memcpy(&m_order[EFF_MAX - run], tmp, run * sizeof(int));
        }
    }
    return EFF_MAX;
}

// ──────────────────────────────────────────────────────────────────────
// order[n] into the sorted order[0..n): a binary search for its depth (an
// equal depth stops the search where its lower bound is), then the rest
// moves up one.
// ROR 0x00177c00

void EffMan::insertDrawOrder(int n)
{
    Eff0 *eff = m_eff[m_order[n]];
    int lo = 0;
    int hi = n - 1;
    if (hi - lo >= 1)
    {
        float z = eff->m_z;
        for (;;)
        {
            int mid = lo + (hi - lo + 1) / 2;
            float mz = m_eff[m_order[mid]]->m_z;
            if (z < mz)
                lo = mid + 1;
            else if (z <= mz)
                break;
            else
                hi = mid - 1;
            if (hi - lo < 1)
                break;
            z = eff->m_z;
        }
    }
    if (eff->m_z < m_eff[m_order[lo]]->m_z)
        lo++;
    int no = m_order[n];
    memmove(&m_order[lo + 1], &m_order[lo], (n - lo) * sizeof(int));
    m_order[lo] = no;
}

// ──────────────────────────────────────────────────────────────────────
// A layer at key with the scene's light set 0, its fog when the map has
// one (fog record 0), and view record 0 at the camera.
// ROR 0x00177d20

I3dLayer *EffMan::createLayer(i3dContext *ctx, float key)
{
    I3dLayer *layer = i3dCtxCreateLayer(ctx, key);
    i3dLayerSetLight(layer, m_scene->m_light_pool.createLight(ctx, 0, 0, 1.0f));
    if (m_scene->m_2aa84 != 0)
        i3dLayerSetFog(layer, i3dCtxSetFog(ctx, &m_scene->m_fog[0]));
    MagScene *scene = m_scene;
    sceVu0FVECTOR *matrix = scene->m_camera_entity.getMatrix();
    i3dLayerSetView(layer, i3dCtxCreateView(ctx, &scene->m_view[0], matrix));
    return layer;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00177e04

void EffMan::pushWeak(int no)
{
    m_weak[m_num_weak] = no;
    m_num_weak++;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00177e24

void EffMan::removeWeak(int no)
{
    int count = m_num_weak;
    for (int i = 0; i < count; i++)
    {
        if (m_weak[i] == no)
        {
            memmove(&m_weak[i], &m_weak[i + 1], (count - 1 - i) * sizeof(int));
            m_num_weak--;
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// The oldest weak entry's number, taken off the FIFO; -1 when empty.
// ROR 0x00177e9c

int EffMan::popWeak()
{
    int no = -1;
    if (m_num_weak != 0)
    {
        no = m_weak[0];
        memmove(&m_weak[0], &m_weak[1], m_num_weak * sizeof(int) - sizeof(int));
        m_num_weak--;
    }
    return no;
}

// ──────────────────────────────────────────────────────────────────────
// Moves entry no into the first empty retired slot (0x100..0x10f) with
// its finish countdown at 0 (its next live finishes it), freeing the
// number. With all 16 retired slots used nothing moves (and addEff then
// overwrites the entry: the ROM loses that effect).
// ROR 0x00177ef8

void EffMan::retireEff(int no)
{
    for (int i = EFF_MAX_LIVE; i < EFF_MAX; i++)
    {
        if (m_eff[i] == NULL)
        {
            m_eff[i] = m_eff[no];
            m_eff[no] = NULL;
            m_eff[i]->m_life = 0;
            m_num_eff--;
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00177f54

void EffMan::makePackName(int kind, char *out, const char *suffix)
{
    const char *name = s_pack_name[kind];
    int len = (int)strlen(name);
    memcpy(out, name, len);
    if (suffix != NULL)
    {
        int n = (int)strlen(suffix);
        memcpy(out + len, suffix, n);
        len += n;
    }
    out[len] = '\0';
}

// ──────────────────────────────────────────────────────────────────────
// eff.bin's first 0x800 bytes (its RTPK index) into a game-heap buffer;
// the file stays open for readPack.
// ROR 0x001780c0

void EffMan::loadIndex()
{
    m_index = g_utlMemory.alloc(0x800, -1);
    m_req = FileReadReq();
    m_req.m_size = 0x800;
    m_req.m_buf = m_index;
    m_req.m_name = "data/eff/eff.bin";
    m_req.m_whence = 0;
    m_req.m_offset = 0;
    // PC: the ROM keeps eff.bin open from here to term (m_close 0); on the
    // host a stream left open stops the boot after the title (the intro's
    // ADPCM never opens, open-items.md), so every read opens and closes it.
    m_req.m_close = 1;
    m_reader.read(&m_req, 0);
    m_req.m_buf = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// The loaded pack whose name starts name (the comparison runs the pack
// name's length), -1. kind is not compared.
// ROR 0x00178148

int EffMan::findPack(int kind, const char *name)
{
    (void)kind;
    for (int i = 0; i < EFF_MAX_PACK; i++)
    {
        EffPack *pack = &m_pack[i];
        if (pack->m_kind != 0 && memcmp(pack->m_name, name, strlen(pack->m_name)) == 0)
            return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001781d0

int EffMan::findFreePack()
{
    for (int i = 0; i < EFF_MAX_PACK; i++)
    {
        if (m_pack[i].m_kind == 0)
            return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// kind's pack (its name, then suffix): the slot it is loaded in, read
// into a free slot when it is not; -1 for a kind without a pack or when
// the read fails.
// ROR 0x0017820c

int EffMan::loadPack(int kind, const char *suffix, int keep)
{
    EffPackReq req;
    makePackName(kind, req.m_name, suffix);
    int no = -1;
    if (req.m_name[0] != '\0')
    {
        no = findPack(kind, req.m_name);
        if (no == -1)
        {
            int n = findFreePack();
            // PC: the ROM goes on with slot -1 (the words before m_pack)
            // when all 16 are used; the host gives up.
            if (n < 0)
                return -1;
            req.m_kind = kind;
            req.m_pack = &m_pack[n];
            req.m_keep = keep;
            no = -1;
            if (readPack(&req) != 0)
                no = n;
        }
    }
    return no;
}

// ──────────────────────────────────────────────────────────────────────
// The pack req names, read from eff.bin into the game heap, and its files
// in order: the I3D_BIN models relocated, then the I3D_I3M motions
// relocated, then the TIM2 textures added to the scene's table (the
// first file of another kind ends the list: the effect's parameters);
// with textures the table goes to the context, and the models get it.
// callcheck: FUN_00215a30, clear (the assert handler inside ROR_ASSERT and the inline getNumFile / getFile; the clear before the constructor, which sets every member)
// ROR 0x001782cc

int EffMan::readPack(EffPackReq *req)
{
    EffPack *pack = req->m_pack;
    pack->m_ref = 0;
    pack->m_keep = req->m_keep;
    pack->m_kind = req->m_kind;
    memcpy(pack->m_name, req->m_name, sizeof(pack->m_name));

    BedPfh pfh(m_index);
    int n = pfh.getFileIndex(req->m_name);
    // PC: the ROM reads the size and offset tables at -1 for a name the
    // index lacks; the host leaves the slot free.
    if (n < 0)
    {
        pack->m_kind = 0;
        return 0;
    }
    int size = pfh.getFileSize(n);
    pack->m_data = g_utlMemory.alloc(size, -1);
    // PC: reopened for this read and closed after it (loadIndex).
    m_req.m_name = "data/eff/eff.bin";
    m_req.m_close = 1;
    m_req.m_whence = 0;
    m_req.m_offset = pfh.getFileOffset(n);
    m_req.m_size = size;
    m_req.m_buf = pack->m_data;
    m_reader.read(&m_req, 0);

    DogTexInfo *tex = &m_scene->m_tex_info;
    EffPackFile *file = pack->m_file;
    pfh = BedPfh(pack->m_data);
    int num = pfh.getNumFile();
    pack->m_num_i3r = 0;
    pack->m_num_tex = 0;
    pack->m_num_i3m = 0;

    int i = 0;
    for (; i < num; i++)
    {
        void *p = pfh.getFile(i);
        if (memcmp(p, "I3D_BIN", 7) != 0)
            break;
        i3rRelocate((I3rRoot *)p);
        (file++)->m_ptr = p;
        pack->m_num_i3r++;
    }
    for (; i < num; i++)
    {
        void *p = pfh.getFile(i);
        if (memcmp(p, "I3D_I3M", 7) != 0)
            break;
        i3mRelocate((I3mRoot *)p);
        (file++)->m_ptr = p;
        pack->m_num_i3m++;
    }
    for (; i < num; i++)
    {
        if (memcmp(pfh.getFile(i), "TIM2", 4) != 0)
            break;
        const char *name = pfh.getFileName(i);
        (file++)->m_tex = tex->addData(name, pfh.getFile(i), 0, -1);
        pack->m_num_tex++;
    }

    if (pack->m_num_tex > 0)
        tex->setTexTable(g_effCtx);
    for (int k = 0; k < pack->m_num_i3r; k++)
        i3rAssignTex((I3rRoot *)pack->m_file[k].m_ptr, tex->m_tex_info, tex->m_num_tex);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Pack n freed (a kept one only when forced): its textures out of the
// scene's table, its buffer back to the game heap, the slot cleared. Its
// users (m_ref) are not checked.
// ROR 0x001787b0

void EffMan::releasePack(int n, int force)
{
    EffPack *pack = &m_pack[n];
    if (pack->m_kind == 0)
        return;
    if (!force && (pack->m_keep & 1))
        return;
    DogTexInfo *tex = &m_scene->m_tex_info;
    /* The ROM zeroes the model and motion counts first and releases
     * m_file[0..m_num_tex): with models in the pack those words are
     * pointers, which release's range check ignores, and the textures
     * after them stay in the table. */
    int first = pack->m_num_i3r + pack->m_num_i3m;
    pack->m_num_i3r = 0;
    pack->m_num_i3m = 0;
    for (int i = 0; i < pack->m_num_tex; i++)
    {
        // PC: a pointer is no texture number on the host; skip what the
        // ROM's release ignores.
        if (i < first)
            continue;
        tex->release(pack->m_file[i].m_tex);
    }
    pack->m_num_tex = 0;
    if (pack->m_data != NULL)
    {
        g_utlMemory.free(pack->m_data);
        pack->m_data = NULL;
    }
    pack->m_ref = 0;
    pack->m_kind = 0;
    pack->m_name[0] = '\0';
    pack->m_keep = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00178894

void EffMan::releasePacks(int force)
{
    for (int i = 0; i < EFF_MAX_PACK; i++)
        releasePack(i, force);
}
