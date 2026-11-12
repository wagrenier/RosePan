/* thsemap.h  (IOP semaphore manager — PC-port shim; see iop_host.h) */
#ifndef _IOP_THSEMAP_H
#define _IOP_THSEMAP_H

#include "iop_host.h"

/* The EE kernel shim declares a different struct under this name; see
 * iop_host.h. */
#define SemaParam           IopSemaParam

#define CreateSema          RosePan_IopCreateSema
#define DeleteSema          RosePan_IopDeleteSema
#define SignalSema          RosePan_IopSignalSema
/* The interrupt-context form; there is no separate one on the host. */
#define iSignalSema         RosePan_IopSignalSema
#define WaitSema            RosePan_IopWaitSema
#define PollSema            RosePan_IopPollSema
#define ReferSemaStatus     RosePan_IopReferSemaStatus

#endif /* _IOP_THSEMAP_H */
