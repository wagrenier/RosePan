/* timrman.h  (IOP hardware timers — PC-port shim; see iop_host.h) */
#ifndef _IOP_TIMRMAN_H
#define _IOP_TIMRMAN_H

#include "iop_host.h"

#define AllocHardTimer      RosePan_IopAllocHardTimer
#define SetTimerHandler     RosePan_IopSetTimerHandler
#define SetupHardTimer      RosePan_IopSetupHardTimer
#define StartHardTimer      RosePan_IopStartHardTimer
#define StopHardTimer       RosePan_IopStopHardTimer

#endif /* _IOP_TIMRMAN_H */
