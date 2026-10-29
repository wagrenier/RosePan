/* ==========================================================================
 *  utl/utltm2.h
 *
 *  TIM2 helpers (utl/utltm2.cpp, ROR 0x104e1c..0x105650; the asserts
 *  from 0x104e64 on name the file): sizes, picture lookup and the sepia
 *  tint DogTexInfo applies to old photographs.
 *  A TIM2 file: a 0x10-byte header (0x80 when the format byte +5 is set),
 *  the picture count at +6, then the pictures, each starting with its
 *  total size; a picture's colour type is its byte +0x13.
 * ======================================================================== */
#ifndef _UTL_UTLTM2_H
#define _UTL_UTLTM2_H

struct I3dTim2Picture;

/* The whole file's size: the header and every picture. */
int  utlTm2GetSize(const void *tm2);                                     // ROR 0x00104e1c
/* Picture theN's header (asserts theN < h->Pictures; its colour type is
 * the byte +0x13). */
const I3dTim2Picture *utlTm2GetPicture(const void *tm2, int theN);       // ROR 0x00104e64
/* Rewrites the first picture's colours (CLUT or image, by colour type) as
 * their luminance (0x4d / 0x96 / 0x1d) times r, g, b >> 16 (DogTexInfo
 * passes 0xff 0xdc 0x98: sepia). */
void utlTm2ToSepia(void *tm2, int r, int g, int b);                      // ROR 0x0010559c

#endif /* _UTL_UTLTM2_H */
