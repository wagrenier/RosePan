// bed/bedpad.cpp -- BedPad, one controller port on libpad2 (bed/bedpad.h).

#include "bed/bedpad.h"

#include <math.h>

#include "common/ror_assert.h"
#include "sdk/libpad2.h"
#include "sdk/libvib.h"
#include "sdk/scetypes.h"

// ROR 0x00308280: libpad2's DMA buffers, 0x100 bytes per port.
alignas(64) static u_long128 zDmaBuf[2][0x10];

// ROR 0x00101aec  // callcheck: operator_delete (the deleting variant, compiler generated)
BedPad::~BedPad()
{
    if (m_port >= 0) {
        term();
        m_port = -1;
    }
}

// ROR 0x00101b50
// small < 0 / big < 0 leaves that motor alone; small is on / off (bit 0),
// big a 0..255 level, packed after it when both are set.
void BedPad::setVibration(int small, int big)
{
    // PC: zeroed; the ROM sends whatever the stack holds when both are < 0.
    u_char profile[4] = { 0, 0, 0, 0 };
    u_char data[4] = { 0, 0, 0, 0 };
    int size = 1;

    if (small >= 0) {
        profile[0] = 1;
        data[0] = small & 1;
        if (big >= 0) {
            size = 2;
            profile[0] = 3;
            u_int bits = (small & 1) | ((big & 0xff) << 1);
            data[0] = (u_char)bits;
            data[1] = (u_char)(bits >> 8);
        }
    } else if (big >= 0) {
        profile[0] = 2;
        data[0] = big & 0xff;
    }
    sceVibSetActParam(m_socket, 1, profile, size, data);
}

// ROR 0x00101bcc  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
void BedPad::init(int thePortNo)
{
    ROR_ASSERT(thePortNo >= 0 && thePortNo < 2);   /* 86 */
    ROR_ASSERT((((int)(size_t)zDmaBuf)&0x3F) == 0); /* 88 */
    m_port = thePortNo;

    scePad2SocketParam param;
    param.option = 2;
    param.port = thePortNo != 0;
    param.slot = 0;
    param.number = 0;
    param.name[0] = 0;
    m_socket = scePad2CreateSocket(&param, zDmaBuf[thePortNo]);
}

// ROR 0x00101c7c  // callcheck: dbgAssertFailed (the assert handler, inside ROR_ASSERT)
void BedPad::term()
{
    ROR_ASSERT(m_socket>=0);   /* 133 */
    int socket = m_socket;
    m_socket = -1;
    scePad2DeleteSocket(socket);
}

// ROR 0x00101ccc
// Returns 0 (every view cleared, sticks centred) when the port is closed or
// not stable.
int BedPad::update()
{
    if (m_socket < 0 || scePad2GetState(m_socket) != scePad2StateStable) {
        m_stick[0] = 0x80;
        m_dir_len = 0.0f;
        m_trig = 0;
        m_hold = 0;
        m_old = 0;
        m_stick[3] = 0x80;
        m_stick[2] = 0x80;
        m_stick[1] = 0x80;
        m_dir_y = 0.0f;
        m_dir_x = 0.0f;
        m_dir_change = 0.0f;
        return 0;
    }

    scePad2Read(m_socket, this);
    u_short hold = ~m_raw;
    u_short trig = hold & ~m_hold;
    m_stick[0] = m_raw_stick[0];
    m_stick[1] = m_raw_stick[1];
    m_stick[2] = m_raw_stick[2];
    m_stick[3] = m_raw_stick[3];
    m_frame++;
    m_old = m_hold;
    m_hold = hold;
    m_trig = trig;
    m_repeat = 0;
    if (hold == 0) {
        m_hold_count = 0;
    } else {
        int count = ++m_hold_count;
        if (trig != 0) {
            m_repeat |= m_trig;
        } else if (m_repeat_wait != 0 && m_repeat_interval != 0) {
            if (count >= m_repeat_wait && (count - m_repeat_wait) % m_repeat_interval == 0)
                m_repeat = hold;
        }
    }

    updateDir();
    m_dir_change = calcDirChange();
    if (m_dir_change >= 0x1.0624dcp-10f) {  /* 0x3a83126e, 0.001 */
        m_last_dir_x = m_dir_x;
        m_last_dir_len = m_dir_len;
        m_last_dir_y = m_dir_y;
    }
    return 1;
}

// A stick byte to -1..1 with the 0x60..0xa0 dead zone.
static inline float bedPadStick(u_char v)
{
    if (v < 0x60)
        return ((float)v - 96.0f) / 96.0f;
    if (v > 0xa0)
        return ((float)v - 160.0f) / 95.0f;
    return 0.0f;
}

// ROR 0x00101e64
float BedPad::getRightX() const
{
    return bedPadStick(m_stick[0]);
}

// ROR 0x00101eec
float BedPad::getRightY() const
{
    return bedPadStick(m_stick[1]);
}

// ROR 0x00101f74
float BedPad::getLeftX() const
{
    return bedPadStick(m_stick[2]);
}

// ROR 0x00101ffc
float BedPad::getLeftY() const
{
    return bedPadStick(m_stick[3]);
}

// ROR 0x00102084
float BedPad::getDir(float *x, float *y) const
{
    *x = m_dir_x;
    *y = m_dir_y;
    return m_dir_len;
}

// ROR 0x0010209c
float BedPad::getDirChange() const
{
    return m_dir_change;
}

// ROR 0x001020a4
// 0 for the same heading and length, 1 for the opposite heading, plus a
// tenth of the length change.
float BedPad::calcDirChange() const
{
    float len = m_dir_len;
    float x, y;
    if (len == 0.0f) {
        y = 0.0f;
        x = 0.0f;
        if (m_last_dir_len == 0.0f)
            return 0.0f;
    } else {
        x = m_dir_x / len;
        y = m_dir_y / len;
    }

    float last_len = m_last_dir_len;
    float last_x, last_y;
    if (last_len == 0.0f) {
        last_y = 0.0f;
        last_x = 0.0f;
    } else {
        last_y = m_last_dir_y / last_len;
        last_x = m_last_dir_x / last_len;
    }

    float d = len - last_len;
    if (d < 0.0f)
        d = -d;
    return (1.0f - (last_x * x + last_y * y)) * 0.5f + d * 100.0f * 0x1.0624dcp-10f;  /* 0x3a83126e, 0.001 */
}

// ROR 0x00102188  // callcheck: FUN_0026cb48 (sqrtf, the libm error path for a negative argument)
// The d-pad wins over the left stick, per axis.
void BedPad::updateDir()
{
    u_int bits = 0;

    if (m_hold >> 4 & 1) {
        bits = 0x10;
        m_dir_y = -1.0f;
    } else if (m_hold >> 6 & 1) {
        bits = 0x40;
        m_dir_y = 1.0f;
    } else {
        m_dir_y = getLeftY();
        if (m_dir_y < -0.7f)
            bits = 0x10;
        else if (m_dir_y > 0.7f)
            bits = 0x40;
    }

    if (m_hold >> 7 & 1) {
        bits |= 0x80;
        m_dir_x = -1.0f;
    } else if (m_hold >> 5 & 1) {
        bits |= 0x20;
        m_dir_x = 1.0f;
    } else {
        m_dir_x = getLeftX();
        if (m_dir_x < -0.7f)
            bits |= 0x80;
        else if (m_dir_x > 0.7f)
            bits |= 0x20;
    }

    u_int old = m_dir_hold;
    m_dir_repeat = 0;
    m_dir_hold = bits;
    m_dir_trig = ~old & bits;
    if (bits == 0) {
        m_dir_hold_count = 0;
    } else {
        int count = ++m_dir_hold_count;
        if (m_dir_trig != 0) {
            m_dir_repeat |= m_dir_trig;
        } else if (m_repeat_wait != 0 && m_repeat_interval != 0) {
            if (count >= m_repeat_wait && (count - m_repeat_wait) % m_repeat_interval == 0)
                m_dir_repeat = bits;
        }
    }

    m_dir_len = sqrtf(m_dir_x * m_dir_x + m_dir_y * m_dir_y);
    u_int quad = 0;
    if (m_dir_len >= 0.5f) {
        if (m_dir_y > 0x1.999998p-4f)          /* 0x3dcccccc, 0.1 */
            quad = 1;
        else if (m_dir_y < -0x1.999998p-4f)
            quad = 2;
        if (m_dir_x > 0x1.999998p-4f)
            quad |= 4;
        else if (m_dir_x < -0x1.999998p-4f)
            quad |= 8;
    }

    // A quadrant held 16 frames ends a wiggle; a change within 15 counts one.
    if (quad == m_quad && m_frame - m_quad_frame > 15)
        m_quad_flips = 0;
    int last = m_quad_frame;
    if (m_quad != quad) {
        m_quad = quad;
        m_quad_frame = m_frame;
        if (m_frame - last < 15)
            m_quad_flips++;
        else
            m_quad_flips = 0;
    }
}
