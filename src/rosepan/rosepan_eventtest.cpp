/* rosepan/rosepan_eventtest.cpp -- see rosepan_eventtest.h. */

#include "rosepan/rosepan_eventtest.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>

#include "bed/bedpfh.h"
#include "game/gamman.h"
#include "game/gamstat.h"
#include "game/gamtrg.h"
#include "mag/magsystem.h"
#include "rosepan/io/rosepan_log.h"
#include "rosepan/io/rosepan_paths.h"
#include "sequence/seqcall.h"
#include "sequence/sequence.h"
#include "task/naga_chunk.h"
#include "task/tsk_man.h"
#include "task/tsk_scene.h"

/* ---- the chapters ---------------------------------------------------- */

/* data 0x2db2c0 / 0x2db2f8, the developers' chapter page (naga_debug.cpp),
 * without its title row. */
static RosePanEventChapter s_chapters[] = {
    { 1, "Chapter 1", false, {}, {}, {}, {} },
    { 2, "Chapter 2", false, {}, {}, {}, {} },
    { 3, "Chapter 3", false, {}, {}, {}, {} },
    { 4, "Chapter 4", false, {}, {}, {}, {} },
    { 5, "Chapter 5a", false, {}, {}, {}, {} },
    { 13, "Chapter 5b", false, {}, {}, {}, {} },
    { 6, "Chapter 6", false, {}, {}, {}, {} },
    { 7, "Chapter 7", false, {}, {}, {}, {} },
    { 8, "Chapter 8", false, {}, {}, {}, {} },
    { 10, "Chapter 10", false, {}, {}, {}, {} },
    { 11, "Chapter 11", false, {}, {}, {}, {} },
    { 12, "Chapter 12", false, {}, {}, {}, {} },
};
static const int kChapterCount = (int)(sizeof(s_chapters) / sizeof(s_chapters[0]));

/* The disc's room folders are numbered below this. */
static const int kMaxRoom = 0x100;

/* ---- file reads ------------------------------------------------------ */

typedef std::vector<unsigned char> Bytes;

static uint32_t Word(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static FILE *OpenData(const char *name)
{
    char path[1024];
    if (!RosePan_PathResolveData(name, path, sizeof(path)))
        return NULL;
    return fopen(path, "rb");
}

static bool ReadAt(FILE *f, long offset, void *out, size_t size)
{
    return fseek(f, offset, SEEK_SET) == 0 && fread(out, 1, size, f) == size;
}

/* A pack's file table (bed/bedpfh.h); the rooms' packs are too big to read
 * whole just for their scripts. */
struct PackFile
{
    uint32_t offset;
    uint32_t size;
};

static bool ReadPackTable(FILE *f, std::vector<PackFile> *files)
{
    unsigned char head[0x20];
    if (!ReadAt(f, 0, head, sizeof(head)) || memcmp(head, "RTPK", 4) != 0)
        return false;
    const unsigned flags = (unsigned)head[0xa] | ((unsigned)head[0xb] << 8);
    const unsigned count = (unsigned)head[0xe] | ((unsigned)head[0xf] << 8);
    const uint32_t total = Word(head + 4);
    if (count == 0 || count > 0x1000)
        return false;

    long table = 0x20;
    Bytes sizes(count * 4), offsets(count * 4);
    if ((flags & 1) != 0) {
        if (!ReadAt(f, table, sizes.data(), sizes.size()))
            return false;
        table += (long)sizes.size();
    }
    if ((flags & 2) == 0 || !ReadAt(f, table, offsets.data(), offsets.size()))
        return false;

    files->resize(count);
    for (unsigned i = 0; i < count; i++) {
        (*files)[i].offset = Word(&offsets[i * 4]);
        if ((flags & 1) != 0)
            (*files)[i].size = Word(&sizes[i * 4]);
        else if (i + 1 < count && Word(&offsets[(i + 1) * 4]) != 0)
            (*files)[i].size = Word(&offsets[(i + 1) * 4]) - (*files)[i].offset;
        else
            (*files)[i].size = total - (*files)[i].offset;
    }
    return true;
}

static bool ReadPackFile(FILE *f, const PackFile &file, Bytes *out)
{
    /* A sanity limit: no script or event pack comes near it. */
    if (file.size == 0 || file.size > 0x800000)
        return false;
    out->resize(file.size);
    return ReadAt(f, (long)file.offset, out->data(), out->size());
}

/* The pack's script image: its file that starts "ELFR". */
static bool ReadScriptImage(FILE *f, const std::vector<PackFile> &files, Bytes *image)
{
    for (const PackFile &file : files) {
        char magic[4];
        if (!ReadAt(f, (long)file.offset, magic, sizeof(magic)))
            return false;
        if (memcmp(magic, "ELFR", 4) == 0)
            return ReadPackFile(f, file, image);
    }
    return false;
}

/* ---- the script scanner ---------------------------------------------- */

/* A script image (architecture.md "The sequence script VM"): MIPS code at
 * +0x30 linked at 0x8160, the data at [+0x14] linked at [+0x24]. The pass
 * follows the compiler's fixed shapes: `lui r, hi` + `addiu r, r, lo` loads
 * a string, and an event scene's string is loaded in the function that runs
 * it. Nothing here executes the script. */
struct ScriptImage
{
    const unsigned char *data;
    size_t   size;
    uint32_t data_off;
    uint32_t data_link;
    uint32_t words;

    bool Open(const Bytes &image)
    {
        if (image.size() < 0x30 || memcmp(image.data(), "ELFR", 4) != 0)
            return false;
        data = image.data();
        size = image.size();
        data_off = Word(data + 0x14);
        data_link = Word(data + 0x24);
        if (data_off < 0x30 || data_off > size)
            return false;
        words = (data_off - 0x30) / 4;
        return true;
    }

    uint32_t Code(uint32_t i) const { return Word(data + 0x30 + i * 4); }
    static uint32_t Address(uint32_t i) { return 0x8160 + i * 4; }

    /* The C string at link address addr; empty when it is not one. */
    std::string StringAt(uint32_t addr) const
    {
        if (addr < data_link)
            return std::string();
        const uint64_t off = (uint64_t)addr - data_link + data_off;
        if (off >= size)
            return std::string();
        const char *s = (const char *)data + off;
        const size_t max = size - (size_t)off;
        const size_t len = strnlen(s, max);
        if (len < 2 || len == max || len > 0x100)
            return std::string();
        for (size_t i = 0; i < len; i++) {
            const unsigned char c = (unsigned char)s[i];
            if ((c < 0x20 || c >= 0x7f) && c != '\n')
                return std::string();
        }
        return std::string(s, len);
    }
};

/* The fields of the instruction words the pass reads. */
static inline uint32_t Op(uint32_t w) { return w >> 26; }
static inline uint32_t Rs(uint32_t w) { return (w >> 21) & 31; }
static inline uint32_t Rt(uint32_t w) { return (w >> 16) & 31; }
static inline int Imm(uint32_t w) { return (int)(short)(w & 0xffff); }

static const uint32_t kOpAddiu = 0x09;
static const uint32_t kOpLui = 0x0f;
static const uint32_t kOpJal = 0x03;
static const uint32_t kOpSw = 0x2b;

/* `addiu sp, sp, -n`: what a function opens with (the VM's seqFindEntry
 * looks for the same word). */
static bool IsPrologue(uint32_t w)
{
    return (w & 0xffff8000) == 0x27bd8000;
}

/* A jal to native n (target 0x144 + n * 0x40, 16 bits kept). */
static bool IsNative(uint32_t w, int n)
{
    return Op(w) == kOpJal && ((w << 2) & 0xffff) == (uint32_t)(0x144 + n * 0x40);
}

static void TrimPrint(std::string *s)
{
    while (!s->empty() && (s->back() == '\n' || s->back() == ' '))
        s->pop_back();
}

static bool StartsWith(const std::string &s, const char *prefix)
{
    return s.compare(0, strlen(prefix), prefix) == 0;
}

static bool EndsWith(const std::string &s, const char *suffix)
{
    const size_t n = strlen(suffix);
    return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
}

static std::string Lower(std::string s)
{
    for (char &c : s)
        c = (char)tolower((unsigned char)c);
    return s;
}

/* After a "[Set Step] NAME" print: `addiu v1, v0, 0x9c` (global 0x27), a
 * constant and the store. Returns the constant, -1 when the shape differs. */
static int ScanStepValue(const ScriptImage &img, uint32_t at)
{
    int reg = -1;
    int value = -1;
    int value_reg = -1;
    const uint32_t end = std::min(img.words, at + 16);
    for (uint32_t i = at; i < end; i++) {
        const uint32_t w = img.Code(i);
        if (Op(w) == kOpAddiu && Imm(w) == 0x9c && Rs(w) != 0) {
            reg = (int)Rt(w);
        } else if (Op(w) == kOpAddiu && Rs(w) == 0) {
            value = Imm(w);
            value_reg = (int)Rt(w);
        } else if (Op(w) == kOpSw && reg >= 0 && (int)Rs(w) == reg && Imm(w) == 0 &&
                   (int)Rt(w) == value_reg) {
            return value;
        }
    }
    return -1;
}

/* Before a "[Set Flag] NAME on" print: native 21 op 7 / 8 (set / clear a
 * sequence bit) with the bit in a1. Returns the bit, -1 when the print
 * belongs to another kind of write. */
static int ScanFlagBit(const ScriptImage &img, uint32_t at)
{
    const uint32_t lo = at > 12 ? at - 12 : 0;
    for (uint32_t call = at; call-- > lo;) {
        const uint32_t w = img.Code(call);
        if (Op(w) != kOpJal)
            continue;
        if (!IsNative(w, 21))
            return -1;
        int op = -1;
        int bit = -1;
        for (uint32_t i = call; i-- > lo;) {
            const uint32_t arg = img.Code(i);
            if (Op(arg) == kOpJal)
                break;
            if (Op(arg) == kOpAddiu && Rs(arg) == 0 && Rt(arg) == 4 && op < 0)
                op = Imm(arg);
            else if (Op(arg) == kOpAddiu && Rs(arg) == 0 && Rt(arg) == 5 && bit < 0)
                bit = Imm(arg);
        }
        return op == 7 || op == 8 ? bit : -1;
    }
    return -1;
}

/* Drops the message window's codes: "#F020", "#w120", "#R", "#n", "#K003". */
static std::string PlainText(const char *s)
{
    std::string out;
    for (; *s != '\0'; s++) {
        if (*s != '#') {
            out.push_back(*s);
            continue;
        }
        s++;
        if (*s == '\0')
            break;
        if (*s == 'n')
            out.push_back(' ');
        while (s[1] >= '0' && s[1] <= '9')
            s++;
    }
    return out;
}

/* One image's names into the chapter. room -1: the chapter's own image. */
static void ScanImage(const Bytes &bytes, int room, RosePanEventChapter *chapter,
                      std::string *area_name, std::map<int, std::string> *steps,
                      std::map<int, std::string> *flags)
{
    ScriptImage img;
    if (!img.Open(bytes))
        return;

    int64_t high[32];
    for (int i = 0; i < 32; i++)
        high[i] = -1;

    uint32_t func = 0;
    std::set<std::string> seen;     /* one record per event and function */
    std::map<uint32_t, std::string> func_text;
    const size_t first_event = chapter->events.size();
    for (uint32_t i = 0; i < img.words; i++) {
        const uint32_t w = img.Code(i);
        if (IsPrologue(w)) {
            func = i;
            seen.clear();
        }
        if (Op(w) == kOpLui) {
            high[Rt(w)] = (int64_t)(w & 0xffff) << 16;
            continue;
        }
        if (Op(w) != kOpAddiu || Rs(w) != Rt(w) || high[Rs(w)] < 0)
            continue;
        const uint32_t addr = (uint32_t)(high[Rs(w)] + Imm(w));
        high[Rs(w)] = -1;

        std::string s = img.StringAt(addr);
        if (s.empty())
            continue;
        TrimPrint(&s);

        if (StartsWith(s, "[Area init] ")) {
            *area_name = s.substr(12);
        } else if (StartsWith(s, "[Set Step] ")) {
            const int value = ScanStepValue(img, i);
            if (value >= 0)
                (*steps)[value] = s.substr(11);
        } else if (StartsWith(s, "[Set Flag] ")) {
            const size_t end = s.rfind(' ');
            const int bit = ScanFlagBit(img, i);
            if (bit >= 0 && end > 11)
                (*flags)[bit] = s.substr(11, end - 11);
        } else if (EndsWith(s, ".scn") || EndsWith(s, ".PSS")) {
            if (!seen.insert(s).second)
                continue;
            RosePanEvent event;
            event.movie = EndsWith(s, ".PSS");
            event.name = event.movie ? s : s.substr(0, s.size() - 4);
            event.room = room;
            event.image = room < 0 ? 1 : 2;
            event.entry = ScriptImage::Address(func);
            event.frames = 0;
            event.has_scene = false;
            chapter->events.push_back(event);
        } else if (s[0] == '#') {
            /* A message window's text: says what the event is better than
             * its name does, when its scene has none of its own. */
            func_text.emplace(ScriptImage::Address(func), PlainText(s.c_str()));
        }
    }
    for (size_t i = first_event; i < chapter->events.size(); i++) {
        const auto it = func_text.find(chapter->events[i].entry);
        if (it != func_text.end())
            chapter->events[i].text = it->second;
    }
}

/* What the stage's event pack says about a scene (task/tsk_scene.h): its
 * length, its room's name and its first line of text. The pack is W000.WRD
 * file 4, a pack of "name/name" packs whose file 0 is a chunk table. */
struct SceneInfo
{
    int         frames;
    std::string map;
    std::string text;
};

/* False when the pack does not hold the scene. */
static bool ReadSceneInfo(const unsigned char *pack, const std::string &name, SceneInfo *info)
{
    info->frames = 0;
    info->map.clear();
    info->text.clear();
    const std::string path = name + "/" + name;
    BedPfh evt(pack);
    const int n = evt.getFileIndex(path.c_str());
    if (n < 0)
        return false;
    BedPfh scene(evt.getFile(n));
    NagaChunk chunk(scene.getFile(0));
    if (chunk.find("frame"))
        info->frames = chunk.getInt();
    chunk.rewind();
    /* "map" is a room name in the file; scenePlayStart overwrites it with
     * 0xcccc, but only in the game's own copy of the pack. */
    if (chunk.find("map"))
        info->map = (const char *)chunk.getData();
    chunk.rewind();
    if (chunk.find("MESSAGE")) {
        NagaChunk message(chunk.getData());
        if (message.find("strings"))
            info->text = PlainText((const char *)message.getData());
    }
    return true;
}

/* Fills in the events the scripts named, and adds the scenes they did not:
 * the developers' leftovers, or ones reached in a way the scan does not
 * follow. Those still play by themselves. */
static void AddScenes(const Bytes &pack, RosePanEventChapter *chapter)
{
    std::map<std::string, int> room_by_name;
    for (const RosePanEventRoom &room : chapter->rooms)
        room_by_name[Lower(room.name)] = room.room;

    SceneInfo info;
    std::set<std::string> known;
    for (RosePanEvent &event : chapter->events) {
        known.insert(event.name);
        if (event.movie)
            continue;
        event.has_scene = ReadSceneInfo(pack.data(), event.name, &info);
        event.frames = info.frames;
        if (!info.text.empty())
            event.text = info.text;
    }

    BedPfh evt(pack.data());
    const int count = evt.getNumFile();
    for (int i = 0; i < count; i++) {
        std::string name = evt.getFileName(i);
        const size_t slash = name.find('/');
        if (slash != std::string::npos)
            name.resize(slash);
        if (known.count(name) != 0)
            continue;
        ReadSceneInfo(pack.data(), name, &info);
        RosePanEvent event;
        event.name = name;
        event.room = -1;
        event.image = 0;
        event.entry = 0;
        event.movie = false;
        event.has_scene = true;
        event.frames = info.frames;
        event.text = info.text;
        const std::string map = Lower(info.map);
        if (room_by_name.count(map) != 0)
            event.room = room_by_name[map];
        chapter->events.push_back(event);
    }
}

static void ScanChapter(RosePanEventChapter *chapter)
{
    chapter->scanned = true;
    std::map<int, std::string> steps;
    std::map<int, std::string> flags;
    Bytes scenes;
    char name[0x40];

    snprintf(name, sizeof(name), "data/SEQ/W%03d/W000.WRD", chapter->stage);
    FILE *f = OpenData(name);
    if (f != NULL) {
        std::vector<PackFile> files;
        Bytes bytes;
        if (ReadPackTable(f, &files)) {
            std::string unused;
            if (ReadScriptImage(f, files, &bytes))
                ScanImage(bytes, -1, chapter, &unused, &steps, &flags);
            if (files.size() > 4)
                ReadPackFile(f, files[4], &scenes);
        }
        fclose(f);
    }

    for (int room = 0; room < kMaxRoom; room++) {
        snprintf(name, sizeof(name), "data/SEQ/W%03d/A%03d/A%03d.ARA", chapter->stage, room, room);
        f = OpenData(name);
        if (f == NULL)
            continue;
        std::vector<PackFile> files;
        Bytes bytes;
        std::string area;
        if (ReadPackTable(f, &files) && ReadScriptImage(f, files, &bytes)) {
            ScanImage(bytes, room, chapter, &area, &steps, &flags);
            RosePanEventRoom rec;
            rec.room = room;
            rec.name = area;
            chapter->rooms.push_back(rec);
        }
        fclose(f);
    }

    for (const auto &it : steps) {
        RosePanEventStep step;
        step.name = it.second;
        step.value = it.first;
        chapter->steps.push_back(step);
    }
    for (const auto &it : flags) {
        RosePanEventFlag flag;
        flag.name = it.second;
        flag.bit = it.first;
        chapter->flags.push_back(flag);
    }
    if (scenes.size() > 0x20 && memcmp(scenes.data(), "RTPK", 4) == 0)
        AddScenes(scenes, chapter);
    /* Story order is unknown; by name reads best, and the rooms of one
     * event sort together. */
    std::stable_sort(chapter->events.begin(), chapter->events.end(),
                     [](const RosePanEvent &a, const RosePanEvent &b) { return a.name < b.name; });
    RosePan_LogPrintf("event test: %s (W%03d): %d rooms, %d events, %d steps, %d flags\n",
                      chapter->label, chapter->stage, (int)chapter->rooms.size(),
                      (int)chapter->events.size(), (int)chapter->steps.size(),
                      (int)chapter->flags.size());
}

int RosePanEventTest_ChapterCount(void)
{
    return kChapterCount;
}

const char *RosePanEventTest_ChapterLabel(int index)
{
    return index >= 0 && index < kChapterCount ? s_chapters[index].label : "";
}

int RosePanEventTest_ChapterOfStage(int stage)
{
    for (int i = 0; i < kChapterCount; i++) {
        if (s_chapters[i].stage == stage)
            return i;
    }
    return -1;
}

const RosePanEventChapter *RosePanEventTest_Chapter(int index)
{
    if (index < 0 || index >= kChapterCount)
        return NULL;
    /* The UI's thread and a scripted run's tick can both get here. */
    static std::mutex s_scan_lock;
    std::lock_guard<std::mutex> guard(s_scan_lock);
    if (!s_chapters[index].scanned)
        ScanChapter(&s_chapters[index]);
    return &s_chapters[index];
}

/* ---- the jump -------------------------------------------------------- */

/* The stage the title runs on, the room script's main thread, and the id
 * of the thread a jump starts (the image in the top two bits comes on top;
 * a number no script uses). */
static const int kTitleStage = 99;
static const int kFlagBits = 0x800 * 8;     /* m_bits is 0x800 bytes */
static const u_int kRoomMainThread = 0x84000000;
static const u_int kThreadNumber = 0x04fffff0;

/* Frames with the room up and the PC free before an event starts; frames
 * an event waits for that before it takes the room over, and frames a jump
 * waits at all. */
static const int kSettleFrames = 30;
static const int kBusyRoomFrames = 300;
static const int kWaitLimit = 900;

enum Phase
{
    PHASE_IDLE,
    PHASE_CHAPTER,      /* the chapter loads and its script reaches ac_jump_area */
    PHASE_ROOM,         /* the room loads and its script settles */
};

/* The queue, between the caller's thread and the tick. */
static std::mutex s_lock;
static bool s_pending;
static RosePanEventJump s_request;
static bool s_cancel;
static int s_flag_bit = -1;
static int s_flag_value;

/* The tick's own, the game thread's. */
static Phase s_phase = PHASE_IDLE;
static RosePanEventJump s_jump;
static int s_settle;
static int s_wait;
static bool s_placed;

static RosePanEventStatus s_status;

static void SetStatus(const char *fmt, const char *arg)
{
    snprintf(s_status.text, sizeof(s_status.text), fmt, arg);
    RosePan_LogPrintf("event test: %s\n", s_status.text);
}

void RosePanEventTest_Request(const RosePanEventJump &jump)
{
    std::lock_guard<std::mutex> guard(s_lock);
    s_request = jump;
    s_pending = true;
    s_cancel = false;
}

void RosePanEventTest_Cancel(void)
{
    std::lock_guard<std::mutex> guard(s_lock);
    s_pending = false;
    s_cancel = true;
}

void RosePanEventTest_SetFlag(int bit, int value)
{
    std::lock_guard<std::mutex> guard(s_lock);
    s_flag_bit = bit;
    s_flag_value = value;
}

int RosePanEventTest_GetFlag(int bit)
{
    if (bit < 0 || bit >= kFlagBits)
        return 0;
    return (g_sequence.m_bits[bit >> 5] >> (bit & 31)) & 1;
}

void RosePanEventTest_GetStatus(RosePanEventStatus *status)
{
    *status = s_status;
    status->stage = g_gamMan.m_chapter;
    status->room = g_gamMan.m_wrdNo;
    status->step = g_sequence.m_global[0x27];
    status->busy = s_pending || s_phase != PHASE_IDLE;
}

/* The room's own script has finished: its main thread has been and gone
 * (it waits on the start list for a frame), no stage change waits and the
 * PC exists. */
static bool RoomLoaded()
{
    if ((g_gamMan.m_flags2 & 3) != 0 || g_gamMan.m_pcPtr == NULL)
        return false;
    if (g_sequence.m_start.m_used.m_count != 0)
        return false;
    SeqPool<SeqThread> *pool = g_sequence.m_threads[2];
    for (SeqNode<SeqThread> *it = pool->m_used.begin(); it != pool->m_used.end(); it = it->m_next) {
        if (it->m_data.m_id == kRoomMainThread)
            return false;
    }
    return true;
}

/* The scripts have let go of the PC: the running bits an event's
 * [unplayable] clears. */
static bool PcFree()
{
    return (g_gamMan.m_flags & 6) == 6;
}

/* Runs the camera trigger under the PC, which is what picks the room's
 * camera; true when one did (mode 0 is the scripts' own free camera). */
static bool PickCamera()
{
    g_gamMan.m_area.checkPc();
    return g_magSystem.m_scene.m_camera_mode != 0;
}

/* Stands the PC at pos, on the floor there, and tries for a camera. */
static bool TryPosition(float x, float z)
{
    GamMan &gam = g_gamMan;
    float pos[4] = { x, 0.0f, z, 0.0f };
    pos[1] = gam.getFloorY(pos);
    gam.setPcPos(pos);
    gam.storePcTransform();
    return PickCamera();
}

/* The room's script has put the PC at its default spot. Most rooms have a
 * real one, but in many it is the developers' (0, 0, 50), which can be
 * outside the map: no camera trigger holds it and the screen stays on the
 * empty free camera. W000.WRD file 2 looks like it holds start positions,
 * but all but 14 of its 473 records are at (0, 0, 0). So: in front of a
 * door, as an arrival through it would be, or else the middle of a camera
 * trigger, which is somewhere the PC can stand since that is what the
 * trigger is for. */
static void PlacePc()
{
    GamMan &gam = g_gamMan;
    if (PickCamera())
        return;
    for (SeqNode<FCHRstat *> *node = gam.m_fchrList.m_used.begin();
         node != gam.m_fchrList.m_used.end(); node = node->m_next) {
        const u_int oid = node->m_data->m_oid;
        if (gamOidType(oid) != 9)
            continue;
        int param[4] = { 0, 0, 0, 0 };
        gam.getObjParam(oid, 0x41, param);
        if (gam.findFchrType9(param[0]) != node->m_data)
            continue;
        gam.m_1a684 = (u_int)param[0];
        gam.setPcStartPos();
        if (PickCamera())
            return;
    }
    SeqList<GamAreaTrg> &active = gam.m_area.m_active.m_used;
    for (SeqNode<GamAreaTrg> *it = active.begin(); it != active.end(); it = it->m_next) {
        const GamAreaTrg &trg = it->m_data;
        if ((trg.m_flags & 0x4000001) != 0x4000001 || trg.m_entry == 0)
            continue;
        if (TryPosition((trg.m_rect[0] + trg.m_rect[2]) * 0.5f,
                        (trg.m_rect[1] + trg.m_rect[3]) * 0.5f))
            return;
    }
    RosePan_LogPrintf("event test: no spot with a camera in room %d\n", gam.m_wrdNo);
}

static void StartEvent()
{
    s_phase = PHASE_IDLE;
    if (!s_jump.scene.empty()) {
        /* One -1 per THREAD of the scene: it takes each actor's model from
         * the scene when the word is no live object. */
        static int s_noOids[0x40];
        memset(s_noOids, 0xff, sizeof(s_noOids));
        SetStatus("playing scene %s", s_jump.scene.c_str());
        scenePlayStart(s_jump.scene.c_str(), (intptr_t)s_noOids, 1);
        return;
    }
    if (s_jump.entry == 0) {
        SetStatus("arrived: %s", s_jump.label.c_str());
        return;
    }
    /* The entry as the image's m_base counts it, which is the script's
     * first function (native 018 case 0x1e). */
    const u_int image = (u_int)s_jump.image;
    const intptr_t entry =
        (intptr_t)((u_int)g_sequence.m_base[image] + (s_jump.entry - 0x8160));
    g_sequence.queueThread((image << 30) | kThreadNumber, entry, 0, 0, 0, "evtest");
    SetStatus("started %s", s_jump.label.c_str());
}

/* A jump starts between stage changes (requestStage overwrites the one in
 * flight), as native 029 waits for the background loader, and with no
 * in-game menu open: a chapter change closes those itself, a room change
 * leaves them running over the new room. */
static bool CanStart()
{
    GamMan &gam = g_gamMan;
    if (gam.m_chapter == 0 || (gam.m_flags2 & 3) != 0)
        return false;
    if ((gam.m_flags & 0x800) != 0 && g_taskMan.getTask(gam.m_bgLoadTaskId) != 0)
        return false;
    /* A chapter jump under way must reach its first room first. */
    if (s_phase == PHASE_CHAPTER)
        return false;
    const bool chapter = s_request.stage >= 0 && s_request.stage != gam.m_chapter;
    return chapter || g_magSystem.m_scene.m_menu_id == -1;
}

/* The step goes in once the chapter has set its own. */
static void ApplyStep()
{
    if (s_jump.step >= 0)
        g_sequence.m_global[0x27] = s_jump.step;
}

static void Begin()
{
    s_settle = 0;
    s_wait = 0;

    if (s_jump.stage < 0 && s_jump.room < 0 && s_jump.entry == 0 && s_jump.scene.empty()) {
        /* Only the step. */
        ApplyStep();
        SetStatus("set %s", s_jump.label.c_str());
        return;
    }

    /* An event under way goes first: its scene holds actors of the room
     * being left, and its thread would go on running over the next one. */
    static const int s_sceneKinds[] = { 0xf, -1 };
    g_taskMan.killKinds(s_sceneKinds);
    g_sequence.endThread((1u << 30) | kThreadNumber, 0);
    g_sequence.endThread((2u << 30) | kThreadNumber, 0);

    if (s_jump.stage >= 0 && s_jump.stage != g_gamMan.m_chapter) {
        /* The chapter's script goes on to ac_jump_area by itself, and
         * RosePanEventTest_OverrideRoom picks the room there. */
        SetStatus("loading %s", s_jump.label.c_str());
        g_gamMan.requestStage(1, -1, -1, s_jump.stage, 0xccccccccu);
        s_phase = PHASE_CHAPTER;
        return;
    }
    ApplyStep();
    if (s_jump.room >= 0) {
        SetStatus("going to %s", s_jump.label.c_str());
        /* The room already loaded reloads too, so an event can be run again
         * from the same state; requestStage alone would take it for a menu
         * closing. */
        if (s_jump.room == g_gamMan.m_wrdNo)
            g_gamMan.m_wrdNo = -1;
        /* 0xcccccccc and no 0x20: not a door's arrival (startStage2 would
         * carry the PC over by the rooms' offsets), the room's script places
         * the PC. A chapter change clears the bit too (execStageReq). */
        g_gamMan.m_flags &= ~0x20u;
        g_gamMan.requestStage(2, s_jump.room, 0, -1, 0xccccccccu);
        s_placed = false;
        s_phase = PHASE_ROOM;
        return;
    }
    /* No room asked for: the PC stays where it stands. */
    s_placed = true;
    s_phase = PHASE_ROOM;
}

void RosePanEventTest_OverrideRoom(int *room, unsigned *word)
{
    if (s_phase != PHASE_CHAPTER)
        return;
    ApplyStep();
    s_placed = false;
    if (s_jump.room >= 0) {
        *room = s_jump.room;
        *word = 0xccccccccu;
    } else if (s_jump.entry == 0 && s_jump.scene.empty()) {
        /* Just the chapter: the rest is the chapter's own start. */
        SetStatus("arrived: %s", s_jump.label.c_str());
        s_phase = PHASE_IDLE;
        return;
    }
    s_phase = PHASE_ROOM;
}

static RosePanEventJump MakeJump(int stage, int room, const char *label)
{
    RosePanEventJump jump;
    jump.stage = stage;
    jump.room = room;
    jump.step = -1;
    jump.image = 0;
    jump.entry = 0;
    jump.label = label;
    return jump;
}

/* ---- scripted runs --------------------------------------------------- */

/* The sweep, a scripted run's bulk check: one chapter's rooms or events one
 * after the other, with what each did in the log ("sweep:" lines). */
enum SweepKind
{
    SWEEP_NONE,
    SWEEP_ROOMS,
    SWEEP_EVENTS,
};

/* Frames to look at each room (a screenshot lands in it), and the longest
 * an event runs before the next one cuts it off. */
static const int kSweepRoomFrames = 90;
static const int kSweepEventFrames = 900;

static SweepKind s_sweep;
static int s_sweep_stage;
static int s_sweep_next;
static int s_sweep_wait;

static void TickSweep()
{
    if (s_sweep == SWEEP_NONE || s_phase != PHASE_IDLE || s_pending)
        return;
    if (s_sweep_wait > 0) {
        s_sweep_wait--;
        return;
    }
    const RosePanEventChapter *chapter =
        RosePanEventTest_Chapter(RosePanEventTest_ChapterOfStage(s_sweep_stage));
    if (chapter == NULL) {
        s_sweep = SWEEP_NONE;
        return;
    }

    /* What the last one left. */
    float pos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (g_gamMan.m_pcPtr != NULL)
        g_gamMan.m_pcPtr->getPos(pos);
    RosePan_LogPrintf("sweep: left room %d pc (%.0f %.0f %.0f) camera mode %d fade %d: %s\n",
                      g_gamMan.m_wrdNo, pos[0], pos[1], pos[2], g_magSystem.m_scene.m_camera_mode,
                      g_magSystem.m_scene.getFadeState(), s_status.text);

    char label[96];
    if (s_sweep == SWEEP_ROOMS) {
        if (s_sweep_next >= (int)chapter->rooms.size()) {
            RosePan_LogPrintf("sweep: W%03d rooms finished\n", s_sweep_stage);
            s_sweep = SWEEP_NONE;
            return;
        }
        const RosePanEventRoom &room = chapter->rooms[(size_t)s_sweep_next];
        snprintf(label, sizeof(label), "A%03d %s", room.room, room.name.c_str());
        RosePan_LogPrintf("sweep: %d/%d %s\n", s_sweep_next + 1, (int)chapter->rooms.size(), label);
        RosePanEventTest_Request(MakeJump(s_sweep_stage, room.room, label));
        s_sweep_wait = kSweepRoomFrames;
    } else {
        if (s_sweep_next >= (int)chapter->events.size()) {
            RosePan_LogPrintf("sweep: W%03d events finished\n", s_sweep_stage);
            s_sweep = SWEEP_NONE;
            return;
        }
        const RosePanEvent &event = chapter->events[(size_t)s_sweep_next];
        snprintf(label, sizeof(label), "%s", event.name.c_str());
        RosePan_LogPrintf("sweep: %d/%d %s room %d (%d frames) %s\n", s_sweep_next + 1,
                          (int)chapter->events.size(), label, event.room, event.frames,
                          event.text.c_str());
        if (!event.movie && !event.has_scene) {
            RosePan_LogPrintf("sweep: %s has no scene in the pack\n", label);
            s_sweep_next++;
            return;
        }
        RosePanEventJump jump = MakeJump(s_sweep_stage, event.room, label);
        jump.image = event.image;
        jump.entry = event.entry;
        if (event.entry == 0 && !event.movie)
            jump.scene = event.name;
        RosePanEventTest_Request(jump);
        s_sweep_wait = event.frames > 0 && event.frames + 120 < kSweepEventFrames
                           ? event.frames + 120 : kSweepEventFrames;
    }
    s_sweep_next++;
}

/* ROSEPAN_EVENT_TEST for a scripted run (the smoke run's way in), read once
 * the title is up:
 *   <stage>[:<room>[:<event>]]  jumps there; "scene:" in front of the event
 *                               plays its scene without the script function
 *   rooms:<stage>[:<first>]     sweeps the chapter's rooms
 *   events:<stage>[:<first>]    sweeps its events */
static void CheckEnvironment()
{
    static int s_frames;
    static bool s_done;
    if (s_done)
        return;
    const char *env = getenv("ROSEPAN_EVENT_TEST");
    if (env == NULL || *env == '\0') {
        s_done = true;
        return;
    }
    if (g_gamMan.m_chapter != kTitleStage || ++s_frames < 120)
        return;
    s_done = true;

    if (strncmp(env, "rooms:", 6) == 0 || strncmp(env, "events:", 7) == 0) {
        s_sweep = env[0] == 'r' ? SWEEP_ROOMS : SWEEP_EVENTS;
        const char *arg = strchr(env, ':') + 1;
        s_sweep_stage = atoi(arg);
        const char *first = strchr(arg, ':');
        s_sweep_next = first != NULL ? atoi(first + 1) : 0;
        return;
    }

    RosePanEventJump jump = MakeJump(atoi(env), -1, env);
    char event[64] = "";
    const char *colon = strchr(env, ':');
    if (colon != NULL) {
        jump.room = atoi(colon + 1);
        const char *second = strchr(colon + 1, ':');
        if (second != NULL)
            snprintf(event, sizeof(event), "%s", second + 1);
    }
    if (strncmp(event, "scene:", 6) == 0) {
        jump.scene = event + 6;
    } else if (event[0] != '\0') {
        const RosePanEventChapter *chapter =
            RosePanEventTest_Chapter(RosePanEventTest_ChapterOfStage(jump.stage));
        if (chapter != NULL) {
            for (const RosePanEvent &rec : chapter->events) {
                if (rec.name == event && (jump.room < 0 || rec.room == jump.room) &&
                    (rec.movie || rec.has_scene)) {
                    jump.room = rec.room;
                    jump.image = rec.image;
                    jump.entry = rec.entry;
                    break;
                }
            }
        }
        if (jump.entry == 0)
            RosePan_LogPrintf("event test: no event %s\n", event);
    }
    RosePanEventTest_Request(jump);
}

/* ROSEPAN_EVENT_TEST_LOG: the game's state and script threads once a second. */
static void LogState()
{
    static const bool s_enabled = getenv("ROSEPAN_EVENT_TEST_LOG") != NULL;
    static int s_frame;
    if (!s_enabled || ++s_frame % 60 != 0)
        return;
    GamMan &gam = g_gamMan;
    MagScene &scene = g_magSystem.m_scene;
    float pos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (gam.m_pcPtr != NULL)
        gam.m_pcPtr->getPos(pos);
    RosePan_LogPrintf("evt: frame %d W%03d room %d step %d flags %08x flags2 %08x phase %d\n",
                      s_frame, gam.m_chapter, gam.m_wrdNo, g_sequence.m_global[0x27], gam.m_flags,
                      gam.m_flags2, (int)s_phase);
    RosePan_LogPrintf("evt:   fade %d menu %x pc (%.1f %.1f %.1f) camera mode %d (%.1f %.1f %.1f)\n",
                      scene.getFadeState(), scene.m_menu_id, pos[0], pos[1], pos[2],
                      scene.m_camera_mode, scene.m_camera_entity.m_trans[0],
                      scene.m_camera_entity.m_trans[1], scene.m_camera_entity.m_trans[2]);
    for (int image = 0; image < 3; image++) {
        SeqPool<SeqThread> *pool = g_sequence.m_threads[image];
        if (pool == NULL)
            continue;
        for (SeqNode<SeqThread> *it = pool->m_used.begin(); it != pool->m_used.end();
             it = it->m_next) {
            const SeqThread &thread = it->m_data;
            RosePan_LogPrintf("evt:   image %d id 0x%08x '%.8s' pc 0x%x wait %d\n", image,
                              thread.m_id, thread.m_name,
                              (u_int)(thread.m_pc - (intptr_t)g_sequence.m_image[image]) + 0x8130,
                              thread.m_wait);
        }
    }
}

void RosePanEventTest_Tick(void)
{
    CheckEnvironment();
    LogState();

    bool start = false;
    {
        std::lock_guard<std::mutex> guard(s_lock);
        if (s_cancel) {
            s_cancel = false;
            s_phase = PHASE_IDLE;
            SetStatus("cancelled%s", "");
        }
        if (s_flag_bit >= 0 && s_flag_bit < kFlagBits) {
            u_int *word = &g_sequence.m_bits[s_flag_bit >> 5];
            const u_int mask = 1u << (s_flag_bit & 31);
            if (s_flag_value < 0)
                *word ^= mask;
            else if (s_flag_value != 0)
                *word |= mask;
            else
                *word &= ~mask;
        }
        s_flag_bit = -1;
        if (s_pending && CanStart()) {
            s_pending = false;
            s_jump = s_request;
            start = true;
        }
    }
    if (start)
        Begin();

    if (s_phase != PHASE_IDLE && ++s_wait > kWaitLimit) {
        SetStatus("still waiting on the game's own scripts: %s", s_jump.label.c_str());
        s_phase = PHASE_IDLE;
    }
    if (s_phase == PHASE_ROOM) {
        if (s_wait > kBusyRoomFrames && (s_jump.entry != 0 || !s_jump.scene.empty())) {
            /* A room that opens with a long script of its own, as a
             * chapter's first room plays the chapter's opening, may never
             * let go of the PC: it stops, and the event takes over. */
            g_sequence.endThread(kRoomMainThread, 0);
            if (!s_placed) {
                PlacePc();
                s_placed = true;
            }
            StartEvent();
        } else if (!RoomLoaded()) {
            s_settle = 0;
        } else {
            if (!s_placed) {
                PlacePc();
                s_placed = true;
            }
            if (!PcFree())
                s_settle = 0;
            else if (++s_settle >= kSettleFrames)
                StartEvent();
        }
    }
    TickSweep();
}
