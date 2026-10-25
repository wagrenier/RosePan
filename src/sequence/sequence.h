/* ==========================================================================
 *  sequence/sequence.h
 *
 *  The event sequence system (sequence/sequence.cpp 0x20e3d8..0x20ff34 and
 *  the VM run after it, 0x20ff34..0x215e1c, still unassigned): one global
 *  at bss 0x7a4440 that loads the sequence images GamMan hands it and steps
 *  them every frame ("mst", "trg", "imdy", "single" thread kinds). The
 *  scripts are MIPS code the VM 0x214c24 interprets; their calls into the
 *  game go through the native table of sequence/seqcall.cpp.
 * ======================================================================== */
#ifndef _SEQUENCE_SEQUENCE_H
#define _SEQUENCE_SEQUENCE_H

#include <stdint.h>

#include "sdk/eetypes.h"

/* EE size at least 0xdc5c. layout: host -- only the members the native
 * functions touch, EE offsets in the comments. The constructor 0x20e454
 * (three node pools) arrives with the rest of sequence.cpp. */
struct Sequence
{
    void init();                                                         // ROR 0x0020e690
    void exec();                                                         // ROR 0x0020f98c
    /* Makes the VM continue at target, an address in the current image
     * (the native function then returns SEQ_RET_JUMP). */
    void jumpTo(intptr_t target);                                        // ROR 0x002113a4

    /* The base address of each loaded image (index = the VM's current
     * image, bss 0x7b4dd0); system call 0x1e sets one. */
    intptr_t m_base[5];         // ROR 0xdc08
    /* The global variables, 0x800 bytes (cleared with the bitset by
     * 0x20f1a8). Words read here: [0x17] (system call 0xf), [0x64] (set
     * by system call 0x35). */
    int     *m_global;          // ROR 0xdc1c
    /* A bitset of 0x800 bytes (word 0 kept by the clear): GamMan::addItem
     * sets the bit of the item's record. */
    u_int   *m_bits;            // ROR 0xdc20
    /* Bit 0 set by GamMan::enterMenu for the item / map menus; 0x100
     * picks the word 0x20f140 returns; 0x400 set by scenePlayStart. */
    u_int    m_flags;           // ROR 0xdc24
};

extern Sequence g_sequence;     /* bss 0x7a4440 */

/* Writes "SEQ" into the sequence data folder name (data 0x2d9910, 0x40
 * bytes); both arguments are unused in the US build. */
void seqSetLanguage(int lang, int arg);                                  // ROR 0x0020e3d8
/* The folder name seqSetLanguage writes. */
char *seqGetDataDir(void);                                               // ROR 0x0020e400
/* The global sequence's path: compiled (data/SEQ/G000.GRB) when kind is
 * non-zero, else its source (data/seq/seq_global/g000.seq). */
char *seqMakeGlobalPath(char *buf, int size, int kind);                  // ROR 0x0020e40c

/* data 0x2d9e18 -> "1.01": the number in main's version line (also read
 * by 0x1d6078). */
extern const char *g_versionNumber;

#endif /* _SEQUENCE_SEQUENCE_H */
