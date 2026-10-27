/* ==========================================================================
 *  mag/magscene2.cpp
 *
 *  The scene's loaders (RoR's "mag/magscene2.cpp", 0x137300..0x139540):
 *  the system data at start (options.txt, the font and the common
 *  textures of PREMIERE.RPK), map units into the four sky slots (a model,
 *  a second model with the STQ map, a collision model, mapparam.bbb's
 *  view / fog values, the _c.bin records, the _l.bin light, sector.sec),
 *  characters into the 128 actors (shared when an actor already holds the
 *  same id or name), motion packs and single textures.
 * ======================================================================== */
#include "mag/magscene.h"

#include <stdint.h>

#include "bed/bedpfh.h"
#include "sdk/libgraph.h"
#include "utl/utlfile.h"
#include "utl/utlmisc.h"

static const char s_file[] = "mag/magscene2.cpp";

/* data 0x298b80: the font file in PREMIERE.RPK (a pointer variable; only
 * loadSystemData reads it). */
static const char *s_font_file = "fnt/fnt16US.bin";

#define SIZE_HEADER 0x2000      /* mapunit.bin's header read (the assert's name) */

/* Rounds a file size up to whole 2048-byte sectors (the ROM's signed
 * (x + 0x7ff) >> 11 << 11). */
static inline int roundSector(int size)
{
    return (size + 0x7ff) / 0x800 * 0x800;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00137300

void MagScene::initCharaLoader()
{
    m_chara_loader.init();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00137318

void MagScene::initMapLoader()
{
    m_map_loader.init();
}

// ──────────────────────────────────────────────────────────────────────
// MagScene::start's first step: the options file when there is one, the
// font file and the common textures (file 0) of PREMIERE.RPK, the
// texture numbers of r_shadow / noise and the four bit8 font pages, then
// the map and character lists. Returns 0.
// callcheck: BedPfh, FileReadReq, FileReader, ~FileReader, FUN_00215a30 (constructors and destructors of locals; the assert handler inside ROR_ASSERT)
// ROR 0x00137330

int MagScene::loadSystemData()
{
    static const char *common_tex[2] = { "r_shadow", "noise" };
    /* bit8e is in the ROM's table too, but only four pages are taken. */
    static const char *font_tex[5] = { "bit8a", "bit8b", "bit8c", "bit8d", "bit8e" };

    FileReader reader;
    alignas(64) u_char head[0x800];     /* the ROM aligns a 0x840-byte stack buffer to 64 */

    memset(head, 0, sizeof(head));
    if (fileExists("data/options.txt"))
    {
        FileReadReq req;
        req.m_name = "data/options.txt";
        req.m_size = 0x800;
        req.m_close = 1;
        req.m_08 = 1;
        req.m_buf = head;
        if (reader.read(&req, 0) >= 0)
            parseOption((char *)head);
    }

    FileReadReq req;
    req.m_buf = head;
    req.m_size = 0x800;
    req.m_name = "data/PREMIERE.RPK";
    reader.read(&req, 0);

    BedPfh pfh_head(head);
    ROR_ASSERT(pfh_head.getFileOffset(0) == (2048)); /* 283 */

    int filePackIdx = pfh_head.getFileIndex(s_font_file);
    ROR_ASSERT(filePackIdx != -1); /* 302 */
    int filelen = pfh_head.getFileSize(filePackIdx);
    ROR_ASSERT(filelen > 0); /* 312 */
    int size = roundSector(filelen);
    m_font_data = g_utlMemory.allocDebug(size, s_file, 315, 0, 1);
    ROR_ASSERT(m_font_data != NULL); /* 316 */
    req.m_offset = pfh_head.getFileOffset(filePackIdx);
    req.m_buf = m_font_data;
    req.m_whence = 0;
    req.m_size = size;
    reader.read(&req, 0);

    filelen = pfh_head.getFileSize(0);
    ROR_ASSERT(filelen > 0); /* 330 */
    size = roundSector(filelen);
    m_premiere = g_utlMemory.allocDebug(size, s_file, 333, 0, 1);
    ROR_ASSERT(m_font_data != NULL); /* 334: the ROM tests m_font_data again */
    req.m_offset = pfh_head.getFileOffset(0);
    req.m_buf = m_premiere;
    req.m_close = 1;
    req.m_whence = 0;
    req.m_size = size;
    reader.read(&req, 0);

    BedPfh pfh(m_premiere);
    m_tex_info.addPfh(&pfh, 0, 0, 0);
    for (int i = 0; i < 2; i++)
    {
        m_common_tex_no[i] = m_tex_info.find(common_tex[i]);
        ROR_ASSERT(m_common_tex_no[i] >= 0); /* 363 */
        m_tex_info.setTexFlag(m_common_tex_no[i], 1);
    }
    for (int i = 0; i < 4; i++)
    {
        m_font_tex_no[i] = m_tex_info.find(font_tex[i]);
        ROR_ASSERT(m_font_tex_no[i] >= 0); /* 381 */
        m_tex_info.setTexFlag(m_font_tex_no[i], 1);
    }

    initMapLoader();
    initCharaLoader();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Takes a map unit pack into the first free sky slot (slot 4 when all
// are taken: the ROM does not check): copies it to the game heap when
// copy is set, loads the pack's model (file 0, named <name>.xxx), its
// second model with the STQ map (file 2), mapparam.bbb (view far, fog,
// three scene words, a 0..1 factor), the collision model, the _c.bin
// records and the _l.bin light, the sky textures (with a context) and
// sector.sec. Returns the slot, -5 / -7 when a model does not load.
// callcheck: dbgTraceVector4 x3 (traces of the clip spheres; the retail build prints nothing)
// callcheck: i3rCopyClipSphere, i3rReplaceStqMapEntity, FUN_00215a30 (inside the inline DogModel::copyClipSphere / replaceStqMapEntity; the assert handler)
// ROR 0x001377f0

int MagScene::load(i3dContext *ctx, int id, void *data, int copy)
{
    int n = 0;
    if (m_tenkyu[0].m_id >= 0)
    {
        for (n = 1; n < 4 && m_tenkyu[n].m_id >= 0; n++)
            ;
    }
    MagSceneTenkyu *t = &m_tenkyu[n];

    BedPfh pfh(data);
    void *p = data;
    if (copy)
    {
        int size = pfh.getTotalSize();
        p = g_utlMemory.allocDebug(size, s_file, 424, 0, 1);
        ROR_ASSERT(p); /* 425 */
        memcpy(p, data, size);
    }

    BedPfh pfh_head(p);
    void *model_file = pfh_head.getFile(0);
    BedPfh pfh_model(model_file);

    char name[0x100];
    utlStrCopy(name, 0x100, pfh_head.getFileName(0));
    for (int i = 0; i < 0x100 && name[i] != '\0'; i++)
    {
        if (name[i] == '.')
        {
            name[i] = '\0';
            break;
        }
    }

    float center[4];
    float radius;
    if (t->m_model[0].loadPack(&m_tex_info, BedPfh(model_file), m_22cc, 0) != 0)
    {
        ROR_ASSERT(0); /* 456 */
        return -5;
    }
    t->m_model[0].setupNodeFlags();
    t->m_model[0].copyClipSphere(center, &radius);
    dbgTraceVector4(center);

    if (pfh_model.getNumFile() > 2)
    {
        if (t->m_model[1].loadData(&m_tex_info, pfh_model.getFile(2), NULL) != 0)
        {
            ROR_ASSERT(0); /* 497 */
            return -7;
        }
        int r = t->m_model[1].replaceStqMapEntity(&m_stq_map);
        ROR_ASSERT(r == I3D_OK); /* 505 */
    }
    // PC: the ROM reads the clip sphere of the second model even when the
    // pack has none (a NULL root reads low memory on the PS2).
    if (t->m_model[1].m_i3r_root)
        t->m_model[1].copyClipSphere(center, &radius);
    dbgTraceVector4(center);
    t->m_id = id;

    char file[0x100];
    utlStrCopy2(file, 0x100, name, "_c.bin");
    int idx = pfh_head.getFileIndex(file);
    if (idx >= 0)
    {
        u_int *c = (u_int *)pfh_head.getFile(idx);
        u_int head = c[0];
        t->m_c_data = c;
        t->m_c_num = head & 0xffff;
        // PC: the ROM writes the record pointers over the offsets after the
        // head word (4-byte pointers); the host keeps them in a game-heap
        // array, freed by releaseTenkyu.
        t->m_path_data = (MagSceneCRec **)g_utlMemory.alloc(t->m_c_num * (int)sizeof(MagSceneCRec *) + 1, 1);
        for (int i = 0; i < t->m_c_num; i++)
        {
            MagSceneCRec *rec = (MagSceneCRec *)((u_char *)c + c[1 + i]);
            t->m_path_data[i] = rec;
            rec->m_2c = 0;
            rec->m_02 = (u_char)(head >> 16);
            rec->m_03 = 0;
            rec->m_20 = rec->m_00 >> 1;
            rec->m_24 = 0;
            rec->m_28 = 0;
        }
    }

    m_2a9cc = 1.0f;
    idx = pfh_head.getFileIndex("mapparam.bbb");
    if (idx >= 0)
    {
        int *mp = (int *)pfh_head.getFile(idx);
        if ((u_int)(mp[0] - 1) < 0xffff)
        {
            setViewFar((float)mp[0]);
            m_fog[1].m_color[0] = (float)mp[1] / 255.0f;
            m_fog[1].m_color[1] = (float)mp[2] / 255.0f;
            m_fog[1].m_color[3] = 0.0f;
            m_fog[1].m_color[2] = (float)mp[3] / 255.0f;
            m_fog[1].m_param[0] = (float)mp[4];
            m_fog[1].m_param[1] = (float)mp[5] / 100.0f;
            m_fog[1].m_param[2] = (float)mp[6];
            m_fog[1].m_param[3] = (float)mp[7] / 100.0f;
            setFog(&m_fog[1], 0);
            m_2aa84 = 1;
            if (pfh_head.getFileSize(idx) > 0x20)
            {
                m_854a4 = mp[8];
                m_mirror_axis = mp[9];
                m_mirror_plane = ((const float *)mp)[10];   /* lwc1 / swc1: the word is a float */
            }
            float f = (float)mp[11] / 100.0f;
            m_2a9cc = f;
            if (f == 0.0f)
                m_2a9cc = 1.0f;
            else if (f < 0.0f)
                m_2a9cc = 0.0f;
            else if (f > 1.0f)
                m_2a9cc = 1.0f;
        }
    }

    utlStrCopy2(file, 0x100, name, "_col.i3s.i3r");
    idx = pfh_head.getFileIndex(file);
    if (idx >= 0)
        t->m_model[2].loadData(&m_tex_info, pfh_head.getFile(idx), pfh_head.getFile(idx + 1));

    utlStrCopy2(file, 0x100, name, "_l.bin");
    idx = pfh_head.getFileIndex(file);
    if (idx < 0 || pfh_head.getFileSize(idx) < 1)
        t->m_light_data = NULL;
    else
        t->m_light_data = pfh_head.getFile(idx);

    if (ctx)
    {
        m_tex_info.setTexTable(ctx);
        loadTenkyuTex(n, &pfh, ctx);
    }

    idx = pfh.getFileIndex("sector.sec");
    if (idx >= 0 && pfh.getFileSize(idx) > 0)
    {
        int i = FUN_0017c1f4(&m_8542c, pfh.getFile(idx), copy != 0);
        ROR_ASSERT(i != 0); /* 651 */
    }

    // PC: as above for the collision model.
    if (t->m_model[2].m_i3r_root)
        t->m_model[2].copyClipSphere(center, &radius);
    dbgTraceVector4(center);

    t->m_buf = copy ? p : NULL;
    if (t->m_light_data)
        loadLight(t->m_light_data);
    setDefaultLight();
    resetEntities();
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// Reads map unit theNo of data/bg/mapunit.bin (the 0x2000-byte header
// first) into the game heap and loads it. Nothing calls it.
// callcheck: BedPfh, FileReadReq, FileReader, ~FileReader, FUN_00215a30 (constructors and destructors of locals; the assert handler inside ROR_ASSERT)
// ROR 0x0013836c

int MagScene::loadMapUnit(i3dContext *ctx, int theNo)
{
    ROR_ASSERT(theNo >= 0); /* 705 */

    FileReader reader;
    FileReadReq req;
    alignas(64) u_char head[SIZE_HEADER];
    req.m_name = "data/bg/mapunit.bin";
    req.m_08 = 1;
    req.m_buf = head;
    req.m_size = SIZE_HEADER;
    reader.read(&req, 0);

    BedPfh pfh1(head);
    ROR_ASSERT(pfh1.getFileOffset(0) <= SIZE_HEADER); /* 726 */
    ROR_ASSERT(theNo < pfh1.getNumFile()); /* 728 */
    int size = pfh1.getFileSize(theNo);
    if (size < 1)
    {
        ROR_ASSERT(0); /* 734 */
        return -6;
    }
    size = roundSector(size);
    void *p = g_utlMemory.allocDebug(size, s_file, 739, 0, 1);
    ROR_ASSERT(p != NULL); /* 740 */
    ROR_ASSERT((((int)(intptr_t)p)%64)==0); /* 741 */
    req.m_offset = pfh1.getFileOffset(theNo);
    req.m_buf = p;
    req.m_close = 1;
    req.m_whence = 0;
    req.m_size = size;
    reader.read(&req, 0);
    return load(ctx, theNo, p, 0);
}

// ──────────────────────────────────────────────────────────────────────
// Loads the map unit named theName (id 9999); the sky slot keeps the
// buffer.
// callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
// ROR 0x00138610

void MagScene::loadMapByName(i3dContext *ctx, const char *theName)
{
    ROR_ASSERT(theName); /* 763 */
    void *p = m_map_loader.loadByName(theName);
    ROR_ASSERT(p); /* 768 */
    int n = load(ctx, 9999, p, 0);
    m_tenkyu[n].m_buf = p;
}

// ──────────────────────────────────────────────────────────────────────
// Frees sky slot no when it is in use: waits for the GS path, releases
// its three models and its buffer, frees its textures.
// ROR 0x001386d4

void MagScene::releaseTenkyu(int no)
{
    MagSceneTenkyu *t = &m_tenkyu[no];

    if (t->m_id < 0)
        return;
    sceGsSyncPath(0, 0);
    t->m_model[0].release(0);
    t->m_model[1].release(0);
    t->m_model[2].release(0);
    t->m_c_data = NULL;
    t->m_c_num = 0;
    // PC: the host's record table (see load).
    if (t->m_path_data)
        g_utlMemory.free(t->m_path_data);
    t->m_path_data = NULL;
    t->m_light_data = NULL;
    if (t->m_buf)
    {
        g_utlMemory.free(t->m_buf);
        t->m_buf = NULL;
    }
    t->m_id = -1;
    releaseTenkyuTex(no);
    FUN_0017c2c0(&m_8542c);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x00138804

void MagScene::releaseTenkyuAll()
{
    for (int i = 0; i < 4; i++)
        releaseTenkyu(i);
}

// ──────────────────────────────────────────────────────────────────────
// The first actor without a model, -1 when all 128 hold one.
// callcheck: I3rRoot__CheckIdString, FUN_00215a30 (inside the inline DogModel::isAlive; the assert handler)
// ROR 0x0013884c

int MagScene::findFreeActor()
{
    int i;
    for (i = 0; i < 128; i++)
    {
        if (!m_actors[i].isAlive())
            break;
    }
    return i < 128 ? i : -1;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: I3rRoot__CheckIdString, FUN_00215a30 (inside the inline DogModel::isAlive; the assert handler)
// ROR 0x001388e8

int MagScene::findActorById(int theID)
{
    if (theID < 0)
    {
        ROR_ASSERT(theID >= 0); /* 867 */
        return -1;
    }
    for (int i = 0; i < 128; i++)
    {
        if (m_actors[i].isAlive() && m_actors[i].getId() == theID)
            return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: I3rRoot__CheckIdString, FUN_00215a30 (inside the inline DogModel::isAlive; the assert handler)
// ROR 0x001389c4

int MagScene::findActorByName(const char *theName)
{
    if (!theName)
    {
        ROR_ASSERT(theName); /* 891 */
        return -1;
    }
    for (int i = 0; i < 128; i++)
    {
        if (m_actors[i].isAlive() && m_actors[i].checkName(theName))
            return i;
    }
    return -1;
}

// ──────────────────────────────────────────────────────────────────────
// A free actor showing actor theActorNo's model. Returns it, -100 when
// none is free, -200 when sharing fails.
// callcheck: DogEntity::reset (EE virtual slot 1, called through the vtable)
// callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
// ROR 0x00138ac8

int MagScene::shareActor(u_int theActorNo)
{
    if (theActorNo >= 128)
    {
        ROR_ASSERT(theActorNo >= 0 && theActorNo < 128); /* 965 */
        return -300;
    }
    int n = findFreeActor();
    if (n < 0)
        return -100;
    DogActor *a = &m_actors[n];
    a->reset();
    m_actor_flags[n] = 0;
    m_actor_snd[n] = NULL;
    if (a->shareModel(&m_actors[theActorNo]) != 0)
        return -200;
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// A character by name: shares an actor that already shows it, else reads
// it from CHARA.RPK.
// callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
// ROR 0x00138bc4

int MagScene::loadModelByName(i3dContext *ctx, const char *theCharaName, int arg,
                              const char *rpk)
{
    (void)arg;
    (void)rpk;
    ROR_ASSERT(theCharaName); /* 993 */
    int n = findActorByName(theCharaName);
    if (n >= 0)
        return shareActor(n);

    int k = m_chara_loader.m_index.find(theCharaName);
    ROR_ASSERT(k >= 0); /* 1019 */
    void *p = m_chara_loader.load(k);
    ROR_ASSERT(p); /* 1021 */
    return loadActor(ctx, m_chara_loader.getId(k), p, 0);
}

// ──────────────────────────────────────────────────────────────────────
// Puts character theID into a free actor: shares an actor already
// holding the id (freeing thePtr unless keep), else loads the pack
// thePtr. Returns the actor, -1 when none is free, -2 / -4 on failure.
// callcheck: DogEntity::reset (EE virtual slot 1, called through the vtable)
// callcheck: BedPfh, FUN_00215a30 (the local pack view constructor; the assert handler)
// ROR 0x00138ce8

int MagScene::loadActor(i3dContext *ctx, int theID, void *thePtr, int keep)
{
    int n = findFreeActor();
    if (n < 0)
        return -1;
    DogActor *a = &m_actors[n];
    a->reset();
    m_actor_flags[n] = 0;
    m_actor_snd[n] = NULL;

    if (theID >= 0)
    {
        int src = findActorById(theID);
        if (src >= 0)
        {
            if (a->shareModel(&m_actors[src]) != 0)
                return -2;
            if (!keep && thePtr)
                g_utlMemory.free(thePtr);
            return n;
        }
    }

    ROR_ASSERT(thePtr); /* 1222 */
    BedPfh pfh(thePtr);
    if (a->load(pfh, &m_tex_info, keep ? 2 : 1, m_22cc) != 0)
        return -4;
    m_actor_flags[n] = 0;
    m_actor_snd[n] = NULL;
    a->setId(theID);
    a->setParam1d8(5);
    a->ready();
    if (ctx)
        m_tex_info.setTexTable(ctx);
    return n;
}

// ──────────────────────────────────────────────────────────────────────
// A character by id: shares an actor that already shows it, else reads
// it from CHARA.RPK. Returns the actor, -999 for a negative id.
// callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
// ROR 0x00138ed0

int MagScene::loadModel(i3dContext *ctx, int theID, const char *rpk)
{
    (void)rpk;
    if (theID < 0)
    {
        ROR_ASSERT(theID >= 0); /* 1281 */
        return -999;
    }
    int n = findActorById(theID);
    if (n >= 0)
        return shareActor(n);

    int k = m_chara_loader.getIndex(theID);
    ROR_ASSERT(k >= 0); /* 1302 */
    void *p = m_chara_loader.load(k);
    ROR_ASSERT(p); /* 1304 */
    return loadActor(ctx, theID, p, 0);
}

// ──────────────────────────────────────────────────────────────────────
// Empties actor theN (after the GS has drawn it); unbinds the PC when it
// was the PC's actor.
// callcheck: FUN_00215a30 (the assert handler, inside ROR_ASSERT)
// ROR 0x00139010

void MagScene::releaseActor(u_int theN)
{
    sceGsSyncPath(0, 0);
    ROR_ASSERT(theN >= 0 && theN < 128); /* 1383 */
    m_actors[theN].releaseModel(&m_tex_info);
    m_actor_flags[theN] = 0;
    m_actor_snd[theN] = NULL;
    if (theN == (u_int)m_pc_no)
        setPC(-1, 0);
}

// ──────────────────────────────────────────────────────────────────────
// Loads actor no's motion pack from the PC's pack file (the ROM passes
// b before a).
// ROR 0x001390d4

void MagScene::setActorAnim(int no, int a, int b)
{
    /* PORT: GamMan::setPcAnim passes m_pcActor, still -1 while
     * MagLifePC::setActor is a stub; m_actors[-1] lies over the text
     * windows (loadPackMotion's clears filled m_window, hazards.md). */
    if (no < 0 || no >= 128)
        return;
    m_actors[no].loadPackMotion("data/ch/pc/pc/MOTIONPK.RPK", b, a);
}

// ──────────────────────────────────────────────────────────────────────
// Reads the motion pack name whole into the game heap (the actor keeps
// it) and loads its motions into actor no. Returns -1 without a name, -2
// when the file is missing or empty.
// callcheck: BedPfh, FileReadReq, FileReader, ~FileReader, FUN_00215a30 (constructors and destructors of locals; the assert handler inside ROR_ASSERT)
// ROR 0x0013910c

int MagScene::loadActorAnim(int no, int arg, const char *name)
{
    if (!name)
        return -1;
    if (fileGetSize(name) < 1)
        return -2;

    FileReader reader;
    FileReadReq req;
    alignas(64) u_char head[0x800];
    req.m_buf = head;
    req.m_name = name;
    req.m_size = 0x800;
    req.m_08 = 1;
    reader.read(&req, 0);

    BedPfh pfh_head(head);
    int size = roundSector(pfh_head.getTotalSize());
    void *p = g_utlMemory.allocDebug(size, s_file, 1484, 0, 1);
    ROR_ASSERT(p != NULL); /* 1485 */
    ROR_ASSERT((((int)(intptr_t)p)%64)==0); /* 1486 */
    req.m_buf = p;
    req.m_close = 1;
    req.m_offset = 0;
    req.m_whence = 0;
    req.m_size = size;
    reader.read(&req, 0);

    BedPfh pfh(p);
    return m_actors[no].loadMotion(pfh, arg);
}

// ──────────────────────────────────────────────────────────────────────
// Reads entry theName of the pack theFile into a temporary buffer and
// adds it to the scene's textures; returns the texture index.
// callcheck: BedPfh, FileReadReq, FileReader, ~FileReader, FUN_00215a30 (constructors and destructors of locals; the assert handler inside ROR_ASSERT)
// ROR 0x00139308

int MagScene::loadTexture(const char *theFile, const char *theName)
{
    alignas(64) u_char head[0x800];
    memset(head, 0, sizeof(head));

    FileReader reader;
    FileReadReq req;
    req.m_buf = head;
    req.m_size = 0x800;
    req.m_name = theFile;
    reader.read(&req, 0);

    BedPfh pfh_head(head);
    ROR_ASSERT(pfh_head.getFileOffset(0) == (2048)); /* 1548 */
    int n = pfh_head.getFileIndex(theName);
    ROR_ASSERT(n >= 0); /* 1553 */
    int filelen = pfh_head.getFileSize(n);
    ROR_ASSERT(filelen > 0); /* 1557 */
    int size = roundSector(filelen);
    void *p = g_utlMemory.alloc(size, 1);
    ROR_ASSERT(p != NULL); /* 1563 */
    req.m_offset = pfh_head.getFileOffset(n);
    req.m_buf = p;
    req.m_whence = 0;
    req.m_size = size;
    reader.read(&req, 0);

    int no = m_tex_info.addData(theName, p, 1, -1);
    g_utlMemory.free(p);
    return no;
}
