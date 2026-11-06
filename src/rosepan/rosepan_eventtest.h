/* ==========================================================================
 *  rosepan/rosepan_eventtest.h
 *
 *  The event test: jump to a chapter, a room, a story step or an event
 *  without playing up to it (the ImGui "Event test" window, rosepan_ui.cpp;
 *  ROSEPAN_EVENT_TEST for scripted runs). No ROM counterpart, though it does
 *  what the developers' own menu did: EventTestTask (debug/naga_debug.cpp)
 *  was left in the ROM with its pages, but nothing constructs it and its
 *  lists (DBGFLAG / DBGSTEP / DBGAREA.BIN) are not on the disc.
 *
 *  So the lists are read from the game's own scripts instead. Every room's
 *  A<room>.ARA and every chapter's W000.WRD holds a script image, and the
 *  retail images keep the debug prints: "[Area init] OG_GATE_F" names the
 *  room, "[Set Step] STEP_01_OPENING" and "[Set Flag] FLG_.. on" sit next to
 *  the writes that do it, and an event scene is a string "ev108.scn" handed
 *  to system call 0x46 inside the script function that runs the event.
 *  W000.WRD file 4 adds each scene's length and text. RosePanEventTest_Chapter
 *  scans one chapter's files on first use.
 *
 *  A jump never touches the game from the caller's thread: it is queued and
 *  RosePanEventTest_Tick runs it at the head of Sequence::exec, where the
 *  scripts' own stage changes run from.
 * ======================================================================== */
#ifndef ROSEPAN_EVENTTEST_H
#define ROSEPAN_EVENTTEST_H

#include <string>
#include <vector>

/* A room with a script. */
struct RosePanEventRoom
{
    int         room;           /* the room number (A<room>.ARA) */
    std::string name;           /* "OG_GATE_F", from its [Area init] print */
};

/* One script function that plays an event scene or a movie. An event used
 * from several rooms has one record per room; a scene no script names has
 * entry 0 and can only play by itself. Some scripts name a scene their
 * chapter's event pack does not hold (ev120 in chapter 1): dead code the
 * game never reaches, and scenePlayStart asserts on it, so has_scene false
 * means it must not run. */
struct RosePanEvent
{
    std::string name;           /* "ev108", or "MV01.PSS" for a movie */
    int         room;           /* -1: no room, or the chapter's own script */
    int         image;          /* the script image: 1 chapter, 2 room */
    unsigned    entry;          /* script address of the function, 0 none */
    bool        movie;
    bool        has_scene;      /* in the chapter's event pack */
    int         frames;         /* the scene's length, 0 unknown */
    std::string text;           /* its first line of text, or the script
                                   function's, if it has one */
};

/* A value for the story step, sequence global 0x27. */
struct RosePanEventStep
{
    std::string name;           /* "STEP_01_OPENING" */
    int         value;
};

/* A sequence flag bit. */
struct RosePanEventFlag
{
    std::string name;           /* "FLAG_01_DOGCRY_OGGATEF" */
    int         bit;
};

/* A chapter and what its scripts name. */
struct RosePanEventChapter
{
    int         stage;          /* the W<stage> folder, GamMan m_chapter */
    const char *label;
    bool        scanned;
    std::vector<RosePanEventRoom>  rooms;
    std::vector<RosePanEvent>      events;
    std::vector<RosePanEventStep>  steps;
    std::vector<RosePanEventFlag>  flags;
};

/* What a jump asks for; a field of -1 (NULL, empty) stays as it is. */
struct RosePanEventJump
{
    int         stage;          /* a chapter */
    int         room;
    int         step;
    int         image;          /* with entry: the script function to run */
    unsigned    entry;
    std::string scene;          /* a scene to play by itself, without its
                                   script function ("ev108") */
    std::string label;          /* for the status line */
};

/* The chapters, in story order (RoR's own table, data 0x2db2c0). */
int  RosePanEventTest_ChapterCount(void);
/* Its name, without scanning it. */
const char *RosePanEventTest_ChapterLabel(int index);
/* Chapter index, scanned on the first call for it; NULL out of range. */
const RosePanEventChapter *RosePanEventTest_Chapter(int index);
/* The index of chapter stage, -1 when it is not in the table. */
int  RosePanEventTest_ChapterOfStage(int stage);

/* Queues a jump, replacing one not started yet. Any thread. */
void RosePanEventTest_Request(const RosePanEventJump &jump);
/* Stops waiting on a jump under way. Any thread. */
void RosePanEventTest_Cancel(void);
/* Sets a flag bit, or flips it when value is -1, at the next tick. */
void RosePanEventTest_SetFlag(int bit, int value);

/* What the window shows; read by any thread (each field is one word). */
struct RosePanEventStatus
{
    int  stage;
    int  room;
    int  step;
    int  busy;                  /* a jump is under way */
    char text[96];              /* what the jump is doing, or last did */
};
void RosePanEventTest_GetStatus(RosePanEventStatus *status);
/* Reads a flag bit; 0 before the sequence is up. */
int  RosePanEventTest_GetFlag(int bit);

/* ---- called by the game ---- */

/* The head of Sequence::exec: runs what is queued. */
void RosePanEventTest_Tick(void);
/* Native 029 (ac_jump_area) about to change room: during a chapter jump the
 * chapter's first room is swapped for the one the jump wants, so its room
 * never loads. */
void RosePanEventTest_OverrideRoom(int *room, unsigned *word);

#endif /* ROSEPAN_EVENTTEST_H */
