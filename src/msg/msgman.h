/* ==========================================================================
 *  msg/msgman.h
 *
 *  MsgMan (msg/msgman.cpp, 0x18330c..): a text shown in a scene window,
 *  laid out from a string (m_strBuf), with an optional voice stream.
 *  MsgManSet (0x18741c..0x18761c, unassigned@00184f2c) is a pair of them
 *  found by window number; MnuMcd holds one. msgGetSysText returns the
 *  system texts (the memory card messages, the yes / no choices). Class
 *  names from the file (ror-inferred). Not written yet: declared for their
 *  callers, padded to the EE size.
 * ======================================================================== */
#ifndef _MSG_MSGMAN_H
#define _MSG_MSGMAN_H

#include <stddef.h>

#include "sdk/eetypes.h"

/* System text no (data 0x298fe8, 0x1d4 pointers); NULL past the end. */
const char *msgGetSysText(int no);                                       // ROR 0x001832e8

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

/* EE 0x3bc. layout: EE size only -- not written, padded (its words +0x10
 * hold the MagSystem pointer on the EE: give it host members when the
 * class is written). */
struct MsgMan
{
    int  open();                                                         // ROR 0x00183bb8
    int  close();                                                        // ROR 0x00183c4c
    void setMessage(const char *text, int flag);                         // ROR 0x00183db0
    /* ex->m_src into ex->m_dst with its tags expanded from ex->m_args. */
    int  expandText(MsgExpand *ex);                                      // ROR 0x00184128

    int    m_window;            // ROR 0x00: the scene window it writes into, -1 none
    u_char _04[0x3bc - 0x04];
};
static_assert(offsetof(MsgMan, m_window) == 0x00, "m_window");
static_assert(sizeof(MsgMan) == 0x3bc, "EE 0x3bc");

/* Two MsgMan, addressed by their window. EE 0x778. */
struct MsgManSet
{
    void open();                                                         // ROR 0x0018741c
    void close();                                                        // ROR 0x00187464
    void update();                                                       // ROR 0x001874ac
    void setMessage(int window, const char *text, int flag);             // ROR 0x001874f4
    /* 0 when no MsgMan shows window; non-zero while its text runs. */
    int  getState(int window);                                           // ROR 0x001875c0
    /* The MsgMan showing window is reset (its four parts cleared, window
     * -1, word 1 = 1). */
    void release(int window);                                            // ROR 0x0018761c

    MsgMan m_msg[2];            // ROR 0x000, 0x3bc
};
static_assert(sizeof(MsgManSet) == 0x778, "EE 0x778");

#endif /* _MSG_MSGMAN_H */
