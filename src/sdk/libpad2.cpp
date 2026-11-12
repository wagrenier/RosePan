/* ==========================================================================
 *  libpad2.cpp  (SCE controller library 2 -- PC-port shim)
 *
 *  A socket is a port number. Port 0 is libpad's shim (sdk/libpad.cpp:
 *  rosepan_input, or the scripted ROSEPAN_AUTOPAD in smoke runs); port 1
 *  has nothing plugged in. scePad2Read drops libpad's 2-byte header, which
 *  leaves the report libpad2 returns. libvib's sceVibSetActParam is here
 *  too: the PS2 motors hold their state until changed, the host rumble
 *  call lasts 120 ms, so the state is kept and re-sent on every read.
 * ======================================================================== */

#include "libpad2.h"
#include "libpad.h"
#include "libvib.h"

#include "../rosepan/io/rosepan_input.h"

#include <string.h>

#define PAD2_SOCKET_MAX 16

struct Pad2Socket
{
    int open;
    int port;
    unsigned char small;        /* small motor on / off */
    unsigned char big;          /* big motor 0..255 */
};

static int s_pad2Init;
static Pad2Socket s_socket[PAD2_SOCKET_MAX];

static Pad2Socket *pad2Socket(int socket)
{
    if (socket < 0 || socket >= PAD2_SOCKET_MAX || !s_socket[socket].open)
        return 0;
    return &s_socket[socket];
}

extern "C" {

int scePad2Init(int mode)
{
    (void)mode;
    memset(s_socket, 0, sizeof(s_socket));
    s_pad2Init = 1;
    return 1;
}

int scePad2End(void)
{
    memset(s_socket, 0, sizeof(s_socket));
    s_pad2Init = 0;
    return 1;
}

int scePad2CreateSocket(scePad2SocketParam *param, void *addr)
{
    int i;

    if (((size_t)addr & 0x3f) != 0)
        return -1;
    for (i = 0; i < PAD2_SOCKET_MAX; i++) {
        if (!s_socket[i].open) {
            memset(&s_socket[i], 0, sizeof(s_socket[i]));
            s_socket[i].open = 1;
            s_socket[i].port = param != 0 ? param->port : 0;
            if (s_socket[i].port == 0)
                scePadPortOpen(0, 0, addr);
            return i;
        }
    }
    return -1;
}

int scePad2DeleteSocket(int socket)
{
    Pad2Socket *s = pad2Socket(socket);

    if (s == 0)
        return -1;
    if (s->port == 0 && (s->small || s->big))
        RosePan_InputRumble(0, 0);
    s->open = 0;
    return 1;
}

int scePad2GetState(int socket)
{
    Pad2Socket *s = pad2Socket(socket);

    if (s == 0 || s->port != 0)
        return scePad2StateNoLink;
    return scePadGetState(0, 0) == scePadStateStable ? scePad2StateStable : scePad2StateNoLink;
}

int scePad2Read(int socket, void *data)
{
    Pad2Socket *s = pad2Socket(socket);
    unsigned char rdata[32];

    if (s == 0 || s->port != 0 || data == 0)
        return -1;
    if (scePadRead(0, 0, rdata) == 0)
        return -1;
    if (s->small || s->big)
        RosePan_InputRumble(s->small, s->big);
    /* buttons, then right X / Y, left X / Y: no pressure on the host */
    memcpy(data, rdata + 2, 6);
    return 6;
}

int sceVibSetActParam(int socket, int profilesize, unsigned char *profile,
                      int datasize, unsigned char *data)
{
    Pad2Socket *s = pad2Socket(socket);
    unsigned int bits = 0;
    int i;
    int shift = 0;

    if (s == 0 || profilesize < 1 || profile == 0 || data == 0)
        return 0;
    for (i = 0; i < datasize && i < 4; i++)
        bits |= (unsigned int)data[i] << (i * 8);
    if (profile[0] & 1) {
        s->small = (unsigned char)(bits & 1);
        shift = 1;
    }
    if (profile[0] & 2)
        s->big = (unsigned char)(bits >> shift);
    if (s->port == 0)
        RosePan_InputRumble(s->small, s->big);
    return 1;
}

}
