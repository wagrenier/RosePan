/* ==========================================================================
 *  sdk/libpad2.h  (SCE PS2 controller library 2)
 *
 *  Rule of Rose reads its pads through libpad2 (scePad2*), not FF2 / FF3's
 *  libpad: main brings it up and down (Init / End), BedPad opens one socket
 *  per port and reads it every frame. The host shim (libpad2.cpp) backs the
 *  sockets with libpad's shim, so the scripted smoke pad and rosepan_input
 *  reach both libraries the same way.
 * ======================================================================== */

#ifndef _SDK_LIBPAD2_H
#define _SDK_LIBPAD2_H

/* scePad2GetState: BedPad::update reads only on Stable (the ROM compares 1). */
#define scePad2StateNoLink  0
#define scePad2StateStable  1

/* What BedPad::init fills (stores 0x00 / 0x04 / 0x08 / 0x0c, a byte at 0x10);
 * CreateSocket copies 0x20 bytes and ORs bit 0 into option. */
typedef struct {
    int option;                 /* BedPad passes 2 */
    int port;
    int slot;
    int number;
    unsigned char name[16];
} scePad2SocketParam;

#ifdef __cplusplus
extern "C" {
#endif

int scePad2Init(int mode);      /* clears the 16 port records; 1 */
int scePad2End(void);           /* closes every open port; 1 */
/* addr: a 64-byte aligned 0x100-byte DMA buffer (-1 otherwise); returns the socket. */
int scePad2CreateSocket(scePad2SocketParam *param, void *addr);
int scePad2DeleteSocket(int socket);                /* 1 on success */
int scePad2GetState(int socket);                    /* scePad2State* */
/* Copies the report: buttons (2 bytes, active-low), right X / Y, left X / Y,
 * pressure. Returns its byte count, -1 when there is none. */
int scePad2Read(int socket, void *data);

#ifdef __cplusplus
}
#endif

#endif /* _SDK_LIBPAD2_H */
