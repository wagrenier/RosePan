/* ==========================================================================
 *  sdk/libpad2.h  (SCE PS2 controller library 2)
 *
 *  Rule of Rose reads its pads through libpad2 (scePad2*), not FF2 / FF3's
 *  libpad. Only Init / End are declared so far, for main's device bring-up;
 *  the port and read calls arrive with the TU that uses them (the pad
 *  manager), backed by rosepan_input like libpad.
 * ======================================================================== */

#ifndef _SDK_LIBPAD2_H
#define _SDK_LIBPAD2_H

#ifdef __cplusplus
extern "C" {
#endif

int scePad2Init(int mode);      /* clears the 16 port records; 1 */
int scePad2End(void);           /* closes every open port; 1 */

#ifdef __cplusplus
}
#endif

#endif /* _SDK_LIBPAD2_H */
