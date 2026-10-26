/* ==========================================================================
 *  bed/bedpad.h
 *
 *  BedPad (bed/bedpad.cpp; the class name is in the ELF's parameter types,
 *  e.g. MagLifeWindow::processPad(BedPad const &, BedPad const &)): one
 *  controller port on libpad2. MagSystem holds two at +0x40 and updates
 *  them first thing every frame. The libpad2 report is read straight into
 *  the object's first 0x20 bytes; update turns it into held / triggered /
 *  repeated button bits, and updateDir turns the d-pad or the left stick
 *  into a second set of four direction bits plus a direction vector.
 *
 *  Button bits are the PS2 report's (byte 0 low, byte 1 high, active-low in
 *  the report, active-high in the views): 0x1 select, 0x8 start, 0x10 up,
 *  0x20 right, 0x40 down, 0x80 left, 0x2000 circle, 0x4000 cross. The
 *  direction bits use the d-pad values (0x10 up, 0x20 right, 0x40 down,
 *  0x80 left).
 * ======================================================================== */
#ifndef _BED_BEDPAD_H
#define _BED_BEDPAD_H

#include <stddef.h>

/* EE 0x90 (MagSystem's two pads are 0x90 apart); no pointers. */
struct BedPad
{
    // Inline in the ROM (MagSystem's constructor holds two copies): no
    // socket, no port, the frame counter and the repeat words cleared.
    BedPad() : m_socket(-1), m_port(-1), m_frame(0), m_repeat_wait(0), m_repeat_interval(0) {}
    ~BedPad();                                                           // ROR 0x00101aec

    void  setVibration(int small, int big);                              // ROR 0x00101b50
    void  init(int thePortNo);                                           // ROR 0x00101bcc
    void  term();                                                        // ROR 0x00101c7c
    int   update();                                                      // ROR 0x00101ccc
    float getRightX() const;                                             // ROR 0x00101e64
    float getRightY() const;                                             // ROR 0x00101eec
    float getLeftX() const;                                              // ROR 0x00101f74
    float getLeftY() const;                                              // ROR 0x00101ffc
    float getDir(float *x, float *y) const;                              // ROR 0x00102084
    float getDirChange() const;                                          // ROR 0x0010209c
    float calcDirChange() const;                                         // ROR 0x001020a4
    void  updateDir();                                                   // ROR 0x00102188

    unsigned short m_raw;           // ROR 0x00: report buttons, active-low (scePad2Read writes 0x00..)
    unsigned char  m_raw_stick[4];  // ROR 0x02: report sticks: right X, right Y, left X, left Y
    unsigned char  _06[0x20 - 0x06];// ROR 0x06: rest of the report buffer
    int   m_socket;                 // ROR 0x20: the libpad2 socket (assert m_socket>=0), -1 when closed
    int   m_port;                   // ROR 0x24: thePortNo 0 / 1, -1 before init
    unsigned short m_old;           // ROR 0x28: last frame's m_hold
    unsigned char  _2a[0x02];
    int   m_hold_count;             // ROR 0x2c: frames the buttons have been held, 0 when none
    int   m_frame;                  // ROR 0x30: update count while connected
    int   m_repeat_wait;            // ROR 0x34: frames before auto-repeat (MagSystem::initGraph 15)
    int   m_repeat_interval;        // ROR 0x38: frames between repeats (MagSystem::initGraph 10)
    unsigned char  _3c[0x04];
    float m_last_dir_x;             // ROR 0x40: the direction latched when it changed enough
    float m_last_dir_y;             // ROR 0x44
    float m_last_dir_len;           // ROR 0x48
    float m_dir_x;                  // ROR 0x4c: -1..1, d-pad or left stick
    float m_dir_y;                  // ROR 0x50: -1..1 (negative up)
    float m_dir_len;                // ROR 0x54
    float m_dir_change;             // ROR 0x58: calcDirChange's result
    int   m_quad_frame;             // ROR 0x5c: m_frame when m_quad last changed
    unsigned int m_quad;            // ROR 0x60: 1 down / 2 up | 4 right / 8 left, the vector's quadrant
    int   m_quad_flips;             // ROR 0x64: quadrant changes less than 15 frames apart (stick wiggling)
    unsigned int m_dir_hold;        // ROR 0x68: direction bits held
    unsigned int m_dir_trig;        // ROR 0x6c: direction bits pressed this frame
    unsigned int m_dir_repeat;      // ROR 0x70: m_dir_trig plus auto-repeat (menu cursors)
    int   m_dir_hold_count;         // ROR 0x74: frames direction bits have been held
    unsigned short m_hold;          // ROR 0x78: buttons held
    unsigned short m_trig;          // ROR 0x7a: buttons pressed this frame
    unsigned short m_repeat;        // ROR 0x7c: m_trig plus auto-repeat of m_hold
    unsigned char  m_stick[4];      // ROR 0x7e: right X, right Y, left X, left Y (0x80 centred when not read)
    unsigned char  _82[0x90 - 0x82];
};
static_assert(sizeof(BedPad) == 0x90, "BedPad is 0x90 bytes on the EE");
static_assert(offsetof(BedPad, m_raw) == 0x00, "m_raw");
static_assert(offsetof(BedPad, m_raw_stick) == 0x02, "m_raw_stick");
static_assert(offsetof(BedPad, m_socket) == 0x20, "m_socket");
static_assert(offsetof(BedPad, m_port) == 0x24, "m_port");
static_assert(offsetof(BedPad, m_old) == 0x28, "m_old");
static_assert(offsetof(BedPad, m_hold_count) == 0x2c, "m_hold_count");
static_assert(offsetof(BedPad, m_frame) == 0x30, "m_frame");
static_assert(offsetof(BedPad, m_repeat_wait) == 0x34, "m_repeat_wait");
static_assert(offsetof(BedPad, m_repeat_interval) == 0x38, "m_repeat_interval");
static_assert(offsetof(BedPad, m_last_dir_x) == 0x40, "m_last_dir_x");
static_assert(offsetof(BedPad, m_last_dir_y) == 0x44, "m_last_dir_y");
static_assert(offsetof(BedPad, m_last_dir_len) == 0x48, "m_last_dir_len");
static_assert(offsetof(BedPad, m_dir_x) == 0x4c, "m_dir_x");
static_assert(offsetof(BedPad, m_dir_y) == 0x50, "m_dir_y");
static_assert(offsetof(BedPad, m_dir_len) == 0x54, "m_dir_len");
static_assert(offsetof(BedPad, m_dir_change) == 0x58, "m_dir_change");
static_assert(offsetof(BedPad, m_quad_frame) == 0x5c, "m_quad_frame");
static_assert(offsetof(BedPad, m_quad) == 0x60, "m_quad");
static_assert(offsetof(BedPad, m_quad_flips) == 0x64, "m_quad_flips");
static_assert(offsetof(BedPad, m_dir_hold) == 0x68, "m_dir_hold");
static_assert(offsetof(BedPad, m_dir_trig) == 0x6c, "m_dir_trig");
static_assert(offsetof(BedPad, m_dir_repeat) == 0x70, "m_dir_repeat");
static_assert(offsetof(BedPad, m_dir_hold_count) == 0x74, "m_dir_hold_count");
static_assert(offsetof(BedPad, m_hold) == 0x78, "m_hold");
static_assert(offsetof(BedPad, m_trig) == 0x7a, "m_trig");
static_assert(offsetof(BedPad, m_repeat) == 0x7c, "m_repeat");
static_assert(offsetof(BedPad, m_stick) == 0x7e, "m_stick");

#endif /* _BED_BEDPAD_H */
