/* ==========================================================================
 *  bed/bedpad.h
 *
 *  BedPad (bed/bedpad.cpp; the class name is in the ELF's parameter types,
 *  e.g. MagLifeWindow::processPad(BedPad const &, BedPad const &)): one
 *  controller port on libpad2. MagSystem holds two at +0x40 and updates
 *  them first thing every frame. Minimal: only what MagSystem reaches.
 * ======================================================================== */
#ifndef _BED_BEDPAD_H
#define _BED_BEDPAD_H

#include <stddef.h>

/* EE 0x90 (MagSystem's two pads are 0x90 apart). No pointers known; the
 * fields not reconstructed yet are padding. */
struct BedPad
{
    // Inline in the ROM (MagSystem's constructor holds two copies): no
    // socket, no port, the three repeat words cleared.
    BedPad() : m_socket(-1), m_port(-1), m_30(0), m_repeat_wait(0), m_repeat_interval(0) {}
    ~BedPad();                                                           // ROR 0x00101aec

    void setVibration(int small, int big);                               // ROR 0x00101b50
    void init(int thePortNo);                                            // ROR 0x00101bcc
    void term();                                                         // ROR 0x00101c7c
    void update();                                                       // ROR 0x00101ccc

    unsigned char _00[0x20];
    int  m_socket;              // ROR 0x20: the libpad2 socket (assert m_socket>=0), -1 when closed
    int  m_port;                // ROR 0x24: thePortNo 0 / 1, -1 before init
    unsigned char _28[0x08];
    int  m_30;                  // ROR 0x30: cleared by the constructor; role unknown
    int  m_repeat_wait;         // ROR 0x34: MagSystem::initGraph sets 15 (unverified: key-repeat delay)
    int  m_repeat_interval;     // ROR 0x38: MagSystem::initGraph sets 10 (unverified: key-repeat rate)
    unsigned char _3c[0x04];
    int  m_40;                  // ROR 0x40: MagSystem::setPC clears it; role unknown
    int  m_44;                  // ROR 0x44: idem
    int  m_48;                  // ROR 0x48: idem
    unsigned char _4c[0x78 - 0x4c];
    unsigned short m_78;        // ROR 0x78: button bits MagScene::update tests (bit 14 closes a waiting window); which view of the buttons (pressed / triggered / repeated) is unverified
    unsigned short m_7a;        // ROR 0x7a: button bits (bits 8 / 9: MagScene::changeCamera); role unverified
    unsigned short m_7c;        // ROR 0x7c: button bits (bit 3 the item menu, bit 0 menu 0x10001, bits 13 / 14 close the system message); role unverified
    unsigned char _7e[0x90 - 0x7e];
};
static_assert(sizeof(BedPad) == 0x90, "BedPad is 0x90 bytes on the EE");
static_assert(offsetof(BedPad, m_socket) == 0x20, "m_socket");
static_assert(offsetof(BedPad, m_port) == 0x24, "m_port");
static_assert(offsetof(BedPad, m_30) == 0x30, "m_30");
static_assert(offsetof(BedPad, m_repeat_wait) == 0x34, "m_repeat_wait");
static_assert(offsetof(BedPad, m_repeat_interval) == 0x38, "m_repeat_interval");
static_assert(offsetof(BedPad, m_40) == 0x40, "m_40");
static_assert(offsetof(BedPad, m_44) == 0x44, "m_44");
static_assert(offsetof(BedPad, m_48) == 0x48, "m_48");
static_assert(offsetof(BedPad, m_7c) == 0x7c, "m_7c");

#endif /* _BED_BEDPAD_H */
