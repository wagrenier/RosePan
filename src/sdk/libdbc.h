/* ==========================================================================
 *  sdk/libdbc.h  (SCE PS2 controller-port library, the layer under libpad2)
 *
 *  Rule of Rose's main brings it up and down around libpad2 (mainInitDevices
 *  / mainExitDevices). The host has no dbcman.irx to bind: Init reports
 *  success and End releases nothing.
 * ======================================================================== */

#ifndef _SDK_LIBDBC_H
#define _SDK_LIBDBC_H

#ifdef __cplusplus
extern "C" {
#endif

int sceDbcInit(void);           /* 1 when ready (the ROM's 0 on a bind or version error) */
int sceDbcEnd(void);            /* 1 when it was initialised */

#ifdef __cplusplus
}
#endif

#endif /* _SDK_LIBDBC_H */
