/* ==========================================================================
 *  rx/rxthread.h
 *
 *  The rx middleware's thread layer ("rxthread:" / "rxThread:" messages, ROR
 *  ~0x249000..0x24b490; extent not pinned yet): a dispatch semaphore used as
 *  a nesting count, a worker thread the main thread hands requests to on an
 *  alarm, and per-tick callbacks. Only what rxsys and rxfile call is declared
 *  so far. An rx thread is an index into an 8-entry table (0x807f70).
 * ======================================================================== */
#ifndef _RX_RXTHREAD_H
#define _RX_RXTHREAD_H

typedef int (*RxThRequestFunc)(int a, int b, int c);

int  rxThIsDispatchDisabled(void);                              // ROR 0x00249d48
int  rxThAddCallback(void (*func)(void));                       // ROR 0x0024a258
void rxThYield(void);                                           // ROR 0x0024a508
void rxThDisableDispatch(void);                                 // ROR 0x0024a8c8
void rxThEnableDispatch(void);                                  // ROR 0x0024a958
int  rxThGetIndex(void);                                        // ROR 0x0024aae0
void rxThDelete(int index);                                     // ROR 0x0024ab40
void rxThSetPriority(int index, int priority);                  // ROR 0x0024ac28
void *rxThGetParam(int index);                                  // ROR 0x0024ac68
void rxThSetState(int index, short state);                      // ROR 0x0024acc8
int  rxThCreate(void (*entry)(void), void *stack, int stackSize,
                int priority, void *param);                     // ROR 0x0024ace8
void rxThRequest(RxThRequestFunc func, int a, int b, int c);    // ROR 0x0024ae60
void rxThInitOnce(void);                                        // ROR 0x0024b0b0
int  rxThSync(int mode);                                        // ROR 0x0024b3f8

#endif /* _RX_RXTHREAD_H */
