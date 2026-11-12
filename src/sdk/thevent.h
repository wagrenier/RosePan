/* thevent.h  (IOP event flags -- PC-port shim; see iop_host.h) */
#ifndef _IOP_THEVENT_H
#define _IOP_THEVENT_H

#include "iop_host.h"

#define EventFlagParam      IopEventFlagParam

#define CreateEventFlag     RosePan_IopCreateEventFlag
#define SetEventFlag        RosePan_IopSetEventFlag
/* The interrupt-context form; there is no separate one on the host. */
#define iSetEventFlag       RosePan_IopSetEventFlag
#define ClearEventFlag      RosePan_IopClearEventFlag
#define WaitEventFlag       RosePan_IopWaitEventFlag

#endif /* _IOP_THEVENT_H */
