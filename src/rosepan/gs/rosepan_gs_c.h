#ifndef ROSEPAN_GS_C_H
#define ROSEPAN_GS_C_H

#include "sce_gs.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void RosePan_GsClear(void);
void RosePan_GsUpload(sceGsLoadImage *image_load, unsigned char *image);
/* While non-zero, RosePan_GsUpload() drops what it is handed -- nothing is
 * written, invalidated or counted.  Set only around a model's own texture
 * sends once the host has resolved that model's textures (see
 * RosePan_Graph3dBeginTim2Upload()); every other upload is untouched. */
void RosePan_GsSetUploadSuppressed(int suppressed);
/* While set, called with every transfer RosePan_GsUpload() goes on to perform
 * -- the host texture binding uses it to learn exactly what a character's or
 * item's send writes.  NULL clears it. */
void RosePan_GsSetUploadObserver(void (*observer)(const sceGsLoadImage *image,
                                                 void *user),
                                void *user);
void RosePan_GsStore(sceGsStoreImage *sp, unsigned char *out);
unsigned char *RosePan_GsDownloadTexture(sceGsTex0 *tex0, uint64_t *hash);
uint64_t RosePan_GetTextureHash(sceGsTex0 *tex0);

void RosePan_GsResetFrameMetrics(void);
int RosePan_GsGetUploadCount(void);
uint64_t RosePan_GsGetUploadCountLive(void);
uint64_t RosePan_GsGetUploadBytesLive(void);
int RosePan_GsGetUploadBytes(void);
float RosePan_GsGetUploadMs(void);
int RosePan_GsGetDownloadCount(void);
int RosePan_GsGetDownloadBytes(void);
float RosePan_GsGetDownloadMs(void);

int RosePan_GsHasPendingUploads(void);
void RosePan_GsConsumePendingUploads(
    void (*cb)(int addr, int size, void *ud), void *ud);
void RosePan_GetTextureGsRegion(sceGsTex0 *tex0, int *out_addr, int *out_size);

#ifdef __cplusplus
}
#endif

#endif
