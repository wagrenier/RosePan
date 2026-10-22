/* ==========================================================================
 *  main.h
 *
 *  Rule of Rose's main and its file (main.cpp, ROR 0x2043dc..0x204d38: no
 *  source-path string; the static-init pair 0x20439c / 0x2043bc ends the
 *  file before it). main parses the command line, brings the rx middleware
 *  and the game heap up, loads the common data and the first stage behind
 *  the now-loading screen, then runs the frame loop for ever. The debug
 *  exception handler and its install / remove pair live here too.
 * ======================================================================== */
#ifndef _MAIN_H
#define _MAIN_H

#include "sdk/scetypes.h"

/* One row of mainExceptionHandler's register dump (data 0x2d9000, nine
 * rows and a NULL name). EE 0x10. layout: host -- the name is a pointer. */
struct MainRegRow
{
    const char *m_name;         // ROR 0x00: "\nat/v0-1    " ...; "" continues the line
    int         m_first;        // ROR 0x04: first GPR
    int         m_count;        // ROR 0x08: GPRs on this row
    int         m_pad;          // ROR 0x0c: always 0
};

int  mainParseArgs(int argc, char **argv);                               // ROR 0x002043dc
int  mainGame(int argc, char **argv);                                    // ROR 0x002045a8
void mainInitGame(int stage);                                            // ROR 0x002046d4
void mainExit(void);                                                     // ROR 0x0020483c
void mainLoop(void);                                                     // ROR 0x00204864
void mainInitGraph(void);                                                // ROR 0x0020491c
void mainInitDevices(void);                                              // ROR 0x0020499c
void mainExitDevices(void);                                              // ROR 0x002049d4
void mainExceptionHandler(u_int stat, u_int cause, u_int epc, u_int bva, u_int bpa,
                          u_long128 *gpr);                               // ROR 0x00204a00
void mainSetExceptionHandlers(void);                                     // ROR 0x00204bf8
void mainResetExceptionHandlers(void);                                   // ROR 0x00204ca4

#endif /* _MAIN_H */
