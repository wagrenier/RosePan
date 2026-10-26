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

#include <new>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/rosepan_stub.h"
#include "game/gamman.h"

#include "bed/bedmemory.h"
#include "bed/bedpfh.h"
#include "game/gamfile.h"
#include "lib/unassigned.h"
#include "mag/magsystem.h"
#include "rx/rxsnd.h"
#include "sdk/libvu0.h"
#include "sequence/sequence.h"
#include "snd/sndman.h"
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
    m_18bc4 = (GamObjSet *)g_utlMemory.allocDebug(0xc00, s_file, 347, 0, 1);
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
    m_msgParam[1] = 10;
    m_msgParam[2] = 0xffff;
    m_msgParam[4] = 0x100;
    m_pcEquipItemOID = 0;
    m_equipItem = 0xa00;
    m_selItemNum = 0;
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
    m_msgParam[0] = 10;
    m_msgParam[3] = 0x100;
    m_msgParam[6] = 0;
    m_msgParam[5] = 0;
    for (int i = 0; i < 4; i++)
        m_camStack[i].m_mode = -1;
    for (int i = 0; i < 3; i++)
        m_fogStack[i].m_color[0] = -1.0f;
    m_camDepth = 0;
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
    memset(m_vfchrSlot, 0, 0x180);     /* 0x180 bytes of the 0x600, as in the ROM */
    m_225e4 = 0;
    m_keepBuf = NULL;
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
        m_sysData.set((u_char *)pfh.getFile(0));
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
// Shows (process on, standard colour, state / disp on, anim - 1) or
// hides the scene's message sprite no.
// ROR 0x001c5094

void GamMan::showMesSprite(int no, int show, int anim)
{
    S2dMes &mes = g_magSystem.m_scene.m_mes;
    if (show != 0) {
        mes.setSpriteProcessOn(no);
        mes.setSpriteColorStd(no);
        mes.setSpriteState(no, 1);
        mes.setSpriteDisp(no, 1);
        mes.setSpriteAnim(no, anim > 0 ? anim - 1 : 0);
        return;
    }
    mes.setSpriteState(no, 0);
    mes.setSpriteDisp(no, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c5168

void GamMan::setMesSpritePos(int no, int x, int y)
{
    S2dMes &mes = g_magSystem.m_scene.m_mes;
    mes.setSpriteStdPoint(no, 0, 0);
    mes.setSpriteOffPoint(no, x << 4, y << 4);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c51e0

void GamMan::fadeInMesSprite(int no, int time)
{
    S2dMes &mes = g_magSystem.m_scene.m_mes;
    mes.setSpriteProcessOn(no);
    mes.setSpriteFadeIn(no, time);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c5240

void GamMan::fadeOutMesSprite(int no, int time)
{
    g_magSystem.m_scene.m_mes.setSpriteFadeOut(no, time);
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
// The script's SE pack swaps (native 252 cases 48 / 100) and the chapter
// pack reloads (cases 101 / 102, initStage2).
// ROR 0x001c7500

void GamMan::loadScriptSePack(const char *name)
{
    g_sndMan.stopAll(SND_TYPE_SE);
    g_sndMan.freeBank(0xb);
    g_sndMan.freeBank(0x20);
    g_sndMan.freeBank(0x40);
    m_sndMan.loadSePack(name, 0);
    loadItemSe(0xa00, 1);
    m_flags2 |= 0x1000000;
}

// ROR 0x001c75bc

void GamMan::loadScriptSePack2(const char *name)
{
    g_sndMan.stopAll(SND_TYPE_SE);
    g_sndMan.freeBank(0x20);
    g_sndMan.freeBank(0x40);
    m_sndMan.loadSePack(name, 0);
    loadItemSe(0xa00, 1);
    m_flags2 |= 0x2000000;
}

// ROR 0x001c766c

void GamMan::reloadChapterSe1f()
{
    g_sndMan.stopAll(SND_TYPE_SE);
    g_sndMan.freeBank(0x1f);
    loadChapterSe(m_chapter, 1);
}

// ROR 0x001c76cc

void GamMan::reloadChapterSe0c()
{
    g_sndMan.stopAll(SND_TYPE_SE);
    g_sndMan.freeBank(0xc);
    loadChapterSe(m_chapter, 0);
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
// 1 while the message task runs or the system message window is open.
// ROR 0x001cf9d4

int GamMan::isMsgBusy()
{
    if (g_taskMan.getTask(m_msgTaskId) == NULL)
    {
        if (m_sysMsgHandle == -1)
            return 0;
        if (!g_magSystem.m_scene.isWindowOpen(m_sysMsgHandle))
            return 0;
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cfb38

void GamMan::setSysMsgHandle(int handle)
{
    m_sysMsgHandle = handle;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cf7d0

void GamMan::setMsgParam(const int *param)
{
    memcpy(m_msgParam, param, sizeof(m_msgParam));
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cf95c

void GamMan::killMsgTask()
{
    if (g_taskMan.getTask(m_msgTaskId) != NULL)
    {
        g_taskMan.killTask(m_msgTaskId);
        m_msgTaskId = -1;
    }
    m_msgMode = 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c4f70

void GamMan::releasePcMotion(int id)
{
    g_magSystem.releaseActorMotion(m_pcActor, id);
}

// ──────────────────────────────────────────────────────────────────────
// Marks light set theD in use in m_flags (0 0x2000000, 1 0x4000000, 6
// 0x8000000, 7 0x10000000; others none) and copies set theS over it.
// ROR 0x001d2724

void GamMan::setLightFlag(int theD, int theS)
{
    if (theD == 1)
        m_flags |= 0x4000000;
    else if (theD == 0)
        m_flags |= 0x2000000;
    else if (theD == 6)
        m_flags |= 0x8000000;
    else if (theD == 7)
        m_flags |= 0x10000000;
    g_magSystem.m_scene.copyLightSet(theD, theS);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c6470

void GamMan::pushFog()
{
    for (int i = 0; i < 3; i++)
    {
        if (m_fogStack[i].m_color[0] == -1.0f)
        {
            m_fogStack[i] = g_magSystem.m_scene.m_fog[0];
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c651c

void GamMan::popFog()
{
    for (int i = 2; i >= 0; i--)
    {
        if (m_fogStack[i].m_color[0] != -1.0f)
        {
            g_magSystem.m_scene.setFog(&m_fogStack[i], 0);
            m_fogStack[i].m_color[0] = -1.0f;
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// m_flags 0x8000000: set 0 is saved in 7; 0x10000000: set 1 is saved in 6.
// Set 1 is saved only while m_flags has 0x4000000.
// ROR 0x001d2494

void GamMan::saveLightSets(int mode)
{
    MagScene &scene = g_magSystem.m_scene;
    if (mode == 1)
    {
        if ((m_flags & 0x4000000) == 0)
            return;
        scene.copyLightSet(6, 1);
        m_flags |= 0x10000000;
        return;
    }
    if (mode == 0)
    {
        scene.copyLightSet(7, 0);
        if ((m_flags & 0x4000000) != 0)
        {
            scene.copyLightSet(6, 1);
            m_flags |= 0x10000000;
        }
    }
    else if (mode == 2)
        scene.copyLightSet(7, 0);
    else
        return;
    m_flags |= 0x8000000;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d25e4

void GamMan::restoreLightSets(int mode)
{
    MagScene &scene = g_magSystem.m_scene;
    if ((mode == 0 || mode == 2) && (m_flags & 0x8000000) != 0)
        scene.copyLightSet(0, 7);
    if ((mode == 0 || mode == 1) && (m_flags & 0x10000000) != 0)
        scene.copyLightSet(1, 6);
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
            info.m_alt = rec->m_28 != m_wrdNo;
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
// A stage change for afterSeqExec to run (flags2 bit 2). Kind 1 is a new
// chapter (globals 0x2f / 0x30 get the new and the old one; nothing when it
// is the current one), kind 2 a room of the stage (the room already loaded
// only resumes the menu pause), kind 3 the stage-2 data with a3; the tasks
// other than the current one pause until execStageReq resumes them.
// ROR 0x001cd044

void GamMan::requestStage(int kind, int room, int a3, int chapter, u_int word)
{
    m_1a684 = word;
    m_flags2 |= 0x80000000u;
    if (kind == 1)
    {
        if (chapter == m_chapter)
            return;
        g_sequence.m_global[0x2f] = chapter;
        g_sequence.m_global[0x30] = m_chapter;
        m_stageReq = 5;
        m_chapter = chapter;
    }
    else
    {
        if (kind == m_cur_stat && kind == 2 && m_wrdNo == room)
        {
            resume(0x100000);
            return;
        }
        m_stageReq = 3;
    }
    m_prev_stat = m_cur_stat;
    m_flags2 |= 2;
    m_21f84 = m_wrdNo;
    m_21f88 = m_21f80;
    m_cur_stat = kind;
    if (kind == 2)
    {
        m_wrdNo = room;
    }
    else if (kind == 3)
    {
        m_21f80 = a3;
    }
    else
    {
        m_21f80 = -1;
        m_wrdNo = -1;
    }
    g_taskMan.pauseOthers(1);
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
    g_sequence.m_global[1] = m_wrdNo;
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

// ──────────────────────────────────────────────────────────────────────
// Written ahead for natives 152 and 155 (sequence/seqcall.cpp).
// ROR 0x001d2d7c

int GamMan::hasVFchrParam(int which, int value)
{
    for (SeqNode<FCHRstat *> *node = m_vfchrList.m_used.begin(); node != m_vfchrList.m_used.end();
         node = node->m_next)
    {
        int param[4];
        node->m_data->getParam(which, param);
        if (value == param[0])
            return 1;
    }
    return 0;
}

// ROR 0x001c7da8

int GamMan::isInOidList(int id)
{
    const int *list = (const int *)m_1f240;
    for (int i = 0; i <= 0x168; i++)
    {
        if (list[i] == id)
            return 1;
    }
    return 0;
}

// ROR 0x001c98c0

int GamMan::isObjRecRef(int id)
{
    const GamObjRec *rec = m_objRec;
    for (int i = 0; i < m_objTable->m_num; i++, rec++)
    {
        if (rec->m_28 == id || rec->m_2c == id)
            return 1;
    }
    return 0;
}

// ROR 0x001d1720

int GamMan::testObjFlags(u_int oid, u_int bits)
{
    if (!isObj(oid))
        return 0;
    ObjStat *obj;
    switch (gamOidList(oid))
    {
    case 1:  obj = getAchr(oid); break;
    case 0:  obj = getVFchr(oid); break;
    case 2:  obj = getFchr(oid); break;
    default: return 0;
    }
    return (obj->m_flags & bits) != 0;
}

// ════════════════════════════════════════════════════════════════════════
// Slice 3: the stage sounds, the items and the stock, object ids and the
// chr teardown.
// ════════════════════════════════════════════════════════════════════════

// One SE pack file into buf (loadStageSnd's and loadItemSe's request: the
// file whole, closed after the read).
static void gamReadSe(FileReader &reader, const char *path, void *buf, int size)
{
    FileReadReq req;
    req.m_name = path;
    req.m_08 = 1;
    req.m_offset = 0;
    req.m_whence = 0;
    req.m_buf = buf;
    req.m_size = size;
    req.m_close = 1;
    reader.read(&req, 0);
}

// ──────────────────────────────────────────────────────────────────────
// The stage's SE packs by chapter: the sound stops, slots 10 / 11 / 0x20 /
// 0x1f / 0xc are freed, then one 1xxx.se pair (the walk / area sounds) and
// one 3xxx.se (the chapter's own) are read through one 1 MB buffer and
// registered under their names. Chapters 9 and > 13 get no 3xxx pack;
// chapter 9 no 1xxx pair either.
// ROR 0x001c2f50 // callcheck: FileReader, FileReadReq, read, ~FileReader (the reader local and the request in gamReadSe, inline in the ROM)

void GamMan::loadStageSnd(int chapter)
{
    GamSndMan *snd = &m_sndMan;
    void *buf = g_utlMemory.allocDebug(0x101480, s_file, 1579, 0, 1);
    FileReader reader;
    snd->funcSnd(0x11, 0, 0, 0, -1);
    snd->freeSePack(10);
    snd->freeSePack(0xb);
    snd->freeSePack(0x20);
    snd->freeSePack(0x1f);
    snd->freeSePack(0xc);

    switch (chapter)
    {
    case 1:
    case 7:
    case 13:
        gamReadSe(reader, "data/snd/xv/1001.se", buf, 0x80000);
        snd->loadSePack("1001.se", buf, 0);
        gamReadSe(reader, "data/snd/xv/1102.se", buf, 0x80000);
        snd->loadSePack("1102.se", buf, 0);
        break;
    case 2:
    case 3:
    case 4:
    case 10:
    case 11:
    case 12:
        gamReadSe(reader, "data/snd/xv/1000.se", buf, 0x80000);
        snd->loadSePack("1000.se", buf, 0);
        gamReadSe(reader, "data/snd/xv/1100.se", buf, 0x80000);
        snd->loadSePack("1100.se", buf, 0);
        break;
    case 5:
        gamReadSe(reader, "data/snd/xv/1001.se", buf, 0x80000);
        snd->loadSePack("1001.se", buf, 0);
        gamReadSe(reader, "data/snd/xv/1101.se", buf, 0x80000);
        snd->loadSePack("1101.se", buf, 0);
        break;
    case 6:
        gamReadSe(reader, "data/snd/xv/1001.se", buf, 0x80000);
        snd->loadSePack("1001.se", buf, 0);
        gamReadSe(reader, "data/snd/xv/1103.se", buf, 0x80000);
        snd->loadSePack("1103.se", buf, 0);
        break;
    case 8:
        gamReadSe(reader, "data/snd/xv/1002.se", buf, 0x80000);
        snd->loadSePack("1002.se", buf, 0);
        gamReadSe(reader, "data/snd/xv/1104.se", buf, 0x80000);
        snd->loadSePack("1104.se", buf, 0);
        break;
    case 9:
        break;
    default:
        goto done;
    }

    switch (chapter)
    {
    case 1:  gamReadSe(reader, "data/snd/xv/3200.se", buf, 0x80000); snd->loadSePack("3200.se", buf, 0); break;
    case 2:  gamReadSe(reader, "data/snd/xv/3201.se", buf, 0x80000); snd->loadSePack("3201.se", buf, 0); break;
    case 3:  gamReadSe(reader, "data/snd/xv/3202.se", buf, 0x80000); snd->loadSePack("3202.se", buf, 0); break;
    case 4:  gamReadSe(reader, "data/snd/xv/3203.se", buf, 0x80000); snd->loadSePack("3203.se", buf, 0); break;
    case 5:
    case 13: gamReadSe(reader, "data/snd/xv/3204.se", buf, 0x80000); snd->loadSePack("3204.se", buf, 0); break;
    case 6:  gamReadSe(reader, "data/snd/xv/3205.se", buf, 0x80000); snd->loadSePack("3205.se", buf, 0); break;
    case 7:  gamReadSe(reader, "data/snd/xv/3206.se", buf, 0x80000); snd->loadSePack("3206.se", buf, 0); break;
    case 8:  gamReadSe(reader, "data/snd/xv/3207.se", buf, 0x80000); snd->loadSePack("3207.se", buf, 0); break;
    case 10: gamReadSe(reader, "data/snd/xv/3208.se", buf, 0x80000); snd->loadSePack("3208.se", buf, 0); break;
    case 11: gamReadSe(reader, "data/snd/xv/3209.se", buf, 0x80000); snd->loadSePack("3209.se", buf, 0); break;
    case 12: gamReadSe(reader, "data/snd/xv/3210.se", buf, 0x80000); snd->loadSePack("3210.se", buf, 0); break;
    default: break;
    }
done:
    utlMemFree(buf);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c6c98

int GamMan::getPlayMinutes()
{
    return m_playMinutes + m_vcount / 1800;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c6ccc

void GamMan::getStatus(int which, void *out)
{
    u_int *word = (u_int *)out;
    switch (which) {
    case 1:    *word = m_flags; break;
    case 2:    *word = (u_int)m_vcount; break;
    case 3:    *word = (u_int)getPlayMinutes(); break;
    case 4:    *(short *)out = (short)m_chapter; break;
    case 5:    *word = (u_int)m_wrdNo; break;
    case 6:    *word = (u_int)m_21f80; break;
    case 7:    *word = (u_int)m_21f68; break;
    case 8:    *word = (u_int)m_cameraMode; break;
    case 9:    *word = (u_int)m_pcActor; break;
    case 10:   *word = (u_int)m_dogChrNo; break;
    case 0xb:  *word = (u_int)m_murderChrNo; break;
    // PC: the three ObjStat pointers whole (a word on the EE).
    case 0xc:  *(ObjStat **)out = m_pcPtr; break;
    case 0xd:  *(ObjStat **)out = m_dogPtr; break;
    case 0xe:  *(ObjStat **)out = m_murderPtr; break;
    case 0xf:  *(short *)out = (short)m_volume; break;
    case 0x10: *word = (u_int)m_optBrightLevel; break;
    case 0x11: *word = (u_int)m_optOutput; break;
    case 0x12: *word = (u_int)m_optVibration; break;
    case 0x13: *word = (u_int)m_21f94; break;
    case 0x14: *word = (u_int)m_optMasterLevel; break;
    // PC: the address whole (a word on the EE).
    case 0x15: *(int **)out = &m_optOutput; break;
    case 0x16: memcpy(out, &m_21fbc[0], 4); break;
    case 0x17: memcpy(out, &m_21fbc[1], 4); break;
    case 0x19: *word = (u_int)(u_short)m_findTargetItem; break;
    case 0x1a: *word = (u_int)(u_short)m_equipItem; break;
    case 0x1c: *word = (u_int)m_04; break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// A new object id. Types 0 / 1 / 3 and 2 are achrs (list 1) with their own
// counters (+0x225f8, +0x225f4), the other types below 10 take list and the
// counter +0x225fc, all counted down past the ids isObj finds taken; types
// 10..15 share +0x22600 (13 skips the ids GamArea holds, the others take
// the next number as it is). Types >= 16 get 0.
// ROR 0x001c6fe0

u_int GamMan::makeOid(u_int list, u_int type, int image)
{
    if (image == -1)
        image = m_cur_stat;
    u_int oid = 0;
    if (type < 0x10)
    {
        u_int stage = (u_int)m_wrdNo;
        u_int n;
        switch (type)
        {
        case 0:
        case 1:
        case 3:
            oid = (u_int)image << 30 | (stage & 0x7f) << 16 | type << 23 | 0x10000000 | m_225f4[1];
            while (isObj(oid))
                oid--;
            m_225f4[1] = (oid & 0x2ff) - 1;
            break;
        case 2:
            oid = (u_int)image << 30 | (stage & 0x7f) << 16 | type << 23 | 0x10000000 | m_225f4[0];
            while (isObj(oid))
                oid--;
            m_225f4[0] = (oid & 0x2ff) - 1;
            break;
        default:
            oid = (u_int)image << 30 | (stage & 0x7f) << 16 | type << 23 | (list & 3) << 28 | m_225f4[2];
            while (isObj(oid))
                oid--;
            m_225f4[2] = (oid & 0x2ff) - 1;
            break;
        case 10:
        case 11:
        case 12:
        case 14:
        case 15:
            n = m_225f4[3];
            m_225f4[3] = n - 1;
            oid = (u_int)image << 30 | type << 23 | (stage & 0x7f) << 16 | n;
            break;
        case 13:
            oid = (u_int)image << 30 | type << 23 | (stage & 0x7f) << 16 | m_225f4[3];
            while (m_area.hasId(oid))
                oid--;
            m_225f4[3] = (oid & 0x2ff) - 1;
            break;
        }
    }
    return oid;
}

// ──────────────────────────────────────────────────────────────────────
// The SE pack of a held item (the sounds its use makes): 6400.se for any
// item without its own.
// ROR 0x001c772c // callcheck: FileReader, FileReadReq, read, ~FileReader (the reader local and the request in gamReadSe, inline in the ROM)

void GamMan::loadItemSe(u_short id, int cur)
{
    void *buf = g_utlMemory.allocDebug(0x18040, s_file, 4573, 0, 1);
    if (cur != 0)
        id = (u_short)m_equipItem;

    int no;
    switch (id)
    {
    case 0xa36: case 0xa37: case 0xa40: case 0xa41:
    case 0xafd: case 0xafe: case 0xaff: case 0xb00: case 0xb01:
    case 0xb33:
        no = 1; break;
    case 0xa38: case 0xb35:
        no = 2; break;
    case 0xa3b: case 0xb27: case 0xb31: case 0xb34:
        no = 3; break;
    case 0xa3c:
        no = 4; break;
    case 0xa39: case 0xa44: case 0xb32:
        no = 5; break;
    case 0xa3a:
        no = 6; break;
    case 0xa3e: case 0xa43:
        no = 7; break;
    case 0xa3d: case 0xb25: case 0xb36:
        no = 8; break;
    default:
        no = 0; break;
    }
    char path[0x20];
    char name[0x10];
    snprintf(path, sizeof(path), "data/snd/xv/640%d.se", no);
    snprintf(name, sizeof(name), "640%d.se", no);

    FileReader reader;
    gamReadSe(reader, path, buf, 0x18000);
    m_sndMan.loadSePack(name, buf, 0);
    utlMemFree(buf);
}

// ──────────────────────────────────────────────────────────────────────
// The PC takes item id in hand: flags2 0x200000 when it changes, the PC's
// param 0x32, the old held object released, a new one created and parented
// to the PC's node p_hn_r (the lamp 0xa3e / 0xa43 keeps its count in
// m_selItemNum), and its SE pack once flags2 0x40 or 0x200000 is set.
// ROR 0x001c7a4c // callcheck: FUN_00215a30, i3rGetNodeIndexByName, memset (the assert handler and the call inside the inline DogModel::getNodeIndexByName; memset clears the zero vector, an initializer here)

void GamMan::equipItem(short id)
{
    short item = id;
    if (id == 0xa3e || id == 0xa43)
        selectItem(id);
    if (m_equipItem != item)
        m_flags2 |= 0x200000;
    m_equipItem = item;
    setPcParam(0x32, &item);

    float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (m_pcEquipItemChrID != -1)
        releaseChr(m_pcEquipItemOID);
    if (item == 0xa00)
    {
        if (m_pcEquipItemOID != 0)
        {
            releaseChr(m_pcEquipItemOID);
            m_pcEquipItemOID = 0;
            m_pcEquipItemChrID = -1;
        }
    }
    else
    {
        u_int oid = chrCreateItemObj(item, zero, zero);
        m_pcEquipItemOID = (int)oid;
        m_pcEquipItemChrID = g_gamMan.getActorNo(oid);
        MagScene &scene = g_magSystem.m_scene;
        int node = scene.m_actors[m_pcActor].getNodeIndexByName("p_hn_r");
        scene.changeActorMotion(0x1.11112ep-6f, m_pcEquipItemChrID, 2, 0, 0); /* 0x3c888897 */
        g_magSystem.setActorParent(m_pcEquipItemChrID, m_pcActor, node);
        if (m_equipItem == 0xa3e || m_equipItem == 0xa43)
            m_selItemNum = getItemNum(m_equipItem);
    }
    if ((m_flags2 & 0x200040) != 0)    /* the ROM tests the doubleword +0x21f48 & 0x20004000000000 */
        loadItemSe((u_short)item, 0);
}

// ──────────────────────────────────────────────────────────────────────
// Item id into the slots: its flag bit (table column 3 of the row whose
// column 2 is id) and system data flag 4, then the first slot holding it
// or the first free one. A stacking item (system flag 0x20000) counts up
// to 99 -- num != 0 sets the count of the slot outright; others take a new
// slot with a new object id. 1 when no slot is free.
// ROR 0x001c7e40

int GamMan::addItem(short id, int num)
{
    int bit = lookupFlagTable(2, id, 3);
    if (bit != -1)
        g_sequence.m_bits[(u_int)bit >> 5] |= 1u << (bit & 0x1f);
    m_sysData.setFlags(id, 4);

    GamItemSlot *slot = m_itemHave.m_begin;
    int i = 0;
    int found = -1;
    for (; i < 0xc; i++)
    {
        if (slot[i].m_id == id)
        {
            found = i;
            break;
        }
        if (slot[i].m_id == 0xa00)
            break;
    }
    if (i == 0xc && !hasFreeItemSlot())
        return 1;

    const void *rec = m_sysData.getRec(id);
    if (m_sysData.testFlags(id, 0x20000))
    {
        if (num != 0)
        {
            u_int oid = makeOid(2, 5, -1);
            if (num > 99)
                num = 99;
            slot[i].m_rec = rec;
            slot[i].m_oid = oid;
            slot[i].m_id = id;
            slot[i].m_num = (u_short)num;
            slot[i]._c = 0;
            return 0;
        }
        if (found == -1)
        {
            /* the ROM looks on from the free slot */
            for (int j = i; j < 0xc; j++)
            {
                if (slot[j].m_id == id)
                {
                    found = j;
                    break;
                }
            }
        }
        if (found != -1)
        {
            slot[found].m_num++;
            if (slot[found].m_num > 99)
                slot[found].m_num = 99;
            return 0;
        }
    }
    u_int oid = makeOid(2, 5, -1);
    slot[i].m_rec = rec;
    slot[i].m_oid = oid;
    slot[i].m_id = id;
    slot[i].m_num = 1;
    slot[i]._c = 0;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Item id out of the slots: put away first when held, no longer the find
// target; a stacking item with more than one counts down, else its slot
// empties (one: only the first such slot).
// ROR 0x001c82f8

void GamMan::delItem(short id, int one)
{
    if (id == m_equipItem)
        equipItem(0xa00);
    if (id == m_findTargetItem)
        setFindTargetItem(0xa00);
    for (int i = 0; i < 0xc; i++)
    {
        GamItemSlot *slot = &m_itemHave.m_begin[i];
        if (slot->m_id != id)
            continue;
        if (m_sysData.testFlags(id, 0x20000) && slot->m_num > 1)
        {
            slot->m_num--;
            return;
        }
        slot->m_id = 0xa00;
        if (one)
            return;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c8414

void GamMan::delItemAll(short id)
{
    delItem(id, 0);
    delStock(id, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c8508

int GamMan::getItemNum(short id)
{
    for (int i = 0; i < 0xc; i++)
    {
        GamItemSlot *slot = &m_itemHave.m_begin[i];
        if (slot->m_id == id)
            return slot->m_num;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c8598

void GamMan::selectItem(short id)
{
    m_selItem = NULL;
    for (int i = 0; i < 0xc; i++)
    {
        GamItemSlot *slot = &m_itemHave.m_begin[i];
        if (slot->m_id == id)
        {
            m_selItem = slot;
            m_selItemNum = slot->m_num;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// 1 when a slot (or with withStock a stock record) holds item id.
// ROR 0x001c8a28

int GamMan::hasItem(short id, int withStock)
{
    for (int i = 0; i < 0xc; i++)
    {
        if (m_itemHave.m_begin[i].m_id == id)
            return 1;
    }
    if (withStock != 0)
    {
        for (SeqNode<GamItemSlot> *node = m_itemStock.m_used.begin(); node != m_itemStock.m_used.end();
             node = node->m_next)
        {
            if (node->m_data.m_id == id)
                return 1;
        }
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// How many of item id the slots (and with withStock the stock) hold: a
// stacking item's counts, one per slot else.
// ROR 0x001c8ae8

int GamMan::getItemCount(short id, int withStock)
{
    int count = 0;
    for (int i = 0; i < 0xc; i++)
    {
        GamItemSlot *slot = &m_itemHave.m_begin[i];
        if (slot->m_id != id)
            continue;
        if (m_sysData.testFlags(slot->m_id, 0x20000))
            count += slot->m_num;
        else
            count++;
    }
    if (withStock != 0)
    {
        for (SeqNode<GamItemSlot> *node = m_itemStock.m_used.begin(); node != m_itemStock.m_used.end();
             node = node->m_next)
        {
            if (node->m_data.m_id != id)
                continue;
            if (m_sysData.testFlags(node->m_data.m_id, 0x20000))
                count += node->m_data.m_num;
            else
                count++;
        }
    }
    return count;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c8c50

int GamMan::hasFreeItemSlot()
{
    int i = 0;
    if (m_itemHave.m_begin[0].m_id != 0xa00)
    {
        for (i = 1; i < 0xc && m_itemHave.m_begin[i].m_id != 0xa00; i++)
            ;
    }
    return i < 0xc;
}

// ──────────────────────────────────────────────────────────────────────
// Whether item id fits the slots (id 0: the item of object oid, its param
// 2): a stacking item already held under 99, or a free slot before the
// first empty one.
// ROR 0x001c8ca4

int GamMan::canAddItem(u_int oid, short id)
{
    short param[8];
    param[0] = 0;
    short item = id;
    if (id == 0 && isObj(oid))
    {
        getObjParam(oid, 2, param);
        item = param[0];
    }

    int i = 0;
    if (m_itemHave.m_begin[0].m_id != 0xa00)
    {
        do
        {
            GamItemSlot *slot = &m_itemHave.m_begin[i];
            if (slot->m_id == item && slot->m_id != 0 && m_sysData.testFlags(item, 0x20000) &&
                slot->m_num < 99)
                return 1;
            i++;
        } while (i < 0xc && m_itemHave.m_begin[i].m_id != 0xa00);
    }
    return i < 0xc;
}

// ──────────────────────────────────────────────────────────────────────
// Item id into the stock: a stacking item already there counts up (1 when
// it reaches 99), else a new record (count 1 or count, at most 99) with a
// new object id. 1 when the stock's 0x14e records are taken.
// ROR 0x001c8f7c

int GamMan::addStock(short id, int count)
{
    if ((u_int)m_itemStock.m_used.m_count >= 0x14e)
        return 1;

    u_short num = (u_short)count;
    if (m_sysData.testFlags(id, 0x20000))
    {
        for (SeqNode<GamItemSlot> *node = m_itemStock.m_used.begin(); node != m_itemStock.m_used.end();
             node = node->m_next)
        {
            if (node->m_data.m_id != id)
                continue;
            if (count == 0)
                num = 1;
            node->m_data.m_num += num;
            if (node->m_data.m_num < 100)
                return 0;
            node->m_data.m_num = 99;
            return 1;
        }
    }
    if (count == 0)
        num = 1;
    if (num > 99)
        num = 99;

    GamItemSlot item;
    item.m_rec = m_sysData.getRec(id);
    item.m_oid = makeOid(2, 5, -1);
    item.m_id = id;
    item.m_num = num;
    item._c = 0;
    m_itemStock.add(item);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Item id out of the stock: with one a stacking record counts down first
// and only one record goes; without, every record of id. 1 when a record
// was freed.
// ROR 0x001c94d8

int GamMan::delStock(short id, int one)
{
    if (m_itemStock.m_used.m_count == 0)
        return 0;
    SeqNode<GamItemSlot> *node = m_itemStock.m_used.begin();
    while (node != m_itemStock.m_used.end())
    {
        if (node->m_data.m_id != id)
        {
            node = node->m_next;
            continue;
        }
        if (m_sysData.testFlags(id, 0x20000) && one != 0 && node->m_data.m_num > 1)
        {
            node->m_data.m_num--;
            return 0;
        }
        SeqNode<GamItemSlot> *next = node->m_next;
        m_itemStock.remove(node);
        if (one != 0 || m_itemStock.m_used.m_count == 0)
            return 1;
        node = next;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Empty in the US build (seqFunc027 calls it).
// ROR 0x001ca0d4

void GamMan::empty1ca0d4()
{
}

// ──────────────────────────────────────────────────────────────────────
// Every sky dome of the stage released (kind is unused: the ROM reads only
// this), then the map's keep buffer freed when there was one.
// ROR 0x001cb01c // callcheck: rorDebugPrintf (the empty debug print of dropMap : m_p_keep_buf, left out)

void GamMan::releaseTenkyus(int kind)
{
    (void)kind;
    bool any = false;
    SeqNode<GamTenkyuRec> *node = m_pool1d240.m_used.begin();
    while (node != m_pool1d240.m_used.end())
    {
        g_magSystem.m_scene.releaseTenkyu(node->m_data.m_no);
        SeqNode<GamTenkyuRec> *next = node->m_next;
        any = true;
        m_pool1d240.remove(node);
        node = next;
    }
    if (any)
    {
        utlMemFree(m_keepBuf);
        m_keepBuf = NULL;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Bit 0 of every sky dome record's flags cleared. Native 004 case 1 waits
// on the result, which is always 0.
// ROR 0x001cb1c0

int GamMan::resetTenkyuFlags()
{
    for (SeqNode<GamTenkyuRec> *node = m_pool1d240.m_used.begin();
         node != m_pool1d240.m_used.end(); node = node->m_next)
    {
        if ((node->m_data.m_flags & 1) != 0)
            node->m_data.m_flags &= ~1u;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cb4f0

void GamMan::setPcParam(int which, const void *data)
{
    if (m_pcPtr != NULL)
        ((ACHRstat *)m_pcPtr)->setParam(which, data);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cba0c

void GamMan::addObjFx(u_int oid, int no)
{
    // The ROM first walks the used list to its end and does nothing with it.
    GamObjFxRec rec;
    rec.m_oid = oid;
    rec.m_no = no;
    m_pool20b00.add(rec);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cbafc

int GamMan::findObjFx(u_int oid)
{
    for (SeqNode<GamObjFxRec> *node = m_pool20b00.m_used.begin();
         node != m_pool20b00.m_used.end(); node = node->m_next)
    {
        if (node->m_data.m_oid == oid)
            return node->m_data.m_no;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cbb80

void GamMan::ctlObj1cbb80(u_int oid)
{
    SeqNode<GamObjFxRec> *node = m_pool20b00.m_used.begin();
    while (node != m_pool20b00.m_used.end())
    {
        if (node->m_data.m_oid != oid)
        {
            node = node->m_next;
            continue;
        }
        if (node->m_data.m_no != -1)
            g_magSystem.m_scene.m_837e0.release(node->m_data.m_no);
        SeqNode<GamObjFxRec> *next = node->m_next;
        m_pool20b00.remove(node);
        node = next;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cc004

void GamMan::clearVFchrSlot(u_int oid)
{
    for (int i = 0; i < 0x180; i++)
    {
        if ((u_int)m_vfchrSlot[i] == oid)
        {
            m_vfchrSlot[i] = 0;
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// The fchr of oid: its actor released, its slot cleared, the object deleted
// and its list node freed.
// ROR 0x001cc7c8 // callcheck: FUN_00105ba4 (the object's deleting destructor, vtable slot 1 with 3 in the ROM: delete here)

void GamMan::releaseFchr(u_int oid)
{
    for (SeqNode<FCHRstat *> *node = m_fchrList.m_used.begin(); node != m_fchrList.m_used.end();
         node = node->m_next)
    {
        FCHRstat *chr = node->m_data;
        if (chr->m_oid != oid)
            continue;
        chr->releaseActor();
        m_fchr[gamOidSlot(chr->m_oid)] = NULL;
        delete node->m_data;
        m_fchrList.remove(node);
        return;
    }
}

// ──────────────────────────────────────────────────────────────────────
// The vfchr of oid: re-linked, its avfchr records freed, then its own
// record (the actor released when it has one, its +0x1f24c entry
// cleared). The object is not deleted.
// ROR 0x001cc970

void GamMan::releaseVFchr(u_int oid)
{
    if (m_vfchrList.m_used.m_count == 0)
        return;
    relinkObj(oid);

    SeqNode<FCHRstat *> *node = m_avfchrList.m_used.begin();
    while (node != m_avfchrList.m_used.end())
    {
        SeqNode<FCHRstat *> *next = node->m_next;
        if (node->m_data->m_oid == oid)
            m_avfchrList.remove(node);
        node = next;
    }

    for (node = m_vfchrList.m_used.begin(); node != m_vfchrList.m_used.end(); node = node->m_next)
    {
        if (node->m_data->m_oid != oid)
            continue;
        if (node->m_data->m_actor != -1)
        {
            markReset();
            node->m_data->releaseActor();
        }
        clearVFchrSlot(oid);
        m_vfchrList.remove(node);
        return;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Object oid leaves the game: its private collisions (kind 5 one, kind 6
// four) go first. An achr -- not the dog -- loses its links, its secta
// cell, EmyManager's object (type 2), its actor and its slot, is deleted
// and its node freed (the PC's words cleared when it was the PC); this
// returns before the sounds. An fchr / vfchr goes through releaseFchr /
// releaseVFchr. Then the object's sounds stop.
// ROR 0x001ccc30 // callcheck: FUN_00105ba4 (the object's deleting destructor, vtable slot 1 with 3 in the ROM: delete here)

void GamMan::releaseChr(u_int oid)
{
    if (!isObj(oid))
        return;

    GamObjColli colli;
    colli.m_kind = -1;
    getObjParam(oid, 0x18, &colli);
    MagScene &scene = g_magSystem.m_scene;
    if (colli.m_kind == 5)
    {
        scene.removePrivateColli(colli.m_colli[0]);
    }
    else if (colli.m_kind == 6)
    {
        scene.removePrivateColli(colli.m_colli[0]);
        scene.removePrivateColli(colli.m_colli[1]);
        scene.removePrivateColli(colli.m_colli[2]);
        scene.removePrivateColli(colli.m_colli[3]);
    }

    u_int list = gamOidList(oid);
    if (list == 1)
    {
        if (m_achrList.m_used.m_count == 0 || !isAchr(oid))
            return;
        if (oid == (m_dogPtr != NULL ? m_dogPtr->m_oid : 0))
            return;
        if (oid == (m_pcPtr != NULL ? m_pcPtr->m_oid : 0))
        {
            m_pcPtr = NULL;
            m_pcMotion = -1;
            m_pcActorPtr = NULL;
            m_pcActor = -1;
        }
        relinkObj(oid);
        for (SeqNode<ACHRstat *> *node = m_achrList.m_used.begin(); node != m_achrList.m_used.end();
             node = node->m_next)
        {
            if (node->m_data->m_oid != oid)
                continue;
            if (oid == (m_pcPtr != NULL ? m_pcPtr->m_oid : 0))
                m_pcPtr = NULL;
            node->m_data->clearSecta(1);
            ACHRstat *chr = node->m_data;
            u_int id = chr->m_oid;
            if (gamOidType(id) == 2)
                g_emyManager.releaseObj(id);
            chr->releaseActor();
            m_achr[gamOidSlot(id)] = NULL;
            delete node->m_data;
            m_achrList.remove(node);
            return;
        }
    }
    else if (list == 0 || list == 2)
    {
        if (m_fchrList.m_used.m_count != 0 && isFchr(oid))
        {
            markReset();
            relinkObj(oid);
            releaseFchr(oid);
        }
        releaseVFchr(oid);
    }
    m_sndMan.del_se_3d_pos_chr(oid);
    m_sndMan.del_str_3d_pos_chr(oid);
}

// ──────────────────────────────────────────────────────────────────────
// Empty in the US build (seqFunc027 calls it).
// ROR 0x001cd7c0

void GamMan::empty1cd7c0()
{
}

// ──────────────────────────────────────────────────────────────────────
// The param which of object oid through its class (achr, or fchr for the
// vfchr and fchr lists); nothing for an unknown oid. Returns 0 in the ROM.
// ROR 0x001d1c80

void GamMan::getObjParam(u_int oid, int which, void *out)
{
    int live = isObj(oid);
    u_int list = gamOidList(oid);
    if (!live)
        return;
    if (list == 1)
        getAchr(oid)->getParam(which, out);
    else if (list == 0)
        getVFchr(oid)->getParam(which, out);
    else if (list == 2)
        getFchr(oid)->getParam(which, out);
}

// ════════════════════════════════════════════════════════════════════════
//  Object creation and the per-object accessors (session 4)
// ════════════════════════════════════════════════════════════════════════

/* The vfchr storage m_1f248 is 0x180 FCHRstat of the EE's 0x2c0 bytes. */
static_assert(sizeof(FCHRstat) <= 0x2c0, "FCHRstat must fit the vfchr storage stride");

// The ROM's inline store after loading model 0x800 or the "target" model:
// the actor's scale x, y, z = 9.9999994e-8 (0x33d6bf94), its matrix dirty.
static void gamShrinkActor(int no)
{
    if (no < 0)
        return;     // PORT: the ROM writes actor -1's words (a failed load)
    DogActor &actor = g_magSystem.m_scene.m_actors[no];
    actor.m_dirty = 1;
    actor.m_scale[2] = 9.9999994e-8f;
    actor.m_scale[0] = 9.9999994e-8f;
    actor.m_scale[1] = 9.9999994e-8f;
}

// The ROM's inline actor virtual +0x44 with 1 (DogModel::setHide).
static void gamHideActor(int no)
{
    g_magSystem.m_scene.m_actors[no].setHide(1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cbfb0

FCHRstat *GamMan::allocVFchr(u_int oid)
{
    for (int i = 0; i < 0x180; i++)
    {
        if (m_vfchrSlot[i] == 0)
        {
            m_vfchrSlot[i] = (int)oid;
            return (FCHRstat *)(m_1f248 + i * sizeof(FCHRstat));
        }
    }
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// A vfchr object: an FCHRstat built in its storage slot, params 8 (arg, the
// room it belongs to) and 0x43 (pos), m_140 = 1, the vfchr list, and the
// avfchr list when arg is the room loaded now.
// ROR 0x001cc044 // callcheck: ObjStat (ObjStat::ObjStat, through the inline FCHRstat constructor)

void GamMan::addVFchr(int model, u_int oid, int arg, int kind, intptr_t pos)
{
    if (isVFchr(oid))
        return;
    markReset();
    FCHRstat *chr = allocVFchr(oid);
    if (chr == NULL)
        return;     // PORT: the ROM goes on with NULL
    new (chr) FCHRstat(oid, model, (short)kind);
    chr->init((short)kind);
    if (pos != 0)
        chr->setPos((const float *)pos);
    chr->setParam(8, &arg);
    chr->setParam(0x43, (const void *)pos);
    chr->m_140 = 1;
    m_vfchrList.add(chr);
    if (m_wrdNo == arg)
        m_avfchrList.add(chr);
}

// ──────────────────────────────────────────────────────────────────────
// Model 0xffff means none. An achr (list 1) gets its slot and list node; a
// type-2 achr (an enemy) also an EmyManager enemy with the chr no of its
// kind's field 0x72, the image slot place and the script word, placed again
// at pos. An fchr (list 2) gets its slot and node. Other lists add nothing.
// ROR 0x001cc250 // callcheck: operator_new, ObjStat (new ACHRstat / FCHRstat: their constructors are inline over ObjStat::ObjStat)

void GamMan::addChr(int model, u_int oid, int kind, const void *pos, int place)
{
    if (model == 0xffff)
        model = -1;
    u_int list = gamOidList(oid);
    if (list == 1)
    {
        ACHRstat *chr = new ACHRstat(oid, model, (short)kind);
        chr->init((short)kind);
        chr->initParamFuncs();
        m_achr[gamOidSlot(oid)] = chr;
        if (pos != NULL)
            chr->setPos((const float *)pos);
        m_achrList.add(chr);
        if (gamOidType(oid) == 2)
        {
            u_short chrNo[8] = {0};     // the ROM's stack words
            m_sysData.getField((u_short)kind, 0x72, chrNo);
            g_emyManager.addEmy(chrNo[0], (short)kind, chr, m_seqWord, m_image[place], pos, oid);
            if (pos != NULL)
                setObjPos(oid, (const float *)pos);
        }
    }
    else if (list == 2)
    {
        markReset();
        FCHRstat *chr = new FCHRstat(oid, model, (short)kind);
        chr->init((short)kind);
        m_fchr[gamOidSlot(oid)] = chr;
        if (pos != NULL)
            chr->setPos((const float *)pos);
        m_fchrList.add(chr);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The model groups' archives (jump table data 0x2fbed0, the strings at
// 0x2fbcc8..); group 7 has none.
// ROR 0x001ca604

const char *GamMan::getChrRpk(u_int no)
{
    static const char *const s_rpk[0x16] = {
        "data/ch/action/ACTION.RPK", "data/ch/target/TRG.RPK",  "data/ch/fix/FIX.RPK",
        "data/ch/item/ITEM.RPK",     "data/ch/inst/INST.RPK",   "data/ch/enemy/ENEMY.RPK",
        "data/ch/door/DOOR.RPK",     NULL,                      "data/ch/pc/PC.RPK",
        "data/ch/bwn/BWN.RPK",       "data/ch/01/CHAP01.RPK",   "data/ch/02/CHAP02.RPK",
        "data/ch/03/CHAP03.RPK",     "data/ch/04/CHAP04.RPK",   "data/ch/05/CHAP05.RPK",
        "data/ch/06/CHAP06.RPK",     "data/ch/07/CHAP07.RPK",   "data/ch/08/CHAP08.RPK",
        "data/ch/09/CHAP09.RPK",     "data/ch/10/CHAP10.RPK",   "data/ch/11/CHAP11.RPK",
        "data/ch/12/CHAP12.RPK",
    };
    if (no < 0x16)
        return s_rpk[no];
    return NULL;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ca598

void GamMan::setObjLight(int actor, u_int oid)
{
    switch (gamOidType(oid))
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 5:
        g_magSystem.setActorLightID(actor, 1);
        return;
    case 4:
    case 9:
        g_magSystem.setActorLightID(actor, 0);
        return;
    default:
        return;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001ca8e8

int GamMan::createObjByModel(int model, u_int oid, int kind, const void *pos,
                             int place, int add)
{
    if (oid != 0 && isObj(oid))
        return -1;
    int actor = g_magSystem.loadModel((short)model, -1);
    setObjLight(actor, oid);
    if (add != 0)
        g_gamMan.addChr(actor, oid, kind, pos, place);
    if ((short)model == 0x800)
        gamShrinkActor(actor);
    return actor;
}

// ──────────────────────────────────────────────────────────────────────
// A new object from the model file name in the archive of its group (kind
// >> 10). oid 0 makes one: list 2 for groups 1..7, else 1, the type from
// init's table m_225f4[4 + group]. Params 0x1004 (the name) and 0x1000
// (kind). group is never read.
// ROR 0x001ca72c

u_int GamMan::createObjByName(u_int kind, const char *name, int group, u_int oid,
                              int kind2, const void *pos, int place)
{
    kind &= 0xffff;
    if (oid == 0)
    {
        u_int no = kind >> 10;
        u_int list = 1;
        if (no != 0 && no < 8)
            list = 2;
        oid = makeOid(list, (u_int)m_225f4[4 + no], -1);
    }
    else if (isObj(oid))
    {
        return 0;
    }
    int actor = g_magSystem.m_scene.loadModelByName(&g_magSystem.m_i3d, name, -1,
                                                    getChrRpk(kind >> 10));
    setObjLight(actor, oid);
    g_gamMan.addChr(actor, oid, kind2, pos, place);
    u_int param[4];
    param[0] = kind;
    setObjParam(oid, 0x1004, name);
    setObjParam(oid, 0x1000, param);
    if (strcmp(name, "target") == 0)
        gamShrinkActor(actor);
    return oid;
}

// ──────────────────────────────────────────────────────────────────────
// A type-2 achr's param 0x18 (its shape) also restarts the place task with
// the new radius.
// ROR 0x001d1d5c

int GamMan::setObjParam(u_int oid, int which, const void *data)
{
    if (!isObj(oid))
        return 0;
    u_int list = gamOidList(oid);
    if (list == 1)
    {
        ACHRstat *chr = getAchr(oid);
        chr->setParam(which, data);
        if (which == 0x18 && gamOidType(oid) == 2)
            startAchrPlaceTask(getObjRadius(oid, NULL), chr, 0);
    }
    else if (list == 0)
    {
        getVFchr(oid)->setParam(which, data);
    }
    else if (list == 2)
    {
        getFchr(oid)->setParam(which, data);
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cfbe4

int GamMan::setObjPos(u_int oid, const float *pos)
{
    if (!isObj(oid))
        return 0;
    u_int list = gamOidList(oid);
    if (list == 1)
        getAchr(oid)->setPos(pos);
    else if (list == 0)
        getVFchr(oid)->setPos(pos);
    else if (list == 2)
        getFchr(oid)->setPos(pos);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d05f4

int GamMan::getObjPos(u_int oid, float *out)
{
    if (!isObj(oid))
        return 0;
    u_int list = gamOidList(oid);
    if (list == 1)
        getAchr(oid)->getPos(out);
    else if (list == 0)
        getVFchr(oid)->getPos(out);
    else if (list == 2)
        getFchr(oid)->getPos(out);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d06b8

int GamMan::getObjRot(u_int oid, float *out)
{
    if (!isObj(oid))
        return 0;
    u_int list = gamOidList(oid);
    if (list == 1)
        getAchr(oid)->getRot(out);
    else if (list == 2)
        getFchr(oid)->getRot(out);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001d0758

int GamMan::getObjRotY(u_int oid, float *out)
{
    if (!isObj(oid))
        return 0;
    u_int list = gamOidList(oid);
    int actor;
    if (list == 1)
        actor = getAchr(oid)->m_actor;
    else if (list == 2)
        actor = getFchr(oid)->m_actor;
    else
        actor = getVFchr(oid)->m_actor;     // PORT: the ROM reads the word at 0x108 (a NULL object's)
    *out = g_magSystem.m_scene.m_actors[actor].getRotY();
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cfb48

float GamMan::getObjDirTo(u_int oid, u_int target)
{
    if (!isObj(oid) || !isObj(target))
        return 0.0f;
    float from[4];
    float to[4];
    getObjPos(oid, from);
    getObjPos(target, to);
    return utlAtan2(to[0] - from[0], to[2] - from[2]);
}

// ──────────────────────────────────────────────────────────────────────
// dir is wrapped into -pi..pi once (in doubles, as the ROM does); still
// outside it, nothing changes. Then the rotation's y.
// ROR 0x001d024c // callcheck: dpadd, dpcmp, dpsub, dptofp, fptodp (the soft-float doubles, C doubles here)

void GamMan::setObjDir(u_int oid, float dir)
{
    const double pi = 3.141592653589793;
    if ((double)dir < -pi || (double)dir > pi)
    {
        if ((double)dir < -pi)
            dir = (float)((double)dir + 2.0 * pi);
        if ((double)dir > pi)
            dir = (float)((double)dir - 2.0 * pi);
        if ((double)dir < -pi || (double)dir > pi)
            return;
    }
    float rot[4];
    u_int list = gamOidList(oid);
    if (list == 1)
    {
        ACHRstat *chr = getAchr(oid);
        chr->getRot(rot);
        rot[1] = dir;
        chr->setRot(rot);
    }
    else if (list == 0)
    {
        FCHRstat *chr = getVFchr(oid);
        chr->getRot(rot);
        rot[1] = dir;
        chr->setRot(rot);
    }
    else if (list == 2)
    {
        FCHRstat *chr = getFchr(oid);
        chr->getRot(rot);
        rot[1] = dir;
        chr->setRot(rot);
    }
}

// ──────────────────────────────────────────────────────────────────────
// resetObj's counterpart. achr: 0x40 takes it off the secta (flags lose
// 0x10000080, a new cell, queued for update; an enemy's place task again),
// 0x10000000 re-links, 1 hides it (and, for the PC, its held item's actor;
// the linked object's actor; an enemy's +0x3f4 bit 0), 0x10 the marker.
// vfchr / fchr: the bits on both records, 1 drops the marker and hides,
// 0x40 / 0x10000000 as for an achr, 0x10 the marker of an active one.
// ROR 0x001d10d0

int GamMan::setObj(u_int oid, u_int bits)
{
    if (!isObj(oid))
        return 0;
    u_int list = gamOidList(oid);
    if (list == 1)
    {
        ACHRstat *chr = getAchr(oid);
        if ((bits & 0x40) != 0)
        {
            chr->m_flags &= ~0x10000080u;
            chr->findSecta();
            queueObjUpdate(0, oid);
            if (gamOidType(oid) == 2)
                startAchrPlaceTask(getObjRadius(oid, NULL), chr, 1);
        }
        if ((bits & 0x10000000) != 0)
            relinkObj(oid);
        if ((bits & 1) != 0)
        {
            if (oid == (u_int)m_21fcc && m_pcEquipItemChrID != -1)
                gamHideActor(m_pcEquipItemChrID);
            int link = chr->m_link;
            if (link != -1 && isObj((u_int)link))
                gamHideActor(getActorNo((u_int)link));
            chr->setHide(1);
            if (gamOidType(oid) == 2)
                chr->m_attr[2] &= ~1u;
        }
        if ((bits & 0x10) != 0)
            ctlObj1cb8e4(oid, 0x32, 0);
        chr->m_flags |= bits;
        return 1;
    }
    if (list != 0 && list != 2)
        return 0;

    FCHRstat *vchr = getVFchr(oid);
    if (vchr != NULL)
    {
        vchr->m_flags |= bits;
        if ((bits & 1) != 0)
        {
            ctlObj1cbb80(oid);
            vchr->setHide(1);
        }
    }
    FCHRstat *chr = getFchr(oid);
    if (chr != NULL)
    {
        chr->m_flags |= bits;
        if ((bits & 0x40) != 0)
        {
            chr->m_flags &= ~0x10000080u;
            chr->findSecta();
            queueObjUpdate(0, oid);
            chr->findSecta();
        }
        if ((bits & 0x10000000) != 0)
            relinkObj(oid);
        if ((bits & 1) != 0)
        {
            ctlObj1cbb80(oid);
            chr->setHide(1);
        }
        if ((bits & 0x10) != 0 && isFchrActive(oid))
            ctlObj1cb8e4(oid, 0x32, 0);
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// placeAchr, then, unless flag 0x8000000 holds it, an achr in move state 2
// with no place task gets sub state 4, word [4] cleared and a TskSeqEvent
// {60 frames, radius} on m_placeTask. The ROM copies the move block out
// and back around the change; the task reads only its first two words.
// ROR 0x001d0efc // callcheck: operator_new (new TskSeqEvent)

void GamMan::startAchrPlaceTask(float radius, ACHRstat *chr, int keep)
{
    u_char place[0x20];
    placeAchr((int)chr->m_oid, place, 1);
    if ((chr->m_flags & 0x8000000) != 0)
        return;
    if (chr->m_colPrm[0] != 2 || g_taskMan.getTask(chr->m_placeTask) != NULL)
        return;
    chr->m_colPrm[1] = 4;
    chr->m_colPrm[4] = 0;
    struct
    {
        int   m_frames;
        float m_radius;
    } param = {0x3c, radius};
    if (g_taskMan.getTask(chr->m_placeTask) == NULL)
    {
        TskSeqEvent *task = new TskSeqEvent(chr->m_oid, 0, &param);
        g_taskMan.addTask(1, -1, 1, 0, task, 0, &chr->m_placeTask, 0x11);
    }
    else if (keep == 0)
    {
        // Unreachable (the task was just found missing), as in the ROM.
        static_cast<TskSeqEvent *>(g_taskMan.findTask(chr->m_placeTask))->m_radius = radius;
    }
}

// ──────────────────────────────────────────────────────────────────────
// An object without a marker gets one at its position (b is never read).
// ROR 0x001cb8e4

void GamMan::ctlObj1cb8e4(u_int oid, int a, int b)
{
    if (findObjFx(oid) != -1)
        return;
    float pos[4];
    g_gamMan.getObjPos(oid, pos);
    addObjMarker(oid, a, pos);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cb970

void GamMan::addObjMarker(u_int oid, int kind, const float *pos)
{
    if (kind != 0x32)
        return;
    EffParam param;
    param.m_kind = 0x32;
    param.m_08 = 0;
    float data[4] = {pos[0], pos[1], pos[2], 50.0f};
    memcpy(param.m_data, data, sizeof(data));
    addObjFx(oid, g_magSystem.m_scene.m_837e0.createEff(&param));
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c6f08

void GamMan::setSysParam(int which, const void *v)
{
    switch (which)
    {
    case 0x11:
        m_optOutput = *(const int *)v;
        return;
    case 0x12:
        m_optVibration = *(const int *)v;
        return;
    case 0x13:
        m_21f94 = *(const int *)v;
        return;
    case 0x14:
        m_optMasterLevel = *(const int *)v;
        return;
    case 0x16:
        memcpy(&m_21fbc[0], v, 4);
        return;
    case 0x17:
        memcpy(&m_21fbc[1], v, 4);
        return;
    case 0x18:
        memcpy(&m_21fbc[2], v, 4);
        return;
    case 0x1b:
        m_21f54 = *(const int *)v;
        return;
    case 0x1c:
        m_04 = *(const int *)v;
        return;
    default:        // 0x15, 0x19, 0x1a and the rest: nothing
        return;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c898c

int GamMan::getSelItemNum()
{
    return m_selItemNum;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cf854 // callcheck: operator_new (new TskMsgMan)

void GamMan::openMsg(const char *text, int arg, int time)
{
    if (g_taskMan.getTask(m_msgTaskId) != NULL)
    {
        g_taskMan.killTask(m_msgTaskId);
        m_msgTaskId = -1;
    }
    TskMsgMan *task = new TskMsgMan(text, arg, m_msgParam, 1, m_msgMode, time);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, &m_msgTaskId, 0x11);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cf6bc

void GamMan::showMsgSprite(int on, const int *param)
{
    S2dMes &mes = g_magSystem.m_scene.m_mes;
    if (on == 0)
    {
        mes.setSpriteFadeOut(3, param[4]);
        return;
    }
    int anim = param[0];
    mes.setSpriteProcessOn(3);
    mes.setSpriteColorStd(3);
    mes.setSpriteState(3, 1);
    mes.setSpriteDisp(3, 1);
    mes.setSpriteAnim(3, anim);
    mes.setSpriteStdPoint(3, param[1] << 4, param[2] << 4, 0x80010);
    mes.setSpriteOffPoint(3, param[5] << 4, param[6] << 4);
    int fade = param[3];
    mes.setSpriteProcessOn(3);
    mes.setSpriteFadeIn(3, fade);
}

// ──────────────────────────────────────────────────────────────────────
// The chapter's own SE pack read and registered again (reloadChapterSe1f /
// 0c, after a script pack swap): 3xxx.se as loadStageSnd picks it, then
// with withArea the 1xxx area pack of loadStageSnd's pair. Chapter 9 has
// no 3xxx pack, chapter 9 and > 13 no area pack.
// ROR 0x001c2848 // callcheck: FileReader, FileReadReq, read, ~FileReader (the reader local and the request in gamReadSe, inline in the ROM)

void GamMan::loadChapterSe(int chapter, int withArea)
{
    GamSndMan *snd = &m_sndMan;
    void *buf = g_utlMemory.allocDebug(0x101480, s_file, 1509, 0, 1);
    FileReader reader;

    switch (chapter)
    {
    case 1:  gamReadSe(reader, "data/snd/xv/3200.se", buf, 0x80000); snd->loadSePack("3200.se", buf, 0); break;
    case 2:  gamReadSe(reader, "data/snd/xv/3201.se", buf, 0x80000); snd->loadSePack("3201.se", buf, 0); break;
    case 3:  gamReadSe(reader, "data/snd/xv/3202.se", buf, 0x80000); snd->loadSePack("3202.se", buf, 0); break;
    case 4:  gamReadSe(reader, "data/snd/xv/3203.se", buf, 0x80000); snd->loadSePack("3203.se", buf, 0); break;
    case 5:
    case 13: gamReadSe(reader, "data/snd/xv/3204.se", buf, 0x80000); snd->loadSePack("3204.se", buf, 0); break;
    case 6:  gamReadSe(reader, "data/snd/xv/3205.se", buf, 0x80000); snd->loadSePack("3205.se", buf, 0); break;
    case 7:  gamReadSe(reader, "data/snd/xv/3206.se", buf, 0x80000); snd->loadSePack("3206.se", buf, 0); break;
    case 8:  gamReadSe(reader, "data/snd/xv/3207.se", buf, 0x80000); snd->loadSePack("3207.se", buf, 0); break;
    case 10: gamReadSe(reader, "data/snd/xv/3208.se", buf, 0x80000); snd->loadSePack("3208.se", buf, 0); break;
    case 11: gamReadSe(reader, "data/snd/xv/3209.se", buf, 0x80000); snd->loadSePack("3209.se", buf, 0); break;
    case 12: gamReadSe(reader, "data/snd/xv/3210.se", buf, 0x80000); snd->loadSePack("3210.se", buf, 0); break;
    default: break;
    }

    if (withArea != 0)
    {
        switch (chapter)
        {
        case 1:
        case 7:
        case 13:
            gamReadSe(reader, "data/snd/xv/1102.se", buf, 0x80000);
            snd->loadSePack("1102.se", buf, 0);
            break;
        case 2:
        case 3:
        case 4:
        case 10:
        case 11:
        case 12:
            gamReadSe(reader, "data/snd/xv/1100.se", buf, 0x80000);
            snd->loadSePack("1100.se", buf, 0);
            break;
        case 5:
            gamReadSe(reader, "data/snd/xv/1101.se", buf, 0x80000);
            snd->loadSePack("1101.se", buf, 0);
            break;
        case 6:
            gamReadSe(reader, "data/snd/xv/1103.se", buf, 0x80000);
            snd->loadSePack("1103.se", buf, 0);
            break;
        case 8:
            gamReadSe(reader, "data/snd/xv/1104.se", buf, 0x80000);
            snd->loadSePack("1104.se", buf, 0);
            break;
        default:
            break;
        }
    }
    utlMemFree(buf);
}

// ──────────────────────────────────────────────────────────────────────
// Map units (the sky / room packs of data/bg/mapunit.bin, one scene sky
// slot each, kept in the +0x1d240 records). A unit already loaded is only
// shown by mode; otherwise the stage's units are released and the named
// one (its name up to the first '.') is read from the pack into a
// keep buffer and handed to loadMapPack.
// ROR 0x001ca0dc // callcheck: rorDebugPrintf, BedPfh, FileReader, FileReadReq, ~FileReader (the empty debug print, left out; the locals' constructors and destructor)

void GamMan::loadMapUnit(const char *name, int id, int mode)
{
    for (SeqNode<GamTenkyuRec> *node = m_pool1d240.m_used.begin(); node != m_pool1d240.m_used.end();
         node = node->m_next)
    {
        if (node->m_data.m_unit == id)
        {
            showMapUnits(mode);
            return;
        }
    }
    releaseTenkyus(2);

    int unit;
    void *buf;
    {
        FileReader reader;
        FileReadReq req;
        alignas(64) u_char head[0x1800];    /* the ROM aligns a stack buffer to 64 */
        req.m_buf = head;
        req.m_size = 0x1800;
        req.m_08 = 1;
        req.m_name = "data/bg/mapunit.bin";
        reader.read(&req, 0);
        BedPfh pfh(req.m_buf);

        char unitName[0x40];
        strncpy(unitName, name, 0x40);
        *strstr(unitName, ".") = '\0';
        unit = pfh.getFileIndex(unitName);
        if (unit < 0)
            return;     // PORT: the ROM reads the word before the size table
        int size = pfh.getFileSize(unit);
        if (size < 1)
            return;
        size = (size + 0x7ff) / 2048 * 2048;
        buf = g_utlMemory.allocDebug(size, s_file, 6603, 0, 0x102);
        req.m_offset = pfh.getFileOffset(unit);
        req.m_whence = 0;
        req.m_buf = buf;
        req.m_size = size;
        req.m_close = 1;
        reader.read(&req, 0);
    }
    loadMapPack((u_short)unit, buf, mode);
    m_keepBuf = buf;
    // PORT: the ROM stores an uninitialised stack word (sp + 8) into
    // m_mapModelNo here; loadMapPack's slot is kept.
    showMapUnits(mode);
}

// ROR 0x001c9e20

void GamMan::showMapUnits(int mode)
{
    for (SeqNode<GamTenkyuRec> *node = m_pool1d240.m_used.begin(); node != m_pool1d240.m_used.end();
         node = node->m_next)
    {
        const GamTenkyuRec &rec = node->m_data;
        DogModel &model = g_magSystem.m_scene.m_tenkyu[rec.m_no].m_model[0];
        u_int kind = rec.m_flags >> 30;
        if (mode == 1)
        {
            model.setHide(0);
        }
        else if (mode == 2 || mode == 3)
        {
            u_int shown = mode == 2 ? 2 : 3;
            if (kind == shown)
            {
                model.setHide(0);
                m_mapModelNo = rec.m_no;
            }
            else if (kind == 2 || kind == 3)
            {
                model.setHide(1);
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// A unit's RTPK data into a scene sky slot: none when the unit is loaded
// (only shown by mode); a bad magic stops the game.
// ROR 0x001ca38c // callcheck: rorDebugPrintf (the empty debug prints, left out)

void GamMan::loadMapPack(u_short unit, void *data, int mode)
{
    for (SeqNode<GamTenkyuRec> *node = m_pool1d240.m_used.begin(); node != m_pool1d240.m_used.end();
         node = node->m_next)
    {
        if ((u_int)node->m_data.m_unit == unit)
        {
            showMapUnits(mode);
            return;
        }
    }
    const char *p = (const char *)data;
    if (p[0] != 'R' || p[1] != 'T' || p[2] != 'P' || p[3] != 'K')
    {
        scePrintf("map binary magic error.\n");
        abort();    /* break 0 in the ROM */
    }
    int no = g_magSystem.loadScene(unit, data, 0);
    resetAVFchrMotions();
    GamTenkyuRec rec;
    rec.m_flags = (u_int)mode << 30;
    rec.m_unit = unit;
    rec.m_no = no;
    rec._c = 0;
    m_pool1d240.add(rec);
    m_mapModelNo = no;
    showMapUnits(mode);
}

// ROR 0x001c4610

void GamMan::resetAVFchrMotions()
{
    for (SeqNode<FCHRstat *> *node = m_avfchrList.m_used.begin(); node != m_avfchrList.m_used.end();
         node = node->m_next)
        g_magSystem.m_scene.changeActorMotion(0.0166666936f, node->m_data->m_actor, 1, 0, 0);
}

// ──────────────────────────────────────────────────────────────────────
// The murderer (native 021): its oid and achr.
// ROR 0x001cb424

void GamMan::setMurder(int oid)
{
    m_murderOid = oid;
    m_murderPtr = getAchr((u_int)oid);
}

// ROR 0x001cb520

void GamMan::setPcPos(const float *pos)
{
    if (m_pcPtr == NULL)
        return;
    m_pcPtr->setPos(pos);
    m_pcPos[0] = pos[0];
    m_pcPos[1] = pos[1];
    m_pcPos[2] = pos[2];
    m_pcPos[3] = pos[3];
}

// ROR 0x001cb57c

void GamMan::setPcRot(const float *rot)
{
    if (m_pcPtr == NULL)
        return;
    m_pcPtr->setRot(rot);
    m_pcRot[0] = rot[0];
    m_pcRot[1] = rot[1];
    m_pcRot[2] = rot[2];
    m_pcRot[3] = rot[3];
}

// ROR 0x001c4f38

void GamMan::setPcAnim(int anim)
{
    g_magSystem.setActorAnim(m_pcActor, anim, 1);
}

// ROR 0x001c53e0

void GamMan::setPcJnfc(int on)
{
    m_pcJnfc = on;
    g_magSystem.setPC(m_pcActor, on);
}

// ──────────────────────────────────────────────────────────────────────
// The title's new game: the PC's and the dog's param 0x1c copied into 0x1b,
// the frame count and three words cleared.
// ROR 0x001c1378

void GamMan::resetPcStat()
{
    float param[4];
    if (m_pcPtr != NULL)
    {
        ((ACHRstat *)m_pcPtr)->getParam(0x1c, param);
        ((ACHRstat *)m_pcPtr)->setParam(0x1b, param);
    }
    if (m_dogPtr != NULL)
    {
        ((ACHRstat *)m_dogPtr)->getParam(0x1c, param);
        ((ACHRstat *)m_dogPtr)->setParam(0x1b, param);
    }
    empty1d84b0();
    m_pcJnfc = 0;
    m_vcount = 0;
    m_22004 = 0;
    m_22008 = 0;
}

// ROR 0x001d84b0

int GamMan::empty1d84b0()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// System ids below 0x800 are achrs, 0x800..0x13ff fchrs: every live one of
// the id gets the bits.
// ROR 0x001d1fd8

void GamMan::setKindFlags(int id, u_int bits, int on)
{
    u_short kind = (u_short)id;
    m_sysData.setFlags((short)kind, bits);
    if ((kind >> 9) < 4)
    {
        for (SeqNode<ACHRstat *> *node = m_achrList.m_used.begin(); node != m_achrList.m_used.end();
             node = node->m_next)
        {
            ACHRstat *chr = node->m_data;
            if ((u_short)chr->m_kind == kind)
                chr->m_flags = on != 0 ? chr->m_flags | bits : chr->m_flags & ~bits;
        }
    }
    else if ((kind >> 9) < 10)
    {
        for (SeqNode<FCHRstat *> *node = m_fchrList.m_used.begin(); node != m_fchrList.m_used.end();
             node = node->m_next)
        {
            FCHRstat *chr = node->m_data;
            if ((u_short)chr->m_kind == kind)
                chr->m_flags = on != 0 ? chr->m_flags | bits : chr->m_flags & ~bits;
        }
    }
}

// ROR 0x001d2a14

void GamMan::moveAchr(u_int oid, const float *d)
{
    float dx = d[0];
    float dz = d[2];
    float dy = d[1];
    if (gamOidList(oid) != 1)
        return;
    ACHRstat *chr = getAchr(oid);
    float pos[4];
    chr->getPos(pos);
    pos[0] += dx;
    pos[1] += dy;
    pos[2] += dz;
    chr->setPos(pos);
}

// ──────────────────────────────────────────────────────────────────────
// The scene's effect table emptied and every +0x20b00 record freed.
// ROR 0x001cbd0c

void GamMan::clearPool20b00(int kind)
{
    (void)kind;
    g_magSystem.m_scene.m_837e0.releaseAll();
    m_pool20b00.clear();
}

// ──────────────────────────────────────────────────────────────────────
// One wrap of *a into -pi..pi, in doubles as the ROM's soft-float code
// does; 0 when it is still outside.
static int gamWrapAngle(float *a)
{
    const double pi = 3.141592653589793;
    if ((double)*a >= -pi && (double)*a <= pi)
        return 1;
    float v = *a;
    if ((double)v < -pi)
        v = (float)((double)v + 2.0 * pi);
    if ((double)v > pi)
        v = (float)((double)v - 2.0 * pi);
    *a = v;
    return (double)v >= -pi && (double)v <= pi;
}

// The same wrap without the check (setPcStartPos, storePcTransform).
static float gamWrapAngleOnce(float v)
{
    const double pi = 3.141592653589793;
    if ((double)v < -pi)
        v = (float)((double)v + 2.0 * pi);
    if ((double)v > pi)
        v = (float)((double)v - 2.0 * pi);
    return v;
}

// rot's three angles wrapped in place, one at a time: one still outside
// -pi..pi leaves the object as it is (the angles before it stay wrapped).
// ROR 0x001cfe78 // callcheck: dpadd, dpcmp, dpsub, dptofp, fptodp (the soft-float doubles, C doubles here)

void GamMan::setObjRot(u_int oid, float *rot)
{
    if (!gamWrapAngle(&rot[0]) || !gamWrapAngle(&rot[1]) || !gamWrapAngle(&rot[2]))
        return;
    u_int list = gamOidList(oid);
    if (list == 1)
    {
        getAchr(oid)->setRot(rot);
    }
    else if (list == 0)
    {
        FCHRstat *chr = getVFchr(oid);
        if (chr != NULL)
            chr->setRot(rot);
    }
    else if (list == 2)
    {
        FCHRstat *chr = getFchr(oid);
        if (chr != NULL)
            chr->setRot(rot);
    }
}

// ──────────────────────────────────────────────────────────────────────
// A W000.WRD file 2 record (EE 0x20, ended by room -1): a PC start of a
// room.
struct GamStartRec
{
    int   m_wrd;                // ROR 0x00: the room
    int   _04[3];
    float m_pos[3];             // ROR 0x10
    float m_rotY;               // ROR 0x1c
};
static_assert(sizeof(GamStartRec) == 0x20, "GamStartRec");

// m_1a684 says where the PC starts: 0xcccccccc nowhere new; 0xcccccccb the
// kept transforms of the PC and the dog; -n the n-th start of this room;
// else an fchr oid of type 9: 50 units in front of it, facing as it does
// (turned round when the object's param 12 room is this one). Bit 0x4000
// of m_flags marks a start, 0x20 a placed PC.
// ROR 0x001c3d38 // callcheck: memset, dpadd, dpcmp, dpsub, dptofp, fptodp (initializers; the soft-float doubles, C doubles here)

void GamMan::setPcStartPos()
{
    float pos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float rot[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    u_int start = m_1a684;
    m_flags2 &= 0x7fffffff;
    if (start == 0xcccccccc)
        return;
    m_flags |= 0x4000;

    if ((int)start < 0 && start != 0xcccccccb)
    {
        const GamStartRec *rec = (const GamStartRec *)m_wrdFile2;
        int n = 0;
        for (int i = 1; rec->m_wrd != -1; i++)
        {
            if (rec->m_wrd == m_wrdNo && ++n == -(int)start)
            {
                pos[0] = rec->m_pos[0];
                pos[1] = rec->m_pos[1];
                pos[2] = rec->m_pos[2];
                rot[0] = 0.0f;
                rot[1] = rec->m_rotY;
                rot[2] = 0.0f;
                setPcPos(pos);
                setPcRot(rot);
                storePcTransform();
                m_flags |= 0x20;
                return;
            }
            rec++;
            if (i >= 0x200)
                break;
        }
        return;
    }

    if (start == 0xcccccccb)
    {
        rot[1] = m_pcStart[3];
        rot[0] = rot[2] = rot[3] = 0.0f;
        setPcPos(m_pcStart);
        setPcRot(rot);
        rot[1] = m_dogStart[3];
        rot[0] = rot[2] = rot[3] = 0.0f;
        setObjPos((u_int)m_21fd0, m_dogStart);
        setObjRot((u_int)m_21fd0, rot);
    }
    else
    {
        FCHRstat *obj = findFchrType9((int)start);
        if (obj == NULL)
            return;
        obj->getPos(pos);
        obj->getRot(rot);
        float dir = gamWrapAngleOnce(rot[1]);
        int info[4];
        ctlObjParam(start, 0xc, info, 0);
        if (info[1] == m_wrdNo)
            dir = gamWrapAngleOnce((float)((double)dir + 3.141592653589793));
        rot[1] = dir;
        pos[0] += sinf(dir) * 50.0f;
        pos[2] += cosf(rot[1]) * 50.0f;
        setPcPos(pos);
        setPcRot(rot);
        storePcTransform();
    }
    m_flags |= 0x20;
}

// ──────────────────────────────────────────────────────────────────────
// The dog stands 35 units from the PC, at the PC's heading - pi/2, facing
// as the PC does.
// ROR 0x001cd404 // callcheck: dpadd, dpcmp, dpsub, dptofp, fptodp (the soft-float doubles, C doubles here)

void GamMan::storePcTransform()
{
    m_pcPtr->getPos(m_pcPos);
    m_pcPtr->getRot(m_pcRot);
    float rot[4] = { m_pcRot[0], m_pcRot[1], m_pcRot[2], m_pcRot[3] };
    float a = gamWrapAngleOnce((float)((double)rot[1] - 1.5707963267948966));
    float off[4];
    off[0] = sinf(a) * 35.0f;
    off[2] = cosf(a) * 35.0f;
    off[1] = 0.0f;
    off[3] = 0.0f;
    sceVu0AddVector(m_dogPos, m_pcPos, off);
    m_dogPtr->setPos(m_dogPos);
    m_dogPtr->setRot(rot);
    float v[4] = { m_dogPos[0], m_dogPos[1], m_dogPos[2], m_dogPos[3] };
    gamSetBwnPos(v);
    v[0] = rot[0];
    v[1] = rot[1];
    v[2] = rot[2];
    v[3] = rot[3];
    gamSetBwnRot(v);
}

// ──────────────────────────────────────────────────────────────────────
// Image 2's main thread ended (Sequence::reapThreads): the held item again;
// with flags2 0x40 the kept start transforms, the BGM record, and the bits
// of the +0x18bc4 list on their objects.
// ROR 0x001c43f0

void GamMan::onSubSeqEnd()
{
    if (m_equipItem != 0xa00)
        equipItem(m_equipItem);
    if ((m_flags2 & 0x40) != 0)
    {
        m_flags2 &= ~0x40u;
        m_1a684 = 0xcccccccb;
        setPcStartPos();
        m_sndMan.setBgmRec(&m_bgmRec);
        m_sndMan.resumeBgm(1, 0, 0);

        static const u_int s_bits[5] = { 4, 1, 0x40, 0x80, 0x10 };
        const GamObjSet *rec = m_18bc4;
        for (int i = 0; i < 0x180 && rec[i].m_oid != 0; i++)
        {
            u_int oid = rec[i].m_oid;
            for (int b = 0; b < 5; b++)
            {
                if ((rec[i].m_flags & s_bits[b]) != 0)
                    setObj(oid, s_bits[b]);
                else
                    resetObj(oid, s_bits[b]);
            }
        }
    }
    m_flags2 &= ~0x1800u;
}

// ──────────────────────────────────────────────────────────────────────
// Stage change kind 3: the PC / dog transforms parked far away, the kept
// PC and dog set up again (flags2 0x10000000), the room's vfchr models, a
// pending chapter SE reload, then the room's data/<dir>/W<ch>/A<room>/
// A<room>.ARA: its file 0 lists the actor model ids (-1 ends it); from
// the end, file n-4 is the stage-2 script image, n-2 the WRD room data,
// and the actor files run down from n-5. Each actor is loaded hidden and
// kept in the +0x1f880 records.
// ROR 0x001c47c4 // callcheck: FUN_00215a30, BedPfh, FileReadReq, FileReader, ~FileReader (the assert handler inside BedPfh's inlines; the locals' constructors and destructor)

void GamMan::initStage2()
{
    u_int flags2 = m_flags2;
    m_225dc = 0;
    m_dogPos[2] = -100000.0f;
    m_flags2 = flags2 | 0x8001800;
    m_pcPos[0] = -100000.0f;
    m_pcPos[1] = -100000.0f;
    m_pcPos[2] = -100000.0f;
    m_pcPos[3] = 0.0f;
    m_dogPos[0] = -100000.0f;
    m_dogPos[1] = -100000.0f;
    m_dogPos[3] = 0.0f;
    if ((flags2 & 0x10000000) != 0)
        setupPcDogStage2();
    gamCreateEff1a();
    reloadVFchrModels();
    if ((m_flags2 & 0x1000000) != 0)
    {
        reloadChapterSe1f();
        m_flags2 &= ~0x1000000u;
    }
    else if ((m_flags2 & 0x2000000) != 0)
    {
        reloadChapterSe0c();
        m_flags2 &= ~0x2000000u;
    }

    FileReader reader;
    empty1c1a88();
    char path[0x40];
    snprintf(path, 0x40, "data/%s/W%03d/A%03d/A%03d.ARA", seqGetDataDir(), m_chapter, m_wrdNo,
             m_wrdNo);
    alignas(64) u_char head[0x1800];    /* the ROM aligns a stack buffer to 64 */

    FileReadReq req;
    req.m_size = 0x1800;
    req.m_08 = 1;
    req.m_whence = 0;
    req.m_offset = 0;
    req.m_close = 1;
    req.m_name = path;
    req.m_buf = head;
    reader.read(&req, 0);
    BedPfh pfh1(req.m_buf);
    int size = (pfh1.getTotalSize() + 0x7ff) / 2048 * 2048;
    void *buf = g_utlMemory.allocDebug(size, s_file, 2572, 0, 0x101);
    setStagePack(buf);

    FileReadReq req2;
    req2.m_close = 1;
    req2.m_08 = 1;
    req2.m_buf = buf;
    req2.m_whence = 0;
    req2.m_offset = 0;
    req2.m_name = path;
    req2.m_size = size;
    reader.read(&req2, 0);
    BedPfh pfh(req2.m_buf);

    int num = pfh.getNumFile();
    const short *ids = (const short *)pfh.getFile(0);
    u_int count = 0;
    while ((int)count < num && ids[count] != -1)
        count++;
    int base = (int)((u_int)(num - (int)count - 6) & 0xffff) + (int)count;
    // PORT: loadSubImage copies 0x30000 bytes, the ARA's image is 12..31 KB
    // near the end of the pack buffer: the PS2 reads what follows, the host
    // faults past the heap block. The image goes through a zeroed full-size
    // buffer.
    static u_char s_subImage[0x30000];
    int imageSize = pfh.getFileSize(base + 2);
    if (imageSize < 0 || imageSize > (int)sizeof(s_subImage))
        imageSize = sizeof(s_subImage);
    memset(s_subImage, 0, sizeof(s_subImage));
    memcpy(s_subImage, pfh.getFile(base + 2), imageSize);
    g_sequence.loadSubImage(2, s_subImage);
    gamSetWrdRoom(this, pfh.getFile(base + 4));

    int file = base + 1;
    while (count != 0)
    {
        short model = ids[count - 1];
        GamActorRec rec;
        rec.m_actor = g_magSystem.m_scene.loadActor(&g_magSystem.m_i3d, model,
                                                    (void *)pfh.getFile(file), 1);
        rec.m_flags = 1;
        rec.m_model = model;
        rec.m_8 = 0;
        file--;
        g_magSystem.m_scene.m_actors[rec.m_actor].setHide(1);
        count--;
        m_pool1f880.add(rec);
    }
    utlMemFree(buf);
    gamStage2Loaded();
}

// ROR 0x001c387c

void GamMan::reloadVFchrModels()
{
    m_avfchrList.clear();
    for (SeqNode<FCHRstat *> *node = m_vfchrList.m_used.begin(); node != m_vfchrList.m_used.end();
         node = node->m_next)
    {
        FCHRstat *chr = node->m_data;
        u_int oid = chr->m_oid;
        int room;
        chr->getParam(8, &room);
        if (room != m_wrdNo)
            continue;
        int model[2];
        float pos[4];
        chr->getParam(0x1000, model);
        chr->getPos(pos);
        if (model[0] == 0xffff || model[0] == 0)
            continue;
        loadVFchrModel((u_short)model[0], oid, (intptr_t)pos);
        int actor;
        chr->getParam(3, &actor);
        if ((chr->m_flags & 1) != 0 || (chr->m_flags & 0x200000) != 0)
            chr->setHide(1);
    }
}

// ROR 0x001cd954

void GamMan::setupPcDogStage2()
{
    m_flags2 = (m_flags2 & ~0x10000000u) | 0x20000000;
    if ((u_int)(m_21ff4 - 0x2005) > 4)
        m_21ff4 = 0x2000;
    reloadAchrModel((u_short)m_21ff4, 0x50000001);
    if ((u_int)(m_21ff8 - 0x2401) > 3)
        m_21ff8 = 0x2400;
    reloadAchrModel((u_short)m_21ff8, 0x50800002);
    setPc(0x50000001);
    setDog(0x50800002);
    resetObj(0x50000001, 1);
    if ((g_sequence.m_bits[1] & 4) == 0)
        setObj(0x50800002, 1);
    else
        resetObj(0x50800002, 1);
    switch (m_chapter)
    {
    case 1:  setPcAnim(4); break;
    case 2:  setPcAnim(5); break;
    case 3:  setPcAnim(6); break;
    case 4:  setPcAnim(7); break;
    case 5:  setPcAnim(8); break;
    case 6:  setPcAnim(9); break;
    case 7:  setPcAnim(6); break;
    case 8:  setPcAnim(10); break;
    case 10: setPcAnim(0xb); break;
    case 11: setPcAnim(0xc); break;
    case 12: setPcAnim(0xd); break;
    default: break;
    }
    g_sequence.m_global[0x51] = m_21ff4;
    g_sequence.m_global[0x52] = m_21ff8;
}

// ROR 0x001caf78

int GamMan::reloadAchrModel(u_short model, u_int oid)
{
    u_int param = model;
    ACHRstat *chr = getAchr(oid);
    chr->releaseActor();
    int actor = g_magSystem.loadModel(model, -1);
    setObjLight(actor, oid);
    chr->setParam(3, &actor);
    chr->setParam(0x1000, &param);
    return actor;
}

// ════════════════════════════════════════════════════════════════════════
//  The stage resets, the vfchr models, the camera stack (session 6)
// ════════════════════════════════════════════════════════════════════════

// ──────────────────────────────────────────────────────────────────────
// sysTermManagers' call.
// ROR 0x001c0d44

void GamMan::term()
{
    releaseAll();
    init();
}

// ──────────────────────────────────────────────────────────────────────
// Native 212 case 50. An enemy turned off hides; turned on again, the
// object its param 0x67 names is reset (shown) first.
// The actor's virtual +0x44 is DogModel::setHide.
// ROR 0x001c5a5c

void GamMan::switchEnemies(int on)
{
    for (SeqNode<ACHRstat *> *node = m_achrList.m_used.begin(); node != m_achrList.m_used.end();
         node = node->m_next)
    {
        ACHRstat *chr = node->m_data;
        if (gamOidType(chr->m_oid) != 2)
            continue;
        int actor = chr->m_actor;
        if (on == 0)
        {
            if ((chr->m_attr[2] & 1) == 0)
                continue;
            int param[4];
            chr->getParam(0x67, param);
            if (param[0] != 0)
                resetObj((u_int)param[0], 1);
            if (actor >= 0)     // PORT: the ROM writes actor -1's words
                g_magSystem.m_scene.m_actors[actor].setHide(0);
            chr->m_attr[2] &= ~1u;
        }
        else
        {
            if (actor >= 0)     // PORT: as above
                g_magSystem.m_scene.m_actors[actor].setHide(1);
            chr->m_attr[2] |= 1;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// Native 016 (and the map menu): at most three deep; modes 6.. are not
// kept. A map camera (3..5) keeps its path point too.
// ROR 0x001c5ffc

void GamMan::pushCamera()
{
    if (m_camDepth >= 3)
        return;
    int mode[2];
    g_magSystem.getCameraMode(mode);
    mode[0] &= 0xf;
    if ((u_int)mode[0] >= 6)
        return;
    GamCamRec &rec = m_camStack[m_camDepth];
    rec.m_mode = mode[0];
    rec.m_arg = mode[1];
    rec.m_21f78 = m_21f78;
    MagScene &scene = g_magSystem.m_scene;
    if ((u_int)(mode[0] - 3) < 3)
        rec.m_pathPos = scene.getCameraPathPos();
    DogCamera &cam = scene.m_camera_entity;
    rec.m_trans[0] = cam.m_trans[0];
    rec.m_trans[1] = cam.m_trans[1];
    rec.m_trans[2] = cam.m_trans[2];
    rec.m_rotate[0] = cam.m_rotate[0];
    rec.m_rotate[1] = cam.m_rotate[1];
    rec.m_rotate[2] = cam.m_rotate[2];
    rec.m_angle = cam.m_angle;
    m_camDepth++;
}

// ──────────────────────────────────────────────────────────────────────
// The last pushed camera back: modes 0..2 directly, 3..5 as a map camera
// (number m_21f78, the kept word third) at its path point, else mode 1;
// then the entity's translation, rotation (normalised) and angle.
// ROR 0x001c621c

void GamMan::popCamera()
{
    if (m_camDepth == 0)
        return;
    m_camDepth--;
    GamCamRec &rec = m_camStack[m_camDepth];
    MagScene &scene = g_magSystem.m_scene;
    int mode = rec.m_mode;
    if (mode >= 0 && mode < 3)
    {
        g_magSystem.setCameraMode(mode, rec.m_arg, NULL);
    }
    else if (mode >= 0 && mode < 6)
    {
        g_gamMan.getMapCamNum();    // its answer is not read
        int param[3] = { -1, -1, rec.m_arg };
        g_magSystem.setCameraMode(rec.m_mode, (int)rec.m_21f78, param);
        scene.setCameraPathPos(rec.m_pathPos);
    }
    else
    {
        g_magSystem.setCameraMode(1, 0, NULL);
    }
    DogCamera &cam = scene.m_camera_entity;
    cam.m_trans[0] = rec.m_trans[0];
    cam.m_trans[1] = rec.m_trans[1];
    cam.m_trans[2] = rec.m_trans[2];
    cam.m_dirty = 1;
    cam.m_rotate[0] = rec.m_rotate[0];
    cam.m_rotate[1] = rec.m_rotate[1];
    cam.m_rotate[2] = rec.m_rotate[2];
    cam.normalizeRotate();
    cam.m_angle = rec.m_angle;
    cam.m_dirty = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001c7dec

void GamMan::clearOidListImages()
{
    u_int *word = (u_int *)m_1f240;
    for (int i = 0; i < 0x169; i++)
    {
        if ((word[i] >> 30) != 0)
            word[i] = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Native 100's vfchr models (sequence/seqcall.cpp) and reloadVFchrModels.
// ROR 0x001caa14

void GamMan::loadVFchrModel(int model, u_int oid, intptr_t pos)
{
    model &= 0xffff;
    FCHRstat *chr = getVFchr(oid);
    int actor = g_magSystem.loadModel(model, -1);
    setObjLight(actor, oid);
    u_int param = (u_int)model;
    chr->setParam(3, &actor);
    chr->setParam(0x1000, &param);
    chr->setPos((const float *)pos);
    g_magSystem.m_scene.changeActorMotion(0.0166666936f, actor, 1, 0, 0);
    if (isFchrActive(oid))
        ctlObj1cb8e4(oid, 0x32, 0);
    if (model == 0x800)
        gamShrinkActor(actor);
    m_avfchrList.add(chr);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cabd4

void GamMan::loadVFchrModelByName(int model, const char *name, u_int oid, intptr_t pos)
{
    FCHRstat *chr = getVFchr(oid);
    int actor = g_magSystem.m_scene.loadModelByName(&g_magSystem.m_i3d, name, -1,
                                                    getChrRpk((u_int)(model & 0xffff) >> 10));
    setObjLight(actor, oid);
    u_int param = (u_int)(model & 0xffff);
    chr->setParam(0x1000, &param);
    chr->setParam(0x1004, name);
    chr->setParam(3, &actor);
    chr->setPos((const float *)pos);
    g_magSystem.m_scene.changeActorMotion(0.0166666936f, actor, 1, 0, 0);
    if (isFchrActive(oid))
        ctlObj1cb8e4(oid, 0x32, 0);
    m_avfchrList.add(chr);
}

// ──────────────────────────────────────────────────────────────────────
// Native 100 cases 44 / 45 (keepPos 1). A live object only; nothing
// changes when the model (or file name) is the one it has.
// ROR 0x001cad88 // callcheck: getAchr, getVFchr, getFchr (through the inline find)

u_int GamMan::changeObjModel(u_int model, const char *name, u_int oid, int keepPos)
{
    model &= 0xffff;
    if (!isObj(oid))
        return 0;
    float pos[4];
    if (keepPos != 0)
        getObjPos(oid, pos);
    ObjStat *chr = find(oid);
    if (chr == NULL)
        return oid;     // PORT: the ROM goes on with NULL (isObj passed: not reached)
    int actor;
    if (name == NULL)
    {
        u_int cur[4];
        chr->getParam(0x1000, cur);
        if (model == cur[0])
            return 0;
        chr->releaseActor();
        actor = g_magSystem.loadModel((int)model, -1);
    }
    else
    {
        char cur[0x10];
        chr->getParam(0x1004, cur);
        if (strcmp(name, cur) == 0)
            return 0;
        chr->releaseActor();
        actor = g_magSystem.m_scene.loadModelByName(&g_magSystem.m_i3d, name, -1,
                                                    getChrRpk(model >> 10));
        chr->setParam(0x1004, name);
    }
    setObjLight(actor, oid);
    chr->setParam(3, &actor);
    chr->setParam(0x1000, &model);
    if (keepPos != 0)
        setObjPos(oid, pos);
    return oid;
}

// ──────────────────────────────────────────────────────────────────────
// Native 100 case 8. The group's achrs are built as addChr builds one,
// but with oid 0: EmyManager::addEmy makes their oids (0x9100006f + n)
// and fills their m_achr slots.
// ROR 0x001cc560 // callcheck: operator_new, ObjStat (new ACHRstat: its constructor is inline over ObjStat::ObjStat)

void GamMan::addChrGroup(int model, u_int oid, int a, int b, int c, int count, int d,
                         int e, int kind, const void *pos, int place, float f)
{
    u_short id = (u_short)kind;
    u_short chrNo[8] = {0};     // the ROM's stack words
    m_sysData.getField(id, 0x72, chrNo);
    g_emyManager.setGroup(f, a, b, c, count, d, pos);
    for (int i = count; i > 0; i--)
    {
        u_short chrModel[8] = {0};
        chrModel[0] = (u_short)model;   // overwritten by the field
        m_sysData.getField(id, 0xf, chrModel);
        int actor = createObjByModel(chrModel[0], 0, id, pos, place, 0);
        ACHRstat *chr = new ACHRstat(0, actor, (short)id);
        chr->init((short)id);
        chr->initParamFuncs();
        u_int newOid = g_emyManager.addEmy(chrNo[0], (short)id, chr, m_seqWord, m_image[place],
                                           pos, 0, a);
        if (pos != NULL)
            setObjPos(newOid, (const float *)pos);
        m_achrList.add(chr);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The ROM's release test of releaseObjs' loops: kind 2 the objects not of
// image 1 (the stage-2 ones), kind 3 those of image 3, else every one.
static int gamReleaseKindMatch(int kind, u_int oid)
{
    if (kind == 2)
        return (oid >> 30) != 1;
    if (kind == 3)
        return (oid >> 30) == 3;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Kinds 0 / 1 (task/tsk_bg_load.cpp) and 2 (releaseStage2). Kind 2 sets
// the kept PC and dog again first (flags2 0x10000000) and frees the PC
// actor's motion 3 held for flags2 0x200000; the script callbacks of
// images 2 / 3 go. The achr loops do not clear m_pcPtr (the PC is image
// 1, kept by kind 2).
// ROR 0x001cdf5c // callcheck: FUN_00105ba4 (the objects' deleting destructor, vtable slot 1 with 3 in the ROM: delete here)

void GamMan::releaseObjs(int kind, int keepFchrs)
{
    if (kind >= 0 && kind < 2)
    {
        g_taskMan.killStageTasks();
        g_emyManager.resetWork();
        m_seqPair[1].m_a = 0;
        m_seqPair[1].m_b = 0;
        m_seqPair[2].m_a = 0;
        m_seqPair[2].m_b = 0;
    }
    else if (kind == 2)
    {
        g_taskMan.killStageTasks();
        if ((m_flags2 & 0x10000000) != 0)
        {
            setPc(0x50000001);
            setDog(0x50800002);
        }
        if ((m_flags2 & 0x200000) != 0 && m_pcActor != -1)
        {
            g_magSystem.setPC(m_pcActor, m_pcJnfc);
            g_magSystem.m_scene.m_actors[m_pcActor].releaseMotion(3);
            g_magSystem.setPC(m_pcActor, m_pcJnfc);
            g_magSystem.setPC(-1, 0);
            m_flags2 &= ~0x200000u;
        }
        g_emyManager.resetWork();
        for (int i = 0; i < 3; i++)
        {
            if (((u_int)m_seqPair[i].m_b >> 30) > 1)
            {
                m_seqPair[i].m_a = 0;
                m_seqPair[i].m_b = 0;
            }
        }
    }

    clearChrSectaPtrs();
    m_pool2246c->clear();
    g_sequence.stopImages(kind);
    m_area.stopTrgs();
    m_area.clear();
    clearPool20b00(kind);
    m_sndMan.stopObjSnds();
    m_camDepth = 0;
    m_225f4[0] = 0x7f;
    m_225f4[1] = 0x6f;
    m_225f4[2] = 0x27f;
    m_225f4[3] = 0x2ff;

    SeqNode<ACHRstat *> *node;
    for (node = m_achrList.m_used.begin(); node != m_achrList.m_used.end(); node = node->m_next)
    {
        if (gamReleaseKindMatch(kind, node->m_data->m_oid))
            node->m_data->clearSecta(1);
    }
    node = m_achrList.m_used.begin();
    while (node != m_achrList.m_used.end())
    {
        SeqNode<ACHRstat *> *next = node->m_next;
        ACHRstat *chr = node->m_data;
        u_int oid = chr->m_oid;
        if (gamReleaseKindMatch(kind, oid))
        {
            if (gamOidType(oid) == 2)
                g_emyManager.releaseObj(oid);
            chr->releaseActor();
            m_achr[gamOidSlot(oid)] = NULL;
            delete chr;
            m_achrList.remove(node);
        }
        node = next;
    }
    if (keepFchrs != 0)
        return;

    SeqNode<FCHRstat *> *fnode = m_fchrList.m_used.begin();
    while (fnode != m_fchrList.m_used.end())
    {
        SeqNode<FCHRstat *> *next = fnode->m_next;
        FCHRstat *chr = fnode->m_data;
        u_int oid = chr->m_oid;
        if (gamReleaseKindMatch(kind, oid))
        {
            if (oid == (u_int)m_pcEquipItemOID)
            {
                g_magSystem.setActorParent(m_pcEquipItemChrID, -1, 0);
                m_pcEquipItemOID = 0;
                m_pcEquipItemChrID = -1;
            }
            chr->releaseActor();
            m_fchr[gamOidSlot(chr->m_oid)] = NULL;
            delete chr;
            m_fchrList.remove(fnode);
        }
        fnode = next;
    }

    for (fnode = m_avfchrList.m_used.begin(); fnode != m_avfchrList.m_used.end();
         fnode = fnode->m_next)
    {
        if (kind >= 0 && kind <= 2)
            fnode->m_data->releaseActor();
    }
    m_avfchrList.clear();

    fnode = m_vfchrList.m_used.begin();
    while (fnode != m_vfchrList.m_used.end())
    {
        SeqNode<FCHRstat *> *next = fnode->m_next;
        FCHRstat *chr = fnode->m_data;
        int room[4];
        chr->getParam(8, room);
        if (kind == 2)
        {
            if (room[0] == m_21f84)
                chr->releaseActor();
        }
        else if (kind >= 0 && kind < 2)
        {
            chr->releaseActor();
            // PORT: the ROM deletes the object (it lives in the m_1f248
            // storage) and frees the node with the fchr pool's inline:
            // the node lands in m_fchrList's free list, its counts go
            // wrong. Here the object is destroyed in place and the node
            // goes back to the vfchr pool; its m_vfchrSlot stays taken,
            // as in the ROM.
            chr->~FCHRstat();
            m_vfchrList.remove(fnode);
        }
        fnode = next;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Stage change kind 5 (afterSeqExec) and 0x1cb638: the room unset, the
// menus, camera, PC and dog dropped, the object records' 0x8000 bits
// cleared; unless script bit [7] 0x80000000, the held item (or, with
// [2] 0x8000, only its chr no) and the image oids go and [6] loses bits
// 0..3. Then sounds, callbacks, messages, effects, images, objects and
// area triggers.
// ROR 0x001cdbf0 // callcheck: rorDebugPrintf (the empty debug print of m_keepBuf, left out)

void GamMan::resetStage()
{
    m_wrdNo = -1;
    g_magSystem.m_scene.closeMenu();
    m_pool2246c->clear();
    g_gamMan.setCameraMode(1, 0);
    m_camDepth = 0;
    g_magSystem.clearPC();
    setPc(-1);
    setDog(-1);
    for (int i = 0; i < m_objTable->m_num; i++)
        m_objRec[i].m_flags &= ~0x8000u;
    if ((int)g_sequence.m_bits[7] >= 0)
    {
        if ((g_sequence.m_bits[2] & 0x8000) == 0)
        {
            equipItem(0xa00);
            m_findTargetItem = 0xa00;
        }
        else
        {
            m_pcEquipItemChrID = -1;
        }
        clearOidListImages();
        g_sequence.m_bits[6] &= ~1u;
        g_sequence.m_bits[6] &= ~2u;
        g_sequence.m_bits[6] &= ~4u;
        g_sequence.m_bits[6] &= ~8u;
    }
    g_sndMan.stopAll(1);
    g_sndMan.stopAll(8);
    m_sndMan.reset();
    m_seqPair[1].m_b = 0;
    m_seqPair[1].m_a = 0;
    m_seqPair[0].m_b = 0;
    m_seqPair[0].m_a = 0;
    m_seqPair[2].m_b = 0;
    m_seqPair[2].m_a = 0;
    int param[8] = {0};     // only [4] (the fade time) is set in the ROM
    killMsgTask();
    showMsgSprite(0, param);
    clearPool20b00(0);
    g_sequence.stopImages(1);
    releaseAllChrs();
    m_area.stopTrgs();
    m_area.clear();
    m_225f4[0] = 0x7f;
    m_225f4[3] = 0x2ff;
    m_225f4[1] = 0x6f;
    m_225f4[2] = 0x27f;
}

// ──────────────────────────────────────────────────────────────────────
// Stage 2 started with m_flags 0x20: the PC goes to the position kept in
// m_sysWord, shifted by the difference between the old room offset
// (globals [5] / [7]) and this room's, 50 units along global [10] (a
// float's bits); the dog beside it. The globals then hold this room's
// offset.
// ROR 0x001cd60c

void GamMan::startStage2()
{
    int *global = g_sequence.m_global;
    if ((m_flags & 0x20) != 0)
    {
        m_flags |= 0x4000;
        float pos[4];
        pos[0] = (float)global[5];
        pos[2] = (float)global[7];
        pos[1] = 0.0f;
        pos[3] = 0.0f;
        float dir;
        memcpy(&dir, &global[10], sizeof(dir));
        float off[4];
        off[0] = sinf(dir) * 50.0f;
        off[1] = 0.0f;
        off[2] = cosf(dir) * 50.0f;
        off[3] = 0.0f;
        off[0] = off[0] + (gamGetWrdOfsX() - pos[0]);
        off[2] = off[2] + (gamGetWrdOfsZ() - pos[2]);
        memcpy(m_pcPos, m_sysWord, sizeof(m_pcPos));
        sceVu0AddVector(pos, m_pcPos, off);
        m_pcPtr->setPos(pos);
        if (m_dogPtr != NULL)
            storePcTransform();
    }
    global[5] = (int)gamGetWrdOfsX();
    global[7] = (int)gamGetWrdOfsZ();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cea44

void GamMan::releaseVFchrs(int keep)
{
    SeqNode<FCHRstat *> *node = m_vfchrList.m_used.begin();
    while (node != m_vfchrList.m_used.end())
    {
        SeqNode<FCHRstat *> *next = node->m_next;
        FCHRstat *chr = node->m_data;
        int actor = chr->m_actor;
        u_int oid = chr->m_oid;
        int room[4];
        chr->getParam(8, room);     // not read
        if (actor != -1)
            chr->releaseActor();
        if (keep == 0)
        {
            clearVFchrSlot(oid);
            m_vfchrList.remove(node);
        }
        node = next;
    }
}

// ──────────────────────────────────────────────────────────────────────
// releaseAll's work (also 0x1cb638 and resetStage). The PC and dog lose
// their words when their achr goes (the PC actor's motions 1 and 3 freed
// with it set as the PC once more).
// ROR 0x001ced18 // callcheck: FUN_00105ba4 (the objects' deleting destructor, vtable slot 1 with 3 in the ROM: delete here)

void GamMan::releaseAllChrs()
{
    markReset();
    int keepPcDog = (g_sequence.m_bits[2] & 0x8000) != 0;
    SeqNode<ACHRstat *> *node = m_achrList.m_used.begin();
    while (node != m_achrList.m_used.end())
    {
        SeqNode<ACHRstat *> *next = node->m_next;
        u_int oid = node->m_data->m_oid;
        u_int pcOid = m_pcPtr != NULL ? m_pcPtr->m_oid : 0;
        u_int dogOid = m_dogPtr != NULL ? m_dogPtr->m_oid : 0;
        if (keepPcDog)
        {
            if (oid == pcOid || oid == dogOid)
            {
                node = next;
                continue;
            }
        }
        else
        {
            if (oid == pcOid)
            {
                g_magSystem.setPC(m_pcActor, m_pcJnfc);
                DogActor &actor = g_magSystem.m_scene.m_actors[m_pcActor];
                actor.releaseMotion(1);
                actor.releaseMotion(3);
                g_magSystem.setPC(-1, 0);
                m_pcPtr = NULL;
                m_pcActorPtr = NULL;
                m_pcMotion = -1;
                m_pcActor = -1;
            }
            if (oid == (m_dogPtr != NULL ? m_dogPtr->m_oid : 0))
            {
                m_dogPtr = NULL;
                m_dogChrNo = -1;
            }
        }
        ACHRstat *chr = node->m_data;
        chr->clearSecta(1);
        u_int id = chr->m_oid;
        if (gamOidType(id) == 2)
            g_emyManager.releaseObj(id);
        chr->releaseActor();
        m_achr[gamOidSlot(id)] = NULL;
        delete chr;
        m_achrList.remove(node);
        node = next;
    }

    SeqNode<FCHRstat *> *fnode = m_fchrList.m_used.begin();
    while (fnode != m_fchrList.m_used.end())
    {
        SeqNode<FCHRstat *> *next = fnode->m_next;
        FCHRstat *chr = fnode->m_data;
        chr->releaseActor();
        m_fchr[gamOidSlot(chr->m_oid)] = NULL;
        delete chr;
        m_fchrList.remove(fnode);
        fnode = next;
    }

    for (fnode = m_avfchrList.m_used.begin(); fnode != m_avfchrList.m_used.end();
         fnode = fnode->m_next)
        fnode->m_data->releaseActor();
    m_avfchrList.clear();

    if ((int)g_sequence.m_bits[7] >= 0)
        releaseVFchrs(0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001cebf8

void GamMan::clearChrSectaPtrs()
{
    for (SeqNode<ACHRstat *> *node = m_achrList.m_used.begin(); node != m_achrList.m_used.end();
         node = node->m_next)
        node->m_data->clearSectaPtrs();
    for (SeqNode<FCHRstat *> *node = m_fchrList.m_used.begin(); node != m_fchrList.m_used.end();
         node = node->m_next)
        node->m_data->clearSectaPtrs();
}

// ──────────────────────────────────────────────────────────────────────
// Native 006 case 3. The four ints are never read.
// ROR 0x001cfa5c

void GamMan::openSysMsg(int msg, int a, int b, int c, int d)
{
    m_sysMsgHandle = g_magSystem.m_scene.openSysMsgWindow(msg, NULL, 0);
}

// ──────────────────────────────────────────────────────────────────────
// Native 152. Achr objects only.
// ROR 0x001d17c8

int GamMan::testAchrFlag(u_int oid, u_int bits)
{
    if (gamOidList(oid) != 1)
        return 0;
    ACHRstat *chr = getAchr(oid);
    return (chr->m_attr[bits >> 24] & bits & 0xffffff) != 0;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of ROR 0x001d091c
void GamMan::addObjColliBox(u_int oid, const float *box)
{
    ROSEPAN_STUB("GamMan::addObjColliBox", 0x001d091c);
}

// stub of ROR 0x001d0c64
void GamMan::addObjColliQuad(u_int oid, const float *quad)
{
    ROSEPAN_STUB("GamMan::addObjColliQuad", 0x001d0c64);
}

// stub of ROR 0x001d080c
void GamMan::addObjColliSegment(u_int oid, const float *seg)
{
    ROSEPAN_STUB("GamMan::addObjColliSegment", 0x001d080c);
}

// stub of ROR 0x001d1f74
void GamMan::addObjParam(u_int oid, int which, const void *data)
{
    ROSEPAN_STUB("GamMan::addObjParam", 0x001d1f74);
}

// stub of ROR 0x001d2334
void GamMan::addObjWork(u_int oid, int no, int value)
{
    ROSEPAN_STUB("GamMan::addObjWork", 0x001d2334);
}

// stub of ROR 0x001d18c4
void GamMan::clearObjAttr(u_int oid, u_int bits)
{
    ROSEPAN_STUB("GamMan::clearObjAttr", 0x001d18c4);
}

// stub of ROR 0x001cfda4
void GamMan::placeObjAt(u_int oid, int where)
{
    ROSEPAN_STUB("GamMan::placeObjAt", 0x001cfda4);
}

// stub of ROR 0x001d2428
void GamMan::setActorBindMode(u_int oid, int mode)
{
    ROSEPAN_STUB("GamMan::setActorBindMode", 0x001d2428);
}

// stub of ROR 0x001d1838
void GamMan::setObjAttr(u_int oid, u_int bits)
{
    ROSEPAN_STUB("GamMan::setObjAttr", 0x001d1838);
}

// stub of ROR 0x001d1e88
void GamMan::setObjScale(u_int oid, float scale)
{
    ROSEPAN_STUB("GamMan::setObjScale", 0x001d1e88);
}

// stub of ROR 0x001d2270
void GamMan::setObjWork(u_int oid, int no, int value)
{
    ROSEPAN_STUB("GamMan::setObjWork", 0x001d2270);
}

// stub of ROR 0x001d1a5c
void GamMan::attachObj(u_int oid, u_int parent, const char *node)
{
    ROSEPAN_STUB("GamMan::attachObj", 0x001d1a5c);
}

// stub of ROR 0x001d1bec
void GamMan::detachObj(u_int oid, u_int parent)
{
    ROSEPAN_STUB("GamMan::detachObj", 0x001d1bec);
}

// stub of ROR 0x001d217c
int GamMan::getObjWork(u_int oid, int no)
{
    ROSEPAN_STUB("GamMan::getObjWork", 0x001d217c);
    return {};
}

