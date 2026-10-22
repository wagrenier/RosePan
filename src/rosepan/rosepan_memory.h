#ifndef ROSEPAN_MEMORY_H
#define ROSEPAN_MEMORY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void  RosePan_InitPs2Memory(void);
int   RosePan_TryGetHostPointer(uintptr_t address, void **host_ptr);
void *RosePan_GetHostPointer(uintptr_t address);
int   RosePan_IsPs2Address(uintptr_t address);

/* Inverse of RosePan_GetHostPointer: host pointer -> EE address, 0 if the
 * pointer is not inside the emulated RAM.  Needed by anything that has to
 * persist a pointer outside the process -- see the save-block fixups in
 * system/mc/prg/mc_set_data.c. */
uintptr_t RosePan_GetPs2Address(const void *host_ptr);

#ifdef __cplusplus
}
#endif

#endif /* ROSEPAN_MEMORY_H */
