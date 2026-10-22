/* ==========================================================================
 *  libpad2.cpp  (SCE controller library 2 -- PC-port shim)
 *
 *  No ports yet: the host keeps only the init flag the ROM's library keeps.
 * ======================================================================== */

#include "libpad2.h"

static int s_pad2Init;

extern "C" {

int scePad2Init(int mode)
{
    (void)mode;
    s_pad2Init = 1;
    return 1;
}

int scePad2End(void)
{
    s_pad2Init = 0;
    return 1;
}

}
