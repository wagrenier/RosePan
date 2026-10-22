/* ==========================================================================
 *  libscf.cpp  (SCE system-configuration library -- PC-port shim)
 * ======================================================================== */

#include "libscf.h"

extern "C" {

int sceScfGetLanguage(void)
{
    return SCE_ENGLISH_LANGUAGE;
}

/* sceCdReadClock already returns the host's local time: nothing to convert. */
void sceScfGetLocalTimefromRTC(sceCdCLOCK *rtc)
{
    (void)rtc;
}

}
