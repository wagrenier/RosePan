/* ==========================================================================
 *  libdbc.cpp  (SCE controller-port library -- PC-port shim)
 * ======================================================================== */

#include "libdbc.h"

static int s_dbcInit;

extern "C" {

int sceDbcInit(void)
{
    s_dbcInit = 1;
    return 1;
}

int sceDbcEnd(void)
{
    if (s_dbcInit == 0)
        return 0;
    s_dbcInit = 0;
    return 1;
}

}
