/* sysmem.h  (IOP system memory — PC-port shim; see iop_host.h) */
#ifndef _IOP_SYSMEM_H
#define _IOP_SYSMEM_H

#include "iop_host.h"

#define AllocSysMemory          RosePan_IopAllocSysMemory
#define FreeSysMemory           RosePan_IopFreeSysMemory
#define QueryMemSize            RosePan_IopQueryMemSize
#define QueryMaxFreeMemSize     RosePan_IopQueryMaxFreeMemSize
#define QueryTotalFreeMemSize   RosePan_IopQueryTotalFreeMemSize

/* The IOP kernel's own console printf.  Only cdvd_callback() uses it. */
#define Kprintf printf

#endif /* _IOP_SYSMEM_H */
