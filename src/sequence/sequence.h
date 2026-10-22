/* ==========================================================================
 *  sequence/sequence.h
 *
 *  The event sequence system (sequence/sequence.cpp and the run after it,
 *  0x20eda4..0x215e1c, still unassigned): one global at bss 0x7a4440 that
 *  loads the sequence images GamMan hands it and steps them every frame
 *  ("mst", "trg", "imdy", "single" thread kinds). Minimal: only what main
 *  reaches is declared.
 * ======================================================================== */
#ifndef _SEQUENCE_SEQUENCE_H
#define _SEQUENCE_SEQUENCE_H

/* EE size at least 0xdc5c. layout: host -- no members reconstructed. */
struct Sequence
{
    void init();                                                         // ROR 0x0020e690
    void exec();                                                         // ROR 0x0020f98c
};

extern Sequence g_sequence;     /* bss 0x7a4440 */

/* Writes "SEQ" into the sequence data folder name (data 0x2d9910, 0x40
 * bytes); both arguments are unused in the US build. */
void seqSetLanguage(int lang, int arg);                                  // ROR 0x0020e3d8

/* data 0x2d9e18 -> "1.01": the number in main's version line (also read
 * by 0x1d6078). */
extern const char *g_versionNumber;

#endif /* _SEQUENCE_SEQUENCE_H */
