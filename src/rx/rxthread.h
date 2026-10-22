/* ==========================================================================
 *  rx/rxthread.h
 *
 *  The rx middleware's thread layer ("rxthread:" / "rxThread:" messages, ROR
 *  ~0x249000..0x24b490; extent not pinned yet): a dispatch semaphore used as
 *  a nesting count, a worker thread the main thread hands requests to on an
 *  alarm, and per-tick callbacks. Only what rxsys calls is declared so far.
 * ======================================================================== */
#ifndef _RX_RXTHREAD_H
#define _RX_RXTHREAD_H

typedef int (*RxThRequestFunc)(int a, int b, int c);

int  rxThIsDispatchDisabled(void);                              // ROR 0x00249d48
int  rxThAddCallback(void (*func)(void));                       // ROR 0x0024a258
void rxThYield(void);                                           // ROR 0x0024a508
void rxThDisableDispatch(void);                                 // ROR 0x0024a8c8
void rxThEnableDispatch(void);                                  // ROR 0x0024a958
void rxThRequest(RxThRequestFunc func, int a, int b, int c);    // ROR 0x0024ae60
void rxThInitOnce(void);                                        // ROR 0x0024b0b0

#endif /* _RX_RXTHREAD_H */
