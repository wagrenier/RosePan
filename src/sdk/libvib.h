/* ==========================================================================
 *  sdk/libvib.h  (SCE PS2 vibration library, on a libpad2 socket)
 *
 *  BedPad::setVibration is the only user. The profile is one bit per
 *  actuator (bit 0 the small on/off motor, bit 1 the big 0..255 one); the
 *  data packs the selected actuators' values in that order, LSB first.
 *  The host shim lives in libpad2.cpp (it needs the sockets).
 * ======================================================================== */

#ifndef _SDK_LIBVIB_H
#define _SDK_LIBVIB_H

#ifdef __cplusplus
extern "C" {
#endif

int sceVibSetActParam(int socket, int profilesize, unsigned char *profile,
                      int datasize, unsigned char *data);

#ifdef __cplusplus
}
#endif

#endif /* _SDK_LIBVIB_H */
