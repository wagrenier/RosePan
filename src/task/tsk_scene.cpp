/* ==========================================================================
 *  task/tsk_scene.cpp
 *
 *  The event scene player and the script tasks next to it (pinned
 *  0x1f6c64..0x1fc4f0); see tsk_scene.h. Written so far: TskSnd (sound
 *  fades, delayed SEs, the stream switch), TskCamera (the script's stick /
 *  face-to-face cameras), gamSystemDataLoaded / gamStage2Loaded, the
 *  event scene player (ScenePlay with DisplayMessage / SeManager /
 *  VoiceManager / ScenePlaySingle, scenePlayStart), TskAttack and
 *  TskDamage (setup, the reaction frames, kill) with scnChrRequest.
 * ======================================================================== */
#include "task/tsk_scene.h"
#include "common/rosepan_stub.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bed/bedmemory.h"
#include "bed/bedpad.h"
#include "bed/bedpfh.h"
#include "debug/motnamecnv0.h"
#include "dog/dogactor.h"
#include "game/gamfile.h"
#include "game/gamman.h"
#include "lib/unassigned.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "sdk/libvu0.h"
#include "sequence/sequence.h"
#include "snd/sndman.h"
#include "task/naga_chunk.h"
#include "utl/utlmisc.h"

/* allocDebug's file name (data 0x2fe858). */
static const char s_file[] = "task/tsk_scene.cpp";

/* A record word read as the float it holds (kinds 1 and 2). */
static float scnRecFloat(int word)
{
    float f;
    memcpy(&f, &word, sizeof(f));
    return f;
}

/* The sound position for an actor without a Bip01 node (bss 0x6e9780). */
static sceVu0FVECTOR s_scnSndPos;

/* The position play3d gives the sound: actor's Bip01 node in the world,
 * or its translation copied into s_scnSndPos (the body SeManager::play3d
 * and VoiceManager::play3d share in the ROM). */
static float *scnActorSndPos(DogActor &actor, sceVu0FMATRIX mtx)
{
    u_int node = (u_int)actor.getNodeIndexByName("Bip01");
    if (node < 100)
    {
        actor.getNodeWorldMatrix(mtx, (int)node);
        return mtx[3];
    }
    s_scnSndPos[2] = actor.m_trans[2];
    s_scnSndPos[0] = actor.m_trans[0];
    s_scnSndPos[1] = actor.m_trans[1];
    return s_scnSndPos;
}

// ══════════════════════════════════════════════════════════════════════
//  DisplayMessage
// ══════════════════════════════════════════════════════════════════════

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f6cb8

void DisplayMessage::tick()
{
    if (m_timer != 0 && --m_timer == 0)
        g_gamMan.killMsgTask();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f6cec

void DisplayMessage::reset()
{
    m_cur = -1;
    m_timer = 0;
    m_count = 0;
    g_gamMan.killMsgTask();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f6d20

void DisplayMessage::addLine(char *line)
{
    m_line[m_count] = line;
    m_line[m_count][strlen(line)] = '\0';
    m_count++;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: NagaChunk, ~NagaChunk (the local reader, inline)
// ROR 0x001f6d8c

void DisplayMessage::load(void *data)
{
    NagaChunk chunk(data);

    m_count = 0;
    m_timer = 0;
    m_cur = -1;
    while (chunk.find("strings"))
        addLine((char *)chunk.getData());
}

// ──────────────────────────────────────────────────────────────────────
// The message window at (0x20, 0x140), 0x1c0 x 0x50.
// ROR 0x001f6dfc

void DisplayMessage::show(int no, int frames)
{
    if (no < -1)
        no = -1;
    if (m_cur == no)
        return;
    g_gamMan.killMsgTask();
    if (no < 0)
    {
        no = -1;
    }
    else if (no < m_count)
    {
        int param[8] = { 0x20, 0x140, 0xffff, 0x1c0, 0x50, 0, 0, 0 };
        g_gamMan.setMsgParam(param);
        g_gamMan.openMsg(m_line[no], 0, 0xb4);
        m_timer = frames;
    }
    else
    {
        no = -1;
    }
    m_cur = no;
}

// ══════════════════════════════════════════════════════════════════════
//  SeManager
// ══════════════════════════════════════════════════════════════════════

// ──────────────────────────────────────────────────────────────────────
// callcheck: i3rGetNodeIndexByName, FUN_00215a30 (DogModel::getNodeIndexByName, inline in scnActorSndPos, with its m_i3r_root assert)
// ROR 0x001f6f4c

void SeManager::play3d(u_int se, int actor, int frames)
{
    sceVu0FMATRIX mtx;

    m_entry[m_count].m_se = se;
    m_entry[m_count].m_actor = actor;
    m_entry[m_count].m_frames = frames;
    g_gamMan.m_sndMan.m_mode = 1;   /* setSnd3DCnt(1), inline */
    float *pos = scnActorSndPos(g_magSystem.m_scene.m_actors[actor], mtx);
    g_gamMan.m_sndMan.playSE(se, false, pos, 0);
    m_count++;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f70c8

void SeManager::play(u_int se, int vol, int pan, int frames)
{
    int volL = vol;
    int volR = vol;

    m_entry[m_count].m_se = se;
    m_entry[m_count].m_actor = 0;
    m_entry[m_count].m_frames = frames;
    if (pan >= 0)
        volL = (0x3f - pan) * vol / 64;
    if (pan < 1)
        volR = (pan + 0x3f) * vol / 64;
    g_gamMan.m_sndMan.playSE(se, volL, volR, 1.0f, 0, 0, 0);
    m_count++;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f71b8

void SeManager::tick()
{
    for (int i = 0; i < m_count; i++)
    {
        if (m_entry[i].m_frames != 0 && --m_entry[i].m_frames == 0)
        {
            stop(i);
            i--;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f723c

void SeManager::stopAll()
{
    for (int n = m_count; n > 0; n--)
        stop(0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f728c

void SeManager::stop(int n)
{
    if (n >= m_count)
        return;
    g_gamMan.m_sndMan.funcSE(m_entry[n].m_se, 1, NULL);
    if (--m_count != n)
        m_entry[n] = m_entry[m_count];
}

// ──────────────────────────────────────────────────────────────────────
// Unreferenced. The ROM tests entry m_count (one past the last) on every
// pass, not entry i.
// ROR 0x001f7338

void SeManager::stopSE(u_int se, int actor)
{
    for (int i = 0; i < m_count; i++)
    {
        if (m_count >= 0x40)        /* PC: entry m_count would be past the array */
            break;
        const Entry &e = m_entry[m_count];
        if (e.m_actor == actor && e.m_se == se)
        {
            stop(i);
            i--;
        }
    }
}

// ══════════════════════════════════════════════════════════════════════
//  VoiceManager
// ══════════════════════════════════════════════════════════════════════

// ──────────────────────────────────────────────────────────────────────
// callcheck: i3rGetNodeIndexByName, FUN_00215a30 (DogModel::getNodeIndexByName, inline in scnActorSndPos, with its m_i3r_root assert)
// ROR 0x001f7444

void VoiceManager::play3d(int str, int actor, int frames)
{
    sceVu0FMATRIX mtx;

    m_frames = frames;
    m_str = str;
    m_actor = actor;
    g_gamMan.m_sndMan.m_mode = 1;   /* setSnd3DCnt(1), inline */
    float *pos = scnActorSndPos(g_magSystem.m_scene.m_actors[actor], mtx);
    g_gamMan.m_sndMan.playStr(str, -1, pos, false, 0);
}

// ──────────────────────────────────────────────────────────────────────
// SndMan id 1, not looping.
// ROR 0x001f7570

void VoiceManager::play(int str, int vol, int pan, int frames)
{
    int volL = vol;
    int volR = vol;

    m_frames = frames;
    m_str = str;
    m_actor = 0;
    if (pan >= 0)
        volL = (0x3f - pan) * vol / 64;
    if (pan < 1)
        volR = (pan + 0x3f) * vol / 64;
    g_gamMan.m_sndMan.playStr(str, volL, volR, 1.0f, 0, 1, 0);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f7610

void VoiceManager::tick()
{
    if (m_frames != 0 && --m_frames == 0)
        stop();
}

// ══════════════════════════════════════════════════════════════════════
//  ScenePlaySingle
// ══════════════════════════════════════════════════════════════════════

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f764c

ScenePlaySingle::ScenePlaySingle(int actor, ScenePlayRecord *data)
{
    m_oid = (u_int)-1;
    m_active = 1;
    m_data = data;
    m_actor = actor;
    m_frame = -1;
    m_index = 0;
    m_owner = NULL;                 /* PC: addSingle sets it */
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f76b0

int ScenePlaySingle::exec()
{
    step();
    move();
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f76e0

bool ScenePlaySingle::step()
{
    if (m_active == 0)
        return false;
    m_frame++;
    if (m_data[m_index].m_kind == 0)
        return false;
    while (m_data[m_index].m_frame <= m_frame && run(0))
        ;
    return m_active != 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 1 wraps the angle once into -pi..pi with the ROM's constants
// (0x40490fdc / 0x40c90fdc); kind 2 starts the motion at 1/60
// (0x3c888897), then sets its speed to the record's frames a second.
// ROR 0x001f7788

bool ScenePlaySingle::run(int skip)
{
    const float kPi = 0x1.921fb8p+1f;
    const float kTwoPi = 0x1.921fb8p+2f;
    MagScene &scene = g_magSystem.m_scene;
    DogActor &actor = scene.m_actors[m_actor];
    const ScenePlayRecord &rec = m_data[m_index];

    switch (rec.m_kind)
    {
    case 1:
    {
        actor.m_trans[1] = scnRecFloat(rec.m_0c);
        actor.m_dirty = 1;
        actor.m_trans[0] = scnRecFloat(rec.m_08);
        actor.m_trans[2] = scnRecFloat(rec.m_10);
        float ry = scnRecFloat(rec.m_14);
        if (ry < -kPi)
            ry += kTwoPi;
        else if (ry >= kPi)
            ry -= kTwoPi;
        float rot[4] = { actor.m_rotate[0], ry, actor.m_rotate[2], 0.0f };
        g_magSystem.setActorRotate(m_actor, rot);
        actor.setBaseRotateY(ry);
        break;
    }
    case 2:
        actor.setParam1d8(rec.m_14);
        actor.changeMotion(0x1.11112ep-6f, rec.m_08, 1, -1, -1);
        actor.m_motion.m_speed = scnRecFloat(rec.m_10) / 60.0f;
        break;
    case 3:
        if (skip == 0)
        {
            int se = g_gamMan.m_sndMan.get_se_oid(rec.m_name);
            if (rec.m_10 == 0 && rec.m_14 != 0)
                m_owner->m_se.play((u_int)se, rec.m_14, rec.m_pan, rec.m_frames);
            else
                m_owner->m_se.play3d((u_int)se, m_actor, rec.m_frames);
        }
        break;
    case 4:
        if (skip == 0)
        {
            int str = g_gamMan.m_sndMan.getStrNO(rec.m_name);
            if (rec.m_10 == 0 && rec.m_14 != 0)
                m_owner->m_voice.play(str, rec.m_14, rec.m_pan, 0);
            else
                m_owner->m_voice.play3d(str, m_actor, 0);
        }
        break;
    case 5:
        if (skip == 0)
            m_owner->m_msg.show(rec.m_08, rec.m_0c);
        break;
    case 6:
    {
        int state = scene.getFadeState();
        if (state != 3 && state != 5)
            scene.fadeIn(rec.m_08, 7, 0);
        break;
    }
    case 7:
    {
        int state = scene.getFadeState();
        if (state != 4 && state != 6)
            scene.fadeOut(rec.m_08, 7, 0);
        break;
    }
    case 8:
        if (m_oid != (u_int)-1)
            g_gamMan.showLinked(m_oid);
        actor.setHide(0);
        break;
    case 9:
        if (m_oid != (u_int)-1 && (((int)m_oid >> 23) & 0x1f) == 0)
            g_gamMan.getEmyOids();      /* the ROM passes m_oid in a1; not read */
        actor.setHide(1);
        break;
    }

    if (m_data[m_index + 1].m_kind == 0)
    {
        m_active = 0;
        return false;
    }
    m_index++;
    return true;
}

// ──────────────────────────────────────────────────────────────────────
// The motion's root move over this frame (twice the speed in 120 Hz
// frames), turned by the actor's base Y rotation.
// ROR 0x001f7c0c

void ScenePlaySingle::move()
{
    MagScene &scene = g_magSystem.m_scene;
    DogActor &actor = scene.m_actors[m_actor];
    float rot[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    sceVu0FVECTOR d = { 0.0f, 0.0f, 0.0f, 1.0f };
    sceVu0FVECTOR pos = { 0.0f, 0.0f, 0.0f, 1.0f };
    sceVu0FMATRIX mtx;

    float frame = actor.getMotionFrame120();
    float speed = scene.getActorMotionSpeed(m_actor);
    actor.getMotionMove(frame, frame + speed * 60.0f + speed * 60.0f, d, rot, pos);
    float ry = actor.m_base_rotate[1];
    d[1] = 0.0f;
    d[3] = 1.0f;
    sceVu0UnitMatrix(mtx);
    sceVu0RotMatrixY(mtx, mtx, ry);
    sceVu0ApplyMatrix(d, mtx, d);
    actor.addTrans(d, pos[1], rot[0]);
}

// ══════════════════════════════════════════════════════════════════════
//  ScenePlay
// ══════════════════════════════════════════════════════════════════════

// ──────────────────────────────────────────────────────────────────────
// callcheck: DisplayMessage, SeManager, VoiceManager, ScenePlay::init (the member constructors and the empty slot-3 body, inline)
// ROR 0x001f7d54

ScenePlay::ScenePlay(int arg)
{
    memset(m_single, 0, sizeof(m_single));
    m_frame = m_frames = 0;         /* PC: the ROM leaves them to setFrames */
    memset(m_cameraPath, 0, sizeof(m_cameraPath));
    memset(m_focusPath, 0, sizeof(m_focusPath));
    memset(m_actor, 0, sizeof(m_actor));
    memset(m_name, 0, sizeof(m_name));
    init();
    m_3c = arg;
    m_focusData = NULL;
    m_eff = -1;
    m_numSingle = 0;
    m_numActor = 0;
    m_cameraData = NULL;
}

// ──────────────────────────────────────────────────────────────────────
// The SEs stopped, the loaded models released, the threads deleted, the
// effect released and the message closed.
// ROR 0x001f7e50

void ScenePlay::term()
{
    MagScene &scene = g_magSystem.m_scene;

    m_se.stopAll();
    gamStage2Loaded();
    for (int i = 0; i < m_numActor; i++)
        scene.releaseActor((u_int)m_actor[i]);
    m_numActor = 0;
    for (int i = 0; i < m_numSingle; i++)
    {
        if (m_single[i] != NULL)
            delete m_single[i];
        m_single[i] = NULL;         /* PC: the destructor runs term again */
    }
    m_numSingle = 0;
    if (m_eff != -1)
    {
        scene.m_eff_man.release(m_eff);
        m_eff = -1;
    }
    m_msg.show(-1, 0);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: operator_new (the new expression)
// ROR 0x001f7f68

void ScenePlay::addSingle(int actor, ScenePlayRecord *data, u_int oid)
{
    ScenePlaySingle *single = new ScenePlaySingle(actor, data);

    m_single[m_numSingle] = single;
    m_single[m_numSingle]->m_owner = this;
    m_single[m_numSingle]->m_oid = oid;
    m_numSingle++;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f8028

void ScenePlay::setFrames(int frames)
{
    m_frame = frames;
    m_frames = frames;
}

// ──────────────────────────────────────────────────────────────────────
// The event number is script global 0x1f.
// callcheck: FileReader, ~FileReader, FileReadReq (the inline locals)
// ROR 0x001f8034

void ScenePlay::loadCamera(const char *name)
{
    int evt = g_sequence.m_global[0x1f];

    if (m_cameraData != NULL)
    {
        utlMemFree(m_cameraData);
        m_cameraData = NULL;
    }
    if (name == NULL || strlen(name) == 0)
        return;

    FileReader reader;
    char *path = m_cameraPath;
    strcpy(path, "data/");
    char base[0x40];                    /* PC: the ROM prints the path over itself */
    strcpy(base, path);
    snprintf(path, 0x40, "%sevt/%02d/camera/", base, evt);
    strcpy(path + strlen(path), name);
    m_cameraData = g_utlMemory.allocDebug(0x20000, s_file, 790, 0, 1);

    FileReadReq req;
    req.m_buf = m_cameraData;
    req.m_close = 1;
    req.m_08 = 1;
    req.m_size = 0x20000;
    req.m_whence = 0;
    req.m_offset = 0;
    req.m_name = path;
    reader.read(&req, 0);

    EffParam eff;
    memset(&eff, 0, sizeof(eff));       /* PC: zeroed (the ROM sets the words below) */
    eff.m_kind = 0xd;
    eff.m_08 = 0;
    eff.m_file = m_cameraData;
    m_eff = g_magSystem.m_scene.m_eff_man.createEff(&eff);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: FileReader, ~FileReader, FileReadReq (the inline locals)
// ROR 0x001f81a8

void ScenePlay::loadFocus(const char *name)
{
    int evt = g_sequence.m_global[0x1f];

    if (m_focusData != NULL)
    {
        utlMemFree(m_focusData);
        m_focusData = NULL;
    }
    if (name == NULL || strlen(name) == 0)
        return;

    FileReader reader;
    char *path = m_focusPath;
    snprintf(path, 0x40, "data/evt/%02d/focus/%s", evt, name);
    int size = fileGetSize(path);
    m_focusData = g_utlMemory.allocDebug(size, s_file, 852, 0, 1);

    FileReadReq req;
    req.m_buf = m_focusData;
    req.m_close = 1;
    req.m_08 = 1;
    req.m_whence = 0;
    req.m_offset = 0;
    req.m_size = size;
    req.m_name = path;
    reader.read(&req, 0);
    g_magSystem.m_scene.setFocusData(m_focusData, size);
}

// ──────────────────────────────────────────────────────────────────────
// Ends the task when the frames run out.
// ROR 0x001f82e4

int ScenePlay::exec(int id, int time, int arg)
{
    (void)id;
    (void)time;
    (void)arg;
    for (int i = 0; i < m_numSingle; i++)
        m_single[i]->exec();
    if (--m_frame >= 1)
    {
        m_msg.tick();
        m_se.tick();
        m_voice.tick();
    }
    return m_frame < 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f836c

void ScenePlay::setName(const char *name)
{
    strncpy(m_name, name, 0x1f);
    m_name[0x1f] = '\0';
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f839c

char *scnMakeScenePath(char *out, const char *name)
{
    char base[0x20];

    strcpy(base, name);
    char *dot = strstr(base, ".");
    if (dot != NULL)                /* PC: the ROM stores through NULL without a '.' */
        *dot = '\0';
    sprintf(out, "%s/%s", base, base);
    return out;
}

// ──────────────────────────────────────────────────────────────────────
// The file scnMakeScenePath(name) of GamMan's event pack: one
// ScenePlaySingle per CHARA / THREAD (its model loaded unless oids gives
// a live object; one oid a thread), the motion names converted once (the
// "map" word becomes 0xcccc), then the frame count and the message lines.
// withEff (native 0x46) adds the pack's effect (file 1) and focus track
// (file 2). The FileReader is constructed and destroyed unused.
// callcheck: FUN_00215a30, FileReader, ~FileReader, NagaChunk, ~NagaChunk, operator_new (BedPfh's inline asserts, the inline locals, the new expression)
// ROR 0x001f83f4

int scenePlayStart(const char *name, intptr_t oidsArg, int withEff)
{
    MagScene &scene = g_magSystem.m_scene;
    const int *oids = (const int *)oidsArg;
    FileReader reader;
    int kinds[2] = { 5, -1 };
    char path[0x20];
    int frames = 0;
    int converted = 0;
    BedPfh pack;

    g_gamMan.resume(0x10000);
    g_gamMan.m_flags2 &= 0x7fffffff;
    g_sequence.m_flags |= 0x400;
    g_taskMan.killKinds(kinds);
    pack.clear();
    scnMakeScenePath(path, name);
    BedPfh evt(g_gamMan.m_evtPack);
    int n = evt.getFileIndex(path);
    pack = BedPfh(evt.getFile(n));
    void *top = pack.getFile(0);
    ScenePlay *play = new ScenePlay(0);

    NagaChunk chunk(top);
    if (chunk.find("map"))
        converted = chunk.getInt() == 0xcccc;
    chunk.rewind();
    if (chunk.find("CHARA"))
    {
        NagaChunk chara(chunk.getData());
        while (chara.find("THREAD"))
        {
            const char *modelName = NULL;
            ScenePlayRecord *data = NULL;
            NagaChunk thread(chara.getData());
            if (thread.find("name"))
                modelName = (const char *)thread.getData();

            int actor;
            if (*oids == -1 || g_gamMan.isObj((u_int)*oids) == 0)
            {
                int arg = 0;
                u_int oid = 0;
                int cat = motNameGetCategory(motNameFindChara(modelName));
                int num = cat < 8 ? 0 : cat - 7;
                if (num < 1)
                {
                    switch (cat)
                    {
                    case 0: oid = g_gamMan.makeOid(1, 0, -1); arg = 0x2000; break;
                    case 1: oid = g_gamMan.makeOid(1, 1, -1); arg = 0x2400; break;
                    case 2: oid = g_gamMan.makeOid(1, 3, -1); break;
                    case 3: oid = g_gamMan.makeOid(2, 5, -1); arg = 0xc00;  break;
                    case 4: oid = g_gamMan.makeOid(2, 4, -1); arg = 0x800;  break;
                    case 5: oid = g_gamMan.makeOid(1, 2, -1); arg = 0x1400; break;
                    case 6: oid = g_gamMan.makeOid(2, 9, -1); arg = 0x1800; break;
                    }
                }
                else
                {
                    arg = num * 0x400 + 0x2400;
                    oid = g_gamMan.makeOid(1, 3, -1);
                }
                char lower[0x80];
                utlStrToLower(lower, modelName);
                actor = scene.loadModelByName(&g_magSystem.m_i3d, lower, arg, NULL);
                g_gamMan.setObjLight(actor, oid);
                play->m_actor[play->m_numActor++] = actor;
            }
            else
            {
                actor = g_gamMan.getActorNo((u_int)*oids);
            }
            if (actor < 0)                  /* PC: the ROM indexes the actors with -1 */
            {
                oids++;
                continue;
            }
            scene.m_actors[actor].setHide(0);

            thread.rewind();
            if (thread.find("data"))
            {
                data = (ScenePlayRecord *)thread.getData();
                play->addSingle(actor, data, (u_int)*oids);
            }
            thread.rewind();
            if (thread.find("STR"))
            {
                intptr_t names[0x14][2];
                int i = 0;
                NagaChunk str(thread.getData());
                while (str.next() && i < 0x13)
                {
                    names[i][0] = (intptr_t)str.getData();
                    names[i][1] = (int)(*(u_int *)str.getEntry() & 0x7fffffff);
                    i++;
                }
                names[i][0] = 0;
                names[i][1] = 0;
                (void)strcmp("PC2", modelName);     /* the ROM drops the result */
                if (converted == 0)
                    motNameConvert(actor, data, &names[0][0]);
            }
            oids++;
        }
    }

    chunk.rewind();
    if (chunk.find("map"))
        *(int *)chunk.getData() = 0xcccc;
    chunk.rewind();
    if (chunk.find("frame"))
    {
        play->setFrames(chunk.getInt());
        frames = chunk.getInt();
    }
    chunk.rewind();
    play->m_msg.reset();
    if (chunk.find("MESSAGE"))
        play->m_msg.load(chunk.getData());

    if (withEff != 0)
    {
        EffParam param;
        memset(&param, 0, sizeof(param));
        param.m_kind = 0xd;
        param.m_08 = 0;
        param.m_file = pack.getFile(1);
        play->m_eff = scene.m_eff_man.createEff(&param);
        if (pack.getNumFile() > 2)
            scene.setFocusData(pack.getFile(2), pack.getFileSize(2));
    }
    g_taskMan.addTask(1, -1, 1, 0, play, 0, &g_gamMan.m_frameTaskId, 0x11);
    return frames;
}

// ──────────────────────────────────────────────────────────────────────
// scenePlayStart's pack lookup without its threads: the ScenePlay only
// carries the pack's effect (file 1) and the name. It reads no "frame"
// chunk, so the ScenePlay's frame count is the constructor's (the PC's
// 0: the task ends on its first exec and releases the effect; the ROM
// leaves the word to the heap -- see open-items.md). The FileReader and
// the NagaChunk over file 0 are constructed and destroyed unused.
// callcheck: FUN_00215a30, FileReader, ~FileReader, NagaChunk, ~NagaChunk, operator_new (BedPfh's inline asserts, the inline locals, the new expression)
// ROR 0x001f8cec

void scenePlayStartSimple(const char *name)
{
    MagScene &scene = g_magSystem.m_scene;
    FileReader reader;
    char path[0x20];
    BedPfh pack;

    pack.clear();
    scnMakeScenePath(path, name);
    BedPfh evt(g_gamMan.m_evtPack);
    int n = evt.getFileIndex(path);
    pack = BedPfh(evt.getFile(n));
    void *top = pack.getFile(0);
    ScenePlay *play = new ScenePlay(0);
    NagaChunk chunk(top);
    (void)chunk;

    EffParam param;
    memset(&param, 0, sizeof(param));   /* PC: zeroed (the ROM sets these three) */
    param.m_kind = 0xd;
    param.m_08 = 0;
    param.m_file = pack.getFile(1);
    play->m_eff = scene.m_eff_man.createEff(&param);
    play->setName(name);
    g_taskMan.addTask(1, -1, 1, 0, play, 0, &g_gamMan.m_frameTaskId, 0x11);
}

// ══════════════════════════════════════════════════════════════════════
//  TskSnd
// ══════════════════════════════════════════════════════════════════════

// ──────────────────────────────────────────────────────────────────────
// Copies the parameter block and picks the kind's step; kind 6 ends at
// once (stepEnd) when the SE does not play (funcSE 0x16), kind 7 keeps
// the stream's name. The exec step is always stepFade.
// ROR 0x001f8fc0

TskSnd::TskSnd(int kind, const TskSndParam *param)
{
    m_param = *param;
    m_frames = m_param.m_frames;
    m_frame = 0;
    m_exec = &TskSnd::stepFade;
    m_kind = kind;
    memset(m_name, 0, sizeof(m_name));   /* PC: the ROM leaves the name unterminated past 0x1b chars */
    switch ((u_int)kind)
    {
    case 0:
        m_step = &TskSnd::stepSEVol;
        break;
    case 1:
        m_step = &TskSnd::stepStrVol;
        break;
    case 2:
        m_step = &TskSnd::stepMstVol;
        break;
    case 3:
        m_step = &TskSnd::stepMstVolSe;
        break;
    case 4:
        m_step = &TskSnd::stepMstVolStr;
        break;
    case 5:
        m_step = &TskSnd::stepPlaySE;
        break;
    case 6:
        if (g_gamMan.m_sndMan.funcSE((u_int)m_param.m_snd, 0x16, NULL) == 0)
            m_step = &TskSnd::stepEnd;
        else
            m_step = &TskSnd::stepSEFadeOut;
        break;
    case 7:
        strncpy(m_name, param->m_name, 0x1b);
        m_step = &TskSnd::stepPlayStream;
        break;
    default:
        m_step = &TskSnd::stepEnd;       /* PC: the ROM leaves the step unset for kinds above 7 */
        break;
    }
    m_state = 100;
}

// ──────────────────────────────────────────────────────────────────────
// The exec step (stepFade).
// ROR 0x001f91dc

int TskSnd::exec(int id, int time, int arg)
{
    (void)id;
    (void)time;
    (void)arg;
    return (this->*m_exec)();
}

// ──────────────────────────────────────────────────────────────────────
// A master volume fade ends on its target (both words the left volume,
// as the ROM passes them).
// ROR 0x001f9244

void TskSnd::term()
{
    if (m_kind == 2)
        g_gamMan.m_sndMan.funcSnd(0xd, m_param.m_volL, m_param.m_volL, 0, -1);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f9294

float TskSnd::getRate()
{
    float rate = 0.0f;
    if (m_frames > 0)
        rate = (float)m_frame / (float)m_frames;
    return rate;
}

// ──────────────────────────────────────────────────────────────────────
// In state 100: the kind's step with the volume m_rate of the way from
// the start to the target (the rate taken before the frame count goes
// up). State 101 ends the task, others wait.
// ROR 0x001f92c8

int TskSnd::stepFade()
{
    if (m_state == 100)
    {
        float rate = getRate();
        m_frame++;
        return (this->*m_step)(
            (int)((float)m_param.m_volL * rate + (float)m_param.m_curL * (1.0f - rate)),
            (int)((float)m_param.m_volR * rate + (float)m_param.m_curR * (1.0f - rate)));
    }
    return m_state == 101;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 0: the SE track's volume; at 0 / 0 the SE is stopped (funcSE 1)
// first. Ends when the SE is gone or the frames have run.
// ROR 0x001f93d8

int TskSnd::stepSEVol(int volL, int volR)
{
    GamSndMan &snd = g_gamMan.m_sndMan;
    if (volL == 0 && volR == 0)
        snd.funcSE((u_int)m_param.m_snd, 1, NULL);
    if (snd.setVolSEtrack((u_int)m_param.m_snd, m_param.m_id, volL, volR) == 0)
        return 1;
    return m_frames < m_frame;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 1: the stream track's volume; at 0 / 0 the stream is stopped
// (funcStr 1, or 0x17 with m_arg) and the task ends.
// ROR 0x001f9484

int TskSnd::stepStrVol(int volL, int volR)
{
    GamSndMan &snd = g_gamMan.m_sndMan;
    int str = m_param.m_snd;
    if (volL == 0 && volR == 0)
    {
        if (m_param.m_arg == 0)
            snd.funcStr(str, 1, NULL);
        else
            snd.funcStr(str, 0x17, NULL);
        return 1;
    }
    if (snd.setVolSTRtrack((u_int)str, m_param.m_id, volL, volR) == 0)
        return 1;
    return m_frames < m_frame;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 2: the master volume.
// ROR 0x001f9550

int TskSnd::stepMstVol(int volL, int volR)
{
    g_gamMan.m_sndMan.funcSnd(0xd, volL, volR, 0, -1);
    return m_frames < m_frame;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 3: the SE master volume.
// ROR 0x001f95a8

int TskSnd::stepMstVolSe(int volL, int volR)
{
    g_gamMan.m_sndMan.funcSnd(0xe, volL, volR, 0, -1);
    return m_frames < m_frame;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 4: the stream master volume.
// ROR 0x001f9600

int TskSnd::stepMstVolStr(int volL, int volR)
{
    g_gamMan.m_sndMan.funcSnd(0xf, volL, volR, 0, -1);
    return m_frames < m_frame;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f9658

int TskSnd::getVolume(int *volL, int *volR)
{
    *volL = m_param.m_volL;
    *volR = m_param.m_volR;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 5 (GamSndMan::playSE's delay): plays the SE now, at the block's
// volume, pitch and attribute; ends.
// ROR 0x001f9670

int TskSnd::stepPlaySE(int volL, int volR)
{
    (void)volL;
    (void)volR;
    g_gamMan.m_sndMan.playSE((u_int)m_param.m_snd, m_param.m_volL, m_param.m_volR,
                             m_param.m_pitch, m_param.m_attr, 0, 0);
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 7: stops the old stream (SndMan id m_id) each frame until it has
// stopped, then plays m_name into id m_id2 (looping with m_arg); ends.
// ROR 0x001f96c0

int TskSnd::stepPlayStream(int volL, int volR)
{
    (void)volL;
    (void)volR;
    if (g_sndMan.getStreamState(m_param.m_id) == -1)
    {
        g_sndMan.playStream(m_name, m_param.m_mode, m_param.m_id2, m_param.m_arg);
        if (m_param.m_arg != 0)
            g_sndMan.setStreamLoop(m_param.m_id2, 1);
        return 1;
    }
    g_sndMan.stopStream(m_param.m_id);
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 6 while the SE plays: after m_frames (counted twice a frame, here
// and in stepFade) the SE fades to 0 over 10 frames (funcSE 0x13); ends.
// ROR 0x001f974c

int TskSnd::stepSEFadeOut(int volL, int volR)
{
    (void)volL;
    (void)volR;
    int frame = m_frame++;
    int end = m_frames <= frame;
    if (end)
    {
        TskSndParam fade = {};   /* PC: zeroed (the ROM sets only these) */
        fade.m_volL = 0;
        fade.m_volR = 0;
        fade.m_frames = 10;
        g_gamMan.m_sndMan.funcSE((u_int)m_param.m_snd, 0x13, &fade);
    }
    return end;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 6 when the SE did not play.
// ROR 0x001f97c0

int TskSnd::stepEnd(int volL, int volR)
{
    (void)volL;
    (void)volR;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: operator_new (the new expression)
// ROR 0x001f97c8

void scnPlaySEDelayed(u_int se, int delay)
{
    int start = g_taskMan.m_timer.m_updates + delay;
    TskGenFrameFunc *task = new TskGenFrameFunc(1, (int)se, 0);
    g_taskMan.addTask(1, start, 1, 0, task, 0, NULL, 1);
}

// ══════════════════════════════════════════════════════════════════════
//  TskAttack
// ══════════════════════════════════════════════════════════════════════

// ──────────────────────────────────────────────────────────────────────
// callcheck: memset (clearing the step pointer: m_step = NULL)
// ROR 0x001f985c

TskAttack::TskAttack(u_int oid, const int *param)
{
    m_step = NULL;
    m_oid = oid;
    setup(oid, param);
}

// ──────────────────────────────────────────────────────────────────────
// The achr, the kind's motion and frames, and the step. Kinds 0x13 /
// 0x14 also start motion 9 / 10 on the target's actor at once.
// ROR 0x001f98c0

void TskAttack::setup(u_int oid, const int *param)
{
    MagScene &scene = g_magSystem.m_scene;
    /* data 0x3c888897, a hair above 1/60 */
    const float speed = scnRecFloat(0x3c888897);
    float rot[4];

    (void)oid;
    m_1c = 0;
    m_20 = 0;
    m_achr = g_gamMan.getAchr(m_oid);
    m_actor = m_achr->m_actor;
    m_kind = param[2];
    m_34 = param[0];
    g_gamMan.getObjRot(m_oid, rot);     /* the ROM drops the rotation */

    switch (m_kind)
    {
    case 0x12:
    {
        u_short kind;
        short type;
        g_gamMan.getObjParam(m_oid, 0x66, &kind);
        g_gamMan.m_sysData.getField(kind, 0x28, &type);
        if (type == 4)
        {
            if (utlRandRange(0, 100) < 0x32)
            {
                m_motion = 0x42;
                m_44 = 0x10;
                m_48 = 0x14;
            }
            else
            {
                m_motion = 0x43;
                m_44 = 0x1e;
                m_48 = 0x2b;
            }
        }
        else
        {
            m_motion = 0x44;
            m_44 = 0x14;
            m_48 = 0x17;
        }
        m_step = &TskAttack::stepKind12;
        break;
    }
    case 0x13:
        m_44 = 0x1c;
        m_motion = 0x51;
        m_48 = 0x21;
        m_target = (u_int)param[5];
        /* The ROM passes m_target in a sixth register setActorMotion does not read. */
        scene.setActorMotion(speed, g_gamMan.getActorNo((u_int)param[5]), 9, 0, 0);
        m_step = &TskAttack::stepKind12;
        break;
    case 0x14:
        m_44 = 0x20;
        m_motion = 0x52;
        m_48 = 0x26;
        m_target = (u_int)param[5];
        scene.setActorMotion(speed, g_gamMan.getActorNo((u_int)param[5]), 10, 0, 0);
        m_step = &TskAttack::stepKind12;
        break;
    case 0x15:
    {
        u_short kind;
        u_short type;
        int size;
        g_gamMan.getObjParam(m_oid, 0x66, &kind);
        g_gamMan.getObjParam(m_oid, 0x67, &m_4c);
        g_gamMan.m_sysData.getField(kind, 0x28, &type);  /* not read */
        m_motion = 0x45;
        m_44 = 0x2e;
        g_gamMan.m_sysData.getField(kind, 0x1b, &size);
        if (size == 3)
            m_40 = 70.0f;
        else if (size == 2)
            m_40 = 50.0f;
        else
            m_40 = 20.0f;
        m_54 = param[3];
        m_58 = param[4];
        m_step = &TskAttack::stepKind15;
        break;
    }
    case 0x26:
    case 0x27:
    case 0x28:
        if (m_kind == 0x26)
        {
            m_motion = 0x14;
            m_40 = 110.0f;
            m_44 = 0xe;
            m_48 = 0x11;
        }
        else if (m_kind == 0x27)
        {
            m_motion = 0x15;
            m_40 = 110.0f;
            m_44 = 8;
            m_48 = 0xb;
        }
        else
        {
            m_motion = 0x16;
            m_40 = 120.0f;
            m_44 = 0x4b;
            m_48 = 0x4e;
        }
        m_step = &TskAttack::stepKind26;
        break;
    case 0x2e:
        m_40 = scnRecFloat(param[10]);
        m_44 = 0x18;
        m_motion = param[6];
        m_48 = 0x2b;
        m_step = &TskAttack::stepKind2e;
        break;
    case 0x2f:
        m_40 = scnRecFloat(param[10]);
        m_44 = 0x23;
        m_motion = param[6];
        m_48 = 0x37;
        m_step = &TskAttack::stepKind2f;
        break;
    case 0x30:
        m_40 = scnRecFloat(param[10]);
        m_44 = 0x23;
        m_motion = param[6];
        m_48 = 0x86;
        m_step = &TskAttack::stepKind30;
        break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001f9ce8

void TskAttack::term()
{
}

// ──────────────────────────────────────────────────────────────────────
// The step while one is set and the oid is not -1, else 0.
// ROR 0x001f9cf0

int TskAttack::exec(int id, int time, int arg)
{
    (void)id;
    (void)time;
    (void)arg;
    if (m_step == NULL || m_oid == (u_int)-1)
        return 0;
    return (this->*m_step)();
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: operator_new (the new expression)
// ROR 0x001f9d90

void scnAttackStart(u_int oid, const int *param)
{
    ACHRstat *achr = g_gamMan.getAchr(oid);

    achr->killTasks();
    TskAttack *task = new TskAttack(oid, param);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, &achr->m_task[3], 0x11);
}

// ══════════════════════════════════════════════════════════════════════
//  TskDamage
// ══════════════════════════════════════════════════════════════════════

// ──────────────────────────────────────────────────────────────────────
// The steps by chr word (EE pfns 0x1fe160 .. 0x2005b8); 3 and 4 set none.
// ROR 0x001f9e58 // callcheck: dpadd, dpcmp, dpsub, dptofp, fptodp (the soft-float doubles, C doubles here)

void TskDamage::setup(u_int oid, u_int attacker, const ScnDamageParam *param, int kind)
{
    const double pi = 3.141592653589793, twoPi = 6.283185307179586;

    m_kind = (short)kind;
    m_oid = oid;
    m_flags = 0;
    m_1c = 0;
    m_frames = 0;
    m_achr = NULL;
    m_achr = g_gamMan.getAchr(oid);
    m_actor = m_achr->m_actor;
    m_damage = param->m_a2;
    m_motion = g_magSystem.m_scene.getActorMotion(m_actor);
    m_achr->getParam(0x1b, &m_lp);
    /* The ROM traps (break 7) when the enemy word is 0. */
    m_emy1d8 = (u_short)g_emyManager.getEmyWord1d8(m_oid);

    if (g_sequence.m_global[0x173] != 0)
        kind = 0;
    m_40 = (kind == 0 || kind == 1) ? 2 : 1;

    /* How far the attacker is off the achr's heading, 0..pi. */
    float rot = g_magSystem.m_scene.m_actors[m_actor].getRotY();
    float from[4], pos[4];
    int front = 0;
    g_gamMan.getObjPos(attacker, from);
    m_achr->getPos(pos);
    float off = rot - atan2f(pos[0] - from[0], pos[2] - from[2]);
    if ((double)off < -pi)
        off = (float)((double)off + twoPi);
    if ((double)off > pi)
        off = (float)((double)off - twoPi);
    if (off < 0.0f)
        off = -off;

    if ((double)off < pi / 2)
    {
        front = 1;
        g_emyManager.clearEmyWord18(m_oid);
    }
    else
    {
        g_emyManager.setEmyWord18(m_oid);
    }
    m_frames = getReactFrames(param->m_chr, &front);
    if (m_frames == 0)
    {
        if (m_40 == 2)
        {
            g_emyManager.notifyEmy(m_oid, 4);
            m_frames = front ? 10 : 0;
        }
        else
        {
            g_emyManager.notifyEmy(m_oid, 8);
            m_frames = front ? 30 : 20;
        }
    }

    if ((m_achr->m_flags & 0xc00) == 0)
    {
        if (m_damage < (int)m_lp)
        {
            m_lp -= (u_short)m_damage;
            g_gamMan.setObjParam(m_oid, 0x1b, &m_lp);
        }
        else
        {
            kill();
        }
    }
    g_emyManager.notifyEmy(m_oid, 1);
    if (param->m_word == 6)
        g_emyManager.notifyEmy(m_oid, 0x10);
    m_motion0 = m_motion;
    m_flags |= 4;
    m_achr->m_flags |= 0x400;
    m_achr->m_flags |= 0x200;

    int se = 6;
    switch (param->m_chr)
    {
    case 0:
        m_step = &TskDamage::stepKind0;
        break;
    case 1:
        m_step = &TskDamage::stepKind1;
        break;
    case 2:
        m_step = &TskDamage::stepKind2;
        break;
    case 5:
        m_step = &TskDamage::stepKind5;
        break;
    case 6:
        m_step = &TskDamage::stepKind6;
        break;
    case 7:
        if (g_emyManager.getEmyWord238(m_oid) != 0)
            m_flags = (m_flags | 8) ^ 4;
        m_step = &TskDamage::stepKind7;
        break;
    case 8:
        m_step = &TskDamage::stepKind8;
        break;
    case 3:
    case 4:
        break;
    case 9:
        m_step = &TskDamage::stepKind9;
        break;
    case 10:
        m_step = &TskDamage::stepKindA;
        break;
    default:
        emyPlaySE(m_oid, se);
        return;
    }

    switch (param->m_chr)
    {
    case 0:
    case 1:
    case 5:
    case 6:
        se = 10;
        if ((m_flags & 0x100) == 0)
        {
            se = 8;
            if (m_40 == 2)
                se = utlRandRange(0, 99) > 0x31 ? 7 : 6;
        }
        break;
    case 2:
    case 3:
    case 4:
    case 8:
        se = 6;
        break;
    case 7:
        se = 0x29;
        if ((m_flags & 0x100) == 0)
        {
            int r = utlRandRange(0, 100);
            switch (param->m_word)
            {
            case 3:
            case 4:
            case 7:
                se = r > 0x31 ? 0x2f : 0x2e;
                break;
            case 5:
                se = r > 0x31 ? 0x31 : 0x30;
                break;
            case 6:
                se = 0x23;
                break;
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
                se = 0x32;
                break;
            default:
                se = 0x16;
                break;
            }
        }
        break;
    case 9:
        se = 0x3a;
        break;
    case 10:
        se = 0x5e;
        if ((m_flags & 0x100) == 0)
        {
            utlRandRange(0, 100);       /* drawn and dropped, as the ROM does */
            switch (param->m_word)
            {
            case 3:
            case 4:
            case 7:
                se = 0x5b;
                break;
            case 5:
                se = 0x5c;
                break;
            case 6:
                se = 0x5d;
                break;
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
                se = 0x52;
                break;
            default:
                se = 0x57;
                break;
            }
        }
        break;
    }
    emyPlaySE(m_oid, se);
}

// ──────────────────────────────────────────────────────────────────────
// callcheck: operator_new, FUN_001f9e38 (the new expression; TskDamage's inline constructor)
// ROR 0x001fa5bc

void scnDamageStart(u_int oid, u_int attacker, const ScnDamageParam *param, int kind)
{
    MagScene &scene = g_magSystem.m_scene;
    ACHRstat *achr = g_gamMan.getAchr(oid);
    int *slot = &achr->m_task[3];

    if ((achr->m_flags & 0x400) != 0)
        return;
    g_emyManager.pauseEmy(oid, 1);
    if (g_taskMan.getTask(*slot) != 0)
    {
        g_taskMan.killTask(*slot);
        *slot = -1;
    }
    TskDamage *task = new TskDamage(oid, attacker, param, kind);
    g_taskMan.addTask(1, -1, 1, 0, task, 0, slot, 0x11);

    float pos[4];
    achr->getPos(pos);
    EffParam eff;
    memset(&eff, 0, sizeof(eff));       /* PC: zeroed (the ROM sets the words below) */
    eff.m_kind = 1;
    eff.m_08 = 0;
    eff.m_hit.m_pos[0] = pos[0];
    eff.m_hit.m_pos[1] = pos[1] - 40.0f;
    eff.m_hit.m_pos[2] = pos[2];
    eff.m_hit.m_1c = 0;
    eff.m_hit.m_20 = -1.0f;
    eff.m_hit.m_24 = 0;
    eff.m_hit.m_scale = kind == 2 ? 1.75f : 1.0f;
    scene.m_eff_man.createEff(&eff);
}

// ──────────────────────────────────────────────────────────────────────
// Jump table data 0x2fecb0 (chr 0..10; the others return 0). The motion
// lists are the ROM's, negatives included.
// ROR 0x001fa768

int TskDamage::getReactFrames(int chr, int *front)
{
    if ((u_int)chr >= 11)
        return 0;
    switch (chr)
    {
    case 0:
        if ((m_achr->m_attr[0] & 8) != 0 && (m_achr->m_attr[0] & 0x10) != 0)
            return 0x50;
        switch (m_motion)
        {
        case 0x28: case 0x29: case 0x2b: case 0x2c: case 0x2d:
        case -0x29: case -0x2a: case -0x2c: case -0x2d: case -0x2e:
            *front = 0;
            g_emyManager.setEmyWord18(m_oid);
            return 0x46;
        case 0x2e: case 0x2f: case 0x31: case 0x32: case 0x33:
        case -0x2f: case -0x30: case -0x32: case -0x33: case -0x34:
            *front = 1;
            g_emyManager.clearEmyWord18(m_oid);
            return 0x3c;
        default:
            return 0;
        }
    case 1:
        switch (m_motion)
        {
        case 0x1e: case 0x23: case 0x28: case 0x29:
        case -0x1f: case -0x24: case -0x29: case -0x2a:
            *front = 0;
            g_emyManager.clearEmyWord18(m_oid);
            return 0x46;
        case 0x21: case 0x22: case 0x2a: case 0x2b: case 0x2c: case 0x2d:
        case -0x22: case -0x23: case -0x2b: case -0x2c: case -0x2d: case -0x2e:
            *front = 1;
            g_emyManager.setEmyWord18(m_oid);
            return 0x3c;
        default:
            return 0;
        }
    case 8:
        switch (m_motion)
        {
        case 0x14: case 0x17: case -0x15: case -0x18:
            return 0x46;
        default:
            return 0;
        }
    default:
        return 0;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fa9c4

void TskDamage::kill()
{
    m_achr->m_flags |= 0x400;
    m_achr->m_flags |= 0x2000;
    m_achr->clearSecta(0);
    m_achr->killTasks();
    g_sequence.endThreadsWithArg((int)m_oid);
    g_emyManager.notifyEmy(m_oid, 2);
    m_lp = 0;
    m_flags |= 0x100;
    m_frames += 10000;
    g_gamMan.setObjParam(m_oid, 0x1b, &m_lp);
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001faa70

int TskDamage::pickAttrBit()
{
    static const u_int s_bits[5] = { 1, 2, 4, 8, 0x10 };
    u_int clear[8];
    u_int n = 0;

    for (int i = 0; i < 5; i++)
        if ((m_achr->m_attr[0] & s_bits[i]) == 0)
            clear[n++] = s_bits[i];
    if (n == 0)
        return 0;
    m_attrBit = clear[utlRandRange(0, (int)n - 1)];
    m_achr->m_attr[m_attrBit >> 24] |= m_attrBit & 0xffffff;
    u_int attr = m_achr->m_attr[0];
    return ((attr & 7) == 7 || (attr & 0x1e) == 0x1e || (attr & 0x19) == 0x19) ? 1 : 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fabd0

void TskDamage::term()
{
    if (m_achr != NULL)
        m_achr->empty1da074();
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fabf8

int TskDamage::exec(int id, int time, int arg)
{
    (void)id;
    (void)time;
    (void)arg;
    if (m_step == NULL)                 /* PC: chr 3 / 4 set no step (the ROM calls heap garbage); end the task */
        return 1;
    return (this->*m_step)();
}

// ──────────────────────────────────────────────────────────────────────
// chr 11's chr from object chrOid's kind 2 system field 0x72 (0x2fed10).
static const u_char s_scnChrOfType[11] = { 0, 0, 0, 1, 5, 6, 7, 8, 9, 2, 10 };

// ROR 0x001fac60

void scnChrRequest(int chr, u_int oid, int a2, int a3, int word, u_int chrOid, int a6)
{
    (void)a3;
    if (chr == 11)
    {
        switch ((chrOid >> 23) & 0x1f)
        {
        case 0:
            chr = 4;
            break;
        case 1:
            chr = 3;
            break;
        case 2:
        {
            u_short type;
            g_gamMan.m_sysData.getField(g_gamMan.getObjKind(chrOid), 0x72, &type);
            chr = type < 11 ? s_scnChrOfType[type] : 0;
            break;
        }
        default:
            break;
        }
    }
    if ((u_int)chr > 10)
        return;

    switch (chr)
    {
    case 3:
        scnChrRequestDog(oid, word);
        break;
    case 4:
        scnChrRequestPC(oid, word);
        break;
    default:
    {
        float pos[4];
        g_gamMan.getObjPos(chrOid, pos);    /* the ROM drops the position */
        ScnDamageParam param = { a2, word, chr };
        scnDamageStart(chrOid, oid, &param, a6);
        break;
    }
    }
}

// ──────────────────────────────────────────────────────────────────────
// The heading of the attacker as seen from the dog is computed and
// dropped. inu mode 1 is also the 26..50 percent reaction.
// ROR 0x001fae14 // callcheck: utlAtan2 (its result unused)

void scnChrRequestDog(u_int oid, int word)
{
    int inuMode = g_gamMan.m_inuMode;

    if (inuMode != 1 || g_magSystem.getBwnState() == 10)
        return;
    ObjStat *dog = g_gamMan.m_dogPtr;
    u_int dogOid = dog->m_oid;
    int damage = scnCalcDamage(oid, dogOid, word);
    u_short maxLp, lp;
    g_gamMan.getObjParam(dogOid, 0x1c, &maxLp);
    g_gamMan.getObjParam(dogOid, 0x1b, &lp);
    DogActor &actor = g_magSystem.m_scene.m_actors[g_gamMan.getActorNo(oid)];
    float pos[4];
    g_gamMan.getObjPos(dogOid, pos);
    utlAtan2(pos[0] - actor.m_trans[0], pos[2] - actor.m_trans[2]);

    /* The ROM traps (break 7) on a max LP of 0. */
    int share = maxLp != 0 ? damage * 100 / (int)maxLp : 0;
    int react = 0;
    if (share < 51)
        react = share < 26 ? 2 : inuMode;
    if (word == 0x25 || word == 0x2d)
        react = -1;

    u_int locked = dog->m_flags & 0x400;
    if (damage < (int)lp)
    {
        if (locked == 0)
        {
            lp -= (u_short)damage;
            g_gamMan.setObjParam(dogOid, 0x1b, &lp);
        }
        if (react >= 0)
        {
            if (react < 2)
                magBwnHurt(1, damage);
            else if (react == 2)
                magBwnHurt(0, damage);
        }
    }
    else if (locked == 0)
    {
        lp = 0;
        g_gamMan.setObjParam(dogOid, 0x1b, &lp);
    }
}

// ──────────────────────────────────────────────────────────────────────
// The PC's hurt state by attack word (data 0x2feea0).
static const u_char s_scnPcStateOfWord[0x34] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 7,
    7, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 2,
    2, 2, 9, 2, 2, 2, 2, 2, 2, 13, 14, 10, 2, 2, 2, 2,
    2, 2, 1, 2,
};

// The heading difference is wrapped once into -pi..pi in double precision
// (like seqWrapPi) and taken absolute.
// callcheck: dpadd, dpcmp, dpsub, dptofp, fptodp, operator_new (the soft-float doubles, C doubles here; the new expression)
// ROR 0x001fb084

void scnChrRequestPC(u_int oid, int word)
{
    const double pi = 3.141592653589793, twoPi = 6.283185307179586;
    MagScene &scene = g_magSystem.m_scene;
    ObjStat *pc = g_gamMan.m_pcPtr;

    if (g_magSystem.getPCStateClass() == 9)
        return;
    int damage = scnCalcDamage(oid, pc->m_oid, word);
    u_short lp;
    g_gamMan.getPcParam(0x1b, &lp);
    DogActor &actor = scene.m_actors[g_gamMan.getActorNo(oid)];
    float dir = atan2f(actor.m_trans[0] - g_gamMan.m_pcPos[0],
                       actor.m_trans[2] - g_gamMan.m_pcPos[2]);
    float rot;
    g_gamMan.getObjRotY(g_gamMan.m_pcPtr != NULL ? g_gamMan.m_pcPtr->m_oid : 0, &rot);
    u_int flags = pc->m_flags;
    int front = (flags & 0x800) == 0;

    if (damage >= (int)lp)
    {
        if ((flags & 0x400) == 0)
        {
            lp = 0;
            g_gamMan.setPcParam(0x1b, &lp);
        }
        int cls = g_magSystem.getPCStateClass();
        scene.setPCState14(3, front);
        if (g_taskMan.getTask(g_gamMan.m_frameTaskId) == 0)
        {
            TskFrameFunc *task = new TskFrameFunc(0xe, cls, 0);
            g_taskMan.addTask(1, -1, 1, 0, task, 0, &g_gamMan.m_frameTaskId, 0x11);
        }
        return;
    }

    if ((flags & 0x400) == 0)
    {
        lp -= (u_short)damage;
        g_gamMan.setPcParam(0x1b, &lp);
    }
    int state = (u_int)word < 0x34 ? s_scnPcStateOfWord[word] : 0;   /* PC: the ROM reads past the table */
    switch (state)
    {
    case 9:
        scene.setPCState14(g_emyManager.getEmyHoldWord24c(oid) != 0 ? 0xb : 9, front);
        break;
    case 10:
        scene.setPCState14(g_emyManager.getEmyHoldWord23c(oid) != 0 ? 0xc : 10, front);
        break;
    case 2:
    {
        float off = dir - rot;
        if ((double)off < -pi)
            off = (float)((double)off + twoPi);
        if ((double)off > pi)
            off = (float)((double)off - twoPi);
        if (off < 0.0f)
            off = -off;
        scene.setPCState14((double)off <= pi / 2 ? 5 : 4, front);
        break;
    }
    default:
        scene.setPCState14(state, front);
        break;
    }
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fb5d4

int scnCalcPcDamage(u_int target, int word)
{
    u_int pcOid;
    g_gamMan.getPcParam(1, &pcOid);
    return scnCalcDamage(pcOid, target, word);
}

// ──────────────────────────────────────────────────────────────────────
// The attack word's percent of the attacker's ST (scnCalcDamage's switch,
// its own jump table): fixed for some words, else a random range.
static int scnAttackPercent(int word)
{
    int lo, hi;

    switch (word)
    {
    case 1: case 2:             lo = 5;   hi = 20;  break;
    case 3:                     lo = 130; hi = 170; break;
    case 4:                     return utlRandMean(90, 110, 3);
    case 5:                     return utlRandPeak(70, 160, 100, 8);
    case 6: case 0xc: case 0x15: case 0x16:
                                return 0;
    case 7:                     lo = 200; hi = 250; break;
    case 8:                     lo = 60;  hi = 140; break;
    case 9:                     lo = 80;  hi = 160; break;
    case 0xb:                   return utlRandPeak(100, 400, 300, 8);
    case 0xd:                   lo = 80;  hi = 90;  break;
    case 0xf: case 0x10: case 0x21: case 0x2e: case 0x33:
                                lo = 80;  hi = 120; break;
    case 0x11:                  lo = 75;  hi = 85;  break;
    case 0x13:                  lo = 90;  hi = 110; break;
    case 0x14:                  lo = 140; hi = 160; break;
    case 0x17:                  lo = 80;  hi = 150; break;
    case 0x18:                  lo = 200; hi = 260; break;
    case 0x1a:                  return 150;
    case 0x1b:                  return 400;
    case 0x1c:                  return 75;
    case 0x1d: case 0x1f:       lo = 100; hi = 120; break;
    case 0x1e: case 0x20:       lo = 80;  hi = 90;  break;
    case 0x22: case 0x2b:       lo = 90;  hi = 100; break;
    case 0x23:                  lo = 180; hi = 220; break;
    case 0x24: case 0x2c:       lo = 70;  hi = 80;  break;
    case 0x25: case 0x2d:       return 200;
    case 0x26:                  lo = 90;  hi = 110; break;
    case 0x27:                  lo = 90;  hi = 120; break;
    case 0x28:                  lo = 150; hi = 180; break;
    case 0x29: case 0x2a:       lo = 60;  hi = 70;  break;
    case 0x2f: case 0x30:       lo = 40;  hi = 100; break;
    case 0x31:                  lo = 60;  hi = 80;  break;
    case 0x32:                  lo = 30;  hi = 40;  break;
    default:                    return 100;
    }
    return utlRandRange(lo, hi);
}

// The weapon's fields: 0x2a the bonus, 0x2b the multiplier. The PC's is
// its equipped item (none for 0xa00 or word 7), a kind-2 object's its
// param 0x66 (none for 0 / 0xa00).
// callcheck: utlRandMean, utlRandPeak (in scnAttackPercent, the switch split out)
// ROR 0x001fb624

int scnCalcDamage(u_int attacker, u_int target, int word)
{
    GamSysData &sys = g_gamMan.m_sysData;
    short targetKind, attackerKind;
    u_short st = 0, def = 0;
    short weapon;
    int bonus = 0;
    float mult = 1.0f;

    g_gamMan.getObjParam(target, 2, &targetKind);
    g_gamMan.getObjParam(attacker, 2, &attackerKind);   /* dropped */
    (void)attackerKind;
    switch ((attacker >> 23) & 0x1f)
    {
    case 0:
        g_gamMan.getPcParam(0x1d, &st);
        weapon = g_gamMan.m_equipItem;
        if (weapon != 0xa00 && word != 7)
        {
            sys.getField(weapon, 0x2a, &bonus);
            sys.getField(weapon, 0x2b, &mult);
        }
        break;
    case 2:
        g_gamMan.getObjParam(attacker, 0x1d, &st);
        g_gamMan.getObjParam(attacker, 0x66, &weapon);
        if (weapon != 0xa00 && weapon != 0)
        {
            sys.getField(weapon, 0x2a, &bonus);
            sys.getField(weapon, 0x2b, &mult);
        }
        break;
    case 5:
    {
        short kind;
        g_gamMan.getObjParam(attacker, 2, &kind);
        sys.getField(kind, 0x2a, &bonus);
        sys.getField(kind, 0x2b, &mult);
        int r = utlRandRange(70, 100);
        return (int)(mult * (float)r / 100.0f) * bonus;
    }
    default:
        g_gamMan.getObjParam(attacker, 0x1d, &st);
        break;
    }

    sys.getField((u_short)targetKind, 0x1e, &def);
    int r = utlRandRange(90, 110);
    float pct = (float)scnAttackPercent(word);
    int damage = (int)((float)st * pct / 100.0f * mult) + bonus - (int)((float)def * (float)r / 100.0f);
    return damage < 0 ? 0 : damage;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fbb04

int TskDamage::endReact()
{
    if ((m_flags & 8) == 0)
        return 0;
    m_achr->m_flags &= ~0x200u;
    if (m_frames < 10000)
    {
        g_emyManager.pauseEmy(m_oid, 0);
        g_emyManager.setEmyMode6(m_oid);
    }
    else
    {
        g_emyManager.stopEmyMove(m_oid);
        m_achr->killTasks();
    }
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fbba4

int scnGetMaxFreeMem(void)
{
    BEDMEMINFO info;
    g_utlMemory.getInfo(&info);   // the ROM passes a third word 0
    return info.m_max_free;
}

// ══════════════════════════════════════════════════════════════════════
//  TskCamera
// ══════════════════════════════════════════════════════════════════════

/* setupFace's offset of the eye from object 0, turned by its heading
 * (data 0x2fefc0). */
static const sceVu0FVECTOR s_faceEyeOfs = { 30.0f, 0.0f, 0.0f, 1.0f };

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fbc0c

TskCamera::TskCamera(intptr_t kind, intptr_t param)
{
    m_step = &TskCamera::stepNone;   /* PC: the ROM leaves it unset for kinds above 1 */
    m_kind = (int)kind;
    if (kind == 0)
        setupPad((const int *)param);
    else if (kind == 1)
        setupFace((const int *)param);
}

// ──────────────────────────────────────────────────────────────────────
// Kind 0: the centre, limits and speed from the script's words; the
// angles and the drift start at 0.
// ROR 0x001fbc6c

void TskCamera::setupPad(const int *param)
{
    const float *f = (const float *)param;
    m_0c = 0;
    m_step = &TskCamera::stepNone;
    m_center[0] = f[1];
    m_center[1] = f[2];
    m_center[2] = f[3];
    m_center[3] = 0.0f;
    m_min[0] = f[4];
    m_min[1] = f[5];
    m_min[2] = f[6];
    m_min[3] = 0.0f;
    m_max[0] = f[7];
    m_max[1] = f[8];
    m_max[2] = f[9];
    m_max[3] = 0.0f;
    m_speed = f[10];
    for (int i = 0; i < 4; i++)
    {
        m_angle[i] = 0.0f;
        m_angleSmooth[i] = 0.0f;
        m_drift[i] = 0.0f;
        m_driftSmooth[i] = 0.0f;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Kind 1: object a turns to face b (and b to face a with param[2]); the
// camera (-0.2 pitch, mode 1) goes 30 to object a's side and 70 behind
// on the line from b's head (160 up) -- 15 nearer b when the scene is
// hit on the way -- and looks at b's head.
// ROR 0x001fbd2c

void TskCamera::setupFace(const int *param)
{
    GamMan &gam = g_gamMan;
    u_int a = (u_int)param[0];
    int both = param[2];
    u_int b = (u_int)param[1];
    sceVu0FVECTOR dir, tmp, look, eye, posB, posA, rotA, rot, eye2;

    gam.getObjPos(a, eye);
    gam.getObjPos(b, posB);
    sceVu0SubVector(dir, posB, eye);
    gam.getObjRot(a, rot);
    rot[1] = atan2f(dir[0], dir[2]);
    gam.setObjRot(a, rot);
    if (both != 0)
    {
        gam.getObjRot(b, rot);
        rot[1] = atan2f(dir[0], dir[2]) + 0x1.921faep+1f; /* 0x40490fd7 */
        gam.setObjRot(b, rot);
    }

    memset(rot, 0, sizeof(rot));
    rot[0] = -0x1.999998p-3f; /* 0xbe4ccccc (-0.2) */
    gam.setCameraMode(1, 0);  // the ROM passes a third word 0

    gam.getObjPos(a, posA);
    gam.getObjRot(a, rotA);
    float ry = rotA[1];
    sceVu0FVECTOR ofs;
    for (int i = 0; i < 4; i++)
        ofs[i] = s_faceEyeOfs[i];
    eye[0] = cosf(ry) * ofs[0] + sinf(ry) * ofs[2] + posA[0];
    eye[1] = ofs[1] + posA[1];
    eye[2] = -sinf(ry) * ofs[0] + cosf(ry) * ofs[2] + posA[2];

    gam.getObjPos(b, posB);
    posB[1] -= 160.0f;
    eye[1] -= 160.0f;
    sceVu0SubVector(dir, eye, posB);
    sceVu0ScaleVector(tmp, dir, 0.0f);
    sceVu0SubVector(posB, posB, tmp);
    sceVu0Normalize(tmp, dir);
    sceVu0ScaleVector(tmp, tmp, -70.0f);
    sceVu0SubVector(eye2, eye, tmp);
    sceVu0SubVector(look, posB, eye2);

    MagSceneRayHit hit;
    for (int i = 0; i < 5; i++)
    {
        hit.m_vec[i][0] = 0.0f;
        hit.m_vec[i][1] = 0.0f;
        hit.m_vec[i][2] = 0.0f;
        hit.m_vec[i][3] = 1.0f;
    }
    hit.m_50 = -1;
    sceVu0FVECTOR from, along;
    for (int i = 0; i < 4; i++)
    {
        from[i] = eye2[i];
        along[i] = tmp[i];
    }
    float t = g_magSystem.m_scene.castRay(&hit, from, along, 0);
    if (0.0f <= t && t <= 1.0f)
    {
        sceVu0Normalize(tmp, look);
        sceVu0ScaleVector(tmp, tmp, 15.0f);
        sceVu0AddVector(eye2, eye2, tmp);
    }

    DogCamera &cam = g_magSystem.m_scene.m_camera_entity;
    cam.m_trans[0] = eye2[0];
    cam.m_trans[2] = eye2[2];
    cam.m_dirty = 1;
    cam.m_trans[1] = eye2[1];
    rot[1] = atan2f(look[0], look[2]);
    cam.m_rotate[1] = rot[1];
    cam.m_rotate[0] = rot[0];
    cam.m_rotate[2] = rot[2];
    cam.normalizeRotate();
    cam.m_dirty = 1;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc164

void TskCamera::init()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc16c

void TskCamera::term()
{
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc174

int TskCamera::exec(int id, int time, int arg)
{
    (void)id;
    (void)time;
    (void)arg;
    if (m_kind == 0)
        return execPad();
    if (m_kind == 1)
        return execFace();
    return (this->*m_step)();
}

// ──────────────────────────────────────────────────────────────────────
// Kind 0, every frame: the left stick moves the angle (up / down on x,
// left / right on y) within the limits, a random drift of +-0.005 a
// frame decays by 0.9; both are smoothed and added to the centre as the
// scene camera's rotation. Never ends.
// ROR 0x001fc210

int TskCamera::execPad()
{
    BedPad &pad = g_magSystem.m_pad[0];
    float ly = pad.getLeftY();
    float lx = pad.getLeftX();

    float x = m_angle[0] - ly * m_speed;
    float cx = m_min[0];
    if (cx <= x)
    {
        cx = m_max[0];
        if (x <= m_max[0])
            cx = x;
    }
    float y = m_angle[1] + lx * m_speed;
    m_angle[0] = cx;
    float cy = m_min[1];
    if (cy <= y)
    {
        cy = m_max[1];
        if (y <= m_max[1])
            cy = y;
    }
    m_angle[1] = cy;

    m_drift[0] += ((float)RosePan_Rand() / ROSEPAN_RAND_MAXF - 0.5f) * 0x1.47ae14p-7f; /* 0x3c23d70a */  // callcheck: rand (RosePan_Rand keeps the ROM 31-bit range)
    m_drift[1] += ((float)RosePan_Rand() / ROSEPAN_RAND_MAXF - 0.5f) * 0x1.47ae14p-7f;

    sceVu0FVECTOR a, b;
    sceVu0ScaleVectorXYZ(m_drift, m_drift, 0.9f);                 /* 0x3f666666 */
    sceVu0ScaleVectorXYZ(a, m_driftSmooth, 0x1.b33332p-1f);       /* 0x3f599999 */
    sceVu0ScaleVectorXYZ(b, m_drift, 0x1.999998p-4f);             /* 0x3dcccccc */
    sceVu0AddVector(m_driftSmooth, a, b);
    sceVu0ScaleVectorXYZ(a, m_angleSmooth, 0.9f);
    sceVu0ScaleVectorXYZ(b, m_angle, 0x1.999998p-4f);
    sceVu0AddVector(m_angleSmooth, a, b);
    sceVu0AddVector(a, m_center, m_angleSmooth);
    sceVu0AddVector(a, a, m_driftSmooth);

    DogCamera &cam = g_magSystem.m_scene.m_camera_entity;
    cam.m_rotate[0] = a[0];
    cam.m_rotate[1] = a[1];
    cam.m_rotate[2] = a[2];
    cam.normalizeRotate();
    cam.m_dirty = 1;
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// Kind 1: the camera stays where setupFace put it. Never ends.
// ROR 0x001fc46c

int TskCamera::execFace()
{
    return 0;
}

// ──────────────────────────────────────────────────────────────────────
// ROR 0x001fc474

int TskCamera::stepNone()
{
    return 0;
}

// ══════════════════════════════════════════════════════════════════════
//  GamMan's load hooks
// ══════════════════════════════════════════════════════════════════════

// ──────────────────────────────────────────────────────────────────────
// Empty in the ROM.
// ROR 0x001f8fb8

void gamSystemDataLoaded(void)
{
}

// ──────────────────────────────────────────────────────────────────────
// The heap's free memory after a stage load, for the debug print
// ("free mem   max/total/size   = %d/%d"; rorDebugPrintf is empty).
// ROR 0x001fbbd0 // callcheck: rorDebugPrintf (the empty debug print, left out)

void gamStage2Loaded(void)
{
    BEDMEMINFO info;
    g_utlMemory.getInfo(&info);   // the ROM passes a third word 0
    (void)info;
}
