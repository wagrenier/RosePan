/* ==========================================================================
 *  snd/rxmpeg.h
 *
 *  The movie player (snd/rxmpeg.cpp). Minimal: only what reconstructed
 *  code calls so far.
 * ======================================================================== */
#ifndef _SND_RXMPEG_H
#define _SND_RXMPEG_H

/* Stores the V-blank count (data 0x298fd0); MagSystem's V-blank callback
 * passes it on every V-blank. */
void rxMpegSetVCount(int count);                                         // ROR 0x00182118

#endif /* _SND_RXMPEG_H */
