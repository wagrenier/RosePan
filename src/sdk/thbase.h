/* thbase.h  (IOP thread manager — PC-port shim; see iop_host.h) */
#ifndef _IOP_THBASE_H
#define _IOP_THBASE_H

#include "iop_host.h"

/* TH_C — the only attribute iopsys.irx ever asks for. */
#define TH_C 0x02000000

/* The EE kernel shim declares a different struct under this name; see
 * iop_host.h. */
#define ThreadParam         IopThreadParam

/* The IOP SDK's names.  Redirected because CreateThread, ExitThread and
 * TerminateThread are Win32 API functions on this host. */
#define CreateThread        RosePan_IopCreateThread
#define DeleteThread        RosePan_IopDeleteThread
#define StartThread         RosePan_IopStartThread
#define ExitThread          RosePan_IopExitThread
#define TerminateThread     RosePan_IopTerminateThread
#define GetThreadId         RosePan_IopGetThreadId
#define ReferThreadStatus   RosePan_IopReferThreadStatus
#define SleepThread         RosePan_IopSleepThread
#define WakeupThread        RosePan_IopWakeupThread
#define iWakeupThread       RosePan_IopWakeupThread
#define DelayThread         RosePan_IopDelayThread
#define USec2SysClock       RosePan_IopUSec2SysClock

#endif /* _IOP_THBASE_H */
