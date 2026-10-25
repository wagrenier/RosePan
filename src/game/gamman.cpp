// game/gamman.cpp -- the game manager GamMan (0x1c05c0..0x1d3b44; the file
// name is in its asserts and allocDebug tags).
//
// One global (bss 0x7812c0). main builds it through sysInitManagers (init),
// loads the common data (loadSystemData: one pack with the message data, two
// SE packs, the sound tables and the system script image) and the boot stage
// (loadStage: data/SEQ/W<stage>/W000.WRD -- the stage script image, the
// object table, two data files), then runs update every frame before the
// task manager. The rest -- items, the PC / dog / murderer objects, the
// pause masks, the script callbacks -- is written as the scripts reach it.

#include <stdio.h>
#include <string.h>

#include "common/rosepan_stub.h"
#include "game/gamman.h"

#include "bed/bedmemory.h"
#include "bed/bedpfh.h"
#include "game/gamfile.h"
#include "lib/unassigned.h"
#include "mag/magsystem.h"
#include "rx/rxsnd.h"
#include "sequence/sequence.h"
#include "utl/utlmisc.h"

static const char s_file[] = "game/gamman.cpp";

// ──────────────────────────────────────────────────────────────────────
// The rate word, the members' constructors (GamArea, GamSndMan), the item
// vector empty over its 12 slots and every node pool linked free.
// ROR 0x001c05c0 // callcheck: FUN_001dd1e8, FUN_001e0df0 (GamArea's and GamSndMan's constructors: the members' implicit constructors on the host)

GamMan::GamMan()
{
    m_rate = 1;
    m_18dc8 = 0;
    m_18dc4 = 0;
    m_itemHave.m_begin = m_itemHave.m_slot;
    m_itemHave.m_end = m_itemHave.m_slot;
    m_itemHave.m_capacity = 0xc;
    m_itemStock.build(m_itemStock.m_storage, 0x14e);
    m_pool1d240.build(m_pool1d240.m_storage, 4);
    m_achrList.build(m_achrList.m_storage, 0x80);
    m_fchrList.build(m_fchrList.m_storage, 0x40);
    m_vfchrList.build(m_vfchrList.m_storage, 0x180);
    m_avfchrList.build(m_avfchrList.m_storage, 0x40);
    m_pool1f880.build(m_pool1f880.m_storage, 0x80);
    m_pool202c0.build(m_pool202c0.m_storage, 0x80);
    m_pool20b00.build(m_pool20b00.m_storage, 0x80);
}

// ──────────────────────────────────────────────────────────────────────
// The flags, the chr numbers and pointers, a 0x100-node pool, the game-heap
// buffers (object table, file 2 of the stage pack, the event pack ...), the
// options at their defaults (brightness 5, master 4, SE / stream 8, output
// 0), the area and sound managers, init's table at +0x225f4, then the game
// state.
// ROR 0x001c0d70 // callcheck: operator_new (the host's new of the pool)

void GamMan::init()
{
    m_flags2 = 0x8000;
    m_pcEquipItemChrID = -1;
    m_vcount = 0;
    m_flags = 0;
    m_pcActor = -1;
    m_dogChrNo = -1;
    m_murderChrNo = -1;
    m_21ff4 = -1;
    m_21ff8 = -1;
    m_pcActorPtr = NULL;
    m_pcMotion = -1;
    m_pcPtr = NULL;
    m_dogPtr = NULL;

    m_pool2246c = new SeqFixedPool<GamRec16, 0x100>;
    m_pool2246c->build(m_pool2246c->m_storage, 0x100);
    m_pool2246c->clear();

    m_objTable = (GamObjTable *)g_utlMemory.allocDebug(0x5020, s_file, 342, 0, 1);
    m_objRec = m_objTable->m_rec;
    m_18bc0 = g_utlMemory.allocDebug(0x600, s_file, 345, 0, 1);
    m_18bc4 = g_utlMemory.allocDebug(0xc00, s_file, 347, 0, 1);
    m_1f240 = (u_char *)g_utlMemory.allocDebug(0x5a4, s_file, 350, 0, 1);
    m_evtPack = (u_char *)g_utlMemory.allocDebug(0xa0000, s_file, 351, 0, 1);
    m_wrdFile2 = (u_char *)g_utlMemory.allocDebug(0x4000, s_file, 353, 0, 1);
    m_1f248 = (u_char *)g_utlMemory.allocDebug(0x42000, s_file, 354, 0, 1);
    memset(m_1f240, 0, 0x5a4);

    m_optOutput = 0;
    m_optVibration = 1;
    m_21f94 = 1;
    m_optMasterLevel = 4;
    m_optBrightLevel = 5;
    m_optSeLevel = 8;
    m_optStrLevel = 8;
    m_saved_volume = -1;
    m_volume = 0x80;
    setOption(0, 5);
    setOption(1, m_optMasterLevel);
    setOption(2, m_optStrLevel);
    setOption(3, m_optSeLevel);
    setOption(6, m_optOutput);
    m_area.init();
    m_sndMan.init();
    m_22a84 = -1;
    memset(m_image, 0, sizeof(m_image));
    m_seqWord = 0;

    static const int s_225f4[0x1a] = {
        0x7f, 0x6f, 0x27f, 0x2ff, 3, 8, 4, 5, 7, 2, 9, 0xb, 0, 1,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    };
    for (int i = 0; i < 0x1a; i++)
        m_225f4[i] = s_225f4[i];

    m_objTable->m_num = 0;
    m_1a684 = 0xcccccccc;
    initGameState();
}

// ──────────────────────────────────────────────────────────────────────
// The game state a new game (seqFunc001) starts from: no equipped or found
// item, every task id -1, no script callback, the chr pools and the stock
// emptied, the item slots reset.
// ROR 0x001c143c

void GamMan::initGameState()
{
    m_findTargetItem = 0xa00;
    m_itemUseTaskId = -1;
    m_inuMode = 2;
    m_22280[1] = 10;
    m_22280[2] = 0xffff;
    m_22280[4] = 0x100;
    m_pcEquipItemOID = 0;
    m_equipItem = 0xa00;
    m_1d200 = 0;
    m_inuTask = -1;
    m_sysMsgHandle = -1;
    m_msgTaskId = -1;
    m_selectTaskId = -1;
    m_222a8 = -1;
    m_frameTaskId = -1;
    m_movieTaskId = -1;
    m_bgLoadTaskId = -1;
    m_camTaskId = -1;
    m_21f74 = (u_int)-1;
    for (int i = 0; i < 4; i++)
    {
        m_seqPair[i].m_b = 0;
        m_seqPair[i].m_a = 0;
    }
    m_22280[0] = 10;
    m_22280[3] = 0x100;
    m_22280[6] = 0;
    m_22280[5] = 0;
    for (int i = 0; i < 4; i++)
        m_22470[i].m_id = -1;
    for (int i = 0; i < 3; i++)
        m_22540[i].m_00 = -1.0f;
    m_22530 = 0;
    m_21f70 = 0;
    m_pool1d240.clear();
    m_pool1f880.clear();
    m_pool202c0.clear();
    m_pool20b00.clear();
    m_sndMan.clear_snd_lst(0);
    m_21fbc[0] = 35.0f;
    m_21fbc[1] = 40.0f;
    m_21fbc[2] = 55.0f;
    for (int i = 0; i < 4; i++)
        m_2236c[i][0] = 0;
    m_msgMode = 0;
    m_pcJnfc = 0;
    m_22004 = 0;
    m_22008 = 0;
    m_21f54 = 0;
    m_1f244 = 0;
    memset(m_1f24c, 0, sizeof(m_1f24c));
    m_225e4 = 0;
    m_225e8 = 0;
    m_225f0 = 0;
    markReset();
    clearItemHave();
    m_itemStock.clear();
    m_22ab0 = 0;
    resetExtra();
}

// ──────────────────────────────────────────────────────────────────────
// The common data: one pack (seqMakeGlobalPath kind 1) read whole into a
// 1 MB buffer -- file 0 the system data, 1 / 2 the SE packs 0000.se and
// 6400.se, 3 the sound table, 4 the system script image, 5 the stream
// table. The buffer is cut to the pack's size, then to 0x60000 once the
// script image is copied out.
// ROR 0x001c1b48 // callcheck: FUN_00215a30, BedPfh, FileReadReq, FileReader, ~FileReader (the assert handler inside BedPfh's inlines; the locals' constructors and destructor)

void GamMan::loadSystemData()
{
    m_sndMan.funcSnd(0x11, 0, 0, 0, -1);
    {
        FileReader reader;
        m_cur_stat = 0;
        void *buf = g_utlMemory.allocDebug(0x101480, s_file, 838, 0, 1);

        FileReadReq req;
        char path[0x40];
        req.m_name = seqMakeGlobalPath(path, 0x40, 1);
        req.m_size = 0x101440;
        req.m_close = 1;
        req.m_08 = 1;
        req.m_whence = 0;
        req.m_offset = 0;
        req.m_buf = buf;
        reader.read(&req, 0);

        BedPfh pfh(req.m_buf);
        void *top = (void *)pfh.getTop();
        utlMemRealloc(top, pfh.getTotalSize());
        m_sysData.set((const u_char *)pfh.getFile(0));
        m_sndMan.loadSePack("0000.se", pfh.getFile(1), 0);
        m_sndMan.loadSePack("6400.se", pfh.getFile(2), 0);
        m_sndMan.registSnd(0, pfh.getFile(3));
        g_sequence.loadSystemImage((const u_char *)pfh.getFile(4));
        m_sndMan.registStrSnd(0, pfh.getFile(5));
        utlMemRealloc(top, 0x60000);
    }
    gamSystemDataLoaded();
}

// ──────────────────────────────────────────────────────────────────────
// The old object table format (version != 1): the header, then count
// 0x30-byte records into the 0x50-byte slots (their last 0x20 bytes kept).
// ROR 0x001c1f84

void GamMan::convertObjTable(GamObjTable *dst, const GamObjTable *src, int count)
{
    memcpy(dst, src, 0x20);
    u_char *d = (u_char *)dst + 0x20;
    const u_char *s = (const u_char *)src + 0x20;
    for (; count > 0; count--)
    {
        memcpy(d, s, 0x30);
        d += 0x50;
        s += 0x30;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Stage loading: drops the scene's stage data and two SE slots, takes the
// stage's word table, keeps the PC and the dog across the change when the
// debug word asks, then reads data/SEQ/W<stage>/W000.WRD -- its header
// first (at most 3 sectors), then the whole pack: file 0 the stage script
// image, 1 the object table, 2 and 4 copied into their buffers. Then the
// stage's sounds, the camera at (0, 1000, 0) turned -90 degrees, and the PC
// and dog objects reset.
// ROR 0x001c204c // callcheck: FUN_00215a30, BedPfh, FileReadReq, FileReader, ~FileReader, memset (the assert handler inside ROR_ASSERT and BedPfh's inlines; the locals' constructors and destructor; memset clears the two vectors, initializers here)

void GamMan::loadStage(int stage)
{
    GamSndMan *snd = &m_sndMan;
    g_magSystem.clearScene22cc();
    snd->freeSePack(0xc);
    snd->freeSePack(0x1f);
    g_wrdTable.load(stage);
    m_flags2 |= 0x3800;
    snd->m_mode = 1;
    if (m_chapter == 6 && (g_sequence.m_bits[0] & 0x8000) != 0)
        m_flags2 |= 0x20000000;
    if ((g_sequence.m_bits[2] & 0x8000) != 0)
    {
        if (m_pcActor != -1 && m_pcPtr != NULL)
        {
            m_pcPtr->save(m_pcSave);
            releaseChr(m_21fcc);
            m_pcActor = -1;
            m_pcMotion = -1;
        }
        if (m_dogChrNo != -1 && m_dogPtr != NULL)
        {
            m_dogPtr->save(m_dogSave);
            releaseChr(m_21fd0);
            m_dogChrNo = -1;
        }
    }
    g_sequence.m_bits[1] &= ~0x100u;

    {
        FileReader reader;
        m_chapter = stage;
        m_cur_stat = 1;
        char path[0x40];
        snprintf(path, 0x40, "data/SEQ/W%03d/W000.WRD", stage);
        alignas(64) u_char head[0x1800];    /* the ROM aligns a stack buffer to 64 */

        FileReadReq req;
        req.m_size = 0x1800;
        req.m_close = 1;
        req.m_08 = 1;
        req.m_whence = 0;
        req.m_offset = 0;
        req.m_name = path;
        req.m_buf = head;
        reader.read(&req, 0);
        BedPfh pfh1(req.m_buf);
        ROR_ASSERT(pfh1.getFileOffset(0) <= (2048) * 3); /* 1161 */

        int size = (pfh1.getTotalSize() + 0x7ff) / 2048 * 2048;
        void *buf = g_utlMemory.allocDebug(size, s_file, 1166, 0, 1);
        setStagePack(buf);

        FileReadReq req2;
        req2.m_08 = 1;
        req2.m_whence = 0;
        req2.m_offset = 0;
        req2.m_buf = buf;
        req2.m_close = 1;
        req2.m_name = path;
        req2.m_size = size;
        reader.read(&req2, 0);
        BedPfh pfh2(req2.m_buf);

        g_sequence.loadStageImage((const u_char *)pfh2.getFile(0));

        const GamObjTable *obj = (const GamObjTable *)pfh2.getFile(1);
        int num = obj->m_num;
        if (obj->m_version == 1)
        {
            memcpy(m_objTable, obj, 0x20);
            memcpy(m_objRec, obj->m_rec, num * 0x50);
        }
        else
        {
            convertObjTable(m_objTable, obj, num);
        }

        int end = pfh1.getFileOffset(3);
        int start = pfh2.getFileOffset(2);
        memcpy(m_wrdFile2, pfh2.getFile(2), end - start);
        end = pfh1.getFileOffset(5);
        start = pfh2.getFileOffset(4);
        memcpy(m_evtPack, pfh2.getFile(4), end - start);

        utlMemRealloc((void *)pfh2.getTop(), pfh2.getTotalSize());
        utlMemFree(buf);
        m_chapter = stage;
    }
    loadStageSnd(m_chapter);

    float pos[4] = { 0.0f, 1000.0f, 0.0f, 0.0f };
    float rot[4] = { -1.57079637f, 0.0f, 0.0f, 0.0f };
    setCameraMode(1, 0);
    DogCamera &cam = g_magSystem.m_scene.m_camera_entity;
    cam.m_trans[0] = pos[0];
    cam.m_trans[1] = pos[1];
    cam.m_trans[2] = pos[2];
    cam.m_rotate[0] = rot[0];
    cam.m_rotate[1] = rot[1];
    cam.m_rotate[2] = rot[2];
    cam.m_dirty = 1;
    cam.normalizeRotate();
    cam.m_dirty = 1;

    resetObj(m_pcPtr != NULL ? m_pcPtr->getOID() : 0, 1);
    resetObj(m_dogPtr != NULL ? m_dogPtr->getOID() : 0, 1);
}

// ──────────────────────────────────────────────────────────────────────
// Empty in the US build: loadStage hands it the stage pack's buffer.
// ROR 0x001c46f4

void GamMan::setStagePack(void *buf)
{
}

// ──────────────────────────────────────────────────────────────────────
// Empty in the US build; nothing calls it.
// ROR 0x001c1a88

void GamMan::empty1c1a88()
{
}

// ──────────────────────────────────────────────────────────────────────
// Empty in the US build; nothing calls it.
// ROR 0x001c4f30

void GamMan::empty1c4f30()
{
}

// ──────────────────────────────────────────────────────────────────────
// Every frame: the frame count (bit 0 of the flags every 25 frames, the
// reset bit 0x40000 cleared), the PC's and the dog's positions while the
// game is not halted (flags2 0x8000000), the sound, and the area rects
// against the PC when flags bit 2 asks.
// ROR 0x001c526c

void GamMan::update()
{
    gamClearFrameReq();
    m_vcount++;
    m_flags &= ~0x40001u;
    if (m_vcount % 25 == 0)
        m_flags |= 1;
    if ((m_flags2 & 0x8000000) == 0)
    {
        if (m_pcPtr != NULL)
        {
            m_pcPtr->updatePos(m_pcPos);
            m_pcPtr->getRot(m_pcRot);
        }
        if (m_dogPtr != NULL)
            m_dogPtr->updatePos(m_dogPos);
    }
    m_sndMan.update(m_pcPos);
    if ((m_flags & 2) != 0)
        m_area.update(m_pcPos, m_pcRot);
}

// ──────────────────────────────────────────────────────────────────────
// Empty in the US build (main's last call of the frame).
// ROR 0x001c53d8

void GamMan::postUpdate()
{
}

// ──────────────────────────────────────────────────────────────────────
// The camera mode: 1 and 3 go to the scene as 0 and 3, 2 follows the PC
// (word +0x21f70) when there is one, 4 and 5 pick map camera no of the
// current sky slot (none past its count: nothing changes); 4 does nothing
// but flag 0x20000 when the same camera is asked again.
// ROR 0x001c7210

void GamMan::setCameraMode(int mode, u_int no)
{
    no &= 0xffff;
    switch (mode)
    {
    case 1:
        g_magSystem.setCameraMode(0, 0, NULL);
        break;
    case 2:
        if (m_pcActor == -1)
            g_magSystem.setCameraMode(2, 0, NULL);
        else
            g_magSystem.setCameraMode(2, m_21f70, NULL);
        m_21f74 = (u_int)-1;
        m_flags &= ~0x20000u;
        break;
    case 3:
        g_magSystem.setCameraMode(3, 0, NULL);
        break;
    case 4:
    {
        if (g_gamMan.getMapCamNum() <= (int)no)
            return;
        int param[3] = { -1, -1, m_21f70 };
        no = m_mapModelNo << 8 | no;
        m_21f78 = no;
        if (m_21f74 == no)
        {
            m_flags |= 0x20000;
            break;
        }
        g_magSystem.setCameraMode(4, no, param);
        m_21f74 = m_21f78;
        m_flags &= ~0x20000u;
        break;
    }
    case 5:
    {
        if (g_gamMan.getMapCamNum() <= (int)no)
            return;
        int param[3] = { -1, -1, m_21f70 };
        no = m_mapModelNo << 8 | no;
        m_21f78 = no;
        g_magSystem.setCameraMode(5, no, param);
        m_21f74 = (u_int)-1;
        m_flags &= ~0x20000u;
        break;
    }
    default:
        break;
    }
    m_cameraMode = mode;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c7494 // callcheck: FUN_00215a30 (the assert handler, inside MagScene::getTenkyuCNum)

int GamMan::getMapCamNum()
{
    return g_magSystem.m_scene.getTenkyuCNum(m_mapModelNo);
}

// ──────────────────────────────────────────────────────────────────────
// The item vector emptied (its slots have no destructor) and refilled with
// 12 slots of item 0xa00.
// ROR 0x001c845c

void GamMan::clearItemHave()
{
    m_itemHave.m_capacity = 0xc;
    m_itemHave.m_end = m_itemHave.m_slot;
    m_itemHave.m_begin = m_itemHave.m_slot;
    GamItemSlot slot;
    memset(&slot, 0, sizeof(slot));     /* PC: stack garbage in the ROM */
    slot.m_id = 0xa00;
    for (int i = 11; i >= 0; i--)
        *m_itemHave.m_end++ = slot;
}

// ──────────────────────────────────────────────────────────────────────
// Empty in the US build (before the map menu).
// ROR 0x001c98b8

void GamMan::beforeMapMenu()
{
}

// ──────────────────────────────────────────────────────────────────────
// The options. Levels are clamped to 0..8; 0 brightness (32 + level * 24),
// 1 master volume (the same scale, applied through setVolume), 2 / 3 the
// stream / SE levels (the sound side reads them when funcSnd 0xf / 0xe
// sets the master volumes), 4 / 5 read 2 / 3 back into *(int *)arg, 6 the
// output mode (1 = the library's mode 0), 7 vibration.
// ROR 0x001c9c1c

int GamMan::setOption(u_int no, intptr_t arg)
{
    int value = (int)arg;
    switch (no)
    {
    case 0:
    {
        int v = value < 0 ? 0 : value;
        if (v >= 9)
            v = 8;
        g_bss_780d04 = (int)((float)v * 0.125f * 192.0f) + 32;
        break;
    }
    case 1:
    {
        int v = value < 0 ? 0 : value;
        if (v >= 9)
            v = 8;
        m_masterVolume = (int)((float)v * 0.125f * 192.0f + 32.0f);
        setVolume(m_volume);
        break;
    }
    case 2:
    {
        int v = value < 0 ? 0 : value;
        if (v >= 9)
            v = 8;
        m_optStrLevel = v;
        m_sndMan.funcSnd(0xf, m_sndMan.m_mstVolStr[0], m_sndMan.m_mstVolStr[1], 0, -1);
        break;
    }
    case 3:
    {
        int v = value < 0 ? 0 : value;
        if (v >= 9)
            v = 8;
        m_optSeLevel = v;
        m_sndMan.funcSnd(0xe, m_sndMan.m_mstVolSe[0], m_sndMan.m_mstVolSe[1], 0, -1);
        break;
    }
    case 4:
        *(int *)arg = m_optStrLevel;
        break;
    case 5:
        *(int *)arg = m_optSeLevel;
        break;
    case 6:
        m_optOutput = value;
        rxSndSetOutputMode(value != 1);
        break;
    case 7:
        m_optVibration = value != 0;
        g_magSystem.m_vib_on = m_optVibration;
        break;
    default:
        break;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cfb38

void GamMan::setSysMsgHandle(int handle)
{
    m_sysMsgHandle = handle;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d1a48

int GamMan::isMenuOpen()
{
    return g_magSystem.m_scene.m_menu_id != -1;
}

// ──────────────────────────────────────────────────────────────────────
// The reset bit; update clears it the next frame.
// ROR 0x001d3588

void GamMan::markReset()
{
    m_flags |= 0x40000;
}

// ──────────────────────────────────────────────────────────────────────
// The volume scaled by the master volume (32..224 of 255, doubled) to the
// scene's sound life object; nothing while flags bit 0x40 holds it.
// ROR 0x001d3a38

void GamMan::setVolume(int volume)
{
    if ((m_flags & 0x40) != 0)
        return;
    m_volume = volume;
    float v = (float)(volume * m_masterVolume) / 255.0f;
    g_magSystem.m_scene.m_life_418f0.setVolume((int)(v + v));
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d3ac8

void GamMan::switchVolume(int on)
{
    if (on != 0)
    {
        if (m_saved_volume != -1)
        {
            setVolume(m_saved_volume);
            m_saved_volume = -1;
        }
        return;
    }
    m_saved_volume = m_volume;
    setVolume(0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d84a8

int GamMan::resetExtra()
{
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// PC: the ROM indexes the scene's actor array with whatever number it got,
// -1 included (memory before the array); the host skips that one.

static void gamShowActor(int no)
{
    if (no < 0)
        return;
    g_magSystem.m_scene.m_actors[no].setHide(0);
}

// ──────────────────────────────────────────────────────────────────────
// The end of the stage script's main thread. After a stage change (flags2
// 0x40) the dog's mode is set again, the PC and the dog get their saved
// state back, the start records reset their objects' table words, the
// options apply once (until flags2 0x400 is set) and the find target is
// re-sent; the debug restart word does the PC / dog part on its own.
// ROR 0x001c4168

void GamMan::onStageSeqEnd()
{
    if ((m_flags2 & 0x40) != 0)
    {
        setInuMode(m_inuMode);
        if (m_pcPtr != NULL)    /* PC: the ROM does not test it */
            m_pcPtr->load(m_pcSave);
        if (m_dogPtr != NULL)
            m_dogPtr->load(m_dogSave);
        const GamObjReset *rec = (const GamObjReset *)m_18bc0;
        if (rec->m_oid != 0)
        {
            u_int word = rec->m_flags;
            for (;;)
            {
                ctlObjParam((u_int)(int)rec->m_oid, 0x11, &word, 0);
                word = rec->m_0c;
                ctlObjParam((u_int)(int)rec->m_oid, 4, &word, 0);
                if (rec[1].m_oid == 0)
                    break;
                word = rec[1].m_flags;
                rec++;
            }
        }
        if ((m_flags2 & 0x400) == 0)
        {
            setOption(0, m_optBrightLevel);
            setOption(1, m_optMasterLevel);
            setOption(2, m_optStrLevel);
            setOption(3, m_optSeLevel);
            setOption(6, m_optOutput);
            setOption(7, m_optVibration);
            m_flags2 &= ~0x400u;
        }
        if (m_findTargetItem != 0xa00)
            setFindTargetItem(m_findTargetItem);
    }
    if ((g_sequence.m_bits[2] & 0x8000) != 0 && m_pcPtr != NULL)
    {
        m_pcPtr->load(m_pcSave);
        if (m_dogPtr != NULL)   /* PC: the ROM does not test it */
            m_dogPtr->load(m_dogSave);
        g_sequence.m_bits[2] &= ~0x8000u;
    }
    m_flags2 &= ~0x2000u;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c541c

void GamMan::setInuMode(int mode)
{
    m_inuMode = mode;
    switch (mode)
    {
    case 2:
        resume(0x200000);
        pause(0x100);
        if (m_dogPtr != NULL)
        {
            m_dogPtr->setHide(1);
            m_dogPtr->m_flags |= 0x10000000;
        }
        g_sequence.m_bits[1] &= ~2u;
        g_sequence.m_bits[1] &= ~4u;
        m_findTargetItem = 0xa00;
        gamSetBwnFollow(0);
        return;
    case 1:
        pause(0x200000);
        if (m_dogPtr != NULL)
        {
            m_dogPtr->setHide(0);
            m_dogPtr->m_flags &= ~0x10000000u;
        }
        g_sequence.m_bits[1] |= 2;
        g_sequence.m_bits[1] |= 4;
        resume(0x100);
        gamSetBwnFollow(1);
        return;
    case 3:
        pause(0x200000);
        pause(0x100);
        if (m_dogPtr != NULL)
        {
            m_dogPtr->setHide(1);
            m_dogPtr->m_flags |= 0x10000000;
        }
        g_sequence.m_bits[1] |= 2;
        g_sequence.m_bits[1] |= 4;
        gamSetBwnFollow(0);
        return;
    case 4:
        pause(0x200000);
        pause(0x100);
        if (m_dogPtr != NULL)
        {
            m_dogPtr->setHide(0);
            m_dogPtr->m_flags &= ~0x10000000u;
        }
        g_sequence.m_bits[1] |= 2;
        g_sequence.m_bits[1] |= 4;
        gamSetBwnFollow(0);
        return;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c58b8

int GamMan::empty1c58b8(int on)
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c58c0

int GamMan::pauseAchr(int on)
{
    if ((m_flags2 & 0x10000) == 0)
    {
        if (on == 0)
            return 0;
    }
    else if (on == 1)
    {
        return 0;
    }
    for (SeqNode<ACHRstat *> *node = m_achrList.m_used.begin(); node != m_achrList.m_used.end();
         node = node->m_next)
    {
        u_int oid = node->m_data->m_oid;
        if (gamOidType(oid) == 2)
        {
            getActor(oid);      /* the ROM discards it */
            node->m_data->pauseTasks(on, 1);
        }
    }
    int kinds[4] = { 0x13, 0x14, -1, 0 };
    g_taskMan.pauseKinds(kinds, on);
    g_taskMan.pauseTask(g_emyManagerTaskId, on);
    if (on == 1)
        m_flags2 |= 0x10000;
    else
        m_flags2 &= 0xfffcffff;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c5bf4

void GamMan::enterMenu(int kind)
{
    m_sndMan.stopBgm(0);
    m_flags2 &= ~8u;
    int kinds[2] = { 5, -1 };
    g_taskMan.pauseKinds(kinds, 1);
    m_flags2 |= 0x100000;
    if (kind == 1 || kind == 2)
        g_sequence.m_flags |= 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c5e1c

void GamMan::pauseForMenu(int menu)
{
    m_flags2 |= 4;
    pause(2);
    if (menu < 0x10005)
    {
        if (menu > 0x10001 || menu == 0x10000)
        {
            enterMenu(1);
            return;
        }
        if (menu > 0x10000)
        {
            enterMenu(2);
            return;
        }
        if (menu < 1 && menu > -3)
            return;
    }
    else if (menu < 0x1000a)
    {
        if (menu < 0x10008)
            return;
    }
    else if (menu == 0x1000a)
    {
        m_flags2 |= 0x20;
        enterMenu(1);
        return;
    }
    if ((u_int)(menu - 0x20000) > 0x3dffff)
        enterMenu(1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c5f70

void GamMan::resumeFromMenu(int arg)
{
    if ((m_flags2 & 0x20) != 0)
    {
        g_magSystem.m_scene.m_mes.setSpriteState(2, 0);
        g_magSystem.m_scene.m_mes.setSpriteDisp(2, 0);
    }
    m_flags2 |= 8;
}

// ──────────────────────────────────────────────────────────────────────
// The running bits come back (m_flags |= mask): 0x100004 the PC, the
// characters' tasks, the held item's actor and the BGM; 0x100104 the dog
// when it follows; 0x80000 Brown's controller.
// ROR 0x001c65e8 // callcheck: FUN_00281db0 (the actor's setHide, a virtual slot in the ROM)

void GamMan::resume(u_int mask)
{
    if ((mask & 0x100000) != 0)
        resumeFromMenu(0);
    if ((mask & 0x100004) != 0)
    {
        pauseAchr(0);
        empty1c58b8(0);
        u_int f2 = m_flags2;
        m_flags &= ~0x10000u;
        m_flags2 = f2 & ~0x8000000u;
        if ((f2 & 0x1000) == 0)
            m_flags2 = f2 & 0xf7fff7ff;
        if (m_pcPtr != NULL)
        {
            g_magSystem.setPC(m_pcActor, m_pcJnfc);
            m_pcPtr->clearSecta(1);
            if ((m_flags2 & 0x1000) == 0)
            {
                u_char pos[0x20];
                placeAchr(m_21fcc, pos, 1);
            }
        }
        if (m_pcEquipItemChrID != -1)
            gamShowActor(m_pcEquipItemChrID);
        getEmyOids();
        showLinked(m_21fcc);
        m_sndMan.resumeBgm(0, 0, 1);
    }
    if ((mask & 0x100104) != 0)
    {
        if (m_inuMode == 1 && (m_flags2 & 0x2000) == 0)
        {
            gamSetBwnFollow(1);
            if (m_dogPtr != NULL && m_dogChrNo != -1)
            {
                g_magSystem.setBwn(m_dogChrNo, 0);
                m_dogPtr->clearSecta(1);
            }
        }
        else
        {
            mask &= ~0x100u;
        }
    }
    if ((mask & 0x80000) != 0)
        gamSetBwnWait(1);
    m_flags |= mask;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c687c // callcheck: FUN_00281db0 (the actors' setHide, a virtual slot in the ROM)

void GamMan::showLinked(u_int oid)
{
    if (!isObj(oid) || gamOidList(oid) != 1)
        return;
    ACHRstat *chr = getAchr(oid);
    if (chr->m_link != -1)
        gamShowActor(getActorNo(chr->m_link));
    if (gamOidType(oid) == 0)
    {
        if (m_22004 != 0 && isObj(m_22004))
            gamShowActor(getActorNo(m_22004));
        if (m_22008 != 0 && isObj(m_22008))
            gamShowActor(getActorNo(m_22008));
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c6a04

void GamMan::getEmyOids()
{
    g_emyManager.getOids(&m_22004, &m_22008);
}

// ──────────────────────────────────────────────────────────────────────
// The running bits go (m_flags &= ~mask): 0x100000 the menu part,
// 0x100004 the PC and the characters' tasks, 0x100104 the dog (back to
// its motion 1 unless it is in a mode), 0x80000 Brown's controller,
// 0x200000 shows the dog.
// ROR 0x001c6a40

void GamMan::pause(u_int mask)
{
    if ((mask & 0x100000) != 0)
        pauseForMenu(-1);
    if ((mask & 0x100004) != 0)
    {
        m_flags |= 0x10000;
        m_flags2 = (m_flags2 & ~0x80000u) | 0x800;
        pauseAchr(1);
        empty1c58b8(1);
        if (m_pcPtr != NULL)
            setPc(-1);
    }
    if ((mask & 0x100104) != 0)
    {
        gamSetBwnFollow(0);
        if (m_dogPtr != NULL)
        {
            if (!g_magSystem.m_scene.m_actors[m_dogChrNo].isModeSet())
                chrChangeMotion(m_21fd0, 1, -1, 0x100, 0x1.11111p-6f /* 0x3c888888 */,
                                32769.0f /* 0x47000100 */);
            setDog(-1);
        }
    }
    if ((mask & 0x80000) != 0)
        gamSetBwnWait(0);
    if ((mask & 0x200000) != 0 && m_dogPtr != NULL)
    {
        m_dogPtr->setHide(0);
        m_dogPtr->m_flags &= ~0x10000000u;
    }
    m_flags &= ~mask;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c96c8

int GamMan::isFchrActive(u_int oid)
{
    u_int type = gamOidType(oid);
    if (type - 4 >= 2)
        return 0;
    FCHRstat *chr = getFchr(oid);
    if (chr->m_actor == -1)
        return 0;
    u_int flags = chr->m_flags;
    if (type == 4 && (flags & 1) == 0)
        return 1;
    if ((flags & 0x200000) != 0 || (flags & 1) != 0)
        return 0;
    if ((flags & 0x10) != 0)
        return 1;
    if ((flags & 8) == 0)
        return 0;
    return (flags & 2) != 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c9784

void GamMan::setFindTargetItem(short id)
{
    m_findTargetItem = id;
    if (id == 0xa00)
    {
        gamSetBwnItem(0);
        return;
    }
    gamSetBwnItem(id);
}

// ──────────────────────────────────────────────────────────────────────
// A jump table of 20 ops (data 0x2fbba0); ops 3, 0x12 and 0x13 do nothing.
// ROR 0x001c9920

int GamMan::ctlObjParam(u_int oid, u_int op, void *data, int alt)
{
    GamObjRec *rec = m_objRec;
    if (oid == (u_int)-1)
        return 0;
    u_int *word = (u_int *)data;
    for (int i = 0; i < m_objTable->m_num; i++, rec++)
    {
        if (rec->m_oid != oid)
            continue;
        if (op > 0x13)
            return -1;
        switch (op)
        {
        case 0x00: rec->m_flags |= *word;               return 0;
        case 0x01: rec->m_flags &= ~*word;              return 0;
        case 0x02: return (rec->m_flags & *word) != 0;
        case 0x04: rec->m_0c = (int)*word;              return 0;
        case 0x05: *word = (u_int)rec->m_0c;            return 0;
        case 0x06: *word = (u_int)rec->m_28;            return 0;
        case 0x07: *word = (u_int)rec->m_2c;            return 0;
        case 0x08: memcpy(data, rec->m_10, 0x10);       return 0;
        case 0x09: rec->m_08 = (int)*word;              return 0;
        case 0x0a: *word = (u_int)rec->m_08;            return 0;
        case 0x0b: memcpy(data, alt == 0 ? rec->m_30 : rec->m_40, 0x10); return -1;
        case 0x0c:
            word[0] = (u_int)rec->m_28;
            word[1] = (u_int)rec->m_2c;
            return -1;
        case 0x0d: *(float *)data = rec->m_20;          return -1;
        case 0x0e: memcpy(data, rec->m_10, 0x10);       return -1;
        case 0x0f: *word = (u_int)rec->m_08;            return -1;
        case 0x10:
        {
            FCHRstat *chr = findFchrType9((int)oid);
            if (chr == NULL)
                return -1;
            float rot[3];
            chr->getRot(rot);
            GamObjInfo info;
            info.m_alt = rec->m_28 != m_21f7c;
            const float *pos = info.m_alt ? rec->m_40 : rec->m_30;
            info.m_oid = chr->m_oid;
            info.m_rotY = rot[1];
            info.m_pos[0] = pos[0];
            info.m_pos[1] = pos[1];
            info.m_pos[2] = pos[2];
            info.m_flags = rec->m_flags & 0xf8;
            info._1c = 0;       /* stack garbage in the ROM */
            memcpy(data, &info, sizeof(info));
            return -1;
        }
        case 0x11: rec->m_flags = *word;                return 0;
        default:                                        return -1;
        }
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cb24c

void GamMan::setPc(int oid)
{
    if (oid == -1)
    {
        g_magSystem.setPC(-1, 0);
        return;
    }
    m_21fcc = oid;
    ACHRstat *pc = getAchr((u_int)oid);
    m_pcPtr = pc;
    if (pc == NULL)
        return;
    pc->clearSecta(1);
    m_pcPtr->m_flags |= 0x10000;
    m_pcActor = m_pcPtr->m_actor;
    m_pcActorPtr = &g_magSystem.m_scene.m_actors[m_pcActor];
    g_magSystem.setPC(-1, 0);
    m_21f74 = (u_int)-1;
    m_21f70 = m_pcActor;
    setCameraMode(m_cameraMode, (u_char)m_21f78);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cb384

void GamMan::setDog(int oid)
{
    if (oid == -1)
    {
        gamSetBwnFollow(0);
        return;
    }
    m_21fd0 = oid;
    ACHRstat *dog = getAchr((u_int)oid);
    m_dogPtr = dog;
    if (dog == NULL)
        return;
    dog->m_flags |= 0x10000;
    m_dogChrNo = m_dogPtr->m_actor;
    g_magSystem.setBwn(m_dogChrNo, 0);
}

// ──────────────────────────────────────────────────────────────────────
// Reads the global, not this (as the ROM does).
// ROR 0x001cbdb8

DogActor *GamMan::getActor(u_int oid)
{
    int no = g_gamMan.getActorNo(oid);
    if (no == -1)
        return NULL;
    return &g_magSystem.m_scene.m_actors[no];
}

// ──────────────────────────────────────────────────────────────────────
// The image's pixels lie before it, w * h bytes rounded up to 16.
// ROR 0x001cbdfc

void GamMan::setImage(int slot, u_char *image)
{
    GamImage *img = (GamImage *)image;
    u_int size = (u_int)img->m_w * img->m_h;
    if ((size & 0xf) != 0)
        size = (size & 0xfff0) + 0x10;
    // PC: a host address does not fit the word; the VM's word store keeps
    // it whole for the script (and the readers that go through seqVmLoadWord).
    seqVmStoreWord((intptr_t)&img->m_pixels, (intptr_t)(image - size));
    m_image[slot] = img;
}

// ──────────────────────────────────────────────────────────────────────
// The inline GamMan::find in the ROM, with -1 for no object.
// ROR 0x001cbe38 // callcheck: getAchr, getFchr, getVFchr (inside the inline find)

int GamMan::getActorNo(u_int oid)
{
    if (!isObj(oid))
        return -1;
    ObjStat *obj = find(oid);
    if (obj == NULL)
        return -1;
    return obj->m_actor;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cd224

void GamMan::releaseStage2()
{
    releaseTenkyus(2);
    releaseObjs(2, 0);
}

// ──────────────────────────────────────────────────────────────────────
// A stage change asked by the scripts: flags2 bit 2 runs it now (the PC
// and dog off their cells, camera mode 1, the menu pause, then kind 3 the
// stage-2 images and areas or kind 5 a full reset, and execStageReq); bit
// 1 waits on the background loading task.
// ROR 0x001cd25c

void GamMan::afterSeqExec()
{
    if ((m_flags & 0x8000) != 0)
        return;
    u_int f2 = m_flags2;
    if ((f2 & 3) == 0)
        return;
    u_int keep;
    if ((f2 & 2) == 0)
    {
        if ((f2 & 1) == 0)
            return;
        if (g_taskMan.getTask(m_bgLoadTaskId) == NULL)
            return;
        keep = ~1u;
        f2 = m_flags2;
    }
    else
    {
        if (m_pcPtr != NULL)
            m_pcPtr->clearSecta(1);
        if (m_dogPtr != NULL)
            m_dogPtr->clearSecta(1);
        m_21f74 = (u_int)-1;
        setCameraMode(1, 0);
        pause(0x100000);
        if (m_stageReq == 3)
        {
            g_sequence.stopImages(2);
            m_area.stopKind(2);
            m_area.clearKind(2);
            clearPool20b00(2);
        }
        else if (m_stageReq == 5)
        {
            resetStage();
            releaseTenkyus(0);
            g_sequence.clearStageVars();
        }
        execStageReq();
        f2 = m_flags2;
        keep = ~2u;
    }
    m_flags2 = f2 & keep;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cd7c8

void GamMan::execStageReq()
{
    int stat = m_cur_stat;
    if (stat == 2)
    {
        releaseStage2();
        initStage2();
        m_225e4 = 0;
        startStage2();
        gamStage2Loaded();
    }
    else if (stat == 3)
    {
        empty1c4f30();
    }
    else if (stat == 1)
    {
        m_flags &= ~0x20u;
        g_sequence.m_global[5] = 0;
        g_sequence.m_global[7] = 0;
        loadStage(m_chapter);
    }
    g_sequence.m_global[3] = m_21f84;
    g_sequence.m_global[4] = m_21f88;
    g_sequence.m_global[1] = m_21f7c;
    g_sequence.m_global[2] = m_stageReq != 2 ? m_21f80 : -1;
    resume(2);
    g_taskMan.pauseOthers(0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d046c

int GamMan::isObj(u_int oid)
{
    switch (gamOidList(oid))
    {
    case 1:
        return isAchr(oid);
    case 0:
        return isVFchr(oid);
    case 2:
        if (gamOidType(oid) != 5)
            return isFchr(oid);
        if (isVFchr(oid))
            return 1;
        return isFchr(oid) != 0;
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// fchr / vfchr objects: the bits off their flags (both lists for a type 5),
// then 0x40 re-finds the cell and re-links twice, 0x10000000 follows the
// actor, 1 shows it (or keeps it hidden while held), 0x10 the 0x1cbb80 part.
// achr objects: 0x40 re-links and kills its task, 1 shows it, the held
// item's actor for the PC and its linked object's actor.
// ROR 0x001d1410 // callcheck: FUN_00281db0 (the actors' setHide, a virtual slot in the ROM)

int GamMan::resetObj(u_int oid, u_int bits)
{
    if (!isObj(oid))
        return 0;
    u_int list = gamOidList(oid);
    if (list != 1)
    {
        if (list != 0 && list != 2)
            return 0;
        FCHRstat *vchr = getVFchr(oid);
        if (vchr != NULL)
            vchr->m_flags &= ~bits;
        FCHRstat *chr = getFchr(oid);
        if (chr != NULL)
        {
            chr->m_flags &= ~bits;
            if ((bits & 0x40) != 0)
            {
                chr->findSecta();
                chr->m_flags |= 0x10000080;
                relinkObj(oid);
                relinkObj(oid);
            }
            if ((bits & 0x10000000) != 0)
                chr->syncActorPos();
            if ((bits & 1) != 0)
            {
                if ((chr->m_flags & 0x200000) == 0)
                {
                    chr->setHide(0);
                    if (isFchrActive(oid) && gamOidType(oid) == 5)
                        ctlObj1cb8e4(oid, 0x32, 0);
                }
                else
                {
                    chr->m_flags |= 1;
                }
            }
            if ((bits & 0x10) != 0)
                ctlObj1cbb80(oid);
        }
        return 1;
    }

    ACHRstat *chr = getAchr(oid);
    if ((bits & 0x40) != 0)
    {
        relinkObj(oid);
        chr->m_flags |= 0x10000080;
        g_taskMan.killTask(chr->m_taskId);
    }
    if ((bits & 0x10000000) != 0)
        chr->syncActorPos();
    if ((bits & 1) != 0)
    {
        chr->setHide(0);
        if (oid == (u_int)m_21fcc && m_pcEquipItemChrID != -1 && (m_flags2 & 0x4000000) == 0)
            gamShowActor(m_pcEquipItemChrID);
        int link = chr->m_link;
        if (link != -1 && isObj((u_int)link))
            gamShowActor(getActorNo((u_int)link));
    }
    if ((bits & 0x10) != 0)
        ctlObj1cbb80(oid);
    chr->m_flags &= ~bits;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d2aac

int GamMan::isAchr(u_int oid)
{
    u_int slot = gamOidSlot(oid);
    if (slot >= 0x80)
        return 0;
    return m_achr[slot] != NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d2adc

int GamMan::isFchr(u_int oid)
{
    u_int slot = gamOidSlot(oid);
    if (slot - 1 >= 0x27f)
        return 0;
    return m_fchr[slot] != NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d2b10

int GamMan::isVFchr(u_int oid)
{
    for (SeqNode<FCHRstat *> *node = m_vfchrList.m_used.begin(); node != m_vfchrList.m_used.end();
         node = node->m_next)
    {
        if (node->m_data->m_oid == oid)
            return 1;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d2b8c

ACHRstat *GamMan::getAchr(u_int oid)
{
    return m_achr[gamOidSlot(oid)];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d2ba8

FCHRstat *GamMan::getFchr(u_int oid)
{
    if (gamOidType(oid) == 5)
    {
        FCHRstat *chr = getVFchr(oid);
        if (chr != NULL)
            return chr;
    }
    return m_fchr[gamOidSlot(oid)];
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d2c0c

FCHRstat *GamMan::findFchrType9(int no)
{
    for (SeqNode<FCHRstat *> *node = m_fchrList.m_used.begin(); node != m_fchrList.m_used.end();
         node = node->m_next)
    {
        u_int oid = node->m_data->m_oid;
        if (gamOidType(oid) != 9)
            continue;
        int param[4];
        getObjParam(oid, 0x41, param);
        if (param[0] == no)
            return node->m_data;
    }
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d2d00

FCHRstat *GamMan::getVFchr(u_int oid)
{
    for (SeqNode<FCHRstat *> *node = m_vfchrList.m_used.begin(); node != m_vfchrList.m_used.end();
         node = node->m_next)
    {
        if (node->m_data->m_oid == oid)
            return node->m_data;
    }
    return NULL;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x001c2f50
void GamMan::loadStageSnd(int chapter)
{
    ROSEPAN_STUB("GamMan::loadStageSnd", 0x001c2f50);
}

// stub of ROR 0x001ccc30
void GamMan::releaseChr(int arg)
{
    ROSEPAN_STUB("GamMan::releaseChr", 0x001ccc30);
}

// stub of ROR 0x001c7e40
void GamMan::addItem(short id, int arg)
{
    ROSEPAN_STUB("GamMan::addItem", 0x001c7e40);
}

// stub of ROR 0x001c8f7c
int GamMan::addStock(short id, int count)
{
    ROSEPAN_STUB("GamMan::addStock", 0x001c8f7c);
    return {};
}

// stub of ROR 0x001c8ca4
int GamMan::canAddItem(int arg, short id)
{
    ROSEPAN_STUB("GamMan::canAddItem", 0x001c8ca4);
    return {};
}

// stub of ROR 0x001c82f8
void GamMan::delItem(short id, int arg)
{
    ROSEPAN_STUB("GamMan::delItem", 0x001c82f8);
}

// stub of ROR 0x001c8414
void GamMan::delItemAll(short id)
{
    ROSEPAN_STUB("GamMan::delItemAll", 0x001c8414);
}

// stub of ROR 0x001c94d8
int GamMan::delStock(short id, int one)
{
    ROSEPAN_STUB("GamMan::delStock", 0x001c94d8);
    return {};
}

// stub of ROR 0x001c7a4c
void GamMan::equipItem(short id)
{
    ROSEPAN_STUB("GamMan::equipItem", 0x001c7a4c);
}

// stub of ROR 0x001c8ae8
int GamMan::getItemCount(short id, int withStock)
{
    ROSEPAN_STUB("GamMan::getItemCount", 0x001c8ae8);
    return {};
}

// stub of ROR 0x001c6ccc
void GamMan::getStatus(int which, void *out)
{
    ROSEPAN_STUB("GamMan::getStatus", 0x001c6ccc);
}

// stub of ROR 0x001c8a28
int GamMan::hasItem(short id, int withStock)
{
    ROSEPAN_STUB("GamMan::hasItem", 0x001c8a28);
    return {};
}

// stub of ROR 0x001c8598
void GamMan::selectItem(short id)
{
    ROSEPAN_STUB("GamMan::selectItem", 0x001c8598);
}

// stub of ROR 0x001c3d38
void GamMan::setPcStartPos()
{
    ROSEPAN_STUB("GamMan::setPcStartPos", 0x001c3d38);
}

// stub of ROR 0x001cd404
void GamMan::storePcTransform()
{
    ROSEPAN_STUB("GamMan::storePcTransform", 0x001cd404);
}

// stub of ROR 0x001c43f0
void GamMan::onSubSeqEnd()
{
    ROSEPAN_STUB("GamMan::onSubSeqEnd", 0x001c43f0);
}

// stub of ROR 0x001cbd0c
void GamMan::clearPool20b00(int kind)
{
    ROSEPAN_STUB("GamMan::clearPool20b00", 0x001cbd0c);
}

// stub of ROR 0x001cb8e4
void GamMan::ctlObj1cb8e4(u_int oid, int a, int b)
{
    ROSEPAN_STUB("GamMan::ctlObj1cb8e4", 0x001cb8e4);
}

// stub of ROR 0x001cbb80
void GamMan::ctlObj1cbb80(u_int oid)
{
    ROSEPAN_STUB("GamMan::ctlObj1cbb80", 0x001cbb80);
}

// stub of ROR 0x001d1c80
void GamMan::getObjParam(u_int oid, int which, void *out)
{
    ROSEPAN_STUB("GamMan::getObjParam", 0x001d1c80);
}

// stub of ROR 0x001c47c4
void GamMan::initStage2()
{
    ROSEPAN_STUB("GamMan::initStage2", 0x001c47c4);
}

// stub of ROR 0x001cdf5c
void GamMan::releaseObjs(int kind, int arg)
{
    ROSEPAN_STUB("GamMan::releaseObjs", 0x001cdf5c);
}

// stub of ROR 0x001cb01c
void GamMan::releaseTenkyus(int kind)
{
    ROSEPAN_STUB("GamMan::releaseTenkyus", 0x001cb01c);
}

// stub of ROR 0x001cdbf0
void GamMan::resetStage()
{
    ROSEPAN_STUB("GamMan::resetStage", 0x001cdbf0);
}

// stub of ROR 0x001cd60c
void GamMan::startStage2()
{
    ROSEPAN_STUB("GamMan::startStage2", 0x001cd60c);
}
