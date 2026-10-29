/* ==========================================================================
 *  msg/msgman.cpp
 *
 *  The message window text runner MsgMan and its pair MsgManSet
 *  (0x18330c..0x187724, the file named by its asserts). See msg/msgman.h.
 *
 *  PORT: all of MsgMan and MsgManSet (msgGetSysText 0x1832e8 sits before
 *  the file, unassigned@001832e8).
 * ======================================================================== */
#include "msg/msgman.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bed/bedpad.h"
#include "common/ror_assert.h"
#include "game/gamman.h"
#include "mag/magscene.h"
#include "mag/magsystem.h"
#include "sequence/sequence.h"
#include "snd/sndman.h"

/* ---- data ---- */

/* The tags (data 0x299738): id and text, ended by the id 0x29. The second
 * "#s" (0x1c) is never found -- findTag takes the first match.
 * layout: host -- string pointers. */
struct MsgTagRec
{
    int         m_id;
    const char *m_tag;
};
static const MsgTagRec msgTagTable[] = {
    { 0, "##" },  { 1, "#n" },  { 2, "#r" },  { 3, "#d" },  { 4, "#D" },   // 0x00299738
    { 5, "#c" },  { 6, "#C" },  { 7, "#R" },  { 8, "#T" },  { 9, "#L" },   // 0x00299760
    { 10, "#G" }, { 11, "#X" }, { 12, "#Y" }, { 13, "#[" }, { 14, "#]" },  // 0x00299788
    { 15, "#j" }, { 16, "#J" }, { 17, "#F" }, { 18, "#f" }, { 19, "#A" },  // 0x002997b0
    { 20, "#a" }, { 21, "#V" }, { 22, "#s" }, { 23, "#W" }, { 24, "#H" },  // 0x002997d8
    { 25, "#m" }, { 26, "#v" }, { 27, "#w" }, { 28, "#s" }, { 29, "#p" },  // 0x00299800
    { 30, "#K" }, { 31, "#I" }, { 32, "#i" }, { 33, "#/" }, { 34, "#S" },  // 0x00299828
    { 35, "#b" }, { 36, "#B" }, { 37, "#g" }, { 38, "#q" }, { 39, "#_" },  // 0x00299850
    { 40, "" },                                                            // 0x00299878
    { MSG_TAG_COUNT, "" },                                                 // 0x00299880
};

/* #v's digits: SJIS full-width 0..9 (data 0x299890, indexed by the ASCII
 * digit from 0x2997d0). */
static const char *const msgFullDigits[10] = {
    "\x82\x4f", "\x82\x50", "\x82\x51", "\x82\x52", "\x82\x53",
    "\x82\x54", "\x82\x55", "\x82\x56", "\x82\x57", "\x82\x58",
};

/* getDataText's default: "アンノウン" (unknown) in SJIS (rodata 0x2f71f0). */
static const char msgUnknownText[] = "\x83\x41\x83\x93\x83\x6d\x83\x45\x83\x93";

/* The charset word (data 0x2f73e4, 2 in this ROM, nothing writes it):
 * 3 reads the texts as one-byte (UTF-8 decoded) Latin, else SJIS. */
static int msgCharset = 2;

/* setMessage's decoded copy for charset 3 (bss 0x6e92d0, up to MagSystem). */
static char msgStrBuf[0x570];

/* The scene's message icons: sprite 0 the key icon, 1 the inline icon,
 * 2 the icon (bss 0x73e930 = g_magSystem.m_scene.m_mes). */
static inline S2dMes &msgMes()
{
    return g_magSystem.m_scene.m_mes;
}

// ROR 0x00183ce0
char *msgUtf8ToLatin1(char *dst, const char *src)
{
    int len = strlen(src);
    int out = 0;
    int i = 0;
    do
    {
        int next = i + 1;
        u_char c = (u_char)src[i];
        if (c < 0x80)
        {
            dst[out++] = c;
        }
        else if ((u_int)(c - 0xc2) < 0x1e && next <= len &&
                 (u_int)((u_char)src[i + 1] - 0x80) < 0x40)
        {
            next = i + 2;
            dst[out++] = (char)((c << 6) | (src[i + 1] & 0x3f));
        }
        i = next;
    } while (i < len);
    /* the ROM ends the string at the source length, past the output */
    dst[len] = '\0';
    return dst;
}

/* ---- MsgMan ---- */

// ROR 0x0018330c
MsgMan::MsgMan()
{
    m_open = 0;
    m_strBuf = NULL;
    m_exec[0] = &MsgMan::cmdEnd;                 m_measure[0] = &MsgMan::skipEnd;
    m_exec[1] = &MsgMan::cmdNewLine;             m_measure[1] = &MsgMan::skipNewLine;
    m_exec[2] = &MsgMan::startPage;              m_measure[2] = &MsgMan::skipNewPage;
    m_exec[3] = &MsgMan::cmdDelay;               m_measure[3] = &MsgMan::skipDelay;
    m_exec[4] = &MsgMan::cmdDelaySkip;           m_measure[4] = &MsgMan::skipDelaySkip;
    m_exec[5] = &MsgMan::cmdColor;               m_measure[5] = &MsgMan::skipColor;
    m_exec[6] = &MsgMan::cmdCenter;              m_measure[6] = &MsgMan::skipCenter;
    m_exec[7] = &MsgMan::cmdCenterV;             m_measure[7] = &MsgMan::skipCenterV;
    m_exec[8] = &MsgMan::cmdTop;                 m_measure[8] = &MsgMan::skipTop;
    m_exec[9] = &MsgMan::cmdLeft;                m_measure[9] = &MsgMan::skipLeft;
    m_exec[10] = &MsgMan::cmdStyle;              m_measure[10] = &MsgMan::skipStyle;
    m_exec[11] = &MsgMan::cmdX;                  m_measure[11] = &MsgMan::skipX;
    m_exec[12] = &MsgMan::cmdY;                  m_measure[12] = &MsgMan::skipY;
    m_exec[13] = &MsgMan::cmdWindowX;            m_measure[13] = &MsgMan::skipWindowX;
    m_exec[14] = &MsgMan::cmdWindowY;            m_measure[14] = &MsgMan::skipWindowY;
    m_exec[15] = &MsgMan::cmdFont30;             m_measure[15] = &MsgMan::skipFont30;
    m_exec[16] = &MsgMan::cmdFont34;             m_measure[16] = &MsgMan::skipFont34;
    m_exec[17] = &MsgMan::cmdFadeIn;             m_measure[17] = &MsgMan::skipFadeIn;
    m_exec[18] = &MsgMan::cmdFadeOut;            m_measure[18] = &MsgMan::skipFadeOut;
    m_exec[19] = &MsgMan::cmdSkipNumber;         m_measure[19] = &MsgMan::skipSkipNumber;
    m_exec[20] = &MsgMan::cmdFadeOutSprite3;     m_measure[20] = &MsgMan::skipFadeOutSprite3;
    m_exec[21] = &MsgMan::cmdVoice;              m_measure[21] = &MsgMan::skipVoice;
    m_exec[22] = &MsgMan::cmdStopVoice;          m_measure[22] = &MsgMan::skipStopVoice;
    m_exec[23] = &MsgMan::cmdIconWidth;          m_measure[23] = &MsgMan::skipIconWidth;
    m_exec[24] = &MsgMan::cmdLineHeight;         m_measure[24] = &MsgMan::skipLineHeight;
    m_exec[25] = &MsgMan::cmdInsertText;         m_measure[25] = &MsgMan::skipInsertText;
    m_exec[26] = &MsgMan::cmdInsertDigits;       m_measure[26] = &MsgMan::skipInsertDigits;
    m_exec[27] = &MsgMan::cmdWait;               m_measure[27] = &MsgMan::skipWait;
    m_exec[28] = &MsgMan::cmdNumber28;           m_measure[28] = &MsgMan::skipNumber28;
    m_exec[29] = &MsgMan::cmdPause;              m_measure[29] = &MsgMan::skipPause;
    m_exec[30] = &MsgMan::cmdKeyWait;            m_measure[30] = &MsgMan::skipKeyWait;
    m_exec[31] = &MsgMan::cmdIcon;               m_measure[31] = &MsgMan::skipIcon;
    m_exec[32] = &MsgMan::cmdKeyWaitIcon;        m_measure[32] = &MsgMan::skipKeyWaitIcon;
    m_exec[33] = &MsgMan::cmdInlineIcon;         m_measure[33] = &MsgMan::skipInlineIcon;
    m_exec[34] = &MsgMan::cmdScroll;             m_measure[34] = &MsgMan::skipScroll;
    m_exec[35] = &MsgMan::cmdProportional;       m_measure[35] = &MsgMan::skipProportional;
    m_exec[36] = &MsgMan::cmdFixed;              m_measure[36] = &MsgMan::skipFixed;
    m_exec[37] = &MsgMan::cmdInsertGlobal;       m_measure[37] = &MsgMan::skipInsertGlobal;
    m_exec[38] = &MsgMan::cmdQuit;               m_measure[38] = &MsgMan::skipQuit;
    m_exec[39] = &MsgMan::cmdNop;                m_measure[39] = &MsgMan::skipNop;
    m_exec[40] = &MsgMan::cmdEnd;                m_measure[40] = &MsgMan::skipEnd;
    reset();
}

// ROR 0x00183b24; callcheck: operator_delete (GCC 2's delete flag; the host's delete frees)
MsgMan::~MsgMan()
{
    close();
}

// ROR 0x00183b74
int MsgMan::reset()
{
    m_fontRefresh = 1;
    m_winW = 0x9c;
    m_stream = -1;
    m_strBuf = NULL;
    m_window = -1;
    m_pause = 0;
    m_system = NULL;
    m_ins = NULL;
    m_color = 0;
    m_centerV = 0;
    m_style = 0;
    m_posSaved = 0;
    return 0;
}

// ROR 0x00183bb8
int MsgMan::open()
{
    if (m_open == 1)
        return -1;
    m_fontRefresh = 1;
    m_open = 1;
    m_active = 0;
    m_centerH = 1;
    m_stream = -1;
    m_system = MagSystem::m_current_system;
    m_lineH = 0x14;
    m_font34 = 2;
    m_winW = 0x9c;
    m_idle = 1;
    m_x = 0;
    m_y = 0;
    m_iconW = 0x14;
    m_inlineX = 0;
    m_inlineY = 0;
    m_pause = 0;
    m_proportional = 1;
    m_ins = NULL;
    m_font30 = 0;
    m_color = 0;
    m_centerV = 0;
    m_style = 0;
    m_posSaved = 0;
    return 0;
}

// ROR 0x00183c4c
int MsgMan::close()
{
    if (m_open == 0)
        return -1;
    m_window = -1;
    showKeyIcon(0);
    showIcon(0);
    showInlineIcon(0);
    setStyle(0);
    m_system = NULL;
    m_open = 0;
    if (m_stream != -1)
    {
        g_sndMan.stopStream(m_stream);
        m_stream = -1;
    }
    return 0;
}

// ROR 0x00183db0; callcheck: FUN_002155f4 (the assert handler, inside ROR_ASSERT)
void MsgMan::setMessage(int window, const char *text, int flag)
{
    (void)flag;
    m_window = window;
    if (msgCharset == 3)
        m_strBuf = msgUtf8ToLatin1(msgStrBuf, text);
    else
        m_strBuf = text;
    ROR_ASSERT(m_strBuf);  /* 509 */
    m_pos = 0;
    m_active = 1;
    m_proportional = 1;
    m_system->m_scene.setWindowProportional(m_window, 1);
    m_font30 = 0;
    m_font34 = 2;
    applyFont();
    m_waitCount = 0;
    m_stream = -1;
    m_waitTime = 0;
    m_wait = 0;
    m_delayCount = 0;
    m_delay = 0;
    m_delayOn = 0;
    m_skip = 0;
    m_skippable = 0;
    m_delayForce = 0;
    m_forceA = 0;
    m_forceDelay = 0;
    m_pause = 0;
    m_keyWait = 0;
    m_keyMode = 0;
    m_keyIconOn = 0;
    m_keyIconAnim = 0;
    m_iconOn = 0;
    m_iconAnim = 0;
    m_inlineOn = 0;
    m_inlineAnim = 0;
    m_scroll = 0;
    m_scrollTime = 0;
    m_scrollLines = 0;
    m_scrollStep = 0.0f;
    m_scrollDone = 0;
    m_scrollTotal = 0;
    m_scrollFrame = 0;
    m_idle = 0;
    m_ins = NULL;
    MagScene &scene = m_system->m_scene;
    if (m_posSaved == 0)
    {
        scene.getWindowPos3(window, &m_winX, &m_winY, &m_winZ);
        m_homeX = m_winX;
        m_homeY = m_winY;
        m_homeZ = m_winZ;
        scene.getWindowSize(window, &m_winW, &m_winH);
        scene.getWindowWords84(window, &m_rect[0], &m_rect[1], &m_rect[2], &m_rect[3]);
        scene.getWindowWordCc(window, &m_lineH);
        m_posSaved = 1;
    }
    else
    {
        m_winZ = m_homeZ;
        m_winX = m_homeX;
        m_winY = m_homeY;
        scene.setWindowPos(m_window, m_homeX, m_homeY, m_homeZ);
    }
    m_color = 0;
}

// ROR 0x00184008
int MsgMan::update()
{
    if (m_open == 0)
        return -1;
    if (m_active == 1)
        process();
    return 0;
}

// ROR 0x00184048
const char *MsgMan::getDataText(int id)
{
    const char *text = NULL;
    g_gamMan.m_sysData.getField((short)id, 0xc, &text);
    return text != NULL ? text : msgUnknownText;
}

// ROR 0x00184098
const char *MsgMan::getTagText(const char *p)
{
    int id = findTag(p);
    if (id != MSG_TAG_COUNT)
    {
        int len = strlen(getTagName(id));
        if (id == MSG_TAG_INSERT)
        {
            int no = 0;  /* PC: the ROM reads it unset when no digits follow */
            readInt(p + len, &no);
            return getDataText(no);
        }
    }
    return NULL;
}

/* A string into the output with its size kept (expandText's #m / #s). */
static int msgAppend(MsgExpand *ex, int dst, const char *text)
{
    if (dst + (int)strlen(text) + 1 < ex->m_dst_size)
    {
        strcpy(ex->m_dst + dst, text);
        return dst + strlen(text);
    }
    strncpy(ex->m_dst + dst, text, ex->m_dst_size - dst);
    ex->m_dst[ex->m_dst_size - 1] = '\0';
    return ex->m_dst_size;
}

// ROR 0x00184128; callcheck: strcpy, strncpy (in msgAppend)
int MsgMan::expandText(MsgExpand *ex)
{
    int src = 0;
    int dst = 0;
    int lines = 0;
    memset(ex->m_dst, 0, ex->m_dst_size);
    for (;;)
    {
        int id = findTag(ex->m_src + src);
        if (id == MSG_TAG_COUNT)
        {
            ex->m_dst[dst++] = ex->m_src[src++];
        }
        else
        {
            int len = strlen(getTagName(id));
            int no = 0;
            int n;
            switch (id)
            {
            case MSG_TAG_END:
            case MSG_TAG_STREND:
                ex->m_dst[dst] = ex->m_src[src];
                if (dst + 1 < ex->m_dst_size)
                    ex->m_dst[dst + 1] = ex->m_src[src + 1];
                return 0;
            case MSG_TAG_NEWLINE:
                ex->m_dst[dst++] = '\n';
                lines++;
                src += len;
                continue;
            case MSG_TAG_CENTER:
                ex->m_dst[dst++] = 0x11;
                src += len;
                continue;
            case MSG_TAG_INSERT:
            {
                int at = src + len;
                const char *q = ex->m_src + at;
                if (*q == 'g')
                {
                    n = readInt(ex->m_src + at + 1, &no);
                    if (n < 1)
                        return at + 2;
                    no = g_sequence.m_global[no];
                    at++;
                }
                else
                {
                    n = readInt(q, &no);
                }
                if (n < 1)
                    return at + 1;
                src = at + n;
                if (no > 15)
                    return at + 1;
                dst = msgAppend(ex, dst, getDataText(ex->m_args[no]));
                break;
            }
            case MSG_TAG_NUMBER:
            {
                char num[16];
                src += len;
                const char *q = ex->m_src + src;
                if (*q == 'g')
                {
                    n = readInt(ex->m_src + src + 1, &no);
                    if (n < 1)
                        return src + 2;
                    no = g_sequence.m_global[no];
                    src++;
                }
                else
                {
                    n = readInt(q, &no);
                }
                if (n < 1)
                    return src + 1;
                if (no > 15)
                    return src + 1;
                src += n;
                sprintf(num, "%d", ex->m_args[no]);
                dst = msgAppend(ex, dst, num);
                continue;
            }
            case MSG_TAG_KEYWAIT:
                /* The ROM moves dst by the digit count and leaves src at
                 * them, so they follow a gap of zero bytes. */
                src += 2;
                showChoiceIcon(ex->m_src + src, &n, lines);
                dst += n;
                continue;
            default:
                ex->m_dst[dst++] = ex->m_src[src++];
                break;
            }
        }
        if (ex->m_dst_size <= dst)
            return 0;
    }
}

// ROR 0x001844c4
int MsgMan::matchTag(const char *tag, const char *p)
{
    int len = strlen(tag);
    if (len == 0)
        return *p == '\0' ? 0 : -1;
    for (int i = 0; i < len; i++)
    {
        if (tag[i] != p[i])
            return -1;
    }
    return 0;
}

// ROR 0x0018454c
int MsgMan::countDigits(const char *p)
{
    int n = 0;
    while ((u_int)((u_char)p[n] - '0') < 10)
        n++;
    return n;
}

// ROR 0x00184578
int MsgMan::readInt(const char *p, int *out)
{
    char buf[16];
    memset(buf, 0, sizeof(buf));
    int n = countDigits(p);
    int copy = n < 16 ? n : 15;
    if (n != 0)
    {
        memcpy(buf, p, copy);
        *out = atoi(buf);
    }
    return n;
}

// ROR 0x00184600
int MsgMan::readHex(const char *p, int *out)
{
    char buf[16];
    memset(buf, 0, sizeof(buf));
    int n = countDigits(p);  /* decimal digits only, as the ROM counts */
    int copy = n < 16 ? n : 15;
    if (n != 0)
    {
        memcpy(buf, p, copy);
        *out = strtol(buf, NULL, 16);
    }
    return n;
}

// ROR 0x00184698
int MsgMan::readDigits(const char *p, char *out)
{
    memset(out, 0, 16);
    int n = countDigits(p);
    if (n != 0)
        memcpy(out, p, n < 16 ? n : 15);
    return n;
}

// ROR 0x00184710
int MsgMan::findTag(const char *p)
{
    /* 0x1a (a file's EOF byte) ends the text: the ROM writes the NUL into it */
    if (*p == 0x1a)
        *(char *)p = '\0';
    int i = 0;
    for (; msgTagTable[i].m_id != MSG_TAG_COUNT; i++)
    {
        if (matchTag(msgTagTable[i].m_tag, p) == 0)
            break;
    }
    return msgTagTable[i].m_id;
}

// ROR 0x001847bc
const char *MsgMan::getTagName(int id)
{
    int i = 0;
    for (; msgTagTable[i].m_id != MSG_TAG_COUNT; i++)
    {
        if (id == msgTagTable[i].m_id)
            return msgTagTable[i].m_tag;
    }
    return msgTagTable[i].m_tag;
}

// ROR 0x00184820
int MsgMan::isSingleByte(int c)
{
    c &= 0xff;
    if (msgCharset == 3)
        return (u_int)(c - 0x20) < 0xe0;
    if ((u_int)(c - 0x20) < 0x60)
        return 1;
    return (u_int)(c - 0xa0) < 0x40;
}

// ROR 0x00184874
int MsgMan::isLeadByte(int c)
{
    c &= 0xff;
    if (msgCharset == 3)
        return 0;
    if ((u_int)(c - 0x80) < 0x20)
        return 1;
    return (u_int)(c - 0xe0) < 0xb;
}

// ROR 0x001848c4
void MsgMan::drawChar(const char *p, int *pos)
{
    char ch[3];
    memset(ch, 0, sizeof(ch));
    ch[0] = p[0];
    if (isSingleByte(p[0]) == 1)
    {
        *pos += 1;
    }
    else if (isLeadByte(ch[0]) == 1)
    {
        ch[1] = p[1];
        ch[0] = p[0];
        *pos += 2;
    }
    else
    {
        *pos += 1;
        m_x += m_iconW;
        return;
    }
    /* SJIS 0x8164 / 0x8149 sit narrower: nudged right, fixed advances */
    if (ch[0] == (char)0x81 && ch[1] == 'd')
        m_x += 3;
    else if (ch[0] == (char)0x81 && ch[1] == 'I')
        m_x += 4;
    m_system->drawView(m_window, m_x, m_y, ch, m_color);
    int w = m_system->m_scene.getWindowTextWidth(m_window, ch);
    if (ch[0] == (char)0x81 && ch[1] == 'd')
        w = 0x11;
    else if (ch[0] == (char)0x81 && ch[1] == 'I')
        w = 8;
    m_x += w;
}

// ROR 0x00184a94; callcheck: FUN_002155f4 (the assert handler, inside ROR_ASSERT)
int MsgMan::process()
{
    bool run = true;
    if (m_open == 0)
    {
        /* The ROM loops for ever here. PC: return. */
        return 0;
    }
    if (m_stream != -1 && g_sndMan.getStreamState(m_stream) == -1)
        m_stream = -1;
    if (m_idle == 1)
        return 0;
    ROR_ASSERT(m_strBuf);  /* 1209 */
    BedPad *pad = m_system->m_pad_ref[0].m_pad;
    if (m_pause == 1)
        return 0;
    if (m_pos == 0)
    {
        m_fontRefresh = 0;
        startPage();
        setStyle(m_style);
        applyStyle();
        m_fontRefresh = 1;
    }
    updateKeyIcon();
    updateIcon();
    updateInlineIcon();
    if (m_wait == 1)
    {
        if (--m_waitCount > 0)
            return 0;
        m_wait = 0;
    }
    if (m_keyWait == 1)
    {
        if (pad->m_trig & 0x4000)
        {
            m_keyWait = 0;
            showKeyIcon(0);
            showIcon(0);
        }
        return 0;
    }
    if (m_scroll == 1)
    {
        run = false;
        int done = m_scrollDone;
        int step;
        if (m_scrollFrame < m_scrollTime)
        {
            m_scrollFrame++;
            step = (int)(m_scrollStep * (float)m_scrollFrame - (float)done);
            m_scrollDone = done + step;
        }
        else
        {
            m_scroll = 0;
            m_scrollDone = m_scrollTotal;
            step = m_scrollTotal - done;
        }
        m_system->m_scene.scrollWindow(m_window, -step);
    }
    if (!run)
        return 0;
    for (;;)
    {
        if (m_ins == NULL)
        {
            int id = findTag(m_strBuf + m_pos);
            if (id == MSG_TAG_COUNT)
            {
                if (m_skippable == 1 && (pad->m_trig & 0x4000))
                    m_skip = 1;
                if (m_skip == 0 && m_delayOn == 1)
                {
                    if (--m_delayCount > 0)
                        return 0;
                    m_delayCount = m_delayForce == 1 ? m_forceDelay : m_delay;
                }
                drawChar(m_strBuf + m_pos, &m_pos);
            }
            else
            {
                m_pos += strlen(getTagName(id));
                if (id == MSG_TAG_END || id == MSG_TAG_QUIT || id == MSG_TAG_STREND)
                {
                    m_idle = 1;
                    m_window = -1;
                    return 0;
                }
                if ((this->*m_exec[id])() != 0)
                    run = false;
            }
        }
        else
        {
            if (m_skippable == 1 && (pad->m_trig & 0x4000))
                m_skip = 1;
            if (m_skip == 0 && m_delayOn == 1)
            {
                if (--m_delayCount > 0)
                    return 0;
                m_delayCount = m_delayForce == 1 ? m_forceDelay : m_delay;
            }
            drawChar(m_ins + m_insPos, &m_insPos);
            if (m_ins[m_insPos] == '\0')
            {
                m_ins = NULL;
                m_insPos = 0;
            }
        }
        updateKeyIcon();
        updateIcon();
        updateInlineIcon();
        if (!run)
            return 0;
    }
}

// ROR 0x00184f2c
int MsgMan::getLineWidth(const char *p)
{
    int width = 0;
    bool run = true;
    char ch[3];
    memset(ch, 0, sizeof(ch));
    m_measIns = NULL;
    MagScene &scene = m_system->m_scene;
    do
    {
        if (m_measIns == NULL)
        {
            int id = findTag(p);
            if (id == MSG_TAG_COUNT)
            {
                ch[0] = *p;
                if (*p < 0)
                {
                    ch[1] = p[1];
                    p += 2;
                    ch[2] = '\0';
                }
                else
                {
                    p += 1;
                    ch[1] = '\0';
                }
                width += scene.getWindowTextWidth(m_window, ch);
            }
            else
            {
                int len = strlen(getTagName(id));
                if ((id >= 0 && id < 3) || id == MSG_TAG_QUIT || id == MSG_TAG_STREND)
                    return width;
                int arg = 0;
                if ((this->*m_measure[id])(p + len, &arg) != 0)
                    run = false;
                p += len + arg;
            }
        }
        else
        {
            /* the ROM measures the last character's buffer again, not the
             * inserted text */
            width += scene.getWindowTextWidth(m_window, ch);
            m_measIns = NULL;
        }
    } while (run);
    return width;
}

// ROR 0x001850fc
int MsgMan::countLines(const char *p)
{
    int lines = 0;
    bool run = true;
    m_measIns = NULL;
    do
    {
        if (m_measIns == NULL)
        {
            int id = findTag(p);
            if (id == MSG_TAG_COUNT)
            {
                p += *p < 0 ? 2 : 1;
            }
            else
            {
                int len = strlen(getTagName(id));
                switch (id)
                {
                case MSG_TAG_END:
                case MSG_TAG_NEWPAGE:
                case MSG_TAG_QUIT:
                case MSG_TAG_STREND:
                    return lines + 1;
                case MSG_TAG_NEWLINE:
                    lines++;
                    break;
                }
                int arg = 0;
                if ((this->*m_measure[id])(p + len, &arg) != 0)
                    run = false;
                p += len + arg;
            }
        }
        else
        {
            m_measIns = NULL;
        }
    } while (run);
    return lines;
}

/* One S2dMes icon on with anim (the ROM's anim words count from 1), or off. */
static void msgShowSprite(int no, int on, int anim)
{
    S2dMes &mes = msgMes();
    /* PC: the boot memory card check opens and closes its MsgManSet before
     * the scene's S2dMes has sprites; the ROM then writes into low EE
     * memory, the host would fault on the NULL m_sprite. */
    if (mes.m_spr.m_sprite == NULL)
        return;
    if (on == 1)
    {
        mes.setSpriteProcessOn(no);
        mes.setSpriteColorStd(no);
        mes.setSpriteState(no, 1);
        mes.setSpriteDisp(no, 1);
        mes.setSpriteAnim(no, anim < 1 ? 0 : anim - 1);
        return;
    }
    mes.setSpriteState(no, 0);
    mes.setSpriteDisp(no, 0);
}

// ROR 0x00185268; callcheck: setSpriteAnim, setSpriteColorStd, setSpriteDisp, setSpriteProcessOn, setSpriteState (in msgShowSprite)
void MsgMan::showKeyIcon(int on)
{
    m_keyIconOn = on == 1 ? 1 : 0;
    msgShowSprite(0, on, m_keyIconAnim);
}

// ROR 0x00185348
void MsgMan::updateKeyIcon()
{
    if (m_keyIconOn == 1)
    {
        msgMes().setSpriteStdPoint(0, m_winX << 4, m_winY << 4);
        msgMes().setSpriteOffPoint(0, m_x << 4, m_y << 4);
    }
}

// ROR 0x001853cc; callcheck: setSpriteAnim, setSpriteColorStd, setSpriteDisp, setSpriteProcessOn, setSpriteState (in msgShowSprite)
void MsgMan::showIcon(int on)
{
    m_iconOn = on == 1 ? 1 : 0;
    msgShowSprite(2, on, m_iconAnim);
}

// ROR 0x001854ac
void MsgMan::updateIcon()
{
    if (m_iconOn != 1)
        return;
    if (m_keyMode == 2)
    {
        msgMes().setSpriteStdPoint(2, m_homeX << 4, m_homeY << 4);
        msgMes().setSpriteOffPoint(2, ((m_winW >> 1) - 0xc) * 0x10, 0xba0);
        return;
    }
    msgMes().setSpriteStdPoint(2, m_winX << 4, m_winY << 4);
    msgMes().setSpriteOffPoint(2, m_x << 4, m_y << 4);
}

// ROR 0x00185594; callcheck: setSpriteAnim, setSpriteColorStd, setSpriteDisp, setSpriteProcessOn, setSpriteState (in msgShowSprite)
void MsgMan::showInlineIcon(int on)
{
    m_inlineOn = on == 1 ? 1 : 0;
    msgShowSprite(1, on, m_inlineAnim);
}

// ROR 0x00185670
void MsgMan::updateInlineIcon()
{
    if (m_inlineOn == 1)
    {
        msgMes().setSpriteStdPoint(1, m_winX << 4, m_winY << 4);
        msgMes().setSpriteOffPoint(1, m_inlineX << 4, m_inlineY << 4);
    }
}

// ROR 0x001856f4
void MsgMan::setStyle(int style)
{
    m_style = style;
}

// ROR 0x001856fc
void MsgMan::applyStyle()
{
}

// ROR 0x00185704
void MsgMan::applyFont()
{
    if (m_system != NULL)
    {
        m_system->m_scene.setWindowFontWord30(m_window, m_font30);
        m_system->m_scene.setWindowFontWord34(m_window, m_font34);
    }
}

// ROR 0x00185760
int MsgMan::getLineStartX()
{
    applyFont();
    int x = 0;
    if (m_centerH != 0 && m_centerH == 1)
    {
        int w = getLineWidth(m_strBuf + m_pos);
        x = ((m_rect[2] - m_rect[0]) >> 1) - (w >> 1);
    }
    return x;
}

// ROR 0x001857c8
int MsgMan::getCenterOffset(int lineH, int lines, int height)
{
    int y = height / 2 - (lineH * lines) / 2;
    return m_lineH == 0x26 ? y + 10 : y;
}

/* The page's lines centred in the window, or the window at its home y
 * (#R, #T, #G, #H and startPage). */
#define MSG_PLACE_WINDOW()                                                       \
    do {                                                                         \
        if (m_centerV == 1)                                                      \
        {                                                                        \
            m_lines = countLines(m_strBuf + m_pos);                              \
            int off = getCenterOffset(m_lineH, m_lines, m_winH);                 \
            m_winY = m_homeY + off;                                              \
            m_system->m_scene.setWindowPos(m_window, m_winX, m_homeY + off, m_winZ); \
        }                                                                        \
        else                                                                     \
        {                                                                        \
            m_winY = m_homeY;                                                    \
            m_system->m_scene.setWindowPos(m_window, m_winX, m_homeY, m_winZ);   \
        }                                                                        \
    } while (0)

/* ---- the tag handlers ---- */

// ROR 0x001857fc
int MsgMan::cmdEnd()
{
    return 0;
}

// ROR 0x00185804
int MsgMan::skipEnd(const char *, int *)
{
    return 0;
}

// ROR 0x0018580c
int MsgMan::cmdNewLine()
{
    m_x = getLineStartX();
    m_y += m_lineH;
    return 0;
}

// ROR 0x00185848
int MsgMan::skipNewLine(const char *, int *)
{
    return 0;
}

// ROR 0x00185850; callcheck: countLines, getCenterOffset, setWindowPos (in MSG_PLACE_WINDOW)
int MsgMan::startPage()
{
    if (m_fontRefresh == 1)
        m_system->m_scene.updateWindowFont(m_window);
    m_x = getLineStartX();
    m_y = 0;
    m_delayForce = 0;
    m_skip = 0;
    m_skippable = 0;
    MSG_PLACE_WINDOW();
    return 1;
}

// ROR 0x00185954
int MsgMan::skipNewPage(const char *, int *)
{
    return 0;
}

// ROR 0x0018595c
int MsgMan::cmdDelay()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
    {
        m_delayOn = n != 0;
        m_delayCount = n;
        m_delay = n;
    }
    return 0;
}

// ROR 0x001859cc
int MsgMan::skipDelay(const char *arg, int *len)
{
    int n;
    *len = readInt(arg, &n);
    return 0;
}

// ROR 0x001859fc
int MsgMan::cmdDelaySkip()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
    {
        m_skippable = n != 0;
        m_delayOn = n != 0;
        m_delayCount = n;
        m_delay = n;
    }
    return 0;
}

// ROR 0x00185a74
int MsgMan::skipDelaySkip(const char *arg, int *len)
{
    int n;
    *len = readInt(arg, &n);
    return 0;
}

/* A number (hex for #c), or g<n>: script global n (#c, #m), read at the
 * text position. The ROM's handlers each write this out; false when no
 * digits were read. */
static bool msgReadArg(MsgMan *m, bool hex, int *out)
{
    const char *q = m->m_strBuf + m->m_pos;
    int len;
    if (*q == 'g')
    {
        m->m_pos++;
        len = m->readInt(m->m_strBuf + m->m_pos, out);
        m->m_pos += len;
        if (len < 1)
            return false;
        *out = g_sequence.m_global[*out];
        return true;
    }
    len = hex ? m->readHex(q, out) : m->readInt(q, out);
    m->m_pos += len;
    return len >= 1;
}

// ROR 0x00185aa4; callcheck: readHex, readInt (in msgReadArg)
int MsgMan::cmdColor()
{
    int n;
    if (msgReadArg(this, true, &n))
        m_color = n;
    return 0;
}

// ROR 0x00185b58
int MsgMan::skipColor(const char *arg, int *len)
{
    int n;
    if (*arg == 'g')
    {
        *len = 1;
        *len += readInt(arg + 1, &n);
    }
    else
    {
        *len = readHex(arg, &n);
    }
    return 0;
}

// ROR 0x00185bbc
int MsgMan::cmdCenter()
{
    m_centerH = 1;
    m_x = getLineStartX();
    applyStyle();
    return 0;
}

// ROR 0x00185bf8
int MsgMan::skipCenter(const char *, int *len)
{
    *len = 0;
    return 0;
}

// ROR 0x00185c04; callcheck: countLines, getCenterOffset, setWindowPos (in MSG_PLACE_WINDOW)
int MsgMan::cmdCenterV()
{
    m_centerV = 1;
    MSG_PLACE_WINDOW();
    return 0;
}

// ROR 0x00185c84
int MsgMan::skipCenterV(const char *, int *len)
{
    *len = 0;
    return 0;
}

// ROR 0x00185c90; callcheck: setWindowPos (in MSG_PLACE_WINDOW)
int MsgMan::cmdTop()
{
    m_centerV = 0;
    MSG_PLACE_WINDOW();
    return 0;
}

// ROR 0x00185cdc
int MsgMan::skipTop(const char *, int *len)
{
    *len = 0;
    return 0;
}

// ROR 0x00185ce8
int MsgMan::cmdLeft()
{
    m_centerH = 0;
    m_x = getLineStartX();
    applyStyle();
    return 0;
}

// ROR 0x00185d20
int MsgMan::skipLeft(const char *, int *len)
{
    *len = 0;
    return 0;
}

// ROR 0x00185d2c; callcheck: readInt (in msgReadArg)
int MsgMan::cmdInsertText()
{
    int n;
    if (msgReadArg(this, false, &n))
    {
        const char *text = getDataText(n);
        m_insPos = 0;
        m_ins = text;
    }
    return 0;
}

// ROR 0x00185dec
int MsgMan::skipInsertText(const char *arg, int *len)
{
    int n;
    int l;
    if (*arg == 'g')
    {
        *len = 1;
        l = readInt(arg + 1, &n);
        *len += l;
        if (l < 1)
            return 0;
        n = g_sequence.m_global[n];
    }
    else
    {
        l = readInt(arg, &n);
        *len = l;
        if (l < 1)
            return 0;
    }
    const char *text = getDataText(n);
    m_measInsPos = 0;
    m_measIns = text;
    return 0;
}

/* #v's digits into m_insBuf as full-width SJIS. */
static void msgFillDigits(char *buf, const char *digits, int n)
{
    for (int i = 0; i < n; i++)
    {
        const char *d = msgFullDigits[digits[i] - '0'];
        buf[i * 2] = d[0];
        buf[i * 2 + 1] = d[1];
    }
}

// ROR 0x00185ea4
int MsgMan::cmdInsertDigits()
{
    char digits[32];
    /* PC: the ROM clears 0x20 bytes at m_ins, wherever it points (address
     * 0 when no insert is pending); the host clears m_insBuf, which is
     * what it fills. */
    memset(m_insBuf, 0, sizeof(m_insBuf));
    int n = readDigits(m_strBuf + m_pos, digits);
    if (n > -1)
    {
        m_pos += n;
        msgFillDigits(m_insBuf, digits, n);
        m_insPos = 0;
        m_ins = m_insBuf;
    }
    return 0;
}

// ROR 0x00185f68
int MsgMan::skipInsertDigits(const char *arg, int *len)
{
    char digits[32];
    int n = readDigits(arg, digits);
    if (n > -1)
    {
        *len += n;
        msgFillDigits(m_insBuf, digits, n);
        /* the ROM sets the drawing side's insert here, not m_measIns */
        m_insPos = 0;
        m_ins = m_insBuf;
    }
    return 0;
}

// ROR 0x00186018
int MsgMan::cmdWait()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
    {
        m_wait = n != 0;
        m_waitCount = n;
        m_waitTime = n;
        return 1;
    }
    return 0;
}

// ROR 0x00186088
int MsgMan::skipWait(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x001860c0; callcheck: countLines, getCenterOffset, setWindowPos (in MSG_PLACE_WINDOW)
int MsgMan::cmdStyle()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
    {
        m_style = n;
        if (n == 0)
        {
            m_centerV = 0;
        }
        else
        {
            m_centerV = 1;
            m_lineH = 0x26;
        }
        MSG_PLACE_WINDOW();
        setStyle(m_style);
        applyStyle();
    }
    return 0;
}

// ROR 0x001861cc
int MsgMan::skipStyle(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x00186204
int MsgMan::cmdX()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
        m_x = n;
    return 0;
}

// ROR 0x00186258
int MsgMan::skipX(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x00186290
int MsgMan::cmdY()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
        m_y = n;
    return 0;
}

// ROR 0x001862e4
int MsgMan::skipY(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x0018631c
int MsgMan::cmdWindowX()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
    {
        m_homeX = n;
        m_winX = n;
        m_system->m_scene.setWindowPos(m_window, n, m_winY, m_winZ);
        applyStyle();
        updateIcon();
        updateKeyIcon();
        updateInlineIcon();
    }
    return 0;
}

// ROR 0x001863bc
int MsgMan::skipWindowX(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x001863f4
int MsgMan::cmdWindowY()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
    {
        m_homeY = n;
        m_winY = n;
        m_system->m_scene.setWindowPos(m_window, m_winX, n, m_winZ);
        applyStyle();
        updateIcon();
        updateKeyIcon();
        updateInlineIcon();
    }
    return 0;
}

// ROR 0x00186494
int MsgMan::skipWindowY(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x001864cc
int MsgMan::cmdFont30()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
    {
        m_font30 = n;
        m_system->m_scene.setWindowFontWord30(m_window, n);
    }
    return 0;
}

// ROR 0x00186540
int MsgMan::skipFont30(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x00186578
int MsgMan::cmdFont34()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
    {
        m_font34 = n;
        m_system->m_scene.setWindowFontWord34(m_window, n);
    }
    return 0;
}

// ROR 0x001865ec
int MsgMan::skipFont34(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x00186624
int MsgMan::cmdFadeIn()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
    {
        m_system->m_scene.fadeInWindow(m_window, n, 0, 0x80);
        S2dMes &mes = msgMes();
        mes.setSpriteProcessOn(0);
        mes.setSpriteFadeIn(0, n);
        mes.setSpriteProcessOn(2);
        mes.setSpriteFadeIn(2, n);
        mes.setSpriteProcessOn(1);
        mes.setSpriteFadeIn(1, n);
    }
    return 0;
}

// ROR 0x00186714
int MsgMan::skipFadeIn(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x0018674c
int MsgMan::cmdFadeOut()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
    {
        m_system->m_scene.fadeOutWindow(m_window, n, 0, 0x80);
        S2dMes &mes = msgMes();
        mes.setSpriteFadeOut(0, n);
        mes.setSpriteFadeOut(2, n);
        mes.setSpriteFadeOut(1, n);
    }
    return 0;
}

// ROR 0x00186804
int MsgMan::skipFadeOut(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x0018683c
int MsgMan::cmdSkipNumber()
{
    int n;
    m_pos += readInt(m_strBuf + m_pos, &n);
    return 0;
}

// ROR 0x00186880
int MsgMan::skipSkipNumber(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x001868b8
int MsgMan::cmdFadeOutSprite3()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
        msgMes().setSpriteFadeOut(3, n);
    return 0;
}

// ROR 0x00186928
int MsgMan::skipFadeOutSprite3(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x00186960
int MsgMan::cmdVoice()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
    {
        if (m_stream != -1)
        {
            g_sndMan.stopStream(m_stream);
            m_stream = -1;
        }
        m_stream = g_gamMan.m_sndMan.playStr(n, -1, NULL, false, 0);
    }
    return 0;
}

// ROR 0x00186a04
int MsgMan::skipVoice(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x00186a3c
int MsgMan::cmdStopVoice()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0 && m_stream != -1)
    {
        g_sndMan.stopStream(m_stream);
        m_stream = -1;
    }
    return 0;
}

// ROR 0x00186ab4
int MsgMan::skipStopVoice(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x00186aec
int MsgMan::cmdIconWidth()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
        m_iconW = n;
    return len > 0;
}

// ROR 0x00186b44
int MsgMan::skipIconWidth(const char *arg, int *len)
{
    int n;
    int l = readInt(arg, &n);
    *len += l;
    return l > 0;
}

// ROR 0x00186b80; callcheck: countLines, getCenterOffset, setWindowPos (in MSG_PLACE_WINDOW)
int MsgMan::cmdLineHeight()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
    {
        m_lineH = n;
        if (m_centerV == 1)
            MSG_PLACE_WINDOW();
    }
    return 0;
}

// ROR 0x00186c38
int MsgMan::skipLineHeight(const char *arg, int *len)
{
    int n;
    *len += readInt(arg, &n);
    return 0;
}

// ROR 0x00186c70
int MsgMan::cmdNumber28()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    return len > 0;
}

// ROR 0x00186cb8
int MsgMan::skipNumber28(const char *arg, int *len)
{
    int n;
    int l = readInt(arg, &n);
    *len += l;
    return l > 0;
}

// ROR 0x00186cf4
int MsgMan::cmdPause()
{
    m_pause = m_pause != 1;
    return 1;
}

// ROR 0x00186d0c
int MsgMan::skipPause(const char *, int *)
{
    return 1;
}

// ROR 0x00186d14
int MsgMan::cmdKeyWait()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len < 1)
        return 0;
    m_keyWait = 1;
    m_keyIconAnim = n;
    showKeyIcon(n != 0 ? 1 : 0);
    showIcon(0);
    if (m_keyMode == 1)
        m_x = m_winW - m_iconW;
    return 1;
}

// ROR 0x00186dc4
int MsgMan::skipKeyWait(const char *arg, int *len)
{
    int n;
    *len = readInt(arg, &n);
    return 0;
}

// ROR 0x00186df4
int MsgMan::showChoiceIcon(const char *arg, int *len, int line)
{
    int n = 3;
    *len = readInt(arg, &n);
    m_keyMode = 0;
    showKeyIcon(0);
    m_iconOn = 0;
    S2dMes &mes = msgMes();
    mes.setSpriteStdPoint(2, 0, 0);
    mes.setSpriteOffPoint(2, 0xec0, (line * 0x14 + 0x11c) * 0x10);
    mes.setSpriteProcessOn(2);
    mes.setSpriteColorStd(2);
    mes.setSpriteState(2, 1);
    mes.setSpriteDisp(2, 1);
    mes.setSpriteAnim(2, n < 1 ? 0 : n - 1);
    return 0;
}

// ROR 0x00186ef4
int MsgMan::cmdIcon()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
    {
        m_iconAnim = n;
        showIcon(n != 0 ? 1 : 0);
        if (m_keyMode == 1)
            m_x = m_winW - m_iconW;
    }
    return 0;
}

// ROR 0x00186f90
int MsgMan::skipIcon(const char *arg, int *len)
{
    int n;
    *len = readInt(arg, &n);
    return 0;
}

// ROR 0x00186fc0
int MsgMan::cmdKeyWaitIcon()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len < 1)
        return 0;
    m_keyWait = 1;
    m_iconAnim = n;
    showIcon(n != 0 ? 1 : 0);
    m_keyMode = 2;
    return 1;
}

// ROR 0x0018704c
int MsgMan::skipKeyWaitIcon(const char *arg, int *len)
{
    int n;
    *len = readInt(arg, &n);
    return 0;
}

// ROR 0x0018707c
int MsgMan::cmdInlineIcon()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > 0)
    {
        m_inlineAnim = n;
        showInlineIcon(n != 0 ? 1 : 0);
        m_inlineX = m_x;
        m_x = m_x + m_iconW;
        m_inlineY = m_y;
    }
    return 0;
}

// ROR 0x00187114
int MsgMan::skipInlineIcon(const char *arg, int *len)
{
    int n;
    *len = readInt(arg, &n);
    return 0;
}

// ROR 0x00187144
int MsgMan::cmdScroll()
{
    int n;
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len < 1)
        return 0;
    m_scroll = 1;
    int time = n / 10;
    int lines = n % 10;
    int total = m_lineH * lines;
    m_scrollTotal = total;
    m_scrollFrame = 0;
    m_scrollTime = time;
    m_scrollLines = lines;
    m_scrollDone = 0;
    m_scrollStep = time == 0 ? (float)total : (float)total / (float)time;
    return 1;
}

// ROR 0x00187220
int MsgMan::skipScroll(const char *arg, int *len)
{
    int n;
    *len = readInt(arg, &n);
    return 0;
}

// ROR 0x00187250
int MsgMan::cmdProportional()
{
    m_proportional = 1;
    m_system->m_scene.setWindowProportional(m_window, 1);
    return 0;
}

// ROR 0x0018728c
int MsgMan::skipProportional(const char *, int *)
{
    return 0;
}

// ROR 0x00187294
int MsgMan::cmdFixed()
{
    m_proportional = 0;
    m_system->m_scene.setWindowProportional(m_window, 0);
    return 0;
}

// ROR 0x001872cc
int MsgMan::skipFixed(const char *, int *)
{
    return 0;
}

// ROR 0x001872d4
int MsgMan::cmdInsertGlobal()
{
    int n = 0;  /* PC: the ROM reads it unset when no digits follow */
    int len = readInt(m_strBuf + m_pos, &n);
    m_pos += len;
    if (len > -1)
    {
        sprintf(m_insBuf, "%d", g_sequence.m_global[n]);
        m_ins = m_insBuf;
        m_insPos = 0;
    }
    return 0;
}

// ROR 0x00187368
int MsgMan::skipInsertGlobal(const char *arg, int *len)
{
    int n = 0;  /* PC: as cmdInsertGlobal */
    int l = readInt(arg, &n);
    *len += l;
    if (l > -1)
    {
        sprintf(m_insBuf, "%d", g_sequence.m_global[n]);
        m_measIns = m_insBuf;
        m_measInsPos = 0;
    }
    return 0;
}

// ROR 0x001873fc
int MsgMan::cmdQuit()
{
    return 1;
}

// ROR 0x00187404
int MsgMan::skipQuit(const char *, int *)
{
    return 1;
}

// ROR 0x0018740c
int MsgMan::cmdNop()
{
    return 0;
}

// ROR 0x00187414
int MsgMan::skipNop(const char *, int *)
{
    return 0;
}

/* ---- MsgManSet ---- */

// ROR 0x0018741c
void MsgManSet::open()
{
    for (int i = 0; i < 2; i++)
        m_msg[i].open();
}

// ROR 0x00187464
void MsgManSet::close()
{
    for (int i = 0; i < 2; i++)
        m_msg[i].close();
}

// ROR 0x001874ac
void MsgManSet::update()
{
    for (int i = 0; i < 2; i++)
        m_msg[i].update();
}

// ROR 0x001874f4
void MsgManSet::setMessage(int window, const char *text, int flag)
{
    if (window < 0)
        return;
    for (int i = 0; i < 2; i++)
    {
        if (window == m_msg[i].m_window)
            return;
    }
    int free = 0x378;  /* none */
    for (int i = 0; i < 2; i++)
    {
        if (m_msg[i].m_idle == 1)
        {
            free = i;
            break;
        }
    }
    if (free != 0x378)
        m_msg[free].setMessage(window, text, flag);
}

// ROR 0x001875c0
int MsgManSet::getState(int window)
{
    if (window < 0)
        return 0;
    for (int i = 0; i < 2; i++)
    {
        MsgMan &m = m_msg[i];
        if (window == m.m_window)
        {
            int state = m.m_idle == 0;
            if (m.m_pause == 1)
                state |= 2;
            return state;
        }
    }
    return 0;
}

// ROR 0x0018761c
void MsgManSet::release(int window)
{
    if (window < 0)
        return;
    for (int i = 0; i < 2; i++)
    {
        MsgMan &m = m_msg[i];
        if (window == m.m_window)
        {
            m.showKeyIcon(0);
            m.showIcon(0);
            m.showInlineIcon(0);
            m.setStyle(0);
            m.m_window = -1;
            m.m_idle = 1;
        }
    }
}

// ROR 0x001876b0
void MsgManSet::togglePause(int window)
{
    if (window < 0)
        return;
    for (int i = 0; i < 2; i++)
    {
        if (window == m_msg[i].m_window)
            m_msg[i].m_pause = m_msg[i].m_pause != 1;
    }
}

// ROR 0x001876e8
void MsgManSet::setDelay(int window, int delay)
{
    if (window < 0)
        return;
    for (int i = 0; i < 2; i++)
    {
        MsgMan &m = m_msg[i];
        if (window == m.m_window)
        {
            m.m_forceDelay = delay;
            m.m_forceA = delay;
            m.m_delayCount = 1;
            m.m_delayForce = 1;
        }
    }
}
