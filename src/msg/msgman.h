/* ==========================================================================
 *  msg/msgman.h
 *
 *  MsgMan (msg/msgman.cpp, 0x18330c..0x18741c): a text shown in a scene
 *  window one character a frame, laid out from a string (m_strBuf) with
 *  #-tags (the 41 codes of data 0x299738: #n new line, #r new page, #d
 *  character delay, #K wait for the cross button, #V voice stream ...),
 *  and the three message icons of the scene's S2dMes. Each tag has two
 *  handlers, both pointer-to-member tables the constructor fills: the
 *  exec one (process, reading its argument at m_strBuf + m_pos) and the
 *  measure one (getLineWidth / countLines, skipping it). MsgManSet
 *  (0x18741c..0x187724) is a pair of them found by window number; the
 *  menus and TskMsgMan hold one. msgGetSysText returns the system texts
 *  (the memory card messages, the yes / no choices). Names ror-inferred.
 * ======================================================================== */
#ifndef _MSG_MSGMAN_H
#define _MSG_MSGMAN_H

#include <stddef.h>

#include "sdk/eetypes.h"

struct MagSystem;

/* System text no (data 0x298fe8, 0x1d4 pointers); NULL past the end. */
const char *msgGetSysText(int no);                                       // ROR 0x001832e8
/* Two-byte UTF-8 (0xc2..0xdf lead) to one byte, src into dst; returns
 * dst. setMessage's path for the charset word 3. */
char *msgUtf8ToLatin1(char *dst, const char *src);                       // ROR 0x00183ce0

/* The tag ids findTag returns (the table at data 0x299738 in this order;
 * the handlers below follow it). */
enum MsgTag
{
    MSG_TAG_END       = 0,      // "##"
    MSG_TAG_NEWLINE   = 1,      // "#n"
    MSG_TAG_NEWPAGE   = 2,      // "#r"
    MSG_TAG_CENTER    = 6,      // "#C" (expandText: a 0x11 byte)
    MSG_TAG_INSERT    = 0x19,   // "#m"
    MSG_TAG_NUMBER    = 0x1c,   // "#s" (the shadowed second entry)
    MSG_TAG_KEYWAIT   = 0x1e,   // "#K"
    MSG_TAG_QUIT      = 0x26,   // "#q"
    MSG_TAG_STREND    = 0x28,   // "" (the string's NUL)
    MSG_TAG_COUNT     = 0x29,   // and findTag's "plain character"
};

/* MsgMan::expandText's record: the source text with tag codes, the 16
 * words the tags take, and the output buffer. EE 0x4c (the ROM's
 * openSysMsgWindow builds it on the stack). layout: host -- pointers. */
struct MsgExpand
{
    int         m_args[16];     // ROR 0x00: the argument words
    const char *m_src;          // ROR 0x40: the text to expand
    char       *m_dst;          // ROR 0x44: the output (cleared to m_dst_size first)
    int         m_dst_size;     // ROR 0x48: its size in bytes
};

/* EE 0x3bc. layout: host -- MagSystem and text pointers, and the two
 * pointer-to-member tables (8-byte GCC 2 {delta, index, pfn} entries on
 * the EE, 16 on the host); the EE offsets are in the comments. */
struct MsgMan
{
    typedef int (MsgMan::*ExecFn)();
    typedef int (MsgMan::*MeasureFn)(const char *arg, int *len);

    MsgMan();                                                            // ROR 0x0018330c
    ~MsgMan();                                                           // ROR 0x00183b24
    int  reset();                                                        // ROR 0x00183b74
    /* -1 when already open; binds the current MagSystem. */
    int  open();                                                         // ROR 0x00183bb8
    /* -1 when not open; hides the icons, stops the voice. */
    int  close();                                                        // ROR 0x00183c4c
    /* Starts text in window (flag: unused). */
    void setMessage(int window, const char *text, int flag);             // ROR 0x00183db0
    int  update();                                                       // ROR 0x00184008
    const char *getDataText(int id);                                     // ROR 0x00184048
    const char *getTagText(const char *p);                               // ROR 0x00184098
    /* ex->m_src into ex->m_dst with its tags expanded from ex->m_args. */
    int  expandText(MsgExpand *ex);                                      // ROR 0x00184128
    int  matchTag(const char *tag, const char *p);                       // ROR 0x001844c4
    int  countDigits(const char *p);                                     // ROR 0x0018454c
    int  readInt(const char *p, int *out);                               // ROR 0x00184578
    int  readHex(const char *p, int *out);                               // ROR 0x00184600
    int  readDigits(const char *p, char *out);                           // ROR 0x00184698
    int  findTag(const char *p);                                         // ROR 0x00184710
    const char *getTagName(int id);                                      // ROR 0x001847bc
    int  isSingleByte(int c);                                            // ROR 0x00184820
    int  isLeadByte(int c);                                              // ROR 0x00184874
    void drawChar(const char *p, int *pos);                              // ROR 0x001848c4
    int  process();                                                      // ROR 0x00184a94
    int  getLineWidth(const char *p);                                    // ROR 0x00184f2c
    int  countLines(const char *p);                                      // ROR 0x001850fc
    void showKeyIcon(int on);                                            // ROR 0x00185268
    void updateKeyIcon();                                                // ROR 0x00185348
    void showIcon(int on);                                               // ROR 0x001853cc
    void updateIcon();                                                   // ROR 0x001854ac
    void showInlineIcon(int on);                                         // ROR 0x00185594
    void updateInlineIcon();                                             // ROR 0x00185670
    void setStyle(int style);                                            // ROR 0x001856f4
    void applyStyle();                                                   // ROR 0x001856fc
    void applyFont();                                                    // ROR 0x00185704
    int  getLineStartX();                                                // ROR 0x00185760
    int  getCenterOffset(int lineH, int lines, int height);              // ROR 0x001857c8
    int  showChoiceIcon(const char *arg, int *len, int line);            // ROR 0x00186df4

    /* The tag handlers, in tag order (exec / measure). */
    int cmdEnd();                                                        // ROR 0x001857fc
    int skipEnd(const char *arg, int *len);                              // ROR 0x00185804
    int cmdNewLine();                                                    // ROR 0x0018580c
    int skipNewLine(const char *arg, int *len);                          // ROR 0x00185848
    int startPage();                                                     // ROR 0x00185850
    int skipNewPage(const char *arg, int *len);                          // ROR 0x00185954
    int cmdDelay();                                                      // ROR 0x0018595c
    int skipDelay(const char *arg, int *len);                            // ROR 0x001859cc
    int cmdDelaySkip();                                                  // ROR 0x001859fc
    int skipDelaySkip(const char *arg, int *len);                        // ROR 0x00185a74
    int cmdColor();                                                      // ROR 0x00185aa4
    int skipColor(const char *arg, int *len);                            // ROR 0x00185b58
    int cmdCenter();                                                     // ROR 0x00185bbc
    int skipCenter(const char *arg, int *len);                           // ROR 0x00185bf8
    int cmdCenterV();                                                    // ROR 0x00185c04
    int skipCenterV(const char *arg, int *len);                          // ROR 0x00185c84
    int cmdTop();                                                        // ROR 0x00185c90
    int skipTop(const char *arg, int *len);                              // ROR 0x00185cdc
    int cmdLeft();                                                       // ROR 0x00185ce8
    int skipLeft(const char *arg, int *len);                             // ROR 0x00185d20
    int cmdInsertText();                                                 // ROR 0x00185d2c
    int skipInsertText(const char *arg, int *len);                       // ROR 0x00185dec
    int cmdInsertDigits();                                               // ROR 0x00185ea4
    int skipInsertDigits(const char *arg, int *len);                     // ROR 0x00185f68
    int cmdWait();                                                       // ROR 0x00186018
    int skipWait(const char *arg, int *len);                             // ROR 0x00186088
    int cmdStyle();                                                      // ROR 0x001860c0
    int skipStyle(const char *arg, int *len);                            // ROR 0x001861cc
    int cmdX();                                                          // ROR 0x00186204
    int skipX(const char *arg, int *len);                                // ROR 0x00186258
    int cmdY();                                                          // ROR 0x00186290
    int skipY(const char *arg, int *len);                                // ROR 0x001862e4
    int cmdWindowX();                                                    // ROR 0x0018631c
    int skipWindowX(const char *arg, int *len);                          // ROR 0x001863bc
    int cmdWindowY();                                                    // ROR 0x001863f4
    int skipWindowY(const char *arg, int *len);                          // ROR 0x00186494
    int cmdFont30();                                                     // ROR 0x001864cc
    int skipFont30(const char *arg, int *len);                           // ROR 0x00186540
    int cmdFont34();                                                     // ROR 0x00186578
    int skipFont34(const char *arg, int *len);                           // ROR 0x001865ec
    int cmdFadeIn();                                                     // ROR 0x00186624
    int skipFadeIn(const char *arg, int *len);                           // ROR 0x00186714
    int cmdFadeOut();                                                    // ROR 0x0018674c
    int skipFadeOut(const char *arg, int *len);                          // ROR 0x00186804
    int cmdSkipNumber();                                                 // ROR 0x0018683c
    int skipSkipNumber(const char *arg, int *len);                       // ROR 0x00186880
    int cmdFadeOutSprite3();                                             // ROR 0x001868b8
    int skipFadeOutSprite3(const char *arg, int *len);                   // ROR 0x00186928
    int cmdVoice();                                                      // ROR 0x00186960
    int skipVoice(const char *arg, int *len);                            // ROR 0x00186a04
    int cmdStopVoice();                                                  // ROR 0x00186a3c
    int skipStopVoice(const char *arg, int *len);                        // ROR 0x00186ab4
    int cmdIconWidth();                                                  // ROR 0x00186aec
    int skipIconWidth(const char *arg, int *len);                        // ROR 0x00186b44
    int cmdLineHeight();                                                 // ROR 0x00186b80
    int skipLineHeight(const char *arg, int *len);                       // ROR 0x00186c38
    int cmdNumber28();                                                   // ROR 0x00186c70
    int skipNumber28(const char *arg, int *len);                         // ROR 0x00186cb8
    int cmdPause();                                                      // ROR 0x00186cf4
    int skipPause(const char *arg, int *len);                            // ROR 0x00186d0c
    int cmdKeyWait();                                                    // ROR 0x00186d14
    int skipKeyWait(const char *arg, int *len);                          // ROR 0x00186dc4
    int cmdIcon();                                                       // ROR 0x00186ef4
    int skipIcon(const char *arg, int *len);                             // ROR 0x00186f90
    int cmdKeyWaitIcon();                                                // ROR 0x00186fc0
    int skipKeyWaitIcon(const char *arg, int *len);                      // ROR 0x0018704c
    int cmdInlineIcon();                                                 // ROR 0x0018707c
    int skipInlineIcon(const char *arg, int *len);                       // ROR 0x00187114
    int cmdScroll();                                                     // ROR 0x00187144
    int skipScroll(const char *arg, int *len);                           // ROR 0x00187220
    int cmdProportional();                                               // ROR 0x00187250
    int skipProportional(const char *arg, int *len);                     // ROR 0x0018728c
    int cmdFixed();                                                      // ROR 0x00187294
    int skipFixed(const char *arg, int *len);                            // ROR 0x001872cc
    int cmdInsertGlobal();                                               // ROR 0x001872d4
    int skipInsertGlobal(const char *arg, int *len);                     // ROR 0x00187368
    int cmdQuit();                                                       // ROR 0x001873fc
    int skipQuit(const char *arg, int *len);                             // ROR 0x00187404
    int cmdNop();                                                        // ROR 0x0018740c
    int skipNop(const char *arg, int *len);                              // ROR 0x00187414

    int         m_window;       // ROR 0x000: the scene window, -1 none
    int         m_idle;         // ROR 0x004: 1 = free (MsgManSet::setMessage takes it)
    int         m_open;         // ROR 0x008
    int         m_posSaved;     // ROR 0x00c: the window's layout read once
    MagSystem  *m_system;       // ROR 0x010: MagSystem::m_current_system at open
    int         m_active;       // ROR 0x014: a text is set
    const char *m_strBuf;       // ROR 0x018: the text
    int         m_pos;          // ROR 0x01c: read offset into it
    int         m_pause;        // ROR 0x020: #p; 1 stops process
    int         m_style;        // ROR 0x024: #G (setStyle)
    int         m_stream;       // ROR 0x028: the voice's SndMan id, -1 none
    int         m_fontRefresh;  // ROR 0x02c: startPage calls updateWindowFont
    int         m_proportional; // ROR 0x030: #b / #B
    int         m_centerH;      // ROR 0x034: #C / #L
    int         m_x;            // ROR 0x038: the cursor
    int         m_y;            // ROR 0x03c
    int         m_iconW;        // ROR 0x040: #W: an icon's / an unknown byte's advance
    int         m_lineH;        // ROR 0x044: the window's word cc, #H
    int         m_inlineX;      // ROR 0x048: #/'s icon position
    int         m_inlineY;      // ROR 0x04c
    int         m_homeX;        // ROR 0x050: the window position setMessage keeps
    int         m_homeY;        // ROR 0x054
    int         m_homeZ;        // ROR 0x058
    int         m_winX;         // ROR 0x05c: the window position now
    int         m_winY;         // ROR 0x060
    int         m_winZ;         // ROR 0x064
    int         m_winW;         // ROR 0x068
    int         m_winH;         // ROR 0x06c
    int         m_rect[4];      // ROR 0x070: getWindowWords84 (left, top, right, bottom)
    int         m_lines;        // ROR 0x080: countLines of the page
    int         m_centerV;      // ROR 0x084: #R / #T
    int         m_font30;       // ROR 0x088: #j
    int         m_font34;       // ROR 0x08c: #J
    int         m_color;        // ROR 0x090: #c
    int         m_wait;         // ROR 0x094: #w
    int         m_waitCount;    // ROR 0x098
    int         m_waitTime;     // ROR 0x09c
    int         m_delayOn;      // ROR 0x0a0: #d / #D
    int         m_skip;         // ROR 0x0a4: the cross button pressed during #D
    int         m_skippable;    // ROR 0x0a8
    int         m_delayCount;   // ROR 0x0ac
    int         m_delay;        // ROR 0x0b0
    int         m_delayForce;   // ROR 0x0b4: MsgManSet::setDelay
    int         m_forceA;       // ROR 0x0b8
    int         m_forceDelay;   // ROR 0x0bc
    int         m_keyWait;      // ROR 0x0c0: #K / #i: wait for the cross button
    int         m_keyMode;      // ROR 0x0c4: 1 icon at the right, 2 at the bottom
    int         m_keyIconOn;    // ROR 0x0c8: S2dMes sprite 0
    int         m_keyIconAnim;  // ROR 0x0cc
    int         m_iconOn;       // ROR 0x0d0: S2dMes sprite 2
    int         m_iconAnim;     // ROR 0x0d4
    int         m_inlineOn;     // ROR 0x0d8: S2dMes sprite 1
    int         m_inlineAnim;   // ROR 0x0dc
    int         m_scroll;       // ROR 0x0e0: #S
    int         m_scrollTime;   // ROR 0x0e4
    int         m_scrollLines;  // ROR 0x0e8
    float       m_scrollStep;   // ROR 0x0ec
    int         m_scrollDone;   // ROR 0x0f0
    int         m_scrollTotal;  // ROR 0x0f4
    int         m_scrollFrame;  // ROR 0x0f8
    int         m_insPos;       // ROR 0x0fc: into m_ins
    int         m_measInsPos;   // ROR 0x100
    const char *m_ins;          // ROR 0x104: inserted text process draws first
    const char *m_measIns;      // ROR 0x108: the measure side's
    char        m_insBuf[0x20]; // ROR 0x10c: #v / #g's text
    ExecFn      m_exec[MSG_TAG_COUNT];    // ROR 0x12c
    MeasureFn   m_measure[MSG_TAG_COUNT]; // ROR 0x274
};
static_assert(offsetof(MsgMan, m_window) == 0x00, "m_window");
static_assert(offsetof(MsgMan, m_insBuf) >= 0x10c, "m_insBuf");
static_assert(sizeof(MsgMan) >= 0x3bc, "EE 0x3bc");

/* Two MsgMan, addressed by their window. EE 0x778. layout: host. */
struct MsgManSet
{
    void open();                                                         // ROR 0x0018741c
    void close();                                                        // ROR 0x00187464
    void update();                                                       // ROR 0x001874ac
    /* The first idle MsgMan shows text in window, unless one does already. */
    void setMessage(int window, const char *text, int flag);             // ROR 0x001874f4
    /* 0 when no MsgMan shows window; bit 0 while its text runs, bit 1
     * while it is paused. */
    int  getState(int window);                                           // ROR 0x001875c0
    /* The MsgMan showing window is reset (icons off, window -1, idle). */
    void release(int window);                                            // ROR 0x0018761c
    void togglePause(int window);                                        // ROR 0x001876b0
    void setDelay(int window, int delay);                                // ROR 0x001876e8

    MsgMan m_msg[2];            // ROR 0x000, 0x3bc
};
static_assert(sizeof(MsgManSet) >= 0x778, "EE 0x778");

#endif /* _MSG_MSGMAN_H */
