/* intrman.h  (IOP interrupt manager — PC-port shim; see iop_host.h) */
#ifndef _IOP_INTRMAN_H
#define _IOP_INTRMAN_H

#include "iop_host.h"

/* The three iopRpcLoop() unmasks: SPU2 core 0 / core 1 DMA completion, and
 * the SPU interrupt itself. */
#define IOP_IRQ_SPU         0x09
#define IOP_IRQ_DMA_SPU     0x24
#define IOP_IRQ_DMA_SPU2    0x28

#define EnableIntr          RosePan_IopEnableIntr
#define CpuSuspendIntr      RosePan_IopCpuSuspendIntr
#define CpuResumeIntr       RosePan_IopCpuResumeIntr

#endif /* _IOP_INTRMAN_H */
